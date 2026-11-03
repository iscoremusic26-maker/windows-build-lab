// POST04 laboratorio ADVERSARIAL de postura: motor completo (observador POST03.1 + sidecar de postura) sobre un MODELO de damage_check derivado
// de la fuente (d_a_e_oc.cpp:638-785), con un ORACULO independiente (estado por actor llevado aparte, reglas escritas de otra forma).
// LAB_PASS = consistencia del nucleo con ese modelo; NO es runtime. Las mascaras AT_TYPE son SINTETICAS.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "../src/posture_core.hpp"

using namespace post03;
namespace pp = post04::policy;

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
enum : std::uint32_t { SWORD = 0x4000000u, SHIELD = 0x10u, ARROW = 0x2000u, BOMB = 0x20u, IRON = 0x400000u, BOOM = 0x10000u, OTHER = 0x80000u, UNK = 0x80000000u };

struct World {
  std::map<std::uintptr_t, std::uint32_t> types;
  int derefs = 0, bad_derefs = 0;
  std::uint64_t native_processed = 0, native_finish = 0;
  int power = 8;
  std::map<std::uint32_t, std::uintptr_t> present;  // id -> puntero de los actores que EXISTEN en el mundo simulado
};
struct Fake {
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  ActorFacts f;
  int health = 220, mode = 3, old_mode = 0, state = 0, cooldown = 0;
  bool cutdown = false;
  bool tg[2] = {false, false};
  std::uintptr_t hit[2] = {0, 0}, sel = 0;
  Fake(std::uint32_t i, std::uintptr_t p) : id(i), ptr(p) {
    f.profile_ok = f.live = f.status_10000 = true;
    f.name = NameKind::E_OC;
    f.atp = 1;
  }
  void hit_with(int s, std::uintptr_t c) { tg[s] = true; hit[s] = c; }
};
static void set_mode(Fake& a, int m, int st) { a.old_mode = a.mode; a.mode = m; a.state = st; }
static void clear_tg(Fake& a) { a.tg[0] = a.tg[1] = false; a.hit[0] = a.hit[1] = 0; }
static void native(Fake& a, World& w) {
  if (a.cutdown) { a.cutdown = false; a.health = 0; set_mode(a, 9, 1); clear_tg(a); ++w.native_finish; return; }
  if (a.cooldown) return;
  bool had = false;
  for (int i = 0; i < 2; ++i)
    if (a.tg[i]) { a.sel = a.hit[i]; had = true; }
  if (!had) return;
  const std::uint32_t t = w.types.at(a.sel);
  a.cooldown = (t & 0xD8000000u) ? 20 : 10;
  ++w.native_processed;
  int my = 2;
  if (t & M.sword) my = 2;
  else if (t & M.shield) my = 3;
  else if (t & M.arrow) my = 0;
  else if (t & M.bomb) { my = 5; a.health = 0; }
  else if (t & M.iron_ball) { my = 5; a.health += 80; }
  else if (t & M.boomerang) my = 4;
  a.health -= w.power;
  if (a.health < 0) a.health = 0;
  if (a.health <= 1 && my < 5) my = 5;
  if (my >= 5) { set_mode(a, 6, my - 5); clear_tg(a); } else set_mode(a, 5, my);
}
static void execute_start(Fake& a) { if (a.cooldown > 0) --a.cooldown; }
static Snapshot snap_of(const Fake& a) {
  Snapshot s;
  s.health = a.health; s.mode = a.mode; s.state = a.state; s.old_mode = a.old_mode; s.cooldown = a.cooldown;
  s.cutdown = a.cutdown ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((a.tg[0] ? 1 : 0) | (a.tg[1] ? 2 : 0));
  return s;
}
static PreReads pre_of(const Fake& a) {
  PreReads r;
  r.facts = a.f;
  if (admit(a.f).adm == Admission::E_OC_CANDIDATE) {
    r.snap = snap_of(a);
    r.raw0 = a.tg[0] ? a.hit[0] : 0;
    r.raw1 = a.tg[1] ? a.hit[1] : 0;
  }
  return r;
}
static PostReads post_of(const Fake& a) {
  PostReads p;
  p.alive = a.f.profile_ok && a.f.live;
  if (p.alive) { p.snap = snap_of(a); p.selected = a.sel; }
  return p;
}

struct Hx {
  post04::PostureEngine pe;
  World w;
  std::vector<HitReport> events;
  std::vector<post04::PItem> items;
  Hx() { pe.eng.masks = M; }
  bool reader(std::uintptr_t sel, std::uint32_t& type) {
    ++w.derefs;
    auto it = w.types.find(sel);
    if (it == w.types.end() || !deref_alignment_ok(sel)) { ++w.bad_derefs; return false; }
    type = it->second;
    return true;
  }
  void drain() { post04::PItem it; while (pe.px.pop(it)) items.push_back(it); }
  void spawn(Fake& a) { w.present[a.id] = a.ptr; }
  void despawn(Fake& a) { w.present.erase(a.id); }
  bool call(Fake& a) {
    execute_start(a);
    Events ev;
    pe.on_pre(a.ptr, a.id, pre_of(a), ev);
    native(a, w);
    Events ev2; HitReport rep;
    const bool has = pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return reader(s, t); }, ev2, rep);
    if (has) events.push_back(rep);
    drain();
    return has;
  }
  Lookup lookup(const Pick& p) {
    auto it = w.present.find(p.id);
    if (it == w.present.end()) return Lookup::Absent;
    return it->second == p.ptr ? Lookup::PresentMatch : Lookup::PresentMismatchPtr;
  }
  // Replica de do_update() de mod.cpp: barrido por presupuesto cada 10 llamadas, DESPUES el tick matematico (siempre).
  void update() {
    Observer& o = pe.eng.obs;
    if (o.sweep_due()) {
      for (unsigned q = 0; q < kSweepQuota; ++q) {
        Pick p;
        if (!o.ledger.next_sweep(p, o.sweep_stamp)) break;
        Events ev;
        o.ledger.apply_lookup(p, lookup(p), o.epoch.epoch, ev);
      }
    }
    pe.on_update();
    drain();
  }
  void ticks(unsigned n) { for (unsigned i = 0; i < n; ++i) update(); }
  // un cuadro realista: mod_update y luego la ejecucion de cada actor (que llama a damage_check)
  void frame(std::vector<Fake*>& as) { update(); for (Fake* a : as) call(*a); }
  int slot_of(std::uint32_t id) const {
    for (unsigned i = 0; i < kCapacity; ++i) {
      const Instance& s = pe.eng.obs.ledger.slot(i);
      if (s.id == id && (s.life == Life::Active || s.life == Life::DeathSeen || s.life == Life::Unverified)) return static_cast<int>(i);
    }
    return -1;
  }
  const post04::Entry* ent(std::uint32_t id) const { const int s = slot_of(id); return s < 0 ? nullptr : &pe.px.entry(static_cast<unsigned>(s)); }
  int posture(std::uint32_t id) const { const post04::Entry* e = ent(id); return e && e->bound ? e->posture : -1; }
  std::uint64_t skipped(post04::Skip s) const { return pe.px.c.skipped[static_cast<unsigned>(s)]; }
  unsigned items_of(post04::PKind k) const { unsigned n = 0; for (auto& i : items) n += i.kind == k; return n; }
};
static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }

// golpe aislado: espera a que venza el cooldown (llamadas sin TG), golpea UNA llamada, limpia el TG como hace el sistema de colisiones
static bool strike(Hx& h, Fake& a, std::uintptr_t coll, int sphere = 1) {
  while (a.cooldown > 0 && a.health > 0) h.call(a);
  a.hit_with(sphere, coll);
  const bool has = h.call(a);
  clear_tg(a);
  return has;
}
static void audit_ok(Hx& h) {
  const post04::AuditResult au = h.pe.px.audit();
  CHECK(au.ok && au.e1 && au.e2 && au.e3 && au.e4 && au.e5 && au.invariants);
  CHECK(h.pe.eng.conserved());
}
static void types(Hx& h) {
  h.w.types[P(1)] = SWORD; h.w.types[P(2)] = SWORD; h.w.types[P(3)] = IRON; h.w.types[P(4)] = BOOM; h.w.types[P(5)] = ARROW;
  h.w.types[P(6)] = OTHER; h.w.types[P(7)] = SWORD | ARROW; h.w.types[P(8)] = BOMB; h.w.types[P(9)] = UNK;
}

