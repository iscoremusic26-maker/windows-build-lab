#include "native_backend.hpp"
#include "trace.hpp"
#include "census_values.hpp"
#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <cstring>
#include <limits>
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
IMPORT_SERVICE(ActorService,svc_actor);
namespace {
devf::first::Controller controller;devf::pilot::Trace trace;
bool active=false,updating=false;std::uint64_t updates=0;
template<class T>bool compatible(const T* p,unsigned major,unsigned minor){return p&&p->header.struct_size>=sizeof(T)&&p->header.major_version==major&&p->header.minor_version>=minor;}
class OfficialSink final:public devf::pilot::Sink {
public:bool write(const char* text)override {
 if(!compatible(svc_log,1,0)||!svc_log->write)return false;
 svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);return true; // official write is void, no invented status.
}};
void drain()noexcept{OfficialSink sink;trace.drain(sink);}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error){
 controller.attach_trace(&trace);trace.record(devf::pilot::Event::ModuleInit,controller);
 if(active)return MOD_CONFLICT;
 if(!compatible(svc_host,2,2)||!svc_host->version||(std::strcmp(svc_host->version,"2.0.3")&&std::strcmp(svc_host->version,"2.0.3.0"))||!compatible(svc_log,1,0)||!svc_log->write||!compatible(svc_game,2,0)||!compatible(svc_actor,1,0)||!svc_actor->create_actor||!svc_actor->get_actor_room_num){
  trace.record(devf::pilot::Event::AbiFailed,controller);drain();
  return mods::set_error(error,MOD_UNSUPPORTED,"Official2.0.3 Host2.2 Log1 Game2 Actor1 required; OFFLINE pilot only");
 }
 active=true;trace.record(devf::pilot::Event::AbiVerified,controller);
 trace.record(devf::pilot::Event::SessionAdmissionDenied,controller);drain();return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){
 if(!active)return MOD_OK;
 if(updating){controller.stop(devf::first::Reason::Reentry);return MOD_ERROR;}
 updating=true;struct End{~End(){updating=false;}} end;
 if(updates==std::numeric_limits<std::uint64_t>::max()){controller.stop(devf::first::Reason::Context);drain();return MOD_ERROR;}
 trace.set_update(++updates);devf::pilot::set_capture_update(updates);
 try {
  devf::first::NativeBackend backend(mod_ctx);const auto permission=devf::first::current_admission();
  // Provider is unchanged and always NONE: short-circuit BEFORE any native read/set/request.
  if(!controller.consumed())devf_first_request_link_anchor(controller,backend,permission);
  else controller.poll(backend);
 }catch(...){controller.stop(devf::first::Reason::Exception);active=false;drain();return MOD_ERROR;}
 drain();return MOD_OK; // external logging runs only after Controller/scope have returned.
}
MOD_EXPORT ModResult mod_shutdown(ModError*){active=false;controller.shutdown();drain();return MOD_OK;}
}
