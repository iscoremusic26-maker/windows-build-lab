// LNK SESSION01 0.10.0 laboratory (LAB_MODEL_ONLY): LNK02 direct hits, LNK03 parries, LNK04 life/guard recovery, LNK07 variants, regressions.
// The tick model runs the REAL post08::Parry / ArmTrack / LinkTrack and the REAL lnk01::Posture / VariantTrack, on a host model taken from the source:
//  * collision pass: only ARMED At spheres are registered/cleared (c_cc_s.cpp); a hit writes the attacker's sphere into the Tg record of ONE Link
//    cylinder (each attacker hits a configured cylinder; same cylinder = last writer wins) and, if Link shields that side, the AtShieldHit mark;
//  * Link execute (daAlink_c::checkDamageAction, d_a_alink_damage.inc:505-780): mDamageTimer != 0 => no damage (and it counts down); otherwise the
//    FIRST cylinder with a Tg hit decides: shield => guard (GUARD_SLIP, every 4th consecutive block GUARD_BREAK); PROC_GUARD_SLIP => no damage;
//    else setDamagePoint(atp): ItemLifeCount -= atp (magic armour: rupees instead), mDamageTimer = invincible time;
//  * dMeter2_c::moveLife applies ItemLifeCount to the life (meter runs before or after the actors, both orders tested);
//  * Expansion shield chip: a BLOCKED Black fury blow costs 1 life unit through ItemLifeCount, without mDamageTimer.
// Nothing receives a precomputed "hit=true": every contact, damage and life change comes from the modelled host.
#include "../src/lnk_core.hpp"
#include "../src/parry_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using post08::Result;
using lnk01::policy::Variant;
static unsigned checks = 0, failures = 0;
static unsigned fam_checks[13] = {}, fam_fail[13] = {};
static int fam = 0;
static void ck(bool b, const char* what = "") {
  ++checks;
  ++fam_checks[fam];
  if (!b) {
    ++failures;
    ++fam_fail[fam];
    if (fam_fail[fam] <= 4) std::printf("FAIL S%02d %s\n", fam, what);
  }
}

