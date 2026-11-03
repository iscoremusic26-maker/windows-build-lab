// POST05 laboratorio ADVERSARIAL de supresion selectiva: motor completo (observador POST03.1 + postura POST04 + decision POST05) sobre un MODELO de
// damage_check derivado de la fuente (d_a_e_oc.cpp:584-594 setActionMode, :638-785 damage_check) con un ORACULO independiente.
// LAB_PASS = consistencia del nucleo con ese modelo. NO demuestra continuidad de animacion ni de IA en el juego: eso solo lo puede ver un humano.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include "../src/reaction_core.hpp"

using namespace post03;
namespace rp = post05::policy;

static int g_checks = 0, g_fail = 0;
#define CHECK(c)                                                           \
  do {                                                                     \
    ++g_checks;                                                            \
    if (!(c)) {                                                            \
      ++g_fail;                                                            \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);             \
    }                                                                      \
  } while (0)

static const AtMasks M{0x4000002u, 0x10u, 0x2000u, 0x20u, 0x400000u, 0x10000u, 0x40u, 0x80u};
enum : std::uint32_t { SWORD = 0x4000000u, SHIELD = 0x10u, ARROW = 0x2000u, BOMB = 0x20u, IRON = 0x400000u, BOOM = 0x10000u, OTHER = 0x80000u, UNKT = 0x80000000u };

struct World {
  std::map<std::uintptr_t, std::uint32_t> types;
  std::map<std::uint32_t, std::uintptr_t> present;
  int power = 20, derefs = 0, bad_derefs = 0;
  std::uint64_t native_calls = 0, native_processed = 0;
};
// Actor simulado: campos que el modelo de damage_check y setActionMode tocan.
struct Fake {
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  ActorFacts f;
  int health = 220, mode = 4, old_mode = 3, state = 1, cooldown = 0;
  bool cutdown = false;
  bool tg[2] = {false, false};
  std::uintptr_t hit[2] = {0, 0}, sel = 0;
  std::int16_t t6c0 = 0, t6c2 = 0;
  bool head_lock = false;
  std::uint8_t set_bits = 0x0F;
  int cut = 0;              // my_val de getCutType para espada: 0..2 ordinario, >=5 BIG (giro, salto...)
  bool jump_cancel = false; // rama checkCutJumpCancelTurn: DAMAGE + cooldown 3
  bool spl = false;         // GetAtSpl()==1 -> BIG
  int unrelated = 12345;    // testigo: ningun codigo de POST05 debe tocarlo
  Fake(std::uint32_t i, std::uintptr_t p) : id(i), ptr(p) {
    f.profile_ok = f.live = f.status_10000 = true;
    f.name = NameKind::E_OC;
    f.atp = 1;
  }
  void hit_with(int s, std::uintptr_t c) { tg[s] = true; hit[s] = c; }
};
static void set_action_mode(Fake& a, int m, int st) {  // d_a_e_oc.cpp:584-594 / binario 0x140e0d260
  a.old_mode = a.mode;
  a.mode = m;
  a.state = st;
  a.t6c0 = a.t6c2 = 0;
  a.set_bits |= 0x0F;
  a.head_lock = false;
}
static void off_tg_sph(Fake& a) { a.set_bits &= static_cast<std::uint8_t>(~0x05u); a.tg[0] = a.tg[1] = false; }
static void clear_tg(Fake& a) { a.tg[0] = a.tg[1] = false; a.hit[0] = a.hit[1] = 0; }
static void native(Fake& a, World& w) {  // modelo de daE_OC_c::damage_check
  ++w.native_calls;
  if (a.cutdown) { a.cutdown = false; a.health = 0; set_action_mode(a, 9, 1); off_tg_sph(a); return; }
  if (a.cooldown) return;
  bool had = false;
  for (int i = 0; i < 2; ++i)
    if (a.tg[i]) { a.sel = a.hit[i]; had = true; }
  if (!had) return;
  const std::uint32_t t = w.types.at(a.sel);
  a.cooldown = (t & 0xD8000000u) ? 20 : 10;
  ++w.native_processed;
  int my = 2;
  if (t & M.sword) my = a.cut;
  else if (t & M.shield) my = 3;
  else if (t & M.arrow) my = 0;
  else if (t & M.bomb) { my = 5; a.health = 0; }
  else if (t & M.iron_ball) { my = 5; a.health += 80; }
  else if (t & M.boomerang) my = 4;
  a.health -= w.power;  // cc_at_check
  if (a.health < 0) a.health = 0;
  if (a.spl && my < 5) my = 5;
  if (a.health <= 1 && my < 5) my = 5;
  if (a.jump_cancel) { set_action_mode(a, 5, my); a.cooldown = 3; return; }
  if (my >= 5) { set_action_mode(a, 6, my - 5); off_tg_sph(a); }
  else set_action_mode(a, 5, my);
}
static void execute_start(Fake& a) { if (a.cooldown > 0) --a.cooldown; }
static Snapshot snap_of(const Fake& a) {
  Snapshot s;
  s.health = a.health; s.mode = a.mode; s.state = a.state; s.old_mode = a.old_mode; s.cooldown = a.cooldown;
  s.cutdown = a.cutdown ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((a.tg[0] ? 1 : 0) | (a.tg[1] ? 2 : 0));
  return s;
}
static NativeAux aux_of(const Fake& a) {
  NativeAux x; x.valid = 1; x.t6c0 = a.t6c0; x.t6c2 = a.t6c2; x.head_lock = a.head_lock ? 1 : 0; x.set_bits = a.set_bits; return x;
}
static PreReads pre_of(const Fake& a) {
  PreReads r; r.facts = a.f;
  if (admit(a.f).adm == Admission::E_OC_CANDIDATE) { r.snap = snap_of(a); r.raw0 = a.tg[0] ? a.hit[0] : 0; r.raw1 = a.tg[1] ? a.hit[1] : 0; r.aux = aux_of(a); }
  return r;
}
static PostReads post_of(const Fake& a) {
  PostReads p; p.alive = a.f.profile_ok && a.f.live;
  if (p.alive) { p.snap = snap_of(a); p.selected = a.sel; p.aux = aux_of(a); }
  return p;
}
// Todo lo que NO es uno de los tres campos que POST05 puede escribir.
struct Rest {
  int health, cooldown, unrelated; std::int16_t t6c0, t6c2; bool head_lock, cutdown; std::uint8_t set_bits; std::uintptr_t sel;
  bool operator==(const Rest&) const = default;
};
static Rest rest_of(const Fake& a) { return Rest{a.health, a.cooldown, a.unrelated, a.t6c0, a.t6c2, a.head_lock, a.cutdown, a.set_bits, a.sel}; }

