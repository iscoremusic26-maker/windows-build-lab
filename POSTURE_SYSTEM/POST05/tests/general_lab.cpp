// POST05-GENERAL (0.5.4) laboratorio: regla POSTURA < 100 => SIN TAMBALEO en cualquier modo normal, sobre el motor COMPLETO (POST03.1 + POST04 +
// POST05) con el modelo de damage_check de compat_lab ampliado a lo que setActionMode/offTgSph escriben (timers, HeadLock, bits Tg/Co, golpe Tg).
// Casos reales de los logs del usuario (Red 172 en FIND; Blue 161/162 solos fuera de ATTACK) como fixtures; rutas BIG_DAMAGE (giro, bola);
// letal/Finish/latch/cruce; familias; modos de guion; restauracion EXACTA de timers/HeadLock/bits; gemelos y oraculo independiente.
// LAB_PASS = consistencia con ESTE modelo de damage_check (d_a_e_oc.cpp:584-786). NO prueba animacion, IA ni contactos en el juego.
#include <climits>
#include <cstdio>
#include <cstring>
#include <map>
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
enum : std::uint32_t { SWORD = 0x4000000u, SHIELD = 0x10u, ARROW = 0x2000u, BOMB = 0x20u, IRON = 0x400000u, BOOM = 0x10000u, AT40 = 0x40u, OTHER = 0x80000u,
                       AMBIG = 0x4000000u | 0x2000u };
enum Mode { WAIT = 0, WALK = 1, TALK = 2, FIND = 3, ATTACK = 4, DAMAGE = 5, BIG = 6, WATCH = 7, SOUND = 8, DEATH = 9, WATER = 10, DEMO = 11, FALL = 14, STAY = 15, MOVE_OUT = 16 };

