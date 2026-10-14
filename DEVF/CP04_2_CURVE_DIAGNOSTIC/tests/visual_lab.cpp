#include "../src/registration.hpp"
#include "../src/particle_access.hpp"
#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
extern "C" void GXDestroyTexObj(GXTexObj*){std::abort();} // Lab-only; never production.
namespace {
struct Key {
 alignas(4) unsigned char bytes[4112]{};const unsigned char* mDataStart=bytes;bool bad_data=false;unsigned count_override=0;
 unsigned getID()const{return bytes[8];}unsigned getKeyNum()const{return count_override?count_override:bytes[9];}bool isLoop()const{return bytes[11]!=0;}
 const BE(float)* getKeyData()const{return bad_data?nullptr:reinterpret_cast<const BE(float)*>(bytes+12);}
 void word(unsigned at,std::uint32_t n){for(unsigned j=0;j<4;++j)bytes[at+j]=static_cast<unsigned char>(n>>(24-8*j));}
 void point(unsigned i,float t,float v,float in=0,float out=0){const float values[]={t,v,in,out};for(unsigned n=0;n<4;++n)word(12+16*i+4*n,std::bit_cast<std::uint32_t>(values[n]));}
 void init(unsigned id=0,unsigned n=2){std::memset(bytes,0,sizeof(bytes));std::memcpy(bytes,"KFA1",4);word(4,12+16*n);bytes[8]=id;bytes[9]=n;point(0,0,0.4f);if(n>1)point(1,60,0.4f);}
 Key(){init();}
};
struct Dyn {
 void* mpData=this;unsigned flags=0x118;float rate=0.4f,rate_random=0,life_random=0;
 unsigned getFlag()const{return flags;}unsigned getRateStep()const{return 0;}
 float getRate()const{return rate;}float getRateRndm()const{return rate_random;}float getLifetimeRndm()const{return life_random;}
 int getLifetime()const{return 18;}int getStartFrame()const{return 0;}int getMaxFrame()const{return 0;}
};
struct Resource {
 Dyn dyn;Key a,b;Key* keys[2]={&a,&b};Key** ppKey=keys;unsigned keyNum=2,id=devf::kAura;bool dyn_null=false;
 Resource(){b.init(4);b.point(0,0,18);b.point(1,60,18);}
 unsigned getUsrIdx()const{return id;}Dyn* getDyn(){return dyn_null?nullptr:&dyn;}
 const void* getCsp()const{return nullptr;}const void* getEts()const{return nullptr;}
};
struct Bank {
 Resource resource;bool missing=false;unsigned work=0x12,lookups=0,work_reads=0;
 Resource* getResource(unsigned id){++lookups;return missing||id!=devf::kAura?nullptr:&resource;}
 unsigned getResUserWork(unsigned){++work_reads;return work;}
};
struct Manager {
 Bank bank;Bank* banks[1]={&bank};Bank** pResMgrAry=banks;unsigned ridMax=2,gidMax=19;
 // Deliberately no emitter/pool/entry API. The same production template MUST compile without it.
};
struct Control {};
struct Access {Control control;Manager manager;unsigned reads=0;devf::AuditResult inspect(const devf::VisualProbe&)noexcept{++reads;return devf::ParticleAccess::inspect_checked(&control,&manager);}};
struct Events {unsigned logs=0;const char* reason=nullptr;void operator()(const char* event,unsigned,unsigned,const char* why,const char*){if(!std::strcmp(event,"CP04_VISUAL_REJECTED")){++logs;reason=why;}}};
devf::VisualProbe probe(){return {147,true,true,true,1,2,3};}
bool run_case(const char* name){
 using G=devf::Gate;
 if(!std::strcmp(name,"native_group_9")){
  unsigned group=0;if(!devf::known_group(0x12,group)||group!=9)return false;
  for(unsigned bad:{0u,2u,0x10u,0x112u,0x412u,0x812u,0x1012u,0xFFFFu})if(devf::known_group(bad,group))return false;return true;
 }
 if(!std::strcmp(name,"curve_constant")){
  Key k;devf::CurveInfo r;if(devf::summarize_key(&k,r)!=G::Complete||!r.summarized||r.points!=2||r.id!=0||r.hull_min!=double(0.4f)||r.hull_max!=double(0.4f))return false;
  k.init(4,1);k.point(0,5,18,17,-31);r={};return devf::summarize_key(&k,r)==G::Complete&&r.hull_min==18&&r.hull_max==18&&r.first==5&&r.last==5;
 }
 if(!std::strcmp(name,"curve_overshoot_hull")){
  Key k;k.point(0,0,1,0,3);k.point(1,1,1,-3,0);devf::CurveInfo r;
  if(devf::summarize_key(&k,r)!=G::Complete||r.value_min!=1||r.value_max!=1||r.hull_max!=2)return false;
  for(unsigned n=0;n<=1000;++n){double t=double(n)/1000,v=1+3*t*(1-t);if(v<r.hull_min||v>r.hull_max)return false;}
  k.bytes[11]=1;r={};return devf::summarize_key(&k,r)==G::Complete&&r.loop==1;
 }
 if(!std::strcmp(name,"curve_shape_failures")){
  for(unsigned mode=0;mode<6;++mode){Key k;devf::CurveInfo r;
   if(mode==0)k.point(0,0,std::numeric_limits<float>::quiet_NaN());
   if(mode==1)k.point(1,std::numeric_limits<float>::infinity(),1);
   if(mode==2)k.point(1,0,1);if(mode==3)k.point(1,-1,1);
   if(mode==4)k.point(1,60,2e6f);if(mode==5)k.point(1,1e-6f,1);
   const auto expected=mode<2?G::NonFinite:mode==3||mode==4?G::ScalarRange:G::TimeOrder;
   if(devf::summarize_key(&k,r)!=expected||r.summarized)return false;
  }return true;
 }
 if(!std::strcmp(name,"key_metadata_bounds")){
  devf::CurveInfo r;if(devf::summarize_key<Key>(nullptr,r)!=G::KeyNull)return false;
  for(unsigned mode=0;mode<8;++mode){Key k;devf::CurveInfo out;
   if(mode==0)k.mDataStart=nullptr;if(mode==1)k.bytes[0]='X';if(mode==2)k.word(4,11);if(mode==3)k.word(4,8192);
   if(mode==4)k.bytes[9]=0;if(mode==5)k.count_override=256;if(mode==6)k.word(4,12);if(mode==7)k.bad_data=true;
   const G expect[]={G::HeaderNull,G::HeaderMagic,G::HeaderSize,G::HeaderSize,G::PointsLimit,G::PointsLimit,G::HeaderSize,G::DataPointer};
   if(devf::summarize_key(&k,out)!=expect[mode]||out.summarized)return false;
  }
  Key k;k.init(4,255);for(unsigned n=0;n<255;++n)k.point(n,float(n),18);r={};return devf::summarize_key(&k,r)==G::Complete&&r.points==255;
 }
 if(!std::strcmp(name,"native_read_branch_matrix")){
  for(unsigned mode=0;mode<13;++mode){Manager m;Control c;auto* manager=&m;auto* control=&c;
   if(mode==0)control=nullptr;if(mode==1)manager=nullptr;if(mode==2)m.pResMgrAry=nullptr;
   if(mode==3)m.ridMax=0;if(mode==4)m.bank.missing=true;if(mode==5)m.bank.resource.dyn_null=true;
   if(mode==6)m.bank.resource.id=7;if(mode==7)m.bank.work=0x412;if(mode==8)m.gidMax=9;
   if(mode==9)m.bank.resource.keyNum=3;if(mode==10)m.bank.resource.ppKey=nullptr;
   if(mode==11)m.bank.resource.keys[1]=nullptr;
   const G expect[]={G::ControlNull,G::ManagerNull,G::ArrayNull,G::BankMissing,G::ResourceMissing,G::DynamicsNull,G::IdentityChanged,G::WorkChanged,G::GroupUnavailable,G::KeyCountChanged,G::KeyArrayNull,G::KeyNull,G::Complete};
   auto r=devf::ParticleAccess::inspect_checked(control,manager);if(r.gate!=expect[mode])return false;
   if(mode==12&&(r.probe_mask!=127||r.group!=9||r.curves[0].id!=0||r.curves[1].id!=4||r.curves[1].hull_max!=18))return false;
  }return true;
 }
 if(!std::strcmp(name,"one_shot_no_effect")){
  devf::Visual v;Access a;Events e;auto emit=[&](const char* n,unsigned i,unsigned k,const char* why,const char* proof){e(n,i,k,why,proof);};
  for(unsigned n=0;n<1000;++n)v.step(probe(),a,emit);
  return v.once&&v.fault&&!v.key&&a.reads==1&&e.logs==1&&!std::strcmp(e.reason,devf::gate_reason(G::Complete));
 }
 if(!std::strcmp(name,"original_actors_untouched")){
  devf::Policy hp;hp.armed=true;devf::Visual v;Access a;Events e;
  auto emit=[&](const char* n,unsigned i,unsigned k,const char* why,const char* proof){e(n,i,k,why,proof);};
  for(unsigned n=0;n<2;++n){devf::Snapshot s;s.id=900+n;s.profile=510;s.room=3;s.layer=6;s.set_id=65535;s.stage=true;s.parameters=n?0xFFFF0100u:0xFFFFFFFFu;s.health=n?220:40;
   s.resource_eoc2=n!=0;s.x=devf::kX+1;s.y=devf::kY;s.z=devf::kZ;s.home_x=262;
   const auto health=s.health;if(hp.inspect(s)!=devf::Decision::Ignore||s.health!=health)return false;
   auto p=probe();p.id=s.id;p.eligible=false;v.step(p,a,emit);
  }return !a.reads&&!e.logs&&!v.once;
 }
 if(!std::strcmp(name,"shutdown_no_game_access")){
  devf::Visual v;Access a;auto emit=[](const char*,unsigned,unsigned,const char*,const char*){};v.step(probe(),a,emit);v.shutdown(emit);v.shutdown(emit);return a.reads==1&&!v.id&&!v.key;
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
int main(int argc,char** argv){if(argc!=2)return 2;bool ok=run_case(argv[1]);std::printf("%s TEST_FAKE curve_case=%s game=NOT_EXECUTED native_resource=NOT_READ emission=FORBIDDEN\n",ok?"PASS":"FAIL",argv[1]);return ok?0:1;}
