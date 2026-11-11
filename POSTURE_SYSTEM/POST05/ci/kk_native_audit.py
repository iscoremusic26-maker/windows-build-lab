"""LIGHT FADES TWICE 0.13.0 CHILFOS01: the native lines of TwilitRealm/dusklight@40457c6 the Chilfos module relies on (src/d/actor/d_a_e_kk.cpp and the
E_KK resource header). If any line changes, the module's assumptions (frame events, lethal branches, spear child, HP, AT gating) are void: FAIL.
Usage: kk_native_audit.py <official dusklight source>"""
import json, sys
from pathlib import Path
src = Path(sys.argv[1])
KK = (src / 'src/d/actor/d_a_e_kk.cpp').read_text(encoding='utf-8')
RES = (src / 'assets/GZ2E01/res/Object/E_kk.h').read_text(encoding='utf-8')
LINES = {
    'attack_open_frame24': ('        if ((s32)mpMorfSO->getFrame() == 0x18) {\n            speedF = 80.0f;\n            field_0x67e = 0;', KK),
    'attack_se03_bck6': ('            if (field_0x764 == 6) {\n                mCreatureSound.startCreatureSound(Z2SE_EN_KK_ATTACK03, 0, -1);', KK),
    'attack_se01_frame27': ('    if (field_0x764 == 5 && (s32)mpMorfSO->getFrame() == 0x1B) {', KK),
    'attack_close_ge33': ('    } else if ((s32)mpMorfSO->getFrame() >= 0x21) {\n        field_0x67e = 1;', KK),
    'attack_clip_5_or_6': ('            if (cM_rndF(1.0f) < 0.5f) {\n                setBck(5, 0, 3.0f, 1.0f);\n            } else {\n                setBck(6, 0, 3.0f, 1.0f);', KK),
    'attack_stop_next': ('        if (mpMorfSO->isStop()) {\n            nextActionCheck();\n        }\n        break;\n    }\n\n    if (dComIfG_Bgsp().LineCross(&linChk) && field_0x67c == 0) {', KK),
    'throw_spawn_frame23': ('        if ((s32)mpMorfSO->getFrame() == 0x17) {\n            fopAcM_createChild(fpcNm_E_KK_e, fopAcM_GetID(this), 0xFF0001, &field_0x698,', KK),
    'throw_disarm': ('            field_0x67d = 1;\n            mCyl.OffTgNoHitMark();\n            mCyl.OffTgShield();\n            mCreatureSound.startCreatureSound(Z2SE_EN_KK_THROW, 0, -1);', KK),
    'throw_melee_cancel': ('            if (!fopAcM_otherBgCheck(this, player) && eyePos.y + 50.0f > player->current.pos.y) {\n                setActionMode(8, 0);', KK),
    'throw_restore_clips': ('            setBck(0x12, 0, 0.0f, 1.0f);\n            setWeaponBck(0x1E, 0, 0.0f, 1.0f);', KK),
    'weapon_ready_ge22': ('        if ((s32)mpWeaponMorfSO->getFrame() >= 0x22) {', KK),
    'back_stop_restore': ('        if (mTimer == 0 || dComIfG_Bgsp().LineCross(&linChk) || way_gake_check()) {\n            setBck(0x13, 0, 0.0f, 1.0f);\n            setWeaponBck(0x1E, 0, 0.0f, 1.0f);', KK),
    'damage_early_return': ('    if (health <= 1 || field_0x679 == 1 || mActionMode == 10) {\n        return;', KK),
    'damage_timer_gate': ('    if (mDamageTimer == 0) {\n        if (mSph.ChkAtHit() && mSph.ChkAtShieldHit()) {', KK),
    'damage_auto_deflect': ('                if (mCutTypeCheck(0)) {\n                    mCyl.OnTgShield();\n                } else {\n                    mCyl.OffTgShield();', KK),
    'damage_shield_attack': ('            } else if (mCyl.GetTgHitObj()->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {\n                onHeadLockFlg();', KK),
    'damage_skill_lethal': ('                       mCutTypeCheck(2))\n            {\n                mCyl.OnTgNoHitMark();\n                health = 0;', KK),
    'damage_guard_branch': ('                        mCreatureSound.startCollisionSE(Z2SE_HIT_SWORD, 7);\n                        dComIfGp_setHitMark(2, this, &position, &shape_angle, NULL, 0);\n                        setActionMode(6, 0);', KK),
    'damage_spearless_hp': ('                    cc_at_check(this, &mAtInfo);\n                    if (health <= 1 || mCyl.GetTgHitObj()->ChkAtType(AT_TYPE_BOMB)) {', KK),
    'damage_spearless_reaction': ('                    mCyl.OffTgNoHitMark();\n                    setActionMode(7, 1);', KK),
    'damage_ice_particle': ('                dComIfGp_particle_set(0x85BA, &position, &shape_angle, &hioScale);', KK),
    'cut_helm_lethal': ('    if (player->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_TWIRL)', KK),
    'yoroke_wait_stop': ('    case 2:\n        if (mpMorfSO->isStop()) {\n            offHeadLockFlg();', KK),
    'damage_exec_backwalk': ('        if (mpMorfSO->isStop()) {\n            current.angle.y = fopAcM_searchPlayerAngleY(this) + 0x8000;\n            setActionMode(4, 0);', KK),
    'action_close_outside_attack': ('        if (mActionMode != 8) {\n            field_0x67e = 1;', KK),
    'cc_set_at_gate': ('    if (field_0x67d == 0 && field_0x67e == 0) {\n        mSph.SetC(field_0x68c);', KK),
    'mtx_tip': ('    mult_vec.set(230.0f, 0.0f, 0.0f);\n    mDoMtx_stack_c::multVec(&mult_vec, &field_0x68c);', KK),
    'create_hp_140': ('        health = 0x8C;\n        field_0x560 = 0x8C;', KK),
    'create_subtype': ('        field_0x679 = fopAcM_GetParam(this);', KK),
    'create_spear_child': ('        if (field_0x679 == 1) {\n            fopAcM_SetMtx(this, mpWeaponMorfSO->getModel()->getBaseTRMtx());\n            setActionMode(9, 0);', KK),
    'set_bck_records_index': ('    mpMorfSO->setAnm(animation, i_attr, i_morf, i_rate, 0.0f, -1.0f);\n    field_0x764 = i_index;', KK),
    'res_attack01': ('dRes_INDEX_E_KK_BCK_KK_ATTACK01_e=0x5,', RES),
    'res_attack03': ('dRes_INDEX_E_KK_BCK_KK_ATTACK03_e=0x6,', RES),
    'res_throw': ('dRes_INDEX_E_KK_BCK_KK_THROW_e=0x18,', RES),
    'res_back_walk': ('dRes_INDEX_E_KK_BCK_KK_BACK_WALK_e=0x7,', RES),
    'res_weapon_restore': ('dRes_INDEX_E_KK_BCK_KK_WEPON_RESTORE_e=0x1E,', RES),
}
out = {'status': 'KK_NATIVE_CONTRACT_PASS', 'checked': len(LINES), 'errors': []}
for name, (snippet, text) in LINES.items():
    n = text.count(snippet)
    if n < 1:
        out['errors'].append('%s: native line not found' % name)
# negatives: a changed native line (one character of each of the first 8 snippets) must be rejected by the same check
negatives = 0
for name, (snippet, text) in list(LINES.items())[:8]:
    changed = text.replace(snippet, snippet[:-1] + ('X' if snippet[-1] != 'X' else 'Y'), 1)
    if changed.count(snippet) != 0:
        out['errors'].append('%s: negative not rejected' % name)
    negatives += 1
out['negatives_rejected'] = negatives
if out['errors']:
    out['status'] = 'KK_NATIVE_CONTRACT_FAIL'
print(json.dumps(out, indent=1))
sys.exit(1 if out['errors'] else 0)
