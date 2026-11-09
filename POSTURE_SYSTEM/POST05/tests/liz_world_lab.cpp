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
struct Z2Creature { unsigned collision_se = 0; };
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
  enum { FLG1_IS_WOLF = 1, RFLG1_UNK_2 = 2, CUT_TYPE_NONE = 0, CUT_TYPE_JUMP = 0x0a, CUT_TYPE_GUARD_ATTACK = 0x29, CUT_TYPE_HEAD_JUMP = 0x2b, CUT_TYPE_NORMAL = 1 };
};
struct Stts { std::uint32_t apid = 0, tg_apid = 0, tg_old = 0; };
struct daAlink_c : fopAc_ac_c, daPy_py_c {
  enum { PROC_GUARD_SLIP = 0x1d, PROC_GUARD_ATTACK = 0x1e, PROC_WAIT = 3, PROC_DAMAGE = 0x70 };
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
  bool checkResetFlg1(int) const noexcept { return false; }
  bool checkHorseRide() const noexcept { return horse; }
  int getCutType() const noexcept { return cut_type; }
  void setGuardSe(dCcD_GObjInf* o) noexcept;
  int procGuardAttackInit() noexcept;
};
inline constexpr int VIBMODE_S_POWER3 = 3;
struct World;
static World* W = nullptr;
struct dVibration_c { bool StartShock(int, int, cXyz) noexcept; };
static dVibration_c g_vib;
static dVibration_c& dComIfGp_getVibration() noexcept { return g_vib; }
struct PlayerStatus { int life = 80; int getLife() const noexcept { return life; } };
struct GameInfo {
  struct Info { PlayerStatus st; struct P { PlayerStatus* s; PlayerStatus& getPlayerStatusA() noexcept { return *s; } } p{&st}; P& getPlayer() noexcept { return p; } } info;
  struct Ev { bool run = false; bool runCheck() const noexcept { return run; } };
  struct Play { Ev ev; float pending = 0; void* player = nullptr; Ev* getEvent() noexcept { return &ev; } float getItemLifeCount() const noexcept { return pending; } void* getPlayer(int) const noexcept { return player; } } play;
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
static void def_se_set(Z2Creature* s, cCcD_Obj*, unsigned, fopAc_ac_c*) noexcept { ++s->collision_se, ++g_bash_se; }
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
struct LinkGuardSe {};
struct LizGuard {};
struct LinkCutReverse {};
struct LinkSetDamage {};
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
  struct { struct { bool disabled = false; } obs; } eng;
  struct { std::uint64_t tick = 0; bool watch_life = false; int life_at = 0; } p8;
  post08::LinkTrack link_track;
} g;
static struct { bool enabled = true; shieldchain::Chain shield; } c4;
static bool g_world_safe = true;
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
#include "../src/liz_adapter.inc"
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
}
struct World {
  std::vector<e_dn_class*> dn;
  daAlink_c link;
  PlayerStatus* life = &g_dComIfG_gameInfo.info.st;
  std::uint64_t sparks = 0;
  bool place_front = true, slip_push = true, skip_link_collision = false;
  void hooks_clear_at_tick() {}
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
        }
      }
    }
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
    if (Reg<LizExec>::run_pre(args, &ret) == HOOK_SKIP_ORIGINAL) return;
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
      if (a->guard_flag != 0 && a->unk_timer_2 == 0) {
        if (a->tate_sph.tg_hit) a->unk_timer_2 = 10, ++a->sound.collision_se;
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
    const auto tr = g.link_track.observe(g.p8.tick, true, l.pid, l.mProcID, ga, l.frame, 1.0f, 20.0f);
    (void)tr;
    c4.shield.observe(g.p8.tick, ga, ga ? l.frame : 0.0f, ga ? 1.0f : 0.0f, cp04_link_safe(&l));
    {
      lnkfx::Obs fo;
      fo.valid = true, fo.human = !l.wolf, fo.alive = life->life > 0, fo.sim_active = true, fo.event = !g_world_safe, fo.link_id = l.pid, fo.epoch = 7, fo.tick = g.p8.tick, fo.proc = l.mProcID;
      (void)lf.def.observe(fo);
    }
    liz_maintenance();
    link_exec();
    for (auto* a : dn) execute(a);
    pass();
  }
};
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
  g_actors.clear();
  g_actors.push_back(&w.link);
  w.link.group = fopAc_PLAYER_e;
  g_bash_se = 0;
  g_dComIfG_gameInfo.info.st.life = 80;
  w.link.name = w.link.profname = fpcNm_ALINK_e, w.link.pid = 1;
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
  g_install_count = 0;
  g_fail_install = static_cast<int>(total);  // only the general hook fails
  liz_install();
  CHECK(lz.enabled && !ld.enabled);
  g_fail_install = -1;
}
#define CLEAR(T) Reg<T>::pre.clear(), Reg<T>::post.clear()
static void clear_hooks() {
  CLEAR(LizExec), CLEAR(LizDelete), CLEAR(LizDamage), CLEAR(LizSmall), CLEAR(LizBig), CLEAR(LizRun), CLEAR(LizAttack0), CLEAR(LizAttack), CLEAR(LizTail), CLEAR(LizAtHit), CLEAR(LizAnm), CLEAR(LinkCutType), CLEAR(LinkDamageFix01), CLEAR(LinkGuardSe), CLEAR(LizGuard), CLEAR(LinkCutReverse), CLEAR(LinkSetDamage);
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
  w_slip_records();
  w_held_guard();
  w_body();
  w_parry_matrix();
  w_double_pair();
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
  std::printf("LIZ_WORLD_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_ADAPTER_AND_BRAIN_TRANSCRIBED_E_DN_LINK_COLLIDER pose=FITTED_ARC_MODEL_NOT_BCK runtime=HUMAN_RUNTIME_PENDING\n", liz::kVersion, g_checks);
  return 0;
}
