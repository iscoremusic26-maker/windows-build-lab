#pragma once
// CP19 FIX02 value policy (Expansion 0.16.2). Shared by the DLL and the laboratory: no game pointers, no native calls. mod.cpp samples, calls, applies.
//  A AUDIO  : the E_OC family (daE_OC_c: profiles E_OC and E_OC2, one class, mSound.setEnemyName("E_oc"), the Z2SE_EN_OC_* ids) needs the samples of the
//             SE wave group the engine itself loads where native E_OC live (Z2SceneMgr::setSceneName, F_SP121 rooms 2/3/4: se_wave2 0x5e). In 0.16.1
//             Z2SceneMgr::loadSeWave(0x5e) returned false ten times with the group idle (status 0): by the official source the only remaining cause is
//             JASHeap::alloc on the game's fixed 10 MiB audio ARAM heap (JASWaveArc::load, mHeap.alloc(getRootHeap(), mFileLength)) which the scene's own
//             groups fill. FIX02 never uses that heap: one own ARAM block is reserved from the JKRAram graph heap (24 MiB emulated ARAM, audio part fixed at
//             10 MiB, the rest graph) and the engine's loadSeWave loads the group into it (JASWaveArcLoader::getRootHeap() returns sAramHeap when set; the
//             DLL sets it only around that one synchronous call). The group then stays resident for the whole session and is never erased by the mod:
//             JASBasicWaveBank::decWaveTable clears every wave handle pointing at an erased group, including ids that groups loaded later share, so a
//             release would silence room sounds. No room, stage or layer list: any scene where a DEVF Bokoblin can exist gets the same dependency.
//  B CHIP   : a distinct blow of a classified Black that STARTED during CP10.6 fury and that the engine itself confirms as blocked by Link's shield
//             (Link tg cylinder: ChkTgHit + ChkTgShieldHit + GetTgHitObj == this Black's AT sphere; that sphere: ChkAtHit + ChkAtShieldHit + GetAtHitObj ==
//             that cylinder) takes exactly 1 life unit (1/4 heart) through the meter's pending life count, outside setDamagePoint: no damageMagnification
//             (Dusklight damage multiplier), no armour, no Magic Armour rupees, no mDamageTimer, no damage proc. One charge per (Black id, generation, blow
//             sequence); hit flags read before the blow's spheres were registered in a collision pass are stale and ignored.
//  B FLASH  : a render-only red pulse with the native damage colour schedule (daAlink_c::draw: mDamageTimer 30 + setDamageColorTime, HIO R0/R1/R2 15/10/5,
//             |sin(timer*0x800)|) written to Link's tevStr.TevKColor only around daAlink_c::modelDraw of the body/face/hat/hand models and restored after it.
#include "cp19_policy.hpp"
#include <cstdint>
namespace devf_cp19f2 {
using devf_cp19::AudioGate;
// ---------------- A. universal SE wave residency ----------------
constexpr std::uint8_t kOcFamilyWave=0x5e;
constexpr std::uint32_t kReserveBytes=4u<<20;   // own ARAM block for the group
constexpr std::uint32_t kGraphKeep=4u<<20;      // graph ARAM that must still be free in one block after the reservation
constexpr unsigned kRetryPasses=300,kMaxAttempts=3,kSlowLoadPasses=600;
constexpr const char* kDependencyWhy="E_OC_FAMILY_SE_GROUP_0X5E_FROM_NATIVE_SCENE_TABLE_ROOMS_2_3_4_SAMPLE_CONTENT_INFERRED";
enum class Res:std::uint8_t{Idle,Reserved,Loading,Ready,GaveUp};
inline const char* res_text(Res r)noexcept{switch(r){case Res::Idle:return "IDLE";case Res::Reserved:return "RESERVED";case Res::Loading:return "LOADING";case Res::Ready:return "READY";default:return "GAVE_UP";}}
struct Resident{Res st=Res::Idle;bool identified=false,reserved=false,foreign_logged=false,slow_logged=false;unsigned passes=0,attempts=0,retry_at=0,loading_since=0,lost=0,accepted=0,refused=0,foreign=0;std::uint32_t used=0;};
// Io contract (every member below the FIX01 gate): status(wave) = Z2SceneMgr::getWaveLoadStatus(wave,0); own_used() = bytes held in the own reservation
// (0 = none); transition(); reserve(bytes,keep,why) creates the own block once; load_own(wave,why) = loadSeWave with the own block as root heap;
// game_free() = JASKernel::getAramFreeSize() (largest free block of the game's audio ARAM heap).
template<class Io,class Emit>inline void resident_pass(Resident&r,Io&io,bool need,Emit&&emit)noexcept{
 ++r.passes;
 if(!need||r.st==Res::GaveUp)return;
 if(!r.identified){r.identified=true;emit("AUDIO_DEPENDENCY_IDENTIFIED",kDependencyWhy,kOcFamilyWave,0,0,0);}
 const int status=io.status(kOcFamilyWave);const std::uint32_t used=io.own_used();const bool own=used>0;
 if(r.st==Res::Ready&&!own){   // the group left the own block: only the engine can erase it (the mod never does)
  ++r.lost;emit("AUDIO_RESIDENT_LOST_ENGINE_ERASED","OWN_BLOCK_EMPTY_AFTER_READY_NOT_ERASED_BY_THE_MOD",kOcFamilyWave,int(r.lost),status,0);
  r.st=r.reserved?Res::Reserved:Res::Idle;r.retry_at=r.passes+kRetryPasses;
 }
 if(status==2){
  if(own){
   if(r.st!=Res::Ready){
    r.st=Res::Ready;r.used=used;const std::uint32_t gf=io.game_free();
    emit("AUDIO_LOAD_READY","ENGINE_STATUS_2_GROUP_IN_OWN_ARAM_BLOCK_WAVE_TABLE_REGISTERED_BY_ONLOADDONE",kOcFamilyWave,int(used),int(r.passes-r.loading_since),int(r.attempts));
    emit("AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED",gf<used?"GAME_AUDIO_ARAM_LARGEST_FREE_BLOCK_SMALLER_THAN_THE_GROUP":"GAME_AUDIO_ARAM_WOULD_FIT_THE_GROUP_NOW_CAUSE_NOT_PROVEN",kOcFamilyWave,int(gf),int(used),0);
   }
   return;
  }
  if(!r.foreign_logged){r.foreign_logged=true;++r.foreign;emit("AUDIO_RESIDENT_NOT_OWNED","GROUP_LOADED_BY_THE_SCENE_OR_A_PREVIOUS_MOD_INSTANCE_NOT_CLAIMED",kOcFamilyWave,status,0,0);}
  return;
 }
 r.foreign_logged=false;
 if(status==1){
  if(own&&r.st==Res::Loading&&!r.slow_logged&&r.passes-r.loading_since>=kSlowLoadPasses){r.slow_logged=true;emit("AUDIO_LOAD_NOT_READY_YET","ENGINE_STATUS_1_DVD_LOAD_STILL_IN_FLIGHT_AFTER_600_PASSES",kOcFamilyWave,int(used),0,0);}
  return;   // our load in flight, or the scene loading the group itself: never interfere
 }
 if(status!=0)return;
 if(r.st==Res::Loading){r.st=r.reserved?Res::Reserved:Res::Idle;r.retry_at=r.passes+kRetryPasses;return;}   // an accepted load vanished before completion
 if(io.transition()||r.passes<r.retry_at)return;
 if(!r.reserved){
  const char* why="NONE";
  if(!io.reserve(kReserveBytes,kGraphKeep,why)){
   ++r.attempts;++r.refused;r.retry_at=r.passes+kRetryPasses;emit("AUDIO_RESERVATION_REFUSED",why,kOcFamilyWave,int(kReserveBytes),int(r.attempts),0);
   if(r.attempts>=kMaxAttempts){r.st=Res::GaveUp;emit("AUDIO_LOAD_GAVE_UP","THREE_REFUSALS_NO_FURTHER_ATTEMPT_THIS_SESSION",kOcFamilyWave,int(r.attempts),0,0);}
   return;
  }
  r.reserved=true;r.st=Res::Reserved;emit("AUDIO_RESERVATION_ACQUIRED","OWN_ARAM_BLOCK_FROM_JKRARAM_GRAPH_HEAP_OUTSIDE_THE_GAME_AUDIO_BUDGET",kOcFamilyWave,int(kReserveBytes),int(kGraphKeep),0);
 }
 emit("AUDIO_LOAD_REQUESTED","ENGINE_LOADSEWAVE_WITH_OWN_ARAM_BLOCK_AS_ROOT_HEAP",kOcFamilyWave,status,int(r.attempts),0);
 const char* why="NONE";const bool ok=io.load_own(kOcFamilyWave,why);++r.attempts;
 if(ok){r.st=Res::Loading;r.loading_since=r.passes;r.slow_logged=false;++r.accepted;emit("AUDIO_LOAD_ACCEPTED","LOADSEWAVE_TRUE_GROUP_ALLOCATED_IN_OWN_BLOCK_DVD_TO_ARAM_QUEUED",kOcFamilyWave,io.status(kOcFamilyWave),int(io.own_used()),0);return;}
 ++r.refused;r.retry_at=r.passes+kRetryPasses;emit("AUDIO_LOAD_REFUSED",why,kOcFamilyWave,status,int(r.attempts),0);
 if(r.attempts>=kMaxAttempts){r.st=Res::GaveUp;emit("AUDIO_LOAD_GAVE_UP","THREE_REFUSALS_NO_FURTHER_ATTEMPT_THIS_SESSION",kOcFamilyWave,int(r.attempts),0,0);}
}
struct AudioPass{devf_cp19::GateTrack gate;Resident res;devf_cp19::Bgm bgm;bool bgm_known=false;unsigned bgm_logs=0,passes=0;};
// Same gate as FIX01 (unchanged devf_cp19::audio_gate / gate_transition): no Io audio call before it returned Ready in this pass.
template<class Io,class Emit>inline AudioGate audio_pass(AudioPass&st,Io&io,bool need,Emit&&emit)noexcept{
 ++st.passes;
 const AudioGate g=devf_cp19::audio_gate(io.flags());
 if(const char* ev=devf_cp19::gate_transition(st.gate,g))emit(ev,devf_cp19::gate_text(g),int(st.gate.deferred_passes),int(st.gate.ready_transitions),0,0);
 if(g!=AudioGate::Ready)return g;
 resident_pass(st.res,io,need,emit);
 const devf_cp19::Bgm now=io.bgm();
 if((!st.bgm_known||now.dist!=st.bgm.dist||now.seq!=st.bgm.seq||(now.lasthit>0)!=(st.bgm.lasthit>0))&&st.bgm_logs<300){++st.bgm_logs;emit("BGM_BATTLE_STATE","BATTLE_DIST_STATE_SEQ_STATE_ENEMIES_LASTHIT_FLAGS_READ_ONLY",now.dist,now.seq,int(now.enemies),int(now.lasthit));}
 st.bgm=now;st.bgm_known=true;return g;
}
// Shutdown: no audio call at all. A resident group stays in the own block (game heap objects, no pointer into the DLL): erasing it could clear wave
// handles that scene groups loaded after it share (decWaveTable), and the block is outside the game's audio ARAM budget.
template<class Emit>inline void audio_shutdown(AudioPass&st,Emit&&emit)noexcept{
 if(st.res.st==Res::Ready||st.res.st==Res::Loading)emit("AUDIO_RESIDENT_KEPT_NO_RELEASE","RELEASE_UNSAFE_DECWAVETABLE_SHARED_IDS_GROUP_STAYS_IN_OWN_ARAM_BLOCK",kOcFamilyWave,int(st.res.used),0,0);
 st=AudioPass{};
}
// ---------------- B. Black fury shield chip ----------------
constexpr int kChipLifeUnits=1;          // internal life units; 4 per heart
constexpr unsigned kChipCapacity=64;     // = devf_dyn::kCapacity (checked in mod.cpp)
struct ChipBlow{std::uint32_t id=0,gen=0;unsigned seq=0;bool fury=false,registered=false,charged=false;};
// One Black POST, after black_ai post(): seq = the policy's blow sequence (incremented at the POST where a blow starts), fury = CP10.6 fury counter > 0 in
// that POST (the value post() used to decide x2), at_set = both AT spheres set by this execute's cc_set (registered in the coming collision pass),
// shield = the engine's shield confirmation read from the previous collision pass (see header), live = health > 0 and not deleting.
struct ChipView{std::uint32_t id=0,gen=0;unsigned seq=0;bool fury=false,at_set=false,shield=false,live=false;};
enum class ChipOp:std::uint8_t{None,Charge,Duplicate,NotFury,NotRegistered,NoBlow};
inline const char* chip_text(ChipOp o)noexcept{switch(o){case ChipOp::Charge:return "SHIELD_CHIP_APPLIED";case ChipOp::Duplicate:return "SHIELD_CHIP_DUPLICATE_IGNORED";
 case ChipOp::NotFury:return "SHIELD_BLOCK_NOT_FURY_NO_CHIP";case ChipOp::NotRegistered:return "SHIELD_FLAGS_STALE_BEFORE_REGISTRATION_IGNORED";case ChipOp::NoBlow:return "SHIELD_FLAGS_WITHOUT_TRACKED_BLOW_IGNORED";default:return "NONE";}}
// The engine's shield confirmation for one (AT sphere, Link tg cylinder) pair, both directions of the same dCcS::SetAtTgGObjInf call: the sphere's hit and
// shield-hit bits with its hit object = that cylinder, the cylinder's hit and shield-hit bits with its hit object = that sphere. OnTgShieldHit is set only
// when dCcS::ChkShield accepted the hit (no damage added to Link's stts); OnAtShieldHit alone is not enough (also set for a hit-mark-8 target).
inline bool shield_confirmed(bool at_hit,bool at_shield,const void* at_obj,const void* cyl,bool tg_hit,bool tg_shield,const void* tg_obj,const void* sph)noexcept{
 return at_hit&&at_shield&&at_obj!=nullptr&&at_obj==cyl&&tg_hit&&tg_shield&&tg_obj!=nullptr&&tg_obj==sph;
}
// Flags read in this POST come from the collision pass that ran BEFORE it, so they are judged against the blow known before this POST's sequence
// change; only then does a new blow (new seq) take over, and its spheres count as registered only from the next POST on.
inline ChipOp chip_step(ChipBlow&b,const ChipView&v)noexcept{
 if(b.id!=v.id||b.gen!=v.gen){b=ChipBlow{};b.id=v.id;b.gen=v.gen;}
 ChipOp op=ChipOp::None;
 if(v.shield&&v.live){
  if(b.seq==0)op=ChipOp::NoBlow;
  else if(!b.registered)op=ChipOp::NotRegistered;
  else if(!b.fury)op=ChipOp::NotFury;
  else if(b.charged)op=ChipOp::Duplicate;
  else{b.charged=true;op=ChipOp::Charge;}
 }
 if(v.seq!=b.seq){b.seq=v.seq;b.fury=v.fury;b.registered=false;b.charged=false;}
 if(v.at_set&&b.seq)b.registered=true;
 return op;
}
// Link side of a charge, sampled by the adapter: Link present (ALINK), life > 0, no event.
enum class ChipLink:std::uint8_t{Ok,NoLink,LifeZero,Event};
inline const char* chip_link_text(ChipLink l)noexcept{switch(l){case ChipLink::NoLink:return "SHIELD_CHIP_SKIPPED_NO_LINK";case ChipLink::LifeZero:return "SHIELD_CHIP_SKIPPED_LIFE_ALREADY_ZERO";case ChipLink::Event:return "SHIELD_CHIP_SKIPPED_EVENT_RUNNING";default:return "OK";}}
// Exactly one unit through the pending count: new = old - 1, readback exact (small integers are exact in f32).
inline float chip_pending_after(float before)noexcept{return before-float(kChipLifeUnits);}
inline bool chip_readback_ok(float before,float after)noexcept{return after==before-float(kChipLifeUnits);}
// What the meter (dMeter2_c, setItemLifeCount accumulator) does with it: clamp to [0, max].
inline int meter_apply(int life,float pending,int max_life)noexcept{int n=life+int(pending);if(n>max_life)n=max_life;if(n<0)n=0;return n;}
// ---------------- B. render-only red flash (native schedule) ----------------
constexpr int kFlashDamageTimer=30;   // daAlinkHIO_damage_c0::m.mInvincibleTime (only the colour schedule is reused; mDamageTimer is never written)
constexpr std::uint8_t kFlashR0=15,kFlashR1=10,kFlashR2=5;   // HIO mDamageR0/R1/R2 (G/B 0)
// |sin| of the 32-step native angle (color_timer*0x800): period 16.
constexpr float kAbsSin16[16]={0.0f,0.19509032f,0.38268343f,0.55557023f,0.70710678f,0.83146961f,0.92387953f,0.98078528f,1.0f,0.98078528f,0.92387953f,0.83146961f,0.70710678f,0.55557023f,0.38268343f,0.19509032f};
struct Flash{int dmg=0,col=0;unsigned started=0,ticks=0;};
inline void flash_start(Flash&f)noexcept{f.dmg=kFlashDamageTimer;f.col=32-(kFlashDamageTimer%16);++f.started;}   // setDamageColorTime(): 32 - (timer % 16)
inline bool flash_on(const Flash&f)noexcept{return f.dmg>0;}
inline void flash_tick(Flash&f)noexcept{if(f.dmg>0){++f.ticks;if(f.col>0)--f.col;--f.dmg;}}   // damageTimerCount(): colour and timer count down together
struct Rgb{std::uint8_t r=0,g=0,b=0;};
inline Rgb flash_color(const Flash&f)noexcept{
 Rgb c;if(f.dmg<=0)return c;const int t=f.dmg+f.col;
 const std::uint8_t base=t>32?kFlashR0:t>16?kFlashR1:kFlashR2;
 c.r=std::uint8_t(float(base)*kAbsSin16[unsigned(t)&15u]);return c;
}
// Never lowers a native value (a native damage flash may already be in the same register): per-channel max, restored after the model call.
inline Rgb flash_merge(Rgb native,Rgb own)noexcept{Rgb c;c.r=native.r>own.r?native.r:own.r;c.g=native.g>own.g?native.g:own.g;c.b=native.b>own.b?native.b:own.b;return c;}
}