struct Fake {
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  ActorFacts f;
  int health = 100000, mode = FIND, old_mode = WAIT, state = 4, cooldown = 0;
  bool cutdown = false, tg[2] = {false, false};
  std::uintptr_t hit[2] = {0, 0}, sel = 0;
  std::int16_t t6c0 = 0, t6c2 = 0;
  bool head_lock = false;
  std::uint8_t set_bits = 0x0F;
  int cut = 0, cut_alt = 0, cut_timer = 0;
  bool spl = false, jump_cancel = false;
  int atp_in_call = -1;  // escritor desconocido de AtAtp DENTRO de damage_check
  Fake(std::uint32_t i, std::uintptr_t p, NameKind nk = NameKind::E_OC, int atp = 1) : id(i), ptr(p) {
    f.profile_ok = f.live = true; f.name = nk; f.status_10000 = nk == NameKind::E_OC; f.atp = atp;
  }
  post05::LiveMode live() const { return post05::LiveMode{mode, state, old_mode, t6c0, t6c2, static_cast<std::uint8_t>(head_lock ? 1 : 0), set_bits}; }
  void put(const post05::LiveMode& t) { mode = t.mode; state = t.state; old_mode = t.old_mode; t6c0 = t.t6c0; t6c2 = t.t6c2; head_lock = t.head_lock != 0; set_bits = t.set_bits; }
};
struct World { std::map<std::uintptr_t, std::uint32_t> types; std::map<std::uint32_t, std::uintptr_t> present; int power = 20; };
// d_a_e_oc.cpp:584 setActionMode / :631 offTgSph
static void set_action_mode(Fake& a, int m, int st) { a.old_mode = a.mode; a.mode = m; a.state = st; a.t6c0 = a.t6c2 = 0; a.set_bits |= 0x0F; a.head_lock = false; }
static void off_tg_sph(Fake& a) { a.set_bits &= static_cast<std::uint8_t>(~0x05u); a.tg[0] = a.tg[1] = false; }
// d_a_e_oc.cpp:638-786 damage_check (las ramas que importan aqui)
static void native(Fake& a, World& w) {
  if (a.atp_in_call >= 0) { a.f.atp = a.atp_in_call; a.atp_in_call = -1; }
  if (a.cutdown) { a.cutdown = false; a.health = 0; set_action_mode(a, DEATH, 1); off_tg_sph(a); return; }
  if (a.cooldown) return;
  bool had = false;
  for (int i = 0; i < 2; ++i) if (a.tg[i]) { a.sel = a.hit[i]; had = true; }
  if (!had) return;
  const std::uint32_t t = w.types.at(a.sel);
  a.cooldown = 10;
  int my = 2;
  if (t & M.sword) {
    my = a.cut;
    if (my <= 2) { if (a.cut_timer) { a.cut_alt ^= 1; my = a.cut_alt; } else a.cut_alt = my; a.cut_timer = 30; }  // alternancia field_0x6d6/6dc
  } else if (t & M.shield) my = 3;
  else if (t & M.arrow) my = 0;
  else if (t & M.bomb) { my = 5; a.health = 0; }
  else if (t & M.iron_ball) { my = 5; a.health += 80; }
  else if (t & M.boomerang) my = 4;
  a.health -= w.power;  // cc_at_check
  if (a.health < 0) a.health = 0;
  if (a.spl && my < 5) my = 5;
  if (a.health <= 1 && my < 5) my = 5;
  if (a.jump_cancel) { set_action_mode(a, DAMAGE, my); a.cooldown = 3; return; }
  if (my >= 5) { set_action_mode(a, BIG, my - 5); off_tg_sph(a); }
  else set_action_mode(a, DAMAGE, my);
}
static Snapshot snap_of(const Fake& a) {
  Snapshot s; s.health = a.health; s.mode = a.mode; s.state = a.state; s.old_mode = a.old_mode; s.cooldown = a.cooldown; s.cutdown = a.cutdown ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((a.tg[0] ? 1 : 0) | (a.tg[1] ? 2 : 0)); return s;
}
static NativeAux aux_of(const Fake& a) {
  NativeAux x; x.valid = 1; x.t6c0 = a.t6c0; x.t6c2 = a.t6c2; x.head_lock = a.head_lock ? 1 : 0; x.set_bits = a.set_bits;
  x.atp = static_cast<std::int8_t>(a.f.atp); x.resource = static_cast<std::uint8_t>(a.f.name);
  x.anim_ok = 1; x.anim_attr = 2; x.anim_state = 0; x.anim_end = 40; x.anim_rate = 1.0f; x.anim_frame = 5.0f; x.speed = 0.0f;
  return x;
}
struct Hx {
  post04::PostureEngine pe;
  post05::Reaction rx;
  World w;
  std::vector<post05::DecisionResult> dec;
  std::vector<post05::Apply> app;
  std::vector<HitReport> reps;
  int writes = 0;
  bool broken_writer = false, partial_writer = false;
  explicit Hx(post05::StaggerPolicy sp = post05::StaggerPolicy::General) {
    pe.eng.masks = M; pe.eng.policy = AdmissionPolicy::V2Compat; rx.e2_policy = post05::E2Policy::MatureBC; rx.stagger_policy = sp; rx.e2_env = post05::E2Env::Absent;
    w.types[P(1)] = SWORD; w.types[P(3)] = IRON; w.types[P(4)] = BOOM; w.types[P(5)] = ARROW; w.types[P(6)] = OTHER; w.types[P(8)] = BOMB;
    w.types[P(9)] = SHIELD; w.types[P(10)] = AT40; w.types[P(11)] = AMBIG;
  }
  static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }
  PreReads pre_of(const Fake& a) const {
    PreReads r; r.facts = a.f;
    if (is_candidate(admit(a.f, pe.eng.policy).adm)) { r.snap = snap_of(a); r.raw0 = a.tg[0] ? a.hit[0] : 0; r.raw1 = a.tg[1] ? a.hit[1] : 0; r.aux = aux_of(a); }
    return r;
  }
  static PostReads post_of(const Fake& a) { PostReads p; p.alive = true; p.snap = snap_of(a); p.selected = a.sel; p.aux = aux_of(a); return p; }
  void spawn(Fake& a) { w.present[a.id] = a.ptr; }
  bool call(Fake& a) {
    if (a.cooldown > 0) --a.cooldown;
    if (a.cut_timer > 0) --a.cut_timer;
    Events ev; pe.on_pre(a.ptr, a.id, pre_of(a), ev);
    native(a, w);
    Events ev2; HitReport rep;
    const bool has = pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) {
      auto it = w.types.find(s); if (it == w.types.end()) return false; t = it->second; return true; }, ev2, rep);
    if (has) {
      reps.push_back(rep);
      post05::Apply ap = post05::Apply::NotAttempted;
      const post05::DecisionResult d = post05::process(rx, pe, rep, [&a] { return a.live(); },
                                                       [&](const post05::LiveMode& t) { ++writes; if (partial_writer) { a.mode = t.mode; a.state = t.state; a.old_mode = t.old_mode; } else if (!broken_writer) a.put(t); }, ap);
      dec.push_back(d); app.push_back(ap);
    }
    post04::PItem it; while (pe.px.pop(it)) {}
    return has;
  }
  void update() {
    Observer& o = pe.eng.obs;
    if (o.sweep_due())
      for (unsigned q = 0; q < kSweepQuota; ++q) {
        Pick p; if (!o.ledger.next_sweep(p, o.sweep_stamp)) break;
        auto it = w.present.find(p.id);
        const Lookup l = it == w.present.end() ? Lookup::Absent : it->second == p.ptr ? Lookup::PresentMatch : Lookup::PresentMismatchPtr;
        Events ev; o.ledger.apply_lookup(p, l, o.epoch.epoch, ev);
      }
    pe.on_update(); post04::PItem it; while (pe.px.pop(it)) {}
  }
  int slot_of(std::uint32_t id) const {
    for (unsigned i = 0; i < kCapacity; ++i) { const Instance& s = pe.eng.obs.ledger.slot(i); if (s.id == id && s.life == Life::Active) return static_cast<int>(i); }
    return -1;
  }
  int posture(std::uint32_t id) const { const int s = slot_of(id); if (s < 0) return -1; const post04::Entry& e = pe.px.entry(static_cast<unsigned>(s)); return e.bound ? e.posture : -1; }
  post05::Reason last_reason() const { return dec.empty() ? post05::Reason::Count : dec.back().reason; }
  post05::Apply last_apply() const { return app.empty() ? post05::Apply::NotAttempted : app.back(); }
};
// Golpe en el modo/estado que el actor YA tiene (no se fuerza ATTACK). Espera al cooldown tick a tick (sin tocar el modo).
static bool strike(Hx& h, Fake& a, std::uintptr_t c, int sphere = 1) {
  while (a.cooldown > 0 && a.health > 0) {
    const post05::LiveMode keep = a.live(); h.call(a); a.put(keep);  // el modelo no ejecuta la IA: el modo PRE lo fija el caso
  }
  a.tg[sphere] = true; a.hit[sphere] = c;
  const bool has = h.call(a);
  a.tg[0] = a.tg[1] = false;
  return has;
}
static void audit_all(Hx& h) { CHECK(h.rx.audit().ok); CHECK(h.pe.px.audit().ok); CHECK(h.pe.eng.conserved()); }

