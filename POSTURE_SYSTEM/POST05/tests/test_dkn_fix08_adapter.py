"""DKN01 FIX08 (0.13.17): execute PRODUCTION code of src/dkn_adapter.inc against doubles: dkn_geo_evade (the whole native path of an evasion read with the
game's static queries over a synthetic terrain: a pit, a sunken floor, a step, a wall, water), dkn_evade_end + dkn_evade_pre (the evasion lease: exceptions,
watchdog, the native landing; the neutral receipt armed after it), dkn_helm_look_end (the native neck flag of mType 1 back OFF, never stale). Plus source
contracts of the action PRE (the evasion only after a parry roll that armed nothing, before the guard entry; the native evasion entries taken back; a native
attempt never bypasses the neutral state; the B1 eased recovery). Every mutant must be detected; a compile error is never a detection. Doubles, not the game."""
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
#include "dkn_core.hpp"
struct cXyz { float x = 0, y = 0, z = 0; cXyz() = default; cXyz(float a, float b, float c) : x(a), y(b), z(c) {} };
namespace kk { inline float fsqrt(float v) { return std::sqrt(v); } }
struct Pos { cXyz pos; };
struct fopAc_ac_c { std::uint32_t id = 0; Pos current; };
// terrain: flat y = 0; a pit x in 600..700; sunken floor -200 over x in 900..1100; a step +80 for z > 500; a wall at x = 1200; water x in 1300..1500
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
struct daAlink_c : fopAc_ac_c {};
struct daB_TN_c : fopAc_ac_c { int mActionMode1 = 9, mActionMode2 = 1; bool mUpdateNeckAngle = false; std::int16_t mNeckAngle = 0; std::int16_t shape_angle_y = 0;
                               struct { std::int16_t y = 0; } shape_angle; unsigned neutral_writes = 0, neck_writes = 0; };
