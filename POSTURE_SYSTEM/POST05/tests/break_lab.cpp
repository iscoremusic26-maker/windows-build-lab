// POST06 (0.6.0) laboratorio: ruptura -> castigo x1.20 FIJO -> caida NATIVA (1 s de inactividad / 3 s maximo) -> suelo (Finish nativo) ->
// levantamiento nativo -> rearme a 0; recuperacion 5/4/3/2/1 por banda de vida. Motor COMPLETO (POST03.1 + POST04 + POST05 general + POST06) en el
// MISMO orden que mod.cpp, sobre un modelo de damage_check + executeDamage + executeBigDamage (d_a_e_oc.cpp:638-786, 1534-1724).
// LAB_PASS = consistencia con ESTE modelo. NO prueba animaciones, colisiones ni Finish en el juego (eso es la prueba humana).
#include <climits>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include "../src/break_core.hpp"

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
enum : std::uint32_t { SWORD = 0x4000000u, ARROW = 0x2000u, IRON = 0x400000u };
enum Mode { WAIT = 0, WALK = 1, TALK = 2, FIND = 3, ATTACK = 4, DAMAGE = 5, BIG = 6, DEATH = 9 };
enum : unsigned { kDamageAnim = 15, kAirTicks = 8, kLandAnim = 20, kDownTimer = 45, kGetUpAnim = 25 };

