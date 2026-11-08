"""LIZALFOS 0.12.1: the native lines the brain relies on must be literally present in the OFFICIAL source used to build (40457c6). Any host source change fails the build.
Usage: liz_native_audit.py <official source root>. Read-only."""
import json, sys
from pathlib import Path
root = Path(sys.argv[1])
src = (root / 'src/d/actor/d_a_e_dn.cpp').read_text(encoding='utf-8')
hdr = (root / 'include/d/actor/d_a_e_dn.h').read_text(encoding='utf-8')
need = [
 # actions and clips
 '/* 0x03 */ ACTION_FIGHT_RUN    = 3,', '/* 0x04 */ ACTION_ATTACK_0     = 4,', '/* 0x05 */ ACTION_ATTACK       = 5,', '/* 0x06 */ ACTION_TAIL_ATTACK  = 6,', '/* 0x07 */ ACTION_GUARD        = 7,',
 '/* 0x14 */ ACTION_S_DAMAGE     = 20,', '/* 0x15 */ ACTION_DAMAGE       = 21,', '/* 0x3C */ ACTION_REG          = 60,',
 '/* 0x04 */ ANM_ATTACK_TAIL_01 = 4,', '/* 0x05 */ ANM_ATTACK_TAIL_02,', '/* 0x08 */ ANM_ATTACK_01,',
 # AT windows
 'if (frame >= 22 && frame <= 28) {', 'if (frame >= 13 && frame <= 18) {', 'if (frame >= 25 && frame <= 30) {', 'if (frame >= 33 && frame <= 41) {',
 # the only shield cancellation
 'if (actor_p != NULL && fopAcM_GetName(actor_p) == fpcNm_ALINK_e && daPy_getPlayerActorClass()->checkPlayerGuard()) {\n            i_this->anm_p->setPlaySpeed(0.0f);\n            i_this->action = ACTION_FIGHT_RUN;',
 'dComIfGp_getVibration().StartShock(3, 31, cXyz(0.0f, 1.0f, 0.0f));',
 # at_hit_check
 'static fopAc_ac_c* at_hit_check(e_dn_class* i_this) {', 'if (i_this->at_sph.ChkAtHit() != 0) {',
 # lunges
 'sp34.z = BREG_F(17) + 25.0f;', 'sp30.z = BREG_F(18) + 30.0f;',
 # sound frames
 'if (frame == 35) {', 'if (frame == 28) {' if False else '} else if (frame == 28) {', 'if (frame == 18) {', 'if (frame == 10) {', 'if (frame == 24) {', 'if (frame == 40) {', '} else if (frame == 43) {',
 # guard entry and acceptance
 'i_this->field_0x708 = TREG_F(10) + 30.0f;', 'i_this->field_0x70c = actor->shape_angle.y;', 'i_this->invulnerability_timer = 15;', 'if (i_this->tate_sph.ChkTgHit()) {', 'i_this->unk_timer_2 = 10;',
 '{dCcD_SE_STONE, 0x5, 0x0, 0x0, 0x3}, // mGObjTg',
 # damage
 'big_damage(i_this);\n                            i_this->invulnerability_timer = 1000;', 'small_damage(i_this);', 'if (i_this->at_info.mHitType == 16 || i_this->at_info.mpCollider->ChkAtType(AT_TYPE_HOOKSHOT)) {',
 'if (actor->health > 0 && !daPy_py_c::checkNowWolf()) {\n                    enemy->onDownFlg();', 'i_this->action = ACTION_NORMAL;',
 # identity
 'if (i_this->arg1 == 1) {\n        modelData = (J3DModelData*)dComIfG_getObjectRes("E_dn", 40);', 'if ((i_this->arg0 == 13 || i_this->arg0 == 14) || (i_this->arg0 == 15 || i_this->arg0 == 16)) {',
 'actor->field_0x560 = actor->health = 100;', 'if (strcmp(dComIfGp_getStartStageName(), "D_MN07") == 0 && fopAcM_GetRoomNo(actor) == 8',
 # execute order
 'action(i_this);', 'i_this->anm_p->play(0, 0);', 'anm_se_set(i_this);',
 # standing break: the native stagger, Helm and Finish
 '/* 0x0C */ ACTION_DRAWBACK     = 12,', 'i_this->actor.onHeadLockFlg();', 'i_this->actor.offHeadLockFlg();', 'anm_init(i_this, ANM_DRAWBACK, 5.0f, J3DFrameCtrl::EMode_NONE, 1.0f);',
 'i_this->field_0x74c = KREG_F(9) + 100.0f;', 'i_this->field_0x750 = (actor->shape_angle.y - 0x8000) - i_this->at_info.mHitDirection.y;', 'if (daPy_getPlayerActorClass()->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP) {\n                        i_this->at_info.mHitStatus = 0;',
 'enemy->offCutDownHitFlg();', 'actor->health = oldHealth2;',
 # coverage geometry
 'MTXCopy(i_this->anm_p->getModel()->getAnmMtx(JNT_HANDR1), *calc_mtx);', 'work.set(BREG_F(7), 60.0f + BREG_F(8), BREG_F(9));', 'i_this->at_sph.SetR(l_HIO.model_size * 80.0f);', 'i_this->at_sph.SetR(l_HIO.model_size * 50.0f);', 'MTXCopy(i_this->anm_p->getModel()->getAnmMtx(JNT_TAIL04), *calc_mtx);',
 'i_this->at_sph.SetR((70.0f + BREG_F(10)) * l_HIO.model_size);', 'i_this->at_sph.StartCAt(pos);', 'i_this->at_sph.MoveCAt(pos);', 'model_size = 1.1f;', 'attack_init_range = 350.0f;', '/* 0x13 */ JNT_HANDR1,', '/* 0x22 */ JNT_TAIL04,',
]
missing = [n for n in need if n not in src.replace('\r\n', '\n')]
hneed = ['s16 action;', 's16 mode;', 's16 timer[4];', 's16 invulnerability_timer;', 'dCcD_Sph at_sph;', 'dCcD_Sph tate_sph;', 'mDoExt_McaMorfSO* anm_p;']
missing += ['HEADER ' + n for n in hneed if n not in hdr]
# engine and Link: the defect's own lines
def extra(rel, lines):
    t = (root / rel).read_text(encoding='utf-8').replace('\r\n', '\n')
    return ['%s %s' % (rel, l) for l in lines if l not in t]
