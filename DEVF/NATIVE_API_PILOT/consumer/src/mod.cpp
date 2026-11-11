#include <mods/service.hpp>
#include <mods/svc/log.h>
#include "probe.hpp"
#ifndef DEVF_SOURCE_SHA
#error Pin source
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(DevfNativePlacementService,svc_devf_native);
namespace {
devf_probe::Probe probe;
void report(const char* event,ModResult r) noexcept {
 if(!svc_log||!svc_log->write)return;char text[384]{};
 std::snprintf(text,sizeof(text),"DEVF_API_PROBE event=%s version=0.1.0 source=%s result=%d token=%llu reason=%s provider_service=dev.devf.native_placement@1.0 site=FIELD actor_id=NOT_OBSERVED evidence=API_STAGE_EDIT_ONLY human=HUMAN_PENDING",
 event,DEVF_SOURCE_SHA,int(r),static_cast<unsigned long long>(probe.token),probe.reason);
 try{svc_log->write(mod_ctx,r==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 const auto result=probe.initialize(mod_ctx,svc_host,svc_devf_native);report("REQUEST_RESULT",result);
 return result==MOD_OK?MOD_OK:mods::set_error(error,result,"DEVF probe request rejected; use provider JSON NONE, cold start; no fallback spawn");
}
MOD_EXPORT ModResult mod_update(ModError*){return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError* error){
 const auto result=probe.shutdown();report("SHUTDOWN_RESULT",result);
 return result==MOD_OK?MOD_OK:mods::set_error(error,result,"Probe edit removal failed; provider lifecycle watch/detach fallback");
}
}
