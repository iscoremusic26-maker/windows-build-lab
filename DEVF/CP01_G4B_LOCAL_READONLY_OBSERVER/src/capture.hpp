#pragma once
#include "../../CP01_A2_DISARMED_CONTROLLER/src/controller_core.hpp"
#include "../../CP01_G2_READONLY_PACKAGE/src/core.hpp"
#include <array>
#include <cstdint>
#if !defined(DEVF_G4B_READ_ONLY) || DEVF_G4B_READ_ONLY != 1
#error "Permanent READ_ONLY build required"
#endif
#if defined(DEVF_ENABLE_SPAWN) || defined(DEVF_ENABLE_MUTATION)
#error "No armed configuration exists"
#endif
namespace devf::g4b {
using a2::Id; inline constexpr Id unknown=a2::unknown;
inline constexpr std::array<double,3> C{-4806.7346,-6729.480961460304,65135.8281};
inline constexpr double radius=501; // Initial resource query only, never navigation or actor bound.
inline constexpr unsigned max_records=16,cadence=120,max_actors=12,max_bg=8,max_visits=256;
enum class Status { NotVerified, Observed, NoHitAtProbe, Invalid, UnavailableSafeApi };
enum class Session { Incomplete, CompatibleObserved, IncompatibleObserved };
enum class Occupancy { OverlapNotVerified, Proximate, SeparatedUnderBound, OverlapUnderBound, Invalid };
struct Surface {
    Status status=Status::NotVerified; double y=0;
    int bg=-1,poly=-1,room=-1,ground=-1,special=-1,color=-1;
    Id actor=unknown; bool moving=false;
};
struct Actor {
    Id id=unknown,profile=unknown,owner=unknown; int room=-1;
    std::array<double,3> xyz{}; double distance=0,bound=0;
    bool initialized=false,owner_valid=false,bound_proven=false;
    unsigned included=0; // 1 Link, 2 center within501, 4 BG owner.
    Occupancy occupancy=Occupancy::OverlapNotVerified;
};
struct Bg {
    int index=-1,room=-1; Id actor=unknown;
    bool ready=false,moving=false,locked=false,owner_valid=false;
    std::array<double,3> lo{},hi{}; double distance=0;
};
struct Details {
    Id node_profile=unknown,play_profile=unknown; int node_init=-1,play_init=-1,room_param=-1;
    unsigned status_mask=0; bool mask_read=false,next_stage=false;
};
struct Capture {
    Id id=unknown; a2::Owner owner{}; Details details{};
    a1::Observation base=a1::Observation::Unavailable;
    Surface ground{},water{};
    std::array<Actor,max_actors> actors{}; unsigned actor_count=0;
    std::array<Bg,max_bg> bg{}; unsigned bg_count=0;
    unsigned visited=0,bg_slots=0;
    bool actors_complete=false,actors_truncated=false,bg_scan_complete=false,bg_truncated=false;
    bool context_stable=false,budget_exceeded=false,event_read=false,event_active=false;
    bool resources_read=false,e_oc_registered=false,e_ocb_registered=false;
    bool capture_valid=false; Session session=Session::Incomplete;
    Status exact_bg=Status::UnavailableSafeApi,water_volume=Status::UnavailableSafeApi;
};
static_assert(std::is_trivially_copyable_v<Capture>);
struct Budget {
    bool active=false,busy=false; unsigned lines=0,ticks=0;
    bool due() noexcept { return active && !busy && lines<max_records && ticks++%cadence==0; }
};
double distance_to_C(const std::array<double,3>&) noexcept;
bool sphere_box_candidate(const std::array<double,3>& lo,const std::array<double,3>& hi,double& distance) noexcept;
Occupancy classify_actor(const Actor&) noexcept;
void evaluate(Capture&) noexcept;
bool format(const Capture&,const char* host,const char* build,char*,std::size_t) noexcept;
Capture capture_native(Id);
// Scalar-only sidecar to the adapted G2 pass. No borrowed pointers in this storage.
Capture& sidecar() noexcept;
void collect_bg(Capture&);
}