// ---------------------------------------------------------------------------------------------------------------------
static void t01_bind_init() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  h.call(a);
  const post04::Entry* e = h.ent(10);
  CHECK(e && e->bound && e->posture == 0 && !e->latched && !e->dead && !e->frozen && e->state() == post04::PState::Tracking);
  CHECK(e && e->id == 10 && e->generation == h.pe.eng.obs.ledger.slot(static_cast<unsigned>(h.slot_of(10))).generation && e->k == 0);
  CHECK(h.pe.px.c.binds == 1 && h.pe.px.c.rebinds == 0 && h.items_of(post04::PKind::Bind) == 1);
  CHECK(h.pe.px.c.weighted == 0 && h.pe.px.c.confirmed_consumable == 0);
  audit_ok(h);
}
static void t02_sword_x4() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  for (int i = 1; i <= 3; ++i) { CHECK(strike(h, a, P(1))); CHECK(h.posture(10) == 25 * i); CHECK(!h.ent(10)->latched); }
  CHECK(strike(h, a, P(1)));
  CHECK(h.posture(10) == 100 && h.ent(10)->latched && h.ent(10)->state() == post04::PState::BreakReadyLatched);
  CHECK(h.pe.px.c.crossings == 1 && h.items_of(post04::PKind::BreakReady) == 1);
  CHECK(strike(h, a, P(1)));                       // 5.o golpe: procesado por el juego, omitido por la postura
  CHECK(h.posture(10) == 100 && h.skipped(post04::Skip::Latched) == 1 && h.pe.px.c.crossings == 1 && h.items_of(post04::PKind::BreakReady) == 1);
  CHECK(h.pe.px.c.weighted == 4 && h.pe.px.c.confirmed_consumable == 5 && h.pe.px.c.units_added == 100);
  audit_ok(h);
}
static void t03_iron() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(3)) && h.posture(10) == 50 && !h.ent(10)->latched);
  CHECK(strike(h, a, P(3)) && h.posture(10) == 100 && h.ent(10)->latched && h.pe.px.c.crossings == 1 && h.pe.px.c.saturations == 0);
  audit_ok(h);
  Hx g; types(g);                                  // saturacion exacta: 75 + 50 = 65 -> 100, sin desbordar
  Fake b(11, 0x9400); g.spawn(b);
  CHECK(strike(g, b, P(1)) && strike(g, b, P(1)) && strike(g, b, P(1)) && g.posture(11) == 75);
  CHECK(strike(g, b, P(3)) && g.posture(11) == 100 && g.pe.px.c.saturations == 1 && g.pe.px.c.units_added == 100 && g.pe.px.c.crossings == 1);
  audit_ok(g);
}
static void t04_mixed() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(1)) && strike(h, a, P(3)) && h.posture(10) == 75 && !h.ent(10)->latched);
  CHECK(strike(h, a, P(1)) && h.posture(10) == 100 && h.ent(10)->latched && h.pe.px.c.crossings == 1);
  audit_ok(h);
}
static void t05_dual_tg() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  a.hit_with(0, P(1)); a.hit_with(1, P(1));       // misma espada en las dos esferas
  CHECK(h.call(a));
  clear_tg(a);
  CHECK(h.posture(10) == 25 && h.events.size() == 1 && h.events[0].j.dual_tg && h.pe.px.c.weighted == 1 && h.w.derefs == 1);
  Hx g; types(g);                                  // colliders distintos: cc1 prevalece; un solo peso
  Fake b(11, 0x9400); g.spawn(b);
  b.hit_with(0, P(1)); b.hit_with(1, P(3));
  CHECK(g.call(b));
  clear_tg(b);
  CHECK(g.events.size() == 1 && g.events[0].col.pair_distinct && g.events[0].fam.family == Family::IronBall && g.posture(11) == 50 && g.w.derefs == 1);
  audit_ok(g);
}
static void t06_duplicates() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(1)));
  CHECK(h.posture(10) == 25);
  const HitReport rep = h.events[0];
  h.pe.px.consume(rep, h.pe.eng.obs.ledger);       // el MISMO informe otra vez
  h.pe.px.consume(rep, h.pe.eng.obs.ledger);
  CHECK(h.posture(10) == 25 && h.pe.px.c.dup_consume_calls == 2 && h.pe.px.c.weighted == 1 && h.pe.px.c.multi_consume == 0);
  // POST duplicado y callback huerfano: el motor los cuenta y la postura no cambia
  Events ev; HitReport r2;
  const bool dup = h.pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return h.reader(s, t); }, ev, r2);
  CHECK(!dup && h.posture(10) == 25 && (h.pe.eng.obs.ledger.c.dup_post_orphan + h.pe.eng.obs.ledger.c.unmatched_post) == 1);
  Fake ghost(99, 0xA000);                          // POST de un actor sin PRE
  const bool orphan = h.pe.on_post(ghost.ptr, post_of(ghost), [&](std::uintptr_t, std::uint32_t&) { return false; }, ev, r2);
  CHECK(!orphan && h.pe.px.c.weighted == 1 && h.ent(99) == nullptr);
  // replay del mismo frame de motor: la puerta de serial lo suprime
  CHECK(h.pe.eng.conserved());
  audit_ok(h);
}
static void t07_hp_delta_zero() {
  Hx h; types(h);
  h.w.power = 0;                                   // HIT_CONFIRMED sin perdida de HP
  Fake a(10, 0x9000); h.spawn(a);
  const int hp0 = a.health;
  CHECK(strike(h, a, P(1)) && a.health == hp0 && h.posture(10) == 25);        // SWORD con delta=0: +25 (no se deriva del HP)
  CHECK(h.events[0].pre.health == h.events[0].post.health && h.events[0].j.kind == HitKind::Confirmed);
  CHECK(strike(h, a, P(4)) && h.posture(10) == 25 && h.skipped(post04::Skip::ZeroWeight) == 1);  // BOOMERANG: +0 por politica
  CHECK(h.events[1].fam.family == Family::Boomerang && h.events[1].j.kind == HitKind::Confirmed);
  audit_ok(h);
}
static void t08_lethal_finish() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a); a.health = 5;    // el golpe de espada (8) lo mata
  CHECK(strike(h, a, P(1)));
  CHECK(h.events[0].j.lethal && h.skipped(post04::Skip::Lethal) == 1 && h.pe.px.c.weighted == 0);
  CHECK(h.ent(10)->dead && h.ent(10)->posture == 0 && h.ent(10)->state() == post04::PState::InactiveDeath && h.pe.px.c.death_closed == 1);
  h.ticks(500);
  CHECK(h.ent(10)->posture == 0);                  // el muerto no recupera ni acumula
  Hx g; types(g);                                  // IRON letal
  Fake b(11, 0x9400); g.spawn(b); b.health = 5;
  CHECK(strike(g, b, P(3)));                       // iron: +80 de vida en el modelo, no letal
  Hx j; types(j);
  Fake c(12, 0x9800); j.spawn(c); c.health = 5;
  CHECK(strike(j, c, P(8)) && j.skipped(post04::Skip::Lethal) == 1);                 // bomba: letal, sin peso
  // Finish nativo: nunca pesa, no es un golpe
  Hx k; types(k);
  Fake d(13, 0x9C00); k.spawn(d);
  CHECK(strike(k, d, P(1)) && k.posture(13) == 25);
  d.cutdown = true;
  CHECK(k.call(d));
  CHECK(k.events.back().j.kind == HitKind::FinishNative && k.pe.px.c.finish_events == 1 && k.posture(13) == 25 && k.pe.px.c.weighted == 1);
  CHECK(k.ent(13)->dead);
  audit_ok(h); audit_ok(g); audit_ok(j); audit_ok(k);
}
static void t09_other_unknown() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(6)) && h.events[0].fam.family == Family::Other && h.posture(10) == 0 && h.skipped(post04::Skip::ZeroWeight) == 1);   // at=0x00080000
  CHECK(strike(h, a, P(9)) && h.events[1].fam.family == Family::Other && h.posture(10) == 0);                                              // bits desconocidos
  CHECK(strike(h, a, P(5)) && h.events[2].fam.family == Family::Arrow && h.posture(10) == 0);                                              // flecha: sin politica
  CHECK(strike(h, a, P(8)) && h.skipped(post04::Skip::Lethal) + h.skipped(post04::Skip::ZeroWeight) == 4);                                 // bomba mata (HP 0): letal
  Hx g; types(g);                                  // collider no coincidente o mal alineado: sin lectura, UNKNOWN_FAMILY
  Fake b(11, 0x9400); g.spawn(b);
  g.w.types[0x100002u] = SWORD;                    // mal alineado a 4
  CHECK(strike(g, b, 0x100002u) && g.w.bad_derefs == 0 && g.w.derefs == 0 && g.skipped(post04::Skip::UnknownFamily) == 1 && g.posture(11) == 0);
  audit_ok(h); audit_ok(g);
}
static void t10_hm_independence() {
  int out[3];
  const int powers[3] = {8, 16, 0};
  for (int i = 0; i < 3; ++i) {
    Hx h; types(h); h.w.power = powers[i];
    Fake a(10, 0x9000); h.spawn(a);
    strike(h, a, P(1)); strike(h, a, P(3));
    out[i] = h.posture(10);
  }
  CHECK(out[0] == 75 && out[1] == 75 && out[2] == 75);   // el dano (x1/x2/0) no cambia los puntos
}
// Oraculo de recuperacion: tras un golpe ponderado, unidades recuperadas tras k ticks elegibles = (k > D) ? (k - D) / R : 0
static int regen_units(unsigned k) { return k > 60 ? static_cast<int>((k - 60) / 6) : 0; }
static void t11_regen_edges() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(1)) && h.posture(10) == 25);
  std::vector<Fake*> as{&a};
  for (unsigned k = 1; k <= 400; ++k) {
    h.update();                                     // solo ticks: el actor no llama al gancho
    const int want = 25 - regen_units(k);
    const int expect = want < 0 ? 0 : want;
    CHECK(h.posture(10) == expect);
    if (k == 59 || k == 60 || k == 61 || k == 65) CHECK(h.posture(10) == 25);   // borde exacto: nada hasta k=60 inclusive y ninguna unidad hasta k=66
    if (k == 66) CHECK(h.posture(10) == 24);
    if (k == 67 || k == 71) CHECK(h.posture(10) == 24);
    if (k == 72) CHECK(h.posture(10) == 23);
  }
  CHECK(h.posture(10) == 0 && h.pe.px.c.units_recovered == 25 && h.pe.px.c.regen_runs == 1 && h.items_of(post04::PKind::Regen) == 1);
  h.ticks(100);
  CHECK(h.posture(10) == 0 && h.pe.px.c.underflow == 0 && h.pe.px.c.units_recovered == 25);
  audit_ok(h);
}
static void t12_hit_resets_delay() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1));
  h.ticks(40);
  CHECK(h.posture(10) == 25);
  strike(h, a, P(1));                                // reinicia el retardo
  CHECK(h.posture(10) == 50 && h.ent(10)->k == 0);
  h.ticks(60);
  CHECK(h.posture(10) == 50);                        // 100 + 60 ticks desde el 1.o golpe, pero solo 60 desde el 2.o
  h.ticks(5);
  CHECK(h.posture(10) == 50);
  h.ticks(1);
  CHECK(h.posture(10) == 49);                        // 66.o tick elegible desde el 2.o golpe
  // orden hook/update: golpe EN el cuadro (despues del update) no recupera en ese mismo tick
  Hx g; types(g);
  Fake b(11, 0x9400); g.spawn(b);
  strike(g, b, P(1));
  g.ticks(65);
  CHECK(g.posture(11) == 25);
  strike(g, b, P(1));                                // golpe justo antes del tick 66: reinicia k a 0 y no hay recuperacion simultanea
  CHECK(g.posture(11) == 50);
  g.ticks(1);
  CHECK(g.posture(11) == 50 && g.ent(11)->k == 1);
  audit_ok(h); audit_ok(g);
}
static void t13_latched_frozen() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  for (int i = 0; i < 4; ++i) strike(h, a, P(1));
  CHECK(h.posture(10) == 100 && h.ent(10)->latched);
  h.ticks(2000);
  CHECK(h.posture(10) == 100 && h.pe.px.c.crossings == 1 && h.items_of(post04::PKind::BreakReady) == 1);   // ni recupera ni reemite
  strike(h, a, P(3)); strike(h, a, P(1));
  CHECK(h.posture(10) == 100 && h.pe.px.c.crossings == 1 && h.skipped(post04::Skip::Latched) == 2);
  CHECK(h.pe.px.c.latched_entry_ticks == 2000);
  // el marcador no pasa a la generacion siguiente
  h.despawn(a);
  h.ticks(200);                                      // el barrido verifica la ausencia y retira
  CHECK(h.ent(10) == nullptr);
  Fake a2(10, 0x9000); h.spawn(a2);                  // mismo id y misma direccion, nueva vida
  h.call(a2);
  CHECK(h.posture(10) == 0 && !h.ent(10)->latched && h.pe.px.c.latched_retired == 1);
  CHECK(strike(h, a2, P(1)) && h.posture(10) == 25 && h.pe.px.c.crossings == 1);
  audit_ok(h);
}
static void t14_two_actors() {
  Hx h; types(h);
  Fake a(10, 0x9000), b(11, 0x9400); h.spawn(a); h.spawn(b);
  strike(h, a, P(1)); strike(h, b, P(3)); strike(h, b, P(1));
  CHECK(h.posture(10) == 25 && h.posture(11) == 75);
  h.ticks(66);
  CHECK(h.posture(10) == 24 && h.posture(11) == 74);                 // cada uno con su propio reloj elegible
  strike(h, a, P(1));                                                 // golpear a no afecta a b
  h.ticks(6);
  CHECK(h.posture(10) == 49 && h.posture(11) == 73);
  audit_ok(h);
}
static void t15_no_carry_over() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  for (int i = 0; i < 3; ++i) strike(h, a, P(1));
  CHECK(h.posture(10) == 75);
  const std::uint64_t gen_a = h.ent(10)->generation;
  h.despawn(a);
  h.ticks(60);
  CHECK(h.ent(10) == nullptr);
  // (1) ID reciclado con otra direccion
  Fake b(10, 0x9800); h.spawn(b); h.call(b);
  CHECK(h.posture(10) == 0 && h.ent(10)->generation != gen_a && h.ent(10)->k == 0 && !h.ent(10)->latched);
  // (2) otra ID en la misma direccion de un actor vivo: el viejo se retira (ADDRESS_REUSED), sin traslado
  strike(h, b, P(1));
  Fake c(77, 0x9800); h.spawn(c); h.despawn(b);
  const int pre = h.posture(10);
  h.call(c);
  CHECK(pre == 25 && h.posture(77) == 0 && h.ent(10) == nullptr);
  bool saw_reuse = false;
  for (auto& it : h.items) if (it.kind == post04::PKind::Retire && std::strcmp(it.reason, "ADDRESS_REUSED_BY_OTHER_ID") == 0) { saw_reuse = true; CHECK(!it.delete_claim); }
  CHECK(saw_reuse);
  // (3) mismo ID con direccion distinta mientras el anterior sigue vivo: IDENTITY_CONFLICT, sin delete_claim
  Fake d(88, 0xB000); h.spawn(d); strike(h, d, P(3));
  Fake d2(88, 0xB800); h.spawn(d2); h.call(d2);
  CHECK(h.posture(88) == 0 && h.pe.eng.obs.ledger.c.identity_conflicts == 1);
  for (auto& it : h.items) if (it.kind == post04::PKind::Retire && it.reason && std::strcmp(it.reason, "IDENTITY_CONFLICT_SAME_ID_OTHER_ADDRESS") == 0) CHECK(!it.delete_claim);
  // (4) cambio de contexto (epoch): el vivo no verificado se retira como CONTEXT_RETIRED (sin delete_claim)
  Hx e; types(e);
  Fake f(55, 0xC000); e.spawn(f); strike(e, f, P(1));
  e.w.present[55] = 0xDEAD00;                       // el mundo devuelve OTRO puntero: LOOKUP_PTR_MISMATCH -> Unverified
  e.ticks(30);
  CHECK(e.ent(55) && e.ent(55)->frozen && e.ent(55)->posture == 25);
  e.pe.eng.obs.epoch.update(post03::stage_token("F_SP103")); e.pe.eng.obs.epoch.update(post03::stage_token("F_SP104"));
  e.ticks(30);
  CHECK(e.ent(55) == nullptr);
  for (auto& it : e.items) if (it.kind == post04::PKind::Retire) CHECK(it.delete_claim == (std::strcmp(it.reason, "LOOKUP_ABSENT_VERIFIED") == 0));
  // (5) wrap controlado de la generacion
  Hx w; types(w); w.pe.eng.obs.ledger.set_generation_for_test(~0ull - 1);
  Fake g1(1, 0x1000), g2(2, 0x1400), g3(3, 0x1800); w.spawn(g1); w.spawn(g2); w.spawn(g3);
  w.call(g1); w.call(g2); w.call(g3);
  CHECK(w.pe.px.c.binds == 3 && w.pe.eng.obs.ledger.c.generation_rollovers == 1 && w.pe.px.c.generation_leak == 0);
  strike(w, g1, P(1)); strike(w, g3, P(3));
  CHECK(w.posture(1) == 25 && w.posture(2) == 0 && w.posture(3) == 50);
  audit_ok(h); audit_ok(e); audit_ok(w);
}
static void t16_unsafe_freeze() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1));
  CHECK(h.posture(10) == 25);
  std::vector<Fake*> as{&a};
  for (int i = 0; i < 40; ++i) h.frame(as);
  CHECK(h.ent(10)->k == 40);
  a.f.atp = 2;                                       // AtAtp distinto: UNKNOWN_UNSAFE, identidad conservada
  h.frame(as);
  CHECK(h.ent(10)->frozen && h.ent(10)->state() == post04::PState::FrozenUnverified && h.pe.eng.obs.ledger.c.admission_changes == 1);
  const std::uint64_t gen = h.ent(10)->generation;
  const std::uint64_t k_at_freeze = h.ent(10)->k;
  for (int i = 0; i < 500; ++i) h.frame(as);
  CHECK(h.ent(10)->k == k_at_freeze && h.posture(10) == 25 && h.ent(10)->generation == gen);   // congelado: ni recupera ni cuenta tiempo
  strike(h, a, P(1));                                // golpe durante la congelacion: el juego lo procesa; la postura no lo recibe
  CHECK(h.posture(10) == 25 && h.pe.eng.obs.ledger.c.confirmed == 1 + 0);
  a.f.atp = 1;
  h.frame(as);
  CHECK(!h.ent(10)->frozen && h.ent(10)->generation == gen && h.pe.px.c.freeze_events == 1 && h.pe.px.c.thaw_events == 1);
  const std::uint64_t k_after_thaw = h.ent(10)->k;
  CHECK(k_after_thaw <= k_at_freeze + 2);            // no se aplican de golpe los ~500 ticks congelados
  while (h.ent(10)->k < 65) h.frame(as);
  CHECK(h.posture(10) == 25);
  h.frame(as);
  CHECK(h.posture(10) == 24);
  CHECK(h.pe.px.c.frozen_entry_ticks >= 500);
  audit_ok(h);
}
static void t17_excluded() {
  Hx h; types(h);
  Fake a(10, 0x9000); a.f.name = NameKind::E_OC2; h.spawn(a);
  h.call(a); a.hit_with(1, P(1)); h.call(a); clear_tg(a);
  CHECK(h.ent(10) == nullptr || !h.ent(10)->bound);
  CHECK(h.pe.px.c.binds == 0 && h.pe.px.c.excluded_generations == 1 && h.pe.px.c.confirmed_consumable == 0 && h.events.empty());
  Fake b(11, 0x9400); b.f.profile_ok = false; h.spawn(b); h.call(b);
  CHECK(h.pe.px.c.binds == 0 && h.pe.px.c.excluded_generations == 2);
  h.ticks(300);
  CHECK(h.pe.px.c.units_added == 0 && h.pe.px.c.units_recovered == 0);
  // un actor candidato que pasa a perfil excluido: se retira sin traslado
  Fake c(12, 0x9800); h.spawn(c); strike(h, c, P(1)); c.f.name = NameKind::E_OC2; h.call(c);
  CHECK(h.pe.px.c.binds == 1);
  audit_ok(h);
}
static void t18_lifecycle_states() {
  Hx h; types(h);
  Fake a(10, 0x9000), b(11, 0x9400), c(12, 0x9800); h.spawn(a); h.spawn(b); h.spawn(c);
  strike(h, a, P(1)); strike(h, b, P(1)); strike(h, c, P(1));
  // En creacion / no disponible: congelado, sin delete_claim y sin uso del puntero guardado
  h.pe.eng.obs.ledger.c.lookups += 0;
  Pick pk; pk.slot = h.slot_of(10); pk.id = 10; pk.ptr = 0x9000; pk.generation = h.ent(10)->generation;
  Events ev;
  h.pe.eng.obs.ledger.apply_lookup(pk, Lookup::Creating, 1, ev);
  h.pe.px.sync(h.pe.eng.obs.ledger);
  CHECK(h.ent(10)->frozen && h.ent(10)->posture == 25);
  h.w.present[10] = 0xBAD000;                          // el barrido del lab seguiria devolviendo «no verificado» (puntero distinto)
  h.ticks(20);
  CHECK(h.ent(10)->k == 0 && h.posture(10) == 25 && h.ent(10)->frozen);
  h.w.present[10] = 0x9000;
  h.pe.eng.obs.ledger.apply_lookup(pk, Lookup::PresentMatch, 1, ev);   // revalida
  h.pe.px.sync(h.pe.eng.obs.ledger);
  CHECK(!h.ent(10)->frozen && h.posture(10) == 25);
  // solo la AUSENCIA verificada produce delete_claim
  h.despawn(c);
  h.ticks(300);
  unsigned claims = 0, retires = 0;
  for (auto& it : h.items) if (it.kind == post04::PKind::Retire) { ++retires; claims += it.delete_claim; CHECK(it.delete_claim == (std::strcmp(it.reason, "LOOKUP_ABSENT_VERIFIED") == 0)); }
  CHECK(retires >= 1 && claims == retires && h.ent(12) == nullptr && h.ent(10) != nullptr);
  // muerte vista: cierra pero NO retira ni declara borrado
  b.cutdown = true; h.call(b);
  CHECK(h.ent(11)->dead && h.pe.px.c.death_closed >= 1);
  for (auto& it : h.items) if (it.kind == post04::PKind::Retire) CHECK(it.id != 11 || it.delete_claim == 0 || std::strcmp(it.reason, "LOOKUP_ABSENT_VERIFIED") == 0);
  audit_ok(h);
}
static void t19_capacity() {
  Hx h; types(h);
  std::vector<Fake> as;
  as.reserve(70);
  for (unsigned i = 0; i < 64; ++i) as.emplace_back(1000 + i, 0x200000u + i * 0x400u);
  for (auto& a : as) { h.spawn(a); h.call(a); }
  CHECK(h.pe.px.c.binds == 64 && h.pe.eng.obs.ledger.active() == 64);
  as.emplace_back(2000, 0x300000u); Fake& extra = as.back(); h.spawn(extra);
  h.call(extra);                                       // el 65.o se rechaza de forma explicita; nada de los vivos se pierde
  CHECK(h.pe.eng.obs.ledger.c.capacity_drops == 1 && h.pe.px.c.binds == 64 && h.ent(2000) == nullptr);
  strike(h, extra, P(1));
  CHECK(h.pe.px.c.confirmed_consumable == 0 && h.pe.px.c.weighted == 0);
  for (unsigned i = 0; i < 64; ++i) CHECK(h.ent(1000 + i) && h.ent(1000 + i)->bound);
  strike(h, as[0], P(1));
  CHECK(h.posture(1000) == 25);
  // retirar algunos y dar de alta otros: el slot se REUTILIZA con nueva generacion
  for (unsigned i = 0; i < 10; ++i) h.despawn(as[i]);
  h.ticks(300);
  CHECK(h.ent(1000) == nullptr);
  Fake n1(3000, 0x400000u); h.spawn(n1); h.call(n1);
  CHECK(h.ent(3000) && h.posture(3000) == 0 && h.pe.px.c.rebinds >= 1);
  audit_ok(h);
  unsigned bound = 0;
  for (unsigned i = 0; i < kCapacity; ++i) bound += h.pe.px.entry(i).bound ? 1u : 0u;
  CHECK(bound == h.pe.px.audit().bound_now && bound <= kCapacity);
  // divergencia forzada: el ledger cambia sin pasar por la envoltura; sync lo repara sin heredar
  Hx d; types(d);
  Fake x(5, 0x5000); d.spawn(x); d.call(x); strike(d, x, P(1));
  Events ev;
  d.pe.eng.on_pre(0x7000, 6, pre_of(Fake(6, 0x7000)), ev);       // alta directa en el ledger, sin sync
  CHECK(d.pe.px.c.binds == 1);
  d.pe.px.sync(d.pe.eng.obs.ledger);
  CHECK(d.pe.px.c.binds == 2);
}
static void t20_clock_wrap_and_sweep() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1));
  h.pe.px.set_adv_seq_for_test(~0ull - 20);          // el sello de avance cruza el wrap en medio de la recuperacion
  h.ticks(400);
  CHECK(h.pe.px.c.seq_wraps == 1 && h.posture(10) == 0 && h.pe.px.c.units_recovered == 25);   // ni se multiplica ni se omite ninguna unidad
  // el barrido cada 10 mod_update NO es el reloj: con 1 actor y con 20 actores el ritmo de recuperacion es identico
  for (int n : {1, 20}) {
    Hx g; types(g);
    std::vector<Fake> as; as.reserve(25);
    for (int i = 0; i < n; ++i) as.emplace_back(100 + i, 0x500000u + i * 0x400u);
    for (auto& f : as) { g.spawn(f); g.call(f); }
    strike(g, as[0], P(1));
    g.ticks(150);
    CHECK(g.posture(100) == 25 - regen_units(150));
    CHECK(g.pe.px.c.ticks == 150);
  }
}
static void t21_declared_timebase() {
  // Comportamiento DECLARADO (no se afirma nada sobre pausas reales): el tick avanza con mod_update, con o sin llamadas al gancho.
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1));
  for (unsigned i = 0; i < 66; ++i) h.pe.on_update();            // sin barridos ni llamadas del actor (p. ej. un menu sin damage_check)
  CHECK(h.posture(10) == 24);
  Hx g; types(g);
  Fake b(11, 0x9400); g.spawn(b);
  strike(g, b, P(1));
  for (int i = 0; i < 1000; ++i) g.call(b);                       // llamadas al gancho SIN mod_update: no hay tiempo
  CHECK(g.posture(11) == 25 && g.ent(11)->k == 0);
}
static void t22_disabled() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1));
  h.pe.eng.obs.disable("TEST");
  const post04::PCounters before = h.pe.px.c;
  CHECK(strike(h, a, P(1)) == false);
  h.ticks(300);
  CHECK(h.pe.px.c.weighted == before.weighted && h.pe.px.c.ticks == before.ticks && h.pe.px.c.consume_calls == before.consume_calls && h.posture(10) == 25);
  CHECK(h.pe.px.pending_items() == 0);
  // pasividad: las lecturas entran por valor (PreReads/PostReads); el motor no tiene referencias al actor simulado
  Hx g; types(g);
  Fake b(11, 0x9400); g.spawn(b);
  const Fake b0 = b;
  Events e1, e2; HitReport rp;
  g.pe.on_pre(b.ptr, b.id, pre_of(b), e1);
  g.pe.on_post(b.ptr, post_of(b), [&](std::uintptr_t, std::uint32_t&) { return false; }, e2, rp);
  CHECK(std::memcmp(&b0.health, &b.health, sizeof(int)) == 0 && b0.mode == b.mode && b0.state == b.state && b0.cooldown == b.cooldown && b0.sel == b.sel &&
        b0.tg[0] == b.tg[0] && b0.tg[1] == b.tg[1] && b0.old_mode == b.old_mode && b0.cutdown == b.cutdown && b0.f.atp == b.f.atp);
}
static void t23_logs_and_budget() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  strike(h, a, P(1));
  char line[512];
  for (auto& it : h.items) {
    const int n = post04::fmt_item(line, sizeof(line), it);
    CHECK(n > 0 && n < 400 && std::strstr(line, "POST04_") == line);
    CHECK(std::strstr(line, "0x") == nullptr || it.kind == post04::PKind::Hit);        // solo la mascara AT de 8 digitos
    const char* x = std::strstr(line, "0x");
    if (x) { unsigned len = 0; for (const char* c = x + 2; std::isxdigit(static_cast<unsigned char>(*c)); ++c) ++len; CHECK(len == 8); }
  }
  const post04::AuditResult au = h.pe.px.audit();
  CHECK(post04::fmt_summary(line, sizeof(line) , h.pe.px, au, "TEST") > 0);
  char big[2048];
  CHECK(post04::fmt_summary(big, sizeof(big), h.pe.px, au, "TEST") < static_cast<int>(sizeof(big)));
  CHECK(post04::fmt_conservation(big, sizeof(big), h.pe.px, au, "TEST") < static_cast<int>(sizeof(big)));
  // presupuesto: 5000 golpes sin leer el ring -> el ring no se desborda sin contar; con log_take el total nunca pasa de 500
  Hx g; types(g);
  Fake b(11, 0x9400); g.spawn(b);
  unsigned taken = 0;
  for (int i = 0; i < 5000; ++i) { if (g.pe.px.log_take(post04::PKind::Hit)) ++taken; }
  CHECK(taken <= 250);
  for (int i = 0; i < 5000; ++i) { for (int k = 0; k < 5; ++k) if (g.pe.px.log_take(static_cast<post04::PKind>(k))) ++taken; }
  CHECK(taken <= post04::Posture::kLogBudget);
  Hx r; types(r);
  Fake c(12, 0x9800); r.spawn(c); c.health = 1000000;
  for (int i = 0; i < 200; ++i) { c.hit_with(1, P(6)); execute_start(c); Events e1; r.pe.on_pre(c.ptr, c.id, pre_of(c), e1); native(c, r.w); Events e2; HitReport rep; r.pe.on_post(c.ptr, post_of(c), [&](std::uintptr_t s, std::uint32_t& t) { return r.reader(s, t); }, e2, rep); clear_tg(c); c.cooldown = 0; }
  CHECK(r.pe.px.c.ring_dropped > 0 || r.pe.px.pending_items() <= post04::Posture::ring_capacity());   // sin drenar: el exceso se CUENTA, nunca se pierde en silencio
  CHECK(r.pe.px.c.skipped[static_cast<unsigned>(post04::Skip::ZeroWeight)] == 200);
}
static void t24_ambiguous() {
  Hx h; types(h);
  Fake a(10, 0x9000); h.spawn(a);
  CHECK(strike(h, a, P(7)));                          // SWORD|ARROW: la rama nativa seria SWORD, la familia es AMBIGUOUS
  CHECK(h.events[0].fam.family == Family::Ambiguous && h.events[0].fam.native_first == Family::Sword);
  CHECK(h.posture(10) == 0 && h.skipped(post04::Skip::Ambiguous) == 1 && h.pe.px.c.weighted == 0);
  audit_ok(h);
}
static void t25_collider_regression() {
  struct Case { std::uintptr_t ptr; bool reads_ok; };
  const Case cs[] = {{0x200000, true}, {0x200004, true}, {0x200002, false}, {0x8, false}};
  for (const Case& c : cs) {
    Hx h; types(h); h.w.types[c.ptr] = SWORD;
    Fake a(10, 0x9000); h.spawn(a);
    strike(h, a, c.ptr);
    CHECK(h.w.derefs <= 1 && h.w.bad_derefs == 0);
    CHECK(h.posture(10) == (c.reads_ok ? 25 : 0));
    CHECK(h.pe.eng.obs.ledger.c.family_count[static_cast<unsigned>(Family::Sword)] == (c.reads_ok ? 1u : 0u));
    audit_ok(h);
  }
  Hx h2; types(h2);                                   // mpCollider distinto del hit obj crudo: nunca se lee
  Fake a(10, 0x9000); h2.spawn(a);
  a.hit_with(1, P(1));
  execute_start(a);
  Events ev; h2.pe.on_pre(a.ptr, a.id, pre_of(a), ev);
  native(a, h2.w);
  PostReads pr = post_of(a); pr.selected = P(2);
  Events ev2; HitReport rep;
  const bool has = h2.pe.on_post(a.ptr, pr, [&](std::uintptr_t s, std::uint32_t& t) { return h2.reader(s, t); }, ev2, rep);
  CHECK(has && h2.w.derefs == 0 && h2.pe.px.c.weighted == 0 && h2.skipped(post04::Skip::UnknownFamily) == 1);
}

