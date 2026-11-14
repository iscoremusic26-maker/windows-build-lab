// LIGHT FADES TWICE 0.13.8 DARKNUT RESURRECTION (DKR01): production adapter of the normal Darknut (daB_TN_c, Enemy Spawner parameters 0x000001FF).
// Included once in mod.cpp's anonymous namespace after dkr_bridge.inc; tests/dkr_world_lab.cpp includes it verbatim against a transcription of the
// native paths. The pure decisions live in dkr_core.hpp.
//
// What it does, and nothing else:
//  * FIRST lethal transition of the second phase (setDamage -> ACT_ENDING, seen as executeZakoEnding's mode 0): the native call runs untouched
//    (BCK_TN2B_DIE, DEAD2 voice, frame 3/33/68 SEs and shakes). Its POST undoes only what would end the body: the darkening (mMaterialColor), the
//    sword drop (mStates[15] 1 -> 2) and the attention reset; the swords' AT is switched off with the native setSwordAtBit(0) (the lying body
//    never hurts). When the clip stops, the native mode 2 (disappear + fopAcM_delete) is replaced by a held mode no native case handles.
//  * Down: native down flag and down position (the Lizalfos' recipe: body joint, 45 up, 30 toward Link), lock-on kept: Link's real Ending Blow
//    is available. The native receipt (onCutDownHitFlg, given by procCutDownLandInit only on a real landing) is the only success signal.
//  * Rise: after Link's whole Ending Blow (path A) or 10 s after the fall ended (path B, frozen while any Ending Blow is in flight), the user's
//    resurrection_A_agil_natural_FIX02.bck (0.13.9; was FIX01) plays ONCE on the body model (native McaMorfSO::setAnm, mode once, no morph: it starts on DIE's last pose).
//  * Back to combat: field_0x6fc (the native damage accumulator) 0, health 100, attention/neck as before the fall, ACT_CHASEL mode 0 and
//    field_0xa91 like the end of the native executeDamageL. The armour is never rebuilt; nothing else of the actor is written.
//  * SECOND lethal transition: nothing of ours. The original executeZakoEnding runs whole: darkening, sword drop and vanish, disappear effect, delete.
//
// The clip: the bundle's res/darknut/resurrection_A_agil_natural_FIX02.bck, loaded once through ResourceService into a buffer this module owns until it is
// unloaded, checked (size, CRC-32, J3D1/bck1/ANK1, loop once, 88 frames, 29 joints, every key table in its pool), then bound ONCE by the native loader
// J3DAnmLoaderDataBase::setResource into a module-static clone of a native B_tn mDoExt_transAnmBas (the host's own vtable; d_resorce.cpp builds every
// archive BCK the same way: transAnmBas + setResource). No game heap, no JKR allocation, nothing global of the Darknut is replaced: one shared clip
// object, like every native resource, each actor with its own frame control.
#if defined(_MSC_VER)
#define DKR_NOINLINE __declspec(noinline)
#else
#define DKR_NOINLINE __attribute__((noinline))
#endif

inline constexpr std::size_t kDkrBasOffset = (sizeof(J3DAnmTransformKey) + alignof(void*) - 1) / alignof(void*) * alignof(void*);
static_assert(sizeof(mDoExt_transAnmBas) == kDkrBasOffset + sizeof(void*), "mDoExt_transAnmBas = J3DAnmTransformKey + mBas");

