#include "../src/break_core.hpp"
#include "../src/helm_core.hpp"
#include <cstdio>
#include <initializer_list>
#include <string_view>
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19_policy.hpp"
using namespace post06;
unsigned checks=0,failures=0;void ck(bool b){++checks;if(!b)++failures;}
View view(){View v;v.tracked=true;v.slot=0;v.id=100;v.generation=1;v.pre_hp=v.native_hp=500;v.mode=5;v.state=0;v.writer_ok=true;return v;}
void open(Breaker&b,View&v){v.hit=v.crossed=v.latched=true;auto a=b.step(v);ck(a.act==Act::Stun);b.commit(v,a,true);ck(b.slot(0).phase==Phase::Punish);v.hit=v.crossed=false;v.mode=5;v.state=5;}
int main(){
 for(std::uintptr_t selected:{0,100,200,300,400,500,600}){
  const auto c=classify_link_contact(selected,100,200,300,400,500);
  ck(c==(selected==500?LinkContact::Shield:(selected>=100&&selected<=400)?LinkContact::Sword:LinkContact::None));
 }
 // End to end: the actual shield collider prepares the native Helm path.
 {Breaker b;b.enabled=true;auto v=view();open(b,v);
  const auto c=classify_link_contact(500,100,200,300,400,500);
  v.hit=true;v.shield=c==LinkContact::Shield;v.from_link=c!=LinkContact::None;
  auto a=b.step(v);ck(a.act==Act::Stun&&a.why==Why::ShieldPrepared);b.commit(v,a,true);
  v.shield=false;v.helm=v.sword=true;v.native_hp=450;a=b.step(v);ck(a.act==Act::Bonus&&a.why==Why::DecisiveHelm);
 }

 for(int mode:{0,1,3,4,5,7,8,15,16})for(bool strong:{false,true}){Breaker b;b.enabled=true;auto v=view();v.pre_mode=mode;v.mode=strong?6:5;open(b,v);ck(b.c.bonus_applied==0);}
 for(bool shield_only:{false,true}){Breaker b;b.enabled=true;auto v=view();open(b,v);for(int i=1;i<=90;++i){v.hit=shield_only&&i%4==0;v.shield=v.hit;v.from_link=true;auto a=b.step(v);ck(a.act!=Act::Bonus);if(i<90)ck(a.act==Act::None||a.act==Act::Stun);else ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm);b.commit(v,a,true);}ck(b.slot(0).phase==Phase::Tracking&&b.c.bonus_applied==0);}
 for(int base=1;base<=100;++base)for(bool helm:{false,true}){Breaker b;b.enabled=true;auto v=view();open(b,v);if(helm){v.hit=v.shield=v.from_link=true;auto a=b.step(v);ck(a.act==Act::Stun);b.commit(v,a,true);v.shield=false;}
  v.hit=v.sword=v.from_link=true;v.helm=helm;v.pre_hp=500;v.native_hp=500-base;auto a=b.step(v);ck(a.act==Act::Bonus&&a.extra==(base+1)/2&&a.base==base&&a.convert_to_big);b.commit(v,a,true);b.commit(v,a,true);ck(b.c.bonus_applied==1&&b.slot(0).phase==Phase::Falling);
  a=b.step(v);ck(a.act!=Act::Bonus);v.hit=false;v.mode=6;v.state=5;b.step(v);ck(b.slot(0).phase==Phase::Down);v.state=6;b.step(v);ck(b.slot(0).phase==Phase::GettingUp);v.mode=3;v.state=5;a=b.step(v);ck(a.act==Act::Rearm&&a.rearm_to==0);b.commit(v,a,true);ck(b.slot(0).phase==Phase::Tracking);
 }
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.hit=v.sword=v.from_link=v.helm=true;v.native_hp=400;auto a=b.step(v);ck(a.act!=Act::Bonus);}
 for(int i=0;i<60000;++i){Breaker b;b.enabled=true;auto v=view();v.id=100+i;v.generation=1+i;open(b,v);v.hit=(i%3)==0;v.sword=true;v.from_link=false;v.native_hp=490;auto a=b.step(v);ck(a.act!=Act::Bonus);v.id++;v.generation++;v.hit=false;v.latched=false;v.mode=3;b.step(v);ck(b.slot(0).phase==Phase::Tracking&&!b.slot(0).shield_prepared);}
 for(int floor:{50,100})for(int pre:{20,50,100,200,500})for(int native:{0,1,10,40,100,190,480}){const auto a=normalize_helm(pre,native,floor,true);ck(a.hp<=native);if(native>0&&native<pre){const int effective=pre-native>floor?pre-native:floor;ck(a.hp==(pre>effective?pre-effective:0));}else ck(!a.write);}
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.stun_ended=true;auto a=b.step(v);ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm);}
 {Breaker b;b.enabled=true;auto v=view();v.hit=v.crossed=true;auto a=b.step(v);b.commit(v,a,false);ck(b.slot(0).phase==Phase::Tracking&&b.c.write_failed==1);}
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.writer_ok=false;v.mode=3;auto a=b.step(v);ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm);}
 {Breaker b;b.enabled=true;auto v=view();v.writer_ok=false;v.latched=true;v.mode=3;auto a=b.step(v);ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm);}
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.hit=true;v.sword=false;v.mode=6;v.native_hp=450;auto a=b.step(v);ck(a.act==Act::Stun&&a.why==Why::NonDecisiveHold);b.commit(v,a,true);ck(b.slot(0).phase==Phase::Punish&&!b.slot(0).shield_prepared&&b.c.bonus_applied==0);}
 {HelmEpisode h;h.observe(7,1,99,true);h.seen=true;
  for(int i=0;i<20;++i){h.observe(7,1,99,true);ck(h.seen);}h.observe(7,1,0,false,false);ck(h.seen);
  h.observe(7,1,99,false);ck(!h.seen);h.seen=true;h.observe(7,2,99,true);ck(!h.seen);
 }
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.mode=14;for(int i=0;i<120;++i)ck(b.step(v).act==Act::None);v.mode=3;auto a=b.step(v);ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm);}
 for(unsigned mask=0;mask<32;++mask)ck(helm_contact_verified(mask&1,mask&2,mask&4,mask&8,mask&16)==(mask==31));
 // Exact CP21 POST is a monotone min: the independent effective Helm and
 // the decisive bonus must survive when Expansion was the owner of cut masking.
 for(int floor:{50,100})for(int loss:{1,10,50,100,150,200}){
  const int pre=500;auto independent=normalize_helm(pre,pre-loss,floor,true);
  const int base=pre-independent.hp;const int decisive=independent.hp-(base+1)/2;
  devf_cp19::Helm h;h.mask=true;h.pre_health=pre;h.damage=floor;
  const auto post=devf_cp19::helm_post(h,true,8,decisive);
  ck(post.unmask&&!post.write&&post.health==decisive);
 }
 {post04::PostureEngine pe;post03::Events ev;post03::PreReads r;r.facts.profile_ok=r.facts.live=r.facts.status_10000=true;r.facts.name=post03::NameKind::E_OC;r.facts.atp=1;
  pe.on_pre(0x10000,7,r,ev);auto& L=pe.eng.obs.ledger;post03::HitReport h;h.slot=0;h.id=7;h.generation=L.slot(0).generation;h.life=post03::Life::Active;h.j.kind=post03::HitKind::Confirmed;h.j.elig=post03::Eligibility::Confirmed;h.at_valid=true;h.fam.family=post03::Family::Sword;h.post.health=100;
  for(int e=1;e<=4;++e){h.event=e;pe.px.consume(h,L);}ck(pe.px.entry(0).latched);ck(pe.px.rearm(0,h.generation,7,postbal01::kStandingRearm));ck(pe.px.entry(0).posture==postbal01::kStandingRearm&&!pe.px.entry(0).latched);ck(pe.px.audit().ok);ck(!pe.px.rearm(0,h.generation+1,7,0));
 }
 // FIX_REARM50 (0.7.0). Real log pattern of id=161: decisive bonus -> BIG 5 -> BIG 6 -> (Expansion WATCH 3 shout in its execute PRE) -> damage_check sees
 // WATCH 3, unhit, alive. The get-up completed (same 49-tick native duration as id=163, which exited through FIND 5): rearm 0, not 50.
 auto to_getup=[](Breaker&b,View&v){open(b,v);v.hit=v.sword=v.from_link=true;v.pre_hp=500;v.native_hp=480;auto a=b.step(v);ck(a.act==Act::Bonus);b.commit(v,a,true);
  v.hit=v.sword=v.from_link=false;v.pre_hp=v.native_hp=a.new_hp;v.mode=6;v.state=1;b.step(v);v.state=5;b.step(v);ck(b.slot(0).phase==Phase::Down);v.state=6;b.step(v);ck(b.slot(0).phase==Phase::GettingUp);};
 {Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.mode=7;v.state=3;auto a=b.step(v);ck(a.act==Act::Rearm&&a.rearm_to==0&&a.why==Why::RearmAfterGetUpRedirected);b.commit(v,a,true);
  ck(b.slot(0).phase==Phase::Tracking&&b.c.rearms==1&&b.c.rearms_redirected==1&&b.c.rearms25==0);
  char line[400];View lv=v;ck(fmt_event(line,sizeof line,lv,b.slot(0),a,Phase::GettingUp,true)>0);std::string_view sv(line);
  ck(sv.find("why=REARM_AFTER_NATIVE_GET_UP_REDIRECTED_0")!=std::string_view::npos&&sv.find("rearm=0 mode=7 state=3 hit=0")!=std::string_view::npos);}
 // Native exit unchanged (FIND 5, with or without a hit in the same call).
 for(bool hit:{false,true}){Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.mode=3;v.state=5;v.hit=hit;auto a=b.step(v);ck(a.act==Act::Rearm&&a.rearm_to==0&&a.why==Why::RearmAfterGetUp);b.commit(v,a,true);ck(b.c.rearms_redirected==0);}
 // Every live normal mode after observed BIG 6, unhit: rearm 0. Same modes with a hit in the same call: conservative 50 (pre-0.7.0 behaviour).
 for(int mode:{0,1,3,4,5,7,8,15,16})for(int state:{0,1,2,3,4,6})for(bool hit:{false,true}){Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.mode=mode;v.state=state;v.hit=hit;auto a=b.step(v);
  if(!hit){ck(a.act==Act::Rearm&&a.rearm_to==0);}else ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm&&a.why==Why::TechnicalAbort);b.commit(v,a,true);ck(b.slot(0).phase==Phase::Tracking);}
 // Interrupted BEFORE the get-up (DOWN_FINISH, state 6 never observed): still 50, never 0.
 for(int mode:{0,1,3,4,5,7,8,15,16}){Breaker b;b.enabled=true;auto v=view();to_getup(b,v);Breaker c;c.enabled=true;auto w=view();open(c,w);w.hit=w.sword=w.from_link=true;w.native_hp=480;auto a=c.step(w);c.commit(w,a,true);
  w.hit=w.sword=w.from_link=false;w.pre_hp=w.native_hp=a.new_hp;w.mode=6;w.state=5;c.step(w);ck(c.slot(0).phase==Phase::Down);w.mode=mode;w.state=mode==3?4:0;a=c.step(w);ck(a.act==Act::Recover&&a.rearm_to==postbal01::kStandingRearm);ck(c.c.rearms==0);}
 // Death/water/fall death while getting up: DEAD, no rearm; 0 HP outside BIG: DEAD; scripted non-normal modes wait without acting.
 for(int mode:{9,10,13}){Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.mode=mode;v.state=0;auto a=b.step(v);ck(a.act==Act::None&&b.slot(0).phase==Phase::Dead);v.mode=3;v.state=5;ck(b.step(v).act==Act::None);}
 {Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.mode=3;v.state=0;v.native_hp=0;auto a=b.step(v);ck(a.act==Act::None&&b.slot(0).phase==Phase::Dead);}
 for(int mode:{2,11,12,14,17}){Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.mode=mode;ck(b.step(v).act==Act::None&&b.slot(0).phase==Phase::GettingUp);v.mode=7;v.state=3;ck(b.step(v).act==Act::Rearm);}
 // Recycled identity: a new generation in the same slot resets the cycle; it never inherits GETTING_UP nor rearms.
 {Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.generation++;v.mode=7;v.state=3;v.latched=false;ck(b.step(v).act==Act::None&&b.slot(0).phase==Phase::Tracking&&b.c.slot_resets==1);}
 // Writer closed for the session: the redirect path never runs (the closed-writer branch is evaluated first).
 {Breaker b;b.enabled=true;auto v=view();to_getup(b,v);v.writer_ok=false;v.mode=7;v.state=3;auto a=b.step(v);ck(a.act!=Act::Rearm);}
 std::printf("POST06_REWORK_LAB checks=%u failures=%u RESULT=%s\n",checks,failures,failures?"LAB_FAIL":"LAB_PASS");return failures?1:0;
}
