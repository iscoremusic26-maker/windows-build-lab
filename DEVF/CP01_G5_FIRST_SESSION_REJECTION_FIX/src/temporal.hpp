#pragma once
#include "policy_values.hpp"
namespace devf::first {struct Frame;}
namespace devf::pilot {
enum class DeltaState:unsigned { NoPrevious,Consecutive,SameUpdate,Skipped,Rewind,IdentityChanged,Incomplete,ChangedWithinUpdate };
struct Delta {DeltaState state=DeltaState::NoPrevious;std::uint64_t previous_update=0,gap=0;std::array<double,3> xyz{};bool valid=false;};
class Temporal {
 bool have_=false;CaptureFacts previous_{};
public: Delta observe(const first::Frame&) noexcept;void reset()noexcept{have_=false;previous_={};}
};
}
