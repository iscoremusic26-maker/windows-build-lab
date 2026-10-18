// Same policy compiled into the .dusk; native movement below is a SOURCE PORT, not engine or real terrain execution.
#include "../runtime/DEVF/CP16_BLUE_RECRUITER_NATIVE_LEASH/expansion/blue_recruit_policy.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string>
#include <vector>
namespace r=devf_recruit;namespace b=devf_blue;using V=devf_variant::Variant;
unsigned cases=0;void need(bool p,const char* m){if(!p){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void test(const char*n,const std::function<void()>& f){f();++cases;std::printf("CASE_PASS %s\n",n);}
struct Events{std::vector<std::string> v;void operator()(const char*e,unsigned,unsigned,const r::Frame&,const char*){v.emplace_back(e);}unsigned count(const char*e)const{unsigned n=0;for(auto&s:v)if(s==e)++n;return n;}};
r::Frame frame(unsigned id,V v=V::Red,float x=1000){r::Frame f;f.id=id;f.gen=1;f.seq=10;f.variant=v;std::memcpy(f.stage,"F_SP121",8);f.room=3;f.layer=6;f.mode=r::Wait;f.health=v==V::Black?440:v==V::Red?330:220;f.pos={x,0,0};f.home=f.pos;f.range=5000;f.ready=true;return f;}
struct Lab{r::Table t;Events e;Lab(){t.publish(0,frame(1,V::Blue,0));t.publish(1,frame(2));}r::Entry& s(){return t.entries[0];}r::Entry& a(){return t.entries[1];}r::Call call(){return r::call(t,s(),true,10,8,e);}bool valid(){return r::order_valid(t,a(),10,8,[](auto&){return true;},e);}};
// Port of source SOUND_WATCH 0/1/2/3, WATCH3/4 and relevant FIND4 gate. Native cLib chase, angle smoothing and collision world represented explicitly.
// No guarantee of actual KCL navigation/audio/animation. LOS, wall, water and fall are controlled negative inputs, never treated as engine evidence.
struct Native{
 int mode=r::Wait,state=0,old=r::Wait,anim=0,c0=0;std::int16_t c2=0;float x=1000,z=0,speed=0;int shape=-16384;unsigned blows=0,cries=0,voiceFind=0,animation=0;bool wall=false,water=false,pit=false,hit=false,detect=false,shield=false;
 void set(int m,int s){old=mode;mode=m;state=s;c0=c2=0;}
 void step(r::Point goal){
  if(c0>0)--c0;if(c2>0)--c2;
  if(hit){set(devf_variant::A_DAMAGE,0);return;}if(water){set(devf_variant::A_WATER_DEATH,0);return;}if(pit){set(devf_variant::A_FALL,0);return;}
  if(mode==r::Sound){
   if(state==2&&detect){set(r::Find,3);return;}
   switch(state){case 0:speed=0;anim=13;++voiceFind;c0=60;state=1;[[fallthrough]];
    case 1:if(c0==0){anim=11;state=2;c0=60;}break;
    case 2:{if(speed<20)speed+=1;const double dx=goal.x-x,dz=goal.z-z;const int target=int(std::lround(std::atan2(dx,dz)*32768./3.141592653589793));
     const auto diff=std::int16_t(target-shape);int q=diff/4;if(q>0x800)q=0x800;if(q< -0x800)q=-0x800;if(std::abs(q)<0x100)q=diff>=0?std::min(0x100,int(diff)):std::max(-0x100,int(diff));shape=std::int16_t(shape+q);
     if(std::sqrt(dx*dx+dz*dz)<300||c0==0){anim=22;state=3;speed=0;}break;}
    case 3:if(++animation>=20)set(r::Wait,0);break;
   }
  }else if(mode==r::Watch){if(state==3&&c0==0){state=4;anim=7;++cries;animation=0;}else if(state==4&&++animation>=30)set(r::Find,2);}
  else if(mode==r::Find&&state==4&&!shield){if(c2==0){set(r::Attack,0);++blows;}}
  if(!wall&&mode==r::Sound&&state==2){const double a=shape*3.141592653589793/32768.;x+=float(std::sin(a)*speed);z+=float(std::cos(a)*speed);}
 }
};
struct Run {float moved=0;unsigned arrived=0,approach=0,aborted=0,waitTicks=0;bool pending=false;};
Run travel(bool command,bool wall=false,bool water=false,bool pit=false,float start=1000,bool detected=false){
 Lab l;l.a().f.pos.x=start;l.a().f.home=l.a().f.pos;if(command)l.call();Native n;n.x=start;n.wall=wall;n.water=water;n.pit=pit;n.detect=detected;bool consumed=false;
 for(unsigned tick=0;tick<260;++tick){auto f=l.a().f;f.mode=n.mode;f.state=n.state;f.pos={n.x,0,n.z};f.seq=10;f.ready=!water&&!pit;auto& a=l.t.publish(1,f);l.s().ticks++;
  if(r::order_valid(l.t,a,10,8,[](auto&){return true;},l.e)){
   if(!a.order.consumed){n.set(r::Sound,0);r::consumed(a,l.e);consumed=true;}
   else if(n.mode==r::Sound&&n.state==2&&n.c0<2)n.c0=2;
  }
  if(consumed&&!a.order.active&&n.mode==r::Sound)n.set(r::Wait,0);
  n.step(a.order.goal);a.f.mode=n.mode;a.f.state=n.state;a.f.pos={n.x,0,n.z};r::after(l.t,a,l.e);
  r::waiting(l.t,l.s(),10,8,l.e);
 }
 return{start-n.x,l.e.count("RECRUIT_ARRIVED"),l.e.count("RECRUIT_APPROACH_STARTED"),l.e.count("RECRUIT_ABORTED"),0,l.s().combat.waiting};
}
int main(){
 test("all-Blue-Red-Black-recipients-not-only-two",[]{Lab l;for(unsigned i=1;i<64;++i)l.t.publish(i,frame(i+1,i%3==0?V::Blue:i%3==1?V::Red:V::Black,700.f+i));auto c=l.call();need(c.offered==63,"63 recipients");});
 test("radius-exact-1800-height-250",[]{Lab l;l.a().f.pos={1800,250,0};need(l.call().offered==1,"inclusive");});
 test("radius-outside-1800",[]{Lab l;l.a().f.pos.x=1800.01f;need(!l.call().offered,"XZ");});
 test("height-outside-250",[]{Lab l;l.a().f.pos.y=250.01f;need(!l.call().offered,"Y");});
 test("different-room-stage-layer-blocked",[]{for(int n=0;n<3;++n){Lab l;if(n==0)l.a().f.room=4;if(n==1)std::memcpy(l.a().f.stage,"F_SP113",8);if(n==2)l.a().f.layer=5;need(!l.call().offered,"context");}});
 test("unknown-layer-never-assumed",[]{Lab l;l.a().f.layer=-1;need(!l.call().offered,"layer");});
 test("dead-demo-damage-attack-guard-no-interrupt",[]{for(int m:{devf_variant::A_DEATH,devf_variant::A_DEMO_MASTER,devf_variant::A_DAMAGE,devf_variant::A_ATTACK,devf_variant::A_SOUND_WATCH}){Lab l;l.a().f.mode=m;need(!l.call().offered,"native priority");}});
 test("hit-before-mode-change-rejected",[]{Lab l;l.a().f.hit=true;need(!l.call().offered,"hit");});
 test("stale-culled-entry-rejected",[]{Lab l;l.a().f.seq=1;need(!l.call().offered,"stale");});
 test("unclassified-empty-registry-no-recipient",[]{Lab l;l.a().f.variant=V::None;need(!l.call().offered,"none");});
 test("nonfinite-position-or-leash-rejected",[]{for(int n=0;n<2;++n){Lab l;if(n)l.a().f.range=std::numeric_limits<float>::infinity();else l.a().f.pos.x=std::numeric_limits<float>::quiet_NaN();need(!l.call().offered,"finite");}});
 test("own-home-leash-not-caller-distance",[]{Lab l;l.a().f.home.x=5500;l.a().f.range=5000;need(!l.call().offered,"own leash");});
 test("leash-zero-native-unbounded-but-context-required",[]{Lab l;l.a().f.range=0;need(l.call().offered==1,"native zero");l.a().f.room=4;need(!l.valid(),"context retained");});
 test("leash-margin-exact-100",[]{Lab l;l.a().f.home.x=1000;l.a().f.range=1100;need(l.call().offered==1,"margin inclusive");});
 test("leash-margin-outside-and-native-return-flag",[]{Lab l;l.a().f.range=1099;need(!l.call().offered,"margin");Lab q;q.a().f.leash_out=true;need(!q.call().offered,"native return flag");});
 test("changed-destination-before-consumption-rejected",[]{Lab l;l.call();l.s().f.pos.x=-5000;need(!l.valid(),"updated leash");});
 test("offer-is-not-consumption-or-arrival",[]{Lab l;l.call();need(!l.a().order.consumed&&!l.a().order.arrived&&l.e.count("RECRUIT_ARRIVED")==0,"no false arrival");});
 test("first-emission-immediate-no-artificial-delay",[]{Lab l;need(l.call().issued&&l.a().order.active&&l.s().combat.shout_pending,"immediate");});
 test("consumption-authoritative-identity-check",[]{Lab l;l.call();bool checked=false;need(!r::order_valid(l.t,l.a(),10,8,[&](auto&){checked=true;return false;},l.e)&&checked&&!l.a().order.active,"identity");});
 test("generation-recycle-and-context-reset-no-inheritance",[]{Lab l;l.call();auto f=l.a().f;++f.gen;l.t.publish(1,f);need(!l.a().order.active,"generation");l.call();f=l.a().f;f.room=5;l.t.publish(1,f);need(!l.a().order.active,"room");});
 test("caller-disappears-generation-stale-cancel",[]{for(int n=0;n<3;++n){Lab l;l.call();if(n==0)l.s()={};if(n==1)++l.s().f.gen;if(n==2)l.s().f.seq=0;need(!l.valid(),"caller");}});
 test("caller-damage-hit-or-native-leash-return-cancels",[]{for(int n=0;n<3;++n){Lab l;l.call();if(n==0)l.s().f.mode=devf_variant::A_DAMAGE;if(n==1)l.s().f.hit=true;if(n==2)l.s().f.leash_out=true;need(!l.valid()&&!l.a().order.active,"caller native priority");}});
 test("recall-150-ticks-no-restart-existing-order",[]{Lab l;l.call();const auto first=l.a().order.offered_tick;for(unsigned i=0;i<149;++i){++l.s().ticks;need(!l.call().issued,"not per frame");}++l.s().ticks;need(l.call().issued&&l.a().order.offered_tick==first,"repeat not reset");});
 test("unlimited-rounds-no-logical-call-cap",[]{Lab l;l.call();for(unsigned i=0;i<1000;++i){l.s().ticks+=150;need(l.call().issued,"no call cap");}need(l.s().combat.shout_pending,"one shout eligibility");});
 test("two-Blue-first-valid-caller-stable",[]{Lab l;l.t.publish(2,frame(3,V::Blue,50));l.call();r::call(l.t,l.t.entries[2],true,10,8,l.e);need(l.a().order.caller==1,"retain incumbent");});
 test("recruited-Blue-no-automatic-domino",[]{Lab l;l.a().f.variant=V::Blue;l.call();need(!r::call(l.t,l.a(),true,10,8,l.e).issued,"incoming inhibits relay");});
 test("combat-reset-only-150-calm-without-order",[]{Lab l;l.call();l.s().combat.shout_used=true;l.s().combat.shout_pending=false;l.s().ticks+=149;r::call(l.t,l.s(),false,10,8,l.e);need(l.s().combat.active,"not early");++l.s().ticks;r::call(l.t,l.s(),false,10,8,l.e);need(!l.s().combat.active,"calm reset");l.call();need(l.s().combat.shout_pending,"new encounter");});
 test("caller-gates-new-FIND-blow-in-PRE-not-POST",[]{for(int old:{0,1}){Native n;n.mode=r::Find;n.state=4;n.c2=static_cast<std::int16_t>(old);auto lease=r::gate(true,n.mode,n.state,n.c2);n.step({});need(n.blows==0,"no offensive start");r::restore(lease,n.mode,n.state,n.c2);need(n.c2==0,"expected restore");}});
 test("no-callable-allies-native-solo-attacks",[]{Native n;n.mode=r::Find;n.state=4;r::gate(false,n.mode,n.state,n.c2);n.step({});need(n.blows==1,"native solo recovered");});
 test("in-progress-attack-and-foreign-timer-untouched",[]{std::int16_t c=17;auto q=r::gate(true,r::Find,4,c);need(!q.wrote&&c==17,"foreign native cooldown");c=0;q=r::gate(true,r::Attack,1,c);need(!q.wrote&&c==0,"attack priority");});
 test("timer-restore-not-overwrite-native-damage-state",[]{std::int16_t c=0;auto q=r::gate(true,r::Find,4,c);c=33;r::restore(q,devf_variant::A_DAMAGE,0,c);need(c==33,"damage owns");});
 test("real-group-readiness-not-dispatch-bonus",[]{Lab l;l.call();need(r::waiting(l.t,l.s(),10,8,l.e),"wait real ally");l.a().f.pos.x=650;need(!r::waiting(l.t,l.s(),10,8,l.e)&&l.e.count("BLUE_GROUP_READY")==1,"actual radius");});
 test("unconsumed-offer-grace-bounded",[]{Lab l;l.call();l.a().ticks+=3;need(!r::waiting(l.t,l.s(),10,8,l.e),"offer not executed");});
 test("max-wait-240-fallback-no-perpetual-freeze",[]{Lab l;l.call();l.a().order.consumed=true;l.s().ticks=l.s().combat.wait_start+240;need(!r::waiting(l.t,l.s(),10,8,l.e),"deadline");});
 test("no-progress-60-running-ticks-aborts",[]{Lab l;l.call();need(l.valid(),"valid");r::consumed(l.a(),l.e);l.a().f.mode=r::Sound;l.a().f.state=2;r::after(l.t,l.a(),l.e);l.a().ticks+=60;r::after(l.t,l.a(),l.e);need(!l.a().order.active&&l.a().retry_after>l.a().ticks,"blocked bounded");});
 test("initial-alert-is-not-falsely-no-progress",[]{Lab l;l.call();r::consumed(l.a(),l.e);l.a().f.mode=r::Sound;l.a().f.state=1;l.a().ticks+=89;r::after(l.t,l.a(),l.e);need(l.a().order.active,"native alert90");});
 test("progress-extension-absolute-240-cap",[]{Lab l;l.call();r::consumed(l.a(),l.e);l.a().f.mode=r::Sound;l.a().f.state=2;l.a().order.moving_since=1;l.a().ticks=l.a().order.started+180;l.a().order.progress_tick=l.a().ticks;l.a().order.checked=l.a().ticks;r::after(l.t,l.a(),l.e);need(l.a().order.extended,"progress extension");l.a().ticks+=60;l.a().order.progress_tick=l.a().ticks;r::after(l.t,l.a(),l.e);need(!l.a().order.active,"hard cap");});
 test("native-combat-recognized-not-reordered",[]{Lab l;l.call();r::consumed(l.a(),l.e);l.a().f.mode=r::Find;r::after(l.t,l.a(),l.e);need(l.a().order.active&&l.a().order.engaged&&l.e.count("RECRUIT_ENGAGED_NATIVE_MODE")==1,"engaged is not arrived and keeps bounded progress observation");});
 test("native-shout-WATCH3-CRY-once-not-death-simulation",[]{Native n;n.set(r::Watch,3);for(int i=0;i<100;++i)n.step({});need(n.cries==1&&n.mode==r::Find,"one native clip");});
 test("shout-preempted-before-native-readback-retries-safely",[]{Lab l;l.call();r::shout_requested(l.s());need(!l.s().combat.shout_used,"request not proof");need(r::shout_left_watch(l.s(),devf_variant::A_DAMAGE)==1&&l.s().combat.shout_pending,"defer priority");r::shout_requested(l.s());need(r::shout_observed(l.s(),r::Watch,4,true)&&l.s().combat.shout_used,"native readback");need(r::shout_left_watch(l.s(),r::Find)==2&&!l.s().combat.shout_pending,"no repeated cry");});
 test("shout-mode-with-wrong-animation-is-not-success",[]{Lab l;l.call();r::shout_requested(l.s());need(!r::shout_observed(l.s(),r::Watch,4,false)&&!l.s().combat.shout_used,"wrong clip");});
 test("source-port-control-no-call-no-approach",[]{const auto a=travel(false);need(a.moved==0&&a.arrived==0&&a.approach==0,"causal control");});
 test("source-port-command-moves-and-arrives",[]{const auto a=travel(true);need(a.moved>=350&&a.arrived==1&&a.approach==1,"actual position progress in source port");});
 test("source-port-1800-distance-bounded-movement",[]{const auto a=travel(true,false,false,false,1800);need(a.moved>500&&a.approach==1,"extended native walk");});
 test("source-port-wall-no-false-arrival-fallback",[]{const auto a=travel(true,true);need(a.moved==0&&a.arrived==0&&a.aborted>0&&!a.pending,"blocked model");});
 test("source-port-water-and-fall-priority",[]{for(int i=0;i<2;++i){const auto a=travel(true,false,i==0,i==1);need(a.arrived==0&&a.moved==0&&a.aborted>0,"terrain priority");}});
 test("source-port-native-detect-can-preempt-march-not-arrival",[]{const auto a=travel(true,false,false,false,1000,true);need(a.arrived==0,"no lying about early native find");});
 test("speed-group-1-2-3-exact-CP15-constants",[]{need(b::rate_for(1)==1&&b::rate_for(2)==1.15f&&b::rate_for(3)==1.30f,"rates");need(b::lockout_for(1)==20&&b::lockout_for(2)==17&&b::lockout_for(3)==15,"lockouts");});
 test("speed-Blue-only-no-Red-Black-acceleration",[]{need(b::applies_to(V::Blue)&&!b::applies_to(V::Red)&&!b::applies_to(V::Black),"variants");});
 test("speed-fresh-native-attack-rate-only",[]{b::AtkView v;v.mode=r::Attack;v.state=1;v.anim_b=true;v.frame=1;need(b::decide_rate(v,2,false)==b::RateAction::Apply,"fresh");v.play_speed=.8f;need(b::decide_rate(v,3,false)==b::RateAction::SkipForeignSpeed,"foreign");v.play_speed=1;v.frame=10;need(b::decide_rate(v,3,false)==b::RateAction::SkipNotFresh,"ongoing");});
 test("speed-lockout-20-to17-or15-preserved",[]{auto s=b::make_snap(false,true,300,r::Find,0,r::Attack,0,0);need(b::lockout_write(s,r::Find,20,2)==17&&b::lockout_write(s,r::Find,20,3)==15&&b::lockout_write(s,r::Find,19,3)==-1,"lockout exact");});
 test("bonus-650-250-not-1800-dispatch",[]{Lab l;l.call();need(!r::group(l.t,l.s(),10,8),"not radius1800");l.a().f.pos={650,250,0};need(r::group(l.t,l.s(),10,8),"650 inclusive");l.a().f.pos.y=250.1f;need(!r::group(l.t,l.s(),10,8),"Y");});
 test("group-level-confirm-three-solo-ticks",[]{b::Tracker s;b::Group g;g.companions=2;need(b::track(s,g).to==3,"up");g={};need(!b::track(s,g).changed&&!b::track(s,g).changed&&b::track(s,g).to==1,"down");});
 test("table-budget-value-only-64-memory-bounded",[]{need(r::kCapacity==64&&sizeof(r::Table)<32768,"table budget");});
 std::printf("CP16_LAB_PASS cases=%u evidence=POLICY_AND_NATIVE_SOURCE_PORT_NOT_GAMEPLAY table_bytes=%zu\n",cases,sizeof(r::Table));
}
