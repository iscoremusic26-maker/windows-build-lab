// LIGHT FADES TWICE 0.13.16 LDG01 (DKN01 FIX07 change 6): deterministic laboratory of the pure dodge i-frame core (src/ldg_core.hpp). No game, no SDK.
// The adapter's order is replayed tick by tick: Link's setCollision PRE (observe) then that tick's collision pass (verdict / HeldSet). A paused tick
// (hitstop, menu, event pause) runs neither Link's execute nor a pass with his cylinders: it is simply absent from the sequence.
// Build: g++ -std=c++20 tests/ldg_lab.cpp  (CI: MSVC).
#include "../src/ldg_core.hpp"
#include <cstdio>
#include <string_view>

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
using namespace ldg;
using namespace ldg::native;
static constexpr int kWait = 3, kAtnMove = 5, kCut = 0x21, kGuardAttack = 0x1E;

// One dodge of `dir` started at tick 100 (frame grows 1 per tick) after `prev` procedures; returns the passes it protected (bitmask over ticks 100..131).
static unsigned window_mask(int proc, int side, int prev = kAtnMove, bool combat = true, bool human = true, Atk cls = Atk::Ordinary, unsigned len = 30) {
  Tracker t;
  for (std::uint64_t k = 90; k < 100; ++k) t.observe(k, prev, 0, 0, true, combat);
  unsigned m = 0;
  for (unsigned i = 0; i < 32; ++i) {
    const std::uint64_t now = 100 + i;
    t.observe(now, i < len ? proc : kWait, side, static_cast<float>(i), human, combat);
    if (t.verdict(now, cls) == Res::Evaded) m |= 1u << i;
  }
  return m;
}
static unsigned popcount(unsigned v) {
  unsigned n = 0;
  for (; v; v &= v - 1) ++n;
  return n;
}

