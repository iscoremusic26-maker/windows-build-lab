"""DKN01 FIX09 (0.13.18): execute PRODUCTION code of src/dkn_adapter.inc against doubles. The real DknSlot / DknLease, the real writers (timer7, no_guard,
guard_ready, jump, neutral, action, recover clip, timer1, rate, attack flags, AT) and the real FIX09 functions: on_dkn_after_cross_post (the evasion's i-frames, the
same swing, the rejected Shield Attack, everything else native), dkn_native_evade_post (a native 6 / 11 / 14 written by the action() taken back in its POST with
mTimer7 restored), dkn_iframe_post (the window opened by the native take-off, 6 executed ticks), dkn_evade_start, dkn_evade_end, dkn_hit_end_apply (the voluntary
end after a confirmed life hit: routes, drops, the counter bridge), dkn_at_authority (no new window while ending / while the offer waits for its receipt),
dkn_move_end + dkn_seq_end (real: the reason LINK_HP_HIT, the decision armed, no continuation), dkn_posthit_pre (Table A: hold / back with the real path read),
dkn_front_restore. Every source mutant below must be detected (a compile error is never a detection). Doubles, not the game: no runtime / feel claim."""
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


def slot_types(text):
    a = text.index('enum class DknLease : std::uint8_t')
    b = text.index('struct DknState {')
    return text[a:b]


PREFIX = r'''
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include "dkn_core.hpp"
typedef unsigned char u8;
typedef unsigned short u16;
struct cXyz { float x = 0, y = 0, z = 0; cXyz() = default; cXyz(float a, float b, float c) : x(a), y(b), z(c) {} };
namespace kk { inline float fsqrt(float v) { return std::sqrt(v); } }
struct fopAc_ac_c { std::uint32_t id = 0; short name = 0; struct { cXyz pos; } current; };
enum { fpcNm_B_TN_e = 531, fpcNm_OTHER = 1 };
inline std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) { return a->id; }
inline short fopAcM_GetName(const fopAc_ac_c* a) { return a->name; }
struct cCcD_GObjInf { fopAc_ac_c* ac = nullptr; bool at = false; fopAc_ac_c* GetAc() { return ac; } bool ChkAtSet() const { return at; } void OnAtSetBit() { at = true; }
                      void OffAtSetBit() { at = false; } void ClrAtHit() {} };
struct ModContext {};
namespace mods { template <class T> T arg(void* args, int i) { return reinterpret_cast<T*>(static_cast<void**>(args)[i])[0]; } }
// terrain: flat y = 0, a pit for z in -700..-600 (behind a Darknut at z 0 facing +z), a wall at x = 1200
float ground_at(float x, float z, bool& ok) { ok = true; if (z <= -600 && z >= -700) return ok = false, -1e30f; return 0.0f; }
struct fopAcM_gc_c { static float mGroundY; static bool gndCheck(const cXyz* p) { bool ok; mGroundY = ground_at(p->x, p->z, ok); return ok; } };
float fopAcM_gc_c::mGroundY = 0;
struct fopAcM_lc_c { static bool lineCheck(const cXyz* a, const cXyz* b, const fopAc_ac_c*) { return (a->x - 1200) * (b->x - 1200) < 0; } };
struct fopAcM_wt_c { static float mWaterY; static bool waterCheck(const cXyz*) { return false; } };
float fopAcM_wt_c::mWaterY = 0;
struct daPy_py_c { enum { FLG1_IS_WOLF = 1 }; };
struct daAlink_c : fopAc_ac_c { cCcD_GObjInf mAtCps[3], mAtCyl, mGuardAtCps, mArrow; bool wolf = false; bool checkNoResetFlg1(int) const { return wolf; } };
struct Morf { float frame = 0; struct { float mRate = 1; } mFrameCtrl; float getFrame() const { return frame; } };
struct Acch { bool ground = true; bool ChkGroundHit() const { return ground; } };
struct daB_TN_c : fopAc_ac_c {
  int mActionMode1 = 9, mActionMode2 = 1, mActionMode2Copy = 0, mTimer7 = 0, mTimer1 = 0, field_0x6fc = 0, bck = -1, sword_at = 0;
  u8 mTimer10 = 0;
  bool field_0xa91 = true, mCutFlag = false;
  float speedF = 0, morph = 0;
  struct { std::int16_t y = 0; } shape_angle;
  cCcD_GObjInf mSphA[3], mSphB[3], mSphC, mSwordSphs[4], mCps;
  Acch mAcch;
  Morf morf2;
  Morf* mpModelMorf2 = &morf2;
  unsigned headlock_off = 0, timer7_writes = 0;
  daB_TN_c() { for (auto& x : mSphA) x.ac = this; for (auto& x : mSphB) x.ac = this; mSphC.ac = this; for (auto& x : mSwordSphs) x.ac = this; mCps.ac = this; name = fpcNm_B_TN_e; }
  void setBck(int b, int, float m, float) { bck = b, morph = m; }
  void setSwordAtBit(int w) { sword_at = w; for (auto& x : mSwordSphs) x.at = w != 0; mCps.at = w != 0; }
  void offHeadLockFlg() { ++headlock_off; }
};
constexpr int kLogLineMax = 512;
struct DknGeo { dkn::Geo g = dkn::Geo::NotChecked; float ground[4] = {}; float land_x = 0, land_z = 0; };
struct DknStateT { bool enabled = true; std::uint64_t cross_calls = 0, cross_evaded = 0, cross_held = 0, cross_sa = 0, unsafe_entries = 0; } dn;
'''

