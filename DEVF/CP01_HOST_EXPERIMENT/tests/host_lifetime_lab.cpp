// Actual patched functions; synthetic adapters deliberately do not represent engine ABI.
// Only LoadLibrary/FreeLibrary below exercise the real Windows OS on an owned fixture.
#include <mods/api.h>
#include <mods/svc/actor.h>
#include <mods/svc/log.h>
#include <Windows.h>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <utility>
struct ModContext {void* owner=nullptr;};
using s16=short; using s32=int; using BOOL=int;
namespace fs=std::filesystem;
constexpr int fpcNm_MAX_NUM=1000;
constexpr int cPhs_ERROR_e=5;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
namespace lab {
struct Logger {template<class... T>void warn(T...){} template<class... T>void error(T...){} template<class... T>void info(T...){} } Log;
namespace log {template<class... T>void write(T...){} }
struct Native {using Fn=ModResult(*)(ModError*); Fn fn_shutdown=nullptr; HMODULE handle=nullptr;
 ~Native(){if(handle)FreeLibrary(handle);} };
struct RuntimeService {ModResult(*deactivate)(ModContext*,ModContext*,ModError*)=nullptr;};
struct LoadedMod {
 bool active=true,initialized=true,initializationEntered=true,deactivationStarted=false,deactivationPending=false,teardownPoisoned=false;
 unsigned lifetimeCallbacks=0;bool servicesRegistered=true,enabledApplied=true,suspendedByProvider=false,nativeInPlace=true;
 struct {std::string id="owned-lab";} metadata;
 std::unique_ptr<ModContext> context=std::make_unique<ModContext>();
 std::unique_ptr<Native> native;
 struct Runtime{RuntimeService* service;ModContext* providerContext;};std::optional<Runtime> runtime;
 struct Edge{LoadedMod* mod;};std::vector<Edge> dependents;
 fs::path nativePath,nativeDir;std::string nativeDirUtf8;unsigned enabledSubscription=0;bool enabled=true;bool* cvarIsEnabled=&enabled;
};
struct node_class {node_class* next=nullptr;};
#define NODE_GET_NEXT(node) ((node)->next)
struct create_tag_class{node_class base;void* mpTagData=nullptr;};
struct create_tag {struct {node_class node;void* mpTagData=nullptr;} base;};
struct fopAc_ac_c{ s16 name=2000;};struct base_process_class{s16 name=2000;};
struct delete_tag_class{struct {node_class node;void* mpTagData=nullptr;}base;int timer=0;};
struct Queue{node_class* mpHead=nullptr;};Queue g_fopAcTg_Queue,g_fpcDtTg_Queue,g_fpcCtTg_Queue;
using fpcCtRq_phase_handler=int(*)(void*);
struct Methods{fpcCtRq_phase_handler phase_handler;};
struct create_request{create_tag base;Methods* methods=nullptr;bool is_cancel=false;};
struct standard_create_request_class{create_request base;s16 process_name=2000;void* mod_owner=nullptr;};
int fpcSCtRq_Handler(void*){return 0;}
struct ActorSlot{bool forceDelete=false;unsigned callbacks=0;int(*deleteFunction)(void*)=nullptr;int(*createFunction)(void*)=nullptr;int(*executeFunction)(void*)=nullptr;int(*drawFunction)(void*)=nullptr;int(*isDeleteFunction)(void*)=nullptr;
 struct{struct{struct{s16 name=2000;}base;}base;}profile;struct{const char* name="labactr";}objNameInf;};
struct Entry{LoadedMod* owner;std::unique_ptr<ActorSlot> value=std::make_unique<ActorSlot>();};
struct Slots{
 std::unordered_map<ActorHandle,std::unique_ptr<Entry>> entries;
 Entry* find(ActorHandle h){auto it=entries.find(h);return it==entries.end()?nullptr:it->second.get();}
 Entry* find_owned(ActorHandle h,LoadedMod& o){auto* e=find(h);return e&&e->owner==&o?e:nullptr;}
 void erase_owned(ActorHandle h,LoadedMod& o){if(find_owned(h,o))entries.erase(h);}
 template<class F>void for_each(F f){for(auto&[h,e]:entries)f(h,*e);}
} s_slots;
thread_local LoadedMod* s_requestOwner=nullptr;thread_local bool s_acceptedRequest=false;
std::thread::id s_actorThread=std::this_thread::get_id();s32 s_nextProfileName=fpcNm_MAX_NUM;
std::unordered_map<s16,ActorHandle> procNameToHandle;
LoadedMod* mod_from_context(ModContext* c){return c?static_cast<LoadedMod*>(c->owner):nullptr;}
ActorSlot* get_actor_slot(void*){auto* e=s_slots.find(42);return e?e->value.get():nullptr;}
namespace dusk::mods {using LoadedMod=::lab::LoadedMod;}
LoadedMod* owner_of_slot(ActorSlot*);
#include "scopes_exact.inc"
#include "ownership_exact.inc"
namespace dusk::mods::svc::actor_impl {using ::lab::profile_owner;}
#include "pending_exact.inc"
int delete_result=0,finish_result=0;bool remove_on_finish=false;
int fopAcM_delete(fopAc_ac_c*){return delete_result;}
int fpcDt_deleteMethod(base_process_class*){return finish_result;}
template<class F>int fpcDtTg_Do(delete_tag_class* t,F fn){int result=fn(t->base.mpTagData);if(result&&remove_on_finish){g_fpcDtTg_Queue={};g_fopAcTg_Queue={};}return result;}
void remove_handle_from_maps(ActorHandle h){auto* e=s_slots.find(h);if(e)procNameToHandle.erase(e->value->profile.base.base.name);}
#include "actor_exact.inc"
int prehooks=0,detaches=0,services_removed=0,registry_shutdown=0,subscriptions_removed=0;bool prethrow=false,detachthrow=false;
namespace svc {
 namespace actor_impl {using ::lab::CallbackScope;}
 void modules_mod_deactivating(LoadedMod&){++prehooks;if(prethrow)throw std::runtime_error("prehook");}
 bool modules_mod_drained(LoadedMod& m){return actor_drain_mod(m);}
 void modules_mod_detached(LoadedMod& m){++detaches;if(detachthrow)throw std::runtime_error("detach");actor_remove_mod(m);}
 void remove_services_for_provider(LoadedMod&){++services_removed;}
 void modules_shutdown(){++registry_shutdown;}
}
namespace config{void unsubscribe(unsigned){++subscriptions_removed;}}
void unregister(bool&){}
std::string lifecycle_error_message(const char*,ModResult,ModError&){return "fixture failure";}
struct ModLoader {
 struct RetiredNative{std::unique_ptr<Native> native;fs::path directory;};
 std::vector<std::unique_ptr<LoadedMod>> m_mods;std::vector<RetiredNative> m_retiredNatives;bool m_terminalRetained=false;
 auto mods() const {return m_mods|std::views::transform([](const auto& m)->LoadedMod&{return *m;});}
 bool deactivate_mod(LoadedMod&);std::vector<LoadedMod*> collect_lifecycle_set(LoadedMod&)const;
 void unload_native(LoadedMod&);void drain_retired_natives();void shutdown();
};
#include "native_exact.inc"
#include "loader_exact.inc"
void reset(){s_slots.entries.clear();procNameToHandle.clear();g_fopAcTg_Queue={};g_fpcDtTg_Queue={};g_fpcCtTg_Queue={};s_requestOwner=nullptr;s_acceptedRequest=false;
 prehooks=detaches=services_removed=registry_shutdown=subscriptions_removed=0;prethrow=detachthrow=false;delete_result=finish_result=0;remove_on_finish=false;}
ModContext* ctx(LoadedMod& m){m.context->owner=&m;return m.context.get();}
void slot(LoadedMod& m){ctx(m);auto e=std::make_unique<Entry>();e->owner=&m;e->value->deleteFunction=[](void*){return 0;};e->value->createFunction=e->value->executeFunction=e->value->drawFunction=e->value->isDeleteFunction=[](void*){return 7;};s_slots.entries[42]=std::move(e);procNameToHandle[2000]=42;}
void queue(standard_create_request_class& r,LoadedMod& m){static Methods methods{fpcSCtRq_Handler};r.base.methods=&methods;r.base.base.base.mpTagData=&r.base;r.mod_owner=&m;g_fpcCtTg_Queue.mpHead=&r.base.base.base.node;}
ModResult success(ModError*){return MOD_OK;}int shutdown_calls=0;ModResult tracked_success(ModError*){++shutdown_calls;return MOD_OK;}
void native(LoadedMod& m,Native::Fn fn=success){m.native=std::make_unique<Native>();m.native->fn_shutdown=fn;ctx(m);}
}
int cases=0;template<class F>void test(const char* name,F f){lab::reset();f();++cases;std::printf("PASS %s\n",name);}
int main(int argc,char**argv){try {using namespace lab;check(argc==2,"fixture path required");
 test("pending_before_first_callback_blocks_profile_and_unload",[]{LoadedMod m;slot(m);native(m);standard_create_request_class r;queue(r,m);ModLoader l;
 check(!l.deactivate_mod(m)&&m.native&&m.context&&s_slots.find(42)&&detaches==0,"pre-callback ownership lost");check(!creation_allowed(2000),"closing profile accepted creation");
 g_fpcCtTg_Queue={};check(l.deactivate_mod(m)&&!m.native&&!s_slots.find(42),"drained request cannot retry");});
 test("requester_and_profile_owner_both_retained",[]{LoadedMod requester,provider;slot(provider);standard_create_request_class r;queue(r,requester);check(fpcSCtRq_HasPendingForOwner(&requester)&&fpcSCtRq_HasPendingForOwner(&provider),"cross-owner pending lost");});
 test("closing_profile_owner_blocks_other_requester_before_predrain",[]{LoadedMod a,b;slot(b);b.deactivationPending=true;b.lifetimeCallbacks=1;s_requestOwner=&a;check(!creation_allowed(2000),"other owner bypassed closing profile before predain");});
 test("opaque_request_conservatively_blocks",[]{LoadedMod m;standard_create_request_class r;r.base.base.base.mpTagData=&r.base;g_fpcCtTg_Queue.mpHead=&r.base.base.base.node;check(fpcSCtRq_HasPendingForOwner(&m),"opaque request cast");});
 test("active_actor_blocks_unregister",[]{LoadedMod m;slot(m);fopAc_ac_c actor;create_tag_class tag;tag.mpTagData=&actor;g_fopAcTg_Queue.mpHead=&tag.base;check(unregister_actor(ctx(m),42)==MOD_UNAVAILABLE&&s_slots.find(42)->value->forceDelete,"active actor removed profile");});
 test("force_delete_preserves_failure",[]{LoadedMod m;slot(m);s_slots.find(42)->value->forceDelete=true;check(actor_delete(nullptr)==0&&m.lifetimeCallbacks==0&&s_slots.find(42)->value->callbacks==0,"callback failure overridden");});
 test("delete_queue_requires_actual_removal",[]{LoadedMod m;slot(m);base_process_class actor;delete_tag_class tag;tag.base.mpTagData=&actor;g_fpcDtTg_Queue.mpHead=&tag.base.node;finish_result=1;check(!finish_delete_all_actors_of_name(2000),"success value treated as actual removal");remove_on_finish=true;check(finish_delete_all_actors_of_name(2000),"completed deletion cannot drain");});
 test("failed_delete_retains_then_retry",[]{LoadedMod m;slot(m);native(m);base_process_class actor;delete_tag_class tag;tag.base.mpTagData=&actor;g_fpcDtTg_Queue.mpHead=&tag.base.node;ModLoader l;check(!l.deactivate_mod(m)&&m.native&&s_slots.find(42),"failed deletion released owner");finish_result=1;remove_on_finish=true;check(l.deactivate_mod(m)&&!m.native&&prehooks==1,"retry did not preserve one prehook");});
 test("callback_reentrancy_blocks_unregister",[]{LoadedMod m;slot(m);s_slots.find(42)->value->deleteFunction=[](void* p){auto& o=*static_cast<LoadedMod*>(p);check(unregister_actor(ctx(o),42)==MOD_UNAVAILABLE,"profile freed inside own callback");return 0;};check(actor_delete(&m)==0&&s_slots.find(42),"callback slot lost");});
 test("callback_on_stack_blocks_teardown",[]{LoadedMod m;native(m);m.lifetimeCallbacks=1;ModLoader l;check(!l.deactivate_mod(m)&&m.native&&prehooks==0,"callback on stack unloaded");m.lifetimeCallbacks=0;check(l.deactivate_mod(m),"callback drain retry failed");});
 test("original_profiles_allowed_closing_custom_denied",[]{LoadedMod m;slot(m);check(creation_allowed(50)&&creation_allowed(2000),"open creation changed");s_slots.find(42)->value->forceDelete=true;check(creation_allowed(50)&&!creation_allowed(2000),"retail profile affected");s_requestOwner=&m;m.deactivationPending=true;check(!creation_allowed(50),"closing requester created retail actor");});
 test("accepted_request_allocation_only_can_drain",[]{LoadedMod m;slot(m);m.deactivationPending=true;check(!creation_allowed(2000),"new request accepted");s_acceptedRequest=true;check(creation_allowed(2000),"accepted allocation blocked");s_acceptedRequest=false;check(!creation_allowed(2000),"allocation permit leaked");});
 test("profile_names_never_reused_and_exhaustion_blocks",[]{s_nextProfileName=fpcNm_MAX_NUM;auto a=allocate_profile_name();auto b=allocate_profile_name();check(b==a+1,"name reuse");s_nextProfileName=0x7ffe;check(allocate_profile_name()==0x7ffe&&allocate_profile_name()==-1&&allocate_profile_name()==-1,"name exhaustion wrapped");});
 test("cancel_marked_request_blocks_until_actual_queue_removal",[]{LoadedMod m;slot(m);standard_create_request_class r;queue(r,m);r.base.is_cancel=true;check(fpcSCtRq_HasPendingForOwner(&m),"cancel flag treated as completion");g_fpcCtTg_Queue={};check(!fpcSCtRq_HasPendingForOwner(&m),"removed request still pending");});
 test("exact_callback_scopes_nested_exception_restore",[]{LoadedMod a,b;s_requestOwner=&a;try {CallbackScope one(&a);check(a.lifetimeCallbacks==1,"owner callback not counted");{CallbackScope two(&b);check(a.lifetimeCallbacks==1&&b.lifetimeCallbacks==1&&s_requestOwner==&b,"nested owner lost");}check(s_requestOwner==&a&&b.lifetimeCallbacks==0,"nested scope did not restore");throw std::runtime_error("controlled");}catch(const std::exception&){}check(a.lifetimeCallbacks==0&&s_requestOwner==&a,"exception scope leaked");});
 test("exact_accepted_scope_exception_restores_gate",[]{LoadedMod m;slot(m);m.deactivationPending=true;try {AcceptedRequestScope permit;check(creation_allowed(2000),"accepted allocation denied");throw std::runtime_error("controlled");}catch(const std::exception&){}check(!s_acceptedRequest&&!creation_allowed(2000),"allocation permit escaped scope");});
 test("closing_profile_suppresses_execute_draw_but_drains_create",[]{LoadedMod m;slot(m);s_slots.find(42)->value->forceDelete=true;check(actor_execute(nullptr)==1&&actor_draw(nullptr)==1&&actor_is_delete(nullptr)==1,"closing callback invoked");check(actor_create(nullptr)==7&&m.lifetimeCallbacks==0&&s_slots.find(42)->value->callbacks==0,"accepted create did not drain");});
 test("pending_context_blocks_provider_collection",[]{ModLoader l;auto p=std::make_unique<LoadedMod>();auto d=std::make_unique<LoadedMod>();p->dependents.push_back({d.get()});d->active=false;d->deactivationPending=true;l.m_mods.push_back(std::move(p));l.m_mods.push_back(std::move(d));auto set=l.collect_lifecycle_set(*l.m_mods[0]);check(set.size()==2,"retained dependent omitted");});
 test("failed_initialization_poison_retains",[]{LoadedMod m;native(m);m.initialized=false;m.teardownPoisoned=true;ModLoader l;check(!l.deactivate_mod(m)&&m.native&&m.context&&prehooks==0,"failed init reclaimed");});
 test("prehook_exception_retains_without_repeat",[]{LoadedMod m;native(m);prethrow=true;ModLoader l;check(!l.deactivate_mod(m)&&m.teardownPoisoned&&m.native,"throw freed DLL");prethrow=false;check(!l.deactivate_mod(m)&&prehooks==1,"uncertain hook retried");});
 test("detach_exception_retains_no_double_shutdown",[]{LoadedMod m;native(m,tracked_success);shutdown_calls=0;detachthrow=true;ModLoader l;check(!l.deactivate_mod(m)&&m.native&&m.teardownPoisoned&&shutdown_calls==1,"detach throw freed DLL");check(!l.deactivate_mod(m)&&shutdown_calls==1,"shutdown repeated after success");});
 test("shutdown_incomplete_real_windows_fixture_retained",[&]{LoadedMod m;slot(m);native(m);auto path=fs::absolute(argv[1]);m.native->handle=LoadLibraryW(path.c_str());check(m.native->handle!=nullptr,"LoadLibrary failed");m.native->fn_shutdown=reinterpret_cast<Native::Fn>(GetProcAddress(m.native->handle,"mod_shutdown"));check(m.native->fn_shutdown!=nullptr,"shutdown export missing");ModLoader l;
 check(!l.deactivate_mod(m)&&m.native&&m.context&&s_slots.find(42)&&detaches==0&&l.m_retiredNatives.empty(),"failed shutdown lost profile/context/code");check(GetModuleHandleW(path.filename().c_str())!=nullptr,"fixture unloaded on failure");
 m.native->fn_shutdown=success;check(l.deactivate_mod(m)&&!m.native&&l.m_retiredNatives.size()==1&&!s_slots.find(42),"successful shutdown did not retire");check(GetModuleHandleW(path.filename().c_str())!=nullptr,"retirement freed too early");l.drain_retired_natives();check(GetModuleHandleW(path.filename().c_str())==nullptr,"fixture FreeLibrary not completed");});
 test("successful_shutdown_idempotent",[]{LoadedMod m;native(m,tracked_success);shutdown_calls=0;ModLoader l;check(l.deactivate_mod(m)&&l.deactivate_mod(m)&&shutdown_calls==1,"shutdown called twice");});
 test("normal_shutdown_drains_registry_after_contexts",[]{ModLoader l;auto m=std::make_unique<LoadedMod>();native(*m);l.m_mods.push_back(std::move(m));l.shutdown();check(l.m_mods.empty()&&registry_shutdown==1&&l.m_retiredNatives.empty(),"normal terminal order failed");});
 test("blocked_terminal_quarantine_is_idempotent",[]{ModLoader l;auto m=std::make_unique<LoadedMod>();native(*m);m->teardownPoisoned=true;auto* retained=m.get();l.m_mods.push_back(std::move(m));l.shutdown();check(l.m_terminalRetained&&retained->native&&retained->context&&registry_shutdown==0&&l.m_mods.empty(),"terminal owner lost");l.shutdown();check(registry_shutdown==0&&retained->native,"repeat shutdown cleared retained registry");});
 std::printf("HOST_EXPERIMENT_PASS cases=%d platform=Windows_MSVC synthetic_engine=true engine_runtime=NOT_TESTED DX12=NOT_TESTED\n",cases);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
