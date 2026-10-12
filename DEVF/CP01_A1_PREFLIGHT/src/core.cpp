#include "core.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
namespace devf::a1 {
bool State::sample_due() {
    if (!active || lines >= kLogLimit - 1) return false; // reserve shutdown line
    if (countdown) { --countdown; return false; }
    countdown = kCadence - 1;
    return true;
}
bool valid_values(const Snapshot& s) {
    if (s.stage[8] != '\0' || s.stage[0] == '\0' || s.room < 0 || s.room >= 64 ||
        s.logical_layer < 0 || s.logical_layer >= 15 || s.census_count > s.census.size()) return false;
    for (char ch : s.stage) {
        if (!ch) break;
        if (!(ch >= 'A' && ch <= 'Z') && !(ch >= '0' && ch <= '9') && ch != '_') return false;
    }
    for (float f : s.link_xyz) if (!std::isfinite(f)) return false;
    for (std::size_t i = 0; i < s.census_count; ++i)
        for (float f : s.census[i].xyz) if (!std::isfinite(f)) return false;
    return true;
}
const char* observation_name(Observation s) {
    switch (s) {
    case Observation::Transition: return "SKIP_TRANSITION";
    case Observation::InvalidRegistry: return "SKIP_LAYER_REGISTRY";
    case Observation::NoPlayer: return "SKIP_PLAYER_NOT_IN_QUEUE";
    case Observation::IncompletePlayer: return "SKIP_PLAYER_CREATING_OR_DELETING";
    case Observation::InvalidRoom: return "SKIP_ROOM_RANGE";
    case Observation::NoRoomNode: return "SKIP_ROOM_NODE";
    case Observation::IncompleteRoom: return "SKIP_ROOM_CREATING_OR_DELETING";
    case Observation::WrongContext: return "SKIP_OUTSIDE_F_SP121_R3";
    case Observation::InvalidValues: return "SKIP_INVALID_VALUES";
    case Observation::Captured: return "CAPTURED_NOT_RUNTIME_VALIDATED";
    default: return "NOT_VERIFIED";
    }
}
void format_snapshot(const Snapshot& s, char* out, std::size_t capacity) {
    if (!capacity) return;
    if (s.status != Observation::Captured || !valid_values(s)) {
        std::snprintf(out, capacity, "DISARMED %s", observation_name(
            s.status == Observation::Captured ? Observation::InvalidValues : s.status));
        return;
    }
    int n = std::snprintf(out, capacity,
        "DISARMED stage=%s room=%d logical=%d current=%u link_owner=%u room_node=%u room_layer=%u "
        "room_creates=%d room_deletes=%d link=(%.3f,%.3f,%.3f) yaw=%d "
        "E_OC_registered=%d E_ocb_registered=%d loaded=NOT_VERIFIED renderer=NOT_VERIFIED census=%zu truncated=%d",
        s.stage.data(), s.room, s.logical_layer, s.current_layer, s.link_owner, s.room_node,
        s.room_layer, s.room_creates, s.room_deletes, s.link_xyz[0], s.link_xyz[1], s.link_xyz[2],
        s.link_yaw, s.e_oc_registered, s.e_ocb_registered, s.census_count, s.census_truncated);
    std::size_t used = n < 0 ? 0 : static_cast<std::size_t>(n);
    for (std::size_t i = 0; i < s.census_count && used < capacity; ++i) {
        const auto& r = s.census[i];
        n = std::snprintf(out + used, capacity - used,
            " | E_OC id=%u rm=%d owner=%u p=%08x hx=%d xyz=(%.1f,%.1f,%.1f)",
            r.id, r.room, r.owner, r.parameters, r.home_x, r.xyz[0], r.xyz[1], r.xyz[2]);
        if (n < 0) break;
        used += static_cast<std::size_t>(n);
    }
    out[capacity - 1] = '\0';
}
}
