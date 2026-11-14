// LIGHT FADES TWICE 0.13.9 DKN01: deterministic laboratory of the pure Darknut core (src/dkn_core.hpp). No game, no SDK.
// It prints the rhythm matrix (8 combos, 46 impacts, tick-exact simulation of the adapter's play model against the user's recordings) and fails on any
// broken rule. Build: g++ -std=c++20 -I../src tests/dkn_lab.cpp  (CI: MSVC /W4, target post05_dkn_lab).
#include "../src/dkn_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <initializer_list>

static int g_fail = 0, g_checks = 0;
#define CHECK(c, ...)                                   \
  do {                                                  \
    ++g_checks;                                         \
    if (!(c)) {                                         \
      ++g_fail;                                         \
      std::printf("FAIL %s:%d ", __FILE__, __LINE__);   \
      std::printf(__VA_ARGS__);                         \
      std::printf("\n");                                \
    }                                                   \
  } while (0)

using namespace dkn;

// ---------------------------------------------------------------- the adapter's play model, tick by tick
struct SimOut {
  unsigned n = 0;
  double tick[9] = {};
  unsigned move_of[9] = {};
  bool ok = true;
  float b1_last_frame = -1;  // the last frame B1 ever reached (must stay before the cut-2 window)
  float worst_cut_gap = 0;   // |frame at the switch - planned cut|: the previous clip really ends at its planned frame
  unsigned frozen = 0;       // ticks of a running combo with (almost) no motion: a held pose would read as a freeze
};
static SimOut simulate(const Combo& c, const Plan& p) {
  SimOut o;
  const Timeline tl = timeline(p);
  unsigned seg = 0;
  float frame = p.seg[0].start;  // executeAttack PRE of tick 0: the first clip at its start frame
  for (unsigned t = 0; t < 400; ++t) {
    // executeAttack POST of tick t: the frame the timeline wants after this tick's play, the switch if the segment changed, then the rate
    unsigned ts = 0;
    float want = 0;
    const bool on = locate(p, tl, static_cast<float>(t + 1), ts, want);
    if (ts != seg) {
      const float gap = frame - p.seg[seg].cut;
      if ((gap < 0 ? -gap : gap) > o.worst_cut_gap) o.worst_cut_gap = gap < 0 ? -gap : gap;
      seg = ts;
      frame = switch_frame(p.seg[seg], static_cast<float>(t) - tl.T[seg]);
    }
    float rate = want - frame;
    rate = rate < 0 ? 0 : rate > kTickRateMax ? kTickRateMax : rate;
    if (on && rate < 0.25f) ++o.frozen;
    const float prev = frame;
    frame += rate;  // action(): mpModelMorf2->play()
    if (p.seg[seg].m == Move::F2B1) o.b1_last_frame = frame;
    const MoveInfo& mi = info(p.seg[seg].m);
    for (unsigned k = 0; k < mi.impacts; ++k)
      if (prev < mi.impact[k] && frame >= mi.impact[k] && o.n < 9) o.tick[o.n] = t, o.move_of[o.n] = seg, ++o.n;
    if (!on) break;
  }
  o.ok = o.n == c.n_impacts;
  return o;
}

