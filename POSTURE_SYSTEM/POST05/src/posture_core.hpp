#pragma once
// Enemy Posture System, POST04: NUCLEO NUMERICO PASIVO de postura por instancia (logica PURA, sin cabeceras del juego ni del SDK).
//
// Que hace: consume UNA sola vez cada HIT_CONFIRMED verificado por POST03.1, acumula «posture_units» enteras por instancia
// (clave = slot + generation + id del ledger), las recupera con retardo en ticks del observador y emite UNA marca de umbral
// (POST04_BREAK_READY). Que NO hace: no escribe nada en el juego, no cambia HP/IA, no provoca flinch/derribo/Finish, no dibuja.
// BREAK_READY es solo DIAGNOSTICO: ningun codigo de POST04 reacciona a el.
//
// Los pesos y tiempos son una POLITICA DE LABORATORIO PROVISIONAL (POST09 decidira el balance), no un valor aprobado.
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "engine.hpp"

namespace post04 {

using post03::Admission;
using post03::Family;
using post03::HitKind;
using post03::HitReport;
using post03::Instance;
using post03::Ledger;
using post03::Life;

inline constexpr const char* kPostureVersion = "0.6.0";
inline constexpr unsigned kCap = post03::kCapacity;  // mismos 64 slots que el ledger: el sidecar nunca es mayor que el ledger

// ---- politica TEST (constantes; ningun valor magico fuera de este bloque) -------------------------------------------------
namespace policy {
inline constexpr int kMaxPosture = 100;       // tope duro
inline constexpr int kThreshold = 100;        // cruce de umbral: saturado
inline constexpr int kWeightSword = 25;
inline constexpr int kWeightIronBall = 50;
// POST08: postura de UN parry confirmado (Shield Attack en ventana que el motor registra como bloqueo del golpe de ESTE enemigo). [BALANCE_PROVISIONAL_POST08]
// No es un impacto sobre el enemigo: entra por apply_parry(), nunca por consume().
inline constexpr int kParryPostureGain = 40;
static_assert(kParryPostureGain > 0 && kParryPostureGain <= 100, "parry gain");
inline constexpr std::uint64_t kRecoverDelayTicks = 120;  // ticks ELEGIBLES sin recuperar tras el ultimo golpe ponderado
inline constexpr std::uint64_t kRecoverPeriodTicks = 6;   // luego 1 unidad cada 6 ticks elegibles (= tasa base 5/s)
// POST06: la recuperacion es un ACUMULADOR de puntos/s (rate 1..5) a 30 ticks/s: +rate por tick elegible tras el retardo, 1 unidad cada 30 acumulados.
// Con rate 5 emite exactamente en k = delay + 6, +12, ... (identico al periodo 6 anterior). Sin deriva: rate unidades por cada 30 ticks.
inline constexpr std::uint32_t kRegenTicksPerSecond = 30;
inline constexpr std::uint8_t kRegenRateDefault = 5;
inline constexpr const char* kPolicyId = "POST04_TEST_POLICY_V1_PROVISIONAL";
// Peso por familia. Cualquier otra familia, incluida OTHER_VERIFIED y BOOMERANG, pesa 0 POR DEFECTO (sin politica confirmada).
inline constexpr int weight_of(Family f) noexcept {
  switch (f) {
    case Family::Sword: return kWeightSword;
    case Family::IronBall: return kWeightIronBall;
    default: return 0;
  }
}
static_assert(kMaxPosture >= kThreshold && kThreshold > 0 && kMaxPosture <= 255, "posture is stored in a uint8");
static_assert(kRecoverPeriodTicks > 0, "recovery period must be positive");
}  // namespace policy

// ---- razones de omision (mutuamente excluyentes; orden de evaluacion = orden de este enum) -----------------------------------
enum class Skip : std::uint8_t {
  StaleGeneration = 0,  // slot/generation/id ya no son vigentes en el ledger
  NotAdmitted,          // instancia viva pero sin sidecar (excluida, aun no admitida o fuera de cupo)
  ActorDead,            // la entrada ya estaba cerrada por muerte vista
  Lethal,               // ESTE golpe es letal (lethal=1, HP<=0 o el ledger ya vio la muerte)
  UnknownFamily,        // sin lectura fiable del tipo AT
  Ambiguous,            // varios grupos de bits: nunca se reduce a SWORD
  ZeroWeight,           // familia fiable pero sin peso en la politica (OTHER_VERIFIED, BOOMERANG, ...)
  Frozen,               // instancia no verificada o UNKNOWN_UNSAFE: sin acumulacion
  Latched,              // BREAK_READY ya marcado: acumulacion congelada
  Count
};
inline constexpr unsigned kSkipCount = static_cast<unsigned>(Skip::Count);
inline const char* name(Skip s) noexcept {
  switch (s) {
    case Skip::StaleGeneration: return "STALE_GENERATION";
    case Skip::NotAdmitted: return "NOT_ADMITTED_OR_UNBOUND";
    case Skip::ActorDead: return "ACTOR_DEAD";
    case Skip::Lethal: return "LETHAL";
    case Skip::UnknownFamily: return "UNKNOWN_FAMILY";
    case Skip::Ambiguous: return "AMBIGUOUS";
    case Skip::ZeroWeight: return "ZERO_WEIGHT_POLICY";
    case Skip::Frozen: return "FROZEN_UNVERIFIED";
    case Skip::Latched: return "BREAK_READY_LATCHED";
    default: return "NONE";
  }
}

// Estado de lectura de una entrada (NO es la fuente de verdad: lo es el ledger; se deriva y se re-sincroniza).
enum class PState : std::uint8_t { Unbound = 0, Tracking, BreakReadyLatched, FrozenUnverified, InactiveDeath };
inline const char* name(PState s) noexcept {
  switch (s) {
    case PState::Tracking: return "TRACKING";
    case PState::BreakReadyLatched: return "BREAK_READY_LATCHED";
    case PState::FrozenUnverified: return "FROZEN_UNVERIFIED";
    case PState::InactiveDeath: return "INACTIVE_DEATH";
    default: return "UNBOUND";
  }
}

inline constexpr std::uint8_t kUnsafeAdmission = 1, kUnsafeLookup = 2;  // POST05-COMPAT: motivos del historial inseguro
struct Entry {
  bool bound = false, latched = false, dead = false, frozen = false, in_run = false;
  bool ever_frozen = false;  // POST05: esta generacion estuvo alguna vez UNKNOWN_UNSAFE o no verificada (no es un detector de Blue)
  std::uint8_t unsafe_mask = 0;  // POST05-COMPAT: POR QUE fue insegura (kUnsafeAdmission | kUnsafeLookup); ever_frozen == (unsafe_mask != 0)
  std::uint8_t posture = 0;  // 0..kMaxPosture
  std::uint8_t run_from = 0;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  std::uint64_t bind_tick = 0, last_hit_tick = 0, last_applied_event = 0;
  std::uint64_t k = 0;            // ticks ELEGIBLES transcurridos desde el ultimo golpe ponderado
  std::uint64_t last_adv_seq = 0; // sello de avance: una entrada se actualiza como maximo una vez por advance()
  std::uint64_t run_ticks = 0;
  std::uint32_t weighted_hits = 0;
  std::uint8_t regen_rate = policy::kRegenRateDefault;  // POST06: puntos/s segun la banda de vida (1..5)
  std::uint8_t regen_acc = 0;                           // POST06: acumulador 0..29
  std::uint64_t last_parry_token = 0;                   // POST08: token del ultimo parry aplicado (un +40 por ataque)
  PState state() const noexcept {
    if (!bound) return PState::Unbound;
    if (dead) return PState::InactiveDeath;
    if (frozen) return PState::FrozenUnverified;
    return latched ? PState::BreakReadyLatched : PState::Tracking;
  }
};

struct PCounters {
  // --- entrada de eventos ---
  std::uint64_t consume_calls = 0, dup_consume_calls = 0, reports_new = 0;
  std::uint64_t finish_events = 0, not_confirmed = 0, confirmed_consumable = 0, weighted = 0;
  std::uint64_t skipped[kSkipCount] = {};
  std::uint64_t fam_confirmed[post03::kFamilyCount] = {}, fam_weighted[post03::kFamilyCount] = {};
  // --- ciclo de vida del sidecar ---
  std::uint64_t binds = 0, rebinds = 0, retired = 0, latched_retired = 0, death_closed = 0, excluded_generations = 0, divergence_repairs = 0;
  // --- umbral y unidades ---
  std::uint64_t crossings = 0, saturations = 0;
  std::uint64_t units_added = 0, units_recovered = 0, units_closed = 0;
  // --- tiempo ---
  std::uint64_t ticks = 0, frozen_entry_ticks = 0, latched_entry_ticks = 0, regen_runs = 0, freeze_events = 0, thaw_events = 0, seq_wraps = 0;
  // --- invariantes (deben ser 0) ---
  std::uint64_t underflow = 0, overflow = 0, generation_leak = 0, multi_consume = 0;
  std::uint64_t rearms = 0, units_rearmed = 0, rate_sets = 0;
  // POST08: parries (evento propio, no es un impacto): llamadas = aplicados + rechazos; unidades y cruces forman parte de units_added / crossings
  std::uint64_t parry_calls = 0, parries = 0, parry_units = 0, parry_crossings = 0, parry_saturations = 0, parry_rejected[8] = {};  // POST06: rearme tras levantarse (latch 100 -> 0) y cambios de banda
  // --- registro ---
  std::uint64_t ring_dropped = 0, log_written = 0, log_dropped = 0;
};

// ---- cola de eventos de registro (tamano fijo; el log nunca es la fuente de verdad: lo son los contadores) --------------------
enum class PKind : std::uint8_t { Bind = 0, Hit, Regen, BreakReady, Retire };
inline constexpr unsigned kPKindCount = 5;
struct PItem {
  PKind kind = PKind::Bind;
  bool apply = false, latched = false, dead = false, delete_claim = false, at_valid = false, lethal = false;
  Skip skip = Skip::Count;
  std::uint8_t family = 0;
  std::uint32_t id = 0, at = 0;
  std::uint64_t generation = 0, event = 0, tick = 0, ticks = 0;
  int before = 0, delta = 0, after = 0;
  const char* reason = "";
};

// POST05: resultado del consumo de UN informe (por valor; valido solo para ese evento)
struct ConsumeOutcome {
  std::uint64_t event = 0;
  int slot = -1;
  std::uint64_t generation = 0;
  bool consumed_new = false;  // informe nuevo (no duplicado)
  bool applied = false;       // golpe ponderado aplicado
  bool crossed = false;       // este golpe cruzo el umbral
  bool latched_after = false, frozen = false, dead = false, ever_frozen = false;
  std::uint8_t unsafe_mask = 0;  // POST05-COMPAT
  int before = 0, after = 0;
  Skip skip = Skip::Count;
};

struct AuditResult {
  unsigned bound_now = 0, latched_now = 0, dead_now = 0, frozen_now = 0;
  std::uint64_t posture_sum = 0;
  bool e1 = false, e2 = false, e3 = false, e4 = false, e5 = false, e6 = false, invariants = false, ok = false;
};

class Posture {
 public:
  PCounters c{};

