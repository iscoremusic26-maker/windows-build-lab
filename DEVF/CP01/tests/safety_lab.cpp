#include "controller.hpp"
#include "lifecycle.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

static void check(bool condition, const char* message) { if(!condition) throw std::runtime_error(message); }
// Queue/slot substitutes, NOT game structures or an ABI adapter. Exact upstream function bodies
// are compiled below; all underlying engine, heap and queue operations are explicitly simulated.
namespace upstream_audit {
using s16=short;
struct LoadedMod {};
struct node_class { node_class* next=nullptr; };
#define NODE_GET_NEXT(node) ((node)->next)
struct fopAc_ac_c { s16 name=2000; };
struct create_tag_class { node_class base; void* mpTagData=nullptr; };
struct base_process_class { s16 name=2000; };
struct delete_tag_class { struct { node_class node; void* mpTagData=nullptr; } base; int timer=0; };
struct Queue { node_class* mpHead=nullptr; };
Queue g_fopAcTg_Queue,g_fpcDtTg_Queue;
struct ActorSlot { bool forceDelete=false; int (*deleteFunction)(void*)=nullptr;
 struct { struct { struct { s16 name=2000; } base; } base; } profile;
 struct { const char* name="dvfrock"; } objNameInf;
};
struct Entry { std::unique_ptr<ActorSlot> value=std::make_unique<ActorSlot>(); };
struct Slots {
 Entry entry; bool present=true;
 Entry* find_owned(ActorHandle,LoadedMod&) { return present?&entry:nullptr; }
 void erase_owned(ActorHandle,LoadedMod&) { present=false; }
} s_slots;
LoadedMod owner;
LoadedMod* mod_from_context(ModContext* ctx) { return ctx?&owner:nullptr; }
ActorSlot* get_actor_slot(void*) { return s_slots.present?s_slots.entry.value.get():nullptr; }
int delete_request_result=0;
int fopAcM_delete(fopAc_ac_c*) { return delete_request_result; }
template<class Function> int fpcDtTg_Do(delete_tag_class*,Function) { return 0; }
int fpcDt_deleteMethod(base_process_class*) { return 0; }
void remove_handle_from_maps(ActorHandle) {}
struct Logger { template<class... Args> void warn(Args...) {} } Log;
#include "host_cleanup_exact.inc"
void reset() { s_slots.present=true;s_slots.entry.value=std::make_unique<ActorSlot>();g_fopAcTg_Queue={};g_fpcDtTg_Queue={};delete_request_result=0; }
}
using namespace devf::cp01;
struct Ops {
 static constexpr int complete_step=4,error_step=5,cancelled_step=3;
 int phase=4,loads=0;bool release_ok=false;
 std::vector<std::string> events;
 int load_archive(){++loads;return phase;}
 bool create_heap(){return true;} bool register_collision(){return true;} bool initialize_scene(){return true;}
 bool release_collision(){events.push_back("collision");return release_ok;}
 void release_archive(){events.push_back("archive");}
 void clear_heap_references(){events.push_back("clear");}
};
struct ServiceFixture {
 ActorService actor{};StageService stage{};int registrations=0,spawns=0;
 ServiceFixture(){
  actor.register_actor=[](ModContext* ctx,const ActorProfileDesc*,ProfileName* n,ActorHandle* h){auto& f=*reinterpret_cast<ServiceFixture*>(ctx);++f.registrations;*n=2000;*h=42;return MOD_OK;};
  actor.unregister_actor=[](ModContext*,ActorHandle){return MOD_OK;};
  actor.create_actor=[](ModContext*ctx,ProfileName,const ActorSpawnParams*,ActorId*id){++reinterpret_cast<ServiceFixture*>(ctx)->spawns;*id=1;return MOD_OK;};
  stage.add_actor=[](ModContext*,const char*,uint8_t,int8_t,const void*,size_t,StageActorHandle*){return MOD_OK;};
  stage.remove_actor_edit=[](ModContext*,StageActorHandle){return MOD_OK;};
 }
 Controller controller(){return Controller(reinterpret_cast<ModContext*>(this),actor,stage);}
};
static ActorProfileDesc profile(){return {.name="dvfrock"};}
static int reject_delete(void*){return 0;}
static int custom_post_create(void*){return 1;}
static int cases=0;
template<class Function> void test(const char* name,Function f){f();++cases;std::printf("PASS %s\n",name);}
int main(int argc,char**argv){try{
 check(argc==3,"two owned Windows fixture DLL paths required");
 test("guard_closed_missing_each_requirement_no_service_calls",[]{
  for(int missing=0;missing<4;++missing){ServiceFixture f;auto c=f.controller();ActivationRequirements r{true,true,true,true};
   if(missing==0)r.location_verified=false;if(missing==1)r.pending_requests_drained=false;
   if(missing==2)r.collision_teardown_verified=false;if(missing==3)r.host_unload_barrier=false;
   check(c.start_guarded(profile(),nullptr,r)==MOD_UNSUPPORTED && f.registrations==0 && !c.registered(),"closed guard registered profile");}
 });
 test("guard_default_and_repeat_shutdown_no_ownership",[]{ServiceFixture f;auto c=f.controller();ActivationRequirements r;
  check(c.start_guarded(profile(),nullptr,r)==MOD_UNSUPPORTED,"default opened");
  check(c.stop(true)==MOD_OK && c.stop()==MOD_OK && f.registrations==0,"disarmed shutdown owns handles");});
 test("collision_release_failure_retains_archive_and_refs",[]{ResourceLifecycle s;Ops o;check(s.advance(o)==4,"setup failed");
  check(!s.release(o) && s.ready(),"failed cleanup erased ownership");check(o.events==std::vector<std::string>{"collision"},"archive freed before collision removed");});
 test("collision_release_retry_and_idempotence",[]{ResourceLifecycle s;Ops o;check(s.advance(o)==4,"setup failed");
  check(!s.release(o),"failure swallowed");o.release_ok=true;check(s.release(o)&&s.release(o),"retry failed");
  check(o.events==std::vector<std::string>{"collision","collision","archive","clear"},"double free or ordering changed");});
 test("cancelled_phase_terminal_no_more_creation",[]{ResourceLifecycle s;Ops o;o.phase=3;
  check(s.advance(o)==3 && s.advance(o)==5 && o.loads==1,"cancelled request retried");
  check(s.release(o)&&o.events==std::vector<std::string>{"archive","clear"},"cancelled cleanup incorrect");});
 test("runtime_post_callback_rejected_before_request",[]{ServiceFixture f;auto c=f.controller();check(c.start(profile(),nullptr)==MOD_OK,"setup failed");
  ActorSpawnParams p{};p.room_num=0;p.scale={1,1,1};p.create_function=custom_post_create;ActorId id=0;
  check(c.request_runtime(p,&id)==MOD_UNSUPPORTED && f.spawns==0,"unowned extra callback queued");check(c.stop()==MOD_OK,"cleanup failed");});
 test("exact_upstream_unregister_does_not_drain_pre_callback_requests",[]{using namespace upstream_audit;reset();int pending_requests=1;
  check(unregister_actor(reinterpret_cast<ModContext*>(&owner),42)==MOD_OK && !s_slots.present && pending_requests==1,"upstream coverage unexpectedly changed");});
 test("exact_upstream_forced_delete_masks_callback_failure",[]{using namespace upstream_audit;reset();s_slots.entry.value->deleteFunction=reject_delete;
  check(actor_delete(nullptr)==0,"ordinary delete result lost");s_slots.entry.value->forceDelete=true;
  check(actor_delete(nullptr)==1,"upstream force-delete override changed");});
 test("exact_upstream_unavailable_keeps_slot_but_has_no_unload_veto",[]{using namespace upstream_audit;reset();fopAc_ac_c actor;create_tag_class tag;tag.mpTagData=&actor;g_fopAcTg_Queue.mpHead=&tag.base;
  check(unregister_actor(reinterpret_cast<ModContext*>(&owner),42)==MOD_UNAVAILABLE && s_slots.present && !s_slots.entry.value->forceDelete,"failed unregister released slot");});
 test("real_windows_free_library_ignores_failed_shutdown",[&]{
  auto path=std::filesystem::absolute(argv[1]);HMODULE module=LoadLibraryW(path.c_str());check(module!=nullptr,"fixture DLL load failed");
  auto shutdown=reinterpret_cast<ModShutdownFn>(GetProcAddress(module,"mod_shutdown"));check(shutdown && shutdown(nullptr)==MOD_UNAVAILABLE,"fixture failure missing");
  check(FreeLibrary(module)!=0,"FreeLibrary failed");check(GetModuleHandleW(path.filename().c_str())==nullptr,"ordinary fixture not unloaded");
  // Never call the now-stale shutdown/callback pointer.
 });
 test("real_windows_pin_keeps_code_only_not_host_slot",[&]{
  auto path=std::filesystem::absolute(argv[2]);HMODULE module=LoadLibraryW(path.c_str());check(module!=nullptr,"pin fixture load failed");
  auto callback=GetProcAddress(module,"fixture_callback");HMODULE pinned=nullptr;
  check(callback && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(callback),&pinned),"pin failed");
  check(FreeLibrary(module) && GetModuleHandleW(path.filename().c_str())!=nullptr,"pinned code unloaded");
  check(reinterpret_cast<int(*)()>(callback)()==17,"pinned own fixture code unavailable");
  using namespace upstream_audit;reset();check(unregister_actor(reinterpret_cast<ModContext*>(&owner),42)==MOD_OK && !s_slots.present,"pin preserved host data unexpectedly");
  // Pin applies only to this owned lab DLL and is released by process exit; not used in the mod.
 });
 std::printf("CP01_SAFETY_PASS cases=%d platform=Windows exact_upstream_functions=5 engine_runtime=NOT_TESTED\n",cases);return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