MID = r'''
DknSlot g_slots[2];
std::uint64_t g_tick = 1000;
std::uint64_t dknb_tick() { return g_tick; }
daAlink_c g_link;
daAlink_c* g_link_ptr = &g_link;
daAlink_c* dknb_link() { return g_link_ptr; }
bool g_alive = true, g_safe = true, g_broken = false;
bool dknb_link_alive() { return g_alive; }
bool dknb_world_safe() { return g_safe; }
bool dknb_link_broken() { return g_broken; }
void dknb_burst_gap(std::uint32_t) {}
unsigned g_logs = 0, g_lines = 0;
char g_last_log[64] = {};
void dkn_log(const char* ev, const DknSlot&, const daB_TN_c*, const char* = "", bool = false) { ++g_logs; std::snprintf(g_last_log, sizeof(g_last_log), "%s", ev); }
void dkn_line(bool, const char*) { ++g_lines; }
DknSlot* dkn_find(std::uint32_t id) { for (DknSlot& s : g_slots) if (s.on && s.id == id) return &s; return nullptr; }
bool dkn_combat(const DknSlot& s) { return !s.exec && (s.ph == dkn::Ph::Armor || dkn::phase2(s.ph)); }
float dkn_frame(const daB_TN_c* a) { return a->mpModelMorf2->getFrame(); }
float dkn_dist(const daB_TN_c* a, const daAlink_c* k) { const float dx = k->current.pos.x - a->current.pos.x, dz = k->current.pos.z - a->current.pos.z; return std::sqrt(dx * dx + dz * dz); }
std::int16_t dkn_angle_to(const daB_TN_c*, const daAlink_c*) { return 0; }
DknStage& dkn_stage(DknSlot& s) { return s.stg[dkn::stage(s.ph)]; }
'''

