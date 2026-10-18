#pragma once
// CP16 real policy shared by DLL and laboratory. Orders contain IDs/generations/values, never actor pointers.
#include "blue_ai_policy.hpp"
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_recruit {
using Variant=devf_variant::Variant;
constexpr unsigned kCapacity=64,kRecall=150,kCalm=150,kBudget=180,kMaxBudget=240,kNoProgress=60,kBackoff=90,kRefresh=15,kFreshGrace=2;
constexpr double kRadius=1800.,kHeight=250.,kMargin=100.;
constexpr int Wait=devf_variant::A_WAIT,Walk=devf_variant::A_WALK,Find=devf_variant::A_FIND,Attack=devf_variant::A_ATTACK,Watch=devf_variant::A_WATCH,Sound=devf_variant::A_SOUND_WATCH,FindStay=devf_variant::A_FIND_STAY;
struct Point{float x=0,y=0,z=0;};
inline bool finite(float x) noexcept{return x==x&&x>=-10000000.f&&x<=10000000.f;}
inline bool finite(Point p)noexcept{return finite(p.x)&&finite(p.y)&&finite(p.z);}
inline double xz2(Point a,Point b)noexcept{double x=double(a.x)-b.x,z=double(a.z)-b.z;return x*x+z*z;}
inline double abs_d(double x)noexcept{return x<0?-x:x;}
struct Frame{
 std::uint32_t id=0,gen=0,seq=0;Variant variant=Variant::None;char stage[9]{};int room=-1,layer=-1,mode=-1,state=-1,health=0;
 Point pos{},home{};float range=0;bool ready=false,hit=false,leash_out=false;
};
inline bool context(const Frame&a,const Frame&b)noexcept{return a.room==b.room&&a.layer==b.layer&&a.layer>=0&&a.layer<15&&a.stage[0]&&std::memcmp(a.stage,b.stage,9)==0;}
inline bool usable(const Frame&f)noexcept{return f.ready&&f.id&&f.gen&&f.variant!=Variant::None&&f.health>0&&f.room>=0&&f.room<64&&f.layer>=0&&f.layer<15&&f.stage[0]&&finite(f.pos)&&finite(f.home)&&finite(f.range)&&f.range>=0&&!devf_variant::blocks_regen(f.mode);}
inline bool fresh(const Frame&f,std::uint32_t seq,unsigned window)noexcept{return std::uint32_t(seq-f.seq)<=window;}
inline bool idle(int m)noexcept{return m==Wait||m==Walk;}
inline bool fighting(int m)noexcept{return m==Find||m==Attack||m==FindStay;}
inline bool leash(const Frame&f,Point goal)noexcept{
 if(!finite(goal)||!finite(f.home)||!finite(f.range)||f.range<0||f.leash_out)return false;
 if(f.range==0)return true; // Native action() skips its radius branch for zero. Context/radius/height remain mandatory.
 return f.range>=kMargin&&xz2(f.home,goal)<=double(f.range-kMargin)*(f.range-kMargin)&&xz2(f.home,f.pos)<=double(f.range)*f.range;
}
inline bool radius(const Frame&a,const Frame&b,double r)noexcept{return context(a,b)&&xz2(a.pos,b.pos)<=r*r&&abs_d(double(a.pos.y)-b.pos.y)<=kHeight;}
struct Order{
 bool active=false,consumed=false,moved=false,arrived=false,extended=false,engaged=false;
 std::uint32_t caller=0,caller_gen=0;unsigned offered_tick=0,started=0,moving_since=0,progress_tick=0,checked=0,refreshed=0;
 Point start{},goal{};double best=0;
};
struct Combat{bool active=false,shout_pending=false,shout_used=false,shout_live=false,waiting=false;unsigned last_threat=0,next_call=0,wait_start=0,backoff_until=0,shout_start=0;};
struct Entry{Frame f{};unsigned ticks=0,retry_after=0;Order order{};Combat combat{};};
struct Table{
 std::array<Entry,kCapacity> entries{};
 Entry* find(std::uint32_t id,std::uint32_t gen)noexcept{for(auto& e:entries)if(e.f.id==id&&e.f.gen==gen)return &e;return nullptr;}
 Entry& publish(unsigned slot,const Frame& f)noexcept{
  auto& e=entries[slot<kCapacity?slot:0];
  if(e.f.id!=f.id||e.f.gen!=f.gen||(e.f.id&&!context(e.f,f))){e={};}
  e.f=f;++e.ticks;return e;
 }
};
template<class Emit>inline void abort(Entry& r,Emit&& emit,const char* why)noexcept{
 if(!r.order.active)return;emit("RECRUIT_ABORTED",r.order.caller,r.order.caller_gen,r.f,why);r.order={};r.retry_after=r.ticks+kBackoff;
}
inline bool group(Table& t,const Entry& self,std::uint32_t seq,unsigned window)noexcept{
 for(const auto& c:t.entries)if(c.f.id!=self.f.id&&usable(c.f)&&fresh(c.f,seq,window)&&radius(self.f,c.f,650.))return true;return false;
}
struct Call {bool issued=false;unsigned offered=0,leash_skipped=0,busy=0;};
template<class Emit>inline Call call(Table&t,Entry& s,bool detected,std::uint32_t seq,unsigned window,Emit&& emit)noexcept{
 Call out;auto& c=s.combat;
 if(!usable(s.f)||s.f.variant!=Variant::Blue)return out;
 bool incoming=s.order.active;
 if(detected||fighting(s.f.mode)||incoming)c.last_threat=s.ticks;
 if(c.active&&!detected&&!fighting(s.f.mode)&&!incoming&&s.ticks-c.last_threat>=kCalm){c={};emit("BLUE_COMBAT_RESET",s.f.id,s.f.gen,s.f,"150_CALM_TICKS_NO_THREAT_OR_ORDER");}
 // A recruited Blue cannot propagate an order. Own direct native detection may call after it enters combat and its order ends.
 if(!detected||incoming)return out;
 if(!c.active){c.active=true;c.shout_pending=true;c.next_call=s.ticks;c.last_threat=s.ticks;emit("BLUE_LINK_DETECTED",s.f.id,s.f.gen,s.f,"OWN_NATIVE_SEARCH_PLAYER_NOT_COMPANION_CASCADE");}
 if(s.ticks<c.next_call)return out;c.next_call=s.ticks+kRecall;out.issued=true;
 for(auto& r:t.entries){
  if(r.f.id==s.f.id||!usable(r.f)||!fresh(r.f,seq,window)||!radius(s.f,r.f,kRadius)||!idle(r.f.mode)||r.f.hit)continue;
  if(!leash(r.f,s.f.pos)){++out.leash_skipped;emit("RECRUIT_SKIPPED_NATIVE_LEASH",s.f.id,s.f.gen,r.f,"DESTINATION_OUTSIDE_OWN_NATIVE_LEASH_MARGIN_100");continue;}
  if(r.order.active||r.combat.active||r.ticks<r.retry_after){++out.busy;continue;}
  r.order={};r.order.active=true;r.order.caller=s.f.id;r.order.caller_gen=s.f.gen;r.order.offered_tick=r.ticks;r.order.goal=s.f.pos;
  ++out.offered;emit("RECRUIT_OFFERED",s.f.id,s.f.gen,r.f,"SNAPSHOT_ONLY_ADMISSION_DEFERRED_TO_RECEIVER_EXECUTE");
 }
 if(out.offered&&!c.waiting&&s.ticks>=c.backoff_until&&!group(t,s,seq,window)){c.waiting=true;c.wait_start=s.ticks;emit("BLUE_WAITING_FOR_ALLY",s.f.id,s.f.gen,s.f,"BOUNDED_REAL_ORDERS_NOT_A_BONUS");}
 emit("BLUE_CALL_ISSUED",s.f.id,s.f.gen,s.f,out.offered?"ORDERS_OFFERED_ALL_ELIGIBLE_LEDGER_ENTRIES":"NO_NEW_CALLABLE_ALLIES_NATIVE_COMBAT");return out;
}
// Receiver admission/refresh must be called only in that actor's execute. verify(caller) does the authoritative ID/address/generation check.
template<class Verify,class Emit>inline bool order_valid(Table&t,Entry&r,std::uint32_t seq,unsigned window,Verify&& verify,Emit&& emit)noexcept{
 if(!r.order.active)return false;
 auto* s=t.find(r.order.caller,r.order.caller_gen);
 if(!s||!usable(r.f)||!usable(s->f)||!fresh(s->f,seq,window)||!context(s->f,r.f)||((!r.order.consumed||r.ticks-r.order.refreshed>=kRefresh)&&!verify(s->f))){abort(r,emit,"CALLER_GONE_STALE_GENERATION_OR_CONTEXT");return false;}
 if(!leash(r.f,s->f.pos)){abort(r,emit,"NATIVE_LEASH_REJECTED_NO_RANGE_OR_HOME_WRITE");return false;}
 if(r.f.hit||(!r.order.consumed&&!idle(r.f.mode))||(r.order.consumed&&r.f.mode!=Sound&&!fighting(r.f.mode))){abort(r,emit,"NATIVE_DAMAGE_SPECIAL_OR_OTHER_CONTROL_PRIORITY");return false;}
 if(r.order.consumed&&r.ticks-r.order.refreshed<kRefresh)return true;
 r.order.goal=s->f.pos;r.order.refreshed=r.ticks;return true;
}
template<class Emit>inline void consumed(Entry&r,Emit&& emit)noexcept{
 auto& o=r.order;o.consumed=true;o.started=r.ticks;o.progress_tick=r.ticks;o.checked=r.ticks;o.start=r.f.pos;o.best=devf_dyn::sqrt_d(xz2(r.f.pos,o.goal));
 emit("RECRUIT_ORDER_CONSUMED",o.caller,o.caller_gen,r.f,"OWN_EXECUTE_NATIVE_SOUND_WATCH_0_READBACK");
}
template<class Emit>inline void after(Table&t,Entry&r,Emit&& emit)noexcept{
 auto& o=r.order;if(!o.active||!o.consumed)return;
 if(!usable(r.f)||r.f.hit){abort(r,emit,"NATIVE_DAMAGE_DEATH_OR_SPECIAL_PRIORITY");return;}
 if(fighting(r.f.mode)&&!o.engaged){o.engaged=true;emit("RECRUIT_ENGAGED_NATIVE_MODE",o.caller,o.caller_gen,r.f,"NATIVE_FIND_OR_ATTACK_MODE_NOT_A_MEASURED_LINK_HIT_WAIT_REQUIRES_ACTUAL_GROUP_OR_PROGRESS");}
 if(r.f.mode!=Sound&&!fighting(r.f.mode)){abort(r,emit,"NATIVE_APPROACH_ENDED_OR_OVERRIDDEN");return;}
 auto* s=t.find(o.caller,o.caller_gen);
 if(s&&radius(r.f,s->f,650.)&&!o.arrived){o.arrived=true;emit("RECRUIT_ARRIVED",o.caller,o.caller_gen,r.f,"ACTUAL_CURRENT_POSITIONS_WITHIN_650_XZ_250_Y_NOT_DISPATCH");}
 if((r.f.mode==Sound&&r.f.state==2)||fighting(r.f.mode)){
  if(!o.moving_since){o.moving_since=r.ticks;o.progress_tick=r.ticks;o.checked=r.ticks;}
  if(!o.moved&&xz2(r.f.pos,o.start)>=25.&&devf_dyn::sqrt_d(xz2(r.f.pos,o.goal))+5.<=o.best){o.moved=true;emit("RECRUIT_APPROACH_STARTED",o.caller,o.caller_gen,r.f,"POSITION_CHANGED_AND_PROGRESS_AT_LEAST_5_TOWARD_NATIVE_TARGET");}
  if(r.ticks-o.checked>=30){double d=devf_dyn::sqrt_d(xz2(r.f.pos,o.goal));if(d+50.<=o.best){o.best=d;o.progress_tick=r.ticks;}o.checked=r.ticks;}
  if(!o.arrived&&r.ticks-o.progress_tick>=kNoProgress){abort(r,emit,"60_MOVING_TICKS_WITHOUT_50_UNITS_PROGRESS");return;}
 }
 unsigned age=r.ticks-o.started;
 if(age>=kBudget&&!o.extended&&o.moving_since&&r.ticks-o.progress_tick<=30){o.extended=true;emit("RECRUIT_PROGRESS_EXTENSION",o.caller,o.caller_gen,r.f,"MEASURABLE_PROGRESS_ABSOLUTE_CAP_240");}
 if(age>=(o.extended?kMaxBudget:kBudget))abort(r,emit,"APPROACH_DEADLINE_NATIVE_FALLBACK");
}
template<class Emit>inline bool waiting(Table&t,Entry&s,std::uint32_t seq,unsigned window,Emit&&emit)noexcept{
 auto& c=s.combat;if(!c.waiting)return false;
 if(group(t,s,seq,window)){c.waiting=false;emit("BLUE_GROUP_READY",s.f.id,s.f.gen,s.f,"ACTUAL_650_250_CONTEXT_FILTER_BONUS_UNCHANGED");return false;}
 bool pending=false;
 for(auto& r:t.entries){const auto&o=r.order;if(!o.active||o.caller!=s.f.id||o.caller_gen!=s.f.gen||!usable(r.f)||!fresh(r.f,seq,window)||!context(r.f,s.f)||!leash(r.f,s.f.pos))continue;
  if(o.consumed||(r.ticks-o.offered_tick<=kFreshGrace))pending=true;
 }
 if(!pending||s.ticks-c.wait_start>=kMaxBudget){c.waiting=false;c.backoff_until=s.ticks+kRecall;emit("BLUE_FALLBACK_NATIVE_COMBAT",s.f.id,s.f.gen,s.f,pending?"ABSOLUTE_WAIT_CAP_240":"NO_REMAINING_CALLABLE_PROGRESSING_ALLY");return false;}
 return true;
}
// Gesture state is separate from dispatch. Request is NOT proof the native animation ran.
inline void shout_requested(Entry& r)noexcept{auto& c=r.combat;c.shout_pending=false;c.shout_live=true;c.shout_start=r.ticks;}
inline bool shout_observed(Entry& r,int mode,int state,bool clip7)noexcept{
 auto& c=r.combat;if(c.shout_live&&!c.shout_used&&mode==Watch&&state==4&&clip7){c.shout_used=true;return true;}return false;
}
// 1 = preempted before animation: retry at a future safe PRE. 2 = a completed/overridden native gesture. Neither restores modes.
inline unsigned shout_left_watch(Entry& r,int mode)noexcept{
 auto& c=r.combat;if(!c.shout_live||mode==Watch)return 0;c.shout_live=false;
 if(!c.shout_used){c.shout_pending=true;return 1;}return 2;
}
// Closed native scratch lease: PRE writes 2 only if needed; execute decrements to1; POST restores the expected decremented original.
struct TimerLease{bool wrote=false;std::int16_t before=0;};
inline TimerLease gate(bool wait,int mode,int state,std::int16_t& timer)noexcept{
 TimerLease l;if(wait&&mode==Find&&(state==3||state==4)&&timer>=0&&timer<2){l={true,timer};timer=2;}return l;
}
inline void restore(TimerLease l,int mode,int state,std::int16_t& timer)noexcept{
 if(l.wrote&&mode==Find&&(state==3||state==4)&&timer==1)timer=static_cast<std::int16_t>(l.before>0?l.before-1:0);
}
}
