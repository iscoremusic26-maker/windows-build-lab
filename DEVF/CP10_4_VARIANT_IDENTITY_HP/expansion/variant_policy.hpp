#pragma once
// CP10.4 pure policy (no game headers): Bokoblin variant IDENTITY, initial/maximum HP and regeneration.
// Evolution of CP10.3 regen_policy.hpp. The tick/accumulator/death rules of `step()` are unchanged; what changes is that the
// maximum is no longer the native creation value but the MOD's per-variant value, decided ONCE per actor generation.
//
// Native creation signature (src/d/actor/d_a_e_oc.cpp create() L2776-2778): E_OC -> field_0x560 = health = 40, E_OC2 -> 220.
// It is only used to VERIFY a pristine first creation. It is never the maximum under this mod.
//
// Identity precedence per actor and generation:
//   1. exact DEVF registration, variant black   -> BLACK (configured hp, 4 HP/s)
//   2. exact DEVF registration, normal/legacy   -> BLUE  (220, 2 HP/s) even though the broker spawns it with the E_OC2 resource
//   3. no registration, mName == "E_OC"         -> BLUE  (40 -> 220, 2 HP/s)
//   4. no registration, mName == "E_OC2"        -> RED   (220 -> 330, 3 HP/s)
//   otherwise (other name, modified native signature, not a first creation, failed Black admission) -> REJECT, nothing is written.
// Time base: one completed daE_OC_c::execute pair = one 1/30 s simulation tick (see CP10.3 IMPLEMENTATION.md).
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_variant {
constexpr unsigned kTicksPerSecond=30;
constexpr unsigned kRateBlue=2,kRateRed=3,kRateBlack=4;               // HP per second of game time
constexpr std::int16_t kNativeHpE_OC=40,kNativeHpE_OC2=220;           // native creation signatures
constexpr std::int16_t kMaxHpBlue=220,kMaxHpRed=330,kBlackHpMin=1,kBlackHpMax=999;
constexpr unsigned kCapacity=64;
constexpr unsigned kHealLogEveryHp=10,kHealLogMaxPerActor=32;
// Mirror of enum OC_ACTIONS (d_a_e_oc.cpp L20-37).
enum Action:int {A_WAIT=0,A_WALK,A_TALK,A_FIND,A_ATTACK,A_DAMAGE,A_BIG_DAMAGE,A_WATCH,A_SOUND_WATCH,A_DEATH,A_WATER_DEATH,A_DEMO_MASTER,A_DEMO_CHILD,A_FALL_DEAD,A_FALL,A_FIND_STAY,A_MOVE_OUT,A_COUNT};
constexpr bool blocks_regen(int action) noexcept {
 return action==A_DEATH||action==A_WATER_DEATH||action==A_FALL_DEAD||action==A_FALL||action==A_DEMO_MASTER||action==A_DEMO_CHILD||action<0||action>=A_COUNT;
}
enum class Variant:std::uint8_t {None,Blue,Red,Black};
enum class Source:std::uint8_t {None,NativeEOC,NativeEOC2,DevfNormal,DevfBlack};
enum class Registration:std::uint8_t {None,Normal,Black,BlackAdmissionFailed};   // result of the exact DEVF record match
enum class Why:std::uint8_t {None,UnknownName,FirstExecuteNotObserved,NativeSignature,NativeHealthNotInitial,BlackAdmissionRejected,BadRegistration,BlackHpRange,RegistryFull,Reentrant};
inline const char* variant_text(Variant v) noexcept {return v==Variant::Blue?"BLUE":v==Variant::Red?"RED":v==Variant::Black?"BLACK":"UNCLASSIFIED";}
inline const char* source_text(Source s) noexcept {
 switch(s){case Source::NativeEOC:return "NATIVE_E_OC";case Source::NativeEOC2:return "NATIVE_E_OC2";case Source::DevfNormal:return "DEVF_NORMAL";case Source::DevfBlack:return "DEVF_BLACK";default:return "NONE";}
}
inline const char* why_text(Why w) noexcept {
 switch(w){case Why::UnknownName:return "UNKNOWN_BOKOBLIN_NAME";case Why::FirstExecuteNotObserved:return "FIRST_EXECUTE_NOT_OBSERVED_NO_MID_COMBAT_ASSIGNMENT";
  case Why::NativeSignature:return "NATIVE_FIELD_0x560_SIGNATURE_MISMATCH";case Why::NativeHealthNotInitial:return "NATIVE_INITIAL_HEALTH_ALREADY_MODIFIED";
  case Why::BlackAdmissionRejected:return "BLACK_REGISTRATION_MATCHED_BUT_ADMISSION_REJECTED";case Why::BadRegistration:return "REGISTRATION_INCOMPATIBLE_WITH_RESOURCE";
  case Why::BlackHpRange:return "BLACK_HP_OUT_OF_RANGE";case Why::RegistryFull:return "REGISTRY_FULL";case Why::Reentrant:return "REENTRANT_EXECUTE";default:return "NONE";}
}
struct Plan {
 Variant variant=Variant::None;Source source=Source::None;
 std::int16_t native_base=0,initial_hp=0,max_hp=0;std::uint8_t rate=0;
 bool write_hp=false;          // true only for native Blue/Red; DEVF Black HP is written by the existing exact admission (HP_APPLIED)
 Why why=Why::None;
};
// Decides identity + HP ONCE, at the first execute of a fresh actor generation.
//  `field_0x560` and `native_health` must be read BEFORE this module writes anything for this actor.
inline Plan make_plan(const char* name,std::int16_t field_0x560,int native_health,bool first_execute,Registration reg,int black_hp) noexcept {
 Plan p;
 std::int16_t expected=0;bool e_oc2=false;
 if(name&&!std::strcmp(name,"E_OC")){expected=kNativeHpE_OC;}
 else if(name&&!std::strcmp(name,"E_OC2")){expected=kNativeHpE_OC2;e_oc2=true;}
 else {p.why=Why::UnknownName;return p;}
 if(!first_execute){p.why=Why::FirstExecuteNotObserved;return p;}   // never assign 220/330/440 in the middle of a fight
 if(field_0x560!=expected){p.why=Why::NativeSignature;return p;}    // another mod changed the creation value: do not overwrite it
 if(native_health!=expected){p.why=Why::NativeHealthNotInitial;return p;}
 p.native_base=expected;
 switch(reg){
  case Registration::BlackAdmissionFailed:p.why=Why::BlackAdmissionRejected;return p;
  case Registration::Black:
   if(!e_oc2){p.why=Why::BadRegistration;return p;}
   if(black_hp<kBlackHpMin||black_hp>kBlackHpMax){p.why=Why::BlackHpRange;return p;}
   p.variant=Variant::Black;p.source=Source::DevfBlack;p.initial_hp=p.max_hp=static_cast<std::int16_t>(black_hp);p.rate=kRateBlack;p.write_hp=false;return p;
  case Registration::Normal:
   if(!e_oc2){p.why=Why::BadRegistration;return p;}
   p.variant=Variant::Blue;p.source=Source::DevfNormal;p.initial_hp=p.max_hp=kMaxHpBlue;p.rate=kRateBlue;p.write_hp=false;return p; // native 220 already equals Blue
  default:break;
 }
 if(!e_oc2){p.variant=Variant::Blue;p.source=Source::NativeEOC;p.initial_hp=p.max_hp=kMaxHpBlue;p.rate=kRateBlue;p.write_hp=true;return p;}
 p.variant=Variant::Red;p.source=Source::NativeEOC2;p.initial_hp=p.max_hp=kMaxHpRed;p.rate=kRateRed;p.write_hp=true;return p;
}
struct Entry {
 std::uint32_t id=0;const void* identity=nullptr;
 Variant variant=Variant::None;Source source=Source::None;std::int16_t native_base=0,max_hp=0;std::uint8_t rate=0,acc=0,events=0;
 bool used=false,pending=false,classified=false,rejected=false,tracked_logged=false;Why why=Why::None;
 std::uint32_t ticks=0,healed=0,generation=0;
 const char* encounter=nullptr;     // DEVF encounter id (points into the registration table), for logs only
};
inline void adopt(Entry& e,const Plan& p) noexcept {
 e.classified=true;e.variant=p.variant;e.source=p.source;e.native_base=p.native_base;e.max_hp=p.max_hp;e.rate=p.rate;e.rejected=p.variant==Variant::None;e.why=p.why;
}
struct Step {int heal=0;std::int16_t new_health=0;bool milestone=false;const char* skip="NONE";};
// One completed execute pair (POST). Unchanged rules from CP10.3.
inline Step step(Entry& e,std::int16_t health,int action) noexcept {
 Step s;s.new_health=health;
 ++e.ticks;
 if(e.rejected||e.variant==Variant::None){s.skip="REJECTED";return s;}
 if(health<=0){e.acc=0;s.skip="DEAD";return s;}
 if(blocks_regen(action)){e.acc=0;s.skip="IRREVERSIBLE_OR_SCRIPTED_ACTION";return s;}
 if(health>=e.max_hp){e.acc=0;s.skip=health==e.max_hp?"FULL":"ABOVE_MAX_UNTOUCHED";return s;}
 unsigned acc=e.acc+e.rate;
 const unsigned heal=acc/kTicksPerSecond;e.acc=static_cast<std::uint8_t>(acc%kTicksPerSecond);
 if(!heal){s.skip="FRACTION_ACCUMULATED";return s;}
 int target=int(health)+int(heal);if(target>e.max_hp)target=e.max_hp;
 s.heal=target-health;s.new_health=static_cast<std::int16_t>(target);
 const std::uint32_t before=e.healed;e.healed+=static_cast<std::uint32_t>(s.heal);
 s.milestone=before==0||(before/kHealLogEveryHp)!=(e.healed/kHealLogEveryHp);
 return s;
}
struct Registry {
 std::array<Entry,kCapacity> entries{};unsigned used=0,cursor=0;std::uint32_t next_generation=0;bool full_logged=false;
 Entry* find(std::uint32_t id) noexcept {for(auto& e:entries)if(e.used&&e.id==id)return &e;return nullptr;}
 void release(Entry& e) noexcept {if(e.used&&used)--used;e=Entry{};}
 Entry* begin(std::uint32_t id,const void* identity,bool initializing,bool* released_stale=nullptr,Why* refused=nullptr) noexcept {
  if(released_stale)*released_stale=false;
  if(!id||id==0xFFFFFFFFu||!identity)return nullptr;
  Entry* e=find(id);
  if(e&&(e->identity!=identity||initializing)){
   if(released_stale)*released_stale=true;
   release(*e);e=nullptr;
  }
  if(e){
   if(e->pending){if(refused)*refused=Why::Reentrant;return nullptr;}
   e->pending=true;return e;
  }
  for(auto& slot:entries)if(!slot.used){
   slot=Entry{};slot.used=true;slot.id=id;slot.identity=identity;slot.pending=true;slot.generation=++next_generation;++used;return &slot;
  }
  if(refused)*refused=Why::RegistryFull;
  return nullptr;
 }
 Entry* end(std::uint32_t id,const void* identity) noexcept {
  Entry* e=find(id);
  if(!e||!e->pending||e->identity!=identity)return nullptr;
  e->pending=false;return e;
 }
};
} // namespace devf_variant
