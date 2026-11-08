// CP06 FIX01 0.11.12 world laboratory. Compiles the PRODUCTION adapter (src/cp06f1_adapter.inc)
// against minimal stubs and drives it through a tick loop whose native parts are TRANSCRIBED
// from the official 40457c6 source (line references below), with Bokoblin Expansion 0.17.0's
// REAL x2 sweep (red_ai_policy.hpp) and REAL flash schedule (cp19f2_policy.hpp), and a hook
// dispatcher with the host's ordering (src/dusk/mods/svc/hook.cpp:138-145: priority descending,
// then registration order; PRE and POST alike).
// MODELED (no game data available to the lab): the sword kinematics of the B/C clips (three
// speed profiles), Link's cylinder placement (setCollisionPos), Link's reaction to a Tg hit
// and the shield front-range angle. Every runtime/visual claim stays HUMAN_RUNTIME_PENDING.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19f2_policy.hpp"
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/red_ai_policy.hpp"
static unsigned long long checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"CP06F1_WORLD_FAIL line=%d %s\n",__LINE__,#x);std::exit(1);}}while(false)
// ======================================================================= stubs
#define POST05_SOURCE_COMMIT "world-lab"
typedef float Mtx[3][4];typedef float (*MtxP)[4];
enum LogLevel{LOG_LEVEL_TRACE,LOG_LEVEL_DEBUG,LOG_LEVEL_INFO,LOG_LEVEL_WARN,LOG_LEVEL_ERROR};
struct ModContext{};enum HookAction{HOOK_CONTINUE=0,HOOK_SKIP_ORIGINAL=1};enum ModResult{MOD_OK=0,MOD_ERROR=1};
struct HookOptions{int priority=0;void* userdata=nullptr;};
#define HOOK_OPTIONS_INIT HookOptions{}
using PreFn=HookAction(*)(ModContext*,void*,void*,void*);using PostFn=void(*)(ModContext*,void*,void*,void*);
struct HookSlot{struct P{int prio;unsigned order;PreFn f;};struct Q{int prio;unsigned order;PostFn f;};std::vector<P> pre;std::vector<Q> post;};
static unsigned g_order=0;
template<class Tag>HookSlot& hook_slot(){static HookSlot s;return s;}
template<class T>void host_sort(std::vector<T>& v){std::stable_sort(v.begin(),v.end(),[](const T& a,const T& b){return a.prio!=b.prio?a.prio>b.prio:a.order<b.order;});}
namespace mods{
template<class T>T arg(void* args,int i){return static_cast<T>(static_cast<void**>(args)[i]);}
namespace hook{
template<class Tag>ModResult add_pre(PreFn f,const HookOptions* o=nullptr){auto& s=hook_slot<Tag>();s.pre.push_back({o?o->priority:0,g_order++,f});host_sort(s.pre);return MOD_OK;}
template<class Tag>ModResult add_post(PostFn f,const HookOptions* o=nullptr){auto& s=hook_slot<Tag>();s.post.push_back({o?o->priority:0,g_order++,f});host_sort(s.post);return MOD_OK;}
}}
template<class Tag>void run_pre(void** a){for(auto& p:hook_slot<Tag>().pre)p.f(nullptr,a,nullptr,nullptr);}
template<class Tag>void run_post(void** a){for(auto& p:hook_slot<Tag>().post)p.f(nullptr,a,nullptr,nullptr);}
template<class Tag>void clear_hooks(){hook_slot<Tag>()=HookSlot{};}
struct LinkModelDrawF1{};struct Cp04EocExecute{};struct LinkCollisionFx{};
inline constexpr unsigned kLogLineMax=512;
static std::vector<std::string> g_lines;
void lnk_write(LogLevel,const char* line)noexcept{g_lines.emplace_back(line);}
bool lnk_take(unsigned& lines,std::uint64_t& dropped,unsigned cap)noexcept{if(lines>=cap){++dropped;return false;}++lines;return true;}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}float abs()const{return std::sqrt(x*x+y*y+z*z);}};
struct GXColor{std::uint8_t r=0,g=0,b=0,a=255;};
struct cCcD_Obj;
struct cCcD_ObjHitInf{bool at_hit=false,tg_hit=false;cCcD_Obj* at_obj=nullptr;cCcD_Obj* tg_obj=nullptr;
 bool ChkAtHit()const{return at_hit;}cCcD_Obj* GetAtHitObj()const{return at_obj;}bool ChkTgHit()const{return tg_hit;}cCcD_Obj* GetTgHitObj()const{return tg_obj;}};
struct cCcD_Obj:cCcD_ObjHitInf{bool at_set=false,tg_set=false,at_shield=false,tg_shield=false;
 bool ChkAtSet()const{return at_set;}bool ChkTgSet()const{return tg_set;}void OnAtSetBit(){at_set=true;}void OffAtSetBit(){at_set=false;}
 bool ChkAtShieldHit()const{return at_shield;}bool ChkTgShieldHit()const{return tg_shield;}};
static unsigned g_sphere_writes=0;
struct dCcD_Sph:cCcD_Obj{cXyz c;float r=0;bool setter=false;const cXyz& GetC()const{return c;}float GetR()const{return r;}void SetC(const cXyz& v){c=v;setter=true;++g_sphere_writes;}void SetR(float v){r=v;setter=true;}};
struct dCcD_Cyl:cCcD_Obj{cXyz c;float r=35,h=180;bool shield=false;const cXyz& GetC()const{return c;}float GetR()const{return r;}float GetH()const{return h;}};
struct J3DModelData{unsigned short n=20;unsigned short getJointNum()const{return n;}};
struct J3DModel{J3DModelData d;Mtx m[20]{};J3DModelData* getModelData(){return &d;}MtxP getAnmMtx(int i){return m[i];}};
struct J3DFrameCtrl{float mFrame=0,mRate=1;float getFrame()const{return mFrame;}};
struct J3DTransformInfo{int x=0;};struct Quaternion{int x=0;};
struct daE_OC_c;
struct mDoExt_McaMorfSO{J3DModel model;J3DFrameCtrl mFrameCtrl;float morf=1;J3DTransformInfo ti;Quaternion q;bool has_old=true;daE_OC_c* owner=nullptr;unsigned calcs=0;
 J3DModel* getModel(){return &model;}float getFrame(){return mFrameCtrl.mFrame;}float getPlaySpeed(){return mFrameCtrl.mRate;}float getMorfRatio(){return morf;}
 J3DTransformInfo* getOldTransInfo(){return has_old?&ti:nullptr;}Quaternion* getOldQuaternion(){return has_old?&q:nullptr;}void modelCalc();};
enum{fpcNm_ALINK_e=253,fpcNm_E_OC_e=510};
struct daPy_py_c{int profname=fpcNm_ALINK_e;struct{int init_state=0;}state;struct{cXyz pos;}current;};
struct dKy_tevstr_c{GXColor TevKColor;};
struct daAlink_c:daPy_py_c{enum{PROC_WAIT=3,PROC_DAMAGE=0x5a,PROC_GUARD_SLIP=0x1d,PROC_GUARD_ATTACK=0x1e};dCcD_Cyl mTgCyls[3];int mProcID=PROC_WAIT;short mDamageTimer=0;dKy_tevstr_c tevStr;
 J3DModel *mpLinkModel=nullptr,*mpLinkFaceModel=nullptr,*mpLinkHatModel=nullptr,*mpLinkHandModel=nullptr,*mpDemoFCBlendModel=nullptr,*mpDemoFCTongueModel=nullptr,*mpDemoHLTmpModel=nullptr,*mpDemoHRTmpModel=nullptr;};
struct daE_OC_c{int profname=fpcNm_E_OC_e;struct{int init_state=0;}state;int mActionMode=0,mOcState=0;mDoExt_McaMorfSO* mpMorf=nullptr;dCcD_Sph mSphs_at[2];struct{cXyz pos;}current;struct{short x=0,y=0,z=0;}shape_angle;
 float yaw=0,field_0x6a0=0;cXyz root0;int profile=1;bool clip_b=true;unsigned apid=1;};
namespace post03{struct Instance{std::uint32_t id=0;std::uint64_t generation=0;};}
struct Ledger{post03::Instance s[64];const post03::Instance& slot(unsigned i)const{return s[i<64?i:0];}};
struct{struct{std::uint64_t tick=1;}p8;struct{struct{bool disabled=false;Ledger ledger;}obs;}eng;}g;
struct{struct{int variants=0;}posture;}lg;
struct NativeAux{int atp=0;};
namespace postbal01{enum class Variant:std::uint8_t{Blue,Red,Black,BlackFury,Unknown};struct Snapshot{bool verified=true;Variant variant=Variant::Black;};
 static Variant g_variant[64];inline Snapshot capture(const Ledger&,int,int slot,std::uint32_t,std::uint64_t,const NativeAux&){Snapshot s;s.variant=g_variant[slot&63];return s;}}
