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
#include "strokes_policy.hpp"
#include "perf_policy.hpp"
// Game headers after the DEVF config headers (same ordering rule as CP10.2).
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
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
static_assert(sizeof(decltype(&daE_OC_c::execute))==8,"Reviewed Windows x64 compact PMF required");
static_assert(std::is_same_v<decltype(daE_OC_c::health),s16>);
static_assert(sizeof(ResTIMG)==0x20,"ResTIMG layout");
static_assert(std::is_same_v<decltype(J3DMatPacket::mpTexture),J3DTexture*>,"Per-instance material packet texture field");
namespace {
constexpr const char* kVersion="0.8.9";
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
bool build_eye_set(unsigned t,J3DTexture* original,const char*& why)noexcept{
 auto& set=eye_sets[t];
 if(set.built)return true;
 if(set.failed){why="EYE_VARIANTS_PREVIOUSLY_FAILED";return false;}
 set.failed=true;
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
  if(!devf_eye::recolor_cmpr(image,bytes,devf_eye::amount_pm(k),devf_eye::kPurple,st)||st.blocks!=bytes/8){delete[] memory;why="EYE_RECOLOR_CONTRACT";return false;}
  if(k==devf_eye::kVariants-1){set.stats=st;if(!st.endpoints_changed){delete[] memory;why="NO_GREEN_ENDPOINTS_IN_ATLAS_EYES_WOULD_NOT_CHANGE";return false;}}
  for(u16 i=0;i<n;++i){
   std::memcpy(&res[i],original->getResTIMG(i),sizeof(ResTIMG));
   res[i].imageOffset=static_cast<s32>(image-reinterpret_cast<std::uint8_t*>(&res[i]));
   res[i].paletteOffset=0;
  }
  J3DTexture* texture=nullptr;try{texture=new J3DTexture(n,res);}catch(...){texture=nullptr;}
  if(!texture){delete[] memory;why="EYE_J3DTEXTURE_ALLOCATION";return false;}
  for(u16 i=0;i<n;++i)if(texture->getImgDataPtr(i)!=image){why="EYE_TEXTURE_POINTER_CONTRACT";return false;}   // keep memory: the object may hold GX ids
  set.memory[k]=memory;set.texture[k]=texture;
 }
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
  else if(first)tlog("BLACK_EYES_PREPARED",&table.regs[index],id,"SEVEN_PRIVATE_PURPLE_VARIANTS_OF_THE_ATLAS_FOR_THE_EYE_PACKET_ONLY",t);}
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
  std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=RESOURCE_PROBE owner=dev.bokoblin.expansion version=%s resource=%s bank=%u ok=1 used_by=BLUE_AND_BLACK banks=%u groups=%u user_work=0x%08x dyn_flags=0x%08x volume_type=%u volume_size=%u max_frame=%d start_frame=%d life=%d rate_x1000=%d rate_rndm_x1000=%d vel_dir_x1000=%d emitter_dir_x1000=(%d,%d,%d) shape_type=%u dir_type=%u rot_type=%u base_size_x100=(%d,%d) clr_anm=%u prm_rgba=(%u,%u,%u,%u) env_rgb=(%u,%u,%u) keys=%u key0=(id=%u,loop=%u,n=%u) key1=(id=%u,loop=%u,n=%u) fields=%u textures=%u child=%d extra=%d scale_anim=%d scale_in_x100=(%d,%d) scale_out_x100=(%d,%d) scale_in_t_x100=%d scale_out_t_x100=%d pixels=NOT_VERIFIED",
   kVersion,res_name(id),devf_stk::kBank,unsigned(m->ridMax),unsigned(m->gidMax),dyn?unsigned(dyn->getResUserWork()):0u,dyn?unsigned(dyn->getFlag()):0u,dyn?unsigned(dyn->getVolumeType()):0u,dyn?unsigned(dyn->getVolumeSize()):0u,
   dyn?int(dyn->getMaxFrame()):0,dyn?int(dyn->getStartFrame()):0,dyn?int(dyn->getLifetime()):0,dyn?int(dyn->getRate()*1000.0f):0,dyn?int(dyn->getRateRndm()*1000.0f):0,dyn?int(dyn->getInitVelDir()*1000.0f):0,
   int(dir.x*1000.0f),int(dir.y*1000.0f),int(dir.z*1000.0f),bsp?bsp->getType():0u,bsp?bsp->getDirType():0u,bsp?bsp->getRotType():0u,bsp?int(bsp->getBaseSizeX()*100.0f):0,bsp?int(bsp->getBaseSizeY()*100.0f):0,bsp?bsp->getClrAnmType():0u,
   unsigned(prm.r),unsigned(prm.g),unsigned(prm.b),unsigned(prm.a),unsigned(env.r),unsigned(env.g),unsigned(env.b),unsigned(res->keyNum),kid0,kloop0,knum0,kid1,kloop1,knum1,unsigned(res->fldNum),unsigned(res->texNum),res->getCsp()?1:0,res->getEsp()?1:0,
   sa?1:0,sa?int(esp->getScaleInValueX()*100.0f):0,sa?int(esp->getScaleInValueY()*100.0f):0,sa?int(esp->getScaleOutValueX()*100.0f):0,sa?int(esp->getScaleOutValueY()*100.0f):0,sa?int(esp->getScaleInTiming()*100.0f):0,sa?int(esp->getScaleOutTiming()*100.0f):0);
 }
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
 if(ok&&res)glog(id,res);
 std::snprintf(text,sizeof(text),"DEVF_CP10_6D event=%s owner=dev.bokoblin.expansion version=%s resource=%s bank=%u used_by=BLUE_AND_BLACK reason=%s pixels=NOT_VERIFIED",ok&&res?"RESOURCE_FOUND":"RESOURCE_MISSING",kVersion,res_name(id),devf_stk::kBank,reason);
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
struct GameFx {
 daE_OC_c* a=nullptr;devf_stk::AuraPart<RealEngine>* part=nullptr;const char* last="NONE";
 const char* reason()const{return last;}
 bool aura(devf_vis::Entry& e,int l,int alpha){const bool ok=part->aura(e,l,alpha);last=part->why;return ok;}
 void aura_release(devf_vis::Entry& e){part->aura_release(e);}
 // idx 0 = native texture, 1..7 = private purple variants. Writes only if the eye packet still holds exactly what this module put there.
 bool eye(devf_vis::Entry& e,unsigned idx){
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
  return true;
 }
};
void visual_drop(std::uint32_t id,const char* why)noexcept{
 auto* v=devf_perf::find_used(vledger,id);if(!v)return;
 if(v->aura_live)slog("EMITTER_RELEASED",v,nullptr,why,0,nullptr,0,devf_stk::resource_for(v->variant));   // actor gone or replaced: the key is only forgotten, the engine removes the unrefreshed emitter
 if(v->aura_live||v->eye_idx>0){devf_vis::Event x;x.reason=why;x.eye_idx=v->eye_idx;xlog("VISUAL_RELEASED",v,nullptr,"BOTH",x);}   // no model, packet or texture is touched
 vledger.release(*v);
}
void visual_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry* d)noexcept{
 const bool aura=(d->variant==devf_variant::Variant::Blue&&d->group)||(d->variant==devf_variant::Variant::Black&&d->fury>0);
 const bool eyes=d->variant==devf_variant::Variant::Black&&d->fury>0;
 auto* v=devf_perf::find_used(vledger,id);
 if(v&&v->identity!=a){visual_drop(id,"ADDRESS_CHANGED");v=nullptr;}
 if(!v){
  if(!aura&&!eyes)return;
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
 const auto out=devf_vis::step(vledger,*v,fx,aura,eyes);
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
   case devf_stk::Note::Acquired:slog("EMITTER_ACQUIRED",v,e,n.why,n.fails,nullptr,0,n.resource);acquired=true;break;
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
// POST of an execute pair (after the NATIVE execute, before regeneration): companion snapshot, Blue group, Black fury.
void dynamic_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,int post_health)noexcept{
 ++sim_seq;
 auto* d=devf_perf::find_used(ledger,id);if(!d||d->identity!=a)return;
 SphereIo io{a};devf_dyn::Post in;in.post_health=post_health;in.action=a->mActionMode;in.room=fopAcM_GetRoomNo(a);
 in.x=a->current.pos.x;in.y=a->current.pos.y;in.z=a->current.pos.z;in.stage=stage_tag();in.seq=sim_seq;in.window=devf_dyn::window_for(registry.used);
 const auto ev=devf_dyn::post_update(ledger,*d,io,in,[](const devf_dyn::Entry& c){
  const auto* f=fopAcM_SearchByID(c.id);return f&&f==c.identity&&f->state.init_state!=3&&f->profname==510;});
 visual_post(a,id,e,d);   // CP10.6A: reads the logic state just computed above (d->group / d->fury), never recomputes it
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
// CP10.4 universal tracking + one-time classification (PRE of the actor's execute). No actor scan: only the executing actor is touched.
devf_variant::Entry* regen_begin(daE_OC_c* a,std::uint32_t id,bool initializing)noexcept{
 if(!regen_enabled)return nullptr;
 bool stale=false;devf_variant::Why refused=devf_variant::Why::None;
 devf_variant::Entry* e=registry.begin(id,a,initializing,&stale,&refused);
 if(stale){rlog("REGEN_RELEASED",nullptr,"ID_ADDRESS_REUSE_OR_NEW_NATIVE_CREATION_NO_STATE_INHERITED","NONE");damage_drop(id,"ID_ADDRESS_REUSE_OR_NEW_NATIVE_CREATION_NO_STATE_INHERITED");}
 if(!e&&refused==devf_variant::Why::RegistryFull&&!registry.full_logged){registry.full_logged=true;rlog("REGEN_REJECTED",nullptr,devf_variant::why_text(refused),"NONE");}
 return e;
}
HookAction on_execute(ModContext*,void* args,void*,void*)noexcept{
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
  }
 }
 // CP10.6: health baseline of THIS pair, captured after every HP initialisation write above (220->440 / 220->150 / 40->220 are never a hit).
 if(entry){auto* d=devf_perf::find_used(ledger,id);if(d&&d->identity==a)devf_dyn::capture_pre(*d,a->health);}
 return HOOK_CONTINUE;
}
// POST (after the native execute): heal only now, so native damage/death of this tick is already decided and a lethal hit is never pre-empted.
void after_execute(ModContext*,void* args,void*,void*)noexcept{
 if(!regen_enabled||!args||!static_cast<void**>(args)[0])return;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||a->profname!=510)return;
 devf_variant::Entry* e=registry.end(fopAcM_GetID(a),a);
 if(!e)return;
 const s16 before=a->health;   // right after the native execute: native damage is already applied, regeneration is not
 dynamic_post(a,fopAcM_GetID(a),e,before);   // CP10.6 reads `before` first; the regeneration step below can therefore never look like a hit
 const auto st=devf_variant::step(*e,before,a->mActionMode);
 if(st.heal>0){
  a->health=st.new_health; // the only actor write of the regeneration route
  if(st.milestone&&e->events<devf_variant::kHealLogMaxPerActor){++e->events;rlog("REGEN_APPLIED",e,"FIXED_HP_PER_SIM_TICK_ACCUMULATOR",devf_variant::source_text(e->source),before,a->health);}
 }
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
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){black_poll();regen_poll();return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 table.armed=false;regen_enabled=false; // the host removes the hooks on detach; callbacks arriving earlier see the flag and return
 damage_shutdown();
 visual_shutdown();   // before black_shutdown: it needs the Black bindings to restore the eye packet
 black_shutdown();
 const auto r=client.shutdown();event(client.reason,8,0,r);return r==MOD_OK?MOD_OK:mods::set_error(error,r,"Expansion own edit removal failed; provider owner-detach fallback");
}
}