int main() {
  // ---------------------------------------------------------------- 1. tables
  CHECK(policy::kHp1Max == 420 && policy::kRes2Max == 480 && policy::kPosture1Max == 120 && policy::kPosture2Max == 130, "approved maxima");
  CHECK(policy::kRegenDelayTicks == 60 && policy::kBreakTicks == 90 && policy::kBreakHits == 3, "approved timings");
  CHECK(policy::kPassiveShieldTicks == 38 && policy::kPassiveIdleTicks == 75, "passivity thresholds");
  CHECK(dk_parry_gain(Move::F1B, 0) == 11 && dk_parry_gain(Move::F1A, 0) == 9 && dk_parry_gain(Move::F1Shield, 0) == 14, "Table A phase 1");
  CHECK(dk_parry_gain(Move::F2A, 0) == 11 && dk_parry_gain(Move::F2B, 0) == 9 && dk_parry_gain(Move::F2B, 1) == 9 && dk_parry_gain(Move::F2B1, 0) == 11 &&
            dk_parry_gain(Move::F2C, 0) == 15, "Table A phase 2");
  struct Tb { Move m; int b, d; } tb[] = {{Move::F1B, 26, 36}, {Move::F1A, 20, 30}, {Move::F1Shield, 42, 50}, {Move::F2A, 23, 33}, {Move::F2B, 12, 22}, {Move::F2B1, 23, 33}, {Move::F2C, 38, 45}};
  for (const Tb& x : tb) {
    CHECK(link_gain(x.m, Outcome::Block) == x.b && link_gain(x.m, Outcome::Direct) == x.d && link_gain(x.m, Outcome::Parry) == 0, "Table B %s", name(x.m));
  }
  // ---------------------------------------------------------------- 2. combos: inventory and exact posture sums
  unsigned total = 0, p1 = 0, p2 = 0;
  const int exp_dk[8] = {51, 33, 51, 51, 65, 69, 91, 55};
  const int exp_link[8] = {118, 78, 118, 118, 95, 117, 163, 108};
  for (unsigned ci = 0; ci < 8; ++ci) {
    const Combo& c = kCombos[ci];
    CHECK(impacts_of(c) == c.n_impacts, "%s impact count", c.name);
    total += c.n_impacts, (c.phase2 ? p2 : p1) += c.n_impacts;
    int dk = 0, lk = 0;
    for (unsigned m = 0; m < c.n_moves; ++m) {
      CHECK(info(c.moves[m]).phase2 == c.phase2, "%s move phase", c.name);
      for (unsigned k = 0; k < info(c.moves[m]).impacts; ++k) dk += dk_parry_gain(c.moves[m], k), lk += link_gain(c.moves[m], Outcome::Block);
    }
    const int mx = c.phase2 ? policy::kPosture2Max : policy::kPosture1Max;
    CHECK(dk == exp_dk[ci], "%s all-parry Darknut sum %d != %d", c.name, dk, exp_dk[ci]);
    CHECK(lk == exp_link[ci], "%s all-block Link sum %d != %d", c.name, lk, exp_link[ci]);
    std::printf("POSTURE %s parries=%d/%d (%.1f%%) blocks_link_theoretical=%d\n", c.name, dk, mx, 100.0 * dk / mx, lk);
    // Link saturation: blocks from 0, capped at 100; the impact that saturates
    int lp = 0, sat = -1, idx = 0;
    for (unsigned m = 0; m < c.n_moves; ++m)
      for (unsigned k = 0; k < info(c.moves[m]).impacts; ++k) {
        ++idx;
        lp += link_gain(c.moves[m], Outcome::Block);
        if (lp >= policy::kLinkMax && sat < 0) sat = idx;
        if (lp > policy::kLinkMax) lp = policy::kLinkMax;
      }
    if (ci == 6) CHECK(sat == 7, "F2-C3 Link saturates at impact %d (expected 7)", sat);
    if (exp_link[ci] >= 100) CHECK(sat > 0, "%s saturates", c.name);
  }
  CHECK(total == 46 && p1 == 18 && p2 == 28, "46 impacts (18 + 28), got %u", total);
  // the two cuts of a B are two independent opportunities; F2-C2 picks 2-3 are two DIFFERENT thrusts
  CHECK(kCombos[5].moves[1] == Move::F2A && kCombos[5].moves[2] == Move::F2A, "F2-C2: two separate thrusts at ~0.21 s");
  CHECK(kCombos[7].moves[3] == Move::F2A && kCombos[7].moves[0] == Move::F2C, "F2-C4 starts with the kick and ends with a thrust A (not B1)");
  CHECK(kCombos[4].moves[3] == Move::F2B1, "F2-C1 ends with B1");

  // ---------------------------------------------------------------- 3. rhythm: plan + tick simulation against the recordings
  std::printf("\nRHYTHM MATRIX (30 Hz; interval error = simulated - audio, ticks; cumulative error from the first impact)\n");
  double worst = 0, worst_cum = 0;
  unsigned clamped = 0;
  for (unsigned ci = 0; ci < 8; ++ci) {
    const Combo& c = kCombos[ci];
    const Plan p = plan(c);
    const SimOut o = simulate(c, p);
    CHECK(o.ok, "%s simulated %u impacts of %u", c.name, o.n, c.n_impacts);
    std::printf("%s:", c.name);
    for (unsigned m = 0; m < p.n; ++m)
      std::printf(" [%s s=%.1f cut=%.1f in=%.2f mid=%.2f out=%.2f]", name(p.seg[m].m), p.seg[m].start, p.seg[m].cut, p.seg[m].rate_in, p.seg[m].rate_mid, p.seg[m].rate_out);
    std::printf("\n");
    for (unsigned j = 0; j + 1 < o.n; ++j) {
      const double audio = (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03;
      const double sim = o.tick[j + 1] - o.tick[j];
      const double cum = (o.tick[j + 1] - o.tick[0]) - (c.peak_ms[j + 1] - c.peak_ms[0]) * 0.03;
      const double err = sim - audio;
      if (std::fabs(err) > worst) worst = std::fabs(err);
      if (std::fabs(cum) > worst_cum) worst_cum = std::fabs(cum);
      if (p.clamped[j]) ++clamped;
      std::printf("   pair %u-%u audio=%.3fs (%.2f t) sim=%.0f t err=%+.2f t cum=%+.2f t%s\n", j + 1, j + 2, (c.peak_ms[j + 1] - c.peak_ms[j]) / 1000.0, audio, sim, err, cum,
                  p.clamped[j] ? " RATE_CLAMPED" : "");
      CHECK(std::fabs(err) <= 1.0 + 1e-6, "%s pair %u: |err| %.2f > 1 tick", c.name, j + 1, err);
      CHECK(std::fabs(cum) <= 2.0 + 1e-6, "%s pair %u: cumulative |err| %.2f > 2 ticks", c.name, j + 1, cum);
    }
    for (unsigned m = 0; m < p.n; ++m) {
      CHECK(p.seg[m].rate_in >= kRateMin && p.seg[m].rate_in <= kRateMax && p.seg[m].rate_out >= kRateMin && p.seg[m].rate_out <= kRateMax &&
                p.seg[m].rate_mid >= kRateMin && p.seg[m].rate_mid <= kRateMax, "%s rate range", c.name);
      const MoveInfo& mi = info(p.seg[m].m);
      CHECK(p.seg[m].start >= mi.s_min - 1e-4f && p.seg[m].start <= mi.s_max + 1e-4f, "%s start frame in range", c.name);
      if (p.seg[m].cut >= 0) CHECK(p.seg[m].cut > mi.impact[mi.impacts - 1], "%s cut after the last impact", c.name);
    }
    CHECK(o.frozen == 0, "%s: %u frozen ticks inside the combo", c.name, o.frozen);
    CHECK(o.worst_cut_gap <= 2.6f, "%s: a move switched %.2f frames away from its planned cut", c.name, o.worst_cut_gap);
    if (ci == 4) CHECK(o.b1_last_frame >= 0 && o.b1_last_frame < info(Move::F2B).at_open[1] + 1.5f && o.b1_last_frame < info(Move::F2B).impact[1],
                       "B1 never reaches the second cut (last frame %.2f)", o.b1_last_frame);
  }
  CHECK(clamped == 0, "%u intervals need a rate outside [%.2f, %.2f]", clamped, kRateMin, kRateMax);
  std::printf("RHYTHM worst pair error %.2f ticks, worst cumulative %.2f ticks, clamped %u\n\n", worst, worst_cum, clamped);

  // ---------------------------------------------------------------- 4. posture model
  {
    Posture p;
    p.reset(policy::kPosture1Max);
    for (int i = 0; i < 10; ++i) CHECK(p.gain(11) == Gain::Added, "gain below max");
    CHECK(p.points() == 110 && p.gain(11) == Gain::Reached && p.points() == 120 && p.latched, "saturation at 120");
    CHECK(p.gain(5) == Gain::Ignored && p.points() == 120, "no gain while latched");
    for (unsigned t = 0; t < 300; ++t) p.regen(10);
    CHECK(p.points() == 120 && p.latched, "no regeneration while broken (300 ticks)");
    p.reset(policy::kPosture2Max);
    CHECK(p.points() == 0 && p.max_pts == 130, "reset to 0/130");
    p.gain(30);
    for (unsigned t = 0; t < 60; ++t) p.regen(12);
    CHECK(p.points() == 30, "60 eligible ticks of delay");
    for (unsigned t = 0; t < 30; ++t) p.regen(12);
    CHECK(p.points() == 18, "12 pts/s -> 30 ticks = 12 points (got %d)", p.points());
    p.gain(1);
    CHECK(!p.regen(12) && p.since_gain == 1, "a genuine gain restarts the delay");
    // bands: borders belong to the lower band, both phases
    CHECK(band(420, 420) == 0 && band(316, 420) == 0 && band(315, 420) == 1 && band(211, 420) == 1 && band(210, 420) == 2 && band(105, 420) == 3 && band(42, 420) == 4 &&
              band(43, 420) == 3 && band(0, 420) == 4, "phase-1 bands");
    CHECK(regen_rate(false, 420, 420) == 10 && regen_rate(false, 300, 420) == 7 && regen_rate(false, 200, 420) == 4 && regen_rate(false, 60, 420) == 2 && regen_rate(false, 40, 420) == 1,
          "phase-1 regen table");
    CHECK(regen_rate(true, 480, 480) == 12 && regen_rate(true, 360, 480) == 8 && regen_rate(true, 200, 480) == 5 && regen_rate(true, 100, 480) == 2 && regen_rate(true, 48, 480) == 1,
          "phase-2 regen table");
    CHECK(res2_remaining(0) == 480 && res2_remaining(500) == 0 && res2_remaining(120) == 360, "phase-2 remaining");
  }
  // ---------------------------------------------------------------- 5. HP 420 and the armour: the native setBreakPart model (mType 1)
  {
    unsigned short st[16] = {};
    int next = 0;
    auto broken = [&]() {
      int n = 0;
      for (int i = 0; i < 13; ++i) n += st[native::kBreakPartNo[i]] != 0;
      return n;
    };
    auto native_break = [&](int idx) {  // transcription of daB_TN_c::setBreakPart (S L420-456), mType 1
      if (idx >= 12) idx = 13;
      int v = idx - broken();
      if (v != 0)
        for (int i = 0; i <= 12; ++i)
          if (st[native::kBreakPartNo[i]] == 0) {
            st[native::kBreakPartNo[i]] = 2;
            if (native::kBreakPartNo[i] == 4) ++next, ++v;
            if (--v == 0) return;
          }
    };
    int hp = policy::kHp1Max, last_prog = 0, calls = 0, max_jump = 0;
    const int hits[] = {4, 8, 4, 4, 12, 3, 4, 8, 8, 4, 30, 4, 4, 4, 8, 50, 4, 4, 2, 4, 6, 4, 8, 8, 4, 4, 4, 30, 4, 4, 8, 4, 8, 4, 4, 4, 30, 30, 4, 4, 4, 30, 4, 8, 8};
    for (unsigned it = 0; it < 400 && hp > 0; ++it) {
      const int d = hits[it % (sizeof(hits) / sizeof(hits[0]))];
      hp -= d;
      if (hp < 0) hp = 0;
      const int prog = armor_progress(hp);
      CHECK(prog >= last_prog, "armour progress monotonic");
      if (hp > 0) CHECK(prog <= 11, "never 12 while HP remains (hp %d prog %d)", hp, prog);
      const int b0 = broken();
      const int call = armor_call(prog, b0);
      if (call >= 0) {
        CHECK(call > b0, "never a call below the broken count");
        native_break(call), ++calls;
        if (broken() - b0 > max_jump) max_jump = broken() - b0;
      }
      next = armor_next(prog, broken());
      CHECK(next == broken() || (next > broken() && next == prog), "mNextBreakPart consistent with the parts (next %d broken %d)", next, broken());
      if (hp > 0) CHECK(next <= 11, "no native transition before HP 0");
      last_prog = prog;
    }
    CHECK(hp == 0 && armor_progress(0) == 12, "the reserve ends exactly at 0 -> 12");
    native_break(12);  // executeZakoChangeDemo mode 0: setBreakPart(12) -> 13
    CHECK(broken() == 13, "the native transition breaks the rest");
    std::printf("ARMOR hits=%zu setBreakPart_calls=%d max_parts_per_call=%d\n", sizeof(hits) / sizeof(hits[0]), calls, max_jump);
    CHECK(max_jump <= 3, "no multiple unjustified detachment");
    // the dangerous native case is never produced: progress 9 broke 10 parts (head bump), progress 9/10 must not call again
    CHECK(armor_call(9, 10) == -1 && armor_call(10, 10) == -1 && armor_call(11, 10) == 11, "head bump handled");
    CHECK(armor_progress(420) == 0 && armor_progress(385) == 1 && armor_progress(386) == 0 && armor_progress(35) == 11 && armor_progress(1) == 11, "35 HP per step");
  }
  // ---------------------------------------------------------------- 6. break controller and the held clip
  {
    Break b;
    b.latch(), b.begin();
    CHECK(b.st == Bk::Stagger && b.breaks == 1, "stagger");
    for (unsigned t = 1; t < 90; ++t) CHECK(!b.tick(), "window open at %u", t);
    CHECK(b.tick() && b.st == Bk::Recover && b.why == BkEnd::Timeout, "90 ticks");
    CHECK(!b.recover(false) && b.recover(true) && b.st == Bk::Combat, "recovery ends with the native clip");
    b.latch(), b.begin();
    for (unsigned t = 0; t < 20; ++t) b.tick();
    CHECK(!b.hit() && !b.hit() && b.ticks == 20, "hits never restart the window");
    CHECK(b.hit() && b.why == BkEnd::Hits, "3 unique hits end it early");
    Break w;
    w.latch(), w.begin();
    w.tick(), w.abort(BkEnd::Phase);
    CHECK(w.st == Bk::Combat && w.why == BkEnd::Phase, "a phase change preempts the stagger");
    // the stretched clip never ends before the window, never loops
    for (float end : {native::kEndTnaDamageL, native::kEndTnbDownSippu}) {
      float f = 0;
      unsigned stop_at = 0;
      for (unsigned age = 0; age < policy::kBreakTicks; ++age) {
        f += stagger_rate(age, f, end);
        if (f >= end && !stop_at) stop_at = age;
      }
      CHECK(stop_at == 0 && f >= end - 2.5f && f < end, "clip %.0f held to the end of the window (frame %.2f)", end, f);
    }
  }
  // ---------------------------------------------------------------- 7. Link passivity
  {
    Passive ps;
    Pv v = Pv::None;
    unsigned t = 0;
    for (; t < 60 && v == Pv::None; ++t) v = ps.step(Act::Passive, true, false);
    CHECK(v == Pv::Shield && t == 38, "shield passivity after 38 ticks (got %u)", t);
    ps.fire(100);
    CHECK(!ps.ready(339) && ps.ready(340), "independent cooldown");
    v = Pv::None;
    for (t = 0; t < 100 && v == Pv::None; ++t) v = ps.step(Act::Passive, false, false);
    CHECK(v == Pv::Idle && t == 75, "idle passivity after 75 ticks (got %u)", t);
    Passive q;
    for (t = 0; t < 30; ++t) q.step(Act::Passive, true, false);
    q.step(Act::Active, false, false);
    CHECK(q.shield_ticks == 0, "an attack / parry / dodge resets");
    for (t = 0; t < 30; ++t) q.step(Act::Passive, true, false);
    for (t = 0; t < 50; ++t) CHECK(q.step(Act::Hold, true, false) == Pv::None, "forced recovery holds");
    CHECK(q.shield_ticks == 30, "hold neither counts nor resets");
    for (t = 0; t < 50; ++t) q.step(Act::Passive, true, true);
    CHECK(q.shield_ticks == 30, "reading a Darknut attack is not passivity");
    CHECK(classify(native::kProcGuardAttack) == Act::Active && classify(native::kProcWait) == Act::Passive && classify(native::kProcGuardSlip) == Act::Hold && classify(0x21) == Act::Active,
          "procedure classes");
  }
  // ---------------------------------------------------------------- 8. combo selection 25/25/25/25 (conditional), provisional share
  {
    Selector s;
    s.rng.seed(0x1234);
    for (unsigned i = 0; i < 200000; ++i) s.pick(0);
    const double share = 100.0 * s.combos / s.rolls;
    CHECK(share > 39.0 && share < 41.0, "combo share %.2f%%", share);
    for (unsigned k = 0; k < 4; ++k) {
      const double pk = 100.0 * s.picked[k] / s.combos;
      CHECK(pk > 24.4 && pk < 25.6, "combo %u picked %.2f%%", k, pk);
      std::printf("SELECT combo %u: %.2f%%\n", k + 1, pk);
    }
    s.ended(1000);
    CHECK(s.pick(1000 + policy::kComboCooldownTicks - 1) == -1, "combo cooldown");
    Rng a, b;
    a.seed(7), b.seed(8);
    CHECK(a.next() != b.next(), "instances do not share a stream");
  }
  // ---------------------------------------------------------------- 9. impact ledger: one resolution per impact, B = two, B1 = one
  {
    Ledger l;
    l.open_window(Move::F2B, 0);
    CHECK(l.resolve() && !l.resolve(), "one contact per impact");
    l.close();
    l.open_window(Move::F2B, 1);
    CHECK(l.resolve(), "the second cut is a new opportunity");
    CHECK(window_of(Move::F2B, 14) == 0 && window_of(Move::F2B, 25) == 1 && window_of(Move::F2B, 39) == -1, "B windows");
    CHECK(window_of(Move::F2B1, 19.9f) == 0 && window_of(Move::F2B1, 20.0f) == -1 && window_of(Move::F2B1, 26.0f) == -1, "B1 has no second window");
    CHECK(window_of(Move::F1B, 26) == 0 && window_of(Move::F1A, 22) == 0, "mace windows hold the impact frames");
  }
  std::printf("\nDKN_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "LAB_FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
