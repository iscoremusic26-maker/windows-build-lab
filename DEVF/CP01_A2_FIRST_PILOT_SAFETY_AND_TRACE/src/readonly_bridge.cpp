#include "experiment_core.hpp"
#include "census_values.hpp"
#include "capture.hpp"
#include <f_op/f_op_actor.h>
#include <cmath>
namespace devf::pilot {
namespace {std::uint64_t update_index=0;Id capture_sequence=0;}
void set_capture_update(std::uint64_t u) noexcept{update_index=u;}
}
namespace devf::first {
Frame read_only_preflight() {
 Frame f;
 if(devf::pilot::capture_sequence>=unknown-1)return f;
 const Id seq=++devf::pilot::capture_sequence;
 devf::pilot::census_values().reset(seq,devf::pilot::update_index);
 const auto capture=devf::g4b::capture_native(seq);const auto& in=capture.owner;auto& o=f.owner;
#define COPY(x) o.x=in.x
 COPY(host_ok);COPY(thread_ok);COPY(registry_ok);COPY(player_ok);COPY(node_present);COPY(node_profile);COPY(node_complete);COPY(node_id_ok);
 COPY(child_registered);COPY(binding_ok);COPY(play_ok);COPY(previous_registered);COPY(transition);COPY(retiring);COPY(stage);
 COPY(room);COPY(logical);COPY(next_room);COPY(node);COPY(child);COPY(parent);COPY(play);COPY(play_layer);COPY(previous);COPY(root);COPY(link_owner);
 COPY(creates);COPY(deletes);COPY(parent_creates);COPY(parent_deletes);
#undef COPY
 f.stable=capture.context_stable;f.event_read=capture.event_read;f.event_active=capture.event_active;
 f.registered_eoc=capture.e_oc_registered;f.registered_weapon=capture.e_ocb_registered;
 f.medium_observed=capture.capture_valid&&capture.ground.status==devf::g4b::Status::Observed&&capture.ground.bg==0&&capture.ground.room==3&&
  capture.ground.poly==2174&&capture.ground.ground==0&&capture.ground.special==0&&!capture.ground.moving&&
  std::isfinite(capture.ground.y)&&std::abs(capture.ground.y-candidate[1])<=0.01&&
  capture.water.status==devf::g4b::Status::NoHitAtProbe&&capture.bg_scan_complete&&!capture.bg_truncated&&capture.bg_count==0;
 auto& v=f.pilot;v=devf::pilot::census_values().values();
 v.context={o.node,o.child,o.play,o.parent,o.room,o.logical};
 v.coherent=capture.id==seq&&capture.context_stable;
 v.actors_complete=capture.actors_complete;v.actors_truncated=v.actors_truncated||capture.actors_truncated;
 v.visited=capture.visited;v.bg_complete=capture.bg_scan_complete;v.bg_truncated=capture.bg_truncated;v.bg_candidates=capture.bg_count;
 v.resources_read=capture.resources_read;v.water_status=unsigned(capture.water.status);
 const auto& g=capture.ground;v.ground={unsigned(g.status),g.y,g.bg,g.poly,g.room,g.ground,g.special,g.moving};
 v.link_read=capture.link_read;v.link_profile_checked=capture.link_read&&capture.link.profile==fpcNm_ALINK_e;
 const auto& link=capture.link;v.link.id=link.id;v.link.profile=link.profile;v.link.owner=link.owner;v.link.room=link.room;v.link.init=link.initialized?2:-1;
 v.link.xyz=link.xyz;v.link.initialized=link.initialized;v.link.owner_valid=link.owner_valid;v.link.is_link=true;
 bool speed_read=false;
 for(unsigned i=0;i<v.count;++i)if(v.actors[i].is_link&&v.actors[i].id==link.id){
  if(speed_read){v.duplicates=true;continue;}speed_read=true;v.link.speed=v.actors[i].speed;v.link.speed_forward=v.actors[i].speed_forward;
 }
 if(!speed_read)v.link_read=false;
 return f; // public actor fields/census are scalar copies only; historical files never changed.
}
}
