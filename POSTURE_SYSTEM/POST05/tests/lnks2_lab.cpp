// LNK SESSION02 0.11.0 laboratory (LAB_MODEL_ONLY): LNK05 native guard break + LNK06 vulnerability/recovery.
// Part 1 drives the REAL lnk05::Machine and gate through every case of the brief. Part 2 runs a tick model of Link's native state machine taken from the
// source (d_a_alink_damage.inc checkDamageAction, d_a_alink_guard.inc procGuardSlipInit/procGuardBreakInit/procGuardBreak, mGuardBreakAnm {end 35,
// cancel 28}): the native 4th-block counter, GUARD_SLIP, invulnerability, damage interrupting the break, checkNextAction at cancel/end; with the REAL
// post08::Parry/ArmTrack/LinkTrack, lnk01::Posture and lnk05::Machine wired exactly as mod.cpp wires them (request on a confirmed saturating
// block/direct hit, trigger in the checkDamageAction POST, observe once per mod_update, reset after the observed native exit, regen held meanwhile).
// A passing model is NOT a runtime observation: HUMAN_RUNTIME_PENDING.
#include "../src/lnk_core.hpp"
#include "../src/lnks2_core.hpp"
#include "../src/parry_core.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using lnk05::Ev;
using lnk05::Gate;
using lnk05::Kind;
using post08::Result;
static unsigned checks = 0, failures = 0;
static unsigned fam_checks[10] = {}, fam_fail[10] = {};
static int fam = 0;
static void ck(bool b, const char* what = "") {
  ++checks;
  ++fam_checks[fam];
  if (!b) {
    ++failures;
    ++fam_fail[fam];
    if (fam_fail[fam] <= 6 || std::getenv("LNKS2_ALLFAIL")) std::printf("FAIL B%02d %s\n", fam, what);
  }
}
static lnk05::Episode ep(Kind k, std::uint64_t tick = 10, std::uint64_t token = 1, int before = 80000, int after = 100000) {
  lnk05::Episode e;
  e.link_id = 7, e.epoch = 1, e.tick = tick, e.kind = k, e.attacker = 100, e.generation = 1, e.token = token, e.before = before, e.after = after;
  return e;
}
static lnk05::GateIn gin(int proc, std::uint64_t tick = 11) {
  lnk05::GateIn g;
  g.valid = g.human = g.alive = g.sim_active = true, g.link_id = 7, g.epoch = 1, g.tick = tick, g.proc = proc;
  return g;
}
static lnk05::ObsIn obs(int proc, float frame, std::uint64_t tick, bool alive = true) {
  lnk05::ObsIn o;
  o.valid = true, o.alive = alive, o.sim_active = true, o.link_id = 7, o.epoch = 1, o.tick = tick, o.proc = proc, o.frame = frame;
  return o;
}

