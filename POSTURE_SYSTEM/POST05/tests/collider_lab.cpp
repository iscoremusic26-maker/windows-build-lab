// POST03.1 laboratorio del GATE DE COLLIDER: motor puro con punteros OPACOS elegidos a mano. LAB_PASS = consistencia del criterio; NO es runtime.
// Invariante central: la lectura (read_at) solo ocurre con HIT_CONFIRMED + sel == hit obj crudo de una esfera + multiplo de 4 + >= 0x10000, y nunca mas de 1 vez por evento.
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
static constexpr std::uint32_t SWORD = 0x04000002u, ARROW = 0x2000u;

struct Out {
  bool event = false;
  int reads = 0;
  std::vector<std::uintptr_t> read_ptrs;
  HitReport rep;
  Engine e;
};
struct Scn {
  std::uintptr_t raw0 = 0, raw1 = 0, sel = 0;
  std::uint8_t bits = 1;
  std::uint32_t type0 = SWORD, type1 = SWORD;  // tipo AT de cada hit obj
  int pre_mode = 3, post_mode = 5, post_old = 3, post_cd = 10, pre_cd = 0;
  bool cutdown = false;
};
static ActorFacts good() { ActorFacts f; f.profile_ok = f.live = f.status_10000 = true; f.name = NameKind::E_OC; f.atp = 1; return f; }

static void run(const Scn& s, Out& o, std::uint32_t id = 10, std::uintptr_t actor = 0x9000) {
  o.e.masks = M;
  PreReads pr; pr.facts = good();
  pr.snap.health = 40; pr.snap.mode = s.pre_mode; pr.snap.cooldown = s.pre_cd; pr.snap.cutdown = s.cutdown ? 1 : 0; pr.snap.tg_bits = s.bits;
  pr.raw0 = (s.bits & 1) ? s.raw0 : 0; pr.raw1 = (s.bits & 2) ? s.raw1 : 0;
  Events ev; o.e.on_pre(actor, id, pr, ev);
  PostReads po; po.alive = true; po.snap = pr.snap;
  po.snap.health = 20; po.snap.mode = s.post_mode; po.snap.old_mode = s.post_old; po.snap.cooldown = s.post_cd; po.snap.tg_bits = 0;
  po.selected = s.sel;
  const int before = o.reads;
  o.event = o.e.on_post(actor, po, [&](std::uintptr_t p, std::uint32_t& t) {
    ++o.reads; o.read_ptrs.push_back(p);
    if (p == s.raw1 && (s.bits & 2)) t = s.type1; else t = s.type0;
    return true;
  }, ev, o.rep);
  CHECK(o.reads - before <= 1);                 // jamas dos lecturas por evento
}