int main() {
  std::printf("LDG_LAB version=%s tag=%s\n", kVersion, kTag);
  CHECK(std::string_view(kVersion) == "0.13.16" && std::string_view(kTag) == "LDG01", "version / tag");
  // ---------------------------------------------------------------- 1. approved windows: exactly N passes from the real start, never N - 1, never N + 1
  CHECK(policy::kForwardTicks == 9 && policy::kSideTicks == 6 && policy::kBackTicks == 6, "approved 9 / 6 / 6 ticks (0.30 / 0.20 / 0.20 s at 30 Hz)");
  {
    const unsigned f = window_mask(kProcFrontRoll, 0), l = window_mask(kProcSideStep, kDirLeft), r = window_mask(kProcSideStep, kDirRight), b = window_mask(kProcBackJump, 0),
                   hb = window_mask(kProcSideStep, kDirBackward);
    std::printf("WINDOWS forward=%u side_left=%u side_right=%u backflip=%u heavy_back=%u (passes from the start tick)\n", popcount(f), popcount(l), popcount(r), popcount(b), popcount(hb));
    CHECK(f == (1u << 9) - 1, "forward roll: passes 0..8 exactly (mask %x)", f);
    CHECK(l == (1u << 6) - 1 && r == (1u << 6) - 1, "side jumps: passes 0..5 exactly");
    CHECK(b == (1u << 6) - 1 && hb == (1u << 6) - 1, "back jump (backflip and the heavy back jump): passes 0..5 exactly");
    CHECK(!(f & (1u << 9)) && (f & (1u << 8)) && !(l & (1u << 6)) && (l & (1u << 5)), "N - 1 protected, N not (no N + 1)");
    CHECK(window_mask(kProcSideStep, kDirForward) == 0, "a side step with no lateral / back direction is not mapped");
    CHECK(window_mask(kProcSideRoll, 0) == 0, "PROC_SIDE_ROLL (the Back Slice preparation / a landing) never mapped");
    CHECK(window_mask(0x15, 0) == 0 && window_mask(kCut, 0) == 0 && window_mask(kGuardAttack, 0) == 0 && window_mask(kWait, 0) == 0, "jumps, cuts, Shield Attack, idle: no i-frames");
  }
  // ---------------------------------------------------------------- 2. eligibility: combat context, human, never the recovery of a fall / jump
  {
    CHECK(window_mask(kProcFrontRoll, 0, kAtnMove, false) == 0, "no combat context (no enemy near, no Z-target): a travel roll has no i-frames");
    CHECK(window_mask(kProcFrontRoll, 0, kAtnMove, true, false) == 0, "wolf (not human): never");
    for (int prev : {kProcFall, kProcLand, kProcAutoJump, kProcDiveJump, kProcRollJump, kProcSlide, kProcSlideLand, kProcSlip})
      CHECK(window_mask(kProcFrontRoll, 0, prev) == 0, "a roll after proc 0x%02x is a fall / jump recovery: no i-frames", prev);
    CHECK(window_mask(kProcFrontRoll, 0, kProcSideStepLand) == (1u << 9) - 1 && window_mask(kProcSideStep, kDirLeft, kProcBackJumpLand) == (1u << 6) - 1,
          "chaining a dodge from a dodge landing: a new eligible dodge");
  }
  // ---------------------------------------------------------------- 3. one window per REAL action: held input, repeated reads, restarts, chains
  {
    Tracker t;
    t.observe(10, kAtnMove, 0, 0, true, true);
    unsigned prot = 0;
    for (unsigned i = 0; i < 40; ++i) {  // the same roll read for 40 ticks (input held / long animation): ONE window
      t.observe(11 + i, kProcFrontRoll, 0, static_cast<float>(i), true, true);
      t.observe(11 + i, kProcFrontRoll, 0, static_cast<float>(i), true, true);  // a second read in the same tick never advances or reopens
      prot += t.verdict(11 + i, Atk::Ordinary) == Res::Evaded;
    }
    CHECK(prot == 9 && t.starts[unsigned(Dir::Forward)] == 1, "held input / repeated reads: one action, 9 protected passes (%u, starts %llu)", prot,
          static_cast<unsigned long long>(t.starts[1]));
    // the same procedure restarted (its clip went back): a NEW action, a new window
    Tracker r;
    r.observe(10, kAtnMove, 0, 0, true, true);
    unsigned p1 = 0, p2 = 0;
    for (unsigned i = 0; i < 12; ++i) r.observe(11 + i, kProcSideStep, kDirLeft, static_cast<float>(i), true, true), p1 += r.verdict(11 + i, Atk::Ordinary) == Res::Evaded;
    for (unsigned i = 0; i < 12; ++i) r.observe(23 + i, kProcSideStep, kDirLeft, static_cast<float>(i), true, true), p2 += r.verdict(23 + i, Atk::Ordinary) == Res::Evaded;
    CHECK(p1 == 6 && p2 == 6 && r.restarts == 1 && r.d.serial == 2, "a real restart is a new action (6 + 6)");
    // spam: dodge -> land -> dodge: each real action its own window, never a continuous protection
    Tracker s;
    unsigned prot_total = 0, ticks = 0;
    std::uint64_t now = 0;
    for (unsigned k = 0; k < 10; ++k) {
      for (unsigned i = 0; i < 14; ++i, ++now, ++ticks) s.observe(now, kProcSideStep, kDirRight, static_cast<float>(i), true, true), prot_total += s.verdict(now, Atk::Ordinary) == Res::Evaded;
      for (unsigned i = 0; i < 8; ++i, ++now, ++ticks) s.observe(now, kProcSideStepLand, 0, static_cast<float>(i), true, true), prot_total += s.verdict(now, Atk::Ordinary) == Res::Evaded;
    }
    CHECK(prot_total == 60 && ticks == 220, "10 side jumps in a row: 10 x 6 protected passes out of 220 (%u)", prot_total);
  }
  // ---------------------------------------------------------------- 4. pause / hitstop: no executed Link tick = no pass, the window neither advances nor expires
  {
    Tracker t;
    t.observe(10, kAtnMove, 0, 0, true, true);
    unsigned prot = 0;
    std::uint64_t now = 11;
    for (unsigned i = 0; i < 9; ++i) {
      if (i == 3) now += 40;  // 40 paused ticks (menu / hitstop): Link does not execute, nothing is observed, no pass sees his cylinders
      t.observe(now, kProcBackJump, 0, static_cast<float>(i), true, true);
      prot += t.verdict(now, Atk::Ordinary) == Res::Evaded;
      ++now;
    }
    CHECK(prot == 6, "a pause inside the window keeps exactly 6 simulation passes (%u)", prot);
    CHECK(t.verdict(now + 1, Atk::Ordinary) == Res::OutsideWindow, "a pass with no observation of that tick is outside the window");
    // render frames: the tracker only exists per simulation tick; extra presentation frames never call it
  }
  // ---------------------------------------------------------------- 5. Sekiro-inspired exclusions: forward vs thrust / sweep, side / back vs sweep
  {
    CHECK(window_mask(kProcFrontRoll, 0, kAtnMove, true, true, Atk::Thrust) == 0, "forward roll: no i-frames against a thrust");
    CHECK(window_mask(kProcSideStep, kDirLeft, kAtnMove, true, true, Atk::Thrust) == (1u << 6) - 1 && window_mask(kProcBackJump, 0, kAtnMove, true, true, Atk::Thrust) == (1u << 6) - 1,
          "side / back: protected against a thrust");
    for (int p : {kProcFrontRoll, kProcBackJump}) CHECK(window_mask(p, 0, kAtnMove, true, true, Atk::Sweep) == 0, "a genuine sweep is never dodged by i-frames (proc 0x%02x)", p);
    CHECK(window_mask(kProcSideStep, kDirLeft, kAtnMove, true, true, Atk::Sweep) == 0, "side: never against a sweep");
    CHECK(excluded(Dir::Forward, Atk::Thrust) && !excluded(Dir::Side, Atk::Thrust) && !excluded(Dir::Back, Atk::Thrust) && excluded(Dir::Side, Atk::Sweep) && !excluded(Dir::Forward, Atk::Ordinary),
          "exclusion table");
    Tracker t;
    t.observe(1, kAtnMove, 0, 0, true, true);
    t.observe(2, kProcFrontRoll, 0, 0, true, true);
    CHECK(t.verdict(2, Atk::Thrust) == Res::Excluded && t.verdict(2, Atk::Ordinary) == Res::Evaded && t.verdict(3, Atk::Ordinary) == Res::OutsideWindow, "verdicts: excluded / evaded / outside");
    Tracker n;
    n.observe(1, kAtnMove, 0, 0, true, false);
    n.observe(2, kProcFrontRoll, 0, 0, true, false);
    CHECK(n.verdict(2, Atk::Ordinary) == Res::NotEligible, "a non-combat roll: NOT_ELIGIBLE, the contact stands");
    Tracker z;
    CHECK(z.verdict(5, Atk::Ordinary) == Res::Contact, "no dodge at all: the ordinary contact");
  }
  // ---------------------------------------------------------------- 6. the evaded contact held: no deferred fictitious damage, a second real impact still hits
  {
    HeldSet h;
    const std::uintptr_t blade = 0x1000, other = 0x2000;
    h.add(blade, 7, 0, 100, 1);
    CHECK(h.continues(blade, 7, 0, 101) && h.continues(blade, 7, 0, 102) && h.continues(blade, 7, 0, 103), "the same blade crossing Link pass after pass stays evaded");
    CHECK(!h.continues(blade, 7, 0, 105), "a gap of one pass: a new contact (judged again)");
    CHECK(!h.continues(other, 7, 0, 101) && !h.continues(blade, 8, 0, 101), "another collider / attacker: never covered");
    HeldSet ha;
    ha.add(blade, 7, 0, 300, 1);
    CHECK(!ha.continues(blade, 8, 0, 301) && ha.continues(blade, 7, 0, 301), "the same collider pointer reused by ANOTHER actor in the next pass is a new contact");
    HeldSet d;  // the Darknut: the impact-window serial is the exact identity
    d.add(blade, 9, 41, 200, 1);
    CHECK(d.continues(blade, 9, 41, 200) && d.continues(blade, 9, 41, 207), "the same Darknut impact window (serial 41) stays evaded, even after a gap");
    CHECK(!d.continues(blade, 9, 42, 208), "the next impact (the double's second cut, serial 42) is a new contact");
    d.prune(207 + 91);
    CHECK(!d.continues(blade, 9, 41, 299), "a held impact identity is forgotten long after its window (90 passes after its last crossing)");
    h.prune(110);
    CHECK(!h.continues(blade, 7, 0, 111), "pruned after one pass without a crossing");
    HeldSet full;
    for (unsigned i = 0; i < 12; ++i) full.add(0x100 * (i + 1), i, 0, 50, 1);
    CHECK(full.continues(0x100 * 12, 11, 0, 51) && !full.continues(0x100 * 1, 0, 0, 51), "bounded: the oldest evaded contacts make room (8 held)");
    // the window ends while the same blade still crosses: held (no damage); after a real gap and outside a window: the contact stands
    Tracker t;
    HeldSet hs;
    t.observe(0, kAtnMove, 0, 0, true, true);
    unsigned evaded = 0, held = 0, contact = 0;
    for (unsigned i = 0; i < 20; ++i) {
      const std::uint64_t now = 1 + i;
      t.observe(now, i < 12 ? kProcSideStep : kWait, kDirLeft, static_cast<float>(i), true, true);
      const bool crossing = (i >= 3 && i <= 10) || i == 15;  // the blade crosses Link at passes 3..10, then again at 15 (a new contact)
      if (!crossing) continue;
      if (hs.continues(0xABC, 5, 0, now)) {
        ++held;
        continue;
      }
      const Res r = t.verdict(now, Atk::Ordinary);
      if (r == Res::Evaded) ++evaded, hs.add(0xABC, 5, 0, now, t.d.serial);
      else ++contact;
    }
    CHECK(evaded == 1 && held == 7 && contact == 1, "one evaded contact held through the end of the window (1 + 7), the later new contact hits (%u %u %u)", evaded, held, contact);
  }
  // ---------------------------------------------------------------- 7. determinism / cleanup
  {
    Tracker a, b;
    unsigned mism = 0;
    for (unsigned i = 0; i < 5000; ++i) {
      const int proc = (i / 13) % 5 == 0 ? kProcFrontRoll : (i / 13) % 5 == 1 ? kProcSideStep : (i / 13) % 5 == 2 ? kProcBackJump : kAtnMove;
      a.observe(i, proc, kDirLeft, static_cast<float>(i % 13), true, (i / 70) % 2 == 0);
      b.observe(i, proc, kDirLeft, static_cast<float>(i % 13), true, (i / 70) % 2 == 0);
      mism += a.verdict(i, Atk::Ordinary) != b.verdict(i, Atk::Ordinary);
    }
    CHECK(mism == 0, "deterministic");
    Tracker r;
    r.observe(10, kAtnMove, 0, 0, true, true);
    r.observe(11, kProcFrontRoll, 0, 0, true, true);
    CHECK(r.verdict(11, Atk::Ordinary) == Res::Evaded, "a window is open");
    r.reset();
    CHECK(!r.d.on && r.verdict(11, Atk::Ordinary) == Res::Contact && r.protect_pass == ~0ull, "reset (scene / Link change): the open window is gone");
    r.observe(11, kProcFrontRoll, 0, 1, true, true);  // the same tick observed again after the reset is a fresh observation, never a continuation
    CHECK(r.d.on && r.d.age == 0 && r.d.serial == 2, "after a reset the next observation starts a new action (serial %llu)", static_cast<unsigned long long>(r.d.serial));
  }
  std::printf("\nLDG_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "LAB_FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
