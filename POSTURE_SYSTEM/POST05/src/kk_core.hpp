// LIGHT FADES TWICE 0.13.6 CHILFOS FIX06: the pure core of the Chilfos (E_KK) duelist. No SDK, no game header: every decision the adapter
// (src/kk_adapter.inc) applies to the native daE_KK_c is taken here, and tests/kk_lab.cpp + tests/kk_world_lab.cpp run this very file.
// Native facts used below (TwilitRealm/dusklight@40457c6, src/d/actor/d_a_e_kk.cpp, read, and the 2.0.3 EXE disassembly):
//   executeAttack mode 3: `(s32)frame == 0x18` -> speedF 80 + field_0x67e = 0 (the AT spheres are registered by cc_set), `>= 0x21` -> field_0x67e = 1.
//   SE: ATTACK03 at frame 24 of BCK 6, ATTACK01 at frame 27 of BCK 5. executeSpearThrow: `(s32)frame == 0x17` -> fopAcM_createChild(E_KK, 0xFF0001).
//   Weapon restore: `(s32)weaponFrame >= 0x22` (a >= test: it can never be skipped). The frame is read with cvttss2si (truncation), once per Execute.
#pragma once
#include <cstdint>

namespace kk {
inline constexpr const char* kVersion = "0.13.6";
inline constexpr const char* kTag = "CHILFOS_FIX06";
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
// FIX02: the counter is dispatched by this module (the native attack entry) kCounterDelay ticks after the confirmed parry, instead of waiting for the
// native guard clip to end and the native AI to pick an attack (0.13.1: 9 of 11 opportunities expired in that wait, the 2 started were cut by a block).
inline constexpr unsigned kCounterDelay = 6;
// FIX02 (user test 0.13.1: 38 of 100 frontal swings met a cooldown): 120/90/60 -> 45/35/25 ticks, still longer than Link's native rebound recoil,
// so a parry can never chain on the very swing that is still bouncing.
inline constexpr unsigned kParryCooldown = 45, kParryCooldownB = 35, kParryCooldownC = 25;
inline constexpr unsigned kParryLease = 12;      // ticks a prepared parry stays presented for one swing
inline constexpr unsigned kWoundReadyDelay = 6;  // a guard prepared by wounds is up only after this delay (never retroactive)
inline constexpr unsigned kWoundMemory = 90;
inline constexpr unsigned kPressureWindow = 75, kSustainedWindow = 150, kBlockPairWindow = 45;
inline constexpr int kPctA = 12, kPctB = 35, kPctC = 45, kPctTwoBlocks = 80;
inline constexpr int kBlockBase = 30;            // [BALANCE_PROVISIONAL] not in the request: guard roll of a frontal, ready Chilfos with no wound streak
inline constexpr int kWoundPct[3] = {45, 80, 100};
// FIX02 pressure guard: the guard roll grows with the aimed swings of the last kSustainedWindow ticks (1, 2, 3, 4+) and is no longer reset to the base by
// a block (0.13.1 oscillated 45 -> 30 -> 45 under repeated B). After kGuaranteeAfter frontal swings in a row that met a READY guard and still wounded
// it, the next ready frontal swing is blocked for sure (never retroactive: the wound-ready delay applies). Flank and back hits are never blocked.
inline constexpr int kPressurePct[4] = {30, 50, 70, 85};
inline constexpr unsigned kGuaranteeAfter = 2;
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
inline constexpr float kRateD = 1.25f, kRateE = 1.30f, kRateEBurst = 1.10f, kRateDFast = 1.40f, kRateBack = 1.25f, kRateThrow = 1.45f, kRateRestore = 1.60f;
// executeBackWalk writes speedF = 15 every frame for BCK7 at rate 1.0. FIX01: the C3 backstep plays BCK7 at kRateBack, so its ground speed is scaled by
// the same factor (15 x 1.25) or the legs would stride 25% faster than the body moves (foot sliding).
inline constexpr float kBackSpeedNative = 15.0f;
inline constexpr std::uint32_t kSeHitSword = 0x40000;  // Z2SE_HIT_SWORD (Z2AudioLib/Z2SeMgr.h, pinned by ci/kk_native_audit.py): the native Shield Attack contact SE
inline constexpr float kBurstMax = 0.40f;  // seconds: an E-E pair recorded closer than this is a burst (FIX02: both thrusts at x1.10, the Lizalfos FIX05 double)
// FIX02 motion. executeAttack writes speedF = 80 at frame 24 (the lunge), action() then decays it with cLib_addCalc0(&speedF, 0.7f, 10.0f) before
// fopAcM_posMoveF moves the actor: a full lunge travels ~284 units. Against a Link already in reach the lunge only shoves him (body collision): it is
// capped so the Chilfos stops kContactDist from Link (bodies 70 + Link's apart; the spear tip is 230 ahead, so the thrust still reaches).
inline constexpr float kLungeNative = 80.0f, kContactDist = 150.0f;
inline constexpr float kRecoilNative = -70.0f;
// FIX03. Link's cut types (SDK include/d/actor/d_a_player.h) and procCutFinishInit's native table indices (d_a_alink_cut.inc).
inline constexpr int kCutHeadJump = 0x05, kCutLargeJump = 0x13, kCutLargeJumpFinish = 0x14, kCutLargeTurnLeft = 0x17, kCutLargeTurnRight = 0x18,
                     kCutMortalDrawA = 0x1A, kCutTwirl = 0x1E, kCutMortalDrawB = 0x1F;
inline constexpr int kFinishMortalDrawA = 3, kFinishMortalDrawB = 4;  // CUT_FINISH_PARAM_MORTAL_DRAW_A/B: B when R is not held (the native checkCutAction rule)
// The Mortal Draw finisher: B (a native cut decision of checkCutAction) on a broken Chilfos Link faces, within reach. A Mortal Draw that really started
// may outlast the 90-tick stagger by at most kMdExtendMax ticks, never a Shield Attack/Helm Splitter path.
inline constexpr float kMdReach = 350.0f;
inline constexpr int kMdArc = 0x2AAA;  // 60 degrees each side of Link's facing
inline constexpr unsigned kMdExtendMax = 30;
inline constexpr unsigned kPerfFrames = 90;  // presented frames measured before and after each C3 spear release
// FIX04 P0 (the C3 throw frame drop). executeSpearThrow mode 2 tests its melee cancel every Execute while frame < 23 and Link is within direct_attack_range:
// `!fopAcM_otherBgCheck(this, player) && ... && eyePos.y + 50 > player.y`. fopAcM_otherBgCheck (EXE 0x14076a110) casts a dBgS_LinChk from
// (pos.x, eyePos.y [actor+0x630], pos.z) to Link + 100 and dBgWKCol::LineCheck (EXE 0x14080b150) walks EVERY KCL along that segment in 500-unit steps
// (constant 500.0f at 0x141e4e718). 0.13.0..0.13.3 parked eyePos.y at -1e9 to defeat the cancel: a ~1e9-unit segment, ~2 000 000 steps per KCL per Execute
// for the ~15 Execute calls of the wind-up (the log's 15 ticks of ~44 ms per throw). The cancel needs only `eyePos.y + 50 <= Link.y`: the eye is parked
// kThrowEyeBelow under Link's feet, so the native segment is as short as the vanilla one and the cancel still never fires.
inline constexpr float kThrowEyeBelow = 60.0f;   // > the native 50
inline constexpr float kLosStep = 500.0f;        // dBgWKCol::LineCheck stride
inline constexpr float kLosOffsetY = 100.0f;     // fopAcM_otherBgCheck: end = Link + 100
// FIX04 P1 (one contact per spear attack). cc_set registers mSph only while field_0x67d == 0 && field_0x67e == 0 and arms the six shaft spheres' AT with
// the same condition; cCcS::ClrAtHitInf clears only REGISTERED AT objects, so a contact still held when the window closes leaves STALE AT-hit flags that
// damage_check replays when its 8-tick timer expires (the log: 10 impacts with a second native round exactly 8 ticks later, 11 rounds outside any window,
// motion_recoil_fixed=430 for ~330 real receipts). damage_check also plays one Z2SE_HIT_SWORD per sphere of the same pass.
inline constexpr std::uint32_t kSeMapSpear = 0x28, kSeMapGuard = 7, kSeMapClash = 0x1F;  // startCollisionSE map infos (damage_check / def_se_set)
// FIX04 P3: the ready Chilfos watches Link while it waits (the native guard's own turn: cLib_addCalcAngleS2(.., 8, 0x500)).
inline constexpr int kFaceScale = 8, kFaceStep = 0x500;
inline constexpr float kFaceRange = 900.0f;
// FIX05 P0 (the stagger seen twice at its natural end). executeYoroke asks DAMAGE_BOW (9) / DAMAGE_BOWB (0xB) with attr 0: ONE play of the clip, the bow AND
// the native return to the stance, then nextActionCheck. 0.13.3/0.13.4 looped it (attr 2, J3DFrameCtrl EMode_LOOP, loop frame 0) for the 90-tick window: the
// clip is shorter than 90 ticks (the 0.13.4 log reads frame 40 at age 40 without a wrap), so the Chilfos straightened up at the clip's end, then the loop jumped
// back to frame 0 and bowed again before RECOVERED cut it to the wait. Now the clip plays ONCE across the window: its first frames at the native rate, the rest
// stretched so its own last frame (the upright stance) is reached when the window ends; the loop frame is the last frame (EMode_LOOP never sets the stop
// state, so the native Yoroke never leaves early, and nothing ever jumps back to frame 0).
inline constexpr unsigned kBowJoltFrames = 10;
inline constexpr float kBowRateMin = 0.25f, kBowRateMax = 1.0f;
// FIX05 P1 (repeated Jump Attacks). procCutJump runs changeCutReverseProc(ANM_CUT_JUMP) (aerial: heading reversed, mNormalSpeed 27, POWER5 shock, no clip of
// its own) and procCutJumpLand runs changeCutReverseProc(ANM_CUT_RECOIL_B) (PROC_CUT_REVERSE with its recoil clip): both are native rebound routes. setCutType
// (CUT_TYPE_JUMP) is called twice per jump (procCutJumpInit, procCutJumpLandInit): ONE swing serial per jump.
inline constexpr unsigned kJumpWindow = 180;             // ticks a jump attack stays in the streak (6 s)
inline constexpr int kJumpParryPct[4] = {0, 40, 60, 75}; // prepared-parry floor for the 1st, 2nd, 3rd, 4th+ jump of a streak (0: the ordinary level roll)
inline constexpr int kJumpBlockPct[4] = {0, 60, 80, 90}; // guard floor for the same jumps
inline constexpr unsigned kJumpParryLease = 30;          // a jump's contact comes after its airtime: the prepared parry covers takeoff to landing
// FIX05 P3 (Helm Splitter too low). procCutHeadInit aims at getHeadLockPos() = eyePos = head joint 4 - 50 (cc_set): the stagger clip bows the head, so the
// jump height clamp(dy, 30, mMaxHeight) is computed from a bowed head. While broken with the Shield Attack prepared, the head-lock point is lifted to this
// actor's own measured upright head height (sampled in its stance), never lowered.
inline constexpr float kHeadSampleMin = 80.0f, kHeadSampleMax = 600.0f;
// FIX05 P4 (Link's torso after the Mortal Draw kill). procCutFinish calls setBodyAngleXReadyAnime(1) every frame toward mTargetedActor->eyePos, and during
// PROC_CUT_FINISH the target is the Mortal Draw's own (field_0x280c): the dead Chilfos falls and Link's torso follows its eye down. The eye of THAT dead
// Chilfos keeps its kill height while Link is still in that Mortal Draw, at most kMdPoseHold ticks.
inline constexpr unsigned kMdPoseHold = 60;
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
  unsigned uncovered = 0;            // FIX02: frontal swings in a row that met a ready guard and still wounded
  std::uint64_t last_uncovered = 0;
  std::uint64_t guarantees = 0;
  std::uint64_t last_wound = 0, cooldown_until = 0;
  std::uint64_t block_ticks[2] = {}, block_serials[2] = {};
  unsigned nblocks = 0;
  // prepared parry for ONE swing
  bool prepared = false;
  std::uint64_t prepared_serial = 0, prepared_until = 0;
  // counters
  std::uint64_t rolls[4] = {}, prepares[4] = {};  // A,B,C,two-blocks
  // FIX05 P1: the jump attacks of a streak (one entry per jump, takeoff and landing are one swing)
  std::uint64_t jumps[4] = {};
  unsigned njump = 0;
  std::uint64_t jump_serial = 0;
  // FIX06 P4: floors of ONE swing (the adapter sets them for a Back Slice of a streak; 0 = none)
  std::uint64_t extra_serial = 0;
  int extra_parry = 0, extra_block = 0;
  void set_extra(std::uint64_t serial, int parry_floor, int block_floor) noexcept { extra_serial = serial, extra_parry = parry_floor, extra_block = block_floor; }
  int extra(std::uint64_t serial, bool parry) const noexcept { return serial != 0 && serial == extra_serial ? (parry ? extra_parry : extra_block) : 0; }
  unsigned jump_streak(std::uint64_t now) const noexcept {
    unsigned c = 0;
    for (unsigned i = 0; i < 4 && i < njump; ++i)
      if (jumps[i] && now - jumps[i] <= policy::kJumpWindow) ++c;
    return c;
  }
  int jump_floor(const int* table, std::uint64_t now, std::uint64_t serial) const noexcept {
    if (serial == 0 || serial != jump_serial) return 0;
    const unsigned n = jump_streak(now);
    return n == 0 ? 0 : table[n >= 4 ? 3 : n - 1];
  }
  void swing(std::uint64_t t) noexcept { swings[nsw++ % kHist] = t; }
  unsigned within(std::uint64_t now, unsigned w) const noexcept {
    unsigned c = 0;
    for (unsigned i = 0; i < kHist && i < nsw; ++i)
      if (swings[i] && now - swings[i] <= w) ++c;
    return c;
  }
  int level(std::uint64_t now) const noexcept {  // 0=A 1=B 2=C
    if (within(now, policy::kSustainedWindow) >= 4) return 2;
    if (within(now, policy::kPressureWindow) >= 3) return 1;
    return 0;
  }
  bool two_blocks(std::uint64_t now) const noexcept {
    return nblocks >= 2 && block_serials[0] != block_serials[1] && now - block_ticks[(nblocks - 1) % 2] <= policy::kBlockPairWindow;
  }
  void on_block(std::uint64_t now, std::uint64_t serial) noexcept {
    block_ticks[nblocks % 2] = now, block_serials[nblocks % 2] = serial, ++nblocks;
    hits_since_defense = 0, wound_streak = 0, uncovered = 0;
  }
  void on_uncovered(std::uint64_t now) noexcept {  // a ready, frontal guard that this swing still beat
    if (now - last_uncovered > policy::kWoundMemory) uncovered = 0;
    ++uncovered, last_uncovered = now;
  }
  void on_wound(std::uint64_t now) noexcept {
    if (now - last_wound > policy::kWoundMemory) wound_streak = 0;
    ++wound_streak, ++hits_since_defense, last_wound = now;
  }
  void on_enemy_parry(std::uint64_t now) noexcept {
    cooldown_until = now + (level(now) == 2 ? policy::kParryCooldownC : level(now) == 1 ? policy::kParryCooldownB : policy::kParryCooldown), prepared = false, nblocks = 0, hits_since_defense = 0, wound_streak = 0,
    uncovered = 0;
  }
  // A new real swing of Link aimed at this actor: at most one roll. Returns the reason it was not prepared.
  Np on_swing(std::uint64_t now, std::uint64_t serial, bool spear, bool attacking, bool frontal, bool broken, Rng& r, bool jump = false) noexcept {
    swing(now);
    if (jump) jumps[njump++ % 4] = now, jump_serial = serial;
    if (broken) return Np::Broken;
    if (!spear) return Np::NoSpear;
    if (attacking) return Np::Attacking;
    if (!frontal) return Np::Flank;
    if (now < cooldown_until) return Np::Cooldown;
    const bool pair = two_blocks(now);
    const int lv = level(now);
    const int lpct = pair ? policy::kPctTwoBlocks : lv == 2 ? policy::kPctC : lv == 1 ? policy::kPctB : policy::kPctA;
    const int jf0 = jump_floor(policy::kJumpParryPct, now, serial);  // FIX05: a jump streak answers with parries, never every time (75 % at most)
    const int jf = extra(serial, true) > jf0 ? extra(serial, true) : jf0;  // FIX06: a Back Slice streak too (70 % at most)
    const int pct = jf > lpct ? jf : lpct;
    const unsigned slot = pair ? 3u : unsigned(lv);
    ++rolls[slot];
    if (pair) nblocks = 0;  // the pair is consumed by this continuation, whatever the roll says
    if (!r.pct(pct)) return Np::Rng;
    ++prepares[slot];
    prepared = true, prepared_serial = serial, prepared_until = now + (jump ? policy::kJumpParryLease : policy::kParryLease);
    return Np::None;
  }
  bool presenting(std::uint64_t now, std::uint64_t serial) const noexcept { return prepared && prepared_serial == serial && now <= prepared_until; }
  void expire(std::uint64_t now) noexcept {
    if (prepared && now > prepared_until) prepared = false;
  }
  // Block roll for a swing that was NOT prepared as a parry: the guard is a native reaction, the wound streak prepares it (never retroactively).
  int wound_pct(std::uint64_t now) const noexcept {
    if (wound_streak == 0 || now - last_wound > policy::kWoundMemory) return policy::kBlockBase;
    if (now - last_wound < policy::kWoundReadyDelay) return policy::kBlockBase;  // not up yet
    return policy::kWoundPct[wound_streak >= 3 ? 2 : wound_streak - 1];
  }
  int pressure_pct(std::uint64_t now) const noexcept {
    const unsigned n = within(now, policy::kSustainedWindow);
    return policy::kPressurePct[n >= 4 ? 3 : n == 0 ? 0 : n - 1];
  }
  bool guaranteed(std::uint64_t now) const noexcept {  // FIX02: never retroactive, forgotten with the wounds
    return uncovered >= policy::kGuaranteeAfter && now - last_uncovered <= policy::kWoundMemory && now - last_uncovered >= policy::kWoundReadyDelay;
  }
  int block_pct(std::uint64_t now, std::uint64_t serial = 0) const noexcept {
    if (guaranteed(now)) return 100;
    const int w = wound_pct(now), pr = pressure_pct(now), j = jump_floor(policy::kJumpBlockPct, now, serial);
    const int m = w > pr ? w : pr;
    const int jm = j > m ? j : m, e = extra(serial, false);  // FIX06 P4: a Back Slice streak's guard floor for this swing
    return e > jm ? e : jm;
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
enum class Cc : std::uint8_t { NoSpear = 0, Broken, Dead, Event, LinkFar, Action, Count };  // FIX02: why a pending counter could not start
inline const char* name(Cc c) noexcept {
  static constexpr const char* n[] = {"NO_SPEAR", "BROKEN", "DEAD", "EVENT", "LINK_FAR", "ACTION"};
  return unsigned(c) < unsigned(Cc::Count) ? n[unsigned(c)] : "?";
}
enum class End : std::uint8_t { Completed = 0, Posture100, Death, NativePriority, Defense, LinkLeft, SceneOrDelete, Event, Watchdog, Count };
inline const char* name(End e) noexcept {
  static constexpr const char* n[] = {"COMPLETED", "POSTURE_100_BREAK", "DEATH", "NATIVE_PRIORITY", "ADAPTIVE_DEFENSE_AT_SAFE_BOUNDARY", "LINK_LEFT", "SCENE_OR_DELETE", "EVENT", "WATCHDOG"};
  return unsigned(e) < unsigned(End::Count) ? n[unsigned(e)] : "?";
}
// FIX02: the distance a lunge of speed v travels under action()'s cLib_addCalc0(&speedF, 0.7f, 10.0f) + fopAcM_posMoveF (decay first, then move).
inline float lunge_travel(float v) noexcept {
  float d = 0;
  for (int i = 0; i < 64 && v > 0.01f; ++i) {
    const float step = v * 0.7f < 10.0f ? v * 0.7f : 10.0f;
    v -= step, d += v;
  }
  return d;
}
inline float lunge_for(float allowed) noexcept {  // the largest native-shaped lunge that travels at most `allowed`
  if (allowed <= 0.0f) return 0.0f;
  if (lunge_travel(policy::kLungeNative) <= allowed) return policy::kLungeNative;
  float lo = 0.0f, hi = policy::kLungeNative;
  for (int i = 0; i < 30; ++i) {
    const float mid = 0.5f * (lo + hi);
    (lunge_travel(mid) <= allowed ? lo : hi) = mid;
  }
  return lo;
}
// FIX03: exactly the cut types for which the native daE_KK_c::mCutTypeCheck(2) answers true (the instant-kill branch of damage_check)
inline bool native_instant_cut(int c) noexcept {
  return c == policy::kCutLargeJump || c == policy::kCutLargeJumpFinish || c == policy::kCutLargeTurnLeft || c == policy::kCutLargeTurnRight ||
         c == policy::kCutMortalDrawA || c == policy::kCutMortalDrawB || c == policy::kCutHeadJump || c == policy::kCutTwirl;
}
inline bool mortal_draw_cut(int c) noexcept { return c == policy::kCutMortalDrawA || c == policy::kCutMortalDrawB; }
// FIX04 P0: the eye height for one wind-up Execute of the C3 throw. Never raised; only lowered to just under Link's feet (cancel off, short native segment).
inline float throw_eye(float eye, float link_y) noexcept {
  const float want = link_y - policy::kThrowEyeBelow;
  return eye < want ? eye : want;
}
// The number of 500-unit strides dBgWKCol::LineCheck walks for a segment of this length (per KCL): `do { ... } while ((k += 500) <= len)`.
inline std::uint64_t los_strides(float len) noexcept { return len <= 0.0f ? 0ull : static_cast<std::uint64_t>(len / policy::kLosStep) + 1ull; }
// FIX04 P1: what one damage_check call does with the AT-hit flags of the spear's seven spheres (bit 0 = mSph tip, bits 1..6 = mSpheres[0..5]).
//   live:        the spheres were registered with their AT in the pass that produced these flags (field_0x67d == 0 && field_0x67e == 0 at that cc_set)
//   timer_zero:  mDamageTimer == 0 (the native plays a round this call)
//   token:       the native AT window (attack) these flags belong to; sounded: the last token that already had its round
enum class Round : std::uint8_t { None = 0, Stale, Echo, Keep, Self };
struct RoundOut {
  Round r = Round::None;
  unsigned keep = 0;   // the one bit the native round may see (Keep)
  unsigned clear = 0;  // the bits to clear before the native reads them
};
inline unsigned low_bit(unsigned m) noexcept { return m & (~m + 1u); }
inline unsigned bits(unsigned m) noexcept {
  unsigned n = 0;
  for (; m; m &= m - 1u) ++n;
  return n;
}
inline RoundOut spear_round(unsigned any_hit, unsigned shield_hit, bool live, bool timer_zero, std::uint64_t token, std::uint64_t sounded) noexcept {
  RoundOut o;
  if (!any_hit) return o;
  if (!live) return o.r = Round::Stale, o.clear = any_hit, o;  // no pass touched them: nothing is in contact now
  if (!shield_hit) return o;                                   // a body contact: its own receipt path (kk_link_contact), the native plays no SE
  if (token == sounded) return o.r = Round::Echo, o.clear = shield_hit, o;  // the same attack already had its contact
  if (!timer_zero) return o.r = Round::Self, o.clear = shield_hit, o;       // first contact of a new attack while the timer of an older event runs
  o.r = Round::Keep, o.keep = low_bit(shield_hit), o.clear = shield_hit & ~o.keep;
  return o;
}
// FIX04 P3: cLib_addCalcAngleS2(&a, target, scale, max_step) exactly (SSystem/SComponent/c_lib.cpp): as the native guard turns.
inline std::int16_t turn_toward(std::int16_t cur, std::int16_t target, int scale, int max_step) noexcept {
  const std::int16_t diff = static_cast<std::int16_t>(target - cur);
  int step = diff / scale;
  if (step > max_step) step = max_step;
  if (step < -max_step) step = -max_step;
  return static_cast<std::int16_t>(cur + step);
}
inline int angle_diff(int a, int b) noexcept {
  const int d = int(std::int16_t(std::uint16_t(a - b)));
  return d < 0 ? -d : d;
}

// ---------------------------------------------------------------- FIX05 P0: one play of the stagger clip across the window
// The jolt: the clip's first frames at the native rate (never more than a third of it).
inline float bow_jolt(float end) noexcept {
  const float t = end / 3.0f;
  return t < float(policy::kBowJoltFrames) ? t : float(policy::kBowJoltFrames);
}
// The rate that brings the clip from `frame` to its last frame (end - 1) in `ticks_left` Execute calls, clamped (never faster than native).
inline float bow_rate(float frame, float end, std::uint64_t ticks_left) noexcept {
  const float left = end - 1.0f - frame;
  if (left <= 0.0f) return policy::kBowRateMin;
  float r = left / float(ticks_left ? ticks_left : 1);
  if (r < policy::kBowRateMin) r = policy::kBowRateMin;
  if (r > policy::kBowRateMax) r = policy::kBowRateMax;
  return r;
}
// The loop frame of the stagger clip: its last frame, so EMode_LOOP holds the stance instead of jumping back to frame 0 (s16, as J3DFrameCtrl::mLoop).
inline std::int16_t bow_loop(std::int16_t end) noexcept { return end > 1 ? std::int16_t(end - 1) : std::int16_t(0); }
// ---------------------------------------------------------------- FIX05 P3: the Helm Splitter aim point of a broken Chilfos
// `upright` is the head-lock height above current.pos measured in this actor's own stance (0 = never measured: native kept). The adapter writes it only
// when it is HIGHER than the native point (never lowered).
inline float head_lock_y(float native_y, float pos_y, float upright) noexcept {
  if (upright < policy::kHeadSampleMin || upright > policy::kHeadSampleMax) return native_y;
  return pos_y + upright;
}
inline bool head_sample_ok(float rel) noexcept { return rel >= policy::kHeadSampleMin && rel <= policy::kHeadSampleMax; }

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
// FIX01: the 0.13.0 diagonal rule (sideways > 1.2 x forward) rejected a real sweep. The user's 0.13.0 log (engine pose, Dusklight 2.0.3): BCK5 tip
// moved 320..357 sideways for 325..441 forward (13 of 14 UNSURE), BCK6 0..23 sideways for 60..169 forward (27/27 THRUST). An arc swept from the side
// to the front has both components of the same size, so a sweep is now a sideways/vertical motion of at least 100 units and 0.6 x the forward one.
inline Tip classify_tip(float dx, float dy, float dz) noexcept {
  const float lv = fsqrt(dx * dx + dy * dy), fz = dz < 0 ? -dz : dz;
  if (dz > 30.0f && dz > 1.5f * lv) return Tip::Thrust;
  if (lv > 100.0f && lv >= 0.6f * fz) return Tip::Diagonal;
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
  X(native_deaths) X(fallback_deaths) X(decisive_no_receipt) X(headlock_stripped) X(watchdog) X(regen_ticks)                                     \
  X(bash_absorbed) X(bash_se) X(resumed) X(resume_skipped) X(immediate_breaks) X(sword_dupes) X(back_speed_fixes) X(rearmed)                  \
  X(spear_break_prevented) X(blocked_spear_loss) X(other_spear_loss) X(counter_dispatched) X(counter_started) X(recoil_fixed) X(lunge_clamped)     \
  X(guard_guarantees)                                                                                                                                  \
  X(guard_active_blocks) X(skill_instant_suppressed) X(stagger_shield_closed) X(stagger_clip_kept) X(md_requests) X(md_started) X(md_cancelled)     \
  X(md_kills) X(md_extended) X(death_hp_zero) X(death_skill_instant) X(death_sword_decisive) X(death_helm) X(death_iron) X(death_other)              \
  X(spear_children)                                                                                                                                    \
  X(throw_eye_writes) X(throw_eye_kept) X(stale_cleared) X(echo_suppressed) X(se_merged) X(se_self) X(marks_kept) X(marks_suppressed) X(guard_marks)    \
  X(guard_raised) X(guard_raise_fail) X(guard_after_wound) X(face_ticks) X(parry_settle_post) X(contact_tokens)                                     \
  X(bow_plans) X(bow_stretches) X(bow_wraps) X(bow_exit_end) X(jump_swings) X(jump_land_merged) X(jump_parries) X(jump_blocks) X(jump_damage)          \
  X(jump_rebound_air) X(jump_rebound_land) X(block_dupes) X(burst_gap_ticks) X(head_samples) X(head_lifts) X(md_pose_holds)                    \
  X(js_contacts) X(js_credited) X(js_dupes) X(js_floored) X(js_nonlethal) X(js_hp_units) X(js_native_units) X(js_breaks_kept) X(js_headroom_miss)        \
  X(def_hp_restored) X(land_recoil_armed) X(land_recoil_taken) X(twirl_swings) X(twirl_turn_ticks) X(twirl_parries) X(twirl_blocks) X(twirl_damage)        \
  X(chain_rolls) X(chain_scheduled) X(chain_started) X(chain_no) X(chain_cancelled) X(spark_moved) X(spark_native)
struct Stats {
  std::uint64_t selected[kPatterns] = {}, completed[kPatterns] = {}, ended[unsigned(End::Count)] = {};
  std::uint64_t verdict[unsigned(V::Count)] = {}, no_parry[unsigned(Np::Count)] = {}, counter_cancel[unsigned(Cc::Count)] = {};
#define KK_STAT_FIELD(f) std::uint64_t f = 0;
  KK_STATS_SCALARS(KK_STAT_FIELD)
#undef KK_STAT_FIELD
  void add(const Stats& x) noexcept {
    for (unsigned i = 0; i < kPatterns; ++i) selected[i] += x.selected[i], completed[i] += x.completed[i];
    for (unsigned i = 0; i < unsigned(End::Count); ++i) ended[i] += x.ended[i];
    for (unsigned i = 0; i < unsigned(V::Count); ++i) verdict[i] += x.verdict[i];
    for (unsigned i = 0; i < unsigned(Np::Count); ++i) no_parry[i] += x.no_parry[i];
    for (unsigned i = 0; i < unsigned(Cc::Count); ++i) counter_cancel[i] += x.counter_cancel[i];
#define KK_STAT_ADD(f) f += x.f;
    KK_STATS_SCALARS(KK_STAT_ADD)
#undef KK_STAT_ADD
  }
};

// ================================================================ FIX06 (0.13.6): pure decisions shared by the Chilfos, Lizalfos and Bokoblin adapters
namespace f6 {
// ---- P0 Jump Strike (Great Jump). Link's cut types (SDK d_a_player.h): LARGE_JUMP_INIT 0x12, LARGE_JUMP 0x13, LARGE_JUMP_FINISH 0x14.
// d_a_alink_cut.inc: procCutLargeJumpInit setSwordAtParam(atp 2) + cut 0x12; the airborne swing setSwordAtParam(spl 1, atp 4) + cut 0x13; the landing
// procCutLargeJumpLandInit cut 0x13 again, then initCutTurnAt(r, atp 4) + cut 0x14. d_cc_uty.cpp at_power_get (mPowerType 0): atp 2 -> 10, 3 -> 30,
// 6 -> 80, >= 4 -> 200; cc_at_check: x2 Master Sword, /2 wooden sword, x2 sword-up time, 100 for a light material. So the airborne and landing hits of
// the Jump Strike take 200 x 2 = 400 HP with the Master Sword (the log: Lizalfos 350 -> 0 at cut 20, Chilfos 280 -> 0 at cut 19) where a cut takes 20.
inline constexpr int kCutLargeJumpInit = 0x12, kCutLargeJump = 0x13, kCutLargeJumpFinish = 0x14;
inline constexpr bool js_cut(int c) noexcept { return c == kCutLargeJumpInit || c == kCutLargeJump || c == kCutLargeJumpFinish; }
// Headroom raised on the actor's HP before its damage_check so the native never sees a lethal result for that one Jump Strike contact: >= the largest
// possible native power (200 x2 Master Sword x2 sword-up = 800; light material 100). Never above s16 for a 340/370 HP actor.
inline constexpr int kJsHeadroom = 800;
inline constexpr int kJsNum = 7, kJsDen = 4;  // [BALANCE_PROVISIONAL] 1.75 x the ordinary EFFECTIVE sword damage of the same hit (user range 1.5..2.0)
// The lowest HP at which each native still processes a hit as a living actor: E_KK damage_check returns at once at health <= 1 (it could never be hit
// again) and E_OC treats health <= 1 as its fall-and-die (my_val 5, attention flags 0): 2. E_DN only dies at health <= 0: 1.
inline constexpr int kFloorKk = 2, kFloorDn = 1, kFloorOc = 2;
inline constexpr int kGainJsKk = 8, kGainJsDn = 10;  // user table: 1.6 x the ordinary sword gain (Chilfos 5, Lizalfos 6), ONCE per Jump Strike
inline constexpr int atp_power(int atp) noexcept {  // at_power_get, mPowerType 0, without the AT_TYPE_10000000 case (never a sword)
  return atp <= 0 ? 0 : atp == 1 ? 1 : atp == 2 ? 10 : atp == 3 ? 30 : atp == 6 ? 80 : 200;
}
// The ordinary effective damage of the same contact: what a cut (atp 2) would have taken with the same multipliers (sword, sword-up time).
inline constexpr int ordinary_of(int native_damage, int atp) noexcept {
  return native_damage <= 0 ? 0 : atp_power(atp) <= 0 ? native_damage : (native_damage * atp_power(2) + atp_power(atp) / 2) / atp_power(atp);
}
inline constexpr int js_damage(int ordinary) noexcept {  // strictly more than a cut, bounded: 1.75 x (20 -> 35 with the Master Sword)
  return ordinary <= 0 ? 0 : ((ordinary * kJsNum + kJsDen / 2) / kJsDen > ordinary ? (ordinary * kJsNum + kJsDen / 2) / kJsDen : ordinary + 1);
}
// HP after the Jump Strike contact: never below the native living floor, never a gain, never lethal by itself.
inline constexpr int js_target(int hp_before, int damage, int floor) noexcept {
  return hp_before <= floor ? hp_before : (hp_before - damage < floor ? floor : hp_before - damage);
}
// ONE HP credit and ONE posture credit per Jump Strike per actor: the episode is Link's last LARGE_JUMP_INIT (every Jump Strike sets it first).
struct JsCredit {
  std::uint64_t ep = 0;
  bool take(std::uint64_t episode) noexcept {
    if (episode == 0) return true;  // no Jump Strike clock seen (its cut 18 was never observed): every contact is credited, still bounded and never lethal
    if (ep == episode) return false;
    ep = episode;
    return true;
  }
};
// ---- P2 Jump Attack parried in the air: Link's own landing recoil. procCutJump runs changeCutReverseProc(ANM_CUT_JUMP) (heading reversed, speed 27,
// POWER5 shock, NO clip) and lands a few frames later with mNormalSpeed = 0, so the repulsion is barely visible; procCutJumpLand runs
// changeCutReverseProc(ANM_CUT_RECOIL_B) on its first ticks: TRUE there is the native PROC_CUT_REVERSE with the recoil clip. A parry of the AIR phase
// arms one token for the SAME jump; the landing's checkCutReverseAt answers it once (native route, native clip), never without a real parry.
inline constexpr unsigned kLandRecoilTtl = 45;  // ticks from the air parry to the landing (the jump's airtime is ~10-25)
struct LandRecoil {
  bool armed = false;
  std::uint64_t serial = 0, tick = 0;
  std::uint64_t armed_n = 0, taken = 0, expired = 0;
  void arm(std::uint64_t jump_serial, std::uint64_t now) noexcept {
    if (jump_serial == 0) return;
    armed = true, serial = jump_serial, tick = now, ++armed_n;
  }
  // land: Link is in PROC_CUT_JUMP_LAND; cur_serial: the jump serial of that landing (the takeoff's, the landing is merged into it)
  bool take(bool land, std::uint64_t cur_serial, std::uint64_t now) noexcept {
    if (!armed) return false;
    if (now > tick + kLandRecoilTtl) return armed = false, ++expired, false;
    if (!land || cur_serial != serial) return false;
    armed = false, ++taken;
    return true;
  }
};
// Shared FIX06 clocks (one Link): the Jump Strike episode (Link's LARGE_JUMP_INIT) and the Jump Attack identity (each PROC_CUT_JUMP takeoff), kept by
// the Chilfos setCutType POST for every admitted enemy family; the landing-recoil token is armed by whichever duelist parried the jump in the air.
inline std::uint64_t g_js_episode = 0, g_jump_id = 0;
inline LandRecoil g_land_recoil;
struct JsTally { std::uint64_t contacts = 0, credited = 0, dupes = 0, floored = 0, hp_units = 0, native_units = 0, lines = 0, dropped = 0; };
inline JsTally g_js_tally[3];  // per family with a Posture System: 0 Chilfos (summary from its slots), 1 Lizalfos, 2 Bokoblin
// ---- P3 counter -> chained combo. After the counter really ENDS (its last window closed, End::Completed), ~65% of the time one of the four normal
// patterns (1/4 each) starts after a short natural gap; at most ONE chained combo per counter (the chained combo never chains again by itself).
inline constexpr int kChainPct = 65;                       // [BALANCE_PROVISIONAL] user request, to be tuned by play
inline constexpr unsigned kChainGapMin = 4, kChainGapMax = 9;  // ticks between the counter's end and the native attack entry of the chained combo
inline constexpr unsigned kChainTtl = 30;                  // the opportunity is cancelled if the actor cannot start within this
inline constexpr unsigned kChainLinkQuiet = 24;            // the chained combo needs a real opening: no new swing of Link in the last 0.8 s
enum class Ch : std::uint8_t { None = 0, Pending, Started, NoChain, Cancelled };
struct Chain {
  Ch st = Ch::None;
  std::uint64_t token = 0, at = 0, until = 0, ended = 0;
  unsigned pattern = 0;
  // counter_token: the parry serial of the counter that completed; returns true when a chain is scheduled
  // completed: the counter episode ended on its own terms (its last window closed); anything else (death, break, defense, scene) never chains
  template <class R>
  bool on_counter_end(bool completed, std::uint64_t counter_token, std::uint64_t now, R& rng) noexcept {
    if (st == Ch::Pending) st = Ch::Cancelled;
    if (!completed || counter_token == 0 || counter_token == token) {
      if (completed && counter_token != 0 && counter_token == token) st = Ch::NoChain;  // never twice for the same counter
      else if (!completed) st = Ch::Cancelled;
      return false;
    }
    token = counter_token, ended = now;
    if (!rng.pct(kChainPct)) return st = Ch::NoChain, false;
    pattern = rng.below(4), at = now + kChainGapMin + rng.below(kChainGapMax - kChainGapMin + 1), until = now + kChainTtl, st = Ch::Pending;
    return true;
  }
  bool due(std::uint64_t now) const noexcept { return st == Ch::Pending && now >= at; }
  bool expired(std::uint64_t now) const noexcept { return st == Ch::Pending && now > until; }
  void cancel() noexcept { if (st == Ch::Pending) st = Ch::Cancelled; }
  void started() noexcept { st = Ch::Started; }
};
// ---- P4 Back Slice (CUT_TYPE_TWIRL 0x1E: procCutFinishJumpUp / ...Land, PROC 35/36: no changeCutReverseProc in either, so a parry NEVER makes Link
// recoil: the clip goes on). Repeated Back Slices within kTwirlWindow make the defender turn toward Link while he circles (real frontal cover, never a
// 360 degree guard: the first Back Slice of a streak is not tracked) and raise its parry/guard floors.
inline constexpr int kCutTwirl = 0x1E;
inline constexpr unsigned kTwirlWindow = 240;
inline constexpr int kTwirlParryPct[4] = {0, 35, 55, 70}, kTwirlBlockPct[4] = {0, 50, 70, 85};
inline constexpr int kTwirlTurnStep[4] = {0, 0x0A00, 0x1000, 0x1400};  // per tick, the turn while Link's Back Slice is live (streak 1: none)
inline constexpr int kLinkTwirlParry = 25;  // Link's posture on a parried Back Slice (the existing enemy-parry scale), no proc change
struct Twirl {
  std::uint64_t t[4] = {}, serial = 0;
  unsigned n = 0;
  void on(std::uint64_t now, std::uint64_t s) noexcept {
    if (s != 0 && s == serial) return;
    t[n++ % 4] = now, serial = s;
  }
  unsigned streak(std::uint64_t now) const noexcept {
    unsigned c = 0;
    for (unsigned i = 0; i < 4 && i < n; ++i)
      if (t[i] && now - t[i] <= kTwirlWindow) ++c;
    return c;
  }
  int floor(const int* table, std::uint64_t now) const noexcept {
    const unsigned c = streak(now);
    return c == 0 ? 0 : table[c >= 4 ? 3 : c - 1];
  }
  int turn_step(std::uint64_t now) const noexcept {
    const unsigned c = streak(now);
    return c == 0 ? 0 : kTwirlTurnStep[c >= 4 ? 3 : c - 1];
  }
};
// ---- P6 the spark of a spear contact on Link's guard. dCcS::ProcAtTgHitmark draws it at the AT/TG cross point; at point-blank range that point lies
// inside Link (behind his shield face) and the emitter is hidden by his own model. Only then it is moved, along the actor->Link line, to the shield
// face in front of Link (at shield height); a contact already in front is untouched.
inline constexpr float kShieldFront = 28.0f;            // the guard face in front of Link's origin, toward the attacker
inline constexpr float kShieldYMin = 55.0f, kShieldYMax = 135.0f;
struct V3 { float x = 0, y = 0, z = 0; };
struct Spark { bool moved = false; V3 p; float proj = 0; };
inline float fsq(float v) noexcept { return v * v; }
inline Spark spark_point(V3 link, V3 enemy, V3 hit) noexcept {
  Spark o;
  o.p = hit;
  float dx = enemy.x - link.x, dz = enemy.z - link.z;
  const float d2 = dx * dx + dz * dz;
  if (!(d2 > 1.0f)) return o;
  float inv = 1.0f, d = d2;  // sqrt by Newton (no <cmath> in this header)
  for (int i = 0; i < 20; ++i) d = 0.5f * (d + d2 / d);
  inv = 1.0f / d, dx *= inv, dz *= inv;
  o.proj = (hit.x - link.x) * dx + (hit.z - link.z) * dz;
  if (o.proj >= kShieldFront) return o;  // already on the attacker's side of the guard face: native position
  o.moved = true;
  o.p.x = link.x + dx * kShieldFront, o.p.z = link.z + dz * kShieldFront;
  const float y = hit.y - link.y;
  o.p.y = link.y + (y < kShieldYMin ? kShieldYMin : y > kShieldYMax ? kShieldYMax : y);
  return o;
}
}  // namespace f6
}  // namespace kk
