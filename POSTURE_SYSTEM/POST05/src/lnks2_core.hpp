#pragma once
// LNK SESSION02 0.11.0 (LNK05 + LNK06): ruptura FISICA nativa de la guardia de Link al saturar su postura por un impacto valido, vulnerabilidad nativa y
// recuperacion nativa. Logica PURA (biblioteca estandar y nucleos locales; ninguna cabecera del juego, ninguna direccion, ninguna escritura).
//
// Hechos nativos [S host 40457c6 / B EXE 2.0.3]:
//  * daAlink_c::checkDamageAction (d_a_alink_damage.inc:423) se ejecuta UNA vez por execute de Link (d_a_alink.cpp:18262) y es la funcion desde la que el
//    propio juego entra en la ruptura: `return procGuardBreakInit();` (ataques at_spl 9/10/11) y procGuardSlipInit -> procGuardBreakInit al 4.o bloqueo
//    (field_0x2fd5, d_a_alink_guard.inc:244-246). Su retorno solo se usa para PROC_LARGE_DAMAGE + botas magneticas.
//  * procGuardBreakInit (guard.inc:494): commonProcInit(PROC_GUARD_BREAK) + setSingleAnimeParam(ANM_GUARD_BREAK, mGuardBreakAnm) + mNormalSpeed=0 +
//    vibracion + Z2SE_AL_V_GUARD_BROKEN; devuelve 1. Exportada: ?procGuardBreakInit@daAlink_c@@QEAAHXZ.
//  * procGuardBreak (guard.inc:509): fin de animacion => checkNextAction(0); frame > mCancelFrame => onModeFlg(4) + checkNextAction(1) (cancelacion
//    legitima). mGuardBreakAnm (d_a_alink_HIO_data.inc, datos const de release) = {fin 35, velocidad 1.0, inicio 0, interp 3, cancelacion 28}.
// El mod NO sustituye la maquina de Link: SOLICITA la ruptura por la MISMA ruta nativa (procGuardBreakInit dentro de checkDamageAction, en el execute de
// Link), CONFIRMA la entrada (mProcID == PROC_GUARD_BREAK tras la llamada) y OBSERVA la salida nativa (mProcID distinto en un mod_update posterior).
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "lnk_core.hpp"

