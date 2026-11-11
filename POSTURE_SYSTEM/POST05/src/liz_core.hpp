#pragma once
// LIZALFOS DUELIST 0.12.2 (FIX02 over 0.12.1): complete per-instance combat brain for the ordinary Lizalfos (native E_DN, e_dn_class).
// PURE logic: standard library + post08 value types only; no game headers, no addresses, no native RNG, no CRT math.
// liz_adapter.inc copies native fields into a View, calls one callback per native hook and applies the returned Write.
// tests/liz_lab.cpp drives the SAME callbacks in the same native order against a transcription of d_a_e_dn.cpp.
//
// Native facts [S src/d/actor/d_a_e_dn.cpp @40457c6, B dusklight.exe 2.0.3 symbol manifest + disassembly]:
//  * ACTION_ATTACK_0(4) ANM_ATTACK_TAIL_01(4): AT pre-play frames 22..28, tail flag 0, sound frame 18, shield contact only vibrates.
//  * ACTION_ATTACK(5) ANM_ATTACK_TAIL_02(5): AT 13..18 flag 0 and 25..30 flag 1 (+25 per tick), sounds 10/24; at_hit_check()==Link && guard
//    => setPlaySpeed(0); action=FIGHT_RUN; mode=0 (the ONLY shield cancellation of E_DN).
//  * ACTION_TAIL_ATTACK(6) ANM_ATTACK_01(8): AT 33..41 flag 2 (+30 per tick), sounds 28/35 (post-play) 40/43 (pre-play); no at_hit_check.
//  * `int frame` is read at the top of each attack function BEFORE anm_init of mode 0 (stale previous clip frame) and before play().
//  * damage_check: small_damage / big_damage (big then writes invulnerability_timer=1000); health<=0 or mHitStatus => big; mHitType 16 => DRAWBACK.
//  * e_dn_damage mode 2 sets onDownFlg (native Ending Blow route), mode 3 waits timer[0], 20 get-up, 21 => ACTION_NORMAL.
//  * tate_sph is a Tg SHIELD sphere (SPrm 3): AtShieldHit on Link's sword => native changeCutReverseProc; Execute sets unk_timer_2=10 on acceptance.
//  * ACTION_DRAWBACK(12): the native standing stagger of E_DN (Shield Attack, mHitType 16); its mode 1 raises fopEn HeadLock => Helm Splitter. Clip end => FIGHT_RUN mode 0.
//  * Link (d_a_alink_damage.inc:629): `var_r29->ChkTgHit() && mProcID != PROC_GUARD_SLIP`: a contact during the ~12 tick GUARD_SLIP (d_a_alink_guard.inc: clip 11 frames, 14.0 speed -1.25/tick)
//    gets NO setGuardSe, NO vibration, NO reaction, NO Shield Attack. Vanilla E_DN never reaches it: its only follow-up (D2) is cancelled by e_dn_attack. 0.12.0 removed that cancel => D2 is mute at 11 ticks.
//  * dCcS::ChkShieldFrontRange (d_cc_s.cpp:129): the block test uses the AT sphere's VELOCITY (MoveCAt) when non zero, the actor->Link direction when zero (StartCAt, first armed tick).
//  * damage_check: learn |= mHitBit; e_dn_fight_run guards whenever (learn & cc_pl_cut_bit_get()) != 0: vanilla raises the shield always after the first hit of each cut type. The sword bits are this module's decision now.
//  * e_dn_guard refreshes invulnerability_timer=5 in modes 1/6/7 and the entry writes 15; Execute moves the cc_sph body spheres away while it is non zero: native guard = a body nobody can touch, from every side, before the shield is up. Mode 8 (lowering) has no refresh: body live with the shield still up.
//  * tate_sph hit => unk_timer_2=10 => the shield sphere leaves for 10 ticks whatever swing comes next.
//  * Link: changeCutReverseProc -> checkCutReverseAt(mAtCps[i]) = ChkAtHit && ChkAtShieldHit: any tate_sph contact rebounds his sword; it reads the record in Link's Execute, which runs before this actor's.
#include "cp06f1_core.hpp"
#include <cstdint>
#include "parry_core.hpp"

namespace liz {
inline constexpr const char* kVersion = "0.12.4";
inline constexpr unsigned kSlots = 16;

namespace native {
inline constexpr std::int16_t kNormal = 0, kFightRun = 3, kAttack0 = 4, kAttack = 5, kTailAttack = 6, kGuard = 7, kDrawback = 12, kSDamage = 20, kDamage = 21, kReg = 60;
inline constexpr int kAnmN = 4, kAnmD = 5, kAnmC = 8;
inline constexpr float kGuardRange = 300.0f;  // l_HIO.battle_init_range
inline constexpr float kAttackInitRange = 350.0f;  // l_HIO.attack_init_range
inline constexpr std::int16_t kNativeHp = 100;
inline constexpr int kJntHandR1 = 0x13, kJntTail04 = 0x22;  // d_a_e_dn.cpp joint enum
inline constexpr float kSwordY = 60.0f, kModelSize = 1.1f;   // Execute: at_sph on JNT_HANDR1*(0,60,0), r=size*50 (D2 size*80); tail: JNT_TAIL04 origin, r=size*70
inline constexpr float kLinkSlipSpeed = 14.0f;               // daAlinkHIO_guard_c0::m.mGuardSpeedNormal
inline constexpr std::int16_t kLinkProcGuardSlip = 0x1d;
}  // namespace native

namespace policy {
inline constexpr std::int16_t kMaxHp = 370;
inline constexpr int kMilli = 1000, kMaxPosture = 100, kMaxMilli = kMaxPosture * kMilli;
inline constexpr int kGainSword = 6, kGainLinkParry = 12, kGainIronBall = 30, kGainBlocked = 5, kGainLinkOnEnemyParry = 25;
inline constexpr std::uint32_t kRegenDelayTicks = 60;
inline constexpr int kTicksPerSecond = 30;
// Link's OWN posture when he blocks / takes a Lizalfos hit: the existing LNK07 table rows by native AtAtp (N/D ATP1 -> Blue row, tail ATP2 -> Red row).
inline constexpr int kLinkBlockAtp1 = 20, kLinkDirectAtp1 = 30, kLinkBlockAtp2 = 25, kLinkDirectAtp2 = 35;
// Adaptive defense
inline constexpr int kParryPctA = 12, kParryPctB = 35, kParryPctC = 45;
inline constexpr std::uint32_t kPressureWindow = 75;       // three distinct swings in ~2.5 s
inline constexpr std::uint32_t kSustainedWindow = 150;     // four distinct swings in 5 s (level C)
inline constexpr unsigned kSustainedSwings = 4;
inline constexpr std::uint32_t kDemoteTicks = 45;
inline constexpr float kResetDistance = 600.0f;
inline constexpr std::uint32_t kResetFarTicks = 30;
inline constexpr std::uint32_t kPendingSwingTicks = 6;     // level A and combo boundaries: a swing still counts for this long, then it is gone
inline constexpr std::uint32_t kPressureLeaseTicks = 30;   // level B/C (proven pressure): the same swing can still earn its ONE roll when a committed state ends, then it is gone
// Measured on the native order: entry in FIGHT_RUN POST (tick T), e_dn_guard places tate_sph at T+1, its collision pass is accepted at T+2 => delta 3 is the first physically possible contact.
inline constexpr std::uint32_t kParryWindowFirst = 3, kParryWindowLast = 7;  // 5 simulation ticks from the first real tate_sph pass; real contact only
inline constexpr std::uint32_t kParryCooldownA = 120, kParryCooldownB = 90, kParryCooldownC = 60;  // by pressure level at the moment of the parry
inline constexpr std::int16_t kFrontHalfAngle = 0x3000;    // parry only inside +-67.5 deg of the actor's facing
inline constexpr std::int16_t kWayHalfAngle = 0x4000;      // native way_check front half-plane
inline constexpr std::int16_t kSwingAimHalfAngle = 0x2000; // Link's swing counts for THIS actor only when aimed at it (+-45 deg)
inline constexpr float kSwingRange = 320.0f;
// Progressive guard after real sword damage: one roll per distinct wounding swing, never retroactive, taken at the next safe transition.
inline constexpr int kGuardPctHit1 = 45, kGuardPctHit2 = 80;  // a third wound without having guarded: certain preparation, still not a success
inline constexpr std::uint32_t kDamageStreakTicks = 150, kGuardIntentLeaseTicks = 150;
inline constexpr std::uint32_t kGuardReactTicks = 6;  // a reaction, not a reflex: never the same tick as the wound
inline constexpr std::uint32_t kGuardHoldTicks = 30, kGuardHoldRefreshTicks = 20;  // guard held while Link keeps swinging, bounded by kGuardWatchdog
inline constexpr float kGuardReadyFrame = 3.0f, kGuardLowerFrames = 5.0f;  // ANM_GUARD: shield presented / still presented while it lowers (mode 8)
inline constexpr std::uint32_t kHitLockTicks = 10;  // the invulnerability of a real wound is never shortened by the guard's body rule
// FIX04 P0: orientation is judged by the BODY's facing, presentation by the guard clip, collision by the engine's own tate_sph contact.
// 0.12.3 demanded that the arm's anchor stood 10 units toward Link: the HELD pose (mode 7) does not (log 204237: 25 of 40 sword contacts in mode 7 fell on
// the body, 1 was a block), so the shield "covered" nothing exactly while it was up. The arc is asymmetric because the shield is on the left arm.
inline constexpr std::int16_t kShieldSideLimit = 0x5000;     // Link up to +112.5 deg toward the actor's left (the shield arm)
inline constexpr std::int16_t kOpenSideLimit = 0x3000;       // Link up to -67.5 deg toward the actor's right (the open side); beyond either limit: never covered
inline constexpr std::int16_t kGuardTurnScale = 2, kGuardTurnMax = 0x800;  // native e_dn_fight_run law: cLib_addCalcAngleS2(cur, search, 2, 0x800)
inline constexpr int kLearnSwordMask = 0x0FFF;     // cc_pl_cut_bit_get's bits in e_dn_class::learn. Hookshot 0x1000, bomb, boomerang and arrow bits stay native.
// Two ordinary blocks: one roll that PREPARES A NEW PARRY WINDOW against a continuation. It never authorizes a counter.
inline constexpr std::uint32_t kBlockChainTicks = 90;      // two distinct contacts further apart are not the same exchange
inline constexpr int kDeflectPct = 80;
inline constexpr std::uint32_t kDeflectLeaseTicks = 45;
// Short counter: only after a real enemy parry. Kind is exactly 50/50.
inline constexpr std::uint32_t kCounterLeaseTicks = 120;   // the intent survives the native guard exit, bounded
inline constexpr std::int16_t kCounterFaceHalf = 0x3000;   // no magic turn from the back: starts only inside +-67.5 deg
inline constexpr float kCounterExpireDistance = 600.0f;
inline constexpr std::int16_t kCounterHeightHalf = 0x1000; // native fight_run's search_angle_x band: Link on another level is not an opportunity
// Standing break (semantics of the Bokoblin Breaker, native states of E_DN)
inline constexpr std::uint32_t kPunishTicks = 90;          // absolute deadline of the standing vulnerability, as post06::policy::kPunishMaxExec
inline constexpr std::uint32_t kPunishHoldTimer0 = 2;      // native S_DAMAGE timer[0] kept above zero while the deadline has not passed
// Combo pacing
inline constexpr std::uint32_t kMaxGapTicks = 60, kActionWatchdog = 150, kGuardWatchdog = 180, kBreakWatchdog = 900;
inline constexpr int kRestMin = 45;
inline constexpr std::uint32_t kRestSpan = 31;  // 45..75 ticks of the native attack timer after an episode
inline constexpr float kMinWindupFrames = 8.0f;
inline constexpr std::int16_t kTurnStep = 0x800;
inline constexpr float kSlideStep = 12.0f;
inline constexpr float kStandoffN = 160.0f, kStandoffD = 190.0f, kStandoffC = 190.0f;
inline constexpr float kLungeD2 = 25.0f, kLungeC = 30.0f;  // native BREG_F(17)+25 / BREG_F(18)+30 per tick inside the window
// Sampling diagnostics: Link Tg cylinders are never thinner than this in practice (radius used only to flag a possible sampling hole).
inline constexpr float kThinnestLinkRadius = 30.0f;
// Link's fast re-arm between two impacts of one episode (D1->D2 contacts 6..11 ticks apart; CP04's 12 tick slip rule cannot fit a second press there). Two real presses, two native clips, always.
inline constexpr std::uint64_t kDoubleRearmTicks = 4;
}  // namespace policy

// ---------------------------------------------------------------- identity
enum class Excl : std::uint8_t { None = 0, WrongActor, Dynalfos, SkullModel, ScriptedRun, Status, SpecialRoom, Count };
inline const char* name(Excl e) noexcept {
  static constexpr const char* n[] = {"NONE", "WRONG_ACTOR", "DYNALFOS_ARG1", "SKULL_MODEL", "SCRIPTED_REG", "STATUS", "SPECIAL_ROOM_D_MN07_8"};
  return unsigned(e) < unsigned(Excl::Count) ? n[unsigned(e)] : "?";
}
struct IdIn {
  bool profile_ok = false, skull_model = false, special_room = false;
  int arg0 = 0, arg1 = 0, status = 0, action = 0;
};
// Positive classification; anything unidentified keeps the vanilla actor.
inline Excl classify(const IdIn& i) noexcept {
  if (!i.profile_ok) return Excl::WrongActor;
  if (i.arg1 == 1) return Excl::Dynalfos;           // useHeapInit loads the skull; soul "strong", GAKEJUMP and bomb kick branch on arg1==1
  if (i.skull_model) return Excl::SkullModel;
  if (i.arg0 >= 13 && i.arg0 <= 16) return Excl::ScriptedRun;  // create: status=1 + ACTION_REG scripted run
  if (i.action == native::kReg) return Excl::ScriptedRun;
  if (i.status != 0) return Excl::Status;
  if (i.special_room) return Excl::SpecialRoom;     // native comment: City in the Sky scripted room
  return Excl::None;
}

// ---------------------------------------------------------------- RNG (own stream: the game's cM_rnd sequence is never consumed)
class Rng {
 public:
  void seed(std::uint64_t v) noexcept {
    std::uint64_t z = v + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    s_ = (z ^ (z >> 31)) | 1ull;
  }
  std::uint32_t next() noexcept {
    s_ ^= s_ >> 12;
    s_ ^= s_ << 25;
    s_ ^= s_ >> 27;
    return static_cast<std::uint32_t>((s_ * 0x2545F4914F6CDD1Dull) >> 32);
  }
  // Unbiased: rejection of the top partial range.
  std::uint32_t below(std::uint32_t n) noexcept {
    const std::uint32_t lim = 0xFFFFFFFFu - (0xFFFFFFFFu % n + 1u) % n;
    for (int guard = 0; guard < 64; ++guard) {
      const std::uint32_t x = next();
      if (x <= lim) return x % n;
    }
    return next() % n;
  }

