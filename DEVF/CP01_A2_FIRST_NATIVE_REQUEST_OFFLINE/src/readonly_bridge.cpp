#include "experiment_core.hpp"
#include "../../CP01_G4B_LOCAL_READONLY_OBSERVER/src/capture.hpp"
#include <cmath>
namespace devf::first {
Frame read_only_preflight() {
 const auto capture=devf::g4b::capture_native(0);const auto& in=capture.owner;Frame f;auto& o=f.owner;
#define COPY(field) o.field=in.field
 COPY(host_ok);COPY(thread_ok);COPY(registry_ok);COPY(player_ok);COPY(node_present);COPY(node_profile);COPY(node_complete);COPY(node_id_ok);
 COPY(child_registered);COPY(binding_ok);COPY(play_ok);COPY(previous_registered);COPY(transition);COPY(retiring);COPY(stage);
 COPY(room);COPY(logical);COPY(next_room);COPY(node);COPY(child);COPY(parent);COPY(play);COPY(play_layer);COPY(previous);COPY(root);COPY(link_owner);
 COPY(creates);COPY(deletes);COPY(parent_creates);COPY(parent_deletes);
#undef COPY
 f.stable=capture.context_stable;f.event_read=capture.event_read;f.event_active=capture.event_active;
 f.registered_eoc=capture.e_oc_registered;f.registered_weapon=capture.e_ocb_registered;
 f.medium_observed=capture.capture_valid&&capture.ground.status==devf::g4b::Status::Observed&&capture.ground.room==3&&
  capture.ground.poly==2174&&capture.ground.ground==0&&capture.ground.special==0&&!capture.ground.moving&&
  std::isfinite(capture.ground.y)&&std::abs(capture.ground.y-candidate[1])<=0.01&&
  capture.water.status==devf::g4b::Status::NoHitAtProbe&&capture.bg_count==0;
 // Medium precheck is scoped evidence only; Admission remains independently mandatory.
 return f;
}
}