static void c01_c03() {
  Out a; Scn s; s.raw0 = 0x200000; s.sel = 0;
  run(s, a);
  CHECK(a.event && a.reads == 0 && a.rep.col.reason == ColReason::NoSelected && a.rep.fam.family == Family::Unavailable);       // C01
  Out b; s.sel = 0x8000;
  run(s, b);
  CHECK(b.reads == 0 && b.rep.col.reason == ColReason::SelectedTooLow && b.rep.col.sel_class == PtrClass::TooLow);              // C02
  Out c; s.sel = 0x300000;                                                                                                       // C03: bien alineado, distinto de raw
  run(s, c);
  CHECK(c.reads == 0 && c.rep.col.reason == ColReason::NotEqualToRaw && c.rep.col.src == -1 && c.rep.fam.family == Family::Unavailable);
  Out d; s.bits = 1; s.raw0 = 0; s.sel = 0x300000;                                                                              // TG activo con hit obj crudo nulo
  run(s, d);
  CHECK(d.reads == 0 && d.rep.col.reason == ColReason::RawNull);
}
static void c04_c08() {
  Scn s; s.raw0 = 0x200000; s.raw1 = 0x200400; 
  s.bits = 1; s.sel = s.raw0; Out a; run(s, a);                                                                                 // C04
  CHECK(a.event && a.reads == 1 && a.rep.col.src == 0 && a.rep.col.reason == ColReason::FreshMatchCc0 && a.rep.fam.family == Family::Sword && a.rep.at_valid);
  s.bits = 2; s.sel = s.raw1; Out b; run(s, b);                                                                                 // C05
  CHECK(b.reads == 1 && b.rep.col.src == 1 && b.rep.col.reason == ColReason::FreshMatchCc1 && b.read_ptrs[0] == s.raw1);
  s.bits = 3; s.type1 = ARROW; s.sel = s.raw1; Out c; run(s, c);                                                                // C06: dos colliders distintos, gana cc1
  CHECK(c.event && c.reads == 1 && c.read_ptrs[0] == s.raw1 && c.rep.col.pair_distinct && c.rep.fam.family == Family::Arrow && c.rep.j.dual_tg);
  Scn t; t.raw0 = t.raw1 = 0x200000; t.bits = 3; t.sel = t.raw0; Out d; run(t, d);                                              // C07: mismo collider
  CHECK(d.reads == 1 && !d.rep.col.pair_distinct && d.rep.col.src == 1 && d.rep.fam.family == Family::Sword);
  Scn u; u.raw0 = 0x200000; u.raw1 = 0x200400; u.bits = 3; u.sel = 0x200800; Out e; run(u, e);                                  // C08: ninguna coincide
  CHECK(e.event && e.reads == 0 && e.rep.col.reason == ColReason::NotEqualToRaw);
}
static void c09_c10() {
  // C09: puntero 4 mod 8 que SI coincide con el hit obj crudo: ahora se lee (el campo leido solo exige alineacion de 4)
  Scn s; s.raw0 = 0x200004; s.sel = s.raw0; s.bits = 1; Out a; run(s, a);
  CHECK(ptr_class(0x200004) == PtrClass::Mod8Four && a.reads == 1 && a.rep.col.deref_ok && a.rep.col.sel_class == PtrClass::Mod8Four && a.rep.fam.family == Family::Sword);
  CHECK(a.e.obs.ledger.c.deref_mod8_4 == 1 && a.e.obs.ledger.c.deref_mod8_0 == 0 && a.e.obs.ledger.c.sel_class[static_cast<unsigned>(PtrClass::Mod8Four)] == 1);
  // 4 mod 8 que NO coincide: sigue sin leerse
  Scn t; t.raw0 = 0x200008; t.sel = 0x200004; Out b; run(t, b);
  CHECK(b.reads == 0 && b.rep.col.reason == ColReason::NotEqualToRaw);
  // C10: no multiplo de 4 aunque coincida con el crudo: SELECTED_ALIGNMENT_UNSUPPORTED, sin lectura, pero la coincidencia queda registrada
  for (std::uintptr_t off : {1u, 2u, 3u, 5u, 6u, 7u}) {
    Scn u; u.raw0 = 0x200000 + off; u.sel = u.raw0; Out c; run(u, c);
    CHECK(c.event && c.reads == 0 && c.rep.col.reason == ColReason::SelectedAlignment && c.rep.col.src == 0 && !c.rep.col.deref_ok && c.rep.col.sel_class == PtrClass::Misaligned);
  }
  CHECK(!deref_alignment_ok(0x200002) && deref_alignment_ok(0x200004) && deref_alignment_ok(0x200008) && !deref_alignment_ok(0xFFFC) && !deref_alignment_ok(0));
  CHECK(kReadAlignBytes == 4);
}
static void c11_c12() {
  // eventos sin HIT_CONFIRMED: jamas se lee
  Scn s; s.raw0 = 0x200000; s.sel = s.raw0;
  Out a; s.post_cd = 0; run(s, a);                                         // TG sin cooldown nuevo: CANDIDATE_NOT_PROCESSED
  CHECK(a.event && a.rep.j.kind == HitKind::CandidateNotProcessed && a.reads == 0);
  Out b; s.post_cd = 10; s.post_mode = 4; run(s, b);                       // firma no coincide
  CHECK(b.event && b.rep.j.kind == HitKind::ProcessedMismatch && b.reads == 0);
  Out c; s.post_mode = 5; s.post_old = 9; run(s, c);                       // mOldActionMode distinto
  CHECK(c.rep.j.kind == HitKind::ProcessedMismatch && c.reads == 0);
  Out d; Scn f; f.cutdown = true; f.bits = 0; f.sel = 0x200000; f.post_mode = 9; f.post_cd = 0; run(f, d);   // Finish con mpCollider obsoleto
  CHECK(d.event && d.rep.j.kind != HitKind::Confirmed && d.reads == 0);
  Out e; Scn g; g.raw0 = 0x200000; g.sel = g.raw0; g.pre_cd = 7; g.post_cd = 7; run(g, e);   // cooldown bloqueando: sin evento
  CHECK(!e.event && e.reads == 0);
  Out h; Scn z; z.bits = 0; z.sel = 0x200000; run(z, h);                    // sin TG, mpCollider obsoleto
  CHECK(!h.event && h.reads == 0);
}
// Propiedad: con punteros de todas las clases, el motor lee exactamente cuando un oraculo INDEPENDIENTE dice que debe leer.
static void c13_property() {
  std::uint64_t seed = 0x9E3779B97F4A7C15ull;
  auto rnd = [&](unsigned m) { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return static_cast<unsigned>(seed % m); };
  const std::uintptr_t pool[] = {0, 0x8, 0x8000, 0xFFFC, 0x10000, 0x10004, 0x200000, 0x200001, 0x200002, 0x200004, 0x200008, 0x300003, 0x30000C};
  unsigned expected_reads = 0, total_reads = 0, events = 0;
  Engine shared;
  for (unsigned i = 0; i < 6000; ++i) {
    Out o; Scn s;
    s.bits = static_cast<std::uint8_t>(1 + rnd(3));
    s.raw0 = pool[rnd(13)]; s.raw1 = pool[rnd(13)];
    const unsigned pick = rnd(4);
    s.sel = pick == 0 ? s.raw0 : pick == 1 ? s.raw1 : pick == 2 ? pool[rnd(13)] : 0;
    s.post_cd = rnd(8) ? 10 : 0; s.post_mode = rnd(8) ? 5 + static_cast<int>(rnd(2)) : 4;
    run(s, o, 100 + i, 0x9000 + i * 0x400);
    ++events;
    // oraculo independiente
    const bool signature = s.post_cd != 0 && (s.post_mode == 5 || s.post_mode == 6);
    const std::uintptr_t selected_hit = ((s.bits & 2) && s.raw1 == s.sel) ? s.raw1 : (((s.bits & 1) && s.raw0 == s.sel) ? s.raw0 : 0);
    const bool matches = s.sel != 0 && selected_hit == s.sel;
    const bool aligned = s.sel >= 0x10000 && (s.sel % 4) == 0;
    const bool should = signature && matches && aligned;
    expected_reads += should ? 1u : 0u;
    total_reads += static_cast<unsigned>(o.reads);
    CHECK(o.reads == (should ? 1 : 0));
    if (o.reads) CHECK(o.read_ptrs[0] == s.sel);
  }
  CHECK(expected_reads == total_reads && events == 6000 && expected_reads > 100);
}
static void c14_counters() {
  Out p; Scn s; s.raw0 = 0x200004; s.sel = s.raw0; run(s, p, 999, 0xF000);
  const Counters& k = p.e.obs.ledger.c;
  std::uint64_t reasons = 0; for (unsigned i = 0; i < kColReasonCount; ++i) reasons += k.col_reason[i];
  std::uint64_t sel = 0; for (unsigned i = 0; i < kPtrClassCount; ++i) sel += k.sel_class[i];
  CHECK(reasons == k.confirmed && sel == k.confirmed && k.confirmed == 1);
  char line[kLogLineMax * 2];
  const int n = fmt_collider_diag(line, sizeof(line), k, "SHUTDOWN");
  CHECK(n > 0 && static_cast<unsigned>(n) < sizeof(line) && std::strstr(line, "POST03_1_COLLIDER_DIAG") && std::strstr(line, "deref_ok_mod8_4=1") && std::strstr(line, "v=0.5.2"));
  // el evento no contiene direcciones: solo clases
  fmt_hit_event(line, sizeof(line), p.rep);
  CHECK(std::strstr(line, "sel_class=mod8_4") && std::strstr(line, "SELECTED_FRESH_MATCH_CC0") && std::strstr(line, "family=SWORD"));
  unsigned run_hex = 0, max_hex = 0;
  for (const char* q = line; *q; ++q) { if (std::strchr("0123456789abcdef", *q)) ++run_hex; else run_hex = 0; if (run_hex > max_hex) max_hex = run_hex; }
  CHECK(max_hex <= 8);
}

int main() {
  c01_c03(); c04_c08(); c09_c10(); c11_c12(); c13_property(); c14_counters();
  std::printf("POST03_1_COLLIDER_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
