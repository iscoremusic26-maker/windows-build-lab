#pragma once
// CP16 retains CP15 group/rate/lockout code. Solo evasion and POST attack veto are removed.
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
constexpr bool applies_to(Variant v) noexcept {return v==Variant::Blue;}   // Red and Black are never accelerated
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
// What the PRE saw: POST only identifies the existing post-attack lockout. No veto or mode rollback.
struct PreSnap {bool valid=false,vetoable=false,expect_lockout=false;int mode=0,state=0,old=0;std::int16_t c0=0,c2=0;};
constexpr PreSnap make_snap(bool solo,bool link_present,float dist,int mode,int state,int old,std::int16_t c0,std::int16_t c2) noexcept {
 PreSnap s;s.valid=true;s.mode=mode;s.state=state;s.old=old;s.c0=c0;s.c2=c2;
 s.vetoable=false; // Kept as internal snapshot compatibility only; never authorizes a write.
 s.expect_lockout=mode==kModeFind&&state==0&&old==kModeAttack;                      // the tick that runs executeFind state 0 right after a blow
 return s;
}
constexpr bool in_blow(int mode,int state) noexcept {return mode==kModeAttack&&(state==kAttackB||state==kAttackC);}   // the rate decision is per blow: forget it outside states 1/2
// executeFind state 0 right after a blow sets field_0x6c2 = 20; return the scaled value, or -1 when nothing is to be written.
constexpr int lockout_write(const PreSnap& s,int mode,std::int16_t c2,unsigned level) noexcept {
 if(!s.valid||!s.expect_lockout||mode!=kModeFind||c2!=static_cast<std::int16_t>(kNativeLockout))return -1;
 return lockout_for(level)!=kNativeLockout?int(lockout_for(level)):-1;
}
}