NativeAux read_aux(const daE_OC_c*){return {};}
static daE_OC_c* g_actor_slot[64];
int cp04_slot(daE_OC_c* a)noexcept{for(int i=0;i<64;++i)if(g_actor_slot[i]==a)return i;return -1;}
struct{bool enabled=true;std::uint64_t compensated=0;}c6;
struct{struct{float pending=0;float getItemLifeCount()const{return pending;}}play;}g_dComIfG_gameInfo;
static daAlink_c* g_link=nullptr;
daPy_py_c* dComIfGp_getLinkPlayer(){return g_link;}
#include "../src/cp06f1_core.hpp"
#include "reference/cp06f1_native_cross.inc"
// The production adapter, byte for byte (the source audit pins it).
#include "../src/cp06f1_adapter.inc"
// ===================================================== MODELED sword kinematics
// Local frame: x right, y up, z forward. Hand (bone 0xc origin) on a shoulder arc; blade = bone z axis.
struct Pose{float hand[3],dir[3];};
static float ease(float u,int profile){if(u<0)u=0;if(u>1)u=1;const float s=u*u*(3-2*u);if(profile==0)return u;if(profile==1)return s;return s*s*(3-2*s);}   // slow/linear, medium, fast middle
static Pose model_pose(bool b,int profile,float frame){
 Pose p;const float d2r=3.14159265f/180.0f;
 if(b&&profile==3){const float u=ease((frame-13.0f)/5.0f,2);const float a=(55.0f-110.0f*u)*d2r;   // early forward strike, mostly inside frames 13..18
  p.dir[0]=0.05f;p.dir[1]=std::sin(a);p.dir[2]=std::cos(a);const float sh[3]={10,110,10};for(int i=0;i<3;++i)p.hand[i]=sh[i]+50.0f*p.dir[i];}
 else if(!b&&profile==3){const float u=ease((frame-13.0f)/5.0f,2);const float a=(105.0f-210.0f*u)*d2r;
  p.dir[0]=std::sin(a);p.dir[1]=-0.12f;p.dir[2]=std::cos(a);const float sh[3]={0,100,15};for(int i=0;i<3;++i)p.hand[i]=sh[i]+45.0f*p.dir[i];}
 else if(b){const float u=ease((frame-11.0f)/12.0f,profile);const float a=(115.0f-170.0f*u)*d2r;   // overhead: up/back -> down/forward in the Y-Z plane
  p.dir[0]=0.05f;p.dir[1]=std::sin(a);p.dir[2]=std::cos(a);const float sh[3]={10,125,10};for(int i=0;i<3;++i)p.hand[i]=sh[i]+50.0f*p.dir[i];}
 else{const float u=ease((frame-9.0f)/13.0f,profile);const float a=(105.0f-210.0f*u)*d2r;      // horizontal: right -> left in the X-Z plane
  p.dir[0]=std::sin(a);p.dir[1]=-0.12f;p.dir[2]=std::cos(a);const float sh[3]={0,100,15};for(int i=0;i<3;++i)p.hand[i]=sh[i]+45.0f*p.dir[i];}
 const float n=std::sqrt(p.dir[0]*p.dir[0]+p.dir[1]*p.dir[1]+p.dir[2]*p.dir[2]);for(float& v:p.dir)v/=n;return p;
}
// World bone matrix of joint 0xc for the actor root `root` and yaw: columns x,y,z (blade = z), translation = hand.
static void bone_matrix(const daE_OC_c* a,const cXyz& root,float frame,Mtx out){
 const Pose p=model_pose(a->clip_b,a->profile,frame);const float c=std::cos(a->yaw),s=std::sin(a->yaw);
 auto rot=[&](const float v[3],float o[3]){o[0]=c*v[0]+s*v[2];o[1]=v[1];o[2]=-s*v[0]+c*v[2];};
 float z[3],h[3];rot(p.dir,z);rot(p.hand,h);float up[3]={0,1,0};float x[3]={up[1]*z[2]-up[2]*z[1],up[2]*z[0]-up[0]*z[2],up[0]*z[1]-up[1]*z[0]};
 float xn=std::sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);if(xn<1e-4f){x[0]=1;x[1]=0;x[2]=0;xn=1;}for(float& v:x)v/=xn;
 const float y[3]={z[1]*x[2]-z[2]*x[1],z[2]*x[0]-z[0]*x[2],z[0]*x[1]-z[1]*x[0]};
 for(int r=0;r<3;++r){out[r][0]=x[r];out[r][1]=y[r];out[r][2]=z[r];}out[0][3]=root.x+h[0];out[1][3]=root.y+h[1];out[2][3]=root.z+h[2];
}
// Engine model calc stand-in: every joint is a deterministic function of (frame, root); joint 0xc is the sword bone.
static int g_fault_calls=-1;   // >=0: the Nth modelCalc from now returns a perturbed pose (non-deterministic engine)
void mDoExt_McaMorfSO::modelCalc(){
 ++calcs;const float f=mFrameCtrl.mFrame;const bool fault=g_fault_calls==0;if(g_fault_calls>=0)--g_fault_calls;
 for(int j=0;j<20;++j){if(j==0xc){bone_matrix(owner,owner->current.pos,f,model.m[j]);continue;}
  for(int r=0;r<3;++r)for(int c=0;c<4;++c)model.m[j][r][c]=float(j*0.37+r*1.1+c*0.13)+f*float(0.01*(j+1))+(c==3?owner->current.pos.x*0.001f*float(r+1):0.0f);}
 if(fault)model.m[3][0][3]+=0.001f;
}
// ============================================== transcribed native pipeline
// Link (daAlink_c::setCollision d_a_alink.cpp:6761-6880 for the Tg bits and registration; GStts Move d_cc_d.cpp:86-91).
struct LinkSim{daAlink_c k;J3DModel models[4];GXColor drawn[4]{};GXColor after[4]{};GXColor orig[4]{};cXyz pos;float face=0;bool guard=false;int parry_from=-1,parry_to=-1;unsigned tg_apid=0,tg_old_apid=0;int life=80,hits=0,blocks=0,parries=0,hits_this_swing=0;std::uint64_t parry_ticks=0;};
static LinkSim L;static bool g_skip_link_collision_post=false,g_no_exp_draw_post=false;static float g_lean=0;
static std::vector<cCcD_Obj*> g_at_list,g_tg_list;
static void link_execute(std::uint64_t tick){
 daAlink_c& k=L.k;
 // reaction to the previous pass (MODELED checkDamageAction): one result per pass
 bool hit=false,shield=false;for(auto& c:k.mTgCyls){if(c.tg_hit){hit=true;shield=shield||c.tg_shield;}}
 if(hit){if(shield){if(k.mProcID==daAlink_c::PROC_GUARD_ATTACK){++L.parries;}else{++L.blocks;k.mProcID=daAlink_c::PROC_GUARD_SLIP;}}
  else{++L.hits;++L.hits_this_swing;--L.life;k.mDamageTimer=30;k.mProcID=daAlink_c::PROC_DAMAGE;}}
 else if(k.mProcID!=daAlink_c::PROC_GUARD_ATTACK)k.mProcID=daAlink_c::PROC_WAIT;
 if(L.parry_from>=0&&int(tick)>=L.parry_from&&int(tick)<=L.parry_to&&k.mProcID!=daAlink_c::PROC_DAMAGE)k.mProcID=daAlink_c::PROC_GUARD_ATTACK;
 else if(k.mProcID==daAlink_c::PROC_GUARD_ATTACK)k.mProcID=daAlink_c::PROC_WAIT;
 if(k.mDamageTimer>0)--k.mDamageTimer;   // damageTimerCount
 // setCollision
 L.tg_old_apid=L.tg_apid;L.tg_apid=0;      // mCcStts.Move()
 const bool shield_now=L.guard||k.mProcID==daAlink_c::PROC_GUARD_ATTACK;
 for(int i=0;i<3;++i){auto& c=k.mTgCyls[i];c.shield=shield_now;}
 const float ln=std::sqrt(L.pos.x*L.pos.x+L.pos.z*L.pos.z);const cXyz lean=ln>1?cXyz(-L.pos.x/ln*g_lean,0,-L.pos.z/ln*g_lean):cXyz();   // MODELED guard lean toward the attacker (bone-placed cylinders)
 k.mTgCyls[0].c=cXyz(L.pos.x+lean.x,L.pos.y,L.pos.z+lean.z);k.mTgCyls[0].h=180;k.mTgCyls[1].c=cXyz(L.pos.x+lean.x,L.pos.y+45,L.pos.z+lean.z);k.mTgCyls[1].h=90;k.mTgCyls[2].c=cXyz(L.pos.x+lean.x,L.pos.y+90,L.pos.z+lean.z);k.mTgCyls[2].h=90;
 for(auto& c:k.mTgCyls){if(k.mDamageTimer!=0||false){c.tg_set=false;c.tg_hit=false;c.tg_shield=false;c.tg_obj=nullptr;}else c.tg_set=true;g_tg_list.push_back(&c);}
 k.current.pos=L.pos;
 if(!g_skip_link_collision_post){void* args[1]={&k};run_post<LinkCollisionFx>(args);}
}
// Expansion pieces (REAL headers): x2 sweep state per actor, fury chip, global flash.
struct ExpActor{devf_red_ai::State sweep;bool fury=false,x2=false,charged=false;};
static ExpActor E[8];static devf_cp19f2::Flash g_exp_flash;static bool g_exp_on=true,g_cp06_on=true;
struct Cp06Stub{bool armed=false;float before=0;}g_cp06;
static void exp_post(void** args){
 auto* a=mods::arg<daE_OC_c*>(args,0);const int i=cp04_slot(a);if(i<0||!g_exp_on)return;auto& x=E[i];
 // CP19F2 chip: engine shield-hit record of the previous pass, fury blow, once per blow
 if(x.fury&&!x.charged){for(auto& s:a->mSphs_at)if(s.at_hit&&s.at_shield){bool link=false;for(auto& c:L.k.mTgCyls)link=link||s.at_obj==&c;
   if(link){x.charged=true;g_dComIfG_gameInfo.play.pending-=1.0f;devf_cp19f2::flash_start(g_exp_flash);break;}}}
 // CP17/CP18 x2 sweep, real function, this execute's Link position
 if(x.x2){devf_red_ai::Actor r;r.mode=devf_red_ai::Attack;r.state=a->mOcState;r.b=a->mOcState==1;r.c=a->mOcState==2;r.frame=a->mpMorf->getFrame();r.rate=a->mpMorf->getPlaySpeed();r.end=a->clip_b?42:47;
  r.tip={a->mSphs_at[0].c.x,a->mSphs_at[0].c.y,a->mSphs_at[0].c.z};r.hilt={a->mSphs_at[1].c.x,a->mSphs_at[1].c.y,a->mSphs_at[1].c.z};r.tip_r=a->mSphs_at[0].r;r.hilt_r=a->mSphs_at[1].r;r.at_set=a->mSphs_at[0].at_set&&a->mSphs_at[1].at_set;
  devf_red_ai::Link l;l.sampled=true;l.pos={L.pos.x,L.pos.y,L.pos.z};
  x.sweep.decided=true;x.sweep.owned=true;x.sweep.own_rate=devf_red_ai::kBurst;
  const auto sw=devf_red_ai::sweep(x.sweep,r,l);
  if(sw.write0){a->mSphs_at[0].c=cXyz(sw.c0.x,sw.c0.y,sw.c0.z);a->mSphs_at[0].r=sw.r0;}
  if(sw.write1){a->mSphs_at[1].c=cXyz(sw.c1.x,sw.c1.y,sw.c1.z);a->mSphs_at[1].r=sw.r1;}}
}
static void exp_post_hook(ModContext*,void* args,void*,void*){exp_post(static_cast<void**>(args));}
// CP06 (0.11.11) stand-in: an authenticated fury parry's exact -1 returned in the same execute.
static void cp06_capture(ModContext*,void* args,void*,void*){g_cp06.armed=g_cp06_on&&L.k.mProcID==daAlink_c::PROC_GUARD_ATTACK;g_cp06.before=g_dComIfG_gameInfo.play.pending;(void)args;}
static void cp06_decide(ModContext*,void*,void*,void*){if(g_cp06.armed&&g_dComIfG_gameInfo.play.pending==g_cp06.before-1.0f){g_dComIfG_gameInfo.play.pending+=1.0f;++c6.compensated;}g_cp06.armed=false;}
// Expansion draw hooks: transcription of on_link_model_draw / after_link_model_draw (expansion/mod.cpp:1741-1758).
struct{bool active=false;daAlink_c* link=nullptr;std::uint8_t r=0,g=0,b=0;}g_exp_save;
static HookAction exp_draw_pre(ModContext*,void* args,void*,void*){
 g_exp_save.active=false;if(!g_exp_on||!devf_cp19f2::flash_on(g_exp_flash))return HOOK_CONTINUE;auto* lk=mods::arg<daAlink_c*>(args,0);J3DModel* m=mods::arg<J3DModel*>(args,1);
 if(!lk||!m||lk!=g_link||!f1_flash_model(lk,m))return HOOK_CONTINUE;GXColor& kc=lk->tevStr.TevKColor;g_exp_save={true,lk,kc.r,kc.g,kc.b};
 devf_cp19f2::Rgb n;n.r=kc.r;n.g=kc.g;n.b=kc.b;const auto c=devf_cp19f2::flash_merge(n,devf_cp19f2::flash_color(g_exp_flash));kc.r=c.r;kc.g=c.g;kc.b=c.b;return HOOK_CONTINUE;}