struct Hx {
  post04::PostureEngine pe;
  post05::Reaction rx;
  World w;
  std::vector<post05::DecisionResult> dec;
  std::vector<post05::Apply> app;
  std::vector<HitReport> reps;
  int writes = 0;
  Hx() { pe.eng.masks = M; }
  bool reader(std::uintptr_t sel, std::uint32_t& type) {
    ++w.derefs;
    auto it = w.types.find(sel);
    if (it == w.types.end() || !deref_alignment_ok(sel)) { ++w.bad_derefs; return false; }
    type = it->second;
    return true;
  }
  void spawn(Fake& a) { w.present[a.id] = a.ptr; }
  void despawn(Fake& a) { w.present.erase(a.id); }
  // Replica de do_pre/do_post de mod.cpp: el original corre UNA vez; POST05 decide despues de POST03.1/POST04.
  bool call(Fake& a) {
    execute_start(a);
    Events ev;
    pe.on_pre(a.ptr, a.id, pre_of(a), ev);
    native(a, w);
    Events ev2; HitReport rep;
    const bool has = pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return reader(s, t); }, ev2, rep);
    post05::LiveMode drain{};
    (void)drain;
    if (has) {
      reps.push_back(rep);
      post05::Apply ap = post05::Apply::NotAttempted;
      const post05::DecisionResult d = post05::process(rx, pe, rep, [&a] { return post05::LiveMode{a.mode, a.state, a.old_mode}; },
                                                       [&](int m, int s, int o) { ++writes; a.mode = m; a.state = s; a.old_mode = o; }, ap);
      dec.push_back(d);
      app.push_back(ap);
    }
    post04::PItem it;
    while (pe.px.pop(it)) {}
    return has;
  }
  Lookup lookup(const Pick& p) {
    auto it = w.present.find(p.id);
    if (it == w.present.end()) return Lookup::Absent;
    return it->second == p.ptr ? Lookup::PresentMatch : Lookup::PresentMismatchPtr;
  }
  void update() {
    Observer& o = pe.eng.obs;
    if (o.sweep_due())
      for (unsigned q = 0; q < kSweepQuota; ++q) { Pick p; if (!o.ledger.next_sweep(p, o.sweep_stamp)) break; Events ev; o.ledger.apply_lookup(p, lookup(p), o.epoch.epoch, ev); }
    pe.on_update();
    post04::PItem it;
    while (pe.px.pop(it)) {}
  }
  void ticks(unsigned n) { for (unsigned i = 0; i < n; ++i) update(); }
  int slot_of(std::uint32_t id) const {
    for (unsigned i = 0; i < kCapacity; ++i) { const Instance& s = pe.eng.obs.ledger.slot(i); if (s.id == id && (s.life == Life::Active || s.life == Life::DeathSeen || s.life == Life::Unverified)) return static_cast<int>(i); }
    return -1;
  }
  int posture(std::uint32_t id) const { const int s = slot_of(id); return s < 0 ? -1 : pe.px.entry(static_cast<unsigned>(s)).posture; }
  post05::Reason last_reason() const { return dec.empty() ? post05::Reason::Count : dec.back().reason; }
  post05::Apply last_apply() const { return app.empty() ? post05::Apply::NotAttempted : app.back(); }
  std::uint64_t applied() const { return rx.c.applied; }
};
static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }
static void types(Hx& h) {
  h.w.types[P(1)] = SWORD; h.w.types[P(2)] = SWORD; h.w.types[P(3)] = IRON; h.w.types[P(4)] = BOOM; h.w.types[P(5)] = ARROW; h.w.types[P(6)] = OTHER;
  h.w.types[P(7)] = SWORD | ARROW; h.w.types[P(8)] = BOMB; h.w.types[P(9)] = SHIELD; h.w.types[P(10)] = SWORD | UNKT;
}
// Golpe aislado en un estado PRE dado: espera cooldown (llamadas sin TG), coloca el modo/estado PRE y golpea UNA llamada.
static bool strike(Hx& h, Fake& a, std::uintptr_t c, int pre_mode = 4, int pre_state = 1, int sphere = 1) {
  while (a.cooldown > 0 && a.health > 0) h.call(a);
  a.mode = pre_mode; a.state = pre_state;
  a.hit_with(sphere, c);
  const bool has = h.call(a);
  clear_tg(a);
  return has;
}
static void audit_all(Hx& h) {
  CHECK(h.rx.audit().ok);
  CHECK(h.pe.px.audit().ok);
  CHECK(h.pe.eng.conserved());
}

