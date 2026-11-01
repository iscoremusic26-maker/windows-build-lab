// POST03 laboratorio ADVERSARIAL de identidad y ciclo de vida: logica pura con DOBLES. LAB_PASS no es runtime.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include "../src/lifecycle_core.hpp"

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

static const Admission CAND = Admission::E_OC_CANDIDATE;

// Invariantes que deben cumplirse SIEMPRE, tras cualquier secuencia de operaciones.
static void invariants(const Ledger& L, const char* where) {
  std::set<std::uint32_t> ids;
  std::set<std::uintptr_t> ptrs;
  std::set<std::uint64_t> gens;
  unsigned live = 0;
  for (unsigned i = 0; i < kCapacity; ++i) {
    const Instance& s = L.slot(i);
    if (s.life == Life::Active || s.life == Life::DeathSeen || s.life == Life::Unverified) {
      ++live;
      if (!ids.insert(s.id).second) { ++g_fail; std::printf("INV %s: duplicate live id\n", where); }
      if (!ptrs.insert(s.ptr).second) { ++g_fail; std::printf("INV %s: duplicate live ptr\n", where); }
      if (s.generation == 0 || !gens.insert(s.generation).second) { ++g_fail; std::printf("INV %s: bad/duplicate generation\n", where); }
    }
    if (s.life == Life::Retired && s.retired == RetireReason::None) { ++g_fail; std::printf("INV %s: retired without reason\n", where); }
  }
  ++g_checks;
  if (live > kCapacity || live != L.active()) { ++g_fail; std::printf("INV %s: live count\n", where); }
}

struct Run {
  Ledger L;
  std::uint64_t call = 0;
  std::uint32_t epoch = 1;
  std::vector<Event> log;
  std::set<std::uint64_t> all_generations;
  Observation see(std::uint32_t id, std::uintptr_t ptr, Admission a = CAND) {
    Events ev;
    Observation o = L.observe(id, ptr, a, epoch, ++call, ev);
    for (unsigned i = 0; i < ev.n; ++i) {
      log.push_back(ev.e[i]);
      if (ev.e[i].kind == EvKind::FirstSeen) {
        // una generacion NUNCA se reutiliza
        CHECK(all_generations.insert(ev.e[i].generation).second);
      }
    }
    invariants(L, "see");
    return o;
  }
  void lookup(const Pick& p, Lookup r) {
    Events ev;
    L.apply_lookup(p, r, epoch, ev);
    for (unsigned i = 0; i < ev.n; ++i) log.push_back(ev.e[i]);
    invariants(L, "lookup");
  }
  // barrido completo de todas las instancias con un resultado dado por la funcion f(id)
  template <class F>
  void sweep_all(F f) {
    std::uint64_t stamp = ++stamp_;
    Pick p;
    while (L.next_sweep(p, stamp)) lookup(p, f(p));
  }
  unsigned count(EvKind k) const { unsigned n = 0; for (auto& e : log) n += e.kind == k; return n; }
  unsigned retired(RetireReason r) const { unsigned n = 0; for (auto& e : log) n += (e.kind == EvKind::Retired && e.reason == r); return n; }
  void death(const Observation& o) { Events ev; L.mark_death(o.slot, o.generation, epoch, ev); for (unsigned i = 0; i < ev.n; ++i) log.push_back(ev.e[i]); }
 private:
  std::uint64_t stamp_ = 0;
};