TEST = r'''
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line=%d: %s\n", __LINE__, #c); return 1; } } while (0)
bool cross(const void* at, const void* tg, bool ret0 = false) {
  bool r = ret0;
  const void* args[8] = {nullptr, nullptr, nullptr, &at, &tg, nullptr, nullptr, nullptr};
  on_dkn_after_cross_post(nullptr, const_cast<void**>(args), &r, nullptr);
  return r;
}
int main() {
  using namespace dkn;
  daB_TN_c a;
  a.id = 77;
  DknSlot& s = g_slots[0];
  s = DknSlot{};
  s.on = true, s.id = 77, s.generation = 3, s.ph = Ph::Light, s.post.reset(160);
  g_link.current.pos = cXyz(0, 0, 250);
  // ---- 1. the crossing filter: i-frame window, the same swing, a new swing, the rejected Shield Attack, everything else native
  s.ifr.begin(1), s.evade_token = 1, s.lease = DknLease::Evade, s.link_key = 50;
  g_tick = 2000;
  CHECK(s.ifr.tick(g_tick, true, true));
  CHECK(cross(&g_link.mAtCps[0], &a.mSphA[1]) && s.st_x_evaded == 1);           // sword inside the window: evaded
  CHECK(cross(&g_link.mAtCyl, &a.mSphB[0]) && s.st_x_evaded == 2);              // the spin collider too
  CHECK(!cross(&g_link.mArrow, &a.mSphA[0]));                                     // an arrow / object: native
  CHECK(!cross(&g_link.mAtCps[0], &a.mSwordSphs[0]));                             // not a hurt sphere: native
  CHECK(cross(&g_link.mAtCps[0], &a.mSphA[0], true) && s.st_x_evaded == 2);      // already "no hit": untouched, not counted
  for (int i = 1; i < 6; ++i) { ++g_tick; CHECK(s.ifr.tick(g_tick, true, true)); CHECK(cross(&g_link.mAtCps[1], &a.mSphA[2])); }
  ++g_tick;
  CHECK(!s.ifr.tick(g_tick, true, true));                                         // the 7th executed tick: closed
  CHECK(cross(&g_link.mAtCps[1], &a.mSphA[2]) && s.st_x_held == 1);               // the SAME swing crossing on right after: evaded (consumed)
  ++g_tick, s.link_key = 51;
  CHECK(!cross(&g_link.mAtCps[1], &a.mSphA[2]));                                  // a NEW swing after the window: native (it can hit)
  g_tick += 3, s.link_key = 50;
  CHECK(!cross(&g_link.mAtCps[1], &a.mSphA[2]));                                  // a gap: native
  // the rejected Shield Attack (phases 2-3) never crosses; the Helm route and phase 1 stay native
  s.lease = DknLease::None;
  CHECK(cross(&g_link.mGuardAtCps, &a.mSphA[0]) && s.st_sa_filtered == 1);
  s.brk.st = Bk::StaggerA, s.brk.start = 100, s.sa_serial = 4, s.sa_used = 3, s.sa_tick = 150;
  CHECK(!cross(&g_link.mGuardAtCps, &a.mSphA[0]));                                // a NEW Shield Attack in the first stagger: native (Helm Splitter)
  s.sa_used = 4;
  CHECK(cross(&g_link.mGuardAtCps, &a.mSphA[0]));                                 // the same one again: rejected anyway -> no crossing
  s.brk.st = Bk::Combat, s.ph = Ph::Armor;
  CHECK(!cross(&g_link.mGuardAtCps, &a.mSphA[0]) && !cross(&g_link.mAtCps[0], &a.mSphC));  // phase 1: the physical shield, native
  s.ph = Ph::Light;
  daB_TN_c other;
  other.id = 99;
  CHECK(!cross(&g_link.mAtCps[0], &other.mSphA[0]) && !cross(&g_link.mGuardAtCps, &other.mSphA[0]));  // a Darknut without a slot: native
  g_link_ptr = nullptr;
  CHECK(!cross(&g_link.mAtCps[0], &a.mSphA[0]));
  g_link_ptr = &g_link;
  // ---- 2. a native evasion written by THIS action(): taken back in its POST, mTimer7 restored; the module's own jump and the projectile evasions untouched
  s.pa_tick = g_tick, s.pa_act = 9, s.pa_mode = 4, s.pa_t7 = 0, s.lease = DknLease::None;
  a.mActionMode1 = 9, a.mActionMode2 = 11, a.mTimer7 = 10;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 1 && a.mTimer7 == 0 && s.st_t7_restored == 1 && s.st_native_evade_post == 1);
  s.pa_t7 = 3, a.mActionMode2 = 14, a.mTimer7 = 10, s.pa_mode = 1;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 1 && a.mTimer7 == 3);                                   // the pre-action value, never 0 by default
  s.pa_mode = 6, a.mActionMode2 = 6, a.mTimer7 = 0;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 6 && s.st_native_evade_post == 2);                      // the module's own back jump (written in the PRE): kept
  s.pa_mode = 1, a.mActionMode2 = 10, a.mTimer7 = 10;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 10 && a.mTimer7 == 10);                                 // the projectile evasion (10 / 13): native
  a.mActionMode2 = 11, s.pa_tick = g_tick - 1;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 11);                                                    // a stale snapshot never decides anything
  s.pa_tick = g_tick, s.ph = Ph::Armor;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 11);                                                    // phase 1: no such entry
  s.ph = Ph::Light, s.pa_act = 15, s.pa_mode = 10, a.mActionMode1 = 9, a.mActionMode2 = 14, a.mTimer7 = 10, s.pa_t7 = 0;
  dkn_native_evade_post(s, &a, g_tick);
  CHECK(a.mActionMode2 == 1 && a.mTimer7 == 0);                                   // executeYoroke's own 14: taken back too
  // ---- 3. the maneuver start (no guard, one token, the cooldown) and its window opened by the native take-off
  DknSlot& e = g_slots[1];
  e = DknSlot{};
  e.on = true, e.id = 78, e.ph = Ph::Light, e.post.reset(160);
  daB_TN_c b;
  b.id = 78, b.field_0xa91 = true, b.mTimer10 = 5, b.mCutFlag = true, b.mActionMode2 = 1;
  g_tick = 3000;
  dkn_evade_start(e, &b, Ed::Right, 220.0f, g_tick, false);
  CHECK(e.lease == DknLease::Evade && b.mActionMode2 == 11 && b.speedF == 0.0f && !b.field_0xa91 && b.mTimer10 == 0 && !b.mCutFlag && e.ifr.armed && !e.ifr.open);
  CHECK(e.evade.cooldown_until == g_tick + policy::kEvadeCooldownTicks && e.evade_token == 1 && !e.evade_retreat);
  unsigned covered = 0;
  for (int t = 0; t < 30; ++t, ++g_tick) {
    b.speedF = t >= 4 ? 45.0f : 0.0f;                                            // the native take-off at clip frame 4
    if (t == 12) b.mActionMode2 = 12, b.speedF = 15.0f;                          // the landing clip: still the lease, never a re-arm
    dkn_iframe_post(e, &b, g_tick);
    covered += e.ifr.covers(g_tick);
    if (t == 3) CHECK(!e.ifr.covers(g_tick));                                     // before the take-off: vulnerable
    if (t == 4) CHECK(e.ifr.covers(g_tick) && e.ifr_open_tick == g_tick);
  }
  CHECK(covered == 6 && e.ifr.closed_full == 1);
  // a landing clip with speed but no take-off seen never opens a window (e.g. a maneuver whose take-off tick did not run)
  DknSlot f = e;
  f.ifr.begin(9), f.evade_takeoff = false, b.mActionMode2 = 12, b.speedF = 15.0f;
  for (int t = 0; t < 5; ++t) { dkn_iframe_post(f, &b, ++g_tick); CHECK(!f.ifr.covers(g_tick)); }
  // the end of the maneuver closes the window at once
  f.ifr.begin(10), f.evade_takeoff = false, b.mActionMode2 = 11, b.speedF = 45.0f;
  dkn_iframe_post(f, &b, ++g_tick);
  CHECK(f.ifr.covers(g_tick));
  dkn_evade_end(f, &b, &g_link, "LANDED_NATIVE", false);
  CHECK(!f.ifr.covers(g_tick) && !f.ifr.open);                                   // the end itself closes it in the same tick (no POST needed)
  dkn_iframe_post(f, &b, ++g_tick);
  CHECK(!f.ifr.covers(g_tick) && f.lease == DknLease::None);
  // ---- 4. the voluntary end after a confirmed life hit
  DknSlot c = DknSlot{};
  c.on = true, c.id = 80, c.ph = Ph::Light, c.post.reset(160);
  for (unsigned i = 0; i < kPlans; ++i) g_plans[i] = plan(combo_of(i), i);
  daB_TN_c d;
  d.id = 80;
  const auto start_combo = [&](unsigned idx) {
    c.lease = DknLease::Combo, c.combo_idx = idx, c.plan = g_plans[idx], c.seg = 0, c.tail = false, c.hit_ending = false, ++c.off_id, c.combo_tick = g_tick;
    d.mActionMode1 = info(c.plan.seg[0].m).act, d.mActionMode2 = info(c.plan.seg[0].m).mode, d.setSwordAtBit(1);
  };
  const auto hit = [&](Move m, unsigned k) {
    c.hit_pending = HitSnap{};
    c.hit_pending.live = true, c.hit_pending.offensive = c.off_id, c.hit_pending.move = m, c.hit_pending.k = static_cast<std::uint8_t>(k), c.hit_pending.seg = static_cast<std::uint8_t>(c.seg);
  };
  // (a) F2-C1: the double's FIRST cut hurt Link: eased recovery now, its second cut never armed
  g_tick = 5000;
  start_combo(4);
  d.morf2.frame = 18.0f;
  hit(Move::F2B, 0);
  dkn_hit_end_apply(c, &d, g_tick);
  CHECK(c.lease == DknLease::None && d.mActionMode1 == native::kActChaseL && d.mActionMode2 == 2 && d.bck == policy::kB1RecoverBck && d.morph == policy::kB1RecoverMorph);
  CHECK(d.mTimer1 == policy::kB1RecoverTicks && d.sword_at == 0 && c.st_hit_end == 1 && c.pht.pending && !c.hit_ending && std::strcmp(g_last_log, "HIT_END") == 0);
  // (b) F2-C2: a thrust in the middle of the plan: its own follow-through (tail at the native rate), no further window
  start_combo(5);
  d.morf2.frame = 11.0f, d.morf2.mFrameCtrl.mRate = 1.7f;
  hit(Move::F2A, 0);
  dkn_hit_end_apply(c, &d, ++g_tick);
  CHECK(c.lease == DknLease::Combo && c.tail && c.hit_ending && d.morf2.mFrameCtrl.mRate == 1.0f && d.mActionMode2Copy == 1 && !d.field_0xa91 && c.hit_route == He::Tail);
  // no new impact window while it ends (the second thrust's window would be the next one)
  c.led.close();
  d.mActionMode1 = native::kActAttackL, d.mActionMode2 = 1, d.morf2.frame = 9.0f;   // a frame inside the thrust window
  const std::uint64_t ser = c.led.serial;
  dkn_at_authority(c, &d);
  CHECK(c.led.serial == ser && c.st_barrier >= 1 && d.sword_at == 0);
  // the tail ends at the native clip end with the reason LINK_HP_HIT: counted apart, the one decision armed, never a completed combo
  const std::uint64_t ends0 = c.stg[1].combo_end[1], cancels0 = c.stg[1].combo_cancel[1];
  c.pht.pending = false;
  dkn_move_end(c, &d, "LINK_HP_HIT", false);
  CHECK(c.stg[1].combo_end[1] == ends0 && c.stg[1].combo_cancel[1] == cancels0 && c.pht.pending && c.st_hit_end == 2 && c.lease == DknLease::None);
  // (c) another offensive / a break / an ending already running: nothing written
  start_combo(5);
  const int m1 = d.mActionMode1, m2 = d.mActionMode2;
  hit(Move::F2A, 0);
  c.hit_pending.offensive = c.off_id - 1;
  dkn_hit_end_apply(c, &d, ++g_tick);
  CHECK(c.lease == DknLease::Combo && !c.tail && !c.hit_ending && d.mActionMode1 == m1 && d.mActionMode2 == m2 && std::strcmp(g_last_log, "HIT_END_DROPPED") == 0);
  hit(Move::F2A, 0);
  c.brk.st = Bk::StaggerA;
  dkn_hit_end_apply(c, &d, ++g_tick);
  CHECK(!c.hit_ending && c.st_hit_dropped == 2);
  c.brk.st = Bk::Combat;
  // (d) the offer waits for its receipt this tick: a NEW window is deferred one tick, then opens (a rejected offer never removes the second cut)
  start_combo(4);
  c.off.live = true, c.off.tick = g_tick;
  d.mActionMode1 = native::kActAttackL, d.mActionMode2 = 2, d.morf2.frame = 22.0f, c.led.close();
  dkn_at_authority(c, &d);
  CHECK(!c.led.open && d.sword_at == 0);
  ++g_tick;
  dkn_at_authority(c, &d);
  CHECK(c.led.open && c.led.k == 1 && d.sword_at == 1);
  // (e) a counter that ended naturally on that very impact: its 80 / 20 continuation is never rolled (the sequence ends LinkHit) and one decision follows
  c.lease = DknLease::None, c.pht.pending = false;
  c.seq.rng.seed(1);
  CHECK(c.seq.begin(g_tick, g_tick - 30, false, 12) && c.seq.started(false));
  c.seq.st = Sq::Bridge;
  c.ended_off = c.off_id, c.ended_off_tick = g_tick;
  hit(Move::F2A, 0);
  dkn_hit_end_apply(c, &d, ++g_tick);
  CHECK(!c.seq.owns() && c.seq.ended[unsigned(SqEnd::LinkHit)] == 1 && c.pht.pending);
  // (f) a counter double ended on its FIRST cut by the hit: LinkHit, no continuation roll
  c.pht.pending = false;
  CHECK(c.seq.begin(g_tick, g_tick - 30, false, 11));
  c.def.cp = Cp::None;
  start_combo(11);
  c.lease = DknLease::Counter;
  CHECK(c.seq.started(false));
  const std::uint64_t rolls0 = c.seq.cont_rolls;
  hit(Move::F2B, 0);
  dkn_hit_end_apply(c, &d, ++g_tick);
  CHECK(c.lease == DknLease::None && !c.seq.owns() && c.seq.cont_rolls == rolls0 && c.pht.pending);
  // ---- 5. the ONE decision after the recovery (Table A): hold in place / the native back jump with its path read; ineligible = no roll
  DknSlot h = DknSlot{};
  h.on = true, h.id = 81, h.ph = Ph::Revived, h.post.reset(180), h.post.gain(170);  // posture 170/180: critical band
  daB_TN_c x;
  x.id = 81, x.current.pos = cXyz(0, 0, 0), x.mActionMode1 = 9, x.mActionMode2 = 2, x.mTimer1 = 10;
  g_link.current.pos = cXyz(0, 0, 200);
  h.pht.arm(g_tick, 1);
  CHECK(!dkn_posthit_pre(h, &x, &g_link, g_tick, true, 9, 2, 200) && h.pht.pending);   // the eased stance still plays: wait
  x.mTimer1 = 2;
  h.evade.cooldown_until = g_tick + 30;                                            // the common cooldown: not eligible -> hold, no roll
  const std::uint64_t r0 = h.pht.rng.s;
  CHECK(!dkn_posthit_pre(h, &x, &g_link, g_tick, true, 9, 2, 200) && !h.pht.pending && h.pht.rng.s == r0 && h.pht.holds[unsigned(Ph9::HoldIneligible)] == 1);
  CHECK(x.mActionMode2 == 2 && x.mTimer1 == policy::kPostHitHoldTicks && h.ph_hold_until == g_tick + policy::kPostHitHoldTicks);
  // eligible, critical band: one roll; back = the native back jump (an evasion: no guard, i-frame token, the cooldown), hold = in place
  unsigned backs = 0, holds = 0;
  for (unsigned i = 0; i < 40; ++i) {
    h.lease = DknLease::None, h.evade.cooldown_until = 0, h.pht.arm(g_tick, 1), x.mActionMode2 = 1, x.field_0xa91 = true;
    Rng probe = h.pht.rng;
    const bool want_back = probe.pct(retreat_pct(3));
    const bool r = dkn_posthit_pre(h, &x, &g_link, g_tick, true, 9, 1, 200);
    CHECK(r == want_back);
    if (r) {
      CHECK(h.lease == DknLease::Evade && x.mActionMode2 == 6 && h.evade_retreat && !x.field_0xa91 && h.ifr.armed);
      ++backs;
    } else {
      CHECK(x.mActionMode2 == 2 && x.mTimer1 == policy::kPostHitHoldTicks);
      ++holds;
    }
    g_tick += 200;
  }
  CHECK(backs > 15 && holds > 5);
  // an unsafe path behind (the pit 600..700 behind): a won roll HOLDS (never a side / forward substitute), the roll was consumed once
  x.current.pos = cXyz(0, 0, -200);
  g_link.current.pos = cXyz(0, 0, 0);
  unsigned geo = 0;
  for (unsigned i = 0; i < 40; ++i) {
    h.lease = DknLease::None, h.evade.cooldown_until = 0, h.pht.arm(g_tick, 1), x.mActionMode2 = 1;
    CHECK(!dkn_posthit_pre(h, &x, &g_link, g_tick, true, 9, 1, 200) && h.lease == DknLease::None);
    g_tick += 200;
  }
  geo = static_cast<unsigned>(h.pht.holds[unsigned(Ph9::HoldGeometry)]);
  CHECK(geo > 10);
  // phase 1: hold, no write (the native ChaseH), no roll
  DknSlot p1 = DknSlot{};
  p1.on = true, p1.id = 82, p1.ph = Ph::Armor, p1.post.reset(200), p1.pht.arm(g_tick, 1);
  daB_TN_c y;
  y.id = 82, y.mActionMode1 = native::kActChaseH, y.mActionMode2 = 5;
  const std::uint64_t q0 = p1.pht.rng.s;
  CHECK(!dkn_posthit_pre(p1, &y, &g_link, g_tick, false, native::kActChaseH, 5, 200) && y.mActionMode2 == 5 && p1.pht.rng.s == q0 && p1.pht.holds[unsigned(Ph9::HoldF1)] == 1);
  // the decision never waits forever
  h.pht.arm(g_tick, 1);
  x.mActionMode1 = native::kActGuardL;
  CHECK(!dkn_posthit_pre(h, &x, &g_link, g_tick + policy::kPostHitDecideMax + 1, true, native::kActGuardL, 1, 200) && !h.pht.pending && h.pht.watchdog == 1);
  // ---- 6. the front rule's restore: the neutral stance gets its guard-ready flag back after the one damage_check
  DknSlot& fr = g_slots[1];
  daB_TN_c z;
  z.id = 78, z.field_0xa91 = false, fr.a91_cleared = true, fr.brk.st = Bk::Combat;
  void* zargs[1] = {nullptr};
  daB_TN_c* zp = &z;
  zargs[0] = &zp;
  dkn_front_restore(zargs);
  CHECK(z.field_0xa91 && !fr.a91_cleared);
  z.field_0xa91 = false, fr.a91_cleared = true, z.mActionMode1 = native::kActEnding;
  dkn_front_restore(zargs);
  CHECK(!z.field_0xa91 && !fr.a91_cleared);                                        // the lethal hit: nothing restored
  std::printf("DKN_FIX09_ADAPTER_PASS cross=%llu evaded=%llu held=%llu sa=%llu hit_ends=%llu backs=%u holds=%u geo=%u runtime=HUMAN_RUNTIME_PENDING\n",
              static_cast<unsigned long long>(dn.cross_calls), static_cast<unsigned long long>(dn.cross_evaded), static_cast<unsigned long long>(dn.cross_held),
              static_cast<unsigned long long>(dn.cross_sa), static_cast<unsigned long long>(c.st_hit_end), backs, holds, geo);
  return 0;
}
'''

