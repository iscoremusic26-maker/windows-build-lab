"""LIGHT FADES TWICE 0.13.8 DKR01: native contract. Every fact of the official Dusklight 2.0.3 source (TwilitRealm/dusklight @40457c6, the pinned SDK
checkout the gate builds against) that the Darknut resurrection depends on, pinned as exact text. If the host source ever differs, this fails before
anything is built. Usage: dkr_native_audit.py <dusklight source checkout>"""
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


def audit(src):
    s = Path(src)
    e = []
    tn = (s / 'src/d/actor/d_a_b_tn.cpp').read_text(encoding='utf-8')
    zako = body(tn, 'void daB_TN_c::executeZakoEnding() {')
    for need in ('    case ACTION2_0_e:\n        attention_info.flags = 0;\n        setBck(4, 0, 3.0f, 1.0f);\n        mSound.startCreatureVoice(Z2SE_EN_TN_V_DEAD2, -1);\n        mActionMode2 = ACTION2_1_e;\n        speedF = 0.0f;\n        mUpdateNeckAngle = false;',
                 '        if (mpModelMorf2->checkFrame(3.0f)) {\n            mStates[15] = 2;',
                 '        cLib_chaseF(&mMaterialColor, -50.0f, 0.3f);\n\n        if (mpModelMorf2->isStop()) {\n            mActionMode2 = ACTION2_2_e;\n            mTimer1 = 10;\n        }',
                 '            fopAcM_createDisappear(this, &sp20, 10, 0, 29);', '                fopAcM_delete(this);'):
        if need not in zako:
            e.append('executeZakoEnding changed: ' + need.strip()[:60])
    if 'default:' in zako or len(re.findall(r'\n    case ACTION2_\d+_e:', zako)) != 4:
        e.append('executeZakoEnding has a default case or not exactly modes 0..3 (the held mode must be a no-op)')
    dc = body(tn, 'void daB_TN_c::damage_check() {')
    if '    if (mActionMode1 == ACT_ENDING) {\n        return;\n    }' not in dc or dc.index('mActionMode1 == ACT_ENDING') > dc.index('dCcD_Sph dStack_160;'):
        e.append('damage_check no longer returns at once in ACT_ENDING')
    sd = body(tn, 'void daB_TN_c::setDamage(dCcD_Sph* i_sph, int param_2) {')
    if not sd.split('\n')[3].strip() == 'health = 100;' or '        if (field_0x6fc >= field_0x700) {\n            setActionMode(ACT_ENDING, ACTION2_0_e);' not in sd or 'field_0x6fc += 100 - health;' not in sd:
        e.append('setDamage second-phase accumulator / lethal transition changed')
    if len(re.findall(r'setActionMode\(ACT_ENDING', tn)) != 1:
        e.append('ACT_ENDING has another entry than setDamage')
    if len(re.findall(r'field_0x6fc\s*(?:[-+]?=)', tn)) != 3:
        e.append('field_0x6fc has another writer')
    ac = body(tn, 'void daB_TN_c::action() {')
    for need in ('    damage_check();\n    field_0x700 = l_HIO.field_0x24;', '    case ACT_ENDING:\n        if (mType == 0) {\n            executeEnding();\n        } else {\n            executeZakoEnding();\n        }'):
        if need not in ac:
            e.append('action() order changed: ' + need.strip()[:50])
    if ac.index('executeZakoEnding();') > ac.index('calcPartMove();') or ac.index('calcPartMove();') > ac.index('mpModelMorf2->play('):
        e.append('action(): ending, part move and model play order changed')
    if body(tn, 'void daB_TN_c::setActionMode(int i_mode1, int i_mode2) {').split('{', 1)[1].split() != ['offHeadLockFlg();', 'mActionMode1', '=', 'i_mode1;', 'mActionMode2', '=', 'i_mode2;', '}']:
        e.append('setActionMode is not the three writes the back-to-combat writer replicates')
    dl = body(tn, 'void daB_TN_c::executeDamageL() {')
    if '        if (mpModelMorf2->isStop()) {\n            setActionMode(ACT_CHASEL, ACTION2_0_e);\n            field_0xa91 = true;' not in dl:
        e.append('the native exit of executeDamageL (ACT_CHASEL 0 + field_0xa91) changed')
    cl = body(tn, 'void daB_TN_c::executeChaseL() {')
    if '    case ACTION2_0_e:\n        field_0xa9d = 0;' not in cl or '        setSwordAtBit(0);\n        field_0xa91 = true;\n        break;' not in cl:
        e.append('executeChaseL mode 0 changed')
    cr = body(tn, 'int daB_TN_c::create() {')
    for need in ('mSwitchNo = fopAcM_GetParam(this);', 'mType = fopAcM_GetParamBit(this, 8, 8);', 'mType2 = fopAcM_GetParamBit(this, 24, 24);', 'mStayNo2Flag = true;'):
        if need not in cr:
            e.append('create() identity changed: ' + need)
    sb = body(tn, 'void daB_TN_c::calcSwordMoveB() {')
    if '            } else {\n                fopAcM_createDisappear(this, pos, 3, 0, 0xff);\n                mStates[15] = 7;' not in sb:
        e.append('calcSwordMoveB mType 1 drop changed')
    ms = body(tn, 'void daB_TN_c::mtx_set() {')
    if '        case 1:\n            if (i == 15) {\n                mDoMtx_stack_c::copy(model1->getAnmMtx(15));' not in ms:
        e.append('the in-hand sword (mStates[15] 1) changed')
    dr = body(tn, 'int daB_TN_c::draw() {')
    if '    if (mMaterialColor) {' not in dr:
        e.append('draw darkening gate changed')
    if 'void daB_TN_c::setBck(int i_index, u8 i_attr, f32 i_morf, f32 i_rate) {\n    mpModelMorf2->setAnm((J3DAnmTransform*)dComIfG_getObjectRes("B_tn", i_index), i_attr, i_morf,\n                         i_rate, 0.0f, -1.0f);' not in tn:
        e.append('setBck changed (every body clip is a B_tn archive BCK)')
    # Link's Ending Blow
    cu = (s / 'src/d/actor/d_a_alink_cut.inc').read_text(encoding='utf-8')
    ds = body(cu, 'BOOL daAlink_c::checkDownAttackState() {')
    for need in ('fopEn_enemy_c* enemy = (fopEn_enemy_c*)mTargetedActor;', '&& enemy->checkDownFlg()', '&& enemy->getDownPos().abs2(current.pos) < SQUARE(800.0f)'):
        if need not in ds:
            e.append('checkDownAttackState changed: ' + need)
    if '    } else if (checkDownAttackState()) {\n        return procCutDownInit();' not in body(cu, 'BOOL daAlink_c::checkDoCutAction() {'):
        e.append('the Finish press no longer re-checks the down state')
    ci = body(cu, 'int daAlink_c::procCutDownInit() {')
    if 'field_0x280c.setData(mTargetedActor);' not in ci or 'commonProcInit(PROC_CUT_DOWN);' not in ci:
        e.append('procCutDownInit target / procedure changed')
    li = body(cu, 'int daAlink_c::procCutDownLandInit(fopEn_enemy_c* i_enemy) {')
    for need in ('i_enemy->checkDownFlg() &&', 'mSwordTopPos.abs2XZ(i_enemy->getDownPos()) < SQUARE(100.0f)', 'fabsf(i_enemy->current.pos.y - current.pos.y) < 100.0f',
                 '            i_enemy->onCutDownHitFlg();', 'commonProcInit(PROC_CUT_DOWN_LAND);'):
        if need not in li:
            e.append('procCutDownLandInit receipt changed: ' + need)
    if len(re.findall(r'onCutDownHitFlg\(\)', cu)) != 1:
        e.append('another native writer of the Ending Blow receipt in alink_cut')
    if '                    procBackJumpInit(1);' not in body(cu, 'int daAlink_c::procCutDownLand() {'):
        e.append('the success exit (back jump) changed')
    h = (s / 'include/d/actor/d_a_alink.h').read_text(encoding='utf-8')
    for need in ('/* 0x012 */ PROC_BACK_JUMP,', '/* 0x013 */ PROC_BACK_JUMP_LAND,', '/* 0x02B */ PROC_CUT_DOWN,', '/* 0x02C */ PROC_CUT_DOWN_LAND,', 'daPy_actorKeep_c field_0x280c;'):
        if need not in h:
            e.append('daAlink_c enum / field changed: ' + need)
    en = (s / 'include/f_op/f_op_actor.h').read_text(encoding='utf-8')
    for need in ('void onDownFlg() { mFlags |= (u16)fopEn_flag_Down; }', 'BOOL checkCutDownHitFlg() const { return mFlags & fopEn_flag_CutDownHit; }',
                 'void offDownFlg() { mFlags &= (u16)~(fopEn_flag_WolfDownPull | fopEn_flag_WolfDownStart | fopEn_flag_CutDownHit | fopEn_flag_Down); }',
                 'void setDownPos(const cXyz* i_pos) { mDownPos = *i_pos; }'):
        if need not in en:
            e.append('fopEn_enemy_c down flags changed: ' + need[:40])
    # the clip object and its loader
    ext = (s / 'include/m_Do/m_Do_ext.h').read_text(encoding='utf-8')
    if 'class mDoExt_transAnmBas : public J3DAnmTransformKey {\npublic:\n    mDoExt_transAnmBas(void* bas) { mBas = bas; }\n    virtual ~mDoExt_transAnmBas() {}\n\n    void* getBas() { return mBas; }\n\nprivate:\n    /* 0x28 */ void* mBas;\n};' not in ext:
        e.append('mDoExt_transAnmBas changed')
    me = (s / 'src/m_Do/m_Do_ext.cpp').read_text(encoding='utf-8')
    sa = body(me, 'void mDoExt_McaMorfSO::setAnm(J3DAnmTransform* i_anm, int i_attr, f32 i_morf, f32 i_rate,')
    if 'mpBas = static_cast<mDoExt_transAnmBas*>(i_anm)->getBas();' not in sa or 'mFrameCtrl.init(mpAnm->getFrameMax());' not in sa:
        e.append('McaMorfSO::setAnm changed')
    rs = (s / 'src/d/d_resorce.cpp').read_text(encoding='utf-8')
    if 'mDoExt_transAnmBas* transAnmBas = JKR_NEW mDoExt_transAnmBas(bas);' not in rs or 'J3DAnmLoaderDataBase::setResource(transAnmBas, res);' not in rs:
        e.append('archive BCKs are no longer transAnmBas + setResource')
    ld = (s / 'libs/JSystem/src/J3DGraphLoader/J3DAnmLoader.cpp').read_text(encoding='utf-8')
    st = body(ld, 'void J3DAnmKeyLoader_v15::setAnmTransform(J3DAnmTransformKey* param_1,')
    if '#if TARGET_LITTLE_ENDIAN\n    ByteSwapTransformKeyData(*param_1, *param_2);\n#endif' not in st or "case 'bck1': {\n        J3DAnmKeyLoader_v15 loader;\n        loader.setResource(param_1, i_data);" not in ld:
        e.append('the native BCK binding / in-place swap changed')
    an = (s / 'libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp').read_text(encoding='utf-8')
    if 'J3DAnmTransformKeyTable* entryX = &mAnmTable[idx];' not in an or 'u16 idx = jointNo * 3;' not in an:
        e.append('calcTransform key table layout changed')
    ru = (s / 'src/dusk/mods/svc/resource.cpp').read_text(encoding='utf-8')
    if 'return fmt::format("res/{}", relativePath);' not in ru or 'void* copy = std::malloc(data.size());' not in ru:
        e.append('ResourceService path / owned copy changed')
    mk = (s / 'cmake/ModSDK.cmake').read_text(encoding='utf-8')
    if '        list(APPEND _zip_args res)' not in mk:
        e.append('RES_DIR packaging changed')
    return e


if __name__ == '__main__':
    errs = audit(sys.argv[1])
    print(json.dumps({'status': 'DKR_NATIVE_CONTRACT_PASS' if not errs else 'DKR_NATIVE_CONTRACT_FAIL', 'errors': errs, 'source': sys.argv[1].replace('\\', '/').split('/')[-1]}, indent=1))
    sys.exit(1 if errs else 0)
