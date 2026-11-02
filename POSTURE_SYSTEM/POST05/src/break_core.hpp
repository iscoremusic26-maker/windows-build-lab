#pragma once
// Enemy Posture System, POST06 (0.6.0): RUPTURA real -> CASTIGO (x1.20 fijo) -> caida NATIVA -> suelo (Finish NATIVO) -> levantamiento NATIVO -> rearme.
// Nucleo PURO (sin punteros ni juego): recibe por valor lo que mod.cpp leyo en el PRE/POST de UNA llamada a daE_OC_c::damage_check y devuelve UNA accion.
// mod.cpp ejecuta la accion con su unico escritor (recheck + verify) y este nucleo cuenta el resultado.
//
// Reloj: ejecuciones PROPIAS del actor (damage_check se llama exactamente una vez por daE_OC_c::action(), d_a_e_oc.cpp:2415-2418): 30 = 1 s a 30 Hz de
// simulacion. No avanza en pausa/menus/cambio de sala (el actor no ejecuta). 1 s sin espadazo -> caida; 3 s absolutos desde la ruptura -> caida.
//
// Cadena nativa reutilizada (executeBigDamage, d_a_e_oc.cpp:1603-1724): BIG_DAMAGE 0..2 (offTgSph, impulso, voz) -> 3 (vuelo) -> 4 (aterrizaje; si
// health < 1 -> DEATH) -> 5 (onDownFlg + setStabPos + timer 35..55; Finish de Link = CUT_TYPE_DOWN, d_a_e_oc.cpp damage_check checkCutDownHitFlg -> DEATH)
// -> 6 (offDownFlg + animacion de levantarse) -> setActionMode(FIND, 5) = control normal. El rearme (postura 0) ocurre SOLO al salir de BIG_DAMAGE vivo.
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "reaction_core.hpp"

