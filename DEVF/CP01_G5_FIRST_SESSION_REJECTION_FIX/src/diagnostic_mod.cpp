#include "experiment_core.hpp"
#include "trace.hpp"
#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <cstring>
#include <limits>
DEFINE_MOD();IMPORT_SERVICE(HostService,svc_host);IMPORT_SERVICE(LogService,svc_log);
namespace devf::first {Frame read_only_preflight();}
namespace devf::pilot {void set_capture_update(std::uint64_t)noexcept;}
namespace {
devf::first::Controller inert;devf::pilot::Trace trace;bool active=false,initialized_once=false,updating=false;std::uint64_t updates=0;
template<class T>bool compatible(const T* p,unsigned ma,unsigned mi){return p&&p->header.struct_size>=sizeof(T)&&p->header.major_version==ma&&p->header.minor_version>=mi;}
bool contracts(){return compatible(svc_host,2,2)&&svc_host->version&&(!std::strcmp(svc_host->version,"2.0.3")||!std::strcmp(svc_host->version,"2.0.3.0"))&&compatible(svc_log,1,0)&&svc_log->write&&compatible(svc_game,2,0);}
struct Sink:devf::pilot::Sink{bool write(const char* s)override{if(!compatible(svc_log,1,0)||!svc_log->write)return false;svc_log->write(mod_ctx,LOG_LEVEL_INFO,s);return true;}};
void drain()noexcept{Sink s;trace.drain(s);}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* err){
 inert.attach_trace(&trace);trace.record(devf::pilot::Event::ModuleInit,inert);
 if(initialized_once)return MOD_CONFLICT;initialized_once=true;
 if(!contracts()){trace.record(devf::pilot::Event::AbiFailed,inert);drain();return mods::set_error(err,MOD_UNSUPPORTED,"2.0.3 Host2.2 Log1 Game2 required; DISARMED_DIAGNOSTIC");}
 active=true;trace.record(devf::pilot::Event::AbiVerified,inert);trace.record(devf::pilot::Event::SessionAdmissionDenied,inert);drain();return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){
 if(!active)return MOD_OK;if(updating){inert.stop(devf::first::Reason::Reentry);active=false;return MOD_ERROR;}
 updating=true;struct End{~End(){updating=false;}}end;
 if(!contracts()||updates==std::numeric_limits<std::uint64_t>::max()){inert.stop(devf::first::Reason::Host);active=false;drain();return MOD_ERROR;}
 trace.set_update(++updates);devf::pilot::set_capture_update(updates);
 try {const auto f=devf::first::read_only_preflight();if(!active)return MOD_ERROR;trace.capture(f);}catch(...){inert.stop(devf::first::Reason::Exception);active=false;drain();return MOD_ERROR;}
 drain();return MOD_OK; // No NativeBackend, Controller.attempt/poll, ActorService or LayerScope in production.
}
MOD_EXPORT ModResult mod_shutdown(ModError*){active=false;inert.shutdown();drain();return MOD_OK;}
}
