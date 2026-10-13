#pragma once
#include "policy_values.hpp"
namespace devf::pilot {
// Owning scalar sidecar on the existing synchronous visitor; never actor/layer pointers.
class CensusValues {
 CaptureFacts facts_{};
public:
 void reset(Id sequence,std::uint64_t update) noexcept;
 void add(const ActorSample&) noexcept;
 const CaptureFacts& values()const noexcept{return facts_;}
};
CensusValues& census_values() noexcept;
void collect_actor(Id id,Id profile,int room,const std::array<double,3>& xyz,Id owner,bool owner_valid,
 bool is_link,const std::array<double,3>& speed,double forward) noexcept;
void set_capture_update(std::uint64_t) noexcept;
}
