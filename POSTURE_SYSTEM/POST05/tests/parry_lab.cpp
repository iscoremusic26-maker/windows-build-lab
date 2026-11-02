// POST08 0.8.0 parry laboratory: pure cores only (parry_core + posture_core + break_core + hud_core). It cannot prove the game's timing, collision or
// Link protection (HUMAN_PENDING); it proves the decision rules, the bookkeeping and the hand-off. Families T01..T42 of POSTURE_SYSTEM/POST08/TEST_PLAN.md.
#include "../src/break_core.hpp"
#include "../src/hud_core.hpp"
#include "../src/parry_core.hpp"
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>
#include <algorithm>

using post08::Input;
using post08::Parry;
using post08::Result;
static unsigned checks = 0, failures = 0, fam = 0, fam_checks[43] = {}, fam_fail[43] = {};
static void ck(bool b, const char* what) {
  ++checks;
  ++fam_checks[fam];
  if (!b) {
    ++failures;
    if (++fam_fail[fam] <= 3) std::printf("FAIL T%02u %s\n", fam, what);
  }
}

// ---- inputs -------------------------------------------------------------------------------------------------------------------
static Input swing(int slot = 0, std::uint32_t id = 100, std::uint64_t gen = 1, int state = 1, float frame = 15) {
  Input in;
  in.tracked = true, in.slot = slot, in.id = id, in.generation = gen, in.hp = 200, in.mode = 4, in.state = state, in.anim_ok = true, in.anim_frame = frame, in.anim_rate = 1.0f;
  in.link_ok = true;
  in.fresh = true;  // contactos estaticos: registro de una pasada armada (la frescura real se prueba en run_sim2)
  return in;
}
// Link en la PASADA y AHORA con el mismo estado (casos estaticos); la separacion temporal real se prueba en la simulacion por fases (P01..P12).
static post08::LinkView lv(bool ga, float frame) {
  post08::LinkView v;
  v.valid = true, v.guard_attack = ga, v.proc = ga ? 0x1e : 0, v.frame = frame, v.start = 1.0f, v.end = 12.0f, v.serial = ga ? 1 : 0;
  v.age_ticks = ga && frame > 1.0f ? static_cast<std::uint32_t>(frame - 1.0f) : 0;
  return v;
}
static Input with_link(Input in, bool ga, float frame) {
  in.pass = in.now = lv(ga, frame);
  return in;
}
static Input blocked(Input in, float link_frame, int sphere = 0, bool guard_attack = true) {
  in.at_hit[sphere] = in.at_on_link[sphere] = in.at_shield[sphere] = true;
  return with_link(in, guard_attack, link_frame);
}

// ---- posture engine with N admitted E_OC actors -------------------------------------------------------------------------------
struct World {
  post04::PostureEngine pe;
  std::vector<std::uint64_t> gens;
  std::uint64_t event = 0;
  explicit World(unsigned n = 1) {
    for (unsigned i = 0; i < n; ++i) {
      post03::Events ev;
      post03::PreReads r;
      r.facts.profile_ok = r.facts.live = r.facts.status_10000 = true;
      r.facts.name = post03::NameKind::E_OC;
      r.facts.atp = 1;
      pe.on_pre(0x10000 + 0x2000 * i, 100 + i, r, ev);
      pe.px.sync(pe.eng.obs.ledger);
      gens.push_back(pe.eng.obs.ledger.slot(i).generation);
    }
  }
  const post03::Ledger& L() const { return pe.eng.obs.ledger; }
  const post04::Entry& e(unsigned i) const { return pe.px.entry(i); }
  bool hit(unsigned i, post03::Family f) {
    post03::HitReport h;
    h.slot = static_cast<int>(i), h.id = 100 + i, h.generation = gens[i], h.life = post03::Life::Active;
    h.j.kind = post03::HitKind::Confirmed, h.j.elig = post03::Eligibility::Confirmed, h.at_valid = true, h.fam.family = f, h.post.health = 100;
    h.event = ++event;
    return pe.px.consume(h, L()).crossed;
  }
  post04::Posture::ParryOutcome parry(unsigned i, std::uint64_t token) { return pe.px.apply_parry(static_cast<int>(i), gens[i], 100 + i, token, L()); }
  void ticks(unsigned n) {
    for (unsigned k = 0; k < n; ++k) pe.on_update();
  }
  bool audit_ok() { return pe.px.audit().ok; }
};

// ================= FIX01: simulacion TEMPORAL por fases de un tick (orden real del host, ver IMPLEMENTATION) =======================================
//  1) mod_update: el rastreador observa a Link (estado tras su execute del tick anterior = estado en la pasada anterior)
//  2) execute de actores: Link y el Bokoblin en el orden indicado. Link procesa el bloqueo de la pasada anterior -> GUARD_SLIP (procGuardSlipInit).
//     El Bokoblin: PRE de damage_check (marcas de la pasada anterior) -> juicio -> si CONFIRMED el adaptador lo desvia (estado 3, sin esferas At).
//  3) pasada de colision: esfera At activa del Bokoblin (frames 14..22 de su clip) contra Link: bloqueada si Link tiene escudo (GUARD_ATTACK/guardia) y
//     de frente; si no, golpe con dano.
enum LinkProc { kIdle = 0, kGuard = 1, kGuardAttack = 0x1e, kGuardSlip = 2, kDamaged = 3 };
struct SimParams {
  bool link_first = true;      // Link ejecuta antes que el Bokoblin (lo que muestran los 74 NORMAL_GUARD de 0.8.0)
  int press_tick = -1;         // tick en que el jugador inicia el Shield Attack (-1 = nunca)
  bool hold_guard = false;     // guardia normal mantenida todo el tiempo
  bool front = true;           // golpe de frente (rango del escudo)
  bool enemy_attacks = true;
  float enemy_rate = 1.0f;
  bool v080_semantics = false;  // juzgar con el estado de Link AHORA (como 0.8.0)
  bool skip_mod_update_at = false;  // P10: sin instantanea en el tick del contacto
  int ticks = 60;
};
struct SimResult {
  int confirmed = 0, committed = 0, normal_guard = 0, not_blocked = 0, missed_early = 0, missed_late = 0, duplicate = 0, no_snapshot = 0, link_damage = 0;
  int first_contact_tick = -1, transitioned = 0;
  float delta = -99;
};
static SimResult run_sim(const SimParams& sp) {
  SimResult res;
  Parry p;
  p.enabled = true;
  post08::LinkTrack track;
  int proc = sp.hold_guard ? kGuard : kIdle, slip = 0;
  float lframe = 0;
  bool pend_block = false, pend_hit = false;  // efecto de la pasada anterior sobre Link
  bool flag_hit = false, flag_shield = false;  // marcas At de la pasada anterior sobre el Bokoblin
  int estate = 0;
  float eframe = 0;
  bool deflected = false;
  auto link_exec = [&](int t) {
    if (pend_block) {
      pend_block = false;
      if (proc == kGuardAttack || proc == kGuard) proc = kGuardSlip, slip = 6;  // procGuardSlipInit
    }
    if (pend_hit) pend_hit = false, ++res.link_damage, proc = kDamaged, slip = 10;
    if (proc == kGuardSlip || proc == kDamaged) {
      if (--slip <= 0) proc = sp.hold_guard ? kGuard : kIdle;
    } else if (t == sp.press_tick && (proc == kIdle || proc == kGuard)) {
      proc = kGuardAttack, lframe = 1.0f;  // procGuardAttackInit: ANM_SHIELD_ATTACK desde el frame 1
    } else if (proc == kGuardAttack) {
      lframe += 1.0f;
      if (lframe > 18.0f) proc = sp.hold_guard ? kGuard : kIdle;
    }
  };
  auto enemy_exec = [&](int t, const post08::LinkView& now) {
    Input in = swing(0, 100, 1, estate == 0 ? 1 : estate, eframe);
    in.mode = (estate == 1 || estate == 2 || estate == 3) ? 4 : 0;
    in.state = estate;
    in.anim_rate = sp.enemy_rate;
    in.at_hit[0] = flag_hit, in.at_on_link[0] = flag_hit, in.at_shield[0] = flag_hit && flag_shield;
    in.now = now;
    in.pass = sp.v080_semantics ? now : track.pass_for(static_cast<std::uint64_t>(t), true, 7);
    const auto v = p.judge(in);
    if (v.result != Result::None && res.first_contact_tick < 0) res.first_contact_tick = t, res.delta = v.delta;
    if (v.transitioned) ++res.transitioned;
    switch (v.result) {
      case Result::Confirmed:
        ++res.confirmed;
        p.commit(0, 1, 100, v.token, true);
        ++res.committed;
        estate = 3, deflected = true;  // post08_deflect: golpe rechazado, sin esferas At
        break;
      case Result::NormalGuard: ++res.normal_guard; break;
      case Result::NotBlocked: ++res.not_blocked; break;
      case Result::MissedEarly: ++res.missed_early; break;
      case Result::MissedLate: ++res.missed_late; break;
      case Result::Duplicate: ++res.duplicate; break;
      case Result::NoPassSnapshot: ++res.no_snapshot; break;
      default: break;
    }
    // executeAttack: el golpe empieza en el tick 10
    if (sp.enemy_attacks && t == 10 && estate == 0) estate = 1, eframe = 0;
    if (estate == 1 || estate == 3) {
      eframe += sp.enemy_rate;
      if (eframe > 30.0f) estate = 0;
    }
  };
  for (int t = 1; t <= sp.ticks; ++t) {
    const bool ga = proc == kGuardAttack;
    if (!(sp.skip_mod_update_at && flag_hit)) track.observe(static_cast<std::uint64_t>(t), true, 7, proc, ga, lframe, 1.0f, 12.0f);
    auto now_view = [&] {
      post08::LinkView v;
      v.valid = true, v.proc = proc, v.guard_attack = proc == kGuardAttack, v.frame = lframe, v.start = 1.0f, v.end = 12.0f;
      return v;
    };
    if (sp.link_first) link_exec(t), enemy_exec(t, now_view());
    else enemy_exec(t, now_view()), link_exec(t);
    // pasada de colision del tick t
    const bool at_active = estate == 1 && eframe >= 14.0f && eframe <= 22.0f;
    const bool shield_on = proc == kGuardAttack || proc == kGuard || proc == kGuardSlip;
    flag_hit = at_active;
    flag_shield = at_active && shield_on && sp.front;
    if (at_active) (flag_shield ? pend_block : pend_hit) = true;
  }
  (void)deflected;
  return res;
}

