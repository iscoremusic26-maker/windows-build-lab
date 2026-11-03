// LNK SESSION02 FIX01 0.11.1 laboratory (LAB_MODEL_ONLY): manual shield (R), R+B Shield Attack, extended post-break penalty, dodge veto, rearm latch,
// LNK04 x1.5 from the real manual guard. Part 1 drives the REAL lnkfx::Defense and its decision functions. Part 2 runs a tick model of Link's execute in the
// native order measured in the 2.0.3 source/EXE (setStickData -> checkDamageAction -> setShieldGuard -> procedure [sword / Shield Attack site
// setRStatus(0x3a)+spActionTrigger / checkMoveDoAction] -> setCollision), with procGuardBreak re-raising the guard flag every frame and ERFLG1_UNK_20 only
// when Link is not locked on, plus the REAL lnk05::Machine, lnk01::Posture and post08::Parry/ArmTrack/LinkTrack wired exactly as mod.cpp wires them.
// A passing model is NOT a runtime observation: HUMAN_RUNTIME_PENDING.
#include "../src/lnk_core.hpp"
#include "../src/lnks2_core.hpp"
#include "../src/lnks2fix_core.hpp"
#include "../src/parry_core.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

using lnkfx::Phase;
using post08::Result;
static unsigned checks = 0, failures = 0;
static unsigned fam_checks[12] = {}, fam_fail[12] = {};
static int fam = 0;
static void ck(bool b, const char* what = "") {
  ++checks;
  ++fam_checks[fam];
  if (!b) {
    ++failures;
    ++fam_fail[fam];
    if (fam_fail[fam] <= 6 || std::getenv("FX_ALLFAIL")) std::printf("FAIL F%02d %s\n", fam, what);
  }
}
// Every machine-driving loop is bounded: a mutant that never reaches the expected phase FAILS here instead of hanging the mutation runner.
static bool bounded(int& n) {
  if (++n <= 1000) return true;
  ck(false, "loop bound exceeded: the machine never reached the expected phase");
  return false;
}
static lnkfx::Begin bg(std::uint64_t tick, bool extended = true, lnkfx::Origin o = lnkfx::Origin::ModBreak) {
  lnkfx::Begin b;
  b.link_id = 7, b.epoch = 1, b.tick = tick, b.origin = o, b.extended = extended;
  return b;
}
static lnkfx::Obs ob(std::uint64_t tick, int proc, bool sim = true) {
  lnkfx::Obs o;
  o.valid = o.human = o.alive = true, o.sim_active = sim, o.link_id = 7, o.epoch = 1, o.tick = tick, o.proc = proc;
  return o;
}

// ============================================ Link + foes tick model ===============================================================================
enum P {
  kWait = 3, kMove = 4, kAtnWait = 6, kSideStep = 0x0a, kFrontRoll = 0x0e, kSideRoll = 0x11, kBackJump = 0x12, kSlip = 0x1d, kGuardAttack = 0x1e, kBreak = 0x1f,
  kCut = 0x21, kDamage = 0x32
};
static bool dodge_proc(int p) { return p == kSideStep || p == kFrontRoll || p == kSideRoll || p == kBackJump; }
struct In {  // the player's hands at this tick
  bool lock = false, r = false, b = false, a = false;  // b/a = new presses (trigger); r/lock = held
  int stick = 0;                                       // 0 none, 1 forward, 2 back, 3 side
};
struct Foe {
  std::uint32_t id = 100;
  std::uint64_t gen = 1;
  bool e_oc2 = false;
  int atp = 1, first_delay = 0, cyl = 0, cooldown = 8;
  bool front = true, reach = true;
  int state = 0;
  float frame = 0, rate = 1.0f;
  int find = 0;
  bool hit[2] = {false, false}, shield[2] = {false, false}, used = false, armed = false;
};
struct Sim {
  int ticks = 300;
  bool link_first = true, fx = true, learned = true, has_shield = true;
  std::function<In(int)> input = [](int) { return In{true, true, false, false, 0}; };
  std::vector<Foe> foes = {Foe{}};
  int start_blocks = 0;  // Blue blocks pre-credited before the fight
  int pause_from = -1, pause_to = -1, event_from = -1, event_to = -1, death_at = -1, link_change_at = -1, wolf_from = -1;
  int forced_roll_at = -1;  // a FORCED native front roll (landing/demo path: procFrontRollInit without checkMoveDoAction)
  int native_break_at = -1; // a native break without saturation (e.g. at_spl 9/10/11 on the shield) at this tick
};
struct Out {
  std::vector<int> proc, posture, phase;
  std::vector<char> shield_reg, flag;
  lnkfx::Counters dc;
  lnk05::Counters bc;
  lnk01::PCounters pc;
  post08::PCounters parry;
  int mod_breaks = 0, native_breaks = 0, bashes = 0, cuts = 0, dodges = 0, dodges_denied = 0, forced_rolls = 0, damage = 0, blocks = 0, b_consumed = 0;
  int shield_during_block = 0, bash_during_block = 0, dodge_start_during_block = 0, cut_during_penalty = 0, moves_during_penalty = 0;
  int guard_ticks = 0, bonus_ticks = 0, bonus_while_blocked = 0, bonus_without_manual = 0, regen_guard_ticks = 0;
  int flag_clears_guard = 0, flag_clears_collision = 0, residue_frames = 0, damage_in_post = 0;
  std::vector<int> enter_ticks, rearm_ticks, complete_ticks, exit_ticks;
  bool conserved = false, reset_in_break = false;
  int life_end = 0;
};

