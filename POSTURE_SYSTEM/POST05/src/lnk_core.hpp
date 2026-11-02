#pragma once
// LNK SESSION01 0.10.1 (LNK01 + LNK02 + LNK03 + LNK04 + LNK07): postura PROPIA de Link (Sekiro-style) + barra inferior + supresion VISUAL de los prompts
// de accion inferiores. Logica PURA (biblioteca estandar y nucleos locales; ninguna cabecera del juego ni del SDK, ninguna direccion desreferenciada).
//
// UN UNICO contador de postura de Link, alimentado por:
//   CONFIRMED_BLOCK      (LNK01/LNK07): veredicto POST08 NORMAL_GUARD / MISSED_EARLY / LATCHED / FURY_CHIP_POSSIBLE de un contacto FRESCO (ArmTrack)
//                         con AtShieldHit nativo => peso de bloqueo de la variante.
//   CONFIRMED_DIRECT_HIT (LNK02/LNK07): veredicto POST08 NOT_BLOCKED de un contacto FRESCO, ATRIBUIDO (el primer cilindro Tg de Link con impacto
//                         registro exactamente la esfera de este atacante: es el cilindro del que daAlink_c::checkDamageAction toma el dano) => PENDIENTE;
//                         se CONFIRMA en el mod_update siguiente solo si el dano nativo ocurrio: flanco de subida de mDamageTimer (setDamagePoint) y
//                         perdida real de vida (vida bajo la referencia o resta pendiente ItemLifeCount < 0). Sin flanco/vida o con ambiguedad => nada.
//   CONFIRMED_PARRY      (LNK03): POST08 CONFIRMED/DUPLICATE => +0 a Link y el ataque queda excluido de bloqueo y de golpe directo.
//   RECOVERY_TICK        (LNK04): un tick por mod_update tras 30 ticks sin impactos; tasa por banda de vida (10/8/5/2 pts/s) x1,5 con guardia segura.
// Un credito por (slot, id, generacion, token de ataque). LNK no escribe en Link, en la vida, en el contador nativo de guardia ni en ningun actor.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "hud_core.hpp"
#include "parry_core.hpp"

