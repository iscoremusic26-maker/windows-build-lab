#pragma once
// Enemy Posture System, POST02: nucleo PURO de admision y evaluacion de impactos (copiado de POST01 sin cambios de semantica).
// Sin cabeceras del juego ni del SDK. No escribe nada del juego.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace post02 {

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

}  // namespace post02
