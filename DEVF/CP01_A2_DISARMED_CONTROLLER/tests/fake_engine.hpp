#pragma once
#include "simulation.hpp"
#include <cstring>
#include <stdexcept>
#include <functional>
namespace devf::a2::lab {
inline Owner valid_owner() {
    Owner o; std::memcpy(o.stage.data(),"F_SP121",8);
    o.host_ok=o.thread_ok=o.registry_ok=o.player_ok=true;
    o.node_present=o.node_profile=o.node_complete=o.node_id_ok=true;
    o.child_registered=o.binding_ok=o.play_ok=o.previous_registered=true;
    o.room=o.next_room=3; o.logical=6; o.root=0; o.previous=9; o.node=102;
    o.child=16; o.parent=o.play_layer=15; o.play=101; o.link_owner=15;
    o.creates=o.deletes=o.parent_creates=o.parent_deletes=0;
    return o; // Synthetic layers16/15, not IDs from G2.
}
struct FakeEngine {
    Owner owner=valid_owner();
    Id current_layer=9;
    unsigned resolves=0,requests=0,selections=0;
    Id result_id=700, captured_owner=unknown;
    bool result_ok=true, throw_resolve=false, throw_select=false, select_mismatch=false;
    bool throw_request=false, throw_after_enqueue=false, dead_previous=false, fail_restore=false;
    std::function<void(FakeEngine&)> on_resolve, on_request;
    Owner resolve() {
        ++resolves; if (throw_resolve) throw std::runtime_error("synthetic resolver error");
        if(on_resolve) on_resolve(*this); return owner;
    }
    Owner resolve_selected(Id previous) { auto o=resolve(); o.previous=previous; return o; }
    Id current() const { return current_layer; }
    bool registered(Id id) const { return !(dead_previous && id==owner.previous) && id!=unknown; }
    void select(Id id) {
        ++selections;
        if ((throw_select && id==owner.child) || (fail_restore && id==owner.previous))
            throw std::runtime_error("synthetic select/restore error");
        current_layer=select_mismatch && id==owner.child ? owner.previous : id;
    }
    struct Result { bool ok; Id id; };
    Result request() {
        if(throw_request) throw std::runtime_error("synthetic exception before enqueue");
        ++requests; captured_owner=current_layer;
        if(on_request) on_request(*this);
        if(throw_after_enqueue) throw std::runtime_error("synthetic exception after enqueue");
        return {result_ok,result_id};
    }
};
inline Observation ready(Id id=700) {
    Observation a; a.id=id; a.profile=Preset::profile; a.owner=16; a.room=3;
    a.visible=a.initialized=a.actor_init=a.resources=true; return a;
}
}