namespace lnk05 {
inline constexpr const char* kVersion = "0.11.0";

namespace policy {
inline constexpr int kProcGuardSlip = 0x1d, kProcGuardAttack = 0x1e, kProcGuardBreak = 0x1f;
// Origenes desde los que la transicion es segura: la reaccion de bloqueo (GUARD_SLIP, que el juego ya escala a GUARD_BREAK en su 4.o bloqueo) y los
// estados LIBRES en suelo (espera, movimiento, fijado, giro, paso, agachado). Nunca: rodar, saltar, caer, atacar, Shield Attack, cortes, dano, caballo,
// nado, escalada, cargar, eventos.
inline constexpr int kAllowedOrigins[] = {0x01 /*SERVICE_WAIT*/, 0x02 /*TIRED_WAIT*/, 0x03 /*WAIT*/,     0x04 /*MOVE*/,     0x05 /*ATN_MOVE*/,
                                          0x06 /*ATN_ACTOR_WAIT*/, 0x07 /*ATN_ACTOR_MOVE*/, 0x08 /*WAIT_TURN*/, 0x09 /*MOVE_TURN*/,
                                          0x1b /*STEP_MOVE*/,    0x1c /*CROUCH*/,    0x1d /*GUARD_SLIP*/};
// mModeFlg: la rama nativa de ruptura sin escudo exige !checkModeFlg(0x70C52) (nado, caballo, colgado...) y el bloqueo exige !checkModeFlg(0x4000000).
inline constexpr std::uint32_t kModeBlockedMask = 0x70C52u, kModeSpecialMask = 0x4000000u;
inline constexpr float kCancelFrame = 28.0f, kEndFrame = 35.0f;  // mGuardBreakAnm (datos nativos)
inline constexpr float kFrameSlack = 1.5f;                       // una observacion por tick: el ultimo frame visto puede ir 1 por detras
inline constexpr std::uint64_t kBlockTriggerTicks = 3;   // un bloqueo dispara en el mismo execute de Link o en el siguiente
inline constexpr std::uint64_t kDirectTriggerTicks = 24; // un golpe directo espera a que termine la reaccion nativa de dano (ventana acotada)
inline constexpr std::uint64_t kStuckTicks = 150;        // solo anomalia registrada: el mod nunca fuerza la salida
}  // namespace policy

inline bool allowed_origin(int proc) noexcept {
  for (int p : policy::kAllowedOrigins)
    if (p == proc) return true;
  return false;
}

enum class Kind : std::uint8_t { Block = 0, Direct, NativeAdopted, Count };
inline const char* name(Kind k) noexcept {
  static constexpr const char* n[] = {"BLOCK", "DIRECT", "NATIVE_ADOPTED"};
  return static_cast<unsigned>(k) < 3 ? n[static_cast<unsigned>(k)] : "NONE";
}
enum class State : std::uint8_t { Ready = 0, TriggerPending, NativeActive };
inline const char* name(State s) noexcept {
  static constexpr const char* n[] = {"READY", "TRIGGER_PENDING", "NATIVE_BREAK_ACTIVE"};
  return static_cast<unsigned>(s) < 3 ? n[static_cast<unsigned>(s)] : "READY";
}
enum class Gate : std::uint8_t {
  Ok = 0, NoPending, Expired, LinkMismatch, NotHuman, Dead, SimInactive, EventRunning, AlreadyBreaking, ProcNotAllowed, ModeFlags, DamageThisCall, Count
};
inline const char* name(Gate g) noexcept {
  static constexpr const char* n[] = {"OK",          "NO_PENDING",   "EXPIRED",          "LINK_MISMATCH",   "NOT_HUMAN",  "DEAD",
                                      "SIM_INACTIVE", "EVENT_RUNNING", "ALREADY_BREAKING", "PROC_NOT_ALLOWED", "MODE_FLAGS", "DAMAGE_THIS_CALL"};
  return static_cast<unsigned>(g) < static_cast<unsigned>(Gate::Count) ? n[static_cast<unsigned>(g)] : "OK";
}
enum class Ev : std::uint8_t {
  None = 0,
  Requested,          // TRIGGER_PENDING
  RequestRejected,    // saturacion sin ruptura (activa ya, Link distinta...)
  Entered,            // procGuardBreakInit llamado y mProcID == PROC_GUARD_BREAK verificado
  TriggerFailed,      // la llamada no dejo a Link en PROC_GUARD_BREAK: postura intacta, nada mas
  Expired,            // ninguna ventana segura dentro del plazo del episodio: postura intacta (100)
  CancelledByParry,   // un parry confirmado mientras habia una solicitud pendiente: proteccion del parry
  NativeAdopted,      // el juego entro solo en GUARD_BREAK con la postura saturada o pendiente: UNA ruptura, la nativa
  NativeObserved,     // ruptura nativa sin saturacion: solo se cuenta
  RecoveredCancel,    // salida nativa tras el frame de cancelacion (checkNextAction(1))
  RecoveredEnd,       // salida nativa al fin de la animacion (checkNextAction(0))
  Interrupted,        // salida antes de la cancelacion por un procedimiento nativo prioritario (dano, evento...)
  Aborted,            // Link perdida, muerta o distinta: limpieza sin fabricar recuperacion
  Stuck,              // anomalia: GUARD_BREAK mas alla de kStuckTicks (solo registro)
  Count
};
inline const char* name(Ev e) noexcept {
  static constexpr const char* n[] = {"NONE",         "REQUESTED",       "REQUEST_REJECTED", "NATIVE_BREAK_ENTERED", "TRIGGER_FAILED",
                                      "EXPIRED",      "CANCELLED_BY_PARRY", "NATIVE_BREAK_ADOPTED", "NATIVE_BREAK_OBSERVED", "RECOVERED_NATIVE_CANCEL",
                                      "RECOVERED_NATIVE_END", "INTERRUPTED_NATIVE_PRIORITY", "ABORTED_SAFE", "STUCK_ANOMALY"};
  return static_cast<unsigned>(e) < static_cast<unsigned>(Ev::Count) ? n[static_cast<unsigned>(e)] : "NONE";
}

// Identidad del episodio: Link + epoch + impacto que saturo (atacante/generacion/token) + tick + postura antes/despues.
struct Episode {
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0, tick = 0, deadline = 0;
  Kind kind = Kind::Block;
  std::uint32_t attacker = 0;
  std::uint64_t generation = 0, token = 0;
  int before = 0, after = 0;
};
// Lo que el adaptador lee de la Link viva en el POST de checkDamageAction (por valor).
struct GateIn {
  bool valid = false, human = false, alive = false, sim_active = false, event = false, damage_this_call = false;
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0, tick = 0;
  int proc = -1;
  std::uint32_t mode_flags = 0;
};
// Lo que el adaptador lee de la Link viva UNA vez por mod_update (por valor).
struct ObsIn {
  bool valid = false, alive = false, sim_active = false, event = false;
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0, tick = 0;
  int proc = -1;
  float frame = 0;
};
struct Out {
  Ev ev = Ev::None;
  Gate gate = Gate::Ok;
  Episode ep;
  State before = State::Ready, after = State::Ready;
  int proc = -1, next_proc = -1;
  float last_frame = 0;
  std::uint64_t active_ticks = 0;
  bool by_mod = false, reset_posture = false;
};
struct Counters {
  std::uint64_t saturating_impacts = 0, saturations_block = 0, saturations_direct = 0, requested = 0, request_rejected = 0, entered = 0, trigger_failed = 0,
                expired = 0, cancelled_by_parry = 0, native_adopted = 0, native_observed = 0, recovered_cancel = 0, recovered_end = 0, interrupted = 0,
                aborted = 0, stuck = 0, resets = 0, gate_checks = 0, gate_refused[static_cast<unsigned>(Gate::Count)] = {}, duplicate_requests = 0,
                active_ticks = 0, paused_active_ticks = 0;
};

class Machine {
 public:
  Counters c{};

