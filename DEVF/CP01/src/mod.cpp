#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/resource.h>
#include "controller.hpp"
#include "rock.hpp"
#include <cstdio>
#include <cstring>
#include <optional>

DEFINE_MOD();
IMPORT_SERVICE(ActorService, svc_actor);
IMPORT_SERVICE(StageService, svc_stage);
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ResourceService, svc_resource);

namespace {
std::optional<devf::cp01::Controller> controller;
// No verified placement was supplied. A .cfg cannot silently enable unverified coordinates.
constexpr char policy[] = "DISARMED_NO_REGISTRATION\n";
constexpr devf::cp01::ActivationRequirements requirements{};
void diagnostics() {
    const auto& c = devf::cp01::counters;
    char text[256];
    std::snprintf(text, sizeof(text), "CP01 counters constructed=%u ready=%u destroyed=%u execute=%u draw=%u (runtime evidence only if host actually ran)",
        c.constructed, c.ready, c.destroyed, c.execute, c.draw);
    svc_log->info(mod_ctx, text);
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if (!svc_host || !svc_host->version ||
        (std::strcmp(svc_host->version, "2.0.3") != 0 && std::strcmp(svc_host->version, "2.0.3.0") != 0))
        return mods::set_error(error, MOD_UNSUPPORTED, "DEVF CP01 requires Dusklight 2.0.3; Windows DX12 runtime remains unverified");
    if (controller) return mods::set_error(error, MOD_CONFLICT, "CP01 already owns a registration");
    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    const auto loaded = svc_resource->load(mod_ctx, "safety-policy.txt", &buffer);
    if (loaded != MOD_OK) return mods::set_error(error, loaded, "CP01 safety policy unavailable; nothing registered");
    const bool valid = buffer.data && buffer.size == sizeof(policy) - 1 &&
        std::memcmp(buffer.data, policy, sizeof(policy) - 1) == 0;
    svc_resource->free(mod_ctx, &buffer);
    if (!valid) return mods::set_error(error, MOD_INVALID_ARGUMENT, "CP01 refuses altered safety policy; no verified placement available");
    controller.emplace(mod_ctx, *svc_actor, *svc_stage);
    const auto result = controller->start_guarded(devf::cp01::Rock::profile, nullptr, requirements);
    if (result == MOD_UNSUPPORTED && !controller->registered() && !controller->has_stage_edit()) {
        controller.reset();
        svc_log->warn(mod_ctx, "CP01 DISARMED: no profiles, stage edits or actor requests; location and host unload guarantees missing");
        return MOD_OK;
    }
    if (result != MOD_OK) {
        if (!controller->registered()) controller.reset();
        return mods::set_error(error, result, "CP01 profile registration failed; rollback state recorded");
    }
    svc_log->warn(mod_ctx, "DEVF CP01 BUILD ONLY: dvfrock profile registered; Stage/runtime spawns disabled; no location verified; no game/DX12 PASS");
    return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*) { return MOD_OK; }
MOD_EXPORT ModResult mod_shutdown(ModError* error) {
    if (!controller) return MOD_OK;
    auto result = controller->stop(true);
    if (devf::cp01::counters.constructed != devf::cp01::counters.destroyed ||
        devf::cp01::counters.cleanup_failures)
        result = MOD_UNAVAILABLE;
    diagnostics();
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "CP01 cleanup incomplete. Host unload ignores shutdown failure: exit process; do not hot reload");
        return mods::set_error(error, result, "CP01 cleanup incomplete; process restart required; return code cannot prevent host DLL unload");
    }
    controller.reset();
    return MOD_OK;
}
}
