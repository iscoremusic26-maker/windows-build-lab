#pragma once
// LNK01 0.9.0: postura PROPIA de Link (Sekiro-style) + barra inferior + supresion VISUAL de los prompts de accion inferiores. Logica PURA (biblioteca
// estandar y nucleos locales; ninguna cabecera del juego ni del SDK, ninguna direccion desreferenciada, sin memoria dinamica).
//
// Fuente de la postura: SOLO el veredicto de POST08 sobre un contacto FRESCO (ArmTrack) de un E_OC/E_OC2 admitido, en la misma llamada de damage_check:
//   NORMAL_GUARD / MISSED_EARLY / LATCHED / FURY_CHIP_POSSIBLE = el motor bloqueo el golpe con el escudo de Link (AtShieldHit nativo) y NO fue un parry
//   => bloqueo ordinario (+kGain, provisional). CONFIRMED/DUPLICATE (parry) => +0 (LNK03). STALE_CONTACT => nada. NOT_BLOCKED / NOT_ATTACKING => nada (LNK02).
//   Un credito por (slot, id, generacion, token de ataque): dos esferas, callbacks repetidos, el rebote nativo o una marca reciclada no suman dos veces.
// Reloj: UN tick por mod_update (tick de simulacion, 30 Hz nominal), nunca por frame de presentacion; congelado en pausa/evento/menu/sin Link.
// LNK01 NO escribe en Link, en la vida, en el contador nativo de guardia (field_0x2fd5) ni en ningun actor: el contador vive en esta memoria privada.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "hud_core.hpp"
#include "parry_core.hpp"

namespace lnk01 {
inline constexpr const char* kVersion = "0.9.0";
inline constexpr unsigned kSlots = post08::kSlots;  // un registro de atacante por slot del ledger (64)

namespace policy {
inline constexpr int kMaxPoints = 100;  // player_posture_max (saturado)
inline constexpr int kMilli = 1000;     // aritmetica entera en milesimas de punto (sin deriva)
inline constexpr int kMaxMilli = kMaxPoints * kMilli;
// Tabla de pesos por tipo de atacante, preparada para diferenciar Blue/Red/Black (LNK07). LNK01 usa UN peso de prueba comun.
enum class Kind : std::uint8_t { Blue = 0, Red = 1, OtherE_OC2 = 2, Count = 3 };
inline constexpr int kGainPoints[3] = {20, 20, 20};  // [TEST_WEIGHT_PROVISIONAL] bloqueo ordinario fisico
inline constexpr int kParryCostPoints = 0;            // LNK03 integrara el coste propio del desvio
inline constexpr int kRedAtp = post08::policy::kRedAtp;
inline constexpr std::uint32_t kRegenDelayTicks = 30;  // ~1 s a 30 Hz tras el ultimo bloqueo valido
inline constexpr int kRegenPointsPerSecond = 10;       // en neutral, vencido el retraso
inline constexpr int kTicksPerSecond = 30;             // reloj de simulacion nominal (game_clock time scale 1)
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

inline policy::Kind kind_of(bool e_oc2, int atp) noexcept {
  if (!e_oc2) return policy::Kind::Blue;
  return atp == policy::kRedAtp ? policy::Kind::Red : policy::Kind::OtherE_OC2;
}
inline int gain_milli(policy::Kind k) noexcept {
  const unsigned i = static_cast<unsigned>(k);
  return (i < 3 ? policy::kGainPoints[i] : 0) * policy::kMilli;
}

// ======================================== 1. postura de Link ========================================================================
enum class Verdict : std::uint8_t {
  None = 0,          // ningun contacto de este atacante con Link en la ultima pasada
  Confirmed,         // CONFIRMED_NATIVE_BLOCK: bloqueo ordinario real => +gain
  RejectedStale,     // registro At de una pasada no armada (FIX02)
  RejectedParry,     // parry POST08 (o episodio ya desviado): +0 para Link en LNK01
  RejectedNonBlock,  // contacto sin bloqueo nativo / sin golpe activo (dano directo = LNK02)
  Duplicate,         // este ataque ya produjo su credito (segunda esfera, rebote, callbacks, golpe ya resuelto)
  RejectedUntracked, // identidad del atacante o del episodio no demostrada
  RejectedLink,      // Link no valida / no humana / distinta de la ligada
  RejectedEvent,     // evento / cinematica
  Count
};
inline constexpr unsigned kVerdictCount = static_cast<unsigned>(Verdict::Count);
inline const char* name(Verdict v) noexcept {
  static constexpr const char* n[] = {"NONE",      "CONFIRMED_NATIVE_BLOCK", "REJECTED_STALE",      "REJECTED_PARRY", "REJECTED_NONBLOCK",
                                      "DUPLICATE", "REJECTED_UNTRACKED",     "REJECTED_LINK_STATE", "REJECTED_EVENT"};
  return static_cast<unsigned>(v) < kVerdictCount ? n[static_cast<unsigned>(v)] : "NONE";
}

// Lo que el adaptador copia (por valor) del veredicto POST08 de UNA llamada de damage_check, despues de post08_step (parry ya comprometido o no).
struct Contact {
  post08::Result result = post08::Result::None;
  bool blocked = false;  // AtShieldHit nativo en la esfera de contacto
  std::uint64_t token = 0;
  int slot = -1;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  bool episode_parried = false;  // post08::Parry::episode(slot).parried para ESTA identidad
  bool e_oc2 = false;
  int atp = 0;
  bool link_ok = false, link_human = false;
  std::uint32_t link_id = 0;
  bool event_running = false;
  std::uint64_t tick = 0;
};

struct Outcome {
  Verdict v = Verdict::None;
  policy::Kind kind = policy::Kind::Blue;
  int before = 0, after = 0;  // milesimas
  bool saturated = false;     // llego a 100 con este bloqueo
};

struct RegenEvent {
  bool started = false, zero = false, paused_edge = false, resumed_edge = false;
  int before = 0, after = 0;
};

struct PCounters {
  std::uint64_t contacts = 0, verdict[kVerdictCount] = {}, by_kind[3] = {}, gained_milli = 0, regen_milli = 0, regen_ticks = 0, active_ticks = 0,
                paused_ticks = 0, regen_starts = 0, saturations = 0, zeros = 0, resets = 0, link_changes = 0, binds = 0;
};

class Posture {
 public:
  PCounters c{};

