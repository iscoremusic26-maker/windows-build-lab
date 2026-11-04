#pragma once
#include "cp05_fix02_core.hpp"
namespace cp05fix02 {
struct Sample {
 cp04::Key key{};std::uint64_t tick=0;int mode=-1,state=-1,old=-1,t0=0,t2=0,bck=0,hp=0,atp=0,link_proc=-1,facing=0,variant=4;
 float frame=0,rate=0,end=0,speed=0,range2=-1,height=0;
 bool valid=false,input=false,held=false,guard=false,penalty=false,sim=false,target=false,battle=false,stop=false,at=false,stale_link_hit=false,contact=false,parry=false;
};
enum class GuardEvent:std::uint8_t{None,Start,Persist,End,Context,Expired,EarlyRebound};
inline const char* name(GuardEvent e)noexcept{const char* a[]={"NONE","START","PERSIST","END","CONTEXT_END","EXPIRED_INCONCLUSIVE","STALE_EARLY_REBOUND"};return a[unsigned(e)];}
inline const char* wait_reason(const Sample& s)noexcept{
 if(!s.input)return "INPUT_SAMPLE_MISSING";if(!s.sim||!s.target)return "SIMULATION_OR_TARGET_UNAVAILABLE";if(s.penalty)return "LINK_GUARD_BREAK_OR_REARM";
 if(s.mode==5||s.mode==6||s.mode>=9||s.mode==7||s.mode==8)return "NATIVE_PRIORITY_OR_RECOVERY";
 if(s.mode==4)return s.state==3?"NATIVE_SHIELD_RECOIL":s.stop?"NATIVE_TERMINAL_EXIT_PENDING":"NATIVE_OFFENSIVE_CYCLE";
 if(s.range2<40000||s.range2>160000)return "OUTSIDE_NATIVE_FIND_STRIKE_BAND_OR_EXPANSION_APPROACH";
 if(s.mode==3&&s.t2>0)return "NATIVE_OR_OWNED_COMBO_COUNTDOWN";
 return "OBSERVATION_ONLY_NOT_A_STALL_VERDICT";
}
class GuardMonitor {
 public:
 struct Row {bool bound=false,on=false;cp04::Key key{};std::uint64_t last_tick=0,own=0,start=0,last_attempt=0,last_at=0,last_contact_tick=0,last_parry_tick=0;unsigned attempts=0,arms=0,transitions=0,timer_progress=0,early=0,lines=0;Sample last{};};
 struct Counts {std::uint64_t samples=0,duplicates=0,starts=0,ends=0,expired=0,contexts=0,early=0,persistent=0,attempts=0,arms=0,by_variant[5]{},duration_hist[8]{};}c;
 GuardEvent feed(unsigned slot,const Sample& pre,const Sample& post)noexcept{
  if(slot>=64||!post.valid)return GuardEvent::None;auto& r=rows_[slot];
  if(r.bound&&!(r.key==post.key)){if(r.on){finish(r);++c.contexts;}r=Row{};}
  if(!r.bound){r.bound=true;r.key=post.key;}if(r.last_tick==post.tick){++c.duplicates;return GuardEvent::None;}
  if(post.contact)r.last_contact_tick=post.tick;if(post.parry)r.last_parry_tick=post.tick;
  ++c.samples;const bool consecutive=r.last_tick&&post.tick==r.last_tick+1;r.last_tick=post.tick;r.last=post;
  if(!post.sim||!post.input||!post.target){if(r.on){finish(r);++c.expired;return GuardEvent::Expired;}return GuardEvent::None;}
  ++r.own;if(!post.held){if(r.on){finish(r);++c.ends;return GuardEvent::End;}return GuardEvent::None;}
  if(!r.on){r.on=true;r.start=r.own;++c.starts;++c.by_variant[unsigned(post.variant)<5?unsigned(post.variant):4];return GuardEvent::Start;}
  if(consecutive&&pre.valid&&pre.key==post.key&&pre.tick==post.tick){
   if(pre.mode!=post.mode||pre.state!=post.state)++r.transitions;
   if(post.t0<pre.t0||post.t2<pre.t2)++r.timer_progress;
   if(pre.mode==4&&pre.state==0){++r.attempts;++c.attempts;r.last_attempt=r.own;}
   if(post.at){++r.arms;++c.arms;r.last_at=r.own;}
   if(pre.mode==4&&(pre.state==1||pre.state==2)&&pre.frame<14&&pre.rate>0&&pre.stale_link_hit&&post.mode==4&&post.state==3){++r.early;++c.early;return GuardEvent::EarlyRebound;}
  }
  if((r.own-r.start)%120==0){++c.persistent;return GuardEvent::Persist;}return GuardEvent::None;
 }
 bool expire(unsigned i,std::uint64_t tick)noexcept{if(i>=64)return false;auto& r=rows_[i];if(r.on&&tick>r.last_tick+60){finish(r);++c.expired;return true;}return false;}
 void logged(unsigned i)noexcept{if(i<64)++rows_[i].lines;}
 const Row& row(unsigned i)const noexcept{return rows_[i<64?i:0];}
 void shutdown()noexcept{for(auto& r:rows_)if(r.on){finish(r);++c.ends;}}
 private:
 void finish(Row& r)noexcept{const auto n=r.own-r.start;const unsigned b=n<30?0:n<60?1:n<120?2:n<240?3:n<480?4:n<960?5:n<1920?6:7;++c.duration_hist[b];r.on=false;}
 Row rows_[64]{};
};
}
