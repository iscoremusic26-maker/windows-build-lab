#pragma once
// LIGHT FADES TWICE 0.12.3 FIX03 P1-3: a CONFIRMED parry keeps the game's own impact spark and metallic clash, only orange and higher.
// PURE: standard library only. parry_fx_adapter.inc owns the native objects (emitters, sound handles); this file owns the decision bookkeeping.
//
// Native facts [S src/d/d_cc_s.cpp, src/d/d_particle.cpp, libs/JSystem/src/JParticle, src/Z2AudioLib @40457c6]:
//  * A shield block of an enemy At on Link's Tg cylinders runs dCcS::SetAtTgGObjInf -> ProcAtTgHitmark(..., chk_shield) INSIDE the collision pass
//    (dScnPly_Draw -> Ccsp()->Move()), which calls dComIfGp_setHitMark(Link's Tg hit mark 6, Link, ...) -> dPa_control_c::setHitMark: TWO native
//    emitters ID_ZI_J_HITNG_A / ID_ZI_J_HITNG_B (the blue spark) + dKy_SordFlush_set. The same tick then runs particle_calc3D: the spark's first frame
//    is computed BEFORE the next tick's damage verdict. Hence the decision is taken inside the pass, with the SAME pure judges, and confirmed (or
//    reverted, counted as a mismatch) by the authoritative verdict of the next tick.
//  * Link's sword on the Lizalfos tate_sph: ProcAtTgHitmark(tate hit mark 5) in the pass, then E_DN's own Execute setHitMark(2, NULL, ...) on the shield.
//  * JPAResource::calc calls the emitter callback's executeAfter() after the emitter's colour is computed and new particles exist, but BEFORE calc_p
//    (which may rewrite a particle's colour from the resource's key frames). JPAResource::drawP registers the emitter's colour first, then runs the emitter
//    draw callback, then reads each particle's colour. So: executeAfter recolours the emitter (registered next) and draw recolours the particles (read
//    next). Colours come from emtr->mPrmClr/mEnvClr and ptcl->mPrmClr/mEnvClr, multiplied by the instance's global colours. Changing THOSE fields of THIS
//    instance recolours exactly this spark from its first frame: same resource, texture, shape, count, trajectory, size, alpha, animation, duration.
//    Nothing shared (JPAResource) is ever written.
//  * The guard / clash sound: daAlink_c::setGuardSe -> Z2CreatureLink::startCollisionSE and E_DN def_se_set -> startCollisionSE, both ending in
//    Z2SoundObjBase::startCollisionSE, which returns the Z2SoundHandlePool of THIS playback; JAISoundParamsMove::movePitch(v, 0) sets that playback's
//    pitch at once, before mDoAud_Execute renders it. The sample and every other sound stay untouched.
#include <cstdint>