namespace lnk01 {
inline constexpr const char* kVersion = "0.10.1";
inline constexpr const char* kLnk01Version = "0.9.0";  // la barra y la ocultacion de prompts no cambian respecto de LNK01
inline constexpr unsigned kSlots = post08::kSlots;      // un registro de atacante por slot del ledger (64)

namespace policy {
inline constexpr int kMaxPoints = 100;  // player_posture_max (saturado)
inline constexpr int kMilli = 1000;     // aritmetica entera en milesimas de punto (sin deriva)
inline constexpr int kMaxMilli = kMaxPoints * kMilli;
// LNK07: variantes VERIFICADAS (Bokoblin Expansion 0.17.0, CP10.4/10.5/10.6): recurso + potencia At de las dos esferas en la pasada del contacto.
enum class Variant : std::uint8_t { Blue = 0, Red = 1, Black = 2, BlackFury = 3, Unknown = 4, Count = 5 };
struct Weight {
  int block = 0, direct = 0;  // puntos
};
// [BALANCE_PROVISIONAL_TP] tabla UNICA de pesos (no cifras de Sekiro). Unknown = la fila mas baja: nunca recibe pesos de Black.
inline constexpr Weight kWeights[5] = {{20, 30}, {25, 35}, {30, 40}, {35, 45}, {20, 30}};
// Ataque especial DEMOSTRABLE: el buff de grupo de Expansion (CP10.6) dobla la potencia del Blue (1 -> 2). Ajuste aditivo sobre la fila Blue.
inline constexpr Weight kGroupPowerAdjust = {5, 5};
inline constexpr int kParryCostPoints = 0;  // LNK03: decision definitiva de esta sesion: un parry confirmado da +0 a Link
// Potencias At de Expansion (CP10.5/CP10.6) y nativas (d_a_e_oc.cpp create: E_OC 1, E_OC2 2).
inline constexpr int kAtpBlue = 1, kAtpBlueGroup = 2, kAtpRed = 2, kAtpBlack = 3, kAtpBlackFury = 6;
static_assert(kAtpRed == post08::policy::kRedAtp, "POST08 Red signature");
// LNK04: recuperacion
inline constexpr std::uint32_t kRegenDelayTicks = 30;  // ~1 s a 30 Hz tras el ultimo impacto valido (bloqueo o golpe directo)
inline constexpr int kBandMinPermille[4] = {750, 500, 250, 0};  // vida actual / vida maxima, en milesimas
inline constexpr int kBandPointsPerSecond[4] = {10, 8, 5, 2};    // [BALANCE_PROVISIONAL_TP] siempre decreciente al bajar la vida
inline constexpr int kGuardMulNum = 3, kGuardMulDen = 2;         // x1,5 con guardia segura
inline constexpr int kRegenPointsPerSecond = kBandPointsPerSecond[0];  // vida completa = LNK01
inline constexpr int kTicksPerSecond = 30;                              // reloj de simulacion nominal (game_clock time scale 1)
// LNK02: confirmacion del golpe directo
inline constexpr std::uint32_t kDirectEdgeTicks = 1;   // el flanco de mDamageTimer debe aparecer en el mod_update siguiente al contacto
inline constexpr std::uint32_t kDirectLifeTicks = 3;   // la perdida de vida (aplicada por dMeter2_c::moveLife) debe verse como mucho 3 ticks despues
// HUD
inline constexpr std::uint32_t kHoldTicks = 21;  // 0,7 s visible a 0 antes del fundido
inline constexpr std::uint32_t kFadeTicks = 9;   // 0,3 s de fundido
inline constexpr std::uint8_t kPulseTicks = 24, kHitTicks = 8, kRecoverTicks = 15;
inline constexpr int kPulseRearmBelowPoints = 90;  // un pulso por llegada a 100; se rearma al bajar de 90
// Geometria normalizada (no cifras de Sekiro): centro horizontal del viewport, 87 % de altura, 32 % del ancho util 16:9 (safe area).
inline constexpr float kCenterX = 0.5f, kCenterY = 0.87f, kWidthOfSafe = 0.32f, kSafeAspect = 16.0f / 9.0f;
inline constexpr float kRefHeightPx = 1080.0f;                    // pixeles de referencia de las alturas
inline constexpr float kFillHalfPx = 5.5f, kFrameHalfPx = 8.0f;   // 1080p: relleno 11 px, marco 16 px
inline constexpr float kEnemyBarWidthUnits = 28.0f;               // POST07: ancho total de la barra enemiga (unidades orto)
}  // namespace policy

// ======================================== 1. identidad de variante (LNK07) ========================================================
inline const char* name(policy::Variant v) noexcept {
  static constexpr const char* n[] = {"BLUE", "RED", "BLACK", "BLACK_FURY", "UNKNOWN"};
  return static_cast<unsigned>(v) < 5 ? n[static_cast<unsigned>(v)] : "UNKNOWN";
}
enum class IdMethod : std::uint8_t {
  E_OC_Resource = 0,      // recurso E_OC: Blue (Expansion regla 3 / registro tipado Blue; nativo sin Expansion)
  E_OC2_Atp1LegacyBlue,   // E_OC2 con potencia 1 (ahora o antes en ESTA generacion): solo el Blue normal/legado de DEVF
  E_OC2_Atp2Red,          // E_OC2 con potencia 2 y nunca 1 ni 3/6 en esta generacion: Red (Expansion regla 4 / registro Red / nativo)
  E_OC2_Atp3Black,        // potencia 3: solo Black base (CP10.5)
  E_OC2_Atp6BlackFury,    // potencia 6: solo Black durante los 300 ticks de furia (CP10.6)
  E_OC2_BlackHistory,     // potencia transitoria (esferas en desacuerdo) en un actor ya visto con 3/6
  UnknownAtp,             // ninguna firma reconocida: fila Unknown (la mas baja), nunca pesos de Black
  Count
};
inline const char* name(IdMethod m) noexcept {
  static constexpr const char* n[] = {"E_OC_RESOURCE", "E_OC2_ATP1_LEGACY_BLUE", "E_OC2_ATP2_RED", "E_OC2_ATP3_BLACK", "E_OC2_ATP6_BLACK_FURY",
                                      "E_OC2_BLACK_HISTORY", "UNKNOWN_ATP"};
  return static_cast<unsigned>(m) < 7 ? n[static_cast<unsigned>(m)] : "UNKNOWN_ATP";
}
struct VariantId {
  policy::Variant v = policy::Variant::Unknown;
  IdMethod m = IdMethod::UnknownAtp;
  bool group_power = false;  // Blue con la potencia doblada por el grupo (CP10.6)
  int atp = 0;
};
// Pesos de UNA tabla: fila de la variante + ajuste especial demostrable (grupo Blue). Nunca un multiplicador de velocidad de ataque.
inline policy::Weight weight_of(const VariantId& id) noexcept {
  const unsigned i = static_cast<unsigned>(id.v) < 5 ? static_cast<unsigned>(id.v) : 4u;
  policy::Weight w = policy::kWeights[i];
  if (id.group_power && id.v == policy::Variant::Blue) w.block += policy::kGroupPowerAdjust.block, w.direct += policy::kGroupPowerAdjust.direct;
  return w;
}
inline constexpr std::uint8_t atp_bit(int atp) noexcept {
  return atp == 1 ? 1u : (atp == 2 ? 2u : (atp == 3 ? 4u : (atp == 6 ? 8u : 0u)));
}
// Clasificacion PURA a partir del recurso, la potencia actual y la historia de potencias de ESTA identidad (slot+id+generacion).
inline VariantId classify_variant(bool e_oc2, int atp, std::uint8_t seen) noexcept {
  VariantId r;
  r.atp = atp;
  seen = static_cast<std::uint8_t>(seen | atp_bit(atp));
  if (!e_oc2) {
    r.v = policy::Variant::Blue, r.m = IdMethod::E_OC_Resource, r.group_power = atp == policy::kAtpBlueGroup;
  } else if (atp == policy::kAtpBlackFury) {
    r.v = policy::Variant::BlackFury, r.m = IdMethod::E_OC2_Atp6BlackFury;
  } else if (atp == policy::kAtpBlack) {
    r.v = policy::Variant::Black, r.m = IdMethod::E_OC2_Atp3Black;
  } else if (seen & (4u | 8u)) {
    r.v = policy::Variant::Black, r.m = IdMethod::E_OC2_BlackHistory;
  } else if (seen & 1u) {
    r.v = policy::Variant::Blue, r.m = IdMethod::E_OC2_Atp1LegacyBlue, r.group_power = atp == policy::kAtpBlueGroup;
  } else if (atp == policy::kAtpRed) {
    r.v = policy::Variant::Red, r.m = IdMethod::E_OC2_Atp2Red;
  }
  return r;
}
class VariantTrack {
 public:
  // UNA observacion por PRE de damage_check de un actor seguido (O(1)). Identidad nueva o reciclada: la historia empieza de cero.
  void observe(int slot, std::uint32_t id, std::uint64_t generation, bool e_oc2, int atp) noexcept {
    if (slot < 0 || static_cast<unsigned>(slot) >= kSlots || id == 0) return;
    S& s = s_[slot];
    if (!s.bound || s.id != id || s.generation != generation || s.e_oc2 != e_oc2) s = S{true, e_oc2, id, generation, 0};
    s.seen = static_cast<std::uint8_t>(s.seen | atp_bit(atp));
  }
  VariantId classify(int slot, std::uint32_t id, std::uint64_t generation, bool e_oc2, int atp) const noexcept {
    std::uint8_t seen = 0;
    if (slot >= 0 && static_cast<unsigned>(slot) < kSlots) {
      const S& s = s_[slot];
      if (s.bound && s.id == id && s.generation == generation && s.e_oc2 == e_oc2) seen = s.seen;
    }
    return classify_variant(e_oc2, atp, seen);
  }
  void reset() noexcept {
    for (S& s : s_) s = S{};
  }

