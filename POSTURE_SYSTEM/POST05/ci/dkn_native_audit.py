"""LIGHT FADES TWICE 0.13.9 DKN01 (+ 0.13.10 FIX01, 0.13.12 FIX03, 0.13.14 FIX05, 0.13.15 FIX06 facts): native contract. Every fact of the official Dusklight 2.0.3 source (TwilitRealm/dusklight @40457c6, the pinned SDK
checkout the gate builds against) that the Darknut miniboss depends on, pinned as exact text. If the host source differs, this fails before anything
is built. (The out-of-line calls and offsets in the 2.0.3 EXE are pinned separately: hook_prologue_audit.py and the static_asserts in mod.cpp.)
Usage: dkn_native_audit.py <dusklight source checkout>"""
from pathlib import Path
import json, re, sys


def body(t, sig):
    i = t.index(sig)
    j = t.index('{', i)
    d = 0
    for k in range(j, len(t)):
        d += t[k] == '{'
        d -= t[k] == '}'
        if d == 0:
            return t[i:k + 1]
    raise ValueError(sig)


FACTS = [
    # action(): damage_check before the 360 threshold write; field_0x700 read only by setDamage
    ('action', 'void daB_TN_c::action() {', ['    damage_check();\n    field_0x700 = l_HIO.field_0x24;']),
    ('setDamage', 'void daB_TN_c::setDamage(dCcD_Sph* i_sph, int param_2) {', [
        '    health = 100;\n    cc_at_check(this, &mAtInfo);',
        '        mNextBreakPart += param_2;\n        setActionMode(ACT_DAMAGEH, mNextActionMode2);',
        '            field_0x6fc += 100 - health;',
        '        if (field_0x6fc >= field_0x700) {\n            setActionMode(ACT_ENDING, ACTION2_0_e);',
        '            setActionMode(ACT_DAMAGEL, mNextActionMode2);']),
    ('setActionMode', 'void daB_TN_c::setActionMode(int i_mode1, int i_mode2) {', ['    offHeadLockFlg();\n    mActionMode1 = i_mode1;\n    mActionMode2 = i_mode2;']),
    ('setBreakPart', 'void daB_TN_c::setBreakPart(int i_idx) {', [
        '        6, 11, 8, 1, 10, 0, 9, 7, 4, 5, 12, 2, 3,', '    if (i_idx >= 12) {\n        i_idx = 13;\n    }', '    iVar1 = i_idx - iVar1;',
        '                if (mType == 1 && break_part_no[i] == 4) {\n                    mNextBreakPart++;\n                    iVar1++;', '                if (iVar1 == 0) {\n                    return;']),
    ('setBreakHeadPart', 'void daB_TN_c::setBreakHeadPart(int i_idx) {', ['        4, 5, 7, 6, 11, 8, 1, 10, 0, 9, 12, 2, 3,', '                if (break_part_no2[i] == 4) {\n                    mNextBreakPart++;']),
    ('executeDamageH', 'void daB_TN_c::executeDamageH() {', [
        '    if (mTimer5 == 0) {\n        field_0xa91 = true;\n        mTimer10 = 30;\n    }', '        if (part_idx >= 12) {\n            setActionMode(ACT_CHANGEDEMO, ACTION2_0_e);',
        '                setBck(BCK_TNA_DAMAGE_L, 0, 0.0f, 1.0f);', '        if (mpModelMorf2->isStop()) {\n            setActionMode(ACT_CHASEH, ACTION2_0_e);']),
    ('executeZakoChangeDemo', 'void daB_TN_c::executeZakoChangeDemo() {', [
        '        setBck(BCK_TNA_DAMAGE_LAST, 0, 3.0f, 1.0f);', '        setBreakPart(mNextBreakPart);', '            setActionMode(ACT_CHASEL, ACTION2_0_e);\n            initChaseL(3);']),
    ('executeYoroke', 'void daB_TN_c::executeYoroke() {', [
        '            setBck(BCK_TNB_DOWN_SIPPU, 0, 3.0f, 1.0f);', '        } else if (mCutJumpStatus == 2 && player->checkCutJumpMode() &&',
        '        if (mpModelMorf2->isStop()) {\n            setActionMode(ACT_CHASEL, ACTION2_0_e);\n            field_0xa91 = true;']),
    ('damage_check', 'void daB_TN_c::damage_check() {', [
        '    if (mActionMode1 == ACT_CHANGEDEMO) {\n        return;\n    }', '    if (mActionMode1 == ACT_ENDING) {\n        return;\n    }',
        '                        setActionMode(ACT_YOROKE, ACTION2_0_e);', '        if (mSwordSphs[i].ChkAtHit()) {\n            field_0xa9a = true;\n            if (mSwordSphs[i].ChkAtShieldHit() && mSwordSphs[i].GetAtSpl() == 10) {',
        '                if (mTimer10 != 0 ||']),
    ('setSwordAtBreak', 'void daB_TN_c::setSwordAtBreak(int param_1) {', ['            mSwordSphs[i].SetAtSpl((dCcG_At_Spl)0x1);', '            mSwordSphs[i].SetAtSpl((dCcG_At_Spl)0xA);']),
    ('executeAttackH', 'void daB_TN_c::executeAttackH() {', [
        '            setBck(BCK_TNA_ATACK_A, 0, 3.0f, 1.0f);', '        if (mpModelMorf2->checkFrame(18.0f)) {\n            setSwordAtBit(1);',
        '        if (mpModelMorf2->checkFrame(29.0f)) {\n            setSwordAtBit(0);', '        if (mpModelMorf2->checkFrame(22.0f)) {\n            setSwordAtBit(1);',
        '        if (mpModelMorf2->checkFrame(30.0f)) {\n            setSwordAtBit(0);', '        if (mpModelMorf2->checkFrame(26.0f)) {\n            mSound.startCreatureSound(Z2SE_EN_TN_SWD_IMPCT, 0, -1);']),
    ('executeAttackShieldH', 'void daB_TN_c::executeAttackShieldH() {', [
        '        if (mpModelMorf2->checkFrame(5.0f)) {\n            mSphC.OnAtSetBit();', '        if (mpModelMorf2->checkFrame(9.0f)) {\n            mSphC.OffAtSetBit();',
        '        if (mpModelMorf2->isStop()) {\n            setActionMode(ACT_ATTACKH, ACTION2_0_e);']),
    ('executeAttackL', 'void daB_TN_c::executeAttackL() {', [
        '        if (mpModelMorf2->checkFrame(7.0f)) {\n            setSwordAtBit(1);\n        } else if (mpModelMorf2->checkFrame(14.0f)) {\n            setSwordAtBit(0);',
        '        if (mpModelMorf2->checkFrame(13.0f)) {\n            setSwordAtBit(1);\n            setSwordAtBreak(0);\n        } else if (mpModelMorf2->checkFrame(38.0f)) {\n            setSwordAtBit(0);',
        '        if (mActionMode2Copy != 0) {\n            if (mpModelMorf2->checkFrame(40.0f)) {', '            if (mpModelMorf2->checkFrame(41.0f) && mWalkDir == 10) {',
        '        } else if (mpModelMorf2->checkFrame(26.0f)) {\n            mSound.startCreatureSound(Z2SE_EN_TN_SWD_S, 0, -1);', '    speedF = 0.0f;\n    current.angle.y = shape_angle.y;']),
    ('executeAttackShieldL', 'void daB_TN_c::executeAttackShieldL() {', [
        '        if (mpModelMorf2->checkFrame(8.0f)) {\n            mSphC.OnAtSetBit();', '        if (mpModelMorf2->checkFrame(12.0f)) {\n            mSphC.OffAtSetBit();',
        '            if (checkAttackAble() && checkNormalAttackAble()) {\n                setActionMode(ACT_ATTACKL, ACTION2_20_e);']),
    ('initChaseL', 'void daB_TN_c::initChaseL(int param_1) {', ['    case ACTION2_2_e:\n        setBck(BCK_TNB_WAIT, 2, 10.0f, 1.0f);\n        mTimer1 = 30;']),
    ('cc_set', 'void daB_TN_c::cc_set() {', ['    if (mStates[12] == 0) {\n        mDoMtx_stack_c::copy(model->getAnmMtx(9));', '    } else if (mSphC.ChkAtSet() && mActionMode1 > 8) {\n        mDoMtx_stack_c::copy(model->getAnmMtx(24));']),
    # ---- 0.13.10 FIX01: the native offensive authority the controller replaces, the Shield Attack guard, the parry / Helm Splitter / getup facts
    ('checkNormalAttackAble', 'bool daB_TN_c::checkNormalAttackAble() {', ['        if (daPy_getPlayerActorClass()->getCutCount() != 0) {\n            mTimer3 = cM_rndF(60.0f) + 30.0f;\n            return 0;']),
    ('checkNextMove', 'bool daB_TN_c::checkNextMove() {', [
        '        if (player->getCutType() != daPy_py_c::CUT_TYPE_JUMP && mPlayerDistance < 200.0f) {\n            setActionMode(ACT_ATTACKSHIELDL, ACTION2_0_e);',
        '        if (mTimer3 == 0 && checkNormalAttackAble()) {\n            setActionMode(ACT_ATTACKL, ACTION2_0_e);']),
    ('damage_check FIX01', 'void daB_TN_c::damage_check() {', [
        '            if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {\n                field_0xaa8 = true;\n                def_se_set(&mSound, dStack_160.GetTgHitObj(), 42, this);\n                setActionMode(ACT_GUARDL, ACTION2_0_e);',
        '                        setActionMode(ACT_GUARDL, ACTION2_11_e);', '        setDamage(&dStack_160, 1);\n        return;']),
    ('executeGuardL', 'void daB_TN_c::executeGuardL() {', [
        '                setBck(BCK_TNB_GUARD_A, 0, 3.0f, 1.0f);', '            setBck(BCK_TNB_DOWN, 0, 3.0f, 1.0f);', '            setActionMode(ACT_ATTACKL, ACTION2_11_e);']),
    ('executeGuardH', 'void daB_TN_c::executeGuardH() {', ['        if (field_0xaa8) {\n            onHeadLockFlg();']),
    ('setBodyShield', 'void daB_TN_c::setBodyShield() {', [
        '        if (player->getCutType() != daPy_py_c::CUT_TYPE_HEAD_JUMP &&\n            ((field_0xa91 == true && mCutFlag) || mTimer10 != 0 || daPy_py_c::checkNowWolf()))']),
    ('action FIX01', 'void daB_TN_c::action() {', ['    setHeadLockPos(&sp38);', '        executeZakoEnding();']),
    ('executeZakoEnding', 'void daB_TN_c::executeZakoEnding() {', ['        setBck(4, 0, 3.0f, 1.0f);', '            fopAcM_delete(this);']),
    ('executeAttackShieldH chain', 'void daB_TN_c::executeAttackShieldH() {', ['            setActionMode(ACT_ATTACKH, ACTION2_0_e);']),
    # FIX03 0.13.12: the phase-1 shield (TgShield on mSphC while it exists; mSphA with a91 + front or mTimer10; none at mNextBreakPart >= 11)
    ('setBodyShield phase 1', 'void daB_TN_c::setBodyShield() {', [
        '        if (!field_0xa91 || abs((s16)(fopAcM_searchPlayerAngleY(this) - shape_angle.y)) > 0x3000) {\n            check = false;\n        }',
        '        if (mNextBreakPart >= 11) {\n            check = false;\n            mTimer10 = 0;\n        }',
        '        if (!(check || daPy_py_c::checkNowWolf() || mTimer10 != 0)) {']),
    # FIX03: executeGuardH mode 0 never reads the collider (the phase-1 module guard enters at mode 1 with the same clip anyway)
    ('executeGuardH FIX03', 'void daB_TN_c::executeGuardH() {', ['            setBck(BCK_TNA_GUARD, 0, 0.0f, 1.0f);',
        '        if (mpModelMorf2->isStop() &&\n            daPy_getPlayerActorClass()->getCutType() != daPy_py_c::CUT_TYPE_HEAD_JUMP)\n        {\n            setActionMode(ACT_CHASEH, ACTION2_0_e);']),
    # FIX03: the passivity / counter kick: native AT by checkFrame 8 / 12 (crossing-safe at rate 1/1.12)
    ('executeAttackShieldL FIX03', 'void daB_TN_c::executeAttackShieldL() {', ['        if (mpModelMorf2->checkFrame(8.0f)) {\n            mSphC.OnAtSetBit();',
        '        if (mpModelMorf2->checkFrame(12.0f)) {\n            mSphC.OffAtSetBit();']),
    # FIX03: TNB_DAMAGE_R is the native phase-2 damage reaction (the second stagger clip)
    ('executeDamageL FIX03', 'void daB_TN_c::executeDamageL() {', ['            setBck(BCK_TNB_DAMAGE_R, 0, 3.0f, 1.0f);']),
]
FIX05_FACTS = [
    # Closure: native whole-weapon OFF also clears the four physical sphere contacts.
    ('setSwordAtBit', 'void daB_TN_c::setSwordAtBit(int param_1) {', [
        '            mSwordSphs[i].OffAtSetBit();\n            mSwordSphs[i].ClrAtHit();',
        '        mCps.OffAtSetBit();\n        mCps.ClrAtHit();',
        '            mSwordSphs[i].OnAtSetBit();', '        mCps.OnAtSetBit();']),
    # the sword draw of the transformation: the weapon AT on at the throw (frame 23), the sword in hand at the pull (frame 10), ChaseL 3 after it (no AT off)
    ('executeZakoChangeDemo', 'void daB_TN_c::executeZakoChangeDemo() {', [
        '        if (mpModelMorf2->checkFrame(23.0f)) {\n            mStates[13] = 2;\n            setSwordAtBit(1);\n        }',
        '        if (mpModelMorf2->checkFrame(10.0f)) {\n            mSound.startCreatureSound(Z2SE_EN_TN_PULL_OUT, 0, -1);\n            mStates[15] = 1;',
        '            setActionMode(ACT_CHASEL, ACTION2_0_e);\n            initChaseL(3);\n            mSphC.OffAtSetBit();']),
    # cc_set: the capsule closes from the PREVIOUS anchor mCpsPos; precedence mace in hand > sword in hand > flying mace; the anchor only moves with a collider
    ('cc_set', 'void daB_TN_c::cc_set() {', [
        '    if (mStates[13] == 1) {', '    } else if (mStates[15] == 1) {', '    } else if (mStates[13] == 3) {',
        '        mCps.cM3dGCps::Set(mCpsPos, sp38, 45.0f);\n        mCpsPos = sp38;\n        dComIfG_Ccsp()->Set(&mCps);']),
    # the thrown mace (mType 1) flies at Link and vanishes on the ground / a wall
    ('calcSwordMoveA', 'void daB_TN_c::calcSwordMoveA() {', ['            cXyz sp20 = daPy_getPlayerActorClass()->current.pos;', '            if (mType == 1) {\n                field_0x99c[idx].y = 0x1800;\n                fopAcM_createDisappear(this, &mPositions[idx], 3, 0, 0xff);\n                mStates[idx] = 7;']),
    # execute order: action() (our POST) then mtx_set() then cc_set()
    ('execute', 'int daB_TN_c::execute() {', ['    action();\n    mtx_set();', '    cc_set();']),
    # the native guard knockback of Link's 4th+ cut (mCutFlag): TNB_DOWN, speedF 17 decelerated 1 per tick, backwards; mode 3 attacks at frame 15
    ('executeGuardL', 'void daB_TN_c::executeGuardL() {', ['            setBck(BCK_TNB_DOWN, 0, 3.0f, 1.0f);\n            speedF = 17.0f;', '        current.angle.y = shape_angle.y + 0x8000;',
                                                            '            setActionMode(ACT_ATTACKL, ACTION2_11_e);']),
    ('setBodyShield', 'void daB_TN_c::setBodyShield() {', ['            player->getCutCount() >= 4)\n        {\n            mCutFlag = true;']),
]
# DKN01 FIX06 0.13.15: what the sequence, phase-1 executions, the sway, the reposition jump and the sword darkening rely on
FIX06_FACTS = [
    ('src/d/actor/d_a_b_tn.cpp', 'int daB_TN_c::draw() {', [
        '    if (mMaterialColor) {\n        for (u16 i = 0; i < modelData->getMaterialNum(); i++) {', '            mColor = mMatNodePtr->getTevColor(0);\n            mColor->r = mMaterialColor;',
        '    mpModelMorf2->entryDL();', '            if (i == 15) {\n                model = mpModelMorf1->getModel();\n                g_env_light.setLightTevColorType_MAJI(model, &tevStr);\n                mpModelMorf1->entryDL();',
        '    if (mDisappear || mInactive) {\n        return 1;', '        if (mStates[i] != 7) {']),
    ('src/d/actor/d_a_b_tn.cpp', 'int daB_TN_c::CreateHeap() {', [
        "                                        0, -1, &mSound, 0x80000, 0x11000084);\n    if (mpModelMorf2 == NULL", "                                        0, -1, &mSound, 0x80000, 0x11000084);\n    if (mpModelMorf1 == NULL"]),
    ('src/d/actor/d_a_b_tn.cpp', 'void daB_TN_c::executeZakoEnding() {', ['        cLib_chaseF(&mMaterialColor, -50.0f, 0.3f);']),
    ('src/m_Do/m_Do_ext.cpp', 'void mDoExt_modelEntryDL(J3DModel* i_model) {', ['        i_model->calcMaterial();\n        i_model->diff();\n        return;', '        mDoExt_modelDiff(i_model);']),
    ('src/d/actor/d_a_b_tn.cpp', 'void daB_TN_c::initChaseL(int param_1) {', [
        '    mActionMode2 = param_1;\n    speedF = 0.0f;', '    case ACTION2_6_e:\n        setBck(BCK_TNB_JUMP_B_1, 0, 3.0f, 1.0f);\n        break;',
        '    case ACTION2_8_e:\n    case ACTION2_17_e:\n        setBck(BCK_TNB_JUMP_F_1, 0, 3.0f, 1.0f);\n        break;',
        '    case ACTION2_9_e:\n        setBck(BCK_TNB_JUMP_F_2, 0, 5.0f, 1.0f);\n        mSound.startCreatureSound(Z2SE_EN_TN_FOOT_M1, 0, -1);\n        break;']),
    ('src/d/actor/d_a_b_tn.cpp', 'void daB_TN_c::executeChaseL() {', [
        '            if (mpModelMorf2->checkFrame(4.0f)) {\n                speedF = 50.0f;\n                speed.y = 19.0f;\n                gravity = -4.0f;',
        '            if (mpModelMorf2->isStop()) {\n                setBck(BCK_TNB_JUMP_B_2, 0, 5.0f, 0.0f);\n            }\n        } else if (mAcch.ChkGroundHit()) {\n            initChaseL(7);',
        '            } else if (mPlayerDistance < 200.0f) {\n                setActionMode(ACT_ATTACKSHIELDL, ACTION2_0_e);\n            } else {\n                setActionMode(ACT_ATTACKL, ACTION2_10_e);',
        '            if (mActionMode2 == ACTION2_8_e) {\n                speedF = 35.0f;\n                speed.y = 4.0f;',
        '        if (mpModelMorf2->isStop() && mAcch.ChkGroundHit()) {\n            if (!checkAttackAble() || !checkNormalAttackAble()) {\n                initChaseL(9);',
        '        cLib_chaseF(&speedF, 0.0f, 7.0f);']),
    ('src/d/actor/d_a_b_tn.cpp', 'void daB_TN_c::executeDamageH() {', [
        '                setBck(BCK_TNA_DAMAGE_R, 0, 0.0f, 1.0f);', '            mActionMode2 = ACTION2_2_e;']),
    ('src/d/actor/d_a_b_tn.cpp', 'void daB_TN_c::damage_check() {', [
        '        cut_type != daPy_py_c::CUT_TYPE_HEAD_JUMP &&\n        cut_type != daPy_py_c::CUT_TYPE_MORTAL_DRAW_B &&\n        cut_type != daPy_py_c::CUT_TYPE_MORTAL_DRAW_A)']),
    ('src/d/actor/d_a_b_tn.cpp', 'void daB_TN_c::action() {', ['    cXyz sp38(eyePos);\n    sp38.y += 300.0f;\n    setHeadLockPos(&sp38);']),
]
LGC_FACTS = [
    # LGC01: the ordinary cuts, their AT / cancel frames, the cut's own end (resetCombo(TRUE) + checkNextAction(0)), the AT window of the procedure
    ('src/d/actor/d_a_alink_cut.inc', 'int daAlink_c::procCutNormalInit(int i_type) {', ['    field_0x3478 = cutData->mAttackStartFrame;\n    field_0x347c = cutData->mAttackEndFrame;\n    field_0x3480 = cutData->mSpeed;',
                                                                                     '    field_0x3484 = cutData->mCutAnm.mCancelFrame;', '            CUT_TYPE_COMBO_STAB,', '            CUT_TYPE_NM_STAB,']),
    ('src/d/actor/d_a_alink_cut.inc', 'int daAlink_c::procCutNormal() {', ['    if (checkAnmEnd(frameCtrl)) {\n        resetCombo(TRUE);', '            checkNextAction(0);',
                                                                         '        if (frameCtrl->getFrame() >= field_0x3478 && frameCtrl->getFrame() < field_0x347c) {', '            onResetFlg0(RFLG0_UNK_2);',
                                                                         '               !checkCutCancelNextMode(3))']),
    ('src/d/actor/d_a_alink.cpp', 'int daAlink_c::checkNextAction(int param_0) {', ['    if (param_0 != 0 && !checkInputOnR() && !checkEquipAnime()) {\n        mMaxSpeed = prev_maxSpeed;\n        return 0;\n    }']),
    ('src/d/actor/d_a_alink_guard.inc', 'void daAlink_c::setShieldGuard() {', ['         !checkRideOn() && checkGuardActionChange()))']),
    ('src/d/actor/d_a_alink_guard.inc', 'BOOL daAlink_c::checkGuardAccept() {', ['    return checkModeFlg(0x80) &&']),
    ('src/d/actor/d_a_alink_HIO_data.inc', 'DUSK_GAME_DATA const daAlinkHIO_cutNormal_c1 daAlinkHIO_cutNmV_c0::m = {', ['        2.0f,\n        2.0f,\n        16.0f,\n\t},\n    5.0f,\n    5.0f,\n    10.0f,']),
]
LINK_FACTS = [
    ('src/d/actor/d_a_alink_cut.inc', 'BOOL daAlink_c::checkCutReverseAt(dCcD_GObjInf* i_gobj) {', ['    if (checkAtShieldHit(*i_gobj)) {\n        return TRUE;']),
    ('src/d/actor/d_a_alink_cut.inc', 'BOOL daAlink_c::changeCutReverseProc(daAlink_c::daAlink_ANM i_anmID) {', ['        return procCutReverseInit(i_anmID);']),
    ('src/d/actor/d_a_alink_cut.inc', 'BOOL daAlink_c::checkCutHeadState() {', ['            && enemy->checkHeadLockFlg();']),
    ('src/d/actor/d_a_alink_cut.inc', 'int daAlink_c::procCutHeadInit() {', ['        sp2C = enemy_p->getHeadLockPos() - current.pos;', '    setCutType(CUT_TYPE_HEAD_JUMP);']),
    ('libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp', 'void J3DFrameCtrl::update() {', [
        '    case EMode_NONE:\n        if (mFrame < mStart) {\n            mFrame = mStart;\n            mRate = 0.0f;\n            mState |= (u8)1;']),
]