 private:
  std::uint64_t s_ = 1;
};

// ---------------------------------------------------------------- posture (0..100, independent of post04: never a Bokoblin variant)
// Exact HP bands, integer ratio over 370: >75% 10, (50,75] 7, (25,50] 4, (10,25] 2, <=10% 1 points per second.
inline int regen_points_per_second(int hp, int max_hp) noexcept {
  if (hp <= 0 || max_hp <= 0) return 0;
  const long long h = static_cast<long long>(hp) * 100;
  const long long m = max_hp;
  if (h > 75 * m) return 10;
  if (h > 50 * m) return 7;
  if (h > 25 * m) return 4;
  if (h > 10 * m) return 2;
  return 1;
}
struct PostureState {
  int milli = 0, acc = 0;
  std::uint32_t since = 0;
  bool latched = false;
  std::uint64_t gained = 0, regenerated = 0, breaks = 0, rearms = 0;
  int points() const noexcept { return milli / policy::kMilli; }
  // Returns true exactly once: the crossing that latches.
  bool add(int pts) noexcept {
    if (latched || pts <= 0) return false;
    const int before = milli;
    milli = before + pts * policy::kMilli > policy::kMaxMilli ? policy::kMaxMilli : before + pts * policy::kMilli;
    gained += static_cast<std::uint64_t>(milli - before);
    since = 0;
    acc = 0;
    if (milli >= policy::kMaxMilli) {
      latched = true;
      ++breaks;
      return true;
    }
    return false;
  }
  // One eligible simulation tick. 60 eligible ticks delay; integer accumulator at 30 ticks/s (no drift). NEVER HP.
  void regen(bool eligible, int hp, int max_hp) noexcept {
    if (!eligible || latched || milli <= 0) return;
    if (since < policy::kRegenDelayTicks) {
      ++since;
      return;
    }
    acc += regen_points_per_second(hp, max_hp) * policy::kMilli;
    const int d = acc / policy::kTicksPerSecond;
    acc -= d * policy::kTicksPerSecond;
    const int take = d > milli ? milli : d;
    milli -= take;
    regenerated += static_cast<std::uint64_t>(take);
    if (milli == 0) acc = 0;
  }
  void rearm() noexcept {
    milli = 0, acc = 0, since = 0, latched = false;
    ++rearms;
  }
};

// ---------------------------------------------------------------- combos
enum class Act : std::uint8_t { N = 0, D = 1, CSlow = 2, CFast = 3 };
inline constexpr float kRateN = 1.25f, kRateD = 1.10f, kRateCSlow = 1.10f, kRateCFast = 1.20f;
inline constexpr float rate_of(Act a) noexcept { return a == Act::N ? kRateN : a == Act::D ? kRateD : a == Act::CSlow ? kRateCSlow : kRateCFast; }
inline constexpr std::int16_t native_action(Act a) noexcept { return a == Act::N ? native::kAttack0 : a == Act::D ? native::kAttack : native::kTailAttack; }
inline constexpr int native_anm(Act a) noexcept { return a == Act::N ? native::kAnmN : a == Act::D ? native::kAnmD : native::kAnmC; }
inline constexpr bool is_tail(Act a) noexcept { return a == Act::CSlow || a == Act::CFast; }
inline constexpr unsigned impacts_of(Act a) noexcept { return a == Act::D ? 2u : 1u; }
inline constexpr float first_at(Act a) noexcept { return a == Act::N ? 22.0f : a == Act::D ? 13.0f : 33.0f; }
inline constexpr float last_open(Act a) noexcept { return a == Act::N ? 22.0f : a == Act::D ? 25.0f : 33.0f; }  // opening of the LAST window
inline constexpr float last_at(Act a) noexcept { return a == Act::N ? 28.0f : a == Act::D ? 30.0f : 41.0f; }
inline constexpr float standoff(Act a) noexcept { return a == Act::N ? policy::kStandoffN : a == Act::D ? policy::kStandoffD : policy::kStandoffC; }

inline constexpr unsigned kPatterns = 4, kCounterN = 4, kCounterD = 5, kPatternSlots = 6, kMaxActs = 6;
struct Pattern {
  unsigned acts, impacts;
  Act a[kMaxActs];
  float gap_s[kMaxActs];  // [i]: recorded seconds from the LAST impact of action i to the first impact of action i+1
};
// Composition, action rates and intervals fixed by the user's confirmed impact table. D's internal pair is not a schedule value: it is the clip.
inline constexpr Pattern kPattern[kPatternSlots] = {
    {3, 4, {Act::N, Act::D, Act::CFast}, {0.50f, 0.63f}},
    {6, 7, {Act::N, Act::N, Act::N, Act::D, Act::N, Act::CFast}, {0.58f, 0.60f, 0.54f, 0.66f, 0.67f}},
    {6, 9, {Act::D, Act::N, Act::D, Act::N, Act::N, Act::D}, {0.52f, 0.53f, 0.54f, 0.61f, 0.52f}},
    {5, 6, {Act::CSlow, Act::CSlow, Act::N, Act::D, Act::CFast}, {1.34f, 1.26f, 0.52f, 0.63f}},
    {1, 1, {Act::N}, {}},  // short counter: native ACTION_ATTACK_0 x1.25, NOT a fifth combo, never part of the 25% draw
    {1, 2, {Act::D}, {}},  // short counter: native ACTION_ATTACK x1.10 with both windows, NOT a sixth combo
};
// Recorded per-IMPACT intervals of the user's table (documentation + lab fixture).
inline constexpr unsigned kIntervals[kPatterns] = {3, 6, 8, 5};
inline constexpr float kRecorded[kPatterns][8] = {
    {0.50f, 0.13f, 0.63f},
    {0.58f, 0.60f, 0.54f, 0.10f, 0.66f, 0.67f},
    {0.12f, 0.52f, 0.53f, 0.10f, 0.54f, 0.61f, 0.52f, 0.11f},
    {1.34f, 1.26f, 0.52f, 0.11f, 0.63f},
};

// Pre-play int(frame) sequence start+k*rate reaches every frame the native sound table compares with ==, so no sound is lost at fractional rates.
inline constexpr bool visits_frame(float start, float rate, int want) noexcept {
  float f = start;
  for (int k = 0; k < 200; ++k) {
    const int i = static_cast<int>(f);
    if (i == want) return true;
    if (i > want) return false;
    f += rate;
  }
  return false;
}
inline constexpr bool sounds_ok(Act a, float start) noexcept {
  const float r = rate_of(a);
  const int si = static_cast<int>(start);
  if (a == Act::N) return si > 18 || visits_frame(start, r, 18);
  if (a == Act::D) return (si > 10 || visits_frame(start, r, 10)) && (si > 24 || visits_frame(start, r, 24));
  return (si > 28 || visits_frame(start, r, 28)) && (si > 35 || visits_frame(start, r, 35)) && visits_frame(start, r, 40) && visits_frame(start, r, 43);
}
// Smallest sub-frame phase that keeps every native sound of the clip at this rate. Deterministic; proved by the lab with accumulating floats.
inline constexpr float phase_for(Act a, float entry) noexcept {
  for (int k = 0; k < 20; ++k) {
    const float p = static_cast<float>(k) * 0.05f;
    if (sounds_ok(a, entry + p)) return p;
  }
  return 0.0f;
}
// Ticks from the anm_init tick to the tick whose pre-play int(frame) first reaches `at`.
inline constexpr std::uint32_t windup_ticks(float start, float rate, float at) noexcept {
  std::uint32_t k = 0;
  float f = start;
  while (f < at && k < 400) {
    f += rate;
    ++k;
  }
  return k;
}
inline constexpr std::uint32_t tail_ticks(Act a) noexcept {  // last window open -> first tick with int(frame) beyond the window
  std::uint32_t k = 0;
  float f = last_open(a);
  const float r = rate_of(a);
  while (f < last_at(a) + 1.0f && k < 100) {
    f += r;
    ++k;
  }
  return k;
}
inline constexpr int round_ticks(float s) noexcept { return static_cast<int>(s * policy::kTicksPerSecond + 0.5f); }
// Native entry frame of action `i` of pattern `p`: the wind-up starts inside the native clip so the recorded gap fits. The first action of an
// episode always plays its complete native wind-up; no AT window is ever skipped; at least kMinWindupFrames of real wind-up remain.
inline constexpr float entry_frame(unsigned p, unsigned i) noexcept {
  if (i == 0 || p >= kPatterns) return 0.0f;
  const Pattern& P = kPattern[p];
  const Act prev = P.a[i - 1], cur = P.a[i];
  // The boundary runs on the tick AFTER the window was seen closed, hence the -1. Rounded UP so the wind-up always fits and next_start can add the exact remainder.
  const float avail = static_cast<float>(round_ticks(P.gap_s[i - 1])) - static_cast<float>(tail_ticks(prev)) - 1.0f;
  float s = first_at(cur) - avail * rate_of(cur);
  const float mx = first_at(cur) - policy::kMinWindupFrames;
  if (s > mx) s = mx;
  if (s < 0.0f) s = 0.0f;
  const int fl = static_cast<int>(s);
  return static_cast<float>(static_cast<float>(fl) < s ? fl + 1 : fl);
}
inline constexpr float start_frame(unsigned p, unsigned i) noexcept {
  const float e = entry_frame(p, i);
  return e + phase_for(kPattern[p].a[i], e);
}
inline constexpr std::uint32_t windup_of(unsigned p, unsigned i) noexcept { return windup_ticks(start_frame(p, i), rate_of(kPattern[p].a[i]), first_at(kPattern[p].a[i])); }

enum class End : std::uint8_t { None = 0, Completed, Posture100, Death, NativePriority, AdaptiveDefense, SceneOrDelete, Event, Watchdog, Count };
inline const char* name(End e) noexcept {
  static constexpr const char* n[] = {"NONE", "COMPLETED", "POSTURE_100_BREAK", "DEATH", "NATIVE_PRIORITY", "ADAPTIVE_DEFENSE_AT_SAFE_BOUNDARY", "SCENE_OR_DELETE", "EVENT", "WATCHDOG"};
  return unsigned(e) < unsigned(End::Count) ? n[unsigned(e)] : "?";
}
struct Episode {
  bool active = false, anm_pending = false, window_on = false, act_done = false, any_window = false;
  std::uint8_t pattern = 0, act = 0, impact = 0, window = 0, voluntary = 0;
  std::uint64_t token = 0, start_tick = 0, act_tick = 0, last_open_tick = 0, next_start = 0, last_window_close = 0;
};
// Voluntary defense is only ever evaluated at a safe boundary (between actions, AT closed), and for long combos it is rare and positional.
inline bool boundary_allowed(unsigned pattern, unsigned next_act, int level, unsigned voluntary) noexcept {
  if (voluntary != 0 || level < 1 || pattern >= kPatterns) return false;
  switch (pattern) {
    case 0: return false;                        // P1: finish the three actions; defense between episodes
    case 1: return next_act >= 1 && next_act < kPattern[1].acts;  // P2: occasional, any action boundary under verified pressure
    case 2: return next_act >= 3 && next_act < kPattern[2].acts;  // P3: completeness first: never before its half
    case 3: return next_act == 1 || next_act == 2;              // P4: the deliberate opening pauses; pair and final tail protected
    default: return false;
  }
}

// ---------------------------------------------------------------- adaptive defense memory (one per actor)
class Defense {
 public:
  static constexpr unsigned kRing = 8;
  int level = 0;  // 0=A 12% 1=B 35% 2=C 45%. HP is deliberately NOT an input.
  std::uint64_t cooldown_until = 0;
  std::uint64_t rolls[3] = {}, attempts[3] = {}, promotions = 0, demotions = 0, resets = 0;
  void stamp(std::uint64_t t) noexcept {
    ring_[head_ % kRing] = t;
    ++head_;
  }
  unsigned swings_within(std::uint64_t now, std::uint64_t win) const noexcept {
    unsigned n = 0;
    const unsigned c = head_ < kRing ? head_ : kRing;
    for (unsigned i = 0; i < c; ++i) {
      const std::uint64_t t = ring_[(head_ - 1 - i) % kRing];
      if (now - t <= win) ++n;
    }
    return n;
  }
  bool cond_b(std::uint64_t now) const noexcept { return swings_within(now, policy::kPressureWindow) >= 3; }
  bool cond_c(std::uint64_t now) const noexcept { return swings_within(now, policy::kSustainedWindow) >= policy::kSustainedSwings; }
  // One per simulation tick: promotion is immediate, demotion one step after kDemoteTicks without its condition. Backing off clears the memory.
  void update(std::uint64_t now, float pl_dir) noexcept {
    if (pl_dir > policy::kResetDistance) {
      if (++far_ >= policy::kResetFarTicks) {
        clear();
        far_ = 0;
      }
    } else {
      far_ = 0;
    }
    const int want = cond_c(now) ? 2 : cond_b(now) ? 1 : 0;
    if (want > level) {
      level = want;
      since_ok_ = 0;
      ++promotions;
    } else if (want == level) {
      since_ok_ = 0;
    } else if (++since_ok_ >= policy::kDemoteTicks) {
      --level;
      since_ok_ = 0;
      ++demotions;
    }
  }
  static int percent(int level) noexcept { return level >= 2 ? policy::kParryPctC : level == 1 ? policy::kParryPctB : policy::kParryPctA; }
  bool roll(Rng& r) noexcept {
    const unsigned l = level < 0 ? 0u : level > 2 ? 2u : unsigned(level);
    ++rolls[l];
    const bool hit = r.below(100) < static_cast<std::uint32_t>(percent(level));
    if (hit) ++attempts[l];
    return hit;
  }
  void reset() noexcept {  // Link gone, scene, or the break: the memory of a fight that is over never survives it
    if (level > 0) ++demotions;
    level = 0;
    since_ok_ = 0;
    clear();
  }
  static std::uint32_t cooldown_of(int level) noexcept { return level >= 2 ? policy::kParryCooldownC : level == 1 ? policy::kParryCooldownB : policy::kParryCooldownA; }
  bool cooled(std::uint64_t now) const noexcept { return now >= cooldown_until; }
  void clear() noexcept {
    head_ = 0;
    for (auto& t : ring_) t = 0;
    ++resets;
  }