static void exp_draw_post(ModContext*,void* args,void*,void*){if(!g_exp_save.active)return;g_exp_save.active=false;auto* lk=mods::arg<daAlink_c*>(args,0);if(lk!=g_exp_save.link)return;GXColor& kc=lk->tevStr.TevKColor;kc.r=g_exp_save.r;kc.g=g_exp_save.g;kc.b=g_exp_save.b;}
static bool g_foreign_writer=false;static J3DModel* g_third_party_model=nullptr;static unsigned g_third_party_draws=0,g_third_party_kept=0;static std::uint8_t g_third_party_paint=0;
static HookAction third_party_pre(ModContext*,void* args,void*,void*){if(g_third_party_model&&mods::arg<J3DModel*>(args,1)==g_third_party_model){auto* lk=mods::arg<daAlink_c*>(args,0);devf_cp19f2::Rgb n;n.r=lk->tevStr.TevKColor.r;n.g=lk->tevStr.TevKColor.g;n.b=lk->tevStr.TevKColor.b;
 g_third_party_paint=devf_cp19f2::flash_merge(n,devf_cp19f2::flash_color(g_exp_flash)).r;lk->tevStr.TevKColor.r=g_third_party_paint;}return HOOK_CONTINUE;}
static HookAction foreign_draw_pre(ModContext*,void* args,void*,void*){if(g_foreign_writer){auto* lk=mods::arg<daAlink_c*>(args,0);lk->tevStr.TevKColor.g=40;}return HOOK_CONTINUE;}
// Bokoblin execute: daE_OC_c::action/executeAttack/play/mtx_set/cc_set (d_a_e_oc.cpp:2415-2534, 1379-1500, 2537-2583).
static void eoc_execute(daE_OC_c* a){
 void* args[1]={a};run_pre<Cp04EocExecute>(args);
 a->mSphs_at[0].OffAtSetBit();a->mSphs_at[1].OffAtSetBit();
 auto& fc=a->mpMorf->mFrameCtrl;
 if(a->mActionMode==4&&(a->mOcState==1||a->mOcState==2)){
  const float my_float=ref_lunge(a->clip_b,fc.mFrame);
  if(ref_at_on(fc.mFrame)){a->mSphs_at[0].OnAtSetBit();a->mSphs_at[1].OnAtSetBit();}
  bool hit_link=false;for(auto& s:a->mSphs_at)if(s.at_hit){for(auto& c:L.k.mTgCyls)hit_link=hit_link||s.at_obj==&c;}
  if(hit_link&&(L.guard||L.k.mProcID==daAlink_c::PROC_GUARD_ATTACK||L.k.mProcID==daAlink_c::PROC_GUARD_SLIP)){fc.mRate=-1.0f;a->mOcState=3;}   // checkPlayerGuard -> recoil
  a->current.pos.x+=(my_float-a->field_0x6a0)*std::sin(a->yaw);a->current.pos.z+=(my_float-a->field_0x6a0)*std::cos(a->yaw);a->field_0x6a0=my_float;
 }
 float rate=fc.mRate;ref_frame_update(fc.mFrame,rate,0.0f,a->clip_b?42.0f:47.0f);if(a->mOcState!=3)fc.mRate=rate;
 if(a->mOcState==3&&fc.mFrame<=0.0f){a->mActionMode=3;a->mOcState=0;}
 if(a->mOcState!=3&&rate==0.0f&&a->mActionMode==4){a->mActionMode=3;a->mOcState=0;}
 a->mpMorf->modelCalc();
 // cc_set: centre = AnmMtx(0xc)*(0,0,70) r45 / (0,0,20) r30, registered when the AT bit is set
 MtxP m=a->mpMorf->model.getAnmMtx(0xc);
 a->mSphs_at[0].setter=a->mSphs_at[1].setter=false;
 a->mSphs_at[0].c=cXyz(m[0][2]*70+m[0][3],m[1][2]*70+m[1][3],m[2][2]*70+m[2][3]);a->mSphs_at[0].r=45;
 a->mSphs_at[1].c=cXyz(m[0][2]*20+m[0][3],m[1][2]*20+m[1][3],m[2][2]*20+m[2][3]);a->mSphs_at[1].r=30;
 for(auto& s:a->mSphs_at)if(s.at_set)g_at_list.push_back(&s);
 run_post<Cp04EocExecute>(args);
}
// cCcS::ChkAtTg (c_cc_s.cpp:323-367) + dCcS::ChkAtTgHitAfterCross NoConHit (d_cc_s.cpp:217-247) + SetAtTgGObjInf (530-608).
static std::vector<daE_OC_c*> g_actors;static bool g_last_front=true;
static void collision_pass(){
 for(auto* o:g_at_list){o->at_hit=false;o->at_shield=false;o->at_obj=nullptr;}
 for(auto* o:g_tg_list){o->tg_hit=false;o->tg_shield=false;o->tg_obj=nullptr;}
 for(auto* at:g_at_list){if(!at->ChkAtSet())continue;auto* s=static_cast<dCcD_Sph*>(at);daE_OC_c* owner=nullptr;for(auto* a:g_actors)if(&a->mSphs_at[0]==s||&a->mSphs_at[1]==s)owner=a;
  for(auto* tg:g_tg_list){if(!tg->ChkTgSet())continue;auto* c=static_cast<dCcD_Cyl*>(tg);
   cp06f1::Cyl k;k.c={c->c.x,c->c.y,c->c.z};k.r=c->r;k.h=c->h;cp06f1::Sph q;q.c={s->c.x,s->c.y,s->c.z};q.r=s->r;float pen=0;
   if(!ref_cM3d_Cross_CylSph(&k,&q,&pen))continue;
   L.tg_apid=owner->apid;if(L.tg_old_apid==owner->apid)continue;          // Link Tg OnTgNoConHit
   // ChkShield + front range (MODELED angle: the attacker within +-90 deg of Link's facing)
   const float dx=owner->current.pos.x-L.pos.x,dz=owner->current.pos.z-L.pos.z;const float fx=std::sin(L.face),fz=std::cos(L.face);
   const bool front=dx*fx+dz*fz>0;const bool sh=c->shield&&front;
   g_last_front=front;s->at_hit=true;s->at_obj=c;if(sh)s->at_shield=true;c->tg_hit=true;c->tg_obj=s;if(sh)c->tg_shield=true;
  }}
 g_at_list.clear();g_tg_list.clear();
}
static void draw_link(){
 // native draw: tevStr.TevKColor carries the native damage colour while mDamageTimer runs (d_a_alink.cpp:19546-19567, simplified schedule)
 GXColor native{};if(L.k.mDamageTimer>0)native.r=std::uint8_t(5+L.k.mDamageTimer%10);
 for(int i=0;i<4;++i){L.k.tevStr.TevKColor=native;L.orig[i]=native;void* args[2]={&L.k,&L.models[i]};run_pre<LinkModelDrawF1>(args);
  L.drawn[i]=L.k.tevStr.TevKColor;
  run_post<LinkModelDrawF1>(args);L.after[i]=L.k.tevStr.TevKColor;}
 if(g_third_party_model&&devf_cp19f2::flash_on(g_exp_flash)){L.k.tevStr.TevKColor=native;void* args[2]={&L.k,g_third_party_model};run_pre<LinkModelDrawF1>(args);
  ++g_third_party_draws;if(L.k.tevStr.TevKColor.r==g_third_party_paint)++g_third_party_kept;run_post<LinkModelDrawF1>(args);}
}
// ================================================================== scenarios
struct Install{bool f1=true,expansion_first=true,foreign=false;bool invariant=true;};
static void install(const Install& in){
 clear_hooks<LinkModelDrawF1>();clear_hooks<Cp04EocExecute>();clear_hooks<LinkCollisionFx>();f1=F1Runtime{};
 auto exp=[&]{if(!g_no_exp_draw_post)mods::hook::add_post<LinkModelDrawF1>(exp_draw_post);mods::hook::add_pre<LinkModelDrawF1>(exp_draw_pre);mods::hook::add_post<Cp04EocExecute>(exp_post_hook);};
 auto own=[&]{HookOptions b;b.priority=90;HookOptions a;a.priority=-250;mods::hook::add_post<Cp04EocExecute>(cp06_capture,&b);mods::hook::add_post<Cp04EocExecute>(cp06_decide,&a);if(in.f1)CHECK(cp06f1_install());};
 if(in.expansion_first){exp();own();}else{own();exp();}
 if(in.foreign){HookOptions h;h.priority=10;mods::hook::add_pre<LinkModelDrawF1>(foreign_draw_pre,&h);}
}