  // Un impacto VALIDO (bloqueo ordinario confirmado o golpe directo confirmado) deja la postura en el maximo => solicitud ligada a ESTE episodio.
  // Nunca por regeneracion, nunca por un parry (no llega aqui), nunca sin impacto. Ruptura activa o pendiente => la solicitud se rechaza (sin cola).
  Out request(const Episode& e) noexcept {
    Out o;
    o.before = s_;
    o.ep = e;
    ++c.saturating_impacts;
    ++(e.kind == Kind::Direct ? c.saturations_direct : c.saturations_block);
    if (s_ != State::Ready || e.link_id == 0) {
      ++c.request_rejected;
      if (s_ != State::Ready) ++c.duplicate_requests;
      o.ev = Ev::RequestRejected;
      o.after = s_;
      return o;
    }
    ep_ = e;
    ep_.deadline = e.tick + (e.kind == Kind::Direct ? policy::kDirectTriggerTicks : policy::kBlockTriggerTicks);
    s_ = State::TriggerPending;
    ++c.requested;
    o.ev = Ev::Requested;
    o.ep = ep_;
    o.after = s_;
    return o;
  }

  // Puerta PURA: ?puede el adaptador llamar a procGuardBreakInit AHORA (POST de checkDamageAction, dentro del execute de Link)?
  Gate gate(const GateIn& g) const noexcept {
    if (s_ != State::TriggerPending) return Gate::NoPending;
    if (g.tick > ep_.deadline) return Gate::Expired;
    if (!g.valid || g.link_id != ep_.link_id || g.epoch != ep_.epoch) return Gate::LinkMismatch;
    if (!g.human) return Gate::NotHuman;
    if (!g.alive) return Gate::Dead;
    if (!g.sim_active) return Gate::SimInactive;
    if (g.event) return Gate::EventRunning;
    if (g.proc == policy::kProcGuardBreak) return Gate::AlreadyBreaking;  // el juego ya rompio: se adopta en observe(), jamas se reinicia
    if (!allowed_origin(g.proc)) return Gate::ProcNotAllowed;
    if ((g.mode_flags & policy::kModeBlockedMask) != 0 || (g.mode_flags & policy::kModeSpecialMask) != 0) return Gate::ModeFlags;
    if (g.damage_this_call) return Gate::DamageThisCall;  // la reaccion nativa de dano de ESTE execute tiene prioridad
    return Gate::Ok;
  }
  Gate check(const GateIn& g) noexcept {
    const Gate r = gate(g);
    if (s_ == State::TriggerPending) {
      ++c.gate_checks;
      if (r != Gate::Ok) ++c.gate_refused[static_cast<unsigned>(r)];
    }
    return r;
  }

