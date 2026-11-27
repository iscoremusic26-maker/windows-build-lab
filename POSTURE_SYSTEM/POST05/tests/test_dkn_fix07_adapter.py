"""DKN01 FIX07: execute PRODUCTION code of src/dkn_adapter.inc against doubles: dkn_dir_to + dkn_geo (the trajectory of a native leap read with the game's
static queries: fopAcM_gc_c::gndCheck / mGroundY, fopAcM_lc_c::lineCheck, fopAcM_wt_c::waterCheck / mWaterY, over a synthetic terrain with a pit, a ledge, a
step, a wall and water), dkn_mace_post + dkn_mace_contact (the thrown mace: anchor tracking, ONE resolution per throw, +50 / +60 / 0 for Link, never the
Darknut), dkn_leap_cancel + dkn_leap_pre (air -> landing -> the combo or a legitimate cancel, never an attack in the void), dkn_ldg_query (the thrust class and
the impact identity LDG01 reads). Eight mutants must be detected; a compile error is never a detection. Doubles, not the game."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def body(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += text[end] == '{'
        depth -= text[end] == '}'
        if depth == 0:
            return text[start:end + 1]
    raise ValueError(signature)


PREFIX = r'''
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include "dkn_core.hpp"
#include "parry_fx_core.hpp"
typedef std::uint16_t u16;
struct cXyz { float x = 0, y = 0, z = 0; cXyz() = default; cXyz(float a, float b, float c) : x(a), y(b), z(c) {} };
namespace kk { inline float fsqrt(float v) { return std::sqrt(v); } }
struct Pos { cXyz pos; };
struct fopAc_ac_c { std::uint32_t id = 0; int name = 0; Pos current; std::int16_t shape_angle_y = 0; };
enum { fpcNm_B_TN_e = 531, fpcNm_OTHER_e = 7 };
std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) { return a->id; }
int fopAcM_GetName(const fopAc_ac_c* a) { return a->name; }
// ---- terrain double: flat floor y = 0; a pit 600..700 (no ground); a sunken floor -200 over 900..1100; a step +80 for z > 500; a wall at x = 1200;
// water over x in 1300..1500 (surface y = 30)
float ground_at(float x, float z, bool& ok) {
  ok = true;
  if (x >= 600 && x <= 700) return ok = false, -1e30f;
  float y = x > 900 && x < 1100 ? -200.0f : 0.0f;
  if (z > 500) y += 80.0f;
  return y;
}
struct fopAcM_gc_c { static float mGroundY; static unsigned calls; static bool gndCheck(const cXyz* p) { ++calls; bool ok; mGroundY = ground_at(p->x, p->z, ok); return ok; } };
float fopAcM_gc_c::mGroundY = 0;
unsigned fopAcM_gc_c::calls = 0;
struct fopAcM_lc_c { static unsigned calls; static bool lineCheck(const cXyz* a, const cXyz* b, const fopAc_ac_c*) { ++calls; return (a->x - 1200) * (b->x - 1200) < 0; } };
unsigned fopAcM_lc_c::calls = 0;
struct fopAcM_wt_c { static float mWaterY; static bool waterCheck(const cXyz* p) { if (p->x > 1300 && p->x < 1500) return mWaterY = 30, true; return false; } };
float fopAcM_wt_c::mWaterY = 0;
// ---- colliders
struct cCcD_Obj;
struct cCcD_ObjHitInf { bool at_hit = false; const cCcD_Obj* hit_obj = nullptr; bool ChkAtHit() const { return at_hit; } const cCcD_Obj* GetAtHitObj() const { return hit_obj; } };
struct cCcD_Obj : cCcD_ObjHitInf {};
struct cCcD_GObjInf : cCcD_Obj {};
struct dCcD_GObjInf : cCcD_GObjInf { bool shield = false, at_set = false; int spl = 1; bool ChkAtShieldHit() const { return shield; } bool ChkAtSet() const { return at_set; } int GetAtSpl() const { return spl; } };
struct FrameCtrl { float f = 0; float getFrame() const { return f; } };
struct daAlink_c : fopAc_ac_c { dCcD_GObjInf mTgCyls[3]; };
struct daB_TN_c : fopAc_ac_c { int mActionMode1 = 9, mActionMode2 = 1; std::uint16_t mStates[16] = {}; dCcD_GObjInf mSwordSphs[4], mCps, mSphC; };
daAlink_c g_link;
bool g_alive = true, g_safe = true, g_broken = false;
daAlink_c* dknb_link() { return &g_link; }
bool dknb_link_alive() { return g_alive; }
bool dknb_world_safe() { return g_safe; }
bool dknb_link_broken() { return g_broken; }
struct DknPass { bool valid = false, guard_attack = false, in_window = false; std::uint64_t serial = 0; float delta = 0; };
struct DknGain { bool confirmed = false, saturated = false; int before = 0, after = 0; };
bool g_window = false;
DknPass dknb_link_pass(daAlink_c*) { DknPass p; p.valid = true, p.in_window = g_window, p.serial = 77; return p; }
int g_link_post = 0, g_gains = 0, g_parry_conf = 0, g_blocked = 0, g_offers = 0, g_settle_true = 0, g_settle_false = 0, g_log_bad = 0;
std::uint64_t g_offer_token = 0;
DknGain dknb_link_gain(int pts, bool, std::uint32_t, std::uint64_t, std::uint64_t) { DknGain r; r.confirmed = true, r.before = g_link_post * 1000; g_link_post = g_link_post + pts > 100 ? 100 : g_link_post + pts; r.after = g_link_post * 1000; ++g_gains; return r; }
void dknb_link_parry_confirmed(std::uint32_t, std::uint64_t) { ++g_parry_conf; }
void dknb_link_blocked(std::uint32_t, bool) { ++g_blocked; }
void dknb_direct_offer(unsigned, std::uint32_t, std::uint64_t, std::uint64_t token, const void*) { ++g_offers, g_offer_token = token; }
void pfx_settle(pfx::Kind, std::uint32_t, bool parry) { ++(parry ? g_settle_true : g_settle_false); }
std::uint64_t g_tick = 1000;
std::uint64_t dknb_tick() { return g_tick; }
'''

SLOT = r'''
struct DknSlot {
  bool on = true;
  std::uint32_t id = 30;
  std::uint64_t generation = 1;
  dkn::Ph ph = dkn::Ph::Light;
  dkn::Initiative ini;
  dkn::Ledger led;
  DknLease lease = DknLease::None;
  bool tail = false;
  unsigned seg = 0;
  dkn::Plan plan;
  dkn::Lp leap_phase = dkn::Lp::None;
  dkn::Leap leap = dkn::Leap::None;
  int leap_combo = -1;
  std::uint64_t leap_tick = 0, leap_land_tick = 0, leap_combo_tick = 0;
  unsigned leap_ground = 0;
  float leap_d0 = 0, leap_x0 = 0, leap_z0 = 0, leap_land_d = 0;
  bool leap_first_impact_logged = false;
  dkn::MaceLedger mace;
  bool mace_anchor = false;
  std::uint64_t mace_direct_token = 0;
  std::uint64_t st_leap_land = 0, st_leap_combo = 0, st_leap_cancel = 0, st_mace_contacts = 0, st_mace_repeat = 0;
};
DknSlot g_slot;
struct Dn { bool enabled = true; DknSlot* s0 = &g_slot; } dn;
DknSlot* dkn_find(std::uint32_t id) { return g_slot.on && g_slot.id == id ? &g_slot : nullptr; }
unsigned g_fx_routes = 0, g_fx_route_last = 99;
void dkn_fx_route(DknSlot&, const daB_TN_c*, unsigned route, std::uint64_t, const char*) { ++g_fx_routes, g_fx_route_last = route; }  // FIX08-06 ledger (double)
unsigned dkn_index(const DknSlot*) { return 0; }
bool dkn_combat(const DknSlot& s) { return s.ph == dkn::Ph::Armor || dkn::phase2(s.ph); }
unsigned g_logs = 0;
char g_last_log[64] = {};
void dkn_log(const char* ev, const DknSlot&, const daB_TN_c*, const char* = "", bool = false) { ++g_logs; std::snprintf(g_last_log, sizeof(g_last_log), "%s", ev); }
float dkn_dist(const daB_TN_c* a, const daAlink_c* k) { const float dx = k->current.pos.x - a->current.pos.x, dz = k->current.pos.z - a->current.pos.z; return std::sqrt(dx * dx + dz * dz); }
bool g_front = true;
bool dkn_front(const daB_TN_c*, const daAlink_c*) { return g_front; }
int g_begin = 0, g_begin_ok = 1;
unsigned g_begin_idx = 99;
bool dkn_move_begin(DknSlot& s, daB_TN_c*, daAlink_c*, unsigned idx, DknLease kind, const char*) { ++g_begin, g_begin_idx = idx; if (!g_begin_ok) return false; s.lease = kind; return true; }
bool dkn_move_now(const DknSlot& s, const daB_TN_c* a, dkn::Move& m) {
  if (a->mActionMode1 == dkn::native::kActAttackL && a->mActionMode2 == 1) return m = dkn::Move::F2A, true;
  if (a->mActionMode1 == dkn::native::kActAttackL && a->mActionMode2 == 2) return m = dkn::Move::F2B, true;
  (void)s;
  return false;
}
'''

MAIN = r'''
unsigned checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { std::printf("FAIL line=%d: %s\n", __LINE__, #c); return 1; } } while (0)
int main() {
  using namespace dkn;
  daB_TN_c a;
  a.id = 30, a.name = fpcNm_B_TN_e;
  g_link.id = 1;
  // ---------------------------------------------------------------- 1. geometry of the native leaps
  a.current.pos = {0, 0, 0};
  g_link.current.pos = {520, 0, 0};
  const DknDir fw = dkn_dir_to(&a, &g_link, false), bw = dkn_dir_to(&a, &g_link, true);
  CHECK(std::fabs(fw.x - 1) < 1e-5 && std::fabs(fw.z) < 1e-5 && std::fabs(bw.x + 1) < 1e-5);
  const unsigned g0 = fopAcM_gc_c::calls, l0 = fopAcM_lc_c::calls;
  CHECK(dkn_geo(&a, fw, 245).g == Geo::Ok);                         // flat ground, nothing in the way
  CHECK(fopAcM_gc_c::calls - g0 == 4 && fopAcM_lc_c::calls - l0 == 1);  // three path samples + the landing body sample, one obstacle line: per decision, never per tick
  a.current.pos = {400, 0, 0};
  CHECK(dkn_geo(&a, fw, 405).g == Geo::NoGround);                   // the long leap would cross the pit (600..700) even though x = 805 has ground
  a.current.pos = {700, 0, 0};
  CHECK(dkn_geo(&a, fw, 245).g == Geo::Drop);                       // the landing body sample over the ledge (-200)
  a.current.pos = {635, 0, 0};
  CHECK(dkn_geo(&a, fw, 245).g == Geo::Drop);                       // landing at 880 on solid ground, but the body (+50) would hang over the -200 edge
  a.current.pos = {0, 0, 300};
  g_link.current.pos = {0, 0, 900};
  CHECK(dkn_geo(&a, dkn_dir_to(&a, &g_link, false), 245).g == Geo::Rise);  // a step of +80 (> 50) on the path
  a.current.pos = {1150, 0, 0};
  g_link.current.pos = {1500, 0, 0};
  CHECK(dkn_geo(&a, dkn_dir_to(&a, &g_link, false), 245).g == Geo::Wall);  // the wall at x = 1200 (ground everywhere: only the obstacle line sees it)
  a.current.pos = {1250, 0, 0};
  g_link.current.pos = {1600, 0, 0};
  CHECK(dkn_geo(&a, dkn_dir_to(&a, &g_link, false), 120).g == Geo::Water);  // landing in water (surface 30 over the ground 0)
  a.current.pos = {500, 0, 0};
  g_link.current.pos = {800, 0, 0};
  CHECK(dkn_geo(&a, dkn_dir_to(&a, &g_link, true), 450).g == Geo::Ok);     // the back jump AWAY from Link: flat ground behind (x 50..-0)
  a.current.pos = {1000, -200, 0};
  g_link.current.pos = {1050, -200, 0};
  CHECK(dkn_geo(&a, dkn_dir_to(&a, &g_link, true), 450).g == Geo::NoGround || dkn_geo(&a, dkn_dir_to(&a, &g_link, true), 450).g == Geo::Rise);  // behind: the pit / the ledge up
  // ---------------------------------------------------------------- 2. the thrown mace
  a.current.pos = {0, 0, 0};
  g_link.current.pos = {300, 0, 0};
  g_slot.ph = Ph::Transform;
  a.mActionMode1 = native::kActChangeDemo;
  a.mStates[13] = 2;
  dkn_mace_post(g_slot, &a);
  CHECK(!g_slot.mace.live && !g_slot.mace_anchor);                 // state 2: cc_set anchors nothing this tick
  a.mStates[13] = 3, a.mSwordSphs[0].at_set = true;
  dkn_mace_post(g_slot, &a);
  CHECK(g_slot.mace.live && g_slot.mace.serial == 1 && g_slot.mace_anchor);
  // direct contact: +60 offered on Link's native receipt, never the Darknut
  a.mSwordSphs[2].at_hit = true, a.mSwordSphs[2].hit_obj = &g_link.mTgCyls[0];
  dkn_mace_contact(g_slot, &a);
  CHECK(g_offers == 1 && g_offer_token == (0x2000000000000000ull | 1) && g_slot.mace_direct_token == g_offer_token && g_gains == 0 && g_settle_false == 1);
  dkn_mace_contact(g_slot, &a);                                    // the same throw again (another collider / a bounce): nothing
  CHECK(g_offers == 1 && g_slot.st_mace_repeat == 1 && g_slot.mace.directs == 1);
  // a second throw (a later transformation): block +50
  a.mStates[13] = 7;
  dkn_mace_post(g_slot, &a);
  CHECK(!g_slot.mace.live && !g_slot.mace_anchor);
  dkn_mace_contact(g_slot, &a);                                    // landed: no anchor, the flags of a stale tick are never a contact
  CHECK(g_offers == 1 && g_gains == 0);
  a.mStates[13] = 3;
  dkn_mace_post(g_slot, &a);
  a.mSwordSphs[2].shield = true;
  dkn_mace_contact(g_slot, &a);
  CHECK(g_gains == 1 && g_link_post == 50 && g_blocked == 1 && g_slot.mace.blocks == 1);
  // a third throw: parry (shield + Link's Shield Attack window): +0, the spark confirmed orange, the shield chain confirms it
  a.mStates[13] = 7, dkn_mace_post(g_slot, &a), a.mStates[13] = 3, dkn_mace_post(g_slot, &a);
  g_window = true;
  dkn_mace_contact(g_slot, &a);
  CHECK(g_gains == 1 && g_link_post == 50 && g_parry_conf == 1 && g_settle_true == 1 && g_slot.mace.parries == 1);
  g_window = false;
  // the sword drawn (TNB_PULL frame 10: mStates[15] = 1): the colliders follow the sword, no mace any more
  a.mStates[13] = 7, dkn_mace_post(g_slot, &a), a.mStates[13] = 3, a.mStates[15] = 1;
  dkn_mace_post(g_slot, &a);
  CHECK(!g_slot.mace_anchor && !g_slot.mace.live);
  dkn_mace_contact(g_slot, &a);
  CHECK(g_gains == 1 && g_offers == 1 && g_parry_conf == 1);
  // a contact on something that is not Link: nothing
  a.mStates[15] = 0, a.mStates[13] = 7, dkn_mace_post(g_slot, &a), a.mStates[13] = 3, dkn_mace_post(g_slot, &a);
  dCcD_GObjInf other;
  a.mSwordSphs[2].hit_obj = &other;
  dkn_mace_contact(g_slot, &a);
  CHECK(g_gains == 1 && g_offers == 1 && g_slot.mace.resolved != g_slot.mace.serial);
  // Link saturates at 100 (block 50 twice)
  a.mSwordSphs[2].hit_obj = &g_link.mTgCyls[1];
  dkn_mace_contact(g_slot, &a);
  CHECK(g_link_post == 100);
  a.mStates[13] = 7, dkn_mace_post(g_slot, &a), a.mStates[13] = 3, dkn_mace_post(g_slot, &a);
  dkn_mace_contact(g_slot, &a);
  CHECK(g_link_post == 100);
  // ---------------------------------------------------------------- 3. the thrust class / impact identity LDG01 reads
  g_slot.ph = Ph::Light;
  a.mActionMode1 = native::kActAttackL, a.mActionMode2 = 1;  // TNB_ATACK_A (F2A, thrust)
  g_slot.led.open_window(Move::F2A, 0);
  DknLdg q = dkn_ldg_query(&a, &a.mSwordSphs[1]);
  CHECK(q.own && q.cls == 1 && q.impact == g_slot.led.serial);
  q = dkn_ldg_query(&a, &a.mSphC);
  CHECK(q.own && q.cls == 0);                                          // the shield / kick collider is never the thrust
  a.mActionMode2 = 2;                                                  // the double (F2B): ordinary
  CHECK(dkn_ldg_query(&a, &a.mCps).cls == 0);
  g_slot.ph = Ph::Transform;
  q = dkn_ldg_query(&a, &a.mCps);
  CHECK(q.own && q.cls == 0 && q.impact == (0x2000000000000000ull | g_slot.mace.serial));  // the mace: ordinary class, the throw is its identity
  fopAc_ac_c stranger;
  stranger.name = fpcNm_OTHER_e;
  CHECK(!dkn_ldg_query(&stranger, nullptr).own);
  // ---------------------------------------------------------------- 4. the leap phase: air -> landing -> combo; cancels
  g_slot.ph = Ph::Light;
  a.current.pos = {0, 0, 0};
  g_link.current.pos = {300, 0, 0};
  auto start_leap = [&]() { g_slot.lease = DknLease::Leap, g_slot.leap_phase = Lp::Air, g_slot.leap = Leap::Fwd, g_slot.leap_combo = 5, g_slot.leap_tick = g_tick, g_slot.leap_d0 = 545; };
  start_leap();
  a.mActionMode1 = native::kActChaseL, a.mActionMode2 = native::kChaseJumpFwd;
  for (int i = 0; i < 12; ++i) ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_slot.lease == DknLease::Leap && g_slot.leap_phase == Lp::Air && g_begin == 0);  // in the air: nothing starts
  a.mActionMode2 = native::kChaseJumpFwdLand;
  ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_slot.leap_phase == Lp::Air && g_begin == 0);                                   // the native landing clip: still the leap
  a.mActionMode2 = 1;                                                                    // landed: the neutral chase
  ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_begin == 1 && g_begin_idx == 5 && g_slot.lease == DknLease::Combo && g_slot.st_leap_land == 1 && g_slot.st_leap_combo == 1);  // the rolled combo, at once
  // landed out of reach: waits (never an attack in the void), then the bounded wait cancels
  start_leap();
  g_link.current.pos = {500, 0, 0};
  a.mActionMode2 = 1;
  const int b0 = g_begin;
  for (int i = 0; i < 44; ++i) ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_begin == b0 && g_slot.lease == DknLease::Leap);
  ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_slot.lease == DknLease::None && g_slot.st_leap_cancel == 1 && std::strcmp(g_last_log, "LEAP_CANCEL") == 0);
  // Link escaped beyond 700, an event, no target, guard-broken: cancelled at once, nothing scheduled
  struct { float lx; bool safe, alive, broken; } cases[] = {{800, true, true, false}, {300, false, true, false}, {300, true, false, false}, {300, true, true, true}};
  for (auto& c : cases) {
    start_leap();
    g_link.current.pos = {c.lx, 0, 0}, g_safe = c.safe, g_alive = c.alive, g_broken = c.broken;
    a.mActionMode2 = c.lx > 700 ? 1 : native::kChaseJumpFwd;
    ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
    CHECK(g_slot.lease == DknLease::None);
  }
  g_safe = true, g_alive = true, g_broken = false;
  // the combo refused by the move start (context changed): cancelled, never retried
  start_leap();
  g_link.current.pos = {300, 0, 0};
  a.mActionMode2 = 1;
  g_begin_ok = 0;
  ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_slot.lease == DknLease::None && g_slot.leap_phase == Lp::None);
  g_begin_ok = 1;
  // watchdog in the air
  start_leap();
  a.mActionMode2 = native::kChaseJumpFwd;
  for (int i = 0; i < 130; ++i) ++g_tick, dkn_leap_pre(g_slot, &a, &g_link, g_tick);
  CHECK(g_slot.lease == DknLease::None);
  std::printf("DKN_FIX07_ADAPTER_PASS checks=%u gnd_calls=%u line_calls=%u logs=%u runtime=HUMAN_RUNTIME_PENDING\n", checks, fopAcM_gc_c::calls, fopAcM_lc_c::calls, g_logs);
  return 0;
}
'''

FUNCS = ['struct DknGeo {', 'struct DknDir {', 'DknDir dkn_dir_to(', 'DknGeo dkn_geo(', 'void dkn_mace_post(', 'void dkn_mace_contact(', 'struct DknLdg {', 'DknLdg dkn_ldg_query(',
         'void dkn_leap_cancel(', 'void dkn_leap_pre(']


def extract(source):
    lease = source[source.index('enum class DknLease'):source.index('\n', source.index('enum class DknLease'))]
    out = [lease]
    for f in FUNCS:
        b = body(source, f)
        out.append(b + (';' if f.startswith('struct') else ''))
        if f == 'struct DknGeo {':
            out.append(SLOT)
    return '\n'.join(out)


def run_variant(source, directory, label):
    cpp = directory / (label + '.cpp')
    cpp.write_text(PREFIX + '\n' + extract(source) + '\n' + MAIN)
    exe = directory / (label + ('.exe' if os.name == 'nt' else ''))
    inc = str(ROOT / 'src')
    if os.name == 'nt':
        cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/I' + inc, str(cpp), '/Fe' + str(exe), '/Fo' + str(directory / (label + '.obj'))]
    else:
        cmd = [shutil.which('g++') or 'clang++', '-std=c++20', '-O2', '-I' + inc, str(cpp), '-o', str(exe)]
    built = subprocess.run(cmd, capture_output=True, text=True, cwd=directory, timeout=120)
    if built.returncode:
        raise RuntimeError('compile failure never counts as detection: ' + built.stdout + built.stderr)
    return subprocess.run([str(exe)], capture_output=True, text=True, timeout=20)


def main():
    source = (ROOT / 'src/dkn_adapter.inc').read_text()
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        positive = run_variant(source, directory, 'positive')
        print(positive.stdout, end='')
        if positive.returncode:
            print(positive.stderr)
            return 1
        mutations = [
            ('geo_landing_only', '  const float frac[4] = {1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 1.0f};', '  const float frac[4] = {1.0f, 1.0f, 1.0f, 1.0f};'),
            ('geo_no_wall', '  const bool wall = fopAcM_lc_c::lineCheck(&s0, &s1, a);', '  const bool wall = false && fopAcM_lc_c::lineCheck(&s0, &s1, a);'),
            ('geo_no_body_radius', '    const float d = dist * frac[i] + (i == 3 ? policy::kLeapBodyR : 0.0f);', '    const float d = dist * frac[i];'),
            ('mace_any_state', '  const bool anchor = a->mStates[13] == 3 && a->mStates[15] != 1;', '  const bool anchor = a->mStates[13] == 3;'),
            ('mace_no_ledger', '  if (!s.mace.resolve(m)) {', '  if (!s.mace.resolve(m) && false) {'),
            ('mace_parry_without_window', '  const Mace m = mace_verdict(true, shield, shield && pass.in_window);', '  const Mace m = mace_verdict(true, shield, shield);'),
            ('leap_combo_in_the_void', '  if (st == LpStep::Wait) return;', '  if (st == LpStep::Wait && false) return;'),
            ('thrust_any_collider', '    r.cls = sword && m == dkn::Move::F2A ? 1 : 0;', '    r.cls = m == dkn::Move::F2A ? 1 : 0;'),
        ]
        for label, old, new in mutations:
            assert source.count(old) == 1, label
            result = run_variant(source.replace(old, new, 1), directory, label)
            if result.returncode != 1 or 'FAIL line=' not in result.stdout:
                raise RuntimeError('mutation survived: ' + label + result.stdout + result.stderr)
            print(label + ': DETECTED ' + result.stdout.strip())
    print(json.dumps({'status': 'DKN_FIX07_ADAPTER_PASS', 'mutants_detected': len(mutations), 'mutants_total': len(mutations),
                      'scope': 'production dkn_geo / dkn_mace_post / dkn_mace_contact / dkn_leap_pre / dkn_ldg_query with doubles (terrain, colliders, bridges); no game run'}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
