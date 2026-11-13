// LIGHT FADES TWICE 0.13.8 DKR01: laboratory of the PURE core (src/dkr_core.hpp): identity, the per-actor state machine (one resurrection, the timer,
// the native Ending Blow receipt, the freeze while any Ending Blow is in flight, watchdogs), the slot table and the clip's structural check against the
// shipped bytes and against every corruption of each field it checks. No game type.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../src/dkr_core.hpp"
static unsigned long long g_checks = 0;
#define CHECK(c)                                            \
  do {                                                      \
    ++g_checks;                                             \
    if (!(c)) {                                             \
      std::printf("DKR_LAB_FAIL line=%d %s\n", __LINE__, #c); \
      std::fflush(stdout);                                  \
      std::exit(1);                                         \
    }                                                       \
  } while (0)
using namespace dkr;
static LinkView idle() { return {}; }
static LinkView fin(bool me) { LinkView v; v.finishing = true, v.on_me = me; return v; }
static LinkView back() { LinkView v; v.back_jump = true; return v; }
static void identity() {
  Identity i;
  i.param = kTargetParam, i.type = 1, i.type2 = 0, i.switch_no = 0xFF;
  CHECK(eligible(i));
  Identity j = i; j.param = 0x010001FFu, j.type2 = 1;  CHECK(!eligible(j));       // Darknut (Sword)
  j = i; j.type = 0;                                    CHECK(!eligible(j));       // Temple of Time
  j = i; j.switch_no = 0x12, j.param = 0x112;           CHECK(!eligible(j));       // placed with a switch
  j = i; j.stay_no2 = true;                             CHECK(!eligible(j));       // Hyrule Castle stay-no-2
  j = i; j.param = 0x000101FFu;                         CHECK(!eligible(j));       // another break frame
  j = i; j.type2 = 1;                                   CHECK(!eligible(j));
  // the decoded fields must agree with the parameters (a different decode of the same word is refused)
  j = i; j.switch_no = 0x12;                            CHECK(!eligible(j));
  j = i; j.type = 0;                                    CHECK(!eligible(j));
}
static unsigned to_down(Ctl& c) {
  CHECK(c.on_lethal(true) == Step::TakeFall && c.phase == Phase::Falling);
  unsigned n = 0;
  for (; n < 78; ++n) CHECK(c.after_fall(kModeFall) == Step::FallFixups);
  CHECK(c.after_fall(kModeVanish) == Step::EnterDown && c.phase == Phase::Down && c.timer == kReviveDelayTicks);
  return n;
}
static void timer_path() {
  Ctl c;
  to_down(c);
  for (unsigned k = 1; k < kReviveDelayTicks; ++k) CHECK(c.held(idle(), false) == Step::HoldDown);
  CHECK(c.held(idle(), false) == Step::Rise && c.phase == Phase::Rising && c.path == Path::Timer && c.revivals == 1 && c.consumed());
  for (int k = 0; k < 87; ++k) CHECK(c.rising(true, false) == Step::RiseHold);
  CHECK(c.rising(true, true) == Step::BackToCombat && c.phase == Phase::Consumed);
  CHECK(c.on_lethal(true) == Step::NativeDeath && c.phase == Phase::Consumed);  // the second defeat: native, never a second fall of ours
  CHECK(c.on_lethal(true) == Step::NativeDeath && c.revivals == 1);
  CHECK(c.held(idle(), true) == Step::None && c.rising(true, true) == Step::None && c.after_fall(kModeVanish) == Step::None);
}
static void finish_path() {
  Ctl c;
  to_down(c);
  for (int k = 0; k < 10; ++k) c.held(idle(), false);
  const unsigned t = c.timer;
  CHECK(c.held(fin(true), false) == Step::HoldDown && c.phase == Phase::Committed && c.commits == 1 && c.timer == t);
  for (int k = 0; k < 17; ++k) CHECK(c.held(fin(true), false) == Step::HoldDown && c.timer == t);
  CHECK(c.held(fin(true), true) == Step::HoldDown && c.phase == Phase::FinishHit && c.hit);  // the receipt, Link still stabbing
  for (int k = 0; k < 51; ++k) CHECK(c.held(fin(true), false) == Step::HoldDown);
  for (int k = 0; k < 22; ++k) CHECK(c.held(back(), false) == Step::HoldDown);           // the back flip and its landing are part of it
  CHECK(c.held(idle(), false) == Step::Rise && c.path == Path::Finish && c.revivals == 1 && !c.watchdog);
  CHECK(c.rising(false, false) == Step::BackToCombat && c.phase == Phase::Consumed);       // a clip replaced by someone else ends the rise safely
}
static void freeze_and_misses() {
  // any Ending Blow in flight freezes the timer, even at its last tick; only the one on this actor commits
  Ctl c;
  to_down(c);
  while (c.timer > 1) c.held(idle(), false);
  CHECK(c.held(fin(false), false) == Step::HoldDown && c.phase == Phase::Down && c.timer == 1 && c.commits == 0);
  CHECK(c.held(fin(true), false) == Step::HoldDown && c.phase == Phase::Committed && c.timer == 1);
  CHECK(c.held(idle(), false) == Step::Rise && c.misses == 1 && c.path == Path::Timer);  // missed: the timer resumes and expires now
  // a receipt seen without the commit tick (the stab landed between two of our ticks) still counts
  Ctl d;
  to_down(d);
  CHECK(d.held(fin(true), true) == Step::HoldDown && d.phase == Phase::FinishHit && d.commits == 0);
  CHECK(d.held(idle(), false) == Step::Rise && d.path == Path::Finish);
  // watchdogs: a commit never resolving falls back to the timer rules (still frozen while finishing); a receipt whose Link never ends rises at 20 s
  Ctl e;
  to_down(e);
  e.held(fin(true), false);
  for (unsigned k = 1; k < kFinishWatchdogTicks; ++k) CHECK(e.held(fin(true), false) == Step::HoldDown && e.phase == Phase::Committed);
  CHECK(e.held(fin(true), false) == Step::HoldDown && e.phase == Phase::Down && e.watchdog && e.misses == 1);
  CHECK(e.held(fin(true), false) == Step::HoldDown && e.phase == Phase::Down);  // no re-commit after the watchdog, and no rise mid-jump
  Ctl f;
  to_down(f);
  CHECK(f.held(fin(true), true) == Step::HoldDown && f.hit_wait_ticks == 1);  // the receipt tick already counts as one tick of waiting
  for (unsigned k = 2; k < kFinishWatchdogTicks; ++k) CHECK(f.held(fin(true), false) == Step::HoldDown);
  CHECK(f.held(fin(true), false) == Step::Rise && f.watchdog && f.path == Path::Finish);
}
static void guards() {
  Ctl c;
  CHECK(c.on_lethal(false) == Step::NativeDeath && c.phase == Phase::Native && c.revivals == 0);  // no clip: native, permanently
  CHECK(c.on_lethal(true) == Step::NativeDeath);
  Ctl d;
  to_down(d);
  CHECK(d.on_lethal(true) == Step::None && d.phase == Phase::Down);  // idempotent while held
  CHECK(d.rising(true, true) == Step::None && d.after_fall(kModeVanish) == Step::None);
  Ctl e;
  CHECK(e.held(idle(), false) == Step::None && e.rising(true, true) == Step::None && e.after_fall(kModeVanish) == Step::None);
  CHECK(kMaxRevivals == 1 && kReviveDelayTicks == 300 && kHeldMode > 3 && kHeldMode != kModeVanish);
}
static void table() {
  Table t;
  for (unsigned i = 0; i < kMaxSlots; ++i) CHECK(t.take(1000 + i) != nullptr);
  CHECK(t.take(5000) == nullptr && t.full == 1 && t.live() == kMaxSlots);
  CHECK(t.take(1003) == t.find(1003));  // same id, same slot
  Ctl* c = t.find(1003);
  c->revivals = 1;
  CHECK(t.retire(1003) && !t.find(1003) && t.live() == kMaxSlots - 1 && !t.retire(1003));
  Ctl* n = t.take(7000);
  CHECK(n && n == c && n->revivals == 0 && n->phase == Phase::Armed && n->id == 7000 && n->generation == kMaxSlots + 1);  // the freed slot, a fresh actor
  CHECK(t.find(1004) && t.find(1004)->id == 1004);
}
static std::vector<unsigned char> read_file(const char* p) {
  std::vector<unsigned char> v;
  if (FILE* f = std::fopen(p, "rb")) {
    unsigned char b[4096];
    size_t n;
    while ((n = std::fread(b, 1, sizeof(b), f)) > 0) v.insert(v.end(), b, b + n);
    std::fclose(f);
  }
  return v;
}
static void clip() {
  std::vector<unsigned char> f = read_file(DKR_BCK_FILE);
  CHECK(f.size() == kBckBytes && check_bck(f.data(), f.size()) == Bck::Ok && crc32(f.data(), f.size()) == kBckCrc32Value);
  CHECK(check_bck(nullptr, 0) == Bck::Size && check_bck(f.data(), f.size() - 1) == Bck::Size);
  // every byte flip is caught (CRC), and each structural rule catches its own field even with a matching CRC (rules checked on a re-CRC'd copy)
  for (size_t at : {size_t(0), size_t(0x29), size_t(0x40), size_t(0x660), size_t(0x5000), size_t(f.size() - 1)}) {
    std::vector<unsigned char> g = f;
    g[at] ^= 0x10;
    CHECK(check_bck(g.data(), g.size()) == Bck::Crc);
  }
  struct Mut { size_t at; unsigned char v; Bck want; };
  const Mut muts[] = {{0x00, 'X', Bck::Magic}, {0x07, '2', Bck::Magic}, {0x0F, 2, Bck::Blocks}, {0x20, 'B', Bck::Tag}, {0x28, 2, Bck::Loop}, {0x2B, 89, Bck::Frames},
                      {0x2D, 30, Bck::Joints}, {0x37, 0xFF, Bck::Tables}};
  for (const Mut& m : muts) {
    std::vector<unsigned char> g = f;
    g[m.at] = m.v;
    CHECK(check_bck(g.data(), g.size()) == Bck::Crc);           // production order: size, CRC, structure
    CHECK(check_bck_layout(g.data(), g.size()) == m.want);       // and each structural rule catches its own field
  }
  CHECK(check_bck_layout(f.data(), f.size()) == Bck::Ok);
  {  // a key table pointing past its pool
    std::vector<unsigned char> g = f;
    const size_t tbl = 0x20 + 0x40 + 0 * 54 + 1 * 6;  // joint 0, X, rotation: keys, offset, type
    g[tbl + 2] = 0xFF, g[tbl + 3] = 0xFF;
    CHECK(check_bck_layout(g.data(), g.size()) == Bck::Tables);
    g = f;
    g[tbl + 5] = 2;  // tangent type 2 does not exist
    CHECK(check_bck_layout(g.data(), g.size()) == Bck::Tables);
    g = f;  // a multi-key rotation table whose offset is inside the pool but whose keys run past its end
    {
      const unsigned char* b = g.data() + 0x20;
      const unsigned jt = be32(b + 0x14), rc = be16(b + 0x10);
      size_t at = 0;
      for (unsigned j = 0; j < kBckJoints && !at; ++j)
        for (unsigned c = 0; c < 3 && !at; ++c) {
          const size_t o = 0x20 + jt + j * 54 + c * 18 + 6;
          if (be16(g.data() + o) > 1) at = o;
        }
      CHECK(at != 0);
      g[at + 2] = static_cast<unsigned char>((rc - 2) >> 8), g[at + 3] = static_cast<unsigned char>((rc - 2) & 0xFF);
      CHECK(check_bck_layout(g.data(), g.size()) == Bck::Tables);
    }
    g = f;
    g[0x20 + 0x18 + 3] = 0x61;  // scale pool offset misaligned
    CHECK(check_bck_layout(g.data(), g.size()) == Bck::Tables);
  }
  CHECK(be32(f.data()) == 0x4A334431u && be16(f.data() + 0x2A) == kBckFrames && be16(f.data() + 0x2C) == kBckJoints && f[0x28] == 0);
}
int main() {
  identity();
  timer_path();
  finish_path();
  freeze_and_misses();
  guards();
  table();
  clip();
  std::printf("DKR_LAB checks=%llu RESULT=LAB_PASS failures=0\n", g_checks);
  return 0;
}
