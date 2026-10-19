#pragma once
// CP19 (0.16.0): a called receiver is no longer refused or aborted for its OWN native leash (the CP19 permission then replaces its leash returns);
// radius, height, stage, room, layer, identity, caller leash and every other CP16/FIX01 rule are unchanged.
// CP16 real policy shared by DLL and laboratory. Orders contain IDs/generations/values, never actor pointers.
#include "blue_ai_policy.hpp"
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_recruit {
using Variant=devf_variant::Variant;
constexpr unsigned kCapacity=64,kRecall=150,kCalm=150,kBudget=180,kMaxBudget=240,kNoProgress=60,kBackoff=90,kRefresh=15,kFreshGrace=2,kCallerGrace=10;
constexpr double kRadius=2800.,kHeight=250.,kMargin=100.;
constexpr int Wait=devf_variant::A_WAIT,Walk=devf_variant::A_WALK,Find=devf_variant::A_FIND,Attack=devf_variant::A_ATTACK,Watch=devf_variant::A_WATCH,Sound=devf_variant::A_SOUND_WATCH,FindStay=devf_variant::A_FIND_STAY;
struct Point{float x=0,y=0,z=0;};
inline bool finite(float x) noexcept{return x==x&&x>=-10000000.f&&x<=10000000.f;}
inline bool finite(Point p)noexcept{return finite(p.x)&&finite(p.y)&&finite(p.z);}
inline double xz2(Point a,Point b)noexcept{double x=double(a.x)-b.x,z=double(a.z)-b.z;return x*x+z*z;}
inline double abs_d(double x)noexcept{return x<0?-x:x;}
struct Frame{
 std::uint32_t id=0,gen=0,seq=0;Variant variant=Variant::None;char stage[9]{};int room=-1,layer=-1,mode=-1,state=-1,health=0;
 Point pos{},home{};float range=0;bool ready=false,hit=false,leash_out=false;std::uint16_t unsafe_flags=0;
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
 bool active=false,consumed=false,moved=false,arrived=false,extended=false,engaged=false,caller_transient=false,identity_checked=false,identity_verified=false;
 std::uint32_t caller=0,caller_gen=0;unsigned offered_tick=0,started=0,moving_since=0,progress_tick=0,checked=0,refreshed=0,caller_transient_start=0,verified_tick=0;
 Point start{},goal{},progress_pos{};
};
struct Combat{bool active=false,shout_pending=false,shout_used=false,shout_live=false,waiting=false;unsigned last_threat=0,next_call=0,wait_start=0,backoff_until=0,shout_start=0;};
struct Entry{Frame f{};unsigned ticks=0,retry_after=0,detail_cursor=0;Order order{};Combat combat{};};
struct Table{
 std::array<Entry,kCapacity> entries{};
 Entry* find(std::uint32_t id,std::uint32_t gen)noexcept{for(auto& e:entries)if(e.f.id==id&&e.f.gen==gen)return &e;return nullptr;}
 Entry* find_id(std::uint32_t id)noexcept{for(auto& e:entries)if(e.f.id==id)return &e;return nullptr;}
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
// Primary diagnostic order is deliberate: identity/domain -> classification/life -> freshness/context -> range -> native mode/safety -> leash -> ownership/backoff.
enum class Skip:std::uint8_t{Offered,InvalidSnapshot,NotClassified,DeadOrTerminal,Stale,DifferentStage,DifferentRoom,DifferentLayer,OutOfRadius,HeightLimit,AlreadyFighting,NotIdle,Unsafe,NativeLeash,Busy,Backoff,Count};
constexpr unsigned kReasons=static_cast<unsigned>(Skip::Count);
inline const char* skip_text(Skip r)noexcept{
 constexpr const char* names[]={"OFFERED","INVALID_SNAPSHOT_CONTEXT","NOT_CLASSIFIED","DEAD_OR_TERMINAL_SPECIAL","STALE_OR_UNLOADED_SNAPSHOT","DIFFERENT_STAGE","DIFFERENT_ROOM","DIFFERENT_LAYER","OUT_OF_RADIUS","HEIGHT_LIMIT","ALREADY_FIGHTING","NOT_IDLE","RECEIVER_HIT_OR_UNSAFE","NATIVE_LEASH","BUSY_OR_EXISTING_ORDER","RETRY_BACKOFF"};
 return static_cast<unsigned>(r)<kReasons?names[static_cast<unsigned>(r)]:"UNKNOWN";
}
enum Unsafe:std::uint16_t{BadCanary=1,Hidden=2,Deleting=4,Event=8,Transition=16,NoGround=32};
inline bool domain(const Frame& f)noexcept{return f.id&&f.gen&&f.room>=0&&f.room<64&&f.layer>=0&&f.layer<15&&f.stage[0]&&finite(f.pos)&&finite(f.home)&&finite(f.range)&&f.range>=0;}
inline Skip candidate(const Entry&s,const Entry&r,std::uint32_t seq,unsigned window)noexcept{
 const auto& f=r.f;
 if(!domain(f))return Skip::InvalidSnapshot;
 if(f.variant==Variant::None)return Skip::NotClassified;
 if(f.health<=0||devf_variant::blocks_regen(f.mode))return Skip::DeadOrTerminal;
 if(!fresh(f,seq,window))return Skip::Stale;
 if(std::memcmp(s.f.stage,f.stage,9)!=0)return Skip::DifferentStage;
 if(s.f.room!=f.room)return Skip::DifferentRoom;
 if(s.f.layer!=f.layer)return Skip::DifferentLayer;
 if(xz2(s.f.pos,f.pos)>kRadius*kRadius)return Skip::OutOfRadius;
 if(abs_d(double(s.f.pos.y)-f.pos.y)>kHeight)return Skip::HeightLimit;
 if(fighting(f.mode))return Skip::AlreadyFighting;
 if(!idle(f.mode))return Skip::NotIdle;
 if(f.hit||!f.ready)return Skip::Unsafe;
 if(r.order.active||r.combat.active)return Skip::Busy;
 if(r.ticks<r.retry_after)return Skip::Backoff;
 return Skip::Offered;
}
struct Call {
 bool issued=false;unsigned offered=0,leash_skipped=0,busy=0,examined_tracked=0,empty_slots=0,self_slots=0,detail_count=0,distance_samples=0;
 std::array<unsigned,kReasons> skipped{};double min_distance=-1,max_distance=-1;const char* origin="NONE";
};
inline bool call_safe(const Frame& f)noexcept{
 return usable(f)&&!f.hit&&!f.leash_out&&(idle(f.mode)||fighting(f.mode)||f.mode==Watch)&&!(f.mode==Attack&&f.state>=3);
}
template<class Emit>inline Call call(Table&t,Entry& s,bool detected,std::uint32_t seq,unsigned window,Emit&& emit,bool link_available=true)noexcept{
 Call out;auto& c=s.combat;
 if(!usable(s.f)||s.f.variant!=Variant::Blue)return out;
 const bool incoming=s.order.active,confirmed_fight=link_available&&fighting(s.f.mode);
 if(detected||confirmed_fight||incoming)c.last_threat=s.ticks;
 if(c.active&&!detected&&!confirmed_fight&&!incoming&&s.ticks-c.last_threat>=kCalm){c={};emit("BLUE_COMBAT_RESET",s.f.id,s.f.gen,s.f,"150_CALM_TICKS_NO_THREAT_OR_ORDER");}
 if(incoming||!link_available||!call_safe(s.f))return out;
 bool initial=false;
 if(!c.active){
  if(!detected)return out;initial=true;c.active=true;c.shout_pending=true;c.next_call=s.ticks;c.last_threat=s.ticks;
  emit("BLUE_LINK_DETECTED",s.f.id,s.f.gen,s.f,"OWN_NATIVE_SEARCH_PLAYER_NOT_COMPANION_CASCADE");
 }
 // Active combat never invents LOS or starts another combat/cry; first activation remains direct native detection only.
 if(!detected&&!confirmed_fight)return out;
 if(s.ticks<c.next_call)return out;c.next_call=s.ticks+kRecall;out.issued=true;
 out.origin=initial?"INITIAL_DIRECT_DETECTION":detected?"RECALL_CURRENT_DIRECT_DETECTION":"RECALL_ACTIVE_COMBAT_WITHOUT_CURRENT_LOS";
 for(unsigned n=0;n<kCapacity;++n){
  auto& r=t.entries[(s.detail_cursor+n)%kCapacity];
  if(!r.f.id){++out.empty_slots;continue;}if(r.f.id==s.f.id){++out.self_slots;continue;}++out.examined_tracked;
  const auto reason=candidate(s,r,seq,window);++out.skipped[static_cast<unsigned>(reason)];
  if(domain(r.f)&&fresh(r.f,seq,window)&&context(s.f,r.f)){
   const double d=devf_dyn::sqrt_d(xz2(s.f.pos,r.f.pos));if(!out.distance_samples||d<out.min_distance)out.min_distance=d;if(!out.distance_samples||d>out.max_distance)out.max_distance=d;++out.distance_samples;
  }
  if(reason!=Skip::Offered){
   if(reason==Skip::NativeLeash){++out.leash_skipped;emit("RECRUIT_SKIPPED_NATIVE_LEASH",s.f.id,s.f.gen,r.f,"DESTINATION_OUTSIDE_OWN_NATIVE_LEASH_MARGIN_100");}
   if(reason==Skip::Busy)++out.busy;
   if(out.detail_count<2){++out.detail_count;emit("RECRUIT_CANDIDATE_SKIPPED",s.f.id,s.f.gen,r.f,skip_text(reason));}
   continue;
  }
  r.order={};r.order.active=true;r.order.caller=s.f.id;r.order.caller_gen=s.f.gen;r.order.offered_tick=r.ticks;r.order.goal=s.f.pos;
  ++out.offered;emit("RECRUIT_OFFERED",s.f.id,s.f.gen,r.f,"SNAPSHOT_ONLY_ADMISSION_DEFERRED_TO_RECEIVER_EXECUTE");
 }
 s.detail_cursor=(s.detail_cursor+7)%kCapacity; // diagnostic fairness only; never moves/reassigns an existing order
 if(out.offered&&!c.waiting&&s.ticks>=c.backoff_until&&!group(t,s,seq,window)){c.waiting=true;c.wait_start=s.ticks;emit("BLUE_WAITING_FOR_ALLY",s.f.id,s.f.gen,s.f,"BOUNDED_REAL_ORDERS_NOT_A_BONUS");}
 emit("BLUE_CALL_ISSUED",s.f.id,s.f.gen,s.f,out.origin);return out;
}
inline const char* unsafe_reason(const Frame& f,bool caller)noexcept{
 if(f.unsafe_flags&BadCanary)return caller?"CALLER_NATIVE_CANARY_UNSAFE":"RECEIVER_NATIVE_CANARY_UNSAFE";
 if(f.unsafe_flags&Hidden)return caller?"CALLER_HIDDEN":"RECEIVER_HIDDEN";
 if(f.unsafe_flags&Deleting)return caller?"CALLER_DELETING":"RECEIVER_DELETING";
 if(f.unsafe_flags&Event)return caller?"CALLER_EVENT_ACTIVE":"RECEIVER_EVENT_ACTIVE";
 if(f.unsafe_flags&Transition)return caller?"CALLER_STAGE_TRANSITION":"RECEIVER_STAGE_TRANSITION";
 if(f.unsafe_flags&NoGround)return caller?"CALLER_GROUND_NOT_READY":"RECEIVER_GROUND_NOT_READY";
 return caller?"CALLER_UNSAFE_SNAPSHOT_UNKNOWN":"RECEIVER_UNSAFE_SNAPSHOT_UNKNOWN";
}
// Damage0..3/5 is the ordinary hit/recovery path (native1534-1587); gale4/6, BIG_DAMAGE, death/fall/demo are not granted grace.
inline bool transient_caller(const Frame& f)noexcept{
 const bool damage=f.mode==devf_variant::A_DAMAGE&&((f.state>=0&&f.state<=3)||f.state==5);
 return damage||(f.hit&&(idle(f.mode)||fighting(f.mode)||f.mode==Watch));
}
// Admission checks are ordered and all hard guards precede the narrow already-consumed caller-hit grace.
template<class Verify,class Emit>inline bool order_valid(Table&t,Entry&r,std::uint32_t seq,unsigned window,Verify&& verify,Emit&& emit)noexcept{
 auto& o=r.order;if(!o.active)return false;
 auto reject=[&](const char* why){abort(r,emit,why);return false;};
 auto* s=t.find(o.caller,o.caller_gen);
 if(!s){const auto* other=t.find_id(o.caller);return reject(other?"CALLER_GENERATION_MISMATCH":"CALLER_NOT_TRACKED_OR_RELEASED");}
 if(!domain(r.f))return reject("RECEIVER_INVALID_SNAPSHOT_CONTEXT");
 if(r.f.variant==Variant::None)return reject("RECEIVER_NOT_CLASSIFIED");
 if(r.f.health<=0||devf_variant::blocks_regen(r.f.mode))return reject("RECEIVER_DEAD_OR_TERMINAL_SPECIAL");
 if(!fresh(r.f,seq,window))return reject("RECEIVER_SNAPSHOT_STALE");
 if(!r.f.ready)return reject(unsafe_reason(r.f,false));
 if(r.f.hit)return reject("RECEIVER_HIT_PRIORITY");
 if(!domain(s->f))return reject("CALLER_INVALID_SNAPSHOT_CONTEXT");
 if(s->f.variant==Variant::None)return reject("CALLER_NOT_CLASSIFIED");
 if(s->f.variant!=Variant::Blue)return reject("CALLER_CLASSIFICATION_CHANGED");
 if(s->f.health<=0||devf_variant::blocks_regen(s->f.mode))return reject("CALLER_DEAD_OR_TERMINAL_SPECIAL");
 if(!fresh(s->f,seq,window))return reject("CALLER_SNAPSHOT_STALE");
 if(std::memcmp(s->f.stage,r.f.stage,9)!=0)return reject("CALLER_STAGE_CHANGED");
 if(s->f.room!=r.f.room)return reject("CALLER_ROOM_CHANGED");
 if(s->f.layer!=r.f.layer)return reject("CALLER_LAYER_CHANGED");
 if(!s->f.ready)return reject(unsafe_reason(s->f,true));
 if(s->f.leash_out)return reject("CALLER_NATIVE_LEASH_RETURN");
 if((!o.consumed&&!idle(r.f.mode))||(o.consumed&&r.f.mode!=Sound&&!fighting(r.f.mode)))return reject("RECEIVER_NATIVE_MODE_PRIORITY");
 const bool transient=transient_caller(s->f);
 if(!o.identity_checked||r.ticks-o.verified_tick>=kRefresh||(transient&&!o.caller_transient)){
  o.identity_checked=true;o.identity_verified=verify(s->f);o.verified_tick=r.ticks;
  if(!o.identity_verified)return reject("CALLER_VERIFIED_IDENTITY_FAILED");
 }
 if(transient){
  if(!o.consumed)return reject("CALLER_TRANSIENT_BEFORE_CONSUMPTION");
  if(!o.caller_transient){o.caller_transient=true;o.caller_transient_start=r.ticks;emit("RECRUIT_CALLER_TRANSIENT_GRACE_STARTED",o.caller,o.caller_gen,r.f,"CONSUMED_ONLY_NATIVE_NORMAL_HIT_DAMAGE_GRACE10_NO_COMMAND_WRITES");}
  if(r.ticks-o.caller_transient_start>=kCallerGrace)return reject("CALLER_TRANSIENT_GRACE_EXPIRED");
  return false; // keep the value-only order; adapter performs no target/timer/state writes while held
 }
 if(!(idle(s->f.mode)||fighting(s->f.mode)||s->f.mode==Watch))return reject("CALLER_NATIVE_MODE_UNSUPPORTED");
 if(o.caller_transient){o.caller_transient=false;emit("RECRUIT_CALLER_TRANSIENT_RESUMED",o.caller,o.caller_gen,r.f,"RECOVERED_WITHOUT_RESETTING_START_PROGRESS_OR_240_CAP");}
 if(o.consumed&&r.ticks-o.refreshed<kRefresh)return true;
 o.goal=s->f.pos;o.refreshed=r.ticks;return true;
}
template<class Emit>inline Entry& publish_checked(Table& t,unsigned slot,const Frame& f,Emit&& emit)noexcept{
 auto& old=t.entries[slot<kCapacity?slot:0];
 if(old.order.active){
  const char* why=old.f.id!=f.id?"RECEIVER_SLOT_ID_CHANGED":old.f.gen!=f.gen?"RECEIVER_GENERATION_CHANGED":std::memcmp(old.f.stage,f.stage,9)!=0?"RECEIVER_STAGE_CHANGED":old.f.room!=f.room?"RECEIVER_ROOM_CHANGED":old.f.layer!=f.layer?"RECEIVER_LAYER_CHANGED":nullptr;
  if(why)abort(old,emit,why);
 }
 return t.publish(slot,f);
}
// Shared diagnostic budget: per30 management passes at most16 call events,16 summaries,8 details. Separate from critical lifecycle budget1200.
enum class Diag:std::uint8_t{Call,Summary,Detail};
struct Diagnostics{
 unsigned ticks=0,calls=0,summaries=0,details=0,all_calls=0,all_summaries=0,all_details=0,suppressed=0;
 void advance()noexcept{if(++ticks%30==0)calls=summaries=details=0;}
 bool take(Diag k)noexcept{
  unsigned* n=k==Diag::Call?&calls:k==Diag::Summary?&summaries:&details;unsigned* all=k==Diag::Call?&all_calls:k==Diag::Summary?&all_summaries:&all_details;
  const unsigned window=k==Diag::Detail?8:16,total=k==Diag::Detail?240:600;
  if(*n>=window||*all>=total){++suppressed;return false;}++*n;++*all;return true;
 }
};
template<class Emit>inline void consumed(Entry&r,Emit&& emit)noexcept{
 auto& o=r.order;o.consumed=true;o.started=r.ticks;o.progress_tick=r.ticks;o.checked=r.ticks;o.start=r.f.pos;o.progress_pos=r.f.pos;
 emit("RECRUIT_ORDER_CONSUMED",o.caller,o.caller_gen,r.f,"OWN_EXECUTE_NATIVE_SOUND_WATCH_0_READBACK");
}
template<class Emit>inline void after(Table&t,Entry&r,Emit&& emit)noexcept{
 auto& o=r.order;if(!o.active||!o.consumed)return;
 if(!usable(r.f)||r.f.hit){const char* why=!domain(r.f)?"RECEIVER_INVALID_SNAPSHOT_CONTEXT":r.f.health<=0||devf_variant::blocks_regen(r.f.mode)?"RECEIVER_DEAD_OR_TERMINAL_SPECIAL":!r.f.ready?unsafe_reason(r.f,false):r.f.hit?"RECEIVER_HIT_PRIORITY":"RECEIVER_NOT_CLASSIFIED";abort(r,emit,why);return;}
 if(fighting(r.f.mode)&&!o.engaged){o.engaged=true;emit("RECRUIT_ENGAGED_NATIVE_MODE",o.caller,o.caller_gen,r.f,"NATIVE_FIND_OR_ATTACK_MODE_NOT_A_MEASURED_LINK_HIT_WAIT_REQUIRES_ACTUAL_GROUP_OR_PROGRESS");}
 if(r.f.mode!=Sound&&!fighting(r.f.mode)){abort(r,emit,"NATIVE_APPROACH_ENDED_OR_OVERRIDDEN");return;}
 auto* s=t.find(o.caller,o.caller_gen);
 if(s&&radius(r.f,s->f,650.)&&!o.arrived){o.arrived=true;emit(o.moved?"RECRUIT_ARRIVED":"RECRUIT_GROUP_PROXIMITY_WITHOUT_RECEIVER_APPROACH",o.caller,o.caller_gen,r.f,"ACTUAL_CURRENT_POSITIONS_WITHIN_650_XZ_250_Y_NOT_DISPATCH");}
 if((r.f.mode==Sound&&r.f.state==2)||fighting(r.f.mode)){
  if(!o.moving_since){o.moving_since=r.ticks;o.progress_tick=r.ticks;o.checked=r.ticks;}
  if(!o.moved&&xz2(r.f.pos,o.start)>=25.&&devf_dyn::sqrt_d(xz2(r.f.pos,o.goal))+5.<=devf_dyn::sqrt_d(xz2(o.start,o.goal))){o.moved=true;emit("RECRUIT_APPROACH_STARTED",o.caller,o.caller_gen,r.f,"POSITION_CHANGED_AND_PROGRESS_AT_LEAST_5_TOWARD_NATIVE_TARGET");}
  if(r.ticks-o.checked>=30){double d=devf_dyn::sqrt_d(xz2(r.f.pos,o.goal));if(d+50.<=devf_dyn::sqrt_d(xz2(o.progress_pos,o.goal))){o.progress_pos=r.f.pos;o.progress_tick=r.ticks;}o.checked=r.ticks;}
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
 for(auto& r:t.entries){const auto&o=r.order;if(!o.active||o.caller!=s.f.id||o.caller_gen!=s.f.gen||!usable(r.f)||!fresh(r.f,seq,window)||!context(r.f,s.f))continue;
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
