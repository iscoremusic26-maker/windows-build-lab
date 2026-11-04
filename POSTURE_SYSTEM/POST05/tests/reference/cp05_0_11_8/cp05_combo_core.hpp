#pragma once
// CP05 extends the value-only CP04 primitives, leaving its Blue scheduler intact.
#include "cp04_combo_core.hpp"
namespace cp05 {
inline constexpr unsigned kSlots=64,kMaxSwings=8,kWindupMax=32,kBufferToAt=10,kMinimumGap=2;
inline constexpr float kRedRate=1.5f,kBlackRate=1.45f;
enum class Variant:std::uint8_t{Red,Black};
// Existing POST08 refuses a Black rate>=1.5 as possible chip/fury. Preserve
// that detector exactly: CP05 NORMAL Black stays strictly below its bound.
inline float rate(Variant v)noexcept{return v==Variant::Red?kRedRate:kBlackRate;}
enum class Kind:std::uint8_t{Normal,Red4,Black8,Black7};
enum class Phase:std::uint8_t{Idle,Selected,Swing,Gap,Recovery};
enum class Cancel:std::uint8_t{Context,Posture100,Priority,Fury,Special,Foreign,Unsafe,Windup,Reentry,Pause,Readback,NoWindow,Shutdown,Count};
inline constexpr unsigned count(Kind k)noexcept{return k==Kind::Red4?4:k==Kind::Black8?8:k==Kind::Black7?7:0;}
inline const char* name(Kind k)noexcept{return k==Kind::Red4?"RED4":k==Kind::Black8?"BLACK8":k==Kind::Black7?"BLACK7":"NORMAL";}
inline const char* name(Cancel r)noexcept{
 const char* names[]={"ACTOR_CONTEXT","POSTURE_100","NATIVE_PRIORITY","BLACK_FURY","RED_SPECIAL","FOREIGN_OWNER","UNSAFE_ANIMATION","WINDUP_TIMEOUT","REENTRY","PAUSE_OR_EVENT","READBACK","NO_NATIVE_WINDOW","SHUTDOWN"};
 return names[unsigned(r)<unsigned(Cancel::Count)?unsigned(r):0];
}
// Audio intervals in centiseconds * 30Hz * uniform scale5, rounded to nearest
// tick. Rates and post-clip waits are distinct. These are AT BEGIN targets,
// never claimed contact times or authored clip lengths.
inline unsigned target(Kind k,unsigned boundary)noexcept{
 constexpr unsigned r[]={180,36,198},b8[]={134,147,39,38,36,39,131},b7[]={38,156,159,72,74,72};
 if(k==Kind::Red4&&boundary<3)return r[boundary];
 if(k==Kind::Black8&&boundary<7)return b8[boundary];
 if(k==Kind::Black7&&boundary<6)return b7[boundary];return 0;
}
struct Range {
 std::uint64_t n=0,min=0,max=0,sum=0;
 void add(std::uint64_t x)noexcept{if(!n||x<min)min=x;if(!n||x>max)max=x;++n;sum+=x;}
};
struct Distribution{std::uint64_t opportunities=0,normal=0,combo1=0,combo2=0,draws=0,rejections=0,excluded=0;};
struct Stats{
 std::uint64_t selected=0,not_started=0,completed=0,cancelled=0,breaks=0,rates=0,foreign=0,returned=0,extended=0;
 std::uint64_t starts[8]{},armed[8]{},contacts[8]{},other[8]{},parries[8]{},guard[8]{},unblocked[8]{},miss[8]{};
 std::uint64_t cancel[unsigned(Cancel::Count)]{};
 Range at[7]{},contact[7]{},gap[7]{};
};
struct Counters{Distribution distribution[2]{};Stats kind[4]{};std::uint64_t duplicate=0,reentered=0,overflow=0,untracked=0,fury=0,special=0,errors=0;};
// 2^64 has remainder1 on division by3: reject UINT64_MAX. Remaining words
// contain exactly (2^64-1)/3 instances of each residue. No 33/33/34 shortcut.
inline bool reduce3(std::uint64_t word,unsigned& out)noexcept{
 if(word==~std::uint64_t(0))return false;out=unsigned(word%3);return true;
}
struct Combo {
 cp04::Key key{};bool bound=false;Variant variant=Variant::Red;Kind kind=Kind::Normal;Phase phase=Phase::Idle;
 std::uint64_t context_serial=0,seed=0,rng=0,opportunity=0,combo_id=0,token=0,local_tick=0,last_execute=0,last_decision=0;
 std::uint64_t raw_tick=0,start_tick=0,finish_tick=0,previous_at=0,previous_contact=0,launch_at=0,launch_tick=0;
 unsigned previous_contact_swing=0,swing=0,last_draws=0,last_rejections=0,origin=0;int bck=0;float end=0,last_frame=0,attack_frame=0,rate_value=0;
 bool attack_entered=false,start_pending=false,will_finish=false,launch_requested=false,rate_owned=false,at_seen=false,at_live=false,contact_seen=false,other_seen=false;
 bool native_fresh=false,lease=false,processed=false,fury=false,valid=false;int native_timer=0,fresh_bck=0;float raw_rate=0,fresh_end=0;
 bool active()const noexcept{return phase!=Phase::Idle;}
 void bind(const cp04::Key& k,Variant v)noexcept{
  const auto context=context_serial+1?context_serial+1:1;
  *this=Combo{};bound=true;key=k;variant=v;context_serial=context;
  seed=cp04::mix((std::uint64_t(k.id)<<32)^k.generation^(std::uint64_t(k.stage)<<1)^k.epoch^
   (std::uint64_t(unsigned(k.room))<<16)^unsigned(k.layer)^cp04::mix(context_serial)^0xc505a17e674932bdULL);rng=seed;
 }
 bool execute(std::uint64_t stamp,Counters& c)noexcept{
  if(last_execute==stamp){++c.duplicate;return false;}last_execute=stamp;++local_tick;
  attack_entered=start_pending=will_finish=native_fresh=processed=false;return true;
 }
 Kind decide(std::uint64_t stamp,bool normal_proven,Counters& c)noexcept{
  if(!normal_proven||active()||last_decision==stamp){if(last_decision==stamp)++c.duplicate;return Kind::Normal;}
  last_decision=stamp;++opportunity;auto& d=c.distribution[unsigned(variant)];++d.opportunities;last_draws=last_rejections=0;
  auto word=[&](){++last_draws;++d.draws;rng+=0x9e3779b97f4a7c15ULL;return cp04::mix(rng);};
  if(variant==Variant::Red)kind=(word()>>63)?Kind::Red4:Kind::Normal;
  else{
   unsigned result=0;
   if(!reduce3(word(),result)){++last_rejections;++d.rejections;
    // A bijective mixer on a full-period counter cannot repeat the sole
    // rejected word on consecutive draws. Exactly two calls bound the loop.
    if(!reduce3(word(),result)){++c.errors;return Kind::Normal;}
   }
   kind=result==0?Kind::Normal:result==1?Kind::Black8:Kind::Black7;
  }
  if(kind==Kind::Normal){++d.normal;combo_id=0;swing=0;}else{
   if(kind==Kind::Black7)++d.combo2;else ++d.combo1;
   phase=Phase::Selected;combo_id=opportunity;swing=0;previous_at=previous_contact=0;previous_contact_swing=0;++c.kind[unsigned(kind)].selected;
  }return kind;
 }
 bool begin(int clip,float actual_end,float frame,float handoff,Counters& c)noexcept{
  if((phase!=Phase::Selected&&phase!=Phase::Gap)||swing>=count(kind)||(clip!=5&&clip!=6)||
     !cp04::finite(actual_end)||actual_end<=25||actual_end>300||!cp04::near(frame,1)||handoff!=1.15f)return false;
  auto& stats=c.kind[unsigned(kind)];if(swing)stats.gap[swing-1].add(local_tick-finish_tick);
  ++swing;++token;++stats.starts[swing-1];phase=Phase::Swing;bck=clip;end=actual_end;rate_value=rate(variant);last_frame=frame;start_tick=local_tick;
  launch_requested=start_pending=at_seen=at_live=contact_seen=other_seen=rate_owned=false;return true;
 }
 void arm(Counters& c)noexcept{
  if(phase!=Phase::Swing||at_seen||!swing)return;auto& stats=c.kind[unsigned(kind)];
  at_seen=true;++stats.armed[swing-1];if(swing>=2&&previous_at)stats.at[swing-2].add(local_tick-previous_at);previous_at=local_tick;
 }
 void finish(Counters& c)noexcept{
  if(phase!=Phase::Swing||!swing)return;auto& stats=c.kind[unsigned(kind)];
  if(!contact_seen&&!other_seen)++stats.miss[swing-1];rate_owned=false;finish_tick=local_tick;
  if(swing==count(kind)){++stats.completed;++stats.returned;phase=Phase::Recovery;return;}
  phase=Phase::Gap;launch_requested=false;
  // Native full clip/recoil is authoritative. A target that no longer fits
  // gets a logged safety extension, never a truncation or phantom window.
  const auto deadline=previous_at+target(kind,swing-1);
  launch_at=deadline>=kBufferToAt?deadline-kBufferToAt:0;
  if(launch_at<local_tick+kMinimumGap){launch_at=local_tick+kMinimumGap;++stats.extended;}
 }
 void contact(bool link,bool parry,bool blocked,Counters& c)noexcept{
  if(phase!=Phase::Swing||!swing||!at_seen)return;auto& stats=c.kind[unsigned(kind)];
  if(!link){if(!other_seen){other_seen=true;++stats.other[swing-1];}return;}
  if(contact_seen)return;contact_seen=true;++stats.contacts[swing-1];
  if(parry)++stats.parries[swing-1];else if(blocked)++stats.guard[swing-1];else ++stats.unblocked[swing-1];
  if(swing>=2&&previous_contact&&previous_contact_swing+1==swing)stats.contact[swing-2].add(local_tick-previous_contact);previous_contact=local_tick;previous_contact_swing=swing;
 }
 void cancel(Cancel why,Counters& c)noexcept{
  if(!active())return;if(phase==Phase::Recovery){phase=Phase::Idle;rate_owned=false;return;}auto& stats=c.kind[unsigned(kind)];
  if(phase==Phase::Selected)++stats.not_started;++stats.cancelled;++stats.cancel[unsigned(why)];
  if(why==Cancel::Posture100)++stats.breaks;
  phase=Phase::Idle;rate_owned=false;start_pending=launch_requested=will_finish=false;
 }
};
}
