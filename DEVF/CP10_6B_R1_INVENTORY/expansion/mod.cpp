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
#include "inventory_policy.hpp"
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
constexpr const char* kVersion="0.8.3";
expansion::Client client;multi_config::Config config;const char* mode_reason="NOT_READ";
devf_black2::Table table,normal_table;   // exact-match registrations: Black (HP from JSON) and Normal/legacy (identity BLUE)
devf_variant::Registry registry;bool regen_enabled=false;unsigned regen_tracked_logs=0,variant_logs=0;
devf_dyn::Ledger ledger;unsigned damage_logs=0,dynamic_logs=0;std::uint32_t sim_seq=0;   // CP10.6: one record per CP10.4-classified generation (base profile, dynamic state, companion snapshot)
struct SphereIo {daE_OC_c* a;std::uint8_t get(unsigned i)const{return a->mSphs_at[i].GetAtAtp();}void set(unsigned i,std::uint8_t v){a->mSphs_at[i].SetAtAtp(v);}};
std::uint32_t stage_tag()noexcept{const char* n=dComIfGp_getStartStageName();std::uint32_t h=2166136261u;if(!n)return h;for(;*n;++n){h^=static_cast<unsigned char>(*n);h*=16777619u;}return h;}
// ---- private textures: one per distinct original atlas (normally exactly one), kept for the whole session ----
struct PrivateTexture {const std::uint8_t* source=nullptr;std::uint64_t source_hash=0;std::uint8_t* memory=nullptr;J3DTexture* texture=nullptr;devf_tex::Stats stats;};
std::array<PrivateTexture,4> textures{};unsigned texture_count=0;
// CP10.6B-R1 DIAGNOSTIC BUILD: the aura emitter path is REMOVED (no dComIfGp_particle_set, no emitter is created, modified or destroyed). The visual controller still
// runs for the Black eyes; its aura channel is never requested. The new code only READS the registered JPA resources (inventory_policy.hpp).
devf_vis::Ledger vledger;unsigned visual_logs=0;
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
 const char* stage=dComIfGp_getStartStageName();s.stage=stage&&std::strncmp(stage,"F_SP121",8)==0;s.layer=dComIfG_play_c::getLayerNo(0);return s;
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
// ---- CP10.6B-R1 visual controller: Black eyes only. The aura channel is never requested in this diagnostic build. ----
int black_index_for(std::uint32_t id)noexcept{for(unsigned i=0;i<table.count;++i)if(table.regs[i].used&&table.regs[i].admitted_id==id)return static_cast<int>(i);return -1;}
const char* state_text(int level,bool target)noexcept{
 const auto st=devf_vis::state_of(level,target);
 return st==devf_vis::State::Active?"ACTIVE":st==devf_vis::State::FadingIn?"FADING_IN":st==devf_vis::State::FadingOut?"FADING_OUT":"INACTIVE";
}
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
struct GameFx {
 daE_OC_c* a=nullptr;const char* last="NONE";
 const char* reason()const{return last;}
 bool aura(devf_vis::Entry&,int,int){last="AURA_DISABLED_DIAGNOSTIC_BUILD";return false;}   // never reached: visual_post never requests the aura
 void aura_release(devf_vis::Entry& e){e.key=0;}
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
 auto* v=vledger.find(id);if(!v)return;
 if(v->eye_idx>0){devf_vis::Event x;x.reason=why;x.eye_idx=v->eye_idx;xlog("VISUAL_RELEASED",v,nullptr,"BOTH",x);}   // actor gone or replaced: no model, packet or texture is touched
 vledger.release(*v);
}
void visual_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e,devf_dyn::Entry* d)noexcept{
 const bool eyes=d->variant==devf_variant::Variant::Black&&d->fury>0;
 auto* v=vledger.find(id);
 if(v&&v->identity!=a){visual_drop(id,"ADDRESS_CHANGED");v=nullptr;}
 if(!v){
  if(!eyes)return;
  v=vledger.record(id,a,d->generation,d->variant);
  if(!v)return;
 }
 GameFx fx;fx.a=a;
 const auto out=devf_vis::step(vledger,*v,fx,false,eyes);   // aura target is ALWAYS false in the diagnostic build
 for(unsigned i=0;i<out.n;++i){
  const auto& x=out.ev[i];
  switch(x.kind){
   case devf_vis::Ev::EyesOn:xlog("BLACK_EYES_ON",v,e,"EYES",x);break;
   case devf_vis::Ev::EyesOff:xlog("BLACK_EYES_OFF",v,e,"EYES",x);break;
   case devf_vis::Ev::EyesRejected:xlog("BLACK_VISUAL_REJECTED",v,e,"EYES",x);break;
   default:break;
  }
 }
}
// Unload: put the native eye texture back on a still-live SAME Black whose eye packet is still ours.
void visual_shutdown()noexcept{
 for(auto& v:vledger.entries){
  if(!v.used)continue;
  const auto* found=fopAcM_SearchByID(v.id);
  const bool live=found&&found==v.identity&&found->state.init_state!=3&&found->profname==510;
  GameFx fx;fx.a=live?static_cast<daE_OC_c*>(const_cast<fopAc_ac_c*>(found)):nullptr;
  const bool had_eyes=v.eye_idx>0;
  devf_vis::cleanup(v,fx,live);
  devf_vis::Event x;x.reason=live?(had_eyes?"SHUTDOWN_EYES_RESTORED_IF_STILL_OURS":"SHUTDOWN_NOTHING_TO_RESTORE"):"SHUTDOWN_ACTOR_GONE_NOTHING_TOUCHED";
  xlog("VISUAL_CLEANUP",&v,nullptr,"BOTH",x);
  vledger.release(v);
 }
}
// ---- CP10.6B-R1 READ-ONLY JPA INVENTORY ----
// Reads, in the game callback, the registered resources of the common bank (slot 0) and of the scene bank (slot 1): resRegNum, pResAry[i]->getUsrIdx() and scalar
// metadata getters. No emitter is created, modified or destroyed, dComIfGp_particle_set / getEmitter are NOT imported, nothing is written to any engine object.
// The pResMgrAry entry is re-read from the live JPAEmitterManager every time (the engine nulls it before destroying a scene heap); only a numeric TOKEN of the
// scene manager's address is kept, to detect a new instance, and it is never dereferenced.
devf_inv::Shortlist inv_sl;devf_inv::Stats inv_st;devf_inv::Snapshot inv_snap;devf_inv::Rendered inv_rend;devf_inv::Tracker inv_tracker;devf_inv::LineBudget inv_budget;
unsigned inv_poll_counter=0,inv_snapshots=0,inv_scene_snapshots_logged=0;bool inv_common_logged=false,inv_complete=false,inv_scene_done=false;
std::uint32_t inv_last_scene_checksum=0;unsigned inv_last_scene_unique=0,inv_last_scene_snapshot=0;bool inv_have_last_scene=false;
void ilog(const char* ev,const char* body)noexcept{
 if(!svc_log||!svc_log->write)return;
 if(!inv_budget.take()){
  if(!inv_budget.exhausted_reported){inv_budget.exhausted_reported=true;char t[360]{};
   std::snprintf(t,sizeof(t),"DEVF_CP10_6B_R1_INVENTORY event=INVENTORY_PARTIAL owner=dev.bokoblin.expansion version=%s build=DIAGNOSTIC reason=LOG_BUDGET_EXHAUSTED_%u_LINES",kVersion,devf_inv::kMaxLines);
   try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,t);}catch(...){}}
  return;
 }
 char text[1400]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_6B_R1_INVENTORY event=%s owner=dev.bokoblin.expansion version=%s build=DIAGNOSTIC aura=REMOVED_NO_VISUAL_EFFECT_EXPECTED %s",ev,kVersion,body);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
