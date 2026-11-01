#pragma once
// Enemy Posture System, POST01: logica PURA del observador pasivo (sin cabeceras del juego, sin SDK).
// mod.cpp lee el juego y llama a estas funciones; tests/core_lab.cpp las ejercita con dobles.
// Nada aqui escribe en memoria del juego: solo en las estructuras privadas del mod.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace post01 {

inline constexpr const char* kModVersion = "0.1.0";
inline constexpr unsigned kLedgerCapacity = 16;   // POST02 sustituira esto por identidad completa
inline constexpr unsigned kFrameCapacity = 4;     // pila acotada de invocaciones PRE/POST en curso
inline constexpr unsigned kLogLineMax = 512;
inline constexpr std::uint64_t kSummaryPeriodCalls = 18000;
inline constexpr unsigned kDisableLogErrors = 3;  // errores consecutivos de log antes de apagar el observador

// ---- log: presupuesto duro por categoria (suma 2000 lineas por sesion) ---------------------------------
enum class LogCat : std::uint8_t { Seen = 0, Hit = 1, Other = 2, Summary = 3 };
inline constexpr unsigned kBudget[4] = {400, 1200, 300, 100};
inline constexpr unsigned kBudgetTotal = 400 + 1200 + 300 + 100;

// ---- admision --------------------------------------------------------------------------------------------
enum class Admission : std::uint8_t { E_OC_CANDIDATE = 0, E_OC2_EXCLUDED = 1, PROFILE_OTHER = 2, UNKNOWN_UNSAFE = 3 };
inline const char* name(Admission a) noexcept {
  switch (a) {
    case Admission::E_OC_CANDIDATE: return "E_OC_CANDIDATE";
    case Admission::E_OC2_EXCLUDED: return "E_OC2_EXCLUDED";
    case Admission::PROFILE_OTHER: return "PROFILE_OTHER";
    default: return "UNKNOWN_UNSAFE";
  }
}

enum class NameKind : std::uint8_t { Unread, E_OC, E_OC2, Other };

// Lectura ACOTADA de mName: como maximo 6 bytes y se detiene en el primer NUL.
inline NameKind classify_name(const char* s) noexcept {
  if (!s) return NameKind::Unread;
  char b[7] = {};
  for (unsigned i = 0; i < 6; ++i) {
    b[i] = s[i];
    if (!s[i]) break;
  }
  if (!std::strcmp(b, "E_OC")) return NameKind::E_OC;
  if (!std::strcmp(b, "E_OC2")) return NameKind::E_OC2;
  return NameKind::Other;
}

