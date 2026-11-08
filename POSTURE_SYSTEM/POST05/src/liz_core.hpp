#pragma once
// LIZALFOS DUELIST 0.12.0: complete per-instance combat brain for the ordinary Lizalfos (native E_DN, e_dn_class).
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
#include <cstdint>
#include "parry_core.hpp"

namespace liz {
inline constexpr const char* kVersion = "0.12.0";
inline constexpr unsigned kSlots = 16;

namespace native {
inline constexpr std::int16_t kNormal = 0, kFightRun = 3, kAttack0 = 4, kAttack = 5, kTailAttack = 6, kGuard = 7, kSDamage = 20, kDamage = 21, kReg = 60;
inline constexpr int kAnmN = 4, kAnmD = 5, kAnmC = 8;
inline constexpr float kGuardRange = 300.0f;  // l_HIO.battle_init_range
inline constexpr std::int16_t kNativeHp = 100;
}  // namespace native

namespace policy {
inline constexpr std::int16_t kMaxHp = 370;
inline constexpr int kMilli = 1000, kMaxPosture = 100, kMaxMilli = kMaxPosture * kMilli;
inline constexpr int kGainSword = 15, kGainLinkParry = 22, kGainIronBall = 30, kGainBlocked = 5, kGainLinkOnEnemyParry = 25;
inline constexpr std::uint32_t kRegenDelayTicks = 60;
inline constexpr int kTicksPerSecond = 30;
// Link's OWN posture when he blocks / takes a Lizalfos hit: the existing LNK07 table rows by native AtAtp (N/D ATP1 -> Blue row, tail ATP2 -> Red row).
inline constexpr int kLinkBlockAtp1 = 20, kLinkDirectAtp1 = 30, kLinkBlockAtp2 = 25, kLinkDirectAtp2 = 35;
// Adaptive defense
inline constexpr int kParryPctA = 12, kParryPctB = 35, kParryPctC = 45;
inline constexpr std::uint32_t kPressureWindow = 75;       // three distinct swings in ~2.5 s
inline constexpr std::uint32_t kSustainedWindow = 150;     // six distinct swings in 5 s
inline constexpr unsigned kSustainedSwings = 6;
inline constexpr unsigned kUnusedOpportunities = 2;        // Lizalfos impacts Link neither blocked nor parried since his last defense
inline constexpr std::uint32_t kDemoteTicks = 45;
inline constexpr float kResetDistance = 600.0f;
inline constexpr std::uint32_t kResetFarTicks = 30;
inline constexpr std::uint32_t kPendingSwingTicks = 6;     // a swing still counts at the next safe boundary for this long, then it is gone
// Measured on the native order: entry in FIGHT_RUN POST (tick T), e_dn_guard places tate_sph at T+1, its collision pass is accepted at T+2 => delta 3 is the first physically possible contact.
inline constexpr std::uint32_t kParryWindowFirst = 3, kParryWindowLast = 7;  // 5 simulation ticks from the first real tate_sph pass; real contact only
inline constexpr std::uint32_t kParryCooldownTicks = 120;
inline constexpr std::int16_t kFrontHalfAngle = 0x3000;    // parry only inside +-67.5 deg of the actor's facing
inline constexpr std::int16_t kWayHalfAngle = 0x4000;      // native way_check front half-plane
inline constexpr std::int16_t kSwingAimHalfAngle = 0x2000; // Link's swing counts for THIS actor only when aimed at it (+-45 deg)
inline constexpr float kSwingRange = 320.0f;
// Initiative after a real enemy parry
inline constexpr std::uint32_t kInitiativeTicks = 45;
inline constexpr int kInitiativeShortPct = 40, kInitiativeComboPct = 40;  // remainder: reposition (native AI only)
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
// Link's fast re-arm only between D1 and D2 (the clip gives 6..11 ticks; CP04's 12 tick slip rule cannot fit a second press there).
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

inline constexpr unsigned kPatterns = 4, kCounterPattern = 4, kMaxActs = 6;
struct Pattern {
  unsigned acts, impacts;
  Act a[kMaxActs];
  float gap_s[kMaxActs];  // [i]: recorded seconds from the LAST impact of action i to the first impact of action i+1
};
// Composition, action rates and intervals fixed by the user's confirmed impact table. D's internal pair is not a schedule value: it is the clip.
inline constexpr Pattern kPattern[kPatterns + 1] = {
    {3, 4, {Act::N, Act::D, Act::CFast}, {0.50f, 0.63f}},
    {6, 7, {Act::N, Act::N, Act::N, Act::D, Act::N, Act::CFast}, {0.58f, 0.60f, 0.54f, 0.66f, 0.67f}},
    {6, 9, {Act::D, Act::N, Act::D, Act::N, Act::N, Act::D}, {0.52f, 0.53f, 0.54f, 0.61f, 0.52f}},
    {5, 6, {Act::CSlow, Act::CSlow, Act::N, Act::D, Act::CFast}, {1.34f, 1.26f, 0.52f, 0.63f}},
    {1, 1, {Act::N}, {}},  // short counter after a real enemy parry: NOT a fifth combo, never part of the 25% draw
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
  if (i == 0) return 0.0f;
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
  unsigned unused = 0;
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
  bool cond_c(std::uint64_t now) const noexcept { return swings_within(now, policy::kSustainedWindow) >= policy::kSustainedSwings && unused >= policy::kUnusedOpportunities; }
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
  void on_link_parry() noexcept {  // Link changed strategy: drop one level now, forget the pressure history
    if (level > 0) {
      --level;
      ++demotions;
    }
    unused = 0;
    clear();
  }
  void on_link_defended() noexcept { unused = 0; }
  void on_link_undefended() noexcept {
    if (unused < 16) ++unused;
  }
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
  std::int16_t action = 0, mode = 0, hp = 0, max_hp = 0, invuln = 0, timer2 = 0, ut2 = 0, search_y = 0, shape_y = 0, cur_y = 0, fc = 0;
  std::int8_t at_chk = 0, tail = 0;
  int anm_no = 0;
  float frame = 0, rate = 0, end = 0, pl_dir = 0, recoil = 0;
  bool event = false, link_ok = false, link_human = false, horse = false, ground = true;
  float at_c[3] = {0, 0, 0}, at_r = 0;
  bool at_hit = false, at_on_link = false, at_shield = false;
  bool tate_hit = false, tate_link_sword = false;
};
struct LinkFrame {
  std::uint64_t swing_serial = 0;  // +1 at each distinct swing start
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
};
enum class EvK : std::uint8_t {
  None = 0, Admitted, Excluded, HpSet, ComboStart, ComboEnd, ActionStart, ImpactOpen, WindowClose, Contact, LinkGain, LinkBlock, LinkDirectOffer, Roll, GuardEntry,
  Block, EnemyParry, Initiative, Break, Rearm, CancelSuppressed, Critical, Coverage, Recovered, PostureGain, Count
};
struct Ev {
  EvK k = EvK::None;
  int a = 0, b = 0, c = 0;
  std::uint64_t t = 0;
  float f = 0, g = 0;
};
struct Stats {
  std::uint64_t selected[kPatterns] = {}, completed[kPatterns] = {}, impacts_open[kPatterns + 1] = {}, ended[unsigned(End::Count)] = {};
  std::uint64_t contact[unsigned(Hit::Count)] = {}, link_parry = 0, d_pair_parry = 0, sword_hits = 0, iron_hits = 0, blocks = 0, enemy_parries = 0, cancels_suppressed = 0;
  std::uint64_t stagger_suppressed_small = 0, stagger_suppressed_big = 0, shield_bash_cleared = 0, guards_native = 0, guards_attempt = 0, guards_boundary = 0;
  std::uint64_t counter_short = 0, initiative = 0, reposition = 0, watchdog = 0, sample_gap = 0, armed_ticks = 0, windows = 0, slide = 0;
};

class Brain {
 public:
  static constexpr unsigned kEv = 12;
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
  bool in_guard = false;
  // initiative
  enum class Init : std::uint8_t { None, Short, Combo } init = Init::None;
  std::uint64_t init_until = 0;
  bool rest_pending = false;
  // break
  bool break_pending = false, in_break = false;
  std::uint64_t break_tick = 0;
  // damage_check call
  std::int16_t hp_pre = 0;
  bool credited_call = false, skipped_big = false, skipped_small = false;
  std::int16_t fc_pre = 0, ut2_pre = 0;
  bool have_at_prev = false;
  float at_prev[3] = {0, 0, 0};
  float max_step = 0;
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
  static bool facing(std::int16_t shape_y, std::int16_t search_y, std::int16_t half) noexcept { return iabs16(angle_diff(shape_y, search_y)) <= half; }
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
    if (v.event) {
      if (ep.active) end_episode(End::Event);
      attempt_armed = false;
      armed_last = armed_now = false;
      return w;
    }
    if (dead || v.hp <= 0) return w;
    // Link's swing start: serial is global; the memory of THIS actor only takes swings aimed at it and in range.
    if (L.swing_serial != last_swing) {
      last_swing = L.swing_serial;
      if (v.link_human && v.pl_dir < policy::kSwingRange && swing_aimed(L.shape_y, v.search_y)) {
        def.stamp(local);
        pending_swing_tick = local;
        pending_swing_serial = L.swing_serial;
      }
    }
    def.update(local, v.pl_dir);
    // at_sph contact of the last collision pass
    HitIn h;
    h.at_hit = v.at_hit, h.on_link = v.at_on_link, h.shield = v.at_shield, h.armed_last = armed_last, h.event = v.event, h.link_ok = v.link_ok && v.link_human, h.latched = p.latched;
    h.impact = cur_impact, h.resolved = resolved_impact, h.pass = L.pass;
    const Hit r = judge_hit(h);
    armed_last = false;
    if (r != Hit::None) {
      ++st.contact[unsigned(r)];
      push(EvK::Contact, int(r), int(ep.pattern), int(ep.impact), L.pass.frame - L.pass.start, v.frame);
      const int atp = (ep.active && is_tail(kPattern[ep.pattern].a[ep.act])) ? 2 : 1;
      if (r == Hit::Parry || r == Hit::ParryLatched) {
        resolved_impact = cur_impact;
        ++st.link_parry;
        def.on_link_parry();
        if (ep.active && kPattern[ep.pattern].a[ep.act] == Act::D && ep.window == 1) ++st.d_pair_parry;
        if (r == Hit::Parry && p.add(policy::kGainLinkParry)) request_break(v);
        if (r == Hit::Parry) push(EvK::PostureGain, policy::kGainLinkParry, p.points(), 0);
      } else if (r == Hit::Guard || r == Hit::MissedEarly) {
        resolved_impact = cur_impact;
        def.on_link_defended();
        push(EvK::LinkBlock, atp == 2 ? policy::kLinkBlockAtp2 : policy::kLinkBlockAtp1, atp);
      } else if (r == Hit::Direct) {
        resolved_impact = cur_impact;
        def.on_link_undefended();
        push(EvK::LinkDirectOffer, atp == 2 ? policy::kLinkDirectAtp2 : policy::kLinkDirectAtp1, atp);
      }
    }
    if (break_pending) deliver_break(v, w);
    return w;
  }
  // ---- damage_check PRE
  Write on_damage_pre(const View& v, bool tg_shield_attack) noexcept {
    Write w;
    credited_call = skipped_big = skipped_small = false;
    hp_pre = v.hp;
    if (!admitted || v.invuln != 0 || v.event || v.horse || dead) return w;
    if (tg_shield_attack && !p.latched) {  // REGLA ABSOLUTA: Link's Shield Attack on the body does not stagger or cancel below 100 (native DRAWBACK branch)
      w.any = true, w.clear_tg = true;
      ++st.shield_bash_cleared;
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
    if (skipped_big) {
      w.any = true, w.set_invuln = true, w.invuln = 10;  // the value of an ordinary hit: never the 1000 big_damage's caller wrote
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
    if (init != Init::None) {
      if (local > init_until) {
        init = Init::None;
      } else if (v.timer2 != 0) {
        w.any = true, w.set_timer2 = true, w.timer2 = 0;  // the native attack decision is evaluated now; it may still refuse (distance, way, height)
      }
      rest_pending = false;
    } else if (rest_pending) {
      rest_pending = false;  // an opening for Link after every episode: the same 1..2 s scale the native timer[2] uses between attacks
      w.any = true, w.set_timer2 = true, w.timer2 = static_cast<std::int16_t>(policy::kRestMin + static_cast<int>(rng.below(policy::kRestSpan)));
    }
    return false;
  }
  // ---- e_dn_fight_run POST: native attack decision -> combo; swing opportunity -> parry attempt
  Write on_run_post(const View& v, bool jump_cut) noexcept {
    Write w;
    if (!admitted || v.event || dead || v.hp <= 0 || p.latched) return w;
    if (v.action == native::kAttack0 || v.action == native::kAttack || v.action == native::kTailAttack) {
      unsigned pat;
      if (init == Init::Short) {
        pat = kCounterPattern;
        ++st.counter_short;
      } else {
        pat = rng.below(kPatterns);  // exactly one draw per episode start, exact 25% each
        ++st.selected[pat];
      }
      init = Init::None;
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
        guard_entry_write(v, w, jump_cut);
        ++st.guards_attempt;
      }
    }
    return w;
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
        push(EvK::ImpactOpen, int(ep.pattern), int(ep.impact), int(a), v.frame, v.rate);
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
  // ---- at_hit_check POST inside ACTION_ATTACK: true = return NULL (the native cancellation can never run; Tg/Link/hitstop are not touched)
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
    frame = ep.pattern == kCounterPattern ? phase_for(a, 0.0f) : start_frame(ep.pattern, ep.act);
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
    }
    const bool native_recoil = v.action == native::kDamage;
    p.regen(!dead && !p.latched && !native_recoil, v.hp, policy::kMaxHp);
    if (in_break) {
      if (dead) {
        in_break = false;
      } else if (v.action != native::kDamage) {
        in_break = false;
        p.rearm();
        push(EvK::Rearm, p.points(), int(v.action), int(v.mode));
      } else if (local - break_tick > policy::kBreakWatchdog) {
        push(EvK::Critical, 2, int(v.mode), v.hp);
        break_tick = local;
      }
    }
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
    if ((v.action == native::kAttack0 || v.action == native::kAttack || v.action == native::kTailAttack) && v.rate == 0.0f) {
      // No mode of the three attacks ever stops the clip once the shield cancellation is gone. Fail-safe, logged as critical if it ever fires.
      w.any = true, w.set_rate = true, w.rate = ep.active ? rate_of(kPattern[ep.pattern].a[ep.act]) : 1.0f;
      ++st.watchdog;
      push(EvK::Critical, 4, int(v.action), int(v.mode));
    }
    return w;
  }
  bool swing_pending() const noexcept {
    return pending_swing_serial != 0 && pending_swing_serial != rolled_serial && local - pending_swing_tick <= policy::kPendingSwingTicks;
  }
  bool double_gap() const noexcept {  // D1 closed, D2 not yet: the only span where the fast shield re-arm may be used
    return ep.active && kPattern[ep.pattern].a[ep.act] == Act::D && ep.impact > 0 && ep.window == 0 && !ep.window_on && !ep.act_done;
  }
  void on_delete() noexcept {
    if (ep.active) end_episode(End::SceneOrDelete);
  }
  void end_episode(End e) noexcept {
    if (!ep.active) return;
    if (e == End::Completed && ep.pattern < kPatterns) ++st.completed[ep.pattern];
    ++st.ended[unsigned(e)];
    push(EvK::ComboEnd, int(ep.pattern), int(e), int(ep.impact));
    ep = Episode{};
    cur_impact = 0;
    rest_pending = true;
  }
  static float root_approx(float x) noexcept {  // no CRT import; 8 Newton steps are plenty for a diagnostic
    if (!(x > 0.0f)) return 0.0f;
    float g = x > 1.0f ? x * 0.5f : 1.0f;
    for (int i = 0; i < 8; ++i) g = 0.5f * (g + x / g);
    return g;
  }