enum LinkProc { kIdle = 0, kGuard = 1, kGuardAttack = 2, kGuardSlip = 3, kGuardBreak = 4, kDamaged = 5, kAttacking = 6 };
struct Foe {
  std::uint32_t id = 100;
  std::uint64_t gen = 1;
  bool e_oc2 = false;
  int atp = 1;
  int fury_from = -1, fury_to = -1;  // Expansion CP10.6: atp 3 -> 6 in this tick window (E_OC2 atp 3 only)
  int alone_from = -1;               // legacy Blue: atp 1 before this tick, 2 (group) after (Expansion CP10.6)
  bool front = true, reach = true;
  int cyl = 0;  // Link Tg cylinder this blade reaches
  int state = 0;
  float frame = 0, rate = 1.0f;
  int find_ticks = 0, first_delay = 0;
  bool hit[2] = {false, false}, shield[2] = {false, false};
  bool overlap_used = false, armed_this_tick = false;
  int recycle_at = -1;  // the slot is taken by a new generation at this tick
};
struct SimParams {
  int ticks = 40;
  bool link_first = true, meter_first = false;
  bool hold_guard = true, attack_stance = false;  // attack_stance: guard input held but Link is swinging (no real guard)
  bool both_spheres = false;
  std::vector<int> sa_ticks;
  std::vector<Foe> foes = {Foe{}};
  int life_max_raw = 100;  // getMaxLife(): hearts * 5
  int life_start = -1;     // quarter-heart units; -1 = full
  int invincible_until = 0;
  bool magic_armour = false;
  std::vector<int> ambient_damage_ticks;  // lava/fall-like setDamagePoint with no enemy
  int stray_object_tick = -1;             // a non-tracked object (arrow) hits cylinder 0 in this pass
  int pause_from = -1, pause_to = -1, event_from = -1, event_to = -1, link_change_at = -1;
  bool wolf = false;
  bool attach = true;
};
struct SimOut {
  int native_blocks = 0, native_damage = 0, guard_breaks = 0, parries = 0, chips = 0, life_end = 0, peak = 0, final_milli = 0;
  std::vector<int> posture, life;
  lnk01::PCounters pc;
  post08::PCounters parry_c;
  std::vector<lnk01::DirectEvent> direct;
  std::vector<lnk01::RegenEvent> regen;
};
static SimOut run(const SimParams& sp) {
  SimOut out;
  post08::Parry p;
  p.enabled = true;
  post08::LinkTrack track;
  post08::ArmTrack arm;
  lnk01::Posture lp;
  std::vector<Foe> foes = sp.foes;
  for (Foe& f : foes) f.find_ticks = f.first_delay, f.state = f.first_delay ? 9 : 0;
  const int life_max = (sp.life_max_raw / 5) * 4;
  int life = sp.life_start < 0 ? life_max : sp.life_start;
  float item_life = 0;
  int timer = sp.invincible_until, slip = 0, guard_counter = 0, proc = sp.attack_stance ? kAttacking : (sp.hold_guard ? kGuard : kIdle);
  float lframe = 0;
  std::uintptr_t tg[3] = {0, 0, 0};  // Link Tg records (attacker sphere keys) of the last pass
  bool tg_shield[3] = {false, false, false};
  std::uint32_t link_id = 7;
  auto paused = [&](int t) { return t >= sp.pause_from && t < sp.pause_to; };
  auto event = [&](int t) { return t >= sp.event_from && t < sp.event_to; };
  auto sphere_key = [&](size_t i, int s) { return static_cast<std::uintptr_t>(0x1000 + i * 16 + static_cast<size_t>(s)); };
  auto rest = [&] { return sp.attack_stance ? kAttacking : (sp.hold_guard ? kGuard : kIdle); };
  auto set_damage = [&](int dmg) {  // setDamagePoint
    if (dmg <= 0) return;
    if (!sp.magic_armour) item_life -= static_cast<float>(dmg);
    timer = 30;
    ++out.native_damage;
  };
  auto meter = [&](int t) {
    if (paused(t) || item_life == 0.0f) return;
    life = std::max(0, std::min(life_max, life + static_cast<int>(item_life)));
    item_life = 0;
  };
  auto link_exec = [&](int t) {
    if (paused(t)) return;
    for (int at : sp.ambient_damage_ticks)
      if (at == t && timer == 0) set_damage(2);
    int first = -1;
    for (int k = 0; k < 3 && first < 0; ++k)
      if (tg[k]) first = k;
    if (timer != 0) {
      --timer;  // damageTimerCount; invulnerable: the Tg hit is ignored
    } else if (first >= 0) {
      if (tg_shield[first]) {
        ++out.native_blocks;
        if (++guard_counter == 4) guard_counter = 0, proc = kGuardBreak, slip = 20, ++out.guard_breaks;
        else proc = kGuardSlip, slip = 8;
      } else if (proc != kGuardSlip) {
        int dmg = 2;
        for (size_t i = 0; i < foes.size(); ++i)
          for (int s = 0; s < 2; ++s)
            if (tg[first] == sphere_key(i, s)) dmg = foes[i].atp;
        set_damage(dmg);
        proc = kDamaged, slip = 10;
      }
    }
    if (proc == kGuardSlip || proc == kDamaged || proc == kGuardBreak) {
      if (--slip <= 0) proc = rest();
    } else if (std::find(sp.sa_ticks.begin(), sp.sa_ticks.end(), t) != sp.sa_ticks.end() && (proc == kIdle || proc == kGuard)) {
      proc = kGuardAttack, lframe = 1.0f;
    } else if (proc == kGuardAttack) {
      lframe += 1.0f;
      if (lframe > 18.0f) proc = rest();
    }
  };
  auto view_of = [&] {
    post08::LinkView v;
    v.valid = true, v.proc = proc == kGuardAttack ? 0x1e : (proc == kGuardSlip ? 0x1d : (proc == kGuardBreak ? 0x1f : proc));
    v.guard_attack = proc == kGuardAttack, v.frame = lframe, v.start = 1, v.end = 12;
    return v;
  };
  auto foe_exec = [&](size_t i, int t) {
    if (paused(t)) return;
    Foe& f = foes[i];
    if (t == f.recycle_at) f.gen += 1, f.id += 50, f.state = 0, f.hit[0] = f.hit[1] = false;
    // Expansion CP10.6 power for THIS tick (written in its execute POST = before the pass, read by our next PRE)
    if (f.e_oc2 && (f.atp == 3 || f.atp == 6)) f.atp = (t >= f.fury_from && t < f.fury_to) ? 6 : 3;
    if (f.alone_from >= 0) f.atp = t < f.alone_from ? 1 : 2;
    const int slot = static_cast<int>(i);
    post08::Input in;
    in.tracked = true, in.slot = slot, in.generation = f.gen, in.id = f.id, in.hp = 50;
    in.mode = (f.state == 0 || f.state == 1 || f.state == 3) ? 4 : 3;
    in.state = f.state == 9 ? 4 : f.state;
    in.anim_ok = true, in.anim_frame = f.frame, in.anim_rate = f.rate, in.e_oc2 = f.e_oc2, in.atp = f.atp;
    for (int s = 0; s < 2; ++s) in.at_hit[s] = in.at_on_link[s] = f.hit[s], in.at_shield[s] = f.hit[s] && f.shield[s];
    in.link_ok = true;
    in.now = view_of();
    in.pass = track.pass_for(static_cast<std::uint64_t>(t), true, link_id);
    in.fresh = arm.fresh(slot, f.id, f.gen, static_cast<std::uint64_t>(t));
    in.event_running = event(t);
    const post08::Verdict v = p.judge(in);
    // lnk_pre mirror: variant history + Tg attribution (first Link cylinder with a hit)
    if (sp.attach) lp.variants.observe(slot, f.id, f.gen, f.e_oc2, f.atp);
    bool tg_match = false;
    for (int k = 0; k < 3; ++k)
      if (tg[k]) {
        tg_match = tg[k] == sphere_key(i, 0) || tg[k] == sphere_key(i, 1);
        break;
      }
    if (v.result == Result::Confirmed) ++out.parries, p.commit(slot, f.gen, f.id, v.token, true);
    if (sp.attach && v.result != Result::None) {
      lnk01::Contact k;
      k.result = v.result, k.blocked = v.blocked, k.token = v.token, k.slot = slot, k.id = f.id, k.generation = f.gen;
      const post08::Parry::Episode& e = p.episode(static_cast<unsigned>(slot));
      k.episode_parried = e.bound && e.id == f.id && e.generation == f.gen && e.token == v.token && e.parried;
      k.e_oc2 = f.e_oc2, k.atp = f.atp, k.link_ok = true, k.link_human = !sp.wolf, k.link_id = link_id, k.event_running = in.event_running;
      k.tg_match = tg_match, k.tick = static_cast<std::uint64_t>(t);
      lp.on_contact(k);
    }
    arm.note_post(slot, f.id, f.gen, static_cast<std::uint64_t>(t), post08::ArmTrack::armed_now(in.mode == 4 && f.state == 1 ? 4 : 0, f.state == 1 ? 1 : 0, true, f.frame));
    f.armed_this_tick = false;
    const bool link_guard = proc == kGuard || proc == kGuardSlip;
    switch (f.state) {
      case 0: f.state = 1, f.frame = 0, f.rate = 1.0f, f.overlap_used = false; break;
      case 1:
        if (f.frame >= 14.0f && f.frame <= 22.0f) f.armed_this_tick = true;
        if ((f.hit[0] || f.hit[1]) && link_guard && f.front) {
          f.rate = -1.0f, f.state = 3;
          break;
        }
        f.frame += f.rate;
        if (f.frame >= 30.0f) f.state = 9, f.find_ticks = 8;
        break;
      case 3:
        f.frame += f.rate;
        if (f.frame <= 0.0f || f.frame >= 30.0f) f.state = 9, f.find_ticks = 8, f.rate = 1.0f;
        break;
      case 9:
        if (--f.find_ticks <= 0) f.state = 0;
        break;
    }
  };
  for (int t = 1; t <= sp.ticks; ++t) {
    if (t == sp.link_change_at) link_id = 8;
    // ---- mod_update ----
    track.observe(static_cast<std::uint64_t>(t), true, link_id, view_of().proc, proc == kGuardAttack, lframe, 1, 12);
    if (sp.attach) {
      lnk01::LinkObs ob;
      ob.valid = true, ob.human = !sp.wolf, ob.id = link_id, ob.life = life, ob.life_max = life_max, ob.damage_timer = timer, ob.pending_life = item_life;
      ob.active = !paused(t) && !event(t) && life > 0;
      ob.guard_safe = ob.active && ob.human && proc == kGuard && timer == 0;  // checkUpperGuardAnime replica minus excluded procs
      lp.observe_link(true, link_id);
      lnk01::DirectEvents de;
      lp.confirm_direct(ob, static_cast<std::uint64_t>(t), de);
      for (unsigned k = 0; k < de.n; ++k) out.direct.push_back(de.e[k]);
      out.regen.push_back(lp.tick(ob.active, lnk01::life_permille(life, life_max), ob.guard_safe));
    }
    // ---- actors (and the meter, before or after them) ----
    if (sp.meter_first) meter(t);
    if (sp.link_first) link_exec(t);
    for (size_t i = 0; i < foes.size(); ++i) foe_exec(i, t);
    if (!sp.link_first) link_exec(t);
    if (!sp.meter_first) meter(t);
    // ---- collision pass ----
    if (!paused(t)) {
      for (int k = 0; k < 3; ++k) tg[k] = 0, tg_shield[k] = false;  // Link's Tg cylinders are registered every pass
      if (t == sp.stray_object_tick) tg[0] = 0xBEEF;
      for (size_t i = 0; i < foes.size(); ++i) {
        Foe& f = foes[i];
        if (!f.armed_this_tick) continue;
        f.hit[0] = f.hit[1] = f.shield[0] = f.shield[1] = false;
        if (!f.overlap_used && f.frame >= 15.0f && f.reach) {
          f.overlap_used = true;
          const bool guarded = (proc == kGuardAttack || proc == kGuard || proc == kGuardSlip) && f.front;
          for (int s = 0; s < (sp.both_spheres ? 2 : 1); ++s) f.hit[s] = true, f.shield[s] = guarded;
          tg[f.cyl] = sphere_key(i, 0), tg_shield[f.cyl] = guarded;
          if (guarded && f.e_oc2 && f.atp == 6) item_life -= 1.0f, ++out.chips;  // Expansion CP19 FIX02 shield chip (no mDamageTimer)
        }
      }
    }
    out.posture.push_back(lp.milli());
    out.life.push_back(life);
  }
  out.pc = lp.c;
  out.parry_c = p.c;
  out.final_milli = lp.milli();
  out.life_end = life;
  for (int v : out.posture) out.peak = std::max(out.peak, v);
  return out;
}
static int V(const SimOut& o, lnk01::Verdict v) { return static_cast<int>(o.pc.verdict[static_cast<unsigned>(v)]); }
static int D(const SimOut& o, lnk01::DirectResult r) { return static_cast<int>(o.pc.direct_result[static_cast<unsigned>(r)]); }
static Foe foe(std::uint32_t id, bool e_oc2, int atp, int delay = 0, int cyl = 0) {
  Foe f;
  f.id = id, f.e_oc2 = e_oc2, f.atp = atp, f.first_delay = delay, f.cyl = cyl;
  return f;
}
// regeneration measurement: ticks to drain `pts` points from a single block at the given life/guard
static int drain_ticks(int life_permille_target, bool guard, int pts) {
  lnk01::Posture p;
  p.observe_link(true, 7);
  lnk01::Contact k;
  k.result = Result::NormalGuard, k.blocked = true, k.slot = 0, k.id = 9, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
  int n = 0;
  for (int g = 0; g < pts; g += 20) k.token = static_cast<std::uint64_t>(++n), p.on_contact(k);  // Blue block = 20 each
  for (int t = 0; t < 30; ++t) p.tick(true, life_permille_target, guard);
  int ticks = 0;
  while (p.milli() > 0 && ticks < 100000) p.tick(true, life_permille_target, guard), ++ticks;
  return ticks;
}