struct DkrState {
  bool enabled = false, hooks = false, clip_ok = false, clip_bound = false, clip_failed = false;
  dkr::Bck check = dkr::Bck::Size;
  ResourceBuffer buf = RESOURCE_BUFFER_INIT;
  dkr::Table t;
  const J3DAnmTransform* die[dkr::kMaxSlots] = {};
  bool disarmed[dkr::kMaxSlots] = {};
  std::uint64_t lethal = 0, ineligible = 0, taken = 0, native_final = 0, native_other = 0, slot_full = 0, downs = 0, commits = 0, misses = 0, hits = 0;
  std::uint64_t rises_finish = 0, rises_timer = 0, back = 0, anim_lost = 0, watchdogs = 0, deletes = 0, unload_fixed = 0, fall_fix_sword = 0;
  unsigned lines = 0;
  std::uint64_t dropped = 0;
} dz;
alignas(alignof(std::max_align_t)) unsigned char dkr_clip_store[sizeof(mDoExt_transAnmBas)];
J3DAnmTransform* dkr_clip() noexcept { return reinterpret_cast<J3DAnmTransform*>(dkr_clip_store); }

void dkr_line(bool warn, const char* line) noexcept {
  if (dz.lines >= 600) {
    ++dz.dropped;
    return;
  }
  ++dz.lines;
  dkrb_write(warn, line);
}
unsigned dkr_index(const dkr::Ctl* c) noexcept { return static_cast<unsigned>(c - &dz.t.slot[0]); }
dkr::Identity dkr_identity(const daB_TN_c* a) noexcept {
  dkr::Identity i;
  i.param = static_cast<std::uint32_t>(fopAcM_GetParam(a));
  i.type = a->mType, i.type2 = a->mType2, i.switch_no = a->mSwitchNo, i.stay_no2 = a->mStayNo2Flag;
  return i;
}
dkr::LinkView dkr_link_view(const daB_TN_c* a) noexcept {
  dkr::LinkView v;
  daAlink_c* k = dkrb_link();
  if (!k) return v;
  const int p = static_cast<int>(k->mProcID);
  v.finishing = p == daAlink_c::PROC_CUT_DOWN || p == daAlink_c::PROC_CUT_DOWN_LAND;
  v.on_me = v.finishing && k->field_0x280c.getActor() == static_cast<const fopAc_ac_c*>(a);
  v.back_jump = p == daAlink_c::PROC_BACK_JUMP || p == daAlink_c::PROC_BACK_JUMP_LAND;
  return v;
}
// The Lizalfos' down position recipe (d_a_e_dn.cpp L3250-3264): the body joint, 45 up and 30 toward Link. Joint 2 is the body point the native
// ending itself uses for the disappear effect (executeZakoEnding mode 2).
cXyz dkr_down_pos(daB_TN_c* a) noexcept {
  const MtxP m = a->mpModelMorf2->getModel()->getAnmMtx(2);
  cXyz p(m[0][3], m[1][3], m[2][3]);
  if (daAlink_c* k = dkrb_link()) {
    const cXyz to(k->current.pos.x - p.x, 0.0f, k->current.pos.z - p.z);
    const float d = to.abs();
    if (d > 1.0f) p.x += to.x / d * 30.0f, p.z += to.z / d * 30.0f;
  }
  p.y += 45.0f;
  return p;
}

