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
  struct { cXyz pos; csXyz angle; } current{};
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
  // the SDK numbering (include/d/actor/d_a_player.h); FIX02 fixed HEAD_JUMP (0x05: 0x15 is FINISH_RIGHT, a chain finisher the 0.13.1 log shows)
  enum { CUT_TYPE_NONE = 0, CUT_TYPE_VERTICAL = 1, CUT_TYPE_HEAD_JUMP = 0x05, CUT_TYPE_FINISH_LEFT = 0x06, CUT_TYPE_FINISH_VERTICAL = 0x07, CUT_TYPE_JUMP = 0x0a,
         CUT_TYPE_FINISH_RIGHT = 0x15, CUT_TYPE_FINISH_STAB = 0x20, CUT_TYPE_COMBO_STAB = 0x21, CUT_TYPE_GUARD_ATTACK = 0x29 };
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
  int reverse_rebounds = 0, reverse_suppressed = 0;
  // models
  bool tip_swapped = false;
  float reach = 240;
  std::uint64_t behind_at = 0;  // Link reaches the Chilfos' back right before this tick's collision pass
  std::vector<std::uint64_t> native_opens;  // ticks in which the native executeAttack itself set field_0x67e = 0
  std::vector<int> native_open_bck;
  std::vector<float> native_open_rate;  // FIX02: the clip rate at every native frame-24 opening
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
      current.angle.y = shape_angle.y;
      mDamageTimer = 8;
      if (o->ChkAtType(kAtArrow)) {
        setActionMode(5, 0);
      } else if (o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
        onHeadLockFlg();
        if (field_0x67d == 0) mCreatureSound.startCollisionSE(Z2SE_HIT_SWORD, 0x28);
        setActionMode(5, field_0x67d == 0 ? 1 : 0);
      } else if ((o->ChkAtType(kAtIron) && !W.status_400) || cut_check(2)) {
        health = 0;
        setActionMode(10, 0);
      } else {
        ++ice;
        if (field_0x67d == 0) {
          if (o->strong || W.status_400) setActionMode(7, 0);
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
          W.native_opens.push_back(W.tick), W.native_open_bck.push_back(field_0x764), W.native_open_rate.push_back(morf.rate);  // every native execution of the frame-24 branch
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
  W.status_400 = false;
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
      W.native_opens.clear(), W.native_open_bck.clear(), W.native_open_rate.clear();
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
  reset_world();
  W.link.current.pos = {0, 0, 400}, W.reach = 0;
  a = spawn(301);
  until(a, 60, [](daE_KK_c* x) { return slot_of(x) && slot_of(x)->ep.active; });
  s = slot_of(a);
  s->ep.counter = true;
  a->mMoveMode = 3, a->field_0x764 = 6;
  s->clip.tracked = true, s->clip.last_read = 22.0f, s->clip.fired = 0;
  a->morf.rate = 1.5f, a->morf.mFrameCtrl.mFrame = 24.0f;
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
    CHECK(a->mActionMode == act && a->mMoveMode == mode && a->current.angle.y == head && count_log("CHILFOS_POISE event=RESUMED") == 1);
    step(40);
    CHECK(!reacted(a, from, false) && a->disp >= d0 && !s->brk.active());
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
    int swings = 0;
    for (int i = 0; i < n; ++i) {
      W.sword_strong = (i % 3) != 0;
      link_swing(cuts[i % 9]);
      for (int k = 0; k < 13; ++k) {
        step();
        CHECK(a->field_0x67d == 0 && a->mActionMode != 7 && a->mActionMode != 4);
      }
      ++swings;
      if (a->health <= 60) a->health = 300;  // a scripted heal keeps the stress going (this lab measures the spear, not the death)
      if (s->brk.active() || s->brk.st == kk::Bk::Pending) s->brk.reset(), s->p.reset(), a->setActionMode(0, 0);
      s->p.u = 0;
    }
    W.sword_strong = false;
    CHECK(swings == n && s->st.blocked_spear_loss == 0 && s->st.other_spear_loss == 0 && s->st.spear_breaks == 0 && count_log("source=SPEAR_BREAK") == 0);
    CHECK(s->st.blocks + s->st.enemy_parries > unsigned(n) / 3 && s->st.spear_break_prevented > 0);
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
      link_swing(2);
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
    for (int i = 0; i < 200 && confirmed < 6; ++i) {
      if (s->ep.active) {
        step();
        continue;
      }
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
  for (unsigned i = 0; i < 14; ++i) total += kKkCaps[i];
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
  lab_poise();
  lab_fix02();
  lab_lifecycle();
  kk_summary();
  std::printf("KK_WORLD_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_ADAPTER_OVER_TRANSCRIBED_E_KK_AND_SCRIPTED_LINK runtime=HUMAN_RUNTIME_PENDING\n", kk::kVersion, g_checks);
  return 0;
}
