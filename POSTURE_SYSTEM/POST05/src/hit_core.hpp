#pragma once
// Enemy Posture System, POST03: nucleo PURO de admision, veredicto de impacto, clasificacion de familia y colliders.
// Sin cabeceras del juego ni del SDK. No escribe nada del juego. Admision y PreClass proceden de POST02 sin cambios de semantica.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace post03 {

// ---- admision --------------------------------------------------------------------------------------------
// POST05-COMPAT (0.5.1): E_OC2_CANDIDATE = recurso E_OC2 con firma coherente; SOLO observacion y postura (la supresion lo difiere).
enum class Admission : std::uint8_t { E_OC_CANDIDATE = 0, E_OC2_EXCLUDED = 1, PROFILE_OTHER = 2, UNKNOWN_UNSAFE = 3, E_OC2_CANDIDATE = 4 };
inline constexpr unsigned kAdmissionCount = 5;
inline const char* name(Admission a) noexcept {
  switch (a) {
    case Admission::E_OC_CANDIDATE: return "E_OC_CANDIDATE";
    case Admission::E_OC2_EXCLUDED: return "E_OC2_EXCLUDED";
    case Admission::PROFILE_OTHER: return "PROFILE_OTHER";
    case Admission::E_OC2_CANDIDATE: return "E_OC2_CANDIDATE";
    default: return "UNKNOWN_UNSAFE";
  }
}
// Candidato = se observa en detalle (POST03), recibe postura (POST04) y pasa a la decision de POST05. NO implica supresion.
inline constexpr bool is_candidate(Admission a) noexcept { return a == Admission::E_OC_CANDIDATE || a == Admission::E_OC2_CANDIDATE; }
// V1Strict = admision de POST02..POST05 0.5.0 (E_OC con AtAtp 1/1; E_OC2 excluido). La usan los laboratorios heredados.
// V2Compat = POST05-COMPAT 0.5.1 (la usa el mod): E_OC 1/1 o 2/2 (grupo Blue) y E_OC2 {1,2,3,6} en lockstep, siempre con el bit 0x10000 coherente.
enum class AdmissionPolicy : std::uint8_t { V1Strict = 0, V2Compat = 1 };
inline const char* name(AdmissionPolicy p) noexcept { return p == AdmissionPolicy::V2Compat ? "V2_COMPAT" : "V1_STRICT"; }
inline constexpr bool e_oc2_atp_known(int atp) noexcept { return atp == 1 || atp == 2 || atp == 3 || atp == 6; }

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
inline AdmissionResult admit(const ActorFacts& f, AdmissionPolicy pol = AdmissionPolicy::V1Strict) noexcept {
  if (!f.profile_ok) return {Admission::PROFILE_OTHER, "PROFILE_NOT_E_OC"};
  if (!f.live) return {Admission::UNKNOWN_UNSAFE, "ACTOR_NOT_LIVE"};
  const bool v2 = pol == AdmissionPolicy::V2Compat;
  switch (f.name) {
    case NameKind::Unread: return {Admission::UNKNOWN_UNSAFE, "NAME_UNAVAILABLE"};
    case NameKind::E_OC2:
      if (!v2) return {Admission::E_OC2_EXCLUDED, "RESOURCE_E_OC2"};
      // create() no pone 0x10000 en E_OC2 (d_a_e_oc.cpp:2792-2801); un 0x10000 aqui es un escritor desconocido
      if (f.status_10000) return {Admission::UNKNOWN_UNSAFE, "E_OC2_STATUS_10000_PRESENT"};
      if (f.atp == -2) return {Admission::UNKNOWN_UNSAFE, "E_OC2_ATP_SPHERES_DISAGREE"};
      if (!e_oc2_atp_known(f.atp)) return {Admission::UNKNOWN_UNSAFE, "E_OC2_ATP_OUTSIDE_SET"};
      // Red de Expansion, E_OC2 nativo, Black (3/6) y Blue DEVF sobre E_OC2 caben aqui: el origen NO se deduce.
      return {Admission::E_OC2_CANDIDATE, "E_OC2_SIGNATURE_MATCH"};
    case NameKind::Other: return {Admission::UNKNOWN_UNSAFE, "RESOURCE_UNRECOGNIZED"};
    case NameKind::E_OC: break;
  }
  if (!f.status_10000) return {Admission::UNKNOWN_UNSAFE, "E_OC_STATUS_10000_ABSENT"};
  if (f.atp == -2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_SPHERES_DISAGREE"};
  if (v2) {
    if (f.atp != 1 && f.atp != 2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_OUTSIDE_SET"};
  } else if (f.atp != 1) {
    return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_NOT_1"};
  }
  // Un E_OC nativo, un Blue (solo o en grupo) y un Blue de DEVF producen esta firma: el origen NO se puede deducir.
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

// ---- snapshots -------------------------------------------------------------------------------------------------
// POST05: lecturas auxiliares POR VALOR de los campos que setActionMode (d_a_e_oc.cpp:584-594; binario 0x140e0d260) escribe ademas del modo:
// field_0x6c0/0x6c2 (u32 a cero en +0x8e0), bits Tg/Co de las dos esferas mSphs_cc (orl $1) y el bit HeadLock de fopEn_enemy_c::mFlags (and $0xff7f).
// Solo sirven para demostrar que esas escrituras fueron NO-OPS en una llamada concreta; nunca se escriben.
struct NativeAux {
  std::uint8_t valid = 0;      // 1 si las lecturas se hicieron (actor E_OC candidato y vivo)
  std::int16_t t6c0 = 0, t6c2 = 0;
  std::uint8_t head_lock = 0;  // checkHeadLockFlg() != 0
  std::uint8_t set_bits = 0;   // bit0 tg0, bit1 co0, bit2 tg1, bit3 co1 (ChkTgSet/ChkCoSet de cada esfera)
  // POST05-COMPAT: AtAtp comun de mSphs_at[0/1] (-1 no leido, -2 discrepan) y recurso (NameKind) en ESTE instante. Solo lectura.
  std::int8_t atp = -1;
  std::uint8_t resource = 0;
};
inline constexpr std::uint8_t kAllSetBits = 0x0F;

struct Snapshot {
  std::int32_t health = 0, mode = 0, state = 0, cooldown = 0, old_mode = 0;
  std::uint8_t cutdown = 0, tg_bits = 0;
};
inline constexpr std::int32_t kModeDamage = 5;     // E_OC_ACTION_DAMAGE      (d_a_e_oc.cpp:26 / binario POST00)
inline constexpr std::int32_t kModeBigDamage = 6;  // E_OC_ACTION_BIG_DAMAGE
inline constexpr std::int32_t kModeDeath = 9;      // E_OC_ACTION_DEATH

// ---- familias de ataque ----------------------------------------------------------------------------------------
// Mascaras inyectadas: mod.cpp usa los AT_TYPE_* reales; el lab usa mascaras SINTETICAS.
struct AtMasks {
  std::uint32_t sword, shield, arrow, bomb, iron_ball, boomerang, at40, slingshot;
};
enum class Family : std::uint8_t { Unavailable = 0, Sword, ShieldAttack, Arrow, Bomb, IronBall, Boomerang, At40, Slingshot, Other, Ambiguous };
inline constexpr unsigned kFamilyCount = 11;
inline const char* name(Family f) noexcept {
  switch (f) {
    case Family::Sword: return "SWORD";
    case Family::ShieldAttack: return "SHIELD_ATTACK";
    case Family::Arrow: return "ARROW";
    case Family::Bomb: return "BOMB";
    case Family::IronBall: return "IRON_BALL";
    case Family::Boomerang: return "BOOMERANG";
    case Family::At40: return "AT_TYPE_40";
    case Family::Slingshot: return "SLINGSHOT";
    case Family::Other: return "OTHER_VERIFIED";
    case Family::Ambiguous: return "AMBIGUOUS";
    default: return "UNAVAILABLE";
  }
}
struct FamilyResult {
  Family family = Family::Unavailable;  // AMBIGUOUS si varios grupos de bits coinciden
  Family native_first = Family::Unavailable;  // el que elegiria la cadena if/else de damage_check
  unsigned groups = 0;
  const char* reason = "TYPE_ZERO";
};
// Misma prioridad que la cadena de damage_check (d_a_e_oc.cpp:684-726): la rama que el juego tomo es la PRIMERA coincidencia.
inline FamilyResult classify_family(std::uint32_t type, const AtMasks& m) noexcept {
  FamilyResult r;
  if (type == 0) return r;
  const std::uint32_t masks[8] = {m.sword, m.shield, m.arrow, m.bomb, m.iron_ball, m.boomerang, m.at40, m.slingshot};
  const Family fams[8] = {Family::Sword, Family::ShieldAttack, Family::Arrow, Family::Bomb, Family::IronBall, Family::Boomerang, Family::At40, Family::Slingshot};
  for (unsigned i = 0; i < 8; ++i) {
    if (type & masks[i]) {
      if (r.groups == 0) r.native_first = fams[i];
      ++r.groups;
    }
  }
  if (r.groups == 0) {
    r.family = Family::Other;
    r.reason = "BITS_OUTSIDE_NATIVE_CHAIN";
  } else if (r.groups == 1) {
    r.family = r.native_first;
    r.reason = "NATIVE_BRANCH_ORDER";
  } else {
    r.family = Family::Ambiguous;
    r.reason = "MULTIPLE_FAMILY_BITS";
  }
  return r;
}

// ---- collider del impacto (valores opacos; solo mod.cpp puede desreferenciar, y solo si ColliderRes.deref_ok) ----------------------
// POST03.1: la guarda de alineacion de POST03 exigia 8 bytes. El anfitrion NO lo garantiza: los actores y colliders embebidos viven en
// JKRExpHeap, que con alineacion <= 4 usa allocFromHead/allocFromTail(size) alineando solo a 4 y redondea tamanos con ALIGN_NEXT(size, 4)
// (JKRExpHeap.cpp:172-198, 271-285, 405-414; cMl::memalignB(-4, size) en f_pc_base.cpp:152). La UNICA lectura (GetAtType, un int de 4 bytes en +0x20)
// solo requiere alineacion de 4. Los residuos se registran como CLASES, nunca como direcciones.
enum class PtrClass : std::uint8_t { Null = 0, TooLow, Mod8Zero, Mod8Four, Misaligned };
inline const char* name(PtrClass c) noexcept {
  switch (c) {
    case PtrClass::TooLow: return "too_low";
    case PtrClass::Mod8Zero: return "mod8_0";
    case PtrClass::Mod8Four: return "mod8_4";
    case PtrClass::Misaligned: return "mod4_bad";
    default: return "null";
  }
}
inline constexpr unsigned kPtrClassCount = 5;
inline constexpr std::uintptr_t kMinPlausible = 0x10000u;
inline PtrClass ptr_class(std::uintptr_t p) noexcept {
  if (!p) return PtrClass::Null;
  if (p < kMinPlausible) return PtrClass::TooLow;
  if (p & 3u) return PtrClass::Misaligned;
  return (p & 7u) ? PtrClass::Mod8Four : PtrClass::Mod8Zero;
}
// Alineacion MINIMA para la unica lectura (int de 4 bytes). NO es un valor editable: lo fija el tamano del campo leido.
inline constexpr unsigned kReadAlignBytes = 4;
inline bool deref_alignment_ok(std::uintptr_t p) noexcept { return p >= kMinPlausible && (p & (kReadAlignBytes - 1u)) == 0; }

enum class ColReason : std::uint8_t {
  NoSelected = 0,        // mAtInfo.mpCollider == 0
  SelectedTooLow,        // < 0x10000: sin acceder
  SelectedAlignment,     // coincide con un hit obj crudo pero no es multiplo de 4: sin acceder
  RawNull,               // bit TG activo con hit obj crudo nulo y sin coincidencia
  NotEqualToRaw,         // ambos hit obj crudos capturados pero ninguno coincide
  FreshMatchCc0,
  FreshMatchCc1
};
inline constexpr unsigned kColReasonCount = 7;
inline const char* name(ColReason r) noexcept {
  switch (r) {
    case ColReason::SelectedTooLow: return "SELECTED_TOO_LOW";
    case ColReason::SelectedAlignment: return "SELECTED_ALIGNMENT_UNSUPPORTED";
    case ColReason::RawNull: return "RAW_TG_HIT_OBJ_NULL";
    case ColReason::NotEqualToRaw: return "SELECTED_NOT_EQUAL_TO_RAW_CC0_CC1";
    case ColReason::FreshMatchCc0: return "SELECTED_FRESH_MATCH_CC0";
    case ColReason::FreshMatchCc1: return "SELECTED_FRESH_MATCH_CC1";
    default: return "NO_SELECTED_COLLIDER";
  }
}
struct ColliderRes {
  int src = -1;                 // 0/1 = esfera cuyo hit obj crudo coincide con mAtInfo.mpCollider; -1 sin coincidencia
  bool pair_distinct = false;   // las dos esferas apuntaban a colliders distintos
  bool deref_ok = false;        // coincidencia + alineacion de lectura: unica condicion que autoriza la desreferencia
  ColReason reason = ColReason::NoSelected;
  PtrClass sel_class = PtrClass::Null, raw0_class = PtrClass::Null, raw1_class = PtrClass::Null;
};
// sel = mAtInfo.mpCollider tras el POST; raw0/raw1 = hit obj CRUDO de cada esfera capturado en el PRE (misma llamada).
// La COMPARACION numerica de valores opacos es inocua; la AUTORIZACION de leer exige ademas frescura (la aplica judge) y alineacion de 4.
inline ColliderRes resolve_collider(std::uintptr_t sel, std::uintptr_t raw0, std::uintptr_t raw1, std::uint8_t tg_bits) noexcept {
  ColliderRes r;
  const bool b0 = (tg_bits & 1u) != 0, b1 = (tg_bits & 2u) != 0;
  r.pair_distinct = b0 && b1 && raw0 != raw1;
  r.sel_class = ptr_class(sel);
  r.raw0_class = b0 ? ptr_class(raw0) : PtrClass::Null;
  r.raw1_class = b1 ? ptr_class(raw1) : PtrClass::Null;
  if (!sel) return r;
  if (sel < kMinPlausible) {
    r.reason = ColReason::SelectedTooLow;
    return r;
  }
  // La fuente deja en mpCollider el de cc[1] si cc[1] tuvo hit, si no el de cc[0] (d_a_e_oc.cpp:656-666).
  if (b1 && raw1 == sel) {
    r.src = 1;
    r.reason = ColReason::FreshMatchCc1;
  } else if (b0 && raw0 == sel) {
    r.src = 0;
    r.reason = ColReason::FreshMatchCc0;
  } else {
    r.reason = ((b0 && !raw0) || (b1 && !raw1)) ? ColReason::RawNull : ColReason::NotEqualToRaw;
    return r;
  }
  if (!deref_alignment_ok(sel)) {
    r.reason = ColReason::SelectedAlignment;
    return r;
  }
  r.deref_ok = true;
  return r;
}

// ---- veredicto -----------------------------------------------------------------------------------------------------
enum class HitKind : std::uint8_t {
  None = 0,
  FinishNative,           // ruta CutDownHit nativa observada (estado de muerte esperado)
  FinishUnexpected,       // CutDownHit en PRE pero el estado final no es el esperado
  Confirmed,              // procesado + firma de «reaccion comprometida» (ver HIT_PATH_AUDIT.md)
  ProcessedMismatch,      // cooldown puesto pero la firma de reaccion no coincide: sin confirmar
  CandidateNotProcessed   // bit TG crudo sin que damage_check llegara a poner el cooldown
};
inline const char* name(HitKind k) noexcept {
  switch (k) {
    case HitKind::FinishNative: return "FINISH_NATIVE_OBSERVED";
    case HitKind::FinishUnexpected: return "FINISH_UNEXPECTED_STATE";
    case HitKind::Confirmed: return "HIT_CONFIRMED";
    case HitKind::ProcessedMismatch: return "HIT_PROCESSED_SIGNATURE_MISMATCH";
    case HitKind::CandidateNotProcessed: return "HIT_CANDIDATE_NOT_PROCESSED";
    default: return "NONE";
  }
}
enum class Eligibility : std::uint8_t { No = 0, Undetermined, Confirmed };
inline const char* name(Eligibility e) noexcept {
  switch (e) {
    case Eligibility::Confirmed: return "CONFIRMED";
    case Eligibility::Undetermined: return "UNDETERMINED";
    default: return "NO";
  }
}

struct Judgement {
  HitKind kind = HitKind::None;
  const char* reason = "";
  bool processed = false;
  bool dual_tg = false;
  bool lethal = false;
  bool hp_changed = false;
  bool read_collider = false;  // mod.cpp puede leer AtType del collider seleccionado
  Eligibility elig = Eligibility::No;
};

// Firma de «reaccion comprometida» para E_OC (mName == "E_OC"): TODA ruta que supera la linea 673 de damage_check termina en
// setActionMode(DAMAGE|BIG_DAMAGE) (lineas 737-742, 773-784); setActionMode guarda el modo previo en mOldActionMode (584-587).
inline bool reaction_signature(const Snapshot& pre, const Snapshot& post) noexcept {
  return (post.mode == kModeDamage || post.mode == kModeBigDamage) && post.old_mode == pre.mode;
}

inline Judgement judge(PreClass cls, const Snapshot& pre, const Snapshot& post, const ColliderRes& col) noexcept {
  Judgement j;
  j.hp_changed = pre.health != post.health;
  j.lethal = post.health <= 0;
  j.dual_tg = (pre.tg_bits & 3u) == 3u;
  switch (cls) {
    case PreClass::CUT_DOWN_FINISH_PATH:
      if (post.health == 0 && post.mode == kModeDeath) {
        j.kind = HitKind::FinishNative;
        j.reason = "HEALTH0_MODE_DEATH";
      } else {
        j.kind = HitKind::FinishUnexpected;
        j.reason = "EXPECTED_DEATH_STATE_NOT_SEEN";
      }
      break;
    case PreClass::TG_CANDIDATE:
      j.processed = pre.cooldown == 0 && post.cooldown != 0;  // solo damage_check escribe field_0x6cc distinto de 0 (674/676/680/774)
      if (!j.processed) {
        j.kind = HitKind::CandidateNotProcessed;
        j.reason = "COOLDOWN_NOT_SET_AFTER_TG";
      } else if (reaction_signature(pre, post)) {
        j.kind = HitKind::Confirmed;
        j.reason = "COOLDOWN_SET_AND_REACTION_COMMITTED";
        j.elig = Eligibility::Confirmed;
        j.read_collider = col.deref_ok;  // solo si mpCollider coincide con el hit obj crudo de una esfera y esta alineado para el int de 4 bytes
      } else {
        j.kind = HitKind::ProcessedMismatch;
        j.reason = "COOLDOWN_SET_REACTION_SIGNATURE_MISMATCH";
        j.elig = Eligibility::Undetermined;
      }
      break;
    default: break;  // COOLDOWN_NO_PROCESS, NO_TG_CANDIDATE, UNKNOWN: sin evento
  }
  return j;
}

}  // namespace post03
