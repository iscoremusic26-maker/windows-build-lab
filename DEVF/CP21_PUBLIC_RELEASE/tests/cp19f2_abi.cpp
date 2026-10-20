#include <d/actor/d_a_alink.h>
#include <d/actor/d_a_e_oc.h>
#include <m_Do/m_Do_ext.h>
#include <Z2AudioLib/Z2AudioMgr.h>
#include <m_Do/m_Do_audio.h>
#include <m_Do/m_Do_Reset.h>
#include <d/d_com_inf_game.h>
#include <JSystem/JAudio2/JASHeapCtrl.h>
#include <JSystem/JAudio2/JASWaveArcLoader.h>
#include <JSystem/JKernel/JKRAram.h>
#include <JSystem/JKernel/JKRAramHeap.h>
#include <JSystem/JKernel/JKRAramBlock.h>
#include <JSystem/JKernel/JKRHeap.h>
#include <cstddef>
#include <cstdio>
#include <type_traits>
static_assert(sizeof(void*)==8);
static_assert(sizeof(((daAlink_c*)nullptr)->mProcID)==2);
static_assert(daAlink_c::PROC_CUT_TURN==0x28&&daAlink_c::PROC_CUT_TURN_CHARGE==0x29&&daAlink_c::PROC_CUT_TURN_MOVE==0x2a&&daAlink_c::PROC_CUT_LARGE_JUMP_CHARGE==0x2f&&daAlink_c::PROC_CUT_LARGE_JUMP==0x30);
// CP18 keeps the CP17/CP17 FIX01 probe unchanged (Link view and Red) and adds the daE_OC_c fields the Black adapter reads/writes.
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
static_assert(std::is_same_v<decltype(((daE_OC_c*)nullptr)->field_0x6c0),s16>&&std::is_same_v<decltype(((daE_OC_c*)nullptr)->field_0x6e3),u8>&&std::is_same_v<decltype(((daE_OC_c*)nullptr)->mMoveRange),f32>&&std::is_same_v<decltype(((daE_OC_c*)nullptr)->mOcState),int>&&std::is_same_v<decltype(((fopAc_ac_c*)nullptr)->current.roomNo),s8>);   // CP18
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
// CP19: Link cut type (Helm Splitter mask), attention flags, E_OC invulnerability timer and sound object last-hit flag, Z2 scene/seq managers' fields.
static_assert(std::is_same_v<decltype(((daPy_py_c*)nullptr)->mCutType),u8>&&daPy_py_c::CUT_TYPE_HEAD_JUMP==5&&daPy_py_c::CUT_TYPE_TURN_RIGHT==8);
static_assert(std::is_same_v<decltype(((daE_OC_c*)nullptr)->field_0x6cc),s16>&&std::is_same_v<decltype(((daE_OC_c*)nullptr)->mSound),Z2CreatureEnemy>&&std::is_same_v<decltype(((Z2CreatureEnemy*)nullptr)->field_0xa3),u8>);
static_assert(std::is_same_v<decltype(((Z2SceneMgr*)nullptr)->requestSeWave_2),u8>&&std::is_same_v<decltype(((Z2SeqMgr*)nullptr)->mBattleDistState),u8>&&std::is_base_of_v<Z2SceneMgr,Z2AudioMgr>&&std::is_base_of_v<Z2SeqMgr,Z2AudioMgr>&&std::is_base_of_v<Z2SoundObjMgr,Z2AudioMgr>);
static_assert(fopAc_AttnFlag_BATTLE_e==4,"BATTLE flag value from the official header");
// CP19 FIX01: audio readiness data (exported engine data read by the gate).
static_assert(std::is_same_v<decltype(mDoAud_zelAudio_c::mInitFlag),u8>&&std::is_same_v<decltype(mDoAud_zelAudio_c::mResetFlag),u8>&&std::is_same_v<decltype(g_mDoAud_audioHeap),JKRSolidHeap*>&&std::is_same_v<decltype(mDoRst::mResetData),mDoRstData*>);
static_assert(std::is_base_of_v<Z2AudioMgr,mDoAud_zelAudio_c>&&std::is_same_v<decltype(((Z2AudioMgr*)nullptr)->mResettingFlag),bool>);
// CP19 FIX02: own ARAM block (graph heap tail block + JASHeap root), wave loader root heap, meter pending life count, Link render tint and model pointers.
static_assert(std::is_same_v<decltype(JASWaveArcLoader::sAramHeap),JASHeap*>&&std::is_same_v<decltype(JASKernel::getAramFreeSize()),u32>);
static_assert(std::is_same_v<decltype(&JKRAram::getAramHeap),JKRAramHeap*(*)()>&&std::is_same_v<decltype(((JKRAram*)nullptr)->mAramHeap),JKRAramHeap*>&&JKRAramHeap::TAIL==1);
static_assert(std::is_same_v<decltype(((JKRAramBlock*)nullptr)->mAddress),u32>&&std::is_same_v<decltype(((JKRAramBlock*)nullptr)->mSize),u32>&&std::is_same_v<decltype(((JASHeap*)nullptr)->mBase),u8*>&&std::is_same_v<decltype(((JASHeap*)nullptr)->mSize),u32>);
static_assert(std::is_same_v<decltype(&dComIfGp_setItemLifeCount),void(*)(f32,u8)>&&std::is_same_v<decltype(&dComIfGp_getItemLifeCountType),u8(*)()>&&std::is_same_v<decltype(&dComIfGp_getItemLifeCount),f32(*)()>);
static_assert(std::is_same_v<decltype(&daAlink_c::modelDraw),void(daAlink_c::*)(J3DModel*,int)>&&std::is_same_v<decltype(((dKy_tevstr_c*)nullptr)->TevKColor),GXColor>);
static_assert(std::is_same_v<decltype(((daAlink_c*)nullptr)->mpLinkModel),J3DModel*>&&std::is_same_v<decltype(((daAlink_c*)nullptr)->mpDemoHRTmpModel),J3DModel*>);
static_assert(std::is_same_v<decltype(((daAlink_c*)nullptr)->mTgCyls),dCcD_Cyl[3]>&&std::is_same_v<decltype(((daE_OC_c*)nullptr)->mSphs_at),dCcD_Sph[2]>);
// CP18: Black adapter fields (FIND timer, leash flag, range, state, room, home) read from the same compiled official headers.
__declspec(dllexport) extern const unsigned long long cp18_abi_layout[] = {
 0x4350313842414231ull,offsetof(daE_OC_c,field_0x6c0),offsetof(daE_OC_c,field_0x6c2),offsetof(daE_OC_c,field_0x6e3),offsetof(daE_OC_c,mMoveRange),offsetof(daE_OC_c,mOcState),offsetof(daE_OC_c,mActionMode),offsetof(fopAc_ac_c,current),offsetof(actor_place,roomNo),offsetof(fopAc_ac_c,home),sizeof(daE_OC_c)
};
__declspec(dllexport) extern const unsigned long long cp19_abi_layout[] = {
 0x4350313942554731ull,offsetof(daPy_py_c,mCutType),offsetof(fopAc_ac_c,attention_info),offsetof(daE_OC_c,field_0x6cc),offsetof(daE_OC_c,mSound),offsetof(Z2CreatureEnemy,field_0xa3),offsetof(Z2SceneMgr,requestSeWave_1),offsetof(Z2SceneMgr,loadedDemoWave),offsetof(Z2SeqMgr,mBattleDistState),offsetof(Z2SeqMgr,field_0xc4),sizeof(Z2AudioMgr),unsigned(fopAc_AttnFlag_BATTLE_e)
};
__declspec(dllexport) extern const unsigned long long cp19f2_abi_layout[] = {
 0x4350313946303232ull,offsetof(fopAc_ac_c,tevStr),offsetof(dKy_tevstr_c,TevKColor),offsetof(daAlink_c,mpLinkModel),offsetof(daAlink_c,mpLinkFaceModel),offsetof(daAlink_c,mpLinkHatModel),offsetof(daAlink_c,mpLinkHandModel),
 offsetof(daAlink_c,mpDemoFCBlendModel),offsetof(daAlink_c,mpDemoFCTongueModel),offsetof(daAlink_c,mpDemoHLTmpModel),offsetof(daAlink_c,mpDemoHRTmpModel),
 sizeof(JASHeap),offsetof(JASHeap,mBase),offsetof(JASHeap,mSize),offsetof(JKRAram,mAramHeap),offsetof(JKRAramHeap,mHeap),offsetof(JKRAramBlock,mAddress),offsetof(JKRAramBlock,mSize),unsigned(JKRAramHeap::TAIL)
};
__declspec(dllexport) extern const unsigned long long cp19f1_abi_layout[] = {
 0x4350313946303131ull,offsetof(mDoRstData,mShutdown),offsetof(mDoRstData,mReturnToMenu),sizeof(mDoRstData),offsetof(Z2AudioMgr,mResettingFlag),sizeof(mDoAud_zelAudio_c)
};
}
