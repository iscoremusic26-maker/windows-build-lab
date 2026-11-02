#pragma once
// Enemy Posture System, POST05: decision PURA de supresion selectiva de UNA reaccion ordinaria (DAMAGE) y plan de restauracion exacto.
//
// Que se suprime (y nada mas): la ULTIMA sentencia de daE_OC_c::damage_check en la ruta ordinaria, `setActionMode(E_OC_ACTION_DAMAGE, my_val)`
// (d_a_e_oc.cpp:783; binario: call en 0x140e067b9 seguido solo del epilogo). setActionMode (d_a_e_oc.cpp:584-594; binario 0x140e0d260) escribe:
//   mOldActionMode <- mActionMode; mActionMode <- DAMAGE; mOcState <- my_val;          (los tres que POST05 RESTAURA)
//   field_0x6c0 = field_0x6c2 = 0; OnCo/OnTgSetBit de mSphs_cc[0..1]; offHeadLockFlg(); (que POST05 EXIGE que hayan sido no-ops)
// Si esas escrituras extra fueron no-ops (comprobado con lecturas PRE y POST de esta misma llamada), restaurar los tres campos a su valor PRE deja
// el actor EXACTAMENTE como si esa ultima llamada no hubiera ocurrido. Todo lo demas del golpe se conserva: HP y SE/hit-stop/marca (cc_at_check),
// cooldown field_0x6cc, alternancia de corte (field_0x6d6/6dc), mStts.Move(). executeDamage (que pone la animacion y la voz de dano) corre DESPUES en
// el mismo action() (d_a_e_oc.cpp:2418 damage_check -> :2475 switch), asi que no ha empezado.
// Este archivo no toca el juego: devuelve un plan y aplica escrituras solo a traves del escritor que le pasa mod.cpp.
//
// POST05-RB (0.5.2): ruta E_OC2 (Red nativo, Red de Expansion, Black 3/3 y 6/6) con el MISMO escritor y los MISMOS gates de seguridad, mas un gate de
// FASE NATIVA leido en el PRE de esta llamada (mpMorf->mFrameCtrl y speedF, ABI fijada contra el EXE): B/C maduro (frame >= 2, clip B/C autenticado por
// su fin nativo 42/47, modo once, rate nativa o de Expansion 1/1.15/2, sin parada) o frenada de ATTACK 0 que no puede arrancar el clip en este tick.
// Excluidos por diseno: el tick que arranca B/C (A5-1), el tick frame 1->2 (A5-2), el fin de clip (cadena Black), ATTACK 3 y todo lo no autenticado.
// Ver POST05/RED_BLACK/IMPLEMENTATION/. E2Policy::Deferred (por defecto) reproduce exactamente 0.5.1; mod.cpp fija MatureBC una sola vez.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>

#include "posture_core.hpp"

