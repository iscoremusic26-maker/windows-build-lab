#include "policy.hpp"
#include "experiment_core.hpp"
#include <cmath>
#include <limits>
namespace devf::pilot {
namespace {
bool finite3(const std::array<double,3>& p) noexcept {
 for(double v:p)if(!std::isfinite(v)||std::abs(v)>std::numeric_limits<float>::max())return false;
 return true;
}
bool identifiable(const ActorSample& a,const first::Frame& f) noexcept {
 return a.id!=unknown&&a.id!=0&&a.profile!=unknown&&a.owner!=unknown&&a.owner_valid&&
        room_scope(a,f)!=RoomScope::Invalid&&a.init==2&&a.initialized;
}
bool same_actor(const ActorSample& a,const ActorSample& b) noexcept {
 return a.id==b.id&&a.profile==b.profile&&a.owner==b.owner&&a.room==b.room&&a.xyz==b.xyz&&a.is_link==b.is_link&&a.group==b.group;
}
}
// Room validity exception only, NOT an exclusion from distance/census/freshness.
RoomScope room_scope(const ActorSample& a,const first::Frame& f) noexcept {
 if(a.room>=0&&a.room<64)return RoomScope::Ordinary;
 const auto& o=f.owner;
 if(a.room==-1&&a.profile==686&&a.group==0&&!a.is_link&&a.owner_valid&&a.owner==o.parent&&
    o.parent==o.play_layer&&o.parent==o.link_owner&&o.parent!=o.root&&o.parent!=o.child&&
    first::validate_owner(o)==first::Reason::None&&first::context_valid(f))return RoomScope::KnownStageKYTAG04;
 return RoomScope::Invalid;
}
Metrics metrics(const std::array<double,3>& p) noexcept {
 Metrics m;if(!finite3(p))return m;
 const double dx=p[0]-spawn[0],dz=p[2]-spawn[2];m.dy=p[1]-spawn[1];
 m.xz=std::sqrt(dx*dx+dz*dz);m.d3=std::sqrt(dx*dx+m.dy*m.dy+dz*dz);
 m.valid=std::isfinite(m.xz)&&std::isfinite(m.d3)&&std::isfinite(m.dy);return m;
}
const char* guard_name(GuardReason r) noexcept {
 switch(r){
#define G(x,s) case GuardReason::x:return s
 G(None,"NONE");G(CaptureStale,"CAPTURE_STALE");G(LinkUnknown,"LINK_UNKNOWN");G(LinkTooClose,"LINK_TOO_CLOSE");
 G(LinkMoving,"LINK_MOVING");G(ActorNearC,"ACTOR_NEAR_C");G(ActorCensusIncomplete,"ACTOR_CENSUS_INCOMPLETE");
 G(DistanceInvalid,"DISTANCE_INVALID");G(BgIncomplete,"BG_SCAN_INCOMPLETE");G(BgNearC,"BG_NEAR_C");
 G(WaterNotVerified,"WATER_PROBE_NOT_VERIFIED");G(GroundMismatch,"GROUND_MISMATCH");G(ResourcesNotRegistered,"RESOURCES_NOT_REGISTERED");
 G(LinkChanged,"LINK_CHANGED_DURING_PREFLIGHT");G(ActorChanged,"ACTOR_CHANGED_DURING_PREFLIGHT");
#undef G
 }return "UNKNOWN";
}
Decision assess(const first::Frame& f) noexcept {
 Decision d;const auto& v=f.pilot;const auto k=first::key(f.owner);
 const ContextStamp expected{k.node,k.child,k.play,k.parent,k.room,k.logical};
 if(v.sequence==unknown||v.sequence==0||v.update_index==0||!v.coherent||v.context!=expected)return d;
 auto deny=[&](GuardReason reason){d.reason=reason;return d;};
 if(!v.actors_complete||v.actors_truncated||v.duplicates||v.count==0||v.count>actor_cap||v.visited!=v.count||v.visited>actor_cap)
  return deny(GuardReason::ActorCensusIncomplete);
 if(!v.bg_complete||v.bg_truncated)return deny(GuardReason::BgIncomplete);
 if(v.bg_candidates!=0)return deny(GuardReason::BgNearC);
 if(v.water_status!=2)return deny(GuardReason::WaterNotVerified); // NO_HIT_AT_PROBE only, never volume absence.
 const auto& g=v.ground;
 if(g.status!=1||g.bg!=0||g.poly!=2174||g.room!=3||g.ground!=0||g.special!=0||g.moving||
    !std::isfinite(g.y)||std::abs(g.y-spawn[1])>0.01)return deny(GuardReason::GroundMismatch);
 if(!v.resources_read||!f.registered_eoc||!f.registered_weapon)return deny(GuardReason::ResourcesNotRegistered);
 if(!v.link_read||!v.link_profile_checked||!identifiable(v.link,f)||v.link.room!=f.owner.room||v.link.owner!=f.owner.link_owner||!v.link.is_link)
  return deny(GuardReason::LinkUnknown);
 d.link=metrics(v.link.xyz);
 if(!d.link.valid||!finite3(v.link.speed)||(!std::isfinite(v.link.speed_forward)||std::abs(v.link.speed_forward)>std::numeric_limits<float>::max()))return deny(GuardReason::DistanceInvalid);
 if(d.link.xz<link_min_xz)return deny(GuardReason::LinkTooClose);
 if(v.link.speed!=std::array<double,3>{}||v.link.speed_forward!=0)return deny(GuardReason::LinkMoving);
 unsigned links=0;
 for(unsigned i=0;i<v.count;++i){const auto& a=v.actors[i];d.offender=a;d.offender_metrics=metrics(a.xyz);
  if(!identifiable(a,f))return deny(GuardReason::ActorCensusIncomplete);
  if(!d.offender_metrics.valid)return deny(GuardReason::DistanceInvalid);
  for(unsigned j=0;j<i;++j)if(v.actors[j].id==a.id)return deny(GuardReason::ActorCensusIncomplete);
  if(a.is_link){++links;if(!same_actor(a,v.link))return deny(GuardReason::LinkUnknown);continue;}
  if(a.id==v.link.id)return deny(GuardReason::LinkUnknown); // only the explicitly matched live Link is excluded.
  if(d.offender_metrics.xz<actor_min_xz)return deny(GuardReason::ActorNearC);
 }
 if(links!=1)return deny(GuardReason::LinkUnknown);
 d.allowed=true;d.reason=GuardReason::None;d.offender={};d.offender_metrics={};return d;
}
GuardReason freshness(const first::Frame& previous,const first::Frame& current) noexcept {
 const auto& a=previous.pilot;const auto& b=current.pilot;
 if(b.sequence<=a.sequence||b.sequence==unknown||a.update_index!=b.update_index||a.context!=b.context)return GuardReason::CaptureStale;
 if(!same_actor(a.link,b.link))return GuardReason::LinkChanged;
 if(a.count!=b.count)return GuardReason::ActorChanged;
 for(unsigned i=0;i<a.count;++i)if(!same_actor(a.actors[i],b.actors[i]))return GuardReason::ActorChanged;
 return GuardReason::None;
}
}
