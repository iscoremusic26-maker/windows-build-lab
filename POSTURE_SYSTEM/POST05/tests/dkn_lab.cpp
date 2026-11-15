// LIGHT FADES TWICE 0.13.10 DKN01 FIX01 (base 0.13.9): deterministic laboratory of the pure Darknut core (src/dkn_core.hpp). No game, no SDK.
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
// 0.13.10 (c1020a7) plan fingerprint: FIX02 must not change any combo / single timing (user rule).
#define TIMING_FINGERPRINT_0_13_10 145463.5789  // computed with the c1020a7 dkn_core.hpp (FIX02 evidence: dkn_fix02_evidence) = 0.13.11 dc5fe6d
// FIX03: phase 1 is FROZEN. These four numbers were computed with the 0.13.11 dkn_core.hpp (dc5fe6d) by the same formulas (dkn_fix03_evidence):
#define F1_PLAN_FINGERPRINT_0_13_11 58041.6504   // the plans of F1-C1..C4 + F1-SINGLE-A/B (cuts, starts, rates, morphs, scales)
#define F1_MOVES_FINGERPRINT_0_13_11 21162.0     // the three phase-1 MoveInfo (impacts, AT windows, frame ranges, clip ends, morphs, clips, native action/mode)
#define FIX02_PLANS_BUT_F2C4_0_13_11 129229.2156 // the 0.13.11 fingerprint minus F2-C4 (16234.3633: its kick cut range is the ONE FIX03 move change)
#define F1_COMBOS_FINGERPRINT_0_13_11 386376.0  // the four F1 combos (recorded peaks + move sequences)
// FIX04: the F2 MoveInfo of 0.13.12 (00b3ce6). FIX04 changes ONLY s_min of A/B/B1 (-> 1) and c_max of A (24 -> 35), B (50 -> 54), C (26 -> 28).
struct OldRange { Move m; float s_min, s_max, c_min, c_max, end; };
static const OldRange kFix03Ranges[4] = {{Move::F2A, 3, 6, 12, 24, 36}, {Move::F2B, 5, 11, 33, 50, 55}, {Move::F2B1, 5, 11, 20, 20, 55}, {Move::F2C, 0, 3, 12, 26, 29}};
// FIX02 (0.13.11) and FIX03 (0.13.12) simulated F2 impact intervals (ticks), from IMPLEMENTATION_0_13_12.md (tests of those versions).
static const int kFix02Iv[4][8] = {{8, 9, 8, 9, 9, 25}, {27, 6, 34, 8, 9, 9}, {20, 6, 25, 19, 7, 24, 10, 10}, {27, 24, 6, 30}};
static const int kFix03Iv[4][8] = {{10, 11, 10, 11, 12, 31}, {33, 8, 42, 11, 11, 11}, {25, 8, 31, 24, 8, 30, 12, 13}, {33, 30, 8, 38}};
static const float kFix03MinRate[4] = {1.00f, 0.58f, 0.87f, 0.70f};  // the slowest region of each FIX03 F2 plan (evidence/dkn_lab_output.txt of 0.13.12)
static double plan_fp(unsigned pi, const Plan& p) {
  double fp = 0;
  for (unsigned s = 0; s < p.n; ++s)
    fp += (s + 1) * (pi + 1) * (p.seg[s].start * 3.0 + p.seg[s].cut * 5.0 + p.seg[s].rate_in * 7.0 + p.seg[s].rate_mid * 11.0 + p.seg[s].rate_out * 13.0 + p.seg[s].morph * 17.0);
  return fp + p.scale * 19.0 * (pi + 1);
}

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

// FIX02: per-impact parry opportunity. The AT of a window is on for the collision pass after every tick whose post-play frame lies in the window
// (dkn_at_authority runs in the action POST, after play). For each impact: ticks with the AT on, whether the impact frame is crossed inside the window,
// and whether the window CLOSED (a post-play frame outside it) before the next window of the same move/index (a separate ledger ticket).
struct ImpactOpp {
  unsigned open_ticks = 0;
  bool impact_inside = false, separated = true;
  Move m = Move::F2A;
  unsigned k = 0;
};
struct OppOut {
  ImpactOpp imp[9];
  unsigned n = 0;
};
static OppOut opportunities(const Combo& c, const Plan& p) {
  OppOut o;
  const Timeline tl = timeline(p);
  unsigned seg = 0;
  float frame = p.seg[0].start;
  int open_w = -1;  // window index currently open (-1 none)
  for (unsigned t = 0; t < 600; ++t) {
    unsigned ts = 0;
    float want = 0;
    const bool on = locate(p, tl, static_cast<float>(t + 1), ts, want);
    bool switched = false;
    if (ts != seg) seg = ts, frame = switch_frame(p.seg[seg], static_cast<float>(t) - tl.T[seg]), switched = true;
    float rate = on ? want - frame : 1.0f;
    rate = rate < 0 ? 0 : rate > kTickRateMax ? kTickRateMax : rate;
    const float prev = frame;
    frame += rate;
    const Move m = p.seg[seg].m;
    const MoveInfo& mi = info(m);
    const int w = window_of(m, frame);
    if (w >= 0) {
      const bool fresh = open_w < 0 || switched || o.imp[o.n ? o.n - 1 : 0].m != m || static_cast<int>(o.imp[o.n ? o.n - 1 : 0].k) != w;
      if (fresh) {
        if (open_w >= 0 && o.n && o.imp[o.n - 1].m == m && static_cast<int>(o.imp[o.n - 1].k) == w) o.imp[o.n - 1].separated = false;  // same ticket would be reused
        if (o.n < 9) o.imp[o.n].m = m, o.imp[o.n].k = static_cast<unsigned>(w), ++o.n;
      }
      ImpactOpp& im = o.imp[o.n - 1];
      ++im.open_ticks;
      if (prev < mi.impact[w] && frame >= mi.impact[w]) im.impact_inside = true;
      if (frame >= mi.impact[w] && prev < mi.impact[w] && prev < mi.at_open[w]) im.impact_inside = true;  // crossed open+impact in one tick: AT on this pass
      open_w = w;
    } else {
      open_w = -1;
    }
    if (!on) break;
  }
  (void)c;
  return o;
}

