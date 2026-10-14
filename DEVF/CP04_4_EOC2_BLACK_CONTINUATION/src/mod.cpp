#include <mods/service.hpp>
#include <mods/svc/hook.hpp>
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_manager.h>
#include <cstdio>
#include <type_traits>
#include "registration.hpp"
#include "xf_scope.hpp"
#include <JSystem/J3DGraphBase/J3DShape.h>
#include <JSystem/J3DGraphAnimator/J3DModelData.h>
#include <JSystem/J3DGraphBase/J3DMaterial.h>
#include <JSystem/J3DGraphBase/J3DTexture.h>
#include "observer.hpp"
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(StageService,svc_stage);
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
devf::Policy policy;devf::Registration registration;devf::Observer observer;bool shutdown_once=false;
void black_admit(u32) noexcept;ModResult install_black() noexcept;void black_poll() noexcept;void black_shutdown() noexcept;
void report(const char* event,const char* reason,u32 id,int before,int after,ModResult result) noexcept {
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 char line[384]{};
 std::snprintf(line,sizeof(line),"DEVF_CP03_HP id=dev.devf.cp04_eoc2_black version=0.4.4 event=%s actor_id=%u stage=F_SP121 room=3 layer=6 old_hp=%d new_hp=%d phase=%s result=%d edit_handle=%llu reason=%s evidence=%s combat=NOT_VERIFIED",event,id,before,after,!std::strcmp(event,"HP_APPLIED")?"PRE_FIRST_EXECUTE":"NO_HP_WRITE",int(result),static_cast<unsigned long long>(registration.edit),reason,!std::strcmp(event,"HP_APPLIED")?"FIELD_READBACK":"NO_HP_WRITE");
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);}catch(...){policy.reject("LOG_EXCEPTION");}
}
void emit_observation(const devf::Observation& e) noexcept {
 if(e.kind==devf::EventKind::Rejected)policy.reject(e.reason);
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 const char* event=e.kind==devf::EventKind::Delta?"HP_DELTA":e.kind==devf::EventKind::Increase?"HP_INCREASE_OBSERVED":
  e.kind==devf::EventKind::Suppressed?"HP_DELTA_SUPPRESSED":e.kind==devf::EventKind::Summary?"HP_SUMMARY":"HP_DIAG_REJECTED";
 char line[512]{};const int change=e.after-e.before,loss=change<0?-change:0;
 std::snprintf(line,sizeof(line),"DEVF_CP03_HP id=dev.devf.cp04_eoc2_black version=0.4.4 event=%s actor_id=%u hp_before=%d hp_after=%d observed_hp_loss=%d hp_change=%+d ordinal=%llu emitted=%u source=%s classification=%s reason=%s attack=UNKNOWN raw_attack_power=NOT_MEASURED",event,e.id,e.before,e.after,loss,change,static_cast<unsigned long long>(e.ordinal),e.emitted,e.source,e.classification,e.reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);}catch(...){policy.reject("LOG_EXCEPTION");observer.fault=true;observer.active=false;}
}
devf::Snapshot record_snapshot(daE_OC_c* a) noexcept {
 devf::Snapshot s;s.profile=a->profname;s.room=fopAcM_GetRoomNo(a);s.parameters=fopAcM_GetParam(a);
 s.set_id=a->setID;s.x=a->home.pos.x;s.y=a->home.pos.y;s.z=a->home.pos.z;
 s.home_x=a->home.angle.x;s.home_y=a->home.angle.y;s.home_z=a->home.angle.z;return s;
}
HookAction on_execute(ModContext*,void* args,void*,void*) noexcept {
 // Paired sentinels for ignored/nested E_OC calls; fixed stack of IDs and HP only.
 if(!observer.push(emit_observation))return HOOK_CONTINUE;
 if(!policy.armed||policy.fault)return HOOK_CONTINUE;
 if(!args||!static_cast<void**>(args)[0])return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a)return HOOK_CONTINUE;
 auto s=record_snapshot(a);
 if(!devf::record_matches(s))return HOOK_CONTINUE;
 const char* stage=dComIfGp_getStartStageName();
 s.stage=stage&&std::strncmp(stage,"F_SP121",8)==0;s.layer=dComIfG_play_c::getLayerNo(0);
 if(!devf::context_matches(s))return HOOK_CONTINUE;
 s.id=fopAcM_GetID(a);s.health=a->health;
 s.initializing=a->state.init_state>=0&&a->state.init_state<2&&a->current.angle.x==0&&fpcM_IsCreating(s.id);
 s.resource_eoc2=a->mName&&std::strcmp(a->mName,"E_OC2")==0;
 // Query only the previously verified ID, never a census/profile/proximity search.
 // The pointer is used in this stack frame and never retained.
 if(policy.last_id&&policy.last_id!=s.id){
  const auto* previous=fopAcM_SearchByID(policy.last_id);
  s.previous_live=previous&&previous->state.init_state!=3;
 }
 const auto decision=policy.inspect(s);
 if(decision==devf::Decision::Applied){
  a->health=devf::kHp; // The only actor-field write in this module. field_0x560 remains native.
  report("HP_APPLIED","EXCLUSIVE_FIRST_CREATE",s.id,s.health,a->health,MOD_OK);
  if(!policy.fault)black_admit(s.id);
  if(!policy.fault)observer.start(s.id,a->health,emit_observation); // Baseline is 150, never false loss70.
 }else if(decision==devf::Decision::Fault){
  report("HP_REJECTED",policy.reason,s.id,s.health,s.health,MOD_ERROR);
  observer.active=false;
 }
 if(policy.armed&&!policy.fault&&s.id==policy.last_id&&s.resource_eoc2&&a->state.init_state>=0&&a->state.init_state<3)
  observer.capture(s.id,a->health,emit_observation);
 return HOOK_CONTINUE; // Original execute/action/damage/return value always preserved.
}
void after_execute(ModContext*,void* args,void*,void*) noexcept {
 const auto frame=observer.pending();
 if(!frame.eligible||!policy.armed||policy.fault){if(policy.fault)observer.active=false;observer.complete(0,0,false,emit_observation);return;}
 if(!args||!static_cast<void**>(args)[0]){observer.complete(0,0,false,emit_observation);return;}
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a){observer.complete(0,0,false,emit_observation);return;}
 // Official fopAcM_delete only queues. fpcDt_Handler frees outside execute, never here.
 // State3 is queued deletion, still allocated through this original-call stack frame.
 const auto s=record_snapshot(a);const auto id=fopAcM_GetID(a);
 const bool matches=id==frame.id&&devf::record_matches(s)&&a->mName&&std::strcmp(a->mName,"E_OC2")==0&&
  a->state.init_state>=0&&a->state.init_state<=3;
 observer.complete(id,matches?a->health:0,matches,emit_observation);
}
ModResult install_pre(HookPreFn fn){return mods::hook::add_pre<NativeEocExecute>(fn);}
ModResult install_post(HookPostFn fn){return mods::hook::add_post<NativeEocExecute>(fn);}
ModResult remove_hook(){return mods::hook::uninstall<NativeEocExecute>();}
}

