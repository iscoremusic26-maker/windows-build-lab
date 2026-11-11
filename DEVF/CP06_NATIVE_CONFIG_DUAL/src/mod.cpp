#include <mods/service.hpp>
#include <mods/svc/log.h>
#include <cstdio>
#include "runtime.hpp"
#ifndef DEVF_SOURCE_SHA
#error Source SHA must be pinned
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(FileService,svc_file);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(StageService,svc_stage);
namespace {
native_config::Runtime runtime;
constexpr const char* id="dev.devf.cp06_native_config_dual";
bool log_event(const char* event,ModResult result) noexcept {
 if(!native_qa::compatible(svc_log,1,0)||!svc_log->write)return false;
 char line[768]{};
 std::snprintf(line,sizeof(line),"DEVF_CP06_JSON_%s id=%s version=0.6.2 source=%s schema=1 count=%u enabled=%u mode=NATIVE_ONLY stage=F_SP121 room=3 layer=6 field_id=%s north_id=%s edit0=%llu edit1=%llu result=%d reason=%s active=%u no_actor_registration=%s actor_id=NOT_OBSERVED evidence=STAGE_EDIT_ONLY gameplay=HUMAN_RUNTIME_PENDING cleanup_result=%d",
 event,id,DEVF_SOURCE_SHA,runtime.config.count,runtime.config.enabled_count,
 runtime.config.id_for(0),runtime.config.id_for(1),
 static_cast<unsigned long long>(runtime.registration.handles[0]),static_cast<unsigned long long>(runtime.registration.handles[1]),
 int(result),runtime.error.code,runtime.registration.ready?runtime.registration.expected:0,
 !runtime.registration.attempted||runtime.registration.expected==0?"true":"false",int(runtime.registration.cleanup_result));
 try{svc_log->write(mod_ctx,result==MOD_OK&&runtime.accepted?LOG_LEVEL_INFO:LOG_LEVEL_WARN,line);return true;}catch(...){return false;}
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 if(!native_qa::compatible(svc_log,1,0)||!svc_log->write)
  return mods::set_error(err,MOD_UNSUPPORTED,"JSON native QA requires LogService1.0; no edits");
 const auto result=runtime.initialize(mod_ctx,svc_host,svc_file,svc_stage);
 bool logged=log_event(runtime.accepted?"CONFIG_ACCEPTED":result==MOD_OK?"CONFIG_REJECTED":"REGISTRATION_REJECTED",result);
 if(result==MOD_OK&&runtime.accepted)logged=log_event("STAGE_EDIT_REGISTERED",MOD_OK)&&logged;
 if(result!=MOD_OK||!logged){
  runtime.shutdown(mod_ctx,svc_stage);
  return mods::set_error(err,result==MOD_OK?MOD_ERROR:result,"Native JSON QA initialization failed; own edit rollback and host detach, no live-actor deletion");
 }
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){return runtime.update();}
MOD_EXPORT ModResult mod_shutdown(ModError* err){
 const auto result=runtime.shutdown(mod_ctx,svc_stage);log_event("OWN_EDITS_REMOVED_NOT_LIVE_ACTOR_DELETION",result);
 return result==MOD_OK?MOD_OK:mods::set_error(err,result,"Own edit cleanup failed; detach fallback removes edits, not actors");
}
}
