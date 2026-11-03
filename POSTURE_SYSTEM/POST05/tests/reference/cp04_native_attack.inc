void daE_OC_c::executeAttack() {
    f32 my_float = 0.0f;
    int frame_ctrl = (mpMorf->getFrame() - 9.0f);
    if (frame_ctrl >= 0) {
        if (frame_ctrl >= 9) {
            frame_ctrl = 9;
        }

        if (checkBck(5)) {
            my_float = E_OC_n::oc_attackb_trans[frame_ctrl];
        } else {
            my_float = E_OC_n::oc_attackc_trans[frame_ctrl];
        }
    }

    mBattleOn = true;
    mPrevShapeAngle = shape_angle.y;
    switch (mOcState) {
        case 0: {
            if (cLib_chaseF(&speedF, 0.0f, 1.0f)) {
                if (cM_rndF(1.0f) < 0.5f) {
                    setBck(5, 0, 5.0f, 1.0f);
                    mSound.startCreatureVoice(Z2SE_EN_OC_V_ATTACK_B, -1);
                    mOcState = 1;
                } else {
                    setBck(6, 0, 5.0f, 1.0f);
                    mSound.startCreatureVoice(Z2SE_EN_OC_V_ATTACK_C, -1);
                    mOcState = 2;
                }

                field_0x6a0 = 0.0f;
            }

            break;
        }

        case 2: {
            MtxP my_mtx_p = mpMorf->getModel()->getAnmMtx(0);
            cXyz my_vec_0(my_mtx_p[0][3], my_mtx_p[1][3], my_mtx_p[2][3]);
            mParticleKeys[0] = dComIfGp_particle_set(mParticleKeys[0], dPa_RM(ID_ZM_S_ORCATTACKBLUR00),
                                                     &my_vec_0, &tevStr,
                                                     &shape_angle, NULL, -1, NULL, -1,
                                                     NULL, NULL, NULL);
            mParticleKeys[1] = dComIfGp_particle_set(mParticleKeys[1], dPa_RM(ID_ZM_S_ORCATTACKBLUR01),
                                                     &my_vec_0, &tevStr,
                                                     &shape_angle, NULL, -1, NULL, -1,
                                                     NULL, NULL, NULL);
        }
        // fallthrough intentional
        case 1: {
            if (mOcState == 1) {
                if (mpMorf->checkFrame(10.0f)) {
                    mSound.startCreatureVoice(Z2SE_EN_OC_V_ATTACK_B2, -1);
                } else if (mpMorf->checkFrame(12.5f)) {
                    mSound.startCreatureSound(Z2SE_EN_OC_ATTACK_B, 0, -1);
                } else if (mpMorf->checkFrame(19.0f)) {
                    // See comment in damage_check
#if AVOID_UB
                    if (strcmp(mName, "E_OC") == 0) {
#else
                    if (mName == "E_OC") {
#endif
                        mSound.startCreatureSound(Z2SE_EN_OC_CLUB_HIT, 0, -1);
                    } else {
                        mSound.startCreatureSound(Z2SE_EN_OC_NATA_HIT, 0, -1);
                    }
                }
            } else {
                if (mpMorf->checkFrame(7.0f)) {
                    mSound.startCreatureVoice(Z2SE_EN_OC_V_ATTACK_C2, -1);
                } else if (mpMorf->checkFrame(11.5f)) {
                    mSound.startCreatureSound(Z2SE_EN_OC_ATTACK_C, 0, -1);
                }
            }

            if (mpMorf->getFrame() >= 14.0f && mpMorf->getFrame() <= 22.0f) {
                mSphs_at[0].OnAtSetBit();
                mSphs_at[1].OnAtSetBit();
            }

            if (mpMorf->getFrame() >= 22.0f) {
                mPrevShapeAngle = fopAcM_searchPlayerAngleY(this);
            }

            u8 my_bool = 0;
            fopAc_ac_c* hit_actor;
            if (mSphs_at[0].ChkAtHit()) {
                hit_actor = dCc_GetAc(mSphs_at[0].GetAtHitObj()->GetAc());
                if (fopAcM_GetName(hit_actor) == fpcNm_ALINK_e) {
                    my_bool = 1;
                }
            }

            if (mSphs_at[1].ChkAtHit()) {
                hit_actor = dCc_GetAc(mSphs_at[1].GetAtHitObj()->GetAc());
                if (fopAcM_GetName(hit_actor) == fpcNm_ALINK_e) {
                    my_bool = 1;
                }
            }

            if (my_bool && daPy_getPlayerActorClass()->checkPlayerGuard()) {
                mpMorf->setPlaySpeed(-1.0);
                mOcState = 3;
                dComIfGp_getVibration().StartShock(3, 0x1f, cXyz(0.0f, 1.0f, 0.0f));
            }

            current.pos.x += (my_float - field_0x6a0) * cM_ssin(shape_angle.y);
            current.pos.z += (my_float - field_0x6a0) * cM_scos(shape_angle.y);
            field_0x6a0 = my_float;
            if (mpMorf->isStop()) {
                setBck(0x1c, 2, 0.0f, 1.0f);
                mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);
                if (field_0x6e3) {
                    setActionMode(E_OC_ACTION_MOVE_OUT, 0);
                } else if (field_0x6ca && fopAcM_searchPlayerDistance(this) < 500.0f) {
                    mOcState = 0;
                } else {
                    setActionMode(E_OC_ACTION_FIND, 0);
                }
            }

            break;
        }

        case 3: {
            current.pos.x += (my_float - field_0x6a0) * cM_ssin(shape_angle.y);
            current.pos.z += (my_float - field_0x6a0) * cM_scos(shape_angle.y);
            field_0x6a0 = my_float;
            if (!mpMorf->isStop()) {
                break;
            }

            setBck(0x1c, 2, 5.0f, 1.0f);
            mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);
            if (field_0x6e3) {
                setActionMode(E_OC_ACTION_MOVE_OUT, 0);
                break;
            }

            if (field_0x6ca && fopAcM_searchPlayerDistance(this) < 500.0f) {
                if (abs(shape_angle.y - fopAcM_searchPlayerAngleY(this)) < 0x1000) {
                    mOcState = 0;
                    break;
                }
            }

            setActionMode(E_OC_ACTION_FIND, 0);
            int _; // forces b in dbg asm
            break;
        }
    }

    current.angle.y = shape_angle.y;
}