  // UNA vez por mod_update: identidad de la Link viva. Sin Link o Link distinta => reinicio determinista (0, sin herencia).
  void observe_link(bool valid, std::uint32_t id) noexcept {
    if (!valid || id == 0) {
      if (bound_) reset(false);
      return;
    }
    if (!bound_ || id != link_id_) {
      if (bound_) ++c.link_changes;
      reset(true);
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
    o.kind = kind_of(k.e_oc2, k.atp);
    o.before = o.after = cur_;
    o.v = classify(k);
    ++c.verdict[static_cast<unsigned>(o.v) < kVerdictCount ? static_cast<unsigned>(o.v) : 0];
    if (o.v != Verdict::Confirmed) return o;
    const int g = gain_milli(o.kind);
    cur_ = cur_ + g > policy::kMaxMilli ? policy::kMaxMilli : cur_ + g;
    c.gained_milli += static_cast<std::uint64_t>(cur_ - o.before);
    ++c.by_kind[static_cast<unsigned>(o.kind) % 3];
    since_ = 0;
    acc_ = 0;
    regenerating_ = false;
    o.after = cur_;
    o.saturated = o.before < policy::kMaxMilli && cur_ == policy::kMaxMilli;
    if (o.saturated) ++c.saturations;
    return o;
  }

  // UN tick de simulacion (mod_update). active = Link valida, sin pausa, sin evento, sin menu. Nada se acumula mientras no esta activo.
  RegenEvent tick(bool active) noexcept {
    RegenEvent ev;
    ev.before = ev.after = cur_;
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
    if (!regenerating_) regenerating_ = true, ++c.regen_starts, ev.started = true;
    acc_ += policy::kRegenPointsPerSecond * policy::kMilli;  // milesimas por segundo, repartidas por tick con resto exacto
    int d = acc_ / policy::kTicksPerSecond;
    acc_ -= d * policy::kTicksPerSecond;
    if (d > cur_) d = cur_;
    cur_ -= d;
    c.regen_milli += static_cast<std::uint64_t>(d);
    ++c.regen_ticks;
    if (cur_ == 0) {
      ++c.zeros;
      ev.zero = true;
      acc_ = 0;
      regenerating_ = false;
    }
    ev.after = cur_;
    return ev;
  }

  int milli() const noexcept { return cur_; }
  bool bound() const noexcept { return bound_; }
  std::uint32_t link_id() const noexcept { return link_id_; }
  std::uint64_t epoch() const noexcept { return epoch_; }
  std::uint32_t since_block() const noexcept { return since_; }
  bool regenerating() const noexcept { return regenerating_; }
  bool paused() const noexcept { return paused_; }
  // Invariante de conservacion: lo ganado menos lo regenerado es exactamente el valor actual mas lo perdido en reinicios.
  bool conserved() const noexcept { return c.gained_milli == c.regen_milli + static_cast<std::uint64_t>(cur_) + lost_milli_; }
  std::uint64_t lost_milli() const noexcept { return lost_milli_; }

 private:
  struct L {
    bool bound = false;
    std::uint32_t id = 0;
    std::uint64_t generation = 0, credited = 0, parried = 0;
  };
  L* entry(const Contact& k) noexcept {
    if (k.slot < 0 || static_cast<unsigned>(k.slot) >= kSlots || k.id == 0) return nullptr;
    L& e = led_[k.slot];
    if (!e.bound || e.id != k.id || e.generation != k.generation) e = L{true, k.id, k.generation, 0, 0};  // identidad nueva o reciclada: nada se hereda
    return &e;
  }
  Verdict classify(const Contact& k) noexcept {
    using R = post08::Result;
    L* e = entry(k);
    switch (k.result) {
      case R::StaleContact: return Verdict::RejectedStale;
      case R::Confirmed:
      case R::Duplicate:
        if (e && k.token) e->parried = k.token;  // el resto de este ataque nunca es un bloqueo ordinario
        return Verdict::RejectedParry;
      case R::NotBlocked:
      case R::NotAttacking: return Verdict::RejectedNonBlock;
      case R::MissedLate: return Verdict::Duplicate;
      case R::EventRunning: return Verdict::RejectedEvent;
      case R::NormalGuard:
      case R::MissedEarly:
      case R::Latched:
      case R::FuryChipPossible: break;
      default: return Verdict::RejectedUntracked;  // Untracked, LinkInvalid, NoPassSnapshot
    }
    if (!k.blocked) return Verdict::RejectedNonBlock;
    if (k.event_running) return Verdict::RejectedEvent;
    if (!k.link_ok || !k.link_human || !bound_ || k.link_id != link_id_) return Verdict::RejectedLink;
    if (!e || k.token == 0) return Verdict::RejectedUntracked;
    if (k.episode_parried || e->parried == k.token) return Verdict::RejectedParry;
    if (e->credited == k.token) return Verdict::Duplicate;
    e->credited = k.token;
    return Verdict::Confirmed;
  }
  void reset(bool rebind) noexcept {
    lost_milli_ += static_cast<std::uint64_t>(cur_);
    cur_ = 0, acc_ = 0, since_ = 0xFFFFFFFFu, regenerating_ = false, paused_ = false;
    bound_ = false, link_id_ = 0;
    for (L& e : led_) e = L{};
    ++c.resets;
    (void)rebind;
  }
  L led_[kSlots]{};
  bool bound_ = false, regenerating_ = false, paused_ = false;
  std::uint32_t link_id_ = 0, since_ = 0xFFFFFFFFu;
  std::uint64_t epoch_ = 0, lost_milli_ = 0;
  int cur_ = 0, acc_ = 0;
};

// ======================================== 2. barra HUD de Link (display only) =======================================================
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

// ======================================== 3. supresion VISUAL de los prompts de accion inferiores =====================================
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

// ======================================== 4. telemetria (lineas acotadas, sin direcciones) ==========================================
inline int fmt_block(char* out, std::size_t n, const Contact& k, const Outcome& o, int pass_proc, bool fresh) noexcept {
  return std::snprintf(out, n,
                       "LNK01_BLOCK event=%s post08=%s attacker_id=%u generation=%llu attack_token=%llu kind=%s e_oc2=%d atp=%d tick=%llu before=%d.%03d after=%d.%03d "
                       "gain=%d contact_fresh=%d blocked=%d link_state=%s pass_proc=%d saturated=%d",
                       name(o.v), post08::name(k.result), k.id, static_cast<unsigned long long>(k.generation), static_cast<unsigned long long>(k.token),
                       o.kind == policy::Kind::Blue ? "BLUE" : (o.kind == policy::Kind::Red ? "RED" : "E_OC2_OTHER"), k.e_oc2 ? 1 : 0, k.atp,
                       static_cast<unsigned long long>(k.tick), o.before / policy::kMilli, o.before % policy::kMilli, o.after / policy::kMilli,
                       o.after % policy::kMilli, gain_milli(o.kind) / policy::kMilli, fresh ? 1 : 0, k.blocked ? 1 : 0,
                       !k.link_ok ? "INVALID" : (k.link_human ? "HUMAN" : "NOT_HUMAN"), pass_proc, o.saturated ? 1 : 0);
}
}  // namespace lnk01
