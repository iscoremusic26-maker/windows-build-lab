#pragma once
// CP12 (0.9.0) changes on top of 0.8.9, nothing else:
//  * BLACK aura colour: violet -> intense red (global prm 255,32,24 / env 130,0,6). Same route, geometry, alpha, fades: only the colour.
//  * TEVSTR WAIT: dPa_control_c::set refuses an emitter while the actor's tevStr is not initialised, and the Bokoblin initialises it only in its
//    DRAW (daE_OC_c::draw -> settingTevStruct -> Material_use_fg = 124, d_kankyo.cpp 3717). An actor that is not drawn (off camera) therefore
//    stays "not ready". The human 0.8.9 log shows Blues 454/455, 772/773 and 1574 exhausting the 60-tick budget this way and then losing their
//    aura for the rest of their life. Waiting for the first draw is a field read (no engine call), so it no longer consumes the retry budget;
//    real engine refusals (no emitter / lookup failure) stay bounded exactly as before.
//
// CP10.6D-FIX02 pure policy (no game headers): ONE body-hugging strokes route shared by Blue and Black (0.8.8).
//
// FIX02 (human round 0.8.7: visible but faint, floating as a loose ring away from the body). Same resource, same canonical frame, same draw basis.
//  * CENTRE: the ring is centred, in XZ, on the midpoint of the Bokoblin's own torso and head collision spheres (mSphs_cc[0] / [1], recomputed by
//    daE_OC_c::cc_set inside execute, read in our POST), i.e. on the hunched body instead of on the feet axis; feet fallback if implausible.
//  * HUGGING BAND: radius 24 x 2.4 = 57.6, births between 41.5 and 57.6 from that centre (0.83 .. 1.15 x the 50-unit torso collider): the strokes
//    straddle the silhouette (some in front of the body, some just outside it) instead of orbiting 58-91 units away.
//  * PRESENCE: global alpha cap 255; the EMITTER's base prm colour (copied from the resource by JPABaseEmitter::init, alpha 150) is set to
//    (255,255,255,255) on this emitter -- the draw multiplies it by the global colour (JPABaseShape.cpp) -- unless the resource animates prm colour,
//    in which case the engine overwrites it (reported as prm_anim=1); rate step 0 (emit every tick); strokes 10 % longer, same width.
//
// (CP10.6D-FIX01:) ONE corrected strokes route shared by Blue and Black, with a NON-DEGENERATE draw basis.
//
// FIX01 (0.8.7): 0.8.6 was invisible because its canonical frame made the quad's draw basis degenerate. JPABaseParticle::init_p sets
// mBaseAxis = Y column of (global rotation x local rotation) = (0,1,0) here; JPADrawDirection / JPADrawRotDirection (shape 4 = direction cross)
// take axisY = normalize(global rotation x local rotation x mLocalDir) (dir type 3, dirTypeEmtrDir) and axisZ = cross(mBaseAxis, axisY), and RETURN
// WITHOUT DRAWING when |axisZ|^2 <= 32 x FLT_EPSILON. With mLocalDir = (0,1,0): cross((0,1,0),(0,1,0)) = 0 -> no quad was ever submitted.
// On Dusklight presentation frames the draw works on an interpolated COPY (dusk::interp::particle::present), so the re-orthogonalised base is not
// written back: the margin must hold EVERY frame with mBaseAxis still (0,1,0). FIX01 tilts the shared stroke direction to (0.10, 0.99499, 0):
// |cross| = 0.10 every frame (|cross|^2 = 0.01, about 2600 x the engine threshold), strokes 5.7 degrees from vertical. The direction does not enter
// the particle BIRTH position (volume offset uses only rotation x scale), so the full horizontal ring of 0.8.6 is unchanged.
//
// (CP10.6D:) ONE corrected strokes route shared by Blue and Black.
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
//   local translation (0,0,0) | local rotation (0,0,0) | local scale (1,1,1) | direction kGeometry.dir (FIX01: 5.7 deg tilt) | sweep 1.0 | min radius 0.64
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
constexpr std::uint16_t resource_for(Variant v) noexcept {return v==Variant::Blue||v==Variant::Black||v==Variant::Red?kResource:std::uint16_t(0);}   // CP14: Red (gold, during its regeneration window) uses the same route
constexpr bool never_used(std::uint16_t id) noexcept {for(auto x:kNeverUsed)if(x==id)return true;return false;}
// Source constant, not a tuning value: radius of the Bokoblin's body collision sphere (d_a_e_oc.cpp cc_set: mSphs_cc[0].SetR(50.0f); head sphere 45).
constexpr float kTorsoColliderRadius=50.0f;
// Sanity bounds for the MEASURED eyePos.y - current.pos.y (not a body height): an uninitialised or glitched eyePos cannot bury or throw the emitter.
constexpr float kMinBodyDy=60.0f,kMaxBodyDy=500.0f;
// THE shared geometry (one instance, used by both variants). With the canonical frame these are exact emitter parameters:
//   volume_scale 2.4   ring radius 24 x 2.4 = 57.6 (FIX02; 0.8.6/0.8.7: 3.8 -> 91.2, read by the user as a loose ring)
//   min_rad_pm  720    spawn band [41.5 .. 57.6] (FIX02; was 58.4 .. 91.2): 0.83 .. 1.15 x the 50-unit torso collider, hugging the silhouette
//   sweep       1.0    full 360-degree circle (the resource's own sweep is logged in RESOURCE_GEOMETRY)
//   width/length 0.70 / 0.60   quads 14 x 180 nominal (SWL_C base size 0.2 x 3.0), thin vertical strokes (FIX02: length 0.55 -> 0.60)
//   height_pm   650    ring centre at 0.65 of the measured eye offset: chest, not waist (0.8.5: 0.58)
//   dir         (0.10, 0.99498744, 0)   unit stroke direction, 5.7 degrees from vertical: keeps cross(mBaseAxis=(0,1,0), dir) at 0.10 (FIX01)
struct Geometry {float volume_scale,width_scale,length_scale,sweep;int min_rad_pm,height_pm;float dir_x,dir_y,dir_z;};
constexpr Geometry kGeometry{2.4f,0.70f,0.60f,1.0f,720,650,0.10f,0.99498744f,0.0f};
static_assert(kGeometry.dir_x*kGeometry.dir_x+kGeometry.dir_y*kGeometry.dir_y+kGeometry.dir_z*kGeometry.dir_z>0.999999f&&
              kGeometry.dir_x*kGeometry.dir_x+kGeometry.dir_y*kGeometry.dir_y+kGeometry.dir_z*kGeometry.dir_z<1.000001f,"unit stroke direction");
