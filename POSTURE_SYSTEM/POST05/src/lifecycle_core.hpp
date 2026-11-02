#pragma once
// Enemy Posture System, POST03: ledger de instancias con GENERACION, ciclo de vida y contabilidad de impactos (logica PURA).
// Sin cabeceras del juego. mod.cpp lee el juego, hace la consulta de existencia y llama a estas funciones.
// Nada aqui toca memoria del juego. Todo el almacenamiento es fijo (sin new/map/vector en el camino caliente).
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "hit_core.hpp"

namespace post03 {

inline constexpr const char* kModVersion = "0.10.1";
inline constexpr unsigned kCapacity = 64;       // vivos simultaneos: 32 encuentros DEVF + nativos de la sala, con margen; no es un mapa infinito
inline constexpr unsigned kFrameCapacity = 4;
inline constexpr unsigned kLogLineMax = 512;
inline constexpr unsigned kSweepPeriodTicks = 10;  // un barrido cada 10 llamadas a mod_update
inline constexpr unsigned kSweepQuota = 2;         // consultas de existencia por barrido
inline constexpr unsigned kUnavailableStreakLimit = 3;
inline constexpr std::uint64_t kSummaryPeriodCalls = 18000;
inline constexpr unsigned kDisableLogErrors = 3;
inline constexpr std::uint32_t kIdUnknown = 0xFFFFFFFEu;  // fpcM_UNK_PROCESS_ID_e
inline constexpr std::uint32_t kIdError = 0xFFFFFFFFu;    // fpcM_ERROR_PROCESS_ID_e

// ---- log: presupuesto duro por categoria (suma 1500 lineas del observador; ver POST04 para el total) -----------------------------------------
enum class LogCat : std::uint8_t { Life = 0, Hit = 1, Other = 2, Summary = 3 };
// POST04: 1500 lineas del observador (antes 2000) + 500 de postura (posture_core.hpp) = el mismo techo global de 2000 lineas por sesion.
inline constexpr unsigned kBudget[4] = {450, 700, 250, 100};
inline constexpr unsigned kBudgetTotal = 450 + 700 + 250 + 100;

// ---- estados ----------------------------------------------------------------------------------------------------
enum class Life : std::uint8_t { Free = 0, Active, DeathSeen, Unverified, Retired };
inline const char* name(Life l) noexcept {
  switch (l) {
    case Life::Active: return "ACTIVE";
    case Life::DeathSeen: return "DEATH_SEEN";
    case Life::Unverified: return "UNVERIFIED";
    case Life::Retired: return "RETIRED";
    default: return "FREE";
  }
}
// Motivos de retirada: NINGUNO de ellos, salvo LOOKUP_ABSENT_VERIFIED, afirma que el actor fue borrado.
enum class RetireReason : std::uint8_t { None = 0, LookupAbsentVerified, AddressReused, IdentityConflict, CapacityEvicted, ContextRetired };
inline const char* name(RetireReason r) noexcept {
  switch (r) {
    case RetireReason::LookupAbsentVerified: return "LOOKUP_ABSENT_VERIFIED";
    case RetireReason::AddressReused: return "ADDRESS_REUSED_BY_OTHER_ID";
    case RetireReason::IdentityConflict: return "IDENTITY_CONFLICT_SAME_ID_OTHER_ADDRESS";
    case RetireReason::CapacityEvicted: return "CAPACITY_EVICTED";
    case RetireReason::ContextRetired: return "CONTEXT_RETIRED";
    default: return "NONE";
  }
}
inline bool delete_claim(RetireReason r) noexcept { return r == RetireReason::LookupAbsentVerified; }
enum class UnverifiedReason : std::uint8_t { None = 0, PtrMismatch, ProfileMismatch, Creating, LookupUnavailable };
inline const char* name(UnverifiedReason r) noexcept {
  switch (r) {
    case UnverifiedReason::PtrMismatch: return "LOOKUP_PTR_MISMATCH";
    case UnverifiedReason::ProfileMismatch: return "LOOKUP_PROFILE_MISMATCH";
    case UnverifiedReason::Creating: return "LOOKUP_CREATING";
    case UnverifiedReason::LookupUnavailable: return "LOOKUP_UNAVAILABLE";
    default: return "NONE";
  }
}
// Resultado de la consulta de existencia que hace mod.cpp (fopAcM_SearchByID, solo lectura).
enum class Lookup : std::uint8_t { Absent, PresentMatch, PresentMismatchPtr, PresentMismatchProfile, Creating, Unavailable, Failed };
inline const char* name(Lookup l) noexcept {
  switch (l) {
    case Lookup::Absent: return "SEARCH_ABSENT";
    case Lookup::PresentMatch: return "SEARCH_PRESENT_MATCH";
    case Lookup::PresentMismatchPtr: return "SEARCH_PRESENT_MISMATCH_PTR";
    case Lookup::PresentMismatchProfile: return "SEARCH_PRESENT_MISMATCH_PROFILE";
    case Lookup::Creating: return "SEARCH_CREATING";
    case Lookup::Unavailable: return "SEARCH_UNAVAILABLE";
    default: return "SEARCH_FAILED";
  }
}

// Modos nativos que cuentan como «muerte vista» (indices confirmados en el binario en POST00): DEATH 9, WATER_DEATH 10, FALL_DEAD 13.
inline bool death_evidence(std::int32_t health, std::int32_t mode) noexcept {
  return health <= 0 || mode == 9 || mode == 10 || mode == 13;
}

// ---- epoch de contexto (cambio del nombre de stage de inicio) ---------------------------------------------------
inline std::uint32_t stage_token(const char* s) noexcept {  // FNV-1a sobre como maximo 8 bytes
  std::uint32_t h = 2166136261u;
  if (!s) return h;
  for (unsigned i = 0; i < 8 && s[i]; ++i) h = (h ^ static_cast<std::uint8_t>(s[i])) * 16777619u;
  return h;
}
inline bool epoch_older(std::uint32_t a, std::uint32_t b) noexcept { return static_cast<std::int32_t>(b - a) > 0; }  // seguro ante wrap
struct EpochTracker {
  std::uint32_t epoch = 1;
  std::uint32_t token = 0;
  bool have = false;
  void update(std::uint32_t t) noexcept {
    if (!have) {
      have = true;
      token = t;
    } else if (t != token) {
      token = t;
      ++epoch;
    }
  }
};

// ---- eventos ----------------------------------------------------------------------------------------------------
enum class EvKind : std::uint8_t { None, FirstSeen, AdmissionChanged, DeathSeen, Revalidated, Retired, Unverified, CapacityExhausted, InvalidKey };
struct Event {
  EvKind kind = EvKind::None;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  Admission from = Admission::UNKNOWN_UNSAFE, to = Admission::UNKNOWN_UNSAFE;
  RetireReason reason = RetireReason::None;
  UnverifiedReason unv = UnverifiedReason::None;
  std::uint32_t epoch = 0;
  bool context_changed = false;
};
struct Events {
  Event e[6];
  unsigned n = 0;
  Event* add(EvKind k) noexcept {
    if (n >= 6) return nullptr;
    e[n] = Event{};
    e[n].kind = k;
    return &e[n++];
  }
};

// ---- instancia y contadores ------------------------------------------------------------------------------------
struct Instance {
  Life life = Life::Free;
  Admission adm = Admission::UNKNOWN_UNSAFE;
  RetireReason retired = RetireReason::None;
  UnverifiedReason unv = UnverifiedReason::None;
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;  // valor opaco SOLO para comparar; nunca se desreferencia ni se registra
  std::uint64_t generation = 0;
  std::uint64_t first_call = 0, last_call = 0, retire_seq = 0, swept_stamp = 0;
  std::uint64_t last_event_serial = 0;  // dedup por instancia: un frame (serial) solo puede emitir un evento
  std::uint32_t epoch_first = 0, epoch_last = 0, hits = 0;
  std::uint8_t unavailable_streak = 0;
};

struct Counters {
  std::uint64_t calls = 0, calls_adm[kAdmissionCount] = {}, pre_class[5] = {};
  // --- impactos (POST03) ---
  std::uint64_t cand_started = 0;          // frames de candidato E_OC con TG_CANDIDATE apilados en PRE
  std::uint64_t raw_tg = 0, dual_tg = 0, pair_distinct = 0;  // sobre cand_started
  std::uint64_t confirmed = 0, processed_mismatch = 0, not_processed = 0;  // veredictos de candidatos finalizados
  std::uint64_t finish = 0, finish_unexpected = 0, finish_started = 0;
  std::uint64_t cooldown_blocked = 0, cooldown_with_tg = 0, no_tg = 0, excluded_calls = 0, unknown_unsafe_calls = 0;
  std::uint64_t dup_serial_gate = 0, dup_post_orphan = 0, cross_generation_drops = 0, cand_discarded = 0, finish_discarded = 0;
  std::uint64_t classification_unavailable = 0, collider_unmatched = 0, family_count[kFamilyCount] = {};
  std::uint64_t col_reason[kColReasonCount] = {}, sel_class[kPtrClassCount] = {}, raw0_class[kPtrClassCount] = {}, raw1_class[kPtrClassCount] = {};
  std::uint64_t deref_mod8_0 = 0, deref_mod8_4 = 0;
  std::uint64_t lethal = 0, hp_changed = 0, anomalies = 0, events_emitted = 0;
  // --- ciclo de vida (POST02) ---
  std::uint64_t records_created = 0, max_active = 0, death_seen = 0, absent_verified = 0, context_retired = 0, address_reused = 0;
  std::uint64_t identity_conflicts = 0, capacity_drops = 0, capacity_evictions = 0, unverified = 0, revalidated = 0;
  std::uint64_t lookups = 0, lookup_unavailable = 0, generation_rollovers = 0, admission_changes = 0, invalid_keys = 0, stale_picks = 0;
  std::uint64_t unmatched_post = 0, discarded_frames = 0, frame_overflow = 0, post_generation_mismatch = 0, stale_frames = 0;
  std::uint64_t log_written[4] = {}, log_dropped[4] = {}, log_errors = 0, kind_throttled = 0;
};

struct Observation {
  int slot = -1;
  std::uint64_t generation = 0;
  bool tracked = false;
  bool created = false;
};
struct Pick {
  int slot = -1;
  std::uint32_t id = 0;
  std::uintptr_t ptr = 0;
  std::uint64_t generation = 0;
};

class Ledger {
 public:
  Counters c{};

