#pragma once
#include <mods/service.hpp>
#include <mods/svc/actor.h>
#include <mods/svc/stage.h>
#include <array>
#include <cstdint>

namespace devf::cp01 {
struct Placement {
    std::array<char, 9> stage{};
    std::uint8_t room = 0xff;
    std::int8_t layer = -1;
    std::array<unsigned char, 32> record{};
    // External evidence is required. No production placement is supplied in this checkpoint.
    bool location_verified = false;
};
class Controller {
    ModContext* context_;
    const ActorService& actor_;
    const StageService& stage_;
    ActorHandle profile_handle_ = 0;
    ProfileName profile_name_ = -1;
    StageActorHandle stage_handle_ = 0;
    bool runtime_requested_ = false;
    ModResult last_cleanup_ = MOD_OK;
public:
    Controller(ModContext* context, const ActorService& actor, const StageService& stage)
        : context_(context), actor_(actor), stage_(stage) {}
    ModResult start(const ActorProfileDesc& profile, const Placement* placement);
    // ActorService's modDeactivating hook runs BEFORE mod_shutdown in this exact host.
    ModResult stop(bool host_already_deactivating = false);
    // Separate authority: forbidden when a StageService registration exists. One request/activation.
    ModResult request_runtime(const ActorSpawnParams& parameters, ActorId* requested_id);
    bool registered() const { return profile_handle_ != 0; }
    bool has_stage_edit() const { return stage_handle_ != 0; }
    ModResult last_cleanup() const { return last_cleanup_; }
};
}
