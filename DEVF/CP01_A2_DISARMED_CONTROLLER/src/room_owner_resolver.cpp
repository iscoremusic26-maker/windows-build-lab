#include "room_owner_resolver.hpp"
#include "../../CP01_G2_READONLY_PACKAGE/src/core.hpp"
#include <d/d_com_inf_game.h>
#include <f_op/f_op_scene_mng.h>
#include <f_pc/f_pc_layer.h>
#include <f_pc/f_pc_manager.h>
namespace devf::a2 {
Owner capture_owner(bool host_verified,bool update_context) {
    Owner o; o.host_ok=host_verified; o.thread_ok=update_context;
    if (!host_verified || !update_context) return o;
    // Reuse the EXACT historical G2 reader, registry, bounded player/census and completeness.
    const auto s=devf::a1::capture_native();
    o.stage=s.stage; o.room=s.room; o.logical=s.logical_layer;
    o.previous=s.current_layer; o.node=s.room_node; o.child=s.room_layer;
    o.parent=s.room_owner; o.link_owner=s.link_owner;
    o.creates=s.room_creates; o.deletes=s.room_deletes;
    o.transition=s.status==devf::a1::Observation::Transition;
    if (s.status!=devf::a1::Observation::Captured) return o;
    using Layers=devf::a1::Registry<layer_class>;
    Layers layers; layers.collect(fpcLy_RootLayer(),[](const layer_class* l) {
        return reinterpret_cast<const layer_class*>(l->node.mpNextNode);
    });
    auto id_of=[&](const layer_class* p,Id& id) {
        return layers.id_of(p,id,[](const layer_class* l) { return l->layer_id; }) && id!=unknown;
    };
    o.registry_ok=layers.complete && id_of(fpcLy_RootLayer(),o.root);
    o.previous_registered=id_of(fpcLy_CurrentLayer(),o.previous) && o.previous==s.current_layer;
    auto* room=fopScnM_SearchByID(dStage_roomControl_c::getStatusProcID(s.room));
    const auto play_id=dStage_roomControl_c::getProcID(); // Set by dScnPly_Create, official code.
    auto* play=fopScnM_SearchByID(play_id);
    o.node_present=room!=nullptr;
    if (!room || !play) return o;
    o.node_profile=room->base.base.profname==fpcNm_ROOM_SCENE_e;
    o.node_id_ok=room->base.base.id==s.room_node && room->base.base.parameters==unsigned(s.room);
    o.node_complete=room->base.base.state.init_state==2 && !fpcM_IsCreating(s.room_node);
    o.play=play_id;
    o.play_ok=play->base.base.id==play_id && play->base.base.profname==fpcNm_PLAY_SCENE_e &&
        play->base.base.state.init_state==2 && !fpcM_IsCreating(play_id) && id_of(&play->base.layer,o.play_layer);
    Id child=unknown,parent=unknown;
    o.child_registered=id_of(&room->base.layer,child) && child==s.room_layer;
    o.binding_ok=o.play_ok && o.child_registered && id_of(room->base.base.layer_tag.layer,parent) &&
        parent==s.room_owner && room->base.base.layer_tag.layer==&play->base.layer &&
        room->base.layer.process_node==&room->base && play->base.layer.process_node==&play->base;
    if (!o.binding_ok) return o;
    o.creates=room->base.layer.counts.create_count; o.deletes=room->base.layer.counts.delete_count;
    o.parent_creates=play->base.layer.counts.create_count; o.parent_deletes=play->base.layer.counts.delete_count;
    o.next_room=dStage_roomControl_c::getNextStayNo();
    o.retiring=dComIfGp_roomControl_checkStatusFlag(s.room,0x2e)!=0 ||
        fpcLy_IsDeletingMesg(&room->base.layer) || fpcLy_IsDeletingMesg(&play->base.layer);
    o.transition=dComIfGp_isEnableNextStage() || o.next_room!=o.room;
    o.player_ok=true; // G2 already validated a live initialized Link in the public actor queue.
    // Scalar second check: no borrowed native pointer is stored after this function.
    if (dStage_roomControl_c::getStatusProcID(s.room)!=o.node ||
        dStage_roomControl_c::getProcID()!=o.play || dStage_roomControl_c::getStayNo()!=o.room ||
        dComIfG_play_c::getLayerNo(0)!=o.logical) o.transition=true;
    return o;
}
}