  unsigned active() const noexcept {
    unsigned n = 0;
    for (const Instance& i : s_) n += (i.life == Life::Active || i.life == Life::DeathSeen || i.life == Life::Unverified) ? 1u : 0u;
    return n;
  }
  const Instance& slot(unsigned i) const noexcept { return s_[i < kCapacity ? i : 0]; }
  bool valid(int slot_idx, std::uint64_t generation) const noexcept {
    if (slot_idx < 0 || static_cast<unsigned>(slot_idx) >= kCapacity) return false;
    const Instance& i = s_[slot_idx];
    return i.generation == generation && (i.life == Life::Active || i.life == Life::DeathSeen || i.life == Life::Unverified);
  }
  void set_generation_for_test(std::uint64_t g) noexcept { gen_ = g; }

  // PRE de damage_check: asocia (id, ptr) a una instancia. Cuatro casos de reciclaje:
  //  id nuevo+ptr nuevo / id nuevo+ptr viejo (direccion reciclada) / id viejo+ptr distinto (conflicto) / id y ptr iguales tras un retiro.
  Observation observe(std::uint32_t id, std::uintptr_t ptr, Admission adm, std::uint32_t epoch, std::uint64_t call, Events& ev) noexcept {
    Observation o;
    if (id == 0 || id == kIdUnknown || id == kIdError || ptr == 0) {
      ++c.invalid_keys;
      if (Event* e = ev.add(EvKind::InvalidKey)) e->id = id;
      return o;
    }
    for (unsigned i = 0; i < kCapacity; ++i) {
      Instance& s = s_[i];
      if (!is_live(s) || s.id != id) continue;
      if (s.ptr == ptr) {  // misma instancia
        s.last_call = call;
        s.epoch_last = epoch;
        if (s.life == Life::Unverified) {
          s.life = Life::Active;
          s.unv = UnverifiedReason::None;
          s.unavailable_streak = 0;
          ++c.revalidated;
          if (Event* e = ev.add(EvKind::Revalidated)) fill(*e, s, epoch);
        }
        if (s.adm != adm) {
          ++c.admission_changes;
          if (Event* e = ev.add(EvKind::AdmissionChanged)) {
            fill(*e, s, epoch);
            e->from = s.adm;
            e->to = adm;
          }
          s.adm = adm;  // la identidad se conserva
        }
        o.slot = static_cast<int>(i);
        o.generation = s.generation;
        o.tracked = true;
        return o;
      }
      // mismo ID en otra direccion: el estado antiguo NO se traslada
      ++c.identity_conflicts;
      retire(s, RetireReason::IdentityConflict, epoch, ev);
      break;
    }
    // direccion reciclada por otro ID: dos actores no pueden vivir en la misma direccion
    for (unsigned i = 0; i < kCapacity; ++i) {
      Instance& s = s_[i];
      if (is_live(s) && s.ptr == ptr && s.id != id) {
        ++c.address_reused;
        retire(s, RetireReason::AddressReused, epoch, ev);
      }
    }
    const int idx = alloc(epoch, ev);
    if (idx < 0) {
      ++c.capacity_drops;
      if (Event* e = ev.add(EvKind::CapacityExhausted)) e->id = id;
      return o;
    }
    Instance& n = s_[idx];
    n = Instance{};
    n.life = Life::Active;
    n.adm = adm;
    n.id = id;
    n.ptr = ptr;
    n.generation = next_generation();
    n.first_call = n.last_call = call;
    n.epoch_first = n.epoch_last = epoch;
    ++c.records_created;
    const unsigned a = active();
    if (a > c.max_active) c.max_active = a;
    if (Event* e = ev.add(EvKind::FirstSeen)) fill(*e, n, epoch);
    o.slot = idx;
    o.generation = n.generation;
    o.tracked = true;
    o.created = true;
    return o;
  }

