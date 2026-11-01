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
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "posture_core.hpp"

namespace post05 {

using post03::Admission;
using post03::Family;
using post03::HitKind;
using post03::HitReport;
using post03::Life;

inline constexpr const char* kReactionVersion = "0.5.0";

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
inline constexpr const char* kPolicyId = "POST05_TEST_POLICY_V1_ATTACK_STATES_0_1_2";
}  // namespace policy

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
  SuppressionDisabled, FamilyNotSword, Lethal, BigDamage, PostModeNotDamage, CooldownNotOrdinary, NotWeighted, CrossesThreshold, LatchedAfter,
  PreModeNotAttack, PreStateNotWhitelisted, PostStateNotOrdinary,  // ALLOW_VANILLA (politica)
  SessionDisabled, TokenMismatch, NotCandidateActive, AdmissionEverUnstable, FrozenOrDead, PostSignatureMismatch, PreNotFresh, ColliderNotFresh,
  AuxMissing, PreTimersNonZero, PreHeadLock, PreSetBitsOff, PostAuxUnexpected,                      // SKIP_UNSAFE (seguridad)
  Eligible,                                                                                        // SUPPRESS_ELIGIBLE
  Count
};
inline constexpr unsigned kReasonCount = static_cast<unsigned>(Reason::Count);
inline const char* name(Reason r) noexcept {
  static constexpr const char* k[kReasonCount] = {
      "NOT_CONFIRMED", "SUPPRESSION_DISABLED", "FAMILY_NOT_SWORD", "LETHAL", "BIG_DAMAGE", "POST_MODE_NOT_DAMAGE", "COOLDOWN_NOT_ORDINARY", "NOT_WEIGHTED",
      "CROSSES_THRESHOLD", "LATCHED_AFTER", "PRE_MODE_NOT_ATTACK", "PRE_STATE_NOT_WHITELISTED", "POST_STATE_NOT_ORDINARY", "SESSION_DISABLED", "TOKEN_MISMATCH",
      "NOT_CANDIDATE_ACTIVE", "ADMISSION_EVER_UNSTABLE", "FROZEN_OR_DEAD", "POST_SIGNATURE_MISMATCH", "PRE_NOT_FRESH", "COLLIDER_NOT_FRESH", "AUX_MISSING",
      "PRE_TIMERS_NONZERO", "PRE_HEAD_LOCK", "PRE_SET_BITS_OFF", "POST_AUX_UNEXPECTED", "ELIGIBLE"};
  const unsigned i = static_cast<unsigned>(r);
  return i < kReasonCount ? k[i] : "NONE";
}
inline Decision decision_of(Reason r) noexcept {
  if (r == Reason::NotConfirmed) return Decision::NotApplicable;
  if (r <= Reason::PostStateNotOrdinary) return Decision::AllowVanilla;
  if (r <= Reason::PostAuxUnexpected) return Decision::SkipUnsafe;
  return Decision::SuppressEligible;
}

// Entrada POR VALOR (nada de punteros a actores): informe de POST03.1, resultado de POST04 y estado del ledger para el mismo token.
struct DecisionInput {
  HitReport rep{};
  post04::ConsumeOutcome outc{};
  bool ledger_valid = false;  // valid(slot, generation) && id coincide
  Admission adm = Admission::UNKNOWN_UNSAFE;
  Life life = Life::Free;
};
struct RestorePlan {
  int mode = 0, state = 0, old_mode = 0;                 // valores PRE a restaurar
  int expect_mode = 0, expect_state = 0, expect_old = 0;  // valores que setActionMode dejo (se re-comprueban justo antes de escribir)
};
struct DecisionResult {
  bool counted = false;  // false = informe duplicado (no es una decision nueva)
  Decision decision = Decision::NotApplicable;
  Reason reason = Reason::NotConfirmed;
  RestorePlan plan{};
};
struct LiveMode {
  int mode = 0, state = 0, old_mode = 0;
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
  std::uint64_t log_written = 0, log_dropped = 0;
};