namespace post06 {

inline constexpr const char* kBreakVersion = "0.6.0";

namespace policy {
inline constexpr std::uint64_t kInactivityExec = 30;   // 1 s sin espadazo valido -> caida
inline constexpr std::uint64_t kPunishMaxExec = 90;    // 3 s absolutos desde la ruptura (techo; los espadazos NO lo extienden)
inline constexpr std::uint64_t kDropSafetyExec = 60;   // si la caida vencida no encuentra un punto seguro en 2 s mas: se aborta el ciclo (rearme, sin caida)
inline constexpr int kBonusDen = 5;                    // x1.20 = +1/5 del dano efectivo observado, con acumulador de fracciones por actor
inline constexpr int kModeWait = 0, kModeWalk = 1, kModeFind = 3, kModeAttack = 4, kModeDamage = 5, kModeBig = 6, kModeWatch = 7, kModeSound = 8,
                     kModeDeath = 9, kModeWaterDeath = 10, kModeFallDead = 13, kModeStay = 15, kModeMoveOut = 16;
inline constexpr int kBigDown = 5, kBigGettingUp = 6;
// Modos desde los que el nativo tambien puede entrar en BIG_DAMAGE por un golpe (damage_check corre al inicio de action() en cualquiera de ellos).
inline constexpr bool drop_mode_safe(int m) noexcept {
  return m == kModeWait || m == kModeWalk || m == kModeFind || m == kModeAttack || m == kModeDamage || m == kModeWatch || m == kModeSound || m == kModeStay ||
         m == kModeMoveOut;
}
inline constexpr bool death_mode(int m) noexcept { return m == kModeDeath || m == kModeWaterDeath || m == kModeFallDead; }
// Bandas de recuperacion de postura por vida restante (porcentaje sobre el maximo observado de ESTA generacion).
inline constexpr std::uint8_t regen_rate_for(int hp, int hp_max) noexcept {
  if (hp_max <= 0 || hp <= 0 || hp > hp_max) return 5;  // maximo desconocido o incoherente: tasa base de POST04 (fallback seguro)
  const long long pct100 = static_cast<long long>(hp) * 100;  // comparacion exacta sin redondeo: hp/max > X%  <=> hp*100 > X*max
  if (pct100 > 75LL * hp_max) return 5;
  if (pct100 > 50LL * hp_max) return 4;
  if (pct100 > 25LL * hp_max) return 3;
  if (pct100 > 10LL * hp_max) return 2;
  return 1;
}
}  // namespace policy

enum class Phase : std::uint8_t { Tracking = 0, Punish, Falling, Down, GettingUp, Dead, Count };
inline constexpr unsigned kPhaseCount = static_cast<unsigned>(Phase::Count);
inline const char* name(Phase p) noexcept {
  static constexpr const char* k[kPhaseCount] = {"TRACKING", "BROKEN_PUNISH", "FALLING", "DOWN_FINISH", "GETTING_UP", "DEAD"};
  const unsigned i = static_cast<unsigned>(p);
  return i < kPhaseCount ? k[i] : "NONE";
}
enum class Act : std::uint8_t { None = 0, Bonus, Drop, Rearm };
enum class Why : std::uint8_t { None = 0, BreakStanding, BreakNativeKnockdown, PunishHitNativeKnockdown, Inactivity1s, Max3s, DropAbortedUnsafe, Landed,
                                GettingUp, RearmAfterGetUp, RearmLeftBigWithoutDown, Death, MissedCrossingLatched, Count };
inline const char* name(Why w) noexcept {
  switch (w) {
    case Why::BreakStanding: return "BREAK_STANDING_PUNISH";
    case Why::BreakNativeKnockdown: return "BREAK_NATIVE_KNOCKDOWN";
    case Why::PunishHitNativeKnockdown: return "PUNISH_HIT_NATIVE_KNOCKDOWN";
    case Why::Inactivity1s: return "DROP_INACTIVITY_1S";
    case Why::Max3s: return "DROP_MAX_3S";
    case Why::DropAbortedUnsafe: return "DROP_ABORTED_NO_SAFE_POINT_REARM";
    case Why::Landed: return "LANDED_DOWN";
    case Why::GettingUp: return "GETTING_UP";
    case Why::RearmAfterGetUp: return "REARM_AFTER_NATIVE_GET_UP";
    case Why::RearmLeftBigWithoutDown: return "REARM_LEFT_KNOCKDOWN_ABNORMALLY";
    case Why::Death: return "DEATH";
    case Why::MissedCrossingLatched: return "LATCH_SEEN_WITHOUT_CROSSING_PUNISH";
    default: return "NONE";
  }
}

// Lo que mod.cpp leyo en ESTA llamada (por valor).
struct View {
  int slot = -1;
  std::uint64_t generation = 0;
  std::uint32_t id = 0;
  bool tracked = false;          // candidato vivo con slot/generacion validos en el ledger y entrada de postura enlazada
  int pre_hp = 0, native_hp = 0; // vida PRE y vida tras damage_check (antes de cualquier escritura de POST06)
  int mode = 0, state = 0;       // modo/estado FINALES de la llamada (tras la decision de POST05)
  bool hit = false;              // esta llamada proceso un HIT_CONFIRMED (informe POST03)
  bool sword = false;            // ... de la familia ESPADA verificada (collider procesado, tipo AT leido)
  bool crossed = false;          // POST04: este golpe llevo la postura a 100
  bool latched = false;          // POST04: la entrada esta enganchada en 100
  bool writer_ok = true;         // layout/sesion permiten escribir
};
// Accion para mod.cpp. Bonus: health := new_hp (y, si lethal, rama letal nativa: attention flags 0 y BIG_DAMAGE 0 si la reaccion era DAMAGE).
struct Action {
  Act act = Act::None;
  Why why = Why::None;
  int base = 0, extra = 0, new_hp = 0;
  bool lethal = false, convert_to_big = false;
};

struct BCounters {
  std::uint64_t calls = 0, breaks = 0, breaks_knockdown = 0, punish_sword_hits = 0, bonus_applied = 0, bonus_zero = 0, bonus_units = 0, base_units = 0,
                bonus_lethal = 0, drops_inactivity = 0, drops_max = 0, drops_aborted = 0, punish_native_knockdowns = 0, downs = 0, getups = 0, rearms = 0,
                rearms_abnormal = 0, deaths_in_cycle = 0, write_failed = 0, slot_resets = 0, log_written = 0, log_dropped = 0;
  std::uint64_t phase_calls[kPhaseCount] = {};
};

class Breaker {
 public:
  BCounters c{};
  bool enabled = false;  // mod.cpp lo activa UNA vez; los laboratorios heredados no lo usan

