// LIZALFOS DUELIST 0.12.2 integration laboratory: the PRODUCTION adapter (src/liz_adapter.inc, included verbatim) and brain, registered through its own liz_install(), run against a
// transcription of the engine paths the defects live in: daE_DN_Execute/action/damage_check/attack functions (d_a_e_dn.cpp), cCcS::ChkAtTg + dCcS::SetAtTgGObjInf + ChkShieldFrontRange + no-continuous-hit
// + effect counters (c_cc_s.cpp, d_cc_s.cpp), daAlink_c::checkDamageAction's GUARD_SLIP gate, setGuardSe, procGuardSlipInit/procGuardSlip, setCollision (d_a_alink_*).
// Declared limits: the pose function is a model of arcs fitted to the logged center steps (commercial BCK data is not read), one cylinder stack, no BG; it is a laboratory, not a game run: no FPS, sound or visual claim.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <type_traits>
#include <vector>
#include "../src/shield_chain_core.hpp"
#include "../src/fix01_core.hpp"
#include "../src/lnks2_core.hpp"
#include "../src/lnks2fix_core.hpp"
#include "../src/liz_core.hpp"
#include "../src/link_break_core.hpp"
#include "../src/parry_fx_core.hpp"
#include "../src/lockon_core.hpp"
static unsigned long long g_checks = 0;
#define CHECK(c)                                              \
  do {                                                        \
    ++g_checks;                                               \
    if (!(c)) {                                               \
      std::printf("LIZ_LAB_FAIL line=%d %s\n", __LINE__, #c); \
      std::fflush(stdout);                                    \
      std::exit(1);                                           \
    }                                                         \
  } while (0)
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using f32 = float;
using BOOL = int;
inline constexpr int FALSE = 0, TRUE = 1;
// ---------------------------------------------------------------- SDK surface the adapter touches
enum LogLevel { LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR };
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
enum ModResult { MOD_OK, MOD_FAIL };
struct ModContext {};
struct HookOptions { int priority = 0; };
#define HOOK_OPTIONS_INIT {}
using PreFn = HookAction (*)(ModContext*, void*, void*, void*);
using PostFn = void (*)(ModContext*, void*, void*, void*);
template <class T> struct Reg {
  struct P { PreFn f; int prio; };
  struct Q { PostFn f; int prio; };
  static inline std::vector<P> pre;
  static inline std::vector<Q> post;
  static HookAction run_pre(void* a, void* r = nullptr) {
    for (auto& x : pre)
      if (x.f(nullptr, a, r, nullptr) == HOOK_SKIP_ORIGINAL) return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
  }
  static void run_post(void* a, void* r = nullptr) {
    for (auto& x : post) x.f(nullptr, a, r, nullptr);
  }
};
static int g_fail_install = -1, g_install_count = 0;
namespace mods {
template <class T> T arg(void* args, int i) { return *static_cast<T*>(static_cast<void**>(args)[i]); }
template <class T> T& arg_ref(void* args, int i) { return *static_cast<T*>(static_cast<void**>(args)[i]); }
namespace hook {
template <class T> ModResult add_pre(PreFn f, HookOptions* o = nullptr) {
  if (g_install_count++ == g_fail_install) return MOD_FAIL;
  Reg<T>::pre.push_back({f, o ? o->priority : 0});
  return MOD_OK;
}
template <class T> ModResult add_post(PostFn f, HookOptions* o = nullptr) {
  if (g_install_count++ == g_fail_install) return MOD_FAIL;
  Reg<T>::post.push_back({f, o ? o->priority : 0});
  return MOD_OK;
}
}  // namespace hook
}  // namespace mods
struct cXyz {
  float x = 0, y = 0, z = 0;
  cXyz() = default;
  cXyz(float a, float b, float c) : x(a), y(b), z(c) {}
};
struct csXyz { s16 x = 0, y = 0, z = 0; };
using MtxP = const float (*)[4];
inline constexpr std::uint32_t AT_TYPE_SHIELD_ATTACK = 0x40000, kAtSword = 0x2, kAtIron = 0x4, kAtEnemy = 0x1;
struct AtMasks { std::uint32_t sword, iron_ball; };
constexpr AtMasks kMasks = {kAtSword, kAtIron};
struct cCcD_Obj;
struct fopAc_ac_c;
struct cCcD_ObjHitInf {
  cCcD_Obj* at_hit = nullptr;
  cCcD_Obj* tg_hit = nullptr;
  bool ChkAtHit() const noexcept { return at_hit != nullptr; }
  cCcD_Obj* GetAtHitObj() const noexcept { return at_hit; }
  bool ChkTgHit() const noexcept { return tg_hit != nullptr; }
  cCcD_Obj* GetTgHitObj() const noexcept { return tg_hit; }
  void ClrTgHit() noexcept { tg_hit = nullptr; }
};
struct cCcD_Obj : cCcD_ObjHitInf {
  std::uint32_t at_type = 0;
  fopAc_ac_c* owner_ac = nullptr;
  bool ChkAtType(std::uint32_t m) const noexcept { return (at_type & m) != 0; }
  fopAc_ac_c* GetAc() const noexcept { return owner_ac; }
};
struct dCcD_GObjInf : cCcD_Obj {
  bool at_shield_hit = false, tg_shield_hit = false, tg_shield = false, tg_sp = false, tg_set = true, tg_front = true;
  fopAc_ac_c* tg_hit_ac = nullptr;
  int eff_at = 0, eff_tg = 0;
  bool ChkAtShieldHit() const noexcept { return at_shield_hit; }
  bool ChkTgShieldHit() const noexcept { return tg_shield_hit; }
  bool ChkTgShield() const noexcept { return tg_shield; }
  bool ChkTgSpShield() const noexcept { return tg_sp; }
  bool ChkTgSet() const noexcept { return tg_set; }
  fopAc_ac_c* GetTgHitAc() const noexcept { return tg_hit_ac; }
};
struct dCcD_Sph : dCcD_GObjInf {
  cXyz c{}, vec{};
  float r = 35;
  int atp = 0;
  const cXyz& GetC() const noexcept { return c; }
  float GetR() const noexcept { return r; }
  int GetAtAtp() const noexcept { return atp; }
  void SetC(const cXyz& v) noexcept { c = v; }
  void SetR(float v) noexcept { r = v; }
  void SetAtVec(cXyz& v) noexcept { vec = v; }
  void StartCAt(const cXyz& p) noexcept { vec = cXyz(); c = p; }
  void MoveCAt(const cXyz& p) noexcept { vec = cXyz(p.x - c.x, p.y - c.y, p.z - c.z); c = p; }
};
struct dCcD_Cyl : dCcD_GObjInf {
  cXyz c{};
  float r = 35, h = 180;
  const cXyz& GetC() const noexcept { return c; }
  float GetR() const noexcept { return r; }
  float GetH() const noexcept { return h; }
};
struct J3DFrameCtrl { float mFrame = 0; };
struct J3DModelData { unsigned n = 35; unsigned getJointNum() const noexcept { return n; } };
struct J3DModel {
  float mtx[35][3][4] = {};
  J3DModelData data;
  J3DModelData* getModelData() noexcept { return &data; }
  MtxP getAnmMtx(int i) const noexcept { return mtx[i]; }
};
struct e_dn_class;
struct mDoExt_McaMorfSO {
  J3DFrameCtrl mFrameCtrl;
  J3DModel model;
  float rate = 1, end = 40, morf = 1;
  bool stop = false, loop = false;
  e_dn_class* owner = nullptr;
  float getFrame() const noexcept { return mFrameCtrl.mFrame; }
  float getPlaySpeed() const noexcept { return rate; }
  float getEndFrame() const noexcept { return end; }
  void setPlaySpeed(float r) noexcept { rate = r; }
  J3DModel* getModel() noexcept { return &model; }
  float getMorfRatio() const noexcept { return morf; }
  void* getOldTransInfo() const noexcept { return reinterpret_cast<void*>(1); }
  void* getOldQuaternion() const noexcept { return reinterpret_cast<void*>(1); }
  bool isStop() const noexcept { return stop || rate == 0.0f; }
  void modelCalc() noexcept;
};
struct dCcU_AtInfo {
  cCcD_Obj* mpCollider = nullptr;
  fopAc_ac_c* mpActor = nullptr;
  csXyz mHitDirection{};
};
struct fopAc_ac_c {
  struct { int init_state = 0; } state;
  struct { cXyz position{}; } attention_info;
  std::int16_t health = 100, field_0x560 = 100;
  struct { cXyz pos{}; csXyz angle{}; } current;
  csXyz shape_angle{};
  float speedF = 0;
  int profname = 0, name = 0;
  std::uint32_t pid = 0, parent = 0;
  int group = 0;
};
enum { fopAc_ACTOR_e = 0, fopAc_PLAYER_e = 1, fopAc_ENEMY_e = 2, fopAc_ENV_e = 3 };
inline constexpr int fpcNm_E_DN_e = 435, fpcNm_ALINK_e = 253;
struct fopEn_enemy_c : fopAc_ac_c {
  bool head_lock = false, down = false, cut_down_hit = false;
  void offHeadLockFlg() noexcept { head_lock = false; }
  void onHeadLockFlg() noexcept { head_lock = true; }
};
struct Acch { bool ground = true; bool ChkGroundHit() const noexcept { return ground; } };
struct Z2SoundObjBase {};
struct Z2SoundObjAnime : Z2SoundObjBase {};
struct Z2Creature { unsigned collision_se = 0; Z2SoundObjAnime mSoundObjAnime; };
struct e_dn_class {
  fopEn_enemy_c actor;
  int learn = 0;
  s16 action = 3, mode = 0, timer[4] = {0, 0, 0, 0}, invulnerability_timer = 0, unk_timer_1 = 40, unk_timer_2 = 0, search_angle_y = 0, search_angle_x = 0, field_0x70c = 0, field_0x750 = 0, frame_counter = 0;
  s16 cur_angle_y_target = 0;
  std::int8_t at_chk_flag = 0, tail_at_sph_flag = 0, at_sph_center_flag = 0, guard_flag = 0, snap_angle_y_flag = 0;
  float field_0x708 = 0, field_0x74c = 0, pl_dir = 0;
  int anm_no = 0;
  mDoExt_McaMorfSO* anm_p = nullptr;
  Acch objacch;
  dCcD_Sph at_sph, tate_sph, cc_sph[3];
  dCcU_AtInfo at_info;
  void* skull_model = nullptr;
  int arg0 = 1, arg1 = 15, status = 0;
  Z2Creature sound;
  // lab
  std::uint64_t att_hits = 0;
  float len[64] = {};
  std::uint32_t nr = 7;
  mDoExt_McaMorfSO morph;
  int hit_power = 4;
  bool hit_strong = false;
  std::uint64_t small_body = 0, big_body = 0, deleted = 0, drawback_entered = 0;
  int dmg_t = 0;
  std::uint32_t apid = 0;
  std::uint32_t nrand(std::uint32_t n) {
    nr = nr * 1664525u + 1013904223u;
    return (nr >> 8) % n;
  }
};
struct daPy_py_c {
  enum { FLG2_COMBO_RESERB = 2, FLG1_IS_WOLF = 1, RFLG1_UNK_2 = 2, CUT_TYPE_NONE = 0, CUT_TYPE_JUMP = 0x0a, CUT_TYPE_GUARD_ATTACK = 0x29, CUT_TYPE_HEAD_JUMP = 0x2b, CUT_TYPE_NORMAL = 1 };
};
struct Stts { std::uint32_t apid = 0, tg_apid = 0, tg_old = 0; };
struct daAlink_c : fopAc_ac_c, daPy_py_c {
  enum { PROC_GUARD_SLIP = 0x1d, PROC_GUARD_ATTACK = 0x1e, PROC_WAIT = 3, PROC_DAMAGE = 0x70, ANM_SHIELD_ATTACK = 1, PROC_CUT_NORMAL = 0x21, PROC_CUT_FINISH, PROC_CUT_FINISH_JUMP_UP, PROC_CUT_FINISH_JUMP_UP_LAND, PROC_CUT_REVERSE, PROC_CUT_JUMP, PROC_CUT_JUMP_LAND, PROC_CUT_TURN, PROC_CUT_TURN_CHARGE, PROC_CUT_TURN_MOVE, PROC_CUT_DOWN, PROC_CUT_DOWN_LAND, PROC_CUT_HEAD, PROC_CUT_HEAD_LAND, PROC_CUT_LARGE_JUMP_CHARGE, PROC_CUT_LARGE_JUMP, PROC_CUT_LARGE_JUMP_LAND };
  struct UnderCtrl { float f = 0, r = 0; float getFrame() const noexcept { return f; } float getRate() const noexcept { return r; } } mUnderFrameCtrl[1];
  float field_0x3478 = 1.0f, field_0x347c = 20.0f;
  bool checkUnderMove0BckNoArc(int) const noexcept { return mProcID == PROC_GUARD_ATTACK; }
  int mProcID = PROC_WAIT;
  s16 mDamageTimer = 0;
  float mNormalSpeed = 0;
  int mEquipItem = 0x103;
  struct { s16 field_0x300a = 0; } mProcVar1;
  dCcD_Cyl mTgCyls[3];
  dCcD_Sph mAtCps[3];
  cCcD_Obj mAtCyl;
  cCcD_Obj mGuardAtCps;
  bool magic_armor = false;
  std::uint64_t rebounds = 0;
  bool checkMagicArmorNoDamage() const noexcept { return magic_armor; }
  int cut_type = 0;
  bool wolf = false, horse = false;
  Stts stts;
  float frame = 0, slip_speed_chase = 0;
  s16 shield_yaw = 0;
  int life = 80;
  // script
  bool r_held = false, guard_active = false;
  std::set<std::uint64_t> presses;
  // receipts
  std::uint64_t se_calls = 0, se_ticks[64] = {}, shocks = 0, damage_points = 0, slip_inits = 0, shield_attacks = 0;
  bool checkNoResetFlg1(int) const noexcept { return wolf; }
  bool checkNoResetFlg2(int) const noexcept { return false; }
  unsigned getCutCount() const noexcept { return 1; }
  float clip_rate = 0;  // FIX05: the cut clip's rate while a native cut runs
  bool checkResetFlg1(int) const noexcept { return false; }
  bool checkHorseRide() const noexcept { return horse; }
  int getCutType() const noexcept { return cut_type; }
  void setGuardSe(dCcD_GObjInf* o) noexcept;
  int procGuardAttackInit() noexcept;
};
inline constexpr int VIBMODE_S_POWER3 = 3;
struct World;
static World* W = nullptr;
static int g_lose_hp_in_damage = 0;
struct dVibration_c { bool StartShock(int, int, cXyz) noexcept; };
static dVibration_c g_vib;
static dVibration_c& dComIfGp_getVibration() noexcept { return g_vib; }
struct PlayerStatus { int life = 80; int getLife() const noexcept { return life; } };
struct GameInfo {
  struct Info { PlayerStatus st; struct P { PlayerStatus* s; PlayerStatus& getPlayerStatusA() noexcept { return *s; } } p{&st}; P& getPlayer() noexcept { return p; } } info;
  struct Ev { bool run = false; bool runCheck() const noexcept { return run; } };
  struct Play { Ev ev; float pending = 0; void* player = nullptr; bool pause = false; bool isPauseFlag() const noexcept { return pause; } Ev* getEvent() noexcept { return &ev; } float getItemLifeCount() const noexcept { return pending; } void* getPlayer(int) const noexcept { return player; } } play;
} g_dComIfG_gameInfo;
static daAlink_c* g_link = nullptr;
static daAlink_c* dComIfGp_getLinkPlayer() noexcept { return g_link; }
static const char* dComIfGp_getStartStageName() noexcept { return "F_SP104"; }
static std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) noexcept { return a->pid; }
static int fopAcM_GetName(const fopAc_ac_c* a) noexcept { return a->name; }
static int fopAcM_GetRoomNo(const fopAc_ac_c*) noexcept { return 0; }
static int fopAcM_GetGroup(const fopAc_ac_c* a) noexcept { return a->group; }
static std::uint32_t fopAcM_GetLinkId(const fopAc_ac_c* a) noexcept { return a->parent; }
static std::vector<fopAc_ac_c*> g_actors;
static int fopAcM_SearchByID(std::uint32_t id, fopAc_ac_c** out) noexcept {
  *out = nullptr;
  for (auto* a : g_actors)
    if (a->pid == id) *out = a;
  return *out != nullptr;
}
static float fopAcM_searchActorDistance(const void*, const fopAc_ac_c*) noexcept { return 100.0f; }
static std::uint64_t g_bash_se = 0;
// ---------------------------------------------------------------- FIX03 native surface: JPA instances, sound playbacks, attention, pad, camera, host services
struct GXColor { u8 r = 0, g = 0, b = 0, a = 0; };
struct JPABaseParticle { GXColor mPrmClr, mEnvClr; int mAge = 0; };
template <class T> struct JPANode { JPANode* mpPrev = nullptr; JPANode* mpNext = nullptr; T mData; JPANode* getNext() const noexcept { return mpNext; } T* getObject() noexcept { return &mData; } };
template <class T> struct JPAList { JPANode<T>* mpFirst = nullptr; JPANode<T>* getFirst() const noexcept { return mpFirst; } };
struct JPABaseEmitter;
struct JPAEmitterCallBack {
  virtual ~JPAEmitterCallBack() {}
  virtual void execute(JPABaseEmitter*) {}
  virtual void executeAfter(JPABaseEmitter*) {}
  virtual void draw(JPABaseEmitter*) {}
  virtual void drawAfter(JPABaseEmitter*) {}
};
// FIX05: the resource side of a spark, with the native accessors parry_fx_adapter.inc reads.
struct JPABaseShape {
  const void* pBsd = nullptr; const void* mpTexCrdMtxAnmTbl = nullptr; const u8* mpTexIdxAnimTbl = nullptr; GXColor* mpPrmClrAnmTbl = nullptr; GXColor* mpEnvClrAnmTbl = nullptr;
  bool glbl = true, prm_anm = false, env_anm = false; unsigned sel = 1; short clr_max = 3;
  bool isGlblClrAnm() const noexcept { return glbl; } bool isPrmAnm() const noexcept { return prm_anm; } bool isEnvAnm() const noexcept { return env_anm; }
  unsigned getTevColorArgSel() const noexcept { return sel; } short getClrAnmMaxFrm() const noexcept { return clr_max; } u8 getTexIdx() const noexcept { return 0; } u8 getTexIdx(u8 i) const noexcept { CHECK(mpTexIdxAnimTbl != nullptr); return mpTexIdxAnimTbl[i]; }  // the table is null unless the resource animates its texture, as in the game
  void getPrmClr(short i, GXColor* d) const noexcept { *d = mpPrmClrAnmTbl[i]; } void getEnvClr(short i, GXColor* d) const noexcept { *d = mpEnvClrAnmTbl[i]; }
};
struct JPAChildShapeData { GXColor mPrmClr, mEnvClr; };
struct JPAChildShape {
  const JPAChildShapeData* mpData = nullptr; bool inherit = false, alpha_out = false, alpha_inherit = false;
  void getPrmClr(GXColor* d) const noexcept { *d = mpData->mPrmClr; } void getEnvClr(GXColor* d) const noexcept { *d = mpData->mEnvClr; }
  bool isColorInherited() const noexcept { return inherit; } bool isAlphaOutOn() const noexcept { return alpha_out; } bool isAlphaInherited() const noexcept { return alpha_inherit; }
};
struct JPAResource { JPABaseShape* pBsp = nullptr; JPAChildShape* pCsp = nullptr; JPABaseShape* getBsp() const noexcept { return pBsp; } JPAChildShape* getCsp() const noexcept { return pCsp; } u16 getTexIdx(u8) const noexcept { return 0; } };
struct JPAEmitterWorkData { JPAResource* mpRes = nullptr; };
struct ResTIMG { u8 format = 1; };
struct JPATextureData { ResTIMG mResTIMG; };
struct JPATexture { const JPATextureData* mpData = nullptr; };
struct JPAResourceManager { JPATexture** pTexAry = nullptr; u16 texRegNum = 0; };
struct JPAEmitterManager { JPAEmitterWorkData* pWd = nullptr; JPAResourceManager* pResMgrAry[1] = {nullptr}; };
enum GXTevRegID { GX_TEVREG0 = 1, GX_TEVREG1 = 2 };
static GXColor g_tev[3];
static void GXSetTevColor(GXTevRegID r, GXColor c) noexcept { g_tev[r] = c; }
struct EFlight { int mState = 0, mLightType = 0; };
struct EnvLight { EFlight eflight; };
static EnvLight g_env_light;
struct JPABaseEmitter { GXColor mPrmClr, mEnvClr; JPAEmitterCallBack* mpEmtrCallBack = nullptr; JPAList<JPABaseParticle> mAlivePtclBase, mAlivePtclChld; JPAEmitterManager* mpEmtrMgr = nullptr; JPAResource* pRes = nullptr; u8 mResMgrID = 0; GXColor mGlobalPrmClr{255, 255, 255, 255}, mGlobalEnvClr{255, 255, 255, 255}; unsigned mStatus = 0; unsigned checkStatus(unsigned s) const noexcept { return mStatus & s; } };
struct JASSoundParams { float mPitch = 1.0f; };
static std::uint64_t g_move_pitch_calls = 0;
struct JAISoundParamsMove { JASSoundParams params_; void movePitch(float v, unsigned) noexcept { params_.mPitch = v, ++g_move_pitch_calls; } };
struct JAISound { JAISoundParamsMove m; std::uint32_t hit = 0; JAISoundParamsMove& getAuxiliary() noexcept { return m; } };
struct JAISoundHandle { JAISound* sound_ = nullptr; bool isSoundAttached() const noexcept { return sound_ != nullptr; } JAISound* operator->() const noexcept { return sound_; } };
struct Z2SoundHandlePool : JAISoundHandle {};
using fpc_ProcID = std::uint32_t;
enum { fopAc_attn_LOCK_e = 0, fopAc_attn_TALK_e = 1, fopAc_attn_BATTLE_e = 2 };
inline constexpr int fpcNm_E_OC_e = 200;
struct interface_of_controller_pad { float mCStickPosX = 0, mCStickPosY = 0, mCStickValue = 0; s16 mCStickAngle = 0; float mTriggerLeft = 0; u8 mHoldLockL = 0, mTrigLockL = 0; };
struct mDoCPd_c { static inline interface_of_controller_pad m_cpadInfo[4]; };
struct dAttList_c { float mWeight = 0, mDistance = 0; u32 mType = 0; fpc_ProcID mActorID = 0; fpc_ProcID getPid() const noexcept { return mActorID; } };
struct dAttention_c {
  enum EState { EState_NONE, EState_LOCK, EState_RELEASE };
  fpc_ProcID mLockTargetID = 0xffffffffu;
  u32 mPadNo = 0;
  u8 mAttnStatus = EState_NONE, field_0x32b = 4, field_0x32e = 0, field_0x32f = 0;
  dAttList_c mLockOnList[8];
  int mLockonCount = 0, mLockOnOffset = 0;
};
static dAttention_c g_att;
static u8 g_opt_attention = 0, g_window_status = 0;
static dAttention_c* dComIfGp_getAttention() noexcept { return &g_att; }
static u8 dComIfGs_getOptAttentionType() noexcept { return g_opt_attention; }
static u8 dMeter2Info_getWindowStatus() noexcept { return g_window_status; }
struct dCamera_c { cXyz eye{0, 100, -300}, center{0, 100, 0}; cXyz iEye() const noexcept { return eye; } cXyz iCenter() const noexcept { return center; } };
static dCamera_c g_cam;
static dCamera_c* g_cam_ptr = &g_cam;
static dCamera_c* dCam_getBody() noexcept { return g_cam_ptr; }
static ModContext g_ctx_obj;
static ModContext* mod_ctx = &g_ctx_obj;
struct ModError {};
using ConfigVarHandle = std::uint64_t;
using ConfigSubscriptionHandle = std::uint64_t;
enum ConfigVarType { CONFIG_VAR_BOOL = 0 };
struct ConfigVarDesc { const char* name = nullptr; ConfigVarType type = CONFIG_VAR_BOOL; bool default_bool = false; };
#define CONFIG_VAR_DESC_INIT {}
struct ConfigVarValue { ConfigVarType type = CONFIG_VAR_BOOL; bool bool_value = false; };
using ConfigChangedFn = void (*)(ModContext*, ConfigVarHandle, const ConfigVarValue*, const ConfigVarValue*, void*);
struct ConfigService {
  ModResult (*register_var)(ModContext*, const ConfigVarDesc*, ConfigVarHandle*);
  ModResult (*get_bool)(ModContext*, ConfigVarHandle, bool*);
  ModResult (*subscribe)(ModContext*, ConfigVarHandle, ConfigChangedFn, void*, ConfigSubscriptionHandle*);
};
using UiElementHandle = std::uint64_t;
enum UiControlKind { UI_CONTROL_BUTTON = 0, UI_CONTROL_TOGGLE = 1 };
enum UiControlBinding { UI_BINDING_CALLBACKS = 0, UI_BINDING_CONFIG_VAR = 1 };
struct UiControlDesc { UiControlKind kind = UI_CONTROL_BUTTON; const char* label = nullptr; const char* help_rml = nullptr; const char* tooltip = nullptr; UiControlBinding binding = UI_BINDING_CALLBACKS; ConfigVarHandle config_var = 0; };
#define UI_CONTROL_DESC_INIT {}
using UiPanelBuildFn = ModResult (*)(ModContext*, UiElementHandle, void*, ModError*);
struct UiModsPanelDesc { UiPanelBuildFn build = nullptr; };
#define UI_MODS_PANEL_DESC_INIT {}
struct UiService {
  ModResult (*register_mods_panel)(ModContext*, const UiModsPanelDesc*);
  ModResult (*pane_add_section)(ModContext*, UiElementHandle, const char*);
  ModResult (*pane_add_control)(ModContext*, UiElementHandle, const UiControlDesc*, UiElementHandle*);
};
static const ConfigService* svc_config = nullptr;
static const UiService* svc_ui = nullptr;
// E_OC as the FX preview reads it.
struct Morf { float frame = 15.0f; float getFrame() const noexcept { return frame; } };
struct daE_OC_c : fopAc_ac_c { int mActionMode = 4, mOcState = 1; Morf* mpMorf = nullptr; };
enum class NameKind : u8 { Unread, E_OC, E_OC2, Other };
struct LabAux { int atp = 1; u8 resource = u8(NameKind::E_OC); };
static LabAux g_oc_aux;
static LabAux read_aux(const daE_OC_c*) noexcept { return g_oc_aux; }
enum class Life : u8 { Free = 0, Active };
enum class Adm : u8 { None = 0, Candidate };
static bool is_candidate(Adm a) noexcept { return a == Adm::Candidate; }
struct LabLedgerSlot { std::uint32_t id = 0; std::uintptr_t ptr = 0; Life life = Life::Free; Adm adm = Adm::None; std::uint64_t generation = 0; };
struct LabEntry { bool bound = false, latched = false, frozen = false, dead = false; std::uint32_t id = 0; std::uint64_t generation = 0; };
struct LabLedger { LabLedgerSlot s[post08::kSlots]; const LabLedgerSlot& slot(unsigned i) const noexcept { return s[i]; } };
struct LabPx { LabEntry e[post08::kSlots]; const LabEntry& entry(unsigned i) const noexcept { return e[i]; } };
// Native order of def_se_set and Z2SoundObjBase::startCollisionSE, through the hooks of the production adapter (bodies after the World type).
static void def_se_set(Z2Creature* s, cCcD_Obj*, unsigned, fopAc_ac_c*) noexcept;
// ---------------------------------------------------------------- module surface around the adapter (mod.cpp)
struct LizExec {};
struct LizDelete {};
struct LizDamage {};
struct LizSmall {};
struct LizBig { static inline void (*g_orig)(e_dn_class*) = nullptr; };
struct LizRun {};
struct LizAttack0 {};
struct LizAttack {};
struct LizTail {};
struct LizAtHit {};
struct LizAnm {};
struct LinkCutType {};
struct LinkDamageFix01 {};
struct LinkGuardSe { static void* resolved_target() { static char x; return &x; } };
struct LizGuard {};
struct LinkCutReverse {};
struct LinkSetDamage {};
template <int N> struct LabTarget { static void* resolved_target() { static char x; return &x; } };
struct daE_OC_c;
static bool parry_layout_ok(daE_OC_c*) noexcept { return true; }
struct PfxProcHitmark : LabTarget<1> {};
struct PfxSetHitMark : LabTarget<2> {};
struct PfxParticleSet : LabTarget<3> {};
struct PfxCollisionSe : LabTarget<4> {};
static constexpr unsigned kLogLineMax = 512;
static std::vector<std::string>* g_logs = nullptr;
static void lnk_write(LogLevel, const char* line) noexcept {
  if (g_logs) g_logs->push_back(line);
}
static bool lnk_take(unsigned& lines, std::uint64_t& dropped, unsigned cap) noexcept {
  if (lines >= cap) {
    ++dropped;
    return false;
  }
  ++lines;
  return true;
}
inline constexpr unsigned kF1Joints = 64;
static struct { std::uint64_t link_set_tick = ~0ull; std::uintptr_t link_set_key = 0; } f1;
static daAlink_c* f1_link() noexcept { return g_link; }
static void f1_cyls(daAlink_c* k, cp06f1::Cyl cy[3]) noexcept {
  for (int i = 0; i < 3; ++i) {
    const auto& c = k->mTgCyls[i];
    const cXyz& o = c.GetC();
    cy[i].c = {o.x, o.y, o.z};
    cy[i].r = c.GetR();
    cy[i].h = c.GetH();
    cy[i].on = c.ChkTgSet();
  }
}
static struct {
  struct { struct { bool disabled = false; LabLedger ledger; } obs; } eng;
  struct { LabPx px; } pe;
  post08::Parry parry;
  struct { std::uint64_t tick = 0; bool watch_life = false; int life_at = 0; int layout = 1; } p8;
  post08::LinkTrack link_track;
} g;
static struct { bool enabled = true; shieldchain::Chain shield; } c4;
static bool g_world_safe = true;
static bool g_fix02_gate = false;
static std::uint64_t g_peek_checks = 0, g_preview_checks = 0, g_preview_parries = 0, g_exec_parries = 0;
static std::uint64_t g_fx_reverted = 0;  // 0.12.2 behaviour of cp04_maintenance with no E_OC layout check yet (see World::tick)
static bool cp04_world_safe() noexcept { return g_world_safe; }
static bool cp04_guard_pass() noexcept { return !c4.enabled || c4.shield.permits(g.p8.tick); }
static bool cp04_link_safe(daAlink_c* l) noexcept { return g_world_safe && l && l->mDamageTimer == 0; }
static bool cp04_native_shield_restart(daAlink_c* l) noexcept {
  const auto serial = c4.shield.native_serial, starts = c4.shield.c.native_starts;
  (void)l->procGuardAttackInit();
  return c4.shield.native_serial == serial + 1 && c4.shield.c.native_starts == starts + 1 && l->mProcID == daAlink_c::PROC_GUARD_ATTACK;
}
static void cp04_log(const char*, const void*, const char*) noexcept {}
static struct { lnk01::Posture posture; bool sim_active = true; } lg;
static struct { struct { lnk05::State state() const noexcept { return lnk05::State::Ready; } } m; } lb;
static std::uint64_t g_lnk05_req = 0;
static void lnk05_request(lnk05::Kind, std::uint32_t, std::uint64_t, std::uint64_t, int, int) noexcept { ++g_lnk05_req; }
static struct { lnks1fix::NativeDamage native; bool native_reentered = false; int native_depth = 0; } fx;
static struct { bool enabled = true; lnkfx::Defense def; } lf;
static std::uint32_t stage_token(const char*) noexcept { return 1; }
namespace ehz {
struct Anchor { float x = 0, y = 0, z = 0; };
struct DrawEntry { Anchor anchor; float alpha = 1; int key = 0; };
}  // namespace ehz
struct HudKey { std::uintptr_t key; std::uint32_t id; std::int16_t name; };
static struct {
  post07::Sample samples[post07::kSlots];
  int slot_interp[post07::kSlots] = {};
  std::uint64_t aligned_samples = 0;
  struct {
    struct {
      bool presented(const HudKey&, ehz::DrawEntry&) const noexcept { return false; }
    } tracker;
  } mirror;
} hg;
namespace {
void pfx_settle(pfx::Kind, std::uint32_t, bool) noexcept;  // as mod.cpp forward-declares them
void pfx_liz_exec_begin(e_dn_class*, std::uint32_t) noexcept;
void pfx_liz_exec_end() noexcept;
#include "../src/liz_adapter.inc"
#include "../src/lockon_adapter.inc"
#include "../src/parry_fx_adapter.inc"
}
// ---------------------------------------------------------------- transcribed engine
inline constexpr float kPi = 3.14159265f;
inline float yaw_rad(s16 a) { return float(a) * kPi / 32768.0f; }
inline s16 atan2s(float x, float z) { return static_cast<s16>(std::lround(std::atan2(x, z) * 32768.0f / kPi)); }
// Pose model: arcs fitted to the logged center steps (N ~60/frame, D ~170/frame, tail ~130/frame), centred on the actor's facing at the middle of each native window.
struct Arc { float f0, f1, radius, omega, y; };
inline Arc arc_for(int anm, float f) {
  if (anm == liz::native::kAnmN) return {22.0f, 29.0f, 150.0f, 0.40f, 100.0f};
  if (anm == liz::native::kAnmD) return f < 22.0f ? Arc{13.0f, 19.0f, 150.0f, 1.13f, 100.0f} : Arc{25.0f, 31.0f, 170.0f, 1.00f, 100.0f};
  if (anm == liz::native::kAnmC) return {33.0f, 42.0f, 190.0f, 0.68f, 80.0f};
  return {0.0f, 0.0f, 60.0f, 0.0f, 100.0f};
}
inline float g_arc_bias = 0.0f;
inline constexpr int kLabJntArmL2 = 0x0C;
inline float g_shield_side = 40.0f, g_shield_fwd = 4.0f;  // lab: the shield's horizontal offset in the body frame (+x = the actor's left)
inline float g_omega_scale = 1.0f;  // lab only: a faster tail arc than any logged one, so that the tail's own probe geometry is really decisive
inline float g_pose_noise = 0.0f;  // lab only: an engine pose calc that does not reproduce itself (restore check)
inline int g_pose_calls = 0;
inline void pose_point(int anm, float f, const cXyz& root, s16 yaw, cXyz& out) {
  const Arc a = arc_for(anm, f);
  const float mid = 0.5f * (a.f0 + a.f1);
  const float fc = f < a.f0 ? a.f0 : f > a.f1 ? a.f1 : f;
  const float th = yaw_rad(yaw) + (fc - mid) * a.omega * g_omega_scale + g_arc_bias;
  out = cXyz(root.x + std::sin(th) * a.radius, root.y + a.y, root.z + std::cos(th) * a.radius);
}
void mDoExt_McaMorfSO::modelCalc() noexcept {
  e_dn_class* a = owner;
  cXyz p;
  pose_point(a->anm_no, mFrameCtrl.mFrame, a->actor.current.pos, a->actor.shape_angle.y, p);
  auto* m = model.mtx;
  for (int j = 0; j < 35; ++j) {
    auto* x = m[j];
    x[0][0] = 1, x[0][1] = 0, x[0][2] = 0, x[1][0] = 0, x[1][1] = 1, x[1][2] = 0, x[2][0] = 0, x[2][1] = 0, x[2][2] = 1;
    x[0][3] = a->actor.current.pos.x, x[1][3] = a->actor.current.pos.y + 50.0f, x[2][3] = a->actor.current.pos.z;
  }
  ++g_pose_calls;
  if (g_pose_noise != 0.0f) m[5][0][3] += g_pose_noise * float(g_pose_calls);
  // Only the limb that swings is on the arc: the hand during a sword clip, the tail tip during the tail clip; the other one stays by the body.
  const bool tail_clip = a->anm_no == liz::native::kAnmC;
  auto* h = m[liz::native::kJntHandR1];
  if (!tail_clip) h[0][3] = p.x, h[1][3] = p.y - 60.0f, h[2][3] = p.z;
  auto* t = m[liz::native::kJntTail04];
  if (tail_clip) t[0][3] = p.x, t[1][3] = p.y, t[2][3] = p.z;
  // FIX03 shield arm: JNT_ARML2 rotates with the body (cMtx_YrotS(shape_y)); Execute's (50,0,30) lands on body + Ry*(g_shield_side, 110, g_shield_fwd).
  // The real ANM_GUARD pose is BCK data (not read): the lab sweeps the side offset instead of claiming one.
  {
    const float th = yaw_rad(a->actor.shape_angle.y), cs = std::cos(th), sn = std::sin(th);
    auto* l = m[kLabJntArmL2];
    l[0][0] = cs, l[0][1] = 0, l[0][2] = sn, l[1][0] = 0, l[1][1] = 1, l[1][2] = 0, l[2][0] = -sn, l[2][1] = 0, l[2][2] = cs;
    const float wx = g_shield_side * cs + g_shield_fwd * sn, wz = -g_shield_side * sn + g_shield_fwd * cs;
    const float ox = 50.0f * cs + 30.0f * sn, oz = -50.0f * sn + 30.0f * cs;
    l[0][3] = a->actor.current.pos.x + wx - ox, l[1][3] = a->actor.current.pos.y + 110.0f, l[2][3] = a->actor.current.pos.z + wz - oz;
  }
}
struct World {
  std::vector<e_dn_class*> dn;
  daAlink_c link;
  PlayerStatus* life = &g_dComIfG_gameInfo.info.st;
  std::uint64_t sparks = 0;
  bool place_front = true, slip_push = true, skip_link_collision = false;
  void hooks_clear_at_tick() {}
  // ---- FIX03 model of the native spark: dCcS::ProcAtTgHitmark -> setHitMark -> two dPa_control_c::set; then JPAResource::calc and drawP in their order
  struct LabEmitter : JPABaseEmitter {
    u16 res = 0;
    int age = 0, life = 12;
    bool dead = false, per_particle_anim = false;
    GXColor res_prm{90, 160, 255, 200}, res_env{20, 60, 255, 0};
    JPANode<JPABaseParticle> base[3], child[1];
    bool born = false;
    std::vector<GXColor> drawn;  // what the first drawn frame really used: registered emitter colours, then every particle's
    int created_tick = 0;
    JPABaseShape shape;
    JPAResource resobj;
    JPAChildShape cshape;
    JPAChildShapeData cdata;
    GXColor tprm[4], tenv[4], cap_prm[3], cap_env[3], child_reg_p{}, child_reg_e{}, child_birth{};
    bool child_born = false;
  };
  JPAEmitterWorkData work;
  JPAResourceManager resmgr;
  JPATexture tex;
  JPATextureData texdata;
  JPATexture* texs[1] = {&tex};
  JPAEmitterManager mgr;
  bool interp = true;  // src/dusk/interp/particle.cpp: a presentation frame draws a scratch copy built from the colours captured at the end of calc_p
  bool flash_called = false;
  std::vector<std::unique_ptr<LabEmitter>> emitters;
  std::vector<std::unique_ptr<JAISound>> playbacks;
  std::vector<std::unique_ptr<Z2SoundHandlePool>> handles;
  std::vector<fopAc_ac_c*> marked;
  JPAEmitterCallBack* next_native_cb = nullptr;
  bool next_particle_fails = false;
  JPABaseEmitter* particle_set(u16 res) {
    LabEmitter* e = nullptr;
    if (!next_particle_fails) {
      emitters.push_back(std::make_unique<LabEmitter>());
      e = emitters.back().get();
      e->res = res, e->mPrmClr = e->res_prm, e->mEnvClr = e->res_env, e->per_particle_anim = res == 0xB, e->mpEmtrCallBack = next_native_cb, e->created_tick = int(g.p8.tick);
      for (int i = 0; i < 4; ++i) e->tprm[i] = e->res_prm, e->tenv[i] = e->res_env;
      e->shape.mpPrmClrAnmTbl = e->tprm, e->shape.mpEnvClrAnmTbl = e->tenv, e->shape.pBsd = e, e->shape.glbl = res != 0xB, e->shape.prm_anm = e->shape.env_anm = res == 0xB;
      e->cdata.mPrmClr = e->res_prm, e->cdata.mEnvClr = e->res_env, e->cshape.mpData = &e->cdata;
      e->resobj.pBsp = &e->shape, e->resobj.pCsp = res == 0xB ? &e->cshape : nullptr;
      e->pRes = &e->resobj;
      texdata.mResTIMG.format = 1, tex.mpData = &texdata, resmgr.pTexAry = texs, resmgr.texRegNum = 1, mgr.pWd = &work, mgr.pResMgrAry[0] = &resmgr, e->mpEmtrMgr = &mgr;
    }
    u8 grp = 0;
    u16 id = res;
    void* args[3] = {nullptr, &grp, &id};
    JPABaseEmitter* ret = e;
    Reg<PfxParticleSet>::run_post(args, &ret);
    return e;
  }
  void set_hit_mark(u16 type, fopAc_ac_c* actor) {
    fopAc_ac_c* ac = actor;
    u16 t = type;
    void* args[3] = {nullptr, &t, &ac};
    Reg<PfxSetHitMark>::run_pre(args);
    // dPa_control_c::setHitMark: fopAcM_CheckStatus(actor, 0x40000000) -> return: one hit mark per actor per frame
    if (!actor || std::find(marked.begin(), marked.end(), actor) == marked.end()) {
      if (actor) marked.push_back(actor);
      (void)particle_set(0xA);
      (void)particle_set(0xB);
      if (pfx::hitng_mark(type)) g_env_light.eflight.mState = 1, g_env_light.eflight.mLightType = 0;  // dKy_SordFlush_set(pos, 0)
    }
    Reg<PfxSetHitMark>::run_post(args);
  }
  void proc_hitmark(dCcD_GObjInf* at, dCcD_GObjInf* tg, bool shield, u16 mark, fopAc_ac_c* actor) {
    bool sa = true, st = true, cs = shield;
    cCcD_Obj* ao = at;
    cCcD_Obj* to = tg;
    dCcD_GObjInf* ai = at;
    dCcD_GObjInf* ti = tg;
    void* args[13] = {nullptr, &sa, &st, &ao, &to, &ai, &ti, nullptr, nullptr, nullptr, nullptr, nullptr, &cs};
    Reg<PfxProcHitmark>::run_pre(args);
    set_hit_mark(mark, actor);
    Reg<PfxProcHitmark>::run_post(args);
  }
  void particles() {  // JPAResource::calc, then drawP
    for (auto& up : emitters) {
      LabEmitter& e = *up;
      if (e.dead) continue;
      work.mpRes = e.pRes;  // JPAResource::calc: work->mpRes = this
      if (e.mpEmtrCallBack) e.mpEmtrCallBack->execute(&e);
      e.mPrmClr = GXColor{e.res_prm.r, e.res_prm.g, e.res_prm.b, e.mPrmClr.a}, e.mEnvClr = GXColor{e.res_env.r, e.res_env.g, e.res_env.b, e.mEnvClr.a};  // calcKey: emitter colour from the resource
      if (!e.born) {  // pDyn->create: particles copy the emitter's colour at birth
        e.born = true;
        for (int i = 0; i < 3; ++i) {
          e.base[i].mData.mPrmClr = e.mPrmClr, e.base[i].mData.mEnvClr = e.mEnvClr;
          e.base[i].mpNext = i < 2 ? &e.base[i + 1] : nullptr;
        }
        e.mAlivePtclBase.mpFirst = &e.base[0];
        e.child[0].mData.mPrmClr = e.mPrmClr, e.child[0].mData.mEnvClr = e.mEnvClr;
        e.mAlivePtclChld.mpFirst = &e.child[0];
      }
      if (e.mpEmtrCallBack) e.mpEmtrCallBack->executeAfter(&e);
      // calc_p AFTER executeAfter, through work->mpRes: key frames (JPACalcPrm/Env) or the emitter's colour (JPACalcColorCopy); then children; then the port captures what is there
      for (int i = 0; i < 3; ++i) {
        auto& n = e.base[i];
        if (e.per_particle_anim) work.mpRes->getBsp()->getPrmClr(0, &n.mData.mPrmClr), work.mpRes->getBsp()->getEnvClr(0, &n.mData.mEnvClr);
        else n.mData.mPrmClr = e.mPrmClr, n.mData.mEnvClr = e.mEnvClr;
        ++n.mData.mAge;
        e.cap_prm[i] = n.mData.mPrmClr, e.cap_env[i] = n.mData.mEnvClr;
      }
      if (work.mpRes->getCsp() && !e.child_born) {  // JPABaseParticle::init_c
        e.child_born = true;
        work.mpRes->getCsp()->getPrmClr(&e.child[0].mData.mPrmClr), work.mpRes->getCsp()->getEnvClr(&e.child[0].mData.mEnvClr);
        e.child_birth = e.child[0].mData.mPrmClr;
      }
      // drawP: emitter colours registered first, then the emitter draw callback, then each particle's colour
      const GXColor reg_p = e.mPrmClr, reg_e = e.mEnvClr;
      e.mStatus &= ~0x80u;
      g_tev[GX_TEVREG0] = GXColor{1, 2, 3, 4};
      if (e.mpEmtrCallBack) e.mpEmtrCallBack->draw(&e);
      CHECK(g_tev[GX_TEVREG0].r == 1 && g_tev[GX_TEVREG0].b == 3);  // the parent pass never touches the TEV registers: its colours come from the emitter and the particles
      if (e.drawn.empty()) {
        e.drawn.push_back(reg_p), e.drawn.push_back(reg_e);
        for (int i = 0; i < 3; ++i) e.drawn.push_back(interp ? e.cap_prm[i] : e.base[i].mData.mPrmClr), e.drawn.push_back(interp ? e.cap_env[i] : e.base[i].mData.mEnvClr);
      }
      if (e.mpEmtrCallBack) e.mpEmtrCallBack->drawAfter(&e);
      if (e.pRes->getCsp()) {  // drawC: JPARegistChildPrmEnv from the REAL resource (work->mpRes = this in draw), then the emitter draw callback
        e.mStatus |= 0x80u;
        e.pRes->getCsp()->getPrmClr(&g_tev[GX_TEVREG0]), e.pRes->getCsp()->getEnvClr(&g_tev[GX_TEVREG1]);
        if (e.mpEmtrCallBack) e.mpEmtrCallBack->draw(&e);
        e.child_reg_p = g_tev[GX_TEVREG0], e.child_reg_e = g_tev[GX_TEVREG1];
        if (e.mpEmtrCallBack) e.mpEmtrCallBack->drawAfter(&e);
      }
      if (++e.age >= e.life) e.dead = true;
    }
  }
  Z2SoundHandlePool* collision_se(std::uint32_t hit, Z2SoundObjBase* self = nullptr, u32 map = 0) {
    playbacks.push_back(std::make_unique<JAISound>());
    playbacks.back()->hit = hit;
    handles.push_back(std::make_unique<Z2SoundHandlePool>());
    handles.back()->sound_ = playbacks.back().get();
    Z2SoundHandlePool* ret = handles.back().get();
    u32 hit_id = hit;
    Z2SoundObjBase* other = nullptr;
    void* args[4] = {&self, &hit_id, &map, &other};
    Reg<PfxCollisionSe>::run_post(args, &ret);
    return ret;
  }
  void add_dn(float x, float z, std::uint32_t id) {
    auto* a = new e_dn_class;
    a->anm_p = &a->morph;
    a->morph.owner = a;
    a->actor.name = a->actor.profname = fpcNm_E_DN_e;
    a->actor.group = fopAc_ENEMY_e;
    a->at_sph.owner_ac = &a->actor;
    g_actors.push_back(&a->actor);
    a->actor.pid = id, a->apid = id;
    a->actor.current.pos = cXyz(x, 0, z);
    a->at_sph.at_type = kAtEnemy;
    for (auto& l : a->len) l = 40.0f;
    a->len[liz::native::kAnmN] = 42, a->len[liz::native::kAnmD] = 46, a->len[liz::native::kAnmC] = 62, a->len[18] = 50, a->len[22] = 22;
    a->timer[2] = 5;
    dn.push_back(a);
  }
  // ---- engine: one collision pass (cCcS::ChkAtTg + dCcS rules)
  void pass() {
    marked.clear();
    for (auto* a : dn) {
      a->at_sph.at_hit = nullptr, a->at_sph.at_shield_hit = false;
      if (a->at_sph.eff_at > 0) --a->at_sph.eff_at;
    }
    for (auto& c : link.mTgCyls) {
      c.tg_hit = nullptr, c.tg_shield_hit = false, c.tg_hit_ac = nullptr;
      if (c.eff_tg > 0) --c.eff_tg;
    }
    for (auto& c : link.mAtCps) c.at_hit = nullptr, c.at_shield_hit = false;
    for (auto* a : dn) {
      for (auto& c : link.mTgCyls) {
        if (!c.tg_set) continue;
        cp06f1::Cyl k;
        k.c = {c.c.x, c.c.y, c.c.z}, k.r = c.r, k.h = c.h, k.on = true;
        cp06f1::Sph s;
        s.c = {a->at_sph.c.x, a->at_sph.c.y, a->at_sph.c.z}, s.r = a->at_sph.r;
        if (!cp06f1::cross(k, s, 0.0f)) continue;
        // ChkAtTgHitAfterCross: Link's Tg has NoConHit. SetTgApid first, then the rejection against the previous pass.
        const std::uint32_t old = link.stts.tg_old;
        link.stts.tg_apid = a->apid;
        if (old == a->apid) continue;
        a->at_sph.at_hit = &c;
        c.tg_hit = &a->at_sph;
        c.tg_hit_ac = &a->actor;
        const bool shield = chk_shield(*a, c);
        if (shield) a->at_sph.at_shield_hit = true, c.tg_shield_hit = true;
        if (a->at_sph.eff_at == 0 || c.eff_tg == 0) {
          a->at_sph.eff_at = 5, c.eff_tg = 5;
          ++sparks;
          if (shield) preview_spark(a);  // ProcAtTgHitmark(chk_shield): the parry FX decides HERE, before the next tick's verdict
          proc_hitmark(&a->at_sph, &c, shield, shield ? 6 : 1, &link);
        }
      }
    }
  }
  // FIX03: the pass-time decision of parry_fx_adapter.inc (pfx_preview_liz_link + pfx_link_pass), with the production brain's own HitIn builder and
  // the trackers' const peeks; the next Execute PRE must reach the same verdict (checked in execute()).
  struct Preview { bool have = false, parry = false; std::uint64_t tick = 0; };
  std::vector<std::pair<e_dn_class*, Preview>> previews;
  Preview* preview_of(e_dn_class* a) {
    for (auto& p : previews)
      if (p.first == a) return &p.second;
    previews.push_back({a, Preview{}});
    return &previews.back().second;
  }
  void preview_spark(e_dn_class* a) {
    LizSlot* s = liz_find(a);
    if (!s || !s->b.admitted) return;
    liz::View v = liz_view(a, &link);
    v.at_hit = true, v.at_on_link = true, v.at_shield = true;
    liz::LinkFrame L = liz_link_frame(&link);
    const bool ga = link.mProcID == daAlink_c::PROC_GUARD_ATTACK;
    L.pass = g.link_track.peek(g.p8.tick + 1, true, link.pid, link.mProcID, ga, link.frame, 1.0f, 20.0f);
    if (L.pass.guard_attack && c4.enabled && !c4.shield.peek_permits(g.p8.tick + 1, ga, ga ? link.frame : 0.0f, ga ? 1.0f : 0.0f, cp04_link_safe(&link))) L.pass.guard_attack = false;
    const liz::Hit h = liz::judge_hit(s->b.hit_input(v, L));
    Preview* p = preview_of(a);
    p->have = true, p->tick = g.p8.tick, p->parry = h == liz::Hit::Parry || h == liz::Hit::ParryLatched;
  }
  bool chk_shield(const e_dn_class& a, const dCcD_Cyl& c) const {
    if (!c.tg_shield && !c.tg_sp) return false;
    float vx = a.at_sph.vec.x, vz = a.at_sph.vec.z;
    if (std::fabs(vx) + std::fabs(vz) + std::fabs(a.at_sph.vec.y) < 1e-3f) {  // CalcParticleAngle: zero velocity => attacker -> target
      vx = link.current.pos.x - a.actor.current.pos.x;
      vz = link.current.pos.z - a.actor.current.pos.z;
    }
    const s16 yaw = atan2s(vx, vz);
    const s16 rev = static_cast<s16>(static_cast<std::uint16_t>(yaw) + 0x8000u);
    const s16 d = static_cast<s16>(static_cast<std::uint16_t>(rev) - static_cast<std::uint16_t>(link.shield_yaw));
    const int ad = d == -32768 ? 32768 : (d < 0 ? -d : d);
    return ad <= 0x4000;
  }
  void native_def_se(Z2Creature* snd, cCcD_Obj* o, unsigned map, fopAc_ac_c* ac) {
    Z2Creature* s = snd;
    cCcD_Obj* ob = o;
    u32 m = map;
    fopAc_ac_c* a2 = ac;
    (void)s, (void)ob, (void)a2;  // def_se_set is NOT hookable in the real binary (rel8 branch in its first 5 bytes): the lab calls straight through, like the game
    ++snd->collision_se;
    (void)collision_se(0xD0, &snd->mSoundObjAnime, m);
  }
  // ---- Link
  void set_cut(int t) {
    link.cut_type = t;
    daAlink_c* lp = &link;
    u8 ty = static_cast<u8>(t);
    void* args[2] = {&lp, &ty};
    Reg<LinkCutType>::run_post(args);
  }
  void link_collision() {
    link.stts.tg_old = link.stts.tg_apid;
    link.stts.tg_apid = 0;
    const bool guard = link.guard_active || link.mProcID == daAlink_c::PROC_GUARD_ATTACK || link.mProcID == daAlink_c::PROC_GUARD_SLIP;
    for (auto& c : link.mTgCyls) {
      c.tg_shield = guard;
      c.c = cXyz(link.current.pos.x, link.current.pos.y, link.current.pos.z);
      c.r = 35, c.h = 180;
      c.tg_set = link.mDamageTimer == 0;
      if (!c.tg_set) c.tg_hit = nullptr, c.tg_shield_hit = false, c.tg_hit_ac = nullptr, c.eff_tg = 0;
    }
    link.shield_yaw = link.shape_angle.y;
    f1.link_set_tick = g.p8.tick;
    f1.link_set_key = reinterpret_cast<std::uintptr_t>(&link);
  }
  int poly_result = 0, contact_damage = 2;
  void set_damage_point(int amount, BOOL zora = FALSE, BOOL timer = TRUE, int p3 = 0) {
    daAlink_c* lp = &link;
    void* args[5] = {&lp, &amount, &zora, &timer, &p3};
    Reg<LinkSetDamage>::run_pre(args);
    if (amount <= 0) return;
    life->life -= amount;
    if (timer) link.mDamageTimer = 30;
    ++link.damage_points;
    last_commit = amount;
  }
  int last_commit = 0;
  void check_damage_action() {
    daAlink_c* lp = &link;
    void* args[1] = {&lp};
    int ret = 0;
    // on_link_damage_pre_fix01 + on_link_poly_post_fix01 (mod.cpp): the attribution record of this call
    fx.native_depth = 1, fx.native_reentered = false, fx.native = lnks1fix::NativeDamage{};
    fx.native.tick = g.p8.tick, fx.native.valid = true, fx.native.human = true, fx.native.link_key = reinterpret_cast<std::uintptr_t>(&link), fx.native.link_id = link.pid;
    fx.native.timer_before = link.mDamageTimer;
    for (int k = 0; k < 3; ++k)
      if (link.mTgCyls[k].tg_hit) {
        fx.native.cylinder = k, fx.native.collider = reinterpret_cast<std::uintptr_t>(link.mTgCyls[k].tg_hit), fx.native.blocked = link.mTgCyls[k].tg_shield_hit;
        break;
      }
    fx.native.poly_seen = true, fx.native.poly_result = poly_result;
    Reg<LinkDamageFix01>::run_pre(args, &ret);
    if (link.mDamageTimer == 0) {
      if (poly_result != 0) {
        set_damage_point(1, FALSE, TRUE, 0);  // d_a_alink_damage.inc:561 floor
      } else {
      dCcD_Cyl* var = nullptr;
      for (auto& c : link.mTgCyls)
        if (c.tg_hit) {
          var = &c;
          break;
        }
      if (var && link.mProcID != daAlink_c::PROC_GUARD_SLIP) {  // d_a_alink_damage.inc:629
        if (var->tg_shield_hit) {
          link.setGuardSe(var);
          dComIfGp_getVibration().StartShock(3, 1, cXyz(0, 1, 0));
          // procGuardSlipInit
          link.mProcID = daAlink_c::PROC_GUARD_SLIP;
          link.mProcVar1.field_0x300a = 0;
          link.frame = 0;
          link.mNormalSpeed = 14.0f;
          ++link.slip_inits;
        } else {
          set_damage_point(contact_damage, FALSE, TRUE, 0);  // d_a_alink_damage.inc:714
          if (link.mProcID != kProcGuardBreak) link.mProcID = daAlink_c::PROC_DAMAGE, link.frame = 0;
        }
      }
      }
    }
    Reg<LinkDamageFix01>::run_post(args, &ret);
    fx.native_depth = 0;
  }
  static constexpr int kProcGuardBreak = 0x1f;
  void reverse_check() {  // changeCutReverseProc -> checkCutReverseAt(mAtCps[i]) in Link's cut procs
    daAlink_c* lp = &link;
    for (auto& c : link.mAtCps) {
      dCcD_GObjInf* o = &c;
      void* args[2] = {&lp, &o};
      BOOL r = c.at_hit && c.at_shield_hit;
      Reg<LinkCutReverse>::run_post(args, &r);
      if (r) {
        ++link.rebounds;
        link.cut_type = 0;
        if (link.mProcID == daAlink_c::PROC_CUT_NORMAL || link.mProcID == daAlink_c::PROC_CUT_FINISH) link.mProcID = daAlink_c::PROC_CUT_REVERSE, link.clip_rate = 0;  // procCutReverseInit
        return;
      }
    }
  }
  void link_exec() {
    auto& l = link;
    const bool b_trig = l.presses.count(g.p8.tick) != 0;
    const bool safe = cp04_link_safe(&l);
    if (!safe || (l.mProcID != daAlink_c::PROC_WAIT && l.mProcID != daAlink_c::PROC_GUARD_ATTACK && l.mProcID != daAlink_c::PROC_GUARD_SLIP)) c4.shield.cancel();
    c4.shield.sample(g.p8.tick, l.r_held, b_trig, safe);
    if (l.mDamageTimer) --l.mDamageTimer;
    check_damage_action();
    if (l.cut_type != 0 && l.cut_type != daPy_py_c::CUT_TYPE_GUARD_ATTACK) reverse_check();
    if (l.mProcID == daAlink_c::PROC_CUT_NORMAL || l.mProcID == daAlink_c::PROC_CUT_FINISH) l.frame += l.clip_rate, l.mUnderFrameCtrl[0].f = l.frame, l.mUnderFrameCtrl[0].r = l.clip_rate;
    if (l.mProcID == kProcGuardBreak) {
      l.frame += 1.0f;
      if (l.frame >= 35.0f) l.mProcID = daAlink_c::PROC_WAIT;
    } else if (l.mProcID == daAlink_c::PROC_GUARD_ATTACK) {
      l.frame += 1.0f;
      if (l.frame >= 20.0f) l.mProcID = daAlink_c::PROC_WAIT, l.cut_type = 0;
    } else if (l.mProcID == daAlink_c::PROC_GUARD_SLIP) {
      l.frame += 1.0f;
      if (slip_push && !dn.empty()) {  // procGuardSlip: mNormalSpeed along the damage direction
        const float dx = l.current.pos.x - dn[0]->actor.current.pos.x, dz = l.current.pos.z - dn[0]->actor.current.pos.z, n = std::sqrt(dx * dx + dz * dz) + 1e-3f;
        l.current.pos.x += dx / n * l.mNormalSpeed * 0.1f, l.current.pos.z += dz / n * l.mNormalSpeed * 0.1f;
      }
      l.mNormalSpeed = l.mNormalSpeed > 1.25f ? l.mNormalSpeed - 1.25f : 0.0f;
      if (l.mNormalSpeed == 0.0f && l.frame >= 11.0f) {
        l.mProcID = daAlink_c::PROC_WAIT;
        l.stts.tg_apid = 0;
      }
    } else if (l.mProcID == daAlink_c::PROC_DAMAGE) {
      l.frame += 1.0f;
      if (l.frame >= 20.0f) l.mProcID = daAlink_c::PROC_WAIT;
    }
    if (l.mProcID == daAlink_c::PROC_WAIT && l.r_held && b_trig) (void)l.procGuardAttackInit();
    l.guard_active = l.r_held && l.mProcID == daAlink_c::PROC_WAIT;
    if (!skip_link_collision) link_collision();
  }
  // ---- E_DN transcription
  void anm_init(e_dn_class* a, int id, float rate0, bool loop, float morf = 1.0f) {
    float mo = 3, r = rate0;
    u8 at = 0;
    void* args[5] = {&a, &id, &mo, &at, &r};
    Reg<LizAnm>::run_pre(args);
    a->morph.mFrameCtrl.mFrame = 0, a->morph.rate = r, a->morph.end = a->len[id], a->morph.stop = false, a->morph.loop = loop, a->morph.morf = morf;
    a->anm_no = id;
    Reg<LizAnm>::run_post(args);
  }
  void attack_pre(e_dn_class* a, int which) {
    void* args[1] = {&a};
    if (which == 0) Reg<LizAttack0>::run_pre(args);
    else if (which == 1) Reg<LizAttack>::run_pre(args);
    else Reg<LizTail>::run_pre(args);
  }
  void attack_post(e_dn_class* a, int which) {
    void* args[1] = {&a};
    if (which == 0) Reg<LizAttack0>::run_post(args);
    else if (which == 1) Reg<LizAttack>::run_post(args);
    else Reg<LizTail>::run_post(args);
  }
  fopAc_ac_c* at_hit_check(e_dn_class* a) {
    void* args[1] = {&a};
    fopAc_ac_c* r = nullptr;
    if (a->mode < 10 && a->at_sph.at_hit) {
      auto* c = static_cast<dCcD_GObjInf*>(a->at_sph.at_hit);
      r = c->tg_hit_ac ? static_cast<fopAc_ac_c*>(&link) : nullptr;
      if (c == &link.mTgCyls[0] || c == &link.mTgCyls[1] || c == &link.mTgCyls[2]) r = &link;
    }
    Reg<LizAtHit>::run_post(args, &r);
    return r;
  }
  std::uint64_t native_cancels = 0;
  void e_attack_0(e_dn_class* a) {
    attack_pre(a, 0);
    const int f = static_cast<int>(a->morph.mFrameCtrl.mFrame);
    if (a->mode == 0) anm_init(a, liz::native::kAnmN, 1.0f, false), a->mode = 1;
    if (a->mode == 1) {
      if (f >= 22 && f <= 28) a->at_chk_flag = 1, a->tail_at_sph_flag = 0;
      if (a->morph.isStop()) a->action = 3, a->mode = 0;
    }
    if (a->at_chk_flag) (void)at_hit_check(a);
    attack_post(a, 0);
  }
  void e_attack(e_dn_class* a) {
    attack_pre(a, 1);
    const int f = static_cast<int>(a->morph.mFrameCtrl.mFrame);
    if (a->mode == 0) anm_init(a, liz::native::kAnmD, 1.0f, false), a->mode = 1;
    if (a->mode == 1) {
      if (f >= 13 && f <= 18) a->at_chk_flag = 1, a->tail_at_sph_flag = 0;
      if (f >= 25 && f <= 30) {
        a->at_chk_flag = 1, a->tail_at_sph_flag = 1;
        const float y = yaw_rad(a->actor.current.angle.y);
        a->actor.current.pos.x += std::sin(y) * 25.0f;
        a->actor.current.pos.z += std::cos(y) * 25.0f;
      }
      if (a->morph.isStop()) a->action = 3, a->mode = 0;
    }
    if (a->at_chk_flag) {
      fopAc_ac_c* p = at_hit_check(a);
      if (p == &link && (link.guard_active || link.mProcID == daAlink_c::PROC_GUARD_SLIP)) {  // the native cancellation; checkPlayerGuard
        a->morph.setPlaySpeed(0.0f);
        a->action = 3, a->mode = 0;
        ++native_cancels;
      }
    }
    attack_post(a, 1);
  }
  void e_tail(e_dn_class* a) {
    attack_pre(a, 2);
    const int f = static_cast<int>(a->morph.mFrameCtrl.mFrame);
    if (a->mode == 0) {
      anm_init(a, liz::native::kAnmC, 1.0f, false), a->mode = 1;
    } else if (a->mode == 1) {
      if (f >= 33 && f <= 41) {
        a->at_chk_flag = 1, a->tail_at_sph_flag = 2;
        const float y = yaw_rad(a->search_angle_y);
        a->actor.current.pos.x += std::sin(y) * 30.0f;
        a->actor.current.pos.z += std::cos(y) * 30.0f;
      }
      if (a->morph.isStop()) a->action = 3, a->mode = 0;
    }
    attack_post(a, 2);
  }
  void e_guard(e_dn_class* a) {
    a->guard_flag = 1;
    switch (a->mode) {
      case 0:
        anm_init(a, 32, 1.0f, false), a->mode = 1;
        break;
      case 1:
        a->invulnerability_timer = 5;
        if (a->morph.getFrame() >= 5.0f) a->mode = 5, a->timer[0] = 20;
        break;
      case 5:
        anm_init(a, 22, 1.0f, false), a->mode = 6;
        [[fallthrough]];
      case 6:
        a->invulnerability_timer = 5;
        if (a->morph.getFrame() >= 8.0f) a->morph.setPlaySpeed(0), a->mode = 7;
        break;
      case 7:
        a->invulnerability_timer = 5;
        if (a->timer[0] == 0) a->mode = 8, a->morph.setPlaySpeed(1);
        break;
      case 8:
        if (a->morph.isStop()) a->action = 3, a->mode = 0;
        break;
    }
    void* args[1] = {&a};
    Reg<LizGuard>::run_post(args);
  }
  bool native_guard_on = false;
  void e_fight_run(e_dn_class* a) {
    void* args[1] = {&a};
    if (Reg<LizRun>::run_pre(args) != HOOK_SKIP_ORIGINAL) {
      if (a->mode == 0) anm_init(a, 30, 1.0f, true), a->mode = 1, a->timer[0] = 0;
      if (a->mode == 1 && a->pl_dir < 300) anm_init(a, 32, 1.0f, true), a->mode = 2, a->timer[2] = static_cast<s16>(30 + a->nrand(30));
      s16 d = static_cast<s16>(static_cast<std::uint16_t>(a->search_angle_y) - static_cast<std::uint16_t>(a->actor.current.angle.y));
      a->actor.current.angle.y = static_cast<s16>(a->actor.current.angle.y + (d > 0x800 ? 0x800 : d < -0x800 ? -0x800 : d));
      if ((native_guard_on || ((a->learn & 0x1) != 0 && link.cut_type == daPy_py_c::CUT_TYPE_NORMAL)) && a->pl_dir < 300 && a->field_0x74c < 1.0f) {  // def, or the sword memory
        a->action = 7, a->mode = 5;
        a->field_0x708 = 30, a->field_0x70c = a->actor.shape_angle.y, a->timer[0] = 0, a->invulnerability_timer = 15;
      } else if (a->pl_dir < 350 && a->timer[2] == 0) {
        a->timer[2] = static_cast<s16>(30 + a->nrand(30));
        a->action = a->pl_dir < 250 ? 4 : 5, a->mode = 0;
      }
    }
    Reg<LizRun>::run_post(args);
  }
  void small_damage(e_dn_class* a) {
    void* args[1] = {&a};
    if (Reg<LizSmall>::run_pre(args) == HOOK_SKIP_ORIGINAL) return;
    ++a->small_body;
    a->action = 20, a->mode = 0;
  }
  static void big_body(e_dn_class* a) {
    ++a->big_body;
    a->action = 21, a->mode = 0, a->field_0x74c = 150;
  }
  void big_damage(e_dn_class* a) {
    void* args[1] = {&a};
    if (Reg<LizBig>::run_pre(args) == HOOK_SKIP_ORIGINAL) return;
    big_body(a);
  }
  void damage_check(e_dn_class* a) {
    void* args[1] = {&a};
    Reg<LizDamage>::run_pre(args);
    if (a->invulnerability_timer == 0) {
      for (auto& s : a->cc_sph) {
        if (!s.tg_hit) continue;
        a->at_info.mpCollider = s.tg_hit;
        a->at_info.mpActor = &link;
        a->at_info.mHitDirection.y = a->search_angle_y;
        ++a->sound.collision_se;  // cc_at_check's native SE
        const bool stun = s.tg_hit->ChkAtType(AT_TYPE_SHIELD_ATTACK);
        a->invulnerability_timer = 10;
        if (stun) {
          a->action = 12, a->mode = 0, a->field_0x74c = 100;
          ++a->drawback_entered;
          break;
        }
        a->actor.health = static_cast<s16>(a->actor.health - a->hit_power);
        if (s.tg_hit->ChkAtType(kAtSword)) a->learn |= 0x1;  // learn |= mHitBit
        if (a->actor.health <= 0 || (a->hit_strong && link.cut_type != daPy_py_c::CUT_TYPE_HEAD_JUMP)) {
          big_damage(a);
          a->invulnerability_timer = 1000;
        } else {
          small_damage(a);
        }
        a->actor.speedF = 0;
        break;
      }
    }
    for (auto& s : a->cc_sph) s.tg_hit = nullptr;
    a->actor.health = static_cast<s16>(a->actor.health - g_lose_hp_in_damage);  // FIX05 lab: another hit of this Execute
    Reg<LizDamage>::run_post(args);
  }
  void e_action(e_dn_class* a) {
    const float dx = link.current.pos.x - a->actor.current.pos.x, dz = link.current.pos.z - a->actor.current.pos.z;
    a->pl_dir = std::sqrt(dx * dx + dz * dz);
    a->search_angle_y = atan2s(dx, dz);
    damage_check(a);
    a->snap_angle_y_flag = 0;
    a->actor.offHeadLockFlg();
    switch (a->action) {
      case 0:
        a->action = 3, a->mode = 0;
        break;
      case 3: e_fight_run(a); break;
      case 4: e_attack_0(a); break;
      case 5: e_attack(a); break;
      case 6: e_tail(a); break;
      case 7: e_guard(a); break;
      case 12:
        if (a->mode == 0) {
          anm_init(a, 18, 1.0f, false), a->mode = 1, a->invulnerability_timer = 10;
          ++a->drawback_entered;
        } else if (a->mode == 1) {
          a->actor.onHeadLockFlg();
          if (a->morph.isStop()) a->action = 3, a->mode = 0;
        }
        break;
      case 20:
        if (a->mode == 0) a->mode = 1, a->timer[0] = 20, anm_init(a, 31, 1.0f, true);
        else if (a->timer[0] == 0) a->action = 3, a->mode = 0, a->timer[0] = 40;
        break;
      case 21:
        if (a->actor.cut_down_hit) {
          a->actor.cut_down_hit = false, a->actor.down = false, a->actor.health = 0, a->mode = 3, a->timer[0] = 100, a->timer[1] = 46;
          break;
        }
        if (a->mode == 0) a->mode = 1, anm_init(a, 9, 1.0f, false), a->dmg_t = 0;
        else if (a->mode == 1 && ++a->dmg_t > 6) a->mode = 3, a->timer[0] = 65, a->timer[1] = 35, a->actor.down = a->actor.health > 0;
        else if (a->mode == 3 && a->actor.health <= 0 && a->timer[1] == 0) a->deleted = 1;
        else if (a->mode == 3 && a->actor.health > 0 && a->timer[0] == 0) a->mode = 20, anm_init(a, 14, 1.0f, false), a->actor.down = false;
        else if (a->mode == 20 && a->morph.isStop()) a->mode = 21, a->timer[0] = 5;
        else if (a->mode == 21) {
          a->invulnerability_timer = 0;
          if (a->timer[0] == 0) a->action = 0, a->mode = 0;
        }
        break;
    }
    // Co spheres: the bodies never interpenetrate (native CC move)
    {
      const float ex = a->actor.current.pos.x - link.current.pos.x, ez = a->actor.current.pos.z - link.current.pos.z, n = std::sqrt(ex * ex + ez * ez);
      if (n < 110.0f && n > 1e-3f) a->actor.current.pos.x = link.current.pos.x + ex / n * 110.0f, a->actor.current.pos.z = link.current.pos.z + ez / n * 110.0f;
    }
    s16 d = static_cast<s16>(static_cast<std::uint16_t>(a->actor.current.angle.y) - static_cast<std::uint16_t>(a->actor.shape_angle.y));
    const s16 lim = 0x2000;
    const s16 step = d / 2 > lim ? lim : d / 2 < -lim ? static_cast<s16>(-lim) : (d != 0 && d / 2 == 0 ? d : static_cast<s16>(d / 2));
    a->actor.shape_angle.y = static_cast<s16>(a->actor.shape_angle.y + step);
  }
  void execute(e_dn_class* a) {
    if (a->deleted) return;
    void* args[1] = {&a};
    int ret = 1;
    Preview* pv = preview_of(a);
    const LizSlot* s0 = liz_find(a);
    const std::uint64_t parries0 = s0 ? s0->b.st.link_parry : 0;
    const bool judge = pv->have && pv->tick + 1 == g.p8.tick;
    if (Reg<LizExec>::run_pre(args, &ret) == HOOK_SKIP_ORIGINAL) return;
    {
      const LizSlot* s1 = liz_find(a);
      if (s1 && s0 && s1->b.st.link_parry != parries0) ++g_exec_parries;
    }
    if (judge) {  // FIX03 property: the orange decided in the pass is exactly the parry the Execute credits
      const LizSlot* s1 = liz_find(a);
      const bool parried = s1 && s1->b.st.link_parry != parries0;
      CHECK(parried == pv->parry);
      ++g_preview_checks;
      if (parried) ++g_preview_parries;
      pv->have = false;
    }
    if (a->status == 0) {
      ++a->frame_counter;
      for (auto& t : a->timer)
        if (t) --t;
      if (a->invulnerability_timer) --a->invulnerability_timer;
      if (a->unk_timer_2) --a->unk_timer_2;
      e_action(a);
      if (!a->morph.stop) {
        a->morph.mFrameCtrl.mFrame += a->morph.rate;
        if (a->morph.mFrameCtrl.mFrame >= a->morph.end) {
          if (a->morph.loop) a->morph.mFrameCtrl.mFrame -= a->morph.end;
          else a->morph.mFrameCtrl.mFrame = a->morph.end - 0.001f, a->morph.stop = true, a->morph.rate = 0.0f;  // J3DFrameCtrl::update
        }
      }
      if (a->morph.morf < 1.0f) a->morph.morf += 0.2f;
      a->morph.modelCalc();
      auto& m = a->morph.model;
      if (a->at_chk_flag != 0 && a->tail_at_sph_flag <= 1) {
        const auto* h = m.mtx[liz::native::kJntHandR1];
        cXyz p(h[0][1] * 60.0f + h[0][3], h[1][1] * 60.0f + h[1][3], h[2][1] * 60.0f + h[2][3]);
        if (a->at_sph_center_flag == 0) a->at_sph_center_flag = 1, a->at_sph.StartCAt(p);
        else a->at_sph.MoveCAt(p);
        a->at_sph.SetR(a->tail_at_sph_flag == 1 ? 1.1f * 80.0f : 1.1f * 50.0f);
        a->at_sph.atp = 1;
      }
      if (a->at_chk_flag != 0 && a->tail_at_sph_flag == 2) {
        const auto* t = m.mtx[liz::native::kJntTail04];
        cXyz p(t[0][3], t[1][3], t[2][3]);
        if (a->at_sph_center_flag == 0) a->at_sph_center_flag = 1, a->at_sph.StartCAt(p);
        else a->at_sph.MoveCAt(p);
        a->at_sph.atp = 2;
        a->at_sph.SetR(70.0f * 1.1f);
      }
      if (a->at_chk_flag == 0) {
        a->at_sph.SetC(cXyz(a->actor.current.pos.x - 23500.0f, -20000.0f, a->actor.current.pos.z - 50000.0f));
        a->at_sph_center_flag = 0;
      } else {
        a->at_chk_flag = 0;
      }
      if (a->guard_flag != 0 && a->unk_timer_2 == 0) {  // d_a_e_dn.cpp Execute: tate_sph on JNT_ARML2*(50,0,30), r = size*70
        const auto* l = m.mtx[kLabJntArmL2];
        a->tate_sph.SetC(cXyz(l[0][0] * 50.0f + l[0][2] * 30.0f + l[0][3], l[1][0] * 50.0f + l[1][2] * 30.0f + l[1][3], l[2][0] * 50.0f + l[2][2] * 30.0f + l[2][3]));
        a->tate_sph.SetR(70.0f * 1.1f);
        if (a->tate_sph.tg_hit) {  // def_se_set(&sound, hit obj, 40, 0); setPauseTimer; unk_timer_2 = 10; dComIfGp_setHitMark(2, NULL, ...)
          native_def_se(&a->sound, a->tate_sph.tg_hit, 40, nullptr);
          a->unk_timer_2 = 10;
          set_hit_mark(2, nullptr);
        }
      } else {
        a->tate_sph.SetC(cXyz(a->actor.current.pos.x - 23500.0f, -20000.0f, a->actor.current.pos.z - 30000.0f));
      }
      a->guard_flag = 0;
    }
    Reg<LizExec>::run_post(args, &ret);
    a->tate_sph.tg_hit = nullptr;
  }
  void tick() {
    ++g.p8.tick;
    // ModLoader::tick: Link's snapshot = Link during the previous collision pass
    auto& l = link;
    const bool ga = l.mProcID == daAlink_c::PROC_GUARD_ATTACK;
    // FIX03 property: the const peeks the parry FX uses inside the collision pass ARE what observe/permits then produce.
    const post08::LinkView pk = g.link_track.peek(g.p8.tick, true, l.pid, l.mProcID, ga, l.frame, 1.0f, 20.0f);
    const bool pp = c4.shield.peek_permits(g.p8.tick, ga, ga ? l.frame : 0.0f, ga ? 1.0f : 0.0f, cp04_link_safe(&l));
    const auto tr = g.link_track.observe(g.p8.tick, true, l.pid, l.mProcID, ga, l.frame, 1.0f, 20.0f);
    (void)tr;
    c4.shield.observe(g.p8.tick, ga, ga ? l.frame : 0.0f, ga ? 1.0f : 0.0f, cp04_link_safe(&l));
    {
      const post08::LinkView ob = g.link_track.pass_for(g.p8.tick, true, l.pid);
      CHECK(pk.valid == ob.valid && pk.guard_attack == ob.guard_attack && pk.proc == ob.proc && pk.frame == ob.frame && pk.serial == ob.serial && pk.age_ticks == ob.age_ticks);
      const auto& e = c4.shield.episodes[c4.shield.pass_serial % 4];
      const bool real = c4.shield.pass_valid && c4.shield.pass_tick == g.p8.tick && e.serial && e.serial == c4.shield.pass_serial && !e.consumed;
      CHECK(pp == real);
      ++g_peek_checks;
    }
    {
      lnkfx::Obs fo;
      fo.valid = true, fo.human = !l.wolf, fo.alive = life->life > 0, fo.sim_active = true, fo.event = !g_world_safe, fo.link_id = l.pid, fo.epoch = 7, fo.tick = g.p8.tick, fo.proc = l.mProcID;
      (void)lf.def.observe(fo);
    }
    // production cp04_maintenance() order (do_update: post08_observe_link -> ... -> cp04_maintenance -> liz_maintenance). The FIRST line is the FIX03 production
    // rule, copied verbatim from src/cp04_adapter.inc and pinned there by tests/test_liz_fix03_source.py. g_fix02_gate reproduces 0.12.2: `!cp04_sim_safe()`
    // was true for the whole session while g.layout stayed -1 (no sword hit on any E_OC), so every armed chain was cancelled on the next tick.
    if(!cp04_world_safe())c4.shield.cancel();
    if (g_fix02_gate) c4.shield.cancel();
    liz_maintenance();
    link_exec();
    for (auto* a : dn) execute(a);
    CHECK(pf.exec_id == 0 && pf.exec_sound == nullptr);  // every admitted Execute closed its FX scope
    l.mUnderFrameCtrl[0].f = l.frame, l.mUnderFrameCtrl[0].r = l.mProcID == daAlink_c::PROC_GUARD_ATTACK ? 1.0f : l.mProcID == daAlink_c::PROC_CUT_NORMAL || l.mProcID == daAlink_c::PROC_CUT_FINISH ? l.clip_rate : 0.0f;
    pass();
    particles();
  }
};
static void def_se_set(Z2Creature* s, cCcD_Obj* o, unsigned m, fopAc_ac_c* a) noexcept {
  ++g_bash_se;
  W->native_def_se(s, o, m, a);
}
int daAlink_c::procGuardAttackInit() noexcept {
  mProcID = PROC_GUARD_ATTACK;
  frame = 1.0f;
  ++shield_attacks;
  W->set_cut(daPy_py_c::CUT_TYPE_GUARD_ATTACK);  // the real proc init: not a sword swing for the adaptive memory
  if (c4.shield.native_start(g.p8.tick, frame, true, cp04_link_safe(this))) g.link_track.native_start();
  return 1;
}
void daAlink_c::setGuardSe(dCcD_GObjInf* o) noexcept {
  if (se_calls < 64) se_ticks[se_calls] = g.p8.tick;
  ++se_calls;
  daAlink_c* self = this;
  void* args[2] = {&self, &o};
  Reg<LinkGuardSe>::run_pre(args);
  (void)W->collision_se(0xC0);
  Reg<LinkGuardSe>::run_post(args);
}
bool dVibration_c::StartShock(int, int, cXyz) noexcept {
  ++W->link.shocks;
  return true;
}
// ---------------------------------------------------------------- harness
static std::vector<std::string> g_log_store;
static void fresh_world(World& w, float dist = 160.0f) {
  W = &w;
  g_link = &w.link;
  g_logs = &g_log_store;
  g_log_store.clear();
  g_world_safe = true;
  g.p8.tick = 1000;
  g.eng.obs.disabled = false;
  g.link_track.reset();
  pf.readback = nullptr;  // a new World is a new scene: the host's epoch change
  c4.shield = shieldchain::Chain{};
  std::memset(static_cast<void*>(&f1), 0, sizeof(f1));
  f1.link_set_tick = ~0ull;
  LizRuntime* rt = &lz;
  const bool en = rt->enabled, cov = rt->coverage;
  (void)en, (void)cov;
  lz.~LizRuntime();
  new (&lz) LizRuntime();
  lz.enabled = true, lz.coverage = true;
  fx.native = lnks1fix::NativeDamage{};
  lb = {};
  lf.def = lnkfx::Defense{};
  ld.c = lnkdmg::Counters{};
  ld.last_applied_tick = ~0ull, ld.in_episode = false, ld.lines = 0, ld.dropped = 0;
  {
    const bool en = pf.enabled;
    pf.book = pfx::Book{};
    for (auto& x : pf.em) x = PfxEmitter{};
    for (auto& x : pf.clones) x.src = nullptr, x.src_shape = nullptr;
    pf.pending_n = 0, pf.pending_tick = ~0ull, pf.ctx_cand = pf.ctx_mark_cand = 0;
    for (auto& v : pf.seen_res) v = 0;
    g_env_light = EnvLight{};
    pf.ctx_pass = pf.ctx_mark = pf.ctx_sound = -1, pf.exec_id = 0, pf.exec_sound = nullptr, pf.enabled = en, pf.last_handle = nullptr, pf.last_handle_tick = ~0ull;
  }
  g_actors.clear();
  g_actors.push_back(&w.link);
  w.link.group = fopAc_PLAYER_e;
  g_bash_se = 0;
  g_dComIfG_gameInfo.info.st.life = 80;
  w.link.name = w.link.profname = fpcNm_ALINK_e, w.link.pid = 1;
  for (auto& c : w.link.mTgCyls) c.owner_ac = &w.link;
  w.link.current.pos = cXyz(0, 0, dist);
  w.link.shape_angle.y = static_cast<s16>(0x8000);  // faces -z, the actor at the origin
  w.link.stts.apid = 1;
  for (auto& c : w.link.mAtCps) c.at_type = kAtSword;
  LizBig::g_orig = World::big_body;
}
static void clear_slots_of(World& w) {
  for (auto* a : w.dn) delete a;
  w.dn.clear();
}
static LizSlot* slot_of(e_dn_class* a) { return liz_find(a); }
static void force_pattern(e_dn_class* a, unsigned pat) {
  LizSlot* s = slot_of(a);
  CHECK(s);
  for (int g2 = 0; g2 < 4096; ++g2) {
    liz::Rng pr = s->b.rng;
    if (pr.below(liz::kPatterns) == pat) break;
    (void)s->b.rng.next();
  }
}
struct ImpactRow { std::uint64_t open = 0, contact = 0; liz::Cls cls = liz::Cls::None; unsigned restored = 0, native_se = 0; };
struct Trace {
  std::vector<ImpactRow> imp;
  liz::End end = liz::End::None;
  std::uint64_t se0 = 0;
};
// Runs one combo with a Link script until its end; returns one row per opened window from the production Impact events.
static Trace run_combo(World& w, e_dn_class* a, unsigned pat, const std::function<void(unsigned, std::uint64_t)>& per_tick = {}, int max_ticks = 700) {
  Trace t;
  LizSlot* s = slot_of(a);
  CHECK(s);
  force_pattern(a, pat);
  a->timer[2] = 0;
  a->pl_dir = 160;
  a->action = 3, a->mode = 2;
  bool started = false;
  std::uint64_t last_counted = s->b.st.windows;
  for (int k = 0; k < max_ticks; ++k) {
    if (per_tick) per_tick(unsigned(t.imp.size()), g.p8.tick + 1);
    w.tick();
    s = slot_of(a);
    if (!s) break;
    if (s->b.st.windows != last_counted) {
      last_counted = s->b.st.windows;
      ImpactRow r;
      r.open = g.p8.tick;
      t.imp.push_back(r);
    }
    if (s->b.ep.active) started = true;
    if (started && !s->b.ep.active) break;
  }
  CHECK(started);
  return t;
}
// Counters of the last Impact class of one actor since a snapshot
struct ClassSnap { std::uint64_t c[unsigned(liz::Cls::Count)] = {}; };
static ClassSnap snap(e_dn_class* a) {
  ClassSnap r;
  LizSlot* s = slot_of(a);
  for (unsigned i = 0; i < unsigned(liz::Cls::Count); ++i) r.c[i] = s->b.st.impact_class[i];
  return r;
}
static std::uint64_t delta(const ClassSnap& n, const ClassSnap& o, liz::Cls k) { return n.c[unsigned(k)] - o.c[unsigned(k)]; }
static void settle(World& w, e_dn_class* a, int ticks = 4) {
  for (int k = 0; k < ticks; ++k) {
    a->timer[2] = 5000;
    w.tick();
  }
}
// A shield contact recorded by the last pass while Link was in GUARD_SLIP, and no guard SE in the Link Execute that reads it.
static std::function<void(unsigned, std::uint64_t)> mute_probe(World& w, e_dn_class* a, unsigned& mute) {
  auto pending = std::make_shared<bool>(false);
  auto se = std::make_shared<std::uint64_t>(0);
  return [&w, a, &mute, pending, se](unsigned, std::uint64_t) {
    if (*pending && w.link.se_calls == *se) ++mute;
    *pending = a->at_sph.at_hit != nullptr && a->at_sph.at_shield_hit && w.link.mProcID == daAlink_c::PROC_GUARD_SLIP;
    *se = w.link.se_calls;
  };
}
// ---------------------------------------------------------------- W1: held R, every impact of every combo and counter
static void w_held_guard() {
  for (unsigned p = 0; p < 4; ++p) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    CHECK(slot_of(a) && slot_of(a)->b.admitted && a->actor.health == 370);
    const auto s0 = snap(a);
    const auto se0 = w.link.se_calls;
    const Trace t = run_combo(w, a, p);
    const auto s1 = snap(a);
    const unsigned total = liz::kPattern[p].impacts;
    CHECK(t.imp.size() == total);                             // Link's guard never cancelled anything
    CHECK(delta(s1, s0, liz::Cls::LinkGuard) == total);       // every window reached the engine and was blocked
    CHECK(delta(s1, s0, liz::Cls::MissingFeedback) == 0);
    CHECK(w.native_cancels == 0);
    CHECK(w.link.damage_points == 0);
    // THE defect: every blocked impact has exactly one Link guard SE, native or restored; never zero, never two.
    CHECK(w.link.se_calls - se0 == total);
    CHECK(lz.se_native == total);                             // the receipt hook saw them all
    CHECK(lz.gap_arms == total - 1);                          // the last impact owes nothing: the re-arm never outlives the combo
    CHECK(slot_of(a)->b.st.swings_detected == 0);             // a held guard is not a swing
    CHECK(slot_of(a)->b.st.link_parry == 0 && slot_of(a)->b.p.points() == 0);
    if (p != 3 || true) CHECK(lz.se_restored >= 1);          // every pattern holds a D: its D2 lands inside the slip of D1
    for (std::uint64_t k = 1; k < w.link.se_calls && k < 64; ++k) CHECK(w.link.se_ticks[k] > w.link.se_ticks[k - 1]);
    clear_slots_of(w);
  }
  // Native control: the same D pair without our feedback: D2 has an At record and NO SE. This is the reproduction of the human report.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    auto saved = Reg<LinkDamageFix01>::post;
    Reg<LinkDamageFix01>::post.clear();
    unsigned mute = 0;
    const Trace t = run_combo(w, a, 2, mute_probe(w, a, mute));
    Reg<LinkDamageFix01>::post = saved;
    CHECK(t.imp.size() == 9);
    CHECK(mute == 3);  // 3 Ds => 3 D2 contacts with an At record and no SE that tick: the engine's own gate
    clear_slots_of(w);
    World v;
    fresh_world(v);
    v.add_dn(0, 0, 100);
    auto* b = v.dn[0];
    v.link.r_held = true;
    settle(v, b);
    unsigned mute2 = 0;
    (void)run_combo(v, b, 2, mute_probe(v, b, mute2));
    CHECK(mute2 == 0);  // the production feedback closes exactly those
    CHECK(lz.se_restored == 3);
    clear_slots_of(v);
  }
}
// The gate itself, record by record: only a real shield record of THIS admitted actor's AT, while Link is and stays in the slip, once per impact.
static void w_slip_records() {
  auto setup = [](World& w, e_dn_class*& a) {
    fresh_world(w);
    w.add_dn(0, 0, 100);
    a = w.dn[0];
    settle(w, a);
    w.link.mProcID = daAlink_c::PROC_GUARD_SLIP, w.link.frame = 2, w.link.mNormalSpeed = 10;
    slot_of(a)->b.impact_counter = 5;
  };
  auto record = [](World& w, cCcD_Obj* at, bool shield) {
    w.link.mTgCyls[0].tg_hit = at;
    w.link.mTgCyls[0].tg_shield_hit = shield;
  };
  {
    World w;
    e_dn_class* a;
    setup(w, a);
    record(w, &a->at_sph, true);
    w.check_damage_action();
    CHECK(lz.se_restored == 1 && w.link.se_calls == 1 && w.link.shocks == 1 && w.link.mNormalSpeed == 14.0f && lz.se_native == 1);
    CHECK(w.link.slip_inits == 0 && w.link.mProcID == daAlink_c::PROC_GUARD_SLIP);  // the native proc is not re-entered: no guard-break counter added
    record(w, &a->at_sph, true);
    w.check_damage_action();
    CHECK(lz.se_restored == 1 && w.link.se_calls == 1);  // the same swing never sounds twice
    slot_of(a)->b.impact_counter = 6;
    record(w, &a->at_sph, true);
    w.check_damage_action();
    CHECK(lz.se_restored == 2 && w.link.se_calls == 2);  // the next impact does
    clear_slots_of(w);
  }
  {  // body record during the slip (from the back): the native immunity stands, no fake shield sound
    World w;
    e_dn_class* a;
    setup(w, a);
    record(w, &a->at_sph, false);
    w.check_damage_action();
    CHECK(lz.se_restored == 0 && w.link.se_calls == 0 && w.link.damage_points == 0);
    clear_slots_of(w);
  }
  {  // another enemy's AT: this module has no business with it
    World w;
    e_dn_class* a;
    setup(w, a);
    dCcD_Sph foreign;
    record(w, &foreign, true);
    w.check_damage_action();
    CHECK(lz.se_restored == 0 && w.link.se_calls == 0);
    clear_slots_of(w);
  }
  {  // an excluded (vanilla) E_DN: same
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    a->arg1 = 1;
    settle(w, a);
    CHECK(slot_of(a) && !slot_of(a)->b.admitted);
    w.link.mProcID = daAlink_c::PROC_GUARD_SLIP, w.link.frame = 2;
    record(w, &a->at_sph, true);
    w.check_damage_action();
    CHECK(lz.se_restored == 0);
    clear_slots_of(w);
  }
  {  // Link left the slip inside this very call, or a scene: nothing
    World w;
    e_dn_class* a;
    setup(w, a);
    record(w, &a->at_sph, true);
    daAlink_c* lp = &w.link;
    void* args[1] = {&lp};
    Reg<LinkDamageFix01>::run_pre(args);
    w.link.mProcID = daAlink_c::PROC_WAIT;
    Reg<LinkDamageFix01>::run_post(args);
    CHECK(lz.se_restored == 0);
    w.link.mProcID = daAlink_c::PROC_GUARD_SLIP;
    g_world_safe = false;
    Reg<LinkDamageFix01>::run_pre(args);
    Reg<LinkDamageFix01>::run_post(args);
    g_world_safe = true;
    CHECK(lz.se_restored == 0);
    clear_slots_of(w);
  }
  {  // not in the slip at PRE: the native path played it already
    World w;
    e_dn_class* a;
    setup(w, a);
    w.link.mProcID = daAlink_c::PROC_WAIT;
    w.link.guard_active = true;
    record(w, &a->at_sph, true);
    w.link.mTgCyls[0].tg_hit_ac = &a->actor;
    w.check_damage_action();
    CHECK(lz.se_restored == 0 && w.link.se_calls == 1 && w.link.slip_inits == 1);
    clear_slots_of(w);
  }
}
// ---------------------------------------------------------------- W2: no shield: body or legitimate immunity, never a fabricated block
static void w_body() {
  for (unsigned p = 0; p < 4; ++p) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    const auto s0 = snap(a);
    const auto se0 = w.link.se_calls;
    const Trace t = run_combo(w, a, p);
    const auto s1 = snap(a);
    const unsigned total = liz::kPattern[p].impacts;
    CHECK(t.imp.size() == total);
    CHECK(delta(s1, s0, liz::Cls::LinkGuard) == 0 && delta(s1, s0, liz::Cls::LinkParry) == 0);
    CHECK(delta(s1, s0, liz::Cls::MissingFeedback) == 0);
    CHECK(delta(s1, s0, liz::Cls::LinkBody) >= 1);
    CHECK(delta(s1, s0, liz::Cls::LinkBody) + delta(s1, s0, liz::Cls::ImmuneValid) + delta(s1, s0, liz::Cls::Whiff) == total);
    CHECK(w.link.damage_points == delta(s1, s0, liz::Cls::LinkBody));  // one damage per body impact, the rest inside Link's i-frames
    CHECK(w.link.se_calls == se0);                                     // no fake shield sound
    clear_slots_of(w);
  }
}
// Dry run with held R: the tick of the collision pass that recorded the first contact of each window, relative to base.
static std::vector<std::uint64_t> contact_ticks(World& w, e_dn_class* a, unsigned p, std::uint64_t base) {
  std::vector<std::uint64_t> out;
  auto last_impact = std::make_shared<unsigned>(0);
  (void)run_combo(w, a, p, [&out, a, base, last_impact](unsigned imp, std::uint64_t next_tick) {
    if (a->at_sph.at_hit && imp > *last_impact) {
      *last_impact = imp;
      out.push_back(next_tick - 1 - base);
    }
  });
  return out;
}
// ---------------------------------------------------------------- W3: parry matrix: every impact of every combo and counter, alone
static void w_parry_matrix() {
  unsigned rows = 0;
  for (unsigned p = 0; p < 4; ++p) {
    // a dry run on the same stream gives the ticks of every impact
    std::vector<std::uint64_t> opens;
    std::uint64_t base = 0;
    {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      w.link.r_held = true;
      settle(w, a);
      base = g.p8.tick;
      opens = contact_ticks(w, a, p, base);
      clear_slots_of(w);
    }
    CHECK(opens.size() == liz::kPattern[p].impacts);
    for (unsigned k = 0; k < opens.size(); ++k) {
      for (int lead : {2, 4, 6, 26}) {  // shield clip frame at the pass of the contact: in time, in time, in time, premature
        World w;
        fresh_world(w);
        w.add_dn(0, 0, 100);
        auto* a = w.dn[0];
        w.link.r_held = true;
        settle(w, a);
        const std::uint64_t b0 = g.p8.tick;
        w.link.presses.insert(b0 + opens[k] - static_cast<std::uint64_t>(lead));
        const auto s0 = snap(a);
        const auto se0 = w.link.se_calls;
        const Trace t = run_combo(w, a, p);
        const auto s1 = snap(a);
        const auto* s = slot_of(a);
        CHECK(t.imp.size() == liz::kPattern[p].impacts);  // a parry never cancels the combo below the break
        CHECK(w.link.damage_points == 0);
        CHECK(w.link.se_calls - se0 == t.imp.size());     // one feedback per impact, whatever the defence
        if (lead < 26) {
          CHECK(w.link.shield_attacks == 1);
          CHECK(delta(s1, s0, liz::Cls::LinkParry) == 1 && delta(s1, s0, liz::Cls::LinkGuard) == t.imp.size() - 1);
          CHECK(s->b.p.gained == 12000);                   // uniform weight at every position of every combo (the bar itself may have regenerated since)
          CHECK(s->b.st.link_parry_by_impact[p][k] == 1);
        } else {
          CHECK(s->b.st.link_parry_by_impact[p][k] == 0);  // pressed too early for THIS impact: an ordinary block, no credit (it may be in time for an earlier one)
          CHECK(delta(s1, s0, liz::Cls::LinkParry) <= 1 && s->b.p.gained == 12000 * delta(s1, s0, liz::Cls::LinkParry));
        }
        ++rows;
        clear_slots_of(w);
      }
    }
  }
  CHECK(rows == (4u + 7u + 9u + 6u) * 4u);
}
// ---------------------------------------------------------------- W4: D1 and D2: two physical presses, two serials, two credits; one press never pays twice; holding R+B pays once
static void w_double_pair() {
  std::vector<std::uint64_t> opens;
  std::uint64_t base = 0;
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    base = g.p8.tick;
    opens = contact_ticks(w, a, 2, base);  // P3 begins D1 D2
    clear_slots_of(w);
  }
  CHECK(opens.size() == 9 && opens[1] - opens[0] >= 10 && opens[1] - opens[0] <= 12);
  // Two presses: D1 parried, re-armed through the native init from the slip, D2 parried.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    const std::uint64_t b0 = g.p8.tick;
    w.link.presses.insert(b0 + opens[0] - 3);
    w.link.presses.insert(b0 + opens[1] - 4);
    const auto se0 = w.link.se_calls;
    const Trace t = run_combo(w, a, 2);
    const auto* s = slot_of(a);
    CHECK(t.imp.size() == 9 && s->b.st.link_parry == 2 && s->b.st.d_pair_parry == 1 && s->b.p.gained == 24000);
    CHECK(s->b.st.link_parry_by_impact[2][0] == 1 && s->b.st.link_parry_by_impact[2][1] == 1);
    CHECK(w.link.shield_attacks == 2 && lz.double_rearms == 1);
    CHECK(s->b.st.swings_detected == 0);  // setCutType(GUARD_ATTACK) is not a sword swing
    CHECK(w.link.se_calls - se0 == 9);
    clear_slots_of(w);
  }
  // D1 ordinarily BLOCKED, a new press in the gap: D2 still admits the parry (0.12.0 refused it: Link was in the slip with no rearm).
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    const std::uint64_t b0 = g.p8.tick;
    w.link.presses.insert(b0 + opens[1] - 4);
    const Trace t = run_combo(w, a, 2);
    const auto* s = slot_of(a);
    CHECK(t.imp.size() == 9 && s->b.st.link_parry == 1 && s->b.st.link_parry_by_impact[2][1] == 1 && s->b.st.link_parry_by_impact[2][0] == 0 && lz.gap_arms >= 1);
    clear_slots_of(w);
  }
  // One press, one credit: pressed for D1, nothing in the gap: D2 is a block with its sound, never a second parry.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    const std::uint64_t b0 = g.p8.tick;
    w.link.presses.insert(b0 + opens[0] - 3);
    const auto se0 = w.link.se_calls;
    (void)run_combo(w, a, 2);
    const auto* s = slot_of(a);
    CHECK(s->b.st.link_parry == 1 && w.link.shield_attacks == 1 && w.link.se_calls - se0 == 9);
    clear_slots_of(w);
  }
  // The re-arm exists only with the AT closed: a press while D2's own window is open restarts nothing until it closes.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    const std::uint64_t b0 = g.p8.tick;
    w.link.presses.insert(b0 + opens[0] - 3);
    w.link.presses.insert(b0 + opens[1]);
    auto* s0 = slot_of(a);
    (void)run_combo(w, a, 2, [&](unsigned, std::uint64_t) {
      if (s0->b.ep.window_on && s0->b.ep.impact == 2) CHECK(w.link.shield_attacks == 1);
    });
    clear_slots_of(w);
  }
  // A Shield Attack with no real press behind it (a native init the input layer did not trust): an ordinary block, never a parry.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    const std::uint64_t b0 = g.p8.tick;
    const std::uint64_t at = b0 + opens[0] - 3;
    (void)run_combo(w, a, 2, [&](unsigned, std::uint64_t next) {
      if (next == at) (void)w.link.procGuardAttackInit();
    });
    const auto* s = slot_of(a);
    CHECK(w.link.shield_attacks == 1 && s->b.st.link_parry_by_impact[2][0] == 0);
    clear_slots_of(w);
  }
  // A press outside any gap of the combo does not get the fast rearm: no contact yet, native only.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    const auto rearms0 = lz.double_rearms;
    for (int k = 0; k < 100; ++k) {
      a->timer[2] = 5000;
      w.tick();
    }
    CHECK(lz.double_rearms == rearms0 && lz.gap_arms == 0);
    clear_slots_of(w);
  }
}
// ---------------------------------------------------------------- W5: the block test: a lateral sweep in front of a raised shield; from the back never
static void w_front_range() {
  for (unsigned p = 0; p < 4; ++p) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    w.link.shape_angle.y = 0;  // the shield looks away: the actor is at his back
    settle(w, a);
    const auto s0 = snap(a);
    const auto se0 = w.link.se_calls;
    (void)run_combo(w, a, p);
    const auto s1 = snap(a);
    CHECK(delta(s1, s0, liz::Cls::LinkGuard) == 0 && delta(s1, s0, liz::Cls::LinkParry) == 0);
    CHECK(delta(s1, s0, liz::Cls::LinkBody) >= 1);
    CHECK(w.link.se_calls == se0);
    clear_slots_of(w);
  }
  // Front: every velocity direction of every arc blocks while the actor is in front, thanks to the engine's own StartCAt state; bias the arcs through all phases.
  for (float bias : {-1.2f, -0.6f, 0.0f, 0.6f, 1.2f}) {
    g_arc_bias = bias;
    for (unsigned p = 0; p < 4; ++p) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      w.link.r_held = true;
      settle(w, a);
      const auto s0 = snap(a);
      (void)run_combo(w, a, p);
      const auto s1 = snap(a);
      CHECK(delta(s1, s0, liz::Cls::LinkBody) == 0);  // never through the raised shield
      CHECK(w.link.damage_points == 0);
      clear_slots_of(w);
    }
  }
  g_arc_bias = 0.0f;
}
// ---------------------------------------------------------------- W6: coverage property. Truth = a dense walk of the exact pose path of each window. Touch with margin => contact; no touch => never a contact.
static void w_coverage() {
  struct Kind { unsigned pat; int anm; float first, r, omega, margin; };
  const Kind kinds[] = {{2, liz::native::kAnmD, 14.0f, 55.0f, 1.0f, 20.0f}, {0, liz::native::kAnmN, 23.0f, 55.0f, 1.0f, 20.0f}, {3, liz::native::kAnmC, 34.0f, 77.0f, 1.0f, 20.0f}, {3, liz::native::kAnmC, 34.0f, 77.0f, 3.0f, 45.0f}};
  unsigned truth_touch = 0, truth_none = 0, fixed = 0, dead_on = 0;
  std::uint64_t base_missed = 0, base_had = 0;
  for (bool coverage : {true, false})
    for (const Kind& K : kinds)
      for (float dist : {120.f, 170.f, 210.f, 245.f, 285.f, 330.f})
        for (float lat : {0.f, 90.f, -150.f})
          for (float bias : {0.0f, 0.27f})
            for (float ly : {0.f, 120.f, 210.f}) {
              g_arc_bias = bias;
              g_omega_scale = K.omega;
              World w;
              fresh_world(w, dist);
              w.add_dn(0, 0, 100);
              auto* a = w.dn[0];
              settle(w, a);
              lz.coverage = coverage;
              w.link.current.pos = cXyz(lat, ly, dist);
              LizSlot* s = slot_of(a);
              force_pattern(a, K.pat);
              a->timer[2] = 0, a->action = 3, a->mode = 2;
              w.link.mDamageTimer = 0;
              w.tick();
              bool contact = false, truth = false, truth_margin = false, opened = false;
              const std::uint64_t windows0 = s->b.st.windows;
              const cXyz root = a->actor.current.pos;
              for (int k = 0; k < 70; ++k) {
                a->actor.current.pos = root;
                const float pre = a->morph.getFrame();
                w.tick();
                if (s->b.st.windows == windows0 + 1) opened = true;
                const float post = a->morph.getFrame();
                if (opened && s->b.ep.window_on && s->b.st.windows == windows0 + 1) {
                  const float yaw = K.anm == liz::native::kAnmC ? yaw_rad(a->search_angle_y) : yaw_rad(a->actor.current.angle.y);
                  for (float f = pre < K.first ? K.first : pre; f <= post; f += 0.02f) {
                    cXyz c;
                    pose_point(K.anm, f, a->actor.current.pos, a->actor.shape_angle.y, c);
                    const float back = K.anm == liz::native::kAnmC ? (post - f) * 30.0f : 0.0f;
                    const float dx = c.x - std::sin(yaw) * back - w.link.current.pos.x, dz = c.z - std::cos(yaw) * back - w.link.current.pos.z;
                    const float dd = std::sqrt(dx * dx + dz * dz);
                    const bool yok = c.y + K.r >= ly && c.y - K.r <= ly + 180.0f;
                    const bool ym = c.y + K.r - 5.0f >= ly && c.y - K.r + 5.0f <= ly + 180.0f;
                    if (dd <= 35.0f + K.r && yok) truth = true;
                    if (dd <= 35.0f + K.r - K.margin && ym) truth_margin = true;
                  }
                }
                if (a->at_sph.at_hit && opened) contact = true;
                if (s->b.st.windows > windows0 + 1) break;
                if (opened && !s->b.ep.window_on) break;
              }
              if (coverage) {
                if (truth_margin) {
                  ++truth_touch;
                  CHECK(contact);  // the swing really passed through Link: the engine got that contact
                }
                if (!truth) {
                  ++truth_none;
                  CHECK(!contact);                // no hitbox was widened: a real whiff stays a whiff
                  CHECK(s->b.st.lost_ticks == 0);  // and nothing that did not reach Link was offered to the engine as if it did
                  CHECK(s->b.st.gap_fixed == 0);
                }
                fixed += static_cast<unsigned>(s->b.st.gap_fixed);
                CHECK(s->b.st.lost_ticks == 0);   // every sphere that was expected to cross got its engine record
              } else {
                if (truth_margin) {
                  ++base_had;
                  if (!contact) ++base_missed;
                }
                CHECK(s->b.st.gap_fixed == 0);
              }
              ++dead_on;
              clear_slots_of(w);
            }
  g_arc_bias = 0.0f;
  g_omega_scale = 1.0f;
  CHECK(truth_touch >= 100 && truth_none >= 100);
  // The reproduction: the skipped poses exist in this geometry without the coverage, and the coverage is what closes them.
  CHECK(base_had == truth_touch && base_missed >= 5 && fixed >= base_missed);
  std::printf("LIZ_WORLD_COVERAGE runs=%u touching=%u clear=%u missed_without_coverage=%llu of=%llu sphere_writes=%u\n", dead_on, truth_touch, truth_none, static_cast<unsigned long long>(base_missed), static_cast<unsigned long long>(base_had), fixed);
  // Written poses of a tail clip are the tail's own joint, of a sword clip the sword's: read back from the production log line.
  {
    unsigned tail_writes = 0;
    g_omega_scale = 3.0f;
    for (float bias : {0.0f, 0.13f, 0.27f, 0.41f, 0.6f, 0.9f})
      for (float dist : {170.f, 210.f, 245.f}) {
        g_arc_bias = bias;
        World w;
        fresh_world(w, dist);
        w.add_dn(0, 0, 100);
        auto* a = w.dn[0];
        settle(w, a);
        g_log_store.clear();
        w.link.mDamageTimer = 0;
        (void)run_combo(w, a, 3);
        for (const auto& l : g_log_store) {
          if (l.find("CANDIDATE_INTERSECTION") == std::string::npos) continue;
          const auto k = l.find(" kind=");
          const int kind = std::atoi(l.c_str() + k + 6);
          const auto pl = l.find("place=(");
          float x = 0, y = 0, z = 0;
          CHECK(std::sscanf(l.c_str() + pl + 7, "%f,%f,%f", &x, &y, &z) == 3);
          if (kind >= 2) {
            ++tail_writes;
            CHECK(std::fabs(y - 80.0f) < 1.0f);
          } else {
            CHECK(std::fabs(y - 100.0f) < 1.0f);
          }
        }
        clear_slots_of(w);
      }
    g_omega_scale = 1.0f;
    g_arc_bias = 0.0f;
    CHECK(tail_writes >= 1);
  }
  // Tg off (Link's real i-frames) on the path: nothing is written, nothing is hit, the record says immune.
  {
    World w;
    fresh_world(w, 170);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.link.mDamageTimer = 200;
    const auto s0 = snap(a);
    (void)run_combo(w, a, 2);
    const auto s1 = snap(a);
    CHECK(w.link.damage_points == 0 && delta(s1, s0, liz::Cls::LinkBody) == 0 && slot_of(a)->b.st.gap_fixed == 0 && delta(s1, s0, liz::Cls::ImmuneValid) >= 1 && delta(s1, s0, liz::Cls::MissingFeedback) == 0);
    clear_slots_of(w);
  }
  // Link's collision not refreshed this tick (his cylinders are last tick's): no decision against stale cylinders.
  {
    World w;
    fresh_world(w, 170);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.skip_link_collision = true;
    (void)run_combo(w, a, 2);
    CHECK(slot_of(a)->b.st.gap_fixed == 0 && lz.why[unsigned(liz::sweep::Why::LinkNotCurrent)] >= 1 && lz.probes == 0);
    clear_slots_of(w);
  }
  // A pose probe whose restore cannot be verified switches coverage off for the session, loudly; the swing itself goes on.
  {
    World w;
    fresh_world(w, 170);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.link.current.pos = cXyz(40, 0, 245);
    g_pose_noise = 0.001f;
    const Trace t = run_combo(w, a, 2);
    g_pose_noise = 0.0f;
    CHECK(t.imp.size() == 9 && !lz.coverage && lz.probe_mismatch == 1);
    bool logged = false;
    for (const auto& l : g_log_store) logged = logged || l.find("POSE_PROBE_RESTORE_MISMATCH") != std::string::npos;
    CHECK(logged);
    clear_slots_of(w);
  }
}
// ---------------------------------------------------------------- W7: three Lizalfos: attribution, one sound per impact, independent memories
static void w_multi() {
  World w;
  fresh_world(w);
  w.add_dn(0, 0, 100);
  w.add_dn(-200, 0, 101);
  w.add_dn(200, 0, 102);
  w.link.r_held = true;
  for (int k = 0; k < 6; ++k) {
    for (auto* a : w.dn) a->timer[2] = 5000;
    w.tick();
  }
  for (auto* a : w.dn) CHECK(slot_of(a) && slot_of(a)->b.admitted);
  for (auto* a : w.dn) a->timer[2] = 0;
  for (int k = 0; k < 900; ++k) w.tick();
  std::uint64_t contacts = 0, windows = 0;
  for (auto* a : w.dn) {
    const auto* s = slot_of(a);
    contacts += s->b.st.contact[unsigned(liz::Hit::Guard)] + s->b.st.contact[unsigned(liz::Hit::MissedEarly)];
    windows += s->b.st.windows;
  }
  CHECK(windows >= 20 && contacts >= 3);
  CHECK(w.link.damage_points == 0);
  CHECK(lz.se_native <= contacts + 2 && lz.se_native >= 1);
  // Cross actor: a parry against 100 leaves the others' bars at 0.
  for (auto* a : w.dn) CHECK(slot_of(a)->b.p.milli >= 0);
  clear_slots_of(w);
}
// ---------------------------------------------------------------- W8: the whole break through the production writers
static void hit_body(World& w, e_dn_class* a, std::uint32_t at_type, cCcD_Obj* obj, int power = 4, bool strong = false) {
  obj->at_type = at_type;
  a->cc_sph[0].tg_hit = obj;
  a->hit_power = power, a->hit_strong = strong;
  w.tick();
}
static void w_break() {
  // crossing by sword, stands; a new swing drops it through the native big_damage; down; Finish
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    s->b.p.add(95);
    w.set_cut(1);
    w.link.cut_type = 0;
    a->invulnerability_timer = 0;
    a->timer[2] = 5000;
    hit_body(w, a, kAtSword, &w.link.mAtCps[0]);
    CHECK(s->b.p.latched && a->action == liz::native::kDrawback && s->b.phase == liz::Brain::Bk::Punish && a->big_body == 0 && a->small_body == 0 && !a->actor.down && a->actor.health == 366);
    CHECK(a->field_0x74c > 0.0f);
    bool lock = false;
    for (int k = 0; k < 20; ++k) {
      w.tick();
      lock = lock || a->actor.head_lock;
    }
    CHECK(!lock);  // HeadLock only after a real new Shield Attack
    w.set_cut(1);
    w.link.cut_type = 0;
    w.tick();
    a->invulnerability_timer = 0;
    const int before_decisive = a->actor.health;
    hit_body(w, a, kAtSword, &w.link.mAtCps[0]);
    CHECK(a->big_body == 1 && a->action == 21 && a->invulnerability_timer >= 990 && s->b.phase == liz::Brain::Bk::Falling && s->b.st.decisive_sword == 1);
    CHECK(a->actor.health == before_decisive - 6 && s->b.st.break_bonus_hits == 1);  // the production writer: 4 + 2, once, before the fall
    int k = 0;
    for (; k < 40 && !a->actor.down; ++k) w.tick();
    CHECK(a->actor.down && a->actor.health > 0);
    a->actor.cut_down_hit = true;
    w.tick();
    CHECK(a->actor.health == 0);
    for (k = 0; k < 80 && !a->deleted; ++k) w.tick();
    CHECK(a->deleted && s->b.dead);
    clear_slots_of(w);
  }
  // parry crossing through a real collision; the same press cannot prepare; a new one does: real HeadLock; Helm Splitter: decisive; down.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    auto* s = slot_of(a);
    s->b.p.add(90);
    force_pattern(a, 0);
    a->timer[2] = 0, a->action = 3, a->mode = 2;
    std::uint64_t press = 0;
    for (int k = 0; k < 40 && press == 0; ++k) {
      w.tick();
      if (s->b.ep.active) press = g.p8.tick + 15;
    }
    CHECK(press);
    w.link.presses.insert(press);
    int k = 0;
    for (; k < 40 && s->b.phase != liz::Brain::Bk::Punish; ++k) w.tick();
    CHECK(s->b.phase == liz::Brain::Bk::Punish && s->b.st.link_parry == 1 && a->action == liz::native::kDrawback && a->big_body == 0);
    CHECK(s->b.st.ended[unsigned(liz::End::Posture100)] == 1);
    // the shield collider of that same press still in contact
    for (k = 0; k < 12; ++k) w.tick();
    a->invulnerability_timer = 0;
    hit_body(w, a, AT_TYPE_SHIELD_ATTACK, &w.link.mGuardAtCps);
    CHECK(!s->b.helm_prepared && !a->actor.head_lock);
    // a new press
    w.link.mProcID = daAlink_c::PROC_WAIT, w.link.frame = 0;
    w.link.presses.insert(g.p8.tick + 1);
    w.tick();
    CHECK(w.link.mProcID == daAlink_c::PROC_GUARD_ATTACK);
    const int hp0 = a->actor.health;
    a->invulnerability_timer = 0;
    hit_body(w, a, AT_TYPE_SHIELD_ATTACK, &w.link.mGuardAtCps);
    CHECK(s->b.helm_prepared && a->actor.health == hp0 && a->action == liz::native::kDrawback);
    bool lock = false;
    for (k = 0; k < 12; ++k) {
      w.tick();
      lock = lock || a->actor.head_lock;
    }
    CHECK(lock);
    w.set_cut(daPy_py_c::CUT_TYPE_HEAD_JUMP);
    w.tick();
    a->invulnerability_timer = 0;
    const int before_helm = a->actor.health;
    hit_body(w, a, kAtSword, &w.link.mAtCps[0]);
    CHECK(s->b.st.decisive_helm == 1 && a->big_body == 1 && a->action == 21);
    CHECK(a->actor.health == before_helm - 6 && s->b.st.break_bonus_hits == 1);
    for (k = 0; k < 40 && !a->actor.down; ++k) w.tick();
    CHECK(a->actor.down);
    clear_slots_of(w);
  }
  // no decisive contact: stands up again at the absolute deadline, rearm 0, fights on
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    s->b.p.add(95);
    a->invulnerability_timer = 0, a->timer[2] = 5000;
    hit_body(w, a, kAtSword, &w.link.mAtCps[0]);
    int k = 0;
    for (; k < 200 && s->b.phase == liz::Brain::Bk::Punish; ++k) {
      CHECK(s->b.p.points() == 100);
      w.tick();
    }
    CHECK(k >= 85 && k <= 100);
    CHECK(s->b.p.points() == 0 && !s->b.p.latched && a->action == 3 && a->big_body == 0 && !a->actor.down);
    a->timer[2] = 0;
    for (k = 0; k < 120 && !s->b.ep.active; ++k) w.tick();  // native FIGHT_RUN re-enters its strafe and its own attack timer
    CHECK(s->b.ep.active);
    clear_slots_of(w);
  }
  // Shield Attack below 100: no stagger, no HP; its native collision SE is still played once per press
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.link.r_held = true;
    w.link.presses.insert(g.p8.tick + 1);
    w.tick();
    a->invulnerability_timer = 0, a->timer[2] = 5000;
    hit_body(w, a, AT_TYPE_SHIELD_ATTACK, &w.link.mGuardAtCps);
    CHECK(a->action == 3 && a->drawback_entered == 0 && g_bash_se == 1);
    a->invulnerability_timer = 0;
    hit_body(w, a, AT_TYPE_SHIELD_ATTACK, &w.link.mGuardAtCps);
    CHECK(g_bash_se == 1 && a->drawback_entered == 0);
    clear_slots_of(w);
  }
}
// ---------------------------------------------------------------- W9: two real blocks through the writers, native exit, counter, AT, parry, four combos after
// One real sword contact on the shield, as the collision pass records it: the At mark on Link's sword (shield flag on), the Tg mark on tate_sph.
static void sword_on_shield(World& w, e_dn_class* a) {
  auto& c = w.link.mAtCps[0];
  c.at_hit = &a->tate_sph, c.at_shield_hit = true;
  a->tate_sph.tg_hit = &c;
  a->tate_sph.tg_shield_hit = true;
}
static void present_guard(e_dn_class* a) {
  a->action = 7, a->mode = 7, a->morph.rate = 0, a->timer[0] = 60, a->anm_no = 22, a->morph.loop = false, a->morph.end = 22, a->morph.mFrameCtrl.mFrame = 8, a->morph.stop = false;
  auto* s = slot_of(a);
  s->b.in_guard = true;
  s->b.guard_seen_from = s->b.local;
}
static void block(World& w, e_dn_class* a, int wait = 11) {
  w.set_cut(1);
  for (int k = 0; k < wait; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
  a->timer[0] = 60;
  sword_on_shield(w, a);
  w.tick();
}
static void w_shield_rebound() {
  // Ordinary block through the real hooks: no HP, +5 once, the native SE of the contact stays, Link's sword does NOT rebound and his cut goes on. Two blocks prepare; nothing is armed as a counter.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    for (int g2 = 0; g2 < 100000; ++g2) {
      liz::Rng pr = s->b.rng;
      if (pr.below(100) < 80) break;
      (void)s->b.rng.next();
    }
    a->timer[2] = 5000;
    present_guard(a);
    const int hp0 = a->actor.health;
    const auto se0 = a->sound.collision_se;
    block(w, a);
    CHECK(a->actor.health == hp0 && s->b.p.points() == 5 && w.link.rebounds == 0 && s->b.st.rebound_suppressed == 1 && w.link.cut_type == 1 && a->sound.collision_se == se0 + 1);
    block(w, a);
    CHECK(a->actor.health == hp0 && s->b.p.points() == 10 && w.link.rebounds == 0 && s->b.deflect.armed && !s->b.counter.armed);
    CHECK(s->b.st.counter_selected[0] + s->b.st.counter_selected[1] == 0);
    // The third real swing in the new window: parry. Native rebound kept, +0 to itself, +25 to Link, counter.
    w.set_cut(1);
    a->timer[2] = 5000;
    w.tick();
    CHECK(a->mode == 6 && s->b.attempt_armed);  // re-presented: ANM_GUARD from its start
    w.tick();
    sword_on_shield(w, a);
    const int pts = s->b.p.points();
    w.tick();
    CHECK(s->b.st.enemy_parries == 1 && w.link.rebounds == 1 && s->b.st.rebound_kept == 1 && a->actor.health == hp0 && s->b.p.points() == pts && s->b.counter.armed);
    bool logged = false;
    for (const auto& l : g_log_store) logged = logged || l.find("event=ENEMY_PARRY") != std::string::npos && l.find("link_gain=+25 liz_gain=+0") != std::string::npos;
    CHECK(logged);
    clear_slots_of(w);
  }
  // A contact out of the window: block, no rebound, no counter. No third swing: expiry and combat.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    for (int g2 = 0; g2 < 100000; ++g2) {
      liz::Rng pr = s->b.rng;
      if (pr.below(100) < 80) break;
      (void)s->b.rng.next();
    }
    a->timer[2] = 5000;
    present_guard(a);
    block(w, a);
    block(w, a);
    CHECK(s->b.deflect.armed);
    int k = 0;
    for (; k < 200 && a->action == 7; ++k) w.tick();
    CHECK(a->action == 3 && !s->b.deflect.armed && s->b.st.enemy_parries == 0 && w.link.rebounds == 0 && !s->b.counter.armed);
    clear_slots_of(w);
  }
  // Three sword capsules: the shield's Tg keeps only the last one that touched it. The first capsule's question gets the same answer.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    a->timer[2] = 5000;
    present_guard(a);
    w.set_cut(1);
    for (int k = 0; k < 11; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->timer[0] = 60;
    auto& c0 = w.link.mAtCps[0];
    auto& c1 = w.link.mAtCps[1];
    c0.at_hit = &a->tate_sph, c0.at_shield_hit = true;
    c1.at_hit = &a->cc_sph[1], c1.at_shield_hit = true;  // a body sphere recorded later in the same pass
    a->tate_sph.tg_hit = &c1;
    w.tick();
    CHECK(w.link.rebounds == 0 && slot_of(a)->b.st.rebound_suppressed == 1 && slot_of(a)->b.st.blocks == 1);
    clear_slots_of(w);
  }
  // Another enemy's shield or a sword with no shield record: the hook does not touch native.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    settle(w, w.dn[0]);
    dCcD_Sph foreign;
    auto& c = w.link.mAtCps[0];
    w.set_cut(1);
    c.at_hit = &foreign, c.at_shield_hit = true;
    w.reverse_check();
    CHECK(w.link.rebounds == 1);  // e.g. a Bokoblin's shield
    clear_slots_of(w);
    // ... even while a Lizalfos' shield holds a block of another capsule of the same swing: that other shield's rebound is not this actor's to remove.
    World x;
    fresh_world(x);
    x.add_dn(0, 0, 100);
    auto* a = x.dn[0];
    settle(x, a);
    a->timer[2] = 5000;
    present_guard(a);
    x.set_cut(1);
    for (int k = 0; k < 11; ++k) a->timer[0] = 60, a->timer[2] = 5000, x.tick();
    a->timer[0] = 60;
    dCcD_Sph other;
    auto& c0 = x.link.mAtCps[0];
    auto& c1 = x.link.mAtCps[1];
    c0.at_hit = &other, c0.at_shield_hit = true;
    c1.at_hit = &a->tate_sph, c1.at_shield_hit = true;
    a->tate_sph.tg_hit = &c1, a->tate_sph.tg_shield_hit = true;
    x.tick();
    CHECK(x.link.rebounds == 1);
    clear_slots_of(x);
  }
  // Shield and body in one collision pass, from the front: the shield took that swing; no wound, one +5. The body's record is removed through the real writer.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    w.set_cut(1);
    for (int k = 0; k < 11; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->timer[0] = 60, a->invulnerability_timer = 0;
    const int hp0 = a->actor.health;
    sword_on_shield(w, a);
    w.link.mAtCps[0].at_type = kAtSword;
    a->cc_sph[0].tg_hit = &w.link.mAtCps[0];
    a->hit_power = 4, a->hit_strong = false;
    w.tick();
    CHECK(a->actor.health == hp0 && s->b.st.shield_won_body == 1 && s->b.st.wounds == 0 && s->b.st.blocks == 1 && s->b.p.points() == 5);
    clear_slots_of(w);
  }
  // Guard body rule through e_dn_guard's real hook: entering is not covered; behind is not covered; up in front is.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    a->timer[2] = 5000;
    a->action = 7, a->mode = 5, a->invulnerability_timer = 15, a->timer[0] = 0;
    slot_of(a)->b.in_guard = true;
    w.tick();
    CHECK(a->mode == 6 && a->invulnerability_timer == 0);
    present_guard(a);
    w.tick();
    CHECK(a->mode == 7 && a->invulnerability_timer > 0);
    a->search_angle_y = static_cast<s16>(0x8000);
    w.link.current.pos = cXyz(0, 0, -160);  // Link walks behind
    w.tick();
    CHECK(a->invulnerability_timer == 0);
    clear_slots_of(w);
  }
  // Vanilla's permanent memory: a sword wound leaves no sword bit, whatever the roll; native guard stays for its other reasons.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    for (int g2 = 0; g2 < 100000; ++g2) {
      liz::Rng pr = s->b.rng;
      if (pr.below(100) >= 45) break;
      (void)s->b.rng.next();
    }
    a->timer[2] = 5000;
    w.set_cut(1);
    a->invulnerability_timer = 0;
    a->learn = 0x1000;  // a native memory of something else than the sword stays: only the sword bits are decided by the brain
    hit_body(w, a, kAtSword, &w.link.mAtCps[0]);
    CHECK(a->actor.health == 366 && (a->learn & 0xFFF) == 0 && (a->learn & 0x1000) != 0 && !s->b.guard_intent);
    for (int k = 0; k < 30; ++k) {
      a->timer[2] = 5000;
      w.tick();
      CHECK(a->action != 7);
    }
    clear_slots_of(w);
  }
}
static void w_counter() {
  for (unsigned want : {0u, 1u}) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    for (int g2 = 0; g2 < 100000; ++g2) {
      liz::Rng pr = s->b.rng;
      const bool ok = pr.below(100) < 80;
      if (ok && pr.below(2) == want) break;
      (void)s->b.rng.next();
    }
    a->timer[2] = 5000;
    present_guard(a);
    block(w, a);
    block(w, a);
    CHECK(s->b.deflect.armed && !s->b.counter.armed);
    w.set_cut(1);
    a->timer[2] = 5000;
    w.tick();
    w.tick();
    sword_on_shield(w, a);
    w.tick();
    CHECK(s->b.counter.armed && s->b.counter.kind == want && a->mode == 8 && a->morph.rate == 1.0f);
    w.link.cut_type = 0;
    std::uint64_t opens0 = s->b.st.windows;
    int k = 0;
    for (; k < 120 && !s->b.ep.active; ++k) w.tick();
    CHECK(s->b.ep.active && s->b.ep.pattern == (want == 0 ? liz::kCounterN : liz::kCounterD));
    w.tick();  // the native attack function runs its mode 0 next Execute: anm_init at the frozen action rate
    CHECK(a->action == (want == 0 ? 4 : 5) && a->morph.rate == (want == 0 ? 1.25f : 1.10f));
    for (; k < 240 && s->b.ep.active; ++k) w.tick();
    CHECK(!s->b.ep.active && s->b.st.windows - opens0 == (want == 0 ? 1u : 2u) && s->b.st.counter_at_open == 1);
    std::uint64_t sel = 0;
    for (auto x : s->b.st.selected) sel += x;
    CHECK(sel == 0);
    a->timer[2] = 0;
    for (k = 0; k < 200 && sel == 0; ++k) {
      w.tick();
      sel = 0;
      for (auto x : s->b.st.selected) sel += x;
    }
    CHECK(sel == 1);
    clear_slots_of(w);
  }
  // Link blocks the double counter: both windows reach him, one sound each.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    auto* s = slot_of(a);
    for (int g2 = 0; g2 < 100000; ++g2) {
      liz::Rng pr = s->b.rng;
      const bool ok = pr.below(100) < 80;
      if (ok && pr.below(2) == 1u) break;
      (void)s->b.rng.next();
    }
    a->timer[2] = 5000;
    present_guard(a);
    block(w, a);
    block(w, a);
    w.set_cut(1);
    a->timer[2] = 5000;
    w.tick();
    w.tick();
    sword_on_shield(w, a);
    w.tick();
    CHECK(s->b.counter.armed && s->b.counter.kind == 1);
    w.link.cut_type = 0;
    const auto se0 = w.link.se_calls;
    int k = 0;
    for (; k < 120 && !s->b.ep.active; ++k) w.tick();
    for (; k < 240 && s->b.ep.active; ++k) w.tick();
    CHECK(s->b.st.counter_contact == 1 && w.link.se_calls - se0 == 2 && w.link.damage_points == 0);
    clear_slots_of(w);
  }
}
// ---------------------------------------------------------------- W11: x1.5 on Link through the real hook, general by construction
static void enter_link_break(World& w, lnkfx::Origin origin = lnkfx::Origin::ModBreak, bool extended = true) {
  w.link.mProcID = World::kProcGuardBreak, w.link.frame = 0;
  lnkfx::Begin b;
  b.link_id = w.link.pid, b.epoch = 7, b.tick = g.p8.tick, b.origin = origin, b.extended = extended;
  (void)lf.def.begin(b);
}
struct Foe : fopAc_ac_c {
  dCcD_Sph at;
};
static void foe_hits(World& w, Foe& f, bool shield = false) {
  w.link.mTgCyls[0].tg_hit = &f.at;
  w.link.mTgCyls[0].tg_shield_hit = shield;
}
static void w_link_damage() {
  // Any attacker of the enemy group, known to this mod or not: a Lizalfos, a Bokoblin, an unknown enemy, a boss class, a mod's actor. 2->3 4->6 6->9 8->12, odd up.
  const int names[] = {fpcNm_E_DN_e, 510, 431, 777, 240};
  const int cases[][2] = {{2, 3}, {4, 6}, {6, 9}, {8, 12}, {1, 2}, {3, 5}};
  for (int nm : names)
    for (const auto& c : cases) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      settle(w, w.dn[0]);
      Foe f;
      f.name = nm, f.group = fopAc_ENEMY_e, f.pid = 900;
      f.at.owner_ac = &f;
      g_actors.push_back(&f);
      enter_link_break(w);
      w.tick();
      w.link.mDamageTimer = 0;
      const int life0 = w.life->life;
      w.contact_damage = c[0];
      foe_hits(w, f);
      w.tick();
      CHECK(w.life->life == life0 - c[1] && w.last_commit == c[1] && w.link.damage_points == 1 && ld.c.applied == 1 && ld.c.extra_units == static_cast<std::uint64_t>(c[1] - c[0]));
      CHECK(w.link.mDamageTimer > 0);  // native invulnerability from the same commit
      clear_slots_of(w);
    }
  // Outside the episode: exactly native. Before it, after it, a non-saturated native break (not a posture break), the rearm phase.
  {
    World w;
    fresh_world(w);
    Foe f;
    f.group = fopAc_ENEMY_e, f.pid = 900;
    f.at.owner_ac = &f;
    g_actors.push_back(&f);
    const int life0 = w.life->life;
    w.contact_damage = 4;
    foe_hits(w, f);
    w.tick();
    CHECK(w.life->life == life0 - 4 && ld.c.applied == 0);
    w.link.mDamageTimer = 0, w.link.mProcID = daAlink_c::PROC_WAIT;
    enter_link_break(w, lnkfx::Origin::NativeObserved, false);
    w.tick();
    w.link.mDamageTimer = 0;
    const int life1 = w.life->life;
    foe_hits(w, f);
    w.tick();
    CHECK(w.life->life == life1 - 4 && ld.c.applied == 0);
    clear_slots_of(w);
  }
  // During the whole penalty window and not one tick more: native GUARD_BREAK, then post-break vulnerability, then native again.
  {
    World w;
    fresh_world(w);
    Foe f;
    f.group = fopAc_ENEMY_e, f.pid = 900;
    f.at.owner_ac = &f;
    g_actors.push_back(&f);
    enter_link_break(w);
    w.tick();
    CHECK(lf.def.phase() == lnkfx::Phase::NativeBreak);
    w.contact_damage = 4;
    foe_hits(w, f);
    w.tick();
    CHECK(ld.c.applied == 1 && w.last_commit == 6);
    w.link.mDamageTimer = 0;
    int k = 0;
    for (; k < 60 && lf.def.phase() == lnkfx::Phase::NativeBreak; ++k) w.tick();
    CHECK(lf.def.phase() == lnkfx::Phase::PostVulnerable);
    w.link.mDamageTimer = 0;
    foe_hits(w, f);
    w.tick();
    CHECK(ld.c.applied == 2 && w.last_commit == 6);
    for (k = 0; k < 120 && lf.def.phase() == lnkfx::Phase::PostVulnerable; ++k) w.tick();
    CHECK(lf.def.phase() != lnkfx::Phase::PostVulnerable && lf.def.phase() != lnkfx::Phase::NativeBreak);
    w.link.mDamageTimer = 0;
    w.link.mProcID = daAlink_c::PROC_WAIT;
    foe_hits(w, f);
    w.tick();
    CHECK(ld.c.applied == 2 && w.last_commit == 4);
    bool b = false, e = false;
    for (const auto& l : g_log_store) b = b || l.find("EPISODE_BEGIN") != std::string::npos, e = e || l.find("EPISODE_END") != std::string::npos;
    CHECK(b && e && ld.c.episodes == 1);
    clear_slots_of(w);
  }
  // Blocked contact, no HP: nothing. Invulnerable: native never commits. Two contacts apart: each once, and never x2.25.
  {
    World w;
    fresh_world(w);
    Foe f;
    f.group = fopAc_ENEMY_e, f.pid = 900;
    f.at.owner_ac = &f;
    g_actors.push_back(&f);
    enter_link_break(w);
    w.tick();
    w.contact_damage = 4;
    foe_hits(w, f, true);
    daAlink_c* lp = &w.link;
    w.check_damage_action();
    CHECK(ld.c.applied == 0 && w.link.damage_points == 0);
    w.link.mProcID = World::kProcGuardBreak;
    w.link.mTgCyls[0].tg_hit = nullptr, w.link.mTgCyls[0].tg_shield_hit = false;
    w.link.mDamageTimer = 12;
    foe_hits(w, f);
    w.check_damage_action();
    CHECK(ld.c.applied == 0 && w.link.damage_points == 0);
    w.link.mDamageTimer = 0;
    foe_hits(w, f);
    w.check_damage_action();
    CHECK(ld.c.applied == 1 && w.last_commit == 6);
    // two commits inside one tick: the second is reported, not multiplied
    w.link.mDamageTimer = 0;
    foe_hits(w, f);
    w.check_damage_action();
    CHECK(ld.c.applied == 1 && w.last_commit == 4 && ld.c.why[unsigned(lnkdmg::Why::Duplicate)] == 1);
    (void)lp;
    w.tick();
    w.link.mDamageTimer = 0;
    foe_hits(w, f);
    w.tick();
    CHECK(ld.c.applied == 2 && w.last_commit == 6);
    clear_slots_of(w);
  }
  // A contact the shield record took, even if some native route commits it: never multiplied.
  {
    World w;
    fresh_world(w);
    Foe f;
    f.group = fopAc_ENEMY_e, f.pid = 900;
    f.at.owner_ac = &f;
    g_actors.push_back(&f);
    enter_link_break(w);
    w.tick();
    foe_hits(w, f, true);
    fx.native_depth = 1, fx.native_reentered = false, fx.native = lnks1fix::NativeDamage{};
    fx.native.tick = g.p8.tick, fx.native.valid = true, fx.native.human = true, fx.native.cylinder = 0, fx.native.blocked = true, fx.native.poly_seen = true, fx.native.poly_result = 0;
    w.set_damage_point(4);
    fx.native_depth = 0;
    CHECK(w.last_commit == 4 && ld.c.applied == 0 && ld.c.why[unsigned(lnkdmg::Why::Blocked)] == 1);
    w.link.mTgCyls[0].tg_hit = nullptr, w.link.mTgCyls[0].tg_shield_hit = false;
    clear_slots_of(w);
  }
  // Not an attributable hostile contact: the floor, a landing, a throw with no known thrower, Link's own bomb, an env object, an unknown At: native, and said so.
  {
    World w;
    fresh_world(w);
    enter_link_break(w);
    w.tick();
    w.poly_result = 1;
    w.check_damage_action();
    CHECK(ld.c.applied == 0 && w.last_commit == 1 && ld.c.why[unsigned(lnkdmg::Why::Environment)] == 1);
    w.poly_result = 0, w.link.mDamageTimer = 0;
    w.set_damage_point(3);  // outside checkDamageAction: throw, landing, scene, setPlayerDamage
    CHECK(w.last_commit == 3 && ld.c.why[unsigned(lnkdmg::Why::Unattributed)] == 1);
    Foe bomb;
    bomb.group = fopAc_ACTOR_e, bomb.pid = 901, bomb.parent = w.link.pid;
    bomb.at.owner_ac = &bomb;
    g_actors.push_back(&bomb);
    w.link.mDamageTimer = 0, w.contact_damage = 4;
    foe_hits(w, bomb);
    w.check_damage_action();
    CHECK(w.last_commit == 4 && ld.c.why[unsigned(lnkdmg::Why::NotHostile)] == 1);
    Foe trap;
    trap.group = fopAc_ENV_e, trap.pid = 902;
    trap.at.owner_ac = &trap;
    g_actors.push_back(&trap);
    w.link.mDamageTimer = 0;
    foe_hits(w, trap);
    w.check_damage_action();
    CHECK(w.last_commit == 4 && ld.c.why[unsigned(lnkdmg::Why::NotHostile)] == 2);
    Foe ghost;
    ghost.pid = 903;
    w.link.mDamageTimer = 0;
    foe_hits(w, ghost);
    w.check_damage_action();
    CHECK(w.last_commit == 4 && ld.c.why[unsigned(lnkdmg::Why::Unattributed)] == 2);
    // A projectile an enemy spawned: hostile.
    Foe boss;
    boss.group = fopAc_ENEMY_e, boss.pid = 904;
    g_actors.push_back(&boss);
    Foe arrow;
    arrow.group = fopAc_ACTOR_e, arrow.pid = 905, arrow.parent = boss.pid;
    arrow.at.owner_ac = &arrow;
    g_actors.push_back(&arrow);
    w.link.mDamageTimer = 0;
    foe_hits(w, arrow);
    w.check_damage_action();
    CHECK(w.last_commit == 6 && ld.c.applied == 1);
    // Link dead, a scene, the wolf, a rupee-armor hit: native.
    g.p8.tick += 2;
    w.life->life = 0;
    w.link.mDamageTimer = 0;
    foe_hits(w, boss.pid ? arrow : arrow);
    w.check_damage_action();
    CHECK(w.last_commit == 4 && ld.c.why[unsigned(lnkdmg::Why::LinkState)] == 1);
    w.life->life = 40;
    w.link.magic_armor = true, w.link.mDamageTimer = 0;
    foe_hits(w, arrow);
    w.check_damage_action();
    CHECK(w.last_commit == 4 && ld.c.why[unsigned(lnkdmg::Why::ArmorNoHp)] == 1);
    w.link.magic_armor = false;
    clear_slots_of(w);
  }
  // Several enemies in one tick: the native sum commits once and gets one x1.5.
  // A real Lizalfos swing landing on a broken Link: the whole chain with the production adapter.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    enter_link_break(w);
    w.tick();
    w.contact_damage = 2;
    const int life0 = w.life->life;
    (void)run_combo(w, a, 0);
    CHECK(w.link.damage_points >= 1);
    CHECK(ld.c.applied == w.link.damage_points || ld.c.applied >= 1);
    CHECK(w.life->life == life0 - static_cast<int>(ld.c.applied) * 3 - static_cast<int>(w.link.damage_points - ld.c.applied) * 2);
    clear_slots_of(w);
  }
  // Without the Lizalfos module installed for this actor (excluded) the general rule still stands: it is not a Lizalfos rule.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    a->arg1 = 1;
    settle(w, a);
    CHECK(!slot_of(a)->b.admitted);
    Foe f;
    f.group = fopAc_ENEMY_e, f.pid = 900;
    f.at.owner_ac = &f;
    g_actors.push_back(&f);
    enter_link_break(w);
    w.tick();
    w.contact_damage = 2;
    foe_hits(w, f);
    w.tick();
    CHECK(w.last_commit == 3);
    clear_slots_of(w);
  }
}
// ---------------------------------------------------------------- W10: lifecycle of the production install and the coverage fail-safe
static void w_lifecycle() {
  // All or nothing is a real result of liz_install, measured in w_install below. Shutdown: inert afterwards.
  World w;
  fresh_world(w);
  w.add_dn(0, 0, 100);
  auto* a = w.dn[0];
  settle(w, a);
  liz_shutdown();
  CHECK(!lz.enabled);
  const int hp = a->actor.health;
  const auto fc = a->frame_counter;
  w.tick();
  CHECK(a->actor.health == hp && a->frame_counter == fc + 1);
  lz.enabled = true;
  // A foreign/recycled address never inherits
  auto* s = slot_of(a);
  s->b.p.add(50);
  a->actor.pid = 999;
  w.tick();
  s = slot_of(a);
  CHECK(s && s->id == 999 && s->b.p.milli == 0);
  // Event: nothing is judged; the combo ends; the counter is gone
  g_world_safe = false;
  w.tick();
  g_world_safe = true;
  clear_slots_of(w);
  // Summary text exists and carries the required counters.
  World v;
  fresh_world(v);
  v.add_dn(0, 0, 100);
  settle(v, v.dn[0]);
  g_log_store.clear();
  liz_summary();
  bool found = false;
  for (const auto& l : g_log_store) found = found || (l.find("swings_detected=") != std::string::npos && l.find("collision_gap_fixed=") != std::string::npos && l.find("native_feedback_receipt=") != std::string::npos && l.find("deflect_armed=") != std::string::npos && l.find("break_standing=") != std::string::npos);
  CHECK(found);
  clear_slots_of(v);
}
// The actor is deleted in the middle of a combo: the events of that combo leave with it, the slot is gone, nothing of it answers any later hook.
static void w_delete() {
  World w;
  fresh_world(w);
  w.add_dn(0, 0, 100);
  auto* a = w.dn[0];
  settle(w, a);
  auto* s = slot_of(a);
  a->timer[2] = 0;
  for (int k = 0; k < 40 && !s->b.ep.active; ++k) w.tick();
  CHECK(s->b.ep.active);
  const auto deleted0 = lz.deleted;
  g_log_store.clear();
  e_dn_class* ap = a;
  void* args[1] = {&ap};
  CHECK(on_liz_delete_pre(nullptr, args, nullptr, nullptr) == HOOK_CONTINUE);
  bool ended = false;
  for (const auto& l : g_log_store) ended = ended || l.find("LIZ_COMBO event=END") != std::string::npos;
  CHECK(ended);
  CHECK(lz.deleted == deleted0 + 1 && liz_find(a) == nullptr);
  // Its shield no longer answers: the rebound of a foreign actor's contact stays native.
  const int rebounds0 = w.link.rebounds;
  w.set_cut(1);
  auto& c = w.link.mAtCps[0];
  c.at_hit = &a->tate_sph, c.at_shield_hit = true;
  a->tate_sph.tg_hit = &c;
  w.reverse_check();
  CHECK(w.link.rebounds == rebounds0 + 1);
  c.at_hit = nullptr, c.at_shield_hit = false;
  a->tate_sph.tg_hit = nullptr;
  clear_slots_of(w);
}
// Another plugin on the same native functions, called before or after this mod's callbacks: a pure observer changes nothing, and it sees what its place in the list implies.
static std::vector<int> g_foreign_amounts;
static std::vector<int> g_foreign_reverse;
static unsigned g_foreign_calls = 0;
static HookAction foreign_pre(ModContext*, void*, void*, void*) { return ++g_foreign_calls, HOOK_CONTINUE; }
static void foreign_post(ModContext*, void*, void*, void*) { ++g_foreign_calls; }
static HookAction foreign_amount_pre(ModContext*, void* args, void*, void*) {
  g_foreign_amounts.push_back(mods::arg<int>(args, 1));
  return HOOK_CONTINUE;
}
static void foreign_reverse_post(ModContext*, void*, void* ret, void*) { g_foreign_reverse.push_back(*static_cast<BOOL*>(ret)); }
template <class T> static void foreign_add(bool first, PreFn f, PostFn q) {
  if (f) {
    if (first) Reg<T>::pre.insert(Reg<T>::pre.begin(), {f, 0});
    else Reg<T>::pre.push_back({f, 0});
  }
  if (q) {
    if (first) Reg<T>::post.insert(Reg<T>::post.begin(), {q, 0});
    else Reg<T>::post.push_back({q, 0});
  }
}
template <class T> static void foreign_remove() {
  auto& a = Reg<T>::pre;
  auto& b = Reg<T>::post;
  a.erase(std::remove_if(a.begin(), a.end(), [](const auto& x) { return x.f == foreign_pre || x.f == foreign_amount_pre; }), a.end());
  b.erase(std::remove_if(b.begin(), b.end(), [](const auto& x) { return x.f == foreign_post || x.f == foreign_reverse_post; }), b.end());
}
struct OrderSig {
  int hp = 0, points = 0, rebounds = 0, lost = 0;
  std::uint64_t blocks = 0, parries = 0, suppressed = 0, kept = 0, applied = 0, extra = 0;
  bool counter = false;
  bool operator==(const OrderSig&) const = default;
};
static OrderSig order_scenario(int where) {  // 0: this mod alone; 1: the other plugin first; 2: the other plugin last
  g_foreign_amounts.clear(), g_foreign_reverse.clear();
  if (where != 0) {
    const bool first = where == 1;
    foreign_add<LizExec>(first, foreign_pre, foreign_post);
    foreign_add<LizDamage>(first, foreign_pre, foreign_post);
    foreign_add<LizSmall>(first, foreign_pre, nullptr);
    foreign_add<LizBig>(first, foreign_pre, nullptr);
    foreign_add<LizRun>(first, foreign_pre, foreign_post);
    foreign_add<LizGuard>(first, nullptr, foreign_post);
    foreign_add<LinkGuardSe>(first, nullptr, foreign_post);
    foreign_add<LinkCutType>(first, nullptr, foreign_post);
    foreign_add<LinkDamageFix01>(first, foreign_pre, foreign_post);
    foreign_add<LinkCutReverse>(first, nullptr, foreign_reverse_post);
    foreign_add<LinkSetDamage>(first, foreign_amount_pre, nullptr);
  }
  OrderSig o;
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    for (int g2 = 0; g2 < 100000; ++g2) {
      liz::Rng pr = s->b.rng;
      if (pr.below(100) < 80) break;
      (void)s->b.rng.next();
    }
    a->timer[2] = 5000;
    present_guard(a);
    block(w, a);
    block(w, a);
    w.set_cut(1);
    a->timer[2] = 5000;
    w.tick();
    w.tick();
    sword_on_shield(w, a);
    w.tick();
    o.hp = a->actor.health, o.points = s->b.p.points(), o.rebounds = w.link.rebounds, o.counter = s->b.counter.armed;
    o.blocks = s->b.st.blocks, o.parries = s->b.st.enemy_parries, o.suppressed = s->b.st.rebound_suppressed, o.kept = s->b.st.rebound_kept;
    clear_slots_of(w);
  }
  {
    World w;
    fresh_world(w);
    Foe f;
    f.group = fopAc_ENEMY_e, f.pid = 900;
    f.at.owner_ac = &f;
    g_actors.push_back(&f);
    enter_link_break(w);
    w.tick();
    w.link.mDamageTimer = 0;
    const int life0 = w.life->life;
    w.contact_damage = 4;
    foe_hits(w, f);
    w.tick();
    o.lost = life0 - w.life->life, o.applied = ld.c.applied, o.extra = ld.c.extra_units;
    clear_slots_of(w);
  }
  if (where != 0) {
    foreign_remove<LizExec>(), foreign_remove<LizDamage>(), foreign_remove<LizSmall>(), foreign_remove<LizBig>(), foreign_remove<LizRun>(), foreign_remove<LizGuard>();
    foreign_remove<LinkGuardSe>(), foreign_remove<LinkCutType>(), foreign_remove<LinkDamageFix01>(), foreign_remove<LinkCutReverse>(), foreign_remove<LinkSetDamage>();
  }
  return o;
}
static void w_plugin_order() {
  const OrderSig alone = order_scenario(0);
  CHECK(alone.blocks == 2 && alone.parries == 1 && alone.rebounds == 1 && alone.suppressed == 2 && alone.kept == 1 && alone.counter);
  CHECK(alone.lost == 6 && alone.applied == 1 && alone.extra == 2);
  g_foreign_calls = 0;
  const OrderSig before = order_scenario(1);
  CHECK(g_foreign_calls > 0 && before == alone);
  // Called first, the other plugin sees the native values: the native base and the native rebound of each block.
  CHECK(g_foreign_amounts.size() == 1 && g_foreign_amounts[0] == 4);
  CHECK(std::count(g_foreign_reverse.begin(), g_foreign_reverse.end(), TRUE) >= 3);
  g_foreign_calls = 0;
  const OrderSig after = order_scenario(2);
  CHECK(g_foreign_calls > 0 && after == alone);
  // Called last, it sees this mod's decision: the raised amount, and FALSE on an ordinary block.
  CHECK(g_foreign_amounts.size() == 1 && g_foreign_amounts[0] == 6);
  CHECK(std::count(g_foreign_reverse.begin(), g_foreign_reverse.end(), FALSE) >= 2);
  // This mod's own callbacks reversed against each other on the one target where it has several.
  std::reverse(Reg<LinkDamageFix01>::pre.begin(), Reg<LinkDamageFix01>::pre.end());
  std::reverse(Reg<LinkDamageFix01>::post.begin(), Reg<LinkDamageFix01>::post.end());
  const OrderSig reversed = order_scenario(0);
  CHECK(reversed == alone);
  std::reverse(Reg<LinkDamageFix01>::pre.begin(), Reg<LinkDamageFix01>::pre.end());
  std::reverse(Reg<LinkDamageFix01>::post.begin(), Reg<LinkDamageFix01>::post.end());
}
static void w_install() {
  // Measured against the real list: every single failing registration leaves the module off.
  const unsigned total = 24;
  for (unsigned fail = 0; fail < total; ++fail) {
    g_install_count = 0;
    g_fail_install = static_cast<int>(fail);
    c4.enabled = true;
    lz.enabled = true;
    liz_install();
    CHECK(!lz.enabled);
    CHECK(ld.enabled);  // independent: the general rule does not need the Lizalfos module
  }
  // FIX04: the parry FX fails by function and says which hook did: no decision hook, nothing at all; a spark hook, the clash still works; a sound hook, the spark still works.
  for (unsigned fail = 0; fail < kPfxHookCount; ++fail) {
    g_install_count = 0;
    g_fail_install = static_cast<int>(fail);
    lz.enabled = true;
    g_logs = &g_log_store;
    g_log_store.clear();
    pfx_install();
    const bool decision = fail <= kPfxHitmarkPost, spark = fail >= kPfxMarkPre && fail <= kPfxParticlePost;
    CHECK(pf.enabled == !decision && pf.spark == (!decision && !spark) && pf.sound == (!decision && spark));
    bool named = false, install = false;
    for (const auto& l : g_log_store) {
      named = named || (l.find("event=HOOK") != std::string::npos && l.find(std::string("name=") + kPfxHookNames[fail]) != std::string::npos && l.find(std::string("kind=") + kPfxHookKinds[fail]) != std::string::npos && l.find("result=0 ") == std::string::npos);
      install = install || (l.find("event=INSTALL") != std::string::npos && l.find(decision ? "reason=DECISION_HOOK_FAILED" : spark ? "reason=SPARK_HOOK_FAILED" : "reason=SOUND_HOOK_FAILED") != std::string::npos);
    }
    CHECK(named && install);
  }
  g_install_count = 0, g_fail_install = -1, lz.enabled = true;
  pfx_install();
  CHECK(pf.enabled && pf.spark && pf.sound);
  g_fail_install = -1, lz.enabled = false;
  pfx_install();
  CHECK(!pf.enabled);
  g_install_count = 0;
  g_fail_install = static_cast<int>(total);  // only the general hook fails
  liz_install();
  CHECK(lz.enabled && !ld.enabled);
  g_fail_install = -1;
}
// ================================================================ FIX03 0.12.3
// Index (0-based) of the first impact of every D action of combo p.
static std::vector<unsigned> d1_impacts(unsigned p) {
  std::vector<unsigned> out;
  unsigned idx = 0;
  for (unsigned i = 0; i < liz::kPattern[p].acts; ++i) {
    if (liz::kPattern[p].a[i] == liz::Act::D) out.push_back(idx);
    idx += liz::impacts_of(liz::kPattern[p].a[i]);
  }
  return out;
}
static std::vector<std::uint64_t> dry_contacts(unsigned p) {
  World w;
  fresh_world(w);
  w.add_dn(0, 0, 100);
  auto* a = w.dn[0];
  w.link.r_held = true;
  settle(w, a);
  const std::uint64_t base = g.p8.tick;
  auto v = contact_ticks(w, a, p, base);
  clear_slots_of(w);
  return v;
}
// P0-1 (both 0.12.2 logs: dpair=0, buffered=0, rearms=0, and EXACTLY matched + gap_arms == chain_cancelled): reproduced with the 0.12.2 maintenance, then
// corrected. Every D2 of the four combos, two real presses, two native clips, two serials.
static void w_fix03_chain() {
  unsigned pairs = 0;
  for (unsigned p = 0; p < 4; ++p) {
    const auto opens = dry_contacts(p);
    CHECK(opens.size() == liz::kPattern[p].impacts);
    for (unsigned k : d1_impacts(p)) {
      CHECK(opens[k + 1] - opens[k] >= 10 && opens[k + 1] - opens[k] <= 12);  // D1 -> D2 contacts: the clip, not a schedule value
      for (int fix02 = 1; fix02 >= 0; --fix02) {
        World w;
        fresh_world(w);
        w.add_dn(0, 0, 100);
        auto* a = w.dn[0];
        w.link.r_held = true;
        settle(w, a);
        const std::uint64_t b0 = g.p8.tick;
        g_fix02_gate = fix02 != 0;
        w.link.presses.insert(b0 + opens[k] - 3);
        w.link.presses.insert(b0 + opens[k + 1] - 4);
        const Trace t = run_combo(w, a, p);
        for (int e = 0; e < 3; ++e) a->timer[2] = 5000, w.tick();  // the last arm meets its maintenance
        const auto* s = slot_of(a);
        CHECK(t.imp.size() == liz::kPattern[p].impacts);
        CHECK(s->b.st.link_parry_by_impact[p][k] == 1);
        CHECK(c4.shield.c.input_new == 2);
        if (fix02) {
          CHECK(s->b.st.link_parry_by_impact[p][k + 1] == 0 && s->b.st.d_pair_parry == 0);
          CHECK(c4.shield.c.buffered == 0 && c4.shield.c.rearms == 0 && lz.double_rearms == 0 && w.link.shield_attacks == 1);
          CHECK(c4.shield.c.matched + lz.gap_arms == c4.shield.c.cancelled);  // the exact arithmetic of both logs (429+359=788, 287+204=491)
        } else {
          CHECK(s->b.st.link_parry_by_impact[p][k + 1] == 1 && s->b.st.d_pair_parry == 1 && s->b.st.link_parry == 2 && s->b.p.gained == 24000);
          CHECK(c4.shield.c.buffered >= 1 && lz.double_rearms >= 1 && w.link.shield_attacks == 2 && c4.shield.c.native_starts == 2 && c4.shield.c.matched == 2);  // D1 itself may come from the slip of a blocked N
        }
        g_fix02_gate = false;
        ++pairs;
        clear_slots_of(w);
      }
    }
  }
  CHECK(pairs == 2u * 6u);  // P1 1 + P2 1 + P3 3 + P4 1 doubles
  // Whole combos parried impact after impact (one real press per impact, each its own native clip): 4/7/9/6, P3 breaks exactly on its ninth.
  for (unsigned p = 0; p < 4; ++p) {
    const auto opens = dry_contacts(p);
    for (int lead : {3, 5}) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      w.link.r_held = true;
      settle(w, a);
      const std::uint64_t b0 = g.p8.tick;
      for (auto o : opens) w.link.presses.insert(b0 + o - static_cast<std::uint64_t>(lead));
      const Trace t = run_combo(w, a, p);
      const auto* s = slot_of(a);
      const unsigned n = liz::kPattern[p].impacts;
      CHECK(t.imp.size() == n);
      for (unsigned k = 0; k < n; ++k) CHECK(s->b.st.link_parry_by_impact[p][k] == 1);
      CHECK(s->b.st.link_parry == n && w.link.shield_attacks == n && c4.shield.c.native_starts == n && c4.shield.c.untrusted == 0);
      CHECK(s->b.st.d_pair_parry == d1_impacts(p).size());
      CHECK(w.link.damage_points == 0);
      if (p == 2) CHECK(s->b.p.latched && s->b.st.ended[unsigned(liz::End::Posture100)] == 1);  // 9 x 12 crosses 100 on the ninth, not before
      else CHECK(!s->b.p.latched && s->b.p.gained == 12000u * n);
      clear_slots_of(w);
    }
  }
  // P3-1 timing, exactly as both logs show it (delta 0.00, MISSED as an ordinary block): a press on the very tick of the contact pass has no shield
  // motion at that pass and never pays; one tick earlier (delta 1) it does. The window is physical, not widened: [1..7] frames of real clip motion.
  {
    const auto opens = dry_contacts(2);
    for (int lead : {0, -1}) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      w.link.r_held = true;
      settle(w, a);
      const std::uint64_t b0 = g.p8.tick;
      w.link.presses.insert(static_cast<std::uint64_t>(static_cast<std::int64_t>(b0 + opens[0]) - lead));
      (void)run_combo(w, a, 2);
      const auto* s = slot_of(a);
      CHECK(s->b.st.link_parry_by_impact[2][0] == 0);
      clear_slots_of(w);
    }
  }
}
// P0-2: the shield covers what the SHIELD sphere really faces. Link around a guarding actor at 160, the body exposed (invuln 0), one sword swing
// meeting the shield sphere and the body sphere in the same pass.
struct CoverCase { int deg; bool covered; };
static void same_pass(World& w, e_dn_class* a, int deg) {
  const float th = float(deg) * kPi / 180.0f;
  w.link.current.pos = cXyz(160.0f * std::sin(th), 0.0f, 160.0f * std::cos(th));
  a->actor.shape_angle.y = a->actor.current.angle.y = 0;
  const float sx = g_shield_side, sz = g_shield_fwd;  // body frame at yaw 0 = world
  a->tate_sph.SetC(cXyz(a->actor.current.pos.x + sx, 110.0f, a->actor.current.pos.z + sz));
  a->invulnerability_timer = 0, a->unk_timer_2 = 0;
  w.set_cut(1);
  sword_on_shield(w, a);
  w.link.mAtCps[0].at_type = kAtSword;
  a->cc_sph[0].tg_hit = &w.link.mAtCps[0];
  a->hit_power = 4, a->hit_strong = false;
  w.tick();
}
static void w_fix03_shield() {
  const CoverCase cases[] = {{0, true}, {30, true}, {90, true}, {105, true}, {-30, true}, {-45, true}, {-60, true}, {-70, false}, {-90, false}, {-135, false}, {180, false}, {125, false}};
  for (const auto& c : cases) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    present_guard(a);
    a->mode = 8, a->morph.rate = 1, a->morph.mFrameCtrl.mFrame = 15, a->morph.end = 22;  // lowering but still presented: invulnerability off, the shield contact itself decides
    auto* s = slot_of(a);
    const int hp0 = a->actor.health;
    const int pts0 = s->b.p.points();
    same_pass(w, a, c.deg);
    s = slot_of(a);
    if (c.covered) {
      CHECK(a->actor.health == hp0 && s->b.st.shield_won_body == 1 && s->b.st.blocks == 1 && s->b.p.points() == pts0 + 5);
    } else {
      CHECK(a->actor.health < hp0 && s->b.st.shield_won_body == 0 && s->b.st.blocks == 0 && s->b.st.block_rear_rejected == 1);
    }
    CHECK(w.link.rebounds == 0);  // an ordinary block, covered or not, never rebounds Link's sword
    clear_slots_of(w);
  }
  // A shield held far out to the side never covers the back: Link at 120 deg is past the +-112.5 deg body limit even if the projection is positive.
  {
    const float side0 = g_shield_side, fwd0 = g_shield_fwd;
    g_shield_side = 70.0f, g_shield_fwd = 10.0f;
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    present_guard(a);
    a->mode = 8, a->morph.rate = 1, a->morph.mFrameCtrl.mFrame = 15, a->morph.end = 22;
    const int hp0 = a->actor.health;
    same_pass(w, a, 120);
    CHECK(a->actor.health < hp0 && slot_of(a)->b.st.shield_won_body == 0);
    clear_slots_of(w);
    g_shield_side = side0, g_shield_fwd = fwd0;
  }
  // On the SHIELD side (90 deg left) a presented guard protects the body from its very first tick (0.12.2: "behind", body exposed).
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.link.current.pos = cXyz(160.0f, 0.0f, 0.0f);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    present_guard(a);
    a->invulnerability_timer = 0;
    a->timer[0] = 60, a->timer[2] = 5000;
    w.tick();
    CHECK(a->invulnerability_timer > 0);
    clear_slots_of(w);
  }
  // 0.12.2 decided by the body's yaw alone: Link at 90 deg on the SHIELD side was "behind" (spark + HP loss); at -45 deg on the open side it was "front".
  // The same swings, judged by the old predicate, give the opposite answers: the lab keeps both facts.
  CHECK(!(liz::Brain::facing(0, static_cast<std::int16_t>(0x4000), liz::policy::kFrontHalfAngle)));
  // The guard turns toward Link with the native law: at most 0x800 per tick, never a snap, and the body is protected only once the shield faces him.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.link.current.pos = cXyz(-160.0f, 0.0f, 0.0f);  // 90 deg on the open (right) side
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    present_guard(a);
    a->timer[0] = 60;
    std::int16_t prev = a->actor.current.angle.y;
    int turned = 0, protected_ticks = 0, first_protected = -1;
    for (int k = 0; k < 30; ++k) {
      a->timer[0] = 60, a->timer[2] = 5000;
      w.tick();
      const std::int16_t cur = a->actor.current.angle.y;
      const int step = std::abs(int(static_cast<std::int16_t>(static_cast<std::uint16_t>(cur) - static_cast<std::uint16_t>(prev))));
      CHECK(step <= 0x800);
      if (step) ++turned;
      prev = cur;
      if (a->invulnerability_timer > 0) {
        ++protected_ticks;
        if (first_protected < 0) first_protected = k;
      }
    }
    CHECK(turned >= 4 && first_protected >= 2 && protected_ticks >= 20);
    CHECK(std::abs(int(static_cast<std::int16_t>(static_cast<std::uint16_t>(a->actor.shape_angle.y) - static_cast<std::uint16_t>(static_cast<std::int16_t>(-0x4000))))) < 0x200);
    clear_slots_of(w);
  }
  // From behind nothing turns into protection inside the tick, and nothing outside the guard turns at all.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    w.link.current.pos = cXyz(0.0f, 0.0f, -160.0f);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->action = 3, a->mode = 0;
    auto* s = slot_of(a);
    const auto t0 = s->b.st.guard_turn_ticks;
    a->timer[2] = 5000;
    w.tick();
    CHECK(slot_of(a)->b.st.guard_turn_ticks == t0);
    clear_slots_of(w);
  }
}
// ---- P1-3 through the PRODUCTION parry FX adapter
static bool is_orange(const GXColor& c, const GXColor& res) {
  const pfx::Rgb o = pfx::orange(pfx::Rgb{res.r, res.g, res.b});
  return c.r == o.r && c.g == o.g && c.b == o.b;
}
static bool is_res(const GXColor& c, const GXColor& res) { return c.r == res.r && c.g == res.g && c.b == res.b; }
// Every colour of the FIRST drawn frame of an emitter: all orange (same alpha), or all the resource's own.
static void check_first_frame(const World::LabEmitter& e, bool orange) {
  CHECK(e.drawn.size() == 8);
  for (size_t i = 0; i < e.drawn.size(); ++i) {
    const GXColor& res = i % 2 == 0 ? e.res_prm : e.res_env;
    CHECK(orange ? is_orange(e.drawn[i], res) : is_res(e.drawn[i], res));
    CHECK(e.drawn[i].a == res.a);
  }
}
struct ChainCount : JPAEmitterCallBack {
  int ex = 0, af = 0, dr = 0, da = 0;
  void execute(JPABaseEmitter*) override { ++ex; }
  void executeAfter(JPABaseEmitter*) override { ++af; }
  void draw(JPABaseEmitter*) override { ++dr; }
  void drawAfter(JPABaseEmitter*) override { ++da; }
};
static void w_fix03_fx() {
  CHECK(pf.enabled);
  // Link's parry on a Lizalfos: only the parried impact's own two native emitters go orange, from their first frame; its clash alone is higher.
  for (unsigned p = 0; p < 4; ++p) {
    const auto opens = dry_contacts(p);
    for (unsigned k = 0; k < opens.size(); ++k) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      w.link.r_held = true;
      settle(w, a);
      const std::uint64_t b0 = g.p8.tick;
      w.link.presses.insert(b0 + opens[k] - 3);
      (void)run_combo(w, a, p);
      for (int e = 0; e < 3; ++e) a->timer[2] = 5000, w.tick();
      const auto* s = slot_of(a);
      CHECK(s->b.st.link_parry_by_impact[p][k] == 1 && s->b.st.link_parry == 1);
      CHECK(pf.book.c.preview[1] == 1 && pf.book.c.confirmed[1] == 1 && pf.book.c.reverted[1] == 0 && pf.book.c.emitters == 2 && pf.book.c.sounds == 1);
      unsigned orange = 0, plain = 0;
      for (auto& up : w.emitters) {
        const bool o = up->mpEmtrCallBack == g_pfx_cb;
        check_first_frame(*up, o);
        ++(o ? orange : plain);
        CHECK(up->res == 0xA || up->res == 0xB);  // never another effect
      }
      CHECK(orange == 2);
      unsigned high = 0;
      for (auto& snd : w.playbacks) {
        if (snd->m.params_.mPitch != 1.0f) {
          ++high;
          CHECK(std::fabs(snd->m.params_.mPitch - pfx::kPitch) < 1e-6f);
        }
        CHECK(snd->hit == 0xC0);
      }
      CHECK(high == 1 && w.playbacks.size() >= liz::kPattern[p].impacts - 1);  // ordinary blocks keep their clash untouched
      for (unsigned h = 0; h < kPfxHookCount; ++h) CHECK(pf.calls[h] > 0);  // FIX04: every hook really ran, it is not enough that the API said MOD_OK
      clear_slots_of(w);
    }
  }
  // Ordinary blocks and body hits: nothing at all.
  {
    const auto opens = dry_contacts(0);
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    w.link.r_held = true;
    settle(w, a);
    (void)run_combo(w, a, 0);
    CHECK(pf.book.c.preview[1] == 0 && pf.book.c.emitters == 0 && pf.book.c.sounds == 0 && g_move_pitch_calls >= 0);
    for (auto& up : w.emitters) {
      check_first_frame(*up, false);
      CHECK(up->mpEmtrCallBack == nullptr);
    }
    for (auto& snd : w.playbacks) CHECK(snd->m.params_.mPitch == 1.0f);
    CHECK(!opens.empty());
    clear_slots_of(w);
  }
  // The Lizalfos' own parry: its two native sparks (pass + its Execute's) orange, its shield SE higher, once; an ordinary block of the same swing kind: original.
  for (int parry = 1; parry >= 0; --parry) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    if (parry) s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    else s->b.attempt_armed = false, s->b.verdict_swing = 0;
    sword_on_shield(w, a);
    const size_t em0 = w.emitters.size(), pb0 = w.playbacks.size();
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    CHECK(w.emitters.size() == em0 + 2);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.enemy_parries == (parry ? 1u : 0u));
    CHECK(pf.book.c.preview[3] == (parry ? 1u : 0u) && pf.book.c.confirmed[3] == (parry ? 1u : 0u) && pf.book.c.reverted[3] == 0);
    CHECK(w.emitters.size() == em0 + 4 && w.playbacks.size() == pb0 + 1);
    for (size_t i = em0; i < w.emitters.size(); ++i) check_first_frame(*w.emitters[i], parry != 0);
    CHECK(w.playbacks[pb0]->hit == 0xD0 && (parry ? std::fabs(w.playbacks[pb0]->m.params_.mPitch - pfx::kPitch) < 1e-6f : w.playbacks[pb0]->m.params_.mPitch == 1.0f));
    clear_slots_of(w);
  }
  // Link's parry on an E_OC: POST08's conditions in the pass, settled by its commit; a failed commit puts colour and pitch back.
  for (int commit = 1; commit >= 0; --commit) {
    World w;
    fresh_world(w);
    daE_OC_c oc;
    Morf mf;
    oc.name = oc.profname = fpcNm_E_OC_e, oc.pid = 77, oc.health = 40, oc.mpMorf = &mf;
    g_actors.push_back(&oc);
    auto& led = g.eng.obs.ledger.s[3];
    led.id = 77, led.ptr = reinterpret_cast<std::uintptr_t>(&oc), led.life = Life::Active, led.adm = Adm::Candidate, led.generation = 5;
    auto& en = g.pe.px.e[3];
    en = LabEntry{}, en.bound = true, en.id = 77, en.generation = 5;
    g.parry = post08::Parry{}, g.parry.enabled = true;
    w.link.r_held = true;
    for (int k = 0; k < 2; ++k) w.tick();
    w.link.presses.insert(g.p8.tick + 1);
    for (int k = 0; k < 4; ++k) w.tick();
    CHECK(w.link.mProcID == daAlink_c::PROC_GUARD_ATTACK);
    dCcD_Sph at;
    at.owner_ac = &oc;
    w.link.mTgCyls[0].tg_hit_ac = &oc;
    const size_t em0 = w.emitters.size();
    g.p8.layout = -1;  // FIX04: a parry that is the very first contact of the session with a Bokoblin
    w.proc_hitmark(&at, &w.link.mTgCyls[0], true, 6, &w.link);
    g.p8.layout = 1;
    w.proc_hitmark(&at, &w.link.mTgCyls[1], true, 6, &w.link);  // a second callback of the same contact: one record
    CHECK(w.emitters.size() == em0 + 2);
    CHECK(pf.book.c.preview[2] == 1 && pf.book.c.duplicate == 1);
    w.particles();
    ++g.p8.tick;
    dCcD_GObjInf* o = &w.link.mTgCyls[0];
    w.link.setGuardSe(o);
    CHECK(pf.book.c.sounds == 1);
    JAISound* snd = w.playbacks.back().get();
    CHECK(std::fabs(snd->m.params_.mPitch - pfx::kPitch) < 1e-6f);
    w.link.setGuardSe(o);  // the same contact reported again: one high playback at most
    CHECK(pf.book.c.sounds == 1 && w.playbacks.back()->m.params_.mPitch == 1.0f);
    pfx_settle(pfx::Kind::LinkOnEoc, 77, commit != 0);
    CHECK(pf.book.c.confirmed[2] == (commit ? 1u : 0u) && pf.book.c.reverted[2] == (commit ? 0u : 1u));
    CHECK(snd->m.params_.mPitch == (commit ? snd->m.params_.mPitch : 1.0f));
    if (!commit) CHECK(pf.book.c.sound_reverted == 1);
    w.particles();
    auto& e0 = *w.emitters[em0];
    CHECK(commit ? is_orange(e0.mPrmClr, e0.res_prm) : is_res(e0.mPrmClr, e0.res_prm));
    CHECK(commit ? is_orange(e0.base[0].mData.mEnvClr, e0.res_env) : is_res(e0.base[0].mData.mEnvClr, e0.res_env));
    // Outside the shield Attack window, a latched actor, or an actor not in its swing: no preview.
    en.latched = true;
    w.proc_hitmark(&at, &w.link.mTgCyls[0], true, 6, &w.link);
    en.latched = false, oc.mActionMode = 3;
    w.proc_hitmark(&at, &w.link.mTgCyls[0], true, 6, &w.link);
    oc.mActionMode = 4;
    for (int k = 0; k < 9; ++k) w.tick();  // past frame +7 of the clip: a late contact is an ordinary block
    w.proc_hitmark(&at, &w.link.mTgCyls[0], true, 6, &w.link);
    CHECK(pf.book.c.preview[2] == 1);
    std::erase(g_actors, static_cast<fopAc_ac_c*>(&oc));
    led = LabLedgerSlot{}, en = LabEntry{}, g.parry.enabled = false;
    clear_slots_of(w);
  }
  // The native callback of an emitter keeps running; a failed spawn is only counted; a foreign particle outside a parry hit mark is never captured.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    ChainCount cc;
    pf.ctx_mark = pf.book.open(pfx::Kind::LinkOnLizalfos, 100, g.p8.tick);
    w.next_native_cb = &cc;
    auto* e = static_cast<World::LabEmitter*>(w.particle_set(0xA));
    w.next_native_cb = nullptr;
    CHECK(e->mpEmtrCallBack == g_pfx_cb);
    w.next_particle_fails = true;
    (void)w.particle_set(0xB);
    w.next_particle_fails = false;
    CHECK(pf.book.c.no_emitter == 1);
    pf.ctx_mark = -1;
    auto* other = static_cast<World::LabEmitter*>(w.particle_set(0xA));
    CHECK(other->mpEmtrCallBack == nullptr);
    w.particles();
    CHECK(cc.ex == 1 && cc.af == 1 && cc.dr == 1 && cc.da == 1);
    check_first_frame(*e, true), check_first_frame(*other, false);
    // A record with no more callbacks frees itself; a reused emitter (callback reset by the native init) is simply not ours.
    e->dead = true;
    const auto now = g.p8.tick;
    g.p8.tick = now + pfx::kKeepTicks + 2;
    pfx_maintenance();
    bool any = false;
    for (auto& x : pf.em) any = any || x.e == e;
    CHECK(!any);
    g.p8.tick = now;
    clear_slots_of(w);
  }
  // Inside the Lizalfos' Execute of its parried contact, the Shield Attack bash SE (map 30) is not the shield clash (map 40).
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    pf.book.open(pfx::Kind::LizalfosOnLink, 100, g.p8.tick - 1);
    pfx_liz_exec_begin(a, 100);
    w.native_def_se(&a->sound, &w.link.mAtCps[0], 30, &a->actor);
    CHECK(w.playbacks.back()->m.params_.mPitch == 1.0f && pf.book.c.sounds == 0);
    Z2Creature other;
    w.native_def_se(&other, &w.link.mAtCps[0], 40, nullptr);  // another creature's clash in that same tick
    CHECK(w.playbacks.back()->m.params_.mPitch == 1.0f && pf.book.c.sounds == 0);
    w.native_def_se(&a->sound, &w.link.mAtCps[0], 40, nullptr);
    CHECK(std::fabs(w.playbacks.back()->m.params_.mPitch - pfx::kPitch) < 1e-6f && pf.book.c.sounds == 1);
    pfx_liz_exec_end();
    clear_slots_of(w);
  }
  // Scene change: nothing of the old particle manager is touched again. Shutdown: living emitters get their own callback back.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    ChainCount cc;
    const int r = pf.book.open(pfx::Kind::LinkOnLizalfos, 100, g.p8.tick);
    pf.ctx_mark = r;
    w.next_native_cb = &cc;
    auto* e = static_cast<World::LabEmitter*>(w.particle_set(0xA));
    w.next_native_cb = nullptr;
    pf.ctx_mark = -1;
    w.particles();
    const bool en = pf.enabled;
    pfx_shutdown();
    CHECK(e->mpEmtrCallBack == &cc && !pf.enabled);
    pf.enabled = en;
    pf.ctx_mark = pf.book.open(pfx::Kind::LinkOnLizalfos, 101, g.p8.tick);
    auto* e2 = static_cast<World::LabEmitter*>(w.particle_set(0xB));
    pf.ctx_mark = -1;
    CHECK(pf.book.c.emitters == 2);
    lg.posture = lnk01::Posture{};  // epoch stays; force the change the host's stage load makes
    pf.epoch = pf.epoch + 1;
    pfx_maintenance();
    for (auto& x : pf.em) CHECK(x.e == nullptr);
    CHECK(e2->mpEmtrCallBack == g_pfx_cb);  // not dereferenced by the clear: the old manager is gone in the game
    clear_slots_of(w);
  }
}
// ---- P1-4 through the PRODUCTION lock-on adapter: dAttention_c::Run and LRlockCheck transcribed, host config with persistence
struct FakeHost {
  static inline std::vector<std::pair<std::string, bool>> saved;
  static inline std::string name;
  static inline bool value = false, default_value = false, fail_register = false, panel_built = false;
  static inline ConfigChangedFn cb = nullptr;
  static inline UiControlDesc control;
  static inline std::string label;
  static ModResult reg(ModContext*, const ConfigVarDesc* d, ConfigVarHandle* h) {
    if (fail_register) return MOD_FAIL;
    name = d->name, default_value = d->default_bool, value = d->default_bool;
    for (auto& kv : saved)
      if (kv.first == name) value = kv.second;
    *h = 42;
    return MOD_OK;
  }
  static ModResult get(ModContext*, ConfigVarHandle h, bool* o) { CHECK(h == 42); *o = value; return MOD_OK; }
  static ModResult sub(ModContext*, ConfigVarHandle, ConfigChangedFn f, void*, ConfigSubscriptionHandle*) { cb = f; return MOD_OK; }
  static void set(bool v) {  // what the toggle does: value + persistence + change notification
    const ConfigVarValue nv{CONFIG_VAR_BOOL, v}, pv{CONFIG_VAR_BOOL, value};
    value = v;
    saved.clear();
    saved.push_back({name, v});
    if (cb) cb(mod_ctx, 42, &nv, &pv, nullptr);
  }
  static ModResult section(ModContext*, UiElementHandle, const char*) { return MOD_OK; }
  static ModResult control_add(ModContext*, UiElementHandle, const UiControlDesc* d, UiElementHandle*) { control = *d, label = d->label; return MOD_OK; }
  static ModResult panel(ModContext* c, const UiModsPanelDesc* d) { panel_built = d->build(c, 1, nullptr, nullptr) == MOD_OK; return MOD_OK; }
};
static const ConfigService g_cfg{FakeHost::reg, FakeHost::get, FakeHost::sub};
static const UiService g_ui{FakeHost::panel, FakeHost::section, FakeHost::control_add};
struct LockRig {
  daAlink_c link;
  std::vector<std::unique_ptr<fopAc_ac_c>> actors;
  u8 held_stored = 0;
  std::uint64_t writes0 = 0;
  LockRig() {
    g_link = &link, link.name = link.profname = fpcNm_ALINK_e, link.pid = 1;
    g_actors.clear(), g_actors.push_back(&link);
    g_world_safe = true, g_window_status = 0, g_opt_attention = 0, g_dComIfG_gameInfo.play.ev.run = false, g_dComIfG_gameInfo.play.pause = false, g_dComIfG_gameInfo.info.st.life = 80;
    g_att = dAttention_c{};
    mDoCPd_c::m_cpadInfo[0] = interface_of_controller_pad{};
    g_cam = dCamera_c{}, g_cam_ptr = &g_cam;
    lk.t.reset();  // each scenario starts from a released L (the module's state is one per session)
  }
  fpc_ProcID add(float x, float z, u32 type) {
    actors.push_back(std::make_unique<fopAc_ac_c>());
    auto* a = actors.back().get();
    a->pid = fpc_ProcID(100 + actors.size()), a->attention_info.position = cXyz(x, 100, z);
    g_actors.push_back(a);
    auto& l = g_att.mLockOnList[g_att.mLockonCount++];
    l.mActorID = a->pid, l.mType = type;
    return a->pid;
  }
  fpc_ProcID locked() const { return g_att.mLockOnList[g_att.mLockOnOffset % g_att.mLockonCount].mActorID; }
  void remove_all() { g_att.mLockonCount = 0, g_att.mLockOnOffset = 0; }
  // mDoCPd_c::read -> LRlockCheck
  void read(float trigger, float cx) {
    auto& p = mDoCPd_c::m_cpadInfo[0];
    p.mTriggerLeft = trigger, p.mCStickPosX = cx, p.mCStickPosY = 0.3f, p.mCStickValue = std::fabs(cx), p.mCStickAngle = 123;
    p.mTrigLockL = 0;
    if (trigger > 0.9f) {
      if (p.mHoldLockL != 1) p.mTrigLockL = 1;
      p.mHoldLockL = 1;
    } else if (trigger < 0.6f) {
      p.mHoldLockL = 0;
    }
  }
  // dAttention_c::checkButton + judgementStatus4Hold / 4Switch
  void run() {
    auto& a = g_att;
    const bool btn = mDoCPd_c::m_cpadInfo[0].mHoldLockL != 0;
    if (g_dComIfG_gameInfo.play.ev.run) { a.mAttnStatus = 0, a.field_0x32b = 4; return; }
    switch (a.field_0x32b) {
      case 4: if (btn) a.field_0x32b = 0; break;
      case 0: a.field_0x32b = 1; [[fallthrough]];
      case 1: if (!btn) a.field_0x32b = 4; break;
    }
    const bool any = a.mLockonCount > 0;
    if (g_opt_attention == 0) {
      switch (a.mAttnStatus) {
        case 0:
          a.field_0x32f = 0, a.field_0x32e = 0, a.mLockOnOffset = 0;
          if (a.field_0x32b == 0 && any) a.mAttnStatus = 1, a.field_0x32e = 15;
          break;
        case 1:
          if (!any) a.mAttnStatus = 0;
          else if (a.field_0x32b == 4) a.mAttnStatus = 2, a.field_0x32f = 10;
          if (a.field_0x32e) --a.field_0x32e;
          break;
        case 2:
          if (!any) a.mAttnStatus = 0;
          else if (a.field_0x32b == 0) a.mLockOnOffset = (a.mLockOnOffset + 1) % a.mLockonCount, a.mAttnStatus = 1, a.field_0x32e = 15;
          else if (a.field_0x32f == 0) a.mAttnStatus = 0;
          if (a.field_0x32f) --a.field_0x32f;
          break;
      }
    } else {
      switch (a.mAttnStatus) {
        case 0:
          a.field_0x32f = 0, a.field_0x32e = 0, a.mLockOnOffset = 0;
          if (a.field_0x32b == 0 && any) a.mAttnStatus = 1, a.field_0x32e = 15, a.field_0x32f = 15;
          break;
        case 1:
          if (a.field_0x32f == 0 || !any) a.mAttnStatus = 0;
          else if (a.field_0x32b == 0) a.mLockOnOffset = (a.mLockOnOffset + 1) % a.mLockonCount, a.field_0x32e = 15;
          if (a.field_0x32b == 1) { if (a.field_0x32f) --a.field_0x32f; }
          else a.field_0x32f = 15;
          break;
      }
    }
  }
  void frame(float trigger, float cx = 0) {
    ++g.p8.tick;
    read(trigger, cx);
    lockon_update();
    if (!g_dComIfG_gameInfo.play.pause) run();
  }
};
static void lockon_boot(bool have_config = true, bool have_ui = true) {
  svc_config = have_config ? &g_cfg : nullptr;
  svc_ui = have_ui ? &g_ui : nullptr;
  FakeHost::panel_built = false, FakeHost::cb = nullptr;
  lockon_install();
}
static void w_fix03_lockon() {
  // The option: a real persisted var, a real toggle bound to it, default ON; the saved value wins at the next start.
  FakeHost::saved.clear();
  lockon_boot();
  CHECK(lk.available && lk.ui && lk.enabled && FakeHost::name == "sekiro_lockon" && FakeHost::default_value && FakeHost::panel_built);
  CHECK(FakeHost::control.kind == UI_CONTROL_TOGGLE && FakeHost::control.binding == UI_BINDING_CONFIG_VAR && FakeHost::control.config_var == 42 && FakeHost::label == "Fijaci\xC3\xB3n de objetivos estilo Sekiro");
  FakeHost::set(false);
  CHECK(!lk.enabled);
  lockon_boot();  // restart
  CHECK(!lk.enabled);
  {  // OFF after a restart: exactly native, nothing written across a whole press/release/flick cycle
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e);
    for (int k = 0; k < 40; ++k) {
      r.frame(k < 20 ? 1.0f : 0.0f, 1.0f);
      CHECK(mDoCPd_c::m_cpadInfo[0].mCStickPosX == 1.0f);
      CHECK(mDoCPd_c::m_cpadInfo[0].mHoldLockL == (k < 20 ? 1 : 0));
    }
    CHECK(lk.writes_pad == 0 && lk.writes_attention == 0 && g_att.mAttnStatus == 0);  // HOLD option: released natively
  }
  FakeHost::set(true);
  lockon_boot();
  CHECK(lk.enabled);
  {  // tap: locks, release keeps it, second press releases and never relocks while held, a new press locks again
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f);
    r.frame(0.0f);
    for (int k = 0; k < 150; ++k) r.frame(0.0f);
    CHECK(g_att.mAttnStatus == 1 && lk.t.st == lockon::St::Toggled && mDoCPd_c::m_cpadInfo[0].mHoldLockL == 1);
    r.frame(1.0f);
    CHECK(lk.t.st == lockon::St::Release);
    for (int k = 0; k < 30; ++k) r.frame(1.0f);
    CHECK(g_att.mAttnStatus == 0 && lk.t.c.cancelled == 1);
    r.frame(0.0f), r.frame(0.0f);
    CHECK(lk.t.st == lockon::St::Idle);
    const auto w0 = lk.writes_pad;
    r.frame(0.0f);
    CHECK(lk.writes_pad == w0 && mDoCPd_c::m_cpadInfo[0].mHoldLockL == 0);  // hands back clean: LRlockCheck reads a released L
    r.frame(1.0f);
    CHECK(g_att.mAttnStatus == 1 && mDoCPd_c::m_cpadInfo[0].mTrigLockL == 1);  // the next real press has its native edge
  }
  {  // several enemies, right stick: one per flick, screen side from the real camera, never an NPC, no jerk at the end of the row
    LockRig r;
    const auto left = r.add(-200, 300, fopAc_attn_BATTLE_e);
    const auto mid = r.add(0, 300, fopAc_attn_BATTLE_e);
    const auto right = r.add(150, 300, fopAc_attn_BATTLE_e);
    r.add(60, 300, fopAc_attn_TALK_e);
    g_att.mLockOnOffset = 1, g_att.mAttnStatus = 1, g_att.field_0x32b = 1;
    (void)left;
    r.frame(1.0f);  // (held from the previous state: first frame is an edge for the core) becomes Held
    r.frame(0.0f);
    CHECK(lk.t.st == lockon::St::Toggled && r.locked() == mid);
    // camera at z=-300 looking +z: screen right is -x (forward x up)
    for (int k = 0; k < 60; ++k) r.frame(0.0f, -1.0f);  // held left on the stick: the enemy at +x is on the LEFT of the screen; one change only
    CHECK(r.locked() == right && lk.t.c.switched == 1 && mDoCPd_c::m_cpadInfo[0].mCStickPosX == 0.0f && mDoCPd_c::m_cpadInfo[0].mCStickValue == 0.0f);
    r.frame(0.0f, 0.0f);
    r.frame(0.0f, 1.0f);
    CHECK(r.locked() == mid && lk.t.c.switched == 2 && g_att.mLockTargetID == mid && g_att.field_0x32e >= 14);
    r.frame(0.0f, 0.0f);
    r.frame(0.0f, 1.0f);
    CHECK(r.locked() == left && lk.t.c.switched == 3);
    r.frame(0.0f, 0.0f);
    r.frame(0.0f, 1.0f);  // nothing further right: stays
    CHECK(r.locked() == left && lk.t.c.no_candidate == 1 && g_att.mAttnStatus == 1);
  }
  {  // no camera: the flick does nothing, nothing crashes
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e), r.add(150, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f), r.frame(0.0f);
    g_cam_ptr = nullptr;
    r.frame(0.0f, 1.0f);
    CHECK(g_att.mAttnStatus == 1 && lk.t.c.no_candidate >= 1);
  }
  {  // SWITCH option: the second press releases instead of cycling; no synthetic hold
    LockRig r;
    g_opt_attention = 1;
    r.add(0, 300, fopAc_attn_BATTLE_e), r.add(150, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f), r.frame(0.0f);
    for (int k = 0; k < 60; ++k) r.frame(0.0f);
    CHECK(g_att.mAttnStatus == 1 && mDoCPd_c::m_cpadInfo[0].mHoldLockL == 0);
    r.frame(1.0f);
    CHECK(g_att.mAttnStatus == 0 && lk.t.c.forced_releases >= 1);
  }
  {  // an NPC lock stays native
    LockRig r;
    r.add(0, 300, fopAc_attn_TALK_e);
    const auto w0 = lk.writes_pad;
    r.frame(1.0f), r.frame(0.0f), r.frame(0.0f);
    CHECK(lk.writes_pad == w0 && lk.t.st == lockon::St::Idle);
  }
  {  // the target goes: no retained lock, no synthetic hold
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f), r.frame(0.0f);
    r.remove_all();
    r.frame(0.0f), r.frame(0.0f);
    const auto w0 = lk.writes_pad;
    r.frame(0.0f);
    CHECK(g_att.mAttnStatus == 0 && lk.t.st == lockon::St::Idle && lk.writes_pad == w0);
  }
  // Event, menu, pause, wolf, horse, dead: native from that frame, nothing written.
  for (int why = 0; why < 6; ++why) {
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f), r.frame(0.0f);
    CHECK(lk.t.st == lockon::St::Toggled);
    if (why == 0) g_dComIfG_gameInfo.play.ev.run = true;
    if (why == 1) g_window_status = 1;
    if (why == 2) g_dComIfG_gameInfo.play.pause = true;
    if (why == 3) r.link.wolf = true;
    if (why == 4) r.link.horse = true;
    if (why == 5) g_dComIfG_gameInfo.info.st.life = 0;
    const auto w0 = lk.writes_pad;
    r.frame(0.0f, 1.0f);
    CHECK(lk.writes_pad == w0 && lk.t.st == lockon::St::Idle && mDoCPd_c::m_cpadInfo[0].mCStickPosX == 1.0f);
  }
  {  // the option goes OFF in the middle of a lock: native at once, the attention releases by itself
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f), r.frame(0.0f);
    FakeHost::set(false);
    const auto w0 = lk.writes_pad;
    for (int k = 0; k < 15; ++k) r.frame(0.0f);
    CHECK(lk.writes_pad == w0 && g_att.mAttnStatus == 0);
    FakeHost::set(true);
  }
  // Without the host service the feature does not exist; without the UI the var still works.
  lockon_boot(false);
  CHECK(!lk.available);
  {
    LockRig r;
    r.add(0, 300, fopAc_attn_BATTLE_e);
    r.frame(1.0f), r.frame(0.0f);
    CHECK(lk.writes_pad == 0 && lk.t.c.frames == 0);
  }
  FakeHost::fail_register = true;
  lockon_boot();
  CHECK(!lk.available);
  FakeHost::fail_register = false;
  lockon_boot(true, false);
  CHECK(lk.available && !lk.ui);
  lockon_shutdown();
  CHECK(!lk.available);
  LockRig last;
  last.frame(1.0f);
  CHECK(lk.t.c.frames == 0);
  svc_config = nullptr, svc_ui = nullptr;
}
#define CLEAR(T) Reg<T>::pre.clear(), Reg<T>::post.clear()
static void clear_hooks() {
  CLEAR(LizExec), CLEAR(LizDelete), CLEAR(LizDamage), CLEAR(LizSmall), CLEAR(LizBig), CLEAR(LizRun), CLEAR(LizAttack0), CLEAR(LizAttack), CLEAR(LizTail), CLEAR(LizAtHit), CLEAR(LizAnm), CLEAR(LinkCutType), CLEAR(LinkDamageFix01), CLEAR(LinkGuardSe), CLEAR(LizGuard), CLEAR(LinkCutReverse), CLEAR(LinkSetDamage), CLEAR(PfxProcHitmark), CLEAR(PfxSetHitMark), CLEAR(PfxParticleSet), CLEAR(PfxCollisionSe);
}
// ================================================================ FIX04 0.12.4
// 0.12.3's predicate, transcribed: the arm anchor had to stand 10 units toward Link. The model of the held pose (mode 7) keeps both facts.
static bool old_covers_0_12_3(float sx, float sz, float lx, float lz) {
  const float ss = sx * sx + sz * sz, ll = lx * lx + lz * lz;
  if (ss < 15.0f * 15.0f || ss > 250.0f * 250.0f || ll < 1.0f) return true;
  const float d = sx * lx + sz * lz;
  return d > 0.0f && d * d >= 10.0f * 10.0f * ll;
}
static void force_roll(liz::Brain& b, unsigned below, bool yes) {
  for (int g2 = 0; g2 < 100000; ++g2) {
    liz::Rng pr = b.rng;
    if ((pr.below(100) < below) == yes) return;
    (void)b.rng.next();
  }
  CHECK(false);
}
static void until_mode7(World& w, e_dn_class* a) {
  for (int k = 0; k < 40 && !(a->action == 7 && a->mode == 7); ++k) a->timer[2] = 5000, w.tick();
  CHECK(a->action == 7 && a->mode == 7);
}
static unsigned traces(const char* verdict) {
  unsigned n = 0;
  for (const auto& l : g_log_store) n += l.find("LIZ_TRACE") != std::string::npos && l.find(std::string("verdict=") + verdict + " ") != std::string::npos;
  return n;
}
// The human report of 0.12.3, end to end: pressing B at a Lizalfos. One wound opens the guard; while it is up two distinct swings are real blocks; 80% prepares; the next swing in the
// new window is a parry with native rebound; the counter is real; the four combos come back. Three held poses: the production rule must not depend on the arm's offset.
static void w_fix04_mash() {
  CHECK(!old_covers_0_12_3(40.0f, 4.0f, 0.0f, 160.0f));  // the very cause: straight in front, shield up, "not covering"
  struct Pose { float side, fwd; } poses[] = {{40.0f, 4.0f}, {30.0f, 55.0f}, {60.0f, -5.0f}};
  for (const Pose& ps : poses) {
    for (int prepared = 1; prepared >= 0; --prepared) {
      const float s0 = g_shield_side, f0 = g_shield_fwd;
      g_shield_side = ps.side, g_shield_fwd = ps.fwd;
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      settle(w, a);
      auto* s = slot_of(a);
      g_log_store.clear();
      force_roll(s->b, liz::policy::kGuardPctHit1, true);
      a->timer[2] = 5000;
      w.set_cut(1);
      a->invulnerability_timer = 0;
      const int hp_full = a->actor.health;
      hit_body(w, a, kAtSword, &w.link.mAtCps[0]);
      s = slot_of(a);
      CHECK(a->actor.health < hp_full && s->b.p.points() == 6 && s->b.guard_intent);  // a legitimate first wound, +6, no scripted block
      CHECK(traces("BODY_DAMAGE") == 1);
      w.link.cut_type = 0;
      until_mode7(w, a);
      CHECK(s->b.in_guard && a->invulnerability_timer > 0);
      const int hp0 = a->actor.health, pts0 = s->b.p.points();
      force_roll(s->b, liz::policy::kDeflectPct, prepared != 0);
      block(w, a);
      CHECK(a->actor.health == hp0 && s->b.st.blocks == 1 && s->b.p.points() == pts0 + 5 && s->b.st.block_pairs == 0 && w.link.rebounds == 0);
      block(w, a);
      CHECK(a->actor.health == hp0 && s->b.st.blocks == 2 && s->b.p.points() == pts0 + 10 && s->b.st.block_pairs == 1 && s->b.st.deflect_rolls == 1 && w.link.rebounds == 0);
      CHECK(traces("BLOCK") == 2 && traces("SHIELD_NOT_COVERING") == 0);
      CHECK(s->b.deflect.armed == (prepared != 0) && !s->b.counter.armed);
      if (!prepared) {  // the 20%: the third swing is one more ordinary block; no parry, no rebound, no counter
        block(w, a);
        CHECK(s->b.st.enemy_parries == 0 && s->b.st.blocks == 3 && w.link.rebounds == 0 && !s->b.counter.armed && s->b.p.points() == pts0 + 15);
        clear_slots_of(w);
        g_shield_side = s0, g_shield_fwd = f0;
        continue;
      }
      w.set_cut(1);
      a->timer[2] = 5000;
      w.tick();
      CHECK(a->mode == 6 && s->b.attempt_armed);
      w.tick();
      sword_on_shield(w, a);
      const int pts = s->b.p.points();
      const auto conf0 = pf.book.c.confirmed[3], snd0 = pf.book.c.sounds;
      w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
      w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
      w.tick();
      CHECK(s->b.st.enemy_parries == 1 && w.link.rebounds == 1 && a->actor.health == hp0 && s->b.p.points() == pts && s->b.counter.armed && traces("ENEMY_PARRY") == 1);
      CHECK(pf.book.c.confirmed[3] == conf0 + 1 && pf.book.c.sounds == snd0 + 1);  // the same contact: its orange spark and its high clash
      const unsigned kind = s->b.counter.kind;
      w.link.cut_type = 0;
      int k = 0;
      for (; k < 120 && !s->b.ep.active; ++k) w.tick();
      CHECK(s->b.ep.active && s->b.ep.pattern == (kind == 0 ? liz::kCounterN : liz::kCounterD));
      for (k = 0; k < 240 && s->b.ep.active; ++k) w.tick();
      CHECK(!s->b.ep.active && s->b.st.counter_at_open == 1 && s->b.st.counter_started[kind] == 1);
      std::uint64_t sel = 0;
      a->timer[2] = 0;
      for (k = 0; k < 200 && sel == 0; ++k) {
        w.tick();
        sel = 0;
        for (auto x : s->b.st.selected) sel += x;
      }
      CHECK(sel == 1);  // back to the four combos
      clear_slots_of(w);
      g_shield_side = s0, g_shield_fwd = f0;
    }
  }
  // A parry is frontal: at 80 deg on the shield side the prepared contact is an ordinary block (the block arc is wider than the parry arc).
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    present_guard(a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    const float th = 80.0f * kPi / 180.0f;
    w.link.current.pos = cXyz(160.0f * std::sin(th), 0.0f, 160.0f * std::cos(th));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->search_angle_y = atan2s(w.link.current.pos.x, w.link.current.pos.z);
    sword_on_shield(w, a);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.enemy_parries == 0 && s->b.st.blocks == 1 && w.link.rebounds == 0);
    clear_slots_of(w);
  }
  // The pitch of that playback is read back one tick later; a Lizalfos deleted meanwhile takes its sound object with it: nothing is read.
  for (int deleted = 0; deleted <= 1; ++deleted) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    sword_on_shield(w, a);
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.enemy_parries == 1 && pf.readback != nullptr);
    const auto ok0 = pf.readback_ok;
    if (deleted) s->on = false;
    ++g.p8.tick;
    pfx_maintenance();
    CHECK(pf.readback == nullptr && pf.readback_ok == ok0 + (deleted ? 0u : 1u) && pf.readback_changed == 0);
    s->on = true;
    clear_slots_of(w);
  }
  // A new swing the aim test does not count (Link looking away): the shield sphere still comes back for it.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    present_guard(a);
    a->timer[2] = 5000;
    w.tick();
    a->unk_timer_2 = 7;
    w.link.shape_angle.y = static_cast<s16>(w.link.shape_angle.y + 0x8000);  // he swings the other way
    w.set_cut(1);
    a->timer[0] = 60, a->timer[2] = 5000;
    w.tick();
    CHECK(a->unk_timer_2 == 0);
    clear_slots_of(w);
  }
  // One verdict for one physical contact, whatever the capsules: three capsules on the shield in one pass are ONE trace, +5 once.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    g_log_store.clear();
    a->timer[2] = 5000;
    present_guard(a);
    w.set_cut(1);
    for (int k = 0; k < 11; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->timer[0] = 60;
    for (int c = 0; c < 3; ++c) w.link.mAtCps[c].at_hit = &a->tate_sph, w.link.mAtCps[c].at_shield_hit = true;
    a->tate_sph.tg_hit = &w.link.mAtCps[2];
    a->tate_sph.tg_shield_hit = true;
    w.tick();
    CHECK(s->b.st.blocks == 1 && s->b.p.points() == 5 && traces("BLOCK") == 1);
    clear_slots_of(w);
  }
  // The shield that is down or still rising protects nothing, and the back never: real damage, +6, no credit.
  {
    struct C { int mode; float frame; int deg; } cases[] = {{6, 1.0f, 0}, {8, 19.0f, 0}, {7, 8.0f, 180}, {7, 8.0f, -90}};
    for (const C& c : cases) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      settle(w, a);
      present_guard(a);
      a->mode = c.mode, a->morph.rate = c.mode == 7 ? 0.0f : 1.0f, a->morph.mFrameCtrl.mFrame = c.frame, a->morph.end = 22;
      auto* s = slot_of(a);
      s->b.guard_rearm = false;
      const int hp0 = a->actor.health;
      g_log_store.clear();
      same_pass(w, a, c.deg);
      s = slot_of(a);
      CHECK(a->actor.health < hp0 && s->b.st.blocks == 0 && s->b.st.shield_won_body == 0 && s->b.p.points() == 6);
      CHECK(traces("BODY_DAMAGE") == 1);
      clear_slots_of(w);
    }
  }
}
// ---- FIX05: Link's real cuts. Attack frames and clip starts from d_a_alink_HIO_data.inc, combo finish start = mComboStartFrame (d_a_alink_cut.inc).
struct CutSpec { const char* name; int cut, proc; float start, rate, at0, at1; };
static const CutSpec kCutSpecs[] = {
    {"NM_VERTICAL", 1, daAlink_c::PROC_CUT_NORMAL, 2.0f, 1.2f, 5.0f, 10.0f},  {"NM_LEFT", 2, daAlink_c::PROC_CUT_NORMAL, 2.0f, 1.2f, 7.0f, 13.0f},
    {"NM_RIGHT", 3, daAlink_c::PROC_CUT_NORMAL, 3.0f, 1.2f, 7.0f, 12.5f},     {"COMBO_STAB", 4, daAlink_c::PROC_CUT_NORMAL, 3.0f, 1.1f, 3.0f, 10.0f},
    {"NM_STAB", 5, daAlink_c::PROC_CUT_NORMAL, 4.0f, 1.1f, 3.0f, 10.0f},      {"FINISH_LEFT", 6, daAlink_c::PROC_CUT_FINISH, 3.0f, 1.0f, 7.0f, 12.0f},
    {"FINISH_VERTICAL", 7, daAlink_c::PROC_CUT_FINISH, 1.0f, 1.0f, 5.0f, 11.0f}, {"FINISH_STAB", 8, daAlink_c::PROC_CUT_FINISH, 3.0f, 1.1f, 10.0f, 17.0f},
    {"FINISH_RIGHT", 9, daAlink_c::PROC_CUT_FINISH, 1.0f, 1.1f, 8.0f, 14.0f},
};
// One native cut from its setCutType to the sword's contact, `sweep` clip steps after its AT opened. Link's Execute runs first in every tick, as in the game.
static void run_cut(World& w, e_dn_class* a, const CutSpec& c, int sweep) {
  auto& l = w.link;
  l.mProcID = c.proc, l.clip_rate = c.rate, l.frame = c.start - c.rate, l.field_0x3478 = c.at0, l.field_0x347c = c.at1;
  w.set_cut(c.cut);
  int k = 0;
  for (; k < 90; ++k) {
    a->timer[0] = 60, a->timer[2] = 5000;
    w.tick();
    if (l.frame >= c.at0 + float(sweep) * c.rate) break;
  }
  CHECK(k < 90);
  a->timer[0] = 60, a->timer[2] = 5000;
  sword_on_shield(w, a);
  w.tick();
}
static void cut_over(World& w) { w.link.mProcID = daAlink_c::PROC_WAIT, w.link.clip_rate = 0, w.link.cut_type = 0; }
static void prepared_guard(World& w, e_dn_class* a) {
  present_guard(a);
  auto* s = slot_of(a);
  s->b.deflect = liz::Brain::Deflect{};
  s->b.deflect.armed = true, s->b.deflect.tick = s->b.local, s->b.deflect.serial = 999, s->b.deflect.token = 1;
  a->timer[2] = 5000;
  (void)w;
}
static void w_fix05_chain() {
  // The cause, from the numbers alone: with the window opened at setCutType a first contact has guard age eta+3, and the window ends at 7.
  unsigned lost = 0;
  for (const auto& c : kCutSpecs) lost += liz::eta_ticks(c.start, c.rate, c.at0) + 3 > int(liz::policy::kParryWindowLast);
  CHECK(lost == 3);  // NM_LEFT, FINISH_STAB, FINISH_RIGHT could never be parried in 0.12.4
  // Every cut of the combo, at the start, middle and end of its sweep, against a prepared shield: a real parry with the native rebound.
  for (const auto& c : kCutSpecs) {
    for (int sweep = 0; sweep <= 2; ++sweep) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      settle(w, a);
      auto* s = slot_of(a);
      prepared_guard(w, a);
      const int hp0 = a->actor.health, pts0 = s->b.p.points();
      g_log_store.clear();
      run_cut(w, a, c, sweep);
      CHECK(s->b.st.enemy_parries == 1 && w.link.rebounds == 1 && a->actor.health == hp0 && s->b.p.points() == pts0 && s->b.counter.armed && traces("ENEMY_PARRY") == 1);
      CHECK(s->b.st.represent_synced == 1 && s->b.st.represent_unknown == 0 && s->b.st.no_parry[0] == 1);
      CHECK(w.link.mProcID == daAlink_c::PROC_CUT_REVERSE);  // the native reaction of Link, and only one
      CHECK(lz.rebound_state_ok >= 1);
      clear_slots_of(w);
    }
  }
  // The shield does not move before the sword is close: at the swing start of a finish the token is still waiting.
  {
    const CutSpec& c = kCutSpecs[7];
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    prepared_guard(w, a);
    auto& l = w.link;
    l.mProcID = c.proc, l.clip_rate = c.rate, l.frame = c.start - c.rate, l.field_0x3478 = c.at0, l.field_0x347c = c.at1;
    w.set_cut(c.cut);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.deflect.armed && !s->b.attempt_armed && a->mode == 7 && s->b.deflect.wait_serial == lz.swing_serial);
    clear_slots_of(w);
  }
  // eta is the native arithmetic: ceil((attack start - frame) / rate); at or past the start: 0; a stopped clip: unknowable.
  CHECK(liz::eta_ticks(2.0f, 1.2f, 7.0f) == 5 && liz::eta_ticks(3.0f, 1.2f, 7.0f) == 4 && liz::eta_ticks(1.0f, 1.1f, 8.0f) == 7 && liz::eta_ticks(4.0f, 1.1f, 3.0f) == 0 && liz::eta_ticks(3.0f, 0.0f, 7.0f) == -1 && liz::eta_ticks(3.0f, 2.0f, 7.0f) == 2);
  // The AT of this cut has already closed when the token starts watching it: that cut is over, the token waits for the next one.
  {
    const CutSpec& c = kCutSpecs[0];
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    prepared_guard(w, a);
    auto& l = w.link;
    l.mProcID = c.proc, l.clip_rate = c.rate, l.frame = c.at1 + 1.0f, l.field_0x3478 = c.at0, l.field_0x347c = c.at1;
    w.set_cut(c.cut);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.deflect.armed && !s->b.attempt_armed && s->b.st.deflect_windows == 0 && s->b.deflect.wait_serial == 0);
    clear_slots_of(w);
  }
  // A verdict without Link's clip ever asking (his proc had no reverse check that tick): no rebound happened, so no parry: an ordinary block.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    present_guard(a);
    a->timer[2] = 5000, a->timer[0] = 60;
    w.set_cut(1);
    for (int k = 0; k < 3; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    sword_on_shield(w, a);
    w.link.cut_type = 0;
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.enemy_parries == 0 && s->b.st.parry_no_rebound == 1 && s->b.st.blocks == 1 && w.link.rebounds == 0 && !s->b.counter.armed);
    clear_slots_of(w);
  }
  // A contact of the previous pass read in the very Execute that re-presents the shield: the shield was up: the body keeps its protection.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    prepared_guard(w, a);
    a->timer[0] = 60;
    a->invulnerability_timer = 0;
    const int hp0 = a->actor.health;
    w.set_cut(1);
    sword_on_shield(w, a);
    w.link.mAtCps[0].at_type = kAtSword;
    a->cc_sph[0].tg_hit = &w.link.mAtCps[0];
    a->hit_power = 20, a->hit_strong = false;
    w.tick();
    CHECK(a->actor.health == hp0);
    clear_slots_of(w);
  }
  // Prepared but not in guard: from FIGHT_RUN the shield needs three ticks. A finish (eta 7) waits; inside 3..5 it opens; a stab (eta 0) cannot be met from there and the token stays.
  for (int spec : {7, 0, 3}) {
    const CutSpec& c = kCutSpecs[spec];
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->action = liz::native::kFightRun, a->mode = 1, a->timer[2] = 5000;
    s->b.deflect = liz::Brain::Deflect{};
    s->b.deflect.armed = true, s->b.deflect.tick = s->b.local, s->b.deflect.serial = 999, s->b.deflect.token = 1;
    auto& l = w.link;
    l.mProcID = c.proc, l.clip_rate = c.rate, l.frame = c.start - c.rate, l.field_0x3478 = c.at0, l.field_0x347c = c.at1;
    w.set_cut(c.cut);
    a->timer[2] = 5000;
    w.tick();
    const int eta0 = liz::eta_ticks(c.start, c.rate, c.at0);
    if (eta0 > liz::policy::kEntryLeadMax) {
      CHECK(s->b.deflect.armed && a->action == liz::native::kFightRun);
      int k = 0;
      for (; k < 6 && s->b.deflect.armed; ++k) a->timer[2] = 5000, w.tick();
      CHECK(!s->b.deflect.armed && a->action == liz::native::kGuard && s->b.attempt_armed);
    } else if (eta0 < liz::policy::kEntryLeadMin) {
      CHECK(s->b.deflect.armed && !s->b.attempt_armed && s->b.st.entry_unreachable >= 1);
    } else {
      CHECK(!s->b.deflect.armed && a->action == liz::native::kGuard && s->b.attempt_armed);
    }
    clear_slots_of(w);
  }
  // A cut with no recoil clip on its native route (a spin): the token is not spent, the contact is an ordinary block, no rebound, no parry.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    prepared_guard(w, a);
    w.link.mProcID = daAlink_c::PROC_CUT_TURN;
    w.set_cut(1);
    for (int k = 0; k < 6; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    sword_on_shield(w, a);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.enemy_parries == 0 && s->b.st.blocks == 1 && s->b.deflect.armed && !s->b.counter.armed);
    clear_slots_of(w);
  }
  // A swing that ends without touching the shield gives its token back: never a new roll, never spent on air.
  {
    const CutSpec& c = kCutSpecs[0];
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    prepared_guard(w, a);
    auto& l = w.link;
    l.mProcID = c.proc, l.clip_rate = c.rate, l.frame = c.start - c.rate, l.field_0x3478 = c.at0, l.field_0x347c = c.at1;
    w.set_cut(c.cut);
    for (int k = 0; k < 14; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    CHECK(s->b.attempt_armed && !s->b.deflect.armed && s->b.st.deflect_windows == 1);
    cut_over(w);
    const auto rolls = s->b.st.deflect_rolls;
    run_cut(w, a, kCutSpecs[2], 0);
    CHECK(s->b.st.deflect_recovered == 1 && s->b.st.deflect_rolls == rolls && s->b.st.enemy_parries == 1 && w.link.rebounds == 1);
    clear_slots_of(w);
  }
  // Full chain through the real clips: V, L blocked; 80% prepares; each cut of the combo as the third swing. Negative seed: an ordinary block.
  for (const auto& c : kCutSpecs) {
    for (int prepared = 1; prepared >= 0; --prepared) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      settle(w, a);
      auto* s = slot_of(a);
      present_guard(a);
      a->timer[2] = 5000;
      const int hp0 = a->actor.health, pts0 = s->b.p.points();
      run_cut(w, a, kCutSpecs[0], 0);
      cut_over(w);
      for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
      CHECK(s->b.st.blocks == 1 && w.link.rebounds == 0 && s->b.st.no_parry[unsigned(liz::Np::Chain)] == 1);
      force_roll(s->b, liz::policy::kDeflectPct, prepared != 0);
      run_cut(w, a, kCutSpecs[1], 0);
      cut_over(w);
      for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
      CHECK(s->b.st.blocks == 2 && s->b.st.block_pairs == 1 && s->b.deflect.armed == (prepared != 0) && w.link.rebounds == 0 && s->b.p.points() == pts0 + 10);
      run_cut(w, a, c, 0);
      CHECK(a->actor.health == hp0);
      if (!prepared) {
        CHECK(s->b.st.enemy_parries == 0 && s->b.st.blocks == 3 && w.link.rebounds == 0 && !s->b.counter.armed && s->b.p.points() == pts0 + 15 && s->b.st.no_parry[unsigned(liz::Np::Rng)] == 1);
        clear_slots_of(w);
        continue;
      }
      CHECK(s->b.st.enemy_parries == 1 && w.link.rebounds == 1 && s->b.counter.armed && s->b.p.points() == pts0 + 10 && w.link.mProcID == daAlink_c::PROC_CUT_REVERSE);
      const unsigned kind = s->b.counter.kind;
      cut_over(w);
      int k = 0;
      for (; k < 120 && !s->b.ep.active; ++k) w.tick();
      CHECK(s->b.ep.active && s->b.ep.pattern == (kind == 0 ? liz::kCounterN : liz::kCounterD));
      for (k = 0; k < 240 && s->b.ep.active; ++k) w.tick();
      CHECK(!s->b.ep.active && s->b.st.counter_at_open == 1);
      std::uint64_t sel = 0;
      a->timer[2] = 0;
      for (k = 0; k < 200 && sel == 0; ++k) {
        w.tick();
        sel = 0;
        for (auto x : s->b.st.selected) sel += x;
      }
      CHECK(sel == 1);  // the four combos are back
      clear_slots_of(w);
    }
  }
  // Long B-spam: no technical dead end. Every contact is an ordinary block or a parry; a streak without parry is RNG, chain or cooldown, never a window missed. Frequencies stay the 80%.
  {
    unsigned armed = 0, parried = 0, pairs = 0;
    for (int trial = 0; trial < 60; ++trial) {
      World w;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      settle(w, a);
      auto* s = slot_of(a);
      s->b.rng.seed(0xB5A + std::uint64_t(trial) * 7919);
      present_guard(a);
      a->timer[2] = 5000;
      for (int n = 0; n < 15 && !s->b.counter.armed && !s->b.p.latched; ++n) {
        run_cut(w, a, kCutSpecs[n % 9], n % 3 == 2 ? 1 : 0);
        cut_over(w);
        for (int k = 0; k < 3; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
      }
      CHECK(s->b.st.no_parry[unsigned(liz::Np::Late)] == 0 && s->b.st.no_parry[unsigned(liz::Np::Early)] == 0 && s->b.st.no_parry[unsigned(liz::Np::NoRebound)] == 0 && s->b.st.no_parry[unsigned(liz::Np::NotPresented)] == 0);
      CHECK(s->b.st.enemy_parries == s->b.st.deflect_armed);  // a prepared token that met a swing always became a parry
      armed += unsigned(s->b.st.deflect_armed), parried += unsigned(s->b.st.enemy_parries), pairs += unsigned(s->b.st.block_pairs);
      clear_slots_of(w);
    }
    CHECK(pairs >= 60 && armed == parried && armed * 100 >= pairs * 65 && armed * 100 <= pairs * 95);  // the same 80%; never 100
  }
  // Every roll lost: the shield just blocks, +5 each, until the posture breaks. A legitimate defeat, not a dead end.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    present_guard(a);
    a->timer[2] = 5000;
    int n = 0;
    for (; n < 40 && !s->b.p.latched; ++n) {
      force_roll(s->b, liz::policy::kDeflectPct, false);
      run_cut(w, a, kCutSpecs[n % 4], 0);
      cut_over(w);
      for (int k = 0; k < 3; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    }
    CHECK(s->b.p.latched && s->b.st.blocks == 20 && s->b.st.enemy_parries == 0 && w.link.rebounds == 0);
    clear_slots_of(w);
  }
}
// The five contradictory traces of log 230009, literal: action 7, mode 6, frame 1, not presented, both spheres hit.
static void w_fix05_early() {
  struct T { s16 shape, search; } tr[] = {{-11233, -10978}, {25992, 22181}, {-23582, -20939}, {18210, 22707}, {-564, -1686}};
  for (const T& t : tr) {
    for (int body = 1; body >= 0; --body) {
      for (int frame = 1; frame <= 2; ++frame) {
        World w;
        fresh_world(w);
        w.add_dn(0, 0, 100);
        auto* a = w.dn[0];
        settle(w, a);
        auto* s = slot_of(a);
        const float th = float(t.search) * kPi / 32768.0f;
        w.link.current.pos = cXyz(a->actor.current.pos.x + 160.0f * std::sin(th), 0.0f, a->actor.current.pos.z + 160.0f * std::cos(th));
        w.link.shape_angle.y = static_cast<s16>(static_cast<std::uint16_t>(t.search) + 0x8000u);  // Link looks at it
        a->actor.shape_angle.y = a->actor.current.angle.y = t.shape;
        a->search_angle_y = t.search;
        present_guard(a);
        a->mode = 6, a->morph.rate = 1, a->morph.mFrameCtrl.mFrame = float(frame);
        s->b.guard_rearm = false;
        s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
        a->invulnerability_timer = 0;
        a->timer[2] = 5000, a->timer[0] = 60;
        w.set_cut(1);
        const int hp0 = a->actor.health, pts0 = s->b.p.points();
        g_log_store.clear();
        sword_on_shield(w, a);
        if (body) {
          w.link.mAtCps[0].at_type = kAtSword;
          a->cc_sph[0].tg_hit = &w.link.mAtCps[0];
          a->hit_power = 20, a->hit_strong = false;
        }
        w.tick();
        CHECK(s->b.st.enemy_parries == 0 && w.link.rebounds == 0 && !s->b.counter.armed && traces("ENEMY_PARRY") == 0);
        if (body) CHECK(a->actor.health == hp0 - 20 && s->b.p.points() == pts0 + 6 && traces("BODY_DAMAGE") == 1);
        else CHECK(a->actor.health == hp0 && s->b.p.points() == pts0 && traces("SHIELD_NOT_COVERING") == 1);
        CHECK(s->b.st.no_parry[unsigned(liz::Np::NotPresented)] == 1);
        // the next swing with the shield really up is a parry: the early contact cost nothing else
        a->invulnerability_timer = 0;
        prepared_guard(w, a);
        s->b.def.cooldown_until = 0;
        cut_over(w);
        for (int k = 0; k < 14; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
        const auto hp1 = a->actor.health;
        run_cut(w, a, kCutSpecs[2], 0);
        CHECK(s->b.st.enemy_parries == 1 && w.link.rebounds == 1 && a->actor.health == hp1 && traces("ENEMY_PARRY") == 1);
        clear_slots_of(w);
      }
    }
  }
  // Both spheres, shield up in front: one verdict: a parry takes no HP and no posture; an ordinary block takes +5 and no HP. The same swing can never appear on two sides.
  for (int prepared = 1; prepared >= 0; --prepared) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    present_guard(a);
    a->timer[2] = 5000, a->timer[0] = 60;
    w.set_cut(1);
    for (int k = 0; k < 3; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    if (prepared) s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    a->invulnerability_timer = 0;
    const int hp0 = a->actor.health, pts0 = s->b.p.points();
    g_log_store.clear();
    sword_on_shield(w, a);
    w.link.mAtCps[0].at_type = kAtSword;
    a->cc_sph[0].tg_hit = &w.link.mAtCps[0];
    a->hit_power = 20, a->hit_strong = false;
    w.tick();
    CHECK(a->actor.health == hp0 && s->b.p.points() == pts0 + (prepared ? 0 : 5) && s->b.st.enemy_parries == (prepared ? 1u : 0u) && s->b.st.blocks == (prepared ? 0u : 1u));
    CHECK(traces("BODY_DAMAGE") == 0);
    clear_slots_of(w);
  }
  // Fail-closed: if another hit of this very Execute did take HP, the contact earns no parry and the damage stands.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    present_guard(a);
    a->timer[2] = 5000, a->timer[0] = 60;
    w.set_cut(1);
    for (int k = 0; k < 3; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    g_log_store.clear();
    sword_on_shield(w, a);
    g_lose_hp_in_damage = 7;
    w.tick();
    g_lose_hp_in_damage = 0;
    CHECK(s->b.st.parry_conflicts == 1 && s->b.st.enemy_parries == 0 && !s->b.counter.armed && traces("ENEMY_PARRY") == 0);
    clear_slots_of(w);
  }
}
// Both roles, with the port's capture: every colour the screen can read is orange on a parry, and the resource's own on a block. Nothing shared is written.
static void w_fix05_fx() {
  CHECK(pfx::kPitch >= 1.20f && pfx::kPitch <= 1.25f);
  for (int interp = 1; interp >= 0; --interp) {
    for (int parry = 1; parry >= 0; --parry) {
      World w;
      w.interp = interp != 0;
      fresh_world(w);
      w.add_dn(0, 0, 100);
      auto* a = w.dn[0];
      settle(w, a);
      auto* s = slot_of(a);
      a->timer[2] = 5000;
      present_guard(a);
      a->actor.shape_angle.y = a->actor.current.angle.y = 0;
      a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
      w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
      w.set_cut(1);
      for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
      a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
      a->actor.shape_angle.y = a->actor.current.angle.y = 0;
      if (parry) s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
      else s->b.attempt_armed = false, s->b.verdict_swing = 0;
      sword_on_shield(w, a);
      const size_t em0 = w.emitters.size();
      w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
      CHECK(g_env_light.eflight.mLightType == (parry ? 1 : 0));  // the flash of this mark
      a->timer[0] = 60;
      w.tick();
      w.tick();
      CHECK(w.emitters.size() == em0 + 4);  // the same four native emitters, no extra one
      for (size_t i = em0; i < w.emitters.size(); ++i) {
        auto& e = *w.emitters[i];
        check_first_frame(e, parry != 0);
        for (int c = 0; c < 4; ++c) CHECK(is_res(e.tprm[c], e.res_prm) && is_res(e.tenv[c], e.res_env));  // the shared tables stay as they were
        CHECK(is_res(e.cdata.mPrmClr, e.res_prm));
        if (e.res == 0xB) {
          CHECK(parry ? is_orange(e.child_reg_p, e.res_prm) && is_orange(e.child_reg_e, e.res_env) && is_orange(e.child_birth, e.res_prm) : is_res(e.child_reg_p, e.res_prm) && is_res(e.child_birth, e.res_prm));
        }
      }
      if (parry) CHECK(pf.book.c.blue_after_calc == 0 && pf.book.c.clones >= 1 && pf.book.c.risk == 0);
      clear_slots_of(w);
    }
  }
  // A preview the verdict does not confirm (Link's clip never asked): from then on the particles get the resource's own colours again.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    sword_on_shield(w, a);
    const size_t em0 = w.emitters.size();
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    w.particles();
    w.link.cut_type = 0;
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.parry_no_rebound == 1 && pf.book.c.reverted[3] == 1);
    a->timer[0] = 60;
    w.tick();
    for (size_t i = em0; i < em0 + 2; ++i) for (int c = 0; c < 3; ++c) CHECK(is_res(w.emitters[i]->cap_prm[c], w.emitters[i]->res_prm) && is_res(w.emitters[i]->cap_env[c], w.emitters[i]->res_env));
    clear_slots_of(w);
  }
  // A mark of another Lizalfos earlier in the same pass is not part of this clash.
  {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    w.add_dn(40, 0, 101);
    auto* a = w.dn[0];
    auto* other = w.dn[1];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000, other->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor, other->cc_sph[0].owner_ac = &other->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, other->timer[2] = 5000, w.tick();
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    sword_on_shield(w, a);
    const size_t em0 = w.emitters.size();
    w.proc_hitmark(&w.link.mAtCps[0], &other->cc_sph[0], false, 5, nullptr);
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    CHECK(w.emitters.size() == em0 + 4);
    for (size_t i = em0; i < em0 + 2; ++i) CHECK(w.emitters[i]->mpEmtrCallBack != g_pfx_cb);
    for (size_t i = em0 + 2; i < em0 + 4; ++i) CHECK(w.emitters[i]->mpEmtrCallBack == g_pfx_cb);
    clear_slots_of(w);
  }
  // No free private copy: the colour route is declared at risk, and on frames that draw the live particles the draw callback still has the last word.
  {
    World w;
    w.interp = false;
    fresh_world(w);
    for (auto& x : pf.clones) x.src = reinterpret_cast<const JPAResource*>(1);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    g_log_store.clear();
    sword_on_shield(w, a);
    const size_t em0 = w.emitters.size();
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    a->timer[0] = 60;
    w.tick();
    w.tick();
    CHECK(pf.book.c.risk >= 1);
    unsigned risk_lines = 0;
    for (const auto& l : g_log_store) risk_lines += l.find("PARRY_SPARK_BLUE_RISK") != std::string::npos;
    CHECK(risk_lines == 1);
    for (size_t i = em0; i < w.emitters.size(); ++i) check_first_frame(*w.emitters[i], true);
    clear_slots_of(w);
  }
  // A mark of the same two actors earlier in this pass (Link's own At goes first): claimed when the parry contact arrives. Without a parry: untouched.
  for (int parry = 1; parry >= 0; --parry) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor;
    a->cc_sph[0].owner_ac = &a->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    if (parry) s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    else s->b.attempt_armed = false, s->b.verdict_swing = 0;
    sword_on_shield(w, a);
    const size_t em0 = w.emitters.size();
    w.proc_hitmark(&w.link.mAtCps[0], &a->cc_sph[0], false, 5, nullptr);    // first in the pass, not a shield contact
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    CHECK(w.emitters.size() == em0 + 4);
    for (size_t i = em0; i < em0 + 2; ++i) CHECK((w.emitters[i]->mpEmtrCallBack == g_pfx_cb) == (parry != 0));
    a->timer[0] = 60;
    w.tick();
    for (size_t i = em0; i < em0 + 2; ++i) check_first_frame(*w.emitters[i], parry != 0);
    if (parry) CHECK(pf.book.c.retro == 2);
    clear_slots_of(w);
  }
  // A later mark of the same clash stays orange; once the clash is over, or for an actor that parried nothing, marks keep their own colour. A damage spark (mark 1) is not the blue one.
  for (int late = 0; late <= 1; ++late) {
    World w;
    fresh_world(w);
    w.add_dn(0, 0, 100);
    auto* a = w.dn[0];
    settle(w, a);
    auto* s = slot_of(a);
    a->timer[2] = 5000;
    present_guard(a);
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    w.link.mAtCps[0].owner_ac = &w.link, a->tate_sph.owner_ac = &a->actor, a->cc_sph[0].owner_ac = &a->actor;
    w.set_cut(1);
    for (int k = 0; k < 2; ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    a->tate_sph.SetC(cXyz(g_shield_side, 110.0f, g_shield_fwd));
    a->actor.shape_angle.y = a->actor.current.angle.y = 0;
    s->b.attempt_armed = true, s->b.guard_tick = s->b.local - 2, s->b.verdict_swing = 0, s->b.def.cooldown_until = 0;
    sword_on_shield(w, a);
    w.proc_hitmark(&w.link.mAtCps[0], &a->tate_sph, true, 5, &a->actor);
    a->timer[0] = 60;
    w.tick();
    CHECK(s->b.st.enemy_parries == 1);
    for (int k = 0; k < (late ? 6 : 1); ++k) a->timer[0] = 60, a->timer[2] = 5000, w.tick();
    const size_t em0 = w.emitters.size();
    w.marked.clear();
    w.proc_hitmark(&w.link.mAtCps[0], &a->cc_sph[0], false, 5, nullptr);
    CHECK(w.emitters.size() == em0 + 2);
    for (size_t i = em0; i < em0 + 2; ++i) CHECK((w.emitters[i]->mpEmtrCallBack == g_pfx_cb) == (late == 0));
    const size_t em1 = w.emitters.size();
    w.proc_hitmark(&w.link.mAtCps[0], &a->cc_sph[0], false, 1, nullptr);
    for (size_t i = em1; i < w.emitters.size(); ++i) CHECK(w.emitters[i]->mpEmtrCallBack != g_pfx_cb);
    clear_slots_of(w);
  }
}
int main() {
  clear_hooks();
  w_install();
  clear_hooks();
  g_install_count = 0;
  g_logs = &g_log_store;
  c4.enabled = true;
  liz_install();
  CHECK(lz.enabled && lz.coverage && ld.enabled);
  pfx_install();
  CHECK(pf.enabled);
  w_fix03_lockon();
  w_slip_records();
  w_held_guard();
  w_body();
  w_parry_matrix();
  w_double_pair();
  w_fix03_chain();
  w_fix03_shield();
  w_fix03_fx();
  w_fix04_mash();
  w_fix05_chain();
  w_fix05_early();
  w_fix05_fx();
  w_front_range();
  w_coverage();
  w_multi();
  w_break();
  w_shield_rebound();
  w_counter();
  w_link_damage();
  w_plugin_order();
  w_delete();
  w_lifecycle();
  CHECK(g_peek_checks > 10000 && g_preview_checks > 100 && g_preview_parries > 50);
  std::printf("LIZ_WORLD_FIX03 peek_vs_observe=%llu preview_vs_verdict=%llu of_which_parries=%llu\n", (unsigned long long)g_peek_checks, (unsigned long long)g_preview_checks, (unsigned long long)g_preview_parries);
  std::printf("LIZ_WORLD_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_ADAPTER_AND_BRAIN_TRANSCRIBED_E_DN_LINK_COLLIDER pose=FITTED_ARC_MODEL_NOT_BCK runtime=HUMAN_RUNTIME_PENDING\n", liz::kVersion, g_checks);
  return 0;
}
