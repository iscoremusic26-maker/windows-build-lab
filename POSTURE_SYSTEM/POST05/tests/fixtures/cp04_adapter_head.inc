#include "ROOT/src/balance_adapter.hpp"
#include "ROOT/src/posture_core.hpp"
#include "ROOT/src/parry_core.hpp"
#include "ROOT/src/lnks2_core.hpp"
#include "ROOT/src/lnks2fix_core.hpp"
#include "ROOT/src/cp04_combo_core.hpp"
#include "ROOT/src/shield_chain_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace post03;
using s16=short;using f32=float;using u8=unsigned char;using BOOL=int;
static unsigned long long checks=0;
void ck(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"CP04_ADAPTER_FAIL %s\n",why);std::exit(1);}}

#define POST05_SOURCE_COMMIT "LAB_CALLBACKS_NOT_NATIVE_EXECUTION"
enum{LOG_LEVEL_INFO,MOD_OK,MOD_ERROR};using ModResult=int;struct ModContext{};enum HookAction{HOOK_CONTINUE};
struct HookOptions{unsigned size;int priority;int conflict;void* data;};
#define HOOK_OPTIONS_INIT {sizeof(HookOptions),0,0,nullptr}
static int registration_priorities[6]{};static unsigned registration_count=0;
namespace mods{template<class T>T arg(void* raw,int n){return *static_cast<T*>(static_cast<void**>(raw)[n]);}
namespace hook{template<class T,class C>int add_pre(C,const HookOptions* o){registration_priorities[registration_count++]=o->priority;return MOD_OK;}template<class T,class C>int add_post(C,const HookOptions* o){registration_priorities[registration_count++]=o->priority;return MOD_OK;}}}
struct Cp04EocExecute{};struct AttackObserveFix01{};struct LinkStickFx{};struct Cp04GuardInit{};struct LinkDamageFix01{};
struct State{int init_state=0;};enum{fpcNm_E_OC_e=10,fpcNm_ALINK_e=20};

struct fopAc_ac_c{unsigned id=1;int profname=fpcNm_E_OC_e;State state;};
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}};struct Angle{short y=0;};
using MtxP=float(*)[4];struct Model{float m[3][4]{};MtxP getAnmMtx(int){return m;}};
static bool player_guard=false;static unsigned native_shocks=0;using HitActor=fopAc_ac_c;static HitActor hit_actor;
struct HitObj{HitActor* GetAc(){return &hit_actor;}};
struct cCcD_ObjHitInf{bool on=false,hit=false;bool ChkAtSet()const{return on;}bool ChkAtHit()const{return hit;}void OnAtSetBit(){on=true;}HitObj* GetAtHitObj(){static HitObj h;return &h;}};

struct Frame{float end=42,frame=0,rate=1;float getEnd()const{return end;}float getFrame()const{return frame;}float getRate()const{return rate;}};
struct Morf{Frame mFrameCtrl;int clip=28;bool stopped=false;float prev=0;Model model;Model* getModel(){return &model;}bool checkFrame(float f){return mFrameCtrl.frame<=f&&f<mFrameCtrl.frame+mFrameCtrl.rate;}float getFrame()const{return mFrameCtrl.frame;}float getPlaySpeed()const{return mFrameCtrl.rate;}void setPlaySpeed(float r){mFrameCtrl.rate=r;}bool isStop()const{return stopped;}};

