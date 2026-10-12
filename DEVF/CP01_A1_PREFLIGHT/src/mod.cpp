#include "core.hpp"
#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <cstring>
#include <cstdio>
DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
// GameService epoch 2 is imported by the official FEATURES game translation unit.
namespace {
devf::a1::State state;
void emit(const char* text) {
    if (state.active && state.lines < devf::a1::kLogLimit) {
        svc_log->write(mod_ctx, LOG_LEVEL_INFO, text); ++state.lines;
    }
}
template<class T> bool compatible(const T* s, unsigned major, unsigned minor) {
    return s && s->header.struct_size >= sizeof(T) && s->header.major_version == major &&
        s->header.minor_version >= minor;
}
}
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    static_assert(!devf::a1::kRegistrationAllowed && !devf::a1::kGameMutationAllowed);
    if (state.active) return mods::set_error(error, MOD_CONFLICT, "Observer already initialized");
    state.reset();
    if (!compatible(svc_host, 2, 2) || !svc_host->version ||
        (std::strcmp(svc_host->version, "2.0.3") && std::strcmp(svc_host->version, "2.0.3.0")) ||
        !compatible(svc_log, 1, 0) || !svc_log->write || !compatible(svc_game, 2, 0))
        return mods::set_error(error, MOD_UNSUPPORTED, "Official 2.0.3; Host 2.2, Log 1.0, Game epoch 2 required");
    char build[65]{};
    const auto len = svc_host->build_id_len;
    if (len > 0 && len <= 32 && svc_host->build_id) {
        for (unsigned i = 0; i < len; ++i) std::snprintf(build + 2 * i, 3, "%02x", svc_host->build_id[i]);
    } else std::snprintf(build, sizeof(build), "NOT_VERIFIED");
    state.active = true;
    char line[256];
    std::snprintf(line, sizeof(line), "DEVF A1 observer 0.0.1 DISARMED_NO_REGISTRATION host=%s build=%s game_epoch=2 limit=64 cadence=120_updates renderer=NOT_VERIFIED", svc_host->version, build);
    emit(line);
    return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*) {
    if (state.sample_due()) {
        const auto values = devf::a1::capture_native();
        char text[2048];
        devf::a1::format_snapshot(values, text, sizeof(text));
        emit(text);
    }
    return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*) {
    emit("DISARMED observer shutdown: own scalar state cleared; no native ownership or registrations");
    state.reset();
    return MOD_OK;
}
}