int main() {
  // ---------------------------------------------------------------- 1. tables
  CHECK(policy::kHp1Max == 600 && policy::kRes2Max == 480 && policy::kPosture1Max == 200 && policy::kPosture2Max == 160 && policy::kPosture3Max == 180, "approved maxima (FIX04 600/200, 480, 160/180)");
  CHECK(posture_max(Ph::Armor) == 200 && posture_max(Ph::Transform) == 200 && posture_max(Ph::Light) == 160 && posture_max(Ph::Revived) == 180, "posture maximum per stage");
  CHECK(policy::kGainBlocked1 == 4 && policy::kGainSwordHp1 == 6 && policy::kGainIron1 == 25 && policy::kGainBlocked2 == 3 && policy::kGainSwordHp2 == 5 && policy::kGainIron2 == 25,
        "Table A: F1 +4 blocked / +6 direct / +25 iron ball; F2-F3 +3 / +5 / +25 unchanged");
  CHECK(policy::kRegenDelayTicks == 60 && policy::kBreakTicks == 90 && policy::kDownTicks == 180, "approved timings (no hit cap exists any more)");
  CHECK(policy::kPassiveShieldTicks == 38 && policy::kPassiveIdleTicks == 75, "passivity thresholds");
  CHECK(dk_parry_gain(Move::F1B, 0) == 14 && dk_parry_gain(Move::F1A, 0) == 12 && dk_parry_gain(Move::F1Shield, 0) == 14, "Table A phase 1 (FIX04: vertical 14, lateral 12, bash 14)");
  CHECK(dk_parry_gain(Move::F2A, 0) == 11 && dk_parry_gain(Move::F2B, 0) == 9 && dk_parry_gain(Move::F2B, 1) == 9 && dk_parry_gain(Move::F2B1, 0) == 11 &&
            dk_parry_gain(Move::F2C, 0) == 15, "Table A phase 2");
  struct Tb { Move m; int b, d; } tb[] = {{Move::F1B, 26, 36}, {Move::F1A, 20, 30}, {Move::F1Shield, 42, 50}, {Move::F2A, 23, 33}, {Move::F2B, 12, 22}, {Move::F2B1, 23, 33}, {Move::F2C, 38, 45}};
  for (const Tb& x : tb) {
    CHECK(link_gain(x.m, Outcome::Block) == x.b && link_gain(x.m, Outcome::Direct) == x.d && link_gain(x.m, Outcome::Parry) == 0, "Table B %s", name(x.m));
  }
  // ---------------------------------------------------------------- 2. combos: inventory and exact posture sums
  unsigned total = 0, p1 = 0, p2 = 0;
  const int exp_dk[8] = {66, 42, 66, 66, 65, 69, 91, 55};  // FIX04 F1 parries 12/14
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
    const int mx = c.phase2 ? policy::kPosture2Max : policy::kPosture1Max;  // F3 shares the F2 repertoire with 180
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
    const Plan p = plan(c, ci);
    const SimOut o = simulate(c, p);
    CHECK(o.ok, "%s simulated %u impacts of %u", c.name, o.n, c.n_impacts);
    std::printf("%s:", c.name);
    for (unsigned m = 0; m < p.n; ++m)
      std::printf(" [%s s=%.1f cut=%.1f in=%.2f mid=%.2f out=%.2f]", name(p.seg[m].m), p.seg[m].start, p.seg[m].cut, p.seg[m].rate_in, p.seg[m].rate_mid, p.seg[m].rate_out);
    std::printf("\n");
    for (unsigned j = 0; j + 1 < o.n; ++j) {
      const double audio = (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03 * p.scale;  // phase 1: x S (the user's rule: slower, same separations)
      const double sim = o.tick[j + 1] - o.tick[j];
      const double cum = (o.tick[j + 1] - o.tick[0]) - (c.peak_ms[j + 1] - c.peak_ms[0]) * 0.03 * p.scale;
      const double err = sim - audio;
      if (std::fabs(err) > worst) worst = std::fabs(err);
      if (std::fabs(cum) > worst_cum) worst_cum = std::fabs(cum);
      if (p.clamped[j]) ++clamped;
      std::printf("   pair %u-%u audio=%.3fs x%.2f (%.2f t) sim=%.0f t err=%+.2f t cum=%+.2f t%s\n", j + 1, j + 2, (c.peak_ms[j + 1] - c.peak_ms[j]) / 1000.0, p.scale, audio, sim, err, cum,
                  p.clamped[j] ? " RATE_CLAMPED" : "");
      CHECK(std::fabs(err) <= 1.0 + 1e-6, "%s pair %u: |err| %.2f > 1 tick", c.name, j + 1, err);
      CHECK(std::fabs(cum) <= 2.0 + 1e-6, "%s pair %u: cumulative |err| %.2f > 2 ticks", c.name, j + 1, cum);
    }
    for (unsigned m = 0; m < p.n; ++m) {
      const MoveInfo& mi = info(p.seg[m].m);
      if (c.phase2) {
        CHECK(p.seg[m].rate_in >= kRateMin && p.seg[m].rate_in <= kRateMax && p.seg[m].rate_out >= kRateMin && p.seg[m].rate_out <= kRateMax &&
                  p.seg[m].rate_mid >= kRateMin && p.seg[m].rate_mid <= kRateMax, "%s rate range", c.name);
        CHECK(p.seg[m].start >= mi.s_min - 1e-4f && p.seg[m].start <= mi.s_max + 1e-4f, "%s start frame in range", c.name);
      } else {  // FIX01 heavy phase 1: wind-up + swing ALWAYS native (1.0), recovery paced in [0.70, 1.51], never a fast mace
        CHECK(p.seg[m].rate_in == 1.0f, "%s seg %u: the wind-up/swing plays at the native rate (%.3f)", c.name, m, p.seg[m].rate_in);
        if (m + 1 < p.n) CHECK(p.seg[m].rate_out >= kF1RecoveryRateMin && p.seg[m].rate_out <= kF1RecoveryRateMax, "%s seg %u recovery rate %.3f", c.name, m, p.seg[m].rate_out);
        if (m + 1 < p.n) CHECK(p.seg[m].cut >= 48.0f, "%s seg %u: the recovery is played to frame %.0f (0.13.9 cut at 28-40 = pose gap 126-141)", c.name, m, p.seg[m].cut);
        if (m > 0) CHECK(p.seg[m].morph >= 6.0f, "%s seg %u: blended over >= 6 frames", c.name, m);
        CHECK(p.scale >= 1.0f, "%s never faster than the recording", c.name);
      }
      if (p.seg[m].cut >= 0) CHECK(p.seg[m].cut > mi.impact[mi.impacts - 1], "%s cut after the last impact", c.name);
    }
    if (!c.phase2) {
      // the separations keep the recording's RATIOS exactly: every planned interval / audio interval == S
      for (unsigned j = 0; j + 1 < c.n_impacts; ++j)
        CHECK(std::fabs(p.target_ticks[j] - (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03 * p.scale) < 1e-3, "%s pair %u ratio kept", c.name, j + 1);
      // the fastest phase-1 separation is now >= 0.9 s (was 0.37 s)
      for (unsigned j = 0; j + 1 < o.n; ++j) CHECK(o.tick[j + 1] - o.tick[j] >= 27.0, "%s pair %u: %.0f ticks, a heavy mace never strikes again within 0.9 s", c.name, j + 1, o.tick[j + 1] - o.tick[j]);
    }
    CHECK(o.frozen == 0, "%s: %u frozen ticks inside the combo", c.name, o.frozen);
    CHECK(o.worst_cut_gap <= 2.6f, "%s: a move switched %.2f frames away from its planned cut", c.name, o.worst_cut_gap);
    if (ci == 4) CHECK(o.b1_last_frame >= 0 && o.b1_last_frame < info(Move::F2B).at_open[1] + 1.5f && o.b1_last_frame < info(Move::F2B).impact[1],
                       "B1 never reaches the second cut (last frame %.2f)", o.b1_last_frame);
  }
  CHECK(clamped == 0, "%u intervals need a rate outside [%.2f, %.2f]", clamped, kRateMin, kRateMax);
  std::printf("RHYTHM worst pair error %.2f ticks, worst cumulative %.2f ticks, clamped %u\n\n", worst, worst_cum, clamped);

  // ---------------------------------------------------------------- 3b. FIX02: per-impact parry matrix (8 combos = 46 impacts, + the isolated strikes / counters)
  {
    std::printf("\nPARRY MATRIX (ticks with the impact AT on, impact frame inside the open window, own ticket)\n");
    unsigned total = 0, ok = 0, min_ticks = 99;
    for (unsigned pi = 0; pi < kPlans; ++pi) {
      const Combo& c = combo_of(pi);
      const Plan p = plan(c, pi);
      const OppOut o = opportunities(c, p);
      CHECK(o.n == c.n_impacts, "%s: %u impact windows, expected %u", c.name, o.n, c.n_impacts);
      std::printf("%s:", c.name);
      for (unsigned i = 0; i < o.n; ++i) {
        const ImpactOpp& im = o.imp[i];
        const bool good = im.open_ticks >= 2 && im.impact_inside && im.separated;
        if (pi < 8) ++total, ok += good;
        if (im.open_ticks < min_ticks) min_ticks = im.open_ticks;
        std::printf(" [%u %s k%u at=%ut%s%s]", i + 1, name(im.m), im.k, im.open_ticks, im.impact_inside ? "" : " NO_IMPACT", im.separated ? "" : " SHARED_TICKET");
        CHECK(good, "%s impact %u: AT on %u ticks, impact inside=%d, own ticket=%d", c.name, i + 1, im.open_ticks, im.impact_inside ? 1 : 0, im.separated ? 1 : 0);
        if (pi < 8) CHECK(im.open_ticks >= 5, "%s impact %u: a combo impact keeps >= 5 ticks of real AT (FIX03 standard), got %u", c.name, i + 1, im.open_ticks);
      }
      std::printf("\n");
    }
    CHECK(total == 46 && ok == 46, "parry matrix %u/%u", ok, total);
    {  // F3 (the resurrected stage) plays the very F2 plans: confirmed separately, position by position
      unsigned f3 = 0, f3_ok = 0;
      CHECK(phase2(Ph::Revived) && posture_max(Ph::Revived) == 180 && stage(Ph::Revived) == 2, "F3 is a sword stage of its own (180)");
      for (unsigned pi = 4; pi < 8; ++pi) {
        const OppOut o = opportunities(combo_of(pi), plan(combo_of(pi), pi));
        for (unsigned i = 0; i < o.n; ++i) ++f3, f3_ok += o.imp[i].open_ticks >= 5 && o.imp[i].impact_inside && o.imp[i].separated;
      }
      CHECK(f3 == 28 && f3_ok == 28, "F3 parry matrix %u/%u", f3_ok, f3);
      std::printf("PARRY_MATRIX_F3 %u/28 (same plans as F2, x1.40)\n", f3_ok);
    }
    std::printf("PARRY_MATRIX combos=%u/46 ok, min_open_ticks=%u (singles/counters included), specials: kick AT 8..12 / bash 5..9 at the native rate (4 ticks)\n\n", ok, min_ticks);
    // FIX03 timing regression. (1) Phase 1 FROZEN: the F1 plans, every MoveInfo and the F1 combos are the 0.13.11 numbers. (2) The ONLY phase-2/3 change
    // is the time scale: the twelve 0.13.11 plans rebuilt with scale 1.0 give back the 0.13.10/0.13.11 fingerprint bit for bit.
    double f1 = 0, fix04 = 0, mv = 0, cb = 0;
    for (unsigned pi : {0u, 1u, 2u, 3u, 8u, 9u, 13u}) f1 += pi == 13 ? 0.0 : plan_fp(pi, plan(combo_of(pi), pi));
    for (unsigned pi = 0; pi < kPlans; ++pi) fix04 += plan_fp(pi, plan(combo_of(pi), pi));
    for (unsigned m = 0; m < 3; ++m) {  // F1A, F1B, F1Shield
      const MoveInfo& i = info(Move(m));
      mv += (m + 1) * (i.impact[0] * 3 + i.at_open[0] * 5 + i.at_close[0] * 7 + i.s_min * 11 + i.s_max * 13 + i.c_min * 17 + i.c_max * 19 + i.end * 23 + i.morph * 29 + i.bck * 31 +
                       i.act * 37 + i.mode * 41);
    }
    for (unsigned c = 0; c < 4; ++c) {
      for (unsigned k = 0; k < kCombos[c].n_impacts; ++k) cb += (c + 1) * (k + 1) * kCombos[c].peak_ms[k];
      for (unsigned k = 0; k < kCombos[c].n_moves; ++k) cb += (c + 1) * (k + 1) * 7 * unsigned(kCombos[c].moves[k]);
    }
    std::printf("F1_FROZEN plan_fp=%.4f (0.13.11 %.4f) f1_moves_fp=%.4f (0.13.11 %.4f) f1_combos_fp=%.1f (0.13.11 %.1f)\n", f1, F1_PLAN_FINGERPRINT_0_13_11, mv, F1_MOVES_FINGERPRINT_0_13_11,
                cb, F1_COMBOS_FINGERPRINT_0_13_11);
    std::printf("TIMING_FINGERPRINT fix04=%.4f (F2/F3 x1.40 + the documented F2 ranges; phase 1 above)\n", fix04);
    CHECK(std::fabs(f1 - F1_PLAN_FINGERPRINT_0_13_11) < 0.01, "phase 1 plans changed vs 0.13.11 (%.4f)", f1);
    CHECK(std::fabs(mv - F1_MOVES_FINGERPRINT_0_13_11) < 0.01, "a phase-1 MoveInfo changed vs 0.13.11 (%.4f)", mv);
    CHECK(std::fabs(cb - F1_COMBOS_FINGERPRINT_0_13_11) < 0.01, "an F1 combo changed vs 0.13.11 (%.1f)", cb);
    // FIX04: the F2 MoveInfo differs from 0.13.12 ONLY in the documented natural ranges (start from the native ready pose, cut up to the natural rest);
    // impacts, AT windows, clips, actions, Table A/B, morphs, reach: identical.
    for (const OldRange& o : kFix03Ranges) {
      const MoveInfo& i = info(o.m);
      const bool same_rest = i.s_max == o.s_max && i.c_min == o.c_min && i.end == o.end;
      const float want_smin = o.m == Move::F2C ? 0.0f : 1.0f, want_cmax = o.m == Move::F2A ? 35.0f : o.m == Move::F2B ? 54.0f : o.m == Move::F2C ? 28.0f : 20.0f;
      CHECK(same_rest && i.s_min == want_smin && i.c_max == want_cmax && i.c_max < i.end, "%s range %.0f..%.0f / %.0f..%.0f (0.13.12 %.0f..%.0f / %.0f..%.0f)", name(o.m), i.s_min, i.s_max, i.c_min,
            i.c_max, o.s_min, o.s_max, o.c_min, o.c_max);
    }
    CHECK(info(Move::F2C).at_close[0] == 12.0f && info(Move::F2B).at_open[1] == 21.0f && info(Move::F2B1).c_min == 20.0f && info(Move::F2B1).c_max == 20.0f, "AT windows / B1 cut unchanged");
    for (unsigned pi = 0; pi < kPlans; ++pi)
      if (!combo_of(pi).phase2) CHECK(plan_fp(pi, plan(combo_of(pi), pi)) == plan_fp(pi, plan(combo_of(pi), pi, 1.40f)), "phase-1 plan %u ignores any phase-2 tempo", pi);
    const Plan bash = plan(combo_of(kIdxF1Bash), kIdxF1Bash);
    CHECK(bash.seg[0].rate_in == 1.0f && bash.seg[0].rate_out == 1.0f && bash.seg[0].start == 0.0f && bash.scale == 1.0f, "the F1 counter bash plays the native bash (F1 tempo untouched)");
  }
  // ---------------------------------------------------------------- 3d. FIX04 tempo matrix: F2/F3 combos x1.40 vs FIX02 (x1.12 vs FIX03), ratios kept, no slow motion
  {
    std::printf("\nTEMPO MATRIX FIX02 (0.13.11) / FIX03 (0.13.12) / FIX04 (0.13.13), ticks between impacts at 30 Hz (F3 plays these very plans)\n");
    const int want_total[4] = {95, 130, 169, 122};
    for (unsigned ci = 4; ci < 8; ++ci) {
      const Combo& c = kCombos[ci];
      const Plan p4 = plan(c, ci);
      const SimOut o4 = simulate(c, p4);
      CHECK(o4.ok && p4.scale == policy::kF2ComboScale && policy::kF2ComboScale == 1.40f, "%s simulates at x1.40", c.name);
      int f2 = 0, f3 = 0;
      for (unsigned j = 0; j + 1 < o4.n; ++j) f2 += kFix02Iv[ci - 4][j], f3 += kFix03Iv[ci - 4][j];
      const double tot = o4.tick[o4.n - 1] - o4.tick[0];
      std::printf("%s first->last FIX02 %d / FIX03 %d / FIX04 %.0f t (x%.3f vs FIX02, x%.3f vs FIX03):", c.name, f2, f3, tot, tot / f2, tot / f3);
      for (unsigned j = 0; j + 1 < o4.n; ++j) {
        const double a4 = o4.tick[j + 1] - o4.tick[j], exact = (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03 * 1.40;
        std::printf(" %d/%d/%.0f", kFix02Iv[ci - 4][j], kFix03Iv[ci - 4][j], a4);
        CHECK(std::fabs(a4 - exact) <= 1.0 + 1e-6, "%s pair %u: %.0f ticks, the recording x1.40 is %.2f (+-1 tick at 30 Hz)", c.name, j + 1, a4, exact);
        CHECK(std::fabs(p4.target_ticks[j] - exact) < 1e-3, "%s pair %u planned interval not exactly audio x1.40", c.name, j + 1);
        CHECK(std::fabs(a4 - kFix02Iv[ci - 4][j] * 1.40) <= 2.5 + 1e-6, "%s pair %u vs FIX02 x1.40 (both quantized at 30 Hz)", c.name, j + 1);
        for (unsigned i = 0; i + 1 < o4.n; ++i)  // Δt_i / Δt_j of the recording, planned exactly
          CHECK(std::fabs(p4.target_ticks[j] * (c.peak_ms[i + 1] - c.peak_ms[i]) - p4.target_ticks[i] * (c.peak_ms[j + 1] - c.peak_ms[j])) < 1e-2, "%s ratio %u/%u", c.name, j, i);
      }
      CHECK(std::fabs(tot - want_total[ci - 4]) <= 2.0, "%s first->last %.0f ticks (approved reference %d)", c.name, tot, want_total[ci - 4]);
      CHECK(tot / f3 > 1.08 && tot / f3 < 1.16, "%s x%.3f vs FIX03: x1.40 against FIX02, never x1.40 on top of FIX03", c.name, tot / f3);
      // no slow-motion region (FIX03 0.58-0.72): every rate of the plan near the native speed, no region slower than FIX03's
      float rmin = 9, rmax = 0;
      for (unsigned m = 0; m < p4.n; ++m)
        for (float r : {p4.seg[m].rate_in, p4.seg[m].rate_mid, p4.seg[m].rate_out}) rmin = r < rmin ? r : rmin, rmax = r > rmax ? r : rmax;
      std::printf(" rates %.2f..%.2f (FIX03 slowest %.2f)\n", rmin, rmax, kFix03MinRate[ci - 4]);
      // (F2-C4 kick -> thrust 0.73: the kick clip has 29 frames; filling the recorded 880 ms x1.40 needs that rate or an invented pause; FIX03 0.70)
      CHECK(rmin >= 0.72f && rmin >= kFix03MinRate[ci - 4] - 1e-3f && rmax <= 1.70f, "%s playback %.2f..%.2f: no slow motion (FIX03 slowest %.2f)", c.name, rmin, rmax, kFix03MinRate[ci - 4]);
      // the first impact: a full, readable preparation (never a residual frame): >= 9 ticks of real wind-up at the native rate (FIX03 thrust: 7)
      CHECK(o4.tick[0] >= 8.0 && p4.seg[0].start == info(p4.seg[0].m).s_min && p4.seg[0].rate_in == 1.0f, "%s first impact after %.0f ticks of wind-up", c.name, o4.tick[0] + 1);
      std::printf("   first_impact_after=%.0f ticks (start frame %.1f)\n", o4.tick[0] + 1, p4.seg[0].start);
    }
    for (unsigned pi : {kIdxF2Thrust, kIdxF2Double, kIdxF2Kick}) {
      const Plan p1 = plan(combo_of(pi), pi, 1.0f), p4 = plan(combo_of(pi), pi);
      const double d1 = timeline(p1).T[1], d4 = timeline(p4).T[1];
      const double want = pi == kIdxF2Kick ? policy::kKickScale : policy::kF2SingleScale, fix03 = pi == kIdxF2Kick ? 1.12 : 1.20;
      std::printf("%s native %.1f -> %.1f t (x%.3f; FIX03 x%.2f; first impact after %u ticks)\n", combo_of(pi).name, d1, d4, d4 / d1, fix03, first_impact_tick(pi) + 1);
      CHECK(std::fabs(d4 / d1 - want) < 1e-3 && want / fix03 > 1.10 && want / fix03 < 1.14, "%s x%.3f, wanted x%.2f (+12 %% over FIX03 like the combos)", combo_of(pi).name, d4 / d1, want);
      CHECK(p4.seg[0].rate_in >= kRateMin && first_impact_tick(pi) + 1 >= 12, "%s readable preparation", combo_of(pi).name);
    }
    for (unsigned pi : {kIdxF1SingleA, kIdxF1SingleB, kIdxF1Bash}) {
      const Plan p = plan(combo_of(pi), pi);
      CHECK(p.scale == 1.0f && p.seg[0].rate_in == 1.0f && p.seg[0].rate_out == 1.0f && p.seg[0].start == 0.0f, "%s keeps its FIX03 native tempo", combo_of(pi).name);
    }
    std::printf("TEMPO f2_combos=x%.2f(vs FIX02) singles_counters=x%.2f kick=x%.2f phase1=FROZEN\n", policy::kF2ComboScale, policy::kF2SingleScale, policy::kKickScale);
  }
  // ---------------------------------------------------------------- 3c. FIX02 invariants: no hit-context entry, no native leap, Link's slip
  {
    CHECK(native::needs_hit_context(native::kActGuardL, 0) && !native::needs_hit_context(native::kActGuardL, 1) && !native::needs_hit_context(native::kActGuardH, 0) &&
              !native::needs_hit_context(native::kActChaseL, 0), "only ACT_GUARDL 0 reads this pass' collider (executeGuardL -> getCutType)");
    CHECK(native::native_leap(native::kActChaseL, 8) && native::native_leap(native::kActChaseL, 17) && !native::native_leap(native::kActChaseL, 3) && !native::native_leap(native::kActChaseL, 6),
          "the forward leaps 8/17 only (the evasive jumps stay native)");
    const float nat = slip_distance(kLinkSlipNative);
    CHECK(nat > 150.0f && nat < 175.0f, "native large guard slip ~160 units (%.0f)", nat);
    for (unsigned m = 0; m < unsigned(Move::Count); ++m) {  // FIX03: mace 12, bash/kick 12, sword 10; chained x0.80 / x0.65
      const Move mm = Move(m);
      const float d = slip_distance(link_slip_cap(mm)), d1 = slip_distance(link_slip_cap(mm, 1)), d2 = slip_distance(link_slip_cap(mm, 2)), d9 = slip_distance(link_slip_cap(mm, 9));
      const float want = (mm == Move::F2A || mm == Move::F2B || mm == Move::F2B1) ? 10.0f : 12.0f;  // sword 10; mace, bash, kick 12
      CHECK(link_slip_cap(mm) == want && d >= 40.0f && d <= 70.0f, "%s slip %.0f units (cap %.1f): perceptible, short", name(mm), d, link_slip_cap(mm));
      CHECK(d1 < d && d2 < d1 && d9 == d2 && d2 >= 15.0f, "%s chained slips %.0f / %.0f / %.0f: decreasing, bounded, never zero", name(mm), d, d1, d2);
      std::printf("SLIP %s cap=%.1f first=%.0f second=%.0f third+=%.0f (0.13.11 %.0f, native %.0f)\n", name(mm), link_slip_cap(mm), d, d1, d2,
                  slip_distance(mm == Move::F1A || mm == Move::F1B ? 15.0f : mm == Move::F1Shield || mm == Move::F2C ? 16.0f : 13.0f), nat);
    }
    // FIX04: one chain step per REAL impact. FIX03 advanced the chain twice per impact (the same slip capped again the next tick, log 231220): the profile
    // the user approved is, per real impact, the nominal table at index 1 then >= 2. FIX04 reproduces it exactly and never pushes more.
    for (unsigned m = 0; m < unsigned(Move::Count); ++m) {
      const Move mm = Move(m);
      for (unsigned k = 0; k < 8; ++k) {
        const unsigned fix03_index = 2 * k + 1;  // FIX03: impact k advanced the chain to 2k (PRE) then 2k+1 (POST): the last cap applied
        CHECK(link_slip_cap_impact(mm, k) == link_slip_cap(mm, fix03_index), "%s impact %u: FIX04 cap %.2f != FIX03 realized %.2f", name(mm), k, link_slip_cap_impact(mm, k),
              link_slip_cap(mm, fix03_index));
        CHECK(link_slip_cap_impact(mm, k) <= link_slip_cap(mm, k), "%s impact %u: never more push than the nominal single count", name(mm), k);
      }
    }
    // the reproduction of the log sequence: tick 3101 PRE caps 18.8 -> 12 (chain 0), tick 3102 Link's POST re-caps the SAME slip -> 9.60 (chain 1)
    {
      struct Claim { std::uint64_t serial = 0, done = 0; unsigned chain = 0; std::uint64_t last = 0; float cap = 0; } c;
      auto cap_call = [&](std::uint64_t now, std::uint64_t serial, float speed) {  // dkn_slip_cap with FIX04's per-serial idempotence
        if (c.done != serial) c.done = serial, c.chain = c.last && now - c.last <= policy::kSlipChainTicks ? c.chain + 1 : 0, c.last = now, c.cap = link_slip_cap_impact(Move::F2C, c.chain);
        return speed > c.cap ? c.cap : speed;
      };
      const float a1 = cap_call(3101, 34, 18.8f), a2 = cap_call(3102, 34, a1);
      CHECK(c.chain == 0 && a1 == a2 && a1 == 9.60f, "the same impact capped twice: ONE chain step, same cap %.2f / %.2f (FIX03 log: 12 then 9.60 with chain 1)", a1, a2);
      const float b1 = cap_call(3120, 35, 18.8f);
      CHECK(c.chain == 1 && b1 == link_slip_cap(Move::F2C, 2), "the next real impact is the second link (%.2f)", b1);
    }
    for (unsigned ci = 0; ci < 8; ++ci) {  // a fully blocked combo: Link's whole drift (every impact chained)
      const Combo& c = kCombos[ci];
      float now = 0, fix03 = 0;
      unsigned k = 0;
      for (unsigned m = 0; m < c.n_moves; ++m)
        for (unsigned i = 0; i < info(c.moves[m]).impacts; ++i, ++k) now += slip_distance(link_slip_cap_impact(c.moves[m], k)), fix03 += slip_distance(link_slip_cap(c.moves[m], 2 * k + 1));
      std::printf("SLIP_COMBO %s all-blocked drift %.0f units (FIX03 realized %.0f, native %.0f)\n", c.name, now, fix03, nat * c.n_impacts);
      CHECK(now == fix03, "%s blocked drift %.0f != FIX03 realized %.0f (the approved feel)", c.name, now, fix03);
    }
  }
  // ---------------------------------------------------------------- 4. posture model
  {
    Posture p;
    p.reset(policy::kPosture1Max);
    for (int i = 0; i < 49; ++i) CHECK(p.gain(policy::kGainBlocked1) == Gain::Added, "gain below max");
    CHECK(p.points() == 196 && p.gain(policy::kGainBlocked1) == Gain::Reached && p.points() == 200 && p.latched, "F1: 50 blocked swords (+4) break 200 (got %d)", p.points());
    CHECK(p.gain(5) == Gain::Ignored && p.points() == 200, "no gain while latched");
    for (unsigned t = 0; t < 300; ++t) p.regen(10);
    CHECK(p.points() == 200 && p.latched, "no regeneration while broken (300 ticks)");
    {  // 199/200: one more legitimate gain breaks; nothing breaks below
      Posture q;
      q.reset(policy::kPosture1Max);
      q.gain(199);
      CHECK(q.points() == 199 && !q.latched, "199/200 is not a break");
      CHECK(q.gain(policy::kGainBlocked1) == Gain::Reached && q.points() == 200 && q.latched, "199 + 4 saturates at 200 (no overflow)");
      // the FIX04 counts with no regeneration: 50 blocks / 34 directs / 17 lateral parries / 15 vertical parries (F1, 200)
      const struct { int g, n; } need[] = {{policy::kGainBlocked1, 50}, {policy::kGainSwordHp1, 34}, {12, 17}, {14, 15}};
      for (const auto& x : need) CHECK((policy::kPosture1Max + x.g - 1) / x.g == x.n, "F1 break needs %d gains of +%d", x.n, x.g);
    }
    p.reset(policy::kPosture2Max);
    CHECK(p.points() == 0 && p.max_pts == 160, "reset to 0/160");
    p.gain(30);
    for (unsigned t = 0; t < 60; ++t) p.regen(12);
    CHECK(p.points() == 30, "60 eligible ticks of delay");
    for (unsigned t = 0; t < 30; ++t) p.regen(12);
    CHECK(p.points() == 18, "12 pts/s -> 30 ticks = 12 points (got %d)", p.points());
    p.gain(1);
    CHECK(!p.regen(12) && p.since_gain == 1, "a genuine gain restarts the delay");
    // bands: borders belong to the lower band, both phases
    CHECK(band(600, 600) == 0 && band(451, 600) == 0 && band(450, 600) == 1 && band(301, 600) == 1 && band(300, 600) == 2 && band(151, 600) == 2 && band(150, 600) == 3 &&
              band(61, 600) == 3 && band(60, 600) == 4 && band(0, 600) == 4, "phase-1 bands over 600 (borders to the lower band)");
    CHECK(regen_rate(false, 600, 600) == 10 && regen_rate(false, 400, 600) == 7 && regen_rate(false, 250, 600) == 4 && regen_rate(false, 100, 600) == 2 && regen_rate(false, 50, 600) == 1,
          "phase-1 regen table over 600");
    CHECK(regen_rate(false, 316, policy::kHp1Max) == 7 && regen_rate(false, 450, policy::kHp1Max) == 7, "no 420 denominator left (316/600 = 52.7 %%: band 50-75)");
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
    const int hits[] = {40, 40, 60, 40, 4, 8, 40, 12, 40, 60, 40, 3, 40, 40, 25, 60, 40, 40, 2, 40, 6, 40, 60, 40, 8, 40, 30, 40, 40, 60};  // native Link sword 40 / finish 60 + other sources
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
    CHECK(armor_progress(600) == 0 && armor_progress(550) == 1 && armor_progress(551) == 0 && armor_progress(50) == 11 && armor_progress(51) == 10 && armor_progress(1) == 11,
          "50 HP per step over 600");
    {  // the native 40 / 60 sword hits: never more than two steps per hit, 12 only at 0, transformation exactly once
      int h = policy::kHp1Max, pr = 0, transitions = 0, worst = 0;
      for (unsigned i = 0; i < 100 && h > 0; ++i) {
        h -= (i % 4 == 3) ? 60 : 40;
        if (h < 0) h = 0;
        const int q = armor_progress(h);
        if (q - pr > worst) worst = q - pr;
        if (h > 0) CHECK(q <= 11, "no progress 12 with HP left");
        if (q == 12) ++transitions;
        pr = q;
      }
      CHECK(h == 0 && transitions == 1 && worst <= 2, "40/60 hits: transition once at HP 0, <= 2 steps per hit (worst %d)", worst);
    }
  }
  // ---------------------------------------------------------------- 6. break controller (FIX01): no hit cap, executions only by contact, bounded extension
  {
    // phase 1: 90 ticks, hits never end it, then recovery
    Break b;
    b.latch(), b.begin(100, false);
    CHECK(b.st == Bk::StaggerA && b.breaks == 1 && b.start == 100, "phase-1 stagger");
    CHECK(!b.exec() && !b.helm_prep(101) && b.st == Bk::StaggerA, "phase 1 has NO executor and NO Helm Splitter preparation (in its own stagger)");
    for (unsigned t = 1; t < 90; ++t) CHECK(!b.tick(false), "window open at %u", t);
    CHECK(b.tick(false) && b.st == Bk::Recover && b.why == BkEnd::Timeout, "90 ticks, no hit counter exists");
    CHECK(!b.recover(false) && b.recover(true) && b.st == Bk::Combat, "recovery ends with the native clip");
    // phases 2-3: the break alone never makes him fall; Mortal Draw contact -> Exec
    Break m;
    m.latch(), m.begin(0, true);
    for (unsigned t = 0; t < 50; ++t) m.tick(false);
    CHECK(m.st == Bk::StaggerA, "no fall by the break alone");
    CHECK(m.exec() && m.st == Bk::Exec && m.why == BkEnd::Execution, "Mortal Draw contact -> execution fall");
    CHECK(!m.exec(), "one execution, never twice");
    // Shield Attack -> second stagger -> Helm Splitter contact
    Break h;
    h.latch(), h.begin(0, true);
    for (unsigned t = 0; t < 40; ++t) h.tick(false);
    CHECK(h.helm_prep(40) && h.st == Bk::StaggerB && h.helm_preps == 1, "a new Shield Attack in the first stagger prepares the Helm Splitter");
    CHECK(!h.helm_prep(41), "a second Shield Attack never restarts / repeats the second stagger");
    CHECK(h.exec() && h.st == Bk::Exec, "Helm Splitter contact -> execution fall");
    // second stagger without the Helm Splitter: its own window, then recovery (no free fall)
    Break w2;
    w2.latch(), w2.begin(0, true), w2.helm_prep(10);
    unsigned rec_at = 0;
    for (unsigned t = 1; t <= 200 && !rec_at; ++t)
      if (w2.tick(false)) rec_at = t;
    CHECK(rec_at == policy::kHelmWindowTicks && w2.st == Bk::Recover, "second stagger window %u (got %u)", policy::kHelmWindowTicks, rec_at);
    // bounded extension: only while the finisher flies, at most kFinisherExtMax, once
    Break e;
    e.latch(), e.begin(0, true);
    for (unsigned t = 1; t < 90; ++t) e.tick(false);
    unsigned extra = 0;
    while (!e.tick(true) && extra < 500) ++extra;
    CHECK(extra == policy::kFinisherExtMax && e.st == Bk::Recover, "a Mortal Draw in flight extends the window by at most %u (got %u)", policy::kFinisherExtMax, extra);
    Break e2;
    e2.latch(), e2.begin(0, true);
    for (unsigned t = 1; t < 90; ++t) e2.tick(false);
    CHECK(!e2.tick(true) && !e2.tick(true) && e2.tick(false), "the extension ends the tick the finisher stops (no extension by repeated presses)");
    Break w;
    w.latch(), w.begin(0, true);
    w.tick(false), w.abort(BkEnd::Phase);
    CHECK(w.st == Bk::Combat && w.why == BkEnd::Phase, "a phase change / defeat preempts the stagger");
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
    // second stagger clip (FIX03): TNB_DAMAGE_R from frame 0 to its hold frame 12, then held (distinct from TNB_DOWN_SIPPU, never the TNB_DOWN guard knockback)
    float f = kHelmStart;
    for (unsigned t = 0; t < policy::kHelmWindowTicks + policy::kFinisherExtMax; ++t) {
      const float r = helm_rate(f);
      CHECK(r > 0.0f, "a held clip never plays at rate 0 (the native isStop would end the stagger)");
      f += r;
    }
    CHECK(f >= kHelmHold && f < kHelmHold + 2.0f && f < native::kEndTnbDamageR - 1.0f && kHelmClip == native::kBckTnbDamageR && kHelmClip != native::kBckTnbDownSippu &&
              kHelmClip != native::kBckTnbDown && kHelmClip != native::kBckTnbGuardA && kHelmClip != native::kBckTnbGuardB, "second stagger held at TNB_DAMAGE_R frame %.1f", f);
    CHECK(kHelmHold >= 10.0f && kHelmHold <= 14.0f, "hold inside the measured sword-low span (frames 4-18, lowest 10-12)");
    for (float end : {native::kEndTnaDamageL, native::kEndTnbDownSippu}) {  // the first stagger with the longest extension: never stopped, never at its end
      float g = 0;
      for (unsigned age = 0; age < policy::kBreakTicks + policy::kFinisherExtMax; ++age) {
        const float r = stagger_rate(age < policy::kBreakTicks ? age : policy::kBreakTicks - 1, g, end);
        CHECK(r > 0.0f, "stagger clip %.0f: rate 0 at age %u", end, age);
        g += r;
      }
      CHECK(g < end, "stagger clip %.0f never reaches its end inside the window + extension (%.2f)", end, g);
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
  // ---------------------------------------------------------------- 8. the OWN offensive controller: 25/25/25/25 conditional, no native dependency
  {
    Initiative in;
    in.rng.seed(0x1234);
    InitView v;
    v.phase2 = true, v.dist = 250, v.front = true;
    std::uint64_t now = 1000;
    unsigned combos = 0, singles = 0, starts = 0, max_gap = 0, min_gap = ~0u, last = 0;
    for (unsigned i = 0; i < 400000; ++i, ++now) {
      v.now = now;
      const Decision d = in.decide(v);
      if (d.pick != Pick::None) {
        ++starts, (d.pick == Pick::Combo ? combos : singles)++;
        if (last && now - last > max_gap) max_gap = static_cast<unsigned>(now - last);
        if (last && now - last < min_gap) min_gap = static_cast<unsigned>(now - last);
        last = static_cast<unsigned>(now);
        CHECK(d.pick != Pick::Combo || (d.idx >= 4 && d.idx < 8), "phase-2 combos only");
        CHECK(d.pick != Pick::Single || d.idx == kIdxF2Thrust, "FIX03: the only ordinary phase-2 single is the thrust (F2-SINGLE-B never: got %u)", d.idx);
        in.started(now), in.ended(now + 60, true), now += 60;
      }
    }
    for (unsigned k = 0; k < 4; ++k) {
      const double pk = 100.0 * in.picked[k] / in.combos;
      CHECK(pk > 24.0 && pk < 26.0, "combo %u picked %.2f%% (conditional equity)", k + 1, pk);
      std::printf("SELECT F2 combo %u: %.2f%% of the combos\n", k + 1, pk);
    }
    const double sp = 100.0 * singles / starts;
    CHECK(sp > 23.5 && sp < 26.5, "isolated strikes %.2f%% (PROVISIONAL 25)", sp);
    CHECK(max_gap <= 60 + 46 + 1, "never passive: the longest wait between two own offensives is %u ticks (move 60 + gap <= 46)", max_gap);
    CHECK(min_gap >= 60 + 22, "readable: never a new offensive before the gap (min %u ticks)", min_gap);
    std::printf("INITIATIVE F2 starts=%u combos=%u singles=%u max_gap=%u ticks\n", starts, combos, singles, max_gap);
    // no dependency on the native decision: Link cutting / the native cutCount never matter, only busy / unsafe / reach / front / gap / reentry
    Initiative q;
    q.rng.seed(9);
    InitView z;
    z.phase2 = false, z.dist = 300, z.front = true, z.now = 10;
    z.busy = true;
    CHECK(q.decide(z).why == NoInit::Busy, "busy (owned move / break / execution) never starts");
    z.busy = false, z.safe = false;
    CHECK(q.decide(z).why == NoInit::Unsafe, "event / unsafe world never starts");
    z.safe = true, z.dist = 900;
    CHECK(q.decide(z).why == NoInit::Far, "far: the native chase approaches");
    z.dist = 300, z.front = false;
    CHECK(q.decide(z).why == NoInit::NotFront, "never attacks through its back");
    z.front = true;
    q.reentry(10);
    CHECK(q.decide(z).why == NoInit::Reentry, "re-entry after a recovery / getup / resurrection: no attack, no kick");
    z.now = 10 + policy::kReentryTicks;
    const Decision d1 = q.decide(z);
    CHECK(d1.pick != Pick::None && (d1.pick == Pick::Single ? d1.idx == 8 || d1.idx == 9 : d1.idx < 4), "phase-1 picks phase-1 plans");
    // the old 40 % share and the 120-tick combo cooldown are gone: with the gap elapsed, 100 % of the decision points decide (combo or single)
    Initiative r;
    r.rng.seed(77);
    InitView y;
    y.phase2 = true, y.dist = 200, y.front = true;
    unsigned decided = 0;
    for (unsigned i = 0; i < 1000; ++i) {
      y.now = 100000 + i * 1000;
      decided += r.decide(y).pick != Pick::None;
    }
    CHECK(decided == 1000, "every free decision point decides (%u/1000): no hidden native share", decided);
    // plans: 8 combos + 4 singles, the singles are ONE native strike
    CHECK(kPlans == 14 && combo_of(8).n_moves == 1 && combo_of(11).moves[0] == Move::F2B && combo_of(10).moves[0] == Move::F2A && combo_of(12).moves[0] == Move::F2C &&
              combo_of(13).moves[0] == Move::F1Shield && !combo_of(13).phase2 && combo_of(12).phase2, "single / counter plans (12 kick, 13 bash: counter only)");
    Rng a, b;
    a.seed(7), b.seed(8);
    CHECK(a.next() != b.next(), "instances do not share a stream");
  }
  // ---------------------------------------------------------------- 8b. Shield Attack authority (regression of the 0.13.9 loop)
  {
    unsigned helm = 0, rejected = 0;
    const Bk states[] = {Bk::Combat, Bk::Pending, Bk::Recover, Bk::StaggerB, Bk::Exec};
    for (Bk st : states)
      for (int p2 = 0; p2 < 2; ++p2)
        for (int fresh = 0; fresh < 2; ++fresh) {
          const Sa v = sa_verdict(p2 != 0, st, fresh != 0);
          CHECK(v != Sa::HelmPrep, "Shield Attack in state %s phase2=%d new=%d must not prepare anything", name(st), p2, fresh);
          rejected += v != Sa::HelmPrep;
        }
    CHECK(sa_verdict(true, Bk::StaggerA, true) == Sa::HelmPrep && ++helm, "the ONE legitimate route: new Shield Attack, first stagger, phases 2-3");
    CHECK(sa_verdict(true, Bk::StaggerA, false) == Sa::Reject, "a Shield Attack started BEFORE the break (not new) is rejected");
    CHECK(sa_verdict(false, Bk::StaggerA, true) == Sa::Reject, "phase 1: no Helm Splitter executor");
    // simulated spam: sword / Shield Attack alternated 200 times against a Darknut in combat: posture only from swords, never a forced guard
    Posture post;
    post.reset(policy::kPosture2Max);
    std::uint64_t key = 0, blocked_key = ~0ull;
    unsigned forced_guard = 0;
    for (unsigned i = 0; i < 200; ++i) {
      ++key;  // a new Link action
      const bool sa = i % 2 == 1;
      if (sa) {
        if (sa_verdict(true, Bk::Combat, true) == Sa::HelmPrep) ++forced_guard;
      } else if (key != blocked_key) {
        blocked_key = key;
        post.gain(policy::kGainBlocked2);
      }
      if (post.latched) break;
    }
    CHECK(forced_guard == 0, "no Shield Attack forced the guard");
    CHECK(post.points() == 160 && post.latched, "only the 100 blocked swords counted (+3 each, saturating at 160)");
    std::printf("SHIELD_ATTACK rejected_states=%u legitimate=%u\n", rejected, helm);
  }
  // ---------------------------------------------------------------- 8c. FIX04 defense (F1/F2/F3): ordinal 0/0/0/25/45/60, the exchange, REAL confirmed parry, one counter
  {
    CHECK(policy::kParryOrdinalPct[0] == 0 && policy::kParryOrdinalPct[1] == 0 && policy::kParryOrdinalPct[2] == 0 && policy::kParryOrdinalPct[3] == 25 && policy::kParryOrdinalPct[4] == 45 &&
              policy::kParryOrdinalPct[5] == 60, "ordinal table 0/0/0/25/45/60");
    CHECK(policy::kLinkEnemyParry1 == 20 && policy::kLinkEnemyParry2 == 25, "Link +20 (F1) / +25 (F2-F3) on a real enemy parry");
    // ordinals advance ONLY with real defended contacts; the same swing never twice; whiffs and pauses
    Defense st;
    std::uint64_t key = 100;
    unsigned ords[8] = {};
    for (unsigned i = 0; i < 8; ++i, ++key) {
      ords[i] = st.on_link_swing(1000 + i * 15, key);
      CHECK(st.on_link_swing(1000 + i * 15 + 1, key) == ords[i], "the same swing key seen again keeps its ordinal");
      CHECK(st.on_contact(key) && !st.on_contact(key), "one chain step per swing (a second contact / hook / collider of the same swing never counts)");
      const unsigned w0 = st.whiffs, c0 = st.chain;
      CHECK(st.on_link_swing(1000 + i * 15 + 2, key) == ords[i] && st.whiffs == w0 && st.chain == c0 && st.cur_contacted, "the same swing reported after its contact: same ordinal, nothing changes");
    }
    CHECK(ords[0] == 1 && ords[1] == 2 && ords[2] == 3 && ords[3] == 4 && ords[4] == 5 && ords[5] == 6 && ords[6] == 6 && ords[7] == 6, "ordinals 1..6+ (got %u %u %u %u %u %u %u)", ords[0], ords[1],
          ords[2], ords[3], ords[4], ords[5], ords[6]);
    Defense wh;
    CHECK(wh.on_link_swing(10, 1) == 1 && wh.on_contact(1) && wh.on_link_swing(25, 2) == 2, "contact -> ordinal 2");
    CHECK(wh.on_link_swing(40, 3) == 2, "a whiff (swing 2 without a defended contact) never advances the ordinal");
    CHECK(wh.holds(41) && wh.whiffs == 1, "one empty swing keeps the exchange");
    CHECK(wh.on_link_swing(55, 4) == 1 && !wh.holds(56) && wh.whiff_breaks == 1, "two empty swings in a row end the chain and the exchange (no guard-lock by swinging at the air)");
    CHECK(wh.on_contact(4) && wh.holds(57) && wh.on_link_swing(70, 5) == 2, "a real contact restarts it");
    CHECK(!wh.on_contact(99), "a contact of another / older swing never counts");
    CHECK(wh.on_link_swing(70 + policy::kStreakGapTicks + 1, 6) == 1 && wh.gap_breaks == 1, "a pause > 1.5 s starts a new chain");
    CHECK(wh.holds(70 + policy::kStreakGapTicks + 1 + policy::kStreakGapTicks) && !wh.holds(70 + policy::kStreakGapTicks + 2 + policy::kStreakGapTicks), "the exchange lets go 1.5 s after the last swing");
    // ONE roll per swing key: a second verdict request for the same swing never rolls again
    {
      Defense x;
      x.rng.seed(5);
      for (unsigned i = 0; i < 3; ++i) x.on_link_swing(100 + i * 10, 50 + i), x.on_contact(50 + i);
      const unsigned o = x.on_link_swing(140, 60);
      const std::uint64_t s0 = x.rng.s;
      (void)x.on_swing(140, o, true, true);
      const std::uint64_t s1 = x.rng.s;
      CHECK(x.on_swing(141, o, true, true) == Dv::None && x.rng.s == s1 && x.rerolls_refused == 1 && s1 != s0, "the same swing is never rolled twice (no per-frame / per-hook roll)");
    }
    // the EFFECTIVE attempt probability per ordinal (phase 1 and phases 2-3): exactly the table; ordinals 1-3 never roll, never parry
    for (unsigned ord = 1; ord <= 6; ++ord)
      for (int ph = 0; ph < 2; ++ph) {
        unsigned att = 0, ordinary = 0, rolls = 0;
        const unsigned N = 40000;
        for (unsigned i = 0; i < N; ++i) {
          Defense x;
          x.rng.seed(i * 7919u + ord * 31u + ph);
          for (unsigned j = 1; j < ord; ++j) x.on_link_swing(1000 + j * 12, j), x.on_contact(j);
          const unsigned o = x.on_link_swing(1000 + ord * 12, 100);
          CHECK(o == ord, "ordinal setup");
          const Dv v = x.on_swing(1000 + ord * 12, o, true, ph == 1);
          att += v == Dv::Parry, ordinary += v == Dv::Guard, rolls += unsigned(x.rolls[ord - 1]);
          if (v == Dv::Parry) CHECK(x.armed && x.window_key == 100 && x.window_from == 1000 + ord * 12 + policy::kParryPrepTicks, "armed window bound to this swing");
        }
        const double pa = 100.0 * att / N;
        std::printf("DEFENSE F%d ordinal=%u eligible=%u rolls=%u attempt=%.2f%% (target %d) ordinary_guard=%.2f%%\n", ph ? 2 : 1, ord, N, rolls, pa, policy::kParryOrdinalPct[ord - 1], 100.0 * ordinary / N);
        CHECK(std::fabs(pa - policy::kParryOrdinalPct[ord - 1]) < 1.0, "F%d ordinal %u: attempt %.2f%% != %d%%", ph ? 2 : 1, ord, pa, policy::kParryOrdinalPct[ord - 1]);
        if (ord <= 3) CHECK(att == 0 && rolls == 0, "ordinal %u: never a parry, never a parry roll", ord);
        if (!ph) CHECK(ordinary == 0, "phase 1: no proactive ordinary guard roll (the native shield already blocks)");
      }
    // no parry without the shield (phase 1, mNextBreakPart >= 11 / part 12 broken): never an attempt
    unsigned noshield = 0;
    for (unsigned i = 0; i < 5000; ++i) {
      Defense x;
      x.rng.seed(i + 3);
      for (unsigned j = 1; j < 6; ++j) x.on_link_swing(1000 + j * 12, j), x.on_contact(j);
      noshield += x.on_swing(1100, x.on_link_swing(1100, 77), false, false) != Dv::None;
    }
    CHECK(noshield == 0, "phase 1 without the shield: no parry, no guard roll (%u)", noshield);
    // phases 2-3: a failed / impossible parry roll may still raise an ORDINARY guard (no window), rising with Link's pressure (PROVISIONAL 20 -> 75 %)
    Defense hp;
    for (unsigned c = 0; c < 6; ++c) hp.note_contact(900 + c * 10);
    CHECK(hp.pressure(960) == 6 && hp.pressure(1200) == 0, "pressure counts the last 90 ticks");
    unsigned og_lo = 0, og_hi = 0, n_lo = 0, n_hi = 0;
    for (unsigned i = 0; i < 20000; ++i) {
      Defense x, y = hp;
      x.rng.seed(i + 7), y.rng.seed(i + 7);
      const Dv a = x.on_swing(960, x.on_link_swing(960, 5), true, true), b = y.on_swing(960, y.on_link_swing(960, 5), true, true);
      n_lo += a != Dv::Parry, og_lo += a == Dv::Guard, n_hi += b != Dv::Parry, og_hi += b == Dv::Guard;
    }
    std::printf("DEFENSE ordinary guard (no parry): no pressure %.1f%%, 6 contacts %.1f%%\n", 100.0 * og_lo / n_lo, 100.0 * og_hi / n_hi);
    CHECK(std::fabs(100.0 * og_lo / n_lo - 20.0) < 1.5 && std::fabs(100.0 * og_hi / n_hi - 75.0) < 1.5, "ordinary guard 20%% -> 75%% with pressure");
    // a REAL parry: the window of THIS swing + the rebound; one per window; the same tick again kept, never counted; the credit and the counter only after
    // Link's PROC_CUT_REVERSE is read; the counter lands >= kCounterLandTicks after the rebound; break priority keeps only the credit.
    auto armed_def = [](std::uint64_t seed) {
      Defense p;
      p.rng.seed(seed);
      std::uint64_t k = 1, tt = 100;
      for (unsigned tries = 0; !p.armed && tries < 500; ++tries, ++k, tt += 12) {
        const unsigned o = p.on_link_swing(tt, k);
        if (p.on_swing(tt, o, true, true) != Dv::Parry) p.on_contact(k);
      }
      return p;
    };
    Defense p = armed_def(3);
    CHECK(p.armed && p.chain >= 3, "armed only from the 4th defended swing (chain %u)", p.chain);
    const std::uint64_t t0 = p.guard_tick, wkey = p.window_key;
    CHECK(p.rebound(t0 + 1) == 0, "prep delay: no parry before the window opens");
    CHECK(p.rebound(p.window_from) == 1 && p.parries == 1 && p.cp == Cp::Await && p.chain == 0 && p.parried_key == wkey, "rebound inside the window = parry rebound, chain ends, awaiting confirmation");
    CHECK(p.rebound(p.window_from) == 2 && p.parries == 1, "the same parry seen again in the same tick (another collider): kept, not counted");
    CHECK(p.rebound(p.window_from + 1) == 0, "one parry per window (a later contact is an ordinary block)");
    CHECK(!p.on_contact(wkey), "the parried contact itself never starts the next chain");
    {  // even when the parried swing is reported again afterwards (another hook / a stale key), its contact never re-enters the chain
      Defense q = p;
      q.on_link_swing(p.window_from + 1, wkey);
      CHECK(!q.on_contact(wkey) && q.chain == 0, "the parried swing never counts again");
    }
    CHECK(p.parry_hold(p.window_from) && p.parry_hold(p.window_from + 1) && !p.parry_hold(p.window_from + 2), "the parry contact stays a guard for its damage_check (this tick and the next)");
    CHECK(!p.confirm(p.window_from, false, true, true) && p.cp == Cp::Await && p.due(p.window_from + 100) == -1, "no credit, no counter until Link is really in PROC_CUT_REVERSE");
    CHECK(p.confirm(p.window_from + 1, true, true, true) && p.cp == Cp::Scheduled && !p.confirm(p.window_from + 1, true, true, true), "exactly ONE credit");
    const int k1 = p.counter_idx;
    CHECK((k1 == int(kIdxF2Kick) || k1 == int(kIdxF2Thrust) || k1 == int(kIdxF2Double)), "a phase-2 counter");
    CHECK(p.due(p.counter_at - 1) == -1 && p.due(p.counter_at) == k1 && p.due(p.counter_at) == -1, "the counter starts once, at its tick, never in the rebound tick (at=%llu rebound=%llu)",
          static_cast<unsigned long long>(p.counter_at), static_cast<unsigned long long>(p.parry_tick));
    CHECK(p.counter_at > p.parry_tick, "never in the rebound tick");
    {  // unconfirmed: the window expires, no credit, no counter
      Defense u = armed_def(11);
      const std::uint64_t w = u.window_from;
      CHECK(u.rebound(w) == 1, "rebound");
      u.tick(w + policy::kCounterWaitTicks + 1);
      CHECK(u.cp == Cp::None && u.unconfirmed == 1 && !u.confirm(w + 9, true, true, true) && u.confirmed == 0, "no PROC_CUT_REVERSE in time: no credit, no counter");
    }
    {  // a posture break preempts: the counter is cancelled, the real rebound keeps its ONE credit
      Defense bk = armed_def(13);
      const std::uint64_t w = bk.window_from;
      CHECK(bk.rebound(w) == 1, "rebound");
      bk.cancel();
      CHECK(bk.cp == Cp::CreditOnly && !bk.busy(), "break priority: no counter pending, the Darknut free for its stagger");
      CHECK(bk.confirm(w + 1, true, true, true) && bk.cp == Cp::None && bk.counter_idx == -1 && bk.due(w + 100) == -1, "credit once, no counter after a break");
      Defense sc = armed_def(17);
      sc.rebound(sc.window_from), sc.confirm(sc.window_from, true, true, true);
      sc.cancel();
      CHECK(sc.cp == Cp::None && sc.counter_dropped == 1 && sc.due(sc.counter_at) == -1, "a scheduled counter is dropped by a break / defeat: never a late counter");
      Defense lt = armed_def(19);
      lt.rebound(lt.window_from), lt.confirm(lt.window_from, true, true, true);
      lt.tick(lt.counter_at + policy::kCounterLateTicks + 1);
      CHECK(lt.cp == Cp::None && lt.due(lt.counter_at + 20) == -1, "a counter that could not start in time is dropped");
    }
    // the cooldown after a REAL parry: even a fast new chain cannot arm a second window before kParryCooldownTicks
    {
      unsigned armed_in_cooldown = 0;
      for (unsigned i = 0; i < 2000; ++i) {
        Defense x = armed_def(31 + i);
        const std::uint64_t r0 = x.window_from;
        x.rebound(r0), x.confirm(r0, true, true, true), (void)x.due(x.counter_at);
        std::uint64_t kk = 9000, tt = x.counter_at + 1;
        for (unsigned j = 0; j < 3; ++j, ++kk, tt += 2) x.on_link_swing(tt, kk), x.on_contact(kk);
        for (unsigned j = 0; j < 5 && tt < r0 + policy::kParryCooldownTicks; ++j, ++kk, tt += 2)
          if (x.on_swing(tt, x.on_link_swing(tt, kk), true, true) == Dv::Parry) ++armed_in_cooldown;
          else x.on_contact(kk);
      }
      CHECK(armed_in_cooldown == 0, "cooldown after a parry (%u armed inside it)", armed_in_cooldown);
    }
    // a window belongs to its swing: the next swing closes it (never a window catching another swing's contact)
    {
      Defense w = armed_def(23);
      const std::uint64_t from = w.window_from;
      w.on_link_swing(from, w.window_key + 1000);
      CHECK(!w.armed && w.rebound(from) == 0, "a new swing closes the previous swing's window");
    }
    // counters: every candidate plan lands its first impact >= kCounterLandTicks after the rebound (Link's recoil is cancelable after frame 15), as early
    // as that allows (no dead wait), and the adapter's play model agrees with the core
    for (unsigned idx : {kIdxF2Kick, kIdxF2Thrust, kIdxF2Double, kIdxF1Bash, kIdxF1SingleA, kIdxF1SingleB}) {
      const SimOut o = simulate(combo_of(idx), plan(combo_of(idx), idx));
      const unsigned land = counter_delay(idx) + static_cast<unsigned>(o.tick[0]);
      std::printf("COUNTER %s first_impact_tick=%u delay=%u lands=%u ticks after the rebound (Link control back after %u)\n", combo_of(idx).name, first_impact_tick(idx), counter_delay(idx), land,
                  policy::kRecoilCancelFrames + 1);
      CHECK(o.ok && static_cast<unsigned>(o.tick[0]) == first_impact_tick(idx), "%s core first impact %u == simulated %.0f", combo_of(idx).name, first_impact_tick(idx), o.tick[0]);
      CHECK(land >= policy::kCounterLandTicks && land >= policy::kRecoilCancelFrames + 8 && counter_delay(idx) >= 1, "%s lands %u ticks after the rebound", combo_of(idx).name, land);
      CHECK(counter_delay(idx) == 1 || land == policy::kCounterLandTicks, "%s: no dead wait beyond the landing rule", combo_of(idx).name);
      if (idx == kIdxF2Double) CHECK(o.n == 2, "the counter double is COMPLETE: two cuts, two impacts");
    }
    // counters by thirds among the eligible: phases 2-3 kick / thrust / double; phase 1 bash / lateral / vertical (never the bash without the shield)
    unsigned c2[3] = {}, c1[3] = {}, c1n[3] = {};
    const unsigned N = 30000;
    for (unsigned i = 0; i < N; ++i) {
      Defense x, y, z;
      x.rng.seed(i * 31 + 5), y.rng.seed(i * 37 + 11), z.rng.seed(i * 41 + 13);
      x.cp = y.cp = z.cp = Cp::Await, x.counter_by = y.counter_by = z.counter_by = 1u << 30;
      x.confirm(1, true, true, true), y.confirm(1, true, false, true), z.confirm(1, true, false, false);
      const int a2 = x.counter_idx, a1 = y.counter_idx, a0 = z.counter_idx;
      ++c2[a2 == int(kIdxF2Kick) ? 0 : a2 == int(kIdxF2Thrust) ? 1 : 2];
      ++c1[a1 == int(kIdxF1Bash) ? 0 : a1 == int(kIdxF1SingleA) ? 1 : 2];
      ++c1n[a0 == int(kIdxF1Bash) ? 0 : a0 == int(kIdxF1SingleA) ? 1 : 2];
      CHECK(combo_of(unsigned(a2)).phase2 && !combo_of(unsigned(a1)).phase2 && !combo_of(unsigned(a0)).phase2, "counter plans of the right phase");
    }
    for (unsigned k = 0; k < 3; ++k)
      CHECK(std::fabs(100.0 * c2[k] / N - 33.33) < 1.5 && std::fabs(100.0 * c1[k] / N - 33.33) < 1.5, "counter thirds %u: F2 %.1f%% F1 %.1f%%", k, 100.0 * c2[k] / N, 100.0 * c1[k] / N);
    CHECK(c1n[0] == 0 && std::fabs(100.0 * c1n[1] / N - 50.0) < 1.5, "phase 1 without the shield: lateral / vertical 50/50, never the bash");
    std::printf("COUNTERS F2 kick/thrust/double=%.1f/%.1f/%.1f%% F1 bash/lateral/vertical=%.1f/%.1f/%.1f%% F1_no_shield lateral/vertical=%.1f/%.1f%%\n", 100.0 * c2[0] / N,
                100.0 * c2[1] / N, 100.0 * c2[2] / N, 100.0 * c1[0] / N, 100.0 * c1[1] / N, 100.0 * c1[2] / N, 100.0 * c1n[1] / N, 100.0 * c1n[2] / N);
    // an armed window that runs out closes; the next swing rolls again (each swing its own roll)
    Defense w;
    w.armed = true, w.window_from = 0, w.window_to = 20;
    w.tick(20);
    CHECK(!w.armed && w.expired == 1 && w.cooldown_until == 0, "expired window: closed, no cooldown");
    // a long B-spam session with every swing defended: parries never before the 4th, per ordinal within the table, one counter per confirmed parry
    for (int ph = 0; ph < 2; ++ph) {
      Defense ses;
      ses.rng.seed(2026 + ph);
      unsigned parries = 0, counters = 0, swings = 0;
      std::uint64_t k = 1;
      for (std::uint64_t tt = 1000; tt < 1000 + 30 * 1200; tt += 12, ++k) {
        ses.tick(tt);
        const unsigned o = ses.on_link_swing(tt, k);
        ++swings;
        if (ses.on_swing(tt, o, true, ph == 1) == Dv::Parry) {
          const std::uint64_t hit = ses.window_from + 3;
          ses.tick(hit);
          if (ses.rebound(hit) == 1 && ses.confirm(hit + 1, true, ph == 1, true)) {
            ++parries;
            counters += ses.due(ses.counter_at) >= 0;
          }
          tt = ses.counter_at + 40;  // the counter plays
        } else {
          ses.on_contact(k);
        }
      }
      std::printf("DEFENSE_SESSION F%d swings=%u parries=%u counters=%u parries_by_ordinal=%llu/%llu/%llu/%llu/%llu/%llu contacts_by_ordinal=%llu/%llu/%llu/%llu/%llu/%llu\n", ph ? 2 : 1, swings,
                  parries, counters, (unsigned long long)ses.parries_at[0], (unsigned long long)ses.parries_at[1], (unsigned long long)ses.parries_at[2], (unsigned long long)ses.parries_at[3],
                  (unsigned long long)ses.parries_at[4], (unsigned long long)ses.parries_at[5], (unsigned long long)ses.contacts[0], (unsigned long long)ses.contacts[1],
                  (unsigned long long)ses.contacts[2], (unsigned long long)ses.contacts[3], (unsigned long long)ses.contacts[4], (unsigned long long)ses.contacts[5]);
      CHECK(parries == counters && parries > 0, "one counter per confirmed parry");
      CHECK(ses.parries_at[0] == 0 && ses.parries_at[1] == 0 && ses.parries_at[2] == 0, "never a parry on the first three defended swings");
      for (unsigned o = 3; o < 6; ++o) {
        const double cond = 100.0 * ses.parries_at[o] / (ses.swings[o] ? ses.swings[o] : 1);
        CHECK(cond <= policy::kParryOrdinalPct[o] + 5.0, "ordinal %u: %.1f%% parries per eligible swing (table %d)", o + 1, cond, policy::kParryOrdinalPct[o]);
      }
    }
  }
  // ---------------------------------------------------------------- 8e. FIX04 the exchange owns the initiative: no offensive decision over Link's valid chain
  {
    Initiative in;
    in.rng.seed(4242);
    InitView v;
    v.phase2 = true, v.dist = 200, v.front = true;
    unsigned refused = 0, decided = 0;
    for (std::uint64_t now = 5000; now < 5600; ++now) {  // gap long elapsed: without the exchange every point decides
      v.now = now, v.defending = true;
      const Decision d = in.decide(v);
      refused += d.why == NoInit::Defending, decided += d.pick != Pick::None;
    }
    CHECK(decided == 0 && refused == 600, "while Link's chain owns the exchange: 0 COMBO / SINGLE starts (%u refused)", refused);
    v.defending = false, v.now = 5600;
    in.ended(5600, true);
    CHECK(in.decide(v).why == NoInit::Gap, "released: the controller resumes after its ordinary gap, never at once on a timer that ran out during the defense");
    v.now = 5600 + policy::kGap2Max + 1;
    CHECK(in.decide(v).pick != Pick::None, "then the ordinary initiative comes back");
    // a whole simulated exchange: Link chains defended swings every 12 ticks, the Darknut decides every tick; no decision while held, decisions after
    Defense d;
    Initiative i2;
    i2.rng.seed(77);
    unsigned during = 0, after = 0;
    std::uint64_t k = 1, last_swing = 0;
    for (std::uint64_t now = 108; now < 1300; ++now) {  // from Link's first swing on
      if (now < 700 && now % 12 == 0) d.on_link_swing(now, k), d.on_contact(k), ++k, last_swing = now;
      InitView x;
      x.phase2 = true, x.dist = 200, x.front = true, x.now = now, x.defending = d.holds(now);
      const Decision r = i2.decide(x);
      if (r.pick != Pick::None) (now <= last_swing + policy::kStreakGapTicks ? during : after)++, i2.started(now), i2.ended(now + 60, true), now += 60;
    }
    CHECK(during == 0 && after > 0, "no invasive OWN_INITIATIVE during the chain (%u), the offensive comes back after it (%u)", during, after);
  }
  // ---------------------------------------------------------------- 8d. execution contact verdict and its priorities
  {
    CHECK(exec_verdict(true, Bk::StaggerA, true, false, false) == Ex::Fall, "Mortal Draw contact in the first stagger");
    CHECK(exec_verdict(true, Bk::StaggerB, false, true, false) == Ex::Fall, "Helm Splitter contact in the second stagger");
    CHECK(exec_verdict(true, Bk::StaggerA, false, true, false) == Ex::None, "a Helm Splitter without the Shield Attack preparation is nothing");
    CHECK(exec_verdict(true, Bk::StaggerB, true, false, false) == Ex::None, "a Mortal Draw belongs to the first stagger only");
    CHECK(exec_verdict(true, Bk::Combat, true, true, false) == Ex::None, "no execution outside a break (a native Mortal Draw is an ordinary hit)");
    CHECK(exec_verdict(false, Bk::StaggerA, true, true, false) == Ex::None, "no execution in phase 1");
    CHECK(exec_verdict(true, Bk::StaggerA, false, false, false) == Ex::None, "an ordinary cut on the broken Darknut never makes him fall");
    CHECK(exec_verdict(true, Bk::StaggerA, true, false, true) == Ex::Lethal && exec_verdict(true, Bk::StaggerB, false, true, true) == Ex::Lethal,
          "execution + last resistance: the real defeat has priority (never a getup with 0)");
    Receipt r;
    r.issue(5, 100);
    CHECK(r.valid(5, 100 + policy::kMdReceiptTtl) && !r.valid(5, 101 + policy::kMdReceiptTtl) && !r.valid(6, 110), "Mortal Draw receipt: this generation, bounded time");
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