namespace post05 {

using post03::Admission;
using post03::Family;
using post03::HitKind;
using post03::HitReport;
using post03::Life;

inline constexpr const char* kReactionVersion = "0.6.0";

// ---- politica TEST (constantes; todo cambio de alcance pasa por aqui) ------------------------------------------------------------------
namespace policy {
inline constexpr bool kSuppressionEnabled = true;
inline constexpr int kModeAttack = 4;     // E_OC_ACTION_ATTACK     (d_a_e_oc.cpp:25)
inline constexpr int kModeDamage = 5;     // E_OC_ACTION_DAMAGE     (d_a_e_oc.cpp:26; binario mov %ebp(=5),%edx en 0x140e067b4)
inline constexpr int kModeBigDamage = 6;  // E_OC_ACTION_BIG_DAMAGE (d_a_e_oc.cpp:27)
inline constexpr int kCooldownOrdinary = 10;  // field_0x6cc = 10 para tipos sin AT_TYPE_UNK (d_a_e_oc.cpp:670-679); 3 = salto con giro cancelado; 0x14 = AT_TYPE_UNK
// Estados PRE de ATTACK admitidos (executeAttack, d_a_e_oc.cpp:1379-1532): 0 = frenada y eleccion de animacion de ataque; 1/2 = golpe B/C en curso.
// EXCLUIDO 3 = rebote por guardia (animacion en reversa). Cualquier otro modo PRE (WAIT, WALK, FIND, WATCH, DAMAGE...) queda vanilla.
inline constexpr bool attack_state_allowed(int s) noexcept { return s == 0 || s == 1 || s == 2; }
// my_val de una espada ordinaria (getCutType <= 2, d_a_e_oc.cpp:596-628/:686-700): DAMAGE con estado 0, 1 o 2. 3 = escudo, 4 = bumeran.
inline constexpr bool ordinary_sword_damage_state(int s) noexcept { return s == 0 || s == 1 || s == 2; }
inline constexpr const char* kPolicyId = "POST05_TEST_POLICY_V4_GENERAL_ANTI_STAGGER";
// ---- POST05-GENERAL (0.5.4): regla general POSTURA < 100 => SIN TAMBALEO, en cualquier modo normal ----
// Modos nativos (d_a_e_oc.cpp:20-37): 0 WAIT 1 WALK 2 TALK 3 FIND 4 ATTACK 5 DAMAGE 6 BIG_DAMAGE 7 WATCH 8 SOUND_WATCH 9 DEATH 10 WATER_DEATH
// 11 DEMO_MASTER 12 DEMO_CHILD 13 FALL_DEAD 14 FALL 15 FIND_STAY 16 MOVE_OUT.
inline constexpr int kModeFind = 3, kModeWatch = 7, kModeSoundWatch = 8, kModeWait = 0, kModeWalk = 1;
// PRE excluidos (guion, conversacion, muerte, agua, demo, caida fisica): la reaccion nativa no se toca.
inline constexpr bool general_pre_mode_allowed(int m) noexcept { return m == 0 || m == 1 || (m >= 3 && m <= 8) || m == 15 || m == 16; }
// PRE "desprevenido": en vez de volver al modo PRE, el golpe hace que DESCUBRA a Link igual que el nativo hace con el tirachinas sobre WAIT/WALK/TALK
// (damage_check, rama SLINGSHOT: setActionMode(FIND, 1)): FIND estado 1 = voz V_FIND + animacion de descubrimiento, sin animacion de dano.
inline constexpr bool general_pre_mode_unaware(int m) noexcept { return m == kModeWait || m == kModeWalk || m == kModeWatch || m == kModeSoundWatch; }
// Forma POST de la reaccion que se deshace (unica llamada final de damage_check):
//   DAMAGE (5), estado my_val 0..4 (espada 0/1/2, flecha 0/1, escudo 3, bumeran 4): setActionMode(DAMAGE, s) -> aux POST timers 0, HeadLock 0, bits 0x0F.
//   BIG_DAMAGE (6), estado my_val-5 0..2: setActionMode(BIG_DAMAGE, s) + offTgSph() -> aux POST timers 0, HeadLock 0, bits Co sin Tg (0x0A).
inline constexpr std::uint8_t kBitsCoOnly = 0x0A;
// ---- POST05-RB: ruta E_OC2 ----
// Perfiles E_OC2 por potencia comun PRE==POST (no es identidad: Red nativo y Red de Expansion comparten 2/2; ambos usan la misma interseccion).
inline constexpr int kE2AtpRed = 2, kE2AtpBlack = 3, kE2AtpBlackFury = 6;
// Black 3/3: Expansion escribe 6/6 en SU POST de execute (despues de este damage_check y de cc_set, antes de dCcS::Move). La potencia la consume
// dCcS::SetAtTgGObjInf (GetAtAtp -> PlusDmg) en Move, asi que el golpe que continua contacta con 6 de forma coherente, un solo contacto.
inline constexpr bool kE2BlackFuryStartEnabled = true;
inline constexpr std::int16_t kClipEndB = 42, kClipEndC = 47;  // BCK5 (estado 1) / BCK6 (estado 2), modo once: clip_end real del log de Expansion
inline constexpr float kE2MatureFrame = 1.999f;  // PRE frame >= 2: ya pasaron el tick de arranque (0->1) y el de frame 1->2
inline constexpr float kE2BrakeSpeed = 1.5f;     // ATTACK 0: |speedF| > 1.5 => cLib_chaseF(&speedF,0,1) no llega a 0 en este tick => no hay setBck
inline constexpr bool e2_rate_known(float r) noexcept { return r == 1.0f || r == 1.15f || r == 2.0f; }  // nativa, Expansion x1.15, Expansion x2
inline constexpr bool e2_profile_allowed(int atp) noexcept { return atp == kE2AtpRed || atp == kE2AtpBlackFury || (atp == kE2AtpBlack && kE2BlackFuryStartEnabled); }
}  // namespace policy

enum class E2Policy : std::uint8_t { Deferred = 0, MatureBC };
inline const char* name(E2Policy p) noexcept { return p == E2Policy::MatureBC ? "MATURE_BC_OR_BRAKE" : "DEFERRED"; }
// Fase nativa PRE de un golpe E_OC2 (solo para E_OC2; E_OC no la usa).
enum class E2Phase : std::uint8_t { NotE2 = 0, AnimUnverified, Startup, FrameEarly, ClipEnding, BcMature, Brake, Count };
inline constexpr unsigned kE2PhaseCount = static_cast<unsigned>(E2Phase::Count);
inline const char* name(E2Phase p) noexcept {
  static constexpr const char* k[kE2PhaseCount] = {"NA", "ANIM_UNVERIFIED", "STARTUP_TICK", "FRAME_EARLY", "CLIP_ENDING", "BC_MATURE", "ATTACK0_BRAKE"};
  const unsigned i = static_cast<unsigned>(p);
  return i < kE2PhaseCount ? k[i] : "NONE";
}
inline bool e2_phase_eligible(E2Phase p) noexcept { return p == E2Phase::BcMature || p == E2Phase::Brake; }
// POST05-FINALIZE (0.5.3): entorno de la IA de variantes. Absent = el servicio que Bokoblin Expansion 0.17.0 importa como OBLIGATORIO
// (encuentros V1.1 de EVF) NO esta publicado: por contrato del loader (mods/api.h: un import requerido sin resolver impide cargar el mod)
// Expansion no puede estar activo y nadie decide x1.15/x2 alrededor de damage_check. Possible = publicado alguna vez en la sesion (pegajoso)
// o sin comprobar. Solo la AUSENCIA se usa como prueba; la presencia nunca identifica actores.
enum class E2Env : std::uint8_t { Unknown = 0, Absent, Possible };
inline const char* name(E2Env e) noexcept { return e == E2Env::Absent ? "NATIVE_ONLY" : e == E2Env::Possible ? "EXPANSION_POSSIBLE" : "UNKNOWN"; }
// Por que una fase temprana/final de E_OC2 se permite o no (registro)
enum class E2Compat : std::uint8_t { NA = 0, NativeOnly, X115Owned, BlackBaseNoPending, MatureOrBrake, X2PendingPossible, RateDecisionThisTick, ClipEndChain, RateLostThisBlow, Count };
inline const char* name(E2Compat c) noexcept {
  switch (c) {
    case E2Compat::NativeOnly: return "NATIVE_ONLY_NO_RATE_OWNER";
    case E2Compat::X115Owned: return "X1_15_ALREADY_OWNED";
    case E2Compat::BlackBaseNoPending: return "BLACK_BASE_NO_X2_PENDING";
    case E2Compat::MatureOrBrake: return "MATURE_OR_BRAKE";
    case E2Compat::X2PendingPossible: return "X2_PENDING_POSSIBLE";
    case E2Compat::RateDecisionThisTick: return "RATE_DECISION_THIS_TICK";
    case E2Compat::ClipEndChain: return "CLIP_END_EXPANSION_CHAIN";
    case E2Compat::RateLostThisBlow: return "EXPANSION_RATE_NATIVE_THIS_BLOW";
    default: return "NA";
  }
}
// PURA. Decide si la fase PRE de un golpe E_OC2 permite suprimir, dado el entorno. `atp` = potencia comun PRE (2 Red, 3 Black base, 6 Black furia).
//  - BcMature/Brake: siempre (0.5.2; demostrado con CP21 real).
//  - Entorno NATIVE_ONLY y atp 2: arranque, frame temprano y fin de clip como Blue (nadie posee la tasa). Black (3/6) implica Expansion.
//  - Con Expansion posible: frame 1 con rate 1.15 (la tasa ya esta decidida: un x2 pendiente deja rate 1) o Black 3/3 (furia 0 al decidir => nunca x2
//    pendiente). Arranque (decision de tasa en ESTE tick), frame 1 a rate 1 de Red/Black furia (x2 pendiente posible) y fin de clip: vanilla.
inline bool e2_allowed(E2Phase ph, E2Env env, int atp, float rate, E2Compat& why) noexcept {
  namespace p = policy;
  why = E2Compat::NA;
  if (e2_phase_eligible(ph)) { why = E2Compat::MatureOrBrake; return true; }
  if (ph != E2Phase::Startup && ph != E2Phase::FrameEarly && ph != E2Phase::ClipEnding) return false;
  if (env == E2Env::Absent && atp == p::kE2AtpRed) { why = E2Compat::NativeOnly; return true; }
  if (ph == E2Phase::FrameEarly) {
    if (rate == 1.15f) { why = E2Compat::X115Owned; return true; }
    if (atp == p::kE2AtpBlack) { why = E2Compat::BlackBaseNoPending; return true; }
    why = E2Compat::X2PendingPossible;
    return false;
  }
  why = ph == E2Phase::Startup ? E2Compat::RateDecisionThisTick : E2Compat::ClipEndChain;
  return false;
}
// PURA. `a` = lecturas PRE de esta llamada; `pre_state` = mOcState PRE (el modo PRE ya es ATTACK y el estado 0/1/2 cuando se llama).
inline E2Phase e2_phase(const post03::NativeAux& a, int pre_state) noexcept {
  namespace p = policy;
  if (!a.valid) return E2Phase::AnimUnverified;
  if (pre_state == 0) {
    if (!(a.speed == a.speed)) return E2Phase::AnimUnverified;  // NaN
    return (a.speed > p::kE2BrakeSpeed || a.speed < -p::kE2BrakeSpeed) ? E2Phase::Brake : E2Phase::Startup;
  }
  if (pre_state != 1 && pre_state != 2) return E2Phase::AnimUnverified;
  const std::int16_t want_end = pre_state == 1 ? p::kClipEndB : p::kClipEndC;
  if (!a.anim_ok || a.anim_attr != 0 || a.anim_end != want_end || !(a.anim_frame == a.anim_frame) || a.anim_frame < 0.0f ||
      a.anim_frame >= static_cast<float>(a.anim_end))
    return E2Phase::AnimUnverified;
  if ((a.anim_state & 1u) != 0 || a.anim_rate == 0.0f) return E2Phase::ClipEnding;  // isStop(): el fin de clip (y la cadena Black) queda nativo
  if (!p::e2_rate_known(a.anim_rate)) return E2Phase::AnimUnverified;              // escritor de rate desconocido
  if (a.anim_frame < p::kE2MatureFrame) return E2Phase::FrameEarly;                // tick frame 1 (x2 pendiente de Expansion)
  return E2Phase::BcMature;
}
inline bool same_anim(const post03::NativeAux& x, const post03::NativeAux& y) noexcept {
  return x.anim_ok == y.anim_ok && x.anim_attr == y.anim_attr && x.anim_state == y.anim_state && x.anim_end == y.anim_end && x.anim_rate == y.anim_rate &&
         x.anim_frame == y.anim_frame && x.speed == y.speed;
}

enum class Decision : std::uint8_t { NotApplicable = 0, AllowVanilla, SkipUnsafe, SuppressEligible, Count };
inline constexpr unsigned kDecisionCount = static_cast<unsigned>(Decision::Count);
inline const char* name(Decision d) noexcept {
  switch (d) {
    case Decision::NotApplicable: return "NOT_APPLICABLE";
    case Decision::AllowVanilla: return "ALLOW_VANILLA";
    case Decision::SkipUnsafe: return "SKIP_UNSAFE";
    case Decision::SuppressEligible: return "SUPPRESS_ELIGIBLE";
    default: return "NONE";
  }
}
// Motivos (cada uno pertenece a UNA sola decision; el orden de evaluacion es el del enum)
enum class Reason : std::uint8_t {
  NotConfirmed = 0,                                    // NOT_APPLICABLE
  SuppressionDisabled, E_OC2SuppressionDeferred, FamilyNotSword, Lethal, BigDamage, PostModeNotDamage, CooldownNotOrdinary, NotWeighted, CrossesThreshold, LatchedAfter,
  PreModeNotAttack, PreStateNotWhitelisted, PostStateNotOrdinary,
  E_OC2ProfileDeferred, E_OC2AnimUnverified, E_OC2StartupTick, E_OC2FrameEarly, E_OC2ClipEnding,  // ALLOW_VANILLA (politica; POST05-RB: fase E_OC2)
  FamilyUnverified, PreModeScripted, PostShapeUnknown, PostureSkipUnsafe,                           // ALLOW_VANILLA (politica GENERAL 0.5.4)
  SessionDisabled, TokenMismatch, NotCandidateActive, AdmissionEverUnstable, FrozenOrDead, PostSignatureMismatch, PreNotFresh, ColliderNotFresh,
  AuxMissing, PreTimersNonZero, PreHeadLock, PreSetBitsOff, PostAuxUnexpected, PowerChangedInCall, UnsafeContinuation,  // SKIP_UNSAFE (seguridad)
  Eligible,                                                                                        // SUPPRESS_ELIGIBLE
  Count
};
inline constexpr unsigned kReasonCount = static_cast<unsigned>(Reason::Count);
inline const char* name(Reason r) noexcept {
  static constexpr const char* k[kReasonCount] = {
      "NOT_CONFIRMED", "SUPPRESSION_DISABLED", "E_OC2_SUPPRESSION_DEFERRED", "FAMILY_NOT_SWORD", "LETHAL", "BIG_DAMAGE", "POST_MODE_NOT_DAMAGE", "COOLDOWN_NOT_ORDINARY", "NOT_WEIGHTED",
      "CROSSES_THRESHOLD", "LATCHED_AFTER", "PRE_MODE_NOT_ATTACK", "PRE_STATE_NOT_WHITELISTED", "POST_STATE_NOT_ORDINARY", "E_OC2_PROFILE_DEFERRED",
      "E_OC2_ANIM_UNVERIFIED", "E_OC2_STARTUP_TICK", "E_OC2_FRAME_EARLY", "E_OC2_CLIP_ENDING", "FAMILY_UNVERIFIED", "PRE_MODE_SCRIPTED", "POST_SHAPE_UNKNOWN",
      "POSTURE_SKIP_UNSAFE", "SESSION_DISABLED", "TOKEN_MISMATCH",
      "NOT_CANDIDATE_ACTIVE", "ADMISSION_EVER_UNSTABLE", "FROZEN_OR_DEAD", "POST_SIGNATURE_MISMATCH", "PRE_NOT_FRESH", "COLLIDER_NOT_FRESH", "AUX_MISSING",
      "PRE_TIMERS_NONZERO", "PRE_HEAD_LOCK", "PRE_SET_BITS_OFF", "POST_AUX_UNEXPECTED", "POWER_CHANGED_IN_CALL", "UNSAFE_NATIVE_CONTINUATION", "ELIGIBLE"};
  const unsigned i = static_cast<unsigned>(r);
  return i < kReasonCount ? k[i] : "NONE";
}
inline Decision decision_of(Reason r) noexcept {
  if (r == Reason::NotConfirmed) return Decision::NotApplicable;
  if (r <= Reason::PostureSkipUnsafe) return Decision::AllowVanilla;
  if (r <= Reason::UnsafeContinuation) return Decision::SkipUnsafe;
  return Decision::SuppressEligible;
}

// Entrada POR VALOR (nada de punteros a actores): informe de POST03.1, resultado de POST04 y estado del ledger para el mismo token.
struct DecisionInput {
  HitReport rep{};
  post04::ConsumeOutcome outc{};
  bool continuation_safe = true;  // FIX01: adapter proves native clip/control coherence before any restore
  bool ledger_valid = false;  // valid(slot, generation) && id coincide
  Admission adm = Admission::UNKNOWN_UNSAFE;
  Life life = Life::Free;
};
enum class StaggerPolicy : std::uint8_t { AttackOnly = 0, General };
inline const char* name(StaggerPolicy p) noexcept { return p == StaggerPolicy::General ? "GENERAL_POSTURE_BELOW_100" : "ATTACK_ONLY_LEGACY"; }
enum class PlanKind : std::uint8_t { None = 0, RestorePre, NoticeFind1 };
inline const char* name(PlanKind k) noexcept { return k == PlanKind::RestorePre ? "RESTORE_PRE" : k == PlanKind::NoticeFind1 ? "NOTICE_FIND1" : "NA"; }
// Estado vivo de TODO lo que setActionMode (+ offTgSph en BIG_DAMAGE) escribe: modo, estado, modo previo, field_0x6c0/0x6c2, HeadLock y bits Tg/Co.
struct LiveMode {
  int mode = 0, state = 0, old_mode = 0;
  std::int16_t t6c0 = 0, t6c2 = 0;
  std::uint8_t head_lock = 0, set_bits = post03::kAllSetBits;
};
inline bool same_live(const LiveMode& a, const LiveMode& b) noexcept {
  return a.mode == b.mode && a.state == b.state && a.old_mode == b.old_mode && a.t6c0 == b.t6c0 && a.t6c2 == b.t6c2 && a.head_lock == b.head_lock &&
         a.set_bits == b.set_bits;
}
struct RestorePlan {
  int mode = 0, state = 0, old_mode = 0;                 // valores PRE a restaurar
  int expect_mode = 0, expect_state = 0, expect_old = 0;  // valores que setActionMode dejo (se re-comprueban justo antes de escribir)
  LiveMode target{}, expect{};                           // POST05-GENERAL: objetivo y valor POST esperado de TODOS los campos de la reaccion
  PlanKind kind = PlanKind::None;
  bool aux_write = false;                                // hace falta escribir algo ademas de los 3 campos de modo
};
struct DecisionResult {
  bool counted = false;  // false = informe duplicado (no es una decision nueva)
  Admission adm = Admission::UNKNOWN_UNSAFE;  // POST05-COMPAT: admision del ledger usada en la decision (solo para el registro)
  E2Phase phase = E2Phase::NotE2;              // POST05-RB: fase E_OC2 evaluada (NotE2 si no se llego a evaluar)
  E2Compat compat = E2Compat::NA;              // POST05-FINALIZE: motivo de compatibilidad de la fase
  E2Env env = E2Env::Unknown;                  // POST05-FINALIZE: entorno usado en la decision
  std::uint8_t profile = 0;                    // POST05-RB: 0 E_OC, 1 E_OC2 atp2, 2 E_OC2 atp3, 3 E_OC2 atp6 (indice de applied_by_profile)
  Decision decision = Decision::NotApplicable;
  Reason reason = Reason::NotConfirmed;
  RestorePlan plan{};
};
enum class Apply : std::uint8_t { NotAttempted = 0, Applied, RecheckFailed, VerifyFailed };
inline const char* name(Apply a) noexcept {
  switch (a) {
    case Apply::Applied: return "SUPPRESSION_APPLIED";
    case Apply::RecheckFailed: return "RECHECK_FAILED_NO_WRITE";
    case Apply::VerifyFailed: return "VERIFY_FAILED";
    default: return "NONE";
  }
}

struct RCounters {
  std::uint64_t decisions = 0, duplicates = 0, by_decision[kDecisionCount] = {}, by_reason[kReasonCount] = {};
  std::uint64_t confirmed = 0;  // decisiones sobre HIT_CONFIRMED (denominador de eligible)
  std::uint64_t attempted = 0, applied = 0, recheck_failed = 0, verify_failed = 0, not_attempted = 0;
  std::uint64_t applied_by_pre_state[3] = {};
  std::uint64_t applied_by_pre_mode[17] = {};  // POST05-GENERAL: aplicadas por modo PRE (indice 16 = 16 o mayor)
  std::uint64_t applied_by_kind[3] = {};       // POST05-GENERAL: NA / RESTORE_PRE / NOTICE_FIND1
  std::uint64_t applied_big = 0;               // POST05-GENERAL: aplicadas sobre la ruta BIG_DAMAGE
  std::uint64_t by_resource[3] = {};  // POST05-COMPAT: decisiones contadas por recurso del informe (E_OC / E_OC2 / otro o sin leer)
  std::uint64_t e2_phase[kE2PhaseCount] = {};  // POST05-RB: fase de cada decision E_OC2 que llego al gate de fase
  std::uint64_t e2_applied_phase[kE2PhaseCount] = {};  // POST05-FINALIZE: supresiones E_OC2 aplicadas por fase
  std::uint64_t e2_env_decisions[3] = {};              // POST05-FINALIZE: decisiones E_OC2 por entorno (unknown/native_only/possible)
  std::uint64_t applied_by_profile[4] = {};    // POST05-RB: aplicadas por perfil (E_OC / E_OC2 atp2 / atp3->6 / atp6)
  std::uint64_t log_written = 0, log_dropped = 0;
};

class Reaction {
 public:
  RCounters c{};
  StaggerPolicy stagger_policy = StaggerPolicy::AttackOnly;  // POST05-GENERAL: AttackOnly = 0.5.3 (labs heredados); mod.cpp fija General una vez
  E2Env e2_env = E2Env::Possible;  // POST05-FINALIZE: lo actualiza SOLO mod.cpp desde mod_update (consulta de servicio); por defecto conservador
  E2Policy e2_policy = E2Policy::Deferred;  // POST05-RB: Deferred = comportamiento 0.5.1 (laboratorios heredados); mod.cpp fija MatureBC una vez
  bool session_disabled() const noexcept { return session_disabled_; }