 private:
  std::uint64_t ring_[kRing] = {};
  unsigned head_ = 0;
  std::uint32_t since_ok_ = 0, far_ = 0;
};
// Parry window: pure predicate on a REAL accepted tate_sph contact.
inline bool parry_contact(bool attempt_armed, std::uint64_t guard_tick, std::uint64_t now, bool front, bool cooled) noexcept {
  if (!attempt_armed || !front || !cooled || now < guard_tick) return false;
  const std::uint64_t d = now - guard_tick;
  return d >= policy::kParryWindowFirst && d <= policy::kParryWindowLast;
}

// ---------------------------------------------------------------- Link contact on at_sph (one verdict per physical impact)
enum class Hit : std::uint8_t { None = 0, Stale, NotAttacking, Event, LinkInvalid, Parry, ParryLatched, Guard, MissedEarly, Direct, Duplicate, Count };
inline const char* name(Hit h) noexcept {
  static constexpr const char* n[] = {"NONE", "STALE", "NOT_ATTACKING", "EVENT", "LINK_INVALID", "PARRY", "PARRY_LATCHED", "GUARD", "MISSED_WINDOW_GUARD", "DIRECT", "DUPLICATE"};
  return unsigned(h) < unsigned(Hit::Count) ? n[unsigned(h)] : "?";
}
struct HitIn {
  bool at_hit = false, on_link = false, shield = false, armed_last = false, event = false, link_ok = false, latched = false;
  std::uint64_t impact = 0, resolved = 0;
  post08::LinkView pass;
};
inline Hit judge_hit(const HitIn& h) noexcept {
  if (!h.at_hit || !h.on_link) return Hit::None;
  if (!h.armed_last) return Hit::Stale;
  if (h.event) return Hit::Event;
  if (h.impact == 0) return Hit::NotAttacking;
  if (!h.link_ok) return Hit::LinkInvalid;
  if (h.resolved == h.impact) return Hit::Duplicate;  // one verdict per impact: a held R or a later press never earns a second result
  if (!h.shield) return Hit::Direct;
  if (!h.pass.valid || !h.pass.guard_attack) return Hit::Guard;
  if (!post08::Parry::in_window(h.pass)) return Hit::MissedEarly;
  return h.latched ? Hit::ParryLatched : Hit::Parry;
}

// ---------------------------------------------------------------- native view / writes
enum class HitKind : std::uint8_t { None = 0, Sword, IronBall, Other };
struct View {
  int learn = 0;
  std::int16_t action = 0, mode = 0, hp = 0, max_hp = 0, invuln = 0, timer2 = 0, ut2 = 0, search_y = 0, search_x = 0, shape_y = 0, cur_y = 0, fc = 0, seen_timer = 1;
  std::int8_t at_chk = 0, tail = 0;
  int anm_no = 0;
  float frame = 0, rate = 0, end = 0, pl_dir = 0, recoil = 0;
  bool event = false, link_ok = false, link_human = false, horse = false, ground = true, link_alive = true, cut_helm = false;
  float at_c[3] = {0, 0, 0}, at_r = 0;
  bool at_hit = false, at_on_link = false, at_shield = false;
  bool tate_hit = false, tate_link_sword = false;
};
struct LinkFrame {
  std::uint64_t swing_serial = 0;  // +1 at each distinct swing start
  std::uint64_t shield_serial = 0; // +1 at each real native Shield Attack init
  bool jump_cut = false;
  std::int16_t shape_y = 0;
  post08::LinkView pass;
};
struct Write {
  bool any = false;
  bool set_action = false;
  std::int16_t action = 0;
  bool set_mode = false;
  std::int16_t mode = 0;
  bool set_frame = false;
  float frame = 0;
  bool set_rate = false;
  float rate = 0;
  bool set_invuln = false;
  std::int16_t invuln = 0;
  bool set_timer2 = false;
  std::int16_t timer2 = 0;
  bool set_cur_y = false;
  std::int16_t cur_y = 0;
  float fwd = 0;
  std::int16_t fwd_angle = 0;
  bool set_hp = false;
  bool guard_entry = false;     // native fight_run guard-entry field set
  bool big_damage = false;      // native big_damage() with mHitDirection = search_y
  bool clear_tg = false;        // shield bash Tg mark of this call
  bool bash_se = false;         // native collision SE of a real Shield Attack contact whose Tg mark is cleared below 100 (the native path that plays it no longer runs)
  bool stagger = false;         // native standing stagger: ACTION_DRAWBACK mode 0 with its native recoil fields
  bool set_timer0 = false;      // native S_DAMAGE hold during the standing vulnerability
  std::int16_t timer0 = 0;
  bool head_lock_off = false;   // the crossing of 100 alone never offers Helm Splitter: only a real, new Shield Attack does
  bool clear_tg_sword = false;  // the raised shield took this very swing: its mark on the body spheres of this call goes
  bool clear_learn = false;     // sword bits of e_dn_class::learn: the guard after a wound is decided here, not by vanilla's permanent memory
  bool set_ut2 = false;         // a NEW swing never finds the shield sphere away for the previous one's contact
  std::int16_t hp_extra = 0;    // +50% of the decisive hit's real loss
};
enum class EvK : std::uint8_t {
  None = 0, Admitted, Excluded, HpSet, ComboStart, ComboEnd, ActionStart, ImpactOpen, WindowClose, Contact, LinkGain, LinkBlock, LinkDirectOffer, Roll, GuardEntry,
  Block, EnemyParry, Break, Rearm, CancelSuppressed, Critical, Coverage, Recovered, PostureGain, Trace,
  DeflectRoll, DeflectWindow, DeflectEnd, GuardRoll, GuardEnter, GuardIntentEnd, CounterSelected, CounterGuardExit, CounterCommitted, CounterAtOpen, CounterContact, CounterExpired, HelmPrepared, Decisive, BreakBonus, Down, GetUp, Impact, Count
};
struct Ev {
  EvK k = EvK::None;
  int a = 0, b = 0, c = 0;
  std::uint64_t t = 0;
  float f = 0, g = 0;
};
// FIX04: the ONE final verdict of a physical contact of Link's sword with this actor (shield sphere, body spheres or both).
enum class Tv : std::uint8_t { None = 0, Block, EnemyParry, Uncovered, BodyDamage, ShieldWonBody, Repeat, Count };
inline const char* name(Tv t) noexcept {
  static constexpr const char* n[] = {"NONE", "BLOCK", "ENEMY_PARRY", "SHIELD_NOT_COVERING", "BODY_DAMAGE", "SHIELD_WON_BODY", "SAME_SWING_OR_NO_EFFECT"};
  return unsigned(t) < unsigned(Tv::Count) ? n[unsigned(t)] : "?";
}
struct Trace {
  bool open = false, presented = false, covers = false, body_hit = false, shield_hit = false;
  Tv verdict = Tv::None;
  std::int16_t action = 0, mode = 0, shape_y = 0, search_y = 0, hp_before = 0, hp_after = 0;
  int posture_before = 0, posture_after = 0;
  float frame = 0;
  std::uint64_t local = 0, swing = 0, blocks = 0, parries = 0, rejected = 0, won = 0;
};
// Why a counter intent ended without a contact (or with it).
enum class Cancel : std::uint8_t { None = 0, Lease, Far, Damage, Break, Event, Death, LinkGone, NativeAttack, Count };
inline const char* name(Cancel c) noexcept {
  static constexpr const char* n[] = {"NONE", "LEASE_EXPIRED", "LINK_FAR", "SWORD_DAMAGE", "POSTURE_BREAK", "EVENT", "DEATH", "LINK_GONE", "NATIVE_ATTACK_WHILE_ILLEGAL"};
  return unsigned(c) < unsigned(Cancel::Count) ? n[unsigned(c)] : "?";
}
// One verdict per opened AT window, from what the engine really did.
enum class Cls : std::uint8_t { None = 0, Whiff, LinkGuard, LinkParry, LinkBody, ImmuneValid, MissingFeedback, Count };
inline const char* name(Cls c) noexcept {
  static constexpr const char* n[] = {"NONE", "WHIFF", "LINK_GUARD", "LINK_PARRY", "LINK_BODY", "IMMUNE_VALID", "MISSING_FEEDBACK"};
  return unsigned(c) < unsigned(Cls::Count) ? n[unsigned(c)] : "?";
}
struct Stats {
  std::uint64_t selected[kPatterns] = {}, completed[kPatterns] = {}, impacts_open[kPatternSlots] = {}, ended[unsigned(End::Count)] = {};
  std::uint64_t contact[unsigned(Hit::Count)] = {}, link_parry = 0, d_pair_parry = 0, sword_hits = 0, iron_hits = 0, blocks = 0, enemy_parries = 0, cancels_suppressed = 0;
  std::uint64_t stagger_suppressed_small = 0, stagger_suppressed_big = 0, shield_bash_cleared = 0, guards_native = 0, guards_attempt = 0, guards_boundary = 0;
  std::uint64_t watchdog = 0, sample_gap = 0, armed_ticks = 0, windows = 0, slide = 0;
  std::uint64_t swings_detected = 0, block_pairs = 0, deflect_rolls = 0, deflect_armed = 0, deflect_windows = 0, deflect_expired = 0, guard_rolls[3] = {}, guard_intents[3] = {}, guard_enters = 0, guard_intent_lost = 0, wounds = 0, sword_duplicates = 0, shield_won_body = 0, shield_not_covering = 0, guard_cover_ticks = 0, guard_open_ticks = 0, guard_turn_ticks = 0, trace_verdict[unsigned(Tv::Count)] = {}, block_rear_rejected = 0, rebound_kept = 0, rebound_suppressed = 0, counter_selected[2] = {}, counter_started[2] = {}, counter_at_open = 0, counter_contact = 0, counter_expired = 0, counter_cancel[unsigned(Cancel::Count)] = {};
  std::uint64_t break_bonus_hits = 0, break_bonus_units = 0, break_bonus_lethal = 0, break_standing = 0, helm_prepared = 0, decisive_sword = 0, decisive_helm = 0, punish_end = 0, down_finish = 0, getup = 0, link_parry_by_impact[kPatterns][9] = {};
  std::uint64_t impact_class[unsigned(Cls::Count)] = {}, expect_ticks = 0, candidate_ticks = 0, nocon_ticks = 0, lost_ticks = 0, off_touch_ticks = 0, gap_fixed = 0;
};

// ---------------------------------------------------------------- skipped-pose contact coverage (same discipline as cp06f1: exact engine poses, real Tg cylinders, the engine still decides)
namespace sweep {
using cp06f1::Cyl;
using cp06f1::Sph;
using cp06f1::V3;
inline constexpr unsigned kMaxProbes = 3;
inline constexpr float kMinRate = 1.0001f, kMaxRate = 2.0f;
// Vanilla registers the POST-play pose of every armed tick: first armed PRE frame + 1 is the first pose of the window.
inline constexpr float first_pose(Act a, std::uint8_t win) noexcept { return a == Act::N ? 23.0f : a == Act::D ? (win == 1 ? 26.0f : 14.0f) : 34.0f; }
inline constexpr float radius(Act a, std::uint8_t win) noexcept { return native::kModelSize * (a == Act::N ? 50.0f : a == Act::D ? (win == 1 ? 80.0f : 50.0f) : 70.0f); }
// Poses of this tick's span (pre, post) not registered by the engine, every half frame: one tick never hides a stretch of trajectory longer than half a vanilla frame + the registered pose.
inline unsigned probe_frames(Act a, std::uint8_t win, float pre, float post, bool prev_armed, bool prev_covered, float out[kMaxProbes]) noexcept {
  const float r = post - pre;
  if (!(r >= kMinRate && r <= kMaxRate)) return 0;
  const float first = first_pose(a, win);
  float p = prev_armed ? (prev_covered ? pre + 0.5f : pre) : first;
  unsigned n = 0;
  while (p < post - 0.25f && n < kMaxProbes) {
    out[n++] = p;
    p += 0.5f;
  }
  return n;
}
// The base matrix of this tick already holds the whole native lunge of the tick. Distance the root was behind at pose m (D2: 25/frame along current.angle.y; tail: 30/frame along search_angle_y).
inline constexpr float lunge_back(Act a, std::uint8_t win, float post, float m) noexcept { return a == Act::D ? (win == 1 ? (post - m) * policy::kLungeD2 : 0.0f) : is_tail(a) ? (post - m) * policy::kLungeC : 0.0f; }
enum class Why : std::uint8_t { None = 0, AlreadyTouching, Written, NoMidContact, LinkInvulnerable, NotFast, NotConsecutive, MorphActive, LinkNotCurrent, LinkMissing, OutOfReach, ProbeMismatch, Count };
inline const char* name(Why w) noexcept {
  static constexpr const char* n[] = {"NONE", "ALREADY_TOUCHING", "WRITTEN_EXACT_NATIVE_POSE", "NO_POSE_REACHES_LINK", "LINK_TG_OFF_NATIVE_INVULNERABILITY", "RATE_NOT_ABOVE_1", "NOT_CONSECUTIVE", "MORPH_ACTIVE", "LINK_NOT_CURRENT", "LINK_MISSING", "OUT_OF_REACH", "PROBE_MISMATCH"};
  return unsigned(w) < unsigned(Why::Count) ? n[unsigned(w)] : "?";
}
struct Decision {
  Why why = Why::None;
  bool write = false;
  V3 place{};
};
// A registered sphere crossing an ON cylinder with margin already guarantees the engine contact: nothing is written. A candidate is written ONLY if it really reaches an ON cylinder; the radius is the native one.
inline Decision decide(const Cyl k[3], const Sph& cur, const Sph* c, unsigned n) noexcept {
  Decision d;
  if (cp06f1::touches(k, cur, cp06f1::kWriteSlack, true)) {
    d.why = Why::AlreadyTouching;
    return d;
  }
  bool geom = false;
  for (unsigned i = 0; i < n; ++i) {
    if (!(c[i].r > 0.0f) || !cp06f1::touches(k, c[i], cp06f1::kWriteSlack, false)) continue;
    geom = true;
    if (cp06f1::touches(k, c[i], cp06f1::kWriteSlack, true)) {
      d.why = Why::Written, d.write = true, d.place = c[i].c;
      return d;
    }
  }
  d.why = geom ? Why::LinkInvulnerable : Why::NoMidContact;
  return d;
}
// Performance gate only: a false "out of reach" loses coverage (0.12.0 behaviour), it never creates a contact.
inline bool reachable(const Cyl k[3], const V3& c, const V3& prev, bool have_prev, float r) noexcept {
  const float mx = c.x - prev.x, my = c.y - prev.y, mz = c.z - prev.z;
  const float move = have_prev ? cp06f1::sqrt_upper(mx * mx + my * my + mz * mz) : 250.0f;
  for (int i = 0; i < 3; ++i) {
    const float lim = r + k[i].r + move + 40.0f;
    const float dx = c.x - k[i].c.x, dz = c.z - k[i].c.z;
    if (dx * dx + dz * dz <= lim * lim && c.y + r + move + 40.0f >= k[i].c.y && c.y - r - move - 40.0f <= k[i].c.y + k[i].h) return true;
  }
  return false;
}
}  // namespace sweep
struct ImpactTrack {
  bool open = false, exp_pending = false, crossed_before = false;
  Cls cls = Cls::None;
  std::uint32_t expect = 0, written = 0, off_touch = 0, lost = 0, nocon = 0;
  std::uint8_t pattern = 0, impact = 0, act = 0;
  std::uint64_t open_tick = 0, contact_tick = 0;
  int link_proc = -1;
  std::uint32_t feedback_native = 0, feedback_restored = 0;
};

class Brain {
 public:
  static constexpr unsigned kEv = 14;
  enum class Bk : std::uint8_t { None = 0, Punish, Falling, Down, GettingUp };
  bool bound = false, admitted = false, hp_done = false, dead = false;
  std::uint32_t id = 0;
  std::uint64_t generation = 0, local = 0;
  Excl exclusion = Excl::None;
  PostureState p;
  Episode ep;
  Defense def;
  Rng rng;
  Stats st;
  // single-tick context
  bool armed_last = false, armed_now = false;
  std::uint64_t cur_impact = 0, resolved_impact = 0, impact_counter = 0;
  // defense
  std::uint64_t last_swing = 0, pending_swing_tick = 0, pending_swing_serial = 0, rolled_serial = 0, contact_swing = 0;
  bool attempt_armed = false;
  std::uint64_t guard_tick = 0, guard_seen_from = 0;
  bool in_guard = false, link_gone = false;
  // two ordinary blocks in one exchange
  unsigned chain_count = 0;
  std::uint64_t chain_tick = 0;
  // the prepared new parry window (never a counter)
  struct Deflect {
    bool armed = false;
    std::uint64_t tick = 0, serial = 0, token = 0;
  } deflect;
  std::uint64_t deflect_tokens = 0;
  // progressive guard after real wounds
  unsigned damage_streak = 0;
  std::uint64_t damage_tick = 0, sword_credit_swing = 0, hold_until = 0, hit_lock_until = 0, intent_tick = 0;
  bool guard_intent = false, guard_rearm = false, pass_cover = false;
  std::uint64_t pass_cover_local = ~0ull;
  // one parry verdict per swing, whoever asks first: Link's rebound (his Execute) or this actor's contact
  std::uint64_t verdict_swing = 0, rebound_seen_swing = 0;
  bool verdict_parry = false;
  // short counter intent
  struct Counter {
    bool armed = false, live = false, exit_seen = false, at_open = false, contact = false;
    std::uint8_t kind = 0, source = 0;  // kind 0=N 1=D, source 0=two blocks 1=enemy parry
    std::uint64_t tick = 0, token = 0;
  } counter;
  std::uint64_t counter_tokens = 0;
  bool rest_pending = false;
  // break
  bool break_pending = false, decisive_pending = false, helm_prepared = false;
  std::int16_t decisive_extra = 0;
  Bk phase = Bk::None;
  std::uint64_t break_tick = 0, break_swing = 0, break_shield = 0, shield_serial = 0, bash_se_serial = 0;
  // damage_check call
  std::int16_t hp_pre = 0;
  bool credited_call = false, skipped_big = false, skipped_small = false;
  std::int16_t fc_pre = 0, ut2_pre = 0;
  bool have_at_prev = false;
  float at_prev[3] = {0, 0, 0};
  float max_step = 0;
  ImpactTrack imp, last_imp;
  Trace tr;
  std::uint64_t feedback_impact = 0;
  Ev ev[kEv];
  unsigned nev = 0;
  std::uint64_t ev_dropped = 0;

