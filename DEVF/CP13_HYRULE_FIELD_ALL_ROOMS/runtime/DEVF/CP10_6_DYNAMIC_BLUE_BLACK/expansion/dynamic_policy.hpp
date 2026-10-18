#pragma once
// CP10.6 pure policy (no game headers): dynamic ABSOLUTE attack power on top of the CP10.5 base profile.
//   BLUE : 1 alone, 2 while at least one OTHER live classified Bokoblin is within 650 XZ / 250 Y, same room and stage.
//   RED  : always 2 (counts as a companion, never receives a buff).
//   BLACK: 3, and 6 for exactly 300 simulation ticks after a real, non-lethal loss of HP observed in the native execute.
// Everything here is written as templates over a tiny collider adapter (`Io::get(i)`, `Io::set(i,v)`), so the mod and the lab run the
// SAME code. One authority writes the two spheres (`reconcile`), only on a change of the effective target, only while the pair is
// still verifiably owned (both spheres hold exactly the value this module wrote last).
//
// TIME BASE: a completed PRE+POST pair of daE_OC_c::execute is one 1/30 s tick of that actor (see CP10.3). Pauses execute nothing.
// FURY TICK RULE: the POST that observes the loss is tick 0 and sets `fury = 300`. Each later POST decrements first; the POST where it
// reaches 0 restores the base (tick 300). The 6 is therefore effective in the collision phase of ticks 0..299 = exactly 300 ticks
// (10.000 s of game time). A hit observed in a POST that would otherwise expire renews instead (no gap, no stacking).
#include "../../CP10_5_DAMAGE_BLUE_RED_BLACK/expansion/damage_policy.hpp"
#include <array>
#include <cstdint>
namespace devf_dyn {
using devf_variant::Variant;
constexpr double kGroupRadiusXZ=650.0,kGroupRadiusY=250.0;
constexpr unsigned kFuryTicks=300,kCapacity=64,kEventsPerActor=32;
constexpr std::uint8_t kAtpBlueAlone=1,kAtpBlueGroup=2,kAtpRed=2,kAtpBlackBase=3,kAtpBlackFury=6;
static_assert(kAtpBlueAlone==devf_damage::kAtpBlue&&kAtpRed==devf_damage::kAtpRed&&kAtpBlackBase==devf_damage::kAtpBlack,"CP10.5 base profile");
static_assert(kAtpBlueGroup==2*kAtpBlueAlone&&kAtpBlackFury==2*kAtpBlackBase,"dynamic values are exactly the double of the base");
constexpr unsigned window_for(unsigned tracked) noexcept {return 2*tracked+2;}   // freshness window, in completed execute pairs of tracked actors
struct Entry {
 bool used=false,owned=false,snap=false,group=false,pre_valid=false;
 std::uint32_t id=0,generation=0;const void* identity=nullptr;Variant variant=Variant::None;
 std::uint8_t native=0,base=0,applied=0,events=0;
 // snapshot of the last completed execute pair (values only: a companion is never dereferenced through this record)
 float x=0,y=0,z=0;int room=-1;std::uint32_t stage=0,seq=0;int health=0,action=-1;
 int pre_health=0;unsigned fury=0;
};
struct Ledger {
 std::array<Entry,kCapacity> entries{};unsigned used=0;
 Entry* find(std::uint32_t id) noexcept {if(!id)return nullptr;for(auto& e:entries)if(e.used&&e.id==id)return &e;return nullptr;}
 void release(Entry& e) noexcept {if(e.used&&used)--used;e=Entry{};}
 Entry* record(std::uint32_t id,const void* identity,std::uint32_t generation,Variant v,std::uint8_t base) noexcept {
  if(!id||!identity||find(id))return nullptr;
  for(auto& slot:entries)if(!slot.used){slot=Entry{};slot.used=true;slot.id=id;slot.identity=identity;slot.generation=generation;slot.variant=v;slot.base=base;++used;return &slot;}
  return nullptr;
 }
};
// ---------------- admission (CP10.5 semantics, generic over the collider adapter) ----------------
struct Admit {devf_damage::Plan plan;Entry* rec=nullptr;std::uint8_t b0=0,b1=0,c0=0,c1=0;};
// Every CP10.4-classified actor gets a record (it can be somebody's companion); only a pristine native pair becomes `owned`.
template<class Io> inline Admit admit(Ledger& L,std::uint32_t id,const void* identity,std::uint32_t generation,Variant v,const char* name,Io& io) noexcept {
 Admit r;r.b0=r.c0=io.get(0);r.b1=r.c1=io.get(1);
 r.plan=devf_damage::make_plan(v,name,r.b0,r.b1);
 if(v==Variant::None)return r;
 r.rec=L.record(id,identity,generation,v,devf_damage::target_for(v));
 if(!r.rec){if(r.plan.accepted){r.plan.accepted=false;r.plan.why=devf_damage::Why::LedgerFull;}return r;}
 if(!r.plan.accepted)return r;                                  // recorded as companion candidate, colliders never touched, owned=false
 r.rec->native=r.plan.native;
 if(r.plan.write){io.set(0,r.plan.target);io.set(1,r.plan.target);}
 r.c0=io.get(0);r.c1=io.get(1);
 if(r.c0!=r.plan.target||r.c1!=r.plan.target){
  io.set(0,r.plan.native);io.set(1,r.plan.native);r.c0=io.get(0);r.c1=io.get(1);
  r.plan.accepted=false;r.plan.why=devf_damage::Why::ReadbackMismatch;return r;
 }
 r.rec->owned=true;r.rec->applied=r.plan.target;
 return r;
}
// ---------------- the single writer ----------------
// Returns nullptr when nothing is wrong (including "nothing to do"), else a reason; on failure the pair is no longer owned.
template<class Io> inline const char* reconcile(Entry& e,Io& io,std::uint8_t want) noexcept {
 if(!e.owned||want==e.applied)return nullptr;
 if(io.get(0)!=e.applied||io.get(1)!=e.applied){e.owned=false;return "COLLIDER_CHANGED_BY_OTHERS_NOT_OWNED";}
 io.set(0,want);io.set(1,want);
 if(io.get(0)!=want||io.get(1)!=want){io.set(0,e.applied);io.set(1,e.applied);e.owned=false;return "WRITE_READBACK_MISMATCH";}
 e.applied=want;
 return nullptr;
}
// ---------------- Blue companion query ----------------
// No <cmath> calls on purpose: a CRT import (sqrt/fabs with errno semantics) would change the reviewed import set of the DLL.
inline double abs_d(double v) noexcept {return v<0?-v:v;}
inline double sqrt_d(double v) noexcept {if(!(v>0))return 0;double x=v>1?v:1;for(int i=0;i<60;++i)x=0.5*(x+v/x);return x;}   // telemetry only
inline bool in_group_range(float ax,float ay,float az,float bx,float by,float bz) noexcept {
 const double dx=double(ax)-double(bx),dz=double(az)-double(bz),dy=double(ay)-double(by);
 return dx*dx+dz*dz<=kGroupRadiusXZ*kGroupRadiusXZ&&abs_d(dy)<=kGroupRadiusY;   // euclidean XZ compared squared, both limits inclusive
}
inline bool eligible(const Entry& self,const Entry& c,std::uint32_t seq,unsigned window) noexcept {
 if(!c.used||!c.snap||&c==&self||c.id==self.id||c.identity==self.identity||c.variant==Variant::None)return false;
 if(c.health<=0||devf_variant::blocks_regen(c.action))return false;          // dead, dying, falling, in a demo: not a living companion
 if(c.room!=self.room||c.stage!=self.stage)return false;
 if(std::uint32_t(seq-c.seq)>window)return false;                           // has not executed recently: stopped, culled or gone
 return in_group_range(self.x,self.y,self.z,c.x,c.y,c.z);
}
// `verify(c)` is the authoritative liveness check (id still resolves to the very same, not-destroyed actor). It is called only when
// a Blue is about to turn the bonus ON (needVerify), never per tick while it is already on. A failing candidate is dropped from
// consideration until it executes again.
template<class V> inline const Entry* find_companion(Ledger& L,const Entry& self,std::uint32_t seq,unsigned window,bool need_verify,V&& verify) noexcept {
 for(auto& c:L.entries){
  if(!eligible(self,c,seq,window))continue;
  if(need_verify&&!verify(c)){c.snap=false;continue;}
  return &c;
 }
 return nullptr;
}
// ---------------- POST update ----------------
enum class Ev:std::uint8_t {None,BlueOn,BlueOff,FuryStart,FuryRefresh,FuryEnd,Rejected};
struct Event {Ev kind=Ev::None;std::uint8_t from=0,to=0,c0=0,c1=0;const char* reason="NONE";std::uint32_t companion=0;Variant companion_variant=Variant::None;
 double dxz=0,dy=0;unsigned fury=0;int pre_hp=0,post_hp=0;};
struct Post {int post_health=0,action=-1,room=-1;float x=0,y=0,z=0;std::uint32_t stage=0,seq=0;unsigned window=0;};
inline void capture_pre(Entry& e,int health) noexcept {e.pre_health=health;e.pre_valid=true;}   // end of PRE, AFTER every HP initialisation write
// One completed pair. `in.post_health` is the health right after the NATIVE execute and BEFORE the regeneration write.
template<class Io,class V> inline Event post_update(Ledger& L,Entry& e,Io& io,const Post& in,V&& verify) noexcept {
 Event ev;
 e.x=in.x;e.y=in.y;e.z=in.z;e.room=in.room;e.stage=in.stage;e.seq=in.seq;e.health=in.post_health;e.action=in.action;e.snap=true;
 const bool had_pre=e.pre_valid;const int pre=e.pre_health;e.pre_valid=false;
 if(!e.owned){e.fury=0;e.group=false;return ev;}        // snapshot only: never manage colliders we do not verifiably own
 const bool terminal=in.post_health<=0||devf_variant::blocks_regen(in.action);
 std::uint8_t want=e.applied;
 if(e.variant==Variant::Black){
  if(terminal){if(e.fury){ev.kind=Ev::FuryEnd;ev.reason="TERMINAL_STATE_DEATH_FALL_WATER_OR_DEMO";}e.fury=0;}
  else if(had_pre&&in.post_health<pre){ev.kind=e.fury?Ev::FuryRefresh:Ev::FuryStart;ev.reason=e.fury?"REAL_HP_LOSS_RENEWS_TO_300_NO_STACK":"REAL_NON_LETHAL_HP_LOSS_IN_NATIVE_EXECUTE";e.fury=kFuryTicks;}
  else if(e.fury){if(--e.fury==0){ev.kind=Ev::FuryEnd;ev.reason="EXPIRED_300_SIM_TICKS";}}
  want=e.fury?kAtpBlackFury:kAtpBlackBase;ev.fury=e.fury;ev.pre_hp=pre;ev.post_hp=in.post_health;
 }else if(e.variant==Variant::Blue){
  const Entry* comp=nullptr;
  if(!terminal)comp=find_companion(L,e,in.seq,in.window,!e.group,verify);
  const bool g=comp!=nullptr;
  if(g!=e.group){
   ev.kind=g?Ev::BlueOn:Ev::BlueOff;e.group=g;
   if(g){ev.reason="LIVE_COMPANION_WITHIN_650_XZ_250_Y_SAME_ROOM";ev.companion=comp->id;ev.companion_variant=comp->variant;
    const double dx=double(e.x)-double(comp->x),dz=double(e.z)-double(comp->z);ev.dxz=sqrt_d(dx*dx+dz*dz);ev.dy=abs_d(double(e.y)-double(comp->y));}
   else ev.reason=terminal?"SELF_TERMINAL_STATE":"NO_ELIGIBLE_COMPANION_LEFT";
  }
  want=g?kAtpBlueGroup:kAtpBlueAlone;
 }else want=e.base;                                       // Red (and any future fixed variant): exactly its base, never a buff
 const std::uint8_t from=e.applied;
 if(const char* bad=reconcile(e,io,want)){
  e.fury=0;e.group=false;Event r;r.kind=Ev::Rejected;r.reason=bad;r.from=from;r.to=want;r.c0=io.get(0);r.c1=io.get(1);return r;
 }
 if(ev.kind!=Ev::None){ev.from=from;ev.to=e.applied;ev.c0=io.get(0);ev.c1=io.get(1);}
 return ev;
}
// ---------------- unload ----------------
struct RestorePlan {bool write=false;const char* why="NOTHING_TO_RESTORE";};
// Give back the exact NATIVE value (E_OC 1, E_OC2 2) only for an owned pair of a live, same, not-destroyed actor whose spheres still
// hold exactly what this module wrote last (base OR dynamic value).
inline RestorePlan restore_plan(const Entry& e,bool live_same_actor,std::uint8_t a0,std::uint8_t a1) noexcept {
 RestorePlan r;
 if(!e.used)return r;
 if(!e.owned){r.why="NOT_OWNED_NEVER_MANAGED_OR_CHANGED_BY_OTHERS";return r;}
 if(!live_same_actor){r.why="ACTOR_GONE_OR_REPLACED_NOTHING_READ";return r;}
 if(a0!=e.applied||a1!=e.applied){r.why="COLLIDER_CHANGED_BY_OTHERS_NOT_OWNED";return r;}
 if(e.applied==e.native){r.why="ALREADY_NATIVE_NO_WRITE";return r;}
 r.write=true;r.why="RESTORE_NATIVE_ATTACK_POWER";return r;
}
} // namespace devf_dyn