  // ---- consulta ----
  const Entry& entry(unsigned i) const noexcept { return ent_[i < kCap ? i : 0]; }
  std::uint64_t now() const noexcept { return now_; }
  void set_adv_seq_for_test(std::uint64_t v) noexcept { adv_seq_ = v; }  // lab: simula la cercania del wrap del sello de avance
  static constexpr unsigned ring_capacity() noexcept { return kRingCap; }

  // ---- sincronizacion con el ledger (O(64), sin lecturas del juego) --------------------------------------------------------
  void sync(const Ledger& L) noexcept {
    for (unsigned i = 0; i < kCap; ++i) sync_slot(L, i);
  }

  // ---- un tick matematico: se llama UNA vez por mod_update, DESPUES del barrido de existencia -----------------------------------
  void advance() noexcept {
    ++now_;
    ++adv_seq_;
    if (adv_seq_ == 0) {  // proteccion de wrap del sello: 0 queda reservado como «nunca actualizado»
      ++adv_seq_;
      ++c.seq_wraps;
    }
    ++c.ticks;
    for (unsigned i = 0; i < kCap; ++i) {
      Entry& e = ent_[i];
      if (!e.bound || e.last_adv_seq == adv_seq_) continue;
      e.last_adv_seq = adv_seq_;
      if (e.dead) continue;
      if (e.frozen) {
        ++c.frozen_entry_ticks;
        continue;
      }
      if (e.latched) {
        ++c.latched_entry_ticks;
        continue;
      }
      if (e.posture == 0) continue;
      ++e.k;  // tick elegible desde el ultimo golpe ponderado (el del propio golpe no cuenta: k se reinicia en 0 al golpear)
      if (e.k <= policy::kRecoverDelayTicks) continue;
      e.regen_acc = static_cast<std::uint8_t>(e.regen_acc + (e.regen_rate >= 1 && e.regen_rate <= 5 ? e.regen_rate : policy::kRegenRateDefault));
      if (e.regen_acc >= policy::kRegenTicksPerSecond) {
        e.regen_acc = static_cast<std::uint8_t>(e.regen_acc - policy::kRegenTicksPerSecond);
        if (!e.in_run) {
          e.in_run = true;
          e.run_from = e.posture;
          ++c.regen_runs;
        }
        --e.posture;
        ++c.units_recovered;
        e.run_ticks = e.k - policy::kRecoverDelayTicks;
        if (e.posture == 0) close_run(i, "REACHED_ZERO");
      }
    }
  }