 private:
  struct S {
    bool bound = false, e_oc2 = false;
    std::uint32_t id = 0;
    std::uint64_t generation = 0;
    std::uint8_t seen = 0;
  };
  S s_[kSlots]{};
};

// ======================================== 2. postura de Link (contador UNICO) =====================================================
enum class Verdict : std::uint8_t {
  None = 0,                     // ningun contacto de este atacante con Link en la ultima pasada
  Confirmed,                    // CONFIRMED_NATIVE_BLOCK: bloqueo ordinario real => peso de bloqueo
  RejectedStale,                // registro At de una pasada no armada (FIX02)
  RejectedParry,                // parry POST08 (o episodio ya desviado): +0 para Link
  RejectedNonBlock,             // contacto fuera de golpe activo (pasada de rebote, NOT_ATTACKING)
  Duplicate,                    // este ataque ya produjo su credito (o su golpe directo ya esta pendiente)
  RejectedUntracked,            // identidad del atacante o del episodio no demostrada
  RejectedLink,                 // Link no valida / no humana / distinta de la ligada
  RejectedEvent,                // evento / cinematica
  DirectPending,                // NOT_BLOCKED fresco y atribuido: espera la confirmacion nativa del dano (mod_update siguiente)
  RejectedDirectNotAttributed,  // NOT_BLOCKED pero el cilindro Tg del que Link toma el dano registro OTRO objeto: este contacto no es el que hiere
  Count
};
inline constexpr unsigned kVerdictCount = static_cast<unsigned>(Verdict::Count);
inline const char* name(Verdict v) noexcept {
  static constexpr const char* n[] = {"NONE",      "CONFIRMED_NATIVE_BLOCK", "REJECTED_STALE",      "REJECTED_PARRY", "REJECTED_NONBLOCK",
                                      "DUPLICATE", "REJECTED_UNTRACKED",     "REJECTED_LINK_STATE", "REJECTED_EVENT", "DIRECT_PENDING",
                                      "REJECTED_DIRECT_NOT_ATTRIBUTED"};
  return static_cast<unsigned>(v) < kVerdictCount ? n[static_cast<unsigned>(v)] : "NONE";
}
enum class DirectResult : std::uint8_t {
  Confirmed = 0,   // flanco de mDamageTimer en el tick del contacto + perdida real de vida
  NoNativeDamage,  // sin flanco: invulnerable, GUARD_SLIP, dano 0, estado especial... el juego no hirio a Link con este contacto
  NoLifeLoss,      // hubo flanco pero la vida no bajo (armadura magica, curacion simultanea...): sin postura
  Ambiguous,       // dos candidatos del mismo tick para UN dano: politica conservadora, ninguno suma
  LinkChanged,     // Link desaparecio o cambio antes de confirmar
  Superseded,      // la identidad del slot cambio (actor reciclado) o el ataque fue desviado antes de confirmar
  Count
};
inline constexpr unsigned kDirectResultCount = static_cast<unsigned>(DirectResult::Count);
inline const char* name(DirectResult r) noexcept {
  static constexpr const char* n[] = {"CONFIRMED_DIRECT_HIT", "REJECTED_NO_NATIVE_DAMAGE", "REJECTED_NO_LIFE_LOSS", "REJECTED_AMBIGUOUS",
                                      "REJECTED_LINK_CHANGED", "REJECTED_SUPERSEDED"};
  return static_cast<unsigned>(r) < kDirectResultCount ? n[static_cast<unsigned>(r)] : "NONE";
}

// Lo que el adaptador copia (por valor) del veredicto POST08 de UNA llamada de damage_check, despues de post08_step (parry ya comprometido o no).
struct Contact {
  post08::Result result = post08::Result::None;
  bool blocked = false;  // AtShieldHit nativo en la esfera de contacto
  std::uint64_t token = 0;
  int slot = -1;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  bool episode_parried = false;  // post08::Parry::episode(slot).parried para ESTA identidad y ESTE token
  bool e_oc2 = false;
  int atp = 0;
  bool link_ok = false, link_human = false;
  std::uint32_t link_id = 0;
  bool event_running = false;
  bool tg_match = false;  // LNK02: el PRIMER cilindro Tg de Link con impacto (el que usa checkDamageAction) registro una esfera de ESTE atacante
  std::uint64_t tick = 0;
};

struct Outcome {
  Verdict v = Verdict::None;
  VariantId vid;
  int weight = 0;             // puntos aplicados (o previstos para un golpe directo pendiente)
  int before = 0, after = 0;  // milesimas
  bool saturated = false;     // llego a 100 con este impacto
};

// Lo que el adaptador lee de la Link viva UNA vez por mod_update (solo lectura, por valor).
struct LinkObs {
  bool valid = false, human = false;
  std::uint32_t id = 0;
  int life = 0, life_max = 0;      // vida actual y maxima en las MISMAS unidades (cuartos de corazon: (getMaxLife()/5)*4)
  int damage_timer = 0;            // daPy_py_c::mDamageTimer (setDamagePoint lo arma con el tiempo de invulnerabilidad)
  float pending_life = 0.0f;       // dComIfG_play_c ItemLifeCount: resta pendiente que dMeter2_c::moveLife aplicara
  bool guard_safe = false;         // guardia humana real (replica de lectura de checkUpperGuardAnime) sin GUARD_SLIP ni invulnerabilidad
  bool active = false;             // simulacion avanzando: Link valida, vida > 0, sin pausa, sin evento, sin menu
};
inline int life_permille(int life, int life_max) noexcept {
  if (life_max <= 0 || life < 0) return -1;
  const long long p = static_cast<long long>(life) * 1000 / life_max;
  return p > 1000 ? 1000 : static_cast<int>(p);
}
inline unsigned regen_band(int permille) noexcept {  // permille invalido (sin vida maxima) => banda mas lenta (conservador)
  if (permille < 0) return 3;
  for (unsigned b = 0; b < 4; ++b)
    if (permille >= policy::kBandMinPermille[b]) return b;
  return 3;
}

struct RegenEvent {
  bool started = false, zero = false, paused_edge = false, resumed_edge = false, band_change = false, guard_change = false;
  int before = 0, after = 0;
  unsigned band = 0;
  bool guard = false;
  int rate_milli_per_s = 0;
};
struct DirectEvent {
  DirectResult r = DirectResult::Confirmed;
  int slot = -1;
  std::uint32_t id = 0;
  std::uint64_t generation = 0, token = 0, contact_tick = 0, confirm_tick = 0;
  VariantId vid;
  int weight = 0, before = 0, after = 0, life_before = 0, life_now = 0, timer = 0;
  float pending_life = 0.0f;
  bool saturated = false;
};
inline constexpr unsigned kMaxDirectEvents = 8;
struct DirectEvents {
  DirectEvent e[kMaxDirectEvents]{};
  unsigned n = 0, dropped = 0;
  void push(const DirectEvent& x) noexcept {
    if (n < kMaxDirectEvents) e[n++] = x;
    else ++dropped;
  }
};

struct PCounters {
  std::uint64_t contacts = 0, verdict[kVerdictCount] = {}, block_by_variant[5] = {}, direct_by_variant[5] = {}, direct_result[kDirectResultCount] = {},
                gained_milli = 0, gained_block_milli = 0, gained_direct_milli = 0, regen_milli = 0, regen_ticks = 0, active_ticks = 0, paused_ticks = 0,
                regen_starts = 0, saturations = 0, zeros = 0, resets = 0, link_changes = 0, binds = 0, regen_ticks_by_band[4] = {}, regen_milli_by_band[4] = {},
                regen_guard_ticks = 0, regen_guard_milli = 0, fury_credits = 0, group_adjusted = 0, unknown_variant = 0, damage_edges = 0,
                damage_edges_unattributed = 0, parries_excluded = 0;
};

class Posture {
 public:
  PCounters c{};
  VariantTrack variants;

  // UNA vez por mod_update: identidad de la Link viva. Sin Link o Link distinta => reinicio determinista (0, sin herencia, pendientes descartados).
  void observe_link(bool valid, std::uint32_t id) noexcept {
    if (!valid || id == 0) {
      if (bound_) reset();
      return;
    }
    if (!bound_ || id != link_id_) {
      if (bound_) ++c.link_changes;
      reset();
      bound_ = true;
      link_id_ = id;
      ++epoch_;
      ++c.binds;
    }
  }