struct Scenario{short yaw=0;bool clip_b=true;int profile=1;float rate=2.0f;int rate_from=2;float dist=100,ang=0,dy=0;bool guard=false,face_away=false;int parry_from=-1,parry_to=-1;short iframes=0;postbal01::Variant variant=postbal01::Variant::BlackFury;bool fury=true,x2_sweep=true;};
struct Run{bool contact=false,shield=false,parry=false,front=true;int hits=0,blocks=0,parries=0;unsigned writes=0,probes=0,calcs=0;std::uint64_t contact_tick=0;bool justified=true;int red_draws=0;};
static daE_OC_c g_a;static mDoExt_McaMorfSO g_morf;static unsigned g_offset_writes=0,g_centre_checks=0;
static float field(const std::string& l,const char* key){const auto p=l.find(key);return p==std::string::npos?-1e9f:std::strtof(l.c_str()+p+std::strlen(key),nullptr);}
static Run run(const Scenario& sc,const Install& in){
 g_lines.clear();g.p8.tick=1;L=LinkSim{};g_link=&L.k;g_at_list.clear();g_tg_list.clear();g_actors.clear();g_exp_flash=devf_cp19f2::Flash{};g_dComIfG_gameInfo.play.pending=0;c6.compensated=0;g_cp06={};
 L.k.mpLinkModel=&L.models[0];L.k.mpLinkFaceModel=&L.models[1];L.k.mpLinkHatModel=&L.models[2];L.k.mpLinkHandModel=&L.models[3];
 const float d2r=3.14159265f/180.0f;const float yw=float(sc.yaw)*3.14159265f/32768.0f;L.pos=cXyz(sc.dist*std::sin(yw+sc.ang*d2r),sc.dy,sc.dist*std::cos(yw+sc.ang*d2r));
 L.face=std::atan2(-L.pos.x,-L.pos.z)+(sc.face_away?3.14159265f:0.0f);L.guard=sc.guard;L.parry_from=sc.parry_from;L.parry_to=sc.parry_to;L.k.mDamageTimer=sc.iframes;
 g_a=daE_OC_c{};g_morf=mDoExt_McaMorfSO{};g_a.mpMorf=&g_morf;g_morf.owner=&g_a;g_a.clip_b=sc.clip_b;g_a.profile=sc.profile;g_a.mActionMode=4;g_a.mOcState=sc.clip_b?1:2;g_a.apid=7;g_a.shape_angle.y=sc.yaw;g_a.yaw=yw;
 g_morf.mFrameCtrl.mFrame=0;g_morf.mFrameCtrl.mRate=1.0f;g_actors.push_back(&g_a);
 for(auto& x:g_actor_slot)x=nullptr;g_actor_slot[3]=&g_a;g.eng.obs.ledger.s[3]={7u,11u};postbal01::g_variant[3]=sc.variant;E[3]=ExpActor{};E[3].fury=sc.fury;E[3].x2=sc.x2_sweep&&sc.rate==2.0f;
 install(in);g_foreign_writer=in.foreign;
 Run out;const unsigned calcs0=0;(void)calcs0;
 for(int t=0;t<70&&g_a.mActionMode==4;++t){
  ++g.p8.tick;
  if(in.f1)cp06f1_maintenance();devf_cp19f2::flash_tick(g_exp_flash);
  if(g_morf.mFrameCtrl.mFrame>=float(sc.rate_from)-1e-3f&&g_a.mOcState!=3)g_morf.mFrameCtrl.mRate=sc.rate;
  link_execute(g.p8.tick);
  const unsigned w0=f1.place_writes;
  eoc_execute(&g_a);
  // probe restore invariant: the live pose equals a fresh native calc at the current frame
  if(in.invariant){const int fc=g_fault_calls;g_fault_calls=-1;mDoExt_McaMorfSO fresh=g_morf;fresh.calcs=0;fresh.modelCalc();g_fault_calls=fc;for(int j=0;j<20;++j)for(int r=0;r<3;++r)for(int c=0;c<4;++c)CHECK(fresh.model.m[j][r][c]==g_morf.model.m[j][r][c]);CHECK(g_morf.mFrameCtrl.mRate==g_a.mpMorf->mFrameCtrl.mRate);}
  if(f1.place_writes!=w0){out.writes+=f1.place_writes-w0;
   for(auto it=g_lines.rbegin();it!=g_lines.rend();++it){if(it->find("CP06F1_BACKFILL")==std::string::npos)continue;
    const float pre=field(*it," pre="),post=field(*it," post=");const float pose[2]={field(*it," tip_pose="),field(*it," hilt_pose=")};
    for(int j=0;j<2;++j){if(pose[j]<0||!g_a.mSphs_at[j].setter)continue;const float m=pose[j];
     const float d=m==post?0.0f:ref_lunge(g_a.clip_b,m-1.0f)-ref_lunge(g_a.clip_b,pre);if(std::fabs(d)>1.0f)++g_offset_writes;
     const cXyz root(g_a.current.pos.x+d*std::sin(g_a.yaw),g_a.current.pos.y,g_a.current.pos.z+d*std::cos(g_a.yaw));Mtx bm;bone_matrix(&g_a,root,m,bm);
     const cp06f1::Sph e=j?cp06f1::hilt(bm):cp06f1::tip(bm);const auto& c=g_a.mSphs_at[j].c;++g_centre_checks;
     if(!(std::fabs(c.x-e.c.x)<0.25f&&std::fabs(c.y-e.c.y)<0.25f&&std::fabs(c.z-e.c.z)<0.25f))std::printf("CENTRE j=%d m=%.4f post=%.4f pre=%.4f d=%.2f c=(%.2f,%.2f,%.2f) e=(%.2f,%.2f,%.2f)\n",j,m,post,pre,d,c.x,c.y,c.z,e.c.x,e.c.y,e.c.z);
     CHECK(std::fabs(c.x-e.c.x)<0.25f&&std::fabs(c.y-e.c.y)<0.25f&&std::fabs(c.z-e.c.z)<0.25f);}   // the vanilla registration of that pose (log precision 1e-3 frame)
    break;}
   // every written sphere reaches one of Link's ON cylinders natively in this very pass
   bool reach=false;for(int j=0;j<2;++j){auto& s=g_a.mSphs_at[j];if(s.setter)CHECK(s.r==(j?30.0f:45.0f));cp06f1::Sph q;q.c={s.c.x,s.c.y,s.c.z};q.r=s.r;for(auto& c:L.k.mTgCyls){cp06f1::Cyl k;k.c={c.c.x,c.c.y,c.c.z};k.r=c.r;k.h=c.h;float pen;if(c.tg_set&&ref_cM3d_Cross_CylSph(&k,&q,&pen))reach=true;}}CHECK(reach);   // a written pass always reaches an ON cylinder
  }
  collision_pass();
  for(auto& sph:g_a.mSphs_at)if(sph.at_hit){bool link=false;for(auto& c:L.k.mTgCyls)link=link||sph.at_obj==&c;if(link&&!out.contact){out.contact=true;out.shield=sph.at_shield;out.front=g_last_front;out.contact_tick=g.p8.tick;}}
  draw_link();
  for(int i=0;i<4;++i){CHECK(L.after[i].r==L.orig[i].r&&L.after[i].b==L.orig[i].b&&L.after[i].g==(in.foreign?40:L.orig[i].g));if(L.drawn[i].r>L.orig[i].r)++out.red_draws;   // exact restore; a foreign value is never touched
   if(in.foreign)CHECK(L.drawn[i].g==40);}
 }
 // one more Link execute consumes the last pass
 ++g.p8.tick;link_execute(g.p8.tick);g_tg_list.clear();
 out.hits=L.hits;out.blocks=L.blocks;out.parries=L.parries;out.probes=unsigned(f1.probes);out.calcs=g_morf.calcs;
 for(auto& sl:f1.slots)f1_close(sl,"LAB_END");
 return out;
}
// Dense truth: the vanilla x1 swept volume, poses 15..23 every 0.01 frame, each with its vanilla root lunge(f-1).
struct Truth{bool any=false,robust=false;float first=-1,best_len=0;};
static Truth truth(const Scenario& sc,float margin){
 Truth t;daE_OC_c a;a.clip_b=sc.clip_b;a.profile=sc.profile;const float yw=float(sc.yaw)*3.14159265f/32768.0f;a.yaw=yw;
 const float d2r=3.14159265f/180.0f;const cXyz lp(sc.dist*std::sin(yw+sc.ang*d2r),sc.dy,sc.dist*std::cos(yw+sc.ang*d2r));
 const float ln=std::sqrt(lp.x*lp.x+lp.z*lp.z);const float lx=ln>1?-lp.x/ln*g_lean:0,lz=ln>1?-lp.z/ln*g_lean:0;
 cp06f1::Cyl k[3];k[0].c={lp.x+lx,lp.y,lp.z+lz};k[0].r=35;k[0].h=180;k[1].c={lp.x+lx,lp.y+45,lp.z+lz};k[1].r=35;k[1].h=90;k[2].c={lp.x+lx,lp.y+90,lp.z+lz};k[2].r=35;k[2].h=90;
 float run_start=-1,len=0;
 for(int i=0;i<=800;++i){const float f=15.0f+0.01f*float(i);const float lu=ref_lunge(sc.clip_b,f-1.0f);const cXyz root(lu*std::sin(yw),0,lu*std::cos(yw));Mtx m;bone_matrix(&a,root,f,m);
  const cp06f1::Sph sp[2]={cp06f1::tip(m),cp06f1::hilt(m)};bool in=false;
  for(auto& s:sp)for(auto& c:k){cp06f1::Sph q=s;q.r-=margin;float pen;if(q.r>0&&ref_cM3d_Cross_CylSph(&c,&q,&pen))in=true;}
  if(in){t.any=true;if(t.first<0)t.first=f;if(run_start<0)run_start=f;len=f-run_start;if(len>t.best_len)t.best_len=len;}else run_start=-1;
 }
 t.robust=t.best_len>=1.05f;return t;
}