// ============================== Link + attackers tick model =================================================================================
enum P { kWait = 3, kGuardStance = 6, kStepMove = 0x1b, kSlip = 0x1d, kGuardAttack = 0x1e, kBreak = 0x1f, kDamage = 0x32, kRoll = 0x0e, kSwim = 0x8f };
struct Foe {
  std::uint32_t id = 100;
  std::uint64_t gen = 1;
  bool e_oc2 = false;
  int atp = 1, first_delay = 0, cyl = 0;
  bool front = true, reach = true;
  int state = 0;
  float frame = 0, rate = 1.0f;
  int find = 0;
  bool hit[2] = {false, false}, shield[2] = {false, false}, used = false, armed = false;
};
struct Sim {
  int ticks = 200;
  bool link_first = true, hold_guard = true, cancel_input = true;  // cancel_input: the player acts as soon as the native cancel frame allows
  std::vector<int> sa_ticks;
  std::vector<Foe> foes = {Foe{}};
  int start_posture_blocks = 0;  // Blue blocks pre-credited before the fight (fast path to 100)
  int pause_from = -1, pause_to = -1, event_from = -1, event_to = -1, death_at = -1, link_change_at = -1, wolf_from = -1, swim_from = -1;
  bool attach = true, lnk05_on = true;  // lnk05_on=false: the 0.10.1 posture (no LNK05/LNK06)
};
struct Out {
  int native_blocks = 0, native_damage = 0, life_end = 0, damage_in_break = 0;
  std::uint64_t native_breaks = 0, mod_inits = 0;
  int max_break_frames = 0, breaks_completed = 0;
  std::vector<int> posture, proc;
  lnk05::Counters bc;
  lnk01::PCounters pc;
  post08::PCounters parry;
  std::vector<lnk05::Out> events;
  bool posture_reset_while_breaking = false, regen_while_breaking = false, conserved = false, mod_break_over_damage = false;
};
static Out run(const Sim& sp) {
  Out out;
  post08::Parry p;
  p.enabled = true;
  post08::LinkTrack track;
  post08::ArmTrack arm;
  lnk01::Posture lp;
  lnk05::Machine bm;
  std::vector<Foe> foes = sp.foes;
  for (Foe& f : foes) f.find = f.first_delay, f.state = f.first_delay ? 9 : 0;
  const int life_max = 80;
  int life = life_max, timer = 0, slip = 0, dmg = 0, counter = 0, proc = sp.hold_guard ? kGuardStance : kWait;
  float lframe = 0, item_life = 0;
  std::uintptr_t tg[3] = {0, 0, 0};
  bool tg_shield[3] = {false, false, false};
  std::uint32_t link_id = 7;
  int break_frames = 0;
  auto paused = [&](int t) { return t >= sp.pause_from && t < sp.pause_to; };
  auto event = [&](int t) { return t >= sp.event_from && t < sp.event_to; };
  auto wolf = [&](int t) { return sp.wolf_from >= 0 && t >= sp.wolf_from; };
  auto swim = [&](int t) { return sp.swim_from >= 0 && t >= sp.swim_from; };
  auto key = [&](size_t i) { return static_cast<std::uintptr_t>(0x1000 + i * 16); };
  auto rest = [&] { return sp.hold_guard ? kGuardStance : kWait; };
  auto guard_break_init = [&](bool by_mod) {  // procGuardBreakInit: commonProcInit + ANM_GUARD_BREAK + vibration + voice
    proc = kBreak, lframe = 0;
    ++(by_mod ? out.mod_inits : out.native_breaks);
  };
  if (sp.attach) {
    lp.observe_link(true, link_id);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 63, k.id = 999, k.generation = 1, k.link_ok = k.link_human = true, k.link_id = link_id;
    for (int i = 0; i < sp.start_posture_blocks; ++i) k.token = 100000 + static_cast<std::uint64_t>(i), lp.on_contact(k);
  }
  auto request = [&](Kind kind, std::uint32_t id, std::uint64_t gen, std::uint64_t token, int before, int after, int t) {
    if (!sp.attach || !sp.lnk05_on || after < lnk01::policy::kMaxMilli) return;
    lnk05::Episode e;
    e.link_id = link_id, e.epoch = lp.epoch(), e.tick = static_cast<std::uint64_t>(t), e.kind = kind, e.attacker = id, e.generation = gen, e.token = token;
    e.before = before, e.after = after;
    out.events.push_back(bm.request(e));
    lp.hold(bm.hold());
  };
  auto link_exec = [&](int t) {
    if (paused(t) || life <= 0) return;
    // ---- checkDamageAction (native) ----
    const int timer_before = timer;
    int first = -1;
    for (int k = 0; k < 3 && first < 0; ++k)
      if (tg[k]) first = k;
    if (timer != 0) {
      --timer;
    } else if (first >= 0 && !event(t)) {
      if (tg_shield[first]) {
        ++out.native_blocks;
        if (proc != kSlip) {  // procGuardSlipInit
          if (++counter == 4) counter = 0, guard_break_init(false);
          else proc = kSlip, slip = 8, lframe = 0;
        }
      } else if (proc != kSlip) {
        int d = 1;
        for (size_t i = 0; i < foes.size(); ++i)
          if (tg[static_cast<size_t>(first)] == key(i)) d = foes[i].atp;
        item_life -= static_cast<float>(d), timer = 30, ++out.native_damage;
        if (proc == kBreak) ++out.damage_in_break;
        proc = kDamage, dmg = 12;
      }
    }
    // ---- LNK05: second POST of checkDamageAction (mirror of on_link_damage_post_lnk05) ----
    if (sp.attach && sp.lnk05_on && bm.state() == lnk05::State::TriggerPending) {
      lnk05::GateIn g;
      g.valid = true, g.link_id = link_id, g.epoch = lp.epoch(), g.tick = static_cast<std::uint64_t>(t), g.human = !wolf(t), g.alive = life > 0;
      g.sim_active = !paused(t), g.event = event(t), g.proc = proc, g.mode_flags = swim(t) ? 0x40000u : 0u, g.damage_this_call = timer > timer_before;
      if (bm.check(g) == Gate::Ok) {
        if (proc == kDamage || timer > timer_before) out.mod_break_over_damage = true;
        guard_break_init(true);
        out.events.push_back(bm.triggered(proc == kBreak, proc, static_cast<std::uint64_t>(t)));
      }
    }
    // ---- procedure (native) ----
    switch (proc) {
      case kSlip:
        if (--slip <= 0) proc = rest();
        break;
      case kDamage:
        if (--dmg <= 0) proc = rest();
        break;
      case kBreak:  // procGuardBreak: end => checkNextAction(0); frame > cancel => checkNextAction(1) (the player acts)
        lframe += 1.0f, ++break_frames;
        if (lframe >= lnk05::policy::kEndFrame) proc = rest(), ++out.breaks_completed;
        else if (lframe > lnk05::policy::kCancelFrame && sp.cancel_input) proc = rest(), ++out.breaks_completed;
        break;
      case kGuardAttack:
        lframe += 1.0f;
        if (lframe > 18.0f) proc = rest();
        break;
      default:
        if (std::find(sp.sa_ticks.begin(), sp.sa_ticks.end(), t) != sp.sa_ticks.end() && (proc == kWait || proc == kGuardStance)) proc = kGuardAttack, lframe = 1.0f;
        break;
    }
    if (proc != kBreak) out.max_break_frames = std::max(out.max_break_frames, break_frames), break_frames = 0;
  };
  auto view = [&] {
    post08::LinkView v;
    v.valid = true, v.proc = proc == kGuardAttack ? 0x1e : (proc == kSlip ? 0x1d : proc), v.guard_attack = proc == kGuardAttack, v.frame = lframe, v.start = 1,
    v.end = 12;
    return v;
  };
  auto foe_exec = [&](size_t i, int t) {
    if (paused(t)) return;
    Foe& f = foes[i];
    const int slot = static_cast<int>(i);
    post08::Input in;
    in.tracked = true, in.slot = slot, in.generation = f.gen, in.id = f.id, in.hp = 50;
    in.mode = (f.state == 0 || f.state == 1 || f.state == 3) ? 4 : 3;
    in.state = f.state == 9 ? 4 : f.state;
    in.anim_ok = true, in.anim_frame = f.frame, in.anim_rate = f.rate, in.e_oc2 = f.e_oc2, in.atp = f.atp;
    for (int s = 0; s < 2; ++s) in.at_hit[s] = in.at_on_link[s] = f.hit[s], in.at_shield[s] = f.hit[s] && f.shield[s];
    in.link_ok = true, in.now = view(), in.pass = track.pass_for(static_cast<std::uint64_t>(t), true, link_id);
    in.fresh = arm.fresh(slot, f.id, f.gen, static_cast<std::uint64_t>(t)), in.event_running = event(t);
    const post08::Verdict v = p.judge(in);
    if (v.result == Result::Confirmed) p.commit(slot, f.gen, f.id, v.token, true);
    if (sp.attach && v.result != Result::None) {
      lp.variants.observe(slot, f.id, f.gen, f.e_oc2, f.atp);
      lnk01::Contact k;
      k.result = v.result, k.blocked = v.blocked, k.token = v.token, k.slot = slot, k.id = f.id, k.generation = f.gen;
      const post08::Parry::Episode& e = p.episode(static_cast<unsigned>(slot));
      k.episode_parried = e.bound && e.id == f.id && e.generation == f.gen && e.token == v.token && e.parried;
      k.e_oc2 = f.e_oc2, k.atp = f.atp, k.link_ok = true, k.link_human = !wolf(t), k.link_id = link_id, k.event_running = event(t);
      bool tg_match = false;
      for (int c = 0; c < 3; ++c)
        if (tg[c]) {
          tg_match = tg[c] == key(i);
          break;
        }
      k.tg_match = tg_match, k.tick = static_cast<std::uint64_t>(t);
      const lnk01::Outcome o = lp.on_contact(k);
      if (o.v == lnk01::Verdict::Confirmed) request(Kind::Block, f.id, f.gen, k.token, o.before, o.after, t);
      if (sp.lnk05_on && o.v == lnk01::Verdict::RejectedParry && v.result == Result::Confirmed) out.events.push_back(bm.parry(static_cast<std::uint64_t>(t)));
    }
    arm.note_post(slot, f.id, f.gen, static_cast<std::uint64_t>(t), post08::ArmTrack::armed_now(in.mode == 4 && f.state == 1 ? 4 : 0, f.state == 1 ? 1 : 0, true, f.frame));
    f.armed = false;
    const bool link_guard = proc == kGuardStance || proc == kSlip;
    switch (f.state) {
      case 0: f.state = 1, f.frame = 0, f.rate = 1.0f, f.used = false; break;
      case 1:
        if (f.frame >= 14.0f && f.frame <= 22.0f) f.armed = true;
        if ((f.hit[0] || f.hit[1]) && link_guard && f.front) {
          f.rate = -1.0f, f.state = 3;
          break;
        }
        f.frame += f.rate;
        if (f.frame >= 30.0f) f.state = 9, f.find = 8;
        break;
      case 3:
        f.frame += f.rate;
        if (f.frame <= 0.0f || f.frame >= 30.0f) f.state = 9, f.find = 8, f.rate = 1.0f;
        break;
      case 9:
        if (--f.find <= 0) f.state = 0;
        break;
    }
  };
  for (int t = 1; t <= sp.ticks; ++t) {
    if (t == sp.link_change_at) link_id = 8;
    if (t == sp.death_at) life = 0;
    // ---- mod_update ----
    track.observe(static_cast<std::uint64_t>(t), true, link_id, view().proc, proc == kGuardAttack, lframe, 1, 12);
    if (sp.attach) {
      lnk01::LinkObs ob;
      ob.valid = true, ob.human = !wolf(t), ob.id = link_id, ob.life = life, ob.life_max = life_max, ob.damage_timer = timer, ob.pending_life = item_life;
      ob.active = !paused(t) && !event(t) && life > 0, ob.guard_safe = ob.active && ob.human && proc == kGuardStance && timer == 0;
      lp.observe_link(true, link_id);
      lnk01::DirectEvents de;
      lp.confirm_direct(ob, static_cast<std::uint64_t>(t), de);
      for (unsigned i = 0; i < de.n; ++i)
        if (de.e[i].r == lnk01::DirectResult::Confirmed) request(Kind::Direct, de.e[i].id, de.e[i].generation, de.e[i].token, de.e[i].before, de.e[i].after, t);
      lnk05::ObsIn bi;
      bi.valid = ob.human, bi.alive = life > 0, bi.sim_active = ob.active, bi.event = event(t), bi.link_id = link_id, bi.epoch = lp.epoch(),
      bi.tick = static_cast<std::uint64_t>(t), bi.proc = proc, bi.frame = lframe;
      const lnk05::Out bo = sp.lnk05_on ? bm.observe(bi, lp.milli() >= lnk01::policy::kMaxMilli) : lnk05::Out{};
      if (bo.ev != Ev::None) out.events.push_back(bo);
      if (bo.reset_posture) {
        if (proc == kBreak) out.posture_reset_while_breaking = true;
        lp.break_reset();
      }
      lp.hold(bm.hold());
      const int before = lp.milli();
      lp.tick(ob.active, lnk01::life_permille(life, life_max), ob.guard_safe);
      if (proc == kBreak && bm.state() == lnk05::State::NativeActive && lp.milli() < before) out.regen_while_breaking = true;  // tracked breaks
    }
    if (sp.link_first) link_exec(t);
    for (size_t i = 0; i < foes.size(); ++i) foe_exec(i, t);
    if (!sp.link_first) link_exec(t);
    if (item_life != 0.0f) life = std::max(0, std::min(life_max, life + static_cast<int>(item_life))), item_life = 0;  // dMeter2_c::moveLife
    if (!paused(t)) {
      for (int k = 0; k < 3; ++k) tg[k] = 0, tg_shield[k] = false;
      for (size_t i = 0; i < foes.size(); ++i) {
        Foe& f = foes[i];
        if (!f.armed) continue;
        f.hit[0] = f.hit[1] = f.shield[0] = f.shield[1] = false;
        if (!f.used && f.frame >= 15.0f && f.reach) {
          f.used = true;
          const bool guarded = (proc == kGuardAttack || proc == kGuardStance || proc == kSlip) && f.front && !wolf(t);  // no shield in GUARD_BREAK/damage/wolf
          f.hit[0] = true, f.shield[0] = guarded;
          tg[f.cyl] = key(i), tg_shield[f.cyl] = guarded;
        }
      }
    }
    out.posture.push_back(lp.milli());
    out.proc.push_back(proc);
  }
  out.bc = bm.c, out.pc = lp.c, out.parry = p.c, out.life_end = life, out.conserved = lp.conserved();
  return out;
}
static int count(const Out& o, Ev e) {
  int n = 0;
  for (const auto& x : o.events) n += x.ev == e ? 1 : 0;
  return n;
}

