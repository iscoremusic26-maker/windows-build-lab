#include <d/actor/d_a_alink.h>
#include <d/actor/d_a_e_oc.h>
#include <m_Do/m_Do_ext.h>
#include <cstddef>
#include <cstdio>
#include <type_traits>
static_assert(sizeof(void*)==8);
static_assert(sizeof(((daAlink_c*)nullptr)->mProcID)==2);
static_assert(daAlink_c::PROC_CUT_TURN==0x28&&daAlink_c::PROC_CUT_TURN_CHARGE==0x29&&daAlink_c::PROC_CUT_TURN_MOVE==0x2a&&daAlink_c::PROC_CUT_LARGE_JUMP_CHARGE==0x2f&&daAlink_c::PROC_CUT_LARGE_JUMP==0x30);
using ConstCharge=BOOL(daPy_py_c::*)()const;
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutCharge),ConstCharge>);
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutTurnCharge),ConstCharge>);
static_assert(std::is_same_v<decltype(&daPy_py_c::checkCutLargeJumpCharge),ConstCharge>);
static_assert(std::is_same_v<decltype(static_cast<BOOL(daAlink_c::*)()const>(&daAlink_c::checkCutTurnCharge)),BOOL(daAlink_c::*)()const>);
// Never invoked here. Retained exports allow dumpbin to prove the Windows compiler-generated virtual dispatch and typed field load, with no guessed slot/offset.
extern "C" __declspec(dllexport) __declspec(noinline) int cp17_const_charge(const daPy_py_c*p){return p->checkCutCharge();}
extern "C" __declspec(dllexport) __declspec(noinline) int cp17_const_spin(const daPy_py_c*p){return p->checkCutTurnCharge();}
extern "C" __declspec(dllexport) __declspec(noinline) int cp17_const_jump(const daPy_py_c*p){return p->checkCutLargeJumpCharge();}
extern "C" __declspec(dllexport) __declspec(noinline) unsigned cp17_typed_proc(const daAlink_c*p){return p->mProcID;}
// Constant initializers are read from the compiler's COFF object, never from a fake player object or a game-dependent helper EXE.
extern "C" {
__declspec(dllexport) extern const unsigned long long cp17_abi_layout[] = {
 0x4350313741424931ull,sizeof(void*),sizeof(daAlink_c),sizeof(daPy_py_c),sizeof(daE_OC_c),sizeof(((daAlink_c*)nullptr)->mProcID),offsetof(daAlink_c,mProcID),offsetof(daAlink_c,mProcVar2),offsetof(daPy_py_c,mDamageTimer),offsetof(daE_OC_c,mActionMode),offsetof(daE_OC_c,mpMorf),offsetof(mDoExt_morf_c,mFrameCtrl),sizeof(ConstCharge),unsigned(daAlink_c::PROC_MAX)
};
}