  // POST: evidencia de muerte (HP<=0 o modo mortal nativo). NO es borrado.
  void mark_death(int slot_idx, std::uint64_t generation, std::uint32_t epoch, Events& ev) noexcept {
    if (!valid(slot_idx, generation)) return;
    Instance& s = s_[slot_idx];
    if (s.life != Life::Active) return;
    s.life = Life::DeathSeen;
    ++c.death_seen;
    if (Event* e = ev.add(EvKind::DeathSeen)) fill(*e, s, epoch);
  }
  void add_hit(int slot_idx, std::uint64_t generation) noexcept {
    if (valid(slot_idx, generation)) ++s_[slot_idx].hits;
  }
  // Puerta unica de emision de eventos de impacto: la generacion debe seguir viva y el serial del frame debe ser NUEVO para la instancia.
  // false + contador: generacion cambiada/retirada (cross_generation_drops) o serial repetido (duplicates_suppressed).
  bool try_emit(int slot_idx, std::uint64_t generation, std::uint64_t serial) noexcept {
    if (!valid(slot_idx, generation)) return false;  // el motor cuenta cross_generation_drops
    Instance& s = s_[slot_idx];
    if (serial <= s.last_event_serial) {
      ++c.dup_serial_gate;
      return false;
    }
    s.last_event_serial = serial;
    ++s.hits;
    ++c.events_emitted;
    return true;
  }