// ---------------------------------------------------------------- writers (the only native writes of this module)
// 1/7: the first fall keeps the body: no darkening, the sword stays in the hand (only the frame-3 drop is undone).
DKR_NOINLINE void dkr_native_keep_body(daB_TN_c* a, int sword, float color) noexcept {
  if (a->mStates[15] != static_cast<u16>(sword)) a->mStates[15] = static_cast<u16>(sword), ++dz.fall_fix_sword;
  a->mMaterialColor = color;
}
// 2/7: once, right after the native mode 0: the native sword AT off (setSwordAtBit(0), as executeDamageL does) and the shield AT off, lock-on kept.
DKR_NOINLINE void dkr_native_disarm(daB_TN_c* a, std::uint32_t attn) noexcept {
  a->setSwordAtBit(0);
  a->mSphC.OffAtSetBit();
  a->attention_info.flags = attn;
}
// 3/7: the fall clip stopped: hold the body (no native case runs), down flag, down position (refreshed every held tick until the receipt).
DKR_NOINLINE void dkr_native_hold(daB_TN_c* a, const cXyz& pos) noexcept {
  a->mActionMode2 = dkr::kHeldMode;
  a->setDownPos(&pos);
  a->onDownFlg();
}
// 4/7: the native receipt was read: no second Ending Blow on this body (down and receipt flags off).
DKR_NOINLINE void dkr_native_receipt(daB_TN_c* a) noexcept { a->offDownFlg(); }
// 5/7: the resurrection clip, once, on the body model.
DKR_NOINLINE void dkr_native_rise(daB_TN_c* a, J3DAnmTransform* clip) noexcept {
  a->offDownFlg();
  a->speedF = 0.0f;
  a->mpModelMorf2->setAnm(clip, 0, 0.0f, 1.0f, 0.0f, -1.0f);
}
// 6/7: back to the native second phase with 100 % resistance (the same exit as executeDamageL: ACT_CHASEL mode 0, field_0xa91 true).
DKR_NOINLINE void dkr_native_combat(daB_TN_c* a, std::uint32_t attn, bool neck) noexcept {
  a->field_0x6fc = 0;
  a->health = 100;
  a->attention_info.flags = attn;
  a->mUpdateNeckAngle = neck;
  a->offHeadLockFlg();
  a->mActionMode1 = dkr::kActChaseL;
  a->mActionMode2 = 0;
  a->field_0xa91 = true;
}
// 7/7: the module unloads while a body is held or rising: the native clip back on the model (our clip dies with the module) and the native
// vanish mode: a controlled native death, never an immortal or frozen actor.
DKR_NOINLINE void dkr_native_unload(daB_TN_c* a, const J3DAnmTransform* die) noexcept {
  a->offDownFlg();
  if (a->mpModelMorf2->getAnm() == dkr_clip() && die) {
    a->mpModelMorf2->setAnm(const_cast<J3DAnmTransform*>(die), 0, 0.0f, 1.0f, 0.0f, -1.0f);
    a->mpModelMorf2->setFrame(a->mpModelMorf2->getEndFrame());
  }
  a->mActionMode2 = dkr::kModeVanish;
  a->mTimer1 = 10;
}
// The clip object: a byte copy of a native B_tn mDoExt_transAnmBas (same dynamic type and host vtable), its constructor-only fields reset as the
// native constructor leaves them, then the native loader binds it to our bytes (and byte-swaps the key pools in place: exactly once).
DKR_NOINLINE bool dkr_bind_clip(const J3DAnmTransform* native) noexcept {
  if (!native || native->getKind() != 8 || !dz.buf.data) return false;
  std::memcpy(dkr_clip_store, static_cast<const void*>(native), sizeof(mDoExt_transAnmBas));
  J3DAnmTransform* c = dkr_clip();
  c->field_0x5 = 0, c->field_0x18 = 0, c->field_0x1a = 0, c->field_0x1c = 0;
  void* const no_bas = nullptr;
  std::memcpy(dkr_clip_store + kDkrBasOffset, &no_bas, sizeof(no_bas));
  J3DAnmLoaderDataBase::setResource(c, dz.buf.data);
  return c->getKind() == 8 && c->getFrameMax() == static_cast<s16>(dkr::kBckFrames) && c->getAttribute() == 0 && c->field_0x1e == dkr::kBckJoints;
}
bool dkr_clip_ready(const daB_TN_c* a) noexcept {
  if (dz.clip_bound) return true;
  if (!dz.clip_ok || dz.clip_failed) return false;
  dz.clip_bound = dkr_bind_clip(a->mpModelMorf2->getAnm());
  dz.clip_failed = !dz.clip_bound;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "DKR01 event=CLIP_BIND result=%s path=res/%s bytes=%u frames=%u joints=%u loop=ONCE loader=J3DAnmLoaderDataBase::setResource object=MODULE_STATIC",
                dz.clip_bound ? "OK" : "FAILED_DARKNUTS_STAY_NATIVE", dkr::kBckPath, dkr::kBckBytes, dkr::kBckFrames, dkr::kBckJoints);
  dkr_line(!dz.clip_bound, line);
  return dz.clip_bound;
}
void dkr_log(const char* ev, const dkr::Ctl& c, const daB_TN_c* a, bool warn = false) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "DKR01 event=%s actor=%u generation=%llu phase=%s path=%s revivals=%u/%u timer=%u down_ticks=%u commits=%u misses=%u hit=%d act=%d mode=%d acc=%d/%d hp=%d tick=%llu", ev, c.id,
                static_cast<unsigned long long>(c.generation), dkr::name(c.phase), dkr::name(c.path), c.revivals, dkr::kMaxRevivals, c.timer, c.down_ticks, c.commits, c.misses, c.hit ? 1 : 0,
                a ? a->mActionMode1 : -1, a ? a->mActionMode2 : -1, a ? a->field_0x6fc : -1, a ? a->field_0x700 : -1, a ? static_cast<int>(a->health) : -1, static_cast<unsigned long long>(dkrb_tick()));
  dkr_line(warn, line);
}

