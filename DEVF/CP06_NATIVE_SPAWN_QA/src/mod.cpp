#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <cstdio>
#include <cstring>
#include "registration.hpp"
#ifndef DEVF_SOURCE_SHA
#error Source SHA must be pinned by the build
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(StageService,svc_stage);
namespace {
native_qa::Registration registration;bool log_failed=false;
#if DEVF_QA_MODE == 1
constexpr bool dual=false;constexpr const char* id="dev.devf.cp06_qa_north_native";constexpr const char* mode="A_NORTH_ONLY";
#else
static_assert(DEVF_QA_MODE==2);
constexpr bool dual=true;constexpr const char* id="dev.devf.cp06_qa_dual_native";constexpr const char* mode="B_DUAL_CONDITIONED_ON_A";
#endif
void log_event(const char* event,ModResult result) noexcept {
 if(!native_qa::compatible(svc_log,1,0)||!svc_log->write){log_failed=true;return;}
 char msg[768]{};
 std::snprintf(msg,sizeof(msg),"DEVF_NATIVE_QA id=%s version=0.6.1 mode=%s source=%s event=%s result=%d stage=F_SP121 room=3 layer=6 records=%u edit0=%llu edit1=%llu north_x=-1250 north_y=%.9f north_z=61950 reason=%s cleanup_result=%d actor_id=NOT_OBSERVED evidence=STAGE_EDIT_ONLY human=NOT_TESTED install=ALONE_COLD_START",
 id,mode,DEVF_SOURCE_SHA,event,int(result),registration.expected,
 static_cast<unsigned long long>(registration.handles[0]),static_cast<unsigned long long>(registration.handles[1]),
 double(native_qa::north_spawn),registration.reason,int(registration.cleanup_result));
 try{svc_log->write(mod_ctx,result==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,msg);}catch(...){log_failed=true;}
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 if(!mod_ctx||!native_qa::compatible(svc_host,2,2)||!svc_host->version||
  (std::strcmp(svc_host->version,"2.0.3")&&std::strcmp(svc_host->version,"2.0.3.0"))||
  !native_qa::compatible(svc_log,1,0)||!svc_log->write)
  return mods::set_error(err,MOD_UNSUPPORTED,"Native QA requires official Dusklight2.0.3 Host2.2/Log1.0/Stage1.0");
 const auto result=registration.initialize(mod_ctx,svc_stage,dual);
 log_event(result==MOD_OK?"STAGE_EDIT_REGISTERED":"REGISTRATION_REJECTED",result);
 if(result!=MOD_OK||log_failed){
  registration.cleanup(mod_ctx,svc_stage);
  return mods::set_error(err,result==MOD_OK?MOD_ERROR:result,"Native QA rejected; only own edits rolled back, host detach fallback; no actor observation");
 }
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){return MOD_OK;} // Never register/create from a frame callback.
MOD_EXPORT ModResult mod_shutdown(ModError* err){
 const auto result=registration.cleanup(mod_ctx,svc_stage);
 log_event("OWN_EDITS_REMOVED_NOT_LIVE_ACTOR_DELETION",result);
 return result==MOD_OK?MOD_OK:mods::set_error(err,result,"Own edit cleanup failed; host detach removes this mod's edits, not live actors");
}
}
