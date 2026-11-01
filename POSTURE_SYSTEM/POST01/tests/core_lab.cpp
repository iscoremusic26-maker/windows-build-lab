// POST01 laboratorio: logica pura con DOBLES. Un LAB_PASS no es runtime: no ejecuta el juego ni el SDK.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../src/observer_core.hpp"

using namespace post01;

static int g_checks = 0, g_fail = 0;
#define CHECK(c)                                                                         \
  do {                                                                                   \
    ++g_checks;                                                                          \
    if (!(c)) {                                                                          \
      ++g_fail;                                                                          \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);                           \
    }                                                                                    \
  } while (0)

static ActorFacts good() {
  ActorFacts f;
  f.profile_ok = true;
  f.live = true;
  f.name = NameKind::E_OC;
  f.status_10000 = true;
  f.atp = 1;
  return f;
}
static Snapshot snap(int hp, int mode, int state, int cd, int cut = 0, int tg = 0) {
  Snapshot s;
  s.health = hp; s.mode = mode; s.state = state; s.cooldown = cd;
  s.cutdown = static_cast<std::uint8_t>(cut); s.tg_bits = static_cast<std::uint8_t>(tg);
  return s;
}

static void test_name_and_admission() {
  CHECK(classify_name(nullptr) == NameKind::Unread);
  CHECK(classify_name("E_OC") == NameKind::E_OC);
  CHECK(classify_name("E_OC2") == NameKind::E_OC2);
  CHECK(classify_name("E_OC22") == NameKind::Other);
  CHECK(classify_name("E_O") == NameKind::Other);
  CHECK(classify_name("") == NameKind::Other);
  // Lectura acotada: un bloque de exactamente 6 bytes sin NUL no debe leerse mas alla (ASAN/UBSAN lo vigilan).
  char* six = static_cast<char*>(std::malloc(6));
  std::memcpy(six, "E_OC2X", 6);
  CHECK(classify_name(six) == NameKind::Other);
  std::free(six);

  CHECK(admit(good()).adm == Admission::E_OC_CANDIDATE);
  ActorFacts f = good(); f.name = NameKind::E_OC2;
  CHECK(admit(f).adm == Admission::E_OC2_EXCLUDED);  // Red/Black de DEVF y E_OC2 nativos
  f = good(); f.profile_ok = false;
  CHECK(admit(f).adm == Admission::PROFILE_OTHER);
  f = good(); f.live = false;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  f = good(); f.name = NameKind::Unread;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  f = good(); f.name = NameKind::Other;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  f = good(); f.status_10000 = false;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  f = good(); f.atp = 2;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  f = good(); f.atp = -2;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  f = good(); f.atp = -1;
  CHECK(admit(f).adm == Admission::UNKNOWN_UNSAFE);
  // Blue de DEVF y E_OC nativo: hechos identicos => mismo resultado, origen NO deducible.
  const ActorFacts native = good(), blue = good();
  CHECK(admit(native).adm == admit(blue).adm);
  CHECK(std::strcmp(admit(blue).reason, "E_OC_SIGNATURE_MATCH") == 0);
  // El HP no es identidad: ActorFacts no lo contiene y el log lo marca solo como informacion.
  char a[kLogLineMax], b[kLogLineMax];
  fmt_seen(a, sizeof(a), 1, 77, Admission::E_OC_CANDIDATE, "R", true, snap(40, 3, 0, 0));
  fmt_seen(b, sizeof(b), 1, 77, Admission::E_OC_CANDIDATE, "R", true, snap(400, 3, 0, 0));
  CHECK(std::strstr(a, "hp_info=40") && std::strstr(b, "hp_info=400") && std::strstr(a, "origin=UNDETERMINED"));
}

static void test_pre_classes() {
  CHECK(classify_pre(false, false, 0, true, true) == PreClass::UNKNOWN);
  CHECK(classify_pre(true, true, 10, true, true) == PreClass::CUT_DOWN_FINISH_PATH);
  CHECK(classify_pre(true, false, 10, true, false) == PreClass::COOLDOWN_NO_PROCESS);
  CHECK(classify_pre(true, false, 0, false, false) == PreClass::NO_TG_CANDIDATE);
  CHECK(classify_pre(true, false, 0, true, false) == PreClass::TG_CANDIDATE);
  CHECK(classify_pre(true, false, 0, false, true) == PreClass::TG_CANDIDATE);
  CHECK(classify_pre(true, false, 0, true, true) == PreClass::TG_CANDIDATE);  // dos TG = UN candidato
  CHECK(tg_source(0) == -1 && tg_source(1) == 0 && tg_source(2) == 1 && tg_source(3) == 1);  // cc[1] prevalece
  CHECK(std::strcmp(tg_text(3), "cc0+cc1") == 0);
}