class Reaction {
 public:
  RCounters c{};
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
    d.reason = classify(in);
    d.decision = decision_of(d.reason);
    ++c.decisions;
    ++c.by_decision[static_cast<unsigned>(d.decision)];
    ++c.by_reason[static_cast<unsigned>(d.reason)];
    if (r.j.kind == HitKind::Confirmed) ++c.confirmed;
    if (d.decision == Decision::SuppressEligible) {
      d.plan.mode = r.pre.mode;
      d.plan.state = r.pre.state;
      d.plan.old_mode = r.pre.old_mode;
      d.plan.expect_mode = r.post.mode;
      d.plan.expect_state = r.post.state;
      d.plan.expect_old = r.post.old_mode;
    }
    return d;
  }

  // Aplica el plan: relee los tres campos, escribe SOLO esos tres y verifica. `read` y `write` los da mod.cpp (o el laboratorio).
  template <class Read, class Write>
  Apply apply(const DecisionResult& d, Read read, Write write) noexcept {
    if (!d.counted || d.decision != Decision::SuppressEligible) return Apply::NotAttempted;
    if (session_disabled_) {
      ++c.not_attempted;
      return Apply::NotAttempted;
    }
    ++c.attempted;
    const LiveMode now = read();
    if (now.mode != d.plan.expect_mode || now.state != d.plan.expect_state || now.old_mode != d.plan.expect_old) {
      ++c.recheck_failed;  // algo cambio entre la lectura POST y este punto: no se escribe nada
      return Apply::RecheckFailed;
    }
    write(d.plan.mode, d.plan.state, d.plan.old_mode);
    const LiveMode after = read();
    if (after.mode != d.plan.mode || after.state != d.plan.state || after.old_mode != d.plan.old_mode) {
      ++c.verify_failed;
      session_disabled_ = true;  // fallo seguro: no se intenta ninguna supresion mas en la sesion
      return Apply::VerifyFailed;
    }
    ++c.applied;
    if (d.plan.state >= 0 && d.plan.state < 3) ++c.applied_by_pre_state[d.plan.state];
    return Apply::Applied;
  }

  // Ecuaciones de conservacion de POST05 (independientes de POST03/POST04)
  struct Audit {
    bool r1 = false, r2 = false, r3 = false, r4 = false, r5 = false, ok = false;
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
    a.r5 = c.attempted <= eligible && eligible <= c.confirmed && c.confirmed <= c.decisions && c.applied_by_pre_state[0] + c.applied_by_pre_state[1] + c.applied_by_pre_state[2] == c.applied;
    a.ok = a.r1 && a.r2 && a.r3 && a.r4 && a.r5;
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

  Reason classify(const DecisionInput& in) const noexcept {
    const HitReport& r = in.rep;
    const post04::ConsumeOutcome& o = in.outc;
    namespace p = policy;
    if (r.j.kind != HitKind::Confirmed || r.j.elig != post03::Eligibility::Confirmed) return Reason::NotConfirmed;
    // ---- politica: fuera de alcance => la reaccion nativa transcurre intacta ----
    if (!p::kSuppressionEnabled) return Reason::SuppressionDisabled;
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
    // ---- seguridad: cualquier duda => vanilla, contado ----
    if (session_disabled_) return Reason::SessionDisabled;
    if (!in.ledger_valid || o.event != r.event || o.slot != r.slot || o.generation != r.generation || r.slot < 0) return Reason::TokenMismatch;
    if (in.adm != Admission::E_OC_CANDIDATE || in.life != Life::Active || r.life != Life::Active) return Reason::NotCandidateActive;
    if (o.ever_frozen) return Reason::AdmissionEverUnstable;
    if (o.frozen || o.dead) return Reason::FrozenOrDead;
    if (r.post.old_mode != r.pre.mode) return Reason::PostSignatureMismatch;
    if (r.pre.cooldown != 0 || r.pre.cutdown != 0 || r.post.cutdown != 0) return Reason::PreNotFresh;
    if (!r.col.deref_ok) return Reason::ColliderNotFresh;
    if (!r.aux_pre.valid || !r.aux_post.valid) return Reason::AuxMissing;
    if (r.aux_pre.t6c0 != 0 || r.aux_pre.t6c2 != 0) return Reason::PreTimersNonZero;
    if (r.aux_pre.head_lock != 0) return Reason::PreHeadLock;
    if (r.aux_pre.set_bits != post03::kAllSetBits) return Reason::PreSetBitsOff;
    if (r.aux_post.t6c0 != 0 || r.aux_post.t6c2 != 0 || r.aux_post.head_lock != 0 || r.aux_post.set_bits != post03::kAllSetBits) return Reason::PostAuxUnexpected;
    return Reason::Eligible;
  }
};

