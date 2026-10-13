#pragma once
#include "policy_values.hpp"
namespace devf::first {struct Frame;}
namespace devf::pilot {
Decision assess(const first::Frame&) noexcept;
GuardReason freshness(const first::Frame& previous,const first::Frame& current) noexcept;
}