  // Barrido por presupuesto: devuelve la siguiente instancia a consultar (una vez por `stamp`).
  bool next_sweep(Pick& p, std::uint64_t stamp) noexcept {
    for (unsigned k = 0; k < kCapacity; ++k) {
      const unsigned i = (cursor_ + k) % kCapacity;
      Instance& s = s_[i];
      if (!is_live(s) || s.swept_stamp == stamp) continue;
      s.swept_stamp = stamp;
      cursor_ = (i + 1) % kCapacity;
      p.slot = static_cast<int>(i);
      p.id = s.id;
      p.ptr = s.ptr;
      p.generation = s.generation;
      return true;
    }
    return false;
  }

  void apply_lookup(const Pick& p, Lookup r, std::uint32_t epoch, Events& ev) noexcept {
    ++c.lookups;
    if (p.slot < 0 || static_cast<unsigned>(p.slot) >= kCapacity) {
      ++c.stale_picks;
      return;
    }
    Instance& s = s_[p.slot];
    if (!is_live(s) || s.generation != p.generation || s.id != p.id) {  // el slot cambio entre la eleccion y la aplicacion
      ++c.stale_picks;
      return;
    }
    switch (r) {
      case Lookup::Absent:
        ++c.absent_verified;
        retire(s, RetireReason::LookupAbsentVerified, epoch, ev);
        return;
      case Lookup::PresentMatch:
        s.unavailable_streak = 0;
        if (s.life == Life::Unverified) {
          s.life = Life::Active;
          s.unv = UnverifiedReason::None;
          ++c.revalidated;
          if (Event* e = ev.add(EvKind::Revalidated)) fill(*e, s, epoch);
        }
        return;
      case Lookup::PresentMismatchPtr: set_unverified(s, UnverifiedReason::PtrMismatch, epoch, ev); break;
      case Lookup::PresentMismatchProfile: set_unverified(s, UnverifiedReason::ProfileMismatch, epoch, ev); break;
      case Lookup::Creating: set_unverified(s, UnverifiedReason::Creating, epoch, ev); break;
      case Lookup::Unavailable:
      case Lookup::Failed:
        ++c.lookup_unavailable;
        if (s.unavailable_streak < 255) ++s.unavailable_streak;
        if (s.unavailable_streak >= kUnavailableStreakLimit) set_unverified(s, UnverifiedReason::LookupUnavailable, epoch, ev);
        break;
    }
    // Una instancia NO verificada de un contexto anterior se retira como CONTEXT_RETIRED (sin afirmar borrado nativo).
    if (s.life == Life::Unverified && epoch_older(s.epoch_last, epoch)) {
      ++c.context_retired;
      retire(s, RetireReason::ContextRetired, epoch, ev, true);
    }
  }

  // Throttle por tipo de evento ruidoso (los eventos normales de ciclo de vida no se limitan mas que por el presupuesto).
  bool may_log(EvKind k) noexcept {
    unsigned cap = 0;
    switch (k) {
      case EvKind::AdmissionChanged: cap = 40; break;
      case EvKind::Revalidated: cap = 40; break;
      case EvKind::Unverified: cap = 40; break;
      case EvKind::CapacityExhausted: cap = 20; break;
      case EvKind::InvalidKey: cap = 10; break;
      default: return true;
    }
    unsigned& n = kind_logged_[static_cast<unsigned>(k)];
    if (n >= cap) {
      ++c.kind_throttled;
      return false;
    }
    ++n;
    return true;
  }

 private:
  Instance s_[kCapacity]{};
  std::uint64_t gen_ = 0, retire_seq_ = 0;
  unsigned cursor_ = 0;
  unsigned kind_logged_[10] = {};

