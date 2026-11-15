// LIGHT FADES TWICE 0.13.14 (DKN01 FIX05) LGC01: deterministic laboratory of Link's guard cancel core (src/lgc_core.hpp). No game, no SDK.
// It replays the NATIVE frame model of each ordinary cut (HIO 2.0.3: start / speed / AT / cancel / end frames, lunge 2.2 decel), measures R -> guard latency
// before (native: the cut runs to its clip end with R held and the stick still) and after, and fails on any broken rule (H/I matrices, technical part).
// Build: g++ -std=c++20 tests/lgc_lab.cpp  (CI: MSVC /W4, target post05_lgc_lab).
#include "../src/lgc_core.hpp"
#include <cstdio>
#include <cmath>

static int g_fail = 0, g_checks = 0;
#define CHECK(c, ...)                                 \
  do {                                                \
    ++g_checks;                                       \
    if (!(c)) {                                       \
      ++g_fail;                                       \
      std::printf("FAIL %s:%d ", __FILE__, __LINE__); \
      std::printf(__VA_ARGS__);                       \
      std::printf("\n");                              \
    }                                                 \
  } while (0)
using namespace lgc;

// The five ordinary cuts, measured from d_a_alink_HIO_data.inc (2.0.3): clip start / rate / end, AT frames, cancel frame, lunge speed.
struct CutHio { const char* name; int type; float start, rate, end, at_s, at_e, cancel, lunge; };
static const CutHio kCuts[5] = {
    {"NM_VERTICAL", kCutNmVertical, 2.0f, 1.2f, 27.0f, 5.0f, 10.0f, 16.0f, 5.0f},
    {"NM_LEFT", kCutNmLeft, 2.0f, 1.2f, 27.0f, 7.0f, 13.0f, 16.0f, 3.0f},
    {"NM_RIGHT", kCutNmRight, 3.0f, 1.2f, 27.0f, 7.0f, 12.5f, 16.0f, 3.0f},
    {"COMBO_STAB", kCutComboStab, 3.0f, 1.1f, 31.0f, 3.0f, 10.0f, 17.0f, 10.0f},
    {"NM_STAB", kCutNmStab, 4.0f, 1.1f, 27.0f, 3.0f, 10.0f, 17.0f, 8.0f},
};
// One simulated cut: the procedure is called once per tick (frame = clip frame BEFORE this tick's play), Link's lunge set at the first AT tick and
// decelerated 2.2 per tick. r_at = tick (relative) of the R press, b_at = tick of a B press (-1 none). Returns the tick of the cancel (-1: none) and what
// happened; also checks that no AT tick ever comes AFTER a cancel and that a PREP cancel never had an AT tick.
struct SimRes { int cancel_tick = -1, hold_ticks = 0, at_ticks_before = 0, native_recovery_calls = 0; bool bash = false; Region region = Region::Prep; bool ok = true; float frame_at_cancel = 0; };
static SimRes run_cut(Ctl& c, const CutHio& h, int r_at, int b_at, std::uint64_t t0, bool safe = true, bool lock = true, bool ground = true, bool sword = true,
                      int r_release = -1, int pause_at = -1) {
  SimRes s;
  float frame = h.start, speed = 0;
  bool lunged = false;
  std::uint64_t tick = t0, r_down = 0;
  bool r = false;
  for (int k = 0; k < 60 && frame < h.end; ++k, ++tick) {
    if (k == pause_at) tick += 2;  // a native hitstop: Link frozen 2 ticks, the simulation tick runs on
    if (k == r_at) r = true, r_down = tick;
    if (k == r_release) r = false;
    View v;
    v.cut_normal = true, v.cut = h.type, v.combo = 1, v.safe = safe, v.human = true, v.sword = sword, v.ground = ground, v.lock = lock && r;
    v.r_held = r, v.r_down_tick = r_down, v.b_trig = k == b_at, v.frame = frame, v.at_start = h.at_s, v.at_end = h.at_e, v.speed = speed, v.tick = tick;
    const Out o = c.step(v);
    if (o.v == Verdict::Cancel) {
      s.cancel_tick = k, s.bash = o.bash, s.region = o.region, s.frame_at_cancel = frame;
      if (o.region == Region::Active) s.ok = false;                                // never inside the AT
      if (o.region == Region::Prep && s.at_ticks_before) s.ok = false;            // a PREP cancel never had an AT tick
      if (v.b_trig) s.ok = false;                                                  // never in a tick carrying a new B
      if (o.region == Region::Recovery && !s.native_recovery_calls) s.ok = false;  // the first recovery call (last AT contact: rebound / parry) is native
      return s;
    }
    if (o.v == Verdict::Hold) {
      ++s.hold_ticks;
      if (!s.native_recovery_calls) s.ok = false;  // never skip the call that evaluates the last AT contact
      frame += h.rate;
      continue;  // the procedure body is skipped this tick: the clip still plays
    }
    if (region(frame, h.at_s, h.at_e) == Region::Recovery) ++s.native_recovery_calls;  // the native body ran in the recovery (incl. changeCutReverseProc)
    // the native procedure body: AT while at_s <= frame < at_e (lunge at the first AT tick), then the clip plays
    if (frame >= h.at_s && frame < h.at_e) {
      ++s.at_ticks_before;
      if (!lunged) speed = h.lunge, lunged = true;
    }
    speed = speed > 2.2f ? speed - 2.2f : 0.0f;
    frame += h.rate;
  }
  return s;
}
static int native_guard_tick(const CutHio& h) {  // native: R held, stick still -> the cut runs to its clip end, then checkNextAction(0) -> the guard proc
  float frame = h.start;
  int k = 0;
  while (frame < h.end) frame += h.rate, ++k;
  return k;
}

