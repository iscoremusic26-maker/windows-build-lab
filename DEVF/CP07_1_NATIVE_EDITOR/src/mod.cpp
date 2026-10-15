#include <mods/service.hpp>
#include <mods/svc/log.h>
#include "editor_ui.hpp"
#ifndef DEVF_SOURCE_SHA
#error Pin exact source SHA
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(FileService,svc_file);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(StageService,svc_stage);
IMPORT_OPTIONAL_SERVICE(UiService,svc_ui);
namespace {
native_config::Runtime core;editor7::Ui editor;
void log(const char* event,ModResult result) noexcept {
 if(!native_qa::compatible(svc_log,1,0)||!svc_log->write)return;
 char text[640]{};
 std::snprintf(text,sizeof(text),"DEVF_CP07_1 event=%s id=dev.devf.cp06_native_config_dual version=0.7.0 source=%s result=%d configured=%u edits=%u actor_id=NOT_OBSERVED reason=%s ui=%s evidence=REGISTRATION_AND_UI_LOGICAL_ONLY human=HUMAN_RUNTIME_PENDING",
 event,DEVF_SOURCE_SHA,int(result),core.config.enabled_count,core.registration.ready?core.registration.expected:0,core.error.code,editor.reason);
 try{svc_log->write(mod_ctx,result==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 const auto result=core.initialize(mod_ctx,svc_host,svc_file,svc_stage);
 if(result!=MOD_OK){core.shutdown(mod_ctx,svc_stage);log("CORE_REGISTRATION_FAILED",result);return mods::set_error(error,result,"CP07 core initialization failed; own edits rollback/detach");}
 editor.initialize(mod_ctx,svc_host,svc_file,svc_ui);
 log(core.accepted?"CONFIG_ACCEPTED_STAGE_EDITS_REGISTERED":"CONFIG_REJECTED_NO_EDITS",MOD_OK);
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){return core.update();} // No UI IO, no reinjection
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 const auto ui=editor.shutdown();const auto result=core.shutdown(mod_ctx,svc_stage);log("OWN_UI_AND_EDITS_REMOVED_NOT_LIVE_ACTORS",result);
 if(result!=MOD_OK)return mods::set_error(error,result,"Own stage edit cleanup failed; host detach fallback");
 // UI detach owns residual slots. Never alter actors or write config from shutdown.
 (void)ui;return MOD_OK;
}
}