  static bool is_live(const Instance& s) noexcept { return s.life == Life::Active || s.life == Life::DeathSeen || s.life == Life::Unverified; }
  static void fill(Event& e, const Instance& s, std::uint32_t epoch) noexcept {
    e.id = s.id;
    e.generation = s.generation;
    e.from = e.to = s.adm;
    e.reason = s.retired;
    e.unv = s.unv;
    e.epoch = epoch;
  }
  std::uint64_t next_generation() noexcept {
    ++gen_;
    if (gen_ == 0) {  // wrap (practicamente inalcanzable): 0 queda reservado como «sin generacion»
      ++c.generation_rollovers;
      gen_ = 1;
    }
    return gen_;
  }
  void retire(Instance& s, RetireReason why, std::uint32_t epoch, Events& ev, bool context = false) noexcept {
    s.life = Life::Retired;
    s.retired = why;
    s.retire_seq = ++retire_seq_;
    if (Event* e = ev.add(EvKind::Retired)) {
      fill(*e, s, epoch);
      e->reason = why;
      e->context_changed = context || epoch_older(s.epoch_last, epoch);
    }
  }
  void set_unverified(Instance& s, UnverifiedReason why, std::uint32_t epoch, Events& ev) noexcept {
    if (s.life == Life::Unverified && s.unv == why) return;
    s.life = Life::Unverified;
    s.unv = why;
    ++c.unverified;
    if (Event* e = ev.add(EvKind::Unverified)) fill(*e, s, epoch);
  }
  // FREE, luego RETIRED mas antiguo, luego UNVERIFIED mas antiguo (evicion explicita, no es borrado). Nunca un ACTIVO/DEATH_SEEN.
  int alloc(std::uint32_t epoch, Events& ev) noexcept {
    for (unsigned i = 0; i < kCapacity; ++i)
      if (s_[i].life == Life::Free) return static_cast<int>(i);
    int best = -1;
    for (unsigned i = 0; i < kCapacity; ++i)
      if (s_[i].life == Life::Retired && (best < 0 || s_[i].retire_seq < s_[best].retire_seq)) best = static_cast<int>(i);
    if (best >= 0) return best;
    for (unsigned i = 0; i < kCapacity; ++i)
      if (s_[i].life == Life::Unverified && (best < 0 || s_[i].last_call < s_[best].last_call)) best = static_cast<int>(i);
    if (best >= 0) {
      ++c.capacity_evictions;
      retire(s_[best], RetireReason::CapacityEvicted, epoch, ev);
      return best;
    }
    return -1;
  }
};

// ---- presupuesto de log ----------------------------------------------------------------------------------------
struct LogBudget {
  unsigned used[4] = {};
  bool take(LogCat cat, Counters& k) noexcept {
    const unsigned i = static_cast<unsigned>(cat);
    if (used[i] >= kBudget[i]) {
      ++k.log_dropped[i];
      return false;
    }
    ++used[i];
    ++k.log_written[i];
    return true;
  }
};

// ---- pila PRE/POST con generacion ------------------------------------------------------------------------------
struct CallFrame {
  std::uintptr_t actor = 0;
  std::uint32_t id = 0;
  std::uint64_t generation = 0, serial = 0;
  int slot = -1;
  bool tracked = false;  // tiene instancia en el ledger
  bool alive = false;    // perfil y estado permiten leer health/modo en el POST
  bool detail = false;   // candidato E_OC con snapshot PRE valido
  bool started = false;  // contado en cand_started / finish_started (conservacion aritmetica)
  PreClass cls = PreClass::UNKNOWN;
  Snapshot pre{};
  std::uintptr_t raw0 = 0, raw1 = 0;  // hit obj CRUDO de cada esfera en el PRE: valores opacos de ESTA llamada, solo para comparar
  NativeAux aux_pre{};                // POST05: lecturas auxiliares PRE (por valor)
};

class FrameStack {
 public:
  CallFrame* push(std::uintptr_t actor, std::uint32_t id, Counters& c) noexcept {
    // Un PRE que llega a un actor que ya tiene frame abierto (damage_check no es recursivo) o un frame mucho mas antiguo que el serial actual
    // indican un POST perdido: se descartan como OBSOLETOS (contados), para que un POST perdido no ocupe la pila para siempre.
    unsigned w = 0;
    for (unsigned i = 0; i < depth_; ++i) {
      const bool stale = frames_[i].actor == actor || frames_[i].serial + kStaleAge < serial_ + 1;
      if (stale) {
        ++c.stale_frames;
        ++c.discarded_frames;
        if (frames_[i].started) (frames_[i].cls == PreClass::CUT_DOWN_FINISH_PATH ? c.finish_discarded : c.cand_discarded)++;
      } else {
        if (w != i) frames_[w] = frames_[i];
        ++w;
      }
    }
    depth_ = w;
    if (depth_ >= kFrameCapacity) {
      ++c.frame_overflow;
      return nullptr;
    }
    CallFrame& f = frames_[depth_++];
    f = CallFrame{};
    f.actor = actor;
    f.id = id;
    f.serial = ++serial_;
    return &f;
  }
  // POST: busca el frame mas reciente del mismo actor y descarta los apilados encima (contando los candidatos perdidos).
  bool end(std::uintptr_t actor, CallFrame& out, Counters& c) noexcept {
    for (unsigned i = depth_; i-- > 0;) {
      if (frames_[i].actor == actor) {
        for (unsigned k = i + 1; k < depth_; ++k) {
          ++c.discarded_frames;
          if (frames_[k].started) (frames_[k].cls == PreClass::CUT_DOWN_FINISH_PATH ? c.finish_discarded : c.cand_discarded)++;
        }
        out = frames_[i];
        depth_ = i;
        remember(out);
        return true;
      }
    }
    // POST sin frame: ¿repeticion de un frame ya cerrado (duplicado) o POST huerfano?
    for (unsigned k = 0; k < kDoneRing; ++k) {
      if (done_[k].actor == actor && done_[k].serial != 0) {
        ++c.dup_post_orphan;
        return false;
      }
    }
    ++c.unmatched_post;
    return false;
  }
  unsigned depth() const noexcept { return depth_; }
  // candidatos aun en vuelo (para cerrar la conservacion al apagar)
  unsigned pending_started() const noexcept {
    unsigned n = 0;
    for (unsigned i = 0; i < depth_; ++i) n += frames_[i].started ? 1u : 0u;
    return n;
  }
  void clear_done_for_test() noexcept {
    for (auto& d : done_) d = Done{};
  }