// Hechos que mod.cpp lee del actor. El HP NO forma parte de la identidad (HM/DEVF/dano previo lo alteran).
// atp: AtAtp comun de mSphs_at[0/1]; -1 = no leido, -2 = las dos esferas discrepan.
struct ActorFacts {
  bool profile_ok = false;
  bool live = false;
  NameKind name = NameKind::Unread;
  bool status_10000 = false;
  int atp = -1;
};
struct AdmissionResult {
  Admission adm;
  const char* reason;
};
inline AdmissionResult admit(const ActorFacts& f) noexcept {
  if (!f.profile_ok) return {Admission::PROFILE_OTHER, "PROFILE_NOT_E_OC"};
  if (!f.live) return {Admission::UNKNOWN_UNSAFE, "ACTOR_NOT_LIVE"};
  switch (f.name) {
    case NameKind::Unread: return {Admission::UNKNOWN_UNSAFE, "NAME_UNAVAILABLE"};
    case NameKind::E_OC2: return {Admission::E_OC2_EXCLUDED, "RESOURCE_E_OC2"};
    case NameKind::Other: return {Admission::UNKNOWN_UNSAFE, "RESOURCE_UNRECOGNIZED"};
    case NameKind::E_OC: break;
  }
  if (!f.status_10000) return {Admission::UNKNOWN_UNSAFE, "E_OC_STATUS_10000_ABSENT"};
  if (f.atp == -2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_SPHERES_DISAGREE"};
  if (f.atp != 1) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_NOT_1"};
  // Un E_OC nativo y un Blue de DEVF producen exactamente esta firma: el origen NO se puede deducir.
  return {Admission::E_OC_CANDIDATE, "E_OC_SIGNATURE_MATCH"};
}

// ---- clasificacion PRE de la llamada ----------------------------------------------------------------------
enum class PreClass : std::uint8_t {
  CUT_DOWN_FINISH_PATH = 0,
  COOLDOWN_NO_PROCESS = 1,
  NO_TG_CANDIDATE = 2,
  TG_CANDIDATE = 3,
  UNKNOWN = 4
};
inline const char* name(PreClass c) noexcept {
  switch (c) {
    case PreClass::CUT_DOWN_FINISH_PATH: return "CUT_DOWN_FINISH_PATH";
    case PreClass::COOLDOWN_NO_PROCESS: return "COOLDOWN_NO_PROCESS";
    case PreClass::NO_TG_CANDIDATE: return "NO_TG_CANDIDATE";
    case PreClass::TG_CANDIDATE: return "TG_CANDIDATE";
    default: return "UNKNOWN";
  }
}
// Mismo orden que daE_OC_c::damage_check (d_a_e_oc.cpp:639-670): CutDownHit, luego field_0x6cc, luego TG.
inline PreClass classify_pre(bool readable, bool cutdown, int cooldown, bool tg0, bool tg1) noexcept {
  if (!readable) return PreClass::UNKNOWN;
  if (cutdown) return PreClass::CUT_DOWN_FINISH_PATH;
  if (cooldown != 0) return PreClass::COOLDOWN_NO_PROCESS;
  if (tg0 || tg1) return PreClass::TG_CANDIDATE;
  return PreClass::NO_TG_CANDIDATE;
}
// Con los dos TG activos la fuente usa el collider de cc[1] (d_a_e_oc.cpp:662-666). Devuelve 0, 1 o -1.
inline int tg_source(std::uint8_t tg_bits) noexcept {
  if (tg_bits & 2u) return 1;
  if (tg_bits & 1u) return 0;
  return -1;
}
inline const char* tg_text(std::uint8_t tg_bits) noexcept {
  switch (tg_bits & 3u) {
    case 1: return "cc0";
    case 2: return "cc1";
    case 3: return "cc0+cc1";
    default: return "none";
  }
}

// ---- snapshots y evaluacion --------------------------------------------------------------------------------
struct Snapshot {
  std::int32_t health = 0, mode = 0, state = 0, cooldown = 0;
  std::uint8_t cutdown = 0, tg_bits = 0;
};

enum class Verdict : std::uint8_t {
  NOT_EVALUATED = 0,
  ACCEPTED_LIKELY,
  PROCESSED_NO_VISIBLE_CHANGE,
  EARLY_RETURN_POSSIBLE,
  FINISH_PATH_OBSERVED,
  FINISH_PATH_UNCONFIRMED,
  UNKNOWN,
  // Reservado: POST01 NUNCA lo emite (ver HOOK_ABI_PASSIVITY.md); lo cerrara POST03 con pruebas humanas.
  ACCEPTED_CONFIRMED
};
inline const char* name(Verdict v) noexcept {
  switch (v) {
    case Verdict::ACCEPTED_LIKELY: return "ACCEPTED_LIKELY";
    case Verdict::PROCESSED_NO_VISIBLE_CHANGE: return "PROCESSED_NO_VISIBLE_CHANGE";
    case Verdict::EARLY_RETURN_POSSIBLE: return "EARLY_RETURN_POSSIBLE";
    case Verdict::FINISH_PATH_OBSERVED: return "FINISH_PATH_OBSERVED";
    case Verdict::FINISH_PATH_UNCONFIRMED: return "FINISH_PATH_UNCONFIRMED";
    case Verdict::UNKNOWN: return "UNKNOWN";
    case Verdict::ACCEPTED_CONFIRMED: return "ACCEPTED_CONFIRMED";
    default: return "NOT_EVALUATED";
  }
}
inline constexpr std::int32_t kModeDeath = 9;  // E_OC_ACTION_DEATH (d_a_e_oc.cpp:30, confirmado en el binario)

struct Evaluation {
  Verdict verdict = Verdict::NOT_EVALUATED;
  const char* reason = "";
  bool transition = false;  // cambio de HP, modo, estado o puesta del cooldown
  bool processed = false;   // field_0x6cc paso de 0 a !=0: solo damage_check lo escribe (lineas 674/676/680/774)
  bool lethal = false;
  bool anomaly = false;     // cambio de estado en una llamada sin candidato
  bool emit = false;        // debe generar linea HIT
};

inline Evaluation evaluate(PreClass cls, const Snapshot& pre, const Snapshot& post) noexcept {
  Evaluation e;
  const bool hp = pre.health != post.health, mode = pre.mode != post.mode, st = pre.state != post.state;
  e.processed = pre.cooldown == 0 && post.cooldown != 0;
  e.transition = hp || mode || st || e.processed;
  e.lethal = post.health <= 0;
  switch (cls) {
    case PreClass::TG_CANDIDATE:
      e.emit = true;
      if (!e.processed) {
        e.verdict = Verdict::EARLY_RETURN_POSSIBLE;
        e.reason = "COOLDOWN_NOT_SET_AFTER_TG";
      } else if (hp || mode || st) {
        e.verdict = Verdict::ACCEPTED_LIKELY;
        e.reason = hp ? (mode ? "HP_AND_MODE_CHANGED" : "HP_CHANGED") : (mode ? "MODE_CHANGED" : "STATE_CHANGED");
      } else {
        e.verdict = Verdict::PROCESSED_NO_VISIBLE_CHANGE;
        e.reason = "COOLDOWN_SET_NO_HP_MODE_STATE_CHANGE";
      }
      break;
    case PreClass::CUT_DOWN_FINISH_PATH:
      e.emit = true;
      if (post.health == 0 && post.mode == kModeDeath) {
        e.verdict = Verdict::FINISH_PATH_OBSERVED;
        e.reason = "HEALTH0_MODE_DEATH";
      } else {
        e.verdict = Verdict::FINISH_PATH_UNCONFIRMED;
        e.reason = "EXPECTED_DEATH_STATE_NOT_SEEN";
      }
      break;
    case PreClass::COOLDOWN_NO_PROCESS:
    case PreClass::NO_TG_CANDIDATE:
      e.anomaly = hp || mode || st;  // damage_check no deberia cambiar nada sin TG ni Finish
      break;
    default:
      e.verdict = Verdict::UNKNOWN;
      e.reason = "PRE_UNREADABLE";
      break;
  }
  return e;
}

// ---- contadores, presupuesto de log, ledger, pila de invocaciones ------------------------------------------
struct Counters {
  std::uint64_t calls = 0, calls_adm[4] = {}, pre_class[5] = {};
  std::uint64_t hit_events = 0, processed = 0, accepted_likely = 0, early_return = 0, no_visible = 0, finish = 0;
  std::uint64_t anomalies = 0, unmatched_post = 0, discarded_frames = 0, frame_overflow = 0;
  std::uint64_t ledger_inserts = 0, ledger_wraps = 0, id_ptr_mismatch = 0;
  std::uint64_t log_written[4] = {}, log_dropped[4] = {}, log_errors = 0;
};

struct LogBudget {
  unsigned used[4] = {};
  bool take(LogCat c, Counters& k) noexcept {
    const unsigned i = static_cast<unsigned>(c);
    if (used[i] >= kBudget[i]) {
      ++k.log_dropped[i];
      return false;
    }
    ++used[i];
    ++k.log_written[i];
    return true;
  }
};

struct CallFrame {
  std::uintptr_t actor = 0;
  std::uint32_t id = 0;
  std::uint64_t serial = 0;
  Admission adm = Admission::UNKNOWN_UNSAFE;
  PreClass cls = PreClass::UNKNOWN;
  bool detail = false;  // hay snapshot PRE valido (solo candidatos)
  Snapshot pre{};
};

enum class Touch : std::uint8_t { Inserted, Known, PtrMismatch };

class Observer {
 public:
  Counters c{};
  LogBudget budget{};
  bool disabled = false;
  bool disabled_logged = false;
  const char* disabled_reason = "";
  std::uint64_t next_summary_at = kSummaryPeriodCalls;

  void disable(const char* reason) noexcept {
    if (!disabled) {
      disabled = true;
      disabled_reason = reason;
    }
  }

  // Ledger acotado (NO es la gestion de identidad de POST02): evita repetir ACTOR_SEEN y detecta ID con otro puntero.
  Touch touch(std::uint32_t id, std::uintptr_t ptr) noexcept {
    for (unsigned i = 0; i < kLedgerCapacity; ++i) {
      if (ledger_[i].used && ledger_[i].id == id) {
        if (ledger_[i].ptr == ptr) return Touch::Known;
        ledger_[i].ptr = ptr;
        ++c.id_ptr_mismatch;
        return Touch::PtrMismatch;
      }
    }
    if (ledger_[next_slot_].used) ++c.ledger_wraps;
    ledger_[next_slot_] = {true, id, ptr};
    next_slot_ = (next_slot_ + 1) % kLedgerCapacity;
    ++c.ledger_inserts;
    return Touch::Inserted;
  }

  // PRE: apila la invocacion. Devuelve nullptr si la pila esta saturada (fail-open: nada se almacena).
  CallFrame* push(std::uintptr_t actor, std::uint32_t id) noexcept {
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

  // POST: empareja con la invocacion MAS RECIENTE del mismo actor y descarta las apiladas encima.
  bool end(std::uintptr_t actor, CallFrame& out) noexcept {
    for (unsigned i = depth_; i-- > 0;) {
      if (frames_[i].actor == actor) {
        c.discarded_frames += depth_ - 1 - i;
        out = frames_[i];
        depth_ = i;
        return true;
      }
    }
    ++c.unmatched_post;
    return false;
  }

  unsigned depth() const noexcept { return depth_; }
  bool summary_due() noexcept {
    if (c.calls < next_summary_at) return false;
    next_summary_at = c.calls + kSummaryPeriodCalls;
    return true;
  }

 private:
  struct Entry {
    bool used = false;
    std::uint32_t id = 0;
    std::uintptr_t ptr = 0;
  };
  Entry ledger_[kLedgerCapacity]{};
  unsigned next_slot_ = 0;
  CallFrame frames_[kFrameCapacity]{};
  unsigned depth_ = 0;
  std::uint64_t serial_ = 0;
};

// ---- secuencia de instalacion (plantilla: la lab la prueba con dobles, mod.cpp con el SDK real) --------------
enum class InstallState : std::uint8_t { Ok, PostFailed, PreFailedRolledBack, PreFailedRollbackFailed };
struct InstallOutcome {
  InstallState state = InstallState::Ok;
  int post = 0, pre = 0, rollback = 0;
};
template <class AddPost, class AddPre, class Uninstall>
InstallOutcome install_sequence(AddPost add_post, AddPre add_pre, Uninstall uninstall) noexcept {
  InstallOutcome o;
  // POST primero: ningun efecto PRE puede existir sin su POST (precedente DEVF); ambos son observacionales.
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

// ---- formato de lineas de log (stack buffers, sin punteros, "UNKNOWN" nunca como 0) -------------------------
struct Num {
  char t[16];
  Num(bool valid, long v) noexcept {
    if (valid) std::snprintf(t, sizeof(t), "%ld", v);
    else std::snprintf(t, sizeof(t), "UNKNOWN");
  }
};

inline int fmt_seen(char* out, std::size_t n, std::uint64_t call, std::uint32_t id, Admission a, const char* reason,
                    bool have_detail, const Snapshot& s) noexcept {
  const Num hp(have_detail, s.health), md(have_detail, s.mode);
  return std::snprintf(out, n,
                       "POST01_ACTOR_SEEN call=%llu actor_id=%u adm=%s reason=%s origin=%s hp_info=%s mode_info=%s",
                       static_cast<unsigned long long>(call), id, name(a), reason,
                       a == Admission::E_OC_CANDIDATE ? "UNDETERMINED" : "NA", hp.t, md.t);
}

inline int fmt_hit(char* out, std::size_t n, std::uint64_t call, std::uint32_t id, PreClass cls, const Snapshot& pre,
                   const Snapshot& post, const Evaluation& e) noexcept {
  return std::snprintf(
      out, n,
      "POST01_HIT_OBSERVED call=%llu actor_id=%u adm=E_OC_CANDIDATE origin=UNDETERMINED pre=%s tg=%s at=OMITTED "
      "hp=%d>%d mode=%d>%d state=%d>%d cd=%d>%d cutdown=%d>%d transition=%d processed=%d lethal=%d verdict=%s reason=%s",
      static_cast<unsigned long long>(call), id, name(cls), tg_text(pre.tg_bits), pre.health, post.health,
      pre.mode, post.mode, pre.state, post.state, pre.cooldown, post.cooldown, pre.cutdown, post.cutdown,
      e.transition ? 1 : 0, e.processed ? 1 : 0, e.lethal ? 1 : 0, name(e.verdict), e.reason);
}

inline int fmt_summary(char* out, std::size_t n, const Counters& k, const char* tag) noexcept {
  return std::snprintf(
      out, n,
      "POST01_SUMMARY tag=%s calls=%llu adm(cand/e_oc2/other/unsafe)=%llu/%llu/%llu/%llu "
      "pre(finish/cooldown/no_tg/tg/unknown)=%llu/%llu/%llu/%llu/%llu hits=%llu processed=%llu accepted_likely=%llu "
      "early_return=%llu no_visible=%llu finish=%llu anomalies=%llu unmatched_post=%llu discarded=%llu "
      "frame_overflow=%llu ledger(ins/wrap/ptr_mismatch)=%llu/%llu/%llu "
      "log(seen/hit/other/summary written)=%llu/%llu/%llu/%llu dropped=%llu/%llu/%llu/%llu log_errors=%llu",
      tag, static_cast<unsigned long long>(k.calls), static_cast<unsigned long long>(k.calls_adm[0]),
      static_cast<unsigned long long>(k.calls_adm[1]), static_cast<unsigned long long>(k.calls_adm[2]),
      static_cast<unsigned long long>(k.calls_adm[3]), static_cast<unsigned long long>(k.pre_class[0]),
      static_cast<unsigned long long>(k.pre_class[1]), static_cast<unsigned long long>(k.pre_class[2]),
      static_cast<unsigned long long>(k.pre_class[3]), static_cast<unsigned long long>(k.pre_class[4]),
      static_cast<unsigned long long>(k.hit_events), static_cast<unsigned long long>(k.processed),
      static_cast<unsigned long long>(k.accepted_likely), static_cast<unsigned long long>(k.early_return),
      static_cast<unsigned long long>(k.no_visible), static_cast<unsigned long long>(k.finish),
      static_cast<unsigned long long>(k.anomalies), static_cast<unsigned long long>(k.unmatched_post),
      static_cast<unsigned long long>(k.discarded_frames), static_cast<unsigned long long>(k.frame_overflow),
      static_cast<unsigned long long>(k.ledger_inserts), static_cast<unsigned long long>(k.ledger_wraps),
      static_cast<unsigned long long>(k.id_ptr_mismatch),
      static_cast<unsigned long long>(k.log_written[0]), static_cast<unsigned long long>(k.log_written[1]),
      static_cast<unsigned long long>(k.log_written[2]), static_cast<unsigned long long>(k.log_written[3]),
      static_cast<unsigned long long>(k.log_dropped[0]), static_cast<unsigned long long>(k.log_dropped[1]),
      static_cast<unsigned long long>(k.log_dropped[2]), static_cast<unsigned long long>(k.log_dropped[3]),
      static_cast<unsigned long long>(k.log_errors));
}

}  // namespace post01
