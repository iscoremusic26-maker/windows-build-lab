// LIGHT FADES TWICE 0.13.0 CHILFOS01 integration laboratory: the PRODUCTION adapter (src/kk_adapter.inc, included verbatim) and core, registered through its
// own kk_install(), run against a transcription of the native paths it touches (TwilitRealm/dusklight@40457c6 src/d/actor/d_a_e_kk.cpp: execute, action,
// damage_check, executeWait/Attack/SpearThrow/BackWalk/Yoroke/Guard/Damage/Dead, nextActionCheck, setBck/setWeaponBck) and of the collision pass between
// Link and the Chilfos (sword AT on the body cylinder with or without TgShield, the Chilfos spheres on Link's Tg cylinder with or without his shield).
// Declared limits: clip lengths and the spear-tip arcs are a MODEL (no BCK is read); no BG beyond a wall/cliff flag; Link is a scripted actor; the bridge to the
// shared FIX05 systems is stubbed and recorded. A laboratory, not a game run: no FPS, sound or visual claim.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../src/kk_core.hpp"
static unsigned long long g_checks = 0;
#define CHECK(c)                                                 \
  do {                                                           \
    ++g_checks;                                                  \
    if (!(c)) {                                                  \
      std::printf("KK_WORLD_LAB_FAIL line=%d %s\n", __LINE__, #c); \
      std::fflush(stdout);                                       \
      std::exit(1);                                              \
    }                                                            \
  } while (0)
using u8 = std::uint8_t;
using s16 = std::int16_t;
using f32 = float;
using BOOL = int;
inline constexpr int FALSE = 0, TRUE = 1;
inline constexpr unsigned kLogLineMax = 512;
// ---------------------------------------------------------------- SDK surface the adapter touches
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
enum ModResult { MOD_OK, MOD_FAIL };
struct ModContext {};
struct HookOptions { int priority = 0; };
#define HOOK_OPTIONS_INIT {}
using PreFn = HookAction (*)(ModContext*, void*, void*, void*);
using PostFn = void (*)(ModContext*, void*, void*, void*);
template <class T> struct Reg {
  static inline std::vector<PreFn> pre;
  static inline std::vector<PostFn> post;
};
static int g_fail_install = -1, g_install_count = 0;
namespace mods {
template <class T> T arg(void* args, int i) { return *static_cast<T*>(static_cast<void**>(args)[i]); }
template <class T> T& arg_ref(void* args, int i) { return *static_cast<T*>(static_cast<void**>(args)[i]); }
namespace hook {
template <class T> ModResult add_pre(PreFn f, HookOptions* = nullptr) {
  if (g_install_count++ == g_fail_install) return MOD_FAIL;
  Reg<T>::pre.push_back(f);
  return MOD_OK;
}
template <class T> ModResult add_post(PostFn f, HookOptions* = nullptr) {
  if (g_install_count++ == g_fail_install) return MOD_FAIL;
  Reg<T>::post.push_back(f);
  return MOD_OK;
}
}  // namespace hook
}  // namespace mods
template <class T> bool run_pre(void** a, void* r = nullptr) {
  for (auto f : Reg<T>::pre)
    if (f(nullptr, a, r, nullptr) == HOOK_SKIP_ORIGINAL) return false;
  return true;
}
template <class T> void run_post(void** a, void* r = nullptr) {
  for (auto f : Reg<T>::post) f(nullptr, a, r, nullptr);
}
struct cXyz { float x = 0, y = 0, z = 0; };
struct csXyz { s16 x = 0, y = 0, z = 0; };
inline constexpr std::uint32_t AT_TYPE_SHIELD_ATTACK = 0x40000, kAtSword = 0x2, kAtIron = 0x4, kAtArrow = 0x8, kAtBomb = 0x10;
struct fopAc_ac_c;
struct cCcD_Obj;
struct cCcD_ObjHitInf {
  cCcD_Obj* at_hit = nullptr;
  cCcD_Obj* tg_hit = nullptr;
  bool ChkAtHit() const noexcept { return at_hit != nullptr; }
  cCcD_Obj* GetAtHitObj() const noexcept { return at_hit; }
  bool ChkTgHit() const noexcept { return tg_hit != nullptr; }
  cCcD_Obj* GetTgHitObj() const noexcept { return tg_hit; }
  void ClrTgHit() noexcept { tg_hit = nullptr; }
  void ClrAtHit() noexcept { at_hit = nullptr; }
};
struct cCcD_Obj : cCcD_ObjHitInf {
  std::uint32_t at_type = 0;
  fopAc_ac_c* owner = nullptr;
  int power = 2;          // sword damage of this collider (cc_at_check)
  bool strong = false;    // at_power_check mHitStatus != 0
  std::uint8_t atp = 2;   // LFT 0.13.6: the collider's own AtAtp (Link's cuts 2, the Jump Strike's airborne/landing hits 4)
  std::uint8_t GetAtAtp() const noexcept { return atp; }
  bool ChkAtType(std::uint32_t m) const noexcept { return (at_type & m) != 0; }
  fopAc_ac_c* GetAc() noexcept { return owner; }
};
struct dCcD_GObjInf : cCcD_Obj {
  bool at_shield_hit = false, tg_shield = false;
  int eff = 0, clr_at = 0;  // dCcD_GAtTgCoCommonBase::mEffCounter of its AT (5 after a hit mark, -1 per ClrAtHit)
  void ClrAtHit() noexcept {  // dCcD_GObjInf::ClrAtHit (virtual in the SDK): hit, shield-hit and one step of the AT effect counter
    at_hit = nullptr, at_shield_hit = false, ++clr_at;
    if (eff > 0) --eff;
  }
  bool ChkAtShieldHit() const noexcept { return at_shield_hit; }
  int tg_shield_on_calls = 0;
  void OnTgShield() noexcept { tg_shield = true, ++tg_shield_on_calls; }
  bool ChkTgShield() noexcept { return tg_shield; }
  void OffTgShield() noexcept { tg_shield = false; }
};
struct dCcD_Sph : dCcD_GObjInf {};
struct dCcD_Cyl : dCcD_GObjInf {};
// FIX05: J3DFrameCtrl transcribed (JSystem/J3DGraphAnimator/J3DAnimation.cpp, update(); modes 0 NONE, 2 LOOP used by E_KK) with its real fields
struct J3DFrameCtrl {
  u8 mAttribute = 0, mState = 0;
  s16 mStart = 0, mEnd = 50, mLoop = 0;
  float mRate = 1, mFrame = 0;
  void update() noexcept {
    mState = 0;
    mFrame += mRate;
    if (mAttribute == 0) {
      if (mFrame < mStart) mFrame = mStart, mRate = 0.0f, mState |= 1;
      if (mFrame >= mEnd) mFrame = mEnd - 0.001f, mRate = 0.0f, mState |= 1;
    } else if (mAttribute == 2) {
      while (mFrame < mStart) {
        mState |= 2;
        if (mLoop - mStart <= 0.0f) break;
        mFrame += mLoop - mStart;
      }
      while (mFrame >= mEnd) {
        mState |= 2;
        if (mEnd - mLoop <= 0.0f) break;
        mFrame -= mEnd - mLoop;
      }
    }
  }
};
struct mDoExt_McaMorfSO {
  J3DFrameCtrl mFrameCtrl;
  float getFrame() const noexcept { return mFrameCtrl.mFrame; }
  float getPlaySpeed() const noexcept { return mFrameCtrl.mRate; }
  float getEndFrame() const noexcept { return mFrameCtrl.mEnd; }
  bool isStop() const noexcept { return (mFrameCtrl.mState & 1) || mFrameCtrl.mRate == 0.0f; }  // m_Do_ext.h
  void play() noexcept { mFrameCtrl.update(); }
};
enum { fopAc_attn_BATTLE_e = 0 };
inline constexpr int fpcNm_E_KK_e = 0x1F0, fpcNm_ALINK_e = 0x2;
struct AttnInfo { std::uint8_t distances[4] = {0x4C, 0, 0, 0}; };
struct fopAc_ac_c {
  int name = 0;
  std::uint32_t pid = 0;
  s16 health = 0, field_0x560 = 0;
  cXyz eyePos{};
  struct { cXyz pos; csXyz angle; } current{};
  csXyz shape_angle{};
  AttnInfo attention_info{};
  bool headlock = false;
  cXyz mHeadLockPos{};
  cXyz& getHeadLockPos() noexcept { return mHeadLockPos; }
  void offHeadLockFlg() noexcept { headlock = false; }
  void onHeadLockFlg() noexcept { headlock = true; }
  bool checkHeadLockFlg() const noexcept { return headlock; }
};
inline std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) noexcept { return a->pid; }
inline int fopAcM_GetName(const fopAc_ac_c* a) noexcept { return a->name; }
inline s16 cM_atan2s(float x, float z) noexcept { return s16(std::lround(std::atan2(x, z) * 32768.0 / 3.14159265358979)); }
struct daPy_py_c {
  // the SDK numbering (include/d/actor/d_a_player.h); FIX02 fixed HEAD_JUMP (0x05: 0x15 is FINISH_RIGHT, a chain finisher the 0.13.1 log shows)
  enum { CUT_TYPE_NONE = 0, CUT_TYPE_VERTICAL = 1, CUT_TYPE_HEAD_JUMP = 0x05, CUT_TYPE_FINISH_LEFT = 0x06, CUT_TYPE_FINISH_VERTICAL = 0x07, CUT_TYPE_TURN_RIGHT = 0x08,
         CUT_TYPE_JUMP = 0x0a, CUT_TYPE_LARGE_JUMP = 0x13, CUT_TYPE_LARGE_JUMP_FINISH = 0x14, CUT_TYPE_FINISH_RIGHT = 0x15, CUT_TYPE_TURN_LEFT = 0x16,
         CUT_TYPE_LARGE_TURN_LEFT = 0x17, CUT_TYPE_LARGE_TURN_RIGHT = 0x18, CUT_TYPE_MORTAL_DRAW_A = 0x1A, CUT_TYPE_TWIRL = 0x1E, CUT_TYPE_MORTAL_DRAW_B = 0x1F,
         CUT_TYPE_FINISH_STAB = 0x20, CUT_TYPE_COMBO_STAB = 0x21, CUT_TYPE_GUARD_ATTACK = 0x29 };
  enum { FLG1_IS_WOLF = 1 };
};
struct daAlink_c : fopAc_ac_c {
  enum { PROC_CUT_NORMAL = 0x20, PROC_CUT_FINISH = 0x21, PROC_CUT_REVERSE = 0x25, PROC_CUT_TURN = 0x26, PROC_CUT_JUMP = 0x27, PROC_CUT_JUMP_LAND = 0x28, PROC_CUT_HEAD = 0x2D,
         PROC_WAIT = 3 };
  int mProcID = PROC_WAIT, cut = 0;
  float mNormalSpeed = 0.0f;  // LFT 0.13.6: read only (the landing recoil receipt)
  fopAc_ac_c* mTargetedActor = nullptr;
  csXyz mBodyAngle{};
  bool wolf = false;
  dCcD_Cyl mTgCyls[3];
  dCcD_Sph mAtCps[3];
  int getCutType() const noexcept { return cut; }
  bool checkNoResetFlg1(int) const noexcept { return wolf; }
  int cut_count = 0;
  int procCutFinishInit(int type);  // the native one: commonProcInit(PROC_CUT_FINISH) + setCutType(cutParamTable[type]) + its own AT (transcribed below)
};
// ---------------------------------------------------------------- the native Chilfos (transcribed)
inline constexpr std::uint32_t Z2SE_HIT_SWORD = 0x40000;
struct Z2Creature {  // Z2Creature::startCollisionSE is virtual in the SDK (Z2AudioLib/Z2Creature.h)
  int collision_se = 0;
  std::uint32_t last_id = 0, last_map = 0;
  virtual ~Z2Creature() = default;
  virtual void* startCollisionSE(std::uint32_t id, std::uint32_t map) { ++collision_se, last_id = id, last_map = map; return nullptr; }
};
struct World;
extern World W;
struct daE_KK_c : fopAc_ac_c {
  mDoExt_McaMorfSO morf, wmorf;
  mDoExt_McaMorfSO* mpMorfSO = &morf;
  mDoExt_McaMorfSO* mpWeaponMorfSO = &wmorf;
  int mActionMode = 0, mMoveMode = 0;
  s16 mTimer = 0, field_0x672 = 0;
  u8 mDamageTimer = 0, field_0x679 = 0, field_0x67c = 0, field_0x67d = 0, field_0x67e = 1, field_0x76c = 0;
  int field_0x764 = -1, field_0x768 = -1;
  cXyz field_0x68c{};
  float speedF = 0, disp = 0;  // disp: ground travel along current.angle.y (fopAcM_posMoveF), signed
  Z2Creature mCreatureSound;
  std::vector<int> acts;        // mActionMode after every action()
  dCcD_Cyl mCyl;
  dCcD_Sph mSph, mSpheres[6];
  bool deleted = false, wall = false, cliff = false, crash_se = false;
  int se_attack01 = 0, se_attack03 = 0, se_restore = 0, ice = 0, guard_se7 = 0, hit_se40 = 0;
  void setBck(int i, u8 attr, f32 morfv, f32 rate);
  void setWeaponBck(int i, u8 attr, f32 morfv, f32 rate);
  void nextActionCheck();
  void damage_check();
  void executeAttack();
  void executeSpearThrow();
  void executeBackWalk();
  void executeDamage();
  void executeWait();
  void executeYoroke();
  void executeGuard();
  void executeDead();
  void action();
  int execute();
  int _delete();
  void setActionMode(int a, int m) { mActionMode = a, mMoveMode = m; }
  bool mCutTypeCheck(int p);
  int child_ttl = 0;  // a thrown spear child (field_0x679 == 1): ticks left before the native deletes it
  // FIX04 lab bookkeeping (not native state): its AT windows as the collision pass saw them, and the ones in which the spear touched Link
  bool lab_live_prev = false;
  int lab_window = 0, lab_contact_window = -1, lab_contact_windows = 0, lab_marks = 0;
  int bck_sets = 0;  // FIX05: native setBck calls that reached setAnm
  float lab_dead_from = 150.0f;
};
struct KkExec {}; struct KkDelete {}; struct KkDamage {}; struct KkAttack {}; struct KkSetBck {}; struct KkSetWeaponBck {}; struct KkBack {}; struct KkThrow {}; struct KkDmgExec {};
struct KkNext {}; struct LinkCutType {}; struct LinkCutReverse {}; struct LinkDamageFix01 {}; struct KkCutCheck {}; struct LinkCutAction {}; struct MeterDrawPost07 {};
struct PfxProcHitmark {};  // dCcS::ProcAtTgHitmark (FIX04: the module's PRE runs after the parry FX PRE)
// ---------------------------------------------------------------- world
struct World {
  std::uint64_t tick = 1;
  daAlink_c link;
  std::vector<daE_KK_c*> kk;
  bool safe = true, link_first = true;
  // Link's scripted state
  bool guarding = false, guard_attack = false, ga_in_window = false, link_damage_wait = false, status_400 = false;
  std::uint64_t swing_start = 0;
  int swing_len = 0;
  bool swing_can_rebound = true, sword_strong = false;
  std::uint32_t sword_type = kAtSword;
  bool shield_attack_hit = false;
  // records
  std::vector<std::string> log;
  struct Gain { int pts; bool block; std::uint32_t id; };
  std::vector<Gain> gains;
  int parry_confirms = 0, blocked_calls = 0, burst_gaps = 0, spawns = 0, link_hits = 0, rng_n = 0;
  int settles[3][2] = {};  // [link_on_kk/kk_on_link][parry]
  bool direct_pending = false;
  std::uint32_t direct_id = 0;
  std::uint64_t direct_token = 0;
  bool reverse_pending = false;
  int reverse_rebounds = 0, reverse_suppressed = 0, air_rebounds = 0, land_rebounds = 0, land_recoil_fix06 = 0;
  // models
  bool tip_swapped = false;
  float reach = 240;
  std::uint64_t behind_at = 0;  // Link reaches the Chilfos' back right before this tick's collision pass
  std::vector<std::uint64_t> native_opens;  // ticks in which the native executeAttack itself set field_0x67e = 0
  std::vector<int> native_open_bck;
  std::vector<float> native_open_rate;  // FIX02: the clip rate at every native frame-24 opening
  // FIX03
  bool spawn_children = false;
  int children_spawned = 0, finish_inits = 0, last_finish = -1;
  std::int64_t qpc = 1000000, ft = 5000000000ll;
  std::int64_t frame_qpc = 166667;  // 60 fps presented frames
  int spike_frames = 0;
  std::int64_t spike_qpc = 0;
  // FIX04
  int contact_spheres = 0;              // shaft spheres (mSpheres[5], [4], ...) that touch Link with the tip in one pass
  int link_tg_eff = 0, marks = 0, marks_skipped = 0, mark_calls = 0;
  float floor_y = 0;                    // the ground under both: a BG line from below it to Link crosses it
  bool wall_between = false;
  int kcl = 1;                          // KCL BG elements every LineCross walks
  std::uint64_t los_strides = 0, los_calls = 0, los_max = 0;
  int native_guard_marks = 0, guard_marks_mod = 0, clash_se = 0;
  float eye_after_throw = 150.0f, eye_after_throw_min = 1e9f;
  int eye_probe_calls = 0;
  std::uint32_t clash_map = 7;
  std::vector<std::string> seq;         // ordered events of one damage_check: marks and settles
};
World W;
// ---------------------------------------------------------------- bridge stubs (production: src/kk_bridge.inc)
struct KkPass { bool valid = false, guard_attack = false, in_window = false; std::uint64_t serial = 0; float delta = 0; };
struct KkGain { bool confirmed = false, saturated = false; int before = 0, after = 0; };
std::uint64_t kkb_tick() noexcept { return W.tick; }
daAlink_c* kkb_link() noexcept { return &W.link; }
bool kkb_world_safe() noexcept { return W.safe; }
std::uint32_t kkb_stage() noexcept { return 7; }
void kkb_write(bool, const char* line) noexcept { W.log.emplace_back(line); }
bool kkb_take(unsigned& lines, std::uint64_t& dropped, unsigned cap) noexcept {
  if (lines >= cap) return ++dropped, false;
  return ++lines, true;
}
std::uint32_t kkb_sword_mask() noexcept { return kAtSword; }
std::uint32_t kkb_iron_mask() noexcept { return kAtIron; }
std::int64_t kkb_qpc() noexcept { return W.qpc; }  // the lab's simulated wall clock: 10 MHz QPC, 100 ns file time
std::int64_t kkb_filetime() noexcept { return W.ft; }
KkPass kkb_link_pass(daAlink_c*) noexcept {
  KkPass p;
  p.valid = true, p.guard_attack = W.guard_attack, p.in_window = W.guard_attack && W.ga_in_window, p.serial = 77;
  return p;
}
void kkb_link_parry_confirmed(std::uint32_t, std::uint64_t) noexcept { ++W.parry_confirms; }
void kkb_link_blocked(std::uint32_t, bool) noexcept { ++W.blocked_calls; }
void kkb_burst_gap(std::uint32_t) noexcept { ++W.burst_gaps; }
static int g_link_posture = 0;
KkGain kkb_link_gain(int points, bool block, std::uint32_t id, std::uint64_t, std::uint64_t) noexcept {
  W.gains.push_back({points, block, id});
  KkGain r;
  r.confirmed = true, r.before = g_link_posture * 1000;
  g_link_posture = std::min(100, g_link_posture + points);
  r.after = g_link_posture * 1000, r.saturated = g_link_posture >= 100;
  return r;
}
void kkb_pfx_begin(daE_KK_c*, std::uint32_t) noexcept {}
void kkb_pfx_end() noexcept {}
void kkb_pfx_settle(bool link_on_kk, std::uint32_t, bool parry) noexcept {
  ++W.settles[link_on_kk ? 0 : 1][parry ? 1 : 0];
  if (parry) W.seq.push_back(link_on_kk ? "SETTLE_LINK_PARRY" : "SETTLE_ENEMY_PARRY");
}
void kkb_guard_mark(daE_KK_c*) noexcept { ++W.guard_marks_mod, W.seq.push_back("MOD_MARK"); }
bool kkb_strong_hit(daAlink_c*, const cCcD_Obj* o) noexcept { return W.status_400 || (o && o->strong); }
void kkb_pfx_clash_map(std::uint32_t m) noexcept { W.clash_map = m; }
void kkb_direct_offer(unsigned, std::uint32_t id, std::uint64_t, std::uint64_t token, const void*) noexcept { W.direct_pending = true, W.direct_id = id, W.direct_token = token; }
bool kkb_direct_take(unsigned slot, std::uint32_t& id, std::uint64_t& token) noexcept {
  if (!W.direct_pending || slot != 0) return false;
  W.direct_pending = false, id = W.direct_id, token = W.direct_token;
  return true;
}
void kkb_direct_capture(daAlink_c*) noexcept {}
bool kkb_hud(unsigned, std::uintptr_t, std::uint32_t, std::uint64_t, int, bool, bool, bool, fopAc_ac_c*, bool& busy) noexcept { return busy = false, true; }
#include "../src/kk_adapter.inc"
// ---------------------------------------------------------------- native bodies
static float dist_link(const daE_KK_c* a) { return std::hypot(W.link.current.pos.x - a->current.pos.x, W.link.current.pos.z - a->current.pos.z); }
static s16 angle_to_link(const daE_KK_c* a) { return cM_atan2s(W.link.current.pos.x - a->current.pos.x, W.link.current.pos.z - a->current.pos.z); }
static float clip_end(int i) {
  switch (i) {
    case 5: case 6: return 50;
    case 0x18: return 70;
    case 0x12: case 0x13: return 45;
    case 9: case 0xB: return 40;
    case 0xF: return 35;
    case 0x10: case 0x11: return 25;
    case 0xC: case 0xD: return 30;
    case 8: return 40;
    case 0xE: return 50;
    default: return 60;
  }
}
void daE_KK_c::setBck(int i, u8 attr, f32 morfv, f32 rate) {
  daE_KK_c* me = this;
  void* a[] = {&me, &i, &attr, &morfv, &rate};
  if (!run_pre<KkSetBck>(a)) return;
  if (i == 0xE) lab_dead_from = eyePos.y - current.pos.y;
  morf.mFrameCtrl = J3DFrameCtrl{};
  morf.mFrameCtrl.mFrame = 0, morf.mFrameCtrl.mRate = rate, morf.mFrameCtrl.mAttribute = attr, morf.mFrameCtrl.mEnd = s16(clip_end(i));
  ++bck_sets;
  field_0x764 = i;
  run_post<KkSetBck>(a);
}
void daE_KK_c::setWeaponBck(int i, u8 attr, f32 morfv, f32 rate) {
  daE_KK_c* me = this;
  void* a[] = {&me, &i, &attr, &morfv, &rate};
  if (!run_pre<KkSetWeaponBck>(a)) return;
  wmorf.mFrameCtrl = J3DFrameCtrl{};
  wmorf.mFrameCtrl.mFrame = 0, wmorf.mFrameCtrl.mRate = rate, wmorf.mFrameCtrl.mAttribute = attr, wmorf.mFrameCtrl.mEnd = s16(i == 0x1E ? 45 : 40);
  field_0x768 = i;
  run_post<KkSetWeaponBck>(a);
}
template <class Tag, class F> void hooked(daE_KK_c* k, F body) {
  daE_KK_c* me = k;
  void* a[] = {&me};
  if (run_pre<Tag>(a)) body();
  run_post<Tag>(a);
}
static bool native_cut_check(int p) {  // daE_KK_c::mCutTypeCheck, transcribed whole (d_a_e_kk.cpp)
  const int c = W.link.cut;
  using P = daPy_py_c;
  if (p == 0 && W.link.cut_count >= 4) return true;
  if (p != 2 && (c == P::CUT_TYPE_TURN_RIGHT || c == P::CUT_TYPE_JUMP || c == P::CUT_TYPE_TWIRL || c == P::CUT_TYPE_TURN_LEFT)) return true;
  if (c == P::CUT_TYPE_LARGE_JUMP || c == P::CUT_TYPE_LARGE_JUMP_FINISH || c == P::CUT_TYPE_LARGE_TURN_LEFT || c == P::CUT_TYPE_LARGE_TURN_RIGHT || c == P::CUT_TYPE_MORTAL_DRAW_A ||
      c == P::CUT_TYPE_MORTAL_DRAW_B)
    return true;
  return c == P::CUT_TYPE_HEAD_JUMP || c == P::CUT_TYPE_TWIRL;
}
bool daE_KK_c::mCutTypeCheck(int p) {  // hooked like the real export: PRE may answer and skip, POST sees the answer
  daE_KK_c* me = this;
  int pp = p;
  bool r = false;
  void* a[] = {&me, &pp};
  if (run_pre<KkCutCheck>(a, &r)) r = native_cut_check(pp);
  run_post<KkCutCheck>(a, &r);
  return r;
}
void daE_KK_c::nextActionCheck() {
  hooked<KkNext>(this, [&] {
    const float d = dist_link(this);
    if (W.safe && d > 900 && d <= 1500) return setActionMode(3, 0);
    if (W.safe && d <= 900) {
      if (W.link_damage_wait && mActionMode != 0) return setActionMode(0, 0);
      return setActionMode(8, 0);
    }
    if (field_0x672 == 0 && mActionMode != 0) setActionMode(0, 0);
  });
}
void daE_KK_c::damage_check() {
  hooked<KkDamage>(this, [&] {
    if (health <= 1 || field_0x679 == 1 || mActionMode == 10) return;
    if (mDamageTimer != 0) return;
    if (mSph.ChkAtHit() && mSph.ChkAtShieldHit()) ++hit_se40, mSph.ClrAtHit(), mDamageTimer = 8, speedF = -70;
    for (auto& s : mSpheres)
      if (s.ChkAtHit() && s.ChkAtShieldHit()) ++hit_se40, s.ClrAtHit(), mDamageTimer = 8, speedF = -70;
    if (field_0x67d == 0 && dist_link(this) <= 900) {  // the native auto-deflect: mCutTypeCheck(0)
      if (mCutTypeCheck(0)) mCyl.OnTgShield();
      else mCyl.OffTgShield();
    }
    if (mCyl.ChkTgHit()) {
      cCcD_Obj* o = mCyl.GetTgHitObj();
      current.angle.y = shape_angle.y;
      mDamageTimer = 8;
      if (o->ChkAtType(kAtArrow)) {
        setActionMode(5, 0);
      } else if (o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
        onHeadLockFlg();
        if (field_0x67d == 0) mCreatureSound.startCollisionSE(Z2SE_HIT_SWORD, 0x28);
        setActionMode(5, field_0x67d == 0 ? 1 : 0);
      } else if ((o->ChkAtType(kAtIron) && !W.status_400) || mCutTypeCheck(2)) {
        health = 0;
        setActionMode(10, 0);
      } else {
        ++ice;
        if (field_0x67d == 0) {
          if (o->strong || W.status_400) ++W.clash_se, setActionMode(7, 0);  // def_se_set(.., 0x1F): the clash SE, no hit mark
          else ++guard_se7, ++W.native_guard_marks, W.seq.push_back("NATIVE_MARK"), setActionMode(6, 0);  // startCollisionSE(.., 7) + setHitMark(2, this)
        } else {
          health = s16(health - o->power);  // cc_at_check
          if (health <= 1 || o->ChkAtType(kAtBomb)) {
            if (field_0x67d == 0) crash_se = true;
            setActionMode(10, 0);
            mCyl.ClrTgHit();
            return;
          }
          setActionMode(7, 1);
        }
      }
      mCyl.ClrTgHit();
    }
  });
}
void daE_KK_c::executeWait() {
  if (mMoveMode == 0) {
    if (dist_link(this) <= 900) setBck(0x1A, 2, 3, 1), mTimer = 30;
    else setBck(0x19, 2, 10, 1), field_0x672 = 30;
    mMoveMode = 1;
  } else if (field_0x672 == 0 && !W.link_damage_wait) {
    nextActionCheck();
  }
}
void daE_KK_c::executeAttack() {
  hooked<KkAttack>(this, [&] {
    switch (mMoveMode) {
      case 0: field_0x67c = 0, mMoveMode = 2; return;
      case 2: setBck((W.rng_n++ & 1) ? 6 : 5, 0, 3, 1), ++mMoveMode; return;
      case 3:
        if (int(morf.getFrame()) == 0x18) {
          W.native_opens.push_back(W.tick), W.native_open_bck.push_back(field_0x764), W.native_open_rate.push_back(morf.mFrameCtrl.mRate);  // every native execution of the frame-24 branch
          speedF = 80, field_0x67e = 0;
          if (field_0x764 == 6) ++se_attack03;
        }
        if (field_0x764 == 5 && int(morf.getFrame()) == 0x1B) ++se_attack01;
        if (morf.isStop()) nextActionCheck();
        break;
    }
    if (int(morf.getFrame()) < 0x19) shape_angle.y = angle_to_link(this);
    else if (int(morf.getFrame()) >= 0x21) field_0x67e = 1;
  });
}
void daE_KK_c::executeSpearThrow() {
  struct EyeProbe { daE_KK_c* a; ~EyeProbe() { W.eye_after_throw = a->eyePos.y, W.eye_after_throw_min = std::min(W.eye_after_throw_min, a->eyePos.y), W.eye_probe_calls++; } } probe{this};  // the eye the rest of this Execute sees (FIX05: cc_set rewrites it after)
  hooked<KkThrow>(this, [&] {
    switch (mMoveMode) {
      case 0:
        shape_angle.y = angle_to_link(this);
        [[fallthrough]];
      case 1:
        shape_angle.y = angle_to_link(this);
        setBck(0x18, 0, 3, 1);
        mMoveMode = 2;
        return;
      case 2: {
        // the native melee cancel, with the cost of its fopAcM_otherBgCheck BG lines (dBgWKCol::LineCheck: one stride per 500 units, every KCL)
        auto los = [&]() {
          const float dx = W.link.current.pos.x - current.pos.x, dy = W.link.current.pos.y + 100.0f - eyePos.y, dz = W.link.current.pos.z - current.pos.z;
          const std::uint64_t n = kk::los_strides(float(std::sqrt(double(dx) * dx + double(dy) * dy + double(dz) * dz))) * std::uint64_t(W.kcl);
          W.los_strides += n, ++W.los_calls, W.los_max = std::max(W.los_max, n);
          return W.wall_between || (eyePos.y < W.floor_y && W.link.current.pos.y + 100.0f > W.floor_y);  // LineCross hit
        };
        if (W.safe && dist_link(this) <= 900 && !los() && int(morf.getFrame()) < 0x17)
          if (!los() && eyePos.y + 50 > W.link.current.pos.y) return setActionMode(8, 0);
      }
        if (int(morf.getFrame()) == 0x17) {
          ++W.spawns, field_0x67d = 1;
          if (W.spawn_children) {  // fopAcM_createChild(fpcNm_E_KK_e, ..., 0xFF0001): a real E_KK actor, field_0x679 = 1, its own lifetime
            auto* c = new daE_KK_c();
            static std::uint32_t next_child_pid = 90000;  // process ids are unique for the whole game session
            ++W.children_spawned;
            c->name = fpcNm_E_KK_e, c->pid = ++next_child_pid, c->field_0x679 = 1, c->health = 140, c->field_0x560 = 140, c->child_ttl = 40;
            c->current.pos = current.pos, c->setActionMode(9, 0);
            W.kk.push_back(c);
          }
        }
        if (morf.isStop()) {
          mMoveMode = 4;
          setBck(0x12, 0, 0, 1);
          setWeaponBck(0x1E, 0, 0, 1);
        }
        return;
      case 4:
        if (int(wmorf.getFrame()) >= 0x22) {
          if (field_0x67d != 0) ++se_restore;
          field_0x67d = 0, field_0x76c = 0;
        }
        if (morf.isStop() && wmorf.isStop()) field_0x67d = 0, nextActionCheck();
        return;
    }
  });
}
void daE_KK_c::executeBackWalk() {
  hooked<KkBack>(this, [&] {
    switch (mMoveMode) {
      case 0:
        setBck(7, 2, 3, 1);
        if (field_0x76c == 0) mTimer = 100, field_0x76c = 1;
        mMoveMode = wall ? 2 : 1;
        return;
      case 1:
        current.angle.y = s16(angle_to_link(this) + 0x8000);  // native: walks away from Link, the shape keeps facing him
        [[fallthrough]];
      case 2:
        speedF = 15;
        if (mTimer == 0 || wall || cliff) setBck(0x13, 0, 0, 1), setWeaponBck(0x1E, 0, 0, 1), speedF = 0, mMoveMode = 3;
        return;
      case 3:
        if (int(wmorf.getFrame()) >= 0x22) {
          if (field_0x67d != 0) ++se_restore;
          field_0x67d = 0;
        }
        if (morf.isStop() && wmorf.isStop()) field_0x67d = 0, ++mMoveMode, nextActionCheck();
        return;
    }
  });
}
void daE_KK_c::executeYoroke() {
  switch (mMoveMode) {
    case 0: setBck(field_0x67d == 0 ? 9 : 0xB, 0, 3, 1), mMoveMode = 2; return;
    case 1: setBck(0xF, 0, 3, 1), mMoveMode = 2; return;
    case 2:
      if (morf.isStop()) {
        offHeadLockFlg();
        if (field_0x67d == 0) return nextActionCheck();
        setActionMode(4, 0);
      }
  }
}
void daE_KK_c::executeGuard() {
  if (mMoveMode == 0) setWeaponBck(0x1D, 0, 0, 1), setBck(0x10, 0, 3, 1), mMoveMode = 1;
  else if (morf.isStop()) nextActionCheck();
  current.angle.y = kk::turn_toward(current.angle.y, angle_to_link(this), 8, 0x500), shape_angle.y = current.angle.y;  // cLib_addCalcAngleS2(.., 8, 0x500)
}
void daE_KK_c::executeDamage() {
  hooked<KkDmgExec>(this, [&] {
    switch (mMoveMode) {
      case 0: setBck(8, 0, 3, 1), speedF = -80, field_0x67d = 1, mMoveMode = 2; return;
      case 1: setBck(0xC, 0, 3, 1), mMoveMode = 3; return;
      default:
        if (morf.isStop()) setActionMode(4, 0);
    }
  });
}
void daE_KK_c::executeDead() {
  if (mMoveMode == 0) {
    health = 0, field_0x67d = 1, offHeadLockFlg();
    setBck(0xE, 0, 3, 1), ++mMoveMode;
  } else if (morf.isStop()) {
    deleted = true;
  }
}
void daE_KK_c::action() {
  damage_check();
  switch (mActionMode) {
    case 0: executeWait(); break;
    case 3: executeSpearThrow(); break;
    case 4: executeBackWalk(); break;
    case 5: executeYoroke(); break;
    case 6: executeGuard(); break;
    case 7: executeDamage(); break;
    case 8: executeAttack(); break;
    case 10: executeDead(); break;
  }
  if (mActionMode != 8) field_0x67e = 1;
  if (mActionMode != 4 && mActionMode != 2) {  // cLib_addCalc0(&speedF, 0.7f, 10.0f)
    const float step = std::min(std::fabs(speedF) * 0.7f, 10.0f);
    speedF = speedF > 0 ? std::max(0.0f, speedF - step) : std::min(0.0f, speedF + step);
  }
  disp += speedF;  // fopAcM_posMoveF along current.angle.y
  acts.push_back(mActionMode);
  morf.play();
  if (field_0x67d == 0 || field_0x768 == 0x1E) {
    wmorf.play();
    if (field_0x768 == 0x1E && wmorf.isStop()) setWeaponBck(0x1D, 0, 0, 1);
  }
}
static void tip_model(daE_KK_c* a) {  // MODEL of the spear tip in the actor frame: bck 6 thrusts, bck 5 sweeps a diagonal (swapped when tip_swapped)
  const float f = a->morf.getFrame();
  const int bck = a->field_0x764;
  float lx = 0, ly = 150, lz = 120;
  const bool thrust = (bck == 6) != W.tip_swapped;
  if (bck == 5 || bck == 6) {
    const float t = f < 18 ? 0.0f : f > 33 ? 1.0f : (f - 18.0f) / 15.0f;
    if (thrust) lz += 160.0f * t;
    else lx += 140.0f * t, ly -= 120.0f * t, lz += 20.0f * t;
  }
  const float yaw = float(a->shape_angle.y) * 3.14159265f / 32768.0f;
  const float s = std::sin(yaw), c = std::cos(yaw);
  a->field_0x68c = {a->current.pos.x + lx * c + lz * s, a->current.pos.y + ly, a->current.pos.z - lx * s + lz * c};
}
// FIX05: MODEL of cc_set's eyePos (head joint 4 - 50) above current.pos: upright 150; the stagger clip bows it (lowest in its middle, back up over its last
// 30 %: the clip ends in the native stance); the dead clip lays it on the ground. setHeadLockPos(&eyePos) follows it (the Helm Splitter aim point).
float g_head_upright = 150.0f, g_head_bowed = 55.0f;
static float head_rel(const daE_KK_c* a) {
  const float f = a->morf.getFrame(), e = a->morf.getEndFrame();
  const int b = a->field_0x764;
  if ((b == 9 || b == 0xB) && e > 0) {
    const float t = f / e;
    const float bow = t < 0.15f ? t / 0.15f : t < 0.7f ? 1.0f : std::max(0.0f, (1.0f - t) / 0.3f);
    return g_head_upright - (g_head_upright - g_head_bowed) * bow;
  }
  if (b == 0xE) return a->lab_dead_from - (a->lab_dead_from - 15.0f) * std::min(1.0f, f / 20.0f);  // from the pose it died in (morf), down to the ground
  return g_head_upright;
}
int daE_KK_c::execute() {
  int r = 1;
  daE_KK_c* me = this;
  void* a[] = {&me};
  if (run_pre<KkExec>(a)) {
    if (mTimer) --mTimer;
    if (field_0x672) --field_0x672;
    if (mDamageTimer) --mDamageTimer;
    action();
    tip_model(this);
    if (field_0x679 != 1) eyePos.y = current.pos.y + head_rel(this), mHeadLockPos = eyePos;  // cc_set
  }
  run_post<KkExec>(a, &r);
  return r;
}
int daE_KK_c::_delete() {
  daE_KK_c* me = this;
  void* a[] = {&me};
  run_pre<KkDelete>(a);
  return 1;
}
// ---------------------------------------------------------------- Link and the collision pass
static dCcD_Sph g_sword_obj;
static dCcD_Sph g_bash_obj;
static void link_swing(int cut = daPy_py_c::CUT_TYPE_VERTICAL, int len = 6, int proc = daAlink_c::PROC_CUT_NORMAL) {
  W.link.cut = cut, W.link.mProcID = proc, W.swing_start = W.tick, W.swing_len = len;
  daAlink_c* k = &W.link;
  u8 t = u8(cut);
  void* a[] = {&k, &t};
  run_post<LinkCutType>(a);
}
int daAlink_c::procCutFinishInit(int type) {  // d_a_alink_cut.inc: cutParamTable[type] -> PROC_CUT_FINISH, setCutType, its own sword AT
  static const int cuts[6] = {daPy_py_c::CUT_TYPE_FINISH_LEFT, daPy_py_c::CUT_TYPE_FINISH_VERTICAL, daPy_py_c::CUT_TYPE_FINISH_STAB, daPy_py_c::CUT_TYPE_MORTAL_DRAW_A,
                              daPy_py_c::CUT_TYPE_MORTAL_DRAW_B, daPy_py_c::CUT_TYPE_FINISH_RIGHT};
  ++W.finish_inits, W.last_finish = type;
  link_swing(cuts[type], 8, PROC_CUT_FINISH);
  return 1;
}
// The native checkCutAction of a B press with the sword out: commonCutAction declined, an ordinary cut is chosen, then this module's POST runs.
static int link_cut_action(int cut = daPy_py_c::CUT_TYPE_VERTICAL) {
  link_swing(cut, 6, daAlink_c::PROC_CUT_NORMAL);
  daAlink_c* k = &W.link;
  int ret = 1;
  void* a[] = {&k};
  run_post<LinkCutAction>(a, &ret);
  return ret;
}
static bool sword_active() { return W.swing_len > 0 && W.tick >= W.swing_start + 2 && W.tick < W.swing_start + 2 + W.swing_len; }
static void link_exec() {
  if (W.reverse_pending) {  // checkCutReverseAt for the shield contact of the last pass (only on procs with a recoil clip)
    W.reverse_pending = false;
    daAlink_c* k = &W.link;
    dCcD_GObjInf* o = &W.link.mAtCps[0];
    BOOL r = TRUE;
    void* a[] = {&k, &o};
    run_post<LinkCutReverse>(a, &r);
    if (r && W.link.mProcID == daAlink_c::PROC_CUT_JUMP) ++W.reverse_rebounds, ++W.air_rebounds, W.link.shape_angle.y = s16(W.link.shape_angle.y + 0x8000);  // procCutJump: heading reversed, speed 27, no proc change
    else if (r) ++W.reverse_rebounds, W.land_rebounds += W.link.mProcID == daAlink_c::PROC_CUT_JUMP_LAND, W.link.mProcID = daAlink_c::PROC_CUT_REVERSE;
    else ++W.reverse_suppressed;
  } else if (W.link.mProcID == daAlink_c::PROC_CUT_JUMP_LAND) {
    // LFT 0.13.6: procCutJumpLand's FIRST statement every tick: changeCutReverseProc(ANM_CUT_RECOIL_B) asks checkCutReverseAt(&mAtCps[0]) even without a contact
    daAlink_c* k = &W.link;
    dCcD_GObjInf* o = &W.link.mAtCps[0];
    BOOL r = FALSE;
    void* a[] = {&k, &o};
    run_post<LinkCutReverse>(a, &r);
    if (r) ++W.reverse_rebounds, ++W.land_rebounds, ++W.land_recoil_fix06, W.link.mProcID = daAlink_c::PROC_CUT_REVERSE;  // procCutReverseInit(ANM_CUT_RECOIL_B)
  }
}
// dCcS::SetAtTgGObjInf -> ProcAtTgHitmark for one AT/TG pair: the effect counters gate the call, the hooks may skip the original (the hit mark)
static void hitmark(daE_KK_c* a, dCcD_GObjInf& at) {
  dCcD_GObjInf& tg = W.link.mTgCyls[0];
  tg.owner = &W.link;
  if (at.eff && W.link_tg_eff) return;  // `!atObjInf->ChkAtEffCounter() || !tgObjInf->ChkTgEffCounter()`
  at.eff = 5, W.link_tg_eff = 5;
  ++W.mark_calls;
  void* self = nullptr;
  bool set_at = true, set_tg = true, shield = at.at_shield_hit;
  cCcD_Obj* ato = &at;
  cCcD_Obj* tgo = &tg;
  dCcD_GObjInf* ati = &at;
  dCcD_GObjInf* tgi = &tg;
  void* none = nullptr;
  void* args[13] = {&self, &set_at, &set_tg, &ato, &tgo, &ati, &tgi, &none, &none, &none, &none, &none, &shield};
  if (run_pre<PfxProcHitmark>(args)) ++W.marks;
  else ++W.marks_skipped;
  (void)a;
}
static void pass() {
  if (W.link_tg_eff > 0) --W.link_tg_eff;  // Link's TG is registered every pass: ClrTgHit steps its counter
  for (auto* a : W.kk) {
    if (a->deleted || a->mActionMode == 10) continue;
    // cc_set registered the spear's AT this frame only with the spear in hand and the native window open; cCcS::ClrAtHitInf clears REGISTERED AT
    // objects only: the flags of a window that closed on a contact stay as they were (stale) until the next window
    const bool live = a->field_0x67d == 0 && a->field_0x67e == 0;
    if (live) {
      a->mSph.ClrAtHit();
      for (auto& s : a->mSpheres) s.ClrAtHit();
    }
    a->mCyl.at_shield_hit = false;
    const float d = dist_link(a);
    if (W.shield_attack_hit && d <= W.reach) {
      g_bash_obj.at_type = AT_TYPE_SHIELD_ATTACK, g_bash_obj.owner = &W.link;
      a->mCyl.tg_hit = &g_bash_obj;
    } else if (sword_active() && d <= W.reach) {
      dCcD_Sph& sw = W.link.mAtCps[0];
      sw.at_type = W.sword_type, sw.owner = &W.link, sw.strong = W.sword_strong, sw.power = 2;
      a->mCyl.tg_hit = &sw;
      if (a->mCyl.tg_shield) {
        sw.at_hit = &a->mCyl, sw.at_shield_hit = true;
        if (W.link.mProcID == daAlink_c::PROC_CUT_NORMAL || W.link.mProcID == daAlink_c::PROC_CUT_FINISH || W.link.mProcID == daAlink_c::PROC_CUT_JUMP ||
            W.link.mProcID == daAlink_c::PROC_CUT_JUMP_LAND)
          W.reverse_pending = true;
      }
      W.swing_len = 0;  // one contact per swing (Link's own no-continuous-hit)
    }
    if (live && !a->lab_live_prev) ++a->lab_window;
    a->lab_live_prev = live;
    if (live && a->mActionMode == 8 && d <= W.reach) {  // the tip (mSph, radius 40) and, deeper in, the shaft spheres nearest to it
      if (a->lab_contact_window != a->lab_window) a->lab_contact_window = a->lab_window, ++a->lab_contact_windows;
      auto touch = [&](dCcD_GObjInf& x) {
        x.owner = a, x.at_hit = &W.link.mTgCyls[0], x.at_shield_hit = W.guarding;
        const int m0 = W.marks;
        hitmark(a, x);
        a->lab_marks += W.marks - m0;
      };
      touch(a->mSph);
      for (int j = 0; j < W.contact_spheres && j < 6; ++j) touch(a->mSpheres[5 - j]);
      if (!W.guarding) ++W.link_hits;
    }
  }
  W.shield_attack_hit = false;
}
static void present_frame() {  // FIX03: one presented frame (dMeter2_c::_draw POST) on the simulated wall clock
  const std::int64_t d = W.frame_qpc + (W.spike_frames > 0 ? W.spike_qpc : 0);
  if (W.spike_frames > 0) --W.spike_frames;
  W.qpc += d, W.ft += d;
  void* a[] = {nullptr};
  run_post<MeterDrawPost07>(a);
}
static void step(int n = 1) {
  for (int i = 0; i < n; ++i) {
    if (W.link_first) link_exec();
    for (std::size_t k = 0; k < W.kk.size(); ++k) {  // by index: a thrown spear child may be created during this pass
      daE_KK_c* a = W.kk[k];
      if (a->deleted) continue;
      a->execute();
      if (a->field_0x679 == 1 && a->child_ttl > 0 && --a->child_ttl == 0) a->_delete(), a->deleted = true;  // the native end of a thrown spear
    }
    if (!W.link_first) link_exec();
    if (W.behind_at && W.tick == W.behind_at) W.link.current.pos = {0, 0, -180};
    pass();
    ++W.tick;  // kk_maintenance runs inside the module's own Execute PRE, once per tick, as in production
    present_frame(), present_frame();  // 60 presented frames per 30 Hz simulation tick
  }
}
// ---------------------------------------------------------------- scenario helpers
static daE_KK_c* spawn(std::uint32_t id, u8 sub = 0, float z = 0) {
  auto* a = new daE_KK_c();
  a->name = fpcNm_E_KK_e, a->pid = id, a->field_0x679 = sub, a->health = 140, a->field_0x560 = 140;
  a->current.pos = {0, 0, z};
  a->eyePos = {0, 150, z};
  a->setActionMode(0, 0);
  W.kk.push_back(a);
  return a;
}
static void reset_world() {
  for (auto* a : W.kk) a->_delete(), delete a;
  W.kk.clear();
  for (auto& s : kz.slots) kk_retire(s);
  kz.totals = kk::Stats{};
  for (auto& l : kz.lines) l = 0;
  W.log.clear(), W.gains.clear();
  W.parry_confirms = W.blocked_calls = W.burst_gaps = W.spawns = W.link_hits = 0;
  std::memset(W.settles, 0, sizeof(W.settles));
  W.guarding = W.guard_attack = W.ga_in_window = W.link_damage_wait = false;
  W.status_400 = false;
  W.swing_len = 0, W.safe = true, W.link_first = true, W.tip_swapped = false, W.sword_strong = false, W.sword_type = kAtSword;
  W.link = daAlink_c{};
  W.link.name = fpcNm_ALINK_e, W.link.pid = 1, W.link.current.pos = {0, 0, 180}, W.link.shape_angle.y = s16(0x8000);
  W.reverse_pending = false, W.reverse_rebounds = W.reverse_suppressed = W.air_rebounds = W.land_rebounds = W.land_recoil_fix06 = 0;
  kk::f6::g_land_recoil = kk::f6::LandRecoil{};
  W.direct_pending = false;
  g_link_posture = 0;
  W.spawn_children = false, W.children_spawned = 0, W.finish_inits = 0, W.last_finish = -1, W.spike_frames = 0;
  W.contact_spheres = 0, W.link_tg_eff = 0, W.marks = W.marks_skipped = W.mark_calls = 0, W.floor_y = 0, W.wall_between = false, W.kcl = 1;
  W.los_strides = W.los_calls = W.los_max = 0, W.native_guard_marks = W.guard_marks_mod = W.clash_se = 0, W.clash_map = 7;
  W.seq.clear();
}
static int count_log(const char* part) {
  int n = 0;
  for (auto& l : W.log) n += l.find(part) != std::string::npos;
  return n;
}
static KkSlot* slot_of(daE_KK_c* a) { return kk_find(a); }
static void until(daE_KK_c* a, int max, bool (*pred)(daE_KK_c*)) {
  for (int i = 0; i < max && !pred(a); ++i) step();
}
// ---------------------------------------------------------------- A/B/C: identity, patterns, rhythm, windows
static void lab_patterns_and_rhythm() {
  for (int swapped = 0; swapped < 2; ++swapped) {
    kz.ident = kk::Identity{};  // a new session of the model
    std::uint64_t seen_patterns = 0;
    for (int run = 0; run < 24 && seen_patterns != 0xF; ++run) {
      reset_world();
      W.tip_swapped = swapped != 0;
      W.link.current.pos = {0, 0, 400};  // out of reach of the lunges, inside the native attack range: no contact, pure rhythm
      W.reach = 0;
      daE_KK_c* a = spawn(100 + run);
      for (int i = 0; i < run && !(slot_of(a) && slot_of(a)->ep.active); ++i) step();  // vary the seed tick, never past an episode start
      until(a, 60, [](daE_KK_c* x) { return slot_of(x) && slot_of(x)->ep.active; });
      KkSlot* s = slot_of(a);
      CHECK(s && s->ep.active && a->health == 340 && a->field_0x560 == 340);
      const unsigned pat = s->ep.pattern;
      seen_patterns |= 1ull << pat;
      const kk::Plan plan = kk::make_plan(pat);
      W.native_opens.clear(), W.native_open_bck.clear(), W.native_open_rate.clear();
      const bool measured = kz.ident.verified;  // the mapping this episode used from its first action
      std::uint64_t at_ticks = 0;
      const std::uint64_t t_first = W.tick;
      for (int t = 0; t < 600 && (s->ep.active || W.tick < t_first + 5); ++t) {
        step();
        if (a->field_0x67e == 0 && a->mActionMode == 8) ++at_ticks;  // registered by cc_set this tick
        if (s->ep.active && s->ep.ph == KkEp::Melee && a->mActionMode == 8) CHECK(a->morf.mFrameCtrl.mRate > 1.0f || a->morf.isStop());  // never a native clip inside an episode (J3DFrameCtrl zeroes the rate of a stopped clip)
      }
      const std::vector<std::uint64_t>& opens = W.native_opens;
      const std::vector<int>& bcks = W.native_open_bck;
      CHECK(opens.size() == plan.n);  // C: exactly one opening per D/E, no fabricated second window
      for (unsigned i = 0; i < plan.n; ++i) {
        if (measured) CHECK(bcks[i] == kz.ident.bck(plan.a[i].kind));  // the MEASURED clip of each kind: D sweeps, E thrusts
        if (measured) CHECK((bcks[i] == 6) != W.tip_swapped ? plan.a[i].kind == kk::K::E : plan.a[i].kind == kk::K::D);  // against the model's truth
        if (i) CHECK(int(opens[i] - opens[i - 1]) == int(plan.a[i].open - plan.a[i - 1].open));  // B: the planned interval, tick-exact
      }
      std::uint64_t want_at = 0;
      for (unsigned i = 0; i < plan.n; ++i) want_at += plan.a[i].window;
      CHECK(at_ticks == want_at);  // every native window kept whole: no skipped or merged AT tick
      CHECK(a->se_attack01 + a->se_attack03 >= 1);
      CHECK(s->st.duplicates == 0);
      if (plan.tail.on) {
        CHECK(W.spawns == 1 && s->st.spawns == 1 && s->st.spawn_dupes == 0);  // D: one real spear
        CHECK(a->field_0x67d == 0 && s->st.restores == 1);                    // restored, back to the fight
        CHECK(s->st.ended[unsigned(kk::End::Completed)] == 1);
      } else {
        CHECK(W.spawns == 0 && s->st.ended[unsigned(kk::End::Completed)] == 1);
      }
      CHECK(count_log("CHILFOS_RHYTHM event=RECORDED_VS_ACTUAL") == int(plan.n) - 1);
      CHECK(s->rest_until > 0);  // a rest follows every completed episode
      for (int t = 0; t < 40; ++t) {
        step();
        CHECK(!s->ep.active);
      }
      CHECK(s->st.rest > 0);
    }
    CHECK(seen_patterns == 0xF);
    // identity: two clips measured, opposite classes -> verified; swapped model -> swapped mapping
    CHECK(kz.ident.verified || kz.ident.votes[0][1] + kz.ident.votes[0][2] + kz.ident.votes[1][1] + kz.ident.votes[1][2] >= 2);
    if (kz.ident.verified) CHECK(kz.ident.swapped == (swapped != 0));
  }
}
// The frame-crossing guarantee: an accelerated burst thrust whose integer frames never read 24 still opens its AT, plays its SE once, never twice.
static void lab_crossing() {
  reset_world();
  W.link.current.pos = {0, 0, 400}, W.reach = 0;
  daE_KK_c* a = spawn(300);
  until(a, 60, [](daE_KK_c* x) { return slot_of(x) && slot_of(x)->ep.active; });
  KkSlot* s = slot_of(a);
  int opens = 0;
  bool was = false;
  for (int t = 0; t < 400 && s->ep.active; ++t) {
    step();
    const bool open = a->field_0x67e == 0;
    opens += open && !was;
    was = open;
  }
  CHECK(opens >= 6);
  // direct unit: reads 22.5 -> 25.5 at x1.5 skip 24: snapped once; 25.5 -> 27.0 fires 27 natively
  const kk::Cross c1 = kk::crossing(22.5f, 25.5f, kk::kAttackEvents, 2, 0);
  CHECK(c1.snap && c1.frame == 24.0f && c1.event == 0);
  const kk::Cross c2 = kk::crossing(24.0f, 27.0f, kk::kAttackEvents, 2, 1);
  CHECK(!c2.snap);
  const kk::Cross c3 = kk::crossing(21.75f, 23.2f, kk::kThrowEvents, 4, 1);  // 23 read natively at x1.45 from 0
  CHECK(!c3.snap);
  const kk::Cross c4 = kk::crossing(22.6f, 24.05f, kk::kThrowEvents, 4, 1);  // 23 skipped (low fps / morf): snapped
  CHECK(c4.snap && c4.frame == 23.0f);
  const kk::Cross c5 = kk::crossing(23.0f, 23.4f, kk::kThrowEvents, 4, 3);  // 23 again after it fired: flagged duplicate, never snapped
  CHECK(c5.duplicate && !c5.snap);
  reset_world();
  W.link.current.pos = {0, 0, 400}, W.reach = 0;
  a = spawn(301);
  until(a, 60, [](daE_KK_c* x) { return slot_of(x) && slot_of(x)->ep.active; });
  s = slot_of(a);
  s->ep.counter = true;
  a->mMoveMode = 3, a->field_0x764 = 6;
  s->clip.tracked = true, s->clip.last_read = 22.0f, s->clip.fired = 0;
  a->morf.mFrameCtrl.mRate = 1.5f, a->morf.mFrameCtrl.mFrame = 24.0f;
  a->executeAttack();
  const auto native_count = W.native_opens.size();
  const int sounds = a->se_attack03;
  a->executeAttack();
  CHECK(W.native_opens.size() == native_count && a->se_attack03 == sounds);
  CHECK(a->morf.getFrame() == 24.0f && a->field_0x67e == 0);
}
// ---------------------------------------------------------------- D: C3 tail with a wall, with Link near, with death
static daE_KK_c* run_to_pattern(unsigned want, std::uint32_t id_base) {
  for (int run = 0; run < 64; ++run) {
    reset_world();
    W.link.current.pos = {0, 0, 400}, W.reach = 0;
    daE_KK_c* a = spawn(id_base + run);
    for (int i = 0; i < run && !(slot_of(a) && slot_of(a)->ep.active); ++i) step();
    until(a, 60, [](daE_KK_c* x) { return slot_of(x) && slot_of(x)->ep.active; });
    if (slot_of(a)->ep.pattern == want) return a;
  }
  return nullptr;
}
static void lab_c3_tail() {
  daE_KK_c* a = run_to_pattern(2, 400);
  CHECK(a);
  KkSlot* s = slot_of(a);
  until(a, 400, [](daE_KK_c* x) { return slot_of(x)->ep.ph == KkEp::Back; });
  CHECK(s->ep.ph == KkEp::Back && a->mActionMode == 4);
  for (int i = 0; i < 3; ++i) {
    step();
    CHECK(a->field_0x768 != 0x1E);
  }  // walking back with the spear in the hand: no weapon-restore clip
  a->wall = true;  // the native backstep meets a wall: it throws from there, without walking through
  until(a, 60, [](daE_KK_c* x) { return slot_of(x)->ep.ph != KkEp::Back; });
  CHECK(a->field_0x768 != 0x1E);
  CHECK(s->ep.ph == KkEp::Throw && s->st.early_throw == 1 && a->mActionMode == 3);
  CHECK(a->field_0x764 != 0x13);  // no spear-restore clip while the spear is in the hand
  W.link.current.pos = {0, 0, 300};  // Link walks in during the throw: the native melee cancel is held, the planned spear flies
  until(a, 80, [](daE_KK_c* x) { return x->field_0x67d == 1; });
  CHECK(W.spawns == 1 && a->field_0x67d == 1 && a->eyePos.y == 150.0f);
  CHECK(a->morf.mFrameCtrl.mRate == kk::policy::kRateThrow || a->field_0x764 == 0x12);
  until(a, 120, [](daE_KK_c* x) { return x->field_0x764 == 0x12; });
  CHECK(a->morf.mFrameCtrl.mRate == kk::policy::kRateRestore && a->wmorf.mFrameCtrl.mRate == kk::policy::kRateRestore);  // the fast regeneration, body and spear
  until(a, 200, [](daE_KK_c* x) { return !slot_of(x)->ep.active; });
  CHECK(W.spawns == 1 && a->field_0x67d == 0 && s->st.ended[unsigned(kk::End::Completed)] == 1);
  // death in the backstep: the episode ends, nothing is thrown afterwards
  a = run_to_pattern(2, 500);
  s = slot_of(a);
  until(a, 400, [](daE_KK_c* x) { return slot_of(x)->ep.ph == KkEp::Back; });
  W.link.current.pos = {0, 0, 150}, W.reach = 240;
  W.sword_type = kAtIron;
  link_swing();
  step(4);
  CHECK(a->mActionMode == 10 && s->dead && W.spawns == 0);
  step(80);
  CHECK(W.spawns == 0);
}
// ---------------------------------------------------------------- E: HP, exclusions
static void lab_identity_hp() {
  reset_world();
  W.link.current.pos = {0, 0, 2000};
  daE_KK_c* a = spawn(600);
  daE_KK_c* spear = spawn(601, 1);
  daE_KK_c* foreign = spawn(602);
  foreign->health = 280, foreign->field_0x560 = 280;  // another mod scaled it: kept
  daE_KK_c* hidden = spawn(603, 2);
  hidden->attention_info.distances[0] = 0;
  step(3);
  CHECK(a->health == 340 && a->field_0x560 == 340);
  CHECK(spear->health == 140 && !slot_of(spear) && kz.spear_children >= 1 && count_log("reason=SPEAR_CHILD") <= 1);  // FIX03: a spear child takes no slot
  CHECK(foreign->health == 280 && slot_of(foreign)->admitted);
  CHECK(!slot_of(hidden)->admitted && hidden->health == 140);
  hidden->attention_info.distances[0] = 0x45;  // the iron ball was obtained: it joins
  step(2);
  CHECK(slot_of(hidden)->admitted && hidden->health == 340);
  a->health = 200;
  step(3);
  CHECK(a->health == 200);  // once only
  CHECK(count_log("event=HP_SET") == 2 && count_log("event=HP_FOREIGN") == 1);
}
// ---------------------------------------------------------------- F/H/I: Link parries, blocks, direct hits, the Chilfos' defense, one verdict per swing
static daE_KK_c* fight(std::uint32_t id, bool idle = true) {
  reset_world();
  W.reach = 240;
  W.link.current.pos = {0, 0, 180};
  daE_KK_c* a = spawn(id);
  step(2);
  if (idle) slot_of(a)->rest_until = W.tick + 100000;  // keep it out of its combos: a pure defense scenario
  return a;
}
static void lab_link_side() {
  daE_KK_c* a = run_to_pattern(3, 700);  // C4: its burst 5-6 is the hardest physical test
  KkSlot* s = slot_of(a);
  W.link.current.pos = {0, 0, 180}, W.reach = 240;
  W.guarding = true, W.guard_attack = true, W.ga_in_window = true;  // Link parries every window
  int windows = 0;
  bool was = false;
  for (int t = 0; t < 600 && s->ep.active; ++t) {
    step();
    const bool open = a->field_0x67e == 0;
    windows += open && !was;
    was = open;
  }
  CHECK(windows >= 6 && s->st.link_parry == unsigned(windows) && W.parry_confirms == int(s->st.link_parry));  // each window, exactly its own parry
  CHECK(s->p.points() >= 6 * kk::policy::kGainLinkParry - 1 || s->p.latched);
  CHECK(W.burst_gaps >= 1);  // the burst gap re-arms Link's shield chain for the second thrust
  for (auto& g : W.gains) CHECK(g.pts != kk::policy::kLinkBlock);  // a parry is never also a block for Link
  // blocks: +20 to Link, nothing to the Chilfos
  a = run_to_pattern(0, 800);
  s = slot_of(a);
  W.link.current.pos = {0, 0, 180}, W.reach = 240, W.guarding = true, W.guard_attack = false;
  const int before = s->p.points();
  until(a, 120, [](daE_KK_c* x) { return slot_of(x)->st.link_block > 0; });
  CHECK(s->st.link_block == 1 && W.gains.size() == 1 && W.gains[0].pts == kk::policy::kLinkBlock && W.gains[0].block && s->p.points() == before);
  // direct hit: offered, credited +30 with the native receipt (stubbed join)
  a = run_to_pattern(0, 900);
  s = slot_of(a);
  W.link.current.pos = {0, 0, 180}, W.reach = 240, W.guarding = false;
  until(a, 120, [](daE_KK_c* x) { return slot_of(x)->st.link_direct_offered > 0; });
  step(1);
  CHECK(s->st.link_direct_offered == 1 && !W.gains.empty() && W.gains.back().pts == kk::policy::kLinkDirect && !W.gains.back().block);
  // poise: a real sword hit while the Chilfos' own AT is open costs HP and posture, and the combo goes on with its clip
  a = run_to_pattern(0, 950);
  s = slot_of(a);
  W.link.current.pos = {0, 0, 180}, W.reach = 240, W.guarding = false;  // a blocked Chilfos thrust starts the native 8-tick no-damage window: Link trades instead
  until(a, 200, [](daE_KK_c* x) { return x->field_0x67e == 0; });
  const int hp = a->health;
  const unsigned act = s->ep.act;
  link_swing();
  step(4);
  CHECK(a->health < hp && s->st.sword_damage == 1 && s->st.poise == 1 && s->ep.active && a->mActionMode == 8 && s->ep.act == act);
  until(a, 400, [](daE_KK_c* x) { return !slot_of(x)->ep.active; });
  CHECK(s->st.ended[unsigned(kk::End::Completed)] == 1);
}
static void lab_defense() {
  for (int order = 0; order < 2; ++order) {
    // Unprepared frontal swings: block (+3) or damage (+5, HP lost) by the seeded roll; never both, never parry.
    daE_KK_c* a = fight(1000 + order);
    W.link_first = order == 0;
    KkSlot* s = slot_of(a);
    int blocks = 0, wounds = 0;
    for (int i = 0; i < 40; ++i) {
      s->def.cooldown_until = W.tick + 1000;  // no parry preparation in this part
      const int hp = a->health, pts = s->p.points();
      link_swing();
      step(12);
      if (a->health < hp) {
        ++wounds;
        CHECK(s->p.points() == std::min(100, pts + 5) || s->p.latched);
      } else if (s->sv.v == kk::V::Block) {
        ++blocks;
        CHECK(s->p.points() == std::min(100, pts + 3) || s->p.latched);
      }
      if (s->brk.st != kk::Bk::None || a->mActionMode == 10) break;
      a->setActionMode(0, 0);
    }
    CHECK(blocks > 0 && wounds > 0 && s->st.enemy_parries == 0 && W.reverse_rebounds == 0);
    // Prepared parry: TgShield presented, Link's clip rebounds natively, +25 Link, +0 Chilfos, no HP lost, a real counter follows.
    a = fight(1100 + order);
    W.link_first = order == 0;
    s = slot_of(a);
    s->def.nblocks = 2, s->def.block_ticks[0] = W.tick, s->def.block_ticks[1] = W.tick, s->def.block_serials[0] = 1, s->def.block_serials[1] = 2;  // two blocks: the 80% continuation
    bool parried = false;
    for (int i = 0; i < 30 && !parried; ++i) {
      const int hp = a->health, pts = s->p.points();
      W.gains.clear();
      link_swing();
      step(14);
      if (s->st.enemy_parries > 0) {
        parried = true;
        CHECK(a->health == hp && s->p.points() <= pts);  // I: never parry + HP lost on the same swing
        CHECK(W.gains.size() == 1 && W.gains[0].pts == kk::policy::kLinkEnemyParry);
        CHECK(W.reverse_rebounds >= 1);
        CHECK(s->counter_pending || s->ep.counter || s->st.counters > 0);
      } else {
        s->def.nblocks = 2, s->def.block_ticks[0] = s->def.block_ticks[1] = W.tick, s->def.block_serials[0] = 10 + i, s->def.block_serials[1] = 20 + i;
        a->setActionMode(0, 0);  // back to the idle wait only when no parry happened (FIX02: a parry's counter is already on its way)
      }
    }
    CHECK(parried);
    s->rest_until = 0, a->field_0x672 = 0;  // the laboratory's own idle wait (fight()) is lifted with it
    until(a, 200, [](daE_KK_c* x) { return slot_of(x)->st.counters > 0 && slot_of(x)->ep.active; });
    CHECK(s->st.counters == 1 && s->ep.counter && s->ep.n == 1);
    until(a, 200, [](daE_KK_c* x) { return !slot_of(x)->ep.active; });
    CHECK(s->st.impacts_open >= 1);  // the counter is a real attack with its own window
  }
  // Flank: the parry was presented while Link stood in front; Link reached the back before the pass. Never a parry, never a block, no rebound.
  daE_KK_c* a = fight(1200);
  KkSlot* s = slot_of(a);
  for (int i = 0; i < 6; ++i) {
    W.link.current.pos = {0, 0, 180};
    a->shape_angle.y = a->current.angle.y = 0;  // FIX04: the native guard turns from current.angle.y and writes it into shape_angle.y
    s->def.cooldown_until = 0;
    s->def.prepared = true, s->def.prepared_serial = kz.swing_serial + 1, s->def.prepared_until = W.tick + 30;
    link_swing();
    W.behind_at = W.tick + 2;  // the first tick of Link's sword AT: presented in this Execute, Link behind at the pass
    step(12);
    a->setActionMode(0, 0);
  }
  W.behind_at = 0;
  CHECK(s->st.enemy_parries == 0 && s->st.blocks == 0 && s->st.sword_damage > 0);
  CHECK(W.reverse_suppressed > 0 && W.reverse_rebounds == 0);  // the shield-flagged flank contact never rebounds Link
}
// ---------------------------------------------------------------- G: break, two finishers, timeout, normal death
static daE_KK_c* broken(std::uint32_t id) {
  daE_KK_c* a = fight(id);
  KkSlot* s = slot_of(a);
  s->p.u = std::int64_t(97) * kk::policy::kUnitsPerPoint;
  s->def.cooldown_until = W.tick + 100000;
  s->rng.seed(5);
  for (int i = 0; i < 30 && s->brk.st == kk::Bk::None; ++i) {  // the swing that crosses 100 (block +3 or damage +5)
    link_swing();
    step(12);
    a->setActionMode(a->mActionMode == 5 ? 5 : 0, a->mActionMode == 5 ? a->mMoveMode : 0);
  }
  step(2);
  return a;
}
static void lab_break() {
  // B) a new sword swing during the stagger: native death, once, whatever the HP
  daE_KK_c* a = broken(1300);
  KkSlot* s = slot_of(a);
  CHECK(s->brk.active() && a->mActionMode == 5 && a->health > 1);  // the crossing swing did not kill
  CHECK(a->morf.mFrameCtrl.mAttribute == 2);                                        // the stagger clip holds
  const int hp = a->health;
  CHECK(hp > 100);
  link_swing();
  step(6);
  CHECK(a->mActionMode == 10 && s->dead && s->st.decisive_sword == 1 && s->st.fallback_deaths == 0);
  step(80);
  CHECK(a->deleted && count_log("event=NATIVE_DEATH") == 1);
  // A collision whose native damage is zero must not manufacture a death or consume HP.
  a = broken(1350);
  s = slot_of(a);
  const int ignored_hp = a->health;
  cCcD_Obj zero_hit;
  zero_hit.at_type = kAtSword, zero_hit.owner = &W.link, zero_hit.power = 0;
  ++kz.swing_serial;
  a->mDamageTimer = 0;
  a->mCyl.tg_hit = &zero_hit;
  a->damage_check();
  CHECK(a->health == ignored_hp && a->mActionMode != 10 && !s->dead);
  CHECK(s->st.decisive_sword == 0 && s->st.fallback_deaths == 0);
  CHECK(a->mActionMode == 5 && s->brk.active() && s->st.decisive_no_receipt == 1);  // no native spear-less reaction ends the stagger early
  CHECK(count_log("event=DECISIVE_NO_RECEIPT") == 1);
  // The swing that crossed 100 contacting again after the native 8-tick timer: never damage, guard or finisher from that swing, whatever it was cached as.
  for (int cached = 0; cached < 3; ++cached) {
    a = broken(1360 + std::uint32_t(cached));
    s = slot_of(a);
    CHECK(s->brk.active() && kz.swing_serial == s->brk.latch_serial && s->sv.of(kz.swing_serial) != kk::V::None);  // the real path cached the crossing verdict
    if (cached) s->sv.set(kz.swing_serial, cached == 1 ? kk::V::Block : kk::V::EnemyParry);
    a->health = 3;  // one more native sword damage would be lethal
    const auto ign0 = s->st.ignored;
    cCcD_Obj again;
    again.at_type = kAtSword, again.owner = &W.link, again.power = 2;
    a->mDamageTimer = 0;
    a->mCyl.tg_hit = &again;
    a->damage_check();
    CHECK(a->health == 3 && a->mActionMode == 5 && !s->dead && s->brk.active() && s->st.ignored == ign0 + 1 && s->st.decisive_sword == 0);
    CHECK(s->sv.of(kz.swing_serial) == kk::V::Ignored);
  }
  // ... while a NEW swing during the same stagger still finishes it through the native lethal branch
  link_swing();
  step(6);
  CHECK(a->mActionMode == 10 && s->dead && s->st.decisive_sword == 1);
  // A) Shield Attack then Helm Splitter: a Shield Attack alone does not kill, the Helm Splitter that follows does
  a = broken(1400);
  s = slot_of(a);
  W.shield_attack_hit = true;
  step(3);
  CHECK(a->mActionMode == 5 && a->checkHeadLockFlg() && s->brk.shield_prep && a->health > 1);
  step(10);  // Link jumps: the native 8-tick no-damage window of the Shield Attack contact is over
  CHECK(a->mActionMode == 5 && a->health > 1);  // the Shield Attack alone did not kill
  link_swing(daPy_py_c::CUT_TYPE_HEAD_JUMP, 6, 0x30);
  step(6);
  CHECK(a->mActionMode == 10 && s->st.decisive_helm == 1 && s->st.decisive_sword == 0 && count_log("route=HELM_SPLITTER") == 1);
  // timeout: 90 ticks, posture 0, HP kept, back to the fight; the window is absolute (a Shield Attack does not renew it)
  a = broken(1500);
  s = slot_of(a);
  const std::uint64_t start = s->brk.start;
  const int hp2 = a->health;
  W.shield_attack_hit = true;
  step(3);
  until(a, 200, [](daE_KK_c* x) { return !slot_of(x)->brk.active(); });
  CHECK(W.tick - start <= kk::policy::kBreakTicks + 2 && W.tick - start >= kk::policy::kBreakTicks);
  CHECK(a->health == hp2 && s->p.points() == 0 && !s->p.latched && a->mActionMode != 10 && a->mActionMode != 5 && !a->checkHeadLockFlg());
  CHECK(s->st.recovered == 1);
  // FIX01: a Shield Attack outside the stagger neither staggers (no native DRAWBACK) nor opens Helm Splitter (FIX05 rule)
  a = fight(1600);
  s = slot_of(a);
  W.shield_attack_hit = true;
  step(3);
  CHECK(a->mActionMode != 5 && !a->checkHeadLockFlg() && s->st.bash_absorbed == 1 && s->st.bash_se == 1 && a->mCreatureSound.collision_se == 1);
  // an Iron Ball the native does not kill with (Link's status 0x400): +30 once, no death
  a = fight(1650);
  s = slot_of(a);
  W.status_400 = true, W.sword_type = kAtIron;
  const int pts0 = s->p.points();
  link_swing();
  step(6);
  CHECK(a->mActionMode != 10 && s->p.points() == pts0 + kk::policy::kGainIron && count_log("source=IRON_BALL") == 1);
  // Repeated callbacks within the native invulnerability interval cannot credit again.
  cCcD_Obj iron;
  iron.at_type = kAtIron, iron.owner = &W.link;
  for (int i = 0; i < 7; ++i) {
    a->mCyl.tg_hit = &iron;
    a->damage_check();
  }
  CHECK(s->p.points() == pts0 + 30 && count_log("source=IRON_BALL") == 1);
  // A different hook skipping the original must not count a PRE contact as a receipt.
  a = fight(1660);
  s = slot_of(a);
  a->mCyl.tg_hit = &iron;
  daE_KK_c* me = a;
  void* args[] = {&me};
  on_kk_damage_pre(nullptr, args, nullptr, nullptr);
  on_kk_damage_post(nullptr, args, nullptr, nullptr);
  CHECK(s->p.points() == 0 && a->health == 340);
  // The ordinary lethal route remains native, with no +30.
  W.status_400 = false;
  a->damage_check();
  CHECK(a->mActionMode == 10 && a->health == 0 && s->p.points() == 0 && s->dead);
  W.status_400 = false, W.sword_type = kAtSword;
  // normal death at 0 HP without a break
  a = fight(1700);
  s = slot_of(a);
  a->health = 3, a->field_0x67d = 1;  // spear-less: native damage
  link_swing();
  step(6);
  CHECK(a->mActionMode == 10 && s->st.decisive_sword == 0 && s->brk.st == kk::Bk::None);
}