// ------------------------------------------------------------------------------------------------------------------------------------
static void t01_t02_eligible_attack() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  const int hp0 = a.health;
  const Rest before_rest = rest_of(a);
  (void)before_rest;
  CHECK(strike(h, a, P(1), 4, 1));
  CHECK(h.last_reason() == post05::Reason::Eligible && h.last_apply() == post05::Apply::Applied);
  CHECK(a.mode == 4 && a.state == 1 && a.old_mode == 3);                 // ATTACK 1 conservado; mOldActionMode restaurado
  CHECK(a.health == hp0 - h.w.power && a.cooldown == 10);               // dano y cooldown nativos intactos
  CHECK(h.posture(10) == 25 && h.pe.px.c.weighted == 1);                 // POST04 consumio una vez
  CHECK(h.writes == 1);
  for (int st : {0, 2}) {                                                 // 25->50 (estado 0) y 50->75 (estado 2)
    CHECK(strike(h, a, P(1), 4, st));
    CHECK(h.last_apply() == post05::Apply::Applied && a.mode == 4 && a.state == st);
  }
  CHECK(h.posture(10) == 75 && h.applied() == 3);
  CHECK(h.rx.c.applied_by_pre_state[0] == 1 && h.rx.c.applied_by_pre_state[1] == 1 && h.rx.c.applied_by_pre_state[2] == 1);
  // WAIT, WALK, FIND, WATCH no estan en la whitelist: DAMAGE nativo
  for (int m : {0, 1, 3, 7}) {  // un actor nuevo por modo: la postura no llega al umbral
    Hx g; types(g);
    Fake b(11, 0x9400); g.spawn(b);
    CHECK(strike(g, b, P(1), m, 0));
    CHECK(g.last_reason() == post05::Reason::PreModeNotAttack && b.mode == 5 && g.writes == 0);
    audit_all(g);
  }
  audit_all(h);
}
static void t03_t04_threshold_and_latch() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  for (int i = 0; i < 3; ++i) strike(h, a, P(1), 4, 1);
  CHECK(h.posture(10) == 75 && h.applied() == 3);
  CHECK(strike(h, a, P(1), 4, 1));                                       // 75 -> 100: el golpe de ruptura queda VANILLA
  CHECK(h.last_reason() == post05::Reason::CrossesThreshold && a.mode == 5 && h.posture(10) == 100);
  CHECK(h.pe.px.c.crossings == 1 && h.pe.px.entry(static_cast<unsigned>(h.slot_of(10))).latched);
  CHECK(strike(h, a, P(1), 4, 1));                                       // latched: ni acumula ni suprime
  CHECK(h.last_reason() == post05::Reason::NotWeighted && a.mode == 5 && h.pe.px.c.crossings == 1 && h.applied() == 3);
  audit_all(h);
}
static void t05_t06_lethal_and_big() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a); a.health = 15;                         // HP<=1 tras el golpe -> BIG_DAMAGE + letal
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::Lethal && a.mode == 6 && h.writes == 0);
  Hx g; types(g);
  Fake b(11, 0x9400); g.spawn(b); b.health = 22;                          // queda a 2 (no letal): DAMAGE ordinario, la vida no lo impide
  CHECK(strike(g, b, P(1), 4, 1) && g.last_reason() == post05::Reason::Eligible);
  Hx k; types(k);                                                          // giro/salto (my_val>=5) y especial (spl): BIG_DAMAGE nativo
  Fake c(12, 0x9800); k.spawn(c); c.cut = 6;
  CHECK(strike(k, c, P(1), 4, 1) && k.last_reason() == post05::Reason::BigDamage && c.mode == 6 && k.writes == 0);
  c.cut = 0; c.spl = true;
  CHECK(strike(k, c, P(1), 4, 1) && k.last_reason() == post05::Reason::BigDamage && c.mode == 6);
  audit_all(h); audit_all(g); audit_all(k);
}
static void t07_other_families() {
  const std::pair<std::uintptr_t, post05::Reason> cases[] = {{P(9), post05::Reason::FamilyNotSword}, {P(5), post05::Reason::FamilyNotSword}, {P(3), post05::Reason::FamilyNotSword},
                                                           {P(4), post05::Reason::FamilyNotSword}, {P(6), post05::Reason::FamilyNotSword}, {P(7), post05::Reason::FamilyNotSword},
                                                           {P(8), post05::Reason::FamilyNotSword}};
  for (const auto& cs : cases) {
    Hx h; types(h);
    Fake a(10, 0x9000); h.spawn(a);
    CHECK(strike(h, a, cs.first, 4, 1));
    CHECK(h.last_reason() == cs.second && h.writes == 0);
    CHECK(a.mode == 5 || a.mode == 6 || a.health == 0);
  }
  Hx u; types(u);                                                          // collider no legible (desalineado): UNAVAILABLE -> vanilla
  u.w.types[0x100002u] = SWORD;
  Fake b(11, 0x9400); u.spawn(b);
  CHECK(strike(u, b, 0x100002u, 4, 1) && u.last_reason() == post05::Reason::FamilyNotSword && u.writes == 0 && b.mode == 5);
}
static void t08_finish_and_death() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1), 4, 1);
  a.cutdown = true; a.mode = 4; a.state = 1;
  CHECK(h.call(a));
  CHECK(h.last_reason() == post05::Reason::NotConfirmed && a.mode == 9 && a.health == 0 && h.writes == 1);   // solo el primer golpe se suprimio
  audit_all(h);
}
static void t09_admission() {
  { Hx h; types(h); Fake a(10, 0x9000); a.f.name = NameKind::E_OC2; h.spawn(a); a.hit_with(1, P(1)); h.call(a); CHECK(h.dec.empty() && h.writes == 0 && a.mode == 5); }
  { Hx h; types(h); Fake a(10, 0x9000); a.f.profile_ok = false; h.spawn(a); a.hit_with(1, P(1)); h.call(a); CHECK(h.dec.empty() && h.writes == 0); }
  { Hx h; types(h); Fake a(10, 0x9000); a.f.atp = 2; h.spawn(a); a.hit_with(1, P(1)); h.call(a); CHECK(h.dec.empty() && h.writes == 0); }   // UNKNOWN_UNSAFE
  {  // inestable alguna vez en esta generacion -> nunca se suprime, aunque vuelva a ser candidato
    Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
    h.call(a); a.f.atp = 2; h.call(a); a.f.atp = 1; h.call(a);
    CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::AdmissionEverUnstable && h.writes == 0 && a.mode == 5);
    CHECK(h.posture(10) == 25);  // POST04 sigue acumulando
    audit_all(h);
  }
  {  // primera observacion como UNKNOWN_UNSAFE, despues candidata
    Hx h; types(h); Fake a(10, 0x9000); a.f.atp = 2; h.spawn(a); h.call(a); a.f.atp = 1; h.call(a);
    CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::AdmissionEverUnstable && h.writes == 0);
  }
  {  // capacidad agotada: el 65.o no tiene informe ni decision
    Hx h; types(h);
    std::vector<Fake> as; as.reserve(70);
    for (unsigned i = 0; i < 64; ++i) { as.emplace_back(1000 + i, 0x200000u + i * 0x400u); h.spawn(as.back()); h.call(as.back()); }
    Fake x(5000, 0x400000u); h.spawn(x); x.hit_with(1, P(1)); h.call(x);
    CHECK(h.dec.empty() && h.writes == 0 && x.mode == 5 && h.pe.eng.obs.ledger.c.capacity_drops == 1);
  }
}
static void t10_duplicates_and_dual() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  a.mode = 4; a.state = 1; a.hit_with(0, P(1)); a.hit_with(1, P(2));   // dos esferas, colliders distintos: UNA decision
  CHECK(h.call(a));
  clear_tg(a);
  CHECK(h.dec.size() == 1 && h.writes == 1 && h.rx.c.decisions == 1 && h.pe.px.c.weighted == 1);
  // el mismo informe otra vez: duplicado, sin escritura
  post05::Apply ap = post05::Apply::NotAttempted;
  const post05::DecisionResult d2 = post05::process(h.rx, h.pe, h.reps.back(), [&a] { return post05::LiveMode{a.mode, a.state, a.old_mode}; },
                                                    [&](int, int, int) { ++h.writes; }, ap);
  CHECK(!d2.counted && ap == post05::Apply::NotAttempted && h.writes == 1 && h.rx.c.duplicates == 1);
  // POST huerfano
  Events ev; HitReport r;
  CHECK(!h.pe.on_post(a.ptr, post_of(a), [](std::uintptr_t, std::uint32_t&) { return false; }, ev, r));
  audit_all(h);
}
static void t11_t19_generations() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1), 4, 1); strike(h, a, P(1), 4, 1);
  CHECK(h.posture(10) == 50);
  h.despawn(a); h.ticks(100);
  Fake b(10, 0x9000); h.spawn(b);                                        // mismo id y direccion, nueva vida
  CHECK(strike(h, b, P(1), 4, 1) && h.posture(10) == 25 && h.last_apply() == post05::Apply::Applied);
  h.pe.eng.obs.epoch.update(stage_token("F_SP103")); h.pe.eng.obs.epoch.update(stage_token("F_SP104"));
  Fake c(20, 0xA000); h.spawn(c);
  CHECK(strike(h, c, P(1), 4, 2) && h.posture(20) == 25 && h.last_apply() == post05::Apply::Applied);
  audit_all(h);
}
static void t12_t13_states_and_signature() {
  { Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
    CHECK(strike(h, a, P(1), 4, 3) && h.last_reason() == post05::Reason::PreStateNotWhitelisted && a.mode == 5 && h.writes == 0);   // rebote por guardia
    CHECK(strike(h, a, P(1), 4, 9) && h.last_reason() == post05::Reason::PreStateNotWhitelisted && h.writes == 0); }
  // firma alterada / estado POST no ordinario / token: informes sinteticos sobre un actor real
  Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(1), 4, 1) && h.last_apply() == post05::Apply::Applied);
  const HitReport base = h.reps.back();
  const post04::ConsumeOutcome oc = h.pe.last;
  auto try_rep = [&](HitReport r, post04::ConsumeOutcome o, std::uint64_t ev) {
    r.event = ev; o.event = ev;
    post05::DecisionInput in; in.rep = r; in.outc = o; in.ledger_valid = true; in.adm = Admission::E_OC_CANDIDATE; in.life = Life::Active;
    return h.rx.decide(in).reason;
  };
  HitReport r1 = base; r1.post.old_mode = 7;      CHECK(try_rep(r1, oc, 900) == post05::Reason::PostSignatureMismatch);
  HitReport r2 = base; r2.post.state = 3;         CHECK(try_rep(r2, oc, 901) == post05::Reason::PostStateNotOrdinary);
  HitReport r3 = base; r3.pre.cooldown = 2;       CHECK(try_rep(r3, oc, 902) == post05::Reason::PreNotFresh);
  HitReport r4 = base; r4.col.deref_ok = false;   CHECK(try_rep(r4, oc, 903) == post05::Reason::ColliderNotFresh);
  HitReport r5 = base; r5.aux_pre.t6c2 = 7;       CHECK(try_rep(r5, oc, 904) == post05::Reason::PreTimersNonZero);
  HitReport r6 = base; r6.aux_pre.head_lock = 1;  CHECK(try_rep(r6, oc, 905) == post05::Reason::PreHeadLock);
  HitReport r7 = base; r7.aux_pre.set_bits = 0x0B;CHECK(try_rep(r7, oc, 906) == post05::Reason::PreSetBitsOff);
  HitReport r8 = base; r8.aux_post.t6c0 = 1;      CHECK(try_rep(r8, oc, 907) == post05::Reason::PostAuxUnexpected);
  HitReport r9 = base; r9.aux_pre.valid = 0;      CHECK(try_rep(r9, oc, 908) == post05::Reason::AuxMissing);
  post04::ConsumeOutcome o2 = oc; o2.slot = 63;   CHECK(try_rep(base, o2, 909) == post05::Reason::TokenMismatch);
  post04::ConsumeOutcome o3 = oc; o3.frozen = true; CHECK(try_rep(base, o3, 910) == post05::Reason::FrozenOrDead);
  post04::ConsumeOutcome o4 = oc; o4.dead = true; CHECK(try_rep(base, o4, 911) == post05::Reason::FrozenOrDead);
  HitReport r10 = base; r10.post.cooldown = 3;    CHECK(try_rep(r10, oc, 912) == post05::Reason::CooldownNotOrdinary);
  HitReport r11 = base; r11.pre.mode = 5; r11.post.old_mode = 5; CHECK(try_rep(r11, oc, 913) == post05::Reason::PreModeNotAttack);
  { post05::DecisionInput in; in.rep = base; in.rep.event = 914; in.outc = oc; in.outc.event = 914; in.ledger_valid = false; in.adm = Admission::E_OC_CANDIDATE; in.life = Life::Active;
    CHECK(h.rx.decide(in).reason == post05::Reason::TokenMismatch); }
  { post05::DecisionInput in; in.rep = base; in.rep.event = 915; in.outc = oc; in.outc.event = 915; in.ledger_valid = true; in.adm = Admission::UNKNOWN_UNSAFE; in.life = Life::Active;
    CHECK(h.rx.decide(in).reason == post05::Reason::NotCandidateActive); }
  CHECK(h.rx.audit().ok);
}
static void t14_session_closed() {
  Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
  h.rx.close_session();                                                   // p. ej. layout distinto del binario
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::SessionDisabled && a.mode == 5 && h.writes == 0);
  CHECK(h.posture(10) == 25);
  // motor deshabilitado (fallo de gancho): ni decisiones ni escrituras
  Hx g; types(g); Fake b(11, 0x9400); g.spawn(b); g.pe.eng.obs.disable("TEST");
  b.mode = 4; b.state = 1; b.hit_with(1, P(1)); g.call(b);
  CHECK(g.dec.empty() && g.writes == 0 && b.mode == 5);
  audit_all(h);
}
static void t15_cooldown_and_jump_cancel() {
  Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
  a.mode = 4; a.state = 1; a.cooldown = 5; a.hit_with(1, P(1));
  for (int i = 0; i < 3; ++i) h.call(a);                                   // cooldown: sin proceso, sin decision
  CHECK(h.dec.empty() && h.writes == 0);
  clear_tg(a); a.cooldown = 0;
  a.jump_cancel = true;
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::CooldownNotOrdinary && a.mode == 5 && a.cooldown == 3 && h.writes == 0);
  a.jump_cancel = false;
  Hx k; types(k); Fake b(11, 0x9400); k.spawn(b);
  CHECK(strike(k, b, P(10), 4, 1) && k.last_reason() == post05::Reason::CooldownNotOrdinary && b.cooldown == 20);   // AT_TYPE_UNK: cooldown 20
  audit_all(h);
}
static void t16_t17_t20_preservation() {
  // Gemelos: mismo guion con POST05 abierto y cerrado. Solo pueden diferir los tres campos de modo; HP, cooldown, aux y POST04 son identicos.
  Hx on; types(on); Hx off; types(off); off.rx.close_session();
  Fake a(10, 0x9000), b(10, 0x9000); on.spawn(a); off.spawn(b);
  const int pre_states[] = {1, 0, 2, 1, 1, 0};
  for (int i = 0; i < 6; ++i) {
    const std::uint64_t nc0 = on.w.native_calls, nc1 = off.w.native_calls;
    strike(on, a, P(1), 4, pre_states[i]); strike(off, b, P(1), 4, pre_states[i]);
    CHECK(on.w.native_calls - nc0 == off.w.native_calls - nc1);            // el original corre las mismas veces (una por llamada)
    CHECK(rest_of(a) == rest_of(b));                                       // nada fuera de los tres campos difiere
    CHECK(a.health == b.health);
  }
  CHECK(on.pe.px.c.weighted == off.pe.px.c.weighted && on.pe.px.c.crossings == off.pe.px.c.crossings && on.pe.px.c.units_added == off.pe.px.c.units_added);
  CHECK(on.pe.eng.obs.ledger.c.confirmed == off.pe.eng.obs.ledger.c.confirmed && on.pe.eng.obs.ledger.c.events_emitted == off.pe.eng.obs.ledger.c.events_emitted);
  CHECK(on.applied() == 3 && off.applied() == 0);                          // 0->25->50->75 suprimidos; el de ruptura y los latched no
  CHECK(on.w.native_calls == off.w.native_calls);
  // tras una supresion la llamada siguiente (sin golpe) ve ATTACK: el modo nativo prosigue (en el juego, executeAttack del mismo cuadro)
  Hx s; types(s); Fake c(30, 0xB000); s.spawn(c);
  strike(s, c, P(1), 4, 2);
  CHECK(c.mode == 4 && c.state == 2);
  s.call(c);
  CHECK(c.mode == 4 && c.state == 2);
  audit_all(on); audit_all(off); audit_all(s);
}
static void t18_freeze_with_marker() {
  Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
  for (int i = 0; i < 4; ++i) strike(h, a, P(1), 4, 1);
  CHECK(h.pe.px.c.crossings == 1);
  Pick pk; pk.slot = h.slot_of(10); pk.id = 10; pk.ptr = 0x9000; pk.generation = h.pe.px.entry(static_cast<unsigned>(pk.slot)).generation;
  Events ev; h.pe.eng.obs.ledger.apply_lookup(pk, Lookup::Creating, 1, ev); h.pe.px.sync(h.pe.eng.obs.ledger);
  CHECK(h.pe.px.entry(static_cast<unsigned>(pk.slot)).frozen && h.pe.px.entry(static_cast<unsigned>(pk.slot)).latched);
  strike(h, a, P(1), 4, 1);   // revalida en el PRE (misma direccion) y sigue latched: vanilla
  CHECK(h.last_reason() == post05::Reason::NotWeighted && a.mode == 5);
  audit_all(h);
}
static void t22_recheck_and_verify() {
  Hx h; types(h); Fake a(10, 0x9000); h.spawn(a);
  a.mode = 4; a.state = 1; a.hit_with(1, P(1)); execute_start(a);
  Events ev; h.pe.on_pre(a.ptr, a.id, pre_of(a), ev); native(a, h.w);
  Events ev2; HitReport rep;
  CHECK(h.pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return h.reader(s, t); }, ev2, rep));
  a.state = 4;  // algo cambio entre la lectura POST y la escritura: no se escribe nada
  post05::Apply ap; int w = 0;
  post05::process(h.rx, h.pe, rep, [&a] { return post05::LiveMode{a.mode, a.state, a.old_mode}; }, [&](int, int, int) { ++w; }, ap);
  CHECK(ap == post05::Apply::RecheckFailed && w == 0 && a.mode == 5 && h.rx.c.recheck_failed == 1);
  // escritura que no se refleja: fallo de verificacion -> sesion cerrada, sin mas intentos
  Hx g; types(g); Fake b(11, 0x9400); g.spawn(b);
  b.mode = 4; b.state = 1; b.hit_with(1, P(2)); execute_start(b);
  Events e1; g.pe.on_pre(b.ptr, b.id, pre_of(b), e1); native(b, g.w);
  Events e2; HitReport r2;
  CHECK(g.pe.on_post(b.ptr, post_of(b), [&](std::uintptr_t s, std::uint32_t& t) { return g.reader(s, t); }, e2, r2));
  post05::Apply ap2;
  post05::process(g.rx, g.pe, r2, [&b] { return post05::LiveMode{b.mode, b.state, b.old_mode}; }, [&](int, int, int) {}, ap2);
  CHECK(ap2 == post05::Apply::VerifyFailed && g.rx.session_disabled());
  clear_tg(b);
  strike(g, b, P(1), 4, 1);
  CHECK(g.last_reason() == post05::Reason::SessionDisabled);
  CHECK(h.rx.audit().ok && g.rx.audit().ok);
}