struct daE_OC_c:fopAc_ac_c{int health=220,mActionMode=4,mOcState=0,mOldActionMode=3,field_0x6e3=0,room=0;short field_0x6c2=0,field_0x6c0=0;float speedF=0;bool water=false,blue=true;bool mBattleOn=false;int field_0x6ca=150;float field_0x6a0=0;short mPrevShapeAngle=0;Angle shape_angle;struct{cXyz pos;Angle angle;}current;
 unsigned mParticleKeys[2]{};int tevStr=0;const char* mName="E_OC";
 struct Sound{unsigned calls[6]{};void startCreatureVoice(int n,int){++calls[n];}void startCreatureSound(int n,int,int){++calls[n];}}mSound;
 void executeAttack();void setBck(int b,int,float,float r){morf.clip=b;morf.mFrameCtrl.frame=0;morf.prev=-1;morf.mFrameCtrl.rate=r;morf.mFrameCtrl.end=b==5?42:b==6?47:60;morf.stopped=false;}
 Morf morf;Morf* mpMorf=&morf;cCcD_ObjHitInf mSphs_at[2];
 bool checkBck(int b){return mpMorf->clip==b;}bool checkBeforeFloorBg(float){return water;}
 void setActionMode(int m,int s){mOldActionMode=mActionMode;mActionMode=m;mOcState=s;field_0x6c2=field_0x6c0=0;}
};
struct daAlink_c:fopAc_ac_c{enum{PROC_GUARD_ATTACK=0x1e,PROC_GUARD_SLIP=0x1d,ANM_SHIELD_ATTACK=0x2e,BTN_R=0x40,BTN_B=8,BTN_A=0x10};
 void* mShieldModel=this;int mDamageTimer=0,mProcID=3,mModeFlg=0,mEquipItem=0x103,mItemButton=0,mItemTrigger=0;bool wolf=false,draw=true,actual_clip=true;
 Frame mUnderFrameCtrl[1];float field_0x3478=1,field_0x347c=12,mNormalSpeed=0;struct{int field_0x300a=0;}mProcVar1;
 bool checkShieldDraw(){return draw;}bool checkUnderMove0BckNoArc(int){return actual_clip;}int procGuardAttackInit();
} player;
unsigned fopAcM_GetID(const fopAc_ac_c* a){return a->id;}int fopAcM_GetRoomNo(const daE_OC_c* a){return a->room;}
static int layer=0,window=0,life=40;static bool pause=false,event=false;static const char* stage="LAB";
struct dComIfG_play_c{static int getLayerNo(int){return layer;}};
const char* dComIfGp_getStartStageName(){return stage;}int dMeter2Info_getWindowStatus(){return window;}
struct FakeEvent{int runCheck(){return event;}};struct LifeValue{int getLife(){return life;}};struct PlayerData{LifeValue getPlayerStatusA(){return {};}};
struct{struct{bool isPauseFlag(){return pause;}FakeEvent* getEvent(){static FakeEvent e;return &e;}}play;struct{PlayerData getPlayer(){return {};}}info;}g_dComIfG_gameInfo;
struct FakeRx{bool off=false;bool session_disabled(){return off;}};
struct{post04::PostureEngine pe;post03::Engine& eng=pe.eng;FakeRx rx;int layout=1;post08::LinkTrack link_track;struct{std::uint64_t tick=1;}p8;}g;
struct{lnk01::Posture posture;}lg;
struct{
 bool enabled=true,sample_ok=false,bash_offered_prev=false,bash_offered_now=false,bash_pending=false,b_consumed=false,r_held=false,b_trig=false,a_trig=false;
 std::uintptr_t key=0;std::uint64_t sample_tick=0,executes=0,r_down=0,r_up=0,input_dropped=0,shield_dropped=0,b_consumed_n=0,failures=0;
 unsigned input_lines=0,shield_lines=0;lnkfx::Defense def;
}lf;
struct{lnk05::Machine m;}lb;
bool lnkfx_link(const daAlink_c* a,std::uint32_t& id){if(a!=&player||a->state.init_state==3||a->wolf)return false;id=a->id;return true;}
void lnk_write(int,const char*){}
post03::NativeAux read_aux(const daE_OC_c* a){post03::NativeAux x;x.valid=1;x.resource=unsigned(a->blue?post03::NameKind::E_OC:post03::NameKind::E_OC2);x.atp=a->blue?1:2;return x;}

#include "ROOT/tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/blue_ai_policy.hpp"
enum{E_OC_ACTION_MOVE_OUT=16,E_OC_ACTION_FIND=3};
enum{Z2SE_EN_OC_V_ATTACK_B=0,Z2SE_EN_OC_V_ATTACK_C=0,Z2SE_EN_OC_V_ATTACK_B2=1,Z2SE_EN_OC_ATTACK_B=2,Z2SE_EN_OC_CLUB_HIT=3,Z2SE_EN_OC_NATA_HIT=3,Z2SE_EN_OC_V_ATTACK_C2=1,Z2SE_EN_OC_ATTACK_C=2,Z2SE_EN_OC_V_WAIT_ST=4,ID_ZM_S_ORCATTACKBLUR00,ID_ZM_S_ORCATTACKBLUR01};
namespace E_OC_n{const float oc_attackb_trans[10]={0,0,0,0,15.3f,38.4f,63.3f,86.2f,103.3f,112};const float oc_attackc_trans[10]={0,31.3f,72.7f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f};}
float cM_ssin(short x){return std::sin(x*3.14159265f/32768);}float cM_scos(short x){return std::cos(x*3.14159265f/32768);}
bool cLib_chaseF(float* v,float target,float step){if(*v<target)*v=std::min(target,*v+step);else *v=std::max(target,*v-step);return *v==target;}
static bool variant_c=false;float cM_rndF(float){return variant_c?.75f:.25f;}
static float player_distance=300;short fopAcM_searchPlayerAngleY(daE_OC_c*){return 0;}float fopAcM_searchPlayerDistance(daE_OC_c*){return player_distance;}
using fopAc_ac_c_hit=HitActor;
HitActor* dCc_GetAc(HitActor* a){return a;}int fopAcM_GetName(HitActor* a){return a->profname;}
struct Py {bool checkPlayerGuard(){return player_guard;}};Py* daPy_getPlayerActorClass(){static Py p;return &p;}
struct Vibration{void StartShock(int,int,cXyz){++native_shocks;}};Vibration& dComIfGp_getVibration(){static Vibration v;return v;}
int dPa_RM(int i){return i;}template<class...A>unsigned dComIfGp_particle_set(unsigned k,A...){return k+1;}

void lnkfx_fail(){lf.enabled=false;++lf.failures;}
void lnkfx_write(int,const char*){}
void lnkfx_penalty_log(const lnkfx::Out&){}
bool lnk_take(unsigned& n,std::uint64_t& dropped,unsigned cap){if(n>=cap){++dropped;return false;}++n;return true;}
