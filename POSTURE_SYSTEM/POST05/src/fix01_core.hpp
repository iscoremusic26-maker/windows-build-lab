#pragma once
// SESSION01 FIX01. Value-only joins and safety policy; no game pointers or writes.
#include "lnk_core.hpp"
#include "reaction_core.hpp"
namespace lnks1fix {
inline bool finite(float f) noexcept { return f==f && f>-1.0e20f && f<1.0e20f; }
struct NativeDamage {
 std::uint64_t tick=0; std::uintptr_t link_key=0, collider=0;
 std::uint32_t link_id=0; int cylinder=-1, timer_before=0,timer_after=0,life_before=0,life_after=0;
 float pending_before=0,pending_after=0;
 bool valid=false,human=false,blocked=false,event=false,poly_seen=false,electric=false;
 int poly_result=-1;
 bool resolved() const noexcept {
  return valid&&human&&!event&&!blocked&&poly_seen&&poly_result==0&&!electric&&link_id&&collider&&cylinder>=0&&cylinder<3&&
   timer_before==0&&timer_after>0&&life_before>0&&life_after>0&&finite(pending_before)&&finite(pending_after)&&
   (life_after<life_before||(pending_after<0&&pending_after<pending_before));
 }
};
// Deferred until the NEXT simulation update, joining events from the SAME pass
// independently of whether Link or the Bokoblin executes first. Never widens time.
class DirectJoin {
 public:
 struct Offer {bool on=false;lnk01::Contact k;std::uintptr_t sphere[2]{};};
 struct Result {bool present=false,matched=false;lnk01::Contact k;NativeDamage native;const char* why="NO_NATIVE_RECEIPT";};
 struct Counters {std::uint64_t offered=0,duplicates=0,superseded=0,native_calls=0,native_damage=0,native_rejected=0,matched=0,unmatched=0,ambiguous=0,stale=0,identity_lost=0;}c;
 void capture(const NativeDamage& n) noexcept {
  ++c.native_calls;if(!n.resolved()){++c.native_rejected;return;}++c.native_damage;
  auto&r=receipts_[n.tick%2];if(r.on&&r.n.tick==n.tick){r.ambiguous=true;++c.ambiguous;return;}
  r=Receipt{};r.on=true;r.n=n;
 }
 void offer(const lnk01::Contact& k,std::uintptr_t s0,std::uintptr_t s1,bool fresh) noexcept {
  if(k.slot<0||static_cast<unsigned>(k.slot)>=lnk01::kSlots)return;
  auto&o=offers_[k.slot];
  if(o.on&&o.k.id==k.id&&o.k.generation==k.generation&&o.k.token==k.token){++c.duplicates;return;}
  if(o.on)++c.superseded;
  if(!fresh||k.result!=post08::Result::NotBlocked||k.blocked||!k.token){++c.stale;return;}
  o=Offer{};o.on=true;o.k=k;o.sphere[0]=s0;o.sphere[1]=s1;++c.offered;
 }
 Result take(unsigned slot,std::uint64_t tick,bool identity_ok,std::uint32_t link_id,bool active) noexcept {
  Result z;if(slot>=lnk01::kSlots)return z;auto&o=offers_[slot];if(!o.on||o.k.tick>=tick)return z;
  z.present=true;z.k=o.k;o.on=false;z.k.tg_match=false;
  auto&r=receipts_[z.k.tick%2];
  if(z.k.tick+1!=tick){++c.stale;z.why="CONTACT_WRONG_TICK";}
  else if(!identity_ok||!active||z.k.link_id!=link_id){++c.identity_lost;z.k.link_ok=false;z.why="IDENTITY_OR_SIMULATION_LOST";}
  else if(r.on&&r.n.tick==z.k.tick&&r.n.link_id==link_id&&!r.ambiguous&&!r.used&&
          r.n.collider!=0&&(r.n.collider==o.sphere[0]||r.n.collider==o.sphere[1])){
   r.used=true;z.matched=z.k.tg_match=true;z.native=r.n;z.why="NATIVE_SELECTED_CYLINDER_AND_SCOPED_DAMAGE";++c.matched;return z;
  }
  ++c.unmatched;return z;
 }
 const Offer& offered(unsigned slot) const noexcept{return offers_[slot<lnk01::kSlots?slot:0];}
 void reset() noexcept {for(auto&o:offers_)o=Offer{};for(auto&r:receipts_)r=Receipt{};}
 private:
 struct Receipt {bool on=false,used=false,ambiguous=false;NativeDamage n;};
 Receipt receipts_[2]{};Offer offers_[lnk01::kSlots]{};
};
enum class Continuation:std::uint8_t {Safe,AnimationChanged,AttackClipMismatch,ReverseOrStoppedAttack,RecoilMismatch,DamageLoop,PhysicalChain,UnknownState};
inline const char* name(Continuation c) noexcept {const char*n[]={"SAFE_NATIVE_CONTINUATION","ANIMATION_CHANGED_IN_CALL","ATTACK_CLIP_MISMATCH","REVERSE_OR_ZERO_ATTACK_RATE","RECOIL_CLIP_MISMATCH","DAMAGE_LOOP_NOT_CONTINUABLE","NATIVE_PHYSICAL_CHAIN_PRIORITY","UNKNOWN_NATIVE_STATE"};return n[static_cast<unsigned>(c)];}
// Correct at the write's origin. Preserve a native reaction rather than restoring
// an ATTACK continuation whose clip cannot reach its native isStop exit.
inline Continuation continuation(int mode,int state,const post03::NativeAux& a,const post03::NativeAux& b) noexcept {
 if(!post05::same_anim(a,b))return Continuation::AnimationChanged;
 if(mode==6)return Continuation::PhysicalChain;
 if(mode==4){
  if(state==0)return finite(a.speed)?Continuation::Safe:Continuation::UnknownState; // native chooses its own attack clip
  if(state<1||state>3)return Continuation::UnknownState;
  if(!a.anim_ok||a.anim_attr!=0||!finite(a.anim_frame)||a.anim_frame<0||a.anim_frame>a.anim_end+0.01f||!finite(a.anim_rate))return Continuation::AttackClipMismatch;
  if(state==1||state==2){
   if(a.anim_end!=(state==1?42:47))return Continuation::AttackClipMismatch;
   if(a.anim_rate<=0)return Continuation::ReverseOrStoppedAttack;
  }else if((a.anim_end!=42&&a.anim_end!=47)||a.anim_rate>=0)return Continuation::RecoilMismatch;
 }
 if(mode==5&&state==5&&(!a.anim_ok||a.anim_attr!=0||a.anim_rate<=0))return Continuation::DamageLoop;
 return Continuation::Safe;
}

enum class AiVerdict:std::uint8_t {None,NormalRecovery,AttackLoop,RepeatedAborts,TransitionStall,DeadBeforeRecovery,ExpiredInconclusive,Count};
inline const char* name(AiVerdict v) noexcept {const char*n[]={"NONE","NORMAL_RECOVERY_OBSERVED","POSSIBLE_ATTACK_LOOP","REPEATED_ABORTS_WITHOUT_PROGRESS","POSSIBLE_STATE_TRANSITION_STALL","ACTOR_DEAD_BEFORE_RECOVERY_VERIFIED","MONITOR_EXPIRED_INCONCLUSIVE"};return n[static_cast<unsigned>(v)];}
struct AiSample {
 std::uint64_t tick=0;std::uint32_t id=0;std::uint64_t generation=0;int slot=-1,mode=-1,state=-1,hp=0,atp=0;
 post03::NativeAux aux;float distance=-1;bool target=false,guard=false,event=false,hit=false,dead=false,ko=false;
 lnk01::policy::Variant variant=lnk01::policy::Variant::Unknown;
};
class AiMonitor {
 public:
 static constexpr std::uint64_t kWindow=240; // diagnostic only; NEVER a watchdog or a write deadline
 struct S {bool bound=false,on=false,suspected=false,in_swing=false,armed=false,interrupted=false,native_completed=false;std::uint32_t id=0;std::uint64_t gen=0,start=0,last_tick=0,last_intervention=0;unsigned starts=0,arms=0,completions=0,aborts=0,cancels=0,restarts=0,abort_run=0,restart_run=0,alternations=0,frozen=0;AiSample last;};
 struct Counters {std::uint64_t interventions=0,suppressions=0,unsafe=0,starts=0,armed=0,completed=0,aborts=0,cancelled=0,restarts=0,dead=0,expired=0,suspected=0,unresolved=0,resolved=0,recycled=0,by_variant[5]{},verdict[static_cast<unsigned>(AiVerdict::Count)]{};}c;
 void intervention(int slot,std::uint32_t id,std::uint64_t gen,std::uint64_t tick,bool applied,bool unsafe) noexcept {
  if(slot<0||static_cast<unsigned>(slot)>=lnk01::kSlots)return;auto&s=bind(slot,id,gen);if(!s.on){s.on=true;s.start=tick;}s.last_intervention=tick;
  ++c.interventions;if(applied)++c.suppressions;if(unsafe)++c.unsafe;
 }
 void native_attack(int slot,std::uint32_t id,std::uint64_t gen,bool actual_armed,bool completed=false) noexcept {
  if(slot<0||static_cast<unsigned>(slot)>=lnk01::kSlots)return;auto&s=s_[slot];if(!s.bound||s.id!=id||s.gen!=gen||!s.on)return;
  if(actual_armed&&!s.armed){s.armed=true;++s.arms;++c.armed;}
  if(completed)s.native_completed=true;
 }
 AiVerdict feed(const AiSample& x) noexcept {
  if(x.slot<0||static_cast<unsigned>(x.slot)>=lnk01::kSlots)return AiVerdict::None;auto&s=bind(x.slot,x.id,x.generation);if(!s.on)return AiVerdict::None;
  if(x.dead){++c.dead;if(s.suspected)++c.unresolved;s.on=false;return result(AiVerdict::DeadBeforeRecovery);}
  if(s.native_completed&&s.armed&&!s.interrupted&&x.target&&!x.ko&&!x.event){
   ++s.completions;++c.completed;++c.by_variant[static_cast<unsigned>(x.variant)%5];
   s.abort_run=s.restart_run=s.alternations=0;s.in_swing=s.armed=s.native_completed=false;
   if(s.suspected)++c.resolved;s.suspected=false;s.on=false;s.last=x;s.last_tick=x.tick;return result(AiVerdict::NormalRecovery);
  }
  const bool swing=x.mode==4&&(x.state==1||x.state==2||x.state==3);
  const bool eligible=x.target&&!x.event&&!x.ko&&finite(x.distance)&&x.distance>200&&x.distance<500;
  if(s.last_tick&&x.tick>s.last_tick){
   if(x.ko||x.event){s.interrupted=true;s.native_completed=false;}
   const bool exit=s.in_swing&&(!swing||(x.state<=2&&s.last.state==3));
   if(exit){
    ++s.aborts;++c.aborts;
    if(s.interrupted||x.hit||!eligible){++s.cancels;++c.cancelled;s.abort_run=0;}else ++s.abort_run;
    s.in_swing=false;s.armed=false;s.interrupted=false;s.native_completed=false;
   }
   if(swing&&s.in_swing&&x.state==s.last.state&&x.state!=3&&x.aux.anim_rate>0&&x.aux.anim_rate==s.last.aux.anim_rate&&
      x.aux.anim_frame+0.01f<s.last.aux.anim_frame&&!x.hit&&eligible){++s.restarts;++c.restarts;++s.restart_run;}
   if(eligible&&x.mode!=s.last.mode&&((x.mode==4&&s.last.mode==3)||(x.mode==3&&s.last.mode==4)))++s.alternations;
   const bool same=x.mode==s.last.mode&&x.state==s.last.state&&post05::same_anim(x.aux,s.last.aux)&&x.aux.t6c0==s.last.aux.t6c0&&x.aux.t6c2==s.last.aux.t6c2;
   s.frozen=same&&eligible&&swing?s.frozen+1:0;
  }
  if(swing&&!s.in_swing){s.in_swing=true;s.armed=false;s.interrupted=false;s.native_completed=false;++s.starts;++c.starts;}
  s.last=x;s.last_tick=x.tick;
  if(!s.suspected&&eligible&&(s.abort_run>=4||s.restart_run>=3||s.alternations>=10||s.frozen>=60)){
   s.suspected=true;++c.suspected;return result(s.frozen>=60?AiVerdict::TransitionStall:s.restart_run>=3?AiVerdict::AttackLoop:AiVerdict::RepeatedAborts);
  }
  if(x.tick-s.start>=kWindow){s.on=false;++c.expired;if(s.suspected)++c.unresolved;return result(AiVerdict::ExpiredInconclusive);}
  return AiVerdict::None;
 }
 AiVerdict expire_one(std::uint64_t tick) noexcept {for(auto&s:s_)if(s.on&&tick>s.last_tick+60&&tick>s.start+60){s.on=false;++c.expired;if(s.suspected)++c.unresolved;return result(AiVerdict::ExpiredInconclusive);}return AiVerdict::None;}
 unsigned active() const noexcept {unsigned n=0;for(const auto&s:s_)n+=s.on?1u:0u;return n;}
 unsigned pending_anomalies() const noexcept {unsigned n=0;for(const auto&s:s_)n+=s.on&&s.suspected?1u:0u;return n;}
 const S& slot(unsigned i)const noexcept{return s_[i<lnk01::kSlots?i:0];}
 private:
 AiVerdict result(AiVerdict v) noexcept {++c.verdict[static_cast<unsigned>(v)];return v;}
 S& bind(int i,std::uint32_t id,std::uint64_t gen) noexcept {auto&s=s_[i];if(!s.bound||s.id!=id||s.gen!=gen){if(s.bound&&s.on){++c.recycled;++c.unresolved;}s=S{};s.bound=true;s.id=id;s.gen=gen;}return s;}
 S s_[lnk01::kSlots]{};
};
}