  // ---- consumo de UN evento de impacto (una sola vez por serial de evento) ---------------------------------------------------
  ConsumeOutcome consume(const HitReport& r, const Ledger& L) noexcept {
    ConsumeOutcome out;
    out.event = r.event;
    out.slot = r.slot;
    out.generation = r.generation;
    ++c.consume_calls;
    if (r.event != 0 && r.event <= last_event_) {  // el mismo informe (o uno anterior) llegando otra vez: NO se vuelve a contar
      ++c.dup_consume_calls;
      return out;
    }
    out.consumed_new = true;
    last_event_ = r.event;
    ++c.reports_new;
    if (r.j.kind == HitKind::FinishNative || r.j.kind == HitKind::FinishUnexpected) {
      ++c.finish_events;
      return out;
    }
    if (r.j.kind != HitKind::Confirmed || r.j.elig != post03::Eligibility::Confirmed) {
      ++c.not_confirmed;
      return out;
    }
    ++c.confirmed_consumable;
    const unsigned fi = static_cast<unsigned>(r.fam.family);
    ++c.fam_confirmed[fi < post03::kFamilyCount ? fi : 0];
    const bool lethal = r.j.lethal || r.post.health <= 0 || r.life == Life::DeathSeen;
    const bool known_family = r.at_valid && r.fam.family != Family::Unavailable;
    const int w = known_family ? policy::weight_of(r.fam.family) : 0;
    Entry* e = nullptr;
    Skip why = Skip::Count;
    if (r.slot < 0 || static_cast<unsigned>(r.slot) >= kCap || !L.valid(r.slot, r.generation) || L.slot(static_cast<unsigned>(r.slot)).id != r.id) {
      why = Skip::StaleGeneration;
    } else {
      e = &ent_[r.slot];
      if (!(e->bound && e->generation == r.generation && e->id == r.id)) why = Skip::NotAdmitted;
      else if (e->dead) why = Skip::ActorDead;
      else if (lethal) why = Skip::Lethal;
      else if (!known_family) why = Skip::UnknownFamily;
      else if (r.fam.family == Family::Ambiguous) why = Skip::Ambiguous;
      else if (w <= 0) why = Skip::ZeroWeight;
      else if (e->frozen) why = Skip::Frozen;
      else if (e->latched) why = Skip::Latched;
    }
    PItem it;
    it.kind = PKind::Hit;
    it.id = r.id;
    it.generation = r.generation;
    it.event = r.event;
    it.tick = now_;
    it.family = static_cast<std::uint8_t>(fi);
    it.at_valid = r.at_valid;
    it.at = r.at_type;
    it.lethal = lethal;
    if (why != Skip::Count) {
      ++c.skipped[static_cast<unsigned>(why)];
      it.apply = false;
      it.skip = why;
      it.reason = name(why);
      it.before = it.after = e ? e->posture : 0;
      if (skip_log_allow(why)) push(it);
      out.skip = why;
      if (e) fill_out(out, *e);
      return out;
    }
    // ---- aplicar ----
    if (e->in_run) close_run(static_cast<unsigned>(r.slot), "INTERRUPTED_BY_HIT");
    if (e->last_applied_event != 0 && e->last_applied_event >= r.event) ++c.multi_consume;  // invariante: nunca el mismo evento dos veces
    const int before = e->posture;
    int sum = before + w;
    if (sum > policy::kMaxPosture) {
      sum = policy::kMaxPosture;
      ++c.saturations;
    }
    e->posture = static_cast<std::uint8_t>(sum);
    e->k = 0;
    e->regen_acc = 0;
    e->last_hit_tick = now_;
    e->last_applied_event = r.event;
    ++e->weighted_hits;
    ++c.weighted;
    ++c.fam_weighted[fi < post03::kFamilyCount ? fi : 0];
    c.units_added += static_cast<std::uint64_t>(sum - before);
    it.apply = true;
    it.reason = "WEIGHTED_HIT";
    it.before = before;
    it.delta = sum - before;
    it.after = sum;
    push(it);
    if (before < policy::kThreshold && sum >= policy::kThreshold) {  // 1 sola transicion por ciclo (latched impide repetirla)
      e->latched = true;
      ++c.crossings;
      PItem b;
      b.kind = PKind::BreakReady;
      b.id = e->id;
      b.generation = e->generation;
      b.event = r.event;
      b.tick = now_;
      b.before = before;
      b.after = sum;
      b.reason = "WEIGHTED_HIT";
      push(b);
      out.crossed = true;
    }
    out.applied = true;
    fill_out(out, *e);
    out.before = before;
    return out;
  }