// ---- T27: estres aleatorio con semillas fijas y ORACULO independiente --------------------------------------------------------------------
struct Rng { std::uint64_t s; std::uint64_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; } unsigned below(unsigned n) { return static_cast<unsigned>(next() % n); } };
struct Orc {                                         // estado del oraculo por actor (NO comparte codigo ni estructuras con Posture)
  bool bound = false, dead = false, frozen = false, latched = false;
  int p = 0;
  unsigned k = 0;
};
struct Actor { Fake f; Orc o; bool called = true; unsigned rate = 14; Actor(std::uint32_t i, std::uintptr_t p) : f(i, p) {} };
static int oracle_weight(std::uint32_t type) {          // por TIPO de collider, sin pasar por classify_family
  const bool sword = (type & M.sword) != 0, iron = (type & M.iron_ball) != 0;
  const unsigned groups = (type & M.sword ? 1u : 0u) + (type & M.shield ? 1u : 0u) + (type & M.arrow ? 1u : 0u) + (type & M.bomb ? 1u : 0u) +
                          (type & M.iron_ball ? 1u : 0u) + (type & M.boomerang ? 1u : 0u) + (type & M.at40 ? 1u : 0u) + (type & M.slingshot ? 1u : 0u);
  if (groups != 1) return 0;                            // 0 grupos = OTHER, >1 = AMBIGUOUS
  return sword ? 25 : (iron ? 50 : 0);
}
static unsigned long long g_stress_compares = 0, g_cov_weighted = 0, g_cov_cross = 0, g_cov_recov = 0, g_cov_freeze = 0, g_cov_binds = 0, g_cov_skips = 0, g_cov_runs = 0, g_cov_dead = 0;
static void stress(std::uint64_t seed, unsigned frames) {
  Hx h; types(h);
  Rng rng{seed};
  std::vector<std::unique_ptr<Actor>> all;
  std::vector<Actor*> live;
  std::vector<std::uintptr_t> old_ptrs;
  std::uint32_t next_id = 1000;
  std::uintptr_t next_ptr = 0x600000;
  const std::uintptr_t cols[6] = {P(1), P(3), P(4), P(5), P(6), P(7)};
  std::uint64_t o_weighted = 0, o_cross = 0, o_units = 0;
  unsigned mism = 0;
  for (unsigned fr = 0; fr < frames; ++fr) {
    if (live.size() < 6 && rng.below(40) == 0) {
      std::uintptr_t ptr = next_ptr;
      next_ptr += 0x400;
      if (!old_ptrs.empty() && rng.below(4) == 0) {   // reciclaje de direccion (solo si ningun vivo la usa)
        const std::uintptr_t cand = old_ptrs[rng.below(static_cast<unsigned>(old_ptrs.size()))];
        bool used = false;
        for (Actor* a : live) used = used || a->f.ptr == cand;
        if (!used) ptr = cand;
      }
      all.push_back(std::make_unique<Actor>(next_id++, ptr));
      static const unsigned kRates[5] = {14, 60, 160, 400, 900};   // desde golpes muy frecuentes hasta muy espaciados: ejercita la recuperacion
      all.back()->rate = kRates[rng.below(5)];
      all.back()->f.health = 4000;                                  // aguanta cientos de golpes; la muerte se ejercita aparte
      live.push_back(all.back().get());
      h.spawn(live.back()->f);
    }
    if (!live.empty() && rng.below(900) == 0) {          // algunos actores mueren por un golpe letal: se fuerza vida baja
      Actor* v = live[rng.below(static_cast<unsigned>(live.size()))];
      if (v->f.health > 20) v->f.health = 12;
    }
    if (!live.empty() && rng.below(400) == 0) {
      const unsigned i = rng.below(static_cast<unsigned>(live.size()));
      h.despawn(live[i]->f);
      old_ptrs.push_back(live[i]->f.ptr);
      live.erase(live.begin() + i);
    }
    if (!live.empty() && rng.below(70) == 0) {
      Actor* a = live[rng.below(static_cast<unsigned>(live.size()))];
      a->f.f.atp = a->f.f.atp == 1 ? 2 : 1;
    }
    // --- tick matematico (oraculo): usa el estado de congelacion observado en el cuadro anterior ---
    for (Actor* a : live) {
      Orc& o = a->o;
      if (!o.bound || o.dead || o.frozen || o.latched || o.p == 0) continue;
      ++o.k;
      if (o.k > 60 && (o.k - 60) % 6 == 0) --o.p;
    }
    h.update();
    for (Actor* a : live) {
      Fake& f = a->f;
      Orc& o = a->o;
      const std::uint64_t pr0 = h.w.native_processed;
      bool striking = false;
      std::uintptr_t col = 0;
      if (f.health > 0 && rng.below(a->rate) == 0) {
        col = cols[rng.below(6)];
        const unsigned mode = rng.below(3);
        if (mode != 1) f.hit_with(1, col);
        if (mode != 0) f.hit_with(0, mode == 2 ? col : P(2));
        striking = true;
      }
      h.call(f);
      const bool processed = h.w.native_processed > pr0;
      const bool candidate = f.f.atp == 1;
      if (candidate && !o.bound) { o.bound = true; o.p = 0; o.k = 0; o.frozen = false; }
      else if (o.bound) o.frozen = !candidate;
      if (processed && striking && candidate && o.bound && !o.dead) {
        const bool lethal = f.health <= 0;
        const std::uintptr_t sel = f.sel;
        const int w = oracle_weight(h.w.types.at(sel));
        if (!lethal && w > 0 && !o.latched) {
          const int before = o.p;
          o.p = before + w > 100 ? 100 : before + w;
          o.k = 0;
          ++o_weighted;
          o_units += static_cast<std::uint64_t>(o.p - before);
          if (before < 100 && o.p >= 100) { o.latched = true; ++o_cross; }
        }
      }
      if (f.health <= 0 || f.mode == 9) { if (o.bound) o.dead = true; }
      clear_tg(f);
      // ---- comparacion ----
      ++g_checks;
      ++g_stress_compares;
      const post04::Entry* e = h.ent(f.id);
      if (o.bound) {
        const bool same = e && e->bound && e->posture == o.p && e->latched == o.latched && e->dead == o.dead && e->frozen == (o.frozen && !o.dead);
        if (!same) { ++mism; if (mism < 5) std::printf("MISMATCH seed=%llu frame=%u id=%u want p=%d latched=%d dead=%d frozen=%d got p=%d\n", static_cast<unsigned long long>(seed), fr, f.id, o.p, o.latched, o.dead, o.frozen, e ? e->posture : -1); }
      } else {
        if (e && e->bound) { ++mism; if (mism < 5) std::printf("MISMATCH unbound seed=%llu frame=%u id=%u\n", static_cast<unsigned long long>(seed), fr, f.id); }
      }
    }
    for (Actor* a : live) if (a->f.health <= 0 && rng.below(6) == 0) { h.despawn(a->f); old_ptrs.push_back(a->f.ptr); a->f.health = 0; a->called = false; }
    live.erase(std::remove_if(live.begin(), live.end(), [](Actor* a) { return !a->called; }), live.end());
    if (fr % 500 == 499) audit_ok(h);
  }
  CHECK(mism == 0);
  CHECK(h.pe.px.c.weighted == o_weighted && h.pe.px.c.crossings == o_cross && h.pe.px.c.units_added == o_units);
  CHECK(h.pe.px.c.ring_dropped == 0 && h.pe.px.c.generation_leak == 0 && h.pe.px.c.multi_consume == 0 && h.pe.px.c.underflow == 0 && h.pe.px.c.overflow == 0);
  CHECK(h.w.bad_derefs == 0);
  audit_ok(h);
  const post04::PCounters& c = h.pe.px.c;
  g_cov_weighted += c.weighted; g_cov_cross += c.crossings; g_cov_recov += c.units_recovered; g_cov_freeze += c.freeze_events; g_cov_binds += c.binds;
  g_cov_runs += c.regen_runs; g_cov_dead += c.death_closed;
  for (unsigned i = 0; i < post04::kSkipCount; ++i) g_cov_skips += c.skipped[i];
}
static void t27_stress() {
  for (std::uint64_t seed : {1ull, 2ull, 3ull, 7ull, 42ull, 1234567ull, 99991ull, 0xC0FFEEull}) stress(seed * 0x9E3779B97F4A7C15ull + 1, 12000);
}

