#pragma once
// CP10.6B pure policy (no game headers): acquisition of the WindLine aura emitter, tolerant of the engine's transient refusals.
//
// RESOURCE: ID_AK_JN_WINDLINE00 = 0x0046, `AK_JN_windLine00.jpa`, COMMON particle bank (no 0x8000 bit; 0x0046 and 0x8046 are different resources,
// bank 0 vs the scene bank). The CP10.6A glow ID_ZF_J_FAIRY00_GLOW (0x072F) is retired from this path and must never be passed to the engine.
//
// WHY RETRIES: dPa_control_c::set() returns NULL (no emitter) when the actor's tevStr is not initialised yet (Material_use_fg != 0x7C; d_particle.cpp
// L1481), when the emitter pool is exhausted or the resource manager is missing. CP10.6A treated the first NULL as a permanent fault for the whole
// actor generation (seen in a real log as BLUE_VISUAL_REJECTED PARTICLE_SET_RETURNED_NO_EMITTER). Here each of these is a bounded transient: the
// logical state stays on, the acquisition is retried every tick for kRetryTicks (2 s of game time) and only then rejected for the generation.
//
// The fade/ownership state machine is the unchanged CP10.6A `devf_vis::step`; this header only provides its aura half (`AuraPart`) over a tiny
// engine adapter `En`, so the mod and the lab run the SAME code:
//   bool probe(const char*& why)                      once per session: common bank loaded and resource present (and its metadata logged by the mod)
//   bool ready()                                      the actor's tevStr is initialised
//   std::uint32_t set(std::uint16_t id,key,x,y,z)    engine keyed set (creates or refreshes + moves); 0 = no emitter
//   bool configure(key,const Tuning&,alpha,created,Readback&)   look the emitter up and apply scale/colour/alpha; false = not found
//   void stop(std::uint32_t key)                      stop drawing; nobody refreshes it any more, the engine removes it
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include <array>
#include <cstdint>
namespace devf_wl {
using devf_variant::Variant;
constexpr std::uint16_t kResource=0x0046,kRetiredGlow=0x072F;
constexpr unsigned kBank=0,kRetryTicks=60,kMaxEmitters=devf_vis::kMaxAuras;
static_assert((kResource&0x8000)==0&&kBank==0,"common bank: 0x0046 is not the scene-bank 0x8046");
static_assert(kResource!=kRetiredGlow,"the CP10.6A glow is retired from this path");
// First-round values: ONE table to tune after the first human round. Alpha boost: WindLine strokes are thin, so the CP10.6A alpha caps (110/130)
// are scaled up (per mille). Colour is applied per emitter (global prm/env multiply the particle colour in the draw code); scale is the emitter scale.
struct Tuning {int r,g,b,er,eg,eb;float scale;int alpha_boost_pm;};
constexpr Tuning kBlue{110,190,255,20,70,140,1.5f,1800};
constexpr Tuning kBlack{185,90,255,60,20,120,1.5f,1800};
constexpr const Tuning& tuning_for(Variant v) noexcept {return v==Variant::Black?kBlack:kBlue;}
constexpr int boosted_alpha(Variant v,int alpha) noexcept {const long a=long(alpha)*tuning_for(v).alpha_boost_pm/1000;return a>255?255:a<0?0:int(a);}
struct Retry {std::uint32_t id=0,generation=0;unsigned fails=0;};
// Side table indexed like the visual ledger, stamped with id+generation: a new generation never inherits a retry count.
template<std::size_t N> inline Retry& retry_for(std::array<Retry,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 Retry& r=table[index<N?index:0];if(r.id!=id||r.generation!=generation){r=Retry{};r.id=id;r.generation=generation;}return r;
}
struct Readback {bool ok=false;int r=0,g=0,b=0,a=0;};
struct Note {enum Kind:std::uint8_t {None,RetryStart,Acquired,ColorApplied,ColorUnavailable,Exhausted} kind=None;const char* why="NONE";unsigned fails=0;std::uint32_t key=0;Readback color;};
struct Notes {Note n[4];unsigned count=0;void add(const Note& x) noexcept {if(count<4)n[count++]=x;}};
template<class En> struct AuraPart {
 En& en;Retry& rt;const char* why="NONE";Notes notes;float x=0,y=0,z=0;
 AuraPart(En& e,Retry& r) noexcept : en(e),rt(r) {}
 const char* reason() const noexcept {return why;}
 bool aura(devf_vis::Entry& e,int,int alpha) noexcept {
  auto transient=[&](const char* w)->bool{
   why=w;++rt.fails;
   if(rt.fails>kRetryTicks){Note n;n.kind=Note::Exhausted;n.why=w;n.fails=rt.fails;notes.add(n);return false;}   // permanent for this generation
   if(rt.fails==1){Note n;n.kind=Note::RetryStart;n.why=w;n.fails=1;notes.add(n);}
   return true;                                                                                                // logical state stays on; no emitter yet
  };
  const char* pw="NONE";
  if(!en.probe(pw))return transient(pw);
  if(!en.ready())return transient("TEVSTR_NOT_READY_YET");
  const std::uint32_t previous=e.key;
  const std::uint32_t key=en.set(kResource,previous,x,y,z);   // the ONLY resource id this path ever passes
  if(!key){e.key=0;return transient("PARTICLE_SET_RETURNED_NO_EMITTER");}
  const bool created=key!=previous;
  Readback cb;
  if(!en.configure(key,tuning_for(e.variant),boosted_alpha(e.variant,alpha),created,cb)){e.key=0;return transient("EMITTER_LOOKUP_FAILED");}
  e.key=key;
  if(created){
   if(rt.fails){Note n;n.kind=Note::Acquired;n.why="ACQUIRED_AFTER_RETRY";n.fails=rt.fails;n.key=key;notes.add(n);}
   Note c;c.kind=cb.ok?Note::ColorApplied:Note::ColorUnavailable;c.why=cb.ok?"PER_EMITTER_GLOBAL_PRM_ENV_ALPHA_READBACK_MATCHES":"GLOBAL_COLOUR_READBACK_MISMATCH";c.key=key;c.color=cb;notes.add(c);
  }
  rt.fails=0;
  return true;
 }
 void aura_release(devf_vis::Entry& e) noexcept {if(e.key){en.stop(e.key);e.key=0;}rt.fails=0;}
};
} // namespace devf_wl