static Out run(const Sim& sp) {
  Out out;
  post08::Parry p;
  p.enabled = true;
  post08::LinkTrack track;
  post08::ArmTrack arm;
  lnk01::Posture lp;
  lnk05::Machine bm;
  lnkfx::Defense def;
  std::vector<Foe> foes = sp.foes;
  for (Foe& f : foes) f.find = f.first_delay, f.state = f.first_delay ? 9 : 0;
  const int life_max = 80;
  int life = life_max, timer = 0, slip = 0, dmg = 0, counter = 0, proc = kWait, busy = 0;
  float lframe = 0, item_life = 0;
  bool flag = false, erflg20 = false, rstatus_set = false;
  std::uintptr_t tg[3] = {0, 0, 0};
  bool tg_shield[3] = {false, false, false};
  std::uint32_t link_id = 7;
  unsigned trig = 0, held = 0;  // mItemTrigger / mItemButton of this execute
  bool fx_sample_ok = false, fx_r = false, fx_b = false, offered_prev = false, offered_now = false, pending = false, guard_accepted = false;
  bool shield_registered = false;
  auto paused = [&](int t) { return t >= sp.pause_from && t < sp.pause_to; };
  auto event = [&](int t) { return t >= sp.event_from && t < sp.event_to; };
  auto wolf = [&](int t) { return sp.wolf_from >= 0 && t >= sp.wolf_from; };
  auto key = [&](size_t i) { return static_cast<std::uintptr_t>(0x1000 + i * 16); };
  auto rest = [&](const In& in) { return in.lock ? kAtnWait : (in.stick ? kMove : kWait); };
  auto free_proc = [&](int pr) { return pr == kWait || pr == kMove || pr == kAtnWait; };
  if (sp.start_blocks) {
    lp.observe_link(true, link_id);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 63, k.id = 999, k.generation = 1, k.link_ok = k.link_human = true, k.link_id = link_id;
    for (int i = 0; i < sp.start_blocks; ++i) k.token = 100000 + static_cast<std::uint64_t>(i), lp.on_contact(k);
  }
  auto request = [&](lnk05::Kind kind, std::uint32_t id, std::uint64_t gen, std::uint64_t token, int before, int after, int t) {
    if (after < lnk01::policy::kMaxMilli) return;
    if (sp.fx && def.request_blocked()) return;  // mod.cpp lnk05_request: no cascading break during the penalty
    lnk05::Episode e;
    e.link_id = link_id, e.epoch = lp.epoch(), e.tick = static_cast<std::uint64_t>(t), e.kind = kind, e.attacker = id, e.generation = gen, e.token = token;
    e.before = before, e.after = after;
    bm.request(e);
    lp.hold(bm.hold());
  };
  auto guard_break_init = [&](bool by_mod) { proc = kBreak, lframe = 0, ++(by_mod ? out.mod_breaks : out.native_breaks); };
  auto link_exec = [&](int t) {
    if (paused(t) || life <= 0) return;
    const In in = sp.input(t);
    const bool human = !wolf(t);
    // ---- setStickData (native): sample PAD_1 unless an event/death ----
    trig = held = 0;
    if (!event(t)) {
      if (in.b) trig |= lnkfx::policy::kBtnB;
      if (in.a) trig |= lnkfx::policy::kBtnA;
      if (in.r) held |= lnkfx::policy::kBtnR;
      if (in.r && !(fx_r)) trig |= lnkfx::policy::kBtnR;  // getTrigLockR (edge) - used by the NATIVE Shield Attack when FIX01 is off
    }
    // ---- setStickData POST (mirror of on_stick_post_fx) ----
    fx_sample_ok = false;
    if (sp.fx && human) {
      offered_prev = offered_now, offered_now = false, pending = false;
      fx_r = (held & lnkfx::policy::kBtnR) != 0, fx_b = (trig & lnkfx::policy::kBtnB) != 0;
      fx_sample_ok = true;
      def.sample(link_id, lp.epoch(), fx_r, static_cast<std::uint64_t>(t));
      if (lnkfx::consume_b(fx_b, fx_r, def.phase(), offered_prev)) trig &= ~lnkfx::policy::kBtnB, ++out.b_consumed;
    } else {
      fx_r = (held & lnkfx::policy::kBtnR) != 0;
    }
    // ---- checkDamageAction (native) ----
    const int timer_before = timer;
    int first = -1;
    for (int k = 0; k < 3 && first < 0; ++k)
      if (tg[k]) first = k;
    if (timer != 0) {
      --timer;
    } else if (first >= 0 && !event(t)) {
      if (tg_shield[first]) {
        ++out.blocks;
        if (proc != kSlip) {  // procGuardSlipInit
          if (++counter == 4) counter = 0, guard_break_init(false);
          else proc = kSlip, slip = 8, lframe = 0;
        }
      } else if (proc != kSlip) {
        int d = 1;
        for (size_t i = 0; i < foes.size(); ++i)
          if (tg[first] == key(i)) d = foes[i].atp;
        item_life -= static_cast<float>(d), timer = 30, ++out.damage;
        if (def.phase() == Phase::PostVulnerable) ++out.damage_in_post;
        proc = kDamage, dmg = 12;
      }
    }
    if (t == sp.native_break_at && proc != kBreak) guard_break_init(false);  // native at_spl break (no saturation)
    // ---- LNK05 POST (mirror of on_link_damage_post_lnk05) + FIX01 penalty begin ----
    if (bm.state() == lnk05::State::TriggerPending) {
      lnk05::GateIn g;
      g.valid = true, g.link_id = link_id, g.epoch = lp.epoch(), g.tick = static_cast<std::uint64_t>(t), g.human = human, g.alive = life > 0;
      g.sim_active = !paused(t), g.event = event(t), g.proc = proc, g.damage_this_call = timer > timer_before;
      if (bm.check(g) == lnk05::Gate::Ok) {
        guard_break_init(true);
        const lnk05::Out o = bm.triggered(proc == kBreak, proc, static_cast<std::uint64_t>(t));
        if (o.ev == lnk05::Ev::Entered && sp.fx) {
          lnkfx::Begin b;
          b.link_id = link_id, b.epoch = lp.epoch(), b.tick = static_cast<std::uint64_t>(t), b.origin = lnkfx::Origin::ModBreak, b.extended = true;
          if (def.begin(b).ev == lnkfx::Ev::EnterNative) out.enter_ticks.push_back(t);
        }
      }
    }
    // ---- setShieldGuard (native) + POST (mirror of on_shield_post_fx) ----
    const bool mandatory = proc == kSlip;
    flag = mandatory || (in.lock && sp.has_shield && human && proc != kDamage && proc != kCut && !dodge_proc(proc));
    if (sp.fx && human) {
      const lnkfx::Guard gd = lnkfx::decide_guard(flag, mandatory, fx_sample_ok && fx_r, def.phase());
      if (lnkfx::clears_flag(gd)) flag = false, ++out.flag_clears_guard;
      guard_accepted = gd == lnkfx::Guard::Accepted;
    } else {
      guard_accepted = false;
    }
    // ---- procedure ----
    rstatus_set = false;
    switch (proc) {
      case kBreak:  // procGuardBreak: setUpperGuardAnime every frame; cancel 28 / end 35; ERFLG1_UNK_20 only when not locked on
        flag = true, lframe += 1.0f;
        if (lframe >= lnk05::policy::kEndFrame || lframe > lnk05::policy::kCancelFrame) {
          if (!in.lock) erflg20 = true;
          proc = rest(in);
        }
        break;
      case kSlip:
        flag = true;
        if (--slip <= 0) {
          if (!in.lock) erflg20 = true;
          proc = rest(in);
        }
        break;
      case kGuardAttack:
        flag = true, lframe += 1.0f;
        if (lframe > 18.0f) proc = rest(in);
        break;
      case kDamage:
        if (--dmg <= 0) proc = rest(in);
        break;
      default:
        if (busy > 0) {
          if (--busy == 0) proc = rest(in);
          break;
        }
        if (!free_proc(proc)) break;
        proc = rest(in);
        // checkItemAction: sword first (swordSwingTrigger = mItemTrigger & B), then the Shield Attack site
        if (trig & lnkfx::policy::kBtnB) {
          proc = kCut, busy = 20, ++out.cuts;
          if (def.phase() == Phase::NativeBreak || def.phase() == Phase::PostVulnerable) ++out.cut_during_penalty;
          break;
        }
        if (in.lock && sp.learned && sp.has_shield && human && !rstatus_set) {
          bool spa = false;
          if (sp.fx) {  // setRStatus PRE: offer + skip; spActionTrigger POST: R+B
            offered_now = true, pending = true;
            pending = false;
            spa = lnkfx::decide_bash(fx_r, fx_b, def.phase()) == lnkfx::Bash::Accepted;
          } else {
            rstatus_set = true;
            spa = (trig & lnkfx::policy::kBtnR) != 0;  // native: R trigger
          }
          if (spa) {
            proc = kGuardAttack, lframe = 1.0f, ++out.bashes;
            if (def.guard_blocked()) ++out.bash_during_block;
            break;
          }
        }
        // checkMoveDoAction: A with DoStatus JUMP (locked on) or front roll (running)
        if (in.a) {
          const unsigned st = in.lock ? lnkfx::policy::kStatusJump : (in.stick ? lnkfx::policy::kStatusFrontRoll : 0u);
          if (st) {
            const std::int16_t rel = in.stick == 2 ? static_cast<std::int16_t>(0x8000) : (in.stick == 3 ? static_cast<std::int16_t>(0x4000) : 0);
            if (sp.fx && def.dodge_blocked() && lnkfx::classify_dodge(true, st, rel, false) != lnkfx::Dodge::None) {
              ++out.dodges_denied;
              break;
            }
            proc = st == lnkfx::policy::kStatusFrontRoll ? kFrontRoll : (in.stick == 2 ? kBackJump : kSideStep), busy = 14, ++out.dodges;
            if (def.dodge_blocked()) ++out.dodge_start_during_block;
          }
        }
        if (def.guard_blocked() && (proc == kMove || proc == kAtnWait)) ++out.moves_during_penalty;
        break;
    }
    if (t == sp.forced_roll_at) proc = kFrontRoll, busy = 14, ++out.forced_rolls;  // forced native path: never vetoed
    // ---- setCollision PRE (mirror of on_collision_pre_fx) + registration ----
    if (sp.fx && human && def.guard_blocked() && proc != kBreak && proc != kSlip && proc != kGuardAttack && flag) flag = false, ++out.flag_clears_collision;
    const bool upper_guard = flag && proc != kBreak && proc != kGuardAttack;
    shield_registered = sp.has_shield && (upper_guard || proc == kGuardAttack) && !erflg20;
    if (def.guard_blocked() && flag) ++out.residue_frames;
    erflg20 = false;
  };
  auto view = [&] {
    post08::LinkView v;
    v.valid = true, v.proc = proc, v.guard_attack = proc == kGuardAttack, v.frame = lframe, v.start = 1, v.end = 12;
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
    if (v.result != Result::None) {
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
      if (o.v == lnk01::Verdict::Confirmed) request(lnk05::Kind::Block, f.id, f.gen, k.token, o.before, o.after, t);
      if (o.v == lnk01::Verdict::RejectedParry && v.result == Result::Confirmed) bm.parry(static_cast<std::uint64_t>(t));
    }
    arm.note_post(slot, f.id, f.gen, static_cast<std::uint64_t>(t), post08::ArmTrack::armed_now(in.mode == 4 && f.state == 1 ? 4 : 0, f.state == 1 ? 1 : 0, true, f.frame));
    f.armed = false;
    switch (f.state) {
      case 0: f.state = 1, f.frame = 0, f.rate = 1.0f, f.used = false; break;
      case 1:
        if (f.frame >= 14.0f && f.frame <= 22.0f) f.armed = true;
        if ((f.hit[0] || f.hit[1]) && f.shield[0] && f.front) {
          f.rate = -1.0f, f.state = 3;
          break;
        }
        f.frame += f.rate;
        if (f.frame >= 30.0f) f.state = 9, f.find = f.cooldown;
        break;
      case 3:
        f.frame += f.rate;
        if (f.frame <= 0.0f || f.frame >= 30.0f) f.state = 9, f.find = f.cooldown, f.rate = 1.0f;
        break;
      case 9:
        if (--f.find <= 0) f.state = 0;
        break;
    }
  };
  for (int t = 1; t <= sp.ticks; ++t) {
    if (t == sp.link_change_at) link_id = 8;
    if (t == sp.death_at) life = 0;
    // ---- mod_update (order of lnk01_update) ----
    track.observe(static_cast<std::uint64_t>(t), true, link_id, view().proc, proc == kGuardAttack, lframe, 1, 12);
    lnk01::LinkObs lo;
    lo.valid = true, lo.human = !wolf(t), lo.id = link_id, lo.life = life, lo.life_max = life_max, lo.damage_timer = timer, lo.pending_life = item_life;
    lo.active = !paused(t) && !event(t) && life > 0;
    const bool upper = flag && proc != kBreak && proc != kGuardAttack;
    lo.guard_safe = lo.active && lo.human && upper && timer == 0 && proc != kSlip && proc != kGuardAttack && proc != kBreak;
    if (lo.guard_safe && !guard_accepted && sp.fx) ++out.bonus_without_manual;
    lo.guard_safe = lnkfx::regen_bonus(lo.guard_safe, sp.fx, guard_accepted, def.phase());  // the SAME function mod.cpp uses
    if (lo.guard_safe) {
      ++out.bonus_ticks;
      if (def.guard_blocked()) ++out.bonus_while_blocked;
    }
    lp.observe_link(true, link_id);
    lnk01::DirectEvents de;
    lp.confirm_direct(lo, static_cast<std::uint64_t>(t), de);
    for (unsigned i = 0; i < de.n; ++i)
      if (de.e[i].r == lnk01::DirectResult::Confirmed) request(lnk05::Kind::Direct, de.e[i].id, de.e[i].generation, de.e[i].token, de.e[i].before, de.e[i].after, t);
    lnk05::ObsIn bi;
    bi.valid = lo.human, bi.alive = life > 0, bi.sim_active = lo.active, bi.event = event(t), bi.link_id = link_id, bi.epoch = lp.epoch(),
    bi.tick = static_cast<std::uint64_t>(t), bi.proc = proc, bi.frame = lframe;
    const lnk05::Out bo = bm.observe(bi, lp.milli() >= lnk01::policy::kMaxMilli);
    if (bo.reset_posture) {
      if (proc == kBreak) out.reset_in_break = true;
      lp.break_reset();
    }
    lp.hold(bm.hold());
    if (sp.fx && (bo.ev == lnk05::Ev::NativeAdopted || bo.ev == lnk05::Ev::NativeObserved) && lo.human) {
      lnkfx::Begin b;
      b.link_id = link_id, b.epoch = lp.epoch(), b.tick = static_cast<std::uint64_t>(t);
      b.origin = bo.ev == lnk05::Ev::NativeAdopted ? lnkfx::Origin::NativeAdopted : lnkfx::Origin::NativeObserved;
      b.extended = bo.ev == lnk05::Ev::NativeAdopted;
      if (def.begin(b).ev == lnkfx::Ev::EnterNative) out.enter_ticks.push_back(t);
    }
    if (sp.fx) {
      lnkfx::Obs fo;
      fo.valid = true, fo.human = lo.human, fo.alive = life > 0, fo.sim_active = lo.active, fo.event = event(t), fo.link_id = link_id, fo.epoch = lp.epoch();
      fo.tick = static_cast<std::uint64_t>(t), fo.proc = proc;
      const lnkfx::Out fo2 = def.observe(fo);
      if (fo2.ev == lnkfx::Ev::Complete || fo2.ev == lnkfx::Ev::NativeExit) out.complete_ticks.push_back(t);
      if (fo2.ev == lnkfx::Ev::PostVulnerable || fo2.ev == lnkfx::Ev::NativeExit) out.exit_ticks.push_back(t);
    }
    const int before_regen = static_cast<int>(lp.c.regen_guard_ticks);
    lp.tick(lo.active, lnk01::life_permille(life, life_max), lo.guard_safe);
    out.regen_guard_ticks += static_cast<int>(lp.c.regen_guard_ticks) - before_regen;
    // ---- actors ----
    const Phase ph_before = def.phase();
    if (sp.link_first) link_exec(t);
    for (size_t i = 0; i < foes.size(); ++i) foe_exec(i, t);
    if (!sp.link_first) link_exec(t);
    if (def.phase() == Phase::Ready && ph_before == Phase::RearmRequired) out.rearm_ticks.push_back(t);
    if (item_life != 0.0f) life = std::max(0, std::min(life_max, life + static_cast<int>(item_life))), item_life = 0;
    if (def.guard_blocked() && shield_registered && proc != kGuardAttack) ++out.shield_during_block;
    if (guard_accepted) ++out.guard_ticks;
    // ---- collision pass ----
    if (!paused(t)) {
      for (int k = 0; k < 3; ++k) tg[k] = 0, tg_shield[k] = false;
      for (size_t i = 0; i < foes.size(); ++i) {
        Foe& f = foes[i];
        if (!f.armed) continue;
        f.hit[0] = f.hit[1] = f.shield[0] = f.shield[1] = false;
        if (!f.used && f.frame >= 15.0f && f.reach && !dodge_proc(proc)) {  // a dodge carries Link out of reach
          f.used = true;
          const bool guarded = shield_registered && f.front;
          f.hit[0] = true, f.shield[0] = guarded;
          tg[f.cyl] = key(i), tg_shield[f.cyl] = guarded;
        }
      }
    }
    out.proc.push_back(proc);
    out.posture.push_back(lp.milli());
    out.phase.push_back(static_cast<int>(def.phase()));
    out.shield_reg.push_back(shield_registered ? 1 : 0);
    out.flag.push_back(flag ? 1 : 0);
  }
  out.dc = def.c, out.bc = bm.c, out.pc = lp.c, out.parry = p.c, out.conserved = lp.conserved(), out.life_end = life;
  return out;
}
static void dump(const char* tag, const Out& o) {
  if (!std::getenv("FX_DUMP")) return;
  auto v = [](const std::vector<int>& x) { std::string r; for (int i : x) r += std::to_string(i) + ","; return r; };
  std::printf("DUMP %s mod=%d nat=%d begins=%llu ext=%llu completes=%llu rearmed=%llu aborted=%llu resets=%llu blocks=%d damage=%d enter=[%s] exit=[%s] complete=[%s] rearm=[%s] guard_ticks=%d bonus=%d\n",
              tag, o.mod_breaks, o.native_breaks, (unsigned long long)o.dc.begins, (unsigned long long)o.dc.begins_extended, (unsigned long long)o.dc.completes,
              (unsigned long long)o.dc.rearmed, (unsigned long long)o.dc.aborted, (unsigned long long)o.pc.break_resets, o.blocks, o.damage, v(o.enter_ticks).c_str(),
              v(o.exit_ticks).c_str(), v(o.complete_ticks).c_str(), v(o.rearm_ticks).c_str(), o.guard_ticks, o.bonus_ticks);
}
static int first_index(const std::vector<int>& v, int x, int from = 0) {
  for (size_t i = static_cast<size_t>(from); i < v.size(); ++i)
    if (v[i] == x) return static_cast<int>(i);
  return -1;
}

