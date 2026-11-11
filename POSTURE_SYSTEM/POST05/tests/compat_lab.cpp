// POST05-COMPAT (0.5.1) laboratorio ADVERSARIAL: admision V2 (Blue de grupo AtAtp 2/2, E_OC2 Red/Black/nativo observado), diferimiento de la
// supresion de E_OC2, POWER_CHANGED_IN_CALL, mascara de historial inseguro, contadores por recurso y formato. Motor completo (POST03.1 + POST04 +
// POST05) sobre el MISMO modelo de damage_check que reaction_lab, con un ORACULO independiente para el estres.
// LAB_PASS = consistencia del nucleo con ese modelo. NO demuestra nada del juego real (IA de Expansion, animacion): eso es observacion humana.
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../src/reaction_core.hpp"

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
enum : std::uint32_t { SWORD = 0x4000000u, SHIELD = 0x10u, ARROW = 0x2000u, BOMB = 0x20u, IRON = 0x400000u, BOOM = 0x10000u, OTHER = 0x80000u, UNKT = 0x80000000u };
constexpr AdmissionPolicy V1 = AdmissionPolicy::V1Strict, V2 = AdmissionPolicy::V2Compat;

struct World {
  std::map<std::uintptr_t, std::uint32_t> types;
  std::map<std::uint32_t, std::uintptr_t> present;
  std::map<std::uint32_t, Lookup> forced;  // lookup forzado (p. ej. Creating -> UNVERIFIED)
  int power = 20, derefs = 0, bad_derefs = 0;
  std::uint64_t native_calls = 0, native_processed = 0;
};
struct Fake {
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  ActorFacts f;
  int health = 100000, mode = 4, old_mode = 3, state = 1, cooldown = 0;
  bool cutdown = false;
  bool tg[2] = {false, false};
  std::uintptr_t hit[2] = {0, 0}, sel = 0;
  std::int16_t t6c0 = 0, t6c2 = 0;
  bool head_lock = false;
  std::uint8_t set_bits = 0x0F;
  int cut = 0;
  bool jump_cancel = false, spl = false;
  int atp_in_call = -1;  // >=0: un escritor cambia AtAtp DENTRO de damage_check (modelo de escritor desconocido)
  int unrelated = 12345;
  Fake(std::uint32_t i, std::uintptr_t p, NameKind nk = NameKind::E_OC, int atp = 1) : id(i), ptr(p) {
    f.profile_ok = f.live = true;
    f.name = nk;
    f.status_10000 = nk == NameKind::E_OC;  // create(): solo E_OC lleva 0x10000
    f.atp = atp;
  }
  void hit_with(int s, std::uintptr_t c) { tg[s] = true; hit[s] = c; }
};
static void set_action_mode(Fake& a, int m, int st) { a.old_mode = a.mode; a.mode = m; a.state = st; a.t6c0 = a.t6c2 = 0; a.set_bits |= 0x0F; a.head_lock = false; }
static void off_tg_sph(Fake& a) { a.set_bits &= static_cast<std::uint8_t>(~0x05u); a.tg[0] = a.tg[1] = false; }
static void clear_tg(Fake& a) { a.tg[0] = a.tg[1] = false; a.hit[0] = a.hit[1] = 0; }
static void native(Fake& a, World& w) {  // mismo modelo que reaction_lab (damage_check no depende del recurso salvo la tirachinas: C1)
  ++w.native_calls;
  if (a.atp_in_call >= 0) { a.f.atp = a.atp_in_call; a.atp_in_call = -1; }
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
  a.health -= w.power;
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
static NativeAux aux_of(const Fake& a) {  // replica de read_aux de mod.cpp (incluye atp y recurso de 0.5.1)
  NativeAux x; x.valid = 1; x.t6c0 = a.t6c0; x.t6c2 = a.t6c2; x.head_lock = a.head_lock ? 1 : 0; x.set_bits = a.set_bits;
  x.atp = static_cast<std::int8_t>(a.f.atp); x.resource = static_cast<std::uint8_t>(a.f.name); return x;
}
struct Rest {
  int health, cooldown, unrelated, atp; std::int16_t t6c0, t6c2; bool head_lock, cutdown; std::uint8_t set_bits; std::uintptr_t sel;
  bool operator==(const Rest&) const = default;
};
static Rest rest_of(const Fake& a) { return Rest{a.health, a.cooldown, a.unrelated, a.f.atp, a.t6c0, a.t6c2, a.head_lock, a.cutdown, a.set_bits, a.sel}; }

struct Hx {
  post04::PostureEngine pe;
  post05::Reaction rx;
  World w;
  AdmissionPolicy pol;
  std::vector<post05::DecisionResult> dec;
  std::vector<post05::Apply> app;
  std::vector<HitReport> reps;
  std::vector<std::uint32_t> dec_id;
  std::map<std::uint32_t, int> writes_by;
  std::uint64_t admission_events = 0;
  int writes = 0;
  explicit Hx(AdmissionPolicy p = V2) : pol(p) { pe.eng.masks = M; pe.eng.policy = p; }
  bool reader(std::uintptr_t sel, std::uint32_t& type) {
    ++w.derefs;
    auto it = w.types.find(sel);
    if (it == w.types.end() || !deref_alignment_ok(sel)) { ++w.bad_derefs; return false; }
    type = it->second;
    return true;
  }
  PreReads pre_of(const Fake& a) const {  // replica de do_pre: lecturas de detalle SOLO para candidatos de la politica activa
    PreReads r; r.facts = a.f;
    if (is_candidate(admit(a.f, pol).adm)) { r.snap = snap_of(a); r.raw0 = a.tg[0] ? a.hit[0] : 0; r.raw1 = a.tg[1] ? a.hit[1] : 0; r.aux = aux_of(a); }
    return r;
  }
  static PostReads post_of(const Fake& a) {
    PostReads p; p.alive = a.f.profile_ok && a.f.live;
    if (p.alive) { p.snap = snap_of(a); p.selected = a.sel; p.aux = aux_of(a); }
    return p;
  }
  void count_events(const Events& ev) { for (unsigned i = 0; i < ev.n; ++i) if (ev.e[i].kind == EvKind::AdmissionChanged) ++admission_events; }
  void spawn(Fake& a) { w.present[a.id] = a.ptr; }
  void despawn(Fake& a) { w.present.erase(a.id); }
  bool call(Fake& a) {
    execute_start(a);
    Events ev;
    pe.on_pre(a.ptr, a.id, pre_of(a), ev);
    count_events(ev);
    native(a, w);
    Events ev2; HitReport rep;
    const bool has = pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return reader(s, t); }, ev2, rep);
    count_events(ev2);
    if (has) {
      reps.push_back(rep);
      post05::Apply ap = post05::Apply::NotAttempted;
      const post05::DecisionResult d = post05::process(rx, pe, rep, [&a] { return post05::LiveMode{a.mode, a.state, a.old_mode}; },
                                                       [&](int m, int s, int o) { ++writes; ++writes_by[a.id]; a.mode = m; a.state = s; a.old_mode = o; }, ap);
      dec.push_back(d); app.push_back(ap); dec_id.push_back(a.id);
    }
    post04::PItem it;
    while (pe.px.pop(it)) {}
    return has;
  }
  Lookup lookup(const Pick& p) {
    auto f = w.forced.find(p.id);
    if (f != w.forced.end()) return f->second;
    auto it = w.present.find(p.id);
    if (it == w.present.end()) return Lookup::Absent;
    return it->second == p.ptr ? Lookup::PresentMatch : Lookup::PresentMismatchPtr;
  }
  void update() {
    Observer& o = pe.eng.obs;
    if (o.sweep_due())
      for (unsigned q = 0; q < kSweepQuota; ++q) { Pick p; if (!o.ledger.next_sweep(p, o.sweep_stamp)) break; Events ev; o.ledger.apply_lookup(p, lookup(p), o.epoch.epoch, ev); count_events(ev); }
    pe.on_update();
    post04::PItem it;
    while (pe.px.pop(it)) {}
  }
  void ticks(unsigned n) { for (unsigned i = 0; i < n; ++i) update(); }
  int slot_of(std::uint32_t id) const {
    for (unsigned i = 0; i < kCapacity; ++i) { const Instance& s = pe.eng.obs.ledger.slot(i); if (s.id == id && (s.life == Life::Active || s.life == Life::DeathSeen || s.life == Life::Unverified)) return static_cast<int>(i); }
    return -1;
  }
  const post04::Entry* entry(std::uint32_t id) const { const int s = slot_of(id); return s < 0 ? nullptr : &pe.px.entry(static_cast<unsigned>(s)); }
  int posture(std::uint32_t id) const { const post04::Entry* e = entry(id); return e && e->bound ? e->posture : -1; }
  post05::Reason last_reason() const { return dec.empty() ? post05::Reason::Count : dec.back().reason; }
  post05::Apply last_apply() const { return app.empty() ? post05::Apply::NotAttempted : app.back(); }
};
static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }
static void types(Hx& h) {
  h.w.types[P(1)] = SWORD; h.w.types[P(2)] = SWORD; h.w.types[P(3)] = IRON; h.w.types[P(4)] = BOOM; h.w.types[P(5)] = ARROW; h.w.types[P(6)] = OTHER;
  h.w.types[P(7)] = SWORD | ARROW; h.w.types[P(8)] = BOMB; h.w.types[P(9)] = SHIELD; h.w.types[P(10)] = SWORD | UNKT;
}
static bool strike(Hx& h, Fake& a, std::uintptr_t c, int pre_mode = 4, int pre_state = 1, int sphere = 1) {
  while (a.cooldown > 0 && a.health > 0) h.call(a);
  a.mode = pre_mode; a.state = pre_state;
  a.hit_with(sphere, c);
  const bool has = h.call(a);
  clear_tg(a);
  return has;
}
static void mask_invariant(const Hx& h) {
  for (unsigned i = 0; i < kCapacity; ++i) { const post04::Entry& e = h.pe.px.entry(i); if (e.bound) CHECK(e.ever_frozen == (e.unsafe_mask != 0)); }
}
static void audit_all(Hx& h) {
  CHECK(h.rx.audit().ok);
  CHECK(h.pe.px.audit().ok);
  CHECK(h.pe.eng.conserved());
  mask_invariant(h);
}
// Regla de 0.5.0 re-escrita a mano (ORACULO V1) y regla V2 documentada en 05_VARIANT_IDENTITY_AND_TRANSITIONS.md
static Admission oracle_admit(const ActorFacts& f, AdmissionPolicy p) {
  if (!f.profile_ok) return Admission::PROFILE_OTHER;
  if (!f.live || f.name == NameKind::Unread || f.name == NameKind::Other) return Admission::UNKNOWN_UNSAFE;
  if (f.name == NameKind::E_OC2) {
    if (p == V1) return Admission::E_OC2_EXCLUDED;
    return (!f.status_10000 && (f.atp == 1 || f.atp == 2 || f.atp == 3 || f.atp == 6)) ? Admission::E_OC2_CANDIDATE : Admission::UNKNOWN_UNSAFE;
  }
  if (!f.status_10000) return Admission::UNKNOWN_UNSAFE;
  if (f.atp == 1) return Admission::E_OC_CANDIDATE;
  if (f.atp == 2 && p == V2) return Admission::E_OC_CANDIDATE;
  return Admission::UNKNOWN_UNSAFE;
}

