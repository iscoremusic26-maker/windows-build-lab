#include <mods/service.hpp>
#include <mods/svc/log.h>
#include "editor_ui.hpp"
#include "broker.hpp"
#ifndef DEVF_SOURCE_SHA
#error Pin exact source SHA
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(FileService,svc_file);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(StageService,svc_stage);
IMPORT_OPTIONAL_SERVICE(UiService,svc_ui);
EXPORT_DEFERRED_SERVICE(devf_native_api,DEVF_NATIVE_SERVICE_ID,DEVF_NATIVE_MAJOR,DEVF_NATIVE_MINOR);
namespace {
native_config::Runtime core;editor7::Ui editor;devf_api::Broker broker;
extern "C" ModResult api_request(ModContext* c,const DevfNativePlacementRequest* r,DevfNativePlacementReceipt* out) noexcept {return broker.request(c,r,out);}
extern "C" ModResult api_remove(ModContext* c,uint64_t token) noexcept {return broker.remove(c,token);}
const DevfNativePlacementService native_service{{sizeof(DevfNativePlacementService),DEVF_NATIVE_MAJOR,DEVF_NATIVE_MINOR},api_request,api_remove};
void api_event(const char* event,const char* owner,const char* placement,uint64_t token,unsigned site,ModResult result) noexcept {
 if(!svc_log||!svc_log->write)return;char text[512]{};std::snprintf(text,sizeof(text),"DEVF_NATIVE_API event=%s owner=%.63s placement=%.31s token=%llu site=%u result=%d actor_id=NOT_OBSERVED evidence=STAGE_EDIT_ONLY human=HUMAN_PENDING",event,owner,placement,static_cast<unsigned long long>(token),site,int(result));
 try{svc_log->write(mod_ctx,result==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
void log(const char* event,ModResult result) noexcept {
 if(!native_qa::compatible(svc_log,1,0)||!svc_log->write)return;
 char text[640]{};
 std::snprintf(text,sizeof(text),"DEVF_CP07_1 event=%s id=dev.devf.cp06_native_config_dual version=0.7.1 source=%s result=%d configured=%u edits=%u actor_id=NOT_OBSERVED reason=%s ui=%s evidence=REGISTRATION_AND_UI_LOGICAL_ONLY human=HUMAN_RUNTIME_PENDING",
 event,DEVF_SOURCE_SHA,int(result),core.config.enabled_count,core.registration.ready?core.registration.expected:0,core.error.code,editor.reason);
 try{svc_log->write(mod_ctx,result==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 const auto result=core.initialize(mod_ctx,svc_host,svc_file,svc_stage);
 if(result!=MOD_OK){core.shutdown(mod_ctx,svc_stage);log("CORE_REGISTRATION_FAILED",result);return mods::set_error(error,result,"CP07 core initialization failed; own edits rollback/detach");}
 editor.initialize(mod_ctx,svc_host,svc_file,svc_ui);
 if(!broker.initialize(mod_ctx,svc_host,svc_stage,core,api_event)){editor.shutdown();core.shutdown(mod_ctx,svc_stage);return mods::set_error(error,MOD_UNSUPPORTED,"DEVF API lifecycle unavailable; no deferred publication");}
 const auto published=svc_host->publish_service(mod_ctx,DEVF_NATIVE_SERVICE_ID,DEVF_NATIVE_MAJOR,&native_service);
 if(published!=MOD_OK){broker.shutdown();editor.shutdown();core.shutdown(mod_ctx,svc_stage);return mods::set_error(error,published,"DEVF deferred service publication failed");}
 log("API_PUBLISHED_BOOTSTRAP_OPEN",MOD_OK);
 log(core.accepted?"CONFIG_ACCEPTED_STAGE_EDITS_REGISTERED":"CONFIG_REJECTED_NO_EDITS",MOD_OK);
 return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError* error){broker.close_admission();return broker.fault?mods::set_error(error,MOD_ERROR,"DEVF API fail-closed; host detach rolls back owned edits"):core.update();} // No UI IO, no reinjection
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 const auto api=broker.shutdown();const auto ui=editor.shutdown();const auto result=core.shutdown(mod_ctx,svc_stage);log("OWN_UI_AND_EDITS_REMOVED_NOT_LIVE_ACTORS",result!=MOD_OK?result:ui);
 if(result!=MOD_OK)return mods::set_error(error,result,"Own stage edit cleanup failed; host detach fallback");
 if(api!=MOD_OK)return mods::set_error(error,api,"API edit/watch cleanup failed; provider host detach fallback");
 // UI detach owns residual slots. Never alter actors or write config from shutdown.
 if(ui!=MOD_OK)return mods::set_error(error,ui,"Own UI cleanup failed; host UiService detach removes residual owned slots");
 return MOD_OK;
}
}
