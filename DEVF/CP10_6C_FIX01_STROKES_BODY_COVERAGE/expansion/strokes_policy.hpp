#pragma once
// CP10.6C-FIX01 pure policy (no game headers): acquisition of the "light-sword strokes" aura emitter, ONE native emitter per actor, now with a
// body-coverage geometry (ring around the torso instead of a ring that the body mostly hides).
//
// RESOURCES (unchanged from CP10.6C; common bank 0, user indices WITHOUT the 0x8000 bit; probed at run time, both found in the user's session):
//   BLUE  during the group bonus : 0x0A58 ID_ZI_J_LK_SWL_A
//   BLACK during the fury        : 0x0B9D ID_ZI_J_LK_SWL_C
// SWL_B (0x0A59), the missing WindLine 0x0046 / 0x8046 and the retired glow 0x072F are never passed. No sword matrix, model or bone is used.
//
// WHY A NEW GEOMETRY (all from the exact engine source, libs/JSystem/src/JParticle, Dusklight 40457c6):
//  * Circle volume (type 5): spawn offset = size x (minRad + r x (1 - minRad)) in the emitter's XZ plane, rotated by the emitter rotation and multiplied by
//    the GLOBAL DYNAMICS scale (JPAParticle::init_p: position = mGlobalPos + SR(volumePos) x mPublicScale). The resource's minRad is not logged; with
//    minRad = 0 a large part of the particles is born INSIDE the body and is hidden by the model (the Bokoblin's own torso collider radius is 50,
//    d_a_e_oc.cpp cc_set, mSphs_cc[0].SetR(50.0f)). FIX01 sets a per-emitter minRad so every particle is born OUTSIDE that radius.
//  * The ring centre is mGlobalPos = globalTranslation + globalScale x resource LOCAL translation (calcWorkData_c). Raising the dynamics scale
//    multiplies that offset too, so FIX01 subtracts it (read from the resource, inline getters) from the position handed to the engine: the ring
//    centre lands on the requested body point whatever the resource's local translation is (no-op when it is 0).
//  * Direction type 3 (dirTypeEmtrDir) orients every quad along the emitter direction (resource direction x resource rotation). If the resource
//    direction is not vertical and its local rotation is zero, FIX01 sets a vertical direction on THIS emitter (setDirection); otherwise it leaves it.
//  * keyNum==2 (ids 0 = rate, 4 = lifetime) are re-applied to mRate/mLifeTime EVERY tick (JPAResource::calcKey), so a permanent emission patch is neither
//    needed nor possible from here: density comes from the curves; FIX01 only moves, sizes and centres the same particles.
// Particle width and length are scaled independently (JPABaseEmitter::setGlobalParticleScale(x,y): quad X = width, Y = along the emitter direction).
//
// RETRIES (from CP10.6B): dPa_control_c::set() returns NULL when the actor's tevStr is not initialised (Material_use_fg != 0x7C), the pool is full or the
// manager is missing. Each refusal is a bounded retry (kRetryTicks = 60 ticks, 2 s of game time), then a clean rejection for that actor generation.
// The fade/ownership state machine is the unchanged CP10.6A `devf_vis::step`; this header provides its aura half over a small engine adapter `En`:
//   bool probe(std::uint16_t id,const char*& why)       once per id and session: common bank loaded and THIS id registered (logged by the mod)
//   const Facts& facts(std::uint16_t id)                scalars of the resource read once at probe time (local translation, direction, rotation, minRad, size)
//   bool ready()                                         the actor's tevStr is initialised
//   std::uint32_t set(std::uint16_t id,key,x,y,z)        engine keyed set (creates or refreshes + moves); 0 = no emitter
//   bool configure(key,const Tuning&,const Plan&,alpha,created,Readback&)   look the emitter up and apply scales/colour/alpha/minRad/direction; false = not found
//   void stop(std::uint32_t key)                         stop drawing; nobody refreshes it any more, the engine removes it
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include <array>
#include <cstdint>
namespace devf_stk {
using devf_variant::Variant;
constexpr std::uint16_t kResourceBlue=0x0A58,kResourceBlack=0x0B9D;
constexpr std::uint16_t kNeverUsed[]={0x0A59,0x0046,0x8046,0x072F};          // SWL_B, WindLine (absent), WindLine scene id, retired glow
constexpr unsigned kBank=0,kRetryTicks=60,kMaxEmitters=devf_vis::kMaxAuras,kEmittersPerActor=1;
static_assert((kResourceBlue&0x8000)==0&&(kResourceBlack&0x8000)==0&&kBank==0,"common bank ids, no 0x8000 bit");
static_assert(kResourceBlue!=kResourceBlack&&kResourceBlue==0x0A58&&kResourceBlack==0x0B9D,"A for Blue, C for Black");
constexpr std::uint16_t resource_for(Variant v) noexcept {return v==Variant::Blue?kResourceBlue:v==Variant::Black?kResourceBlack:std::uint16_t(0);}
constexpr bool never_used(std::uint16_t id) noexcept {for(auto x:kNeverUsed)if(x==id)return true;return false;}
// Source constant, not a tuning value: radius of the Bokoblin's body collision sphere (d_a_e_oc.cpp cc_set: mSphs_cc[0].SetR(50.0f); head sphere 45).
constexpr float kTorsoColliderRadius=50.0f;
// Sanity bounds for MEASURED values (not body dimensions): eyePos.y - current.pos.y is clamped into this range before it is used, so an uninitialised or
// animation-glitched eyePos can neither bury the emitter in the floor nor throw it far away; a compensation larger than kCompLimit per axis is clamped.
constexpr float kMinBodyDy=60.0f,kMaxBodyDy=500.0f,kCompLimit=400.0f,kVerticalDirMinSq=0.81f;   // |dir.y| >= 0.9 of the direction length, compared squared (no sqrt import)
// ONE table for both variants (same spatial criterion; only colour differs). Spatial values, all justified in IMPLEMENTATION.md:
//   volume_scale 4.2   ring radius 24 x 4.2 = 100.8 units (the resource's volume size is 24, log RESOURCE_PROBE)
//   min_rad_pm  600    spawn band [60.5 .. 100.8]: inner edge = 1.21 x the 50-unit torso collider, so no particle is born inside the torso collider
//   width_scale 0.70   quad width  20 x 0.70 = 14 units  (CP10.6C: 9)
//   length_scale 0.55  quad length 280 x 0.55 = 154 (Blue) / 300 x 0.55 = 165 (Black) units (CP10.6C: 126 / 135)
//   height_pm   580    emitter height = current.pos.y + 0.58 x (eyePos.y - current.pos.y)  (CP10.6C: 0.50)
//   alpha_boost_pm 1800 unchanged from CP10.6C (peak 198 Blue / 234 Black); the fade-in samples 19 / 23 seen in the log are tick 1 of the 10-tick fade-in
struct Tuning {int r,g,b,er,eg,eb;float volume_scale,width_scale,length_scale;int min_rad_pm,height_pm,alpha_boost_pm;};
constexpr Tuning kBlue{70,170,255,20,70,150,4.2f,0.70f,0.55f,600,580,1800};     // resource prm (255,255,195,120) x tint -> cyan/blue
constexpr Tuning kBlack{185,90,255,60,20,120,4.2f,0.70f,0.55f,600,580,1800};    // resource prm (255,255,195,150) x tint -> violet
constexpr const Tuning& tuning_for(Variant v) noexcept {return v==Variant::Black?kBlack:kBlue;}
constexpr int boosted_alpha(Variant v,int alpha) noexcept {const long a=long(alpha)*tuning_for(v).alpha_boost_pm/1000;return a>255?255:a<0?0:int(a);}
// Nominal geometry (volume size is the resource's, 24 for both). NOMINAL: not rendered dimensions (curves, texture and projection change what is seen).
constexpr float nominal_radius(unsigned volume_size,const Tuning& t) noexcept {return float(volume_size)*t.volume_scale;}
constexpr float nominal_inner(unsigned volume_size,const Tuning& t) noexcept {return nominal_radius(volume_size,t)*float(t.min_rad_pm)/1000.0f;}
constexpr float absf(float v) noexcept {return v<0.0f?-v:v;}
constexpr float clampf(float v,float lo,float hi) noexcept {return v<lo?lo:v>hi?hi:v;}
// Scalars of the loaded resource, read once at probe time with inline getters (no import, no resource modification).
struct Facts {float tx=0,ty=0,tz=0, dx=0,dy=1,dz=0, min_rad=0;bool rot_zero=true;unsigned volume_size=24;};
// What to apply to ONE emitter (decided from the resource facts and the tuning; pure, testable).
struct Plan {float cx=0,cy=0,cz=0;bool comp_clamped=false,lock_dir=false,dir_nonvertical_kept=false;float min_rad=0;};
constexpr float comp_axis(float local,float scale,bool& clamped) noexcept {
 const float v=0.0f-local*scale;const float c=clampf(v,-kCompLimit,kCompLimit);if(c!=v)clamped=true;return c;
}
constexpr Plan plan_for(const Facts& f,const Tuning& t) noexcept {
 Plan p;
 p.cx=comp_axis(f.tx,t.volume_scale,p.comp_clamped);p.cy=comp_axis(f.ty,t.volume_scale,p.comp_clamped);p.cz=comp_axis(f.tz,t.volume_scale,p.comp_clamped);
 const float len2=f.dx*f.dx+f.dy*f.dy+f.dz*f.dz;
 const bool vertical=len2>0.0f&&f.dy*f.dy>=kVerticalDirMinSq*len2;          // a zero vector is not vertical
 p.lock_dir=!vertical&&f.rot_zero;                       // identity local rotation: the emitter direction IS the stroke axis, so a vertical one is exact
 p.dir_nonvertical_kept=!vertical&&!f.rot_zero;          // rotated resource: the world axis is unknown from here, leave it alone (reported)
 const float want=float(t.min_rad_pm)/1000.0f;
 p.min_rad=f.min_rad>want?f.min_rad:want;                // never narrower than the resource's own hole
 return p;
}
// Emitter height above the actor's feet for a MEASURED eye offset (clamped into the sanity range).
constexpr float body_height(float eye_dy,const Tuning& t) noexcept {return clampf(eye_dy,kMinBodyDy,kMaxBodyDy)*float(t.height_pm)/1000.0f;}
struct Retry {std::uint32_t id=0,generation=0;unsigned fails=0;};
// Side table indexed like the visual ledger, stamped with id+generation: a new generation never inherits a retry count.
template<std::size_t N> inline Retry& retry_for(std::array<Retry,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 Retry& r=table[index<N?index:0];if(r.id!=id||r.generation!=generation){r=Retry{};r.id=id;r.generation=generation;}return r;
}
// Read back from the emitter after configure(): `ok` is the colour/alpha check of CP10.6C; `geo_ok` says scales, minRad and (when requested) direction match.
struct Readback {bool ok=false,geo_ok=false;int r=0,g=0,b=0,a=0;float min_rad=0;};
struct Applied {float y_off=0,radius=0,inner=0,cx=0,cy=0,cz=0;Plan plan;};
struct Note {enum Kind:std::uint8_t {None,Requested,RetryStart,Acquired,ColorApplied,ColorUnavailable,TuningApplied,Exhausted} kind=None;const char* why="NONE";unsigned fails=0;std::uint32_t key=0;std::uint16_t resource=0;Readback color;Applied applied;};
struct Notes {Note n[6];unsigned count=0;void add(const Note& x) noexcept {if(count<6)n[count++]=x;}};
template<class En> struct AuraPart {
 En& en;Retry& rt;const char* why="NONE";Notes notes;
 float fx=0,fy=0,fz=0,eye_dy=0;                                          // the actor's feet position and the MEASURED eyePos.y - current.pos.y (set by the caller each tick)
 AuraPart(En& e,Retry& r) noexcept : en(e),rt(r) {}
 const char* reason() const noexcept {return why;}
 bool aura(devf_vis::Entry& e,int,int alpha) noexcept {
  const std::uint16_t res=resource_for(e.variant);
  auto transient=[&](const char* w)->bool{
   why=w;++rt.fails;
   if(rt.fails>kRetryTicks){Note n;n.kind=Note::Exhausted;n.why=w;n.fails=rt.fails;n.resource=res;notes.add(n);return false;}   // permanent for this generation
   if(rt.fails==1){Note n;n.kind=Note::RetryStart;n.why=w;n.fails=1;n.resource=res;notes.add(n);}
   return true;                                                                                                // logical state stays on; no emitter yet
  };
  if(!res||never_used(res)){why="NO_PILOT_RESOURCE_FOR_THIS_VARIANT";return false;}                         // Red / unclassified / a forbidden id: never requested
  const std::uint32_t previous=e.key;
  if(!previous&&rt.fails==0){Note n;n.kind=Note::Requested;n.why="EMITTER_REQUESTED_FOR_THIS_ACTOR_STATE";n.resource=res;notes.add(n);}   // once per acquisition episode, before any probe/retry
  const char* pw="NONE";
  if(!en.probe(res,pw))return transient(pw);
  if(!en.ready())return transient("TEVSTR_NOT_READY_YET");
  const Tuning& t=tuning_for(e.variant);const Facts& f=en.facts(res);const Plan pl=plan_for(f,t);
  const float h=body_height(eye_dy,t);
  const std::uint32_t key=en.set(res,previous,fx+pl.cx,fy+h+pl.cy,fz+pl.cz);                                 // the ONLY ids this path ever passes: 0x0A58 / 0x0B9D
  if(!key){e.key=0;return transient("PARTICLE_SET_RETURNED_NO_EMITTER");}
  const bool created=key!=previous;
  Readback cb;
  if(!en.configure(key,t,pl,boosted_alpha(e.variant,alpha),created,cb)){e.key=0;return transient("EMITTER_LOOKUP_FAILED");}
  e.key=key;
  if(created){
   Note a;a.kind=Note::Acquired;a.why=rt.fails?"ACQUIRED_AFTER_RETRY":"ACQUIRED_FIRST_ATTEMPT";a.fails=rt.fails;a.key=key;a.resource=res;notes.add(a);
   Note c;c.kind=cb.ok?Note::ColorApplied:Note::ColorUnavailable;c.why=cb.ok?"PER_EMITTER_GLOBAL_PRM_ENV_ALPHA_READBACK_MATCHES":"GLOBAL_COLOUR_READBACK_MISMATCH";c.key=key;c.resource=res;c.color=cb;notes.add(c);
   Note g;g.kind=Note::TuningApplied;g.why=cb.geo_ok?"SCALES_MINRAD_DIRECTION_READBACK_MATCHES":"GEOMETRY_READBACK_MISMATCH";g.key=key;g.resource=res;g.color=cb;
   g.applied.y_off=h;g.applied.radius=nominal_radius(f.volume_size,t);g.applied.inner=nominal_inner(f.volume_size,t);g.applied.cx=pl.cx;g.applied.cy=pl.cy;g.applied.cz=pl.cz;g.applied.plan=pl;notes.add(g);
  }
  rt.fails=0;
  return true;
 }
 void aura_release(devf_vis::Entry& e) noexcept {if(e.key){en.stop(e.key);e.key=0;}rt.fails=0;}
};
} // namespace devf_stk
