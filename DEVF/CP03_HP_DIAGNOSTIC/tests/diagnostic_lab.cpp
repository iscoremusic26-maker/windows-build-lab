#include "../src/registration.hpp"
#include "../src/observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
// Historical SDK unused-inline destructor link tripwire: test-only, never DLL.
extern "C" void GXDestroyTexObj(GXTexObj*){std::abort();}
namespace {
struct Events {
 devf::Observation list[64]{};unsigned n=0;
 void operator()(const devf::Observation& e) noexcept {if(n>=64)std::abort();list[n++]=e;}
};
// Pass a reference through a lambda; Observer never retains this callback.
devf::Snapshot own(unsigned id=147){devf::Snapshot s;s.id=id;s.profile=510;s.room=3;s.layer=6;s.set_id=65535;s.parameters=0xFFFF0100;
 s.x=devf::kX;s.y=devf::kY;s.z=devf::kZ;s.home_x=262;s.stage=true;s.initializing=true;s.resource_eoc2=true;s.health=220;return s;}
bool observations(const char* name){
 devf::Observer o;Events e;auto emit=[&](const devf::Observation& x)noexcept{e(x);};
 devf::Policy policy;policy.armed=true;auto s=own();unsigned writes=0;
 auto create=[&](unsigned id){s=own(id);if(policy.inspect(s)==devf::Decision::Applied){s.health=devf::kHp;++writes;o.start(s.id,s.health,emit);}};
 auto window=[&](int before,int after){o.push(emit);o.capture(s.id,before,emit);o.complete(s.id,after,true,emit);};
 if(!std::strcmp(name,"originals_silent")){
  for(int variant=0;variant<2;++variant){auto original=own();original.x+=1;original.health=variant?220:40;
   const int before=original.health;o.push(emit);if(policy.inspect(original)!=devf::Decision::Ignore)return false;
   o.complete(original.id,original.health,true,emit);if(original.health!=before)return false;}
  return e.n==0&&writes==0&&!o.active;
 }
 o.push(emit);create(147);o.capture(s.id,s.health,emit);o.complete(s.id,s.health,true,emit);
 if(!std::strcmp(name,"initial_baseline"))return writes==1&&s.health==150&&e.n==0&&o.last_hp==150;
 if(!std::strcmp(name,"hits_and_unchanged")){
  window(150,115);window(115,100);window(100,100);
  return e.n==2&&e.list[0].before==150&&e.list[0].after==115&&e.list[1].before==115&&e.list[1].after==100&&o.changes==2;
 }
 if(!std::strcmp(name,"zero_unknown")){
  window(150,20);window(20,0);
  return e.n==2&&e.list[1].before-e.list[1].after==20&&!std::strcmp(e.list[1].classification,"UNKNOWN_OR_SPECIAL")&&o.last_hp==0;
 }
 if(!std::strcmp(name,"readonly_increase")){
  window(150,115);s.initializing=false;s.health=115;if(policy.inspect(s)!=devf::Decision::Ignore)return false;
  window(115,130); // No extra write or claim about Iron Ball cause.
  return writes==1&&s.health==115&&e.n==2&&e.list[1].kind==devf::EventKind::Increase&&e.list[1].after-e.list[1].before==15;
 }
 if(!std::strcmp(name,"new_generation")){
  window(150,110);o.push(emit);create(581);o.capture(s.id,s.health,emit);o.complete(s.id,150,true,emit);window(150,120);
  return writes==2&&e.n==3&&e.list[1].kind==devf::EventKind::Summary&&e.list[1].id==147&&e.list[2].id==581&&e.list[2].ordinal==1;
 }
 if(!std::strcmp(name,"nested_ignored")){
  o.push(emit);o.capture(147,150,emit);o.push(emit);o.complete(900,40,true,emit);o.complete(147,120,true,emit);
  return e.n==1&&o.depth==0&&!o.fault&&e.list[0].after==120;
 }
 if(!std::strcmp(name,"reentrancy_failclosed")){
  o.push(emit);o.capture(147,150,emit);o.push(emit);o.capture(147,140,emit);o.complete(147,130,true,emit);o.complete(147,120,true,emit);
  return o.fault&&e.n==1&&e.list[0].kind==devf::EventKind::Rejected&&o.depth==0;
 }
 if(!std::strcmp(name,"depth_failclosed")){
  for(unsigned i=0;i<10;++i)o.push(emit);for(unsigned i=0;i<10;++i)o.complete(0,0,false,emit);
  return o.fault&&e.n==1&&o.depth==0;
 }
 if(!std::strcmp(name,"post_identity_failclosed")){
  o.push(emit);o.capture(147,150,emit);o.complete(148,0,false,emit);
  return o.fault&&e.n==1&&e.list[0].kind==devf::EventKind::Rejected;
 }
 if(!std::strcmp(name,"queued_deletion")){
  // Source contract model: delete enqueues; allocated remains true until AFTER post.
  bool allocated=true;int hp=150;o.push(emit);o.capture(147,hp,emit);hp=0;int state=3;
  if(!allocated||state!=3)return false;o.complete(147,hp,true,emit);allocated=false;
  return !allocated&&e.n==1&&e.list[0].after==0&&!o.fault;
 }
 if(!std::strcmp(name,"shutdown")){
  window(150,120);o.shutdown(emit);unsigned n=e.n;o.push(emit);o.complete(147,0,true,emit);o.shutdown(emit);
  return !o.active&&o.own_id==0&&o.depth==0&&e.n==n&&e.list[n-1].kind==devf::EventKind::Summary;
 }
 if(!std::strcmp(name,"cap_and_between")){
  window(140,135); // Outside execute loss10, then window loss5, separately labelled.
  if(e.n!=2||std::strcmp(e.list[0].source,"OBSERVED_BETWEEN_EXECUTES")||std::strcmp(e.list[1].source,"EXECUTE_WINDOW"))return false;
  int hp=135;for(int i=0;i<100;++i){window(hp,hp+1);++hp;}
  unsigned suppression=0;for(unsigned i=0;i<e.n;++i)if(e.list[i].kind==devf::EventKind::Suppressed)++suppression;
  return o.emitted==32&&suppression==1&&e.n==33&&o.changes==102&&o.last_hp==235;
 }
 return false;
}
int pre_calls=0,post_calls=0,adds=0,removed=0;bool pre_retained=false,post_retained=false,payload_ok=false;
int mode=0; // 1 pre-fail, 2 post-fail, 3 post-throw, 4 stage-fail, 5 zero-handle.
HookAction pre(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
void post(ModContext*,void*,void*,void*){}
ModResult install_pre(HookPreFn fn){++pre_calls;pre_retained=fn!=nullptr;return mode==1?MOD_UNAVAILABLE:MOD_OK;}
ModResult install_post(HookPostFn fn){++post_calls;post_retained=fn!=nullptr;if(mode==3)throw std::runtime_error("fake post");return mode==2?MOD_UNAVAILABLE:MOD_OK;}
ModResult remove_hook(){++removed;pre_retained=post_retained=false;return MOD_OK;}
ModResult add(ModContext*,const char* stage,uint8_t room,int8_t layer,const void* raw,size_t n,StageActorHandle* h){
 ++adds;const auto& a=*static_cast<const stage_actor_data_class*>(raw);const auto* b=static_cast<const unsigned char*>(raw);
 payload_ok=pre_retained&&post_retained&&!std::strcmp(stage,"F_SP121")&&room==3&&layer==6&&n==32&&
 std::memcmp(a.name,"E_oc\0\0\0\0",8)==0&&u32(a.base.parameters)==0xFFFF0100u&&float(a.base.position.x)==devf::kX&&
 float(a.base.position.y)==devf::kY&&float(a.base.position.z)==devf::kZ&&s16(a.base.angle.x)==262&&s16(a.base.angle.y)==0&&
 s16(a.base.angle.z)==0&&u16(a.base.setID)==0xFFFF&&b[8]==255&&b[9]==255&&b[10]==1&&b[11]==0&&b[24]==1&&b[25]==6;
 *h=mode==5?0:17;return mode==4?MOD_INVALID_ARGUMENT:MOD_OK;
}
bool registration(const char* name){
 HostService host{};host.header=SERVICE_HEADER(HostService,2,2);host.version="2.0.3";
 LogService log{};log.header=SERVICE_HEADER(LogService,1,0);log.write=[](ModContext*,LogLevel,const char*){};
 StageService stage{};stage.header=SERVICE_HEADER(StageService,1,0);stage.add_actor=add;
 HookService hook{};hook.header=SERVICE_HEADER(HookService,1,1);
 hook.install=[](ModContext*,void*,void*,void**){return MOD_OK;};
 hook.add_pre=[](ModContext*,void*,HookPreFn,const HookOptions*){return MOD_OK;};
 hook.add_post=[](ModContext*,void*,HookPostFn,const HookOptions*){return MOD_OK;};
 hook.dispatch_pre=[](ModContext*,void*,void*,void*,int*){return MOD_OK;};
 hook.dispatch_post=[](ModContext*,void*,void*,void*){return MOD_OK;};
 hook.uninstall=[](ModContext*,void*,void**){return MOD_OK;};
 int first=0,last=0;
 if(!std::strcmp(name,"post_partial_failures")){first=1;last=3;}
 if(!std::strcmp(name,"stage_partial_failures")){first=4;last=5;}
 const bool missing=!std::strcmp(name,"missing_post");if(missing)hook.add_post=nullptr;
 for(mode=first;mode<=last;++mode){
  pre_calls=post_calls=adds=removed=0;pre_retained=post_retained=payload_ok=false;devf::Registration r;
  auto result=r.initialize(reinterpret_cast<ModContext*>(1),&host,&log,&stage,&hook,install_pre,install_post,remove_hook,pre,post);
  const auto expected=missing?MOD_UNSUPPORTED:mode==0?MOD_OK:mode==3||mode==5?MOD_ERROR:mode==4?MOD_INVALID_ARGUMENT:MOD_UNAVAILABLE;
  if(result!=expected||r.ready!=(expected==MOD_OK)||adds!=(missing||mode>=1&&mode<=3?0:1)||adds&&!payload_ok)return false;
  if(expected!=MOD_OK&&(pre_retained||post_retained))return false;
  if(r.initialize(reinterpret_cast<ModContext*>(1),&host,&log,&stage,&hook,install_pre,install_post,remove_hook,pre,post)!=MOD_CONFLICT)return false;
  remove_hook();if(pre_retained||post_retained)return false; // Fake detach, not real backend.
 }
 return true;
}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 const bool reg=!std::strcmp(argv[1],"registration_pair_order")||!std::strcmp(argv[1],"post_partial_failures")||
  !std::strcmp(argv[1],"missing_post")||!std::strcmp(argv[1],"stage_partial_failures");
 const bool ok=reg?registration(argv[1]):observations(argv[1]);
 std::printf("%s TEST_FAKE diagnostic_case=%s game=NOT_EXECUTED DLL=NOT_LOADED backend=NOT_EXECUTED\n",ok?"PASS":"FAIL",argv[1]);return ok?0:1;
}