  void bind(std::uint32_t i, std::uint64_t gen, std::uint64_t seed) noexcept {
    *this = Brain{};
    bound = true, id = i, generation = gen;
    rng.seed(seed ^ (static_cast<std::uint64_t>(i) << 32) ^ gen);
  }
  void push(EvK k, int a = 0, int b = 0, int c = 0, float f = 0, float g = 0) noexcept {
    if (nev >= kEv) {
      ++ev_dropped;
      return;
    }
    Ev& e = ev[nev++];
    e.k = k, e.a = a, e.b = b, e.c = c, e.t = local, e.f = f, e.g = g;
  }
  void admit(const IdIn& in) noexcept {
    exclusion = classify(in);
    admitted = exclusion == Excl::None;
    push(admitted ? EvK::Admitted : EvK::Excluded, int(exclusion), in.arg0, in.arg1);
  }
  static std::int16_t angle_diff(std::int16_t a, std::int16_t b) noexcept { return static_cast<std::int16_t>(static_cast<std::uint16_t>(a) - static_cast<std::uint16_t>(b)); }
  static std::int16_t iabs16(std::int16_t a) noexcept { return a == -32768 ? 32767 : (a < 0 ? static_cast<std::int16_t>(-a) : a); }
  // The one HitIn of a Link contact on at_sph: the Execute PRE judges with it, and the parry FX previews the very same one inside the collision pass.
  HitIn hit_input(const View& v, const LinkFrame& L) const noexcept {
    HitIn h;
    h.at_hit = v.at_hit, h.on_link = v.at_on_link, h.shield = v.at_shield, h.armed_last = armed_last, h.event = v.event, h.link_ok = v.link_ok && v.link_human, h.latched = p.latched;
    h.impact = cur_impact, h.resolved = resolved_impact, h.pass = L.pass;
    return h;
  }
  static bool facing(std::int16_t shape_y, std::int16_t search_y, std::int16_t half) noexcept { return iabs16(angle_diff(shape_y, search_y)) <= half; }
  // FIX04: Link stands inside the arc the shield can face. search_y - shape_y > 0 is the actor's left. Where the sword met the sphere is the engine's business.
  static bool covers(const View& v) noexcept {
    const std::int16_t rel = angle_diff(v.search_y, v.shape_y);
    return rel >= 0 ? rel <= policy::kShieldSideLimit : rel >= static_cast<std::int16_t>(-policy::kOpenSideLimit);
  }
  // ANM_GUARD presents the shield from frame 3 of mode 6 (or at once when re-armed), holds it in 7, and keeps it while mode 8 has not begun to lower it.
  bool presented_now(const View& v) const noexcept {
    return (v.mode == 6 && (guard_rearm || v.frame >= policy::kGuardReadyFrame)) || v.mode == 7 || (v.mode == 8 && v.frame < v.end - policy::kGuardLowerFrames);
  }
  bool presented(const View& v) const noexcept { return presented_now(v); }
  // cLib_addCalcAngleS2(&current.angle.y, search_angle_y, 2, 0x800) exactly: half the remaining angle per tick, at most 11.25 deg.
  void guard_turn(const View& v, Write& w) noexcept {
    const std::int16_t diff = angle_diff(v.search_y, v.cur_y);
    const std::int16_t step = static_cast<std::int16_t>(diff / policy::kGuardTurnScale);
    const std::int16_t d = step > policy::kGuardTurnMax ? policy::kGuardTurnMax : step < -policy::kGuardTurnMax ? static_cast<std::int16_t>(-policy::kGuardTurnMax) : step;
    if (d == 0) return;
    ++st.guard_turn_ticks;
    w.any = true, w.set_cur_y = true;
    w.cur_y = static_cast<std::int16_t>(static_cast<std::uint16_t>(v.cur_y) + static_cast<std::uint16_t>(d));
  }
  // Link's swing direction: he looks along shape_y; the actor sits at search_y + 0x8000 seen from Link.
  static bool swing_aimed(std::int16_t link_shape_y, std::int16_t search_y) noexcept {
    return iabs16(angle_diff(link_shape_y, static_cast<std::int16_t>(static_cast<std::uint16_t>(search_y) + 0x8000u))) <= policy::kSwingAimHalfAngle;
  }