constexpr const Geometry& geometry_for(Variant) noexcept {return kGeometry;}   // deliberately the same object for every variant
// The only per-variant visual difference. The engine multiplies the resource colour (255,255,195) by these per emitter.
struct Colour {int r,g,b,er,eg,eb;};
constexpr Colour kBlueColour{80,190,255,30,90,170};      // blue / cyan (FIX02: slightly brighter than 70,170,255)
constexpr Colour kBlackColour{255,32,24,130,0,6};        // CP12: intense red (0.8.9: violet 190,100,255 / 70,25,130); no blue, almost no green -> not pink, not orange
// FIX02 presence: the emitter's own base prm colour (resource: 255,255,195,150) becomes opaque white on THIS emitter; the global colour above tints it.
struct Rgba {int r,g,b,a;};
constexpr Rgba kBasePrm{255,255,255,255};
constexpr unsigned kRateStep=0;                          // emit on every tick (the resource's own rate step is logged)
constexpr Colour kRedGoldColour{255,190,30,150,90,0};   // CP14: gold (Red while its group-regeneration window is open); more green than the Black red, no blue
constexpr const Colour& colour_for(Variant v) noexcept {return v==Variant::Black?kBlackColour:v==Variant::Red?kRedGoldColour:kBlueColour;}
// One alpha curve for both: level 0..100 of the unchanged CP10.6A fade -> 0..255 (FIX02; was 234). Level 10 (first fade-in tick) -> 25.
constexpr int kAlphaCap=255;
constexpr int shared_alpha(int level) noexcept {return level<=0?0:level>=devf_vis::kLevelMax?kAlphaCap:level*kAlphaCap/devf_vis::kLevelMax;}
constexpr float nominal_radius(unsigned volume_size,const Geometry& g) noexcept {return float(volume_size)*g.volume_scale;}
constexpr float nominal_inner(unsigned volume_size,const Geometry& g) noexcept {return nominal_radius(volume_size,g)*float(g.min_rad_pm)/1000.0f;}
constexpr float clampf(float v,float lo,float hi) noexcept {return v<lo?lo:v>hi?hi:v;}
constexpr float body_height(float eye_dy,const Geometry& g) noexcept {return clampf(eye_dy,kMinBodyDy,kMaxBodyDy)*float(g.height_pm)/1000.0f;}
constexpr unsigned kVolumeSize=24;
// FIX02 ring centre in XZ: midpoint of the torso and head collision spheres (real engine data, follows the hunched pose), else the feet.
constexpr float kMaxTorsoOffset=80.0f;                   // sanity bound per axis between that midpoint and current.pos (not a body dimension)
struct Centre {float x=0,z=0;bool torso=false;};
constexpr Centre ring_centre(float fx,float fz,bool ok,float c0x,float c0z,float c1x,float c1z) noexcept {
 if(ok){const float mx=0.5f*(c0x+c1x),mz=0.5f*(c0z+c1z),dx=mx-fx,dz=mz-fz;
  if(dx>=-kMaxTorsoOffset&&dx<=kMaxTorsoOffset&&dz>=-kMaxTorsoOffset&&dz<=kMaxTorsoOffset)return Centre{mx,mz,true};}
 return Centre{fx,fz,false};
}   // SWL_C volume size (RESOURCE_PROBE volume_size=24 in the user's session); used for the nominal numbers in the log only
// ---- draw basis (engine rule, see the FIX01 header): pure arithmetic, used by the mod on READ-BACK emitter values and by the lab ----
constexpr float kJpaEpsilon=32.0f*1.1920929e-7f;                 // JGeometry::TUtil<f32>::epsilon() = 32 x FLT_EPSILON
struct V3 {float x=0,y=0,z=0;};
constexpr V3 cross3(V3 a,V3 b) noexcept {return V3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
constexpr float dot3(V3 a,V3 b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
constexpr float nsqrt(float v) noexcept {if(v<=0.0f)return 0.0f;float r=v>1.0f?v:1.0f;for(int i=0;i<40;++i)r=0.5f*(r+v/r);return r;}   // Newton, no CRT import
constexpr V3 unit3(V3 a) noexcept {const float l=nsqrt(dot3(a,a));return l>0.0f?V3{a.x/l,a.y/l,a.z/l}:V3{};}
struct Basis {V3 base,axis;float dot=0,cross_sq=0,cross_len=0;bool drawable=false;};
// base = mBaseAxis at birth (Y column of global x local rotation); axis_raw = global x local rotation x mLocalDir (normalised here, as JPADrawDirection does).
constexpr Basis draw_basis(V3 base,V3 axis_raw) noexcept {
 Basis b;b.base=base;b.axis=unit3(axis_raw);const V3 z=cross3(base,b.axis);
 b.dot=dot3(base,b.axis);b.cross_sq=dot3(z,z);b.cross_len=nsqrt(b.cross_sq);b.drawable=dot3(b.axis,b.axis)>kJpaEpsilon&&b.cross_sq>kJpaEpsilon;return b;
}
// The configured geometry with the identity global rotation of our emitters (no rotation passed to dPa_control_c::set, local rotation 0).
constexpr Basis configured_basis(const Geometry& g) noexcept {return draw_basis(V3{0.0f,1.0f,0.0f},V3{g.dir_x,g.dir_y,g.dir_z});}
static_assert(configured_basis(kGeometry).drawable&&configured_basis(kGeometry).cross_sq>100.0f*kJpaEpsilon,"FIX01: the shared draw basis is non-degenerate with a wide margin");
static_assert(!draw_basis(V3{0.0f,1.0f,0.0f},V3{0.0f,1.0f,0.0f}).drawable,"0.8.6 basis: parallel axes, the engine would not draw");
struct Retry {std::uint32_t id=0,generation=0;unsigned fails=0,waits=0;};   // waits: ticks spent waiting for the first draw (not budgeted)
// Side table indexed like the visual ledger, stamped with id+generation: a new generation never inherits a retry count.
template<std::size_t N> inline Retry& retry_for(std::array<Retry,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 Retry& r=table[index<N?index:0];if(r.id!=id||r.generation!=generation){r=Retry{};r.id=id;r.generation=generation;}return r;
}
// Read back from the emitter after configure(): `ok` = colour/alpha; `geo_ok` = canonical frame + geometry exactly as requested.
struct Readback {bool ok=false,geo_ok=false,presence_ok=false;int r=0,g=0,b=0,a=0;float min_rad=0,sweep=0;V3 dir;Basis basis;Rgba base_prm{};unsigned rate_step=0;};   // dir / basis: read back from the emitter
struct Applied {float y_off=0,radius=0,inner=0,cdx=0,cdz=0;bool torso=false;};
struct Note {enum Kind:std::uint8_t {None,Requested,RetryStart,Waiting,Acquired,ColorApplied,ColorUnavailable,TuningApplied,DrawBasisCheck,Exhausted} kind=None;const char* why="NONE";unsigned fails=0;std::uint32_t key=0;std::uint16_t resource=0;Readback color;Applied applied;};
struct Notes {Note n[6];unsigned count=0;void add(const Note& x) noexcept {if(count<6)n[count++]=x;}};
template<class En> struct AuraPart {
 En& en;Retry& rt;const char* why="NONE";Notes notes;
 float fx=0,fy=0,fz=0,eye_dy=0;                                          // the actor's feet position and the MEASURED eyePos.y - current.pos.y (set by the caller each tick)
 bool torso_ok=false;float c0x=0,c0z=0,c1x=0,c1z=0;                      // FIX02: torso / head collision sphere centres (XZ) and whether they are usable
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
  if(!previous&&rt.fails==0&&rt.waits==0){Note n;n.kind=Note::Requested;n.why="EMITTER_REQUESTED_FOR_THIS_ACTOR_STATE";n.resource=res;notes.add(n);}   // once per acquisition episode, before any probe/retry
  const char* pw="NONE";
  if(!en.probe(res,pw))return transient(pw);
  if(!en.ready()){                                                                                            // CP12: wait for the first draw, unbudgeted
   why="TEVSTR_NOT_READY_ACTOR_NOT_DRAWN_YET";
   if(++rt.waits==1){Note n;n.kind=Note::Waiting;n.why=why;n.fails=rt.fails;n.resource=res;notes.add(n);}   // once per acquisition episode
   return true;                                                                                              // logical state stays on; no engine call made
  }
  const Geometry& g=geometry_for(e.variant);
  const float h=body_height(eye_dy,g);
  const Centre ctr=ring_centre(fx,fz,torso_ok,c0x,c0z,c1x,c1z);
  const std::uint32_t key=en.set(res,previous,ctr.x,fy+h,ctr.z);                                                  // the ONLY id this path ever passes: 0x0B9D
  if(!key){e.key=0;return transient("PARTICLE_SET_RETURNED_NO_EMITTER");}
  const bool created=key!=previous;
  Readback cb;
  if(!en.configure(key,g,colour_for(e.variant),shared_alpha(level),created,cb)){e.key=0;return transient("EMITTER_LOOKUP_FAILED");}
  e.key=key;
  if(created){
   Note a;a.kind=Note::Acquired;a.why=rt.fails?"ACQUIRED_AFTER_RETRY":rt.waits?"ACQUIRED_AFTER_FIRST_DRAW":"ACQUIRED_FIRST_ATTEMPT";a.fails=rt.fails;a.key=key;a.resource=res;notes.add(a);
   Note c;c.kind=cb.ok?Note::ColorApplied:Note::ColorUnavailable;c.why=cb.ok?"PER_EMITTER_GLOBAL_PRM_ENV_ALPHA_READBACK_MATCHES":"GLOBAL_COLOUR_READBACK_MISMATCH";c.key=key;c.resource=res;c.color=cb;notes.add(c);
   Note t;t.kind=Note::TuningApplied;t.why=cb.geo_ok&&cb.presence_ok?"CANONICAL_FRAME_SHARED_GEOMETRY_AND_PRESENCE_READBACK_MATCHES":"GEOMETRY_OR_PRESENCE_READBACK_MISMATCH";t.key=key;t.resource=res;t.color=cb;
   t.applied.y_off=h;t.applied.radius=nominal_radius(kVolumeSize,g);t.applied.inner=nominal_inner(kVolumeSize,g);t.applied.torso=ctr.torso;t.applied.cdx=ctr.x-fx;t.applied.cdz=ctr.z-fz;notes.add(t);
   Note d;d.kind=Note::DrawBasisCheck;d.why=cb.basis.drawable?"NON_DEGENERATE":"DEGENERATE_QUAD_WOULD_NOT_DRAW";d.key=key;d.resource=res;d.color=cb;notes.add(d);
  }
  rt.fails=0;rt.waits=0;
  return true;
 }
 void aura_release(devf_vis::Entry& e) noexcept {if(e.key){en.stop(e.key);e.key=0;}rt.fails=0;rt.waits=0;}
};
} // namespace devf_stk
