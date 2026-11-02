// POST05-RB (0.5.2) laboratorio: ruta E_OC2 (Red nativo, Red de Expansion, Black 3/3 -> 6/6 y 6/6).
// Parte A: politica + escritor sobre el motor COMPLETO (POST03.1 + POST04 + POST05) con el mismo modelo de damage_check que compat_lab, gemelos
//          con/sin POST05 y casos negativos. E_OC debe decidir EXACTAMENTE igual que en 0.5.1.
// Parte B: gemelos con el codigo REAL de Bokoblin Expansion 0.17.0 (cabeceras CP21 192a773 copiadas byte a byte en tests/cp21, SHA-256 en
//          tests/cp21/CP21_MANIFEST.json): un golpe B/C completo con el modelo nativo de animacion (executeAttack + J3DFrameCtrl::update), con y sin un
//          golpe SUPRIMIDO en cada tick. Propiedad: en todo tick que el gate de POST05 habilita, la trayectoria observable (modo, estado, clip, frame,
//          rate, escrituras de rate/esferas de Expansion, chip de escudo) es IDENTICA a la del gemelo sin golpe. Donde difiere, el gate dice vanilla.
// LAB_PASS = consistencia con esos modelos y con el codigo real de la politica de Expansion. NO prueba contactos, animacion ni FPS en el juego.
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <set>
#include <vector>
#include "../src/reaction_core.hpp"
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19f2_policy.hpp"
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"

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

// =====================================================================================================================================
// Parte A: motor completo
// =====================================================================================================================================
static const AtMasks M{0x4000002u, 0x10u, 0x2000u, 0x20u, 0x400000u, 0x10000u, 0x40u, 0x80u};
enum : std::uint32_t { SWORD = 0x4000000u, SHIELD = 0x10u, ARROW = 0x2000u, BOMB = 0x20u, IRON = 0x400000u, BOOM = 0x10000u, OTHER = 0x80000u };