 private:
  static constexpr unsigned kDoneRing = 4;
  static constexpr std::uint64_t kStaleAge = 1000;
  struct Done {
    std::uintptr_t actor = 0;
    std::uint64_t serial = 0;
  };
  void remember(const CallFrame& f) noexcept {
    // un actor aparece una sola vez en el anillo: el POST duplicado del frame mas reciente de ese actor se reconoce
    for (auto& d : done_) {
      if (d.actor == f.actor) {
        d.serial = f.serial;
        return;
      }
    }
    done_[done_next_] = {f.actor, f.serial};
    done_next_ = (done_next_ + 1) % kDoneRing;
  }
  CallFrame frames_[kFrameCapacity]{};
  Done done_[kDoneRing]{};
  unsigned done_next_ = 0;
  unsigned depth_ = 0;
  std::uint64_t serial_ = 0;
};

class Observer {
 public:
  Ledger ledger;
  FrameStack frames;
  LogBudget budget;
  EpochTracker epoch;
  bool disabled = false, disabled_logged = false;
  const char* disabled_reason = "";
  std::uint64_t next_summary_at = kSummaryPeriodCalls;
  std::uint64_t tick = 0, sweep_stamp = 0;

 private:
  unsigned hit_logged_[8] = {};

 public:

  void disable(const char* reason) noexcept {
    if (!disabled) {
      disabled = true;
      disabled_reason = reason;
    }
  }
  // Tope por tipo de evento «no normal» (los confirmados y los finish nativos solo estan limitados por el presupuesto).
  bool may_log_hit(HitKind k) noexcept {
    unsigned cap = 0;
    switch (k) {
      case HitKind::CandidateNotProcessed: cap = 30; break;
      case HitKind::ProcessedMismatch: cap = 30; break;
      case HitKind::FinishUnexpected: cap = 30; break;
      default: return true;
    }
    unsigned& n = hit_logged_[static_cast<unsigned>(k)];
    if (n >= cap) {
      ++ledger.c.kind_throttled;
      return false;
    }
    ++n;
    return true;
  }
  bool summary_due() noexcept {
    if (ledger.c.calls < next_summary_at) return false;
    next_summary_at = ledger.c.calls + kSummaryPeriodCalls;
    return true;
  }
  // true cuando toca barrer en este mod_update
  bool sweep_due() noexcept {
    ++tick;
    if (tick % kSweepPeriodTicks != 0) return false;
    ++sweep_stamp;
    return true;
  }
};

// ---- instalacion (plantilla: la lab la prueba con dobles) ------------------------------------------------------
enum class InstallState : std::uint8_t { Ok, PostFailed, PreFailedRolledBack, PreFailedRollbackFailed };
struct InstallOutcome {
  InstallState state = InstallState::Ok;
  int post = 0, pre = 0, rollback = 0;
};
template <class AddPost, class AddPre, class Uninstall>
InstallOutcome install_sequence(AddPost add_post, AddPre add_pre, Uninstall uninstall) noexcept {
  InstallOutcome o;
  o.post = add_post();
  if (o.post != 0) {
    o.state = InstallState::PostFailed;
    return o;
  }
  o.pre = add_pre();
  if (o.pre != 0) {
    o.rollback = uninstall();
    o.state = o.rollback == 0 ? InstallState::PreFailedRolledBack : InstallState::PreFailedRollbackFailed;
  }
  return o;
}
inline const char* name(InstallState s) noexcept {
  switch (s) {
    case InstallState::Ok: return "OK";
    case InstallState::PostFailed: return "POST_FAILED";
    case InstallState::PreFailedRolledBack: return "PRE_FAILED_ROLLED_BACK";
    default: return "PRE_FAILED_ROLLBACK_FAILED";
  }
}

// ---- formato de lineas (sin punteros; la generacion y el id identifican la instancia) ----------------------------
inline int fmt_event(char* out, std::size_t n, const Event& e, std::uint64_t call) noexcept {
  const unsigned long long cl = static_cast<unsigned long long>(call), gen = static_cast<unsigned long long>(e.generation);
  switch (e.kind) {
    case EvKind::FirstSeen:
      return std::snprintf(out, n,
                           "POST03_INSTANCE_FIRST_SEEN call=%llu id=%u generation=%llu epoch=%u adm=%s reason=FIRST_HOOK_CALL origin=%s "
                           "(not SPAWN_CONFIRMED)",
                           cl, e.id, gen, e.epoch, name(e.to), is_candidate(e.to) ? "UNDETERMINED" : "NA");
    case EvKind::AdmissionChanged:
      return std::snprintf(out, n, "POST03_ADMISSION_CHANGED call=%llu id=%u generation=%llu from=%s to=%s identity=KEPT", cl, e.id, gen,
                           name(e.from), name(e.to));
    case EvKind::DeathSeen:
      return std::snprintf(out, n, "POST03_INSTANCE_DEATH_SEEN call=%llu id=%u generation=%llu evidence=HP_OR_NATIVE_MODE (NO_DELETE_CLAIM)", cl,
                           e.id, gen);
    case EvKind::Revalidated:
      return std::snprintf(out, n, "POST03_INSTANCE_REVALIDATED call=%llu id=%u generation=%llu", cl, e.id, gen);
    case EvKind::Retired:
      return std::snprintf(out, n, "POST03_INSTANCE_RETIRED call=%llu id=%u generation=%llu reason=%s delete_claim=%d context_changed=%d epoch=%u",
                           cl, e.id, gen, name(e.reason), delete_claim(e.reason) ? 1 : 0, e.context_changed ? 1 : 0, e.epoch);
    case EvKind::Unverified:
      return std::snprintf(out, n, "POST03_INSTANCE_UNVERIFIED call=%llu id=%u generation=%llu reason=%s", cl, e.id, gen, name(e.unv));
    case EvKind::CapacityExhausted:
      return std::snprintf(out, n, "POST03_CAPACITY_EXHAUSTED call=%llu id=%u (new candidate not tracked; original untouched)", cl, e.id);
    case EvKind::InvalidKey:
      return std::snprintf(out, n, "POST03_INVALID_KEY call=%llu id=0x%08x (not tracked)", cl, e.id);
    default: return 0;
  }
}

// ---- eventos de impacto y resumenes -------------------------------------------------------------------------------
struct HitReport {
  std::uint64_t event = 0, call = 0, generation = 0;
  std::uint32_t id = 0;
  Life life = Life::Active;
  PreClass cls = PreClass::UNKNOWN;
  Snapshot pre{}, post{};
  Judgement j{};
  ColliderRes col{};
  bool at_valid = false;
  std::uint32_t at_type = 0;
  FamilyResult fam{};
  const char* fam_reason = "NOT_EVALUATED";  // por que la familia no esta disponible
  int slot = -1;                              // POST04: slot del ledger de esta instancia (clave junto con id + generation)
  NativeAux aux_pre{}, aux_post{};            // POST05: lecturas auxiliares PRE/POST de ESTA llamada (por valor)
};
inline const char* branch_name(HitKind k) noexcept {
  switch (k) {
    case HitKind::FinishNative:
    case HitKind::FinishUnexpected: return "FINISH";
    case HitKind::Confirmed:
    case HitKind::ProcessedMismatch: return "PROCESSED";
    case HitKind::CandidateNotProcessed: return "UNPROCESSED_EARLY_EXIT";
    default: return "NONE";
  }
}
inline int fmt_hit_event(char* out, std::size_t n, const HitReport& r) noexcept {
  char at[24];
  if (r.at_valid) std::snprintf(at, sizeof(at), "0x%08x", static_cast<unsigned>(r.at_type));
  else std::snprintf(at, sizeof(at), "UNAVAILABLE");
  const bool fam_ok = r.fam.family != Family::Unavailable;
  const char* src = r.col.src == 1 ? "cc1" : (r.col.src == 0 ? "cc0" : "none");
  return std::snprintf(
      out, n,
      "POST03_HIT_EVENT event=%llu call=%llu id=%u generation=%llu life=%s kind=%s branch=%s cc_at_check=UNDETERMINED origin=UNDETERMINED "
      "tg=%s dual=%d distinct=%d collider_src=%s collider=%s sel_class=%s raw_class=%s/%s family=%s native_first=%s basis=%s at=%s hp=%d>%d lethal=%d mode=%d>%d old_mode=%d "
      "state=%d>%d cd=%d>%d cutdown=%d eligible=%s reason=%s",
      static_cast<unsigned long long>(r.event), static_cast<unsigned long long>(r.call), r.id, static_cast<unsigned long long>(r.generation),
      name(r.life), name(r.j.kind), branch_name(r.j.kind), tg_text(r.pre.tg_bits), r.j.dual_tg ? 1 : 0, r.col.pair_distinct ? 1 : 0, src,
      name(r.col.reason), name(r.col.sel_class), name(r.col.raw0_class), name(r.col.raw1_class), fam_ok ? name(r.fam.family) : "UNAVAILABLE", fam_ok ? name(r.fam.native_first) : "NA",
      fam_ok ? r.fam.reason : r.fam_reason, at, r.pre.health, r.post.health, r.j.lethal ? 1 : 0, r.pre.mode, r.post.mode, r.post.old_mode,
      r.pre.state, r.post.state, r.pre.cooldown, r.post.cooldown, r.pre.cutdown, name(r.j.elig), r.j.reason);
}

inline int fmt_summary(char* out, std::size_t n, const Counters& k, unsigned active_now, std::uint32_t epoch, unsigned pending, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  return std::snprintf(
      out, n,
      "POST03_SUMMARY v=0.6.0 tag=%s calls=%llu adm(cand/e_oc2/other/unsafe/e_oc2_cand)=%llu/%llu/%llu/%llu/%llu records_created=%llu active=%u max_active=%llu "
      "death_seen=%llu absent_verified=%llu context_retired=%llu address_reused=%llu identity_conflicts=%llu capacity_drops=%llu "
      "capacity_evictions=%llu unverified=%llu revalidated=%llu lookups=%llu lookup_unavailable=%llu generation_rollovers=%llu "
      "admission_changes=%llu invalid_keys=%llu stale_picks=%llu epoch=%u unmatched_post=%llu discarded=%llu frame_overflow=%llu "
      "post_generation_mismatch=%llu stale_frames=%llu pending=%u log(life/hit/other/summary written)=%llu/%llu/%llu/%llu dropped=%llu/%llu/%llu/%llu "
      "throttled=%llu log_errors=%llu",
      tag, u(k.calls), u(k.calls_adm[0]), u(k.calls_adm[1]), u(k.calls_adm[2]), u(k.calls_adm[3]), u(k.calls_adm[4]), u(k.records_created), active_now,
      u(k.max_active), u(k.death_seen), u(k.absent_verified), u(k.context_retired), u(k.address_reused), u(k.identity_conflicts),
      u(k.capacity_drops), u(k.capacity_evictions), u(k.unverified), u(k.revalidated), u(k.lookups), u(k.lookup_unavailable),
      u(k.generation_rollovers), u(k.admission_changes), u(k.invalid_keys), u(k.stale_picks), epoch, u(k.unmatched_post),
      u(k.discarded_frames), u(k.frame_overflow), u(k.post_generation_mismatch), u(k.stale_frames), pending, u(k.log_written[0]), u(k.log_written[1]),
      u(k.log_written[2]), u(k.log_written[3]), u(k.log_dropped[0]), u(k.log_dropped[1]), u(k.log_dropped[2]), u(k.log_dropped[3]),
      u(k.kind_throttled), u(k.log_errors));
}

inline int fmt_hit_summary(char* out, std::size_t n, const Counters& k, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  return std::snprintf(
      out, n,
      "POST03_HIT_SUMMARY v=0.6.0 tag=%s pre(finish/cooldown/no_tg/tg/unknown)=%llu/%llu/%llu/%llu/%llu cand_started=%llu raw_tg=%llu dual_tg=%llu "
      "pair_distinct=%llu confirmed=%llu processed_mismatch=%llu not_processed=%llu finish_started=%llu finish=%llu finish_unexpected=%llu "
      "cooldown_blocked=%llu cooldown_with_tg=%llu no_tg=%llu excluded=%llu unknown_unsafe=%llu duplicates_suppressed=%llu "
      "cross_generation_drops=%llu cand_discarded=%llu finish_discarded=%llu classification_unavailable=%llu collider_unmatched=%llu "
      "family(sword/shield/arrow/bomb/iron/boomerang/at40/sling/other/ambiguous/unavail)=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
      "lethal=%llu hp_changed=%llu anomalies=%llu events_emitted=%llu",
      tag, u(k.pre_class[0]), u(k.pre_class[1]), u(k.pre_class[2]), u(k.pre_class[3]), u(k.pre_class[4]), u(k.cand_started), u(k.raw_tg),
      u(k.dual_tg), u(k.pair_distinct), u(k.confirmed), u(k.processed_mismatch), u(k.not_processed), u(k.finish_started), u(k.finish),
      u(k.finish_unexpected), u(k.cooldown_blocked), u(k.cooldown_with_tg), u(k.no_tg), u(k.excluded_calls), u(k.unknown_unsafe_calls),
      u(k.dup_serial_gate + k.dup_post_orphan), u(k.cross_generation_drops), u(k.cand_discarded), u(k.finish_discarded), u(k.classification_unavailable),
      u(k.collider_unmatched), u(k.family_count[1]), u(k.family_count[2]), u(k.family_count[3]), u(k.family_count[4]), u(k.family_count[5]),
      u(k.family_count[6]), u(k.family_count[7]), u(k.family_count[8]), u(k.family_count[9]), u(k.family_count[10]), u(k.family_count[0]),
      u(k.lethal), u(k.hp_changed), u(k.anomalies), u(k.events_emitted));
}

inline int fmt_collider_diag(char* out, std::size_t n, const Counters& k, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  return std::snprintf(
      out, n,
      "POST03_1_COLLIDER_DIAG v=0.6.0 tag=%s confirmed=%llu reason(no_selected/too_low/alignment/raw_null/not_equal_raw/match_cc0/match_cc1)="
      "%llu/%llu/%llu/%llu/%llu/%llu/%llu sel_class(null/too_low/mod8_0/mod8_4/mod4_bad)=%llu/%llu/%llu/%llu/%llu "
      "raw_cc0_class=%llu/%llu/%llu/%llu/%llu raw_cc1_class=%llu/%llu/%llu/%llu/%llu deref_ok_mod8_0=%llu deref_ok_mod8_4=%llu "
      "classification_unavailable=%llu",
      tag, u(k.confirmed), u(k.col_reason[0]), u(k.col_reason[1]), u(k.col_reason[2]), u(k.col_reason[3]), u(k.col_reason[4]), u(k.col_reason[5]),
      u(k.col_reason[6]), u(k.sel_class[0]), u(k.sel_class[1]), u(k.sel_class[2]), u(k.sel_class[3]), u(k.sel_class[4]), u(k.raw0_class[0]),
      u(k.raw0_class[1]), u(k.raw0_class[2]), u(k.raw0_class[3]), u(k.raw0_class[4]), u(k.raw1_class[0]), u(k.raw1_class[1]), u(k.raw1_class[2]),
      u(k.raw1_class[3]), u(k.raw1_class[4]), u(k.deref_mod8_0), u(k.deref_mod8_4), u(k.classification_unavailable));
}

}  // namespace post03