// ---------------------------------------------------------------- hooks
HookAction on_dkr_ending_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dz.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a || a->mActionMode2 != dkr::kModeStart) return HOOK_CONTINUE;
  ++dz.lethal;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  dkr::Ctl* c = dz.t.find(id);
  if (!c) {
    if (!dkr::eligible(dkr_identity(a))) {
      ++dz.ineligible;
      return HOOK_CONTINUE;  // never a slot: Temple of Time, Darknut (Sword), placed variants
    }
    c = dz.t.take(id);
    if (!c) {
      ++dz.slot_full;
      dkr_line(true, "DKR01 event=SLOT_FULL result=NATIVE_DEATH");
      return HOOK_CONTINUE;
    }
  }
  const bool usable = dkr_clip_ready(a);
  const dkr::Step s = c->on_lethal(usable);
  if (s == dkr::Step::TakeFall) {
    ++dz.taken;
    c->saved_sword = a->mStates[15], c->saved_color = a->mMaterialColor, c->saved_attn = a->attention_info.flags, c->saved_neck = a->mUpdateNeckAngle;
    dz.die[dkr_index(c)] = nullptr, dz.disarmed[dkr_index(c)] = false;
    dkr_log("FIRST_DEFEAT_NATIVE_FALL", *c, a);
  } else if (s == dkr::Step::NativeDeath) {
    if (c->phase == dkr::Phase::Consumed) ++dz.native_final, dkr_log("SECOND_DEFEAT_NATIVE_DEATH", *c, a);
    else ++dz.native_other, dkr_log("NATIVE_DEATH_NO_CLIP", *c, a, true);
  }
  return HOOK_CONTINUE;
}
void on_dkr_ending_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dz.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  dkr::Ctl* c = dz.t.find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!c) return;
  const unsigned i = dkr_index(c);
  switch (c->phase) {
    case dkr::Phase::Falling: {
      if (!dz.disarmed[i]) {
        dz.die[i] = a->mpModelMorf2->getAnm();  // the native setBck(BCK_TN2B_DIE) of this very call
        dkr_native_disarm(a, c->saved_attn);
        dz.disarmed[i] = true;
      }
      const dkr::Step s = c->after_fall(a->mActionMode2);
      dkr_native_keep_body(a, c->saved_sword, c->saved_color);
      if (s == dkr::Step::EnterDown) {
        dkr_native_hold(a, dkr_down_pos(a));
        ++dz.downs;
        dkr_log("DOWN_FINISH_AVAILABLE", *c, a);
      }
      return;
    }
    case dkr::Phase::Down:
    case dkr::Phase::Committed:
    case dkr::Phase::FinishHit: {
      const dkr::Phase before = c->phase;
      const bool receipt = a->checkCutDownHitFlg() != 0;
      const dkr::Step s = c->held(dkr_link_view(a), receipt);
      if (before != dkr::Phase::FinishHit && c->hit) {
        dkr_native_receipt(a);
        ++dz.hits;
        dkr_log("FINISH_HIT_NATIVE_RECEIPT", *c, a);
      }
      if (before != dkr::Phase::Committed && c->phase == dkr::Phase::Committed) ++dz.commits, dkr_log("FINISH_COMMITTED", *c, a);
      if (before == dkr::Phase::Committed && c->phase != dkr::Phase::Committed && !c->hit) ++dz.misses, dkr_log("FINISH_MISSED_TIMER_RESUMES", *c, a);
      if (s == dkr::Step::Rise) {
        if (c->watchdog) ++dz.watchdogs;
        dkr_native_rise(a, dkr_clip());
        if (c->path == dkr::Path::Finish) ++dz.rises_finish;
        else ++dz.rises_timer;
        dkr_log("RESURRECTION_START", *c, a, c->watchdog);
      } else if (s == dkr::Step::HoldDown && c->phase != dkr::Phase::FinishHit) {
        dkr_native_hold(a, dkr_down_pos(a));  // the down position follows the body; mode and flag unchanged (idempotent)
      }
      return;
    }
    case dkr::Phase::Rising: {
      const bool ours = a->mpModelMorf2->getAnm() == dkr_clip();
      const dkr::Step s = c->rising(ours, a->mpModelMorf2->isStop() != 0);
      if (s == dkr::Step::BackToCombat) {
        if (!ours) ++dz.anim_lost;
        dkr_native_combat(a, c->saved_attn, c->saved_neck);
        ++dz.back;
        dkr_log(ours ? "RESURRECTED_SECOND_PHASE_100PCT" : "RESURRECTED_CLIP_LOST", *c, a, !ours);
      }
      return;
    }
    default:
      return;
  }
}
HookAction on_dkr_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  if (dkr::Ctl* c = dz.t.find(id)) {
    dkr_log("ACTOR_DELETED_SLOT_RETIRED", *c, a);
    const unsigned i = dkr_index(c);
    dz.die[i] = nullptr, dz.disarmed[i] = false;
    dz.t.retire(id);
    ++dz.deletes;
  }
  return HOOK_CONTINUE;
}