// ---- T01-T12: identidad (C0) ---------------------------------------------------------------------------------------------------------
static void t01_t12_identity() {
  auto F = [](NameKind n, bool st, int atp, bool prof = true, bool live = true) { ActorFacts f; f.profile_ok = prof; f.live = live; f.name = n; f.status_10000 = st; f.atp = atp; return f; };
  // T01 E_OC 1/1 / T02 E_OC 2/2 (Blue de grupo)
  CHECK(admit(F(NameKind::E_OC, true, 1), V2).adm == Admission::E_OC_CANDIDATE);
  CHECK(admit(F(NameKind::E_OC, true, 2), V2).adm == Admission::E_OC_CANDIDATE && admit(F(NameKind::E_OC, true, 2), V1).adm == Admission::UNKNOWN_UNSAFE);
  // T03 E_OC fuera de {1,2}
  for (int a : {0, 3, 4, 5, 6, 7, 100, 255}) CHECK(admit(F(NameKind::E_OC, true, a), V2).adm == Admission::UNKNOWN_UNSAFE && !std::strcmp(admit(F(NameKind::E_OC, true, a), V2).reason, "E_OC_ATP_OUTSIDE_SET"));
  // T04 esferas en desacuerdo / T05 sin 0x10000
  CHECK(admit(F(NameKind::E_OC, true, -2), V2).adm == Admission::UNKNOWN_UNSAFE && !std::strcmp(admit(F(NameKind::E_OC, true, -2), V2).reason, "E_OC_ATP_SPHERES_DISAGREE"));
  CHECK(admit(F(NameKind::E_OC, false, 1), V2).adm == Admission::UNKNOWN_UNSAFE && admit(F(NameKind::E_OC, false, 2), V2).adm == Admission::UNKNOWN_UNSAFE);
  // T06 E_OC2 {1,2,3,6} sin 0x10000 / T07 fuera de lista, desacuerdo o 0x10000
  for (int a : {1, 2, 3, 6}) CHECK(admit(F(NameKind::E_OC2, false, a), V2).adm == Admission::E_OC2_CANDIDATE);
  for (int a : {-1, 0, 4, 5, 7, 12, 255}) CHECK(admit(F(NameKind::E_OC2, false, a), V2).adm == Admission::UNKNOWN_UNSAFE);
  CHECK(!std::strcmp(admit(F(NameKind::E_OC2, false, -2), V2).reason, "E_OC2_ATP_SPHERES_DISAGREE"));
  for (int a : {1, 2, 3, 6}) CHECK(admit(F(NameKind::E_OC2, true, a), V2).adm == Admission::UNKNOWN_UNSAFE);
  // T08 perfil/vida/nombre igual que V1
  CHECK(admit(F(NameKind::E_OC, true, 1, false), V2).adm == Admission::PROFILE_OTHER && admit(F(NameKind::E_OC, true, 1, true, false), V2).adm == Admission::UNKNOWN_UNSAFE);
  CHECK(admit(F(NameKind::Unread, true, 1), V2).adm == Admission::UNKNOWN_UNSAFE && admit(F(NameKind::Other, false, 2), V2).adm == Admission::UNKNOWN_UNSAFE);
  // T09/T10 rejilla EXHAUSTIVA contra el oraculo: V1 identico a 0.5.0; V2 solo cambia en las celdas documentadas
  unsigned diff_cells = 0;
  for (int prof = 0; prof < 2; ++prof)
    for (int live = 0; live < 2; ++live)
      for (NameKind n : {NameKind::Unread, NameKind::E_OC, NameKind::E_OC2, NameKind::Other})
        for (int st = 0; st < 2; ++st)
          for (int atp = -2; atp <= 12; ++atp) {
            const ActorFacts f = F(n, st != 0, atp, prof != 0, live != 0);
            CHECK(admit(f).adm == oracle_admit(f, V1));          // por defecto = V1 (los laboratorios heredados no cambian)
            CHECK(admit(f, V1).adm == oracle_admit(f, V1));
            CHECK(admit(f, V2).adm == oracle_admit(f, V2));
            if (admit(f, V1).adm != admit(f, V2).adm) {
              ++diff_cells;
              CHECK(prof && live && ((n == NameKind::E_OC && st && atp == 2) || n == NameKind::E_OC2));
            }
          }
  CHECK(diff_cells == 1 + 2 * 15);  // E_OC 2/2 con 0x10000, y TODA celda E_OC2 viva (excluida en V1; candidata o insegura en V2)
  // T11 is_candidate / T12 nombres
  CHECK(is_candidate(Admission::E_OC_CANDIDATE) && is_candidate(Admission::E_OC2_CANDIDATE) && !is_candidate(Admission::E_OC2_EXCLUDED) &&
        !is_candidate(Admission::PROFILE_OTHER) && !is_candidate(Admission::UNKNOWN_UNSAFE));
  CHECK(!std::strcmp(name(Admission::E_OC2_CANDIDATE), "E_OC2_CANDIDATE") && !std::strcmp(name(V2), "V2_COMPAT") && !std::strcmp(name(V1), "V1_STRICT"));
}