 private:
  bool reaction(const View& v, HitKind kind, bool big) noexcept {
    if (!admitted || v.event || v.horse || dead) return false;
    const bool lethal = v.hp <= 0;
    if (kind == HitKind::Sword) {
      if (lethal) return false;  // native death, always
      if (!credited_call) {
        credited_call = true;
        ++st.sword_hits;
        if (v.hp < hp_pre) {
          if (p.add(policy::kGainSword)) request_break(v);
          push(EvK::PostureGain, policy::kGainSword, p.points(), 1);
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
      if (crossed) {
        if (big) {  // the native knock-down IS the break
          in_break = true;
          break_tick = local;
          push(EvK::Break, p.points(), v.hp, 1);
          if (ep.active) end_episode(End::Posture100);
          return false;
        }
        request_break(v);
        skipped_small = true;
        return true;  // the small reaction is replaced by the real break in damage_check POST
      }
    }
    return false;  // heavy/bomb/arrow/other: native reaction kept (documented native exceptions)
  }
  void request_break(const View& v) noexcept {
    (void)v;
    break_pending = true;
  }
  static bool breakable(std::int16_t a) noexcept {  // combat states only: water, falls, wolf bite, jumps, bomb kicks and scripted paths are never overwritten
    return a == native::kNormal || a == native::kFightRun || a == native::kAttack0 || a == native::kAttack || a == native::kTailAttack || a == native::kGuard || a == 9 || a == 10 || a == 12 || a == native::kSDamage;
  }
  void deliver_break(const View& v, Write& w) noexcept {
    if (v.hp <= 0) {
      break_pending = false;
      return;
    }
    if (v.action == native::kDamage) {  // already in the native knock-down: that one IS the break, no second big_damage
      break_pending = false;
      if (ep.active) end_episode(End::Posture100);
      in_break = true;
      break_tick = local;
      push(EvK::Break, p.points(), v.hp, 1);
      return;
    }
    if (!breakable(v.action)) return;  // stays pending and latched until the native state ends; the actor keeps running native code meanwhile
    break_pending = false;
    if (ep.active) end_episode(End::Posture100);
    attempt_armed = false;
    init = Init::None;
    in_break = true;
    break_tick = local;
    w.any = true, w.big_damage = true, w.set_invuln = false;
    push(EvK::Break, p.points(), v.hp, 0);
  }
  void start_episode(unsigned pat) noexcept {
    ep = Episode{};
    ep.active = true, ep.pattern = static_cast<std::uint8_t>(pat), ep.anm_pending = true;
    ep.start_tick = ep.act_tick = local;
    ep.token = ++tokens_;
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
    // Voluntary defense: only here (AT closed, between actions, never inside the D pair) and only for a real pending swing.
    if (swing_pending() && eligible_boundary(v) && boundary_allowed(ep.pattern, next, def.level, ep.voluntary)) {
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
  void resolve_guard_contact(const View& v, const LinkFrame& L, Write& w) noexcept {
    if (contact_swing == L.swing_serial && L.swing_serial != 0) return;  // one credit per swing, whatever the sword re-touches
    contact_swing = L.swing_serial;
    const bool front = facing(v.shape_y, v.search_y, policy::kFrontHalfAngle);
    // local was already advanced for this Execute: the guard started at guard_tick and this contact came from the previous collision pass.
    if (parry_contact(attempt_armed, guard_tick, local, front, def.cooled(local))) {
      attempt_armed = false;
      def.cooldown_until = local + policy::kParryCooldownTicks;
      ++st.enemy_parries;
      push(EvK::EnemyParry, policy::kGainLinkOnEnemyParry, def.level, static_cast<int>(local - guard_tick));
      push(EvK::LinkGain, policy::kGainLinkOnEnemyParry);
      // Initiative: a chance, not an attack. The guard leaves through its native exit (mode 8), the native decision still decides if there is an opening.
      const std::uint32_t c = rng.below(100);
      init = c < static_cast<std::uint32_t>(policy::kInitiativeShortPct) ? Init::Short
             : c < static_cast<std::uint32_t>(policy::kInitiativeShortPct + policy::kInitiativeComboPct) ? Init::Combo : Init::None;
      if (init == Init::None) ++st.reposition; else ++st.initiative;
      init_until = local + policy::kInitiativeTicks;
      push(EvK::Initiative, int(init), 0, 0);
      if (v.mode >= 5 && v.mode <= 7) {
        w.any = true, w.set_mode = true, w.mode = 8, w.set_rate = true, w.rate = 1.0f;
      }
      return;
    }
    attempt_armed = false;
    ++st.blocks;
    push(EvK::Block, policy::kGainBlocked, p.points(), 0);
    if (p.add(policy::kGainBlocked)) request_break(v);
    push(EvK::PostureGain, policy::kGainBlocked, p.points(), 3);
  }
  std::uint64_t tokens_ = 0;
};
}  // namespace liz
