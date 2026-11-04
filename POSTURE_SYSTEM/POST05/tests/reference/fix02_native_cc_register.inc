void cCcS::Set(cCcD_Obj* obj) {
    if (obj->ChkAtSet()) {
        if (mObjAtCount >= ARRAY_SIZEU(mpObjAt)) {
            OS_REPORT("\x1b[43;30m");
            OS_REPORT("cCcS::Set AT Overflow.Now Max is %d.\n", ARRAY_SIZEU(mpObjAt));
            OS_REPORT("\x1b[m");
        } else {
            mpObjAt[mObjAtCount] = obj;
            mObjAtCount++;
        }
    }

    if (obj->ChkTgSet()) {
        if (mObjTgCount >= ARRAY_SIZEU(mpObjTg)) {
            OS_REPORT("\x1b[43;30m");
            OS_REPORT("cCcS::Set TG Overflow.Now Max is %d.\n", ARRAY_SIZEU(mpObjTg));
            OS_REPORT("\x1b[m");
        } else {
            mpObjTg[mObjTgCount] = obj;
            mObjTgCount++;
        }
    }

    if (obj->ChkCoSet()) {
        if (mObjCoCount >= ARRAY_SIZEU(mpObjCo)) {
            OS_REPORT("\x1b[43;30m");
            OS_REPORT("cCcS::Set CO Overflow.Now Max is %d.\n", ARRAY_SIZEU(mpObjCo));
            OS_REPORT("\x1b[m");
        } else {
            mpObjCo[mObjCoCount] = obj;
            mObjCoCount++;
        }
    }

    if (mObjCount >= ARRAY_SIZEU(mpObj)) {
        OS_REPORT("\x1b[43;30m");
        OS_REPORT("cCcS::Set SET Overflow.Now Max is %d.\n", ARRAY_SIZEU(mpObj));
        OS_REPORT("\x1b[m");
    } else {
        mpObj[mObjCount] = obj;
        mObjCount++;
    }
}
