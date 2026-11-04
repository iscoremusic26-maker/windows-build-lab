void daE_OC_c::setActionMode(int i_action, int i_state) {
    mOldActionMode = mActionMode;
    mActionMode = i_action;
    mOcState = i_state;
    field_0x6c0 = field_0x6c2 = 0;
    mSphs_cc[0].OnCoSetBit();
    mSphs_cc[0].OnTgSetBit();
    mSphs_cc[1].OnCoSetBit();
    mSphs_cc[1].OnTgSetBit();
    offHeadLockFlg();
}