// ---- formato (sin punteros ni rutas) -------------------------------------------------------------------------------------------------
inline int fmt_decision(char* out, std::size_t n, const HitReport& r, const post04::ConsumeOutcome& o, const DecisionResult& d, Apply a) noexcept {
  return std::snprintf(out, n,
                       "POST05_DECISION id=%u generation=%llu event=%llu family=%s decision=%s reason=%s pre_mode=%d pre_state=%d post_mode=%d post_state=%d cd=%d>%d "
                       "posture=%d>%d action=%s",
                       r.id, static_cast<unsigned long long>(r.generation), static_cast<unsigned long long>(r.event),
                       r.at_valid ? post03::name(r.fam.family) : "UNAVAILABLE", name(d.decision), name(d.reason), r.pre.mode, r.pre.state, r.post.mode,
                       r.post.state, r.pre.cooldown, r.post.cooldown, o.before, o.after, name(a));
}
inline int fmt_summary(char* out, std::size_t n, const Reaction& x, const char* tag) noexcept {
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  const RCounters& c = x.c;
  int k = std::snprintf(out, n, "POST05_SUMMARY v=%s tag=%s decisions=%llu duplicates=%llu not_applicable=%llu allow_vanilla=%llu skip_unsafe=%llu eligible=%llu "
                                "confirmed=%llu attempted=%llu applied=%llu recheck_failed=%llu verify_failed=%llu not_attempted=%llu applied_by_pre_state(0/1/2)=%llu/%llu/%llu "
                                "session_disabled=%d log(written/dropped)=%llu/%llu reasons=",
                        kReactionVersion, tag, u(c.decisions), u(c.duplicates), u(c.by_decision[0]), u(c.by_decision[1]), u(c.by_decision[2]), u(c.by_decision[3]),
                        u(c.confirmed), u(c.attempted), u(c.applied), u(c.recheck_failed), u(c.verify_failed), u(c.not_attempted), u(c.applied_by_pre_state[0]),
                        u(c.applied_by_pre_state[1]), u(c.applied_by_pre_state[2]), x.session_disabled() ? 1 : 0, u(c.log_written), u(c.log_dropped));
  for (unsigned i = 0; i < kReasonCount && k > 0 && static_cast<std::size_t>(k) < n; ++i)
    k += std::snprintf(out + k, n - static_cast<std::size_t>(k), "%s%llu", i ? "/" : "", u(c.by_reason[i]));
  return k;
}
inline int fmt_conservation(char* out, std::size_t n, const Reaction& x, const char* tag) noexcept {
  const Reaction::Audit a = x.audit();
  return std::snprintf(out, n, "POST05_CONSERVATION v=%s tag=%s ok=%d r1_decisions=%d r2_reasons=%d r3_eligible=%d r4_attempted=%d r5_bounds=%d", kReactionVersion, tag,
                       a.ok ? 1 : 0, a.r1 ? 1 : 0, a.r2 ? 1 : 0, a.r3 ? 1 : 0, a.r4 ? 1 : 0, a.r5 ? 1 : 0);
}

// ---- tubo POST completo (lo usan mod.cpp y el laboratorio; ningun acceso al juego aqui) -------------------------------------------------------
// Construye la entrada desde el motor de postura (que ya consumio el informe) y el ledger, decide y, si procede, aplica con el escritor dado.
template <class Read, class Write>
DecisionResult process(Reaction& rx, const post04::PostureEngine& pe, const HitReport& rep, Read read, Write write, Apply& applied) noexcept {
  DecisionInput in;
  in.rep = rep;
  in.outc = pe.last;
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