WRITERS = ['DKN_NOINLINE void dkn_native_action(daB_TN_c* a, int act, int mode) noexcept {', 'DKN_NOINLINE void dkn_native_rate(daB_TN_c* a, float rate) noexcept {',
           'DKN_NOINLINE void dkn_native_at(daB_TN_c* a, bool sword, int want, bool capsule_only = false) noexcept {',
           'DKN_NOINLINE void dkn_native_attack_flags(daB_TN_c* a) noexcept {', 'DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept {',
           'DKN_NOINLINE void dkn_native_neutral(daB_TN_c* a, bool phase2) noexcept {', 'DKN_NOINLINE void dkn_native_jump(daB_TN_c* a, int mode) noexcept {',
           'DKN_NOINLINE void dkn_native_recover_clip(daB_TN_c* a, int bck, float morph) noexcept {', 'DKN_NOINLINE void dkn_native_timer7(daB_TN_c* a, int v) noexcept {',
           'DKN_NOINLINE void dkn_native_no_guard(daB_TN_c* a) noexcept {', 'DKN_NOINLINE void dkn_native_guard_ready(daB_TN_c* a, bool on) noexcept {']
FUNCS = ['int dkn_remaining(const DknSlot& s, const daB_TN_c* a) noexcept {', 'int dkn_life_max(const DknSlot& s) noexcept {',
         'bool dkn_owned_move(const DknSlot& s) noexcept {', 'bool dkn_move_now(const DknSlot& s, const daB_TN_c* a, dkn::Move& m) noexcept {',
         'bool dkn_sword_move(dkn::Move m) noexcept {', 'void dkn_at_authority(DknSlot& s, daB_TN_c* a) noexcept {',
         'void dkn_seq_end(DknSlot& s, daB_TN_c* a, dkn::SqEnd e, const char* detail) noexcept {', 'void dkn_move_end(DknSlot& s, daB_TN_c* a, const char* why, bool cancel) noexcept {',
         'DknGeo dkn_geo_evade(const daB_TN_c* a, const dkn::EvPath& pth) noexcept {', 'void dkn_evade_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why, bool cancel) noexcept {',
         'unsigned dkn_band(const DknSlot& s, const daB_TN_c* a) noexcept {',
         'void dkn_evade_start(DknSlot& s, daB_TN_c* a, dkn::Ed dir, float dist, std::uint64_t now, bool retreat) noexcept {',
         'void dkn_hit_end_apply(DknSlot& s, daB_TN_c* a, std::uint64_t now) noexcept {',
         'bool dkn_posthit_pre(DknSlot& s, daB_TN_c* a, daAlink_c* k, std::uint64_t now, bool p2, int act, int mode, float dist) noexcept {',
         'void dkn_native_evade_post(DknSlot& s, daB_TN_c* a, std::uint64_t now) noexcept {', 'void dkn_iframe_post(DknSlot& s, daB_TN_c* a, std::uint64_t now) noexcept {',
         'void on_dkn_after_cross_post(ModContext*, void* args, void* ret, void*) noexcept {', 'void dkn_front_restore(void* args) noexcept {']


