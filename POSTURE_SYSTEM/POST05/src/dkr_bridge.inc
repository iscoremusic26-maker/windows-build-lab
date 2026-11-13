// LIGHT FADES TWICE 0.13.8 DKR01: the ONLY coupling of the Darknut module to the rest of the mod. Included once in mod.cpp's anonymous namespace,
// before dkr_adapter.inc. Read-only: the simulation tick, Link (the same live-player getter CP06 FIX01 uses), the shared log writer, and the bundle's
// own res/ file through ResourceService (optional; without it the Darknut stays 100 % native).
static_assert(std::is_same_v<decltype(&daB_TN_c::executeZakoEnding), void (daB_TN_c::*)()> && std::is_same_v<decltype(&daB_TN_c::_delete), int (daB_TN_c::*)()> &&
              std::is_same_v<decltype(&daB_TN_c::setSwordAtBit), void (daB_TN_c::*)(int)>, "DKR01 hook / call signatures");
// Measured in the 2.0.3 EXE: executeZakoEnding `mov ecx,[rcx+0x984]` (mode switch), `cmp dword [rbx+0xcfc],0` (mTimer1), `movzx eax,byte [rbx+0xd34]` (mSwitchNo),
// `cmp byte [rbx+0xd3d],0` (mStayNo2Flag), `mov [rbx+0x650],edi` (attention flags = 0), `mov [rbx+0x620],edi` (speedF), `mov byte [rbx+0xd23],dil` (neck),
// `mov word [rbx+0xcaa],2` (mStates[15]), `lea rcx,[rbx+0x9b8]` + cLib_chaseF (mMaterialColor), `mov rax,[rbx+0x760]` (mpModelMorf2); setDamage
// `mov eax,[rbx+0x980]; cmp eax,8` (mActionMode1), `[rbx+0x98c]` / `[rbx+0x990]` (field_0x6fc / field_0x700), `cmp byte [rbx+0xd2f]` (mCutFlag); action `cmp byte [rsi+0xd35],0` (mType).
static_assert(offsetof(daB_TN_c, mActionMode1) == 0x980 && offsetof(daB_TN_c, mActionMode2) == 0x984 && offsetof(daB_TN_c, field_0x6fc) == 0x98c &&
              offsetof(daB_TN_c, field_0x700) == 0x990 && offsetof(daB_TN_c, mMaterialColor) == 0x9b8 && offsetof(daB_TN_c, mStates) + 15 * sizeof(u16) == 0xcaa &&
              offsetof(daB_TN_c, mTimer1) == 0xcfc && offsetof(daB_TN_c, mUpdateNeckAngle) == 0xd23 && offsetof(daB_TN_c, mSwitchNo) == 0xd34 && offsetof(daB_TN_c, mType) == 0xd35 &&
              offsetof(daB_TN_c, mStayNo2Flag) == 0xd3d && offsetof(daB_TN_c, mCutFlag) == 0xd2f && offsetof(daB_TN_c, mpModelMorf2) == 0x760 && offsetof(fopAc_ac_c, speedF) == 0x620 &&
              offsetof(fopAc_ac_c, attention_info) + offsetof(actor_attention_types, flags) == 0x650, "DKR01 daB_TN_c layout != exe 2.0.3");
static_assert(static_cast<int>(daAlink_c::PROC_CUT_DOWN) == 0x2b && static_cast<int>(daAlink_c::PROC_CUT_DOWN_LAND) == 0x2c && static_cast<int>(daAlink_c::PROC_BACK_JUMP) == 0x12 &&
              static_cast<int>(daAlink_c::PROC_BACK_JUMP_LAND) == 0x13 && static_cast<int>(daB_TN_c::ACT_ENDING) == dkr::kActEnding && static_cast<int>(daB_TN_c::ACT_CHASEL) == dkr::kActChaseL,
              "DKR01 native enums");
std::uint64_t dkrb_tick() noexcept { return g.p8.tick; }
daAlink_c* dkrb_link() noexcept { return f1_link(); }
void dkrb_write(bool warn, const char* line) noexcept { lnk_write(warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line); }
bool dkrb_resource_ready() noexcept { return compatible(svc_resource, 1, 0) && svc_resource->load && svc_resource->free; }
bool dkrb_load(const char* path, ResourceBuffer& out) noexcept { return dkrb_resource_ready() && svc_resource->load(mod_ctx, path, &out) == MOD_OK; }
void dkrb_free(ResourceBuffer& b) noexcept {
  if (dkrb_resource_ready()) svc_resource->free(mod_ctx, &b);
}
