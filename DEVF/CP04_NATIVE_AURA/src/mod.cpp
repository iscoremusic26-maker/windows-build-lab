#include <mods/service.hpp>
#include <mods/svc/hook.hpp>
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_manager.h>
#include <cstdio>
#include <type_traits>
#include "registration.hpp"
#include "observer.hpp"
#include "particle_access.hpp"
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(StageService,svc_stage);
IMPORT_SERVICE(HookService,svc_hook);
DEFINE_HOOK(&daE_OC_c::execute,NativeEocExecute);
static_assert(std::is_same_v<decltype(&daE_OC_c::execute),int(daE_OC_c::*)()>);
static_assert(sizeof(decltype(&daE_OC_c::execute))==8,"Reviewed Windows x64 compact PMF required");
static_assert(std::is_same_v<decltype(daE_OC_c::health),s16>);
namespace {
devf::Policy policy;devf::Registration registration;devf::Observer observer;devf::Visual visual;devf::ParticleAccess particles;bool shutdown_once=false;
void visual_event(const char* event,u32 id,u32 key,const char* reason,const char* evidence)noexcept{
 if(!std::strcmp(event,"CP04_VISUAL_REJECTED"))policy.reject(reason);
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 char line[384]{};
 std::snprintf(line,sizeof(line),"DEVF_CP04_VISUAL id=dev.devf.cp04_native_aura version=0.4.0 event=%s actor_id=%u emitter_key=%u technique=NATIVE_PARTICLE_MARKER resource=0x029D reason=%s evidence=%s appearance=HUMAN_RUNTIME_NOT_TESTED",event,id,key,reason,evidence);
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);}catch(...){policy.reject("LOG_EXCEPTION");}
}
void report(const char* event,const char* reason,u32 id,int before,int after,ModResult result) noexcept {
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 char line[384]{};
 std::snprintf(line,sizeof(line),"DEVF_CP04_HP id=dev.devf.cp04_native_aura version=0.4.0 event=%s actor_id=%u stage=F_SP121 room=3 layer=6 old_hp=%d new_hp=%d phase=%s result=%d edit_handle=%llu reason=%s evidence=%s combat=NOT_VERIFIED",event,id,before,after,!std::strcmp(event,"HP_APPLIED")?"PRE_FIRST_EXECUTE":"NO_HP_WRITE",int(result),static_cast<unsigned long long>(registration.edit),reason,!std::strcmp(event,"HP_APPLIED")?"FIELD_READBACK":"NO_HP_WRITE");
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);}catch(...){policy.reject("LOG_EXCEPTION");}
}
void emit_observation(const devf::Observation& e) noexcept {
 if(e.kind==devf::EventKind::Rejected)policy.reject(e.reason);
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 const char* event=e.kind==devf::EventKind::Delta?"HP_DELTA":e.kind==devf::EventKind::Increase?"HP_INCREASE_OBSERVED":
  e.kind==devf::EventKind::Suppressed?"HP_DELTA_SUPPRESSED":e.kind==devf::EventKind::Summary?"HP_SUMMARY":"HP_DIAG_REJECTED";
 char line[512]{};const int change=e.after-e.before,loss=change<0?-change:0;
 std::snprintf(line,sizeof(line),"DEVF_CP04_HP id=dev.devf.cp04_native_aura version=0.4.0 event=%s actor_id=%u hp_before=%d hp_after=%d observed_hp_loss=%d hp_change=%+d ordinal=%llu emitted=%u source=%s classification=%s reason=%s attack=UNKNOWN raw_attack_power=NOT_MEASURED",event,e.id,e.before,e.after,loss,change,static_cast<unsigned long long>(e.ordinal),e.emitted,e.source,e.classification,e.reason);
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
 if(matches&&!policy.fault){
  devf::VisualProbe probe;probe.id=id;probe.eligible=id==policy.last_id;probe.live=a->state.init_state<3&&a->health>0;
  probe.model_ready=a->mpMorf&&a->mpMorf->getModel()&&a->mpModel&&a->mpMorf->getModel()->getUserArea()==reinterpret_cast<uintptr_t>(a);
  // eyePos is the native current joint position computed by cc_set, copied into the emitter.
  probe.x=a->eyePos.x;probe.y=a->eyePos.y;probe.z=a->eyePos.z;
  visual.step(probe,particles,visual_event);
 }
}
ModResult install_pre(HookPreFn fn){return mods::hook::add_pre<NativeEocExecute>(fn);}
ModResult install_post(HookPostFn fn){return mods::hook::add_post<NativeEocExecute>(fn);}
ModResult remove_hook(){return mods::hook::uninstall<NativeEocExecute>();}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 const auto result=registration.initialize(mod_ctx,svc_host,svc_log,svc_stage,svc_hook,install_pre,install_post,remove_hook,on_execute,after_execute);
 if(result!=MOD_OK){policy.armed=false;report("INIT_REJECTED","HOST_DETACH_ROLLS_BACK",0,-1,-1,result);return mods::set_error(err,result,"CP04 hook/stage initialization failed; no retry; host detach required");}
 policy.armed=true;report("EDIT_REGISTERED","HP_NOT_YET_APPLIED",0,-1,-1,MOD_OK);
 visual_event("CP04_VISUAL_ARMED",0,0,"ONE_STAGE_EDIT_AND_EXISTING_EXECUTE_HOOK","RESOURCE_GATE_PENDING_NO_EMITTER");return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError* err){
 // No HP write, registration or retries here. A hook rejection is consumed outside dispatch.
 return policy.fault?mods::set_error(err,MOD_ERROR,"CP04 first-execute/identity rejected; full exit and inspect log"):MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*){
 policy.armed=false;
 visual.shutdown(visual_event);
 observer.shutdown(emit_observation);
 if(!shutdown_once){shutdown_once=true;report("SHUTDOWN","HOST_DETACH_REMOVES_HOOK_AND_EDIT_NOT_LIVE_ACTOR",0,-1,-1,MOD_OK);}
 // HookService.modDetached owns callback/trampoline removal; StageService removes the edit.
 // No manual hot-unload, no actor deletion and no pending ActorService callback.
 return MOD_OK;
}
}