  // Resultado de la UNICA llamada nativa: entered = mProcID == PROC_GUARD_BREAK inmediatamente despues.
  Out triggered(bool entered, int proc_after, std::uint64_t tick) noexcept {
    Out o;
    o.before = s_;
    o.ep = ep_;
    o.proc = proc_after;
    if (s_ != State::TriggerPending) {
      o.after = s_;
      return o;
    }
    if (!entered) {
      ++c.trigger_failed;
      o.ev = Ev::TriggerFailed;
      s_ = State::Ready;
      o.after = s_;
      return o;
    }
    ++c.entered;
    s_ = State::NativeActive;
    by_mod_ = true;
    entered_tick_ = tick;
    active_ticks_ = 0;
    last_frame_ = 0;
    prev_proc_ = policy::kProcGuardBreak;
    o.ev = Ev::Entered;
    o.by_mod = true;
    o.after = s_;
    return o;
  }

  // Proteccion del parry: un parry confirmado cancela cualquier solicitud pendiente (nunca una ruptura ya iniciada por el juego).
  Out parry(std::uint64_t tick) noexcept {
    Out o;
    o.before = s_;
    o.ep = ep_;
    (void)tick;
    if (s_ == State::TriggerPending) {
      ++c.cancelled_by_parry;
      s_ = State::Ready;
      o.ev = Ev::CancelledByParry;
    }
    o.after = s_;
    return o;
  }

