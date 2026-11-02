#include "../src/break_core.hpp"
#include "../src/helm_core.hpp"
#include <cstdio>
#include <initializer_list>
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19_policy.hpp"
using namespace post06;
unsigned checks=0,failures=0;void ck(bool b){++checks;if(!b)++failures;}
View view(){View v;v.tracked=true;v.slot=0;v.id=100;v.generation=1;v.pre_hp=v.native_hp=500;v.mode=5;v.state=0;v.writer_ok=true;return v;}
void open(Breaker&b,View&v){v.hit=v.crossed=v.latched=true;auto a=b.step(v);ck(a.act==Act::Stun);b.commit(v,a,true);ck(b.slot(0).phase==Phase::Punish);v.hit=v.crossed=false;v.mode=5;v.state=5;}
int main(){
 for(int mode:{0,1,3,4,5,7,8,15,16})for(bool strong:{false,true}){Breaker b;b.enabled=true;auto v=view();v.pre_mode=mode;v.mode=strong?6:5;open(b,v);ck(b.c.bonus_applied==0);}
 for(bool shield_only:{false,true}){Breaker b;b.enabled=true;auto v=view();open(b,v);for(int i=1;i<=90;++i){v.hit=shield_only&&i%4==0;v.shield=v.hit;v.from_link=true;auto a=b.step(v);ck(a.act!=Act::Bonus);if(i<90)ck(a.act==Act::None||a.act==Act::Stun);else ck(a.act==Act::Recover&&a.rearm_to==50);b.commit(v,a,true);}ck(b.slot(0).phase==Phase::Tracking&&b.c.bonus_applied==0);}
 for(int base=1;base<=100;++base)for(bool helm:{false,true}){Breaker b;b.enabled=true;auto v=view();open(b,v);if(helm){v.hit=v.shield=v.from_link=true;auto a=b.step(v);ck(a.act==Act::Stun);b.commit(v,a,true);v.shield=false;}
  v.hit=v.sword=v.from_link=true;v.helm=helm;v.pre_hp=500;v.native_hp=500-base;auto a=b.step(v);ck(a.act==Act::Bonus&&a.extra==(base+1)/2&&a.base==base&&a.convert_to_big);b.commit(v,a,true);b.commit(v,a,true);ck(b.c.bonus_applied==1&&b.slot(0).phase==Phase::Falling);
  a=b.step(v);ck(a.act!=Act::Bonus);v.hit=false;v.mode=6;v.state=5;b.step(v);ck(b.slot(0).phase==Phase::Down);v.state=6;b.step(v);ck(b.slot(0).phase==Phase::GettingUp);v.mode=3;v.state=5;a=b.step(v);ck(a.act==Act::Rearm&&a.rearm_to==0);b.commit(v,a,true);ck(b.slot(0).phase==Phase::Tracking);
 }
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.hit=v.sword=v.from_link=v.helm=true;v.native_hp=400;auto a=b.step(v);ck(a.act!=Act::Bonus);}
 for(int i=0;i<60000;++i){Breaker b;b.enabled=true;auto v=view();v.id=100+i;v.generation=1+i;open(b,v);v.hit=(i%3)==0;v.sword=true;v.from_link=false;v.native_hp=490;auto a=b.step(v);ck(a.act!=Act::Bonus);v.id++;v.generation++;v.hit=false;v.latched=false;v.mode=3;b.step(v);ck(b.slot(0).phase==Phase::Tracking&&!b.slot(0).shield_prepared);}
 for(int floor:{50,100})for(int pre:{20,50,100,200,500})for(int native:{0,1,10,40,100,190,480}){const auto a=normalize_helm(pre,native,floor,true);ck(a.hp<=native);if(native>0&&native<pre){const int effective=pre-native>floor?pre-native:floor;ck(a.hp==(pre>effective?pre-effective:0));}else ck(!a.write);}
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.stun_ended=true;auto a=b.step(v);ck(a.act==Act::Recover&&a.rearm_to==50);}
 {Breaker b;b.enabled=true;auto v=view();v.hit=v.crossed=true;auto a=b.step(v);b.commit(v,a,false);ck(b.slot(0).phase==Phase::Tracking&&b.c.write_failed==1);}
 {Breaker b;b.enabled=true;auto v=view();open(b,v);v.writer_ok=false;v.mode=3;auto a=b.step(v);ck(a.act==Act::Recover&&a.rearm_to==50);}
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
  for(int e=1;e<=4;++e){h.event=e;pe.px.consume(h,L);}ck(pe.px.entry(0).latched);ck(pe.px.rearm(0,h.generation,7,50));ck(pe.px.entry(0).posture==50&&!pe.px.entry(0).latched);ck(pe.px.audit().ok);ck(!pe.px.rearm(0,h.generation+1,7,0));
 }
 std::printf("POST06_REWORK_LAB checks=%u failures=%u RESULT=%s\n",checks,failures,failures?"LAB_FAIL":"LAB_PASS");return failures?1:0;
}
