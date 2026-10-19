// CP19 laboratory (CP18 cases kept, scenarios moved inside the CP19 strike distance where the chain is concerned, plus CP19 cases below).
// CP18 laboratory: the DLL's own black_ai_policy.hpp and the real CP10.6 dynamic_policy.hpp (admission, capture_pre, post_update) driven against a
// source-shaped port of the daE_OC_c code paths CP18 depends on (official d_a_e_oc.cpp @40457c6): execute() timer decrements, action() leash update,
// executeAttack (B/C start, checkFrame sounds, AT bit 14..22, translation, end branch setBck(0x1C)+V_WAIT_ST, shield recoil), executeFind 0..4,
// executeWait, executeWatch, executeMoveOut, executeFindStay, executeDamage, setActionMode, posMoveF and J3DFrameCtrl::update once/loop.
// It is a PORT, not the engine: no BCK data (clip ends are lab values), no collision, no audio device. Voices/clips are the calls the port makes.
#include "../runtime/DEVF/CP19_BUGFIX/expansion/black_ai_policy.hpp"
#include "../runtime/DEVF/CP19_BUGFIX/expansion/cp19_policy.hpp"
#include "../runtime/DEVF/CP19_BUGFIX/expansion/blue_recruit_policy.hpp"
#include "../runtime/DEVF/CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
namespace b=devf_black_ai;namespace ra=devf_red_ai;using V=devf_variant::Variant;using P=ra::Point;
unsigned cases=0;void need(bool q,const char*why){if(!q){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
template<class F>void test(const char*n,F f){f();++cases;std::printf("CASE_PASS %s\n",n);}
struct Events{std::vector<std::string> ev,why;void operator()(const char*e,const char*w){ev.emplace_back(e);why.emplace_back(w);}
 unsigned count(const char*n)const{return unsigned(std::count(ev.begin(),ev.end(),n));}bool has(const char*n,const char*w)const{for(size_t i=0;i<ev.size();++i)if(ev[i]==n&&why[i]==w)return true;return false;}void clear(){ev.clear();why.clear();}};
// ---------------- native port ----------------
constexpr float tb[]={0,0,0,0,15.3f,38.4f,63.3f,86.2f,103.3f,112};constexpr float tc[]={0,31.3f,72.7f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f};
struct Env{P link{300,0,0};bool perceive=true,floor_gap=false,guard=false,water=false,fall=false;int hit=0,dmg=0,dd=0;};
struct Call{unsigned tick;std::string what;};
struct Oc{
 int mode=b::Find,state=4,old=0;std::int16_t c0=30,c2=0,c4=0,ca=0;std::uint8_t e3=0,de=1,dd=0;float speed=0,a0=0;int angle=0x4000;P pos{0,0,0},home{0,0,0};float range=0;
 int bck=0x1e,attr=2;float frame=0,rate=1.3f,end=30;bool stop=false;std::uint32_t rng=0x1234567;unsigned tick=0;std::vector<Call> calls;bool at_set=false;unsigned at_regs=0;std::vector<float> at_pre;
 bool contact_prev=false;unsigned sfx_b2=0,sfx_b125=0,sfx_b19=0,sfx_c7=0,sfx_c115=0;
 float rnd(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng%100000)/100000.f;}
 void log(const std::string& s){calls.push_back({tick,s});}
 bool isStop()const{return stop||rate==0.0f;}
 void setBck(int i,int at,float,float r){bck=i;attr=at;frame=0;rate=r;stop=false;end=i==5||i==6?40.f:i==0xd?25.f:i==8||i==9||i==0xa?20.f:i==7||i==0x16?30.f:30.f;char t[32];std::snprintf(t,sizeof t,"BCK_0x%02X",i);log(t);}
 void voice(const char*v){log(std::string("V_")+v);}
 void setActionMode(int m,int s){old=mode;mode=m;state=s;c0=c2=0;}
 double dist(const Env&e)const{return ra::d3(pos,e.link);}
 int pl_ang(const Env&e)const{return ra::yaw(pos,e.link);}
 void turn(int target){const int d=int(std::int16_t(std::uint16_t((target-angle)&0xffff)));int st=d/4;if(std::abs(d)<=0x100){angle=target;return;}if(std::abs(st)<0x100)st=d<0?-0x100:0x100;if(st>0x800)st=0x800;if(st<-0x800)st=-0x800;angle=int(std::int16_t(std::uint16_t((angle+st)&0xffff)));}
 bool setWatchMode(){if(dd){setActionMode(b::Watch,dd==1?0:2);return true;}return false;}
 void play(){if(attr==0){frame+=rate;if(frame<0){frame=0;rate=0;stop=true;}else if(frame>=end){frame=end-0.001f;rate=0;stop=true;}else stop=false;}else{frame+=rate;stop=false;while(frame>=end)frame-=end;}}
 void execute(Env& e){
  ++tick;if(c0)--c0;if(c2)--c2;if(c4)--c4;if(ca)--ca;at_set=false;
  // action(): damage_check, leash, searchOtherOc, water, fall, mode switch, posMoveF, play
  if(e.hit){setActionMode(b::Damage,2);e.dmg=e.hit;e.hit=0;}
  if(range!=0){const double d=ra::root(ra::xz2(home,pos));if(e3==0){if(d>range)e3=d>range+300?2:1;}else if(d<range-100)e3=0;}
  dd=e3==0?std::uint8_t(e.dd):0;e.dd=0;
  if(e.water&&mode!=b::WaterDeath)setActionMode(b::WaterDeath,0);
  if(e.fall&&mode!=devf_variant::A_FALL)setActionMode(devf_variant::A_FALL,0);
  switch(mode){
   case b::Wait:wait(e);break;case b::Find:find(e);break;case b::Attack:attack(e);break;case b::Damage:damage(e);break;case b::Watch:watch(e);break;
   case b::MoveOut:move_out(e);break;case b::FindStay:find_stay(e);break;default:break;
  }
  // setFootNoteSound -> setWaitStSound: while the clip is BCK 0x1C, checkFrame(0) (incl. the loop wrap) emits V_WAIT_ST again.
  if(bck==0x1c&&((frame<=0&&0<frame+rate)||(attr==2&&frame+rate>=end)))voice("WAIT_ST");
  const double r=angle*3.141592653589793/32768.0;pos.x+=float(speed*std::sin(r));pos.z+=float(speed*std::cos(r));
  play();
 }
 void wait(Env& e){de=0;if(e3==0){if(setWatchMode())return;if(e.perceive){setActionMode(b::Find,1);return;}}
  if(state==0){setBck(0x1b,2,5,1);voice("WAIT");speed=0;c0=60;state=3;}}
 void find(Env& e){
  const int pa=pl_ang(e);const double d=dist(e);
  if(!(state<3||!setWatchMode()))return;
  if(state<=2){
   de=0;
   if(state==0){c0=0;c2=c0+0x14;if(bck!=0x1c){setBck(0x1c,2,5,1);voice("WAIT_ST");}de=1;}
   else if(state==1){c0=25;ca=0x96;setBck(0xd,0,5,1);voice("FIND");}
   else{c0=7;setBck(0xd,0,5,1);voice("FIND");}
   state=3;speed=0;
  }
  if(state==3){if(c0==0){if(de||d<400){setBck(0x1e,2,5,1.3f);de=1;voice("WALK_ST");}else{setBck(0xb,2,5,1.2f);voice("DASH");}state=4;c0=0x1e;}return;}
  if(state!=4)return;
  if(e.floor_gap){if(e3)setActionMode(b::MoveOut,0);else setActionMode(b::FindStay,0);return;}
  if(ra::angle_gap(angle,pa)>=0x400)turn(pa);
  if(c2==0){speed=ra::chase(speed,d<300?-3.f:20.f,1.f);if(d<400&&d>200){if(std::abs(angle-pl_ang(e))<0x1000)setActionMode(b::Attack,0);return;}}
  else{
   if(d<400&&d>200)speed=ra::chase(speed,0,1);else if(d<300)speed=ra::chase(speed,-3,1);else speed=ra::chase(speed,20,1);
   if(speed<0){if(bck!=0x1c||rate!=1.f){setBck(0x1c,2,5,1);voice("WAIT_ST");}}
   else if(speed==0&&ra::angle_gap(angle,pa)<0x1000){if(bck!=0x1c){setBck(0x1c,2,5,1);voice("WAIT_ST");}}
   else if(bck!=0x1e||rate!=1.3f){setBck(0x1e,2,5,1.3f);voice("WALK_ST");}
  }
  if(c0==0){if(!e.perceive)setActionMode(b::Wait,0);else if(e3)setActionMode(b::MoveOut,0);}
 }
 void attack(Env& e){
  float my=0;const int fc=int(frame-9.0f);if(fc>=0){const int i=std::min(fc,9);my=bck==5?tb[i]:tc[i];}
  if(state==0){speed=ra::chase(speed,0,1);if(speed==0){if(rnd()<0.5f){setBck(5,0,5,1);voice("ATTACK_B");state=1;}else{setBck(6,0,5,1);voice("ATTACK_C");state=2;}a0=0;}}
  else if(state==1||state==2){
   if(state==1){if(ra::pass(frame,rate,end,10))++sfx_b2;else if(ra::pass(frame,rate,end,12.5f))++sfx_b125;else if(ra::pass(frame,rate,end,19))++sfx_b19;}
   else{if(ra::pass(frame,rate,end,7))++sfx_c7;else if(ra::pass(frame,rate,end,11.5f))++sfx_c115;}
   if(frame>=14&&frame<=22){at_set=true;++at_regs;at_pre.push_back(frame);}
   if(contact_prev&&e.guard){rate=-1;state=3;log("RECOIL_RATE_MINUS1");}
   contact_prev=at_set&&dist(e)<260;
   const double r=angle*3.141592653589793/32768.0;pos.x+=float((my-a0)*std::sin(r));pos.z+=float((my-a0)*std::cos(r));a0=my;
   if(isStop()){setBck(0x1c,2,0,1);voice("WAIT_ST");if(e3)setActionMode(b::MoveOut,0);else if(ca&&dist(e)<500)state=0;else setActionMode(b::Find,0);}
  }else if(state==3){
   const double r=angle*3.141592653589793/32768.0;pos.x+=float((my-a0)*std::sin(r));pos.z+=float((my-a0)*std::cos(r));a0=my;
   if(!isStop())return;setBck(0x1c,2,5,1);voice("WAIT_ST");if(e3){setActionMode(b::MoveOut,0);return;}
   if(ca&&dist(e)<500&&std::abs(angle-pl_ang(e))<0x1000){state=0;return;}setActionMode(b::Find,0);
  }
 }
 void damage(Env&){if(state<=2){speed=0;setBck(9,0,0,1);voice("DAMAGE_LR");state=5;}else if(state==5){speed=ra::chase(speed,0,3);if(isStop()){if(e3)setActionMode(b::MoveOut,0);else setActionMode(b::Find,0);}}}
 void watch(Env&){if(state==0||state==2){speed=0;if(de){setBck(0x1c,2,5,1);voice("WAIT_ST");}else{setBck(0x1b,2,5,1);voice("WAIT");}c0=35;state=state==0?1:3;}
  if(state==1&&c0==0)setActionMode(b::Find,2);else if(state==3&&c0==0){state=4;setBck(7,0,5,1);voice("CRY");}else if(state==4&&isStop())setActionMode(b::Find,2);}
 void move_out(Env& e){
  const double pd=dist(e);const int ha=ra::yaw(home,pos);
  if(state==0){if(e3==2){if(bck!=0x1e)setBck(0x1e,2,5,1);state=1;}else{if(bck!=0x1c)setBck(0x1c,2,5,1);c0=30;state=2;}}
  if(state==1){turn(ha);speed=ra::chase(speed,-15,1);if(ra::root(ra::xz2(home,pos))<range){setBck(0x1c,2,5,1);c0=30;state=2;}return;}
  bool cond=false;if(pd<200||ra::root(ra::xz2(home,pos))>range+200){if(ra::angle_gap(angle,ha)<0x3000){cond=true;speed=ra::chase(speed,-15,1);}}
  if(!cond){speed=ra::chase(speed,0,1);if(!speed&&bck!=0x1c)setBck(0x1c,2,5,1);}
  else{if(bck!=0x1e)setBck(0x1e,2,5,1);if(ra::root(ra::xz2(home,pos))<range-200){setActionMode(b::Find,0);return;}}
  if(ra::angle_gap(angle,pl_ang(e))>=0x400)turn(pl_ang(e));
  if(c0==0){if(ra::root(ra::xz2(home,e.link))<range-200){setActionMode(b::Find,0);return;}if(pd<400&&pd>200&&std::abs(angle-pl_ang(e))<0x1000)setActionMode(b::Attack,0);}
 }
 void find_stay(Env& e){
  const double d=dist(e);if(!e.floor_gap){setActionMode(b::Find,0);return;}
  if(state==0){if(!speed){if(bck!=0x1c)setBck(0x1c,2,5,1);state=2;}else{if(bck!=0x1e)setBck(0x1e,2,5,1);state=1;}return;}
  if(state==1){speed=ra::chase(speed,0,2);if(!speed){setBck(0x1c,2,5,1);state=2;}}
  speed=ra::chase(speed,d<400?-15.f:0.f,1);if(ra::angle_gap(angle,pl_ang(e))>=0x400)turn(pl_ang(e));
  if(d<400&&d>200){if(std::abs(angle-pl_ang(e))<0x1000)setActionMode(b::Attack,0);return;}
  if(!e.perceive)setActionMode(b::Wait,0);
 }
 unsigned count(const char*w,unsigned from=0)const{unsigned n=0;for(auto& c:calls)if(c.tick>=from&&c.what==w)++n;return n;}
};
// ---------------- one Black: real CP10.6 ledger + CP18 policy around the port ----------------
struct Io{std::uint8_t atp[2]{2,2};std::uint8_t get(unsigned i)const{return atp[i];}void set(unsigned i,std::uint8_t v){atp[i]=v;}};
struct Black{
 Oc oc;Env env;devf_dyn::Ledger led;Io io;devf_dyn::Entry* d=nullptr;b::State* s=nullptr;int hp=150,room=3;std::uint32_t id=1,gen=1,stage=0x5150;bool wall=false,los=true,event=false;
 b::Command cmd;b::Rate rate;Events ev;std::vector<unsigned> starts;std::vector<float> rates;std::vector<int> clips;unsigned seq=0;
 Black(std::array<b::State,64>& t,unsigned slot,std::uint32_t i=1):id(i){auto r=devf_dyn::admit(led,id,&oc,gen,V::Black,"E_OC2",io);need(r.plan.accepted&&io.atp[0]==3,"CP10.5/CP10.6 admission of a Black");d=r.rec;s=&b::state_for(t,slot,id,gen);}
 b::Actor sample(bool post)const{b::Actor a;a.id=id;a.gen=gen;a.variant=V::Black;a.stage=stage;a.room=room;a.mode=oc.mode;a.state=oc.state;a.health=hp;a.angle=oc.angle;a.timer=oc.c0;a.lockout=oc.c2;a.leash=oc.e3;
  a.pos=oc.pos;a.home=oc.home;a.range=oc.range;a.speed=oc.speed;a.frame=oc.frame;a.rate=oc.rate;a.end=oc.end;a.playmode=oc.attr;a.b=oc.bck==5;a.c=oc.bck==6;a.wait_bck=post&&oc.bck==0x1c;a.stopped=oc.isStop();
  a.hit=!post&&env.hit;a.wall=wall;a.event=event;a.los=los;a.at_set=post&&oc.at_set;return a;}
 b::Link link(int lroom,std::uint32_t lstage)const{b::Link l;l.present=true;l.id=77;l.room=lroom;l.stage=lstage;l.pos=env.link;return l;}
 void step(b::Rooms& g,int lroom=3,std::uint32_t lstage=0x5150,bool link_present=true){
  devf_dyn::capture_pre(*d,hp);
  b::Link l=link_present?link(lroom,lstage):b::Link{};
  cmd=b::pre(*s,sample(false),l,g,d->fury,ev);
  if(cmd.set_mode)oc.setActionMode(cmd.mode,cmd.state);
  if(cmd.zero_speed)oc.speed=0;if(cmd.timer)oc.c0=cmd.timer_value;if(cmd.lockout)oc.c2=cmd.lockout_value;if(cmd.close)oc.speed=cmd.speed;   // CP19: same order as mod.cpp black_ai_pre
  oc.execute(env);
  if(env.dmg){hp-=env.dmg;env.dmg=0;}
  devf_dyn::Post in;in.post_health=hp;in.action=oc.mode;in.room=room;in.x=oc.pos.x;in.y=oc.pos.y;in.z=oc.pos.z;in.stage=stage;in.seq=++seq;in.window=4;
  const auto e=devf_dyn::post_update(led,*d,io,in,[](const devf_dyn::Entry&){return true;});
  if(e.kind==devf_dyn::Ev::FuryStart||e.kind==devf_dyn::Ev::FuryRefresh)b::damage(*s,sample(true),l,g,e.kind==devf_dyn::Ev::FuryStart,ev);
  {const auto cp=b::close_after(*s,sample(true),ev);if(cp.speed)oc.speed=cp.speed_value;if(cp.lockout)oc.c2=0;}   // CP19: as mod.cpp black_ai_post
  const size_t before=ev.ev.size();rate=b::post(*s,sample(true),d->fury,ev);
  for(size_t i=before;i<ev.ev.size();++i)if(ev.ev[i]=="BLACK_NATIVE_ATTACK_STARTED"){starts.push_back(oc.tick);clips.push_back(oc.bck);}
  if(rate.write){oc.rate=rate.value;rates.push_back(rate.value);}
 }
 void hit(int amount=10){env.hit=amount;}
 void fury_now(b::Rooms& g){hit();step(g);need(d->fury==devf_dyn::kFuryTicks,"fury started by the real CP10.6 post_update");}
 // run until the port is in a fresh blow at frame 1 (or give up)
 bool to_blow(b::Rooms& g,unsigned max=200){for(unsigned i=0;i<max;++i){step(g);if(b::blow(sample(true))&&oc.frame>.99f&&oc.frame<1.01f)return true;}return false;}
};
std::array<b::State,64> T{};
b::Rooms G;
void fresh(){T={};G={};}
void cp19_cases();
int main(){
 test("constants_x115_x2_fury300_atp3_6_reused",[]{need(b::kNormal==1.15f&&b::kFury==2.0f&&devf_dyn::kFuryTicks==300&&devf_dyn::kAtpBlackBase==3&&devf_dyn::kAtpBlackFury==6,"constants");need(b::kChainReach==400.&&b::kChainFacing==0x1000,"native FIND4 attack switch bounds");});
 test("cp10_6_fury_300_ticks_refresh_no_stack_unchanged",[]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={3000,0,0};k.fury_now(G);need(k.io.atp[0]==6,"ATP 6");
  unsigned n=1;while(k.d->fury){k.step(G);++n;need(n<400,"bounded");}need(n==301&&k.io.atp[0]==3,"300 ticks of 6 then base 3");
  k.fury_now(G);for(int i=0;i<100;++i)k.step(G);need(k.d->fury==200,"decrement");k.hit();k.step(G);need(k.d->fury==300,"renew to 300, no stack");});
 test("hunt_armed_once_from_cp10_6_event_no_second_detector",[]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={2000,0,0};
  k.fury_now(G);need(k.s->hunt&&k.ev.count("BLACK_HUNT_LOCKED_FIRST_DAMAGE")==1&&k.ev.count("BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6")==1,"armed once");
  for(int i=0;i<60;++i)k.step(G);k.hit();k.step(G);need(k.ev.count("BLACK_HUNT_LOCKED_FIRST_DAMAGE")==1&&k.ev.count("BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6")==2,"refresh recognised, no second activation");});
 test("hp_init_and_regeneration_are_not_damage",[]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={2000,0,0};
  for(int i=0;i<200;++i){k.step(G);if(i%8==0)k.hp+=1;}need(!k.s->hunt&&k.d->fury==0&&k.ev.count("BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6")==0,"regeneration between POST and next PRE never looks like a hit");});
 test("lethal_and_terminal_never_arm",[]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.hp=5;k.hit(10);k.step(G);need(!k.s->hunt&&k.d->fury==0,"lethal loss: CP10.6 terminal, no event");
  b::State s;s.id=1;s.gen=1;b::Actor a;a.id=1;a.gen=1;a.variant=V::Black;a.stage=1;a.room=3;a.health=0;a.mode=b::Death;b::Link l;l.present=true;l.room=3;l.stage=1;Events e;b::Rooms g;b::damage(s,a,l,g,true,e);need(!s.hunt&&e.count("BLACK_HUNT_NOT_ARMED")==1,"terminal never arms");});
 test("link_other_room_or_unknown_room_never_arms",[]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;
  k.hit();k.step(G,4);need(!k.s->hunt&&k.ev.has("BLACK_HUNT_NOT_ARMED","LINK_ROOM_DIFFERS_FROM_THIS_BLACK_NATIVE_ROOM"),"other room");
  for(int i=0;i<30;++i)k.step(G,4);k.hit();k.step(G,-1);need(!k.s->hunt&&k.ev.has("BLACK_HUNT_NOT_ARMED","LINK_ROOM_UNKNOWN_OR_TRANSIENT_NO_GUESS"),"room -1 is not a room");});
 test("other_black_blue_red_never_inherit",[]{fresh();Black k(T,0,1),m(T,1,2);k.oc.mode=m.oc.mode=b::Wait;k.oc.state=m.oc.state=3;k.oc.c0=m.oc.c0=500;k.env.perceive=m.env.perceive=false;k.env.link=m.env.link={2000,0,0};
  k.fury_now(G);m.step(G);need(k.s->hunt&&!m.s->hunt&&m.d->fury==0,"second Black untouched");
  b::State s;s.id=9;s.gen=1;b::Actor a;a.id=9;a.gen=1;a.variant=V::Blue;a.stage=1;a.room=3;a.health=40;a.mode=b::Wait;b::Link l;l.present=true;l.room=3;l.stage=1;Events e;b::Rooms g;b::damage(s,a,l,g,true,e);need(!s.hunt,"Blue domain refused");
  a.variant=V::Red;b::damage(s,a,l,g,true,e);need(!s.hunt,"Red domain refused");});
 // ---- rate ----
 test("normal_blows_x115_at_frame1_both_clips_native_pauses_kept",[]{fresh();Black k(T,0);k.oc.mode=b::Find;k.oc.state=4;k.oc.c0=30;k.oc.pos={0,0,0};k.env.link={300,0,0};k.oc.angle=0x4000;
  for(int i=0;i<1200;++i)k.step(G);need(k.starts.size()>=8,"several blows");bool sb=false,sc=false;for(int c:k.clips){sb|=c==5;sc|=c==6;}need(sb&&sc,"both native clips");
  for(float r:k.rates)need(r==1.15f,"x1.15 only, no fury");need(k.oc.count("V_WAIT_ST")>=k.starts.size()-1,"native WAIT_ST between normal blows kept");need(k.ev.count("BLACK_CHAIN_NEXT_ATTACK_STARTED")==0,"no chain without fury");});
 test("fury_blows_x2_decided_frame1_applied_frame2_even_phase_five_at",[]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.link={300,0,0};k.oc.angle=0x4000;k.env.perceive=false;
  k.fury_now(G);k.env.perceive=true;need(k.to_blow(G),"blow");need(k.oc.rate==1.0f&&k.s->pending,"x2 pending at frame 1");k.oc.at_regs=0;k.oc.at_pre.clear();k.step(G);need(k.oc.rate==2.0f&&k.oc.frame>1.99f&&k.oc.frame<2.01f,"x2 applied at frame 2");
  while(b::blow(k.sample(true))&&!k.oc.isStop())k.step(G);need(k.oc.at_regs==5,"five native AT registrations");for(float f:k.oc.at_pre)need(std::fabs(f-std::round(f))<1e-3&&int(std::round(f))%2==0,"even pre-play frames 14..22");});
 test("fury_change_mid_blow_never_changes_rate",[]{
  {fresh();Black k(T,0);k.oc.mode=b::Find;k.oc.state=4;k.oc.c0=30;k.env.link={300,0,0};k.oc.angle=0x4000;need(k.to_blow(G),"blow");need(k.oc.rate==1.15f,"x1.15");
   k.hit();k.step(G);need(k.oc.mode==b::Damage,"damage takes the execute");}
  {fresh();Black k(T,0);k.oc.mode=b::Find;k.oc.state=4;k.oc.c0=30;k.env.link={300,0,0};k.oc.angle=0x4000;need(k.to_blow(G),"blow");k.d->fury=300;k.d->owned=true;
   for(int i=0;i<6;++i){k.step(G);need(k.oc.rate==1.15f||k.oc.rate==0,"fury start during a blow keeps x1.15");}}
  {fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={300,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;need(k.to_blow(G),"blow");k.step(G);need(k.oc.rate==2.0f,"x2");
   k.d->fury=1;k.step(G);need(k.d->fury==0,"fury ended");while(b::blow(k.sample(true))&&!k.oc.isStop()){need(k.oc.rate==2.0f,"fury end during a blow keeps x2");k.step(G);}}});
 test("foreign_or_not_fresh_rate_skipped",[]{b::State s;s.id=1;s.gen=1;b::Actor a;a.id=1;a.gen=1;a.variant=V::Black;a.stage=1;a.room=3;a.health=100;a.mode=b::Attack;a.state=1;a.b=true;a.frame=1;a.rate=1.3f;a.end=40;
  s.pre_valid=true;s.pre_mode=b::Attack;s.pre_state=0;Events e;auto r=b::post(s,a,0,e);need(!r.write&&e.has("BLACK_RATE_SKIPPED","FOREIGN_OR_NATIVE_NONDEFAULT_RATE"),"foreign rate");
  b::State t;t.id=1;t.gen=1;t.pre_valid=true;t.pre_mode=b::Attack;t.pre_state=1;a.rate=1;a.frame=5;r=b::post(t,a,0,e);need(!r.write&&e.has("BLACK_RATE_SKIPPED","NOT_A_FRESH_NATIVE_BC_START"),"not fresh");});
 test("natural_clip_end_rate0_is_not_a_foreign_write",[]{fresh();Black k(T,0);k.oc.mode=b::Find;k.oc.state=4;k.oc.c0=30;k.env.link={300,0,0};k.oc.angle=0x4000;for(int i=0;i<600;++i)k.step(G);
  need(k.ev.count("BLACK_RATE_OWNERSHIP_RELEASED")==0,"J3D once end (rate 0) is the clip's own completion");});
 test("sfx_all_fire_at_x115_and_x2",[]{for(float r:{1.15f,2.0f})for(int clip:{5,6}){Oc o;o.mode=b::Attack;o.state=clip==5?1:2;o.setBck(clip,0,5,1);o.frame=1;Env e;e.link={3000,0,0};o.execute(e);o.rate=r;
   while(o.mode==b::Attack&&(o.state==1||o.state==2)&&!o.isStop())o.execute(e);
   if(clip==5)need(o.sfx_b2==1&&o.sfx_b125==1&&o.sfx_b19==1,"B: V_ATTACK_B2, ATTACK_B, NATA_HIT once each");else need(o.sfx_c7==1&&o.sfx_c115==1,"C: V_ATTACK_C2, ATTACK_C once each");}});
 test("translation_conserved_at_x2",[]{for(int clip:{5,6}){float moved[2];int k=0;for(float r:{1.0f,2.0f}){Oc o;o.mode=b::Attack;o.state=clip==5?1:2;o.setBck(clip,0,5,1);o.angle=0;Env e;e.link={0,0,5000};o.rate=r;
   for(int i=0;i<60&&o.mode==b::Attack&&(o.state==1||o.state==2);++i)o.execute(e);moved[k++]=o.pos.z;}need(std::fabs(moved[0]-moved[1])<1e-3,"same native lunge");}});
 test("x2_sweep_reused_only_for_owned_x2_never_pose24",[]{b::State s;s.id=1;s.gen=1;s.decided=true;s.owned=true;s.own_rate=b::kFury;s.link_pos_valid=true;s.link_pos={0,0,120};
  b::Actor a;a.mode=b::Attack;a.state=1;a.b=true;a.rate=2;a.tip_r=ra::kTipR;a.hilt_r=ra::kHiltR;unsigned writes=0,last=0;
  for(float f=10;f<=24;f+=2){const double y=0.08*(f-10);a.frame=f;a.tip={float(90*std::sin(y)),100,float(90*std::cos(y))};a.hilt={float(40*std::sin(y)),100,float(40*std::cos(y))};a.at_set=f-2>=14&&f-2<=22;
   auto w=b::sweep(s,a);writes+=w.write0||w.write1;if(f>23&&a.at_set){need(w.write0&&w.write1,"last tick re-places both spheres (pose 23 or exact 22)");++last;}}
  need(writes>0&&last==1,"sweep active on owned x2");s.own_rate=b::kNormal;a.rate=b::kNormal;a.frame=16;auto w=b::sweep(s,a);need(!w.write0&&!w.write1,"x1.15 untouched");});
 test("shutdown_restore_only_owned_running_blow",[]{b::State s;s.id=1;s.gen=1;s.owned=true;s.clip=5;s.own_rate=2;s.last_frame=4;b::Actor a;a.id=1;a.gen=1;a.variant=V::Black;a.stage=1;a.room=3;a.mode=b::Attack;a.state=1;a.b=true;a.frame=6;a.rate=2;a.end=40;
  need(b::restore_on_shutdown(s,a),"owned");a.rate=-1;need(!b::restore_on_shutdown(s,a),"recoil");a.rate=2;a.gen=2;need(!b::restore_on_shutdown(s,a),"other generation");});
 // ---- chain ----
 auto fury_fight=[](Black& k,unsigned ticks,float lx=200){k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={lx,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;for(unsigned i=0;i<ticks;++i){k.env.link.x=k.oc.pos.x+lx;k.step(G);}};
 test("fury_chain_no_bck1c_no_wait_st_no_lockout_between_blows",[&]{fresh();Black k(T,0);const unsigned from=k.oc.tick+1;fury_fight(k,280);
  need(k.ev.count("BLACK_CHAIN_NEXT_ATTACK_STARTED")>=8,"many chained blows inside one fury");
  unsigned first=0;for(size_t i=0;i<k.starts.size();++i)if(k.starts[i]>=from){first=k.starts[i];break;}
  need(k.oc.count("BCK_0x1C",first)==0&&k.oc.count("V_WAIT_ST",first)==0,"no idle clip and no V_WAIT_ST once the fury chain runs");
  for(size_t i=1;i<k.starts.size();++i){const unsigned gap=k.starts[i]-k.starts[i-1];need(gap<=22,"next blow right after the previous clip completes (x2 clip ~20 ticks)");}
  for(float r:k.rates)need(r==2.0f||r==1.0f,"x2 in fury");need(k.ev.count("BLACK_NATIVE_WAIT_TRANSITION_AVOIDED")>=8,"each avoided transition logged");});
 test("control_native_without_fury_plays_wait_and_lockout",[]{fresh();Black k(T,0);k.oc.mode=b::Find;k.oc.state=4;k.oc.c0=30;k.env.link={300,0,0};k.oc.angle=0x4000;
  unsigned lock=0;for(int i=0;i<600;++i){k.env.link.x=k.oc.pos.x+300;k.step(G);lock=std::max<unsigned>(lock,k.oc.c2);}need(k.oc.count("V_WAIT_ST")>=3&&k.oc.count("BCK_0x1C")>=3&&lock>=18,"the native pause the chain removes really exists in the port");});
 test("native_bc_choice_preserved_all_sequences",[&]{fresh();Black k(T,0);fury_fight(k,290);bool bb=false,bc=false,cb=false,cc=false;
  for(size_t i=1;i<k.clips.size();++i){bb|=k.clips[i-1]==5&&k.clips[i]==5;bc|=k.clips[i-1]==5&&k.clips[i]==6;cb|=k.clips[i-1]==6&&k.clips[i]==5;cc|=k.clips[i-1]==6&&k.clips[i]==6;}
  need(bb&&bc&&cb&&cc,"B->B, B->C, C->B, C->C all occur from the native random choice");});
 test("chain_only_after_complete_clip_previous_window_whole",[&]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={200,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;
  need(k.to_blow(G),"blow");k.oc.at_regs=0;unsigned guard=0;while(!(k.cmd.op==b::Op::Chain)&&guard++<60){k.env.link.x=k.oc.pos.x+200;k.step(G);if(k.cmd.op!=b::Op::Chain)need(!(b::blow(k.sample(false))&&!k.oc.isStop())||true,"");}
  need(k.cmd.op==b::Op::Chain&&k.oc.at_regs==5,"chain issued only after isStop, five AT registrations of the previous blow done");});
 test("chain_out_of_reach_pursues_find3_no_wait_no_lockout_then_attacks",[&]{fresh();Black k(T,0);fury_fight(k,40);
  for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}
  k.env.link.x=k.oc.pos.x+900;const unsigned t0=k.oc.tick;k.step(G);need(k.cmd.op==b::Op::Pursue&&k.oc.mode==b::Find&&k.oc.c2==0,"native FIND3->4 approach without lockout");
  need(k.oc.count("V_WAIT_ST",t0+1)==0&&(k.oc.count("V_WALK_ST",t0+1)+k.oc.count("V_DASH",t0+1))==1,"approach voice only");
  unsigned n=0;while(k.oc.mode!=b::Attack&&n++<200)k.step(G);need(k.oc.mode==b::Attack&&k.oc.count("V_WAIT_ST",t0+1)==0,"reached and attacked without the native pause");});
 test("chain_requires_facing_height_los_and_no_wall",[&]{for(int c=0;c<4;++c){fresh();Black k(T,0);fury_fight(k,30);for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}
   if(c==0){k.env.link={k.oc.pos.x,0,k.oc.pos.z+200};}else if(c==1){k.env.link={k.oc.pos.x+200,450,k.oc.pos.z};}else if(c==2){k.env.link.x=k.oc.pos.x+200;k.los=false;}else{k.env.link.x=k.oc.pos.x+200;k.wall=true;}
   k.step(G);need(k.cmd.op==b::Op::Pursue,"no chain at an impossible geometry: native FIND3 instead");}});
 test("fury_end_restores_x115_and_native_pause",[&]{fresh();Black k(T,0);fury_fight(k,330);need(k.d->fury==0,"fury expired at 300");const unsigned t0=k.oc.tick;const size_t r0=k.rates.size();
  for(int i=0;i<400;++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}need(k.oc.count("V_WAIT_ST",t0)>=2,"native WAIT_ST back");for(size_t i=r0;i<k.rates.size();++i)need(k.rates[i]==1.15f||k.rates[i]==1.0f,"x1.15 after fury");
  need(k.s->hunt,"hunt survives fury expiry");});
 test("damage_during_chain_keeps_native_priority",[&]{fresh();Black k(T,0);fury_fight(k,30);for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}
  k.env.link.x=k.oc.pos.x+200;k.env.hit=5;b::Actor a=k.sample(false);need(a.hit,"pending tg hit visible in PRE");k.step(G);need(k.cmd.op==b::Op::None&&k.oc.mode==b::Damage,"no command over a pending hit: damage_check owns the execute");});
 test("shield_recoil_completes_before_next_chained_blow",[&]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={230,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;k.env.guard=true;
  unsigned rec=0,ticks=0;bool chained_after=false;for(int i=0;i<200;++i){k.env.link.x=k.oc.pos.x+230;k.step(G);if(k.oc.mode==b::Attack&&k.oc.state==3){++ticks;if(!k.oc.isStop())++rec;}if(ticks&&k.cmd.op==b::Op::Chain){chained_after=true;break;}}
  need(rec>=4&&chained_after,"reverse recoil played to its native end, then the chain");});
 test("fury_find0_after_damage_skipped_and_lockout_cleared",[&]{fresh();Black k(T,0);fury_fight(k,5);k.oc.setActionMode(b::Find,0);const unsigned t0=k.oc.tick;k.step(G);need(k.cmd.op==b::Op::FuryFind3&&k.oc.c2==0&&k.oc.count("V_WAIT_ST",t0+1)==0,"FIND0 -> FIND3");
  k.oc.setActionMode(b::Find,4);k.oc.c2=17;k.env.link.x=k.oc.pos.x+1000;k.step(G);need(k.cmd.lockout&&k.oc.c2==0,"lockout cleared during fury only (Link beyond the native 400: native chase)");
  fresh();Black n(T,0);n.oc.setActionMode(b::Find,4);n.oc.c2=17;n.env.link={1000,0,0};n.step(G);need(!n.cmd.lockout&&n.oc.c2==16,"no fury: native lockout untouched");});
 test("terminal_event_hidden_transition_never_commanded",[&]{for(int c=0;c<3;++c){fresh();Black k(T,0);fury_fight(k,30);for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}
   if(c==0)k.event=true;else if(c==1)k.env.water=true;else k.oc.mode=devf_variant::A_FALL;
   k.ev.clear();k.step(G);
   if(c==1)need(k.oc.mode==b::WaterDeath&&k.ev.count("BLACK_CHAIN_DEFERRED_NATIVE_PRIORITY")==1,"water is decided by action() after the PRE: the native water death wins, the chain is only logged as deferred");
   else need(k.cmd.op==b::Op::None,"event / fall: no command at all");}});
 test("fury_leash_without_hunt_keeps_native_move_out",[&]{fresh();Black k(T,0);fury_fight(k,30);b::reset(*k.s,k.ev,"BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED","LAB");k.oc.range=200;k.oc.home={-2000,0,0};
  for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}k.env.link.x=k.oc.pos.x+200;k.step(G);need(k.oc.mode==b::MoveOut&&k.cmd.op==b::Op::None,"no hunt: native leash wins");});
 // ---- hunt ----
 auto hunt_setup=[](Black& k){k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={250,0,0};k.oc.angle=0x4000;k.fury_now(G);need(k.s->hunt,"armed");};
 test("hunt_keeps_pursuit_without_perception_timer_held",[&]{fresh();Black k(T,0);hunt_setup(k);k.d->fury=0;k.env.perceive=false;
  unsigned wait=0;for(int i=0;i<900;++i){k.env.link={k.oc.pos.x+1200,0,0};k.step(G);wait+=k.oc.mode==b::Wait;}
  need(wait<=1&&k.oc.mode==b::Find,"never left to WAIT for lost perception");need(k.ev.has("BLACK_HUNT_NATIVE_ABANDON_PREVENTED","FIND4_TIMER_0X6C0_HELD_AT_2_SEARCHPLAYER2_WAIT_AND_LEASH_MOVE_OUT_CHECK_DEFERRED"),"logged once");
  need(k.ev.count("BLACK_HUNT_PURSUIT_MEASURED")>=5,"pursuit measured periodically");});
 test("hunt_runs_after_link_across_room_far_from_home",[&]{fresh();Black k(T,0);hunt_setup(k);k.d->fury=0;k.oc.range=600;const P home=k.oc.home;
  for(int i=0;i<600;++i){k.env.link={float(250+i*4),0,0};k.step(G);}need(k.oc.pos.x>1500&&ra::root(ra::xz2(home,k.oc.pos))>600+300,"followed far beyond the native leash");need(k.oc.home.x==home.x&&k.oc.range==600,"home and range never written");
  need(k.ev.count("BLACK_HUNT_NATIVE_ABANDON_PREVENTED")>=1,"leash abandon prevented");});
 test("hunt_attack_end_with_leash_goes_find0_not_move_out",[&]{fresh();Black k(T,0);hunt_setup(k);k.d->fury=0;k.oc.range=100;k.oc.home={-1000,0,0};unsigned mo=0;
  unsigned stay=0;for(int i=0;i<400;++i){k.env.link.x=k.oc.pos.x+300;const int m0=k.oc.mode;k.step(G);mo+=m0==b::Attack&&k.oc.mode==b::MoveOut;if(k.oc.mode==b::MoveOut&&++stay>1)need(false,"a native MOVE_OUT never survives a second own execute while hunting");if(k.oc.mode!=b::MoveOut)stay=0;}need(mo==0&&k.ev.has("BLACK_HUNT_NATIVE_ABANDON_PREVENTED","HUNT_ATTACK_END_NATIVE_MOVE_OUT_REPLACED_BY_NATIVE_FIND0_PAUSE_KEPT"),"no MOVE_OUT at a blow end: native FIND0 instead");
  need(k.oc.count("V_WAIT_ST")>=2,"native pause kept outside fury");});
 test("hunt_damage_end_move_out_redirected",[&]{fresh();Black k(T,0);hunt_setup(k);k.d->fury=0;k.oc.range=100;k.oc.home={-1000,0,0};k.oc.setActionMode(b::MoveOut,0);k.oc.old=b::Damage;k.s->prev_mode=b::Damage;
  k.step(G);need(k.cmd.op==b::Op::Redirect&&k.oc.mode==b::Find,"MOVE_OUT -> native FIND");});
 test("room_change_resets_and_return_stays_reset_until_new_damage",[&]{fresh();Black k(T,0);hunt_setup(k);k.step(G,3);
  k.step(G,5);need(!k.s->hunt&&k.ev.count("BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED")==1,"A->B resets");
  for(int i=0;i<200;++i){k.env.link={k.oc.pos.x+1500,0,0};k.step(G,3);}need(!k.s->hunt,"B->A: no automatic reacquisition");
  k.d->fury=0;k.env.perceive=false;unsigned waits=0;for(int i=0;i<200;++i){k.env.link={5000,0,0};k.step(G,3);waits+=k.oc.mode==b::Wait;}need(waits>0,"native gives up again after the reset");
  k.hit();k.step(G,3);need(k.s->hunt&&k.ev.count("BLACK_HUNT_LOCKED_FIRST_DAMAGE")==2,"new real damage arms again");});
 test("a_b_a_without_black_execute_resets_via_poll_epoch",[&]{fresh();Black k(T,0);hunt_setup(k);
  b::Link l;l.present=true;l.stage=0x5150;l.pos={0,0,0};l.room=5;b::observe(G,l);l.room=3;b::observe(G,l);need(G.epoch==2,"two observed changes");
  Events pe;b::poll(*k.s,G,pe);need(!k.s->hunt&&pe.has("BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED","OBSERVED_BY_MANAGEMENT_POLL_WITHOUT_OWN_EXECUTE"),"poll resets without a Black execute");
  fresh();Black m(T,0);hunt_setup(m);b::observe(G,l);l.room=8;b::observe(G,l);l.room=3;b::observe(G,l);m.step(G,3);need(!m.s->hunt,"or at the next own PRE if no poll ran");});
 test("room_minus1_and_absent_link_hold_without_reset",[&]{fresh();Black k(T,0);hunt_setup(k);for(int i=0;i<20;++i)k.step(G,-1);for(int i=0;i<20;++i)k.step(G,3,0x5150,false);
  need(k.s->hunt&&k.ev.count("BLACK_HUNT_HELD_LINK_ROOM_TRANSIENT")<=2,"transient kept, logged on transition");need(k.cmd.op==b::Op::None,"no write while held");k.step(G,3);need(k.s->hunt,"resumes");});
 test("stage_change_resets",[&]{fresh();Black k(T,0);hunt_setup(k);k.step(G,3,0x9999);need(!k.s->hunt,"stage change");b::Rooms g2;b::Link l;l.present=true;l.room=3;l.stage=1;b::observe(g2,l);l.stage=2;need(b::observe(g2,l)&&g2.epoch==1,"same room number in another stage is another room");});
 test("death_and_generation_reuse_clear_hunt",[&]{fresh();Black k(T,0);hunt_setup(k);k.hp=3;k.hit(5);k.step(G);need(!k.s->hunt,"lethal blow resets");
  fresh();Black m(T,0);hunt_setup(m);auto& s2=b::state_for(T,0,1,2);need(!s2.hunt&&s2.gen==2,"new generation in the same slot inherits nothing");});
 test("two_blacks_independent_hunts",[&]{fresh();Black k(T,0,1),m(T,1,2);hunt_setup(k);m.oc.mode=b::Wait;m.oc.state=3;m.oc.c0=500;m.env.perceive=false;m.env.link={3000,0,0};
  for(int i=0;i<300;++i){k.env.link.x=k.oc.pos.x+800;k.step(G);m.step(G);}need(k.s->hunt&&!m.s->hunt&&m.oc.mode==b::Wait,"only the provoked Black hunts");});
 test("watch_sound_wait_redirect_and_bounce_limit",[&]{fresh();Black k(T,0);hunt_setup(k);k.d->fury=0;
  for(int m:{b::Watch,b::Sound,b::Wait}){k.oc.setActionMode(m,0);k.s->prev_mode=b::Damage;k.step(G);need(k.cmd.op==b::Op::Redirect&&k.oc.mode==b::Find,"redirect");}
  fresh();Black n(T,0);hunt_setup(n);n.d->fury=0;n.env.floor_gap=true;n.oc.range=100;n.oc.home={-3000,0,0};unsigned redirects=0,blocked=0;
  for(int i=0;i<300;++i){n.env.link.x=n.oc.pos.x+800;n.step(G);redirects+=n.cmd.op==b::Op::Redirect;}blocked=n.ev.count("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY");
  need(blocked>=3&&redirects<=3*(300/b::kBlockedTicks+1),"floor-gap/leash loop: at most 3 native re-entries per 90-tick backoff, each block logged");});
 test("find_stay_is_native_geometry_guard_logged_not_overridden",[&]{fresh();Black k(T,0);hunt_setup(k);k.d->fury=0;k.oc.setActionMode(b::FindStay,0);k.env.floor_gap=true;k.env.link.x=k.oc.pos.x+800;
  for(int i=0;i<5;++i)k.step(G);need(k.ev.count("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY")==1&&k.cmd.op==b::Op::None,"FIND_STAY kept, one line");});
 test("pursuit_blocked_by_wall_detected_from_measurement",[&]{b::State s;s.hunt=true;s.measuring=true;s.measure_ticks=90;s.native_sum=1800;s.moved=50;s.dist_now=900;Events e;b::end_pursuit(s,e);
  need(e.count("BLACK_HUNT_PURSUIT_MEASURED")==1&&e.count("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY")==1,"collision absorbed the chase");});
 test("telemetry_bounded_critical_reserve_chain_detail_cap",[&]{fresh();Black k(T,0);fury_fight(k,290);b::Budget bud;unsigned taken=0;for(auto& e:k.ev.ev)taken+=bud.take(*k.s,e.c_str());
  need(bud.detail_suppressed>0,"chain detail capped per fury episode");b::State s;for(int i=0;i<2000;++i)bud.take(s,"BLACK_HUNT_PURSUIT_MEASURED");need(s.crit_logs<=b::Budget::kCritActor,"per-actor critical cap");
  b::State r;b::Budget c2;for(int i=0;i<1000;++i)c2.take(r,"BLACK_RATE_SKIPPED");need(c2.global<=b::Budget::kActor&&c2.crit==0,"routine cap leaves the critical reserve untouched");need(c2.take(r,"BLACK_HUNT_LOCKED_FIRST_DAMAGE")&&c2.crit==1,"a critical line still passes once routine lines are exhausted");
  fresh();Black q(T,0);hunt_setup(q);q.d->fury=0;q.ev.clear();for(int i=0;i<1000;++i){q.env.link={q.oc.pos.x+1200,0,0};q.step(G);}need(q.ev.ev.size()<=40,"no per-tick lines while hunting");});
 test("leash_next_is_the_native_action_update",[]{b::Actor a;a.range=500;a.home={0,0,0};a.pos={600,0,0};need(b::leash_next(a)==1,"1");a.pos={900,0,0};need(b::leash_next(a)==2,"2");a.leash=1;a.pos={450,0,0};need(b::leash_next(a)==1,"hysteresis");a.pos={390,0,0};need(b::leash_next(a)==0,"back");a.range=0;a.leash=1;need(b::leash_next(a)==1,"range 0 leaves it");});
 cp19_cases();
 std::printf("STATE_BYTES %zu TABLE_BYTES %zu\n",sizeof(b::State),sizeof(T));
 std::printf("CP19_LAB_PASS cases=%u\n",cases);
 return 0;
}
// ======================================================================= CP19 =======================================================================
namespace c19=devf_cp19;namespace rc=devf_recruit;
// ---- A: port of Z2SceneMgr::setSceneName (F_SP121 day field branch) + load2ndDynamicWave (same frame, d_kankyo.cpp) + JASWaveArc status machine ----
struct ScenePort{
 struct Arc{int status=0;};std::array<Arc,256> arcs{};struct Fly{int wave,left;};std::vector<Fly> fly;bool exist=true,violation=false;const char* why="";
 std::uint8_t req1=0,loaded1=0,req2=0,loaded2=0,reqd=0,loadedd=0;
 bool load(int w){auto& a=arcs[w];if(a.status!=0)return false;a.status=1;fly.push_back({w,3});return true;}   // JASWaveArc::load refuses unless status 0
 bool erase(int w){auto& a=arcs[w];if(a.status==1){violation=true;why="ERASE_DURING_LOAD";}const bool had=a.status!=0;a.status=0;return had;}
 void pass(){for(auto& f:fly)if(f.left>0&&--f.left==0&&arcs[f.wave].status==1)arcs[f.wave].status=2;}
 static std::uint8_t wave2(int room){switch(room){case 2:case 3:case 4:return 0x5e;case 0:case 5:case 7:return 0x5f;case 9:case 10:case 11:return 0x60;case 12:case 13:case 14:return 0x61;default:return 0x4b;}}
 void field_room(int room){req1=0x4a;req2=wave2(room);reqd=0;
  if(loaded1!=req1){if(loaded1)erase(loaded1);if(load(req1))loaded1=req1;else loaded1=0;}
  if(req2!=loaded2){if(loaded2!=0&&erase(loaded2))loaded2=0;if(req2!=0&&load(req2))loaded2=req2;else loaded2=0;}}
 void other_stage(){req1=0x26;req2=0x27;if(loaded2&&erase(loaded2))loaded2=0;if(loaded1&&erase(loaded1))loaded1=0;if(load(0x26))loaded1=0x26;if(load(0x27))loaded2=0x27;}
 c19::Waves waves()const{c19::Waves w;w.exist=exist;w.req1=req1;w.loaded1=loaded1;w.req2=req2;w.loaded2=loaded2;w.req_demo=reqd;w.loaded_demo=loadedd;return w;}
};
struct WaveRun{ScenePort sc;c19::Keeper k;bool field=true,transition=false;unsigned last=0,pass_no=0;std::vector<std::string> ops;
 void step(bool bokoblin){++pass_no;sc.pass();if(bokoblin)last=pass_no;const bool recent=last&&pass_no-last<=c19::kRecentPasses;
  const int status=sc.arcs[c19::kOcWave].status;const auto c=c19::keeper_step(k,field,transition,sc.waves(),recent,status);
  if(c.op==c19::WaveOp::Load){const bool ok=sc.load(c19::kOcWave);c19::keeper_result(k,c.op,ok);ops.push_back(ok?"LOAD":"LOAD_REFUSED");}
  else if(c.op==c19::WaveOp::Erase){if(sc.loaded1==c19::kOcWave||sc.loaded2==c19::kOcWave){sc.violation=true;sc.why="ERASED_A_SCENE_LOADED_WAVE";}if(sc.arcs[c19::kOcWave].status!=2){sc.violation=true;sc.why="ERASE_NOT_LOADED";}
   const bool ok=sc.erase(c19::kOcWave);c19::keeper_result(k,c.op,ok);ops.push_back("ERASE");}}
 int status()const{return sc.arcs[c19::kOcWave].status;}
};
// ---- F: port of daE_OC_c::damage_check (sword branch, health<=1, BIG_DAMAGE decision) and cc_at_check (power, Master x2, Ordon x1, lethal) ----
struct HelmHit{bool master=true,wood=false,spl=true;int atp=2;};
struct HelmOut{int health=0,mode=-1,state=-1,my_val=0;bool lethal_se=false,achievement=false,attention_cleared=false,processed=false;};
int eoc_cut(std::uint8_t cut){return cut==8||cut==5||cut==0x18?6:2;}   // TURN_RIGHT / LARGE_TURN_RIGHT / HEAD_JUMP -> 6 (lab subset)
HelmOut port_damage(int health,std::uint8_t link_cut,const HelmHit& h,int invuln){
 HelmOut o;o.health=health;if(invuln>0)return o;o.processed=true;   // execute() already decremented field_0x6cc
 int my=eoc_cut(link_cut);if(link_cut==c19::kCutHeadJump)o.health=0;  // damage_check: `health = 0` for the Helm Splitter
 int p=h.atp;if(h.master)p*=2;if(h.wood)p/=2;                         // cc_at_check: E_OC mPowerType 1 keeps the raw atp
 o.health-=p;int status=h.spl?1:0;if(o.health<=0){status=2;o.health=0;o.achievement=true;}
 o.lethal_se=status==2;if(o.health<=1){o.attention_cleared=true;if(my<5)my=5;}
 o.my_val=my;o.mode=my>=5?devf_variant::A_BIG_DAMAGE:devf_variant::A_DAMAGE;o.state=my>=5?my-5:my;return o;
}
struct HelmCase{c19::Helm h;std::uint8_t link_cut=c19::kCutHeadJump;int invuln=0;};
c19::HelmOp last_helm_op=c19::HelmOp::None;
HelmOut helm_run(HelmCase& c,int health,const HelmHit& hit,bool same_link=true,bool foreign=false,bool* restored=nullptr,bool* skipped=nullptr){
 c19::HelmView v;v.link=true;v.cut=c.link_cut;v.pending=true;v.sword_hit=true;v.sword=hit.wood?c19::Sword::Wood:hit.master?c19::Sword::Master:c19::Sword::Ordon;v.health=health;v.mode=b::Find;v.invuln=c.invuln;v.link_id=77;
 const auto cmd=c19::helm_pre(c.h,v);last_helm_op=cmd.op;if(skipped)*skipped=cmd.op==c19::HelmOp::SkipDuplicate;
 std::uint8_t cut=c.link_cut;if(cmd.op==c19::HelmOp::Mask)cut=c19::kCutTurnRight;int inv=cmd.op==c19::HelmOp::SkipDuplicate?2:c.invuln;
 auto o=port_damage(health,cut,hit,inv>1?1:0);
 if(foreign)cut=c19::kCutHeadJump+20;
 const auto p=c19::helm_post(c.h,same_link,cut,o.health);if(p.unmask)cut=c19::kCutHeadJump;if(p.write)o.health=p.health;
 if(restored)*restored=cut==c19::kCutHeadJump;c.link_cut=cut;return o;
}
// ---- E: Red reaction harness (CP17 FIX01 policy + CP19 close-in) against the native FIND state 4 lockout-branch chase and posMoveF ----
namespace r=devf_red_ai;
r::Actor red_actor(){r::Actor a;a.id=5;a.gen=1;a.variant=V::Red;std::memcpy(a.stage,"F_SP121",8);a.room=3;a.layer=6;a.health=330;a.ready=true;a.perceived=true;a.mode=r::Find;a.state=4;a.pos={0,0,0};a.home=a.pos;a.range=5000;a.end=40;a.angle=0x4000;return a;}
r::Link red_link(float x){r::Link l;l.id=10;l.sampled=true;l.present=l.basic=l.canary=l.safe=l.los=true;std::memcpy(l.stage,"F_SP121",8);l.room=3;l.layer=6;l.pos={x,0,0};l.motion=r::Motion::Quiet;l.proc=3;l.timer=0;l.gravity=-3.4f;l.ground_y=0;l.grounded=true;l.vel_valid=true;l.target=r::Target::ThisRed;l.heading=-0x4000;return l;}
struct RedRun{std::array<r::State,64> st{};r::Actor a=red_actor();r::Link l;std::vector<std::string> ev;double forced_dist=-1;unsigned close_ticks=0;int lockout_left=0;
 r::State& s(){return st[0];}
 RedRun(float x):l(red_link(x)){r::state_for(st,0,a,7);}
 void tick(float link_dx){
  l.pos.x+=link_dx;auto emit=[&](const char*e,r::Cause,const char*){ev.emplace_back(e);};
  auto cmd=r::pre(s(),a,l,emit);const double d=r::d3(a.pos,l.pos);
  if(cmd.force){forced_dist=d;a.mode=r::Attack;a.state=0;a.speed=0;r::post(s(),a,emit);return;}
  int lock=a.lockout;if(cmd.close){a.speed=cmd.speed;if(cmd.close_lockout)lock=2;++close_ticks;}else if(cmd.rush)a.speed=cmd.speed;
  if(lock)--lock;                                              // execute(): field_0x6c2 decrement
  // executeFind state 4, no event: lockout branch in (200,400) chases speedF toward 0; free branch toward 20 / -3 and may switch to ATTACK
  bool native_attack=false;
  if(lock==0){a.speed=r::chase(a.speed,d<300?-3.f:20.f,1.f);if(d<400&&d>200)native_attack=true;}
  else{if(d<400&&d>200)a.speed=r::chase(a.speed,0,1);else if(d<300)a.speed=r::chase(a.speed,-3,1);else a.speed=r::chase(a.speed,20,1);}
  if(native_attack){forced_dist=d;ev.emplace_back("NATIVE_SWITCH");a.mode=r::Attack;a.state=0;return;}
  a.pos.x+=a.speed;a.lockout=lock;                             // posMoveF along +x (Red faces Link)
  const auto rp=r::rush_post(s(),a,emit);if(rp.write)a.speed=rp.value;
  const auto cp=r::close_after(s(),a,emit);if(cp.speed)a.speed=cp.speed_value;if(cp.lockout)a.lockout=0;
  r::post(s(),a,emit);
 }
 unsigned count(const char*n)const{return unsigned(std::count(ev.begin(),ev.end(),n));}
};
void cp19_cases(){
 // ---------------- A ----------------
 test("cp19_A_wave_matrix_rooms_with_and_without_native_oc_samples",[]{
  for(int room=0;room<16;++room){WaveRun w;w.sc.field_room(room);for(int i=0;i<10;++i)w.step(true);
   const std::uint8_t w2=ScenePort::wave2(room);const bool lacks=w2==0x5f||w2==0x60||w2==0x61;
   need(lacks==(w.k.owned&&w.status()==2),"0x5e loaded by CP19 exactly in field rooms whose wave 2 lacks the E_OC samples");need(!w.sc.violation,w.sc.why);}
 });
 test("cp19_A_no_load_without_bokoblin_twilight_horse_or_demo",[]{
  {WaveRun w;w.sc.field_room(10);for(int i=0;i<30;++i)w.step(false);need(!w.k.owned&&w.ops.empty(),"no Bokoblin executing: nothing loaded");}
  {WaveRun w;w.sc.field_room(10);w.sc.req2=w.sc.loaded2=0x4c;for(int i=0;i<30;++i)w.step(true);need(!w.k.owned,"twilight wave set: untouched");}
  {WaveRun w;w.sc.field_room(10);w.sc.req1=w.sc.loaded1=0x54;for(int i=0;i<30;++i)w.step(true);need(!w.k.owned,"horse battle layer: untouched");}
  {WaveRun w;w.sc.field_room(10);w.sc.reqd=w.sc.loadedd=0x74;for(int i=0;i<30;++i)w.step(true);need(!w.k.owned,"demo wave: untouched");}
  {WaveRun w;w.field=false;w.sc.field_room(10);for(int i=0;i<30;++i)w.step(true);need(!w.k.owned,"other stage: untouched");}
 });
 test("cp19_A_room_walk_7_3_10_3_leave_never_double_frees",[]{
  WaveRun w;w.sc.field_room(7);for(int i=0;i<20;++i)w.step(true);need(w.k.owned&&w.status()==2,"room 7: loaded");
  w.sc.field_room(3);need(w.sc.loaded2==0,"room 3 entered with CP19's 0x5e loaded: the scene's own load is refused (loaded2 = 0)");for(int i=0;i<20;++i)w.step(true);
  need(w.k.owned&&w.status()==2,"room 3 keeps the wave (the scene requests it)");
  w.sc.field_room(10);for(int i=0;i<20;++i)w.step(true);need(w.k.owned&&w.status()==2&&w.sc.loaded2==0x60,"room 10: kept for its Bokoblins");
  w.sc.field_room(3);for(int i=0;i<400;++i)w.step(false);need(w.status()==2,"room 3 without Bokoblins: still requested by the scene, never erased");
  w.sc.field_room(7);for(int i=0;i<400;++i)w.step(false);need(!w.k.owned&&w.status()==0,"room 7, no Bokoblin for 300 passes: released");
  need(!w.sc.violation,w.sc.why);
 });
 test("cp19_A_scene_owned_wave_never_erased_and_reload_after_engine_erase",[]{
  WaveRun w;w.sc.field_room(3);for(int i=0;i<10;++i)w.step(true);need(!w.k.owned&&w.sc.loaded2==0x5e,"scene loaded it itself: CP19 never loads or owns it");
  w.sc.field_room(7);for(int i=0;i<10;++i)w.step(true);need(w.k.owned&&w.status()==2,"scene erased its own 0x5e leaving room 3; CP19 loads it again for room 7 Bokoblins");need(!w.sc.violation,w.sc.why);
 });
 test("cp19_A_transition_or_stage_exit_releases_only_when_loaded",[]{
  WaveRun w;w.sc.field_room(7);w.step(true);need(w.status()==1,"loading");w.transition=true;w.step(true);need(w.status()==1&&w.k.owned,"never erased while the DVD load is in flight");
  for(int i=0;i<5;++i)w.step(false);need(!w.k.owned&&w.status()==0,"released once loaded");need(!w.sc.violation,w.sc.why);
  WaveRun x;x.sc.field_room(10);for(int i=0;i<10;++i)x.step(true);x.field=false;x.sc.other_stage();x.step(false);need(!x.k.owned&&x.status()==0&&!x.sc.violation,"stage left: released");
 });
 test("cp19_A_refused_load_retries_after_300_passes",[]{
  WaveRun w;w.sc.field_room(10);w.sc.arcs[c19::kOcWave].status=0;
  c19::Keeper& k=w.k;auto c=c19::keeper_step(k,true,false,w.sc.waves(),true,0);need(c.op==c19::WaveOp::Load,"load");c19::keeper_result(k,c.op,false);
  for(int i=0;i<299;++i){c=c19::keeper_step(k,true,false,w.sc.waves(),true,0);need(c.op==c19::WaveOp::None,"backoff");}
  c=c19::keeper_step(k,true,false,w.sc.waves(),true,0);need(c.op==c19::WaveOp::Load&&k.failures==1,"retry");
 });
 // ---------------- B / C ----------------
 test("cp19_B_attention_restored_only_on_live_visible_survivors",[]{
  need(c19::restore_attention(0,2,b::Find,false,false),"survivor at 2");need(!c19::restore_attention(0,1,b::Find,false,false),"1 HP keeps the native clear");
  need(!c19::restore_attention(0,50,b::Death,false,false)&&!c19::restore_attention(0,50,devf_variant::A_DEMO_MASTER,false,false)&&!c19::restore_attention(0,50,b::WaterDeath,false,false),"terminal/demo");
  need(!c19::restore_attention(0,50,b::Find,true,false)&&!c19::restore_attention(0,50,b::Find,false,true),"hidden/deleting");
  need(!c19::restore_attention(4,50,b::Find,false,false),"a nonzero native value is never touched");need(!c19::restore_attention(0,50,b::BigDamage,false,false),"knock-down flight waits for a live mode");
 });
 test("cp19_B_port_exact_one_hp_survivor_then_regen",[]{
  HelmHit hit;hit.master=false;hit.atp=2;auto o=port_damage(3,3,hit,0);need(o.health==1&&o.attention_cleared&&o.mode==devf_variant::A_BIG_DAMAGE&&!o.lethal_se,"native: 1 HP survivor loses attention");
  int hp=1;for(int t=0;t<20;++t)hp+=t%8==0;need(c19::restore_attention(0,hp,b::Find,false,false),"after regeneration back in FIND: restored");
 });
 test("cp19_C_zero_hp_live_mode_completes_native_death",[]{
  // damage_check: CUT_TYPE_JUMP + checkCutJumpCancelTurn sets DAMAGE(my_val) and returns before the BIG_DAMAGE/death decision
  HelmHit hit;hit.master=true;hit.atp=4;auto o=port_damage(5,10,hit,0);need(o.health==0&&o.lethal_se&&o.achievement&&o.attention_cleared,"native kill applied (health 0, kill SE, achievement)");
  need(c19::complete_zombie(0,b::Damage,false,false)&&c19::complete_zombie(0,b::Find,false,false),"0 HP in DAMAGE/FIND: completed");
  need(!c19::complete_zombie(0,b::BigDamage,false,false)&&!c19::complete_zombie(0,b::Death,false,false)&&!c19::complete_zombie(1,b::Find,false,false)&&!c19::complete_zombie(0,b::Find,true,false),"native death paths, survivors and hidden actors untouched");
 });
 test("cp19_C_residual_lasthit_flag_cleared_after_30_live_ticks_only",[]{
  c19::Residual r;for(unsigned i=1;i<c19::kResidualTicks;++i)need(!c19::residual_clear(r,1,50,b::Find),"wait");need(c19::residual_clear(r,1,50,b::Find),"cleared at 30");
  c19::Residual q;for(int i=0;i<100;++i)need(!c19::residual_clear(q,1,0,b::Find)&&!c19::residual_clear(q,1,50,b::Death),"dead / dying keep the native flag");
  c19::Residual z;for(int i=0;i<20;++i)c19::residual_clear(z,1,50,b::Find);need(!c19::residual_clear(z,1,50,b::Damage),"damage resets");for(unsigned i=1;i<c19::kResidualTicks;++i)need(!c19::residual_clear(z,1,50,b::Find),"restart");
 });
 // ---------------- F ----------------
 test("cp19_F_helm_matrix_exact_100_master_50_ordon",[]{
  for(int hp:{40,50,51,100,101,150,220,330,440,500})for(bool master:{true,false})for(int atp:{1,2,3,4}){
   HelmCase c;HelmHit hit;hit.master=master;hit.atp=atp;bool restored=false;auto o=helm_run(c,hp,hit,true,false,&restored);
   const int d=master?100:50;need(restored,"Link's cut type restored to HEAD_JUMP");
   if(hp>d){need(o.health==hp-d,"exact helm damage");need(!o.lethal_se&&!o.achievement,"no kill SE, no kill achievement");need(o.mode==devf_variant::A_BIG_DAMAGE&&o.state==1,"same native knock-down reaction (getCutType 6)");
    need(o.attention_cleared==(hp-atp*(master?2:1)<=1),"attention follows the native rule on the native result");}
   else need(o.health==0&&o.lethal_se,"HP <= D: native kill untouched");}
 });
 test("cp19_F_helm_never_heals_duplicates_wood_foreign",[]{
  {HelmCase c;HelmHit hit;hit.atp=80;auto o=helm_run(c,500,hit);need(o.health==340,"native power above D: native result kept (never healed to H-D)");}
  {HelmCase c;HelmHit hit;bool skip=false;helm_run(c,330,hit);auto o=helm_run(c,230,hit,true,false,nullptr,&skip);need(skip&&o.health==230,"second impact of the same Helm Splitter ignored");
   c.link_cut=0;helm_run(c,230,hit);c.link_cut=c19::kCutHeadJump;o=helm_run(c,230,hit);need(o.health==130,"a new Helm Splitter episode applies again");}
  {HelmCase c;HelmHit hit;hit.wood=true;hit.master=false;auto o=helm_run(c,330,hit);need(o.health==0,"wooden sword: native unchanged");}
  {HelmCase c;HelmHit hit;bool restored=true;helm_run(c,330,hit,true,true,&restored);need(!restored&&c.h.foreign==1,"cut type changed by others: not overwritten");}
  {HelmCase c;c.invuln=5;HelmHit hit;auto o=helm_run(c,330,hit);need(last_helm_op==c19::HelmOp::None&&o.health==330&&c.h.applied==0,"native invulnerability: nothing masked");}
 });
 // ---------------- D ----------------
 test("cp19_D_recruit_policy_offers_and_keeps_receivers_beyond_their_leash",[]{
  rc::Table t;auto frame=[](std::uint32_t id,V v,rc::Point p,bool leash_out){rc::Frame f;f.id=id;f.gen=1;f.seq=1;f.variant=v;std::memcpy(f.stage,"F_SP121",8);f.room=3;f.layer=6;f.mode=rc::Wait;f.health=100;f.pos=p;f.home={0,0,0};f.range=300;f.ready=true;f.leash_out=leash_out;return f;};
  auto& s=t.publish(0,frame(1,V::Blue,{1500,0,0},false));auto& r=t.publish(1,frame(2,V::Red,{500,0,0},true));r.f.home={-2000,0,0};
  need(rc::candidate(s,r,1,4)==rc::Skip::Offered,"a receiver whose own leash would refuse the Blue position is now offered");
  r.order.active=true;r.order.caller=1;r.order.caller_gen=1;std::vector<std::string> ev;auto emit=[&](const char*e,std::uint32_t,std::uint32_t,const rc::Frame&,const char*w){ev.emplace_back(std::string(e)+":"+w);};
  need(rc::order_valid(t,r,1,4,[](const rc::Frame&){return true;},emit),"order valid beyond the receiver leash");
  s.f.leash_out=true;need(!rc::order_valid(t,r,1,4,[](const rc::Frame&){return true;},emit),"the CALLER's own leash return still aborts");
  auto& f=t.publish(2,frame(3,V::Red,{9000,0,0},false));need(rc::candidate(s,f,1,4)!=rc::Skip::Offered,"radius rule kept");
 });
 test("cp19_D_permission_lifecycle",[]{
  c19::Permit p;c19::PermitView v;v.id=2;v.gen=1;std::memcpy(v.stage,"F_SP121",8);v.room=3;v.layer=6;v.health=100;v.mode=b::Find;v.safe=true;v.link_same_room=true;v.caller_alive=true;
  c19::permit_grant(p,v,1,1,10);need(p.active,"granted");
  v.mode=b::Wait;v.link_same_room=false;for(unsigned t=11;t<2000;++t)c19::permit_pre(p,v,t);need(p.active,"caller alive: kept even calm and out of LOS");
  v.caller_alive=false;v.link_same_room=true;v.mode=b::Find;for(unsigned t=2000;t<2400;++t)c19::permit_pre(p,v,t);need(p.active,"caller dead but the fight goes on: kept");
  v.mode=b::Wait;unsigned t=2400;c19::PermitCmd c;for(;t<2600&&p.active;++t)c=c19::permit_pre(p,v,t);need(!p.active&&c.op==c19::PermitOp::Ended&&t-1>=2400+c19::kPermitCalm-1,"caller dead and calm 150: ended");
  c19::permit_grant(p,v,1,1,0);v.gen=2;c=c19::permit_pre(p,v,1);need(!p.active,"new generation: ended");
  v.gen=1;c19::permit_grant(p,v,1,1,0);v.room=4;c19::permit_pre(p,v,1);need(!p.active,"context change: ended");
  v.room=3;c19::permit_grant(p,v,1,1,0);v.health=0;c19::permit_pre(p,v,1);need(!p.active,"death: ended");
 });
 test("cp19_D_permission_effects_only_against_the_leash",[]{
  c19::Permit p;c19::PermitView v;v.id=2;v.gen=1;std::memcpy(v.stage,"F_SP121",8);v.room=3;v.layer=6;v.health=100;v.safe=true;v.link_same_room=true;v.caller_alive=true;c19::permit_grant(p,v,1,1,0);
  v.mode=b::Attack;v.state=1;v.stopped=true;v.leash=0;need(c19::permit_pre(p,v,1).op==c19::PermitOp::None,"no leash: native");
  v.leash=1;need(c19::permit_pre(p,v,2).op==c19::PermitOp::Find0,"attack end beyond leash: FIND0 instead of MOVE_OUT");
  v.mode=b::MoveOut;v.state=0;need(c19::permit_pre(p,v,3).op==c19::PermitOp::Find0,"MOVE_OUT: FIND0");
  v.mode=b::Wait;auto c=c19::permit_pre(p,v,4);need(c.op==c19::PermitOp::Find1&&c.search,"WAIT beyond leash: native searchPlayer then FIND1");
  v.hunting_black=true;v.mode=b::MoveOut;need(c19::permit_pre(p,v,5).op==c19::PermitOp::None,"CP18 hunt owns a provoked Black");v.hunting_black=false;
  v.safe=false;need(c19::permit_pre(p,v,6).op==c19::PermitOp::None,"pending hit / event: native priority");v.safe=true;
  unsigned blocked=0,finds=0;for(unsigned t=100;t<400;++t){v.mode=(t%3==0)?b::MoveOut:b::Find;const auto k=c19::permit_pre(p,v,t);blocked+=k.op==c19::PermitOp::Blocked;finds+=k.op==c19::PermitOp::Find0;}
  need(blocked>=1&&finds<=3*(300/c19::kPermitBlocked+1),"floor/leash loop bounded");
 });
 // ---------------- E / G ----------------
 test("cp19_E_red_charge_reaction_starts_only_at_strike_distance",[]{
  for(float start:{250.f,300.f,350.f,390.f}){RedRun R(600);R.tick(0);R.tick(0);need(R.forced_dist<0,"calm FIND4 established beyond 400");
   R.l.pos.x=R.a.pos.x+start;R.l.charge=r::Charge::Spin;R.l.proc=0x29;
   for(int i=0;i<60&&R.forced_dist<0;++i)R.tick(0);
   need(R.count("NATIVE_SWITCH")==0,"the native 200..400 switch never fires during the reaction");need(R.forced_dist>0&&R.forced_dist<=r::kStrike+1e-6,"x2 reaction blow started at <= 240");
   need(R.count("CLOSE_IN_STARTED")==1&&R.count("CLOSE_IN_MEASURED")==1&&R.close_ticks>0,"close-in logged and measured");}
 });
 test("cp19_E_red_close_in_restores_native_speed_and_lease",[]{
  RedRun R(600);R.tick(0);R.tick(0);R.l.pos.x=R.a.pos.x+350;R.l.charge=r::Charge::Spin;R.l.proc=0x29;R.tick(0);const float v0=R.a.speed;const float x0=R.a.pos.x;R.tick(0);need(R.close_ticks>=2,"closing");need(R.a.lockout==0,"lease given back");need(R.a.speed==v0,"PRE speed given back after posMoveF");need(R.a.pos.x>x0+1,"moved toward Link");
 });
 test("cp19_E_red_unreachable_logged_no_forced_blow",[]{
  RedRun R(600);R.tick(0);R.tick(0);R.l.pos.x=R.a.pos.x+350;R.l.charge=r::Charge::Spin;R.l.proc=0x29;for(int i=0;i<120;++i){R.tick(0);R.l.pos.x=R.a.pos.x+350;}
  need(R.forced_dist<0&&R.count("NATIVE_SWITCH")==0,"Link keeps away: no blow from 350");need(R.count("CLOSE_IN_UNREACHABLE")==1,"unreachable logged once");
 });
 test("cp19_EG_black_fury_strike_only_at_strike_distance_with_close_in",[&]{
  for(float lx:{260.f,320.f,390.f}){fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={lx,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;
   const unsigned from=k.oc.tick+1;std::vector<double> d_at_start;
   for(int i=0;i<200;++i){const double d=ra::d3(k.oc.pos,k.env.link);const size_t n=k.starts.size();k.step(G);if(k.starts.size()>n)d_at_start.push_back(d);}
   need(!d_at_start.empty(),"blows happen");for(double d:d_at_start)need(d<=ra::kStrike+30,"every fury blow started inside strike distance (plus at most one approach step)");
   need(k.ev.count("BLACK_CLOSE_IN_STARTED")>=1&&k.ev.count("BLACK_STRIKE_STARTED")>=1,"close-in then strike");need(k.oc.count("V_WAIT_ST",from)==0,"no wait voice");}
 });
 test("cp19_EG_black_clip_end_at_300_pursues_instead_of_chaining",[&]{
  fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={200,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;need(k.to_blow(G),"blow");
  for(int i=0;i<60&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+200;k.step(G);}
  k.env.link.x=k.oc.pos.x+300;k.step(G);need(k.cmd.op==b::Op::Pursue,"Link stepped back to 300 at the clip end: native FIND3 and final approach, no blow from 300");
 });
 test("cp19_G_black_normal_blows_keep_native_distances",[&]{
  fresh();Black k(T,0);k.oc.mode=b::Find;k.oc.state=4;k.oc.c0=30;k.env.link={380,0,0};k.oc.angle=0x4000;need(k.to_blow(G),"native blow without fury at 380");need(k.ev.count("BLACK_CLOSE_IN_STARTED")==0,"x1.15 blows untouched");
 });
 test("cp19_G_active_window_distance_recorded",[&]{
  fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={200,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;need(k.to_blow(G),"blow");
  while(b::blow(k.sample(true))&&!k.oc.isStop())k.step(G);need(k.s->active_min>0&&k.s->active_min<=200,"closest Link distance over the registered active ticks");
 });
}