  // UN juicio por contacto (por llamada de damage_check con veredicto POST08 distinto de None). O(1).
  Outcome on_contact(const Contact& k) noexcept {
    Outcome o;
    if (k.result == post08::Result::None) return o;
    ++c.contacts;
    o.vid = variants.classify(k.slot, k.id, k.generation, k.e_oc2, k.atp);
    o.before = o.after = cur_;
    L* e = nullptr;
    o.v = classify(k, e);
    ++c.verdict[static_cast<unsigned>(o.v) < kVerdictCount ? static_cast<unsigned>(o.v) : 0];
    if (o.v == Verdict::RejectedParry) ++c.parries_excluded;
    const policy::Weight w = weight_of(o.vid);
    if (o.v == Verdict::DirectPending && e) {
      e->pend = true, e->pend_token = k.token, e->pend_tick = k.tick, e->pend_vid = o.vid, e->pend_edge = false, e->pend_edge_tick = 0, e->pend_life_ref = 0;
      e->direct = k.token;
      ++pending_;
      o.weight = w.direct;
      return o;
    }
    if (o.v != Verdict::Confirmed) return o;
    o.weight = w.block;
    apply(o.weight * policy::kMilli, o.before, o.after, o.saturated);
    c.gained_block_milli += static_cast<std::uint64_t>(o.after - o.before);
    ++c.block_by_variant[static_cast<unsigned>(o.vid.v) % 5];
    note_variant(o.vid);
    return o;
  }

  // UNA vez por mod_update, DESPUES de observe_link: confirma o descarta los golpes directos pendientes con la evidencia nativa de este tick.
  void confirm_direct(const LinkObs& ob, std::uint64_t tick, DirectEvents& out) noexcept {
    const bool edge = ob.valid && bound_ && ob.id == link_id_ && ob.damage_timer > 0 && ob.damage_timer > prev_timer_;
    if (edge) ++c.damage_edges;
    unsigned attributed = 0;
    if (pending_) {
      unsigned same_tick = 0;  // candidatos cuyo contacto fue en el tick anterior: un flanco = UN dano
      for (const L& e : led_)
        if (e.pend && !e.pend_edge && e.pend_tick + policy::kDirectEdgeTicks == tick) ++same_tick;
      for (unsigned i = 0; i < kSlots; ++i) {
        L& e = led_[i];
        if (!e.pend || e.pend_tick >= tick) continue;  // contacto de este mismo tick: se juzga en el mod_update siguiente
        DirectEvent d = event_of(i, e, tick, ob);
        if (!ob.valid || !bound_ || ob.id != link_id_) {
          finish(e, DirectResult::LinkChanged, d, out);
          continue;
        }
        if (e.parried == e.pend_token) {
          finish(e, DirectResult::Superseded, d, out);
          continue;
        }
        if (!e.pend_edge) {
          if (e.pend_tick + policy::kDirectEdgeTicks != tick) {  // el mod_update siguiente al contacto ya paso sin flanco
            finish(e, DirectResult::NoNativeDamage, d, out);
            continue;
          }
          if (!edge) {
            finish(e, DirectResult::NoNativeDamage, d, out);
            continue;
          }
          if (same_tick != 1) {
            finish(e, DirectResult::Ambiguous, d, out);
            continue;
          }
          ++attributed;
          e.pend_edge = true, e.pend_edge_tick = tick, e.pend_life_ref = prev_life_;
          d.life_before = prev_life_;
        }
        if (ob.life < e.pend_life_ref || ob.pending_life < 0.0f) {  // perdida real: aplicada por moveLife o pendiente de aplicar
          const policy::Weight w = weight_of(e.pend_vid);
          d.weight = w.direct;
          apply(d.weight * policy::kMilli, d.before, d.after, d.saturated);
          c.gained_direct_milli += static_cast<std::uint64_t>(d.after - d.before);
          ++c.direct_by_variant[static_cast<unsigned>(e.pend_vid.v) % 5];
          note_variant(e.pend_vid);
          finish(e, DirectResult::Confirmed, d, out);
        } else if (tick >= e.pend_edge_tick + policy::kDirectLifeTicks) {
          finish(e, DirectResult::NoLifeLoss, d, out);
        }
      }
    }
    if (edge && attributed == 0) ++c.damage_edges_unattributed;  // dano de otra fuente (entorno, otro enemigo no seguido, caida...): nunca se acredita
    prev_timer_ = ob.valid ? ob.damage_timer : 0;
    prev_life_ = ob.valid ? ob.life : 0;
  }

  // UN tick de simulacion (mod_update), DESPUES de confirm_direct. Recuperacion por banda de vida, x1,5 con guardia segura; congelada si !active.
  RegenEvent tick(bool active, int permille, bool guard_safe) noexcept {
    RegenEvent ev;
    ev.before = ev.after = cur_;
    ev.band = regen_band(permille);
    ev.guard = guard_safe;
    ev.rate_milli_per_s = rate_milli_per_s(ev.band, guard_safe);
    if (!bound_) return ev;
    if (!active) {
      ++c.paused_ticks;
      if (!paused_) paused_ = true, ev.paused_edge = true;
      return ev;
    }
    if (paused_) paused_ = false, ev.resumed_edge = true;
    ++c.active_ticks;
    if (since_ < 0xFFFFFFFFu) ++since_;
    if (cur_ <= 0 || since_ <= policy::kRegenDelayTicks) return ev;
    if (!regenerating_) regenerating_ = true, ++c.regen_starts, ev.started = true, last_band_ = ev.band, last_guard_ = guard_safe;
    if (ev.band != last_band_) ev.band_change = true, last_band_ = ev.band;
    if (guard_safe != last_guard_) ev.guard_change = true, last_guard_ = guard_safe;
    // milesimas por segundo x den, repartidas por tick con resto exacto (sin deriva; el cambio de banda conserva el resto)
    acc_ += ev.rate_milli_per_s * policy::kGuardMulDen;
    int d = acc_ / (policy::kTicksPerSecond * policy::kGuardMulDen);
    acc_ -= d * policy::kTicksPerSecond * policy::kGuardMulDen;
    if (d > cur_) d = cur_;
    cur_ -= d;
    c.regen_milli += static_cast<std::uint64_t>(d);
    ++c.regen_ticks;
    ++c.regen_ticks_by_band[ev.band];
    c.regen_milli_by_band[ev.band] += static_cast<std::uint64_t>(d);
    if (guard_safe) ++c.regen_guard_ticks, c.regen_guard_milli += static_cast<std::uint64_t>(d);
    if (cur_ == 0) {
      ++c.zeros;
      ev.zero = true;
      acc_ = 0;
      regenerating_ = false;
    }
    ev.after = cur_;
    return ev;
  }
  // Compatibilidad LNK01: recuperacion neutral a vida completa.
  RegenEvent tick(bool active) noexcept { return tick(active, 1000, false); }

  static int rate_milli_per_s(unsigned band, bool guard_safe) noexcept {
    const int base = policy::kBandPointsPerSecond[band < 4 ? band : 3] * policy::kMilli;
    return guard_safe ? base * policy::kGuardMulNum / policy::kGuardMulDen : base;
  }
  int milli() const noexcept { return cur_; }
  bool bound() const noexcept { return bound_; }
  std::uint32_t link_id() const noexcept { return link_id_; }
  std::uint64_t epoch() const noexcept { return epoch_; }
  std::uint32_t since_block() const noexcept { return since_; }
  bool regenerating() const noexcept { return regenerating_; }
  bool paused() const noexcept { return paused_; }
  unsigned pending() const noexcept { return pending_; }
  // Invariante de conservacion: lo ganado (bloqueos + golpes directos) es exactamente lo regenerado + el valor actual + lo perdido en reinicios.
  bool conserved() const noexcept {
    return c.gained_milli == c.regen_milli + static_cast<std::uint64_t>(cur_) + lost_milli_ && c.gained_milli == c.gained_block_milli + c.gained_direct_milli;
  }
  std::uint64_t lost_milli() const noexcept { return lost_milli_; }

