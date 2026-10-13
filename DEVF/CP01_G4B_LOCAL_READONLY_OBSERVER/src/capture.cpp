#include "capture.hpp"
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
namespace devf::g4b {
namespace { bool finite3(const std::array<double,3>& p) { for(auto v:p)if(!std::isfinite(v))return false;return true; } }
double distance_to_C(const std::array<double,3>& p) noexcept {
    if(!finite3(p))return -1;
    double q=0;for(unsigned i=0;i<3;++i){const auto d=p[i]-C[i];q+=d*d;}return std::sqrt(q);
}
bool sphere_box_candidate(const std::array<double,3>& lo,const std::array<double,3>& hi,double& distance) noexcept {
    distance=-1;if(!finite3(lo)||!finite3(hi))return false;
    double d2=0;for(unsigned i=0;i<3;++i){if(lo[i]>hi[i])return false;const auto d=std::max({lo[i]-C[i],0.0,C[i]-hi[i]});d2+=d*d;}
    distance=std::sqrt(d2);return distance<=radius;
}
Occupancy classify_actor(const Actor& a) noexcept {
    if(a.id==unknown||a.owner==unknown||!a.owner_valid||!a.initialized||a.room<0||a.room>=64||!finite3(a.xyz)||!std::isfinite(a.distance)||a.distance<0)
        return Occupancy::Invalid;
    if(a.bound_proven) {
        if(!std::isfinite(a.bound)||a.bound<0)return Occupancy::Invalid;
        return a.distance>radius+a.bound?Occupancy::SeparatedUnderBound:Occupancy::OverlapUnderBound;
    }
    return a.distance<=radius?Occupancy::Proximate:Occupancy::OverlapNotVerified;
}
void evaluate(Capture& c) noexcept {
    bool invalid=c.actor_count>max_actors||c.bg_count>max_bg||c.visited>max_visits||c.bg_slots>256;
    if(c.actor_count>max_actors)c.actor_count=max_actors;
    if(c.bg_count>max_bg)c.bg_count=max_bg;
    for(unsigned i=0;i<c.actor_count;++i){
        auto& a=c.actors[i];a.distance=distance_to_C(a.xyz);a.occupancy=classify_actor(a);
        if(a.occupancy==Occupancy::Invalid)invalid=true;
        for(unsigned j=0;j<i;++j)if(c.actors[j].id==a.id)invalid=true;
    }
    for(const auto* s:{&c.ground,&c.water}){
        if(s->status==Status::Invalid || s->status==Status::NotVerified || s->status==Status::UnavailableSafeApi)invalid=true;
        if(s->status==Status::Observed &&
        (!std::isfinite(s->y)||s->y>=C[1]+501||s->bg<0||s->bg>=256||s->poly<0||s->room<0||s->room>=64))invalid=true;
    }
    for(unsigned i=0;i<c.bg_count;++i){double distance;const bool hit=sphere_box_candidate(c.bg[i].lo,c.bg[i].hi,distance);
        if(!hit||distance<0||c.bg[i].index<0||c.bg[i].index>=256||!c.bg[i].ready||!c.bg[i].moving)invalid=true;
        c.bg[i].distance=distance;
    }
    const bool compatible=a2::validate_owner(c.owner)==a2::Reason::None && c.context_stable && c.event_read && !c.event_active;
    c.session=(!c.event_read||!c.context_stable)?Session::Incomplete:
        compatible?Session::CompatibleObserved:Session::IncompatibleObserved;
    c.capture_valid=compatible&&!invalid&&!c.budget_exceeded&&c.actors_complete&&!c.actors_truncated&&c.bg_scan_complete&&!c.bg_truncated;
    // Independent APIs can succeed; physical clearance/volume coverage always remains partial.
}
bool format(const Capture& c,const char* host,const char* build,char* out,std::size_t cap) noexcept {
    if(!cap)return false;std::size_t used=0;bool ok=true;
    auto add=[&](const char* fmt,...){if(!ok)return;va_list a;va_start(a,fmt);int n=std::vsnprintf(out+used,cap-used,fmt,a);va_end(a);
        if(n<0||std::size_t(n)>=cap-used){ok=false;out[cap-1]=0;}else used+=n;};
    add("G4B DISARMED_NO_REGISTRATION id=%u host=%s build=%s C=(%.7f,%.12f,%.7f) r=501 query_f32=(%.7f,%.7f,%.7f) stage=%.8s room=%d logical=%d next=%d next_stage=%d base=%u owner_result=%u root=%u previous=%u current_at_preflight=%u link_owner=%u node=%u node_profile=%u node_init=%d room_param=%d child=%u child_registered=%d play=%u play_profile=%u play_init=%d play_layer=%u parent=%u binding=%d counts=(%d,%d,%d,%d) status_2e=%u mask_read=%d retiring=%d stable=%d session=%u event_read=%d event_active=%d valid=%d ",
        c.id,host,build,C[0],C[1],C[2],double(float(C[0])),double(float(C[1]+501)),double(float(C[2])),c.owner.stage.data(),c.owner.room,c.owner.logical,c.owner.next_room,c.details.next_stage,unsigned(c.base),unsigned(a2::validate_owner(c.owner)),c.owner.root,c.owner.previous,c.owner.previous,c.owner.link_owner,c.owner.node,c.details.node_profile,c.details.node_init,c.details.room_param,c.owner.child,c.owner.child_registered,c.owner.play,c.details.play_profile,c.details.play_init,c.owner.play_layer,c.owner.parent,c.owner.binding_ok,c.owner.creates,c.owner.deletes,c.owner.parent_creates,c.owner.parent_deletes,c.details.status_mask,c.details.mask_read,c.owner.retiring,c.context_stable,unsigned(c.session),c.event_read,c.event_active,c.capture_valid);
    auto surface=[&](const char* name,const Surface& s){add("%s=(status=%u y=%.7f bg=%d poly=%d room=%d ground=%d special=%d color=%d actor=%u moving=%d) ",name,unsigned(s.status),s.y,s.bg,s.poly,s.room,s.ground,s.special,s.color,s.actor,s.moving);};
    surface("ground",c.ground);surface("water_group_probe",c.water);
    add("water_volume=UNAVAILABLE_WITH_SAFE_API bg_exact=UNAVAILABLE_WITH_SAFE_API bg_scan=%d bg_truncated=%d bg_candidates=%u ",c.bg_scan_complete,c.bg_truncated,c.bg_count);
    for(unsigned i=0;i<c.bg_count&&i<max_bg;++i){const auto& b=c.bg[i];add("BG[%u]=(idx=%d actor=%u owner_valid=%d room=%d locked=%d d_aabb=%.3f lo=%.3f,%.3f,%.3f hi=%.3f,%.3f,%.3f) ",i,b.index,b.actor,b.owner_valid,b.room,b.locked,b.distance,b.lo[0],b.lo[1],b.lo[2],b.hi[0],b.hi[1],b.hi[2]);}
    add("visits=%u incomplete_actor=%u incomplete_profile=%u incomplete_init=%d actors_complete=%d actors_truncated=%d local_count=%u ",c.visited,c.incomplete_actor,c.incomplete_profile,c.incomplete_init,c.actors_complete,c.actors_truncated,c.actor_count);
    for(unsigned i=0;i<c.actor_count&&i<max_actors;++i){const auto& a=c.actors[i];add("A[%u]=(id=%u profile=%u room=%d owner=%u init=%d xyz=%.3f,%.3f,%.3f d3=%.3f reason=%u occupancy=%u bound_known=%d) ",i,a.id,a.profile,a.room,a.owner,a.initialized,a.xyz[0],a.xyz[1],a.xyz[2],a.distance,a.included,unsigned(a.occupancy),a.bound_proven);}
    add("resource_read=%d E_OC_registered=%d E_ocb_registered=%d READY=NOT_VERIFIED overlap=OVERLAP_NOT_VERIFIED coverage=G4B_PARTIAL_SAFE_API_COVERAGE bg_transform=UNAVAILABLE_WITH_SAFE_API local_coach_escort_flags=NOT_VERIFIED no_location_admission=1",c.resources_read,c.e_oc_registered,c.e_ocb_registered);
    return ok;
}
}