static void test_evaluate() {
  // golpe aceptado con perdida de HP y paso a DAMAGE(5)
  Evaluation e = evaluate(PreClass::TG_CANDIDATE, snap(40, 3, 0, 0, 0, 1), snap(32, 5, 2, 10, 0, 1));
  CHECK(e.emit && e.processed && e.verdict == Verdict::ACCEPTED_LIKELY && e.transition && !e.lethal);
  // cooldown puesto sin cambio visible (p. ej. dano 0): no se afirma aceptacion
  e = evaluate(PreClass::TG_CANDIDATE, snap(40, 3, 0, 0, 0, 1), snap(40, 3, 0, 10, 0, 1));
  CHECK(e.verdict == Verdict::PROCESSED_NO_VISIBLE_CHANGE && e.processed);
  // solo cambio de modo SIN cooldown: no es aceptacion (otra ruta)
  e = evaluate(PreClass::TG_CANDIDATE, snap(40, 3, 0, 0, 0, 1), snap(40, 5, 2, 0, 0, 1));
  CHECK(e.verdict == Verdict::EARLY_RETURN_POSSIBLE && !e.processed);
  // solo perdida de HP SIN cooldown: tampoco
  e = evaluate(PreClass::TG_CANDIDATE, snap(40, 3, 0, 0, 0, 2), snap(30, 3, 0, 0, 0, 2));
  CHECK(e.verdict == Verdict::EARLY_RETURN_POSSIBLE);
  // golpe letal
  e = evaluate(PreClass::TG_CANDIDATE, snap(5, 3, 0, 0, 0, 3), snap(0, 6, 0, 10, 0, 3));
  CHECK(e.lethal && e.verdict == Verdict::ACCEPTED_LIKELY);
  // HP alterado por HM (p. ej. 400): la evaluacion depende de deltas, no del valor
  e = evaluate(PreClass::TG_CANDIDATE, snap(400, 3, 0, 0, 0, 1), snap(380, 5, 2, 10, 0, 1));
  CHECK(e.verdict == Verdict::ACCEPTED_LIKELY);
  // Finish
  e = evaluate(PreClass::CUT_DOWN_FINISH_PATH, snap(12, 9, 5, 0, 1, 0), snap(0, 9, 1, 0, 0, 0));
  CHECK(e.emit && e.verdict == Verdict::FINISH_PATH_OBSERVED);
  e = evaluate(PreClass::CUT_DOWN_FINISH_PATH, snap(12, 6, 5, 0, 1, 0), snap(12, 6, 5, 0, 1, 0));
  CHECK(e.emit && e.verdict == Verdict::FINISH_PATH_UNCONFIRMED);
  // cooldown / sin TG: sin evento, anomalia solo si algo cambia
  e = evaluate(PreClass::COOLDOWN_NO_PROCESS, snap(40, 3, 0, 7, 0, 1), snap(40, 3, 0, 7, 0, 1));
  CHECK(!e.emit && !e.anomaly);
  e = evaluate(PreClass::NO_TG_CANDIDATE, snap(40, 3, 0, 0), snap(35, 3, 0, 0));
  CHECK(!e.emit && e.anomaly);
  // sin datos seguros
  e = evaluate(PreClass::UNKNOWN, snap(1, 1, 1, 1), snap(2, 2, 2, 2));
  CHECK(!e.emit && e.verdict == Verdict::UNKNOWN);
  // ACCEPTED_CONFIRMED NUNCA se emite en POST01 (barrido de combinaciones)
  for (int cls = 0; cls < 5; ++cls)
    for (int hp0 : {0, 1, 40}) for (int hp1 : {0, 1, 40}) for (int m0 : {3, 5, 9}) for (int m1 : {3, 5, 6, 9})
      for (int cd0 : {0, 7}) for (int cd1 : {0, 10}) for (int tg : {0, 1, 2, 3}) {
        const Evaluation v = evaluate(static_cast<PreClass>(cls), snap(hp0, m0, 0, cd0, 0, tg), snap(hp1, m1, 1, cd1, 0, tg));
        CHECK(v.verdict != Verdict::ACCEPTED_CONFIRMED);
      }
  // dos TG simultaneos: un solo evento (el evaluador no multiplica)
  e = evaluate(PreClass::TG_CANDIDATE, snap(40, 3, 0, 0, 0, 3), snap(30, 5, 2, 10, 0, 3));
  CHECK(e.emit);
}

