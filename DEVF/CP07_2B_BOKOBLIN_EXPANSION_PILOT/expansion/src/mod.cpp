#include <mods/service.hpp>
#include <mods/svc/log.h>
#include "client.hpp"
#ifndef DEVF_SOURCE_SHA
#error Exact source required
#endif
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(FileService,svc_file);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(DevfEncounterPlacementService,svc_devf_encounter);
namespace {
expansion::Client client;const char* mode_reason="NOT_READ";
void event(const char* e,unsigned i,uint64_t token,ModResult r)noexcept{
 if(!svc_log||!svc_log->write)return;char text[640]{};const auto* s=i<expansion::captured.size()?&expansion::captured[i]:nullptr;
 std::snprintf(text,sizeof(text),"BOKOBLIN_EXPANSION event=%s owner=dev.bokoblin.expansion encounter_id=%s stage=F_SP121 room=3 layer=6 xyz=(%.9g,%.9g,%.9g) yaw=%d ticket=%llu result=%d actor_id=NOT_OBSERVED evidence=STAGE_EDIT_ONLY terrain=NOT_VERIFIED source=%s",e,s?s->id:"NONE",s?double(s->x):0.,s?double(s->y):0.,s?double(s->z):0.,s?s->yaw:0,static_cast<unsigned long long>(token),int(r),DEVF_SOURCE_SHA);
 try{svc_log->write(mod_ctx,r==MOD_OK?LOG_LEVEL_INFO:LOG_LEVEL_WARN,text);}catch(...){}
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 expansion::Mode mode;const auto loaded=expansion::load_mode(mod_ctx,svc_host,svc_file,mode,mode_reason);
 if(loaded!=MOD_OK){event(mode_reason,2,0,loaded);return mods::set_error(error,loaded,"Expansion qa.json invalid/unreadable; zero requests");}
 event(mode_reason,2,0,MOD_OK);const auto r=client.initialize(mod_ctx,svc_host,svc_devf_encounter,mode,event);event(client.reason,2,0,r);
 if(r!=MOD_OK)return mods::set_error(error,r,"Expansion request batch rejected; own edits rolled back/detach fallback");return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError* error){const auto r=client.shutdown();event(client.reason,2,0,r);return r==MOD_OK?MOD_OK:mods::set_error(error,r,"Expansion own edit removal failed; provider owner-detach fallback");}
}
