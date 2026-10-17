#pragma once
// CP10.3 pure regeneration policy (no game headers): passive, fixed HP-per-game-second regeneration for Bokoblins.
//
// TIME BASE (verified in Dusklight 40457c6):
//  * src/dusk/game_clock.h: constexpr kSimPeriod = 1/30 s. m_Do_main.cpp calls fapGm_Execute() once per simulation tick.
//  * fopAc_Execute (src/f_op/f_op_actor.cpp ~L323-335) skips the actor while dComIfGp_isPauseFlag()/dScnPly_c::isPause()/
//    dComIfA_PauseCheck() or its stop status is set, so daE_OC_c::execute() runs exactly once per UNPAUSED game tick.
//  => one completed execute pair (PRE+POST) of one actor == one tick == 1/30 s of game time for that actor.
// Regeneration is therefore counted in ticks, never in wall-clock time: no dependence on render FPS, nothing is granted for
// pauses, menus, loading screens, events that stop the actor, or any gap in which the actor did not execute.
//
// EXACT ARITHMETIC: rate is an integer HP per second. Each tick adds `rate` to an integer accumulator in 1/30 HP units;
// every 30 units become 1 HP. Over any whole number of game seconds the healed amount is exactly rate*seconds. No floats.
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_regen {
constexpr unsigned kTicksPerSecond=30;             // 1 / kSimPeriod
constexpr unsigned kRateNormalHpPerSecond=2;       // native Bokoblins and DEVF Normal
constexpr unsigned kRateBlackHpPerSecond=4;        // DEVF Black (x2)
constexpr std::int16_t kMaxHpE_OC=40;              // d_a_e_oc.cpp create(): mName=="E_OC"  -> field_0x560 = health = 40
constexpr std::int16_t kMaxHpE_OC2=220;            // d_a_e_oc.cpp create(): otherwise      -> field_0x560 = health = 220
constexpr unsigned kCapacity=64;                   // simultaneously tracked actors (nativos + DEVF), fail-closed when full
constexpr unsigned kHealLogEveryHp=10;             // one REGEN_APPLIED log per 10 HP healed
constexpr unsigned kHealLogMaxPerActor=32;
// Mirror of the enum OC_ACTIONS in src/d/actor/d_a_e_oc.cpp L20-37 (values start at 0, in this order).
enum Action:int {A_WAIT=0,A_WALK,A_TALK,A_FIND,A_ATTACK,A_DAMAGE,A_BIG_DAMAGE,A_WATCH,A_SOUND_WATCH,A_DEATH,A_WATER_DEATH,A_DEMO_MASTER,A_DEMO_CHILD,A_FALL_DEAD,A_FALL,A_FIND_STAY,A_MOVE_OUT,A_COUNT};
// Death / irreversible / scripted states: never heal. (health<=0 is checked separately.)
constexpr bool blocks_regen(int action) noexcept {
 return action==A_DEATH||action==A_WATER_DEATH||action==A_FALL_DEAD||action==A_FALL||action==A_DEMO_MASTER||action==A_DEMO_CHILD||action<0||action>=A_COUNT;
}
enum class Kind:std::uint8_t {None,Native,Black};
enum class Why:std::uint8_t {None,UnknownName,MaxHpContract,RegistryFull,BadBlackHp,Reentrant};
inline const char* why_text(Why w) noexcept {
 switch(w){case Why::UnknownName:return "UNKNOWN_BOKOBLIN_NAME";case Why::MaxHpContract:return "NATIVE_MAX_HP_CONTRACT_MISMATCH";case Why::RegistryFull:return "REGISTRY_FULL";
  case Why::BadBlackHp:return "BLACK_HP_INVALID";case Why::Reentrant:return "REENTRANT_EXECUTE";default:return "NONE";}
}
struct Entry {
 std::uint32_t id=0;const void* identity=nullptr;      // actor id + address, compared only, never dereferenced
 Kind kind=Kind::None;std::int16_t max_hp=0;std::uint8_t rate=0,acc=0,events=0;
 bool used=false,pending=false,tracked_logged=false,rejected=false,dead_logged=false;Why why=Why::None;
 std::uint32_t ticks=0,healed=0,generation=0;
 int black_hp=0;                                        // set only by the exact DEVF Black registration admission
};
struct Classification {Kind kind=Kind::None;std::int16_t max_hp=0;std::uint8_t rate=0;Why why=Why::None;const char* source="NONE";};
// `name` = daE_OC_c::mName, `native_base` = field_0x560, `black_hp` > 0 only for an actor admitted by the exact DEVF Black registration.
inline Classification classify(const char* name,std::int16_t native_base,int black_hp) noexcept {
 Classification c;
 std::int16_t expected=0;
 if(name&&!std::strcmp(name,"E_OC")){expected=kMaxHpE_OC;c.source="NATIVE_E_OC";}
 else if(name&&!std::strcmp(name,"E_OC2")){expected=kMaxHpE_OC2;c.source="NATIVE_E_OC2";}
 else {c.why=Why::UnknownName;return c;}
 if(native_base!=expected){c.why=Why::MaxHpContract;return c;}       // another mod changed the native maximum: do not guess
 if(black_hp>0){
  if(black_hp>999){c.why=Why::BadBlackHp;return c;}   // same bound as the schema (hp 1..999)
  c.kind=Kind::Black;c.max_hp=static_cast<std::int16_t>(black_hp);c.rate=kRateBlackHpPerSecond;c.source="DEVF_BLACK_CONFIGURED_HP";return c;
 }
 c.kind=Kind::Native;c.max_hp=expected;c.rate=kRateNormalHpPerSecond;return c;
}
struct Step {int heal=0;std::int16_t new_health=0;bool milestone=false;const char* skip="NONE";};
// One completed execute pair. `health` is read AFTER the native execute (damage already processed), so a lethal hit is never pre-empted.
inline Step step(Entry& e,std::int16_t health,int action) noexcept {
 Step s;s.new_health=health;
 ++e.ticks;
 if(e.rejected||e.kind==Kind::None){s.skip="REJECTED";return s;}
 if(health<=0){e.acc=0;s.skip="DEAD";return s;}
 if(blocks_regen(action)){e.acc=0;s.skip="IRREVERSIBLE_OR_SCRIPTED_ACTION";return s;}
 if(health>=e.max_hp){e.acc=0;s.skip=health==e.max_hp?"FULL":"ABOVE_MAX_UNTOUCHED";return s;}   // never lowers, never banks
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
 // PRE of an execute pair. Returns nullptr when this call must not be counted (invalid, reentrant, registry full).
 Entry* begin(std::uint32_t id,const void* identity,bool initializing,bool* released_stale=nullptr,Why* refused=nullptr) noexcept {
  if(released_stale)*released_stale=false;
  if(!id||id==0xFFFFFFFFu||!identity)return nullptr;
  Entry* e=find(id);
  if(e&&(e->identity!=identity||initializing)){                // id reuse or a new native creation: never inherit state
   if(released_stale)*released_stale=true;
   release(*e);e=nullptr;
  }
  if(e){
   if(e->pending){if(refused)*refused=Why::Reentrant;return nullptr;}   // second PRE before POST: do not count another tick
   e->pending=true;return e;
  }
  for(auto& slot:entries)if(!slot.used){
   slot=Entry{};slot.used=true;slot.id=id;slot.identity=identity;slot.pending=true;slot.generation=++next_generation;++used;return &slot;
  }
  if(refused)*refused=Why::RegistryFull;
  return nullptr;
 }
 // POST: only a pending entry of the same actor completes a tick; anything else is ignored.
 Entry* end(std::uint32_t id,const void* identity) noexcept {
  Entry* e=find(id);
  if(!e||!e->pending||e->identity!=identity)return nullptr;
  e->pending=false;return e;
 }
};
} // namespace devf_regen