// ---- T13-T16: transiciones (C2) -------------------------------------------------------------------------------------------------------
static void t13_t14_blue_group(AdmissionPolicy p) {
  Hx h(p); types(h);
  Fake a(222, 0x9000);  // Blue nativo con Expansion: E_OC, 0x10000
  h.spawn(a);
  for (int i = 0; i < 5; ++i) h.call(a);             // primera observacion con atp 1
  a.f.atp = 2; for (int i = 0; i < 3; ++i) h.call(a); // BLUE_GROUP_ON (escrito por Expansion ENTRE llamadas)
  a.f.atp = 1; for (int i = 0; i < 3; ++i) h.call(a); // BLUE_GROUP_OFF
  a.f.atp = 2;                                        // (a) grupo otra vez y golpe en ATTACK 1 CON potencia de grupo 2/2
  const bool has_a = strike(h, a, P(1), 4, 1);
  if (p == V2) {
    CHECK(has_a && h.admission_events == 0);                                // T13: sin cambios de admision
    CHECK(h.last_reason() == post05::Reason::Eligible && h.last_apply() == post05::Apply::Applied && a.mode == 4 && a.state == 1);
    CHECK(h.entry(222) && !h.entry(222)->ever_frozen && h.entry(222)->unsafe_mask == 0 && h.posture(222) == 25);
    CHECK(h.dec.back().adm == Admission::E_OC_CANDIDATE && h.reps.back().aux_pre.atp == 2);
  } else {
    CHECK(!has_a && h.dec.empty() && h.admission_events == 3);            // T14: V1 ni siquiera observa el golpe de un Blue en grupo
    CHECK(h.writes == 0 && a.mode == 5);
  }
  a.f.atp = 1; h.call(a);                             // (b) fin del grupo y golpe en ATTACK 1 con 1/1: el caso EXACTO de B1 evento 7
  CHECK(strike(h, a, P(1), 4, 1));
  if (p == V2) {
    CHECK(h.last_reason() == post05::Reason::Eligible && h.writes == 2 && h.admission_events == 0 && h.posture(222) == 50);
  } else {
    CHECK(h.last_reason() == post05::Reason::AdmissionEverUnstable && h.writes == 0 && a.mode == 5 && h.admission_events == 4);
    CHECK(h.entry(222)->unsafe_mask == post04::kUnsafeAdmission);
  }
  audit_all(h);
}
static void t15_disagreement_marks_unsafe() {
  Hx h; types(h);
  Fake a(30, 0xA000); h.spawn(a);
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::Eligible);
  a.f.atp = -2; h.call(a); a.f.atp = 1;              // esferas en desacuerdo un instante: UNKNOWN_UNSAFE
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::AdmissionEverUnstable && h.writes == 1 && a.mode == 5);
  CHECK(h.entry(30)->unsafe_mask == post04::kUnsafeAdmission && h.entry(30)->ever_frozen);
  audit_all(h);
}
static void t16_lookup_unverified_mask() {
  Hx h; types(h);
  Fake a(31, 0xA400); h.spawn(a);
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::Eligible);
  h.w.forced[31] = Lookup::Creating; h.ticks(400);
  CHECK(h.pe.eng.obs.ledger.slot(static_cast<unsigned>(h.slot_of(31))).life == Life::Unverified);
  h.w.forced.erase(31); h.ticks(400);
  CHECK(h.pe.eng.obs.ledger.slot(static_cast<unsigned>(h.slot_of(31))).life == Life::Active);
  CHECK(h.entry(31)->unsafe_mask == post04::kUnsafeLookup);
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::AdmissionEverUnstable && h.writes == 1);
  audit_all(h);
}

