#pragma once
// CP10.6D pure policy (no game headers): ONE corrected strokes route shared by Blue and Black.
//
// RESOURCE (both variants): 0x0B9D ID_ZI_J_LK_SWL_C, common bank 0, the effect the Black already used (present in the user's session; still probed at
// run time). Blue no longer uses 0x0A58 (SWL_A). SWL_B 0x0A59, WindLine 0x0046 / 0x8046 and the retired glow 0x072F are never passed.
// Blue and Black differ ONLY in colour and in the logic condition that turns the aura on (Blue group bonus / Black fury, unchanged CP10.6 logic).
//
// WHY THE STROKES CLUMPED (engine source, libs/JSystem/src/JParticle, Dusklight 40457c6). Every emitter copies, at creation, the resource's
// sword-frame data (JPABaseEmitter::init): local scale, local translation, local rotation, direction and the circle SWEEP. Particle birth is
//   theta = mVolumeSweep x random_s16                       (JPAVolumeCircle: sweep < 1 -> only an ARC of the circle, a fixed world sector)
//   offset = SR(distance x (sin theta, 0, cos theta))       (SR = global rotation x LOCAL ROTATION x LOCAL SCALE, calcWorkData_c)
//   position = (global translation + global scale x LOCAL TRANSLATION) + offset x global scale
// and every quad is drawn along global rotation x local rotation x DIRECTION (dirTypeEmtrDir). With the emitter's global rotation at identity
// (no sword matrix), any of: sweep < 1, an anisotropic local scale, a non-zero local rotation or local translation, puts the particles in one
// fixed sector / squashed line / tilted plane / offset spot, whatever the ring radius. 0.8.4 controlled none of them, 0.8.5 only the translation.
// CP10.6D therefore writes a CANONICAL FRAME on THIS emitter (never on the shared JPAResource), every tick (idempotent; no key curve drives any
// of these fields here: the two curves are id 0 rate and id 4 lifetime):
//   local translation (0,0,0) | local rotation (0,0,0) | local scale (1,1,1) | direction (0,1,0) | sweep 1.0 (full circle) | min radius 0.64
// so the result depends only on the shared geometry below: a full horizontal ring centred on the actor, vertical strokes, at chest height.
//
// RETRIES (from CP10.6B): dPa_control_c::set() returns NULL when the actor's tevStr is not initialised (Material_use_fg != 0x7C), the pool is full or the
// manager is missing. Each refusal is a bounded retry (kRetryTicks = 60 ticks, 2 s of game time), then a clean rejection for that actor generation.
// The fade/ownership state machine is the unchanged CP10.6A `devf_vis::step`; this header provides its aura half over a small engine adapter `En`:
//   bool probe(std::uint16_t id,const char*& why)       once per session: common bank loaded and THIS id registered (logged by the mod)
//   bool ready()                                         the actor's tevStr is initialised
//   std::uint32_t set(std::uint16_t id,key,x,y,z)        engine keyed set (creates or refreshes + moves); 0 = no emitter
//   bool configure(key,const Geometry&,const Colour&,alpha,created,Readback&)   look the emitter up, apply frame/geometry/colour/alpha; false = not found
//   void stop(std::uint32_t key)                         stop drawing; nobody refreshes it any more, the engine removes it
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include <array>
#include <cstdint>
namespace devf_stk {
using devf_variant::Variant;
constexpr std::uint16_t kResource=0x0B9D;                                       // SWL_C, shared by Blue and Black
constexpr std::uint16_t kNeverUsed[]={0x0A58,0x0A59,0x0046,0x8046,0x072F};      // SWL_A (Blue's old route), SWL_B, WindLine, WindLine scene id, retired glow
constexpr unsigned kBank=0,kRetryTicks=60,kMaxEmitters=devf_vis::kMaxAuras,kEmittersPerActor=1;
static_assert((kResource&0x8000)==0&&kBank==0&&kResource==0x0B9D,"common bank id SWL_C, no 0x8000 bit");
constexpr std::uint16_t resource_for(Variant v) noexcept {return v==Variant::Blue||v==Variant::Black?kResource:std::uint16_t(0);}
constexpr bool never_used(std::uint16_t id) noexcept {for(auto x:kNeverUsed)if(x==id)return true;return false;}
// Source constant, not a tuning value: radius of the Bokoblin's body collision sphere (d_a_e_oc.cpp cc_set: mSphs_cc[0].SetR(50.0f); head sphere 45).
constexpr float kTorsoColliderRadius=50.0f;
// Sanity bounds for the MEASURED eyePos.y - current.pos.y (not a body height): an uninitialised or glitched eyePos cannot bury or throw the emitter.
constexpr float kMinBodyDy=60.0f,kMaxBodyDy=500.0f;
// THE shared geometry (one instance, used by both variants). With the canonical frame these are exact emitter parameters:
//   volume_scale 3.8   ring radius 24 x 3.8 = 91.2 (the resource's volume size 24, RESOURCE_PROBE), about 1.8 x the 50-unit torso collider
//   min_rad_pm  640    spawn band [58.4 .. 91.2]: the inner edge clears the torso collider (1.17 x), nothing is born inside the body
//   sweep       1.0    full 360-degree circle (the resource's own sweep is logged in RESOURCE_GEOMETRY)
//   width/length 0.70 / 0.55   quads 14 x 165 nominal (SWL_C base size 0.2 x 3.0), thin vertical strokes
//   height_pm   650    ring centre at 0.65 of the measured eye offset: chest, not waist (0.8.5: 0.58)
struct Geometry {float volume_scale,width_scale,length_scale,sweep;int min_rad_pm,height_pm;};
constexpr Geometry kGeometry{3.8f,0.70f,0.55f,1.0f,640,650};
constexpr const Geometry& geometry_for(Variant) noexcept {return kGeometry;}   // deliberately the same object for every variant
// The only per-variant visual difference. The engine multiplies the resource colour (255,255,195) by these per emitter.
struct Colour {int r,g,b,er,eg,eb;};
constexpr Colour kBlueColour{70,170,255,20,70,150};      // blue / cyan
constexpr Colour kBlackColour{185,90,255,60,20,120};     // violet / purple
constexpr const Colour& colour_for(Variant v) noexcept {return v==Variant::Black?kBlackColour:kBlueColour;}
// One alpha curve for both (the Black's 0.8.5 peak): level 0..100 of the unchanged CP10.6A fade -> 0..234. Level 10 (first fade-in tick) -> 23.
constexpr int kAlphaCap=234;
constexpr int shared_alpha(int level) noexcept {return level<=0?0:level>=devf_vis::kLevelMax?kAlphaCap:level*kAlphaCap/devf_vis::kLevelMax;}
constexpr float nominal_radius(unsigned volume_size,const Geometry& g) noexcept {return float(volume_size)*g.volume_scale;}
constexpr float nominal_inner(unsigned volume_size,const Geometry& g) noexcept {return nominal_radius(volume_size,g)*float(g.min_rad_pm)/1000.0f;}
constexpr float clampf(float v,float lo,float hi) noexcept {return v<lo?lo:v>hi?hi:v;}
constexpr float body_height(float eye_dy,const Geometry& g) noexcept {return clampf(eye_dy,kMinBodyDy,kMaxBodyDy)*float(g.height_pm)/1000.0f;}
constexpr unsigned kVolumeSize=24;   // SWL_C volume size (RESOURCE_PROBE volume_size=24 in the user's session); used for the nominal numbers in the log only
struct Retry {std::uint32_t id=0,generation=0;unsigned fails=0;};
// Side table indexed like the visual ledger, stamped with id+generation: a new generation never inherits a retry count.
template<std::size_t N> inline Retry& retry_for(std::array<Retry,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 Retry& r=table[index<N?index:0];if(r.id!=id||r.generation!=generation){r=Retry{};r.id=id;r.generation=generation;}return r;
}
// Read back from the emitter after configure(): `ok` = colour/alpha; `geo_ok` = canonical frame + geometry exactly as requested.
struct Readback {bool ok=false,geo_ok=false;int r=0,g=0,b=0,a=0;float min_rad=0,sweep=0;};
struct Applied {float y_off=0,radius=0,inner=0;};
struct Note {enum Kind:std::uint8_t {None,Requested,RetryStart,Acquired,ColorApplied,ColorUnavailable,TuningApplied,Exhausted} kind=None;const char* why="NONE";unsigned fails=0;std::uint32_t key=0;std::uint16_t resource=0;Readback color;Applied applied;};
struct Notes {Note n[6];unsigned count=0;void add(const Note& x) noexcept {if(count<6)n[count++]=x;}};
template<class En> struct AuraPart {
 En& en;Retry& rt;const char* why="NONE";Notes notes;
 float fx=0,fy=0,fz=0,eye_dy=0;                                          // the actor's feet position and the MEASURED eyePos.y - current.pos.y (set by the caller each tick)
 AuraPart(En& e,Retry& r) noexcept : en(e),rt(r) {}
 const char* reason() const noexcept {return why;}
 bool aura(devf_vis::Entry& e,int level,int) noexcept {                   // the per-variant alpha of CP10.6A is ignored on purpose: one shared curve
  const std::uint16_t res=resource_for(e.variant);
  auto transient=[&](const char* w)->bool{
   why=w;++rt.fails;
   if(rt.fails>kRetryTicks){Note n;n.kind=Note::Exhausted;n.why=w;n.fails=rt.fails;n.resource=res;notes.add(n);return false;}   // permanent for this generation
   if(rt.fails==1){Note n;n.kind=Note::RetryStart;n.why=w;n.fails=1;n.resource=res;notes.add(n);}
   return true;                                                                                                // logical state stays on; no emitter yet
  };
  if(!res||never_used(res)){why="NO_PILOT_RESOURCE_FOR_THIS_VARIANT";return false;}                         // Red / unclassified: never requested
  const std::uint32_t previous=e.key;
  if(!previous&&rt.fails==0){Note n;n.kind=Note::Requested;n.why="EMITTER_REQUESTED_FOR_THIS_ACTOR_STATE";n.resource=res;notes.add(n);}   // once per acquisition episode, before any probe/retry
  const char* pw="NONE";
  if(!en.probe(res,pw))return transient(pw);
  if(!en.ready())return transient("TEVSTR_NOT_READY_YET");
  const Geometry& g=geometry_for(e.variant);
  const float h=body_height(eye_dy,g);
  const std::uint32_t key=en.set(res,previous,fx,fy+h,fz);                                                  // the ONLY id this path ever passes: 0x0B9D
  if(!key){e.key=0;return transient("PARTICLE_SET_RETURNED_NO_EMITTER");}
  const bool created=key!=previous;
  Readback cb;
  if(!en.configure(key,g,colour_for(e.variant),shared_alpha(level),created,cb)){e.key=0;return transient("EMITTER_LOOKUP_FAILED");}
  e.key=key;
  if(created){
   Note a;a.kind=Note::Acquired;a.why=rt.fails?"ACQUIRED_AFTER_RETRY":"ACQUIRED_FIRST_ATTEMPT";a.fails=rt.fails;a.key=key;a.resource=res;notes.add(a);
   Note c;c.kind=cb.ok?Note::ColorApplied:Note::ColorUnavailable;c.why=cb.ok?"PER_EMITTER_GLOBAL_PRM_ENV_ALPHA_READBACK_MATCHES":"GLOBAL_COLOUR_READBACK_MISMATCH";c.key=key;c.resource=res;c.color=cb;notes.add(c);
   Note t;t.kind=Note::TuningApplied;t.why=cb.geo_ok?"CANONICAL_FRAME_AND_SHARED_GEOMETRY_READBACK_MATCHES":"GEOMETRY_READBACK_MISMATCH";t.key=key;t.resource=res;t.color=cb;
   t.applied.y_off=h;t.applied.radius=nominal_radius(kVolumeSize,g);t.applied.inner=nominal_inner(kVolumeSize,g);notes.add(t);
  }
  rt.fails=0;
  return true;
 }
 void aura_release(devf_vis::Entry& e) noexcept {if(e.key){en.stop(e.key);e.key=0;}rt.fails=0;}
};
} // namespace devf_stk