int main() {
  using lnkfx::policy::kTotalPenaltyTicks;
  // ================= F01 machine: phases, clock, latch, aborts =====================================================================================
  fam = 1;
  {
    lnkfx::Defense d;
    ck(d.phase() == Phase::Ready && !d.guard_blocked() && !d.dodge_blocked(), "READY by default");
    const auto b = d.begin(bg(10));
    ck(b.ev == lnkfx::Ev::EnterNative && d.phase() == Phase::NativeBreak && d.guard_blocked() && d.dodge_blocked() && d.bash_blocked() && d.request_blocked(),
       "confirmed native entry: everything defensive blocked at once");
    ck(d.begin(bg(11)).ev == lnkfx::Ev::DuplicateBegin && d.c.duplicate_begins == 1, "no queue, no clock restart");
    std::uint64_t t = 10;
    for (int i = 0; i < 30; ++i) d.observe(ob(++t, 0x1f));
    const auto x = d.observe(ob(++t, 6));
    ck(x.ev == lnkfx::Ev::PostVulnerable && d.phase() == Phase::PostVulnerable && d.active_ticks() == 31, "native exit before the budget => POST_BREAK_VULNERABLE");
    ck(d.dodge_blocked() && d.guard_blocked(), "restrictions continue after the animation");
    for (int n_ = 0; (d.phase() == Phase::PostVulnerable) && bounded(n_);) d.observe(ob(++t, 6));
    ck(d.phase() == Phase::RearmRequired && t == 10 + kTotalPenaltyTicks && d.c.completes == 1 && d.c.window_min == kTotalPenaltyTicks,
       "budget = 75 ACTIVE ticks from the native entry");
    ck(!d.dodge_blocked() && d.guard_blocked() && d.bash_blocked() && !d.request_blocked(), "after the budget: dodges back, shield waits for a new R");
    ck(d.sample(7, 1, true, ++t).ev == lnkfx::Ev::None && d.phase() == Phase::RearmRequired, "R held through the unlock: NOT rearmed");
    ck(d.sample(7, 1, false, ++t).ev == lnkfx::Ev::None, "release");
    ck(d.sample(7, 1, true, ++t).ev == lnkfx::Ev::Rearmed && d.phase() == Phase::Ready && !d.guard_blocked(), "release then press => rearmed");
  }
  {
    lnkfx::Defense d;  // R pressed DURING the penalty and held => not rearmed
    d.sample(7, 1, false, 9);
    d.begin(bg(10));
    std::uint64_t t = 10;
    for (int i = 0; i < 40; ++i) d.sample(7, 1, i > 20, t), d.observe(ob(++t, i < 30 ? 0x1f : 6));
    for (int n_ = 0; (d.phase() != Phase::RearmRequired) && bounded(n_);) d.sample(7, 1, true, t), d.observe(ob(++t, 6));
    for (int i = 0; i < 20; ++i) d.sample(7, 1, true, ++t);
    ck(d.phase() == Phase::RearmRequired && d.c.held_through_unlock == 1, "pressed mid-penalty and held: still REARM_REQUIRED");
    d.sample(7, 1, false, ++t);
    ck(d.sample(7, 1, true, ++t).ev == lnkfx::Ev::Rearmed, "new press after release: rearmed");
  }
  {
    lnkfx::Defense d;  // R toggled during the window: no hidden rearm
    d.begin(bg(10));
    std::uint64_t t = 10;
    bool r = false;
    for (int n_ = 0; (d.phase() != Phase::RearmRequired) && bounded(n_);) {
      r = !r, d.sample(7, 1, r, t);
      ++t;
      d.observe(ob(t, t < 40 ? 0x1f : 6));
    }
    ck(d.phase() == Phase::RearmRequired, "toggling R during the window never rearms");
    if (r) d.sample(7, 1, false, ++t);
    ck(d.sample(7, 1, true, ++t).ev == lnkfx::Ev::Rearmed, "R released at the unlock: one new press rearms");
  }
  {
    lnkfx::Defense d;  // pause freezes the clock
    d.begin(bg(10));
    std::uint64_t t = 10;
    for (int i = 0; i < 20; ++i) d.observe(ob(++t, 0x1f));
    for (int i = 0; i < 500; ++i) d.observe(ob(++t, 0x1f, false));
    ck(d.phase() == Phase::NativeBreak && d.active_ticks() == 20 && d.c.paused_ticks == 500, "pause/menu: nothing advances");
    for (int i = 0; i < 9; ++i) d.observe(ob(++t, 0x1f));
    for (int n_ = 0; (d.phase() == Phase::PostVulnerable || d.phase() == Phase::NativeBreak) && bounded(n_);) d.observe(ob(++t, 6));
    ck(d.c.window_max == kTotalPenaltyTicks, "same active budget after a long pause");
  }
  {
    lnkfx::Defense d;  // native animation longer than the budget: end = max(native, budget)
    d.begin(bg(10));
    std::uint64_t t = 10;
    for (int i = 0; i < 100; ++i) d.observe(ob(++t, 0x1f));
    ck(d.phase() == Phase::NativeBreak && d.guard_blocked(), "still in the native break past 75: guard stays blocked");
    const auto e = d.observe(ob(++t, 6));
    ck(e.ev == lnkfx::Ev::NativeExit && d.phase() == Phase::RearmRequired, "exit after the budget: straight to rearm");
    lnkfx::Defense w;
    w.begin(bg(10));
    for (std::uint64_t i = 0; i < lnkfx::policy::kNativeWatchdogTicks; ++i) w.observe(ob(11 + i, 0x1f));
    ck(w.c.watchdog == 1 && w.phase() == Phase::NativeBreak, "watchdog is diagnostic only (never forces an exit)");
  }
  {
    lnkfx::Defense d;  // latch only (native break without saturation)
    d.begin(bg(10, false, lnkfx::Origin::NativeObserved));
    ck(!d.dodge_blocked() && d.guard_blocked(), "latch-only break: no dodge veto");
    std::uint64_t t = 10;
    for (int i = 0; i < 30; ++i) d.observe(ob(++t, 0x1f));
    ck(d.observe(ob(++t, 6)).ev == lnkfx::Ev::NativeExit && d.phase() == Phase::RearmRequired && d.c.completes == 0, "no 2.5 s extension without saturation");
  }
  for (int k = 0; k < 6; ++k) {  // aborts: never a permanently disabled shield
    lnkfx::Defense d;
    d.begin(bg(10));
    std::uint64_t t = 10;
    for (int i = 0; i < 40; ++i) d.observe(ob(++t, i < 30 ? 0x1f : 6));
    lnkfx::Obs o = ob(++t, 6);
    if (k == 0) o.alive = false;
    if (k == 1) o.human = false;
    if (k == 2) o.link_id = 8;
    if (k == 3) o.epoch = 2;
    if (k == 4) o.event = true, o.sim_active = false;
    if (k == 5) o.valid = false;
    ck(d.observe(o).ev == lnkfx::Ev::Aborted && d.phase() == Phase::Ready && !d.guard_blocked() && !d.need_release(), "death/wolf/new Link/event/absent: clean abort");
  }
  {
    lnkfx::Defense d;  // new break while waiting for the rearm: a fresh full cycle
    d.begin(bg(10));
    std::uint64_t t = 10;
    for (int n_ = 0; (d.phase() != Phase::RearmRequired) && bounded(n_);) {
      ++t;
      d.observe(ob(t, t < 40 ? 0x1f : 6));
    }
    ck(d.begin(bg(t)).ev == lnkfx::Ev::EnterNative && d.phase() == Phase::NativeBreak && d.active_ticks() == 0, "second cycle after the first completes");
    d.shutdown();
    ck(d.phase() == Phase::Ready && !d.guard_blocked(), "shutdown/reload leaves nothing blocked");
  }

  // ================= F02 decisions =================================================================================================================
  fam = 2;
  {
    using lnkfx::Guard;
    for (int ph = 0; ph < 4; ++ph)
      for (int r = 0; r < 2; ++r)
        for (int m = 0; m < 2; ++m)
          for (int f = 0; f < 2; ++f) {
            const Guard g = lnkfx::decide_guard(f, m, r, static_cast<Phase>(ph));
            if (!f) ck(g == Guard::NativeOff && !lnkfx::clears_flag(g), "never switches the guard ON");
            else if (m) ck(g == Guard::Mandatory && !lnkfx::clears_flag(g), "GUARD_SLIP / small guard never touched");
            else if (ph == 1 || ph == 2) ck(g == Guard::DeniedPenalty && lnkfx::clears_flag(g), "no guard during the penalty even with R");
            else if (ph == 3) ck(g == Guard::DeniedRearm && lnkfx::clears_flag(g), "no guard until the rearm");
            else ck((g == Guard::Accepted) == (r == 1) && lnkfx::clears_flag(g) == (r == 0), "READY: lock-on guard only with R");
          }
    for (int ph = 0; ph < 4; ++ph)
      for (int r = 0; r < 2; ++r)
        for (int b = 0; b < 2; ++b)
          for (int o = 0; o < 2; ++o) {
            ck(lnkfx::consume_b(b, r, static_cast<Phase>(ph), o) == (b && r && ph == 0 && o), "B consumed only for an offered R+B Shield Attack in READY");
            const lnkfx::Bash x = lnkfx::decide_bash(r, b, static_cast<Phase>(ph));
            ck((x == lnkfx::Bash::Accepted) == (b && r && ph == 0), "Shield Attack only with R+B in READY");
            if (b && ph != 0) ck(x == lnkfx::Bash::DeniedPenalty, "R+B during the penalty: denied");
          }
    ck(lnkfx::classify_dodge(false, 0x19, 0, false) == lnkfx::Dodge::None, "no A press: nothing");
    ck(lnkfx::classify_dodge(true, 0x19, static_cast<std::int16_t>(0x8000), false) == lnkfx::Dodge::Backflip, "back + JUMP = backflip");
    ck(lnkfx::classify_dodge(true, 0x19, 0x4000, false) == lnkfx::Dodge::Sidehop, "side + JUMP = sidehop");
    ck(lnkfx::classify_dodge(true, 0x19, 0x4000, true) == lnkfx::Dodge::SideRollCandidate, "side roll armed");
    ck(lnkfx::classify_dodge(true, 0x79, 0, false) == lnkfx::Dodge::FrontRoll, "0x79 = front roll");
    for (unsigned st : {0u, 0x09u, 0x1au, 0x22u, 0x3au, 0x78u, 0x7au}) ck(lnkfx::classify_dodge(true, st, 0, false) == lnkfx::Dodge::None, "other A actions untouched");
    for (int ph = 0; ph < 4; ++ph)
      for (int n = 0; n < 2; ++n)
        for (int e = 0; e < 2; ++e)
          for (int a = 0; a < 2; ++a)
            ck(lnkfx::regen_bonus(n, e, a, static_cast<Phase>(ph)) == (n && (!e || (a && ph == 0))), "x1.5 = native safe guard AND (0.11.0 rule | accepted manual guard in READY)");
    ck(lnkfx::backward(0x6001) && !lnkfx::backward(0x6000) && lnkfx::backward(-0x6001) && !lnkfx::backward(0), "getDirectionFromAngle replica");
  }

  // ================= F03 manual shield (A1-A9) =======================================================================================================
  fam = 3;
  {
    Sim s;  // A1: lock without R => no shield; blocks impossible
    s.ticks = 200, s.input = [](int) { return In{true, false, false, false, 0}; };
    const Out o = run(s);
    ck(o.guard_ticks == 0 && o.blocks == 0 && o.damage >= 1, "A1 L without R: no shield, hits land natively");
    ck(std::count(o.shield_reg.begin(), o.shield_reg.end(), 1) == 0, "A1 no Tg shield registered");
    Sim n = s;
    n.fx = false;
    ck(run(n).blocks >= 1, "control: 0.11.0 (FIX01 off) L alone raises the shield");
  }
  {
    Sim s;  // A2: L+R => real guard, blocks (window shorter than five blocks: no break yet)
    s.ticks = 120, s.input = [](int) { return In{true, true, false, false, 0}; };
    const Out o = run(s);
    dump("A2", o);
    ck(o.guard_ticks > 80 && o.blocks >= 3 && o.damage == 0, "A2 L+R: native guard (GUARD_SLIP ticks are native, not manual), hits blocked");
    Sim r = s;  // A3: release R => guard down at once
    r.input = [](int t) { return In{true, t < 60, false, false, 0}; };
    const Out q = run(r);
    ck(q.flag[70] == 0 && q.shield_reg[70] == 0, "A3 release R: no residual flag/shield");
  }
  {
    Sim s;  // A4/A5: R+B => ONE Shield Attack, no sword; B alone => sword
    s.ticks = 60, s.foes[0].reach = false;
    s.input = [](int t) { return In{true, true, t == 20 || t == 21 || t == 22, false, 0}; };
    const Out o = run(s);
    ck(o.bashes == 1 && o.cuts == 0 && o.b_consumed >= 1, "A4 R+B: one Shield Attack, B consumed, no extra sword");
    Sim h = s;
    h.input = [](int t) { return In{true, true, t == 20 || t == 25 || t == 30, false, 0}; };  // B pressed again during the Shield Attack
    const Out hh = run(h);
    ck(hh.bashes == 1 && hh.cuts == 0, "B pressed again during the Shield Attack: no second bash, no sword");
    Sim b = s;
    b.input = [](int t) { return In{true, false, t == 20, false, 0}; };
    const Out c = run(b);
    ck(c.cuts == 1 && c.bashes == 0, "A5 B without R: normal sword");
    Sim u = s;  // Shield Attack not learned: B is NOT eaten (the game never offered the Shield Attack)
    u.learned = false;
    u.input = [](int t) { return In{true, true, t == 20, false, 0}; };
    const Out v = run(u);
    ck(v.cuts == 1 && v.bashes == 0 && v.b_consumed == 0, "R+B without the Shield Attack skill: B stays a sword attack");
    Sim nl = s;  // A6: R without lock-on: no shield, no crouch shield
    nl.input = [](int t) { return In{false, true, t == 20, false, 0}; };
    const Out w = run(nl);
    ck(w.guard_ticks == 0 && w.bashes == 0 && w.cuts == 1, "A6 R without target: no guard, B = sword");
    Sim ns = s;  // A7: no shield equipped
    ns.has_shield = false;
    ns.input = [](int t) { return In{true, true, t == 20, false, 0}; };
    const Out x = run(ns);
    ck(x.guard_ticks == 0 && x.bashes == 0 && x.b_consumed == 0 && x.cuts == 1, "A7 no shield: no guard, B untouched");
    Sim nf = s;  // FIX01 off: native R trigger bash
    nf.fx = false;
    nf.input = [](int t) { return In{true, t >= 20, false, false, 0}; };
    ck(run(nf).bashes == 1, "control: 0.11.0 R alone = Shield Attack");
  }
  {
    Sim s;  // A4 + POST08: a real parry through R+B
    s.ticks = 40;
    s.input = [](int t) { return In{true, true, t == 13, false, 0}; };
    const Out o = run(s);
    ck(o.bashes == 1 && o.parry.confirmed == 1, "R+B Shield Attack still produces a POST08 parry");
    ck(*std::max_element(o.posture.begin(), o.posture.end()) == 0, "parry: Link +0");
  }

  // ================= F04 break + penalty (B1-B16) ===================================================================================================
  fam = 4;
  for (bool lf : {true, false})
    for (bool hold_r : {true, false}) {
      Sim s;  // one cycle (a second break would need four more confirmed direct hits)
      s.ticks = 140, s.link_first = lf, s.start_blocks = 4;
      s.input = [hold_r](int t) { return In{true, hold_r || t < 30, false, false, 0}; };
      const Out o = run(s);
      ck(o.mod_breaks == 1 && o.enter_ticks.size() == 1, "B1 one native break for the saturating block");
      const int e = o.enter_ticks.empty() ? 0 : o.enter_ticks[0];
      const int bstart = first_index(o.proc, kBreak), bend = bstart >= 0 ? first_index(o.proc, kAtnWait, bstart) : -1;
      ck(bstart >= 0 && bend - bstart >= 28 && bend - bstart <= 35, "B4 native animation 28..35 frames at speed 1.0 (no slow-down)");
      ck(o.pc.break_resets == 1 && !o.reset_in_break, "B5 posture reset ONCE at the native exit");
      ck(o.posture[static_cast<size_t>(bend + 2)] == 0 && o.phase[static_cast<size_t>(bend + 2)] == static_cast<int>(Phase::PostVulnerable),
         "B5 posture already 0 while the penalty continues");
      ck(!o.complete_ticks.empty() && o.complete_ticks.back() - e == static_cast<int>(kTotalPenaltyTicks), "B6 window = 75 active ticks from the native entry");
      ck(o.shield_during_block == 0 && o.bash_during_block == 0, "B6/B7 no shield and no Shield Attack during the penalty");
      if (hold_r) ck(o.rearm_ticks.empty() && o.guard_ticks < e, "B7 R held all along: the shield never comes back");
      else ck(!o.rearm_ticks.empty() || o.guard_ticks < e, "R released: rearm needs a new press");
      ck(o.dc.completes == 1 && o.dc.window_min == kTotalPenaltyTicks, "one complete penalty");
    }
  {
    Sim s;  // B7/B8: release+press after the unlock rearms; press mid-penalty and hold does not (the Blue attacks once)
    s.ticks = 300, s.start_blocks = 4, s.foes[0].cooldown = 400;
    s.input = [](int t) { return In{true, t < 30 || (t >= 60 && t < 200) || t >= 220, false, false, 0}; };
    const Out o = run(s);
    dump("B8", o);
    const int e = o.enter_ticks.empty() ? 0 : o.enter_ticks[0];
    ck(e > 0 && e + static_cast<int>(kTotalPenaltyTicks) < 200, "setup: unlock happens while R is held (pressed mid-penalty)");
    ck(!o.rearm_ticks.empty() && o.rearm_ticks[0] >= 220, "B8 pressed mid-penalty and held: rearm only after release (199) and new press (220)");
    ck(o.guard_ticks > 0 && o.shield_reg[static_cast<size_t>(o.rearm_ticks.empty() ? 0 : o.rearm_ticks[0] + 1)] == 1, "B7 the new press really raises the shield");
  }
  {
    Sim s;  // B9: walking/attacking allowed; R+B during the penalty: no Shield Attack, B still a sword
    s.ticks = 200, s.start_blocks = 4;
    s.input = [](int t) { return In{true, true, t == 80 || t == 100, false, t > 70 ? 1 : 0}; };
    const Out o = run(s);
    ck(o.bashes == 0 && o.cuts >= 1 && o.cut_during_penalty >= 1, "A8/B9 R+B in the penalty: never Shield Attack, sword still legal");
    ck(o.moves_during_penalty > 0, "B9 locomotion keeps working");
  }
  {
    Sim s;  // B11/B12: hit during the post-vulnerability = native damage; no new break, no clock restart (measured over the window)
    s.ticks = 100, s.start_blocks = 4;
    s.foes[0].cooldown = 2;
    s.input = [](int) { return In{true, true, false, false, 0}; };
    const Out o = run(s);
    ck(o.damage_in_post >= 1, "B11 a lone Blue can land a hit during the post-break window");
    ck(o.mod_breaks == 1 && o.dc.duplicate_begins == 0, "B12 no cascading break");
    ck(o.dc.completes == 1 && o.dc.window_max == kTotalPenaltyTicks, "B12 the clock is not restarted by hits");
  }
  {
    Sim s;  // B13: 4th native slip coincides with the saturation (Blue 20 + Red 4 x 25) => ONE break, the native one, adopted and extended
    s.ticks = 200, s.start_blocks = 1, s.foes[0].e_oc2 = true, s.foes[0].atp = 2;
    s.input = [](int) { return In{true, true, false, false, 0}; };
    const Out o = run(s);
    dump("B13", o);
    ck(o.native_breaks == 1 && o.mod_breaks == 0 && o.dc.begins_extended == 1 && o.bc.native_adopted == 1, "B13 4th slip at 100: one adopted break, extended penalty");
    ck(o.dc.begins == static_cast<std::uint64_t>(o.mod_breaks + o.native_breaks), "B13 each physical break begins one penalty");
    Sim n = s;  // B14: native break without saturation => latch only
    n.start_blocks = 0, n.foes[0].reach = false, n.native_break_at = 30;
    n.input = [](int t) { return In{true, true, false, t == 60, 3}; };
    const Out q = run(n);
    ck(q.dc.begins_latch_only == 1 && q.dc.completes == 0 && q.dodges == 1 && q.dodges_denied == 0, "B14 native break w/o saturation: no 2.5 s extension, dodges free");
    ck(q.rearm_ticks.empty() && q.guard_ticks < 40, "B14 but the shield still needs a new press");
  }
  {
    Sim s;  // B15: full bar without a new impact => no break
    s.ticks = 200, s.start_blocks = 5, s.foes[0].reach = false;
    s.input = [](int) { return In{true, true, false, false, 0}; };
    const Out o = run(s);
    ck(o.mod_breaks == 0 && o.dc.begins == 0, "B15 no spontaneous break at 100");
  }
  {
    Sim s;  // B16: three cycles, rearming each time
    s.ticks = 1200, s.start_blocks = 4;
    s.input = [](int t) { const int c = t % 300; return In{true, c < 240 || c > 250, false, false, 0}; };
    const Out o = run(s);
    ck(o.mod_breaks + o.native_breaks >= 2 && o.dc.completes >= 2 && o.dc.window_min == kTotalPenaltyTicks && o.dc.window_max == kTotalPenaltyTicks,
       "B16 repeated cycles behave the same");
    ck(o.conserved && o.shield_during_block == 0, "B16 conservation, no shield leak");
  }

  // ================= F05 dodges (C1-C4) =============================================================================================================
  fam = 5;
  for (int stick : {1, 2, 3}) {
    Sim s;
    s.ticks = 260, s.start_blocks = 4;
    s.input = [stick](int t) {
      In i{stick != 1, true, false, false, stick};
      if (stick == 1) i.r = false;
      i.a = t == 2 || (t > 40 && t % 7 == 0);
      return i;
    };
    if (stick == 1) s.input = [](int t) { return In{t < 20, t < 20, false, t == 2 || (t > 40 && t % 7 == 0), t > 25 ? 1 : 0}; };
    const Out o = run(s);
    ck(o.dodges >= 1, "C1 dodges work before the break");
    ck(o.dodge_start_during_block == 0, "C2 no voluntary dodge starts during the penalty");
    if (o.mod_breaks == 1) ck(o.dodges_denied >= 1, "C2 attempts were vetoed at admission");
    const int e = o.enter_ticks.empty() ? 1000 : o.enter_ticks[0];
    bool after = false;
    for (size_t i = static_cast<size_t>(std::min<int>(e + static_cast<int>(kTotalPenaltyTicks) + 1, static_cast<int>(o.proc.size()))); i < o.proc.size(); ++i)
      after = after || dodge_proc(o.proc[i]);
    if (o.mod_breaks == 1) ck(after, "C1 dodges come back after the 75 ticks");
  }
  {
    Sim s;  // C3: a forced native roll during the penalty is not vetoed
    s.ticks = 200, s.start_blocks = 4, s.forced_roll_at = 80;
    s.input = [](int) { return In{true, true, false, false, 0}; };
    const Out o = run(s);
    ck(o.forced_rolls == 1 && o.proc[80] == kFrontRoll, "C3 forced native transitions pass");
  }

  // ================= F06 regeneration (D1-D7) ========================================================================================================
  fam = 6;
  {
    auto regen = [](std::function<In(int)> f, bool fx = true) {
      Sim s;
      s.ticks = 300, s.start_blocks = 2, s.foes[0].reach = false, s.input = f, s.fx = fx;
      return run(s);
    };
    const Out a = regen([](int) { return In{true, true, false, false, 0}; });
    ck(a.regen_guard_ticks > 0 && a.bonus_ticks > 0, "D1 R + real guard => x1.5");
    const Out b = regen([](int) { return In{true, false, false, false, 0}; });
    ck(b.regen_guard_ticks == 0 && b.bonus_ticks == 0, "D2 L without R => x1");
    const Out c = regen([](int) { return In{false, true, false, false, 0}; });
    ck(c.regen_guard_ticks == 0, "D3 R without lock-on (no real guard) => x1");
    Sim ns;
    ns.ticks = 300, ns.start_blocks = 2, ns.foes[0].reach = false, ns.has_shield = false, ns.input = [](int) { return In{true, true, false, false, 0}; };
    ck(run(ns).regen_guard_ticks == 0, "D3 R without shield => x1");
    const Out n = regen([](int) { return In{true, false, false, false, 0}; }, false);
    ck(n.regen_guard_ticks > 0, "control: 0.11.0 L-guard gave x1.5 (now requires R)");
    Sim s;
    s.ticks = 400, s.start_blocks = 4, s.foes[0].cooldown = 400;
    s.input = [](int t) { return In{true, t < 200 || t > 210, false, false, 0}; };
    const Out d = run(s);
    dump("D", d);
    ck(d.bonus_while_blocked == 0, "D4/D5 no x1.5 during the break or the penalty");
    ck(d.pc.break_resets == 1, "D4 one reset");
    ck(d.bonus_ticks > 0 && !d.rearm_ticks.empty(), "D6 after the new R press the bonus can return");
    ck(lnk01::policy::kBandPointsPerSecond[0] == 10 && lnk01::policy::kBandPointsPerSecond[1] == 8 && lnk01::policy::kBandPointsPerSecond[2] == 5 &&
           lnk01::policy::kBandPointsPerSecond[3] == 2 && lnk01::policy::kGuardMulNum == 3 && lnk01::policy::kGuardMulDen == 2 && lnk01::policy::kRegenDelayTicks == 30,
       "D7 10/8/5/2, x1.5, delay 30 unchanged");
  }

  // ================= F07 lifecycle (E1-E4) ===========================================================================================================
  fam = 7;
  {
    Sim s;
    s.ticks = 400, s.start_blocks = 4, s.pause_from = 60, s.pause_to = 160;
    s.input = [](int) { return In{true, true, false, false, 0}; };
    const Out o = run(s);
    dump("E1", o);
    const int e = o.enter_ticks.empty() ? 0 : o.enter_ticks[0];
    ck(!o.complete_ticks.empty() && o.complete_ticks[0] - e == static_cast<int>(kTotalPenaltyTicks) + 100, "E1 a 100-tick pause shifts the end by exactly 100");
    for (int k = 0; k < 4; ++k) {
      Sim a;
      a.ticks = 150, a.start_blocks = 4;
      a.input = [](int) { return In{true, true, false, false, 0}; };
      if (k == 0) a.death_at = 70;
      if (k == 1) a.wolf_from = 70;
      if (k == 2) a.link_change_at = 70;
      if (k == 3) a.event_from = 70, a.event_to = 90;
      const Out q = run(a);
      ck(q.dc.aborted == 1 && q.phase.back() == static_cast<int>(Phase::Ready), "E2 death/wolf/new Link/event: aborted, nothing left blocked");
    }
  }

  // ================= F08 sweep: invariants over orders, inputs, foes, specials ======================================================================
  fam = 8;
  {
    unsigned runs = 0, broke = 0, rearmed = 0, denied = 0;
    std::uint32_t seed = 0x2f1u;  // reproducible: fixed seeds, fixed counts
    auto rnd = [&seed](unsigned n) { seed = seed * 1664525u + 1013904223u; return (seed >> 8) % n; };
    for (std::uint32_t s0 : {0x2f1u, 0x9e37u, 0x51edu, 0xc0deu})
    for (int blocks = (seed = s0, 2); blocks <= 5; ++blocks)
      for (bool lf : {true, false})
        for (int atp : {1, 2, 3, 6})
          for (int group = 0; group < 2; ++group)
            for (int special = 0; special < 6; ++special)
              for (int pattern = 0; pattern < 6; ++pattern) {
                Sim s;
                s.ticks = 320, s.link_first = lf, s.start_blocks = blocks;
                s.foes[0].e_oc2 = atp != 1, s.foes[0].atp = atp, s.foes[0].cooldown = 2 + static_cast<int>(rnd(10));
                if (group) {
                  Foe b;
                  b.id = 101, b.first_delay = 5 + static_cast<int>(rnd(9)), b.cyl = 1, b.e_oc2 = true, b.atp = 2, b.cooldown = 3 + static_cast<int>(rnd(8));
                  s.foes.push_back(b);
                }
                const std::uint32_t ps = seed;
                s.input = [pattern, ps](int t) {
                  std::uint32_t h = ps ^ static_cast<std::uint32_t>(t * 2654435761u);
                  h ^= h >> 13, h *= 0x5bd1e995u, h ^= h >> 15;
                  In i;
                  i.lock = pattern != 5 || (t / 40) % 2 == 0;
                  i.r = pattern == 0 ? true : pattern == 1 ? (t / 25) % 2 == 0 : pattern == 2 ? (h & 7) != 0 : pattern == 3 ? t < 50 || t > 150 : (h & 3) != 0;
                  i.b = (h & 0x70) == 0x70;
                  i.a = (h & 0x380) == 0x380;
                  i.stick = static_cast<int>((h >> 12) & 3);
                  return i;
                };
                if (special == 1) s.pause_from = 40 + static_cast<int>(rnd(60)), s.pause_to = s.pause_from + 30;
                if (special == 2) s.event_from = 80 + static_cast<int>(rnd(60)), s.event_to = s.event_from + 15;
                if (special == 3) s.death_at = 100 + static_cast<int>(rnd(100));
                if (special == 4) s.link_change_at = 90 + static_cast<int>(rnd(100));
                if (special == 5) s.forced_roll_at = 60 + static_cast<int>(rnd(60));
                const Out o = run(s);
                ++runs;
                broke += o.dc.begins ? 1u : 0u, rearmed += o.dc.rearmed ? 1u : 0u, denied += o.dodges_denied ? 1u : 0u;
                ck(o.shield_during_block == 0, "S1 no Tg shield while the defence is blocked (incl. the native exit frame)");
                ck(o.bash_during_block == 0, "S2 no Shield Attack / parry during penalty or rearm");
                ck(o.dodge_start_during_block == 0, "S3 no voluntary dodge during the extended penalty");
                ck(o.bonus_while_blocked == 0, "S4 no x1.5 while blocked");
                ck(o.dc.completes == 0 || (o.dc.window_min >= kTotalPenaltyTicks && o.dc.window_max >= kTotalPenaltyTicks), "S5 window never shorter than 75");
                ck(o.dc.window_max <= kTotalPenaltyTicks + 40, "S6 window = max(native, 75): never a runaway");
                ck(o.pc.break_resets == static_cast<std::uint64_t>(o.bc.resets) && !o.reset_in_break, "S7 exactly one reset per closed break, never during it");
                ck(o.conserved, "S8 posture conservation");
                ck(o.dc.begins <= static_cast<std::uint64_t>(o.mod_breaks + o.native_breaks), "S9 a penalty only from a physical break");
                ck(o.dc.duplicate_begins == 0, "S10 no double break for one impact");
                ck(o.mod_breaks <= static_cast<int>(o.bc.requested), "S11 each mod break had a request");
                if (special == 5) ck(o.forced_rolls == 1, "S12 forced transitions never vetoed");
              }
    std::printf("F08 sweep runs=%u with_break=%u rearmed=%u dodge_denied_runs=%u\n", runs, broke, rearmed, denied);
    ck(broke > runs / 3 && rearmed > runs / 10 && denied > runs / 20, "the sweep really exercises break, rearm and veto");
  }
  // ================= F09 constants / format ===========================================================================================================
  fam = 9;
  {
    ck(std::strcmp(lnkfx::kVersion, "0.11.1") == 0 && kTotalPenaltyTicks == 75 && lnkfx::policy::kManualShield && lnkfx::policy::kExtendedPenalty, "version and balance");
    ck(lnk05::policy::kCancelFrame == 28.0f && lnk05::policy::kEndFrame == 35.0f, "native animation untouched (cancel 28 / end 35)");
    char line[1024];
    lnkfx::Out x;
    x.link_id = 4294967295u, x.epoch = x.entry_tick = x.tick = x.active_ticks = x.native_ticks = x.remaining = 18446744073709551615ull;
    x.reason = "LINK_CHANGED_OR_ABSENT";
    ck(lnkfx::fmt_penalty(line, sizeof line, "LNK_FIX01_PENALTY", x) < 600, "bounded log line");
    ck(sizeof(lnkfx::Defense) < 512, "small bounded state");
  }
  for (int i = 1; i <= 9; ++i) std::printf("F%02d checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("LNK_S2_FIX01_LAB checks=%u failures=%u RESULT=%s LAB_MODEL_ONLY\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
