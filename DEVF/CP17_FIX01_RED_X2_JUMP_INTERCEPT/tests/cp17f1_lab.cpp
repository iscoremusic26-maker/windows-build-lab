// CP17 FIX01 laboratory: the DLL's own red_ai_policy.hpp against source-shaped ports of the native code paths it depends on.
// Ports (not the engine): cLib_chaseF, executeFind state-4 speed/attack switch, executeAttack frame/AT-bit/SFX/translation, setCutJumpSpeed,
// Link jump integration (gravity -3.4), and a synthetic sword-swing geometry for contact coverage. No game data, no animation assets.
#include "../runtime/DEVF/CP17_FIX01_RED_X2_JUMP_INTERCEPT/expansion/red_ai_policy.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
namespace r=devf_red_ai;using V=devf_variant::Variant;
unsigned cases=0;void need(bool q,const char*why){if(!q){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
template<class F>void test(const char*n,F f){f();++cases;std::printf("CASE_PASS %s\n",n);}
struct Events{std::vector<std::string> ev,why;std::vector<r::Cause> cause;void operator()(const char*e,r::Cause c,const char*w){ev.emplace_back(e);why.emplace_back(w);cause.push_back(c);}
 unsigned count(const char*n)const{return unsigned(std::count(ev.begin(),ev.end(),n));}bool has(const char*n,const char*w)const{for(size_t i=0;i<ev.size();++i)if(ev[i]==n&&why[i]==w)return true;return false;}void clear(){ev.clear();why.clear();cause.clear();}};
r::Actor actor(unsigned id=1){r::Actor a;a.id=id;a.gen=1;a.variant=V::Red;std::memcpy(a.stage,"F_SP121",8);a.room=3;a.layer=6;a.health=330;a.ready=true;a.perceived=true;a.mode=r::Find;a.state=4;a.pos={0,0,0};a.home=a.pos;a.range=5000;a.end=40;a.angle=0x4000;return a;}
r::Link link(){r::Link l;l.id=10;l.sampled=true;l.present=l.basic=l.canary=l.safe=l.los=true;std::memcpy(l.stage,"F_SP121",8);l.room=3;l.layer=6;l.pos={300,0,0};l.motion=r::Motion::Quiet;l.proc=3;l.timer=0;l.gravity=-3.4f;l.ground_y=0;l.grounded=true;l.vel_valid=true;l.target=r::Target::ThisRed;l.heading=-0x4000;return l;}
// One Red under test with its slot table; pre/post mirror the adapter call order (PRE command applied, native execute ported by the caller, POST).
struct Lab{
 std::array<r::State,64> states{};r::Actor a=actor();r::Link l=link();Events ev;r::Command cmd;
 r::State& s(){return states[0];}
 Lab(){r::state_for(states,0,a,7);}
 r::Command pre(){cmd=r::pre(s(),a,l,ev);if(cmd.force){a.mode=r::Attack;a.state=0;}if(cmd.zero_speed)a.speed=0;if(cmd.rush)a.speed=cmd.speed;return cmd;}
 r::Rate post(){return r::post(s(),a,ev);}
 // Native ATTACK state 0 with speed 0 -> setBck B/C, play -> frame 1 (rate 1).
 r::Rate start(int clip=5){a.mode=r::Attack;a.state=0;a.b=a.c=false;a.frame=0;a.rate=1;a.speed=0;pre();a.state=clip==5?1:2;a.b=clip==5;a.c=clip==6;a.frame=1;auto x=post();if(x.write)a.rate=x.value;return x;}
 r::Rate begin_forced(int clip=5){a.state=clip==5?1:2;a.b=clip==5;a.c=clip==6;a.frame=1;a.rate=1;auto x=post();if(x.write)a.rate=x.value;return x;}
 void end(){a.mode=r::Find;a.state=0;a.b=a.c=false;a.frame=0;a.rate=1;post();a.state=4;}
 // A FIND execute; a forced start becomes the native B start in the same execute (executeAttack state 0 with speedF 0), as in the engine.
 void find_tick(){a.mode=r::Find;a.state=4;pre();if(cmd.force){begin_forced();return;}post();}
 // One own execute of a running blow: PRE, native AT bit on the pre-play frame, play at the current rate, POST (which may write the rate).
 r::Rate tick_blow(){pre();a.at_set=a.frame>=14&&a.frame<=22;a.frame+=a.rate;if(a.frame>=a.end){a.frame=a.end;a.stopped=true;}auto x=post();if(x.write)a.rate=x.value;return x;}
 // The x2 decision taken at frame 1 is applied at frame 2: returns that second POST.
 r::Rate apply2(){return tick_blow();}
 // Plays an owned blow to its end (rate as decided) with the native AT bit window, then returns to FIND.
 void finish(){for(int i=0;i<80&&r::blow(a);++i){tick_blow();if(a.stopped){a.stopped=false;end();}}}
};
// ---- native ports ----
struct FindPort{float speed=0;int lockout=0;bool attack=false;};
// executeFind case 4 (no event, no watch, floor ok): chase target from distance/lockout, then the attack switch, then fopAcM_posMoveF uses speedF.
inline void find_state4(FindPort&f,double dist,bool facing){
 if(f.lockout==0){f.speed=r::chase(f.speed,dist<300?-3.f:20.f,1.f);if(dist<400&&dist>200&&facing)f.attack=true;}
 else{if(dist<400&&dist>200)f.speed=r::chase(f.speed,0.f,1.f);else if(dist<300)f.speed=r::chase(f.speed,-3.f,1.f);else f.speed=r::chase(f.speed,20.f,1.f);}
}
struct NativeTrace{unsigned voice10=0,sound125=0,sound19=0,voice7=0,sound115=0,active=0;float push=0;std::vector<float> active_pre,poses;};
constexpr float tb[]={0,0,0,0,15.3f,38.4f,63.3f,86.2f,103.3f,112};constexpr float tc[]={0,31.3f,72.7f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f};
// executeAttack excerpt: sounds by else-if checkFrame on the pre-play frame, AT bit 14..22 on the pre-play frame, cumulative translation table, then play.
// Frames as the policy drives them: frame 1 after the native start, x1 to frame 2 (x2 decided at 1, applied at 2), then `rate`.
// `rate` 1.15 is applied at frame 1 (normal blows); `phase` shifts the start for the robustness sweep.
NativeTrace trace(int clip,float rate,float phase=1,bool even=true){NativeTrace t;float old=0;for(float f=phase;f<35;f+=(even&&rate==2.0f&&f<phase+1)?1.0f:rate){
 if(clip==5){if(r::pass(f,rate,40,10))++t.voice10;else if(r::pass(f,rate,40,12.5f))++t.sound125;else if(r::pass(f,rate,40,19))++t.sound19;}
 else{if(r::pass(f,rate,40,7))++t.voice7;else if(r::pass(f,rate,40,11.5f))++t.sound115;}
 if(f>=14&&f<=22){++t.active;t.active_pre.push_back(f);t.poses.push_back(f+rate);}int i=int(f-9);float next=i>=0?(clip==5?tb[std::min(i,9)]:tc[std::min(i,9)]):0;t.push+=next-old;old=next;}
 return t;}
// setCutJumpSpeed port (d_a_alink_cut.inc) with a target eye 150 above the Red; returns the horizontal speed.
float cut_jump_speed(r::Point link,float vy,float g,r::Point red,bool targeted){
 if(!targeted)return 0;const double ang=std::atan2(double(red.x)-link.x,double(red.z)-link.z);r::Point eye{red.x,red.y+150,red.z};
 r::Point tgt{float(eye.x-std::sin(ang)*70.0),red.y,float(eye.z-std::cos(ang)*70.0)};
 const float dy=vy+g,ig=1.0f/g;float f1=tgt.y-(link.y-(ig*(dy*dy)*0.5f));if(f1>0)f1=0;const float div=std::sqrt(f1*2.0f*ig)-dy*ig;
 float dist=float(std::sqrt(r::xz2(link,tgt)));if(dist>500)dist=500;return dist/div;
}
// Link jump simulation: PROC26 (V27) takes off immediately, PROC30 (V33) after 5 grounded start frames. Returns per-tick samples.
struct LinkTick{r::Point pos;float vy;bool grounded;unsigned proc;};
std::vector<LinkTick> jump_path(r::Point start,r::Point red,bool strike,bool targeted=true){
 std::vector<LinkTick> v;r::Point p=start;const float g=-3.4f;float vy=0,h=0;bool air=false;const double ang=std::atan2(double(red.x)-start.x,double(red.z)-start.z);
 for(int k=0;k<80;++k){
  if(!air&&(!strike||k>=5)){vy=strike?33.f:27.f;h=cut_jump_speed(p,vy,g,red,targeted);if(!targeted)h=strike?20.f:25.f;air=true;}
  if(air){vy+=g;if(vy<-60)vy=-60;p.y+=vy;p.x+=float(std::sin(ang)*h);p.z+=float(std::cos(ang)*h);if(p.y<=0&&vy<0){p.y=0;v.push_back({p,vy,true,strike?r::kProcLargeJumpLand:r::kProcCutJumpLand});break;}}
  v.push_back({p,air?vy:0.f,!air,strike?r::kProcLargeJump:r::kProcCutJump});
 }
 return v;
}
// Drives one Red through a Link jump; returns the tick of the forced start (or -1) and the landing tick.
struct Intercept{int forced=-1,land=-1;unsigned forced_count=0;r::Cause cause=r::Cause::Normal;double contact_dist=1e9,contact_y=0,window_max_y=-1e9,window_max_vy=-1e9;Events ev;};
Intercept run_jump(r::Point start,bool strike,int lockout=0,bool targeted=true,int red_state=4,float red_speed=0){
 Lab L;L.a.lockout=lockout;L.a.speed=red_speed;L.a.state=red_state;L.l.pos=start;L.l.target=targeted?r::Target::ThisRed:r::Target::None;L.l.heading=r::yaw(start,L.a.pos);
 L.a.angle=r::yaw(L.a.pos,start);
 // two calm samples so FIND state 4 is established and the velocity sample is consecutive
 L.l.proc=3;L.find_tick();L.find_tick();
 auto path=jump_path(start,L.a.pos,strike,targeted);Intercept out;r::Point prev=start;
 for(size_t k=0;k<path.size();++k){
  auto& t=path[k];L.l.vel={t.pos.x-prev.x,t.vy,t.pos.z-prev.z};prev=t.pos;L.l.pos=t.pos;L.l.grounded=t.grounded;L.l.proc=int(t.proc);
  L.l.jump=r::jump_view(t.proc,t.proc==r::kProcCutJump,false);L.l.motion=r::Motion::Offensive;
  if(t.proc==r::kProcCutJumpLand||t.proc==r::kProcLargeJumpLand){out.land=int(k);}
  auto c=L.pre();
  if(c.force){++out.forced_count;out.forced=int(k);out.cause=c.cause;L.begin_forced();
   // where Link really is during the x2 active ticks t+8..t+11 (path ground truth, not the plan)
   for(size_t j=r::kOpen;j<=size_t(r::kClose)&&k+j<path.size();++j){const double dxz=std::sqrt(r::xz2(L.a.pos,path[k+j].pos)),h=path[k+j].pos.y-L.a.pos.y;out.window_max_y=std::max(out.window_max_y,h);out.window_max_vy=std::max(out.window_max_vy,double(path[k+j].vy));if(h<=r::kReachY&&dxz<out.contact_dist){out.contact_dist=dxz;out.contact_y=h;}}
  }else if(r::blow(L.a)){L.a.at_set=L.a.frame>=14&&L.a.frame<=22;L.a.frame+=L.a.rate;auto x=L.post();if(x.write)L.a.rate=x.value;}
  else L.post();
  if(out.land>=0)break;
 }
 out.ev=L.ev;return out;
}
// ---- contact geometry model ----
struct Pose{r::Point tip,hilt;};
// Synthetic swing: hand on an arc (radius arm) around a shoulder at height sh, blade along the radial direction tilted by pitch; continuous in frame f.
struct Swing{double yaw0,yaw1,pitch0,pitch1,arm,sh,blade;Pose at(double f)const{const double u=std::clamp((f-13.0)/11.0,0.0,1.0);const double s=u*u*(3-2*u);
 const double y=yaw0+(yaw1-yaw0)*s,p=pitch0+(pitch1-pitch0)*s;const double cx=std::cos(p)*std::sin(y),cy=std::sin(p),cz=std::cos(p)*std::cos(y);
 r::Point hand{float(arm*cx),float(sh+arm*cy),float(arm*cz)};auto along=[&](double d){return r::Point{float(hand.x+d*cx),float(hand.y+d*cy),float(hand.z+d*cz)};};
 return {along(70*blade),along(20*blade)};}};
bool hits(r::Point c,float rad,r::Point link){const double h=std::sqrt(r::xz2(c,link));const double dh=std::max(0.0,h-r::kLinkR);const double y=std::clamp(double(c.y),double(link.y),double(link.y)+180.0);const double dy=y-c.y;return dh*dh+dy*dy<=double(rad)*rad;}
struct Coverage{unsigned native=0,odd=0,even=0,sweep=0,miss_odd=0,miss_even=0,miss_sweep=0,phantom_sweep=0,phantom_even=0,cells=0;double depth=0;};
// truth: the native spheres swept continuously over poses 15..23 (1/16 frame); native: discrete x1 poses 15..23; odd: plain x2 applied at frame 1
// (poses 17..23); even: plain x2 applied at frame 2 (poses 16..24); sweep: even phase with the policy's sweep() placing the spheres per tick,
// Link standing on the cell. Phantom depth = distance from a phantom cell to the nearest truth cell (1 unit search).
Coverage coverage(const Swing& w,int step){
 Coverage cv;
 auto pose_hit=[&](double f,r::Point g){auto p=w.at(f);return hits(p.tip,r::kTipR,g)||hits(p.hilt,r::kHiltR,g);};
 auto truth_at=[&](r::Point g){for(int q=15*16;q<=23*16;++q)if(pose_hit(q/16.0,g))return true;return false;};
 for(int x=-330;x<=330;x+=step)for(int z=-120;z<=380;z+=step){r::Point g{float(x),0.f,float(z)};++cv.cells;
  bool native=false,odd=false,even=false,sw=false;
  for(int f=15;f<=23;++f)native|=pose_hit(f,g);
  for(int f=17;f<=23;f+=2)odd|=pose_hit(f,g);
  for(int f=16;f<=24;f+=2)even|=pose_hit(f,g);
  r::State s;s.decided=true;s.owned=true;s.own_rate=r::kBurst;
  r::Actor a=actor();a.mode=r::Attack;a.state=1;a.b=true;a.rate=r::kBurst;a.tip_r=r::kTipR;a.hilt_r=r::kHiltR;r::Link l=link();l.pos=g;
  for(float pre=10;pre<=22;pre+=2){auto p=w.at(pre+2);a.frame=pre+2;a.tip=p.tip;a.hilt=p.hilt;a.at_set=pre>=14&&pre<=22;auto out=r::sweep(s,a,l);
   if(!a.at_set)continue;
   sw|=out.write0?hits(out.c0,out.r0,g):hits(p.tip,r::kTipR,g);
   sw|=out.write1?hits(out.c1,out.r1,g):hits(p.hilt,r::kHiltR,g);}
  const bool truth=(native||sw||even)?truth_at(g):false;
  cv.native+=native;cv.odd+=odd;cv.even+=even;cv.sweep+=sw;cv.miss_odd+=native&&!odd;cv.miss_even+=native&&!even;cv.miss_sweep+=native&&!sw;cv.phantom_even+=even&&!truth;
  if(sw&&!truth){++cv.phantom_sweep;double best=1e9;for(int dx=-20;dx<=20;++dx)for(int dz=-20;dz<=20;++dz)if(dx*dx+dz*dz<best*best&&truth_at({g.x+dx,0.f,g.z+dz}))best=std::sqrt(double(dx*dx+dz*dz));cv.depth=std::max(cv.depth,best);}
 }
 return cv;
}
std::vector<Swing> swings(){
 std::vector<Swing> v={{-1.2,1.2,-0.1,-0.1,60,110,1.0},{-1.6,1.4,0.2,-0.3,70,120,1.0},{0.2,0.2,1.3,-0.6,55,130,1.0},{-0.9,0.9,0.6,-0.5,65,115,1.1},{1.3,-1.3,0.0,-0.2,75,105,0.9},{-2.0,1.8,0.1,-0.1,60,110,1.0}};
 std::uint32_t x=12345;auto u=[&]{x^=x<<13;x^=x>>17;x^=x<<5;return (x%100000)/100000.0;};
 for(int i=0;i<12;++i){double y0=-2.2+u()*1.6,y1=0.2+u()*2.0;if(u()<.5)std::swap(y0,y1);v.push_back({y0,y1,-0.6+u()*1.6,-0.6+u()*1.0,50+u()*30,100+u()*40,0.9+u()*0.25});}
 return v;
}
int main(){
 test("constants_x2_exact_normal_115_rush_15_no_x3",[]{need(r::kBurst==2.0f&&r::kNormal==1.15f&&r::kRush==1.5f,"rates");need(r::kRandomPercent==12&&r::kPassive==105&&r::kIntent==90,"cadence");need(r::kOpen==8&&r::kClose==12,"window");});
 test("crt_free_root_and_arctan2_exact_enough",[]{double worst=0;for(double x=1e-6;x<1e15;x*=1.37){const double a=r::root(x),b=std::sqrt(x);need(std::fabs(a-b)<=1e-12*b+1e-15,"root");}
  for(int i=0;i<20000;++i){const double ang=(i-10000)*0.000314159,y=std::sin(ang)*(1+i%7),x=std::cos(ang)*(1+i%7);worst=std::max(worst,std::fabs(r::arctan2(y,x)-std::atan2(y,x)));}need(worst<2.5e-4,"atan2 error below 2.5e-4 rad");
  need(r::yaw({0,0,0},{1,0,0})==0x4000&&r::yaw({0,0,0},{0,0,1})==0&&r::yaw({0,0,0},{-1,0,0})==-0x4000&&(std::abs(r::yaw({0,0,0},{0,0,-1}))==0x8000||r::yaw({0,0,0},{0,0,-1})==-0x8000),"s16 yaw convention (cM_atan2s(dx,dz))");std::printf("ATAN2_MAX_ERR_RAD %.2e\n",worst);});
 test("fresh_blow_normal_115_and_random_x2_once",[]{Lab L;L.s().next_random=100000;auto x=L.start();need(x.write&&x.value==r::kNormal&&x.cause==r::Cause::Normal,"normal");L.finish();
  Lab R;R.s().combat=true;R.l.motion=r::Motion::Offensive;unsigned bursts=0;for(int i=0;i<400;++i){R.find_tick();auto y=R.start();if(!y.write&&R.s().pending){auto z=R.apply2();need(z.write&&z.value==r::kBurst&&z.cause==r::Cause::Random&&R.a.frame==2,"random x2 at frame 2");++bursts;}else need(y.value==r::kNormal,"normal at frame1");R.finish();}need(bursts>0,"random reachable");});
 test("x2_even_phase_window_ticks_8_to_12",[]{auto t=trace(5,2.0f);need(t.active==5,"5 active ticks");need(t.active_pre==std::vector<float>({14,16,18,20,22}),"pre frames");need(t.poses==std::vector<float>({16,18,20,22,24}),"poses");
  auto o=trace(5,2.0f,1,false);need(o.active==4&&o.active_pre==std::vector<float>({15,17,19,21}),"odd phase (decision applied at frame1) only 4");
  for(int k=2;k<=15;++k){const float f=float(2*(k-1));const bool on=f>=14&&f<=22;need(on==(k>=r::kOpen&&k<=r::kClose),"k window");}
  Lab L;L.s().combat=true;L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.find_tick();auto c=L.pre();need(c.force,"forced");L.begin_forced();int first=-1,last=-1;
  for(int k=1;k<=20&&r::blow(L.a);++k){L.tick_blow();if(L.a.at_set){if(first<0)first=k;last=k;}}need(first==r::kOpen&&last==r::kClose,"policy-driven blow active on t+8..t+12");});
 test("x2_no_lost_native_sfx_any_phase_b_and_c",[]{for(int i=0;i<200;++i){const float ph=1+i*0.01f;auto b=trace(5,2.0f,ph),c=trace(6,2.0f,ph);need(b.voice10==1&&b.sound125==1&&b.sound19==1,"B else-if complete at x2");need(c.voice7==1&&c.sound115==1,"C complete");}
  auto n=trace(5,1.15f);need(n.voice10==1&&n.sound125==1&&n.sound19==1,"x1.15 complete");
  for(int i=0;i<200;++i){const float ph=1+i*0.01f;auto b=trace(5,2.0f,ph,false),c=trace(6,2.0f,ph,false);need(b.voice10==1&&b.sound125==1&&b.sound19==1&&c.voice7==1&&c.sound115==1,"odd phase also complete");}});
 test("historical_x3_lost_b_sfx_documented_only",[]{auto t=trace(5,3.0f,1,false);need(t.voice10==1&&t.sound125==0,"0.14.0 x3 defect reproduced (history, not a rate used)");});
 test("x2_translation_total_preserved",[]{need(std::fabs(trace(5,2.0f).push-112)<1e-3&&std::fabs(trace(6,2.0f).push-91.5f)<1e-3&&std::fabs(trace(5,1.15f).push-112)<1e-3,"cumulative trans");});
 test("foreign_rate_recoil_and_stop_never_overwritten",[]{Lab L;L.a.mode=r::Attack;L.a.state=0;L.pre();L.a.state=1;L.a.b=true;L.a.frame=1;L.a.rate=1.3f;auto x=L.post();need(!x.write,"foreign");L.end();
  Lab R;R.start();R.a.state=3;R.a.rate=-1;R.a.frame=5;auto y=R.post();need(!y.write,"recoil not written");});
 test("x2_pending_invalidated_by_native_change_never_written",[]{Lab L;L.s().combat=true;L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.find_tick();L.pre();L.begin_forced();
  L.pre();L.a.state=3;L.a.rate=-1;L.a.frame=0.5f;auto x=L.post();need(!x.write&&L.ev.has("RATE_SKIPPED","X2_DECIDED_AT_FRAME1_BUT_NATIVE_CLIP_RATE_OR_FRAME_CHANGED_BEFORE_FRAME2"),"no write after native change");
  Lab M;M.s().combat=true;M.l.charge=r::Charge::Spin;M.l.proc=0x2a;M.l.pos={250,0,0};M.a.angle=r::yaw(M.a.pos,M.l.pos);M.find_tick();M.pre();M.begin_forced();M.pre();M.a.rate=1.3f;M.a.frame=2.3f;need(!M.post().write,"foreign rate before frame2");});
 test("owned_rate_restored_only_while_same_clip",[]{Lab L;L.start();L.a.frame=3;L.a.rate=r::kNormal;L.post();L.a.mode=r::Find;L.a.state=0;L.a.b=true;L.a.frame=5;auto x=L.post();need(x.write&&x.value==1,"restore owned");
  Lab R;R.start();R.a.b=false;R.a.c=true;R.a.mode=r::Find;auto y=R.post();need(!y.write,"other clip untouched");});
 test("charge_every_eligible_episode_no_roll",[]{for(unsigned seed=1;seed<6;++seed){Lab L;r::seed(L.s(),seed,1,seed);unsigned sel=0;
  for(int ep=0;ep<12;++ep){L.l.pos={600,0,0};L.l.proc=0x2a;L.l.charge=r::Charge::Spin;L.ev.clear();L.find_tick();sel+=L.ev.count("REACTION_SELECTED");
   L.l.pos={300,0,0};L.l.charge=r::Charge::None;L.l.proc=3;for(int i=0;i<r::kReactiveCooldown+5;++i)L.find_tick();L.s().next_reactive=0;}
  need(sel==12,"every episode selected regardless of seed");}});
 test("charge_reevaluated_until_eligible_not_consumed_by_failed_tick",[]{Lab L;L.l.charge=r::Charge::Jump;L.l.proc=0x2a;L.l.los=false;L.find_tick();need(L.ev.count("REACTION_SELECTED")==0,"no LOS");
  L.l.los=true;L.find_tick();need(L.ev.count("REACTION_SELECTED")==1,"selected next tick");
  Lab R;R.s().next_reactive=R.s().ticks+3;R.l.charge=r::Charge::Spin;R.l.proc=0x2a;R.find_tick();need(R.ev.count("CHARGE_REACTION_DEFERRED")==1,"deferred");for(int i=0;i<4;++i)R.find_tick();need(R.ev.count("REACTION_SELECTED")==1&&R.ev.count("CHARGE_REACTION_DEFERRED")==1,"selected once cooldown ends, deferral logged once");
  Lab F;F.l.charge=r::Charge::Spin;F.l.proc=0x2a;F.l.pos={1200,0,0};F.find_tick();need(F.ev.has("CHARGE_REACTION_DEFERRED","OUTSIDE_900_REACTION_RADIUS"),"radius");F.l.pos={700,0,0};F.find_tick();need(F.ev.count("REACTION_SELECTED")==1,"radius re-evaluated");});
 test("reactive_and_random_cooldowns_independent",[]{Lab L;L.s().combat=true;L.s().next_random=L.s().ticks+500;L.s().normal_since_random=false;L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();need(L.ev.count("REACTION_SELECTED")==1,"random cooldown does not block charge");
  Lab R;R.l.motion=r::Motion::Offensive;R.s().next_reactive=R.s().ticks+500;R.s().combat=true;R.s().next_random=0;R.s().normal_since_random=true;unsigned rnd=0;for(int i=0;i<200;++i){R.find_tick();R.start();rnd+=R.s().pending&&R.s().blow_cause==r::Cause::Random;R.finish();R.s().next_reactive=R.s().ticks+500;}need(rnd>0,"reactive cooldown does not block random");});
 test("charge_intent_forces_native_attack_in_range_and_x2",[]{Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.ev.clear();auto c=L.pre();
  need(c.force&&c.cause==r::Cause::ChargeSpin,"forced");auto x0=L.begin_forced();need(!x0.write&&L.s().pending,"decided at frame1");auto x=L.apply2();need(x.write&&x.value==r::kBurst&&x.cause==r::Cause::ChargeSpin&&L.a.frame==2,"x2 charge at frame2");need(L.s().episode_reacted,"episode consumed by attempt");
  L.finish();L.l.proc=0x2a;for(int i=0;i<100;++i)L.find_tick();need(L.ev.count("REACTION_SELECTED")==0,"same episode never reacts twice");});
 test("charge_point_blank_under_200_forced_where_native_backs_off",[]{Lab L;L.l.pos={120,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Jump;L.l.proc=0x2f;L.find_tick();auto c=L.pre();need(c.force&&c.cause==r::Cause::ChargeJump,"forced <200");});
 test("charge_force_respects_lockout_facing_find4_and_native_clips",[]{
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.lockout=7;L.find_tick();auto c=L.pre();need(!c.force,"lockout");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos)+0x1400;L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();auto c=L.pre();need(!c.force,"facing");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.state=3;L.pre();L.post();auto c=L.pre();need(!c.force,"FIND state3 timers");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.mode=r::FindStay;L.a.state=0;L.pre();L.post();auto c=L.pre();need(!c.force,"FIND_STAY ledge");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.hit=true;auto c=L.pre();need(!c.force,"pending damage");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.cutdown=true;auto c=L.pre();need(!c.force,"cut down");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.ready=false;auto c=L.pre();need(!c.force,"no ground/event");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.incoming=true;auto c=L.pre();need(!c.force,"recruit order");}
  {Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.range=300;L.a.pos={150,0,0};L.l.pos={350,0,0};L.find_tick();auto c=L.pre();need(!c.force,"leash margin");}
 });
 test("rush_exact_x1_5_displacement_native_speed_trajectory_restored",[]{Lab L;L.l.pos={900,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();
  FindPort native;native.speed=0;double moved_native=0,moved_rush=0;std::vector<float> nat,obs;
  for(int k=0;k<12;++k){const double dist=r::d3(L.a.pos,L.l.pos);L.a.speed=L.a.speed;auto c=L.pre();need(c.rush,"rush each approach tick");
   FindPort f;f.speed=L.a.speed;find_state4(f,dist,true);L.a.speed=f.speed;moved_rush+=f.speed;   // native chase from the written value, posMoveF uses it
   find_state4(native,dist,true);moved_native+=native.speed;
   auto rp=r::rush_post(L.s(),L.a,L.ev);need(rp.write,"restore");L.a.speed=rp.value;need(std::fabs(L.a.speed-native.speed)<1e-5,"speed variable equals native trajectory");
   L.a.pos.x+=f.speed;L.post();}
  need(std::fabs(moved_rush-1.5*moved_native)<1e-3,"exact 1.5 displacement");});
 test("rush_never_when_native_backs_off_or_lockout_or_leash",[]{
  {Lab L;L.l.pos={290,0,0};L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.angle=r::yaw(L.a.pos,L.l.pos)+0x3000;L.find_tick();auto c=L.pre();need(!c.rush&&!c.force,"<300 native retreats");}
  {Lab L;L.l.pos={380,0,0};L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.angle=r::yaw(L.a.pos,L.l.pos)+0x3000;L.a.lockout=5;L.find_tick();auto c=L.pre();need(!c.rush,"lockout 0 target in 200..400");}
  {Lab L;L.l.pos={700,0,0};L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.a.range=230;L.find_tick();auto c=L.pre();need(!c.rush,"leash slack");}
  {Lab L;L.l.pos={700,0,0};L.l.charge=r::Charge::None;L.l.proc=3;L.find_tick();auto c=L.pre();need(!c.rush,"no reaction no rush");}
  {Lab L;L.l.pos={700,0,0};L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.speed=-3;auto c=L.pre();need(!c.rush,"negative native speed never boosted");}});
 test("rush_preempted_and_foreign_speed_write",[]{Lab L;L.l.pos={900,0,0};L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.speed=10;auto c=L.pre();need(c.rush,"rush");
  auto p=r::rush_post(L.s(),L.a,L.ev);need(p.write&&p.value==10,"WATCH before chase restores pre value");L.post();
  c=L.pre();need(c.rush,"rush again");L.a.speed=0;auto q=r::rush_post(L.s(),L.a,L.ev);need(!q.write&&L.ev.count("RUSH_RELEASED")==1,"native zero kept");});
 test("rush_measured_logged_on_blow_or_cancel",[]{Lab L;L.l.pos={900,0,0};L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();for(int i=0;i<3;++i){L.pre();FindPort f{L.a.speed};find_state4(f,900,true);L.a.speed=f.speed;auto p=r::rush_post(L.s(),L.a,L.ev);L.a.speed=p.value;L.post();}
  L.l.charge=r::Charge::None;L.l.proc=3;L.find_tick();need(L.ev.count("RUSH_MEASURED")==1,"measured on cancel");});
 // ---- interception ----
 test("jump_view_exactly_two_families_and_negatives",[]{need(r::jump_view(0x26,true,false)==r::Jump::JumpAttack&&r::jump_view(0x30,false,false)==r::Jump::JumpStrike,"positives");
  for(unsigned p:{0x2du,0x2eu,0x2bu,0x2cu,0x23u,0x24u,0x27u,0x31u,0x28u,0x29u,0x2au,0x2fu,0x18u,0x19u,0x1au,0x25u,0x22u,0x0u,0x3u})need(r::jump_view(p,false,false)==r::Jump::None,"negative proc");
  need(r::jump_view(0x26,false,false)==r::Jump::None&&r::jump_view(0x2d,true,false)==r::Jump::None&&r::jump_view(0x30,false,true)==r::Jump::None,"canary contradictions rejected");});
 test("jump_attack_intercepted_before_landing_all_distances",[]{for(float d=180;d<=520;d+=20){auto x=run_jump({d,0,0},false);std::printf("TIMING jump_attack start=%.0f forced_tick=%d first_active=%d land_tick=%d contact_xz=%.0f contact_feet_y=%.0f\n",d,x.forced,x.forced+r::kOpen,x.land,x.contact_dist,x.contact_y);
   need(x.forced_count==1&&x.cause==r::Cause::InterceptJumpAttack,"one forced interception");need(x.window_max_y<=110.0,"independent reference: Link's feet <= 110 on every active tick");need(x.land<0||x.forced+r::kOpen<x.land,"active window opens before landing");need(x.contact_dist<=r::kReachXZ+1,"Link inside modelled reach (XZ and feet height) during active ticks");}});
 test("jump_strike_waits_takeoff_then_intercepts_before_landing",[]{for(float d=180;d<=520;d+=20){auto x=run_jump({0,0,d},true);std::printf("TIMING jump_strike start=%.0f forced_tick=%d first_active=%d land_tick=%d contact_xz=%.0f contact_feet_y=%.0f\n",d,x.forced,x.forced+r::kOpen,x.land,x.contact_dist,x.contact_y);
   need(x.forced_count==1&&x.cause==r::Cause::InterceptJumpStrike,"one strike interception");need(x.forced>=5,"not before take-off");need(x.window_max_vy<0&&x.window_max_y<=110.0,"independent reference: all active ticks in the descent with Link's feet <= 110");need(x.land<0||x.forced+r::kOpen<x.land,"before landing");need(x.contact_dist<=r::kReachXZ+1,"reach (XZ and feet height)");}});
 test("intercept_bypasses_lockout_and_runs_from_attack0",[]{auto x=run_jump({300,0,0},false,9);need(x.forced_count==1,"lockout bypassed for interception");
  Lab L;L.a.mode=r::Attack;L.a.state=0;L.a.speed=12;L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.grounded=false;L.l.vel={-15,20,0};L.l.pos={260,40,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.s().combat=true;auto c=L.pre();need(c.zero_speed&&!c.force,"ATTACK0 only stops the native deceleration");});
 test("intercept_missed_not_enough_time_logged_never_faked",[]{Lab L;L.find_tick();L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.grounded=false;L.l.pos={150,20,0};L.l.vel={-10,-15,0};auto c=L.pre();need(!c.force,"no start");need(L.ev.count("INTERCEPT_MISSED_NOT_ENOUGH_TIME")==1,"miss logged");
  L.post();L.pre();need(L.ev.count("INTERCEPT_MISSED_NOT_ENOUGH_TIME")==1,"once per episode");});
 test("intercept_not_directed_or_targeting_other",[]{auto x=run_jump({300,0,0},false,0,false);need(x.forced_count==1,"untargeted but heading at Red");
  Lab L;L.find_tick();L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.target=r::Target::Other;L.l.grounded=false;L.l.vel={-15,20,0};auto c=L.pre();need(!c.force&&L.ev.has("INTERCEPT_SKIPPED","LINK_TARGETS_ANOTHER_ACTOR"),"other target");
  Lab M;M.find_tick();M.l.jump=r::Jump::JumpAttack;M.l.proc=0x26;M.l.target=r::Target::None;M.l.heading=0x4000;M.l.grounded=false;M.l.vel={15,20,0};auto d=M.pre();need(!d.force&&M.ev.has("INTERCEPT_SKIPPED","NOT_DIRECTED_AT_THIS_RED"),"heading away");});
 test("intercept_moving_away_waits_and_never_starts",[]{Lab L;L.find_tick();L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.grounded=false;L.l.pos={600,0,0};L.l.vel={12,25,0};L.l.target=r::Target::ThisRed;bool f=false;for(int k=0;k<10;++k){L.l.pos.x+=12;L.l.vel.y-=3.4f;L.l.pos.y+=L.l.vel.y;f|=L.pre().force;L.post();}need(!f,"retreating trajectory");});
 test("intercept_skipped_while_own_blow_runs_no_retiming",[]{Lab L;L.start();L.a.frame=7;L.a.rate=r::kNormal;L.post();L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.grounded=false;L.l.vel={-15,20,0};auto c=L.pre();need(!c.force&&L.ev.has("INTERCEPT_SKIPPED","OWN_NATIVE_BLOW_ALREADY_RUNNING_NO_RETIMING"),"skip");auto x=L.post();need(!x.write,"no retime");});
 test("charge_then_jump_strike_same_episode_single_reaction",[]{
  Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Jump;L.l.proc=0x2a;L.find_tick();auto c=L.pre();need(c.force,"charge reaction attempted");L.begin_forced();
  L.l.charge=r::Charge::None;L.l.proc=0x30;L.l.jump=r::Jump::JumpStrike;L.l.grounded=true;auto d=L.pre();need(!d.force&&L.ev.has("INTERCEPT_SKIPPED","SAME_JUMP_STRIKE_EPISODE_ALREADY_REACTED_TO_CHARGE"),"no double reaction");
  Lab M;M.l.pos={900,0,0};M.l.charge=r::Charge::Jump;M.l.proc=0x2a;M.find_tick();need(M.s().intent,"pending charge intent");M.l.charge=r::Charge::None;M.l.proc=0x30;M.l.jump=r::Jump::JumpStrike;M.find_tick();
  need(M.ev.count("REACTION_UPGRADED")==1&&!M.s().intent,"pending intent upgraded to interception");});
 test("forced_start_preempted_by_native_keeps_episode_open",[]{Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();auto c=L.pre();need(c.force,"forced");
  L.a.mode=devf_variant::A_WATER_DEATH;L.a.state=0;L.post();need(L.ev.count("NATIVE_ATTACK_FORCED_PREEMPTED")==1&&!L.s().episode_reacted,"not consumed");
  L.a.mode=r::Find;L.a.state=4;L.a.speed=0;L.find_tick();need(L.ev.count("NATIVE_ATTACK_FORCED")==2&&L.s().episode_reacted&&L.s().decided,"re-armed and started in the same episode");});
 test("one_interception_per_episode",[]{auto x=run_jump({320,0,0},false);need(x.forced_count==1&&x.ev.count("INTERCEPT_STARTED")==1&&x.ev.count("JUMP_ATTACK_DETECTED")==1,"single");});
 test("interception_priority_over_charge_and_passivity",[]{Lab L;L.s().combat=true;L.l.motion=r::Motion::Guard;L.l.pos={400,0,0};for(int i=0;i<110;++i)L.find_tick();L.s().next_passive=0;L.find_tick();need(L.s().intent&&L.s().intent_cause==r::Cause::Passive,"passive intent");
  L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();need(L.s().intent_cause==r::Cause::ChargeSpin,"charge replaces passive");
  auto x=run_jump({300,0,0},false);need(x.cause==r::Cause::InterceptJumpAttack,"intercept cause");});
 test("passivity_105_with_reactive_cooldown",[]{Lab L;L.s().combat=true;L.l.motion=r::Motion::Guard;L.l.pos={400,0,0};for(int i=0;i<104;++i)L.find_tick();need(!L.s().intent,"not before 105");L.find_tick();need(L.s().intent&&L.s().intent_cause==r::Cause::Passive,"105 reached");
  Lab M;M.s().combat=true;M.s().next_reactive=M.s().ticks+1000;M.l.motion=r::Motion::Guard;M.l.pos={400,0,0};for(int i=0;i<200;++i)M.find_tick();need(!M.s().intent,"reactive cooldown");});
 test("random_cooldown_and_normal_between_bursts",[]{Lab L;L.s().combat=true;L.l.motion=r::Motion::Offensive;unsigned prev=0;bool consecutive=false;unsigned last_random_tick=0,min_gap=100000;
  for(int i=0;i<600;++i){L.find_tick();L.start();const bool rnd=L.s().pending&&L.s().blow_cause==r::Cause::Random;if(rnd){if(prev)consecutive=true;if(last_random_tick)min_gap=std::min(min_gap,L.s().ticks-last_random_tick);last_random_tick=L.s().ticks;}prev=rnd;L.finish();}
  need(!consecutive,"a normal blow between random bursts");need(min_gap>=r::kRandomMin,"random cooldown 120..210 own ticks");});
 test("intercept_waits_while_red_busy_then_starts_or_misses_with_reason",[]{
  {Lab L;L.find_tick();L.a.state=3;auto path=jump_path({300,0,0},L.a.pos,false);r::Point prev={300,0,0};int forced=-1;
   for(size_t k=0;k<path.size();++k){auto& tk=path[k];L.l.vel={tk.pos.x-prev.x,tk.vy,tk.pos.z-prev.z};prev=tk.pos;L.l.pos=tk.pos;L.l.grounded=tk.grounded;L.l.proc=int(tk.proc);L.l.jump=r::jump_view(tk.proc,tk.proc==r::kProcCutJump,false);
    if(k==2){L.a.state=4;}auto c=L.pre();if(c.force&&forced<0){forced=int(k);L.begin_forced();}else L.post();if(tk.proc==r::kProcCutJumpLand)break;}
   need(forced==3,"FIND state 4 established one tick later, then started");need(L.ev.count("INTERCEPT_WAITING")==1,"waiting logged once");}
  {Lab L;L.find_tick();L.a.state=3;auto path=jump_path({300,0,0},L.a.pos,false);r::Point prev={300,0,0};bool forced=false;
   for(size_t k=0;k<path.size();++k){auto& tk=path[k];L.l.vel={tk.pos.x-prev.x,tk.vy,tk.pos.z-prev.z};prev=tk.pos;L.l.pos=tk.pos;L.l.grounded=tk.grounded;L.l.proc=int(tk.proc);L.l.jump=r::jump_view(tk.proc,tk.proc==r::kProcCutJump,false);
    forced|=L.pre().force;L.post();if(tk.proc==r::kProcCutJumpLand)break;}
   need(!forced&&L.ev.count("INTERCEPT_MISSED_NOT_ENOUGH_TIME")==1&&L.ev.has("INTERCEPT_MISSED_NOT_ENOUGH_TIME","RED_NOT_IN_NATIVE_FIND_APPROACH"),"miss with the blocking reason, never faked");}});
 test("rush_restored_when_native_find_switches_to_attack_after_chase",[]{Lab L;L.l.pos={330,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos)+0x1200;L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.a.speed=10;auto c=L.pre();need(c.rush,"rush at 330 while not yet facing");
  FindPort f{L.a.speed};find_state4(f,330,true);L.a.speed=f.speed;L.a.mode=r::Attack;L.a.state=0;auto rp=r::rush_post(L.s(),L.a,L.ev);need(rp.write&&rp.value==11,"native speed value restored for ATTACK state 0 deceleration");});
 test("wolf_mount_demo_unsafe_link_never_intercepted",[]{for(int k=0;k<3;++k){Lab L;L.find_tick();L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.grounded=false;L.l.vel={-15,20,0};L.l.safe=false;auto c=L.pre();need(!c.force,"unsafe Link (wolf/ride/demo/event/damage timer)");}
  Lab M;M.find_tick();M.l.jump=r::Jump::JumpAttack;M.l.proc=0x26;M.l.grounded=false;M.l.vel={-15,20,0};M.l.vel_valid=false;auto d=M.pre();need(!d.force,"no trajectory without two consecutive samples");});
 test("one_rate_decision_per_blow_no_mid_attack_write",[]{Lab L;L.s().combat=true;L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.find_tick();L.find_tick();need(r::blow(L.a),"forced blow started");
  unsigned writes=0;for(int i=0;i<40&&r::blow(L.a);++i){auto x=L.tick_blow();writes+=x.write&&x.value!=1;if(L.a.stopped)break;}need(writes==1,"exactly one x2 write (frame 2) in the whole blow");});
 // ---- contact, shield, damage, interruption telemetry ----
 test("contact_ignores_stale_flags_until_registered",[]{Lab L;L.start();L.a.at_hit=true;L.a.hit_link=true;L.a.frame=3;L.post();need(L.ev.count("NATIVE_CONTACT_TARGET_IDENTIFIED")==0,"stale");
  L.a.at_hit=false;L.a.frame=15;L.a.at_set=true;L.post();L.a.at_set=false;L.a.at_hit=true;L.a.frame=17;L.post();need(L.ev.count("NATIVE_CONTACT_TARGET_IDENTIFIED")==1&&L.ev.has("NATIVE_CONTACT_TARGET_IDENTIFIED","TARGET_LINK_TG_CYLINDER_BY_POINTER_IDENTITY"),"identified once");L.a.frame=19;L.post();need(L.ev.count("NATIVE_CONTACT_TARGET_IDENTIFIED")==1,"once per blow");});
 test("shield_block_native_recoil_observed",[]{Lab L;L.start();L.a.frame=15;L.a.at_set=true;L.post();L.a.at_set=false;L.a.at_hit=true;L.a.hit_link=true;L.a.state=3;L.a.rate=-1;L.a.frame=16;L.post();need(L.ev.count("SHIELD_BLOCK_NATIVE_OBSERVED")==1&&L.ev.has("NATIVE_ATTACK_ENDED","LINK_CONTACT_SHIELD_RECOIL"),"shield");});
 test("damage_confirmed_only_from_cp14_forwarding",[]{Lab L;L.start();r::damage_confirmed(L.s(),L.ev);need(L.ev.count("DAMAGE_CONFIRMED")==1&&L.ev.cause.back()==r::Cause::Normal,"attributed to blow cause");
  for(int i=0;i<70;++i)L.find_tick();r::damage_confirmed(L.s(),L.ev);need(L.ev.has("DAMAGE_CONFIRMED","CP14_ATTRIBUTION_BLOW_CAUSE_UNKNOWN_OR_STALE"),"stale");});
 test("interruption_observed_not_observed_unknown",[]{
  auto run=[](unsigned next_proc,int ticks){Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.pre();L.begin_forced();L.a.frame=15;L.a.at_set=true;L.post();L.a.at_set=false;L.a.at_hit=true;L.a.hit_link=true;L.a.frame=17;L.post();
   L.l.proc=int(next_proc);L.l.charge=r::Charge::None;for(int i=0;i<ticks;++i){L.pre();L.a.frame+=2;L.post();}return L.ev;};
  need(run(0x32,2).count("INTERRUPTION_OBSERVED")==1,"damage proc");need(run(0x28,2).count("INTERRUPTION_NOT_OBSERVED")==1,"spin released");need(run(0x3,40).count("INTERRUPTION_UNKNOWN")==1,"unknown");});
 test("no_contact_reactive_blow_reports_no_interruption",[]{Lab L;L.l.pos={250,0,0};L.a.angle=r::yaw(L.a.pos,L.l.pos);L.l.charge=r::Charge::Spin;L.l.proc=0x2a;L.find_tick();L.pre();L.begin_forced();L.finish();need(L.ev.has("INTERRUPTION_NOT_OBSERVED","NO_IDENTIFIED_CONTACT_IN_THIS_BLOW"),"no fake interruption");});
 // ---- contact coverage model (synthetic swings; geometry only) ----
 test("contact_sweep_reduces_x2_misses_without_phantoms",[]{
  unsigned miss_odd=0,miss_even=0,miss_sw=0,native=0,phantom=0,phantom_even=0;double depth=0;
  for(auto& w:swings()){auto cv=coverage(w,10);native+=cv.native;miss_odd+=cv.miss_odd;miss_even+=cv.miss_even;miss_sw+=cv.miss_sweep;phantom+=cv.phantom_sweep;phantom_even+=cv.phantom_even;depth=std::max(depth,cv.depth);
   need(cv.miss_sweep<=cv.miss_odd,"never worse than the plain odd-phase x2 per swing");}
  std::printf("COVERAGE native=%u miss_plain_odd_x2=%u miss_plain_even_x2=%u miss_sweep=%u phantom_cells=%u max_phantom_depth=%.1f phantom_plain_even_pose24=%u\n",native,miss_odd,miss_even,miss_sw,phantom,depth,phantom_even);
  need(depth<=2.0,"phantom limited to sub-grid boundary noise (<=2 units)");need(phantom_even>phantom,"pose 24 never left registered");
  need(miss_sw*10<=miss_odd*6,"at least 40% fewer misses than plain x2");});
 test("contact_sweep_guards",[]{r::State s;s.decided=true;s.owned=true;s.own_rate=r::kBurst;r::Actor a=actor();a.mode=r::Attack;a.state=1;a.b=true;a.rate=r::kBurst;a.tip_r=r::kTipR;a.hilt_r=r::kHiltR;r::Link l=link();l.pos={2000,0,0};
  a.frame=15;a.tip={0,100,90};a.hilt={0,100,40};a.at_set=false;need(!r::sweep(s,a,l).write1,"bit off");
  a.frame=17;a.tip={60,100,70};a.hilt={25,100,35};a.at_set=true;auto o=r::sweep(s,a,l);need(o.write1&&!o.write0&&o.r1<=r::kTipR&&o.r1>=r::kSweepMinR,"midpoint written");
  a.frame=21;need(!r::sweep(s,a,l).write1,"non consecutive frame");
  a.frame=23;a.tip={600,100,70};need(!r::sweep(s,a,l).write1,"jump too large");
  a.frame=25;a.tip={620,100,70};a.hilt={585,100,35};a.wall=true;need(!r::sweep(s,a,l).write1,"wall");
  r::State t;t.decided=true;t.owned=true;t.own_rate=r::kNormal;a.rate=r::kNormal;a.wall=false;need(!r::sweep(t,a,l).write1,"only x2");});
 test("contact_last_tick_registers_pose23_or_exact_pose22_never_24",[]{r::State s;s.decided=true;s.owned=true;s.own_rate=r::kBurst;r::Actor a=actor();a.mode=r::Attack;a.state=1;a.b=true;a.rate=r::kBurst;a.tip_r=r::kTipR;a.hilt_r=r::kHiltR;r::Link l=link();l.pos={2000,0,0};
  a.frame=22;a.tip={60,100,70};a.hilt={25,100,35};a.at_set=true;r::sweep(s,a,l);a.frame=24;a.tip={90,100,40};a.hilt={40,100,20};auto o=r::sweep(s,a,l);
  need(o.write0&&o.write1&&std::string(o.why)=="LAST_TICK_NATIVE_POSE23_TIP_AND_HILT"&&o.c0.x>60&&o.c0.x<90,"pose 23 both spheres");
  r::State u;u.decided=true;u.owned=true;u.own_rate=r::kBurst;a.frame=22;a.tip={60,100,70};a.hilt={25,100,35};r::sweep(u,a,l);a.frame=24;a.tip={60,100,-120};a.hilt={25,100,-60};auto q=r::sweep(u,a,l);
  need(q.write0&&q.write1&&std::string(q.why)=="LAST_TICK_EXACT_PREVIOUS_POSE22"&&q.c0.z==70&&q.r0==r::kTipR,"exact pose 22 fallback");});
 test("contact_sweep_keeps_hilt_when_only_hilt_reaches_link",[]{r::State s;s.decided=true;s.owned=true;s.own_rate=r::kBurst;r::Actor a=actor();a.mode=r::Attack;a.state=1;a.b=true;a.rate=r::kBurst;a.tip_r=r::kTipR;a.hilt_r=r::kHiltR;r::Link l=link();
  a.frame=15;a.tip={0,100,140};a.hilt={0,100,90};a.at_set=true;r::sweep(s,a,l);a.frame=17;a.tip={40,100,130};a.hilt={20,100,85};l.pos={-20,0,60};auto o=r::sweep(s,a,l);need(!o.write1&&o.write0&&o.r0<r::kHiltR+0.01f&&std::string(o.why)=="SKIPPED_NATIVE_POSE_INCLUDED","both reaching hilt poses registered: native hilt kept, tip slot carries the skipped hilt");
  l.pos={2000,0,0};a.frame=19;a.tip={70,100,110};a.hilt={35,100,80};auto d=r::sweep(s,a,l);need(d.write1&&!d.write0&&d.r1>r::kHiltR,"Link far: default tip + skipped tip");});
 // ---- telemetry ----
 test("link_sample_transition_not_per_tick_spam",[]{Lab L;L.a.mode=devf_variant::A_WAIT;L.a.state=0;unsigned lines=0,old=0;std::uint32_t legacy_link=0;
  // 0.14.0: LINK_REACTIVE_SAMPLE_OBSERVED fired on ticks==0||link_id!=id, but link_id was only stored after the non-combat early return.
  for(int i=0;i<1000;++i){lines+=r::sample_transition(L.s(),L.l,r::context(L.a,L.l));old+=(i==0||legacy_link!=L.l.id);if(r::fight(L.a.mode))legacy_link=L.l.id;r::pre(L.s(),L.a,L.l,L.ev);}
  need(L.s().link_id==L.l.id,"FIX01 stores the Link id before any early return");
  need(lines==1,"one line for a stable key");need(old==1000,"0.14.0 predicate reproduced: link_id never updated outside combat");
  L.l.canary=false;need(r::sample_transition(L.s(),L.l,true),"class change logged");L.l.id=11;need(r::sample_transition(L.s(),L.l,true),"new Link logged");});
 test("critical_budget_reserved",[]{r::Budget b;std::array<r::State,64> s{};for(unsigned i=0;i<64;++i)for(int k=0;k<60;++k)b.take(s[i],false);need(b.global==r::Budget::kGlobal,"normal exhausted");
  need(b.take(s[0],true)&&b.crit==1,"critical still logged");for(int k=0;k<80;++k)b.take(s[1],true);need(s[1].crit_logs==r::Budget::kCritActor,"per actor critical cap");
  need(r::critical("INTERCEPT_STARTED",r::Cause::InterceptJumpAttack)&&r::critical("NATIVE_ATTACK_STARTED",r::Cause::Random)&&!r::critical("NATIVE_ATTACK_STARTED",r::Cause::Normal)&&!r::critical("LINK_REACTIVE_SAMPLE_OBSERVED",r::Cause::Normal),"classes");});
 test("slots_identity_generation_reset_and_size",[]{std::array<r::State,64> st{};r::Actor a=actor(5);auto& s=r::state_for(st,3,a,1);s.ticks=9;a.gen=2;auto& t=r::state_for(st,3,a,1);need(t.ticks==0&&t.gen==2,"generation reset");a.room=4;need(r::state_for(st,3,a,1).room==4,"context reset");
  std::printf("STATE_BYTES %zu TABLE_BYTES %zu\n",sizeof(r::State),sizeof(st));need(sizeof(st)<=64*512,"bounded table");});
 test("shutdown_restore_only_owned_live",[]{Lab L;L.start();L.a.frame=3;L.a.rate=r::kNormal;L.post();need(r::restore_on_shutdown(L.s(),L.a),"owned");L.a.rate=1.3f;need(!r::restore_on_shutdown(L.s(),L.a),"foreign");});
 test("non_red_and_context_rejected",[]{Lab L;L.a.variant=V::Blue;auto c=L.pre();need(!c.force&&!c.rush,"blue untouched");Lab M;M.l.room=4;M.l.jump=r::Jump::JumpAttack;M.l.proc=0x26;auto d=M.pre();need(!d.force,"other room");});
 test("canary_failure_disables_reactive_and_interception",[]{Lab L;L.l.canary=false;L.l.charge=r::Charge::None;L.find_tick();need(L.ev.count("INDEPENDENT_LINK_STATE_CANARY_FAILED_REACTIVE_DISABLED")==1,"logged");L.l.jump=r::Jump::JumpAttack;L.l.proc=0x26;L.l.grounded=false;L.l.vel={-15,20,0};auto c=L.pre();need(!c.force,"no interception on failed canary");});
 std::printf("CP17F1_LAB_PASS cases=%u state_bytes=%zu evidence=SHARED_POLICY_AND_SOURCE_PORTS_NOT_ENGINE_CONTACT_OR_AUDIO\n",cases,sizeof(r::State));
}
