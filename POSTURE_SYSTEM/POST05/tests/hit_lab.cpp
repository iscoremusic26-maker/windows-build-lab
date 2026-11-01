// POST03 laboratorio ADVERSARIAL de impactos: motor puro + MODELO de damage_check derivado de la fuente (d_a_e_oc.cpp:638-785).
// LAB_PASS = consistencia del motor con ese modelo; NO es runtime. Las mascaras AT_TYPE son SINTETICAS.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "../src/engine.hpp"

using namespace post03;

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
enum : std::uint32_t { SWORD = 0x4000000u, SHIELD = 0x10u, ARROW = 0x2000u, BOMB = 0x20u, IRON = 0x400000u, BOOM = 0x10000u, AT40 = 0x40u, SLING = 0x80u, UNK = 0x80000000u };

struct World {
  std::map<std::uintptr_t, std::uint32_t> types;  // collider (opaco) -> AT type
  int derefs = 0, bad_derefs = 0;
  std::vector<std::uintptr_t> deref_ptrs;
  std::uint64_t native_processed = 0, native_finish = 0;
  int power = 8;
};
struct Fake {
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  ActorFacts f;
  int health = 40, mode = 3, old_mode = 0, state = 0, cooldown = 0;
  bool cutdown = false;
  bool tg[2] = {false, false}, actor_ok[2] = {true, true};
  std::uintptr_t hit[2] = {0, 0}, sel = 0;
  Fake(std::uint32_t i, std::uintptr_t p) : id(i), ptr(p) {
    f.profile_ok = f.live = f.status_10000 = true;
    f.name = NameKind::E_OC;
    f.atp = 1;
  }
  void hit_with(int sphere, std::uintptr_t collider) { tg[sphere] = true; hit[sphere] = collider; }
};

static void set_mode(Fake& a, int m, int st) { a.old_mode = a.mode; a.mode = m; a.state = st; }
static void clear_tg(Fake& a) { a.tg[0] = a.tg[1] = false; a.hit[0] = a.hit[1] = 0; }

// Modelo de daE_OC_c::damage_check (solo lo que determina las senales observables).
static void native(Fake& a, World& w) {
  if (a.cutdown) { a.cutdown = false; a.health = 0; set_mode(a, 9, 1); clear_tg(a); ++w.native_finish; return; }
  if (a.cooldown) return;
  bool had = false;
  for (int i = 0; i < 2; ++i)
    if (a.tg[i] && a.actor_ok[i]) { a.sel = a.hit[i]; had = true; }  // la segunda esfera sobrescribe (d_a_e_oc.cpp:656-666)
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
  else if (t & M.at40) a.health += 10;
  else if (t & M.slingshot) {
    a.health -= 5;
    if (a.health < 0) { a.health = 0; set_mode(a, 6, 0); clear_tg(a); return; }  // retorno anticipado SIN cc_at_check
  }
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

struct Harness {
  Engine e;
  World w;
  std::vector<HitReport> events;
  std::vector<Event> life;
  Harness() { e.masks = M; }
  bool reader(std::uintptr_t sel, std::uint32_t& type) {
    ++w.derefs;
    w.deref_ptrs.push_back(sel);
    auto it = w.types.find(sel);
    if (it == w.types.end() || !deref_alignment_ok(sel)) { ++w.bad_derefs; return false; }
    type = it->second;
    return true;
  }
  void keep(Events& ev) { for (unsigned i = 0; i < ev.n; ++i) life.push_back(ev.e[i]); }
  void pre(Fake& a) { Events ev; e.on_pre(a.ptr, a.id, pre_of(a), ev); keep(ev); }
  bool post(Fake& a) {
    Events ev; HitReport rep;
    const bool has = e.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return reader(s, t); }, ev, rep);
    keep(ev);
    if (has) events.push_back(rep);
    return has;
  }
  // una llamada completa a damage_check (con el decremento previo de execute)
  bool call(Fake& a) { execute_start(a); pre(a); native(a, w); return post(a); }
  unsigned kinds(HitKind k) const { unsigned n = 0; for (auto& r : events) n += r.j.kind == k; return n; }
};