// ---- T29: repeticion de los 36 eventos REALES (A1/B1, saneados) por el nucleo; decision esperada calculada por tests/gen_real_trace.py ----------------------
struct RealRow { int session; unsigned ord; unsigned id; unsigned gen; int life_death; int finish; int lethal; int hp_post; unsigned at; int family; int action; int after; };
#include "real_trace.inc"
static void t29_real_trace_replay() {
  CHECK(kRealRowCount == 36);
  for (int sess = 0; sess < 2; ++sess) {
    post04::PostureEngine pe; pe.eng.masks = M;
    Ledger& L = pe.eng.obs.ledger;
    std::map<std::pair<unsigned, unsigned>, std::pair<int, std::uint64_t>> inst;
    std::uintptr_t ptr = 0x100000;
    std::uint64_t call = 0;
    unsigned seen[21] = {};
    for (unsigned i = 0; i < kRealRowCount; ++i) {
      const RealRow& r = kRealRows[i];
      if (r.session != sess) continue;
      auto key = std::make_pair(r.id, r.gen);
      if (!inst.count(key)) {
        Events ev;
        const Observation o = L.observe(r.id, ptr, Admission::E_OC_CANDIDATE, 1, ++call, ev);
        ptr += 0x400;
        pe.px.sync(L);
        CHECK(o.tracked);
        inst[key] = {o.slot, o.generation};
      }
      HitReport rep;
      rep.event = r.ord; rep.call = r.ord; rep.id = r.id; rep.slot = inst[key].first; rep.generation = inst[key].second;
      rep.life = r.life_death ? Life::DeathSeen : Life::Active;
      rep.j.kind = r.finish ? HitKind::FinishNative : HitKind::Confirmed;
      rep.j.elig = r.finish ? Eligibility::No : Eligibility::Confirmed;
      rep.j.lethal = r.lethal != 0;
      rep.post.health = r.hp_post;
      rep.at_valid = !r.finish; rep.at_type = r.at; rep.fam.family = static_cast<Family>(r.family);
      const post04::PCounters before = pe.px.c;
      pe.px.consume(rep, L);
      int got = -1;
      if (pe.px.c.finish_events > before.finish_events) got = 20;
      else if (pe.px.c.weighted > before.weighted) got = 0;
      else for (unsigned k = 0; k < post04::kSkipCount; ++k) if (pe.px.c.skipped[k] > before.skipped[k]) got = static_cast<int>(k);
      CHECK(got == r.action);
      if (got >= 0 && got < 21) ++seen[got];
      CHECK(pe.px.entry(static_cast<unsigned>(rep.slot)).posture == r.after);
      if (r.lethal || r.finish) { Events ev; L.mark_death(rep.slot, rep.generation, 1, ev); }
      pe.px.sync(L);
    }
    const unsigned exp_apply = sess ? kExp_B1_APPLY : kExp_A1_APPLY, exp_leth = sess ? kExp_B1_LETHAL : kExp_A1_LETHAL, exp_zero = sess ? kExp_B1_ZERO : kExp_A1_ZERO;
    const unsigned exp_latch = sess ? kExp_B1_LATCHED : kExp_A1_LATCHED, exp_fin = sess ? kExp_B1_FINISH : kExp_A1_FINISH;
    CHECK(seen[0] == exp_apply && seen[3] == exp_leth && seen[6] == exp_zero && seen[8] == exp_latch && seen[20] == exp_fin);
    CHECK(pe.px.c.weighted == exp_apply && pe.px.c.finish_events == exp_fin && pe.px.c.confirmed_consumable == exp_apply + exp_leth + exp_zero + exp_latch);
    CHECK(pe.px.c.dup_consume_calls == 0 && pe.px.c.multi_consume == 0);
    const post04::AuditResult au = pe.px.audit();
    CHECK(au.ok);
    if (sess == 1) CHECK(pe.px.c.crossings == 3 && pe.px.c.saturations == 1 && au.latched_now == 3);       // 161/8, 162/9 y 868/18 llegan al umbral
    else CHECK(pe.px.c.crossings == 0 && pe.px.c.weighted == 0);                                           // en A1 los unicos no letales son OTHER_VERIFIED: peso 0
  }
}

