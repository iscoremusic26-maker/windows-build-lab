void cCcS::ClrAtHitInf() {
    for (cCcD_Obj** obj = mpObjAt; obj < mpObjAt + mObjAtCount; ++obj) {
        if (*obj != NULL) {
            (*obj)->GetGObjInf()->ClrAtHit();

            if ((*obj)->GetStts() != NULL) {
                (*obj)->GetStts()->ClrAt();
            }
        }
    }
}
