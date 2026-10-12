#include "controller.hpp"
#include <cmath>
#include <cstring>

namespace devf::cp01 {
ModResult Controller::start_guarded(const ActorProfileDesc& profile, const Placement* placement,
    const ActivationRequirements& requirements) {
    if (!requirements.permits_registration()) return MOD_UNSUPPORTED;
    return start(profile, placement);
}
ModResult Controller::start(const ActorProfileDesc& profile, const Placement* placement) {
    if (registered() || has_stage_edit()) return MOD_CONFLICT;
    if (!context_ || !actor_.register_actor || !actor_.unregister_actor ||
        !actor_.create_actor || !stage_.add_actor || !stage_.remove_actor_edit)
        return MOD_UNAVAILABLE;
    if (!profile.name[0] || profile.name[7] != '\0') return MOD_INVALID_ARGUMENT;
    if (placement) {
        if (!placement->location_verified) return MOD_UNSUPPORTED;
        if (!placement->stage[0] || placement->stage[8] != '\0' || placement->room == 0xff ||
            placement->layer < -1 || placement->layer > 15 ||
            std::memcmp(profile.name, placement->record.data(), 8) != 0)
            return MOD_INVALID_ARGUMENT;
    }
    last_cleanup_ = MOD_OK;
    auto result = actor_.register_actor(context_, &profile, &profile_name_, &profile_handle_);
    if (result != MOD_OK) {
        profile_handle_ = 0; profile_name_ = -1;
        return result;
    }
    if (!profile_handle_) { profile_name_ = -1; return MOD_ERROR; }
    if (placement) {
        result = stage_.add_actor(context_, placement->stage.data(), placement->room,
            placement->layer, placement->record.data(), placement->record.size(), &stage_handle_);
        if (result != MOD_OK || !stage_handle_) {
            const auto failure = result == MOD_OK ? MOD_ERROR : result;
            last_cleanup_ = stop();
            return failure;
        }
    }
    return MOD_OK;
}
ModResult Controller::stop(bool host_already_deactivating) {
    if (stage_handle_) {
        const auto result = stage_.remove_actor_edit(context_, stage_handle_);
        if (result != MOD_OK) return last_cleanup_ = result;
        stage_handle_ = 0;
    }
    if (profile_handle_) {
        const auto result = actor_.unregister_actor(context_, profile_handle_);
        // On failure retain ownership, forbid another registration, and report cleanup incomplete.
        if (result != MOD_OK && !(host_already_deactivating && result == MOD_INVALID_ARGUMENT))
            return last_cleanup_ = result;
        profile_handle_ = 0; profile_name_ = -1;
    }
    runtime_requested_ = false;
    return last_cleanup_ = MOD_OK;
}
ModResult Controller::request_runtime(const ActorSpawnParams& p, ActorId* requested_id) {
    if (!requested_id) return MOD_INVALID_ARGUMENT;
    if (!registered()) return MOD_UNAVAILABLE;
    if (has_stage_edit() || runtime_requested_) return MOD_CONFLICT;
    // The host copies this extra callback into a creation request with no lifetime token.
    if (p.create_function) return MOD_UNSUPPORTED;
    if (p.room_num < 0 || !std::isfinite(p.position.x) || !std::isfinite(p.position.y) ||
        !std::isfinite(p.position.z) || !std::isfinite(p.scale.x) || !std::isfinite(p.scale.y) ||
        !std::isfinite(p.scale.z) || p.scale.x <= 0 || p.scale.y <= 0 || p.scale.z <= 0)
        return MOD_INVALID_ARGUMENT;
    const auto result = actor_.create_actor(context_, profile_name_, &p, requested_id);
    if (result == MOD_OK) runtime_requested_ = true;
    // ID is a request result only. Never retain it or delete arbitrary actors by a possibly reused ID.
    return result;
}
}