struct DknGeo { dkn::Geo g = dkn::Geo::NotChecked; float ground[4] = {}; float land_x = 0, land_z = 0; };
enum class DknLease : std::uint8_t { None = 0, Combo, Single, Counter, Special, Reposition, Leap, Evade };
struct DknSlot {
  DknLease lease = DknLease::None;
  dkn::Evade evade;
  dkn::Ed evade_dir = dkn::Ed::None;
  std::uint64_t evade_tick = 0, evade_land_tick = 0, evade_key = 0;
  bool evade_link_close = false, evade_hit = false;
  float evade_d0 = 0, evade_x0 = 0, evade_z0 = 0;
  dkn::NeutralGate ngate;
  dkn::HelmLook helm_look;
  bool neck_on = false;
  std::int16_t helm_err0 = 0;
  dkn::Iframe ifr;  // FIX09 (0.13.18): the slot's i-frame window, closed by dkn_evade_end
  bool evade_retreat = false;
  std::uint64_t ifr_open_tick = 0;
};
std::uint64_t g_tick = 1000;
std::uint64_t dknb_tick() { return g_tick; }
daAlink_c g_link;
bool g_alive = true, g_safe = true, g_broken = false;
bool dknb_link_alive() { return g_alive; }
bool dknb_world_safe() { return g_safe; }
bool dknb_link_broken() { return g_broken; }
unsigned g_logs = 0;
char g_last_log[64] = {};
void dkn_log(const char* ev, const DknSlot&, const daB_TN_c*, const char* = "", bool = false) { ++g_logs; std::snprintf(g_last_log, sizeof(g_last_log), "%s", ev); }
float dkn_dist(const daB_TN_c* a, const daAlink_c* k) { const float dx = k->current.pos.x - a->current.pos.x, dz = k->current.pos.z - a->current.pos.z; return std::sqrt(dx * dx + dz * dz); }
std::int16_t dkn_angle_to(const daB_TN_c*, const daAlink_c*) { return 0; }
void dkn_native_neutral(daB_TN_c* a, bool) { a->mActionMode1 = dkn::native::kActChaseL, a->mActionMode2 = 1, ++a->neutral_writes; }
void dkn_native_neck(daB_TN_c* a, bool on) { a->mUpdateNeckAngle = on, ++a->neck_writes; }
'''

TEST = r'''
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line=%d: %s\n", __LINE__, #c); return 1; } } while (0)
int main() {
  using namespace dkn;
  // ---- 1. dkn_geo_evade over the terrain double
  daB_TN_c a;
  a.current.pos = cXyz(0, 0, 0);
  const EvPath L = evade_path(Ed::Left, 0, 250, 0), R = evade_path(Ed::Right, 0, 250, 0), B = evade_path(Ed::Back, 0, 250, 0);
  CHECK(dkn_geo_evade(&a, L).g == Geo::Ok && dkn_geo_evade(&a, R).g == Geo::Ok && dkn_geo_evade(&a, B).g == Geo::Ok);
  const unsigned g0 = fopAcM_gc_c::calls, l0 = fopAcM_lc_c::calls;
  (void)dkn_geo_evade(&a, L);
  CHECK(fopAcM_gc_c::calls - g0 == 4 && fopAcM_lc_c::calls - l0 >= 1);  // three path samples + the body beyond the landing; the obstacle line
  a.current.pos = cXyz(420, 0, 0);   // the left arc (+x; samples +120 / +271 / +315): only its middle (691) is over the pit 600..700, the landing (735) is ground
  CHECK(dkn_geo_evade(&a, L).g == Geo::NoGround && dkn_geo_evade(&a, R).g == Geo::Ok);
  a.current.pos = cXyz(730, 0, 0);   // the left arc reaches the sunken floor 900..1100
  CHECK(dkn_geo_evade(&a, L).g == Geo::Drop);
  a.current.pos = cXyz(1050, 0, 0);  // the left arc crosses the wall at 1200 (the landing beyond it is fine ground)
  CHECK(dkn_geo_evade(&a, L).g == Geo::Wall);
  a.current.pos = cXyz(1150, 0, 0);  // left landing inside the water
  {
    const DknGeo g = dkn_geo_evade(&a, L);
    CHECK(g.g == Geo::Wall || g.g == Geo::Water);
  }
  a.current.pos = cXyz(0, 0, 300);   // the back jump (-z, 450) from z 300: fine; a step +80 behind the forward arcs only
  CHECK(dkn_geo_evade(&a, B).g == Geo::Ok);
  a.current.pos = cXyz(0, 0, 450);
  CHECK(dkn_geo_evade(&a, L).g == Geo::Rise);  // the side arc turns toward Link (z 250 ahead): it climbs the +80 step
  EvPath tiny;
  tiny.n = 2;
  CHECK(dkn_geo_evade(&a, tiny).g == Geo::NoGround);  // a degenerate path is never "safe"
  // ---- 2. the evasion lease: exceptions, watchdog, landing; the neutral receipt is armed by its end
  DknSlot s;
  a.current.pos = cXyz(0, 0, 0), g_link.current.pos = cXyz(0, 0, 300);
  s.lease = DknLease::Evade, s.evade_tick = 1000, s.evade_dir = Ed::Right, a.mActionMode1 = native::kActChaseL, a.mActionMode2 = 11;
  g_tick = 1005;
  dkn_evade_pre(s, &a, &g_link, g_tick);
  CHECK(s.lease == DknLease::Evade && s.evade.lands == 0);  // take-off: still owned
  a.mActionMode2 = 12;
  dkn_evade_pre(s, &a, &g_link, ++g_tick);
  CHECK(s.lease == DknLease::Evade);  // the native landing clip: still owned
  a.mActionMode2 = 1;
  dkn_evade_pre(s, &a, &g_link, ++g_tick);
  CHECK(s.lease == DknLease::None && s.evade.lands == 1 && s.evade_land_tick == g_tick && s.ngate.st == Nr::Await && s.ngate.cause == 4);
  CHECK(std::strcmp(g_last_log, "EVADE_LANDED") == 0);
  // exceptions: no target / event / Link guard-broken / watchdog each end it ONCE, as a cancel
  const auto start = [&](int mode) { s.lease = DknLease::Evade, s.evade_tick = g_tick, a.mActionMode1 = native::kActChaseL, a.mActionMode2 = mode, s.ngate.clear(); };
  start(14);
  dkn_evade_pre(s, &a, nullptr, ++g_tick);
  CHECK(s.lease == DknLease::None && s.evade.cancels == 1 && std::strcmp(g_last_log, "EVADE_CANCEL") == 0 && s.ngate.st == Nr::Await);
  start(14), g_safe = false;
  dkn_evade_pre(s, &a, &g_link, ++g_tick);
  CHECK(s.lease == DknLease::None && s.evade.cancels == 2);
  g_safe = true, start(6), g_broken = true;
  dkn_evade_pre(s, &a, &g_link, ++g_tick);
  CHECK(s.lease == DknLease::None && s.evade.cancels == 3);
  g_broken = false, start(6);
  const unsigned n0 = a.neutral_writes;
  g_tick += policy::kEvadeWatchdogTicks + 1;
  dkn_evade_pre(s, &a, &g_link, g_tick);
  CHECK(s.lease == DknLease::None && s.evade.cancels == 4 && a.neutral_writes == n0 + 1);  // never stuck in a jump mode
  dkn_evade_end(s, &a, &g_link, "AGAIN", false);
  CHECK(s.evade.lands == 1 && s.evade.cancels == 4);  // nothing to end twice
  // ---- 3. the Helm recovery look ends: the native flag of mType 1 back OFF, once
  daB_TN_c h;
  DknSlot t;
  t.helm_look.begin(10);
  dkn_native_neck(&h, true), t.neck_on = true;
  dkn_helm_look_end(t, &h, &g_link, "TAIL_OVER");
  CHECK(!h.mUpdateNeckAngle && !t.neck_on && !t.helm_look.on() && h.neck_writes == 2 && std::strcmp(g_last_log, "HELM_LOOK_END") == 0);
  dkn_helm_look_end(t, &h, &g_link, "AGAIN");
  CHECK(h.neck_writes == 2);
  t.helm_look.begin(20);  // a look that never followed (the neck was never written): ends without a write
  dkn_helm_look_end(t, &h, nullptr, "NO_TARGET");
  CHECK(h.neck_writes == 2 && !h.mUpdateNeckAngle);
  std::printf("DKN_FIX08_ADAPTER_PASS geo_calls=%u line_calls=%u runtime=HUMAN_RUNTIME_PENDING\n", fopAcM_gc_c::calls, fopAcM_lc_c::calls);
  return 0;
}
'''

FUNCS = ['DknGeo dkn_geo_evade(const daB_TN_c* a, const dkn::EvPath& pth) noexcept {',
         'void dkn_evade_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why, bool cancel) noexcept {',
         'void dkn_evade_pre(DknSlot& s, daB_TN_c* a, daAlink_c* k, std::uint64_t now) noexcept {',
         'void dkn_helm_look_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why) noexcept {']


def build(source):
    return PREFIX + '\n'.join(body(source, f) for f in FUNCS) + TEST


def run_variant(source, directory, label):
    cpp = directory / (label + '.cpp')
    exe = directory / (label + ('.exe' if os.name == 'nt' else ''))
    cpp.write_text(build(source))
    inc = str(ROOT / 'src')
    if os.name == 'nt':
        cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/I' + inc, str(cpp), '/Fe' + str(exe), '/Fo' + str(directory / (label + '.obj'))]
    else:
        cmd = [shutil.which('g++') or 'clang++', '-std=c++20', '-O1', '-I' + inc, str(cpp), '-o', str(exe)]
    built = subprocess.run(cmd, capture_output=True, text=True, cwd=directory, timeout=180)
    if built.returncode:
        raise RuntimeError('compile failure never counts as detection: ' + label + built.stdout[-3000:] + built.stderr[-3000:])
    return subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)


def source_contracts(src):
    e = []
    pre = body(src, 'HookAction on_dkn_action_pre(ModContext*, void* args, void*, void*) noexcept {')
    i_ev = pre.find('if (v != Dv::Parry && p2 && k) {')
    i_guard = pre.find('if (v != Dv::None) {', i_ev if i_ev >= 0 else 0)
    if i_ev < 0 or i_guard < 0 or i_ev > i_guard or pre.find('const Dv v = s.def.on_swing(') > i_ev:
        e.append('the evasion is not decided after the parry roll and before the guard entry of the same swing')
    if 'iv.neutral = neutral && gate_ok' not in pre or 'iv.neutral = neutral || from_native' in pre:
        e.append('a native attempt bypasses the neutral state / the neutral receipt')
    if 'iv.link_swing = link_swing;' not in pre:
        e.append('the ordinary initiative ignores Link\'s running swing')
    if pre.find('evade_native_entry_any(act0, mode0)') < 0 or pre.find('evade_native_entry_any(act0, mode0)') > pre.find('native::native_attack_entry(act0, mode0)'):
        e.append('the native evasion entries are not taken back before they run')
    if pre.find('if (s.lease == DknLease::Evade) {') < 0 or pre.find('if (s.lease == DknLease::Evade) {') < pre.find('if (s.brk.st != Bk::Combat) return HOOK_CONTINUE;'):
        e.append('the evasion lease is not handled after the break priority')
    if '!evade_hold' not in pre:
        e.append('the offensive leap may roll on the distance the evasion created')
    post = body(src, 'void dkn_combo_post(DknSlot& s, daB_TN_c* a, daAlink_c* k) noexcept {')
    if 'dkn_native_recover_clip(a, policy::kB1RecoverBck, policy::kB1RecoverMorph);' not in post or 'dkn_native_wait_clip(a);' in post or 'dkn_native_timer1(a, policy::kB1RecoverTicks);' not in post:
        e.append('B1 does not end with the native eased recovery into the combat stance')
    apost = body(src, 'void on_dkn_action_post(ModContext*, void* args, void*, void*) noexcept {')
    if 'if (s.lease == DknLease::Evade) {' not in apost or 'LANDING_ATTACK_TAKEN_BACK' not in apost:
        e.append('the native landing attack of an evasion is not taken back')
    if 'dkn_native_neck(a, true)' not in apost or 's.helm_look.turn(dkn_frame(a))' not in apost:
        e.append('the Helm recovery look (neck, then the body from frame 24) is missing')
    brk = body(src, 'void dkn_begin_break(DknSlot& s, daB_TN_c* a) noexcept {')
    if 's.lease == DknLease::Evade' not in brk or 'dkn_helm_look_end(s, a, dknb_link(), "NEW_BREAK");' not in brk:
        e.append('a posture break does not end the evasion / the Helm look')
    draw = body(src, 'HookAction on_dkn_draw_pre(ModContext*, void* args, void*, void*) noexcept {')
    if 'dn.tint_value = dkn::sword_tint_value(a->mMaterialColor)' not in draw or 'mType != 1' not in draw:
        e.append('the sword darkening value / its Darknut-only scope changed')
    return e


def main():
    source = (ROOT / 'src/dkn_adapter.inc').read_text()
    for f in FUNCS:
        assert source.count(f) == 1, f
    errors = source_contracts(source)
    if errors:
        print(json.dumps({'status': 'DKN_FIX08_ADAPTER_FAIL', 'errors': errors}, indent=1))
        return 1
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        positive = run_variant(source, directory, 'positive')
        print(positive.stdout, end='')
        if positive.returncode:
            print(positive.stderr)
            return 1
        mutations = [
            ('geo_landing_only', '  const unsigned idx[3] = {pth.n / 3, (2 * pth.n) / 3, pth.n - 1};', '  const unsigned idx[3] = {pth.n - 1, pth.n - 1, pth.n - 1};'),
            ('geo_no_wall', '  const bool wall = fopAcM_lc_c::lineCheck(&s0, &s1, a) || fopAcM_lc_c::lineCheck(&s1, &s2, a);', '  const bool wall = false;'),
            ('geo_degenerate_ok', '  if (pth.n < 3) return r.g = Geo::NoGround, r;', '  if (pth.n < 3) return r.g = Geo::Ok, r;'),
            ('evade_end_no_neutral_gate', '  s.ngate.defense(now, 4);  // the next ordinary initiative after an evasion', '  (void)now;  // the next ordinary initiative after an evasion'),
            ('evade_no_watchdog', '  if (now > s.evade_tick + dkn::policy::kEvadeWatchdogTicks) {', '  if (false) {'),
            ('evade_ends_in_flight', '  if (a->mActionMode1 == dkn::native::kActChaseL && dkn::evade_jump_mode(a->mActionMode2)) return;  // take-off', '  if (false) return;  // take-off'),
            ('evade_ignores_event', '  if (!dknb_world_safe()) return dkn_evade_end(s, a, k, "EVENT", true);', ''),
            ('neck_left_on', '  if (s.neck_on) dkn_native_neck(a, false), s.neck_on = false;\n  const int err', '  const int err'),
        ]
        for label, old, new in mutations:
            assert source.count(old) == 1, label
            result = run_variant(source.replace(old, new, 1), directory, label)
            if result.returncode != 1 or 'FAIL line=' not in result.stdout:
                raise RuntimeError('mutation survived: ' + label + result.stdout + result.stderr)
            print(label + ': DETECTED ' + result.stdout.strip())
        source_mutants = [
            ('evade_over_parry', 'if (v != Dv::Parry && p2 && k) {', 'if (p2 && k) {'),
            ('native_attempt_bypass', 'iv.neutral = neutral && gate_ok', 'iv.neutral = neutral || from_native'),
            ('b1_fix07_snap', 'dkn_native_recover_clip(a, policy::kB1RecoverBck, policy::kB1RecoverMorph);\n      dkn_native_timer1(a, policy::kB1RecoverTicks);\n      ++s.st_b1_recover;',
             'dkn_native_wait_clip(a);\n      dkn_native_timer1(a, policy::kB1RecoverTicks);\n      ++s.st_b1_recover;'),  # FIX09: the B1 site (the hit-end / hold reuse the eased clip)
            ('sword_raw_value', 'dn.tint_value = dkn::sword_tint_value(a->mMaterialColor)', 'dn.tint_value = dkn::tint_value(a->mMaterialColor)'),
        ]
        for label, old, new in source_mutants:
            assert source.count(old) == 1, label
            if not source_contracts(source.replace(old, new, 1)):
                raise RuntimeError('source mutation survived: ' + label)
            print(label + ': DETECTED (source contract)')
    total = len(mutations) + len(source_mutants)
    print(json.dumps({'status': 'DKN_FIX08_ADAPTER_PASS', 'mutants_detected': total, 'mutants_total': total,
                      'scope': 'production dkn_geo_evade / dkn_evade_pre / dkn_evade_end / dkn_helm_look_end with doubles + action PRE / POST / B1 / draw source contracts; no game run'}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
