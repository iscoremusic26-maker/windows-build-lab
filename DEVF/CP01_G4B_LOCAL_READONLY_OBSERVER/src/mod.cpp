#include "capture.hpp"
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
devf::g4b::Budget budget;unsigned sequence=0;char version[16]{},build[65]{};
template<class T> bool compatible(const T* s,unsigned major,unsigned minor){return s && s->header.struct_size>=sizeof(T)&&s->header.major_version==major&&s->header.minor_version>=minor;}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* e){
    if(budget.active)return mods::set_error(e,MOD_CONFLICT,"Observer already initialized");
    if(!compatible(svc_host,2,2)||!svc_host->version||(std::strcmp(svc_host->version,"2.0.3")&&std::strcmp(svc_host->version,"2.0.3.0"))||
        !compatible(svc_log,1,0)||!svc_log->write||!compatible(svc_game,2,0))return mods::set_error(e,MOD_UNSUPPORTED,"Official2.0.3 Host2.2 Log1.0 Game2 required");
    std::snprintf(version,sizeof(version),"%s",svc_host->version);
    std::memset(build,0,sizeof(build));
    if(svc_host->build_id && svc_host->build_id_len>0 && svc_host->build_id_len<=32){for(unsigned i=0;i<svc_host->build_id_len;++i)std::snprintf(build+2*i,3,"%02x",svc_host->build_id[i]);}
    else std::snprintf(build,sizeof(build),"NOT_VERIFIED");
    budget.active=true;return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*){
    if(!budget.due())return MOD_OK;budget.busy=true;
    struct End{~End(){budget.busy=false;}} end;
    const auto id=++sequence;
    // Consume a diagnostic slot even on C++ exceptions/errors; never unlimited retries.
    ++budget.lines;
    try {const auto c=devf::g4b::capture_native(id);char text[8192];
        if(!devf::g4b::format(c,version,build,text,sizeof(text)))std::snprintf(text,sizeof(text),"G4B DISARMED_NO_REGISTRATION id=%u CAPTURE_INCOMPLETE=LOG_TRUNCATION",id);
        svc_log->write(mod_ctx,LOG_LEVEL_INFO,text);
    }catch(...){budget.active=false;budget.lines=devf::g4b::max_records;return MOD_ERROR;}
    return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*){budget.active=false;return MOD_OK;}
}