// ---------------------------------------------------------------- P0-B sequences
// Each tick: mod_update (our maintenance + Expansion's flash tick), then the scheduled E_OC executes
// (Expansion's chip in its POST(0) when scheduled: exact -1 + flash_start; CP06 returns it when the
// blow was a committed fury parry), then Link's four model draws through the real hook chain.
// Reference: two instances of Expansion's REAL Flash: shown (every chip) and legit (legit chips only).
enum class Ev:std::uint8_t{None,Legit,Parry};
struct SeqTick{Ev ev[2]{};short native_damage=0;bool paused=false;};
static Ev g_sched[64];
static void exp_chip_hook(ModContext*,void* args,void*,void*){auto* a=mods::arg<daE_OC_c*>(args,0);const int i=cp04_slot(a);if(i<0||!g_exp_on)return;
 if(g_sched[i]!=Ev::None){g_dComIfG_gameInfo.play.pending-=1.0f;devf_cp19f2::flash_start(g_exp_flash);}}
static bool g_parry_now[64];
static void cp06_cap2(ModContext*,void* args,void*,void*){auto* a=mods::arg<daE_OC_c*>(args,0);const int i=cp04_slot(a);g_cp06.armed=i>=0&&g_sched[i]==Ev::Parry;g_cp06.before=g_dComIfG_gameInfo.play.pending;}
static unsigned flash_sequence(const std::vector<SeqTick>& seq,bool f1_on,bool exp_first,bool foreign,unsigned& red_drawn,unsigned& mismatches){
 clear_hooks<LinkModelDrawF1>();clear_hooks<Cp04EocExecute>();clear_hooks<LinkCollisionFx>();f1=F1Runtime{};g_lines.clear();
 L=LinkSim{};g_link=&L.k;L.k.mpLinkModel=&L.models[0];L.k.mpLinkFaceModel=&L.models[1];L.k.mpLinkHatModel=&L.models[2];L.k.mpLinkHandModel=&L.models[3];
 static daE_OC_c act[2];static mDoExt_McaMorfSO mo[2];for(int i=0;i<2;++i){act[i]=daE_OC_c{};mo[i]=mDoExt_McaMorfSO{};act[i].mpMorf=&mo[i];mo[i].owner=&act[i];act[i].mActionMode=3;}
 for(auto& s:g_actor_slot)s=nullptr;g_actor_slot[5]=&act[0];g_actor_slot[9]=&act[1];g.eng.obs.ledger.s[5]={21u,1u};g.eng.obs.ledger.s[9]={22u,1u};
 g_exp_flash=devf_cp19f2::Flash{};g_dComIfG_gameInfo.play.pending=0;c6.compensated=0;g_cp06={};
 auto exp=[&]{if(!g_no_exp_draw_post)mods::hook::add_post<LinkModelDrawF1>(exp_draw_post);mods::hook::add_pre<LinkModelDrawF1>(exp_draw_pre);mods::hook::add_post<Cp04EocExecute>(exp_chip_hook);};
 auto own=[&]{HookOptions b;b.priority=90;HookOptions a;a.priority=-250;mods::hook::add_post<Cp04EocExecute>(cp06_cap2,&b);mods::hook::add_post<Cp04EocExecute>(cp06_decide,&a);if(f1_on)CHECK(cp06f1_install());};
 if(exp_first){exp();own();}else{own();exp();}
 if(foreign){HookOptions h;h.priority=-100;mods::hook::add_pre<LinkModelDrawF1>(foreign_draw_pre,&h);}g_foreign_writer=foreign;
 if(g_third_party_model){HookOptions h;h.priority=-100;mods::hook::add_pre<LinkModelDrawF1>(third_party_pre,&h);}
 devf_cp19f2::Flash shown,legit;unsigned draws=0;red_drawn=0;
 for(const auto& t:seq){
  ++g.p8.tick;if(f1_on)cp06f1_maintenance();devf_cp19f2::flash_tick(g_exp_flash);devf_cp19f2::flash_tick(shown);devf_cp19f2::flash_tick(legit);
  if(t.native_damage)L.k.mDamageTimer=t.native_damage;else if(L.k.mDamageTimer>0)--L.k.mDamageTimer;
  if(!t.paused){
   for(int i=0;i<2;++i){g_sched[i?9:5]=t.ev[i];void* args[1]={&act[i]};run_post<Cp04EocExecute>(args);g_sched[i?9:5]=Ev::None;
    if(t.ev[i]!=Ev::None){devf_cp19f2::flash_start(shown);if(t.ev[i]==Ev::Legit)devf_cp19f2::flash_start(legit);}}
   CHECK(g_dComIfG_gameInfo.play.pending<=0.0f);
  }
  draw_link();
  for(int i=0;i<4;++i){++draws;const devf_cp19f2::Rgb nat{L.orig[i].r,L.orig[i].g,L.orig[i].b};
   const auto want_shown=devf_cp19f2::flash_on(shown)?devf_cp19f2::flash_merge(nat,devf_cp19f2::flash_color(shown)):nat;
   const auto want_legit=devf_cp19f2::flash_on(legit)?devf_cp19f2::flash_merge(nat,devf_cp19f2::flash_color(legit)):nat;
   const auto& want=f1_on&&!foreign?want_legit:want_shown;
   CHECK(L.drawn[i].r==want.r&&L.drawn[i].b==want.b&&L.drawn[i].g==(foreign?40:want.g));
   if(!g_no_exp_draw_post)CHECK(L.after[i].r==L.orig[i].r&&L.after[i].b==L.orig[i].b);
   else if(f1_on&&!(L.drawn[i].r==L.orig[i].r))CHECK(L.after[i].r==(devf_cp19f2::flash_on(shown)?devf_cp19f2::flash_merge(nat,devf_cp19f2::flash_color(shown)).r:L.orig[i].r)||L.after[i].r==L.orig[i].r);
   if(L.drawn[i].r>L.orig[i].r)++red_drawn;}
 }
 mismatches=unsigned(f1.flash.c.mismatch);return draws;
}
static std::vector<SeqTick> make_seq(std::initializer_list<std::pair<int,std::pair<int,Ev>>> events,int len,int damage_at=-1,int pause_from=-1,int pause_to=-1){
 std::vector<SeqTick> s(std::size_t(len),SeqTick{});for(auto& e:events)s[std::size_t(e.first)].ev[e.second.first]=e.second.second;
 if(damage_at>=0)s[std::size_t(damage_at)].native_damage=30;for(int i=pause_from;i>=0&&i<=pause_to&&i<len;++i)s[std::size_t(i)].paused=true;return s;}
