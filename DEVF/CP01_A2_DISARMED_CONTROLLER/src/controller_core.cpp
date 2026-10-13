#include "controller_core.hpp"
#include <cstring>
namespace devf::a2 {
Context context_of(const Owner& o) noexcept { return {o.node,o.child,o.play,o.parent,o.room,o.logical}; }
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
bool same_preflight(const Owner& a,const Owner& b) noexcept {
    return validate_owner(b)==Reason::None && context_of(a)==context_of(b) &&
        a.previous==b.previous && a.root==b.root;
}
bool Controller::begin_simulation(const Owner& o,bool permission,bool position) noexcept {
    if (busy_) { fail(Reason::Reentry,true); return false; }
    if (consumed_ || state_==State::StopSession || state_==State::Failed) return false; // Includes failed/reload/deactivate paths; no reset API.
    if (!permission) { state_=State::Disarmed; reason_=Reason::Disarmed; return false; }
    auto r=validate_owner(o);
    if (r==Reason::None && !position) r=Reason::Position;
    if (r!=Reason::None) { state_=State::ValidationRejected; reason_=r; return false; }
    consumed_=true; busy_=true; context_=context_of(o); elapsed_=0;
    state_=State::AttemptConsumed; reason_=Reason::None;
    return true;
}
void Controller::requested(Id id) noexcept {
    if (state_!=State::AttemptConsumed) return; // Reentrant stop cannot be overwritten by success.
    if (id==unknown) { fail(Reason::InvalidId,true); return; }
    id_=id; state_=State::Requested;
}
void Controller::fail(Reason r,bool uncertain) noexcept {
    state_=uncertain ? State::StopSession : State::Failed; reason_=r;
}
void Controller::disarm() noexcept {
    if (consumed_) fail(Reason::Disarmed,true);
    else { state_=State::Disarmed; reason_=Reason::Disarmed; }
}
void Controller::observe(const Owner& o,const Observation& a) noexcept {
    if (state_!=State::Requested && state_!=State::Pending && state_!=State::ReadyObserved) return;
    // Creation count may include our request. It is not a preflight rejection after enqueue.
    Owner monitor=o; monitor.creates=0; monitor.parent_creates=0;
    if (validate_owner(monitor)!=Reason::None || context_of(o)!=context_) {
        fail(Reason::Context,true); return;
    }
    if (a.reused || a.ambiguous || a.id!=id_) { fail(Reason::Identity,true); return; }
    if (a.failed) { fail(Reason::Phase); return; }
    if (a.removed) { fail(Reason::Removed,true); return; }
    if (a.visible && (a.profile!=Preset::profile || a.owner!=context_.child || a.room!=context_.room)) {
        fail(Reason::Identity,true); return;
    }
    if (state_==State::ReadyObserved) {
        if (a.pending || !a.visible || !a.initialized || !a.actor_init || !a.resources)
            fail(Reason::Ambiguous,true);
        return;
    }
    if (a.pending || (a.visible && (!a.initialized || !a.actor_init))) {
        if (++elapsed_>=timeout_) fail(Reason::Timeout,true);
        else state_=State::Pending;
        return;
    }
    if (!a.visible) { fail(Reason::MissingActor,true); return; }
    if (!a.resources) { fail(Reason::Resources,true); return; }
    state_=State::ReadyObserved; reason_=Reason::None;
}
}