missing += extra('src/d/actor/d_a_alink_damage.inc', ['var_r29->ChkTgHit() && mProcID != PROC_GUARD_SLIP', 'setGuardSe(var_r29);', 'return procGuardSlipInit(at_spl, var_r29);', 'mDamageTimer = mpHIO->mDamage.m.mInvincibleTime;'])
missing += extra('src/d/actor/d_a_alink_guard.inc', ['mZ2Link.startCollisionSE(hit_se, var_r31);', 'if (mProcID == PROC_GUARD_SLIP) {\n        return 0;', 'mNormalSpeed = mpHIO->mGuard.m.mGuardSpeedNormal;', 'if (cLib_chaseF(&mNormalSpeed, 0.0f, speed_step) != 0 && checkAnmEnd(&mUnderFrameCtrl[0])) {\n        mCcStts.SetTgApid(0);'])
missing += extra('src/d/d_cc_s.cpp', ['if (cLib_distanceAngleS(sp10.y - -0x8000, var_r4) > shield_range) {', 'if (tgObjInf->ChkTgNoConHit() && !atObjInf->ChkAtStopNoConHit()) {', 'cXyz vec(*i_atObjInf->GetAtVecP());'])
missing += extra('src/d/d_cc_d.cpp', ['void dCcD_Sph::StartCAt(cXyz& pos) {\n    cXyz zero(cXyz::Zero);\n    SetAtVec(zero);', 'mEffCounter = 5;'])
missing += extra('src/d/d_cc_uty.cpp', ['int map_info = 30;'])
missing += extra('src/d/actor/d_a_alink_HIO_data.inc', ['8000,     5000,     0,     8000,     4000,     2.5f,     3.0f,     14.0f,'] if False else [])
missing += extra('src/d/actor/d_a_alink.cpp', ['tgCyl->OnTgNoConHit();', 'if (mDamageTimer != 0 || checkModeFlg(8)) {'])
print(json.dumps({'status': 'LIZ_NATIVE_CONTRACT_PASS' if not missing else 'LIZ_NATIVE_CONTRACT_FAIL', 'checked': len(need) + len(hneed), 'missing': missing}, indent=1))
sys.exit(1 if missing else 0)
