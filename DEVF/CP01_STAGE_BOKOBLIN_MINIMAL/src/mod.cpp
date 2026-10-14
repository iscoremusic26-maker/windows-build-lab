#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/stage.h>
#include <d/d_stage.h>
#include <cstdio>
#include <cstring>
DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(StageService, svc_stage);
namespace {
bool initialized_once=false,shutdown_once=false;
StageActorHandle edit_handle=0;
constexpr unsigned ordinary_no_story_switches=0xFFFF0000u;
// Normal E_OC (high byte zero), 600-unit native home range (low byte six).
// Range is an AI threshold, never a hard movement boundary.
constexpr short normal_with_home_range_600=6;
template<class T>bool compatible(const T* p,unsigned major,unsigned minor){
 return p&&p->header.struct_size>=sizeof(T)&&p->header.major_version==major&&p->header.minor_version>=minor;
}
void report(const char* event,ModResult result) noexcept {
 if(!compatible(svc_log,1,0)||!svc_log->write)return;
 char line[384]{};
 std::snprintf(line,sizeof(line),"DEVF_STAGE_BOKOBLIN id=dev.devf.cp01_stage_bokoblin_minimal version=0.1.0 event=%s stage=F_SP121 room=3 layer=6 alias=E_oc params=FFFF0000 xyz=(-1250,-6304.0083,60750) angle=(6,0,0) setID=FFFF result=%d edit_handle=%llu registration_only=1 runtime_spawn=NOT_VERIFIED",event,int(result),static_cast<unsigned long long>(edit_handle));
 svc_log->write(mod_ctx,LOG_LEVEL_INFO,line);
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 if(initialized_once)return mods::set_error(err,MOD_CONFLICT,"Initialize already attempted; full exit required");
 initialized_once=true; // Before validation/service call; no retries, including partial failure/reentry.
 if(!mod_ctx||!compatible(svc_host,2,2)||!svc_host->version||
   (std::strcmp(svc_host->version,"2.0.3")&&std::strcmp(svc_host->version,"2.0.3.0"))||
   !compatible(svc_stage,1,0)||!svc_stage->add_actor||!compatible(svc_log,1,0)||!svc_log->write)
  return mods::set_error(err,MOD_UNSUPPORTED,"Dusklight2.0.3 Host2.2/Stage1.0/Log1.0 required");
 static_assert(sizeof(stage_actor_data_class)==0x20);
 const stage_actor_data_class record{
  .name="E_oc", .base={.parameters=ordinary_no_story_switches,
   .position={-1250.0f,-6304.0083f,60750.0f},.angle={normal_with_home_range_600,0,0},.setID=0xFFFF},
 };
 ModResult result=MOD_ERROR;
 try {result=svc_stage->add_actor(mod_ctx,"F_SP121",3,6,&record,sizeof(record),&edit_handle);}
 catch(...){report("REGISTER_EXCEPTION",MOD_ERROR);return mods::set_error(err,MOD_ERROR,"StageService registration exception; no retry");}
 if(result!=MOD_OK){report("REGISTER_REJECTED",result);return mods::set_error(err,result,"StageService rejected addition; no retry");}
 if(edit_handle==0){report("INVALID_HANDLE",MOD_ERROR);return mods::set_error(err,MOD_ERROR,"StageService returned invalid zero handle");}
 report("EDIT_REGISTERED",MOD_OK);return MOD_OK; // Never means actor initialized/rendered.
}
MOD_EXPORT ModResult mod_update(ModError*) {return MOD_OK;} // No calls or retries per frame.
MOD_EXPORT ModResult mod_shutdown(ModError*) {
 if(!shutdown_once){shutdown_once=true;report("SHUTDOWN_HOST_DETACH_REMOVES_EDIT_NOT_LIVE_ACTOR",MOD_OK);}
 return MOD_OK; // Host StageService.modDetached removes owned edits before DLL unloading.
}
}
