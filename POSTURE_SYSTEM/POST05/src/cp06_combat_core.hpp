#pragma once
// CP06 0.11.11: value-only evidence. No game headers, addresses, RNG or native
// writes here; the adapter samples native state and applies bounded results.
#include <cstdint>
namespace cp06 {
inline constexpr const char* kVersion="0.11.11";
inline constexpr unsigned kSlots=64;
// Bokoblin Expansion 0.17.0 CP18/CP19F2: a Black blow started during CP10.6
// fury plays exactly x2 and its engine-confirmed shield block takes exactly one
// pending life unit in Expansion's own E_OC execute POST (priority 0).
inline constexpr float kFuryRate=2.0f,kChipUnits=1.0f;
inline constexpr int kFuryAtp=6;
// A fury parry is offered ONLY with every independent signal agreeing: E_OC2,
// both AT spheres at power6 (common atp), the per-generation LNK07 class
// BLACK_FURY and Expansion's exact fury rate on a valid clip. Anything else
// keeps the legacy conservative POST08 refusal (rate>=1.5 or unknown).
inline bool fury_parry_authenticated(bool e_oc2,int atp,bool anim_ok,float anim_rate,bool variant_fury_verified)noexcept{
 return e_oc2&&atp==kFuryAtp&&anim_ok&&anim_rate==kFuryRate&&variant_fury_verified;
}
enum class Chip:std::uint8_t{None,Neutralized,NoChipObserved,NotArmed,NotCaptured,ContextChanged,DeltaMismatch,TypeChanged,LifeChanged,LinkInvalid,ReadbackFailed,Count};
inline const char* name(Chip c)noexcept{
 const char* n[]={"NONE","NEUTRALIZED_ATTRIBUTED_EXPANSION_CHIP","NO_CHIP_OBSERVED_IN_WINDOW","NOT_ARMED","NOT_CAPTURED_BEFORE_EXPANSION_POST","ACTOR_OR_TOKEN_CHANGED","PENDING_DELTA_NOT_EXACTLY_MINUS_ONE","PENDING_TYPE_CHANGED","LINK_LIFE_CHANGED_IN_WINDOW","LINK_INVALID_OR_EVENT","WRITE_READBACK_FAILED"};
 return n[unsigned(c)<unsigned(Chip::Count)?unsigned(c):0];
}
struct ChipView{float pending=0;std::uint8_t type=0;int life=0;bool link_ok=false,shield_pair=false;};
// One armed window per committed authenticated fury parry: the SAME actor
// execute, from our POST before Expansion (capture) to our POST after it.
class ChipGuard{
 public:
 struct Counters{std::uint64_t armed=0,captured=0,decided=0,result[unsigned(Chip::Count)]{},duplicate_arm=0,shield_pair_missing=0;}c{};
 struct Row{bool armed=false,captured=false;std::uint32_t id=0;std::uint64_t generation=0,token=0,tick=0;ChipView before{};};
 bool arm(unsigned slot,std::uint32_t id,std::uint64_t generation,std::uint64_t token,std::uint64_t tick)noexcept{
  if(slot>=kSlots||!id||!token)return false;Row& r=rows_[slot];
  if(r.armed&&r.id==id&&r.generation==generation&&r.token==token){++c.duplicate_arm;return false;}
  for(unsigned n=0;n<kSlots;++n)if(n!=slot&&rows_[n].armed&&rows_[n].id==id&&rows_[n].generation==generation&&rows_[n].token==token){++c.duplicate_arm;return false;}
  if(token<=last_token_[slot]&&last_id_[slot]==id&&last_gen_[slot]==generation){++c.duplicate_arm;return false;} // one neutralization per blow token, ever
  if(!r.armed)++armed_;r=Row{};r.armed=true;r.id=id;r.generation=generation;r.token=token;r.tick=tick;++c.armed;return true;
 }
 // Before Expansion's POST of the same execute. Returns false if nothing armed.
 bool capture(unsigned slot,std::uint32_t id,std::uint64_t generation,std::uint64_t tick,const ChipView& v)noexcept{
  if(slot>=kSlots)return false;Row& r=rows_[slot];
  if(!r.armed||r.captured||r.id!=id||r.generation!=generation||r.tick!=tick)return false;
  if(!v.link_ok){clear(slot);record(Chip::LinkInvalid);return false;}
  if(!v.shield_pair)++c.shield_pair_missing;
  r.captured=true;r.before=v;++c.captured;return true;
 }
 // After Expansion's POST. Compensate only an exact attributable unit.
 Chip decide(unsigned slot,std::uint32_t id,std::uint64_t generation,std::uint64_t tick,const ChipView& now,float& restore,std::uint8_t& type)noexcept{
  restore=0;type=0;if(slot>=kSlots)return Chip::None;Row& r=rows_[slot];
  if(!r.armed)return Chip::None;
  if(r.id!=id||r.generation!=generation||r.tick!=tick){const bool stale=r.tick!=tick;clear(slot);return record(stale?Chip::NotCaptured:Chip::ContextChanged);}
  const Row x=r;clear(slot);last_id_[slot]=x.id;last_gen_[slot]=x.generation;last_token_[slot]=x.token;
  if(!x.captured)return record(Chip::NotCaptured);
  if(!now.link_ok||!x.before.link_ok)return record(Chip::LinkInvalid);
  if(now.life!=x.before.life)return record(Chip::LifeChanged);
  if(now.pending==x.before.pending&&now.type==x.before.type)return record(Chip::NoChipObserved);
  if(now.type!=x.before.type)return record(Chip::TypeChanged);
  if(!x.before.shield_pair||now.pending!=x.before.pending-kChipUnits)return record(Chip::DeltaMismatch);
  restore=kChipUnits;type=now.type;++c.decided;return Chip::Neutralized;
 }
 Chip record(Chip k)noexcept{++c.result[unsigned(k)<unsigned(Chip::Count)?unsigned(k):0];return k;}
 const Row& row(unsigned slot)const noexcept{return rows_[slot<kSlots?slot:0];}
 // O(1) gate for the per-execute callbacks: no armed window anywhere => no slot lookup.
 unsigned armed_count()const noexcept{return armed_;}
 void reset()noexcept{armed_=0;for(auto& r:rows_)r=Row{};for(unsigned n=0;n<kSlots;++n){last_id_[n]=0;last_gen_[n]=0;last_token_[n]=0;}}
 private:
 void clear(unsigned slot)noexcept{if(rows_[slot].armed&&armed_)--armed_;rows_[slot]=Row{};}
 unsigned armed_=0;Row rows_[kSlots]{};std::uint32_t last_id_[kSlots]{};std::uint64_t last_gen_[kSlots]{},last_token_[kSlots]{};
};
// Post-hoc timing diagnosis of a resolved, non-parried blocked/unblocked
// contact: did a NEW R+B edge arrive only AFTER the collision pass that
// resolved the blow? Read-only; it never changes a verdict or grants a parry.
enum class Late:std::uint8_t{None,AfterResolution,Count};
class LateInput{
 public:
 static constexpr unsigned kRing=16,kHorizon=8;
 struct Event{bool on=false,reported=false;std::uint32_t id=0;std::uint64_t generation=0,token=0,tick=0;int variant=4,result=0;float rate=0,frame=0;};
 struct Counters{std::uint64_t contacts=0,late=0,by_delta[kHorizon+1]{},by_variant[5]{},rate2=0;}c{};
 void contact(std::uint32_t id,std::uint64_t generation,std::uint64_t token,std::uint64_t tick,int variant,int result,float rate,float frame)noexcept{
  Event& e=ring_[next_++%kRing];e=Event{};e.on=true;e.id=id;e.generation=generation;e.token=token;e.tick=tick;e.variant=variant<0||variant>4?4:variant;e.result=result;e.rate=rate;e.frame=frame;++c.contacts;
 }
 // `tick` = tick of the new input edge (Link execute); the contact was judged
 // at the damage_check PRE of e.tick, i.e. its pass ran at the end of e.tick-1.
 template<class Emit>void input(std::uint64_t tick,Emit&& emit)noexcept{
  for(auto& e:ring_){if(!e.on||e.reported||tick<e.tick||tick-e.tick>kHorizon)continue;e.reported=true;const unsigned d=unsigned(tick-e.tick);
   ++c.late;++c.by_delta[d];++c.by_variant[e.variant];if(e.rate==2.0f)++c.rate2;emit(e,d);}
 }
 void reset()noexcept{for(auto& e:ring_)e=Event{};next_=0;}
 private:
 Event ring_[kRing]{};unsigned next_=0;
};
}