// ---- T17-T24: E_OC2 (C3/C4) ----------------------------------------------------------------------------------------------------------
static void t17_red_deferred() {
  Hx h; types(h);
  Fake r(172, 0xB000, NameKind::E_OC2, 2); h.spawn(r);
  int expect_p = 0;
  for (int st = 0; st < 3; ++st) {
    const Fake pre = r;
    CHECK(strike(h, r, P(1), 4, st));
    expect_p += 25;
    CHECK(h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && h.dec.back().decision == post05::Decision::AllowVanilla);
    CHECK(r.mode == 5 && r.old_mode == 4 && h.writes == 0);           // reaccion NATIVA intacta
    CHECK(h.posture(172) == expect_p && h.dec.back().adm == Admission::E_OC2_CANDIDATE);
    CHECK(rest_of(r).atp == pre.f.atp);
  }
  CHECK(h.rx.c.by_resource[1] == 3 && h.rx.c.by_resource[0] == 0);
  audit_all(h);
}
static void t18_black_fury_between_calls() {
  Hx h; types(h);
  Fake b(218, 0xB400, NameKind::E_OC2, 3); h.spawn(b);
  CHECK(strike(h, b, P(1), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred);
  b.f.atp = 6;                                       // BLACK_FURY_START en el POST de execute de Expansion (despues de damage_check)
  for (int i = 0; i < 3; ++i) { CHECK(strike(h, b, P(1), 4, 2) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred); }
  b.f.atp = 3;                                       // BLACK_FURY_END
  CHECK(strike(h, b, P(1), 4, 0) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred);
  CHECK(h.admission_events == 0 && h.writes == 0 && h.posture(218) == 100 - 0 && !h.entry(218)->ever_frozen);   // 4.o golpe cruza 100 (latch)
  audit_all(h);
}
static void t19_power_change_in_call() {
  {  // Blue: 1 -> 2 DENTRO de la llamada: escritor desconocido -> SKIP_UNSAFE, sin escritura
    Hx h; types(h);
    Fake a(40, 0xC000); h.spawn(a);
    a.atp_in_call = 2;
    CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::PowerChangedInCall && h.writes == 0 && a.mode == 5);
    CHECK(h.dec.back().decision == post05::Decision::SkipUnsafe);
    CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::Eligible);   // sin cambio en la llamada siguiente: vuelve a ser elegible
    audit_all(h);
  }
  {  // Black: 3 -> 6 dentro de la llamada: la politica de E_OC2 manda antes (vanilla)
    Hx h; types(h);
    Fake b(41, 0xC400, NameKind::E_OC2, 3); h.spawn(b);
    b.atp_in_call = 6;
    CHECK(strike(h, b, P(1), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && h.writes == 0);
    audit_all(h);
  }
}
static void t20_e_oc2_v1_excluded() {
  Hx h(V1); types(h);
  Fake r(50, 0xD000, NameKind::E_OC2, 2); h.spawn(r);
  CHECK(!strike(h, r, P(1), 4, 1));                  // V1: ni detalle ni evento ni postura
  CHECK(h.dec.empty() && h.posture(50) == -1 && r.mode == 5 && h.pe.eng.obs.ledger.c.calls_adm[static_cast<unsigned>(Admission::E_OC2_EXCLUDED)] > 0);
  audit_all(h);
}
static void t21_resource_signal_any_source() {  // el diferimiento no depende solo de la admision del ledger
  Hx h; types(h);
  Fake a(60, 0xD400); h.spawn(a);
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::Eligible);
  post05::DecisionInput in;
  in.rep = h.reps.back(); in.outc = h.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC_CANDIDATE; in.life = Life::Active;
  in.rep.event += 1000; in.outc.event = in.rep.event;
  in.rep.aux_post.resource = static_cast<std::uint8_t>(NameKind::E_OC2);
  CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
  in.rep.event += 1; in.outc.event = in.rep.event; in.rep.aux_post.resource = in.rep.aux_pre.resource;
  in.rep.aux_pre.resource = static_cast<std::uint8_t>(NameKind::E_OC2);
  CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
  in.rep.event += 1; in.outc.event = in.rep.event; in.rep.aux_pre.resource = in.rep.aux_post.resource; in.adm = Admission::E_OC2_EXCLUDED;
  CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
  in.rep.event += 1; in.outc.event = in.rep.event; in.adm = Admission::E_OC_CANDIDATE;
  CHECK(h.rx.decide(in).reason == post05::Reason::Eligible);   // control: sin senal de E_OC2 vuelve a ser elegible
}
static void t22_e_oc2_lethal_and_other_families() {
  Hx h; types(h);
  Fake r(70, 0xE000, NameKind::E_OC2, 2); r.health = 21; h.spawn(r);
  CHECK(strike(h, r, P(1), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && r.health == 1 && r.mode == 6 && h.writes == 0);
  Fake s(71, 0xE400, NameKind::E_OC2, 6); h.spawn(s);
  for (unsigned c : {3u, 4u, 5u, 9u}) CHECK(strike(h, s, P(c), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && h.writes == 0);
  audit_all(h);
}
static void t23_mixed_population_counters() {
  Hx h; types(h);
  Fake blue(80, 0xF000), red(81, 0xF400, NameKind::E_OC2, 2), black(82, 0xF800, NameKind::E_OC2, 3), group(83, 0xFC00, NameKind::E_OC, 2);
  for (Fake* f : {&blue, &red, &black, &group}) h.spawn(*f);
  int n = 0;
  for (int k = 0; k < 2; ++k)
    for (Fake* f : {&blue, &red, &black, &group}) { CHECK(strike(h, *f, P(1), 4, k)); ++n; }
  CHECK(h.rx.c.by_resource[0] == 4 && h.rx.c.by_resource[1] == 4 && h.rx.c.by_resource[2] == 0 && h.rx.c.decisions == static_cast<std::uint64_t>(n));
  CHECK(h.writes_by[80] == 2 && h.writes_by[83] == 2 && h.writes_by[81] == 0 && h.writes_by[82] == 0);
  CHECK(h.rx.audit().r6);
  post05::Reaction bad = h.rx; ++bad.c.by_resource[2];   // R6 detecta un contador por recurso manipulado
  CHECK(!bad.audit().r6 && !bad.audit().ok);
  audit_all(h);
}
static void t24_devf_blue_on_e_oc2() {
  Hx h; types(h);
  Fake d(90, 0x11000, NameKind::E_OC2, 1); h.spawn(d);   // Blue DEVF registrado sobre recurso E_OC2 (1/1): candidato E_OC2, diferido
  CHECK(strike(h, d, P(1), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && h.writes == 0);
  d.f.atp = 2; CHECK(strike(h, d, P(1), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && h.admission_events == 0);
  audit_all(h);
}

// ---- T25-T27: contabilidad y formato ---------------------------------------------------------------------------------------------------
static void t25_t27_counters_and_format() {
  Hx h; types(h);
  Fake a(100, 0x12000), r(101, 0x12400, NameKind::E_OC2, 2);
  h.spawn(a); h.spawn(r);
  CHECK(strike(h, a, P(1), 4, 1) && strike(h, r, P(1), 4, 1));
  const Counters& k = h.pe.eng.obs.ledger.c;
  CHECK(k.calls_adm[static_cast<unsigned>(Admission::E_OC2_CANDIDATE)] > 0 && k.calls_adm[static_cast<unsigned>(Admission::E_OC2_EXCLUDED)] == 0);
  char line[kLogLineMax * 4];
  CHECK(fmt_summary(line, sizeof(line), k, 2, 1, 0, "SHUTDOWN") > 0 && std::strstr(line, "adm(cand/e_oc2/other/unsafe/e_oc2_cand)=") && std::strstr(line, "v=0.6.0"));
  // linea de decision de E_OC2: campos 0.5.1 y nada mas (lista blanca del extractor DEC_051)
  char d[kLogLineMax];
  CHECK(post05::fmt_decision(d, sizeof(d), h.reps.back(), h.pe.last, h.dec.back(), h.app.back()) > 0);
  CHECK(std::strstr(d, " resource=E_OC2 atp=2>2 admission=E_OC2_CANDIDATE") && std::strstr(d, "reason=E_OC2_SUPPRESSION_DEFERRED"));
  std::set<std::string> keys;
  for (const char* p = std::strchr(d, ' '); p; p = std::strchr(p + 1, ' ')) { const char* e = std::strchr(p + 1, '='); const char* sp = std::strchr(p + 1, ' '); if (e && (!sp || e < sp)) keys.insert(std::string(p + 1, e)); }
  const std::set<std::string> want = {"id", "generation", "event", "family", "decision", "reason", "pre_mode", "pre_state", "post_mode", "post_state", "cd", "posture", "action", "resource", "atp", "admission", "phase", "frame", "rate", "speed", "env", "compat", "plan"};
  CHECK(keys == want);
  CHECK(post05::fmt_decision(d, sizeof(d), h.reps.front(), h.pe.last, h.dec.front(), h.app.front()) > 0 && std::strstr(d, " resource=E_OC atp=1>1 admission=E_OC_CANDIDATE"));
  // peor caso de longitud: cabe en el buffer de mod.cpp (kLogLineMax) sin truncar
  HitReport big = h.reps.back(); big.id = UINT_MAX; big.generation = UINT64_MAX; big.event = UINT64_MAX; big.pre.mode = big.pre.state = big.post.mode = big.post.state = INT_MIN;
  big.pre.cooldown = big.post.cooldown = INT_MIN; big.aux_pre.atp = big.aux_post.atp = -128; big.fam.family = Family::Ambiguous;
  big.aux_pre.anim_frame = -1e30f; big.aux_pre.anim_rate = -1e30f; big.aux_pre.speed = -1e30f;  // 0.5.2: campos de fase acotados a -999
  post04::ConsumeOutcome bo; bo.before = bo.after = INT_MIN;
  post05::DecisionResult bd = h.dec.back(); bd.reason = post05::Reason::E_OC2SuppressionDeferred; bd.phase = post05::E2Phase::AnimUnverified;
  const int n = post05::fmt_decision(d, sizeof(d), big, bo, bd, post05::Apply::RecheckFailed);
  CHECK(n > 0 && n < static_cast<int>(kLogLineMax));
  post05::Reaction maxr = h.rx;
  for (auto& v : maxr.c.by_reason) v = UINT64_MAX;
  for (auto& v : maxr.c.by_resource) v = UINT64_MAX;
  maxr.c.decisions = maxr.c.duplicates = maxr.c.confirmed = maxr.c.attempted = maxr.c.applied = maxr.c.log_written = maxr.c.log_dropped = UINT64_MAX;
  const int ns = post05::fmt_summary(line, sizeof(line), maxr, "SHUTDOWN");
  CHECK(ns > 0 && ns < static_cast<int>(sizeof(line)) && std::strstr(line, "resource(e_oc/e_oc2/other)="));
  CHECK(post05::fmt_conservation(line, sizeof(line), h.rx, "SHUTDOWN") > 0 && std::strstr(line, "ok=1") && std::strstr(line, "r6_resources=1"));
  for (const char* s = line; *s; ++s) CHECK(!(s[0] == '0' && s[1] == 'x'));
}

// ---- T28-T29: gemelos (C5: unico escritor, solo 3 campos) ------------------------------------------------------------------------------
static void t28_t29_twins() {
  for (NameKind nk : {NameKind::E_OC, NameKind::E_OC2}) {
    for (int atp : {1, 2}) {
      Hx on; types(on);
      Hx off; types(off); off.rx.close_session();   // gemelo: POST05 nunca escribe
      Fake a(110, 0x13000, nk, atp), b(110, 0x13000, nk, atp);
      on.spawn(a); off.spawn(b);
      for (int st = 0; st < 3; ++st) {
        CHECK(strike(on, a, P(1), 4, st) && strike(off, b, P(1), 4, st));
        CHECK(rest_of(a) == rest_of(b));                                    // HP, cooldown, timers, flags, bits, AtAtp: identicos
        if (nk == NameKind::E_OC) CHECK(a.mode == 4 && a.state == st && b.mode == 5);
        else CHECK(a.mode == b.mode && a.state == b.state && a.old_mode == b.old_mode);   // E_OC2: identico en TODO
        CHECK(on.posture(110) == off.posture(110));
      }
      CHECK(on.writes == (nk == NameKind::E_OC ? 3 : 0) && off.writes == 0);
      CHECK(on.pe.px.c.weighted == off.pe.px.c.weighted && on.pe.eng.obs.ledger.c.confirmed == off.pe.eng.obs.ledger.c.confirmed && on.w.native_calls == off.w.native_calls);
    }
  }
}

// ---- T30: estres con oraculo independiente ----------------------------------------------------------------------------------------
struct Rng { std::uint64_t s; std::uint64_t next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; } unsigned below(unsigned n) { return static_cast<unsigned>(next() % n); } };
struct Orc { bool unstable = false, latched = false, dead = false; int p = 0; };
static unsigned long long g_cmp = 0, g_applied = 0, g_applied_atp2 = 0, g_deferred = 0, g_deferred_shaped = 0, g_power = 0, g_unsafe = 0, g_vanilla = 0;
static void stress(std::uint64_t seed, unsigned frames) {
  Hx h; types(h);
  Rng rng{seed};
  std::vector<std::unique_ptr<Fake>> all;
  std::vector<Fake*> live;
  std::map<Fake*, Orc> orc;
  std::uint32_t next_id = 3000;
  std::uintptr_t next_ptr = 0x900000;
  const std::uintptr_t cols[] = {P(1), P(1), P(1), P(1), P(2), P(3), P(4), P(5), P(6), P(7), P(9), P(10)};
  unsigned mism = 0;
  for (unsigned fr = 0; fr < frames; ++fr) {
    if (live.size() < 8 && rng.below(10) == 0) {
      const unsigned kind = rng.below(4);  // 0 Blue/nativo E_OC, 1 Red E_OC2, 2 Black E_OC2, 3 Blue de grupo
      const NameKind nk = (kind == 1 || kind == 2) ? NameKind::E_OC2 : NameKind::E_OC;
      const int atp = kind == 2 ? 3 : (kind == 1 || kind == 3) ? 2 : 1;
      all.push_back(std::make_unique<Fake>(next_id++, next_ptr, nk, atp)); next_ptr += 0x400;
      Fake* f = all.back().get(); live.push_back(f); h.spawn(*f); orc[f] = Orc{};
    }
    if (!live.empty() && rng.below(120) == 0) { const unsigned i = rng.below(static_cast<unsigned>(live.size())); h.despawn(*live[i]); live.erase(live.begin() + i); }
    // escrituras de Expansion ENTRE llamadas (grupo Blue 1<->2, furia Black 3<->6) y, rara vez, un desacuerdo transitorio
    if (!live.empty() && rng.below(40) == 0) {
      Fake* f = live[rng.below(static_cast<unsigned>(live.size()))];
      if (f->f.name == NameKind::E_OC) f->f.atp = f->f.atp == 1 ? 2 : 1;
      else if (f->f.atp == 3 || f->f.atp == 6) f->f.atp = f->f.atp == 3 ? 6 : 3;
    }
    h.update();
    for (Fake* f : live) {
      Orc& o = orc[f];
      const bool disagree = rng.below(400) == 0;
      const int keep_atp = f->f.atp;
      if (disagree) f->f.atp = -2;
      f->mode = rng.below(2) ? 4 : static_cast<int>(rng.below(9)); f->state = static_cast<int>(rng.below(5));
      f->t6c0 = rng.below(15) == 0 ? 5 : 0; f->t6c2 = rng.below(15) == 0 ? 3 : 0; f->head_lock = rng.below(15) == 0;
      f->set_bits = rng.below(15) == 0 ? 0x0B : 0x0F; f->cut = rng.below(10) == 0 ? 6 : static_cast<int>(rng.below(3));
      f->jump_cancel = rng.below(20) == 0; f->spl = rng.below(25) == 0;
      if (f->health < 50 && !o.dead) f->health = 100000;
      if (!disagree && rng.below(12) == 0) f->atp_in_call = f->f.name == NameKind::E_OC ? (f->f.atp == 1 ? 2 : 1) : (f->f.atp == 3 ? 6 : f->f.atp == 6 ? 3 : 2);
      const Fake pre = *f;
      const bool striking = !o.dead && f->cooldown == 0 && rng.below(3) == 0;
      if (striking) f->hit_with(static_cast<int>(rng.below(2)), cols[rng.below(sizeof(cols) / sizeof(cols[0]))]);
      const std::uint64_t pr0 = h.w.native_processed;
      const std::size_t nd = h.dec.size();
      const int w0 = h.writes;
      // postura REAL antes de la llamada: la regeneracion la verifica posture_lab; aqui se comprueba el incremento de ESTE golpe
      const post04::Entry* e0 = h.entry(f->id);
      const int p_before = e0 && e0->bound ? e0->posture : 0;
      const bool latched_before = e0 && e0->bound && e0->latched;
      h.call(*f);
      clear_tg(*f);
      if (disagree) f->f.atp = keep_atp;
      const bool candidate = is_candidate(oracle_admit(pre.f, V2));
      if (!candidate) o.unstable = true;
      if (!(h.w.native_processed > pr0) || h.dec.size() == nd) { if (h.writes != w0) ++mism; continue; }
      // ---- oraculo ----
      const std::uint32_t t = h.w.types.at(f->sel);
      const bool sword_only = (t & M.sword) && !(t & (M.shield | M.arrow | M.bomb | M.iron_ball | M.boomerang | M.at40 | M.slingshot));
      const bool lethal = f->health <= 0;
      const unsigned groups = ((t & M.sword) ? 1u : 0u) + ((t & M.shield) ? 1u : 0u) + ((t & M.arrow) ? 1u : 0u) + ((t & M.bomb) ? 1u : 0u) + ((t & M.iron_ball) ? 1u : 0u) +
                              ((t & M.boomerang) ? 1u : 0u) + ((t & M.at40) ? 1u : 0u) + ((t & M.slingshot) ? 1u : 0u);
      int w = 0;
      if (!lethal && !latched_before && !o.dead && groups == 1) w = (t & M.sword) ? 25 : ((t & M.iron_ball) ? 50 : 0);
      if (!candidate) w = 0;
      const int before = p_before, after = std::min(100, before + w);
      const bool ordinary = !pre.spl && !pre.jump_cancel && pre.cut <= 2 && !(t & 0xD8000000u) && (pre.health - h.w.power) > 1;
      const bool shaped = candidate && !o.unstable && sword_only && !lethal && ordinary && w > 0 && after < 100 && pre.mode == 4 && pre.state <= 2 &&
                          pre.t6c0 == 0 && pre.t6c2 == 0 && !pre.head_lock && pre.set_bits == 0x0F && pre.cooldown == 0;
      const bool e_oc2 = pre.f.name == NameKind::E_OC2;
      const bool power_changed = pre.atp_in_call >= 0 && pre.atp_in_call != pre.f.atp;
      const bool eligible = shaped && !e_oc2 && !power_changed;
      o.p = after;
      if (before < 100 && after >= 100) o.latched = true;
      const post05::DecisionResult& d = h.dec.back();
      const bool got = d.decision == post05::Decision::SuppressEligible;
      ++g_checks; ++g_cmp;
      bool ok = got == eligible && h.posture(f->id) == o.p;
      if (e_oc2) ok = ok && d.reason == post05::Reason::E_OC2SuppressionDeferred && h.writes == w0;   // E_OC2: nunca escrito (la reaccion es la del modelo nativo)
      if (shaped && !e_oc2 && power_changed) ok = ok && d.reason == post05::Reason::PowerChangedInCall && h.writes == w0;
      if (got) ok = ok && f->mode == pre.mode && f->state == pre.state && f->old_mode == pre.old_mode && h.writes == w0 + 1;
      else ok = ok && h.writes == w0;
      if (!ok) {
        ++mism;
        if (mism < 6) std::printf("MISMATCH seed=%llu fr=%u id=%u e_oc2=%d got=%s want=%d reason=%s p=%d/%d\n", static_cast<unsigned long long>(seed), fr, f->id, e_oc2,
                                  post05::name(d.decision), eligible, post05::name(d.reason), h.posture(f->id), o.p);
      }
      if (lethal) { o.dead = true; f->cooldown = 999; }
      if (got) { ++g_applied; if (pre.f.atp == 2) ++g_applied_atp2; }
      else if (d.reason == post05::Reason::E_OC2SuppressionDeferred) { ++g_deferred; if (shaped) ++g_deferred_shaped; }
      else if (d.reason == post05::Reason::PowerChangedInCall) ++g_power;
      else if (d.decision == post05::Decision::SkipUnsafe) ++g_unsafe;
      else ++g_vanilla;
    }
    if (fr % 1000 == 999) audit_all(h);
  }
  CHECK(mism == 0);
  CHECK(h.rx.c.verify_failed == 0 && h.rx.c.recheck_failed == 0 && h.w.bad_derefs == 0);
  for (auto& [id, n] : h.writes_by) { auto it = std::find_if(all.begin(), all.end(), [&](const std::unique_ptr<Fake>& x) { return x->id == id; }); CHECK(it != all.end() && ((*it)->f.name == NameKind::E_OC || n == 0)); }
  audit_all(h);
}

// ---- T31-T32: repeticion de los patrones reales de B1 ---------------------------------------------------------------------------------
static void t31_t32_b1_patterns() {
  // B1 [R]: 221/3 x4, 222/4 x4 y 161/8 x2 cambios de admision (= BLUE_GROUP_ON/OFF) con 0.5.0; con V2 ninguno
  for (AdmissionPolicy p : {V1, V2}) {
    Hx h(p); types(h);
    Fake b221(221, 0x20000), b222(222, 0x20400), b161(161, 0x20800);
    for (Fake* f : {&b221, &b222, &b161}) { h.spawn(*f); h.call(*f); }
    for (int k = 0; k < 2; ++k) for (Fake* f : {&b221, &b222}) { f->f.atp = 2; h.call(*f); f->f.atp = 1; h.call(*f); }
    b161.f.atp = 2; h.call(b161); b161.f.atp = 1; h.call(b161);
    CHECK(h.admission_events == (p == V1 ? 10u : 0u));
    // evento 7 de B1: 222 en ATTACK 1, espada, primer golpe (postura 0->25)
    CHECK(strike(h, b222, P(1), 4, 1));
    CHECK(h.last_reason() == (p == V1 ? post05::Reason::AdmissionEverUnstable : post05::Reason::Eligible));
    audit_all(h);
  }
}

// ---- T33-T36: regresion y casos limite ------------------------------------------------------------------------------------------------
static void t33_native_e_oc_identical_v1_v2() {  // E_OC nativo con atp 1 fijo: V1 y V2 deciden EXACTAMENTE igual
  Hx a(V1), b(V2); types(a); types(b);
  Fake x(120, 0x21000), y(120, 0x21000);
  a.spawn(x); b.spawn(y);
  Rng r1{77}, r2{77};
  for (int i = 0; i < 400; ++i) {
    const int m1 = r1.below(2) ? 4 : static_cast<int>(r1.below(9)), s1 = static_cast<int>(r1.below(4)); const unsigned c1 = 1 + r1.below(10);
    const int m2 = r2.below(2) ? 4 : static_cast<int>(r2.below(9)), s2 = static_cast<int>(r2.below(4)); const unsigned c2 = 1 + r2.below(10);
    strike(a, x, P(c1), m1, s1); strike(b, y, P(c2), m2, s2);
    if (x.health < 100) x.health = y.health = 100000;
  }
  CHECK(a.dec.size() == b.dec.size() && a.writes == b.writes && a.dec.size() > 100);
  bool same = true;
  for (std::size_t i = 0; i < a.dec.size() && i < b.dec.size(); ++i) same = same && a.dec[i].reason == b.dec[i].reason;
  CHECK(same);
}
static void t34_group_posture_v1_vs_v2() {
  Hx a(V1), b(V2); types(a); types(b);
  Fake x(130, 0x22000, NameKind::E_OC, 2), y(130, 0x22000, NameKind::E_OC, 2);
  a.spawn(x); b.spawn(y);
  strike(a, x, P(1), 3, 0); strike(b, y, P(1), 3, 0);
  CHECK(a.posture(130) == -1 && b.posture(130) == 25);   // V1: un Blue que NACE en grupo nunca recibe postura; V2: si
}
static void t35_duplicate_token_e_oc2() {
  Hx h; types(h);
  Fake r(140, 0x23000, NameKind::E_OC2, 2); h.spawn(r);
  CHECK(strike(h, r, P(1), 4, 1));
  post05::DecisionInput in; in.rep = h.reps.back(); in.outc = h.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC2_CANDIDATE; in.life = Life::Active;
  const std::uint64_t n0 = h.rx.c.decisions;
  CHECK(!h.rx.decide(in).counted && h.rx.c.decisions == n0 && h.rx.c.duplicates == 1);
}
static void t36_session_closed_keeps_e_oc2_reason() {
  Hx h; types(h); h.rx.close_session();
  Fake r(150, 0x24000, NameKind::E_OC2, 2), a(151, 0x24400); h.spawn(r); h.spawn(a);
  CHECK(strike(h, r, P(1), 4, 1) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred);
  CHECK(strike(h, a, P(1), 4, 1) && h.last_reason() == post05::Reason::SessionDisabled && h.writes == 0);
}

// ---- T37-T40: tablas y versiones ----------------------------------------------------------------------------------------------------
static void t37_t40_tables() {
  CHECK(post05::decision_of(post05::Reason::E_OC2SuppressionDeferred) == post05::Decision::AllowVanilla);
  CHECK(post05::decision_of(post05::Reason::PowerChangedInCall) == post05::Decision::SkipUnsafe);
  CHECK(post05::decision_of(post05::Reason::PostAuxUnexpected) == post05::Decision::SkipUnsafe && post05::decision_of(post05::Reason::Eligible) == post05::Decision::SuppressEligible);
  CHECK(post05::kReasonCount == 39);  // 0.5.2: +5 motivos de fase E_OC2
  std::set<std::string> names;
  for (unsigned i = 0; i < post05::kReasonCount; ++i) { const char* s = post05::name(static_cast<post05::Reason>(i)); CHECK(std::strcmp(s, "NONE") != 0); names.insert(s); }
  CHECK(names.size() == post05::kReasonCount && names.count("E_OC2_SUPPRESSION_DEFERRED") && names.count("POWER_CHANGED_IN_CALL"));
  CHECK(!std::strcmp(kModVersion, "0.12.5") && !std::strcmp(post04::kPostureVersion, "0.6.0") && !std::strcmp(post05::kReactionVersion, "0.6.0"));
  CHECK(!std::strcmp(post05::policy::kPolicyId, "POST05_TEST_POLICY_V4_GENERAL_ANTI_STAGGER"));   // la politica de REACCION de E_OC no cambia
  CHECK(!std::strcmp(post05::resource_text(static_cast<std::uint8_t>(NameKind::E_OC2)), "E_OC2") && !std::strcmp(post05::resource_text(0), "UNREAD"));
  Engine e;  // el motor arranca en V1 salvo que mod.cpp lo cambie
  CHECK(e.policy == V1);
}

int main() {
  t01_t12_identity();
  t13_t14_blue_group(V2);
  t13_t14_blue_group(V1);
  t15_disagreement_marks_unsafe();
  t16_lookup_unverified_mask();
  t17_red_deferred();
  t18_black_fury_between_calls();
  t19_power_change_in_call();
  t20_e_oc2_v1_excluded();
  t21_resource_signal_any_source();
  t22_e_oc2_lethal_and_other_families();
  t23_mixed_population_counters();
  t24_devf_blue_on_e_oc2();
  t25_t27_counters_and_format();
  t28_t29_twins();
  for (std::uint64_t seed : {11ull, 23ull, 57ull, 101ull, 977ull, 4099ull}) stress(seed, 15000);
  t31_t32_b1_patterns();
  t33_native_e_oc_identical_v1_v2();
  t34_group_posture_v1_vs_v2();
  t35_duplicate_token_e_oc2();
  t36_session_closed_keeps_e_oc2_reason();
  t37_t40_tables();
  std::printf("stress coverage: oracle_compares=%llu applied=%llu (atp2=%llu) e_oc2_deferred=%llu (would_be_eligible=%llu) power_changed=%llu unsafe=%llu vanilla=%llu\n",
              g_cmp, g_applied, g_applied_atp2, g_deferred, g_deferred_shaped, g_power, g_unsafe, g_vanilla);
  CHECK(g_applied >= 100 && g_applied_atp2 >= 20 && g_deferred >= 200 && g_deferred_shaped >= 20 && g_power >= 10 && g_unsafe >= 10);   // cobertura minima exigida
  std::printf("POST05_COMPAT_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