  // UNA observacion por mod_update: caducidad, adopcion de la ruptura nativa, seguimiento del frame y salida nativa.
  Out observe(const ObsIn& in, bool posture_saturated) noexcept {
    Out o;
    o.before = s_;
    o.ep = ep_;
    o.proc = in.proc;
    const bool same_link = in.valid && (s_ == State::Ready || (in.link_id == ep_.link_id && in.epoch == ep_.epoch));
    if (s_ != State::Ready && !same_link) {  // Link desaparecida o distinta: limpieza; la identidad de la postura ya la reinicia
      ++c.aborted;
      o.ev = Ev::Aborted;
      o.last_frame = last_frame_;
      s_ = State::Ready;
      prev_proc_ = in.valid ? in.proc : -1;
      o.after = s_;
      return o;
    }
    if (s_ == State::TriggerPending && in.tick > ep_.deadline) {
      ++c.expired;
      o.ev = Ev::Expired;
      s_ = State::Ready;
    } else if (s_ != State::NativeActive && in.valid && in.proc == policy::kProcGuardBreak && prev_proc_ != policy::kProcGuardBreak) {
      if (s_ == State::TriggerPending || posture_saturated) {  // UNA ruptura: la del juego, sincronizada con la postura
        ++c.native_adopted;
        if (s_ != State::TriggerPending) ep_ = Episode{in.link_id, in.epoch, in.tick, in.tick, Kind::NativeAdopted, 0, 0, 0, 0, 0};
        else ep_.kind = Kind::NativeAdopted;
        o.ev = Ev::NativeAdopted;
        s_ = State::NativeActive;
        by_mod_ = false;
        entered_tick_ = in.tick;
        active_ticks_ = 0;
        last_frame_ = in.frame;
        o.ep = ep_;
      } else {
        ++c.native_observed;
        o.ev = Ev::NativeObserved;
      }
    } else if (s_ == State::NativeActive) {
      if (in.proc == policy::kProcGuardBreak) {
        if (in.sim_active) ++active_ticks_, ++c.active_ticks;
        else ++c.paused_active_ticks;
        last_frame_ = in.frame;
        if (active_ticks_ == policy::kStuckTicks) ++c.stuck, o.ev = Ev::Stuck;
      } else {
        o.next_proc = in.proc;
        o.last_frame = last_frame_;
        o.active_ticks = active_ticks_;
        o.by_mod = by_mod_;
        if (!in.alive) {
          ++c.aborted;
          o.ev = Ev::Aborted;
        } else if (last_frame_ >= policy::kEndFrame - policy::kFrameSlack) {
          ++c.recovered_end;
          o.ev = Ev::RecoveredEnd;
          o.reset_posture = true;
        } else if (last_frame_ >= policy::kCancelFrame - policy::kFrameSlack) {
          ++c.recovered_cancel;
          o.ev = Ev::RecoveredCancel;
          o.reset_posture = true;
        } else {
          ++c.interrupted;  // el ciclo de ruptura termino (Link fue alcanzada, evento...): tambien se restablece, sin fingir una recuperacion
          o.ev = Ev::Interrupted;
          o.reset_posture = true;
        }
        if (o.reset_posture) ++c.resets;
        s_ = State::Ready;
      }
    }
    prev_proc_ = in.valid ? in.proc : -1;
    o.after = s_;
    o.active_ticks = o.active_ticks ? o.active_ticks : active_ticks_;
    return o;
  }

  // Regeneracion ordinaria suspendida mientras hay una solicitud o una ruptura activa (la barra no pierde la saturacion a mitad de la animacion).
  bool hold() const noexcept { return s_ != State::Ready; }
  State state() const noexcept { return s_; }
  const Episode& episode() const noexcept { return ep_; }
  float last_frame() const noexcept { return last_frame_; }

 private:
  State s_ = State::Ready;
  Episode ep_{};
  bool by_mod_ = false;
  int prev_proc_ = -1;
  float last_frame_ = 0;
  std::uint64_t entered_tick_ = 0, active_ticks_ = 0;
};

inline int fmt_event(char* out, std::size_t n, const char* family, const Out& o, int posture_milli) noexcept {
  return std::snprintf(out, n,
                       "%s event=%s state=%s>%s kind=%s link_id=%u epoch=%llu tick=%llu attacker_id=%u generation=%llu attack_token=%llu posture_before=%d.%03d "
                       "posture_after=%d.%03d posture_now=%d.%03d gate=%s proc=%d next_proc=%d last_frame=%.2f cancel_frame=%.0f end_frame=%.0f active_ticks=%llu "
                       "by_mod=%d reset=%d",
                       family, name(o.ev), name(o.before), name(o.after), name(o.ep.kind), o.ep.link_id, static_cast<unsigned long long>(o.ep.epoch),
                       static_cast<unsigned long long>(o.ep.tick), o.ep.attacker, static_cast<unsigned long long>(o.ep.generation),
                       static_cast<unsigned long long>(o.ep.token), o.ep.before / 1000, o.ep.before % 1000, o.ep.after / 1000, o.ep.after % 1000,
                       posture_milli / 1000, posture_milli % 1000, name(o.gate), o.proc, o.next_proc, static_cast<double>(o.last_frame),
                       static_cast<double>(policy::kCancelFrame), static_cast<double>(policy::kEndFrame), static_cast<unsigned long long>(o.active_ticks),
                       o.by_mod ? 1 : 0, o.reset_posture ? 1 : 0);
}
}  // namespace lnk05