 private:
  struct L {
    bool bound = false;
    std::uint32_t id = 0;
    std::uint64_t generation = 0, credited = 0, parried = 0, direct = 0;
    // golpe directo pendiente (uno por identidad de atacante)
    bool pend = false, pend_edge = false;
    std::uint64_t pend_token = 0, pend_tick = 0, pend_edge_tick = 0;
    int pend_life_ref = 0;
    VariantId pend_vid;
  };
  L* entry(const Contact& k) noexcept {
    if (k.slot < 0 || static_cast<unsigned>(k.slot) >= kSlots || k.id == 0) return nullptr;
    L& e = led_[k.slot];
    if (!e.bound || e.id != k.id || e.generation != k.generation) {  // identidad nueva o reciclada: nada se hereda (un pendiente queda superado)
      if (e.pend) {
        ++c.direct_result[static_cast<unsigned>(DirectResult::Superseded)];
        --pending_;
      }
      e = L{};
      e.bound = true, e.id = k.id, e.generation = k.generation;
    }
    return &e;
  }
  Verdict classify(const Contact& k, L*& e) noexcept {
    using R = post08::Result;
    e = entry(k);
    bool direct = false;
    switch (k.result) {
      case R::StaleContact: return Verdict::RejectedStale;
      case R::Confirmed:
      case R::Duplicate:
        if (e && k.token) e->parried = k.token;  // el resto de este ataque nunca es bloqueo ordinario ni golpe directo
        return Verdict::RejectedParry;
      case R::NotAttacking: return Verdict::RejectedNonBlock;
      case R::MissedLate: return Verdict::Duplicate;
      case R::EventRunning: return Verdict::RejectedEvent;
      case R::NotBlocked: direct = true; break;
      case R::NormalGuard:
      case R::MissedEarly:
      case R::Latched:
      case R::FuryChipPossible: break;
      default: return Verdict::RejectedUntracked;  // Untracked, LinkInvalid, NoPassSnapshot
    }
    if (direct ? k.blocked : !k.blocked) return Verdict::RejectedNonBlock;  // la marca nativa de escudo decide bloqueo frente a golpe directo
    if (k.event_running) return Verdict::RejectedEvent;
    if (!k.link_ok || !k.link_human || !bound_ || k.link_id != link_id_) return Verdict::RejectedLink;
    if (!e || k.token == 0) return Verdict::RejectedUntracked;
    if (k.episode_parried || e->parried == k.token) return Verdict::RejectedParry;
    if (e->credited == k.token || e->direct == k.token) return Verdict::Duplicate;  // UN resultado por ataque: bloqueo XOR golpe directo
    if (direct) {
      if (!k.tg_match) return Verdict::RejectedDirectNotAttributed;
      return Verdict::DirectPending;
    }
    e->credited = k.token;
    return Verdict::Confirmed;
  }
  void apply(int gain_milli, int& before, int& after, bool& saturated) noexcept {
    before = cur_;
    cur_ = cur_ + gain_milli > policy::kMaxMilli ? policy::kMaxMilli : cur_ + gain_milli;
    c.gained_milli += static_cast<std::uint64_t>(cur_ - before);
    since_ = 0;  // todo impacto valido (bloqueo o golpe directo) reinicia el retraso
    acc_ = 0;
    regenerating_ = false;
    after = cur_;
    saturated = before < policy::kMaxMilli && cur_ == policy::kMaxMilli;
    if (saturated) ++c.saturations;
  }
  void note_variant(const VariantId& v) noexcept {
    if (v.v == policy::Variant::BlackFury) ++c.fury_credits;
    if (v.v == policy::Variant::Unknown) ++c.unknown_variant;
    if (v.group_power && v.v == policy::Variant::Blue) ++c.group_adjusted;
  }
  DirectEvent event_of(unsigned i, const L& e, std::uint64_t tick, const LinkObs& ob) const noexcept {
    DirectEvent d;
    d.slot = static_cast<int>(i), d.id = e.id, d.generation = e.generation, d.token = e.pend_token, d.contact_tick = e.pend_tick, d.confirm_tick = tick;
    d.vid = e.pend_vid, d.before = d.after = cur_, d.life_before = e.pend_edge ? e.pend_life_ref : prev_life_, d.life_now = ob.life, d.timer = ob.damage_timer;
    d.pending_life = ob.pending_life;
    return d;
  }
  void finish(L& e, DirectResult r, DirectEvent& d, DirectEvents& out) noexcept {
    e.pend = false;
    if (pending_) --pending_;
    ++c.direct_result[static_cast<unsigned>(r)];
    d.r = r;
    out.push(d);
  }
  void reset() noexcept {
    lost_milli_ += static_cast<std::uint64_t>(cur_);
    for (L& e : led_)
      if (e.pend) ++c.direct_result[static_cast<unsigned>(DirectResult::LinkChanged)];
    cur_ = 0, acc_ = 0, since_ = 0xFFFFFFFFu, regenerating_ = false, paused_ = false, pending_ = 0;
    bound_ = false, link_id_ = 0, prev_timer_ = 0, prev_life_ = 0;
    for (L& e : led_) e = L{};
    ++c.resets;
  }
  L led_[kSlots]{};
  bool bound_ = false, regenerating_ = false, paused_ = false, last_guard_ = false;
  unsigned last_band_ = 0, pending_ = 0;
  std::uint32_t link_id_ = 0, since_ = 0xFFFFFFFFu;
  std::uint64_t epoch_ = 0, lost_milli_ = 0;
  int cur_ = 0, acc_ = 0, prev_timer_ = 0, prev_life_ = 0;
};

// ======================================== 3. barra HUD de Link (display only) =======================================================
struct Visual {
  float fill = 0, alpha = 0, hit = 0, pulse = 0, recover = 0;
  bool saturated = false;
};

struct HudCounters {
  std::uint64_t ticks = 0, paused = 0, hits = 0, recoveries = 0, pulses = 0, shown_ticks = 0, fade_outs = 0, resets = 0, hidden_not_human = 0;
};

class PlayerHud {
 public:
  HudCounters c{};
  void reset() noexcept {  // Link nueva / sin Link: sin barra fantasma ni herencia; los contadores de sesion se conservan
    const HudCounters keep = c;
    *this = PlayerHud{};
    c = keep;
    ++c.resets;
  }
  // UN tick de simulacion. advance = false (pausa, menu, evento, HUD nativo oculto): nada se mueve. human = false (lobo, sin Link): se oculta rapido.
  void tick(int target_milli, bool advance, bool human) noexcept {
    if (!advance) {
      ++c.paused;
      return;
    }
    ++c.ticks;
    if (hit_) --hit_;
    if (pulse_) --pulse_;
    if (recover_) --recover_;
    const int t = target_milli < 0 ? 0 : (target_milli > policy::kMaxMilli ? policy::kMaxMilli : target_milli);
    if (t > target_) hit_ = policy::kHitTicks, ++c.hits;
    else if (t < target_) recover_ = policy::kRecoverTicks, ++c.recoveries;
    if (t >= policy::kMaxMilli && pulse_armed_) pulse_ = policy::kPulseTicks, pulse_armed_ = false, ++c.pulses;
    else if (t < policy::kPulseRearmBelowPoints * policy::kMilli) pulse_armed_ = true;
    target_ = t;
    const float T = static_cast<float>(t) / static_cast<float>(policy::kMilli);
    if (T > display_) {  // subida rapida (pocos ticks), retirada continua hacia el centro
      display_ += post07::maxf((T - display_) * 0.5f, 2.0f);
      if (display_ > T) display_ = T;
    } else if (T < display_) {
      display_ -= post07::maxf((display_ - T) * 0.25f, 0.15f);
      if (display_ < T) display_ = T;
    }
    const bool content = t > 0 || display_ > 0.05f || pulse_;
    if (!human) {
      ++c.hidden_not_human;
      alpha_ = post07::maxf(0.0f, alpha_ - 0.34f);
      hold_ = 0;
    } else if (content) {
      hold_ = policy::kHoldTicks;
      alpha_ = post07::minf(1.0f, alpha_ + 0.34f);
    } else if (hold_ > 0) {
      --hold_;
    } else if (alpha_ > 0.0f) {
      alpha_ = post07::maxf(0.0f, alpha_ - 1.0f / static_cast<float>(policy::kFadeTicks));
      if (alpha_ < 0.01f) alpha_ = 0.0f, ++c.fade_outs;
    }
    if (alpha_ >= 0.01f) ++c.shown_ticks;
  }
  bool drawable() const noexcept { return alpha_ >= 0.01f; }
  Visual visual() const noexcept {
    Visual v;
    v.fill = post07::clamp01(display_ / static_cast<float>(policy::kMaxPoints));
    v.alpha = alpha_;
    v.hit = static_cast<float>(hit_) / policy::kHitTicks;
    v.pulse = static_cast<float>(pulse_) / policy::kPulseTicks;
    v.recover = static_cast<float>(recover_) / policy::kRecoverTicks;
    v.saturated = target_ >= policy::kMaxMilli;
    return v;
  }
  float display() const noexcept { return display_; }
  std::uint32_t hold() const noexcept { return hold_; }

