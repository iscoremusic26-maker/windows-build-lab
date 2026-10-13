#include "room_owner_resolver.hpp"
#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <cstring>
#include <cstdio>
DEFINE_MOD();
IMPORT_SERVICE(HostService,svc_host);
IMPORT_SERVICE(LogService,svc_log);
namespace {
bool active=false, updating=false;
unsigned ticks=0, lines=0;
devf::a2::Controller controller;
template<class T> bool compatible(const T* s,unsigned major,unsigned minor) {
    return s && s->header.struct_size>=sizeof(T) && s->header.major_version==major && s->header.minor_version>=minor;
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    static_assert(!devf::a2::native_requests_allowed);
    if (active) return mods::set_error(error,MOD_CONFLICT,"Already initialized");
    if (!compatible(svc_host,2,2) || !svc_host->version ||
        (std::strcmp(svc_host->version,"2.0.3") && std::strcmp(svc_host->version,"2.0.3.0")) ||
        !compatible(svc_log,1,0) || !svc_log->write || !compatible(svc_game,2,0))
        return mods::set_error(error,MOD_UNSUPPORTED,"Official 2.0.3 Host2.2 Log1.0 Game2 required");
    active=true; return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*) {
    if (!active || updating) return MOD_OK;
    updating=true;
    struct End { ~End() { updating=false; } } end;
    if (lines>=16 || ticks++%120) return MOD_OK;
    const auto owner=devf::a2::capture_owner(true,true);
    controller.disarm(); // Permanent: no request backend or simulation code is linked here.
    char text[256];
    std::snprintf(text,sizeof(text),"DEVF A2 DISARMED_NO_REGISTRATION owner_validation=%u node=%u child=%u play=%u previous=%u XYZ=NO_ASIGNADO requests=0",
        unsigned(devf::a2::validate_owner(owner)),owner.node,owner.child,owner.play,owner.previous);
    ++lines; svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);
    return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*) {
    active=false; controller.disarm(); return MOD_OK;
}
}
