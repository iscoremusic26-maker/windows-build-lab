#pragma once
// CP19 value policy (Expansion 0.16.0). Shared by the DLL and the laboratory: no game pointers, no native calls. mod.cpp samples, calls, applies.
//  A AUDIO  : F_SP121 field rooms whose scene SE wave 2 is 0x5f/0x60/0x61 (rooms 0,5,7,9..14) do not carry the E_OC samples; rooms with native E_OC use 0x5e
//             (2,3,4) or 0x4b (1,6,15) (Z2SceneMgr::setSceneName + the 2026-10-19 human log). While a classified Bokoblin executes there, the engine's own
//             Z2SceneMgr::loadSeWave(0x5e) is requested once; it is erased (eraseSeWave) only when loaded (status 2), owned, and neither wanted nor requested
//             by the scene. Never during a load, never a wave the scene manager loaded itself.
//  B TARGET : daE_OC_c::damage_check clears attention_info.flags when a hit leaves health <= 1; with health exactly 1 the actor survives the knock-down
//             and regeneration/lifesteal revive it untargetable. Native create() value BATTLE is restored once health >= 2 in a live, visible mode.
//  C ZOMBIE : the native Jump Attack cancel-turn path (setActionMode(DAMAGE) before the BIG_DAMAGE/death decision) leaves a 0-HP Bokoblin alive with
//             attention 0 and its sound object's last-hit flag set (Z2SoundObjMgr::searchEnemy then freezes the field battle mix). Its kill (health 0,
//             kill SE, achievement) already happened; CP19 completes it with the native BIG_DAMAGE 0 death path. A live Bokoblin keeping the last-hit
//             flag for 30 own ticks has it cleared (residual state only).
//  F HELM   : damage_check sets health = 0 for any sword hit while Link's cut type is HEAD_JUMP. During that one Bokoblin execute Link's mCutType is masked
//             to TURN_RIGHT (same daE_OC_c::getCutType 6 reaction, same searchSound result, mHitBit unused by E_OC) and restored in POST; the hit then
//             takes exactly D (Master Sword 100, Ordon 50) from the PRE health: min(native result, H - D). H <= D: the native kill runs untouched.
//  D LEASH  : a receiver that consumed a Blue call gets a permission bound to (receiver id/gen, caller id/gen, stage/room/layer). While the caller lives,
//             and after its death while the receiver keeps fighting (150 calm ticks), native leash returns are replaced by native FIND entries.
#include "black_ai_policy.hpp"
#include <cstdint>
#include <cstring>
namespace devf_cp19 {
namespace ra=devf_red_ai;
using Variant=devf_variant::Variant;
constexpr int Wait=devf_variant::A_WAIT,Walk=devf_variant::A_WALK,Talk=devf_variant::A_TALK,Find=devf_variant::A_FIND,Attack=devf_variant::A_ATTACK,Damage=devf_variant::A_DAMAGE,
 BigDamage=devf_variant::A_BIG_DAMAGE,Watch=devf_variant::A_WATCH,Sound=devf_variant::A_SOUND_WATCH,Death=devf_variant::A_DEATH,WaterDeath=devf_variant::A_WATER_DEATH,
 DemoMaster=devf_variant::A_DEMO_MASTER,DemoChild=devf_variant::A_DEMO_CHILD,FallDead=devf_variant::A_FALL_DEAD,Fall=devf_variant::A_FALL,FindStay=devf_variant::A_FIND_STAY,MoveOut=devf_variant::A_MOVE_OUT;
inline bool live_mode(int m)noexcept{return m==Wait||m==Walk||m==Talk||m==Find||m==Attack||m==Damage||m==Watch||m==Sound||m==FindStay||m==MoveOut;}
inline bool terminal_mode(int m)noexcept{return m==Death||m==WaterDeath||m==FallDead||m==DemoMaster||m==DemoChild||m<0||m>=devf_variant::A_COUNT;}
// ---------------- A. scene SE wave keeper ----------------
constexpr std::uint8_t kFieldSeWave1=0x4a,kOcWave=0x5e;constexpr unsigned kLinger=300,kRetry=300,kRecentPasses=60;
struct Waves{bool exist=false;std::uint8_t req1=0,loaded1=0,req2=0,loaded2=0,req_demo=0,loaded_demo=0;};
inline bool field_without_oc(const Waves&w)noexcept{
 return w.exist&&w.req1==kFieldSeWave1&&w.loaded1==kFieldSeWave1&&w.req2==w.loaded2&&(w.req2==0x5f||w.req2==0x60||w.req2==0x61)&&w.req_demo==0&&w.loaded_demo==0;
}
inline bool scene_requests(const Waves&w,std::uint8_t x)noexcept{return w.req1==x||w.req2==x||w.req_demo==x;}
inline bool scene_loaded(const Waves&w,std::uint8_t x)noexcept{return w.loaded1==x||w.loaded2==x||w.loaded_demo==x;}
enum class WaveOp:std::uint8_t{None,Load,Erase};
struct WaveCmd{WaveOp op=WaveOp::None;const char* why="NONE";};
struct Keeper{bool owned=false,failed_logged=false;unsigned ticks=0,last_need=0,retry_after=0,loads=0,erases=0,failures=0,released_by_engine=0;};
// One management pass. `status` is Z2SceneMgr::getWaveLoadStatus(kOcWave,0) (0 none, 1 loading, 2 loaded); `bokoblin` = a classified Bokoblin executed recently.
inline WaveCmd keeper_step(Keeper&k,bool field_stage,bool transition,const Waves&w,bool bokoblin,int status)noexcept{
 WaveCmd c;++k.ticks;
 const bool need_now=field_stage&&!transition&&field_without_oc(w)&&bokoblin;
 if(need_now)k.last_need=k.ticks;
 const bool want=field_stage&&!transition&&field_without_oc(w)&&k.last_need&&k.ticks-k.last_need<=kLinger;
 if(k.owned){
  if(status==0){k.owned=false;++k.released_by_engine;c.why="OWNED_WAVE_NO_LONGER_LOADED_ENGINE_RELEASED_IT";return c;}
  if(want||scene_requests(w,kOcWave))return c;                 // still needed, or the scene now serves its own room with it
  if(status!=2){c.why="ERASE_DEFERRED_WHILE_LOADING";return c;}   // never free a wave arc whose DVD load is still in flight
  c.op=WaveOp::Erase;c.why=transition?"SCENE_TRANSITION_RELEASE":!field_stage?"STAGE_LEFT_RELEASE":"NO_BOKOBLIN_FOR_300_PASSES_OR_ROOM_WAVE_CHANGED";return c;
 }
 if(!need_now||status!=0||scene_loaded(w,kOcWave)||scene_requests(w,kOcWave)||k.ticks<k.retry_after)return c;
 c.op=WaveOp::Load;c.why="FIELD_ROOM_SE_WAVE_WITHOUT_E_OC_SAMPLES_LOAD_ROOM_2_3_4_WAVE";return c;
}
inline void keeper_result(Keeper&k,WaveOp op,bool ok)noexcept{
 if(op==WaveOp::Load){if(ok){k.owned=true;++k.loads;k.failed_logged=false;}else{++k.failures;k.retry_after=k.ticks+kRetry;}}
 else if(op==WaveOp::Erase){k.owned=false;++k.erases;}
}
// ---------------- B/C. attention, zombie completion, residual last-hit flag ----------------
constexpr unsigned kResidualTicks=30;
inline bool restore_attention(unsigned flags,int health,int mode,bool hidden,bool deleting)noexcept{return flags==0&&health>=2&&live_mode(mode)&&!hidden&&!deleting;}
inline bool complete_zombie(int health,int mode,bool hidden,bool deleting)noexcept{return health<=0&&live_mode(mode)&&!hidden&&!deleting;}
struct Residual{unsigned ticks=0;};
inline bool residual_clear(Residual&r,unsigned a3,int health,int mode)noexcept{
 if(a3==0||health<1||!live_mode(mode)||mode==Damage){r.ticks=0;return false;}
 if(++r.ticks<kResidualTicks)return false;r.ticks=0;return true;
}
// ---------------- F. Helm Splitter ----------------
constexpr std::uint8_t kCutHeadJump=5,kCutTurnRight=8;constexpr int kHelmMaster=100,kHelmOrdon=50;
enum class Sword:std::uint8_t{None,Wood,Ordon,Master};
struct Helm{bool mask=false,episode=false;int pre_health=0,damage=0;std::uint32_t link_id=0;unsigned applied=0,lethal=0,duplicates=0,foreign=0;};
struct HelmView{bool link=false,pending=false,sword_hit=false,cutdown=false;std::uint8_t cut=0;Sword sword=Sword::None;int health=0,mode=-1,invuln=0;std::uint32_t link_id=0;};
enum class HelmOp:std::uint8_t{None,Mask,SkipDuplicate,NativeLethal};
struct HelmCmd{HelmOp op=HelmOp::None;const char* why="NONE";};
inline int helm_damage(Sword s)noexcept{return s==Sword::Master?kHelmMaster:s==Sword::Ordon?kHelmOrdon:0;}
inline HelmCmd helm_pre(Helm&h,const HelmView&v)noexcept{
 HelmCmd c;h.mask=false;
 if(!v.link||v.cut!=kCutHeadJump){h.episode=false;return c;}   // a new Helm Splitter episode starts at the next HEAD_JUMP
 // damage_check processes the pending tg hit in this execute only if field_0x6cc reaches 0 after execute()'s decrement and no cut-down is pending.
 if(!v.pending||!v.sword_hit||v.cutdown||v.invuln>1||v.health<=0||!live_mode(v.mode))return c;
 const int d=helm_damage(v.sword);if(!d){c.why="WOOD_OR_UNKNOWN_SWORD_NATIVE_UNCHANGED";return c;}
 if(h.episode){++h.duplicates;c.op=HelmOp::SkipDuplicate;c.why="SECOND_IMPACT_OF_THE_SAME_HELM_SPLITTER_NATIVE_INVULNERABILITY_TIMER";return c;}
 h.episode=true;
 if(v.health<=d){++h.lethal;c.op=HelmOp::NativeLethal;c.why="HP_AT_OR_BELOW_HELM_DAMAGE_NATIVE_KILL_UNTOUCHED";return c;}
 h.mask=true;h.pre_health=v.health;h.damage=d;h.link_id=v.link_id;c.op=HelmOp::Mask;c.why=v.sword==Sword::Master?"MASTER_SWORD_100":"ORDON_SWORD_50";return c;
}
struct HelmPost{bool unmask=false,write=false;int health=0;const char* why="NONE";};
// `cut_now` is Link's mCutType after the native execute (still the mask unless somebody else wrote it); `health` is the native result.
inline HelmPost helm_post(Helm&h,bool same_link,std::uint8_t cut_now,int health)noexcept{
 HelmPost p;if(!h.mask)return p;h.mask=false;
 p.unmask=same_link&&cut_now==kCutTurnRight;if(!p.unmask){++h.foreign;p.why="LINK_CUT_TYPE_CHANGED_BY_OTHERS_NOT_RESTORED";}
 if(health>=h.pre_health){p.why=p.unmask?"HIT_NOT_PROCESSED_BY_NATIVE_DAMAGE_CHECK":p.why;return p;}
 const int target=h.pre_health-h.damage;
 if(target<health){p.write=true;p.health=target;}else p.health=health;   // never heals: the native result is kept when it is already lower
 ++h.applied;if(p.unmask)p.why="HELM_SPLITTER_EXACT_DAMAGE";return p;
}
// ---------------- D. Blue call leash permission ----------------
constexpr unsigned kPermitCalm=150,kPermitBounceWindow=45,kPermitBounceLimit=3,kPermitBlocked=90;
struct Permit{
 bool active=false,caller_dead=false;std::uint32_t id=0,gen=0,caller=0,caller_gen=0;char stage[9]{};int room=-1,layer=-1;
 unsigned granted=0,last_combat=0,bounce=0,last_redirect=0,blocked_until=0,redirects=0;int prev_mode=-1;
};
struct PermitView{std::uint32_t id=0,gen=0;char stage[9]{};int room=-1,layer=-1,mode=-1,state=-1,health=0,leash=0;bool stopped=false,link_same_room=false,caller_alive=false,hunting_black=false,safe=false;};
inline void permit_grant(Permit&p,const PermitView&v,std::uint32_t caller,std::uint32_t caller_gen,unsigned tick)noexcept{
 p=Permit{};p.active=true;p.id=v.id;p.gen=v.gen;p.caller=caller;p.caller_gen=caller_gen;std::memcpy(p.stage,v.stage,9);p.room=v.room;p.layer=v.layer;p.granted=p.last_combat=tick;
}
inline bool fighting(int m)noexcept{return m==Find||m==Attack||m==FindStay||m==Damage||m==BigDamage||m==Sound;}
enum class PermitOp:std::uint8_t{None,Find0,Find1,Ended,Blocked};
struct PermitCmd{PermitOp op=PermitOp::None;bool search=false;const char* why="NONE";};
// `search` asks the adapter to call the native daE_OC_c::searchPlayer() (WAIT/WALK beyond the leash cannot re-detect natively) before Find1.
inline PermitCmd permit_pre(Permit&p,const PermitView&v,unsigned tick)noexcept{
 PermitCmd c;if(!p.active)return c;const int prev=p.prev_mode;p.prev_mode=v.mode;
 auto end=[&](const char*why){p.active=false;c.op=PermitOp::Ended;c.why=why;return c;};
 if(v.id!=p.id||v.gen!=p.gen)return end("RECEIVER_SLOT_REUSED_OR_NEW_GENERATION");
 if(v.health<=0||terminal_mode(v.mode))return end("RECEIVER_DEAD_OR_TERMINAL");
 if(std::memcmp(v.stage,p.stage,9)!=0||v.room!=p.room||v.layer!=p.layer)return end("RECEIVER_CONTEXT_CHANGED");
 if(!v.caller_alive&&!p.caller_dead){p.caller_dead=true;p.last_combat=tick;}
 if(v.link_same_room&&fighting(v.mode))p.last_combat=tick;
 if(p.caller_dead&&tick-p.last_combat>=kPermitCalm)return end("CALLER_DEAD_AND_RECEIVER_CALM_FOR_150_TICKS");
 if(v.hunting_black||!v.safe||!v.link_same_room||v.leash==0)return c;   // CP18 hunt owns a provoked Black; the native leash is not in the way
 if(v.mode==Attack&&v.state>=1&&v.state<=3&&v.stopped){c.op=PermitOp::Find0;c.why="CALLED_ALLY_ATTACK_END_NATIVE_MOVE_OUT_REPLACED_BY_NATIVE_FIND0";return c;}
 if(v.mode==MoveOut){
  if(tick<p.blocked_until)return c;
  if(prev==Find||prev==FindStay)p.bounce=p.last_redirect&&tick-p.last_redirect<=kPermitBounceWindow?p.bounce+1:1;
  if(p.bounce>=kPermitBounceLimit){p.bounce=0;p.blocked_until=tick+kPermitBlocked;c.op=PermitOp::Blocked;c.why="NATIVE_LEASH_RETURN_3_TIMES_IN_45_TICKS_GEOMETRY_NATIVE_KEPT_90_TICKS";return c;}
  p.last_redirect=tick;++p.redirects;c.op=PermitOp::Find0;c.why="CALLED_ALLY_NATIVE_MOVE_OUT_REPLACED_BY_NATIVE_FIND0";return c;
 }
 if(v.mode==Wait||v.mode==Walk){c.search=true;c.op=PermitOp::Find1;c.why="CALLED_ALLY_BEYOND_LEASH_NATIVE_SEARCHPLAYER_THEN_NATIVE_FIND1";return c;}
 return c;
}
}
