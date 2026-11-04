void daE_OC_c::executeFind() {
    s16 pl_ang = fopAcM_searchPlayerAngleY(this);
    f32 pl_dist = fopAcM_searchPlayerDistance(this);
    if (mOcState < 3 || !setWatchMode()) {
        if (field_0x6b4 == 2 && !dComIfGp_event_runCheck()) {
            fopAcM_OffStatus(this, fopAcStts_UNK_0x4000_e);
            field_0x6b4 = 0;
        }

        mPrevShapeAngle = pl_ang;
        switch (mOcState) {
            case 0:
            case 1:
            case 2:
                field_0x6de = 0;
                if (mOcState == 0) {
                    field_0x6c0 = 0;
                    field_0x6c2 = field_0x6c0 + 0x14;
                    if (checkBck(0x1c) == 0) {
                        setBck(0x1c, 2, 5.0f, 1.0f);
                        mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);
                    }

                    field_0x6de = 1;
                } else if (mOcState == 1) {
                    if (field_0x6e2) {
                        field_0x6c0 = l_HIO.demo_waiting_time;
                    } else {
                        field_0x6c0 = cM_rndF(10.0f) + 20.0f;
                    }

                    field_0x6e2 = 0;
                    field_0x6ca = 0x96;
                    setBck(0xd, 0, 5.0f, 1.0f);
                    mSound.startCreatureVoice(Z2SE_EN_OC_V_FIND, -1);
                } else {
                    field_0x6c0 = cM_rndF(5.0f) + 5.0f;
                    setBck(0xd, 0, 5.0f, 1.0f);
                    mSound.startCreatureVoice(Z2SE_EN_OC_V_FIND, -1);
                }

                mOcState = 3;
                speedF = 0.0f;
            /* fallthrough intentional */
            case 3:
                if (field_0x6c0 == 0) {
                    if (field_0x6de || pl_dist < 400.0f) {
                        setBck(0x1e, 2, 5.0f, 1.3f);
                        field_0x6de = 1;
                        mSound.startCreatureVoice(Z2SE_EN_OC_V_WALK_ST, -1);
                    } else {
                        setBck(0xb, 2, 5.0f, 1.2f);
                        mSound.startCreatureVoice(Z2SE_EN_OC_V_DASH, -1);
                    }

                    mOcState = 4;
                    field_0x6c0 = 0x1e;
                }

                break;
            case 4:
                if (checkBeforeFloorBg(200.0f)) {
                    if (field_0x6e3) {
                        setActionMode(E_OC_ACTION_MOVE_OUT, 0);
                    } else {
                        setActionMode(E_OC_ACTION_FIND_STAY, 0);
                    }

                    return;
                }

                mBattleOn = true;
                if ((s16)cLib_distanceAngleS(shape_angle.y, pl_ang) >= 0x400) {
                    cLib_addCalcAngleS(&shape_angle.y, pl_ang, 4, 0x800, 0x100);
                }

                current.angle.y = shape_angle.y;
                if (field_0x6ce) {
                    current.angle.y = field_0x6da;
                } else {
                    if (checkBeforeBgFind()) {
                        field_0x6ce = 20;
                    }
                }

                if (!dComIfGp_event_runCheck()) {
                    if (field_0x6c2 == 0) {
                        if (pl_dist < 300.0f) {
                            cLib_chaseF(&speedF, -3.0f, 1.0f);
                        } else {
                            cLib_chaseF(&speedF, 20.0f + nREG_F(0), 1.0f);
                        }

                        if (pl_dist < 400.0f && pl_dist > 200.0f) {
                            if (abs(shape_angle.y - fopAcM_searchPlayerAngleY(this)) < 0x1000) {
                                if (!dComIfGp_event_runCheck()) {
                                    setActionMode(E_OC_ACTION_ATTACK, 0);
                                }
                            }

                            return;
                        }
                    } else {
                        if (pl_dist < 400.0f && pl_dist > 200.0f) {
                            cLib_chaseF(&speedF, 0.0f, 1.0f);
                        } else if (pl_dist < 300.0f) {
                            cLib_chaseF(&speedF, -3.0f, 1.0f);
                        } else {
                            cLib_chaseF(&speedF, 20.0f + nREG_F(0), 1.0f);
                        }

                        if (speedF < 0.0f) {
                            if (checkBck(0x1c) == 0 || mpMorf->getPlaySpeed() != 1.0f) {
                                setBck(0x1c, 2, 5.0f, 1.0f);
                                mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);
                            }
                        } else if (!speedF && (s16)cLib_distanceAngleS(shape_angle.y, pl_ang) < 0x1000) {
                            if (checkBck(0x1c) == 0) {
                                setBck(0x1c, 2, 5.0f, 1.0f);
                                mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);
                            }
                        } else {
                            if (checkBck(0x1e) == 0 || mpMorf->getPlaySpeed() != 1.3f) {
                                setBck(0x1e, 2, 5.0f, 1.3f);
                                mSound.startCreatureVoice(Z2SE_EN_OC_V_WALK_ST, -1);
                            }
                        }
                    }
                } else {
                    field_0x6c2 = 0x14;
                    if (mAcch.ChkWallHit()) {
                        field_0x6c4 = 5;
                    }

                    if (pl_dist < 400.0f && field_0x6c4 == 0) {
                        cLib_chaseF(&speedF, -15.0f, 1.0f);
                    } else {
                        cLib_chaseF(&speedF, 0.0f, 1.0f);
                    }

                    if (speedF < 0.0f) {
                        if (checkBck(0x1e) == 0 || mpMorf->getPlaySpeed() != -1.0f) {
                            setBck(0x1e, 2, 5.0f, -1.0f);
                            mSound.startCreatureVoice(Z2SE_EN_OC_V_WALK_ST, -1);
                        }
                    } else if (!speedF && (s16)cLib_distanceAngleS(shape_angle.y, pl_ang) < 0x1000) {
                        if (checkBck(0x1c) == 0) {
                            setBck(0x1c, 2, 5.0f, 1.0f);
                            mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);
                        }
                    } else {
                        if (checkBck(0x1e) == 0 || mpMorf->getPlaySpeed() != 1.3f) {
                            setBck(0x1e, 2, 5.0f, 1.3f);
                            mSound.startCreatureVoice(Z2SE_EN_OC_V_WALK_ST, -1);
                        }
                    }

                    if (0 == strcmp("D_MN05", dComIfGp_getStartStageName())
                        && dComIfGp_getStartStageRoomNo() == 0x16) {
                        // Forest Temple - Entrance
                        fopAc_ac_c* ks_actor;
                        // Caged girl monkey in 1st room:
                        fopAcM_SearchByName(fpcNm_NPC_KS_e, &ks_actor);
                        if (ks_actor) {
                            mWatchPos = ks_actor->current.pos;
                            mWatchPos.y += 100.0f;
                            mPrevShapeAngle = cLib_targetAngleY(&current.pos, &mWatchPos);
                        }
                    }
                }

                if (field_0x6c0 == 0) {
                    if (!searchPlayer2()) {
                        setActionMode(E_OC_ACTION_WAIT, 0);
                    } else if (field_0x6e3) {
                        setActionMode(E_OC_ACTION_MOVE_OUT, 0);
                    }
                }

                break;
            case 5:
                setBck(0x16, 0, 5.0f, 1.0f);
                mSound.startCreatureVoice(Z2SE_EN_OC_V_SEARCH, -1);
                mOcState = 6;
                field_0x6de = 0;
                // fallthrough intentional.
            case 6:
                mBattleOn = true;
                mPrevShapeAngle = shape_angle.y;
                if (mpMorf->isStop()) {
                    mOcState = 0;
                }

                if (pl_dist < 500.0f) {
                    mOcState = 0;
                }

                if (mpMorf->getFrame() >= 20.0f && searchPlayerShakeHead()) {
                    mOcState = 0;
                }

                break;
            default:
                break;
        }
    }
}
