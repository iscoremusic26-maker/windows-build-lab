// LIGHT FADES TWICE 0.13.0 CHILFOS01 core laboratory: src/kk_core.hpp only (pure logic). Every number below is the request's, or the recordings'
// measured onsets (CHILFOS01/RHYTHM_ANALYSIS.md); a balance value that was not requested is marked [BALANCE_PROVISIONAL] in the core.
// `--table` prints the recorded -> realizable rhythm table used by the documentation.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include "../src/kk_core.hpp"
static unsigned long long g_checks = 0;
#define CHECK(c)                                           \
  do {                                                     \
    ++g_checks;                                            \
    if (!(c)) {                                            \
      std::printf("KK_LAB_FAIL line=%d %s\n", __LINE__, #c); \
      std::exit(1);                                        \
    }                                                      \
  } while (0)
using namespace kk;
static void lab_patterns() {
  // A: the four combos, type and order exactly as requested; 26 melee contacts + 1 throw = 27 offensive actions
  const char* want[4] = {"DDDEED", "DEEEED", "EEEEDED", "DDEDEEE"};
  unsigned melee = 0;
  for (unsigned p = 0; p < kPatterns; ++p) {
    CHECK(std::strlen(want[p]) == kPattern[p].n);
    for (unsigned i = 0; i < kPattern[p].n; ++i) CHECK((kPattern[p].act[i] == K::D ? 'D' : 'E') == want[p][i]);
    for (unsigned i = 1; i < kPattern[p].n; ++i) CHECK(kPattern[p].onset[i] > kPattern[p].onset[i - 1]);
    melee += kPattern[p].n;
    CHECK(kPattern[p].ranged_tail == (p == 2));
  }
  CHECK(melee == 26 && total_actions() == 27);
  CHECK(kPattern[0].n == 6 && kPattern[1].n == 6 && kPattern[2].n == 7 && kPattern[3].n == 7);
  // D and E are two different clips; the default is one constant, and the runtime identity can swap it
  CHECK(bck_of(K::D) != bck_of(K::E));
  Identity id;
  CHECK(id.bck(K::D) == kBckDiagonal && id.bck(K::E) == kBckThrust && !id.verified);
  CHECK(!id.observe(5, Tip::Diagonal) && !id.observe(6, Tip::Thrust) && !id.observe(5, Tip::Diagonal));
  CHECK(id.observe(6, Tip::Thrust) && id.verified && !id.swapped && id.bck(K::D) == 5 && id.bck(K::E) == 6);
  Identity sw;
  sw.observe(5, Tip::Thrust), sw.observe(5, Tip::Thrust), sw.observe(6, Tip::Diagonal);
  CHECK(sw.observe(6, Tip::Diagonal) && sw.swapped && sw.bck(K::D) == 6 && sw.bck(K::E) == 5);
  Identity cf;
  cf.observe(5, Tip::Thrust), cf.observe(5, Tip::Thrust), cf.observe(6, Tip::Thrust), cf.observe(6, Tip::Thrust);
  CHECK(!cf.verified && cf.conflict && cf.bck(K::D) == kBckDiagonal);  // two thrusts: nothing is swapped on contradictory evidence
  Identity mixed;
  for (int i = 0; i < 3; ++i) mixed.observe(5, Tip::Diagonal), mixed.observe(5, Tip::Thrust), mixed.observe(6, Tip::Thrust);
  CHECK(!mixed.verified);  // a clip with mixed votes never settles
  CHECK(!id.observe(9, Tip::Thrust));
  // tip classifier
  CHECK(classify_tip(0, 0, 150) == Tip::Thrust && classify_tip(120, -110, 20) == Tip::Diagonal && classify_tip(10, 5, 12) == Tip::Unsure && classify_tip(60, 0, 80) == Tip::Unsure);
  float lx = 0, lz = 0;
  local_of(0, 100, 0, lx, lz);
  CHECK(lz > 99.9f && lx < 0.1f && lx > -0.1f);
  local_of(100, 0, 0x4000, lx, lz);  // yaw 90 deg: world +X is forward
  CHECK(lz > 99.9f && lx < 0.1f && lx > -0.1f);
}
static void lab_rhythm() {
  // B: every planned interval within one tick of the recording, except the bursts no two distinct native windows can reach
  float worst_non_burst = 0;
  for (unsigned p = 0; p < kPatterns; ++p) {
    const Plan pl = make_plan(p);
    CHECK(pl.n == kPattern[p].n);
    for (unsigned i = 0; i < pl.n; ++i) {
      const ActPlan& a = pl.a[i];
      CHECK(a.bck == bck_of(a.kind));
      CHECK(a.rate == policy::kRateD || a.rate == policy::kRateE || a.rate == policy::kRateEBurst || a.rate == policy::kRateDFast);
      CHECK((a.kind == K::E) == (a.rate == policy::kRateE || a.rate == policy::kRateEBurst));
      CHECK(a.burst == (a.rate == policy::kRateEBurst));
      CHECK(a.window >= 6 && a.window <= 8);  // the whole native 24..33 window at the clip's rate
      const Clip c{a.start, a.rate};
      CHECK(c.open_tick() >= 1 && a.start <= float(policy::kAtOpenFrame - (a.burst ? policy::kBurstPrep : policy::kMinPrep)));
      if (i == 0) {
        CHECK(a.start == 0 && a.gap == 0);
        continue;
      }
      CHECK(a.open > pl.a[i - 1].open + pl.a[i - 1].window);  // never merged with the previous window: at least one closed Execute
      const float d = a.achieved_s - a.recorded_s;
      if (a.burst && pl.a[i - 1].kind == K::E && a.recorded_s < policy::kBurstMax) {
        CHECK((d <= 1.0f / 30.0f + 0.001f && d >= -1.0f / 30.0f - 0.001f) || (d > 0.0f && a.achieved_s <= 0.34f));  // within a tick, or the realizable minimum of two distinct burst windows (10 ticks)
      } else {
        CHECK(d <= 1.0f / 30.0f + 0.001f && d >= -1.0f / 30.0f - 0.001f);
        worst_non_burst = d > worst_non_burst ? d : -d > worst_non_burst ? -d : worst_non_burst;
      }
    }
  }
  CHECK(worst_non_burst < 0.034f);
  // the documented physical limits
  CHECK(make_plan(3).a[5].achieved_s > 0.33f && make_plan(3).a[5].recorded_s < 0.14f);  // C4 5->6: 0.137 recorded, 0.333 realizable
  CHECK(make_plan(0).a[4].burst && make_plan(0).a[3].burst);                             // C1 4-5 burst pair
  CHECK(make_plan(2).a[1].burst && make_plan(2).a[3].burst && !make_plan(2).a[5].burst);  // C3 1-2, 3-4 bursts; 6th ordinary
  // C3 tail: backstep, throw from frame 0 at x1.45, release on the recorded mark
  const Plan c3 = make_plan(2);
  CHECK(c3.tail.on && c3.tail.back_ticks >= 1 && !make_plan(0).tail.on);
  const float dt = c3.tail.achieved_s - c3.tail.recorded_s;
  CHECK(dt < 0.034f && dt > -0.034f);
  // clip arithmetic: x1.5 from 0 reads 24.0 exactly at tick 16; x1.25 from 0 opens at 20 and closes at 27
  CHECK((Clip{0.0f, 1.5f}).open_tick() == 16 && (Clip{0.0f, 1.25f}).open_tick() == 20 && (Clip{0.0f, 1.25f}).close_tick() == 27);
}
static void lab_crossing() {
  // B/C: the native `(s32)frame == N` events, accelerated: exactly once whatever the rate, never twice
  for (float rate : {1.25f, 1.30f, 1.40f, 1.45f, 1.50f, 1.60f}) {
    for (float start = 0; start <= 18.0f; start += 0.5f) {
      std::uint32_t fired = 0;
      unsigned count[4] = {};
      float f = start, last = start - rate;
      for (int t = 0; t < 120 && f < 70.0f; ++t) {
        const Cross c = crossing(last, f, kThrowEvents, 4, fired);
        CHECK(!c.duplicate);
        float seen = f;
        if (c.snap) seen = c.frame, fired |= 1u << c.event;
        for (unsigned i = 0; i < 4; ++i)
          if (int(seen) == kThrowEvents[i]) ++count[i], fired |= 1u << i;
        last = seen;
        f += rate;
      }
      for (unsigned i = 0; i < 4; ++i)
        if (float(kThrowEvents[i]) > start) CHECK(count[i] == 1);  // the spear spawn (23) among them: exactly one
    }
  }
  // the spear release frame itself: listed, and read exactly once for every start and rate (one spear per throw)
  bool listed = false;
  for (int e : kThrowEvents) listed = listed || e == policy::kThrowSpawnFrame;
  CHECK(listed && policy::kThrowSpawnFrame == 23 && policy::kAtOpenFrame == 24 && kAttackEvents[0] == 24);
  for (float rate : {1.0f, 1.25f, 1.45f, 1.60f}) {
    for (float start = 0; start <= 20.0f; start += 0.25f) {
      std::uint32_t fired = 0;
      int reads23 = 0;
      float f = start, last = start - rate;
      for (int t = 0; t < 200 && f < 70.0f; ++t) {
        const Cross c = crossing(last, f, kThrowEvents, 4, fired);
        float seen = c.snap ? c.frame : f;
        if (c.snap) fired |= 1u << c.event;
        for (unsigned i = 0; i < 4; ++i)
          if (int(seen) == kThrowEvents[i]) fired |= 1u << i;
        reads23 += int(seen) == 23;
        last = seen;
        f += rate;
      }
      if (start < 23.0f) CHECK(reads23 == 1);
    }
  }
  // low fps / a stalled frame: the same integer read twice after the event fired is flagged, never snapped again
  const Cross d = crossing(23.0f, 23.5f, kThrowEvents, 4, 0x2);
  CHECK(d.duplicate && !d.snap);
}
static void lab_posture() {
  // F: gains, exact latch at 100 once, never above
  Posture p;
  CHECK(!p.gain(policy::kGainLinkParry, 1) && p.points() == 13);
  CHECK(!p.gain(policy::kGainSwordHp, 2) && p.points() == 18);
  CHECK(!p.gain(policy::kGainBlock, 3) && p.points() == 21);
  CHECK(!p.gain(policy::kGainEnemyParrySelf, 4) && p.points() == 21);
  int latched = 0;
  for (int i = 0; i < 20; ++i) latched += p.gain(policy::kGainLinkParry, 10 + i) ? 1 : 0;
  CHECK(latched == 1 && p.latched && p.points() == 100);
  for (int t = 0; t < 200; ++t) p.tick(340, true);
  CHECK(p.points() == 100);  // never regenerates while latched
  p.reset();
  CHECK(p.points() == 0 && !p.latched);
  // regeneration bands by current HP: exact boundaries of the request
  CHECK(regen_per_second(340) == 12 && regen_per_second(256) == 12 && regen_per_second(255) == 8 && regen_per_second(171) == 8 && regen_per_second(170) == 5);
  CHECK(regen_per_second(86) == 5 && regen_per_second(85) == 2 && regen_per_second(35) == 2 && regen_per_second(34) == 1 && regen_per_second(1) == 1 && regen_per_second(0) == 0);
  // delay 60 eligible ticks, then exactly n points per 30 ticks, no drift over a minute
  for (int hp : {340, 200, 100, 50, 10}) {
    Posture q;
    q.gain(60, 0);
    for (int t = 0; t < 60; ++t) q.tick(hp, true);
    CHECK(q.points() == 60);
    for (int t = 0; t < 30; ++t) q.tick(hp, true);
    CHECK(q.points() == 60 - regen_per_second(hp));
    for (int t = 0; t < 30 * 4; ++t) q.tick(hp, true);
    CHECK(q.points() == (60 - 5 * regen_per_second(hp) > 0 ? 60 - 5 * regen_per_second(hp) : 0));
  }
  // a genuine gain restarts the delay; ineligible ticks (native reaction) do not count; HP 0 never regenerates
  Posture r;
  r.gain(30, 0);
  for (int t = 0; t < 59; ++t) r.tick(300, true);
  r.gain(5, 59);
  for (int t = 0; t < 59; ++t) r.tick(300, true);
  CHECK(r.points() == 35);
  for (int t = 0; t < 100; ++t) r.tick(300, false);
  CHECK(r.points() == 35);
  for (int t = 0; t < 200; ++t) r.tick(0, true);
  CHECK(r.points() == 35);
}
static void lab_defense() {
  // H: pressure levels A/B/C, two blocks -> 80%, one roll per swing, no parry without the spear / while attacking / from the flank / in cooldown
  Rng rng;
  rng.seed(42);
  Defense d;
  CHECK(d.level(100) == 0);
  for (int i = 0; i < 3; ++i) d.swing(100 + i * 10);
  CHECK(d.level(130) == 1);
  for (int i = 0; i < 3; ++i) d.swing(140 + i * 10);
  d.hits_since_defense = 2;
  CHECK(d.level(170) == 2);
  // FIX05 pressure parity: four aimed swings, no wounds required, cooldown by level.
  for (int n = 0; n <= 4; ++n) {
    Defense pressure;
    for (int i = 0; i < n; ++i) pressure.swing(100 + i);
    pressure.on_enemy_parry(110);
    CHECK(pressure.cooldown_until == 110u + (n >= 4 ? 60u : n >= 3 ? 90u : 120u));
  }
  Defense e;
  CHECK(e.on_swing(10, 1, false, false, true, false, rng) == Np::NoSpear);
  CHECK(e.on_swing(11, 2, true, true, true, false, rng) == Np::Attacking);
  CHECK(e.on_swing(12, 3, true, false, false, false, rng) == Np::Flank);
  CHECK(e.on_swing(13, 4, true, false, true, true, rng) == Np::Broken);
  e.cooldown_until = 100;
  CHECK(e.on_swing(14, 5, true, false, true, false, rng) == Np::Cooldown);
  // the 12/35/45/80 percentages, measured over 20000 rolls each (+-2 points)
  struct Case { int level; bool pair; int pct; } cases[] = {{0, false, 12}, {1, false, 35}, {2, false, 45}, {0, true, 80}};
  for (auto c : cases) {
    unsigned hits = 0;
    for (int i = 0; i < 20000; ++i) {
      Defense x;
      std::uint64_t now = 1000;
      if (c.level >= 1) for (int k = 0; k < 2; ++k) x.swing(now - 20 + k);
      if (c.level == 2) {
        for (int k = 0; k < 3; ++k) x.swing(now - 100 + k);
        x.hits_since_defense = 2;
      }
      if (c.pair) x.on_block(now - 10, 7), x.on_block(now - 5, 8);
      hits += x.on_swing(now, 99, true, false, true, false, rng) == Np::None ? 1u : 0u;
    }
    const int pct = int(hits * 100 / 20000);
    CHECK(pct >= c.pct - 2 && pct <= c.pct + 2);
  }
  // the pair is consumed by the continuation it served, and needs two DIFFERENT swings
  Defense f;
  f.on_block(10, 3), f.on_block(12, 3);
  CHECK(!f.two_blocks(14));
  f.on_block(20, 4);
  CHECK(f.two_blocks(22) && !f.two_blocks(20 + policy::kBlockPairWindow + 1));
  (void)f.on_swing(22, 9, true, false, true, false, rng);
  CHECK(f.nblocks == 0);
  // presenting is for that swing only, and expires
  Defense g;
  g.prepared = true, g.prepared_serial = 5, g.prepared_until = 20;
  CHECK(g.presenting(15, 5) && !g.presenting(15, 6) && !g.presenting(21, 5));
  g.expire(21);
  CHECK(!g.prepared);
  // wound-prepared guard: 45/80/100 after 1/2/3 consecutive wounds, only after the ready delay, forgotten after 90 ticks
  Defense w;
  CHECK(w.block_pct(0) == policy::kBlockBase);
  w.on_wound(100);
  CHECK(w.block_pct(101) == policy::kBlockBase && w.block_pct(100 + policy::kWoundReadyDelay) == 45);
  w.on_wound(110);
  CHECK(w.block_pct(120) == 80);
  w.on_wound(120);
  CHECK(w.block_pct(130) == 100);
  CHECK(w.block_pct(120 + policy::kWoundMemory + 1) == policy::kBlockBase);
  w.on_block(131, 3);
  CHECK(w.wound_streak == 0 && w.block_pct(132) == policy::kBlockBase);
  // after an enemy parry: cooldown, memory cleared
  Defense h;
  h.on_block(1, 1), h.on_block(2, 2), h.on_wound(3);
  h.on_enemy_parry(10);
  CHECK(h.cooldown_until == 10 + policy::kParryCooldown && h.nblocks == 0 && h.wound_streak == 0 && !h.prepared);
}
static void lab_verdict_and_break() {
  // I: one verdict per swing; G: the stagger, its two finishers, the crossing swing never kills
  SwingVerdict v;
  v.set(4, V::Block);
  CHECK(v.of(4) == V::Block && v.of(5) == V::None);
  Break b;
  CHECK(!b.active());
  b.latch(10);
  CHECK(b.st == Bk::Pending && b.latch_serial == 10);
  b.latch(11);
  CHECK(b.latch_serial == 10);  // the first latch wins
  b.enter(100);
  CHECK(b.active() && !b.sword_finishes(10) && !b.sword_finishes(9) && b.sword_finishes(11));
  CHECK(!b.helm_finishes(true));
  b.shield_attack();
  CHECK(b.shield_prep && b.helm_finishes(true) && !b.helm_finishes(false));
  const std::uint64_t start = b.start;
  b.shield_attack();
  CHECK(b.start == start);  // a Shield Attack never renews the window
  CHECK(!b.expired(100 + policy::kBreakTicks - 1) && b.expired(100 + policy::kBreakTicks));
  b.reset();
  CHECK(b.st == Bk::None && !b.active());
  // E: classification
  CHECK(classify({true, 0, false}) == Excl::None && classify({true, 3, false}) == Excl::None && classify({true, 2, false}) == Excl::None);
  CHECK(classify({true, 1, false}) == Excl::Spear && classify({true, 2, true}) == Excl::HiddenIronBall && classify({true, 7, false}) == Excl::UnknownSubtype && classify({false, 0, false}) == Excl::Profile);
  CHECK(policy::kMaxHp == 340 && policy::kNativeHp == 140 && policy::kMaxPosture == 100 && policy::kBreakTicks == 90 && policy::kRegenDelayTicks == 60);
  CHECK(policy::kLinkBlock == 20 && policy::kLinkDirect == 30 && policy::kLinkEnemyParry == 25 && policy::kGainIron == 30);
  CHECK(angle_diff(0x7FFF, -0x7FFF) == 2 && angle_diff(0, 0x3000) == 0x3000);
}
static void lab_rng() {
  Rng r;
  r.seed(7);
  unsigned c[4] = {};
  for (int i = 0; i < 40000; ++i) ++c[r.below(4)];
  for (unsigned x : c) CHECK(x > 9400 && x < 10600);  // 25% each combo
  Rng a, b;
  a.seed(9), b.seed(9);
  for (int i = 0; i < 100; ++i) CHECK(a.next() == b.next());
}
static void print_table() {
  std::printf("pattern act kind bck rate start_frame gap_ticks recorded_s achieved_s diff_s window_ticks burst\n");
  for (unsigned p = 0; p < kPatterns; ++p) {
    const Plan pl = make_plan(p);
    for (unsigned i = 0; i < pl.n; ++i) {
      const ActPlan& a = pl.a[i];
      std::printf("C%u %u %c %d %.2f %.0f %u %.3f %.3f %+.3f %u %d\n", p + 1, i + 1, a.kind == K::D ? 'D' : 'E', a.bck, a.rate, a.start, a.gap, a.recorded_s, a.achieved_s,
                  i ? a.achieved_s - a.recorded_s : 0.0f, a.window, a.burst ? 1 : 0);
    }
    if (pl.tail.on) std::printf("C%u tail backstep_ticks=%u throw_rate=%.2f spawn_frame=23 recorded_s=%.3f achieved_s=%.3f diff_s=%+.3f restore_rate=%.2f\n", p + 1, pl.tail.back_ticks, policy::kRateThrow,
                                pl.tail.recorded_s, pl.tail.achieved_s, pl.tail.achieved_s - pl.tail.recorded_s, policy::kRateRestore);
  }
}
int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "--table") == 0) {
    print_table();
    return 0;
  }
  lab_patterns();
  lab_rhythm();
  lab_crossing();
  lab_posture();
  lab_defense();
  lab_verdict_and_break();
  lab_rng();
  std::printf("KK_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PURE_CHILFOS_CORE runtime=HUMAN_RUNTIME_PENDING\n", kVersion, g_checks);
  return 0;
}
