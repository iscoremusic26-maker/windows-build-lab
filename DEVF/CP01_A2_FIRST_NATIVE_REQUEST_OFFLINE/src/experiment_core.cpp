#include "experiment_core.hpp"
#include <cstring>
#include <cmath>
#include <limits>
namespace devf::first {
Key key(const Owner& o) noexcept {return {o.node,o.child,o.play,o.parent,o.room,o.logical};}
Reason validate_owner(const Owner& o) noexcept {
    if (!o.host_ok) return Reason::Host;
    if (!o.thread_ok) return Reason::Thread;
    if (std::memcmp(o.stage.data(),"F_SP121\0",8) || o.stage[8] || o.room!=3 || o.logical!=6)
        return Reason::Context;
    if (!o.registry_ok || o.root==unknown) return Reason::Registry;
    if (!o.player_ok || o.link_owner==unknown) return Reason::Player;
    if (!o.node_present || !o.node_profile || !o.node_complete || !o.node_id_ok || o.node==unknown)
        return Reason::Node;
    if (!o.child_registered || !o.binding_ok || !o.play_ok || o.child==unknown ||
        o.child==o.root || o.child==o.parent || o.parent==unknown || o.play==unknown || o.parent!=o.play_layer)
        return Reason::Binding;
    if (o.transition || o.retiring || o.next_room!=o.room) return Reason::Transition;
    if (o.creates!=0 || o.deletes!=0 || o.parent_creates!=0 || o.parent_deletes!=0)
        return Reason::PendingContext;
    if (!o.previous_registered || o.previous==unknown) return Reason::Previous;
    return Reason::None;
}

bool context_valid(const Frame& f) noexcept {
 auto o=f.owner;o.creates=o.parent_creates=0; // pending creates attributed separately by monitor
 return validate_owner(o)==Reason::None && f.stable && f.event_read && !f.event_active;
}
bool same_context(const Owner& a,const Owner& b) noexcept {return key(a)==key(b)&&a.root==b.root;}
bool convert_candidate(const std::array<double,3>& xyz,Preset& p) noexcept {
 for(unsigned i=0;i<3;++i){if(!std::isfinite(xyz[i])||std::abs(xyz[i])>std::numeric_limits<float>::max())return false;
  p.xyz[i]=static_cast<float>(xyz[i]);if(!std::isfinite(p.xyz[i]))return false;}
 return xyz==candidate;
}
bool LayerScope::restore() noexcept {
 if(restored_)return ok_;restored_=true;
 try {if(!backend_.registered(previous_))return false;backend_.select(previous_);ok_=backend_.current()==previous_;}
 catch(...){ok_=false;}return ok_;
}
void Controller::attempt(Backend& b,const Admission& permission) noexcept {
 if(busy_){stop(Reason::Reentry);return;}
 if(consumed_||terminal()||state_==State::ActorReadyObserved)return;
 if(!permission.location_admitted||!permission.session_authorized){state_=State::Blocked;reason_=Reason::Admission;return;}
 busy_=true;struct End{bool& busy;~End(){busy=false;}} end{busy_};
 try {
  const auto first=b.preflight();reason_=validate_owner(first.owner);
  if(reason_==Reason::None&&!context_valid(first))reason_=Reason::Transition;
  if(reason_==Reason::None&&!first.medium_observed)reason_=Reason::Medium;
  if(reason_==Reason::None&&(!first.registered_eoc||!first.registered_weapon))reason_=Reason::Resources;
  Preset preset;if(reason_==Reason::None&&!convert_candidate(permission.xyz,preset))reason_=Reason::Position;
  if(reason_==Reason::None&&permission.context!=key(first.owner))reason_=Reason::Admission;
  if(reason_==Reason::None&&!b.service_available())reason_=Reason::Service;
  if(reason_!=Reason::None){state_=State::Rejected;return;}
  const auto fresh=b.preflight();
  if(validate_owner(fresh.owner)!=Reason::None||!context_valid(fresh)||!fresh.medium_observed||!fresh.registered_eoc||!fresh.registered_weapon||!same_context(first.owner,fresh.owner)||first.owner.previous!=fresh.owner.previous){stop(Reason::Revalidation);return;}
  if(b.current()!=fresh.owner.previous||!b.registered(fresh.owner.previous)||!b.registered(fresh.owner.child)){stop(Reason::Previous);return;}
  origin_=fresh.owner;consumed_=true;state_=State::AttemptConsumed; // BEFORE global setter / potential enqueue
  LayerScope scope(b,origin_.previous);
  try {
   if(!scope.select(origin_.child))stop(Reason::Selection);
   else {
    const auto selected=b.preflight();
    if(terminal()){} // Reentry is sticky; never overwritten by later success.
    else if(!context_valid(selected)||validate_owner(selected.owner)!=Reason::None||!selected.medium_observed||!selected.registered_eoc||!selected.registered_weapon||!same_context(origin_,selected.owner)||b.current()!=origin_.child)stop(Reason::Revalidation);
    else {
     const auto result=b.request(preset); // ONE backend request point; native service implementation is separate.
     if(result.id!=unknown)id_=result.id; // Keep diagnostic ID even if reentry already forced STOP.
     if(terminal()){}
     else if(result.result!=0)stop(Reason::Service);
     else if(result.id==unknown||result.id==0||result.id>=0x7ffffffeu)stop(Reason::InvalidId);
     else {state_=State::Requested;reason_=Reason::None;}
    }
   }
  }catch(...){stop(Reason::Exception);}
  if(!scope.restore())stop(Reason::Restore);
  if(!terminal() && state_==State::Requested){
   const auto after=b.observe(id_);
   if(!context_valid(after.frame)||!same_context(origin_,after.frame.owner)||b.current()!=origin_.previous)stop(Reason::Revalidation);
   else if(!after.actor_scan_complete||!after.request_scan_complete||after.ambiguous||after.id!=id_)stop(Reason::Ambiguous);
   else if(after.request_present&&after.request_owner!=origin_.child)stop(Reason::Queue);
  }
 }catch(...){stop(Reason::Exception);}
}
void Controller::poll(Backend& b) noexcept {
 if(busy_){stop(Reason::Reentry);return;}
 if(state_!=State::Requested&&state_!=State::Pending&&state_!=State::ActorReadyObserved)return;
 busy_=true;struct End{bool& busy;~End(){busy=false;}} end{busy_};
 try {
  const auto a=b.observe(id_);if(terminal())return;
  ++polls_;
  if(!context_valid(a.frame)||!same_context(origin_,a.frame.owner)){stop(Reason::Context);return;}
  if(!a.actor_scan_complete||!a.request_scan_complete||a.ambiguous||a.reused){stop(Reason::Ambiguous);return;}
  if(a.id!=id_){stop(Reason::Identity);return;}
  if(a.removed||a.request_cancelled){stop(Reason::Removed);return;}
  if(a.creating!=a.request_present||(a.request_present&&a.request_owner!=origin_.child)){stop(Reason::Queue);return;}
  if(a.actor_present&&a.profile!=0x1fe){stop(Reason::Identity);return;}
  if(a.actor_present&&a.init==3){stop(Reason::Removed);return;}
  // Own queued request increments exactly its layer create_count; other counts are NOT attributed by guess.
  const auto& o=a.frame.owner;
  if(o.creates<0||o.parent_creates<0||o.creates>(a.creating?1:0)||o.parent_creates!=0){stop(Reason::CounterAttribution);return;}
  if(a.creating&&o.creates!=1){stop(Reason::CounterAttribution);return;}
  if(a.actor_present&&a.init==2&&a.derived_read){
   if(a.owner!=origin_.child||a.room!=origin_.room||!a.room_api_ok){stop(Reason::Identity);return;}
  }
  const bool ready=a.actor_present&&!a.creating&&a.init==2&&a.actor_init&&a.derived_read&&a.room_api_ok&&a.owner==origin_.child&&a.room==origin_.room;
  if(ready){state_=State::ActorReadyObserved;reason_=Reason::None;ready_seen_=true;missing_=0;if(++ready_polls_>60)stop(Reason::ObservationBudget);return;}
  if(ready_seen_){stop(Reason::Removed);return;}
  state_=State::Pending;
  if(!a.actor_present&&!a.creating && ++missing_>3){stop(Reason::MissingActor);return;}
  if(polls_>=timeout_)stop(Reason::Timeout);
 }catch(...){stop(Reason::Exception);}
}
}
