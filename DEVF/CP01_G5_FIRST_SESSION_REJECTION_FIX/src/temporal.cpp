#include "temporal.hpp"
#include "policy.hpp"
#include "experiment_core.hpp"
#include <cmath>
namespace devf::pilot {
Delta Temporal::observe(const first::Frame& f) noexcept {
 Delta d;const auto& v=f.pilot;
 bool ok=first::context_valid(f)&&v.coherent&&v.context==ContextStamp{f.owner.node,f.owner.child,f.owner.play,f.owner.parent,f.owner.room,f.owner.logical}&&
 v.sequence!=0&&v.sequence!=unknown&&v.update_index!=0&&v.link_read&&v.link_profile_checked&&v.link.is_link&&v.link.id!=0&&v.link.id!=unknown&&v.link.init==2&&v.link.initialized&&v.link.owner_valid&&v.link.room==3&&v.link.owner==f.owner.link_owner&&
 metrics(v.link.xyz).valid&&v.actors_complete&&!v.actors_truncated&&!v.duplicates&&v.count>0&&v.count<=actor_cap&&v.visited==v.count;
 for(unsigned i=0;ok&&i<v.count;++i){const auto& a=v.actors[i];ok=a.id!=unknown&&a.id!=0&&a.profile!=unknown&&a.owner_valid&&a.owner!=unknown&&a.init==2&&a.initialized&&room_scope(a,f)!=RoomScope::Invalid&&metrics(a.xyz).valid;}
 unsigned links=0;
 for(unsigned i=0;ok&&i<v.count;++i)if(v.actors[i].is_link){++links;const auto& a=v.actors[i];ok=a.id==v.link.id&&a.profile==v.link.profile&&a.owner==v.link.owner&&a.room==v.link.room&&a.xyz==v.link.xyz&&a.speed==v.link.speed&&a.speed_forward==v.link.speed_forward;}
 ok=ok&&links==1;for(double x:v.link.speed)ok=ok&&std::isfinite(x);ok=ok&&std::isfinite(v.link.speed_forward);
 if(!ok){reset();d.state=DeltaState::Incomplete;return d;}
 if(!have_){previous_=v;have_=true;return d;}
 d.previous_update=previous_.update_index;
 if(v.update_index<previous_.update_index){reset();d.state=DeltaState::Rewind;return d;}
 if(v.context!=previous_.context||v.link.id!=previous_.link.id||v.link.profile!=previous_.link.profile||v.link.owner!=previous_.link.owner||v.link.room!=previous_.link.room){reset();d.state=DeltaState::IdentityChanged;return d;}
 if(v.update_index==previous_.update_index){d.state=v.link.xyz==previous_.link.xyz?DeltaState::SameUpdate:DeltaState::ChangedWithinUpdate;if(d.state==DeltaState::ChangedWithinUpdate)reset();return d;}
 d.gap=v.update_index-previous_.update_index;
 if(d.gap==1){d.valid=true;d.state=DeltaState::Consecutive;for(unsigned i=0;i<3;++i)d.xyz[i]=v.link.xyz[i]-previous_.link.xyz[i];}
 else d.state=DeltaState::Skipped;
 previous_=v;return d; // Diagnostics only: this class NEVER grants admission or idle.
}
}
