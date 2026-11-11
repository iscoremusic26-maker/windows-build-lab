#pragma once
// LIGHT FADES TWICE 0.12.5 (FIX03 P1-3, repaired by FIX04, completed by FIX05): a CONFIRMED parry keeps the game's own impact spark and metallic clash, only orange and higher.
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
//  * The guard / clash sound: daAlink_c::setGuardSe -> Z2CreatureLink::startCollisionSE and E_DN def_se_set (NOT hookable: rel8 branch in its first 5 bytes)
//    -> Z2CreatureEnemy::startCollisionSE, both tail-jumping into Z2SoundObjBase::startCollisionSE, which returns the Z2SoundHandlePool of THIS playback; JAISoundParamsMove::movePitch(v, 0) sets that playback's
//    pitch at once, before mDoAud_Execute renders it. The sample and every other sound stay untouched.
//  * FIX05, why half the clash stayed blue [S libs/JSystem/src/JParticle/JPAResource.cpp, JPAParticle.cpp, src/dusk/interp/particle.cpp]: a resource with per-particle colour animation rewrites each particle's
//    colour from the key frames in calc_p, AFTER executeAfter. The PC port then captures that colour at the end of calc_p and, on every presentation frame, draws a scratch copy built from the captured
//    colours: the emitter draw callback writes into the live particle and is never seen. So a spark drawn from emitter colours turned orange and a spark with per-particle colours stayed blue.
//    The fix makes calc_p itself read orange: executeAfter points the work data (rewritten by the next emitter's calc) at a private copy of this resource whose colour tables are orange. The shared resource is never written.
//  * Child particles without colour inheritance are born from the child shape's colours (the same private copy serves) and are registered from them at draw by JPARegistChildPrmEnv, which runs just before the emitter draw callback of the child pass.
//  * dKy_SordFlush_set(pos, 0) is the green-blue environment flash of the HITNG mark (d_kankyo.cpp flush_col); type 1 is the game's own warm flash. A parry contact uses type 1.
#include <cstdint>

namespace pfx {
inline constexpr const char* kVersion = "0.12.5";
inline constexpr float kPitch = 1.22f;          // FIX05: +22% frequency (~+3.4 semitones): x1.12 was barely audible to the user; requested trial range x1.20..x1.25, same sample
inline constexpr unsigned kHueNum = 47, kHueDen = 100;  // H = 28.2 deg of #FF8A20 as the fraction 28.2/60 of HSV sector 0 (red -> yellow)
inline constexpr unsigned kChromaPct = 10, kSatFloorPct = 75;  // a colour with at least 10% saturation is a colour (not white/grey): it is given at least 75%, so a pale blue never becomes a pale cream
inline constexpr unsigned kRecords = 16, kEmitters = 8;  // a Lizalfos parry is 2+2 native emitters; siblings of the same clash fit
inline constexpr std::uint64_t kConfirmTicks = 2, kKeepTicks = 300;
inline constexpr std::uint64_t kClashTicks = 4;   // a mark between the same two actors this soon after a parry belongs to that clash (the native mark throttle is 5 ticks; the next swing of a double is at least 6 away)
inline constexpr unsigned kClones = 6, kClrMax = 256, kPending = 8, kSeenRes = 8;

struct Rgb {
  std::uint8_t r = 0, g = 0, b = 0;
};
// HSV hue replacement: V (max channel) is kept and the hue moves to orange (sector 0: R = V, B = min, G between); S is kept when strong, raised to the floor when pale.
// Grey and white stay exactly as they are; idempotent (orange(orange(c)) == orange(c)), so a repeated calc never drifts.
inline Rgb orange(Rgb c) noexcept {
  const unsigned mx = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
  unsigned mn = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
  if ((mx - mn) * 100u >= mx * kChromaPct && mx != 0) {
    const unsigned cap = mx * (100u - kSatFloorPct) / 100u;
    if (mn > cap) mn = cap;
  }
  Rgb o;
  o.r = static_cast<std::uint8_t>(mx);
  o.b = static_cast<std::uint8_t>(mn);
  o.g = static_cast<std::uint8_t>(mn + ((mx - mn) * kHueNum + kHueDen / 2) / kHueDen);
  return o;
}

// FIX05: the native colour route of a resource, read from it at capture. Pure so the decision is testable.
struct ResFlags {
  bool glbl = false, prm_anm = false, env_anm = false, child = false, child_color_inherit = false, child_alpha_out = false, child_alpha_inherit = false;
  unsigned tev_sel = 1, tex_fmt = 0, entries = 0;
};
inline bool per_particle_colors(const ResFlags& f) noexcept { return !f.glbl && (f.prm_anm || f.env_anm); }
inline bool needs_clone(const ResFlags& f) noexcept { return per_particle_colors(f) || f.child; }
inline bool child_static(const ResFlags& f) noexcept { return f.child && !f.child_alpha_out && !f.child_alpha_inherit && !f.child_color_inherit; }
inline bool hitng_mark(unsigned type) noexcept { return type == 2 || type == 5 || type == 6 || type == 8 || type == 9; }  // the marks dPa_control_c::setHitMark draws with ID_ZI_J_HITNG_A/B, the blue spark
enum class Risk : std::uint8_t { None = 0, TableTooLarge, NoCloneSlot, TextureColor, TevTextureOnly, Count };
inline const char* name(Risk r) noexcept {
  static constexpr const char* n[] = {"NONE", "UNMAPPED_TABLE_TOO_LARGE", "UNMAPPED_NO_CLONE_SLOT", "TEXTURE_BLUE_UNRESOLVED", "TEXTURE_ONLY_TEV_UNRESOLVED"};
  return unsigned(r) < unsigned(Risk::Count) ? n[unsigned(r)] : "?";
}
// GX formats 0..3 are intensity (I4/I8/IA4/IA8): no hue of their own, the tint is the whole colour. Tev selection 0 is TEXC alone: the instance colours are not used at all.
inline Risk risk_of(const ResFlags& f, bool clone_ok) noexcept {
  if (needs_clone(f) && !clone_ok) return f.entries > kClrMax ? Risk::TableTooLarge : Risk::NoCloneSlot;
  if (f.tev_sel == 0) return Risk::TevTextureOnly;
  if (f.tex_fmt > 3) return Risk::TextureColor;
  return Risk::None;
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
  std::uint64_t clones = 0, per_particle_emitters = 0, child_static_draws = 0, blue_after_calc = 0, siblings = 0, retro = 0, flashes = 0, risk = 0, block_marks = 0;
  std::uint64_t emitters = 0, emitter_overflow = 0, sounds = 0, sound_reverted = 0, orphan_callbacks = 0, no_emitter = 0, no_sound = 0, neutral_emitters = 0;
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
  // FIX05: a still orange parry of this actor a moment ago: its other marks are part of that same clash.
  int exchange(std::uint32_t actor, std::uint64_t now) const noexcept {
    for (unsigned i = 0; i < kRecords; ++i)
      if ((r[i].st == St::Preview || r[i].st == St::Confirmed) && r[i].actor == actor && now >= r[i].pass && now - r[i].pass <= kClashTicks) return int(i);
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
