// DEVF CP19 FIX02 — Bokoblin Expansion 0.16.2 (on FIX01 0.16.1; fixes A-G and the FIX01 audio readiness gate unchanged):
//  A every Bokoblin of the E_OC family keeps its native SE in any stage/room/layer: the family's SE wave group (0x5e) is loaded once, by the engine's own
//    Z2SceneMgr::loadSeWave, into an own ARAM block taken from the JKRAram graph heap (the game's fixed audio ARAM heap refused it in 0.16.1) and is kept
//    resident (never erased by the mod: decWaveTable would clear wave ids shared with scene groups loaded later).
//  B a blow of a classified Black started during fury and confirmed by the engine as blocked by Link's shield takes exactly 1 life unit (1/4 heart), once
//    per blow, through the meter's pending life count (no damage multiplier, no armour, no mDamageTimer, no damage proc); a render-only red pulse with
//    the native damage colour schedule is written to Link's tevStr.TevKColor around daAlink_c::modelDraw of the body/face/hat/hand models only.
// (CP19 FIX01 header follows.)
// DEVF CP19 FIX01 — Bokoblin Expansion 0.16.1: 0.16.0 crashed at cold start (EXCEPTION_ACCESS_VIOLATION, fault 0x1a0) in Z2SceneMgr::getWaveLoadStatus called
//  from cp19_poll() before the audio section heap existed. Every audio access of cp19_poll()/cp19_shutdown() now runs behind devf_cp19::audio_gate()
//  (mDoAud_zelAudio_c init flag + audio heap + no reset/shutdown/return-to-menu); fixes A-G are otherwise unchanged.
// (CP19 header follows.)
// DEVF CP19 — Bokoblin Expansion 0.16.0: bug fixes on top of 0.15.0 (2026-10-19 human report and log):
//  A field rooms whose SE wave lacks the E_OC samples get the room 2/3/4 wave (0x5e) through Z2SceneMgr::loadSeWave while Bokoblins execute there;
//  B attention BATTLE restored on survivors cleared by the native health<=1 rule; C a native 0-HP live mode completes its death, residual last-hit flag cleared;
//  D a Blue call grants its receiver a leash permission (caller alive, or fight going on); E/G x2 blows (Red reactions, Black fury) start from <= 240
//  after a native final approach; F Helm Splitter takes exactly 100 (Master Sword) / 50 (Ordon) HP instead of the native instant kill.
// (CP18 header follows.)
// DEVF CP18 — Bokoblin Expansion 0.15.0: Black-only combat on top of 0.14.1 (CP17 FIX01 Red, Blue FIX01, CP14 and classification unchanged):
//  * native B/C x1.15 outside fury; a blow started while the CP10.6 fury counter (300 ticks, renewed by real HP loss, unchanged) is >0 plays x2 with the
//    CP17 FIX01 even phase and native AT sphere coverage; ATP 3/6, HP, 4 HP/s, aura and eyes stay CP10.6/CP12.
//  * fury chain: at the PRE whose native executeAttack would run its end branch (setBck 0x1C + V_WAIT_ST), the next native ATTACK state 0 (in reach) or
//    native FIND state 3 (out of reach) is selected instead; the FIND state 0 idle clip/voice/20-tick lockout is skipped while fury>0.
//  * same-room hunt: armed by the CP10.6 HP-loss event (forwarded, not re-detected), keeps the native pursuit alive in the Black's room, reset for good on a
//    Link room/stage change (epoch observed every management pass, A->B->A included) or the actor's end. No teleport, no home/range/collision write.
// (CP17 FIX01 header follows.)
// DEVF CP17 FIX01 — Bokoblin Expansion 0.14.1: Red fast native B/C is exactly x2 (decided at the fresh frame 1, applied at frame 2 = even phase,
//  five native AT registrations); normal Red x1.15. Per tick of an owned x2 blow the two native AT spheres cover the skipped native pose (one pass only).
//  Interception of exactly Jump Attack (PROC26) and Jump Strike (PROC30) by a bounded native ATTACK start before landing; every eligible charge
//  episode reacts; reactive and random cooldowns are separate; FIND state-4 rush is an exact x1.5 of the native chase, restored after posMoveF.
//  0.14.0's x3 (historical) and its B SFX compensation are removed. Blue FIX01, Black, CP14 Red and classification are unchanged.
// (CP16 FIX01 header follows.)
// DEVF CP16 FIX01 — Bokoblin Expansion 0.13.1: CP15 speed/groups retained; solo evasion replaced by native recruitment:
//  * GROUP LEVEL 1/2/3 = 1 + min(eligible companions, 2) (any live classified Blue/Red/Black within the unchanged CP10.6 filter); the Blue damage bonus,
//    BLUE_GROUP_ON/OFF, aura and cyan eyes keep their CP10.6/CP12 semantics.
//  * SOLO Blue detects/calls native allies, waits only for genuinely callable bounded orders, then fights natively. No evasion or POST attack veto.
//  * Blue in a pair / group (level 2 / 3): the attack animations B and C play at 1.15x / 1.30x and the native post-attack lockout shrinks 20 -> 17 / 15 ticks.
//  Red and Black retain their abilities; eligible idle recipients consume native movement orders in their own execute. Provider 0.13.0, editor 0.13.0, encounters and every CP14 Red ability are unchanged.
// (CP14 header follows.)
// DEVF CP14 — Bokoblin Expansion 0.11.0 = CP13 0.10.0 + the first RED abilities (red_policy.hpp), nothing else:
//  * LIFESTEAL: an effective hit of a Red on Link (Link's life really dropped; attribution by the engine's own collision record, claimed at
//    mod_update after the collision pass and confirmed one pass later) heals that Red 25 % of 330 (milli-HP accumulator: 82, 83, 82 ...).
//  * GROUP REGENERATION WINDOW: the same hit opens/renews 4 s (120 of the Red's ticks): Red x2, every live classified Bokoblin in the CP10.6
//    companion range of that Red x2 of its own rate while in range; never more than x2.
//  * GOLD: the Red's aura (same shared strokes route, gold colour) and gold eyes (eye packet only) while its window is open.
//  Blue / Black logic, HP, damage, colours, routes, the 8-aura budget and the CP13 encounter/placement path are unchanged.
// (CP10.4 header follows.)
// DEVF CP10.4 — Bokoblin Expansion 0.6.0: variant IDENTITY (Blue/Red/Black) + per-variant HP and regeneration, on top of CP10.3.
// Identity precedence per actor/generation: exact DEVF Black registration > exact DEVF Normal registration (BLUE) > native E_OC (BLUE) > native E_OC2 (RED).
// HP is initialised ONCE at the actor's first execute (inside daE_OC_c::create) and never again; afterwards only bounded regeneration.
// Unchanged: placement, schema, HP_APPLIED, private Black texture (FIX01). Not implemented: damage, AI, elements, new textures.
// (Previous header, CP10.3:) universal passive regeneration + CP10.2 FIX01 Black.
// Added on top of 0.4.1 (unchanged placement, schema, HP_APPLIED, private-texture Black): a passive HP regeneration for EVERY
// daE_OC_c (native E_OC 40 HP, native/DEVF E_OC2 220 HP, DEVF Black = its configured HP): 2 HP/s, Black 4 HP/s, counted in
// 1/30 s simulation ticks via a POST hook of the same daE_OC_c::execute target (see expansion/regen_policy.hpp).
// Placement, schema and HP are exactly CP10.2 (reused headers). The visual route changes:
//  CP10.2 multiplied MATCOLOR0 by 35% in a drawFast pre/post pair (a uniform "paint" over the whole body).
//  FIX01 gives the Black actor its OWN albedo: a private, regionally recolored copy of the E_OC2 CMPR atlas
//  (skin -> charcoal slate, cloth/metal/leather keep their hue), bound only to that actor's two body material
//  packets (J3DMatPacket::mpTexture). Material colors, TEV, lighting, eyes and every other model stay native.
//  No drawFast hook and no XF/MATCOLOR0 command any more.
#include <mods/service.hpp>
#include <mods/svc/log.h>
#include <mods/svc/hook.hpp>
#include "../../CP10_2_BLACK_BOKOBLIN/expansion/client.hpp"
#include "../../CP10_2_BLACK_BOKOBLIN/expansion/black_policy.hpp"
#include "../../CP10_2_FIX01_BLACK_TEXTURE/expansion/cmpr_recolor.hpp"
#include "../../CP10_2_FIX01_BLACK_TEXTURE/expansion/texture_binding.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include "../../CP10_5_DAMAGE_BLUE_RED_BLACK/expansion/damage_policy.hpp"
#include "../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/eye_recolor.hpp"
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include "../../CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/strokes_policy.hpp"
#include "../../CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/perf_policy.hpp"
#include "../../CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/variant_eyes.hpp"
#include "../../CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/perf_window.hpp"
#include "../../CP14_RED_BOKOBLIN_LIFESTEAL_GROUP_REGEN_GOLD/expansion/red_policy.hpp"
#include "blue_ai_policy.hpp"
#include "blue_recruit_policy.hpp"
#include "red_ai_policy.hpp"
#include "black_ai_policy.hpp"
#include "cp19_policy.hpp"
#include "cp19f2_policy.hpp"
// Game headers after the DEVF config headers (same ordering rule as CP10.2).
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/actor/d_a_alink.h>   // CP14: read-only view of Link's damage state (tg cylinders, damage status, invulnerability timer)
#include <d/d_com_inf_game.h>
#include <Z2AudioLib/Z2AudioMgr.h>   // CP19: the engine's own scene/seq/sound-object managers (exported mAudioMgrPtr)
#include <m_Do/m_Do_audio.h>   // CP19 FIX01: mDoAud_zelAudio_c init/reset flags and g_mDoAud_audioHeap (audio readiness)
#include <m_Do/m_Do_Reset.h>   // CP19 FIX01: mDoRst shutdown / return-to-menu
#include <JSystem/JAudio2/JASHeapCtrl.h>      // CP19 FIX02: JASHeap (own root heap object), JASKernel::getAramFreeSize
#include <JSystem/JAudio2/JASWaveArcLoader.h> // CP19 FIX02: JASWaveArcLoader::sAramHeap (root heap of JASWaveArc::load(NULL))
#include <JSystem/JKernel/JKRAram.h>          // CP19 FIX02: graph ARAM heap
#include <JSystem/JKernel/JKRAramHeap.h>
#include <JSystem/JKernel/JKRAramBlock.h>
#include <JSystem/JKernel/JKRHeap.h>
#include <f_pc/f_pc_manager.h>
#include <cstdio>
#include <cstring>
#include <new>
#include <type_traits>
#include <JSystem/J3DGraphAnimator/J3DModel.h>
#include <JSystem/J3DGraphAnimator/J3DModelData.h>
#include <JSystem/J3DGraphBase/J3DPacket.h>
#include <JSystem/J3DGraphBase/J3DMaterial.h>
#include <JSystem/J3DGraphBase/J3DTexture.h>
#include <JSystem/JParticle/JPAEmitterManager.h>
#include <JSystem/JParticle/JPAResourceManager.h>
#include <JSystem/JParticle/JPAResource.h>
#include <JSystem/JParticle/JPADynamicsBlock.h>
#include <JSystem/JParticle/JPABaseShape.h>
#include <JSystem/JParticle/JPAExtraShape.h>
#include <JSystem/JParticle/JPAKeyBlock.h>
#include <JSystem/JParticle/JPAChildShape.h>
#include <JSystem/JParticle/JPAExTexShape.h>
// CP12: performance-counter access lives in perf_clock.cpp (the only TU with <windows.h>); both functions were already imported by the CRT.
extern "C" unsigned long long devf_perf_qpc() noexcept;
extern "C" unsigned long long devf_perf_filetime() noexcept;
#ifndef DEVF_SOURCE_SHA
#error Exact source required
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(FileService,svc_file);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(DevfEncounterPlacementService,svc_devf_encounter);
IMPORT_SERVICE(HookService,svc_hook);
DEFINE_HOOK(&daE_OC_c::execute,NativeEocExecute);
static_assert(std::is_same_v<decltype(&daE_OC_c::execute),int(daE_OC_c::*)()>);
DEFINE_HOOK(&daAlink_c::modelDraw,LinkModelDraw);   // CP19 FIX02: render-only flash (Link's own model draw call; daAlink_c::draw calls it through the import thunk)
static_assert(std::is_same_v<decltype(&daAlink_c::modelDraw),void(daAlink_c::*)(J3DModel*,int)>);
static_assert(sizeof(decltype(&daAlink_c::modelDraw))==8,"Reviewed Windows x64 compact PMF required");
static_assert(std::is_same_v<decltype(((dKy_tevstr_c*)nullptr)->TevKColor),GXColor>);
static_assert(sizeof(decltype(&daE_OC_c::execute))==8,"Reviewed Windows x64 compact PMF required");
static_assert(std::is_same_v<decltype(daE_OC_c::health),s16>);
static_assert(sizeof(ResTIMG)==0x20,"ResTIMG layout");
static_assert(std::is_same_v<decltype(J3DMatPacket::mpTexture),J3DTexture*>,"Per-instance material packet texture field");
namespace {
constexpr const char* kVersion="0.16.2";
expansion::Client client;multi_config::Config config;const char* mode_reason="NOT_READ";
devf_black2::Table table,normal_table;   // exact-match registrations: Black (HP from JSON) and Normal/legacy (identity BLUE)
devf_variant::Registry registry;bool regen_enabled=false;unsigned regen_tracked_logs=0,variant_logs=0;
devf_dyn::Ledger ledger;unsigned damage_logs=0,dynamic_logs=0;std::uint32_t sim_seq=0;   // CP10.6: one record per CP10.4-classified generation (base profile, dynamic state, companion snapshot)
struct SphereIo {daE_OC_c* a;std::uint8_t get(unsigned i)const{return a->mSphs_at[i].GetAtAtp();}void set(unsigned i,std::uint8_t v){a->mSphs_at[i].SetAtAtp(v);}};
std::uint32_t stage_tag()noexcept{const char* n=dComIfGp_getStartStageName();std::uint32_t h=2166136261u;if(!n)return h;for(;*n;++n){h^=static_cast<unsigned char>(*n);h*=16777619u;}return h;}
// ---- private textures: one per distinct original atlas (normally exactly one), kept for the whole session ----
struct PrivateTexture {const std::uint8_t* source=nullptr;std::uint64_t source_hash=0;std::uint8_t* memory=nullptr;J3DTexture* texture=nullptr;devf_tex::Stats stats;};
std::array<PrivateTexture,4> textures{};unsigned texture_count=0;
// CP10.6D: Blue and Black share ONE corrected strokes route: the same native common-bank emitter ID_ZI_J_LK_SWL_C (0x0B9D, bank 0, no 0x8000 bit), the same
// canonical emitter frame and the same geometry; only the colour (and the CP10.6 logic condition) differs. SWL_A (0x0A58, Blue's old route), SWL_B (0x0A59),
// the missing WindLine 0x0046, 0x8046 and the CP10.6A glow 0x072F are never passed.
static_assert(ID_ZI_J_LK_SWL_C==0x0B9D&&ID_ZI_J_LK_SWL_C==devf_stk::kResource&&(ID_ZI_J_LK_SWL_C&0x8000)==0,"common bank particle id");
devf_vis::Ledger vledger;std::array<devf_stk::Retry,devf_vis::kCapacity> stk_retries{};unsigned visual_logs=0;
bool probe_ok[1]{};unsigned probe_logs[1]{};s32 probe_max_frame[1]{};   // one shared resource
// Scalars of the loaded resource as AUTHORED (read once at probe, inline getters): logged as the "before" of the canonical frame, never written back.
struct ResFacts {float tx=0,ty=0,tz=0,dx=0,dy=1,dz=0,sx=1,sy=1,sz=1,sweep=1,min_rad=0;int rx=0,ry=0,rz=0;unsigned volume_size=0,rate_step=0,prm_anim=0;GXColor prm{};};
ResFacts stk_facts[1]{};bool curve_live[1]{};
devf_perf::LogBudget log_budget;   // CP11: full-detail acquisition lines for the first 2 emitters of each variant per session; anomalies always in full
constexpr unsigned kCurveSampleTick=90;                // emitter tick (1/30 s) at which mRate / mLifeTime are read back once per session
struct EyeSet {bool built=false,failed=false;std::uint8_t* memory[devf_eye::kVariants]{};J3DTexture* texture[devf_eye::kVariants]{};devf_eye::Stats stats;};
std::array<EyeSet,4> eye_sets{};   // parallel to `textures`: the private purple variants of each private atlas
struct Binding {bool bound=false;const void* model=nullptr;int texture=-1;bool reject_logged=false;std::uint64_t generation=0;};
std::array<Binding,devf_black2::capacity> bindings{};

void event(const char* e,unsigned i,uint64_t token,ModResult r)noexcept{
 if(!svc_log||!svc_log->write)return;char text[640]{};const auto* s=i<config.count?&config.requests[i]:nullptr;
 std::snprintf(text,sizeof(text),"BOKOBLIN_EXPANSION event=%s owner=dev.bokoblin.expansion encounter_id=%s stage=%s room=%d layer=%d xyz=(%.9g,%.9g,%.9g) yaw=%d ticket=%llu result=%d actor_id=NOT_OBSERVED evidence=STAGE_EDIT_ONLY terrain=NOT_VERIFIED source=%s",e,s?s->placement_id:"NONE",s?s->stage:"NONE",s?int(s->room):-1,s?int(s->layer):-1,s?double(s->x):0.,s?double(s->y):0.,s?double(s->z):0.,s?s->yaw_s16:0,static_cast<unsigned long long>(token),int(r),DEVF_SOURCE_SHA);
 try{svc_log->write(mod_ctx,r==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
void blog(const char* ev,const devf_black2::Reg* r,std::uint32_t actor,const char* reason,int hp_before=-1,int hp_after=-1)noexcept{
 if(!svc_log||!svc_log->write)return;char text[640]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_2_BLACK event=%s owner=dev.bokoblin.expansion version=%s encounter_id=%s actor_id=%u generation=%llu stage=F_SP121 room=%d layer=%d hp_before=%d hp_after=%d reason=%s visual=HUMAN_RUNTIME_PENDING",
  ev,kVersion,r?r->id:"NONE",actor,static_cast<unsigned long long>(r?r->generation:0),r?r->room:-1,r?r->layer:-1,hp_before,hp_after,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void tlog(const char* ev,const devf_black2::Reg* r,std::uint32_t actor,const char* reason,int texture=-1)noexcept{
 if(!svc_log||!svc_log->write)return;char text[640]{};
 const auto* t=texture>=0&&unsigned(texture)<texture_count?&textures[texture]:nullptr;
 std::snprintf(text,sizeof(text),"DEVF_CP10_2_BLACK_TEXTURE event=%s owner=dev.bokoblin.expansion version=%s encounter_id=%s actor_id=%u generation=%llu mechanism=PRIVATE_RECOLORED_CMPR_ALBEDO_PER_ACTOR_MATPACKET blocks=%llu endpoints_changed=%llu skin_endpoints=%llu reason=%s visual=HUMAN_RUNTIME_PENDING",
  ev,kVersion,r?r->id:"NONE",actor,static_cast<unsigned long long>(r?r->generation:0),
  static_cast<unsigned long long>(t?t->stats.blocks:0),static_cast<unsigned long long>(t?t->stats.endpoints_changed:0),static_cast<unsigned long long>(t?t->stats.skin_endpoints:0),reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void rlog(const char* ev,const devf_variant::Entry* e,const char* reason,const char* source,int hp_before=-1,int hp_after=-1)noexcept{
 if(!svc_log||!svc_log->write)return;char text[736]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_3_REGEN event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u kind=%s source=%s max_hp=%d rate_hp_s=%u hp_before=%d hp_after=%d sim_ticks=%u sim_seconds=%.3f healed_total=%u time_base=SIM_TICK_1_30S reason=%s visual_or_gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,e?e->id:0u,e?e->generation:0u,e?devf_variant::variant_text(e->variant):"NONE",source?source:"NONE",
  e?int(e->max_hp):-1,e?unsigned(e->rate):0u,hp_before,hp_after,e?e->ticks:0u,e?double(e->ticks)/double(devf_variant::kTicksPerSecond):0.0,e?e->healed:0u,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// CP10.4 identity telemetry: once per actor generation (bounded), never per frame.
void vlog(const char* ev,const devf_variant::Entry* e,const devf_variant::Plan* p,int native_hp,int hp_before,int hp_after,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||variant_logs>=400)return;++variant_logs;char text[736]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_4_VARIANT event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u source=%s encounter_id=%s classified_variant=%s native_base_hp=%d observed_native_hp=%d hp_before=%d hp_after=%d effective_max_hp=%d regen_rate_hp_s=%u reason=%s visual_or_gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,e?e->id:0u,e?e->generation:0u,e?devf_variant::source_text(e->source):"NONE",e&&e->encounter?e->encounter:"NONE",e?devf_variant::variant_text(e->variant):"UNCLASSIFIED",
  p?int(p->native_base):-1,native_hp,hp_before,hp_after,e?int(e->max_hp):-1,e?unsigned(e->rate):0u,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// CP10.5 attack-power telemetry: once per actor generation (bounded), never per frame or per hit. It reports what was WRITTEN to the two
// colliders; it does not measure any damage taken by Link (link_damage=NOT_MEASURED).
void dlog(const char* ev,std::uint32_t id,std::uint32_t generation,const devf_variant::Entry* e,const devf_damage::Plan* p,int b0,int b1,int a0,int a1,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||damage_logs>=400)return;++damage_logs;char text[800]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_5_DAMAGE event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u source=%s classified_variant=%s encounter_id=%s native_atp=%d collider0_before=%d collider0_after=%d collider1_before=%d collider1_after=%d target_base=%d written=%s reason=%s link_damage=NOT_MEASURED visual_or_gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,generation,e?devf_variant::source_text(e->source):"NONE",e?devf_variant::variant_text(e->variant):"UNCLASSIFIED",e&&e->encounter?e->encounter:"NONE",
  p?int(p->native):-1,b0,a0,b1,a1,p?int(p->target):-1,p&&p->write?"YES":"NO",reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// CP10.6 dynamic telemetry: only on a change (group on/off, fury start/refresh/end, rejection, release), bounded per actor and per session.
void ylog(const char* ev,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry* d,const devf_dyn::Event* x,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||dynamic_logs>=400)return;if(d){if(d->events>=devf_dyn::kEventsPerActor)return;++d->events;}++dynamic_logs;char text[960]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_6_DYNAMIC event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u source=%s classified_variant=%s encounter_id=%s atp_from=%d atp_to=%d collider0_now=%d collider1_now=%d companion_id=%u companion_variant=%s companion_dxz=%.3f companion_dy=%.3f room=%d hp_pre=%d hp_post_native=%d fury_ticks_remaining=%u duration_ticks=%u reason=%s link_damage=NOT_MEASURED visual_or_gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,d?d->generation:0u,e?devf_variant::source_text(e->source):"NONE",d?devf_variant::variant_text(d->variant):"UNCLASSIFIED",e&&e->encounter?e->encounter:"NONE",
  x?int(x->from):-1,x?int(x->to):-1,x?int(x->c0):-1,x?int(x->c1):-1,x?x->companion:0u,x?devf_variant::variant_text(x->companion_variant):"NONE",x?x->dxz:0.0,x?x->dy:0.0,d?d->room:-1,
  x?x->pre_hp:-1,x?x->post_hp:-1,x?x->fury:0u,devf_dyn::kFuryTicks,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
devf_black2::Snapshot record_snapshot(daE_OC_c* a)noexcept{
 devf_black2::Snapshot s;s.profile=a->profname;s.room=fopAcM_GetRoomNo(a);s.parameters=fopAcM_GetParam(a);
 s.set_id=a->setID;s.x=a->home.pos.x;s.y=a->home.pos.y;s.z=a->home.pos.z;
 s.home_x=a->home.angle.x;s.home_y=a->home.angle.y;s.home_z=a->home.angle.z;
 // CP11: the context (stage, layer) is read only after a registration's record fields match (perf_policy.hpp); getLayerNo recomputes the layer
 // through ~40 stage-name strcmp on every call, so it is now evaluated only for actors that can match (in practice the registered Black itself).
 devf_perf::complete_snapshot(table,normal_table,s,[](devf_black2::Snapshot& c){
  const char* stage=dComIfGp_getStartStageName();c.stage=stage&&std::strncmp(stage,"F_SP121",8)==0;c.layer=dComIfG_play_c::getLayerNo(0);});
 return s;
}
// Same read-only effective-resource contract as the human-observed CP04.4 / CP10.2 build (materials, TEV, CMPR atlas).
bool black_material_contract(J3DModelData* data)noexcept{
 static const char* names[]{"oc_body_mat01","oc_body_mat02","oc_eye_mat"};
 auto* textures_=data->getTexture();
 if(!textures_||textures_->getNum()!=3)return false;
 for(unsigned i=0;i<3;i++){
  auto* image=textures_->getResTIMG(static_cast<u16>(i));
  if(!image||image->format!=GX_TF_CMPR||image->width!=128||image->height!=128||image->mipmapCount!=1)return false;
  auto* material=data->getMaterialNodePointer(static_cast<u16>(i));
  if(!material||!material->mMaterialName||std::strcmp(material->mMaterialName,names[i]))return false;
  auto* color=material->getColorBlock();auto* tev=material->getTevBlock();
  if(!color||!tev||color->getType()!='CLON'||tev->getTevStageNum()!=1)return false;
  auto* channel=color->getColorChan(0);auto* alpha=color->getColorChan(1);
  if(!channel||!alpha||channel->getMatSrc()!=GX_SRC_REG||channel->getAmbSrc()!=GX_SRC_REG||
   channel->getEnable()!=(i==2?0:1)||channel->getDiffuseFn()!=GX_DF_CLAMP||channel->getAttnFn()!=GX_AF_SPOT||
   alpha->getEnable()!=0||alpha->getMatSrc()!=GX_SRC_REG)return false;
  auto* stage=tev->getTevStage(0);auto* order=tev->getTevOrder(0);auto* konst=tev->getTevKColor(0);
  if(!stage||!order||!konst||stage->mTevColorOp!=0x28||stage->mTevColorAB!=0xE8||stage->mTevColorCD!=0xA2||
   stage->mTevAlphaOp!=0x08||stage->mTevAlphaAB!=0xFA||stage->mTevSwapModeInfo!=0x70||
   tev->getTevKColorSel(0)!=GX_TEV_KCSEL_K0||tev->getTevKAlphaSel(0)!=GX_TEV_KASEL_K3_A||
   konst->r||konst->g||konst->b||order->getTevOrderInfo().mTexCoord!=GX_TEXCOORD0||order->getTexMap()!=GX_TEXMAP0||
   order->getTevOrderInfo().mColorChan!=GX_COLOR0A0||tev->getTexNo(0)!=2-i)return false;
 }
 if(!textures_->getImgDataPtr(0)||textures_->getImgDataPtr(0)!=textures_->getImgDataPtr(1)||
  textures_->getImgDataPtr(0)!=textures_->getImgDataPtr(2))return false;
 auto& vertex=data->getVertexData();
 return vertex.getVtxColorArray(0)==nullptr&&vertex.getVtxColorArray(1)==nullptr;
}
// Builds (once) a private J3DTexture whose three ResTIMG entries point to a private recolored copy of the atlas.
// The original ResTIMG array, image bytes, GXTexObjs and J3DModelData are only READ.
int private_texture_for(J3DTexture* original,const char*& why)noexcept{
 const std::uint8_t* source=original->getImgDataPtr(0);
 const std::size_t bytes=devf_tex::cmpr_level_bytes(128,128);
 const std::uint64_t hash=devf_tex::fnv1a(source,bytes);
 if(const int hit=devf_bind::find_cached(textures.data(),texture_count,source,hash);hit>=0)return hit;
 if(texture_count>=textures.size()){why="PRIVATE_TEXTURE_CACHE_FULL";return -1;}
 const u16 n=original->getNum();
 const std::size_t header=std::size_t(n)*sizeof(ResTIMG),image_offset=(header+31)&~std::size_t(31);
 auto* memory=new(std::nothrow) std::uint8_t[image_offset+bytes+32];
 if(!memory){why="PRIVATE_TEXTURE_ALLOCATION";return -1;}
 auto* base=reinterpret_cast<std::uint8_t*>((reinterpret_cast<std::uintptr_t>(memory)+31)&~std::uintptr_t(31));
 auto* res=reinterpret_cast<ResTIMG*>(base);std::uint8_t* image=base+image_offset;
 std::memcpy(image,source,bytes);
 devf_tex::Stats stats;
 if(!devf_tex::recolor_cmpr(image,bytes,devf_tex::kCharcoal,stats)||stats.blocks!=bytes/8||!stats.endpoints_changed){delete[] memory;why="RECOLOR_CONTRACT";return -1;}
 if(devf_tex::fnv1a(source,bytes)!=hash){delete[] memory;why="ORIGINAL_CHANGED_DURING_BUILD";return -1;}
 for(u16 i=0;i<n;++i){
  std::memcpy(&res[i],original->getResTIMG(i),sizeof(ResTIMG));
  res[i].imageOffset=static_cast<s32>(image-reinterpret_cast<std::uint8_t*>(&res[i]));
  res[i].paletteOffset=0;
 }
 J3DTexture* texture=nullptr;
 try{texture=new J3DTexture(n,res);}catch(...){texture=nullptr;}
 if(!texture){delete[] memory;why="PRIVATE_J3DTEXTURE_ALLOCATION";return -1;}
 for(u16 i=0;i<n;++i)if(texture->getImgDataPtr(i)!=image){why="PRIVATE_TEXTURE_POINTER_CONTRACT";return -1;} // keep memory: texture object may hold GX ids
 auto& t=textures[texture_count];t.source=source;t.source_hash=hash;t.memory=memory;t.texture=texture;t.stats=stats;
 return static_cast<int>(texture_count++);
}
void reject_texture(unsigned index,std::uint32_t actor,const char* why)noexcept{
 auto& b=bindings[index];
 if(!b.reject_logged){b.reject_logged=true;tlog("BLACK_TEXTURE_REJECTED",&table.regs[index],actor,why);}
}
auto packet_get=[](J3DMatPacket* p)noexcept{return p->mpTexture;};
auto packet_set=[](J3DMatPacket* p,J3DTexture* t)noexcept{p->setTexture(t);};
// CP10.6A: seven private purple variants of the SAME original atlas (t = k/7), built once per session on caller-owned copies. They are only ever
// bound to the EYE packet of one Black model; the original ResTIMG array, image bytes, GXTexObjs and J3DModelData are only READ.
// Builds the 7 private variants (k/7) of `original`'s atlas for one colour rule, on caller-owned copies (the original is only READ).
bool build_eye_variants(J3DTexture* original,const devf_eye::Params& p,std::uint8_t** memory_out,J3DTexture** texture_out,devf_eye::Stats& stats,const char*& why)noexcept{
 const std::uint8_t* source=original->getImgDataPtr(0);
 const std::size_t bytes=devf_tex::cmpr_level_bytes(128,128);
 const u16 n=original->getNum();
 const std::size_t header=std::size_t(n)*sizeof(ResTIMG),image_offset=(header+31)&~std::size_t(31);
 for(unsigned k=1;k<devf_eye::kVariants;++k){
  auto* memory=new(std::nothrow) std::uint8_t[image_offset+bytes+32];
  if(!memory){why="EYE_VARIANT_ALLOCATION";return false;}
  auto* base=reinterpret_cast<std::uint8_t*>((reinterpret_cast<std::uintptr_t>(memory)+31)&~std::uintptr_t(31));
  auto* res=reinterpret_cast<ResTIMG*>(base);std::uint8_t* image=base+image_offset;
  std::memcpy(image,source,bytes);
  devf_eye::Stats st;
  if(!devf_eye::recolor_cmpr(image,bytes,devf_eye::amount_pm(k),p,st)||st.blocks!=bytes/8){delete[] memory;why="EYE_RECOLOR_CONTRACT";return false;}
  if(k==devf_eye::kVariants-1){stats=st;if(!st.endpoints_changed){delete[] memory;why="NO_EYE_ENDPOINTS_CHANGED_EYES_WOULD_NOT_CHANGE";return false;}}
  for(u16 i=0;i<n;++i){
   std::memcpy(&res[i],original->getResTIMG(i),sizeof(ResTIMG));
   res[i].imageOffset=static_cast<s32>(image-reinterpret_cast<std::uint8_t*>(&res[i]));
   res[i].paletteOffset=0;
  }
  J3DTexture* texture=nullptr;try{texture=new J3DTexture(n,res);}catch(...){texture=nullptr;}
  if(!texture){delete[] memory;why="EYE_J3DTEXTURE_ALLOCATION";return false;}
  for(u16 i=0;i<n;++i)if(texture->getImgDataPtr(i)!=image){why="EYE_TEXTURE_POINTER_CONTRACT";return false;}   // keep memory: the object may hold GX ids
  memory_out[k]=memory;texture_out[k]=texture;
 }
 return true;
}
// CP10.6A Black eye set, now in the CP12 intense red of the Black aura (same detection, same 7-step fade, same binding).
bool build_eye_set(unsigned t,J3DTexture* original,const char*& why)noexcept{
 auto& set=eye_sets[t];
 if(set.built)return true;
 if(set.failed){why="EYE_VARIANTS_PREVIOUSLY_FAILED";return false;}
 set.failed=true;
 if(!build_eye_variants(original,devf_eyes12::kRedEyes,set.memory,set.texture,set.stats,why))return false;
 set.built=true;set.failed=false;
 return true;
}
// Called once per admitted generation (first native execute), after the HP write.
void bind_texture(unsigned index,daE_OC_c* actor,std::uint32_t id)noexcept{
 auto& b=bindings[index];b.bound=false;b.model=nullptr;b.texture=-1;b.generation=table.regs[index].generation;
 if(!actor->mpMorf){reject_texture(index,id,"NO_NATIVE_MORF");return;}
 J3DModel* model=actor->mpMorf->getModel();
 J3DModelData* data=model?model->getModelData():nullptr;
 if(!data||!model->mMatPacket||data->getShapeNum()!=3||data->getMaterialNum()!=3){reject_texture(index,id,"MODEL_CONTRACT");return;}
 if(!black_material_contract(data)){reject_texture(index,id,"E_OC2_EFFECTIVE_RESOURCE_CONTRACT");return;}
 J3DMatPacket* body0=model->getMatPacket(0);J3DMatPacket* body1=model->getMatPacket(1);J3DMatPacket* eye=model->getMatPacket(2);
 if(body0->getMaterial()!=data->getMaterialNodePointer(0)||body1->getMaterial()!=data->getMaterialNodePointer(1)||eye->getMaterial()!=data->getMaterialNodePointer(2)){reject_texture(index,id,"MAT_PACKET_MATERIAL_CONTRACT");return;}
 const char* why="NONE";const unsigned built_before=texture_count;
 const int t=private_texture_for(data->getTexture(),why);
 if(t<0){reject_texture(index,id,why);return;}
 const bool built=texture_count>built_before;
 if(const char* failed=devf_bind::bind_body(body0,body1,eye,data->getTexture(),textures[t].texture,packet_get,packet_set)){reject_texture(index,id,failed);return;}
 b.bound=true;b.model=model;b.texture=t;
 if(built){tlog("BLACK_TEXTURE_PREPARED",&table.regs[index],id,"PRIVATE_COPY_RECOLORED_ORIGINAL_UNCHANGED",t);}
 tlog("BLACK_TEXTURE_BOUND",&table.regs[index],id,"BODY_MATPACKETS_0_1_PRIVATE_EYE_AND_OTHER_MODELS_NATIVE",t);
 // CP10.6A: the purple eye variants are prepared now (never during combat). A failure only disables the eyes; the approved body texture is unaffected.
 {const bool first=!eye_sets[t].built&&!eye_sets[t].failed;const char* ewhy="NONE";
  if(!build_eye_set(static_cast<unsigned>(t),data->getTexture(),ewhy)){if(first)tlog("BLACK_EYES_UNAVAILABLE",&table.regs[index],id,ewhy,t);}
  else if(first)tlog("BLACK_EYES_PREPARED",&table.regs[index],id,"SEVEN_PRIVATE_RED_VARIANTS_OF_THE_ATLAS_FOR_THE_EYE_PACKET_ONLY",t);}
}
// ---- CP10.6D(-FIX02 0.8.8, body-hugging) visual feedback: reads the approved CP10.6 logic state (group / fury) and fades an aura (Blue and Black) and purple eyes (Black).
// Aura = ONE native keyed level emitter per actor (Blue AND Black: 0x0B9D, same canonical frame and geometry, different colour), driven like Link and kytag03 drive theirs: dComIfGp_particle_set(key,...) every tick
// (it creates or refreshes and moves the emitter), colour/alpha/scale set on the emitter, stopDrawParticle when done. NO sword matrix, sword model or Link bone is used: the
// emitter keeps the identity global rotation of JPABaseEmitter::init. An emitter nobody refreshes is removed by the engine itself (dPa_control_c::level_c::execute), so a
// death, a room change or an unload cannot orphan it. The engine's transient refusals (tevStr not ready, pool full) are bounded retries (strokes_policy.hpp). ----
int black_index_for(std::uint32_t id)noexcept{for(unsigned i=0;i<table.count;++i)if(table.regs[i].used&&table.regs[i].admitted_id==id)return static_cast<int>(i);return -1;}
const char* state_text(int level,bool target)noexcept{
 const auto st=devf_vis::state_of(level,target);
 return st==devf_vis::State::Active?"ACTIVE":st==devf_vis::State::FadingIn?"FADING_IN":st==devf_vis::State::FadingOut?"FADING_OUT":"INACTIVE";
}
const char* res_name(std::uint16_t id)noexcept{return id==devf_stk::kResource?"0x0B9D_ID_ZI_J_LK_SWL_C":"NONE";}
int res_slot(std::uint16_t id)noexcept{return id==devf_stk::kResource?0:-1;}
// Eyes and generic cleanup lines keep the CP10.6A prefix.
void xlog(const char* ev,devf_vis::Entry* v,const devf_variant::Entry* e,const char* part,const devf_vis::Event& x)noexcept{
 if(!svc_log||!svc_log->write||visual_logs>=400)return;
 if(v){if(v->events>=devf_vis::kEventsPerActor)return;++v->events;}
 ++visual_logs;char text[900]{};
 const auto variant=v?v->variant:devf_variant::Variant::None;
 const bool eyes_part=std::strcmp(part,"EYES")==0;
 const int level=eyes_part?(v?v->eye_level:0):(v?v->aura_level:0);
 const char* state=!v?"NONE":state_text(level,eyes_part?v->eye_target:v->aura_target);
 std::snprintf(text,sizeof(text),"DEVF_CP10_6A_VISUAL event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s source=%s encounter_id=%s part=%s state=%s level=%d eye_variant=%u of=%u reason=%s visual_pixels=HUMAN_RUNTIME_PENDING",
  ev,kVersion,v?v->id:0u,v?v->generation:0u,devf_variant::variant_text(variant),e?devf_variant::source_text(e->source):"NONE",e&&e->encounter?e->encounter:"NONE",part,state,level,
  x.eye_idx,devf_vis::kEyeVariants-1,x.reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// Aura lines: one line per significant transition, bounded per actor (shared with the eye lines) and per session. ACTOR_STATE_ON/OFF are the LOGIC state
// (group / fury); EMITTER_ACQUIRED is the engine handing us an emitter; NEITHER proves that anything is visible on screen (pixels=NOT_VERIFIED).
// route=SHARED_BODY_HUGGING_STROKES and the geometry tuple are printed on every line, for Blue and for Black alike: they come from the one devf_stk::kGeometry.
void slog(const char* ev,devf_vis::Entry* v,const devf_variant::Entry* e,const char* reason,unsigned retries,const devf_stk::Readback* cb,int alpha,std::uint16_t resource)noexcept{
 if(!svc_log||!svc_log->write||visual_logs>=400)return;
 if(v){if(v->events>=devf_vis::kEventsPerActor)return;++v->events;}
 ++visual_logs;char text[1100];text[0]=0;
 const auto variant=v?v->variant:devf_variant::Variant::None;const auto& g=devf_stk::geometry_for(variant);const auto& c=devf_stk::colour_for(variant);
 const int level=v?v->aura_level:0;
 if(!log_budget.detail(variant)&&!(cb&&!cb->ok)){   // CP11 compact state line (same event names and identifiers, no geometry / readback tuple)
  std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s encounter_id=%s state=%s level=%d resource=%s emitter_key=%u emitters_used=%u route=SHARED_BODY_HUGGING_STROKES retries=%u reason=%s detail=COMPACT pixels=NOT_VERIFIED",
   ev,kVersion,v?v->id:0u,v?v->generation:0u,devf_variant::variant_text(variant),e&&e->encounter?e->encounter:"NONE",v?state_text(level,v->aura_target):"NONE",level,res_name(resource),v?v->key:0u,vledger.live_auras(),retries,reason);
  try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
  return;
 }
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s source=%s encounter_id=%s state=%s level=%d alpha=%d resource=%s bank=%u emitter_key=%u emitters_used=%u emitters_max=%u emitters_per_actor=%u route=SHARED_BODY_HUGGING_STROKES geometry=(scale=%.2f,width=%.2f,length=%.2f,min_rad=%.2f,height=%.2f,sweep=%.2f,dir=(%.3f,%.3f,%.3f)) rgb_target=(%d,%d,%d) rgb_alpha_readback=(%d,%d,%d,%d) retries=%u reason=%s pixels=NOT_VERIFIED",
  ev,kVersion,v?v->id:0u,v?v->generation:0u,devf_variant::variant_text(variant),e?devf_variant::source_text(e->source):"NONE",e&&e->encounter?e->encounter:"NONE",
  v?state_text(level,v->aura_target):"NONE",level,alpha,res_name(resource),devf_stk::kBank,v?v->key:0u,vledger.live_auras(),devf_stk::kMaxEmitters,devf_stk::kEmittersPerActor,
  double(g.volume_scale),double(g.width_scale),double(g.length_scale),double(g.min_rad_pm)/1000.0,double(g.height_pm)/1000.0,double(g.sweep),double(g.dir_x),double(g.dir_y),double(g.dir_z),
  c.r,c.g,c.b,cb&&cb->ok?cb->r:-1,cb&&cb->ok?cb->g:-1,cb&&cb->ok?cb->b:-1,cb&&cb->ok?cb->a:-1,retries,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// One line per created emitter: the canonical frame and shared geometry READ BACK from the emitter, next to what the resource had authored ("authored_*").
// It proves the parameters on the emitter, not pixels.
void tlog(devf_vis::Entry* v,const devf_variant::Entry* e,const devf_stk::Note& n)noexcept{
 if(!svc_log||!svc_log->write||visual_logs>=400)return;
 if(v){if(v->events>=devf_vis::kEventsPerActor)return;++v->events;}
 ++visual_logs;char text[1700];text[0]=0;
 const auto variant=v?v->variant:devf_variant::Variant::None;const auto& g=devf_stk::geometry_for(variant);const auto& c=devf_stk::colour_for(variant);const auto& ap=n.applied;
 const int level=v?v->aura_level:0;const auto& f=stk_facts[0];
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=TUNING_APPLIED owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s encounter_id=%s resource=%s emitter_key=%u status=%s route=SHARED_BODY_HUGGING_STROKES aura_level=%d alpha_now=%d rgb=(%d,%d,%d) y_off=%.1f centre=%s centre_offset_xz=(%.1f,%.1f) radius_nominal=%.1f radius_inner=%.1f geometry=(scale=%.2f,width=%.2f,length=%.2f,height=%.2f) presence=(alpha_cap=%d,base_prm=(%d,%d,%d,%d),rate_step=%u,prm_anim=%u,base_prm_effective=%s) readback=(sweep=%.3f,min_rad=%.3f,local_trs=0,local_rot=0,local_scl=1,dir=(%.4f,%.4f,%.4f)) authored=(sweep=%.3f,min_rad=%.3f,trs=(%.1f,%.1f,%.1f),rot=(%d,%d,%d),scl=(%.3f,%.3f,%.3f),dir=(%.3f,%.3f,%.3f)) pixels=NOT_VERIFIED",
  kVersion,v?v->id:0u,v?v->generation:0u,devf_variant::variant_text(variant),e&&e->encounter?e->encounter:"NONE",res_name(n.resource),n.key,n.why,level,devf_stk::shared_alpha(level),c.r,c.g,c.b,
  double(ap.y_off),ap.torso?"TORSO_COLLIDERS":"FEET_FALLBACK",double(ap.cdx),double(ap.cdz),double(ap.radius),double(ap.inner),double(g.volume_scale),double(g.width_scale),double(g.length_scale),double(g.height_pm)/1000.0,
  devf_stk::kAlphaCap,n.color.base_prm.r,n.color.base_prm.g,n.color.base_prm.b,n.color.base_prm.a,n.color.rate_step,f.prm_anim,f.prm_anim?"NO_RESOURCE_ANIMATES_PRM":"YES",double(n.color.sweep),double(n.color.min_rad),double(n.color.dir.x),double(n.color.dir.y),double(n.color.dir.z),
  double(f.sweep),double(f.min_rad),double(f.tx),double(f.ty),double(f.tz),f.rx,f.ry,f.rz,double(f.sx),double(f.sy),double(f.sz),double(f.dx),double(f.dy),double(f.dz));
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// FIX01: one line per created emitter. Engine draw-basis arithmetic on read-back values; evidence=MATHEMATICAL_READBACK_NOT_RENDERED, never a pixel claim.
void blog(devf_vis::Entry* v,const devf_variant::Entry* e,const devf_stk::Note& n)noexcept{
 if(!svc_log||!svc_log->write||visual_logs>=400)return;
 if(v){if(v->events>=devf_vis::kEventsPerActor)return;++v->events;}
 ++visual_logs;char text[900];text[0]=0;const auto& b=n.color.basis;const auto variant=v?v->variant:devf_variant::Variant::None;
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=DRAW_BASIS_CHECK owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s encounter_id=%s resource=%s emitter_key=%u route=SHARED_BODY_HUGGING_STROKES dir_applied=(%.4f,%.4f,%.4f) base_axis=(%.4f,%.4f,%.4f) axis_y=(%.4f,%.4f,%.4f) dot=%.4f cross_len=%.4f cross_sq=%.6f engine_threshold=%.7f status=%s evidence=MATHEMATICAL_READBACK_NOT_RENDERED",
  kVersion,v?v->id:0u,v?v->generation:0u,devf_variant::variant_text(variant),e&&e->encounter?e->encounter:"NONE",res_name(n.resource),n.key,
  double(n.color.dir.x),double(n.color.dir.y),double(n.color.dir.z),double(b.base.x),double(b.base.y),double(b.base.z),double(b.axis.x),double(b.axis.y),double(b.axis.z),
  double(b.dot),double(b.cross_len),double(b.cross_sq),double(devf_stk::kJpaEpsilon),n.why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// Read-only scalars of one key curve (inline getters of the loaded resource): id, loop, count, first and last (time,value) x1000. Nothing is modified.
struct KeyEnds {unsigned id=255,n=0,loop=0;int t0=0,v0=0,t1=0,v1=0;};
KeyEnds key_ends(const JPAResource* res,int i)noexcept{
 KeyEnds k;
 if(res->keyNum>i&&res->ppKey&&res->ppKey[i]){
  const auto* kb=res->ppKey[i];k.id=kb->getID();k.n=kb->getKeyNum();k.loop=kb->isLoop()?1u:0u;
  const auto* d=kb->getKeyData();
  if(d&&k.n>=1&&k.n<=32){const float t0=d[0],v0=d[1],t1=d[(k.n-1)*4],v1=d[(k.n-1)*4+1];k.t0=int(t0*1000.0f);k.v0=int(v0*1000.0f);k.t1=int(t1*1000.0f);k.v1=int(v1*1000.0f);}
 }
 return k;
}
void glog(std::uint16_t id,const JPAResource* res)noexcept{   // RESOURCE_GEOMETRY (authored frame) + CURVE_PROBE (static key table), once per session (bounded by plog)
 if(!svc_log||!svc_log->write)return;const int slot=res_slot(id);if(slot<0)return;const auto& f=stk_facts[slot];char text[900]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=RESOURCE_GEOMETRY owner=dev.bokoblin.expansion version=%s resource=%s bank=%u authored_sweep_x1000=%d authored_min_rad_x1000=%d local_trs_x1000=(%d,%d,%d) local_rot=(%d,%d,%d) local_scl_x1000=(%d,%d,%d) emitter_dir_x1000=(%d,%d,%d) volume_size=%u authored_rate_step=%u authored_prm_rgba=(%u,%u,%u,%u) prm_anim=%u replaced_on_each_emitter_by=CANONICAL_FRAME pixels=NOT_VERIFIED",
  kVersion,res_name(id),devf_stk::kBank,int(f.sweep*1000.0f),int(f.min_rad*1000.0f),int(f.tx*1000.0f),int(f.ty*1000.0f),int(f.tz*1000.0f),f.rx,f.ry,f.rz,int(f.sx*1000.0f),int(f.sy*1000.0f),int(f.sz*1000.0f),
  int(f.dx*1000.0f),int(f.dy*1000.0f),int(f.dz*1000.0f),f.volume_size,f.rate_step,unsigned(f.prm.r),unsigned(f.prm.g),unsigned(f.prm.b),unsigned(f.prm.a),f.prm_anim);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
 const KeyEnds a=key_ends(res,0),b=key_ends(res,1);
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=CURVE_PROBE owner=dev.bokoblin.expansion version=%s resource=%s bank=%u source=KEY_TABLE_READ_ONLY key0=(id=%u,loop=%u,n=%u,first_t_v_x1000=(%d,%d),last_t_v_x1000=(%d,%d),last_positive=%d) key1=(id=%u,loop=%u,n=%u,first_t_v_x1000=(%d,%d),last_t_v_x1000=(%d,%d),last_positive=%d) note=ID0_RATE_ID4_LIFETIME_REAPPLIED_EVERY_TICK curve_end=%s pixels=NOT_VERIFIED",
  kVersion,res_name(id),devf_stk::kBank,a.id,a.loop,a.n,a.t0,a.v0,a.t1,a.v1,a.v1>0?1:0,b.id,b.loop,b.n,b.t0,b.v0,b.t1,b.v1,b.v1>0?1:0,(a.n&&b.n)?"READ":"CURVE_END_UNVERIFIED");
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void llog(std::uint16_t id,const JPABaseEmitter* em)noexcept{   // live sample of the emitter's own mRate / mLifeTime (what calcKey evaluated this tick); once per session
 if(!svc_log||!svc_log->write)return;char text[600]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=CURVE_PROBE owner=dev.bokoblin.expansion version=%s resource=%s bank=%u source=LIVE_EMITTER_READ_ONLY emitter_tick=%u rate_x1000=%d lifetime=%d min_rad_x1000=%d sweep_x1000=%d pixels=NOT_VERIFIED",
  kVersion,res_name(id),devf_stk::kBank,unsigned(em->mTick),int(em->mRate*1000.0f),int(em->mLifeTime),int(em->mVolumeMinRad*1000.0f),int(em->mVolumeSweep*1000.0f));
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void plog(std::uint16_t id,bool ok,const char* reason,const JPAEmitterManager* m,const JPAResource* res)noexcept{
 const int slot=res_slot(id);if(slot<0||!svc_log||!svc_log->write||probe_logs[slot]>=3)return;++probe_logs[slot];char text[1500]{};
 if(!ok||!res){
  std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=RESOURCE_PROBE owner=dev.bokoblin.expansion version=%s resource=%s bank=%u ok=0 reason=%s banks=%u groups=%u pixels=NOT_VERIFIED",kVersion,res_name(id),devf_stk::kBank,reason,m?unsigned(m->ridMax):0u,m?unsigned(m->gidMax):0u);
 }else{
  // Scalar metadata of the loaded resource, read-only (inline getters, no import).
  const auto* dyn=res->getDyn();const auto* bsp=res->getBsp();const auto* esp=res->getEsp();
  JGeometry::TVec3<f32> dir(0,0,0);if(dyn)dyn->getEmitterDir(&dir);GXColor prm{},env{};if(bsp){bsp->getPrmClr(&prm);bsp->getEnvClr(&env);}
  unsigned kid0=255,kloop0=0,knum0=0,kid1=255,kloop1=0,knum1=0;
  if(res->keyNum>0&&res->ppKey&&res->ppKey[0]){kid0=res->ppKey[0]->getID();kloop0=res->ppKey[0]->isLoop()?1u:0u;knum0=res->ppKey[0]->getKeyNum();}
  if(res->keyNum>1&&res->ppKey&&res->ppKey[1]){kid1=res->ppKey[1]->getID();kloop1=res->ppKey[1]->isLoop()?1u:0u;knum1=res->ppKey[1]->getKeyNum();}
  const bool sa=esp&&esp->isEnableScaleAnm();
  std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=RESOURCE_PROBE owner=dev.bokoblin.expansion version=%s resource=%s bank=%u ok=1 used_by=BLUE_BLACK_AND_RED banks=%u groups=%u user_work=0x%08x dyn_flags=0x%08x volume_type=%u volume_size=%u max_frame=%d start_frame=%d life=%d rate_x1000=%d rate_rndm_x1000=%d vel_dir_x1000=%d emitter_dir_x1000=(%d,%d,%d) shape_type=%u dir_type=%u rot_type=%u base_size_x100=(%d,%d) clr_anm=%u prm_rgba=(%u,%u,%u,%u) env_rgb=(%u,%u,%u) keys=%u key0=(id=%u,loop=%u,n=%u) key1=(id=%u,loop=%u,n=%u) fields=%u textures=%u child=%d extra=%d scale_anim=%d scale_in_x100=(%d,%d) scale_out_x100=(%d,%d) scale_in_t_x100=%d scale_out_t_x100=%d pixels=NOT_VERIFIED",
   kVersion,res_name(id),devf_stk::kBank,unsigned(m->ridMax),unsigned(m->gidMax),dyn?unsigned(dyn->getResUserWork()):0u,dyn?unsigned(dyn->getFlag()):0u,dyn?unsigned(dyn->getVolumeType()):0u,dyn?unsigned(dyn->getVolumeSize()):0u,
   dyn?int(dyn->getMaxFrame()):0,dyn?int(dyn->getStartFrame()):0,dyn?int(dyn->getLifetime()):0,dyn?int(dyn->getRate()*1000.0f):0,dyn?int(dyn->getRateRndm()*1000.0f):0,dyn?int(dyn->getInitVelDir()*1000.0f):0,
   int(dir.x*1000.0f),int(dir.y*1000.0f),int(dir.z*1000.0f),bsp?bsp->getType():0u,bsp?bsp->getDirType():0u,bsp?bsp->getRotType():0u,bsp?int(bsp->getBaseSizeX()*100.0f):0,bsp?int(bsp->getBaseSizeY()*100.0f):0,bsp?bsp->getClrAnmType():0u,
   unsigned(prm.r),unsigned(prm.g),unsigned(prm.b),unsigned(prm.a),unsigned(env.r),unsigned(env.g),unsigned(env.b),unsigned(res->keyNum),kid0,kloop0,knum0,kid1,kloop1,knum1,unsigned(res->fldNum),unsigned(res->texNum),res->getCsp()?1:0,res->getEsp()?1:0,
   sa?1:0,sa?int(esp->getScaleInValueX()*100.0f):0,sa?int(esp->getScaleInValueY()*100.0f):0,sa?int(esp->getScaleOutValueX()*100.0f):0,sa?int(esp->getScaleOutValueY()*100.0f):0,sa?int(esp->getScaleInTiming()*100.0f):0,sa?int(esp->getScaleOutTiming()*100.0f):0);
 }
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
 if(ok&&res)glog(id,res);
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=%s owner=dev.bokoblin.expansion version=%s resource=%s bank=%u used_by=BLUE_BLACK_AND_RED reason=%s pixels=NOT_VERIFIED",ok&&res?"RESOURCE_FOUND":"RESOURCE_MISSING",kVersion,res_name(id),devf_stk::kBank,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// Once per session: the common particle bank must hold THIS user index (the engine's set() reads the resource's user work before checking existence).
// A failure is a retryable condition (the bank may not be loaded yet), logged at most 3 times.
bool aura_probe(std::uint16_t id,const char*& why)noexcept{
 const int slot=res_slot(id);if(slot<0){why="NOT_A_PILOT_RESOURCE";return false;}
 if(probe_ok[slot])return true;
 auto* m=dPa_control_c::getEmitterManager();
 if(!m){why="PARTICLE_EMITTER_MANAGER_NULL";plog(id,false,why,nullptr,nullptr);return false;}
 if(!m->pResMgrAry||m->ridMax<=devf_stk::kBank||!m->pResMgrAry[devf_stk::kBank]){why="COMMON_PARTICLE_BANK_NOT_LOADED";plog(id,false,why,m,nullptr);return false;}
 auto* res=m->pResMgrAry[devf_stk::kBank]->getResource(id);
 if(!res||res->getUsrIdx()!=id){why="RESOURCE_MISSING_FROM_COMMON_BANK";plog(id,false,why,m,nullptr);return false;}
 probe_ok[slot]=true;
 const auto* dyn=res->getDyn();probe_max_frame[slot]=dyn?dyn->getMaxFrame():0;
 {ResFacts f;   // inline getters only: the resource's AUTHORED frame (logged; each emitter then gets the canonical frame)
  if(dyn){JGeometry::TVec3<f32> tr(0,0,0),dr(0,1,0),sc(1,1,1);JGeometry::TVec3<s16> rt;rt.x=rt.y=rt.z=0;dyn->getEmitterTrs(&tr);dyn->getEmitterDir(&dr);dyn->getEmitterRot(&rt);dyn->getEmitterScl(&sc);
   f.tx=tr.x;f.ty=tr.y;f.tz=tr.z;f.dx=dr.x;f.dy=dr.y;f.dz=dr.z;f.sx=sc.x;f.sy=sc.y;f.sz=sc.z;f.rx=rt.x;f.ry=rt.y;f.rz=rt.z;
   f.sweep=dyn->getVolumeSweep();f.min_rad=dyn->getVolumeMinRad();f.volume_size=dyn->getVolumeSize();f.rate_step=dyn->getRateStep();}
  if(const auto* bsp=res->getBsp()){f.prm_anim=bsp->isPrmAnm()?1u:0u;bsp->getPrmClr(&f.prm);}
  stk_facts[slot]=f;}
 plog(id,true,"RESOURCE_REGISTERED_IN_COMMON_BANK",m,res);
 return true;
}
struct RealEngine {
 daE_OC_c* a=nullptr;s32 cur_max_frame=0;std::uint16_t cur_res=0;
 bool probe(std::uint16_t id,const char*& why){const bool ok=aura_probe(id,why);if(ok){const int slot=res_slot(id);cur_max_frame=slot>=0?probe_max_frame[slot]:0;cur_res=id;}return ok;}
 bool ready(){return a&&a->tevStr.Material_use_fg==0x7c;}   // dPa_control_c::set returns NULL otherwise (d_particle.cpp L1481)
 std::uint32_t set(std::uint16_t id,std::uint32_t key,float x,float y,float z){
  const cXyz pos(x,y,z);
  return dComIfGp_particle_set(key,id,&pos,&a->tevStr,nullptr,nullptr,0xFF,nullptr,-1,nullptr,nullptr,nullptr);   // no rotation: the emitter keeps its identity global rotation
 }
 // Canonical frame + shared geometry on THIS emitter instance (the shared JPAResource is never written). Applied every tick: idempotent, and nothing in the
 // engine rewrites these fields after JPABaseEmitter::init (the resource's two key curves drive only mRate and mLifeTime).
 bool configure(std::uint32_t key,const devf_stk::Geometry& g,const devf_stk::Colour& c,int alpha,bool created,devf_stk::Readback& rb){
  JPABaseEmitter* em=dComIfGp_particle_getEmitter(key);
  if(!em)return false;
  const JGeometry::TVec3<f32> zero(0.0f,0.0f,0.0f),one(1.0f,1.0f,1.0f),dir(g.dir_x,g.dir_y,g.dir_z);
  em->setLocalTranslation(zero);                                          // ring centre = the position we pass (no authored sword offset)
  em->mLocalRot.x=0;em->mLocalRot.y=0;em->mLocalRot.z=0;                   // ring in the horizontal plane (no authored tilt)
  em->setLocalScale(one);                                                 // round ring (no authored squash)
  em->setDirection(dir);                                                  // FIX01: 5.7 deg from vertical; exactly (0,1,0) made cross(mBaseAxis,dir)=0 and the engine skipped every quad
  em->mVolumeSweep=g.sweep;                                               // FULL circle: no fixed world sector
  em->mVolumeMinRad=float(g.min_rad_pm)/1000.0f;                          // spawn band outside the torso collider
  em->setGlobalDynamicsScale(JGeometry::TVec3<float>(g.volume_scale,g.volume_scale,g.volume_scale));   // ring radius
  em->setGlobalParticleScale(g.width_scale,g.length_scale);               // quad width (X) and length along the direction (Y)
  em->mPrmClr.r=static_cast<u8>(devf_stk::kBasePrm.r);em->mPrmClr.g=static_cast<u8>(devf_stk::kBasePrm.g);   // FIX02: THIS emitter's base colour (resource
  em->mPrmClr.b=static_cast<u8>(devf_stk::kBasePrm.b);em->mPrmClr.a=static_cast<u8>(devf_stk::kBasePrm.a);   // copy: 255,255,195,150) -> opaque white
  em->setRateStep(static_cast<u8>(devf_stk::kRateStep));                  // FIX02: emit on every tick
  em->setGlobalPrmColor(static_cast<u8>(c.r),static_cast<u8>(c.g),static_cast<u8>(c.b));
  em->setGlobalEnvColor(static_cast<u8>(c.er),static_cast<u8>(c.eg),static_cast<u8>(c.eb));
  em->setGlobalAlpha(static_cast<u8>(alpha));
  if(created&&cur_max_frame!=0)em->becomeContinuousParticle();   // a finite-duration effect must keep emitting while the buff lasts
  // CP11: the writes above stay EVERY tick (dPa_control_c::set -> dPa_kankyocolor_set can re-apply environment colours on each keyed refresh, and the
  // frame must hold while the emitter lives). The read-backs below only feed the creation lines (AuraPart uses `rb` only when `created`), so they now
  // run once per created emitter instead of every tick.
  if(created){
  GXColor rc{};em->getGlobalPrmColor(rc);
  rb.ok=rc.r==c.r&&rc.g==c.g&&rc.b==c.b&&rc.a==alpha;rb.r=rc.r;rb.g=rc.g;rb.b=rc.b;rb.a=rc.a;
  rb.base_prm=devf_stk::Rgba{em->mPrmClr.r,em->mPrmClr.g,em->mPrmClr.b,em->mPrmClr.a};rb.rate_step=em->mRateStep;
  rb.presence_ok=rb.base_prm.r==devf_stk::kBasePrm.r&&rb.base_prm.g==devf_stk::kBasePrm.g&&rb.base_prm.b==devf_stk::kBasePrm.b&&rb.base_prm.a==devf_stk::kBasePrm.a&&rb.rate_step==devf_stk::kRateStep;
  JGeometry::TVec3<f32> ds(0,0,0),ps(0,0,0);em->getGlobalDynamicsScale(&ds);em->getGlobalParticleScale(&ps);rb.min_rad=em->mVolumeMinRad;rb.sweep=em->mVolumeSweep;
  rb.geo_ok=ds.x==g.volume_scale&&ds.y==g.volume_scale&&ds.z==g.volume_scale&&ps.x==g.width_scale&&ps.y==g.length_scale&&em->mVolumeMinRad==float(g.min_rad_pm)/1000.0f&&em->mVolumeSweep==g.sweep&&
   em->mLocalTrs.x==0.0f&&em->mLocalTrs.y==0.0f&&em->mLocalTrs.z==0.0f&&em->mLocalRot.x==0&&em->mLocalRot.y==0&&em->mLocalRot.z==0&&
   em->mLocalScl.x==1.0f&&em->mLocalScl.y==1.0f&&em->mLocalScl.z==1.0f&&em->mLocalDir.x==g.dir_x&&em->mLocalDir.y==g.dir_y&&em->mLocalDir.z==g.dir_z;
  // Draw basis from READ-BACK emitter state, with the engine's own rule (init_p: mBaseAxis = Y column of global x local rotation; JPADrawDirection:
  // axisZ = cross(mBaseAxis, normalize(global x local rotation x mLocalDir)), no draw if |axisZ|^2 <= 32 FLT_EPSILON). Local rotation is read back 0
  // above (geo_ok), so global x local = mGlobalRot. Arithmetic only: it says whether the engine would submit the quad, not what it looks like.
  {const auto& m=em->mGlobalRot;const devf_stk::V3 d{em->mLocalDir.x,em->mLocalDir.y,em->mLocalDir.z};
   const devf_stk::V3 base{m[0][1],m[1][1],m[2][1]};
   const devf_stk::V3 axis{m[0][0]*d.x+m[0][1]*d.y+m[0][2]*d.z,m[1][0]*d.x+m[1][1]*d.y+m[1][2]*d.z,m[2][0]*d.x+m[2][1]*d.y+m[2][2]*d.z};
   rb.dir=d;rb.basis=devf_stk::draw_basis(base,axis);}
  }
  const int slot=res_slot(cur_res);
  if(slot>=0&&!curve_live[slot]&&em->mTick>=kCurveSampleTick){curve_live[slot]=true;llog(cur_res,em);}   // one read-only sample of the emitter's own rate / lifetime per session
  return true;
 }
 void stop(std::uint32_t key){if(JPABaseEmitter* em=dComIfGp_particle_getEmitter(key))em->stopDrawParticle();}   // then nobody refreshes it: the engine removes it
};
// ---- CP12 Blue eyes: per-instance contract, per-atlas private cyan set (built at spawn, never during combat), per-generation ownership ----
struct BlueEyeSet {bool built=false,failed=false,fallback=false;const std::uint8_t* source=nullptr;std::uint64_t hash=0;std::size_t endpoints=0;
 std::uint8_t* memory[devf_eye::kVariants]{};J3DTexture* texture[devf_eye::kVariants]{};devf_eye::Stats stats;const char* why="NONE";};
std::array<BlueEyeSet,4> blue_eye_sets{};unsigned blue_eye_count=0,blue_eye_logs=0;
std::array<devf_eyes12::EyeOwner,devf_vis::kCapacity> eye_owners{};
devf_pw::Window perf_win;devf_pw::Clock perf_clock;std::uint64_t perf_last=0;unsigned perf_windows=0;
// Read-only contract of THIS instance's eye material packet (same family as the CP10.2 Black contract, eye part only).
bool eye_contract(daE_OC_c* a,J3DModel*& model,J3DMatPacket*& eye,J3DTexture*& original,const char*& why)noexcept{
 if(!a||!a->mpMorf){why="NO_NATIVE_MORF";return false;}
 model=a->mpMorf->getModel();J3DModelData* data=model?model->getModelData():nullptr;
 if(!data||!model->mMatPacket||data->getMaterialNum()!=3||data->getShapeNum()!=3){why="EYE_MODEL_CONTRACT";return false;}
 J3DMaterial* mat=data->getMaterialNodePointer(2);
 if(!mat||!mat->mMaterialName||std::strcmp(mat->mMaterialName,"oc_eye_mat")){why="EYE_MATERIAL_NAME";return false;}
 auto* tev=mat->getTevBlock();
 if(!tev||tev->getTevStageNum()!=1||tev->getTexNo(0)!=0){why="EYE_TEV_CONTRACT";return false;}
 eye=model->getMatPacket(2);
 if(!eye||eye->getMaterial()!=mat){why="EYE_PACKET_CONTRACT";return false;}
 original=data->getTexture();
 if(!original||original->getNum()!=3){why="EYE_TEXTURE_COUNT";return false;}
 const auto* img=original->getResTIMG(0);
 if(!img||img->format!=GX_TF_CMPR||img->width!=128||img->height!=128||img->mipmapCount!=1||!original->getImgDataPtr(0)){why="EYE_ATLAS_CONTRACT";return false;}
 return true;
}
// The cyan set for this atlas: cached by (image pointer, FNV of its bytes); detection chosen per atlas (devf_eyes12::choose).
int blue_eye_set_for(J3DTexture* original,const char*& why,bool& built)noexcept{
 built=false;
 const std::uint8_t* source=original->getImgDataPtr(0);const std::size_t bytes=devf_tex::cmpr_level_bytes(128,128);
 const std::uint64_t hash=devf_tex::fnv1a(source,bytes);
 for(unsigned i=0;i<blue_eye_count;++i){auto& b=blue_eye_sets[i];if(b.source==source&&b.hash==hash){if(b.built)return int(i);why=b.why;return -1;}}
 if(blue_eye_count>=blue_eye_sets.size()){why="BLUE_EYE_CACHE_FULL";return -1;}
 auto& b=blue_eye_sets[blue_eye_count++];b.source=source;b.hash=hash;b.failed=true;
 const auto c=devf_eyes12::choose(devf_eyes12::Scheme::BlueCyan,source,bytes);
 b.endpoints=c.endpoints;b.fallback=c.fallback;
 if(!c.params){b.why=c.why;why=c.why;return -1;}
 if(!build_eye_variants(original,*c.params,b.memory,b.texture,b.stats,why)){b.why=why;return -1;}
 if(devf_tex::fnv1a(source,bytes)!=hash){b.why="ORIGINAL_CHANGED_DURING_BUILD";why=b.why;return -1;}   // keep memory: textures may hold GX ids
 b.built=true;b.failed=false;built=true;
 return int(&b-blue_eye_sets.data());
}
void blue_eye_log(const char* ev,const devf_variant::Entry* e,const char* reason,int set)noexcept{
 if(!svc_log||!svc_log->write||blue_eye_logs>=16)return;++blue_eye_logs;char text[640];text[0]=0;
 const auto* b=set>=0&&unsigned(set)<blue_eye_count?&blue_eye_sets[set]:nullptr;
 std::snprintf(text,sizeof(text),"DEVF_CP12_EYES event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=BLUE detection=%s eye_endpoints=%llu endpoints_changed=%llu tint=(%d,%d,%d) mechanism=PRIVATE_CMPR_VARIANTS_EYE_PACKET_ONLY reason=%s visual=HUMAN_RUNTIME_PENDING",
  ev,kVersion,e?e->id:0u,e?e->generation:0u,b?(b->fallback?"ANY_CHROMATIC_ENDPOINT":"GREEN_EYE_ENDPOINT"):"NONE",
  static_cast<unsigned long long>(b?b->endpoints:0),static_cast<unsigned long long>(b?b->stats.endpoints_changed:0),devf_stk::kBlueColour.r,devf_stk::kBlueColour.g,devf_stk::kBlueColour.b,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// At the Blue's classification (first execute, i.e. spawn / room load): prepare the cyan set for its atlas, so no allocation or recolour
// ever happens during combat. A failure only disables the Blue eyes (logged, bounded); nothing is written to the model here.
void prepare_blue_eyes(daE_OC_c* a,const devf_variant::Entry* entry)noexcept{
 J3DModel* model=nullptr;J3DMatPacket* eye=nullptr;J3DTexture* original=nullptr;const char* why="NONE";
 if(!eye_contract(a,model,eye,original,why)){blue_eye_log("BLUE_EYES_UNAVAILABLE",entry,why,-1);return;}
 bool built=false;const int s=blue_eye_set_for(original,why,built);
 if(s<0){blue_eye_log("BLUE_EYES_UNAVAILABLE",entry,why,-1);return;}
 if(built)blue_eye_log("BLUE_EYES_PREPARED",entry,"SEVEN_PRIVATE_CYAN_VARIANTS_OF_THE_ATLAS_FOR_THE_EYE_PACKET_ONLY",s);
}
// ---- CP14 Red gold eyes: the Blue machinery's twin (same contract, same 7 private variants, built at the Red's classification, same ownership) ----
std::array<BlueEyeSet,4> gold_eye_sets{};unsigned gold_eye_count=0,gold_eye_logs=0;
int gold_eye_set_for(J3DTexture* original,const char*& why,bool& built)noexcept{
 built=false;
 const std::uint8_t* source=original->getImgDataPtr(0);const std::size_t bytes=devf_tex::cmpr_level_bytes(128,128);
 const std::uint64_t hash=devf_tex::fnv1a(source,bytes);
 for(unsigned i=0;i<gold_eye_count;++i){auto& b=gold_eye_sets[i];if(b.source==source&&b.hash==hash){if(b.built)return int(i);why=b.why;return -1;}}
 if(gold_eye_count>=gold_eye_sets.size()){why="GOLD_EYE_CACHE_FULL";return -1;}
 auto& b=gold_eye_sets[gold_eye_count++];b.source=source;b.hash=hash;b.failed=true;
 const auto c=devf_red::choose_gold(source,bytes);
 b.endpoints=c.endpoints;
 if(!c.params){b.why=c.why;why=c.why;return -1;}
 if(!build_eye_variants(original,*c.params,b.memory,b.texture,b.stats,why)){b.why=why;return -1;}
 if(devf_tex::fnv1a(source,bytes)!=hash){b.why="ORIGINAL_CHANGED_DURING_BUILD";why=b.why;return -1;}   // keep memory: textures may hold GX ids
 b.built=true;b.failed=false;built=true;
 return int(&b-gold_eye_sets.data());
}
void gold_eye_log(const char* ev,const devf_variant::Entry* e,const char* reason,int set)noexcept{
 if(!svc_log||!svc_log->write||gold_eye_logs>=16)return;++gold_eye_logs;char text[640];text[0]=0;
 const auto* b=set>=0&&unsigned(set)<gold_eye_count?&gold_eye_sets[set]:nullptr;
 std::snprintf(text,sizeof(text),"DEVF_CP14_EYES event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=RED detection=%s eye_endpoints=%llu endpoints_changed=%llu tint=(%d,%d,%d) mechanism=PRIVATE_CMPR_VARIANTS_EYE_PACKET_ONLY reason=%s visual=HUMAN_RUNTIME_PENDING",
  ev,kVersion,e?e->id:0u,e?e->generation:0u,b?"GREEN_EYE_ENDPOINT":"NONE",
  static_cast<unsigned long long>(b?b->endpoints:0),static_cast<unsigned long long>(b?b->stats.endpoints_changed:0),devf_stk::kRedGoldColour.r,devf_stk::kRedGoldColour.g,devf_stk::kRedGoldColour.b,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// At the Red's classification (spawn / room load): prepare the gold set for its atlas, so nothing is allocated or recoloured during combat.
void prepare_gold_eyes(daE_OC_c* a,const devf_variant::Entry* entry)noexcept{
 J3DModel* model=nullptr;J3DMatPacket* eye=nullptr;J3DTexture* original=nullptr;const char* why="NONE";
 if(!eye_contract(a,model,eye,original,why)){gold_eye_log("RED_GOLD_EYES_UNAVAILABLE",entry,why,-1);return;}
 bool built=false;const int s=gold_eye_set_for(original,why,built);
 if(s<0){gold_eye_log("RED_GOLD_EYES_UNAVAILABLE",entry,why,-1);return;}
 if(built)gold_eye_log("RED_GOLD_EYES_PREPARED",entry,"SEVEN_PRIVATE_GOLD_VARIANTS_OF_THE_ATLAS_FOR_THE_EYE_PACKET_ONLY",s);
}
struct GameFx {
 daE_OC_c* a=nullptr;devf_stk::AuraPart<RealEngine>* part=nullptr;const char* last="NONE";
 const char* reason()const{return last;}
 bool aura(devf_vis::Entry& e,int l,int alpha){const bool ok=part->aura(e,l,alpha);last=part->why;return ok;}
 void aura_release(devf_vis::Entry& e){part->aura_release(e);}
 // idx 0 = native texture, 1..7 = private variants (Black red / Blue cyan). Writes only if the eye packet still holds exactly what this module put there.
 bool eye(devf_vis::Entry& e,unsigned idx){
  if(e.variant==devf_variant::Variant::Blue)return blue_eye(e,idx);
  if(e.variant==devf_variant::Variant::Red)return gold_eye(e,idx);   // CP14
  if(e.variant!=devf_variant::Variant::Black){last="NO_EYE_ROUTE_FOR_VARIANT";return false;}
  const char*& why=last;
  const int bi=black_index_for(e.id);
  if(bi<0){why="NO_BLACK_ADMISSION_RECORD";return false;}
  auto& b=bindings[bi];
  if(!b.bound||b.texture<0||!eye_sets[b.texture].built){why="EYE_VARIANTS_UNAVAILABLE";return false;}
  if(!a||!a->mpMorf){why="NO_NATIVE_MORF";return false;}
  J3DModel* model=a->mpMorf->getModel();
  if(!model||model!=b.model||!model->getModelData()){why="MODEL_CHANGED";return false;}
  J3DMatPacket* eye=model->getMatPacket(2);J3DTexture* original=model->getModelData()->getTexture();
  if(!eye||!original||eye->getMaterial()!=model->getModelData()->getMaterialNodePointer(2)){why="EYE_PACKET_CONTRACT";return false;}
  auto& set=eye_sets[b.texture];
  J3DTexture* expected=e.eye_idx==0?original:set.texture[e.eye_idx];
  if(!expected||eye->mpTexture!=expected){why="EYE_PACKET_TEXTURE_NOT_OURS";return false;}
  J3DTexture* target=idx==0?original:set.texture[idx];
  if(!target){why="EYE_VARIANT_MISSING";return false;}
  if(target==expected)return true;
  eye->setTexture(target);
  if(eye->mpTexture!=target){eye->setTexture(expected);why="EYE_WRITE_READBACK_MISMATCH";return false;}
  ++perf_win.eye_swaps;
  return true;
 }
 // CP12 Blue: contract re-checked on every eye step (7 per fade, never per tick); the set is bound once per actor generation.
 bool blue_eye(devf_vis::Entry& e,unsigned idx){
  const char*& why=last;
  auto& o=devf_eyes12::owner_for(eye_owners,static_cast<std::size_t>(&e-vledger.entries.data()),e.id,e.generation);
  J3DModel* model=nullptr;J3DMatPacket* eye=nullptr;J3DTexture* original=nullptr;
  if(!eye_contract(a,model,eye,original,why))return false;
  if(!o.bound){
   if(idx==0&&e.eye_idx==0)return true;                          // nothing of ours on the packet, nothing to restore
   bool built=false;const int s=blue_eye_set_for(original,why,built);   // normally prepared at spawn: a cache hit (one FNV of the atlas per generation)
   if(s<0)return false;
   o.bound=true;o.model=model;o.set=s;
  }
  if(model!=o.model){why="MODEL_CHANGED";return false;}
  auto& set=blue_eye_sets[o.set];
  J3DTexture* expected=e.eye_idx==0?original:set.texture[e.eye_idx];
  if(!expected||eye->mpTexture!=expected){why="EYE_PACKET_TEXTURE_NOT_OURS";return false;}
  J3DTexture* target=idx==0?original:set.texture[idx];
  if(!target){why="EYE_VARIANT_MISSING";return false;}
  if(target==expected)return true;
  eye->setTexture(target);
  if(eye->mpTexture!=target){eye->setTexture(expected);why="EYE_WRITE_READBACK_MISMATCH";return false;}
  ++perf_win.eye_swaps;
  return true;
 }
 // CP14 Red: the blue_eye rule with the gold set (contract re-checked on every eye step; set bound once per actor generation).
 bool gold_eye(devf_vis::Entry& e,unsigned idx){
  const char*& why=last;
  auto& o=devf_eyes12::owner_for(eye_owners,static_cast<std::size_t>(&e-vledger.entries.data()),e.id,e.generation);
  J3DModel* model=nullptr;J3DMatPacket* eye=nullptr;J3DTexture* original=nullptr;
  if(!eye_contract(a,model,eye,original,why))return false;
  if(!o.bound){
   if(idx==0&&e.eye_idx==0)return true;
   bool built=false;const int s=gold_eye_set_for(original,why,built);
   if(s<0)return false;
   o.bound=true;o.model=model;o.set=s;
  }
  if(model!=o.model){why="MODEL_CHANGED";return false;}
  auto& set=gold_eye_sets[o.set];
  J3DTexture* expected=e.eye_idx==0?original:set.texture[e.eye_idx];
  if(!expected||eye->mpTexture!=expected){why="EYE_PACKET_TEXTURE_NOT_OURS";return false;}
  J3DTexture* target=idx==0?original:set.texture[idx];
  if(!target){why="EYE_VARIANT_MISSING";return false;}
  if(target==expected)return true;
  eye->setTexture(target);
  if(eye->mpTexture!=target){eye->setTexture(expected);why="EYE_WRITE_READBACK_MISMATCH";return false;}
  ++perf_win.eye_swaps;
  return true;
 }
};

// ---- CP14 Red state: side table indexed like the CP10.6 ledger, stamped with id+generation (a new generation never inherits anything) ----
std::array<devf_red::State,devf_dyn::kCapacity> red_states{};
std::uint32_t red_window_seq=0;bool red_window_seen=false;   // last sim_seq at which some Red's window was open after its POST (cheap gate for the ally scan)
std::size_t ledger_slot(const devf_dyn::Entry* d)noexcept{return static_cast<std::size_t>(d-ledger.entries.data());}
bool red_window_open(const devf_dyn::Entry* d)noexcept{
 if(!d||d->variant!=devf_variant::Variant::Red)return false;
 const auto* s=devf_red::state_if(red_states,ledger_slot(d),d->id,d->generation);return s&&devf_red::window_open(*s);
}
void visual_drop(std::uint32_t id,const char* why)noexcept{
 auto* v=devf_perf::find_used(vledger,id);if(!v)return;
 if(v->aura_live)slog("EMITTER_RELEASED",v,nullptr,why,0,nullptr,0,devf_stk::resource_for(v->variant));   // actor gone or replaced: the key is only forgotten, the engine removes the unrefreshed emitter
 if(v->aura_live||v->eye_idx>0){devf_vis::Event x;x.reason=why;x.eye_idx=v->eye_idx;xlog("VISUAL_RELEASED",v,nullptr,"BOTH",x);}   // no model, packet or texture is touched
 vledger.release(*v);
}
void visual_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry* d)noexcept{
 const bool red_window=red_window_open(d);   // CP14: the Red's own group-regeneration window (red_policy.hpp), computed in this POST before this call
 const bool aura=(d->variant==devf_variant::Variant::Blue&&d->group)||(d->variant==devf_variant::Variant::Black&&d->fury>0)||red_window;
 const bool eyes=d->variant==devf_variant::Variant::Black&&d->fury>0;
 const bool blue_eyes=d->variant==devf_variant::Variant::Blue&&d->group;   // CP12: the Blue eyes follow exactly the Blue aura condition
 const bool gold_eyes=red_window;                                         // CP14: the Red gold eyes follow exactly the Red aura condition
 auto* v=devf_perf::find_used(vledger,id);
 if(v&&v->identity!=a){visual_drop(id,"ADDRESS_CHANGED");v=nullptr;}
 if(!v){
  if(!aura&&!eyes&&!blue_eyes&&!gold_eyes)return;
  v=vledger.record(id,a,d->generation,d->variant);
  if(!v){slog("EMITTER_REJECTED",nullptr,e,"VISUAL_LEDGER_FULL",0,nullptr,0,devf_stk::resource_for(d->variant));return;}
 }
 const bool was_target=v->aura_target;
 RealEngine en;en.a=a;
 devf_stk::Retry& rt=devf_stk::retry_for(stk_retries,static_cast<std::size_t>(v-vledger.entries.data()),v->id,v->generation);
 devf_stk::AuraPart<RealEngine> part(en,rt);
 part.fx=a->current.pos.x;part.fy=a->current.pos.y;part.fz=a->current.pos.z;part.eye_dy=a->eyePos.y-a->current.pos.y;   // feet + MEASURED eye offset; the policy places the ring height at 0.65 of it and follows the actor every tick
 // FIX02: XZ centre on the torso / head collision spheres set by daE_OC_c::cc_set during this execute (read-only); the policy falls back to the feet.
 part.torso_ok=a->mSphs_cc[0].GetR()>1.0f&&a->mSphs_cc[1].GetR()>1.0f;
 part.c0x=a->mSphs_cc[0].GetCX();part.c0z=a->mSphs_cc[0].GetCZ();part.c1x=a->mSphs_cc[1].GetCX();part.c1z=a->mSphs_cc[1].GetCZ();
 GameFx fx;fx.a=a;fx.part=&part;
 const auto out=devf_vis::step(vledger,*v,fx,aura,eyes);   // aura (both) + Black eyes: unchanged CP10.6A machine
 const bool red_route=v->variant==devf_variant::Variant::Red;   // CP14: the Red gold eyes use the same step_eyes channel as the Blue eyes
 const devf_vis::Events beo=devf_eyes12::eyes_via_step_eyes(v->variant)||red_route?devf_eyes12::step_eyes(*v,fx,red_route?gold_eyes:blue_eyes):devf_vis::Events{};   // CP12 Blue eyes / CP14 Red gold eyes
 const std::uint16_t res=devf_stk::resource_for(v->variant);
 if(was_target&&!aura&&v->aura_live)slog("ACTOR_STATE_OFF",v,e,"LOGIC_STATE_OFF_FADE_OUT_BEGINS",0,nullptr,0,res);
 for(unsigned i=0;i<out.n;++i){
  const auto& x=out.ev[i];
  if(x.kind==devf_vis::Ev::AuraOn)slog("ACTOR_STATE_ON",v,e,x.reason,rt.fails,nullptr,x.alpha,res);   // logic state, NOT an emitter and NOT pixels
 }
 const bool detail=log_budget.detail(v->variant);bool acquired=false;   // CP11: decided once for this batch, before any line of it is written
 for(unsigned i=0;i<part.notes.count;++i){
  const auto& n=part.notes.n[i];
  using F=devf_perf::LineFilter;
  switch(n.kind){
   case devf_stk::Note::Requested:if(F::requested(detail))slog("REQUESTED",v,e,n.why,0,nullptr,0,n.resource);break;
   case devf_stk::Note::RetryStart:slog("EMITTER_REJECTED",v,e,n.why,n.fails,nullptr,0,n.resource);break;                  // transient: retrying for up to kRetryTicks
   case devf_stk::Note::Waiting:slog("EMITTER_PENDING",v,e,n.why,n.fails,nullptr,0,n.resource);break;                       // CP12: waiting for the actor's first draw (unbudgeted)
   case devf_stk::Note::Acquired:slog("EMITTER_ACQUIRED",v,e,n.why,n.fails,nullptr,0,n.resource);acquired=true;++perf_win.created;break;
   case devf_stk::Note::ColorApplied:if(F::colour(detail,true))slog("COLOR_APPLIED",v,e,n.why,0,&n.color,n.color.a,n.resource);break;
   case devf_stk::Note::ColorUnavailable:slog("COLOR_UNAVAILABLE",v,e,n.why,0,&n.color,n.color.a,n.resource);break;   // anomaly: always, in full
   case devf_stk::Note::TuningApplied:if(F::tuning(detail,n.color.geo_ok,n.color.presence_ok))tlog(v,e,n);break;
   case devf_stk::Note::DrawBasisCheck:if(F::basis(detail,n.color.basis.drawable))blog(v,e,n);break;
   default:break;
  }
 }
 if(acquired&&log_budget.consume(v->variant)&&svc_log&&svc_log->write){   // once per variant per session: say that later acquisitions are logged compactly
  char text[400];text[0]=0;
  std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=TELEMETRY_BUDGET owner=dev.bokoblin.expansion version=%s variant=%s detail_acquisitions=%u later=COMPACT_STATE_LINES anomalies=ALWAYS_FULL route=SHARED_BODY_HUGGING_STROKES",
   kVersion,devf_variant::variant_text(v->variant),devf_perf::kDetailAcquisitions);
  try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
 }
 for(unsigned i=0;i<out.n;++i){
  const auto& x=out.ev[i];
  switch(x.kind){
   case devf_vis::Ev::AuraOff:slog("EMITTER_RELEASED",v,e,x.reason,0,nullptr,0,res);break;
   case devf_vis::Ev::AuraRejected:slog("EMITTER_REJECTED",v,e,x.reason,rt.fails,nullptr,0,res);break;                 // permanent for this generation (retries exhausted or capacity)
   case devf_vis::Ev::EyesOn:xlog("BLACK_EYES_ON",v,e,"EYES",x);break;
   case devf_vis::Ev::EyesOff:xlog("BLACK_EYES_OFF",v,e,"EYES",x);break;
   case devf_vis::Ev::EyesRejected:xlog("BLACK_VISUAL_REJECTED",v,e,"EYES",x);break;
   default:break;
  }
 }
 for(unsigned i=0;i<beo.n;++i){
  const auto& x=beo.ev[i];
  switch(x.kind){
   case devf_vis::Ev::EyesOn:if(log_budget.detail(v->variant))xlog(red_route?"RED_GOLD_EYES_ON":"BLUE_EYES_ON",v,e,"EYES",x);break;     // CP11 budget: state lines only while detailed
   case devf_vis::Ev::EyesOff:if(log_budget.detail(v->variant))xlog(red_route?"RED_GOLD_EYES_OFF":"BLUE_EYES_OFF",v,e,"EYES",x);break;
   case devf_vis::Ev::EyesRejected:xlog(red_route?"RED_VISUAL_REJECTED":"BLUE_VISUAL_REJECTED",v,e,"EYES",x);break;
   default:break;
  }
 }
}
// Unload: stop drawing every live aura (the engine then removes it), and put the native eye texture back on a still-live SAME Black whose eye packet is still ours.
void visual_shutdown()noexcept{
 for(auto& v:vledger.entries){
  if(!v.used)continue;
  const auto* found=fopAcM_SearchByID(v.id);
  const bool live=found&&found==v.identity&&found->state.init_state!=3&&found->profname==510;
  RealEngine en;en.a=live?static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(found)):nullptr;
  devf_stk::Retry& rt=devf_stk::retry_for(stk_retries,static_cast<std::size_t>(&v-vledger.entries.data()),v.id,v.generation);
  devf_stk::AuraPart<RealEngine> part(en,rt);
  GameFx fx;fx.a=en.a;fx.part=&part;
  const bool had_eyes=v.eye_idx>0,had_aura=v.aura_live;
  devf_vis::cleanup(v,fx,live);
  if(had_aura)slog("EMITTER_RELEASED",&v,nullptr,"SHUTDOWN_STOPPED_DRAWING_ENGINE_REMOVES_IT",0,nullptr,0,devf_stk::resource_for(v.variant));
  devf_vis::Event x;x.reason=live?(had_eyes?"SHUTDOWN_AURA_STOPPED_EYES_RESTORED_IF_STILL_OURS":"SHUTDOWN_AURA_STOPPED"):"SHUTDOWN_ACTOR_GONE_NOTHING_TOUCHED";
  xlog("VISUAL_CLEANUP",&v,nullptr,"BOTH",x);
  vledger.release(v);
 }
}
// ---- CP10.5 base attack power (unchanged semantics) + CP10.6 ledger: every CP10.4-classified generation gets ONE record. Only a pristine native
// pair is `owned` (and so ever written); the others are still real Bokoblins that may count as somebody's companion. ----
void damage_drop(std::uint32_t id,const char* why)noexcept{
 auto* old=devf_perf::find_used(ledger,id);if(!old)return;
 visual_drop(id,why);
 if(old->owned&&(old->group||old->fury))ylog("DYNAMIC_RELEASED",id,nullptr,old,nullptr,why);   // actor gone or replaced: no collider is read or written
 dlog("DAMAGE_PROFILE_RELEASED",id,old->generation,nullptr,nullptr,-1,-1,-1,-1,why);
 ledger.release(*old);
}
void damage_apply(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* entry)noexcept{
 damage_drop(id,"NEW_GENERATION_OF_SAME_ACTOR_ID_NO_STATE_INHERITED");
 SphereIo io{a};
 const auto r=devf_dyn::admit(ledger,id,a,entry->generation,entry->variant,a->mName,io);
 if(!r.plan.accepted){dlog("DAMAGE_PROFILE_REJECTED",id,entry->generation,entry,&r.plan,r.b0,r.b1,r.c0,r.c1,devf_damage::why_text(r.plan.why));return;}
 dlog("DAMAGE_PROFILE_APPLIED",id,entry->generation,entry,&r.plan,r.b0,r.b1,r.c0,r.c1,r.plan.write?"FIRST_ADMITTED_EXECUTE_ABSOLUTE_TARGET_BOTH_SPHERES":"FIRST_ADMITTED_EXECUTE_NATIVE_ALREADY_EQUALS_TARGET_NO_WRITE");
}
// ---- CP14 telemetry: only on a change (claim outcome involving a Red, heal, window start/renew/end, x2 on/off), bounded per actor and per session ----
unsigned red_logs=0;
struct RedLine {int hp_before=-1,hp_after=-1,heal=0,capped=0,timer=-1,cyl=-1,sphere=-1,life0=-1,e0_x10=-1,e1_x10=-1;unsigned remainder=0,window=0,mult=1,source=0,dmg=0,rate=0;};
void redlog(const char* ev,std::uint32_t id,std::uint32_t generation,const devf_variant::Entry* e,devf_red::State* s,const RedLine& x,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||red_logs>=400)return;
 if(s){if(s->logs>=devf_red::kLogsPerRed)return;++s->logs;}
 ++red_logs;char text[960];text[0]=0;
 std::snprintf(text,sizeof(text),"DEVF_CP14_RED event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=%s source=%s encounter_id=%s hp_before=%d hp_after=%d heal=%d remainder_milli=%u capped=%d window_ticks=%u regen_mult=%u rate_hp_s=%u source_red=%u link_dmg_status=%u link_timer=%d cylinder=%d sphere=%d link_life_before=%d link_effective_x10=(%d,%d) reason=%s policy=LIFESTEAL_25PCT_MAX_MILLI_ACC_WINDOW_120_TICKS_X2_NO_STACK gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,generation,e?devf_variant::variant_text(e->variant):"NONE",e?devf_variant::source_text(e->source):"NONE",e&&e->encounter?e->encounter:"NONE",
  x.hp_before,x.hp_after,x.heal,x.remainder,x.capped,x.window,x.mult,x.rate,x.source,x.dmg,x.timer,x.cyl,x.sphere,x.life0,x.e0_x10,x.e1_x10,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// The Red's own POST, after post_update and BEFORE the visual layer reads the window: apply confirmed heals (the only actor write of the
// lifesteal route), open/renew/close the window.
void red_step(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry* d)noexcept{
 auto& s=devf_red::state_for(red_states,ledger_slot(d),d->id,d->generation);
 const auto o=devf_red::red_post(s,a->health,e->max_hp,a->mActionMode);
 if(o.hp_after!=o.hp_before)a->health=o.hp_after;
 std::int16_t hp=o.hp_before;
 for(unsigned i=0;i<o.applied;++i){
  RedLine x;x.hp_before=hp;x.hp_after=o.heals[i].new_health;x.heal=o.heals[i].heal;x.remainder=o.heals[i].remainder_milli;x.capped=o.heals[i].capped?1:0;x.window=devf_red::kWindowTicks;x.rate=e->rate;
  redlog("RED_LIFESTEAL_APPLIED",id,d->generation,e,&s,x,o.heals[i].capped?"HEAL_25PCT_OF_MAX_CAPPED_AT_MAX":"HEAL_25PCT_OF_MAX");hp=o.heals[i].new_health;
 }
 RedLine w;w.hp_before=o.hp_before;w.hp_after=o.hp_after;w.window=s.window;w.rate=e->rate;w.mult=s.window?devf_red::kRegenMultiplier:1u;
 switch(o.window_ev){
  case devf_red::Ev::WindowStart:redlog("RED_WINDOW_START",id,d->generation,e,&s,w,o.reason);break;
  case devf_red::Ev::WindowRenew:redlog("RED_WINDOW_RENEW",id,d->generation,e,&s,w,o.reason);break;
  case devf_red::Ev::WindowEnd:redlog("RED_WINDOW_END",id,d->generation,e,&s,w,o.reason);break;
  case devf_red::Ev::PendingDiscarded:redlog("RED_LIFESTEAL_DISCARDED",id,d->generation,e,&s,w,o.reason);break;
  default:break;
 }
 if(o.discarded&&o.window_ev==devf_red::Ev::WindowEnd)redlog("RED_LIFESTEAL_DISCARDED",id,d->generation,e,&s,w,"RED_TERMINAL_HIT_NOT_HEALED");
 if(s.window){red_window_seq=sim_seq;red_window_seen=true;}
}
bool black_ai_hunting(const devf_dyn::Entry* d)noexcept;void black_ai_damage(daE_OC_c* a,std::uint32_t id,const devf_dyn::Entry* d,bool start)noexcept;   // CP18: defined in the Black block
bool cp19_permitted(unsigned index,std::uint32_t id,std::uint32_t gen)noexcept;void cp19_permit_grant(unsigned index,const devf_recruit::Entry& r)noexcept;   // CP19: defined in the CP19 block
void cp19f2_chip_post(daE_OC_c* a,const devf_black_ai::State& s,const devf_dyn::Entry& d,const devf_black_ai::Actor& f)noexcept;   // CP19 FIX02: defined in the FIX02 block
// POST of an execute pair (after the NATIVE execute, before regeneration): companion snapshot, Blue group, Black fury.
void dynamic_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,int post_health)noexcept{
 ++sim_seq;
 auto* d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a)return;
 SphereIo io{a};devf_dyn::Post in;in.post_health=post_health;in.action=a->mActionMode;in.room=fopAcM_GetRoomNo(a);
 in.x=a->current.pos.x;in.y=a->current.pos.y;in.z=a->current.pos.z;in.stage=stage_tag();in.seq=sim_seq;in.window=devf_dyn::window_for(registry.used);
 const auto ev=devf_dyn::post_update(ledger,*d,io,in,[](const devf_dyn::Entry& c){
  const auto* f=fopAcM_SearchByID(c.id);return f&&f==c.identity&&f->state.init_state!=3&&f->profname==510;});
 if(d->variant==devf_variant::Variant::Black&&(ev.kind==devf_dyn::Ev::FuryStart||ev.kind==devf_dyn::Ev::FuryRefresh))black_ai_damage(a,id,d,ev.kind==devf_dyn::Ev::FuryStart);   // CP18: read-only forwarding of CP10.6's own HP-loss event
 if(d->variant==devf_variant::Variant::Red)red_step(a,id,e,d);   // CP14: lifesteal + window, before the visual layer reads them
 visual_post(a,id,e,d);   // CP10.6A: reads the logic state just computed above (d->group / d->fury / CP14 Red window), never recomputes it
 switch(ev.kind){
  case devf_dyn::Ev::BlueOn:ylog("BLUE_GROUP_ON",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::BlueOff:ylog("BLUE_GROUP_OFF",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::FuryStart:ylog("BLACK_FURY_START",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::FuryRefresh:ylog("BLACK_FURY_REFRESH",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::FuryEnd:ylog("BLACK_FURY_END",id,e,d,&ev,ev.reason);break;
  case devf_dyn::Ev::Rejected:ylog("DYNAMIC_REJECTED",id,e,d,&ev,ev.reason);break;
  default:break;
 }
}
// Unload: give back the exact NATIVE attack power (E_OC 1, E_OC2 2) to a still-live SAME object whose spheres still hold exactly what this
// module wrote last (base or dynamic value). Gone, replaced, destroyed or third-party-modified: nothing is read or written.
void damage_shutdown()noexcept{
 for(auto& e:ledger.entries){
  if(!e.used)continue;
  const auto* found=e.owned?fopAcM_SearchByID(e.id):nullptr;
  const bool live=found&&found==e.identity&&found->state.init_state!=3&&found->profname==510;
  std::uint8_t c0=0,c1=0;daE_OC_c* actor=nullptr;
  if(live){actor=static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(found));c0=actor->mSphs_at[0].GetAtAtp();c1=actor->mSphs_at[1].GetAtAtp();}
  const auto rp=devf_dyn::restore_plan(e,live,c0,c1);
  if(rp.write&&actor){actor->mSphs_at[0].SetAtAtp(e.native);actor->mSphs_at[1].SetAtAtp(e.native);}
  devf_damage::Plan shown;shown.native=e.native;shown.target=e.applied;shown.write=rp.write;
  dlog(rp.write?"DAMAGE_PROFILE_RESTORED":"DAMAGE_PROFILE_RELEASED",e.id,e.generation,nullptr,&shown,c0,c1,rp.write?e.native:c0,rp.write?e.native:c1,rp.why);
  ledger.release(e);
 }
}
// ---- CP16 Blue recruiter: only own execute writes; the new tables contain IDs/generations/value snapshots, no actor pointers. ----
struct AiState {
 std::uint32_t id=0,generation=0;devf_blue::Tracker lvl;unsigned events=0;
 devf_blue::PreSnap snap;bool canary_logged=false,rate_decided=false,rate_applied=false;float rate=1.0f;
};
std::array<AiState,devf_dyn::kCapacity> ai_states{};unsigned ai_logs=0;
AiState& ai_for(const devf_dyn::Entry* d)noexcept{return devf_blue::stamp(ai_states,static_cast<std::size_t>(d-ledger.entries.data()),d->id,d->generation);}
struct AiLine {unsigned lf=0,lt=0,companions=0,ids[2]{};int mode=-1,state=-1;float frame=-1,rt=1.0f,ra=1.0f;unsigned lock0=0,lock1=0;};
void ailog(const char* ev,const devf_variant::Entry* e,AiState& s,const AiLine& x,const char* reason)noexcept{
 if(!svc_log||!svc_log->write||ai_logs>=400||s.events>=32)return;++s.events;++ai_logs;char text[1000];text[0]=0;
 std::snprintf(text,sizeof(text),"DEVF_CP16_BLUE_AI event=%s owner=dev.bokoblin.expansion version=%s actor_id=%u generation=%u variant=BLUE encounter_id=%s level=%u->%u companions=%u companion_ids=(%u,%u) native_mode=%d native_state=%d rate_target=%.2f rate_applied=%.2f frame=%.3f lockout=%u->%u reason=%s policy=GROUP_1_2_3_NATIVE_SOLO_ATTACK_RATE_1_15_1_30 gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,s.id,s.generation,e&&e->encounter?e->encounter:"NONE",x.lf,x.lt,x.companions,x.ids[0],x.ids[1],x.mode,x.state,double(x.rt),double(x.ra),double(x.frame),x.lock0,x.lock1,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
bool ai_sane(daE_OC_c* a)noexcept{
 return a->mpMorf&&a->mActionMode>=0&&a->mActionMode<devf_variant::A_COUNT&&a->mOcState>=0&&a->mOcState<16&&a->field_0x6c0>=0&&a->field_0x6c0<=400&&a->field_0x6c2>=0&&a->field_0x6c2<=400&&a->speedF>-100.0f&&a->speedF<100.0f;
}
devf_recruit::Table recruits;
struct RecruitLease {
 std::uint32_t id=0,gen=0;bool watch=false,cancel=false,deferred=false,shout_target=false;
 devf_recruit::Point before{},written{};devf_recruit::TimerLease wait_timer{};
};
std::array<RecruitLease,devf_recruit::kCapacity> recruit_leases{};
unsigned recruit_log_count=0,recruit_sweep=0;devf_recruit::Diagnostics recruit_diagnostics;
void recruit_log(const char* event,std::uint32_t caller,std::uint32_t gen,const devf_recruit::Frame& r,const char* reason)noexcept{
 if(!svc_log||!svc_log->write)return;
 const bool call_event=std::strcmp(event,"BLUE_CALL_ISSUED")==0;
 const bool detail=std::strcmp(event,"RECRUIT_CANDIDATE_SKIPPED")==0||std::strcmp(event,"RECRUIT_SKIPPED_NATIVE_LEASH")==0;
 if(call_event){if(!recruit_diagnostics.take(devf_recruit::Diag::Call))return;}
 else if(detail){if(!recruit_diagnostics.take(devf_recruit::Diag::Detail))return;}
 else{if(recruit_log_count>=1200)return;++recruit_log_count;}
 const auto* exact=recruits.find(caller,gen);const auto* observed=exact?exact:recruits.find_id(caller);const auto* recipient=recruits.find(r.id,r.gen);
 const bool distance_known=observed&&devf_recruit::finite(observed->f.pos)&&devf_recruit::finite(r.pos);
 const bool home_known=observed&&devf_recruit::finite(observed->f.pos)&&devf_recruit::finite(r.home);
 const double dist=distance_known?devf_dyn::sqrt_d(devf_recruit::xz2(observed->f.pos,r.pos)):-1.;
 const double dy=distance_known?devf_recruit::abs_d(double(observed->f.pos.y)-r.pos.y):-1.;
 const double home=home_known?devf_dyn::sqrt_d(devf_recruit::xz2(r.home,observed->f.pos)):-1.;
 const auto* order=recipient&&recipient->order.active?&recipient->order:nullptr;
 const char* verified=!order||!order->identity_checked?"NOT_CHECKED":order->identity_verified?"LAST_CHECK_PASS":"LAST_CHECK_FAIL";
 char text[2200];std::snprintf(text,sizeof(text),"DEVF_CP16_RECRUIT event=%s version=%s blue_id=%u blue_gen=%u recipient_id=%u recipient_gen=%u variant=%s stage=%s room=%d layer=%d layer_origin=ACTIVE_GET_LAYER_NO_0 native_mode=%d native_state=%d recipient_hp=%d recipient_hit=%u recipient_ready=%u recipient_unsafe_flags=%u recipient_leash_out=%u distance_xz=%.1f delta_y=%.1f distance_known=%u own_native_leash=%.1f home_to_blue=%.1f home_distance_known=%u recipient_tick=%u caller_snapshot_present=%u caller_gen_observed=%u caller_stage=%s caller_room=%d caller_layer=%d caller_mode=%d caller_state=%d caller_hp=%d caller_hit=%u caller_ready=%u caller_unsafe_flags=%u caller_leash_out=%u caller_snapshot_age=%u snapshot_window=%u identity=%s identity_check_age=%u grace_active=%u grace_age=%u order_age=%u reason=%s evidence=EVENT_NOT_GAMEPLAY_OBSERVATION tracked_scope=CP10_4_LEDGER_ONLY",
 event,kVersion,caller,gen,r.id,r.gen,devf_variant::variant_text(r.variant),r.stage,r.room,r.layer,r.mode,r.state,r.health,unsigned(r.hit),unsigned(r.ready),unsigned(r.unsafe_flags),unsigned(r.leash_out),dist,dy,unsigned(distance_known),double(r.range),home,unsigned(home_known),recipient?recipient->ticks:0,unsigned(observed!=nullptr),observed?observed->f.gen:0,observed?observed->f.stage:"UNKNOWN",observed?observed->f.room:-1,observed?observed->f.layer:-1,observed?observed->f.mode:-1,observed?observed->f.state:-1,observed?observed->f.health:-1,observed?unsigned(observed->f.hit):0,observed?unsigned(observed->f.ready):0,observed?unsigned(observed->f.unsafe_flags):0,observed?unsigned(observed->f.leash_out):0,observed?unsigned(sim_seq-observed->f.seq):0,devf_dyn::window_for(registry.used),verified,order&&order->identity_checked?recipient->ticks-order->verified_tick:0,order?unsigned(order->caller_transient):0,order&&order->caller_transient?recipient->ticks-order->caller_transient_start:0,order&&order->consumed?recipient->ticks-order->started:0,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void recruit_summary(const devf_recruit::Entry& r,unsigned level,const devf_recruit::Call& c)noexcept{
 if(!c.issued||!svc_log||!svc_log->write||!recruit_diagnostics.take(devf_recruit::Diag::Summary))return;
 const auto n=[&](devf_recruit::Skip reason){return c.skipped[static_cast<unsigned>(reason)];};
 char text[1600];std::snprintf(text,sizeof(text),"DEVF_CP16_RECRUIT event=RECRUIT_CALL_SUMMARY version=%s blue_id=%u blue_gen=%u level=%u stage=%s room=%d layer=%d radius_xz=%.1f height_limit=%.1f origin=%s examined_tracked=%u offered=%u self_slots=%u empty_slots=%u invalid_snapshot=%u not_classified=%u dead_terminal_special=%u stale_snapshot=%u different_stage=%u different_room=%u different_layer=%u out_of_radius=%u height=%u already_fighting=%u not_idle=%u hit_unsafe=%u native_leash=%u busy_existing_order=%u retry_backoff=%u distance_samples_fresh_context=%u distance_min_xz=%.1f distance_max_xz=%.1f distance_state=%s selected_candidate_detail_attempts=%u diagnostic_suppressed_total=%u scope=CP10_4_LEDGER_ONLY untracked_actors=UNKNOWN evidence=NOT_GLOBAL_CENSUS_NOT_CURRENT_LOS_FOR_RECALL",
 kVersion,r.f.id,r.f.gen,level,r.f.stage,r.f.room,r.f.layer,devf_recruit::kRadius,devf_recruit::kHeight,c.origin,c.examined_tracked,c.offered,c.self_slots,c.empty_slots,n(devf_recruit::Skip::InvalidSnapshot),n(devf_recruit::Skip::NotClassified),n(devf_recruit::Skip::DeadOrTerminal),n(devf_recruit::Skip::Stale),n(devf_recruit::Skip::DifferentStage),n(devf_recruit::Skip::DifferentRoom),n(devf_recruit::Skip::DifferentLayer),n(devf_recruit::Skip::OutOfRadius),n(devf_recruit::Skip::HeightLimit),n(devf_recruit::Skip::AlreadyFighting),n(devf_recruit::Skip::NotIdle),n(devf_recruit::Skip::Unsafe),n(devf_recruit::Skip::NativeLeash),n(devf_recruit::Skip::Busy),n(devf_recruit::Skip::Backoff),c.distance_samples,c.min_distance,c.max_distance,c.distance_samples?"MEASURED_FROM_SNAPSHOTS":"UNKNOWN",c.detail_count,recruit_diagnostics.suppressed);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// This is the same exact-classified identity already admitted by CP10.4. A stale ID/address or generation never admits a command.
bool recruit_verify(const devf_recruit::Frame& f)noexcept{
 const auto* d=devf_perf::find_used(ledger,f.id);const auto* a=fopAcM_SearchByID(f.id);
 return a&&d&&d->identity==a&&d->generation==f.gen&&d->variant==f.variant&&a->state.init_state!=3&&a->profname==510;
}
devf_recruit::Frame recruit_frame(daE_OC_c* a,const devf_dyn::Entry& d)noexcept{
 devf_recruit::Frame f;f.id=d.id;f.gen=d.generation;f.seq=sim_seq;f.variant=d.variant;f.room=a->current.roomNo;f.layer=dComIfG_play_c::getLayerNo(0);
 const char* stage=dComIfGp_getStartStageName();if(stage&&std::strlen(stage)<=8)std::memcpy(f.stage,stage,std::strlen(stage)+1);
 f.pos={a->current.pos.x,a->current.pos.y,a->current.pos.z};f.home={a->home.pos.x,a->home.pos.y,a->home.pos.z};f.range=a->mMoveRange;
 f.mode=a->mActionMode;f.state=a->mOcState;f.health=a->health;f.leash_out=a->field_0x6e3!=0;
 // Base cCcD hit-bit getter is already used by CP14. No collision flags are changed here. Native damage_check() retains priority.
 f.hit=a->mSphs_cc[0].cCcD_ObjHitInf::ChkTgHit()!=0||a->mSphs_cc[1].cCcD_ObjHitInf::ChkTgHit()!=0;
 f.unsafe_flags=static_cast<std::uint16_t>((ai_sane(a)?0:devf_recruit::BadCanary)|(a->mHide?devf_recruit::Hidden:0)|(a->state.init_state==3?devf_recruit::Deleting:0)|(dComIfGp_event_runCheck()?devf_recruit::Event:0)|(dComIfGp_isEnableNextStage()?devf_recruit::Transition:0)|(a->mAcch.ChkGroundHit()?0:devf_recruit::NoGround));
 f.ready=f.unsafe_flags==0;
 return f;
}
inline bool same_watch(daE_OC_c* a,devf_recruit::Point p)noexcept{return a->mWatchPos.x==p.x&&a->mWatchPos.y==p.y&&a->mWatchPos.z==p.z;}
void recruit_release_watch(daE_OC_c* a,RecruitLease& l,bool safe)noexcept{
 if(!l.watch)return;
 // Stop only our still-owned SoundWatch, on THIS actor and in a safe PRE; never cancel damage/guard/event or another actor's target.
 if(l.shout_target&&a->mActionMode==devf_recruit::Watch)return;
 if(a->mActionMode==devf_recruit::Sound){
  if(!l.cancel||!safe||!same_watch(a,l.written))return;
  a->setActionMode(devf_recruit::Wait,0);
 }
 if(same_watch(a,l.written)){a->mWatchPos.x=l.before.x;a->mWatchPos.y=l.before.y;a->mWatchPos.z=l.before.z;}
 l.watch=false;l.cancel=false;l.shout_target=false;
}
void blue_ai_pre(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 auto* d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a||d->generation!=e->generation||d->variant==devf_variant::Variant::None){
  if(auto* old=recruits.find(id,e->generation))devf_recruit::abort(*old,recruit_log,"RECEIVER_VERIFIED_IDENTITY_FAILED");return;
 }
 const auto index=static_cast<unsigned>(d-ledger.entries.data());auto& r=devf_recruit::publish_checked(recruits,index,recruit_frame(a,*d),recruit_log);auto& l=recruit_leases[index];
 if(l.id!=id||l.gen!=d->generation){l={};l.id=id;l.gen=d->generation;}
 l.wait_timer={};
 if(r.order.active&&black_ai_hunting(d))devf_recruit::abort(r,recruit_log,"BLACK_HUNT_LOCKED_SAME_ROOM_PRIORITY_OVER_RECRUIT_ORDER");   // CP18: a provoked Black is not a callable companion
 const bool safe=devf_recruit::usable(r.f)&&!r.f.hit&&(!r.f.leash_out||r.order.active||cp19_permitted(index,id,d->generation));   // CP19: a called receiver may stand beyond its own leash
 if(l.watch&&!l.shout_target&&r.order.active&&!same_watch(a,l.written))devf_recruit::abort(r,recruit_log,"WATCH_TARGET_CHANGED_BY_NATIVE_OR_OTHER_OWNER");
 if(l.watch&&!r.order.active)l.cancel=true;
 recruit_release_watch(a,l,safe);
 if(!safe){if(r.order.active)devf_recruit::abort(r,recruit_log,!devf_recruit::domain(r.f)?"RECEIVER_INVALID_SNAPSHOT_CONTEXT":r.f.health<=0||devf_variant::blocks_regen(r.f.mode)?"RECEIVER_DEAD_OR_TERMINAL_SPECIAL":!r.f.ready?devf_recruit::unsafe_reason(r.f,false):r.f.hit?"RECEIVER_HIT_PRIORITY":r.f.leash_out?"RECEIVER_NATIVE_LEASH_RETURN":"RECEIVER_NOT_CLASSIFIED");return;}
 if(devf_recruit::order_valid(recruits,r,sim_seq,devf_dyn::window_for(registry.used),recruit_verify,recruit_log)){
  if(!r.order.consumed){
   l.before={a->mWatchPos.x,a->mWatchPos.y,a->mWatchPos.z};l.watch=true;l.written=r.order.goal;
   a->mWatchPos.x=l.written.x;a->mWatchPos.y=l.written.y;a->mWatchPos.z=l.written.z;
   a->setActionMode(devf_recruit::Sound,0);
   if(a->mActionMode==devf_recruit::Sound&&a->mOcState==0){devf_recruit::consumed(r,recruit_log);cp19_permit_grant(index,r);}   // CP19: the consumed call grants the leash permission
   else devf_recruit::abort(r,recruit_log,"NATIVE_SET_ACTION_MODE_READBACK_FAILED");
  }else if(a->mActionMode==devf_recruit::Sound&&same_watch(a,l.written)){
   l.written=r.order.goal;a->mWatchPos.x=l.written.x;a->mWatchPos.y=l.written.y;a->mWatchPos.z=l.written.z;
   // The native 60-tick running timer is extended only while this bounded, progressing command owns SoundWatch2. No speed/physics/home/range write.
   if(a->mOcState==2&&a->field_0x6c0<2)a->field_0x6c0=2;
  }
 }
 if(r.f.variant!=devf_variant::Variant::Blue)return;
 AiState& s=ai_for(d);s.snap={};
 // Native direct perception only (not searchPlayer2's companion relay/save-switch side effects). Never call during terminal or guarded modes.
 const bool detect_mode=devf_recruit::idle(a->mActionMode)||a->mActionMode==devf_recruit::Find||a->mActionMode==devf_recruit::FindStay||a->mActionMode==devf_recruit::Watch;
 daPy_py_c* lp=dComIfGp_getLinkPlayer();const bool link=lp&&fpcM_GetName(lp)==fpcNm_ALINK_e;
 const bool detected=detect_mode&&link&&a->searchPlayer();
 const auto called=devf_recruit::call(recruits,r,detected,sim_seq,devf_dyn::window_for(registry.used),recruit_log,link);
 recruit_summary(r,s.lvl.level,called);
 bool wait=devf_recruit::waiting(recruits,r,sim_seq,devf_dyn::window_for(registry.used),recruit_log);
 auto& combat=r.combat;
 // WATCH3 is the real source path to BCK7+VOICE_CRY; no fabricated dead companion or direct sound/animation API call.
 const bool gesture_safe=!r.order.active&&(devf_recruit::idle(a->mActionMode)||a->mActionMode==devf_recruit::Find||a->mActionMode==devf_recruit::FindStay)&&a->mpMorf&&link&&devf_recruit::finite(devf_recruit::Point{lp->current.pos.x,lp->current.pos.y,lp->current.pos.z});
 if(combat.shout_pending&&!combat.shout_used&&gesture_safe){
  l.before={a->mWatchPos.x,a->mWatchPos.y,a->mWatchPos.z};l.written={lp->current.pos.x,lp->current.pos.y,lp->current.pos.z};l.watch=true;l.shout_target=true;
  a->mWatchPos.x=l.written.x;a->mWatchPos.y=l.written.y;a->mWatchPos.z=l.written.z;
  a->setActionMode(devf_recruit::Watch,3);devf_recruit::shout_requested(r);l.deferred=false;
  recruit_log("BLUE_SHOUT_REQUESTED_NATIVE_WATCH3",id,r.f.gen,r.f,"DISPATCH_NOT_DELAYED_NATIVE_3_TO_4_BCK7_CRY_READBACK_IN_POST");
 }else if(combat.shout_pending&&!l.deferred){l.deferred=true;recruit_log("BLUE_SHOUT_DEFERRED",id,r.f.gen,r.f,"NATIVE_ATTACK_GUARD_SPECIAL_OR_INCOMING_ORDER_PRIORITY");}
 if(const unsigned left=devf_recruit::shout_left_watch(r,a->mActionMode)){if(left==1){recruit_log("BLUE_SHOUT_DEFERRED",id,r.f.gen,r.f,"NATIVE_PRIORITY_PREEMPTED_BEFORE_BCK7_RETRY_ONLY_WHEN_SAFE");}else recruit_log("BLUE_SHOUT_FINISHED",id,r.f.gen,r.f,"NATIVE_MODE_LEFT_WATCH_DAMAGE_AND_ATTACK_NOT_REWOUND");}
 if(combat.shout_live&&r.ticks-combat.shout_start>=120&&a->mActionMode==devf_recruit::Watch&&a->mOcState==4&&a->checkBck(7)){
  a->setActionMode(devf_recruit::Find,2);combat.shout_live=false;recruit_log("BLUE_SHOUT_FINISHED",id,r.f.gen,r.f,"120_TICK_VISUAL_DEADLINE_NATIVE_FIND_2_FALLBACK");
 }
 // FIND_STAY has an independent offensive branch; a safe native WATCH0 alert prevents it while we await a real pending ally. MOVE_OUT/guard/attack are never overridden.
 if(wait&&a->mActionMode==devf_recruit::FindStay)a->setActionMode(devf_recruit::Watch,0);
 l.wait_timer=devf_recruit::gate(wait,a->mActionMode,a->mOcState,a->field_0x6c2);
 s.snap=devf_blue::make_snap(s.lvl.level==1,link,0,a->mActionMode,a->mOcState,a->mOldActionMode,a->field_0x6c0,a->field_0x6c2);
}
void recruit_post(daE_OC_c* a,std::uint32_t id)noexcept{
 auto* d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a)return;const auto i=static_cast<unsigned>(d-ledger.entries.data());auto* r=recruits.find(id,d->generation);if(!r)return;
 r->f=recruit_frame(a,*d);auto& l=recruit_leases[i];
 devf_recruit::restore(l.wait_timer,a->mActionMode,a->mOcState,a->field_0x6c2);l.wait_timer={};
 if(devf_recruit::shout_observed(*r,a->mActionMode,a->mOcState,a->checkBck(7))){
 recruit_log("BLUE_SHOUT_STARTED",id,r->f.gen,r->f,"NATIVE_WATCH_3_TO_4_BCK7_OBSERVED_VOICE_PATH_SOURCE_VERIFIED_NOT_AUDIO_OBSERVED");
 }
 devf_recruit::after(recruits,*r,recruit_log);
 if(l.watch&&(!r->order.active||a->mActionMode!=devf_recruit::Sound)){l.cancel=true;recruit_release_watch(a,l,false);}
}
// Four bounded stale-slot checks per management pass; no actor census and no writes outside own execute for recruitment.
void recruit_poll()noexcept{
 recruit_diagnostics.advance();
 for(unsigned n=0;n<4;++n){const unsigned i=recruit_sweep++%devf_recruit::kCapacity;auto& r=recruits.entries[i];
  if(!r.f.id)continue;const auto* d=devf_perf::find_used(ledger,r.f.id);
  if(!d||d->generation!=r.f.gen){devf_recruit::abort(r,recruit_log,d?"RECEIVER_LEDGER_GENERATION_CHANGED":"RECEIVER_LEDGER_RELEASED");r={};recruit_leases[i]={};}
 }
}
void blue_ai_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 auto* d=devf_perf::find_used(ledger,id);
 if(!d||d->identity!=a||d->variant!=devf_variant::Variant::Blue||!devf_blue::applies_to(e->variant))return;
 AiState& s=ai_for(d);
 if(!ai_sane(a))return;
 // 1. group level
 const auto g=devf_blue::scan_group(ledger,*d,sim_seq,devf_dyn::window_for(registry.used),s.lvl.companions,[](const devf_dyn::Entry& c){
  const auto* f=fopAcM_SearchByID(c.id);return f&&f==c.identity&&f->state.init_state!=3&&f->profname==510;});
 const auto ch=devf_blue::track(s.lvl,g);
 if(ch.changed){
  AiLine x;x.lf=ch.from;x.lt=ch.to;x.companions=s.lvl.companions;x.ids[0]=s.lvl.ids[0];x.ids[1]=s.lvl.ids[1];x.mode=a->mActionMode;x.state=a->mOcState;x.rt=devf_blue::rate_for(ch.to);x.lock1=devf_blue::lockout_for(ch.to);x.lock0=devf_blue::lockout_for(ch.from);
  ailog("BLUE_LEVEL",e,s,x,ch.to>ch.from?(ch.from==1?"COMPANION_ENTERED":"GROUP_GREW"):(ch.to==1?"LAST_COMPANION_GONE_NATIVE_SOLO":"GROUP_SHRANK"));
 }
 // 3. attack animation rate: decided once, at the POST of the execute that started the blow
 if(!devf_blue::in_blow(a->mActionMode,a->mOcState)){s.rate_decided=false;s.rate_applied=false;}
 else if(!s.rate_decided&&s.lvl.level>=2){
  devf_blue::AtkView v;v.mode=a->mActionMode;v.state=a->mOcState;v.frame=a->mpMorf->getFrame();v.play_speed=a->mpMorf->getPlaySpeed();
  v.anim_b=v.state==devf_blue::kAttackB&&a->checkBck(5);v.anim_c=v.state==devf_blue::kAttackC&&a->checkBck(6);
  const auto act=devf_blue::decide_rate(v,s.lvl.level,s.rate_decided);
  if(act!=devf_blue::RateAction::None){
   s.rate_decided=true;
   AiLine x;x.lf=x.lt=s.lvl.level;x.companions=s.lvl.companions;x.ids[0]=s.lvl.ids[0];x.ids[1]=s.lvl.ids[1];x.mode=a->mActionMode;x.state=a->mOcState;x.frame=v.frame;x.rt=devf_blue::rate_for(s.lvl.level);x.ra=v.play_speed;
   if(act==devf_blue::RateAction::Apply){
    a->mpMorf->setPlaySpeed(x.rt);x.ra=a->mpMorf->getPlaySpeed();
    if(x.ra==x.rt){s.rate_applied=true;s.rate=x.rt;ailog("ATTACK_RATE_APPLIED",e,s,x,v.state==devf_blue::kAttackB?"ATTACK_B_BCK5_FROM_NATIVE_1_0":"ATTACK_C_BCK6_FROM_NATIVE_1_0");}
    else{a->mpMorf->setPlaySpeed(1.0f);ailog("ATTACK_RATE_READBACK_MISMATCH_RESTORED_NATIVE",e,s,x,"WRITE_NOT_OBSERVED");}
   }else ailog("ATTACK_RATE_SKIPPED",e,s,x,act==devf_blue::RateAction::SkipForeignSpeed?"RATE_NOT_NATIVE_1_0_SOMEBODY_ELSE_OWNS_IT":"ATTACK_ALREADY_RUNNING_NOT_FRESH");
  }
 }
 // 4. post-attack lockout (executeFind state 0: field_0x6c2 = 20 after an attack)
 if(const int target=devf_blue::lockout_write(s.snap,a->mActionMode,a->field_0x6c2,s.lvl.level);target>=0){
  {
   a->field_0x6c2=static_cast<s16>(target);
   AiLine x;x.lf=x.lt=s.lvl.level;x.companions=s.lvl.companions;x.ids[0]=s.lvl.ids[0];x.ids[1]=s.lvl.ids[1];x.mode=a->mActionMode;x.state=a->mOcState;x.lock0=devf_blue::kNativeLockout;x.lock1=static_cast<unsigned>(target);x.rt=devf_blue::rate_for(s.lvl.level);
   ailog("ATTACK_LOCKOUT_SCALED",e,s,x,"NATIVE_20_TICKS_DIVIDED_BY_THE_ATTACK_RATE");
  }
 }
}
// Unload: give back the native rate 1.0 to a blow in progress whose rate is still exactly the one this module wrote. Nothing else needs restoring.
void blue_ai_shutdown()noexcept{
 for(auto& s:ai_states){
  if(!s.id||!s.rate_applied)continue;
  const auto* f=fopAcM_SearchByID(s.id);auto* d=devf_perf::find_used(ledger,s.id);
  if(f&&d&&f==d->identity&&d->generation==s.generation&&f->state.init_state!=3&&f->profname==510){
   auto* a=static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(f));
   if(a->mpMorf&&a->mActionMode==devf_blue::kModeAttack&&a->mpMorf->getPlaySpeed()==s.rate)a->mpMorf->setPlaySpeed(1.0f);
  }
  s=AiState{};
 }
}
void recruit_shutdown()noexcept{
 for(auto& l:recruit_leases){
  if(!l.id||!l.watch)continue;const auto* d=devf_perf::find_used(ledger,l.id);const auto* f=fopAcM_SearchByID(l.id);
  if(d&&f&&d->identity==f&&d->generation==l.gen&&f->state.init_state!=3&&f->profname==510){
   auto* a=static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(f));
   // Hot unload is unsupported: never change modes here. Restore only a no-longer-consumed target, or leave a native SoundWatch target intact until its native deadline.
   if(a->mActionMode!=devf_recruit::Sound&&same_watch(a,l.written)){a->mWatchPos.x=l.before.x;a->mWatchPos.y=l.before.y;a->mWatchPos.z=l.before.z;}
  }
 }
 recruits={};recruit_leases={};
}
// ---- CP17 FIX01 Red x2 / jump interception: own B/C rate, bounded forced native ATTACK, FIND rush speed, per-tick AT sphere placement. ----
// Writes (all on the executing Red, all bounded and logged): setPlaySpeed on the owned fresh B/C (x1.15 at frame 1, x2 at frame 2) and its restore;
// setActionMode(ATTACK,0) + speedF 0 only from FIND state 4 / ATTACK state 0 for an interception or a reaction in range; speedF in FIND state 4 during a
// reaction (exact x1.5 of the native chase result) restored to the native value in POST; mSphs_at[0/1] centre/radius for one collision pass of an owned
// x2 blow (cc_set rewrites them every execute). Nothing is written to Link, HP, AtHit, frames, timers, home or position.
std::array<devf_red_ai::State,devf_red_ai::kCapacity> red_ai_states{};
devf_red_ai::Budget red_ai_budget;unsigned red_ai_cursor=0,red_ai_passes=0,red_ai_summaries=0;
struct RedAiCause{unsigned blows=0,x2_readbacks=0,normal_readbacks=0,contacts_link=0,contacts_other=0,shield=0,damage=0,interrupt_observed=0,interrupt_not=0,interrupt_unknown=0,sweep_ticks=0;};
struct RedAiCounts{
 RedAiCause by[unsigned(devf_red_ai::Cause::Count)];
 unsigned skipped_rates=0,readback_failed=0,charges=0,charge_eligible=0,charge_deferred=0,selections=0,upgraded=0,cancelled=0,jump_attack=0,jump_strike=0,intercept_started=0,intercept_waiting=0,missed_time=0,missed_other=0,intercept_skipped=0,
  forced=0,forced_preempted=0,rushes=0,rush_measured=0,rush_preempted_ticks=0,rush_released=0,canary_failed=0,samples=0,damage_unattributed=0;double rush_moved=0,rush_native=0;
}red_ai_counts;
inline bool red_ai_is(const char* a,const char* b)noexcept{return std::strcmp(a,b)==0;}
void red_ai_summary(const char*why)noexcept{
 if(!svc_log||!svc_log->write||red_ai_summaries>=40)return;++red_ai_summaries;const auto& c=red_ai_counts;
 char text[1400];std::snprintf(text,sizeof(text),"DEVF_CP17_RED_AI event=SESSION_COUNTERS version=%s link_samples_logged=%u charge_episodes=%u charge_reactions_selected=%u charge_deferred=%u reactions_selected=%u charge_to_jump_upgrades=%u intent_cancellations=%u jump_attack_detected=%u jump_strike_detected=%u intercept_started=%u intercept_waiting=%u intercept_missed_not_enough_time=%u intercept_missed_other=%u intercept_skipped=%u native_attack_forced=%u forced_preempted_by_native=%u rushes=%u rush_measured=%u rush_preempted_ticks=%u rush_released=%u rush_moved_xz=%.0f rush_native_equivalent=%.0f skipped_rates=%u readback_failed=%u damage_confirmed_unattributed=%u independent_canary_failures=%u detail_lines=%u detail_suppressed=%u critical_lines=%u critical_suppressed=%u reason=%s scope=TRACKED_CLASSIFIED_RED_ONLY evidence=COUNTERS_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  kVersion,c.samples,c.charges,c.charge_eligible,c.charge_deferred,c.selections,c.upgraded,c.cancelled,c.jump_attack,c.jump_strike,c.intercept_started,c.intercept_waiting,c.missed_time,c.missed_other,c.intercept_skipped,c.forced,c.forced_preempted,c.rushes,c.rush_measured,c.rush_preempted_ticks,c.rush_released,c.rush_moved,c.rush_native,c.skipped_rates,c.readback_failed,c.damage_unattributed,c.canary_failed,red_ai_budget.global,red_ai_budget.suppressed,red_ai_budget.crit,red_ai_budget.crit_suppressed,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
 for(unsigned i=0;i<unsigned(devf_red_ai::Cause::Count);++i){
  const auto& b=c.by[i];if(!b.blows&&!b.damage)continue;
  std::snprintf(text,sizeof(text),"DEVF_CP17_RED_AI event=CAUSE_SUMMARY version=%s cause=%s native_blows=%u rate_x2_readbacks=%u rate_x115_readbacks=%u contact_link=%u contact_other=%u shield_block_native=%u damage_confirmed_cp14=%u interruption_observed=%u interruption_not_observed=%u interruption_unknown=%u sweep_ticks=%u reason=%s evidence=COUNTERS_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
   kVersion,devf_red_ai::cause_text(devf_red_ai::Cause(i)),b.blows,b.x2_readbacks,b.normal_readbacks,b.contacts_link,b.contacts_other,b.shield,b.damage,b.interrupt_observed,b.interrupt_not,b.interrupt_unknown,b.sweep_ticks,why);
  try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
 }
}
void red_ai_count(const devf_red_ai::State& s,const char* e,devf_red_ai::Cause cause,const char* why)noexcept{
 auto& c=red_ai_counts;auto& b=c.by[unsigned(cause)<unsigned(devf_red_ai::Cause::Count)?unsigned(cause):0];
 if(red_ai_is(e,"NATIVE_ATTACK_STARTED"))++b.blows;
 else if(red_ai_is(e,"NATIVE_ATTACK_ENDED"))b.sweep_ticks+=s.sweeps;
 else if(red_ai_is(e,"RATE_APPLIED_READBACK")){if(s.own_rate==devf_red_ai::kBurst)++b.x2_readbacks;else ++b.normal_readbacks;}
 else if(red_ai_is(e,"RATE_SKIPPED"))++c.skipped_rates;
 else if(red_ai_is(e,"RATE_READBACK_FAILED"))++c.readback_failed;
 else if(red_ai_is(e,"NATIVE_CONTACT_TARGET_IDENTIFIED")){if(red_ai_is(why,"TARGET_LINK_TG_CYLINDER_BY_POINTER_IDENTITY"))++b.contacts_link;else ++b.contacts_other;}
 else if(red_ai_is(e,"SHIELD_BLOCK_NATIVE_OBSERVED"))++b.shield;
 else if(red_ai_is(e,"DAMAGE_CONFIRMED")){if(red_ai_is(why,"CP14_ATTRIBUTION_LINK_LIFE_DROP_FROM_THIS_RED_SPHERE"))++b.damage;else ++c.damage_unattributed;}
 else if(red_ai_is(e,"INTERRUPTION_OBSERVED"))++b.interrupt_observed;
 else if(red_ai_is(e,"INTERRUPTION_NOT_OBSERVED"))++b.interrupt_not;
 else if(red_ai_is(e,"INTERRUPTION_UNKNOWN"))++b.interrupt_unknown;
 else if(red_ai_is(e,"ACTUAL_CHARGE_DETECTED"))++c.charges;
 else if(red_ai_is(e,"CHARGE_REACTION_DEFERRED"))++c.charge_deferred;
 else if(red_ai_is(e,"REACTION_SELECTED")){++c.selections;if(cause==devf_red_ai::Cause::ChargeSpin||cause==devf_red_ai::Cause::ChargeJump)++c.charge_eligible;}
 else if(red_ai_is(e,"REACTION_UPGRADED"))++c.upgraded;
 else if(red_ai_is(e,"SPECIAL_CANCELLED"))++c.cancelled;
 else if(red_ai_is(e,"JUMP_ATTACK_DETECTED")){if(cause==devf_red_ai::Cause::InterceptJumpStrike)++c.jump_strike;else ++c.jump_attack;}
 else if(red_ai_is(e,"INTERCEPT_STARTED"))++c.intercept_started;
 else if(red_ai_is(e,"INTERCEPT_WAITING"))++c.intercept_waiting;
 else if(red_ai_is(e,"INTERCEPT_MISSED_NOT_ENOUGH_TIME"))++c.missed_time;
 else if(red_ai_is(e,"INTERCEPT_MISSED"))++c.missed_other;
 else if(red_ai_is(e,"INTERCEPT_SKIPPED"))++c.intercept_skipped;
 else if(red_ai_is(e,"NATIVE_ATTACK_FORCED"))++c.forced;
 else if(red_ai_is(e,"NATIVE_ATTACK_FORCED_PREEMPTED"))++c.forced_preempted;
 else if(red_ai_is(e,"RUSH_STARTED"))++c.rushes;
 else if(red_ai_is(e,"RUSH_MEASURED")){++c.rush_measured;c.rush_moved+=s.rush_moved;c.rush_native+=s.rush_native;}
 else if(red_ai_is(e,"RUSH_TICK_PREEMPTED_BY_NATIVE"))++c.rush_preempted_ticks;
 else if(red_ai_is(e,"RUSH_RELEASED"))++c.rush_released;
 else if(red_ai_is(e,"LINK_REACTIVE_SAMPLE_OBSERVED"))++c.samples;
 else if(red_ai_is(e,"INDEPENDENT_LINK_STATE_CANARY_FAILED_REACTIVE_DISABLED"))++c.canary_failed;
}
const char* red_ai_jump_text(devf_red_ai::Jump j)noexcept{return j==devf_red_ai::Jump::JumpAttack?"JUMP_ATTACK_PROC26":j==devf_red_ai::Jump::JumpStrike?"JUMP_STRIKE_PROC30":"NONE";}
const char* red_ai_target_text(devf_red_ai::Target t)noexcept{return t==devf_red_ai::Target::ThisRed?"THIS_RED":t==devf_red_ai::Target::None?"NONE":t==devf_red_ai::Target::Other?"OTHER_ACTOR":"UNKNOWN";}
void red_ai_log(devf_red_ai::State& s,const devf_red_ai::Actor& a,const devf_red_ai::Link& l,const char* event,devf_red_ai::Cause cause,const char* why)noexcept{
 red_ai_count(s,event,cause,why);
 if(!svc_log||!svc_log->write||!red_ai_budget.take(s,devf_red_ai::critical(event,cause)))return;
 const double ratio=s.rush_native>0?s.rush_moved/s.rush_native:0;
 char text[1700];std::snprintf(text,sizeof(text),"DEVF_CP17_RED_AI event=%s version=%s actor_id=%u generation=%u variant=RED room=%d layer=%d stage=%s own_tick=%u native_mode=%d native_state=%d bck=%d frame=%.3f clip_end=%.3f play_mode=%d rate=%.3f owned_rate=%.2f speedF=%.2f lockout=%d cause=%s blow_seq=%u link_sample=%s link_present=%u link_id=%u link_room=%d link_layer=%d link_proc=%d link_basic=%u link_canary=%u link_safe=%u link_los=%u link_jump=%s link_target=%s link_feet_above_red=%.1f link_grounded=%u passive_ticks=%u random_ready_tick=%u reactive_ready_tick=%u intent_age=%u jump_block=%s rush_ticks=%u rush_preempted=%u rush_moved_xz=%.1f rush_native_equivalent=%.1f rush_ratio=%.3f sweep_ticks=%u sweep_native_kept=%u reason=%s fast_rate=X2 normal_rate=X1_15 damage=UNCHANGED evidence=POLICY_LINE_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
 event,kVersion,a.id,a.gen,a.room,a.layer,a.stage,s.ticks,a.mode,a.state,a.b?5:a.c?6:0,double(a.frame),double(a.end),a.playmode,double(a.rate),double(s.own_rate),double(a.speed),a.lockout,devf_red_ai::cause_text(cause),s.attack_seq,l.sampled?"READ_ONLY_CURRENT_PRE":"NOT_SAMPLED_IN_THIS_CALL",unsigned(l.present),l.id,l.room,l.layer,l.proc,unsigned(l.basic),unsigned(l.canary),unsigned(l.safe),unsigned(l.los),red_ai_jump_text(l.jump),red_ai_target_text(l.target),l.sampled&&l.present?double(l.pos.y)-a.pos.y:0.0,unsigned(l.grounded),s.passive,s.next_random,s.next_reactive,s.intent?s.ticks-s.intent_start:0,s.jump_block,s.rush_ticks,s.rush_preempted,s.rush_moved,s.rush_native,ratio,s.sweeps,s.sweep_kept,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
devf_red_ai::Actor red_ai_frame(daE_OC_c* a,const devf_dyn::Entry& d,bool colliders)noexcept{
 devf_red_ai::Actor f;const auto r=recruit_frame(a,d);f.id=r.id;f.gen=r.gen;f.variant=r.variant;std::memcpy(f.stage,r.stage,9);f.room=r.room;f.layer=r.layer;f.mode=r.mode;f.state=r.state;f.health=r.health;f.pos={r.pos.x,r.pos.y,r.pos.z};f.home={r.home.x,r.home.y,r.home.z};f.range=r.range;f.ready=r.ready;f.hit=r.hit;f.leash_out=r.leash_out;f.wall=a->mAcch.ChkWallHit()!=0;
 f.speed=a->speedF;f.lockout=a->field_0x6c2;f.angle=a->shape_angle.y;f.cutdown=a->checkCutDownHitFlg()!=0;
 const auto slot=static_cast<std::size_t>(&d-ledger.entries.data());if(slot<recruits.entries.size()){const auto& incoming=recruits.entries[slot];f.incoming=incoming.f.id==f.id&&incoming.f.gen==f.gen&&incoming.order.active;}
 if(a->mpMorf){f.b=a->checkBck(5);f.c=a->checkBck(6);f.frame=a->mpMorf->getFrame();f.rate=a->mpMorf->getPlaySpeed();f.end=a->mpMorf->getEndFrame();f.playmode=a->mpMorf->getPlayMode();f.stopped=a->mpMorf->isStop();}
 if(colliders){
  // After cc_set: native centres/radii of this execute, AT bit, and the hit flags of the previous collision pass (read only).
  const cXyz& t=a->mSphs_at[0].GetC();const cXyz& h=a->mSphs_at[1].GetC();f.tip={t.x,t.y,t.z};f.hilt={h.x,h.y,h.z};f.tip_r=a->mSphs_at[0].GetR();f.hilt_r=a->mSphs_at[1].GetR();
  f.at_set=a->mSphs_at[0].ChkAtSet()&&a->mSphs_at[1].ChkAtSet();
  const bool h0=a->mSphs_at[0].cCcD_ObjHitInf::ChkAtHit()!=0,h1=a->mSphs_at[1].cCcD_ObjHitInf::ChkAtHit()!=0;f.at_hit=h0||h1;
  if(f.at_hit){
   // Target identity by pointer equality with Link's three tg cylinders (addresses only; nothing is dereferenced through the hit object).
   auto* p=dComIfGp_getLinkPlayer();
   if(p&&fpcM_GetName(p)==fpcNm_ALINK_e){auto* k=static_cast<daAlink_c*>(p);
    for(int i=0;i<3;++i){const void* cyl=static_cast<cCcD_Obj*>(&k->mTgCyls[i]);if((h0&&static_cast<const void*>(a->mSphs_at[0].cCcD_ObjHitInf::GetAtHitObj())==cyl)||(h1&&static_cast<const void*>(a->mSphs_at[1].cCcD_ObjHitInf::GetAtHitObj())==cyl))f.hit_link=true;}}
  }
 }
 return f;
}
// Independent CP17 Link reader (read only). Const daPy virtual calls use the SDK's Windows ABI and never the non-const mutating overload.
// PROC29/2F/2A charges and PROC26 (checkCutJumpMode) / PROC30 jumps are cross-checked; any contradiction is a canary failure.
devf_red_ai::Link red_ai_link(daE_OC_c* a,devf_red_ai::Actor& f,devf_red_ai::State& s)noexcept{
 devf_red_ai::Link l;l.sampled=true;auto* p=dComIfGp_getLinkPlayer();
 if(!p||fpcM_GetName(p)!=fpcNm_ALINK_e||p->state.init_state==3){s.link_vel_valid=false;return l;}
 const auto id=fopAcM_GetID(p);if(!id||fopAcM_SearchByID(id)!=p){s.link_vel_valid=false;return l;}
 l.present=true;l.id=id;l.room=p->current.roomNo;l.layer=f.layer;std::memcpy(l.stage,f.stage,9);l.pos={p->current.pos.x,p->current.pos.y,p->current.pos.z};
 l.basic=devf_red_ai::finite(l.pos)&&l.room>=0&&l.room<64&&dComIfGs_getLife()>0;
 // Horizontal velocity is the measured displacement between two samples one management pass apart; vertical speed/gravity/ground are Link's own fields.
 const bool consecutive=s.link_vel_valid&&s.link_id==id&&s.link_pass+1==red_ai_passes;
 l.vel={consecutive?l.pos.x-s.link_pos.x:0.f,p->speed.y,consecutive?l.pos.z-s.link_pos.z:0.f};l.vel_valid=consecutive;l.gravity=p->gravity;l.heading=p->shape_angle.y;
 s.link_pos=l.pos;s.link_pass=red_ai_passes;s.link_vel_valid=true;
 if(!l.basic||!devf_red_ai::context(f,l))return l;
 const auto* k=static_cast<const daAlink_c*>(p);const auto* v=static_cast<const daPy_py_c*>(p);
 l.proc=k->mProcID;l.timer=v->getDamageWaitTimer();l.ground_y=k->mLinkAcch.GetGroundH();l.grounded=k->mLinkAcch.ChkGroundHit();
 l.target=k->mTargetedActor==static_cast<const fopAc_ac_c*>(a)?devf_red_ai::Target::ThisRed:k->mTargetedActor==nullptr?devf_red_ai::Target::None:devf_red_ai::Target::Other;
 const bool any=v->checkCutCharge()!=0,jump_mode=v->checkCutJumpMode()!=0;
 const auto q=devf_red_ai::charge_view(unsigned(l.proc),k->mProcVar2.field_0x300c,any,v->checkCutTurnCharge()!=0,v->checkCutLargeJumpCharge()!=0,unsigned(daAlink_c::PROC_MAX));
 l.canary=q.valid&&jump_mode==(unsigned(l.proc)==devf_red_ai::kProcCutJump)&&l.timer>=0&&l.timer<=150&&devf_red_ai::finite(l.gravity)&&devf_red_ai::finite(l.ground_y);
 l.charge=l.canary?q.family:devf_red_ai::Charge::None;l.jump=l.canary?devf_red_ai::jump_view(unsigned(l.proc),jump_mode,any):devf_red_ai::Jump::None;
 l.safe=l.timer==0&&!v->checkWolf()&&!v->checkRide()&&!k->checkPlayerDemoMode()&&!dComIfGp_event_runCheck()&&!dComIfGp_isEnableNextStage();
 if(!l.safe)l.jump=devf_red_ai::Jump::None;   // wolf/mount/demo/event/invulnerable: never an interception
 using M=devf_red_ai::Motion;
 if(l.canary){
  if(l.proc==daAlink_c::PROC_DAMAGE||l.proc==daAlink_c::PROC_LARGE_DAMAGE_UP||l.proc==daAlink_c::PROC_LAND_DAMAGE)l.motion=M::Damage;
  else if(l.proc==daAlink_c::PROC_FRONT_ROLL||l.proc==daAlink_c::PROC_SIDE_ROLL||l.proc==daAlink_c::PROC_SIDESTEP||l.proc==daAlink_c::PROC_BACK_JUMP)l.motion=M::Dodge;
  else if(l.proc==daAlink_c::PROC_GUARD_ATTACK||(l.proc>=daAlink_c::PROC_CUT_NORMAL&&l.proc<=daAlink_c::PROC_CUT_LARGE_JUMP_LAND))l.motion=M::Offensive;
  else if(l.proc==daAlink_c::PROC_WAIT||l.proc==daAlink_c::PROC_ATN_ACTOR_WAIT||l.proc==daAlink_c::PROC_WAIT_TURN||l.proc==daAlink_c::PROC_GUARD_SLIP)l.motion=v->checkPlayerGuard()?M::Guard:M::Quiet;
 }
 if(l.motion==M::Damage||l.motion==M::Dodge)l.safe=false;
 if(devf_red_ai::safe(f)&&devf_red_ai::fight(f.mode)&&!f.incoming){f.perceived=a->searchPlayer();l.los=f.perceived&&fopAcM_otherBgCheck(a,p)==0;}
 return l;
}
devf_red_ai::State* red_ai_slot(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry*& d)noexcept{
 if(e->variant!=devf_variant::Variant::Red)return nullptr;
 d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a||d->generation!=e->generation||d->variant!=devf_variant::Variant::Red)return nullptr;
 return &red_ai_states[static_cast<unsigned>(d-ledger.entries.data())];
}
void red_ai_pre(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 devf_dyn::Entry* d=nullptr;if(!red_ai_slot(a,id,e,d))return;
 const auto slot=static_cast<unsigned>(d-ledger.entries.data());auto f=red_ai_frame(a,*d,false);auto& s=devf_red_ai::state_for(red_ai_states,slot,f,sim_seq);const auto l=red_ai_link(a,f,s);
 auto emit=[&](const char*event,devf_red_ai::Cause cause,const char*why){red_ai_log(s,f,l,event,cause,why);};
 if(devf_red_ai::sample_transition(s,l,devf_red_ai::context(f,l)))emit("LINK_REACTIVE_SAMPLE_OBSERVED",devf_red_ai::Cause::Normal,"TRANSITION_OF_LINK_ID_PRESENCE_CONTEXT_OR_CANARY_ONLY");
 const auto cmd=devf_red_ai::pre(s,f,l,emit);
 // Bounded native-mode start: the same call executeFind makes; damage/water/fall/watch checks in action() still run after it and keep priority.
 if(cmd.force)a->setActionMode(devf_variant::A_ATTACK,0);
 if(cmd.zero_speed)a->speedF=0.0f;
 if(cmd.rush)a->speedF=cmd.speed;
 if(cmd.close){a->speedF=cmd.speed;if(cmd.close_lockout)a->field_0x6c2=2;}   // CP19 final approach: native chase toward Link, attack switch held by a 2-tick lease
}
void red_ai_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 devf_dyn::Entry* d=nullptr;auto* sp=red_ai_slot(a,id,e,d);if(!sp)return;auto& s=*sp;
 const auto f=red_ai_frame(a,*d,true);if(s.id!=id||s.gen!=e->generation||s.room!=f.room||s.layer!=f.layer||std::memcmp(s.stage,f.stage,9)!=0)return;
 devf_red_ai::Link unknown; // POST never repeats Link virtual/perception work; the sweep uses this execute's PRE Link position.
 auto emit=[&](const char*event,devf_red_ai::Cause cause,const char*why){red_ai_log(s,f,unknown,event,cause,why);};
 const auto rp=devf_red_ai::rush_post(s,f,emit);
 if(rp.write)a->speedF=rp.value;
 {const auto cp=devf_red_ai::close_after(s,f,emit);if(cp.speed)a->speedF=cp.speed_value;if(cp.lockout)a->field_0x6c2=0;}   // CP19: give back speed and lease
 const auto out=devf_red_ai::post(s,f,emit);
 if(out.write&&a->mpMorf){
  a->mpMorf->setPlaySpeed(out.value);
  if(a->mpMorf->getPlaySpeed()==out.value){emit(out.value==1?"RATE_RESTORE_READBACK":"RATE_APPLIED_READBACK",out.cause,"OWN_NATIVE_BC_RATE_READBACK_NOT_HIT_OR_SOUND_PROOF");}
  else{s.owned=false;emit("RATE_READBACK_FAILED",out.cause,"OWNER_RELEASED_NO_RETRY_OR_HIT_CLAIM");}
 }
 devf_red_ai::Link here;here.sampled=s.link_vel_valid&&s.link_pass==red_ai_passes&&s.link_id!=0;here.pos=s.link_pos;
 auto rf=f;if(a->mpMorf)rf.rate=a->mpMorf->getPlaySpeed();
 const auto sw=devf_red_ai::sweep(s,rf,here);
 if(sw.write0){a->mSphs_at[0].SetC(cXyz(sw.c0.x,sw.c0.y,sw.c0.z));a->mSphs_at[0].SetR(sw.r0);}
 if(sw.write1){a->mSphs_at[1].SetC(cXyz(sw.c1.x,sw.c1.y,sw.c1.z));a->mSphs_at[1].SetR(sw.r1);}
}
// CP14 forwards its own confirmed attribution (Link life really dropped from this Red's sphere). CP17 never infers damage itself.
void red_ai_damage_confirmed(std::uint32_t id,std::uint32_t gen)noexcept{
 const auto* d=devf_perf::find_used(ledger,id);if(!d||d->generation!=gen||d->variant!=devf_variant::Variant::Red)return;
 auto& s=red_ai_states[static_cast<unsigned>(d-ledger.entries.data())];if(s.id!=id||s.gen!=gen)return;
 devf_red_ai::Actor f;f.id=id;f.gen=gen;f.variant=devf_variant::Variant::Red;f.room=s.room;f.layer=s.layer;std::memcpy(f.stage,s.stage,9);
 devf_red_ai::Link unknown;devf_red_ai::damage_confirmed(s,[&](const char*event,devf_red_ai::Cause cause,const char*why){red_ai_log(s,f,unknown,event,cause,why);});
}
void red_ai_poll()noexcept{
 if(++red_ai_passes%900==0)red_ai_summary("BOUNDED_900_MANAGEMENT_PASSES");
 for(unsigned n=0;n<4;++n){const auto i=red_ai_cursor++%devf_red_ai::kCapacity;auto& s=red_ai_states[i];if(!s.id)continue;const auto* d=devf_perf::find_used(ledger,s.id);if(!d||d->generation!=s.gen||d->variant!=devf_variant::Variant::Red)s={};}
}
void red_ai_shutdown()noexcept{
 red_ai_summary("COLD_SHUTDOWN_COUNTERS_NOT_GAMEPLAY_PROOF");
 for(auto& s:red_ai_states){if(!s.id)continue;const auto* d=devf_perf::find_used(ledger,s.id);const auto* p=fopAcM_SearchByID(s.id);
  if(d&&p&&d->identity==p&&d->generation==s.gen&&d->variant==devf_variant::Variant::Red&&p->state.init_state!=3&&p->profname==510){
   auto* a=static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(p));const auto f=red_ai_frame(a,*d,false);
   if(devf_red_ai::restore_on_shutdown(s,f)&&a->mpMorf)a->mpMorf->setPlaySpeed(1.0f);
  }s={};
 }
}
// ---- end CP17 FIX01 Red ----
// ---- CP18 Black fury chain / same-room hunt: own B/C rate, native mode entries at clip end, bounded native timers, x2 AT sphere coverage. ----
// Writes (all on the executing Black, all bounded and logged): setPlaySpeed on the owned fresh B/C (x1.15 at frame 1, x2 at frame 2) and its restore;
// setActionMode(ATTACK,0) / setActionMode(FIND,0|2|3) only at the PRE of a complete native clip, of a native FIND state 0 during fury or of a native abandon
// mode while hunting; speedF 0 with the ATTACK state 0 entry; field_0x6c0 held at 2 in FIND state 4 while hunting; field_0x6c2 0 in FIND state 4 while fury;
// mSphs_at[0/1] centre/radius for one collision pass of an owned x2 blow (cc_set rewrites them every execute). Nothing is written to Link, HP, ATP, flags,
// frames, home, mMoveRange, field_0x6e3, position or other actors. CP10.6 keeps the fury counter, ATP and the damage detection.
std::array<devf_black_ai::State,devf_black_ai::kCapacity> black_ai_states{};
devf_black_ai::Budget black_ai_budget;devf_black_ai::Rooms black_rooms;unsigned black_ai_cursor=0,black_ai_passes=0,black_ai_summaries=0;
struct BlackAiCounts{
 unsigned blows_normal=0,blows_fury=0,x2_readbacks=0,x115_readbacks=0,chains=0,chain_deferred=0,wait_avoided=0,lockout_cleared=0,damage_recognized=0,hunt_locked=0,hunt_not_armed=0,
  reset_room=0,reset_terminal=0,reset_generation=0,reset_context=0,held=0,abandon_prevented=0,pursuit_started=0,pursuit_measured=0,blocked=0,priority_kept=0,contact_link=0,contact_other=0,
  shield=0,rate_skipped=0,readback_failed=0,ownership_released=0,room_changes_observed=0;unsigned long long fury_ticks=0,hunt_ticks=0;double moved=0,native=0;
}black_ai_counts;
void black_ai_summary(const char* why)noexcept{
 if(!svc_log||!svc_log->write||black_ai_summaries>=40)return;++black_ai_summaries;const auto& c=black_ai_counts;
 char text[1600];std::snprintf(text,sizeof(text),"DEVF_CP18_BLACK_AI event=BLACK_SUMMARY version=%s blows_x1_15=%u blows_x2_fury=%u rate_x2_readbacks=%u rate_x1_15_readbacks=%u chained_blows=%u chain_deferred=%u wait_transitions_avoided=%u fury_lockouts_cleared=%u fury_own_ticks=%llu damage_recognized_cp10_6=%u hunt_episodes=%u hunt_not_armed=%u hunt_own_ticks=%llu reset_room=%u reset_terminal=%u reset_generation_or_release=%u reset_context=%u held_transient=%u abandon_prevented=%u pursuit_started=%u pursuit_measured=%u pursuit_moved_xz=%.0f pursuit_native_speed_sum=%.0f blocked_geometry=%u native_priority_kept=%u contact_link=%u contact_other=%u shield_block_native=%u rate_skipped=%u readback_failed=%u ownership_released=%u link_room_epoch=%u detail_lines=%u detail_suppressed=%u critical_lines=%u critical_suppressed=%u chain_detail_suppressed=%u reason=%s scope=TRACKED_CLASSIFIED_BLACK_ONLY evidence=COUNTERS_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  kVersion,c.blows_normal,c.blows_fury,c.x2_readbacks,c.x115_readbacks,c.chains,c.chain_deferred,c.wait_avoided,c.lockout_cleared,c.fury_ticks,c.damage_recognized,c.hunt_locked,c.hunt_not_armed,c.hunt_ticks,c.reset_room,c.reset_terminal,c.reset_generation,c.reset_context,c.held,c.abandon_prevented,c.pursuit_started,c.pursuit_measured,c.moved,c.native,c.blocked,c.priority_kept,c.contact_link,c.contact_other,c.shield,c.rate_skipped,c.readback_failed,c.ownership_released,black_rooms.epoch,black_ai_budget.global,black_ai_budget.suppressed,black_ai_budget.crit,black_ai_budget.crit_suppressed,black_ai_budget.detail_suppressed,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void black_ai_count(const devf_black_ai::State& s,const char* e)noexcept{
 auto& c=black_ai_counts;auto is=[&](const char* n){return std::strcmp(e,n)==0;};
 if(is("BLACK_NATIVE_ATTACK_STARTED")){if(s.pending)++c.blows_fury;else ++c.blows_normal;}
 else if(is("BLACK_RATE_APPLIED_READBACK")){if(s.own_rate==devf_black_ai::kFury)++c.x2_readbacks;else ++c.x115_readbacks;}
 else if(is("BLACK_CHAIN_NEXT_ATTACK_STARTED"))++c.chains;
 else if(is("BLACK_CHAIN_DEFERRED_NATIVE_PRIORITY"))++c.chain_deferred;
 else if(is("BLACK_NATIVE_WAIT_TRANSITION_AVOIDED"))++c.wait_avoided;
 else if(is("BLACK_FURY_LOCKOUT_CLEARED"))++c.lockout_cleared;
 else if(is("BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6"))++c.damage_recognized;
 else if(is("BLACK_HUNT_LOCKED_FIRST_DAMAGE"))++c.hunt_locked;
 else if(is("BLACK_HUNT_NOT_ARMED"))++c.hunt_not_armed;
 else if(is("BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED"))++c.reset_room;
 else if(is("BLACK_HUNT_RESET_ACTOR_TERMINAL"))++c.reset_terminal;
 else if(is("BLACK_HUNT_RESET_GENERATION"))++c.reset_generation;
 else if(is("BLACK_HUNT_RESET_ACTOR_CONTEXT"))++c.reset_context;
 else if(is("BLACK_HUNT_HELD_LINK_ROOM_TRANSIENT"))++c.held;
 else if(is("BLACK_HUNT_NATIVE_ABANDON_PREVENTED"))++c.abandon_prevented;
 else if(is("BLACK_HUNT_PURSUIT_STARTED"))++c.pursuit_started;
 else if(is("BLACK_HUNT_PURSUIT_MEASURED")){++c.pursuit_measured;c.moved+=s.moved;c.native+=s.native_sum;}
 else if(is("BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY"))++c.blocked;
 else if(is("BLACK_HUNT_NATIVE_PRIORITY_KEPT"))++c.priority_kept;
 else if(is("BLACK_NATIVE_CONTACT_LINK"))++c.contact_link;
 else if(is("BLACK_NATIVE_CONTACT_OTHER"))++c.contact_other;
 else if(is("BLACK_NATIVE_SHIELD_BLOCK"))++c.shield;
 else if(is("BLACK_RATE_SKIPPED"))++c.rate_skipped;
 else if(is("BLACK_RATE_READBACK_FAILED"))++c.readback_failed;
 else if(is("BLACK_RATE_OWNERSHIP_RELEASED"))++c.ownership_released;
}
void black_ai_log(devf_black_ai::State& s,const devf_black_ai::Actor& a,const devf_black_ai::Link& l,unsigned fury,const char* event,const char* why)noexcept{
 black_ai_count(s,event);
 if(!svc_log||!svc_log->write||!black_ai_budget.take(s,event))return;
 const double dist=l.present&&devf_red_ai::finite(l.pos)&&devf_red_ai::finite(a.pos)?devf_red_ai::d3(a.pos,l.pos):-1.0;
 char text[1700];std::snprintf(text,sizeof(text),"DEVF_CP18_BLACK_AI event=%s version=%s actor_id=%u generation=%u variant=BLACK room=%d stage_tag=%08x own_tick=%u native_mode=%d native_state=%d bck=%d frame=%.3f clip_end=%.3f rate=%.3f owned_rate=%.2f speedF=%.2f lockout_6c2=%d timer_6c0=%d leash_6e3=%d fury_ticks_remaining=%u hunt=%u hunt_room=%d hunt_epoch=%u link_room_epoch=%u link_present=%u link_id=%u link_room=%d link_dist=%.1f blow_seq=%u chained_blow=%u bounce=%u pursuit_ticks=%u pursuit_moved_xz=%.1f pursuit_native_speed_sum=%.1f pursuit_dist_start=%.1f pursuit_dist_now=%.1f active_min_dist=%.1f close_in_ticks=%u reason=%s fury_rate=X2 normal_rate=X1_15 atp=CP10_6_UNCHANGED evidence=POLICY_LINE_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  event,kVersion,a.id,a.gen,a.room,a.stage,s.ticks,a.mode,a.state,a.b?5:a.c?6:a.wait_bck?0x1c:0,double(a.frame),double(a.end),double(a.rate),double(s.own_rate),double(a.speed),a.lockout,a.timer,a.leash,fury,unsigned(s.hunt),s.hunt_room,s.hunt_epoch,black_rooms.epoch,unsigned(l.present),l.id,l.room,dist,s.seq,unsigned(s.chain_blow),s.bounce,s.measure_ticks,s.moved,s.native_sum,s.dist_start,s.dist_now,s.active_min,s.close.ticks,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
devf_black_ai::Actor black_ai_frame(daE_OC_c* a,const devf_dyn::Entry& d,bool colliders)noexcept{
 devf_black_ai::Actor f;f.id=d.id;f.gen=d.generation;f.variant=d.variant;f.stage=stage_tag();f.room=a->current.roomNo;f.mode=a->mActionMode;f.state=a->mOcState;f.health=a->health;
 f.pos={a->current.pos.x,a->current.pos.y,a->current.pos.z};f.home={a->home.pos.x,a->home.pos.y,a->home.pos.z};f.range=a->mMoveRange;f.leash=a->field_0x6e3;f.timer=a->field_0x6c0;f.lockout=a->field_0x6c2;
 f.speed=a->speedF;f.angle=a->shape_angle.y;f.sane=ai_sane(a);f.cutdown=a->checkCutDownHitFlg()!=0;f.wall=a->mAcch.ChkWallHit()!=0;f.ground=a->mAcch.ChkGroundHit()!=0;
 f.hidden=a->mHide!=0;f.deleting=a->state.init_state==3;f.event=dComIfGp_event_runCheck()!=0;f.transition=dComIfGp_isEnableNextStage()!=0;
 // Base cCcD hit-bit getter, as CP16/CP17: a pending tg hit means native damage_check() owns this execute. Nothing is cleared.
 f.hit=a->mSphs_cc[0].cCcD_ObjHitInf::ChkTgHit()!=0||a->mSphs_cc[1].cCcD_ObjHitInf::ChkTgHit()!=0;
 if(a->mpMorf){f.b=a->checkBck(5);f.c=a->checkBck(6);f.frame=a->mpMorf->getFrame();f.rate=a->mpMorf->getPlaySpeed();f.end=a->mpMorf->getEndFrame();f.playmode=a->mpMorf->getPlayMode();f.stopped=a->mpMorf->isStop();}
 if(colliders){
  f.wait_bck=a->mpMorf&&a->checkBck(0x1c);
  // After cc_set: native centres/radii of this execute, AT bit, and the hit flags of the previous collision pass (read only).
  const cXyz& t=a->mSphs_at[0].GetC();const cXyz& h=a->mSphs_at[1].GetC();f.tip={t.x,t.y,t.z};f.hilt={h.x,h.y,h.z};f.tip_r=a->mSphs_at[0].GetR();f.hilt_r=a->mSphs_at[1].GetR();
  f.at_set=a->mSphs_at[0].ChkAtSet()&&a->mSphs_at[1].ChkAtSet();
  const bool h0=a->mSphs_at[0].cCcD_ObjHitInf::ChkAtHit()!=0,h1=a->mSphs_at[1].cCcD_ObjHitInf::ChkAtHit()!=0;f.at_hit=h0||h1;
  if(f.at_hit){
   // Target identity by pointer equality with Link's three tg cylinders (addresses only; nothing is dereferenced through the hit object).
   auto* p=dComIfGp_getLinkPlayer();
   if(p&&fpcM_GetName(p)==fpcNm_ALINK_e){auto* k=static_cast<daAlink_c*>(p);
    for(int i=0;i<3;++i){const void* cyl=static_cast<cCcD_Obj*>(&k->mTgCyls[i]);if((h0&&static_cast<const void*>(a->mSphs_at[0].cCcD_ObjHitInf::GetAtHitObj())==cyl)||(h1&&static_cast<const void*>(a->mSphs_at[1].cCcD_ObjHitInf::GetAtHitObj())==cyl))f.hit_link=true;}}
  }
 }
 return f;
}
// Read-only Link view: presence, the room the engine itself assigns from Link's ground polygon (daAlink_c::setRoomInfo), the stage tag and the position.
devf_black_ai::Link black_ai_link(daPy_py_c*& p)noexcept{
 devf_black_ai::Link l;p=dComIfGp_getLinkPlayer();
 if(!p||fpcM_GetName(p)!=fpcNm_ALINK_e||p->state.init_state==3){p=nullptr;return l;}
 l.present=true;l.id=fopAcM_GetID(p);l.room=p->current.roomNo;l.stage=stage_tag();l.pos={p->current.pos.x,p->current.pos.y,p->current.pos.z};
 return l;
}
devf_black_ai::State* black_ai_slot(daE_OC_c* a,std::uint32_t id,std::uint32_t gen,devf_dyn::Entry*& d)noexcept{
 d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a||d->generation!=gen||d->variant!=devf_variant::Variant::Black)return nullptr;
 return &black_ai_states[static_cast<unsigned>(d-ledger.entries.data())];
}
// The slot's previous owner (another generation or actor) loses its hunt with a line of its own; nothing is inherited.
devf_black_ai::State& black_ai_state(devf_dyn::Entry& d,const devf_black_ai::Actor& f,const devf_black_ai::Link& l)noexcept{
 auto& old=black_ai_states[static_cast<unsigned>(&d-ledger.entries.data())];
 if(devf_black_ai::stale(old,f.id,f.gen)&&old.hunt)devf_black_ai::reset(old,[&](const char* ev,const char*){black_ai_log(old,f,l,d.fury,"BLACK_HUNT_RESET_GENERATION",ev);},"BLACK_HUNT_RESET_GENERATION","SLOT_REUSED_BY_ANOTHER_ACTOR_OR_GENERATION");
 return devf_black_ai::state_for(black_ai_states,static_cast<unsigned>(&d-ledger.entries.data()),f.id,f.gen);
}
bool black_ai_hunting(const devf_dyn::Entry* d)noexcept{
 if(!d||d->variant!=devf_variant::Variant::Black)return false;const auto i=static_cast<unsigned>(d-ledger.entries.data());if(i>=devf_black_ai::kCapacity)return false;
 const auto& s=black_ai_states[i];return s.hunt&&s.id==d->id&&s.gen==d->generation;
}
void black_ai_damage(daE_OC_c* a,std::uint32_t id,const devf_dyn::Entry* cd,bool start)noexcept{
 devf_dyn::Entry* d=nullptr;if(!cd||cd->variant!=devf_variant::Variant::Black||!black_ai_slot(a,id,cd->generation,d))return;
 const auto f=black_ai_frame(a,*d,false);daPy_py_c* p=nullptr;const auto l=black_ai_link(p);auto& s=black_ai_state(*d,f,l);
 devf_black_ai::damage(s,f,l,black_rooms,start,[&](const char* ev,const char* why){black_ai_log(s,f,l,d->fury,ev,why);});
}
void black_ai_pre(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 if(e->variant!=devf_variant::Variant::Black)return;
 devf_dyn::Entry* d=nullptr;if(!black_ai_slot(a,id,e->generation,d))return;
 auto f=black_ai_frame(a,*d,false);daPy_py_c* p=nullptr;const auto l=black_ai_link(p);auto& s=black_ai_state(*d,f,l);
 // Line of sight is read only where the chain needs it: a complete clip during fury.
 if(((f.mode==devf_variant::A_ATTACK&&f.stopped)||(f.mode==devf_variant::A_FIND&&f.state==4))&&d->fury&&p)f.los=fopAcM_otherBgCheck(a,p)==0;   // CP19: also for the fury strike from FIND4
 if(d->fury)++black_ai_counts.fury_ticks;if(s.hunt)++black_ai_counts.hunt_ticks;
 const auto cmd=devf_black_ai::pre(s,f,l,black_rooms,d->fury,[&](const char* ev,const char* why){black_ai_log(s,f,l,d->fury,ev,why);});
 // Native entries only: the same calls executeFind/executeAttack make. damage_check/water/fall in action() still run after them and keep priority.
 if(cmd.set_mode)a->setActionMode(cmd.mode,cmd.state);
 if(cmd.zero_speed)a->speedF=0.0f;
 if(cmd.timer)a->field_0x6c0=cmd.timer_value;
 if(cmd.lockout)a->field_0x6c2=cmd.lockout_value;
 if(cmd.close)a->speedF=cmd.speed;   // CP19 final approach (the native lockout branch chases it to the approach speed)
}
void black_ai_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 if(e->variant!=devf_variant::Variant::Black)return;
 devf_dyn::Entry* d=nullptr;auto* sp=black_ai_slot(a,id,e->generation,d);if(!sp||devf_black_ai::stale(*sp,id,e->generation))return;auto& s=*sp;
 const auto f=black_ai_frame(a,*d,true);devf_black_ai::Link here;here.present=s.link_pos_valid;here.pos=s.link_pos;   // POST repeats no Link read: this execute's PRE sample
 auto emit=[&](const char* ev,const char* why){black_ai_log(s,f,here,d->fury,ev,why);};
 {const auto cp=devf_black_ai::close_after(s,f,emit);if(cp.speed)a->speedF=cp.speed_value;if(cp.lockout)a->field_0x6c2=0;}   // CP19: give back speed and lease
 const auto out=devf_black_ai::post(s,f,d->fury,emit);
 if(out.write&&a->mpMorf){
  a->mpMorf->setPlaySpeed(out.value);
  if(a->mpMorf->getPlaySpeed()==out.value)emit(out.value==1?"BLACK_RATE_RESTORE_READBACK":"BLACK_RATE_APPLIED_READBACK","OWN_NATIVE_BC_RATE_READBACK_NOT_HIT_OR_SOUND_PROOF");
  else{s.owned=false;emit("BLACK_RATE_READBACK_FAILED","OWNER_RELEASED_NO_RETRY_OR_HIT_CLAIM");}
 }
 auto rf=f;if(a->mpMorf)rf.rate=a->mpMorf->getPlaySpeed();
 const auto sw=devf_black_ai::sweep(s,rf);
 if(sw.write0){a->mSphs_at[0].SetC(cXyz(sw.c0.x,sw.c0.y,sw.c0.z));a->mSphs_at[0].SetR(sw.r0);}
 if(sw.write1){a->mSphs_at[1].SetC(cXyz(sw.c1.x,sw.c1.y,sw.c1.z));a->mSphs_at[1].SetR(sw.r1);}
 cp19f2_chip_post(a,s,*d,f);   // CP19 FIX02: shield chip, judged on the engine's shield-hit record of the previous collision pass
}
// One Link room observation per management pass (so A->B->A is seen even when no Black executes) and four bounded stale-slot checks.
void black_ai_poll()noexcept{
 if(++black_ai_passes==1&&svc_log&&svc_log->write){char text[400];std::snprintf(text,sizeof(text),"DEVF_CP18_BLACK_AI event=BLACK_AI_ARMED version=%s fury_rate=X2 normal_rate=X1_15 fury_ticks=%u chain_reach=%.0f chain_facing=%d hunt=SAME_ROOM_UNTIL_LINK_ROOM_OR_STAGE_CHANGE_OR_ACTOR_END evidence=POLICY_LINE_NOT_GAMEPLAY_PROOF",kVersion,devf_dyn::kFuryTicks,devf_black_ai::kChainReach,devf_black_ai::kChainFacing);try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}}
 if(black_ai_passes%900==0)black_ai_summary("BOUNDED_900_MANAGEMENT_PASSES");
 {daPy_py_c* p=nullptr;if(devf_black_ai::observe(black_rooms,black_ai_link(p)))++black_ai_counts.room_changes_observed;}
 for(unsigned n=0;n<4;++n){
  const auto i=black_ai_cursor++%devf_black_ai::kCapacity;auto& s=black_ai_states[i];if(!s.id)continue;const auto* d=devf_perf::find_used(ledger,s.id);
  devf_black_ai::Actor f;f.id=s.id;f.gen=s.gen;f.variant=devf_variant::Variant::Black;f.room=s.hunt_room;f.stage=s.hunt_stage;devf_black_ai::Link none;
  auto emit=[&](const char* ev,const char* why){black_ai_log(s,f,none,d?d->fury:0u,ev,why);};
  if(!d||d->generation!=s.gen||d->variant!=devf_variant::Variant::Black){devf_black_ai::reset(s,emit,"BLACK_HUNT_RESET_GENERATION","LEDGER_RELEASED_ACTOR_GONE_OR_NEW_GENERATION");s={};continue;}
  devf_black_ai::poll(s,black_rooms,emit);
 }
}
void black_ai_shutdown()noexcept{
 black_ai_summary("COLD_SHUTDOWN_COUNTERS_NOT_GAMEPLAY_PROOF");
 for(auto& s:black_ai_states){if(!s.id)continue;const auto* d=devf_perf::find_used(ledger,s.id);const auto* p=fopAcM_SearchByID(s.id);
  if(d&&p&&d->identity==p&&d->generation==s.gen&&d->variant==devf_variant::Variant::Black&&p->state.init_state!=3&&p->profname==510){
   auto* a=static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(p));const auto f=black_ai_frame(a,*d,false);
   if(devf_black_ai::restore_on_shutdown(s,f)&&a->mpMorf)a->mpMorf->setPlaySpeed(1.0f);   // only a still-owned running B/C; hunt and modes are never rolled back
  }s={};
 }
}
// ---- end CP18 Black ----
// ---- CP19 fixes: field SE wave keeper, attention restore, 0-HP completion, Helm Splitter exact damage, Blue call leash permission, BGM diagnostics. ----
// Writes: Z2SceneMgr::loadSeWave/eraseSeWave(0x5e) through the engine's public scene manager (owned wave only); on the executing Bokoblin only:
// attention_info.flags = BATTLE (create() value) when alive with health >= 2, setActionMode(BIG_DAMAGE,0) for a 0-HP live mode, health = H - D after a
// Helm Splitter hit, field_0x6cc = 2 for a duplicate Helm impact, setActionMode(FIND,0|1) for a permitted called ally, mSound.field_0xa3 = 0 (residual
// last-hit flag on a live actor). Link: mCutType HEAD_JUMP <-> TURN_RIGHT for the duration of exactly one Bokoblin execute, restored in POST.
struct Cp19Slot{std::uint32_t id=0,generation=0;devf_cp19::Helm helm;devf_cp19::Residual residual;unsigned attn_restores=0,logs=0;};
std::array<Cp19Slot,devf_dyn::kCapacity> cp19_slots{};std::array<devf_cp19::Permit,devf_dyn::kCapacity> cp19_permits{};
devf_cp19f2::AudioPass cp19_audio;unsigned cp19_passes=0,cp19_last_bokoblin=0,cp19_logs=0,cp19_summaries=0;bool cp19f2_need=false;   // FIX02: residency/BGM state lives in the gated AudioPass
struct Cp19Counts{unsigned wave_loads=0,wave_load_failures=0,wave_erases=0,attention_restored=0,zombies_completed=0,residual_cleared=0,helm_applied=0,helm_lethal=0,helm_duplicates=0,helm_not_processed=0,helm_foreign=0,
 permits=0,permit_ended=0,permit_find=0,permit_blocked=0,audio_ready=0;}cp19_counts;
void cp19_log(const char* ev,std::uint32_t id,std::uint32_t gen,devf_variant::Variant v,int mode,int state,int x,int y,const char* why,Cp19Slot* s=nullptr)noexcept{
 if(!svc_log||!svc_log->write||cp19_logs>=900)return;if(s){if(s->logs>=48)return;++s->logs;}++cp19_logs;
 char text[900];std::snprintf(text,sizeof(text),"DEVF_CP19 event=%s version=%s actor_id=%u generation=%u variant=%s native_mode=%d native_state=%d value_a=%d value_b=%d wave=0x%02x wave_owned=%u reason=%s evidence=POLICY_LINE_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,gen,devf_variant::variant_text(v),mode,state,x,y,unsigned(devf_cp19f2::kOcFamilyWave),unsigned(cp19_audio.res.st==devf_cp19f2::Res::Ready),why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void cp19_summary(const char* why)noexcept{
 if(!svc_log||!svc_log->write||cp19_summaries>=40)return;++cp19_summaries;const auto& c=cp19_counts;
 char text[900];std::snprintf(text,sizeof(text),"DEVF_CP19 event=CP19_SUMMARY version=%s wave_loads=%u wave_load_failures=%u wave_erases=%u wave_owned=%u attention_restored=%u zero_hp_completed=%u residual_lasthit_cleared=%u helm_applied=%u helm_native_lethal=%u helm_duplicates=%u helm_not_processed=%u helm_foreign_cut_type=%u permits=%u permit_ended=%u permit_native_find=%u permit_blocked=%u audio_gate=%s audio_not_ready_passes=%u audio_ready_transitions=%u lines=%u reason=%s evidence=COUNTERS_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  kVersion,c.wave_loads,c.wave_load_failures,c.wave_erases,unsigned(cp19_audio.res.st==devf_cp19f2::Res::Ready),c.attention_restored,c.zombies_completed,c.residual_cleared,c.helm_applied,c.helm_lethal,c.helm_duplicates,c.helm_not_processed,c.helm_foreign,c.permits,c.permit_ended,c.permit_find,c.permit_blocked,devf_cp19::gate_text(cp19_audio.gate.last),cp19_audio.gate.deferred_passes,cp19_audio.gate.ready_transitions,cp19_logs,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
Cp19Slot* cp19_slot(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry*& d)noexcept{
 if(e->variant==devf_variant::Variant::None)return nullptr;
 d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a||d->generation!=e->generation)return nullptr;
 auto& s=cp19_slots[static_cast<unsigned>(d-ledger.entries.data())];if(s.id!=id||s.generation!=e->generation){s=Cp19Slot{};s.id=id;s.generation=e->generation;}
 return &s;
}
bool cp19_permitted(unsigned index,std::uint32_t id,std::uint32_t gen)noexcept{
 if(index>=devf_dyn::kCapacity)return false;const auto& p=cp19_permits[index];return p.active&&p.id==id&&p.gen==gen;
}
// Called by blue_ai_pre the tick a receiver consumed a Blue call (native SOUND_WATCH 0 read back).
void cp19_permit_grant(unsigned index,const devf_recruit::Entry& r)noexcept{
 if(index>=devf_dyn::kCapacity)return;devf_cp19::PermitView v;v.id=r.f.id;v.gen=r.f.gen;std::memcpy(v.stage,r.f.stage,9);v.room=r.f.room;v.layer=r.f.layer;
 devf_cp19::permit_grant(cp19_permits[index],v,r.order.caller,r.order.caller_gen,sim_seq);++cp19_counts.permits;
 cp19_log("BLUE_CALL_LEASH_PERMISSION_GRANTED",r.f.id,r.f.gen,r.f.variant,r.f.mode,r.f.state,int(r.order.caller),int(r.order.caller_gen),"CONSUMED_BLUE_CALL_RECEIVER_OWN_LEASH_REPLACED_BY_NATIVE_FIND_WHILE_CALLER_LIVES_OR_FIGHT_GOES_ON");
}
bool cp19_caller_alive(const devf_cp19::Permit& p)noexcept{
 const auto* c=devf_perf::find_used(ledger,p.caller);
 return c&&c->generation==p.caller_gen&&c->variant==devf_variant::Variant::Blue&&c->snap&&c->health>0&&!devf_variant::blocks_regen(c->action);
}
void cp19_pre(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{
 devf_dyn::Entry* d=nullptr;auto* s=cp19_slot(a,id,e,d);if(!s)return;
 cp19_last_bokoblin=cp19_passes;
 const bool hidden=a->mHide!=0,deleting=a->state.init_state==3;
 // C: a native kill left in a live mode (jump-attack cancel-turn DAMAGE branch) completes through the native BIG_DAMAGE 0 death path.
 if(devf_cp19::complete_zombie(a->health,a->mActionMode,hidden,deleting)){
  const int mode=a->mActionMode,state=a->mOcState;a->setActionMode(devf_variant::A_BIG_DAMAGE,0);++cp19_counts.zombies_completed;
  cp19_log("ZERO_HP_LIVE_MODE_COMPLETED_NATIVE_DEATH_PATH",id,e->generation,e->variant,mode,state,a->health,a->mActionMode,"NATIVE_KILL_ALREADY_APPLIED_HEALTH_0_JUMP_CANCEL_DAMAGE_BRANCH_BIG_DAMAGE_0",s);return;
 }
 // F: Helm Splitter.
 devf_cp19::HelmView hv;daPy_py_c* p=dComIfGp_getLinkPlayer();
 if(p&&fpcM_GetName(p)==fpcNm_ALINK_e&&p->state.init_state!=3){hv.link=true;hv.cut=p->getCutType();hv.link_id=fopAcM_GetID(p);}
 const bool h0=a->mSphs_cc[0].cCcD_ObjHitInf::ChkTgHit()!=0,h1=a->mSphs_cc[1].cCcD_ObjHitInf::ChkTgHit()!=0;hv.pending=h0||h1;
 if(hv.pending&&hv.link&&hv.cut==devf_cp19::kCutHeadJump){
  // damage_check keeps the LAST pending sphere's collider (sphere 1 overrides sphere 0): the same one is read here.
  cCcD_Obj* o=h1?a->mSphs_cc[1].cCcD_ObjHitInf::GetTgHitObj():a->mSphs_cc[0].cCcD_ObjHitInf::GetTgHitObj();   // collider type only; nothing is dereferenced beyond the hit object's own flags
  hv.sword_hit=o&&o->ChkAtType(AT_TYPE_MASTER_SWORD|AT_TYPE_NORMAL_SWORD);
  hv.sword=daPy_py_c::checkMasterSwordEquip()?devf_cp19::Sword::Master:daPy_py_c::checkWoodSwordEquip()?devf_cp19::Sword::Wood:devf_cp19::Sword::Ordon;
 }
 hv.cutdown=a->checkCutDownHitFlg()!=0;hv.health=a->health;hv.mode=a->mActionMode;hv.invuln=a->field_0x6cc;
 const auto hc=devf_cp19::helm_pre(s->helm,hv);
 if(hc.op==devf_cp19::HelmOp::Mask){p->mCutType=devf_cp19::kCutTurnRight;cp19_log("HELM_SPLITTER_HIT_PENDING_CUT_TYPE_MASKED",id,e->generation,e->variant,a->mActionMode,a->mOcState,a->health,s->helm.damage,hc.why,s);}
 else if(hc.op==devf_cp19::HelmOp::SkipDuplicate){a->field_0x6cc=2;++cp19_counts.helm_duplicates;cp19_log("HELM_SPLITTER_DUPLICATE_IMPACT_IGNORED",id,e->generation,e->variant,a->mActionMode,a->mOcState,a->health,0,hc.why,s);}
 else if(hc.op==devf_cp19::HelmOp::NativeLethal){++cp19_counts.helm_lethal;cp19_log("HELM_SPLITTER_NATIVE_LETHAL",id,e->generation,e->variant,a->mActionMode,a->mOcState,a->health,devf_cp19::helm_damage(hv.sword),hc.why,s);}
 // D: Blue call leash permission (never on a CP18-hunting Black, never over a pending hit, an event or a scene change).
 const auto index=static_cast<unsigned>(d-ledger.entries.data());auto& pm=cp19_permits[index];
 if(pm.active){
  devf_cp19::PermitView v;v.id=id;v.gen=e->generation;const char* stage=dComIfGp_getStartStageName();if(stage&&std::strlen(stage)<=8)std::memcpy(v.stage,stage,std::strlen(stage)+1);
  v.room=a->current.roomNo;v.layer=pm.layer;   // the layer cannot change without a stage reload; getLayerNo is not re-evaluated per tick
  v.mode=a->mActionMode;v.state=a->mOcState;v.health=a->health;v.leash=a->field_0x6e3;v.stopped=a->mpMorf&&a->mpMorf->isStop();
  v.link_same_room=hv.link&&p->current.roomNo==a->current.roomNo;v.caller_alive=cp19_caller_alive(pm);v.hunting_black=black_ai_hunting(d);
  v.safe=ai_sane(a)&&!hv.pending&&!hv.cutdown&&!hidden&&!deleting&&!dComIfGp_event_runCheck()&&!dComIfGp_isEnableNextStage();
  if(v.leash==0&&v.mode!=devf_variant::A_MOVE_OUT){   // predict this tick's native leash update (action() runs after this PRE)
   devf_black_ai::Actor la;la.range=a->mMoveRange;la.leash=a->field_0x6e3;la.home={a->home.pos.x,a->home.pos.y,a->home.pos.z};la.pos={a->current.pos.x,a->current.pos.y,a->current.pos.z};v.leash=devf_black_ai::leash_next(la);
  }
  const auto pc=devf_cp19::permit_pre(pm,v,sim_seq);
  switch(pc.op){
   case devf_cp19::PermitOp::Find0:a->setActionMode(devf_variant::A_FIND,0);++cp19_counts.permit_find;cp19_log("BLUE_CALL_LEASH_PERMISSION_NATIVE_FIND",id,e->generation,e->variant,v.mode,v.state,v.leash,int(pm.caller),pc.why,s);break;
   case devf_cp19::PermitOp::Find1:if(a->searchPlayer()){a->setActionMode(devf_variant::A_FIND,1);++cp19_counts.permit_find;cp19_log("BLUE_CALL_LEASH_PERMISSION_NATIVE_FIND",id,e->generation,e->variant,v.mode,v.state,v.leash,int(pm.caller),pc.why,s);}break;
   case devf_cp19::PermitOp::Blocked:++cp19_counts.permit_blocked;cp19_log("BLUE_CALL_LEASH_PERMISSION_BLOCKED_BY_GEOMETRY",id,e->generation,e->variant,v.mode,v.state,v.leash,int(pm.caller),pc.why,s);break;
   case devf_cp19::PermitOp::Ended:++cp19_counts.permit_ended;cp19_log("BLUE_CALL_LEASH_PERMISSION_ENDED",id,e->generation,e->variant,v.mode,v.state,int(v.caller_alive),int(pm.caller),pc.why,s);break;
   default:break;
  }
 }
}
// POST, before CP10.6 reads the post-execute health: unmask Link's cut type and apply the exact Helm Splitter damage (a real HP loss for CP10.6).
void cp19_post_early(daE_OC_c* a,const devf_variant::Entry* e)noexcept{
 devf_dyn::Entry* d=nullptr;auto* s=cp19_slot(a,fopAcM_GetID(a),e,d);if(!s||!s->helm.mask)return;
 daPy_py_c* p=dComIfGp_getLinkPlayer();const bool same=p&&fpcM_GetName(p)==fpcNm_ALINK_e&&fopAcM_GetID(p)==s->helm.link_id;
 const int pre=s->helm.pre_health;const auto hp=devf_cp19::helm_post(s->helm,same,same?p->mCutType:0,a->health);
 if(hp.unmask)p->mCutType=devf_cp19::kCutHeadJump;else ++cp19_counts.helm_foreign;
 if(hp.write)a->health=static_cast<s16>(hp.health);
 if(a->health<pre){++cp19_counts.helm_applied;cp19_log("HELM_SPLITTER_EXACT_DAMAGE_APPLIED",fopAcM_GetID(a),e->generation,e->variant,a->mActionMode,a->mOcState,pre,a->health,hp.why,s);}
 else{++cp19_counts.helm_not_processed;cp19_log("HELM_SPLITTER_HIT_NOT_PROCESSED",fopAcM_GetID(a),e->generation,e->variant,a->mActionMode,a->mOcState,pre,a->health,hp.why,s);}
}
// POST, after regeneration: attention restore and the residual last-hit flag.
void cp19_post_late(daE_OC_c* a,const devf_variant::Entry* e)noexcept{
 devf_dyn::Entry* d=nullptr;auto* s=cp19_slot(a,fopAcM_GetID(a),e,d);if(!s)return;
 const bool hidden=a->mHide!=0,deleting=a->state.init_state==3;
 if(devf_cp19::restore_attention(a->attention_info.flags,a->health,a->mActionMode,hidden,deleting)){
  a->attention_info.flags=fopAc_AttnFlag_BATTLE_e;++cp19_counts.attention_restored;++s->attn_restores;
  cp19_log("ATTENTION_BATTLE_RESTORED",fopAcM_GetID(a),e->generation,e->variant,a->mActionMode,a->mOcState,a->health,int(s->attn_restores),"NATIVE_HEALTH_LE_1_CLEAR_ON_A_SURVIVOR_NOW_HEALTH_GE_2_CREATE_VALUE_BATTLE",s);
 }
 if(devf_cp19::residual_clear(s->residual,a->mSound.field_0xa3,a->health,a->mActionMode)){
  a->mSound.field_0xa3=0;++cp19_counts.residual_cleared;
  cp19_log("RESIDUAL_LASTHIT_FLAG_CLEARED",fopAcM_GetID(a),e->generation,e->variant,a->mActionMode,a->mOcState,a->health,0,"LIVE_ACTOR_KEPT_THE_KILL_SE_FLAG_30_TICKS_FIELD_BATTLE_MIX_WOULD_STAY_FROZEN",s);
 }
}
// FIX01: engine adapter of the gated audio policy. flags() reads exported engine data only (init/reset flags, audio heap, reset data); every other member
// touches audio objects and is called by the policy only after audio_gate() returned Ready in the same pass.
// FIX02: the own ARAM block (JKRAram graph heap tail block + a JASHeap root object, both in game memory, created once and never freed: the group loaded
// into it references them, also after this DLL is unloaded) and loadSeWave routed into it through JASWaveArcLoader::sAramHeap for that one synchronous
// call (JASWaveArc::load(NULL) -> getRootHeap() -> sAramHeap); sAramHeap is reset to null right after and never touched when someone else set it.
struct Cp19OwnAram{JASHeap* heap=nullptr;bool spoiled=false;std::uint32_t address=0,bytes=0,graph_free_before=0,graph_free_after=0;}cp19f2_aram;
struct Cp19AudioIo{
 devf_cp19::AudioFlags flags()const{
  devf_cp19::AudioFlags f;Z2AudioMgr* m=Z2AudioMgr::getInterface();f.manager=m!=nullptr;f.init=mDoAud_zelAudio_c::isInitFlag()!=0;f.heap=g_mDoAud_audioHeap!=nullptr;
  f.reset_flag=mDoAud_zelAudio_c::isResetFlag()!=0;f.resetting=f.manager&&f.init&&f.heap&&m->isResetting();
  f.reset_data=mDoRst::mResetData!=nullptr;f.shutdown=f.reset_data&&mDoRst::isShutdown();f.return_to_menu=f.reset_data&&mDoRst::isReturnToMenu();return f;
 }
 static Z2SceneMgr* scene(){return Z2AudioMgr::getInterface();}
 bool transition()const{return dComIfGp_isEnableNextStage()!=0;}
 int status(std::uint8_t wave)const{return scene()->getWaveLoadStatus(wave,0);}
 std::uint32_t own_used()const{JASHeap* h=cp19f2_aram.heap;return h?h->getSize()-h->getTotalFreeSize():0u;}
 std::uint32_t game_free()const{return JASKernel::getAramFreeSize();}
 bool reserve(std::uint32_t bytes,std::uint32_t keep,const char*& why)const{
  if(cp19f2_aram.heap)return true;
  if(cp19f2_aram.spoiled){why="GRAPH_ARAM_BLOCK_REJECTED_EARLIER_NO_SECOND_ALLOCATION";return false;}
  if(!JKRAram::getManager()){why="JKRARAM_MANAGER_NULL";return false;}
  JKRAramHeap* gh=JKRAram::getAramHeap();if(!gh){why="JKRARAM_GRAPH_HEAP_NULL";return false;}
  const std::uint32_t free0=gh->getFreeSize();
  if(free0<bytes||free0-bytes<keep){why="GRAPH_ARAM_LARGEST_FREE_BLOCK_BELOW_RESERVATION_PLUS_KEEP";cp19f2_aram.graph_free_before=free0;return false;}
  JKRHeap* mh=gh->getMgrHeap();if(!mh){why="GRAPH_ARAM_MANAGER_HEAP_NULL";return false;}
  void* mem=JKRHeap::alloc(sizeof(JASHeap),32,mh);if(!mem){why="NO_GAME_HEAP_MEMORY_FOR_THE_ROOT_HEAP_OBJECT";return false;}
  JKRAramBlock* blk=gh->alloc(bytes,JKRAramHeap::TAIL);
  if(!blk){JKRHeap::free(mem,mh);why="GRAPH_ARAM_TAIL_ALLOCATION_REFUSED";return false;}
  if(blk->getAddress()==0||blk->getSize()<bytes){cp19f2_aram.spoiled=true;why="GRAPH_ARAM_BLOCK_UNEXPECTED_ADDRESS_OR_SIZE_LEFT_UNUSED";return false;}   // never retried
  JASHeap* h=new(mem) JASHeap(nullptr);
  h->initRootHeap(reinterpret_cast<void*>(static_cast<std::uintptr_t>(blk->getAddress())),blk->getSize());
  cp19f2_aram.heap=h;cp19f2_aram.address=blk->getAddress();cp19f2_aram.bytes=h->getSize();cp19f2_aram.graph_free_before=free0;cp19f2_aram.graph_free_after=gh->getFreeSize();
  return true;
 }
 bool load_own(std::uint8_t wave,const char*& why)const{
  if(!cp19f2_aram.heap){why="NO_OWN_ARAM_BLOCK";return false;}
  if(JASWaveArcLoader::sAramHeap!=nullptr){why="FOREIGN_WAVE_LOADER_ROOT_HEAP_ALREADY_SET_NOT_TOUCHED";return false;}
  JASWaveArcLoader::sAramHeap=cp19f2_aram.heap;
  const bool ok=scene()->loadSeWave(wave);
  JASWaveArcLoader::sAramHeap=nullptr;
  if(!ok)why=own_used()==0?"LOADSEWAVE_FALSE_GROUP_ABSENT_ENTRY_INVALID_OR_LARGER_THAN_OWN_BLOCK":"LOADSEWAVE_FALSE_STATUS_NOT_IDLE";
  return ok;
 }
 devf_cp19::Bgm bgm()const{
  Z2AudioMgr* m=Z2AudioMgr::getInterface();Z2SeqMgr* sq=m;Z2SoundObjMgr* om=m;devf_cp19::Bgm b;b.dist=sq->mBattleDistState;b.seq=sq->mBattleSeqState;
  for(JSULink<Z2CreatureEnemy>* i=om->getEnemyList()->getFirst();i&&b.enemies<256;i=i->getNext()){Z2CreatureEnemy* en=i->getObject();if(!en)continue;++b.enemies;if(en->field_0xa3)++b.lasthit;}
  return b;
 }
};
void cp19f2_poll()noexcept;void cp19f2_shutdown()noexcept;   // FIX02 B: defined in the FIX02 block
void cp19_audio_emit(const char* ev,const char* why,int a,int b,int c,int d)noexcept{
 auto is=[&](const char* n){return std::strcmp(ev,n)==0;};
 if(is("AUDIO_LOAD_ACCEPTED"))++cp19_counts.wave_loads;else if(is("AUDIO_LOAD_REFUSED")||is("AUDIO_RESERVATION_REFUSED"))++cp19_counts.wave_load_failures;else if(is("AUDIO_READY")||is("AUDIO_READY_AGAIN"))++cp19_counts.audio_ready;   // FIX02: never an erase
 cp19_log(ev,0,0,devf_variant::Variant::None,a,b,c,d,why);
}
void cp19_poll()noexcept{
 ++cp19_passes;if(cp19_passes%900==0)cp19_summary("BOUNDED_900_MANAGEMENT_PASSES");
 Cp19AudioIo io;const bool recent=cp19_last_bokoblin&&cp19_passes-cp19_last_bokoblin<=devf_cp19::kRecentPasses;
 devf_cp19f2::audio_pass(cp19_audio,io,cp19f2_need||recent,cp19_audio_emit);   // no audio object is touched unless the gate is Ready
 cp19f2_poll();   // FIX02 B: flash schedule tick and the meter readback of the last chip
}
void cp19_shutdown()noexcept{
 cp19_summary("COLD_SHUTDOWN_COUNTERS_NOT_GAMEPLAY_PROOF");
 cp19f2_shutdown();   // FIX02 B summary (reads the residency state) and chip/flash state cleared
 devf_cp19f2::audio_shutdown(cp19_audio,cp19_audio_emit);   // FIX02: no audio call; a resident group stays in the own ARAM block (no release)
 cp19_slots={};cp19_permits={};
}
// ---- end CP19 ----
// ---- CP19 FIX02 B: Black fury shield chip + render-only red flash ----
// Reads: this Black's two AT spheres (hit bit, shield-hit bit, hit object pointer) and Link's three tg cylinders (hit bit, shield-hit bit, hit object
// pointer); pointers are compared, never followed. Writes: the meter's pending life count (dComIfGp_setItemLifeCount(-1, pending type unchanged)) once per
// confirmed blow; in daAlink_c::modelDraw PRE/POST, Link's tevStr.TevKColor r/g/b around one body/face/hat/hand model call, restored right after it.
// Never written: Link's health/meter fields directly, mDamageTimer, mDamageColorTime, procs, guard state, colliders, rupees, damage multiplier, armour.
static_assert(devf_cp19f2::kChipCapacity==devf_dyn::kCapacity,"one chip record per CP10.6 ledger slot");
std::array<devf_cp19f2::ChipBlow,devf_dyn::kCapacity> cp19f2_blows{};devf_cp19f2::Flash cp19f2_flash;bool cp19f2_chip_on=false,cp19f2_flash_hooked=false;
struct Cp19f2Counts{unsigned applied=0,duplicates=0,not_fury=0,stale=0,no_blow=0,skipped=0,readback_failed=0,flash_draws=0,flash_restores=0,meter_checks=0;}cp19f2_counts;
struct Cp19f2Meter{bool pending=false;unsigned pass=0;int life=0;std::uint32_t id=0,gen=0;unsigned seq=0;}cp19f2_meter;
struct Cp19f2FlashSave{bool active=false;daAlink_c* link=nullptr;std::uint8_t r=0,g=0,b=0;}cp19f2_flash_save;
unsigned cp19f2_logs=0,cp19f2_summaries=0;
void cp19f2_log(const char* ev,std::uint32_t id,std::uint32_t gen,unsigned seq,int a,int b,int c,int d,const char* why)noexcept{
 if(!svc_log||!svc_log->write||cp19f2_logs>=600)return;++cp19f2_logs;
 char text[760];std::snprintf(text,sizeof(text),"DEVF_CP19F2 event=%s version=%s actor_id=%u generation=%u blow_seq=%u value_a=%d value_b=%d value_c=%d value_d=%d reason=%s evidence=POLICY_LINE_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  ev,kVersion,id,gen,seq,a,b,c,d,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void cp19f2_summary(const char* why)noexcept{
 if(!svc_log||!svc_log->write||cp19f2_summaries>=40)return;++cp19f2_summaries;const auto& c=cp19f2_counts;const auto& r=cp19_audio.res;
 char text[1100];std::snprintf(text,sizeof(text),"DEVF_CP19F2 event=CP19F2_SUMMARY version=%s audio_state=%s audio_wave=0x%02x audio_attempts=%u audio_accepted=%u audio_refused=%u audio_lost=%u audio_not_owned=%u audio_group_bytes=%u own_aram_bytes=%u graph_free_before=%u graph_free_after=%u chip_applied=%u chip_duplicates=%u chip_not_fury=%u chip_stale=%u chip_no_blow=%u chip_skipped=%u chip_readback_failed=%u flash_hook=%u flash_started=%u flash_draw_writes=%u flash_restores=%u meter_checks=%u lines=%u reason=%s evidence=COUNTERS_NOT_GAMEPLAY_PROOF gameplay=HUMAN_RUNTIME_PENDING",
  kVersion,devf_cp19f2::res_text(r.st),unsigned(devf_cp19f2::kOcFamilyWave),r.attempts,r.accepted,r.refused,r.lost,r.foreign,r.used,cp19f2_aram.bytes,cp19f2_aram.graph_free_before,cp19f2_aram.graph_free_after,
  c.applied,c.duplicates,c.not_fury,c.stale,c.no_blow,c.skipped,c.readback_failed,unsigned(cp19f2_flash_hooked),cp19f2_flash.started,c.flash_draws,c.flash_restores,c.meter_checks,cp19f2_logs,why);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
// The engine's own shield confirmation, both directions of one at/tg pair of the previous collision pass (dCcS::SetAtTgGObjInf: OnTgShieldHit only when
// ChkShield accepted the hit; OnAtShieldHit alone is not enough, it is also set for a hit-mark-8 target).
bool cp19f2_shield_pair(daE_OC_c* a,daAlink_c* k,int& cyl,int& sph)noexcept{
 for(int j=0;j<2;++j){
  auto& s=a->mSphs_at[j];const bool at_hit=s.cCcD_ObjHitInf::ChkAtHit()!=0,at_shield=at_hit&&s.ChkAtShieldHit();
  const void* at_obj=at_hit?static_cast<const void*>(s.cCcD_ObjHitInf::GetAtHitObj()):nullptr;const void* sphere=static_cast<const void*>(static_cast<cCcD_Obj*>(&s));
  for(int i=0;i<3;++i){
   auto& c=k->mTgCyls[i];const bool tg_hit=c.cCcD_ObjHitInf::ChkTgHit()!=0,tg_shield=tg_hit&&c.ChkTgShieldHit();
   const void* tg_obj=tg_hit?static_cast<const void*>(c.cCcD_ObjHitInf::GetTgHitObj()):nullptr;
   if(devf_cp19f2::shield_confirmed(at_hit,at_shield,at_obj,static_cast<const void*>(static_cast<cCcD_Obj*>(&c)),tg_hit,tg_shield,tg_obj,sphere)){cyl=i;sph=j;return true;}
  }
 }
 return false;
}
void cp19f2_chip_post(daE_OC_c* a,const devf_black_ai::State& s,const devf_dyn::Entry& d,const devf_black_ai::Actor& f)noexcept{
 if(!cp19f2_chip_on)return;
 const auto index=static_cast<unsigned>(&d-ledger.entries.data());if(index>=devf_dyn::kCapacity)return;
 daPy_py_c* p=dComIfGp_getLinkPlayer();
 daAlink_c* k=p&&fpcM_GetName(p)==fpcNm_ALINK_e&&p->state.init_state!=3?static_cast<daAlink_c*>(p):nullptr;
 int cyl=-1,sph=-1;devf_cp19f2::ChipView v;v.id=f.id;v.gen=f.gen;v.seq=s.seq;v.fury=d.fury>0;v.at_set=f.at_set;v.live=f.health>0&&!f.deleting;
 v.shield=k&&cp19f2_shield_pair(a,k,cyl,sph);
 auto& b=cp19f2_blows[index];const unsigned judged=b.id==v.id&&b.gen==v.gen?b.seq:0u;   // the blow the previous pass's flags belong to
 const auto op=devf_cp19f2::chip_step(b,v);
 using devf_cp19f2::ChipOp;
 if(op==ChipOp::None)return;
 auto& c=cp19f2_counts;
 if(op!=ChipOp::Charge){
  const unsigned n=op==ChipOp::Duplicate?++c.duplicates:op==ChipOp::NotFury?++c.not_fury:op==ChipOp::NotRegistered?++c.stale:++c.no_blow;
  if(n<=40)cp19f2_log(devf_cp19f2::chip_text(op),v.id,v.gen,judged,cyl,sph,int(v.fury),int(d.fury),op==ChipOp::Duplicate?"SAME_BLOW_SECOND_SPHERE_OR_PERSISTING_FLAG_ALREADY_CHARGED":op==ChipOp::NotFury?"BLOW_STARTED_OUTSIDE_FURY_X1_15_NO_CHIP":op==ChipOp::NotRegistered?"FLAGS_OLDER_THAN_THIS_BLOW_SPHERE_REGISTRATION":"NO_BLOW_STARTED_FOR_THIS_BLACK_YET");
  return;
 }
 devf_cp19f2::ChipLink lk=devf_cp19f2::ChipLink::Ok;
 if(!k)lk=devf_cp19f2::ChipLink::NoLink;else if(dComIfGs_getLife()==0)lk=devf_cp19f2::ChipLink::LifeZero;else if(dComIfGp_event_runCheck())lk=devf_cp19f2::ChipLink::Event;
 if(lk!=devf_cp19f2::ChipLink::Ok){++c.skipped;cp19f2_log(devf_cp19f2::chip_link_text(lk),v.id,v.gen,judged,cyl,sph,0,0,"BLOW_CONSUMED_NO_LIFE_CHANGE");return;}
 const int life=dComIfGs_getLife();const float before=dComIfGp_getItemLifeCount();const u8 type=dComIfGp_getItemLifeCountType();
 dComIfGp_setItemLifeCount(-float(devf_cp19f2::kChipLifeUnits),type);   // the only life write of the mod: meter pending count, type of a pending change kept
 const float after=dComIfGp_getItemLifeCount();
 if(!devf_cp19f2::chip_readback_ok(before,after)){++c.readback_failed;cp19f2_log("SHIELD_CHIP_READBACK_MISMATCH",v.id,v.gen,judged,life,int(before*1000),int(after*1000),cyl*10+sph,"PENDING_COUNT_NOT_EXACTLY_MINUS_ONE_NO_SECOND_WRITE");return;}
 ++c.applied;devf_cp19f2::flash_start(cp19f2_flash);
 if(!cp19f2_meter.pending){cp19f2_meter.pending=true;cp19f2_meter.pass=cp19_passes;cp19f2_meter.life=life;cp19f2_meter.id=v.id;cp19f2_meter.gen=v.gen;cp19f2_meter.seq=judged;}
 cp19f2_log("SHIELD_CHIP_APPLIED",v.id,v.gen,judged,life,int(before*1000),int(after*1000),cyl*10+sph,cp19f2_flash_hooked?"FURY_BLOW_ENGINE_SHIELD_HIT_EXACTLY_ONE_LIFE_UNIT_METER_PENDING_FLASH_STARTED":"FURY_BLOW_ENGINE_SHIELD_HIT_EXACTLY_ONE_LIFE_UNIT_METER_PENDING_FLASH_HOOK_UNAVAILABLE");
}
// Once per management pass: flash schedule tick; the meter readback of the first pending chip (life before/now, pending count now) once the meter consumed it.
void cp19f2_poll()noexcept{
 devf_cp19f2::flash_tick(cp19f2_flash);
 if(cp19_passes%900==0)cp19f2_summary("BOUNDED_900_MANAGEMENT_PASSES");
 if(!cp19f2_meter.pending||cp19_passes==cp19f2_meter.pass)return;
 const float pending=dComIfGp_getItemLifeCount();const unsigned waited=cp19_passes-cp19f2_meter.pass;
 if(pending!=0.0f&&waited<30)return;
 const int now=dComIfGs_getLife();++cp19f2_counts.meter_checks;cp19f2_meter.pending=false;
 cp19f2_log("SHIELD_CHIP_METER_READBACK",cp19f2_meter.id,cp19f2_meter.gen,cp19f2_meter.seq,cp19f2_meter.life,now,int(pending*1000),int(waited),
  pending!=0.0f?"METER_HAS_NOT_CONSUMED_THE_PENDING_COUNT_WITHIN_30_PASSES":now==cp19f2_meter.life-1?"LIFE_DROPPED_EXACTLY_ONE_UNIT":now<cp19f2_meter.life?"LIFE_DROPPED_MORE_OTHER_DAMAGE_OR_CHIPS_IN_THE_SAME_WINDOW":"LIFE_NOT_LOWER_HEAL_IN_THE_SAME_WINDOW_OR_ALREADY_ZERO");
}
void cp19f2_shutdown()noexcept{
 cp19f2_summary("COLD_SHUTDOWN_COUNTERS_NOT_GAMEPLAY_PROOF");
 cp19f2_chip_on=false;cp19f2_flash=devf_cp19f2::Flash{};cp19f2_blows={};cp19f2_meter=Cp19f2Meter{};   // a POST still pending restores below; nothing else is rolled back
}
bool cp19f2_flash_model(const daAlink_c* k,const J3DModel* m)noexcept{
 // Exactly the models daAlink_c::draw draws BEFORE it zeroes TevKColor for sword/sheath/shield/items (native damage-colour scope).
 return m==k->mpLinkModel||m==k->mpLinkFaceModel||m==k->mpLinkHatModel||m==k->mpLinkHandModel||m==k->mpDemoFCBlendModel||m==k->mpDemoFCTongueModel||m==k->mpDemoHLTmpModel||m==k->mpDemoHRTmpModel;
}
HookAction on_link_model_draw(ModContext*,void* args,void*,void*)noexcept{
 cp19f2_flash_save.active=false;
 if(!cp19f2_chip_on||!args||!static_cast<void**>(args)[0]||!devf_cp19f2::flash_on(cp19f2_flash))return HOOK_CONTINUE;
 auto* lk=mods::arg<daAlink_c*>(args,0);J3DModel* m=mods::arg<J3DModel*>(args,1);
 if(!lk||!m||static_cast<daPy_py_c*>(lk)!=dComIfGp_getLinkPlayer()||!cp19f2_flash_model(lk,m))return HOOK_CONTINUE;
 GXColor& link_kcolor=lk->tevStr.TevKColor;
 cp19f2_flash_save.active=true;cp19f2_flash_save.link=lk;cp19f2_flash_save.r=link_kcolor.r;cp19f2_flash_save.g=link_kcolor.g;cp19f2_flash_save.b=link_kcolor.b;
 devf_cp19f2::Rgb native;native.r=link_kcolor.r;native.g=link_kcolor.g;native.b=link_kcolor.b;
 const auto c=devf_cp19f2::flash_merge(native,devf_cp19f2::flash_color(cp19f2_flash));
 link_kcolor.r=c.r;link_kcolor.g=c.g;link_kcolor.b=c.b;++cp19f2_counts.flash_draws;   // render-only: consumed by setLightTevColorType_MAJI of this model
 return HOOK_CONTINUE;
}
void after_link_model_draw(ModContext*,void* args,void*,void*)noexcept{
 if(!cp19f2_flash_save.active)return;cp19f2_flash_save.active=false;
 auto* lk=args&&static_cast<void**>(args)[0]?mods::arg<daAlink_c*>(args,0):nullptr;if(!lk||lk!=cp19f2_flash_save.link)return;
 GXColor& link_kcolor=lk->tevStr.TevKColor;
 link_kcolor.r=cp19f2_flash_save.r;link_kcolor.g=cp19f2_flash_save.g;link_kcolor.b=cp19f2_flash_save.b;++cp19f2_counts.flash_restores;   // native value back
}
ModResult install_flash_hooks()noexcept{
 // POST first (same rule as the execute pair): a PRE write can never exist without its restoring POST.
 const auto result=mods::hook::add_post<LinkModelDraw>(after_link_model_draw);
 if(result!=MOD_OK)return result;
 return mods::hook::add_pre<LinkModelDraw>(on_link_model_draw);
}
// ---- end CP19 FIX02 ----

// CP10.4 universal tracking + one-time classification (PRE of the actor's execute). No actor scan: only the executing actor is touched.
devf_variant::Entry* regen_begin(daE_OC_c* a,std::uint32_t id,bool initializing)noexcept{
 if(!regen_enabled)return nullptr;
 bool stale=false;devf_variant::Why refused=devf_variant::Why::None;
 devf_variant::Entry* e=registry.begin(id,a,initializing,&stale,&refused);
 if(stale){rlog("REGEN_RELEASED",nullptr,"ID_ADDRESS_REUSE_OR_NEW_NATIVE_CREATION_NO_STATE_INHERITED","NONE");damage_drop(id,"ID_ADDRESS_REUSE_OR_NEW_NATIVE_CREATION_NO_STATE_INHERITED");}
 if(!e&&refused==devf_variant::Why::RegistryFull&&!registry.full_logged){registry.full_logged=true;rlog("REGEN_REJECTED",nullptr,devf_variant::why_text(refused),"NONE");}
 return e;
}
HookAction on_execute_body(void* args)noexcept{
 if(!args||!static_cast<void**>(args)[0])return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||a->profname!=510)return HOOK_CONTINUE;
 const std::uint32_t id=fopAcM_GetID(a);
 const int native_health=a->health;   // read before this module writes anything for this actor
 const bool initializing=a->state.init_state>=0&&a->state.init_state<2&&a->current.angle.x==0&&fpcM_IsCreating(id);
 devf_variant::Entry* entry=regen_begin(a,id,initializing);
 const bool fresh=entry&&!entry->classified;
 devf_black2::Snapshot s;bool have_snapshot=false;
 if(fresh||table.armed){s=record_snapshot(a);have_snapshot=true;}
 // ---- Black admission: exactly as CP10.2 / FIX01 (exact record, first execute, id monotonic, one HP write, private texture) ----
 bool black_applied=false;
 if(table.armed&&have_snapshot){
  const int index=table.find(s);
  if(index>=0){
   auto& r=table.regs[index];
   s.id=id;s.health=a->health;s.initializing=initializing;
   s.resource_eoc2=a->mName&&std::strcmp(a->mName,"E_OC2")==0;
   if(r.last_id&&r.last_id!=s.id){const auto* previous=fopAcM_SearchByID(r.last_id);s.previous_live=previous&&previous->state.init_state!=3;}
   const auto decision=table.apply(static_cast<unsigned>(index),s);
   if(decision==devf_black2::Decision::Applied){
    const int before=a->health;a->health=r.hp; // HP route unchanged from CP10.2
    blog("HP_APPLIED",&r,s.id,"EXCLUSIVE_FIRST_CREATE",before,a->health);
    black_applied=true;
    bindings[index].reject_logged=false;
    bind_texture(static_cast<unsigned>(index),a,s.id); // separate visual route; a rejection leaves the native look
   }else if(decision==devf_black2::Decision::Fault){
    r.fault_logged=true;blog("BLACK_REJECTED",&r,s.id,r.reason,s.health,s.health);
   }
  }
 }
 // ---- CP10.4 identity + HP, once per actor generation ----
 if(fresh){
  const auto m=devf_variant::match(table,normal_table,s);
  devf_variant::Registration reg=devf_variant::Registration::None;
  const char* encounter=nullptr;int black_hp=0;
  if(m.black>=0){encounter=table.regs[m.black].id;black_hp=table.regs[m.black].hp;reg=black_applied?devf_variant::Registration::Black:devf_variant::Registration::BlackAdmissionFailed;}
  else if(m.normal>=0){encounter=normal_table.regs[m.normal].id;reg=devf_variant::Registration::Normal;}
  const auto plan=devf_variant::make_plan(a->mName,a->field_0x560,native_health,initializing,reg,black_hp);
  devf_variant::adopt(*entry,plan);entry->encounter=encounter;
  if(plan.variant==devf_variant::Variant::None){
   vlog("VARIANT_REJECTED",entry,&plan,native_health,native_health,a->health,devf_variant::why_text(plan.why));
   rlog("REGEN_REJECTED",entry,devf_variant::why_text(plan.why),devf_variant::source_text(plan.source),a->health,a->health);
   {const int c0=a->mSphs_at[0].GetAtAtp(),c1=a->mSphs_at[1].GetAtAtp();devf_damage::Plan none=devf_damage::make_plan(devf_variant::Variant::None,nullptr,0,0);
    dlog("DAMAGE_PROFILE_REJECTED",id,entry->generation,entry,&none,c0,c1,c0,c1,"CP10_4_CLASSIFICATION_REJECTED_COLLIDERS_UNTOUCHED");}
  }else{
   const int hp_before=native_health;
   if(plan.write_hp)a->health=plan.initial_hp; // the ONLY initial HP write of a native Blue/Red; never repeated
   const int hp_after=a->health;
   vlog("VARIANT_ADMITTED",entry,&plan,native_health,hp_before,hp_after,plan.write_hp?"NATIVE_BY_NAME_NO_DEVF_REGISTRATION":plan.source==devf_variant::Source::DevfBlack?"EXACT_DEVF_BLACK_REGISTRATION":"EXACT_DEVF_NORMAL_REGISTRATION_BLUE_BY_PRECEDENCE");
   vlog("HP_INITIALIZED",entry,&plan,native_health,hp_before,hp_after,plan.write_hp?"ONE_TIME_WRITE_BLUE_OR_RED":plan.source==devf_variant::Source::DevfBlack?"CONFIGURED_BLACK_HP_ALREADY_WRITTEN_BY_HP_APPLIED":"NATIVE_220_ALREADY_EQUALS_BLUE_NO_WRITE");
   if(!entry->tracked_logged&&regen_tracked_logs<200){entry->tracked_logged=true;++regen_tracked_logs;rlog("REGEN_TRACKED",entry,"CLASSIFIED_AT_FIRST_EXECUTE",devf_variant::source_text(plan.source),hp_after,hp_after);}
   damage_apply(a,id,entry); // CP10.5: only for an actor CP10.4 just accepted; rejected actors never reach this line
   if(plan.variant==devf_variant::Variant::Blue)prepare_blue_eyes(a,entry);   // CP12: at spawn, never during combat; read-only on the model
   if(plan.variant==devf_variant::Variant::Red)prepare_gold_eyes(a,entry);    // CP14: same rule for the Red gold eyes
  }
 }
 // CP10.6: health baseline of THIS pair, captured after every HP initialisation write above (220->440 / 220->150 / 40->220 are never a hit).
 if(entry){auto* d=devf_perf::find_used(ledger,id);if(d&&d->identity==a)devf_dyn::capture_pre(*d,a->health);}
 if(entry&&!fresh)blue_ai_pre(a,id,entry);   // CP16: own-execute recruitment, no evasion
 if(entry&&!fresh)cp19_pre(a,id,entry);   // CP19: 0-HP completion, Helm Splitter mask, called-ally leash permission (before Red/Black PRE)
 if(entry&&!fresh)red_ai_pre(a,id,entry);   // CP17: Red-only; SOUND_WATCH remains owned by recruitment
 if(entry&&!fresh)black_ai_pre(a,id,entry);   // CP18: Black-only; after recruitment, so a released SOUND_WATCH lease is already visible
 return HOOK_CONTINUE;
}
// CP14: x2 for this tick if the actor's own Red window is open or a Red with an open window is in the CP10.6 companion range (no stacking).
// Logs only the transitions, bounded.
unsigned regen_multiplier(daE_OC_c* a,const devf_variant::Entry* e)noexcept{
 auto* d=devf_perf::find_used(ledger,e->id);
 if(!d||d->identity!=a)return 1u;
 const bool own=red_window_open(d);
 const devf_dyn::Entry* src=nullptr;
 if(!own&&red_window_seen&&std::uint32_t(sim_seq-red_window_seq)<=devf_dyn::window_for(registry.used))src=devf_red::ally_source(ledger,red_states,*d,sim_seq,devf_dyn::window_for(registry.used));
 const unsigned mult=devf_red::regen_multiplier(own,src!=nullptr);
 auto& s=devf_red::state_for(red_states,ledger_slot(d),d->id,d->generation);
 const bool on=mult>1u;
 if(on!=s.bonus_prev||(on&&!own&&src&&src->id!=s.bonus_from)){
  RedLine x;x.mult=mult;x.rate=e->rate;x.source=own?d->id:src?src->id:s.bonus_from;x.window=own?s.window:0u;
  redlog(on?"RED_GROUP_REGEN_X2_ON":"RED_GROUP_REGEN_X2_OFF",d->id,d->generation,e,&s,x,on?(own?"OWN_RED_WINDOW_OPEN":"IN_RANGE_OF_RED_WITH_OPEN_WINDOW_650XZ_250Y"):"WINDOW_CLOSED_OR_OUT_OF_RANGE");
  s.bonus_prev=on;s.bonus_from=on?x.source:0u;
 }
 return mult;
}
// POST (after the native execute): heal only now, so native damage/death of this tick is already decided and a lethal hit is never pre-empted.
void after_execute_body(void* args)noexcept{
 if(!regen_enabled||!args||!static_cast<void**>(args)[0])return;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||a->profname!=510)return;
 devf_variant::Entry* e=registry.end(fopAcM_GetID(a),a);
 if(!e)return;
 cp19_post_early(a,e);   // CP19: Link cut type unmasked, exact Helm Splitter damage applied BEFORE CP10.6 reads the post-execute health
 const s16 before=a->health;   // right after the native execute: native damage is already applied, regeneration is not
 dynamic_post(a,fopAcM_GetID(a),e,before);   // CP10.6 reads `before` first; the regeneration step below can therefore never look like a hit
 recruit_post(a,fopAcM_GetID(a));           // CP16: native outcome readback and bounded order lifecycle
 blue_ai_post(a,fopAcM_GetID(a),e);          // CP15 group/rate/lockout preserved; solo veto removed
 red_ai_post(a,fopAcM_GetID(a),e);           // CP17: Red-only fresh-blow rate after native and dynamic/CP14 observation
 black_ai_post(a,fopAcM_GetID(a),e);         // CP18: Black-only fresh-blow rate, chain readback and hunt measurement after CP10.6
 const s16 from=a->health;     // CP14: equals `before` unless the Red lifesteal healed in dynamic_post
 const unsigned mult=regen_multiplier(a,e);
 const auto st=devf_variant::step(*e,from,a->mActionMode,mult);
 if(st.heal>0){
  a->health=st.new_health; // the only actor write of the regeneration route
  if(st.milestone&&e->events<devf_variant::kHealLogMaxPerActor){++e->events;rlog("REGEN_APPLIED",e,mult>1?"FIXED_HP_PER_SIM_TICK_ACCUMULATOR_X2_RED_WINDOW":"FIXED_HP_PER_SIM_TICK_ACCUMULATOR",devf_variant::source_text(e->source),from,a->health);}
 }
 cp19_post_late(a,e);   // CP19: attention restore and residual last-hit flag, on the final health of this tick
}
// ---- CP14 attribution (red_policy.hpp "ATTRIBUTION"): read-only view of Link at mod_update. Pointers of colliders are compared, never followed. ----
devf_red::Claim red_claim;bool link_canary_logged=false;
std::uint32_t red_passes=0,red_last_pass=0,red_last_id=0;devf_red::Why red_last_why=devf_red::Why::NoLink;bool red_claim_quiet=false;   // de-duplication of one swing's repeated outcome
devf_red::LinkSnap link_snapshot()noexcept{
 devf_red::LinkSnap L;
 daPy_py_c* p=dComIfGp_getLinkPlayer();
 if(!p||fpcM_GetName(p)!=fpcNm_ALINK_e)return L;
 auto* k=static_cast<daAlink_c*>(p);
 L.present=true;L.link=k;L.id=fopAcM_GetID(k);
 bool canary=k->mCcStts.GetAc()==k;   // layout canaries: Link's damage status names Link, and all three tg cylinders point at that status
 for(int i=0;i<3;++i){
  auto& c=k->mTgCyls[i];canary=canary&&c.GetStts()==&k->mCcStts;
  L.cyl[i].self=static_cast<cCcD_Obj*>(&c);
  L.cyl[i].raw_hit=c.cCcD_ObjHitInf::ChkTgHit()!=0;
  L.cyl[i].no_actor=c.ChkTgHitNoActor();L.cyl[i].shield=c.ChkTgShieldHit();
  L.cyl[i].hit_obj=L.cyl[i].raw_hit?static_cast<const void*>(c.cCcD_ObjHitInf::GetTgHitObj()):nullptr;
 }
 L.timer=k->mDamageTimer;L.canary=canary&&L.timer>=0&&L.timer<=150;
 L.cyl_mask=k->field_0x2fb8;L.dmg=k->mCcStts.GetDmg();
 L.life=int(dComIfGs_getLife());L.pending=dComIfGp_getItemLifeCount();
 return L;
}
// The Red whose attack sphere is the hit object of the cylinder Link will use; liveness and the Red-side reverse hit are read only then.
devf_dyn::Entry* red_for_hit(const devf_red::LinkSnap& L,devf_red::RedSnap& r)noexcept{
 const int cyl=devf_red::selected_cylinder(L);if(cyl<0||!L.cyl[cyl].hit_obj)return nullptr;
 const void* h=L.cyl[cyl].hit_obj;
 for(auto& c:ledger.entries){
  if(!c.used||c.variant!=devf_variant::Variant::Red||!c.identity)continue;
  auto* ra=static_cast<daE_OC_c*>(const_cast<void*>(c.identity));
  const void* s0=static_cast<cCcD_Obj*>(&ra->mSphs_at[0]);const void* s1=static_cast<cCcD_Obj*>(&ra->mSphs_at[1]);   // addresses only
  if(h!=s0&&h!=s1)continue;
  r=devf_red::RedSnap{};r.sph[0]=s0;r.sph[1]=s1;
  const auto* f=fopAcM_SearchByID(c.id);
  if(!f||f!=c.identity||f->state.init_state==3||f->profname!=510)return &c;   // found but not live: make_claim rejects it
  r.live=ra->mSphs_at[0].GetStts()==&ra->mStts&&ra->mSphs_at[1].GetStts()==&ra->mStts&&ra->mStts.GetAc()==ra;   // same canaries on the Red side
  for(int k=0;k<2;++k){
   r.at_hit[k]=ra->mSphs_at[k].cCcD_ObjHitInf::ChkAtHit()!=0;r.at_shield[k]=ra->mSphs_at[k].ChkAtShieldHit();
   r.at_obj[k]=r.at_hit[k]?static_cast<const void*>(ra->mSphs_at[k].cCcD_ObjHitInf::GetAtHitObj()):nullptr;
  }
  return &c;
 }
 return nullptr;
}
const devf_variant::Entry* registry_entry(std::uint32_t id)noexcept{for(const auto& e:registry.entries)if(e.used&&e.id==id)return &e;return nullptr;}
void red_watch()noexcept{
 if(!regen_enabled)return;
 ++red_passes;                                    // one per mod_update = one per management pass
 const auto L=link_snapshot();
 if(L.present&&!link_canary_logged){
  link_canary_logged=true;RedLine x;x.timer=L.timer;x.life0=L.life;
  redlog(L.canary?"LINK_LAYOUT_CANARY_PASS":"LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED",L.id,0,nullptr,nullptr,x,L.canary?"STTS_ACTOR_AND_3_TG_CYLINDERS_CONSISTENT":"LINK_FIELDS_NOT_WHERE_EXPECTED_NO_CLAIM_EVER");
 }
 if(red_claim.armed){   // pass N+2: did Link really lose life from the claimed hit?
  const auto w=devf_red::confirm(red_claim,L);
  auto* d=devf_perf::find_used(ledger,red_claim.red_id);
  const bool same=d&&d->generation==red_claim.red_generation&&d->variant==devf_variant::Variant::Red;
  const auto* e=same?registry_entry(d->id):nullptr;
  RedLine x;x.timer=L.timer;x.cyl=red_claim.cyl;x.sphere=red_claim.sphere;x.dmg=red_claim.dmg;x.life0=red_claim.life0;x.e0_x10=int(red_claim.e0*10.0f);x.e1_x10=L.present?int(L.effective()*10.0f):-1;
  if(w==devf_red::Why::Confirmed&&same){
   auto& s=devf_red::state_for(red_states,ledger_slot(d),d->id,d->generation);
   ++s.hits;if(s.pending<devf_red::kMaxPending)++s.pending;x.window=s.window;
   redlog("RED_HIT_CONFIRMED",d->id,d->generation,e,&s,x,devf_red::why_text(w));
   red_ai_damage_confirmed(d->id,d->generation);   // CP17 FIX01: read-only forwarding of CP14's own attribution
  }else if(w==devf_red::Why::Confirmed||!red_claim_quiet)redlog(w==devf_red::Why::Confirmed?"RED_HIT_CONFIRMED_RED_GONE":"RED_HIT_NOT_CONFIRMED",red_claim.red_id,red_claim.red_generation,e,nullptr,x,w==devf_red::Why::Confirmed?"RED_RELEASED_BEFORE_HEAL":devf_red::why_text(w));
  red_claim=devf_red::Claim{};
 }
 if(!L.present)return;
 bool any=false;for(const auto& c:L.cyl)any=any||c.raw_hit;
 if(!any)return;                                  // the common case: nothing hit Link in the last collision pass
 devf_red::RedSnap r;devf_dyn::Entry* d=red_for_hit(L,r);
 const auto w=devf_red::make_claim(L,d!=nullptr,r,red_claim);
 if(!d)return;                                    // not a Red's collider: nothing to say
 if(w==devf_red::Why::Claimed){red_claim.red_id=d->id;red_claim.red_generation=d->generation;}
 red_claim_quiet=true;                            // its confirmation is logged only if this claim was
 if(!devf_red::loggable(w))return;              // invulnerability / mask: repeats every pass while a sword overlaps Link
 if(d->id==red_last_id&&w==red_last_why&&red_passes==red_last_pass+1){red_last_pass=red_passes;return;}   // same swing, same outcome: one line
 red_last_id=d->id;red_last_why=w;red_last_pass=red_passes;red_claim_quiet=false;
 RedLine x;x.timer=L.timer;x.cyl=devf_red::selected_cylinder(L);x.sphere=devf_red::sphere_of(L,x.cyl,r);x.dmg=L.dmg;x.life0=L.life;x.e0_x10=int(L.effective()*10.0f);
 redlog(w==devf_red::Why::Claimed?"RED_HIT_CLAIMED":"RED_HIT_REJECTED",d->id,d->generation,registry_entry(d->id),&devf_red::state_for(red_states,ledger_slot(d),d->id,d->generation),x,devf_red::why_text(w));
}
// Round-robin release of at most 8 tracked actors per update: bounded cost, no per-frame census.
void regen_poll()noexcept{
 if(!regen_enabled||!registry.used)return;
 for(unsigned n=0;n<8;++n){
  auto& e=registry.entries[registry.cursor];registry.cursor=(registry.cursor+1)%devf_variant::kCapacity;
  if(!e.used)continue;
  const auto* actor=fopAcM_SearchByID(e.id);
  if(!actor||actor!=e.identity||actor->state.init_state==3){
   const char* why=actor?(actor==e.identity?"NATIVE_DESTRUCTION":"ADDRESS_CHANGED"):"ACTOR_GONE_ROOM_EXIT_OR_DELETE";
   rlog("REGEN_RELEASED",&e,why,devf_variant::source_text(e.source));
   if(e.classified&&!e.rejected)vlog("VARIANT_RELEASED",&e,nullptr,-1,-1,-1,why);
   damage_drop(e.id,why);
   registry.release(e);
  }
 }
}
// CP12: the hooks are timed (two counter reads each) so PERF_WINDOW can show the mod's own share of any hitch.
HookAction on_execute(ModContext*,void* args,void*,void*)noexcept{
 const std::uint64_t t0=devf_perf_qpc();const HookAction r=on_execute_body(args);
 perf_win.add_mod(devf_perf_qpc()-t0);++perf_win.hook_calls;return r;
}
void after_execute(ModContext*,void* args,void*,void*)noexcept{
 const std::uint64_t t0=devf_perf_qpc();after_execute_body(args);
 perf_win.add_mod(devf_perf_qpc()-t0);++perf_win.hook_calls;
}
// One summary line per 300 simulation ticks (~10 s). Logged at ERROR level ONLY so it survives `--log-level error` (the A/B round that silences
// all engine warnings); it is a measurement, not an error, and says so.
void perf_emit()noexcept{
 if(!svc_log||!svc_log->write)return;
 const auto& w=perf_win;char text[1000];text[0]=0;
 const unsigned long long mean=w.n?w.sum_us/w.n:0,mod_us=perf_clock.us(w.mod_ticks),mod_max=perf_clock.us(w.mod_tick_max_ticks);
 std::snprintf(text,sizeof(text),"DEVF_CP12 event=PERF_WINDOW owner=dev.bokoblin.expansion version=%s window=%u sim_ticks=%u wall_ms=%llu tick_interval_us=(mean=%llu,p50=%llu,p95=%llu,p99=%llu,max=%llu) over_40ms=%u over_50ms=%u over_67ms=%u catchup_under_8ms=%u paused_or_load_over_1s=%u mod_cpu_us=(total=%llu,per_tick=%llu,max_tick=%llu) hook_calls=%u auras=(avg_x10=%llu,max=%u) bokoblins_tracked_max=%u emitters_created=%u eye_texture_swaps=%u counter_hz=%llu evidence=WALL_CLOCK_BETWEEN_SIM_TICKS_NOT_GPU_TIME level_note=MEASUREMENT_NOT_AN_ERROR",
  kVersion,perf_windows,w.ticks,static_cast<unsigned long long>(w.sum_us/1000),mean,static_cast<unsigned long long>(w.percentile_us(500)),static_cast<unsigned long long>(w.percentile_us(950)),
  static_cast<unsigned long long>(w.percentile_us(990)),static_cast<unsigned long long>(w.max_us),w.over40,w.over50,w.over67,w.catchup,w.paused,
  mod_us,w.ticks?mod_us/w.ticks:0ull,mod_max,w.hook_calls,static_cast<unsigned long long>(w.ticks?w.aura_sum*10/w.ticks:0),w.aura_max,w.tracked_max,w.created,w.eye_swaps,
  static_cast<unsigned long long>(perf_clock.hz));
 try{svc_log->write(mod_ctx,LOG_LEVEL_ERROR,text);}catch(...){}
}
void perf_tick(std::uint64_t t0,std::uint64_t t1)noexcept{
 perf_clock.calibrate(t0,devf_perf_filetime());
 if(perf_last)perf_win.add_interval(perf_clock.us(t0-perf_last));
 perf_last=t0;
 perf_win.add_mod(t1-t0);
 perf_win.end_tick(vledger.live_auras(),registry.used);
 if(perf_win.full()){++perf_windows;perf_emit();perf_win=devf_pw::Window{};}
}
ModResult install_hooks()noexcept{
 if(!svc_hook||svc_hook->header.struct_size<sizeof(HookService)||svc_hook->header.major_version!=1||svc_hook->header.minor_version<1||!svc_hook->install||!svc_hook->add_pre||!svc_hook->add_post)return MOD_UNSUPPORTED;
 // POST first (same rule as CP04.4): no pre-side effect can exist without its post.
 auto result=mods::hook::add_post<NativeEocExecute>(after_execute);
 if(result!=MOD_OK)return result;
 return mods::hook::add_pre<NativeEocExecute>(on_execute);
}
void black_poll()noexcept{
 for(unsigned i=0;i<table.count;++i){
  auto& r=table.regs[i];if(!r.used||!r.admitted_id)continue;
  const auto* actor=fopAcM_SearchByID(r.admitted_id);
  if(!actor||actor->state.init_state==3){
   if(bindings[i].bound)tlog("BLACK_TEXTURE_RELEASED",&r,r.admitted_id,"NATIVE_DESTRUCTION_MODEL_AND_PACKETS_OWNED_BY_GAME",bindings[i].texture);
   bindings[i]={};r.admitted_id=0;
  }
 }
}
// Shutdown: re-point live bound packets to the original texture so no game object references mod memory.
// Private textures are intentionally retained (bounded: <=4 x ~8.3 KB) because a destroyed-but-not-yet-freed
// model could still reference them; the process heap is reclaimed at exit.
void black_shutdown()noexcept{
 for(unsigned i=0;i<table.count;++i){
  auto& b=bindings[i];auto& r=table.regs[i];
  if(!b.bound||!r.admitted_id||b.texture<0)continue;
  auto* base=fopAcM_SearchByID(r.admitted_id);
  if(base&&base->state.init_state!=3&&base->profname==510){
   auto* actor=static_cast<daE_OC_c*>(base);
   J3DModel* model=actor->mpMorf?actor->mpMorf->getModel():nullptr;
   if(model&&model==b.model&&model->getModelData()){
    const int restored=devf_bind::restore_body(model->getMatPacket(0),model->getMatPacket(1),model->getModelData()->getTexture(),textures[b.texture].texture,packet_get,packet_set);
    tlog(restored==2?"BLACK_TEXTURE_RESTORED":"BLACK_TEXTURE_RESTORE_PARTIAL",&r,r.admitted_id,"SHUTDOWN_PACKETS_BACK_TO_NATIVE_TEXTURE",b.texture);
   }
  }
  b={};
 }
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 const auto loaded=expansion::load_config(mod_ctx,svc_host,svc_file,config,mode_reason);
 if(loaded!=MOD_OK){event(mode_reason,8,0,loaded);return mods::set_error(error,loaded,"Expansion encounters.json invalid/unreadable; zero requests");}
 event(mode_reason,8,0,MOD_OK);
 unsigned black=0;for(unsigned i=0;i<config.count;++i)if(config.variant[i]==multi_config::variant_black)++black;
 // CP10.3: the execute hook pair is ALWAYS installed (universal regeneration also for native Bokoblins and for files with no Black).
 // Placement, Black admission and the private texture stay exactly as in 0.4.1: they only act on exactly registered Black encounters.
 {
  const auto hooked=install_hooks();
  if(hooked!=MOD_OK){event("REGEN_HOOKS_REJECTED_ZERO_REQUESTS",8,0,hooked);return mods::set_error(error,hooked,"Bokoblin execute hooks rejected; zero requests; host detach rolls back this mod only");}
 }
 // CP19 FIX02: the universal SE dependency is wanted whenever DEVF has encounters (and on any classified Bokoblin); the shield chip is always armed. The
 // flash hook is optional: if the host refuses it, the chip still takes its life unit and the flash is reported unavailable (no fake flash claim).
 cp19f2_need=config.count>0;cp19f2_chip_on=true;cp19f2_flash_hooked=install_flash_hooks()==MOD_OK;
 cp19f2_log(cp19f2_flash_hooked?"SHIELD_CHIP_ARMED":"SHIELD_CHIP_ARMED_FLASH_HOOK_UNAVAILABLE",0,0,0,devf_cp19f2::kChipLifeUnits,int(cp19f2_flash_hooked),int(config.count),int(devf_cp19f2::kOcFamilyWave),"FURY_BLOW_ENGINE_SHIELD_HIT_ONE_UNIT_PER_BLOW_METER_PENDING_COUNT_NO_MULTIPLIER_NO_ARMOUR");
 const auto r=client.initialize(mod_ctx,svc_host,svc_devf_encounter,config,event);event(client.reason,8,0,r);
 if(r!=MOD_OK)return mods::set_error(error,r,"Expansion request batch rejected; own edits rolled back/detach fallback");
 // CP10.4: exact-match registrations. Black -> HP from JSON (unchanged admission); Normal/legacy -> identity BLUE. Both stay empty when the
 // file is absent or has no enabled encounters: native E_OC (Blue) and native E_OC2 (Red) are then still classified by name.
 if(!devf_variant::register_encounters(config,multi_config::variant_black,table,normal_table)){
  client.shutdown();event("VARIANT_REGISTRATION_REJECTED_OWN_EDITS_REMOVED",8,0,MOD_ERROR);
  return mods::set_error(error,MOD_ERROR,"Encounter registration rejected; own edits removed");
 }
 regen_enabled=true;
 if(black){
  table.armed=true;
  for(unsigned i=0;i<table.count;++i)blog("BLACK_ARMED",&table.regs[i],0,"REGISTERED_WAITING_NATIVE_FIRST_EXECUTE",-1,table.regs[i].hp);
 }
 {AiState probe_state;AiLine x;x.lf=x.lt=1;x.rt=devf_blue::kRateGroup;x.ra=devf_blue::kRatePair;probe_state.id=0;
  ailog("BLUE_AI_ARMED",nullptr,probe_state,x,"BLUE_RECRUITER_FIX01_RADIUS_FROM_POLICY_NATIVE_LEASH_GROUP_1_2_3_RATE_1_15_1_30_LOCKOUT_17_15");}
 {RedLine x;x.window=devf_red::kWindowTicks;x.mult=devf_red::kRegenMultiplier;x.rate=devf_variant::kRateRed;x.hp_after=devf_variant::kMaxHpRed;
  redlog("RED_ABILITIES_ARMED",0,0,nullptr,nullptr,x,"NATIVE_E_OC2_IS_RED_LIFESTEAL_ON_CONFIRMED_LINK_LIFE_LOSS_RANGE_650XZ_250Y");}
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){const std::uint64_t t0=devf_perf_qpc();black_poll();regen_poll();recruit_poll();red_ai_poll();black_ai_poll();cp19_poll();red_watch();perf_tick(t0,devf_perf_qpc());return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 table.armed=false;regen_enabled=false; // the host removes the hooks on detach; callbacks arriving earlier see the flag and return
 red_claim=devf_red::Claim{};   // CP14: nothing of the Red route is written at unload (heals are gameplay, like regeneration); the gold eyes are restored by visual_shutdown
 red_ai_shutdown();black_ai_shutdown();cp19_shutdown();recruit_shutdown();
 blue_ai_shutdown();   // CP15 native attack rate restoration retained; recruitment is cold-start only, native damage/actions are not rolled back
 damage_shutdown();
 visual_shutdown();   // before black_shutdown: it needs the Black bindings to restore the eye packet
 black_shutdown();
 const auto r=client.shutdown();event(client.reason,8,0,r);return r==MOD_OK?MOD_OK:mods::set_error(error,r,"Expansion own edit removal failed; provider owner-detach fallback");
}
}