  // ---- cierre ordenado (mod_shutdown): solo cierra carreras abiertas; no consulta el juego ------------------------------------
  void flush_runs() noexcept {
    for (unsigned i = 0; i < kCap; ++i)
      if (ent_[i].bound && ent_[i].in_run) close_run(i, "SHUTDOWN_FLUSH");
  }

  // ---- auditoria: ecuaciones de conservacion e invariantes ---------------------------------------------------------------------
  // ---- POST06 (solo desde la maquina de ruptura, con el token slot+generacion+id) ----
  // Tasa de recuperacion (puntos/s) segun la banda de vida de ESTA generacion. No toca la postura.
  bool set_regen_rate(int slot, std::uint64_t generation, std::uint32_t id, std::uint8_t rate) noexcept {
    if (slot < 0 || static_cast<unsigned>(slot) >= kCap || rate < 1 || rate > 5) return false;
    Entry& e = ent_[slot];
    if (!e.bound || e.generation != generation || e.id != id) return false;
    if (e.regen_rate != rate) ++c.rate_sets;
    e.regen_rate = rate;
    return true;
  }
  // Rearme confirmado tras levantarse: latch 100 -> postura 0, contadores temporales a cero. Solo una entrada viva, enganchada y del mismo token.
  bool rearm(int slot, std::uint64_t generation, std::uint32_t id, int target = 0) noexcept {
    if (target != 0 && target != 50) return false;
    if (slot < 0 || static_cast<unsigned>(slot) >= kCap) return false;
    Entry& e = ent_[slot];
    if (!e.bound || e.generation != generation || e.id != id || !e.latched || e.dead || e.frozen || e.posture != policy::kThreshold) return false;
    close_run(static_cast<unsigned>(slot), "REARM");
    ++c.rearms;
    c.units_rearmed += e.posture - target;
    e.posture = static_cast<std::uint8_t>(target);
    e.latched = false;
    e.k = 0;
    e.regen_acc = 0;
    return true;
  }