  // ---- Execute PRE: identity write, last pass contact, swing memory, break delivery
  Write on_exec_pre(const View& v, const LinkFrame& L) noexcept {
    Write w;
    nev = 0;
    fc_pre = v.fc;
    ut2_pre = v.ut2;
    if (!admitted) return w;
    if (!hp_done) {
      hp_done = true;
      if (v.hp == native::kNativeHp && v.max_hp == native::kNativeHp) {
        w.any = true, w.set_hp = true;
        push(EvK::HpSet, policy::kMaxHp, v.hp, v.max_hp);
      } else {
        push(EvK::Critical, 1, v.hp, v.max_hp);  // foreign HP model: left untouched
      }
    }
    shield_serial = L.shield_serial;
    if (v.event) {
      if (ep.active) end_episode(End::Event);
      attempt_armed = false;
      armed_last = armed_now = false;
      cancel_counter(Cancel::Event);
      chain_count = 0;
      end_deflect(Cancel::Event);
      end_intent(Cancel::Event);
      return w;
    }
    if (dead || v.hp <= 0) return w;
    const bool gone = !v.link_ok || !v.link_human || !v.link_alive;
    if (gone && !link_gone) {  // the pressure and the exchange of a Link that is not there never survive
      def.reset();
      chain_count = 0;
      damage_streak = 0;
      end_deflect(Cancel::LinkGone);
      end_intent(Cancel::LinkGone);
    }
    link_gone = gone;
    bool aimed_swing = false, new_swing = false;
    // Link's swing start: serial is global; the memory of THIS actor only takes swings aimed at it and in range.
    if (L.swing_serial != last_swing) {
      last_swing = L.swing_serial;
      new_swing = true;
      if (v.link_human && v.pl_dir < policy::kSwingRange && swing_aimed(L.shape_y, v.search_y)) {
        def.stamp(local);
        ++st.swings_detected;
        aimed_swing = true;
        pending_swing_tick = local;
        pending_swing_serial = L.swing_serial;
      }
    }
    def.update(local, v.pl_dir);
    // what the engine really did with the sphere it was given last pass
    const bool on_link = v.at_hit && v.at_on_link;
    if (imp.exp_pending) {
      if (!on_link) {
        if (imp.crossed_before) ++imp.nocon, ++st.nocon_ticks;
        else ++imp.lost, ++st.lost_ticks;
      }
      imp.crossed_before = true;
      imp.exp_pending = false;
    } else if (!on_link) {
      imp.crossed_before = false;
    }
    if (on_link) imp.crossed_before = true;
    // at_sph contact of the last collision pass
    const Hit r = judge_hit(hit_input(v, L));
    armed_last = false;
    if (r != Hit::None) {
      ++st.contact[unsigned(r)];
      push(EvK::Contact, int(r), int(ep.pattern), int(ep.impact), L.pass.frame - L.pass.start, v.frame);
      const int atp = (ep.active && is_tail(kPattern[ep.pattern].a[ep.act])) ? 2 : 1;
      if (imp.open && imp.contact_tick == 0 && (r == Hit::Parry || r == Hit::ParryLatched || r == Hit::Guard || r == Hit::MissedEarly || r == Hit::Direct)) {
        imp.contact_tick = local, imp.link_proc = L.pass.valid ? L.pass.proc : -1;
        imp.cls = (r == Hit::Parry || r == Hit::ParryLatched) ? Cls::LinkParry : r == Hit::Direct ? (L.pass.valid && L.pass.proc == native::kLinkProcGuardSlip ? Cls::ImmuneValid : Cls::LinkBody) : Cls::LinkGuard;
      }
      if (ep.active && ep.pattern >= kPatterns && !counter.contact && r != Hit::Stale && r != Hit::Duplicate) {
        counter.contact = true;
        ++st.counter_contact;
        push(EvK::CounterContact, int(r), int(counter.kind), 0);
      }
      if (r == Hit::Parry || r == Hit::ParryLatched) {
        resolved_impact = cur_impact;
        ++st.link_parry;
        if (ep.active && kPattern[ep.pattern].a[ep.act] == Act::D && ep.window == 1) ++st.d_pair_parry;
        if (ep.active && ep.pattern < kPatterns && ep.impact >= 1 && ep.impact <= 9) ++st.link_parry_by_impact[ep.pattern][ep.impact - 1];
        if (r == Hit::Parry && p.add(policy::kGainLinkParry)) request_break(v);
        if (r == Hit::Parry) push(EvK::PostureGain, policy::kGainLinkParry, p.points(), 0);
      } else if (r == Hit::Guard || r == Hit::MissedEarly) {
        resolved_impact = cur_impact;
        push(EvK::LinkBlock, atp == 2 ? policy::kLinkBlockAtp2 : policy::kLinkBlockAtp1, atp);
      } else if (r == Hit::Direct) {
        resolved_impact = cur_impact;
        push(EvK::LinkDirectOffer, atp == 2 ? policy::kLinkDirectAtp2 : policy::kLinkDirectAtp1, atp);
      }
    }
    if (break_pending) deliver_break(v, w);
    if (v.action == native::kGuard && in_guard) step_guard(v, w, L, aimed_swing, new_swing);
    // standing vulnerability: the native S_DAMAGE wait lasts until the absolute deadline, not one tick more
    if (phase == Bk::Punish && v.action == native::kSDamage) {
      w.any = true, w.set_timer0 = true;
      w.timer0 = local < break_tick + policy::kPunishTicks ? static_cast<std::int16_t>(policy::kPunishHoldTimer0) : std::int16_t(1);
    }
    return w;
  }
  // ---- damage_check PRE
  Write on_damage_pre(const View& v, bool tg_shield_attack, bool tg_sword = false) noexcept {
    Write w;
    credited_call = skipped_big = skipped_small = decisive_pending = false;
    hp_pre = v.hp;
    // FIX03: ONE shield verdict per collision pass, taken here (the first callback of this Execute, before the shield is re-placed) and reused by the
    // contact credit of this same Execute: the shield that wins the body is exactly the shield that is credited, never another reading.
    if (v.tate_hit && v.tate_link_sword && v.action == native::kGuard) {
      pass_cover_local = local;
      pass_cover = presented(v) && covers(v);  // the engine says the sword met the shield sphere; the clip says it was up; the arc says it could face Link
    }
    if (admitted && !dead && (tg_sword || (v.tate_hit && v.tate_link_sword))) {  // FIX04: opens the contact's causal trace; on_exec_post closes it with the final verdict
      tr = Trace{};
      tr.open = true, tr.local = local, tr.swing = last_swing, tr.action = v.action, tr.mode = v.mode, tr.frame = v.frame, tr.shape_y = v.shape_y, tr.search_y = v.search_y;
      tr.presented = presented(v), tr.covers = covers(v), tr.body_hit = tg_sword, tr.shield_hit = v.tate_hit && v.tate_link_sword;
      tr.hp_before = v.hp, tr.posture_before = p.points(), tr.blocks = st.blocks, tr.parries = st.enemy_parries, tr.rejected = st.block_rear_rejected, tr.won = st.shield_won_body;
    }
    if (!admitted || v.invuln != 0 || v.event || v.horse || dead) return w;
    // Shield and body in one pass: the shield sphere really took that swing and faced Link. Otherwise (the open side, the back) the body keeps its
    // native damage. Before the swing arrives, the body itself is only made untouchable while the shield is presented AND facing (on_guard_post).
    if (tg_sword && v.action == native::kGuard && v.tate_hit && v.tate_link_sword) {
      if (pass_cover_local == local && pass_cover) {
        w.any = true, w.clear_tg_sword = true;
        ++st.shield_won_body;
      } else {
        ++st.shield_not_covering;
      }
    }
    if (tg_shield_attack) {
      if (phase == Bk::Punish && !helm_prepared && !break_pending && shield_serial != 0 && shield_serial != break_shield) {
        // A NEW real Shield Attack inside the vulnerability: the native DRAWBACK branch runs untouched (no HP, native clip, native HeadLock => real Helm Splitter)
        helm_prepared = true;
        ++st.helm_prepared;
        push(EvK::HelmPrepared, p.points(), v.hp, static_cast<int>(local - break_tick));
      } else {  // REGLA ABSOLUTA: below 100 Link's Shield Attack on the body neither staggers nor cancels; during the break the first press never renews the deadline
        w.any = true, w.clear_tg = true;
        ++st.shield_bash_cleared;
        if (shield_serial != bash_se_serial) {
          bash_se_serial = shield_serial;
          w.bash_se = true;
        }
      }
    }
    return w;
  }
  // ---- small_damage / big_damage PRE. true = skip the native reaction.
  bool on_small_pre(const View& v, HitKind kind) noexcept { return reaction(v, kind, false); }
  bool on_big_pre(const View& v, HitKind kind) noexcept { return reaction(v, kind, true); }
  // ---- damage_check POST: combo boundary, deferred break, big-damage invulnerability repair
  Write on_damage_post(const View& v) noexcept {
    Write w;
    if (!admitted || v.event || dead) return w;
    if (v.hp < hp_pre) hit_lock_until = local + policy::kHitLockTicks;
    if ((v.learn & policy::kLearnSwordMask) != 0) w.any = true, w.clear_learn = true;
    if (skipped_big) {
      w.any = true, w.set_invuln = true, w.invuln = 10;  // the value of an ordinary hit: never the 1000 big_damage's caller wrote
    }
    if (decisive_extra > 0) {  // +50% of the real loss of the confirmed decisive hit, once, before the fall reads the health
      w.any = true, w.hp_extra = decisive_extra;
      ++st.break_bonus_hits;
      st.break_bonus_units += static_cast<std::uint64_t>(decisive_extra);
      const int after = v.hp - decisive_extra;
      if (after <= 0) ++st.break_bonus_lethal;
      push(EvK::BreakBonus, hp_pre - v.hp, decisive_extra, after > 0 ? after : 0);
      decisive_extra = 0;
    }
    if (decisive_pending) {  // confirmed decisive sword during the vulnerability: the native fall, down flag and Ending Blow route
      decisive_pending = false;
      w.any = true, w.big_damage = true;
      return w;
    }
    if (break_pending) {
      deliver_break(v, w);
      return w;
    }
    if (v.hp <= 0) {
      if (ep.active) end_episode(End::Death);
      return w;
    }
    if (ep.active) step_episode(v, w);
    return w;
  }
  // ---- e_dn_fight_run PRE: true = skip (inside a combo gap: no native decision may run)
  bool on_run_pre(const View& v, Write& w) noexcept {
    if (!admitted || v.event || dead) return false;
    if (ep.active) {
      turn_toward(v, w);
      return true;
    }
    if (rest_pending) {
      rest_pending = false;  // an opening for Link after every episode: the same 1..2 s scale the native timer[2] uses between attacks
      w.any = true, w.set_timer2 = true, w.timer2 = static_cast<std::int16_t>(policy::kRestMin + static_cast<int>(rng.below(policy::kRestSpan)));
    }
    return false;
  }
  // ---- e_dn_fight_run POST: priority counter; native attack decision -> combo; swing opportunity -> parry attempt
  Write on_run_post(const View& v, bool jump_cut) noexcept {
    Write w;
    if (!admitted || v.event || dead || v.hp <= 0 || p.latched) return w;
    const bool native_attack = v.action == native::kAttack0 || v.action == native::kAttack || v.action == native::kTailAttack;
    if (counter.armed) {
      if ((v.action == native::kFightRun || native_attack || v.action == native::kGuard) && v.mode >= 0 && counter_legal(v)) {
        const unsigned pat = counter.kind == 0 ? kCounterN : kCounterD;
        counter.armed = false, counter.live = true, counter.at_open = false, counter.contact = false;
        ++st.counter_started[counter.kind];
        push(EvK::CounterCommitted, int(counter.kind), int(counter.source), static_cast<int>(local - counter.tick));
        if (v.action == native::kGuard) w.set_invuln = true, w.invuln = 0;  // the native guard entry of this very call never started: its 15 is not kept
        start_episode(pat);
        w.any = true, w.set_action = true, w.action = native_action(kPattern[pat].a[0]), w.set_mode = true, w.mode = 0;
        return w;
      }
      if (native_attack) {
        cancel_counter(Cancel::NativeAttack);
      } else {
        if (v.action == native::kGuard) {
          ++st.guards_native;
          enter_guard_tracking();
        }
        return w;
      }
    }
    // The arbitration of every guard decision, one outcome per opportunity: a prepared new parry window, then the guard a wound asked for, then the ordinary attack decision, then the pressure roll.
    if (v.mode >= 0 && (v.action == native::kFightRun || native_attack) && eligible_guard(v)) {
      if (deflect.armed && def.cooled(local) && swing_fresh() && pending_swing_serial != deflect.serial) {
        open_deflect_window();
        attempt_armed = true;
        guard_tick = local;
        hold_until = local + policy::kGuardHoldTicks;
        end_intent(Cancel::None);
        guard_entry_write(v, w, jump_cut);
        ++st.guards_attempt;
        return w;
      }
      if (guard_intent && local - intent_tick >= policy::kGuardReactTicks) {
        if (swing_fresh()) rolled_serial = pending_swing_serial;  // the same swing is not rolled a second time by the pressure path
        guard_intent = false;
        damage_streak = 0;
        hold_until = local + policy::kGuardHoldTicks;
        ++st.guard_enters;
        push(EvK::GuardEnter, int(def.level), static_cast<int>(local - intent_tick));
        guard_entry_write(v, w, jump_cut);
        return w;
      }
    }
    if (native_attack) {
      const unsigned pat = rng.below(kPatterns);  // exactly one draw per episode start, exact 25% each
      ++st.selected[pat];
      start_episode(pat);
      w.any = true, w.set_action = true, w.action = native_action(kPattern[pat].a[0]), w.set_mode = true, w.mode = 0;
      return w;
    }
    if (v.action == native::kGuard) {  // native guard entry: an ordinary block unless this same swing's roll arms a parry attempt
      ++st.guards_native;
      enter_guard_tracking();
      if (swing_pending() && def.cooled(local) && facing(v.shape_y, v.search_y, policy::kWayHalfAngle)) arm_attempt(v);
      return w;
    }
    if (v.action == native::kFightRun && swing_pending() && eligible_defense(v)) {
      if (arm_attempt(v)) {
        hold_until = local + policy::kGuardHoldTicks;
        guard_entry_write(v, w, jump_cut);
        ++st.guards_attempt;
      }
    }
    return w;
  }
  // ---- e_dn_guard POST, before Execute places the body spheres: the shield protects what it covers. Native invulnerability_timer stays only where the shield is really up and in front.
  Write on_guard_post(const View& v) noexcept {
    Write w;
    if (!admitted || v.event || dead || v.action != native::kGuard || v.mode <= 1) return w;  // modes 0/1: the native hop from a jump cut
    if (v.mode == 7) guard_rearm = false;
    const bool up = presented(v);
    // FIX03: the guard turns toward Link with the native fight_run law (never a snap), and protects only what the shield really faces now.
    if (v.mode >= 5 && v.mode <= 8) guard_turn(v, w);
    const bool front = covers(v);
    if (up) ++(front ? st.guard_cover_ticks : st.guard_open_ticks);
    if (up && front) {
      if (v.invuln == 0) w.any = true, w.set_invuln = true, w.invuln = 2;
    } else if (v.invuln > 0 && local >= hit_lock_until) {
      w.any = true, w.set_invuln = true, w.invuln = 0;
    }
    return w;
  }
  // Link's Execute asks before this actor's contact is processed, and the answer must be the one that contact will get.
  bool contact_is_parry(const View& v, std::uint64_t swing, std::uint64_t now) noexcept {
    if (verdict_swing == swing && swing != 0) return verdict_parry;
    verdict_swing = swing;
    verdict_parry = parry_contact(attempt_armed, guard_tick, now, facing(v.shape_y, v.search_y, policy::kFrontHalfAngle), def.cooled(now));  // a parry is frontal; a block has the wider arc
    return verdict_parry;
  }
  // checkCutReverseAt POST: true = rebound (a real parry). An ordinary block lets Link's offensive go on.
  bool sword_rebounds(const View& v, std::uint64_t swing) noexcept {
    const bool p = contact_is_parry(v, swing, local + 1);
    if (rebound_seen_swing != swing || swing == 0) {  // three sword capsules, several ticks: one count per swing
      rebound_seen_swing = swing;
      if (p) ++st.rebound_kept;
      else ++st.rebound_suppressed;
    }
    return p;
  }
  // ---- attack functions
  Write on_attack_pre(const View& v) noexcept {
    Write w;
    // `int frame` of the attack function reads the PREVIOUS clip in mode 0 and would open a false AT window: neutralise only that stale value.
    if (admitted && ep.active && v.mode == 0 && !v.event) {
      w.any = true, w.set_frame = true, w.frame = 0;
    }
    return w;
  }
  Write on_attack_post(const View& v) noexcept {
    Write w;
    armed_now = v.at_chk != 0;
    if (!admitted || v.event || dead) return w;
    if (armed_now) ++st.armed_ticks;
    if (!ep.active) {
      if (armed_now && cur_impact == 0) cur_impact = ++impact_counter;
      if (!armed_now) cur_impact = 0;
      armed_last = armed_now;
      return w;
    }
    const Act a = kPattern[ep.pattern].a[ep.act];
    if (v.action != native_action(a)) {  // native changed the action during this call: step_episode classifies it next damage_check POST
      armed_last = armed_now;
      return w;
    }
    const std::uint8_t win = (a == Act::D && v.tail == 1) ? 1 : 0;
    if (armed_now) {
      if (!ep.window_on || ep.window != win) {
        if (ep.window_on) close_impact();
        ep.window_on = true;
        ep.window = win;
        ep.any_window = true;
        ++ep.impact;
        cur_impact = ++impact_counter;
        ep.last_open_tick = local;
        ++st.windows;
        ++st.impacts_open[ep.pattern];
        have_at_prev = false;
        max_step = 0;
        imp = ImpactTrack{};
        imp.open = true, imp.pattern = ep.pattern, imp.impact = ep.impact, imp.act = ep.act, imp.open_tick = local;
        push(EvK::ImpactOpen, int(ep.pattern), int(ep.impact), int(a), v.frame, v.rate);
        if (ep.pattern >= kPatterns && !counter.at_open) {
          counter.at_open = true;
          ++st.counter_at_open;
          push(EvK::CounterAtOpen, int(counter.kind), 0, 0);
        }
        const bool last_window = a != Act::D || win == 1;
        if (last_window && ep.act + 1u < kPattern[ep.pattern].acts) {
          const std::int64_t gap = round_ticks(kPattern[ep.pattern].gap_s[ep.act]);
          const std::int64_t wu = windup_of(ep.pattern, ep.act + 1u);
          const std::int64_t at = static_cast<std::int64_t>(local) + gap - wu;
          ep.next_start = at < static_cast<std::int64_t>(local) ? local : static_cast<std::uint64_t>(at);
        }
      }
    } else if (ep.window_on) {
      ep.window_on = false;
      cur_impact = 0;
      ep.last_window_close = local;
      push(EvK::WindowClose, int(ep.pattern), int(ep.impact), 0, v.frame, max_step);
      close_impact();
      if (a != Act::D || ep.window == 1) {
        ep.act_done = true;
      }
    }
    // Assist inside the wind-up only: face Link and close the distance with the native-rate step; never during an AT window.
    if (!armed_now && !ep.any_window && !ep.act_done) {
      turn_toward(v, w);
      if (v.pl_dir > standoff(a) && v.ground) {
        const float gap = v.pl_dir - standoff(a);
        w.any = true;
        w.fwd = gap < policy::kSlideStep ? gap : policy::kSlideStep;
        w.fwd_angle = w.set_cur_y ? w.cur_y : v.cur_y;
        ++st.slide;
      }
    }
    // Native per-tick lunge conserved in total: fewer ticks in the window at rate r, so each tick adds (r-1)*step in the same direction, before CrrPos.
    if (armed_now && v.tail == 1 && a == Act::D) {
      w.any = true;
      w.fwd += (kRateD - 1.0f) * policy::kLungeD2;
      w.fwd_angle = v.cur_y;
    } else if (armed_now && v.tail == 2 && is_tail(a)) {
      w.any = true;
      w.fwd += (rate_of(a) - 1.0f) * policy::kLungeC;
      w.fwd_angle = v.search_y;
    }
    armed_last = armed_now;
    return w;
  }
  // ---- at_hit_check POST inside ACTION_ATTACK: true = return NULL (the native cancellation can never run; the At/Tg records, Link's reaction, sparks and hitstop are decided elsewhere and are not touched)
  bool on_at_hit_post(const View& v, bool ret_is_link) noexcept {
    if (!admitted || !ret_is_link || v.action != native::kAttack) return false;
    ++st.cancels_suppressed;
    return true;
  }
  // ---- anm_init
  bool on_anm_pre(int anm_id, float& rate) noexcept {
    if (!admitted || !ep.active || !ep.anm_pending) return false;
    const Act a = kPattern[ep.pattern].a[ep.act];
    if (anm_id != native_anm(a)) return false;
    rate = rate_of(a);
    return true;
  }
  bool on_anm_post(int anm_id, float& frame) noexcept {
    if (!admitted || !ep.active || !ep.anm_pending) return false;
    const Act a = kPattern[ep.pattern].a[ep.act];
    if (anm_id != native_anm(a)) return false;
    ep.anm_pending = false;
    frame = ep.pattern >= kPatterns ? phase_for(a, 0.0f) : start_frame(ep.pattern, ep.act);
    push(EvK::ActionStart, int(ep.pattern), int(ep.act), int(a), frame, rate_of(a));
    return true;
  }
  // ---- Execute POST
  Write on_exec_post(const View& v, const LinkFrame& L) noexcept {
    Write w;
    const bool ran = v.fc != fc_pre;
    if (!admitted || !ran) return w;
    ++local;
    if (v.event) return w;
    // real accepted tate_sph contact: native moved unk_timer_2 0 -> 10 in this very Execute
    if (v.ut2 == 10 && ut2_pre == 0 && v.tate_hit && v.tate_link_sword && !dead) resolve_guard_contact(v, L, w);
    close_trace(v);
    if (v.action != native::kGuard && in_guard) {
      in_guard = false;
      attempt_armed = false;
    } else if (v.action == native::kGuard && !in_guard) {
      enter_guard_tracking();
    }
    // AT sampling diagnostic
    if (armed_now) {
      if (have_at_prev) {
        const float dx = v.at_c[0] - at_prev[0], dy = v.at_c[1] - at_prev[1], dz = v.at_c[2] - at_prev[2];
        const float d2 = dx * dx + dy * dy + dz * dz;
        const float bound = 2.0f * (v.at_r + policy::kThinnestLinkRadius);
        if (d2 > max_step * max_step) max_step = root_approx(d2);
        if (d2 > bound * bound) {
          ++st.sample_gap;
          push(EvK::Coverage, int(ep.pattern), int(ep.impact), 0, root_approx(d2), bound);
        }
      }
      have_at_prev = true;
      at_prev[0] = v.at_c[0], at_prev[1] = v.at_c[1], at_prev[2] = v.at_c[2];
    } else {
      have_at_prev = false;
    }
    armed_now = false;
    // regeneration, break recovery, death, watchdogs
    if (v.hp <= 0 && !dead) {
      dead = true;
      if (ep.active) end_episode(End::Death);
      cancel_counter(Cancel::Death);
    }
    const bool native_recoil = v.action == native::kDamage;
    p.regen(!dead && !p.latched && !native_recoil, v.hp, policy::kMaxHp);
    if (phase != Bk::None) step_break(v, w);
    if (counter.armed) step_counter(v, w);
    if (deflect.armed && (local - deflect.tick > policy::kDeflectLeaseTicks || v.pl_dir > policy::kCounterExpireDistance)) end_deflect(v.pl_dir > policy::kCounterExpireDistance ? Cancel::Far : Cancel::Lease);
    if (guard_intent && (local - intent_tick > policy::kGuardIntentLeaseTicks || v.pl_dir > policy::kCounterExpireDistance)) end_intent(v.pl_dir > policy::kCounterExpireDistance ? Cancel::Far : Cancel::Lease);
    if (damage_streak != 0 && local - damage_tick > policy::kDamageStreakTicks) damage_streak = 0;
    if (ep.active && local - (ep.act_tick) > policy::kActionWatchdog) {
      ++st.watchdog;
      end_episode(End::Watchdog);
    }
    if (v.action == native::kGuard && in_guard && local - guard_seen_from > policy::kGuardWatchdog) {  // native exit path, never a frozen guard
      w.any = true, w.set_mode = true, w.mode = 8, w.set_rate = true, w.rate = 1.0f;
      ++st.watchdog;
      push(EvK::Critical, 3, int(v.mode), 0);
      guard_seen_from = local;
    }
    return w;
  }
  // ---- coverage result of this tick, from the adapter (it holds the real cylinders and poses). Judged by the next Execute PRE against the engine's record.
  void on_sweep(bool expect, bool written, bool off_touch, bool candidate) noexcept {
    if (!imp.open) return;
    imp.exp_pending = expect || written;
    if (imp.exp_pending) ++imp.expect, ++st.expect_ticks;
    if (written) ++imp.written, ++st.gap_fixed;
    if (off_touch) ++imp.off_touch, ++st.off_touch_ticks;
    if (candidate) ++st.candidate_ticks;
  }
  bool swing_fresh() const noexcept {
    return pending_swing_serial != 0 && pending_swing_serial != rolled_serial && local - pending_swing_tick <= policy::kPendingSwingTicks;
  }
  // The lease: one swing, one roll, never a future input. At level B/C the roll can wait for the end of a committed state.
  bool swing_pending() const noexcept {
    return pending_swing_serial != 0 && pending_swing_serial != rolled_serial && local - pending_swing_tick <= (def.level >= 1 ? policy::kPressureLeaseTicks : policy::kPendingSwingTicks);
  }
  bool double_gap() const noexcept {  // D1 closed, D2 not yet
    return ep.active && kPattern[ep.pattern].a[ep.act] == Act::D && ep.impact > 0 && ep.window == 0 && !ep.window_on && !ep.act_done;
  }
  bool impact_gap() const noexcept {  // AT closed and this episode still owes an impact: the only span where Link's fast shield re-arm may be used
    return ep.active && ep.impact > 0 && !ep.window_on && ep.impact < kPattern[ep.pattern].impacts;
  }
  bool owes_impact() const noexcept { return ep.active && ep.impact < kPattern[ep.pattern].impacts; }
  void on_delete() noexcept {
    if (ep.active) end_episode(End::SceneOrDelete);
    cancel_counter(Cancel::Event);
  }
  void end_episode(End e) noexcept {
    if (!ep.active) return;
    if (ep.window_on) close_impact();
    if (e == End::Completed && ep.pattern < kPatterns) ++st.completed[ep.pattern];
    ++st.ended[unsigned(e)];
    push(EvK::ComboEnd, int(ep.pattern), int(e), int(ep.impact));
    ep = Episode{};
    cur_impact = 0;
    rest_pending = true;
    counter.live = false;
  }
  static float root_approx(float x) noexcept {  // no CRT import; 8 Newton steps are plenty for a diagnostic
    if (!(x > 0.0f)) return 0.0f;
    float g = x > 1.0f ? x * 0.5f : 1.0f;
    for (int i = 0; i < 8; ++i) g = 0.5f * (g + x / g);
    return g;
  }

