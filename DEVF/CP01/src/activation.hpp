#pragma once

namespace devf::cp01 {
// Evidence cannot be substituted by a config switch. No production source supplies these proofs.
struct ActivationRequirements {
    bool location_verified = false;
    bool pending_requests_drained = false;
    bool collision_teardown_verified = false;
    bool host_unload_barrier = false;
    bool permits_registration() const {
        return location_verified && pending_requests_drained &&
            collision_teardown_verified && host_unload_barrier;
    }
};
}