static void flash_matrix(){
 using P=std::pair<int,std::pair<int,Ev>>;unsigned red=0,mm=0,cases=0,removed_total=0;
 const std::vector<std::vector<SeqTick>> seqs={
  make_seq({P{3,{0,Ev::Parry}}},50),                                        // parry fury: chip neutralized, no red
  make_seq({P{3,{0,Ev::Legit}}},50),                                        // normal fury guard: red identical to Expansion
  make_seq({P{3,{0,Ev::Parry}},P{5,{0,Ev::Legit}}},60),                     // parry then immediate guard: second flash kept
  make_seq({P{3,{0,Ev::Legit}},P{8,{0,Ev::Parry}}},60),                     // guard then parry: legit remainder kept, nothing new
  make_seq({P{3,{0,Ev::Parry}},P{3,{1,Ev::Legit}}},60),                     // two Blacks, same tick, parry + block
  make_seq({P{3,{1,Ev::Legit}},P{4,{0,Ev::Parry}},P{20,{1,Ev::Parry}},P{24,{0,Ev::Legit}}},80),   // two Blacks, alternating orders
  make_seq({P{2,{0,Ev::Parry}},P{12,{0,Ev::Parry}},P{22,{1,Ev::Parry}},P{31,{0,Ev::Parry}},P{40,{1,Ev::Parry}}},90),   // parry burst
  make_seq({P{3,{0,Ev::Parry}}},60,6),                                      // real hit right after the parry: native red kept
  make_seq({P{3,{0,Ev::Parry}}},60,-1,5,25),                                // pause during the flash window (mod_update keeps ticking)
  make_seq({},40,4),                                                        // Blue/Red/Black normal: no chip at all, native red only
 };
 for(std::size_t n=0;n<seqs.size();++n)for(bool ef:{true,false}){
  unsigned rb=0,rc=0,mb=0,mc=0;flash_sequence(seqs[n],false,ef,false,rb,mb);flash_sequence(seqs[n],true,ef,false,rc,mc);removed_total+=f1.flash.c.red_removed;
  CHECK(mc==0);if(n==0||n==6)CHECK(rb>0&&rc==0);if(n==1)CHECK(rb==rc&&rb>0);if(n==9)CHECK(rb==rc&&f1.flash.c.counterfactual==0);
  unsigned rf=0,mf=0;flash_sequence(seqs[n],true,ef,true,rf,mf);if(n==0)CHECK(mf>0);   // a foreign writer between the PREs is never overwritten
  red+=rc;mm+=mc;cases+=3;
 }
 std::printf("P0B sequences=%u cases=%u counterfactual_red_removed_draws=%u mismatches_aligned=%u\n",unsigned(seqs.size()),cases,removed_total,mm);
 CHECK(removed_total>100);
}

static void robustness(){
 // the restore of the engine pose is VERIFIED: a non-deterministic calc disables coverage for the session, loudly
 {Scenario sc;sc.clip_b=true;sc.profile=2;sc.rate=2.0f;sc.guard=true;sc.ang=10;Install in;in.invariant=false;Run probe{};
  for(float dd=100;dd<=240&&probe.probes==0;dd+=20){sc.dist=dd;g_fault_calls=-1;probe=run(sc,in);}CHECK(probe.probes>0);   // a scenario that does probe
  bool found=false;for(int at=0;at<400&&!found;at+=1){g_fault_calls=at;const Run r=run(sc,in);g_fault_calls=-1;
   if(f1.probe_mismatch){found=true;CHECK(!f1.coverage);bool line=false;for(auto& l:g_lines)line=line||l.find("CP06F1_CRITICAL event=POSE_PROBE_RESTORE_MISMATCH")!=std::string::npos;CHECK(line);(void)r;}}
  CHECK(found);}
 // Link that did not run setCollision this tick (not registered for the pass): nothing is ever written
 {g_skip_link_collision_post=true;unsigned w=0;for(float ang=-30;ang<=30;ang+=10){Scenario sc;sc.rate=2.0f;sc.profile=2;sc.dist=100;sc.ang=ang;const Run r=run(sc,Install{});w+=r.writes;CHECK(f1.why[unsigned(cp06f1::Why::LinkNotCurrent)]>0);}g_skip_link_collision_post=false;CHECK(w==0);}
 // Expansion's draw POST missing: our own exact value is restored by our POST, never a foreign one
 {g_no_exp_draw_post=true;using P=std::pair<int,std::pair<int,Ev>>;unsigned red=0,mm=0;
  flash_sequence(make_seq({P{3,{0,Ev::Legit}},P{9,{0,Ev::Parry}}},50),true,true,false,red,mm);
  CHECK(mm==0&&f1.flash.c.restored_by_us>0&&f1.flash.c.restore_foreign==0);   // our merged legit value written over Expansion's parry restart, restored by us
  flash_sequence(make_seq({P{3,{0,Ev::Parry}}},40),true,true,false,red,mm);CHECK(red==0&&f1.flash.c.restore_not_needed>0);
  g_no_exp_draw_post=false;}
}
// Two Bokoblins at once (x2 fury from the front, x1.35 Red from the side): independent slots, one result each, no cross-talk.
static void two_actors(){
 unsigned both=0;
 for(float d1=80;d1<=160;d1+=40)for(float a2:{30.0f,55.0f,80.0f}){
  for(bool on:{false,true}){
   g_lines.clear();g.p8.tick=1;L=LinkSim{};g_link=&L.k;g_at_list.clear();g_tg_list.clear();g_actors.clear();g_exp_flash=devf_cp19f2::Flash{};g_dComIfG_gameInfo.play.pending=0;
   L.k.mpLinkModel=&L.models[0];L.k.mpLinkFaceModel=&L.models[1];L.k.mpLinkHatModel=&L.models[2];L.k.mpLinkHandModel=&L.models[3];L.pos=cXyz(0,0,0);L.face=0;L.guard=true;
   static daE_OC_c act[2];static mDoExt_McaMorfSO mo[2];const float d2r=3.14159265f/180.0f;
   for(int i=0;i<2;++i){act[i]=daE_OC_c{};mo[i]=mDoExt_McaMorfSO{};act[i].mpMorf=&mo[i];mo[i].owner=&act[i];act[i].clip_b=i==0;act[i].profile=2;act[i].mActionMode=4;act[i].mOcState=i==0?1:2;act[i].apid=unsigned(30+i);g_actors.push_back(&act[i]);}
   const float ang[2]={0.0f,a2*d2r};const float dist[2]={d1,110.0f};
   for(int i=0;i<2;++i){act[i].current.pos=cXyz(dist[i]*std::sin(ang[i]),0,dist[i]*std::cos(ang[i]));act[i].yaw=std::atan2(-act[i].current.pos.x,-act[i].current.pos.z);act[i].shape_angle.y=short(act[i].yaw*32768.0f/3.14159265f);}
   for(auto& s:g_actor_slot)s=nullptr;g_actor_slot[1]=&act[0];g_actor_slot[2]=&act[1];g.eng.obs.ledger.s[1]={41u,1u};g.eng.obs.ledger.s[2]={42u,1u};
   postbal01::g_variant[1]=postbal01::Variant::BlackFury;postbal01::g_variant[2]=postbal01::Variant::Red;E[1]=ExpActor{};E[1].x2=true;E[2]=ExpActor{};
   Install in;in.f1=on;install(in);
   int contacts[2]={0,0};
   for(int t=0;t<70&&(act[0].mActionMode==4||act[1].mActionMode==4);++t){++g.p8.tick;if(on)cp06f1_maintenance();devf_cp19f2::flash_tick(g_exp_flash);
    mo[0].mFrameCtrl.mRate=mo[0].mFrameCtrl.mFrame>=2.0f&&act[0].mOcState!=3?2.0f:mo[0].mFrameCtrl.mRate;mo[1].mFrameCtrl.mRate=act[1].mOcState!=3&&mo[1].mFrameCtrl.mRate>0?1.35f:mo[1].mFrameCtrl.mRate;
    link_execute(g.p8.tick);for(auto* a:g_actors)if(a->mActionMode==4)eoc_execute(a);collision_pass();
    for(int i=0;i<2;++i)for(auto& s:act[i].mSphs_at)if(s.at_hit){for(auto& c:L.k.mTgCyls)if(s.at_obj==&c)contacts[i]=1;}
    draw_link();}
   CHECK(L.hits==0);if(on&&contacts[0]&&contacts[1]){++both;CHECK(L.blocks>=2);}
   if(on)for(int i=1;i<=2;++i)CHECK(f1.slots[i].swing.id==0||f1.slots[i].swing.id==40u+unsigned(i));
  }
 }
 std::printf("TWO_ACTORS cases=9 both_contact=%u\n",both);CHECK(both>=3);
}

