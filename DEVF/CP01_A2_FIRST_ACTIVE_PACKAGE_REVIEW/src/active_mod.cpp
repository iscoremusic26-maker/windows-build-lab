#include "native_backend.hpp"
#include "trace.hpp"
#include "census_values.hpp"
#include "package_compatibility.hpp"
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
bool active=false,updating=false,initialized_once=false;std::uint64_t updates=0;
using devf::package::compatible;
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
 if(active||initialized_once)return MOD_CONFLICT;
 initialized_once=true; // never rearm this image after failed init/shutdown.
 if(!devf::package::contracts_available(svc_host,svc_log,svc_game,svc_actor)){
  trace.record(devf::pilot::Event::AbiFailed,controller);drain();
  return mods::set_error(error,MOD_UNSUPPORTED,"Official2.0.3 Host2.2 Log1 Game2 Actor1 required; ACTIVE-CAPABLE pilot; execution not authorized");
 }
 active=true;trace.record(devf::pilot::Event::AbiVerified,controller);
 trace.record(devf::pilot::Event::PackageProviderArmed,controller);drain();return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){
 if(!active)return MOD_OK;
 if(updating){controller.stop(devf::first::Reason::Reentry);return MOD_ERROR;}
 updating=true;struct End{~End(){updating=false;}} end;
 if(updates==std::numeric_limits<std::uint64_t>::max()){controller.stop(devf::first::Reason::Context);drain();return MOD_ERROR;}
 if(!devf::package::contracts_available(svc_host,svc_log,svc_game,svc_actor)){controller.stop(devf::first::Reason::Host);active=false;drain();return MOD_ERROR;}
 trace.set_update(++updates);devf::pilot::set_capture_update(updates);
 try {
  devf::first::NativeBackend backend(mod_ctx);const auto permission=devf::first::current_admission();
  // Fixed technical pilot grant; Controller A/B/C remains mandatory. No user session is authorized by this binary.
  if(!controller.consumed())devf_first_request_link_anchor(controller,backend,permission);
  else controller.poll(backend);
 }catch(...){controller.stop(devf::first::Reason::Exception);active=false;drain();return MOD_ERROR;}
 drain();return MOD_OK; // external logging runs only after Controller/scope have returned.
}
MOD_EXPORT ModResult mod_shutdown(ModError*){active=false;controller.shutdown();drain();return MOD_OK;}
}