// ---- G01: caso REAL Red 172 (sesion aislada 0.5.3): FIND, 0->25->50->75 sin DAMAGE; 75->100 ruptura; latch; regeneracion 23->48->73->98 ----
static void g01_red172_find() {
  Hx h; Fake r(172, 0x9000, NameKind::E_OC2, 2); h.spawn(r);
  r.mode = FIND; r.state = 4; r.old_mode = WAIT; r.t6c0 = 7; r.t6c2 = 15;  // persiguiendo, con temporizadores nativos en marcha
  const int hp0 = r.health;
  for (int i = 1; i <= 3; ++i) {
    const post05::LiveMode pre = r.live();
    CHECK(strike(h, r, Hx::P(1)));
    CHECK(h.last_reason() == post05::Reason::Eligible && h.last_apply() == post05::Apply::Applied && h.dec.back().plan.kind == post05::PlanKind::RestorePre);
    CHECK(post05::same_live(r.live(), pre));                    // EXACTAMENTE el PRE: FIND 4, timers 7/15, bits, HeadLock
    CHECK(r.mode != DAMAGE && h.posture(172) == 25 * i && r.health == hp0 - 20 * i && r.cooldown == 10);
  }
  CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::CrossesThreshold && r.mode == DAMAGE && h.posture(172) == 100);   // ruptura (sustituto nativo)
  r.mode = FIND; r.state = 4;
  CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::LatchedAfter && r.mode == DAMAGE);                                // latch: POST04/POST06
  // otra instancia con regeneracion parcial: 0->25->50->75, recupera hasta 23, luego 23->48->73->98 (secuencia real 4-6)
  Hx g; Fake s(173, 0x9400, NameKind::E_OC2, 2); g.spawn(s); s.mode = FIND; s.state = 4;
  for (int i = 0; i < 3; ++i) CHECK(strike(g, s, Hx::P(1)) && g.last_reason() == post05::Reason::Eligible && s.mode == FIND);
  for (int i = 0; i < 2000 && g.posture(173) > 23; ++i) g.update();
  CHECK(g.posture(173) == 23);
  for (int want : {48, 73, 98}) CHECK(strike(g, s, Hx::P(1)) && g.last_reason() == post05::Reason::Eligible && s.mode == FIND && g.posture(173) == want);
  audit_all(h); audit_all(g);
}
// ---- G02: caso REAL Blue 161/162 solo fuera de ATTACK: WAIT/WALK -> descubre a Link (FIND 1) sin animacion de dano; 3 golpes < 100 sin DAMAGE ----
static void g02_blue_alone() {
  for (int pre_mode : {WAIT, WALK, WATCH, SOUND}) {
    Hx h; Fake b(161, 0xa000); h.spawn(b); b.mode = pre_mode; b.state = 2; b.t6c0 = 9;
    CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && h.dec.back().plan.kind == post05::PlanKind::NoticeFind1);
    CHECK(b.mode == FIND && b.state == 1 && b.old_mode == pre_mode && b.t6c0 == 0 && b.t6c2 == 0 && b.set_bits == 0x0F);   // = setActionMode(FIND, 1)
    // ya persigue: los siguientes golpes vuelven al PRE (FIND), sin DAMAGE
    b.state = 4;
    CHECK(strike(h, b, Hx::P(1)) && h.dec.back().plan.kind == post05::PlanKind::RestorePre && b.mode == FIND && b.state == 4);
    CHECK(strike(h, b, Hx::P(1)) && b.mode == FIND && h.posture(161) == 75);
    CHECK(h.rx.c.applied == 3 && h.rx.c.applied_by_kind[2] == 1 && h.rx.c.applied_by_kind[1] == 2);
    audit_all(h);
  }
  // Blue en ATTACK (regresion 0.5.x) y en DAMAGE previo: no reinicia la reaccion (estado 6 = reproduciendo) ni encadena
  Hx h; Fake b(162, 0xa400); h.spawn(b);
  b.mode = ATTACK; b.state = 1; CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && b.mode == ATTACK && b.state == 1);
  b.mode = DAMAGE; b.state = 6; b.old_mode = ATTACK; CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && b.mode == DAMAGE && b.state == 6);
  b.mode = ATTACK; b.state = 3; CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && b.mode == ATTACK && b.state == 3);  // rebote: continua
  audit_all(h);
}
// ---- G03: BIG_DAMAGE por debajo de 100 (giro, 4.o corte, salto, AtSpl, bola de hierro): se deshace TAMBIEN offTgSph (bits Tg) ----
static void g03_big_damage() {
  for (int cut : {5, 6, 7}) {
    Hx h; Fake r(180, 0xb000, NameKind::E_OC2, 2); h.spawn(r); r.mode = FIND; r.state = 4; r.t6c2 = 4;
    const post05::LiveMode pre = r.live(); const int hp = r.health;
    r.cut = cut; CHECK(strike(h, r, Hx::P(1)));
    CHECK(h.last_reason() == post05::Reason::Eligible && h.dec.back().plan.aux_write && post05::same_live(r.live(), pre) && r.set_bits == 0x0F);
    CHECK(r.health == hp - 20 && !r.tg[0] && !r.tg[1] && h.rx.c.applied_big == 1);   // dano aplicado; el golpe Tg ya consumido sigue limpio
    audit_all(h);
  }
  { Hx h; Fake r(181, 0xb400); h.spawn(r); r.mode = ATTACK; r.state = 2; r.spl = true; CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && r.mode == ATTACK); }
  { Hx h; Fake r(182, 0xb800); h.spawn(r); r.mode = WALK; r.cut = 6;
    CHECK(strike(h, r, Hx::P(1)) && h.dec.back().plan.kind == post05::PlanKind::NoticeFind1 && r.mode == FIND && r.state == 1 && r.set_bits == 0x0F); }
  // bola de hierro +50: 0->50 sin derribo; 50->100 cruza => BIG nativo
  Hx h; Fake r(183, 0xbc00); h.spawn(r); r.mode = FIND; r.state = 4;
  CHECK(strike(h, r, Hx::P(3)) && h.last_reason() == post05::Reason::Eligible && r.mode == FIND && h.posture(183) == 50);
  CHECK(strike(h, r, Hx::P(3)) && h.last_reason() == post05::Reason::CrossesThreshold && r.mode == BIG && (r.set_bits & 0x05) == 0);
  audit_all(h);
}
// ---- G04: letal, Finish, latch con familias de peso 0, familias, modos de guion, salto cancelado ----
static void g04_exclusions_and_families() {
  auto one = [](int mode, std::uintptr_t c, int health, post05::Reason want, int expect_mode) {
    Hx h; Fake r(190, 0xc000); h.spawn(r); r.mode = mode; r.state = 1; r.health = health;
    CHECK(strike(h, r, c) && h.last_reason() == want);
    if (expect_mode >= 0) CHECK(r.mode == expect_mode);
    if (want != post05::Reason::Eligible) CHECK(h.writes == 0);
    audit_all(h);
  };
  using R = post05::Reason;
  one(FIND, Hx::P(1), 15, R::Lethal, BIG);                  // letal: muere normalmente
  one(WAIT, Hx::P(8), 100000, R::Lethal, BIG);              // bomba: letal
  one(FIND, Hx::P(5), 100000, R::Eligible, FIND);           // flecha (peso 0): sin tambaleo, postura no cambia
  one(FIND, Hx::P(9), 100000, R::Eligible, FIND);           // escudo (peso 0)
  one(FIND, Hx::P(4), 100000, R::Eligible, FIND);           // bumeran (peso 0)
  one(FIND, Hx::P(10), 100000, R::Eligible, FIND);          // AT_TYPE_40
  one(FIND, Hx::P(11), 100000, R::FamilyUnverified, DAMAGE); // ambiguo: nativo
  for (int m : {TALK, DEATH, WATER, DEMO, FALL}) one(m, Hx::P(1), 100000, R::PreModeScripted, DAMAGE);
  for (int m : {STAY, MOVE_OUT}) one(m, Hx::P(1), 100000, R::Eligible, m);
  { Hx h; Fake r(191, 0xc400); h.spawn(r); r.mode = FIND; r.cutdown = true; strike(h, r, Hx::P(1)); CHECK(r.mode == DEATH && h.writes == 0); }  // Finish
  { Hx h; Fake r(192, 0xc800); h.spawn(r); r.mode = ATTACK; r.state = 1; r.jump_cancel = true; r.cut = 1;
    CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == R::Eligible && r.mode == ATTACK && r.cooldown == 3); }   // salto con giro cancelado (cooldown 3)
  // flecha con postura enganchada a 100: nativo
  Hx h; Fake r(193, 0xcc00); h.spawn(r); r.mode = FIND; r.state = 4;
  for (int i = 0; i < 4; ++i) { strike(h, r, Hx::P(1)); r.mode = FIND; r.state = 4; }
  CHECK(h.posture(193) == 100);
  CHECK(strike(h, r, Hx::P(5)) && h.last_reason() == R::LatchedAfter && r.mode == DAMAGE);
  audit_all(h);
}
// ---- G05: timers y HeadLock PRE no nulos se restauran EXACTOS; escritor de 3 campos con plan auxiliar => no intento; relectura; verificacion ----
static void g05_exact_and_failsafe() {
  { Hx h; Fake r(200, 0xd000); h.spawn(r); r.mode = FIND; r.state = 4; r.t6c0 = 3; r.t6c2 = 18; r.head_lock = true;
    const post05::LiveMode pre = r.live();
    CHECK(strike(h, r, Hx::P(1)) && h.dec.back().plan.aux_write && post05::same_live(r.live(), pre) && r.head_lock && r.t6c2 == 18); }
  { // escritor heredado de 3 campos: un plan que necesita mas campos NO se intenta (sin escritura parcial)
    Hx h; Fake r(201, 0xd400); h.spawn(r); r.mode = FIND; r.state = 4; r.t6c2 = 5;
    while (r.cooldown) h.call(r);
    r.tg[1] = true; r.hit[1] = Hx::P(1);
    if (r.cooldown > 0) --r.cooldown;
    Events ev; h.pe.on_pre(r.ptr, r.id, h.pre_of(r), ev); native(r, h.w); r.tg[1] = false;
    Events ev2; HitReport rep; CHECK(h.pe.on_post(r.ptr, h.post_of(r), [&](std::uintptr_t s, std::uint32_t& t) { t = h.w.types.at(s); return true; }, ev2, rep));
    int w3 = 0; post05::Apply ap = post05::Apply::Applied;
    const post05::DecisionResult d = post05::process(h.rx, h.pe, rep, [&r] { return r.live(); }, [&](int, int, int) { ++w3; }, ap);
    CHECK(d.decision == post05::Decision::SuppressEligible && d.plan.aux_write && ap == post05::Apply::NotAttempted && w3 == 0 && r.mode == DAMAGE);
    CHECK(h.rx.c.not_attempted == 1 && h.rx.audit().ok); }
  { // relectura: otro escritor cambio un timer entre POST y la escritura => RECHECK_FAILED, nada escrito
    Hx h; Fake r(202, 0xd800); h.spawn(r); r.mode = FIND; r.state = 4;
    while (r.cooldown) h.call(r);
    r.tg[1] = true; r.hit[1] = Hx::P(1);
    Events ev; h.pe.on_pre(r.ptr, r.id, h.pre_of(r), ev); native(r, h.w); r.tg[1] = false;
    Events ev2; HitReport rep; CHECK(h.pe.on_post(r.ptr, h.post_of(r), [&](std::uintptr_t s, std::uint32_t& t) { t = h.w.types.at(s); return true; }, ev2, rep));
    r.t6c0 = 1;  // escritor desconocido
    int w = 0; post05::Apply ap = post05::Apply::Applied;
    post05::process(h.rx, h.pe, rep, [&r] { return r.live(); }, [&](const post05::LiveMode& t) { ++w; r.put(t); }, ap);
    CHECK(ap == post05::Apply::RecheckFailed && w == 0 && r.mode == DAMAGE); }
  { // verificacion: escritor sin efecto => VERIFY_FAILED y sesion cerrada (nunca mas se intenta)
    Hx h; h.broken_writer = true; Fake r(203, 0xdc00); h.spawn(r); r.mode = FIND; r.state = 4;
    CHECK(strike(h, r, Hx::P(1)) && h.last_apply() == post05::Apply::VerifyFailed && h.rx.session_disabled());
    h.broken_writer = false; r.mode = FIND; r.state = 4;
    CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::SessionDisabled && r.mode == DAMAGE); }
  { // escritor que solo escribe los 3 campos de modo cuando el plan exige timers: la verificacion completa lo detecta
    Hx h; h.partial_writer = true; Fake r(204, 0xe800); h.spawn(r); r.mode = FIND; r.state = 4; r.t6c2 = 9;
    CHECK(strike(h, r, Hx::P(1)) && h.last_apply() == post05::Apply::VerifyFailed && h.rx.session_disabled()); }
}
// ---- G06: gemelos y oraculo independiente sobre modos/familias/posturas aleatorias ----
static void g06_twins_oracle() {
  std::uint64_t s = 0x2545F4914F6CDD1Dull;
  auto rnd = [&]() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; };
  static const int kModes[] = {WAIT, WALK, TALK, FIND, ATTACK, DAMAGE, BIG, WATCH, SOUND, STAY, MOVE_OUT, DEMO};
  static const unsigned kTypes[] = {1, 1, 1, 1, 3, 4, 5, 9, 10, 11};
  unsigned elig = 0, vanilla = 0, big_elig = 0, find1 = 0;
  for (unsigned run = 0; run < 150; ++run) {
    Hx on; Hx off; off.rx.close_session();
    Fake a(300 + run, 0x20000 + run * 0x100, (run % 2) ? NameKind::E_OC : NameKind::E_OC2, (run % 2) ? 1 : 2), b = a;
    on.spawn(a); off.spawn(b);
    bool died = false;
    for (unsigned k = 0; k < 8; ++k) {
      const int m = kModes[rnd() % 12];
      const unsigned ty = kTypes[rnd() % 10];
      a.mode = b.mode = m; a.state = b.state = static_cast<int>(rnd() % 7); a.cut = b.cut = static_cast<int>(rnd() % 8); if (a.cut == 3 || a.cut == 4) a.cut = b.cut = 0;
      a.t6c0 = b.t6c0 = static_cast<std::int16_t>(rnd() % 4); a.head_lock = b.head_lock = (rnd() % 5) == 0;
      if (rnd() % 9 == 0) a.health = b.health = 15;
      const int p_before = on.posture(a.id);
      const post05::LiveMode pre = a.live();
      CHECK(strike(on, a, Hx::P(ty)) == strike(off, b, Hx::P(ty)));
      CHECK(a.health == b.health && a.cooldown == b.cooldown && a.cut_alt == b.cut_alt && on.posture(a.id) == off.posture(b.id));   // vida/cooldown/alternancia/postura
      // oraculo: proteger <=> confirmado, no letal, familia verificada, modo normal, postura antes < 100 y despues < 100
      const int w = ty == 1 ? 25 : ty == 3 ? 50 : 0;
      const bool lethal = b.health <= 1;
      const bool ok_mode = m == WAIT || m == WALK || (m >= FIND && m <= SOUND) || m == STAY || m == MOVE_OUT;
      const int pb = p_before < 0 ? 0 : p_before;   // primer contacto: la postura se vincula en esta llamada (0)
      const bool expect = !died && !lethal && ty != 11 && ok_mode && pb < 100 && pb + w < 100;
      if (lethal) died = true;   // muerte vista: POST04 cierra la entrada, despues todo es nativo
      const bool got = on.last_reason() == post05::Reason::Eligible && on.last_apply() == post05::Apply::Applied;
      CHECK(expect == got);
      if (expect != got && g_fail < 140) std::printf("  oracle m=%d st=%d ty=%u cut=%d p=%d hp=%d reason=%s apply=%s bmode=%d\n", m, pre.state, ty, a.cut, p_before, b.health,
                                                     post05::name(on.last_reason()), post05::name(on.last_apply()), b.mode);
      if (got) {
        ++elig; if (b.mode == BIG) ++big_elig;
        const bool unaware = m == WAIT || m == WALK || m == WATCH || m == SOUND;
        if (unaware) { ++find1; CHECK(a.mode == FIND && a.state == 1 && a.old_mode == m && a.set_bits == 0x0F && a.t6c0 == 0); }
        else CHECK(post05::same_live(a.live(), pre));
        CHECK(a.mode != DAMAGE || m == DAMAGE);   // nunca ENTRA en DAMAGE por un golpe protegido
      } else {
        ++vanilla; CHECK(a.mode == b.mode && a.state == b.state && a.set_bits == b.set_bits);   // nativo intacto
      }
    }
    CHECK(on.rx.audit().ok && on.pe.px.audit().ok);
  }
  std::printf("general_twins: hits=1200 protected=%u (big_damage_path=%u notice_find1=%u) native=%u\n", elig, big_elig, find1, vanilla);
  CHECK(elig >= 200 && big_elig >= 20 && find1 >= 50 && vanilla >= 200);
}
// ---- G07: varios actores en el mismo tick y reciclado de generacion ----
static void g07_multi_and_reuse() {
  Hx h; Fake a(400, 0xe000), b(401, 0xe400, NameKind::E_OC2, 3);
  h.spawn(a); h.spawn(b); a.mode = WAIT; b.mode = FIND; b.state = 4;
  CHECK(strike(h, a, Hx::P(1)) && strike(h, b, Hx::P(3)));
  CHECK(a.mode == FIND && a.state == 1 && b.mode == FIND && b.state == 4 && h.posture(400) == 25 && h.posture(401) == 50);
  h.w.present.erase(400); for (int i = 0; i < 400; ++i) h.update();
  Fake a2(400, 0xf000); h.spawn(a2); a2.mode = WALK; for (int i = 0; i < 3; ++i) h.call(a2);
  CHECK(strike(h, a2, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && h.posture(400) == 25 && a2.mode == FIND);
  audit_all(h);
}
static void in_zero_weight_check(Hx& g) {
  post05::DecisionInput in; in.rep = g.reps.back(); in.outc = g.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC_CANDIDATE; in.life = Life::Active;
  in.rep.event = in.outc.event = 9000; in.outc.applied = false; in.outc.skip = post04::Skip::ZeroWeight; in.outc.before = in.outc.after = 25;
  CHECK(g.rx.decide(in).reason == post05::Reason::Eligible);   // peso 0 bajo 100: protegido
}
// ---- G09: actor visto primero como no admitido (esferas en desacuerdo) y luego candidato: su postura NO se registra => nativo ----
static void g09_posture_not_tracked() {
  Hx h; Fake r(600, 0x40000); h.spawn(r); r.mode = FIND; r.state = 4; r.f.atp = -2;
  for (int i = 0; i < 3; ++i) h.call(r);
  r.f.atp = 1;
  CHECK(strike(h, r, Hx::P(1)));
  CHECK(h.last_reason() != post05::Reason::Eligible && r.mode == DAMAGE && h.writes == 0);
  std::printf("untracked_posture: reason=%s\n", post05::name(h.last_reason()));
  // defensa en profundidad: un golpe coherente cuya postura POST04 se omitio por algo distinto de "peso 0" (sin sidecar, congelado...) nunca se protege
  Hx g; Fake q(601, 0x40400); g.spawn(q); q.mode = FIND; q.state = 4;
  CHECK(strike(g, q, Hx::P(1)) && g.last_reason() == post05::Reason::Eligible);
  for (post04::Skip sk : {post04::Skip::NotAdmitted, post04::Skip::UnknownFamily, post04::Skip::Frozen, post04::Skip::StaleGeneration}) {
    post05::DecisionInput in; in.rep = g.reps.back(); in.outc = g.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC_CANDIDATE; in.life = Life::Active;
    static std::uint64_t ev = 5000; in.rep.event = in.outc.event = ++ev;
    in.outc.applied = false; in.outc.skip = sk; in.outc.before = in.outc.after = 25;
    CHECK(g.rx.decide(in).reason == post05::Reason::PostureSkipUnsafe);
  }
  in_zero_weight_check(g);
}
// ---- G10: E_OC2 con senales mixtas o politica E_OC2 cerrada => nativo; potencia cambiada DENTRO de la llamada => SKIP_UNSAFE ----
static void g10_e2_identity_and_power() {
  { Hx h; Fake r(700, 0x50000, NameKind::E_OC2, 2); h.spawn(r); r.mode = FIND; r.state = 4;
    CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible);
    post05::DecisionInput in; in.rep = h.reps.back(); in.outc = h.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC2_CANDIDATE; in.life = Life::Active;
    in.rep.event = in.outc.event = 7001; in.rep.aux_post.resource = static_cast<std::uint8_t>(NameKind::E_OC);
    CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
    h.rx.e2_policy = post05::E2Policy::Deferred; in.rep.event = in.outc.event = 7002; in.rep.aux_post.resource = in.rep.aux_pre.resource;
    CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred); }
  { Hx h; Fake b(701, 0x50400, NameKind::E_OC2, 3); h.spawn(b); b.mode = FIND; b.state = 4; b.atp_in_call = 6;
    CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::PowerChangedInCall && b.mode == DAMAGE && h.writes == 0); }
  { Hx h; Fake b(702, 0x50800, NameKind::E_OC2, 6); h.spawn(b); b.mode = ATTACK; b.state = 2;   // Black en furia atacando: protegido (sin cambio en la llamada)
    CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && b.mode == ATTACK && b.state == 2); }
}
// ---- G08: politica legacy intacta (AttackOnly = 0.5.3): FIND sigue siendo PRE_MODE_NOT_ATTACK ----
static void g08_legacy_unchanged() {
  Hx h(post05::StaggerPolicy::AttackOnly); Fake r(500, 0xf400); h.spawn(r); r.mode = FIND; r.state = 4;
  CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::PreModeNotAttack && r.mode == DAMAGE);
  CHECK(!std::strcmp(post05::name(post05::StaggerPolicy::General), "GENERAL_POSTURE_BELOW_100") && post05::kReasonCount == 39);
  for (post05::Reason x : {post05::Reason::FamilyUnverified, post05::Reason::PreModeScripted, post05::Reason::PostShapeUnknown, post05::Reason::PostureSkipUnsafe})
    CHECK(post05::decision_of(x) == post05::Decision::AllowVanilla);
  char d[kLogLineMax]; Hx g; Fake q(501, 0xf800); g.spawn(q); q.mode = WAIT; CHECK(strike(g, q, Hx::P(1)));
  CHECK(post05::fmt_decision(d, sizeof(d), g.reps.back(), g.pe.last, g.dec.back(), g.app.back()) > 0 && std::strstr(d, "pre_mode=0") &&
        std::strstr(d, "post_mode=5") && std::strstr(d, "action=SUPPRESSION_APPLIED") && std::strstr(d, "plan=NOTICE_FIND1"));
  char line[kLogLineMax * 4];
  post05::Reaction mx = g.rx; for (auto& v : mx.c.by_reason) v = UINT64_MAX; for (auto& v : mx.c.applied_by_pre_mode) v = UINT64_MAX;
  const int n = post05::fmt_summary(line, sizeof(line), mx, "PERIODIC");
  CHECK(n > 0 && n < static_cast<int>(sizeof(line)) && std::strstr(line, "stagger_policy=GENERAL_POSTURE_BELOW_100"));
}

int main() {
  g01_red172_find();
  g02_blue_alone();
  g03_big_damage();
  g04_exclusions_and_families();
  g05_exact_and_failsafe();
  g06_twins_oracle();
  g07_multi_and_reuse();
  g08_legacy_unchanged();
  g09_posture_not_tracked();
  g10_e2_identity_and_power();
  std::printf("POST05_GENERAL_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