void dkr_summary(const char* why) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "DKR01_SUMMARY why=%s version=%s enabled=%d clip=%s bound=%d lethal=%llu ineligible=%llu first_falls=%llu downs=%llu commits=%llu misses=%llu finish_hits=%llu rises_finish=%llu "
                "rises_timer=%llu back_to_combat=%llu second_native_deaths=%llu other_native=%llu slot_full=%llu sword_drops_undone=%llu anim_lost=%llu watchdogs=%llu deletes=%llu unload_fixed=%llu live=%u dropped_lines=%llu",
                why, dkr::kVersion, dz.enabled ? 1 : 0, dkr::name(dz.check), dz.clip_bound ? 1 : 0, static_cast<unsigned long long>(dz.lethal), static_cast<unsigned long long>(dz.ineligible),
                static_cast<unsigned long long>(dz.taken), static_cast<unsigned long long>(dz.downs), static_cast<unsigned long long>(dz.commits), static_cast<unsigned long long>(dz.misses),
                static_cast<unsigned long long>(dz.hits), static_cast<unsigned long long>(dz.rises_finish), static_cast<unsigned long long>(dz.rises_timer), static_cast<unsigned long long>(dz.back),
                static_cast<unsigned long long>(dz.native_final), static_cast<unsigned long long>(dz.native_other), static_cast<unsigned long long>(dz.slot_full),
                static_cast<unsigned long long>(dz.fall_fix_sword), static_cast<unsigned long long>(dz.anim_lost), static_cast<unsigned long long>(dz.watchdogs),
                static_cast<unsigned long long>(dz.deletes), static_cast<unsigned long long>(dz.unload_fixed), dz.t.live(), static_cast<unsigned long long>(dz.dropped));
  dkrb_write(false, line);
}
// Unload: the host removes our hooks before calling mod_shutdown. A body we hold (or one playing our clip) is handed to the native vanish; the clip
// buffer is released only after no live model can reference it. Bodies are found again by process id (never by a kept pointer).
void dkr_shutdown() noexcept {
  const bool was = dz.enabled;
  dz.enabled = false;
  for (unsigned i = 0; i < dkr::kMaxSlots; ++i) {
    if (!dz.t.on[i]) continue;
    const dkr::Ctl& c = dz.t.slot[i];
    if (c.phase != dkr::Phase::Down && c.phase != dkr::Phase::Committed && c.phase != dkr::Phase::FinishHit && c.phase != dkr::Phase::Rising) continue;
    fopAc_ac_c* p = nullptr;
    if (fopAcM_SearchByID(c.id, &p) && p && fopAcM_GetName(p) == fpcNm_B_TN_e) {
      dkr_native_unload(static_cast<daB_TN_c*>(p), dz.die[i]);
      ++dz.unload_fixed;
    }
  }
  if (was) dkr_summary("SHUTDOWN");
  if (dz.buf.data) dkrb_free(dz.buf);
  dz.clip_bound = false;
}
void dkr_install() noexcept {
  ResourceBuffer b = RESOURCE_BUFFER_INIT;
  const bool loaded = dkrb_load(dkr::kBckPath, b);
  dz.check = loaded ? dkr::check_bck(static_cast<const unsigned char*>(b.data), b.size) : dkr::Bck::Size;
  dz.clip_ok = loaded && dz.check == dkr::Bck::Ok;
  if (loaded && !dz.clip_ok) dkrb_free(b);
  if (dz.clip_ok) dz.buf = b;
  ModResult r[3] = {MOD_ERROR, MOD_ERROR, MOD_ERROR};
  if (dz.clip_ok) r[0] = mods::hook::add_pre<DkrEnding>(on_dkr_ending_pre), r[1] = mods::hook::add_post<DkrEnding>(on_dkr_ending_post), r[2] = mods::hook::add_pre<DkrDelete>(on_dkr_delete_pre);
  dz.hooks = r[0] == MOD_OK && r[1] == MOD_OK && r[2] == MOD_OK;
  if (!dz.hooks) {
    (void)mods::hook::uninstall<DkrEnding>(), (void)mods::hook::uninstall<DkrDelete>();
    if (dz.buf.data) dkrb_free(dz.buf);
    dz.clip_ok = false;
  }
  dz.enabled = dz.hooks;  // all or nothing: without the clip or a hook every Darknut is 100 % native and the rest of the mod is untouched
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "DKR01 event=INSTALL version=%s tag=%s enabled=%d resource=%s check=%s bytes=%u crc32=%08x hooks=%d/%d/%d target=B_TN_param_%08x(mType1,mType2_0,switch_ff) revive_delay_ticks=%u(10s) "
                "max_revivals=%u held_mode=0x%x finish=NATIVE_onCutDownHitFlg_receipt rise=McaMorfSO::setAnm(once,morf0) back=ACT_CHASEL/0 second_defeat=NATIVE_executeZakoEnding",
                dkr::kVersion, dkr::kTag, dz.enabled ? 1 : 0, loaded ? "LOADED" : (dkrb_resource_ready() ? "MISSING" : "NO_SERVICE"), dkr::name(dz.check), dkr::kBckBytes,
                static_cast<unsigned>(dkr::kBckCrc32Value), static_cast<int>(r[0]), static_cast<int>(r[1]), static_cast<int>(r[2]), static_cast<unsigned>(dkr::kTargetParam), dkr::kReviveDelayTicks,
                dkr::kMaxRevivals, static_cast<unsigned>(dkr::kHeldMode));
  dkrb_write(!dz.enabled, line);
}
#undef DKR_NOINLINE
