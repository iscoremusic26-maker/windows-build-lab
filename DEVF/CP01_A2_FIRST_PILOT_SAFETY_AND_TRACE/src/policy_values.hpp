#pragma once
#include <array>
#include <cstdint>
#include <type_traits>
namespace devf::pilot {
using Id=std::uint32_t;
inline constexpr Id unknown=0xffffffffu;
inline constexpr unsigned actor_cap=256;
inline constexpr double link_min_xz=1600.0, actor_min_xz=1600.0;
// Actual float32 spawn values promoted to double for metrics. Not physical collider bounds.
inline constexpr std::array<double,3> spawn{double(-4806.7346f),double(-6729.480961460304f),double(65135.8281f)};
struct ContextStamp {
 Id node=unknown,child=unknown,play=unknown,parent=unknown;int room=-1,logical=-1;
 bool operator==(const ContextStamp&)const=default;
};
struct ActorSample {
 Id id=unknown,profile=unknown,owner=unknown;int room=-1,init=-1;
 std::array<double,3> xyz{},speed{};double speed_forward=0;
 bool initialized=false,owner_valid=false,is_link=false;
};
struct GroundSample {
 unsigned status=0;double y=0;int bg=-1,poly=-1,room=-1,ground=-1,special=-1;bool moving=false;
};
struct CaptureFacts {
 Id sequence=unknown;std::uint64_t update_index=0;ContextStamp context{};
 bool coherent=false,link_read=false,link_profile_checked=false;
 ActorSample link{};GroundSample ground{};
 unsigned water_status=0,bg_candidates=0,visited=0,count=0;
 bool bg_complete=false,bg_truncated=false,actors_complete=false,actors_truncated=false,duplicates=false,resources_read=false;
 std::array<ActorSample,actor_cap> actors{};
};
enum class GuardReason { None,CaptureStale,LinkUnknown,LinkTooClose,LinkMoving,ActorNearC,
 ActorCensusIncomplete,DistanceInvalid,BgIncomplete,BgNearC,WaterNotVerified,GroundMismatch,
 ResourcesNotRegistered,LinkChanged,ActorChanged };
struct Metrics {double xz=0,d3=0,dy=0;bool valid=false;};
struct Decision {bool allowed=false;GuardReason reason=GuardReason::CaptureStale;Metrics link{},offender_metrics{};ActorSample offender{};};
const char* guard_name(GuardReason) noexcept;
Metrics metrics(const std::array<double,3>&) noexcept;
static_assert(std::is_trivially_copyable_v<CaptureFacts>);
}
