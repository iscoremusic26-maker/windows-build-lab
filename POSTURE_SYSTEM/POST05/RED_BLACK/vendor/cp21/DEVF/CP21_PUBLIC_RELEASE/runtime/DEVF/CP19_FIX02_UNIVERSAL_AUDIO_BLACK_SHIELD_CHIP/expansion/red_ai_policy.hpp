#pragma once
// CP19 (0.16.0): reactions start their native B/C from <= kStrike after a final approach (close_in); everything else is CP17 FIX01.
// CP17 FIX01 Red-only value policy (Expansion 0.14.1). The same header is compiled by the DLL and the laboratory: no game pointers, no native writes here.
// The adapter (mod.cpp) samples native state, calls pre()/post()/rush_post()/sweep() and applies only the bounded commands they return.
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_red_ai {
using Variant=devf_variant::Variant;
constexpr unsigned kCapacity=64,kRandomPercent=12,kRandomMin=120,kRandomMax=210,kReactiveCooldown=45,kPassive=105,kPassiveMin=180,kPassiveMax=300,kIntent=90,kCalm=150;
constexpr float kNormal=1.15f,kBurst=2.0f,kRush=1.5f,kFindSpeed=20.0f;
constexpr double kReactRadius=900.,kPassiveMinDist=200.,kPassiveMaxDist=700.,kHeight=200.,kLeashMargin=100.;
// x2 is decided once at the fresh frame 1 and applied at frame 2 (even phase): own tick t+k (k>=2) starts at pre-play frame 2(k-1), so the native
// 14..22 attack bit is set on t+8..t+12 (pre 14/16/18/20/22), five registrations instead of four with the odd phase (15/17/19/21).
constexpr int kOpen=8,kClose=12,kInterceptFacing=0x1800,kForceFacing=0x1000,kDirectedAngle=0x1800,kInterruptWatch=30;
// Modelled reach of the native B/C at the active ticks (lunge included): Link within 260 XZ of the Red and Link's feet at most 110 above the Red's.
constexpr double kReachXZ=260.,kReachY=110.,kForceDist=400.,kDirectedDist=700.,kRushDistFree=302.,kRushDistLocked=402.,kRushLeashSlack=30.;
// Contact: native mSphs_at[0] tip (0,0,70) r45 and mSphs_at[1] hilt (0,0,20) r30 from joint 0xc; Link tg cylinders r35.
constexpr float kTipR=45.f,kHiltR=30.f,kLinkR=35.f,kLinkH=180.f,kSweepMinR=33.f,kMaxGap=150.f;
constexpr int Find=devf_variant::A_FIND,Attack=devf_variant::A_ATTACK,FindStay=devf_variant::A_FIND_STAY,Sound=devf_variant::A_SOUND_WATCH;
// daAlink_c PROC ids (official enum): 0x26/0x27 Jump Attack + land, 0x30/0x31 Jump Strike + land. Everything else is a negative by construction.
constexpr unsigned kProcCutJump=0x26,kProcCutJumpLand=0x27,kProcLargeJump=0x30,kProcLargeJumpLand=0x31;
struct Point{float x=0,y=0,z=0;};
inline bool finite(float x)noexcept{return x==x&&x>=-10000000.f&&x<=10000000.f;}
inline bool finite(Point p)noexcept{return finite(p.x)&&finite(p.y)&&finite(p.z);}
inline double xz2(Point a,Point b)noexcept{double x=double(a.x)-b.x,z=double(a.z)-b.z;return x*x+z*z;}
// No CRT math in the DLL (reviewed imports): Newton square root from above (monotone, exact to rounding) and a polynomial atan2 (|err| < 2.1e-4 rad, about 2 s16 units against facing bounds of 0x1000+).
inline double root(double x)noexcept{if(!(x>0))return 0;double r=x>1?x:1;for(int i=0;i<80;++i){const double n=0.5*(r+x/r);if(!(n<r))break;r=n;}return r;}
inline double arctan2(double y,double x)noexcept{
 const double ax=x<0?-x:x,ay=y<0?-y:y;if(ax==0&&ay==0)return 0;const double a=ax>ay?ay/ax:ax/ay,s=a*a;
 double r=((-0.0464964749*s+0.15931422)*s-0.327622764)*s*a+a;if(ay>ax)r=1.5707963267948966-r;if(x<0)r=3.141592653589793-r;return y<0?-r:r;
}
inline double d3(Point a,Point b)noexcept{double x=double(a.x)-b.x,y=double(a.y)-b.y,z=double(a.z)-b.z;return root(x*x+y*y+z*z);}
inline double absd(double x)noexcept{return x<0?-x:x;}
inline Point add(Point a,Point b)noexcept{return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Point sub(Point a,Point b)noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Point scale(Point a,float k)noexcept{return {a.x*k,a.y*k,a.z*k};}
inline double len(Point a)noexcept{return root(double(a.x)*a.x+double(a.y)*a.y+double(a.z)*a.z);}
// Native s16 yaw convention (cM_atan2s(dx,dz)); used only for facing bounds, never written.
inline int yaw(Point from,Point to)noexcept{const double a=arctan2(double(to.x)-from.x,double(to.z)-from.z)*32768.0/3.141592653589793;const long v=a>=0?long(a+0.5):-long(-a+0.5);return int(std::int16_t(std::uint16_t(std::uint32_t(v)&0xffffu)));}
inline int angle_gap(int a,int b)noexcept{const int d=int(std::int16_t(std::uint16_t((a-b)&0xffff)));return d<0?-d:d;}
// cLib_chaseF port (c_lib.cpp): step toward target, clamped on arrival.
inline float chase(float v,float target,float step)noexcept{if(v>target)step=-step;v+=step;if(step*(v-target)>=0)v=target;return v;}
// CP19 final approach for x2 blows (Red reactions, Black fury). The native FIND state 4 starts B/C as soon as 200<dist<400 and facing. The CP18 human log
// (54 native Black contacts) shows contacts at 93..266 (median 158) while blows started at a median 317 and 21 of 39 ended without contact. Inside
// (kStrike, kNativeAttackMax) the native attack switch is held by a 2-tick lockout lease (only when no native lockout runs); the native lockout branch
// then chases speedF toward 0 by 1/tick, so v+1 is passed and posMoveF moves v toward Link (native turn, wall avoidance, CrrPos and CC push kept).
// POST gives back the PRE speed and the lease. The blow itself starts at <= kStrike, the whole active window left to the native lunge.
constexpr double kStrike=240.,kNativeAttackMax=400.;constexpr float kCloseAccel=2.0f;
inline bool close_band(double dist)noexcept{return dist>kStrike&&dist<kNativeAttackMax;}
struct CloseIn{bool written=false,lease=false;float v0=0,v=0,own=0;Point from{};unsigned ticks=0,preempted=0;double moved=0;};
struct CloseCmd{bool write=false,lockout=false;float speed=0;};
inline CloseCmd close_in(CloseIn&c,Point from,float speed,int lockout,float target)noexcept{
 CloseCmd k;const float base=c.own>0?c.own:(speed>0?speed:0.0f);const float v=chase(base,target,kCloseAccel);
 k.write=true;k.speed=v+1.0f;k.lockout=lockout==0;c.written=true;c.lease=k.lockout;c.v0=speed;c.v=v;c.own=v;c.from=from;return k;
}
struct ClosePost{bool speed=false,lockout=false,preempted=false;float speed_value=0;};
inline ClosePost close_post(CloseIn&c,float speed,int lockout,Point pos)noexcept{
 ClosePost p;if(!c.written)return p;c.written=false;
 if(speed>=c.v-1e-3f&&speed<=c.v+1e-3f){p.speed=true;p.speed_value=c.v0;++c.ticks;const double dx=double(pos.x)-c.from.x,dz=double(pos.z)-c.from.z;double r=dx*dx+dz*dz,q=r>1?r:1;for(int i=0;i<60&&r>0;++i)q=0.5*(q+r/q);c.moved+=r>0?q:0;}
 else{p.preempted=true;++c.preempted;c.own=0;}   // a native or foreign speed write (watch, damage, mode change) is never overwritten
 if(c.lease&&lockout==1)p.lockout=true;          // our 2 after the native decrement; any other value is not ours
 c.lease=false;return p;
}
inline void close_reset(CloseIn&c)noexcept{c=CloseIn{};}
enum class Charge:std::uint8_t{None,Spin,Jump};
enum class Jump:std::uint8_t{None,JumpAttack,JumpStrike};
enum class Motion:std::uint8_t{Other,Quiet,Guard,Offensive,Dodge,Damage};
enum class Target:std::uint8_t{Unknown,ThisRed,None,Other};
enum class Cause:std::uint8_t{Normal,Random,ChargeSpin,ChargeJump,Passive,InterceptJumpAttack,InterceptJumpStrike,Count};
inline const char* cause_text(Cause c)noexcept{switch(c){case Cause::Random:return "RANDOM_OPPORTUNITY";case Cause::ChargeSpin:return "SPIN_OR_GREAT_SPIN_CHARGE";case Cause::ChargeJump:return "JUMP_STRIKE_CHARGE";case Cause::Passive:return "PASSIVITY_CONFIRMED";case Cause::InterceptJumpAttack:return "JUMP_ATTACK_INTERCEPT";case Cause::InterceptJumpStrike:return "JUMP_STRIKE_INTERCEPT";default:return "NORMAL_NATIVE_BLOW";}}
// Offensive priority: 1 jump interception, 2 charges, 3 passivity, 4 random, 5 normal.
inline int rank(Cause c)noexcept{switch(c){case Cause::InterceptJumpAttack:case Cause::InterceptJumpStrike:return 1;case Cause::ChargeSpin:case Cause::ChargeJump:return 2;case Cause::Passive:return 3;case Cause::Random:return 4;default:return 5;}}
inline bool intercept(Cause c)noexcept{return rank(c)==1;}
inline bool reactive(Cause c)noexcept{return rank(c)==2||rank(c)==3;}
inline bool interruptible(Cause c)noexcept{return rank(c)<=2;}
struct ChargeView{bool valid=false;Charge family=Charge::None;};
// Native daAlink PROC29/2F begin charge, 2A sustained; 28/30 are executing, not charging. Virtual calls are const daPy dispatch, never the mutating overload.
inline ChargeView charge_view(unsigned proc,int var2,bool any,bool spin,bool jump,unsigned proc_max)noexcept{
 ChargeView v;if(proc>=proc_max)return v;const bool sustained=proc==0x2a;
 if(any!=sustained||spin!=(sustained&&var2==0)||jump!=(sustained&&var2!=0))return v;
 v.valid=true;if(proc==0x29||(sustained&&var2==0))v.family=Charge::Spin;else if(proc==0x2f||(sustained&&var2!=0))v.family=Charge::Jump;return v;
}
// Exactly two human sword jump attacks. PROC26 is cross-checked with the const virtual checkCutJumpMode(); PROC30 must not report any charge virtual.
inline Jump jump_view(unsigned proc,bool cut_jump_mode,bool charge_any)noexcept{
 if(cut_jump_mode!=(proc==kProcCutJump))return Jump::None;
 if(proc==kProcCutJump)return Jump::JumpAttack;
 if(proc==kProcLargeJump&&!charge_any)return Jump::JumpStrike;
 return Jump::None;
}
inline bool damage_proc(unsigned proc)noexcept{return proc==0x32||proc==0x33||proc==0x34||proc==0x159||proc==0x15a||proc==0x1f;}
struct Actor{
 std::uint32_t id=0,gen=0;Variant variant=Variant::None;char stage[9]{};int room=-1,layer=-1,mode=-1,state=-1,health=0,angle=0,lockout=0;
 Point pos{},home{};float range=0,frame=0,rate=1,end=0,speed=0;bool ready=false,hit=false,leash_out=false,wall=false,incoming=false,perceived=false,b=false,c=false,stopped=false,cutdown=false;int playmode=0;
 // POST-only collider sample (cc_set already ran). Pointer identities are resolved by the adapter into these booleans.
 Point tip{},hilt{};float tip_r=0,hilt_r=0;bool at_set=false,at_hit=false,hit_link=false;
};
struct Link{
 std::uint32_t id=0;bool sampled=false,present=false,basic=false,canary=false,safe=false,los=false;char stage[9]{};int room=-1,layer=-1,proc=-1,timer=-1,heading=0;
 // vel.x/z: measured displacement between two consecutive own PRE samples (vel_valid); vel.y, gravity, ground_y, grounded: Link's own fields.
 Point pos{},vel{};float gravity=0,ground_y=0;bool vel_valid=false,grounded=true;Charge charge=Charge::None;Jump jump=Jump::None;Motion motion=Motion::Other;Target target=Target::Unknown;
};
inline bool context(const Actor&a,const Link&l)noexcept{return a.stage[0]&&std::memcmp(a.stage,l.stage,9)==0&&a.room==l.room&&a.layer==l.layer&&a.layer>=0&&a.layer<15;}
inline bool domain(const Actor&a)noexcept{return a.id&&a.gen&&a.variant==Variant::Red&&a.stage[0]&&a.room>=0&&a.room<64&&a.layer>=0&&a.layer<15&&finite(a.pos)&&finite(a.home)&&finite(a.range)&&a.range>=0;}
inline bool safe(const Actor&a)noexcept{return domain(a)&&a.health>0&&a.ready&&!a.hit&&!a.leash_out&&!a.cutdown&&!devf_variant::blocks_regen(a.mode);}
inline bool fight(int m)noexcept{return m==Find||m==FindStay||m==Attack;}
inline bool blow(const Actor&a)noexcept{return a.mode==Attack&&((a.state==1&&a.b)||(a.state==2&&a.c));}
inline bool sameclip(const Actor&a,int clip)noexcept{return clip==5?a.b:clip==6?a.c:false;}
inline bool inside(const Actor&a,Point p,double slack)noexcept{return a.range==0||(a.range>=kLeashMargin+slack&&xz2(a.home,p)<=double(a.range-kLeashMargin-slack)*(a.range-kLeashMargin-slack));}
inline bool leash(const Actor&a,Point dest)noexcept{return finite(dest)&&finite(a.home)&&finite(a.pos)&&finite(a.range)&&a.range>=0&&!a.leash_out&&(a.range==0||(a.range>=kLeashMargin&&xz2(a.home,dest)<=double(a.range-kLeashMargin)*(a.range-kLeashMargin)&&xz2(a.home,a.pos)<=double(a.range)*a.range));}
inline bool viable(const Actor&a,const Link&l)noexcept{return safe(a)&&l.present&&l.basic&&l.canary&&l.safe&&l.los&&finite(l.pos)&&context(a,l)&&a.perceived&&!a.wall&&leash(a,l.pos)&&absd(double(a.pos.y)-l.pos.y)<=kHeight;}
// Interception tolerates Link being airborne (height check against the predicted contact instead of the current sample) but keeps every other gate.
inline bool viable_air(const Actor&a,const Link&l)noexcept{return safe(a)&&l.present&&l.basic&&l.canary&&l.safe&&l.los&&finite(l.pos)&&finite(l.vel)&&finite(l.gravity)&&finite(l.ground_y)&&context(a,l)&&a.perceived&&!a.wall&&leash(a,l.pos);}
struct Blow{Cause cause=Cause::Normal;unsigned seq=0,start=0;bool contact=false,link=false,shield=false,damage=false;};
struct State{
 std::uint32_t id=0,gen=0,rng=1,link_id=0;char stage[9]{};int room=-1,layer=-1;
 unsigned ticks=0,next_random=0,next_reactive=0,next_passive=0,passive=0,attack_seq=0,logs=0,crit_logs=0,last_threat=0;
 bool link_pass_logged=false,random_allowed=false,combat=false,normal_since_random=true,link_fault=false,decided=false,owned=false,pre_valid=false;
 // Charge episode: consumed only when an attempt (forced or native fresh blow with that cause) starts. A failed tick never consumes it.
 Charge episode=Charge::None;bool episode_reacted=false,episode_selected=false,episode_deferred_logged=false;unsigned last_jump_charge_tick=0;bool last_jump_charge=false;
 // Jump episode: one decision (attempt or final miss) per PROC26/30 entry.
 Jump jump=Jump::None;bool jump_done=false,jump_wait_logged=false;unsigned jump_start=0;const char* jump_block="NONE";
 // Intent (charge/passive) or forced interception waiting for the fresh native blow.
 bool intent=false,attempted=false,forced=false,rush=false;Cause intent_cause=Cause::Normal,forced_cause=Cause::Normal,blow_cause=Cause::Normal;unsigned intent_start=0,forced_tick=0;Point intent_pos{};
 float own_rate=1,last_frame=0;int clip=0;int pre_mode=-1,pre_state=-1;float pre_frame=0;bool prev_find4=false,pending=false;
 // Rush (FIND state 4 only): exact native chase reproduction and restoration of the native speed variable after posMoveF.
 bool rush_written=false;float rush_v0=0,rush_x=0,rush_r=0,rush_n=0;Point rush_from{};unsigned rush_ticks=0,rush_preempted=0;double rush_moved=0,rush_native=0;
 // Contact sweep: previous own native poses of this owned x2 blow.
 bool pose_valid=false,pose2_valid=false,registered=false;Point tip_p{},hilt_p{},tip_pp{},hilt_pp{};float pose_frame=0;unsigned sweeps=0,sweep_kept=0;
 Blow last;unsigned watch_until=0;Point link_pos{};unsigned link_pass=0;bool link_vel_valid=false;std::uint32_t sample_id=0;std::uint8_t sample_flags=0xff;
 int pre_link_proc=-1;
 CloseIn close{};   // CP19 final approach of a reaction
};
inline std::uint32_t random(State&s)noexcept{auto x=s.rng? s.rng:0x6d2b79f5u;x^=x<<13;x^=x>>17;x^=x<<5;return s.rng=x;}
inline unsigned between(State&s,unsigned lo,unsigned hi)noexcept{return lo+random(s)%(hi-lo+1);}
inline bool chance(State&s,unsigned percent)noexcept{return random(s)%100<percent;}
inline void seed(State&s,std::uint32_t id,std::uint32_t gen,std::uint32_t stamp)noexcept{s.rng=(id*0x9e3779b9u)^(gen*0x85ebca6bu)^(stamp*0xc2b2ae35u)^0x27d4eb2du;if(!s.rng)s.rng=1;}
inline State& state_for(std::array<State,kCapacity>&states,unsigned slot,const Actor&a,std::uint32_t stamp)noexcept{
 auto&s=states[slot<kCapacity?slot:0];if(s.id!=a.id||s.gen!=a.gen||s.room!=a.room||s.layer!=a.layer||std::memcmp(s.stage,a.stage,9)!=0){s={};s.id=a.id;s.gen=a.gen;s.room=a.room;s.layer=a.layer;std::memcpy(s.stage,a.stage,9);seed(s,a.id,a.gen,stamp);}return s;
}
// LINK_REACTIVE_SAMPLE_OBSERVED is a transition, not a per-tick line: once per (Link id, actor, generation, context/canary class).
inline bool sample_transition(State&s,const Link&l,bool ctx)noexcept{
 const std::uint8_t f=std::uint8_t((l.present?1:0)|(l.basic?2:0)|(ctx?4:0)|(l.canary?8:0));
 if(s.sample_id==l.id&&s.sample_flags==f)return false;s.sample_id=l.id;s.sample_flags=f;return true;
}
// Telemetry budget with a separate critical reserve so routine lines can never starve attribution of special blows.
inline bool critical(const char*e,Cause c)noexcept{
 if(c!=Cause::Normal&&(!std::strcmp(e,"NATIVE_ATTACK_STARTED")||!std::strcmp(e,"RATE_APPLIED_READBACK")||!std::strcmp(e,"NATIVE_ATTACK_ENDED")))return true;
 for(const char* p:{"JUMP_","INTERCEPT","CHARGE_REACTION","NATIVE_CONTACT","SHIELD_","DAMAGE_","INTERRUPTION_","RUSH_MEASURED","RATE_READBACK_FAILED","NATIVE_ATTACK_FORCED","REACTION_SELECTED","REACTION_UPGRADED","CLOSE_IN_STARTED","CLOSE_IN_MEASURED","CLOSE_IN_UNREACHABLE"})if(!std::strncmp(e,p,std::strlen(p)))return true;
 return false;
}
struct Budget{
 static constexpr unsigned kGlobal=500,kActor=30,kCritGlobal=500,kCritActor=48;unsigned global=0,crit=0,suppressed=0,crit_suppressed=0;
 bool take(State&s,bool is_critical)noexcept{
  if(is_critical){if(crit<kCritGlobal&&s.crit_logs<kCritActor){++crit;++s.crit_logs;return true;}++crit_suppressed;return false;}
  if(global>=kGlobal||s.logs>=kActor){++suppressed;return false;}++global;++s.logs;return true;
 }
};
// RUSH_MEASURED is emitted while the rush counters are still in State (the adapter prints them), then the counters reset.
template<class Emit>inline void end_rush(State&s,Emit&&emit,Cause c)noexcept{if(s.rush_ticks)emit("RUSH_MEASURED",c,"MEASURED_XZ_DISPLACEMENT_VS_NATIVE_SPEED_SUM_SEE_RATIO");s.rush=false;s.rush_ticks=s.rush_preempted=0;s.rush_moved=s.rush_native=0;}
template<class Emit>inline void cancel(State&s,Emit&&emit,const char*why)noexcept{if(s.intent){emit("SPECIAL_CANCELLED",s.intent_cause,why);if(s.close.ticks||s.close.preempted)emit("CLOSE_IN_UNREACHABLE",s.intent_cause,"STRIKE_DISTANCE_NOT_REACHED_BEFORE_THE_REACTION_ENDED_NO_BLOW_FORCED");close_reset(s.close);end_rush(s,emit,s.intent_cause);s.intent=false;}}
inline bool random_ready(const State&s)noexcept{return s.ticks>=s.next_random&&s.normal_since_random;}
inline bool reactive_ready(const State&s)noexcept{return s.ticks>=s.next_reactive;}
template<class Emit>inline void select(State&s,Cause why,Point pos,Emit&&emit)noexcept{
 s.intent=true;s.rush=false;s.attempted=false;s.intent_cause=why;s.intent_start=s.ticks;s.intent_pos=pos;
 if(why==Cause::Passive){s.next_passive=s.ticks+between(s,kPassiveMin,kPassiveMax);s.passive=0;}
 emit("REACTION_SELECTED",why,why==Cause::Passive?"PASSIVITY_105_OWN_SAMPLES_RUSH_THEN_NATIVE_ATTACK_X2_NOT_A_FORCED_HIT":"EVERY_ELIGIBLE_CHARGE_EPISODE_NO_ROLL_RUSH_THEN_NATIVE_ATTACK_X2_NOT_A_FORCED_HIT");
}
// Commands PRE asks the adapter to apply before the native execute. All are optional and mutually consistent.
struct Command{bool force=false,zero_speed=false,rush=false,close=false,close_lockout=false;float speed=0;Cause cause=Cause::Normal;const char* why="NONE";};
// Can the native E_OC attack be started now without bypassing a native priority? FIND state 4 (floor ahead passed last tick) or ATTACK state 0.
inline const char* force_block(const State&s,const Actor&a,const Link&l,int facing_limit,bool lockout_matters)noexcept{
 if(!safe(a))return "RED_NATIVE_PRIORITY_UNSAFE";
 if(a.incoming)return "RECRUITMENT_ORDER_OWNS_RED";
 if(a.wall)return "RED_WALL_CONTACT";
 if(!inside(a,a.pos,0))return "RED_LEASH_MARGIN";
 const bool find4=a.mode==Find&&a.state==4&&s.prev_find4,attack0=a.mode==Attack&&a.state==0;
 if(!find4&&!attack0)return a.mode==Attack?"RED_NATIVE_CLIP_IN_PROGRESS":"RED_NOT_IN_NATIVE_FIND_APPROACH";
 if(find4&&lockout_matters&&a.lockout!=0)return "NATIVE_POST_ATTACK_LOCKOUT";
 if(angle_gap(a.angle,yaw(a.pos,l.pos))>facing_limit)return "RED_NOT_FACING_LINK";
 return nullptr;
}
template<class Emit>inline Command forced(State&s,Actor&a,Cause cause,const char*why,Emit&&emit)noexcept{
 Command c;c.force=a.mode!=Attack;c.zero_speed=a.speed!=0;c.cause=cause;c.why=why;
 s.forced=true;s.forced_cause=cause;s.forced_tick=s.ticks;s.attempted=true;s.intent=false;s.rush=false;
 s.pre_mode=Attack;s.pre_state=0;   // the adapter applies setActionMode(ATTACK,0) before the native execute
 emit("NATIVE_ATTACK_FORCED",cause,why);return c;
}
// Interception timing (pure prediction from two consecutive own PRE samples of Link; gravity/vertical speed from Link's own fields).
// count = active ticks (k in [kOpen,kClose], strictly before the landing tick) with Link predicted inside reach if the attack starts now;
// later = the same if it starts one tick later. Start when count>0 and waiting cannot increase it (descending Jump Strike waits, Jump Attack starts at once).
struct Plan{int land=-1,count=0,later=0;double best=1e9,best_y=0;int best_k=-1;bool approaching=false,reach=false;bool start()const{return count>0&&count>=later;}};
inline Plan plan(const Actor&a,const Link&l)noexcept{
 Plan p;const double tx=double(a.pos.x)-l.pos.x,tz=double(a.pos.z)-l.pos.z;p.approaching=tx*l.vel.x+tz*l.vel.z>0;
 const double g=l.gravity,ground=l.ground_y;if(!(g<0)||(l.grounded&&l.vel.y<=0))return p;   // not airborne (yet)
 double yy=l.pos.y,vv=l.vel.y;int land=-1;
 for(int k=1;k<=60;++k){vv+=g;if(vv<-60)vv=-60;yy+=vv;if(vv<0&&yy<=ground){land=k;break;}}
 p.land=land;if(land<0)return p;
 yy=l.pos.y;vv=l.vel.y;for(int k=1;k<=kClose+1&&k<land;++k){vv+=g;if(vv<-60)vv=-60;yy+=vv;if(k<kOpen)continue;
  const Point at{float(l.pos.x+l.vel.x*k),float(yy),float(l.pos.z+l.vel.z*k)};const double d=root(xz2(a.pos,at)),h=yy-a.pos.y;
  const bool in=d<=kReachXZ&&h<=kReachY;if(k>kOpen)p.later+=in;if(k>kClose)continue;
  p.count+=in;if((in&&!p.reach)||(in==p.reach&&d<p.best)){p.best=d;p.best_y=h;p.best_k=k;p.reach=in;}}
 return p;
}
inline bool directed(const Actor&a,const Link&l)noexcept{
 if(l.target==Target::ThisRed)return true;
 if(l.target!=Target::None)return false;
 return xz2(a.pos,l.pos)<=kDirectedDist*kDirectedDist&&angle_gap(l.heading,yaw(l.pos,a.pos))<=kDirectedAngle;
}
template<class Emit>inline void jump_final(State&s,Emit&&emit,const char*event,const char*why)noexcept{if(!s.jump_done){s.jump_done=true;emit(event,s.jump==Jump::JumpStrike?Cause::InterceptJumpStrike:Cause::InterceptJumpAttack,why);}}
// PRE: observe Link, maintain episodes, choose at most one bounded command for this own execute.
template<class Emit>inline Command pre(State&s,Actor&a,const Link&l,Emit&&emit)noexcept{
 Command cmd;++s.ticks;s.random_allowed=l.present&&l.basic&&l.safe&&context(a,l);s.pre_valid=safe(a);s.pre_mode=a.mode;s.pre_state=a.state;s.pre_frame=a.frame;
 s.forced=false;s.rush_written=false;
 const bool inblow=blow(a);const bool find4_now=a.mode==Find&&a.state==4;
 struct Find4{State&s;bool now;~Find4(){s.prev_find4=now;}}find4_guard{s,find4_now};
 if(!domain(a)){cancel(s,emit,"IDENTITY_CONTEXT_DOMAIN");s.pre_valid=false;return cmd;}
 if(s.watch_until&&l.present&&l.id==s.link_id&&s.last.link&&!s.last.shield){
  const Cause c=s.last.cause;
  if(l.proc>=0&&damage_proc(unsigned(l.proc))){s.watch_until=0;emit("INTERRUPTION_OBSERVED",c,"LINK_ENTERED_NATIVE_DAMAGE_PROC_AFTER_IDENTIFIED_CONTACT");}
  else if(l.proc==int(kProcCutJumpLand)||l.proc==int(kProcLargeJumpLand)||l.proc==0x28){s.watch_until=0;emit("INTERRUPTION_NOT_OBSERVED",c,"LINK_COMPLETED_LANDING_OR_RELEASE_PROC");}
  else if(s.ticks>=s.watch_until){s.watch_until=0;emit("INTERRUPTION_UNKNOWN",c,"NO_DAMAGE_PROC_OR_COMPLETION_OBSERVED_WITHIN_30_OWN_TICKS");}
 }
 if(l.present&&l.id!=s.link_id){cancel(s,emit,"LINK_ID_CHANGED");s.link_id=l.id;s.episode=Charge::None;s.episode_reacted=s.episode_selected=false;s.jump=Jump::None;s.jump_done=false;s.passive=0;s.link_fault=false;s.link_pass_logged=false;s.combat=false;s.watch_until=0;s.last_jump_charge=false;}
 // Jump episode bookkeeping happens before any safety return so a missed attack is still reported once.
 if(l.present&&l.basic&&l.canary&&l.id==s.link_id&&context(a,l)){
  if(l.jump!=s.jump){
   if(s.jump!=Jump::None)jump_final(s,emit,"INTERCEPT_MISSED",s.jump_block);
   s.jump=l.jump;s.jump_done=false;s.jump_wait_logged=false;s.jump_start=s.ticks;s.jump_block="EPISODE_ENDED_BEFORE_ANY_ELIGIBLE_TICK";
   if(l.jump!=Jump::None){
    const Cause c=l.jump==Jump::JumpStrike?Cause::InterceptJumpStrike:Cause::InterceptJumpAttack;
    emit("JUMP_ATTACK_DETECTED",c,l.jump==Jump::JumpStrike?"PROC30_JUMP_STRIKE_CONST_VIRTUALS_COHERENT":"PROC26_JUMP_ATTACK_CHECKCUTJUMPMODE_COHERENT");
    // Same Jump Strike episode as a held charge: one reaction across charge and jump.
    const bool continued=l.jump==Jump::JumpStrike&&s.last_jump_charge&&s.ticks-s.last_jump_charge_tick<=3;
    if(continued&&s.episode_reacted)jump_final(s,emit,"INTERCEPT_SKIPPED","SAME_JUMP_STRIKE_EPISODE_ALREADY_REACTED_TO_CHARGE");
    else if(continued&&s.intent&&s.intent_cause==Cause::ChargeJump){end_rush(s,emit,Cause::ChargeJump);s.intent=false;emit("REACTION_UPGRADED",c,"PENDING_JUMP_STRIKE_CHARGE_INTENT_BECOMES_THE_SAME_EPISODE_INTERCEPTION");}
   }
  }
  if(l.charge==Charge::Jump){s.last_jump_charge=true;s.last_jump_charge_tick=s.ticks;}else if(s.ticks-s.last_jump_charge_tick>3)s.last_jump_charge=false;
 }
 if(s.combat&&s.ticks-s.last_threat>=kCalm&&(!safe(a)||!fight(a.mode)||!l.present))s.combat=false;
 if(!safe(a)||!fight(a.mode)||(a.mode==Attack&&a.state>=3)||a.incoming){
  cancel(s,emit,a.incoming?"RECRUITMENT_OWNS_RECEIVER_NO_RED_RUSH":"NATIVE_PRIORITY_DAMAGE_GUARD_LEASH_EVENT_OR_OTHER_MODE");
  if(s.jump!=Jump::None&&!s.jump_done)s.jump_block=a.incoming?"RECRUITMENT_ORDER_OWNS_RED":"RED_NATIVE_PRIORITY_OR_RECOIL";
  const double d=xz2(a.pos,l.pos);const bool pause_guard=s.combat&&safe(a)&&a.mode==Attack&&a.state==3&&a.rate<0&&viable(a,l)&&l.charge==Charge::None&&(l.motion==Motion::Guard||l.motion==Motion::Quiet)&&d>=kPassiveMinDist*kPassiveMinDist&&d<=kPassiveMaxDist*kPassiveMaxDist;
  if(!pause_guard)s.passive=0;return cmd;
 }
 if(l.present&&l.basic&&a.perceived&&context(a,l)){s.combat=true;s.last_threat=s.ticks;}
 if(s.combat&&s.ticks-s.last_threat>=kCalm){s.combat=false;cancel(s,emit,"150_OWN_EXECUTES_WITHOUT_CURRENT_NATIVE_PERCEPTION");}
 if(l.present&&l.basic&&l.canary&&!s.link_pass_logged){s.link_pass_logged=true;emit("INDEPENDENT_LINK_STATE_CANARY_PASS",Cause::Normal,"PROC_DOMAIN_AND_CONST_VIRTUAL_SEMANTICS_COHERENT_NOT_PHYSICAL_LAYOUT_HUMAN_CERTIFICATION");}
 if(l.present&&l.basic&&!l.canary&&!s.link_fault){s.link_fault=true;emit("INDEPENDENT_LINK_STATE_CANARY_FAILED_REACTIVE_DISABLED",Cause::Normal,"CP14_CANARY_UNCHANGED_NORMAL_REMAINS_RANDOM_REQUIRES_SAFE_BASIC_LINK");}
 // ---- Priority 1: interception of exactly Jump Attack (PROC26) and Jump Strike (PROC30) ----
 if(s.jump!=Jump::None&&!s.jump_done&&!s.link_fault){
  const Cause c=s.jump==Jump::JumpStrike?Cause::InterceptJumpStrike:Cause::InterceptJumpAttack;
  if(inblow){jump_final(s,emit,"INTERCEPT_SKIPPED","OWN_NATIVE_BLOW_ALREADY_RUNNING_NO_RETIMING");}
  else if(!s.combat||!viable_air(a,l)){s.jump_block="LINK_CONTEXT_LOS_LEASH_OR_CANARY_UNSAFE";}
  else if(!directed(a,l)){jump_final(s,emit,"INTERCEPT_SKIPPED",l.target==Target::Other?"LINK_TARGETS_ANOTHER_ACTOR":"NOT_DIRECTED_AT_THIS_RED");}
  else{
   const Plan p=plan(a,l);
   const bool grounded=l.grounded&&l.vel.y<=0;   // PROC30 before take-off (or a sample before the PROC26 impulse): wait for the native jump
   if(!l.vel_valid){s.jump_block="TRAJECTORY_SAMPLE_PENDING";}
   else if(grounded){s.jump_block="LINK_TAKEOFF_PENDING";if(!s.jump_wait_logged){s.jump_wait_logged=true;emit("INTERCEPT_WAITING",c,"LINK_TAKEOFF_PENDING");}}
   else if(p.land>=0&&p.land-1<kOpen){jump_final(s,emit,"INTERCEPT_MISSED_NOT_ENOUGH_TIME",std::strcmp(s.jump_block,"EPISODE_ENDED_BEFORE_ANY_ELIGIBLE_TICK")&&std::strcmp(s.jump_block,"TRAJECTORY_SAMPLE_PENDING")?s.jump_block:"PREDICTED_LANDING_BEFORE_NATIVE_X2_ACTIVE_WINDOW");}
   else if(p.land<0){s.jump_block="LINK_TRAJECTORY_NOT_PREDICTABLE";}
   else if(!p.start()){
    s.jump_block=p.count?"WAITING_FOR_MORE_ACTIVE_TICKS_IN_REACH":p.approaching?"OUT_OF_REACH_OR_TOO_HIGH_WAITING_FOR_DESCENT_OR_APPROACH":"TRAJECTORY_NOT_APPROACHING_THIS_RED";
    if(!s.jump_wait_logged){s.jump_wait_logged=true;emit("INTERCEPT_WAITING",c,s.jump_block);}
   }else if(const char* why=force_block(s,a,l,kInterceptFacing,false)){s.jump_block=why;if(!s.jump_wait_logged){s.jump_wait_logged=true;emit("INTERCEPT_WAITING",c,why);}}
   else{s.jump_done=true;s.episode_reacted=s.episode_reacted||s.last_jump_charge;emit("INTERCEPT_STARTED",c,"PREDICTED_CONTACT_INSIDE_X2_ACTIVE_WINDOW_BEFORE_LANDING");return forced(s,a,c,"JUMP_INTERCEPTION_NATIVE_ATTACK_STARTED_BEFORE_IMPACT",emit);}
  }
 }
 const bool ok=s.combat&&!s.link_fault&&viable(a,l);
 if(!ok){cancel(s,emit,"LINK_REACTIVE_SAMPLE_CONTEXT_LOS_GROUND_LEASH_OR_CANARY_UNSAFE");s.passive=0;return cmd;}
 // ---- Charge episodes: every eligible episode reacts (no roll); eligibility re-evaluated each tick until an attempt starts ----
 if(s.episode!=l.charge){
  if(s.episode!=Charge::None&&s.episode_selected&&!s.episode_reacted&&s.intent&&s.jump==Jump::None)cancel(s,emit,"CHARGE_FINISHED_CANCELLED_OR_EXECUTING_BEFORE_NATIVE_BLOW");
  s.episode=l.charge;s.episode_reacted=false;s.episode_selected=false;s.episode_deferred_logged=false;
  if(l.charge!=Charge::None)emit("ACTUAL_CHARGE_DETECTED",l.charge==Charge::Spin?Cause::ChargeSpin:Cause::ChargeJump,"VALIDATED_PROC_AND_CONST_VIRTUALS_NOT_A_HIT_OR_INTERRUPT");
 }
 if(s.intent){
  const bool charge_intent=s.intent_cause==Cause::ChargeSpin||s.intent_cause==Cause::ChargeJump;
  const Charge required=s.intent_cause==Cause::ChargeJump?Charge::Jump:Charge::Spin;
  if(s.ticks-s.intent_start>=kIntent){cancel(s,emit,"90_OWN_EXECUTE_INTENT_DEADLINE");if(charge_intent)s.episode_selected=false;}
  else if(charge_intent&&l.charge!=required)cancel(s,emit,"CHARGE_FINISHED_CANCELLED_OR_EXECUTING_BEFORE_NATIVE_BLOW");
  else if(!charge_intent&&l.charge!=Charge::None)cancel(s,emit,"CHARGE_PRIORITY_OVER_PASSIVITY");
  else if(s.intent_cause==Cause::Passive&&l.motion!=Motion::Quiet&&l.motion!=Motion::Guard)cancel(s,emit,"LINK_NO_LONGER_PASSIVE");
 }
 const double d=xz2(a.pos,l.pos);
 const bool quiet=l.charge==Charge::None&&(l.motion==Motion::Quiet||l.motion==Motion::Guard)&&d>=kPassiveMinDist*kPassiveMinDist&&d<=kPassiveMaxDist*kPassiveMaxDist;
 if(quiet){if(s.passive<kPassive)++s.passive;}else s.passive=0;
 if(!inblow&&l.charge!=Charge::None&&!s.episode_reacted&&!s.intent){
  const auto c=l.charge==Charge::Spin?Cause::ChargeSpin:Cause::ChargeJump;
  const char* defer=d>kReactRadius*kReactRadius?"OUTSIDE_900_REACTION_RADIUS":!reactive_ready(s)?"REACTIVE_COOLDOWN":nullptr;
  if(!defer){s.episode_selected=true;s.episode_deferred_logged=false;select(s,c,a.pos,emit);}
  else if(!s.episode_deferred_logged){s.episode_deferred_logged=true;emit("CHARGE_REACTION_DEFERRED",c,defer);}
 }else if(!inblow&&l.charge==Charge::None&&!s.intent&&quiet&&s.passive>=kPassive&&s.ticks>=s.next_passive&&reactive_ready(s))select(s,Cause::Passive,a.pos,emit);
 if(!s.intent||s.attempted||inblow)return cmd;
 // ---- Intent execution: start the native attack when the native constraints (lockout, facing, range) allow; otherwise rush in FIND ----
 const double dist=d3(a.pos,l.pos);
 if(dist<=kStrike&&!force_block(s,a,l,kForceFacing,true)){
  if(s.intent_cause==Cause::ChargeSpin||s.intent_cause==Cause::ChargeJump)s.episode_reacted=true;
  if(s.close.ticks)emit("CLOSE_IN_MEASURED",s.intent_cause,"FINAL_APPROACH_TO_STRIKE_DISTANCE_240_BEFORE_THE_X2_BLOW");
  close_reset(s.close);
  return forced(s,a,s.intent_cause,"REACTION_NATIVE_ATTACK_STARTED_AT_STRIKE_DISTANCE",emit);
 }
 // Final approach: hold the native attack switch in (kStrike,400) and close in with the native chase at the rush speed.
 if(close_band(dist)&&a.mode==Find&&a.state==4&&s.prev_find4&&safe(a)&&!a.incoming&&!a.wall&&a.speed>=0&&inside(a,a.pos,kRushLeashSlack)){
  if(!s.close.ticks&&!s.close.preempted)emit("CLOSE_IN_STARTED",s.intent_cause,"NATIVE_ATTACK_SWITCH_HELD_OUTSIDE_STRIKE_DISTANCE_NATIVE_CHASE_AT_X1_5");
  const auto k=close_in(s.close,a.pos,a.speed,a.lockout,kRush*kFindSpeed);
  cmd.close=true;cmd.close_lockout=k.lockout;cmd.speed=k.speed;cmd.cause=s.intent_cause;cmd.why="CLOSE_IN";return cmd;
 }
 if(a.mode==Find&&a.state==4&&s.prev_find4&&safe(a)&&!a.incoming&&!a.wall&&a.speed>=0&&a.speed<=kFindSpeed&&a.lockout>=0){
  // Native executeFind state 4 chases speedF toward 20 exactly when (lockout==0 && dist>=300) || (lockout!=0 && dist>=400) without an event; margins avoid float boundary.
  const bool approach=a.lockout==0?dist>=kRushDistFree:dist>=kRushDistLocked;
  const Point ahead{a.pos.x+(l.pos.x-a.pos.x)*float(kRushLeashSlack/(dist>1?dist:1)),a.pos.y,a.pos.z+(l.pos.z-a.pos.z)*float(kRushLeashSlack/(dist>1?dist:1))};
  if(approach&&inside(a,a.pos,kRushLeashSlack)&&inside(a,ahead,kRushLeashSlack)){
   const float n=chase(a.speed,kFindSpeed,1.0f),r=kRush*n,x=r>kFindSpeed?r+1.0f:r-1.0f;
   if(chase(x,kFindSpeed,1.0f)==r){
    if(!s.rush){s.rush=true;s.rush_ticks=0;s.rush_preempted=0;s.rush_moved=0;s.rush_native=0;emit("RUSH_STARTED",s.intent_cause,"NATIVE_FIND_STATE4_APPROACH_X1_5_DISPLACEMENT_SPEED_RESTORED_AFTER_POSMOVE");}
    s.rush_written=true;s.rush_v0=a.speed;s.rush_x=x;s.rush_r=r;s.rush_n=n;s.rush_from=a.pos;
    cmd.rush=true;cmd.speed=x;cmd.cause=s.intent_cause;cmd.why="RUSH";
   }
  }
 }
 return cmd;
}
// After the native execute (posMoveF/CrrPos already ran): put back the native speed variable so the boost never leaks into a later native decision.
struct RushPost{bool write=false;float value=0;};
template<class Emit>inline RushPost rush_post(State&s,const Actor&a,Emit&&emit)noexcept{
 RushPost out;if(!s.rush_written)return out;s.rush_written=false;
 const double moved=root(xz2(s.rush_from,a.pos));
 if(absd(double(a.speed)-s.rush_r)<=1e-3){out={true,s.rush_n};++s.rush_ticks;s.rush_moved+=moved;s.rush_native+=s.rush_n;}
 else if(absd(double(a.speed)-s.rush_x)<=1e-3){out={true,s.rush_v0};++s.rush_preempted;++s.rush_ticks;s.rush_moved+=moved;s.rush_native+=s.rush_v0;emit("RUSH_TICK_PREEMPTED_BY_NATIVE",s.intent_cause,"NATIVE_WATCH_OR_FLOOR_STOP_BEFORE_CHASE_SPEED_RESTORED_TO_PRE_VALUE");}
 else{s.rush=false;emit("RUSH_RELEASED",s.intent_cause,"NATIVE_OR_FOREIGN_SPEED_WRITE_OBSERVED_NOT_OVERWRITTEN");}
 return out;
}
// CP19: after posMoveF, give back the PRE speed and the lockout lease of a close-in tick (only values that are still exactly ours).
template<class Emit>inline ClosePost close_after(State&s,const Actor&a,Emit&&emit)noexcept{
 const auto p=close_post(s.close,a.speed,a.lockout,a.pos);
 if(p.preempted)emit("CLOSE_IN_TICK_PREEMPTED_BY_NATIVE",s.intent_cause,"NATIVE_OR_FOREIGN_SPEED_WRITE_NOT_OVERWRITTEN");
 return p;
}
inline bool own_live(const State&s,const Actor&a)noexcept{return s.owned&&sameclip(a,s.clip)&&a.rate==s.own_rate&&a.frame>=s.last_frame&&finite(a.frame)&&a.frame<=a.end&&a.playmode==0;}
struct Rate{bool write=false;float value=1;Cause cause=Cause::Normal;};
template<class Emit>inline Rate post(State&s,const Actor&a,Emit&&emit)noexcept{
 Rate out;out.cause=s.blow_cause;
 // A forced start the native execute did not turn into B/C this tick (water/damage/fall priority) is not an attempt: the episode stays open.
 if(s.forced&&s.forced_tick==s.ticks&&!blow(a)){
  s.forced=false;s.attempted=false;emit("NATIVE_ATTACK_FORCED_PREEMPTED",s.forced_cause,"NATIVE_PRIORITY_KEPT_EPISODE_NOT_CONSUMED");
  if(intercept(s.forced_cause)){s.jump_done=false;s.jump_wait_logged=false;s.jump_block="NATIVE_PRIORITY_PREEMPTED_FORCED_START";}
  else if(s.forced_cause==Cause::ChargeSpin||s.forced_cause==Cause::ChargeJump){s.episode_reacted=false;s.episode_selected=false;}
 }
 if(s.decided&&s.last.seq==s.attack_seq&&!s.last.contact&&s.registered&&a.at_hit){
  s.last.contact=true;s.last.link=a.hit_link;
  emit("NATIVE_CONTACT_TARGET_IDENTIFIED",s.blow_cause,a.hit_link?"TARGET_LINK_TG_CYLINDER_BY_POINTER_IDENTITY":"TARGET_OTHER_NOT_LINK");
  if(a.hit_link&&interruptible(s.blow_cause))s.watch_until=s.ticks+kInterruptWatch;
 }
 if(s.decided&&blow(a)&&a.at_set)s.registered=true;   // after the check: flags read now come from a pass in which this blow was registered
 if(s.decided&&s.last.link&&!s.last.shield&&a.mode==Attack&&a.state==3&&a.rate<0){s.last.shield=true;s.watch_until=0;emit("SHIELD_BLOCK_NATIVE_OBSERVED",s.blow_cause,"NATIVE_RECOIL_STATE3_RATE_MINUS1_AFTER_LINK_CONTACT");}
 if(!blow(a)){
  if(s.pending)emit("RATE_SKIPPED",s.blow_cause,"X2_DECIDED_AT_FRAME1_BUT_NATIVE_CLIP_RATE_OR_FRAME_CHANGED_BEFORE_FRAME2");
  if(s.owned&&sameclip(a,s.clip)&&a.rate==s.own_rate&&finite(a.frame)&&a.frame>=s.last_frame&&!a.stopped){out.write=true;out.value=1;emit("RATE_RESTORED",s.blow_cause,"ONLY_STILL_OWNED_BC_RATE_NATIVE_NEGATIVE_RECOIL_NOT_WRITTEN");}
  if(s.decided)emit("NATIVE_ATTACK_ENDED",s.blow_cause,!s.last.contact?"NO_NATIVE_CONTACT_FLAG":s.last.shield?"LINK_CONTACT_SHIELD_RECOIL":s.last.link?"LINK_CONTACT_DAMAGE_PENDING_CP14_CONFIRMATION":"CONTACT_OTHER_TARGET");
  if(s.decided&&!s.last.contact&&interruptible(s.blow_cause))emit("INTERRUPTION_NOT_OBSERVED",s.blow_cause,"NO_IDENTIFIED_CONTACT_IN_THIS_BLOW");
  s.decided=false;s.owned=false;s.pending=false;s.clip=0;s.pose_valid=s.pose2_valid=s.registered=false;s.sweeps=s.sweep_kept=0;return out;
 }
 if(s.decided&&s.pending){
  s.pending=false;
  if(s.pre_valid&&safe(a)&&sameclip(a,s.clip)&&a.rate==1.0f&&a.playmode==0&&!a.stopped&&absd(double(a.frame)-2.0)<1e-3){s.own_rate=kBurst;s.last_frame=a.frame;s.owned=true;out={true,kBurst,s.blow_cause};}
  else emit("RATE_SKIPPED",s.blow_cause,"X2_DECIDED_AT_FRAME1_BUT_NATIVE_CLIP_RATE_OR_FRAME_CHANGED_BEFORE_FRAME2");
  return out;
 }
 if(s.decided){if(s.owned&&!own_live(s,a)){s.owned=false;emit("RATE_OWNERSHIP_RELEASED",s.blow_cause,"NATIVE_OR_FOREIGN_RATE_FRAME_OR_CLIP_CHANGED_NO_OVERWRITE");}if(s.owned)s.last_frame=a.frame;return out;}
 s.decided=true;++s.attack_seq;s.last=Blow{};s.last.seq=s.attack_seq;s.last.start=s.ticks;s.pose_valid=s.pose2_valid=s.registered=false;s.sweeps=s.sweep_kept=0;
 const bool was_forced=s.forced&&s.forced_tick==s.ticks;s.forced=false;
 if(!s.pre_valid||!safe(a)){emit("RATE_SKIPPED",Cause::Normal,"NATIVE_PRIORITY_UNSAFE");return out;}
 if(a.playmode!=0||!finite(a.end)||a.end<=25){emit("RATE_SKIPPED",Cause::Normal,"NATIVE_CLIP_RANGE_OR_PLAY_MODE_NOT_SUPPORTED");return out;}
 if(s.pre_mode!=Attack||s.pre_state!=0||!(a.frame>=.999f&&a.frame<=1.001f)){emit("RATE_SKIPPED",Cause::Normal,"NOT_A_FRESH_NATIVE_BC_START");return out;}
 if(a.rate!=1.0f){emit("RATE_SKIPPED",Cause::Normal,"FOREIGN_OR_NATIVE_NONDEFAULT_RATE");return out;}
 Cause cause=Cause::Normal;
 if(was_forced)cause=s.forced_cause;
 else if(s.intent&&s.ticks-s.intent_start<kIntent){cause=s.intent_cause;if(cause==Cause::ChargeSpin||cause==Cause::ChargeJump)s.episode_reacted=true;}
 else if(s.combat&&s.random_allowed&&!a.incoming&&random_ready(s)&&chance(s,kRandomPercent)){cause=Cause::Random;s.next_random=s.ticks+between(s,kRandomMin,kRandomMax);}
 end_rush(s,emit,s.intent?s.intent_cause:cause);
 s.intent=false;s.attempted=false;
 if(reactive(cause))s.next_reactive=s.ticks+kReactiveCooldown;
 const float chosen=cause==Cause::Normal?kNormal:kBurst;
 if(cause==Cause::Normal)s.normal_since_random=true;else if(cause==Cause::Random)s.normal_since_random=false;
 s.blow_cause=cause;s.last.cause=cause;s.clip=a.b?5:6;s.last_frame=a.frame;
 if(chosen==kBurst){s.pending=true;s.owned=false;s.own_rate=1;}else{s.own_rate=chosen;s.owned=true;out={true,chosen,cause};}
 emit("NATIVE_ATTACK_STARTED",cause,chosen==kBurst?(was_forced?"FORCED_NATIVE_ATTACK_X2_DECIDED_ONCE_APPLIED_AT_FRAME2_EVEN_PHASE":"NATIVE_BC_CHOICE_X2_DECIDED_ONCE_APPLIED_AT_FRAME2_EVEN_PHASE"):(was_forced?"FORCED_NATIVE_SETACTIONMODE_ATTACK_NATIVE_BC_CHOICE_RATE_DECIDED_ONCE":"NATIVE_BC_CHOICE_PRESERVED_RATE_DECIDED_ONCE"));return out;
}
// Contact coverage at x2. Native x1 registers poses 15..23 (AT bit on pre-play 14..22), tip r45 + hilt r30 each. Even-phase x2 registers poses
// 16/18/20/22/24. For an owned x2 blow registered this tick, the two native spheres are placed for THIS collision pass only (cc_set rewrites centre and
// radius on the next execute) on two of the four native-equivalent spheres of the pose pair {current, skipped}: current tip/hilt (exact) and the
// skipped pose's tip/hilt (hilt midpoint + mean blade vector). Spheres that reach Link's cylinder are chosen first (exact before interpolated), then
// current tip and skipped tip. On the tick past the native window (pose 24) only native pose 23 is registered (or exact pose 22 as a fallback).
// Interpolated radii shrink by an error bound (second difference / 8) so they stay inside the region the native spheres sweep.
inline bool reaches(Point c,float rad,Point link)noexcept{
 const double h=root(xz2(c,link)),dh=h>kLinkR?h-kLinkR:0.0;const double top=double(link.y)+kLinkH;
 const double y=double(c.y)<double(link.y)?double(link.y):double(c.y)>top?top:double(c.y);const double dy=y-c.y;return dh*dh+dy*dy<=double(rad)*rad;
}
struct Sweep{bool write0=false,write1=false;Point c0{},c1{};float r0=0,r1=0;const char* why="NONE";};
inline Sweep sweep(State&s,const Actor&a,const Link&l)noexcept{
 Sweep out;
 if(!s.decided||!s.owned||!blow(a)||a.rate!=kBurst||s.own_rate!=kBurst||!finite(a.tip)||!finite(a.hilt)){s.pose_valid=s.pose2_valid=false;out.why="NOT_AN_OWNED_X2_BLOW";return out;}
 const bool consecutive=s.pose_valid&&absd(double(a.frame)-(s.pose_frame+kBurst))<1e-3;
 const Point tip_p=s.tip_p,hilt_p=s.hilt_p,tip_pp=s.tip_pp,hilt_pp=s.hilt_pp;const bool pp=s.pose2_valid&&consecutive;
 s.tip_pp=s.tip_p;s.hilt_pp=s.hilt_p;s.pose2_valid=consecutive;s.tip_p=a.tip;s.hilt_p=a.hilt;s.pose_frame=a.frame;s.pose_valid=true;
 if(!a.at_set){out.why="NATIVE_AT_BIT_OFF_THIS_TICK";return out;}
 if(!consecutive){out.why="NO_CONSECUTIVE_PREVIOUS_POSE";return out;}
 if(a.tip_r!=kTipR||a.hilt_r!=kHiltR){out.why="NATIVE_RADII_NOT_OBSERVED";return out;}
 if(a.wall){out.why="RED_WALL_CONTACT";return out;}
 const bool last=a.frame>23.0f+1e-3f;
 auto previous=[&](const char*why){out.write0=out.write1=true;out.c0=tip_p;out.r0=kTipR;out.c1=hilt_p;out.r1=kHiltR;out.why=why;++s.sweeps;return out;};
 const Point dp=sub(tip_p,hilt_p),dc=sub(a.tip,a.hilt);const double lp=len(dp),lc=len(dc);
 const double cosang=lp<1||lc<1?-1:(double(dp.x)*dc.x+double(dp.y)*dc.y+double(dp.z)*dc.z)/(lp*lc);
 const double gap=len(sub(a.tip,tip_p));
 Point mid=scale(add(dp,dc),0.5f);const double lm=len(mid);
 const char* bad=lp<1||lc<1||absd(lp-lc)>0.2*(lp>lc?lp:lc)?"BLADE_LENGTH_INCONSISTENT":cosang<-0.17||gap>kMaxGap?"POSE_JUMP_TOO_LARGE":lm<1?"BLADE_DIRECTION_DEGENERATE":nullptr;
 double rt=-1,rh=-1;Point tip_m{},hilt_m{};
 if(!bad){
  mid=scale(mid,float(((lp+lc)*0.5)/lm));hilt_m=scale(add(hilt_p,a.hilt),0.5f);tip_m=add(hilt_m,mid);
  const double err_t=pp?len(add(sub(a.tip,scale(tip_p,2.f)),tip_pp))/8.0:gap/8.0;
  const double err_h=pp?len(add(sub(a.hilt,scale(hilt_p,2.f)),hilt_pp))/8.0:len(sub(a.hilt,hilt_p))/8.0;
  rt=double(kTipR)-err_t;rh=double(kHiltR)-err_h;if(rt<kSweepMinR||rh<kHiltR*0.6)bad="INTERPOLATION_ERROR_BOUND_TOO_LARGE";
 }
 if(last){
  // Never leave native pose 24 registered: pose 23 when interpolable, else the exact (already native-equivalent) pose 22.
  if(bad)return previous("LAST_TICK_EXACT_PREVIOUS_POSE22");
  out.write0=out.write1=true;out.c0=tip_m;out.r0=float(rt);out.c1=hilt_m;out.r1=float(rh);out.why="LAST_TICK_NATIVE_POSE23_TIP_AND_HILT";++s.sweeps;return out;
 }
 if(bad){out.why=bad;return out;}
 struct Cand{Point c;float r;bool exact;};const Cand cand[4]={{a.tip,kTipR,true},{a.hilt,kHiltR,true},{tip_m,float(rt),false},{hilt_m,float(rh),false}};
 int pick[2]={-1,-1},n=0;
 if(l.sampled&&finite(l.pos))for(int i=0;i<4&&n<2;++i)if(reaches(cand[i].c,cand[i].r,l.pos))pick[n++]=i;
 for(int i:{0,2})if(n<2&&pick[0]!=i)pick[n++]=i;
 // Keep each chosen sphere in its native slot when it is that slot's own exact pose; otherwise write.
 int s0=pick[0],s1=pick[1];if(s1==0||(s0==1&&s1!=0)){const int t2=s0;s0=s1;s1=t2;}
 if(s0!=0){out.write0=true;out.c0=cand[s0].c;out.r0=cand[s0].r;}
 if(s1!=1){out.write1=true;out.c1=cand[s1].c;out.r1=cand[s1].r;}
 if(!out.write0&&!out.write1){++s.sweep_kept;out.why="NATIVE_TIP_AND_HILT_ARE_THE_REACHING_POSES";return out;}
 ++s.sweeps;out.why=(cand[s0].exact&&cand[s1].exact)?"NATIVE_POSES":"SKIPPED_NATIVE_POSE_INCLUDED";return out;
}
// CP14's own attribution (Link life really dropped from this Red's sphere) is forwarded here; CP17 never infers damage itself.
template<class Emit>inline void damage_confirmed(State&s,Emit&&emit)noexcept{
 if(s.last.seq&&!s.last.damage&&s.ticks-s.last.start<=60){s.last.damage=true;emit("DAMAGE_CONFIRMED",s.last.cause,"CP14_ATTRIBUTION_LINK_LIFE_DROP_FROM_THIS_RED_SPHERE");}
 else emit("DAMAGE_CONFIRMED",Cause::Normal,"CP14_ATTRIBUTION_BLOW_CAUSE_UNKNOWN_OR_STALE");
}
inline bool restore_on_shutdown(const State&s,const Actor&a)noexcept{return domain(a)&&a.id==s.id&&a.gen==s.gen&&own_live(s,a)&&blow(a)&&!a.stopped;}
// Native checkPass mode0 uses [frame,min(frame+rate,end-.001)); executeAttack B uses else-if 10/12.5/19, C 7/11.5.
inline bool pass(float frame,float rate,float end,float target)noexcept{float next=frame+rate;if(next>=end)next=end-.001f;return frame<=next&&frame<=target&&target<next;}
}