  // Decision PURA (salvo contadores). Una sola por informe nuevo.
  DecisionResult decide(const DecisionInput& in) noexcept {
    DecisionResult d;
    const HitReport& r = in.rep;
    if (r.event != 0 && r.event <= last_event_) {
      ++c.duplicates;
      return d;  // counted=false
    }
    last_event_ = r.event;
    d.counted = true;
    d.adm = in.adm;
    ++c.by_resource[resource_index(in)];
    d.env = e2_env;
    d.reason = stagger_policy == StaggerPolicy::General ? classify_general(in, d.phase, d.compat) : classify(in, d.phase, d.compat);
    if (d.phase != E2Phase::NotE2) { ++c.e2_phase[static_cast<unsigned>(d.phase)]; ++c.e2_env_decisions[static_cast<unsigned>(d.env) < 3 ? static_cast<unsigned>(d.env) : 0]; }
    d.profile = profile_index(in);
    d.decision = decision_of(d.reason);
    ++c.decisions;
    ++c.by_decision[static_cast<unsigned>(d.decision)];
    ++c.by_reason[static_cast<unsigned>(d.reason)];
    if (r.j.kind == HitKind::Confirmed) ++c.confirmed;
    if (d.decision == Decision::SuppressEligible) {
      d.plan.expect_mode = r.post.mode;
      d.plan.expect_state = r.post.state;
      d.plan.expect_old = r.post.old_mode;
      d.plan.expect = LiveMode{r.post.mode, r.post.state, r.post.old_mode, r.aux_post.t6c0, r.aux_post.t6c2, r.aux_post.head_lock, r.aux_post.set_bits};
      if (stagger_policy == StaggerPolicy::General && policy::general_pre_mode_unaware(r.pre.mode)) {
        // Igual que setActionMode(FIND, 1) tras la reaccion: modo previo = PRE, timers 0 y HeadLock 0 (ya los dejo setActionMode), bits Tg/Co activos.
        d.plan.kind = PlanKind::NoticeFind1;
        d.plan.target = LiveMode{policy::kModeFind, 1, r.pre.mode, 0, 0, 0, post03::kAllSetBits};
      } else {
        d.plan.kind = PlanKind::RestorePre;
        d.plan.target = LiveMode{r.pre.mode, r.pre.state, r.pre.old_mode, r.aux_pre.t6c0, r.aux_pre.t6c2, r.aux_pre.head_lock, r.aux_pre.set_bits};  // PRE exacto
      }
      d.plan.mode = d.plan.target.mode;
      d.plan.state = d.plan.target.state;
      d.plan.old_mode = d.plan.target.old_mode;
      d.plan.aux_write = d.plan.target.t6c0 != d.plan.expect.t6c0 || d.plan.target.t6c2 != d.plan.expect.t6c2 ||
                         d.plan.target.head_lock != d.plan.expect.head_lock || d.plan.target.set_bits != d.plan.expect.set_bits;
    }
    return d;
  }