// Expansion's x2 reach model (one cylinder at Link's position) disagrees with Link's real bone-placed
// cylinders: the native current pose it displaces must be restored when it is the one that crosses.
static void lean_matrix(){
 unsigned robust=0,ghost=0,base_ghost=0,from_current=0,unjust=0,vanilla=0,vanilla_lost=0;g_lean=28;
 for(bool b:{true,false})for(int prof:{1,2,3})for(int rf:{1,2})for(float dist=60;dist<=220;dist+=20)for(float ang=-90;ang<=90;ang+=15){
  Scenario sc;sc.clip_b=b;sc.profile=prof;sc.rate=2.0f;sc.rate_from=rf;sc.dist=dist;sc.ang=ang;
  const Truth tr=truth(sc,cp06f1::kWriteSlack+0.25f),loose=truth(sc,0.0f);Install off;off.f1=false;
  const Run rb=run(sc,off);const Run rc=run(sc,Install{});
  for(auto& l:g_lines){if(l.find("CP06F1_BACKFILL")==std::string::npos)continue;const auto p=l.find(" post=");const auto t=l.find(" tip_pose=");const auto h=l.find(" hilt_pose=");
   const float post=std::strtof(l.c_str()+p+6,nullptr),tp=std::strtof(l.c_str()+t+10,nullptr),hp=std::strtof(l.c_str()+h+11,nullptr);if(tp==post||hp==post)++from_current;}
  if(rc.contact&&!rb.contact&&!loose.any)++unjust;
  if(tr.robust){++robust;if(!rb.contact)++base_ghost;if(!rc.contact)++ghost;}
  if(!b&&rf==2){Scenario v=sc;v.rate=1.0f;v.x2_sweep=false;const Run rv=run(v,off);if(rv.contact){++vanilla;if(!rc.contact)++vanilla_lost;}}   // integer poses: FIX01 covers exactly vanilla's
 }
 g_lean=0;std::printf("LEAN x2 robust=%u base_ghost=%u cand_ghost=%u native_current_restored=%u unjustified=%u\n",robust,base_ghost,ghost,from_current,unjust);
 std::printf("LEAN vanilla_x1_contacts(C,even)=%u lost=%u\n",vanilla,vanilla_lost);CHECK(robust>300&&ghost==0&&unjust==0&&from_current>0&&vanilla>50&&vanilla_lost==0);
}
// A third party paints the shield model (outside Expansion's draw scope) with exactly Expansion's
// expected red while its flash runs: never ours to change.
static void third_party_shield(){
 using P=std::pair<int,std::pair<int,Ev>>;unsigned red=0,mm=0;J3DModel shield;
 g_third_party_model=&shield;flash_sequence(make_seq({P{3,{0,Ev::Parry}}},40),true,true,false,red,mm);g_third_party_model=nullptr;
 std::printf("THIRD_PARTY shield_draws=%u kept=%u\n",g_third_party_draws,g_third_party_kept);CHECK(g_third_party_draws>20&&g_third_party_kept==g_third_party_draws);
}