// ---------------------------------------------------------------- FIX01: posture below 100 never staggers (FIX05 rule), combos and the C3 tail survive Link's contacts
static cCcD_Obj g_hit, g_bash;
static void inject_sword(daE_KK_c* a, int power = 2, bool new_swing = true) {  // Link's sword on the body cylinder in the next collision pass
  if (new_swing) link_swing();  // Link's setCutType: a new swing serial
  W.swing_len = 0;              // the contact is this one, not the scripted pass
  g_hit = cCcD_Obj{};
  g_hit.at_type = kAtSword, g_hit.owner = &W.link, g_hit.power = power;
  a->mDamageTimer = 0;
  a->mCyl.tg_hit = &g_hit;
}
static void inject_bash(daE_KK_c* a) {  // Link's Shield Attack (R+B) on the body cylinder
  g_bash = cCcD_Obj{};
  g_bash.at_type = AT_TYPE_SHIELD_ATTACK, g_bash.owner = &W.link;
  a->mCyl.tg_hit = &g_bash;
}
// FIX04: Link at heading `rel` (0 = in front, 0x8000 = behind) of the actor's CURRENT facing, `r` away, his sword pointed at it (a player who keeps
// circling to stay there: the ready Chilfos turns toward a Link who stands still)
static void link_at(const daE_KK_c* a, s16 rel, float r = 180.0f) {
  const double h = double(s16(a->shape_angle.y + rel)) * 3.14159265358979 / 32768.0;
  W.link.current.pos = {a->current.pos.x + float(r * std::sin(h)), 0, a->current.pos.z + float(r * std::cos(h))};
  W.link.shape_angle.y = s16(a->shape_angle.y + rel + 0x8000);
}
static bool reacted(const daE_KK_c* a, std::size_t from, bool allow_back) {  // a native damage/stagger reaction (or a non-C3 backwalk) after a contact
  for (std::size_t i = from; i < a->acts.size(); ++i)
    if (a->acts[i] == 7 || a->acts[i] == 5 || (!allow_back && a->acts[i] == 4)) return true;
  return false;
}
static int ends_with(const char* cause) {
  int n = 0;
  for (auto& l : W.log) n += l.find("CHILFOS_COMBO event=END") != std::string::npos && l.find(cause) != std::string::npos;
  return n;
}
static void lab_poise() {
  // (1) the real D/E evidence of the user's 0.13.0 log (engine pose, Dusklight 2.0.3): BCK5 sweeps, BCK6 thrusts; the 0.13.0 rule left BCK5 UNSURE
  CHECK(kk::classify_tip(343.9f, -21.3f, 379.6f) == kk::Tip::Diagonal && kk::classify_tip(356.5f, -35.7f, 441.4f) == kk::Tip::Diagonal);
  CHECK(kk::classify_tip(322.7f, -12.4f, 324.6f) == kk::Tip::Diagonal);
  CHECK(kk::classify_tip(22.7f, -43.2f, 168.8f) == kk::Tip::Thrust && kk::classify_tip(0.1f, 5.8f, 59.6f) == kk::Tip::Thrust && kk::classify_tip(12.3f, -4.1f, 120.2f) == kk::Tip::Thrust);
  CHECK(kk::classify_tip(10.0f, 5.0f, 12.0f) == kk::Tip::Unsure);  // too small to say anything
  {
    kk::Identity id;
    id.observe(5, kk::classify_tip(343.9f, -21.3f, 379.6f)), id.observe(6, kk::classify_tip(22.7f, -43.2f, 168.8f));
    id.observe(5, kk::classify_tip(356.5f, -35.7f, 441.4f)), id.observe(6, kk::classify_tip(12.3f, -4.1f, 120.2f));
    CHECK(id.verified && !id.swapped && id.diagonal() == 5 && id.thrust() == 6);  // MEASURED_DEFAULT: no constant flipped
  }
  // (2) WAIT, posture 0 and 50, Link behind: HP down, +5 once, no native flinch/backwalk, no backward travel, same action/mode/heading
  for (int pts : {0, 50}) {
    daE_KK_c* a = fight(2000 + std::uint32_t(pts));
    KkSlot* s = slot_of(a);
    s->p.u = std::int64_t(pts) * kk::policy::kUnitsPerPoint;
    W.link.current.pos = {0, 0, -180};  // the back: no guard, no parry
    step(3);
    a->current.angle.y = s16(a->shape_angle.y + 0x4000);  // a heading that is not the facing (the native snaps current.angle.y = shape_angle.y on a hit)
    const int hp = a->health, act = a->mActionMode, mode = a->mMoveMode;
    const s16 head = a->current.angle.y;
    const std::size_t from = a->acts.size();
    const float d0 = a->disp;
    inject_sword(a);
    step();
    CHECK(a->health == hp - 2 && s->p.points() == pts + kk::policy::kGainSwordHp && s->st.resumed == 1 && count_log("source=SWORD_HP") == 1);
    // FIX04 P3: outside its own attack the wound wakes the native guard (6,0): no flinch, no slide, its front covered for the next swing
    CHECK(act == kk::kWait && a->mActionMode == kk::kGuard && count_log("CHILFOS_POISE event=RESUMED") == 1 && count_log("reason=WOUND") == 1 && s->st.guard_after_wound == 1);
    (void)mode, (void)head;
    step(40);
    CHECK(!reacted(a, from, false) && a->disp >= d0 && !s->brk.active());
  }
  {  // (2b) inside its own attack the hit is resumed exactly: same action, mode AND heading (the native snaps current.angle.y = shape_angle.y)
    daE_KK_c* a = fight(2050, false);
    KkSlot* s = slot_of(a);
    until(a, 600, [](daE_KK_c* x) { return slot_of(x)->ep.active && x->mActionMode == 8 && x->morf.getFrame() >= 26.0f && x->morf.getFrame() < 30.0f; });
    CHECK(s->ep.active && a->mActionMode == 8);
    a->current.angle.y = s16(a->shape_angle.y + 0x4000);
    const s16 head = a->current.angle.y;
    const int mode = a->mMoveMode, hp = a->health;
    inject_sword(a);
    step();
    CHECK(a->health < hp && a->mActionMode == 8 && a->mMoveMode == mode && a->current.angle.y == head && s->ep.active && s->st.guard_after_wound == 0);
  }
  // (3) posture 99: the same contact crosses 100 -> the native stagger in the SAME call, never a native flinch first
  {
    daE_KK_c* a = fight(2100);
    KkSlot* s = slot_of(a);
    s->p.u = std::int64_t(99) * kk::policy::kUnitsPerPoint;
    W.link.current.pos = {0, 0, -180};
    step(3);
    const std::size_t from = a->acts.size();
    inject_sword(a);
    step();
    CHECK(s->brk.active() && a->acts.size() == from + 1 && a->acts[from] == 5 && s->st.immediate_breaks == 1 && a->health > 1);
    CHECK(count_log("origin=DAMAGE_POST") == 1);
  }
  // (4) every combo, posture 0 and 50: a sword contact inside EVERY open AT window; the combo keeps every action and completes
  for (unsigned pat = 0; pat < 4; ++pat)
    for (int pts : {0, 50}) {
      daE_KK_c* a = run_to_pattern(pat, 2200 + pat * 100 + std::uint32_t(pts));
      CHECK(a);
      KkSlot* s = slot_of(a);
      s->p.u = std::int64_t(pts) * kk::policy::kUnitsPerPoint;
      const kk::Plan plan = kk::make_plan(pat);
      W.native_opens.clear();
      const std::size_t from = a->acts.size();
      std::uint64_t hit_impact = ~0ull;
      int hits = 0;
      for (int t = 0; t < 800 && s->ep.active; ++t) {
        if (s->ep.window_open && s->ep.ph == KkEp::Melee && hit_impact != s->imp_serial && s->p.points() < 90) {
          hit_impact = s->imp_serial, ++hits;
          inject_sword(a);
        }
        step();
      }
      CHECK(hits == int(plan.n) || s->p.points() >= 90);
      CHECK(W.native_opens.size() == plan.n);                // every D/E still opened its own native window
      CHECK(!reacted(a, from, plan.tail.on));                 // no flinch, no native stagger; a backwalk only as the C3 backstep
      CHECK(s->st.ended[unsigned(kk::End::NativePriority)] == 0 && s->st.completed[pat] == 1);
      CHECK(s->st.resumed == unsigned(hits) && s->st.poise >= unsigned(hits) - (plan.tail.on ? 0u : 0u));
      CHECK(a->health == 340 - 2 * hits);
      if (plan.tail.on) CHECK(W.spawns == 1 && s->st.spawn_dupes == 0 && s->st.rearmed == 1);
    }
  // (5) C3 tail: a contact in BACKSTEP, in THROW before frame 23 and in RESTORE: committed, no guard, the tail goes on and completes
  {
    daE_KK_c* a = run_to_pattern(2, 2700);
    CHECK(a);
    KkSlot* s = slot_of(a);
    const std::size_t from = a->acts.size();
    bool hit_back = false, hit_throw = false, hit_restore = false;
    float back_speed = 0;
    for (int t = 0; t < 900 && s->ep.active; ++t) {
      if (!hit_back && s->ep.ph == KkEp::Back && a->mMoveMode == 1) hit_back = true, inject_sword(a);
      else if (!hit_throw && s->ep.ph == KkEp::Throw && a->mMoveMode == 2 && a->morf.getFrame() > 5 && a->morf.getFrame() < 20) hit_throw = true, inject_sword(a);
      else if (!hit_restore && s->ep.ph == KkEp::Restore && a->field_0x67d == 1) hit_restore = true, inject_sword(a);
      step();
      if (s->ep.ph == KkEp::Back && a->mActionMode == 4 && a->mMoveMode == 1) back_speed = a->speedF;
    }
    CHECK(hit_back && hit_throw && hit_restore && s->st.resumed == 3 && s->st.verdict[unsigned(kk::V::Block)] == 0);
    CHECK(!reacted(a, from, true) && s->st.completed[2] == 1 && W.spawns == 1 && s->st.rearmed == 1 && a->field_0x67d == 0);
    CHECK(back_speed == kk::policy::kBackSpeedNative * kk::policy::kRateBack && s->st.back_speed_fixes > 0);  // the backstep strides with its clip
    CHECK(count_log("CHILFOS_SPEAR event=THROW_START") == 1 && count_log("CHILFOS_SPEAR event=REARMED") == 1);
  }
  // (6) the throw cancelled by the break before frame 23: no spear is ever created afterwards
  {
    daE_KK_c* a = run_to_pattern(2, 2800);
    KkSlot* s = slot_of(a);
    for (int t = 0; t < 900 && !(s->ep.ph == KkEp::Throw && a->mMoveMode == 2 && a->morf.getFrame() > 8); ++t) step();
    CHECK(s->ep.ph == KkEp::Throw && W.spawns == 0);
    s->p.u = std::int64_t(99) * kk::policy::kUnitsPerPoint;
    inject_sword(a);
    step();
    CHECK(s->brk.active() && a->mActionMode == 5 && !s->ep.active && a->eyePos.y > -1.0e8f);
    step(300);
    CHECK(W.spawns == 0 && s->st.spawns == 0);
  }
  // (7) Link's Shield Attack below 100 inside every combo (the 16 premature ends of the user's 0.13.0 log): absorbed, one SE per press, the combo completes
  for (unsigned pat = 0; pat < 4; ++pat) {
    daE_KK_c* a = run_to_pattern(pat, 2900 + pat * 50);
    KkSlot* s = slot_of(a);
    const std::size_t from = a->acts.size();
    int presses = 0;
    for (int t = 0; t < 800 && s->ep.active; ++t) {
      const int k = t % 40;
      if (k < 3 && s->ep.ph == KkEp::Melee) {  // one press = contacts on three consecutive passes
        inject_bash(a);
        if (k == 0) ++presses;
      }
      step();
    }
    CHECK(presses >= 2 && s->st.bash_se == unsigned(presses) && a->mCreatureSound.collision_se == presses);
    CHECK(!reacted(a, from, kk::make_plan(pat).tail.on) && !a->checkHeadLockFlg() && s->st.completed[pat] == 1 && ends_with("cause=NATIVE_PRIORITY") == 0);
    CHECK(a->health == 340 && s->p.points() == 0);  // a Shield Attack is no posture or HP gain below 100 (FIX05)
  }
  // (8) the user's session shape: Link guards and presses R+B again and again while the Chilfos fights; no combo is cut by those presses
  {
    daE_KK_c* a = fight(3100, false);
    KkSlot* s = slot_of(a);
    W.guarding = true;
    unsigned completed = 0;
    for (int t = 0; t < 3000; ++t) {
      if (t % 75 == 10) W.shield_attack_hit = true;
      step();
    }
    for (unsigned i = 0; i < 4; ++i) completed += unsigned(s->st.completed[i]);
    CHECK(completed >= 3 && s->st.ended[unsigned(kk::End::NativePriority)] == 0 && s->st.bash_absorbed > 0 && !s->brk.active() && a->health == 340);
  }
  // (9) one posture credit per swing: the same swing contacting again after the native 8-tick timer is HP as native, never a second +5
  {
    daE_KK_c* a = fight(3200);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, -180};
    step(3);
    inject_sword(a);
    step();
    const int p1 = s->p.points(), hp1 = a->health;
    step(9);
    inject_sword(a, 2, false);
    step();
    CHECK(a->health == hp1 - 2 && s->p.points() <= p1 && s->st.sword_dupes == 1 && count_log("source=SWORD_HP") == 1);
  }
  // (10) the Chilfos' block receipt on Link is resolved even while its own damage timer runs (Link hit it a few ticks before)
  {
    daE_KK_c* a = run_to_pattern(0, 3300);
    KkSlot* s = slot_of(a);
    until(a, 200, [](daE_KK_c* x) { return slot_of(x)->ep.window_open; });
    CHECK(s->ep.window_open);
    const std::size_t g0 = W.gains.size();
    a->mDamageTimer = 5;
    a->mSph.at_hit = &W.link.mTgCyls[0], a->mSph.at_shield_hit = true;
    step();
    CHECK(W.gains.size() == g0 + 1 && W.gains.back().pts == kk::policy::kLinkBlock && W.gains.back().block && s->st.link_block == 1);
  }
  // (11) a non-sword weapon (arrow) keeps its native stun (the FIX05 rule): the end of the combo says so, with its origin
  {
    daE_KK_c* a = run_to_pattern(0, 3400);
    KkSlot* s = slot_of(a);
    step(5);
    static cCcD_Obj arrow;
    arrow.at_type = kAtArrow, arrow.owner = &W.link;
    a->mDamageTimer = 0, a->mCyl.tg_hit = &arrow;
    step();
    CHECK(a->mActionMode == 5 && !s->ep.active && count_log("origin=DAMAGE_PRE_OTHER_WEAPON") == 1 && count_log("last_link=OTHER_WEAPON") == 1);
  }
}