  // Aplica el plan: relee TODOS los campos de la reaccion, escribe y verifica. `read` y `write` los da mod.cpp (o el laboratorio).
  // `write` puede ser (int mode, int state, int old) (solo los 3 campos: laboratorios heredados) o (const LiveMode& target) (escritor completo).
  // Un plan que necesita mas que los 3 campos con un escritor de 3 campos no se intenta (no-attempted).
  template <class Read, class Write>
  Apply apply(const DecisionResult& d, Read read, Write write) noexcept {
    if (!d.counted || d.decision != Decision::SuppressEligible) return Apply::NotAttempted;
    constexpr bool kFull = std::is_invocable_v<Write, const LiveMode&>;
    if (session_disabled_ || (!kFull && d.plan.aux_write)) {
      ++c.not_attempted;
      return Apply::NotAttempted;
    }
    ++c.attempted;
    const LiveMode now = read();
    if (!same_live(now, d.plan.expect)) {
      ++c.recheck_failed;  // algo cambio entre la lectura POST y este punto: no se escribe nada
      return Apply::RecheckFailed;
    }
    if constexpr (kFull) write(d.plan.target);
    else write(d.plan.mode, d.plan.state, d.plan.old_mode);
    const LiveMode after = read();
    if (!same_live(after, d.plan.target)) {
      ++c.verify_failed;
      session_disabled_ = true;  // fallo seguro: no se intenta ninguna supresion mas en la sesion
      return Apply::VerifyFailed;
    }
    ++c.applied;
    if (d.plan.kind == PlanKind::RestorePre && d.plan.mode == policy::kModeAttack && d.plan.state >= 0 && d.plan.state < 3) ++c.applied_by_pre_state[d.plan.state];
    const int pre_mode = d.plan.kind == PlanKind::NoticeFind1 ? d.plan.target.old_mode : d.plan.mode;
    ++c.applied_by_pre_mode[pre_mode >= 0 && pre_mode < 16 ? pre_mode : 16];
    ++c.applied_by_kind[static_cast<unsigned>(d.plan.kind) < 3 ? static_cast<unsigned>(d.plan.kind) : 0];
    if (d.plan.expect.mode == policy::kModeBigDamage) ++c.applied_big;
    ++c.applied_by_profile[d.profile < 4 ? d.profile : 0];
    if (d.profile != 0 && static_cast<unsigned>(d.phase) < kE2PhaseCount) ++c.e2_applied_phase[static_cast<unsigned>(d.phase)];
    return Apply::Applied;
  }

