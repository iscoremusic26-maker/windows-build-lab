#include "../../CP01_G2_READONLY_PACKAGE/src/core.hpp"
#include "capture.hpp"
#include <cmath>
#include <d/d_com_inf_game.h>
#include <f_op/f_op_actor_iter.h>
#include <f_op/f_op_actor_mng.h>
#include <f_op/f_op_scene_mng.h>
#include <f_pc/f_pc_manager.h>
#include <f_pc/f_pc_layer.h>
#include <cstddef>
#include <cstring>
namespace devf::g4b { namespace { Capture current_values; } Capture& sidecar() noexcept { return current_values; } }
namespace devf::a1 {
namespace {
using Layers = Registry<layer_class>;
static_assert(std::is_standard_layout_v<layer_class> && offsetof(layer_class, node) == 0);
bool layer_id(const Layers& layers, const layer_class* p, std::uint32_t& id) {
    return layers.id_of(p, id, [](const layer_class* l) { return l->layer_id; }) && id != kUnknown;
}
struct Match { const void* target; unsigned visited = 0; bool found = false; };
void* match_actor(void* actor, void* data) {
    auto& m = *static_cast<Match*>(data);
    if (!actor) return data; // malformed tag: stop without claiming a player match
    if (actor == m.target) { m.found = true; return actor; }
    // Returning an actor stops this synchronous walk; caller trusts only m.found.
    if (++m.visited >= 256) return actor;
    return nullptr;
}
bool complete(const base_process_class& p) {
    return p.state.init_state == 2 && !fpcM_IsCreating(p.id);
}
struct Census { Snapshot& values; const Layers& layers; unsigned visited = 0; bool incomplete = false; };
void* census_actor(void* actor, void* data) {
    auto& c = *static_cast<Census*>(data);
    if (!actor) { c.incomplete = true; return data; }
    if (++c.visited > 256) { c.values.census_truncated = true; return actor; }
    // The public queue may contain actors before their derived Create has completed.
    // Inspect only the already-established process header before reading actor fields.
    const auto* process = static_cast<const base_process_class*>(actor);
    // Extend the SAME historical bounded visitor for local owning-value diagnostics.
    if (!complete(*process)) { auto& local=devf::g4b::sidecar();local.actors_complete=false;
        local.incomplete_actor=process->id;local.incomplete_profile=process->profname;local.incomplete_init=process->state.init_state;
        c.incomplete=true; return actor; }
    const auto* a = static_cast<const fopAc_ac_c*>(actor);
    if (!(a->actor_condition & fopAcCnd_INIT_e)) { auto& local=devf::g4b::sidecar();
        local.incomplete_actor=process->id;local.incomplete_profile=process->profname;local.incomplete_init=process->state.init_state;
        c.incomplete = true; return actor; }
    auto& local=devf::g4b::sidecar();
    local.visited=c.visited;
    const std::array<double,3> xyz{a->current.pos.x,a->current.pos.y,a->current.pos.z};
    unsigned reason=(a==dComIfGp_getPlayer(0))?1u:0u;
    const auto distance=devf::g4b::distance_to_C(xyz);
    if(distance<0){local.actors_complete=false;c.incomplete=true;return actor;}
    if(distance<=devf::g4b::radius)reason|=2;
    for(unsigned i=0;i<local.bg_count;++i)if(local.bg[i].actor==a->id){
        const auto& slot=dComIfG_Bgsp().m_chk_element[local.bg[i].index];
        devf::g4b::Id owner=devf::g4b::unknown;
        local.bg[i].owner_valid=slot.m_actor_ptr==a && layer_id(c.layers,a->layer_tag.layer,owner);
        reason|=4;
    }
    if(reason){
        if(local.actor_count==devf::g4b::max_actors)local.actors_truncated=true;
        else {auto& r=local.actors[local.actor_count++];r.id=a->id;r.profile=a->profname;r.room=a->current.roomNo;
            r.xyz=xyz;r.distance=distance;r.included=reason;r.initialized=true;r.owner_valid=layer_id(c.layers,a->layer_tag.layer,r.owner);}
    }
    if (process->profname != fpcNm_E_OC_e || a->current.roomNo != c.values.room) return nullptr;
    if (c.values.census_count == c.values.census.size()) {
        c.values.census_truncated = true; return actor;
    }
    auto& r = c.values.census[c.values.census_count++];
    r.id = a->id; r.parameters = a->parameters; r.room = a->current.roomNo;
    r.home_x = a->home.angle.x;
    r.xyz = {a->current.pos.x, a->current.pos.y, a->current.pos.z};
    if (!layer_id(c.layers, a->layer_tag.layer, r.owner)) { c.incomplete = true; return actor; }
    return nullptr;
}
}
Snapshot capture_native() {
    Snapshot s;
    devf::g4b::sidecar()={};
    struct Publish { Snapshot& s; ~Publish() {auto& c=devf::g4b::sidecar();c.base=s.status;c.e_oc_registered=s.e_oc_registered;c.e_ocb_registered=s.e_ocb_registered;} } publish{s};
    // These queries are public scalar getters. No native memory is retained after this call.
    if (dComIfGp_isEnableNextStage()) { s.status = Observation::Transition; return s; }
    const char* stage = dComIfGp_getStartStageName(); // host-owned fixed name, API contract
    if (!stage) return s;
    for (std::size_t i = 0; i < 8 && stage[i]; ++i) s.stage[i] = stage[i];
    s.room = dStage_roomControl_c::getStayNo();
    if (s.room < 0 || s.room >= 64) { s.status = Observation::InvalidRoom; return s; }
    if (std::strcmp(s.stage.data(), "F_SP121") || s.room != 3) {
        s.status = Observation::WrongContext; return s;
    }
    Layers layers;
    layers.collect(fpcLy_RootLayer(), [](const layer_class* l) {
        return reinterpret_cast<const layer_class*>(l->node.mpNextNode);
    });
    if (!layers.complete || !layer_id(layers, fpcLy_CurrentLayer(), s.current_layer)) {
        s.status = Observation::InvalidRegistry; return s;
    }
    auto* player = dComIfGp_getPlayer(0);
    Match match{player};
    if (!player) { s.status = Observation::NoPlayer; return s; }
    fopAcIt_Judge(match_actor, &match); // synchronous, no callback registration or retention
    if (!match.found) { s.status = Observation::NoPlayer; return s; }
    if (player->profname != fpcNm_ALINK_e || !complete(*player) ||
        !(player->actor_condition & fopAcCnd_INIT_e) ||
        !layer_id(layers, player->layer_tag.layer, s.link_owner)) {
        s.status = Observation::IncompletePlayer; return s;
    }
    s.link_xyz = {player->current.pos.x, player->current.pos.y, player->current.pos.z};
    s.link_yaw = player->current.angle.y;
    const auto room_id = dStage_roomControl_c::getStatusProcID(s.room);
    const auto* room = fopScnM_SearchByID(room_id);
    if (!room || room->base.base.id != room_id || room->base.base.profname != fpcNm_ROOM_SCENE_e) {
        s.status = Observation::NoRoomNode; return s;
    }
    if (!complete(room->base.base) || !layer_id(layers, &room->base.layer, s.room_layer) ||
        !layer_id(layers, room->base.base.layer_tag.layer, s.room_owner)) {
        s.status = Observation::IncompleteRoom; return s;
    }
    s.room_node = room_id;
    s.room_creates = room->base.layer.counts.create_count;
    s.room_deletes = room->base.layer.counts.delete_count;
    if (s.room_creates || s.room_deletes) { s.status = Observation::IncompleteRoom; return s; }
    s.logical_layer = dComIfG_play_c::getLayerNo(0);
    // Presence in native registry is NOT proof of installed/fully loaded model or animation.
    devf::g4b::sidecar().resources_read=true;
    s.e_oc_registered = dComIfG_getObjectResInfo("E_OC") != nullptr;
    s.e_ocb_registered = dComIfG_getObjectResInfo("E_ocb") != nullptr;
    devf::g4b::collect_bg(devf::g4b::sidecar());
    devf::g4b::sidecar().actors_complete=true;
    Census census{s, layers}; fopAcIt_Judge(census_actor, &census);
    devf::g4b::sidecar().visited=census.visited;
    if(census.incomplete || s.census_truncated)devf::g4b::sidecar().actors_complete=false;
    if (census.incomplete || s.census_truncated) { s.status = Observation::IncompleteCensus; return s; }
    s.status = valid_values(s) ? Observation::Captured : Observation::InvalidValues;
    return s;
}
}