def build(source):
    w = '\n'.join(body(source, f).replace('DKN_NOINLINE ', '') for f in WRITERS)
    fn = '\n'.join(body(source, f) for f in FUNCS)
    return (PREFIX + slot_types(source) + '\ndkn::Plan g_plans[dkn::kPlans];\n' + MID + w + '\n' + fn + TEST)


def run_variant(source, directory, label):
    cpp = directory / (label + '.cpp')
    exe = directory / (label + ('.exe' if os.name == 'nt' else ''))
    cpp.write_text(build(source))
    inc = str(ROOT / 'src')
    if os.name == 'nt':
        cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/I' + inc, str(cpp), '/Fe' + str(exe), '/Fo' + str(directory / (label + '.obj'))]
    else:
        cmd = [shutil.which('g++') or 'clang++', '-std=c++20', '-O1', '-I' + inc, str(cpp), '-o', str(exe)]
    built = subprocess.run(cmd, capture_output=True, text=True, cwd=directory, timeout=240)
    if built.returncode:
        raise RuntimeError('compile failure never counts as detection: ' + label + built.stdout[-3000:] + built.stderr[-3000:])
    return subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)


def source_contracts(src):
    e = []
    pre = body(src, 'HookAction on_dkn_action_pre(ModContext*, void* args, void*, void*) noexcept {')
    po = body(src, 'void on_dkn_action_post(ModContext*, void* args, void*, void*) noexcept {')
    dpre = body(src, 'HookAction on_dkn_damage_pre(ModContext*, void* args, void*, void*) noexcept {')
    inst = body(src, 'void dkn_install() noexcept {')
    if 'dkn_hit_end_apply(s, a, now);' not in pre or pre.index('dkn_hit_end_apply(s, a, now);') > pre.index('if (s.brk.st == Bk::Pending) {'):
        e.append('the confirmed-hit end is not applied before anything else of the PRE can arm the next impact')
    if 'if (dkn_posthit_pre(s, a, k, now, p2, act, mode, dist)) return HOOK_CONTINUE;' not in pre:
        e.append('the post-hit decision is not run')
    if pre.count('dkn_native_no_guard(a)') != 3:
        e.append('the airborne maneuvers (evasion, leap in the air, reposition) are not all kept off guard')
    if 'ev.band = dkn_band(s, a);' not in pre or 'evade_pick_band(ev.band, dist,' not in pre or '!pending && !repo_gap && !s.pht.pending' not in pre:
        e.append('the evasion ignores the caution band / the pending contact / the reposition gap / the pending decision')
    if 'iv.link_reacting = reacting;' not in pre or '!reacting && !s.pht.pending && now >= s.ph_hold_until' not in pre or 'iv.busy = dkn_busy(s) || def_busy || ph_hold' not in pre:
        e.append('the ordinary initiative / the leap may chase Link into his reaction / over the decision / the hold')
    if 'dkn_native_evade_post(s, a, now);' not in po or 'dkn_iframe_post(s, a, now);' not in po or po.index('dkn_iframe_post(s, a, now);') > po.index('dkn_at_authority(s, a);'):
        e.append('the POST does not take the native evasion back / does not count the i-frame window before cc_set')
    if 'guard_needs_front_clear(true, a->field_0xa91,' not in dpre or 'dkn_native_guard_ready(a, false), s.a91_cleared = true' not in dpre:
        e.append('the phase-2/3 front rule is not applied before the native damage_check')
    if 'dkn_front_restore(args);' not in body(src, 'void on_dkn_damage_post(ModContext*, void* args, void*, void*) noexcept {'):
        e.append('the guard-ready flag of the front rule is never restored')
    if 'mods::hook::add_post<LdgAfterCross>(on_dkn_after_cross_post)' not in inst or 'add_pre<DknAction>(on_dkn_action_pre_fix09)' not in inst:
        e.append('the crossing callback / the PRE snapshot is not installed')
    if 'evade.rng.seed(' not in src or 'pht.rng.seed(' not in src:
        e.append('the evasion / retreat streams are not seeded per actor and generation')
    return e


