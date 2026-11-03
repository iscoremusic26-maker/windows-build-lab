#pragma once
// LNK SESSION02 FIX01 0.11.1: defensa MANUAL (R) + penalizacion extendida tras la ruptura de postura de Link. Logica PURA: sin cabeceras del juego, sin
// direcciones, sin escrituras. El adaptador (mod.cpp) le pasa valores leidos de la Link viva y aplica sus decisiones con las rutas nativas.
//
// Hechos nativos [S 2.0.3 d_a_alink*.inc/.cpp, B EXE 2.0.3]:
//  * daAlink_c::execute: setStickData (d_a_alink.cpp:18020) -> checkDamageAction (18262) -> setShieldGuard (18281) -> procedimiento (18308) ->
//    setCollision (18642). setStickData muestrea PAD_1 en mItemTrigger/mItemButton (BTN_B 0x08, BTN_A 0x10, BTN_R 0x40 = getHoldLockR/getTrigLockR).
//  * setShieldGuard (guard.inc:186) enciende FLG2_UNK_8000000 si (GUARD_SLIP sin bola) || checkSmallUpperGuardAnime() || (checkGuardAccept() && ... &&
//    checkGuardActionChange()) -- la ultima rama es la guardia por FIJACION (checkAttentionLock). LazyTweaks 1.0.0 (Kamilink, hooks.cpp a0c7a320) anade
//    getHoldLockR(PAD_1) SOLO a esa rama recalculando en un POST; como el estado no cambia entre el original y el POST, es equivalente a APAGAR la bandera
//    cuando la rama de fijacion la encendio y R no esta pulsado. Este mod solo apaga (nunca enciende).
//  * Shield Attack nativo (d_a_alink.cpp:11896-11909): fijacion + condiciones -> setRStatus(BUTTON_STATUS_SHIELD_ATTACK 0x3a) -> spActionTrigger() (= R)
//    -> procGuardAttackInit(). EXE: llamada real a setRStatus@daAlink_c y a spActionTrigger en ese sitio (no inlinado); procGuardAttackInit solo desde ahi.
//  * Esquivas voluntarias (11920 checkMoveDoAction): doTrigger (A) con DoStatus JUMP 0x19 -> salto lateral / salto atras (procSideStepInit ->
//    procBackJumpInit) / rodada lateral (checkSideRollAction); DoStatus 0x79 -> rodada frontal. Las demas llamadas a esos procInit son forzadas
//    (aterrizaje, buceo, demo, caballo, colgado, corte) y no pasan por aqui.
//  * setCollision (6761) registra el escudo Tg si checkPlayerGuardAndAttack() && !ERFLG1_UNK_20. procGuardBreak re-enciende la bandera cada frame
//    (guard.inc:512); en el frame de salida, si Link sigue fijando, el escudo quedaria registrado UN frame: la penalizacion lo cierra en setCollision.
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace lnkfx {
inline constexpr const char* kVersion = "0.11.1";

namespace policy {
// Balance (prueba): ventana TOTAL de penalizacion desde la entrada nativa confirmada, en ticks de simulacion ACTIVOS (mod_update, 30 Hz nominal; la
// pausa/menu congela el reloj). 75 ticks = 2,5 s. La fase nativa (PROC_GUARD_BREAK) dura lo que dura (28..36 ticks observados en 0.11.0) y nunca se acorta.
inline constexpr std::uint64_t kTotalPenaltyTicks = 75;
inline constexpr std::uint64_t kNativeWatchdogTicks = 150;  // solo diagnostico
inline constexpr bool kManualShield = true, kExtendedPenalty = true;  // feature gates de diagnostico (la build privada lleva ambos)
inline constexpr unsigned kBtnB = 0x08, kBtnA = 0x10, kBtnR = 0x40;  // daAlink_c::daAlink_ITEM_BTN (static_assert en mod.cpp)
inline constexpr unsigned kStatusShieldAttack = 0x3a, kStatusJump = 0x19, kStatusFrontRoll = 0x79;  // BUTTON_STATUS_* (static_assert en mod.cpp)
inline constexpr int kProcGuardSlip = 0x1d, kProcGuardAttack = 0x1e, kProcGuardBreak = 0x1f;
}  // namespace policy

enum class Phase : std::uint8_t { Ready = 0, NativeBreak, PostVulnerable, RearmRequired };
inline const char* name(Phase p) noexcept {
  static constexpr const char* n[] = {"READY", "NATIVE_BREAK_ACTIVE", "POST_BREAK_VULNERABLE", "REARM_REQUIRED"};
  return static_cast<unsigned>(p) < 4 ? n[static_cast<unsigned>(p)] : "READY";
}
enum class Origin : std::uint8_t { None = 0, ModBreak, NativeAdopted, NativeObserved };
inline const char* name(Origin o) noexcept {
  static constexpr const char* n[] = {"NONE", "MOD_BREAK", "NATIVE_ADOPTED", "NATIVE_OBSERVED"};
  return static_cast<unsigned>(o) < 4 ? n[static_cast<unsigned>(o)] : "NONE";
}
enum class Ev : std::uint8_t {
  None = 0,
  EnterNative,      // ruptura fisica confirmada: comienza el reloj (extendida si satura postura)
  DuplicateBegin,   // segunda entrada durante una penalizacion: ignorada (sin cola, sin reinicio de reloj)
  NativeExit,       // PROC_GUARD_BREAK termino/cancelado/interrumpido (observado); la postura ya se reinicio en LNK06
  PostVulnerable,   // salida nativa antes del presupuesto: sigue la penalizacion sin animacion
  Complete,         // presupuesto cumplido (y salida nativa): restricciones fuera, escudo pendiente de rearme
  Rearmed,          // nueva pulsacion de R tras el desbloqueo
  Aborted,          // muerte, lobo, Link nueva/ausente, evento: limpieza (sin escudo bloqueado para siempre)
  Watchdog,         // GUARD_BREAK nativo mas alla de kNativeWatchdogTicks (solo diagnostico)
  Count
};
inline const char* name(Ev e) noexcept {
  static constexpr const char* n[] = {"NONE",     "ENTER_NATIVE", "DUPLICATE_BEGIN_IGNORED", "NATIVE_EXIT", "POST_VULNERABLE",
                                      "COMPLETE", "SHIELD_REARMED", "ABORTED_SAFE",          "NATIVE_WATCHDOG"};
  return static_cast<unsigned>(e) < static_cast<unsigned>(Ev::Count) ? n[static_cast<unsigned>(e)] : "NONE";
}

struct Begin {
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0, tick = 0;
  Origin origin = Origin::None;
  bool extended = false;  // true solo si la ruptura proviene de una saturacion de postura (LNK05 propia o nativa adoptada)
};
struct Obs {  // una vez por mod_update, por valor
  bool valid = false, human = false, alive = false, sim_active = false, event = false;
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0, tick = 0;
  int proc = -1;
};
struct Out {
  Ev ev = Ev::None;
  Phase before = Phase::Ready, after = Phase::Ready;
  Origin origin = Origin::None;
  bool extended = false;
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0, entry_tick = 0, tick = 0, active_ticks = 0, native_ticks = 0, remaining = 0;
  const char* reason = "";
};
struct Counters {
  std::uint64_t begins = 0, begins_extended = 0, begins_latch_only = 0, duplicate_begins = 0, native_exits = 0, post_vulnerable = 0, completes = 0,
                rearmed = 0, aborted = 0, watchdog = 0, paused_ticks = 0, active_ticks = 0, samples = 0, r_edges = 0, released_after_unlock = 0,
                held_through_unlock = 0;
  std::uint64_t window_min = ~0ull, window_max = 0, native_min = ~0ull, native_max = 0;  // ticks activos medidos (no estimados)
};

// Maquina determinista de restricciones defensivas, SEPARADA de la postura (lnk01::Posture) y de la ruptura (lnk05::Machine).
class Defense {
 public:
  Counters c{};

  Out begin(const Begin& b) noexcept {
    Out o = base(b.tick);
    if (phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable) {  // nunca una cola ni un reinicio del reloj
      ++c.duplicate_begins;
      o.ev = Ev::DuplicateBegin, o.reason = "PENALTY_ALREADY_ACTIVE";
      return o;
    }
    const Phase prev = phase_;  // READY o REARM_REQUIRED (un nuevo ciclo completo tras la recuperacion)
    ++c.begins;
    ++(b.extended ? c.begins_extended : c.begins_latch_only);
    phase_ = Phase::NativeBreak;
    origin_ = b.origin, extended_ = b.extended && policy::kExtendedPenalty;
    link_id_ = b.link_id, epoch_ = b.epoch, entry_tick_ = b.tick, active_ = 0, native_ = 0, need_release_ = true;
    o = base(b.tick);
    o.before = prev;
    o.ev = Ev::EnterNative, o.reason = extended_ ? "POSTURE_BREAK_EXTENDED" : "NATIVE_BREAK_LATCH_ONLY";
    return o;
  }

  // UNA observacion por mod_update. Reloj = ticks de simulacion activos (pausa/menu congelan; evento/muerte/lobo/Link nueva abortan).
  Out observe(const Obs& in) noexcept {
    Out o = base(in.tick);
    if (!in.valid || in.link_id != link_seen_ || in.epoch != epoch_seen_) {  // identidad de entrada: nunca se transplanta R de otra Link
      r_valid_ = false;
      link_seen_ = in.valid ? in.link_id : 0, epoch_seen_ = in.valid ? in.epoch : 0;
    }
    if (phase_ == Phase::Ready) return o;
    const bool same = in.valid && in.link_id == link_id_ && in.epoch == epoch_;
    if (!same || !in.human || !in.alive || in.event) {
      ++c.aborted;
      o.ev = Ev::Aborted;
      o.reason = !same ? "LINK_CHANGED_OR_ABSENT" : (!in.human ? "NOT_HUMAN" : (!in.alive ? "DEAD" : "EVENT"));
      reset_phase();
      o.after = phase_;
      return o;
    }
    if (!in.sim_active) {  // pausa / menu: el reloj no avanza y nada se desbloquea
      if (phase_ != Phase::RearmRequired) ++c.paused_ticks;
      return o;
    }
    if (phase_ == Phase::RearmRequired) return o;
    ++active_, ++c.active_ticks;
    if (phase_ == Phase::NativeBreak) {
      if (in.proc == policy::kProcGuardBreak) {
        ++native_;
        if (native_ == policy::kNativeWatchdogTicks) ++c.watchdog, o.ev = Ev::Watchdog, o.reason = "NATIVE_GUARD_BREAK_TOO_LONG_DIAGNOSTIC_ONLY";
        o.active_ticks = active_, o.native_ticks = native_, o.remaining = remaining();
        return o;
      }
      ++c.native_exits;
      c.native_min = native_ < c.native_min ? native_ : c.native_min, c.native_max = native_ > c.native_max ? native_ : c.native_max;
      if (extended_ && active_ < policy::kTotalPenaltyTicks) {
        ++c.post_vulnerable;
        phase_ = Phase::PostVulnerable;
        o.ev = Ev::PostVulnerable, o.reason = "NATIVE_EXIT_BUDGET_REMAINS";
      } else {
        o.ev = Ev::NativeExit, o.reason = extended_ ? "NATIVE_EXIT_AFTER_BUDGET" : "NATIVE_EXIT_LATCH_ONLY";
        finish();
      }
    } else if (phase_ == Phase::PostVulnerable && active_ >= policy::kTotalPenaltyTicks) {
      o.ev = Ev::Complete, o.reason = "BUDGET_ELAPSED";
      finish();
    }
    o.after = phase_, o.active_ticks = active_, o.native_ticks = native_, o.remaining = remaining();
    return o;
  }

  // UNA muestra por execute de Link (POST de setStickData): R mantenido segun el propio juego (mItemButton & BTN_R). Rearme = R soltado tras el desbloqueo
  // y vuelto a pulsar; mantener R a traves del desbloqueo, o pulsarlo durante la penalizacion y seguir manteniendolo, no rearma.
  Out sample(std::uint32_t link_id, std::uint64_t epoch, bool r_held, std::uint64_t tick) noexcept {
    Out o = base(tick);
    ++c.samples;
    if (!r_valid_ || link_id != link_seen_ || epoch != epoch_seen_) r_valid_ = false, link_seen_ = link_id, epoch_seen_ = epoch;
    const bool edge = r_valid_ && r_held && !r_prev_;
    if (edge) ++c.r_edges;
    r_prev_ = r_held, r_valid_ = true;
    if (phase_ != Phase::RearmRequired || link_id != link_id_ || epoch != epoch_) return o;
    if (!r_held) {
      if (need_release_) ++c.released_after_unlock;
      need_release_ = false;
      return o;
    }
    if (need_release_) return o;  // R mantenido desde antes del desbloqueo
    ++c.rearmed;
    o.ev = Ev::Rearmed, o.reason = "NEW_R_PRESS_AFTER_UNLOCK";
    reset_phase();
    o.after = phase_;
    return o;
  }

  // Consultas para los adaptadores (todas O(1)).
  Phase phase() const noexcept { return phase_; }
  bool guard_blocked() const noexcept { return phase_ != Phase::Ready; }
  bool bash_blocked() const noexcept { return phase_ != Phase::Ready; }
  bool dodge_blocked() const noexcept { return extended_ && (phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable); }
  bool request_blocked() const noexcept { return phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable; }
  bool owns(std::uint32_t link_id, std::uint64_t epoch) const noexcept { return phase_ != Phase::Ready && link_id == link_id_ && epoch == epoch_; }
  bool extended() const noexcept { return extended_; }
  bool need_release() const noexcept { return need_release_; }
  std::uint64_t active_ticks() const noexcept { return active_; }
  std::uint64_t remaining() const noexcept { return extended_ && active_ < policy::kTotalPenaltyTicks ? policy::kTotalPenaltyTicks - active_ : 0; }
  Origin origin() const noexcept { return origin_; }

  // Cierre de sesion / recarga: nada persiste en el juego; el estado propio vuelve a READY.
  void shutdown() noexcept { reset_phase(), r_valid_ = false; }

 private:
  Out base(std::uint64_t tick) const noexcept {
    Out o;
    o.before = o.after = phase_, o.origin = origin_, o.extended = extended_, o.link_id = link_id_, o.epoch = epoch_, o.entry_tick = entry_tick_;
    o.tick = tick, o.active_ticks = active_, o.native_ticks = native_, o.remaining = remaining();
    return o;
  }
  void finish() noexcept {
    if (extended_) {
      ++c.completes;
      c.window_min = active_ < c.window_min ? active_ : c.window_min, c.window_max = active_ > c.window_max ? active_ : c.window_max;
    }
    phase_ = Phase::RearmRequired;
    need_release_ = true;
    if (r_valid_ && r_prev_) ++c.held_through_unlock;
  }
  void reset_phase() noexcept {
    phase_ = Phase::Ready, origin_ = Origin::None, extended_ = false, link_id_ = 0, epoch_ = 0, entry_tick_ = 0, active_ = 0, native_ = 0;
    need_release_ = false;
  }
  Phase phase_ = Phase::Ready;
  Origin origin_ = Origin::None;
  bool extended_ = false, need_release_ = false, r_prev_ = false, r_valid_ = false;
  std::uint32_t link_id_ = 0, link_seen_ = 0;
  std::uint64_t epoch_ = 0, epoch_seen_ = 0, entry_tick_ = 0, active_ = 0, native_ = 0;
};

// ---------------------------------------------------------------- decisiones de los adaptadores (puras) ----------------------------------------------
// POST de setShieldGuard: la bandera ya la calculo el juego. mandatory = GUARD_SLIP (sin bola) o guardia pequena: reaccion nativa, jamas tocada.
enum class Guard : std::uint8_t { NativeOff = 0, Mandatory, Accepted, DeniedNoR, DeniedPenalty, DeniedRearm, Count };
inline const char* name(Guard g) noexcept {
  static constexpr const char* n[] = {"NATIVE_OFF", "NATIVE_MANDATORY", "GUARD_ENABLED", "DENIED_NO_R", "BLOCKED_BY_PENALTY", "REARM_REQUIRED"};
  return static_cast<unsigned>(g) < static_cast<unsigned>(Guard::Count) ? n[static_cast<unsigned>(g)] : "NATIVE_OFF";
}
inline Guard decide_guard(bool flag_after_native, bool mandatory, bool r_held, Phase p) noexcept {
  if (!flag_after_native) return Guard::NativeOff;
  if (mandatory) return Guard::Mandatory;
  if (p == Phase::NativeBreak || p == Phase::PostVulnerable) return Guard::DeniedPenalty;
  if (p == Phase::RearmRequired) return Guard::DeniedRearm;
  if (!r_held) return Guard::DeniedNoR;
  return Guard::Accepted;
}
inline bool clears_flag(Guard g) noexcept { return g == Guard::DeniedNoR || g == Guard::DeniedPenalty || g == Guard::DeniedRearm; }

// LNK04 adaptado: x1.5 solo con guardia nativa segura Y guardia manual realmente aceptada en el ultimo execute Y defensa en READY.
// Sin FIX01 (hooks no instalados): la regla exacta de 0.11.0 (solo la guardia nativa segura).
inline bool regen_bonus(bool native_guard_safe, bool fx_enabled, bool manual_accepted, Phase p) noexcept {
  return native_guard_safe && (!fx_enabled || (manual_accepted && p == Phase::Ready));
}
// POST de setStickData: B se reserva para el Shield Attack SOLO si R esta pulsado, la defensa esta en READY y el juego ofrecio el Shield Attack en el
// execute anterior (todas sus condiciones nativas: aprendido, fijacion, escudo, suelo...). En cualquier otro caso B queda intacto (espada normal).
inline bool consume_b(bool b_trig, bool r_held, Phase p, bool bash_offered_prev) noexcept {
  return b_trig && r_held && p == Phase::Ready && bash_offered_prev;
}
// POST de spActionTrigger en el sitio del Shield Attack: R+B (B muestreado antes del consumo) y defensa en READY.
enum class Bash : std::uint8_t { NotPressed = 0, Accepted, DeniedPenalty, DeniedNoR, Count };
inline const char* name(Bash b) noexcept {
  static constexpr const char* n[] = {"NOT_PRESSED", "R_B_ACCEPTED", "DENIED_DURING_PENALTY", "B_WITHOUT_R"};
  return static_cast<unsigned>(b) < static_cast<unsigned>(Bash::Count) ? n[static_cast<unsigned>(b)] : "NOT_PRESSED";
}
inline Bash decide_bash(bool r_held, bool b_trig, Phase p) noexcept {
  if (!b_trig) return Bash::NotPressed;
  if (p != Phase::Ready) return Bash::DeniedPenalty;
  if (!r_held) return Bash::DeniedNoR;
  return Bash::Accepted;
}
// PRE de checkMoveDoAction: veto de la ADMISION de una esquiva voluntaria (A con DoStatus de salto o rodada), nunca de un procInit forzado.
enum class Dodge : std::uint8_t { None = 0, Backflip, Sidehop, SideRollCandidate, FrontRoll, Count };
inline const char* name(Dodge d) noexcept {
  static constexpr const char* n[] = {"NONE", "BACKFLIP_DENIED", "SIDEHOP_DENIED", "SIDE_ROLL_DENIED", "FRONTROLL_DENIED"};
  return static_cast<unsigned>(d) < static_cast<unsigned>(Dodge::Count) ? n[static_cast<unsigned>(d)] : "NONE";
}
// Replica de daAlink_c::getDirectionFromAngle (d_a_alink.cpp:9760): solo para clasificar el registro.
inline bool backward(std::int16_t rel) noexcept { return (rel < 0 ? -static_cast<int>(rel) : static_cast<int>(rel)) > 0x6000; }
inline Dodge classify_dodge(bool a_trig, unsigned do_status, std::int16_t move_minus_shape, bool side_roll_armed) noexcept {
  if (!a_trig) return Dodge::None;
  if (do_status == policy::kStatusFrontRoll) return Dodge::FrontRoll;
  if (do_status != policy::kStatusJump) return Dodge::None;
  if (backward(move_minus_shape)) return Dodge::Backflip;
  return side_roll_armed ? Dodge::SideRollCandidate : Dodge::Sidehop;
}

inline int fmt_penalty(char* out, std::size_t n, const char* family, const Out& o) noexcept {
  return std::snprintf(out, n,
                       "%s event=%s reason=%s state=%s>%s origin=%s extended=%d link_id=%u epoch=%llu entry_tick=%llu tick=%llu active_ticks=%llu "
                       "native_ticks=%llu remaining=%llu total_penalty_ticks=%llu",
                       family, name(o.ev), o.reason, name(o.before), name(o.after), name(o.origin), o.extended ? 1 : 0, o.link_id,
                       static_cast<unsigned long long>(o.epoch), static_cast<unsigned long long>(o.entry_tick), static_cast<unsigned long long>(o.tick),
                       static_cast<unsigned long long>(o.active_ticks), static_cast<unsigned long long>(o.native_ticks),
                       static_cast<unsigned long long>(o.remaining), static_cast<unsigned long long>(policy::kTotalPenaltyTicks));
}
}  // namespace lnkfx
