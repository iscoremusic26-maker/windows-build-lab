// CP11 local microbenchmark / parity harness TEMPLATE (not part of the mod, not built by CI).
// Fake engine types + the non-visual parts of mod.cpp needed by the aura path. make_bench.py inserts the CURRENT mod.cpp regions at the
// GLOBALS / RESNAME / VISUAL marker lines, so the same harness measures any version with identical inputs.
// It measures CPU time of the MOD's own code with fake engine calls: it says nothing about the real engine, the GPU or in-game FPS.

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <new>
#include <array>
#include "../../../CP10_2_BLACK_BOKOBLIN/expansion/black_policy.hpp"
#include "../../../CP10_2_FIX01_BLACK_TEXTURE/expansion/cmpr_recolor.hpp"
#include "../../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
#include "../../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include "../../../CP10_5_DAMAGE_BLUE_RED_BLACK/expansion/damage_policy.hpp"
#include "../../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include "../../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/eye_recolor.hpp"
#include "../../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include "../../expansion/strokes_policy.hpp"
#include "../../expansion/perf_policy.hpp"
using s16=short;using s8=signed char;using u8=unsigned char;using u16=unsigned short;using u32=unsigned;using s32=int;using f32=float;
enum {ID_ZI_J_LK_SWL_A=0xA58,ID_ZI_J_LK_SWL_C=0xB9D};
struct cXyz{float x,y,z;cXyz(float a,float b,float c):x(a),y(b),z(c){}};
namespace JGeometry{template<class T>struct TVec3{T x,y,z;TVec3(){x=y=z=0;}TVec3(T a,T b,T c):x(a),y(b),z(c){}};}
struct dKy_tevstr_c{u8 Material_use_fg=0x7c;};struct csXyz{};struct GXColor{u8 r,g,b,a;};struct dPa_levelEcallBack{};
struct JPABaseEmitter{JGeometry::TVec3<f32> ds{1,1,1},ps{1,1,1},mLocalDir{0,0,1},mLocalTrs{0,0,40},mLocalScl{1,1,0.1f};JGeometry::TVec3<s16> mLocalRot{90,0,0};f32 mVolumeSweep=0.15f;GXColor mPrmClr{255,255,195,150};u8 mRateStep=1;void setRateStep(u8 v){mRateStep=v;}float mGlobalRot[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};void setLocalTranslation(const JGeometry::TVec3<f32>&v){mLocalTrs=v;}void setLocalScale(const JGeometry::TVec3<f32>&v){mLocalScl=v;}f32 mVolumeMinRad=0,mRate=0.5f;s16 mLifeTime=45;u32 mTick=100;void setGlobalDynamicsScale(const JGeometry::TVec3<float>&v){ds=v;}void setGlobalParticleScale(f32 a,f32 b){ps.x=a;ps.y=b;}void getGlobalDynamicsScale(JGeometry::TVec3<f32>*o)const{*o=ds;}void getGlobalParticleScale(JGeometry::TVec3<f32>*o)const{*o=ps;}void setDirection(const JGeometry::TVec3<f32>&d){mLocalDir=d;}GXColor prmc{255,255,255,255};void setGlobalPrmColor(u8 r,u8 g,u8 b){prmc.r=r;prmc.g=g;prmc.b=b;}void setGlobalEnvColor(u8,u8,u8){}void setGlobalAlpha(u8 x){prmc.a=x;}void stopDrawParticle(){}void becomeContinuousParticle(){}void getGlobalPrmColor(GXColor& c){c=prmc;}};
cXyz g_last_pos{0,0,0};unsigned g_set_calls=0;u32 dComIfGp_particle_set(u32,u16,const cXyz* p,const dKy_tevstr_c*,const csXyz*,const cXyz*,u8,dPa_levelEcallBack*,s8,const GXColor*,const GXColor*,const cXyz*){++g_set_calls;if(p)g_last_pos=*p;return 1;}
JPABaseEmitter* dComIfGp_particle_getEmitter(u32){static JPABaseEmitter e;return &e;}
struct JPADynamicsBlockData{};struct JPABaseShapeData{};
struct JPADynamicsBlock{const JPADynamicsBlockData* mpData=nullptr;u32 getResUserWork()const{return 0;}u32 getFlag()const{return 0;}u32 getVolumeType()const{return 0;}u16 getVolumeSize()const{return 0;}s16 getMaxFrame()const{return 0;}s16 getStartFrame()const{return 0;}s16 getLifetime()const{return 0;}f32 getRate()const{return 1;}f32 getRateRndm()const{return 0;}f32 getInitVelDir()const{return 0;}void getEmitterDir(JGeometry::TVec3<f32>*)const{}void getEmitterTrs(JGeometry::TVec3<f32>*v)const{v->x=0;v->y=0;v->z=30;}void getEmitterRot(JGeometry::TVec3<s16>*v)const{v->x=v->y=v->z=0;}f32 getVolumeMinRad()const{return 0.1f;}void getEmitterScl(JGeometry::TVec3<f32>*v)const{v->x=1;v->y=1;v->z=0.1f;}f32 getVolumeSweep()const{return 0.15f;}u8 getRateStep()const{return 1;}u16 getVolumeSize24()const{return 24;}};
struct JPABaseShape{int isPrmAnm()const{return 0;}const JPABaseShapeData* pBsd=nullptr;u32 getType()const{return 0;}u32 getDirType()const{return 0;}u32 getRotType()const{return 0;}f32 getBaseSizeX()const{return 1;}f32 getBaseSizeY()const{return 1;}u32 getClrAnmType()const{return 0;}void getPrmClr(GXColor*)const{}void getEnvClr(GXColor*)const{}};
struct JPAExtraShape{int isEnableScaleAnm()const{return 1;}f32 getScaleInValueX()const{return 1;}f32 getScaleInValueY()const{return 1;}f32 getScaleOutValueX()const{return 1;}f32 getScaleOutValueY()const{return 1;}f32 getScaleInTiming()const{return 0;}f32 getScaleOutTiming()const{return 1;}};
struct JPAChildShape{};struct JPAExTexShape{};
struct JPAKeyBlock{u8 getID()const{return 0;}u8 getKeyNum()const{return 2;}int isLoop()const{return 1;}const float* getKeyData()const{static const float d[8]={0,0.1f,0,0,30,0.5f,0,0};return d;}};
struct JPAResource{JPADynamicsBlock* getDyn()const{static JPADynamicsBlock d;return &d;}JPABaseShape* getBsp()const{static JPABaseShape b;return &b;}JPAChildShape* getCsp()const{return nullptr;}JPAExtraShape* getEsp()const{static JPAExtraShape e;return &e;}JPAExTexShape* getEts()const{return nullptr;}u16 getUsrIdx()const{return 0xB9D;}u8 keyNum=2,fldNum=0,texNum=1;JPAKeyBlock** ppKey=([]{static JPAKeyBlock k;static JPAKeyBlock* a[2]{&k,&k};return a;})();};
struct JPAResourceManager{JPAResource* getResource(u16)const{static JPAResource r;return &r;}};
struct JPAEmitterManager{JPAResourceManager** pResMgrAry;u8 gidMax=1,ridMax=2;};
struct dPa_control_c{static JPAEmitterManager* getEmitterManager(){static JPAResourceManager rm;static JPAResourceManager* arr[2]{&rm,&rm};static JPAEmitterManager m;m.pResMgrAry=arr;return &m;}};
struct ResTIMG{s32 imageOffset;s32 paletteOffset;unsigned char pad[24];};
struct J3DTexture{u16 n;std::uint8_t* img;J3DTexture(u16 c,ResTIMG* r):n(c),img(reinterpret_cast<std::uint8_t*>(r)+r->imageOffset){}
 const std::uint8_t* getImgDataPtr(u16)const{return img;}u16 getNum()const{return n;}const ResTIMG* getResTIMG(u16)const{static ResTIMG r{};return &r;}};