// ---------------------------------------------------------------- FIX02: the spear never breaks on a block, a defense that holds under repeated B, the counter, no slide, x1.10 bursts
static void force_verdict(daE_KK_c* a, kk::V v) { slot_of(a)->sv.set(kz.swing_serial, v); }  // the verdict kk_decide would have cached for this swing
static void lab_fix02() {
  // (A1) the seven 0.13.1 SPEAR_BREAK lines: guarding (action 6), a strong cut, verdict BLOCK -> native (7,0) CLASH. Now: the guard reaction, spear kept.
  for (int guard_mode = 0; guard_mode < 2; ++guard_mode)
    for (kk::V v : {kk::V::Block, kk::V::EnemyParry}) {
      daE_KK_c* a = fight(4000 + std::uint32_t(guard_mode) * 10 + std::uint32_t(v));
      KkSlot* s = slot_of(a);
      a->setActionMode(guard_mode ? 6 : 0, guard_mode ? 1 : 0);
      const int hp = a->health, pts = s->p.points();
      const std::size_t from = a->acts.size();
      link_swing(32);
      force_verdict(a, v);
      W.swing_len = 0;
      g_hit = cCcD_Obj{};
      g_hit.at_type = kAtSword, g_hit.owner = &W.link, g_hit.strong = true;  // at_power_check mHitStatus != 0
      a->mDamageTimer = 0, a->mCyl.tg_hit = &g_hit;
      step();
      CHECK(a->field_0x67d == 0 && a->mActionMode == 6 && a->health == hp && s->st.spear_break_prevented == 1 && count_log("CHILFOS_SPEAR event=BLOCK_INTACT") == 1);
      CHECK(count_log("source=SPEAR_BREAK") == 0 && (v != kk::V::Block || s->p.points() == pts + kk::policy::kGainBlock));
      step(60);
      CHECK(a->field_0x67d == 0 && !reacted(a, from, false) && s->st.blocked_spear_loss == 0 && s->st.other_spear_loss == 0);
      for (std::size_t i = from; i < a->acts.size(); ++i) CHECK(a->acts[i] != 7);  // executeDamage (CLASH, -80, disarm) never ran
    }
  // (A2) 100 / 300 / 1000 frontal swings of every cut kind, strong and normal, alternating with Link's guard: not one spear lost, ever
  for (int n : {100, 300, 1000}) {
    daE_KK_c* a = fight(4100 + std::uint32_t(n));
    KkSlot* s = slot_of(a);
    static const int cuts[] = {1, 2, 3, 4, 6, 7, 21, 32, 33};
    int swings = 0, offense = 0;  // LFT 0.13.6: swings that met a committed offensive episode (a chained combo included): no guard exists inside one
    for (int i = 0; i < n; ++i) {
      W.sword_strong = (i % 3) != 0;
      if (s->ep.active) ++offense;
      link_swing(cuts[i % 9]);
      for (int k = 0; k < 13; ++k) {
        step();
        // LFT 0.13.6: a counter may now chain one of the four combos; inside a chained C3's own backstep/throw/restore the spear leaves by design
        const bool c3_phase = s->ep.active && s->ep.chained && s->ep.ph != KkEp::Melee;
        CHECK(a->mActionMode != 7 && ((a->field_0x67d == 0 && a->mActionMode != 4) || c3_phase));
      }
      ++swings;
      if (a->health <= 60) a->health = 300;  // a scripted heal keeps the stress going (this lab measures the spear, not the death)
      if (s->brk.active() || s->brk.st == kk::Bk::Pending) s->brk.reset(), s->p.reset(), a->setActionMode(0, 0);
      s->p.u = 0;
    }
    W.sword_strong = false;
    CHECK(swings == n && s->st.blocked_spear_loss == 0 && s->st.other_spear_loss == 0 && s->st.spear_breaks == 0 && count_log("source=SPEAR_BREAK") == 0);
    CHECK(s->st.blocks + s->st.enemy_parries > unsigned(n - offense) / 3 && s->st.spear_break_prevented > 0);
  }
  // (B) repeated frontal B against an idle, ready Chilfos: it blocks most and parries, never two ready frontal wounds in a row without the guarantee
  auto spam = [](std::uint32_t id, bool behind, int swings, int& dmg_ready, int& max_run, int& parries, int& dmg_total) {
    daE_KK_c* a = fight(id);
    KkSlot* s = slot_of(a);
    if (behind) W.link.current.pos = {0, 0, -180};
    int run = 0;
    dmg_ready = max_run = dmg_total = 0;
    for (int i = 0; i < swings; ++i) {
      const auto d0 = s->st.verdict[unsigned(kk::V::Damage)];
      if (behind) link_at(a, s16(0x8000));  // FIX04: he keeps to its back (a static Link behind is faced by the ready Chilfos)
      link_swing(2);
      if (behind)
        for (int k = 0; k < 13; ++k) link_at(a, s16(0x8000)), step();
      else
        step(13);
      const bool damaged = s->st.verdict[unsigned(kk::V::Damage)] > d0;
      dmg_total += damaged ? 1 : 0;
      if (damaged && s->dec_ready_frontal) ++dmg_ready, ++run, max_run = std::max(max_run, run);
      else if (!damaged) run = 0;
      if (a->health <= 60) a->health = 300;
      if (s->brk.active() || s->brk.st == kk::Bk::Pending) s->brk.reset(), a->setActionMode(0, 0);
      s->p.u = 0;
      if (!s->ep.active && !s->counter_pending) s->rest_until = W.tick + 100000;
    }
    parries = int(s->st.enemy_parries);
    if (behind) CHECK(s->st.verdict[unsigned(kk::V::Block)] == 0 && s->st.verdict[unsigned(kk::V::EnemyParry)] == 0);
  };
  int dmg_ready = 0, max_run = 0, parries = 0, dmg_total = 0;
  spam(4200, false, 120, dmg_ready, max_run, parries, dmg_total);
  CHECK(dmg_ready * 100 <= 120 * 20);  // a ready frontal guard is beaten by at most 1 swing in 5
  CHECK(max_run <= int(kk::policy::kGuaranteeAfter));
  CHECK(parries >= 10);                 // parries keep coming under pressure (0.13.1: 120/90/60 cooldowns)
  spam(4300, true, 60, dmg_ready, max_run, parries, dmg_total);
  CHECK(dmg_total >= 57);               // the back stays open: no omniscient guard
  // (C) every confirmed parry ends in a STARTED counter (a real D/E with its own AT) or a CANCELLED one with its reason, never a silent expiry
  {
    daE_KK_c* a = fight(4400);
    KkSlot* s = slot_of(a);
    s->rest_until = 0;
    int confirmed = 0;
    for (int i = 0; i < 600 && confirmed < 6; ++i) {
      if (s->ep.active) {
        step();
        continue;
      }
      s->rest_until = W.tick + 100000;  // idle between swings (a parry's counter ignores the rest): only counters attack
      const auto p0 = s->st.enemy_parries;
      s->def.cooldown_until = 0;
      s->def.prepared = false;
      s->def.nblocks = 2, s->def.block_ticks[0] = s->def.block_ticks[1] = W.tick, s->def.block_serials[0] = 1000 + i, s->def.block_serials[1] = 2000 + i;
      link_swing();
      step(4);
      if (s->st.enemy_parries == p0) {
        step(10);
        continue;
      }
      ++confirmed;
      const std::uint64_t t0 = W.tick;
      const auto open0 = s->st.impacts_open, started0 = s->st.counter_started;
      until(a, 120, [](daE_KK_c* x) { return !slot_of(x)->counter_pending && !slot_of(x)->ep.active; });
      CHECK(s->st.counter_started == started0 + 1 && s->st.impacts_open == open0 + 1);  // started, and its window really opened
      CHECK(W.tick - t0 < 60);
    }
    CHECK(confirmed >= 3 && s->st.counter_expired == 0 && count_log("event=COUNTER_DISPATCH") >= confirmed);
    // Link out of reach right after the parry: cancelled with its reason at the end of the lifetime
    for (int i = 0; i < 200 && s->st.counter_expired == 0; ++i) {
      if (s->ep.active) {
        step();
        continue;
      }
      s->rest_until = W.tick + 100000;
      s->def.cooldown_until = 0, s->def.prepared = false;
      s->def.nblocks = 2, s->def.block_ticks[0] = s->def.block_ticks[1] = W.tick, s->def.block_serials[0] = 5000 + i, s->def.block_serials[1] = 6000 + i;
      W.link.current.pos = {0, 0, 180};
      link_swing();
      for (int k = 0; k < 8 && !s->counter_pending; ++k) step();
      if (s->counter_pending) W.link.current.pos = {0, 0, 1600};  // Link leaves (beyond the native attack and throw ranges) before the dispatch
      step(80);
      if (s->ep.active) step(80);
    }
    CHECK(s->st.counter_cancel[unsigned(kk::Cc::LinkFar)] >= 1 && count_log("event=COUNTER_CANCELLED") >= 1 && count_log("reason=LINK_FAR") >= 1);
  }
  // (C2) a committed counter is not cut by Link swinging into its wind-up (0.13.1: a BLOCK ended both counters that started)
  {
    daE_KK_c* a = fight(4500);
    KkSlot* s = slot_of(a);
    s->counter_pending = true, s->counter_until = W.tick + 60, s->parry_tick = W.tick - kk::policy::kCounterDelay;
    until(a, 30, [](daE_KK_c* x) { return slot_of(x)->ep.active && slot_of(x)->ep.counter; });
    CHECK(s->ep.active && s->ep.counter);
    W.link.current.pos = {0, 0, -180};  // even from the back a hit lands and the counter goes on
    inject_sword(a);
    step();
    W.link.current.pos = {0, 0, 180};
    link_swing();
    step(2);
    until(a, 80, [](daE_KK_c* x) { return !slot_of(x)->ep.active; });
    CHECK(s->st.ended[unsigned(kk::End::Defense)] == 0 && s->st.impacts_open >= 1 && s->st.ended[unsigned(kk::End::Completed)] == 1);
  }
  // (D1) its spear on Link's shield: the native -70 recoil is cancelled (it would slide ~280 units back mid-exchange)
  {
    daE_KK_c* a = run_to_pattern(1, 4600);  // C2: D, E-E, E-E, D
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 200}, W.reach = 240, W.guarding = true;
    float min_speed = 0;
    for (int t = 0; t < 600 && s->ep.active; ++t) {
      step();
      min_speed = std::min(min_speed, a->speedF);
    }
    CHECK(s->st.recoil_fixed >= 3 && min_speed >= 0.0f && s->st.completed[1] == 1 && count_log("kind=SPEAR_ON_LINK_SHIELD") >= 1);
    CHECK(s->st.link_block >= 4);  // every impact still met the shield: one receipt each
  }
  // (D2) the lunge stops kContactDist short of a Link in reach; farther away it is the native one
  for (float d : {180.0f, 300.0f, 700.0f}) {
    daE_KK_c* a = run_to_pattern(0, 4700 + std::uint32_t(d));
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, d};
    W.native_opens.clear();
    float disp0 = 0, travel = -1;
    for (int t = 0; t < 400 && s->ep.active && travel < 0; ++t) {
      step();
      if (W.native_opens.size() == 1 && disp0 == 0) disp0 = a->disp - a->speedF;
      if (W.native_opens.size() == 2) travel = 0;  // the first lunge is over before the second clip's frame 24
    }
    const float lunge = a->disp;  // the lab does not move current.pos: disp is the ground travel
    (void)lunge;
    if (d - kk::policy::kContactDist < kk::lunge_travel(kk::policy::kLungeNative)) CHECK(s->st.lunge_clamped >= 1 && count_log("kind=LUNGE_CAPPED") >= 1);
    else CHECK(s->st.lunge_clamped == 0);
  }
  {  // the travel of one capped lunge never takes the body inside kContactDist
    daE_KK_c* a = run_to_pattern(0, 4800);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 200};
    until(a, 200, [](daE_KK_c* x) { return x->speedF > 0.0f; });
    const float d0 = a->disp;
    step(30);
    CHECK(s->st.lunge_clamped >= 1 && a->disp - d0 <= 200.0f - kk::policy::kContactDist + 0.5f);
  }
  // (E) the E-E bursts at x1.10 (the Lizalfos FIX05 double), each with its own native window
  for (unsigned pat : {0u, 1u, 2u, 3u}) {
    daE_KK_c* a = run_to_pattern(pat, 4900 + pat * 10);
    KkSlot* s = slot_of(a);
    W.native_opens.clear(), W.native_open_rate.clear();
    const kk::Plan plan = kk::make_plan(pat);
    for (int t = 0; t < 800 && s->ep.active; ++t) step();
    CHECK(W.native_open_rate.size() == plan.n);
    for (unsigned i = 0; i < plan.n; ++i) {
      if (plan.a[i].burst) CHECK(std::fabs(W.native_open_rate[i] - 1.10f) < 1e-6f);
      if (plan.a[i].kind == kk::K::E && !plan.a[i].burst) CHECK(std::fabs(W.native_open_rate[i] - 1.30f) < 1e-6f);
      if (i && plan.a[i].burst && plan.a[i - 1].burst) CHECK(W.native_opens[i] - W.native_opens[i - 1] >= 14);  // two whole windows, never merged
    }
  }
  // (B2) the guarantee through the production adapter: two frontal swings in a row that beat its READY guard -> the next ready frontal swing is a block
  {
    daE_KK_c* a = fight(4250);
    KkSlot* s = slot_of(a);
    bool reached = false;
    for (int i = 0; i < 400 && !reached; ++i) {
      s->def.cooldown_until = W.tick + 100000;  // no parries here (their committed counters would interleave): only the guard is measured
      link_swing(2);
      step(40);
      if (a->health <= 60) a->health = 300;
      s->p.u = 0;
      reached = s->def.uncovered >= kk::policy::kGuaranteeAfter;
    }
    CHECK(reached);
    const auto b0 = s->st.verdict[unsigned(kk::V::Block)], g0 = s->st.guard_guarantees;
    link_swing(2);
    step(13);
    CHECK(s->st.verdict[unsigned(kk::V::Block)] == b0 + 1 && s->st.guard_guarantees == g0 + 1 && count_log("guarantee=1") >= 1);
  }
  // (A3) the spear-loss monitor itself: a loss right after a defended contact is reported (WARN), any other loss is counted apart, the C3 throw is not a loss
  {
    daE_KK_c* a = fight(4260);
    KkSlot* s = slot_of(a);
    link_swing(2);
    force_verdict(a, kk::V::Block);
    W.swing_len = 0;
    g_hit = cCcD_Obj{};
    g_hit.at_type = kAtSword, g_hit.owner = &W.link;
    a->mDamageTimer = 0, a->mCyl.tg_hit = &g_hit;
    step();
    a->field_0x67d = 1;  // a defect injected by hand: the spear gone right after the block
    step();
    CHECK(s->st.blocked_spear_loss == 1 && count_log("CHILFOS_SPEAR event=LOST") == 1 && count_log("after_block_or_parry=1") == 1);
    a->field_0x67d = 0;
    step(20);
    a->field_0x67d = 1;  // long after any defense: counted apart
    step();
    CHECK(s->st.blocked_spear_loss == 1 && s->st.other_spear_loss == 1);
  }
  // (F) 3000 ticks of R+B presses and sword swings in turn while it fights: FIX01 kept (no NATIVE_PRIORITY), no spear ever lost on a defense
  {
    daE_KK_c* a = fight(5000, false);
    KkSlot* s = slot_of(a);
    W.guarding = true;
    for (int t = 0; t < 3000; ++t) {
      if (t % 75 == 10) W.shield_attack_hit = true;
      if (t % 75 == 40) W.sword_strong = (t / 75) % 2 == 0, link_swing(t % 150 == 40 ? 32 : 2);
      step();
      if (a->health <= 60) a->health = 300;
      if (s->brk.active() || s->brk.st == kk::Bk::Pending) s->brk.reset(), s->p.reset(), a->setActionMode(0, 0);
    }
    W.sword_strong = false;
    unsigned completed = 0;
    for (unsigned i = 0; i < 4; ++i) completed += unsigned(s->st.completed[i]);
    CHECK(completed >= 3 && s->st.ended[unsigned(kk::End::NativePriority)] == 0 && s->st.blocked_spear_loss == 0 && s->st.other_spear_loss == 0);
  }
}

