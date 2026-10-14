#include "../src/registration.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
// Historical SDK unreferenced-inline destructor tripwire, test-only, never production.
extern "C" void GXDestroyTexObj(GXTexObj*){std::abort();}
namespace {
int installs=0,adds=0,removes=0;bool retained=false,payload=false;const char* scenario="";
HookAction callback(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
ModResult install(HookPreFn fn){++installs;retained=fn!=nullptr;if(!std::strcmp(scenario,"install_throw"))throw std::runtime_error("fake");return !std::strcmp(scenario,"install_fail")?MOD_UNAVAILABLE:MOD_OK;}
ModResult remove(){++removes;retained=false;return MOD_OK;}
ModResult sdk_install(ModContext*,void*,void*,void**){return MOD_OK;}
ModResult sdk_pre(ModContext*,void*,HookPreFn,const HookOptions*){return MOD_OK;}
ModResult sdk_dispatch(ModContext*,void*,void*,void*,int*){return MOD_OK;}
ModResult sdk_post(ModContext*,void*,void*,void*){return MOD_OK;}
ModResult sdk_remove(ModContext*,void*,void**){return MOD_OK;}
ModResult add(ModContext*,const char* stage,uint8_t room,int8_t layer,const void* raw,size_t n,StageActorHandle* h){
 ++adds;const auto& a=*static_cast<const stage_actor_data_class*>(raw);const auto* b=static_cast<const unsigned char*>(raw);
 payload=!std::strcmp(stage,"F_SP121")&&room==3&&layer==6&&n==32&&std::memcmp(a.name,"E_oc\0\0\0\0",8)==0&&
 u32(a.base.parameters)==0xFFFF0100u&&float(a.base.position.x)==devf::kX&&float(a.base.position.y)==devf::kY&&float(a.base.position.z)==devf::kZ&&
 s16(a.base.angle.x)==262&&s16(a.base.angle.y)==0&&s16(a.base.angle.z)==0&&u16(a.base.setID)==0xFFFF&&
 b[8]==255&&b[9]==255&&b[10]==1&&b[11]==0&&b[24]==1&&b[25]==6;
 *h=!std::strcmp(scenario,"zero_handle")?0:17;
 return !std::strcmp(scenario,"stage_fail")?MOD_INVALID_ARGUMENT:MOD_OK;
}
devf::Snapshot own(unsigned id=7){devf::Snapshot s;s.id=id;s.profile=510;s.room=3;s.layer=6;s.set_id=65535;s.parameters=0xFFFF0100;
 s.x=devf::kX;s.y=devf::kY;s.z=devf::kZ;s.home_x=262;s.stage=true;s.initializing=true;s.resource_eoc2=true;s.health=220;return s;}
bool policy_case(){
 devf::Policy p;p.armed=true;auto s=own();
 if(!std::strcmp(scenario,"timing_and_damage")){
  int writes=0,action_hp=0;auto fake_pre=[&](){if(p.inspect(s)==devf::Decision::Applied){s.health=devf::kHp;++writes;}};
  // TEST_FAKE real source order: native init220 -> pre-hook -> original action.
  fake_pre();action_hp=s.health;s.initializing=false;s.health-=35;fake_pre();
  return writes==1&&action_hp==150&&s.health==115&&!p.fault;
 }
 if(!std::strcmp(scenario,"originals")){
  for(int which=0;which<9;++which){auto x=own();
   if(which==0){x.parameters=0xFFFF0000;x.health=40;}
   if(which==1)x.x+=1;if(which==2)x.room=4;if(which==3)x.layer=5;if(which==4)x.parameters^=1;
   if(which==5)x.set_id=3;if(which==6)x.profile=511;if(which==7)x.stage=false;if(which==8)x.home_x=6;
   const auto before=x.health;if(p.inspect(x)!=devf::Decision::Ignore||x.health!=before||p.last_id)return false;
  }return !p.fault;
 }
 if(!std::strcmp(scenario,"room_reload")){
  if(p.inspect(s)!=devf::Decision::Applied)return false;
  s=own(8);return p.inspect(s)==devf::Decision::Applied&&p.last_id==8&&!p.fault;
 }
 if(!std::strcmp(scenario,"ambiguous_or_recycled")){
  if(p.inspect(s)!=devf::Decision::Applied)return false;s=own(8);s.previous_live=true;
  if(p.inspect(s)!=devf::Decision::Fault||s.health!=220)return false;
  devf::Policy q;q.armed=true;s=own();if(q.inspect(s)!=devf::Decision::Applied)return false;
  return q.inspect(s)==devf::Decision::Fault&&s.health==220; // same ID during a new/reentrant create: fail closed.
 }
 if(!std::strcmp(scenario,"incomplete_and_late")){
  for(int which=0;which<4;++which){devf::Policy q;q.armed=true;auto x=own();
   if(which==0)x.health=0;if(which==1)x.health=40;if(which==2)x.resource_eoc2=false;if(which==3)x.initializing=false;
   auto hp=x.health;if(q.inspect(x)!=devf::Decision::Fault||x.health!=hp)return false;
  }return true;
 }
 if(!std::strcmp(scenario,"detach_disarm")){p.armed=false;return p.inspect(s)==devf::Decision::Ignore&&s.health==220&&!p.last_id;}
 return false;
}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;scenario=argv[1];bool ok=false;
 if(!std::strncmp(scenario,"registration",12)||!std::strcmp(scenario,"reentry")||!std::strcmp(scenario,"stage_fail")||
 !std::strcmp(scenario,"zero_handle")||!std::strcmp(scenario,"install_fail")||!std::strcmp(scenario,"install_throw")||
 !std::strcmp(scenario,"missing_hook")||!std::strcmp(scenario,"hook_minor0")||!std::strcmp(scenario,"bad_host")){
  HostService host{};host.header=SERVICE_HEADER(HostService,2,2);host.version="2.0.3";
  LogService log{};log.header=SERVICE_HEADER(LogService,1,0);log.write=[](ModContext*,LogLevel,const char*){};
  StageService stage{};stage.header=SERVICE_HEADER(StageService,1,0);stage.add_actor=add;
  HookService hook{};hook.header=SERVICE_HEADER(HookService,1,1);hook.install=sdk_install;hook.add_pre=sdk_pre;
  hook.dispatch_pre=sdk_dispatch;hook.dispatch_post=sdk_post;hook.uninstall=sdk_remove;
  if(!std::strcmp(scenario,"hook_minor0"))hook.header.minor_version=0;
  if(!std::strcmp(scenario,"bad_host"))host.version="1.3.1";
  const bool invalid=!std::strcmp(scenario,"missing_hook")||!std::strcmp(scenario,"hook_minor0")||!std::strcmp(scenario,"bad_host");
  const auto expected=invalid?MOD_UNSUPPORTED:!std::strcmp(scenario,"install_fail")?MOD_UNAVAILABLE:!std::strcmp(scenario,"stage_fail")?MOD_INVALID_ARGUMENT:
    (!std::strcmp(scenario,"zero_handle")||!std::strcmp(scenario,"install_throw"))?MOD_ERROR:MOD_OK;
  devf::Registration reg;auto result=reg.initialize(reinterpret_cast<ModContext*>(1),&host,&log,&stage,!std::strcmp(scenario,"missing_hook")?nullptr:&hook,install,remove,callback);
  ok=result==expected&&installs==(invalid?0:1)&&adds==(invalid||!std::strcmp(scenario,"install_fail")||!std::strcmp(scenario,"install_throw")?0:1)&&(!adds||payload)&&reg.ready==(expected==MOD_OK);
  ok&=reg.initialize(reinterpret_cast<ModContext*>(1),&host,&log,&stage,&hook,install,remove,callback)==MOD_CONFLICT;
  // TEST_FAKE host detach, not an execution of the real backend or DLL.
  remove();ok&=!retained;
 }else ok=policy_case();
 std::printf("%s TEST_FAKE case=%s native_order_source_model=1 game=NOT_EXECUTED DLL=NOT_LOADED\n",ok?"PASS":"FAIL",scenario);
 return ok?0:1;
}
