#pragma once
// CP15 pure policy (no game headers): the first Blue AI. The mod and the lab run THIS code. Blue ONLY: Red and Black are never touched here.
//
// WHAT IT DOES, on top of the shared native daE_OC_c state machine (nothing is replaced; every effect is a bounded write to a native field):
//  1. GROUP LEVEL 1/2/3 = 1 + min(eligible companions, 2). A companion is ANY live classified Bokoblin (Blue, Red or Black) accepted by the unchanged CP10.6
//     filter devf_dyn::eligible (650 XZ / 250 Y, same room and stage, recent snapshot, alive, not terminal). 4 or more never add anything.
//  2. LEVEL 1 (solo) = EVASIVE: a solo Blue never approaches Link and, while Link is relevant, runs away along a heading probed as safe (walls, water, ledges,
//     leash). Native attack INITIATION is vetoed; an attack already in progress is never cancelled.
//  3. LEVEL 2 / 3 = AGGRESSIVE: the offensive animations (attack B = bck 5, attack C = bck 6) play faster (1.15x / 1.30x of the native rate 1.0) and the native
//     post-attack attack lockout shrinks in the same proportion (20 -> 17 / 15 ticks). Only the animation rate and that one timer change: damage, weapon
//     size, hit spheres, frames [14,22], movement table, HP and every other animation stay native.
//
// ENGINE FACTS (Dusklight 40457c6, src/d/actor/d_a_e_oc.cpp; see IMPLEMENTATION.md for the full reading):
//  * executeFind state 4 chases at speedF 20, turns shape_angle toward Link by clamp(diff/4, 0x100, 0x800) per tick, attacks when 200 < dist < 400 and the
//    facing is within 0x1000 of Link, and uses current.angle.y = field_0x6da instead of the facing while field_0x6ce != 0 (the native wall detour).
//  * FIND -> WAIT happens only when field_0x6c0 == 0 and searchPlayer2() fails; searchPlayer() needs the Blue to FACE Link (|diff| < 0x4000).
//  * J3DFrameCtrl advances mFrame by mRate per tick; checkPass() is rate aware; the contact window is `frame >= 14 && frame <= 22` read at the start of
//    each tick; the forward movement of the blow is a cumulative table lookup (cumulative, so it is rate independent).
#include "../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include <array>
#include <cstdint>
namespace devf_blue {
using devf_variant::Variant;
namespace dy=devf_dyn;
// Mirror of OC_ACTIONS (d_a_e_oc.cpp L20-37) through the CP10.3 enum.
constexpr int kModeWait=devf_variant::A_WAIT,kModeFind=devf_variant::A_FIND,kModeAttack=devf_variant::A_ATTACK,kModeFindStay=devf_variant::A_FIND_STAY,kModeMoveOut=devf_variant::A_MOVE_OUT;
constexpr int kFindApproach=4,kAttackB=1,kAttackC=2;       // executeFind state 4 (the chase) and executeAttack states 1 (bck 5) / 2 (bck 6)
// Side-table slot stamped with id + generation: a new generation (or another actor in the same slot) never inherits anything.
template<class S,std::size_t N> inline S& stamp(std::array<S,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 S& s=table[index<N?index:0];if(s.id!=id||s.generation!=generation){s=S{};s.id=id;s.generation=generation;}return s;
}
// ---------------------------------------------------------------- 1. group level
constexpr unsigned kMaxCompanions=2,kSoloConfirmTicks=3;
struct Group {unsigned companions=0;std::uint32_t ids[kMaxCompanions]{};constexpr unsigned level() const noexcept {return 1+companions;}};
// Counts up to kMaxCompanions DISTINCT eligible companions of `self`. `trusted` = how many were already verified on a previous tick (the first `trusted`
// candidates are not re-verified; any candidate beyond them is verified with `verify` before being counted, exactly like devf_dyn::find_companion does
// for the Blue bonus ON edge). A failed verification drops the snapshot until that actor executes again.
template<class V> inline Group scan_group(dy::Ledger& L,const dy::Entry& self,std::uint32_t seq,unsigned window,unsigned trusted,V&& verify) noexcept {
 Group g;
 for(auto& c:L.entries){
  if(!dy::eligible(self,c,seq,window))continue;
  if(g.companions>=trusted&&!verify(c)){c.snap=false;continue;}
  g.ids[g.companions++]=c.id;
  if(g.companions>=kMaxCompanions)break;
 }
 return g;
}
struct Tracker {unsigned level=1,companions=0,solo_run=kSoloConfirmTicks;std::uint32_t ids[kMaxCompanions]{};};   // a Blue is solo from its first instant
struct LevelChange {bool changed=false;unsigned from=1,to=1;};
// Any tick with a valid companion moves the level up (or between 2 and 3) at once; the way back to SOLO needs kSoloConfirmTicks consecutive solo ticks
// (0.1 s at 30 Hz): it absorbs a single transient stale snapshot / execution-order glitch without delaying a real death more than 3 ticks.
inline LevelChange track(Tracker& t,const Group& g) noexcept {
 LevelChange r;r.from=t.level;
 if(g.companions>0){
  t.solo_run=0;t.companions=g.companions;for(unsigned i=0;i<kMaxCompanions;++i)t.ids[i]=g.ids[i];
  if(t.level!=g.level()){t.level=g.level();r.changed=true;}
 }else{
  if(t.level==1){t.solo_run=kSoloConfirmTicks;t.companions=0;for(auto& i:t.ids)i=0;}
  else if(++t.solo_run>=kSoloConfirmTicks){t.level=1;t.companions=0;for(auto& i:t.ids)i=0;r.changed=true;}
 }
 r.to=t.level;return r;
}
// ---------------------------------------------------------------- 2. attack speed and lockout
constexpr float kRateNative=1.0f,kRatePair=1.15f,kRateGroup=1.30f;
constexpr unsigned kNativeLockout=20;                      // executeFind state 0: field_0x6c2 = field_0x6c0 + 0x14, with field_0x6c0 just set to 0
constexpr float rate_for(unsigned level) noexcept {return level>=3?kRateGroup:level==2?kRatePair:kRateNative;}
constexpr unsigned lockout_for(unsigned level) noexcept {return level>=3?15u:level==2?17u:kNativeLockout;}   // round(20 / rate): 17.39 -> 17, 15.38 -> 15
static_assert(kRateGroup>kRatePair&&kRatePair>kRateNative&&kRateNative==1.0f,"group 3 > group 2 > native");
static_assert(lockout_for(3)<lockout_for(2)&&lockout_for(2)<lockout_for(1)&&lockout_for(1)==kNativeLockout,"shorter lockout with a bigger group");
static_assert(lockout_for(2)==(kNativeLockout*100u+57u)/115u&&lockout_for(3)==(kNativeLockout*100u+65u)/130u,"lockout = native / rate, rounded");
constexpr bool applies_to(Variant v) noexcept {return v==Variant::Blue;}   // Red and Black are never accelerated nor made evasive
struct AtkView {int mode=0,state=0;bool anim_b=false,anim_c=false;float frame=0,play_speed=1.0f;};
enum class RateAction:std::uint8_t {None,Apply,SkipNotFresh,SkipForeignSpeed};
// At the POST of the execute that STARTED an attack: executeAttack state 0 ran setBck(5|6, 0, 5.0, 1.0) (rate 1.0, frame 0), selected state 1|2 and the end of
// execute advanced the frame once (play()). That is the only tick at which the rate is ours to set. Later ticks never write it again: the native end of
// the blow zeroes the rate, a shield recoil writes -1.0 (state 3), and the next setBck resets it to 1.0.
inline RateAction decide_rate(const AtkView& v,unsigned level,bool already_decided) noexcept {
 if(already_decided||v.mode!=kModeAttack)return RateAction::None;
 const bool b=v.state==kAttackB&&v.anim_b,c=v.state==kAttackC&&v.anim_c;
 if(!b&&!c)return RateAction::None;
 if(level<2)return RateAction::None;
 if(v.play_speed!=1.0f)return RateAction::SkipForeignSpeed;
 if(!(v.frame>=0.999f&&v.frame<=1.001f))return RateAction::SkipNotFresh;
 return RateAction::Apply;
}
// What the PRE saw (after its own writes): the POST compares against it to veto a native attack START of a solo Blue and to find the post-attack lockout.
struct PreSnap {bool valid=false,vetoable=false,expect_lockout=false;int mode=0,state=0,old=0;std::int16_t c0=0,c2=0;};
constexpr PreSnap make_snap(bool solo,bool link_present,float dist,int mode,int state,int old,std::int16_t c0,std::int16_t c2) noexcept {
 PreSnap s;s.valid=true;s.mode=mode;s.state=state;s.old=old;s.c0=c0;s.c2=c2;
 s.vetoable=solo&&link_present&&dist>=0.0f&&dist<1200.0f;                          // kEvadeRadius: a solo Blue with Link out of the relevance radius cannot start a blow anyway (native needs dist < 400)
 s.expect_lockout=mode==kModeFind&&state==0&&old==kModeAttack;                      // the tick that runs executeFind state 0 right after a blow
 return s;
}
struct Rollback {bool apply=false;int mode=0,old=0,state=0;std::int16_t c0=0,c2=0;};
// setActionMode(ATTACK, 0) happened during this execute (mode ATTACK, state 0, and the PRE mode was not ATTACK): restore the PRE values. setActionMode zeroed
// field_0x6c0 / field_0x6c2, so give back what the native head decrement of the PRE values leaves. An attack that was already running (PRE mode ATTACK) is never touched.
constexpr Rollback veto_attack_start(const PreSnap& s,int mode,int state) noexcept {
 Rollback r;
 if(!s.valid||!s.vetoable||mode!=kModeAttack||state!=0||s.mode==kModeAttack)return r;
 r.apply=true;r.mode=s.mode;r.old=s.old;r.state=s.state;r.c0=static_cast<std::int16_t>(s.c0>0?s.c0-1:0);r.c2=static_cast<std::int16_t>(s.c2>0?s.c2-1:0);
 return r;
}
constexpr bool in_blow(int mode,int state) noexcept {return mode==kModeAttack&&(state==kAttackB||state==kAttackC);}   // the rate decision is per blow: forget it outside states 1/2
// executeFind state 0 right after a blow sets field_0x6c2 = 20; return the scaled value, or -1 when nothing is to be written.
constexpr int lockout_write(const PreSnap& s,int mode,std::int16_t c2,unsigned level) noexcept {
 if(!s.valid||!s.expect_lockout||mode!=kModeFind||c2!=static_cast<std::int16_t>(kNativeLockout))return -1;
 return lockout_for(level)!=kNativeLockout?int(lockout_for(level)):-1;
}
// ---------------------------------------------------------------- 3. evasion (solo Blue)
constexpr float kEvadeRadius=1200.0f;                      // l_HIO.battle_participation_radius = plyr_srch_max_radius_S (d_a_e_oc.cpp L95-105)
constexpr float kEvadeHysteresis=300.0f;                   // a running Blue stops only at kEvadeRadius + this (and resumes below kEvadeRadius): no stop-go dithering at the edge
constexpr float kFleeSpeed=20.0f;                          // the native chase speed (executeFind state 4)
constexpr float kRampStep=2.0f;                            // speed gained per tick while starting to run (native accelerates by 1)
constexpr std::int16_t kTurnStep=0x2000;                   // body turn per tick toward the flee heading (the native counter-turn is at most 0x800)
constexpr int kTurnedBack=0x3000;                          // facing within this of Link counts as having turned back (searchPlayer needs < 0x4000)
constexpr unsigned kGraceTicks=30;                        // after the evasion ends in the native chase state, keep the alert while the Blue turns back to Link (the native turn is 0x800 per tick: 16 ticks for 180 degrees)
constexpr unsigned kReeval=3;                              // ticks between full heading evaluations
constexpr std::int16_t kOffsetStep=5461;                   // 30 degrees
constexpr std::int16_t kMaxOffset=16384;                   // 90 degrees: never a heading with a component toward Link
static_assert(kEvadeRadius==1200.0f,"make_snap hard-codes the same radius");
constexpr float kProbeWall=300.0f;                         // reach of the native checkBeforeBg line
constexpr float kMaxStep=100.0f;                           // largest ground height step between two samples
constexpr unsigned kSamples=3;
constexpr float kSampleDist[kSamples]={150.0f,300.0f,450.0f};
// ---- integer-angle math without any CRT import: s16 angle, 0 = +Z, positive toward +X (same convention as cM_atan2s / cM_ssin / cM_scos) ----
constexpr float kPi=3.14159265358979f;
constexpr float sin_rad(float x) noexcept {   // |x| <= pi
 if(x>kPi/2)x=kPi-x;else if(x<-kPi/2)x=-kPi-x;
 const float x2=x*x;
 return x*(1.0f+x2*(-1.0f/6+x2*(1.0f/120+x2*(-1.0f/5040+x2*(1.0f/362880+x2*(-1.0f/39916800))))));
}
constexpr float sin_s16(std::int16_t a) noexcept {return sin_rad(float(a)*(kPi/32768.0f));}
constexpr float cos_s16(std::int16_t a) noexcept {return sin_s16(static_cast<std::int16_t>(a+0x4000));}
constexpr float atan_unit(float t) noexcept {   // t in [0,1], error < 1e-5 rad
 const float t2=t*t;
 return t*(0.9998660f+t2*(-0.3302995f+t2*(0.1801410f+t2*(-0.0851330f+t2*0.0208351f))));
}
constexpr std::int16_t atan2s(float x,float z) noexcept {   // angle of the vector (x, z): 0 = +Z, positive toward +X
 const float ax=x<0?-x:x,az=z<0?-z:z;
 if(ax==0.0f&&az==0.0f)return 0;
 float r=ax<=az?atan_unit(ax/az):kPi/2-atan_unit(az/ax);   // angle from +Z toward +X, first quadrant
 if(z<0)r=kPi-r;
 if(x<0)r=-r;
 const float u=r*(32768.0f/kPi);
 const long v=long(u>=0?u+0.5f:u-0.5f);
 return static_cast<std::int16_t>(v);
}
constexpr std::int16_t angle_diff(std::int16_t a,std::int16_t b) noexcept {return static_cast<std::int16_t>(a-b);}
constexpr int abs_i(int v) noexcept {return v<0?-v:v;}
constexpr int abs_diff(std::int16_t a,std::int16_t b) noexcept {return abs_i(angle_diff(a,b));}
// ---- speed pre-compensation: the native chase runs AFTER our PRE write, so write the value that ends up as the desired one ----
constexpr float native_find_target(std::int16_t c2,float d) noexcept {   // executeFind state 4, no event (L1200-1244)
 if(c2==0)return d<300.0f?-3.0f:20.0f;
 return (d<400.0f&&d>200.0f)?0.0f:d<300.0f?-3.0f:20.0f;
}
constexpr float pre_speed_for(float desired,float target) noexcept {return target>desired?desired-1.0f:target<desired?desired+1.0f:desired;}   // cLib_chaseF step 1.0
// ---- probing a candidate heading (all engine queries live in the adapter; this is only the verdict) ----
struct Probe {
 std::uint8_t wall=0;                                      // daE_OC_c::checkBeforeBg(): 0 free, 1 wall < 100, 2/3 wall between 100 and 300
 bool water200=false,water400=false;                       // daE_OC_c::checkBeforeFloorBg(200 / 400)
 bool found[kSamples]{};float ground_y[kSamples]{};        // fopAcM_gc_c ground height ahead at 150 / 300 / 450 (found = a ground exists)
};
enum class Cls:std::uint8_t {Blocked,Loose,Clear};
enum class Why:std::uint8_t {Ok,Wall,Water,Ledge,Leash};
struct Verdict {Cls cls=Cls::Blocked;Why why=Why::Ok;};
inline Verdict classify(const Probe& p,float cur_y) noexcept {
 Verdict v;
 if(p.wall==1){v.why=Why::Wall;return v;}
 if(p.water200){v.why=Why::Water;return v;}
 float prev=cur_y;
 for(unsigned i=0;i<kSamples;++i){
  if(!p.found[i]){v.why=Why::Ledge;return v;}
  const float dy=p.ground_y[i]-prev;
  if(dy<-kMaxStep||dy>kMaxStep){v.why=Why::Ledge;return v;}
  prev=p.ground_y[i];
 }
 v.cls=(p.wall==0&&!p.water400)?Cls::Clear:Cls::Loose;
 return v;
}
struct Leash {bool active=false;float hx=0,hz=0,range=0;};
inline bool within_leash(const Leash& l,float x,float z) noexcept {
 if(!l.active||l.range<=0.0f)return true;
 const double dx=double(x)-double(l.hx),dz=double(z)-double(l.hz);
 return dx*dx+dz*dz<=double(l.range)*double(l.range);
}
struct Self {float x=0,y=0,z=0;};
struct Choice {bool ok=false;std::int16_t heading=0;Cls cls=Cls::Blocked;int offset=0;unsigned probed=0,wall=0,water=0,ledge=0,leash=0;};
// Best-first choice of a flee heading around `away` (the direction from Link to the Blue): the previous heading if still safe and within 90 degrees, then the
// offsets 0, +-30, +-60, +-90 (the preferred side first). First CLEAR heading wins; otherwise the first LOOSE one (a wall further than 100 ahead); otherwise
// none: the caller holds position (fail safe). `probe(heading)` returns the engine verdict inputs for that heading.
template<class P> inline Choice choose(std::int16_t away,const Self& s,const Leash& leash,std::int16_t prev,bool prev_valid,int side,P&& probe) noexcept {
 Choice best;const int sg=side<0?-1:1;
 auto test=[&](std::int16_t h,int offset)->bool{
  ++best.probed;
  const float ex=s.x+sin_s16(h)*kProbeWall,ez=s.z+cos_s16(h)*kProbeWall;
  if(!within_leash(leash,ex,ez)){++best.leash;return false;}
  const Probe p=probe(h);const Verdict v=classify(p,s.y);
  if(v.cls==Cls::Blocked){if(v.why==Why::Wall)++best.wall;else if(v.why==Why::Water)++best.water;else ++best.ledge;return false;}
  if(v.cls==Cls::Clear){best.ok=true;best.heading=h;best.cls=Cls::Clear;best.offset=offset;return true;}
  if(!best.ok){best.ok=true;best.heading=h;best.cls=Cls::Loose;best.offset=offset;}   // keep looking for a CLEAR one
  return false;
 };
 if(prev_valid&&abs_diff(prev,away)<=kMaxOffset){if(test(prev,angle_diff(prev,away)))return best;}
 for(int k=0;k<=3;++k){
  const std::int16_t o=static_cast<std::int16_t>(k*kOffsetStep);
  if(test(static_cast<std::int16_t>(away+sg*o),sg*o))return best;
  if(k>0&&test(static_cast<std::int16_t>(away-sg*o),-sg*o))return best;
 }
 return best;
}
// ---- per-actor evasion state and its single step ----
struct Evade {
 bool active=false,holding=false,far_hold=false,heading_valid=false,hold_pending=false;std::int16_t heading=0;unsigned since=0,ticks=0,hold_ticks=0,grace_left=0;int side=1,turn_sign=0;float d_start=0;
};
enum class EvEv:std::uint8_t {None,Start,End,Hold,Resume,Retarget};
struct EvIn {
 bool eligible=false;                 // classified Blue, SOLO (tracker level 1), native FIND state 4, no event, Link valid, not outside the leash, not terminal
 bool grace=false;                    // native FIND state 4, no event, not outside the leash, alive: the native chase is back in charge (the Blue is no longer solo)
 Self self;float lx=0,lz=0,dist=0;    // Blue position, Link XZ, native 3D distance (fopAcM_searchPlayerDistance)
 std::int16_t shape_y=0,c2=0;float speed=0;
 Leash leash;
};
struct Writes {
 bool turn=false,detour=false,hold_timer=false,speed=false;std::int16_t shape_y=0,heading=0;float speed_pre=0;
};
struct EvOut {EvEv ev=EvEv::None;Writes w;Choice choice;float dist_start=0;unsigned ticks=0;const char* reason="NONE";};
// One PRE. `full(heading)` -> Probe; `water_now(angle)` -> bool: the native state 4 test checkBeforeFloorBg(200) looks along shape_angle.y, so the body must never be
// written to a direction with deep water 200 ahead (the sweep of a 180 degree turn would otherwise turn the chase into FIND_STAY).
template<class F,class W> inline EvOut step_evade(Evade& e,const EvIn& in,F&& full,W&& water_now) noexcept {
 EvOut o;
 if(!in.eligible){
  unsigned grace=e.grace_left;
  if(e.active){o.ev=EvEv::End;o.reason="CONDITIONS_NO_LONGER_HOLD";o.dist_start=e.d_start;o.ticks=e.ticks;grace=in.grace?kGraceTicks:0u;}
  e=Evade{};
  if(grace&&in.grace&&abs_diff(in.shape_y,atan2s(in.lx-in.self.x,in.lz-in.self.z))>=kTurnedBack){o.w.hold_timer=true;e.grace_left=grace-1;}
  return o;
 }
 const float target=native_find_target(in.c2,in.dist);
 if(!e.active){e=Evade{};e.active=true;e.d_start=in.dist;e.far_hold=in.dist>=kEvadeRadius;o.ev=EvEv::Start;o.reason="SOLO_BLUE_IN_NATIVE_CHASE";}
 ++e.ticks;
 const bool far_now=e.far_hold?in.dist>=kEvadeRadius:in.dist>=kEvadeRadius+kEvadeHysteresis;
 if(far_now){   // alert but Link is not relevant: do not approach him (the native chase would), do not flee either
  const bool was=!e.far_hold;e.far_hold=true;e.holding=false;e.heading_valid=false;
  o.w.speed=true;o.w.speed_pre=pre_speed_for(0.0f,target);
  // We arrive at the radius with our back to Link; the native FIND -> WAIT test (searchPlayer needs the Blue to FACE Link) would fire at once and the Blue would
  // stop noticing Link altogether. Keep it alert only while it turns around (the native state 4 turn does the turning); afterwards the native rules decide.
  o.w.hold_timer=abs_diff(in.shape_y,atan2s(in.lx-in.self.x,in.lz-in.self.z))>=kTurnedBack;
  if(was&&o.ev==EvEv::None){o.ev=EvEv::Hold;o.reason="LINK_BEYOND_RELEVANCE_RADIUS_NO_APPROACH";}
  return o;
 }
 const bool was_far=e.far_hold;e.far_hold=false;
 if(was_far&&o.ev==EvEv::None){o.ev=EvEv::Resume;o.reason="LINK_ENTERED_RELEVANCE_RADIUS";}
 const std::int16_t away=atan2s(in.self.x-in.lx,in.self.z-in.lz);
 bool eval=e.holding?e.since>=kReeval:(!e.heading_valid||e.since>=kReeval||abs_diff(e.heading,away)>kMaxOffset);
 if(eval){
  const Choice c=choose(away,in.self,in.leash,e.heading,e.heading_valid,e.side,full);
  o.choice=c;e.since=0;
  if(c.ok){
   const bool changed=!e.heading_valid||abs_diff(c.heading,e.heading)>kOffsetStep/2;
   if(c.offset!=0)e.side=c.offset<0?-1:1;
   e.heading=c.heading;e.heading_valid=true;
   if(e.holding){e.holding=false;if(o.ev==EvEv::None){o.ev=EvEv::Resume;o.reason="SAFE_HEADING_FOUND_AGAIN";}}
   else if(changed&&o.ev==EvEv::None&&e.ticks>1){o.ev=EvEv::Retarget;o.reason=c.cls==Cls::Clear?"HEADING_RECHOSEN_CLEAR":"HEADING_RECHOSEN_LOOSE_WALL_AHEAD";}
  }else{
   e.heading_valid=false;
   if(!e.holding){e.holding=true;if(o.ev==EvEv::None){o.ev=EvEv::Hold;o.reason="NO_SAFE_HEADING_HOLD_POSITION";}else e.hold_pending=true;}   // report it next tick if another event took this one
  }
 }else ++e.since;
 if(e.holding){
  ++e.hold_ticks;
  if(e.hold_pending&&o.ev==EvEv::None){o.ev=EvEv::Hold;o.reason="NO_SAFE_HEADING_HOLD_POSITION";e.hold_pending=false;}
  o.w.hold_timer=true;o.w.speed=true;o.w.speed_pre=pre_speed_for(0.0f,target);
  return o;
 }
 // FLEE: face the heading (bounded turn per tick), move along it exactly (native detour fields), keep the native chase from giving up, run at the native speed.
  o.w.heading=e.heading;
 {
  // Turn the body toward the heading in bounded steps. The sense is the shortest one unless the next step would face deep water; then the long way round; if both
  // are unsafe the body is left as it is this tick (the run itself is steered by the detour fields, not by the facing).
  const int dif=angle_diff(e.heading,in.shape_y);
  int sign=e.turn_sign?e.turn_sign:(dif<0?-1:1);
  auto next_for=[&](int sg)->std::int16_t{
   const int rem=sg>0?(static_cast<std::uint16_t>(e.heading-in.shape_y)):(static_cast<std::uint16_t>(in.shape_y-e.heading));
   return rem<=kTurnStep?e.heading:static_cast<std::int16_t>(in.shape_y+sg*kTurnStep);
  };
  if(in.shape_y==e.heading){e.turn_sign=0;o.w.turn=false;}
  else{
   std::int16_t next=next_for(sign);
   if(water_now(next)){sign=-sign;next=next_for(sign);e.turn_sign=sign;if(water_now(next)){next=in.shape_y;e.turn_sign=0;}}
   o.w.turn=next!=in.shape_y;o.w.shape_y=next;
   if(next==e.heading)e.turn_sign=0;
  }
 }
 o.w.detour=true;o.w.hold_timer=true;
 float want=(in.speed>0.0f?in.speed:0.0f)+kRampStep;if(want>kFleeSpeed)want=kFleeSpeed;
 o.w.speed=true;o.w.speed_pre=pre_speed_for(want,target);
 return o;
}
} // namespace devf_blue
