#include "../src/registration.hpp"
#include "../src/particle_access.hpp"
#include <array>
#include <algorithm>
#include <JSystem/JParticle/JPAParticle.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
extern "C" void GXDestroyTexObj(GXTexObj*){std::abort();} // Lab tripwire only.
namespace {
struct Extra {
 bool enabled=false;float in=0.25f,out=0.75f,random=0,vi=0,vo=0,si=4,so=-4;unsigned type=0;int cycle=3;
 bool isEnableScaleAnm()const{return enabled;}
 float getScaleInTiming()const{return in;}float getScaleOutTiming()const{return out;}
 float getScaleRndm()const{return random;}
 float getScaleInValueX()const{return vi;}float getScaleInValueY()const{return vi;}
 float getScaleOutValueX()const{return vo;}float getScaleOutValueY()const{return vo;}
 float getScaleIncRateX()const{return si;}float getScaleIncRateY()const{return si;}
 float getScaleDecRateX()const{return so;}float getScaleDecRateY()const{return so;}
 unsigned getScaleAnmTypeX()const{return type;}unsigned getScaleAnmTypeY()const{return type;}
 int getScaleAnmCycleX()const{return cycle;}int getScaleAnmCycleY()const{return cycle;}
};
struct Shape {
 void* pBsd=this;bool no_draw=false;float x=50,y=50;unsigned type=2;
 bool isNoDrawParent()const{return no_draw;}float getBaseSizeX()const{return x;}float getBaseSizeY()const{return y;}unsigned getType()const{return type;}
};
struct Dyn {
 void* mpData=this;void* mpCalcVolumeFunc=this;unsigned flags=0x408,start=0,volume=4;float rate_random=0,life_random=0;
 unsigned getFlag()const{return flags;}float getRateRndm()const{return rate_random;}float getLifetimeRndm()const{return life_random;}
 int getStartFrame()const{return int(start);}unsigned getVolumeType()const{return volume;}
};
struct Resource {
 Dyn dyn;Shape shape;Extra extra;bool missing_dyn=false,missing_shape=false,child=false,indirect=false;
 unsigned keyNum=0,fldNum=0,texNum=1,id=devf::kAura;void* fields[4]={this,this,this,this};void** ppFld=fields;
 unsigned indices[8]{};unsigned* mpTDB1=indices;
 unsigned getUsrIdx()const{return id;}Dyn* getDyn(){return missing_dyn?nullptr:&dyn;}Shape* getBsp(){return missing_shape?nullptr:&shape;}
 Extra* getEsp(){return &extra;}const void* getCsp()const{return child?this:nullptr;}const void* getEts()const{return indirect?this:nullptr;}
 unsigned getTexIdx(unsigned i)const{return indices[i];}
};
struct Texture {void* mpData=this;};
struct Bank {
 Resource res;Texture tex;Texture* textures[1]={&tex};Texture** pTexAry=textures;unsigned texRegNum=1,work=0x12,lookups=0;bool missing=false;
 Resource* getResource(unsigned id){++lookups;return missing||id!=devf::kAura?nullptr:&res;}
 unsigned getResUserWork(unsigned){return work;}
};
// Uses actual public SDK emitter setters/getters and layout. No native particle calculation/draw.
// pRes is a test-only shadow for the templated resource identity read.
struct Emitter:JPABaseEmitter {
 Resource* pRes=nullptr;
 Emitter(){mStatus=0;mpUserWork=0;mResMgrID=1;mGroupID=9;mpEmtrCallBack=nullptr;mpPtclCallBack=nullptr;
  mRate=0;mLifeTime=1;mRateStep=7;mMaxFrame=1;mDrawTimes=1;mScaleOut=1;}
};
struct Manager {
 Bank bank;Bank* banks[2]={nullptr,&bank};Bank** pResMgrAry=banks;unsigned ridMax=2,gidMax=19,creates=0;bool pool_fail=false,parameters_ok=false;Emitter emitter;
 Emitter* createSimpleEmitterID(const JGeometry::TVec3<float>& p,unsigned id,unsigned group,unsigned bank_id,void* ec,void* pc){
  ++creates;parameters_ok=id==devf::kAura&&group==9&&bank_id==1&&!ec&&!pc&&p.x==1&&p.y==2&&p.z==3;
  if(pool_fail)return nullptr;emitter.pRes=&bank.res;emitter.mGroupID=static_cast<u8>(group);emitter.mStatus=0;return &emitter;
 }
};
struct Record {
 unsigned id=0,name=devf::kAura;Emitter* emitter=nullptr;void* callback=nullptr;unsigned active=0;
 unsigned getId()const{return id;}unsigned getNameId()const{return name;}void* getCallback()const{return callback;}
 Emitter* getEmitter()const{return emitter;}void onActive(){++active;}
};
struct Control {struct Level {
 Record rec;unsigned entries=0,cuts=0;bool fail=false,parameters_ok=false;
 unsigned entry(unsigned id,Emitter* e,void* cb){++entries;parameters_ok=id==devf::kAura&&e&&!cb;
  if(fail){e->becomeInvalidEmitter();e->quitImmortalEmitter();return 0;}
  e->becomeImmortalEmitter();rec={100+entries,id,e,cb,0};return rec.id;
 }
 Record* get(unsigned key){return rec.id==key?&rec:nullptr;}
 void cutTable(Record* r){++cuts;r->emitter->becomeInvalidEmitter();r->emitter->quitImmortalEmitter();r->id=0;}
 } field_0x210;};
struct Access {
 Manager manager;Control control;unsigned group=255,attempts=0;bool refresh_fail=false;
 devf::CreateResult create(const devf::VisualProbe& p){++attempts;auto r=devf::ParticleAccess::create_checked(p,&control,&manager);group=r.group;return r;}
 bool refresh(unsigned k,const devf::VisualProbe& p){return !refresh_fail&&devf::ParticleAccess::refresh_checked(&control,k,p,group);}
 int release(unsigned k,unsigned id){return devf::ParticleAccess::release_checked(&control,k,id,group);}
};
struct Events {unsigned applied=0,rejected=0,released=0;const char* why=nullptr;
 void operator()(const char* event,unsigned,unsigned,const char* reason,const char*){
  if(!std::strcmp(event,"CP04_VISUAL_APPLIED"))++applied;
  if(!std::strcmp(event,"CP04_VISUAL_REJECTED")){++rejected;why=reason;}
  if(!std::strcmp(event,"CP04_VISUAL_RELEASED"))++released;
 }
};
devf::VisualProbe probe(unsigned id=147){return {id,true,true,true,1,2,3};}
bool run_case(const char* name){
 using G=devf::Gate;
 if(!std::strcmp(name,"native_preflight_branches")){
  for(unsigned n=0;n<=unsigned(G::EntryFailed);++n){
   const auto want=G(n);Manager m;Control c;auto* manager=&m;auto* control=&c;auto& r=m.bank.res;
   switch(want){
   case G::Ready:break;case G::ControlNull:control=nullptr;break;case G::ManagerNull:manager=nullptr;break;
   case G::ArrayNull:m.pResMgrAry=nullptr;break;case G::BankMissing:m.banks[1]=nullptr;break;
   case G::ResourceMissing:m.bank.missing=true;break;case G::IdentityChanged:r.id=7;break;
   case G::DynamicsNull:r.dyn.mpData=nullptr;break;case G::ShapeNull:r.shape.pBsd=nullptr;break;
   case G::GroupUnsafe:m.bank.work=0x112;break;case G::GroupUnavailable:m.gidMax=9;break;
   case G::ChildUnsafe:r.child=true;break;case G::IndirectUnsafe:r.indirect=true;break;
   case G::FixedIntervalUnsafe:r.dyn.flags|=2;break;case G::RandomUnsafe:r.dyn.rate_random=std::numeric_limits<float>::quiet_NaN();break;
   case G::StartFrameUnsafe:r.dyn.start=1;break;case G::VolumeTypeUnsafe:r.dyn.volume=7;break;
   case G::KeysUnsupported:r.keyNum=255;break;case G::FieldLimit:r.fldNum=5;break;
   case G::FieldNull:r.fldNum=1;r.ppFld=nullptr;break;case G::ShapeUnsafe:r.shape.x=std::numeric_limits<float>::infinity();break;
   case G::ScaleUnsafe:r.extra.enabled=true;r.extra.random=2;break;
   case G::TextureUnsafe:r.indices[0]=1;break;case G::PoolFailed:m.pool_fail=true;break;case G::EntryFailed:c.field_0x210.fail=true;break;
   }
   const auto out=devf::ParticleAccess::create_checked(probe(),control,manager);
   if(out.gate!=want)return false;
   const bool create=want==G::Ready||want==G::PoolFailed||want==G::EntryFailed;
   if(m.creates!=unsigned(create)||bool(out)!=(want==G::Ready))return false;
   if(create&&!m.parameters_ok)return false;
   if(want==G::EntryFailed&&(!m.emitter.checkStatus(JPAEmtrStts_StopDraw)||!m.emitter.checkStatus(JPAEmtrStts_StopEmit)))return false;
  }
  Manager m;Control c;m.ridMax=1;return devf::ParticleAccess::create_checked(probe(),&c,&m).gate==G::BankMissing;
 }
 if(!std::strcmp(name,"groups_and_flags")){
  unsigned group=255;
  for(unsigned bit=0;bit<32;++bit){const unsigned w=1u<<bit;const bool allowed=(w&0x00EF007F)!=0;
   if(devf::native_group(w,group)!=allowed)return false;}
  for(unsigned w:{0u,1u,2u,4u,8u,0x10u,0x11u,0x12u,0x14u,0x18u,0xEF007Fu}){
   if(!devf::native_group(w,group))return false;
   const unsigned expected=(w&1?0u:w&2?1u:w&4?2u:w&8?3u:0u)+((w&0x10)?8u:0u);
   if(group!=expected)return false;
  }return true;
 }
 if(!std::strcmp(name,"all_curves_fail_closed")){
  // Count rejects before ppKey/curve data access, including malformed headers, infinite
  // values, lifetime/rate/volume IDs, loop or extreme tangents: none can reach emission.
  for(unsigned count:{1u,2u,255u}){Manager m;Control c;m.bank.res.keyNum=count;
   auto r=devf::ParticleAccess::create_checked(probe(),&c,&m);if(r.gate!=G::KeysUnsupported||m.creates)return false;
  }return true;
 }
 if(!std::strcmp(name,"fp32_population_bound")){
  // Model of published native formulas, NOT linked native calc or measured JPC.
  for(unsigned mode=0;mode<4;++mode){float residual=std::nextafter(1.0f,0.0f);std::vector<int> ages;unsigned peak=0;
   for(unsigned tick=0;tick<200000;++tick){
    const float rnd=mode==0?1.0f:mode==1?-1.0f:mode==2?0.0f:float(int(tick%1001)-500)/500.0f;
    const float next=0.4f*(1.0f+1.0f*rnd);residual+=next;int count=int(residual);residual-=float(count);
    if(tick==0&&next>0&&next<1)count=1;
    if(count<0||count>1||residual<0||residual>=1)return false;
    for(int i=0;i<count;++i)ages.push_back(-1);
    peak=std::max(peak,unsigned(ages.size()));if(peak>19)return false;
    for(auto& age:ages)++age;
    ages.erase(std::remove_if(ages.begin(),ages.end(),[](int age){return age>=18;}),ages.end());
    if(ages.size()>18)return false;
   }
  }
  for(float random:{0.0f,0.5f,1.0f})for(float rf:{0.0f,0.5f,std::nextafter(1.0f,0.0f),1.0f}){
   float life=(1.0f-random*rf)*18.0f;if(!(life>=0&&life<=18))return false;
  }return true;
 }
 if(!std::strcmp(name,"extra_scale_bounds")){
  Extra x;x.enabled=true;if(!devf::ParticleAccess::scale_safe(&x))return false;
  for(unsigned mode=0;mode<7;++mode){Extra bad=x;
   if(mode==0)bad.si=std::numeric_limits<float>::quiet_NaN();if(mode==1)bad.out=-1;
   if(mode==2)bad.random=2;if(mode==3)bad.vi=3;if(mode==4)bad.si=64;
   if(mode==5)bad.type=3;if(mode==6){bad.type=1;bad.cycle=0;}
   if(devf::ParticleAccess::scale_safe(&bad))return false;
  }return true;
 }
 devf::Visual v;Access a;Events events;auto emit=[&](const char* e,unsigned id,unsigned k,const char* why,const char* proof){events(e,id,k,why,proof);};auto p=probe();
 if(!std::strcmp(name,"missing_and_pool_no_retry")){
  for(unsigned mode=0;mode<3;++mode){devf::Visual vv;Access aa;Events ee;auto log=[&](const char* e,unsigned id,unsigned k,const char* why,const char* proof){ee(e,id,k,why,proof);};
   if(mode==0)aa.manager.bank.missing=true;if(mode==1)aa.manager.pool_fail=true;if(mode==2)aa.control.field_0x210.fail=true;
   for(unsigned n=0;n<1000;++n)vv.step(p,aa,log);
   if(aa.attempts!=1||ee.rejected!=1||ee.applied||vv.key)return false;
  }return true;
 }
 if(!std::strcmp(name,"partial_config_failure")){a.refresh_fail=true;v.step(p,a,emit);return v.fault&&!v.key&&events.rejected==1&&!events.applied&&a.control.field_0x210.cuts==1;}
 if(!std::strcmp(name,"original_eoc2_untouched")){
  devf::Policy hp;hp.armed=true;for(unsigned n=0;n<2;++n){devf::Snapshot s;s.id=900+n;s.profile=510;s.room=3;s.layer=6;s.set_id=65535;s.stage=true;s.parameters=0xFFFF0100;s.resource_eoc2=true;s.health=220;s.x=devf::kX+1;s.y=devf::kY;s.z=devf::kZ;s.home_x=262;
   if(hp.inspect(s)!=devf::Decision::Ignore||s.health!=220)return false;auto pp=p;pp.id=s.id;pp.eligible=false;v.step(pp,a,emit);
  }return !a.attempts&&!events.applied&&!events.rejected;
 }
 if(!std::strcmp(name,"nonfinite_anchor")){
  p.x=std::numeric_limits<float>::quiet_NaN();v.step(p,a,emit);
  return v.fault&&!v.key&&events.rejected==1&&!a.attempts;
 }
 v.step(p,a,emit);if(!v.key||events.applied!=1||events.rejected||!a.manager.parameters_ok)return false;
 if(!std::strcmp(name,"compatible_private_marker")){
  const auto source_id=a.manager.bank.res.id;for(unsigned n=0;n<1000;++n){p.x+=0.1f;v.step(p,a,emit);}
  return a.attempts==1&&a.manager.bank.lookups==1&&events.applied==1&&!events.rejected&&
   a.manager.emitter.mGlobalTrs.x==p.x&&source_id==a.manager.bank.res.id&&a.manager.bank.work==0x12&&a.manager.bank.res.dyn.rate_random==0;
 }
 if(!std::strcmp(name,"death_and_reentry")){
  p.live=false;v.step(p,a,emit);v.step(p,a,emit);if(v.key||a.control.field_0x210.cuts!=1||!a.manager.emitter.checkStatus(JPAEmtrStts_StopDraw))return false;
  p=probe(568);v.step(p,a,emit);return a.attempts==2&&events.applied==2&&!v.fault;
 }
 if(!std::strcmp(name,"actor_id_recycled")){
  p.live=false;v.step(p,a,emit);p.live=true;v.step(p,a,emit);
  return v.fault&&events.rejected==1&&a.attempts==1&&!v.key;
 }
 if(!std::strcmp(name,"next_generation")){
  p=probe(568);v.step(p,a,emit);return a.attempts==2&&events.applied==2&&a.control.field_0x210.cuts==1&&!v.fault;
 }
 if(!std::strcmp(name,"recycled_handle")){
  a.manager.emitter.setUserWork(0);const auto rate=a.manager.emitter.getRate();v.step(p,a,emit);
  return v.fault&&!v.key&&events.rejected==1&&!a.control.field_0x210.cuts&&a.manager.emitter.getRate()==rate;
 }
 if(!std::strcmp(name,"expired_handle_no_retry")){
  a.control.field_0x210.rec.id=0;for(unsigned n=0;n<1000;++n)v.step(p,a,emit);
  return v.fault&&!v.key&&a.attempts==1&&events.rejected==1;
 }
 if(!std::strcmp(name,"shutdown_no_game_access")){
  const auto active=a.control.field_0x210.rec.active;v.shutdown(emit);v.shutdown(emit);
  return !v.key&&!v.id&&events.released==1&&!a.control.field_0x210.cuts&&a.control.field_0x210.rec.active==active;
 }
 if(!std::strcmp(name,"runtime_budget_guard")){
  // API has no fake count override: add 21 SDK list nodes (no engine particles allocated).
  std::array<JPANode<JPABaseParticle>,21> nodes;
  for(auto& node:nodes)a.manager.emitter.mAlivePtclBase.push_back(&node);
  v.step(p,a,emit);const bool ok=v.fault&&!v.key&&events.rejected==1&&a.control.field_0x210.cuts==1;
  while(a.manager.emitter.mAlivePtclBase.getNum())a.manager.emitter.mAlivePtclBase.pop_back();return ok;
 }
 if(!std::strcmp(name,"scene_clear_stale_resource")){
  // Invalid address tripwire: native forced deletion must be detected before reading pRes.
  a.manager.emitter.pRes=reinterpret_cast<Resource*>(std::uintptr_t(1));a.manager.emitter.setStatus(0x300);
  v.step(p,a,emit);return v.fault&&!v.key&&!a.control.field_0x210.cuts&&events.rejected==1;
 }
 if(!std::strcmp(name,"pause_resume_contract")){
  a.manager.emitter.stopCalcEmitter();a.manager.emitter.stopCreateParticle();v.step(p,a,emit);
  return !v.fault&&!a.manager.emitter.checkStatus(JPAEmtrStts_StopCalc|JPAEmtrStts_StopEmit)&&a.attempts==1;
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
int main(int argc,char** argv){if(argc!=2)return 2;bool ok=run_case(argv[1]);std::printf("%s TEST_FAKE visual_case=%s game=NOT_EXECUTED native_resource=NOT_READ pixels=NOT_TESTED\n",ok?"PASS":"FAIL",argv[1]);return ok?0:1;}