// ---- T23: los 27 informes REALES de POST04 A1/B1 (saneados) por el tubo completo ----------------------------------------------------------------
struct TraceRow { int session; unsigned ev; unsigned id; unsigned gen; int finish; int lethal; int family; unsigned at; int hp_post; int mode_pre, mode_post, state_pre, state_post; int exp_posture_apply; int exp_after; int exp_eligible; unsigned tick; };
#include "reaction_trace.inc"
static void t23_real_trace() {
  CHECK(kTraceCount == 27);
  unsigned eligible_total = 0;
  for (int sess = 0; sess < 2; ++sess) {
    post04::PostureEngine pe; pe.eng.masks = M;
    post05::Reaction rx;
    Ledger& L = pe.eng.obs.ledger;
    std::map<std::pair<unsigned, unsigned>, std::pair<int, std::uint64_t>> inst;
    std::uintptr_t ptr = 0x100000;
    std::uint64_t call = 0, weighted = 0;
    unsigned last_tick = 0;
    struct Projection{int base=0;unsigned last_hit=0;bool latched=false;};
    std::map<std::pair<unsigned,unsigned>,Projection> projection;
    for (unsigned i = 0; i < kTraceCount; ++i) {
      const TraceRow& r = kTrace[i];
      if (r.session != sess) continue;
      if (r.tick != 0) {  // el reloj del observador avanza con los ticks REALES registrados entre golpes (la recuperacion de 505/18 sale de aqui)
        for (unsigned k = last_tick ? last_tick : r.tick; k < r.tick; ++k) pe.px.advance();
        last_tick = r.tick;
      }
      auto key = std::make_pair(r.id, r.gen);
      if (!inst.count(key)) { Events ev; const Observation o = L.observe(r.id, ptr, Admission::E_OC_CANDIDATE, 1, ++call, ev); ptr += 0x400; pe.px.sync(L); inst[key] = {o.slot, o.generation}; }
      HitReport rep;
      rep.event = r.ev; rep.call = r.ev; rep.id = r.id; rep.slot = inst[key].first; rep.generation = inst[key].second;
      rep.life = r.lethal ? Life::DeathSeen : Life::Active;
      rep.j.kind = r.finish ? HitKind::FinishNative : HitKind::Confirmed;
      rep.j.elig = r.finish ? Eligibility::No : Eligibility::Confirmed;
      rep.j.lethal = r.lethal != 0;
      rep.post.health = r.hp_post; rep.pre.mode = r.mode_pre; rep.post.mode = r.mode_post; rep.pre.state = r.state_pre; rep.post.state = r.state_post;
      rep.post.old_mode = r.mode_pre; rep.pre.cooldown = 0; rep.post.cooldown = r.finish ? 0 : 10;   // cd=0>10 en las lineas reales procesadas
      rep.at_valid = !r.finish; rep.at_type = r.at; rep.fam.family = static_cast<Family>(r.family); rep.col.deref_ok = !r.finish;
      rep.aux_pre.valid = rep.aux_post.valid = 1; rep.aux_pre.set_bits = rep.aux_post.set_bits = kAllSetBits;   // SUPUESTO: precondiciones nativas cumplidas
      pe.last = pe.px.consume(rep, L);
      CHECK((pe.last.applied ? 1 : 0) == r.exp_posture_apply);
      // The input trace remains immutable evidence of the OLD120-tick policy.
      // Re-project its actual event gaps using independent CP02 delay60/fallback5.
      auto& q=projection[key];const unsigned elapsed=r.tick>=q.last_hit?r.tick-q.last_hit:0;
      const int recovered=q.latched?0:(elapsed>60?int((elapsed-60)/6):0);
      const int before=q.base>recovered?q.base-recovered:0;
      const int gain=r.exp_posture_apply?(r.family==1?25:r.family==5?50:0):0;
      const int projected=before+gain>100?100:before+gain;
      if(r.exp_posture_apply){q.base=projected;q.last_hit=r.tick;if(projected==100)q.latched=true;}
      if (!r.finish) CHECK(pe.px.entry(static_cast<unsigned>(rep.slot)).posture == projected);
      weighted += pe.last.applied;
      post05::Apply ap;
      int writes = 0;
      const post05::DecisionResult d = post05::process(rx, pe, rep, [] { return post05::LiveMode{5, 0, 4}; }, [&](int, int, int) { ++writes; }, ap);
      CHECK((d.decision == post05::Decision::SuppressEligible ? 1 : 0) == r.exp_eligible);
      eligible_total += d.decision == post05::Decision::SuppressEligible;
      if (r.lethal || r.finish) { Events ev; L.mark_death(rep.slot, rep.generation, 1, ev); }
      pe.px.sync(L);
    }
    CHECK(weighted == (sess ? 12u : 0u));                                   // POST04 no cambia: 12 ponderados en B1, 0 en A1
    CHECK(pe.px.audit().ok && rx.audit().ok);
  }
  CHECK(eligible_total == kTraceExpectedEligible);
}

