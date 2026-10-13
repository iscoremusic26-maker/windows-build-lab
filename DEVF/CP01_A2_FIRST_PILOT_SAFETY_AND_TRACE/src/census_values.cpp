#include "census_values.hpp"
namespace devf::pilot {
namespace {CensusValues current;}
CensusValues& census_values() noexcept{return current;}
void CensusValues::reset(Id sequence,std::uint64_t update) noexcept{facts_={};facts_.sequence=sequence;facts_.update_index=update;}
void CensusValues::add(const ActorSample& a) noexcept {
 if(facts_.count==actor_cap){facts_.actors_truncated=true;return;}
 for(unsigned i=0;i<facts_.count;++i)if(facts_.actors[i].id==a.id)facts_.duplicates=true;
 facts_.actors[facts_.count++]=a;
}
void collect_actor(Id id,Id profile,int room,const std::array<double,3>& xyz,Id owner,bool owner_valid,
 bool is_link,const std::array<double,3>& speed,double forward) noexcept {
 ActorSample a;a.id=id;a.profile=profile;a.room=room;a.xyz=xyz;a.owner=owner;a.owner_valid=owner_valid;
 a.is_link=is_link;a.speed=speed;a.speed_forward=forward;a.init=2;a.initialized=true;
 current.add(a);
}
}