def audit(src):
    s = Path(src)
    e = []
    tn = (s / 'src/d/actor/d_a_b_tn.cpp').read_text(encoding='utf-8')
    for name, sig, needs in FACTS:
        try:
            b = body(tn, sig)
        except ValueError:
            e.append('missing ' + name)
            continue
        for n in needs:
            if n not in b:
                e.append('%s changed: %s' % (name, n.strip()[:70]))
    tn = (s / 'src/d/actor/d_a_b_tn.cpp').read_text(encoding='utf-8')
    for name, sig, needs in FIX05_FACTS:
        try:
            b = body(tn, sig)
        except ValueError:
            e.append('FIX05 native function missing: ' + name)
            continue
        for n in needs:
            if n not in b:
                e.append('FIX05 native fact changed in %s: %s' % (name, n.strip()[:70]))
    for rel, sig, needs in LGC_FACTS:
        tx = (s / rel).read_text(encoding='utf-8')
        try:
            b = body(tx, sig) if not sig.startswith('DUSK_GAME_DATA') else tx[tx.index(sig):tx.index('};', tx.index(sig)) + 2]
        except ValueError:
            e.append('LGC01 native function missing: ' + sig[:60])
            continue
        for n in needs:
            if n not in b:
                e.append('LGC01 native fact changed in %s: %s' % (sig[:50], n.strip()[:70]))
    for rel, sig, needs in FIX06_FACTS:
        try:
            b = body((s / rel).read_text(encoding='utf-8'), sig)
        except (ValueError, OSError):
            e.append('FIX06 native function missing: ' + sig[:60])
            continue
        for n in needs:
            if n not in b:
                e.append('FIX06 native fact changed in %s: %s' % (sig[:50], n.strip()[:70]))
    for n in ('/* 0x0B */ BCK_TNA_DAMAGE_R,', '/* 0x1E */ BCK_TNB_JUMP_B_1,', '/* 0x1F */ BCK_TNB_JUMP_B_2,', '/* 0x20 */ BCK_TNB_JUMP_F_1,', '/* 0x21 */ BCK_TNB_JUMP_F_2,'):
        if n not in (s / 'src/d/actor/d_a_b_tn.cpp').read_text(encoding='utf-8'):
            e.append('B_tn BCK ids (FIX06: TNA_DAMAGE_R 0x0B, TNB_JUMP_B/F 0x1E-0x21) changed: ' + n)
    for rel, sig, needs in LINK_FACTS:
        try:
            b = body((s / rel).read_text(encoding='utf-8'), sig)
        except (ValueError, OSError):
            e.append('missing ' + sig)
            continue
        for n in needs:
            if n not in b:
                e.append('%s changed: %s' % (sig[:40], n.strip()[:70]))
    if '/* 0x04 */ BCK_TN2B_DIE = 4,' not in tn or '/* 0x1A */ BCK_TNB_DOWN,' not in tn or '/* 0x1C */ BCK_TNB_GUARD_A,' not in tn:
        e.append('B_tn BCK ids (FIX01: TN2B_DIE, TNB_DOWN, TNB_GUARD_A) changed')
    if len(re.findall(r'field_0x700', tn)) != 4:
        e.append('field_0x700 has another reader / writer than setDamage (3) and action (1)')
    if tn.count('{dCcD_SE_METAL, 0, 9, 0, {0}}') != 3 or 'setSwordAtBreak(1);' not in body(tn, 'int daB_TN_c::create() {'):
        e.append('the native weapon / shield spl (9 at creation, 0xA after setSwordAtBreak(1)) changed')
    if 'BCK_TNA_ATACK_A,\n    /* 0x06 */ BCK_TNA_ATACK_B,' not in tn or '/* 0x14 */ BCK_TNB_ATACK_A,\n    /* 0x15 */ BCK_TNB_ATACK_B,\n    /* 0x16 */ BCK_TNB_ATACK_SHIELD,' not in tn \
            or '/* 0x1B */ BCK_TNB_DOWN_SIPPU,' not in tn or '/* 0x09 */ BCK_TNA_DAMAGE_L,' not in tn or '/* 0x2A */ BCK_TNB_WAIT,' not in tn:
        e.append('B_tn BCK ids changed')
    if '/* 0x18 */ BCK_TNB_DAMAGE_R,' not in tn or '/* 0x0C */ BCK_TNA_GUARD,' not in tn:
        e.append('B_tn BCK ids (FIX03: TNB_DAMAGE_R 0x18, TNA_GUARD 0x0C) changed')
    h = (s / 'include/d/actor/d_a_b_tn.h').read_text(encoding='utf-8')
    for n in ('/* 0x7 */ ACT_DAMAGEH,', '/* 0x8 */ ACT_CHANGEDEMO,', '/* 0xA */ ACT_ATTACKL,', '/* 0xF */ ACT_YOROKE,', '    void setBreakPart(int);', '    void setBck(int, u8, f32, f32);'):
        if n not in h:
            e.append('d_a_b_tn.h changed: ' + n)
    dm = (s / 'src/d/actor/d_a_alink_damage.inc').read_text(encoding='utf-8')
    if 'if (at_spl == 10 || at_spl == 11 || at_spl == 9) {\n                    return procGuardBreakInit();' not in dm:
        e.append('Link guard-break spl rule changed (9/10/11)')
    if 'return i_atSpl == 1 || i_atSpl == 6 || i_atSpl == 10 || i_atSpl == 13;' not in body(dm, 'bool daAlink_c::checkLargeAttack(int i_atSpl) const {'):
        e.append('spl 1 is no longer a large (non-breaking) attack')
    gd = (s / 'src/d/actor/d_a_alink_guard.inc').read_text(encoding='utf-8')
    if 'checkUpperGuardAnime();' not in body(gd, 'BOOL daAlink_c::checkPlayerGuard() const {'):
        e.append('checkPlayerGuard (the REAL guard read by the passivity detector) changed')
    al = (s / 'include/d/actor/d_a_alink.h').read_text(encoding='utf-8')
    for n in ('/* 0x01D */ PROC_GUARD_SLIP,', '/* 0x01E */ PROC_GUARD_ATTACK,', '/* 0x01F */ PROC_GUARD_BREAK,', '/* 0x01C */ PROC_CROUCH,', '/* 0x034 */ PROC_LAND_DAMAGE,'):
        if n not in al:
            e.append('Link procedure enum changed: ' + n)
    return e


if __name__ == '__main__':
    errs = audit(sys.argv[1])
    print(json.dumps({'status': 'DKN_NATIVE_PASS' if not errs else 'DKN_NATIVE_FAIL', 'facts': sum(len(x[2]) for x in FACTS) + sum(len(x[2]) for x in LINK_FACTS) + 13 + sum(len(x[2]) for x in FIX05_FACTS) + sum(len(x[2]) for x in LGC_FACTS) + sum(len(x[2]) for x in FIX06_FACTS) + 5, 'errors': errs}, indent=1))
    sys.exit(0 if not errs else 1)
