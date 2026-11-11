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
  bool ChkAtType(std::uint32_t m) const noexcept { return (at_type & m) != 0; }
  fopAc_ac_c* GetAc() noexcept { return owner; }
};
struct dCcD_GObjInf : cCcD_Obj {
  bool at_shield_hit = false, tg_shield = false;
  bool ChkAtShieldHit() const noexcept { return at_shield_hit; }
  void OnTgShield() noexcept { tg_shield = true; }
  void OffTgShield() noexcept { tg_shield = false; }
};
struct dCcD_Sph : dCcD_GObjInf {};
struct dCcD_Cyl : dCcD_GObjInf {};
struct J3DFrameCtrl { float mFrame = 0; };
struct mDoExt_McaMorfSO {
  J3DFrameCtrl mFrameCtrl;
  float rate = 1, end = 50;
  int attr = 0;
  float getFrame() const noexcept { return mFrameCtrl.mFrame; }
  float getPlaySpeed() const noexcept { return rate; }
  bool isStop() const noexcept { return attr != 2 && mFrameCtrl.mFrame >= end; }
  void play() noexcept {
    mFrameCtrl.mFrame += rate;
    if (attr == 2) {
      if (mFrameCtrl.mFrame >= end) mFrameCtrl.mFrame -= end;
    } else if (mFrameCtrl.mFrame > end) {
      mFrameCtrl.mFrame = end;
    }
  }
};
enum { fopAc_attn_BATTLE_e = 0 };
inline constexpr int fpcNm_E_KK_e = 0x1F0, fpcNm_ALINK_e = 0x2;
struct AttnInfo { std::uint8_t distances[4] = {0x4C, 0, 0, 0}; };
struct fopAc_ac_c {
  int name = 0;
  std::uint32_t pid = 0;
  s16 health = 0, field_0x560 = 0;
  cXyz eyePos{};
  struct { cXyz pos; } current{};
  csXyz shape_angle{};
  AttnInfo attention_info{};
  bool headlock = false;
  void offHeadLockFlg() noexcept { headlock = false; }
  void onHeadLockFlg() noexcept { headlock = true; }
  bool checkHeadLockFlg() const noexcept { return headlock; }
};
inline std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) noexcept { return a->pid; }
inline int fopAcM_GetName(const fopAc_ac_c* a) noexcept { return a->name; }
inline s16 cM_atan2s(float x, float z) noexcept { return s16(std::lround(std::atan2(x, z) * 32768.0 / 3.14159265358979)); }
struct daPy_py_c {
  enum { CUT_TYPE_NONE = 0, CUT_TYPE_JUMP = 0x0a, CUT_TYPE_HEAD_JUMP = 0x15, CUT_TYPE_GUARD_ATTACK = 0x29, CUT_TYPE_VERTICAL = 1 };
  enum { FLG1_IS_WOLF = 1 };
};
struct daAlink_c : fopAc_ac_c {
  enum { PROC_CUT_NORMAL = 0x20, PROC_CUT_FINISH = 0x21, PROC_CUT_TURN = 0x26, PROC_WAIT = 3 };
  int mProcID = PROC_WAIT, cut = 0;
  bool wolf = false;
  dCcD_Cyl mTgCyls[3];
  dCcD_Sph mAtCps[3];
  int getCutType() const noexcept { return cut; }
  bool checkNoResetFlg1(int) const noexcept { return wolf; }
};
// ---------------------------------------------------------------- the native Chilfos (transcribed)
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
  float speedF = 0;
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
};
struct KkExec {}; struct KkDelete {}; struct KkDamage {}; struct KkAttack {}; struct KkSetBck {}; struct KkSetWeaponBck {}; struct KkBack {}; struct KkThrow {}; struct KkDmgExec {};
struct KkNext {}; struct LinkCutType {}; struct LinkCutReverse {}; struct LinkDamageFix01 {};
// ---------------------------------------------------------------- world
struct World {
  std::uint64_t tick = 1;
  daAlink_c link;
  std::vector<daE_KK_c*> kk;
  bool safe = true, link_first = true;
  // Link's scripted state
  bool guarding = false, guard_attack = false, ga_in_window = false, link_damage_wait = false;
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
  int reverse_rebounds = 0, reverse_suppressed = 0;
  // models
  bool tip_swapped = false;
  float reach = 240;
  std::uint64_t behind_at = 0;  // Link reaches the Chilfos' back right before this tick's collision pass
  std::vector<std::uint64_t> native_opens;  // ticks in which the native executeAttack itself set field_0x67e = 0
  std::vector<int> native_open_bck;
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
void kkb_pfx_settle(bool link_on_kk, std::uint32_t, bool parry) noexcept { ++W.settles[link_on_kk ? 0 : 1][parry ? 1 : 0]; }
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
  morf.mFrameCtrl.mFrame = 0, morf.rate = rate, morf.attr = attr, morf.end = clip_end(i);
  field_0x764 = i;
  run_post<KkSetBck>(a);
}
void daE_KK_c::setWeaponBck(int i, u8 attr, f32 morfv, f32 rate) {
  daE_KK_c* me = this;
  void* a[] = {&me, &i, &attr, &morfv, &rate};
  if (!run_pre<KkSetWeaponBck>(a)) return;
  wmorf.mFrameCtrl.mFrame = 0, wmorf.rate = rate, wmorf.attr = attr, wmorf.end = i == 0x1E ? 45 : 40;
  field_0x768 = i;
  run_post<KkSetWeaponBck>(a);
}
template <class Tag, class F> void hooked(daE_KK_c* k, F body) {
  daE_KK_c* me = k;
  void* a[] = {&me};
  if (run_pre<Tag>(a)) body();
  run_post<Tag>(a);
}
static bool cut_check(int p) {  // mCutTypeCheck: only the cut types this lab scripts
  const int c = W.link.cut;
  if (p == 2) return c == daPy_py_c::CUT_TYPE_HEAD_JUMP;
  return false;
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
    if (field_0x67d == 0 && dist_link(this) <= 900) mCyl.OffTgShield();  // mCutTypeCheck(0): no 4th cut / spin in this lab
    if (mCyl.ChkTgHit()) {
      cCcD_Obj* o = mCyl.GetTgHitObj();
      mDamageTimer = 8;
      if (o->ChkAtType(kAtArrow)) {
        setActionMode(5, 0);
      } else if (o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
        onHeadLockFlg();
        setActionMode(5, field_0x67d == 0 ? 1 : 0);
      } else if (o->ChkAtType(kAtIron) || cut_check(2)) {
        health = 0;
        setActionMode(10, 0);
      } else {
        ++ice;
        if (field_0x67d == 0) {
          if (o->strong) setActionMode(7, 0);
          else ++guard_se7, setActionMode(6, 0);
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
          W.native_opens.push_back(W.tick), W.native_open_bck.push_back(field_0x764);  // every native execution of the frame-24 branch
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
      case 2:
        if (W.safe && dist_link(this) <= 900 && int(morf.getFrame()) < 0x17 && eyePos.y + 50 > W.link.current.pos.y) return setActionMode(8, 0);
        if (int(morf.getFrame()) == 0x17) ++W.spawns, field_0x67d = 1;
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
}
void daE_KK_c::executeDamage() {
  hooked<KkDmgExec>(this, [&] {
    switch (mMoveMode) {
      case 0: setBck(8, 0, 3, 1), field_0x67d = 1, mMoveMode = 2; return;
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
  speedF *= 0.7f;
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
static bool sword_active() { return W.swing_len > 0 && W.tick >= W.swing_start + 2 && W.tick < W.swing_start + 2 + W.swing_len; }
static void link_exec() {
  if (W.reverse_pending) {  // checkCutReverseAt for the shield contact of the last pass (only on procs with a recoil clip)
    W.reverse_pending = false;
    daAlink_c* k = &W.link;
    dCcD_GObjInf* o = &W.link.mAtCps[0];
    BOOL r = TRUE;
    void* a[] = {&k, &o};
    run_post<LinkCutReverse>(a, &r);
    if (r) ++W.reverse_rebounds, W.link.mProcID = 0x25;
    else ++W.reverse_suppressed;
  }
}
static void pass() {
  for (auto* a : W.kk) {
    if (a->deleted || a->mActionMode == 10) continue;
    a->mSph.ClrAtHit();
    for (auto& s : a->mSpheres) s.ClrAtHit();
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
        if (W.link.mProcID == daAlink_c::PROC_CUT_NORMAL || W.link.mProcID == daAlink_c::PROC_CUT_FINISH) W.reverse_pending = true;
      }
      W.swing_len = 0;  // one contact per swing (Link's own no-continuous-hit)
    }
    if (a->field_0x67d == 0 && a->field_0x67e == 0 && a->mActionMode == 8 && d <= W.reach) {
      a->mSph.at_hit = &W.link.mTgCyls[0];
      a->mSph.at_shield_hit = W.guarding;
      if (!W.guarding) ++W.link_hits;
    }
  }
  W.shield_attack_hit = false;
}
static void step(int n = 1) {
  for (int i = 0; i < n; ++i) {
    if (W.link_first) link_exec();
    for (auto* a : W.kk)
      if (!a->deleted) a->execute();
    if (!W.link_first) link_exec();
    if (W.behind_at && W.tick == W.behind_at) W.link.current.pos = {0, 0, -180};
    pass();
    ++W.tick;  // kk_maintenance runs inside the module's own Execute PRE, once per tick, as in production
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
  W.swing_len = 0, W.safe = true, W.link_first = true, W.tip_swapped = false, W.sword_strong = false, W.sword_type = kAtSword;
  W.link = daAlink_c{};
  W.link.name = fpcNm_ALINK_e, W.link.pid = 1, W.link.current.pos = {0, 0, 180}, W.link.shape_angle.y = s16(0x8000);
  W.reverse_pending = false, W.reverse_rebounds = W.reverse_suppressed = 0;
  W.direct_pending = false;
  g_link_posture = 0;
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
      W.native_opens.clear(), W.native_open_bck.clear();
      const bool measured = kz.ident.verified;  // the mapping this episode used from its first action
      std::uint64_t at_ticks = 0;
      const std::uint64_t t_first = W.tick;
      for (int t = 0; t < 600 && (s->ep.active || W.tick < t_first + 5); ++t) {
        step();
        if (a->field_0x67e == 0 && a->mActionMode == 8) ++at_ticks;  // registered by cc_set this tick
        if (s->ep.active && s->ep.ph == KkEp::Melee && a->mActionMode == 8) CHECK(a->morf.rate > 1.0f);  // never a native clip inside an episode
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
  CHECK(a->morf.rate == kk::policy::kRateThrow || a->field_0x764 == 0x12);
  until(a, 120, [](daE_KK_c* x) { return x->field_0x764 == 0x12; });
  CHECK(a->morf.rate == kk::policy::kRateRestore && a->wmorf.rate == kk::policy::kRateRestore);  // the fast regeneration, body and spear
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
  CHECK(spear->health == 140 && !slot_of(spear)->admitted && slot_of(spear)->excl == kk::Excl::Spear);
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
      }
      a->setActionMode(0, 0);
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
    a->shape_angle.y = 0;
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
  CHECK(a->morf.attr == 2);                                        // the stagger clip holds
  const int hp = a->health;
  CHECK(hp > 100);
  link_swing();
  step(6);
  CHECK(a->mActionMode == 10 && s->dead && s->st.decisive_sword == 1 && s->st.fallback_deaths == 0);
  step(80);
  CHECK(a->deleted && count_log("event=NATIVE_DEATH") == 1);
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
  // a Shield Attack outside the stagger: native DRAWBACK, but no Helm Splitter opening
  a = fight(1600);
  s = slot_of(a);
  W.shield_attack_hit = true;
  step(3);
  CHECK(a->mActionMode == 5 && !a->checkHeadLockFlg() && s->st.headlock_stripped == 1);
  // normal death at 0 HP without a break
  a = fight(1700);
  s = slot_of(a);
  a->health = 3, a->field_0x67d = 1;  // spear-less: native damage
  link_swing();
  step(6);
  CHECK(a->mActionMode == 10 && s->st.decisive_sword == 0 && s->brk.st == kk::Bk::None);
}
// ---------------------------------------------------------------- J/L: lifecycle, instances, events, budget
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
  for (unsigned i = 0; i < 12; ++i) total += kKkCaps[i];
  W.log.clear();
  step(3000);
  CHECK(W.log.size() <= total);
}
template <class... T> void clear_regs() { ((Reg<T>::pre.clear(), Reg<T>::post.clear()), ...); }
static void clear_all() { clear_regs<KkExec, KkDelete, KkDamage, KkAttack, KkSetBck, KkSetWeaponBck, KkBack, KkThrow, KkDmgExec, KkNext, LinkCutType, LinkCutReverse, LinkDamageFix01>(); }
static void lab_install_all_or_nothing() {
  // a failing hook (any of the 17) leaves the module off: every callback returns at once
  for (int fail = 0; fail < 17; ++fail) {
    clear_all();
    kz.enabled = false;
    g_fail_install = g_install_count + fail;
    kk_install();
    CHECK(!kz.enabled);
  }
  g_fail_install = -1;
  clear_all();
  kk_install();
  CHECK(kz.enabled && Reg<KkExec>::pre.size() == 1 && Reg<KkDamage>::post.size() == 1);
}
int main() {
  kk_install();
  CHECK(kz.enabled);
  lab_install_all_or_nothing();
  lab_patterns_and_rhythm();
  lab_crossing();
  lab_c3_tail();
  lab_identity_hp();
  lab_link_side();
  lab_defense();
  lab_break();
  lab_lifecycle();
  kk_summary();
  std::printf("KK_WORLD_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_ADAPTER_OVER_TRANSCRIBED_E_KK_AND_SCRIPTED_LINK runtime=HUMAN_RUNTIME_PENDING\n", kk::kVersion, g_checks);
  return 0;
}