  // ---- POST08: parry confirmado (identidad slot+generacion+id y token del ataque) --------------------------------------------
  enum class ParryReject : std::uint8_t { None = 0, Stale, NotBound, Dead, Frozen, Latched, DuplicateToken, BadToken };
  struct ParryOutcome {
    bool applied = false, crossed = false;
    ParryReject reject = ParryReject::None;
    int before = 0, after = 0;
  };
  // Vista previa SIN efectos (el adaptador decide su escritura de desvio antes de aplicar).
  ParryOutcome preview_parry(int slot, std::uint64_t generation, std::uint32_t id, std::uint64_t token, const Ledger& L) const noexcept {
    ParryOutcome o;
    if (token == 0) o.reject = ParryReject::BadToken;
    else if (slot < 0 || static_cast<unsigned>(slot) >= kCap || !L.valid(slot, generation) || L.slot(static_cast<unsigned>(slot)).id != id) o.reject = ParryReject::Stale;
    else {
      const Entry& e = ent_[slot];
      if (!(e.bound && e.generation == generation && e.id == id)) o.reject = ParryReject::NotBound;
      else if (e.dead) o.reject = ParryReject::Dead;
      else if (e.frozen) o.reject = ParryReject::Frozen;
      else if (e.latched) o.reject = ParryReject::Latched;
      else if (e.last_parry_token == token) o.reject = ParryReject::DuplicateToken;
      else {
        o.before = e.posture;
        o.after = e.posture + policy::kParryPostureGain > policy::kMaxPosture ? policy::kMaxPosture : e.posture + policy::kParryPostureGain;
        o.crossed = o.before < policy::kThreshold && o.after >= policy::kThreshold;
      }
    }
    return o;
  }
  // +kParryPostureGain saturado a 100; reinicia el retardo de recuperacion como un golpe ponderado; un cruce = el mismo latch que un golpe.
  ParryOutcome apply_parry(int slot, std::uint64_t generation, std::uint32_t id, std::uint64_t token, const Ledger& L) noexcept {
    ++c.parry_calls;
    ParryOutcome o = preview_parry(slot, generation, id, token, L);
    if (o.reject != ParryReject::None) {
      ++c.parry_rejected[static_cast<unsigned>(o.reject) & 7u];
      return o;
    }
    Entry& e = ent_[slot];
    if (e.in_run) close_run(static_cast<unsigned>(slot), "INTERRUPTED_BY_PARRY");
    if (e.posture + policy::kParryPostureGain > policy::kMaxPosture) ++c.parry_saturations;
    e.posture = static_cast<std::uint8_t>(o.after);
    e.k = 0;
    e.regen_acc = 0;
    e.last_hit_tick = now_;
    e.last_parry_token = token;
    ++c.parries;
    c.parry_units += static_cast<std::uint64_t>(o.after - o.before);
    c.units_added += static_cast<std::uint64_t>(o.after - o.before);
    if (o.crossed) {
      e.latched = true;
      ++c.crossings;
      ++c.parry_crossings;
      PItem b;
      b.kind = PKind::BreakReady;
      b.id = e.id;
      b.generation = e.generation;
      b.event = 0;
      b.tick = now_;
      b.before = o.before;
      b.after = o.after;
      b.reason = "PARRY";
      push(b);
    }
    o.applied = true;
    return o;
  }

  AuditResult audit() noexcept {
    AuditResult a;
    for (unsigned i = 0; i < kCap; ++i) {
      const Entry& e = ent_[i];
      if (!e.bound) continue;
      ++a.bound_now;
      a.latched_now += e.latched ? 1u : 0u;
      a.dead_now += e.dead ? 1u : 0u;
      a.frozen_now += e.frozen ? 1u : 0u;
      a.posture_sum += e.posture;
      if (e.posture > policy::kMaxPosture) ++c.overflow;  // un decremento bajo cero en uint8 aparece aqui (255)
      if (e.latched && e.posture != policy::kThreshold) ++c.overflow;
    }
    std::uint64_t sk = 0;
    for (unsigned i = 0; i < kSkipCount; ++i) sk += c.skipped[i];
    a.e1 = c.confirmed_consumable == c.weighted + sk;
    a.e2 = c.reports_new == c.finish_events + c.not_confirmed + c.confirmed_consumable && c.consume_calls == c.reports_new + c.dup_consume_calls;
    a.e3 = c.binds == c.retired + a.bound_now && a.bound_now <= kCap && c.rebinds <= c.binds;
    // POST08: un cruce viene de un golpe ponderado O de un parry (cada uno latchea como mucho una vez); se reparten entre ambos.
    std::uint64_t parry_rej = 0;
    for (unsigned i = 0; i < 8; ++i) parry_rej += c.parry_rejected[i];
    a.e4 = c.crossings <= c.weighted + c.parries && c.parry_crossings <= c.parries && c.crossings == a.latched_now + c.latched_retired + c.rearms;
    a.e6 = c.parry_calls == c.parries + parry_rej && c.parry_units <= c.units_added && c.parry_units <= c.parries * static_cast<std::uint64_t>(policy::kParryPostureGain);
    a.e5 = c.units_added == c.units_recovered + c.units_closed + c.units_rearmed + a.posture_sum;
    a.invariants = c.underflow == 0 && c.overflow == 0 && c.generation_leak == 0 && c.multi_consume == 0;
    a.ok = a.e1 && a.e2 && a.e3 && a.e4 && a.e5 && a.e6 && a.invariants;
    return a;
  }