int main() {
  // ================= S01 LNK02 direct hits: confirmation from native evidence ================================================================
  fam = 1;
  {
    SimParams sp;
    sp.hold_guard = false;
    const SimOut o = run(sp);
    ck(o.native_damage == 1 && o.life_end == 80 - 1, "control: native damage 1 (atp 1), native life loss");
    ck(D(o, lnk01::DirectResult::Confirmed) == 1 && o.peak == 30000, "Blue direct hit: confirmed once, +30");
    ck(o.direct.size() == 1 && o.direct[0].confirm_tick == o.direct[0].contact_tick + 1, "confirmed on the mod_update after the contact");
    for (bool lf : {true, false})
      for (bool mf : {true, false}) {
        SimParams q = sp;
        q.link_first = lf, q.meter_first = mf;
        const SimOut x = run(q);
        ck(D(x, lnk01::DirectResult::Confirmed) == 1 && x.peak == 30000, "same result in every actor/meter order");
      }
    SimParams both = sp;
    both.both_spheres = true;
    const SimOut b = run(both);
    ck(D(b, lnk01::DirectResult::Confirmed) == 1 && b.peak == 30000, "two spheres of one blade: one direct credit");
    SimParams miss = sp;
    miss.foes[0].reach = false;
    const SimOut m = run(miss);
    ck(m.native_damage == 0 && m.peak == 0 && m.pc.contacts == 0, "attack that never reaches Link: nothing");
    SimParams back;
    back.foes[0].front = false;  // guarding, struck from behind: a genuine direct hit
    const SimOut bk = run(back);
    ck(bk.native_damage == 1 && D(bk, lnk01::DirectResult::Confirmed) == 1 && bk.peak == 30000, "struck from behind while guarding: direct hit +30");
    SimParams inv = sp;
    inv.invincible_until = 200;
    const SimOut iv = run(inv);
    ck(iv.native_damage == 0 && iv.life_end == 80 && iv.peak == 0 && D(iv, lnk01::DirectResult::NoNativeDamage) == 1, "invulnerable Link: no damage, no posture");
    SimParams arm = sp;
    arm.magic_armour = true;
    const SimOut ar = run(arm);
    ck(ar.native_damage == 1 && ar.life_end == 80 && ar.peak == 0 && D(ar, lnk01::DirectResult::NoLifeLoss) == 1, "native damage without life loss: no posture");
    SimParams amb;
    amb.ticks = 60, amb.ambient_damage_ticks = {10, 50};
    amb.foes[0].reach = false;
    const SimOut a = run(amb);
    ck(a.native_damage == 2 && a.peak == 0 && a.pc.damage_edges_unattributed == 2, "ambient damage without an enemy contact: never credited");
    SimParams stray = sp;
    stray.foes[0].cyl = 1, stray.stray_object_tick = 16;  // the blade lands on cylinder 1 in the pass where an arrow hit cylinder 0
    const SimOut st = run(stray);
    ck(st.peak == 0 && V(st, lnk01::Verdict::RejectedDirectNotAttributed) == 1, "life loss caused by another object: not attributed to the Bokoblin");
    SimParams wolf = sp;
    wolf.wolf = true;
    ck(run(wolf).peak == 0, "wolf: no human posture");
    SimParams ev = sp;
    ev.event_from = 1, ev.event_to = 40;
    ck(run(ev).peak == 0, "event: nothing");
  }

  // ================= S02 LNK02 attribution between several attackers ======================================================================
  fam = 2;
  {
    SimParams sp;
    sp.hold_guard = false;
    sp.foes = {foe(100, false, 1, 0, 0), foe(101, true, 2, 0, 1)};  // same pass: Blue on cylinder 0, Red on cylinder 1
    const SimOut o = run(sp);
    ck(o.native_damage == 1, "control: one native damage (first cylinder decides)");
    ck(D(o, lnk01::DirectResult::Confirmed) == 1 && o.pc.direct_by_variant[0] == 1 && o.peak == 30000, "only the attacker of the first cylinder (Blue) is credited");
    ck(V(o, lnk01::Verdict::RejectedDirectNotAttributed) == 1, "the other contact is not attributed");
    SimParams same = sp;
    same.foes[1].cyl = 0;  // both on cylinder 0: the last writer (Red) is the hit Link takes
    const SimOut s = run(same);
    ck(D(s, lnk01::DirectResult::Confirmed) == 1 && s.pc.direct_by_variant[1] == 1 && s.peak == 35000, "same cylinder: the native record decides (Red +35)");
    SimParams apart = sp;
    apart.ticks = 100;
    apart.foes[1].first_delay = 36;  // after Link's 30-tick invulnerability from the Blue hit: both genuine
    const SimOut ap = run(apart);
    ck(ap.native_damage >= 2 && D(ap, lnk01::DirectResult::Confirmed) == ap.native_damage && ap.pc.direct_by_variant[0] >= 1 && ap.pc.direct_by_variant[1] >= 1,
       "genuine hits at different moments: one credit per native damage, each to its own attacker");
    SimParams inv = apart;  // the second blow lands while Link is still invulnerable from the first
    inv.foes[1].first_delay = 4;
    const SimOut iv = run(inv);
    ck(D(iv, lnk01::DirectResult::Confirmed) == iv.native_damage && D(iv, lnk01::DirectResult::NoNativeDamage) >= 1, "blows during invulnerability: no credit");
    SimParams blk;
    blk.foes = {foe(100, false, 1, 0, 0), foe(101, true, 2, 0, 1)};
    blk.foes[1].front = false;  // Blue blocked in front, Red hits from behind in the same pass (cylinder 0 = shield: Link guards, no damage)
    const SimOut bb = run(blk);
    ck(V(bb, lnk01::Verdict::Confirmed) == 1 && D(bb, lnk01::DirectResult::Confirmed) == 0 && bb.native_damage == 0,
       "block on the first cylinder: native guard, the other contact is never a direct hit");
    {  // slot recycled by a NEW identity before the confirmation: the pending hit is superseded, never credited to the newcomer
      lnk01::Posture p;
      p.observe_link(true, 7);
      lnk01::Contact k;
      k.result = Result::NotBlocked, k.blocked = false, k.tg_match = true, k.slot = 5, k.id = 700, k.generation = 1, k.token = 1, k.tick = 10;
      k.link_ok = true, k.link_human = true, k.link_id = 7;
      ck(p.on_contact(k).v == lnk01::Verdict::DirectPending, "pending");
      lnk01::Contact n = k;
      n.id = 701, n.generation = 2, n.token = 2, n.result = Result::NotAttacking;
      p.on_contact(n);
      lnk01::LinkObs ob;
      ob.valid = true, ob.human = true, ob.id = 7, ob.life = 70, ob.life_max = 80, ob.damage_timer = 30, ob.active = true;
      lnk01::DirectEvents de;
      p.confirm_direct(ob, 11, de);
      ck(p.milli() == 0 && p.c.direct_result[static_cast<unsigned>(lnk01::DirectResult::Superseded)] == 1 && p.pending() == 0, "recycled slot: superseded, no credit");
      ck(p.c.damage_edges_unattributed == 1, "the life loss stays unattributed");
    }
    {  // two pending candidates from the same tick and ONE native damage: conservative, neither is credited
      lnk01::Posture p;
      p.observe_link(true, 7);
      lnk01::Contact k;
      k.result = Result::NotBlocked, k.blocked = false, k.tg_match = true, k.link_ok = true, k.link_human = true, k.link_id = 7, k.tick = 20, k.generation = 1;
      k.slot = 1, k.id = 801, k.token = 1;
      p.on_contact(k);
      k.slot = 2, k.id = 802, k.token = 2;
      p.on_contact(k);
      lnk01::LinkObs ob;
      ob.valid = true, ob.human = true, ob.id = 7, ob.life = 80, ob.life_max = 80, ob.active = true;
      lnk01::DirectEvents de;
      p.confirm_direct(ob, 20, de);
      ob.life = 77, ob.damage_timer = 30;
      p.confirm_direct(ob, 21, de);
      ck(p.milli() == 0 && p.c.direct_result[static_cast<unsigned>(lnk01::DirectResult::Ambiguous)] == 2, "ambiguous attribution: no credit to either");
    }
    SimParams lc = sp;
    lc.link_change_at = 18;  // Link replaced between contact (17) and confirmation (18)
    const SimOut l = run(lc);
    ck(l.final_milli == 0 && D(l, lnk01::DirectResult::Confirmed) == 0, "Link replaced before confirmation: pending dropped, reset");
  }

  // ================= S03 LNK03 parries: +40 enemy (POST08), +0 Link, exclusive outcome ===================================================
  fam = 3;
  {
    int parries = 0, blocks = 0, directs = 0;
    for (int s = 1; s <= 18; ++s)
      for (bool guard : {true, false}) {
        SimParams sp;
        sp.sa_ticks = {s};
        sp.hold_guard = guard;
        const SimOut x = run(sp);
        const int outcomes = x.parries + V(x, lnk01::Verdict::Confirmed) + D(x, lnk01::DirectResult::Confirmed);
        ck(outcomes == 1, "one blow: parry XOR block XOR direct hit");
        if (x.parries) ck(x.peak == 0 && x.native_damage == 0 && x.pc.parries_excluded >= 1, "parry: +0 Link, no damage");
        parries += x.parries, blocks += V(x, lnk01::Verdict::Confirmed), directs += D(x, lnk01::DirectResult::Confirmed);
      }
    ck(parries >= 10 && blocks >= 5 && directs >= 1, "the sweep produces all three outcomes");
    SimParams cons;
    cons.ticks = 400, cons.sa_ticks = {13, 53, 93, 133, 173, 213};
    const SimOut c = run(cons);
    ck(c.parries >= 4 && c.peak <= 20000 * (static_cast<int>(c.pc.verdict[1])), "consecutive parries add nothing to Link");
    // parry with Link already saturated: still +0, POST08 unaffected
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 0, k.id = 9, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
    for (int i = 1; i <= 6; ++i) k.token = static_cast<std::uint64_t>(i), p.on_contact(k);
    ck(p.milli() == 100000, "saturated");
    k.token = 50, k.result = Result::Confirmed;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedParry && p.milli() == 100000, "parry at 100: +0");
    k.result = Result::NotBlocked, k.blocked = false, k.tg_match = true;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedParry, "a later contact of a parried attack is never a direct hit");
    // other attacker during a parry
    SimParams two;
    two.ticks = 40, two.sa_ticks = {13};
    two.foes = {foe(100, false, 1, 0, 0), foe(101, true, 2, 1, 1)};
    const SimOut t = run(two);
    ck(t.parries >= 1, "parry to one attacker in a group");
    ck(t.pc.contacts >= 2, "the other attacker's contact is judged on its own");
    // Red parry, Black fury excluded from parry (FURY_CHIP_POSSIBLE is a block with the fury weight)
    SimParams red;
    red.sa_ticks = {13};
    red.foes = {foe(101, true, 2)};
    ck(run(red).parries == 1 && run(red).peak == 0, "Red parry: +0 Link");
    lnk01::Posture q;
    q.observe_link(true, 7);
    k = lnk01::Contact{};
    k.result = Result::FuryChipPossible, k.blocked = true, k.slot = 1, k.id = 77, k.generation = 1, k.e_oc2 = true, k.atp = 6, k.link_ok = true, k.link_human = true;
    k.link_id = 7, k.token = 3;
    const lnk01::Outcome fo = q.on_contact(k);
    ck(fo.v == lnk01::Verdict::Confirmed && fo.weight == 35 && q.c.fury_credits == 1, "Black fury in the Shield Attack window: no parry, fury block +35");
  }

  // ================= S04 LNK04 recovery by life band ========================================================================================
  fam = 4;
  {
    ck(drain_ticks(1000, false, 20) == 60, "100 % life: 10 pts/s (20 pts in 60 ticks, = LNK01)");
    ck(drain_ticks(750, false, 20) == 60, "75 %: 10 pts/s");
    ck(drain_ticks(749, false, 20) == 75, "74.9 %: 8 pts/s (75 ticks)");
    ck(drain_ticks(500, false, 20) == 75, "50 %: 8 pts/s");
    ck(drain_ticks(499, false, 20) == 120, "49.9 %: 5 pts/s (120 ticks)");
    ck(drain_ticks(250, false, 20) == 120, "25 %: 5 pts/s");
    ck(drain_ticks(100, false, 20) == 300, "10 %: 2 pts/s (300 ticks)");
    ck(drain_ticks(-1, false, 20) == 300, "unknown max life: slowest band (conservative)");
    bool mono = true;
    int last = -1;
    for (int pm = 0; pm <= 1000; ++pm) {
      const int r = lnk01::Posture::rate_milli_per_s(lnk01::regen_band(pm), false);
      if (r < last) mono = false;
      last = r;
    }
    ck(mono, "recovery never increases when life decreases");
    // variable maximum: same ratio, same band, whatever the number of hearts
    for (int hearts : {3, 5, 10, 13, 20}) {
      const int mx = (hearts * 5 / 5) * 4;
      ck(lnk01::regen_band(lnk01::life_permille(mx, mx)) == 0 && lnk01::regen_band(lnk01::life_permille(mx / 2, mx)) == 1 &&
             lnk01::regen_band(lnk01::life_permille(mx / 8, mx)) == 3,
         "band from the real ratio, any max life");
    }
    // model run: Link at 20 % life recovers slower than at full life after the same direct hit
    SimParams full, low;
    full.ticks = low.ticks = 140;
    full.hold_guard = low.hold_guard = false;
    low.life_start = 16;  // 20 % of 80
    const SimOut f = run(full), l = run(low);
    ck(f.peak == l.peak && l.posture[130] > f.posture[130], "low life: slower recovery in the model (same hits)");
  }

  // ================= S05 LNK04 guard bonus =====================================================================================================
  fam = 5;
  {
    ck(drain_ticks(1000, true, 20) == 40, "safe guard at full life: x1.5 = 15 pts/s (40 ticks)");
    ck(drain_ticks(100, true, 20) == 200, "safe guard at low life: 2 x 1.5 = 3 pts/s (200 ticks)");
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 0, k.id = 9, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7, k.token = 1;
    p.on_contact(k);
    for (int t = 0; t < 30; ++t) p.tick(true, 1000, true);
    ck(p.milli() == 20000, "guard does not shorten the 30-tick delay");
    // a continuous series of blocked hits: the delay restarts every time, no recovery at all
    lnk01::Posture z;
    z.observe_link(true, 7);
    for (int i = 2; i < 12; ++i) {
      k.token = static_cast<std::uint64_t>(i), z.on_contact(k);
      for (int t = 0; t < 20; ++t) z.tick(true, 1000, true);
    }
    ck(z.c.regen_ticks == 0, "blocks every 20 ticks: never recovers (guarded or not)");
    // model: holding a real guard vs a swing with the guard input held (no valid guard)
    SimParams g;
    g.ticks = 120;
    SimParams s = g;
    s.attack_stance = true;  // guard input but Link is attacking: modelled as no upper guard
    s.foes[0].front = false;
    g.foes[0].reach = true;
    const SimOut go = run(g), so = run(s);
    ck(go.pc.regen_guard_ticks > 0, "real guard: guard-rate ticks");
    ck(so.pc.regen_guard_ticks == 0 && so.pc.regen_ticks > 0, "attack stance: neutral rate only");
    // direct hit during recovery restarts the delay
    lnk01::Posture d;
    d.observe_link(true, 7);
    k.token = 99, d.on_contact(k);
    for (int t = 0; t < 45; ++t) d.tick(true, 1000, false);
    const int mid = d.milli();
    lnk01::Contact h = k;
    h.result = Result::NotBlocked, h.blocked = false, h.tg_match = true, h.token = 100, h.tick = 500;
    ck(d.on_contact(h).v == lnk01::Verdict::DirectPending, "direct pending");
    lnk01::LinkObs ob;
    ob.valid = true, ob.human = true, ob.id = 7, ob.life = 70, ob.life_max = 80, ob.damage_timer = 30, ob.active = true;
    lnk01::DirectEvents de;
    lnk01::Posture& dr = d;
    lnk01::LinkObs before = ob;
    before.damage_timer = 0, before.life = 80;
    dr.confirm_direct(before, 500, de);  // reference of the contact tick
    dr.confirm_direct(ob, 501, de);
    ck(de.n == 1 && de.e[0].r == lnk01::DirectResult::Confirmed && d.milli() == mid + 30000, "confirmed direct hit during recovery: +30");
    for (int t = 0; t < 30; ++t) d.tick(true, 1000, false);
    ck(d.milli() == mid + 30000, "and the delay restarted");
  }

  // ================= S06 LNK04 special states and clock ====================================================================================
  fam = 6;
  {
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 0, k.id = 9, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7, k.token = 1;
    p.on_contact(k);
    for (int t = 0; t < 40; ++t) p.tick(true, 1000, false);
    const int v = p.milli();
    for (int t = 0; t < 50000; ++t) p.tick(false, 1000, true);
    ck(p.milli() == v, "pause / menu / cutscene / death / loading: frozen, guard ignored");
    p.tick(true, 1000, false);
    ck(v - p.milli() >= 333 && v - p.milli() <= 334, "one step after resuming (no burst)");
    for (int i = 0; i < 2000; ++i) p.tick(true, i % 7 * 140, i % 3 == 0);
    ck(p.milli() == 0 && p.conserved(), "exactly 0 after mixed bands/guard, never negative, conservation");
    lnk01::Posture s;
    s.observe_link(true, 7);
    for (int i = 1; i <= 10; ++i) {
      k.token = static_cast<std::uint64_t>(100 + i);
      s.on_contact(k);
    }
    ck(s.milli() == 100000, "never above 100");
    SimParams pause;
    pause.ticks = 200, pause.pause_from = 60, pause.pause_to = 150, pause.hold_guard = false;
    const SimOut po = run(pause);
    ck(po.posture[70] == po.posture[140] && po.life[70] == po.life[140], "model pause: posture and life frozen");
    SimParams ch;
    ch.ticks = 80, ch.link_change_at = 50;
    const SimOut c = run(ch);
    ck(c.posture[48] > 0 && c.posture[50] == 0, "room change / death-continue (new Link): reset to 0");
  }

  // ================= S07 LNK07 variant identification =======================================================================================
  fam = 7;
  {
    using lnk01::IdMethod;
    auto C = [](bool e2, int atp, std::uint8_t seen = 0) { return lnk01::classify_variant(e2, atp, seen); };
    ck(C(false, 1).v == Variant::Blue && C(false, 1).m == IdMethod::E_OC_Resource && !C(false, 1).group_power, "E_OC atp1: Blue");
    ck(C(false, 2).v == Variant::Blue && C(false, 2).group_power, "E_OC atp2: Blue with the group power (CP10.6)");
    ck(C(true, 2).v == Variant::Red && C(true, 2).m == IdMethod::E_OC2_Atp2Red, "E_OC2 atp2 never 1: Red");
    ck(C(true, 3).v == Variant::Black && C(true, 6).v == Variant::BlackFury, "E_OC2 atp3 Black, atp6 Black fury");
    ck(C(true, 1).v == Variant::Blue && C(true, 1).m == IdMethod::E_OC2_Atp1LegacyBlue, "E_OC2 atp1: legacy DEVF Blue");
    ck(C(true, 2, 1).v == Variant::Blue && C(true, 2, 1).group_power, "E_OC2 atp2 after atp1 in this generation: legacy Blue in a group, not Red");
    ck(C(true, -2, 4).v == Variant::Black && C(true, -2, 4).m == IdMethod::E_OC2_BlackHistory, "transient atp on a known Black: Black");
    ck(C(true, 5).v == Variant::Unknown && C(true, -2).v == Variant::Unknown && C(true, 0).v == Variant::Unknown, "unrecognised power: UNKNOWN");
    const auto wu = lnk01::weight_of(C(true, 5)), wb = lnk01::weight_of(C(true, 3));
    ck(wu.block == 20 && wu.direct == 30 && wu.block < wb.block, "UNKNOWN never gets Black weights");
    lnk01::VariantTrack vt;
    vt.observe(4, 300, 1, true, 1);
    ck(vt.classify(4, 300, 1, true, 2).v == Variant::Blue, "history keeps the legacy Blue");
    ck(vt.classify(4, 301, 2, true, 2).v == Variant::Red, "new generation in the same slot: history not inherited");
    vt.observe(4, 301, 2, true, 3);
    vt.observe(4, 301, 2, true, 6);
    ck(vt.classify(4, 301, 2, true, 3).v == Variant::Black && vt.classify(4, 301, 2, true, 6).v == Variant::BlackFury, "fury toggles with the power");
    ck(vt.classify(4, 301, 2, true, -2).v == Variant::Black, "the new generation builds its OWN history (transient power stays Black)");
    vt.observe(4, 302, 3, true, 2);
    ck(vt.classify(4, 302, 3, true, -2).v == Variant::Unknown && vt.classify(4, 302, 3, true, 2).v == Variant::Red, "a recycled slot never inherits Black");
    // model: fury window, blocked blows: base 30, fury 35; shield chip costs life but is never a direct hit
    SimParams fu;
    fu.ticks = 120;
    fu.foes = {foe(400, true, 3)};
    fu.foes[0].fury_from = 30, fu.foes[0].fury_to = 200;
    const SimOut f = run(fu);
    ck(f.pc.block_by_variant[2] >= 1 && f.pc.block_by_variant[3] >= 1, "blocks before (Black) and during fury (Black fury)");
    ck(f.chips >= 1 && D(f, lnk01::DirectResult::Confirmed) == 0 && f.pc.damage_edges == 0, "shield chip: life unit taken, no mDamageTimer, no direct credit");
    ck(f.pc.fury_credits == f.pc.block_by_variant[3], "fury credits counted");
    // legacy Blue alone then in group: classified Blue all along
    SimParams lb;
    lb.ticks = 120;
    lb.foes = {foe(500, true, 1)};
    lb.foes[0].alone_from = 50;
    const SimOut b = run(lb);
    ck(b.pc.block_by_variant[0] >= 2 && b.pc.block_by_variant[1] == 0 && b.pc.group_adjusted >= 1, "legacy Blue never misread as Red after joining a group");
  }

  // ================= S08 LNK07 weights per variant (block / direct) =========================================================================
  fam = 8;
  {
    struct Row {
      bool e2;
      int atp;
      int block, direct;
    };
    for (Row r : {Row{false, 1, 20, 30}, Row{true, 2, 25, 35}, Row{true, 3, 30, 40}, Row{true, 6, 35, 45}, Row{false, 2, 25, 35}}) {
      SimParams b;
      b.foes = {foe(600, r.e2, r.atp)};
      if (r.atp == 6) b.foes[0].fury_from = 0, b.foes[0].fury_to = 1000;
      const SimOut bo = run(b);
      ck(bo.peak == r.block * 1000, "block weight of the variant");
      SimParams d = b;
      d.hold_guard = false;
      const SimOut dd = run(d);
      ck(dd.peak == r.direct * 1000, "direct weight of the variant");
      ck(dd.life_end == 80 - r.atp, "native damage = the variant's own attack power (never changed by LNK)");
    }
    ck(lnk01::policy::kWeights[0].block < lnk01::policy::kWeights[1].block && lnk01::policy::kWeights[1].block < lnk01::policy::kWeights[2].block &&
           lnk01::policy::kWeights[2].block < lnk01::policy::kWeights[3].block,
       "block weights increase Blue < Red < Black < Black fury");
    for (unsigned i = 0; i < 5; ++i) ck(lnk01::policy::kWeights[i].direct > lnk01::policy::kWeights[i].block, "a direct hit always weighs more than a block");
  }

  // ================= S09 group combat: no contamination between attackers ===================================================================
  fam = 9;
  {
    SimParams g;
    g.ticks = 300;
    g.foes = {foe(100, false, 1, 0, 0), foe(101, true, 2, 7, 1), foe(102, true, 3, 13, 2), foe(103, true, 6, 21, 0)};
    for (Foe& f : g.foes) f.fury_from = (f.atp == 6) ? 0 : -1, f.fury_to = (f.atp == 6) ? 1000 : -1;
    g.foes[1].front = false;  // Red always from behind: direct hits
    const SimOut o = run(g);
    const auto R = [&](Result x) { return static_cast<int>(o.parry_c.result[static_cast<unsigned>(x)]); };
    ck(V(o, lnk01::Verdict::Confirmed) == R(Result::NormalGuard) + R(Result::MissedEarly) + R(Result::Latched) + R(Result::FuryChipPossible),
       "every POST08 block verdict credited exactly once (several blades on the shield in one pass all count)");
    ck(V(o, lnk01::Verdict::Confirmed) >= o.native_blocks, "at least one credit per Link guard reaction");
    ck(D(o, lnk01::DirectResult::Confirmed) <= o.native_damage, "never more direct credits than native damages");
    ck(o.pc.direct_by_variant[1] >= 1, "Red direct hits credited to Red");
    int sum = 0;
    for (unsigned i = 0; i < lnk01::kVerdictCount; ++i) sum += static_cast<int>(o.pc.verdict[i]);
    ck(sum == static_cast<int>(o.pc.contacts), "every contact has exactly one verdict");
    ck(o.final_milli >= 0 && o.final_milli <= 100000, "bounded");
    SimParams same;
    same.ticks = 200;
    same.foes = {foe(100, false, 1, 0, 0), foe(101, false, 1, 6, 1)};
    const SimOut s = run(same);
    ck(s.pc.block_by_variant[0] == s.parry_c.result[static_cast<unsigned>(Result::NormalGuard)], "two Blues: each block verdict once");
  }

  // ================= S10 regressions: native behaviour identical with and without LNK ======================================================
  fam = 10;
  {
    SimParams sp;
    sp.ticks = 900;
    sp.sa_ticks = {13, 113, 213};
    sp.foes = {foe(100, false, 1, 0, 0), foe(101, true, 2, 9, 1), foe(102, true, 3, 17, 2)};
    sp.foes[1].front = false;
    SimParams off = sp;
    off.attach = false;
    const SimOut a = run(sp), b = run(off);
    ck(a.life == b.life, "native life trace identical (LNK never writes life)");
    ck(a.native_damage == b.native_damage && a.native_blocks == b.native_blocks && a.guard_breaks == b.guard_breaks, "native damage/guard/guard break identical");
    ck(a.parries == b.parries && a.parry_c.contacts == b.parry_c.contacts && a.parry_c.stale == b.parry_c.stale && a.parry_c.confirmed == b.parry_c.confirmed,
       "POST08 verdict stream identical");
    ck(a.guard_breaks >= 1, "native guard break still happens");
  }

  // ================= S11 telemetry lines =====================================================================================================
  fam = 11;
  {
    char line[1024];
    lnk01::DirectEvent d;
    d.id = 4294967295u, d.generation = d.token = d.contact_tick = d.confirm_tick = 18446744073709551615ull, d.life_before = 65535, d.life_now = 65535;
    d.pending_life = -9999.0f, d.before = d.after = 100000, d.weight = 45, d.timer = 32767;
    const int n = lnk01::fmt_direct(line, sizeof line, d);
    ck(n > 0 && n < 1024, "worst-case LNK_S1_DIRECT fits its buffer");
    ck(std::strstr(line, "0x") == nullptr, "no address-like hex");
    lnk01::VariantId v = lnk01::classify_variant(true, 6, 0);
    const int m = lnk01::fmt_balance(line, sizeof line, v, true, 45, 1, 1);
    ck(m > 0 && m < 512 && std::strstr(line, "BLACK_FURY") != nullptr, "LNK_S1_BALANCE fits the log limit");
  }

  // ================= S12 performance / bounds ================================================================================================
  fam = 12;
  {
    ck(sizeof(lnk01::Posture) < 16384, "posture state static and bounded");
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.blocked = false, k.link_ok = true, k.link_human = true, k.link_id = 7, k.tg_match = true, k.generation = 1;
    lnk01::DirectEvents de;
    for (std::uint64_t i = 1; i <= 100000; ++i) {
      k.result = (i % 3) ? Result::NotBlocked : Result::NormalGuard, k.blocked = (i % 3) == 0;
      k.token = i, k.slot = static_cast<int>(i % 64), k.id = static_cast<std::uint32_t>(1000 + i % 64), k.tick = i;
      p.on_contact(k);
      lnk01::LinkObs ob;
      ob.valid = true, ob.human = true, ob.id = 7, ob.life = 50, ob.life_max = 80, ob.damage_timer = static_cast<int>(i % 2) * 30, ob.active = true;
      de.n = 0;
      p.confirm_direct(ob, i + 1, de);
      p.tick(true, 600, (i % 5) == 0);
    }
    ck(p.c.contacts == 100000 && p.conserved() && p.pending() <= 64, "100k mixed contacts: bounded pending, conservation");
  }

  for (int i = 1; i <= 12; ++i) std::printf("S%02d checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("LNK_S1_LAB checks=%u failures=%u RESULT=%s LAB_MODEL_ONLY\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
