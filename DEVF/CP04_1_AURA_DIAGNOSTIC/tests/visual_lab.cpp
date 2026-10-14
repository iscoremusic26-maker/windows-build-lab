#include "../src/registration.hpp"
#include "../src/particle_access.hpp"
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
// Unused SDK inline destructor tripwire; test-only, never production.
extern "C" void GXDestroyTexObj(GXTexObj*){std::abort();}
namespace {
struct Events {unsigned applied=0,released=0,rejected=0;const char* reason=nullptr;void operator()(const char* event,unsigned,unsigned,const char* why,const char*)noexcept{
 if(!std::strcmp(event,"CP04_VISUAL_APPLIED"))++applied;if(!std::strcmp(event,"CP04_VISUAL_RELEASED"))++released;if(!std::strcmp(event,"CP04_VISUAL_REJECTED")){++rejected;reason=why;}}};
struct Asset {int material=73,texture=91;};
struct Emitter {unsigned key=0;std::uintptr_t owner=0;int purple=0;float x=0,y=0,z=0;const Asset* asset=nullptr;};
struct Access {
 Asset asset;Emitter own,original{11,0,0,1,2,3,&asset};bool missing=false,pool_fail=false,refresh_fail=false;
 unsigned creates=0,attempts=0,updates=0,cuts=0;devf::ResourceProbe resource;devf::ParticleGate injected=devf::ParticleGate::Ready;
 devf::CreateResult create(const devf::VisualProbe& p)noexcept{
  ++attempts;devf::CreateResult r;
  r.gate=injected!=devf::ParticleGate::Ready?injected:missing?devf::ParticleGate::ResourceMissing:pool_fail?devf::ParticleGate::PoolCreateFailed:devf::resource_gate(resource);
  if(r.gate!=devf::ParticleGate::Ready)return r;
  ++creates;own={100+creates,devf::owner_stamp(p.id),0,p.x,p.y,p.z,&asset};r.key=own.key;return r;}
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
  {for(int i=0;i<1000;++i)v.step(p,a,emit);
   const auto expected=!std::strcmp(name,"resource_missing")?"RESOURCE_029D_MISSING":!std::strcmp(name,"pool_failure")?"EMITTER_POOL_CREATE_FAILED":!std::strcmp(name,"model_missing")?"MODEL_NOT_READY_OR_NOT_OWNED":"OWNERSHIP_OR_PRIVATE_CONFIG_READBACK_FAILED";
   return v.fault&&e.rejected==1&&e.applied==0&&!v.key&&a.original.purple==0&&e.reason&&!std::strcmp(e.reason,expected)&&a.attempts<=1;}
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
// Native create_checked is compiled here with test doubles, not a second handwritten route.
struct Dyn {
 unsigned flags=0;float rate=0,life=0;
 unsigned getRateStep()const{return 7;}
 unsigned getFlag()const{return flags;}float getRateRndm()const{return rate;}float getLifetimeRndm()const{return life;}
};
struct Key {unsigned value=1;unsigned getID()const{return value;}};
struct ExtraTexture {bool isUseIndirect()const{return false;}};
struct Resource {
 ExtraTexture extra;
 Dyn dyn;Key key;Key* keys[1]={&key};Key** ppKey=keys;unsigned keyNum=0;bool dyn_null=false,child=false,indirect=false;
 const Dyn* getDyn()const{return dyn_null?nullptr:&dyn;}
 const void* getCsp()const{return child?this:nullptr;}const ExtraTexture* getEts()const{return indirect?&extra:nullptr;}
};
struct Bank {
 Resource resource;unsigned work=0,lookups=0,work_reads=0;bool missing=false;
 Resource* getResource(unsigned id){++lookups;return missing||id!=devf::kAura?nullptr:&resource;}
 unsigned getResUserWork(unsigned id){++work_reads;return id==devf::kAura?work:0;}
};
struct NativeEmitter {std::uintptr_t owner=0;bool hidden=false;void setUserWork(std::uintptr_t stamp){owner=stamp;}void stopDrawParticle(){hidden=true;}};
struct Manager {
 Bank bank;Bank* banks[1]={&bank};Bank** pResMgrAry=banks;unsigned ridMax=2,creates=0,expected_group=0;bool pool_fail=false,parameters_ok=false;NativeEmitter emitter;
 NativeEmitter* createSimpleEmitterID(const JGeometry::TVec3<float>& pos,unsigned id,unsigned group,unsigned bank_id,void* ec,void* pc){
  ++creates;parameters_ok=id==devf::kAura&&group==expected_group&&bank_id==0&&!ec&&!pc&&pos.x==1&&pos.y==2&&pos.z==3;
  return pool_fail?nullptr:&emitter;
 }
};
struct Control {struct Level {
 unsigned entries=0;bool fail=false,parameters_ok=false;
 unsigned entry(unsigned id,NativeEmitter* emitter,void* cb){++entries;parameters_ok=id==devf::kAura&&emitter&&!cb;return fail?0:987;}
 } field_0x210;};
bool native_branches(){
 using G=devf::ParticleGate;
 for(unsigned n=1;n<=unsigned(G::LevelEntryFailed);++n){
  const auto wanted=G(n);Manager m;Control c;Manager* manager=&m;Control* control=&c;
  auto& r=m.bank.resource;
  switch(wanted){
   case G::ControlNull:control=nullptr;break;case G::ManagerNull:manager=nullptr;break;
   case G::ArrayNull:m.pResMgrAry=nullptr;break;case G::BankCountInvalid:m.ridMax=0;break;
   case G::Bank0Null:m.banks[0]=nullptr;break;case G::ResourceMissing:m.bank.missing=true;break;
   case G::DynamicsNull:r.dyn_null=true;break;case G::KeyTableNull:r.keyNum=1;r.ppKey=nullptr;break;
   case G::KeyEntryNull:r.keyNum=1;r.keys[0]=nullptr;break;case G::UserWorkUnsafe:m.bank.work=0x400;break;
   case G::ChildUnsafe:r.child=true;break;case G::IndirectUnsafe:r.indirect=true;break;
   case G::FixedIntervalUnsafe:r.dyn.flags=JPADynFlag_FixedInterval;break;
   case G::RateLifetimeKeyUnsafe:r.keyNum=1;r.key.value=4;break;
   case G::RateRandomUnsafe:r.dyn.rate=2;break;case G::LifetimeRandomUnsafe:r.dyn.life=-1;break;
   case G::PoolCreateFailed:m.pool_fail=true;break;case G::LevelEntryFailed:c.field_0x210.fail=true;break;
   default:return false;
  }
  const auto result=devf::ParticleAccess::create_checked(probe(),control,manager);
  if(result||result.gate!=wanted||result.key)return false;
  const unsigned alloc=n>=unsigned(G::PoolCreateFailed)?1:0;
  if(m.creates!=alloc||c.field_0x210.entries!=(wanted==G::LevelEntryFailed?1u:0u))return false;
  if(wanted==G::LevelEntryFailed&&(!m.emitter.hidden||m.emitter.owner!=devf::owner_stamp(147)||!c.field_0x210.parameters_ok))return false;
  // This part verifies the actual Visual consumer of each typed gate is one-shot.
  Events seen;devf::Visual latched;Access aa;aa.injected=wanted;
  auto emit=[&](const char* ev,unsigned id,unsigned key,const char* why,const char* proof)noexcept{seen(ev,id,key,why,proof);};
  for(int frame=0;frame<1000;++frame)latched.step(probe(),aa,emit);
  if(!latched.fault||aa.attempts!=1||seen.rejected!=1||seen.applied||!seen.reason||std::strcmp(seen.reason,devf::gate_reason(wanted))||aa.creates||aa.cuts||aa.updates||aa.original.purple)return false;
  std::printf("PASS TEST_FAKE gate=%s one_attempt=1 rejected_logs=1 native_RAM=NOT_EXECUTED\n",devf::gate_reason(wanted));
 }
 for(unsigned group=0;group<4;++group){
  Manager m;Control c;m.bank.work=1u<<group;m.expected_group=group;auto result=devf::ParticleAccess::create_checked(probe(),&c,&m);
  if(!result||result.key!=987||result.probe_mask!=127||result.rate_step!=7||m.creates!=1||c.field_0x210.entries!=1||!m.parameters_ok||!c.field_0x210.parameters_ok||m.emitter.owner!=devf::owner_stamp(147)||m.emitter.hidden)return false;
 }
 return true;
}
bool reason_boundaries(){
 auto old=[](const devf::ResourceProbe& r){return !(r.work&0xFD90u)&&!r.child&&!r.indirect&&!r.fixed_interval&&!r.rate_or_life_key&&r.rate_random>=0&&r.rate_random<=1&&r.life_random>=0&&r.life_random<=1;};
 const float values[]={-1,0,0.5f,1,2,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()};
 unsigned compared=0;
 for(unsigned bits=0;bits<32;++bits)for(unsigned flags=0;flags<16;++flags)for(float rate:values)for(float life:values){
  devf::ResourceProbe r;r.work=1u<<bits;r.child=flags&1;r.indirect=flags&2;r.fixed_interval=flags&4;r.rate_or_life_key=flags&8;r.rate_random=rate;r.life_random=life;
  if(old(r)!=devf::resource_safe(r))return false;++compared;
 }
 devf::ResourceProbe r;r.child=true;r.indirect=true;r.work=0x400;
 if(devf::resource_gate(r)!=devf::ParticleGate::UserWorkUnsafe)return false;
 r.work=0;if(devf::resource_gate(r)!=devf::ParticleGate::ChildUnsafe)return false;
 r.child=false;if(devf::resource_gate(r)!=devf::ParticleGate::IndirectUnsafe)return false;
 std::printf("PASS TEST_FAKE historical_predicate_comparisons=%u guard_acceptance_unchanged=1\n",compared);return true;
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
int main(int argc,char** argv){if(argc!=2)return 2;bool ok=!std::strcmp(argv[1],"typed_native_create_branches")?native_branches():!std::strcmp(argv[1],"resource_reason_boundaries")?reason_boundaries():visual_case(argv[1]);
 std::printf("%s TEST_FAKE visual_case=%s game=NOT_EXECUTED render=NOT_TESTED DLL=NOT_LOADED\n",ok?"PASS":"FAIL",argv[1]);return ok?0:1;}
