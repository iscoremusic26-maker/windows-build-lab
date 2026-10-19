#pragma once
// CP18 Black-only value policy (Expansion 0.15.0). The same header is compiled by the DLL and the laboratory: no game pointers, no native writes here.
// The adapter (mod.cpp) samples the executing Black, calls pre()/post()/sweep()/damage()/poll() and applies only the bounded commands they return.
//  RATE : native B/C x1.15 outside fury; a blow STARTED while the CP10.6 fury counter is >0 is x2 (decided once at the fresh frame 1, applied at frame 2 =
//         even phase, exactly the CP17 FIX01 Red timing). A fury start/end/refresh never changes a blow already running.
//  CHAIN: during fury, at the PRE of the execute whose native executeAttack would run its end branch (isStop -> setBck(0x1C)+V_WAIT_ST -> MOVE_OUT /
//         state 0 / FIND 0), the native ATTACK state 0 (in reach) or the native FIND state 3 approach (out of reach) is selected instead, so that end
//         branch, the FIND state 0 idle clip/voice and its 20-tick lockout are never executed. The previous clip is complete (isStop) when this happens.
//  HUNT : armed once by the CP10.6 real non-lethal HP loss forwarded from dynamic_post (no second detector), bound to this Black's native room and to the
//         Link room/stage observation epoch; keeps the native pursuit alive inside the same room and is reset, never resumed, when Link changes room or
//         stage (A->B->A included) or when the actor ends. Fury and hunt are independent: neither ever writes the other's state.
#include "red_ai_policy.hpp"   // CP17 FIX01 helpers and its x2 contact sweep, reused byte-identical (never edited here)
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_black_ai {
namespace ra=devf_red_ai;
using Variant=devf_variant::Variant;using Point=ra::Point;
constexpr unsigned kCapacity=64,kBounceWindow=45,kBounceLimit=3,kBlockedTicks=90,kMeasureTicks=90,kChainDetail=6;
constexpr float kNormal=1.15f,kFury=2.0f;
static_assert(kFury==ra::kBurst&&kNormal==ra::kNormal,"Black rates are the CP17 FIX01 rates; the reused x2 sweep is defined for exactly x2");
// Chain reach: the native FIND state 4 attack switch (dist < 400, |facing| < 0x1000); height bound against attacking from/to a ledge.
constexpr double kChainReach=400.,kChainHeight=200.,kBlockedRatio=0.25;constexpr int kChainFacing=0x1000;
constexpr int Wait=devf_variant::A_WAIT,Walk=devf_variant::A_WALK,Find=devf_variant::A_FIND,Attack=devf_variant::A_ATTACK,Damage=devf_variant::A_DAMAGE,
 BigDamage=devf_variant::A_BIG_DAMAGE,Watch=devf_variant::A_WATCH,Sound=devf_variant::A_SOUND_WATCH,Death=devf_variant::A_DEATH,WaterDeath=devf_variant::A_WATER_DEATH,
 FallDead=devf_variant::A_FALL_DEAD,FindStay=devf_variant::A_FIND_STAY,MoveOut=devf_variant::A_MOVE_OUT;
struct Actor{
 std::uint32_t id=0,gen=0,stage=0;Variant variant=Variant::None;int room=-1,mode=-1,state=-1,health=0,angle=0,timer=0,lockout=0,leash=0,playmode=0;
 Point pos{},home{};float range=0,speed=0,frame=0,rate=1,end=0;
 bool b=false,c=false,wait_bck=false,stopped=false,hit=false,cutdown=false,wall=false,event=false,transition=false,hidden=false,deleting=false,ground=true,sane=true,los=false;
 // POST-only collider sample (cc_set already ran). Pointer identities are resolved by the adapter into these booleans.
 Point tip{},hilt{};float tip_r=0,hilt_r=0;bool at_set=false,at_hit=false,hit_link=false;
};
// Link: presence, the room the engine assigns from Link's own ground polygon (daAlink_c::setRoomInfo), the stage tag, the position. Read only.
struct Link{bool present=false;std::uint32_t id=0,stage=0;int room=-1;Point pos{};};
inline bool link_valid(const Link&l)noexcept{return l.present&&l.room>=0&&l.room<64&&l.stage!=0&&ra::finite(l.pos);}
inline bool domain(const Actor&a)noexcept{return a.id&&a.gen&&a.variant==Variant::Black&&a.stage&&a.room>=0&&a.room<64&&ra::finite(a.pos)&&ra::finite(a.home)&&ra::finite(a.range)&&a.range>=0;}
inline bool terminal_mode(int m)noexcept{return m==Death||m==WaterDeath||m==FallDead;}
// Native priorities this policy never overrides: damage/knock-down, death, water, fall, demo, event, scene transition, a pending tg hit, cut-down, hidden, deleting.
inline bool safe(const Actor&a)noexcept{return domain(a)&&a.sane&&a.health>0&&!devf_variant::blocks_regen(a.mode)&&a.mode!=Damage&&a.mode!=BigDamage&&!a.hit&&!a.cutdown&&!a.event&&!a.transition&&!a.hidden&&!a.deleting&&a.ground;}
inline bool blow(const Actor&a)noexcept{return a.mode==Attack&&((a.state==1&&a.b)||(a.state==2&&a.c));}
inline bool sameclip(const Actor&a,int clip)noexcept{return clip==5?a.b:clip==6?a.c:false;}
// Port of the leash update at the top of daE_OC_c::action() (it runs after this PRE, before the mode switch): the field_0x6e3 the native will use now.
inline int leash_next(const Actor&a)noexcept{
 if(a.range==0)return a.leash;
 const double d=ra::root(ra::xz2(a.home,a.pos));
 if(a.leash==0)return d>double(a.range)?(d>double(a.range)+300.?2:1):0;
 return d<double(a.range)-100.?0:a.leash;
}
// One Link room observer for every Black: the epoch moves on each observed change of (valid room, stage). Room -1 or an absent Link is a transient.
struct Rooms{bool valid=false;int room=-1;std::uint32_t stage=0,epoch=0,changes=0;};
inline bool observe(Rooms&g,const Link&l)noexcept{
 if(!link_valid(l))return false;
 if(!g.valid){g.valid=true;g.room=l.room;g.stage=l.stage;return false;}
 if(l.room==g.room&&l.stage==g.stage)return false;
 g.room=l.room;g.stage=l.stage;++g.epoch;++g.changes;return true;
}
enum class Op:std::uint8_t{None,Chain,Pursue,FuryFind3,KeepFind0,Redirect};
struct State{
 std::uint32_t id=0,gen=0;unsigned ticks=0,logs=0,crit_logs=0;
 // Hunt of this generation: armed by a forwarded CP10.6 real HP loss, bound to the activation room/stage/epoch; no timeout, no cooldown.
 bool hunt=false,hold_logged=false,timer_logged=false,stay_logged=false,lockout_logged=false;int hunt_room=-1,prev_mode=-1;std::uint32_t hunt_stage=0,hunt_epoch=0;
 unsigned hunt_start=0,episodes=0,bounce=0,last_redirect=0,blocked_until=0;
 // Pursuit measurement (native FIND while hunting): measured XZ displacement against the native speed actually used by posMoveF.
 bool measuring=false,step_valid=false;unsigned measure_ticks=0;double moved=0,native_sum=0,dist_start=-1,dist_now=-1;Point step_from{};
 // Blow and rate ownership.
 bool decided=false,owned=false,pending=false,chained=false,chain_blow=false,pre_valid=false,registered=false,contact=false,link_contact=false,shield=false;
 int clip=0,pre_mode=-1,pre_state=-1;float own_rate=1,last_frame=0;unsigned seq=0,blow_start=0,fury_detail=0,prev_fury=0;
 Op op=Op::None;unsigned op_tick=0;
 ra::State sweep;Point link_pos{};bool link_pos_valid=false;   // geometry history of the reused CP17 FIX01 sweep; Link position of this execute's PRE
};
// Side-table slot stamped with id + generation: a new generation (or another actor reusing the slot or the id) never inherits a hunt or a blow.
inline bool stale(const State&s,std::uint32_t id,std::uint32_t gen)noexcept{return s.id!=id||s.gen!=gen;}
inline State& state_for(std::array<State,kCapacity>&t,unsigned slot,std::uint32_t id,std::uint32_t gen)noexcept{auto&s=t[slot<kCapacity?slot:0];if(stale(s,id,gen)){s={};s.id=id;s.gen=gen;}return s;}
// Telemetry: a critical reserve for hunt/chain/contact lines; chained-blow detail is kept for the first kChainDetail blows of each fury episode.
inline bool critical(const char*e)noexcept{
 for(const char* p:{"BLACK_HUNT","BLACK_CHAIN","BLACK_NATIVE_WAIT","BLACK_NATIVE_CONTACT","BLACK_NATIVE_SHIELD","BLACK_DAMAGE","BLACK_RATE_READBACK_FAILED"})if(!std::strncmp(e,p,std::strlen(p)))return true;
 return false;
}
inline bool chain_detail(const char*e)noexcept{for(const char* p:{"BLACK_CHAIN_NEXT_ATTACK_STARTED","BLACK_NATIVE_WAIT_TRANSITION_AVOIDED","BLACK_NATIVE_ATTACK_STARTED","BLACK_RATE_APPLIED_READBACK","BLACK_NATIVE_ATTACK_ENDED"})if(!std::strcmp(e,p))return true;return false;}
struct Budget{
 static constexpr unsigned kGlobal=600,kActor=40,kCritGlobal=800,kCritActor=120;unsigned global=0,crit=0,suppressed=0,crit_suppressed=0,detail_suppressed=0;
 bool take(State&s,const char*e)noexcept{
  if(s.fury_detail>kChainDetail&&chain_detail(e)){++detail_suppressed;return false;}
  if(critical(e)){if(crit<kCritGlobal&&s.crit_logs<kCritActor){++crit;++s.crit_logs;return true;}++crit_suppressed;return false;}
  if(global>=kGlobal||s.logs>=kActor){++suppressed;return false;}++global;++s.logs;return true;
 }
};
template<class Emit>inline void end_pursuit(State&s,Emit&&emit)noexcept{
 if(s.measuring&&s.measure_ticks){
  emit("BLACK_HUNT_PURSUIT_MEASURED","NATIVE_FIND_CHASE_MEASURED_XZ_DISPLACEMENT_VS_NATIVE_SPEED_SUM_NO_MULTIPLIER");
  if(s.native_sum>=30.&&s.moved<kBlockedRatio*s.native_sum&&s.dist_now>kChainReach)emit("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY","WALL_OR_OBSTACLE_NATIVE_COLLISION_ABSORBED_THE_CHASE_TARGET_KEPT_NO_TELEPORT");
 }
 s.measuring=false;s.measure_ticks=0;s.moved=s.native_sum=0;s.dist_start=s.dist_now=-1;
}
template<class Emit>inline void reset(State&s,Emit&&emit,const char*ev,const char*why)noexcept{
 if(!s.hunt)return;
 end_pursuit(s,emit);emit(ev,why);
 s.hunt=false;s.hunt_room=-1;s.hunt_stage=0;s.hold_logged=s.timer_logged=s.stay_logged=false;s.bounce=0;s.blocked_until=0;s.last_redirect=0;
}
template<class Emit>inline void finish_blow(State&s,Emit&&emit)noexcept{
 if(s.decided)emit("BLACK_NATIVE_ATTACK_ENDED",!s.contact?"NO_NATIVE_CONTACT_FLAG":s.shield?"LINK_CONTACT_SHIELD_RECOIL":s.link_contact?"LINK_CONTACT_NATIVE_DAMAGE_PATH":"CONTACT_OTHER_TARGET");
 s.decided=s.owned=s.pending=s.registered=s.contact=s.link_contact=s.shield=s.chain_blow=false;s.clip=0;
}
// Forwarded from dynamic_post for exactly the CP10.6 FuryStart/FuryRefresh events (PRE health captured after the HP writes, POST health before the
// regeneration write, post < pre, not lethal, not terminal). This proves an HP loss, not who or what caused it.
template<class Emit>inline void damage(State&s,const Actor&a,const Link&l,Rooms&g,bool start,Emit&&emit)noexcept{
 emit("BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6",start?"CP10_6_FURY_START_REAL_NON_LETHAL_HP_LOSS_PRE_POST_NOT_A_SECOND_DETECTOR":"CP10_6_FURY_REFRESH_REAL_NON_LETHAL_HP_LOSS_PRE_POST_NOT_A_SECOND_DETECTOR");
 if(s.hunt)return;                                                    // already locked: one activation, no second trigger
 if(!domain(a)||a.health<=0||terminal_mode(a.mode)){emit("BLACK_HUNT_NOT_ARMED","ACTOR_TERMINAL_OR_OUTSIDE_DOMAIN");return;}
 if(!link_valid(l)){emit("BLACK_HUNT_NOT_ARMED","LINK_ROOM_UNKNOWN_OR_TRANSIENT_NO_GUESS");return;}
 observe(g,l);
 if(l.room!=a.room||l.stage!=a.stage){emit("BLACK_HUNT_NOT_ARMED","LINK_ROOM_DIFFERS_FROM_THIS_BLACK_NATIVE_ROOM");return;}
 s.hunt=true;s.hunt_room=a.room;s.hunt_stage=a.stage;s.hunt_epoch=g.epoch;s.hunt_start=s.ticks;++s.episodes;s.bounce=0;s.blocked_until=0;s.last_redirect=0;
 s.hold_logged=s.timer_logged=s.stay_logged=false;
 emit("BLACK_HUNT_LOCKED_FIRST_DAMAGE","FIRST_REAL_HP_LOSS_OF_THIS_GENERATION_OR_SINCE_LAST_RESET_SAME_ROOM_NO_TIMEOUT");
}
// Management pass: a room change seen while this Black does not execute (unloaded, culled, Link away) clears the hunt without waiting for its PRE.
template<class Emit>inline void poll(State&s,const Rooms&g,Emit&&emit)noexcept{if(s.hunt&&g.epoch!=s.hunt_epoch)reset(s,emit,"BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED","OBSERVED_BY_MANAGEMENT_POLL_WITHOUT_OWN_EXECUTE");}
struct Command{bool set_mode=false,zero_speed=false,timer=false,lockout=false;int mode=-1,state=-1;std::int16_t timer_value=0,lockout_value=0;Op op=Op::None;const char* why="NONE";};
// PRE: at most one native mode entry per own execute, plus two bounded native timer values. `fury` is the CP10.6 counter after the previous POST.
template<class Emit>inline Command pre(State&s,const Actor&a,const Link&l,Rooms&g,unsigned fury,Emit&&emit)noexcept{
 Command c;++s.ticks;const int prev=s.prev_mode;s.prev_mode=a.mode;s.pre_mode=a.mode;s.pre_state=a.state;s.op=Op::None;s.pre_valid=false;s.step_valid=false;
 if(!domain(a))return c;
 observe(g,l);s.link_pos_valid=link_valid(l);if(s.link_pos_valid)s.link_pos=l.pos;
 if(s.hunt){
  if(a.health<=0||terminal_mode(a.mode))reset(s,emit,"BLACK_HUNT_RESET_ACTOR_TERMINAL","DEATH_WATER_DEATH_OR_FALL_DEATH");
  else if(g.epoch!=s.hunt_epoch)reset(s,emit,"BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED","LINK_ROOM_OR_STAGE_CHANGED_SINCE_ACTIVATION_EPOCH_A_B_A_INCLUDED");
  else if(link_valid(l)&&(l.room!=s.hunt_room||l.stage!=s.hunt_stage))reset(s,emit,"BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED","LINK_ROOM_DIFFERS_FROM_ACTIVATION_ROOM");
  else if(a.room!=s.hunt_room||a.stage!=s.hunt_stage)reset(s,emit,"BLACK_HUNT_RESET_ACTOR_CONTEXT","BLACK_NATIVE_ROOM_OR_STAGE_CHANGED");
 }
 const bool same=link_valid(l)&&l.room==a.room&&l.stage==a.stage,hunting=s.hunt&&same;
 if(s.hunt&&!same){if(!s.hold_logged){s.hold_logged=true;emit("BLACK_HUNT_HELD_LINK_ROOM_TRANSIENT","LINK_ABSENT_OR_ROOM_MINUS1_KEPT_NO_RESET_NO_WRITE");}}else s.hold_logged=false;
 s.pre_valid=safe(a);
 if(!s.pre_valid||!same)return c;
 const double dist=ra::d3(a.pos,l.pos),dy=ra::absd(double(l.pos.y)-a.pos.y);const int gap=ra::angle_gap(a.angle,ra::yaw(a.pos,l.pos));
 const int leash=leash_next(a);const bool fury_on=fury>0;
 auto set=[&](int mode,int state,Op op,const char*why){c.set_mode=true;c.mode=mode;c.state=state;c.op=op;c.why=why;s.op=op;s.op_tick=s.ticks;s.pre_mode=mode;s.pre_state=state;s.prev_mode=mode;};
 // 1. A complete native clip (or a complete native shield recoil): the native end branch would play setBck(0x1C)+V_WAIT_ST now.
 if(a.mode==Attack&&a.state>=1&&a.state<=3&&a.stopped){
  if(fury_on&&(leash==0||hunting)){
   if(dist<kChainReach&&dy<=kChainHeight&&gap<kChainFacing&&!a.wall&&a.los){
    finish_blow(s,emit);
    set(Attack,0,Op::Chain,a.state==3?"FURY_NATIVE_RECOIL_COMPLETE_NEXT_NATIVE_ATTACK_STATE0":"FURY_NATIVE_CLIP_COMPLETE_NEXT_NATIVE_ATTACK_STATE0");c.zero_speed=a.speed!=0;s.chained=true;
   }else set(Find,3,Op::Pursue,dist>=kChainReach?"FURY_OUT_OF_REACH_NATIVE_FIND3_APPROACH":dy>kChainHeight?"FURY_LINK_HEIGHT_NATIVE_FIND3":gap>=kChainFacing?"FURY_NOT_FACING_NATIVE_FIND3_TURNS":a.wall?"FURY_WALL_CONTACT_NATIVE_FIND3":"FURY_NO_LINE_OF_SIGHT_NATIVE_FIND3");
  }else if(hunting&&leash!=0){set(Find,0,Op::KeepFind0,"HUNT_ATTACK_END_NATIVE_MOVE_OUT_REPLACED_BY_NATIVE_FIND0_PAUSE_KEPT");emit("BLACK_HUNT_NATIVE_ABANDON_PREVENTED",c.why);}
  return c;
 }
 // 2. Fury: native FIND state 0 would play setBck(0x1C)+V_WAIT_ST and arm the 20-tick lockout (after damage, fall or a floor stop).
 if(fury_on&&a.mode==Find&&a.state==0&&(leash==0||hunting)){set(Find,3,Op::FuryFind3,"FURY_NATIVE_FIND0_WAIT_CLIP_AND_LOCKOUT_SKIPPED_NATIVE_FIND3");return c;}
 // 3. Fury: a post-attack lockout already running in FIND state 4 (fury began during it) is cleared for this Black only.
 if(!fury_on)s.lockout_logged=false;
 else if(a.mode==Find&&a.state==4&&a.lockout>0){c.lockout=true;c.lockout_value=0;if(!s.lockout_logged){s.lockout_logged=true;emit("BLACK_FURY_LOCKOUT_CLEARED","FIND4_POST_ATTACK_LOCKOUT_OF_THIS_BLACK_ONLY_WHILE_FURY");}}
 if(!hunting)return c;
 // 4. Hunt: FIND state 4 re-checks native perception (searchPlayer2 -> WAIT) and the leash (-> MOVE_OUT) whenever field_0x6c0 reaches 0.
 if(a.mode==Find&&a.state==4){
  if(a.timer<2){c.timer=true;c.timer_value=2;if(!s.timer_logged){s.timer_logged=true;emit("BLACK_HUNT_NATIVE_ABANDON_PREVENTED","FIND4_TIMER_0X6C0_HELD_AT_2_SEARCHPLAYER2_WAIT_AND_LEASH_MOVE_OUT_CHECK_DEFERRED");}}
  s.step_from=a.pos;s.step_valid=true;return c;
 }
 if(a.mode==FindStay){if(!s.stay_logged){s.stay_logged=true;emit("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY","NATIVE_FIND_STAY_FLOOR_CHECK_200_AHEAD_FAILED_TARGET_KEPT_NO_TELEPORT");}return c;}
 s.stay_logged=false;
 // 5. Hunt: native abandon modes (lost perception, leash return, companion watch, sound watch) re-enter the native pursuit, bounded against loops.
 if(a.mode==Wait||a.mode==Walk||a.mode==MoveOut||a.mode==Watch||a.mode==Sound){
  if(s.ticks<s.blocked_until)return c;
  if(prev==Find||prev==FindStay)s.bounce=s.last_redirect&&s.ticks-s.last_redirect<=kBounceWindow?s.bounce+1:1;   // the native left a pursuit, not a damage or blow end
  if(s.bounce>=kBounceLimit){s.bounce=0;s.blocked_until=s.ticks+kBlockedTicks;emit("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY","NATIVE_PURSUIT_ABANDONED_3_TIMES_IN_45_TICKS_NATIVE_KEPT_90_TICKS_TARGET_KEPT");return c;}
  s.last_redirect=s.ticks;
  set(Find,fury_on?3:a.mode==MoveOut?0:2,Op::Redirect,a.mode==MoveOut?"NATIVE_MOVE_OUT_LEASH_RETURN_REPLACED_BY_NATIVE_FIND":a.mode==Watch?"NATIVE_WATCH_PAUSE_REPLACED_BY_NATIVE_FIND":a.mode==Sound?"NATIVE_SOUND_WATCH_REPLACED_BY_NATIVE_FIND":"NATIVE_WAIT_WALK_LOST_PERCEPTION_REPLACED_BY_NATIVE_FIND");
  emit("BLACK_HUNT_NATIVE_ABANDON_PREVENTED",c.why);
 }
 return c;
}
struct Rate{bool write=false;float value=1;};
inline bool own_live(const State&s,const Actor&a)noexcept{return s.owned&&sameclip(a,s.clip)&&a.rate==s.own_rate&&a.frame>=s.last_frame&&ra::finite(a.frame)&&a.frame<=a.end&&a.playmode==0;}
// POST: readback of the PRE command, pursuit measurement, contact/shield telemetry and the single rate decision of a fresh native B/C.
// `fury` is the CP10.6 counter AFTER this execute's dynamic_post (the value applicable to a blow that starts now).
template<class Emit>inline Rate post(State&s,const Actor&a,unsigned fury,Emit&&emit)noexcept{
 Rate out;
 if(fury&&!s.prev_fury)s.fury_detail=0;s.prev_fury=fury;
 if(s.hunt&&(a.health<=0||terminal_mode(a.mode)))reset(s,emit,"BLACK_HUNT_RESET_ACTOR_TERMINAL","DEATH_WATER_DEATH_OR_FALL_DEATH_IN_THIS_EXECUTE");
 if(s.op!=Op::None&&s.op_tick==s.ticks){
  const Op op=s.op;s.op=Op::None;
  if(op==Op::Chain){
   if(blow(a)&&!a.wait_bck&&a.frame>=.999f&&a.frame<=1.001f){++s.fury_detail;emit("BLACK_CHAIN_NEXT_ATTACK_STARTED",a.b?"NATIVE_ATTACK_STATE0_CHOSE_B_BCK5":"NATIVE_ATTACK_STATE0_CHOSE_C_BCK6");
    emit("BLACK_NATIVE_WAIT_TRANSITION_AVOIDED","END_BRANCH_NOT_EXECUTED_STATE0_RAN_POST_CLIP_NOT_0X1C_V_WAIT_ST_EXISTS_ONLY_ON_THAT_BRANCH");}
   else emit("BLACK_CHAIN_DEFERRED_NATIVE_PRIORITY",a.mode==Attack&&a.state==0?"NATIVE_STATE0_WAITING_SPEED_ZERO":"NATIVE_DAMAGE_WATER_FALL_OR_EVENT_TOOK_THE_EXECUTE");
  }else if(op==Op::Pursue||op==Op::FuryFind3){
   if(a.mode==Find&&!a.wait_bck)emit("BLACK_NATIVE_WAIT_TRANSITION_AVOIDED",op==Op::Pursue?"FURY_OUT_OF_REACH_NATIVE_FIND3_POST_CLIP_NOT_0X1C_NO_LOCKOUT":"FURY_NATIVE_FIND0_SKIPPED_POST_CLIP_NOT_0X1C_NO_LOCKOUT");
   else emit("BLACK_CHAIN_DEFERRED_NATIVE_PRIORITY",a.mode==Find?"NATIVE_FIND_KEPT_WAIT_CLIP":"NATIVE_WATCH_DAMAGE_WATER_FALL_OR_EVENT_TOOK_THE_EXECUTE");
  }else if(op==Op::KeepFind0||op==Op::Redirect){
   if(a.mode==Find)emit("BLACK_HUNT_PURSUIT_STARTED",op==Op::KeepFind0?"NATIVE_FIND0_AFTER_BLOW_INSTEAD_OF_MOVE_OUT":"NATIVE_FIND_ENTRY_FROM_ABANDON_MODE");
   else emit("BLACK_HUNT_NATIVE_PRIORITY_KEPT","NATIVE_WATCH_DAMAGE_WATER_FALL_OR_EVENT_TOOK_THE_EXECUTE");
  }
 }
 if(s.hunt&&s.step_valid&&a.mode==Find&&ra::finite(a.pos)){
  if(!s.measuring){s.measuring=true;s.measure_ticks=0;s.moved=s.native_sum=0;s.dist_start=s.link_pos_valid?ra::root(ra::xz2(s.step_from,s.link_pos)):-1;}
  ++s.measure_ticks;s.moved+=ra::root(ra::xz2(s.step_from,a.pos));s.native_sum+=ra::absd(a.speed);s.dist_now=s.link_pos_valid?ra::root(ra::xz2(a.pos,s.link_pos)):-1;
  if(s.measure_ticks>=kMeasureTicks)end_pursuit(s,emit);
 }else if(s.measuring&&a.mode!=Find)end_pursuit(s,emit);
 s.step_valid=false;
 if(s.decided&&!s.contact&&s.registered&&a.at_hit){s.contact=true;s.link_contact=a.hit_link;emit(a.hit_link?"BLACK_NATIVE_CONTACT_LINK":"BLACK_NATIVE_CONTACT_OTHER",a.hit_link?"AT_HIT_OBJECT_IS_A_LINK_TG_CYLINDER_BY_POINTER_IDENTITY":"AT_HIT_OBJECT_NOT_LINK");}
 if(s.decided&&blow(a)&&a.at_set)s.registered=true;   // after the check: flags read later come from a pass in which this blow was registered
 if(s.decided&&s.link_contact&&!s.shield&&a.mode==Attack&&a.state==3&&a.rate<0){s.shield=true;emit("BLACK_NATIVE_SHIELD_BLOCK","NATIVE_RECOIL_STATE3_RATE_MINUS1_AFTER_LINK_CONTACT");}
 if(!blow(a)){
  if(s.pending)emit("BLACK_RATE_SKIPPED","X2_DECIDED_AT_FRAME1_BUT_NATIVE_CLIP_RATE_OR_FRAME_CHANGED_BEFORE_FRAME2");
  if(s.owned&&sameclip(a,s.clip)&&a.rate==s.own_rate&&ra::finite(a.frame)&&a.frame>=s.last_frame&&!a.stopped){out.write=true;out.value=1;emit("BLACK_RATE_RESTORED","ONLY_STILL_OWNED_BC_RATE_NATIVE_NEGATIVE_RECOIL_NOT_WRITTEN");}
  finish_blow(s,emit);return out;
 }
 if(s.decided&&s.pending){
  s.pending=false;
  if(s.pre_valid&&safe(a)&&sameclip(a,s.clip)&&a.rate==1.0f&&a.playmode==0&&!a.stopped&&ra::absd(double(a.frame)-2.0)<1e-3){s.own_rate=kFury;s.last_frame=a.frame;s.owned=true;out={true,kFury};}
  else emit("BLACK_RATE_SKIPPED","X2_DECIDED_AT_FRAME1_BUT_NATIVE_CLIP_RATE_OR_FRAME_CHANGED_BEFORE_FRAME2");
  return out;
 }
 if(s.decided){
  // The native once-mode end (J3DFrameCtrl::update: frame = end - 0.001, rate = 0, isStop) is the clip's own completion, not a foreign write.
  if(s.owned&&!own_live(s,a)){s.owned=false;if(!(a.stopped&&a.rate==0))emit("BLACK_RATE_OWNERSHIP_RELEASED","NATIVE_OR_FOREIGN_RATE_FRAME_OR_CLIP_CHANGED_NO_OVERWRITE");}
  if(s.owned)s.last_frame=a.frame;return out;
 }
 s.decided=true;++s.seq;s.blow_start=s.ticks;s.contact=s.link_contact=s.shield=s.registered=false;
 const bool chained=s.chained;s.chained=false;
 if(!s.pre_valid||!safe(a)){emit("BLACK_RATE_SKIPPED","NATIVE_PRIORITY_UNSAFE");return out;}
 if(a.playmode!=0||!ra::finite(a.end)||a.end<=25){emit("BLACK_RATE_SKIPPED","NATIVE_CLIP_RANGE_OR_PLAY_MODE_NOT_SUPPORTED");return out;}
 if(s.pre_mode!=Attack||s.pre_state!=0||!(a.frame>=.999f&&a.frame<=1.001f)){emit("BLACK_RATE_SKIPPED","NOT_A_FRESH_NATIVE_BC_START");return out;}
 if(a.rate!=1.0f){emit("BLACK_RATE_SKIPPED","FOREIGN_OR_NATIVE_NONDEFAULT_RATE");return out;}
 s.clip=a.b?5:6;s.last_frame=a.frame;s.chain_blow=chained;
 if(fury){s.pending=true;s.owned=false;s.own_rate=1;}else{s.own_rate=kNormal;s.owned=true;out={true,kNormal};}
 emit("BLACK_NATIVE_ATTACK_STARTED",fury?(chained?"FURY_CHAINED_NATIVE_BC_X2_DECIDED_ONCE_APPLIED_AT_FRAME2_EVEN_PHASE":"FURY_NATIVE_BC_X2_DECIDED_ONCE_APPLIED_AT_FRAME2_EVEN_PHASE"):"NATIVE_BC_X1_15_DECIDED_ONCE_AT_FRAME1_NATIVE_PAUSES_KEPT");
 return out;
}
// x2 contact coverage: the CP17 FIX01 sweep (two native AT spheres, skipped native pose, pose 23 on the last tick, never pose 24) on an owned x2 Black blow.
inline ra::Sweep sweep(State&s,const Actor&a)noexcept{
 ra::Actor r;r.mode=a.mode;r.state=a.state;r.b=a.b;r.c=a.c;r.frame=a.frame;r.rate=a.rate;r.end=a.end;r.wall=a.wall;r.tip=a.tip;r.hilt=a.hilt;r.tip_r=a.tip_r;r.hilt_r=a.hilt_r;r.at_set=a.at_set;
 ra::Link l;l.sampled=s.link_pos_valid;l.pos=s.link_pos;
 s.sweep.decided=s.decided;s.sweep.owned=s.owned;s.sweep.own_rate=s.own_rate;
 return ra::sweep(s.sweep,r,l);
}
inline bool restore_on_shutdown(const State&s,const Actor&a)noexcept{return domain(a)&&a.id==s.id&&a.gen==s.gen&&own_live(s,a)&&blow(a)&&!a.stopped;}
}
