#include <mods/service.hpp>
#include <mods/svc/hook.hpp>
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_manager.h>
#include <cstdio>
#include <type_traits>
#include "registration.hpp"
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
devf::Policy policy;devf::Registration registration;bool shutdown_once=false;
void report(const char* event,const char* reason,u32 id,int before,int after,ModResult result) noexcept {
 if(!devf::compatible(svc_log,1,0)||!svc_log->write)return;
 char line[384]{};
 std::snprintf(line,sizeof(line),"DEVF_CP03_HP id=dev.devf.cp03_stage_hook_hp150 version=0.3.0 event=%s actor_id=%u stage=F_SP121 room=3 layer=6 old_hp=%d new_hp=%d phase=%s result=%d edit_handle=%llu reason=%s evidence=%s combat=NOT_VERIFIED",event,id,before,after,!std::strcmp(event,"HP_APPLIED")?"PRE_FIRST_EXECUTE":"NO_HP_WRITE",int(result),static_cast<unsigned long long>(registration.edit),reason,!std::strcmp(event,"HP_APPLIED")?"FIELD_READBACK":"NO_HP_WRITE");
 try{svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);}catch(...){policy.reject("LOG_EXCEPTION");}
}
HookAction on_execute(ModContext*,void* args,void*,void*) noexcept {
 if(!policy.armed||policy.fault)return HOOK_CONTINUE;
 if(!args||!static_cast<void**>(args)[0])return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a)return HOOK_CONTINUE;
 devf::Snapshot s;
 s.profile=a->profname;s.room=fopAcM_GetRoomNo(a);s.parameters=fopAcM_GetParam(a);
 s.set_id=a->setID;s.x=a->home.pos.x;s.y=a->home.pos.y;s.z=a->home.pos.z;
 s.home_x=a->home.angle.x;s.home_y=a->home.angle.y;s.home_z=a->home.angle.z;
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
 }else if(decision==devf::Decision::Fault){
  report("HP_REJECTED",policy.reason,s.id,s.health,s.health,MOD_ERROR);
 }
 return HOOK_CONTINUE; // Original execute/action/damage/return value always preserved.
}
ModResult install_pre(HookPreFn fn){return mods::hook::add_pre<NativeEocExecute>(fn);}
ModResult remove_hook(){return mods::hook::uninstall<NativeEocExecute>();}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 const auto result=registration.initialize(mod_ctx,svc_host,svc_log,svc_stage,svc_hook,install_pre,remove_hook,on_execute);
 if(result!=MOD_OK){policy.armed=false;report("INIT_REJECTED","HOST_DETACH_ROLLS_BACK",0,-1,-1,result);return mods::set_error(err,result,"CP03 hook/stage initialization failed; no retry; host detach required");}
 policy.armed=true;report("EDIT_REGISTERED","HP_NOT_YET_APPLIED",0,-1,-1,MOD_OK);return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError* err){
 // No HP write, registration or retries here. A hook rejection is consumed outside dispatch.
 return policy.fault?mods::set_error(err,MOD_ERROR,"CP03 first-execute/identity rejected; full exit and inspect log"):MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*){
 policy.armed=false;
 if(!shutdown_once){shutdown_once=true;report("SHUTDOWN","HOST_DETACH_REMOVES_HOOK_AND_EDIT_NOT_LIVE_ACTOR",0,-1,-1,MOD_OK);}
 // HookService.modDetached owns callback/trampoline removal; StageService removes the edit.
 // No manual hot-unload, no actor deletion and no pending ActorService callback.
 return MOD_OK;
}
}