 private:
  int target_ = 0;
  float display_ = 0, alpha_ = 0;
  std::uint32_t hold_ = 0;
  std::uint8_t hit_ = 0, pulse_ = 0, recover_ = 0;
  bool pulse_armed_ = true;
};

// Geometria en las unidades ORTO del pintor (setOrtho(minX, minY, width, height)): centrada en el viewport logico, independiente de 1920x1080.
struct Layout {
  bool ok = false;
  float cx = 0, cy = 0, half_w = 0, fill_h = 0, frame_h = 0, u = 0;  // u = unidades orto por pixel de referencia (alto 1080)
};
inline Layout layout(float minx, float miny, float width, float height) noexcept {
  Layout L;
  if (!post07::isfin(minx) || !post07::isfin(miny) || !post07::isfin(width) || !post07::isfin(height) || !(width > 1.0f) || !(height > 1.0f)) return L;
  const float safe_w = post07::minf(width, height * policy::kSafeAspect);  // ultrawide: el ancho util es el 16:9 centrado
  L.u = height / policy::kRefHeightPx;
  L.cx = minx + width * policy::kCenterX;
  L.cy = miny + height * policy::kCenterY;
  L.half_w = safe_w * policy::kWidthOfSafe * 0.5f;
  L.fill_h = policy::kFillHalfPx * L.u;
  L.frame_h = policy::kFrameHalfPx * L.u;
  L.ok = true;
  return L;
}

// UN solo paquete 2D por pasada de simulacion (el de POST07): se encola si hay barras enemigas O si la barra de Link es visible.
inline bool packet_needed(bool allowed, unsigned enemy_bars, bool player_on) noexcept { return allowed && (enemy_bars > 0 || player_on); }

inline constexpr unsigned kMaxQuads = 48;
// Barra propia: placa oscura translucida, marco bronce con remates escalonados, pista oscura y relleno SIMETRICO centro -> extremos (oro -> ambar ->
// naranja -> rojo, la rampa de POST07). Pulso breve a 100 (sin efecto de juego). Orden = orden del pintor (de atras adelante).
inline unsigned build_bar(const Visual& v, const Layout& L, post07::Quad* out, unsigned cap) noexcept {
  using namespace post07;
  using namespace post07::palette;
  QuadSink q(out, cap);
  const float a = clamp01(v.alpha);
  if (!L.ok || a < 0.01f) return 0;
  const float cx = L.cx, cy = L.cy, W = L.half_w, u = L.u, F = L.fill_h, Fr = L.frame_h;
  const float f = clamp01(v.fill), hit = clamp01(v.hit), pulse = clamp01(v.pulse), rec = clamp01(v.recover);
  // 1. placa oscura con extremos difuminados (contraste sobre escenas claras)
  {
    const float bw = W + 16.0f * u, bh = Fr + 3.0f * u, fade = 14.0f * u;
    q.vgrad(cx - bw + fade, cy - bh, cx + bw - fade, cy + bh, alpha(kBlack, 0.40f * a), alpha(kBlack, 0.52f * a));
    q.hgrad(cx - bw, cy - bh, cx - bw + fade, cy + bh, alpha(kBlack, 0.0f), alpha(kBlack, 0.46f * a));
    q.hgrad(cx + bw - fade, cy - bh, cx + bw, cy + bh, alpha(kBlack, 0.46f * a), alpha(kBlack, 0.0f));
  }
  // 2. pulso de saturacion: brasa que se expande por detras del marco (una vez por llegada a 100)
  if (pulse > 0.0f) {
    const float g = 1.0f - pulse, wx = W + (6.0f + 34.0f * g) * u, hy = Fr + (2.0f + 7.0f * g) * u;
    const Rgba c = alpha(kPulse, 0.70f * pulse * a), z = alpha(kPulse, 0.0f);
    q.hgrad(cx - wx, cy - hy, cx, cy + hy, z, c);
    q.hgrad(cx, cy - hy, cx + wx, cy + hy, c, z);
  }
  Rgba base = ramp(f);
  base = mix(base, kWhite, 0.30f * hit);
  // 3. marco bronce, filete oscuro interior y pista
  q.vgrad(cx - W - 2.0f * u, cy - Fr, cx + W + 2.0f * u, cy + Fr, alpha(kBronzeLight, 0.90f * a), alpha(kBronzeDark, 0.90f * a));
  q.vgrad(cx - W - 1.0f * u, cy - Fr + 1.2f * u, cx + W + 1.0f * u, cy + Fr - 1.2f * u, alpha(kEdgeDark, 0.95f * a), alpha(kEdgeDark, 0.95f * a));
  q.vgrad(cx - W, cy - F, cx + W, cy + F, alpha(kTrackTop, 0.92f * a), alpha(kTrackBottom, 0.92f * a));
  // 4. remates escalonados (simetricos) en ambos extremos
  for (unsigned k = 0; k < 4; ++k) {
    const float hk = (Fr + 2.0f * u) * (1.0f - static_cast<float>(k) / 4.0f), x0 = W + 2.0f * u + static_cast<float>(k) * 2.2f * u, x1 = x0 + 2.2f * u;
    const Rgba t = alpha(kBronzeLight, 0.92f * a), b = alpha(kBronzeDark, 0.92f * a);
    q.vgrad(cx + x0, cy - hk, cx + x1, cy + hk, t, b);
    q.vgrad(cx - x1, cy - hk, cx - x0, cy + hk, t, b);
  }
  // 5. relleno desde el CENTRO hacia AMBOS extremos
  const float E = f * W;
  if (E > 0.0f) {
    const Rgba edge = mix(base, kWhite, 0.12f), centre = mix(base, kBlack, 0.20f);
    const Rgba eT = alpha(mix(edge, kWhite, 0.18f), a), eB = alpha(mix(edge, kBlack, 0.25f), a), cT = alpha(mix(centre, kWhite, 0.18f), a),
               cB = alpha(mix(centre, kBlack, 0.25f), a);
    q.add(cx - E, cy - F, cx, cy + F, eT, cT, cB, eB);
    q.add(cx, cy - F, cx + E, cy + F, cT, eT, eB, cB);
    q.vgrad(cx - E, cy - F, cx + E, cy - F * 0.30f, alpha(kWhite, 0.22f * a), alpha(kWhite, 0.03f * a));  // brillo superior
    const float bl = (0.16f + 0.22f * hit + (v.saturated ? 0.14f : 0.0f)) * a;                        // halo calido fuera de la pista
    q.vgrad(cx - E, cy - F - 2.0f * u, cx + E, cy - F, alpha(base, 0.0f), alpha(base, bl));
    q.vgrad(cx - E, cy + F, cx + E, cy + F + 2.0f * u, alpha(base, bl), alpha(base, 0.0f));
    if (pulse > 0.0f) q.vgrad(cx - E, cy - F, cx + E, cy + F, alpha(kWhite, 0.80f * pulse * pulse * a), alpha(kWhite, 0.55f * pulse * pulse * a));
    if (E > 2.0f * u) {  // bordes de avance: calientes al subir, palidos al recuperarse
      const float w = 2.0f * u, hs = F + 0.6f * u;
      Rgba spark = alpha(mix(edge, kWhite, 0.45f + 0.45f * hit), (0.50f + 0.45f * hit) * a);
      if (rec > 0.0f && hit <= 0.0f) spark = alpha(mix(edge, kRecover, 0.4f + 0.6f * rec), (0.35f + 0.55f * rec) * a);
      q.add(cx + E - w, cy - hs, cx + E, cy + hs, alpha(spark, 0.0f), spark, spark, alpha(spark, 0.0f));
      q.add(cx - E, cy - hs, cx - E + w, cy + hs, spark, alpha(spark, 0.0f), alpha(spark, 0.0f), spark);
    }
  }
  // 6. muescas del centro (origen de la barra)
  {
    const Rgba c = alpha(kBronzeLight, 0.95f * a);
    q.vgrad(cx - 1.0f * u, cy - Fr - 3.0f * u, cx + 1.0f * u, cy - Fr, c, c);
    q.vgrad(cx - 1.0f * u, cy + Fr, cx + 1.0f * u, cy + Fr + 3.0f * u, c, c);
  }
  return q.count();
}

// ======================================== 4. supresion VISUAL de los prompts de accion inferiores =====================================
// Familia: dMeterButton_c (mpEmpButton de dMeter2_c): el clúster de botones/textos de accion centrado abajo (mEmpButtonPosY = 175 sobre 448).
// Se oculta la RAIZ de su pantalla de botones (J2DPane::mVisible del J2DScreen "zelda_game_image_button_info.blo": iconos A/B/X/Y/Z/R/C/S/3D/Bin,
// textos info_ar*/fnfo_ar*, flechas yaji_*, Midna, objeto de pesca) y, durante SU draw, se enmascaran los dos slots de brillo (field_0x4be) para que
// drawPikari no pinte destellos huerfanos; los slots se restauran en el POST del mismo draw. El mensaje flotante (mpScreen/mpTextScreen) NO se toca.
// Ningun estado logico del juego (Do/A status, flags, temporizadores, sonidos, entradas) se modifica.
inline constexpr std::uint8_t kButtonNone = 22;  // dMeterButton_c::BUTTON_NONE_e (static_assert en mod.cpp)

enum class UiEvent : std::uint8_t { None = 0, PaneResolved, VisualHidden, LayoutRefused, HudRecreated, Count };
inline const char* name(UiEvent e) noexcept {
  static constexpr const char* n[] = {"NONE", "PANE_RESOLVED", "VISUAL_HIDDEN", "LAYOUT_REFUSED", "HUD_RECREATED"};
  return static_cast<unsigned>(e) < 5 ? n[static_cast<unsigned>(e)] : "NONE";
}
// Lo que el adaptador lee (por valor) del HUD nativo en un punto de control. Las "keys" son valores opacos de igualdad (jamas se desreferencian aqui).
struct UiInput {
  bool meter = false;     // dMeter2_c vivo (el del hook o dMeter2Info_getMeterClass)
  bool prompt = false;    // mpEmpButton != nullptr
  bool screen = false;    // mpButtonScreen != nullptr
  bool aligned = false;   // punteros alineados a 8
  bool vptr_ok = false;   // vptr de mpButtonScreen == vptr de mpMeterDraw->mpScreen == vptr de mpScreen del propio prompt (los tres J2DScreen)
  bool visible = false;   // J2DPane::mVisible de la raiz ahora
  std::uint64_t meter_key = 0, prompt_key = 0;
};
struct UiDecision {
  bool hide = false;
  UiEvent ev = UiEvent::None;
};
struct UiCounters {
  std::uint64_t checks = 0, no_meter = 0, no_prompt = 0, refused = 0, hide_writes = 0, already_hidden = 0, hidden_checks = 0, prompts_seen = 0, meters_seen = 0,
                hud_recreated = 0, masks = 0, mask_restores = 0, mask_foreign = 0, mask_refused = 0, mask_unpaired = 0;
};
class ActionUi {
 public:
  UiCounters c{};
  bool enabled = false;
  // Un punto de control (POST de dMeter2_c::_execute, POST de _draw y PRE del draw del prompt). Idempotente: ocultar ya oculto no escribe.
  UiDecision step(const UiInput& in) noexcept {
    UiDecision d;
    if (!enabled) return d;
    ++c.checks;
    if (!in.meter) {
      ++c.no_meter;
      meter_key_ = 0, prompt_key_ = 0, refused_key_ = 0;
      return d;
    }
    if (in.meter_key != meter_key_) {
      if (meter_key_) ++c.hud_recreated, d.ev = UiEvent::HudRecreated;
      meter_key_ = in.meter_key;
      ++c.meters_seen;
      prompt_key_ = 0, refused_key_ = 0;
    }
    if (!in.prompt) {
      ++c.no_prompt;
      prompt_key_ = 0;
      return d;
    }
    if (!in.screen || !in.aligned || !in.vptr_ok) {  // FAIL_CLOSED: puntero, alineacion o tipo inesperados => no se escribe nada
      ++c.refused;
      if (refused_key_ != in.prompt_key) refused_key_ = in.prompt_key, d.ev = UiEvent::LayoutRefused;
      return d;
    }
    if (in.prompt_key != prompt_key_) {
      prompt_key_ = in.prompt_key;
      ++c.prompts_seen;
      if (d.ev == UiEvent::None) d.ev = UiEvent::PaneResolved;
    }
    if (in.visible) {
      d.hide = true;
      ++c.hide_writes;
      if (d.ev == UiEvent::None) d.ev = UiEvent::VisualHidden;
    } else {
      ++c.already_hidden;
    }
    ++c.hidden_checks;
    return d;
  }
  // El HUD nativo se destruye (PRE de dMeter2_c::_delete): se olvida todo lo ligado a el.
  void forget() noexcept { meter_key_ = 0, prompt_key_ = 0, refused_key_ = 0; }
  bool layout_ok_for(std::uint64_t prompt_key) const noexcept { return prompt_key && prompt_key == prompt_key_; }