// ---- T30: rutas de defensa en profundidad (informes sinteticos que el flujo real casi no produce) y orden PRE/POST -------------------------------------------------
static HitReport synth_report(Hx& h, std::uint32_t id, std::uint64_t event, Family f, bool lethal = false, int hp_post = 100) {
  HitReport r;
  const int slot = h.slot_of(id);
  r.event = event; r.call = event; r.id = id; r.slot = slot;
  r.generation = slot >= 0 ? h.pe.eng.obs.ledger.slot(static_cast<unsigned>(slot)).generation : 0;
  r.life = Life::Active; r.j.kind = HitKind::Confirmed; r.j.elig = Eligibility::Confirmed; r.j.lethal = lethal; r.post.health = hp_post;
  r.at_valid = true; r.at_type = SWORD; r.fam.family = f;
  return r;
}
static void t30_defense_in_depth() {
  // (a) entrada ya cerrada por muerte vista + informe NO letal e inconsistente: se omite como ACTOR_DEAD y no suma
  {
    Hx h; types(h);
    Fake a(10, 0x9000); h.spawn(a); strike(h, a, P(1));
    Events ev; const int slot = h.slot_of(10);
    h.pe.eng.obs.ledger.mark_death(slot, h.pe.eng.obs.ledger.slot(static_cast<unsigned>(slot)).generation, 1, ev);
    h.pe.px.sync(h.pe.eng.obs.ledger);
    CHECK(h.ent(10)->dead);
    const HitReport r = synth_report(h, 10, 9001, Family::Sword);
    h.pe.px.consume(r, h.pe.eng.obs.ledger);
    CHECK(h.skipped(post04::Skip::ActorDead) == 1 && h.pe.px.c.weighted == 1 && h.posture(10) == 25);
    audit_ok(h);
  }
  // (b) generacion obsoleta (el slot fue retirado): STALE_GENERATION y no NOT_ADMITTED
  {
    Hx h; types(h);
    Fake a(10, 0x9000); h.spawn(a); strike(h, a, P(1));
    const HitReport old = synth_report(h, 10, 9001, Family::Sword);
    h.despawn(a);
    h.ticks(100);
    CHECK(h.ent(10) == nullptr);
    h.pe.px.consume(old, h.pe.eng.obs.ledger);
    CHECK(h.skipped(post04::Skip::StaleGeneration) == 1 && h.skipped(post04::Skip::NotAdmitted) == 0 && h.pe.px.c.weighted == 1);
    HitReport bad = old; bad.event = 9002; bad.slot = -1;
    h.pe.px.consume(bad, h.pe.eng.obs.ledger);
    bad.event = 9003; bad.slot = 999;
    h.pe.px.consume(bad, h.pe.eng.obs.ledger);
    CHECK(h.skipped(post04::Skip::StaleGeneration) == 3);
    // id distinto con la misma generacion/slot
    Fake b(20, 0x9400); h.spawn(b); h.call(b);
    HitReport mism = synth_report(h, 20, 9004, Family::Sword); mism.id = 21;
    h.pe.px.consume(mism, h.pe.eng.obs.ledger);
    CHECK(h.skipped(post04::Skip::StaleGeneration) == 4 && h.posture(20) == 0);
    audit_ok(h);
  }
  // (c) entrada congelada (no verificada) que recibe un informe: FROZEN_UNVERIFIED, sin acumulacion
  {
    Hx h; types(h);
    Fake a(10, 0x9000); h.spawn(a); strike(h, a, P(1));
    Pick pk; pk.slot = h.slot_of(10); pk.id = 10; pk.ptr = 0x9000; pk.generation = h.ent(10)->generation;
    Events ev;
    h.pe.eng.obs.ledger.apply_lookup(pk, Lookup::Creating, 1, ev);
    h.pe.px.sync(h.pe.eng.obs.ledger);
    CHECK(h.ent(10)->frozen);
    const HitReport r = synth_report(h, 10, 9001, Family::Sword);
    h.pe.px.consume(r, h.pe.eng.obs.ledger);
    CHECK(h.skipped(post04::Skip::Frozen) == 1 && h.posture(10) == 25 && h.pe.px.c.weighted == 1);
    audit_ok(h);
  }
  // (d) el mod se deshabilita ENTRE el PRE y el POST de un golpe: ese POST no acumula
  {
    Hx h; types(h);
    Fake a(10, 0x9000); h.spawn(a);
    a.hit_with(1, P(1));
    execute_start(a);
    Events e1; h.pe.on_pre(a.ptr, a.id, pre_of(a), e1);
    native(a, h.w);
    h.pe.eng.obs.disable("TEST");
    Events e2; HitReport rep;
    const bool has = h.pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s2, std::uint32_t& t) { return h.reader(s2, t); }, e2, rep);
    CHECK(!has && h.pe.px.c.weighted == 0 && h.pe.px.c.consume_calls == 0 && h.w.derefs == 0);
  }
  // (e) un golpe en un tick de recuperacion elegible: el orden exacto hook/update ya esta fijado en T12; aqui se verifica que k no cuenta el tick del golpe
  {
    Hx h; types(h);
    Fake a(10, 0x9000); h.spawn(a); strike(h, a, P(1));
    h.ticks(70);
    CHECK(h.posture(10) == 24);
    strike(h, a, P(1));
    CHECK(h.ent(10)->k == 0 && h.ent(10)->last_hit_tick == h.pe.px.now());
  }
}