static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }

// ---------------------------------------------------------------------------------------------------------------------
static void t01_t02() {
  Harness h; h.w.types[P(1)] = SWORD; h.w.types[P(2)] = SWORD;
  Fake a(10, 0x9000);
  CHECK(!h.call(a) && h.e.obs.ledger.c.no_tg == 1 && h.events.empty());                        // T01 sin TG
  a.hit_with(0, P(1));                                                                           // T02 esfera 0
  CHECK(h.call(a) && h.events.size() == 1 && h.events[0].pre.tg_bits == 1 && h.events[0].col.src == 0 && h.events[0].j.kind == HitKind::Confirmed);
  Fake b(11, 0x9400); b.hit_with(1, P(2));                                                       // esfera 1
  CHECK(h.call(b) && h.events.size() == 2 && h.events[1].pre.tg_bits == 2 && h.events[1].col.src == 1);
  CHECK(h.e.conserved() && h.w.bad_derefs == 0);
}
static void t03() {
  Harness h; h.w.types[P(1)] = SWORD; h.w.types[P(2)] = ARROW;
  Fake a(10, 0x9000); a.hit_with(0, P(1)); a.hit_with(1, P(1));       // misma espada en las dos esferas
  CHECK(h.call(a) && h.events.size() == 1);                           // UN evento
  CHECK(h.events[0].pre.tg_bits == 3 && h.events[0].j.dual_tg && !h.events[0].col.pair_distinct && h.events[0].fam.family == Family::Sword);
  CHECK(h.w.derefs == 1 && h.e.obs.ledger.c.dual_tg == 1 && h.e.obs.ledger.c.confirmed == 1 && h.e.obs.ledger.c.pair_distinct == 0);
  Harness g; g.w.types[P(1)] = SWORD; g.w.types[P(2)] = ARROW;   // colliders distintos (T15)
  Fake b(10, 0x9000); b.hit_with(0, P(1)); b.hit_with(1, P(2));
  CHECK(g.call(b) && g.events.size() == 1 && g.events[0].col.pair_distinct && g.events[0].col.src == 1);
  CHECK(g.events[0].fam.family == Family::Arrow && g.w.derefs == 1 && g.w.deref_ptrs[0] == P(2));  // solo el que eligio la fuente (cc1), nunca el otro
  CHECK(g.e.obs.ledger.c.pair_distinct == 1);
}
static void t04() {
  Harness h; h.w.types[P(1)] = SWORD; h.w.types[P(2)] = SWORD;
  Fake a(10, 0x9000); a.hit_with(0, P(1));
  CHECK(h.call(a) && a.cooldown == 10);
  a.hit_with(0, P(2));                       // nuevo bit TG durante el bloqueo
  for (int i = 0; i < 9; ++i) { CHECK(!h.call(a)); a.hit_with(0, P(2)); }
  CHECK(h.events.size() == 1 && h.e.obs.ledger.c.cooldown_blocked == 9 && h.e.obs.ledger.c.cooldown_with_tg == 9);
  CHECK(h.w.native_processed == 1 && h.e.conserved());
  a.hit_with(0, P(2));
  CHECK(h.call(a));                          // al decrementar, el cooldown llega a 0 y el golpe se procesa
  CHECK(h.events.size() == 2 && h.w.native_processed == 2);
  // sel obsoleto sin golpe nuevo: nunca se clasifica
  Fake b(11, 0x9400); b.sel = P(1);
  CHECK(!h.call(b) && h.events.size() == 2 && h.w.derefs == 2);
}
static void t05() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000); a.sel = P(1); a.cutdown = true;
  CHECK(h.call(a) && h.events.size() == 1 && h.events[0].j.kind == HitKind::FinishNative);
  CHECK(h.w.derefs == 0 && h.e.obs.ledger.c.confirmed == 0 && h.e.obs.ledger.c.finish == 1 && h.events[0].fam.family == Family::Unavailable);
  CHECK(h.events[0].j.elig == Eligibility::No && h.e.conserved());
  Fake b(11, 0x9400); b.cutdown = true;           // estado final inesperado (otro mod lo altero)
  h.pre(b); native(b, h.w); b.mode = 3; b.health = 5;
  CHECK(h.post(b) && h.events.back().j.kind == HitKind::FinishUnexpected && h.e.obs.ledger.c.finish_unexpected == 1);
}
static void t06() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000); a.hit_with(0, P(1));
  h.pre(a); native(a, h.w);
  CHECK(h.post(a) && h.events.size() == 1);
  CHECK(!h.post(a) && h.events.size() == 1 && h.e.obs.ledger.c.dup_post_orphan == 1);        // POST duplicado
  // replay del MISMO frame (mismo token/serial) por el motor: no puede emitir un segundo evento
  Fake b(11, 0x9400); b.hit_with(0, P(1));
  CallFrame tok;
  Events evx; h.e.on_pre(b.ptr, b.id, pre_of(b), evx);
  native(b, h.w);
  const unsigned before = static_cast<unsigned>(h.events.size());
  PostReads pr = post_of(b);
  CHECK(h.e.obs.frames.end(b.ptr, tok, h.e.obs.ledger.c));              // el frame sale de la pila una vez
  HitReport rp; Events ev3;
  auto rd = [&](std::uintptr_t s, std::uint32_t& t) { return h.reader(s, t); };
  CHECK(h.e.finalize(tok, pr, rd, ev3, rp) && rp.j.kind == HitKind::Confirmed);
  CHECK(!h.e.finalize(tok, pr, rd, ev3, rp) && !h.e.finalize(tok, pr, rd, ev3, rp));   // replay: suprimido
  CHECK(h.e.obs.ledger.c.dup_serial_gate == 2 && h.e.obs.ledger.c.events_emitted == before + 1);
  // la puerta tambien se prueba directamente en el ledger
  Events ev_tmp;
  const int slot = h.e.obs.ledger.observe(77, 0xE000, Admission::E_OC_CANDIDATE, 1, 999, ev_tmp).slot;
  const std::uint64_t gen = h.e.obs.ledger.slot(slot).generation;
  CHECK(h.e.obs.ledger.try_emit(slot, gen, 500) && !h.e.obs.ledger.try_emit(slot, gen, 500) && !h.e.obs.ledger.try_emit(slot, gen, 499));
  CHECK(h.e.obs.ledger.try_emit(slot, gen, 501));                        // un golpe NUEVO (serial mayor) no se suprime
  // POST perdido: el siguiente PRE del mismo actor descarta el frame anterior
  Harness g; g.w.types[P(1)] = SWORD;
  Fake c(12, 0x9800); c.hit_with(0, P(1));
  g.pre(c); native(c, g.w);                      // POST perdido
  c.cooldown = 0; c.hit_with(0, P(1));
  g.call(c);
  CHECK(g.e.obs.ledger.c.stale_frames == 1 && g.e.obs.ledger.c.cand_discarded == 1 && g.events.size() == 1 && g.e.conserved());
}
static void t07() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000), b(11, 0x9400); a.hit_with(0, P(1)); b.hit_with(0, P(1));
  h.pre(a); h.pre(b); native(b, h.w); native(a, h.w);
  CHECK(h.post(b) && h.post(a) && h.events.size() == 2 && h.events[0].id == 11 && h.events[1].id == 10);   // anidado A{B}
  CHECK(h.e.obs.frames.depth() == 0 && h.e.conserved());
  Harness g; g.w.types[P(1)] = SWORD;
  Fake c(10, 0x9000), d(11, 0x9400); c.hit_with(0, P(1)); d.hit_with(0, P(1));
  g.pre(c); g.pre(d); native(c, g.w);
  CHECK(g.post(c) && g.events.size() == 1 && g.e.obs.ledger.c.cand_discarded == 1);               // POST de D perdido: D descartado, no mezclado
  CHECK(!g.post(d) && g.events.size() == 1 && g.e.conserved());
  Harness k; k.w.types[P(1)] = SWORD;                                                               // saturacion de la pila
  std::vector<Fake> v; for (unsigned i = 0; i < 5; ++i) { v.emplace_back(100 + i, 0xA000 + i * 0x400); v.back().hit_with(0, P(1)); }
  for (auto& f : v) k.pre(f);
  CHECK(k.e.obs.ledger.c.frame_overflow == 1 && k.e.obs.frames.depth() == kFrameCapacity);
  for (auto& f : v) native(f, k.w);
  for (unsigned i = kFrameCapacity; i-- > 0;) CHECK(k.post(v[i]));
  CHECK(!k.post(v[4]) && k.events.size() == kFrameCapacity && k.e.conserved());
}
static void t08() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000); a.hit_with(0, P(1));
  h.pre(a); native(a, h.w);
  Fake imp(10, 0xB000);                       // mismo ID en OTRA direccion mientras el frame de A esta abierto
  Events ev; h.e.on_pre(imp.ptr, imp.id, pre_of(imp), ev);
  CHECK(h.e.obs.ledger.c.identity_conflicts == 1);
  CHECK(!h.post(a) && h.events.empty() && h.e.obs.ledger.c.post_generation_mismatch == 1 && h.e.obs.ledger.c.cross_generation_drops == 1);
  // direccion reciclada por otro ID
  Harness g; g.w.types[P(1)] = SWORD;
  Fake b(20, 0x9000); b.hit_with(0, P(1)); g.pre(b); native(b, g.w);
  Fake c(21, 0x9000); Events ev2; g.e.on_pre(c.ptr, c.id, pre_of(c), ev2);
  // fisicamente imposible a la vez (b seguiria vivo), pero el motor debe ser seguro: el frame de b es OBSOLETO, el de c es suyo y no se mezclan
  CHECK(!g.post(b) && g.events.empty() && g.e.obs.ledger.c.address_reused == 1 && g.e.obs.ledger.c.stale_frames == 1 && g.e.obs.ledger.c.cand_discarded == 1);
  CHECK(g.e.obs.ledger.slot(0).id != 21 || g.e.obs.ledger.slot(0).life == Life::Retired);
  CHECK(g.e.conserved());
}
static void t09_t10() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000), b(11, 0x9400);
  a.hit_with(0, P(1)); b.hit_with(0, P(1));                // la MISMA espada golpea a dos Bokoblins en el mismo tick
  CHECK(h.call(a) && h.call(b) && h.events.size() == 2 && h.events[0].id != h.events[1].id && h.w.native_processed == 2);   // T09
  for (int i = 0; i < 12; ++i) { execute_start(a); }       // vence el cooldown nativo
  a.hit_with(0, P(1)); b.hit_with(0, P(1));
  CHECK(h.call(a) && h.events.size() == 3 && h.events[2].id == 10 && h.events[2].generation == h.events[0].generation);   // T10 segundo golpe legitimo
  CHECK(h.e.obs.ledger.slot(0).hits >= 2 && h.e.conserved());
}
static void t11_t12() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000);
  for (int i = 0; i < 50; ++i) { a.health = 80 - i; a.mode = 3 + (i % 3); h.call(a); }          // HP/modo cambian solos (regen HM/DEVF, IA)
  CHECK(h.events.empty() && h.e.obs.ledger.c.confirmed == 0 && h.e.obs.ledger.c.processed_mismatch == 0 && h.w.native_processed == 0);
  h.w.power = 0;                                                                                   // T12 golpe con HP igual
  Fake b(11, 0x9400); b.hit_with(0, P(1));
  CHECK(h.call(b) && h.events.back().j.kind == HitKind::Confirmed && !h.events.back().j.hp_changed && b.health == 40);
  CHECK(h.events.back().pre.health == h.events.back().post.health);
  // otro mod cambia el modo despues del original: el cooldown se puso pero la firma no coincide
  Harness g; g.w.types[P(1)] = SWORD;
  Fake c(12, 0x9800); c.hit_with(0, P(1)); g.pre(c); native(c, g.w); c.mode = 4;
  CHECK(g.post(c) && g.events[0].j.kind == HitKind::ProcessedMismatch && g.events[0].j.elig == Eligibility::Undetermined && g.w.derefs == 0);
  // modo correcto pero mOldActionMode no es el modo previo (segundo setActionMode ajeno): tampoco se confirma
  Fake d(13, 0x9C00); d.hit_with(0, P(1)); g.pre(d); native(d, g.w); d.old_mode = 99;
  CHECK(g.post(d) && g.events[1].j.kind == HitKind::ProcessedMismatch && g.w.derefs == 0 && g.e.obs.ledger.c.processed_mismatch == 2);
}
static void t13() {
  struct Case { std::uint32_t type; Family fam; int hp; };
  const Case cases[] = {{SWORD, Family::Sword, 40}, {SHIELD, Family::ShieldAttack, 40}, {ARROW, Family::Arrow, 40}, {BOMB, Family::Bomb, 40},
                        {IRON, Family::IronBall, 40}, {BOOM, Family::Boomerang, 40}, {AT40, Family::At40, 40}, {SLING, Family::Slingshot, 40},
                        {SLING, Family::Slingshot, 3},      // tirachinas letal: retorno anticipado sin cc_at_check
                        {BOMB, Family::Bomb, 1}, {0x1u, Family::Other, 40}, {SWORD | UNK, Family::Sword, 40}};
  unsigned idx = 0;
  for (auto& c : cases) {
    Harness h; h.w.types[P(1)] = c.type;
    Fake a(10 + idx, 0x9000 + idx * 0x400); a.health = c.hp; a.hit_with(0, P(1)); ++idx;
    CHECK(h.call(a) && h.events.size() == 1 && h.events[0].j.kind == HitKind::Confirmed);
    CHECK(h.events[0].fam.family == c.fam);
    CHECK(h.events[0].at_valid && h.events[0].at_type == c.type);
  }
  // multiples familias a la vez: AMBIGUOUS, con la primera rama nativa visible
  Harness m; m.w.types[P(1)] = SWORD | SHIELD;
  Fake a(30, 0x9000); a.hit_with(0, P(1));
  CHECK(m.call(a) && m.events[0].fam.family == Family::Ambiguous && m.events[0].fam.native_first == Family::Sword && m.events[0].fam.groups == 2);
  // tipo 0: UNAVAILABLE con razon exacta
  Harness z; z.w.types[P(1)] = 0;
  Fake b(31, 0x9400); b.hit_with(0, P(1));
  CHECK(z.call(b) && z.events[0].fam.family == Family::Unavailable && std::strcmp(z.events[0].fam.reason, "TYPE_ZERO") == 0 && z.e.obs.ledger.c.classification_unavailable == 1);
  CHECK(classify_family(0x2, M).family == Family::Sword && classify_family(0x4000000, M).family == Family::Sword && classify_family(SLING | AT40, M).family == Family::Ambiguous);
}
static void t14() {
  Harness h; h.w.types[P(1)] = SWORD; h.w.types[P(9)] = BOMB;
  // mpCollider NO coincide con ninguna esfera (otro mod/ruta no modelada): confirmado sin familia y SIN desreferenciar
  Fake a(10, 0x9000); a.hit_with(0, P(1));
  h.pre(a); native(a, h.w); a.sel = P(9);
  CHECK(h.post(a) && h.events[0].j.kind == HitKind::Confirmed && h.events[0].fam.family == Family::Unavailable && h.w.derefs == 0);
  CHECK(h.e.obs.ledger.c.collider_unmatched == 1 && h.events[0].col.reason == ColReason::NotEqualToRaw);
  // puntero implausible
  Fake b(11, 0x9400); b.hit_with(0, 0x30);                         // alineado pero < 0x10000
  h.w.types[0x30] = SWORD;
  h.pre(b); native(b, h.w);
  CHECK(h.post(b) && h.w.derefs == 0 && h.events[1].fam.family == Family::Unavailable);
  // mpCollider nulo
  Fake c(12, 0x9800); c.hit_with(0, P(1));
  h.pre(c); native(c, h.w); c.sel = 0;
  CHECK(h.post(c) && h.w.derefs == 0 && h.events[2].fam.family == Family::Unavailable);
  CHECK(resolve_collider(0, P(1), 0, 1).src == -1 && resolve_collider(P(1), P(1), 0, 1).src == 0 && resolve_collider(P(1), 0, P(1), 2).src == 1);
  CHECK(resolve_collider(P(1), P(1), P(1), 3).src == 1 && resolve_collider(P(2), P(1), 0, 1).src == -1 && !deref_alignment_ok(0x10001) && deref_alignment_ok(0x10000) && deref_alignment_ok(0x10004) && !deref_alignment_ok(0x30));
  // sel obsoleto de un golpe anterior + nueva candidata NO procesada: no hay clasificacion
  Harness g; g.w.types[P(1)] = SWORD;
  Fake d(13, 0x9C00); d.sel = P(1); d.hit_with(0, P(1)); d.actor_ok[0] = false;   // TG crudo sin actor del impacto: el juego no lo procesa
  CHECK(g.call(d) && g.events[0].j.kind == HitKind::CandidateNotProcessed && g.w.derefs == 0 && g.e.obs.ledger.c.not_processed == 1);
  CHECK(g.events[0].fam.family == Family::Unavailable && g.events[0].j.elig == Eligibility::No);
}
static void t16() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000); a.health = 80;                   // HM x2
  a.hit_with(0, P(1));
  CHECK(h.call(a) && h.events.size() == 1);
  const std::uint64_t gen = h.events[0].generation;
  a.f.atp = 2;                                         // Blue de grupo: AtAtp 1 -> 2
  for (int i = 0; i < 12; ++i) execute_start(a);
  a.hit_with(0, P(1));
  CHECK(!h.call(a) && h.events.size() == 1 && h.e.obs.ledger.c.unknown_unsafe_calls >= 1);   // admision UNKNOWN_UNSAFE: sin detalle ni evento
  a.f.atp = 1;
  for (int i = 0; i < 12; ++i) execute_start(a);
  a.hit_with(0, P(1));
  CHECK(h.call(a) && h.events.size() == 2 && h.events[1].generation == gen && h.e.obs.ledger.c.records_created == 1 && h.e.obs.ledger.c.admission_changes == 2);
  Fake r(11, 0x9400); r.f.name = NameKind::E_OC2; r.f.atp = 2; r.hit_with(0, P(1));     // Red/Black: excluido
  for (int i = 0; i < 3; ++i) { h.call(r); r.cooldown = 0; r.hit_with(0, P(1)); }
  CHECK(h.events.size() == 2 && h.e.obs.ledger.c.excluded_calls >= 3 && h.w.native_processed >= 3);
  // la identidad no depende del HP
  Fake d(12, 0x9800); d.health = 400; d.hit_with(0, P(1));
  CHECK(h.call(d) && h.events.back().id == 12);
}
static void t17_t18() {
  Harness h; h.w.types[P(1)] = SWORD;
  std::uint32_t id = 1000;
  for (unsigned enc = 0; enc < 32; ++enc) {
    std::vector<Fake> grp;
    for (unsigned k = 0; k < 6; ++k) { const std::uint32_t nid = id++; grp.emplace_back(nid, 0xC000 + ((nid + enc) % 7) * 0x400); grp.back().hit_with(0, P(1)); }
    for (auto& f : grp) h.call(f);
    // los actores desaparecen: el barrido los retira
    Events ev; std::uint64_t st = enc + 1; Pick p;
    while (h.e.obs.ledger.next_sweep(p, st)) h.e.obs.ledger.apply_lookup(p, Lookup::Absent, h.e.obs.epoch.epoch, ev);
  }
  CHECK(h.e.obs.ledger.c.records_created > kCapacity || h.e.obs.ledger.c.records_created == 32 * 6);
  CHECK(h.e.conserved() && h.e.obs.ledger.c.capacity_drops == 0 && h.e.obs.ledger.active() == 0);
  std::set<std::uint64_t> gens; unsigned total = 0;
  for (auto& e : h.life) if (e.kind == EvKind::FirstSeen) { gens.insert(e.generation); ++total; }
  CHECK(gens.size() == total);
  // T18: ledger lleno de vivos -> el candidato nuevo se omite (sin evento, sin sobrescribir), el nativo sigue
  Harness g; g.w.types[P(1)] = SWORD;
  std::vector<Fake> v; for (unsigned i = 0; i < kCapacity; ++i) { v.emplace_back(2000 + i, 0xD0000 + i * 0x400); g.call(v.back()); }
  Fake x(5000, 0xF0000); x.hit_with(0, P(1));
  CHECK(!g.call(x) && g.w.native_processed == 1 && g.e.obs.ledger.c.capacity_drops == 1 && g.events.empty());
  CHECK(g.e.obs.ledger.active() == kCapacity && g.e.conserved());
  // presupuesto de log: tope duro
  Observer o; unsigned n = 0;
  for (unsigned i = 0; i < 5000; ++i) n += o.budget.take(LogCat::Hit, o.ledger.c) ? 1 : 0;
  CHECK(n == kBudget[1] && o.ledger.c.log_dropped[1] == 5000 - kBudget[1]);
  for (unsigned i = 0; i < 100; ++i) o.may_log_hit(HitKind::CandidateNotProcessed);
  CHECK(o.ledger.c.kind_throttled == 70 && o.may_log_hit(HitKind::Confirmed) && o.may_log_hit(HitKind::FinishNative));
}
static void t19() {
  int n = 0;
  auto ok = [&] { ++n; return 0; }; auto bad = [&] { ++n; return 2; };
  CHECK(install_sequence(ok, ok, ok).state == InstallState::Ok);
  CHECK(install_sequence(bad, ok, ok).state == InstallState::PostFailed);
  CHECK(install_sequence(ok, bad, ok).state == InstallState::PreFailedRolledBack);
  CHECK(install_sequence(ok, bad, bad).state == InstallState::PreFailedRollbackFailed);
}
// T21: >10000 llamadas, orden variable, POST perdidos/duplicados, actores que mueren y reaparecen; invariantes y verdad terreno
static void t21() {
  std::uint64_t seed = 0x243F6A8885A308D3ull;
  auto rnd = [&](unsigned m) { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return static_cast<unsigned>(seed % m); };
  for (int perturb = 0; perturb < 2; ++perturb) {
    Harness h; h.e.masks = M;
    for (unsigned i = 0; i < 6; ++i) h.w.types[P(i + 1)] = (i == 0 ? SWORD : i == 1 ? ARROW : i == 2 ? BOMB : i == 3 ? SLING : i == 4 ? IRON : BOOM);
    std::vector<Fake> pop; std::uint32_t nid = 1;
    for (unsigned i = 0; i < 8; ++i) pop.emplace_back(nid++, 0x20000 + i * 0x400);
    unsigned calls = 0;
    for (; calls < 12000; ++calls) {
      Fake& a = pop[rnd(static_cast<unsigned>(pop.size()))];
      if (rnd(4) == 0) { const unsigned s = rnd(2); a.hit_with(s, P(1 + rnd(6))); if (rnd(5) == 0) a.hit_with(1 - s, P(1 + rnd(6))); }
      if (rnd(50) == 0) a.cutdown = true;
      if (rnd(40) == 0) a.f.atp = a.f.atp == 1 ? 2 : 1;
      if (rnd(60) == 0) a.health = 40;
      if (rnd(200) == 0) { a.id = nid++; a.ptr = 0x20000 + rnd(8) * 0x400; a.f.atp = 1; a.cooldown = 0; a.mode = 3; a.health = 40; }   // muere y reaparece, direcciones reutilizadas
      if (rnd(7) == 0) { Events ev; std::uint64_t st = calls + 1; Pick p; unsigned q = 0; while (q++ < 2 && h.e.obs.ledger.next_sweep(p, st)) h.e.obs.ledger.apply_lookup(p, rnd(5) == 0 ? Lookup::Absent : Lookup::PresentMatch, h.e.obs.epoch.epoch, ev); }
      execute_start(a);
      h.pre(a); native(a, h.w);
      const unsigned r = perturb ? rnd(10) : 99;
      if (r == 0) continue;                       // POST perdido
      h.post(a);
      if (r == 1) h.post(a);                      // POST duplicado
    }
    const Counters& c = h.e.obs.ledger.c;
    CHECK(h.w.bad_derefs == 0);                                           // jamas se desreferencio un puntero que no fuera un hit obj
    CHECK(h.events.size() == c.events_emitted && c.events_emitted <= c.cand_started + c.finish_started);
    CHECK(h.e.conserved() || perturb);                                    // sin perturbacion la conservacion es exacta
    CHECK(c.confirmed <= h.w.native_processed + c.not_processed);
    if (!perturb) {
      CHECK(h.e.conserved());
      CHECK(c.confirmed + c.processed_mismatch <= h.w.native_processed);
      CHECK(c.finish <= h.w.native_finish);
    }
    std::set<std::pair<std::uint32_t, std::uint64_t>> seen;                // (id, generation, event) irrepetible
    std::set<std::uint64_t> ev_ids;
    for (auto& r : h.events) CHECK(ev_ids.insert(r.event).second && r.generation != 0);
    unsigned d = h.e.obs.frames.depth();
    CHECK(d <= kFrameCapacity);
  }
}
static void t22() {
  Harness h; h.w.types[P(1)] = SWORD;
  Fake a(10, 0x9000); a.hit_with(0, P(1)); a.hit_with(1, P(1));
  h.call(a);
  char line[kLogLineMax * 2];
  const int n = fmt_hit_event(line, sizeof(line), h.events[0]);
  CHECK(n > 0 && static_cast<unsigned>(n) < sizeof(line));
  CHECK(std::strstr(line, "POST03_HIT_EVENT") && std::strstr(line, "kind=HIT_CONFIRMED") && std::strstr(line, "tg=cc0+cc1") && std::strstr(line, "dual=1") &&
        std::strstr(line, "family=SWORD") && std::strstr(line, "basis=NATIVE_BRANCH_ORDER") && std::strstr(line, "origin=UNDETERMINED") &&
        std::strstr(line, "cc_at_check=UNDETERMINED") && std::strstr(line, "eligible=CONFIRMED") && std::strstr(line, "generation=1"));
  // sin direcciones crudas (ningun literal hex de 9 o mas digitos); el at= de 8 digitos es el tipo de ataque
  unsigned run = 0, maxrun = 0;
  for (const char* p = line; *p; ++p) { if (std::strchr("0123456789abcdef", *p)) ++run; else run = 0; if (run > maxrun) maxrun = run; }
  CHECK(maxrun <= 8);
  HitReport u = h.events[0]; u.fam = FamilyResult{}; u.at_valid = false; u.fam_reason = "AT_TYPE_READ_REFUSED"; u.j.kind = HitKind::ProcessedMismatch;
  fmt_hit_event(line, sizeof(line), u);
  CHECK(std::strstr(line, "family=UNAVAILABLE") && std::strstr(line, "at=UNAVAILABLE") && std::strstr(line, "AT_TYPE_READ_REFUSED") && !std::strstr(line, "at=0"));
  Counters k; k.cand_started = 3; k.confirmed = 2;
  char big[kLogLineMax * 4];
  const int m = fmt_hit_summary(big, sizeof(big), k, "SHUTDOWN");
  CHECK(m > 0 && static_cast<std::size_t>(m) < sizeof(big) && std::strstr(big, "POST03_HIT_SUMMARY") && std::strstr(big, "confirmed=2"));
  // el motor nunca produce ACCEPTED_*: solo los veredictos propios de POST03
  CHECK(std::strcmp(name(HitKind::Confirmed), "HIT_CONFIRMED") == 0 && !std::strstr(name(HitKind::ProcessedMismatch), "ACCEPTED"));
}

int main() {
  t01_t02(); t03(); t04(); t05(); t06(); t07(); t08(); t09_t10(); t11_t12(); t13(); t14(); t16(); t17_t18(); t19(); t21(); t22();
  std::printf("POST03_1_HIT_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