 private:
  // One verdict per contact, from what really changed in this Execute.
  void close_trace(const View& v) noexcept {
    if (!tr.open || tr.local + 1 != local) {
      tr.open = false;
      return;
    }
    tr.open = false;
    tr.hp_after = v.hp, tr.posture_after = p.points();
    tr.verdict = st.enemy_parries != tr.parries ? Tv::EnemyParry : st.blocks != tr.blocks ? Tv::Block : v.hp < tr.hp_before ? Tv::BodyDamage : st.block_rear_rejected != tr.rejected ? Tv::Uncovered : st.shield_won_body != tr.won ? Tv::ShieldWonBody : Tv::Repeat;
    ++st.trace_verdict[unsigned(tr.verdict)];
    push(EvK::Trace, int(tr.verdict), 0, 0);
  }
  void close_impact() noexcept {
    if (!imp.open) return;
    imp.open = false;
    if (imp.cls == Cls::None) imp.cls = imp.lost ? Cls::MissingFeedback : imp.off_touch ? Cls::ImmuneValid : Cls::Whiff;
    ++st.impact_class[unsigned(imp.cls)];
    last_imp = imp;
    push(EvK::Impact, int(imp.cls), int(imp.pattern), int(imp.impact));
  }
  void cancel_counter(Cancel c) noexcept {
    if (!counter.armed) return;
    counter.armed = false;
    ++st.counter_expired;
    ++st.counter_cancel[unsigned(c)];
    push(EvK::CounterExpired, int(c), int(counter.kind), static_cast<int>(local - counter.tick));
  }
  void end_deflect(Cancel c) noexcept {
    if (!deflect.armed) return;
    deflect.armed = false;
    if (c != Cancel::None) ++st.deflect_expired;
    push(EvK::DeflectEnd, int(c), 0, static_cast<int>(local - deflect.tick));
  }
  void end_intent(Cancel c) noexcept {
    if (!guard_intent) return;
    guard_intent = false;
    if (c != Cancel::None) ++st.guard_intent_lost;
    push(EvK::GuardIntentEnd, int(c), 0, static_cast<int>(local - intent_tick));
  }
  void open_deflect_window() noexcept {  // the prepared token is spent on ONE real swing; a contact outside its window is an ordinary block
    rolled_serial = pending_swing_serial;
    ++st.deflect_windows;
    push(EvK::DeflectWindow, static_cast<int>(deflect.token), int(def.level), static_cast<int>(local - deflect.tick));
    deflect.armed = false;
  }
  bool eligible_guard(const View& v) const noexcept {
    return v.ground && v.recoil < 1.0f && v.pl_dir < native::kGuardRange && facing(v.shape_y, v.search_y, policy::kWayHalfAngle);
  }
  // A real swing while the shield is up. A prepared parry needs a NEW presentation: ANM_GUARD from its start through the native mode 5 (voice and clip included), never an immobile shield.
  void step_guard(const View& v, Write& w, const LinkFrame& L, bool aimed_swing, bool new_swing) noexcept {
    if (new_swing && v.ut2 > 0) w.any = true, w.set_ut2 = true;  // FIX04: the shield sphere is back for ANY new swing, or a swing the aim test missed finds it parked
    if (aimed_swing) {
      hold_until = local + policy::kGuardHoldRefreshTicks > hold_until ? local + policy::kGuardHoldRefreshTicks : hold_until;
      if (deflect.armed && def.cooled(local) && v.mode >= 6 && v.mode <= 8 && facing(v.shape_y, v.search_y, policy::kFrontHalfAngle)) {
        pending_swing_serial = L.swing_serial;
        open_deflect_window();
        attempt_armed = true;
        guard_tick = local > 0 ? local - 1 : 0;  // entered before this Execute's e_dn_guard: the first real pass is delta 3, as from FIGHT_RUN POST
        guard_rearm = true;
        verdict_swing = 0;
        hold_until = local + policy::kGuardHoldTicks;
        w.any = true, w.set_mode = true, w.mode = 5;
      }
    }
    if (deflect.armed && deflect.tick + policy::kDeflectLeaseTicks > hold_until) hold_until = deflect.tick + policy::kDeflectLeaseTicks;
    if (local < hold_until && v.mode >= 5 && v.mode <= 7) w.any = true, w.set_timer0 = true, w.timer0 = static_cast<std::int16_t>(policy::kPunishHoldTimer0);
  }
  // A real wound from a distinct swing: at most one preparation decision, never retroactive.
  void on_wound() noexcept {
    ++st.wounds;
    rolled_serial = pending_swing_serial;  // the swing that wounded has had its decision: the pressure roll does not get it too
    cancel_counter(Cancel::Damage);
    chain_count = 0;
    end_deflect(Cancel::Damage);
    if (damage_streak < 3) ++damage_streak;
    damage_tick = local;
    if (guard_intent || in_guard) return;
    const unsigned idx = damage_streak - 1u;
    const int pct = idx == 0 ? policy::kGuardPctHit1 : idx == 1 ? policy::kGuardPctHit2 : 100;
    ++st.guard_rolls[idx];
    const bool yes = pct >= 100 || rng.below(100) < static_cast<std::uint32_t>(pct);
    push(EvK::GuardRoll, static_cast<int>(damage_streak), pct, yes ? 1 : 0);
    if (!yes) return;
    ++st.guard_intents[idx];
    guard_intent = true;
    intent_tick = local;
  }
  bool counter_legal(const View& v) const noexcept {
    return v.ground && v.recoil < 1.0f && v.hp > 0 && v.link_ok && v.link_human && v.link_alive && v.seen_timer != 0 && v.pl_dir <= native::kAttackInitRange && iabs16(v.search_x) < policy::kCounterHeightHalf &&
           facing(v.shape_y, v.search_y, policy::kCounterFaceHalf);
  }
  void arm_counter(std::uint8_t source) noexcept {
    if (counter.armed || counter.live) return;  // one response at a time: never a double answer to the same exchange
    counter = Counter{};
    counter.armed = true, counter.source = source, counter.tick = local, counter.token = ++counter_tokens;
    counter.kind = static_cast<std::uint8_t>(rng.below(2));  // exact 50/50, independent of the 25% combo draw
    ++st.counter_selected[counter.kind];
    push(EvK::CounterSelected, int(counter.kind), int(source), 0);
  }
  // The native guard stays ONLY until the intent can leave through its own exit (clip of mode 8 -> FIGHT_RUN). Mode 5 has not started ANM_GUARD yet: it is left a tick later.
  void guard_exit(const View& v, Write& w) noexcept {
    if (v.action != native::kGuard || (v.mode != 6 && v.mode != 7)) return;
    w.any = true, w.set_mode = true, w.mode = 8, w.set_rate = true, w.rate = 1.0f;
    if (!counter.exit_seen) {
      counter.exit_seen = true;
      push(EvK::CounterGuardExit, int(v.mode), 0, 0);
    }
  }
  void step_counter(const View& v, Write& w) noexcept {
    if (!v.link_ok || !v.link_human || !v.link_alive) cancel_counter(Cancel::LinkGone);
    else if (local - counter.tick > policy::kCounterLeaseTicks) cancel_counter(Cancel::Lease);
    else if (v.pl_dir > policy::kCounterExpireDistance) cancel_counter(Cancel::Far);
    else guard_exit(v, w);
  }
  void step_break(const View& v, Write& w) noexcept {
    if (dead) {
      phase = Bk::None;
      return;
    }
    if (phase == Bk::Punish) {
      if (v.action == native::kDamage) {  // a native knockdown from another source (or the decisive fall already applied)
        phase = Bk::Falling;
        return;
      }
      if (!helm_prepared) w.any = true, w.head_lock_off = true;  // also on the clip's last tick: Link must never read a HeadLock the crossing alone raised
      if (v.action == native::kFightRun || v.action == native::kNormal) {
        if (!helm_prepared && local < break_tick + policy::kPunishTicks) {  // the stagger clip ended: dazed on its feet until the absolute deadline
          w.any = true, w.set_action = true, w.action = native::kSDamage, w.set_mode = true, w.mode = 0;
        } else {
          finish_break(v, true);
        }
        return;
      }
      if (local - break_tick > policy::kBreakWatchdog) {  // a foreign state kept it (water, fall, scripted path): never a latched bar for ever
        push(EvK::Critical, 2, int(v.mode), v.hp);
        finish_break(v, true);
      }
      return;
    }
    // Falling / Down / GettingUp: entirely native (e_dn_damage); the posture rearms only after the complete recovery.
    if (v.action != native::kDamage) {
      finish_break(v, false);
      return;
    }
    if (phase == Bk::Falling && v.mode == 3) {
      phase = Bk::Down;
      ++st.down_finish;
      push(EvK::Down, v.hp, 0, 0);
    } else if (phase == Bk::Down && v.mode == 20) {
      phase = Bk::GettingUp;
      ++st.getup;
      push(EvK::GetUp, v.hp, 0, 0);
    } else if (local - break_tick > policy::kBreakWatchdog) {
      push(EvK::Critical, 2, int(v.mode), v.hp);
      break_tick = local;
    }
  }
  void finish_break(const View& v, bool standing) noexcept {
    phase = Bk::None;
    helm_prepared = false;
    if (standing) ++st.punish_end;
    p.rearm();
    push(EvK::Rearm, p.points(), int(v.action), int(v.mode));
  }
  bool reaction(const View& v, HitKind kind, bool big) noexcept {
    if (!admitted || v.event || v.horse || dead) return false;
    const bool lethal = v.hp <= 0;
    if (kind == HitKind::Sword) {
      if (lethal) return false;  // native death, always
      if (phase == Bk::Punish) {
        const bool first = !credited_call;
        credited_call = true;
        if (first) ++st.sword_hits;
        // Decisive = a real HP loss from a NEW swing (Link's cut serial moved after the break; the swing that crossed 100 can never be its own punishment), or the Helm Splitter this break prepared.
        if (first && v.hp < hp_pre && last_swing != break_swing) {
          phase = Bk::Falling;
          break_tick = local;
          const bool helm = v.cut_helm && helm_prepared;
          if (helm) ++st.decisive_helm;
          else ++st.decisive_sword;
          push(EvK::Decisive, helm ? 1 : 0, v.hp, p.points());
          decisive_extra = static_cast<std::int16_t>((hp_pre - v.hp + 1) / 2);  // the Breaker's rule: extra = (base + 1) / 2 over the real loss
          if (big) return false;  // the native big_damage is that fall
          decisive_pending = true;
          skipped_small = true;
          return true;
        }
        if (big) skipped_big = true;
        else skipped_small = true;
        return true;
      }
      if (!credited_call) {
        credited_call = true;
        ++st.sword_hits;
        if (v.hp < hp_pre) {
          if (last_swing != 0 && sword_credit_swing == last_swing) {
            ++st.sword_duplicates;  // the same swing again after its invulnerability: HP as native, no second credit, no second decision
          } else {
            sword_credit_swing = last_swing;
            on_wound();
            if (p.add(policy::kGainSword)) request_break(v);
            push(EvK::PostureGain, policy::kGainSword, p.points(), 1);
          }
        }
      }
      if (big) {
        skipped_big = true;
        ++st.stagger_suppressed_big;
      } else {
        skipped_small = true;
        ++st.stagger_suppressed_small;
      }
      return true;
    }
    if (kind == HitKind::IronBall && !credited_call) {
      credited_call = true;
      ++st.iron_hits;
      const bool crossed = !lethal && p.add(policy::kGainIronBall);
      if (!lethal) push(EvK::PostureGain, policy::kGainIronBall, p.points(), 2);
      if (crossed) {  // same semantics as every other crossing: the standing stagger replaces whatever native reaction this hit had
        request_break(v);
        if (big) skipped_big = true;
        else skipped_small = true;
        return true;
      }
    }
    return false;  // heavy/bomb/arrow/other: native reaction kept (documented native exceptions)
  }
  void request_break(const View& v) noexcept {
    (void)v;
    break_pending = true;
  }
  static bool breakable(std::int16_t a) noexcept {  // combat states only: water, falls, wolf bite, jumps, bomb kicks and scripted paths are never overwritten
    return a == native::kNormal || a == native::kFightRun || a == native::kAttack0 || a == native::kAttack || a == native::kTailAttack || a == native::kGuard || a == 9 || a == 10 || a == native::kDrawback || a == native::kSDamage;
  }
  void deliver_break(const View& v, Write& w) noexcept {
    if (v.hp <= 0) {
      break_pending = false;
      return;
    }
    if (v.action == native::kDamage) {  // already in a native knock-down (a real bomb, ...): no standing window can exist, that fall is the break
      break_pending = false;
      if (ep.active) end_episode(End::Posture100);
      cancel_counter(Cancel::Break);
      end_deflect(Cancel::Break);
      end_intent(Cancel::Break);
      phase = Bk::Falling;
      break_tick = local;
      push(EvK::Break, p.points(), v.hp, 1);
      return;
    }
    if (!breakable(v.action)) return;  // stays pending and latched until the native state ends; the actor keeps running native code meanwhile
    break_pending = false;
    if (ep.active) end_episode(End::Posture100);
    attempt_armed = false;
    cancel_counter(Cancel::Break);
    end_deflect(Cancel::Break);
    end_intent(Cancel::Break);
    damage_streak = 0;
    chain_count = 0;
    def.reset();
    rest_pending = false;
    phase = Bk::Punish;
    break_tick = local;
    break_swing = last_swing;
    break_shield = shield_serial;
    helm_prepared = false;
    ++st.break_standing;
    w.any = true, w.stagger = true;
    push(EvK::Break, p.points(), v.hp, 0);
  }
  void start_episode(unsigned pat) noexcept {
    ep = Episode{};
    ep.active = true, ep.pattern = static_cast<std::uint8_t>(pat), ep.anm_pending = true;
    ep.start_tick = ep.act_tick = local;
    ep.token = ++tokens_;
    chain_count = 0;  // a response or a new combo closes the defensive exchange
    end_deflect(Cancel::NativeAttack);
    attempt_armed = false;
    push(EvK::ComboStart, int(pat), int(kPattern[pat].acts), int(kPattern[pat].impacts));
  }
  void step_episode(const View& v, Write& w) noexcept {
    const Pattern& P = kPattern[ep.pattern];
    const Act a = P.a[ep.act];
    const bool in_expected = v.action == native_action(a);
    if (!in_expected && v.action != native::kFightRun) {
      end_episode(End::NativePriority);
      return;
    }
    // native returned to FIGHT_RUN before the window (only possible if another mod rewrote the action): never a stuck combo
    // (also the normal path when the clip's own end arrives right at the window's end: FIGHT_RUN means the AT is closed)
    if (v.action == native::kFightRun && !ep.anm_pending) {
      ep.act_done = true;
      if (ep.window_on) close_impact();
      ep.window_on = false;
      cur_impact = 0;
    }
    if (!ep.act_done) return;
    if (ep.window_on) return;  // AT open: never a boundary
    if (ep.act + 1u >= P.acts) {
      end_episode(End::Completed);  // the last action keeps its complete native recovery
      return;
    }
    const unsigned next = ep.act + 1u;
    // Voluntary defense: only here (AT closed, between actions, never inside the D pair) and only for a real, fresh swing.
    if (swing_fresh() && eligible_boundary(v) && boundary_allowed(ep.pattern, next, def.level, ep.voluntary)) {
      if (arm_attempt(v)) {
        if (guard_tick > 0) --guard_tick;  // entered before the action switch: e_dn_guard runs this same tick, one tick earlier than a FIGHT_RUN POST entry
        ep.voluntary = 1;
        end_episode(End::AdaptiveDefense);
        guard_entry_write(v, w, false);
        ++st.guards_boundary;
        return;
      }
    }
    if (local >= ep.next_start) {
      ep.act = static_cast<std::uint8_t>(next);
      ep.act_done = false, ep.any_window = false, ep.window_on = false, ep.window = 0, ep.anm_pending = true;
      ep.act_tick = local;
      w.any = true, w.set_action = true, w.action = native_action(P.a[next]), w.set_mode = true, w.mode = 0;
    } else if (local > ep.last_open_tick + policy::kMaxGapTicks + 60) {
      ++st.watchdog;
      end_episode(End::Watchdog);
    }
  }
  void turn_toward(const View& v, Write& w) noexcept {
    const std::int16_t d = angle_diff(v.search_y, v.cur_y);
    if (d == 0) return;
    const std::int16_t step = d > policy::kTurnStep ? policy::kTurnStep : (d < -policy::kTurnStep ? static_cast<std::int16_t>(-policy::kTurnStep) : d);
    w.any = true, w.set_cur_y = true;
    w.cur_y = static_cast<std::int16_t>(static_cast<std::uint16_t>(v.cur_y) + static_cast<std::uint16_t>(step));
  }
  bool eligible_defense(const View& v) const noexcept {
    return def.cooled(local) && v.ground && v.recoil < 1.0f && v.pl_dir < native::kGuardRange && facing(v.shape_y, v.search_y, policy::kWayHalfAngle);
  }
  bool eligible_boundary(const View& v) const noexcept { return eligible_defense(v); }
  bool arm_attempt(const View& v) noexcept {
    (void)v;
    rolled_serial = pending_swing_serial;  // one roll per real swing: never a re-evaluation
    const int lvl = def.level;
    const bool hit = def.roll(rng);
    push(EvK::Roll, lvl, hit ? 1 : 0, Defense::percent(lvl));
    if (hit) {
      attempt_armed = true;
      guard_tick = local;
    }
    return hit;
  }
  void guard_entry_write(const View& v, Write& w, bool jump_cut) noexcept {
    w.any = true, w.set_action = true, w.action = native::kGuard, w.set_mode = true, w.mode = jump_cut ? 0 : 5, w.guard_entry = true;
    enter_guard_tracking();
    push(EvK::GuardEntry, def.level, int(w.mode), int(attempt_armed));
    (void)v;
  }
  void enter_guard_tracking() noexcept {
    in_guard = true;
    guard_seen_from = local;
  }
  // Two ordinary blocks of distinct swings: one roll, then at most one prepared NEW window. It authorizes nothing else: without a third real contact in it, nothing happens.
  void note_block() noexcept {
    if (local - chain_tick > policy::kBlockChainTicks) chain_count = 0;
    ++chain_count;
    chain_tick = local;
    if (chain_count < 2) return;
    chain_count = 0;  // the same two contacts are never counted again, whatever the guard does afterwards
    ++st.block_pairs;
    if (deflect.armed || counter.armed || counter.live || !def.cooled(local)) return;
    ++st.deflect_rolls;
    const bool armed = rng.below(100) < static_cast<std::uint32_t>(policy::kDeflectPct);
    push(EvK::DeflectRoll, armed ? 1 : 0, policy::kDeflectPct, 0);
    if (!armed) return;
    ++st.deflect_armed;
    deflect = Deflect{};
    deflect.armed = true, deflect.tick = local, deflect.serial = contact_swing, deflect.token = ++deflect_tokens;
  }
  void resolve_guard_contact(const View& v, const LinkFrame& L, Write& w) noexcept {
    if (contact_swing == L.swing_serial && L.swing_serial != 0) return;  // one credit per swing, whatever the sword re-touches
    contact_swing = L.swing_serial;
    // local was already advanced for this Execute: the verdict of damage_check is the one stored one tick "earlier" (0.12.3 compared without the +1 and never reused it).
    const bool front = pass_cover_local + 1 == local ? pass_cover : presented(v) && covers(v);
    // The guard started at guard_tick and this contact came from the previous collision pass.
    if (contact_is_parry(v, L.swing_serial, local)) {
      attempt_armed = false;
      def.cooldown_until = local + Defense::cooldown_of(def.level);
      ++st.enemy_parries;
      chain_count = 0;
      push(EvK::EnemyParry, policy::kGainLinkOnEnemyParry, def.level, static_cast<int>(local - guard_tick));
      push(EvK::LinkGain, policy::kGainLinkOnEnemyParry);
      // A won opening: the next legal moment answers with one short native attack. step_counter below (this same Execute POST) takes the guard out through its native exit (mode 8).
      arm_counter(1);
      return;
    }
    attempt_armed = false;
    if (!front) {  // a touch of the shield sphere that does not face Link (behind, open side, not yet presented) covers nothing and credits nothing
      ++st.block_rear_rejected;
      return;
    }
    ++st.blocks;
    push(EvK::Block, policy::kGainBlocked, p.points(), 0);
    if (p.add(policy::kGainBlocked)) request_break(v);
    push(EvK::PostureGain, policy::kGainBlocked, p.points(), 3);
    if (!break_pending && !p.latched) note_block();
  }
  std::uint64_t tokens_ = 0;
};
}  // namespace liz
