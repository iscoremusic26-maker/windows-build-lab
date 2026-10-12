#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#if !defined(DEVF_READ_ONLY) || DEVF_READ_ONLY != 1
#error "Permanent read-only build required"
#endif
#if defined(DEVF_ENABLE_SPAWN) || defined(DEVF_ENABLE_MUTATION)
#error "Arming configuration is forbidden"
#endif
namespace devf::a1 {
inline constexpr bool kRegistrationAllowed = false;
inline constexpr bool kGameMutationAllowed = false;
inline constexpr unsigned kLogLimit = 64, kCadence = 120;
inline constexpr std::uint32_t kUnknown = 0xffffffffu;
struct NativeRecord {
    std::uint32_t id = kUnknown, owner = kUnknown, parameters = 0;
    int room = -1, home_x = 0;
    std::array<float, 3> xyz{};
};
enum class Observation { Unavailable, Transition, InvalidRegistry, NoPlayer, IncompletePlayer,
    InvalidRoom, NoRoomNode, IncompleteRoom, WrongContext, InvalidValues, Captured };
// Owning values only. No borrowed native pointers, strings, handles, resources or callbacks.
struct Snapshot {
    Observation status = Observation::Unavailable;
    std::array<char, 9> stage{};
    int room = -1, logical_layer = -1;
    std::uint32_t current_layer = kUnknown, link_owner = kUnknown, room_node = kUnknown,
        room_layer = kUnknown;
    int room_creates = -1, room_deletes = -1;
    std::array<float, 3> link_xyz{};
    int link_yaw = 0;
    bool e_oc_registered = false, e_ocb_registered = false, census_truncated = false;
    std::array<NativeRecord, 8> census{};
    std::size_t census_count = 0;
};
static_assert(std::is_trivially_copyable_v<Snapshot>);
struct State {
    bool active = false;
    unsigned lines = 0, countdown = 0;
    bool sample_due();
    void reset() { *this = {}; }
};
bool valid_values(const Snapshot&);
const char* observation_name(Observation);
void format_snapshot(const Snapshot&, char* out, std::size_t capacity);
Snapshot capture_native();

// Only the registered list is dereferenced. An unmatched candidate is opaque.
// All native pointers in this helper exist only in one capture's stack frame.
template<class T, std::size_t N = 128> struct Registry {
    std::array<const T*, N> entries{};
    std::size_t count = 0;
    bool complete = false;
    template<class Next> void collect(const T* root, Next next) {
        count = 0; complete = false;
        while (root) {
            for (std::size_t i = 0; i < count; ++i) if (entries[i] == root) return;
            if (count == N) return;
            entries[count++] = root;
            root = next(root);
        }
        complete = count != 0;
    }
    template<class Id> bool id_of(const T* candidate, std::uint32_t& out, Id id) const {
        out = kUnknown;
        if (!complete || !candidate) return false;
        for (std::size_t i = 0; i < count; ++i) if (candidate == entries[i]) {
            out = id(entries[i]); return true;
        }
        return false;
    }
};
}