// ---- T25/T26: estres aleatorio con semillas fijas y ORACULO independiente ------------------------------------------------------------------
struct Rng { std::uint64_t s; std::uint64_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; } unsigned below(unsigned n) { return static_cast<unsigned>(next() % n); } };
struct Orc { bool bound = false, unstable = false, latched = false, dead = false; int p = 0; bool frozen=false; unsigned regen_k=0; };
static unsigned long long g_cmp = 0, g_cov_applied = 0, g_cov_vanilla = 0, g_cov_unsafe = 0, g_cov_states[3] = {}, g_cov_threshold = 0;
static void stress(std::uint64_t seed, unsigned frames) {
  Hx h; types(h);
  Rng rng{seed};
  std::vector<std::unique_ptr<Fake>> all;
  std::vector<Fake*> live;
  std::map<Fake*, Orc> orc;
  std::uint32_t next_id = 2000;
  std::uintptr_t next_ptr = 0x800000;
  const std::uintptr_t cols[] = {P(1), P(1), P(1), P(2), P(3), P(4), P(5), P(6), P(7), P(9), P(10)};
  unsigned mism = 0;
  for (unsigned fr = 0; fr < frames; ++fr) {
    if (live.size() < 8 && rng.below(10) == 0) {
      all.push_back(std::make_unique<Fake>(next_id++, next_ptr)); next_ptr += 0x400;
      Fake* f = all.back().get(); f->health = 100000; live.push_back(f); h.spawn(*f); orc[f] = Orc{};
    }
    if (!live.empty() && rng.below(120) == 0) { const unsigned i = rng.below(static_cast<unsigned>(live.size())); h.despawn(*live[i]); live.erase(live.begin() + i); }
    if (!live.empty() && rng.below(150) == 0) { Fake* f = live[rng.below(static_cast<unsigned>(live.size()))]; f->f.atp = f->f.atp == 1 ? 2 : 1; }
    for(Fake* f:live){auto& o=orc[f];if(o.bound&&!o.dead&&!o.latched&&!o.frozen&&o.p>0){++o.regen_k;if(o.regen_k>60&&(o.regen_k-60)%6==0)--o.p;}}
    h.update();
    for (Fake* f : live) {
      Orc& o = orc[f];
      // estado PRE y entorno aleatorios
      f->mode = rng.below(2) ? 4 : static_cast<int>(rng.below(9)); f->state = static_cast<int>(rng.below(5));
      f->t6c0 = rng.below(15) == 0 ? 5 : 0; f->t6c2 = rng.below(15) == 0 ? 3 : 0; f->head_lock = rng.below(15) == 0;
      f->set_bits = rng.below(15) == 0 ? 0x0B : 0x0F; f->cut = rng.below(10) == 0 ? 6 : static_cast<int>(rng.below(3));
      f->jump_cancel = rng.below(20) == 0; f->spl = rng.below(25) == 0;
      if (f->health < 50 && !o.dead) f->health = 100000;
      const Fake pre = *f;
      const bool striking = !o.dead && f->cooldown == 0 && rng.below(3) == 0;
      std::uintptr_t col = 0;
      if (striking) { col = cols[rng.below(sizeof(cols) / sizeof(cols[0]))]; f->hit_with(rng.below(2), col); }
      const std::uint64_t pr0 = h.w.native_processed;
      const std::size_t nd = h.dec.size();
      h.call(*f);
      clear_tg(*f);
      const bool candidate = pre.f.atp == 1;
      if (!candidate) o.unstable = true;
      if (candidate) o.bound = true;
      o.frozen=!candidate;
      const bool processed = h.w.native_processed > pr0;
      if (!processed || h.dec.size() == nd) continue;
      // ---- oraculo (reglas re-escritas a partir del modelo, no del codigo de reaction_core) ----
      const std::uint32_t t = h.w.types.at(f->sel);
      const bool sword_only = (t & M.sword) && !(t & (M.shield | M.arrow | M.bomb | M.iron_ball | M.boomerang | M.at40 | M.slingshot));
      const bool lethal = f->health <= 0;
      const unsigned groups = ((t & M.sword) ? 1u : 0u) + ((t & M.shield) ? 1u : 0u) + ((t & M.arrow) ? 1u : 0u) + ((t & M.bomb) ? 1u : 0u) + ((t & M.iron_ball) ? 1u : 0u) +
                              ((t & M.boomerang) ? 1u : 0u) + ((t & M.at40) ? 1u : 0u) + ((t & M.slingshot) ? 1u : 0u);
      int w = 0;
      if (!lethal && !o.latched && !o.dead && groups == 1) w = (t & M.sword) ? 25 : ((t & M.iron_ball) ? 50 : 0);
      if (!candidate) w = 0;  // UNKNOWN_UNSAFE en esta llamada: el observador no lo detalla
      const int before = o.p;
      const int after = std::min(100, before + w);
      const bool ordinary = !pre.spl && !pre.jump_cancel && pre.cut <= 2 && !(t & 0xD8000000u) && (pre.health - h.w.power) > 1;
      const bool eligible = candidate && !o.unstable && sword_only && !lethal && ordinary && w > 0 && after < 100 && pre.mode == 4 && pre.state <= 2 &&
                            pre.t6c0 == 0 && pre.t6c2 == 0 && !pre.head_lock && pre.set_bits == 0x0F && pre.cooldown == 0;
      o.p = after;
      if(w>0)o.regen_k=0;
      if (before < 100 && after >= 100) o.latched = true;
      const post05::DecisionResult& d = h.dec.back();
      const bool got = d.decision == post05::Decision::SuppressEligible;
      ++g_checks; ++g_cmp;
      const bool mode_ok = got ? (f->mode == pre.mode && f->state == pre.state && f->old_mode == pre.old_mode) : true;
      if (got != eligible || !mode_ok || h.posture(f->id) != o.p) {
        ++mism;
        if (mism < 6) std::printf("MISMATCH seed=%llu fr=%u id=%u got=%s want=%d reason=%s p=%d/%d\n", static_cast<unsigned long long>(seed), fr, f->id, post05::name(d.decision), eligible, post05::name(d.reason), h.posture(f->id), o.p);
      }
      if (lethal) { o.dead = true; f->cooldown = 999; }   // muerto: el modelo deja de golpearlo (no se revive)
      if (got) { ++g_cov_applied; ++g_cov_states[pre.state]; }
      else if (d.decision == post05::Decision::SkipUnsafe) ++g_cov_unsafe;
      else { ++g_cov_vanilla; if (d.reason == post05::Reason::CrossesThreshold) ++g_cov_threshold; }
    }
    if (fr % 1000 == 999) audit_all(h);
  }
  CHECK(mism == 0);
  CHECK(h.rx.c.verify_failed == 0 && h.rx.c.recheck_failed == 0 && h.w.bad_derefs == 0);
  audit_all(h);
}

