// LIGHT FADES TWICE FIX03 0.12.3 laboratory of the two PRODUCTION pure cores added by FIX03: lockon_core.hpp (the Sekiro-style lock-on option) and
// parry_fx_core.hpp (hue of the native spark instance, decision bookkeeping). Scripted input frames against the core exactly as lockon_update and
// the parry FX adapter call it. Declared limits: no game, no attention object, no camera, no particle manager, no audio: a laboratory, not a run.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "../src/lockon_core.hpp"
#include "../src/parry_fx_core.hpp"
#include "../src/parry_core.hpp"
#include "../src/shield_chain_core.hpp"

static unsigned long long g_checks = 0;
#define CHECK(x)                                                  \
  do {                                                            \
    ++g_checks;                                                   \
    if (!(x)) {                                                   \
      std::printf("LIZ_LAB_FAIL line=%d %s\n", __LINE__, #x);     \
      std::exit(1);                                               \
    }                                                             \
  } while (0)

using namespace lockon;
struct Frame {
  float trig = 0, cx = 0;
  bool locked = false, enemy = false;
};
// One simulated attention: HOLD option LOCK while the (possibly synthetic) L is held, RELEASE for 10 ticks, NONE; SWITCH option: a press locks and stays.
struct Att {
  Mode mode = Mode::Hold;
  bool target = true, enemy = true;
  int status = 0, rel = 0;  // 0 NONE 1 LOCK 2 RELEASE
  bool held_prev = false;
  void run(bool hold, bool forced_release) {
    if (forced_release) status = 0;
    const bool edge = hold && !held_prev;
    held_prev = hold;
    if (!target) {
      status = 0;
      return;
    }
    if (mode == Mode::Hold) {
      if (status == 0 && hold) status = 1;
      else if (status == 1 && !hold) status = 2, rel = 10;
      else if (status == 2) {
        if (edge) status = 1;
        else if (--rel <= 0) status = 0;
      }
    } else {
      if (status == 0 && edge) status = 1;
      else if (status == 1 && edge) status = 1;  // native: cycle (nextAttention), stays locked
    }
  }
};
struct Rig {
  Toggle t;
  Att a;
  In base;
  int synth_hold_frames = 0, muted = 0, flicks = 0;
  Out step(float trig, float cx = 0, bool enabled = true, bool safe = true) {
    In in = base;
    in.enabled = enabled, in.safe = safe, in.trigger = trig, in.cx = cx, in.mode = a.mode;
    in.locked = a.status == 1, in.enemy = a.enemy;
    const Out o = t.step(in);
    // LRlockCheck's own value would be the real hold; the attention sees the synthetic one when written
    const bool real = trig > kOn ? true : trig < kOff ? false : a.held_prev;
    const bool hold = o.write ? o.hold : real;
    if (o.write && o.hold) ++synth_hold_frames;
    if (o.mute_stick) ++muted;
    if (o.switch_dir) ++flicks;
    a.run(hold, o.force_release);
    return o;
  }
};
static void lab_hold_toggle() {
  // press, release: the lock stays; press again: released, and that press never relocks while held; release; press: a fresh native lock.
  Rig r;
  r.step(1.0f);
  CHECK(r.a.status == 1 && r.t.st == St::Idle);  // the attention locks on the press frame; the core sees it next frame
  r.step(1.0f);
  CHECK(r.t.st == St::Held);
  r.step(0.0f);
  CHECK(r.t.st == St::Toggled && r.a.status == 1);
  for (int k = 0; k < 120; ++k) r.step(0.0f);
  CHECK(r.a.status == 1 && r.t.st == St::Toggled);  // released 4 s ago: still locked
  r.step(1.0f);  // second press
  CHECK(r.t.st == St::Release && r.t.c.cancelled == 1);
  for (int k = 0; k < 30; ++k) r.step(1.0f);  // held: hidden, the attention releases and never relocks
  CHECK(r.a.status == 0);
  r.step(0.0f);
  CHECK(r.t.st == St::Idle);
  r.step(0.0f);
  CHECK(r.a.status == 0);
  r.step(1.0f);  // a new press: native lock again
  r.step(1.0f);
  CHECK(r.a.status == 1 && r.t.st == St::Held);
  // A one-frame tap: locked on the press frame, kept after the release.
  Rig q;
  q.step(1.0f);
  q.step(0.0f);
  CHECK(q.t.st == St::Toggled && q.a.status == 1);
  for (int k = 0; k < 60; ++k) q.step(0.0f);
  CHECK(q.a.status == 1);
  // A lock that appears long after any press (target walked into the attention while L was held earlier) is not taken over by a stale tap.
  Rig z;
  z.a.target = false;
  z.step(1.0f);
  z.step(0.0f);
  for (int k = 0; k < 20; ++k) z.step(0.0f);
  z.a.target = true, z.a.status = 1;
  z.step(0.0f);
  CHECK(z.t.st == St::Idle);
}
static void lab_switch_option() {
  Rig r;
  r.a.mode = Mode::Switch;
  r.step(1.0f);
  r.step(0.0f);
  CHECK(r.a.status == 1 && r.t.st == St::Toggled);
  int writes_hold = 0;
  for (int k = 0; k < 60; ++k) writes_hold += r.step(0.0f).hold ? 1 : 0;
  CHECK(writes_hold == 0 && r.a.status == 1);  // SWITCH: never a synthetic hold (15 held ticks would release natively)
  const Out o = r.step(1.0f);
  CHECK(o.force_release && r.a.status == 0 && r.t.c.forced_releases == 1);  // native would cycle: the press releases instead
  for (int k = 0; k < 10; ++k) CHECK(!r.step(1.0f).hold);
  r.step(0.0f);
  CHECK(r.t.st == St::Idle);
}
static void lab_hysteresis_and_band() {
  // The analog L between 0.6 and 0.9 keeps the previous state (LRlockCheck); a press is the crossing of 0.9.
  Rig r;
  r.step(0.95f);
  r.step(0.0f);
  CHECK(r.t.st == St::Toggled);
  r.step(0.75f);  // band from released: not a press
  CHECK(r.t.st == St::Toggled);
  r.step(0.91f);
  CHECK(r.t.st == St::Release);
  r.step(0.7f);  // band from pressed: still pressed, still hidden
  CHECK(r.t.st == St::Release);
  r.step(0.5f);
  CHECK(r.t.st == St::Idle);
}
static void lab_stick() {
  Rig r;
  r.step(1.0f);
  r.step(0.0f);
  int dirs = 0;
  for (int k = 0; k < 90; ++k) dirs += r.step(0.0f, 1.0f).switch_dir;  // held right for 3 s: ONE switch
  CHECK(dirs == 1 && r.t.c.flicks == 1);
  for (int k = 0; k < 5; ++k) CHECK(r.step(0.0f, 0.5f).switch_dir == 0);  // not back near the centre: no re-arm
  CHECK(r.step(0.0f, 0.2f).switch_dir == 0);                                // centre: re-armed
  CHECK(r.step(0.0f, -0.69f).switch_dir == 0);                              // below the engage threshold
  CHECK(r.step(0.0f, -0.71f).switch_dir == -1);
  CHECK(std::uint64_t(r.muted) == r.t.c.muted_frames && r.muted > 90);
  // Out of a lock the stick is never read nor muted.
  Rig f;
  for (int k = 0; k < 30; ++k) {
    const Out o = f.step(0.0f, 1.0f);
    CHECK(!o.mute_stick && o.switch_dir == 0 && !o.write);
  }
}
static void lab_native_paths() {
  // Option OFF: nothing written at any time, whatever the input.
  {
    Rig r;
    for (int k = 0; k < 200; ++k) {
      const Out o = r.step((k / 7) % 2 ? 1.0f : 0.0f, (k % 3) - 1.0f, false);
      CHECK(!o.write && !o.mute_stick && o.switch_dir == 0 && !o.force_release);
    }
  }
  // Turned OFF while toggled: native from that very frame (the attention then sees the real, released L and releases).
  {
    Rig r;
    r.step(1.0f);
    r.step(0.0f);
    CHECK(r.t.st == St::Toggled);
    const Out o = r.step(0.0f, 0.0f, false);
    CHECK(!o.write && r.t.st == St::Idle && r.t.c.option_off == 1);
    for (int k = 0; k < 12; ++k) r.step(0.0f, 0.0f, false);
    CHECK(r.a.status == 0);
  }
  // Event / menu / wolf / horse: same, counted apart.
  {
    Rig r;
    r.step(1.0f);
    r.step(0.0f);
    const Out o = r.step(0.0f, 0.0f, true, false);
    CHECK(!o.write && r.t.c.unsafe == 1 && r.t.st == St::Idle);
  }
  // An NPC / object lock is never made Sekiro-style.
  {
    Rig r;
    r.a.enemy = false;
    r.step(1.0f);
    r.step(0.0f);
    CHECK(r.t.st == St::Idle && r.a.status != 1);
  }
  // No eligible target: a press creates no lock and nothing is held for it.
  {
    Rig r;
    r.a.target = false;
    for (int k = 0; k < 10; ++k) CHECK(!r.step(k < 5 ? 1.0f : 0.0f).write);
    CHECK(r.a.status == 0 && r.t.st == St::Idle && r.synth_hold_frames == 0);
  }
  // The target dies / leaves: the lock goes with it, no retained target, no synthetic hold.
  {
    Rig r;
    r.step(1.0f);
    r.step(0.0f);
    r.a.target = false;
    r.step(0.0f);
    const Out o = r.step(0.0f);
    CHECK(r.t.c.lost == 1 && !o.write && r.t.st == St::Idle);
  }
}
static void lab_choose() {
  Cand c[8];
  // current in the middle at x=0; enemies at -0.5, -0.2, 0.3, 0.9; an NPC at 0.1 (invalid); a point behind the camera (invalid)
  c[0] = {true, false, -0.5f}, c[1] = {true, false, -0.2f}, c[2] = {true, true, 0.0f}, c[3] = {true, false, 0.3f}, c[4] = {true, false, 0.9f}, c[5] = {false, false, 0.1f}, c[6] = {false, false, 0.05f};
  CHECK(choose(c, 7, +1) == 3);
  CHECK(choose(c, 7, -1) == 1);
  c[3].valid = false;
  CHECK(choose(c, 7, +1) == 4);
  c[4].valid = false;
  CHECK(choose(c, 7, +1) == -1);  // nothing on that side: the current lock stays
  c[1].x = -0.01f;                // practically on the current target (overlapping on screen): not "on the left"
  CHECK(choose(c, 7, -1) == 0);
  Cand one[1] = {{true, true, 0.0f}};
  CHECK(choose(one, 1, +1) == -1 && choose(one, 1, -1) == -1);
  CHECK(choose(c, 0, +1) == -1);
}
static void lab_orange() {
  using pfx::Rgb;
  const auto eq = [](Rgb a, Rgb b) { return a.r == b.r && a.g == b.g && a.b == b.b; };
  // grey and white never change; any colour keeps V (max) and min; hue lands at 28 deg; idempotent.
  for (unsigned v = 0; v < 256; v += 5) CHECK(eq(pfx::orange(Rgb{std::uint8_t(v), std::uint8_t(v), std::uint8_t(v)}), Rgb{std::uint8_t(v), std::uint8_t(v), std::uint8_t(v)}));
  for (unsigned r = 0; r < 256; r += 17)
    for (unsigned g = 0; g < 256; g += 17)
      for (unsigned b = 0; b < 256; b += 17) {
        const Rgb c{std::uint8_t(r), std::uint8_t(g), std::uint8_t(b)};
        const Rgb o = pfx::orange(c);
        const unsigned mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
        // FIX04: a real colour (saturation >= 10%) is given at least 75%, so a pale blue never turns into a pale cream; grey keeps its min.
        const unsigned cap = mx * (100u - pfx::kSatFloorPct) / 100u;
        const unsigned want_min = (mx != 0 && (mx - mn) * 100u >= mx * pfx::kChromaPct && mn > cap) ? cap : mn;
        CHECK(o.r == mx && o.b == want_min && o.g >= want_min && o.g <= mx);
        CHECK(eq(pfx::orange(o), o));
        if (mx - mn >= 40) {
          // hue = 60 * (g - b) / (r - b) for sector 0 (R max, B min)
          const float h = 60.0f * float(o.g - o.b) / float(o.r - o.b);  // o.b is the (possibly lowered) minimum
          CHECK(h > 26.5f && h < 30.0f);
        }
      }
  // the blue of a spark becomes a warm orange, neither red nor yellow
  const Rgb b1 = pfx::orange(Rgb{0, 64, 255});
  CHECK(b1.r == 255 && b1.b == 0 && b1.g >= 110 && b1.g <= 130);
  const Rgb b2 = pfx::orange(Rgb{128, 200, 255});
  CHECK(b2.r == 255 && b2.b == 63 && b2.g > 145 && b2.g < 160);
  const Rgb b3 = pfx::orange(Rgb{200, 220, 255});  // a very pale blue: still unmistakably orange
  CHECK(b3.r == 255 && b3.b <= 63 && b3.g > 145 && b3.g < 160);
  CHECK(eq(pfx::orange(Rgb{250, 250, 255}), Rgb{255, 252, 250}) || pfx::orange(Rgb{250, 250, 255}).b == 250);  // under 10% saturation: treated as white, not forced
}
static void lab_book() {
  pfx::Book k;
  const int a = k.open(pfx::Kind::LinkOnLizalfos, 7, 100);
  CHECK(a >= 0 && k.open(pfx::Kind::LinkOnLizalfos, 7, 100) == a && k.c.duplicate == 1);  // two callbacks of one contact: one record
  const int b = k.open(pfx::Kind::LinkOnLizalfos, 8, 100);
  CHECK(b >= 0 && b != a);                                       // two enemies on the same pass: two records
  const int c = k.open(pfx::Kind::LinkOnLizalfos, 7, 101);
  CHECK(c >= 0 && c != a);                                       // the same enemy one tick later: another contact
  CHECK(k.open(pfx::Kind::None, 7, 100) == -1 && k.open(pfx::Kind::LinkOnEoc, 0, 100) == -1);
  CHECK(k.orange_now(a));
  k.settle(a, true);
  CHECK(k.r[a].st == pfx::St::Confirmed && k.orange_now(a) && k.c.confirmed[1] == 1);
  k.settle(a, false);  // a confirmed record is never reverted by a later call
  CHECK(k.r[a].st == pfx::St::Confirmed);
  k.settle(b, false);
  CHECK(k.r[b].st == pfx::St::Reverted && !k.orange_now(b) && k.c.reverted[1] == 1);
  // an unanswered preview reverts after kConfirmTicks; records free themselves after kKeepTicks without callbacks
  int expired = 0;
  k.tick(101 + pfx::kConfirmTicks + 1, [&](int) { ++expired; });
  CHECK(k.r[c].st == pfx::St::Reverted && k.c.expired == 1 && expired == 0);
  k.tick(100 + pfx::kKeepTicks + 1, [&](int) { ++expired; });
  CHECK(expired == 2 && k.r[a].st == pfx::St::Free && k.r[b].st == pfx::St::Free);
  k.tick(101 + pfx::kKeepTicks + 1, [&](int) { ++expired; });
  CHECK(expired == 3);
  pfx::Book f;
  for (unsigned i = 0; i < pfx::kRecords; ++i) CHECK(f.open(pfx::Kind::LizalfosOnLink, 1 + i, 5) >= 0);
  CHECK(f.open(pfx::Kind::LizalfosOnLink, 99, 5) == -1 && f.c.full == 1);
  CHECK(pfx::kPitch > 1.10f - 1e-6f && pfx::kPitch < 1.15f + 1e-6f);
}
// The collision-pass decision reads Link through LinkTrack::peek and Chain::peek_permits: for EVERY state reached by random sequences (Shield Attack
// starts, native restarts inside an open Shield Attack, Link changes, untrusted inits, confirmed and consumed episodes, unsafe ticks) they must equal
// what observe()/pass_for() and observe()/permits() produce on the next tick.
static void lab_peeks() {
  std::uint32_t x = 12345;
  const auto rnd = [&x](unsigned n) { x = x * 1664525u + 1013904223u; return (x >> 8) % n; };
  unsigned restarts_open = 0, consumed_seen = 0, link_changes = 0, permits_true = 0;
  for (int run = 0; run < 200; ++run) {
    post08::LinkTrack t;
    shieldchain::Chain c;
    c.context(1, 1, 1);
    bool ga = false;
    float frame = 0;
    std::uint32_t id = 7;
    for (std::uint64_t tick = 10; tick < 400; ++tick) {
      // Link's execute of tick-1 (what the pass and the next update see)
      const unsigned r = rnd(100);
      const bool r_held = rnd(10) != 0, press = rnd(6) == 0, safe = rnd(20) != 0;
      c.sample(tick - 1, r_held, press, safe);
      if (r < 12) {  // a native Shield Attack init (trusted or not), possibly while one is open
        if (ga) ++restarts_open;
        ga = true, frame = 1.0f;
        if (c.native_start(tick - 1, frame, rnd(8) != 0, safe)) t.native_start();
      } else if (ga) {
        frame += 1.0f;
        if (frame > 20.0f || r > 95) ga = false;
      }
      if (rnd(50) == 0) id = id == 7 ? 8 : 7, ++link_changes;
      const bool valid = rnd(40) != 0;
      const bool clip = ga && rnd(12) != 0;
      const float rate = rnd(30) ? 1.0f : 0.5f;
      const post08::LinkView pk = t.peek(tick, valid, id, ga ? 0x1e : 3, ga, frame, 1.0f, 20.0f);
      const bool pp = c.peek_permits(tick, clip, clip ? frame : 0.0f, clip ? rate : 0.0f, safe);
      (void)t.observe(tick, valid, id, ga ? 0x1e : 3, ga, frame, 1.0f, 20.0f);
      c.observe(tick, clip, clip ? frame : 0.0f, clip ? rate : 0.0f, safe);
      const post08::LinkView ob = t.pass_for(tick, valid, id);
      CHECK(pk.valid == ob.valid && pk.guard_attack == ob.guard_attack && pk.serial == ob.serial && pk.age_ticks == ob.age_ticks && pk.frame == ob.frame && pk.proc == ob.proc);
      if (c.episodes[c.pass_serial % 4].consumed) ++consumed_seen;
      const bool pm = c.permits(tick);
      CHECK(pp == pm);
      if (pm) ++permits_true;
      if (pm && rnd(2) == 0) (void)c.confirm(tick);  // a parry consumes this clip's episode
    }
  }
  CHECK(restarts_open > 100 && consumed_seen > 100 && link_changes > 100 && permits_true > 100);
}
int main() {
  lab_peeks();
  lab_hold_toggle();
  lab_switch_option();
  lab_hysteresis_and_band();
  lab_stick();
  lab_native_paths();
  lab_choose();
  lab_orange();
  lab_book();
  std::printf("LFT03_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_LOCKON_AND_PARRY_FX_CORES runtime=HUMAN_RUNTIME_PENDING\n", lockon::kVersion, g_checks);
  return 0;
}