  // Ecuaciones de conservacion de POST05 (independientes de POST03/POST04)
  struct Audit {
    bool r1 = false, r2 = false, r3 = false, r4 = false, r5 = false, r6 = false, r7 = false, r8 = false, ok = false;
  };
  Audit audit() const noexcept {
    Audit a;
    std::uint64_t sd = 0, per[kDecisionCount] = {};
    for (unsigned i = 0; i < kDecisionCount; ++i) sd += c.by_decision[i];
    for (unsigned i = 0; i < kReasonCount; ++i) per[static_cast<unsigned>(decision_of(static_cast<Reason>(i)))] += c.by_reason[i];
    a.r1 = c.decisions == sd;
    a.r2 = true;
    for (unsigned i = 0; i < kDecisionCount; ++i) a.r2 = a.r2 && per[i] == c.by_decision[i];
    const std::uint64_t eligible = c.by_decision[static_cast<unsigned>(Decision::SuppressEligible)];
    a.r3 = eligible == c.attempted + c.not_attempted;
    a.r4 = c.attempted == c.applied + c.recheck_failed + c.verify_failed;
    std::uint64_t pm = 0;
    for (unsigned i = 0; i < 17; ++i) pm += c.applied_by_pre_mode[i];
    a.r5 = c.attempted <= eligible && eligible <= c.confirmed && c.confirmed <= c.decisions && pm == c.applied &&
           c.applied_by_kind[1] + c.applied_by_kind[2] == c.applied && c.applied_by_pre_state[0] + c.applied_by_pre_state[1] + c.applied_by_pre_state[2] <= c.applied &&
           (stagger_policy == StaggerPolicy::General || c.applied_by_pre_state[0] + c.applied_by_pre_state[1] + c.applied_by_pre_state[2] == c.applied);
    a.r6 = c.by_resource[0] + c.by_resource[1] + c.by_resource[2] == c.decisions;  // POST05-COMPAT
    a.r7 = c.applied_by_profile[0] + c.applied_by_profile[1] + c.applied_by_profile[2] + c.applied_by_profile[3] == c.applied;  // POST05-RB
    std::uint64_t ep = 0;
    for (unsigned i = 0; i < kE2PhaseCount; ++i) ep += c.e2_applied_phase[i];
    a.r8 = ep == c.applied_by_profile[1] + c.applied_by_profile[2] + c.applied_by_profile[3];  // POST05-FINALIZE
    a.ok = a.r1 && a.r2 && a.r3 && a.r4 && a.r5 && a.r6 && a.r7 && a.r8;
    return a;
  }