int main() {
  // ---------------------------------------------------------------- 1. the cut table is the one the core pins
  CHECK(ordinary_cut(kCutNmVertical) && ordinary_cut(kCutNmLeft) && ordinary_cut(kCutNmRight) && ordinary_cut(kCutNmStab) && ordinary_cut(kCutComboStab), "the five ordinary cuts");
  for (int c = 0; c < 0x40; ++c)
    if (c != kCutNmVertical && c != kCutNmLeft && c != kCutNmRight && c != kCutNmStab && c != kCutComboStab) CHECK(!ordinary_cut(c), "cut type 0x%02x is not an ordinary cut", c);
  CHECK(region(4.9f, 5, 10) == Region::Prep && region(5, 5, 10) == Region::Active && region(9.99f, 5, 10) == Region::Active && region(10, 5, 10) == Region::Recovery, "regions");
  // ---------------------------------------------------------------- 2. R latency matrix: every cut, every press tick; before (native) and after
  std::printf("LGC01 R -> guard latency (ticks from the R press to the guard procedure; native = the cut's clip end with R held, stick still)\n");
  for (const CutHio& h : kCuts) {
    const int nat_end = native_guard_tick(h);
    int n = 0, sum_before = 0, sum_after = 0, max_before = 0, max_after = 0, prep = 0, rec = 0;
    for (int r_at = 0; r_at < nat_end; ++r_at) {
      Ctl c;
      const SimRes s = run_cut(c, h, r_at, -1, 1000);
      CHECK(s.ok, "%s R at %d: a cancel inside the AT / a PREP cancel after an AT / a cancel on a B tick", h.name, r_at);
      CHECK(s.cancel_tick >= r_at, "%s: never before the R press", h.name);
      const int before = nat_end - r_at, after = s.cancel_tick - r_at + 1;  // +1: the guard flag comes with the next setShieldGuard
      CHECK(after <= before, "%s R at %d: never slower than the native (%d > %d)", h.name, r_at, after, before);
      ++n, sum_before += before, sum_after += after, max_before = before > max_before ? before : max_before, max_after = after > max_after ? after : max_after;
      prep += s.region == Region::Prep, rec += s.region == Region::Recovery;
      // the AT is never cut short: a press before / during the AT either cancels in PREP (no AT at all) or after the full AT
      if (s.region == Region::Recovery) CHECK(s.frame_at_cancel >= h.at_e, "%s: recovery cancel only after the AT end", h.name);
    }
    std::printf("  %-11s AT %4.1f..%4.1f clip %4.1f..%4.1f: native avg %.1f max %d -> LGC01 avg %.1f max %d (prep %d, recovery %d cancels)\n", h.name, h.at_s, h.at_e, h.start, h.end,
                double(sum_before) / n, max_before, double(sum_after) / n, max_after, prep, rec);
    CHECK(double(sum_after) / n < double(sum_before) / n - 3.0, "%s: measurably faster on average", h.name);
    CHECK(max_after <= 9, "%s: the worst R -> guard after the change is %d ticks (the AT + lunge, never the dead tail)", h.name, max_after);
    if (h.at_s > h.start + h.rate) CHECK(prep > 0, "%s: an R before the AT cancels the preparation", h.name);
  }
  // ---------------------------------------------------------------- 3. fail-closed contexts: the native cut runs untouched
  for (const CutHio& h : kCuts) {
    struct Ctx { const char* name; bool safe, lock, ground, sword; Why why; } ctxs[4] = {
        {"unsafe (penalty / damage / event / no shield)", false, true, true, true, Why::Unsafe}, {"no lock-on", true, false, true, true, Why::NoLock},
        {"airborne", true, true, false, true, Why::Airborne}, {"not the sword", true, true, true, false, Why::NotSword}};
    for (const Ctx& x : ctxs) {
      Ctl c;
      const SimRes s = run_cut(c, h, 0, -1, 2000, x.safe, x.lock, x.ground, x.sword);
      CHECK(s.cancel_tick < 0 && c.refused[unsigned(x.why)] >= 1, "%s / %s: no cancel, refusal counted", h.name, x.name);
    }
  }
  {  // not an ordinary cut / not the cut procedure: nothing, and the state resets
    Ctl c;
    View v;
    v.cut_normal = true, v.cut = 0x10, v.human = v.safe = v.sword = v.ground = v.lock = v.r_held = true, v.r_down_tick = 5, v.tick = 5;
    CHECK(c.step(v).v == Verdict::Native && c.step(v).why == Why::NotOrdinary, "a finisher / skill cut type: native");
    v.cut_normal = false, v.cut = kCutNmVertical;
    CHECK(c.step(v).v == Verdict::Native, "another procedure (rebound, guard break, finisher, jump, spin...): native");
  }
  // ---------------------------------------------------------------- 4. intent rules: R must be pressed AFTER the cut began, stay held; R held before = native
  for (const CutHio& h : kCuts) {
    Ctl c;
    // R already held when the cut began (an R+B the native did not take as a Shield Attack): the cut is what was asked
    float frame = h.start;
    bool any = false;
    for (int k = 0; k < 30 && frame < h.end; ++k, frame += h.rate) {
      View v;
      v.cut_normal = true, v.cut = h.type, v.combo = 1, v.safe = v.human = v.sword = v.ground = v.lock = v.r_held = true, v.r_down_tick = 3000 - 5;
      v.frame = frame, v.at_start = h.at_s, v.at_end = h.at_e, v.tick = 3000 + k;
      any = any || c.step(v).v == Verdict::Cancel;
    }
    CHECK(!any && c.intents == 0, "%s: an R held before the cut began never cancels it", h.name);
    // R released before the cancel point: the intent dies (no ghost guard)
    Ctl d;
    const int at_first = int(std::ceil((h.at_s - h.start) / h.rate));  // the first tick whose frame is inside the AT
    const SimRes s = run_cut(d, h, at_first /* during the AT */, -1, 4000, true, true, true, true, at_first + 1);
    CHECK(s.cancel_tick < 0 && d.released == 1 && !d.intent && d.refused[unsigned(Why::NoLock)] == 0,
          "%s: R released before the recovery: the intent itself dies (no cancel, not merely a refused lock)", h.name);
  }
  // ---------------------------------------------------------------- 5. R+B: the Shield Attack once, never a cut, never on the B tick, bounded buffer
  for (const CutHio& h : kCuts) {
    const int at0 = int(std::ceil((h.at_s - h.start) / h.rate));  // first AT tick
    {  // R during the AT, then B during the AT: buffered, cancel at the recovery with the bash
      Ctl c;
      const SimRes s = run_cut(c, h, at0, at0 + 1, 5000);
      if (s.cancel_tick >= 0) CHECK(s.bash && s.region == Region::Recovery && c.bashes == 1, "%s: R+B in the AT -> Shield Attack offer at the recovery", h.name);
    }
    {  // R+B pressed together in the recovery: the B tick is held (no combo chain), the next tick cancels with the bash
      Ctl c;
      int rec0 = 0;
      for (float f = h.start; f < h.at_e; f += h.rate) ++rec0;
      const SimRes s = run_cut(c, h, rec0 + 1, rec0 + 1, 6000);
      CHECK(s.hold_ticks == 1 && s.cancel_tick == rec0 + 2 && s.bash, "%s: R+B in the recovery -> one held tick then the Shield Attack offer (hold %d cancel %d)", h.name, s.hold_ticks,
            s.cancel_tick);
    }
    {  // R alone: never a bash
      Ctl c;
      const SimRes s = run_cut(c, h, 0, -1, 7000);
      CHECK(s.cancel_tick >= 0 && !s.bash && c.bashes == 0, "%s: R alone -> guard, never a Shield Attack", h.name);
    }
    {  // B before R (a combo press): not a bash (the bash needs R first), dropped by resetCombo at the cancel
      Ctl c;
      const SimRes s = run_cut(c, h, 2, 1, 8000);
      CHECK(!s.bash, "%s: a B before the R press is a combo press, not a Shield Attack", h.name);
    }
  }
  {  // the bash buffer expires (never a stale Shield Attack)
    Ctl c;
    View v;
    v.cut_normal = true, v.cut = kCutNmVertical, v.combo = 1, v.safe = v.human = v.sword = v.ground = v.lock = true, v.at_start = 5, v.at_end = 10;
    v.r_held = true, v.r_down_tick = 9001, v.frame = 2, v.tick = 9000;
    c.step(v);  // first sight of the cut (R pressed one tick later)
    v.tick = 9001, v.frame = 5, v.b_trig = true;
    CHECK(c.step(v).v == Verdict::Buffer && c.bash, "R then B in the AT: buffered");
    v.b_trig = false;
    for (std::uint64_t t = 9002; t <= 9001 + policy::kBashBufferTicks + 1; ++t) v.tick = t, v.frame = 6, c.step(v);
    CHECK(!c.bash && c.bash_expired == 1, "the R+B buffer expires after %u ticks", policy::kBashBufferTicks);
  }
  // ---------------------------------------------------------------- 6. one cancel per cut, a new cut is a new instance, hitstop does not reset it
  for (const CutHio& h : kCuts) {
    Ctl c;
    const SimRes s = run_cut(c, h, 0, -1, 10000, true, true, true, true, -1, 1);
    CHECK(s.cancel_tick >= 0 && c.cuts == 1, "%s: a native hitstop inside the cut keeps the same instance (and the intent)", h.name);
    View v;
    v.cut_normal = true, v.cut = h.type, v.combo = 2, v.safe = v.human = v.sword = v.ground = v.lock = v.r_held = true, v.r_down_tick = 10000, v.frame = h.start, v.at_start = h.at_s,
    v.at_end = h.at_e, v.tick = 10100;
    c.step(v);
    CHECK(c.cuts == 2 && !c.intent, "%s: the next combo cut is a new instance (the old R press never cancels it)", h.name);
  }
  {  // the lunge must be spent before a recovery cancel (never a slide into the guard)
    Ctl c;
    View v;
    v.cut_normal = true, v.cut = kCutComboStab, v.combo = 1, v.safe = v.human = v.sword = v.ground = v.lock = true, v.at_start = 3, v.at_end = 10;
    v.r_held = true, v.r_down_tick = 21, v.frame = 3.5f, v.tick = 20;
    c.step(v);
    v.tick = 21, v.frame = 10.2f, v.speed = 4.0f;
    const Out o0 = c.step(v);
    CHECK(o0.v == Verdict::Buffer && o0.why == Why::LastAtContact, "the first recovery call is native (it evaluates the last AT contact: an enemy's parry rebound)");
    v.tick = 22, v.frame = 10.5f, v.speed = 3.0f;
    const Out o = c.step(v);
    CHECK(o.v == Verdict::Buffer && o.why == Why::Lunge, "recovery with the lunge still moving: wait");
    v.tick = 23, v.frame = 11.6f, v.speed = 0.4f;
    CHECK(c.step(v).v == Verdict::Cancel, "lunge spent: cancel");
  }
  std::printf("LGC01_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "LAB_FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
