#pragma once
// CP17 Red-only value policy. Same implementation is compiled by DLL and laboratory; no game pointers, no native writes here.
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_red_ai {
using Variant=devf_variant::Variant;
constexpr unsigned kCapacity=64,kRandomPercent=12,kChargePercent=70,kBurstMin=120,kBurstMax=210,kPassive=105,kPassiveMin=180,kPassiveMax=300,kIntent=90,kCalm=150;
constexpr float kNormal=1.15f,kBurst=3.0f;constexpr double kReactRadius=900.,kPassiveMinDist=200.,kPassiveMaxDist=700.,kHeight=200.,kLeashMargin=100.;
constexpr int Find=devf_variant::A_FIND,Attack=devf_variant::A_ATTACK,FindStay=devf_variant::A_FIND_STAY,Sound=devf_variant::A_SOUND_WATCH;
struct Point{float x=0,y=0,z=0;};
inline bool finite(float x)noexcept{return x==x&&x>=-10000000.f&&x<=10000000.f;}
inline bool finite(Point p)noexcept{return finite(p.x)&&finite(p.y)&&finite(p.z);}
inline double xz2(Point a,Point b)noexcept{double x=double(a.x)-b.x,z=double(a.z)-b.z;return x*x+z*z;}
inline double absd(double x)noexcept{return x<0?-x:x;}
enum class Charge:std::uint8_t{None,Spin,Jump};
enum class Motion:std::uint8_t{Other,Quiet,Guard,Offensive,Dodge,Damage};
enum class Cause:std::uint8_t{Normal,Random,ChargeSpin,ChargeJump,Passive};
inline const char* cause_text(Cause c)noexcept{switch(c){case Cause::Random:return "RANDOM_OPPORTUNITY";case Cause::ChargeSpin:return "SPIN_OR_GREAT_SPIN_CHARGE";case Cause::ChargeJump:return "JUMP_STRIKE_CHARGE";case Cause::Passive:return "PASSIVITY_CONFIRMED";default:return "NORMAL_NATIVE_BLOW";}}
struct ChargeView{bool valid=false;Charge family=Charge::None;};
// Native daAlink PROC29/2F begin charge,2A sustained;28/30 are executing, not charging. Virtual calls are const daPy dispatch, never the mutating non-const overload.
inline ChargeView charge_view(unsigned proc,int var2,bool any,bool spin,bool jump,unsigned proc_max)noexcept{
 ChargeView v;if(proc>=proc_max)return v;const bool sustained=proc==0x2a;
 if(any!=sustained||spin!=(sustained&&var2==0)||jump!=(sustained&&var2!=0))return v;
 v.valid=true;if(proc==0x29||(sustained&&var2==0))v.family=Charge::Spin;else if(proc==0x2f||(sustained&&var2!=0))v.family=Charge::Jump;return v;
}
struct Actor{
 std::uint32_t id=0,gen=0;Variant variant=Variant::None;char stage[9]{};int room=-1,layer=-1,mode=-1,state=-1,health=0;
 Point pos{},home{};float range=0,frame=0,rate=1,end=0;bool ready=false,hit=false,leash_out=false,wall=false,incoming=false,perceived=false,b=false,c=false,stopped=false;int playmode=0;
};
struct Link{std::uint32_t id=0;bool present=false,basic=false,canary=false,safe=false,los=false;char stage[9]{};int room=-1,layer=-1,proc=-1,timer=-1;Point pos{};Charge charge=Charge::None;Motion motion=Motion::Other;};
inline bool context(const Actor&a,const Link&l)noexcept{return a.stage[0]&&std::memcmp(a.stage,l.stage,9)==0&&a.room==l.room&&a.layer==l.layer&&a.layer>=0&&a.layer<15;}
inline bool domain(const Actor&a)noexcept{return a.id&&a.gen&&a.variant==Variant::Red&&a.stage[0]&&a.room>=0&&a.room<64&&a.layer>=0&&a.layer<15&&finite(a.pos)&&finite(a.home)&&finite(a.range)&&a.range>=0;}
inline bool safe(const Actor&a)noexcept{return domain(a)&&a.health>0&&a.ready&&!a.hit&&!a.leash_out&&!devf_variant::blocks_regen(a.mode);}
inline bool fight(int m)noexcept{return m==Find||m==FindStay||m==Attack;}
inline bool blow(const Actor&a)noexcept{return a.mode==Attack&&((a.state==1&&a.b)||(a.state==2&&a.c));}
inline bool sameclip(const Actor&a,int clip)noexcept{return clip==5?a.b:clip==6?a.c:false;}
inline bool leash(const Actor&a,Point dest)noexcept{return finite(dest)&&finite(a.home)&&finite(a.pos)&&finite(a.range)&&a.range>=0&&!a.leash_out&&(a.range==0||(a.range>=kLeashMargin&&xz2(a.home,dest)<=double(a.range-kLeashMargin)*(a.range-kLeashMargin)&&xz2(a.home,a.pos)<=double(a.range)*a.range));}
inline bool viable(const Actor&a,const Link&l)noexcept{return safe(a)&&l.present&&l.basic&&l.canary&&l.safe&&l.los&&finite(l.pos)&&context(a,l)&&a.perceived&&!a.wall&&leash(a,l.pos)&&absd(double(a.pos.y)-l.pos.y)<=kHeight;}
struct State{
 std::uint32_t id=0,gen=0,rng=1,link_id=0;char stage[9]{};int room=-1,layer=-1;unsigned ticks=0,next_burst=0,next_passive=0,passive=0,attack_seq=0,logs=0,last_threat=0;
 bool link_pass_logged=false,random_allowed=false,combat=false,normal_since_burst=true,episode_done=false,link_fault=false,decided=false,owned=false,intent=false,rush=false,pre_valid=false,contact=false,sfx_pending=false;
 Charge episode=Charge::None;Cause intent_cause=Cause::Normal,blow_cause=Cause::Normal;unsigned intent_start=0,delay=0;Point intent_pos{};float own_rate=1,last_frame=0;int clip=0;
 int pre_mode=-1,pre_state=-1;float pre_frame=0;bool pre_b=false,pre_c=false;
};
inline std::uint32_t random(State&s)noexcept{auto x=s.rng? s.rng:0x6d2b79f5u;x^=x<<13;x^=x>>17;x^=x<<5;return s.rng=x;}
inline unsigned between(State&s,unsigned lo,unsigned hi)noexcept{return lo+random(s)%(hi-lo+1);}
inline bool chance(State&s,unsigned percent)noexcept{return random(s)%100<percent;}
inline void seed(State&s,std::uint32_t id,std::uint32_t gen,std::uint32_t stamp)noexcept{s.rng=(id*0x9e3779b9u)^(gen*0x85ebca6bu)^(stamp*0xc2b2ae35u)^0x27d4eb2du;if(!s.rng)s.rng=1;}
inline State& state_for(std::array<State,kCapacity>&states,unsigned slot,const Actor&a,std::uint32_t stamp)noexcept{
 auto&s=states[slot<kCapacity?slot:0];if(s.id!=a.id||s.gen!=a.gen||s.room!=a.room||s.layer!=a.layer||std::memcmp(s.stage,a.stage,9)!=0){s={};s.id=a.id;s.gen=a.gen;s.room=a.room;s.layer=a.layer;std::memcpy(s.stage,a.stage,9);seed(s,a.id,a.gen,stamp);}return s;
}
template<class Emit>inline void cancel(State&s,Emit&&emit,const char*why)noexcept{if(s.intent){emit("SPECIAL_CANCELLED",s.intent_cause,why);s.intent=false;s.rush=false;}}
inline bool cooldown(const State&s)noexcept{return s.ticks>=s.next_burst&&s.normal_since_burst;}
template<class Emit>inline void select(State&s,Cause why,Point pos,unsigned delay,Emit&&emit)noexcept{
 s.intent=true;s.rush=false;s.intent_cause=why;s.intent_start=s.ticks;s.delay=delay;s.intent_pos=pos;s.next_burst=s.ticks+between(s,kBurstMin,kBurstMax);
 if(why==Cause::Passive){s.next_passive=s.ticks+between(s,kPassiveMin,kPassiveMax);s.passive=0;}
 emit("REACTION_SELECTED",why,"ONE_BOUNDED_OPPORTUNITY_NOT_A_FORCED_ATTACK_OR_HIT");
}
// PRE only observes and queues. No forced action/range/position/timer/speed writes. Native FIND owns approach and collision at its native speed.
template<class Emit>inline void pre(State&s,const Actor&a,const Link&l,Emit&&emit)noexcept{
 ++s.ticks;s.random_allowed=l.present&&l.basic&&l.safe&&context(a,l);s.pre_valid=safe(a);s.pre_mode=a.mode;s.pre_state=a.state;s.pre_frame=a.frame;s.pre_b=a.b;s.pre_c=a.c;s.sfx_pending=false;
 const bool inblow=blow(a);
 if(!domain(a)){cancel(s,emit,"IDENTITY_CONTEXT_DOMAIN");s.pre_valid=false;return;}
 if(s.combat&&s.ticks-s.last_threat>=kCalm&&(!safe(a)||!fight(a.mode)||!l.present))s.combat=false;
 if(!safe(a)||!fight(a.mode)||(a.mode==Attack&&a.state>=3)||a.incoming){cancel(s,emit,a.incoming?"RECRUITMENT_OWNS_RECEIVER_NO_RED_RUSH":"NATIVE_PRIORITY_DAMAGE_GUARD_LEASH_EVENT_OR_OTHER_MODE");s.passive=0;s.episode=Charge::None;s.episode_done=false;return;}
 if(l.id!=s.link_id){cancel(s,emit,"LINK_ID_CHANGED_OR_ABSENT");s.link_id=l.id;s.episode=Charge::None;s.episode_done=false;s.passive=0;s.link_fault=false;s.link_pass_logged=false;s.combat=false;}
 if(l.present&&l.basic&&a.perceived&&context(a,l)){s.combat=true;s.last_threat=s.ticks;}
 if(s.combat&&s.ticks-s.last_threat>=kCalm){s.combat=false;cancel(s,emit,"150_OWN_EXECUTES_WITHOUT_CURRENT_NATIVE_PERCEPTION");}
 if(l.present&&l.basic&&l.canary&&!s.link_pass_logged){s.link_pass_logged=true;emit("INDEPENDENT_LINK_STATE_CANARY_PASS",Cause::Normal,"PROC_DOMAIN_AND_CONST_VIRTUAL_SEMANTICS_COHERENT_NOT_PHYSICAL_LAYOUT_HUMAN_CERTIFICATION");}
 if(l.present&&l.basic&&!l.canary&&!s.link_fault){s.link_fault=true;emit("INDEPENDENT_LINK_STATE_CANARY_FAILED_REACTIVE_DISABLED",Cause::Normal,"CP14_CANARY_UNCHANGED_NORMAL_REMAINS_RANDOM_REQUIRES_SAFE_BASIC_LINK");}
 const bool ok=s.combat&&!s.link_fault&&viable(a,l);
 if(!ok){cancel(s,emit,"LINK_REACTIVE_SAMPLE_CONTEXT_LOS_GROUND_LEASH_OR_CANARY_UNSAFE");s.passive=0;s.episode=Charge::None;s.episode_done=false;return;}
 if(s.episode!=l.charge){s.episode=l.charge;s.episode_done=false;if(l.charge!=Charge::None)emit("ACTUAL_CHARGE_DETECTED",l.charge==Charge::Spin?Cause::ChargeSpin:Cause::ChargeJump,"VALIDATED_PROC_AND_CONST_VIRTUALS_NOT_A_HIT_OR_INTERRUPT");}
 if(s.intent){
  const bool charge_intent=s.intent_cause==Cause::ChargeSpin||s.intent_cause==Cause::ChargeJump;
  const Charge required=s.intent_cause==Cause::ChargeJump?Charge::Jump:Charge::Spin;
  if(s.ticks-s.intent_start>=kIntent)cancel(s,emit,"90_OWN_EXECUTE_INTENT_DEADLINE");
  else if(charge_intent&&l.charge!=required)cancel(s,emit,"CHARGE_FINISHED_CANCELLED_OR_EXECUTING_BEFORE_NATIVE_BLOW");
  else if(!charge_intent&&l.charge!=Charge::None)cancel(s,emit,"CHARGE_PRIORITY_OVER_PASSIVITY");
  else if(s.intent_cause==Cause::Passive&&l.motion!=Motion::Quiet&&l.motion!=Motion::Guard)cancel(s,emit,"LINK_NO_LONGER_PASSIVE");
 }
 const double d=xz2(a.pos,l.pos);
 const bool quiet=l.charge==Charge::None&&(l.motion==Motion::Quiet||l.motion==Motion::Guard)&&d>=kPassiveMinDist*kPassiveMinDist&&d<=kPassiveMaxDist*kPassiveMaxDist;
 if(quiet){if(s.passive<kPassive)++s.passive;}else s.passive=0;
 if(!inblow&&l.charge!=Charge::None&&!s.episode_done){
  s.episode_done=true;const auto c=l.charge==Charge::Spin?Cause::ChargeSpin:Cause::ChargeJump;
  if(d<=kReactRadius*kReactRadius&&cooldown(s)){if(chance(s,kChargePercent))select(s,c,a.pos,between(s,0,8),emit);else emit("REACTION_NOT_SELECTED",c,"SINGLE_70_PERCENT_ROLL_EPISODE_CONSUMED");}
  else emit("REACTION_SKIPPED",c,"RADIUS_OR_BURST_COOLDOWN_EPISODE_CONSUMED");
 }else if(!inblow&&l.charge==Charge::None&&!s.intent&&quiet&&s.passive>=kPassive&&s.ticks>=s.next_passive&&cooldown(s))select(s,Cause::Passive,a.pos,0,emit);
 if(s.intent&&!s.rush&&s.ticks-s.intent_start>=s.delay&&a.mode==Find&&a.state==4&&xz2(a.pos,s.intent_pos)>=25.&&xz2(a.pos,l.pos)<xz2(s.intent_pos,l.pos)){
  s.rush=true;emit("RUSH_STARTED",s.intent_cause,"MEASURED_NATIVE_FIND_APPROACH_NOT_A_SPEED_BOOST_OR_ARRIVAL");
 }
}
// Native checkPass mode0 uses [frame,min(frame+rate,end-.001)); executeAttack B uses else-if10/12.5/19.
inline bool pass(float frame,float rate,float end,float target)noexcept{float next=frame+rate;if(next>=end)next=end-.001f;return frame<=next&&frame<=target&&target<next;}
inline bool lost_b_sfx(const Actor&a)noexcept{return blow(a)&&a.b&&a.playmode==0&&finite(a.end)&&a.end>25&&a.rate==kBurst&&pass(a.frame,a.rate,a.end,10)&&pass(a.frame,a.rate,a.end,12.5f);}
inline bool own_live(const State&s,const Actor&a)noexcept{return s.owned&&sameclip(a,s.clip)&&a.rate==s.own_rate&&a.frame>=s.last_frame&&finite(a.frame)&&a.frame<=a.end&&a.playmode==0;}
struct Rate{bool write=false,repair_sfx=false;float value=1;Cause cause=Cause::Normal;};
template<class Emit>inline Rate post(State&s,const Actor&a,Emit&&emit)noexcept{
 Rate out;out.cause=s.blow_cause;
 // A pending repair is authorized only by the actual native B else-if collision in PRE and same B animation after execute (shield recoil retained).
 out.repair_sfx=s.sfx_pending&&s.pre_valid&&a.mode==Attack&&a.b&&a.state==1&&a.rate==kBurst&&finite(a.frame)&&absd(double(a.frame)-(s.pre_frame+kBurst))<.01;s.sfx_pending=false;
 if(!blow(a)){
  if(s.owned&&sameclip(a,s.clip)&&a.rate==s.own_rate&&finite(a.frame)&&a.frame>=s.last_frame&&!a.stopped){out.write=true;out.value=1;emit("RATE_RESTORED",s.blow_cause,"ONLY_STILL_OWNED_BC_RATE_NATIVE_NEGATIVE_RECOIL_NOT_WRITTEN");}
  if(s.decided)emit("NATIVE_ATTACK_ENDED",s.blow_cause,s.contact?"NATIVE_CONTACT_FLAG_OBSERVED_EFFECTIVE_DAMAGE_AND_INTERRUPT_UNKNOWN":"CONTACT_AND_INTERRUPT_NOT_CONFIRMED");
  s.decided=false;s.owned=false;s.contact=false;s.clip=0;return out;
 }
 if(s.decided){if(s.owned&&!own_live(s,a)){s.owned=false;emit("RATE_OWNERSHIP_RELEASED",s.blow_cause,"NATIVE_OR_FOREIGN_RATE_FRAME_OR_CLIP_CHANGED_NO_OVERWRITE");}if(s.owned)s.last_frame=a.frame;return out;}
 s.decided=true;++s.attack_seq;
 if(!s.pre_valid||!safe(a)){emit("RATE_SKIPPED",Cause::Normal,"NATIVE_PRIORITY_UNSAFE");return out;}
 if(a.playmode!=0||!finite(a.end)||a.end<=25){emit("RATE_SKIPPED",Cause::Normal,"NATIVE_CLIP_RANGE_OR_PLAY_MODE_NOT_SUPPORTED");return out;}
 if(s.pre_mode!=Attack||s.pre_state!=0||!(a.frame>=.999f&&a.frame<=1.001f)){emit("RATE_SKIPPED",Cause::Normal,"NOT_A_FRESH_NATIVE_BC_START");return out;}
 if(a.rate!=1.0f){emit("RATE_SKIPPED",Cause::Normal,"FOREIGN_OR_NATIVE_NONDEFAULT_RATE");return out;}
 Cause cause=Cause::Normal;
 if(s.intent&&s.ticks-s.intent_start>=s.delay&&s.ticks-s.intent_start<kIntent){cause=s.intent_cause;s.intent=false;s.rush=false;}
 else if(s.combat&&s.random_allowed&&!a.incoming&&cooldown(s)&&chance(s,kRandomPercent)){cause=Cause::Random;s.next_burst=s.ticks+between(s,kBurstMin,kBurstMax);}
 const float chosen=cause==Cause::Normal?kNormal:kBurst;
 if(cause==Cause::Normal)s.normal_since_burst=true;else s.normal_since_burst=false;
 s.blow_cause=cause;s.own_rate=chosen;s.clip=a.b?5:6;s.last_frame=a.frame;s.owned=true;out={true,false,chosen,cause};
 emit("NATIVE_ATTACK_STARTED",cause,"NATIVE_BC_CHOICE_PRESERVED_RATE_DECIDED_ONCE_INTERRUPT_ATTEMPT_ONLY");return out;
}
// Caller samples BEFORE native execute so compensation is never guessed from a post-frame alone.
inline void before_blow(State&s,const Actor&a)noexcept{s.sfx_pending=safe(a)&&s.pre_valid&&s.owned&&own_live(s,a)&&lost_b_sfx(a);}
inline bool restore_on_shutdown(const State&s,const Actor&a)noexcept{return domain(a)&&a.id==s.id&&a.gen==s.gen&&own_live(s,a)&&blow(a)&&!a.stopped;}
struct Budget{unsigned global=0,suppressed=0;bool take(State&s)noexcept{if(global>=900||s.logs>=48){++suppressed;return false;}++global;++s.logs;return true;}};
}
