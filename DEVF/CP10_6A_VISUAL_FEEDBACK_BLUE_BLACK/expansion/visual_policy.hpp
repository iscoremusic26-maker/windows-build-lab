#pragma once
// CP10.6A pure visual policy (no game headers): a small fade state machine per effect channel, driven by the APPROVED CP10.6 logic state.
//   BLUE : aura while the group bonus is on (devf_dyn::Entry::group).
//   BLACK: aura + purple eyes while the fury is on (devf_dyn::Entry::fury > 0).
// The visual layer never recomputes gameplay: it only reads `group` / `fury`, so BLACK_FURY_REFRESH (fury renewed while already on)
// changes nothing here, and nothing starts from the HP initialisation, from regeneration or from a stagger.
//
// FADE: an integer level 0..100 per channel. Target on: +10 per tick (10 ticks in). Target off: -7 per tick (15 ticks out). A channel that is
// re-targeted during a fade continues from its CURRENT level (never restarts from zero). Level 0 releases the effect.
//   state: Inactive (target off, level 0), FadingIn (target on, level < 100), Active (target on, level 100), FadingOut (target off, level > 0).
// Effects are reached through a small `Fx` adapter so the mod and the lab run the same code:
//   bool aura(Entry&,int level,int alpha)  create (when not live) or refresh, set colour alpha; false = rejected (Fx::reason())
//   void aura_release(Entry&)               stop drawing and forget the handle (the engine removes an emitter nobody refreshes)
//   bool eye(Entry&,unsigned idx)           idx 0 = native texture, 1..7 = private purple variants; false = ownership/contract failure
//   const char* reason() const
#include "../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include <array>
#include <cstdint>
namespace devf_vis {
using devf_variant::Variant;
constexpr int kLevelMax=100,kStepIn=10,kStepOut=7;
constexpr unsigned kEyeVariants=8,kCapacity=64,kMaxAuras=8,kEventsPerActor=24;
static_assert(kLevelMax/kStepIn==10&&(kLevelMax+kStepOut-1)/kStepOut==15,"10 ticks in, 15 ticks out");
enum class State:std::uint8_t {Inactive,FadingIn,Active,FadingOut};
constexpr State state_of(int level,bool target) noexcept {return target?(level>=kLevelMax?State::Active:State::FadingIn):(level>0?State::FadingOut:State::Inactive);}
constexpr int next_level(int level,bool target) noexcept {
 return target?(level+kStepIn>kLevelMax?kLevelMax:level+kStepIn):(level-kStepOut<0?0:level-kStepOut);
}
// Eye variant index for a level: 0 (native) at level 0, kEyeVariants-1 (full purple) at 100.
constexpr unsigned eye_index(int level) noexcept {return level<=0?0u:level>=kLevelMax?kEyeVariants-1:unsigned((level*int(kEyeVariants-1)+kLevelMax/2)/kLevelMax);}
struct Tuning {int alpha_max;int r,g,b;int er,eg,eb;float scale;};
// Subtle by design; the game multiplies these with the resource's own colours. Tunable in one place after the first human round.
constexpr Tuning kBlue{110,90,170,255,20,60,150,4.0f};
constexpr Tuning kBlack{130,170,70,255,60,10,120,4.4f};
constexpr const Tuning& tuning_for(Variant v) noexcept {return v==Variant::Black?kBlack:kBlue;}
constexpr int alpha_for(Variant v,int level) noexcept {return level<=0?0:level>=kLevelMax?tuning_for(v).alpha_max:level*tuning_for(v).alpha_max/kLevelMax;}
struct Entry {
 bool used=false,aura_live=false,aura_target=false,aura_fault=false,eye_target=false,eye_fault=false;
 std::uint32_t id=0,generation=0,key=0;const void* identity=nullptr;Variant variant=Variant::None;
 int aura_level=0,eye_level=0;unsigned eye_idx=0;std::uint8_t events=0;
};
struct Ledger {
 std::array<Entry,kCapacity> entries{};unsigned used=0;
 Entry* find(std::uint32_t id) noexcept {if(!id)return nullptr;for(auto& e:entries)if(e.used&&e.id==id)return &e;return nullptr;}
 unsigned live_auras() const noexcept {unsigned n=0;for(const auto& e:entries)if(e.used&&e.aura_live)++n;return n;}
 void release(Entry& e) noexcept {if(e.used&&used)--used;e=Entry{};}
 Entry* record(std::uint32_t id,const void* identity,std::uint32_t generation,Variant v) noexcept {
  if(!id||!identity||find(id)||(v!=Variant::Blue&&v!=Variant::Black))return nullptr;
  for(auto& s:entries)if(!s.used){s=Entry{};s.used=true;s.id=id;s.identity=identity;s.generation=generation;s.variant=v;++used;return &s;}
  return nullptr;
 }
};
enum class Ev:std::uint8_t {None,AuraOn,AuraOff,EyesOn,EyesOff,AuraRejected,EyesRejected};
struct Event {Ev kind=Ev::None;const char* reason="NONE";int level=0,alpha=0;unsigned eye_idx=0;std::uint32_t key=0;};
struct Events {Event ev[4];unsigned n=0;void add(const Event& e) noexcept {if(n<4)ev[n++]=e;}};
// One completed POST of a classified actor: `want_aura` / `want_eyes` are the approved CP10.6 logic states.
template<class Fx> inline Events step(Ledger& L,Entry& e,Fx& fx,bool want_aura,bool want_eyes) noexcept {
 Events out;
 // ---- aura ----
 if(!e.aura_fault&&(want_aura||e.aura_level>0||e.aura_live)){
  const bool reverse=e.aura_level>0&&want_aura!=e.aura_target;
  const int level=next_level(e.aura_level,want_aura);
  if(level>0){
   const bool creating=!e.aura_live;
   if(creating&&L.live_auras()>=kMaxAuras){
    e.aura_fault=true;e.aura_level=0;Event r;r.kind=Ev::AuraRejected;r.reason="AURA_CAP_REACHED_NO_EMITTER";out.add(r);
   }else if(!fx.aura(e,level,alpha_for(e.variant,level))){
    const bool had=e.aura_live;if(had)fx.aura_release(e);e.aura_live=false;e.aura_fault=true;e.aura_level=0;
    Event r;r.kind=Ev::AuraRejected;r.reason=fx.reason();r.level=level;out.add(r);
   }else{
    e.aura_live=true;e.aura_level=level;
    if(creating||(reverse&&want_aura)){Event o;o.kind=Ev::AuraOn;o.reason=creating?"LOGIC_STATE_ON":"RESUMED_DURING_FADE_OUT_FROM_CURRENT_LEVEL";o.level=level;o.alpha=alpha_for(e.variant,level);o.key=e.key;out.add(o);}
   }
  }else{
   if(e.aura_live){fx.aura_release(e);e.aura_live=false;Event o;o.kind=Ev::AuraOff;o.reason="FADE_OUT_COMPLETE_EMITTER_RELEASED";out.add(o);}
   e.aura_level=0;
  }
  e.aura_target=want_aura;
 }else if(!e.aura_fault)e.aura_target=want_aura;
 // ---- eyes (Black only) ----
 if(e.variant==Variant::Black&&!e.eye_fault&&(want_eyes||e.eye_level>0)){
  const int level=next_level(e.eye_level,want_eyes);const unsigned idx=eye_index(level);
  e.eye_level=level;e.eye_target=want_eyes;
  if(idx!=e.eye_idx){
   if(!fx.eye(e,idx)){
    e.eye_fault=true;const char* why=fx.reason();
    if(e.eye_idx>0)fx.eye(e,0);                                   // best effort back to native; ownership is re-checked inside
    e.eye_idx=0;e.eye_level=0;Event r;r.kind=Ev::EyesRejected;r.reason=why;r.eye_idx=idx;out.add(r);
   }else{
    const unsigned previous=e.eye_idx;e.eye_idx=idx;
    if(previous==0&&idx>0){Event o;o.kind=Ev::EyesOn;o.reason="LOGIC_STATE_ON";o.level=level;o.eye_idx=idx;out.add(o);}
    else if(idx==0){Event o;o.kind=Ev::EyesOff;o.reason="FADE_OUT_COMPLETE_NATIVE_TEXTURE_RESTORED";o.level=level;o.eye_idx=0;out.add(o);}
   }
  }
 }
 return out;
}
// Unload / explicit cleanup. `actor_live` = the id still resolves to the very same, not-destroyed actor (only then may anything be touched).
template<class Fx> inline void cleanup(Entry& e,Fx& fx,bool actor_live) noexcept {
 if(e.aura_live){fx.aura_release(e);e.aura_live=false;}
 if(actor_live&&e.eye_idx>0&&!e.eye_fault){fx.eye(e,0);}
 e.eye_idx=0;e.aura_level=e.eye_level=0;
}
} // namespace devf_vis
