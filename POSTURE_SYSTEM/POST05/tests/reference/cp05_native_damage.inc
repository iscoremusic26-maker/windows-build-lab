void daE_OC_c::damage_check() {
    if (checkCutDownHitFlg()) {
        // Following needed to use r31 for debug asm:
        int unused;
        offCutDownHitFlg();
        health = 0;
        setActionMode(E_OC_ACTION_DEATH, 1);
        offTgSph();
        return;
    }

    if (field_0x6cc) {
        return;
    }

    mStts.Move();
    int had_hit = 0;
    cXyz my_vec_0;
    if (mSphs_cc[0].ChkTgHit()) {
        mAtInfo.mpCollider = mSphs_cc[0].GetTgHitObj();
        had_hit = 1;
        my_vec_0 = *mSphs_cc[0].GetTgHitPosP();
    }

    if (mSphs_cc[1].ChkTgHit()) {
        mAtInfo.mpCollider = mSphs_cc[1].GetTgHitObj();
        had_hit = 1;
        my_vec_0 = *mSphs_cc[1].GetTgHitPosP();
    }

    if (had_hit == 0) {
        return;
    }

    s16 old_health = health;    // unused.
    if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_UNK)) {
        field_0x6cc = 0x14;
    } else {
        field_0x6cc = 10;
    }

    if (mAtInfo.mAttackPower <= 1) {
        field_0x6cc = 10 + KREG_S(8);
    }

    u8 my_val = 2;
    if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_MASTER_SWORD | AT_TYPE_NORMAL_SWORD)) {
        my_val = getCutType();
        if (daPy_py_c::checkWoodSwordEquip()) {
            my_val = 2;
        }

        if (my_val <= 2) {
            if (field_0x6d6) {
                field_0x6dc ^= (u8) 1;
                my_val = field_0x6dc;
            } else {
                field_0x6dc = my_val;
            }

            field_0x6d6 = 0x1e;
        }

        if (daPy_getPlayerActorClass()->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP) {
            health = 0;
        }
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
        my_val = 3;
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_ARROW)) {
        if (cM_rnd() < 0.5f) {
            my_val = 0;
        } else {
            my_val = 1;
        }
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_BOMB)) {
        my_val = 5;
        health = 0;
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_IRON_BALL)) {
        my_val = 5;
        if (dComIfGp_checkPlayerStatus0(0,0x400)) {
            S16_ADD(health, 140);
        } else {
            S16_ADD(health, 80);
        }
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_BOOMERANG)) {
        my_val = 4;
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_40)) {
        S16_ADD(health, 10);
    } else if (mAtInfo.mpCollider->ChkAtType(AT_TYPE_SLINGSHOT)) {
        // This happens to work with MWCC since the member will only ever be initialized a pointer to a
        // string in this TU's .data section, but comparing against a string literal is still UB.
#if AVOID_UB
        if (strcmp(mName, "E_OC") == 0) {
#else
        if (mName == "E_OC") {
#endif
            S16_SUB(health, 5);
            if (health < 0) {
                health = 0;
                mSound.startCollisionSE(0x40007,0x20);
                dComIfGp_setHitMark(3, this, &my_vec_0, NULL, NULL, 0);
                setActionMode(E_OC_ACTION_BIG_DAMAGE, 0);
                offTgSph();
                mAtInfo.mHitDirection.y = fopAcM_searchPlayerAngleY(this);
                return;
            }
        } else {
            mSound.startCollisionSE(0x40018,0x2d);
            if (mActionMode == E_OC_ACTION_WAIT
             || mActionMode == E_OC_ACTION_WALK
             || mActionMode == E_OC_ACTION_TALK) {
                setActionMode(E_OC_ACTION_FIND, 1);
            }

            return;
        }
    }

    cc_at_check(this, &mAtInfo);
    if ( ((dCcD_GObjInf*)mAtInfo.mpCollider)->GetAtSpl() == 1
        && (!((dCcD_GObjInf*)mAtInfo.mpCollider)->ChkAtType(AT_TYPE_NORMAL_SWORD) || !daPy_py_c::checkWoodSwordEquip())
        && my_val < 5
    ) {
        my_val = 5;
    }

    if (health <= 1) {
        attention_info.flags = 0;
        if (my_val < 5) {
            my_val = 5;
        }
    }

    if (daPy_getPlayerActorClass()->getCutType() == daPy_py_c::CUT_TYPE_JUMP) {
        if (daPy_getPlayerActorClass()->checkCutJumpCancelTurn()) {
            setActionMode(E_OC_ACTION_DAMAGE, my_val);
            field_0x6cc = 3 + NREG_S(7);
            return;
        }
    }

    if (my_val >= 5) {
        setActionMode(E_OC_ACTION_BIG_DAMAGE, my_val - 5);
        offTgSph();
    } else {
        setActionMode(E_OC_ACTION_DAMAGE, my_val);
    }
}