int main() {
  // ================= B01 machine: requests, gate and single activation ==========================================================================
  fam = 1;
  {
    lnk05::Machine m;
    const auto r = m.request(ep(Kind::Block));
    ck(r.ev == Ev::Requested && m.state() == lnk05::State::TriggerPending && m.hold(), "saturating block => TRIGGER_PENDING, regen held");
    ck(m.request(ep(Kind::Block, 10, 2)).ev == Ev::RequestRejected && m.c.duplicate_requests == 1, "second saturation while pending: no queue, no double");
    ck(m.gate(gin(0x1d)) == Gate::Ok, "GUARD_SLIP (block reaction): safe origin");
    for (int pr : {3, 4, 5, 6, 7, 8, 9, 0x1b, 0x1c, 1, 2}) ck(m.gate(gin(pr)) == Gate::Ok, "free ground procedures are safe origins");
    for (int pr : {0x1e, 0x0e, 0x11, 0x12, 0x15, 0x18, 0x19, 0x21, 0x25, 0x2d, 0x32, 0x33, 0x34, 0x3a, 0x44, 0x4f})
      ck(m.gate(gin(pr)) == Gate::ProcNotAllowed, "Shield Attack, rolls, jumps, falls, cuts, damage, horse: refused");
    ck(m.gate(gin(0x1f)) == Gate::AlreadyBreaking, "already in GUARD_BREAK: never re-initialised");
    auto g = gin(3);
    g.human = false;
    ck(m.gate(g) == Gate::NotHuman, "wolf refused");
    g = gin(3), g.alive = false;
    ck(m.gate(g) == Gate::Dead, "dead refused");
    g = gin(3), g.event = true;
    ck(m.gate(g) == Gate::EventRunning, "event/cutscene refused");
    g = gin(3), g.sim_active = false;
    ck(m.gate(g) == Gate::SimInactive, "pause/menu refused");
    g = gin(3), g.mode_flags = 0x40000;
    ck(m.gate(g) == Gate::ModeFlags, "swimming (mode 0x40000) refused");
    g = gin(3), g.mode_flags = 0x10;
    ck(m.gate(g) == Gate::ModeFlags, "horse/hang modes (0x70C52) refused");
    g = gin(3), g.mode_flags = 0x4000000;
    ck(m.gate(g) == Gate::ModeFlags, "special mode 0x4000000 refused");
    g = gin(3), g.damage_this_call = true;
    ck(m.gate(g) == Gate::DamageThisCall, "native damage in the same execute has priority");
    g = gin(3), g.link_id = 8;
    ck(m.gate(g) == Gate::LinkMismatch, "another Link refused");
    g = gin(3), g.epoch = 2;
    ck(m.gate(g) == Gate::LinkMismatch, "another epoch refused");
    ck(m.gate(gin(3, 14)) == Gate::Expired, "block episode expires after 3 ticks");
    const auto t = m.triggered(true, 0x1f, 11);
    ck(t.ev == Ev::Entered && m.state() == lnk05::State::NativeActive && m.c.entered == 1, "entry confirmed by the observed procedure");
    ck(m.gate(gin(3)) == Gate::NoPending, "no second trigger once active");
    ck(m.triggered(true, 0x1f, 12).ev == Ev::None && m.c.entered == 1, "a stray second call does not count");
    ck(m.request(ep(Kind::Direct, 12, 9)).ev == Ev::RequestRejected, "saturation during an active break: rejected");
    lnk05::Machine f;
    f.request(ep(Kind::Block));
    ck(f.triggered(false, 3, 11).ev == Ev::TriggerFailed && f.state() == lnk05::State::Ready && !f.hold(), "failed entry: nothing reset, back to READY");
    lnk05::Machine d;
    d.request(ep(Kind::Direct, 10));
    ck(d.gate(gin(0x32, 20)) == Gate::ProcNotAllowed && d.gate(gin(3, 30)) == Gate::Ok && d.gate(gin(3, 35)) == Gate::Expired,
       "direct episode waits (bounded 24 ticks) for the native damage reaction to end");
  }

  // ================= B02 machine: parry protection, native break adoption, expiry =================================================================
  fam = 2;
  {
    lnk05::Machine m;
    m.request(ep(Kind::Block));
    ck(m.parry(11).ev == Ev::CancelledByParry && m.state() == lnk05::State::Ready && m.c.cancelled_by_parry == 1, "a confirmed parry cancels a pending break");
    lnk05::Machine a;
    a.request(ep(Kind::Block));
    ck(a.observe(obs(3, 0, 11), true).ev == Ev::None, "pending, no native break yet");
    const auto o = a.observe(obs(0x1f, 1, 12), true);
    ck(o.ev == Ev::NativeAdopted && a.c.native_adopted == 1 && a.c.entered == 0, "native 4th-block break during a pending episode: adopted, never doubled");
    lnk05::Machine s;
    ck(s.observe(obs(0x1f, 1, 5), true).ev == Ev::NativeAdopted, "native break with posture already at 100: adopted (one break)");
    lnk05::Machine n;
    ck(n.observe(obs(0x1f, 1, 5), false).ev == Ev::NativeObserved && n.state() == lnk05::State::Ready && n.c.native_observed == 1,
       "native break without saturation: observed only, posture untouched");
    ck(n.observe(obs(0x1f, 2, 6), false).ev == Ev::None, "counted once per entry");
    lnk05::Machine e;
    e.request(ep(Kind::Block, 10));
    ck(e.observe(obs(0x1e, 0, 14), true).ev == Ev::Expired && e.state() == lnk05::State::Ready && !e.hold(), "no safe window in time: EXPIRED, nothing broken");
    lnk05::Machine l;
    l.request(ep(Kind::Block));
    auto lo = obs(3, 0, 11);
    lo.link_id = 8;
    ck(l.observe(lo, true).ev == Ev::Aborted && l.state() == lnk05::State::Ready, "Link replaced: aborted safely");
  }

  // ================= B03 machine: recovery classification and single reset (LNK06) =================================================================
  fam = 3;
  {
    auto active = [] {
      lnk05::Machine m;
      m.request(ep(Kind::Block));
      m.triggered(true, 0x1f, 11);
      return m;
    };
    {
      lnk05::Machine m = active();
      for (int f = 1; f <= 29; ++f) ck(m.observe(obs(0x1f, static_cast<float>(f), 11 + static_cast<std::uint64_t>(f)), true).ev == Ev::None, "active frames");
      const auto o = m.observe(obs(3, 0, 41), true);
      ck(o.ev == Ev::RecoveredCancel && o.reset_posture && m.c.resets == 1, "exit after the cancel frame: legitimate native cancellation, one reset");
      ck(m.observe(obs(3, 0, 42), true).ev == Ev::None && m.c.resets == 1, "no double reset");
    }
    {
      lnk05::Machine m = active();
      for (int f = 1; f <= 34; ++f) m.observe(obs(0x1f, static_cast<float>(f), 11 + static_cast<std::uint64_t>(f)), true);
      ck(m.observe(obs(3, 0, 50), true).ev == Ev::RecoveredEnd, "exit at the animation end: native end");
    }
    {
      lnk05::Machine m = active();
      for (int f = 1; f <= 10; ++f) m.observe(obs(0x1f, static_cast<float>(f), 11 + static_cast<std::uint64_t>(f)), true);
      const auto o = m.observe(obs(0x32, 0, 22), true);
      ck(o.ev == Ev::Interrupted && o.reset_posture && o.next_proc == 0x32, "hit while vulnerable: interrupted by the native damage reaction, cycle closed");
    }
    {
      lnk05::Machine m = active();
      m.observe(obs(0x1f, 5, 12), true);
      ck(m.observe(obs(3, 0, 13, false), true).ev == Ev::Aborted && m.c.resets == 0, "death during the break: aborted, no fabricated recovery");
    }
    {
      lnk05::Machine m = active();
      for (int i = 0; i < 100; ++i) {
        auto p = obs(0x1f, 7, 12 + static_cast<std::uint64_t>(i));
        p.sim_active = false;
        m.observe(p, true);
      }
      ck(m.state() == lnk05::State::NativeActive && m.c.paused_active_ticks == 100 && m.c.active_ticks == 0, "pause: the break waits, no tick counted");
      for (int f = 8; f <= 30; ++f) m.observe(obs(0x1f, static_cast<float>(f), 200 + static_cast<std::uint64_t>(f)), true);
      ck(m.observe(obs(3, 0, 300), true).ev == Ev::RecoveredCancel, "resumes after pause and recovers natively");
    }
    {
      lnk05::Machine m = active();
      for (std::uint64_t i = 0; i < lnk05::policy::kStuckTicks; ++i) m.observe(obs(0x1f, 3, 12 + i), true);
      ck(m.c.stuck == 1 && m.state() == lnk05::State::NativeActive, "stuck is only reported; the mod never forces an exit");
    }
    {
      lnk05::Machine m = active();
      m.observe(obs(0x1f, 30, 41), true);
      ck(m.observe(obs(3, 0, 42), true).reset_posture, "recovered");
      ck(m.request(ep(Kind::Block, 300, 77)).ev == Ev::Requested, "a second break is possible after a completed recovery");
    }
  }

  // ================= B04 model: block saturation -> native break -> vulnerability -> recovery -> reset -> second break =============================
  fam = 4;
  for (bool lf : {true, false}) {
    Sim s;
    s.link_first = lf, s.ticks = 160, s.start_posture_blocks = 4;  // 80 + one Blue block (+20) = 100
    const Out o = run(s);
    ck(o.mod_inits == 1, "exactly one procGuardBreakInit for the saturating block");
    ck(o.bc.entered == 1 && count(o, Ev::Requested) == 1, "requested once, entry confirmed");
    ck(o.bc.recovered_cancel + o.bc.recovered_end == 1 && o.pc.break_resets == 1, "native exit observed, ONE reset");
    ck(!o.posture_reset_while_breaking && !o.regen_while_breaking, "no reset and no regeneration while GUARD_BREAK runs");
    int first_break = -1, reset_tick = -1;
    for (size_t i = 0; i < o.proc.size(); ++i) {
      if (first_break < 0 && o.proc[i] == kBreak) first_break = static_cast<int>(i);
      if (first_break >= 0 && reset_tick < 0 && o.posture[i] == 0) reset_tick = static_cast<int>(i);
    }
    ck(first_break >= 0 && o.posture[static_cast<size_t>(first_break)] == 100000, "posture stays 100 at the start of the break");
    ck(reset_tick > first_break + 28, "reset only after the native cancel/end");
    ck(o.max_break_frames >= 28 && o.max_break_frames <= 35, "native duration respected (cancel 28 .. end 35)");
    ck(o.native_damage == 0 && o.life_end == 80, "no artificial damage");
  }
  {
    Sim s;
    s.ticks = 160, s.start_posture_blocks = 4, s.cancel_input = false;
    const Out o = run(s);
    ck(o.bc.recovered_end == 1 && o.max_break_frames == 35, "no input: the native animation runs to its end (35)");
  }
  {
    Sim s;
    s.ticks = 700, s.start_posture_blocks = 4;  // keep blocking after the first recovery: posture refills (Blue +20 x5) and breaks again
    const Out o = run(s);
    ck(o.mod_inits + o.bc.native_adopted >= 2 && o.pc.break_resets >= 2, "second break after a complete recovery");
    ck(o.mod_inits + o.native_breaks == o.bc.entered + o.bc.native_adopted + o.bc.native_observed, "every native entry accounted, none doubled");
  }

  // ================= B05 model: parry, direct hit, native counter, variants =========================================================================
  fam = 5;
  {
    Sim s;
    s.ticks = 40, s.start_posture_blocks = 5, s.sa_ticks = {13};  // already at 100, perfect parry (the only impact of the window)
    const Out o = run(s);
    ck(o.parry.confirmed == 1 && o.mod_inits == 0 && o.bc.requested == 0 && o.native_breaks == 0, "perfect parry at 100: no break, no request");
    Sim l = s;
    l.start_posture_blocks = 4;  // 80 + parry (+0) must stay 80: never reaches 100
    const Out q = run(l);
    ck(q.parry.confirmed == 1 && *std::max_element(q.posture.begin(), q.posture.end()) == 80000 && q.bc.requested == 0, "parry adds +0 to Link");
    Sim w = s;
    w.ticks = 70;  // the SAME foe attacks again after the parry; Link blocks at 100 => this ordinary block breaks (valid impact)
    const Out r = run(w);
    ck(r.parry.confirmed == 1 && r.mod_inits == 1 && r.bc.requested == 1, "after the parry, the next ordinary block at 100 breaks");
  }
  {
    Sim s;
    s.ticks = 90, s.start_posture_blocks = 4, s.hold_guard = false;  // 80 + Blue direct (+30) => 100 while the damage reaction runs
    s.foes[0].front = true;
    const Out o = run(s);
    ck(o.native_damage >= 1 && o.bc.saturations_direct >= 1, "direct hit saturates");
    ck(o.mod_inits <= 1, "at most one break for the direct episode");
    bool break_during_damage = false;
    for (size_t i = 1; i < o.proc.size(); ++i)
      if (o.proc[i] == kBreak && o.proc[i - 1] == kDamage) break_during_damage = true;
    ck(!break_during_damage, "never breaks over the native damage reaction (waits for it to end)");
    ck(o.mod_inits == 1 || o.bc.expired == 1, "deferred trigger inside the bounded window, or expiry");
  }
  {
    Sim s;  // native 4th consecutive block coincides with our saturation: ONE break (the native one), adopted
    s.ticks = 200, s.start_posture_blocks = 2;
    const Out o = run(s);
    ck(o.native_breaks >= 1, "native counter break happens");
    ck(o.mod_inits + o.native_breaks == o.bc.entered + o.bc.native_adopted + o.bc.native_observed, "each break initialised once");
    for (const auto& e : o.events) ck(!(e.ev == Ev::Entered && e.proc != kBreak), "entry always verified");
  }
  for (int atp : {1, 2, 3, 6}) {
    Sim s;
    s.ticks = 120;
    s.foes[0].e_oc2 = atp != 1, s.foes[0].atp = atp;
    s.start_posture_blocks = 4;
    const Out o = run(s);
    ck(o.mod_inits == 1 && o.pc.break_resets == 1, "Blue/Red/Black/Black fury block crossing 100: one break, one reset");
  }
  {
    Sim s;  // group
    s.ticks = 400, s.start_posture_blocks = 3;
    Foe a, b;
    a.id = 100, b.id = 101, b.first_delay = 5, b.cyl = 1, b.e_oc2 = true, b.atp = 2;
    s.foes = {a, b};
    const Out o = run(s);
    ck(o.mod_inits + o.native_breaks == o.bc.entered + o.bc.native_adopted + o.bc.native_observed, "group: no double initialisation");
    ck(o.conserved && o.pc.break_resets == o.bc.resets, "group: one reset per completed cycle, conservation");
  }

  // ================= B06 model: special states ===================================================================================================
  fam = 6;
  {
    Sim w;
    w.ticks = 60, w.start_posture_blocks = 4, w.wolf_from = 1;
    const Out o = run(w);
    ck(o.mod_inits == 0, "wolf: no break");
    Sim sw;
    sw.ticks = 60, sw.start_posture_blocks = 4, sw.swim_from = 1;
    ck(run(sw).mod_inits == 0, "swimming: no break");
    Sim ev;
    ev.ticks = 60, ev.start_posture_blocks = 4, ev.event_from = 1, ev.event_to = 60;
    ck(run(ev).mod_inits == 0, "event: no break");
    Sim pz;
    pz.ticks = 160, pz.start_posture_blocks = 4, pz.pause_from = 30, pz.pause_to = 90;
    const Out po = run(pz);
    ck(po.mod_inits == 1 && po.pc.break_resets == 1, "pause in the middle: break completes after resuming");
    Sim dt;
    dt.ticks = 120, dt.start_posture_blocks = 4, dt.death_at = 25;
    const Out d = run(dt);
    ck(d.pc.break_resets == 0, "death: no fabricated recovery");
    Sim lc;
    lc.ticks = 120, lc.start_posture_blocks = 4, lc.link_change_at = 25;
    const Out l = run(lc);
    ck(l.pc.break_resets == 0 && l.bc.aborted == 1 && l.posture[24] == 0, "room change / new Link: aborted, posture reset by identity only");
  }

  // ================= B07 regressions: no break without impact, POST08 untouched, conservation =========================================================
  fam = 7;
  {
    Sim idle;
    idle.ticks = 200, idle.start_posture_blocks = 5;
    idle.foes[0].reach = false;
    const Out o = run(idle);
    ck(o.mod_inits == 0 && o.bc.requested == 0, "a full bar alone never breaks (no impact, no regeneration break)");
    Sim on, off;
    on.ticks = off.ticks = 300, on.sa_ticks = off.sa_ticks = {13, 113};
    off.attach = false;
    const Out a = run(on), b = run(off);
    ck(a.parry.confirmed == b.parry.confirmed && a.parry.stale == b.parry.stale, "POST08 verdicts unchanged while the posture is low");
    ck(a.conserved, "posture conservation including break resets");
    ck(std::strcmp(lnk05::kVersion, "0.11.0") == 0 && std::strcmp(lnk01::kVersion, "0.11.0") == 0, "version 0.11.0");
    ck(lnk05::policy::kCancelFrame == 28.0f && lnk05::policy::kEndFrame == 35.0f, "native mGuardBreakAnm cancel/end");
    ck(lnk01::policy::kWeights[0].block == 20 && lnk01::policy::kWeights[3].direct == 45 && lnk01::policy::kBandPointsPerSecond[3] == 2, "balance unchanged");
    char line[1024];
    lnk05::Out x;
    x.ep.link_id = 4294967295u, x.ep.epoch = x.ep.tick = x.ep.generation = x.ep.token = 18446744073709551615ull, x.active_ticks = 18446744073709551615ull;
    ck(lnk05::fmt_event(line, sizeof line, "LNK06_RECOVERY", x, 100000) < 1000, "worst-case event line bounded");
    ck(sizeof(lnk05::Machine) < 512, "small, bounded state");
  }

  // ================= B08 sweep: invariants over orders, variants, groups, parries, pauses, events, deaths and Link changes ==========================
  fam = 8;
  {
    unsigned runs = 0, saturated_runs = 0, broken_runs = 0, identical_runs = 0, native_runs = 0, parry_runs = 0;
    for (int blocks = 0; blocks <= 5; ++blocks)
      for (bool lf : {true, false})
        for (bool guard : {true, false})
          for (int atp : {1, 2, 3, 6})
            for (int group = 0; group < 2; ++group)
              for (int sa = 0; sa < 3; ++sa)
                for (int special = 0; special < 7; ++special)
                  for (bool cancel : {true, false}) {
                    Sim s;
                    s.ticks = 260, s.link_first = lf, s.hold_guard = guard, s.start_posture_blocks = blocks, s.cancel_input = cancel;
                    s.foes[0].e_oc2 = atp != 1, s.foes[0].atp = atp;
                    if (group) {
                      Foe b;
                      b.id = 101, b.first_delay = 3 + 4 * group, b.cyl = 1, b.e_oc2 = true, b.atp = 2;
                      s.foes.push_back(b);
                    }
                    if (sa == 1) s.sa_ticks = {13, 63};
                    if (sa == 2) s.sa_ticks = {40, 90, 140};
                    switch (special) {
                      case 1: s.pause_from = 20, s.pause_to = 50; break;
                      case 2: s.event_from = 30, s.event_to = 70; break;
                      case 3: s.death_at = 45; break;
                      case 4: s.link_change_at = 40; break;
                      case 5: s.wolf_from = 100; break;
                      case 6: s.swim_from = 60; break;
                      default: break;
                    }
                    const Out o = run(s);
                    ++runs;
                    const bool sat = o.bc.saturating_impacts > 0;
                    saturated_runs += sat ? 1u : 0u, broken_runs += o.mod_inits ? 1u : 0u, native_runs += o.native_breaks ? 1u : 0u;
                    parry_runs += o.bc.cancelled_by_parry ? 1u : 0u;
                    if (std::getenv("LNKS2_TRACE") && (o.mod_inits + o.native_breaks != o.bc.entered + o.bc.native_adopted + o.bc.native_observed || o.posture_reset_while_breaking || o.regen_while_breaking)) {
                      static int shown = 0;
                      if (shown++ < 4) {
                        std::printf("TRACE blocks=%d lf=%d guard=%d atp=%d group=%d sa=%d special=%d cancel=%d inits=%llu nat=%llu ent=%llu ad=%llu obs=%llu rw=%d gw=%d\n", blocks, lf, guard, atp, group, sa, special, cancel,
                                    (unsigned long long)o.mod_inits, (unsigned long long)o.native_breaks, (unsigned long long)o.bc.entered, (unsigned long long)o.bc.native_adopted, (unsigned long long)o.bc.native_observed, o.posture_reset_while_breaking, o.regen_while_breaking);
                        for (const auto& e : o.events) std::printf("  ev %s tick=%llu proc=%d next=%d lf=%.1f\n", lnk05::name(e.ev), (unsigned long long)e.ep.tick, e.proc, e.next_proc, (double)e.last_frame);
                      }
                    }
                    ck(o.mod_inits + o.native_breaks == o.bc.entered + o.bc.native_adopted + o.bc.native_observed, "S1 every native entry accounted once (no double break)");
                    ck(o.mod_inits == o.bc.entered && o.bc.entered <= o.bc.requested, "S2 one procGuardBreakInit per accepted request at most");
                    ck(!o.posture_reset_while_breaking && !o.regen_while_breaking, "S3 no reset and no regeneration during GUARD_BREAK");
                    ck(o.conserved && o.pc.break_resets == o.bc.resets, "S4 conservation; one reset per closed cycle");
                    ck(!o.mod_break_over_damage, "S5 never initialised over a native damage reaction");
                    ck(o.max_break_frames <= 35, "S6 never longer than the native animation");
                    ck(o.parry.confirmed == 0 || o.bc.cancelled_by_parry <= o.parry.confirmed, "S7 parry cancellations only from confirmed parries");
                    for (const auto& e : o.events) {
                      if (e.ev == Ev::Requested) ck(e.ep.after == lnk01::policy::kMaxMilli && e.ep.attacker != 0, "S8 request only from a valid impact at 100");
                      if (e.ev == Ev::Entered) ck(e.proc == kBreak && e.by_mod, "S9 entry verified by the procedure readback");
                      if (e.reset_posture) ck(e.next_proc != kBreak && e.ev != Ev::Aborted, "S10 reset only after the native exit");
                    }
                    if (special == 3) ck(o.life_end == 0, "death scenario really dead");
                    if (special == 5 || special == 6) {
                      bool late = false;  // no mod-initiated break may START after wolf/swim began (+3 ticks of pending request at most)
                      for (const auto& e : o.events)
                        if (e.ev == Ev::Entered && static_cast<int>(e.ep.tick) > (special == 5 ? 100 : 60)) late = true;
                      ck(!late, "S11 no break as wolf or while swimming");
                    }
                    if (special == 2) {
                      for (const auto& e : o.events)
                        if (e.ev == Ev::Entered) ck(!(e.ep.tick >= 30 && e.ep.tick < 70), "S12 no break requested during an event");
                    }
                    // regression: identical to 0.10.1 while LNK05 is never involved (no saturation)
                    Sim off = s;
                    off.lnk05_on = false;
                    const Out b = run(off);
                    if (!sat && b.native_breaks == 0) {
                      ++identical_runs;
                      ck(b.posture == o.posture && b.proc == o.proc && b.parry.confirmed == o.parry.confirmed && b.life_end == o.life_end,
                         "S13 bit-identical to 0.10.1 without saturation");
                    }
                    ck(b.mod_inits == 0 && b.bc.requested == 0, "S14 LNK05 disabled => nothing initialised");
                  }
    std::printf("B08 sweep runs=%u saturated=%u mod_breaks=%u native_breaks=%u parry_cancels=%u identical_to_0_10_1=%u\n", runs, saturated_runs, broken_runs,
                native_runs, parry_runs, identical_runs);
    ck(saturated_runs > runs / 4 && broken_runs > runs / 8 && native_runs > 0 && identical_runs > 100, "the sweep really exercises the break and the regression");
  }

  for (int i = 1; i <= 8; ++i) std::printf("B%02d checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("LNK_S2_LAB checks=%u failures=%u RESULT=%s LAB_MODEL_ONLY\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
