#include "../src/registration.hpp"
#include "../src/visual.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
// Unused SDK inline destructor tripwire; test-only, never production.
extern "C" void GXDestroyTexObj(GXTexObj*){std::abort();}
namespace {
struct Events {unsigned applied=0,released=0,rejected=0;void operator()(const char* event,unsigned,unsigned,const char*,const char*)noexcept{
 if(!std::strcmp(event,"CP04_VISUAL_APPLIED"))++applied;if(!std::strcmp(event,"CP04_VISUAL_RELEASED"))++released;if(!std::strcmp(event,"CP04_VISUAL_REJECTED"))++rejected;}};
struct Asset {int material=73,texture=91;};
struct Emitter {unsigned key=0;std::uintptr_t owner=0;int purple=0;float x=0,y=0,z=0;const Asset* asset=nullptr;};
struct Access {
 Asset asset;Emitter own,original{11,0,0,1,2,3,&asset};bool missing=false,pool_fail=false,refresh_fail=false;
 unsigned creates=0,updates=0,cuts=0;devf::ResourceProbe resource;
 unsigned create(const devf::VisualProbe& p)noexcept{if(missing||pool_fail||!devf::resource_safe(resource))return 0;
  ++creates;own={100+creates,devf::owner_stamp(p.id),0,p.x,p.y,p.z,&asset};return own.key;}
 bool refresh(unsigned key,const devf::VisualProbe& p)noexcept{++updates;if(refresh_fail||own.key!=key||own.owner!=devf::owner_stamp(p.id))return false;
  own.purple=1;own.x=p.x;own.y=p.y;own.z=p.z;return true;}
 int release(unsigned key,unsigned id)noexcept{if(!own.key)return 0;if(key!=own.key||own.owner!=devf::owner_stamp(id))return -1;
  own.key=0;own.purple=0;++cuts;return 1;}
};
devf::VisualProbe probe(unsigned id=147){return {id,true,true,true,1,2,3};}
devf::Snapshot record(unsigned id=147){devf::Snapshot s;s.id=id;s.profile=510;s.room=3;s.layer=6;s.set_id=65535;s.parameters=0xFFFF0100;
 s.x=devf::kX;s.y=devf::kY;s.z=devf::kZ;s.home_x=262;s.stage=true;s.initializing=true;s.resource_eoc2=true;s.health=220;return s;}
bool visual_case(const char* name){
 devf::Visual v;Access a;Events e;auto emit=[&](const char* n,unsigned id,unsigned k,const char* why,const char* proof)noexcept{e(n,id,k,why,proof);};auto p=probe();
 if(!std::strcmp(name,"matching_and_original")){
  devf::Policy hp;hp.armed=true;auto s=record();if(hp.inspect(s)!=devf::Decision::Applied)return false;s.health=150;s.initializing=false;s.health=110;
  if(hp.inspect(s)!=devf::Decision::Ignore||s.health!=110)return false;
  for(int n=0;n<2;++n){auto other=record(900+n);other.x+=1;other.health=n?220:40;const auto h=other.health;
   if(hp.inspect(other)!=devf::Decision::Ignore||other.health!=h)return false;
   auto original=probe(other.id);original.eligible=false;v.step(original,a,emit);}
  v.step(p,a,emit);return e.applied==1&&!e.rejected&&a.creates==1&&a.original.purple==0;
 }
 if(!std::strcmp(name,"resource_gates")){
  devf::ResourceProbe x;if(!devf::resource_safe(x))return false;
  for(int n=0;n<8;++n){x={};if(n==0)x.work=0x400;if(n==1)x.child=true;if(n==2)x.indirect=true;if(n==3)x.fixed_interval=true;
   if(n==4)x.rate_or_life_key=true;if(n==5)x.rate_random=2;if(n==6)x.life_random=-1;if(n==7)x.work=0x10;
   if(devf::resource_safe(x))return false;}
  a.resource.child=true;v.step(p,a,emit);return v.fault&&e.rejected==1&&a.creates==0;
 }
 if(!std::strcmp(name,"resource_missing"))a.missing=true;
 if(!std::strcmp(name,"pool_failure"))a.pool_fail=true;
 if(!std::strcmp(name,"model_missing"))p.model_ready=false;
 if(!std::strcmp(name,"config_failure"))a.refresh_fail=true;
 v.step(p,a,emit);
 if(!std::strcmp(name,"resource_missing")||!std::strcmp(name,"pool_failure")||!std::strcmp(name,"model_missing")||!std::strcmp(name,"config_failure"))
  return v.fault&&e.rejected==1&&e.applied==0&&(!v.key)&&a.original.purple==0;
 if(!std::strcmp(name,"shared_asset_private_emitters"))return a.own.asset==a.original.asset&&a.asset.material==73&&a.asset.texture==91&&a.own.purple==1&&a.original.purple==0;
 if(!std::strcmp(name,"single_emitter_anchor")){
  for(int n=0;n<1000;++n){p.x+=1;v.step(p,a,emit);}return a.creates==1&&e.applied==1&&e.released==0&&e.rejected==0&&a.own.x==p.x;
 }
 if(!std::strcmp(name,"death_and_reload")){
  p.live=false;v.step(p,a,emit);v.step(p,a,emit);p=probe(568);v.step(p,a,emit);
  return a.creates==2&&a.cuts==1&&e.applied==2&&e.released==1&&!v.fault&&v.id==568;
 }
 if(!std::strcmp(name,"next_generation")){
  p=probe(568);v.step(p,a,emit);return a.creates==2&&a.cuts==1&&e.released==1&&e.applied==2;
 }
 if(!std::strcmp(name,"recycled_handle")){
  a.own.owner=0;a.own.purple=0;v.step(p,a,emit);return v.fault&&e.rejected==1&&a.cuts==0&&a.own.purple==0&&a.original.purple==0;
 }
 if(!std::strcmp(name,"shutdown_no_game_access")){
  const auto writes=a.updates+a.cuts;v.shutdown(emit);v.shutdown(emit);
  return !v.key&&!v.id&&e.released==1&&a.updates+a.cuts==writes; // Native expiry, not fake immediate engine destruction.
 }
 return false;
}
int pre_count=0,post_count=0,adds=0;bool pre_live=false,post_live=false;int mode=0;
HookAction pre(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}void post(ModContext*,void*,void*,void*){}
ModResult pre_install(HookPreFn){++pre_count;pre_live=true;return mode==1?MOD_UNAVAILABLE:MOD_OK;}
ModResult post_install(HookPostFn){++post_count;post_live=true;if(mode==3)throw std::runtime_error("fake");return mode==2?MOD_UNAVAILABLE:MOD_OK;}
ModResult remove(){pre_live=post_live=false;return MOD_OK;}
bool registration_case(){
 for(mode=0;mode<5;++mode){
  pre_live=post_live=false;pre_count=post_count=adds=0;HostService host{};host.header=SERVICE_HEADER(HostService,2,2);host.version="2.0.3";
  LogService log{};log.header=SERVICE_HEADER(LogService,1,0);log.write=[](ModContext*,LogLevel,const char*){};
  StageService stage{};stage.header=SERVICE_HEADER(StageService,1,0);
  stage.add_actor=[](ModContext*,const char* scene,uint8_t room,int8_t layer,const void* raw,size_t len,StageActorHandle* handle){
   ++adds;const auto& r=*static_cast<const stage_actor_data_class*>(raw);
   if(!pre_live||!post_live||std::strcmp(scene,"F_SP121")||room!=3||layer!=6||len!=32||std::strcmp(r.name,"E_oc")||u32(r.base.parameters)!=0xFFFF0100u||float(r.base.position.x)!=devf::kX||float(r.base.position.y)!=devf::kY||float(r.base.position.z)!=devf::kZ||s16(r.base.angle.x)!=262||u16(r.base.setID)!=65535)return MOD_ERROR;
   *handle=19;return mode==4?MOD_ERROR:MOD_OK;
  };
  HookService hook{};hook.header=SERVICE_HEADER(HookService,1,1);hook.install=[](ModContext*,void*,void*,void**){return MOD_OK;};
  hook.add_pre=[](ModContext*,void*,HookPreFn,const HookOptions*){return MOD_OK;};hook.add_post=[](ModContext*,void*,HookPostFn,const HookOptions*){return MOD_OK;};
  hook.dispatch_pre=[](ModContext*,void*,void*,void*,int*){return MOD_OK;};hook.dispatch_post=[](ModContext*,void*,void*,void*){return MOD_OK;};hook.uninstall=[](ModContext*,void*,void**){return MOD_OK;};
  devf::Registration r;auto result=r.initialize(reinterpret_cast<ModContext*>(1),&host,&log,&stage,&hook,pre_install,post_install,remove,pre,post);
  if((result==MOD_OK)!=(mode==0)||adds!=(mode==0||mode==4?1:0)||mode>0&&(pre_live||post_live))return false;
  if(r.initialize(reinterpret_cast<ModContext*>(1),&host,&log,&stage,&hook,pre_install,post_install,remove,pre,post)!=MOD_CONFLICT)return false;
  remove();if(pre_live||post_live)return false;
 }
 return true;
}
}
int main(int argc,char** argv){if(argc!=2)return 2;bool ok=!std::strcmp(argv[1],"registration_rollback")?registration_case():visual_case(argv[1]);
 std::printf("%s TEST_FAKE visual_case=%s game=NOT_EXECUTED render=NOT_TESTED DLL=NOT_LOADED\n",ok?"PASS":"FAIL",argv[1]);return ok?0:1;}
