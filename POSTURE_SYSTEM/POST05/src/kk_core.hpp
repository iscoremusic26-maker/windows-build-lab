// LIGHT FADES TWICE 0.13.0 CHILFOS01: the pure core of the Chilfos (E_KK) duelist. No SDK, no game header: every decision the adapter
// (src/kk_adapter.inc) applies to the native daE_KK_c is taken here, and tests/kk_lab.cpp + tests/kk_world_lab.cpp run this very file.
// Native facts used below (TwilitRealm/dusklight@40457c6, src/d/actor/d_a_e_kk.cpp, read, and the 2.0.3 EXE disassembly):
//   executeAttack mode 3: `(s32)frame == 0x18` -> speedF 80 + field_0x67e = 0 (the AT spheres are registered by cc_set), `>= 0x21` -> field_0x67e = 1.
//   SE: ATTACK03 at frame 24 of BCK 6, ATTACK01 at frame 27 of BCK 5. executeSpearThrow: `(s32)frame == 0x17` -> fopAcM_createChild(E_KK, 0xFF0001).
//   Weapon restore: `(s32)weaponFrame >= 0x22` (a >= test: it can never be skipped). The frame is read with cvttss2si (truncation), once per Execute.
#pragma once
#include <cstdint>

namespace kk {
inline constexpr const char* kVersion = "0.13.0";
inline constexpr const char* kTag = "CHILFOS01";
inline constexpr unsigned kSlots = 8;

namespace policy {
inline constexpr int kNativeHp = 140, kMaxHp = 340, kMaxPosture = 100;
// Posture gains on the Chilfos (user table) and on Link (shared LNK scale for ATP1, the native atp of cc_kk_at_src).
inline constexpr int kGainLinkParry = 13, kGainSwordHp = 5, kGainBlock = 3, kGainSpearBreak = 5, kGainIron = 30, kGainEnemyParrySelf = 0;
inline constexpr int kLinkBlock = 20, kLinkDirect = 30, kLinkEnemyParry = 25;
// Regeneration: 60 eligible ticks without a genuine gain, then points per second by CURRENT hp / 340. Integer units: 1 point = 30000 units,
// so a rate of n points/s is exactly n*1000 units per 30 Hz tick (no drift).
inline constexpr unsigned kRegenDelayTicks = 60;
inline constexpr std::int64_t kUnitsPerPoint = 30000;
inline constexpr unsigned kBreakTicks = 90;     // absolute stagger window
inline constexpr unsigned kRestMin = 45, kRestMax = 75;
inline constexpr unsigned kCounterTtl = 60;
inline constexpr unsigned kParryCooldown = 120;
inline constexpr unsigned kParryLease = 12;      // ticks a prepared parry stays presented for one swing
inline constexpr unsigned kWoundReadyDelay = 6;  // a guard prepared by wounds is up only after this delay (never retroactive)
inline constexpr unsigned kWoundMemory = 90;
inline constexpr unsigned kPressureWindow = 75, kSustainedWindow = 150, kBlockPairWindow = 45;
inline constexpr int kPctA = 12, kPctB = 35, kPctC = 45, kPctTwoBlocks = 80;
inline constexpr int kBlockBase = 30;            // [BALANCE_PROVISIONAL] not in the request: guard roll of a frontal, ready Chilfos with no wound streak
inline constexpr int kWoundPct[3] = {45, 80, 100};
inline constexpr int kFrontalArc = 0x3000;       // 67.5 degrees, the Lizalfos shield arc
inline constexpr int kTargetArc = 0x2000;        // a swing counts for an actor only when aimed at it (45 degrees) ...
inline constexpr float kTargetDist = 320.0f;     // ... and close
inline constexpr float kComboLeaveDist = 600.0f; // native executeAttack walks in beyond 600: a combo whose target left ends at a safe boundary
inline constexpr unsigned kWatchdogTicks = 600;
// Native frames.
inline constexpr int kAtOpenFrame = 24, kAtCloseFrame = 33, kSe01Frame = 27, kThrowSpawnFrame = 23, kWeaponReadyFrame = 0x22;
inline constexpr int kBckAttack01 = 5, kBckAttack03 = 6, kBckBackWalk = 7, kBckDamageBow = 9, kBckDamageBowB = 0xB, kBckDead = 0xE, kBckDrawback = 0xF,
                     kBckRestoreA = 0x12, kBckRestoreB = 0x13, kBckThrow = 0x18, kBckWaitAlert = 0x1A, kWeaponIdle = 0x1D, kWeaponRestore = 0x1E;
inline constexpr int kMinPrep = 8, kBurstPrep = 6;  // frames of native wind-up kept before frame 24 when a clip restarts mid-combo
inline constexpr float kRateD = 1.25f, kRateE = 1.30f, kRateEBurst = 1.50f, kRateDFast = 1.40f, kRateBack = 1.25f, kRateThrow = 1.45f, kRateRestore = 1.60f;
inline constexpr float kBurstMax = 0.40f;  // seconds: an E-E pair recorded closer than this is a burst (both thrusts at x1.50)
}  // namespace policy

// ---------------------------------------------------------------- native action ids (daE_KK_Action)
enum Act : int { kWait = 0, kIcicle = 1, kWalk = 2, kThrow = 3, kBackWalk = 4, kYoroke = 5, kGuard = 6, kDamage = 7, kAttack = 8, kWeaponMove = 9, kDead = 10 };

// ---------------------------------------------------------------- RNG: own generator (cM_rnd is never consumed), unbiased by rejection
struct Rng {
  std::uint64_t s = 0x9E3779B97F4A7C15ull;
  void seed(std::uint64_t v) noexcept {  // splitmix64: neighbouring seeds (tick, id, generation) give unrelated streams
    v += 0x9E3779B97F4A7C15ull;
    v = (v ^ (v >> 30)) * 0xBF58476D1CE4E5B9ull;
    v = (v ^ (v >> 27)) * 0x94D049BB133111EBull;
    v ^= v >> 31;
    s = v ? v : 0x9E3779B97F4A7C15ull;
  }
  std::uint64_t next() noexcept {
    s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
    return s * 0x2545F4914F6CDD1Dull;
  }
  unsigned below(unsigned n) noexcept {
    if (n <= 1) return 0;
    const std::uint64_t lim = (~0ull / n) * n;
    std::uint64_t x;
    do x = next();
    while (x >= lim);
    return unsigned(x % n);
  }
  bool pct(int p) noexcept { return p >= 100 || (p > 0 && int(below(100)) < p); }
};

// ---------------------------------------------------------------- the four combos (type and order from the request, rhythm from the recordings)
enum class K : std::uint8_t { D = 0, E = 1 };  // D diagonal slash, E frontal thrust
inline constexpr unsigned kPatterns = 4, kMaxActs = 7;
struct Pattern {
  unsigned n;
  K act[kMaxActs];
  float onset[kMaxActs];  // measured onsets of the original M4A (seconds, see CHILFOS01/RHYTHM_ANALYSIS.md)
  bool ranged_tail;       // C3: backstep + one real spear throw + fast restore after the last melee action
  float tail_onset;       // C3 final mark (single transient: launch/impact are not separable in the recording)
};
inline constexpr Pattern kPattern[kPatterns] = {
    {6, {K::D, K::D, K::D, K::E, K::E, K::D}, {0.6790f, 1.8637f, 3.0735f, 4.0901f, 4.3504f, 5.2978f}, false, 0.0f},
    {6, {K::D, K::E, K::E, K::E, K::E, K::D}, {0.5882f, 1.5617f, 1.9325f, 2.6237f, 3.0129f, 3.7575f}, false, 0.0f},
    {7, {K::E, K::E, K::E, K::E, K::D, K::E, K::D}, {0.9966f, 1.2100f, 2.6556f, 2.8884f, 4.3105f, 4.9375f, 5.5415f}, true, 6.9487f},
    {7, {K::D, K::D, K::E, K::D, K::E, K::E, K::E}, {0.7304f, 1.8575f, 3.6551f, 4.6174f, 5.9804f, 6.1171f, 7.0501f}, false, 0.0f},
};
inline constexpr unsigned total_actions() noexcept {
  unsigned t = 0;
  for (const auto& p : kPattern) t += p.n + (p.ranged_tail ? 1u : 0u);
  return t;
}

// Which bck is which motion is an IDENTITY question the code cannot settle from names (KK_ATTACK01/03). The default below is the result of
// CHILFOS01/ANIMATION_IDENTITY.md; it is one constant so a human observation can flip it, and the diagnostic line prints both.
inline constexpr int kBckDiagonal = policy::kBckAttack01, kBckThrust = policy::kBckAttack03;
inline constexpr int bck_of(K k) noexcept { return k == K::D ? kBckDiagonal : kBckThrust; }
static_assert(kBckDiagonal != kBckThrust, "D and E must be two different native clips");

inline bool is_burst(const Pattern& p, unsigned i) noexcept {  // an E that is half of an E-E pair recorded closer than kBurstMax
  if (p.act[i] != K::E) return false;
  if (i > 0 && p.act[i - 1] == K::E && p.onset[i] - p.onset[i - 1] < policy::kBurstMax) return true;
  if (i + 1 < p.n && p.act[i + 1] == K::E && p.onset[i + 1] - p.onset[i] < policy::kBurstMax) return true;
  return false;
}

// ---------------------------------------------------------------- the native clip as a sequence of Execute reads
// Tick 0 of a clip is the Execute that sets it (frame = start); every later Execute reads start + k*rate (the morf plays once per Execute).
// The AT opens at the first read >= 24 (when the truncated reads jump over 24, the crossing guarantee below fires it on that same first read
// past 24, never later), and closes at the first read >= 33.
struct Clip {
  float start = 0, rate = 1;
  float at(unsigned k) const noexcept { float f = start; for (unsigned i = 0; i < k; ++i) f += rate; return f; }  // float32 accumulation, as the engine
  unsigned first_at_least(float target, unsigned from = 0) const noexcept {
    float f = at(from);
    for (unsigned k = from; k < 400; ++k, f += rate)
      if (f >= target) return k;
    return 400;
  }
  unsigned open_tick() const noexcept { return first_at_least(float(policy::kAtOpenFrame), 1); }
  unsigned close_tick() const noexcept { return first_at_least(float(policy::kAtCloseFrame), open_tick()); }
};

// ---------------------------------------------------------------- the rhythm plan: recorded -> realizable, per action
struct ActPlan {
  K kind = K::D;
  int bck = 0;
  float start = 0, rate = 1;
  unsigned gap = 0;              // idle ticks after the previous window closed, before this clip starts
  unsigned open = 0;             // combo-relative tick of this AT opening
  unsigned window = 0;           // registered AT ticks
  float recorded_s = 0, achieved_s = 0;  // interval from the previous impact (0 for the first)
  bool burst = false, fast_d = false;
};
struct TailPlan { unsigned back_ticks = 0; unsigned spawn_tick = 0; float recorded_s = 0, achieved_s = 0; bool on = false; };
struct Plan {
  unsigned n = 0;
  ActPlan a[kMaxActs];
  TailPlan tail;
};
inline unsigned ticks_of(float seconds) noexcept { return unsigned(seconds * 30.0f + 0.5f); }

inline Plan make_plan(unsigned pattern) noexcept {
  Plan pl;
  const Pattern& p = kPattern[pattern % kPatterns];
  pl.n = p.n;
  unsigned prev_open = 0, prev_close = 0;
  for (unsigned i = 0; i < p.n; ++i) {
    ActPlan& a = pl.a[i];
    a.kind = p.act[i], a.bck = bck_of(p.act[i]), a.burst = is_burst(p, i);
    a.rate = a.kind == K::D ? policy::kRateD : (a.burst ? policy::kRateEBurst : policy::kRateE);
    if (i == 0) {  // full wind-up, combo tick 0 = its start
      Clip c{0.0f, a.rate};
      a.start = 0, a.gap = 0, a.open = c.open_tick(), a.window = c.close_tick() - c.open_tick();
      prev_open = a.open, prev_close = a.open + a.window;
      continue;
    }
    a.recorded_s = p.onset[i] - p.onset[i - 1];
    const unsigned target = ticks_of(a.recorded_s);
    const int min_prep = a.burst ? policy::kBurstPrep : policy::kMinPrep;
    auto fit = [&](float rate, float& start, unsigned& gap) -> unsigned {  // returns the achieved interval in ticks
      const unsigned closed = prev_close - prev_open;  // the restart happens in the closing Execute at the earliest (native close value written there)
      for (int s = 0; s <= policy::kAtOpenFrame - min_prep; ++s) {
        Clip c{float(s), rate};
        const unsigned need = closed + c.open_tick();
        if (need <= target) {
          start = float(s), gap = target - need;
          return target;
        }
      }
      Clip c{float(policy::kAtOpenFrame - min_prep), rate};
      start = c.start, gap = 0;
      return closed + c.open_tick();
    };
    float st = 0;
    unsigned gp = 0;
    unsigned got = fit(a.rate, st, gp);
    if (a.kind == K::D && got > target) {  // diagonal rapido x1.40 only when x1.25 cannot make it
      float st2 = 0;
      unsigned gp2 = 0;
      const unsigned got2 = fit(policy::kRateDFast, st2, gp2);
      if (got2 < got) a.rate = policy::kRateDFast, a.fast_d = true, st = st2, gp = gp2, got = got2;
    }
    a.start = st, a.gap = gp;
    Clip c{st, a.rate};
    a.open = prev_open + got;
    a.window = c.close_tick() - c.open_tick();
    a.achieved_s = float(got) / 30.0f;
    prev_open = a.open, prev_close = a.open + a.window;
  }
  if (p.ranged_tail) {  // C3: backstep at x1.25, then the throw clip from frame 0 at x1.45; the release frame 23 lands on the recorded mark
    TailPlan& t = pl.tail;
    t.on = true;
    t.recorded_s = p.tail_onset - p.onset[p.n - 1];
    const unsigned target = ticks_of(t.recorded_s);
    const unsigned closed = prev_close - prev_open;
    Clip th{0.0f, policy::kRateThrow};
    const unsigned to_spawn = th.first_at_least(float(policy::kThrowSpawnFrame), 1);
    t.back_ticks = target > closed + to_spawn ? target - closed - to_spawn : 1;
    t.spawn_tick = prev_open + closed + t.back_ticks + to_spawn;
    t.achieved_s = float(t.spawn_tick - prev_open) / 30.0f;
  }
  return pl;
}

// ---------------------------------------------------------------- frame-crossing guarantee for native `(s32)frame == N` events
// last_read: the frame the native read in the previous Execute of this clip (or start - rate). If an event frame lies strictly between the
// truncated previous read and the truncated current one, the native would never see it: the adapter shows the native that exact frame for
// this one call and restores the true frame afterwards. One event per call at most (rates <= 1.6, events >= 3 frames apart).
struct Cross { bool snap = false; float frame = 0; int event = -1; bool duplicate = false; };
inline Cross crossing(float last_read, float now, const int* events, unsigned n, std::uint32_t fired_mask) noexcept {
  Cross c;
  const int a = int(last_read), b = int(now);
  for (unsigned i = 0; i < n; ++i) {
    const int e = events[i];
    if (b == e && (fired_mask >> i & 1u)) c.duplicate = true;  // the native is about to fire it a second time
    if (a < e && e < b && !(fired_mask >> i & 1u)) {
      c.snap = true, c.frame = float(e), c.event = int(i);
      return c;
    }
  }
  return c;
}
inline constexpr int kAttackEvents[2] = {policy::kAtOpenFrame, policy::kSe01Frame};
inline constexpr int kThrowEvents[4] = {9, policy::kThrowSpawnFrame, 0x22, 0x3D};

// ---------------------------------------------------------------- posture of the Chilfos
inline int regen_per_second(int hp) noexcept {
  if (hp <= 0) return 0;
  if (hp * 4 > 3 * policy::kMaxHp) return 12;  // > 75%  (256..340)
  if (hp * 2 > policy::kMaxHp) return 8;        // > 50%  (171..255)
  if (hp * 4 > policy::kMaxHp) return 5;        // > 25%  (86..170)
  if (hp * 10 > policy::kMaxHp) return 2;       // > 10%  (35..85)
  return 1;                                     // 1..34
}
struct Posture {
  std::int64_t u = 0;  // units, 1 point = 30000
  bool latched = false;
  std::uint64_t last_gain = 0;
  std::uint64_t eligible = 0;  // eligible ticks since the last genuine gain
  int points() const noexcept { return int((u + policy::kUnitsPerPoint - 1) / policy::kUnitsPerPoint); }
  // Returns true exactly once: the gain that latches 100.
  bool gain(int pts, std::uint64_t tick) noexcept {
    if (pts <= 0 || latched) return false;
    u += std::int64_t(pts) * policy::kUnitsPerPoint;
    last_gain = tick, eligible = 0;
    if (u >= std::int64_t(policy::kMaxPosture) * policy::kUnitsPerPoint) {
      u = std::int64_t(policy::kMaxPosture) * policy::kUnitsPerPoint;
      latched = true;
      return true;
    }
    return false;
  }
  // One eligible tick. Never while latched/broken, never with hp 0, never in an excluded native reaction (the caller passes eligible=false).
  void tick(int hp, bool eligible_now) noexcept {
    if (latched || u <= 0 || hp <= 0 || !eligible_now) return;
    if (++eligible <= policy::kRegenDelayTicks) return;
    u -= std::int64_t(regen_per_second(hp)) * 1000;
    if (u < 0) u = 0;
  }
  void reset() noexcept { u = 0, latched = false, eligible = 0; }
};

// ---------------------------------------------------------------- adaptive defense (Lizalfos FIX05 semantics, re-derived for E_KK)
enum class Np : std::uint8_t { None = 0, NoSpear, Attacking, Flank, Cooldown, Rng, Broken, Count };
struct Defense {
  static constexpr unsigned kHist = 8;
  std::uint64_t swings[kHist] = {};
  unsigned nsw = 0;
  unsigned hits_since_defense = 0;  // own damage taken since its last defense
  unsigned wound_streak = 0;
  std::uint64_t last_wound = 0, cooldown_until = 0;
  std::uint64_t block_ticks[2] = {}, block_serials[2] = {};
  unsigned nblocks = 0;
  // prepared parry for ONE swing
  bool prepared = false;
  std::uint64_t prepared_serial = 0, prepared_until = 0;
  // counters
  std::uint64_t rolls[4] = {}, prepares[4] = {};  // A,B,C,two-blocks
  void swing(std::uint64_t t) noexcept { swings[nsw++ % kHist] = t; }
  unsigned within(std::uint64_t now, unsigned w) const noexcept {
    unsigned c = 0;
    for (unsigned i = 0; i < kHist && i < nsw; ++i)
      if (swings[i] && now - swings[i] <= w) ++c;
    return c;
  }
  int level(std::uint64_t now) const noexcept {  // 0=A 1=B 2=C
    if (within(now, policy::kSustainedWindow) >= 6 && hits_since_defense >= 2) return 2;
    if (within(now, policy::kPressureWindow) >= 3) return 1;
    return 0;
  }
  bool two_blocks(std::uint64_t now) const noexcept {
    return nblocks >= 2 && block_serials[0] != block_serials[1] && now - block_ticks[(nblocks - 1) % 2] <= policy::kBlockPairWindow;
  }
  void on_block(std::uint64_t now, std::uint64_t serial) noexcept {
    block_ticks[nblocks % 2] = now, block_serials[nblocks % 2] = serial, ++nblocks;
    hits_since_defense = 0, wound_streak = 0;
  }
  void on_wound(std::uint64_t now) noexcept {
    if (now - last_wound > policy::kWoundMemory) wound_streak = 0;
    ++wound_streak, ++hits_since_defense, last_wound = now;
  }
  void on_enemy_parry(std::uint64_t now) noexcept {
    cooldown_until = now + policy::kParryCooldown, prepared = false, nblocks = 0, hits_since_defense = 0, wound_streak = 0;
  }
  // A new real swing of Link aimed at this actor: at most one roll. Returns the reason it was not prepared.
  Np on_swing(std::uint64_t now, std::uint64_t serial, bool spear, bool attacking, bool frontal, bool broken, Rng& r) noexcept {
    swing(now);
    if (broken) return Np::Broken;
    if (!spear) return Np::NoSpear;
    if (attacking) return Np::Attacking;
    if (!frontal) return Np::Flank;
    if (now < cooldown_until) return Np::Cooldown;
    const bool pair = two_blocks(now);
    const int lv = level(now);
    const int pct = pair ? policy::kPctTwoBlocks : lv == 2 ? policy::kPctC : lv == 1 ? policy::kPctB : policy::kPctA;
    const unsigned slot = pair ? 3u : unsigned(lv);
    ++rolls[slot];
    if (pair) nblocks = 0;  // the pair is consumed by this continuation, whatever the roll says
    if (!r.pct(pct)) return Np::Rng;
    ++prepares[slot];
    prepared = true, prepared_serial = serial, prepared_until = now + policy::kParryLease;
    return Np::None;
  }
  bool presenting(std::uint64_t now, std::uint64_t serial) const noexcept { return prepared && prepared_serial == serial && now <= prepared_until; }
  void expire(std::uint64_t now) noexcept {
    if (prepared && now > prepared_until) prepared = false;
  }
  // Block roll for a swing that was NOT prepared as a parry: the guard is a native reaction, the wound streak prepares it (never retroactively).
  int block_pct(std::uint64_t now) const noexcept {
    if (wound_streak == 0 || now - last_wound > policy::kWoundMemory) return policy::kBlockBase;
    if (now - last_wound < policy::kWoundReadyDelay) return policy::kBlockBase;  // not up yet
    return policy::kWoundPct[wound_streak >= 3 ? 2 : wound_streak - 1];
  }
};

// ---------------------------------------------------------------- one verdict per swing (sword of Link on the Chilfos)
enum class V : std::uint8_t { None = 0, EnemyParry, Block, Damage, Decisive, Ignored, Count };
struct SwingVerdict {
  std::uint64_t serial = 0;
  V v = V::None;
  bool rebound_seen = false;
  void set(std::uint64_t s, V x) noexcept { serial = s, v = x, rebound_seen = false; }
  V of(std::uint64_t s) const noexcept { return s == serial ? v : V::None; }
};

// ---------------------------------------------------------------- the stagger at 100 and its two finishers
enum class Bk : std::uint8_t { None = 0, Pending, Broken, Dead };
struct Break {
  Bk st = Bk::None;
  std::uint64_t start = 0, latch_serial = 0;
  bool shield_prep = false, decisive = false;
  unsigned shield_preps = 0;
  bool active() const noexcept { return st == Bk::Broken; }
  void latch(std::uint64_t serial) noexcept {
    if (st == Bk::None) st = Bk::Pending, latch_serial = serial;
  }
  void enter(std::uint64_t now) noexcept { st = Bk::Broken, start = now, shield_prep = false, decisive = false; }
  bool expired(std::uint64_t now) const noexcept { return st == Bk::Broken && now - start >= policy::kBreakTicks; }
  // A sword contact during the stagger finishes it only when it belongs to a swing AFTER the one that raised posture to 100.
  bool sword_finishes(std::uint64_t serial) const noexcept { return st == Bk::Broken && serial != latch_serial && serial > latch_serial; }
  void shield_attack() noexcept {
    if (st == Bk::Broken) shield_prep = true, ++shield_preps;
  }
  bool helm_finishes(bool helm_cut) const noexcept { return st == Bk::Broken && shield_prep && helm_cut; }
  void reset() noexcept { *this = Break{}; }
};

// ---------------------------------------------------------------- classification of an E_KK
enum class Excl : std::uint8_t { None = 0, Profile, Spear, HiddenIronBall, UnknownSubtype, Count };
struct IdIn { bool profile_ok = false; int sub = 0; bool hidden = false; };
inline Excl classify(const IdIn& in) noexcept {
  if (!in.profile_ok) return Excl::Profile;
  if (in.sub == 1) return Excl::Spear;  // field_0x679 == 1: the thrown spear child, its own physics and lifetime
  if (in.sub != 0 && in.sub != 2 && in.sub != 3) return Excl::UnknownSubtype;
  if (in.sub == 2 && in.hidden) return Excl::HiddenIronBall;  // not yet in the fight (native execute returns before action)
  return Excl::None;
}
inline const char* name(Excl e) noexcept {
  static constexpr const char* n[] = {"NONE", "PROFILE", "SPEAR_CHILD", "IRON_BALL_HIDDEN", "UNKNOWN_SUBTYPE"};
  return unsigned(e) < unsigned(Excl::Count) ? n[unsigned(e)] : "?";
}
inline const char* name(V v) noexcept {
  static constexpr const char* n[] = {"NONE", "ENEMY_PARRY", "BLOCK", "DAMAGE", "DECISIVE", "IGNORED"};
  return unsigned(v) < unsigned(V::Count) ? n[unsigned(v)] : "?";
}
inline const char* name(Np v) noexcept {
  static constexpr const char* n[] = {"PREPARED", "NO_SPEAR", "ATTACKING", "FLANK", "COOLDOWN", "RNG", "BROKEN"};
  return unsigned(v) < unsigned(Np::Count) ? n[unsigned(v)] : "?";
}
enum class End : std::uint8_t { Completed = 0, Posture100, Death, NativePriority, Defense, LinkLeft, SceneOrDelete, Event, Watchdog, Count };
inline const char* name(End e) noexcept {
  static constexpr const char* n[] = {"COMPLETED", "POSTURE_100_BREAK", "DEATH", "NATIVE_PRIORITY", "ADAPTIVE_DEFENSE_AT_SAFE_BOUNDARY", "LINK_LEFT", "SCENE_OR_DELETE", "EVENT", "WATCHDOG"};
  return unsigned(e) < unsigned(End::Count) ? n[unsigned(e)] : "?";
}
inline int angle_diff(int a, int b) noexcept {
  const int d = int(std::int16_t(std::uint16_t(a - b)));
  return d < 0 ? -d : d;
}

// ---------------------------------------------------------------- D/E identity from the engine's own pose (no asset is read)
// The spear tip (field_0x68c, computed by mtx_set from weapon joint 2) in the Chilfos' own frame (origin current.pos, yaw shape_angle.y),
// sampled before frame 24 and inside the AT window of the SAME clip: a thrust moves forward, a diagonal sweeps sideways and down.
enum class Tip : std::uint8_t { Unsure = 0, Thrust, Diagonal, Count };
inline const char* name(Tip t) noexcept {
  static constexpr const char* n[] = {"UNSURE", "THRUST", "DIAGONAL"};
  return unsigned(t) < unsigned(Tip::Count) ? n[unsigned(t)] : "?";
}
inline float fsqrt(float x) noexcept {  // Newton, enough for a ratio test (no <cmath> dependency in the core)
  if (x <= 0.0f) return 0.0f;
  float r = x > 1.0f ? x : 1.0f;
  for (int i = 0; i < 24; ++i) r = 0.5f * (r + x / r);
  return r;
}
inline Tip classify_tip(float dx, float dy, float dz) noexcept {
  const float lv = fsqrt(dx * dx + dy * dy), fz = dz < 0 ? -dz : dz;
  if (dz > 30.0f && dz > 1.5f * lv) return Tip::Thrust;
  if (lv > 30.0f && lv > 1.2f * fz) return Tip::Diagonal;
  return Tip::Unsure;
}
inline void local_of(float wx, float wz, int yaw, float& lx, float& lz) noexcept {  // rotate a world XZ offset into the actor frame (s16 yaw)
  // sin/cos of a 16-bit angle by a 9th-order Taylor on [-pi/2, pi/2] with symmetry: |error| < 1e-5, enough for a ratio test.
  auto s16sin = [](int ang) noexcept {
    float x = float(std::int16_t(std::uint16_t(ang))) * (3.14159265f / 32768.0f);
    if (x > 1.5707963f) x = 3.14159265f - x;
    if (x < -1.5707963f) x = -3.14159265f - x;
    const float x2 = x * x;
    return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f * (1.0f - x2 / 42.0f * (1.0f - x2 / 72.0f))));
  };
  const float s = s16sin(yaw), c = s16sin(yaw + 0x4000);
  lx = wx * c - wz * s;  // right
  lz = wx * s + wz * c;  // forward (engine: forward of yaw y is (sin y, cos y))
}
struct Identity {
  unsigned votes[2][unsigned(Tip::Count)] = {};  // [bck5, bck6]
  bool verified = false, swapped = false, conflict = false;
  int diagonal() const noexcept { return swapped ? kBckThrust : kBckDiagonal; }
  int thrust() const noexcept { return swapped ? kBckDiagonal : kBckThrust; }
  int bck(K k) const noexcept { return k == K::D ? diagonal() : thrust(); }
  static Tip lead(const unsigned* v) noexcept {
    const unsigned t = v[unsigned(Tip::Thrust)], d = v[unsigned(Tip::Diagonal)];
    if (t >= 2 && t > 2 * d) return Tip::Thrust;
    if (d >= 2 && d > 2 * t) return Tip::Diagonal;
    return Tip::Unsure;
  }
  // Returns true when this observation settled (or changed) the identity.
  bool observe(int bck, Tip t) noexcept {
    if (bck != policy::kBckAttack01 && bck != policy::kBckAttack03) return false;
    ++votes[bck == policy::kBckAttack01 ? 0 : 1][unsigned(t)];
    if (verified) return false;
    const Tip a = lead(votes[0]), b = lead(votes[1]);
    if (a == Tip::Unsure || b == Tip::Unsure) return false;
    if (a == b) {
      conflict = true;
      return false;
    }
    verified = true;
    swapped = (a == Tip::Thrust);  // bck 5 measured as the thrust: the default mapping was inverted
    return true;
  }
};