static void test_at_class() {
  const AtMasks m{0x4000002u, 0x10u, 0x2000u, 0x20u, 0x400000u, 0x10000u, 0x40u, 0x80u};  // mascaras SINTETICAS del lab
  CHECK(std::strcmp(classify_at(0x4000000u, m), "SWORD") == 0);
  CHECK(std::strcmp(classify_at(0x10u, m), "SHIELD_ATTACK") == 0);
  CHECK(std::strcmp(classify_at(0x2000u, m), "ARROW") == 0);
  CHECK(std::strcmp(classify_at(0x20u, m), "BOMB") == 0);
  CHECK(std::strcmp(classify_at(0x400000u, m), "IRON_BALL") == 0);
  CHECK(std::strcmp(classify_at(0x10000u, m), "BOOMERANG") == 0);
  CHECK(std::strcmp(classify_at(0x40u, m), "AT_TYPE_40") == 0);
  CHECK(std::strcmp(classify_at(0x80u, m), "SLINGSHOT") == 0);
  CHECK(std::strcmp(classify_at(0x1u, m), "OTHER") == 0);
  CHECK(std::strcmp(classify_at(0x4000010u, m), "SWORD") == 0);  // prioridad igual a la cadena nativa
}

static void test_frames() {
  Observer o;
  // PRE/POST de la misma invocacion
  Frame* f = o.push(0x1000, 5);
  CHECK(f && f->serial == 1 && o.depth() == 1);
  Frame out;
  CHECK(o.end(0x1000, out) && out.id == 5 && o.depth() == 0);
  // POST sin PRE (otro mod hizo SKIP): no empareja
  CHECK(!o.end(0x1000, out) && o.c.unmatched_post == 1);
  // anidamiento con actores distintos
  o.push(0xA, 1); o.push(0xB, 2);
  CHECK(o.end(0xB, out) && out.id == 2 && o.depth() == 1);
  CHECK(o.end(0xA, out) && out.id == 1 && o.depth() == 0);
  // un POST perdido: A queda apilado, llega B y luego el POST de A
  o.push(0xA, 1); o.push(0xB, 2);
  CHECK(o.end(0xA, out) && out.id == 1 && o.depth() == 0 && o.c.discarded_frames == 1);  // B descartado, no mezclado
  CHECK(!o.end(0xB, out));
  // saturacion: el quinto no se almacena y su POST no empareja
  Observer p;
  for (unsigned i = 0; i < kFrameCapacity; ++i) CHECK(p.push(0x100 + i, i) != nullptr);
  CHECK(p.push(0x200, 99) == nullptr && p.c.frame_overflow == 1);
  CHECK(!p.end(0x200, out));
  for (unsigned i = kFrameCapacity; i-- > 0;) CHECK(p.end(0x100 + i, out) && out.id == i);
  CHECK(p.depth() == 0);
  // reentrancia sobre el mismo actor: el POST toma el mas reciente
  Observer q;
  q.push(0x1, 10); q.push(0x1, 11);
  CHECK(q.end(0x1, out) && out.id == 11 && q.depth() == 1);
  CHECK(q.end(0x1, out) && out.id == 10 && q.depth() == 0);
}

static void test_ledger() {
  Observer o;
  CHECK(o.touch(1, 0x10) == Touch::Inserted);
  CHECK(o.touch(1, 0x10) == Touch::Known);
  CHECK(o.touch(1, 0x20) == Touch::PtrMismatch && o.c.id_ptr_mismatch == 1);
  CHECK(o.touch(2, 0x20) == Touch::Inserted);        // misma direccion reutilizada, ID distinto = instancia nueva
  for (std::uint32_t i = 3; i < 3 + kLedgerCapacity; ++i) o.touch(i, 0x1000 + i);
  CHECK(o.c.ledger_wraps >= 1);                       // capacidad fija: sobreescritura circular, sin crecer
  CHECK(o.c.ledger_inserts == 2 + kLedgerCapacity);
}

