#pragma once
// CP04 owns selected Blue attacks only. No engine RNG, addresses or frame clock.
#include <cstdint>
namespace cp04 {
inline constexpr unsigned kSlots=64, kSwings=4, kModerateGap=8, kShortGap=2,kWindupMax=32;
// CP06 0.11.11: opening pair 1.10 (was 1.00), approved closing pair unchanged at 1.50.
inline constexpr float kOpenRate=1.10f,kFastRate=1.5f;
// A bijective mixer and counter stream; the high bit partitions all 2^64
// inputs into equal halves. Independent identity streams; never engine RNG.
inline std::uint64_t mix(std::uint64_t x) noexcept {
 x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
 x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);
}
inline float absf(float x) noexcept{return x<0?-x:x;}
inline bool near(float x,float y) noexcept{return absf(x-y)<.002f;}
inline bool normal_rate(float x) noexcept{return x==1.f||x==1.15f||x==1.30f;}
inline bool finite(float x) noexcept{return x==x&&x>-100000.f&&x<100000.f;}
enum class Phase:std::uint8_t {Idle,Selected,Swing,Gap,Recovery};
struct Key {
 std::uint32_t id=0,epoch=0,stage=0;std::uint64_t generation=0;int room=-1,layer=-1;
 bool operator==(const Key& o)const noexcept{return id==o.id&&epoch==o.epoch&&stage==o.stage&&generation==o.generation&&room==o.room&&layer==o.layer;}
};
struct Counters {
 std::uint64_t opportunities=0,selected=0,normal=0,not_started=0,completed=0,cancelled=0,breaks=0;
 std::uint64_t starts[4]{},contacts[4]{},parries[4]{},armed[4]{},missed=0,blocked=0,damaged=0;
 std::uint64_t rates=0,foreign=0,released=0,duplicate=0,context=0,paused=0;
 std::uint64_t gap_n[3]{},gap_min[3]{},gap_max[3]{},at_n[3]{},at_min[3]{},at_max[3]{};
};
struct Combo {
 Key key{};bool bound=false;Phase phase=Phase::Idle;
 std::uint64_t seed=0,rng=0,opportunity=0,combo_id=0,token=0,last_decision=0,last_execute=0,local_tick=0;
 std::uint64_t start_tick=0,gap_tick=0,contact_tick=0,last_contact=0;
 unsigned swing=0,gap=0;bool rate_owned=false,at_seen=false,contact_seen=false,gap_initialized=false;
 bool attack_entered=false,will_finish=false,start_pending=false,launch_requested=false;std::uint64_t launch_tick=0;
 std::uint64_t previous_arm_tick=0;unsigned previous_arm_swing=0;
 unsigned origin=0,group_hint=0;bool at_live=false;int bck=0;float end=0,rate=0,last_frame=0,attack_frame=0,handoff_rate=0;
 bool active()const noexcept{return phase!=Phase::Idle;}
 void bind(const Key& k)noexcept{
  *this=Combo{};bound=true;key=k;
  seed=mix((std::uint64_t(k.id)<<32)^k.generation^(std::uint64_t(k.stage)<<1)^k.epoch^
           (std::uint64_t(unsigned(k.room))<<16)^unsigned(k.layer));rng=seed;
 }
 bool execute(std::uint64_t stamp)noexcept{
  if(stamp==last_execute)return false;last_execute=stamp;++local_tick;return true;
 }
 // Exactly one draw per actual executeAttack/0 with chaseF reaching zero.
 bool decide(std::uint64_t stamp,bool eligible,Counters& c)noexcept{
  if(!eligible||active()||stamp==last_decision){if(stamp==last_decision)++c.duplicate;return false;}
  last_decision=stamp;++opportunity;++c.opportunities;
  rng+=0x9e3779b97f4a7c15ULL;bool chosen=(mix(rng)>>63)!=0;
  if(chosen){++c.selected;phase=Phase::Selected;combo_id=opportunity;swing=0;}
  else ++c.normal;return chosen;
 }
 float wanted_rate()const noexcept{return swing<=2?kOpenRate:kFastRate;}
 bool begin(int clip,float actual_end,float frame,float observed_rate,Counters& c)noexcept{
  if((phase!=Phase::Selected&&phase!=Phase::Gap)||swing>=kSwings||
     (clip!=5&&clip!=6)||!finite(actual_end)||actual_end<=22.f||actual_end>300.f||
     !near(frame,1.f)||!normal_rate(observed_rate))return false;
  if(phase==Phase::Gap&&swing>=1&&swing<=3){unsigned i=swing-1;auto delta=local_tick-gap_tick;++c.gap_n[i];if(c.gap_n[i]==1||delta<c.gap_min[i])c.gap_min[i]=delta;if(delta>c.gap_max[i])c.gap_max[i]=delta;}
  if(phase==Phase::Selected){previous_arm_tick=0;previous_arm_swing=0;}
  ++swing;++token;++c.starts[swing-1];phase=Phase::Swing;bck=clip;end=actual_end;
  handoff_rate=observed_rate;group_hint=near(observed_rate,1.30f)?3:near(observed_rate,1.15f)?2:1;
  rate=wanted_rate();at_live=false;rate_owned=false;at_seen=contact_seen=false;start_pending=false;
  launch_requested=false;last_frame=frame;start_tick=local_tick;return true;
 }
 void finish(Counters& c)noexcept{
  if(phase!=Phase::Swing)return;
  if(!contact_seen)++c.missed;if(rate_owned)++c.released;rate_owned=false;
  if(swing==kSwings){++c.completed;phase=Phase::Recovery;return;}
  phase=Phase::Gap;gap=swing==3?kShortGap:kModerateGap;gap_tick=local_tick;gap_initialized=false;launch_requested=false;
 }
 unsigned remaining()const noexcept{
  auto elapsed=local_tick-gap_tick;return elapsed>=gap?0:gap-unsigned(elapsed);
 }
 void cancel(Counters& c,bool broken=false)noexcept{
  if(!active())return;if(phase==Phase::Selected)++c.not_started;
  ++c.cancelled;if(broken)++c.breaks;if(rate_owned)++c.released;
  phase=Phase::Idle;rate_owned=false;start_pending=false;launch_requested=false;attack_entered=false;will_finish=false;
 }
};
}
