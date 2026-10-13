#pragma once
#include "policy_values.hpp"
namespace devf::first {struct Frame;}
namespace devf::pilot {
enum class RoomScope { Ordinary,KnownStageKYTAG04,Invalid };
RoomScope room_scope(const ActorSample&,const first::Frame&) noexcept;
Decision assess(const first::Frame&) noexcept;
GuardReason freshness(const first::Frame& previous,const first::Frame& current) noexcept;
}