  // ---- cola de registro ---------------------------------------------------------------------------------------------------------
  bool pop(PItem& out) noexcept {
    if (n_ == 0) return false;
    out = ring_[head_];
    head_ = (head_ + 1) % kRingCap;
    --n_;
    return true;
  }
  unsigned pending_items() const noexcept { return n_; }

  // Presupuesto de lineas de postura por sesion (se suma al del observador sin superar 2000 en total).
  bool log_take(PKind k) noexcept {
    static constexpr unsigned kKindCap[kPKindCount] = {120, 250, 100, 50, 120};
    const unsigned i = static_cast<unsigned>(k);
    if (log_total_ >= kLogBudget || log_kind_[i] >= kKindCap[i]) {
      ++c.log_dropped;
      return false;
    }
    ++log_total_;
    ++log_kind_[i];
    ++c.log_written;
    return true;
  }
  static constexpr unsigned kLogBudget = 500;

 private:
  static constexpr unsigned kRingCap = 128;
  static constexpr unsigned kSkipLogCap = 8;  // primeras N omisiones por razon: el resto solo en contadores

  Entry ent_[kCap]{};
  std::uint64_t handled_gen_[kCap] = {};
  std::uint64_t unsafe_seen_gen_[kCap] = {};  // POST05: generacion vista UNKNOWN_UNSAFE antes de ligarse
  std::uint64_t now_ = 0, adv_seq_ = 0, last_event_ = 0;
  PItem ring_[kRingCap]{};
  unsigned head_ = 0, tail_ = 0, n_ = 0;
  unsigned skip_logged_[kSkipCount] = {};
  unsigned log_kind_[kPKindCount] = {};
  unsigned log_total_ = 0;

  static void fill_out(ConsumeOutcome& o, const Entry& e) noexcept {
    o.before = o.after = e.posture;
    o.latched_after = e.latched;
    o.frozen = e.frozen;
    o.dead = e.dead;
    o.ever_frozen = e.ever_frozen;
    o.unsafe_mask = e.unsafe_mask;
  }
  bool skip_log_allow(Skip s) noexcept {
    unsigned& n = skip_logged_[static_cast<unsigned>(s)];
    if (n >= kSkipLogCap) return false;
    ++n;
    return true;
  }
  void push(const PItem& it) noexcept {
    if (n_ >= kRingCap) {
      ++c.ring_dropped;
      return;
    }
    ring_[tail_] = it;
    tail_ = (tail_ + 1) % kRingCap;
    ++n_;
  }
  static bool live(const Instance& in) noexcept { return in.life == Life::Active || in.life == Life::DeathSeen || in.life == Life::Unverified; }

  void close_run(unsigned i, const char* why) noexcept {
    Entry& e = ent_[i];
    if (!e.in_run) return;
    e.in_run = false;
    PItem it;
    it.kind = PKind::Regen;
    it.id = e.id;
    it.generation = e.generation;
    it.tick = now_;
    it.before = e.run_from;
    it.after = e.posture;
    it.ticks = e.run_ticks;
    it.reason = why;
    push(it);
  }

  void retire(unsigned i, const char* why, bool claim) noexcept {
    Entry& e = ent_[i];
    close_run(i, why);
    ++c.retired;
    if (e.latched) ++c.latched_retired;
    c.units_closed += e.posture;
    PItem it;
    it.kind = PKind::Retire;
    it.id = e.id;
    it.generation = e.generation;
    it.tick = now_;
    it.after = e.posture;
    it.latched = e.latched;
    it.dead = e.dead;
    it.delete_claim = claim;  // SOLO LOOKUP_ABSENT_VERIFIED del ledger
    it.reason = why;
    push(it);
    e = Entry{};  // la entrada vuelve a vacio: nada se hereda a la generacion siguiente
  }

  void bind(unsigned i, const Instance& in) noexcept {
    Entry& e = ent_[i];
    if (e.posture != 0 || e.latched || e.k != 0 || e.dead || e.in_run || e.weighted_hits != 0 || e.last_applied_event != 0) ++c.generation_leak;
    const bool rebind = handled_gen_[i] != 0 && handled_gen_[i] != in.generation;
    e = Entry{};
    e.bound = true;
    e.id = in.id;
    e.generation = in.generation;
    e.bind_tick = now_;
    e.last_adv_seq = adv_seq_;  // no recupera en el tick en que nace
    handled_gen_[i] = in.generation;
    ++c.binds;
    if (rebind) ++c.rebinds;
    PItem it;
    it.kind = PKind::Bind;
    it.id = e.id;
    it.generation = e.generation;
    it.tick = now_;
    it.reason = "FIRST_OBSERVATION_NOT_SPAWN";
    push(it);
  }

