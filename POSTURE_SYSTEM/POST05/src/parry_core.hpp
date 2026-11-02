#pragma once
// POST08 0.8.0: PARRY de precision con el Shield Attack de Link. Logica PURA (biblioteca estandar; sin cabeceras del juego, sin direcciones).
//
// Hechos nativos en los que se apoya (SOURCE_TRACED en el host 40457c6 + ABI_CONFIRMED en el EXE 2.0.3, ver POSTURE_SYSTEM/POST08/IMPLEMENTATION.md):
//  * daAlink_c::setCollision marca los 3 cilindros Tg de Link como ESCUDO frontal mientras checkPlayerGuardAndAttack(), que incluye PROC_GUARD_ATTACK
//    (EXE: call checkPlayerGuardAndAttack en 0x1408cb3b4; cmpw $0x1e,0x4144 en 0x14084f703).
//  * dCcS::SetAtTgGObjInf: si ChkShield() (escudo + rango frontal) el golpe NO suma dano (PlusDmg omitido) y marca AtShieldHit en el ataque y TgShieldHit en
//    Link. Es la decision nativa de bloqueo: el mod no la provoca ni la imita, solo la LEE.
//  * cCcS::ChkAtTg limpia las marcas de impacto de cada colisionador registrado al empezar cada pasada; E_OC registra sus 4 esferas en cada execute (cc_set).
//    => AtHit/AtShieldHit leidos en el PRE de damage_check describen la ULTIMA pasada de colision.
//  * El golpe activo de E_OC: ATTACK (4) estado 1|2 (clip B/C), esferas At activas en los frames 14..22 de su clip.
//  * La ventana del Shield Attack: ANM_SHIELD_ATTACK avanza 1 frame por tick (speed 1.0); su colisionador ofensivo esta activo en frames [1, 12).
// FIX01 (0.8.1) -- CAUSA RAIZ de 0.8.0 (0/79 contactos): el golpe bloqueado en la pasada N hace que Link, en SU execute del tick N+1 (que corre ANTES del
// damage_check del Bokoblin), pase de PROC_GUARD_ATTACK a PROC_GUARD_SLIP (d_a_alink_damage.inc:672-684: modo 0x100 & 0x70C52 == 0, AtSpl 0 => procGuardSlipInit).
// 0.8.0 leia mProcID "ahora" => NORMAL_GUARD. 0.8.1 juzga con el estado de Link CAPTURADO en mod_update del tick N+1: ModLoader::tick corre al inicio de
// fapGm_Execute, antes de fpcM_Management, y la pasada de colision (dScnPly_Draw -> dComIfG_Ccsp()->Move, d_s_play.cpp:557) corre al final del tick N tras
// todos los execute => la instantanea del tick N+1 es el estado de Link DURANTE la pasada N, la misma que produjo las marcas At que lee el PRE del tick N+1.
// Un parry exige, en la MISMA pasada: golpe del enemigo sobre un cilindro Tg de la Link viva + marca nativa de bloqueo por escudo + Link en PROC_GUARD_ATTACK
// con su animacion dentro de los primeros kWindowFrames frames desde el inicio del Shield Attack. Un token por ataque (episodio) impide sumar dos veces.
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace post08 {
inline constexpr const char* kParryVersion = "0.8.1";
inline constexpr unsigned kSlots = 64;  // == post03::kCapacity (static_assert en mod.cpp)

namespace policy {
// 0.8.0 usaba [inicio-1, inicio+5] leyendo a Link AHORA (0,17-0,20 s): nunca llego a evaluarse (ver FIX01 arriba).
// FIX01: con la instantanea exacta ya no hace falta holgura de orden de ejecucion. Ventana = el bloqueo ocurre con la animacion del Shield Attack en
// [inicio - kPreStartSlack, inicio + kWindowFrames]; ANM_SHIELD_ATTACK empieza en el frame 1 y avanza 1 frame/tick (speed 1.0), asi que son los 10 primeros
// ticks del Shield Attack = 0,333 s de anticipacion (pulsar hasta ~1/3 s antes del impacto). Ademas la edad en ticks medida por el rastreador debe ser
// <= kWindowTicksCap (comprobacion cruzada si el ritmo de la animacion no fuera 1.0). [TIMING_PROVISIONAL_POST08_FIX01]
inline constexpr float kWindowFrames = 9.0f;
inline constexpr float kPreStartSlack = 1.0f;
inline constexpr std::uint32_t kWindowTicksCap = 11;
// Black de Expansion en furia: su golpe va a x2 (normal x1.15) y Expansion aplica un "shield chip" de vida a Link sobre CUALQUIER bloqueo nativo de ese
// golpe. El mod no puede impedirlo sin tocar Expansion: con un E_OC2 no-Red a ritmo >= este umbral NO se concede parry (golpe de 0.7.0).
inline constexpr float kFuryRateThreshold = 1.5f;
inline constexpr int kRedAtp = 2;  // E_OC2 con atp 2/2 = Red (POST05-RB); cualquier otro E_OC2 se trata como posible Black
inline constexpr int kModeAttack = 4;
}  // namespace policy

enum class Result : std::uint8_t {
  None = 0,            // ninguna esfera de ESTE enemigo toco a Link en la ultima pasada (caso normal: sin log)
  Confirmed,           // parry
  NotBlocked,          // toco a Link SIN bloqueo nativo de escudo (dano normal o golpe desde atras)
  NormalGuard,         // bloqueado por la guardia normal (no Shield Attack): rebote nativo de 0.7.0
  MissedEarly,         // Shield Attack demasiado pronto: la animacion ya paso la ventana
  MissedLate,          // el golpe ya se habia resuelto contra Link en este ataque antes del Shield Attack
  NotAttacking,        // el enemigo no esta en un golpe activo (ATTACK 1|2)
  Latched,             // postura ya rota (100): sin parry; el escudo hace lo de 0.7.0 (preparacion POST06)
  FuryChipPossible,    // Black en furia: Expansion cobraria chip a Link; no se declara desvio
  Untracked,           // identidad no verificada / congelada / muerta / vida <= 1
  LinkInvalid,         // Link no verificable en esta llamada
  EventRunning,        // evento / cinematica
  Duplicate,           // este ataque ya produjo un parry (dos esferas, callbacks repetidos, frames siguientes)
  NoPassSnapshot,      // FIX01: no hay instantanea de Link de ESTE tick (o es de otra Link): no se puede saber que hacia Link en la pasada
  Count
};
inline constexpr unsigned kResultCount = static_cast<unsigned>(Result::Count);
inline const char* name(Result r) noexcept {
  static constexpr const char* n[] = {"NONE",      "CONFIRMED",  "NOT_BLOCKED", "NORMAL_GUARD", "MISSED_EARLY",  "MISSED_LATE", "NOT_ATTACKING",
                                      "LATCHED",   "FURY_CHIP_POSSIBLE", "UNTRACKED", "LINK_INVALID", "EVENT_RUNNING", "DUPLICATE",
                                      "NO_PASS_SNAPSHOT"};
  return static_cast<unsigned>(r) < kResultCount ? n[static_cast<unsigned>(r)] : "NONE";
}

// Estado de Link en un instante (por valor; sin direcciones).
struct LinkView {
  bool valid = false, guard_attack = false;
  int proc = -1;
  float frame = 0, start = 0, end = 0;
  std::uint64_t serial = 0;     // Shield Attack en curso (0 = ninguno)
  std::uint32_t age_ticks = 0;  // ticks desde que el rastreador vio empezar ese Shield Attack
};

// FIX01: rastreador del Shield Attack de Link, alimentado UNA vez por mod_update. Su instantanea de un tick T describe a Link durante la pasada de colision
// del tick T-1. Solo vale para los PRE del MISMO tick T y para la MISMA Link (id).
class LinkTrack {
 public:
  struct Counters {
    std::uint64_t observed = 0, starts = 0, ends = 0, with_contact = 0, with_block = 0, with_parry = 0, link_changes = 0;
  } c{};
  struct Transition {
    bool started = false, ended = false;
    std::uint64_t serial = 0;
    std::uint32_t duration = 0;
    int exit_proc = -1;
    float last_frame = 0;
    unsigned contacts = 0, blocks = 0, parries = 0;
  };
  Transition observe(std::uint64_t tick, bool valid, std::uint32_t id, int proc, bool guard_attack, float frame, float start, float end) noexcept {
    Transition t;
    ++c.observed;
    tick_ = tick;
    if (!valid || (snap_.valid && id != id_)) {  // sin Link o Link distinta: cierra el Shield Attack abierto y no hereda nada
      if (snap_.valid && id != id_) ++c.link_changes;
      if (open_) t = close(-1);
      snap_ = LinkView{};
      id_ = valid ? id : 0;
      if (!valid) return t;
    }
    id_ = id;
    if (guard_attack && !open_) {
      open_ = true;
      serial_ = ++next_serial_;
      start_tick_ = tick;
      contacts_ = blocks_ = parries_ = 0;
      ++c.starts;
      t.started = true;
      t.serial = serial_;
    } else if (!guard_attack && open_) {
      t = close(proc);
    }
    snap_.valid = true;
    snap_.guard_attack = guard_attack;
    snap_.proc = proc;
    snap_.frame = frame;
    snap_.start = start;
    snap_.end = end;
    snap_.serial = open_ ? serial_ : 0;
    snap_.age_ticks = open_ ? static_cast<std::uint32_t>(tick - start_tick_) : 0;
    last_frame_ = frame;
    return t;
  }
  // La instantanea de la pasada solo para el tick en curso y la misma Link.
  LinkView pass_for(std::uint64_t tick, bool link_ok, std::uint32_t id) const noexcept {
    if (!link_ok || tick != tick_ || id != id_ || !snap_.valid) return LinkView{};
    return snap_;
  }
  void note(std::uint64_t serial, bool contact, bool block, bool parry) noexcept {
    if (!open_ || serial != serial_) return;
    if (contact && contacts_++ == 0) ++c.with_contact;
    if (block && blocks_++ == 0) ++c.with_block;
    if (parry && parries_++ == 0) ++c.with_parry;
  }
  void reset() noexcept {
    open_ = false;
    snap_ = LinkView{};
    id_ = 0;
  }

 private:
  Transition close(int exit_proc) noexcept {
    Transition t;
    t.ended = true;
    t.serial = serial_;
    t.duration = static_cast<std::uint32_t>(tick_ - start_tick_);
    t.exit_proc = exit_proc;
    t.last_frame = last_frame_;
    t.contacts = contacts_, t.blocks = blocks_, t.parries = parries_;
    ++c.ends;
    open_ = false;
    return t;
  }
  LinkView snap_;
  bool open_ = false;
  std::uint32_t id_ = 0;
  std::uint64_t tick_ = 0, serial_ = 0, next_serial_ = 0, start_tick_ = 0;
  float last_frame_ = 0;
  unsigned contacts_ = 0, blocks_ = 0, parries_ = 0;
};

// Todo lo que el adaptador lee (por valor) en el PRE de UNA llamada de damage_check del enemigo.
struct Input {
  // enemigo
  bool tracked = false;  // identidad slot+generacion+id viva en el ledger y admitida
  int slot = -1;
  std::uint64_t generation = 0;
  std::uint32_t id = 0;
  bool latched = false, frozen = false, dead = false;
  int hp = 0, mode = -1, state = -1;
  bool anim_ok = false;
  float anim_frame = 0, anim_rate = 0;
  bool e_oc2 = false;
  int atp = 0;
  // contacto (ultima pasada de colision): por esfera At 0/1
  bool at_hit[2] = {false, false};      // cCcD_ObjHitInf::ChkAtHit (crudo)
  bool at_on_link[2] = {false, false};  // el objeto golpeado es uno de los 3 cilindros Tg de la Link viva (igualdad de direccion)
  bool at_shield[2] = {false, false};   // dCcD_GObjInf::ChkAtShieldHit: el motor bloqueo con escudo (sin dano)
  // Link: identidad viva en ESTA llamada; estado en la PASADA (instantanea de mod_update de este tick) y estado AHORA (solo telemetria)
  bool link_ok = false;
  LinkView pass, now;
  bool event_running = false;
};

struct Verdict {
  Result result = Result::None;
  std::uint64_t token = 0;  // ataque (episodio) del enemigo
  float delta = 0;          // frame de Link EN LA PASADA - inicio del ataque del escudo
  int sphere = -1;
  bool blocked = false;       // marca nativa AtShieldHit en la esfera de contacto
  bool transitioned = false;  // Link estaba en Shield Attack en la pasada y ya no lo esta al leer (p. ej. GUARD_SLIP tras el bloqueo)
  bool first_contact = false;  // primer contacto con Link de este ataque (el adaptador registra solo este y el commit)
};

struct PCounters {
  std::uint64_t judged = 0, contacts = 0, episodes = 0, confirmed = 0, committed = 0, commit_failed = 0, result[kResultCount] = {};
  std::uint64_t log_written = 0, log_dropped = 0;
};

class Parry {
 public:
  PCounters c{};
  bool enabled = false;

  struct Episode {
    bool bound = false, active = false, resolved = false, parried = false, contacted = false;
    std::uint32_t id = 0;
    std::uint64_t generation = 0, token = 0;
    int state = -1;
    float last_frame = 0;
  };
  const Episode& episode(unsigned i) const noexcept { return ep_[i < kSlots ? i : 0]; }

  // Un juicio por llamada PRE. Mantiene el episodio de ataque (token) de la identidad y decide el resultado del contacto de la ultima pasada.
  Verdict judge(const Input& in) noexcept {
    Verdict v;
    if (!enabled) return v;
    ++c.judged;
    if (!in.tracked || in.slot < 0 || static_cast<unsigned>(in.slot) >= kSlots || in.id == 0) {
      v.result = any_contact(in) ? Result::Untracked : Result::None;
      return count(v);
    }
    Episode& e = ep_[in.slot];
    if (!e.bound || e.id != in.id || e.generation != in.generation) {  // identidad nueva o reciclada: nada se hereda
      e = Episode{};
      e.bound = true;
      e.id = in.id;
      e.generation = in.generation;
    }
    const bool swing = in.mode == policy::kModeAttack && (in.state == 1 || in.state == 2);
    if (swing) {
      const bool restart = !e.active || e.state != in.state || (in.anim_ok && in.anim_frame + 0.01f < e.last_frame);
      if (restart) {
        e.active = true;
        e.resolved = false;
        e.parried = false;
        e.contacted = false;
        e.state = in.state;
        e.token = ++next_token_;
        ++c.episodes;
      }
      e.last_frame = in.anim_ok ? in.anim_frame : e.last_frame;
    } else {
      e.active = false;
    }
    v.token = e.active ? e.token : 0;
    int sph = -1, blocked = -1;
    for (int i = 0; i < 2; ++i) {
      if (in.at_hit[i] && in.at_on_link[i]) {
        if (sph < 0) sph = i;
        if (in.at_shield[i] && blocked < 0) blocked = i;
      }
    }
    if (sph < 0) return count(v);  // Result::None
    ++c.contacts;
    v.first_contact = !(e.active && e.contacted);
    if (e.active) e.contacted = true;
    v.sphere = blocked >= 0 ? blocked : sph;
    v.blocked = blocked >= 0;
    v.delta = in.pass.frame - in.pass.start;
    v.transitioned = in.pass.valid && in.pass.guard_attack && !(in.now.valid && in.now.guard_attack);
    // orden: identidad/estado seguro -> golpe activo -> Link -> bloqueo nativo -> Shield Attack -> episodio -> ventana -> postura -> Expansion
    if (in.dead || in.frozen || in.hp <= 1) v.result = Result::Untracked;
    else if (in.event_running) v.result = Result::EventRunning;
    else if (!swing || !e.active) v.result = Result::NotAttacking;
    else if (!in.link_ok) v.result = Result::LinkInvalid;
    else if (!in.pass.valid) v.result = Result::NoPassSnapshot;
    else if (blocked < 0) {
      v.result = Result::NotBlocked;
      e.resolved = true;  // el golpe se resolvio contra Link sin escudo: un Shield Attack posterior en este ataque es tardio
    } else if (!in.pass.guard_attack) {  // FIX01: estado de Link EN LA PASADA del bloqueo, no el de ahora
      v.result = Result::NormalGuard;
      e.resolved = true;
    } else if (e.parried) v.result = Result::Duplicate;
    else if (e.resolved) v.result = Result::MissedLate;
    else if (!in_window(in.pass)) {
      v.result = Result::MissedEarly;
      e.resolved = true;  // el bloqueo ya ocurrio fuera de ventana: el resto de este ataque no puede volverse parry
    } else if (in.latched) v.result = Result::Latched;
    else if (fury_chip_possible(in)) v.result = Result::FuryChipPossible;
    else {
      v.result = Result::Confirmed;
      ++c.confirmed;
    }
    return count(v);
  }

  // El adaptador informa si el parry se aplico (desvio verificado + postura) o fallo; en ambos casos este ataque queda cerrado (fail-closed: sin reintento).
  void commit(int slot, std::uint64_t generation, std::uint32_t id, std::uint64_t token, bool ok) noexcept {
    if (slot < 0 || static_cast<unsigned>(slot) >= kSlots) return;
    Episode& e = ep_[slot];
    if (!e.bound || e.id != id || e.generation != generation || e.token != token || token == 0 || e.parried) return;  // idempotente por token
    e.parried = true;
    e.resolved = true;
    if (ok) ++c.committed;
    else ++c.commit_failed;
  }

  static bool in_window(const LinkView& p) noexcept {
    const float d = p.frame - p.start;
    return p.valid && p.guard_attack && d >= -policy::kPreStartSlack && d <= policy::kWindowFrames && p.age_ticks <= policy::kWindowTicksCap;
  }
  static bool fury_chip_possible(const Input& in) noexcept {
    if (!in.e_oc2 || in.atp == policy::kRedAtp) return false;  // Blue (E_OC) y Red (E_OC2 2/2) nunca reciben chip de Expansion
    return !in.anim_ok || !(in.anim_rate < policy::kFuryRateThreshold);  // ritmo desconocido o >= x1.5: posible furia
  }
  static bool any_contact(const Input& in) noexcept { return (in.at_hit[0] && in.at_on_link[0]) || (in.at_hit[1] && in.at_on_link[1]); }

  bool log_take() noexcept {
    if (logged_ >= 200) {
      ++c.log_dropped;
      return false;
    }
    ++logged_;
    ++c.log_written;
    return true;
  }

 private:
  Verdict count(Verdict v) noexcept {
    ++c.result[static_cast<unsigned>(v.result) < kResultCount ? static_cast<unsigned>(v.result) : 0];
    return v;
  }
  Episode ep_[kSlots]{};
  std::uint64_t next_token_ = 0;
  unsigned logged_ = 0;
};

// Desvio NO rompedor: el golpe termina como ATTACK estado 3 (el estado nativo de "golpe rechazado" de executeAttack, sin esferas At activas) a partir de
// los valores PRE de la llamada. El adaptador solo lo escribe si el estado actual es el PRE intacto o la reaccion nativa del Shield Attack (DAMAGE 3).
struct LiveShape {
  int mode = 0, state = 0, old_mode = 0;
};
inline bool deflect_allowed(const LiveShape& pre, const LiveShape& now) noexcept {
  if (pre.mode != policy::kModeAttack || !(pre.state == 1 || pre.state == 2)) return false;
  const bool untouched = now.mode == pre.mode && now.state == pre.state && now.old_mode == pre.old_mode;
  const bool shield_stun = now.mode == 5 && now.state == 3 && now.old_mode == pre.mode;
  return untouched || shield_stun;
}
inline constexpr int kDeflectState = 3;

// Una linea por oportunidad (contacto) o por commit: identidad, golpe, Link EN LA PASADA y AHORA, timing y resultado. Sin direcciones. < kLogLineMax*2.
inline int fmt_event(char* out, std::size_t n, const Input& in, const Verdict& v, int link_life, int pre_hp, int post_hp, int posture_before, int posture_after,
                     bool crossed, bool stun_suppressed, bool deflected, bool ok, const char* stage) noexcept {
  return std::snprintf(out, n,
                       "POST08_EVENT result=%s stage=%s attacker_id=%u generation=%llu attack_token=%llu sphere=%d blocked=%d pass_valid=%d pass_proc=%d "
                       "pass_shield_attack=%d sa_serial=%llu sa_age_ticks=%u pass_frame=%.2f atk_start=%.2f timing_delta=%.2f window=[-%.0f,+%.0f]f/%ut "
                       "now_proc=%d now_shield_attack=%d transitioned=%d enemy_mode=%d enemy_state=%d enemy_frame=%.2f enemy_rate=%.2f e_oc2=%d atp=%d link_life=%d "
                       "enemy_hp_before=%d enemy_hp_after=%d posture_before=%d posture_after=%d crossed=%d stun_suppressed=%d link_attack_deflected=%d ok=%d",
                       name(v.result), stage, in.id, static_cast<unsigned long long>(in.generation), static_cast<unsigned long long>(v.token), v.sphere,
                       v.blocked ? 1 : 0, in.pass.valid ? 1 : 0, in.pass.proc, in.pass.guard_attack ? 1 : 0, static_cast<unsigned long long>(in.pass.serial),
                       in.pass.age_ticks, in.pass.frame, in.pass.start, v.delta, policy::kPreStartSlack, policy::kWindowFrames, policy::kWindowTicksCap,
                       in.now.proc, in.now.guard_attack ? 1 : 0, v.transitioned ? 1 : 0, in.mode, in.state, in.anim_frame, in.anim_rate, in.e_oc2 ? 1 : 0, in.atp,
                       link_life, pre_hp, post_hp, posture_before, posture_after, crossed ? 1 : 0, stun_suppressed ? 1 : 0, deflected ? 1 : 0, ok ? 1 : 0);
}

// Histograma de timing de los bloqueos ocurridos DURANTE un Shield Attack (para calibrar la ventana con datos humanos).
struct TimingHist {
  static constexpr int kMin = -2, kMax = 20;  // frames de delta; fuera de rango se acumulan en los extremos
  std::uint32_t bin[kMax - kMin + 1] = {};
  void add(float delta) noexcept {
    int d = static_cast<int>(delta < 0 ? delta - 0.5f : delta + 0.5f);
    d = d < kMin ? kMin : (d > kMax ? kMax : d);
    ++bin[d - kMin];
  }
  int fmt(char* out, std::size_t n) const noexcept {  // "d:count,..." solo bins no vacios
    std::size_t w = 0;
    if (n) out[0] = 0;
    for (int d = kMin; d <= kMax; ++d) {
      if (!bin[d - kMin] || w + 16 >= n) continue;
      const int k = std::snprintf(out + w, n - w, "%s%d:%u", w ? "," : "", d, bin[d - kMin]);
      if (k > 0) w += static_cast<std::size_t>(k);
    }
    return static_cast<int>(w);
  }
};
}  // namespace post08