  bool log_take() noexcept {  // tope propio; ademas, las lineas cuentan dentro del presupuesto Hit del observador
    if (logged_ >= kLogCap) {
      ++c.log_dropped;
      return false;
    }
    ++logged_;
    ++c.log_written;
    return true;
  }
  static constexpr unsigned kLogCap = 300;
  void close_session() noexcept { session_disabled_ = true; }

 private:
  std::uint64_t last_event_ = 0;
  bool session_disabled_ = false;
  unsigned logged_ = 0;

  static bool any_e_oc2(const DecisionInput& in) noexcept {
    constexpr std::uint8_t k2 = static_cast<std::uint8_t>(post03::NameKind::E_OC2);
    return in.adm == Admission::E_OC2_CANDIDATE || in.adm == Admission::E_OC2_EXCLUDED || in.rep.aux_pre.resource == k2 || in.rep.aux_post.resource == k2;
  }
  static bool e2_consistent(const DecisionInput& in) noexcept {  // POST05-RB: TODAS las senales dicen E_OC2 (ledger + recurso PRE y POST)
    constexpr std::uint8_t k2 = static_cast<std::uint8_t>(post03::NameKind::E_OC2);
    return in.adm == Admission::E_OC2_CANDIDATE && in.rep.aux_pre.valid && in.rep.aux_post.valid && in.rep.aux_pre.resource == k2 && in.rep.aux_post.resource == k2;
  }
  static std::uint8_t profile_index(const DecisionInput& in) noexcept {
    if (!any_e_oc2(in)) return 0;
    const int atp = in.rep.aux_pre.atp;
    return atp == policy::kE2AtpRed ? 1 : atp == policy::kE2AtpBlack ? 2 : atp == policy::kE2AtpBlackFury ? 3 : 0;
  }
  static unsigned resource_index(const DecisionInput& in) noexcept {
    if (any_e_oc2(in)) return 1;
    return in.adm == Admission::E_OC_CANDIDATE ? 0 : 2;
  }
  // Seguridad COMUN a AttackOnly y General (un solo lugar): devuelve Reason::Count si todo es coherente.
  Reason common_safety(const DecisionInput& in, bool e2) const noexcept {
    const HitReport& r = in.rep;
    const post04::ConsumeOutcome& o = in.outc;
    if (session_disabled_) return Reason::SessionDisabled;
    if (!in.ledger_valid || o.event != r.event || o.slot != r.slot || o.generation != r.generation || r.slot < 0) return Reason::TokenMismatch;
    if (in.adm != (e2 ? Admission::E_OC2_CANDIDATE : Admission::E_OC_CANDIDATE) || in.life != Life::Active || r.life != Life::Active) return Reason::NotCandidateActive;
    if (o.ever_frozen) return Reason::AdmissionEverUnstable;
    if (o.frozen || o.dead) return Reason::FrozenOrDead;
    if (r.post.old_mode != r.pre.mode) return Reason::PostSignatureMismatch;  // setActionMode corrio exactamente una vez en ESTA llamada
    if (r.pre.cooldown != 0 || r.pre.cutdown != 0 || r.post.cutdown != 0) return Reason::PreNotFresh;
    if (!r.col.deref_ok) return Reason::ColliderNotFresh;
    if (!r.aux_pre.valid || !r.aux_post.valid) return Reason::AuxMissing;
    if (!in.continuation_safe) return Reason::UnsafeContinuation;
    return Reason::Count;
  }
  Reason classify(const DecisionInput& in, E2Phase& phase, E2Compat& compat) const noexcept {
    const HitReport& r = in.rep;
    const post04::ConsumeOutcome& o = in.outc;
    namespace p = policy;
    if (r.j.kind != HitKind::Confirmed || r.j.elig != post03::Eligibility::Confirmed) return Reason::NotConfirmed;
    // ---- politica: fuera de alcance => la reaccion nativa transcurre intacta ----
    if (!p::kSuppressionEnabled) return Reason::SuppressionDisabled;
    // POST05-COMPAT C3/C4: E_OC2 (Red de Expansion = E_OC2 nativo; Black 3/6) se observa y acumula postura. Su reaccion queda NATIVA salvo que
    // la politica sea MatureBC (POST05-RB) y TODAS las senales coincidan en E_OC2; entonces sigue las MISMAS reglas que E_OC mas el gate de fase.
    const bool e2 = any_e_oc2(in);
    if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;
    if (!r.at_valid || r.fam.family != Family::Sword) return Reason::FamilyNotSword;
    if (r.j.lethal || r.post.health <= 1) return Reason::Lethal;
    if (r.post.mode == p::kModeBigDamage) return Reason::BigDamage;
    if (r.post.mode != p::kModeDamage) return Reason::PostModeNotDamage;
    if (r.post.cooldown != p::kCooldownOrdinary) return Reason::CooldownNotOrdinary;
    if (!o.applied) return Reason::NotWeighted;
    if (o.crossed || o.after >= post04::policy::kThreshold || o.before >= post04::policy::kThreshold) return Reason::CrossesThreshold;
    if (o.latched_after) return Reason::LatchedAfter;
    if (r.pre.mode != p::kModeAttack) return Reason::PreModeNotAttack;
    if (!p::attack_state_allowed(r.pre.state)) return Reason::PreStateNotWhitelisted;
    if (!p::ordinary_sword_damage_state(r.post.state)) return Reason::PostStateNotOrdinary;
    // ---- POST05-RB: gate E_OC2 de perfil y FASE NATIVA PRE (solo E_OC2; E_OC no pasa por aqui) ----
    if (e2) {
      if (!p::e2_profile_allowed(r.aux_pre.atp)) return Reason::E_OC2ProfileDeferred;
      phase = e2_phase(r.aux_pre, r.pre.state);
      if (!e2_allowed(phase, e2_env, r.aux_pre.atp, r.aux_pre.anim_rate, compat)) {
        if (phase == E2Phase::Startup) return Reason::E_OC2StartupTick;
        if (phase == E2Phase::FrameEarly) return Reason::E_OC2FrameEarly;
        if (phase == E2Phase::ClipEnding) return Reason::E_OC2ClipEnding;
        return Reason::E_OC2AnimUnverified;
      }
    }
    // ---- seguridad: cualquier duda => vanilla, contado ----
    if (const Reason s = common_safety(in, e2); s != Reason::Count) return s;  // identidad/generacion/firma/frescura/collider/aux (comun a ambas politicas)
    if (r.aux_pre.t6c0 != 0 || r.aux_pre.t6c2 != 0) return Reason::PreTimersNonZero;
    if (r.aux_pre.head_lock != 0) return Reason::PreHeadLock;
    if (r.aux_pre.set_bits != post03::kAllSetBits) return Reason::PreSetBitsOff;
    if (r.aux_post.t6c0 != 0 || r.aux_post.t6c2 != 0 || r.aux_post.head_lock != 0 || r.aux_post.set_bits != post03::kAllSetBits) return Reason::PostAuxUnexpected;
    if (e2 && !same_anim(r.aux_pre, r.aux_post)) return Reason::PostAuxUnexpected;  // POST05-RB: damage_check no toca la animacion; si cambio, escritor desconocido
    // POST05-COMPAT: la potencia o el recurso cambiaron DENTRO de esta llamada (escritor desconocido): no se toca nada
    if (r.aux_pre.atp != r.aux_post.atp || r.aux_pre.resource != r.aux_post.resource || r.aux_pre.atp == -2) return Reason::PowerChangedInCall;
    return Reason::Eligible;
  }
  // POST05-GENERAL (0.5.4). REGLA: impacto confirmado, no letal, cuya postura DESPUES de contarlo sigue < 100 (y no estaba enganchada) =>
  // se deshace la reaccion final de damage_check (DAMAGE o BIG_DAMAGE) en CUALQUIER modo normal. El modo/estado PRE NO es una condicion de
  // gameplay: solo decide el plan (volver al PRE exacto o, si estaba desprevenido, descubrir a Link como hace el nativo con el tirachinas).
  // Siguen nativos: letal/Finish, cruce a 100 (ruptura; POST06 no existe), latch 100, guion/muerte/agua/demo/caida, familia no verificada,
  // forma POST desconocida y cualquier fallo de identidad/coherencia (fail-closed por evento).
  Reason classify_general(const DecisionInput& in, E2Phase& phase, E2Compat& compat) const noexcept {
    const HitReport& r = in.rep;
    const post04::ConsumeOutcome& o = in.outc;
    namespace p = policy;
    if (r.j.kind != HitKind::Confirmed || r.j.elig != post03::Eligibility::Confirmed) return Reason::NotConfirmed;
    if (!p::kSuppressionEnabled) return Reason::SuppressionDisabled;
    const bool e2 = any_e_oc2(in);
    if (e2 && (e2_policy != E2Policy::MatureBC || !e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;  // identidad E_OC2 dudosa: nativo
    if (!r.at_valid || r.fam.family == Family::Unavailable || r.fam.family == Family::Ambiguous) return Reason::FamilyUnverified;
    if (r.post.health <= 1 || r.j.lethal) return Reason::Lethal;  // la muerte legitima manda
    // forma POST: la UNICA llamada final de damage_check
    const bool big = r.post.mode == p::kModeBigDamage;
    if (!(r.post.mode == p::kModeDamage && r.post.state >= 0 && r.post.state <= 4) && !(big && r.post.state >= 0 && r.post.state <= 2)) return Reason::PostShapeUnknown;
    if (r.post.cooldown != 3 && r.post.cooldown != p::kCooldownOrdinary && r.post.cooldown != 20) return Reason::CooldownNotOrdinary;
    // postura: decide la regla (no el modo)
    if (o.crossed) return Reason::CrossesThreshold;
    if (o.latched_after || o.before >= post04::policy::kThreshold || o.after >= post04::policy::kThreshold) return Reason::LatchedAfter;
    if (!o.applied && o.skip != post04::Skip::ZeroWeight) return Reason::PostureSkipUnsafe;  // sin sidecar/obsoleto/congelado/familia: nativo
    if (!p::general_pre_mode_allowed(r.pre.mode)) return Reason::PreModeScripted;
    // E_OC2 en ATTACK: la fase y el entorno solo se REGISTRAN (con Expansion, en el tick de arranque o un frame 1 con x2 posible, ese golpe pierde su
    // x1.15/x2 -> velocidad nativa; preferido a tambalearse por la regla general)
    if (e2 && r.pre.mode == p::kModeAttack && r.pre.state >= 0 && r.pre.state <= 2) {
      phase = post05::e2_phase(r.aux_pre, r.pre.state);
      if (!e2_allowed(phase, e2_env, r.aux_pre.atp, r.aux_pre.anim_rate, compat) && e2_env != E2Env::Absent &&
          (compat == E2Compat::RateDecisionThisTick || compat == E2Compat::X2PendingPossible))
        compat = E2Compat::RateLostThisBlow;
    }
    // ---- seguridad: cualquier duda => vanilla, contado ----
    if (const Reason s = common_safety(in, e2); s != Reason::Count) return s;  // identidad/generacion/firma/frescura/collider/aux (comun a ambas politicas)
    if (r.aux_post.t6c0 != 0 || r.aux_post.t6c2 != 0 || r.aux_post.head_lock != 0 || r.aux_post.set_bits != (big ? p::kBitsCoOnly : post03::kAllSetBits))
      return Reason::PostAuxUnexpected;  // la forma exacta que deja setActionMode (+ offTgSph en BIG_DAMAGE)
    if (e2 && !same_anim(r.aux_post, r.aux_pre)) return Reason::PostAuxUnexpected;
    if (r.aux_pre.atp == -2 || r.aux_post.atp != r.aux_pre.atp || r.aux_post.resource != r.aux_pre.resource) return Reason::PowerChangedInCall;
    return Reason::Eligible;
  }
};

// ---- formato (sin punteros ni rutas) -------------------------------------------------------------------------------------------------
inline const char* resource_text(std::uint8_t k) noexcept {
  switch (static_cast<post03::NameKind>(k)) {
    case post03::NameKind::E_OC: return "E_OC";
    case post03::NameKind::E_OC2: return "E_OC2";
    case post03::NameKind::Other: return "OTHER";
    default: return "UNREAD";
  }
}
inline double clamp_print(float v) noexcept {  // longitud de linea acotada (NaN -> -999)
  if (!(v == v)) return -999.0;
  return v > 999.0f ? 999.0 : v < -999.0f ? -999.0 : static_cast<double>(v);
}
inline int fmt_decision(char* out, std::size_t n, const HitReport& r, const post04::ConsumeOutcome& o, const DecisionResult& d, Apply a) noexcept {
  return std::snprintf(out, n,
                       "POST05_DECISION id=%u generation=%llu event=%llu family=%s decision=%s reason=%s pre_mode=%d pre_state=%d post_mode=%d post_state=%d cd=%d>%d "
                       "posture=%d>%d action=%s resource=%s atp=%d>%d admission=%s phase=%s frame=%.2f rate=%.2f speed=%.1f env=%s compat=%s plan=%s",
                       r.id, static_cast<unsigned long long>(r.generation), static_cast<unsigned long long>(r.event),
                       r.at_valid ? post03::name(r.fam.family) : "UNAVAILABLE", name(d.decision), name(d.reason), r.pre.mode, r.pre.state, r.post.mode,
                       r.post.state, r.pre.cooldown, r.post.cooldown, o.before, o.after, name(a), resource_text(r.aux_pre.resource), r.aux_pre.atp, r.aux_post.atp,
                       post03::name(d.adm), name(d.phase), clamp_print(r.aux_pre.anim_frame), clamp_print(r.aux_pre.anim_rate), clamp_print(r.aux_pre.speed),
                       d.phase == E2Phase::NotE2 ? "NA" : name(d.env), name(d.compat), name(d.plan.kind));
}
inline int fmt_summary(char* out, std::size_t n, const Reaction& x, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  const RCounters& c = x.c;
  int k = std::snprintf(out, n, "POST05_SUMMARY v=%s tag=%s decisions=%llu duplicates=%llu not_applicable=%llu allow_vanilla=%llu skip_unsafe=%llu eligible=%llu "
                                "confirmed=%llu attempted=%llu applied=%llu recheck_failed=%llu verify_failed=%llu not_attempted=%llu applied_by_pre_state(0/1/2)=%llu/%llu/%llu "
                                "session_disabled=%d log(written/dropped)=%llu/%llu resource(e_oc/e_oc2/other)=%llu/%llu/%llu e2_policy=%s "
                                "e2_phase(na/anim/startup/early/ending/mature/brake)=%llu/%llu/%llu/%llu/%llu/%llu/%llu applied_by_profile(e_oc/atp2/atp3/atp6)=%llu/%llu/%llu/%llu "
                                "e2_applied_phase(startup/early/ending/mature/brake)=%llu/%llu/%llu/%llu/%llu e2_env(unknown/native_only/possible)=%llu/%llu/%llu "
                                "env_now=%s stagger_policy=%s applied_by_plan(restore/find1)=%llu/%llu applied_big_damage=%llu "
                                "applied_by_pre_mode(0..16)=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu reasons=",
                        kReactionVersion, tag, u(c.decisions), u(c.duplicates), u(c.by_decision[0]), u(c.by_decision[1]), u(c.by_decision[2]), u(c.by_decision[3]),
                        u(c.confirmed), u(c.attempted), u(c.applied), u(c.recheck_failed), u(c.verify_failed), u(c.not_attempted), u(c.applied_by_pre_state[0]),
                        u(c.applied_by_pre_state[1]), u(c.applied_by_pre_state[2]), x.session_disabled() ? 1 : 0, u(c.log_written), u(c.log_dropped),
                        u(c.by_resource[0]), u(c.by_resource[1]), u(c.by_resource[2]), name(x.e2_policy), u(c.e2_phase[0]), u(c.e2_phase[1]), u(c.e2_phase[2]),
                        u(c.e2_phase[3]), u(c.e2_phase[4]), u(c.e2_phase[5]), u(c.e2_phase[6]), u(c.applied_by_profile[0]), u(c.applied_by_profile[1]),
                        u(c.applied_by_profile[2]), u(c.applied_by_profile[3]), u(c.e2_applied_phase[2]), u(c.e2_applied_phase[3]), u(c.e2_applied_phase[4]),
                        u(c.e2_applied_phase[5]), u(c.e2_applied_phase[6]), u(c.e2_env_decisions[0]), u(c.e2_env_decisions[1]), u(c.e2_env_decisions[2]), name(x.e2_env), name(x.stagger_policy),
                        u(c.applied_by_kind[1]), u(c.applied_by_kind[2]), u(c.applied_big), u(c.applied_by_pre_mode[0]), u(c.applied_by_pre_mode[1]),
                        u(c.applied_by_pre_mode[2]), u(c.applied_by_pre_mode[3]), u(c.applied_by_pre_mode[4]), u(c.applied_by_pre_mode[5]), u(c.applied_by_pre_mode[6]),
                        u(c.applied_by_pre_mode[7]), u(c.applied_by_pre_mode[8]), u(c.applied_by_pre_mode[9]), u(c.applied_by_pre_mode[10]), u(c.applied_by_pre_mode[11]),
                        u(c.applied_by_pre_mode[12]), u(c.applied_by_pre_mode[13]), u(c.applied_by_pre_mode[14]), u(c.applied_by_pre_mode[15]), u(c.applied_by_pre_mode[16]));
  for (unsigned i = 0; i < kReasonCount && k > 0 && static_cast<std::size_t>(k) < n; ++i)
    k += std::snprintf(out + k, n - static_cast<std::size_t>(k), "%s%llu", i ? "/" : "", u(c.by_reason[i]));
  return k;
}
inline int fmt_conservation(char* out, std::size_t n, const Reaction& x, const char* tag) noexcept {
  const Reaction::Audit a = x.audit();
  return std::snprintf(out, n, "POST05_CONSERVATION v=%s tag=%s ok=%d r1_decisions=%d r2_reasons=%d r3_eligible=%d r4_attempted=%d r5_bounds=%d r6_resources=%d r7_profiles=%d r8_e2_phases=%d",
                       kReactionVersion, tag, a.ok ? 1 : 0, a.r1 ? 1 : 0, a.r2 ? 1 : 0, a.r3 ? 1 : 0, a.r4 ? 1 : 0, a.r5 ? 1 : 0, a.r6 ? 1 : 0, a.r7 ? 1 : 0, a.r8 ? 1 : 0);
}

// ---- tubo POST completo (lo usan mod.cpp y el laboratorio; ningun acceso al juego aqui) -------------------------------------------------------
// Construye la entrada desde el motor de postura (que ya consumio el informe) y el ledger, decide y, si procede, aplica con el escritor dado.
template <class Read, class Write>
DecisionResult process(Reaction& rx, const post04::PostureEngine& pe, const HitReport& rep, Read read, Write write, Apply& applied, bool continuation_safe = true) noexcept {
  DecisionInput in;
  in.rep = rep;
  in.outc = pe.last;
  in.continuation_safe = continuation_safe;
  const post03::Ledger& L = pe.eng.obs.ledger;
  if (rep.slot >= 0 && static_cast<unsigned>(rep.slot) < post03::kCapacity) {
    const post03::Instance& s = L.slot(static_cast<unsigned>(rep.slot));
    in.ledger_valid = L.valid(rep.slot, rep.generation) && s.id == rep.id;
    in.adm = s.adm;
    in.life = s.life;
  }
  const DecisionResult d = rx.decide(in);
  applied = rx.apply(d, read, write);
  return d;
}

}  // namespace post05
