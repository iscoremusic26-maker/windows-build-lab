// CP15 offline lab: the Blue AI policy (group level, attack rate / lockout, solo evasion) on the REAL CP15 runtime headers, against a PORT of the native
// daE_OC_c logic that matters (executeFind states 0-4, executeAttack, J3DFrameCtrl, cLib helpers; Dusklight 40457c6, d_a_e_oc.cpp / J3DAnimation.cpp / c_lib.cpp).
// What this proves: the policy's decisions and the PRE -> native -> POST interplay AS READ FROM THE SOURCE. What it does NOT prove: the real engine, the real
// collision world, animation lengths, sound or pixels. The native model uses independent reference math (std::sin/cos/atan2), never the policy's own.
#include "../runtime/DEVF/CP15_BLUE_AI_GROUP_SPEED_SOLO_EVASION/expansion/blue_ai_policy.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <set>
#include <string>
#include <vector>
namespace bl=devf_blue;namespace dy=devf_dyn;using devf_variant::Variant;using s16=std::int16_t;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
constexpr double PI=3.14159265358979323846;
// ---------------------------------------------------------------- native ports (reference math, independent of the policy)
s16 ref_atan2s(double x,double z){const double r=std::atan2(x,z)*32768.0/PI;return static_cast<s16>(static_cast<long>(std::lround(r)));}
double ref_sin(s16 a){return std::sin(double(a)*PI/32768.0);}
double ref_cos(s16 a){return std::cos(double(a)*PI/32768.0);}
int ref_chaseF(float* v,float target,float step){   // c_lib.cpp cLib_chaseF
 if(step){if(*v>target)step=-step;*v+=step;if(step*(*v-target)>=0){*v=target;return 1;}}else if(*v==target)return 1;return 0;
}
s16 ref_addCalcAngleS(s16* v,s16 target,s16 scale,s16 maxStep,s16 minStep){   // c_lib.cpp cLib_addCalcAngleS
 s16 diff=static_cast<s16>(target-*v);
 if(*v!=target){
  s16 step=static_cast<s16>(diff/scale);
  if(step>minStep||step<-minStep){if(step>maxStep)step=maxStep;if(step<-maxStep)step=static_cast<s16>(-maxStep);*v=static_cast<s16>(*v+step);}
  else{if(0<=diff){*v=static_cast<s16>(*v+minStep);diff=static_cast<s16>(target-*v);if(0>=diff)*v=target;}
   else{*v=static_cast<s16>(*v-minStep);diff=static_cast<s16>(target-*v);if(0<=diff)*v=target;}}
 }
 return static_cast<s16>(target-*v);
}
int ref_dist_angle(s16 a,s16 b){return std::abs(static_cast<s16>(a-b));}
// J3DFrameCtrl (J3DAnimation.cpp L23-48 checkPass mode NONE, L141-157 update mode NONE)
struct FrameCtrl {
 float frame=0,rate=1.0f,start=0,end=60;bool stop=false;
 void set_anm(float len,float r){frame=0;rate=r;start=0;end=len;stop=false;}
 bool check_pass(float pass)const{
  float next=frame+rate;if(next<start)next=start;if(next>=end)next=end-0.001f;
  if(frame<=next)return frame<=pass&&pass<next;
  return next<=pass&&pass<frame;
 }
 void update(){stop=false;frame+=rate;if(frame<start){frame=start;rate=0;stop=true;}if(frame>=end){frame=end-0.001f;rate=0;stop=true;}}
 bool is_stop()const{return stop||rate==0.0f;}
};
const float kAttackBTrans[10]={0,0,0,0,15.3f,38.4f,63.3f,86.2f,103.3f,112.0f};
const float kAttackCTrans[10]={0,31.3f,72.7f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f};
// ---------------------------------------------------------------- a small world: ground, water, walls, void
struct Rect {float x0,z0,x1,z1;bool in(float x,float z)const{return x>=x0&&x<=x1&&z>=z0&&z<=z1;}};
struct World {
 std::vector<Rect> water,wall,pit,low;float ground_y=0;
 bool in_any(const std::vector<Rect>& v,float x,float z)const{for(const auto& r:v)if(r.in(x,z))return true;return false;}
 bool ground(float x,float z,float& y)const{if(in_any(pit,x,z))return false;y=in_any(low,x,z)?ground_y-300.0f:ground_y;return true;}
 bool is_water(float x,float z)const{return in_any(water,x,z);}
 // line from (x,z) along heading for 300 units sampled every 5: 0 free, 1 wall < 100, 2 wall 100..300
 int wall_ahead(float x,float z,s16 h)const{for(float d=5;d<=300;d+=5){if(in_any(wall,x+float(ref_sin(h))*d,z+float(ref_cos(h))*d))return d<100?1:2;}return 0;}
};
struct Link {float x=0,y=0,z=0,speed=0;};
// ---------------------------------------------------------------- the native Bokoblin (only the parts the AI touches)
struct Native {
 int mode=bl::kModeFind,state=4,old=bl::kModeWait;s16 c0=0,c2=0,ce=0,da=0,shape=0,cur_angle=0;float speedF=0;float x=0,y=0,z=0;FrameCtrl fc;int anim=3;   // 3 = walk/run, 1 = bck 5, 2 = bck 6, 0 = idle
 float a6a0=0,health=220,home_x=0,home_z=0,move_range=0;int e3=0;bool at_bit=false;float anim_len_b=60,anim_len_c=50;unsigned rng=1;
 // observation
 std::vector<std::string> events;std::vector<float> sound_frames;unsigned contact_ticks=0,blows=0;float blow_disp=0;bool recoil_request=false;bool hold_alive=true;bool fell=false;
 float rnd(){rng=rng*1664525u+1013904223u;return float((rng>>8)&0xFFFF)/65536.0f;}
 void set_mode(int m,int s){old=mode;mode=m;state=s;c0=c2=0;}
 void set_bck(int a,float len,float r){anim=a;fc.set_anm(len,r);}
};
bool search_player(const Native& n,const Link& l,float range){   // searchPlayer L307-329 (the LOS test is true in this world)
 const double dx=l.x-n.x,dy=l.y-n.y,dz=l.z-n.z;const float d=float(std::sqrt(dx*dx+dy*dy+dz*dz));
 if(d<range){const int diff=int(n.shape)-int(ref_atan2s(l.x-n.x,l.z-n.z));
  if(d<400.0f){if(l.speed>12.0f)return true;if(std::abs(diff)<0x5000)return true;}else{if(std::abs(diff)<0x4000)return true;}}
 return false;
}
struct Tick {Native* n;const Link* l;const World* w;float player_range;bool event=false;};
// one daE_OC_c::execute (head decrements, executeFind/Attack/Wait, posMove, play) -- see the source references in the header comment
void native_execute(Native& n,const Link& l,const World& w,float player_range,bool event=false){
 if(n.c0)--n.c0;if(n.c2)--n.c2;if(n.ce)--n.ce;
 const double dx=l.x-n.x,dy=l.y-n.y,dz=l.z-n.z;const float D=float(std::sqrt(dx*dx+dy*dy+dz*dz));const s16 pl_ang=ref_atan2s(l.x-n.x,l.z-n.z);
 auto fwd_water=[&](float arg){return w.is_water(n.x+float(ref_sin(n.shape))*arg,n.z+float(ref_cos(n.shape))*arg);};
 float my_float=0;
 if(n.mode==bl::kModeAttack){int fi=int(n.fc.frame-9.0f);if(fi>=0){if(fi>=9)fi=9;my_float=n.anim==1?kAttackBTrans[fi]:kAttackCTrans[fi];}}
 switch(n.mode){
  case bl::kModeWait:
   if(search_player(n,l,player_range))n.set_mode(bl::kModeFind,1);
   n.speedF=0;break;
  case bl::kModeFind:
   switch(n.state){
    case 0:case 1:case 2:
     if(n.state==0){n.c0=0;n.c2=static_cast<s16>(n.c0+0x14);}
     else if(n.state==1){n.c0=25;}else n.c0=7;
     n.state=3;n.speedF=0;
     [[fallthrough]];
    case 3:
     if(n.c0==0){n.anim=3;n.fc.set_anm(40,1.3f);n.state=4;n.c0=0x1e;}
     break;
    case 4:{
     if(fwd_water(200.0f)){n.set_mode(n.e3?bl::kModeMoveOut:bl::kModeFindStay,0);return;}
     if(ref_dist_angle(n.shape,pl_ang)>=0x400)ref_addCalcAngleS(&n.shape,pl_ang,4,0x800,0x100);
     n.cur_angle=n.shape;
     if(n.ce)n.cur_angle=n.da;   // checkBeforeBgFind() is false in this world
     if(!event){
      if(n.c2==0){
       if(D<300.0f)ref_chaseF(&n.speedF,-3.0f,1.0f);else ref_chaseF(&n.speedF,20.0f,1.0f);
       if(D<400.0f&&D>200.0f){if(std::abs(int(n.shape)-int(pl_ang))<0x1000)n.set_mode(bl::kModeAttack,0);goto after_action;}
      }else{
       if(D<400.0f&&D>200.0f)ref_chaseF(&n.speedF,0.0f,1.0f);else if(D<300.0f)ref_chaseF(&n.speedF,-3.0f,1.0f);else ref_chaseF(&n.speedF,20.0f,1.0f);
      }
     }
     if(n.c0==0){if(!search_player(n,l,player_range))n.set_mode(bl::kModeWait,0);else if(n.e3)n.set_mode(bl::kModeMoveOut,0);}
     break;}
    default:break;
   }
   break;
  case bl::kModeAttack:{
   switch(n.state){
    case 0:
     if(ref_chaseF(&n.speedF,0.0f,1.0f)){
      if(n.rnd()<0.5f){n.set_bck(1,n.anim_len_b,1.0f);n.state=1;}else{n.set_bck(2,n.anim_len_c,1.0f);n.state=2;}
      n.a6a0=0;++n.blows;n.blow_disp=0;
     }
     break;
    case 1:case 2:
     if(n.state==1){
      if(n.fc.check_pass(10.0f))n.sound_frames.push_back(10.0f);else if(n.fc.check_pass(12.5f))n.sound_frames.push_back(12.5f);else if(n.fc.check_pass(19.0f))n.sound_frames.push_back(19.0f);
     }else{
      if(n.fc.check_pass(7.0f))n.sound_frames.push_back(7.0f);else if(n.fc.check_pass(11.5f))n.sound_frames.push_back(11.5f);
     }
     n.at_bit=false;
     if(n.fc.frame>=14.0f&&n.fc.frame<=22.0f){n.at_bit=true;++n.contact_ticks;}
     if(n.recoil_request){n.fc.rate=-1.0f;n.state=3;n.recoil_request=false;}
     {const float step=my_float-n.a6a0;n.x+=step*float(ref_sin(n.shape));n.z+=step*float(ref_cos(n.shape));n.blow_disp+=step;n.a6a0=my_float;}
     if(n.fc.is_stop())n.set_mode(bl::kModeFind,0);
     break;
    case 3:
     {const float step=my_float-n.a6a0;n.a6a0=my_float;(void)step;}
     if(n.fc.is_stop())n.set_mode(bl::kModeFind,0);
     break;
   }
   n.cur_angle=n.shape;break;}
  default:break;
 }
 after_action:
 // fopAcM_posMoveF + ground follow
 {const float nx=n.x+n.speedF*float(ref_sin(n.cur_angle)),nz=n.z+n.speedF*float(ref_cos(n.cur_angle));
  if(!w.in_any(w.wall,nx,nz)){n.x=nx;n.z=nz;}
  float gy;if(w.ground(n.x,n.z,gy)){if(gy<n.y-100.0f)n.fell=true;n.y=gy;}else n.fell=true;}
 n.fc.update();
}
// ---------------------------------------------------------------- the mod's PRE / POST, driven by the REAL policy
struct Brain {
 bl::Tracker lvl;bl::Evade ev;bl::PreSnap snap;bool rate_decided=false,rate_applied=false;unsigned vetoes=0,rate_applies=0,lockouts=0;float last_rate=1.0f;
 std::vector<int> event_log;
};
Link g_link;
void brain_pre(Brain& b,Native& n,const Link& l,const World& w,bool event=false){
 bl::EvIn in;in.self=bl::Self{n.x,n.y,n.z};in.shape_y=n.shape;in.c2=n.c2;in.speed=n.speedF;in.lx=l.x;in.lz=l.z;
 const double dx=l.x-n.x,dy=l.y-n.y,dz=l.z-n.z;in.dist=float(std::sqrt(dx*dx+dy*dy+dz*dz));
 in.leash.active=n.move_range>0;in.leash.hx=n.home_x;in.leash.hz=n.home_z;in.leash.range=n.move_range;
 const bool solo=b.lvl.level==1;
 in.grace=n.mode==bl::kModeFind&&n.state==bl::kFindApproach&&!event&&n.e3==0&&n.health>0;
 in.eligible=solo&&in.grace;
 auto full=[&](s16 h)->bl::Probe{
  bl::Probe p;p.wall=static_cast<std::uint8_t>(w.wall_ahead(n.x,n.z,h));
  p.water200=w.is_water(n.x+float(ref_sin(h))*200,n.z+float(ref_cos(h))*200);p.water400=w.is_water(n.x+float(ref_sin(h))*400,n.z+float(ref_cos(h))*400);
  for(unsigned i=0;i<bl::kSamples;++i){float y;p.found[i]=w.ground(n.x+float(ref_sin(h))*bl::kSampleDist[i],n.z+float(ref_cos(h))*bl::kSampleDist[i],y);p.ground_y[i]=p.found[i]?y:0;}
  return p;
 };
 auto water=[&](s16 h){return w.is_water(n.x+float(ref_sin(h))*200,n.z+float(ref_cos(h))*200);};
 const auto o=bl::step_evade(b.ev,in,full,water);
 if(o.w.turn)n.shape=o.w.shape_y;
 if(o.w.detour){n.ce=2;n.da=o.w.heading;}
 if(o.w.hold_timer&&n.c0<2)n.c0=2;
 if(o.w.speed)n.speedF=o.w.speed_pre;
 if(o.ev!=bl::EvEv::None)b.event_log.push_back(int(o.ev));
 b.snap=bl::make_snap(solo,true,in.dist,n.mode,n.state,n.old,n.c0,n.c2);
}
void brain_post(Brain& b,Native& n,const dy::Ledger& L,const dy::Entry& self,std::uint32_t seq,unsigned window){
 const auto g=bl::scan_group(const_cast<dy::Ledger&>(L),self,seq,window,b.lvl.companions,[](const dy::Entry&){return true;});
 bl::track(b.lvl,g);
 if(const auto rb=bl::veto_attack_start(b.snap,n.mode,n.state);rb.apply){n.mode=rb.mode;n.old=rb.old;n.state=rb.state;n.c0=rb.c0;n.c2=rb.c2;++b.vetoes;}
 if(!bl::in_blow(n.mode,n.state)){b.rate_decided=false;b.rate_applied=false;}
 else if(!b.rate_decided&&b.lvl.level>=2){
  bl::AtkView v;v.mode=n.mode;v.state=n.state;v.frame=n.fc.frame;v.play_speed=n.fc.rate;v.anim_b=n.state==bl::kAttackB&&n.anim==1;v.anim_c=n.state==bl::kAttackC&&n.anim==2;
  const auto act=bl::decide_rate(v,b.lvl.level,b.rate_decided);
  if(act!=bl::RateAction::None){b.rate_decided=true;if(act==bl::RateAction::Apply){n.fc.rate=bl::rate_for(b.lvl.level);b.rate_applied=true;b.last_rate=n.fc.rate;++b.rate_applies;}}
 }
 if(const int t=bl::lockout_write(b.snap,n.mode,n.c2,b.lvl.level);t>=0){n.c2=static_cast<s16>(t);++b.lockouts;}
}
// ---------------------------------------------------------------- bokoblin world for the group tests (real CP10.6 ledger)
struct Crowd {
 dy::Ledger L;std::uint32_t seq=0;int ids=0;std::vector<int> addr=std::vector<int>(dy::kCapacity);
 struct Io{std::uint8_t v[2]{1,1};std::uint8_t get(unsigned i)const{return v[i];}void set(unsigned i,std::uint8_t x){v[i]=x;}};
 std::vector<Io> io=std::vector<Io>(dy::kCapacity);
 dy::Entry* add(Variant v,float x,float z,int room=1,std::uint32_t gen=0){
  const int i=ids++;auto* e=L.record(std::uint32_t(100+i),&addr[std::size_t(i)],gen?gen:std::uint32_t(1+i),v,devf_damage::target_for(v));require(e,"record");post(e,x,z,room);return e;
 }
 void post(dy::Entry* e,float x,float z,int room=1,int health=100,int action=0){
  ++seq;dy::Post in;in.post_health=health;in.action=action;in.room=room;in.x=x;in.y=0;in.z=z;in.stage=7;in.seq=seq;in.window=dy::window_for(L.used);
  dy::post_update(L,*e,io[std::size_t(e-L.entries.data())],in,[](const dy::Entry&){return true;});
 }
 bl::Group scan(dy::Entry* self,unsigned trusted=0){return bl::scan_group(L,*self,seq,dy::window_for(L.used),trusted,[](const dy::Entry&){return true;});}
};
float dist2d(float ax,float az,float bx,float bz){return float(std::sqrt(double(ax-bx)*(ax-bx)+double(az-bz)*(az-bz)));}
int main(){
 // ================================================================= 1. math
 {double worst_sin=0,worst_cos=0;for(int a=-32768;a<32768;a+=7){worst_sin=std::fmax(worst_sin,std::fabs(bl::sin_s16(static_cast<s16>(a))-ref_sin(static_cast<s16>(a))));worst_cos=std::fmax(worst_cos,std::fabs(bl::cos_s16(static_cast<s16>(a))-ref_cos(static_cast<s16>(a))));}
  require(worst_sin<5e-6&&worst_cos<5e-6,"sin/cos of every s16 angle within 5e-6 of the reference");
  int worst=0;for(int i=0;i<=720;++i){const double r=i*PI/360.0;for(double m:{1.0,37.0,900.0}){const float x=float(std::sin(r)*m),z=float(std::cos(r)*m);const int d=std::abs(int(static_cast<s16>(bl::atan2s(x,z)-ref_atan2s(x,z))));worst=std::max(worst,d);}}
  require(worst<=2,"atan2s within 2 units (of 65536) of the reference on 2163 vectors, all quadrants");
  require(bl::atan2s(0,0)==0&&bl::atan2s(0,5)==0&&bl::atan2s(5,0)==0x4000&&bl::atan2s(-5,0)==-0x4000&&std::abs(bl::atan2s(0,-5))==-(-32768),"axis angles: +Z 0, +X 0x4000, -X -0x4000, -Z +-0x8000");
  require(bl::angle_diff(static_cast<s16>(-32768),32767)==1&&bl::abs_diff(static_cast<s16>(-30000),30000)==5536,"angle wrap-around is shortest-arc");
  pass("math_sin_cos_atan2_match_reference_without_crt");}
 // ================================================================= 2. speed compensation vs the native chase
 {unsigned n=0;
  for(float c2:{0.0f,1.0f})for(float d:{50.0f,150.0f,200.0f,250.0f,299.0f,300.0f,350.0f,399.0f,400.0f,401.0f,900.0f,1500.0f}){
   const float target=bl::native_find_target(static_cast<s16>(c2),d);
   // independent restatement of the source branches
   float expect;if(c2==0)expect=d<300.0f?-3.0f:20.0f;else expect=(d<400.0f&&d>200.0f)?0.0f:d<300.0f?-3.0f:20.0f;
   require(target==expect,"native_find_target mirrors executeFind state 4");
   for(float want:{0.0f,1.0f,2.0f,10.0f,19.0f,20.0f}){float v=bl::pre_speed_for(want,target);ref_chaseF(&v,target,1.0f);require(v==want,"after the native chase the speed is exactly the wanted one");++n;}
  }
  require(n==144,"all combinations checked");
  pass("pre_speed_compensates_the_native_chase_exactly");}
 // ================================================================= 3. group level
 {Crowd c;auto* b=c.add(Variant::Blue,0,0);require(c.scan(b).companions==0&&c.scan(b).level()==1,"alone: level 1");
  auto* r=c.add(Variant::Red,100,0);require(c.scan(b).level()==2&&c.scan(b).ids[0]==r->id,"Red alone as companion: level 2");
  c.post(b,0,0);c.post(r,100,0);
  c.add(Variant::Black,0,300);c.post(b,0,0);require(c.scan(b).level()==3&&c.scan(b).companions==2,"Red + Black: level 3");
  for(int i=0;i<3;++i)c.add(Variant::Blue,50.f*float(i),50);
  c.post(b,0,0);require(c.scan(b).level()==3&&c.scan(b).companions==2,"4, 5, 6 Bokoblins: still level 3 (cap)");
  pass("group_level_counts_blue_red_black_with_cap_3");}
 {Crowd c;auto* b=c.add(Variant::Blue,0,0);auto* r=c.add(Variant::Red,650,0);c.post(b,0,0);c.post(r,650,0);
  require(c.scan(b).level()==2,"650.0 XZ is inside (inclusive)");
  c.post(r,650.5f,0);require(c.scan(b).level()==1,"650.5 XZ is outside");
  c.post(r,0,0);c.post(r,0,0);auto* far=c.add(Variant::Black,300,0);(void)far;
  r=c.add(Variant::Red,0,0);c.post(r,0,0);
  Crowd d;auto* b2=d.add(Variant::Blue,0,0);auto* r2=d.add(Variant::Red,0,0);
  d.post(r2,0,0);d.io[std::size_t(r2-d.L.entries.data())].v[0]=2;
  {dy::Post in;in.post_health=100;in.action=0;in.room=1;in.x=0;in.y=250;in.z=0;in.stage=7;in.seq=++d.seq;in.window=dy::window_for(d.L.used);dy::post_update(d.L,*r2,d.io[std::size_t(r2-d.L.entries.data())],in,[](const dy::Entry&){return true;});}
  d.post(b2,0,0);require(d.scan(b2).level()==2,"|dy| = 250 is inside");
  {dy::Post in;in.post_health=100;in.action=0;in.room=1;in.x=0;in.y=250.5f;in.z=0;in.stage=7;in.seq=++d.seq;in.window=dy::window_for(d.L.used);dy::post_update(d.L,*r2,d.io[std::size_t(r2-d.L.entries.data())],in,[](const dy::Entry&){return true;});}
  d.post(b2,0,0);require(d.scan(b2).level()==1,"|dy| = 250.5 is outside");
  pass("group_level_radius_boundaries_650_xz_250_y");}
 {Crowd c;auto* b=c.add(Variant::Blue,0,0);auto* r=c.add(Variant::Red,100,0);auto* k=c.add(Variant::Black,0,100);c.post(b,0,0);
  require(c.scan(b).level()==3,"3 in range");
  c.post(k,0,100,1,0,0);require(c.scan(b).level()==2,"one dies (health 0): level 2 at once");
  c.post(r,100,0,1,100,devf_variant::A_DEATH);require(c.scan(b).level()==1,"the other dying (terminal action): level 1");
  c.post(k,0,100,2);c.post(r,100,0,2);require(c.scan(b).level()==1,"both in another room: no companions");
  c.post(k,0,100);c.post(r,100,0);require(c.scan(b).level()==3,"they come back: level 3");
  for(unsigned i=0;i<dy::window_for(c.L.used)+3;++i)c.post(b,0,0);
  require(c.scan(b).level()==1,"companions that stopped executing go stale");
  pass("group_level_death_terminal_room_and_stale_snapshots");}
 {Crowd c;auto* b=c.add(Variant::Blue,0,0);auto* r=c.add(Variant::Red,10,0);auto* k=c.add(Variant::Black,20,0);c.post(b,0,0);
  unsigned verified=0;auto g=bl::scan_group(c.L,*b,c.seq,dy::window_for(c.L.used),0,[&](const dy::Entry&){++verified;return true;});
  require(g.companions==2&&verified==2,"first sight verifies both");
  verified=0;g=bl::scan_group(c.L,*b,c.seq,dy::window_for(c.L.used),2,[&](const dy::Entry&){++verified;return true;});
  require(g.companions==2&&verified==0,"already trusted companions are not re-verified every tick");
  g=bl::scan_group(c.L,*b,c.seq,dy::window_for(c.L.used),1,[&](const dy::Entry& e){return e.id!=k->id;});
  require(g.companions==1&&g.ids[0]==r->id,"a candidate beyond the trusted count that fails verification is dropped");
  require(!k->snap&&r->snap,"and its snapshot is invalidated until it executes again");
  (void)r;pass("group_level_authoritative_verification_only_for_new_companions");}
 // ================================================================= 4. tracker
 {bl::Tracker t;bl::Group g;
  require(t.level==1,"a Blue is SOLO from its first instant");
  g.companions=2;g.ids[0]=5;g.ids[1]=6;auto r=bl::track(t,g);require(r.changed&&r.from==1&&r.to==3&&t.level==3,"1 -> 3 at once");
  g.companions=1;r=bl::track(t,g);require(r.changed&&r.from==3&&r.to==2,"3 -> 2 at once");
  g.companions=0;g.ids[0]=g.ids[1]=0;
  r=bl::track(t,g);require(!r.changed&&t.level==2,"first solo tick: still 2 (debounce)");
  r=bl::track(t,g);require(!r.changed&&t.level==2,"second solo tick: still 2");
  r=bl::track(t,g);require(r.changed&&r.from==2&&r.to==1&&t.level==1&&t.companions==0,"third consecutive solo tick: 1 (0.1 s)");
  g.companions=1;g.ids[0]=9;r=bl::track(t,g);require(r.changed&&t.level==2,"a companion at once: 2");
  g.companions=0;bl::track(t,g);bl::track(t,g);g.companions=1;r=bl::track(t,g);require(!r.changed&&t.level==2,"a glitch of 2 solo ticks then a companion again: never left level 2");
  g.companions=0;bl::track(t,g);bl::track(t,g);bl::track(t,g);require(t.level==1,"back to 1");
  g.companions=2;bl::track(t,g);require(t.level==3,"and straight to 3");
  pass("level_transitions_1_3_2_1_2_3_with_bounded_solo_debounce");}
 {struct S{std::uint32_t id=0,generation=0;int v=0;};std::array<S,4> tab{};
  auto& a=bl::stamp(tab,1,100,1);a.v=7;
  require(bl::stamp(tab,1,100,1).v==7,"same id+generation: same record");
  require(bl::stamp(tab,1,100,2).v==0,"same id, NEW generation: nothing inherited");
  bl::stamp(tab,1,100,2).v=9;require(bl::stamp(tab,1,101,2).v==0,"another actor in the slot: nothing inherited");
  require(bl::stamp(tab,99,5,5).id==5,"out-of-range slot index falls back to slot 0 without UB");
  pass("per_actor_state_never_inherited_by_id_slot_or_generation");}
 // ================================================================= 5. rates
 {require(bl::rate_for(1)==1.0f&&bl::rate_for(2)==1.15f&&bl::rate_for(3)==1.30f&&bl::rate_for(7)==1.30f,"rates 1.0 / 1.15 / 1.30, capped at 3");
  require(bl::lockout_for(1)==20&&bl::lockout_for(2)==17&&bl::lockout_for(3)==15,"lockout 20 / 17 / 15");
  require(bl::rate_for(3)>bl::rate_for(2)&&bl::rate_for(2)>bl::rate_for(1),"group 3 faster than 2 faster than native");
  require(bl::applies_to(Variant::Blue)&&!bl::applies_to(Variant::Red)&&!bl::applies_to(Variant::Black)&&!bl::applies_to(Variant::None),"only Blue");
  bl::AtkView v;v.mode=bl::kModeAttack;v.state=1;v.anim_b=true;v.frame=1.0f;v.play_speed=1.0f;
  require(bl::decide_rate(v,2,false)==bl::RateAction::Apply&&bl::decide_rate(v,3,false)==bl::RateAction::Apply,"fresh blow B at 1.0: apply");
  require(bl::decide_rate(v,1,false)==bl::RateAction::None,"level 1: nothing");
  require(bl::decide_rate(v,2,true)==bl::RateAction::None,"already decided for this blow: nothing");
  v.state=2;v.anim_b=false;v.anim_c=true;require(bl::decide_rate(v,3,false)==bl::RateAction::Apply,"fresh blow C: apply");
  v.anim_c=false;require(bl::decide_rate(v,3,false)==bl::RateAction::None,"state 2 without bck 6: not ours");
  v.anim_c=true;v.frame=2.0f;require(bl::decide_rate(v,3,false)==bl::RateAction::SkipNotFresh,"blow already running (frame 2): never rewrite mid-blow");
  v.frame=1.0f;v.play_speed=-1.0f;require(bl::decide_rate(v,3,false)==bl::RateAction::SkipForeignSpeed,"shield recoil -1.0: not ours");
  v.play_speed=1.3f;require(bl::decide_rate(v,3,false)==bl::RateAction::SkipForeignSpeed,"somebody else's rate: not ours");
  v.play_speed=1.0f;v.mode=bl::kModeFind;require(bl::decide_rate(v,3,false)==bl::RateAction::None,"not attacking");
  v.mode=bl::kModeAttack;v.state=3;require(bl::decide_rate(v,3,false)==bl::RateAction::None,"recoil state 3");
  pass("attack_rate_ordering_ownership_and_blue_only");}
 // ================================================================= 6. blow timing under fractional rates (port of J3DFrameCtrl)
 {struct Res{unsigned ticks=0,contact=0,first_contact=0,last_contact=0;std::vector<float> sounds;float disp=0;bool ended=false;};
  auto blow=[&](int which,float len,float rate){
   Native n;n.mode=bl::kModeAttack;n.state=which;n.anim=which;n.fc.set_anm(len,1.0f);n.fc.update();   // setBck then the play() of the same execute: frame 1.0, rate 1.0
   require(n.fc.frame==1.0f,"setBck + play leaves the frame at exactly 1.0");
   n.fc.rate=rate;Link l;World w;Res r;n.speedF=0;
   for(unsigned t=1;t<200&&n.mode==bl::kModeAttack;++t){native_execute(n,l,w,5000);++r.ticks;if(n.at_bit){if(!r.contact)r.first_contact=t;r.last_contact=t;++r.contact;}}
   r.sounds=n.sound_frames;r.disp=n.blow_disp;r.ended=n.mode==bl::kModeFind;return r;
  };
  for(int which:{1,2})for(float len:{46.0f,60.0f,80.0f}){
   const auto nat=blow(which,len,1.0f);
   require(nat.ended&&nat.contact==9,"native: 9 contact ticks (frames 14..22 at rate 1.0)");
   for(float rate:{1.15f,1.30f}){
    const auto r=blow(which,len,rate);
    require(r.ended,"the blow still ends by itself");
    require(r.contact>=6&&r.contact<nat.contact,"fewer but at least 6 contact ticks");
    require(r.last_contact-r.first_contact+1==r.contact,"contact ticks are contiguous");
    require(r.sounds==nat.sounds,"every frame-triggered sound fires exactly once, in the same order");
    require(std::fabs(r.disp-nat.disp)<1e-3f,"cumulative blow displacement is rate independent");
    require(r.ticks<nat.ticks,"the blow takes fewer ticks");
   }
   if(std::getenv("LAB_TABLE")){const auto q2=blow(which,len,1.15f),q3=blow(which,len,1.30f);std::printf("TABLE blow=%s len=%.0f native(ticks=%u contact=%u) pair1.15(ticks=%u contact=%u) group1.30(ticks=%u contact=%u)\n",which==1?"B":"C",double(len),nat.ticks,nat.contact,q2.ticks,q2.contact,q3.ticks,q3.contact);}
   const auto r2=blow(which,len,1.15f),r3=blow(which,len,1.30f);
   require(r3.ticks<r2.ticks&&r3.ticks<nat.ticks,"group 3 blow is the shortest");
  }
  {const auto b=blow(1,60,1.0f);require(b.sounds==std::vector<float>({10.0f,12.5f,19.0f}),"B sounds at 10, 12.5, 19");const auto c=blow(2,50,1.0f);require(c.sounds==std::vector<float>({7.0f,11.5f}),"C sounds at 7, 11.5");}
  pass("blow_timing_contact_window_sounds_and_movement_under_1_15_and_1_30");}
 // ================================================================= 7. a full attack cadence with the hooks, per level
 {auto run=[&](unsigned level,unsigned ticks,bool solo_allowed=false){
   World w;Link l;l.x=0;l.z=350;Native n;n.shape=0;n.x=0;n.z=0;n.state=4;n.mode=bl::kModeFind;Brain b;
   Crowd c;auto* self=c.add(Variant::Blue,0,0);std::vector<dy::Entry*> comps;
   for(unsigned i=0;i+1<level;++i)comps.push_back(c.add(i%2?Variant::Black:Variant::Red,30.f*float(i+1),0));
   std::vector<unsigned> starts;int prev_mode=n.mode;
   for(unsigned t=0;t<ticks;++t){
    c.post(self,n.x,n.z);for(auto* k:comps)c.post(k,k->x,k->z);
    brain_pre(b,n,l,w);native_execute(n,l,w,5000);brain_post(b,n,c.L,*self,c.seq,dy::window_for(c.L.used));
    if(n.mode==bl::kModeAttack&&prev_mode!=bl::kModeAttack)starts.push_back(t);prev_mode=n.mode;
    l.z=n.z+350;   // Link keeps 350 away (the Blue's attack distance band 200..400)
   }
   struct R{std::vector<unsigned> starts;unsigned vetoes,rates,lockouts;float rate;};return R{starts,b.vetoes,b.rate_applies,b.lockouts,b.last_rate};
  };
  const auto l2=run(2,600),l3=run(3,600);
  require(l2.starts.size()>=6&&l3.starts.size()>=6,"blows keep happening in a pair and in a group");
  auto mean_gap=[](const std::vector<unsigned>& s){double g=0;for(size_t i=1;i<s.size();++i)g+=s[i]-s[i-1];return g/double(s.size()-1);};
  require(l2.rates>=6&&l3.rates>=6&&l2.lockouts>=6&&l3.lockouts>=6,"the rate and lockout are applied to every blow");
  require(l2.rate==1.15f&&l3.rate==1.30f,"pair 1.15, group 1.30");
  require(mean_gap(l3.starts)<mean_gap(l2.starts),"a group-3 Blue starts blows more often than a pair");
  // native baseline: a Blue with the AI disabled (level forced to 2 for the policy but no writes) -> emulate by running the model without hooks
  {World w;Link l;l.z=350;Native n;n.state=4;std::vector<unsigned> st;int pm=n.mode;
   for(unsigned t=0;t<600;++t){native_execute(n,l,w,5000);if(n.mode==bl::kModeAttack&&pm!=bl::kModeAttack)st.push_back(t);pm=n.mode;l.z=n.z+350;}
   require(st.size()>=5&&mean_gap(st)>mean_gap(l2.starts),"the native Blue is slower than the pair, which is slower than the group");}
  pass("attack_cadence_native_slower_than_pair_slower_than_group");}
 // ================================================================= 8. group size changes during a blow
 {World w;Link l;l.z=350;Native n;n.state=4;Brain b;Crowd c;auto* self=c.add(Variant::Blue,0,0);auto* r1=c.add(Variant::Red,30,0);auto* r2=c.add(Variant::Black,60,0);
  bool in_blow=false;float rate_during=0;bool changed_mid=false;unsigned blows=0;std::vector<float> rates;
  for(unsigned t=0;t<400;++t){
   c.post(self,n.x,n.z);c.post(r1,30,0);if(t<150||t>=150)c.post(r2,60,0);
   brain_pre(b,n,l,w);native_execute(n,l,w,5000);brain_post(b,n,c.L,*self,c.seq,dy::window_for(c.L.used));
   if(bl::in_blow(n.mode,n.state)){
    if(!in_blow){in_blow=true;++blows;rate_during=n.fc.rate;rates.push_back(rate_during);
     // kill both companions right now: the level falls while the blow is in progress
     c.post(r1,30,0,1,0,devf_variant::A_DEATH);c.post(r2,60,0,1,0,devf_variant::A_DEATH);}
    else if(n.fc.rate!=0.0f&&n.fc.rate!=rate_during){changed_mid=true;}
   }else in_blow=false;
   l.z=n.z+350;
   if(blows>=1&&!in_blow&&b.lvl.level==1)break;
  }
  require(blows>=1&&!changed_mid&&rates[0]==1.30f,"the rate chosen at the start of the blow (1.30) is kept until it ends, although the group died during it");
  require(b.lvl.level==1,"and the level fell to solo afterwards");
  pass("level_change_mid_blow_keeps_the_rate_of_that_blow");}
 // ================================================================= 9. shield recoil and non-owned rates
 {World w;Link l;l.z=350;Native n;n.state=4;Brain b;Crowd c;auto* self=c.add(Variant::Blue,0,0);c.add(Variant::Red,30,0);bool recoil_done=false;float rate_after_recoil=9;
  for(unsigned t=0;t<300&&!recoil_done;++t){
   c.post(self,n.x,n.z);brain_pre(b,n,l,w);
   if(n.mode==bl::kModeAttack&&(n.state==1||n.state==2)&&n.fc.frame>=3&&!recoil_done)n.recoil_request=true;
   native_execute(n,l,w,5000);brain_post(b,n,c.L,*self,c.seq,dy::window_for(c.L.used));
   if(n.mode==bl::kModeAttack&&n.state==3){recoil_done=true;rate_after_recoil=n.fc.rate;}
   l.z=n.z+350;
  }
  require(recoil_done&&rate_after_recoil==-1.0f,"after the native shield recoil the rate is the native -1.0 (never overwritten)");
  for(unsigned t=0;t<4;++t){c.post(self,n.x,n.z);brain_pre(b,n,l,w);native_execute(n,l,w,5000);brain_post(b,n,c.L,*self,c.seq,dy::window_for(c.L.used));if(n.mode==bl::kModeAttack)require(n.fc.rate==-1.0f||n.fc.rate==0.0f,"recoil keeps running at -1.0 / stops");}
  pass("shield_recoil_keeps_its_native_reverse_rate");}
 // ================================================================= 10. veto and lockout on the pure helpers
 {auto s=bl::make_snap(true,true,300.0f,bl::kModeFind,bl::kFindApproach,bl::kModeWait,0,0);
  auto rb=bl::veto_attack_start(s,bl::kModeAttack,0);require(rb.apply&&rb.mode==bl::kModeFind&&rb.state==4&&rb.old==bl::kModeWait,"a solo Blue's new blow is rolled back to FIND / 4");
  s=bl::make_snap(true,true,300.0f,bl::kModeFind,4,bl::kModeWait,30,0);rb=bl::veto_attack_start(s,bl::kModeAttack,0);require(rb.c0==29&&rb.c2==0,"timers get the native head decrement back");
  s=bl::make_snap(true,true,300.0f,bl::kModeAttack,1,bl::kModeFind,0,0);rb=bl::veto_attack_start(s,bl::kModeAttack,0);require(!rb.apply,"a blow ALREADY in progress is never touched");
  s=bl::make_snap(false,true,300.0f,bl::kModeFind,4,bl::kModeWait,0,0);rb=bl::veto_attack_start(s,bl::kModeAttack,0);require(!rb.apply,"a Blue in a group keeps attacking");
  s=bl::make_snap(true,false,300.0f,bl::kModeFind,4,bl::kModeWait,0,0);rb=bl::veto_attack_start(s,bl::kModeAttack,0);require(!rb.apply,"no Link: nothing to veto");
  s=bl::make_snap(true,true,1300.0f,bl::kModeFind,4,bl::kModeWait,0,0);rb=bl::veto_attack_start(s,bl::kModeAttack,0);require(!rb.apply,"Link beyond the relevance radius: nothing to veto");
  s=bl::make_snap(true,true,300.0f,bl::kModeFind,4,bl::kModeWait,0,0);rb=bl::veto_attack_start(s,bl::kModeAttack,1);require(!rb.apply,"only the START (state 0) is vetoed");
  pass("attack_veto_is_a_start_only_rollback");}
 {auto s=bl::make_snap(true,true,300.0f,bl::kModeFind,0,bl::kModeAttack,0,0);
  require(s.expect_lockout&&bl::lockout_write(s,bl::kModeFind,20,2)==17&&bl::lockout_write(s,bl::kModeFind,20,3)==15,"after a blow the native 20 becomes 17 / 15");
  require(bl::lockout_write(s,bl::kModeFind,20,1)==-1,"solo / level 1: nothing written");
  require(bl::lockout_write(s,bl::kModeFind,19,3)==-1&&bl::lockout_write(s,bl::kModeAttack,20,3)==-1,"only the exact native value 20 in FIND is rewritten");
  s=bl::make_snap(true,true,300.0f,bl::kModeFind,0,devf_variant::A_DAMAGE,0,0);require(!s.expect_lockout&&bl::lockout_write(s,bl::kModeFind,20,3)==-1,"after DAMAGE the lockout is not an attack lockout: untouched");
  s=bl::make_snap(true,true,300.0f,bl::kModeFind,4,bl::kModeAttack,0,0);require(!s.expect_lockout,"only the tick that runs FIND state 0");
  pass("post_attack_lockout_scaled_only_after_a_blow");}
 // ================================================================= 11. SOLO evasion against the native model
 struct Trace {std::vector<float> dist,speed,step,home_dist;unsigned attack_ticks=0,wait_ticks=0,max_blows=0;bool never_fell=true,never_water=true,never_in_wall=true;float min_dist=1e9f,last_dist=0;Native end;unsigned vetoes=0;std::vector<int> evs;};
 auto evade=[&](World w,Native n,Link l,unsigned ticks,std::function<void(unsigned,Link&,Native&)> drive,bool with_ai=true,float range=5000.0f){
  Crowd c;auto* self=c.add(Variant::Blue,n.x,n.z);Brain b;Trace tr;
  for(unsigned t=0;t<ticks;++t){
   drive(t,l,n);
   c.post(self,n.x,n.z);
   const float px=n.x,pz=n.z;
   if(with_ai)brain_pre(b,n,l,w);
   native_execute(n,l,w,range);
   if(with_ai)brain_post(b,n,c.L,*self,c.seq,dy::window_for(c.L.used));
   const float d=dist2d(n.x,n.z,l.x,l.z);tr.dist.push_back(d);tr.speed.push_back(n.speedF);tr.step.push_back(dist2d(n.x,n.z,px,pz));tr.home_dist.push_back(dist2d(n.x,n.z,n.home_x,n.home_z));
   if(n.mode==bl::kModeAttack)++tr.attack_ticks;if(n.mode==bl::kModeWait)++tr.wait_ticks;
   if(w.is_water(n.x,n.z))tr.never_water=false;if(n.fell)tr.never_fell=false;if(w.in_any(w.wall,n.x,n.z))tr.never_in_wall=false;
   tr.min_dist=std::min(tr.min_dist,d);
  }
  tr.end=n;tr.max_blows=n.blows;tr.vetoes=b.vetoes;tr.evs=b.event_log;tr.last_dist=tr.dist.back();return tr;
 };
 auto toward=[&](float speed){return [speed](unsigned,Link& l,Native& n){const float dx=n.x-l.x,dz=n.z-l.z;const float d=std::sqrt(dx*dx+dz*dz);if(d>1.0f){const float s=std::min(speed,d);l.x+=dx/d*s;l.z+=dz/d*s;}l.speed=speed;};};
 auto still=[&](unsigned,Link& l,Native&){l.speed=0;};
 {World w;Native n;n.shape=0;Link l;l.z=900;
  const auto base=evade(w,n,l,200,toward(10.0f),false);
  require(base.attack_ticks>0&&base.max_blows>=1,"BASELINE: the native Blue (no AI) walks up to Link and attacks");
  const auto ai=evade(w,n,l,200,toward(10.0f),true);
  require(ai.attack_ticks==0&&ai.max_blows==0&&ai.vetoes==0,"SOLO Blue with the AI: never starts a blow (the facing alone prevents it)");
  require(ai.end.z<-800.0f,"it ran away: more than 800 units behind its start");
  require(ai.last_dist>=ai.dist.front()-1.0f||ai.last_dist>base.last_dist+300.0f,"the distance to Link ended far larger than the baseline's");
  require(ai.wait_ticks==0,"it stays alert in FIND the whole time Link is relevant (no FIND -> WAIT while we run with our back to Link)");
  float vmax=0;for(float s:ai.step)vmax=std::max(vmax,s);require(vmax<=20.001f,"never faster than the native chase speed 20 (no teleport, no burst)");
  require(std::fabs(ai.end.x)<3.0f,"it ran STRAIGHT away: the native detour fields give the exact heading (without them the native counter-turn drifts it sideways)");
  require(ref_dist_angle(ai.end.shape,ref_atan2s(0,0)+0)>0x6000||ai.end.mode!=bl::kModeFind||true,"(facing checked below)");
  {World w2;Native n2;Link l2;l2.z=900;auto tr=evade(w2,n2,l2,40,toward(10.0f));
   const s16 pl=ref_atan2s(l2.x-tr.end.x,l2.z-tr.end.z);
   require(ref_dist_angle(tr.end.shape,pl)>0x6000,"while it runs it is turned AWAY from Link (the body follows the heading)");}
  require(ai.min_dist>=ai.dist.front()-0.5f-0.0f||ai.min_dist>250.0f,"Link, approaching at 10 per tick, never got closer than the attack band");
  pass("solo_blue_flees_instead_of_chasing_and_never_attacks");}
 {World w;Native n;Link l;l.z=900;const auto ai=evade(w,n,l,300,toward(24.0f));
  require(ai.attack_ticks==0&&ai.max_blows==0,"even when Link outruns it (24 > 20) it never attacks");
  float vmax=0;for(float s:ai.step)vmax=std::max(vmax,s);require(vmax<=20.001f,"speed bounded at 20");
  unsigned exact=0;for(unsigned i=30;i<ai.step.size();++i)if(std::fabs(ai.step[i]-20.0f)<1e-3f)++exact;
  require(exact>=200,"with Link closing in (distance < 300, where the native chase target is -3) the Blue still runs at exactly 20: the PRE write compensates the chase");
  pass("solo_blue_caught_by_a_faster_link_still_never_attacks");}
 {World w;w.wall.push_back({-5000,-170,5000,-150});Native n;Link l;l.z=300;const auto ai=evade(w,n,l,120,still);
  require(ai.never_in_wall&&ai.attack_ticks==0,"wall right behind: never inside it, no attack");
  require(std::fabs(ai.end.x)>400.0f&&ai.end.z>-150.0f,"it slid ALONG the wall (a +-90 degree heading) instead of into it");
  require(ai.last_dist>ai.dist.front(),"and still gained distance to Link");
  pass("solo_blue_cornered_against_a_wall_runs_along_it");}
 {World w;w.wall.push_back({-100,-100,-80,100});w.wall.push_back({80,-100,100,100});w.wall.push_back({-100,-100,100,-80});w.wall.push_back({-100,80,100,100});   // a 160-wide cell: a wall closer than 100 in every direction
  Native n;n.shape=0;Link l;l.z=330;
  const auto ai=evade(w,n,l,90,still);
  float moved=0;for(float s:ai.step)moved=std::max(moved,s);
  require(ai.attack_ticks==0&&ai.max_blows==0,"boxed in with Link at attack range: no blow");
  require(ai.vetoes>20,"the native tried to attack every tick and the veto rolled it back");
  require(moved<=1.001f,"fail safe: holds position (at most the native 1 unit residue), no run into the walls");
  require(std::count(ai.evs.begin(),ai.evs.end(),int(bl::EvEv::Hold))==1,"exactly one EVADE_HOLD event (not repeated every tick)");
  pass("solo_blue_with_no_safe_heading_holds_and_vetoes_attacks");}
 {World w;w.pit.push_back({-5000,-420,5000,-200});Native n;Link l;l.z=300;const auto ai=evade(w,n,l,150,still);
  require(ai.never_fell,"a void behind: it never ran into it");
  require(ai.attack_ticks==0&&std::fabs(ai.end.x)>200.0f,"it went around along the edge");
  pass("solo_blue_never_runs_off_a_ledge");}
 {World w;w.pit.push_back({-5000,-5000,5000,-120});w.pit.push_back({-5000,-120,-250,5000});w.pit.push_back({250,-120,5000,5000});   // an island: ground only in x in [-250,250], z >= -120
  Native n;n.shape=0;Link l;l.z=330;const auto ai=evade(w,n,l,150,still);
  require(ai.never_fell&&ai.attack_ticks==0,"on a small island it never jumps off");
  pass("solo_blue_on_an_island_never_leaves_it");}
 {World w;w.water.push_back({-5000,-290,5000,-170});Native n;Link l;l.z=300;const auto ai=evade(w,n,l,150,still);
  require(ai.never_water&&ai.attack_ticks==0,"deep water behind: never entered");
  require(std::fabs(ai.end.x)>200.0f,"it went around the water");
  pass("solo_blue_never_enters_water");}
 {World w;Native n;n.move_range=600;Link l;l.z=500;const auto ai=evade(w,n,l,200,toward(8.0f));
  float far_home=0;for(float h:ai.home_dist)far_home=std::max(far_home,h);
  require(far_home<=600.0f+20.0f,"the leash (600) is respected: it never runs more than one step past it");
  require(ai.attack_ticks==0,"and it does not attack either");
  pass("solo_blue_flees_inside_its_leash");}
 {World w;Native n;Link l;l.z=1500;const auto ai=evade(w,n,l,60,still);const auto base=evade(w,n,l,60,still,false);
  float moved=0;for(float s:ai.step)moved=std::max(moved,s);
  require(base.end.z>1000.0f||base.dist.back()<base.dist.front()-600.0f,"BASELINE: the native Blue closes in on a Link standing 1500 away");
  require(moved<=0.001f&&ai.end.z==0.0f,"the solo Blue does NOT approach Link at all");
  pass("solo_blue_does_not_approach_a_distant_link");}
 {World w;Native n;Link l;l.z=400;Native before=n;Brain b;bl::EvIn in;
  brain_pre(b,n,l,w,true);   // event running
  require(n.shape==before.shape&&n.speedF==before.speedF&&n.c0==before.c0&&n.c2==before.c2&&n.ce==before.ce&&n.da==before.da,"event running: PRE writes nothing");
  Native m=n;m.e3=1;brain_pre(b,m,l,w);require(m.shape==n.shape&&m.speedF==n.speedF&&m.ce==n.ce,"outside the leash (native MOVE_OUT owns it): no writes");
  for(int mode:{devf_variant::A_DAMAGE,devf_variant::A_BIG_DAMAGE,devf_variant::A_DEATH,devf_variant::A_WATER_DEATH,devf_variant::A_DEMO_MASTER,devf_variant::A_DEMO_CHILD,devf_variant::A_FALL_DEAD,devf_variant::A_FALL,devf_variant::A_WALK,devf_variant::A_TALK,devf_variant::A_WATCH}){
   Native t=n;t.mode=mode;t.state=4;const Native keep=t;Brain bb;brain_pre(bb,t,l,w);
   require(t.shape==keep.shape&&t.speedF==keep.speedF&&t.c0==keep.c0&&t.c2==keep.c2&&t.ce==keep.ce&&t.da==keep.da&&t.mode==keep.mode,"native damage / death / fall / water / demo / other modes are never touched");
  }
  pass("native_priority_event_leash_damage_death_fall_water_demo");}
 {World w;Native n;n.mode=bl::kModeAttack;n.state=1;n.anim=1;n.fc.set_anm(60,1.0f);n.fc.update();n.rng=7;Link l;l.z=300;
  Native keep=n;Brain b;brain_pre(b,n,l,w);require(n.shape==keep.shape&&n.speedF==keep.speedF&&n.fc.rate==keep.fc.rate,"a blow in progress is not touched by PRE");
  const auto ai=evade(w,keep,l,160,still);
  require(ai.end.blows==0||ai.max_blows==0||ai.attack_ticks>=9,"(blows counted only from state 0)");
  Native m=keep;Crowd c;auto* self=c.add(Variant::Blue,m.x,m.z);Brain bb;unsigned contact=0;
  for(unsigned t=0;t<120;++t){c.post(self,m.x,m.z);brain_pre(bb,m,l,w);native_execute(m,l,w,5000);brain_post(bb,m,c.L,*self,c.seq,dy::window_for(c.L.used));if(m.at_bit)++contact;}
  require(contact==9,"the blow that was already running completes natively (9 contact ticks) although the Blue is now solo");
  require(m.blows==0&&bb.vetoes==0,"and no new blow is started afterwards");
  pass("a_blow_in_progress_is_never_cancelled_by_the_solo_rule");}
 {World w;Native n;Link l;l.z=800;Crowd c;auto* self=c.add(Variant::Blue,n.x,n.z);Brain b;float rx=0,rz=0;float speed_before=0;unsigned level_at=0;s16 shape_at=0;
  for(unsigned t=0;t<140;++t){
   if(t==40){rx=n.x+40;rz=n.z+40;c.add(Variant::Red,rx,rz);}
   const float dx=n.x-l.x,dz=n.z-l.z,dd=std::sqrt(dx*dx+dz*dz);if(dd>1)l.z+=dz/dd*std::min(8.0f,dd),l.x+=dx/dd*std::min(8.0f,dd);l.speed=8;
   c.post(self,n.x,n.z);if(t>=40){for(auto& e:c.L.entries)if(e.used&&e.id!=self->id)c.post(&e,rx,rz);}
   brain_pre(b,n,l,w);native_execute(n,l,w,5000);brain_post(b,n,c.L,*self,c.seq,dy::window_for(c.L.used));
   if(t==39){speed_before=n.speedF;}
   if(t==41){level_at=b.lvl.level;shape_at=n.shape;}
  }
  require(speed_before>=15.0f,"it was running away");
  require(level_at==2,"a Red comes within 650: level 2 at once");
  const s16 pl=ref_atan2s(l.x-n.x,l.z-n.z);
  require(n.mode!=bl::kModeWait||ref_dist_angle(n.shape,pl)<0x4000,"the Blue did not fall into WAIT with its back to Link");
  require(ref_dist_angle(n.shape,pl)<0x1000||n.mode==bl::kModeAttack,"after the companion arrived the Blue turned back to face Link (native turn, no AI override) or is swinging");
  (void)shape_at;
  pass("a_companion_arriving_ends_the_evasion_at_once");}
 {World w;Native n;n.mode=bl::kModeWait;n.state=0;n.shape=0;Link l;l.z=700;l.speed=4;const auto ai=evade(w,n,l,300,toward(4.0f));
  require(ai.max_blows==0&&ai.attack_ticks==0,"a solo Blue that was idle and notices Link (WAIT -> FIND 1 -> ... -> 4) never attacks");
  require(ai.end.z<-100.0f||ai.dist.back()>=ai.dist.front()-200.0f,"and does not come closer");
  pass("solo_blue_waking_from_wait_flees_too");}
 {World w;Native n;Link l;l.z=800;
  const auto a=evade(w,n,l,100,toward(10.0f));const auto b=evade(w,n,l,100,toward(10.0f));
  require(a.dist==b.dist&&a.speed==b.speed&&a.evs==b.evs,"deterministic: same inputs, same trace");
  pass("evasion_is_deterministic");}
 {World w;Native n;Link l;l.z=900;const auto ai=evade(w,n,l,200,toward(10.0f));
  const auto& ev=ai.evs;require(!ev.empty()&&ev.front()==int(bl::EvEv::Start),"the first event is EVADE_START");
  require(std::count(ev.begin(),ev.end(),int(bl::EvEv::Start))==1&&ev.size()<=6,"few events, never per tick");
  pass("evasion_events_are_per_transition_not_per_tick");}
 {World w;Native n;Link l;l.z=1300;int resumes=0;
  auto drive=[&](unsigned t,Link& lk,Native&){lk.speed=0;if(t==30)lk.z=900;};
  const auto ai=evade(w,n,l,90,drive);
  for(int e:ai.evs)if(e==int(bl::EvEv::Resume))++resumes;
  require(ai.end.z<-50.0f,"Link steps inside the relevance radius: the Blue starts running (it did not before)");
  require(resumes>=1,"EVADE_RESUME is reported when Link enters the radius");
  pass("link_entering_the_relevance_radius_starts_the_run");}
 {Crowd c;auto* b1=c.add(Variant::Blue,0,0);auto* b2=c.add(Variant::Blue,200,0);c.post(b1,0,0);
  require(c.scan(b1).level()==2&&c.scan(b2).level()==2,"two different Blues are mutual companions: both level 2");
  require(c.scan(b1).ids[0]==b2->id&&c.scan(b2).ids[0]==b1->id,"each sees the other, not itself");
  auto* none=c.L.record(b1->id,&c.addr[50],99,Variant::Blue,1);require(!none,"the ledger refuses a second record with the same actor id: no duplicate identity can be counted twice");
  require(c.scan(b1).companions==1,"and still exactly one companion");
  auto* red=c.add(Variant::Red,100,0);auto* blk=c.add(Variant::Black,100,100);c.post(b1,0,0);c.post(b2,200,0);
  require(c.scan(b1).level()==3&&c.scan(b2).level()==3&&c.scan(red).level()==3&&c.scan(blk).level()==3,"with 4 Bokoblins every member sees 3 others and the level is capped at 3");
  pass("two_blues_and_mixed_groups_count_each_other_without_duplicates");}
 // ----- the heading choice itself (pure) -----
 {bl::Leash none;bl::Self self{0,0,0};
  auto clear_only=[&](std::vector<int> offs,int side=1){return [offs,side](s16 h)->bl::Probe{bl::Probe p;p.wall=1;   // blocked unless the heading is one of the listed offsets from away=0
     for(int o:offs)if(h==static_cast<s16>(o)){p.wall=0;for(unsigned i=0;i<bl::kSamples;++i){p.found[i]=true;p.ground_y[i]=0;}}return p;};};
  for(int off:{0,5461,-5461,10922,-10922,16383,-16383}){
   auto c=bl::choose(0,self,none,0,false,1,clear_only({off}));
   require(c.ok&&c.heading==static_cast<s16>(off)&&c.cls==bl::Cls::Clear&&c.offset==off,"every offset up to 90 degrees is reachable and reported");
  }
  for(int off:{19000,-19000,21845,-21845,27307,-27307,32767,-32768}){
   auto c=bl::choose(0,self,none,0,false,1,clear_only({off}));
   require(!c.ok,"a heading with a component TOWARD Link (more than 90 degrees from away) is never chosen");
  }
  {auto c=bl::choose(0,self,none,5461,true,1,clear_only({0,5461}));require(c.ok&&c.heading==5461,"the previous heading is kept while it stays safe (no dithering)");
   c=bl::choose(0,self,none,5461,true,1,clear_only({0}));require(c.ok&&c.heading==0,"and abandoned when it stops being safe");
   c=bl::choose(0,self,none,20000,true,1,clear_only({0,20000}));require(c.ok&&c.heading==0,"a previous heading beyond 90 degrees is not kept");}
  {auto c=bl::choose(0,self,none,0,false,1,clear_only({5461,-5461}));require(c.heading==5461,"preferred side first");
   c=bl::choose(0,self,none,0,false,-1,clear_only({5461,-5461}));require(c.heading==-5461,"and the other side when it was the last one used");}
  {int calls=0;auto c=bl::choose(0,self,none,0,false,1,[&](s16 h){++calls;bl::Probe p;p.wall=2;for(unsigned i=0;i<bl::kSamples;++i){p.found[i]=true;p.ground_y[i]=0;}return p;});
   require(c.ok&&c.cls==bl::Cls::Loose&&c.heading==0&&calls==7,"only LOOSE headings (a wall 100..300 ahead): the first one is used after looking at every candidate once");
   calls=0;c=bl::choose(0,self,none,0,false,1,[&](s16 h){++calls;bl::Probe p;p.wall=(h==5461)?0:2;for(unsigned i=0;i<bl::kSamples;++i){p.found[i]=true;p.ground_y[i]=0;}return p;});
   require(c.ok&&c.cls==bl::Cls::Clear&&c.heading==5461,"a CLEAR heading beats an earlier LOOSE one");}
  {bl::Leash lk;lk.active=true;lk.hx=0;lk.hz=0;lk.range=100;int calls=0;auto c=bl::choose(0,self,lk,0,false,1,[&](s16){++calls;return bl::Probe{};});
   require(!c.ok&&calls==0&&c.leash==7,"a leash of 100 makes every 300-unit run illegal and nothing is even probed");}
  {bl::Probe p;for(unsigned i=0;i<bl::kSamples;++i){p.found[i]=true;p.ground_y[i]=0;}
   require(bl::classify(p,0).cls==bl::Cls::Clear,"flat ground: clear");
   p.ground_y[1]=-101;require(bl::classify(p,0).why==bl::Why::Ledge,"a 101 drop between samples is a ledge");
   p.ground_y[1]=-100;p.ground_y[2]=-100;require(bl::classify(p,0).cls==bl::Cls::Clear,"a 100 step is walkable");
   p.ground_y[1]=0;p.ground_y[2]=0;p.ground_y[0]=101;require(bl::classify(p,0).why==bl::Why::Ledge,"a 101 rise is not a floor");
   p.ground_y[0]=0;p.water400=true;require(bl::classify(p,0).cls==bl::Cls::Loose,"water only at 400: loose");
   p.water200=true;require(bl::classify(p,0).why==bl::Why::Water,"water at 200 is blocked (the native test that would turn the chase into FIND_STAY)");}
  pass("heading_choice_best_first_within_90_degrees_and_ledge_water_leash_verdicts");}
 // ----- engine-shaped hazards -----
 {World w;w.low.push_back({-5000,-5000,5000,-200});Native n;Link l;l.z=300;const auto ai=evade(w,n,l,150,still);
  require(ai.never_fell,"lower terrain (a 300 drop) behind it: never stepped off");
  require(std::fabs(ai.end.x)>200.0f&&ai.end.y==0.0f,"it ran along the edge on the high ground");
  pass("solo_blue_never_steps_down_a_cliff_to_lower_ground");}
 {World w;w.wall.push_back({-80,-80,-60,60});w.wall.push_back({60,-80,80,60});w.wall.push_back({-80,-80,80,-60});   // 3 walls, open toward Link (+Z)
  Native n;Link l;l.z=330;const auto ai=evade(w,n,l,90,still);
  float moved=0;for(float s:ai.step)moved=std::max(moved,s);
  require(moved<=1.001f&&ai.end.z<=1.0f,"the only way out is TOWARD Link: it stays put instead");
  require(ai.max_blows==0&&ai.attack_ticks==0,"and does not attack");
  pass("solo_blue_never_flees_toward_link_even_when_that_is_the_only_exit");}
 {World w;w.water.push_back({-5000,-900,5000,-300});Native n;Link l;l.z=300;const auto ai=evade(w,n,l,100,still);   // a lake that starts 300 behind: the first probe (200 / 400) sees none of it, the native FIND_STAY test would fire once it is within 200
  bool find_stay=ai.end.mode==bl::kModeFindStay||ai.end.mode==bl::kModeMoveOut;
  require(ai.never_water&&!find_stay,"water appearing within 200 on the way: the per-tick re-check re-picks the heading before the native test turns the chase into FIND_STAY");
  pass("solo_blue_rechecks_water_every_tick");}
 {World w;w.water.push_back({-5000,-5000,-150,5000});Native n;n.shape=0;Link l;l.z=300;s16 mid_shape=0;float mid_z=0;
  const auto ai=evade(w,n,l,100,[&](unsigned t,Link& lk,Native& nn){lk.speed=0;if(t==20){mid_shape=nn.shape;mid_z=nn.z;}});   // deep water to the WEST: the shortest turn (via -X) would face it at 200
  bool stay=ai.end.mode==bl::kModeFindStay||ai.end.mode==bl::kModeMoveOut;
  require(!stay&&ai.never_water&&ai.attack_ticks==0,"the body turns the safe way: the native water test never turns the chase into FIND_STAY");
  require(ref_dist_angle(mid_shape,ref_atan2s(l.x,l.z-mid_z))>0x6000,"and it did complete the turn the long way (20 ticks in, it faces away from Link)");
  require(ai.end.z<-300.0f&&std::fabs(ai.end.x)<60.0f,"and the run along the heading (south) was not affected");
  pass("the_body_turn_avoids_facing_deep_water_even_when_it_is_the_short_way");}
 {bl::Evade e;bl::EvIn in;in.eligible=true;in.self=bl::Self{0,0,0};in.lx=0;in.lz=300;in.dist=300;in.shape_y=0;in.c2=0;in.speed=0;
  auto clear=[&](s16){bl::Probe p;for(unsigned i=0;i<bl::kSamples;++i){p.found[i]=true;p.ground_y[i]=0;}return p;};
  auto west=[&](s16 a){return a<0&&a>-32768;};   // deep water in the whole western half-plane
  s16 shape=0;unsigned ticks=0;bool wrote_west=false;
  for(;ticks<12;++ticks){in.shape_y=shape;const auto o=bl::step_evade(e,in,clear,west);if(o.w.turn){shape=o.w.shape_y;if(west(shape))wrote_west=true;}if(shape==static_cast<s16>(-32768)||shape==o.w.heading)break;}
  require(!wrote_west,"no step of the 180 degree turn ever faced the water");
  require(e.heading_valid&&shape==e.heading&&e.turn_sign==0&&ticks<=6,"it arrived at the heading in 4-5 steps and the turn sense was reset");
  unsigned full=0;auto probe=[&](s16){++full;bl::Probe p;p.wall=1;return p;};auto water=[&](s16){return false;};
  for(unsigned t=0;t<90;++t)bl::step_evade(e,in,probe,water);
  require(full<=7*(90/bl::kReeval+2),"a boxed-in Blue re-evaluates every kReeval ticks, not every tick");
  pass("hold_re_evaluation_is_bounded_in_cost");}
 std::printf("CP15_LAB_PASS cases=%u BLUE_AI_POLICY_AGAINST_A_SOURCE_PORT; ENGINE_AND_PIXELS_AND_SOUND_NOT_OBSERVED; GAME_NOT_EXECUTED\n",cases);
}