 private:
  std::uint64_t meter_key_ = 0, prompt_key_ = 0, refused_key_ = 0;
};

// Mascara de los slots de brillo durante UN draw del prompt vivo: begin en el PRE, end en el POST del MISMO objeto. Sin anidamiento.
class SparkleMask {
 public:
  struct Begin {
    bool apply = false;
    std::uint8_t s0 = 0, s1 = 0;  // valores a escribir (kButtonNone)
  };
  struct End {
    bool restore = false;
    std::uint8_t s0 = 0, s1 = 0;  // valores originales
  };
  Begin begin(bool live_prompt, bool layout_ok, std::uint64_t key, std::uint8_t s0, std::uint8_t s1, UiCounters& c) noexcept {
    Begin b;
    if (on_) ++c.mask_unpaired, on_ = false;  // un PRE sin POST anterior: nunca se restaura a ciegas
    if (!live_prompt) {
      ++c.mask_foreign;  // otro dMeterButton_c (p. ej. el aviso de salto de cinematica): intacto
      return b;
    }
    if (!layout_ok) {
      ++c.mask_refused;
      return b;
    }
    on_ = true, key_ = key, s0_ = s0, s1_ = s1;
    b.apply = true, b.s0 = kButtonNone, b.s1 = kButtonNone;
    ++c.masks;
    return b;
  }
  End end(std::uint64_t key, UiCounters& c) noexcept {
    End e;
    if (!on_ || key != key_) return e;
    on_ = false;
    e.restore = true, e.s0 = s0_, e.s1 = s1_;
    ++c.mask_restores;
    return e;
  }
  bool active() const noexcept { return on_; }

