// Same policy compiled into the .dusk; native movement below is a SOURCE PORT, not engine or real terrain execution.
#include "../runtime/DEVF/CP16_FIX01_BLUE_RECRUITER_RANGE_DIAGNOSTICS/expansion/blue_recruit_policy.hpp"
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
struct Events{std::vector<std::string> reasons;std::vector<std::string> v;void operator()(const char*e,unsigned,unsigned,const r::Frame&,const char*why){v.emplace_back(e);reasons.emplace_back(why);}unsigned count(const char*e)const{unsigned n=0;for(auto&s:v)if(s==e)++n;return n;}};
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
void consume(Lab& l){need(l.call().offered==1,"offer fixture");need(l.valid(),"admit fixture");r::consumed(l.a(),l.e);l.a().f.mode=r::Sound;l.a().f.state=2;}
void reason(Lab& l,const char* expected){need(!l.valid(),"rejected or held");need(!l.a().order.active,"hard abort");need(!l.e.reasons.empty()&&l.e.reasons.back()==expected,expected);}
int main(){

 for(float x:{1799.9f,1800.f,1800.01f,2799.9f,2800.f,2800.01f})test(("radius-"+std::to_string(x)).c_str(),[=]{Lab l;l.a().f.pos.x=x;auto c=l.call();need(c.offered==(x<=2800?1u:0u),"new inclusive radius");});
 test("XZ-Euclidean-not-Manhattan",[]{Lab l;l.a().f.pos={2000,0,1900};l.a().f.home=l.a().f.pos;need(l.call().offered==1,"sqrt(2000^2+1900^2)<2800");});
 test("XZ-Euclidean-outside-circle",[]{Lab l;l.a().f.pos={2100,0,2100};l.a().f.home=l.a().f.pos;need(!l.call().offered,"not independent axes");});
 test("height-inclusive-plus-minus250-reject251",[]{for(float y:{-251.f,-250.f,250.f,251.f}){Lab l;l.a().f.pos.y=y;need(l.call().offered==(std::abs(y)<=250?1u:0u),"Y invariant");}});
 test("old-bonus-650-250-is-not2800",[]{Lab l;l.a().f.pos={650,250,0};need(r::group(l.t,l.s(),10,8),"650/250");l.a().f.pos.x=650.01f;need(!r::group(l.t,l.s(),10,8),"no expansion bonus");});
 test("native-classification-EOC-EOC2-normal-Black",[]{namespace v=devf_variant;const v::Plan ps[]={v::make_plan("E_OC",40,40,true,v::Registration::None,0),v::make_plan("E_OC2",220,220,true,v::Registration::None,0),v::make_plan("E_OC2",220,220,true,v::Registration::Normal,0),v::make_plan("E_OC2",220,220,true,v::Registration::Black,440)};for(const auto& p:ps){Lab l;l.a().f.variant=p.variant;need(p.variant!=V::None&&l.call().offered==1,"real unchanged classifier");}});
 test("2800-still-rejects-own-leash-at2200",[]{Lab l;l.a().f.pos.x=2200;l.a().f.home=l.a().f.pos;l.a().f.range=1500;need(l.call().skipped[unsigned(r::Skip::NativeLeash)]==1,"no range hack");});
 test("leash-zero-native-semantics-and100-margin",[]{Lab l;l.a().f.range=0;l.a().f.pos.x=2799;l.a().f.home=l.a().f.pos;need(l.call().offered==1,"zero native skips radius");Lab q;q.a().f.range=1100;need(q.call().offered==1,"margin100 inclusive");});
 test("first-contact-requires-own-direct-detection",[]{Lab l;l.s().f.mode=r::Find;need(!r::call(l.t,l.s(),false,10,8,l.e).issued&&!l.s().combat.active,"native FIND alone not first contact");});
 for(int mode:{r::Find,r::Attack,r::FindStay})test(("active-no-LOS-recall-mode-"+std::to_string(mode)).c_str(),[=]{Lab l;l.call();l.a().order={};l.s().f.mode=mode;l.s().f.state=0;l.s().ticks+=149;need(!r::call(l.t,l.s(),false,10,8,l.e).issued,"not early");++l.s().ticks;auto c=r::call(l.t,l.s(),false,10,8,l.e);need(c.issued&&c.offered==1&&std::string(c.origin)=="RECALL_ACTIVE_COMBAT_WITHOUT_CURRENT_LOS","active recall");need(l.e.count("BLUE_LINK_DETECTED")==1,"not new perception");});
 test("recall-source-initial-vs-current-direct",[]{Lab l;auto c=l.call();need(std::string(c.origin)=="INITIAL_DIRECT_DETECTION","initial");l.s().ticks+=150;c=l.call();need(std::string(c.origin)=="RECALL_CURRENT_DIRECT_DETECTION","direct repeat");});
 test("no-recall-during-damage-event-guard-or-terminal",[]{for(int m:{devf_variant::A_DAMAGE,devf_variant::A_DEATH,devf_variant::A_DEMO_MASTER,devf_variant::A_ATTACK}){Lab l;l.call();l.s().ticks+=150;l.s().f.mode=m;l.s().f.state=3;need(!r::call(l.t,l.s(),false,10,8,l.e).issued,"priority");}Lab l;l.call();l.s().ticks+=150;l.s().f.ready=false;need(!r::call(l.t,l.s(),true,10,8,l.e).issued,"unsafe event");});
 test("absent-Link-no-infinite-FIND-recalls",[]{Lab l;l.call();l.s().f.mode=r::Find;l.s().ticks+=150;need(!r::call(l.t,l.s(),false,10,8,l.e,false).issued&&!l.s().combat.active,"missing Link permits calm reset despite stale mode FIND");});
 test("calm150-reset-needs-new-direct-contact",[]{Lab l;l.call();l.s().ticks+=150;need(!r::call(l.t,l.s(),false,10,8,l.e).issued&&!l.s().combat.active,"reset");l.s().f.mode=r::Find;l.s().ticks+=150;need(!r::call(l.t,l.s(),false,10,8,l.e).issued,"cannot revive by fight alone");need(l.call().issued,"direct new contact");});
 test("incoming-Blue-no-cascade-even-with-LOS",[]{Lab l;l.a().f.variant=V::Blue;l.call();l.a().f.mode=r::Find;l.a().ticks+=150;need(!r::call(l.t,l.a(),true,10,8,l.e).issued&&!l.a().combat.active,"incoming alone never starts its combat");});
 test("new-ATTACK-recall-never-restarts-cry",[]{Lab l;l.call();r::shout_requested(l.s());need(r::shout_observed(l.s(),r::Watch,4,true),"observed");r::shout_left_watch(l.s(),r::Find);l.s().f.mode=r::Attack;l.s().f.state=1;l.s().ticks+=150;need(r::call(l.t,l.s(),false,10,8,l.e).issued&&l.s().combat.shout_used&&!l.s().combat.shout_pending,"no repeated gesture");});
 test("two-Blues-current-caller-stable",[]{Lab l;l.t.publish(2,frame(3,V::Blue,50));l.call();r::call(l.t,l.t.entries[2],true,10,8,l.e);need(l.a().order.caller==1,"no bounce");});
 test("busy-order-recall-does-not-restart-budget",[]{Lab l;consume(l);const auto old=l.a().order.started;l.s().ticks+=150;l.s().f.mode=r::Find;auto c=r::call(l.t,l.s(),false,10,8,l.e);need(!c.offered&&l.a().order.started==old,"consumed Sound is not idle and cannot reset");});
 test("every-primary-rejection-and-offer",[]{for(unsigned k=0;k<r::kReasons;++k){Lab l;const auto wanted=static_cast<r::Skip>(k);auto& f=l.a().f;
 switch(wanted){case r::Skip::InvalidSnapshot:f.gen=0;break;case r::Skip::NotClassified:f.variant=V::None;break;case r::Skip::DeadOrTerminal:f.health=0;break;case r::Skip::Stale:f.seq=0;break;case r::Skip::DifferentStage:std::memcpy(f.stage,"F_SP113",8);break;case r::Skip::DifferentRoom:f.room=4;break;case r::Skip::DifferentLayer:f.layer=5;break;case r::Skip::OutOfRadius:f.pos.x=2801;break;case r::Skip::HeightLimit:f.pos.y=251;break;case r::Skip::AlreadyFighting:f.mode=r::Find;break;case r::Skip::NotIdle:f.mode=devf_variant::A_DAMAGE;break;case r::Skip::Unsafe:f.hit=true;break;case r::Skip::NativeLeash:f.range=500;break;case r::Skip::Busy:l.a().order.active=true;break;case r::Skip::Backoff:l.a().retry_after=90;break;default:break;}
 need(r::candidate(l.s(),l.a(),10,8)==wanted,r::skip_text(wanted));auto c=l.call();need(c.skipped[k]==1&&c.examined_tracked==1&&c.self_slots==1&&c.empty_slots==62,"one primary cause, no fabricated census");}});
 test("primary-priority-multiple-conditions",[]{Lab l;l.a().f.seq=0;l.a().f.room=5;l.a().f.pos.x=3000;need(r::candidate(l.s(),l.a(),10,8)==r::Skip::Stale,"documented stale before context/radius");l.a().f.health=0;need(r::candidate(l.s(),l.a(),10,8)==r::Skip::DeadOrTerminal,"life precedes stale");});
 test("summary-counts-all64-and-bounded-selected-details",[]{Lab l;for(unsigned i=1;i<64;++i){auto f=frame(i+1,V::Red,i<32?1000.f:3000.f);l.t.publish(i,f);}auto c=l.call();unsigned sum=0;for(auto n:c.skipped)sum+=n;need(c.examined_tracked==63&&c.self_slots==1&&sum==63&&c.offered==31&&c.detail_count<=2,"congruent tracked summary");});
 test("empty-ledger-is-unknown-not-global-no-enemies",[]{r::Table t;Events e;auto& s=t.publish(0,frame(1,V::Blue,0));auto c=r::call(t,s,true,10,8,e);need(c.examined_tracked==0&&c.empty_slots==63&&c.self_slots==1,"empty not rejected recruits");});
 test("diagnostic-budget-per30-passes-and-global-caps",[]{r::Diagnostics d;for(unsigned i=0;i<16;++i)need(d.take(r::Diag::Summary),"summary budget");need(!d.take(r::Diag::Summary),"window bounded");for(unsigned i=0;i<8;++i)need(d.take(r::Diag::Detail),"detail budget");need(!d.take(r::Diag::Detail),"no spam");for(unsigned i=0;i<30;++i)d.advance();need(d.take(r::Diag::Summary)&&d.take(r::Diag::Detail),"next window");for(unsigned n=0;n<2000;++n){d.advance();d.take(r::Diag::Summary);d.take(r::Diag::Call);d.take(r::Diag::Detail);}need(d.all_summaries<=600&&d.all_calls<=600&&d.all_details<=240&&d.suppressed>0,"bounded no dynamic map");});
 test("precise-caller-missing-and-generation",[]{Lab l;consume(l);l.s()={};reason(l,"CALLER_NOT_TRACKED_OR_RELEASED");Lab q;consume(q);++q.s().f.gen;reason(q,"CALLER_GENERATION_MISMATCH");});
 test("precise-caller-dead-terminal-stale-context",[]{for(unsigned n=0;n<6;++n){Lab l;consume(l);const char* why="";switch(n){case 0:l.s().f.health=0;why="CALLER_DEAD_OR_TERMINAL_SPECIAL";break;case 1:l.s().f.mode=devf_variant::A_DEMO_MASTER;why="CALLER_DEAD_OR_TERMINAL_SPECIAL";break;case 2:l.s().f.seq=0;why="CALLER_SNAPSHOT_STALE";break;case 3:std::memcpy(l.s().f.stage,"F_SP113",8);why="CALLER_STAGE_CHANGED";break;case 4:l.s().f.room=4;why="CALLER_ROOM_CHANGED";break;case 5:l.s().f.layer=5;why="CALLER_LAYER_CHANGED";break;}reason(l,why);}});
 test("precise-unsafe-ground-event-transition-layout-hide-delete",[]{const unsigned bits[]={r::BadCanary,r::Hidden,r::Deleting,r::Event,r::Transition,r::NoGround};const char* names[]={"CALLER_NATIVE_CANARY_UNSAFE","CALLER_HIDDEN","CALLER_DELETING","CALLER_EVENT_ACTIVE","CALLER_STAGE_TRANSITION","CALLER_GROUND_NOT_READY"};for(unsigned i=0;i<6;++i){Lab l;consume(l);l.s().f.ready=false;l.s().f.unsafe_flags=static_cast<std::uint16_t>(bits[i]);reason(l,names[i]);}});
 test("precise-caller-and-receiver-leash",[]{Lab l;consume(l);l.s().f.leash_out=true;reason(l,"CALLER_NATIVE_LEASH_RETURN");Lab q;consume(q);q.a().f.range=500;reason(q,"RECEIVER_NATIVE_LEASH_REJECTED");});
 test("precise-late-authoritative-identity-failure",[]{Lab l;consume(l);l.a().ticks+=15;need(!r::order_valid(l.t,l.a(),10,8,[](auto&){return false;},l.e)&&!l.a().order.active&&l.e.reasons.back()=="CALLER_VERIFIED_IDENTITY_FAILED","not a guessed generation mismatch");});
 test("receiver-hit-damage-terminal-never-graced",[]{Lab l;consume(l);l.a().f.hit=true;reason(l,"RECEIVER_HIT_PRIORITY");Lab q;consume(q);q.a().f.mode=devf_variant::A_DAMAGE;reason(q,"RECEIVER_NATIVE_MODE_PRIORITY");Lab z;consume(z);z.a().f.health=0;reason(z,"RECEIVER_DEAD_OR_TERMINAL_SPECIAL");});
 test("unconsumed-order-does-not-admit-injured-caller",[]{Lab l;l.call();l.s().f.hit=true;reason(l,"CALLER_TRANSIENT_BEFORE_CONSUMPTION");});
 test("caller-normal-hit-hold10-then-exact-expiry",[]{Lab l;consume(l);l.s().f.hit=true;const auto start=l.a().order.started;for(unsigned n=0;n<10;++n){need(!l.valid()&&l.a().order.active&&l.a().order.caller_transient,"preserve consumed order no command");++l.a().ticks;}reason(l,"CALLER_TRANSIENT_GRACE_EXPIRED");need(start==1,"fixture start preserved");});
 test("native-damage0-3-and5-recovery-without-order-reset",[]{for(int state:{0,1,2,3,5}){Lab l;consume(l);const auto start=l.a().order.started,progress=l.a().order.progress_tick;l.s().f.mode=devf_variant::A_DAMAGE;l.s().f.state=state;need(!l.valid()&&l.a().order.active,"ordinary source path");l.a().ticks+=5;l.s().f.mode=r::Find;l.s().f.state=0;l.s().f.hit=false;need(l.valid()&&!l.a().order.caller_transient&&l.a().order.started==start&&l.a().order.progress_tick==progress,"resume without resetting deadlines");}});
 test("gale-big-damage-water-fall-no-grace",[]{for(int mode:{devf_variant::A_BIG_DAMAGE,devf_variant::A_WATER_DEATH,devf_variant::A_FALL}){Lab l;consume(l);l.s().f.mode=mode;need(!l.valid()&&!l.a().order.active,"special hard abort");}for(int st:{4,6}){Lab l;consume(l);l.s().f.mode=devf_variant::A_DAMAGE;l.s().f.state=st;need(!l.valid()&&!l.a().order.active,"gale not ordinary damage");}});
 test("grace-never-bypasses-stale-identity-context-leash",[]{for(int n=0;n<4;++n){Lab l;consume(l);l.s().f.hit=true;need(!l.valid()&&l.a().order.active,"grace begin");if(n==0)l.s().f.seq=0;if(n==1)++l.s().f.gen;if(n==2)l.s().f.room=5;if(n==3)l.a().f.range=500;need(!l.valid()&&!l.a().order.active,"hard guard during grace");}});
 test("grace-180-240-budget-not-paused",[]{Lab l;consume(l);l.a().f.state=1;l.a().ticks=l.a().order.started+180;l.s().f.hit=true;need(!l.valid()&&l.a().order.active,"grace entered");r::after(l.t,l.a(),l.e);need(!l.a().order.active&&l.e.reasons.back()=="APPROACH_DEADLINE_NATIVE_FALLBACK","180 still expires");Lab q;consume(q);q.a().order.extended=true;q.a().ticks=q.a().order.started+240;q.s().f.hit=true;q.valid();r::after(q.t,q.a(),q.e);need(!q.a().order.active,"240 absolute");});
 test("grace-no-progress60-still-applies",[]{Lab l;consume(l);r::after(l.t,l.a(),l.e);l.a().ticks+=60;l.s().f.hit=true;l.valid();r::after(l.t,l.a(),l.e);need(!l.a().order.active&&l.e.reasons.back()=="60_MOVING_TICKS_WITHOUT_50_UNITS_PROGRESS","no timeout waiver");});
 test("conditional-log-pattern-629-to630-no-missing-field-inference",[]{Lab l;l.s().f.id=629;l.s().f.gen=10;l.a().f.id=630;l.a().f.gen=11;consume(l);l.a().order.started=3729;l.a().ticks=3920;l.a().order.extended=true;l.a().f.mode=r::Find;l.a().f.state=4;l.s().f.mode=devf_variant::A_DAMAGE;l.s().f.state=5;need(!l.valid()&&l.a().order.active,"conditional normal damage preserves; old log caller cause UNKNOWN");l.s().f.mode=r::Attack;l.s().f.state=1;++l.a().ticks;need(l.valid(),"native attacking caller valid");});
 test("conditional-log-pattern-1007-to1009-two-orders",[]{for(unsigned tick:{1040u,1398u}){Lab l;l.s().f.id=1007;l.s().f.gen=32;l.a().f.id=1009;l.a().f.gen=34;consume(l);l.a().order.started=tick==1040?951:1251;l.a().ticks=tick;l.a().f.mode=tick==1040?r::Sound:r::Find;l.a().f.state=tick==1040?2:4;l.s().f.mode=devf_variant::A_DAMAGE;l.s().f.state=5;need(!l.valid()&&l.a().order.active,"nearby log DAMAGE5 pattern, exact old failing branch unknown");l.a().ticks+=2;l.s().f.mode=r::Find;l.s().f.state=0;need(l.valid(),"short recovery only");}});
 test("publish-context-and-ID-reuse-logs-before-clearing",[]{for(int n=0;n<5;++n){Lab l;consume(l);auto f=l.a().f;if(n==0)++f.id;if(n==1)++f.gen;if(n==2)std::memcpy(f.stage,"F_SP113",8);if(n==3)++f.room;if(n==4)++f.layer;r::publish_checked(l.t,1,f,l.e);need(!l.a().order.active&&l.e.count("RECRUIT_ABORTED")==1,"no silent inherited order");}});
 test("repeat-call64-all-eligible-no-three-recipient-cap",[]{Lab l;for(unsigned i=1;i<64;++i)l.t.publish(i,frame(i+1,i%3?V::Red:V::Black,1801.f+i));need(l.call().offered==63,"64 ledger entries");});
 test("backoff-and-no-foreign-override",[]{Lab l;l.a().retry_after=91;need(l.call().skipped[unsigned(r::Skip::Backoff)]==1,"backoff");Lab q;q.a().order.active=true;q.a().order.caller=99;need(q.call().skipped[unsigned(r::Skip::Busy)]==1&&q.a().order.caller==99,"existing owner");});
 test("source-port-command2200-vs-no-call",[]{auto a=travel(true,false,false,false,2200),b=travel(false,false,false,false,2200);need(a.moved>1000&&a.approach==1&&a.arrived==1&&b.moved==0,"native source port not game observation");});
 test("source-port-wall-water-and-pit-no-false-arrival",[]{auto a=travel(true,true,false,false,2200);need(a.arrived==0&&a.aborted>0&&!a.pending,"wall timeout");for(int i=0;i<2;++i){auto z=travel(true,false,i==0,i==1,2200);need(!z.arrived&&!z.moved,"native hazard priority");}});
 test("caller-motion-still-not-false-receiver-progress",[]{Lab l;consume(l);r::after(l.t,l.a(),l.e);const auto before=l.a().order.progress_tick;l.s().f.pos.x=200;l.a().order.goal=l.s().f.pos;l.a().ticks+=30;r::after(l.t,l.a(),l.e);need(l.a().order.progress_tick==before&&!l.a().order.moved,"same target both samples");});
 test("bonus-rates-and-lockout-unchanged",[]{need(b::rate_for(1)==1&&b::rate_for(2)==1.15f&&b::rate_for(3)==1.30f&&b::lockout_for(1)==20&&b::lockout_for(2)==17&&b::lockout_for(3)==15,"CP15 exact");need(!b::applies_to(V::Red)&&!b::applies_to(V::Black),"no other acceleration");});
 test("bounded-memory-and-no-actor-pointer-state",[]{need(sizeof(r::Table)<32768&&sizeof(r::Diagnostics)<64&&r::kCapacity==64,"fixed arrays only");});
 std::printf("CP16_FIX01_LAB_PASS cases=%u table_bytes=%zu diag_bytes=%zu evidence=SHARED_POLICY_AND_SOURCE_PORT_NOT_GAMEPLAY\n",cases,sizeof(r::Table),sizeof(r::Diagnostics));
}