struct Fake {
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  ActorFacts f;
  int health = 300, mode = FIND, old_mode = WAIT, state = 4, cooldown = 0;
  bool cutdown = false, tg[2] = {false, false}, down_flg = false;
  std::uintptr_t hit[2] = {0, 0}, sel = 0;
  std::int16_t t6c0 = 0, t6c2 = 0;
  bool head_lock = false;
  std::uint8_t set_bits = 0x0F;
  std::uint32_t attention = 0x7;
  int cut = 0, cut_alt = 0, cut_timer = 0, power = 10, anim = 0;
  Fake(std::uint32_t i, std::uintptr_t p, NameKind nk = NameKind::E_OC2, int atp = 2) : id(i), ptr(p) {
    f.profile_ok = f.live = true; f.name = nk; f.status_10000 = nk == NameKind::E_OC; f.atp = atp;
  }
  post05::LiveMode live() const { return post05::LiveMode{mode, state, old_mode, t6c0, t6c2, static_cast<std::uint8_t>(head_lock ? 1 : 0), set_bits}; }
  void put(const post05::LiveMode& t) { mode = t.mode; state = t.state; old_mode = t.old_mode; t6c0 = t.t6c0; t6c2 = t.t6c2; head_lock = t.head_lock != 0; set_bits = t.set_bits; }
};
static void set_action_mode(Fake& a, int m, int st) { a.old_mode = a.mode; a.mode = m; a.state = st; a.t6c0 = a.t6c2 = 0; a.set_bits |= 0x0F; a.head_lock = false; }
static void off_tg_sph(Fake& a) { a.set_bits &= static_cast<std::uint8_t>(~0x05u); a.tg[0] = a.tg[1] = false; }
struct World { std::map<std::uintptr_t, std::uint32_t> types; std::map<std::uint32_t, std::uintptr_t> present; };
static void damage_check_native(Fake& a, World& w) {
  if (a.cutdown) { a.cutdown = false; a.health = 0; set_action_mode(a, DEATH, 1); off_tg_sph(a); return; }   // Finish nativo
  if (a.cooldown) return;
  bool had = false;
  for (int i = 0; i < 2; ++i) if (a.tg[i]) { a.sel = a.hit[i]; had = true; }
  if (!had) return;
  const std::uint32_t t = w.types.at(a.sel);
  a.cooldown = 10;
  int my = 2;
  if (t & M.sword) { my = a.cut; if (my <= 2) { if (a.cut_timer) { a.cut_alt ^= 1; my = a.cut_alt; } else a.cut_alt = my; a.cut_timer = 30; } }
  else if (t & M.arrow) my = 0;
  else if (t & M.iron_ball) { my = 5; a.health += 80; }
  a.health -= a.power;  // cc_at_check (dano efectivo de esta configuracion)
  if (a.health < 0) a.health = 0;
  if (a.health <= 1) { a.attention = 0; if (my < 5) my = 5; }
  if (my >= 5) { set_action_mode(a, BIG, my - 5); off_tg_sph(a); }
  else set_action_mode(a, DAMAGE, my);
}
// resto de action(): executeDamage / executeBigDamage (lo demas no cambia el modo en este modelo)
static void execute_native(Fake& a) {
  if (a.mode == DAMAGE) {
    if (a.state <= 4) { a.state = 5; a.anim = kDamageAnim; }
    else if (--a.anim <= 0) set_action_mode(a, FIND, 0);
  } else if (a.mode == BIG) {
    switch (a.state) {
      case 0: case 1: case 2: off_tg_sph(a); a.state = 3; a.anim = kAirTicks; break;
      case 3: if (--a.anim <= 0) { a.state = 4; a.anim = kLandAnim; } break;
      case 4: if (--a.anim <= 0) { if (a.health < 1) { set_action_mode(a, DEATH, 0); off_tg_sph(a); } else { a.state = 5; a.down_flg = true; a.t6c0 = kDownTimer; } } break;
      case 5: if (a.t6c0 == 0) { a.state = 6; a.down_flg = false; a.anim = kGetUpAnim; } break;
      case 6: if (--a.anim <= 0) set_action_mode(a, FIND, 5); break;
      default: break;
    }
  }
}
static Snapshot snap_of(const Fake& a) {
  Snapshot s; s.health = a.health; s.mode = a.mode; s.state = a.state; s.old_mode = a.old_mode; s.cooldown = a.cooldown; s.cutdown = a.cutdown ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((a.tg[0] ? 1 : 0) | (a.tg[1] ? 2 : 0)); return s;
}
static NativeAux aux_of(const Fake& a) {
  NativeAux x; x.valid = 1; x.t6c0 = a.t6c0; x.t6c2 = a.t6c2; x.head_lock = a.head_lock ? 1 : 0; x.set_bits = a.set_bits;
  x.atp = static_cast<std::int8_t>(a.f.atp); x.resource = static_cast<std::uint8_t>(a.f.name);
  x.anim_ok = 1; x.anim_attr = 2; x.anim_end = 40; x.anim_rate = 1.0f; x.anim_frame = 5.0f; return x;
}
struct Hx {
  post04::PostureEngine pe;
  post05::Reaction rx;
  post06::Breaker br;
  World w;
  std::vector<post06::Action> acts;
  int drops = 0, bonus_writes = 0, rearm_calls = 0, react_writes = 0;
  std::uint64_t drop_exec = 0;
  bool break_writer_ok = true;
  explicit Hx(bool post06_on = true) {
    pe.eng.masks = M; pe.eng.policy = AdmissionPolicy::V2Compat; rx.e2_policy = post05::E2Policy::MatureBC; rx.stagger_policy = post05::StaggerPolicy::General;
    rx.e2_env = post05::E2Env::Absent; br.enabled = post06_on;
    w.types[P(1)] = SWORD; w.types[P(3)] = IRON; w.types[P(5)] = ARROW;
  }
  static std::uintptr_t P(unsigned n) { return 0x100000u + n * 0x200u; }
  void spawn(Fake& a) { w.present[a.id] = a.ptr; }
  // replica de do_pre / do_post / post06_step de mod.cpp
  void tick(Fake& a) {
    if (a.cooldown > 0) --a.cooldown;
    if (a.cut_timer > 0) --a.cut_timer;
    if (a.t6c0 > 0) --a.t6c0;
    PreReads r; r.facts = a.f;
    const bool cand = is_candidate(admit(a.f, pe.eng.policy).adm);
    if (cand) { r.snap = snap_of(a); r.raw0 = a.tg[0] ? a.hit[0] : 0; r.raw1 = a.tg[1] ? a.hit[1] : 0; r.aux = aux_of(a); }
    Events ev; const Observation o = pe.on_pre(a.ptr, a.id, r, ev);
    const int pre_hp = r.snap.health;
    damage_check_native(a, w);
    PostReads p; p.alive = a.mode != DEATH || a.health > 0 || true; p.snap = snap_of(a); p.selected = a.sel; p.aux = aux_of(a);
    Events ev2; HitReport rep;
    const bool has = pe.on_post(a.ptr, p, [&](std::uintptr_t s, std::uint32_t& t) { auto it = w.types.find(s); if (it == w.types.end()) return false; t = it->second; return true; }, ev2, rep);
    if (has) {
      post05::Apply ap = post05::Apply::NotAttempted;
      post05::process(rx, pe, rep, [&a] { return a.live(); }, [&](const post05::LiveMode& t) { ++react_writes; a.put(t); }, ap);
    }
    post04::PItem it; while (pe.px.pop(it)) {}
    // POST06
    if (br.enabled && o.tracked && cand && o.slot >= 0 && pe.eng.obs.ledger.valid(o.slot, o.generation)) {
      const post04::Entry& e = pe.px.entry(static_cast<unsigned>(o.slot));
      if (e.bound && e.generation == o.generation && e.id == a.id && !e.frozen) {   // muerte incluida: POST06 la ve y cierra el ciclo (sin escribir)
        post06::View v; v.slot = o.slot; v.generation = o.generation; v.id = a.id; v.tracked = true; v.pre_hp = pre_hp; v.native_hp = p.snap.health;
        v.mode = a.mode; v.state = a.state; v.hit = has && rep.j.kind == HitKind::Confirmed; v.sword = v.hit && rep.at_valid && rep.fam.family == Family::Sword;
        v.crossed = v.hit && pe.last.event == rep.event && pe.last.crossed; v.latched = e.latched; v.writer_ok = true;
        const post06::Action act = br.step(v);
        bool ok = true;
        if (act.act == post06::Act::Bonus) {
          ok = break_writer_ok && a.health == v.native_hp;
          if (ok) { ++bonus_writes; a.health = act.new_hp; if (act.lethal) a.attention = 0; if (act.convert_to_big) { a.mode = BIG; a.state = 0; } }
        } else if (act.act == post06::Act::Drop) {
          ok = break_writer_ok && post06::policy::drop_mode_safe(a.mode);
          if (ok) { ++drops; drop_exec = br.slot(static_cast<unsigned>(v.slot)).exec; const post05::LiveMode t{BIG, 0, a.mode, 0, 0, 0, post03::kAllSetBits}; a.put(t); }
        } else if (act.act == post06::Act::Rearm) {
          ++rearm_calls; ok = pe.px.rearm(v.slot, v.generation, v.id) || !pe.px.entry(static_cast<unsigned>(v.slot)).latched;
        }
        if (act.act != post06::Act::None) { br.commit(v, act, ok); acts.push_back(act); }
        pe.px.set_regen_rate(v.slot, v.generation, v.id, br.regen_rate(v.slot, a.health));
      }
    }
    execute_native(a);
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
  void frame(Fake& a) { tick(a); update(); }
  void frames(Fake& a, unsigned n) { for (unsigned i = 0; i < n; ++i) frame(a); }
  // Espera al cooldown y golpea en el siguiente tick (los ticks de espera son ticks reales: el reloj del castigo avanza).
  void hit(Fake& a, unsigned type = 1, int cut = 0) {
    while (a.cooldown > 1) frame(a);
    a.cut = cut; a.tg[1] = true; a.hit[1] = P(type);
    frame(a);
    a.tg[0] = a.tg[1] = false; a.cut = 0;
  }
  int slot_of(std::uint32_t id) const {
    for (unsigned i = 0; i < kCapacity; ++i) { const Instance& s = pe.eng.obs.ledger.slot(i); if (s.id == id && (s.life == Life::Active || s.life == Life::DeathSeen)) return static_cast<int>(i); }
    return -1;
  }
  const post04::Entry* entry(std::uint32_t id) const { const int s = slot_of(id); return s < 0 ? nullptr : &pe.px.entry(static_cast<unsigned>(s)); }
  int posture(std::uint32_t id) const { const post04::Entry* e = entry(id); return e && e->bound ? e->posture : -1; }
  post06::Phase phase(std::uint32_t id) const { const int s = slot_of(id); return s < 0 ? post06::Phase::Count : br.slot(static_cast<unsigned>(s)).phase; }
};
static void audit_all(Hx& h) { CHECK(h.rx.audit().ok); CHECK(h.pe.px.audit().ok); CHECK(h.pe.eng.conserved()); }
// Rompe la postura de `a` (3 espadazos protegidos + el 4.o cruza). Devuelve la vida antes del 4.o.
static int break_it(Hx& h, Fake& a) {
  for (int i = 0; i < 3; ++i) { h.hit(a); CHECK(a.mode != DAMAGE && a.mode != BIG); }   // POST05: < 100 sin tambaleo
  const int hp = a.health;
  h.hit(a);
  return hp;
}

// ---- B01-B03: ruptura, castigo x1.20 fijo, caida por inactividad, suelo, levantamiento, rearme, segundo ciclo ----
static void b01_full_cycle() {
  Hx h; Fake a(100, 0x9000); h.spawn(a);
  const int hp_before_break = break_it(h, a);
  CHECK(h.posture(100) == 100 && a.health == hp_before_break - 10);           // golpe de ruptura: dano NORMAL (sin bonus)
  CHECK(a.mode == DAMAGE && h.phase(100) == post06::Phase::Punish);             // reaccion nativa + castigo
  // 3 espadazos de dano base 10 durante el castigo: 12, 12, 12 (nunca 12, 14, 17)
  for (int i = 0; i < 3; ++i) {
    const int before = a.health;
    h.hit(a);
    CHECK(before - a.health == 12);
    CHECK(a.mode == DAMAGE);                                                    // reaccion NATIVA (sin supresion de POST05)
  }
  CHECK(h.br.c.bonus_applied == 3 && h.br.c.bonus_units == 6 && h.bonus_writes == 3);
  // sin mas espadazos: la caida empieza exactamente 30 ejecuciones despues del ultimo
  const int hp_drop = a.health;
  unsigned n = 0;
  while (a.mode != BIG && n < 200) { h.frame(a); ++n; }
  CHECK(a.mode == BIG && h.drops == 1 && h.br.c.drops_inactivity == 1 && a.health == hp_drop);   // caida sin dano, sin golpe fantasma
  CHECK(n <= 31);
  // vuelo -> suelo (Finish posible) -> levantamiento: la postura NO se rearma antes de volver a FIND
  bool saw_down = false, saw_up = false;
  for (unsigned i = 0; i < 300 && a.mode == BIG; ++i) {
    const int st = a.state;   // POST06 ve en ESTA llamada el estado que dejo la ejecucion anterior
    h.frame(a);
    if (st == 5 && a.mode == BIG) { saw_down = true; CHECK(a.down_flg || a.state == 6); CHECK(h.phase(100) == post06::Phase::Down || h.phase(100) == post06::Phase::GettingUp); }
    if (st == 6 || st == 5 || st == 4 || st == 3) CHECK(h.posture(100) == 100);   // sin rearme prematuro
    if (st == 6 && a.mode == BIG) { saw_up = true; CHECK(h.phase(100) == post06::Phase::GettingUp); }
  }
  CHECK(saw_down && saw_up && a.mode == FIND && a.state == 5 && a.health == hp_drop);
  CHECK(h.posture(100) == 100);   // el levantamiento acaba de terminar: el rearme llega en la llamada siguiente (control normal confirmado)
  h.frame(a);
  CHECK(h.phase(100) == post06::Phase::Tracking && h.posture(100) == 0 && !h.entry(100)->latched && h.br.c.rearms == 1);
  // segundo ciclo: de nuevo sin tambaleo < 100 y segunda ruptura posible
  a.mode = FIND; a.state = 4;
  h.frames(a, 5);
  break_it(h, a);
  CHECK(h.phase(100) == post06::Phase::Punish && h.br.c.breaks == 2);
  audit_all(h);
}
// ---- B04: espadazos cada < 1 s: la caida llega a los 3 s ABSOLUTOS, nunca mas tarde ----
static void b04_max_three_seconds() {
  Hx h; Fake a(101, 0x9400); h.spawn(a);
  break_it(h, a);
  const std::uint64_t start = h.br.slot(static_cast<unsigned>(h.slot_of(101))).exec;
  unsigned hits = 0;
  while (a.mode != BIG && hits < 20) { h.frames(a, 8); if (a.mode == BIG) break; h.hit(a); ++hits; }
  const std::uint64_t at = h.drop_exec;
  CHECK(a.mode == BIG && h.br.c.drops_max == 1 && h.br.c.drops_inactivity == 0);
  CHECK(at - start >= 90 && at - start <= 91 && hits >= 4);   // 3 s de ejecuciones propias; si el tick 90 trae un golpe, la caida va en el siguiente
  std::printf("b04: drop_at=%llu hits=%u inact=%llu max=%llu\n", (unsigned long long)(at - start), hits, (unsigned long long)h.br.c.drops_inactivity, (unsigned long long)h.br.c.drops_max);
  audit_all(h);
}
// ---- B05/B06: derribo NATIVO (giro) en la ruptura o en el castigo: sin segunda caida ni impulso falso ----
static void b05_native_knockdowns() {
  { Hx h; Fake a(102, 0x9800); h.spawn(a);
    for (int i = 0; i < 3; ++i) h.hit(a);
    h.hit(a, 1, 6);   // 75->100 con giro: BIG nativo
    CHECK(a.mode == BIG && h.phase(102) == post06::Phase::Falling && h.br.c.breaks_knockdown == 1);
    for (unsigned i = 0; i < 300 && a.mode == BIG; ++i) h.frame(a);
    h.frame(a);
    CHECK(h.drops == 0 && a.mode == FIND && h.posture(102) == 0 && h.br.c.rearms == 1); audit_all(h); }
  { Hx h; Fake a(103, 0x9c00); h.spawn(a);
    break_it(h, a);
    const int before = a.health;
    h.hit(a, 1, 6);   // giro durante el castigo: bonus UNA vez + caida nativa
    CHECK(a.mode == BIG && before - a.health == 12 && h.phase(103) == post06::Phase::Falling);
    for (unsigned i = 0; i < 300 && a.mode == BIG; ++i) h.frame(a);
    CHECK(h.drops == 0 && h.br.c.bonus_applied == 1 && a.mode == FIND); audit_all(h); }
}
// ---- B07: el x1.20 vuelve letal un golpe normalmente no letal: rama letal NATIVA (BIG 0, atencion 0) y muerte al aterrizar ----
static void b07_bonus_lethal() {
  Hx h; Fake a(104, 0xa000); h.spawn(a); a.health = 52;
  break_it(h, a);                    // 52 -> 42 -> 32 -> 22 (protegidos) -> 12 (ruptura)
  CHECK(a.health == 12 && h.phase(104) == post06::Phase::Punish);
  h.hit(a);                          // nativo: 12 -> 2 (no letal, DAMAGE); x1.20: -2 mas -> 0 => letal
  CHECK(a.health == 0 && a.attention == 0 && a.mode == BIG && a.state <= 3 && h.br.c.bonus_lethal == 1);
  for (unsigned i = 0; i < 100 && a.mode == BIG; ++i) h.frame(a);
  h.frame(a);
  CHECK(a.mode == DEATH && h.phase(104) == post06::Phase::Dead && h.br.c.rearms == 0);
  audit_all(h);
}
// ---- B08: Finish nativo en el suelo -> muerte; nunca rearme ----
static void b08_finish() {
  Hx h; Fake a(105, 0xa400); h.spawn(a);
  break_it(h, a);
  for (unsigned i = 0; i < 200 && !(a.mode == BIG && a.state == 5); ++i) h.frame(a);
  h.frame(a);
  CHECK(a.mode == BIG && a.state == 5 && h.phase(105) == post06::Phase::Down);
  a.cutdown = true;  // Link: CUT_TYPE_DOWN (Finish)
  h.frame(a);
  h.frame(a);
  CHECK(a.mode == DEATH && a.health == 0 && h.phase(105) == post06::Phase::Dead && h.br.c.rearms == 0 && h.bonus_writes == 0);
  h.frames(a, 50); CHECK(h.br.c.rearms == 0);
  audit_all(h);
}
// ---- B10/B11: otras familias sin bonus ni renovacion; acumulador de quintos sin escalada ----
static void b10_families_and_fractions() {
  { Hx h; Fake a(106, 0xa800); h.spawn(a);
    break_it(h, a);
    h.frames(a, 15);
    const int before = a.health; h.hit(a, 5);   // flecha: dano normal, sin bonus, no renueva la inactividad
    CHECK(before - a.health == 10 && h.br.c.bonus_applied == 0);
    unsigned n = 0; while (a.mode != BIG && n < 100) { h.frame(a); ++n; }
    CHECK(a.mode == BIG && h.br.c.drops_inactivity == 1 && n <= 20); audit_all(h); }
  { Hx h; Fake a(107, 0xac00); h.spawn(a); a.power = 10;
    break_it(h, a);
    a.power = 3; int extra_total = 0;
    for (int i = 0; i < 5; ++i) { const int b = a.health; h.hit(a); extra_total += (b - a.health) - 3; CHECK(b - a.health == 3 || b - a.health == 4); }
    CHECK(extra_total == 3 && h.br.c.bonus_units == 3 && h.br.c.base_units == 15);   // 15 de dano -> +3 (20 %), sin escalada
    audit_all(h); }
  for (int base : {1, 2, 4, 5, 10}) {
    Hx h; Fake a(108, 0xb000); h.spawn(a); break_it(h, a); a.power = base;
    int total = 0; for (int i = 0; i < 3; ++i) { const int b = a.health; h.hit(a); total += b - a.health; }
    CHECK(total == 3 * base + (3 * base) / 5);
  }
}
// ---- B12: recuperacion 5/4/3/2/1 por banda de vida (tasa exacta, sin deriva) y retardo de 120 ticks ----
static void b12_regen_bands() {
  struct Case { int hp, max; int rate; };
  for (const Case cs : {Case{300, 300, 5}, Case{226, 300, 5}, Case{225, 300, 4}, Case{151, 300, 4}, Case{150, 300, 3}, Case{76, 300, 3}, Case{75, 300, 2},
                        Case{31, 300, 2}, Case{30, 300, 1}, Case{1, 300, 1}, Case{0, 300, 5}, Case{50, 0, 5}, Case{400, 300, 5}})
    CHECK(post06::policy::regen_rate_for(cs.hp, cs.max) == cs.rate);
  for (int rate = 1; rate <= 5; ++rate) {
    Hx h; Fake a(110, 0xb400); h.spawn(a);
    h.hit(a); h.hit(a); h.hit(a);                           // postura 75
    const int s = h.slot_of(110);
    CHECK(h.posture(110) == 75);
    CHECK(h.pe.px.set_regen_rate(s, h.pe.px.entry(static_cast<unsigned>(s)).generation, 110, static_cast<std::uint8_t>(rate)));
    for (int i = 0; i < 125; ++i) h.update();               // retardo: nada antes de 120 ticks elegibles
    const int p0 = h.posture(110);
    CHECK(p0 >= 74);
    for (int i = 0; i < 300; ++i) h.update();                // 10 s
    CHECK(p0 - h.posture(110) == rate * 10);                 // exactamente rate puntos por segundo
  }
  // la banda se aplica desde la vida real del actor vigilado por POST06 (no cambia los pesos por golpe)
  Hx h; Fake a(111, 0xb800); h.spawn(a); a.health = 300; h.frame(a); a.health = 60; h.frame(a);
  CHECK(h.br.regen_rate(h.slot_of(111), a.health) == 2);
  h.hit(a); CHECK(h.posture(111) == 25);
}
// ---- B13: caida vencida en un modo sin punto seguro (guion) -> espera; tras el limite de seguridad: ciclo abortado (rearme), sin caida falsa ----
static void b13_unsafe_point() {
  Hx h; Fake a(112, 0xbc00); h.spawn(a);
  break_it(h, a);
  h.frames(a, 12);
  a.mode = TALK; a.state = 0;
  for (int i = 0; i < 120; ++i) { a.mode = TALK; h.frame(a); }
  CHECK(h.drops == 0 && h.br.c.drops_aborted == 1 && h.posture(112) == 0 && h.phase(112) == post06::Phase::Tracking);
  audit_all(h);
}
// ---- B14: reciclado del id con otro objeto durante el castigo: nada se hereda ni se escribe en la instancia vieja ----
static void b14_reuse() {
  Hx h; Fake a(113, 0xc000); h.spawn(a);
  break_it(h, a);
  h.w.present.erase(113);
  for (int i = 0; i < 400; ++i) h.update();
  Fake b(113, 0xd000); h.spawn(b); h.frames(b, 3);
  CHECK(h.phase(113) == post06::Phase::Tracking && h.posture(113) == 0);
  h.hit(b); CHECK(b.mode == FIND && h.posture(113) == 25);   // POST05 de nuevo activo para la generacion nueva
  CHECK(h.drops == 0);
  audit_all(h);
}
// ---- B15: gemelo sin POST06: en el castigo solo cambia la vida (= bonus), y no hay caida forzada ni rearme ----
static void b15_twin() {
  Hx on(true), off(false); Fake a(114, 0xc400), b(114, 0xc400); on.spawn(a); off.spawn(b);
  break_it(on, a); break_it(off, b);
  CHECK(a.health == b.health && a.mode == b.mode && on.posture(114) == 100 && off.posture(114) == 100);
  for (int i = 0; i < 3; ++i) { on.hit(a); off.hit(b); CHECK(a.mode == b.mode && a.state == b.state && a.cooldown == b.cooldown && a.cut_alt == b.cut_alt); }
  CHECK(b.health - a.health == 6);                       // solo el +20 %: 3 x 2
  for (int i = 0; i < 60; ++i) { on.frame(a); off.frame(b); }
  CHECK(a.mode == BIG || a.mode == FIND); CHECK(b.mode != BIG && off.drops == 0 && off.posture(114) == 100);   // sin POST06: latch permanente (POST05)
}
// ---- B17: varios actores a la vez: ciclos independientes ----
static void b17_multi() {
  Hx h; Fake a(115, 0xc800), b(116, 0xcc00); h.spawn(a); h.spawn(b);
  break_it(h, a);
  h.hit(b);
  CHECK(h.phase(115) == post06::Phase::Punish && h.phase(116) == post06::Phase::Tracking && h.posture(116) == 25 && b.mode == FIND);
  for (int i = 0; i < 40; ++i) { h.frame(a); h.tick(b); }
  CHECK(a.mode == BIG && b.mode == FIND && h.drops == 1);
  audit_all(h);
}
// ---- B18: escritura que no se puede verificar: no cambia la fase; se reintenta; nunca vulnerable para siempre ----
static void b18_write_fail() {
  Hx h; Fake a(117, 0xd400); h.spawn(a);
  break_it(h, a);
  h.break_writer_ok = false;
  h.frames(a, 40);
  CHECK(a.mode != BIG && h.br.c.write_failed >= 5 && h.phase(117) == post06::Phase::Punish);
  h.frames(a, 80);
  CHECK(h.br.c.drops_aborted == 1 && h.posture(117) == 0);   // limite de seguridad: rearme sin caida falsa
  std::printf("b18: aborted=%llu posture=%d failed=%llu phase=%s mode=%d\n", (unsigned long long)h.br.c.drops_aborted, h.posture(117), (unsigned long long)h.br.c.write_failed, post06::name(h.phase(117)), a.mode);
  h.break_writer_ok = true;
  audit_all(h);
}
// ---- B20: nucleo directo: slot reciclado con otra generacion no hereda el ciclo; la caida vencida no se escribe en un tick con golpe (otra familia) ----
static void b20_core_direct() {
  post06::Breaker br; br.enabled = true;
  post06::View v; v.slot = 0; v.generation = 1; v.id = 7; v.tracked = true; v.pre_hp = 100; v.native_hp = 90; v.mode = DAMAGE; v.hit = true; v.sword = true; v.crossed = true; v.latched = true;
  br.step(v);
  CHECK(br.slot(0).phase == post06::Phase::Punish);
  post06::View w = v; w.generation = 2; w.hit = false; w.crossed = false; w.latched = false; w.mode = FIND; w.pre_hp = w.native_hp = 100;
  br.step(w);
  CHECK(br.slot(0).phase == post06::Phase::Tracking && br.c.slot_resets == 1 && br.slot(0).generation == 2);
  // castigo nuevo; 30 llamadas sin espadazo; en la llamada vencida entra una flecha: NO se derriba en ese tick; en la siguiente si
  post06::View x = v; x.generation = 3; br.step(x);
  post06::View q = x; q.hit = false; q.sword = false; q.crossed = false; q.mode = FIND; q.pre_hp = q.native_hp = 90;
  for (int i = 0; i < 29; ++i) CHECK(br.step(q).act == post06::Act::None);
  post06::View arrow = q; arrow.hit = true; arrow.sword = false; arrow.pre_hp = 90; arrow.native_hp = 80; arrow.mode = DAMAGE;
  CHECK(br.step(arrow).act == post06::Act::None);
  const post06::Action d = br.step(q);
  CHECK(d.act == post06::Act::Drop && d.why == post06::Why::Inactivity1s);
}
static void b19_format() {
  Hx h; Fake a(118, 0xd800); h.spawn(a); break_it(h, a);
  char line[600];
  const int n = post06::fmt_summary(line, sizeof(line), h.br, "PERIODIC");
  CHECK(n > 0 && n < 512 && std::strstr(line, "breaks=1") && !std::strstr(line, "0x"));
  post06::View v; v.id = UINT_MAX; v.generation = UINT64_MAX; v.mode = INT_MIN; v.state = INT_MIN; v.pre_hp = INT_MIN; v.native_hp = INT_MIN;
  post06::Breaker::Slot s; s.exec = UINT64_MAX; s.hp_max = INT_MIN; post06::Action act; act.act = post06::Act::Bonus; act.base = INT_MIN; act.extra = INT_MIN; act.new_hp = INT_MIN;
  act.why = post06::Why::RearmLeftBigWithoutDown;
  CHECK(post06::fmt_event(line, sizeof(line), v, s, act, post06::Phase::GettingUp, true) < 512);
  CHECK(!std::strcmp(post06::kBreakVersion, "0.6.0") && !std::strcmp(kModVersion, "0.6.0"));
}

int main() {
  b01_full_cycle();
  b04_max_three_seconds();
  b05_native_knockdowns();
  b07_bonus_lethal();
  b08_finish();
  b10_families_and_fractions();
  b12_regen_bands();
  b13_unsafe_point();
  b14_reuse();
  b15_twin();
  b17_multi();
  b18_write_fail();
  b20_core_direct();
  b19_format();
  std::printf("break_counters: see per-test checks\n");
  std::printf("POST06_BREAK_LAB checks=%d failures=%d RESULT=%s\n", g_checks, g_fail, g_fail ? "FAIL" : "LAB_PASS");
  return g_fail ? 1 : 0;
}