bool read_meta(const JPAResource* r,devf_inv::Meta& m)noexcept{
 if(!r)return false;
 const auto* dyn=r->getDyn();const auto* bsp=r->getBsp();
 if(!dyn||!dyn->mpData||!bsp||!bsp->pBsd)return false;
 m=devf_inv::Meta{};m.id=r->getUsrIdx();
 m.shape=bsp->getType();m.dir=bsp->getDirType();m.rot=bsp->getRotType();m.sx=bsp->getBaseSizeX();m.sy=bsp->getBaseSizeY();m.clr_anm=bsp->getClrAnmType();
 GXColor prm{},env{};bsp->getPrmClr(&prm);bsp->getEnvClr(&env);
 m.prm[0]=prm.r;m.prm[1]=prm.g;m.prm[2]=prm.b;m.prm[3]=prm.a;m.env[0]=env.r;m.env[1]=env.g;m.env[2]=env.b;
 m.volume_type=dyn->getVolumeType();m.volume_size=dyn->getVolumeSize();m.max_frame=dyn->getMaxFrame();m.start=dyn->getStartFrame();m.life=dyn->getLifetime();
 m.rate=dyn->getRate();m.rate_rndm=dyn->getRateRndm();m.vel_dir=dyn->getInitVelDir();m.vel_omni=dyn->getInitVelOmni();m.vel_axis=dyn->getInitVelAxis();
 JGeometry::TVec3<f32> dv(0,0,0);dyn->getEmitterDir(&dv);m.edir[0]=dv.x;m.edir[1]=dv.y;m.edir[2]=dv.z;
 m.dyn_flags=dyn->getFlag();m.user_work=dyn->getResUserWork();m.keys=r->keyNum;m.fields=r->fldNum;m.texs=r->texNum;
 m.child=r->getCsp()!=nullptr;m.extra=r->getEsp()!=nullptr;m.extex=r->getEts()!=nullptr;
 return true;
}
const char* feature_text(unsigned f,char* out)noexcept{unsigned n=0;if(f&devf_inv::F_E)out[n++]='E';if(f&devf_inv::F_V)out[n++]='V';if(f&devf_inv::F_P)out[n++]='P';if(f&devf_inv::F_S)out[n++]='S';if(f&devf_inv::F_N)out[n++]='N';if(!n)out[n++]='-';out[n]=0;return out;}
void inv_shortlist(const JPAResourceManager* mgr,unsigned bank,const char* role,unsigned snapshot)noexcept{
 auto& sl=inv_sl;auto& st=inv_st;sl=devf_inv::Shortlist{};st=devf_inv::Stats{};   // namespace-scope storage: no function-local static (no thread-safe-init CRT imports)
 const unsigned n=mgr->resRegNum;
 for(unsigned i=0;i<n;++i){devf_inv::Meta m;if(!read_meta(mgr->pResAry[i],m)){if(mgr->pResAry[i])++st.no_meta;continue;}st.add(m);sl.offer(m);}
 char body[1000]{};
 std::snprintf(body,sizeof(body),"snapshot=%u bank=%u role=%s resources=%u no_metadata=%u shape_hist(0pt,1line,2bb,3dir,4dir+,5stripe,6stripe+,7rot,8rot+,9dbb,10ybb,other)=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u elongated=%u persistent=%u keyed=%u with_child=%u shortlist=%u eligible=%u heuristic=ELONGATION_VERTICAL_PERSISTENCE_NOT_VISUAL_PROOF",
  snapshot,bank,role,st.total,st.no_meta,st.by_shape[0],st.by_shape[1],st.by_shape[2],st.by_shape[3],st.by_shape[4],st.by_shape[5],st.by_shape[6],st.by_shape[7],st.by_shape[8],st.by_shape[9],st.by_shape[10],st.by_shape[11],st.elongated,st.persistent,st.keyed,st.with_child,sl.n,sl.eligible_count);
 ilog("INVENTORY_STATS",body);
 for(unsigned k=0;k<sl.n;++k){
  const auto& c=sl.top[k];const auto& m=c.meta;char fl[8];
  std::snprintf(body,sizeof(body),"snapshot=%u bank=%u role=%s rank=%u usr_index=0x%04X base_id=0x%04X bank_bit=%u score=%d features=%s shape=%u dir=%u rot=%u base_size_x100=(%d,%d) aspect_x100=%d volume_type=%u volume_size=%u max_frame=%d start=%d life=%d rate_x1000=%d rate_rndm_x1000=%d vel_dir_x1000=%d vel_omni_x1000=%d vel_axis_x1000=%d emitter_dir_x1000=(%d,%d,%d) dyn_flags=0x%08x user_work=0x%08x clr_anm=%u prm_rgba=(%u,%u,%u,%u) env_rgb=(%u,%u,%u) keys=%u fields=%u textures=%u child=%d extra=%d ex_tex=%d catalogue_name=RESOLVE_OFFLINE_FROM_USR_INDEX",
   snapshot,bank,role,k+1,unsigned(m.id),unsigned(m.id&0x7FFF),unsigned((m.id>>15)&1),c.score,feature_text(c.feat,fl),m.shape,m.dir,m.rot,int(m.sx*100.0f),int(m.sy*100.0f),int(devf_inv::aspect_of(m)*100.0f),m.volume_type,m.volume_size,m.max_frame,m.start,m.life,
   int(m.rate*1000.0f),int(m.rate_rndm*1000.0f),int(m.vel_dir*1000.0f),int(m.vel_omni*1000.0f),int(m.vel_axis*1000.0f),int(m.edir[0]*1000.0f),int(m.edir[1]*1000.0f),int(m.edir[2]*1000.0f),unsigned(m.dyn_flags),unsigned(m.user_work),m.clr_anm,
   unsigned(m.prm[0]),unsigned(m.prm[1]),unsigned(m.prm[2]),unsigned(m.prm[3]),unsigned(m.env[0]),unsigned(m.env[1]),unsigned(m.env[2]),m.keys,m.fields,m.texs,m.child?1:0,m.extra?1:0,m.extex?1:0);
  ilog("INVENTORY_SHORTLIST",body);
 }
}
void inv_snapshot(unsigned bank,const char* role)noexcept{
 auto* em=dPa_control_c::getEmitterManager();
 const devf_inv::Why why=devf_inv::collect_bank(em,bank,inv_snap);
 char body[1000]{};
 if(why!=devf_inv::Why::None){
  std::snprintf(body,sizeof(body),"bank=%u role=%s reason=%s entries=%u res_max=%u",bank,role,devf_inv::why_text(why),inv_snap.entries,inv_snap.res_max);
  ilog(why==devf_inv::Why::CountOutOfRange||why==devf_inv::Why::NullArray?"INVENTORY_FAILED":"INVENTORY_PARTIAL",body);return;
 }
 const unsigned snapshot=++inv_snapshots;
 const bool scene=bank==1;
 const bool same=scene&&inv_have_last_scene&&inv_last_scene_checksum==inv_snap.checksum&&inv_last_scene_unique==inv_snap.unique;
 std::snprintf(body,sizeof(body),"snapshot=%u bank=%u role=%s manager_state=READY stage_tag=0x%08x entries=%u listed=%u unique=%u dups=%u nulls=%u res_max=%u ridMax=%u gidMax=%u platform_detected=NOT_AVAILABLE_TO_MOD common_path_if_GCN=/res/Particle/common.jpc common_path_if_Wii=/res/Particle/common-r.jpc ids=SORTED_UNIQUE_RAW_USER_INDEX_BANK1_KEEPS_0x8000",
  snapshot,bank,role,unsigned(stage_tag()),inv_snap.entries,inv_snap.listed,inv_snap.unique,inv_snap.dups,inv_snap.nulls,inv_snap.res_max,unsigned(em->ridMax),unsigned(em->gidMax));
 ilog("INVT_BEGIN",body);
 devf_inv::render(inv_snap,inv_rend);
 if(!same){
  for(unsigned c=0;c<inv_rend.chunks;++c){
   std::snprintf(body,sizeof(body),"snapshot=%u bank=%u chunk=%u/%u first_index=%u ids_in_chunk=%u data=%s",snapshot,bank,c+1,inv_rend.chunks,inv_rend.first_index[c],inv_rend.id_count[c],inv_rend.text[c].data());
   ilog("INVT_IDS",body);
  }
 }
 const bool verified=inv_rend.ids_shown+inv_rend.ids_missing==inv_snap.unique;
 std::snprintf(body,sizeof(body),"snapshot=%u bank=%u count=%u checksum=0x%08x chunks=%u ids_shown=%u ids_missing=%u truncated=%d same_as_snapshot=%d integrity=%s",
  snapshot,bank,inv_snap.unique,unsigned(inv_snap.checksum),same?0u:inv_rend.chunks,same?0u:inv_rend.ids_shown,same?0u:inv_rend.ids_missing,(!same&&inv_rend.ids_missing)?1:0,same?int(inv_last_scene_snapshot):-1,verified?"SHOWN_PLUS_MISSING_EQUALS_UNIQUE":"MISMATCH");
 ilog("INVT_END",body);
 std::snprintf(body,sizeof(body),"snapshot=%u bank=%u count=%u checksum=0x%08x",snapshot,bank,inv_snap.unique,unsigned(inv_snap.checksum));
 ilog(scene?"INVENTORY_READY_SCENE":"INVENTORY_READY_COMMON",body);
 if(!same)inv_shortlist(em->pResMgrAry[bank],bank,role,snapshot);   // metadata of the same, still-live manager, same callback
 if(scene){inv_have_last_scene=true;inv_last_scene_checksum=inv_snap.checksum;inv_last_scene_unique=inv_snap.unique;inv_last_scene_snapshot=snapshot;++inv_scene_snapshots_logged;inv_scene_done=true;}
 else inv_common_logged=true;
 if(!inv_complete&&inv_common_logged&&inv_scene_done){inv_complete=true;std::snprintf(body,sizeof(body),"snapshots=%u common=1 scene=%u",inv_snapshots,inv_scene_snapshots_logged);ilog("INVENTORY_COMPLETE",body);}
}
void inventory_poll()noexcept{
 if(!regen_enabled)return;
 if((++inv_poll_counter&15u)!=1u)return;   // every 16th execute pair (first one included): negligible cost, no per-frame work
 auto* em=dPa_control_c::getEmitterManager();
 const bool mgr_ok=em&&em->pResMgrAry;
 bool common_ready=false,scene_present=false;devf_inv::Sig sig;
 if(mgr_ok){
  if(em->ridMax>0){const JPAResourceManager* c=em->pResMgrAry[0];common_ready=c&&c->pResAry&&c->resRegNum>0;}
  if(em->ridMax>1){const JPAResourceManager* sc=em->pResMgrAry[1];scene_present=sc!=nullptr;
   if(sc){sig.token=static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(sc));sig.res_reg=sc->resRegNum;sig.res_max=sc->resMaxNum;sig.stage=stage_tag();}}
 }
 char body[400]{};
 switch(inv_tracker.poll(mgr_ok,common_ready,scene_present,sig)){
  case devf_inv::Act::SnapCommon:inv_snapshot(0,"COMMON");break;
  case devf_inv::Act::SnapScene:inv_snapshot(1,"SCENE");break;
  case devf_inv::Act::CommonNotReady:std::snprintf(body,sizeof(body),"bank=0 role=COMMON reason=COMMON_BANK_NOT_READY_AFTER_%u_POLLS",devf_inv::Tracker::kCommonGiveUp);ilog("INVENTORY_PARTIAL",body);break;
  case devf_inv::Act::SceneAbsent:std::snprintf(body,sizeof(body),"bank=1 role=SCENE reason=SCENE_BANK_NOT_REGISTERED_AFTER_%u_POLLS",devf_inv::Tracker::kAbsentPolls);ilog("INVENTORY_PARTIAL",body);inv_scene_done=true;break;
  case devf_inv::Act::SceneCapReached:std::snprintf(body,sizeof(body),"bank=1 role=SCENE reason=SCENE_SNAPSHOT_CAP_%u_REACHED",devf_inv::kMaxSceneSnapshots);ilog("INVENTORY_PARTIAL",body);break;
  default:break;
 }
}
// ---- CP10.5 base attack power (unchanged semantics) + CP10.6 ledger: every CP10.4-classified generation gets ONE record. Only a pristine native
// pair is `owned` (and so ever written); the others are still real Bokoblins that may count as somebody's companion. ----
void damage_drop(std::uint32_t id,const char* why)noexcept{
 auto* old=ledger.find(id);if(!old)return;
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
 auto* d=ledger.find(id);if(!d||d->identity!=a)return;
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
 if(entry){auto* d=ledger.find(id);if(d&&d->identity==a)devf_dyn::capture_pre(*d,a->health);}
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
 inventory_poll();   // CP10.6B-R1: bounded, read-only; never touches the actor
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