// ---------------------------------------------------------------- per-actor statistics (summary and labs)
#define KK_STATS_SCALARS(X)                                                                                                                         \
  X(actions) X(impacts_open) X(impacts_closed) X(link_parry) X(link_block) X(link_direct_offered) X(sword_damage) X(blocks) X(enemy_parries)          \
  X(downgraded_parries) X(ignored) X(spear_breaks) X(snaps) X(duplicates) X(gaps_skipped) X(poise) X(rest) X(counters) X(counter_expired) X(throws)    \
  X(spawns) X(spawn_dupes) X(restores) X(backsteps) X(early_throw) X(breaks) X(shield_preps) X(decisive_sword) X(decisive_helm) X(recovered)          \
  X(native_deaths) X(fallback_deaths) X(headlock_stripped) X(watchdog) X(regen_ticks)
struct Stats {
  std::uint64_t selected[kPatterns] = {}, completed[kPatterns] = {}, ended[unsigned(End::Count)] = {};
  std::uint64_t verdict[unsigned(V::Count)] = {}, no_parry[unsigned(Np::Count)] = {};
#define KK_STAT_FIELD(f) std::uint64_t f = 0;
  KK_STATS_SCALARS(KK_STAT_FIELD)
#undef KK_STAT_FIELD
  void add(const Stats& x) noexcept {
    for (unsigned i = 0; i < kPatterns; ++i) selected[i] += x.selected[i], completed[i] += x.completed[i];
    for (unsigned i = 0; i < unsigned(End::Count); ++i) ended[i] += x.ended[i];
    for (unsigned i = 0; i < unsigned(V::Count); ++i) verdict[i] += x.verdict[i];
    for (unsigned i = 0; i < unsigned(Np::Count); ++i) no_parry[i] += x.no_parry[i];
#define KK_STAT_ADD(f) f += x.f;
    KK_STATS_SCALARS(KK_STAT_ADD)
#undef KK_STAT_ADD
  }
};
}  // namespace kk