static void t00_budget_contract() {
  CHECK(post03::kBudgetTotal == 1500 && post03::kBudgetTotal + post04::Posture::kLogBudget == 2000);   // el techo global por sesion sigue siendo 2000 lineas
  CHECK(pp::kMaxPosture == 100 && pp::kThreshold == 100 && pp::kWeightSword == 25 && pp::kWeightIronBall == 50 && pp::kRecoverDelayTicks == 60 && pp::kRecoverPeriodTicks == 6);
  const Family all[] = {Family::Unavailable, Family::Sword, Family::ShieldAttack, Family::Arrow, Family::Bomb, Family::IronBall, Family::Boomerang, Family::At40, Family::Slingshot, Family::Other, Family::Ambiguous};
  for (Family f : all) CHECK(pp::weight_of(f) == (f == Family::Sword ? 25 : (f == Family::IronBall ? 50 : 0)));   // tabla de pesos EXACTA: solo SWORD e IRON_BALL
}

// Escribe lineas REALES del formateador (las mismas que mod.cpp registra) para que los parsers de tools se prueben contra la salida verdadera.
static int emit_sample(const char* path) {
  Hx h; types(h);
  std::FILE* f = std::fopen(path, "w");
  if (!f) return 2;
  const char* pre = "[INFO | dev.posture.post04_numeric_observer] ";
  std::fprintf(f, "%sPOST04_INIT id=dev.posture.post04_numeric_observer version=0.4.0 source_commit=0123456789abcdef0123456789abcdef01234567 policy_id=%s passive=1 writes_to_game=0 "
               "game_actions=NONE hook_target=daE_OC_c::damage_check hook_pre=HOOK_CONTINUE_ALWAYS hook_post=passive capacity=64 key=slot+generation+id max_posture=100 threshold=100 "
               "weights(sword/iron/all_other)=25/50/0 recover_delay_ticks=60 recover_period_ticks=6 timebase=MOD_UPDATE_SIM_TICK timebase_nominal_hz=30 "
               "timebase_basis=source_game_clock_time_scale_1 TIMEBASE_PAUSE_UNVERIFIED balance=PROVISIONAL log_budget_posture=500\n", pre, pp::kPolicyId);
  Fake a(10, 0x9000), b(11, 0x9400), c(12, 0x9800); h.spawn(a); h.spawn(b); h.spawn(c);
  strike(h, a, P(1)); h.ticks(70); strike(h, a, P(1)); strike(h, a, P(3)); strike(h, a, P(1)); strike(h, a, P(1));       // 24+25=... hasta el umbral y un golpe en latch
  strike(h, b, P(4)); strike(h, b, P(6)); strike(h, b, P(1)); h.ticks(300);                                              // omisiones de peso cero y recuperacion hasta 0
  c.health = 5; strike(h, c, P(1));                                                                                       // golpe letal
  h.despawn(a); h.ticks(100);                                                                                             // retiro por ausencia verificada
  h.pe.on_shutdown(); h.drain();
  char line[1024];
  for (auto& it : h.items) { if (post04::fmt_item(line, sizeof(line), it) > 0) std::fprintf(f, "%s%s\n", pre, line); }
  const post04::AuditResult au = h.pe.px.audit();
  const Counters& k = h.pe.eng.obs.ledger.c;
  std::fprintf(f, "%sPOST03_HIT_SUMMARY v=0.4.0 tag=SHUTDOWN confirmed=%llu finish=%llu events_emitted=%llu\n", pre, static_cast<unsigned long long>(k.confirmed),
               static_cast<unsigned long long>(k.finish), static_cast<unsigned long long>(k.events_emitted));
  post04::fmt_summary(line, sizeof(line) * 1, h.pe.px, au, "SHUTDOWN");
  char big[4096];
  post04::fmt_summary(big, sizeof(big), h.pe.px, au, "SHUTDOWN");
  std::fprintf(f, "%s%s\n", pre, big);
  post04::fmt_conservation(big, sizeof(big), h.pe.px, au, "SHUTDOWN");
  std::fprintf(f, "%s%s\n", pre, big);
  std::fclose(f);
  return au.ok ? 0 : 3;
}

