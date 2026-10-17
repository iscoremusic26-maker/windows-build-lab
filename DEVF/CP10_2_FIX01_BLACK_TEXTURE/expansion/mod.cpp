// DEVF CP10.2 FIX01 — Bokoblin Expansion 0.4.1.
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
#include "cmpr_recolor.hpp"
#include "texture_binding.hpp"
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
constexpr const char* kVersion="0.4.1";
expansion::Client client;multi_config::Config config;const char* mode_reason="NOT_READ";
devf_black2::Table table;
// ---- private textures: one per distinct original atlas (normally exactly one), kept for the whole session ----
struct PrivateTexture {const std::uint8_t* source=nullptr;std::uint64_t source_hash=0;std::uint8_t* memory=nullptr;J3DTexture* texture=nullptr;devf_tex::Stats stats;};
std::array<PrivateTexture,4> textures{};unsigned texture_count=0;
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
}
HookAction on_execute(ModContext*,void* args,void*,void*)noexcept{
 if(!table.armed||!args||!static_cast<void**>(args)[0])return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||a->profname!=510)return HOOK_CONTINUE;
 auto s=record_snapshot(a);
 const int index=table.find(s);
 if(index<0)return HOOK_CONTINUE;
 auto& r=table.regs[index];
 s.id=fopAcM_GetID(a);s.health=a->health;
 s.initializing=a->state.init_state>=0&&a->state.init_state<2&&a->current.angle.x==0&&fpcM_IsCreating(s.id);
 s.resource_eoc2=a->mName&&std::strcmp(a->mName,"E_OC2")==0;
 if(r.last_id&&r.last_id!=s.id){const auto* previous=fopAcM_SearchByID(r.last_id);s.previous_live=previous&&previous->state.init_state!=3;}
 const auto decision=table.apply(static_cast<unsigned>(index),s);
 if(decision==devf_black2::Decision::Applied){
  const int before=a->health;a->health=r.hp; // HP route unchanged from CP10.2
  blog("HP_APPLIED",&r,s.id,"EXCLUSIVE_FIRST_CREATE",before,a->health);
  bindings[index].reject_logged=false;
  bind_texture(static_cast<unsigned>(index),a,s.id); // separate visual route; a rejection leaves the native look
 }else if(decision==devf_black2::Decision::Fault){
  r.fault_logged=true;blog("BLACK_REJECTED",&r,s.id,r.reason,s.health,s.health);
 }
 return HOOK_CONTINUE;
}
ModResult install_hooks()noexcept{
 if(!svc_hook||svc_hook->header.struct_size<sizeof(HookService)||svc_hook->header.major_version!=1||svc_hook->header.minor_version<1||!svc_hook->install||!svc_hook->add_pre)return MOD_UNSUPPORTED;
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
 // Hooks only exist when at least one Black Bokoblin is configured: a Normal-only file behaves exactly like 0.3.0.
 if(black){
  const auto hooked=install_hooks();
  if(hooked!=MOD_OK){event("BLACK_HOOKS_REJECTED_ZERO_REQUESTS",8,0,hooked);return mods::set_error(error,hooked,"Black Bokoblin hook rejected; zero requests; host detach rolls back this mod only");}
 }
 const auto r=client.initialize(mod_ctx,svc_host,svc_devf_encounter,config,event);event(client.reason,8,0,r);
 if(r!=MOD_OK)return mods::set_error(error,r,"Expansion request batch rejected; own edits rolled back/detach fallback");
 if(black){
  for(unsigned i=0;i<config.count;++i){
   if(config.variant[i]!=multi_config::variant_black)continue;
   const auto& q=config.requests[i];
   if(!table.add(q.placement_id,q.room,q.layer,q.x,q.y,q.z,q.yaw_s16,config.hp[i])){
    client.shutdown();event("BLACK_REGISTRATION_REJECTED_OWN_EDITS_REMOVED",i,0,MOD_ERROR);
    return mods::set_error(error,MOD_ERROR,"Black Bokoblin registration rejected; own edits removed");
   }
  }
  table.armed=true;
  for(unsigned i=0;i<table.count;++i)blog("BLACK_ARMED",&table.regs[i],0,"REGISTERED_WAITING_NATIVE_FIRST_EXECUTE",-1,table.regs[i].hp);
 }
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){black_poll();return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 table.armed=false;
 black_shutdown();
 const auto r=client.shutdown();event(client.reason,8,0,r);return r==MOD_OK?MOD_OK:mods::set_error(error,r,"Expansion own edit removal failed; provider owner-detach fallback");
}
}
