#pragma once
#include <array>
#include <cstdint>
#include <type_traits>
#if !defined(DEVF_A2_DISARMED) || DEVF_A2_DISARMED != 1
#error "Permanent disarmed configuration required"
#endif
#if defined(DEVF_ENABLE_SPAWN) || defined(DEVF_ENABLE_MUTATION) || defined(DEVF_A2_ARMED)
#error "Arming configurations forbidden"
#endif
namespace devf::a2 {
using Id = std::uint32_t;
inline constexpr Id unknown = 0xffffffffu;
inline constexpr bool native_requests_allowed = false;
struct Preset {
    static constexpr Id profile = 0x1fe, parameters = 0xffffffffu;
    static constexpr int argument = -1, angle_x = -236, yaw = 0, angle_z = 0;
    static constexpr unsigned set_id = 0xffff;
    static constexpr std::array<float,3> scale{1,1,1};
}; // No position or native callback is stored or assigned here.
enum class State { Disarmed, ValidationRejected, AttemptConsumed, Requested, Pending,
    ReadyObserved, Failed, StopSession };
enum class Reason { None, Disarmed, Host, Thread, Context, Registry, Player, Node,
    Binding, Transition, PendingContext, Previous, Position, Revalidation, Reentry,
    Consumed, Selection, Service, InvalidId, Exception, Restore, Identity, Phase,
    MissingActor, Resources, Timeout, Removed, Ambiguous };
// Owning scalars only, never borrowed engine pointers or alleged generation handles.
struct Owner {
    bool host_ok=false, thread_ok=false, registry_ok=false, player_ok=false;
    bool node_present=false, node_profile=false, node_complete=false, node_id_ok=false;
    bool child_registered=false, binding_ok=false, play_ok=false;
    bool previous_registered=false, transition=false, retiring=false;
    std::array<char,9> stage{};
    int room=-1, logical=-1, next_room=-1;
    Id node=unknown, child=unknown, parent=unknown, play=unknown, play_layer=unknown;
    Id previous=unknown, root=unknown, link_owner=unknown;
    int creates=-1, deletes=-1, parent_creates=-1, parent_deletes=-1;
};
struct Context {
    Id node=unknown, child=unknown, play=unknown, parent=unknown;
    int room=-1, logical=-1;
    bool operator==(const Context&) const = default;
};
Context context_of(const Owner&) noexcept;
Reason validate_owner(const Owner&) noexcept;
bool same_preflight(const Owner&, const Owner&) noexcept;
struct Observation {
    Id id=unknown, profile=unknown, owner=unknown;
    int room=-1;
    bool pending=false, visible=false, initialized=false, actor_init=false;
    bool resources=false, failed=false, removed=false, ambiguous=false, reused=false;
};
class Controller {
    State state_=State::Disarmed;
    Reason reason_=Reason::Disarmed;
    bool consumed_=false, busy_=false;
    Id id_=unknown;
    Context context_{};
    unsigned elapsed_=0, timeout_;
public:
    explicit Controller(unsigned timeout=120) noexcept : timeout_(timeout ? timeout : 1) {}
    State state() const noexcept { return state_; }
    Reason reason() const noexcept { return reason_; }
    bool consumed() const noexcept { return consumed_; }
    bool busy() const noexcept { return busy_; }
    Id request_id() const noexcept { return id_; }
    bool begin_simulation(const Owner&, bool fixture_permission, bool synthetic_position) noexcept;
    void end_scope() noexcept { busy_=false; }
    void requested(Id) noexcept;
    void fail(Reason, bool uncertain=false) noexcept;
    void observe(const Owner&, const Observation&) noexcept;
    void disarm() noexcept;
};
static_assert(std::is_trivially_copyable_v<Owner> && std::is_trivially_copyable_v<Observation>);
}