namespace pfx {
inline constexpr const char* kVersion = "0.12.3";
inline constexpr float kPitch = 1.12f;          // +12% frequency (~+2 semitones): inside the requested +10..15%, clearly higher, same sample
inline constexpr unsigned kHueNum = 47, kHueDen = 100;  // H = 28.2 deg of #FF8A20 as the fraction 28.2/60 of HSV sector 0 (red -> yellow)
inline constexpr unsigned kRecords = 16, kEmitters = 4;
inline constexpr std::uint64_t kConfirmTicks = 2, kKeepTicks = 300;

struct Rgb {
  std::uint8_t r = 0, g = 0, b = 0;
};
// HSV hue replacement: V (max channel) and S ((max-min)/max) are kept, only the hue moves to orange (sector 0: R = V, B = min, G between).
// Grey and white stay exactly as they are; idempotent (orange(orange(c)) == orange(c)), so a repeated calc never drifts.
inline Rgb orange(Rgb c) noexcept {
  const unsigned mx = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
  const unsigned mn = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
  Rgb o;
  o.r = static_cast<std::uint8_t>(mx);
  o.b = static_cast<std::uint8_t>(mn);
  o.g = static_cast<std::uint8_t>(mn + ((mx - mn) * kHueNum + kHueDen / 2) / kHueDen);
  return o;
}

enum class Kind : std::uint8_t { None = 0, LinkOnLizalfos, LinkOnEoc, LizalfosOnLink, Count };
inline const char* name(Kind k) noexcept {
  static constexpr const char* n[] = {"NONE", "LINK_PARRY_LIZALFOS", "LINK_PARRY_E_OC", "LIZALFOS_PARRY_LINK"};
  return unsigned(k) < unsigned(Kind::Count) ? n[unsigned(k)] : "?";
}
enum class St : std::uint8_t { Free = 0, Preview, Confirmed, Reverted };

// A record = one physical parry contact: (kind, attacker/defender actor id, collision-pass tick). The adapter attaches native objects by index.
struct Rec {
  St st = St::Free;
  Kind kind = Kind::None;
  std::uint32_t actor = 0;
  std::uint64_t pass = 0, last_seen = 0;
  unsigned emitters = 0;
  bool sound = false;
};
struct Counters {
  std::uint64_t preview[unsigned(Kind::Count)] = {}, confirmed[unsigned(Kind::Count)] = {}, reverted[unsigned(Kind::Count)] = {}, expired = 0, duplicate = 0, full = 0;
  std::uint64_t emitters = 0, emitter_overflow = 0, sounds = 0, sound_reverted = 0, orphan_callbacks = 0, no_emitter = 0, no_sound = 0;
};
class Book {
 public:
  Rec r[kRecords];
  Counters c{};
  // One record per physical contact, whatever number of callbacks / colliders report it. Returns the index or -1.
  int open(Kind k, std::uint32_t actor, std::uint64_t pass) noexcept {
    if (k == Kind::None || actor == 0) return -1;
    const int f = find(k, actor, pass);
    if (f >= 0) {
      ++c.duplicate;
      return f;
    }
    for (unsigned i = 0; i < kRecords; ++i)
      if (r[i].st == St::Free) {
        r[i] = Rec{};
        r[i].st = St::Preview, r[i].kind = k, r[i].actor = actor, r[i].pass = pass, r[i].last_seen = pass;
        ++c.preview[unsigned(k)];
        return int(i);
      }
    ++c.full;
    return -1;
  }
  int find(Kind k, std::uint32_t actor, std::uint64_t pass) const noexcept {
    for (unsigned i = 0; i < kRecords; ++i)
      if (r[i].st != St::Free && r[i].kind == k && r[i].actor == actor && r[i].pass == pass) return int(i);
    return -1;
  }
  // The authoritative verdict of the next tick: true keeps orange/high, false reverts (counted: the preview judged differently).
  void settle(int i, bool parry) noexcept {
    if (i < 0 || unsigned(i) >= kRecords || r[i].st != St::Preview) return;
    r[i].st = parry ? St::Confirmed : St::Reverted;
    ++(parry ? c.confirmed : c.reverted)[unsigned(r[i].kind)];
  }
  bool orange_now(int i) const noexcept { return i >= 0 && unsigned(i) < kRecords && (r[i].st == St::Preview || r[i].st == St::Confirmed); }
  // A preview no verdict ever answered (actor deleted, event) reverts; records free themselves once their emitters stop calling back.
  template <class OnExpire>
  void tick(std::uint64_t now, OnExpire&& on_expire) noexcept {
    for (unsigned i = 0; i < kRecords; ++i) {
      Rec& x = r[i];
      if (x.st == St::Preview && now > x.pass + kConfirmTicks) {
        x.st = St::Reverted;
        ++c.expired;
        ++c.reverted[unsigned(x.kind)];
      }
      if (x.st != St::Free && now > x.last_seen + kKeepTicks) {
        on_expire(int(i));
        x = Rec{};
      }
    }
  }
  void clear() noexcept {
    for (auto& x : r) x = Rec{};
  }
};
}  // namespace pfx
