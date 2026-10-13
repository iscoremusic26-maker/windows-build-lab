#include "native_backend.hpp"
#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <cstring>
#include <cstdio>
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(ActorService,svc_actor);
namespace {devf::first::Controller controller;bool active=false,updating=false;unsigned ticks=0,lines=0;
template<class T>bool compatible(const T* p,unsigned major,unsigned minor){return p&&p->header.struct_size>=sizeof(T)&&p->header.major_version==major&&p->header.minor_version>=minor;}}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 if(active)return MOD_CONFLICT;
 if(!compatible(svc_host,2,2)||!svc_host->version||(std::strcmp(svc_host->version,"2.0.3")&&std::strcmp(svc_host->version,"2.0.3.0"))||!compatible(svc_log,1,0)||!svc_log->write||!compatible(svc_game,2,0)||!compatible(svc_actor,1,0)||!svc_actor->create_actor||!svc_actor->get_actor_room_num)
  return mods::set_error(error,MOD_UNSUPPORTED,"Official2.0.3 Host2.2 Log1 Game2 Actor1 required; OFFLINE experiment only");
 active=true;return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){
 if(!active)return MOD_OK;
 if(updating){controller.stop(devf::first::Reason::Reentry);return MOD_ERROR;}
 updating=true;struct End{~End(){updating=false;}} end;
 try {
  devf::first::NativeBackend backend(mod_ctx);const auto permission=devf::first::current_admission();
  // Probe anchor is retained explicitly; code is creation-capable, current provider denies admission.
  if(!controller.consumed())devf_first_request_link_anchor(controller,backend,permission);
  else controller.poll(backend);
  if(lines<16&&ticks++%120==0){++lines;char text[768];std::snprintf(text,sizeof(text),"DEVF_FIRST_OFFLINE EXPERIMENTAL_NOT_DISTRIBUTED CREATION_CODE_PRESENT admission=%d state=%u reason=%u consumed=%d id=%u polls=%u request_max=1 actor_resources=NOT_VERIFIED rendered=NOT_TESTED no_cleanup_or_cancel=1",permission.location_admitted&&permission.session_authorized,unsigned(controller.state()),unsigned(controller.reason()),controller.consumed(),controller.id(),controller.polls());svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);}
 }catch(...){controller.stop(devf::first::Reason::Exception);active=false;return MOD_ERROR;}
 return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*){active=false;controller.shutdown();return MOD_OK;}
}