  struct Slot {
    bool bound = false;
    std::uint32_t id = 0;
    std::uint64_t generation = 0;
    Phase phase = Phase::Tracking;
    std::uint64_t exec = 0, punish_start = 0, last_sword = 0, due_since = 0;
    bool due = false;
    int hp_max = 0;
    int frac = 0;  // acumulador de quintos (0..4) del castigo actual
  };
  const Slot& slot(unsigned i) const noexcept { return s_[i < post03::kCapacity ? i : 0]; }

  // UNA llamada de damage_check de un actor seguido. Devuelve la accion a ejecutar (como mucho una).
  Action step(const View& v) noexcept {
    Action a;
    if (!enabled || !v.tracked || v.slot < 0 || static_cast<unsigned>(v.slot) >= post03::kCapacity) return a;
    ++c.calls;
    Slot& s = s_[v.slot];
    if (!s.bound || s.generation != v.generation || s.id != v.id) {  // generacion nueva o slot reciclado: nada se hereda
      if (s.bound) ++c.slot_resets;
      s = Slot{};
      s.bound = true;
      s.id = v.id;
      s.generation = v.generation;
    }
    ++s.exec;
    ++c.phase_calls[static_cast<unsigned>(s.phase)];
    if (v.pre_hp > s.hp_max) s.hp_max = v.pre_hp;  // maximo observado de ESTA generacion (Expansion/HM escriben la vida antes del primer contacto)
    if (v.native_hp > s.hp_max) s.hp_max = v.native_hp;
    const bool dead_now = policy::death_mode(v.mode) || (v.native_hp <= 0 && v.mode != policy::kModeBig);
    switch (s.phase) {
      case Phase::Tracking:
        if (dead_now) { s.phase = Phase::Dead; break; }
        if (v.hit && v.crossed) {  // la ruptura: UNA vez por ciclo (POST04 la marca una sola vez gracias al latch). Sin bonus para este golpe.
          ++c.breaks;
          if (v.mode == policy::kModeBig) { ++c.breaks_knockdown; enter_falling(s); a.why = Why::BreakNativeKnockdown; }
          else { s.phase = Phase::Punish; s.punish_start = s.last_sword = s.exec; s.frac = 0; s.due = false; a.why = Why::BreakStanding; }
        } else if (v.latched) {  // latch visto sin el cruce (p. ej. carga a mitad de ciclo): castigo desde ahora, nunca enganchado para siempre
          s.phase = Phase::Punish; s.punish_start = s.last_sword = s.exec; s.frac = 0; s.due = false; a.why = Why::MissedCrossingLatched;
        }
        break;
      case Phase::Punish:
        if (dead_now) { ++c.deaths_in_cycle; s.phase = Phase::Dead; a.why = Why::Death; break; }
        if (v.hit && v.sword) {
          ++c.punish_sword_hits;
          s.last_sword = s.exec;  // renueva SOLO la inactividad; el techo de 3 s (punish_start) no cambia
          const int base = v.pre_hp - v.native_hp;
          if (base > 0 && v.native_hp > 1) {  // golpe natively NO letal con dano real: +20 % (con acumulador). Si ya era letal, el nativo manda.
            s.frac += base;
            const int extra = s.frac / policy::kBonusDen;
            s.frac %= policy::kBonusDen;
            c.base_units += static_cast<std::uint64_t>(base);
            if (extra > 0) {
              a.act = Act::Bonus;
              a.base = base;
              a.extra = extra;
              a.new_hp = v.native_hp - extra < 0 ? 0 : v.native_hp - extra;
              a.lethal = a.new_hp <= 1;  // regla nativa de damage_check: health <= 1 => rama letal
              a.convert_to_big = a.lethal && v.mode == policy::kModeDamage;
            } else {
              ++c.bonus_zero;
            }
          }
          if (v.mode == policy::kModeBig) {  // espadazo que derriba (giro, 4.o corte, letal...): caida NATIVA ya en marcha
            ++c.punish_native_knockdowns;
            enter_falling(s);
            if (a.act == Act::None) a.why = Why::PunishHitNativeKnockdown;
          } else if (a.convert_to_big) {
            a.why = Why::PunishHitNativeKnockdown;  // la caida empieza en commit() SOLO si la rama letal se escribio y verifico
          }
          break;
        }
        if (v.mode == policy::kModeBig) { ++c.punish_native_knockdowns; enter_falling(s); a.why = Why::PunishHitNativeKnockdown; break; }
        if (v.hit) break;  // otra familia en esta llamada: no renueva ni derriba en el mismo tick
        {
          const bool inactive = s.exec - s.last_sword >= policy::kInactivityExec;
          const bool maxed = s.exec - s.punish_start >= policy::kPunishMaxExec;
          if (!inactive && !maxed) break;
          if (!s.due) { s.due = true; s.due_since = s.exec; }
          if (s.exec - s.due_since >= policy::kDropSafetyExec) {  // limite de seguridad (modo sin punto seguro o escritura que no verifica): ciclo abortado
            a.act = Act::Rearm;
            a.why = Why::DropAbortedUnsafe;
            return a;
          }
          if (v.writer_ok && policy::drop_mode_safe(v.mode) && v.native_hp >= 1) {
            a.act = Act::Drop;
            a.why = maxed ? Why::Max3s : Why::Inactivity1s;
            return a;  // la transicion a FALLING se confirma en commit() tras escribir y verificar
          }
        }
        break;
      case Phase::Falling:
        if (dead_now || (v.mode == policy::kModeBig && v.native_hp < 1 && v.state >= policy::kBigDown)) { s.phase = Phase::Dead; ++c.deaths_in_cycle; a.why = Why::Death; break; }
        if (v.mode == policy::kModeBig) {
          if (v.state == policy::kBigDown) { s.phase = Phase::Down; ++c.downs; a.why = Why::Landed; }
          else if (v.state == policy::kBigGettingUp) { s.phase = Phase::GettingUp; ++c.getups; a.why = Why::GettingUp; }
          break;
        }
        a.act = Act::Rearm; a.why = Why::RearmLeftBigWithoutDown;  // salio de BIG_DAMAGE vivo sin pasar por el suelo (guion, etc.)
        return a;
      case Phase::Down:
        if (dead_now) { s.phase = Phase::Dead; ++c.deaths_in_cycle; a.why = Why::Death; break; }   // Finish nativo (o muerte)
        if (v.mode == policy::kModeBig) {
          if (v.state == policy::kBigGettingUp) { s.phase = Phase::GettingUp; ++c.getups; a.why = Why::GettingUp; }
          break;
        }
        a.act = Act::Rearm; a.why = Why::RearmLeftBigWithoutDown;
        return a;
      case Phase::GettingUp:
        if (dead_now) { s.phase = Phase::Dead; ++c.deaths_in_cycle; a.why = Why::Death; break; }
        if (v.mode == policy::kModeBig) break;  // aun levantandose: NO se rearma antes de tiempo
        a.act = Act::Rearm; a.why = v.mode == policy::kModeFind ? Why::RearmAfterGetUp : Why::RearmLeftBigWithoutDown;  // nativo: setActionMode(FIND, 5)
        return a;
      case Phase::Dead:
      default:
        break;
    }
    return a;
  }
  // mod.cpp confirma el resultado de la escritura/rearme (solo entonces cambia la fase).
  void commit(const View& v, const Action& a, bool ok) noexcept {
    if (v.slot < 0 || static_cast<unsigned>(v.slot) >= post03::kCapacity) return;
    Slot& s = s_[v.slot];
    if (!s.bound || s.generation != v.generation || s.id != v.id) return;
    if (!ok) { ++c.write_failed; return; }  // nada cambio en el juego: se reintenta en la llamada siguiente (o se aborta por el limite de seguridad)
    switch (a.act) {
      case Act::Bonus:
        ++c.bonus_applied;
        c.bonus_units += static_cast<std::uint64_t>(a.extra);
        if (a.lethal) ++c.bonus_lethal;
        if (a.convert_to_big) { ++c.punish_native_knockdowns; enter_falling(s); }
        break;
      case Act::Drop:
        if (a.why == Why::Max3s) ++c.drops_max; else ++c.drops_inactivity;
        enter_falling(s);
        break;
      case Act::Rearm:
        if (a.why == Why::DropAbortedUnsafe) ++c.drops_aborted;
        else if (a.why == Why::RearmAfterGetUp) ++c.rearms;
        else ++c.rearms_abnormal;
        s.phase = Phase::Tracking;
        s.frac = 0;
        s.due = false;
        break;
      default:
        break;
    }
  }
  std::uint8_t regen_rate(int slot_i, int hp) const noexcept {
    if (slot_i < 0 || static_cast<unsigned>(slot_i) >= post03::kCapacity) return 5;
    return policy::regen_rate_for(hp, s_[slot_i].hp_max);
  }
  bool log_take() noexcept {
    if (logged_ >= kLogCap) { ++c.log_dropped; return false; }
    ++logged_;
    ++c.log_written;
    return true;
  }
  static constexpr unsigned kLogCap = 300;