int main() {
  t01_t02_eligible_attack(); t03_t04_threshold_and_latch(); t05_t06_lethal_and_big(); t07_other_families(); t08_finish_and_death(); t09_admission();
  t10_duplicates_and_dual(); t11_t19_generations(); t12_t13_states_and_signature(); t14_session_closed(); t15_cooldown_and_jump_cancel();
  t16_t17_t20_preservation(); t18_freeze_with_marker(); t22_recheck_and_verify(); t23_real_trace();
  for (std::uint64_t seed : {1ull, 5ull, 11ull, 77ull, 2026ull, 31337ull}) stress(seed * 0x9E3779B97F4A7C15ull + 3, 15000);
  std::printf("stress coverage: oracle_compares=%llu applied=%llu (pre_state 0/1/2 = %llu/%llu/%llu) vanilla=%llu (threshold %llu) unsafe=%llu\n", g_cmp, g_cov_applied,
              g_cov_states[0], g_cov_states[1], g_cov_states[2], g_cov_vanilla, g_cov_threshold, g_cov_unsafe);
  CHECK(g_cov_applied > 100 && g_cov_states[0] > 10 && g_cov_states[1] > 10 && g_cov_states[2] > 10 && g_cov_vanilla > 500 && g_cov_unsafe > 100 && g_cov_threshold > 10);
  std::printf("POST05_REACTION_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail == 0 ? "LAB_PASS" : "FAIL");
  return g_fail == 0 ? 0 : 1;
}