// ---------------------------------------------------------------- FIX03: HP and posture are two independent ways to defeat it; the guard that is up covers; the stagger
// never shows a guard; the Mortal Draw finisher is Link's native one; the throw's frame pacing is measured
static void inject_cut(daE_KK_c* a, int cut, int power) {
  link_swing(cut);
  W.swing_len = 0;
  g_hit = cCcD_Obj{};
  g_hit.at_type = kAtSword, g_hit.owner = &W.link, g_hit.power = power;
  a->mDamageTimer = 0;
  a->mCyl.tg_hit = &g_hit;
}
static void lab_fix03() {
  using P = daPy_py_c;
  static const int skills[] = {P::CUT_TYPE_LARGE_JUMP, P::CUT_TYPE_LARGE_JUMP_FINISH, P::CUT_TYPE_LARGE_TURN_LEFT, P::CUT_TYPE_LARGE_TURN_RIGHT, P::CUT_TYPE_MORTAL_DRAW_A,
                               P::CUT_TYPE_MORTAL_DRAW_B, P::CUT_TYPE_HEAD_JUMP, P::CUT_TYPE_TWIRL};
  for (int c : skills) CHECK(kk::native_instant_cut(c));  // the module's list is the native one, value by value (below: all 64 cut ids)
  for (int c = 0; c < 0x40; ++c) {
    W.link.cut = c;
    CHECK(kk::native_instant_cut(c) == native_cut_check(2));
  }
  // (A1) every hidden skill of the native instant-kill list, posture 0, HP 340 (the 0.13.2 log: Back Slice ticks 10915 / 11242): ordinary damage, no death
  std::uint32_t id = 6000;
  for (int c : skills) {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, -180};
    step(3);
    // LFT 0.13.6 FIX06 P0: the Jump Strike phases (0x13, 0x14) are no ordinary hit any more: ONE bounded loss (1.75x the ordinary effective) and ONE x1.6 gain
    const bool js = kk::f6::js_cut(c);
    if (js) ++kk::f6::g_js_episode;
    inject_cut(a, c, 2);
    step();
    const int want_hp = js ? 340 - kk::f6::js_damage(kk::f6::ordinary_of(2, 2)) : 338;
    CHECK(a->mActionMode != 10 && !s->dead && a->health == want_hp && s->st.skill_instant_suppressed == 1 && count_log("event=SKILL_INSTANT_SUPPRESSED") == 1);
    CHECK(count_log("event=NATIVE_DEATH") == 0 && s->p.points() == (js ? kk::f6::kGainJsKk : kk::policy::kGainSwordHp));
    if (js) {
      // (A2-JS) the Jump Strike can NEVER take the last HP: at 2 (the E_KK living floor) it lands, keeps the floor, no death; an ordinary cut then kills (HP_ZERO)
      a->health = 2;
      step(9);
      ++kk::f6::g_js_episode;
      inject_cut(a, c, 400);
      step();
      CHECK(a->mActionMode != 10 && !s->dead && a->health == 2 && count_log("event=NATIVE_DEATH") == 0 && s->st.js_nonlethal == 2 && s->st.js_headroom_miss == 0);
      step(9);
      inject_cut(a, P::CUT_TYPE_VERTICAL, 2);
      force_verdict(a, kk::V::Damage);  // an ordinary cut that lands (the guard this wound woke up would otherwise block it)
      step();
      CHECK(a->mActionMode == 10 && s->dead && count_log("cause=HP_ZERO") == 1 && s->st.death_skill_instant == 0 && s->st.death_hp_zero == 1);
      continue;
    }
    // (A2) the same skill when its ordinary damage takes the last HP: a normal death, cause HP_ZERO
    a->health = 2;
    step(9);
    inject_cut(a, c, 2);
    step();
    CHECK(a->mActionMode == 10 && s->dead && count_log("cause=HP_ZERO") == 1 && s->st.death_skill_instant == 0 && s->st.death_hp_zero == 1);
  }
  // (A3) the 0.13.2 log's two LEGITIMATE deaths (ticks 8236 / 12860): HP 20, posture 99 and 98, an ordinary cut of 20 -> dies, no break, cause HP_ZERO
  for (int pts : {99, 98}) {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    s->p.u = std::int64_t(pts) * kk::policy::kUnitsPerPoint;
    a->health = 20;
    W.link.current.pos = {0, 0, -180};
    step(3);
    inject_cut(a, P::CUT_TYPE_VERTICAL, 20);
    step();
    CHECK(a->mActionMode == 10 && s->dead && a->health <= 0 && s->st.breaks == 0 && count_log("cause=HP_ZERO") == 1 && count_log("broken=0") == 1);
  }
  // (B1) the three 0.13.2 leaks through a guard that was up (action 6, ready, frontal): ticks 8184 (HP 60, posture 94, cut 7), 13282 (260, 52, cut 6),
  // 14331 (160, 85, cut 4). Now: a block, HP kept, +3 once, no flinch, spear kept.
  struct Leak { int hp, pts, cut, power; } leaks[] = {{60, 94, P::CUT_TYPE_FINISH_VERTICAL, 40}, {260, 52, P::CUT_TYPE_FINISH_LEFT, 60}, {160, 85, 4, 40}};
  for (const Leak& L : leaks) {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    step(3);
    s->p.u = std::int64_t(L.pts) * kk::policy::kUnitsPerPoint;
    a->health = s16(L.hp);
    a->setActionMode(6, 1);
    const std::size_t from = a->acts.size();
    inject_cut(a, L.cut, L.power);
    step();
    CHECK(a->health == L.hp && s->st.guard_active_blocks == 1 && s->st.verdict[unsigned(kk::V::Block)] == 1 && s->p.points() == L.pts + kk::policy::kGainBlock);
    CHECK(count_log("guard_state=ACTIVE") == 1 && count_log("source=SWORD_HP") == 0 && a->field_0x67d == 0 && !reacted(a, from, false));
  }
  // (B2) 300 swings, normal and strong, into a guard that is up: not one HP lost; from the back the same guard does not cover
  {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    step(3);
    const int hp = a->health;
    for (int i = 0; i < 300; ++i) {
      a->setActionMode(6, 1);
      W.sword_strong = (i & 1) != 0;
      inject_cut(a, (i % 3) ? P::CUT_TYPE_VERTICAL : P::CUT_TYPE_FINISH_STAB, 3);
      step();
      CHECK(a->health == hp && a->field_0x67d == 0);
      s->p.u = 0, s->def.cooldown_until = W.tick + 100000;
      step(9);
    }
    W.sword_strong = false;
    W.link.current.pos = {0, 0, -180};
    a->setActionMode(6, 1);
    inject_cut(a, P::CUT_TYPE_VERTICAL, 3);
    step();
    CHECK(a->health == hp - 3);  // flank/back: no omniscient guard
  }
  // (C) the stagger never shows or presents a guard: the Shield Attack's native DRAWBACK becomes the stagger clip, TgShield is never raised, Helm kills
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    const std::size_t from = a->acts.size();
    W.shield_attack_hit = true;
    step(3);
    CHECK(s->brk.shield_prep && a->checkHeadLockFlg() && a->field_0x764 == kk::policy::kBckDamageBow && s->st.stagger_clip_kept == 1 && count_log("what=STAGGER_CLIP_KEPT") == 1);
    a->mCyl.tg_shield_on_calls = 0;
    step(8);
    link_swing(P::CUT_TYPE_HEAD_JUMP, 6, 0x30);
    for (int i = 0; i < 8 && a->mActionMode != 10; ++i) {
      step();
      CHECK(!a->mCyl.tg_shield);
    }
    CHECK(a->mActionMode == 10 && a->mCyl.tg_shield_on_calls == 0 && s->st.stagger_shield_closed >= 1 && count_log("cause=HELM_SPLITTER") == 1);
    for (std::size_t i = from; i < a->acts.size(); ++i) CHECK(a->acts[i] != 6);
  }
  // (D1) B on a broken Chilfos in front: Link's NATIVE Mortal Draw (procCutFinishInit(4): PROC_CUT_FINISH, cut 0x1F); it is alive at the press, dies on contact
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    link_cut_action();
    CHECK(W.link.mProcID == daAlink_c::PROC_CUT_FINISH && W.link.cut == P::CUT_TYPE_MORTAL_DRAW_B && W.last_finish == kk::policy::kFinishMortalDrawB && W.finish_inits == 1);
    CHECK(s->st.md_started == 1 && s->md_active && a->mActionMode != 10 && !s->dead && count_log("event=START_NATIVE") == 1);
    link_cut_action();  // B again during the Mortal Draw: no second request
    W.link.cut = P::CUT_TYPE_MORTAL_DRAW_B, W.link.mProcID = daAlink_c::PROC_CUT_FINISH;
    CHECK(s->st.md_started == 1);
    step(6);
    CHECK(a->mActionMode == 10 && s->dead && s->st.md_kills == 1 && count_log("route=MORTAL_DRAW") == 1 && count_log("CHILFOS_EXEC event=KILL") == 1);
  }
  // (D2) B when it is not broken: Link's ordinary cut, untouched
  {
    daE_KK_c* a = fight(id++);
    (void)a;
    link_cut_action();
    CHECK(W.link.cut == P::CUT_TYPE_VERTICAL && W.link.mProcID == daAlink_c::PROC_CUT_NORMAL && W.finish_inits == 0);
  }
  // (D3) a Mortal Draw that never touches it (Link 300 away, beyond the sword): no death; when Link's swing ends it is cancelled; the stagger ends normally
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 300};
    const int hp = a->health;
    link_cut_action();
    CHECK(s->st.md_started == 1);
    step(10);
    CHECK(!s->dead && a->health == hp);
    W.link.mProcID = daAlink_c::PROC_WAIT, W.link.cut = 0;
    step(2);
    CHECK(!s->md_active && s->st.md_cancelled == 1 && count_log("reason=LINK_LEFT_MORTAL_DRAW") == 1);
    until(a, 200, [](daE_KK_c* x) { return !slot_of(x)->brk.active(); });
    CHECK(!s->dead && a->health == hp && s->st.recovered == 1);
  }
  // (D4) after a Shield Attack prepared Helm Splitter, B is not taken over (the Helm path keeps its priority)
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    W.shield_attack_hit = true;
    step(3);
    CHECK(s->brk.shield_prep);
    link_cut_action();
    CHECK(W.finish_inits == 0 && s->st.md_started == 0);
  }
  // (D5) Link facing away from the broken Chilfos: no Mortal Draw
  {
    daE_KK_c* a = broken(id++);
    W.link.shape_angle.y = 0;  // Link stands at +z: facing +z is facing away
    link_cut_action();
    CHECK(W.finish_inits == 0 && slot_of(a)->st.md_started == 0);
  }
  // (D6) two broken Chilfos: only the nearest one in front is the target
  {
    reset_world();
    W.reach = 240, W.link.current.pos = {0, 0, 180};
    daE_KK_c* a = spawn(id++, 0, 0);
    daE_KK_c* b = spawn(id++, 0, 80);
    step(2);
    KkSlot* sa = slot_of(a);
    KkSlot* sb = slot_of(b);
    sa->rest_until = sb->rest_until = W.tick + 100000;
    kk_enter_break(*sa, a, "LAB"), kk_enter_break(*sb, b, "LAB");
    step(2);
    link_cut_action();
    CHECK(sb->md_active && !sa->md_active && sb->st.md_started == 1 && sa->st.md_started == 0);
  }
  // (D7) a Mortal Draw started at the end of the window may finish (at most kMdExtendMax more ticks), then the stagger ends
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 300};
    const std::uint64_t start = s->brk.start;
    while (W.tick - s->brk.start < kk::policy::kBreakTicks - 2) step();
    link_cut_action();
    W.link.cut = P::CUT_TYPE_MORTAL_DRAW_B, W.link.mProcID = daAlink_c::PROC_CUT_FINISH;
    step(10);
    CHECK(s->brk.active() && s->st.md_extended == 1 && W.tick - s->brk.start > kk::policy::kBreakTicks);
    W.link.mProcID = daAlink_c::PROC_WAIT, W.link.cut = 0;
    step(3);
    CHECK(!s->brk.active() && !s->dead && s->st.recovered == 1 && W.tick - start <= kk::policy::kBreakTicks + kk::policy::kMdExtendMax + 3);
  }
  // (E) 13 C3 spear releases with real spear children: one child each, no slot taken, one log line for all, a frame-pacing window per release;
  // a spike injected after the release is measured, the module's own time is reported apart
  {
    const std::uint64_t children0 = kz.spear_children, windows0 = kz.perf.windows;
    int spawned = 0, child_lines = 0;
    for (int n = 0; n < 13; ++n) {
      daE_KK_c* a = run_to_pattern(2, 7000 + std::uint32_t(n) * 100);
      KkSlot* s = slot_of(a);
      W.spawn_children = true;
      if (n == 12) W.spike_qpc = 100000;  // the last release: ten presented frames 10 ms slower
      for (int k = 0; k < 900 && s->ep.active; ++k) {
        const int before = W.children_spawned;
        step();
        if (W.children_spawned != before && n == 12) W.spike_frames = 10;
      }
      step(60);
      spawned += W.children_spawned;
      child_lines += count_log("reason=SPEAR_CHILD");  // summed over the 13 worlds (each reset clears the lab log)
      CHECK(W.children_spawned == 1 && s->st.spawns == 1 && s->st.spawn_dupes == 0 && s->st.rearmed == 1);
      unsigned used = 0;
      for (auto& x : kz.slots) used += x.on ? 1u : 0u;
      CHECK(used == 1);  // the thrown spear never took a slot
    }
    CHECK(spawned == 13 && kz.spear_children - children0 == 13 && child_lines <= 1);  // one line per session, never one per thrown spear
    CHECK(kz.perf.windows - windows0 == 13 && count_log("CHILFOS_PERF event=THROW_WINDOW") >= 1 && count_log("gpu=GPU_NOT_MEASURED") >= 1);
    CHECK(count_log("slow_frames=10 ") == 1 && count_log("post_max_ms=26.67") == 1 && count_log("clock=QPC_CALIBRATED") >= 1);
  }
}
// ---------------------------------------------------------------- J/L: lifecycle, instances, events, budget
// ---------------------------------------------------------------- FIX04 0.13.4
static void throw_scenario(std::uint32_t id, cXyz link, float floor_y, int kcl, bool& spawned, std::uint64_t& max_strides) {
  daE_KK_c* a = run_to_pattern(2, id);
  CHECK(a);
  until(a, 400, [](daE_KK_c* x) { return slot_of(x)->ep.ph == KkEp::Back; });
  W.floor_y = floor_y, W.kcl = kcl, W.los_strides = W.los_calls = W.los_max = 0;
  W.link.current.pos = link;
  W.eye_after_throw_min = 1e9f, W.eye_probe_calls = 0;
  until(a, 120, [](daE_KK_c* x) { return x->field_0x67d == 1 || !slot_of(x)->ep.active; });
  spawned = W.spawns == 1;
  max_strides = W.los_max;
  CHECK(a->eyePos.y == 150.0f && W.eye_after_throw_min == 150.0f && W.eye_probe_calls > 0);  // restored after every native call (before cc_set rewrites it)
}
static void lab_fix04_spam();
static void lab_fix04() {
  // ---- P0: the C3 wind-up casts no giant BG line any more; the planned spear still flies (Link close, on a step, below, far, 1 and 8 KCL)
  {
    CHECK(kk::los_strides(1.0e9f + 250.0f) >= 2000000ull);  // what 0.13.3's eye at -1e9 cost: ~2 000 000 strides per KCL per wind-up Execute
    CHECK(kk::throw_eye(150.0f, 0.0f) == -60.0f && kk::throw_eye(-200.0f, 0.0f) == -200.0f && kk::throw_eye(150.0f, 400.0f) == 150.0f);
    struct Case { cXyz link; float floor; int kcl; bool checked; };
    const Case cases[] = {{{0, 0, 300}, 0, 1, true}, {{0, 0, 880}, 0, 1, true}, {{0, 150, 300}, 0, 1, true}, {{0, -120, 300}, 0, 1, true}, {{0, 0, 300}, 0, 8, true},
                          {{0, 0, 1000}, 0, 1, false}};
    std::uint32_t id = 9000;
    for (const Case& c : cases) {
      bool spawned = false;
      std::uint64_t mx = 0;
      throw_scenario(id, c.link, c.floor, c.kcl, spawned, mx);
      id += 100;
      CHECK(spawned);                                                   // the melee cancel never dropped the planned throw
      CHECK(mx <= 3ull * std::uint64_t(c.kcl));                         // vanilla-sized segment: at most 3 strides per KCL per call
      CHECK(c.checked ? W.los_calls > 0 : W.los_calls == 0);            // Link within 900: the native test really ran (and stayed cheap)
      CHECK(W.los_strides <= 2ull * 3ull * std::uint64_t(c.kcl) * W.los_calls);
      CHECK(count_log("CHILFOS_PERF event=THROW_WINDUP") == 1);
    }
    // the frame-pacing window opens at THROW_START: its 90 frames are the wind-up and the release
    CHECK(kz.perf.open || count_log("anchor=THROW_START") >= 1);
  }
  // ---- P1: one native clash and one spark per attack, whatever number of spheres touch, no stale replay 8 ticks later; every attack keeps its contact
  for (int cs : {0, 2, 6}) {
    daE_KK_c* a = fight(9700 + std::uint32_t(cs), false);
    KkSlot* s = slot_of(a);
    W.guarding = true, W.contact_spheres = cs;
    until(a, 3000, [](daE_KK_c* x) { return slot_of(x)->st.impacts_closed >= 12; });
    W.reach = 0, step(20);  // no new contact; past the native 8-tick timer of the last window
    const int n = a->lab_contact_windows;
    CHECK(n >= 8);
    CHECK(a->hit_se40 + int(s->st.se_self) == n);          // exactly one clash per attack that touched the shield
    CHECK(a->lab_marks == n && int(s->st.marks_kept) == n);  // exactly one spark per attack
    CHECK(int(s->st.link_block + s->st.link_parry) == n);    // one receipt per attack: none lost, none doubled (bursts included)
    CHECK(cs == 0 || s->st.se_merged > 0 || s->st.echo_suppressed > 0);
    CHECK(count_log("CHILFOS_CONTACT event=ATTACK") >= 1 && count_log("native_rounds=2") == 0);
  }
  {  // stale flags (a window that closed on a contact; no pass cleared them): no clash, no recoil, no receipt (0.13.3 replayed them 8 ticks late)
    daE_KK_c* a = fight(9740);
    KkSlot* s = slot_of(a);
    step(3);
    CHECK(!s->cc_live && a->field_0x67e == 1);
    for (dCcD_GObjInf* x : {static_cast<dCcD_GObjInf*>(&a->mSph), static_cast<dCcD_GObjInf*>(&a->mSpheres[4])}) x->at_hit = &W.link.mTgCyls[0], x->at_shield_hit = true;
    s->imp_resolved = 4, s->imp_serial = 5;  // an impact window still unresolved: a receipt from these flags would be a phantom
    const int se0 = a->hit_se40;
    const float sp = a->speedF;
    a->mDamageTimer = 0;
    step();
    CHECK(a->hit_se40 == se0 && s->st.stale_cleared == 1 && !a->mSph.ChkAtHit() && !a->mSpheres[4].ChkAtHit() && a->speedF == sp && s->st.link_block == 0);
  }
  {  // body contacts (no shield): one spark and one direct offer per attack
    daE_KK_c* a = fight(9750, false);
    KkSlot* s = slot_of(a);
    W.guarding = false, W.contact_spheres = 3;
    until(a, 3000, [](daE_KK_c* x) { return slot_of(x)->st.impacts_closed >= 8; });
    W.reach = 0, step(20);
    CHECK(a->lab_contact_windows >= 6 && a->lab_marks == a->lab_contact_windows && int(s->st.link_direct_offered) == a->lab_contact_windows && a->hit_se40 == 0);
  }
  {  // geometry: out of reach -> nothing at all; the same attacks in reach -> every one counted (no false positive, no false negative)
    daE_KK_c* a = fight(9760, false);
    KkSlot* s = slot_of(a);
    W.guarding = true, W.reach = 100;  // Link at 180: the tip never reaches
    until(a, 2000, [](daE_KK_c* x) { return slot_of(x)->st.impacts_closed >= 6; });
    CHECK(a->lab_contact_windows == 0 && a->hit_se40 == 0 && W.marks == 0 && s->st.link_block == 0 && s->st.link_parry == 0);
  }
  {  // the native timer of an older event mutes nothing: the new attack's first clash sounds once, now (module SE), and is never replayed late
    daE_KK_c* a = fight(9770, false);
    KkSlot* s = slot_of(a);
    W.guarding = true;
    until(a, 2000, [](daE_KK_c* x) { return x->field_0x67e == 0 && x->mActionMode == 8; });
    a->mDamageTimer = 6;  // e.g. Link's sword hit it 2 ticks ago
    const int se0 = a->hit_se40, c0 = a->mCreatureSound.collision_se;
    step(20);
    CHECK(s->st.se_self == 1 && a->hit_se40 == se0 && a->mCreatureSound.collision_se == c0 + 1 && a->mCreatureSound.last_id == Z2SE_HIT_SWORD && a->mCreatureSound.last_map == 0x28);
  }
  {  // two Chilfos on one Link: each attack of each one has its own clash and spark
    reset_world();
    W.reach = 240, W.guarding = true, W.contact_spheres = 2;
    W.link.current.pos = {0, 0, 180};
    daE_KK_c* a = spawn(9780, 0, 0);
    daE_KK_c* b = spawn(9781, 0, 20);
    until(a, 3000, [](daE_KK_c* x) { return slot_of(x) && slot_of(x)->st.impacts_closed >= 8; });
    W.reach = 0, step(20);
    for (daE_KK_c* x : {a, b}) CHECK(x->lab_contact_windows >= 4 && x->hit_se40 + int(slot_of(x)->st.se_self) == x->lab_contact_windows);
  }
  // ---- P2: a block or parry whose native branch is the strong one gets the native guard spark from the module, the weak one keeps the native's own;
  //          the parry is settled AFTER its spark exists; the strong clash (def_se_set map 0x1F) is the one the parry pitch looks for
  for (int strong = 0; strong < 2; ++strong)
    for (kk::V v : {kk::V::Block, kk::V::EnemyParry, kk::V::Damage}) {
      daE_KK_c* a = fight(9800 + std::uint32_t(strong) * 10 + std::uint32_t(v));
      KkSlot* s = slot_of(a);
      link_swing(32);
      force_verdict(a, v);
      W.swing_len = 0;
      g_hit = cCcD_Obj{};
      g_hit.at_type = kAtSword, g_hit.owner = &W.link, g_hit.strong = strong != 0;
      a->mDamageTimer = 0, a->mCyl.tg_hit = &g_hit;
      W.seq.clear(), W.native_guard_marks = W.guard_marks_mod = W.clash_se = 0;
      step();
      const int marks = W.native_guard_marks + W.guard_marks_mod;
      if (v == kk::V::Damage) {
        CHECK(marks == 0);
      } else {
        CHECK(marks == 1 && (strong ? W.guard_marks_mod == 1 && W.clash_se == 1 : W.native_guard_marks == 1 && W.clash_se == 0));
        CHECK(a->field_0x67d == 0 && a->mActionMode == 6);
      }
      if (v == kk::V::EnemyParry) {
        CHECK(W.seq.size() == 2 && W.seq[0] == (strong ? "MOD_MARK" : "NATIVE_MARK") && W.seq[1] == "SETTLE_ENEMY_PARRY" && s->st.parry_settle_post == 1);
        CHECK(W.clash_map == 7);  // back to the guard map after the call
      }
    }
  {  // 200 contacts alternating strong / weak, block / parry: always exactly one spark
    daE_KK_c* a = fight(9850);
    int sparks = 0;
    for (int i = 0; i < 200; ++i) {
      link_swing(32);
      force_verdict(a, (i & 1) ? kk::V::EnemyParry : kk::V::Block);
      W.swing_len = 0;
      g_hit = cCcD_Obj{};
      g_hit.at_type = kAtSword, g_hit.owner = &W.link, g_hit.strong = (i % 3) == 0;
      a->mDamageTimer = 0, a->mCyl.tg_hit = &g_hit;
      const int m0 = W.native_guard_marks + W.guard_marks_mod;
      step();
      sparks += (W.native_guard_marks + W.guard_marks_mod) - m0 == 1 ? 1 : 0;
      a->setActionMode(0, 0), a->health = 300, slot_of(a)->p.u = 0, slot_of(a)->counter_pending = false;
      step(10);
    }
    CHECK(sparks == 200);
  }
  // ---- P3: it watches Link while it waits; the roll of a swing is taken when Link starts it and raises the native guard; B spam does not win cheaply
  {
    daE_KK_c* a = fight(9900);
    for (int k = 0; k < 120; ++k) {  // Link walks around it at 0x300 per tick (a slow side-step): it keeps him in its front arc after settling
      link_at(a, s16(0x2000 + 0x300), 200.0f);  // he steps ahead of its facing every tick
      step();
      if (k > 20) CHECK(kk::angle_diff(a->shape_angle.y, angle_to_link(a)) <= kk::policy::kFrontalArc);
    }
    CHECK(slot_of(a)->st.face_ticks > 0);
  }
  {  // the roll is taken when Link STARTS the swing: a covered swing raises the native guard before the blade arrives, the contact is a block
    daE_KK_c* a = fight(9905);
    KkSlot* s = slot_of(a);
    step(30);
    CHECK(a->mActionMode == kk::kWait);
    s->def.uncovered = kk::policy::kGuaranteeAfter, s->def.last_uncovered = W.tick - kk::policy::kWoundReadyDelay;  // block_pct 100
    s->def.cooldown_until = W.tick + 1000;  // LFT 0.13.6: this case is the guard raise, never the 12% parry roll (the RNG stream is not part of the contract)
    const int hp = a->health;
    link_swing(1);
    CHECK(s->raise_serial == kz.swing_serial && s->raise_ok && s->st.guard_raised == 1 && a->mActionMode == kk::kGuard);
    step(6);
    CHECK(a->health == hp && s->st.blocks == 1 && (count_log("guard_state=ACTIVE") + count_log("guard_state=RAISED")) == 1);
    // one roll per swing: a failed raise is a hit, never a second chance at contact (even with a 100% roll available)
    a->setActionMode(0, 0);
    step(30);
    const auto b0 = s->st.blocks;
    link_swing(1);
    s->raise_serial = kz.swing_serial, s->raise_ok = false, a->setActionMode(0, 1);  // that swing's roll failed
    step(6);
    CHECK(s->st.blocks == b0 && a->health < hp && count_log("guard_state=RAISE_FAILED") == 1);
  }
  {  // during its own attack nothing is raised (committed), and the guard never turns into a 360 shield: from the back the guard does not cover
    daE_KK_c* a = fight(9910, false);
    KkSlot* s = slot_of(a);
    until(a, 400, [](daE_KK_c* x) { return slot_of(x)->ep.active && x->mActionMode == 8; });
    const auto raised = s->st.guard_raised;
    link_swing(1);
    step();
    CHECK(s->st.guard_raised == raised && a->mActionMode == 8 && s->ep.active);
  }
  lab_fix04_spam();
}
// FIX04 P3: 60 s of repeated B against a Chilfos that fights back (frontal, slow circle, fast flank). Also built alone (-DKK_METRICS_ONLY) over 0.13.3's defense paths.
static void lab_fix04_spam() {
  auto spam = [](std::uint32_t id, int mode, int seconds, int& swings, int& ready_dmg, int& dmg, int& blocks, int& parries, int& counters, int& raised, int& trade, int& link_hit, int& ready_sw) {
    daE_KK_c* a = fight(id, false);  // free to fight back: its combos, counters and rests run as in the game
    KkSlot* s = slot_of(a);
    swings = ready_dmg = dmg = trade = ready_sw = link_hit = 0;
    int seen_hits = W.link_hits;
    std::uint64_t stunned_until = 0;  // Link's native damage proc: a spear hit interrupts him (no swing for ~20 ticks)
    s16 orbit = 0;
    for (int tick = 0; tick < seconds * 30; ++tick) {
      if (mode == 1) orbit = s16(orbit + 0x300);  // slow lateral circle
      if (mode == 2) orbit = s16(orbit + 0x1200);  // a fast flanking circle (rolls / side hops)
      if (mode) {
        const double h = double(orbit) * 3.14159265358979 / 32768.0;
        W.link.current.pos = {float(180.0 * std::sin(h)), 0, float(180.0 * std::cos(h))};
        W.link.shape_angle.y = s16(orbit + 0x8000);
      }
      if (W.link_hits != seen_hits) {
        if (W.tick >= stunned_until) ++link_hit;  // one interruption per hit reaction
        seen_hits = W.link_hits, stunned_until = W.tick + 20;
      }
      if (tick % 12 == 0 && W.tick >= stunned_until) {
        const auto d0 = s->st.verdict[unsigned(kk::V::Damage)];
        unsigned long long v0 = 0, v1 = 0;
        for (auto x : s->st.verdict) v0 += x;
        bool in_attack = s->ep.active;
        link_swing(1);
        for (int k = 0; k < 5; ++k) step(), in_attack = in_attack || s->ep.active;
        for (auto x : s->st.verdict) v1 += x;
        const bool hurt = s->st.verdict[unsigned(kk::V::Damage)] > d0;
        dmg += hurt, ++swings;
        if (v1 > v0 && s->dec_ready_frontal) ++ready_sw;  // the contact met a ready Chilfos whose front faced Link
        if (hurt && s->dec_ready_frontal) ++ready_dmg;
        if (hurt && !s->dec_ready_frontal && in_attack) ++trade;  // its own committed attack: a trade (its spear is out too)
        tick += 4;
      } else {
        step();
      }
      if (a->health <= 60) a->health = 300;
      if (s->brk.active() || s->brk.st == kk::Bk::Pending) s->brk.reset(), s->p.reset(), a->setActionMode(0, 0);
    }
    blocks = int(s->st.blocks), parries = int(s->st.enemy_parries), counters = int(s->st.counter_dispatched), raised = int(s->st.guard_raised + s->st.guard_after_wound);
    (void)a;
  };
  for (int mode = 0; mode < 3; ++mode) {
    int sw = 0, rd = 0, dm = 0, bl = 0, pa = 0, co = 0, ra = 0, tr = 0, lh = 0, rs = 0;
    spam(9920 + std::uint32_t(mode), mode, 60, sw, rd, dm, bl, pa, co, ra, tr, lh, rs);
    std::printf("KK_FIX04_METRICS scenario=B_SPAM_%s seconds=60 swings=%d contacts_on_ready_front=%d wounds_through_ready_front=%d damage=%d trades_in_its_attacks=%d flank_or_open=%d "
                "link_hit_reactions=%d blocks=%d enemy_parries=%d counters=%d guards_raised=%d\n",
                mode == 0 ? "FRONTAL" : mode == 1 ? "SLOW_CIRCLE" : "FAST_FLANK", sw, rs, rd, dm, tr, dm - tr - rd, lh, bl, pa, co, ra);
    CHECK(sw > 50 && dm > 0);                             // never invulnerable: its own attacks and the openings still take hits
    CHECK(tr == 0 || lh > 0);                             // a hit inside its committed attack is a trade: its spear meets Link too
    if (mode < 2) CHECK(rd * 100 <= (rs > 0 ? rs : 1) * 15);  // a ready front Link faces is beaten by at most ~1 contact in 7 under repeated B
    if (mode == 2) CHECK(dm - tr > 0);                    // a real flank still pays outside its attacks: authentic, harder, winnable
  }
}
// ---------------------------------------------------------------- FIX05 0.13.5
// Link's Jump Attack (d_a_alink_cut.inc): procCutJumpInit -> setCutType(JUMP) in PROC_CUT_JUMP, airborne `air` ticks with the sword AT on in its last
// ticks; procCutJumpLandInit -> setCutType(JUMP) again in PROC_CUT_JUMP_LAND with the landing AT. An aerial rebound (procCutJump: heading reversed, speed 27)
// sends Link away: his landing AT then reaches nothing.
struct JumpOut { bool contact = false, air_rebound = false, land_rebound = false; };
static JumpOut link_jump(int air = 12, int land_len = 3, bool aerial_at = true) {
  JumpOut o;
  const int rb0 = W.air_rebounds, lr0 = W.land_rebounds;
  W.sword_strong = true;  // the jump's AT is spl 1: the native strong branch
  link_swing(daPy_py_c::CUT_TYPE_JUMP, aerial_at ? 4 : 0, daAlink_c::PROC_CUT_JUMP);
  W.swing_start = W.tick + std::uint64_t(air - 6);  // the aerial AT: the last 4 airborne ticks
  for (int i = 0; i < air; ++i) step();
  o.air_rebound = W.air_rebounds > rb0;
  link_swing(daPy_py_c::CUT_TYPE_JUMP, o.air_rebound ? 0 : land_len, daAlink_c::PROC_CUT_JUMP_LAND);  // procCutJumpLandInit: setCutType(JUMP) again
  W.swing_start = W.tick - 2;
  for (int i = 0; i < land_len + 3; ++i) step();
  o.land_rebound = W.land_rebounds > lr0;
  W.link.mProcID = daAlink_c::PROC_WAIT, W.sword_strong = false;
  if (W.link.shape_angle.y != s16(0x8000)) W.link.shape_angle.y = s16(0x8000);  // he faces it again for the next jump
  return o;
}
static std::uint64_t jump_verdicts(KkSlot* s) {
  std::uint64_t n = 0;
  for (auto x : s->st.verdict) n += x;
  return n;
}
static void lab_fix05_jump_spam();
#ifndef KK_METRICS_ONLY
static void lab_fix05() {
  using P = daPy_py_c;
  std::uint32_t id = 9600;
  // ---- P0: the stagger plays ONCE across the window and ends in its own stance; no jump back to frame 0, no second RECOVERED, no early native exit
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    CHECK(s->brk.active() && a->mActionMode == 5 && (a->field_0x764 == 9 || a->field_0x764 == 0xB));
    CHECK(s->st.bow_plans == 1 && a->morf.mFrameCtrl.mLoop == a->morf.mFrameCtrl.mEnd - 1 && a->morf.mFrameCtrl.mAttribute == 2);
    for (int cycle = 0; cycle < 30; ++cycle) {
      const std::uint64_t start = s->brk.start;
      const int sets0 = a->bck_sets;
      float last = a->morf.getFrame(), at80 = -1, bow_end = a->morf.getEndFrame();
      int drops = 0;
      while (s->brk.active() && W.tick < start + 200) {
        step();
        if (!s->brk.active()) break;
        CHECK(a->mActionMode == 5);  // the native Yoroke never leaves by itself (EMode_LOOP: no stop state)
        const float f = a->morf.getFrame();
        if (a->bck_sets == sets0 && f + 1.5f < last) ++drops;
        last = f;
        if (W.tick - start == 80) at80 = f;
      }
      CHECK(drops == 0);                                                              // the clip never jumped back toward its bow
      CHECK(at80 >= 0 && at80 < bow_end - 1.5f);                        // still staggering 10 ticks before the end: the stance is not reached early
      CHECK(!s->brk.active() && a->mActionMode == 0 && W.tick - start <= kk::policy::kBreakTicks + 1);  // one RECOVERED at 90 ticks, the native wait next
      CHECK(int(s->st.recovered) == cycle + 1 && int(s->st.bow_exit_end) == cycle + 1 && s->st.bow_wraps == 0);  // ended at the clip's own last frame
      CHECK(count_log("event=RECOVERED") == cycle + 1);
      step(3);
      CHECK(a->field_0x764 == 0x1A || a->mActionMode != 5);  // the native alert wait (morf 3): no stagger clip after RECOVERED
      for (int i = 0; i < 30; ++i) {
        step();
        CHECK(a->mActionMode != 5);  // no re-entry of Yoroke without a new break
      }
      // a REAL new break of the same actor: a new stagger with its own plan (correct)
      s->p.u = std::int64_t(97) * kk::policy::kUnitsPerPoint;
      s->def.cooldown_until = W.tick + 100000, s->rest_until = W.tick + 100000;
      a->setActionMode(0, 0);
      for (int i = 0; i < 30 && s->brk.st == kk::Bk::None; ++i) link_swing(), step(12), a->setActionMode(a->mActionMode == 5 ? 5 : 0, a->mActionMode == 5 ? a->mMoveMode : 0);
      step(2);
      if (cycle + 1 < 30) CHECK(s->brk.active() && int(s->st.bow_plans) == cycle + 2 && a->morf.getFrame() < 16.0f);  // a new stagger clip, planned anew
      else s->brk.reset(), s->p.reset();
    }
    // the Shield Attack's restart of the stagger clip (DRAWBACK -> DAMAGE_BOW) is planned again over what is left of the window
    a = broken(id++);
    s = slot_of(a);
    step(30);
    inject_bash(a);
    step(2);
    CHECK(s->brk.shield_prep && s->st.bow_plans == 2 && s->st.bow_wraps == 0);
    W.link.current.pos = {0, 0, 900};  // no finisher comes
    until(a, 120, [](daE_KK_c* x) { return !slot_of(x)->brk.active(); });
    CHECK(s->st.recovered == 1 && s->st.bow_wraps == 0 && s->st.bow_exit_end == 1);
    // a Mortal Draw that started may extend the window: the clip holds its last frame (loop frame = last frame), never back to the bow
    a = broken(id++);
    s = slot_of(a);
    W.link.current.pos = {0, 0, 300};
    while (W.tick - s->brk.start < kk::policy::kBreakTicks - 4) step();
    link_cut_action();
    CHECK(s->md_active);
    for (int i = 0; i < 25 && s->brk.active(); ++i) {
      W.link.cut = P::CUT_TYPE_MORTAL_DRAW_B, W.link.mProcID = daAlink_c::PROC_CUT_FINISH;
      step();
      if (s->brk.active()) CHECK(a->mActionMode == 5 && (a->morf.getFrame() >= a->morf.getEndFrame() - 1.0f - 0.01f || W.tick - s->brk.start < kk::policy::kBreakTicks));
    }
    CHECK(s->st.bow_wraps == 0);
    W.link.mProcID = daAlink_c::PROC_WAIT;
  }
  // ---- P1: Jump Attacks. One serial per jump (takeoff + landing), a native rebound route in the air and on landing, one result and one gain per jump
  {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 200}, W.reach = 240;
    const std::uint64_t ser0 = kz.swing_serial;
    link_jump();
    CHECK(kz.swing_serial == ser0 + 1 && s->st.jump_swings == 1 && s->st.jump_land_merged == 1);  // the landing's setCutType is the same swing
    CHECK(jump_verdicts(s) <= 1);
    // a prepared parry on a jump: Link's aerial AT meets the presented TgShield -> the native aerial rebound (no proc change), the enemy parry, the counter
    int air = 0, land = 0, parries = 0, blocks = 0, dmg = 0, recoil = 0;  // LFT 0.13.6: recoil = landings of air-parried jumps (P2), not landing contacts
    int fx_total = 0;
    std::uint64_t armed_total = 0;
    for (int j = 0; j < 100; ++j) {
      a = fight(id++);
      s = slot_of(a);
      const int fxj = W.land_recoil_fix06;
      const std::uint64_t arj = kk::f6::g_land_recoil.armed_n;
      W.link.current.pos = {0, 0, 200}, W.reach = 240;
      s->rng.seed(7000 + j);
      const int hp0 = a->health, pts0 = s->p.points();
      const auto v0 = s->st.verdict[unsigned(kk::V::EnemyParry)], b0 = s->st.verdict[unsigned(kk::V::Block)], d0 = s->st.verdict[unsigned(kk::V::Damage)];
      s->def.jumps[0] = W.tick, s->def.jumps[1] = W.tick, s->def.jumps[2] = W.tick, s->def.njump = 3;  // its 4th jump of a streak: 75 % parry floor
      const JumpOut o = link_jump(12, 3, (j % 4) != 3);  // every 4th jump has no aerial AT: its contact is the landing (ANM_CUT_RECOIL_B route)
      const bool p = s->st.verdict[unsigned(kk::V::EnemyParry)] > v0, b = s->st.verdict[unsigned(kk::V::Block)] > b0, d = s->st.verdict[unsigned(kk::V::Damage)] > d0;
      CHECK(int(p) + int(b) + int(d) == 1);   // exactly one result per jump
      if (p || b) CHECK(a->health == hp0);    // a raised guard / parry loses no HP
      if (p) {
        CHECK(o.air_rebound || o.land_rebound);
        ++parries;
        step(12);
        CHECK(s->st.counter_dispatched == 1 || s->counter_pending || s->ep.active);
      }
      if (b) {
        CHECK(!o.air_rebound && !o.land_rebound && s->p.points() - pts0 <= kk::policy::kGainBlock && s->st.blocks == 1);  // one block, one gain (block_dupes for the landing)
        ++blocks;
      }
      if (d) {
        CHECK(s->p.points() - pts0 <= kk::policy::kGainSwordHp);
        ++dmg;
      }
      air += o.air_rebound, land += o.land_rebound && !o.air_rebound, recoil += o.land_rebound && o.air_rebound;
      fx_total += W.land_recoil_fix06 - fxj, armed_total += kk::f6::g_land_recoil.armed_n - arj;
      CHECK(W.land_recoil_fix06 - fxj == (o.air_rebound ? 1 : 0));  // LFT 0.13.6 P2: the override only for an air-parried jump, once
      CHECK(s->st.counter_expired == 0);
    }
    std::printf("KK_FIX05_JUMP_MATRIX jumps=100 streak=4 parries=%d air_rebounds=%d land_rebounds=%d blocks=%d damage=%d\n", parries, air, land, blocks, dmg);
    CHECK(parries >= 50 && air > 0 && land > 0 && parries == air + land);  // 75 % floor: real parries through BOTH native rebound routes
    CHECK(recoil == air);  // LFT 0.13.6 P2: every air-parried jump lands into its native ANM_CUT_RECOIL_B, exactly once
    CHECK(fx_total == air && armed_total == std::uint64_t(air));  // and no other jump of the 100 ever got the override
    std::printf("KK_FIX06_LAND_RECOIL jumps=100 air_parried=%d land_recoils=%d landing_contact_parries=%d\n", air, fx_total, land);
    CHECK(dmg > 0 || blocks > 0);                                         // never a parry every time
    // behind: no guard, no parry (its shield covers only its front)
    a = fight(id++);
    s = slot_of(a);
    link_at(a, s16(0x8000), 200);
    s->def.jumps[0] = s->def.jumps[1] = s->def.jumps[2] = W.tick, s->def.njump = 3;
    link_jump();
    CHECK(s->st.verdict[unsigned(kk::V::EnemyParry)] == 0 && s->st.verdict[unsigned(kk::V::Block)] == 0 && s->st.verdict[unsigned(kk::V::Damage)] == 1 && W.reverse_rebounds == 0);
    // out of reach (no contact): no verdict, no parry, no gain (never a parry for a press without a contact)
    a = fight(id++);
    s = slot_of(a);
    W.link.current.pos = {0, 0, 300}, W.reach = 240;
    s->def.jumps[0] = s->def.jumps[1] = s->def.jumps[2] = W.tick, s->def.njump = 3;
    link_jump();
    CHECK(jump_verdicts(s) == 0 && s->st.enemy_parries == 0 && W.reverse_rebounds == 0 && s->p.points() == 0);
    // during its own combo: committed, no guard, no parry (the trade stays)
    a = run_to_pattern(0, id);
    id += 50;
    s = slot_of(a);
    W.link.current.pos = {0, 0, 200}, W.reach = 240;
    until(a, 200, [](daE_KK_c* x) { return x->field_0x67e == 0; });
    s->def.jumps[0] = s->def.jumps[1] = s->def.jumps[2] = W.tick, s->def.njump = 3;
    const auto ep0 = s->st.enemy_parries;
    link_jump(4, 3, true);
    CHECK(s->st.enemy_parries == ep0);
    // the Helm Splitter (HEAD_JUMP) on a broken Chilfos is never treated as a jump to defend: it finishes
    a = broken(id++);
    s = slot_of(a);
    inject_bash(a);
    step(4);
    link_swing(P::CUT_TYPE_HEAD_JUMP, 6, daAlink_c::PROC_CUT_HEAD);
    step(8);
    CHECK(a->mActionMode == 10 && s->st.jump_swings == 0 && s->st.verdict[unsigned(kk::V::EnemyParry)] == 0);
    W.link.mProcID = daAlink_c::PROC_WAIT;
  }
  // ---- P2: the gap of every E-E burst feeds the shared fast shield re-arm (0.13.4: only skipped gap ticks, which a gap-0 burst never has)
  for (unsigned pat = 0; pat < kk::kPatterns; ++pat) {
    daE_KK_c* a = run_to_pattern(pat, 9800 + pat * 50);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 180}, W.reach = 240;
    W.guarding = true, W.guard_attack = true, W.ga_in_window = true;  // Link parries every window (two real presses per burst)
    const kk::Plan plan = kk::make_plan(pat);
    int pairs = 0, fed = 0, outside = 0, parried_second = 0;
    unsigned act = s->ep.act;
    bool open = false;
    int gaps0 = W.burst_gaps;
    std::uint64_t parries_before = s->st.link_parry;
    for (int t = 0; t < 700 && s->ep.active; ++t) {
      step();
      const bool now_open = s->ep.window_open;
      if (s->ep.act != act || now_open != open) {
        if (now_open && !open) {  // a window opened: was the gap before it a burst gap?
          const bool second = s->ep.act > 0 && plan.a[s->ep.act].burst && plan.a[s->ep.act - 1].burst && plan.a[s->ep.act].recorded_s < kk::policy::kBurstMax;
          const int fedn = W.burst_gaps - gaps0;
          if (second) ++pairs, fed += fedn > 0;
          else outside += fedn;
        }
        if (!now_open && open) gaps0 = W.burst_gaps, parries_before = s->st.link_parry;
        act = s->ep.act, open = now_open;
      }
      if (open && s->st.link_parry > parries_before && s->ep.act > 0 && plan.a[s->ep.act].burst && plan.a[s->ep.act - 1].burst && plan.a[s->ep.act].recorded_s < kk::policy::kBurstMax)
        ++parried_second, parries_before = s->st.link_parry;
    }
    W.guarding = W.guard_attack = W.ga_in_window = false;
    std::printf("KK_FIX05_BURST pattern=%u pairs=%d fed=%d parried_second=%d fed_outside=%d\n", pat + 1, pairs, fed, parried_second, outside);
    CHECK(pairs == fed && outside == 0 && parried_second == pairs);  // every burst gap fed, nothing else; the second thrust accepts its own parry
  }
  // ---- P3: the Helm Splitter aim point of a broken, Shield-Attacked Chilfos is its own upright head height (procCutHeadInit: clamp(dy, 30, max))
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    CHECK(s->head_up == g_head_upright);  // sampled in its stance before the break
    step(10);
    const float bowed = a->getHeadLockPos().y - a->current.pos.y;
    CHECK(bowed < g_head_upright - 40.0f);  // the stagger clip really bows the head (the 0.13.4 aim)
    CHECK(!a->checkHeadLockFlg());          // no lift without the native HeadLock (no Shield Attack yet)
    inject_bash(a);
    step(2);
    CHECK(a->checkHeadLockFlg() || s->brk.shield_prep);
    a->onHeadLockFlg();
    step(1);
    const float aim = a->getHeadLockPos().y - W.link.current.pos.y;
    const float h_new = std::clamp(aim, 30.0f, 250.0f), h_old = std::clamp(bowed, 30.0f, 250.0f);
    std::printf("KK_FIX05_HELM bowed_aim=%.1f lifted_aim=%.1f jump_height_0134=%.1f jump_height_0135=%.1f upright=%.1f\n", bowed, aim, h_old, h_new, g_head_upright);
    CHECK(aim >= g_head_upright - 0.5f && s->st.head_lifts == 1);  // the jump is computed from its upright head
    a->getHeadLockPos().y = a->current.pos.y + 400.0f;                 // a native point already above: never lowered
    { void* x[] = {&a}; int r = 1; run_post<KkExec>(x, &r); }
    CHECK(a->getHeadLockPos().y == a->current.pos.y + 400.0f);
    link_swing(P::CUT_TYPE_HEAD_JUMP, 6, daAlink_c::PROC_CUT_HEAD);
    step(8);
    CHECK(a->mActionMode == 10 && count_log("cause=HELM_SPLITTER") >= 1);  // the kill on contact is intact
    W.link.mProcID = daAlink_c::PROC_WAIT;
    // not broken: native aim kept
    a = fight(id++);
    step(3);
    CHECK(a->getHeadLockPos().y - a->current.pos.y == g_head_upright && slot_of(a)->st.head_lifts == 0);
  }
  // ---- P4: after the Mortal Draw kill, Link's torso (setBodyAngleXReadyAnime toward mTargetedActor->eyePos) does not follow the falling body
  {
    auto run = [](daE_KK_c* a, int& dip) {
      W.link.mTargetedActor = a;
      link_cut_action();
      CHECK(W.link.mProcID == daAlink_c::PROC_CUT_FINISH && slot_of(a)->md_active);
      float at_kill = -1;
      int target_kill = 0, target = 0;
      dip = 0;
      for (int i = 0; i < 40; ++i) {
        if (i < 30) W.link.cut = daPy_py_c::CUT_TYPE_MORTAL_DRAW_B, W.link.mProcID = daAlink_c::PROC_CUT_FINISH;
        else W.link.mProcID = daAlink_c::PROC_WAIT;
        step();
        // getBodyAngleXAtnActor: the angle of (target eye - Link's chest) below the horizontal; setBodyAngleXReadyAnime chases it
        if (W.link.mProcID == daAlink_c::PROC_CUT_FINISH && W.link.mTargetedActor) {
          const float dy = W.link.mTargetedActor->eyePos.y - (W.link.current.pos.y + 100.0f) - 40.0f;
          const float dx = std::hypot(W.link.mTargetedActor->eyePos.x - W.link.current.pos.x, W.link.mTargetedActor->eyePos.z - W.link.current.pos.z);
          target = int(-std::atan2(dy, dx) * 32768.0 / 3.14159265358979);
          W.link.mBodyAngle.x = s16(W.link.mBodyAngle.x + (target - W.link.mBodyAngle.x) / 4);
        } else {
          W.link.mBodyAngle.x = s16(W.link.mBodyAngle.x - W.link.mBodyAngle.x / 4);
        }
        if (a->mActionMode == 10 && at_kill < 0) at_kill = W.link.mBodyAngle.x, target_kill = target;
        if (at_kill >= 0) dip = std::max(dip, int(W.link.mBodyAngle.x - std::max(int(at_kill), target_kill)));  // lower than the aim it had when the blade met
      }
      W.link.mTargetedActor = nullptr;
    };
    daE_KK_c* a = broken(id++);
    int dip = 0;
    run(a, dip);
    std::printf("KK_FIX05_MD_POSE torso_dip_after_kill=%d (s16 units; 0x400 = 5.6 degrees)\n", dip);
    CHECK(a->mActionMode == 10 && slot_of(a)->st.md_kills == 1 && slot_of(a)->st.md_pose_holds == 1);
    CHECK(dip <= 0x80);                     // no dive of the torso after the kill
    CHECK(count_log("event=POSE_HOLD_END") == 1);
    step(3);
    CHECK(a->eyePos.y < a->current.pos.y + 100.0f);  // the hold is over: the native eye (on the ground) is back, nothing persists
    // another death (Helm Splitter): no hold
    a = broken(id++);
    inject_bash(a);
    step(2);
    link_swing(daPy_py_c::CUT_TYPE_HEAD_JUMP, 6, daAlink_c::PROC_CUT_HEAD);
    step(8);
    CHECK(a->mActionMode == 10 && slot_of(a)->st.md_pose_holds == 0 && !slot_of(a)->md_hold);
    W.link.mProcID = daAlink_c::PROC_WAIT;
  }
  lab_fix05_jump_spam();
}
#endif
// 60 s of repeated Jump Attacks from the front, Link interrupted ~20 ticks by every spear hit. Also built alone (-DKK_METRICS_ONLY) over 0.13.4's sources.
static void lab_fix05_jump_spam() {
  for (int mode = 0; mode < 2; ++mode) {
    daE_KK_c* a = fight(9990 + std::uint32_t(mode), false);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, mode ? 230.0f : 200.0f}, W.link.shape_angle.y = s16(0x8000), W.reach = 240;
    int jumps = 0, link_hit = 0, hp_lost = 0, posture_gain = 0, breaks = 0;
    int seen = W.link_hits;
    std::uint64_t stunned = 0;
    for (int tick = 0; tick < 60 * 30;) {
      if (W.link_hits != seen) {
        if (W.tick >= stunned) ++link_hit;
        seen = W.link_hits, stunned = W.tick + 20;
      }
      if (W.tick >= stunned && tick % 36 == 0) {
        const int hp0 = a->health, p0 = s->p.points();
        ++jumps;
        link_jump();
        tick += 18;
        hp_lost += hp0 - a->health;
        posture_gain += std::max(0, s->p.points() - p0);
      } else {
        step(), ++tick;
      }
      W.link.current.pos = {0, 0, mode ? 230.0f : 200.0f};
      if (a->health <= 60) a->health = 300;
      if (s->brk.active() || s->brk.st == kk::Bk::Pending) ++breaks, s->brk.reset(), s->p.reset(), a->setActionMode(0, 0);
    }
    const auto par = s->st.verdict[unsigned(kk::V::EnemyParry)], blk = s->st.verdict[unsigned(kk::V::Block)], dmg = s->st.verdict[unsigned(kk::V::Damage)];
    std::printf("KK_FIX05_METRICS scenario=JUMP_SPAM_%s version=%s seconds=60 jumps=%d enemy_parries=%llu blocks=%llu damage=%llu hp_lost=%d posture_gained=%d breaks=%d counters=%llu "
                "counter_expired=%llu link_hit_reactions=%d rebounds=%d\n",
                mode ? "EDGE" : "FRONT", kk::kVersion, jumps, static_cast<unsigned long long>(par), static_cast<unsigned long long>(blk), static_cast<unsigned long long>(dmg), hp_lost, posture_gain,
                breaks, static_cast<unsigned long long>(s->st.counter_dispatched), static_cast<unsigned long long>(s->st.counter_expired), link_hit, W.reverse_rebounds);
#ifndef KK_METRICS_ONLY
    CHECK(par > 0 && W.reverse_rebounds > 0 && s->st.counter_expired == 0);  // the spam is answered by real parries (native rebound) and counters
    CHECK(dmg > 0 || blk > 0);                                              // never a parry every time
#endif
  }
}
static void lab_lifecycle() {
  reset_world();
  W.link.current.pos = {0, 0, 400}, W.reach = 0;
  daE_KK_c* a = spawn(1800);
  daE_KK_c* b = spawn(1801, 0, 30);
  step(80);
  CHECK(slot_of(a)->admitted && slot_of(b)->admitted && slot_of(a)->generation != slot_of(b)->generation);
  W.safe = false;  // an event: the episode ends at once, nothing is forced
  step(2);
  CHECK(!slot_of(a)->ep.active && !slot_of(b)->ep.active);
  W.safe = true;
  // address reuse by a new process: nothing is inherited
  const std::uint64_t g0 = slot_of(a)->generation;
  slot_of(a)->p.u = std::int64_t(50) * kk::policy::kUnitsPerPoint;
  a->pid = 1899;
  step(1);
  CHECK(slot_of(a)->generation != g0 && slot_of(a)->p.points() == 0);
  // delete mid-episode
  step(60);
  a->_delete();
  CHECK(!slot_of(a));
  // budget: an hour of fight never prints more than the caps
  unsigned total = 0;
  for (unsigned i = 0; i < 16; ++i) total += kKkCaps[i];
  W.log.clear();
  step(3000);
  CHECK(W.log.size() <= total);
}
template <class... T> void clear_regs() { ((Reg<T>::pre.clear(), Reg<T>::post.clear()), ...); }