#include <chrono>
static int bench(int reps){
 using clk=std::chrono::steady_clock;double t_base=0,t_cand=0;unsigned long long calc_base=0,calc_cand=0,armed=0,ticks=0;
 for(int rep=0;rep<reps;++rep)for(bool b:{true,false})for(int prof:{1,2,3})for(float rate:{1.15f,1.35f,2.0f})for(float dist=60;dist<=240;dist+=30)for(float ang=-60;ang<=60;ang+=30){
  Scenario sc;sc.clip_b=b;sc.profile=prof;sc.rate=rate;sc.dist=dist;sc.ang=ang;Install off;off.f1=false;off.invariant=false;Install on;on.invariant=false;
  auto a0=clk::now();const Run rb=run(sc,off);auto a1=clk::now();const Run rc=run(sc,on);auto a2=clk::now();
  t_base+=std::chrono::duration<double,std::micro>(a1-a0).count();t_cand+=std::chrono::duration<double,std::micro>(a2-a1).count();calc_base+=rb.calcs;calc_cand+=rc.calcs;armed+=f1.armed_ticks;ticks+=g.p8.tick;}
 // idle E_OC executes (not attacking): the coverage callback must be O(1)
 Install on;on.invariant=false;install(on);daE_OC_c idle;mDoExt_McaMorfSO mo;idle.mpMorf=&mo;mo.owner=&idle;idle.mActionMode=3;void* args[1]={&idle};
 auto i0=clk::now();for(int n=0;n<2000000;++n)run_post<Cp04EocExecute>(args);auto i1=clk::now();
 clear_hooks<Cp04EocExecute>();auto i2=clk::now();for(int n=0;n<2000000;++n)run_post<Cp04EocExecute>(args);auto i3=clk::now();
 // Link model draws with no flash running
 install(on);g_link=&L.k;L.k.mpLinkModel=&L.models[0];void* dargs[2]={&L.k,&L.models[0]};auto d0=clk::now();for(int n=0;n<2000000;++n){run_pre<LinkModelDrawF1>(dargs);run_post<LinkModelDrawF1>(dargs);f1.depth=0;}auto d1=clk::now();
 clear_hooks<LinkModelDrawF1>();auto d2=clk::now();for(int n=0;n<2000000;++n){run_pre<LinkModelDrawF1>(dargs);run_post<LinkModelDrawF1>(dargs);}auto d3=clk::now();
 const double idle_ns=(std::chrono::duration<double,std::nano>(i1-i0).count()-std::chrono::duration<double,std::nano>(i3-i2).count())/2e6;
 const double draw_ns=(std::chrono::duration<double,std::nano>(d1-d0).count()-std::chrono::duration<double,std::nano>(d3-d2).count())/2e6;
 std::printf("CP06F1_BENCH reps=%d swings=%llu sim_ticks=%llu armed_ticks=%llu world_us_base=%.0f world_us_cand=%.0f extra_modelCalc=%llu extra_modelCalc_per_armed_tick=%.3f idle_execute_callbacks_ns=%.1f link_draw_callbacks_ns_no_flash=%.1f note=MODELED_WORLD_NOT_FPS_REAL_COST_DOMINATED_BY_ENGINE_MODELCALC\n",
  reps,(unsigned long long)(reps*2*3*3*7*5),ticks,armed,t_base,t_cand,calc_cand-calc_base,armed?double(calc_cand-calc_base)/double(armed):0.0,idle_ns,draw_ns);
 return 0;
}
int main(int argc,char** argv){
 if(argc>2&&std::strcmp(argv[1],"--bench")==0)return bench(std::atoi(argv[2]));
 // ---------------------------------------------------------------- P0-A matrix
 const float rates[]={1.0f,1.05f,1.15f,1.30f,1.35f,1.5f,2.0f};
 unsigned scenarios=0,robust_cases=0,base_ghost=0,cand_ghost=0,cand_only=0,cand_only_unjustified=0,base_x2_ghost=0,writes=0,double_hit=0,shield_ok=0,back_hits=0,iframe_cases=0,iframe_contacts=0,parry_cases=0,parry_ok=0,parry_false=0;
 unsigned long long calcs_base=0,calcs_cand=0,armed=0;
 for(bool b:{true,false})for(int prof:{0,1,2,3})for(float rate:rates)for(int rf:{1,2})for(float dist=40;dist<=260;dist+=20)for(float ang=-150;ang<=150;ang+=15)for(float dy:{-30.f,0.f,30.f}){
  if(rate==1.0f&&rf==2)continue;
  Scenario sc;sc.yaw=short((scenarios%3)==0?0:(scenarios%3)==1?0x3000:-0x5a00);sc.clip_b=b;sc.profile=prof;sc.rate=rate;sc.rate_from=rf;sc.dist=dist;sc.ang=ang;sc.dy=dy;sc.guard=false;sc.variant=rate==2.0f?postbal01::Variant::BlackFury:postbal01::Variant::Black;sc.fury=rate==2.0f;
  const Truth tr=truth(sc,cp06f1::kWriteSlack+0.25f);const Truth loose=truth(sc,0.0f);
  Install off;off.f1=false;Install on;on.f1=true;on.expansion_first=(scenarios%2)==0;
  const Run rb=run(sc,off);calcs_base+=rb.calcs;const Run rc=run(sc,on);calcs_cand+=rc.calcs;writes+=rc.writes;armed+=f1.armed_ticks;
  ++scenarios;
  CHECK(rb.hits<=1&&rc.hits<=1);if(rc.hits>1)++double_hit;
  if(rc.contact&&!rb.contact){++cand_only;if(!loose.any)++cand_only_unjustified;}
  CHECK(!(rc.contact&&!rb.contact)||loose.any);          // no contact outside the vanilla swept volume
  if(rb.contact)CHECK(rc.contact);                        // never removes an engine contact
  if(tr.robust){++robust_cases;if(!rb.contact){++base_ghost;if(rate==2.0f)++base_x2_ghost;}if(!rc.contact){++cand_ghost;
    if(std::getenv("CP06F1_WORLD_DEBUG")){std::printf("GHOST clip=%c prof=%d rate=%.2f rf=%d dist=%.0f ang=%.0f dy=%.0f first=%.2f len=%.2f hits=%d why:",b?'B':'C',prof,rate,rf,dist,ang,dy,tr.first,tr.best_len,rc.hits);
     for(unsigned w=0;w<unsigned(cp06f1::Why::Count);++w)if(f1.why[w])std::printf(" %s=%llu",cp06f1::name(cp06f1::Why(w)),(unsigned long long)f1.why[w]);std::printf("\n");for(auto& l:g_lines)if(l.find("CP06F1_SWING")!=std::string::npos)std::printf("   %s\n",l.c_str());}}}
  if(rate==1.0f)CHECK(rc.writes==0||!rb.contact);         // vanilla rate: only a displaced native pose may be restored
  if(rc.contact&&!rc.shield)++back_hits;
 }
 std::printf("P0A scenarios=%u robust=%u base_ghost=%u (x2=%u) cand_ghost=%u cand_only_contacts=%u unjustified=%u writes=%u armed_ticks=%llu modelCalc base=%llu cand=%llu\n",scenarios,robust_cases,base_ghost,base_x2_ghost,cand_ghost,cand_only,cand_only_unjustified,writes,armed,calcs_base,calcs_cand);
 CHECK(scenarios>50000&&robust_cases>10000);
 CHECK(base_x2_ghost>0);                                   // the defect exists in the 0.11.11 pipeline (modeled kinematics)
 CHECK(cand_ghost==0);                                     // every robust vanilla contact reaches the engine
 CHECK(cand_only_unjustified==0&&cand_only>0);
 std::printf("P0A centre_checks=%u nonzero_root_offset_writes=%u\n",g_centre_checks,g_offset_writes);CHECK(g_centre_checks>1000&&g_offset_writes>0);
 // ----------------------------------------------- guard / back / i-frames / parry
 for(bool b:{true,false})for(float rate:{1.15f,1.35f,2.0f})for(float ang=-60;ang<=60;ang+=20)for(float dist=60;dist<=180;dist+=30){
  Scenario sc;sc.clip_b=b;sc.profile=2;sc.rate=rate;sc.dist=dist;sc.ang=ang;sc.fury=rate==2.0f;sc.variant=sc.fury?postbal01::Variant::BlackFury:postbal01::Variant::Red;
  Install on;
  {Scenario g2=sc;g2.guard=true;const Run r=run(g2,on);if(r.contact){CHECK(r.shield==r.front);if(r.front){CHECK(r.hits==0&&r.blocks==1);++shield_ok;}else CHECK(r.hits==1);}}   // engine shield rule: front range at the contact
  {Scenario g2=sc;g2.guard=true;g2.face_away=true;const Run r=run(g2,on);if(r.contact){CHECK(r.shield==r.front);if(!r.front)++back_hits;}}
  {Scenario g2=sc;g2.iframes=60;const Run r=run(g2,on);++iframe_cases;if(r.contact)++iframe_contacts;CHECK(!r.contact&&r.writes==0);}
  {Scenario g2=sc;const Run probe=run(g2,on);if(probe.contact){const int ct=int(probe.contact_tick);
    Scenario p=sc;p.parry_from=ct-3;p.parry_to=ct+1;const Run r=run(p,on);if(r.contact&&!r.front){CHECK(r.parries==0&&r.hits==1);}else{++parry_cases;if(r.parries==1&&r.hits==0)++parry_ok;else std::printf("PARRYFAIL clip=%c rate=%.2f ang=%.0f dist=%.0f contact=%d front=%d parries=%d hits=%d blocks=%d\n",b?'B':'C',rate,ang,dist,r.contact,r.front,r.parries,r.hits,r.blocks);}
    Scenario late=sc;late.parry_from=ct+2;late.parry_to=ct+6;const Run rl=run(late,on);CHECK(rl.parries==0);if(rl.parries)++parry_false;}}
 }
 std::printf("P0A guard_blocks=%u iframe_cases=%u iframe_contacts=%u parry_cases=%u parry_ok=%u late_parry_false=%u\n",shield_ok,iframe_cases,iframe_contacts,parry_cases,parry_ok,parry_false);
 CHECK(shield_ok>50&&iframe_contacts==0&&parry_cases>50&&parry_ok==parry_cases);
 // --------------------------------------------------------------- P0-B flash matrix
 unsigned flash_cases=0;
 auto fury_guard=[](int parry)->Scenario{Scenario s;s.clip_b=true;s.profile=1;s.rate=2.0f;s.dist=110;s.ang=0;s.guard=true;s.fury=true;s.variant=postbal01::Variant::BlackFury;(void)parry;return s;};
 {Scenario s=fury_guard(0);const Run b=run(s,Install{false});const Run c=run(s,Install{true});
  CHECK(b.contact&&b.shield&&g_dComIfG_gameInfo.play.pending==-1.0f);CHECK(c.contact&&c.shield&&b.red_draws==c.red_draws&&c.red_draws>0);++flash_cases;}   // normal fury guard: chip and its red kept, identical
 {Scenario s=fury_guard(0);const Run probe=run(s,Install{true});const int ct=int(probe.contact_tick);s.parry_from=ct-3;s.parry_to=ct+1;
  Install off;off.f1=false;const Run b=run(s,off);CHECK(b.parries==1&&g_dComIfG_gameInfo.play.pending==0.0f&&b.red_draws>0);   // 0.11.11: chip neutralized, red still shown (the reported defect)
  const Run c=run(s,Install{true});CHECK(c.parries==1&&g_dComIfG_gameInfo.play.pending==0.0f&&c.red_draws==0&&f1.flash.c.counterfactual>0&&f1.flash.c.red_removed>0);++flash_cases;
  Install rev;rev.expansion_first=false;const Run c2=run(s,rev);CHECK(c2.red_draws==0&&c2.parries==1);++flash_cases;     // reversed registration order
  Install fo;fo.foreign=true;const Run c3=run(s,fo);CHECK(c3.parries==1&&f1.flash.c.mismatch>0&&f1.flash.c.counterfactual==0);++flash_cases;   // foreign writer never overwritten
 }
 // native damage colour during an open fury hit is never lowered (out-of-order: hit first, i-frames red)
 {Scenario s=fury_guard(0);s.guard=false;const Run c=run(s,Install{true});CHECK(c.hits==1);++flash_cases;}
 std::printf("P0B flash_cases=%u\n",flash_cases);
 flash_matrix();
 robustness();
 two_actors();
 lean_matrix();
 third_party_shield();
 std::printf("CP06F1_WORLD_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS modeled=SWORD_KINEMATICS_LINK_CYLINDER_PLACEMENT_LINK_REACTION_SHIELD_ANGLE transcribed=EXECUTEATTACK_LUNGE_AT_WINDOW_PLAY_CC_SET_CHKATTG_NOCONHIT_SETCOLLISION real=EXPANSION_SWEEP_AND_FLASH_HEADERS runtime=HUMAN_RUNTIME_PENDING\n",cp06f1::kVersion,checks);
 return 0;
}