// llena el ledger con vivos de relleno (ids 9000+) para forzar la reutilizacion de slots RETIRADOS
static void fill(Run& r, unsigned n) { for (unsigned i = 0; i < n; ++i) r.see(9000 + i, 0x800000 + i * 0x40); }
// 01-02
static void t_first_and_reobserve() {
  Run r;
  Observation a = r.see(100, 0x1000);
  CHECK(a.tracked && a.created && a.generation == 1);
  Observation b = r.see(100, 0x1000);
  CHECK(b.tracked && !b.created && b.generation == a.generation && b.slot == a.slot);
  CHECK(r.count(EvKind::FirstSeen) == 1);
  CHECK(r.L.c.records_created == 1 && r.L.active() == 1);
}
// 03 mismo ptr, id distinto
static void t_same_ptr_new_id() {
  Run r;
  Observation a = r.see(100, 0x1000);
  r.L.add_hit(a.slot, a.generation);
  Observation b = r.see(101, 0x1000);
  CHECK(b.created && b.generation != a.generation);
  CHECK(!r.L.valid(a.slot, a.generation) || a.slot != b.slot);  // la generacion vieja ya no es valida
  CHECK(r.retired(RetireReason::AddressReused) == 1 && r.L.c.address_reused == 1);
  CHECK(r.L.slot(b.slot).hits == 0);  // sin estado arrastrado
  CHECK(!delete_claim(RetireReason::AddressReused));
}
// 04 mismo id, ptr distinto
static void t_same_id_new_ptr() {
  Run r;
  Observation a = r.see(100, 0x1000);
  r.L.add_hit(a.slot, a.generation);
  Observation b = r.see(100, 0x2000);
  CHECK(b.created && b.generation != a.generation);
  CHECK(r.L.c.identity_conflicts == 1 && r.retired(RetireReason::IdentityConflict) == 1);
  CHECK(r.L.slot(b.slot).hits == 0);
  CHECK(!r.L.valid(a.slot, a.generation) || a.slot != b.slot);
  CHECK(r.L.active() == 1);
}
// 05 retiro + reaparicion con los mismos valores
static void t_retire_then_reappear() {
  Run r;
  Observation a = r.see(100, 0x1000);
  r.sweep_all([](const Pick&) { return Lookup::Absent; });
  CHECK(r.retired(RetireReason::LookupAbsentVerified) == 1 && r.L.active() == 0);
  r.epoch = 2;
  Observation b = r.see(100, 0x1000);  // mismo id y ptr tras retiro y otro epoch
  CHECK(b.created && b.generation != a.generation && b.generation > a.generation);
  CHECK(r.L.slot(b.slot).epoch_first == 2);
  CHECK(r.L.slot(b.slot).hits == 0 && r.L.slot(b.slot).life == Life::Active);
}
// 06 muerte != borrado
static void t_death_is_not_delete() {
  Run r;
  Observation a = r.see(100, 0x1000);
  CHECK(death_evidence(0, 3) && death_evidence(-5, 3) && death_evidence(10, 9) && death_evidence(10, 10) && death_evidence(10, 13));
  CHECK(!death_evidence(10, 3) && !death_evidence(1, 6));
  r.death(a);
  r.death(a);  // idempotente
  CHECK(r.count(EvKind::DeathSeen) == 1 && r.L.slot(a.slot).life == Life::DeathSeen);
  CHECK(r.count(EvKind::Retired) == 0 && r.L.active() == 1);  // sigue en el ledger, sin retiro
  Observation b = r.see(100, 0x1000);                          // el actor aun ejecuta: misma generacion
  CHECK(b.generation == a.generation && !b.created);
}
// 07 ausencia verificable -> retiro y reaprovechamiento de slot
static void t_absent_reuses_slot() {
  Run r;
  Observation a = r.see(100, 0x1000);
  Pick p;
  CHECK(r.L.next_sweep(p, 1) && p.id == 100 && p.generation == a.generation);
  r.lookup(p, Lookup::Absent);
  CHECK(r.L.slot(a.slot).life == Life::Retired && r.L.c.absent_verified == 1);
  fill(r, kCapacity - 1);  // el resto de la tabla queda ocupado por vivos
  Observation b = r.see(200, 0x3000);
  CHECK(b.slot == a.slot && b.generation != a.generation);  // sin libres, se reaprovecha el slot RETIRADO con generacion nueva
  CHECK(r.L.c.capacity_drops == 0 && r.L.c.capacity_evictions == 0);
}
// 08 lookup no disponible / transitorio -> UNVERIFIED, nunca delete
static void t_unavailable_never_deletes() {
  Run r;
  Observation a = r.see(100, 0x1000);
  for (unsigned i = 0; i < 2; ++i) r.sweep_all([](const Pick&) { return Lookup::Unavailable; });
  CHECK(r.L.slot(a.slot).life == Life::Active && r.L.c.lookup_unavailable == 2);  // aun por debajo del limite
  r.sweep_all([](const Pick&) { return Lookup::Failed; });
  CHECK(r.L.slot(a.slot).life == Life::Unverified && r.L.slot(a.slot).unv == UnverifiedReason::LookupUnavailable);
  CHECK(r.count(EvKind::Retired) == 0 && r.L.c.absent_verified == 0);
  r.sweep_all([](const Pick&) { return Lookup::PresentMatch; });
  CHECK(r.L.slot(a.slot).life == Life::Active && r.L.c.revalidated == 1);
  Run q;
  Observation c = q.see(7, 0x70);
  q.sweep_all([](const Pick&) { return Lookup::Creating; });
  CHECK(q.L.slot(c.slot).life == Life::Unverified && q.L.slot(c.slot).unv == UnverifiedReason::Creating && q.count(EvKind::Retired) == 0);
}
// 09 lookup devuelve otro ptr/perfil: no mezclar generaciones
static void t_lookup_mismatch() {
  Run r;
  Observation a = r.see(100, 0x1000);
  r.sweep_all([](const Pick&) { return Lookup::PresentMismatchPtr; });
  CHECK(r.L.slot(a.slot).life == Life::Unverified && r.L.slot(a.slot).unv == UnverifiedReason::PtrMismatch);
  CHECK(r.L.slot(a.slot).generation == a.generation && r.L.c.absent_verified == 0);
  r.sweep_all([](const Pick&) { return Lookup::PresentMismatchProfile; });
  CHECK(r.L.slot(a.slot).unv == UnverifiedReason::ProfileMismatch && r.L.slot(a.slot).generation == a.generation);
  Observation b = r.see(100, 0x1000);  // el hook vuelve a ver la instancia: revalida sin generacion nueva
  CHECK(b.generation == a.generation && r.L.slot(a.slot).life == Life::Active);
}
// 10 cambio de contexto != delete
static void t_context_change() {
  Run r;
  Observation a = r.see(100, 0x1000);
  r.epoch = 2;  // otro stage
  r.sweep_all([](const Pick&) { return Lookup::PresentMatch; });  // sigue existiendo (otra sala cargada): NO se retira
  CHECK(r.L.slot(a.slot).life == Life::Active && r.count(EvKind::Retired) == 0);
  r.sweep_all([](const Pick&) { return Lookup::Unavailable; });
  r.sweep_all([](const Pick&) { return Lookup::Unavailable; });
  CHECK(r.L.slot(a.slot).life == Life::Active && r.count(EvKind::Retired) == 0);  // aun no concluyente: no se retira por contexto
  r.sweep_all([](const Pick&) { return Lookup::Unavailable; });
  // no concluyente + contexto anterior => CONTEXT_RETIRED, que no afirma borrado nativo
  CHECK(r.L.slot(a.slot).life == Life::Retired && r.L.slot(a.slot).retired == RetireReason::ContextRetired);
  CHECK(!delete_claim(RetireReason::ContextRetired) && r.L.c.context_retired == 1 && r.L.c.absent_verified == 0);
  // y si el lookup demuestra ausencia tras el cambio de contexto, es ausencia verificada con context_changed
  Run q;
  Observation c = q.see(5, 0x50);
  q.epoch = 3;
  q.sweep_all([](const Pick&) { return Lookup::Absent; });
  bool seen = false;
  for (auto& e : q.log) seen |= (e.kind == EvKind::Retired && e.reason == RetireReason::LookupAbsentVerified && e.context_changed);
  CHECK(seen && c.tracked);
  CHECK(!epoch_older(5, 5) && epoch_older(5, 6) && !epoch_older(6, 5));
  // wrap de epoch (u32) simulado
  CHECK(epoch_older(0xFFFFFFFFu, 0u) && !epoch_older(0u, 0xFFFFFFFFu));
  EpochTracker t;
  t.update(stage_token("F_SP121"));
  t.update(stage_token("F_SP121"));
  CHECK(t.epoch == 1);
  t.update(stage_token("F_SP108"));
  CHECK(t.epoch == 2);
  t.epoch = 0xFFFFFFFFu;
  t.update(stage_token("F_SP121"));
  CHECK(t.epoch == 0);  // wrap definido, sin UB
  CHECK(stage_token(nullptr) == stage_token("") && stage_token("ABCDEFGHIJ") == stage_token("ABCDEFGH"));  // lectura acotada a 8 bytes
}
// 11 tabla llena con vivos: sin overwrite silencioso
static void t_full_table_no_silent_overwrite() {
  Run r;
  std::vector<Observation> v;
  for (unsigned i = 0; i < kCapacity; ++i) v.push_back(r.see(1000 + i, 0x10000 + i * 0x10));
  CHECK(r.L.active() == kCapacity && r.L.c.max_active == kCapacity);
  Observation x = r.see(5000, 0x90000);
  CHECK(!x.tracked && x.slot == -1 && r.L.c.capacity_drops == 1 && r.count(EvKind::CapacityExhausted) == 1);
  for (unsigned i = 0; i < kCapacity; ++i) CHECK(r.L.valid(v[i].slot, v[i].generation));  // ningun vivo fue expulsado
  CHECK(r.L.c.capacity_evictions == 0);
  // un DEATH_SEEN tampoco se expulsa
  r.death(v[0]);
  Observation y = r.see(5001, 0x91000);
  CHECK(!y.tracked && r.L.valid(v[0].slot, v[0].generation));
  // pero si hay una UNVERIFIED, se evicta de forma explicita (no es borrado)
  r.sweep_all([&](const Pick& p) { return p.id == 1005 ? Lookup::PresentMismatchPtr : Lookup::PresentMatch; });
  Observation z = r.see(5002, 0x92000);
  CHECK(z.tracked && r.L.c.capacity_evictions == 1 && r.retired(RetireReason::CapacityEvicted) == 1);
  CHECK(!delete_claim(RetireReason::CapacityEvicted) && r.L.c.absent_verified == 0);
  // un retirado libera sitio sin evicciones
  r.sweep_all([&](const Pick& p) { return p.id == 1010 ? Lookup::Absent : Lookup::PresentMatch; });
  Observation w = r.see(5003, 0x93000);
  CHECK(w.tracked && r.L.c.capacity_evictions == 1);
}
// 12 treinta y dos encuentros sucesivos, mas que la capacidad acumulada
static void t_many_encounters() {
  Run r;
  unsigned created = 0;
  std::uint32_t id = 10;
  for (unsigned enc = 0; enc < 32; ++enc) {
    std::vector<Observation> grp;
    for (unsigned k = 0; k < 8; ++k) { grp.push_back(r.see(id, 0x100000 + (id % 5) * 0x40)); ++id; }  // direcciones muy reutilizadas
    for (auto& o : grp) created += o.created ? 1u : 0u;
    r.epoch += (enc % 4 == 3);
    if (enc % 2) r.sweep_all([](const Pick&) { return Lookup::Absent; });
    invariants(r.L, "enc");
  }
  CHECK(created == r.L.c.records_created && r.L.c.records_created > kCapacity);  // mas vidas que slots
  CHECK(r.all_generations.size() == r.L.c.records_created);                       // ninguna generacion repetida
  CHECK(r.L.c.capacity_drops == 0);
  CHECK(r.L.c.records_created == r.count(EvKind::FirstSeen));
  CHECK(r.L.c.address_reused + r.L.c.absent_verified + r.L.active() >= r.L.c.records_created - 0);  // todo creado esta vivo o retirado
}
// 13 reentrancia / POST faltante / duplicado / overflow / retirada entre PRE y POST
static void t_frames() {
  Counters c;
  FrameStack f;
  CallFrame out;
  CHECK(f.push(0xA, 1, c) && f.depth() == 1);
  CHECK(f.end(0xA, out, c) && out.id == 1 && f.depth() == 0);
  CHECK(!f.end(0xA, out, c) && c.dup_post_orphan == 1 && c.unmatched_post == 0);  // POST duplicado de un frame ya cerrado
  CHECK(!f.end(0xDEAD, out, c) && c.unmatched_post == 1);                            // POST de un actor nunca visto: huerfano
  f.push(0xA, 1, c); f.push(0xB, 2, c);
  CHECK(f.end(0xA, out, c) && out.id == 1 && c.discarded_frames == 1 && f.depth() == 0);  // POST de B perdido
  CHECK(!f.end(0xB, out, c));
  for (unsigned i = 0; i < kFrameCapacity; ++i) CHECK(f.push(0x100 + i, i, c));
  CHECK(!f.push(0x500, 9, c) && c.frame_overflow == 1);
  CHECK(!f.end(0x500, out, c));
  for (unsigned i = kFrameCapacity; i-- > 0;) CHECK(f.end(0x100 + i, out, c) && out.id == i);
  f.push(0x1, 10, c); f.push(0x1, 11, c);  // mismo actor: damage_check no es recursivo, el frame anterior se considera POST perdido
  CHECK(c.stale_frames == 1 && f.depth() == 1);
  CHECK(f.end(0x1, out, c) && out.id == 11 && !f.end(0x1, out, c));
  // retirada entre PRE y POST: la generacion del frame deja de ser valida
  Run r;
  Observation a = r.see(100, 0x1000);
  CallFrame fr; fr.slot = a.slot; fr.generation = a.generation; fr.tracked = true;
  CHECK(r.L.valid(fr.slot, fr.generation));
  r.see(100, 0x2000);  // conflicto: el mismo ID aparece en otra direccion mientras el frame esta pendiente
  CHECK(!(r.L.valid(fr.slot, fr.generation) && r.L.slot(fr.slot).ptr == 0x1000));
  Run q;
  Observation b = q.see(1, 0x10);
  q.sweep_all([](const Pick&) { return Lookup::Absent; });
  CHECK(!q.L.valid(b.slot, b.generation));
  fill(q, kCapacity - 1);
  Observation b2 = q.see(2, 0x20);  // el slot se reutiliza: la generacion antigua NO valida al nuevo actor
  CHECK(b2.slot == b.slot && !q.L.valid(b.slot, b.generation) && q.L.valid(b2.slot, b2.generation));
}
// 14-16 firma Blue/nativo, E_OC2, transiciones transitorias, HP modificado
static void t_admission_and_origin() {
  ActorFacts f;
  f.profile_ok = f.live = true; f.name = NameKind::E_OC; f.status_10000 = true; f.atp = 1;
  const ActorFacts native = f, blue = f;
  CHECK(admit(native).adm == CAND && admit(blue).adm == CAND);  // indistinguibles
  char line[kLogLineMax];
  Event e; e.kind = EvKind::FirstSeen; e.id = 9; e.generation = 3; e.to = CAND;
  fmt_event(line, sizeof(line), e, 1);
  CHECK(std::strstr(line, "origin=UNDETERMINED") && !std::strstr(line, "NATIVE") && !std::strstr(line, "BLUE"));
  ActorFacts e2 = f; e2.name = NameKind::E_OC2;
  CHECK(admit(e2).adm == Admission::E_OC2_EXCLUDED);
  // cambios transitorios de AtAtp / status: la identidad se conserva, solo cambia la admision
  Run r;
  Observation a = r.see(100, 0x1000, CAND);
  Observation b = r.see(100, 0x1000, Admission::UNKNOWN_UNSAFE);  // p. ej. DEVF cambia el AtAtp un rato
  Observation c = r.see(100, 0x1000, CAND);
  CHECK(a.generation == b.generation && b.generation == c.generation && !b.created && !c.created);
  CHECK(r.count(EvKind::AdmissionChanged) == 2 && r.L.c.records_created == 1 && r.L.c.admission_changes == 2);
  CHECK(r.count(EvKind::Retired) == 0);
  // el HP no entra en la clave: observe() ni lo recibe; la identidad es la misma con HP 40 que con 400
  Observation d = r.see(100, 0x1000);
  CHECK(d.generation == a.generation);
  // un E_OC2 se registra como instancia pero sin origen
  Observation g = r.see(300, 0x3000, Admission::E_OC2_EXCLUDED);
  fmt_event(line, sizeof(line), r.log.back(), 2);
  CHECK(g.created && std::strstr(line, "origin=NA"));
}
// 17 reset de sesion, wrap de generacion, IDs invalidos y centinelas
static void t_sessions_and_sentinels() {
  Run r;
  CHECK(!r.see(0, 0x1000).tracked && !r.see(kIdUnknown, 0x1000).tracked && !r.see(kIdError, 0x1000).tracked && !r.see(5, 0).tracked);
  CHECK(r.L.c.invalid_keys == 4 && r.L.active() == 0 && r.count(EvKind::InvalidKey) == 4);
  Run s1, s2;  // «dos sesiones»: ledger nuevo, nada heredado
  s1.see(77, 0x1000);
  Observation o = s2.see(77, 0x1000);
  CHECK(o.generation == 1 && s2.L.c.records_created == 1);
  Run w;  // wrap de generacion: 0 queda reservado y se cuenta el rollover
  w.L.set_generation_for_test(0xFFFFFFFFFFFFFFFFull);
  Observation a = w.see(1, 0x10);
  CHECK(a.generation == 1 && w.L.c.generation_rollovers == 1);
  Observation b = w.see(2, 0x20);
  CHECK(b.generation == 2 && a.generation != 0);
  // un Pick obsoleto (slot reutilizado) no afecta a la instancia nueva
  Run q;
  Observation x = q.see(1, 0x10);
  Pick p;
  q.L.next_sweep(p, 100);
  q.sweep_all([](const Pick&) { return Lookup::Absent; });  // retira x
  fill(q, kCapacity - 1);
  Observation y = q.see(2, 0x20);                            // reutiliza el slot
  q.lookup(p, Lookup::Absent);                               // pick viejo
  CHECK(q.L.c.stale_picks == 1 && q.L.valid(y.slot, y.generation) && x.slot == y.slot);
}
// 18 errores de servicio/log: observador inerte (fail-open)
static void t_fail_open() {
  Observer o;
  CHECK(!o.disabled);
  o.disable("A"); o.disable("B");
  CHECK(o.disabled && std::strcmp(o.disabled_reason, "A") == 0);
  Observer p;
  CHECK(!p.summary_due());
  p.ledger.c.calls = kSummaryPeriodCalls;
  CHECK(p.summary_due() && !p.summary_due());
  Observer q;
  unsigned due = 0;
  for (unsigned i = 0; i < 100; ++i) due += q.sweep_due() ? 1u : 0u;
  CHECK(due == 100 / kSweepPeriodTicks);
  // presupuesto de log: tope duro y un flood de un tipo no agota los otros
  Observer b;
  for (unsigned i = 0; i < 10000; ++i) b.budget.take(LogCat::Life, b.ledger.c);
  CHECK(b.budget.take(LogCat::Hit, b.ledger.c) && b.budget.used[0] == kBudget[0]);
  unsigned total = 0;
  Observer t;
  for (unsigned cat = 0; cat < 4; ++cat) for (unsigned i = 0; i < 5000; ++i) total += t.budget.take(static_cast<LogCat>(cat), t.ledger.c) ? 1u : 0u;
  CHECK(total == kBudgetTotal && kBudgetTotal == 1500);
  // throttle por tipo ruidoso
  Ledger L;
  unsigned allowed = 0;
  for (unsigned i = 0; i < 100; ++i) allowed += L.may_log(EvKind::AdmissionChanged) ? 1u : 0u;
  CHECK(allowed == 40 && L.c.kind_throttled == 60 && L.may_log(EvKind::FirstSeen) && L.may_log(EvKind::Retired));
  // instalacion con fallos
  int n = 0;
  auto ok = [&] { ++n; return 0; };
  auto bad = [&] { ++n; return 2; };
  CHECK(install_sequence(ok, ok, ok).state == InstallState::Ok);
  CHECK(install_sequence(bad, ok, ok).state == InstallState::PostFailed);
  CHECK(install_sequence(ok, bad, ok).state == InstallState::PreFailedRolledBack);
  CHECK(install_sequence(ok, bad, bad).state == InstallState::PreFailedRollbackFailed);
}
// formato: sin punteros, sin ceros engañosos
static void t_format() {
  char line[kLogLineMax * 2];
  Event e; e.id = 12; e.generation = 4; e.epoch = 2; e.to = e.from = CAND;
  e.kind = EvKind::Retired; e.reason = RetireReason::LookupAbsentVerified;
  fmt_event(line, sizeof(line), e, 5);
  CHECK(std::strstr(line, "POST03_INSTANCE_RETIRED") && std::strstr(line, "delete_claim=1") && std::strstr(line, "generation=4"));
  e.reason = RetireReason::CapacityEvicted;
  fmt_event(line, sizeof(line), e, 5);
  CHECK(std::strstr(line, "delete_claim=0") && std::strstr(line, "CAPACITY_EVICTED"));
  e.kind = EvKind::DeathSeen;
  fmt_event(line, sizeof(line), e, 5);
  CHECK(std::strstr(line, "NO_DELETE_CLAIM"));
  e.kind = EvKind::InvalidKey; e.id = 0xFFFFFFFEu;
  fmt_event(line, sizeof(line), e, 5);
  CHECK(std::strstr(line, "0xfffffffe"));
  // el formato de impacto se prueba en hit_lab.cpp; aqui solo el resumen de ciclo de vida
  Counters k; k.calls = 123456789012ull; k.records_created = 7;
  int n = fmt_summary(line, sizeof(line), k, 3, 2, 0, "SHUTDOWN");
  CHECK(n > 0 && static_cast<std::size_t>(n) < sizeof(line) && std::strstr(line, "POST03_SUMMARY") && std::strstr(line, "records_created=7"));
}
// aleatorio determinista: secuencias largas respetan invariantes
static void t_random_walk() {
  std::uint64_t seed = 0x9E3779B97F4A7C15ull;
  auto rnd = [&](unsigned m) { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return static_cast<unsigned>(seed % m); };
  Run r;
  std::uint32_t next_id = 1;
  for (unsigned step = 0; step < 6000; ++step) {
    switch (rnd(6)) {
      case 0: case 1: r.see(next_id++, 0x1000 + rnd(80) * 0x40); break;                 // direcciones muy reutilizadas
      case 2: r.see(1 + rnd(next_id), 0x1000 + rnd(80) * 0x40); break;                  // ids viejos con ptr arbitrario
      case 3: { Observation o = r.see(next_id++, 0x1000 + rnd(80) * 0x40); r.death(o); break; }
      case 4: r.epoch += rnd(3) == 0; r.sweep_all([&](const Pick&) { return static_cast<Lookup>(rnd(7)); }); break;
      default: r.sweep_all([](const Pick&) { return Lookup::PresentMatch; }); break;
    }
  }
  invariants(r.L, "walk");
  CHECK(r.L.c.records_created == r.count(EvKind::FirstSeen));
  CHECK(r.all_generations.size() == r.L.c.records_created);
}

int main() {
  t_first_and_reobserve();
  t_same_ptr_new_id();
  t_same_id_new_ptr();
  t_retire_then_reappear();
  t_death_is_not_delete();
  t_absent_reuses_slot();
  t_unavailable_never_deletes();
  t_lookup_mismatch();
  t_context_change();
  t_full_table_no_silent_overwrite();
  t_many_encounters();
  t_frames();
  t_admission_and_origin();
  t_sessions_and_sentinels();
  t_fail_open();
  t_format();
  t_random_walk();
  std::printf("POST03_1_LIFECYCLE_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
