#pragma once
// CP10.6C pure policy (no game headers): acquisition of the "light-sword strokes" aura emitter, one native emitter per actor.
//
// RESOURCES (common bank 0, user indices WITHOUT the 0x8000 bit; verified present in the user's real session inventory, but still probed at run time):
//   BLUE  during the group bonus : 0x0A58 ID_ZI_J_LK_SWL_A
//   BLACK during the fury        : 0x0B9D ID_ZI_J_LK_SWL_C
// The family is Link's light-sword glow (d_a_alink_effect.inc setLightningSwordEffect, three emitters A/B/C attached to the sword matrix with
// setGlobalRTMatrix). The resource itself does not depend on the sword: JPABaseEmitter::init leaves the global rotation at identity and the keyed
// dPa_control_c::set moves the emitter to the position we give every tick; this module never uses a sword matrix, a sword model or Link bones.
// SWL_B (0x0A59) is NOT part of this pilot. The retired CP10.6A glow 0x072F, the missing WindLine 0x0046 and the scene-bank 0x8046 are never passed.
// A strictly-from-source hypothesis (NOT a visual fact): circle volume 5 in the emitter's local XZ plane (radius = volume size 24 x dynamics scale),
// directional quads 20x280 units (vertices +-50 x base size 0.2x2.8) moving along local +Y: thin vertical strokes rising from a ring.
//
// RETRIES (from CP10.6B): dPa_control_c::set() returns NULL when the actor's tevStr is not initialised (Material_use_fg != 0x7C), the pool is full or the
// manager is missing. Each refusal is a bounded retry (kRetryTicks = 60 ticks, 2 s of game time), then a clean rejection for that actor generation.
// The fade/ownership state machine is the unchanged CP10.6A `devf_vis::step`; this header provides its aura half over a small engine adapter `En`:
//   bool probe(std::uint16_t id,const char*& why)       once per id and session: common bank loaded and THIS id registered (logged by the mod)
//   bool ready()                                         the actor's tevStr is initialised
//   std::uint32_t set(std::uint16_t id,key,x,y,z)        engine keyed set (creates or refreshes + moves); 0 = no emitter
//   bool configure(key,const Tuning&,alpha,created,Readback&)   look the emitter up and apply scale/colour/alpha; false = not found
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
// First-round values: ONE table to tune after the first human round. volume_scale multiplies the emission volume and velocities (JPA dynamics scale:
// ring radius 24 x 3 = 72, about the width of a Bokoblin); particle_scale multiplies the particle size (20x280 x 0.45 = 9x126 units). Colour is applied
// per emitter (the draw code multiplies the particle colour by the emitter's global colour and alpha). Alpha: the CP10.6A caps (110/130) boosted x1.8.
struct Tuning {int r,g,b,er,eg,eb;float volume_scale,particle_scale;int alpha_boost_pm;};
constexpr Tuning kBlue{70,170,255,20,70,150,3.0f,0.45f,1800};     // resource prm (255,255,195,120) x tint -> cyan/blue
constexpr Tuning kBlack{185,90,255,60,20,120,3.0f,0.45f,1800};    // resource prm (255,255,195,150) x tint -> violet
constexpr const Tuning& tuning_for(Variant v) noexcept {return v==Variant::Black?kBlack:kBlue;}
constexpr int boosted_alpha(Variant v,int alpha) noexcept {const long a=long(alpha)*tuning_for(v).alpha_boost_pm/1000;return a>255?255:a<0?0:int(a);}
struct Retry {std::uint32_t id=0,generation=0;unsigned fails=0;};
// Side table indexed like the visual ledger, stamped with id+generation: a new generation never inherits a retry count.
template<std::size_t N> inline Retry& retry_for(std::array<Retry,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 Retry& r=table[index<N?index:0];if(r.id!=id||r.generation!=generation){r=Retry{};r.id=id;r.generation=generation;}return r;
}
struct Readback {bool ok=false;int r=0,g=0,b=0,a=0;};
struct Note {enum Kind:std::uint8_t {None,Requested,RetryStart,Acquired,ColorApplied,ColorUnavailable,Exhausted} kind=None;const char* why="NONE";unsigned fails=0;std::uint32_t key=0;std::uint16_t resource=0;Readback color;};
struct Notes {Note n[5];unsigned count=0;void add(const Note& x) noexcept {if(count<5)n[count++]=x;}};
template<class En> struct AuraPart {
 En& en;Retry& rt;const char* why="NONE";Notes notes;float x=0,y=0,z=0;
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
  const std::uint32_t key=en.set(res,previous,x,y,z);                                                           // the ONLY ids this path ever passes: 0x0A58 / 0x0B9D
  if(!key){e.key=0;return transient("PARTICLE_SET_RETURNED_NO_EMITTER");}
  const bool created=key!=previous;
  Readback cb;
  if(!en.configure(key,tuning_for(e.variant),boosted_alpha(e.variant,alpha),created,cb)){e.key=0;return transient("EMITTER_LOOKUP_FAILED");}
  e.key=key;
  if(created){
   Note a;a.kind=Note::Acquired;a.why=rt.fails?"ACQUIRED_AFTER_RETRY":"ACQUIRED_FIRST_ATTEMPT";a.fails=rt.fails;a.key=key;a.resource=res;notes.add(a);
   Note c;c.kind=cb.ok?Note::ColorApplied:Note::ColorUnavailable;c.why=cb.ok?"PER_EMITTER_GLOBAL_PRM_ENV_ALPHA_READBACK_MATCHES":"GLOBAL_COLOUR_READBACK_MISMATCH";c.key=key;c.resource=res;c.color=cb;notes.add(c);
  }
  rt.fails=0;
  return true;
 }
 void aura_release(devf_vis::Entry& e) noexcept {if(e.key){en.stop(e.key);e.key=0;}rt.fails=0;}
};
} // namespace devf_stk
