"""LIGHT FADES TWICE 0.13.0 CHILFOS01: the native lines of TwilitRealm/dusklight@40457c6 the Chilfos module relies on (src/d/actor/d_a_e_kk.cpp and the
E_KK resource header). If any line changes, the module's assumptions (frame events, lethal branches, spear child, HP, AT gating) are void: FAIL.
Usage: kk_native_audit.py <official dusklight source>"""
import json, sys
from pathlib import Path
src = Path(sys.argv[1])
KK = (src / 'src/d/actor/d_a_e_kk.cpp').read_text(encoding='utf-8')
RES = (src / 'assets/GZ2E01/res/Object/E_kk.h').read_text(encoding='utf-8')
SE = (src / 'include/Z2AudioLib/Z2SeMgr.h').read_text(encoding='utf-8')
CUT = (src / 'src/d/actor/d_a_alink_cut.inc').read_text(encoding='utf-8')
PLY = (src / 'include/d/actor/d_a_player.h').read_text(encoding='utf-8')
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
    # FIX01 0.13.1: the native lines the anti-stagger, the absorbed Shield Attack and the backstep speed rely on
    'action_damage_check_first': ('void daE_KK_c::action() {\n    BOOL link_search = false;\n    damage_check();\n\n    switch (mActionMode) {', KK),
    'damage_heading_snap': ('            current.angle.y = shape_angle.y;\n            mDamageTimer = 8;\n            mAtInfo.mpCollider = mCyl.GetTgHitObj();', KK),
    'shield_attack_se': ('                onHeadLockFlg();\n                if (field_0x67d == 0) {\n                    mCreatureSound.startCollisionSE(Z2SE_HIT_SWORD, 0x28);', KK),
    'damage_exec_flinch': ('    case 1:\n        if (field_0x67c == 0) {\n            setBck(0xC, 0, 3.0f, 1.0f);', KK),
    'back_walk_speed15': ('    case 2:\n        speedF = 15.0f;\n        if (mTimer == 0 || dComIfG_Bgsp().LineCross(&linChk) || way_gake_check()) {', KK),
    'cc_set_eye_recomputed': ('        eyePos = attention_info.position;\n        eyePos.y -= 50.0f;\n        setHeadLockPos(&eyePos);', KK),
    'se_hit_sword_value': ('    Z2SE_HIT_SWORD = 0x40000,', SE),
    # FIX02 0.13.2: the native lines the spear-intact block, the counter dispatch and the motion fixes rely on
    'strong_hit_spear_branch': ('                    if (mAtInfo.mHitStatus != 0 || dComIfGp_checkPlayerStatus0(0, 0x400) ||\n                        mCyl.GetTgHitObj()->ChkAtType(AT_TYPE_MIDNA_LOCK))\n                    {\n                        def_se_set(&mCreatureSound, mAtInfo.mpCollider, 0x1F, NULL);\n                        setActionMode(7, 0);', KK),
    'damage_exec_clash_disarm': ('        setBck(8, 0, 3.0f, 1.0f);\n        mDeadEffSet(field_0x698);\n        speedF = -80.0f;', KK),
    'damage_exec_clash_spear_off': ('        field_0x67d = 1;\n        mCyl.OffTgShield();\n        field_0x67c = (s32)cM_rndF(2.0f) & 1;', KK),
    'weapon_shield_recoil_70': ('            mSph.ClrAtHit();\n            mDamageTimer = 8;\n            speedF = -70.0f;', KK),
    'speed_decay_addcalc': ('        if (mActionMode != 4 && mActionMode != 2) {\n            cLib_addCalc0(&speedF, 0.7f, 10.0f);', KK),
    'guard_native_entry': ('    case 0:\n        setWeaponBck(0x1D, 0, 0.0f, 1.0f);\n        field_0x67c &= 1;', KK),
    'attack_entry_mode0': ('    case 0:\n        field_0x67c = 0;\n        if (fopAcM_searchPlayerDistance(this) > 600.0f) {', KK),
    # FIX03 0.13.3: the instant-kill test (exact list), its use in damage_check, Link's Mortal Draw entry and the cut ids
    'cutcheck_instant_list': ('    if (player->getCutType() == daPy_py_c::CUT_TYPE_LARGE_JUMP ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_LARGE_JUMP_FINISH ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_MORTAL_DRAW_A ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_MORTAL_DRAW_B)', KK),
    'cutcheck_helm_twirl': ('    if (player->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP ||\n        player->getCutType() == daPy_py_c::CUT_TYPE_TWIRL)\n    {\n        return 1;', KK),
    'damage_instant_kill_branch': ('            } else if ((mCyl.GetTgHitObj()->ChkAtType(AT_TYPE_IRON_BALL) &&\n                        !dComIfGp_checkPlayerStatus0(0, 0x400)) ||\n                       mCutTypeCheck(2))', KK),
    'link_finish_param_order': ('    CUT_FINISH_PARAM_LEFT,\n    CUT_FINISH_PARAM_VERTICAL,\n    CUT_FINISH_PARAM_STAB,\n    CUT_FINISH_PARAM_MORTAL_DRAW_A,\n    CUT_FINISH_PARAM_MORTAL_DRAW_B,\n    CUT_FINISH_PARAM_RIGHT,', CUT),
    'link_md_b_table': ('            ANM_CUT_MORTAL_DRAW_B,\n            ANM_CUT_RECOIL_A,\n            CUT_TYPE_MORTAL_DRAW_B,', CUT),
    'link_md_native_choice': ('            if (checkInputOnR() && (cutDir == DIR_FORWARD || cutDir == DIR_BACKWARD)) {\n                procCutFinishInit(CUT_FINISH_PARAM_MORTAL_DRAW_A);\n            } else {\n                procCutFinishInit(CUT_FINISH_PARAM_MORTAL_DRAW_B);', CUT),
    'link_finish_proc': ('    commonProcInit(PROC_CUT_FINISH);\n    setCutType(cutParams->m_cutType);', CUT),
    'cut_ids': ('        /* 0x05 */ CUT_TYPE_HEAD_JUMP,  // Helm Splitter', PLY),
    'cut_ids_twirl_md': ('        /* 0x1E */ CUT_TYPE_TWIRL,  // Back Slice\n        /* 0x1F */ CUT_TYPE_MORTAL_DRAW_B,', PLY),
    'cut_id_md_a': ('        /* 0x1A */ CUT_TYPE_MORTAL_DRAW_A,', PLY),
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