MUTANTS = [
    ('t7_not_restored', '    if (t7 != s.pa_t7) dkn_native_timer7(a, s.pa_t7), ++s.st_t7_restored;', '    (void)t7;'),
    ('own_jump_taken_back', 'phase2(s.ph) && native_evade_written(s.pa_act, s.pa_mode, a->mActionMode1, a->mActionMode2)) {', 'phase2(s.ph) && native_evade_entry(a->mActionMode1, a->mActionMode2)) {'),
    ('iframe_opens_on_landing', '  if (s.lease == DknLease::Evade && !s.evade_takeoff && native_evade_entry(a->mActionMode1, a->mActionMode2) && a->speedF > 0.0f) s.evade_takeoff = true;',
     '  if (s.lease == DknLease::Evade && !s.evade_takeoff && a->speedF > 0.0f) s.evade_takeoff = true;'),
    ('iframe_from_start', '    const bool prot = s.ifr.tick(now, s.evade_takeoff, s.lease == DknLease::Evade);', '    const bool prot = s.ifr.tick(now, true, s.lease == DknLease::Evade);'),
    ('cross_any_collider', '  if (!sword && !sa) return;  // every other crossing of the game: pointer comparisons only', '  (void)0;'),
    ('cross_any_sphere', '  if (!hurt) return;\n  const std::uint64_t pass = dknb_tick();', '  const std::uint64_t pass = dknb_tick();'),
    ('cross_no_held', '  x.held = !x.window && s->ifh.continues(s->link_key, pass, s->evade_token);', '  x.held = false;'),
    ('cross_sa_helm_filtered', '  x.helm_route = s->brk.st == dkn::Bk::StaggerA && s->sa_serial != s->sa_used && s->sa_tick >= s->brk.start;', '  x.helm_route = false;'),
    ('evade_start_guard_kept', '  dkn_native_no_guard(a);\n  dkn_native_jump(a, dkn::evade_mode(dir));', '  dkn_native_jump(a, dkn::evade_mode(dir));'),
    ('evade_start_no_token', '  s.ifr.begin(++s.evade_token), s.ifh.clear(), s.evade_takeoff = false, s.evade_retreat = retreat;', '  s.ifh.clear(), s.evade_takeoff = false, s.evade_retreat = retreat;'),
    ('evade_end_keeps_window', '  s.ifr.end();  // FIX09: the i-frames never outlive the maneuver\n', ''),
    ('hit_end_at_kept', '        dkn_native_at(a, true, 0);\n        dkn_native_action(a, native::kActChaseL, 2);', '        dkn_native_action(a, native::kActChaseL, 2);'),
    ('hit_end_any_offensive', '  } else if (h.offensive != s.off_id) {\n    drop = "ANOTHER_OFFENSIVE";', '  } else if (false) {\n    drop = "ANOTHER_OFFENSIVE";'),
    ('hit_end_over_break', '  if (s.brk.st != Bk::Combat || s.exec || !dkn_combat(s)) {\n    drop = "BREAK_OR_STATE_PRIORITY", drop_warn = false;', '  if (s.exec || !dkn_combat(s)) {\n    drop = "BREAK_OR_STATE_PRIORITY", drop_warn = false;'),
    ('hit_end_tail_keeps_rate', '        s.tail = true;\n        dkn_native_rate(a, 1.0f);', '        s.tail = true;'),
    ('hit_end_bridge_continues', '    if (s.seq.owns() && s.seq.st == Sq::Bridge) dkn_seq_end(s, a, SqEnd::LinkHit, "LINK_HP_HIT_AFTER_COUNTER_END");', '    (void)0;'),
    ('barrier_off', '    if (fresh_window && (s.hit_ending || (s.off.live && s.off.tick == dknb_tick()))) {', '    if (false) {'),
    ('barrier_forever', '    if (fresh_window && (s.hit_ending || (s.off.live && s.off.tick == dknb_tick()))) {', '    if (fresh_window && (s.hit_ending || s.off.live)) {'),
    ('move_end_counts_combo', '  if (s.lease == DknLease::Combo && s.combo_idx < 8 && !hit_end) {', '  if (s.lease == DknLease::Combo && s.combo_idx < 8) {'),
    ('move_end_no_decision', '    if (!cancel) s.pht.arm(dknb_tick(), s.off_id);  // ONE hold / back decision once the natural recovery is over (never for an exception)', '    (void)cancel;'),
    ('posthit_no_geometry', '    return gb.g == Geo::Ok;\n  });', '    return true;\n  });'),
    ('posthit_ignores_cooldown', '                        now >= s.evade.cooldown_until && !repo_gap && !s.def.busy() && !s.seq.owns();', '                        !repo_gap && !s.def.busy() && !s.seq.owns();'),
    ('posthit_hold_walks', '      dkn_native_action(a, native::kActChaseL, 2);\n      dkn_native_recover_clip(a, policy::kB1RecoverBck, policy::kB1RecoverMorph);\n    }\n    dkn_native_timer1(a, policy::kPostHitHoldTicks);',
     '      dkn_native_action(a, native::kActChaseL, 3);\n    }\n    dkn_native_timer1(a, policy::kPostHitHoldTicks);'),
    ('posthit_decides_mid_recovery', '  if (!settled) return false;  // the eased stance / the follow-through still plays', '  (void)settled;'),
    ('posthit_no_watchdog', '  if (s.pht.expired(now)) {', '  if (false) {'),
    ('front_never_restored', '  if (a->mActionMode1 != dkn::native::kActEnding && a->mActionMode1 != dkn::native::kActChangeDemo && sp->brk.st == dkn::Bk::Combat) dkn_native_guard_ready(a, true);', '  (void)a;'),
]


