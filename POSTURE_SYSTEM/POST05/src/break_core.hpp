#pragma once
// POST06 rework: no action can start a fall without a future confirmed decisive contact.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "reaction_core.hpp"
namespace post06 {
inline constexpr const char* kBreakVersion="0.8.2";
namespace policy {
inline constexpr std::uint64_t kPunishMaxExec=90;
inline constexpr int kModeWait=0,kModeWalk=1,kModeFind=3,kModeAttack=4,kModeDamage=5,kModeBig=6,kModeWatch=7,kModeSound=8,kModeDeath=9,kModeWaterDeath=10,kModeFallDead=13,kModeStay=15,kModeMoveOut=16,kBigDown=5,kBigGettingUp=6;
inline constexpr bool drop_mode_safe(int m) noexcept {return m==0||m==1||m==3||m==4||m==5||m==7||m==8||m==15||m==16;}
inline constexpr bool death_mode(int m) noexcept{return m==9||m==10||m==13;}
inline constexpr std::uint8_t regen_rate_for(int hp,int max) noexcept {if(max<=0||hp<=0||hp>max)return 5;const long long p=100LL*hp;return p>75LL*max?5:p>50LL*max?4:p>25LL*max?3:p>10LL*max?2:1;}
}
enum class Phase:std::uint8_t{Tracking,Punish,Falling,Down,GettingUp,Dead,Count};
inline constexpr unsigned kPhaseCount=static_cast<unsigned>(Phase::Count);
inline const char* name(Phase p) noexcept {static constexpr const char* n[]={"TRACKING","STAGGER_OPEN","FALLING","DOWN_FINISH","GETTING_UP","DEAD"};return static_cast<unsigned>(p)<kPhaseCount?n[static_cast<unsigned>(p)]:"NONE";}
enum class Act:std::uint8_t{None,Stun,Bonus,Recover,Rearm};
enum class Why:std::uint8_t{None,BreakStanding,ShieldPrepared,NonDecisiveHold,DecisiveSword,DecisiveHelm,TimeoutStanding,NativeStunEnded,GettingUp,RearmAfterGetUp,Death,NativeKnockdown,TechnicalAbort,RearmAfterGetUpRedirected,BreakByParry};
inline const char* name(Why w) noexcept {switch(w){case Why::BreakStanding:return "BREAK_SHIELD_STUN";case Why::ShieldPrepared:return "SHIELD_PREPARED_NO_BONUS";case Why::NonDecisiveHold:return "NONDECISIVE_NATIVE_STUN_NO_BONUS";case Why::DecisiveSword:return "DECISIVE_SWORD_X1_50";case Why::DecisiveHelm:return "DECISIVE_HELM_X1_50";case Why::TimeoutStanding:return "TIMEOUT_ON_FEET_50";case Why::NativeStunEnded:return "NATIVE_STUN_ENDED_ON_FEET_50";case Why::GettingUp:return "GETTING_UP";case Why::RearmAfterGetUp:return "REARM_AFTER_NATIVE_GET_UP_0";case Why::Death:return "DEATH_NATIVE";case Why::NativeKnockdown:return "NATIVE_KNOCKDOWN_NO_BONUS";case Why::TechnicalAbort:return "TECHNICAL_ABORT_ON_FEET_50";case Why::RearmAfterGetUpRedirected:return "REARM_AFTER_NATIVE_GET_UP_REDIRECTED_0";case Why::BreakByParry:return "BREAK_BY_PARRY";default:return "NONE";}}
struct View {
 int slot=-1;std::uint64_t generation=0;std::uint32_t id=0;
 bool tracked=false;int pre_hp=0,native_hp=0,mode=0,state=0,pre_mode=0;
 bool hit=false,sword=false,shield=false,helm=false,from_link=false,crossed=false,latched=false,writer_ok=true,stun_ended=false;
 bool parry_crossed=false;  // POST08: el cruce a 100 lo produjo un parry confirmado (entrega de la ruptura)
 bool parry_break_pending=false;  // POST08 FIX02: parry cruzo 100 en la llamada ANTERIOR del actor; la ruptura llega en esta (executeAttack refresco el registro At)
};
struct Action {Act act=Act::None;Why why=Why::None;int base=0,extra=0,new_hp=0,rearm_to=0;bool lethal=false,convert_to_big=false;std::uint64_t token=0;};
struct BCounters {
 std::uint64_t calls=0,breaks=0,shields=0,decisive_sword=0,decisive_helm=0,bonus_applied=0,bonus_units=0,base_units=0,bonus_lethal=0,timeouts=0,native_stun_ends=0,downs=0,getups=0,rearms=0,rearms_redirected=0,breaks_by_parry=0,rearms50=0,deaths_in_cycle=0,write_failed=0,slot_resets=0,log_written=0,log_dropped=0;
};
class Breaker {
 public:
 BCounters c{};bool enabled=false;
 struct Slot {bool bound=false,shield_prepared=false;std::uint32_t id=0;std::uint64_t generation=0,exec=0,punish_start=0,last_commit=0;Phase phase=Phase::Tracking;int hp_max=0;};
 const Slot& slot(unsigned i)const noexcept{return s_[i<post03::kCapacity?i:0];}
 Action step(const View&v) noexcept {
  Action a;if(!enabled||!v.tracked||v.slot<0||static_cast<unsigned>(v.slot)>=post03::kCapacity)return a;
  auto&s=s_[v.slot];if(!s.bound||s.generation!=v.generation||s.id!=v.id){if(s.bound)++c.slot_resets;s=Slot{};s.bound=true;s.generation=v.generation;s.id=v.id;}
  ++s.exec;++c.calls;a.token=s.exec;if(v.pre_hp>s.hp_max)s.hp_max=v.pre_hp;if(v.native_hp>s.hp_max)s.hp_max=v.native_hp;
  if(policy::death_mode(v.mode)||(v.native_hp<=0&&v.mode!=policy::kModeBig)){if(s.phase!=Phase::Tracking&&s.phase!=Phase::Dead)++c.deaths_in_cycle;s.phase=Phase::Dead;a.why=Why::Death;return a;}
  if(s.phase==Phase::Dead)return a;
  if(!v.writer_ok){if(v.latched&&(v.mode==3||v.mode==16)){a.act=Act::Recover;a.rearm_to=50;a.why=Why::TechnicalAbort;}return a;}
  if(s.phase==Phase::Tracking){
   if(v.parry_break_pending&&v.latched&&!v.parry_crossed)return a;  // FIX02: espera UNA llamada a la ruptura aplazada; nunca rearme 50 entretanto
   if(v.hit&&v.crossed&&v.native_hp>1&&v.writer_ok&&policy::drop_mode_safe(v.pre_mode)&&(policy::drop_mode_safe(v.mode)||v.mode==6)){a.act=Act::Stun;a.why=Why::BreakStanding;}
   // POST08: la MISMA ruptura de pie (mareo nativo, castigo, recuperacion 50) cuando el cruce viene de un parry; nunca bonus ni caida, nunca prepara Helm.
   else if(v.parry_crossed&&v.latched&&v.native_hp>1&&v.writer_ok&&policy::drop_mode_safe(v.pre_mode)&&policy::drop_mode_safe(v.mode)){a.act=Act::Stun;a.why=Why::BreakByParry;}
   else if(v.latched&&!(v.hit&&v.crossed)&&!v.parry_crossed){if(v.mode==6){s.phase=Phase::Falling;}else if(policy::drop_mode_safe(v.mode)){a.act=Act::Recover;a.why=Why::TechnicalAbort;a.rearm_to=50;}}
   return a;
  }
  if(s.phase==Phase::Punish){
   // Absolute deadline precedes any decisive contact. Shield never renews it.
   if(s.exec-s.punish_start>=policy::kPunishMaxExec||v.stun_ended){if(v.mode==6){s.phase=Phase::Falling;return a;}if(!policy::drop_mode_safe(v.mode))return a;a.act=Act::Recover;a.why=v.stun_ended?Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=50;return a;}
   if(v.hit&&v.shield&&v.from_link&&v.native_hp>1){a.act=Act::Stun;a.why=Why::ShieldPrepared;return a;}
   if(v.hit&&v.sword&&v.from_link&&(!v.helm||s.shield_prepared)&&v.native_hp>1){
    const int base=v.pre_hp-v.native_hp;if(base>0&&v.writer_ok){a.act=Act::Bonus;a.why=v.helm?Why::DecisiveHelm:Why::DecisiveSword;a.base=base;a.extra=(base+1)/2;a.new_hp=v.native_hp>a.extra?v.native_hp-a.extra:0;a.lethal=a.new_hp==0;a.convert_to_big=v.mode!=6;return a;}
   }
   // A nondecisive nonlethal contact keeps the standing opportunity, never
   // converts an iron-ball/arrow/boomerang/shield impact into the decisive fall.
   if(v.hit&&v.native_hp>1&&(v.mode==5||v.mode==6)){a.act=Act::Stun;a.why=Why::NonDecisiveHold;return a;}
   // Unrelated physical transitions/death still win without a fabricated bonus.
   if(v.mode==6){s.phase=Phase::Falling;a.why=Why::NativeKnockdown;}
   return a;
  }
  if(v.mode==6){if(v.state==5&&s.phase!=Phase::Down){s.phase=Phase::Down;++c.downs;}else if(v.state==6&&s.phase!=Phase::GettingUp){s.phase=Phase::GettingUp;++c.getups;a.why=Why::GettingUp;}return a;}
  if(s.phase==Phase::GettingUp&&v.mode==3&&v.state==5){a.act=Act::Rearm;a.rearm_to=0;a.why=Why::RearmAfterGetUp;}
  // 0.7.0 (FIX_REARM50): native BIG state 6 was observed and the actor is alive, on its feet, unhit in this call, in a normal mode other than FIND 5:
  // the native exit setActionMode(FIND,5) was redirected by another actor-AI hook before this damage_check (e.g. a WATCH 3 shout). The get-up DID
  // complete, so the cycle rearms to 0 exactly as the FIND 5 exit; a hit in the same call keeps the conservative 50 (unchanged).
  else if(s.phase==Phase::GettingUp&&!v.hit&&v.native_hp>0&&policy::drop_mode_safe(v.mode)){a.act=Act::Rearm;a.rearm_to=0;a.why=Why::RearmAfterGetUpRedirected;}
  else if(policy::drop_mode_safe(v.mode)){a.act=Act::Recover;a.rearm_to=50;a.why=Why::TechnicalAbort;}
  return a;
 }
 void commit(const View&v,const Action&a,bool ok) noexcept {
  if(v.slot<0||static_cast<unsigned>(v.slot)>=post03::kCapacity)return;auto&s=s_[v.slot];if(!s.bound||s.id!=v.id||s.generation!=v.generation||a.token!=s.exec||a.token<=s.last_commit)return;
  if(!ok){++c.write_failed;return;}s.last_commit=a.token;
  switch(a.act){
   case Act::Stun:if(a.why==Why::BreakStanding||a.why==Why::BreakByParry){++c.breaks;if(a.why==Why::BreakByParry)++c.breaks_by_parry;s.phase=Phase::Punish;s.punish_start=s.exec;s.shield_prepared=false;}else if(a.why==Why::ShieldPrepared){s.shield_prepared=true;++c.shields;}break;
   case Act::Bonus:s.phase=Phase::Falling;++c.bonus_applied;c.base_units+=a.base;c.bonus_units+=a.extra;if(a.lethal)++c.bonus_lethal;if(a.why==Why::DecisiveHelm)++c.decisive_helm;else ++c.decisive_sword;break;
   case Act::Recover:s.phase=Phase::Tracking;s.shield_prepared=false;++c.rearms50;if(a.why==Why::NativeStunEnded)++c.native_stun_ends;else if(a.why==Why::TimeoutStanding)++c.timeouts;break;
   case Act::Rearm:s.phase=Phase::Tracking;s.shield_prepared=false;++c.rearms;if(a.why==Why::RearmAfterGetUpRedirected)++c.rearms_redirected;break;
   default:break;
  }
 }
 std::uint8_t regen_rate(int i,int hp)const noexcept{return i<0||static_cast<unsigned>(i)>=post03::kCapacity?5:policy::regen_rate_for(hp,s_[i].hp_max);}
 bool log_take()noexcept {if(logged_>=300){++c.log_dropped;return false;}++logged_;++c.log_written;return true;}
 private:Slot s_[post03::kCapacity]{};unsigned logged_=0;
};
inline int fmt_event(char*out,std::size_t n,const View&v,const Breaker::Slot&s,const Action&a,Phase before,bool ok) noexcept {return std::snprintf(out,n,"POST06_EVENT id=%u generation=%llu phase=%s>%s why=%s act=%u ok=%d age=%llu hp=%d>%d base=%d bonus=%d rearm=%d mode=%d state=%d hit=%d",v.id,static_cast<unsigned long long>(v.generation),name(before),name(s.phase),name(a.why),static_cast<unsigned>(a.act),int(ok),static_cast<unsigned long long>(s.exec-s.punish_start),v.pre_hp,a.act==Act::Bonus?a.new_hp:v.native_hp,a.base,a.extra,a.rearm_to,v.mode,v.state,int(v.hit));}
inline int fmt_summary(char*out,std::size_t n,const Breaker&b,const char*tag) noexcept {return std::snprintf(out,n,"POST06_SUMMARY v=%s tag=%s calls=%llu breaks=%llu shields=%llu bonus=%llu timeouts=%llu native_stun_ends=%llu rearms50=%llu rearms0=%llu rearms0_redirected=%llu failed=%llu",kBreakVersion,tag,static_cast<unsigned long long>(b.c.calls),static_cast<unsigned long long>(b.c.breaks),static_cast<unsigned long long>(b.c.shields),static_cast<unsigned long long>(b.c.bonus_applied),static_cast<unsigned long long>(b.c.timeouts),static_cast<unsigned long long>(b.c.native_stun_ends),static_cast<unsigned long long>(b.c.rearms50),static_cast<unsigned long long>(b.c.rearms),static_cast<unsigned long long>(b.c.rearms_redirected),static_cast<unsigned long long>(b.c.write_failed));}
}