namespace {
devf_black::Scope black_scope;
std::uint64_t black_generation=0,black_ready_generation=0,black_applied_generation=0;
std::uint32_t black_owner_id=0;
bool black_rejection_logged=false;
void black_emit(const devf_black::Command& command) noexcept {GXCallDisplayList(command.data(),static_cast<u32>(command.size()));}
void black_log(const char* event,const char* reason,u32 id,std::uint32_t value=0) noexcept {
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 char line[384]{};
 std::snprintf(line,sizeof(line),"DEVF_CP04_BLACK id=dev.devf.cp04_eoc2_black version=0.4.4 event=%s actor_id=%u generation=%llu mechanism=PRIVATE_FIFO_XF_MATCOLOR0 rgb_word=%08X reason=%s evidence=COMMAND_WORD_READBACK visual=HUMAN_RUNTIME_NOT_TESTED",event,id,static_cast<unsigned long long>(black_generation),value,reason);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);}catch(...){policy.reject("BLACK_LOG_EXCEPTION");}
}
void black_reject(const char* reason) noexcept {
 if(black_scope.active)black_scope.abort(black_emit,reason);
 policy.reject(reason);
 if(!black_rejection_logged){black_rejection_logged=true;black_log("CP04_BLACK_REJECTED",reason,black_owner_id);}
}
void black_admit(u32 id) noexcept {
 if(black_owner_id)black_log("CP04_BLACK_RELEASED","NEW_NATIVE_GENERATION_NO_OWNED_RESOURCE",black_owner_id);
 ++black_generation;black_owner_id=id;black_ready_generation=black_applied_generation=0;
 black_log("CP04_BLACK_PREFLIGHT","ADMITTED_HP150_WAITING_NATIVE_PACKET",id);
}
bool black_material_contract(J3DModelData* data) noexcept {
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
HookAction before_black_packet(ModContext*,void* args,void*,void*) noexcept {
 // The pinned native packet method is non-recursive. Never scan actors or retain model pointers.
 if(black_scope.active){black_reject("NESTED_NATIVE_PACKET_NOT_SUPPORTED");return HOOK_CONTINUE;}
 if(!policy.armed||policy.fault||!black_owner_id||!args||!static_cast<void**>(args)[0])return HOOK_CONTINUE;
 auto* packet=mods::arg<J3DShapePacket*>(args,0);
 if(!packet)return HOOK_CONTINUE;
 auto* base=fopAcM_SearchByID(policy.last_id);
 if(!base||base->profname!=510||base->state.init_state!=2||fpcM_IsCreating(policy.last_id))return HOOK_CONTINUE;
 auto* actor=static_cast<daE_OC_c*>(base);
 auto snapshot=record_snapshot(actor);const char* scene=dComIfGp_getStartStageName();
 snapshot.stage=scene&&std::strncmp(scene,"F_SP121",8)==0;snapshot.layer=dComIfG_play_c::getLayerNo(0);
 if(!devf::record_matches(snapshot)||!devf::context_matches(snapshot)||!actor->mName||std::strcmp(actor->mName,"E_OC2")||
  fopAcM_GetID(actor)!=black_owner_id||policy.last_id!=black_owner_id)return HOOK_CONTINUE;
 if(!actor->mpMorf){black_reject("NO_NATIVE_MORF");return HOOK_CONTINUE;}
 auto* model=actor->mpMorf->getModel();
 if(!model||packet->getModel()!=model)return HOOK_CONTINUE; // includes original E_OC2 and accessory mpModel
 auto* data=model->getModelData();
 if(!data||!model->mShapePacket||!model->mMatPacket||data->getShapeNum()!=3||data->getMaterialNum()!=3||
  model->mDiffFlag!=0x11000084u){black_reject("MODEL_PACKET_OR_DIFF_CONTRACT");return HOOK_CONTINUE;}
 unsigned shape=99,material_index=99;
 if(packet==model->getShapePacket(0)){shape=0;material_index=0;}
 else if(packet==model->getShapePacket(2)){shape=2;material_index=1;}
 else return HOOK_CONTINUE; // eyes remain entirely native
 if(packet->checkFlag(J3DShpFlag_Hidden))return HOOK_CONTINUE;
 if(packet->getShape()!=data->getShapeNodePointer(static_cast<u16>(shape))||!packet->getShape()||
  packet->getShape()->getMaterial()!=data->getMaterialNodePointer(static_cast<u16>(material_index))||
  !black_material_contract(data)){black_reject("E_OC2_EFFECTIVE_RESOURCE_CONTRACT");return HOOK_CONTINUE;}
 auto* material=data->getMaterialNodePointer(static_cast<u16>(material_index));
 auto* mat_packet=model->getMatPacket(static_cast<u16>(material_index));
 auto* display=mat_packet->getDisplayListObj();
 if(mat_packet->getMaterial()!=material||!display||display->mSize>display->mMaxSize){
  black_reject("NATIVE_BASE_DISPLAY_LIST_OWNERSHIP");return HOOK_CONTINUE;
 }
 std::uint32_t baseline=0;
 if(!devf_black::read_base(display->getDisplayList(0),display->getDisplayListSize(),
  material->getColorBlock()->getMatColorOffset(),baseline)){
  black_reject("NATIVE_MATCOLOR0_XF_BOUNDS_HEADER");return HOOK_CONTINUE;
 }
 // This exact native diff mask cannot subsequently override MATCOLOR0 in the shape DL.
 devf_black::Owner owner;owner.id=black_owner_id;owner.admitted_id=policy.last_id;owner.generation=black_generation;
 owner.record=owner.context=owner.eoc2=owner.created=owner.model_matches=owner.packet_matches=true;
 owner.shape=shape;owner.material=material_index;
 if(black_ready_generation!=black_generation){
  black_ready_generation=black_generation;black_log("CP04_BLACK_READY","READ_ONLY_EFFECTIVE_BODY_EYES_TEV_CONTRACT",owner.id,devf_black::charcoal(baseline));
 }
 if(!policy.armed||policy.fault)return HOOK_CONTINUE;
 black_scope.begin(owner,reinterpret_cast<std::uintptr_t>(packet),baseline,black_emit);
 return HOOK_CONTINUE; // Original packet draw always runs once; no native method replacement.
}
void after_black_packet(ModContext*,void* args,void*,void*) noexcept {
 if(!black_scope.active)return;
 const auto key=args&&static_cast<void**>(args)[0]?reinterpret_cast<std::uintptr_t>(mods::arg<J3DShapePacket*>(args,0)):0;
 const auto selected=black_scope.id;const auto readback=black_scope.applied_word;
 if(!black_scope.end(key,black_emit)){black_reject(black_scope.reason);return;}
 // No packet/actor/model dereference after original returns; only saved scalar restoration words.
 if(black_applied_generation!=black_generation){
  black_applied_generation=black_generation;black_log("CP04_BLACK_APPLIED","PREFIX_NATIVE_GEOMETRY_SUFFIX_LOGICAL_ONLY",selected,readback);
 }
}
ModResult install_black() noexcept {
 HookOptions post=HOOK_OPTIONS_INIT;post.priority=2147483647;
 HookOptions pre=HOOK_OPTIONS_INIT;pre.priority=(-2147483647-1);
 // Install POST first; no tint can be emitted until the pair and all initialization succeed.
 auto result=mods::hook::add_post<NativeShapePacketDraw>(after_black_packet,&post);
 if(result!=MOD_OK)return result;
 result=mods::hook::add_pre<NativeShapePacketDraw>(before_black_packet,&pre);
 return result;
}
void black_poll() noexcept {
 if(black_scope.active){black_reject("DRAW_SCOPE_NOT_CLOSED_BEFORE_UPDATE");return;}
 if(black_owner_id){
  const auto* actor=fopAcM_SearchByID(black_owner_id);
  if(!actor||actor->state.init_state==3){
   black_log("CP04_BLACK_RELEASED","NATIVE_DESTRUCTION_NO_PRIVATE_GRAPH",black_owner_id);
   black_owner_id=0;
  }
 }
}
void black_shutdown() noexcept {
 if(black_scope.active)black_scope.abort(black_emit,"SHUTDOWN_PENDING_SCOPE");
 if(black_owner_id){black_log("CP04_BLACK_RELEASED","SHUTDOWN_NO_OWNED_GRAPH_OR_ASSET",black_owner_id);black_owner_id=0;}
}
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 const auto result=registration.initialize(mod_ctx,svc_host,svc_log,svc_stage,svc_hook,install_pre,install_post,remove_hook,on_execute,after_execute);
 if(result!=MOD_OK){policy.armed=false;report("INIT_REJECTED","HOST_DETACH_ROLLS_BACK",0,-1,-1,result);return mods::set_error(err,result,"CP03 hook/stage initialization failed; no retry; host detach required");}
 const auto visual=install_black();
 if(visual!=MOD_OK){policy.armed=false;return mods::set_error(err,visual,"CP04 private packet hook pair rejected; host detach rolls back this mod only");}
 policy.armed=true;report("EDIT_REGISTERED","HP_NOT_YET_APPLIED",0,-1,-1,MOD_OK);return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError* err){
 // No HP write, registration or retries here. A hook rejection is consumed outside dispatch.
 black_poll();
 return policy.fault?mods::set_error(err,MOD_ERROR,"CP03 first-execute/identity rejected; full exit and inspect log"):MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*){
 policy.armed=false;
 black_shutdown();
 observer.shutdown(emit_observation);
 if(!shutdown_once){shutdown_once=true;report("SHUTDOWN","HOST_DETACH_REMOVES_HOOK_AND_EDIT_NOT_LIVE_ACTOR",0,-1,-1,MOD_OK);}
 // HookService.modDetached owns callback/trampoline removal; StageService removes the edit.
 // No manual hot-unload, no actor deletion and no pending ActorService callback.
 return MOD_OK;
}
}