  void refresh_flags(unsigned i, const Instance& in) noexcept {
    Entry& e = ent_[i];
    const bool dead = in.life == Life::DeathSeen;
    if (dead && !e.dead) {
      close_run(i, "DEATH_SEEN");
      e.dead = true;
      ++c.death_closed;
    }
    const bool frozen = !dead && (in.life == Life::Unverified || in.adm == Admission::UNKNOWN_UNSAFE);
    if (frozen) e.ever_frozen = true;
    if (!dead && in.adm == Admission::UNKNOWN_UNSAFE) e.unsafe_mask |= kUnsafeAdmission;
    if (!dead && in.life == Life::Unverified) e.unsafe_mask |= kUnsafeLookup;
    if (frozen != e.frozen) {
      if (frozen) {
        close_run(i, "FROZEN");
        ++c.freeze_events;
      } else {
        ++c.thaw_events;  // se reanuda donde estaba: NO se aplican de golpe los ticks transcurridos en congelacion
      }
      e.frozen = frozen;
    }
  }

  void sync_slot(const Ledger& L, unsigned i) noexcept {
    const Instance& in = L.slot(i);
    Entry& e = ent_[i];
    if (e.bound) {
      if (in.id != e.id || in.generation != e.generation) {  // el slot fue reutilizado antes de sincronizar: se cierra la entrada vieja
        ++c.divergence_repairs;
        retire(i, "SLOT_REUSED_BEFORE_SYNC", false);
      } else if (!live(in)) {
        retire(i, post03::name(in.retired), post03::delete_claim(in.retired));
      } else if (in.adm == Admission::E_OC2_EXCLUDED || in.adm == Admission::PROFILE_OTHER) {
        retire(i, "ADMISSION_EXCLUDED", false);
      } else {
        refresh_flags(i, in);
      }
    }
    if (!ent_[i].bound && live(in) && in.generation != 0 && handled_gen_[i] != in.generation) {
      if (post03::is_candidate(in.adm)) {  // POST05-COMPAT: E_OC y E_OC2 candidatos reciben postura (pasiva)
        bind(i, in);
        if (unsafe_seen_gen_[i] == in.generation) {
          ent_[i].ever_frozen = true;
          ent_[i].unsafe_mask |= kUnsafeAdmission;
        }
        refresh_flags(i, in);
      } else if (in.adm == Admission::UNKNOWN_UNSAFE) {
        unsafe_seen_gen_[i] = in.generation;  // aun sin sidecar: se recuerda para no tratarla nunca como estable
      } else if (in.adm != Admission::UNKNOWN_UNSAFE) {  // E_OC2 / otro perfil: jamas postura; se anota una sola vez
        handled_gen_[i] = in.generation;
        ++c.excluded_generations;
      }
    }
  }
};

// ---- envoltura: motor del observador + postura ---------------------------------------------------------------------------------
class PostureEngine {
 public:
  post03::Engine eng;
  Posture px;

  ConsumeOutcome last{};  // POST05: resultado del consumo del ultimo on_post con informe

  post03::Observation on_pre(std::uintptr_t actor, std::uint32_t id, const post03::PreReads& r, post03::Events& ev) noexcept {
    if (eng.obs.disabled) return post03::Observation{};  // modo inerte: ni observa ni acumula
    const post03::Observation o = eng.on_pre(actor, id, r, ev);
    if (ev.n > 0) px.sync(eng.obs.ledger);  // alta de instancia, cambio de admision, retiradas por conflicto
    return o;
  }
  template <class ReadAt>
  bool on_post(std::uintptr_t actor, const post03::PostReads& r, ReadAt read_at, post03::Events& ev, HitReport& rep) noexcept {
    if (eng.obs.disabled) return false;
    const bool has = eng.on_post(actor, r, read_at, ev, rep);
    last = ConsumeOutcome{};
    if (has) last = px.consume(rep, eng.obs.ledger);  // ANTES de sincronizar la muerte de este mismo golpe: asi un golpe letal se omite como LETHAL
    if (has || ev.n > 0) px.sync(eng.obs.ledger);
    return has;
  }
  // mod_update, tras el barrido de existencia: sincroniza y avanza UN tick matematico.
  void on_update() noexcept {
    if (eng.obs.disabled) return;  // un mod desactivado no hace avanzar el reloj de postura
    px.sync(eng.obs.ledger);
    px.advance();
  }
  void on_shutdown() noexcept {
    px.sync(eng.obs.ledger);
    px.flush_runs();
  }
};

// ---- formato de lineas (sin punteros ni rutas) --------------------------------------------------------------------------------
inline int fmt_item(char* out, std::size_t n, const PItem& it) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  switch (it.kind) {
    case PKind::Bind:
      return std::snprintf(out, n, "POST04_POSTURE_BIND id=%u generation=%llu initial=0 tick=%llu reason=%s", it.id, u(it.generation), u(it.tick), it.reason);
    case PKind::Hit: {
      char at[24];
      if (it.at_valid) std::snprintf(at, sizeof(at), "0x%08x", static_cast<unsigned>(it.at));
      else std::snprintf(at, sizeof(at), "UNAVAILABLE");
      return std::snprintf(out, n,
                           "POST04_POSTURE_HIT id=%u generation=%llu event=%llu family=%s at=%s eligible=CONFIRMED lethal=%d before=%d delta=%d after=%d "
                           "action=%s reason=%s tick=%llu",
                           it.id, u(it.generation), u(it.event), post03::name(static_cast<Family>(it.family)), at, it.lethal ? 1 : 0, it.before, it.delta, it.after,
                           it.apply ? "APPLY" : "SKIP", it.reason, u(it.tick));
    }
    case PKind::Regen:
      return std::snprintf(out, n, "POST04_POSTURE_REGEN id=%u generation=%llu from=%d to=%d ticks=%llu cause=%s tick=%llu", it.id, u(it.generation),
                           it.before, it.after, u(it.ticks), it.reason, u(it.tick));
    case PKind::BreakReady:
      return std::snprintf(out, n,
                           "POST04_BREAK_READY id=%u generation=%llu before=%d after=%d threshold=%d cause=%s game_action=NONE tick=%llu", it.id,
                           u(it.generation), it.before, it.after, policy::kThreshold, it.reason, u(it.tick));
    case PKind::Retire:
      return std::snprintf(out, n, "POST04_POSTURE_RETIRE id=%u generation=%llu reason=%s final=%d latched=%d dead=%d delete_claim=%d tick=%llu", it.id,
                           u(it.generation), it.reason, it.after, it.latched ? 1 : 0, it.dead ? 1 : 0, it.delete_claim ? 1 : 0, u(it.tick));
  }
  return 0;
}