// ======================================== LFT 0.13.6 FIX06: every case below FAILS on 0.13.5 (its mutants in tests/test_kk_mutations.py prove it)
static void js_contact(daE_KK_c* a, int cut, int power, int atp) {  // one Jump Strike phase of Link's sword reaching the body cylinder this pass
  inject_cut(a, cut, power);
  g_hit.atp = static_cast<std::uint8_t>(atp);
  force_verdict(a, kk::V::Damage);  // an uncovered contact (the defense cases set their own verdict)
}
static void lab_fix06() {
  using P = daPy_py_c;
  std::uint32_t id = 9800;
  const int ord = kk::f6::ordinary_of(400, 4), js = kk::f6::js_damage(ord);
  CHECK(ord == 20 && js == 35 && kk::f6::ordinary_of(20, 2) == 20 && kk::f6::js_damage(20) > 20 && kk::f6::js_damage(20) * 2 <= 20 * 2 * 2);  // 1.75x: more than a cut, bounded
  // ---- P0 (A) the log's case: Chilfos HP 280, posture 0, Jump Strike cut 19 at the native 400 (Master Sword) -> alive, 245, +8 once
  {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    a->health = 280;
    W.link.current.pos = {0, 0, -180};
    step(3);
    ++kk::f6::g_js_episode;
    js_contact(a, P::CUT_TYPE_LARGE_JUMP, 400, 4);
    step();
    CHECK(!s->dead && a->mActionMode != 10 && a->health == 280 - js && s->p.points() == kk::f6::kGainJsKk && count_log("event=NATIVE_DEATH") == 0);
    CHECK(s->st.js_credited == 1 && s->st.js_headroom_miss == 0 && count_log("JUMP_STRIKE event=CONTACT family=CHILFOS") == 1 && count_log("result=NON_LETHAL") == 1);
    // the SAME Jump Strike: its landing (cut 19 again) and landing shockwave (cut 20) never credit again: no HP, no posture
    step(9);
    js_contact(a, P::CUT_TYPE_LARGE_JUMP, 400, 4);
    step();
    step(9);
    js_contact(a, P::CUT_TYPE_LARGE_JUMP_FINISH, 400, 4);
    step();
    CHECK(!s->dead && a->health == 280 - js && s->p.points() == kk::f6::kGainJsKk && s->st.js_dupes == 2 && s->st.js_credited == 1);
    // a NEW Jump Strike credits again
    step(9);
    ++kk::f6::g_js_episode;
    js_contact(a, P::CUT_TYPE_LARGE_JUMP_FINISH, 400, 4);
    step();
    CHECK(!s->dead && a->health == 280 - 2 * js && s->p.points() == 2 * kk::f6::kGainJsKk && s->st.js_credited == 2);
  }
  // ---- P0 (B) every phase 18/19/20, HP near max, low, at the floor and at 1: never below the E_KK living floor, never a death
  for (int cut : {0x12, 0x13, 0x14})
    for (int hp0 : {340, 300, 36, 30, 3, 2, 1}) {
      daE_KK_c* a = fight(id++);
      KkSlot* s = slot_of(a);
      a->health = static_cast<s16>(hp0);
      W.link.current.pos = {0, 0, -180};
      step(3);
      ++kk::f6::g_js_episode;
      const int power = cut == 0x12 ? 20 : 400, atp = cut == 0x12 ? 2 : 4;
      js_contact(a, cut, power, atp);
      step();
      const int want = hp0 <= 1 ? hp0 : kk::f6::js_target(hp0, kk::f6::js_damage(kk::f6::ordinary_of(power, atp)), kk::f6::kFloorKk);
      CHECK(!s->dead && a->mActionMode != 10 && a->health == want && a->health >= (hp0 <= 1 ? hp0 : 2) && count_log("event=NATIVE_DEATH") == 0);
    }
  // ---- P0 (C) at the floor the Jump Strike cannot finish it; an ordinary cut that lands does (HP and posture stay independent)
  {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    a->health = 10;
    W.link.current.pos = {0, 0, -180};
    step(3);
    ++kk::f6::g_js_episode;
    js_contact(a, P::CUT_TYPE_LARGE_JUMP, 400, 4);
    step();
    CHECK(!s->dead && a->health == kk::f6::kFloorKk && s->st.js_floored == 1);
    step(9);
    inject_cut(a, P::CUT_TYPE_VERTICAL, 20);
    force_verdict(a, kk::V::Damage);
    step();
    CHECK(s->dead && a->mActionMode == 10 && count_log("cause=HP_ZERO") == 1);
  }
  // ---- P0 (D) a BROKEN Chilfos: the Jump Strike is no finisher (no instant kill, no decisive), the stagger goes on; a real finisher still kills
  {
    daE_KK_c* a = broken(id++);
    KkSlot* s = slot_of(a);
    CHECK(s->brk.active());
    a->health = 40;
    W.link.current.pos = {0, 0, -180};
    ++kk::f6::g_js_episode;
    inject_cut(a, P::CUT_TYPE_LARGE_JUMP, 400);
    g_hit.atp = 4;
    step();
    CHECK(!s->dead && a->mActionMode == kk::kYoroke && s->brk.active() && a->health == kk::f6::js_target(40, js, kk::f6::kFloorKk) && s->st.js_breaks_kept == 1);
    CHECK(count_log("event=NATIVE_DEATH") == 0 && count_log("SWORD_DECISIVE") == 0);
  }
  // ---- P0 (E) a Jump Strike the Chilfos blocks or parries: no HP at all for that contact
  for (kk::V v : {kk::V::Block, kk::V::EnemyParry}) {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, -180};
    step(3);
    const int hp = a->health;
    ++kk::f6::g_js_episode;
    inject_cut(a, P::CUT_TYPE_LARGE_JUMP, 400);
    g_hit.atp = 4;
    force_verdict(a, v);
    step();
    CHECK(!s->dead && a->health == hp && s->st.js_contacts == 0 && s->st.def_hp_restored == 0);
  }
  // ---- P4: a prepared parry of a Back Slice: orange parry (+25 Link posture), counter armed, NO rebound and Link's proc untouched; flank stays open
  {
    daE_KK_c* a = fight(id++);
    KkSlot* s = slot_of(a);
    W.link.current.pos = {0, 0, 200}, W.link.shape_angle.y = s16(0x8000), W.reach = 240;  // in its front, facing it (the jump matrix's stance)
    step(3);
    const int hp = a->health, rb0 = W.reverse_rebounds;
    inject_cut(a, kk::f6::kCutTwirl, 20);
    W.link.mProcID = 35;  // PROC_CUT_FINISH_JUMP_UP: no changeCutReverseProc in it
    s->def.prepared = true, s->def.prepared_serial = kz.swing_serial, s->def.prepared_until = W.tick + 12, s->shield_on = true;
    step();
    CHECK(!s->dead && a->health == hp && s->st.twirl_parries == 1 && s->st.enemy_parries == 1 && s->counter_pending && W.reverse_rebounds == rb0 && W.link.mProcID == 35);
    CHECK(count_log("event=PARRY_WITHOUT_LINK_RECOIL") == 1 && count_log("kind=ENEMY_PARRY points=25") == 1);
    // a Back Slice streak raises the floors (never 100) and the turn follows Link while he circles; the first Back Slice is never tracked
    kk::f6::Twirl tw;
    CHECK(tw.turn_step(100) == 0 && tw.floor(kk::f6::kTwirlParryPct, 100) == 0);
    tw.on(100, 1);
    CHECK(tw.turn_step(110) == kk::f6::kTwirlTurnStep[0] && tw.floor(kk::f6::kTwirlParryPct, 110) == 0);
    tw.on(150, 2), tw.on(200, 3), tw.on(250, 4), tw.on(260, 4);
    CHECK(tw.streak(260) == 4 && tw.floor(kk::f6::kTwirlParryPct, 260) == 70 && tw.floor(kk::f6::kTwirlBlockPct, 260) == 85 && tw.turn_step(260) > 0 && tw.streak(260 + kk::f6::kTwirlWindow + 300) == 0);
  }
  // ---- P3: counter -> chain: ~65% over thousands of completed counters, the four patterns ~1/4 each, gap 4..9, never twice for one counter, never after a cut counter
  {
    kk::Rng r;
    r.seed(0x6A55);
    int sched = 0, pat[4] = {}, twice = 0, cut_chained = 0;
    for (int i = 0; i < 4000; ++i) {
      kk::f6::Chain c;
      if (c.on_counter_end(true, std::uint64_t(i + 1), 1000, r)) {
        ++sched, ++pat[c.pattern];
        CHECK(c.at >= 1000 + kk::f6::kChainGapMin && c.at <= 1000 + kk::f6::kChainGapMax && c.due(c.at) && !c.due(c.at - 1));
      }
      twice += c.on_counter_end(true, std::uint64_t(i + 1), 1001, r) ? 1 : 0;
      kk::f6::Chain d;
      cut_chained += d.on_counter_end(false, std::uint64_t(i + 1), 1000, r) ? 1 : 0;
    }
    std::printf("KK_FIX06_CHAIN completed_counters=4000 chained=%d (%.1f%%) patterns=%d/%d/%d/%d twice=%d cut_counters_chained=%d\n", sched, sched * 100.0 / 4000, pat[0], pat[1], pat[2], pat[3], twice, cut_chained);
    CHECK(sched > 4000 * 60 / 100 && sched < 4000 * 70 / 100 && twice == 0 && cut_chained == 0);
    for (int k = 0; k < 4; ++k) CHECK(pat[k] > sched * 21 / 100 && pat[k] < sched * 29 / 100);
  }
  // ---- P6: the spear spark at point blank is drawn on Link's guard face, never inside him; a contact already in front keeps its native point
  {
    const kk::f6::V3 link{0, 0, 0}, enemy{0, 0, 100};
    const kk::f6::Spark in = kk::f6::spark_point(link, enemy, {0, 90, 5}), front = kk::f6::spark_point(link, enemy, {3, 90, 60}), low = kk::f6::spark_point(link, enemy, {0, 10, -20});
    CHECK(in.moved && in.p.z > 27.9f && in.p.z < 28.1f && in.p.y == 90.0f && !front.moved && front.p.z == 60.0f);
    CHECK(low.moved && low.p.y == kk::f6::kShieldYMin && low.p.z > 27.9f);
  }
  std::printf("KK_FIX06_LAB js_damage=%d of_ordinary=%d floor=%d chilfos_gain=%d\n", js, ord, kk::f6::kFloorKk, kk::f6::kGainJsKk);
}
static void clear_all() { clear_regs<KkExec, KkDelete, KkDamage, KkAttack, KkSetBck, KkSetWeaponBck, KkBack, KkThrow, KkDmgExec, KkNext, LinkCutType, LinkCutReverse, LinkDamageFix01, KkCutCheck, LinkCutAction, MeterDrawPost07, PfxProcHitmark>(); }
static void lab_install_all_or_nothing() {
  // a failing hook (any of the 21) leaves the module off: every callback returns at once
  for (int fail = 0; fail < 21; ++fail) {
    clear_all();
    kz.enabled = false;
    g_fail_install = g_install_count + fail;
    kk_install();
    CHECK(!kz.enabled);
  }
  g_fail_install = -1;
  clear_all();
  kk_install();
  CHECK(kz.enabled && Reg<KkExec>::pre.size() == 1 && Reg<KkDamage>::post.size() == 1 && Reg<PfxProcHitmark>::pre.size() == 1);
}
int main() {
  kk_install();
  CHECK(kz.enabled);
#ifdef KK_METRICS_ONLY
  lab_fix04_spam();
  lab_fix05_jump_spam();
  reset_world();
  return 0;
#endif
  lab_install_all_or_nothing();
  lab_patterns_and_rhythm();
  lab_crossing();
  lab_c3_tail();
  lab_identity_hp();
  lab_link_side();
  lab_defense();
  lab_break();
  lab_poise();
  lab_fix02();
  lab_fix03();
  lab_fix04();
#ifndef KK_METRICS_ONLY
  lab_fix05();
  lab_fix06();
#endif
  lab_lifecycle();
  kk_summary();
  reset_world();  // every actor of the last scenario freed (LeakSanitizer-clean)
  std::printf("KK_WORLD_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_ADAPTER_OVER_TRANSCRIBED_E_KK_AND_SCRIPTED_LINK runtime=HUMAN_RUNTIME_PENDING\n", kk::kVersion, g_checks);
  return 0;
}