 private:
  bool on_ = false;
  std::uint64_t key_ = 0;
  std::uint8_t s0_ = 0, s1_ = 0;
};

// ======================================== 5. telemetria (lineas acotadas, sin direcciones) ==========================================
inline int fmt_block(char* out, std::size_t n, const Contact& k, const Outcome& o, int pass_proc, bool fresh) noexcept {
  return std::snprintf(out, n,
                       "LNK01_BLOCK event=%s post08=%s attacker_id=%u generation=%llu attack_token=%llu variant=%s id_method=%s e_oc2=%d atp=%d tick=%llu "
                       "before=%d.%03d after=%d.%03d weight=%d contact_fresh=%d blocked=%d tg_match=%d link_state=%s pass_proc=%d saturated=%d",
                       name(o.v), post08::name(k.result), k.id, static_cast<unsigned long long>(k.generation), static_cast<unsigned long long>(k.token),
                       name(o.vid.v), name(o.vid.m), k.e_oc2 ? 1 : 0, k.atp, static_cast<unsigned long long>(k.tick), o.before / policy::kMilli,
                       o.before % policy::kMilli, o.after / policy::kMilli, o.after % policy::kMilli, o.weight, fresh ? 1 : 0, k.blocked ? 1 : 0,
                       k.tg_match ? 1 : 0, !k.link_ok ? "INVALID" : (k.link_human ? "HUMAN" : "NOT_HUMAN"), pass_proc, o.saturated ? 1 : 0);
}
inline int fmt_direct(char* out, std::size_t n, const DirectEvent& d) noexcept {
  return std::snprintf(out, n,
                       "LNK_S1_DIRECT event=%s attacker_id=%u generation=%llu attack_token=%llu contact_tick=%llu confirm_tick=%llu evidence=%s "
                       "damage_timer=%d life_before=%d life_now=%d pending_life=%.2f variant=%s id_method=%s weight=%d before=%d.%03d after=%d.%03d saturated=%d "
                       "attribution=%s dedup=TOKEN_PER_ATTACK",
                       name(d.r), d.id, static_cast<unsigned long long>(d.generation), static_cast<unsigned long long>(d.token),
                       static_cast<unsigned long long>(d.contact_tick), static_cast<unsigned long long>(d.confirm_tick),
                       d.r == DirectResult::Confirmed ? "TG_FIRST_CYLINDER_MATCH+DAMAGE_TIMER_EDGE+LIFE_LOSS" : "INCOMPLETE", d.timer, d.life_before,
                       d.life_now, static_cast<double>(d.pending_life), name(d.vid.v), name(d.vid.m), d.weight, d.before / policy::kMilli,
                       d.before % policy::kMilli, d.after / policy::kMilli, d.after % policy::kMilli, d.saturated ? 1 : 0,
                       d.r == DirectResult::Confirmed ? "UNIQUE" : (d.r == DirectResult::Ambiguous ? "AMBIGUOUS_NOT_CREDITED" : "NOT_CREDITED"));
}
inline int fmt_balance(char* out, std::size_t n, const VariantId& v, bool direct, int weight, std::uint32_t id, std::uint64_t generation) noexcept {
  const policy::Weight base = policy::kWeights[static_cast<unsigned>(v.v) < 5 ? static_cast<unsigned>(v.v) : 4u];
  return std::snprintf(out, n,
                       "LNK_S1_BALANCE variant=%s id_method=%s atp=%d hit=%s base_weight=%d special=%s adjust=%d applied=%d fury=%d attacker_id=%u generation=%llu",
                       name(v.v), name(v.m), v.atp, direct ? "DIRECT" : "BLOCK", direct ? base.direct : base.block,
                       v.group_power && v.v == policy::Variant::Blue ? "EXPANSION_GROUP_POWER_X2" : "NONE", weight - (direct ? base.direct : base.block), weight,
                       v.v == policy::Variant::BlackFury ? 1 : 0, id, static_cast<unsigned long long>(generation));
}
}  // namespace lnk01
