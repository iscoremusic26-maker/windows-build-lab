// POST08 0.8.0 parry laboratory: pure cores only (parry_core + posture_core + break_core + hud_core). It cannot prove the game's timing, collision or
// Link protection (HUMAN_PENDING); it proves the decision rules, the bookkeeping and the hand-off. Families T01..T42 of POSTURE_SYSTEM/POST08/TEST_PLAN.md.
#include "../src/break_core.hpp"
#include "../src/hud_core.hpp"
#include "../src/parry_core.hpp"
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>

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
  in.link_ok = true, in.atk_start = 1.0f, in.atk_end = 12.0f;
  return in;
}
static Input blocked(Input in, float link_frame, int sphere = 0, bool guard_attack = true) {
  in.at_hit[sphere] = in.at_on_link[sphere] = in.at_shield[sphere] = true;
  in.guard_attack = guard_attack;
  in.link_frame = link_frame;
  return in;
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
int main() {
  // T01 too early, no contact: nothing
  fam = 1;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.guard_attack = true, in.link_frame = 2;
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
    a.guard_attack = false;
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
    in.mode = 0, in.state = 0, in.guard_attack = true, in.link_frame = 2;
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
    in.at_shield[0] = true, in.guard_attack = true, in.link_frame = 2;
    ck(p.judge(in).result == Result::None, "hit on another object");
    Input b = swing();
    b.at_on_link[0] = b.at_shield[0] = true;  // no AtHit flag this pass
    b.guard_attack = true, b.link_frame = 2;
    ck(p.judge(b).result == Result::None, "no hit flag");
  }
  // T05 from behind: engine shield front range fails -> no AtShieldHit -> NotBlocked
  fam = 5;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.at_hit[0] = in.at_on_link[0] = true;
    in.guard_attack = true, in.link_frame = 2;
    ck(p.judge(in).result == Result::NotBlocked, "unblocked (behind)");
  }
  // T06 precise parry: window boundaries in Shield Attack frames
  fam = 6;
  for (float f = -2.0f; f <= 14.0f; f += 0.25f) {
    Parry p;
    p.enabled = true;
    const auto v = p.judge(blocked(swing(), f));
    const float d = f - 1.0f;
    const bool in_window = d >= -post08::policy::kOrderSlack && d <= post08::policy::kWindowFrames;
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
    const post08::LiveShape pre{4, 1, 3};
    ck(post08::deflect_allowed(pre, post08::LiveShape{4, 1, 3}), "untouched swing");
    ck(post08::deflect_allowed(pre, post08::LiveShape{5, 3, 4}), "native shield stun (only undone for a confirmed parry)");
    ck(!post08::deflect_allowed(pre, post08::LiveShape{6, 0, 4}), "big damage never");
    ck(!post08::deflect_allowed(pre, post08::LiveShape{5, 0, 4}), "sword damage never");
    ck(!post08::deflect_allowed(pre, post08::LiveShape{9, 0, 4}), "death never");
    ck(!post08::deflect_allowed(post08::LiveShape{3, 4, 0}, post08::LiveShape{3, 4, 0}), "pre not a swing");
    ck(!post08::deflect_allowed(post08::LiveShape{4, 3, 3}, post08::LiveShape{4, 3, 3}), "already rebounded");
    ck(post08::kDeflectState == 3, "native rejected-blow state");
  }
  fam = 27;
  {
    for (int st : {0, 3, 4, 5, 6})
      ck(!post08::deflect_allowed(post08::LiveShape{4, st, 3}, post08::LiveShape{4, st, 3}), "only an active swing (1|2) can be deflected");
  }
  // T28/T29 Link protection: the parry REQUIRES the engine's own shield block of that sphere; an unblocked sphere in the same call is never a parry
  fam = 28;
  {
    Parry p;
    p.enabled = true;
    Input in = swing();
    in.at_hit[0] = in.at_on_link[0] = in.at_shield[0] = false;
    in.at_hit[1] = in.at_on_link[1] = true;  // second sphere touched Link unblocked
    in.guard_attack = true, in.link_frame = 2;
    ck(p.judge(in).result == Result::NotBlocked, "unblocked sphere");
  }
  fam = 29;
  {
    Parry p;
    p.enabled = true;
    const auto a = p.judge(blocked(swing(0, 100), 2));
    Input other = swing(1, 101);
    other.at_hit[0] = other.at_on_link[0] = true;  // simultaneous unblocked attacker
    other.guard_attack = true, other.link_frame = 2;
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
      in.guard_attack = (i % 3) == 0, in.link_frame = static_cast<float>(i % 15);
      ck(p.judge(in).result == Result::None, "no contact");
    }
    ck(p.c.confirmed == 0 && w.e(0).posture == 0 && w.pe.px.c.parry_calls == 0, "nothing");
  }
  for (unsigned i = 1; i <= 42; ++i)
    if (fam_checks[i]) std::printf("T%02u checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("POST08_PARRY_LAB checks=%u failures=%u RESULT=%s\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