struct J3DMaterial{};
struct J3DMatPacket{J3DTexture* mpTexture=nullptr;J3DMaterial* m=nullptr;J3DMaterial* getMaterial(){return m;}void setTexture(J3DTexture* t){mpTexture=t;}};
struct J3DModelData{J3DTexture* t=nullptr;J3DMaterial mat[3];J3DTexture* getTexture(){return t;}J3DMaterial* getMaterialNodePointer(u16 i){return &mat[i];}};
struct J3DModel{J3DModelData* d=nullptr;J3DMatPacket p[3];J3DModelData* getModelData(){return d;}J3DMatPacket* getMatPacket(u16 i){return &p[i];}};
struct Morf{J3DModel* m=nullptr;J3DModel* getModel(){return m;}};
struct Sph{unsigned char v=2;unsigned char GetAtAtp()const{return v;}void SetAtAtp(int a){v=(unsigned char)a;}};
struct St{int init_state=0;};
struct fopAc_ac_c{int profname=510;St state;};
struct Cur{cXyz pos{0,0,0};};
struct daE_OC_c:fopAc_ac_c{const char* mName="E_OC2";int mActionMode=0;s16 health=220;Cur current;cXyz eyePos{0,0,0};dKy_tevstr_c tevStr;Morf* mpMorf=nullptr;Sph mSphs_at[2];struct CcSph{float cx=0,cz=0,r=0;float GetR()const{return r;}float GetCX()const{return cx;}float GetCZ()const{return cz;}} mSphs_cc[2];};
fopAc_ac_c* fopAcM_SearchByID(unsigned){return nullptr;}
s8 fopAcM_GetRoomNo(const fopAc_ac_c*){return 3;}
const char* dComIfGp_getStartStageName(){return "F_SP121";}
struct LogSvc{void (*write)(void*,int,const char*)=nullptr;};
void console(void*,int,const char* s){std::puts(s);}LogSvc g_svc{console};LogSvc* svc_log=&g_svc;void* mod_ctx=nullptr;const int LOG_LEVEL_INFO=0;
namespace {
constexpr const char* kVersion="0.8.8";
devf_black2::Table table,normal_table;devf_variant::Registry registry;unsigned variant_logs=0,regen_tracked_logs=0;
devf_dyn::Ledger ledger;unsigned damage_logs=0,dynamic_logs=0;std::uint32_t sim_seq=0;
struct SphereIo {daE_OC_c* a;std::uint8_t get(unsigned i)const{return a->mSphs_at[i].GetAtAtp();}void set(unsigned i,std::uint8_t v){a->mSphs_at[i].SetAtAtp(v);}};
std::uint32_t stage_tag()noexcept{return 0;}
//@@GLOBALS@@
struct EyeSet {bool built=false,failed=false;std::uint8_t* memory[devf_eye::kVariants]{};J3DTexture* texture[devf_eye::kVariants]{};devf_eye::Stats stats;};
std::array<EyeSet,4> eye_sets{};   // parallel to `textures`: the private purple variants of each private atlas
struct Binding {bool bound=false;const void* model=nullptr;int texture=-1;bool reject_logged=false;std::uint64_t generation=0;};
std::array<Binding,devf_black2::capacity> bindings{};
// CP10.5 attack-power telemetry: once per actor generation (bounded), never per frame or per hit. It reports what was WRITTEN to the two
// colliders; it does not measure any damage taken by Link (link_damage=NOT_MEASURED).
void dlog(const char* ev,std::uint32_t id,std::uint32_t generation,const devf_variant::Entry* e,const devf_damage::Plan* p,int b0,int b1,int a0,int a1,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||damage_logs>=400)return;++damage_logs;char text[800]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_5_DAMAGE event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u source=%s classified_variant=%s encounter_id=%s native_atp=%d collider0_before=%d collider0_after=%d collider1_before=%d collider1_after=%d target_base=%d written=%s reason=%s link_damage=NOT_MEASURED visual_or_gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,generation,e?devf_variant::source_text(e->source):"NONE",e?devf_variant::variant_text(e->variant):"UNCLASSIFIED",e&&e->encounter?e->encounter:"NONE",
  p?int(p->native):-1,b0,a0,b1,a1,p?int(p->target):-1,p&&p->write?"YES":"NO",reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// CP10.6 dynamic telemetry: only on a change (group on/off, fury start/refresh/end, rejection, release), bounded per actor and per session.
void ylog(const char* ev,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry* d,const devf_dyn::Event* x,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||dynamic_logs>=400)return;if(d){if(d->events>=devf_dyn::kEventsPerActor)return;++d->events;}++dynamic_logs;char text[960]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_6_DYNAMIC event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u source=%s classified_variant=%s encounter_id=%s atp_from=%d atp_to=%d collider0_now=%d collider1_now=%d companion_id=%u companion_variant=%s companion_dxz=%.3f companion_dy=%.3f room=%d hp_pre=%d hp_post_native=%d fury_ticks_remaining=%u duration_ticks=%u reason=%s link_damage=NOT_MEASURED visual_or_gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,d?d->generation:0u,e?devf_variant::source_text(e->source):"NONE",d?devf_variant::variant_text(d->variant):"UNCLASSIFIED",e&&e->encounter?e->encounter:"NONE",
  x?int(x->from):-1,x?int(x->to):-1,x?int(x->c0):-1,x?int(x->c1):-1,x?x->companion:0u,x?devf_variant::variant_text(x->companion_variant):"NONE",x?x->dxz:0.0,x?x->dy:0.0,d?d->room:-1,
  x?x->pre_hp:-1,x?x->post_hp:-1,x?x->fury:0u,devf_dyn::kFuryTicks,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// CP10.6A: seven private purple variants of the SAME original atlas (t = k/7), built once per session on caller-owned copies. They are only ever
// bound to the EYE packet of one Black model; the original ResTIMG array, image bytes, GXTexObjs and J3DModelData are only READ.
bool build_eye_set(unsigned t,J3DTexture* original,const char*& why)noexcept{
 auto& set=eye_sets[t];
 if(set.built)return true;
 if(set.failed){why="EYE_VARIANTS_PREVIOUSLY_FAILED";return false;}
 set.failed=true;
 const std::uint8_t* source=original->getImgDataPtr(0);
 const std::size_t bytes=devf_tex::cmpr_level_bytes(128,128);
 const u16 n=original->getNum();
 const std::size_t header=std::size_t(n)*sizeof(ResTIMG),image_offset=(header+31)&~std::size_t(31);
 for(unsigned k=1;k<devf_eye::kVariants;++k){
  auto* memory=new(std::nothrow) std::uint8_t[image_offset+bytes+32];
  if(!memory){why="EYE_VARIANT_ALLOCATION";return false;}
  auto* base=reinterpret_cast<std::uint8_t*>((reinterpret_cast<std::uintptr_t>(memory)+31)&~std::uintptr_t(31));
  auto* res=reinterpret_cast<ResTIMG*>(base);std::uint8_t* image=base+image_offset;
  std::memcpy(image,source,bytes);
  devf_eye::Stats st;
  if(!devf_eye::recolor_cmpr(image,bytes,devf_eye::amount_pm(k),devf_eye::kPurple,st)||st.blocks!=bytes/8){delete[] memory;why="EYE_RECOLOR_CONTRACT";return false;}
  if(k==devf_eye::kVariants-1){set.stats=st;if(!st.endpoints_changed){delete[] memory;why="NO_GREEN_ENDPOINTS_IN_ATLAS_EYES_WOULD_NOT_CHANGE";return false;}}
  for(u16 i=0;i<n;++i){
   std::memcpy(&res[i],original->getResTIMG(i),sizeof(ResTIMG));
   res[i].imageOffset=static_cast<s32>(image-reinterpret_cast<std::uint8_t*>(&res[i]));
   res[i].paletteOffset=0;
  }
  J3DTexture* texture=nullptr;try{texture=new J3DTexture(n,res);}catch(...){texture=nullptr;}
  if(!texture){delete[] memory;why="EYE_J3DTEXTURE_ALLOCATION";return false;}
  for(u16 i=0;i<n;++i)if(texture->getImgDataPtr(i)!=image){why="EYE_TEXTURE_POINTER_CONTRACT";return false;}   // keep memory: the object may hold GX ids
  set.memory[k]=memory;set.texture[k]=texture;
 }
 set.built=true;set.failed=false;
 return true;
}
// ---- CP10.6C visual feedback: reads the approved CP10.6 logic state (group / fury) and fades an aura (Blue and Black) and purple eyes (Black).
// Aura = ONE native keyed level emitter per actor (Blue: 0x0A58, Black: 0x0B9D), driven like Link and kytag03 drive theirs: dComIfGp_particle_set(key,...) every tick
// (it creates or refreshes and moves the emitter), colour/alpha/scale set on the emitter, stopDrawParticle when done. NO sword matrix, sword model or Link bone is used: the
// emitter keeps the identity global rotation of JPABaseEmitter::init. An emitter nobody refreshes is removed by the engine itself (dPa_control_c::level_c::execute), so a
// death, a room change or an unload cannot orphan it. The engine's transient refusals (tevStr not ready, pool full) are bounded retries (strokes_policy.hpp). ----
int black_index_for(std::uint32_t id)noexcept{for(unsigned i=0;i<table.count;++i)if(table.regs[i].used&&table.regs[i].admitted_id==id)return static_cast<int>(i);return -1;}
const char* state_text(int level,bool target)noexcept{
 const auto st=devf_vis::state_of(level,target);
 return st==devf_vis::State::Active?"ACTIVE":st==devf_vis::State::FadingIn?"FADING_IN":st==devf_vis::State::FadingOut?"FADING_OUT":"INACTIVE";
}
//@@RESNAME@@
// Eyes and generic cleanup lines keep the CP10.6A prefix.
void xlog(const char* ev,devf_vis::Entry* v,const devf_variant::Entry* e,const char* part,const devf_vis::Event& x)noexcept{
 if(!svc_log||!svc_log->write||visual_logs>=400)return;
 if(v){if(v->events>=devf_vis::kEventsPerActor)return;++v->events;}
 ++visual_logs;char text[900]{};
 const auto variant=v?v->variant:devf_variant::Variant::None;
 const bool eyes_part=std::strcmp(part,"EYES")==0;
 const int level=eyes_part?(v?v->eye_level:0):(v?v->aura_level:0);
 const char* state=!v?"NONE":state_text(level,eyes_part?v->eye_target:v->aura_target);
 std::snprintf(text,sizeof(text),"DEVF_CP10_6A_VISUAL event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s source=%s encounter_id=%s part=%s state=%s level=%d eye_variant=%u of=%u reason=%s visual_pixels=HUMAN_RUNTIME_PENDING",
  ev,kVersion,v?v->id:0u,v?v->generation:0u,devf_variant::variant_text(variant),e?devf_variant::source_text(e->source):"NONE",e&&e->encounter?e->encounter:"NONE",part,state,level,
  x.eye_idx,devf_vis::kEyeVariants-1,x.reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
//@@VISUAL@@
// ---- CP10.5 base attack power (unchanged semantics) + CP10.6 ledger: every CP10.4-classified generation gets ONE record. Only a pristine native
// pair is `owned` (and so ever written); the others are still real Bokoblins that may count as somebody's companion. ----
void damage_drop(std::uint32_t id,const char* why)noexcept{
 auto* old=ledger.find(id);if(!old)return;
 visual_drop(id,why);
 if(old->owned&&(old->group||old->fury))ylog("DYNAMIC_RELEASED",id,nullptr,old,nullptr,why);   // actor gone or replaced: no collider is read or written
 dlog("DAMAGE_PROFILE_RELEASED",id,old->generation,nullptr,nullptr,-1,-1,-1,-1,why);
 ledger.release(*old);
}
void damage_apply(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* entry)noexcept{
 damage_drop(id,"NEW_GENERATION_OF_SAME_ACTOR_ID_NO_STATE_INHERITED");
 SphereIo io{a};
 const auto r=devf_dyn::admit(ledger,id,a,entry->generation,entry->variant,a->mName,io);
 if(!r.plan.accepted){dlog("DAMAGE_PROFILE_REJECTED",id,entry->generation,entry,&r.plan,r.b0,r.b1,r.c0,r.c1,devf_damage::why_text(r.plan.why));return;}
 dlog("DAMAGE_PROFILE_APPLIED",id,entry->generation,entry,&r.plan,r.b0,r.b1,r.c0,r.c1,r.plan.write?"FIRST_ADMITTED_EXECUTE_ABSOLUTE_TARGET_BOTH_SPHERES":"FIRST_ADMITTED_EXECUTE_NATIVE_ALREADY_EQUALS_TARGET_NO_WRITE");
}
// POST of an execute pair (after the NATIVE execute, before regeneration): companion snapshot, Blue group, Black fury.
void dynamic_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,int post_health)noexcept{
 ++sim_seq;
 auto* d=ledger.find(id);if(!d||d->identity!=a)return;
 SphereIo io{a};devf_dyn::Post in;in.post_health=post_health;in.action=a->mActionMode;in.room=fopAcM_GetRoomNo(a);
 in.x=a->current.pos.x;in.y=a->current.pos.y;in.z=a->current.pos.z;in.stage=stage_tag();in.seq=sim_seq;in.window=devf_dyn::window_for(registry.used);
 const auto ev=devf_dyn::post_update(ledger,*d,io,in,[](const devf_dyn::Entry& c){
  const auto* f=fopAcM_SearchByID(c.id);return f&&f==c.identity&&f->state.init_state!=3&&f->profname==510;});
 visual_post(a,id,e,d);   // CP10.6A: reads the logic state just computed above (d->group / d->fury), never recomputes it
 switch(ev.kind){
  case devf_dyn::Ev::BlueOn:ylog("BLUE_GROUP_ON",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::BlueOff:ylog("BLUE_GROUP_OFF",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::FuryStart:ylog("BLACK_FURY_START",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::FuryRefresh:ylog("BLACK_FURY_REFRESH",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::FuryEnd:ylog("BLACK_FURY_END",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::Rejected:ylog("DYNAMIC_REJECTED",id,e,d,&ev,ev.reason);break;
  default:break;
 }
}
// Unload: give back the exact NATIVE attack power (E_OC 1, E_OC2 2) to a still-live SAME object whose spheres still hold exactly what this
// module wrote last (base or dynamic value). Gone, replaced, destroyed or third-party-modified: nothing is read or written.
void damage_shutdown()noexcept{
 for(auto& e:ledger.entries){
  if(!e.used)continue;
  const auto* found=e.owned?fopAcM_SearchByID(e.id):nullptr;
  const bool live=found&&found==e.identity&&found->state.init_state!=3&&found->profname==510;
  std::uint8_t c0=0,c1=0;daE_OC_c* actor=nullptr;
  if(live){actor=static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(found));c0=actor->mSphs_at[0].GetAtAtp();c1=actor->mSphs_at[1].GetAtAtp();}
  const auto rp=devf_dyn::restore_plan(e,live,c0,c1);
  if(rp.write&&actor){actor->mSphs_at[0].SetAtAtp(e.native);actor->mSphs_at[1].SetAtAtp(e.native);}
  devf_damage::Plan shown;shown.native=e.native;shown.target=e.applied;shown.write=rp.write;
  dlog(rp.write?"DAMAGE_PROFILE_RESTORED":"DAMAGE_PROFILE_RELEASED",e.id,e.generation,nullptr,&shown,c0,c1,rp.write?e.native:c0,rp.write?e.native:c1,rp.why);
  ledger.release(e);
 }
}

}
#include "bench_main.inc"
