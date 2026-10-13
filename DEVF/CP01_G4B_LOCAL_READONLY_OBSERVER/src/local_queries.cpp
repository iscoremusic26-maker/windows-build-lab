#include "capture.hpp"
#include "../../CP01_A2_DISARMED_CONTROLLER/src/room_owner_resolver.hpp"
#include <d/d_com_inf_game.h>
#include <d/d_bg_s_gnd_chk.h>
#include <f_op/f_op_scene_mng.h>
#include <cmath>
#include <cstring>
#include <f_pc/f_pc_layer.h>
namespace devf::g4b {
void collect_bg(Capture& c) {
    auto& bg=dComIfG_Bgsp();c.bg_scan_complete=true;
    for(unsigned i=0;i<256;++i){++c.bg_slots;const auto& e=bg.m_chk_element[i];if(!e.ChkUsed())continue;
        auto* b=e.m_bgw_base_ptr;
        if(!b||b->GetId()!=int(i)){c.bg_scan_complete=false;continue;}
        if(b->ChkNotReady()){c.bg_scan_complete=false;continue;}
        if(!b->ChkMoveBg())continue;
        const auto* a=b->GetBnd();if(!a){c.bg_scan_complete=false;continue;}
        const std::array<double,3> lo{a->mMin.x,a->mMin.y,a->mMin.z},hi{a->mMax.x,a->mMax.y,a->mMax.z};
        double d;const bool near=sphere_box_candidate(lo,hi,d);if(d<0){c.bg_scan_complete=false;continue;}if(!near)continue;
        if(c.bg_count==max_bg){c.bg_truncated=true;continue;}
        auto& out=c.bg[c.bg_count++];out.index=int(i);out.actor=e.m_actor_id;out.room=b->GetRoomId();
        out.lo=lo;out.hi=hi;out.distance=d;out.ready=true;out.moving=true;out.locked=b->ChkLock();
        // Registered pointer never dereferenced here. Its ID is validated by G2 actor visitor.
    }
}
namespace {
template<class Q> Surface query() {
    Surface s;Q q;const cXyz probe{float(C[0]),float(C[1]+501),float(C[2])};q.SetPos(&probe);
    auto& bg=dComIfG_Bgsp();
    // Explicit qualified base method avoids the DEBUG host timer/counter wrapper.
    const auto y=bg.cBgS::GroundCross(&q);
    if(y==-1.0e9f){s.status=Status::NoHitAtProbe;return s;}
    if(!std::isfinite(y)||y>=probe.y||!bg.ChkPolySafe(q)){s.status=Status::Invalid;return s;}
    const auto i=q.GetBgIndex();if(i<0||i>=256){s.status=Status::Invalid;return s;}
    s.status=Status::Observed;s.y=y;s.bg=i;s.poly=q.GetPolyIndex();s.room=bg.GetRoomId(q);
    s.ground=bg.GetGroundCode(q);s.special=bg.GetSpecialCode(q);s.color=bg.GetPolyColor(q);
    const auto& e=bg.m_chk_element[i];s.actor=e.m_actor_id;
    if(e.m_bgw_base_ptr)s.moving=e.m_bgw_base_ptr->ChkMoveBg();
    return s; // q and all native pointers die on this stack.
}
void details(Capture& c) {
    if(c.owner.room<0||c.owner.room>=64)return;
    c.details.next_stage=dComIfGp_isEnableNextStage()!=0;
    c.details.status_mask=dComIfGp_roomControl_checkStatusFlag(c.owner.room,0x2e);c.details.mask_read=true;
    const auto* room=fopScnM_SearchByID(dStage_roomControl_c::getStatusProcID(c.owner.room));
    if(room){c.details.node_profile=room->base.base.profname;c.details.node_init=room->base.base.state.init_state;c.details.room_param=int(room->base.base.parameters);}
    const auto* play=fopScnM_SearchByID(dStage_roomControl_c::getProcID());
    if(play){c.details.play_profile=play->base.base.profname;c.details.play_init=play->base.base.state.init_state;}
}
}
Capture capture_native(Id id) {
    // A2 calls the adapted G2 single actor pass, which fills this owning-value sidecar.
    const auto o=a2::capture_owner(true,true);auto c=sidecar();c.id=id;c.owner=o;details(c);
    if(a2::validate_owner(o)==a2::Reason::None){
        // Never dispatch GroundCross through a registry already observed malformed/unready.
        if(c.bg_scan_complete){c.ground=query<dBgS_ObjGndChk>();c.water=query<dBgS_ObjGndChk_Wtr>();}
        c.event_active=dComIfGp_event_runCheck()!=0;c.event_read=true;
        auto* room=fopScnM_SearchByID(o.node);auto* play=fopScnM_SearchByID(o.play);
        c.context_stable=room && play && room->base.base.id==o.node && play->base.base.id==o.play &&
            room->base.base.profname==fpcNm_ROOM_SCENE_e && play->base.base.profname==fpcNm_PLAY_SCENE_e &&
            room->base.base.state.init_state==2 && play->base.base.state.init_state==2 &&
            room->base.layer.layer_id==o.child && play->base.layer.layer_id==o.play_layer &&
            room->base.base.layer_tag.layer==&play->base.layer &&
            !room->base.layer.counts.create_count && !room->base.layer.counts.delete_count &&
            !play->base.layer.counts.create_count && !play->base.layer.counts.delete_count &&
            !fpcLy_IsDeletingMesg(&room->base.layer) && !fpcLy_IsDeletingMesg(&play->base.layer) &&
            dComIfGp_getStartStageName() && !std::strncmp(dComIfGp_getStartStageName(),o.stage.data(),8) && !dComIfGp_isEnableNextStage() && dStage_roomControl_c::getStayNo()==o.room &&
            dStage_roomControl_c::getNextStayNo()==o.next_room && dComIfG_play_c::getLayerNo(0)==o.logical &&
            dStage_roomControl_c::getStatusProcID(o.room)==o.node && dStage_roomControl_c::getProcID()==o.play &&
            (fpcLy_CurrentLayer() && fpcLy_CurrentLayer()->layer_id==o.previous) && !dComIfGp_roomControl_checkStatusFlag(o.room,0x2e);
    }
    evaluate(c);return c;
}
}