 private:
  Slot s_[post03::kCapacity]{};
  unsigned logged_ = 0;
  static void enter_falling(Slot& s) noexcept {
    s.phase = Phase::Falling;
    s.frac = 0;
    s.due = false;
  }
};

// ---- formato (sin punteros ni rutas) ----
inline int fmt_event(char* out, std::size_t n, const View& v, const Breaker::Slot& s, const Action& a, Phase before, bool ok) noexcept {
  return std::snprintf(out, n,
                       "POST06_EVENT id=%u generation=%llu phase=%s>%s why=%s act=%s ok=%d exec_since_break=%llu exec_since_sword=%llu mode=%d state=%d "
                       "hp=%d>%d base=%d bonus=%d hp_max=%d",
                       v.id, static_cast<unsigned long long>(v.generation), name(before), name(s.phase), name(a.why),
                       a.act == Act::Bonus ? "BONUS_X1_20" : a.act == Act::Drop ? "NATIVE_KNOCKDOWN_START" : a.act == Act::Rearm ? "REARM_POSTURE_0" : "NONE",
                       ok ? 1 : 0, static_cast<unsigned long long>(s.exec - s.punish_start), static_cast<unsigned long long>(s.exec - s.last_sword), v.mode,
                       v.state, v.pre_hp, a.act == Act::Bonus ? a.new_hp : v.native_hp, a.base, a.extra, s.hp_max);
}
inline int fmt_summary(char* out, std::size_t n, const Breaker& b, const char* tag) noexcept {
  const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
  const BCounters& c = b.c;
  return std::snprintf(out, n,
                       "POST06_SUMMARY v=%s tag=%s enabled=%d calls=%llu breaks=%llu(knockdown=%llu) punish_sword_hits=%llu bonus(applied/zero/units/base/lethal)="
                       "%llu/%llu/%llu/%llu/%llu drops(inactivity/max/aborted)=%llu/%llu/%llu punish_native_knockdowns=%llu downs=%llu getups=%llu "
                       "rearms=%llu rearms_abnormal=%llu deaths_in_cycle=%llu write_failed=%llu slot_resets=%llu log(written/dropped)=%llu/%llu",
                       kBreakVersion, tag, b.enabled ? 1 : 0, u(c.calls), u(c.breaks), u(c.breaks_knockdown), u(c.punish_sword_hits), u(c.bonus_applied),
                       u(c.bonus_zero), u(c.bonus_units), u(c.base_units), u(c.bonus_lethal), u(c.drops_inactivity), u(c.drops_max), u(c.drops_aborted),
                       u(c.punish_native_knockdowns), u(c.downs), u(c.getups), u(c.rearms), u(c.rearms_abnormal), u(c.deaths_in_cycle), u(c.write_failed),
                       u(c.slot_resets), u(c.log_written), u(c.log_dropped));
}

}  // namespace post06