static void test_log_budget() {
  Observer o;
  unsigned seen = 0;
  for (unsigned i = 0; i < kBudget[0] + 50; ++i) seen += o.budget.take(LogCat::Seen, o.c) ? 1 : 0;
  CHECK(seen == kBudget[0] && o.c.log_dropped[0] == 50);
  unsigned total = seen;
  for (unsigned i = 0; i < 5000; ++i) total += o.budget.take(LogCat::Hit, o.c) ? 1 : 0;
  for (unsigned i = 0; i < 5000; ++i) total += o.budget.take(LogCat::Other, o.c) ? 1 : 0;
  for (unsigned i = 0; i < 5000; ++i) total += o.budget.take(LogCat::Summary, o.c) ? 1 : 0;
  CHECK(total == kBudgetTotal && kBudgetTotal == 2000);
  // un flood de ACTOR_SEEN no puede agotar el presupuesto de HIT
  Observer p;
  for (unsigned i = 0; i < 10000; ++i) p.budget.take(LogCat::Seen, p.c);
  CHECK(p.budget.take(LogCat::Hit, p.c));
}

static void test_disable_and_summary() {
  Observer o;
  CHECK(!o.disabled);
  o.disable("A"); o.disable("B");
  CHECK(o.disabled && std::strcmp(o.disabled_reason, "A") == 0);  // el primer motivo se conserva
  Observer p;
  CHECK(!p.summary_due());
  p.c.calls = kSummaryPeriodCalls;
  CHECK(p.summary_due() && !p.summary_due());
  p.c.calls = 2 * kSummaryPeriodCalls;
  CHECK(p.summary_due());
}

static void test_install_sequence() {
  int ncall = 0;
  auto ok = [&] { ++ncall; return 0; };
  auto fail = [&] { ++ncall; return 2; };
  InstallOutcome o = install_sequence(ok, ok, ok);
  CHECK(o.state == InstallState::Ok && ncall == 2);                     // POST y PRE, sin rollback
  ncall = 0;
  o = install_sequence(fail, ok, ok);
  CHECK(o.state == InstallState::PostFailed && ncall == 1 && o.post == 2);  // PRE no se intenta
  ncall = 0;
  o = install_sequence(ok, fail, ok);
  CHECK(o.state == InstallState::PreFailedRolledBack && ncall == 3 && o.pre == 2 && o.rollback == 0);
  ncall = 0;
  o = install_sequence(ok, fail, fail);
  CHECK(o.state == InstallState::PreFailedRollbackFailed && o.rollback == 2);  // NO se declara limpio
}

static void test_format() {
  char line[kLogLineMax];
  const Snapshot pre = snap(40, 3, 0, 0, 0, 3), post = snap(32, 5, 2, 10, 0, 3);
  Snapshot withat = pre; withat.at_valid = true; withat.at_type = 0x4000000u;
  Evaluation e = evaluate(PreClass::TG_CANDIDATE, withat, post);
  int n = fmt_hit(line, sizeof(line), 7, 123, PreClass::TG_CANDIDATE, withat, post, e, "SWORD");
  CHECK(n > 0 && static_cast<unsigned>(n) < kLogLineMax);
  CHECK(std::strstr(line, "POST01_HIT_OBSERVED") && std::strstr(line, "tg=cc0+cc1") && std::strstr(line, "at=0x04000000/SWORD"));
  CHECK(std::strstr(line, "hp=40>32") && std::strstr(line, "mode=3>5") && std::strstr(line, "verdict=ACCEPTED_LIKELY"));
  // collider desconocido => UNKNOWN, nunca 0
  e = evaluate(PreClass::TG_CANDIDATE, pre, post);
  fmt_hit(line, sizeof(line), 8, 123, PreClass::TG_CANDIDATE, pre, post, e, "OTHER");
  CHECK(std::strstr(line, "at=UNKNOWN") && !std::strstr(line, "at=0"));
  // ACTOR_SEEN sin detalle: UNKNOWN
  fmt_seen(line, sizeof(line), 1, 9, Admission::E_OC2_EXCLUDED, "RESOURCE_E_OC2", false, Snapshot{});
  CHECK(std::strstr(line, "hp_info=UNKNOWN") && std::strstr(line, "origin=NA"));
  char big[kLogLineMax * 2];
  Counters k; k.calls = 123456789012ull;
  n = fmt_summary(big, sizeof(big), k, "PERIODIC");
  CHECK(n > 0 && static_cast<std::size_t>(n) < sizeof(big) && std::strstr(big, "POST01_SUMMARY"));
}

int main() {
  test_name_and_admission();
  test_pre_classes();
  test_evaluate();
  test_at_class();
  test_frames();
  test_ledger();
  test_log_budget();
  test_disable_and_summary();
  test_install_sequence();
  test_format();
  std::printf("POST01_CORE_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
