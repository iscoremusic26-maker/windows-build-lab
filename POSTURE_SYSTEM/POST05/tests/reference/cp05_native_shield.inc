bool dCcS::ChkShieldFrontRange(cCcD_Obj* i_atObj, cCcD_Obj* i_tgObj, int param_2,
                               cXyz const* i_hitPos) {
    dCcD_GObjInf* atObj = (dCcD_GObjInf*)i_atObj->GetGObjInf();
    if (atObj == NULL) {
        return false;
    }

    dCcD_GObjInf* tgObj = (dCcD_GObjInf*)i_tgObj->GetGObjInf();
    if (tgObj == NULL) {
        return false;
    }

    fopAc_ac_c* tgActor = tgObj->GetAc();
    if (tgActor == NULL) {
        return false;
    }

    csXyz sp10;
    CalcParticleAngle(atObj, i_atObj->GetStts(), i_tgObj->GetStts(), &sp10);

    if (param_2 != 0) {
        dJntCol_c* jntcol = fopAcM_GetJntCol(tgActor);

        if (jntcol != NULL) {
            cXyz pos;
            csXyz angle;

            int temp_r3 = jntcol->getHitmarkPosAndAngle(i_hitPos, &sp10, &pos, &angle, 0);

            if (temp_r3 >= 0 && jntcol->checkShieldType(temp_r3)) {
                cXyz sp18;
                mDoMtx_stack_c::ZXYrotS(sp10);
                mDoMtx_stack_c::multVecSR(&cXyz::BaseZ, &sp18);
                mDoMtx_stack_c::ZXYrotS(angle);
                mDoMtx_stack_c::multVecSR(&cXyz::BaseZ, &pos);

                if (pos.inprod(sp18) > 0.0f) {
                    return true;
                } else {
                    return false;
                }
            }

            return false;
        }
    }

    s16 var_r4;
    if (tgObj->GetTgShieldFrontRangeYAngle() != NULL) {
        var_r4 = *tgObj->GetTgShieldFrontRangeYAngle();
    } else {
        var_r4 = fopAcM_GetShapeAngle_p(tgActor)->y;
    }

    s16 shield_range = tgObj->GetTgShieldRange();
    if (param_2 != 0) {
        shield_range >>= 1;
    }

    if (cLib_distanceAngleS(sp10.y - -0x8000, var_r4) > shield_range) {
        return false;
    }

    return true;
}

bool dCcS::ChkShield(cCcD_Obj* i_atObj, cCcD_Obj* i_tgObj, dCcD_GObjInf* i_atObjInf,
                     dCcD_GObjInf* i_tgObjInf, cXyz const* i_hitPos) {
    if (i_atObjInf->ChkAtNoGuard()) {
        return false;
    }

    if (i_tgObjInf->ChkTgShield() || i_tgObjInf->ChkTgSpShield() ||
        (i_tgObjInf->ChkTgSmallShield() && i_atObjInf->GetAtSpl() == 8))
    {
        if (i_tgObjInf->ChkTgShieldFrontRange()) {
            return ChkShieldFrontRange(i_atObj, i_tgObj, !i_tgObjInf->ChkTgShield(), i_hitPos);
        }

        return true;
    }

    return false;
}
