#include "../src/fix01_core.hpp"
#include <cstdio>
#include <cstring>
#include <limits>
#include <initializer_list>
using namespace lnks1fix;
unsigned checks=0,failures=0;
void ck(bool b,const char*what){++checks;if(!b){++failures;if(failures<20)std::printf("FAIL %s\n",what);}}
NativeDamage receipt(std::uint64_t t=10,std::uintptr_t sphere=100){NativeDamage n;n.valid=n.human=n.poly_seen=true;n.poly_result=0;n.tick=t;n.link_id=7;n.link_key=0x7000;n.collider=sphere;n.cylinder=0;n.timer_after=40;n.life_before=n.life_after=40;n.pending_after=-4;return n;}
lnk01::Contact contact(int slot=0,std::uint32_t id=100,std::uint64_t tick=10){lnk01::Contact k;k.result=post08::Result::NotBlocked;k.slot=slot;k.id=id;k.generation=1;k.token=1;k.link_ok=k.link_human=true;k.link_id=7;k.tick=tick;k.atp=1;return k;}
lnk01::LinkObs life(int timer=0,int hp=40){lnk01::LinkObs o;o.valid=o.human=o.active=true;o.id=7;o.life=hp;o.life_max=40;o.damage_timer=timer;return o;}
post03::NativeAux clip(int state=1,float rate=1){post03::NativeAux a;a.valid=a.anim_ok=1;a.anim_attr=0;a.anim_state=0;a.anim_end=state==2?47:42;a.anim_frame=3;a.anim_rate=rate;a.speed=0;a.set_bits=15;return a;}
AiSample sample(std::uint64_t t=1,int mode=4,int state=1,float frame=3){AiSample x;x.tick=t;x.slot=0;x.id=100;x.generation=1;x.mode=mode;x.state=state;x.hp=100;x.target=true;x.distance=300;x.aux=clip(state==2?2:1);x.aux.anim_frame=frame;x.variant=lnk01::policy::Variant::Blue;return x;}
int main(){
 // Reproduce the old timing failure: Link consumes a selected collider, sets
 // damage timer, and setCollision clears Tg before the Bokoblin callback.
 for(int variant=0;variant<7;++variant)for(bool link_first:{false,true}){
  lnk01::Posture old,now;old.observe_link(true,7);now.observe_link(true,7);
  lnk01::DirectEvents de;old.confirm_direct(life(),10,de);now.confirm_direct(life(),10,de);
  auto k=contact();k.e_oc2=variant!=0&&variant!=5;k.atp=variant==1||variant==5?2:variant==2?3:variant==3?6:variant==4?9:1;
  auto n=receipt();DirectJoin j;
  // Current callbacks have no Tg hit after native setCollision on direct damage.
  std::uintptr_t late_tg=0;k.tg_match=late_tg==100;ck(old.on_contact(k).v==lnk01::Verdict::RejectedDirectNotAttributed,"old adapter misses consumed direct hit");
  if(link_first)j.capture(n);j.offer(k,100,101,true);if(!link_first)j.capture(n);
  auto z=j.take(0,11,true,7,true);ck(z.matched&&z.k.tg_match,"same-pass native receipt joins in both actor orders");
  ck(now.on_contact(z.k).v==lnk01::Verdict::DirectPending,"one attributed pending direct");
  auto ob=life(39,36);now.confirm_direct(ob,11,de);
  const int want=variant==1||variant==5?35:variant==2?40:variant==3?45:30;
  ck(now.milli()==want*1000&&de.n==1&&de.e[0].r==lnk01::DirectResult::Confirmed,"all variant direct weights and real life loss");
  ck(!j.take(0,11,true,7,true).present,"join cannot issue twice");ck(now.on_contact(z.k).v==lnk01::Verdict::Duplicate,"one posture credit per attack");
  ck(now.conserved()&&old.milli()==0,"no life write and one conserved posture credit");
 }
 // Negative source selection: the FIRST native cylinder can belong to somebody
 // else. A later matching cylinder cannot steal the environmental/foreign hit.
 for(int bad=0;bad<17;++bad){auto n=receipt();switch(bad){
  case 0:n.blocked=true;break;case 1:n.timer_before=3;break;case 2:n.timer_after=0;break;
  case 3:n.pending_after=0;break;case 4:n.poly_result=1;break;case 5:n.poly_seen=false;break;
  case 6:n.electric=true;break;case 7:n.event=true;break;case 8:n.human=false;break;
  case 9:n.valid=false;break;case 10:n.collider=999;break;case 11:n.link_id=8;break;
  case 12:n.life_after=0;break;case 13:n.pending_before=n.pending_after=-4;break;
  case 14:n.pending_after=std::numeric_limits<float>::quiet_NaN();break;case 15:n.cylinder=-1;break;case 16:n.tick=9;break;
 }
  DirectJoin j;j.capture(n);j.offer(contact(),100,101,true);ck(!j.take(0,11,true,7,true).matched,"native source/timing/life/event negative");
 }
 {DirectJoin j;j.capture(receipt());auto a=contact();auto b=contact(1,200);j.offer(b,200,201,true);j.offer(a,100,101,true);
  ck(!j.take(1,11,true,7,true).matched,"two attackers: wrong source rejected");ck(j.take(0,11,true,7,true).matched,"two attackers: selected source only");}
 for(int bad=0;bad<5;++bad){DirectJoin j;auto n=receipt();j.capture(n);j.offer(contact(),100,101,bad!=0);
  auto z=j.take(0,bad==1?12:11,bad!=2,bad==3?8:7,bad!=4);ck(!z.matched,"stale/recycled/Link replacement/pause excluded");}
 {DirectJoin j;j.capture(receipt());j.capture(receipt());j.offer(contact(),100,101,true);ck(!j.take(0,11,true,7,true).matched,"ambiguous native resolutions reject");}
 {DirectJoin j;j.offer(contact(),100,101,true);j.offer(contact(),100,101,true);ck(j.c.duplicates==1,"duplicate contacts bounded");}
 for(auto r:{post08::Result::Confirmed,post08::Result::Duplicate,post08::Result::NormalGuard,post08::Result::StaleContact}){DirectJoin j;auto k=contact();k.result=r;j.offer(k,100,101,true);j.capture(receipt());ck(!j.take(0,11,true,7,true).matched,"parry/block/chip never double credit as direct");}
 // Zero-cost pause handling and next hit reset still use the unchanged counter.
 {lnk01::Posture p;p.observe_link(true,7);auto k=contact();k.tg_match=true;lnk01::DirectEvents d;p.confirm_direct(life(),10,d);p.on_contact(k);p.confirm_direct(life(40,36),11,d);for(int t=0;t<120;++t)p.tick(false,1000,true);ck(p.milli()==30000,"pause freezes regen after confirmed hit");for(int t=0;t<30;++t)p.tick(true,1000,false);ck(p.milli()==30000,"same30 tick delay");p.tick(true,1000,false);ck(p.milli()<30000,"recovery resumes");}
 // Guard the real process input, not a separate friendly oracle. Legacy policy
 // accepts the malformed state; the added adapter proof refuses its restoration.
 for(bool eoc2:{false,true})for(int state:{1,2,3})for(int fault=0;fault<5;++fault){auto a=clip(state==2?2:1,state==3?-1.f:1.3f);auto b=a;
  switch(fault){case 0:a.anim_attr=b.anim_attr=2;break;case 1:a.anim_rate=b.anim_rate=0;break;case 2:a.anim_ok=b.anim_ok=0;break;case 3:a.anim_end=b.anim_end=15;break;case 4:b.anim_frame+=1;break;}
  ck(continuation(4,state,a,b)!=Continuation::Safe,"unsafe attack continuation excluded for every shared resource");
  (void)eoc2;
 }
 for(float rate:{0.75f,1.f,1.15f,1.3f,2.f})for(int state:{1,2}){auto a=clip(state,rate);ck(continuation(4,state,a,a)==Continuation::Safe,"speed changes retain valid one-shot attacks");}
 {auto a=clip();a.anim_rate=-1;ck(continuation(4,1,a,a)!=Continuation::Safe,"reversed forward attack refused");ck(continuation(4,3,a,a)==Continuation::Safe,"authentic native recoil continues");a.anim_rate=1;ck(continuation(4,3,a,a)!=Continuation::Safe,"positive recoil refused");ck(continuation(6,5,a,a)==Continuation::PhysicalChain,"native Down/Finish chain wins");}
 // Source-derived native attack cycle: actual AtSet in14..22; reverse guard
 // recoil exits at0; forward B/C exits at end. Recovery requires an armed cycle.
 for(int enemies:{1,2,4,8})for(int variant=0;variant<5;++variant)for(float rate:{1.f,1.15f,1.3f,2.f})for(bool guard:{false,true}){
  AiMonitor m;for(int i=0;i<enemies;++i)m.intervention(i,100+i,1,0,true,false);
  float frame[8]{};bool armed[8]{},reverse[8]{},done[8]{};
  for(int t=1;t<90;++t)for(int i=0;i<enemies;++i){if(done[i])continue;auto x=sample(t,4,reverse[i]?3:1,frame[i]);x.slot=i;x.id=100+i;x.variant=static_cast<lnk01::policy::Variant>(variant);x.guard=guard;x.aux.anim_rate=reverse[i]?-1.f:rate;
   if((!reverse[i]&&frame[i]>=42)||(reverse[i]&&frame[i]<=0)){x.mode=3;x.state=0;m.native_attack(i,100+i,1,false,true);const auto v=m.feed(x);ck(v==AiVerdict::NormalRecovery,"armed native cycle finishes and control returns");done[i]=true;continue;}
   ck(m.feed(x)==AiVerdict::None,"normal attack is not a stall");
   const bool at=frame[i]>=14&&frame[i]<=22&&!reverse[i];m.native_attack(i,100+i,1,at);armed[i]|=at;
   if(at&&guard)reverse[i]=true;frame[i]+=reverse[i]?-1.f:rate;
  }
  for(int i=0;i<enemies;++i)ck(done[i]&&armed[i],"all mixed actors actually arm then complete");ck(m.c.suspected==0,"group/rate/guard variations do not create false loop");
 }
 {AiMonitor m;m.intervention(0,100,1,0,true,false);for(int t=1;t<20;++t)m.feed(sample(t,4,1,float(t)));auto dead=sample(20);dead.dead=true;ck(m.feed(dead)==AiVerdict::DeadBeforeRecovery&&m.c.resolved==0,"death never proves recovery");}
 // A previously armed attack does NOT excuse a subsequent run of empty loops.
 {AiMonitor m;m.intervention(0,100,1,0,true,false);m.feed(sample(1));m.native_attack(0,100,1,true);
  auto ko=sample(2,5,0);ko.ko=true;ko.hit=true;m.feed(ko);
  bool suspected=false;for(int cycle=0;cycle<6;++cycle){const int t=3+cycle*3;m.feed(sample(t,4,1,1));m.feed(sample(t+1,4,1,3));suspected|=m.feed(sample(t+2,3,4,0))==AiVerdict::RepeatedAborts;}
  ck(suspected&&m.c.suspected>0,"rolling abort streak after historical arms is detected");auto d=sample(30);d.dead=true;ck(m.feed(d)==AiVerdict::DeadBeforeRecovery&&m.c.unresolved>0,"anomaly retained after actor dies");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);bool suspected=false;for(int t=1;t<=25;++t){auto x=sample(t,4,1,float(t%6));suspected|=m.feed(x)==AiVerdict::AttackLoop;}ck(suspected,"animation restarts without offensive completion");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);for(int t=1;t<70;++t)m.feed(sample(t,4,1,3));ck(m.c.suspected==1,"constant attack transition diagnosed");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);for(int t=1;t<200;++t){auto x=sample(t,t%3?4:5,1,float(t%3));x.ko=x.mode==5;x.hit=x.ko;m.feed(x);}ck(m.c.suspected==0&&m.c.cancelled>0,"legitimate native damage cancellations are not stalls");}
 for(bool event:{false,true}){AiMonitor m;m.intervention(0,100,1,0,true,false);for(int t=1;t<240;++t){auto x=sample(t,3,4,3);x.event=event;x.distance=100;m.feed(x);}ck(m.c.suspected==0,"legitimate waiting close to Link/events not a stall");auto x=sample(240,3,4,3);x.distance=100;ck(m.feed(x)==AiVerdict::ExpiredInconclusive,"observed window expiry is inconclusive");AiMonitor missing;missing.intervention(0,100,1,1,true,false);ck(missing.expire_one(400)==AiVerdict::ExpiredInconclusive,"no proof of recovery when actor stops executing");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);for(int t=1;t<80;++t){auto x=sample(t);x.generation=2;m.feed(x);}ck(m.c.suspected==0&&m.c.recycled==1,"generations never share anomalies");}
 {post08::AiWatch w;w.start(0,100,1,0,post08::AiWatch::Cause::Suppression,false);post08::AiWatch::Sample x;x.mode=4;x.state=1;x.frame=15;w.feed(0,100,1,1,x,false,false);
  for(int t=2;t<101;++t)w.feed(0,100,1,t,x,false,false);x.frame=16;w.feed(0,100,1,101,x,false,false);const auto out=w.feed(0,100,1,102,x,true,false);
  ck(std::strcmp(out.verdict,"ACTOR_DEAD")==0&&w.c.resolved_natively==0,"legacy monitor cannot relabel dead actor as resolved");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);for(int t=1;t<160;++t){auto x=sample(t);x.target=false;m.feed(x);}ck(m.c.suspected==0,"invalid/dead Link target is not offensive stall evidence");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);m.feed(sample(1));m.native_attack(0,100,1,true);
  ck(m.feed(sample(2,3,4,0))!=AiVerdict::NormalRecovery&&m.c.completed==0,"armed but cancelled before native terminal branch is not recovery proof");}
 {AiMonitor m;m.intervention(0,100,1,0,true,false);m.feed(sample(1));m.native_attack(0,100,1,false,true);
  ck(m.feed(sample(2,3,4,0))!=AiVerdict::NormalRecovery,"native terminal without actual collider arm is not recovery proof");}
 // The same Reaction::decide receives the proof in production. A malformed
 // looping clip with ATTACK1/2 was actually suppression-eligible in0.10.0.
 for(int variant=0;variant<5;++variant)for(int state:{1,2}){
  post05::DecisionInput in;auto&r=in.rep;r.slot=0;r.id=100;r.generation=1;r.event=1;r.life=post03::Life::Active;
  r.j.kind=post03::HitKind::Confirmed;r.j.elig=post03::Eligibility::Confirmed;r.at_valid=true;r.fam.family=post03::Family::Sword;r.col.deref_ok=true;
  r.pre.health=500;r.pre.mode=4;r.pre.state=state;r.pre.old_mode=3;r.post.health=490;r.post.mode=5;r.post.state=0;r.post.old_mode=4;r.post.cooldown=10;
  r.aux_pre=clip(state,variant==3?2.f:1.3f);r.aux_pre.resource=static_cast<unsigned char>(variant?post03::NameKind::E_OC2:post03::NameKind::E_OC);
  r.aux_pre.atp=variant==0?1:variant==1?2:variant==2?3:variant==3?6:1;r.aux_pre.anim_attr=2;r.aux_post=r.aux_pre;
  in.outc.event=1;in.outc.slot=0;in.outc.generation=1;in.outc.applied=true;in.outc.before=0;in.outc.after=25;
  in.ledger_valid=true;in.adm=variant?post03::Admission::E_OC2_CANDIDATE:post03::Admission::E_OC_CANDIDATE;in.life=post03::Life::Active;
  post05::Reaction old,newer;old.stagger_policy=newer.stagger_policy=post05::StaggerPolicy::General;old.e2_policy=newer.e2_policy=post05::E2Policy::MatureBC;
  ck(old.decide(in).decision==post05::Decision::SuppressEligible,"0.10.0 accepts looping ATTACK continuation");
  in.continuation_safe=continuation(4,state,r.aux_pre,r.aux_post)==Continuation::Safe;
  const auto d=newer.decide(in);ck(d.reason==post05::Reason::UnsafeContinuation&&d.decision==post05::Decision::SkipUnsafe,"actual production decision refuses unsafe restore for Blue/Red/Black/fury");
 }
 {AiMonitor m;m.intervention(0,100,1,0,true,false);m.feed(sample(1,4,1,1));
  auto out=m.feed(sample(2,3,4,0));ck(out!=AiVerdict::NormalRecovery&&m.c.completed==0,"animation start and exit without real AtSet do not prove recovery");}
 std::printf("LNK_S1_FIX01_LAB checks=%u failures=%u RESULT=%s LAB_MODEL_ONLY\n",checks,failures,failures?"LAB_FAIL":"LAB_PASS");return failures?1:0;
}