// ================= FIX02: simulacion con SEMANTICA DE REGISTRO At (LAB_MODEL_ONLY; reglas tomadas de la fuente del host) ===========================
//  * executeAttack 1|2: arma la esfera si el clip esta en [14,22]; despues lee ChkAtHit (registro de la ULTIMA pasada en que estuvo registrada) y, si
//    Link esta en guardia (checkPlayerGuard: guardia normal o GUARD_SLIP, NO GUARD_ATTACK), rebota: ritmo -1, estado 3 (d_a_e_oc.cpp:1453-1483).
//  * Pasada: solo las esferas ARMADAS en ese tick se registran; solo esas se limpian y recalculan (c_cc_s.cpp:56-66,124-133). Las demas conservan su
//    registro de impacto. El solape geometrico del arma con Link ocurre UNA vez por golpe (en la primera pasada armada con frame >= 15): es lo que
//    muestra el log humano de 0.8.0 (79 contactos para 74 bloqueos nativos), y tras el bloqueo Link se desliza (GUARD_SLIP).
//  * El desvio de 0.8.1 escribe ATTACK/3 en el POST de damage_check (antes de executeAttack); FIX02 usa deflect_plan + ArmTrack.
struct Enemy {
  int state = 0;        // 0 preparar, 1 golpe, 3 rebote/desvio, 7 mareo de ruptura (DAMAGE 3), 9 FIND (espera breve antes del siguiente ataque)
  int stun_ticks = 0, pending_break = 0;
  float frame = 0, rate = 1.0f, base_rate = 1.0f;
  int find_ticks = 0;
  bool flag_hit = false, flag_shield = false;  // registro At (persistente)
  bool overlap_used = false;
  bool armed_this_tick = false;
  int armed_swings = 0, aborted_swings = 0, started_swings = 0, reached = 0;
  bool in_swing = false, swing_armed = false, swing_parried = false;
  int false_confirmed = 0, confirmed = 0, stale = 0;
};
struct Sim2Params {
  bool fix02 = true;
  bool link_first = true;
  int enemies = 1;
  std::vector<int> sa_ticks;        // pulsaciones de Shield Attack
  bool hold_guard = true;           // entre acciones Link mantiene la guardia normal
  int ticks = 600;
  float rate = 1.0f;
  bool break_on_parry = false;      // el parry cruza 100: 0.8.1 marea en el mismo damage_check; FIX02 una llamada despues
};
struct Sim2Result {
  std::vector<Enemy> e;
  int link_damage = 0, confirmed = 0, false_confirmed = 0, stale = 0, damage_on_parried = 0;
  unsigned watch_suspected = 0, watch_still = 0, watch_next_attack = 0;
};
static Sim2Result run_sim2(const Sim2Params& sp) {
  Sim2Result res;
  res.e.resize(static_cast<size_t>(sp.enemies));
  for (auto& en : res.e) en.rate = en.base_rate = sp.rate;
  Parry p;
  p.enabled = true;
  post08::LinkTrack track;
  post08::ArmTrack arm;
  post08::AiWatch watch;
  int proc = sp.hold_guard ? kGuard : kIdle, slip = 0;
  float lframe = 0;
  bool pend_block = false, pend_hit = false;
  auto link_exec = [&](int t) {
    if (pend_block) {
      pend_block = false;
      if (proc == kGuardAttack || proc == kGuard) proc = kGuardSlip, slip = 8;
    }
    if (pend_hit) pend_hit = false, ++res.link_damage, proc = kDamaged, slip = 10;
    if (proc == kGuardSlip || proc == kDamaged) {
      if (--slip <= 0) proc = sp.hold_guard ? kGuard : kIdle;
    } else if (std::find(sp.sa_ticks.begin(), sp.sa_ticks.end(), t) != sp.sa_ticks.end() && (proc == kIdle || proc == kGuard)) {
      proc = kGuardAttack, lframe = 1.0f;
    } else if (proc == kGuardAttack) {
      lframe += 1.0f;
      if (lframe > 18.0f) proc = sp.hold_guard ? kGuard : kIdle;
    }
  };
  auto link_view = [&] {
    post08::LinkView v;
    v.valid = true, v.proc = proc == kGuardAttack ? 0x1e : (proc == kGuardSlip ? 0x1d : proc), v.guard_attack = proc == kGuardAttack, v.frame = lframe, v.start = 1, v.end = 12;
    return v;
  };
  auto enemy_exec = [&](int i, int t) {
    Enemy& en = res.e[static_cast<size_t>(i)];
    const int slot = i;
    const std::uint32_t id = 100 + static_cast<std::uint32_t>(i);
    // ---- PRE de damage_check ----
    Input in = swing(slot, id, 1, en.state == 1 ? 1 : 0, en.frame);
    in.mode = (en.state == 0 || en.state == 1 || en.state == 3) ? 4 : (en.state == 7 ? 5 : 3);
    in.state = en.state == 9 ? 4 : (en.state == 7 ? 3 : en.state);
    in.anim_rate = en.rate;
    in.at_hit[0] = in.at_on_link[0] = en.flag_hit;
    in.at_shield[0] = en.flag_hit && en.flag_shield;
    in.now = link_view();
    in.pass = track.pass_for(static_cast<std::uint64_t>(t), true, 7);
    in.fresh = sp.fix02 ? arm.fresh(slot, id, 1, static_cast<std::uint64_t>(t)) : true;  // 0.8.1 no tenia puerta de frescura
    const bool truly_fresh = arm.fresh(slot, id, 1, static_cast<std::uint64_t>(t));       // verdad del modelo
    post08::AiWatch::Sample sm;
    sm.mode = static_cast<std::int16_t>(in.mode), sm.state = static_cast<std::int16_t>(in.state), sm.frame = en.frame, sm.rate = en.rate,
    sm.t6c0 = static_cast<std::int16_t>(en.find_ticks);
    const auto wo = watch.feed(slot, id, 1, static_cast<std::uint64_t>(t), sm, false, false);
    if (wo.ev == post08::AiWatch::Event::Suspected) ++res.watch_suspected;
    if (wo.ev == post08::AiWatch::Event::Ended && wo.w && wo.w->swings_armed) ++res.watch_next_attack;
    if (wo.ev == post08::AiWatch::Event::Ended && std::strcmp(wo.verdict, "AI_STALL_STILL_SUSPECTED") == 0) ++res.watch_still;
    const auto v = p.judge(in);
    if (v.result == Result::StaleContact) ++en.stale, ++res.stale;
    if (en.pending_break == 2) en.state = 7, en.stun_ticks = 45, en.pending_break = 0, en.in_swing = false;  // ruptura entregada (POST06)
    if (en.pending_break == 1) en.pending_break = 2;
    // ---- POST: adaptador ----
    if (v.result == Result::Confirmed) {
      ++en.confirmed, ++res.confirmed, en.swing_parried = true;
      if (!truly_fresh) ++en.false_confirmed, ++res.false_confirmed;
      p.commit(slot, 1, 100 + static_cast<std::uint32_t>(i), v.token, true);
      watch.start(slot, id, 1, static_cast<std::uint64_t>(t), post08::AiWatch::Cause::Parry, false);
      if (sp.break_on_parry) {
        if (!sp.fix02) en.state = 7, en.stun_ticks = 45;  // 0.8.1: mareo escrito en el POST del contacto (antes de executeAttack)
        else en.pending_break = 1;                       // FIX02: executeAttack corre este tick; el mareo llega en la llamada siguiente
      } else if (!sp.fix02) {
        en.state = 3;  // 0.8.1: ATTACK/3 escrito antes de executeAttack, ritmo positivo
      } else {
        const auto plan = post08::deflect_plan(post08::LiveShape{4, en.state, 3}, post08::LiveShape{4, en.state, 3}, post08::link_protected_now(in.now));
        if (plan == post08::DeflectPlan::ForceRecoil) en.state = 3;
      }
    }
    arm.note_post(slot, id, 1, static_cast<std::uint64_t>(t), post08::ArmTrack::armed_now(in.mode == 4 && en.state == 1 ? 4 : 0, en.state == 1 ? 1 : 0, true, en.frame));
    // ---- executeAttack / FIND ----
    en.armed_this_tick = false;
    const bool link_guard = proc == kGuard || proc == kGuardSlip;  // checkPlayerGuard()
    switch (en.state) {
      case 0:
        en.state = 1, en.frame = 0, en.rate = en.base_rate, en.in_swing = true, en.swing_armed = false, en.swing_parried = false, ++en.started_swings, en.overlap_used = false;
        break;
      case 1:
        if (en.frame >= 14.0f && en.frame <= 22.0f) {
          en.armed_this_tick = true;
          if (!en.swing_armed) en.swing_armed = true, ++en.armed_swings;
        }
        if (en.flag_hit && link_guard) {  // rebote nativo
          en.rate = -1.0f, en.state = 3;
          if (!en.swing_armed) ++en.aborted_swings;
          en.in_swing = false;
          break;
        }
        en.frame += en.rate;
        if (en.frame >= 30.0f) en.state = 9, en.find_ticks = 8, en.in_swing = false;
        break;
      case 3:
        if (en.in_swing && !en.swing_armed) ++en.aborted_swings;
        en.in_swing = false;
        en.frame += en.rate;
        if ((en.rate < 0 && en.frame <= 0.0f) || (en.rate > 0 && en.frame >= 30.0f)) en.state = 9, en.find_ticks = 8, en.rate = en.base_rate;
        break;
      case 7:
        if (--en.stun_ticks <= 0) en.state = 9, en.find_ticks = 8;
        break;
      case 9:
        if (--en.find_ticks <= 0) en.state = 0;
        break;
    }
  };
  for (int t = 1; t <= sp.ticks; ++t) {
    track.observe(static_cast<std::uint64_t>(t), true, 7, proc == kGuardAttack ? 0x1e : (proc == kGuardSlip ? 0x1d : proc), proc == kGuardAttack, lframe, 1, 12);
    if (sp.link_first) link_exec(t);
    for (int i = 0; i < sp.enemies; ++i) enemy_exec(i, t);
    if (!sp.link_first) link_exec(t);
    // pasada de colision
    for (auto& en : res.e) {
      if (!en.armed_this_tick) continue;  // no registrada: su registro At NO se toca
      en.flag_hit = false, en.flag_shield = false;  // ClrAtHitInf
      if (!en.overlap_used && en.frame >= 15.0f) {
        en.overlap_used = true;
        en.flag_hit = true;
        const bool shield = proc == kGuardAttack || proc == kGuard || proc == kGuardSlip;
        en.flag_shield = shield;
        if (shield) pend_block = true;
        else pend_hit = true, res.damage_on_parried += en.swing_parried ? 1 : 0;
      }
    }
    watch.expire(static_cast<std::uint64_t>(t));
  }
  return res;
}
int main() {
  // T01 too early, no contact: nothing
  fam = 1;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in = with_link(in, true, 2);
    ck(p.judge(in).result == Result::None, "no contact -> none");
    ck(p.c.confirmed == 0, "no confirm");
  }
  // T02 too late: hit already resolved against Link (unshielded or guard) earlier in this attack
  fam = 2;
  for (bool guard : {false, true}) {
    Parry p;
    p.enabled = true;
    Input a = swing();
    a.at_hit[0] = a.at_on_link[0] = true;
    a.at_shield[0] = guard;  // guard => normal guard block, no guard => damage
    a = with_link(a, false, 0);
    const auto v0 = p.judge(a);
    ck(v0.result == (guard ? Result::NormalGuard : Result::NotBlocked), "first contact classified");
    const auto v1 = p.judge(blocked(swing(0, 100, 1, 1, 16), 2));
    ck(v1.result == Result::MissedLate && p.c.confirmed == 0, "late shield attack is not a parry");
  }
  // T03 Shield Attack on an idle (non attacking) Bokoblin: no At contact on Link at all -> None; T04 attacking but no contact -> None
  fam = 3;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.mode = 0, in.state = 0;
    in = with_link(in, true, 2);
    ck(p.judge(in).result == Result::None, "idle");
    Input w = in;
    w.at_hit[0] = w.at_on_link[0] = w.at_shield[0] = true;  // stale/odd contact while not swinging
    ck(p.judge(w).result == Result::NotAttacking, "contact outside a swing is never a parry");
  }
  fam = 4;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.at_hit[0] = true;  // hit something that is NOT Link
    in.at_shield[0] = true;
    in = with_link(in, true, 2);
    ck(p.judge(in).result == Result::None, "hit on another object");
    Input b = swing();
    b.at_on_link[0] = b.at_shield[0] = true;  // no AtHit flag this pass
    b = with_link(b, true, 2);
    ck(p.judge(b).result == Result::None, "no hit flag");
  }
  // T05 from behind: engine shield front range fails -> no AtShieldHit -> NotBlocked
  fam = 5;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.at_hit[0] = in.at_on_link[0] = true;
    in = with_link(in, true, 2);
    ck(p.judge(in).result == Result::NotBlocked, "unblocked (behind)");
  }
  // T06 precise parry: window boundaries in Shield Attack frames
  fam = 6;
  for (float f = -2.0f; f <= 14.0f; f += 0.25f) {
    Parry p;
    p.enabled = true;
    const auto v = p.judge(blocked(swing(), f));
    const float d = f - 1.0f;
    const bool in_window = d >= -post08::policy::kPreStartSlack && d <= post08::policy::kWindowFrames;
    ck((v.result == Result::Confirmed) == in_window, "window");
    if (!in_window) ck(v.result == Result::MissedEarly, "outside -> missed early");
  }
  // T07 group: only the attacker that hit the shield; another Bokoblin in the same tick without contact gets nothing
  fam = 7;
  {
    Parry p;
    p.enabled = true;
    ck(p.judge(blocked(swing(0, 100), 2)).result == Result::Confirmed, "attacker");
    ck(p.judge(swing(1, 101)).result == Result::None, "bystander");
  }
  // T08/T09 Red (E_OC2 atp 2, rate x2 allowed) and normal Black (E_OC2 atp 3 at x1.15) are parried; T10 fury Black refused (Expansion chip)
  fam = 8;
  {
    Parry p;
    p.enabled = true;
    Input r = blocked(swing(), 2);
    r.e_oc2 = true, r.atp = 2, r.anim_rate = 2.0f;
    ck(p.judge(r).result == Result::Confirmed, "red x2");
  }
  fam = 9;
  for (int atp : {3, 6}) {
    Parry p;
    p.enabled = true;
    Input b = blocked(swing(), 2);
    b.e_oc2 = true, b.atp = atp, b.anim_rate = 1.15f;
    ck(p.judge(b).result == Result::Confirmed, "normal black");
  }
  fam = 10;
  for (float rate : {1.5f, 2.0f}) {
    Parry p;
    p.enabled = true;
    Input b = blocked(swing(), 2);
    b.e_oc2 = true, b.atp = 3, b.anim_rate = rate;
    ck(p.judge(b).result == Result::FuryChipPossible, "fury black refused");
    Input u = blocked(swing(1, 101), 2);
    u.e_oc2 = true, u.atp = 6, u.anim_ok = false;
    ck(p.judge(u).result == Result::FuryChipPossible, "unknown rate black refused");
  }
  // T11 two spheres of the same swing / T12 repeated calls / frames of the same swing -> one confirmation
  fam = 11;
  {
    Parry p;
    p.enabled = true;
    Input in = blocked(blocked(swing(), 2, 0), 2, 1);
    const auto v = p.judge(in);
    ck(v.result == Result::Confirmed, "one");
    p.commit(0, 1, 100, v.token, true);
    for (float f = 16; f <= 22; f += 1) {
      const auto again = p.judge(blocked(blocked(swing(0, 100, 1, 1, f), 3, 0), 3, 1));
      ck(again.result == Result::Duplicate && again.token == v.token, "duplicate of same swing");
    }
  }
  fam = 12;
  {
    Parry p;
    p.enabled = true;
    const auto v = p.judge(blocked(swing(), 2));
    p.commit(0, 1, 100, v.token, true);
    p.commit(0, 1, 100, v.token, true);
    ck(p.c.committed == 1, "commit once");
    ck(p.judge(blocked(swing(), 2)).result == Result::Duplicate, "same frame again");
  }
  // T13 two different attacks of the same actor: each can be parried (new token at swing restart)
  fam = 13;
  {
    Parry p;
    p.enabled = true;
    const auto v1 = p.judge(blocked(swing(0, 100, 1, 1, 15), 2));
    p.commit(0, 1, 100, v1.token, true);
    Input idle = swing();
    idle.mode = 3, idle.state = 0;
    p.judge(idle);
    const auto v2 = p.judge(blocked(swing(0, 100, 1, 2, 15), 2));
    ck(v2.result == Result::Confirmed && v2.token != v1.token, "second attack");
    p.commit(0, 1, 100, v2.token, true);
    const auto v3 = p.judge(blocked(swing(0, 100, 1, 2, 14.5f), 2));  // clip restarted (native ATTACK state 0 -> new swing with the same state)
    ck(v3.result == Result::Confirmed && v3.token != v2.token, "restart within attack mode");
  }
  // T14 two simultaneous attackers, each with its own blocked contact: independent results, never cross-credited
  fam = 14;
  {
    Parry p;
    p.enabled = true;
    const auto a = p.judge(blocked(swing(0, 100), 2)), b = p.judge(blocked(swing(1, 101), 2));
    ck(a.result == Result::Confirmed && b.result == Result::Confirmed && a.token != b.token, "independent");
    World w(2);
    ck(w.parry(0, a.token).applied && w.parry(1, b.token).applied, "both");
    ck(w.e(0).posture == 40 && w.e(1).posture == 40, "40 each, never 80 on one");
    ck(!w.parry(0, b.token).applied || w.e(0).posture == 80, "a foreign token can only add once to its own slot");
  }
  // T15 recycled identity: new generation/id never inherits episode or token
  fam = 15;
  {
    Parry p;
    p.enabled = true;
    const auto v = p.judge(blocked(swing(0, 100, 1), 2));
    p.commit(0, 1, 100, v.token, true);
    const auto g2 = p.judge(blocked(swing(0, 100, 2), 2));
    ck(g2.result == Result::Confirmed && g2.token != v.token, "new generation");
    const auto i2 = p.judge(blocked(swing(0, 200, 2), 2));
    ck(i2.result == Result::Confirmed && i2.token != g2.token, "new id");
    p.commit(0, 1, 100, v.token, true);  // stale commit ignored
    ck(p.c.committed == 1, "stale commit");
    World w;
    ck(!w.pe.px.apply_parry(0, w.gens[0] + 1, 100, 77, w.L()).applied && w.e(0).posture == 0, "stale generation");
    ck(!w.pe.px.apply_parry(0, w.gens[0], 999, 77, w.L()).applied, "wrong id");
    ck(!w.pe.px.apply_parry(70, w.gens[0], 100, 77, w.L()).applied, "bad slot");
  }
  // T16 event/cutscene, untracked, frozen, dead, hp<=1: no parry
  fam = 16;
  {
    Parry p;
    p.enabled = true;
    Input e = blocked(swing(), 2);
    e.event_running = true;
    ck(p.judge(e).result == Result::EventRunning, "event");
    Input u = blocked(swing(1, 101), 2);
    u.tracked = false;
    ck(p.judge(u).result == Result::Untracked, "untracked");
    for (int k = 0; k < 3; ++k) {
      Input x = blocked(swing(2 + k, 102 + k), 2);
      (k == 0 ? x.frozen : k == 1 ? x.dead : x.dead) = true;
      if (k == 2) x.dead = false, x.hp = 1;
      ck(p.judge(x).result == Result::Untracked, "unsafe identity");
    }
    Parry off;
    ck(off.judge(blocked(swing(), 2)).result == Result::None, "disabled");
  }
  // T17..T20 posture table
  fam = 17;
  {
    World w;
    const auto o = w.parry(0, 1);
    ck(o.applied && o.before == 0 && o.after == 40 && !o.crossed && w.e(0).posture == 40 && !w.e(0).latched, "0->40");
  }
  fam = 18;
  {
    World w;
    w.hit(0, post03::Family::Sword);
    w.ticks(50);
    ck(w.e(0).k == 50, "k advanced");
    const auto o = w.parry(0, 1);
    ck(o.after == 65 && w.e(0).k == 0 && w.e(0).regen_acc == 0, "25->65 resets regeneration delay");
    World w2;
    w2.hit(0, post03::Family::Sword), w2.hit(0, post03::Family::Sword);
    ck(w2.parry(0, 1).after == 90, "50->90");
    // regeneration after the parry: 120 ticks delay then decreases
    w2.ticks(120);
    ck(w2.e(0).posture == 90, "delay respected");
    w2.ticks(12);
    ck(w2.e(0).posture < 90, "regen resumes");
  }
  fam = 19;
  for (int target : {60, 65, 75, 90}) {
    World w;
    std::uint64_t tok = 0;
    if (target == 90) w.hit(0, post03::Family::IronBall), w.parry(0, ++tok);
    else {
      w.hit(0, post03::Family::Sword), w.hit(0, post03::Family::Sword), w.hit(0, post03::Family::Sword);  // 75
      if (target < 75) {
        w.ticks(120);
        while (w.e(0).posture > target) w.ticks(1);
      }
    }
    ck(w.e(0).posture == target && !w.e(0).latched, "reached");
    const auto o = w.parry(0, ++tok);
    ck(o.applied && o.crossed && o.after == 100 && w.e(0).posture == 100 && w.e(0).latched, "crosses to exactly 100");
    ck(w.pe.px.c.parry_saturations == (target + 40 > 100 ? 1u : 0u), "saturation counted");
    ck(w.audit_ok(), "audit after crossing");
  }
  {  // 99 is not reachable in game with weights 25/50/40 and decreasing regeneration (max non-latched = 90); the saturation rule is min(100, P+40)
    for (int pb = 0; pb <= 100; ++pb) ck((pb + post04::policy::kParryPostureGain > 100 ? 100 : pb + 40) >= pb, "saturating rule");
  }
  fam = 20;
  {
    World w;
    w.parry(0, 1), w.parry(0, 2);  // 80
    ck(w.parry(0, 3).crossed, "100");
    const auto o = w.parry(0, 4);
    ck(!o.applied && o.reject == post04::Posture::ParryReject::Latched && w.e(0).posture == 100, "latched: no parry");
    ck(w.pe.px.c.parry_crossings == 1, "one crossing");
    Parry p;
    p.enabled = true;
    Input l = blocked(swing(), 2);
    l.latched = true;
    ck(p.judge(l).result == Result::Latched, "judge refuses latched");
  }
  // T21..T24 POST06 hand-off: BREAK_BY_PARRY = same standing break; no bonus, no fall, no free Helm; later sword / shield->Helm / timeout
  auto broken = [](post06::Breaker& b, post06::View& v) {
    v = post06::View{};
    v.tracked = true, v.slot = 0, v.id = 100, v.generation = 1, v.pre_hp = v.native_hp = 500, v.pre_mode = 4, v.mode = 4, v.state = 1, v.writer_ok = true;
    v.latched = true, v.parry_crossed = true;
    const auto a = b.step(v);
    b.commit(v, a, true);
    v.parry_crossed = false;
    return a;
  };
  fam = 21;
  {
    post06::Breaker b;
    b.enabled = true;
    post06::View v;
    const auto a = broken(b, v);
    ck(a.act == post06::Act::Stun && a.why == post06::Why::BreakByParry && a.extra == 0 && !a.convert_to_big, "break by parry is the standing stun");
    ck(b.slot(0).phase == post06::Phase::Punish && !b.slot(0).shield_prepared && b.c.breaks == 1 && b.c.breaks_by_parry == 1 && b.c.bonus_applied == 0, "punish, no helm");
    v.mode = 5, v.state = 3;
    for (int t = 0; t < 30; ++t) ck(b.step(v).act == post06::Act::None, "no automatic fall");
    // without parry_crossed a latched actor in Tracking is NOT a break (0.7.0 rule unchanged)
    post06::Breaker c;
    c.enabled = true;
    post06::View w = v;
    w.mode = 4, w.pre_mode = 4, w.latched = true, w.parry_crossed = false, w.hit = false;
    ck(c.step(w).why != post06::Why::BreakByParry, "only the flagged parry breaks");
    // fail-safe: a parry crossing whose stun could not be written leaves a latched actor in TRACKING; the next call recovers to 50 on its feet
    post06::Breaker f;
    f.enabled = true;
    post06::View z = w;
    z.parry_crossed = true, z.writer_ok = false;
    ck(f.step(z).act != post06::Act::Stun, "closed writer: no stun");
    z.parry_crossed = false, z.writer_ok = true, z.mode = 3, z.state = 0;
    const auto rec = f.step(z);
    ck(rec.act == post06::Act::Recover && rec.rearm_to == 50 && rec.why == post06::Why::TechnicalAbort, "latched without break recovers to 50");
    // a parry crossing while the actor is already in BIG_DAMAGE never writes the stun
    post06::Breaker d;
    d.enabled = true;
    post06::View x = w;
    x.parry_crossed = true, x.mode = 6;
    ck(d.step(x).act != post06::Act::Stun, "no stun over a fall");
  }
  fam = 22;
  {
    post06::Breaker b;
    b.enabled = true;
    post06::View v;
    broken(b, v);
    v.mode = 5, v.state = 3, v.hit = v.sword = v.from_link = true, v.native_hp = 480;
    const auto a = b.step(v);
    ck(a.act == post06::Act::Bonus && a.why == post06::Why::DecisiveSword && a.extra == 10, "later decisive sword x1.50");
  }
  fam = 23;
  {
    post06::Breaker b;
    b.enabled = true;
    post06::View v;
    broken(b, v);
    v.mode = 5, v.state = 3, v.hit = v.from_link = true;
    v.helm = v.sword = true, v.native_hp = 450;
    ck(b.step(v).act != post06::Act::Bonus, "helm without a real shield preparation is refused");
    v.helm = v.sword = false, v.shield = true, v.native_hp = 500;
    auto a = b.step(v);
    b.commit(v, a, true);
    ck(a.why == post06::Why::ShieldPrepared, "real shield prepares");
    v.shield = false, v.helm = v.sword = true, v.native_hp = 450;
    ck(b.step(v).why == post06::Why::DecisiveHelm, "then helm x1.50");
  }
  fam = 24;
  {
    post06::Breaker b;
    b.enabled = true;
    post06::View v;
    broken(b, v);
    v.mode = 5, v.state = 3;
    post06::Action a;
    for (int t = 0; t < 120 && a.act == post06::Act::None; ++t) a = b.step(v);
    ck(a.act == post06::Act::Recover && a.rearm_to == 50, "inactive link: back to 50 on feet");
  }
  // T25 FIX_REARM50 preserved (full cycle after a parry break)
  fam = 25;
  {
    post06::Breaker b;
    b.enabled = true;
    post06::View v;
    broken(b, v);
    v.mode = 5, v.state = 3, v.hit = v.sword = v.from_link = true, v.native_hp = 480;
    auto a = b.step(v);
    b.commit(v, a, true);
    v.hit = v.sword = v.from_link = false, v.mode = 6;
    for (int st : {1, 5, 6}) v.state = st, b.step(v);
    v.mode = 7, v.state = 3;
    a = b.step(v);
    ck(a.act == post06::Act::Rearm && a.rearm_to == 0 && a.why == post06::Why::RearmAfterGetUpRedirected, "rearm 0 after redirected get-up");
  }
  // T26/T27/T38 reaction shapes accepted by the deflection writer
  fam = 26;
  {
    using post08::DeflectPlan;
    const post08::LiveShape pre{4, 1, 3};
    ck(post08::deflect_plan(pre, post08::LiveShape{4, 1, 3}, true) == DeflectPlan::NativeContinues, "untouched swing + guarded Link: no write");
    ck(post08::deflect_plan(pre, post08::LiveShape{5, 3, 4}, true) == DeflectPlan::RestorePre, "native shield stun undone to the exact PRE");
    ck(post08::deflect_plan(pre, post08::LiveShape{4, 1, 3}, false) == DeflectPlan::ForceRecoil, "unprotected Link: fallback recoil");
    ck(post08::deflect_plan(pre, post08::LiveShape{5, 3, 4}, false) == DeflectPlan::ForceRecoil, "unprotected Link + stun: fallback recoil");
    for (bool prot : {false, true}) {
      ck(post08::deflect_plan(pre, post08::LiveShape{6, 0, 4}, prot) == DeflectPlan::Refuse, "big damage never");
      ck(post08::deflect_plan(pre, post08::LiveShape{5, 0, 4}, prot) == DeflectPlan::Refuse, "sword damage never");
      ck(post08::deflect_plan(pre, post08::LiveShape{9, 0, 4}, prot) == DeflectPlan::Refuse, "death never");
      ck(post08::deflect_plan(post08::LiveShape{3, 4, 0}, post08::LiveShape{3, 4, 0}, prot) == DeflectPlan::Refuse, "pre not a swing");
      ck(post08::deflect_plan(post08::LiveShape{4, 3, 3}, post08::LiveShape{4, 3, 3}, prot) == DeflectPlan::Refuse, "already rebounded");
    }
    post08::LinkView lvw;
    lvw.valid = true;
    for (int proc : {0x1d, 0x1e}) lvw.proc = proc, ck(post08::link_protected_now(lvw), "GUARD_SLIP / GUARD_ATTACK protected");
    for (int proc : {0x1f, 0, 7, 0x1c}) lvw.proc = proc, ck(!post08::link_protected_now(lvw), "GUARD_BREAK and others not protected");
    lvw.valid = false, lvw.proc = 0x1e;
    ck(!post08::link_protected_now(lvw), "invalid Link never protected");
    ck(post08::kDeflectState == 3, "fallback state");
  }
  fam = 27;
  {
    for (int st : {0, 3, 4, 5, 6})
      ck(post08::deflect_plan(post08::LiveShape{4, st, 3}, post08::LiveShape{4, st, 3}, true) == post08::DeflectPlan::Refuse, "only an active swing (1|2) can be deflected");
  }
  // T28/T29 Link protection: the parry REQUIRES the engine's own shield block of that sphere; an unblocked sphere in the same call is never a parry
  fam = 28;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.at_hit[0] = in.at_on_link[0] = in.at_shield[0] = false;
    in.at_hit[1] = in.at_on_link[1] = true;  // second sphere touched Link unblocked
    in = with_link(in, true, 2);
    ck(p.judge(in).result == Result::NotBlocked, "unblocked sphere");
  }
  fam = 29;
  {
    Parry p;
    p.enabled = true;
    const auto a = p.judge(blocked(swing(0, 100), 2));
    Input other = swing(1, 101);
    other.at_hit[0] = other.at_on_link[0] = true;  // simultaneous unblocked attacker
    other = with_link(other, true, 2);
    const auto b = p.judge(other);
    ck(a.result == Result::Confirmed && b.result == Result::NotBlocked, "the other attacker keeps its native result");
  }
  // T30 enemy HP: the posture API never touches HP (it has no HP input); T31 hit weights unchanged
  fam = 30;
  {
    World w;
    const auto before = w.pe.px.c;
    w.parry(0, 1);
    ck(w.pe.px.c.weighted == before.weighted && w.pe.px.c.confirmed_consumable == before.confirmed_consumable, "a parry is not a hit");
  }
  fam = 31;
  {
    World w;
    w.hit(0, post03::Family::Sword);
    ck(w.e(0).posture == 25, "sword 25");
    w.hit(0, post03::Family::IronBall);
    ck(w.e(0).posture == 75, "iron 50");
    w.hit(0, post03::Family::ShieldAttack);
    w.hit(0, post03::Family::Boomerang);
    ck(w.e(0).posture == 75, "others 0");
    ck(post04::policy::kParryPostureGain == 40 && post04::policy::weight_of(post03::Family::ShieldAttack) == 0, "constants");
  }
  // T32 HUD reflects +40 immediately (hit flash), crossing pulses once, rearm retracts
  fam = 32;
  {
    post07::Hud h;
    auto in = [](int p, bool l) {
      post07::In x;
      x.present = true, x.id = 100, x.generation = 1, x.posture = p, x.latched = l, x.phase = post07::Ph::Tracking, x.opacity = 1;
      return x;
    };
    h.tick(0, in(0, false), true);
    h.tick(0, in(40, false), true);
    ck(h.c.hits == 1 && h.visual(0, 0).hit > 0, "hit flash on +40");
    h.tick(0, in(80, false), true);
    h.tick(0, in(100, true), true);
    for (int t = 0; t < 60; ++t) h.tick(0, in(100, true), true);
    ck(h.c.pulses == 1, "one pulse");
  }
  // T33 conservation with hits + parries + rearm 0/50 + retire
  fam = 33;
  {
    World w(3);
    std::uint64_t tok = 0;
    for (int round = 0; round < 40; ++round) {
      for (unsigned i = 0; i < 3; ++i) {
        if ((round + i) % 3 == 0) w.hit(i, post03::Family::Sword);
        if ((round + i) % 4 == 1) w.parry(i, ++tok);
        if ((round + i) % 5 == 2) w.parry(i, tok);  // duplicate token
        if (w.e(i).latched && (round % 2)) w.pe.px.rearm(static_cast<int>(i), w.gens[i], 100 + i, (round % 4) ? 50 : 0);
      }
      w.ticks(37);
      ck(w.audit_ok(), "audit every round");
    }
    const auto& c = w.pe.px.c;
    ck(c.parries > 0 && c.parry_crossings > 0 && c.parry_rejected[static_cast<unsigned>(post04::Posture::ParryReject::DuplicateToken)] > 0, "exercised");
    ck(c.crossings <= c.weighted + c.parries && c.parry_calls == c.parries + c.parry_rejected[0] + c.parry_rejected[1] + c.parry_rejected[2] + c.parry_rejected[3] +
                                                                    c.parry_rejected[4] + c.parry_rejected[5] + c.parry_rejected[6] + c.parry_rejected[7],
       "equations");
    w.pe.px.c.parries += 1;  // corrupted bookkeeping must be detected
    ck(!w.audit_ok(), "corruption detected");
  }
  // T34 POST05/POST06: a parry never reaches consume() (no HitReport), x1.50 needs a later decisive contact (T22), no duplicate bonus
  fam = 34;
  {
    World w;
    const auto calls = w.pe.px.c.consume_calls;
    w.parry(0, 1);
    ck(w.pe.px.c.consume_calls == calls, "no consume");
  }
  // T35 fail-closed: token 0, unknown identity
  fam = 35;
  {
    World w;
    ck(w.pe.px.apply_parry(0, w.gens[0], 100, 0, w.L()).reject == post04::Posture::ParryReject::BadToken, "token 0");
    ck(w.e(0).posture == 0 && w.audit_ok(), "nothing applied");
    Parry p;
    p.enabled = true;
    Input in = blocked(swing(), 2);
    in.slot = 64;
    ck(p.judge(in).result == Result::Untracked, "bad slot");
  }
  // T36 reordered / repeated events: commit before or after judge of a later frame never doubles
  fam = 36;
  {
    Parry p;
    p.enabled = true;
    World w;
    const auto v = p.judge(blocked(swing(), 2));
    const auto v2 = p.judge(blocked(swing(0, 100, 1, 1, 16), 3));  // next frame judged before the first commit
    ck(v2.token == v.token, "same token");
    w.parry(0, v.token);
    p.commit(0, 1, 100, v.token, true);
    ck(!w.parry(0, v2.token).applied && w.e(0).posture == 40, "posture dedupes the same attack token");
  }
  // T37 fury chip: refused, never declared deflected (see T10); T39 bounded logging
  fam = 37;
  {
    Input b = blocked(swing(), 2);
    b.e_oc2 = true, b.atp = 3, b.anim_rate = 2.0f;
    ck(Parry::fury_chip_possible(b), "fury");
    b.atp = 2;
    ck(!Parry::fury_chip_possible(b), "red never");
    b.e_oc2 = false, b.atp = 3;
    ck(!Parry::fury_chip_possible(b), "blue never");
  }
  fam = 39;
  {
    Parry p;
    unsigned taken = 0;
    for (int i = 0; i < 1000; ++i) taken += p.log_take() ? 1 : 0;
    ck(taken == 200 && p.c.log_dropped == 800, "log cap");
  }
  // T40 disabled parry = 0.7.0 (no verdicts, no writes); T42 Link not verifiable / not in Shield Attack
  fam = 40;
  {
    Parry p;
    for (int i = 0; i < 10; ++i) ck(p.judge(blocked(swing(), 2)).result == Result::None, "off");
  }
  fam = 42;
  {
    Parry p;
    p.enabled = true;
    Input in = blocked(swing(), 2);
    in.link_ok = false;
    ck(p.judge(in).result == Result::LinkInvalid, "link invalid");
    Input g = blocked(swing(1, 101), 2, 0, false);
    ck(p.judge(g).result == Result::NormalGuard, "plain guard");
  }
  // T38 differential: without a contact the parry core never changes anything observable by the adapter (no token committed, no posture)
  fam = 38;
  {
    Parry p;
    p.enabled = true;
    World w;
    for (int i = 0; i < 500; ++i) {
      Input in = swing(0, 100, 1, 1 + (i % 2), static_cast<float>(14 + (i % 9)));
      in = with_link(in, (i % 3) == 0, static_cast<float>(i % 15));
      ck(p.judge(in).result == Result::None, "no contact");
    }
    ck(p.c.confirmed == 0 && w.e(0).posture == 0 && w.pe.px.c.parry_calls == 0, "nothing");
  }

  // ===== FIX01 (P-familias) sobre la simulacion temporal =========================================================================================
  // El golpe del Bokoblin empieza en el tick 10; su esfera se activa en el frame 14 => primera pasada con contacto: tick 24 (rate 1.0).
  fam = 1;  // P01/P02: Shield Attack a tiempo; Link ejecuta ANTES (pasa a GUARD_SLIP antes del PRE) y DESPUES del Bokoblin
  for (bool first : {true, false}) {
    SimParams sp;
    sp.link_first = first, sp.press_tick = 20;
    const auto r = run_sim(sp);
    ck(r.confirmed == 1 && r.committed == 1 && r.link_damage == 0 && r.normal_guard == 0, "parry confirmed with the pass snapshot");
    if (first) ck(r.transitioned >= 1, "Link had already left GUARD_ATTACK when the PRE read it");
  }
  fam = 2;  // ROOT_CAUSE_REPRODUCED: la misma linea temporal juzgada como 0.8.0 (estado de Link AHORA) => NORMAL_GUARD, 0 candidatos
  {
    SimParams sp;
    sp.link_first = true, sp.press_tick = 20, sp.v080_semantics = true;
    const auto r = run_sim(sp);
    ck(r.confirmed == 0 && r.normal_guard >= 1, "0.8.0 semantics reproduce 0 candidates / NORMAL_GUARD");
    sp.link_first = false;
    ck(run_sim(sp).confirmed == 1, "0.8.0 only worked if Link executed after the enemy");
  }
  fam = 3;  // P03 guardia normal mantenida: nunca parry
  {
    SimParams sp;
    sp.hold_guard = true;
    const auto r = run_sim(sp);
    ck(r.confirmed == 0 && r.normal_guard >= 1 && r.link_damage == 0, "plain guard");
    sp.press_tick = -1, sp.link_first = false;
    ck(run_sim(sp).confirmed == 0, "plain guard, other order");
  }
  fam = 4;  // P04/P05 Shield Attack sin golpe del enemigo
  {
    SimParams sp;
    sp.enemy_attacks = false, sp.press_tick = 20;
    const auto r = run_sim(sp);
    ck(r.confirmed == 0 && r.first_contact_tick < 0, "no blow");
  }
  fam = 5;  // P06 por detras: sin bloqueo nativo => dano nativo intacto, sin parry
  {
    SimParams sp;
    sp.press_tick = 20, sp.front = false;
    const auto r = run_sim(sp);
    ck(r.confirmed == 0 && r.not_blocked >= 1 && r.link_damage >= 1, "from behind");
  }
  fam = 6;  // P07/P08/P09: barrido del momento de la pulsacion respecto al impacto (tick 24) en ambos ordenes y con ritmo x1.15 / x2 del enemigo
  for (bool first : {true, false})
    for (float rate : {1.0f, 1.15f, 2.0f}) {
      int hit_tick = -1;
      {
        SimParams probe;
        probe.link_first = first, probe.enemy_rate = rate, probe.hold_guard = true;
        hit_tick = run_sim(probe).first_contact_tick - 1;  // la marca se lee un tick despues de la pasada
      }
      for (int lead = -6; lead <= 20; ++lead) {
        SimParams sp;
        sp.link_first = first, sp.enemy_rate = rate, sp.press_tick = hit_tick - lead;
        const auto r = run_sim(sp);
        const bool expect = lead >= 0 && lead <= static_cast<int>(post08::policy::kWindowFrames);
        ck((r.confirmed == 1) == expect, "window = shield attack started 0..9 ticks before the blocked pass");
        if (lead > static_cast<int>(post08::policy::kWindowFrames) && lead <= 17 && sp.press_tick >= 1) ck(r.missed_early >= 1, "too early -> MISSED_EARLY");
        if (lead < 0 || sp.press_tick < 1) ck(r.confirmed == 0 && r.link_damage >= 1, "too late / never pressed: the blow hit, never reinterpreted");
        if (expect) ck(r.link_damage == 0, "protected blow");
      }
    }
  fam = 10;  // P10 instantanea ausente en el tick del contacto => NO_PASS_SNAPSHOT, nunca parry con un sello viejo
  {
    SimParams sp;
    sp.press_tick = 20, sp.skip_mod_update_at = true;
    const auto r = run_sim(sp);
    ck(r.confirmed == 0 && r.no_snapshot >= 1, "stale snapshot refused");
    post08::LinkTrack t;
    t.observe(5, true, 7, 0x1e, true, 3, 1, 12);
    ck(!t.pass_for(6, true, 7).valid && !t.pass_for(5, true, 8).valid && !t.pass_for(5, false, 7).valid && t.pass_for(5, true, 7).valid, "tick/id bound");
  }
  fam = 11;  // P11/P12/P14: tras el parry los frames siguientes no vuelven a sumar; el rastreador abre/cierra un serial por Shield Attack
  {
    post08::LinkTrack t;
    auto a = t.observe(1, true, 7, 0, false, 0, 1, 12);
    ck(!a.started && !a.ended, "idle");
    a = t.observe(2, true, 7, 0x1e, true, 1, 1, 12);
    ck(a.started && a.serial == 1 && t.c.starts == 1, "start");
    const auto v = t.pass_for(4, true, 7);
    ck(!v.valid, "old tick");
    t.observe(3, true, 7, 0x1e, true, 2, 1, 12);
    ck(t.pass_for(3, true, 7).age_ticks == 1 && t.pass_for(3, true, 7).serial == 1, "age");
    a = t.observe(4, true, 7, 2, false, 2, 1, 12);
    ck(a.ended && a.serial == 1 && a.exit_proc == 2 && a.duration == 2, "end with exit proc (GUARD_SLIP)");
    ck(!t.pass_for(4, true, 7).guard_attack && t.pass_for(4, true, 7).serial == 0, "closed");
    a = t.observe(5, true, 9, 0x1e, true, 1, 1, 12);  // otra Link (id): nada se hereda
    ck(t.c.link_changes == 1 && a.started && a.serial == 2, "link change");
    a = t.observe(6, false, 0, -1, false, 0, 0, 0);
    ck(a.ended && !t.pass_for(6, true, 9).valid, "Link gone closes");
  }
  fam = 13;  // P13/P16: dos esferas -> un parry; dos Bokoblins en el mismo tick -> cada uno con su propio resultado (ya cubierto en T11/T14/T29)
  {
    SimParams sp;
    sp.press_tick = 20;
    const auto r = run_sim(sp);
    ck(r.confirmed == 1 && r.duplicate == 0, "one parry, attack ended by deflection");
  }
  fam = 33;  // P33: la ventana se mide en frames de la animacion de Link avanzados por TICKS de simulacion; el render no la toca (no hay entrada de FPS)
  {
    ck(post08::policy::kWindowFrames == 9.0f && post08::policy::kPreStartSlack == 1.0f && post08::policy::kWindowTicksCap == 11u, "window policy 0.8.1");
    post08::LinkView w = lv(true, 10.0f);
    ck(Parry::in_window(w), "frame 10 (delta 9) inside");
    w.frame = 10.5f;
    ck(!Parry::in_window(w), "delta 9.5 outside");
    w = lv(true, 5.0f);
    w.age_ticks = 12;
    ck(!Parry::in_window(w), "tick cap");
  }
  fam = 34;  // P34: histograma de timing
  {
    post08::TimingHist h;
    for (float d : {-5.0f, 0.0f, 0.4f, 3.0f, 9.0f, 30.0f}) h.add(d);
    char buf[256];
    h.fmt(buf, sizeof buf);
    ck(std::strcmp(buf, "-2:1,0:2,3:1,9:1,20:1") == 0, "histogram text");
  }

  // ===== FIX02 ============================================================================================================================
  // El primer golpe empieza en t=1 (estado 0 -> 1), solapa en la primera pasada armada con frame >= 15 (t=17). Shield Attack en t=12 => bloqueo en ventana.
  fam = 1;  // A1/D1: 0.8.1 (desvio ATTACK/3 antes de executeAttack) deja el registro vivo y los golpes siguientes rebotan en el frame ~1: bucle
  for (bool first : {true, false}) {
    Sim2Params sp;
    sp.fix02 = false, sp.link_first = first, sp.sa_ticks = {12};
    const auto old = run_sim2(sp);
    ck(old.confirmed >= 1 && old.e[0].armed_swings <= 1 && old.e[0].aborted_swings >= 5, "0.8.1 reproduces the aborted-swing loop (AI 'pensando')");
    ck(old.watch_suspected >= 1, "the read-only monitor flags it as AI_STALL_SUSPECTED");
    sp.fix02 = true;
    const auto neu = run_sim2(sp);
    ck(neu.confirmed == 1 && neu.false_confirmed == 0 && neu.link_damage == 0, "FIX02: one genuine parry, Link protected");
    ck(neu.e[0].armed_swings >= 10 && neu.e[0].aborted_swings == 0, "FIX02: the Bokoblin keeps attacking (armed swings), no aborted swings");
    ck(neu.watch_suspected == 0 && neu.watch_next_attack >= 1, "monitor: NORMAL_NEXT_ATTACK_OBSERVED");
  }
  fam = 2;  // falsos parries a frame 1: 0.8.1 confirma registros viejos al empezar un golpe nuevo; FIX02 los ignora (STALE_CONTACT)
  {
    Sim2Params sp;
    sp.fix02 = false, sp.hold_guard = false;
    for (int t = 12; t < 600; t += 23) sp.sa_ticks.push_back(t);
    const auto old = run_sim2(sp);
    ck(old.false_confirmed >= 1, "0.8.1 confirms parries from stale At records (the enemy_frame=1.00 lines of the log)");
    sp.fix02 = true;
    const auto neu = run_sim2(sp);
    ck(neu.false_confirmed == 0 && neu.confirmed >= 1 && neu.damage_on_parried == 0, "FIX02: only fresh (armed-pass) contacts can be parries; parried blows never hurt");
  }
  fam = 3;  // B (grupo): A recibe un parry y B pelea; con 0.8.1 A se queda en bucle mientras Link guarda frente a B; con FIX02 ambos atacan
  {
    Sim2Params sp;
    sp.enemies = 2, sp.sa_ticks = {12};
    sp.fix02 = false;
    const auto old = run_sim2(sp);
    ck(old.e[0].aborted_swings >= 5 || old.e[1].aborted_swings >= 5, "0.8.1: one actor stuck after the shared shield block");
    sp.fix02 = true;
    const auto neu = run_sim2(sp);
    for (const auto& en : neu.e) ck(en.armed_swings >= 10 && en.aborted_swings == 0, "FIX02: both actors keep attacking");
    ck(neu.false_confirmed == 0, "no false parry on the neighbour");
  }
  fam = 4;  // sin parry ni Shield Attack: guardia normal y golpes (vanilla) -> ambos versiones identicas y sin bucle (control: el rebote nativo refresca)
  for (bool fix : {false, true}) {
    Sim2Params sp;
    sp.fix02 = fix, sp.enemies = 2;
    const auto r = run_sim2(sp);
    ck(r.confirmed == 0 && r.link_damage == 0, "guard only: no parry, no damage");
    for (const auto& en : r.e) ck(en.armed_swings >= 10 && en.aborted_swings == 0, "native guard rebound never loops (registro refrescado)");
  }
  fam = 5;  // ritmos de Expansion x1.15 / x2 y orden de ejecucion: FIX02 estable
  for (float rate : {1.0f, 1.15f, 2.0f})
    for (bool first : {true, false}) {
      Sim2Params sp;
      sp.rate = rate, sp.link_first = first, sp.enemies = 3;
      for (int t = 12; t < 600; t += 37) sp.sa_ticks.push_back(t);
      const auto r = run_sim2(sp);
      ck(r.false_confirmed == 0 && r.damage_on_parried == 0, "no false parry, never damage from a parried blow");
      for (const auto& en : r.e) ck(en.aborted_swings == 0 && en.armed_swings >= 5, "every actor progresses");
    }
  fam = 6;  // ArmTrack: tick y identidad
  {
    post08::ArmTrack a;
    ck(post08::ArmTrack::armed_now(4, 1, true, 14.0f) && post08::ArmTrack::armed_now(4, 2, true, 22.0f), "arm window inclusive");
    ck(!post08::ArmTrack::armed_now(4, 1, true, 13.9f) && !post08::ArmTrack::armed_now(4, 1, true, 22.1f) && !post08::ArmTrack::armed_now(4, 3, true, 16.0f) &&
           !post08::ArmTrack::armed_now(3, 1, true, 16.0f) && !post08::ArmTrack::armed_now(4, 1, false, 16.0f),
       "outside the native arm condition");
    a.note_post(2, 7, 3, 10, true);
    ck(a.fresh(2, 7, 3, 11) && !a.fresh(2, 7, 3, 12) && !a.fresh(2, 7, 3, 10) && !a.fresh(2, 8, 3, 11) && !a.fresh(2, 7, 4, 11) && !a.fresh(3, 7, 3, 11), "fresh only next tick, same identity");
    a.note_post(2, 7, 3, 11, false);
    ck(!a.fresh(2, 7, 3, 12), "not armed -> stale");
    ck(!a.fresh(-1, 7, 3, 1) && !a.fresh(64, 7, 3, 1), "bad slots");
    Parry p;
    p.enabled = true;
    Input in = blocked(swing(), 2);
    in.fresh = false;
    ck(p.judge(in).result == Result::StaleContact && p.c.contacts == 0 && p.c.stale == 1, "stale contact neither counts nor resolves");
    in.fresh = true;
    ck(p.judge(in).result == Result::Confirmed, "the same episode can still be parried by a fresh contact");
  }
  fam = 7;  // AiWatch: clasificacion
  {
    post08::AiWatch w;
    w.start(0, 1, 1, 0, post08::AiWatch::Cause::Parry, false);
    post08::AiWatch::Sample x;
    x.mode = 4, x.state = 1, x.frame = 1;
    unsigned sus = 0;
    for (int t = 1; t <= 40; ++t) {
      x.state = (t % 4 == 0) ? 3 : 1, x.frame = static_cast<float>(t % 4);
      if (w.feed(0, 1, 1, static_cast<std::uint64_t>(t), x, false, false).ev == post08::AiWatch::Event::Suspected) ++sus;
    }
    ck(sus == 1 && w.c.suspected == 1, "aborted swings -> one SUSPECTED");
    post08::AiWatch f;
    f.start(1, 2, 1, 0, post08::AiWatch::Cause::Suppression, true);
    post08::AiWatch::Sample y;
    y.mode = 3, y.state = 4, y.frame = 10;
    unsigned fz = 0;
    for (int t = 1; t <= 100; ++t)
      if (f.feed(1, 2, 1, static_cast<std::uint64_t>(t), y, false, false).ev == post08::AiWatch::Event::Suspected) ++fz;
    ck(fz == 1 && f.c.suspected_e_oc2 == 1, "frozen FIND/4 (no timer, no frame change) -> SUSPECTED");
    post08::AiWatch n;
    n.start(1, 2, 1, 0, post08::AiWatch::Cause::Suppression, false);
    for (int t = 1; t <= 100; ++t) {
      y.t6c0 = static_cast<std::int16_t>(100 - t);  // espera nativa con temporizador que avanza
      ck(n.feed(1, 2, 1, static_cast<std::uint64_t>(t), y, false, false).ev != post08::AiWatch::Event::Suspected, "native timed wait is NOT a stall");
    }
    post08::AiWatch k;
    k.start(1, 2, 1, 0, post08::AiWatch::Cause::Break, false);
    y.t6c0 = 0;
    for (int t = 1; t <= 120; ++t) ck(k.feed(1, 2, 1, static_cast<std::uint64_t>(t), y, false, true).ev != post08::AiWatch::Event::Suspected, "stun/fall (ko) never a stall");
    const auto e = k.feed(1, 2, 1, 121, y, true, false);
    ck(e.ev == post08::AiWatch::Event::Ended && std::strcmp(e.verdict, "ACTOR_DEAD") == 0, "death closes");
    post08::AiWatch cap;
    for (int i = 0; i < 10; ++i) cap.start(i, 10 + i, 1, 0, post08::AiWatch::Cause::Parry, false);
    ck(cap.active() == post08::AiWatch::kWatches && cap.c.dropped_full == 2, "bounded watches");
    const auto ex = cap.expire(100);
    ck(ex.ev == post08::AiWatch::Event::Ended && cap.active() == post08::AiWatch::kWatches - 1, "expire one per call (actor no longer executed)");
    post08::AiWatch id;
    id.start(0, 1, 1, 0, post08::AiWatch::Cause::Parry, false);
    for (int t = 1; t <= 40; ++t) {  // la generacion reciclada aborta golpes: nunca contamina la vigilancia de la generacion 1
      x.state = (t % 4 == 0) ? 3 : 1, x.frame = static_cast<float>(t % 4);
      ck(id.feed(0, 1, 2, static_cast<std::uint64_t>(t), x, false, false).ev == post08::AiWatch::Event::None, "other generation never fed into this watch");
    }
    ck(id.c.suspected == 0, "no stall attributed across generations");
  }
  fam = 8;  // ruptura por parry: 0.8.1 marea en el tick del contacto (registro vivo -> bucle tras el mareo); FIX02 la aplaza una llamada
  for (bool first : {true, false}) {
    Sim2Params sp;
    sp.break_on_parry = true, sp.link_first = first, sp.sa_ticks = {12};
    sp.fix02 = false;
    const auto old = run_sim2(sp);
    ck(old.e[0].aborted_swings >= 5 && old.e[0].armed_swings <= 1, "0.8.1: aborted-swing loop after the parry break (log: 221 l.3040 -> 3064)");
    sp.fix02 = true;
    const auto neu = run_sim2(sp);
    ck(neu.e[0].aborted_swings == 0 && neu.e[0].armed_swings >= 8 && neu.link_damage == 0, "FIX02: after the deferred break the Bokoblin attacks again");
  }
  {  // POST06: la llamada del contacto espera (ni ruptura ni rearme 50); la siguiente entrega BREAK_BY_PARRY
    post06::Breaker b;
    b.enabled = true;
    post06::View v;
    v.tracked = true, v.slot = 0, v.id = 100, v.generation = 1, v.pre_hp = v.native_hp = 500, v.pre_mode = 4, v.mode = 4, v.state = 1, v.writer_ok = true, v.latched = true;
    v.parry_break_pending = true;
    auto a = b.step(v);
    ck(a.act == post06::Act::None && b.slot(0).phase == post06::Phase::Tracking, "contact call: wait, no Recover 50");
    v.parry_break_pending = false, v.parry_crossed = true, v.state = 3;
    a = b.step(v);
    ck(a.act == post06::Act::Stun && a.why == post06::Why::BreakByParry, "next call: BREAK_BY_PARRY");
  }
  for (unsigned i = 1; i <= 42; ++i)
    if (fam_checks[i]) std::printf("T%02u checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("POST08_PARRY_LAB checks=%u failures=%u RESULT=%s\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