int main(int argc, char** argv) {
  if (argc >= 3 && std::strcmp(argv[1], "--emit-sample") == 0) return emit_sample(argv[2]);
  t00_budget_contract(); t29_real_trace_replay(); t30_defense_in_depth(); t01_bind_init(); t02_sword_x4(); t03_iron(); t04_mixed(); t05_dual_tg(); t06_duplicates(); t07_hp_delta_zero(); t08_lethal_finish();
  t09_other_unknown(); t10_hm_independence(); t11_regen_edges(); t12_hit_resets_delay(); t13_latched_frozen(); t14_two_actors(); t15_no_carry_over();
  t16_unsafe_freeze(); t17_excluded(); t18_lifecycle_states(); t19_capacity(); t20_clock_wrap_and_sweep(); t21_declared_timebase(); t22_disabled();
  t23_logs_and_budget(); t24_ambiguous(); t25_collider_regression(); t27_stress();
  std::printf("stress coverage: oracle_compares=%llu weighted=%llu crossings=%llu units_recovered=%llu regen_runs=%llu freezes=%llu binds=%llu skips=%llu deaths=%llu\n",
              g_stress_compares, g_cov_weighted, g_cov_cross, g_cov_recov, g_cov_runs, g_cov_freeze, g_cov_binds, g_cov_skips, g_cov_dead);
  CHECK(g_cov_weighted > 200 && g_cov_cross > 5 && g_cov_recov > 200 && g_cov_freeze > 20 && g_cov_binds > 50 && g_cov_skips > 200 && g_cov_dead > 5);   // el estres NO es vacuo
  std::printf("POST04_POSTURE_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail == 0 ? "LAB_PASS" : "FAIL");
  return g_fail == 0 ? 0 : 1;
}