inline int fmt_summary(char* out, std::size_t n, const Posture& p, const AuditResult& a, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  const PCounters& c = p.c;
  return std::snprintf(
      out, n,
      "POST04_SUMMARY v=%s tag=%s ticks=%llu reports_new=%llu dup_consume_calls=%llu finish=%llu not_confirmed=%llu confirmed_consumable=%llu weighted=%llu "
      "skipped(stale/unbound/dead/lethal/unknown_fam/ambiguous/zero_weight/frozen/latched)=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
      "fam_confirmed(sword/shield/arrow/bomb/iron/boomerang/at40/sling/other/ambiguous/unavail)=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
      "fam_weighted(sword/iron)=%llu/%llu binds=%llu rebinds=%llu retired=%llu bound_now=%u latched_now=%u dead_now=%u frozen_now=%u death_closed=%llu "
      "excluded_generations=%llu divergence_repairs=%llu crossings=%llu saturations=%llu units(added/recovered/closed/live)=%llu/%llu/%llu/%llu "
      "regen_runs=%llu freeze_thaw=%llu/%llu frozen_entry_ticks=%llu latched_entry_ticks=%llu underflow=%llu overflow=%llu generation_leak=%llu "
      "multi_consume=%llu ring_dropped=%llu log(written/dropped)=%llu/%llu",
      kPostureVersion, tag, u(c.ticks), u(c.reports_new), u(c.dup_consume_calls), u(c.finish_events), u(c.not_confirmed), u(c.confirmed_consumable),
      u(c.weighted), u(c.skipped[0]), u(c.skipped[1]), u(c.skipped[2]), u(c.skipped[3]), u(c.skipped[4]), u(c.skipped[5]), u(c.skipped[6]), u(c.skipped[7]),
      u(c.skipped[8]), u(c.fam_confirmed[1]), u(c.fam_confirmed[2]), u(c.fam_confirmed[3]), u(c.fam_confirmed[4]), u(c.fam_confirmed[5]),
      u(c.fam_confirmed[6]), u(c.fam_confirmed[7]), u(c.fam_confirmed[8]), u(c.fam_confirmed[9]), u(c.fam_confirmed[10]), u(c.fam_confirmed[0]),
      u(c.fam_weighted[1]), u(c.fam_weighted[5]), u(c.binds), u(c.rebinds), u(c.retired), a.bound_now, a.latched_now, a.dead_now, a.frozen_now,
      u(c.death_closed), u(c.excluded_generations), u(c.divergence_repairs), u(c.crossings), u(c.saturations), u(c.units_added), u(c.units_recovered),
      u(c.units_closed), u(a.posture_sum), u(c.regen_runs), u(c.freeze_events), u(c.thaw_events), u(c.frozen_entry_ticks), u(c.latched_entry_ticks),
      u(c.underflow), u(c.overflow), u(c.generation_leak), u(c.multi_consume), u(c.ring_dropped), u(c.log_written), u(c.log_dropped));
}

inline int fmt_conservation(char* out, std::size_t n, const Posture& p, const AuditResult& a, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  const PCounters& c = p.c;
  return std::snprintf(
      out, n,
      "POST04_CONSERVATION v=%s tag=%s ok=%d e1_consumable=%d e2_reports=%d e3_binds=%d e4_threshold=%d e5_units=%d invariants=%d confirmed_consumable=%llu "
      "weighted=%llu binds=%llu retired=%llu bound_now=%u crossings=%llu units_added=%llu units_recovered=%llu units_closed=%llu units_live=%llu",
      kPostureVersion, tag, a.ok ? 1 : 0, a.e1 ? 1 : 0, a.e2 ? 1 : 0, a.e3 ? 1 : 0, a.e4 ? 1 : 0, a.e5 ? 1 : 0, a.invariants ? 1 : 0,
      u(c.confirmed_consumable), u(c.weighted), u(c.binds), u(c.retired), a.bound_now, u(c.crossings), u(c.units_added), u(c.units_recovered),
      u(c.units_closed), u(a.posture_sum));
}

}  // namespace post04