def main():
    src = (ROOT / 'src' / 'dkn_adapter.inc').read_text(encoding='utf-8')
    errors = source_contracts(src)
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as td:  # FIX09: a Windows lock on a just-exited lab exe never fails the suite (detection unchanged)
        d = Path(td)
        r = run_variant(src, d, 'base')
        print(r.stdout.strip()[-800:])
        if r.returncode or 'DKN_FIX09_ADAPTER_PASS' not in r.stdout:
            errors.append('production adapter run failed: ' + (r.stdout + r.stderr)[-1500:])
        detected = 0
        for name, old, new in MUTANTS:
            if src.count(old) != 1:
                errors.append('mutant anchor not unique: ' + name)
                continue
            m = src.replace(old, new, 1)
            if source_contracts(m):
                detected += 1
                print(name + ': DETECTED (source contract)')
                continue
            rr = run_variant(m, d, 'm_' + name)
            if rr.returncode and 'FAIL line=' in rr.stdout:
                detected += 1
                print(name + ': DETECTED ' + rr.stdout.strip().splitlines()[-1][:160])
            else:
                errors.append('mutant survived: ' + name)
    out = {'status': 'DKN_FIX09_ADAPTER_PASS' if not errors else 'DKN_FIX09_ADAPTER_FAIL', 'mutants_detected': detected, 'mutants_total': len(MUTANTS), 'errors': errors,
           'scope': 'production DknSlot + writers + crossing filter / native evasion POST / i-frame window / maneuver start-end / hit-end / AT barrier / move-seq end / '
                    'post-hit decision / front restore with doubles; no game run'}
    print(json.dumps(out))
    return 0 if not errors else 1


if __name__ == '__main__':
    raise SystemExit(main())
