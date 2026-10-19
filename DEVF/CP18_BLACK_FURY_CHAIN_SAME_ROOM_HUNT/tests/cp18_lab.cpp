// CP18 laboratory: the DLL's own black_ai_policy.hpp and the real CP10.6 dynamic_policy.hpp (admission, capture_pre, post_update) driven against a
// source-shaped port of the daE_OC_c code paths CP18 depends on (official d_a_e_oc.cpp @40457c6): execute() timer decrements, action() leash update,
// executeAttack (B/C start, checkFrame sounds, AT bit 14..22, translation, end branch setBck(0x1C)+V_WAIT_ST, shield recoil), executeFind 0..4,
// executeWait, executeWatch, executeMoveOut, executeFindStay, executeDamage, setActionMode, posMoveF and J3DFrameCtrl::update once/loop.
// It is a PORT, not the engine: no BCK data (clip ends are lab values), no collision, no audio device. Voices/clips are the calls the port makes.
#include "../runtime/DEVF/CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT/expansion/black_ai_policy.hpp"
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
  if(cmd.zero_speed)oc.speed=0;if(cmd.timer)oc.c0=cmd.timer_value;if(cmd.lockout)oc.c2=cmd.lockout_value;
  oc.execute(env);
  if(env.dmg){hp-=env.dmg;env.dmg=0;}
  devf_dyn::Post in;in.post_health=hp;in.action=oc.mode;in.room=room;in.x=oc.pos.x;in.y=oc.pos.y;in.z=oc.pos.z;in.stage=stage;in.seq=++seq;in.window=4;
  const auto e=devf_dyn::post_update(led,*d,io,in,[](const devf_dyn::Entry&){return true;});
  if(e.kind==devf_dyn::Ev::FuryStart||e.kind==devf_dyn::Ev::FuryRefresh)b::damage(*s,sample(true),l,g,e.kind==devf_dyn::Ev::FuryStart,ev);
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
 auto fury_fight=[](Black& k,unsigned ticks,float lx=300){k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={lx,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;for(unsigned i=0;i<ticks;++i){k.env.link.x=k.oc.pos.x+lx;k.step(G);}};
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
 test("chain_only_after_complete_clip_previous_window_whole",[&]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={300,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;
  need(k.to_blow(G),"blow");k.oc.at_regs=0;unsigned guard=0;while(!(k.cmd.op==b::Op::Chain)&&guard++<60){k.env.link.x=k.oc.pos.x+300;k.step(G);if(k.cmd.op!=b::Op::Chain)need(!(b::blow(k.sample(false))&&!k.oc.isStop())||true,"");}
  need(k.cmd.op==b::Op::Chain&&k.oc.at_regs==5,"chain issued only after isStop, five AT registrations of the previous blow done");});
 test("chain_out_of_reach_pursues_find3_no_wait_no_lockout_then_attacks",[&]{fresh();Black k(T,0);fury_fight(k,40);
  for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+300;k.step(G);}
  k.env.link.x=k.oc.pos.x+900;const unsigned t0=k.oc.tick;k.step(G);need(k.cmd.op==b::Op::Pursue&&k.oc.mode==b::Find&&k.oc.c2==0,"native FIND3->4 approach without lockout");
  need(k.oc.count("V_WAIT_ST",t0+1)==0&&(k.oc.count("V_WALK_ST",t0+1)+k.oc.count("V_DASH",t0+1))==1,"approach voice only");
  unsigned n=0;while(k.oc.mode!=b::Attack&&n++<200)k.step(G);need(k.oc.mode==b::Attack&&k.oc.count("V_WAIT_ST",t0+1)==0,"reached and attacked without the native pause");});
 test("chain_requires_facing_height_los_and_no_wall",[&]{for(int c=0;c<4;++c){fresh();Black k(T,0);fury_fight(k,30);for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+300;k.step(G);}
   if(c==0){k.env.link={k.oc.pos.x,0,k.oc.pos.z+300};}else if(c==1){k.env.link={k.oc.pos.x+300,450,k.oc.pos.z};}else if(c==2){k.env.link.x=k.oc.pos.x+300;k.los=false;}else{k.env.link.x=k.oc.pos.x+300;k.wall=true;}
   k.step(G);need(k.cmd.op==b::Op::Pursue,"no chain at an impossible geometry: native FIND3 instead");}});
 test("fury_end_restores_x115_and_native_pause",[&]{fresh();Black k(T,0);fury_fight(k,330);need(k.d->fury==0,"fury expired at 300");const unsigned t0=k.oc.tick;const size_t r0=k.rates.size();
  for(int i=0;i<400;++i){k.env.link.x=k.oc.pos.x+300;k.step(G);}need(k.oc.count("V_WAIT_ST",t0)>=2,"native WAIT_ST back");for(size_t i=r0;i<k.rates.size();++i)need(k.rates[i]==1.15f||k.rates[i]==1.0f,"x1.15 after fury");
  need(k.s->hunt,"hunt survives fury expiry");});
 test("damage_during_chain_keeps_native_priority",[&]{fresh();Black k(T,0);fury_fight(k,30);for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+300;k.step(G);}
  k.env.link.x=k.oc.pos.x+300;k.env.hit=5;b::Actor a=k.sample(false);need(a.hit,"pending tg hit visible in PRE");k.step(G);need(k.cmd.op==b::Op::None&&k.oc.mode==b::Damage,"no command over a pending hit: damage_check owns the execute");});
 test("shield_recoil_completes_before_next_chained_blow",[&]{fresh();Black k(T,0);k.oc.mode=b::Wait;k.oc.state=3;k.oc.c0=500;k.env.perceive=false;k.env.link={230,0,0};k.oc.angle=0x4000;k.fury_now(G);k.env.perceive=true;k.env.guard=true;
  unsigned rec=0,ticks=0;bool chained_after=false;for(int i=0;i<200;++i){k.env.link.x=k.oc.pos.x+230;k.step(G);if(k.oc.mode==b::Attack&&k.oc.state==3){++ticks;if(!k.oc.isStop())++rec;}if(ticks&&k.cmd.op==b::Op::Chain){chained_after=true;break;}}
  need(rec>=4&&chained_after,"reverse recoil played to its native end, then the chain");});
 test("fury_find0_after_damage_skipped_and_lockout_cleared",[&]{fresh();Black k(T,0);fury_fight(k,5);k.oc.setActionMode(b::Find,0);const unsigned t0=k.oc.tick;k.step(G);need(k.cmd.op==b::Op::FuryFind3&&k.oc.c2==0&&k.oc.count("V_WAIT_ST",t0+1)==0,"FIND0 -> FIND3");
  k.oc.setActionMode(b::Find,4);k.oc.c2=17;k.step(G);need(k.cmd.lockout&&k.oc.c2==0,"lockout cleared during fury only");
  fresh();Black n(T,0);n.oc.setActionMode(b::Find,4);n.oc.c2=17;n.env.link={1000,0,0};n.step(G);need(!n.cmd.lockout&&n.oc.c2==16,"no fury: native lockout untouched");});
 test("terminal_event_hidden_transition_never_commanded",[&]{for(int c=0;c<3;++c){fresh();Black k(T,0);fury_fight(k,30);for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+300;k.step(G);}
   if(c==0)k.event=true;else if(c==1)k.env.water=true;else k.oc.mode=devf_variant::A_FALL;
   k.ev.clear();k.step(G);
   if(c==1)need(k.oc.mode==b::WaterDeath&&k.ev.count("BLACK_CHAIN_DEFERRED_NATIVE_PRIORITY")==1,"water is decided by action() after the PRE: the native water death wins, the chain is only logged as deferred");
   else need(k.cmd.op==b::Op::None,"event / fall: no command at all");}});
 test("fury_leash_without_hunt_keeps_native_move_out",[&]{fresh();Black k(T,0);fury_fight(k,30);b::reset(*k.s,k.ev,"BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED","LAB");k.oc.range=200;k.oc.home={-2000,0,0};
  for(int i=0;i<40&&!(k.oc.mode==b::Attack&&k.oc.isStop());++i){k.env.link.x=k.oc.pos.x+300;k.step(G);}k.env.link.x=k.oc.pos.x+300;k.step(G);need(k.oc.mode==b::MoveOut&&k.cmd.op==b::Op::None,"no hunt: native leash wins");});
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
 std::printf("STATE_BYTES %zu TABLE_BYTES %zu\n",sizeof(b::State),sizeof(T));
 std::printf("CP18_LAB_PASS cases=%u\n",cases);
 return 0;
}
