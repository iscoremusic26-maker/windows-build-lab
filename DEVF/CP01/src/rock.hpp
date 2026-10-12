#pragma once
#include <mods/svc/actor.h>
#include <f_op/f_op_actor.h>
#include <SSystem/SComponent/c_phase.h>
#include <d/d_bg_s_acch.h>
#include <d/d_bg_w.h>
#include "lifecycle.hpp"

namespace devf::cp01 {
inline constexpr char actor_name[] = "dvfrock";
struct Counters { unsigned constructed = 0, ready = 0, destroyed = 0, execute = 0, draw = 0, cleanup_failures = 0; };
extern Counters counters;
// Adapted from exact 2.0.3 mods/custom_actor_demo/src/m_a_obj_wrock.{hpp,cpp}.
// No copied profile, vtable, retail offsets or enemy logic.
class Rock : public fopAc_ac_c {
public:
    request_of_phase_process_class phase{};
    J3DModel* model = nullptr;
    dBgS_ObjAcch acch;
    cBgS_GndChk ground;
    dBgS_AcchCir circle;
    Mtx collider_matrix{};
    dBgW* collider = nullptr;
    f32 ground_height = -G_CM3D_F_INF;
    ResourceLifecycle lifecycle;
    Rock();
    virtual ~Rock();
    int create();
    int create_heap();
    int execute();
    int update_transform();
    int draw();
    int destroy();
    static int heap_callback(fopAc_ac_c*);
    static const ActorProfileDesc profile;
};
}
