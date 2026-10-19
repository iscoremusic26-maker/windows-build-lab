#include <d/actor/d_a_alink.h>
#include <d/actor/d_a_e_oc.h>
#include <m_Do/m_Do_ext.h>
#include <cstddef>
#include <cstdio>
#include <type_traits>
static_assert(sizeof(void*)==8);
static_assert(sizeof(((daAlink_c*)nullptr)->mProcID)==2);
static_assert(daAlink_c::PROC_CUT_TURN==0x28&&daAlink_c::PROC_CUT_TURN_CHARGE==0x29&&daAlink_c::PROC_CUT_TURN_MOVE==0x2a&&daAlink_c::PROC_CUT_LARGE_JUMP_CHARGE==0x2f&&daAlink_c::PROC_CUT_LARGE_JUMP==0x30);
// CP17 FIX01: exactly the two interceptable families and their landings; the excluded families keep their own ids.
static_assert(daAlink_c::PROC_CUT_JUMP==0x26&&daAlink_c::PROC_CUT_JUMP_LAND==0x27&&daAlink_c::PROC_CUT_LARGE_JUMP_LAND==0x31&&daAlink_c::PROC_CUT_HEAD==0x2d&&daAlink_c::PROC_CUT_HEAD_LAND==0x2e);
static_assert(daAlink_c::PROC_DAMAGE==0x32&&daAlink_c::PROC_LARGE_DAMAGE_UP==0x33&&daAlink_c::PROC_LAND_DAMAGE==0x34&&daAlink_c::PROC_LARGE_DAMAGE==0x159&&daAlink_c::PROC_LARGE_DAMAGE_WALL==0x15a&&daAlink_c::PROC_GUARD_BREAK==0x1f);
using ConstCharge=BOOL(daPy_py_c::*)()const;
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutCharge),ConstCharge>);
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutTurnCharge),ConstCharge>);
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutLargeJumpCharge),ConstCharge>);
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutJumpMode),ConstCharge>);
static_assert(std::is_same_v<decltype(static_cast<BOOL(daAlink_c::*)()const>(&daAlink_c::checkCutTurnCharge)),BOOL(daAlink_c::*)()const>);
static_assert(std::is_same_v<decltype(((daAlink_c*)nullptr)->mTargetedActor),fopAc_ac_c*>);
static_assert(std::is_same_v<decltype(((daE_OC_c*)nullptr)->field_0x6c2),s16>&&std::is_same_v<decltype(((fopAc_ac_c*)nullptr)->speedF),f32>&&std::is_same_v<decltype(((fopAc_ac_c*)nullptr)->gravity),f32>);
// Never invoked here. Retained exports allow dumpbin to prove the Windows compiler-generated virtual dispatch and typed field load, with no guessed slot/offset.
extern "C" __declspec(dllexport) __declspec(noinline) int cp17_const_charge(const daPy_py_c*p){return p->checkCutCharge();}
extern "C" __declspec(dllexport) __declspec(noinline) int cp17_const_spin(const daPy_py_c*p){return p->checkCutTurnCharge();}
extern "C" __declspec(dllexport) __declspec(noinline) int cp17_const_jump(const daPy_py_c*p){return p->checkCutLargeJumpCharge();}
extern "C" __declspec(dllexport) __declspec(noinline) int cp17f1_const_cut_jump_mode(const daPy_py_c*p){return p->checkCutJumpMode();}
extern "C" __declspec(dllexport) __declspec(noinline) unsigned cp17_typed_proc(const daAlink_c*p){return p->mProcID;}
// Constant initializers are read from the compiler's COFF object, never from a fake player object or a game-dependent helper EXE.
extern "C" {
__declspec(dllexport) extern const unsigned long long cp17_abi_layout[] = {
 0x4350313741424931ull,sizeof(void*),sizeof(daAlink_c),sizeof(daPy_py_c),sizeof(daE_OC_c),sizeof(((daAlink_c*)nullptr)->mProcID),offsetof(daAlink_c,mProcID),offsetof(daAlink_c,mProcVar2),offsetof(daPy_py_c,mDamageTimer),offsetof(daE_OC_c,mActionMode),offsetof(daE_OC_c,mpMorf),offsetof(mDoExt_morf_c,mFrameCtrl),sizeof(ConstCharge),unsigned(daAlink_c::PROC_MAX)
};
__declspec(dllexport) extern const unsigned long long cp17f1_abi_layout[] = {
 0x4350313746314142ull,offsetof(daAlink_c,mTargetedActor),offsetof(daAlink_c,mLinkAcch),offsetof(fopAc_ac_c,speed),offsetof(fopAc_ac_c,speedF),offsetof(fopAc_ac_c,gravity),offsetof(fopAc_ac_c,shape_angle),offsetof(daE_OC_c,field_0x6c2),offsetof(daE_OC_c,mSphs_at),sizeof(dCcD_Sph),offsetof(daAlink_c,mTgCyls)
};
}
