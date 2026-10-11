#include <mods/service.hpp>
#include <mods/svc/actor.h>
#include <mods/svc/stage.h>
#include <mods/svc/resource.h>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <f_op/f_op_actor_mng.h>
#include <cstring>
DEFINE_MOD();
IMPORT_SERVICE(ActorService, svc_actor);
IMPORT_SERVICE(StageService, svc_stage);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
// FEATURES game adds the official GameService 2.0 import; no copied actor/profile.
static_assert(sizeof(ActorId) == 4 && sizeof(ActorHandle) == 8);
static_assert(sizeof(ProfileName) == 2 && sizeof(StageActorHandle) == 8);
extern "C" {
// Retain one real game import to verify linking to the exact Windows import library.
// Returns an address only; never searches, creates or mutates actors in CP00.
using ActorLookup = s32 (*)(fpc_ProcID, fopAc_ac_c**);
MOD_EXPORT ActorLookup devf_cp00_game_linkage() {
    return static_cast<ActorLookup>(&fopAcM_SearchByID);
}
MOD_EXPORT ModResult mod_initialize(ModError*) {
    if (!svc_host || !svc_host->version ||
        (std::strcmp(svc_host->version, "2.0.3") != 0 &&
         std::strcmp(svc_host->version, "2.0.3.0") != 0))
        return MOD_UNSUPPORTED;
    svc_log->info(mod_ctx, "DEVF CP00: service imports resolved; no actor or stage operations performed");
    return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*) { return MOD_OK; }
MOD_EXPORT ModResult mod_shutdown(ModError*) { return MOD_OK; }
}
