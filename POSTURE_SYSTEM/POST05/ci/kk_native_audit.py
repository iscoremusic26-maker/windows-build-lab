"""LIGHT FADES TWICE 0.13.0 CHILFOS01 (+ FIX04, FIX05 0.13.5): the native lines of TwilitRealm/dusklight@40457c6 the Chilfos module relies on (src/d/actor/d_a_e_kk.cpp and the
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
# FIX04 0.13.4
ACM = (src / 'src/f_op/f_op_actor_mng.cpp').read_text(encoding='utf-8')
KCL = (src / 'src/d/d_bg_w_kcol.cpp').read_text(encoding='utf-8')
CCS = (src / 'src/SSystem/SComponent/c_cc_s.cpp').read_text(encoding='utf-8')
DCS = (src / 'src/d/d_cc_s.cpp').read_text(encoding='utf-8')
LIB = (src / 'src/SSystem/SComponent/c_lib.cpp').read_text(encoding='utf-8')
UTY = (src / 'src/d/d_cc_uty.cpp').read_text(encoding='utf-8')
# FIX05 0.13.5
ALK = (src / 'src/d/actor/d_a_alink.cpp').read_text(encoding='utf-8')
HIO = (src / 'src/d/actor/d_a_alink_HIO_data.inc').read_text(encoding='utf-8')
J3D = (src / 'libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp').read_text(encoding='utf-8')
MDO = (src / 'include/m_Do/m_Do_ext.h').read_text(encoding='utf-8')
ALH = (src / 'include/d/actor/d_a_alink.h').read_text(encoding='utf-8')
# FIX07 0.13.7
DN = (src / 'src/d/actor/d_a_e_dn.cpp').read_text(encoding='utf-8')
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
    # FIX04 0.13.4: the C3 wind-up BG line (P0), the spear's AT registration / stale flags / per-sphere SE (P1), the guard spark branches (P2), the guard turn (P3)
    'fix04_throw_cancel_bg_line': ('        if (!dComIfGp_event_runCheck() && fopAcM_searchPlayerDistance(this) <= l_HIO.direct_attack_range &&\n            !fopAcM_otherBgCheck(this, dComIfGp_getPlayer(0)) && (s32)mpMorfSO->getFrame() < 0x17 &&', KK),
    'fix04_other_bg_check_eye': ('    start = param_0->current.pos;\n    start.y = param_0->eyePos.y;\n\n    linChk.Set(&start, &end, param_0);', ACM),
    'fix04_kcl_stride_500': ('    f32 var_f25 = 500.0f / var_f31;', KCL),
    'fix04_kcl_stride_loop': ('        var_f29 += 500.0f;\n    } while (var_f29 <= var_f31);', KCL),
    'fix04_clr_at_registered_only': ('    for (cCcD_Obj** obj = mpObjAt; obj < mpObjAt + mObjAtCount; ++obj) {\n        if (*obj != NULL) {\n            (*obj)->GetGObjInf()->ClrAtHit();', CCS),
    'fix04_shaft_at_gate': ('                if (field_0x67e != 0) {\n                    mSpheres[i].OffAtSetBit();\n                } else {\n                    mSpheres[i].OnAtSetBit();', KK),
    'fix04_se_per_sphere': ('        for (int i = 0; i < 6; i++) {\n            if (mSpheres[i].ChkAtHit() && mSpheres[i].ChkAtShieldHit()) {\n                mCreatureSound.startCollisionSE(Z2SE_HIT_SWORD, 0x28);', KK),
    'fix04_strong_clash_no_mark': ('                        def_se_set(&mCreatureSound, mAtInfo.mpCollider, 0x1F, NULL);\n                        setActionMode(7, 0);', KK),
    'fix04_strong_status': ('            if (dCcD_GetGObjInf(i_AtInfo->mpCollider)->GetAtSpl() == 1) {\n                i_AtInfo->mHitStatus = 1;', UTY),
    'fix04_hitmark_eff_gate': ('    if (i_setAt && i_setTg && (!atObjInf->ChkAtEffCounter() || !tgObjInf->ChkTgEffCounter())) {', DCS),
    'fix04_guard_turn': ('    cLib_addCalcAngleS2(&current.angle.y, fopAcM_searchPlayerAngleY(this), 8, 0x500);\n    shape_angle.y = current.angle.y;', KK),
    'fix04_addcalc_angle': ('    s16 diff = target - *pvalue;\n    s16 step = diff / scale;\n    if (step > maxStep) {\n        *pvalue += maxStep;', LIB),
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
    # FIX05 0.13.5: P0 the stagger clip (one play, EMode_LOOP never stops, isStop), P1 the Jump Attack rebound routes and its AT frame, P3 the Helm Splitter
    # aim point, P4 the torso aim of procCutFinish at its own target
    'fix05_yoroke_bow_once': ('        if (field_0x67d == 0) {\n            setBck(9, 0, 3.0f, 1.0f);\n        } else {\n            setBck(0xB, 0, 3.0f, 1.0f);\n        }', KK),
    'fix05_yoroke_leaves_at_stop': ('    case 2:\n        if (mpMorfSO->isStop()) {\n            offHeadLockFlg();\n            if (field_0x67d == 0) {\n                nextActionCheck();', KK),
    'fix05_morf_isstop': ('    BOOL isStop() { return mFrameCtrl.checkState(1) || mFrameCtrl.getRate() == 0.0f; }', MDO),
    'fix05_j3d_loop_wrap': ('        while (mFrame >= mEnd) {\n            mState |= (u8)2;\n            if (mEnd - mLoop <= 0.0f) {\n                break;\n            }\n            mFrame -= mEnd - mLoop;', J3D),
    'fix05_j3d_once_stop': ('        if (mFrame >= mEnd) {\n            mFrame = mEnd - 0.001f;\n            mRate = 0.0f;\n            mState |= (u8)1;', J3D),
    'fix05_headlock_is_eye': ('        eyePos = attention_info.position;\n        eyePos.y -= 50.0f;\n        setHeadLockPos(&eyePos);', KK),
    'fix05_jump_air_rebound': ('    if (current.angle.y == shape_angle.y && changeCutReverseProc(ANM_CUT_JUMP)) {\n        ANGLE_ADD_2(current.angle.y, 0x8000);\n        mNormalSpeed = 27.0f;', CUT),
    'fix05_jump_reverse_no_proc': ('    if (i_anmID == ANM_CUT_JUMP) {\n        return 1;\n    }\n\n    commonProcInit(PROC_CUT_REVERSE);', CUT),
    'fix05_jump_land_rebound': ('    if (changeCutReverseProc(ANM_CUT_RECOIL_B)) {\n        return 1;\n    }', CUT),
    'fix05_jump_settype_twice': ('    commonProcInit(PROC_CUT_JUMP);\n    setSingleAnimeParam(ANM_CUT_JUMP, &mpHIO->mCut.mCutJump.m.mCutAnm);', CUT),
    'fix05_jump_land_settype': ('    onResetFlg0(RFLG0_UNK_2);\n    mNormalSpeed = 0.0f;\n    setCutType(CUT_TYPE_JUMP);', CUT),
    'fix05_jump_at_start_frame': ('    25.0f,\n    27.0f,\n    15.0f,\n    13.0f,\n    13.0f,\n    1.0f,\n    3.0f,\n};', HIO),
    'fix05_jump_at_start_field': ('    /* 0x38 */ f32 mStartAttackFrame;\n    /* 0x3C */ f32 mEndAttackFrame;', ALH),
    'fix05_helm_aim': ('        sp2C = enemy_p->getHeadLockPos() - current.pos;', CUT),
    'fix05_helm_height': ('    f32 fvar9 = cLib_minMaxLimit<f32>(sp2C.y, 30.0f, mpHIO->mCut.mCutHead.m.mMaxHeight);', CUT),
    'fix05_finish_body_aim': ('    daPy_frameCtrl_c* frameCtrl_p = mUnderFrameCtrl;\n\n    setBodyAngleXReadyAnime(1);', CUT),
    'fix05_body_aim_target_eye': ('        cXyz spC = mTargetedActor->eyePos - sp18;', ALK),
    'fix05_finish_target_kept': ('    } else if (mProcID == PROC_CUT_FINISH && field_0x280c.getActor() != NULL) {\n        mTargetedActor = field_0x280c.getActor();', ALK),
    'fix05_md_target_set': ('    if (mAttention->LockonTarget(0) != NULL && var_r30) {\n        field_0x280c.setData(mAttention->LockonTarget(0));', CUT),
    # FIX07 0.13.7: the special skills (cut ids, their colliders and powers), the power table, the E_KK instant-kill / lethal branches and the E_DN guard order
    'fix07_cut_ids_spins': ('        /* 0x08 */ CUT_TYPE_TURN_RIGHT,', PLY),
    'fix07_cut_ids_spins_lr': ('        /* 0x16 */ CUT_TYPE_TURN_LEFT,\n        /* 0x17 */ CUT_TYPE_LARGE_TURN_LEFT,\n        /* 0x18 */ CUT_TYPE_LARGE_TURN_RIGHT,', PLY),
    'fix07_spin_power': ('    int atp;\n    if (checkCutLargeTurnState()) {\n        atp = 4;', CUT),
    'fix07_spin_normal_power': ('    } else {\n        atp = 3;\n        field_0x32cc = Z2SE_AL_KAITENGIRI;', CUT),
    'fix07_spin_at_sph': ('void daAlink_c::initCutTurnAt(f32 i_radius, int i_atp) {\n    mAtSph.ResetAtHit();', CUT),
    'fix07_backslice_cylinder_atp3': ('    setCylAtParam(getSwordAtType(), dCcG_At_Spl_UNK_1, 3, dCcD_SE_SWORD, 3,', CUT),
    'fix07_backslice_twirl': ('    setCutType(CUT_TYPE_TWIRL);\n    speed.y = mpHIO->mCut.mCutFinishJumpUppercut.m.mSpeedV;', CUT),
    'fix07_mortal_draw_atp4': ('        setSwordAtParam(dCcG_At_Spl_UNK_1, 3, cutParams->m_atSe, var_r30 ? 4 : 3,', CUT),
    'fix07_power_table': ('        } else if (power == 3) {\n            power *= 10;\n        } else if (power == 6) {\n            power = 80;\n        } else if (power >= 4) {\n            power = 200;', UTY),
    'fix07_kk_lethal_after_cc': ('                    cc_at_check(this, &mAtInfo);\n                    if (health <= 1 || mCyl.GetTgHitObj()->ChkAtType(AT_TYPE_BOMB)) {', KK),
    'fix07_kk_instant_kill_branch': ('                       mCutTypeCheck(2))\n            {\n                mCyl.OnTgNoHitMark();\n                health = 0;', KK),
    'fix07_dn_guard_entry_invuln': ('            anm_init(i_this, ANM_GUARD, 3.0f, J3DFrameCtrl::EMode_NONE, 1.0f);\n            i_this->sound.startCreatureVoice(Z2SE_EN_DN_V_GUARD, -1);\n            i_this->mode = 6;\n            // fallthrough\n        case 6:\n            i_this->invulnerability_timer = 5;', DN),
    'fix07_dn_tate_after_action': ('    if (i_this->guard_flag != 0 && i_this->unk_timer_2 == 0) {', DN),
    'fix07_dn_tate_sets_ut2': ('            dScnPly_c::setPauseTimer(l_HIO.defense_pause_time);\n            i_this->unk_timer_2 = 10;', DN),
    'fix07_dn_body_parked_invuln': ('    if (i_this->invulnerability_timer != 0) {\n        sp50.set(-20000.0f, 200000.0f, 30000.0f);', DN),
    'fix07_dn_death_big_only': ('                    if (actor->health <= 0 || i_this->at_info.mHitStatus != 0) {', DN),
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
