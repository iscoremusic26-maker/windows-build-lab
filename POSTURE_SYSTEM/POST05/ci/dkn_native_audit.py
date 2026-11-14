"""LIGHT FADES TWICE 0.13.9 DKN01: native contract. Every fact of the official Dusklight 2.0.3 source (TwilitRealm/dusklight @40457c6, the pinned SDK
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
    if len(re.findall(r'field_0x700', tn)) != 4:
        e.append('field_0x700 has another reader / writer than setDamage (3) and action (1)')
    if tn.count('{dCcD_SE_METAL, 0, 9, 0, {0}}') != 3 or 'setSwordAtBreak(1);' not in body(tn, 'int daB_TN_c::create() {'):
        e.append('the native weapon / shield spl (9 at creation, 0xA after setSwordAtBreak(1)) changed')
    if 'BCK_TNA_ATACK_A,\n    /* 0x06 */ BCK_TNA_ATACK_B,' not in tn or '/* 0x14 */ BCK_TNB_ATACK_A,\n    /* 0x15 */ BCK_TNB_ATACK_B,\n    /* 0x16 */ BCK_TNB_ATACK_SHIELD,' not in tn \
            or '/* 0x1B */ BCK_TNB_DOWN_SIPPU,' not in tn or '/* 0x09 */ BCK_TNA_DAMAGE_L,' not in tn or '/* 0x2A */ BCK_TNB_WAIT,' not in tn:
        e.append('B_tn BCK ids changed')
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
    print(json.dumps({'status': 'DKN_NATIVE_PASS' if not errs else 'DKN_NATIVE_FAIL', 'facts': sum(len(x[2]) for x in FACTS) + 12, 'errors': errs}, indent=1))
    sys.exit(0 if not errs else 1)
