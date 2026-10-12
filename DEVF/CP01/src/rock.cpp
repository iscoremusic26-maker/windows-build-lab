#include "rock.hpp"
#include <d/d_com_inf_game.h>
#include <res/Object/WRock.h>
#include <limits>

namespace devf::cp01 {
Counters counters;
static constexpr const char* archive_name = "Wrock";
static constexpr u32 heap_size = ALIGN_NEXT(13952, 0x20) + ALIGN_NEXT(1920, 0x20);
static void count(unsigned& value) { if (value != std::numeric_limits<unsigned>::max()) ++value; }
struct RockOps {
    static constexpr int complete_step = cPhs_COMPLEATE_e, error_step = cPhs_ERROR_e,
        cancelled_step = cPhs_UNK3_e;
    Rock& rock;
    int load_archive() { return dComIfG_resLoad(&rock.phase, archive_name); }
    bool create_heap() {
        const bool success = fopAcM_entrySolidHeap(&rock, Rock::heap_callback, heap_size) != 0;
        // The engine already freed a failed attempt, including allocations recorded by the callback.
        if (!success) { rock.model = nullptr; rock.collider = nullptr; }
        return success;
    }
    bool register_collision() {
        return rock.collider && !dComIfG_Bgsp().Regist(rock.collider, &rock);
    }
    bool initialize_scene() {
        if (!rock.model) return false;
        fopAcM_SetMtx(&rock, rock.model->getBaseTRMtx());
        fopAcM_setCullSizeBox(&rock, -400, -400, -400, 400, 400, 400);
        rock.acch.Set(&rock.current.pos, &rock.old.pos, &rock, 1, &rock.circle,
            &rock.speed, &rock.current.angle, &rock.shape_angle);
        return rock.update_transform() == 1;
    }
    bool release_collision() { return !dComIfG_Bgsp().Release(rock.collider); }
    void release_archive() { dComIfG_resDelete(&rock.phase, archive_name); }
    void clear_heap_references() { rock.model = nullptr; rock.collider = nullptr; }
};
Rock::Rock() { count(counters.constructed); }
Rock::~Rock() { count(counters.destroyed); }
int Rock::create() {
    fopAcM_ct(this, Rock);
    const bool previously_ready = lifecycle.ready();
    RockOps ops{*this};
    const int phase_result = lifecycle.advance(ops);
    if (!previously_ready && lifecycle.ready()) count(counters.ready);
    return phase_result;
}
int Rock::create_heap() {
    model = nullptr;
    collider = nullptr;
    auto* data = static_cast<J3DModelData*>(dComIfG_getObjectRes(archive_name, dRes_INDEX_WROCK_BMD_WROCK_e));
    if (!data) return 0;
    model = mDoExt_J3DModel__create(data, 0x80000, 0x11000084);
    if (!model) return 0;
    collider = JKR_NEW dBgW();
    if (!collider) return 0;
    auto* dzb = static_cast<cBgD_t*>(dComIfG_getObjectRes(archive_name, dRes_INDEX_WROCK_DZB_WROCK_e));
    if (!dzb || collider->Set(dzb, 1, &collider_matrix)) return 0;
    collider->SetCrrFunc(dBgS_MoveBGProc_Typical);
    return 1;
}
int Rock::heap_callback(fopAc_ac_c* actor) { return static_cast<Rock*>(actor)->create_heap(); }
int Rock::destroy() {
    RockOps ops{*this};
    if (!lifecycle.release(ops)) { count(counters.cleanup_failures); return 0; }
    this->~Rock();
    return 1;
}
int Rock::execute() {
    if (!lifecycle.ready()) return 1;
    const int result = update_transform();
    count(counters.execute);
    return result;
}
int Rock::update_transform() {
    if (!model || !collider) return 1;
    acch.CrrPos(dComIfG_Bgsp());
    ground = acch.m_gnd;
    ground_height = acch.GetGroundH();
    if (ground_height != -G_CM3D_F_INF) {
        tevStr.YukaCol = dComIfG_Bgsp().GetPolyColor(ground);
        tevStr.room_no = dComIfG_Bgsp().GetRoomId(ground);
        fopAcM_SetRoomNo(this, tevStr.room_no);
    }
    mDoMtx_stack_c::transS(current.pos.x, current.pos.y, current.pos.z);
    mDoMtx_stack_c::ZXYrotM(shape_angle);
    mDoMtx_stack_c::scaleM(scale);
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    PSMTXCopy(model->getBaseTRMtx(), collider_matrix);
    collider->Move();
    eyePos = attention_info.position = current.pos;
    attention_info.flags = 0;
    return 1;
}
int Rock::draw() {
    if (!lifecycle.ready() || !model) return 1;
    g_env_light.settingTevStruct(0x20, &current.pos, &tevStr);
    g_env_light.setLightTevColorType_MAJI(model, &tevStr);
    dComIfGd_setListBG();
    mDoExt_modelUpdateDL(model);
    dComIfGd_setList();
    count(counters.draw);
    return 1;
}
static int rock_create_callback(void* actor) { return static_cast<Rock*>(actor)->create(); }
static int rock_delete_callback(void* actor) { return static_cast<Rock*>(actor)->destroy(); }
static int rock_execute_callback(void* actor) { return static_cast<Rock*>(actor)->execute(); }
static int rock_draw_callback(void* actor) { return static_cast<Rock*>(actor)->draw(); }
static int rock_is_delete_callback(void*) { return 1; }
const ActorProfileDesc Rock::profile = {
    .name = "dvfrock", .priority_group = 7, .process_size = sizeof(Rock),
    .draw_priority = fpcDwPi_OBJ_LBOX_e,
    .status = fopAcStts_UNK_0x40000_e | fopAcStts_UNK_0x4000_e | fopAcStts_CULL_e,
    .group = fopAc_ACTOR_e, .cull_type = fopAc_CULLBOX_CUSTOM_e,
    .create_function = rock_create_callback, .delete_function = rock_delete_callback, .execute_function = rock_execute_callback,
    .is_delete_function = rock_is_delete_callback, .draw_function = rock_draw_callback,
};
}
