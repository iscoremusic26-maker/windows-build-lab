#include <mods/service.hpp>
#include <mods/svc/log.h>
#include <mods/svc/hook.hpp>
#include "client.hpp"
#include "black_policy.hpp"
#include "xf_scope.hpp"
// Game headers after the DEVF config headers (see native_capture.cpp for the same ordering rule).
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_manager.h>
#include <cstdio>
#include <type_traits>
#include <JSystem/J3DGraphBase/J3DShape.h>
#include <JSystem/J3DGraphAnimator/J3DModelData.h>
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
DEFINE_HOOK(&J3DShapePacket::drawFast,NativeShapePacketDraw);
static_assert(std::is_same_v<decltype(&J3DShapePacket::drawFast),void(J3DShapePacket::*)()>);
static_assert(sizeof(decltype(&J3DShapePacket::drawFast))==8);
static_assert(sizeof(GXColor)==4);
static_assert(std::is_same_v<decltype(&daE_OC_c::execute),int(daE_OC_c::*)()>);
static_assert(sizeof(decltype(&daE_OC_c::execute))==8,"Reviewed Windows x64 compact PMF required");
static_assert(std::is_same_v<decltype(daE_OC_c::health),s16>);
namespace {
expansion::Client client;multi_config::Config config;const char* mode_reason="NOT_READ";
devf_black2::Table table;devf_black::Scope scope;int scope_reg=-1;
void event(const char* e,unsigned i,uint64_t token,ModResult r)noexcept{
 if(!svc_log||!svc_log->write)return;char text[640]{};const auto* s=i<config.count?&config.requests[i]:nullptr;
 std::snprintf(text,sizeof(text),"BOKOBLIN_EXPANSION event=%s owner=dev.bokoblin.expansion encounter_id=%s stage=%s room=%d layer=%d xyz=(%.9g,%.9g,%.9g) yaw=%d ticket=%llu result=%d actor_id=NOT_OBSERVED evidence=STAGE_EDIT_ONLY terrain=NOT_VERIFIED source=%s",e,s?s->placement_id:"NONE",s?s->stage:"NONE",s?int(s->room):-1,s?int(s->layer):-1,s?double(s->x):0.,s?double(s->y):0.,s?double(s->z):0.,s?s->yaw_s16:0,static_cast<unsigned long long>(token),int(r),DEVF_SOURCE_SHA);
 try{svc_log->write(mod_ctx,r==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
void blog(const char* ev,const devf_black2::Reg* r,std::uint32_t actor,const char* reason,std::uint32_t value=0,int hp_before=-1,int hp_after=-1)noexcept{
 if(!svc_log||!svc_log->write)return;char text[640]{};
 std::snprintf(text,sizeof(text),"DEVF_CP10_2_BLACK event=%s owner=dev.bokoblin.expansion version=0.4.0 encounter_id=%s actor_id=%u generation=%llu stage=F_SP121 room=%d layer=%d hp_before=%d hp_after=%d mechanism=PRIVATE_FIFO_XF_MATCOLOR0 rgb_word=%08X reason=%s evidence=COMMAND_WORD_READBACK visual=HUMAN_RUNTIME_PENDING",
  ev,r?r->id:"NONE",actor,static_cast<unsigned long long>(r?r->generation:0),r?r->room:-1,r?r->layer:-1,hp_before,hp_after,value,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}catch(...){}
}
void black_emit(const devf_black::Command& command)noexcept{GXCallDisplayList(command.data(),static_cast<u32>(command.size()));}
devf_black2::Snapshot record_snapshot(daE_OC_c* a)noexcept{
 devf_black2::Snapshot s;s.profile=a->profname;s.room=fopAcM_GetRoomNo(a);s.parameters=fopAcM_GetParam(a);
 s.set_id=a->setID;s.x=a->home.pos.x;s.y=a->home.pos.y;s.z=a->home.pos.z;
 s.home_x=a->home.angle.x;s.home_y=a->home.angle.y;s.home_z=a->home.angle.z;
 const char* stage=dComIfGp_getStartStageName();s.stage=stage&&std::strncmp(stage,"F_SP121",8)==0;s.layer=dComIfG_play_c::getLayerNo(0);return s;
}
void reject_reg(int index,const char* reason)noexcept{
 if(scope.active)scope.abort(black_emit,reason);
 if(index<0||unsigned(index)>=table.count)return;
 auto& r=table.regs[index];table.reject(r,reason);
 if(!r.fault_logged){r.fault_logged=true;blog("BLACK_REJECTED",&r,r.last_id,reason);}
}
// First native execute of the configured record: the one-shot HP write (same rules as the human-observed CP03/CP04.4 mod).
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
 if(r.last_id&&r.last_id!=s.id){ // query only the previously verified id, never a census
  const auto* previous=fopAcM_SearchByID(r.last_id);s.previous_live=previous&&previous->state.init_state!=3;
 }
 const auto decision=table.apply(static_cast<unsigned>(index),s);
 if(decision==devf_black2::Decision::Applied){
  const int before=a->health;a->health=r.hp; // the only actor-field write in this module
  blog("HP_APPLIED",&r,s.id,"EXCLUSIVE_FIRST_CREATE",0,before,a->health);
  blog("BLACK_PREFLIGHT",&r,s.id,"ADMITTED_WAITING_NATIVE_PACKET");
 }else if(decision==devf_black2::Decision::Fault){
  r.fault_logged=true;blog("BLACK_REJECTED",&r,s.id,r.reason,0,s.health,s.health);
 }
 return HOOK_CONTINUE; // original execute always runs
}
bool black_material_contract(J3DModelData* data)noexcept{
 static const char* names[]{"oc_body_mat01","oc_body_mat02","oc_eye_mat"};
 auto* textures=data->getTexture();
 if(!textures||textures->getNum()!=3)return false;
 for(unsigned i=0;i<3;i++){
  auto* image=textures->getResTIMG(static_cast<u16>(i));
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
 if(!textures->getImgDataPtr(0)||textures->getImgDataPtr(0)!=textures->getImgDataPtr(1)||
  textures->getImgDataPtr(0)!=textures->getImgDataPtr(2))return false;
 auto& vertex=data->getVertexData();
 return vertex.getVtxColorArray(0)==nullptr&&vertex.getVtxColorArray(1)==nullptr;
}
HookAction before_black_packet(ModContext*,void* args,void*,void*)noexcept{
 // The pinned native packet method is non-recursive. Never scan actors beyond the admitted ids; never retain model pointers.
 if(scope.active){reject_reg(scope_reg,"NESTED_NATIVE_PACKET_NOT_SUPPORTED");return HOOK_CONTINUE;}
 if(!table.armed||!table.any_admitted()||!args||!static_cast<void**>(args)[0])return HOOK_CONTINUE;
 auto* packet=mods::arg<J3DShapePacket*>(args,0);
 if(!packet)return HOOK_CONTINUE;
 auto* packet_model=packet->getModel();
 if(!packet_model)return HOOK_CONTINUE;
 auto* packet_data=packet_model->getModelData();
 if(!packet_data||packet_data->getShapeNum()!=3||packet_data->getMaterialNum()!=3)return HOOK_CONTINUE; // cheap E_OC2 signature filter
 for(unsigned i=0;i<table.count;++i){
  auto& r=table.regs[i];
  if(!r.used||r.fault||!r.admitted_id)continue;
  auto* base=fopAcM_SearchByID(r.admitted_id);
  if(!base||base->profname!=510||base->state.init_state!=2||fpcM_IsCreating(r.admitted_id))continue;
  auto* actor=static_cast<daE_OC_c*>(base);
  auto snapshot=record_snapshot(actor);
  if(!table.matches(i,snapshot)||!actor->mName||std::strcmp(actor->mName,"E_OC2")||fopAcM_GetID(actor)!=r.admitted_id)continue;
  if(!actor->mpMorf){reject_reg(static_cast<int>(i),"NO_NATIVE_MORF");return HOOK_CONTINUE;}
  auto* model=actor->mpMorf->getModel();
  if(!model||packet_model!=model)continue; // another actor, the accessory model, or the original E_OC2
  auto* data=model->getModelData();
  if(!data||!model->mShapePacket||!model->mMatPacket||data->getShapeNum()!=3||data->getMaterialNum()!=3||
   model->mDiffFlag!=0x11000084u){reject_reg(static_cast<int>(i),"MODEL_PACKET_OR_DIFF_CONTRACT");return HOOK_CONTINUE;}
  unsigned shape=99,material_index=99;
  if(packet==model->getShapePacket(0)){shape=0;material_index=0;}
  else if(packet==model->getShapePacket(2)){shape=2;material_index=1;}
  else return HOOK_CONTINUE; // eyes remain entirely native
  if(packet->checkFlag(J3DShpFlag_Hidden))return HOOK_CONTINUE;
  if(packet->getShape()!=data->getShapeNodePointer(static_cast<u16>(shape))||!packet->getShape()||
   packet->getShape()->getMaterial()!=data->getMaterialNodePointer(static_cast<u16>(material_index))||
   !black_material_contract(data)){reject_reg(static_cast<int>(i),"E_OC2_EFFECTIVE_RESOURCE_CONTRACT");return HOOK_CONTINUE;}
  auto* material=data->getMaterialNodePointer(static_cast<u16>(material_index));
  auto* mat_packet=model->getMatPacket(static_cast<u16>(material_index));
  auto* display=mat_packet->getDisplayListObj();
  if(mat_packet->getMaterial()!=material||!display||display->mSize>display->mMaxSize){reject_reg(static_cast<int>(i),"NATIVE_BASE_DISPLAY_LIST_OWNERSHIP");return HOOK_CONTINUE;}
  std::uint32_t baseline=0;
  if(!devf_black::read_base(display->getDisplayList(0),display->getDisplayListSize(),material->getColorBlock()->getMatColorOffset(),baseline)){
   reject_reg(static_cast<int>(i),"NATIVE_MATCOLOR0_XF_BOUNDS_HEADER");return HOOK_CONTINUE;
  }
  devf_black::Owner owner;owner.id=r.admitted_id;owner.admitted_id=r.admitted_id;owner.generation=r.generation;
  owner.record=owner.context=owner.eoc2=owner.created=owner.model_matches=owner.packet_matches=true;
  owner.shape=shape;owner.material=material_index;
  if(r.ready_generation!=r.generation){r.ready_generation=r.generation;blog("BLACK_READY",&r,owner.id,"READ_ONLY_EFFECTIVE_BODY_EYES_TEV_CONTRACT",devf_black::charcoal(baseline));}
  if(scope.begin(owner,reinterpret_cast<std::uintptr_t>(packet),baseline,black_emit))scope_reg=static_cast<int>(i);
  return HOOK_CONTINUE; // original packet draw always runs once; no native method replacement
 }
 return HOOK_CONTINUE;
}
void after_black_packet(ModContext*,void* args,void*,void*)noexcept{
 if(!scope.active)return;
 const auto key=args&&static_cast<void**>(args)[0]?reinterpret_cast<std::uintptr_t>(mods::arg<J3DShapePacket*>(args,0)):0;
 const auto selected=scope.id;const auto readback=scope.applied_word;const int index=scope_reg;
 if(!scope.end(key,black_emit)){reject_reg(index,scope.reason);return;}
 // No packet/actor/model dereference after the original returns; only saved scalar restoration words.
 if(index>=0&&unsigned(index)<table.count){
  auto& r=table.regs[index];
  if(r.applied_generation!=r.generation){r.applied_generation=r.generation;blog("BLACK_APPLIED",&r,selected,"PREFIX_NATIVE_GEOMETRY_SUFFIX_LOGICAL_ONLY",readback);}
 }
 scope_reg=-1;
}
ModResult install_hooks()noexcept{
 HookOptions post=HOOK_OPTIONS_INIT;post.priority=2147483647;
 HookOptions pre=HOOK_OPTIONS_INIT;pre.priority=(-2147483647-1);
 if(!svc_hook||svc_hook->header.struct_size<sizeof(HookService)||svc_hook->header.major_version!=1||svc_hook->header.minor_version<1||!svc_hook->install||!svc_hook->add_pre||!svc_hook->add_post)return MOD_UNSUPPORTED;
 auto result=mods::hook::add_pre<NativeEocExecute>(on_execute);
 if(result!=MOD_OK)return result;
 // Install POST first; no tint can be emitted until the pair and all initialization succeed.
 result=mods::hook::add_post<NativeShapePacketDraw>(after_black_packet,&post);
 if(result!=MOD_OK)return result;
 return mods::hook::add_pre<NativeShapePacketDraw>(before_black_packet,&pre);
}
void black_poll()noexcept{
 if(scope.active){reject_reg(scope_reg,"DRAW_SCOPE_NOT_CLOSED_BEFORE_UPDATE");return;}
 for(unsigned i=0;i<table.count;++i){
  auto& r=table.regs[i];if(!r.used||!r.admitted_id)continue;
  const auto* actor=fopAcM_SearchByID(r.admitted_id);
  if(!actor||actor->state.init_state==3){blog("BLACK_RELEASED",&r,r.admitted_id,"NATIVE_DESTRUCTION_NO_PRIVATE_GRAPH");r.admitted_id=0;}
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
  if(hooked!=MOD_OK){event("BLACK_HOOKS_REJECTED_ZERO_REQUESTS",8,0,hooked);return mods::set_error(error,hooked,"Black Bokoblin hooks rejected; zero requests; host detach rolls back this mod only");}
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
  for(unsigned i=0;i<table.count;++i)blog("BLACK_ARMED",&table.regs[i],0,"REGISTERED_WAITING_NATIVE_FIRST_EXECUTE",0,-1,table.regs[i].hp);
 }
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){black_poll();return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 table.armed=false;
 if(scope.active)scope.abort(black_emit,"SHUTDOWN_PENDING_SCOPE");
 for(unsigned i=0;i<table.count;++i)if(table.regs[i].admitted_id){blog("BLACK_RELEASED",&table.regs[i],table.regs[i].admitted_id,"SHUTDOWN_NO_OWNED_GRAPH_OR_ASSET");table.regs[i].admitted_id=0;}
 const auto r=client.shutdown();event(client.reason,8,0,r);return r==MOD_OK?MOD_OK:mods::set_error(error,r,"Expansion own edit removal failed; provider owner-detach fallback");
}
}