struct World {
  std::map<std::uintptr_t, std::uint32_t> types;
  std::map<std::uint32_t, std::uintptr_t> present;
  int power = 20;
  std::uint64_t native_calls = 0;
};
struct Anim {  // lo que read_aux lee de mpMorf->mFrameCtrl y speedF
  bool ok = true;
  std::uint8_t attr = 0, fstate = 0;
  std::int16_t end = 42;
  float rate = 1.15f, frame = 10.0f, speed = 0.0f;
  bool operator==(const Anim& o) const {  // por bits (NaN incluido)
    return ok == o.ok && attr == o.attr && fstate == o.fstate && end == o.end && !std::memcmp(&rate, &o.rate, 4) && !std::memcmp(&frame, &o.frame, 4) &&
           !std::memcmp(&speed, &o.speed, 4);
  }
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
  int atp_in_call = -1;      // >=0: escritor desconocido de AtAtp DENTRO de damage_check
  bool anim_in_call = false;  // escritor desconocido de la animacion DENTRO de damage_check
  Anim an;
  Fake(std::uint32_t i, std::uintptr_t p, NameKind nk = NameKind::E_OC2, int atp = 2) : id(i), ptr(p) {
    f.profile_ok = f.live = true;
    f.name = nk;
    f.status_10000 = nk == NameKind::E_OC;
    f.atp = atp;
  }
  void hit_with(int s, std::uintptr_t c) { tg[s] = true; hit[s] = c; }
  // fase PRE: B (estado 1, fin 42) o C (estado 2, fin 47)
  void bc(int st, float frame, float rate) { state = st; an = Anim{}; an.end = st == 1 ? 42 : 47; an.frame = frame; an.rate = rate; }
  void brake(float speed) { state = 0; an = Anim{}; an.attr = 2; an.end = 40; an.frame = 3; an.rate = 1; an.speed = speed; }  // clip de espera en bucle
};
static void set_action_mode(Fake& a, int m, int st) { a.old_mode = a.mode; a.mode = m; a.state = st; a.t6c0 = a.t6c2 = 0; a.set_bits |= 0x0F; a.head_lock = false; }
static void off_tg_sph(Fake& a) { a.set_bits &= static_cast<std::uint8_t>(~0x05u); a.tg[0] = a.tg[1] = false; }
static void clear_tg(Fake& a) { a.tg[0] = a.tg[1] = false; a.hit[0] = a.hit[1] = 0; }
static void native(Fake& a, World& w) {  // mismo modelo que reaction_lab/compat_lab; damage_check NO toca la animacion ni speedF (d_a_e_oc.cpp:638-786)
  ++w.native_calls;
  if (a.atp_in_call >= 0) { a.f.atp = a.atp_in_call; a.atp_in_call = -1; }
  if (a.anim_in_call) { a.an.frame += 1.0f; a.anim_in_call = false; }
  if (a.cutdown) { a.cutdown = false; a.health = 0; set_action_mode(a, 9, 1); off_tg_sph(a); return; }
  if (a.cooldown) return;
  bool had = false;
  for (int i = 0; i < 2; ++i)
    if (a.tg[i]) { a.sel = a.hit[i]; had = true; }
  if (!had) return;
  const std::uint32_t t = w.types.at(a.sel);
  a.cooldown = (t & 0xD8000000u) ? 20 : 10;
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
static Snapshot snap_of(const Fake& a) {
  Snapshot s;
  s.health = a.health; s.mode = a.mode; s.state = a.state; s.old_mode = a.old_mode; s.cooldown = a.cooldown;
  s.cutdown = a.cutdown ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((a.tg[0] ? 1 : 0) | (a.tg[1] ? 2 : 0));
  return s;
}
static NativeAux aux_of(const Fake& a) {  // replica de read_aux de mod.cpp 0.5.2
  NativeAux x; x.valid = 1; x.t6c0 = a.t6c0; x.t6c2 = a.t6c2; x.head_lock = a.head_lock ? 1 : 0; x.set_bits = a.set_bits;
  x.atp = static_cast<std::int8_t>(a.f.atp); x.resource = static_cast<std::uint8_t>(a.f.name);
  x.speed = a.an.speed;
  if (a.an.ok) { x.anim_ok = 1; x.anim_attr = a.an.attr; x.anim_state = a.an.fstate; x.anim_end = a.an.end; x.anim_rate = a.an.rate; x.anim_frame = a.an.frame; }
  return x;
}
struct Rest {
  int health, cooldown, atp; std::int16_t t6c0, t6c2; bool head_lock, cutdown; std::uint8_t set_bits; std::uintptr_t sel; Anim an;
  bool operator==(const Rest&) const = default;
};
static Rest rest_of(const Fake& a) { return Rest{a.health, a.cooldown, a.f.atp, a.t6c0, a.t6c2, a.head_lock, a.cutdown, a.set_bits, a.sel, a.an}; }

struct Hx {
  post04::PostureEngine pe;
  post05::Reaction rx;
  World w;
  std::vector<post05::DecisionResult> dec;
  std::vector<post05::Apply> app;
  std::vector<HitReport> reps;
  int writes = 0;
  explicit Hx(post05::E2Policy p = post05::E2Policy::MatureBC, post05::E2Env env = post05::E2Env::Possible) {
    pe.eng.masks = M; pe.eng.policy = AdmissionPolicy::V2Compat; rx.e2_policy = p; rx.e2_env = env;
    w.types[P(1)] = SWORD; w.types[P(3)] = IRON; w.types[P(4)] = BOOM; w.types[P(5)] = ARROW; w.types[P(6)] = OTHER; w.types[P(8)] = BOMB; w.types[P(9)] = SHIELD;
  }
  static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }
  bool reader(std::uintptr_t sel, std::uint32_t& type) {
    auto it = w.types.find(sel);
    if (it == w.types.end() || !deref_alignment_ok(sel)) return false;
    type = it->second;
    return true;
  }
  PreReads pre_of(const Fake& a) const {
    PreReads r; r.facts = a.f;
    if (is_candidate(admit(a.f, pe.eng.policy).adm)) { r.snap = snap_of(a); r.raw0 = a.tg[0] ? a.hit[0] : 0; r.raw1 = a.tg[1] ? a.hit[1] : 0; r.aux = aux_of(a); }
    return r;
  }
  static PostReads post_of(const Fake& a) {
    PostReads p; p.alive = a.f.profile_ok && a.f.live;
    if (p.alive) { p.snap = snap_of(a); p.selected = a.sel; p.aux = aux_of(a); }
    return p;
  }
  void spawn(Fake& a) { w.present[a.id] = a.ptr; }
  bool call(Fake& a) {
    if (a.cooldown > 0) --a.cooldown;
    Events ev;
    pe.on_pre(a.ptr, a.id, pre_of(a), ev);
    native(a, w);
    Events ev2; HitReport rep;
    const bool has = pe.on_post(a.ptr, post_of(a), [&](std::uintptr_t s, std::uint32_t& t) { return reader(s, t); }, ev2, rep);
    if (has) {
      reps.push_back(rep);
      post05::Apply ap = post05::Apply::NotAttempted;
      const post05::DecisionResult d = post05::process(rx, pe, rep, [&a] { return post05::LiveMode{a.mode, a.state, a.old_mode}; },
                                                       [&](int m, int s, int o) { ++writes; a.mode = m; a.state = s; a.old_mode = o; }, ap);
      dec.push_back(d); app.push_back(ap);
    }
    post04::PItem it;
    while (pe.px.pop(it)) {}
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
    pe.on_update();
    post04::PItem it;
    while (pe.px.pop(it)) {}
  }
  int slot_of(std::uint32_t id) const {
    for (unsigned i = 0; i < kCapacity; ++i) { const Instance& s = pe.eng.obs.ledger.slot(i); if (s.id == id && s.life == Life::Active) return static_cast<int>(i); }
    return -1;
  }
  int posture(std::uint32_t id) const { const int s = slot_of(id); if (s < 0) return -1; const post04::Entry& e = pe.px.entry(static_cast<unsigned>(s)); return e.bound ? e.posture : -1; }
  post05::Reason last_reason() const { return dec.empty() ? post05::Reason::Count : dec.back().reason; }
  post05::Apply last_apply() const { return app.empty() ? post05::Apply::NotAttempted : app.back(); }
};
// Golpe con la fase PRE ya preparada en `a` (modo ATTACK; estado/animacion fijados por el caso)
static bool strike(Hx& h, Fake& a, std::uintptr_t c, int sphere = 1) {
  const int st = a.state; const Anim an = a.an;
  while (a.cooldown > 0 && a.health > 0) h.call(a);
  a.mode = 4; a.state = st; a.an = an;
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
// Gemelo: mismo golpe, POST05 cerrado. Solo deben diferir los 3 campos de modo, y solo si se aplico.
static void twin_check(Fake setup, bool expect_applied, post05::Reason expect) {
  Hx on, off; off.rx.close_session();
  Fake a = setup, b = setup;
  on.spawn(a); off.spawn(b);
  const int st = a.state;
  CHECK(strike(on, a, Hx::P(1)) && strike(off, b, Hx::P(1)));
  CHECK(on.last_reason() == expect);
  CHECK(rest_of(a) == rest_of(b));  // HP, cooldown, timers, flags, AtAtp, animacion: identicos
  CHECK(on.posture(a.id) == off.posture(b.id) && on.posture(a.id) == 25);
  if (expect_applied) {
    CHECK(on.last_apply() == post05::Apply::Applied && on.writes == 1 && a.mode == 4 && a.state == st && b.mode == 5);
  } else {
    CHECK(on.writes == 0 && a.mode == b.mode && a.state == b.state && a.old_mode == b.old_mode);
  }
  audit_all(on);
}

static void a01_red_mature() {
  for (int st : {1, 2})
    for (float rate : {1.0f, 1.15f, 2.0f})
      for (float fr : {2.0f, 2.15f, 9.5f, 16.0f, 22.0f, 40.9f}) {
        if (st == 1 && fr >= 42) continue;
        Fake r(100, 0x9000); r.bc(st, fr, rate);
        twin_check(r, true, post05::Reason::Eligible);
      }
  Fake r(101, 0x9200); r.bc(2, 46.5f, 2.0f);
  twin_check(r, true, post05::Reason::Eligible);
}
static void a02_excluded_phases() {
  Fake r(102, 0x9400);
  r.bc(1, 1.0f, 1.0f); twin_check(r, false, post05::Reason::E_OC2FrameEarly);            // A5-2: tick frame 1 (x2 pendiente)
  r.bc(2, 1.998f, 1.0f); twin_check(r, false, post05::Reason::E_OC2FrameEarly);
  r.bc(1, 0.0f, 1.0f); twin_check(r, false, post05::Reason::E_OC2FrameEarly);
  r.brake(0.0f); twin_check(r, false, post05::Reason::E_OC2StartupTick);                // A5-1: setBck en este tick
  r.brake(1.0f); twin_check(r, false, post05::Reason::E_OC2StartupTick);
  r.brake(1.5f); twin_check(r, false, post05::Reason::E_OC2StartupTick);
  r.brake(-1.5f); twin_check(r, false, post05::Reason::E_OC2StartupTick);
  r.brake(1.6f); twin_check(r, true, post05::Reason::Eligible);                          // frenada: el clip no puede arrancar en este tick
  r.brake(-7.0f); twin_check(r, true, post05::Reason::Eligible);
  r.brake(20.0f); twin_check(r, true, post05::Reason::Eligible);
  r.brake(NAN); twin_check(r, false, post05::Reason::E_OC2AnimUnverified);
  r.bc(1, 30.0f, 1.15f); r.an.fstate = 1; twin_check(r, false, post05::Reason::E_OC2ClipEnding);   // isStop: fin de clip (cadena Black) nativo
  r.bc(2, 46.999f, 0.0f); twin_check(r, false, post05::Reason::E_OC2ClipEnding);
  // animacion no autenticada
  r.bc(1, 10, 1.15f); r.an.ok = false; twin_check(r, false, post05::Reason::E_OC2AnimUnverified);
  r.bc(1, 10, 1.15f); r.an.attr = 2; twin_check(r, false, post05::Reason::E_OC2AnimUnverified);
  r.bc(1, 10, 1.15f); r.an.end = 47; twin_check(r, false, post05::Reason::E_OC2AnimUnverified);   // estado 1 con el fin de C
  r.bc(2, 10, 1.15f); r.an.end = 42; twin_check(r, false, post05::Reason::E_OC2AnimUnverified);
  for (float bad : {1.3f, 1.5f, 0.5f, -1.0f, 1.1499f, 3.0f}) { r.bc(1, 10, bad); twin_check(r, false, post05::Reason::E_OC2AnimUnverified); }
  r.bc(1, NAN, 1.15f); twin_check(r, false, post05::Reason::E_OC2AnimUnverified);
  r.bc(1, 42.0f, 1.15f); twin_check(r, false, post05::Reason::E_OC2AnimUnverified);   // frame >= fin
  r.bc(1, -0.5f, 1.15f); twin_check(r, false, post05::Reason::E_OC2AnimUnverified);
  r.bc(3, 10, -1.0f); r.an.end = 42; twin_check(r, false, post05::Reason::PreStateNotWhitelisted);  // ATTACK 3 (rebote por guardia)
}
static void a03_black() {
  Fake b(103, 0x9600, NameKind::E_OC2, 3); b.bc(1, 12, 1.15f);
  twin_check(b, true, post05::Reason::Eligible);                                      // Black 3/3 (furia arranca en el POST de Expansion)
  Fake f(104, 0x9800, NameKind::E_OC2, 6); f.bc(2, 12, 2.0f);
  twin_check(f, true, post05::Reason::Eligible);                                      // Black en furia 6/6
  // secuencia 3 -> 6 -> 6 entre llamadas (Expansion escribe tras damage_check): admision estable, perfiles contados por separado
  Hx h; Fake k(105, 0x9a00, NameKind::E_OC2, 3); h.spawn(k);
  k.bc(1, 10, 1.15f); CHECK(strike(h, k, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && h.dec.back().profile == 2);
  k.f.atp = 6;  // BLACK_FURY_START en el POST de execute
  for (int i = 0; i < 30; ++i) h.call(k);
  k.bc(2, 14, 2.0f); CHECK(strike(h, k, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && h.dec.back().profile == 3);
  CHECK(h.rx.c.applied_by_profile[2] == 1 && h.rx.c.applied_by_profile[3] == 1 && h.rx.c.applied == 2);
  const int s = h.slot_of(105); CHECK(s >= 0 && !h.pe.px.entry(static_cast<unsigned>(s)).ever_frozen);
  // potencia cambiada DENTRO de la llamada (escritor desconocido): SKIP_UNSAFE, nada escrito
  Fake u(106, 0x9c00, NameKind::E_OC2, 3); u.bc(1, 12, 1.15f); u.atp_in_call = 6;
  { Hx hh; hh.spawn(u); CHECK(strike(hh, u, Hx::P(1)) && hh.last_reason() == post05::Reason::PowerChangedInCall && hh.writes == 0 && u.mode == 5); }
  // perfil fuera de {2,3,6}: atp 1 sobre E_OC2 (Blue DEVF sobre recurso E_OC2) diferido
  Fake p1(107, 0x9e00, NameKind::E_OC2, 1); p1.bc(1, 12, 1.15f);
  twin_check(p1, false, post05::Reason::E_OC2ProfileDeferred);
  // animacion cambiada dentro de la llamada: SKIP_UNSAFE
  Fake an(108, 0xa000); an.bc(1, 12, 1.15f); an.anim_in_call = true;
  { Hx hh; hh.spawn(an); CHECK(strike(hh, an, Hx::P(1)) && hh.last_reason() == post05::Reason::PostAuxUnexpected && hh.writes == 0); }
}
static void a04_common_exclusions() {
  auto one = [](Fake a, std::uintptr_t c, post05::Reason want) {
    Hx h; h.spawn(a); const bool has = strike(h, a, c);
    CHECK(has && h.last_reason() == want && h.writes == 0);
    audit_all(h);
  };
  Fake r(110, 0xa200); r.bc(1, 12, 1.15f);
  one(r, Hx::P(3), post05::Reason::FamilyNotSword);  // bola de hierro (BIG_DAMAGE)
  one(r, Hx::P(4), post05::Reason::FamilyNotSword);  // bumeran
  one(r, Hx::P(5), post05::Reason::FamilyNotSword);  // flecha
  one(r, Hx::P(9), post05::Reason::FamilyNotSword);  // escudo
  one(r, Hx::P(6), post05::Reason::FamilyNotSword);
  { Fake l = r; l.health = 15; one(l, Hx::P(1), post05::Reason::Lethal); }
  { Fake s = r; s.spl = true; one(s, Hx::P(1), post05::Reason::BigDamage); }
  { Fake j = r; j.jump_cancel = true; one(j, Hx::P(1), post05::Reason::CooldownNotOrdinary); }
  { Fake c3 = r; c3.cut = 3; one(c3, Hx::P(1), post05::Reason::PostStateNotOrdinary); }
  // umbral y latch: 3 elegibles, el 4.o cruza 75->100, despues latched
  Hx h; Fake t(111, 0xa400); h.spawn(t);
  for (int i = 0; i < 3; ++i) { t.bc(1, 10, 1.15f); CHECK(strike(h, t, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible); }
  t.bc(1, 10, 1.15f); CHECK(strike(h, t, Hx::P(1)) && h.last_reason() == post05::Reason::CrossesThreshold && t.mode == 5);
  t.bc(1, 10, 1.15f); CHECK(strike(h, t, Hx::P(1)) && h.last_reason() == post05::Reason::NotWeighted && t.mode == 5);  // latch: POST04 no pondera
  CHECK(h.writes == 3);
  // no confirmado / Finish (cutdown): sin decision o NOT_CONFIRMED, sin escritura
  Hx h2; Fake d(112, 0xa600); h2.spawn(d); d.cutdown = true; d.bc(1, 10, 1.15f); strike(h2, d, Hx::P(1));
  CHECK(h2.writes == 0 && d.mode == 9);
  // PRE no ATTACK (DAMAGE en curso)
  Hx h3; Fake q(113, 0xa800); h3.spawn(q); q.bc(1, 10, 1.15f);
  while (q.cooldown > 0) h3.call(q);
  q.mode = 5; q.state = 1; q.hit_with(1, Hx::P(1)); h3.call(q); clear_tg(q);
  CHECK(h3.last_reason() == post05::Reason::PreModeNotAttack && h3.writes == 0);
  audit_all(h); audit_all(h2); audit_all(h3);
}
static void a05_policy_and_consistency() {
  for (post05::Reason r : {post05::Reason::E_OC2SuppressionDeferred, post05::Reason::E_OC2ProfileDeferred, post05::Reason::E_OC2AnimUnverified,
                           post05::Reason::E_OC2StartupTick, post05::Reason::E_OC2FrameEarly, post05::Reason::E_OC2ClipEnding})
    CHECK(post05::decision_of(r) == post05::Decision::AllowVanilla);
  CHECK(post05::decision_of(post05::Reason::SessionDisabled) == post05::Decision::SkipUnsafe && post05::kReasonCount == 38);
  CHECK(!std::strcmp(post05::name(post05::Reason::E_OC2FrameEarly), "E_OC2_FRAME_EARLY") && !std::strcmp(post05::name(post05::Reason::SessionDisabled), "SESSION_DISABLED"));
  // politica Deferred (por defecto = 0.5.1): todo E_OC2 diferido, incluso maduro
  { Hx h(post05::E2Policy::Deferred); Fake r(120, 0xb000); h.spawn(r); r.bc(1, 12, 1.15f);
    CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::E_OC2SuppressionDeferred && h.writes == 0 && r.mode == 5); }
  // senales mixtas: el ledger dice E_OC2 pero el recurso leido POST no (o al reves): diferido
  { Hx h; Fake r(121, 0xb200); h.spawn(r); r.bc(1, 12, 1.15f); CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible);
    post05::DecisionInput in; in.rep = h.reps.back(); in.outc = h.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC2_CANDIDATE; in.life = Life::Active;
    in.rep.event = 1000;
    in.rep.aux_post.resource = static_cast<std::uint8_t>(NameKind::E_OC); CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
    in.rep.event = 1001; in.rep.aux_post.resource = static_cast<std::uint8_t>(NameKind::E_OC2); in.rep.aux_pre.valid = 0;
    CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
    in.rep.event = 1002; in.rep.aux_pre.valid = 1; in.adm = Admission::E_OC_CANDIDATE;   // ledger E_OC, recurso E_OC2
    CHECK(h.rx.decide(in).reason == post05::Reason::E_OC2SuppressionDeferred);
    in.rep.event = 1003; in.adm = Admission::UNKNOWN_UNSAFE; in.rep.aux_pre.resource = in.rep.aux_post.resource = static_cast<std::uint8_t>(NameKind::E_OC);
    CHECK(h.rx.decide(in).decision != post05::Decision::SuppressEligible); }
  // E_OC: la fase E_OC2 NO se aplica; mismas decisiones con Deferred y MatureBC en golpes aleatorios (incluye fases que E_OC2 excluiria)
  unsigned same = 0;
  for (unsigned seed = 1; seed <= 400; ++seed) {
    Hx a(post05::E2Policy::Deferred), b(post05::E2Policy::MatureBC);
    Fake x(130, 0xc000, NameKind::E_OC, 1 + static_cast<int>(seed % 2)), y = x;
    a.spawn(x); b.spawn(y);
    const int st = static_cast<int>(seed % 4);
    if (st == 0) { x.brake(static_cast<float>(seed % 3)); } else if (st == 3) { x.state = 3; } else { x.bc(st, static_cast<float>(seed % 40), (seed % 5) ? 1.0f : 1.3f); }
    if (seed % 7 == 0) x.an.ok = false;
    x.cut = (seed % 11 == 0) ? 3 : 0; y = x;
    strike(a, x, Hx::P(1)); strike(b, y, Hx::P(1));
    if (a.dec.size() == b.dec.size() && !a.dec.empty() && a.dec.back().reason == b.dec.back().reason && a.writes == b.writes && x.mode == y.mode && x.state == y.state) ++same;
  }
  CHECK(same == 400);
}
static void a06_multi_actor_and_reuse() {
  Hx h;
  Fake r(140, 0xd000), b(141, 0xd200, NameKind::E_OC2, 6), e(142, 0xd400, NameKind::E_OC, 1);
  h.spawn(r); h.spawn(b); h.spawn(e);
  r.bc(1, 10, 1.15f); b.bc(2, 20, 2.0f); e.bc(1, 5, 1.0f);
  CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible);
  CHECK(strike(h, b, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible);
  CHECK(strike(h, e, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible);
  r.brake(0.0f); CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::E_OC2StartupTick && r.mode == 5);
  CHECK(h.posture(140) == 50 && h.posture(141) == 25 && h.posture(142) == 25 && h.writes == 3);
  CHECK(h.rx.c.applied_by_profile[0] == 1 && h.rx.c.applied_by_profile[1] == 1 && h.rx.c.applied_by_profile[3] == 1);
  // borrado y reutilizacion del id con otro objeto: postura nueva, sin herencia
  h.w.present.erase(140);
  for (int i = 0; i < 400; ++i) h.update();
  Fake r2(140, 0xe000); h.spawn(r2);
  for (int i = 0; i < 3; ++i) h.call(r2);
  r2.bc(2, 30, 1.15f); CHECK(strike(h, r2, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && h.posture(140) == 25);
  // duplicado del mismo evento: no es una decision nueva
  const std::size_t n = h.dec.size();
  post05::DecisionInput in; in.rep = h.reps.back(); in.outc = h.pe.last; in.ledger_valid = true; in.adm = Admission::E_OC2_CANDIDATE; in.life = Life::Active;
  CHECK(!h.rx.decide(in).counted && h.rx.c.duplicates == 1 && h.dec.size() == n);
  audit_all(h);
  CHECK(h.rx.c.e2_phase[static_cast<unsigned>(post05::E2Phase::BcMature)] == 3 && h.rx.c.e2_phase[static_cast<unsigned>(post05::E2Phase::Startup)] == 1);
}
// Ninguna razon de SKIP_UNSAFE/ALLOW_VANILLA se convierte en ELIGIBLE; ELIGIBLE implica TODAS las condiciones (predicado escrito a mano).
static void a07_eligible_predicate() {
  static const int kAtps[5] = {1, 2, 3, 6, 4};
  static const float kRates[5] = {1.0f, 1.15f, 2.0f, 0.0f, 1.3f};
  std::uint64_t s = 0x9e3779b97f4a7c15ull;
  auto rnd = [&]() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; };
  unsigned elig = 0, e2_elig = 0;
  post05::Reaction rx; rx.e2_policy = post05::E2Policy::MatureBC;
  for (unsigned i = 0; i < 6000; ++i) {
    post05::DecisionInput in;
    HitReport& r = in.rep;
    r.event = i + 1; r.slot = 3; r.generation = 7; r.id = 9;
    r.j.kind = (rnd() % 10) ? HitKind::Confirmed : HitKind::FinishUnexpected; r.j.elig = (rnd() % 10) ? Eligibility::Confirmed : Eligibility::Undetermined;
    r.at_valid = rnd() % 10; r.fam.family = (rnd() % 6) ? Family::Sword : Family::Arrow; r.j.lethal = !(rnd() % 12);
    r.pre.mode = (rnd() % 6) ? 4 : 5; r.pre.state = static_cast<int>(rnd() % 4); r.post.mode = (rnd() % 8) ? 5 : 6; r.post.state = static_cast<int>(rnd() % 4);
    r.post.health = (rnd() % 10) ? 100 : 1; r.post.cooldown = (rnd() % 8) ? 10 : 3; r.pre.cooldown = (rnd() % 12) ? 0 : 1; r.post.old_mode = (rnd() % 12) ? r.pre.mode : 7;
    r.life = Life::Active; r.col.deref_ok = rnd() % 15;
    const bool e2 = rnd() % 2;
    const std::uint8_t res = static_cast<std::uint8_t>(e2 ? NameKind::E_OC2 : NameKind::E_OC);
    for (NativeAux* a : {&r.aux_pre, &r.aux_post}) { a->valid = (rnd() % 20) != 0; a->set_bits = kAllSetBits; a->resource = res; a->atp = static_cast<std::int8_t>(e2 ? kAtps[rnd() % 5] : 1); }
    r.aux_post.atp = (rnd() % 15) ? r.aux_pre.atp : static_cast<std::int8_t>(6);
    NativeAux& a = r.aux_pre;
    a.anim_ok = rnd() % 15; a.anim_attr = (rnd() % 10) ? 0 : 2; a.anim_state = (rnd() % 10) ? 0 : 1;
    a.anim_end = (r.pre.state == 2) ^ !(rnd() % 10) ? 47 : 42; a.anim_rate = kRates[rnd() % 5];
    a.anim_frame = static_cast<float>(rnd() % 50) * 0.5f; a.speed = static_cast<float>(static_cast<int>(rnd() % 9) - 4);
    r.aux_post.anim_ok = a.anim_ok; r.aux_post.anim_attr = a.anim_attr; r.aux_post.anim_state = a.anim_state; r.aux_post.anim_end = a.anim_end;
    r.aux_post.anim_rate = a.anim_rate; r.aux_post.anim_frame = (rnd() % 20) ? a.anim_frame : a.anim_frame + 1; r.aux_post.speed = a.speed;
    in.outc.applied = rnd() % 10; in.outc.event = r.event; in.outc.slot = r.slot; in.outc.generation = r.generation;
    in.outc.before = static_cast<int>(rnd() % 4) * 25; in.outc.after = in.outc.before + 25; in.outc.crossed = in.outc.after >= 100;
    in.ledger_valid = rnd() % 15; in.life = Life::Active;
    in.adm = e2 ? ((rnd() % 10) ? Admission::E_OC2_CANDIDATE : Admission::UNKNOWN_UNSAFE) : Admission::E_OC_CANDIDATE;
    const post05::DecisionResult d = rx.decide(in);
    CHECK(post05::decision_of(d.reason) == d.decision);
    if (d.decision != post05::Decision::SuppressEligible) continue;
    ++elig;
    // predicado independiente
    bool ok = r.j.kind == HitKind::Confirmed && r.at_valid && r.fam.family == Family::Sword && !r.j.lethal && r.post.health > 1 && r.post.mode == 5 &&
              r.post.cooldown == 10 && in.outc.applied && in.outc.after < 100 && r.pre.mode == 4 && r.pre.state <= 2 && r.post.state <= 2 &&
              r.post.old_mode == r.pre.mode && r.pre.cooldown == 0 && r.col.deref_ok && r.aux_pre.valid && r.aux_post.valid && in.ledger_valid &&
              r.aux_pre.atp == r.aux_post.atp;
    if (e2) {
      ++e2_elig;
      const bool phase_ok = r.pre.state == 0 ? (a.speed > 1.5f || a.speed < -1.5f)
                                             : (a.anim_ok && a.anim_attr == 0 && a.anim_state == 0 && a.anim_end == (r.pre.state == 1 ? 42 : 47) &&
                                                a.anim_rate != 0.0f && a.anim_rate != 1.3f && a.anim_frame < a.anim_end &&
                                                (a.anim_frame >= 2.0f || a.anim_rate == 1.15f || a.atp == 3));   // 0.5.3 (entorno Possible)
      ok = ok && in.adm == Admission::E_OC2_CANDIDATE && (a.atp == 2 || a.atp == 3 || a.atp == 6) && phase_ok && r.aux_post.anim_frame == a.anim_frame;
    }
    CHECK(ok);
  }
  CHECK(elig >= 20 && e2_elig >= 10);  // no vacuo
  std::printf("eligible_predicate: decisions=6000 eligible=%u e2_eligible=%u\n", elig, e2_elig);
}
static void a08_format() {
  Hx h; Fake r(150, 0xf000); h.spawn(r); r.bc(1, 12.5f, 1.15f);
  CHECK(strike(h, r, Hx::P(1)));
  char d[kLogLineMax];
  CHECK(post05::fmt_decision(d, sizeof(d), h.reps.back(), h.pe.last, h.dec.back(), h.app.back()) > 0);
  CHECK(std::strstr(d, "decision=SUPPRESS_ELIGIBLE reason=ELIGIBLE") && std::strstr(d, "action=SUPPRESSION_APPLIED resource=E_OC2 atp=2>2 admission=E_OC2_CANDIDATE") &&
        std::strstr(d, " phase=BC_MATURE frame=12.50 rate=1.15 speed=0.0"));
  CHECK(!std::strstr(d, "0x"));
  char line[kLogLineMax * 4];
  post05::Reaction maxr = h.rx; for (auto& v : maxr.c.by_reason) v = UINT64_MAX; for (auto& v : maxr.c.e2_phase) v = UINT64_MAX;
  for (auto& v : maxr.c.applied_by_profile) v = UINT64_MAX;
  const int n = post05::fmt_summary(line, sizeof(line), maxr, "PERIODIC");
  CHECK(n > 0 && n < static_cast<int>(sizeof(line)) && std::strstr(line, "e2_policy=MATURE_BC_OR_BRAKE") && std::strstr(line, "applied_by_profile(e_oc/atp2/atp3/atp6)="));
  CHECK(post05::fmt_conservation(line, sizeof(line), h.rx, "SHUTDOWN") > 0 && std::strstr(line, "ok=1") && std::strstr(line, "r7_profiles=1") && std::strstr(line, "r8_e2_phases=1") && std::strstr(line, "v=0.5.4"));
}

// =====================================================================================================================================
// Parte B: gemelos con el codigo REAL de Expansion (CP21)
// =====================================================================================================================================
namespace ra = devf_red_ai;
namespace ba = devf_black_ai;
namespace dy = devf_dyn;
enum class Kind { NativeRed, Red, RedX2, Black, BlackFury };
enum class Perturb { None, NotOwned, PoseInvalid, NotRegistered, StaleFrame };
struct Sim {  // modelo nativo: daE_OC_c::executeAttack + J3DFrameCtrl::update (attr 0 = once, 2 = loop)
  int mode = 4, state = 0, bck = 0x1c, health = 400, atp = 2;
  float speed = 0, frame = 3, rate = 1;
  std::uint8_t attr = 2, fstate = 0;
  std::int16_t end = 40;
  bool stopped() const { return (fstate & 1) != 0 || rate == 0.0f; }
};
struct Out {  // trayectoria observable de un tick
  int mode, state, bck; float frame, rate, speed; std::uint8_t fstate;
  bool rate_write; float rate_value; bool sw0, sw1; float c0x, c0y, c1x, r0, r1; int chip;
  bool force, zero, set_mode; int cmd_mode, cmd_state;
  bool operator==(const Out&) const = default;
};
struct Exp {
  Kind kind; ra::State rs; ba::State bs; ba::Rooms rooms; devf_cp19f2::ChipBlow chip; dy::Ledger L; dy::Entry* e = nullptr; int identity = 0;
  struct Io { std::uint8_t p[2] = {3, 3}; std::uint8_t get(int i) const { return p[i]; } void set(int i, std::uint8_t v) { p[i] = v; } } io;
};
static ra::Point tip_at(float f) { return {std::cos(f * 0.21f) * 90.0f, 60.0f + f, std::sin(f * 0.21f) * 90.0f}; }
static ra::Point hilt_at(float f) { const ra::Point t = tip_at(f); return {t.x * 0.45f, t.y - 10.0f, t.z * 0.45f}; }
static ra::Actor red_actor(const Sim& s, bool hit, bool post) {
  ra::Actor a; a.id = 7; a.gen = 3; a.variant = ra::Variant::Red; std::strcpy(a.stage, "F_SP104"); a.room = 1; a.layer = 0; a.mode = s.mode; a.state = s.state;
  a.health = s.health; a.ready = true; a.hit = hit; a.range = 0; a.speed = s.speed; a.b = s.bck == 5; a.c = s.bck == 6; a.frame = s.frame; a.rate = s.rate;
  a.end = s.end; a.playmode = s.attr; a.stopped = s.stopped();
  if (post) { a.tip = tip_at(s.frame); a.hilt = hilt_at(s.frame); a.tip_r = 45; a.hilt_r = 30; }
  return a;
}
static ba::Actor black_actor(const Sim& s, bool hit, bool post) {
  ba::Actor a; a.id = 8; a.gen = 4; a.stage = 0x1234; a.variant = ba::Variant::Black; a.room = 1; a.mode = s.mode; a.state = s.state; a.health = s.health;
  a.speed = s.speed; a.b = s.bck == 5; a.c = s.bck == 6; a.wait_bck = s.bck == 0x1c; a.frame = s.frame; a.rate = s.rate; a.end = s.end; a.playmode = s.attr;
  a.stopped = s.stopped(); a.hit = hit;
  if (post) { a.tip = tip_at(s.frame); a.hilt = hilt_at(s.frame); a.tip_r = 45; a.hilt_r = 30; }
  return a;
}
static void set_bck(Sim& s, int clip) { s.bck = clip; s.attr = clip == 0x1c ? 2 : 0; s.end = clip == 5 ? 42 : clip == 6 ? 47 : 40; s.frame = 0; s.rate = 1; s.fstate = 0; }
static void play(Sim& s) {  // J3DFrameCtrl::update (once/loop)
  s.fstate = 0; s.frame += s.rate;
  if (s.attr == 0) { if (s.frame >= s.end) { s.frame = s.end - 0.001f; s.rate = 0; s.fstate |= 1; } }
  else { while (s.frame >= s.end) { s.fstate |= 2; s.frame -= s.end; } }
}
struct TickResult { Out o; post05::E2Phase phase; bool attack_pre; int atp_pre; float rate_pre; };
// Un execute completo. hit = golpe de espada suprimido por POST05 en ESTE tick (el modelo solo describe el caso suprimido: modo intacto).
static TickResult tick(Exp& x, Sim& s, bool hit, bool hp_loss, int clip, unsigned t, unsigned contact_frame, bool shield_after, Perturb pz, unsigned pz_tick) {
  TickResult r{}; Out& o = r.o;
  const ra::Point link{40, 0, 40};
  auto re = [](const char*, ra::Cause, const char*) {};
  auto be = [](const char*, const char*) {};
  if (t == pz_tick) {  // estado PRIVADO distinto con el MISMO snapshot (se aplica igual a ambos gemelos)
    for (ra::State* p : {&x.rs, &x.bs.sweep}) {
      if (pz == Perturb::PoseInvalid) { p->pose_valid = p->pose2_valid = false; }
      if (pz == Perturb::NotRegistered) p->registered = false;
    }
    if (pz == Perturb::NotOwned) { x.rs.owned = false; x.bs.owned = false; }
    if (pz == Perturb::NotRegistered) x.bs.registered = false;
    if (pz == Perturb::StaleFrame) { x.rs.last_frame = 1000; x.bs.last_frame = 1000; }
  }
  // ---- Expansion PRE (red_ai_pre / black_ai_pre) ----
  const int hp_pre = s.health;
  if (x.kind == Kind::Red || x.kind == Kind::RedX2) {
    ra::Actor a = red_actor(s, hit, false); ra::Link l;
    const ra::Command c = ra::pre(x.rs, a, l, re);
    o.force = c.force; o.zero = c.zero_speed;
    if (c.force) { s.mode = 4; s.state = 0; }
    if (c.zero_speed) s.speed = 0;
    if (c.rush || c.close) s.speed = c.speed;
  } else if (x.kind == Kind::Black || x.kind == Kind::BlackFury) {
    dy::capture_pre(*x.e, s.health);
    const ba::Actor a = black_actor(s, hit, false); ba::Link l;
    const ba::Command c = ba::pre(x.bs, a, l, x.rooms, x.e->fury, be);
    o.set_mode = c.set_mode; o.cmd_mode = c.mode; o.cmd_state = c.state; o.zero = c.zero_speed;
    if (c.set_mode) { s.mode = c.mode; s.state = c.state; }
    if (c.zero_speed) s.speed = 0;
    if (c.close) s.speed = c.speed;
  }
  // ---- POST05: lecturas PRE de damage_check y gate de fase ----
  NativeAux aux; aux.valid = 1; aux.speed = s.speed; aux.anim_ok = 1; aux.anim_attr = s.attr; aux.anim_state = s.fstate; aux.anim_end = s.end; aux.anim_rate = s.rate; aux.anim_frame = s.frame;
  r.attack_pre = s.mode == 4 && s.state <= 2;
  r.phase = r.attack_pre ? post05::e2_phase(aux, s.state) : post05::E2Phase::NotE2;
  r.atp_pre = s.atp; r.rate_pre = s.rate;
  const float pre_frame = s.frame;
  // ---- damage_check suprimido: solo la vida (y la marca/cooldown, irrelevantes aqui); modo, animacion y speedF intactos ----
  if (hit || hp_loss) s.health -= 20;  // hp_loss sin hit = la MISMA perdida de vida sin bandera de golpe (gemelo de referencia)
  // ---- resto de action(): executeAttack + play ----
  bool at_set = false;
  if (s.mode == 4) {
    if (s.state == 0) {
      if (s.speed > 1.0f) s.speed -= 1.0f; else if (s.speed < -1.0f) s.speed += 1.0f;
      else { s.speed = 0; set_bck(s, clip); s.state = clip == 5 ? 1 : 2; }
    } else if (s.state == 1 || s.state == 2) {
      if (pre_frame >= 14.0f && pre_frame <= 22.0f) at_set = true;
      if (s.stopped()) { set_bck(s, 0x1c); s.mode = 3; s.state = 0; }
    }
  }
  play(s);
  // RedX2: la reaccion (intent) ya estaba seleccionada cuando arranca el golpe nativo
  if (x.kind == Kind::RedX2 && s.mode == 4 && (s.state == 1 || s.state == 2) && s.frame > 0.999f && s.frame < 1.001f && !x.rs.decided) {
    x.rs.intent = true; x.rs.intent_cause = ra::Cause::ChargeSpin; x.rs.intent_start = x.rs.ticks;
  }
  // ---- Expansion POST (dynamic_post -> red_ai_post / black_ai_post -> chip) ----
  const bool contact = s.mode == 4 && static_cast<unsigned>(pre_frame) == contact_frame;
  if (x.kind == Kind::Red || x.kind == Kind::RedX2) {
    ra::Actor a = red_actor(s, hit, true); a.at_set = at_set; a.at_hit = contact; a.hit_link = contact;
    const ra::Rate rt = ra::post(x.rs, a, re);
    o.rate_write = rt.write; o.rate_value = rt.value;
    if (rt.write) s.rate = rt.value;
    ra::Actor rf = a; rf.rate = s.rate; ra::Link here; here.sampled = true; here.pos = link;
    const ra::Sweep sw = ra::sweep(x.rs, rf, here);
    o.sw0 = sw.write0; o.sw1 = sw.write1; o.c0x = sw.c0.x; o.c0y = sw.c0.y; o.c1x = sw.c1.x; o.r0 = sw.r0; o.r1 = sw.r1;
  } else if (x.kind == Kind::Black || x.kind == Kind::BlackFury) {
    dy::Post p; p.post_health = s.health; p.action = s.mode;
    dy::post_update(x.L, *x.e, x.io, p, [](const dy::Entry&) { return true; });
    s.atp = x.io.get(0);
    ba::Actor a = black_actor(s, hit, true); a.at_set = at_set; a.at_hit = contact; a.hit_link = contact;
    const ba::Rate rt = ba::post(x.bs, a, x.e->fury, be);
    o.rate_write = rt.write; o.rate_value = rt.value;
    if (rt.write) s.rate = rt.value;
    ba::Actor rf = a; rf.rate = s.rate;
    x.bs.link_pos_valid = true; x.bs.link_pos = link;
    const ra::Sweep sw = ba::sweep(x.bs, rf);
    o.sw0 = sw.write0; o.sw1 = sw.write1; o.c0x = sw.c0.x; o.c0y = sw.c0.y; o.c1x = sw.c1.x; o.r0 = sw.r0; o.r1 = sw.r1;
    devf_cp19f2::ChipView v; v.id = 8; v.gen = 4; v.seq = x.bs.seq; v.fury = x.e->fury > 0; v.at_set = at_set; v.live = s.health > 0;
    v.shield = shield_after && s.mode == 4 && pre_frame >= 16.0f;
    o.chip = static_cast<int>(devf_cp19f2::chip_step(x.chip, v));
  }
  (void)hp_pre;
  o.mode = s.mode; o.state = s.state; o.bck = s.bck; o.frame = s.frame; o.rate = s.rate; o.speed = s.speed; o.fstate = s.fstate;
  return r;
}
static void init_exp(Exp& x, Sim& s) {
  if (x.kind == Kind::Black || x.kind == Kind::BlackFury) {
    x.e = x.L.record(8, &x.identity, 4, dy::Variant::Black, 3); x.e->owned = true; x.e->native = 2;
    const bool fury = x.kind == Kind::BlackFury;
    x.e->applied = fury ? 6 : 3; x.e->fury = fury ? 300 : 0; x.io.p[0] = x.io.p[1] = x.e->applied; s.atp = x.e->applied;
    x.bs.id = 8; x.bs.gen = 4;
  }
}
struct Run { std::vector<Out> out; std::vector<TickResult> tr; std::vector<int> atp, fury; };
static Run run(Kind k, int clip, float v0, int hit_tick, unsigned contact_frame, bool shield, Perturb pz, unsigned pz_tick, unsigned ticks, int hp_tick = -1) {
  Exp x; x.kind = k; Sim s; s.speed = v0; init_exp(x, s);
  Run R;
  for (unsigned t = 0; t < ticks; ++t) {
    const TickResult tr = tick(x, s, static_cast<int>(t) == hit_tick, static_cast<int>(t) == hp_tick, clip, t, contact_frame, shield, pz, pz_tick);
    R.out.push_back(tr.o); R.tr.push_back(tr); R.atp.push_back(s.atp); R.fury.push_back(x.e ? static_cast<int>(x.e->fury) : 0);
  }
  return R;
}
static unsigned long long g_absent_rule_breaks = 0, g_new_elig = 0, g_new_elig_same = 0, g_vs_nohit_diff = 0, g_fury_effect = 0, g_twins = 0, g_elig_same = 0, g_nonelig_diff = 0, g_nonelig_same = 0, g_diff_startup = 0, g_diff_frame1 = 0, g_power_36 = 0;
static void b01_twins() {
  const unsigned T = 70;
  for (Kind k : {Kind::NativeRed, Kind::Red, Kind::RedX2, Kind::Black, Kind::BlackFury})
    for (int clip : {5, 6})
      for (float v0 : {0.0f, 4.0f})
        for (Perturb pz : {Perturb::None, Perturb::NotOwned, Perturb::PoseInvalid, Perturb::NotRegistered, Perturb::StaleFrame})
          for (unsigned contact : {0u, 17u}) {
            const bool shield = contact != 0;
            const unsigned pz_tick = 14;
            const Run A = run(k, clip, v0, -1, contact, shield, pz, pz_tick, T);
            for (unsigned h = 0; h < T; ++h) {
              const post05::E2Phase ph = A.tr[h].phase;
              if (!A.tr[h].attack_pre) continue;  // POST05 solo decide sobre ATTACK 0/1/2
              const Run B = run(k, clip, v0, static_cast<int>(h), contact, shield, pz, pz_tick, T);
              const Run R = run(k, clip, v0, -1, contact, shield, pz, pz_tick, T, static_cast<int>(h));  // misma perdida de vida, sin bandera de golpe
              ++g_twins;
              CHECK(B.tr[h].phase == ph);  // el golpe no altera la lectura PRE (damage_check aun no ha corrido)
              const bool same = R.out == B.out;
              // 0.5.3: la politica real (e2_allowed). NativeRed modela "Expansion no cargado" (servicio requerido ausente => NATIVE_ONLY).
              const post05::E2Env env = k == Kind::NativeRed ? post05::E2Env::Absent : post05::E2Env::Possible;
              post05::E2Compat why{}, why_abs{};
              const bool elig = post05::e2_allowed(ph, env, A.tr[h].atp_pre, A.tr[h].rate_pre, why);
              const bool elig_if_absent = post05::e2_allowed(ph, post05::E2Env::Absent, A.tr[h].atp_pre, A.tr[h].rate_pre, why_abs);
              if (k != Kind::NativeRed && elig_if_absent && !same) ++g_absent_rule_breaks;   // contraprueba: la regla NATIVE_ONLY con Expansion SI romperia
              if (elig && !post05::e2_phase_eligible(ph)) { ++g_new_elig; if (same) ++g_new_elig_same; }
              if (!(A.out == B.out)) { ++g_vs_nohit_diff; if (elig) { CHECK(k == Kind::Black); ++g_fury_effect; } }
              if (elig) {
                CHECK(same);
                if (same) ++g_elig_same;
                else std::printf("  twin mismatch kind=%d clip=%d v0=%.0f pz=%d hit=%u phase=%s\n", static_cast<int>(k), clip, v0, static_cast<int>(pz), h, post05::name(ph));
                if (k == Kind::Black && B.atp[h] == 6 && A.atp[h] == 3) ++g_power_36;   // 3->6 en el POST de ESTE tick (tras damage_check)
              } else if (!same) {
                ++g_nonelig_diff;
                if (ph == post05::E2Phase::Startup) ++g_diff_startup;
                if (ph == post05::E2Phase::FrameEarly) ++g_diff_frame1;
              } else {
                ++g_nonelig_same;
              }
            }
          }
  std::printf("cp21_twins: twins=%llu eligible_identical=%llu nonelig_differs=%llu (startup=%llu frame1=%llu) nonelig_identical=%llu black_power_3to6_in_post=%llu "
              "differs_from_no_hit=%llu (eligible: black_fury_start_effect=%llu)\n",
              g_twins, g_elig_same, g_nonelig_diff, g_diff_startup, g_diff_frame1, g_nonelig_same, g_power_36, g_vs_nohit_diff, g_fury_effect);
  std::printf("cp21_twins_053: newly_eligible=%llu identical=%llu native_only_rule_would_break_with_expansion=%llu\n", g_new_elig, g_new_elig_same, g_absent_rule_breaks);
  CHECK(g_new_elig >= 100 && g_new_elig_same == g_new_elig && g_absent_rule_breaks >= 10);
  // no vacuidad: los contraejemplos A5 existen en el codigo real y el gate los excluye; hay muchos ticks elegibles cubiertos
  CHECK(g_elig_same >= 2000 && g_diff_startup >= 20 && g_diff_frame1 >= 10 && g_power_36 >= 50);
}
// Black 3->6 en detalle: la potencia la escribe el POST de Expansion (no damage_check); el golpe en curso no cambia de rate, sin chip, sin x2.
static void b02_black_fury_start() {
  const unsigned T = 70;
  const Run A = run(Kind::Black, 5, 0, -1, 17, true, Perturb::None, 999, T);
  unsigned hit = 0;
  for (unsigned t = 0; t < T; ++t) if (A.tr[t].phase == post05::E2Phase::BcMature && A.out[t].frame > 10) { hit = t; break; }
  CHECK(hit > 0);
  const Run B = run(Kind::Black, 5, 0, static_cast<int>(hit), 17, true, Perturb::None, 999, T);
  CHECK(A.atp[hit] == 3 && B.atp[hit] == 6 && (hit == 0 || B.atp[hit - 1] == 3));  // POST05 lee 3 en PRE y POST de damage_check: no es POWER_CHANGED_IN_CALL
  CHECK(B.fury[hit] == 300 && A.fury[hit] == 0);
  bool charged = false;
  for (unsigned t = 0; t < T; ++t) { CHECK(A.out[t] == B.out[t]); if (B.out[t].chip == static_cast<int>(devf_cp19f2::ChipOp::Charge)) charged = true; }
  CHECK(!charged);  // golpe empezado fuera de furia: el chip de escudo no se aplica (NotFury), igual que sin golpe
  // en furia 6/6 el chip se aplica UNA vez por golpe, con o sin golpe suprimido
  const Run F = run(Kind::BlackFury, 6, 0, -1, 17, true, Perturb::None, 999, T);
  unsigned charges = 0, dups = 0;
  for (const Out& o : F.out) { charges += o.chip == static_cast<int>(devf_cp19f2::ChipOp::Charge); dups += o.chip == static_cast<int>(devf_cp19f2::ChipOp::Duplicate); }
  CHECK(charges == 1 && dups >= 1);
  // Black 3/3 golpeado en la FRENADA de ATTACK 0: la furia empieza antes del clip; el golpe nace como golpe de furia normal de Expansion
  // (decidido en el tick limpio de arranque, x2 escrito una sola vez en frame 2, chip como mucho una vez). Es el efecto de la vida perdida, no de POST05.
  for (int clip : {5, 6}) {
    const Run G = run(Kind::Black, clip, 4, 1, 17, true, Perturb::None, 999, T);
    CHECK(G.tr[1].phase == post05::E2Phase::Brake && G.fury[1] == 300 && G.atp[1] == 6);
    unsigned x2 = 0, other = 0, ch = 0;
    for (const Out& o : G.out) { if (o.rate_write) (o.rate_value == 2.0f ? x2 : other) += 1; ch += o.chip == static_cast<int>(devf_cp19f2::ChipOp::Charge); }
    CHECK(x2 == 1 && other == 0 && ch <= 1);
  }
}
// Contraejemplos A5 explicitos (codigo real): el gate los clasifica como no elegibles.
static void b03_a5_counterexamples() {
  for (Kind k : {Kind::RedX2, Kind::BlackFury}) {
    const Run A = run(k, 5, 0, -1, 0, false, Perturb::None, 999, 60);
    int t1 = -1; for (unsigned t = 0; t < A.tr.size(); ++t) if (A.tr[t].phase == post05::E2Phase::FrameEarly) { t1 = static_cast<int>(t); break; }
    CHECK(t1 > 0);
    const Run B = run(k, 5, 0, t1, 0, false, Perturb::None, 999, 60);
    CHECK(!(A.out == B.out));   // A5-2: con golpe en frame 1 el x2 no se aplica (el clip sigue a x1)
    float ra_ = 0, rb_ = 0; for (unsigned t = static_cast<unsigned>(t1) + 1; t < 60; ++t) if (A.out[t].mode == 4) { ra_ = A.out[t].rate; rb_ = B.out[t].rate; break; }
    CHECK(ra_ == 2.0f && rb_ == 1.0f);
  }
  for (Kind k : {Kind::Red, Kind::Black}) {
    const Run A = run(k, 6, 0, -1, 0, false, Perturb::None, 999, 60);
    CHECK(A.tr[0].phase == post05::E2Phase::Startup);
    const Run B = run(k, 6, 0, 0, 0, false, Perturb::None, 999, 60);
    CHECK(!(A.out == B.out) && A.out[0].rate_write && !B.out[0].rate_write);  // A5-1: sin x1.15 si el golpe cae en el tick de arranque
  }
}

// ---- 0.5.4 (politica GENERAL): con Expansion, el golpe en el tick de arranque o en el frame 1 de un x2 TAMBIEN se suprime. Codigo real de CP21:
// ese golpe pierde su x1.15/x2 (velocidad nativa), pero queda BIEN FORMADO: a lo sumo una escritura de rate por golpe, ningun x2 escrito despues,
// ningun barrido de esferas sin x2 poseido, chip <= 1, y el clip termina en su fin nativo (sin bucle ni corte).
static void b04_general_degraded_wellformed() {
  unsigned checked = 0, lost = 0;
  for (Kind k : {Kind::Red, Kind::RedX2, Kind::Black, Kind::BlackFury})
    for (int clip : {5, 6})
      for (float v0 : {0.0f, 4.0f}) {
        const Run A = run(k, clip, v0, -1, 17, true, Perturb::None, 999, 70);
        for (unsigned h = 0; h < 70; ++h) {
          const post05::E2Phase ph = A.tr[h].phase;
          if (ph != post05::E2Phase::Startup && ph != post05::E2Phase::FrameEarly) continue;
          const Run B = run(k, clip, v0, static_cast<int>(h), 17, true, Perturb::None, 999, 70);
          ++checked;
          unsigned rate_writes = 0, x2_after = 0, sweeps = 0, charges = 0, ended = 0;
          for (unsigned t = h; t < 70; ++t) {
            const Out& o = B.out[t];
            if (o.rate_write && o.rate_value != 1.0f) { ++rate_writes; if (o.rate_value == 2.0f) ++x2_after; }
            if (o.sw0 || o.sw1) ++sweeps;
            charges += o.chip == static_cast<int>(devf_cp19f2::ChipOp::Charge);
            if (t > h && o.mode == 3 && B.out[t - 1].mode == 4) ++ended;
          }
          if (!(A.out == B.out)) ++lost;
          CHECK(rate_writes <= 1 && charges <= 1 && ended == 1);
          if (ph == post05::E2Phase::FrameEarly && (k == Kind::RedX2 || k == Kind::BlackFury)) CHECK(x2_after == 0 && sweeps == 0);  // x2 perdido, sin barrido huerfano
        }
      }
  std::printf("cp21_general_degraded: start_or_frame1_hits=%u rate_lost=%u (all well formed)\n", checked, lost);
  CHECK(checked >= 16 && lost >= 8);
}
// ---- 0.5.3: entorno de la IA de variantes y fixture del evento real (sesion aislada POST05+Zora Swim, Red 163 gen 9 evento 2) ----
static Fake iso_event2_setup(Hx& h, std::uint32_t id) {
  Fake r(id, 0x20000 + id * 0x100); h.spawn(r);
  // evento 1 real: FIND estado 3, 0->25, vanilla (PRE_MODE_NOT_ATTACK)
  r.state = 3; r.an = Anim{}; r.an.attr = 2; r.an.end = 40; r.an.frame = 4; r.an.rate = 0;
  while (r.cooldown > 0) h.call(r);
  r.mode = 3; r.hit_with(1, Hx::P(1)); h.call(r); clear_tg(r);
  CHECK(h.last_reason() == post05::Reason::PreModeNotAttack && h.posture(id) == 25);
  // evento 2 real: ATTACK estado 2 (C, fin 47), frame 1.00, rate 1.00, speed 0, espada no letal, 25->50
  r.bc(2, 1.0f, 1.0f);
  return r;
}
static void a09_env_and_real_event() {
  {  // NATIVE_ONLY (POST05 + Zora Swim): el evento 2 ya no interrumpe el ataque
    Hx h(post05::E2Policy::MatureBC, post05::E2Env::Absent); Fake r = iso_event2_setup(h, 163);
    Fake off = r; Hx ho(post05::E2Policy::MatureBC, post05::E2Env::Absent); ho.rx.close_session();
    CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::Eligible && h.last_apply() == post05::Apply::Applied);
    CHECK(r.mode == 4 && r.state == 2 && h.posture(163) == 50 && h.dec.back().phase == post05::E2Phase::FrameEarly && h.dec.back().compat == post05::E2Compat::NativeOnly);
    char d[kLogLineMax]; CHECK(post05::fmt_decision(d, sizeof(d), h.reps.back(), h.pe.last, h.dec.back(), h.app.back()) > 0);
    CHECK(std::strstr(d, "posture=25>50 action=SUPPRESSION_APPLIED resource=E_OC2 atp=2>2") && std::strstr(d, "phase=FRAME_EARLY frame=1.00 rate=1.00") &&
          std::strstr(d, "env=NATIVE_ONLY compat=NATIVE_ONLY_NO_RATE_OWNER"));
    ho.spawn(off); CHECK(strike(ho, off, Hx::P(1)) && rest_of(off) == rest_of(r) && off.mode == 5);   // gemelo sin POST05: solo difieren los 3 campos
    CHECK(h.rx.c.e2_applied_phase[static_cast<unsigned>(post05::E2Phase::FrameEarly)] == 1 && h.rx.audit().ok);
  }
  for (post05::E2Env env : {post05::E2Env::Possible, post05::E2Env::Unknown}) {  // Expansion posible o sin comprobar: mismo snapshot => vanilla
    Hx h(post05::E2Policy::MatureBC, env); Fake r = iso_event2_setup(h, 164);
    CHECK(strike(h, r, Hx::P(1)) && h.last_reason() == post05::Reason::E_OC2FrameEarly && r.mode == 5 && h.writes == 0);
    CHECK(h.dec.back().compat == post05::E2Compat::X2PendingPossible);
  }
  auto one = [](post05::E2Env env, Fake r, post05::Reason want, post05::E2Compat why) {
    Hx h(post05::E2Policy::MatureBC, env); h.spawn(r); const bool has = strike(h, r, Hx::P(1));
    CHECK(has && h.last_reason() == want && h.dec.back().compat == why);
    if (want == post05::Reason::Eligible) CHECK(h.writes == 1 && r.mode == 4); else CHECK(h.writes == 0);
    CHECK(h.rx.audit().ok);
  };
  using R = post05::Reason; using C = post05::E2Compat; const auto A = post05::E2Env::Absent, P = post05::E2Env::Possible;
  Fake r(170, 0x30000);
  r.brake(0.0f);  one(A, r, R::Eligible, C::NativeOnly);  one(P, r, R::E_OC2StartupTick, C::RateDecisionThisTick);
  r.brake(1.5f);  one(A, r, R::Eligible, C::NativeOnly);  one(P, r, R::E_OC2StartupTick, C::RateDecisionThisTick);
  r.brake(-1.5f); one(A, r, R::Eligible, C::NativeOnly);
  r.brake(1.6f);  one(P, r, R::Eligible, C::MatureOrBrake);
  r.bc(1, 41.5f, 0.0f); r.an.fstate = 1; one(A, r, R::Eligible, C::NativeOnly); one(P, r, R::E_OC2ClipEnding, C::ClipEndChain);
  r.bc(1, 1.0f, 1.15f); one(P, r, R::Eligible, C::X115Owned);                 // x1.15 ya poseido por Expansion
  r.bc(1, 1.0f, 2.0f);  one(P, r, R::E_OC2FrameEarly, C::X2PendingPossible);   // rate 2 en frame 1: no es un estado de Expansion conocido
  r.bc(1, 10, 1.3f);    one(A, r, R::E_OC2AnimUnverified, C::NA);              // animacion no autenticada sigue vanilla aunque no haya Expansion
  Fake b(171, 0x30400, NameKind::E_OC2, 3);
  b.bc(2, 1.0f, 1.0f);  one(P, b, R::Eligible, C::BlackBaseNoPending);        // Black 3/3: furia 0 al decidir => nunca x2 pendiente
  b.brake(0.0f);        one(A, b, R::E_OC2StartupTick, C::RateDecisionThisTick);  // potencia 3/6 implica Expansion: la regla NATIVE_ONLY no aplica
  Fake f(172, 0x30800, NameKind::E_OC2, 6);
  f.bc(2, 1.0f, 1.0f);  one(P, f, R::E_OC2FrameEarly, C::X2PendingPossible);  one(A, f, R::E_OC2FrameEarly, C::X2PendingPossible);
  // contador de la secuencia real 0->25->50->75->100 en NATIVE_ONLY: 3 supresiones (incluida la de frame 1) y la 4.a cruza (vanilla)
  Hx h(post05::E2Policy::MatureBC, A); Fake s(173, 0x30c00); h.spawn(s);
  s.bc(2, 1.0f, 1.0f); CHECK(strike(h, s, Hx::P(1)) && h.last_reason() == R::Eligible);
  s.brake(0.5f);       CHECK(strike(h, s, Hx::P(1)) && h.last_reason() == R::Eligible);
  s.bc(1, 20.0f, 1.0f); CHECK(strike(h, s, Hx::P(1)) && h.last_reason() == R::Eligible);
  s.bc(1, 1.0f, 1.0f); CHECK(strike(h, s, Hx::P(1)) && h.last_reason() == R::CrossesThreshold && s.mode == 5);
  s.bc(1, 1.0f, 1.0f); CHECK(strike(h, s, Hx::P(1)) && h.last_reason() == R::NotWeighted && s.mode == 5);
  CHECK(h.writes == 3 && h.rx.audit().ok && h.rx.c.e2_env_decisions[1] == 3);  // cruce y latch se deciden antes del gate de fase
}

int main() {
  a01_red_mature();
  a02_excluded_phases();
  a03_black();
  a04_common_exclusions();
  a05_policy_and_consistency();
  a06_multi_actor_and_reuse();
  a07_eligible_predicate();
  a08_format();
  b01_twins();
  b02_black_fury_start();
  b03_a5_counterexamples();
  a09_env_and_real_event();
  b04_general_degraded_wellformed();
  std::printf("POST05_RB_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
