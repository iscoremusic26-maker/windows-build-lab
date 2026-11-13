// LIZALFOS DUELIST 0.12.2 production adapter. Included once inside mod.cpp's anonymous namespace after the CP06 FIX01 adapter.
// Every decision lives in liz_core.hpp (liz::Brain); this file only reads native fields by value, calls the Brain from the native hook and applies Write.
// E_OC, Link's own hooks and every existing writer are not touched. Native writes live in the LIZ_NOINLINE functions below (hash-audited by ci/liz_audit.py):
//   liz_native_fields     action/mode/timer[2]/invulnerability/current.angle.y/current.pos of THIS e_dn_class, inside its own Execute, before CrrPos
//   liz_native_anim       morf frame field and play speed of THIS actor's own morf (mode-0 start of a combo action, guard native exit)
//   liz_native_hp         health + max health, once per admitted identity, only from the exact native 100/100 of daE_DN_Create
//   liz_native_guard      the exact field set of the native guard entry in e_dn_fight_run
//   liz_native_break      at_info.mHitDirection + the ORIGINAL big_damage() + the invulnerability its native caller writes
//   liz_native_tg_clear   the inline Tg-hit clear of a cc_sph whose Tg mark is Link's Shield Attack (native DRAWBACK below posture 100)
//   liz_native_bash_se    the native collision SE of that real Shield Attack contact (the cc_at_check that plays it no longer runs for it)
//   liz_native_stagger    ACTION_DRAWBACK mode 0 + its native recoil fields: the standing stagger at posture 100
//   liz_native_hold       timer[0] of the native S_DAMAGE wait + HeadLock off while no real new Shield Attack prepared Helm Splitter
//   liz_native_pose_frame morf frame field + the engine's own modelCalc (pose probe, verified restore)
//   liz_native_at_place   at_sph centre onto an exact skipped native pose that reaches an ON Tg cylinder of Link, and/or its velocity zeroed while Link's shield is up (the engine's StartCAt state)
//   liz_native_hp_bonus   health of THIS admitted actor: +50% of the real loss of its confirmed decisive hit, once, before the native fall reads it
//   liz_native_learn      the sword bits of THIS actor's learn field: vanilla's permanent "always guard this cut" memory, decided by the brain instead
//   liz_native_link_feedback  Link's native setGuardSe + the native shock + the native slip speed, ONLY for a real shield contact of an admitted Lizalfos that native GUARD_SLIP gating dropped
#if defined(_MSC_VER)
#define LIZ_NOINLINE __declspec(noinline)
#else
#define LIZ_NOINLINE __attribute__((noinline))
#endif
struct LizSweep {
  bool armed = false, covered = false, have = false;
  std::uint64_t local = 0, impact = 0;
  float frame = 0;
  cp06f1::V3 center{};
};
struct LizSlot {
  bool on = false;
  std::uintptr_t key = 0;
  std::uint32_t id = 0;
  LizSweep sw{};
  liz::Brain b;
  // FIX06 P0: the Jump Strike contact of this damage_check (HP raised by a headroom so the native never takes its lethal branch for it)
  bool js_raised = false, js_credit_now = false;
  int js_hp_before = 0, js_atp = 0;
  kk::f6::JsCredit js_credit;
  // FIX07 P1: the bounded special skill of this damage_check (spin / great spin / native Mortal Draw): raised headroom, one credit per execution
  bool sk_raised = false, sk_credit_now = false;
  int sk_hp_before = 0, sk_atp = 0;
  kk::f7::Sk sk_kind = kk::f7::Sk::None;
  kk::f7::Credit sk_credit;
};
struct LizRuntime {
  bool enabled = false;
  LizSlot slots[liz::kSlots]{};
  std::uint64_t generations = 0, swing_serial = 0, swing_tick = ~0ull, overflow = 0, deleted = 0, excluded = 0, admitted = 0;
  std::uint32_t stage = 0;
  std::uint32_t d1_parry_id = 0;
  std::uint64_t double_gap_tick = ~0ull, double_rearms = 0, link_gain = 0, link_saturations = 0, break_retry_until = 0, break_retries = 0;
  std::uint32_t retry_attacker = 0;
  std::uint64_t retry_generation = 0, hud_missing = 0, hud_shown = 0, hud_slot_busy = 0;
  bool swing_twirl = false;  // FIX06 P4: the last swing was a Back Slice
  bool swing_jump = false, coverage = false, slip_pre = false;
  bool jump_open = false;  // LFT 0.13.5: the last swing was a Jump Attack takeoff
  std::uint64_t jump_serial = 0, jump_land_merged = 0;
  std::uint64_t promotions = 0;
  std::uint64_t pressure_promotions(const liz::Stats&) const noexcept { return promotions; }
  std::uint64_t slip_pre_tick = ~0ull, se_native = 0, se_restored = 0, se_ignored_foreign = 0, gap_arms = 0, zero_vec = 0, probes = 0, probe_mismatch = 0, place_failed = 0, foreign_sphere = 0, bash_se = 0, no_receipt = 0;
  std::uint64_t why[unsigned(liz::sweep::Why::Count)] = {};
  float snap[kF1Joints][3][4];
  lnks1fix::DirectJoin direct;
  liz::Stats totals{};
  std::uint64_t rolls[3] = {}, attempts[3] = {};
  unsigned lines[17] = {};
  std::uint64_t dropped[17] = {};
  struct CutAgg { std::uint64_t swings = 0, contacts = 0, parries = 0; } cuts[64];  // FIX05: by native CUT_TYPE, the only honest answer to "which swing of the combo was parried"
  std::uint64_t rebound_state_ok = 0, rebound_state_other = 0;
} lz;
enum LizLog : unsigned { kLogAdmit = 0, kLogCombo, kLogSwing, kLogDefense, kLogParry, kLogTurn, kLogBreak, kLogPosture, kLogCritical, kLogLink, kLogImpact, kLogFeedback, kLogGuard, kLogDeflect, kLogTrace, kLogCut, kLogResponse };
inline constexpr unsigned kLizCaps[17] = {64, 240, 400, 240, 500, 240, 240, 200, 80, 160, 700, 240, 800, 900, 1200, 1500, 1200};
void liz_line(LizLog fam, LogLevel lvl, const char* line) noexcept {
  if (lnk_take(lz.lines[fam], lz.dropped[fam], kLizCaps[fam])) lnk_write(lvl, line);
}
bool liz_alive() noexcept { return lz.enabled && !g.eng.obs.disabled; }
LizSlot* liz_find(const e_dn_class* a) noexcept {
  const std::uintptr_t k = reinterpret_cast<std::uintptr_t>(a);
  for (auto& s : lz.slots)
    if (s.on && s.key == k) return &s;
  return nullptr;
}
void liz_summary() noexcept;
void liz_merge(liz::Stats& t, std::uint64_t* rolls, std::uint64_t* atts, const liz::Brain& b) noexcept {
  const auto& x = b.st;
  for (unsigned i = 0; i < liz::kPatterns; ++i) t.selected[i] += x.selected[i], t.completed[i] += x.completed[i];
  for (unsigned i = 0; i < liz::kPatternSlots; ++i) t.impacts_open[i] += x.impacts_open[i];
  for (unsigned i = 0; i < unsigned(liz::End::Count); ++i) t.ended[i] += x.ended[i];
  for (unsigned i = 0; i < unsigned(liz::Hit::Count); ++i) t.contact[i] += x.contact[i];
  t.link_parry += x.link_parry, t.d_pair_parry += x.d_pair_parry, t.sword_hits += x.sword_hits, t.iron_hits += x.iron_hits, t.blocks += x.blocks, t.enemy_parries += x.enemy_parries;
  t.cancels_suppressed += x.cancels_suppressed, t.stagger_suppressed_small += x.stagger_suppressed_small, t.stagger_suppressed_big += x.stagger_suppressed_big, t.shield_bash_cleared += x.shield_bash_cleared;
  t.guards_native += x.guards_native, t.guards_attempt += x.guards_attempt, t.guards_boundary += x.guards_boundary, t.watchdog += x.watchdog, t.sample_gap += x.sample_gap, t.windows += x.windows;
  t.swings_detected += x.swings_detected, t.block_pairs += x.block_pairs, t.deflect_rolls += x.deflect_rolls, t.deflect_armed += x.deflect_armed, t.deflect_windows += x.deflect_windows, t.deflect_expired += x.deflect_expired, t.guard_enters += x.guard_enters, t.guard_intent_lost += x.guard_intent_lost, t.wounds += x.wounds, t.sword_duplicates += x.sword_duplicates, t.shield_won_body += x.shield_won_body, t.shield_not_covering += x.shield_not_covering, t.guard_cover_ticks += x.guard_cover_ticks, t.guard_open_ticks += x.guard_open_ticks, t.guard_turn_ticks += x.guard_turn_ticks, t.block_rear_rejected += x.block_rear_rejected, t.rebound_kept += x.rebound_kept, t.rebound_suppressed += x.rebound_suppressed, t.break_bonus_hits += x.break_bonus_hits, t.break_bonus_units += x.break_bonus_units, t.break_bonus_lethal += x.break_bonus_lethal, t.counter_at_open += x.counter_at_open, t.counter_contact += x.counter_contact, t.counter_expired += x.counter_expired;
  for (unsigned i = 0; i < unsigned(liz::Tv::Count); ++i) t.trace_verdict[i] += x.trace_verdict[i];
  for (unsigned i = 0; i < 2; ++i) t.counter_selected[i] += x.counter_selected[i], t.counter_started[i] += x.counter_started[i];
  for (unsigned i = 0; i < 3; ++i) t.guard_rolls[i] += x.guard_rolls[i], t.guard_intents[i] += x.guard_intents[i];
  for (unsigned i = 0; i < unsigned(liz::Cancel::Count); ++i) t.counter_cancel[i] += x.counter_cancel[i];
  t.break_standing += x.break_standing, t.helm_prepared += x.helm_prepared, t.decisive_sword += x.decisive_sword, t.decisive_helm += x.decisive_helm, t.punish_end += x.punish_end, t.down_finish += x.down_finish, t.getup += x.getup;
  for (unsigned p = 0; p < liz::kPatterns; ++p)
    for (unsigned i = 0; i < 9; ++i) t.link_parry_by_impact[p][i] += x.link_parry_by_impact[p][i];
  for (unsigned i = 0; i < unsigned(liz::Cls::Count); ++i) t.impact_class[i] += x.impact_class[i];
  t.expect_ticks += x.expect_ticks, t.candidate_ticks += x.candidate_ticks, t.nocon_ticks += x.nocon_ticks, t.lost_ticks += x.lost_ticks, t.off_touch_ticks += x.off_touch_ticks, t.gap_fixed += x.gap_fixed;
  for (unsigned i = 0; i < 3; ++i) rolls[i] += b.def.rolls[i], atts[i] += b.def.attempts[i];
  t.f6.add(x.f6);  // FIX06
  t.f7.add(x.f7);  // FIX07
}
void liz_retire(LizSlot& s) noexcept {
  if (s.on && s.b.admitted) liz_merge(lz.totals, lz.rolls, lz.attempts, s.b), lz.promotions += s.b.def.promotions;
  s.on = false;
}
// ---------------------------------------------------------------- writers
LIZ_NOINLINE void liz_native_fields(e_dn_class* a, const liz::Write& w) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  if (w.set_action) a->action = w.action;
  if (w.set_mode) a->mode = w.mode;
  if (w.set_invuln) a->invulnerability_timer = w.invuln;
  if (w.set_timer2) a->timer[2] = w.timer2;
  if (w.set_cur_y) ac->current.angle.y = w.cur_y;
  if (w.fwd != 0.0f) {
    float sn = 0, cs = 1;
    cp06f1::sincos16(w.fwd_angle, sn, cs);
    ac->current.pos.x += sn * w.fwd;
    ac->current.pos.z += cs * w.fwd;
  }
}
LIZ_NOINLINE void liz_native_anim(mDoExt_McaMorfSO* m, bool set_frame, float frame, bool set_rate, float rate) noexcept {
  if (set_frame) m->mFrameCtrl.mFrame = frame;
  if (set_rate) m->setPlaySpeed(rate);
}
LIZ_NOINLINE void liz_native_hp(e_dn_class* a) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  ac->field_0x560 = liz::policy::kMaxHp;
  ac->health = liz::policy::kMaxHp;
}
LIZ_NOINLINE void liz_native_guard(e_dn_class* a) noexcept {  // e_dn_fight_run: field_0x708=30, field_0x70c=shape_angle.y, timer[0]=0, invulnerability_timer=15
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  a->field_0x708 = 30.0f;
  a->field_0x70c = ac->shape_angle.y;
  a->timer[0] = 0;
  a->invulnerability_timer = 15;
}
LIZ_NOINLINE void liz_native_break(e_dn_class* a) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  a->at_info.mHitDirection.y = a->search_angle_y;  // cc_at_check's value for a hit from Link (enemy->attacker direction)
  LizBig::g_orig(a);
  a->invulnerability_timer = 1000;
  ac->speedF = 0.0f;
}
LIZ_NOINLINE cCcD_Obj* liz_native_tg_clear(e_dn_class* a, std::uint32_t at_mask) noexcept {
  cCcD_Obj* first = nullptr;
  for (int i = 0; i < 3; ++i) {
    auto& s = a->cc_sph[i];
    if (!s.cCcD_ObjHitInf::ChkTgHit()) continue;
    cCcD_Obj* o = s.cCcD_ObjHitInf::GetTgHitObj();
    if (o && o->ChkAtType(at_mask)) {
      s.cCcD_ObjHitInf::ClrTgHit();
      if (!first) first = o;
    }
  }
  return first;
}
LIZ_NOINLINE void liz_native_bash_se(e_dn_class* a, cCcD_Obj* o) noexcept {
  def_se_set(&a->sound, o, 30, static_cast<fopAc_ac_c*>(&a->actor));  // 30 = cc_at_check's getMapInfo(mHitStatus 0)
}
LIZ_NOINLINE void liz_native_stagger(e_dn_class* a) noexcept {  // damage_check's mHitType 16 branch, with search_angle_y as the direction of a hit from Link (cc_at_check)
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  a->action = liz::native::kDrawback;
  a->mode = 0;
  if (a->snap_angle_y_flag != 0) ac->current.angle.y = ac->shape_angle.y;
  a->field_0x750 = static_cast<s16>((ac->shape_angle.y - 0x8000) - a->search_angle_y);
  a->field_0x74c = 100.0f;
}
LIZ_NOINLINE void liz_native_hold(e_dn_class* a, bool set_timer, std::int16_t timer0, bool head_off, bool ut2_zero) noexcept {
  if (set_timer) a->timer[0] = timer0;
  if (head_off) a->actor.offHeadLockFlg();
  if (ut2_zero) a->unk_timer_2 = 0;
}
LIZ_NOINLINE void liz_native_hp_bonus(e_dn_class* a, std::int16_t extra) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  ac->health = static_cast<s16>(ac->health > extra ? ac->health - extra : 0);
}
LIZ_NOINLINE void liz_native_learn(e_dn_class* a) noexcept {
  a->learn &= ~liz::policy::kLearnSwordMask;
}
LIZ_NOINLINE void liz_native_pose_frame(mDoExt_McaMorfSO* m, float frame) noexcept {
  m->mFrameCtrl.mFrame = frame;
  m->modelCalc();
}
LIZ_NOINLINE void liz_native_at_place(e_dn_class* a, bool place, float x, float y, float z, bool zero_vec) noexcept {
  if (place) a->at_sph.SetC(cXyz(x, y, z));
  if (zero_vec) {
    cXyz zero(0.0f, 0.0f, 0.0f);
    a->at_sph.SetAtVec(zero);
  }
}
LIZ_NOINLINE void liz_native_link_feedback(daAlink_c* k, dCcD_GObjInf* o) noexcept {
  k->setGuardSe(o);
  dComIfGp_getVibration().StartShock(VIBMODE_S_POWER3, 1, cXyz(0.0f, 1.0f, 0.0f));
  if (k->mNormalSpeed < liz::native::kLinkSlipSpeed) k->mNormalSpeed = liz::native::kLinkSlipSpeed;
}
void liz_apply(e_dn_class* a, const liz::Write& w) noexcept {
  if (!w.any) return;
  if (w.set_hp) liz_native_hp(a);
  if (w.clear_tg) {
    cCcD_Obj* o = liz_native_tg_clear(a, AT_TYPE_SHIELD_ATTACK);
    if (w.bash_se && o) {
      liz_native_bash_se(a, o);
      ++lz.bash_se;
    }
  }
  if (w.clear_tg_sword) (void)liz_native_tg_clear(a, kMasks.sword);
  if (w.clear_learn) liz_native_learn(a);
  if (w.hp_extra > 0) liz_native_hp_bonus(a, w.hp_extra);
  if (w.big_damage) {
    liz_native_break(a);
    return;
  }
  if (w.stagger) liz_native_stagger(a);
  if (w.set_timer0 || w.head_lock_off || w.set_ut2) liz_native_hold(a, w.set_timer0, w.timer0, w.head_lock_off, w.set_ut2);
  if (w.set_action || w.set_mode || w.set_invuln || w.set_timer2 || w.set_cur_y || w.fwd != 0.0f) liz_native_fields(a, w);
  if (w.guard_entry) liz_native_guard(a);
  if (a->anm_p && (w.set_frame || w.set_rate)) liz_native_anim(a->anm_p, w.set_frame, w.frame, w.set_rate, w.rate);
}
// ---------------------------------------------------------------- reads
bool liz_stage_is(const char* have, const char* want) noexcept {
  if (!have) return false;
  for (unsigned i = 0; i < 16; ++i) {
    if (have[i] != want[i]) return false;
    if (!want[i]) return true;
  }
  return false;
}
liz::View liz_view(e_dn_class* a, daAlink_c* k) noexcept {
  liz::View v;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  v.learn = a->learn;
  v.action = a->action, v.mode = a->mode, v.hp = ac->health, v.max_hp = ac->field_0x560, v.invuln = a->invulnerability_timer, v.timer2 = a->timer[2], v.ut2 = a->unk_timer_2;
  v.search_y = a->search_angle_y, v.search_x = a->search_angle_x, v.seen_timer = a->unk_timer_1, v.shape_y = ac->shape_angle.y, v.cur_y = ac->current.angle.y, v.fc = a->frame_counter, v.at_chk = a->at_chk_flag, v.tail = a->tail_at_sph_flag;
  v.anm_no = a->anm_no, v.pl_dir = a->pl_dir, v.recoil = a->field_0x74c;
  if (a->anm_p) v.frame = a->anm_p->getFrame(), v.rate = a->anm_p->getPlaySpeed(), v.end = a->anm_p->getEndFrame();
  v.event = !cp04_world_safe();
  v.link_ok = k != nullptr;
  v.link_human = k && !k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);
  v.horse = k && k->checkHorseRide();
  v.link_alive = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() > 0;
  v.cut_helm = k && k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP;
  v.link_cut = k ? static_cast<int>(k->getCutType()) : -1;  // FIX07
  v.ground = a->objacch.ChkGroundHit() != 0;
  const cXyz& c = a->at_sph.GetC();
  v.at_c[0] = c.x, v.at_c[1] = c.y, v.at_c[2] = c.z, v.at_r = a->at_sph.GetR();
  return v;
}
void liz_contact_reads(e_dn_class* a, daAlink_c* k, liz::View& v) noexcept {
  auto& s = a->at_sph;
  if (s.cCcD_ObjHitInf::ChkAtHit()) {
    v.at_hit = true;
    v.at_shield = s.ChkAtShieldHit();
    const void* obj = static_cast<const void*>(s.cCcD_ObjHitInf::GetAtHitObj());
    for (int i = 0; i < 3 && k; ++i) v.at_on_link = v.at_on_link || obj == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mTgCyls[i]));
  }
  auto& t = a->tate_sph;
  if (t.cCcD_ObjHitInf::ChkTgHit()) {
    v.tate_hit = true;
    const cCcD_Obj* o = t.cCcD_ObjHitInf::GetTgHitObj();
    bool sword = o && o->ChkAtType(kMasks.sword) != 0;
    bool own = false;
    for (int i = 0; i < 3 && k; ++i) own = own || static_cast<const void*>(o) == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mAtCps[i]));
    if (k && static_cast<const void*>(o) == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mAtCyl))) own = true;  // spin attacks use the cylinder
    // FIX07 P1: the cylinder is the Back Slice's (procCutFinishJumpUpInit setCylAtParam); the spins are on mAtSph (procCutTurnInit -> initCutTurnAt):
    // without it a spin's real contact with the shield sphere was never Link's sword, so no spin could ever be blocked by the shield
    if (k && static_cast<const void*>(o) == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mAtSph))) own = true;
    v.tate_link_sword = sword && own;
  }
}
inline constexpr float kLizJumpAtStartFrame = 13.0f;  // LFT 0.13.5: daAlinkHIO_cutJump_c0::m.mStartAttackFrame (pinned by ci/kk_native_audit.py)
liz::LinkFrame liz_link_frame(daAlink_c* k) noexcept {
  liz::LinkFrame L;
  L.swing_serial = lz.swing_serial;
  L.shield_serial = c4.shield.native_serial;
  L.jump_cut = lz.swing_jump;
  if (k) {
    L.shape_y = static_cast<fopAc_ac_c*>(k)->shape_angle.y;
    // FIX05: where this cut really is in its native clip. procCutNormal/procCutFinish open the AT at field_0x3478 and close it at field_0x347c (d_a_alink_cut.inc), and they are the procs that run changeCutReverseProc with a recoil clip.
    const int proc = static_cast<int>(k->mProcID);
    if (proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH) {
      const float f = k->mUnderFrameCtrl[0].getFrame();
      L.at_done = f >= k->field_0x347c;
      L.at_eta = liz::eta_ticks(f, k->mUnderFrameCtrl[0].getRate(), k->field_0x3478);
    } else if (proc == daAlink_c::PROC_CUT_JUMP) {
      // LFT 0.13.5: the Jump Attack HAS a rebound route in the air: procCutJump runs changeCutReverseProc(ANM_CUT_JUMP) (heading reversed, mNormalSpeed 27,
      // POWER5 shock, d_a_alink_cut.inc), as procCutJumpLand runs ANM_CUT_RECOIL_B; it stays can_rebound. procCutJump opens its aerial AT at mCutJump.mStartAttackFrame of ANM_CUT_JUMP (13.0, d_a_alink_HIO_data.inc; the game's own constant,
      // not read from its HIO data: no import), so the shield is presented when THAT AT approaches, not at the takeoff. That AT lasts until the landing
      // (frames 13..~16.5, then the landing AT): one tick later than a ground cut's lead, so the 5-tick window (ages 3..7) covers the aerial contact AND the landing.
      const std::int32_t eta = liz::eta_ticks(k->mUnderFrameCtrl[0].getFrame(), k->mUnderFrameCtrl[0].getRate(), kLizJumpAtStartFrame);
      L.at_eta = eta >= 0 ? eta + 1 : eta;
    } else if (proc != daAlink_c::PROC_CUT_JUMP_LAND && proc >= daAlink_c::PROC_CUT_FINISH_JUMP_UP && proc <= daAlink_c::PROC_CUT_LARGE_JUMP_LAND) {
      L.can_rebound = proc == daAlink_c::PROC_CUT_REVERSE;  // spin, jump and the rest: no recoil clip exists on that route, so no parry can be promised there
    }
  }
  return L;
}
// ---------------------------------------------------------------- event drain: telemetry and Link's own posture (the ONLY place this module touches lg.*)
void liz_link_gain(LizSlot& s, int points, lnk05::Kind kind, std::uint64_t token) noexcept {
  const lnk01::Outcome o = lg.posture.on_external(points);
  if (o.v != lnk01::Verdict::Confirmed) return;
  ++lz.link_gain;
  if (o.saturated) ++lz.link_saturations;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LIZ_LINK event=POSTURE_GAIN actor=%u generation=%llu kind=%s points=%d before=%d.%03d after=%d.%03d saturated=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.b.generation), lnk05::name(kind), points, o.before / 1000, o.before % 1000, o.after / 1000, o.after % 1000, o.saturated ? 1 : 0,
                static_cast<unsigned long long>(g.p8.tick));
  liz_line(kLogLink, LOG_LEVEL_INFO, line);
  if (o.after >= lnk01::policy::kMaxMilli) {
    lnk05_request(kind, s.id, s.b.generation, token, o.before, o.after);
    // Link may be mid-cut: the request's own gate only fires in a safe native origin. Offer it again while he stays saturated, never a forced state.
    lz.break_retry_until = g.p8.tick + 90;
    lz.retry_attacker = s.id, lz.retry_generation = s.b.generation;
  }
}
void liz_drain(LizSlot& s, e_dn_class* a) noexcept {
  auto& b = s.b;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  char line[kLogLineMax];
  for (unsigned i = 0; i < b.nev; ++i) {
    const liz::Ev& e = b.ev[i];
    switch (e.k) {
      case liz::EvK::Admitted:
        ++lz.admitted;
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=ADMITTED version=%s actor=%u generation=%llu arg0=%d arg1=%d hp=%d max=%d action=%d", liz::kVersion, s.id,
                      static_cast<unsigned long long>(b.generation), e.b, e.c, ac->health, ac->field_0x560, a->action);
        liz_line(kLogAdmit, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Excluded:
        ++lz.excluded;
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=EXCLUDED_VANILLA reason=%s actor=%u arg0=%d arg1=%d action=%d", liz::name(static_cast<liz::Excl>(e.a)), s.id, e.b, e.c, a->action);
        liz_line(kLogAdmit, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::HpSet:
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=HP_SET actor=%u generation=%llu hp=%d from=%d/%d once=1", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c);
        liz_line(kLogAdmit, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ComboStart:
        std::snprintf(line, sizeof(line), "LIZ_COMBO event=START actor=%u generation=%llu pattern=%d acts=%d impacts=%d token=%llu tick=%llu posture=%d hp=%d", s.id,
                      static_cast<unsigned long long>(b.generation), e.a + 1, e.b, e.c, static_cast<unsigned long long>(b.ep.token), static_cast<unsigned long long>(e.t), b.p.points(), ac->health);
        liz_line(kLogCombo, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ComboEnd:
        std::snprintf(line, sizeof(line), "LIZ_COMBO event=END actor=%u generation=%llu pattern=%d cause=%s impacts_opened=%d tick=%llu posture=%d hp=%d action=%d", s.id,
                      static_cast<unsigned long long>(b.generation), e.a + 1, liz::name(static_cast<liz::End>(e.b)), e.c, static_cast<unsigned long long>(e.t), b.p.points(), ac->health, a->action);
        liz_line(kLogCombo, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ActionStart:
        std::snprintf(line, sizeof(line), "LIZ_COMBO event=ACTION actor=%u pattern=%d act=%d kind=%d start_frame=%.2f rate=%.2f tick=%llu dist=%.1f", s.id, e.a + 1, e.b, e.c, e.f, e.g,
                      static_cast<unsigned long long>(e.t), a->pl_dir);
        liz_line(kLogCombo, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ImpactOpen:
        std::snprintf(line, sizeof(line), "LIZ_SWING event=OPEN actor=%u pattern=%d impact=%d kind=%d frame=%.2f rate=%.2f tick=%llu dist=%.1f r=%.1f atp=%d", s.id, e.a + 1, e.b, e.c, e.f, e.g,
                      static_cast<unsigned long long>(e.t), a->pl_dir, a->at_sph.GetR(), a->at_sph.GetAtAtp());
        liz_line(kLogSwing, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::WindowClose:
        std::snprintf(line, sizeof(line), "LIZ_SWING event=CLOSE actor=%u pattern=%d impact=%d frame=%.2f max_center_step=%.1f tick=%llu", s.id, e.a + 1, e.b, e.f, e.g, static_cast<unsigned long long>(e.t));
        liz_line(kLogSwing, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Contact:
        std::snprintf(line, sizeof(line), "LIZ_SWING event=CONTACT actor=%u generation=%llu result=%s pattern=%d impact=%d link_delta=%.2f frame=%.2f tick=%llu posture=%d", s.id,
                      static_cast<unsigned long long>(b.generation), liz::name(static_cast<liz::Hit>(e.a)), e.b + 1, e.c, e.f, e.g, static_cast<unsigned long long>(e.t), b.p.points());
        liz_line(kLogSwing, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::LinkBlock: liz_link_gain(s, e.a, lnk05::Kind::Block, b.cur_impact); break;
      case liz::EvK::LinkGain: liz_link_gain(s, e.a, lnk05::Kind::Direct, b.cur_impact); break;
      case liz::EvK::LinkDirectOffer: {
        lnk01::Contact k;
        k.result = post08::Result::NotBlocked, k.token = b.cur_impact ? b.cur_impact : 1, k.slot = static_cast<int>(&s - lz.slots), k.id = s.id, k.generation = b.generation, k.atp = e.b;
        k.link_ok = true, k.link_human = true, k.link_id = lg.posture.link_id(), k.tick = g.p8.tick;
        lz.direct.offer(k, reinterpret_cast<std::uintptr_t>(static_cast<const cCcD_Obj*>(&a->at_sph)), 0, true);
        break;
      }
      case liz::EvK::Roll:
        std::snprintf(line, sizeof(line), "LIZ_DEFENSE event=ROLL actor=%u level=%c pct=%d attempt=%d tick=%llu cooldown_ok=%d swings75=%u swings150=%u dist=%.1f", s.id, "ABC"[e.a < 0 ? 0 : e.a > 2 ? 2 : e.a], e.c, e.b,
                      static_cast<unsigned long long>(e.t), b.def.cooled(b.local) ? 1 : 0, b.def.swings_within(e.t, liz::policy::kPressureWindow), b.def.swings_within(e.t, liz::policy::kSustainedWindow), a->pl_dir);
        liz_line(kLogDefense, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::GuardEntry:
        std::snprintf(line, sizeof(line), "LIZ_DEFENSE event=GUARD_ENTRY actor=%u level=%d mode=%d attempt=%d tick=%llu", s.id, e.a, e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogDefense, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Block:
        std::snprintf(line, sizeof(line), "LIZ_GUARD event=BLOCK actor=%u generation=%llu gain=+%d posture_before=%d hp=%d rebound=0 tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, ac->health,
                      static_cast<unsigned long long>(e.t));
        liz_line(kLogGuard, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::EnemyParry:
        std::snprintf(line, sizeof(line), "LIZ_DEFLECT event=ENEMY_PARRY actor=%u generation=%llu link_gain=+%d liz_gain=+0 rebound=NATIVE level=%d guard_age=%d hp=%d tick=%llu cooldown_until=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c,
                      ac->health, static_cast<unsigned long long>(e.t), static_cast<unsigned long long>(b.def.cooldown_until));
        liz_line(kLogParry, LOG_LEVEL_INFO, line);
        pfx_settle(pfx::Kind::LizalfosOnLink, s.id, true);  // FIX03: the parry the pass previewed is the one this Execute credited
        break;
      case liz::EvK::DeflectRoll:
        std::snprintf(line, sizeof(line), "LIZ_DEFLECT event=PREPARE_ROLL actor=%u generation=%llu armed=%d pct=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, static_cast<unsigned long long>(e.t));
        liz_line(kLogDeflect, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::DeflectWindow:
        std::snprintf(line, sizeof(line), "LIZ_DEFLECT event=NEW_WINDOW actor=%u generation=%llu token=%d level=%d waited=%d mode=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, a->mode, static_cast<unsigned long long>(e.t));
        liz_line(kLogDeflect, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::DeflectEnd:
        std::snprintf(line, sizeof(line), "LIZ_DEFLECT event=EXPIRED actor=%u generation=%llu reason=%s waited=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), liz::name(static_cast<liz::Cancel>(e.a)), e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogDeflect, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::GuardRoll:
        std::snprintf(line, sizeof(line), "LIZ_GUARD event=WOUND_ROLL actor=%u generation=%llu wound=%d pct=%d prepare=%d hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, ac->health, static_cast<unsigned long long>(e.t));
        liz_line(kLogGuard, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::GuardEnter:
        std::snprintf(line, sizeof(line), "LIZ_GUARD event=ENTER source=WOUND actor=%u generation=%llu level=%d waited=%d dist=%.1f tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, a->pl_dir, static_cast<unsigned long long>(e.t));
        liz_line(kLogGuard, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::GuardIntentEnd:
        std::snprintf(line, sizeof(line), "LIZ_GUARD event=INTENT_END actor=%u generation=%llu reason=%s waited=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), liz::name(static_cast<liz::Cancel>(e.a)), e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogGuard, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::BreakBonus:
        std::snprintf(line, sizeof(line), "LIZ_BREAK_BONUS event=DECISIVE actor=%u generation=%llu base=%d extra=%d final_hp=%d lethal=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, e.c == 0 ? 1 : 0, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterSelected:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=COUNTER_SELECTED actor=%u generation=%llu token=%llu kind=%s source=%s tick=%llu dist=%.1f", s.id, static_cast<unsigned long long>(b.generation), static_cast<unsigned long long>(b.counter.token), e.a == 0 ? "NORMAL_X1_25" : "DOUBLE_X1_10",
                      "ENEMY_PARRY", static_cast<unsigned long long>(e.t), a->pl_dir);
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterGuardExit:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=GUARD_EXIT actor=%u token=%llu from_mode=%d tick=%llu", s.id, static_cast<unsigned long long>(b.counter.token), e.a, static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Chain: {  // FIX06 P3: counter -> chained combo (0 scheduled, 1 no chain, 2 cancelled, 3 started)
        static const char* const kChainEv[4] = {"SCHEDULED", "NO_CHAIN", "CANCELLED", "STARTED"};
        std::snprintf(line, sizeof(line), "LIZ_CHAIN event=%s actor=%u generation=%llu counter_token=%llu pattern=%d detail=%d tick=%llu", kChainEv[e.a & 3], s.id, static_cast<unsigned long long>(b.generation),
                      static_cast<unsigned long long>(b.counter.token), e.a == 0 || e.a == 3 ? e.b + 1 : 0, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      }
      case liz::EvK::CounterCommitted:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=ATTACK_COMMITTED actor=%u token=%llu kind=%s source=%s waited_ticks=%d tick=%llu dist=%.1f", s.id, static_cast<unsigned long long>(b.counter.token), e.a == 0 ? "NORMAL_X1_25" : "DOUBLE_X1_10", "ENEMY_PARRY", e.c,
                      static_cast<unsigned long long>(e.t), a->pl_dir);
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterAtOpen:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=AT_OPEN actor=%u token=%llu tick=%llu", s.id, static_cast<unsigned long long>(b.counter.token), static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterContact:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=CONTACT actor=%u token=%llu result=%s tick=%llu", s.id, static_cast<unsigned long long>(b.counter.token), liz::name(static_cast<liz::Hit>(e.a)), static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterExpired:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=EXPIRED actor=%u token=%llu cancel_reason=%s kind=%d waited_ticks=%d tick=%llu", s.id, static_cast<unsigned long long>(b.counter.token), liz::name(static_cast<liz::Cancel>(e.a)), e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::HelmPrepared:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=HELM_PREPARED actor=%u generation=%llu posture=%d hp=%d age=%d link_serial=%llu tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, static_cast<unsigned long long>(b.shield_serial), static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Decisive:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=DECISIVE_HIT actor=%u generation=%llu kind=%s hp=%d posture=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a ? "HELM_SPLITTER" : "SWORD", e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Down:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=DOWN_FINISH actor=%u generation=%llu hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::GetUp:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=GETUP actor=%u generation=%llu hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Impact: {
        const auto& m = b.last_imp;
        const bool anomaly = m.cls == liz::Cls::MissingFeedback;
        const bool defended = m.cls == liz::Cls::LinkGuard || m.cls == liz::Cls::LinkParry;
        if (defended && m.feedback_native + m.feedback_restored == 0) ++lz.no_receipt;
        if (anomaly || m.written || lz.lines[kLogImpact] < 400) {
          std::snprintf(line, sizeof(line), "LIZ_IMPACT actor=%u generation=%llu pattern=%d act=%d impact=%d class=%s link_proc=%d open=%llu contact=%llu expect=%u candidate_written=%u immune_touch=%u lost=%u no_continuous=%u audio_native_receipt=%u audio_restored_call=%u audio=%s effect=ENGINE_NOT_OBSERVED runtime=HUMAN_RUNTIME_PENDING", s.id,
                        static_cast<unsigned long long>(b.generation), m.pattern + 1, m.act, m.impact, liz::name(m.cls), m.link_proc, static_cast<unsigned long long>(m.open_tick), static_cast<unsigned long long>(m.contact_tick), m.expect, m.written, m.off_touch, m.lost, m.nocon, m.feedback_native, m.feedback_restored, m.feedback_native + m.feedback_restored ? "ENGINE_CALL_VERIFIED" : "NONE");
          liz_line(kLogImpact, anomaly ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
        }
        break;
      }
      case liz::EvK::Break:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=BREAK_STANDING actor=%u generation=%llu posture=%d hp=%d native_knockdown=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Rearm:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=REARM actor=%u generation=%llu posture=%d next_action=%d mode=%d hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, ac->health, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::PostureGain:
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=GAIN actor=%u generation=%llu source=%s gain=+%d posture=%d hp=%d latched=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation),
                      e.c == 0 ? "LINK_PARRY" : e.c == 1 ? "SWORD" : e.c == 2 ? "IRON_BALL" : "BLOCK", e.a, e.b, ac->health, b.p.latched ? 1 : 0, static_cast<unsigned long long>(e.t));
        liz_line(kLogPosture, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Trace: {
        const auto& t = b.tr;
        std::snprintf(line, sizeof(line), "LIZ_TRACE actor=%u generation=%llu epoch=%llu swing=%llu tick=%llu action=%d mode=%d frame=%.1f shape_y=%d search_y=%d presented=%d covers=%d body_hit=%d shield_hit=%d verdict=%s hp=%d->%d posture=%d->%d ut2=%d shield_dx=%.1f shield_dz=%.1f link_dx=%.1f link_dz=%.1f", s.id,
                      static_cast<unsigned long long>(b.generation), static_cast<unsigned long long>(lg.posture.epoch()), static_cast<unsigned long long>(t.swing), static_cast<unsigned long long>(g.p8.tick), t.action, t.mode, t.frame, t.shape_y, t.search_y, t.presented ? 1 : 0, t.covers ? 1 : 0, t.body_hit ? 1 : 0, t.shield_hit ? 1 : 0, liz::name(t.verdict), t.hp_before, t.hp_after, t.posture_before, t.posture_after, a->unk_timer_2,
                      a->tate_sph.GetC().x - ac->current.pos.x, a->tate_sph.GetC().z - ac->current.pos.z, f1_link() ? static_cast<fopAc_ac_c*>(f1_link())->current.pos.x - ac->current.pos.x : 0.0f, f1_link() ? static_cast<fopAc_ac_c*>(f1_link())->current.pos.z - ac->current.pos.z : 0.0f);
        liz_line(kLogTrace, LOG_LEVEL_INFO, line);
        if (kk::f7::skill_of(t.cut) != kk::f7::Sk::None) {  // FIX07: the same contact, auditable per special skill
          const bool defended = t.verdict == liz::Tv::Block || t.verdict == liz::Tv::EnemyParry || t.verdict == liz::Tv::ShieldWonBody;
          if (defended && t.hp_after < t.hp_before) ++kk::f7::g_tally[1].leak;  // must stay 0
          if (t.verdict == liz::Tv::Block || t.verdict == liz::Tv::ShieldWonBody) ++kk::f7::g_tally[1].blocks[unsigned(kk::f7::skill_of(t.cut))];
          if (t.verdict == liz::Tv::EnemyParry) ++kk::f7::g_tally[1].parries[unsigned(kk::f7::skill_of(t.cut))];
          std::snprintf(line, sizeof(line), "LIZ_DEFENSE_FIX07 event=CONTACT actor=%u generation=%llu skill=%s cut=%d swing=%llu native_action=%d native_mode=%d frame=%.1f guard_effective=%d frontal_arc=%d "
                        "shield_hit=%d body_hit=%d verdict=%s hp_before=%d hp_after=%d posture_before=%d posture_after=%d streak=%u tick=%llu", s.id, static_cast<unsigned long long>(b.generation),
                        kk::f7::name(kk::f7::skill_of(t.cut)), t.cut, static_cast<unsigned long long>(t.swing), t.action, t.mode, t.frame, t.effective ? 1 : 0, t.covers ? 1 : 0, t.shield_hit ? 1 : 0,
                        t.body_hit ? 1 : 0, liz::name(t.verdict), t.hp_before, t.hp_after, t.posture_before, t.posture_after, b.sk.count(b.local), static_cast<unsigned long long>(g.p8.tick));
          liz_line(kLogCut, LOG_LEVEL_INFO, line);
        }
        break;
      }
      case liz::EvK::Response: {
        const auto why = static_cast<liz::Np>(e.a);
        std::snprintf(line, sizeof(line), "LIZ_RESPONSE event=CONTACT actor=%u generation=%llu epoch=%llu tick=%llu swing=%llu result=%s chain=%d guard_age=%d token=%d mode=%d frame=%.1f hp=%d posture=%d cooldown_until=%llu", s.id, static_cast<unsigned long long>(b.generation), static_cast<unsigned long long>(lg.posture.epoch()), static_cast<unsigned long long>(e.t), static_cast<unsigned long long>(b.last_swing), liz::name(why), e.b, e.c, b.deflect.armed ? 1 : 0, a->mode, a->anm_p ? a->anm_p->getFrame() : 0.0f, ac->health, b.p.points(), static_cast<unsigned long long>(b.def.cooldown_until));
        liz_line(kLogResponse, LOG_LEVEL_INFO, line);
        break;
      }
      case liz::EvK::Represent:
        std::snprintf(line, sizeof(line), "LIZ_RESPONSE event=REPRESENT actor=%u generation=%llu tick=%llu swing=%llu at_eta=%d mode=%d since_swing=%d", s.id, static_cast<unsigned long long>(b.generation), static_cast<unsigned long long>(e.t), static_cast<unsigned long long>(b.last_swing), e.a, e.b, e.c);
        liz_line(kLogResponse, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Conflict:
        std::snprintf(line, sizeof(line), "LIZ_CONFLICT event=DAMAGE_WON_PARRY actor=%u generation=%llu swing=%llu hp=%d->%d guard_age=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), static_cast<unsigned long long>(b.last_swing), e.a, e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogCritical, LOG_LEVEL_WARN, line);
        break;
      case liz::EvK::Critical:
      case liz::EvK::Coverage:
        std::snprintf(line, sizeof(line), "LIZ_CRITICAL event=%s actor=%u code=%d a=%d b=%d f=%.1f g=%.1f tick=%llu", e.k == liz::EvK::Coverage ? "AT_SAMPLE_GAP" : "STATE", s.id, e.a, e.b, e.c, e.f, e.g, static_cast<unsigned long long>(e.t));
        liz_line(kLogCritical, LOG_LEVEL_WARN, line);
        break;
      default: break;
    }
  }
  b.nev = 0;
}
// ---------------------------------------------------------------- hooks
HookAction on_liz_exec_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  LizSlot* s = liz_find(a);
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(ac));
  if (s && s->id != id) {  // address reused by a new process: never inherits anything
    liz_retire(*s);
    s = nullptr;
  }
  if (!s) {
    for (auto& x : lz.slots)
      if (!x.on) {
        s = &x;
        break;
      }
    if (!s) {
      ++lz.overflow;
      return HOOK_CONTINUE;  // vanilla actor, counted
    }
    s->on = true, s->key = reinterpret_cast<std::uintptr_t>(a), s->id = id, s->sw = LizSweep{};
    s->sk_raised = false, s->sk_credit_now = false, s->sk_credit = kk::f7::Credit{};  // FIX07: a recycled slot never inherits a special's credit
    s->b.bind(id, ++lz.generations, g.p8.tick ^ (static_cast<std::uint64_t>(lz.generations) << 20));
    liz::IdIn in;
    in.profile_ok = ac->profname == fpcNm_E_DN_e && ac->name == fpcNm_E_DN_e;
    in.skull_model = a->skull_model != nullptr, in.arg0 = a->arg0, in.arg1 = a->arg1, in.status = a->status, in.action = a->action;
    in.special_room = liz_stage_is(dComIfGp_getStartStageName(), "D_MN07") && fopAcM_GetRoomNo(ac) == 8;
    s->b.admit(in);
    liz_drain(*s, a);
  }
  if (!s->b.admitted) return HOOK_CONTINUE;
  pfx_liz_exec_begin(a, s->id);  // FIX03: this Execute's own shield spark / shield SE belong to its parry contact, if any
  daAlink_c* k = f1_link();
  liz::View v = liz_view(a, k);
  liz_contact_reads(a, k, v);
  liz::LinkFrame L = liz_link_frame(k);
  if (v.at_hit && v.at_on_link && k) {  // the shield provenance only when there is a contact: permits() is a counted gate
    L.pass = g.link_track.pass_for(g.p8.tick, true, static_cast<std::uint32_t>(fopAcM_GetID(k)));
    if (L.pass.guard_attack && !cp04_guard_pass()) L.pass.guard_attack = false;
  }
  const std::uint64_t parries = s->b.st.link_parry;
  const std::uint64_t guard_contact = s->b.st.contact[unsigned(liz::Hit::Guard)] + s->b.st.contact[unsigned(liz::Hit::MissedEarly)];
  const std::uint64_t def_contact = guard_contact + s->b.st.contact[unsigned(liz::Hit::Direct)];
  liz::Write w = s->b.on_exec_pre(v, L);
  pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry != parries);  // FIX03: the verdict of the contact the pass previewed
  if (s->b.st.link_parry != parries) {  // the same shield chain contract CP04 uses for every enemy
    (void)c4.shield.confirm(g.p8.tick);
    lz.d1_parry_id = s->id;  // the fast re-arm below only ever serves the combo of the actor Link just defended against
    g.link_track.note(L.pass.serial, false, false, true);
    g.p8.watch_life = true;
    g.p8.life_at = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "LIZ_PARRY event=LINK_PARRY actor=%u generation=%llu pattern=%d impact=%d gain=+%d posture=%d latched=%d link_serial=%llu delta=%.2f tick=%llu", s->id,
                  static_cast<unsigned long long>(s->b.generation), s->b.ep.pattern + 1, s->b.ep.impact, liz::policy::kGainLinkParry, s->b.p.points(), s->b.p.latched ? 1 : 0,
                  static_cast<unsigned long long>(L.pass.serial), L.pass.frame - L.pass.start, static_cast<unsigned long long>(g.p8.tick));
    liz_line(kLogParry, LOG_LEVEL_INFO, line);
  } else if (s->b.st.contact[unsigned(liz::Hit::Guard)] + s->b.st.contact[unsigned(liz::Hit::Direct)] + s->b.st.contact[unsigned(liz::Hit::MissedEarly)] != def_contact) {
    if (c4.shield.pass_valid) c4.shield.cancel();
    if (s->b.st.contact[unsigned(liz::Hit::Guard)] + s->b.st.contact[unsigned(liz::Hit::MissedEarly)] != guard_contact && s->b.owes_impact()) {
      // An ordinary block of a combo that still owes an impact: native GUARD_SLIP would swallow the next one (no Shield Attack starts from it). A NEW real press may restart the native Shield Attack; nothing starts without it.
      c4.shield.armed = true;
      c4.shield.buffer = false;
      c4.shield.permit_serial = c4.shield.native_serial;
      c4.shield.permit_tick = g.p8.tick;
      lz.d1_parry_id = s->id;
      ++lz.gap_arms;
    }
  }
  liz_apply(a, w);
  liz_drain(*s, a);
  return HOOK_CONTINUE;
}
// ---------------------------------------------------------------- skipped-pose coverage of THIS tick's AT sphere (runs after Execute, before the collision pass that reads it)
// Pure calc path only (checked by the caller). Snapshot, evaluate, exact frame back, verify every joint matrix: same contract as cp06f1_pose_probe, on the actor's own morf.
bool liz_pose_probe(e_dn_class* a, int joint, const float* frames, unsigned count, float restore, float out[liz::sweep::kMaxProbes][3][4]) noexcept {
  mDoExt_McaMorfSO* m = a->anm_p;
  J3DModel* model = m ? m->getModel() : nullptr;
  if (!model || count < 1 || count > liz::sweep::kMaxProbes) return false;
  const unsigned n = model->getModelData()->getJointNum();
  if (n > kF1Joints || n <= static_cast<unsigned>(joint)) return false;
  for (unsigned i = 0; i < n; ++i) std::memcpy(lz.snap[i], model->getAnmMtx(int(i)), sizeof(lz.snap[i]));
  for (unsigned i = 0; i < count; ++i) {
    liz_native_pose_frame(m, frames[i]);
    std::memcpy(out[i], model->getAnmMtx(joint), sizeof(lz.snap[0]));
  }
  liz_native_pose_frame(m, restore);
  bool same = m->getFrame() == restore;
  for (unsigned i = 0; i < n && same; ++i) {
    const MtxP q = model->getAnmMtx(int(i));
    for (int r = 0; r < 3 && same; ++r)
      for (int c = 0; c < 4 && same; ++c) same = q[r][c] == lz.snap[i][r][c];
  }
  return same;
}
void liz_sweep(e_dn_class* a, daAlink_c* k, LizSlot& s, const liz::View& v) noexcept {
  using namespace liz::sweep;
  auto& S = s.sw;
  auto& b = s.b;
  const liz::Act act = liz::kPattern[b.ep.pattern].a[b.ep.act];
  const std::uint8_t win = b.ep.window;
  const bool same_window = S.armed && S.impact == b.cur_impact && S.local + 1 == b.local;  // on_exec_post advances local after this call: the previous armed Execute of this window stored local-1
  const cXyz& cc = a->at_sph.GetC();
  const cp06f1::V3 center{cc.x, cc.y, cc.z};
  cp06f1::Sph cur;
  cur.c = center, cur.r = a->at_sph.GetR();
  bool expect = false, written = false, off_touch = false, candidate = false, covered = false;
  Why why = Why::None;
  const bool current = k && f1.link_set_tick == g.p8.tick && f1.link_set_key == reinterpret_cast<std::uintptr_t>(k);
  if (!k) {
    why = Why::LinkMissing;
  } else if (!current) {
    why = Why::LinkNotCurrent;
  } else {
    cp06f1::Cyl cy[3];
    f1_cyls(k, cy);
    expect = cp06f1::touches(cy, cur, cp06f1::kWriteSlack, true);
    const bool any_on = cp06f1::any_on(cy);
    off_touch = !any_on && cp06f1::touches(cy, cur, cp06f1::kKeepSlack, false);
    covered = true;
    if (expect) {
      why = Why::AlreadyTouching;
    } else {
      mDoExt_McaMorfSO* m = a->anm_p;
      const float post = v.frame, pre = same_window ? S.frame : post - v.rate;
      float fr[kMaxProbes] = {};
      const unsigned np = lz.coverage ? probe_frames(act, win, pre, post, same_window, S.covered, fr) : 0;
      cp06f1::Cyl any[3] = {cy[0], cy[1], cy[2]};
      for (auto& x : any) x.on = true;
      if (!np) {
        why = !lz.coverage || !(post - pre >= kMinRate) ? Why::NotFast : Why::NoMidContact;
      } else if (!reachable(any, center, S.center, S.have, cur.r)) {
        why = Why::OutOfReach;
      } else if (!(m->getMorfRatio() >= 1.0f || !m->getOldTransInfo() || !m->getOldQuaternion())) {
        why = Why::MorphActive;
        covered = false;
      } else if (any_on || !off_touch) {
        float pm[kMaxProbes][3][4];
        ++lz.probes;
        const bool tail = liz::is_tail(act);
        if (!liz_pose_probe(a, tail ? liz::native::kJntTail04 : liz::native::kJntHandR1, fr, np, post, pm)) {
          ++lz.probe_mismatch;
          lz.coverage = false;
          why = Why::ProbeMismatch;
          char line[kLogLineMax];
          std::snprintf(line, sizeof(line), "LIZ_CRITICAL event=POSE_PROBE_RESTORE_MISMATCH actor=%u generation=%llu frame=%.3f probes=%u action=COVERAGE_DISABLED_FOR_SESSION", s.id, static_cast<unsigned long long>(b.generation), post, np);
          lnk_write(LOG_LEVEL_ERROR, line);
        } else {
          float sn = 0, cs = 1;
          cp06f1::sincos16(tail ? a->search_angle_y : static_cast<fopAc_ac_c*>(&a->actor)->current.angle.y, sn, cs);
          cp06f1::Sph cand[kMaxProbes];
          for (unsigned i = 0; i < np; ++i) {
            const float back = lunge_back(act, win, post, fr[i]);
            const float ly = tail ? 0.0f : liz::native::kSwordY;
            cand[i].c = {pm[i][0][1] * ly + pm[i][0][3] - sn * back, pm[i][1][1] * ly + pm[i][1][3], pm[i][2][1] * ly + pm[i][2][3] - cs * back};
            cand[i].r = cur.r;
          }
          const Decision d = decide(cy, cur, cand, np);
          why = d.why;
          candidate = d.why == Why::Written;
          if (d.why == Why::LinkInvulnerable) off_touch = true;
          if (d.write) {
            liz_native_at_place(a, true, d.place.x, d.place.y, d.place.z, false);
            const cXyz& rb = a->at_sph.GetC();
            written = rb.x == d.place.x && rb.y == d.place.y && rb.z == d.place.z;
            if (!written) ++lz.place_failed;
            char line[kLogLineMax * 2];
            if (lnk_take(lz.lines[kLogImpact], lz.dropped[kLogImpact], kLizCaps[kLogImpact])) {
              std::snprintf(line, sizeof(line), "LIZ_IMPACT event=CANDIDATE_INTERSECTION actor=%u generation=%llu pattern=%d act=%d impact=%d kind=%d frame_pre=%.3f frame_post=%.3f rate=%.3f probes=%u place=(%.1f,%.1f,%.1f) registered_were=(%.1f,%.1f,%.1f) r=%.1f link_proc=%d meaning=EXACT_NATIVE_POSE_REACHES_LINK engine_decides=GUARD_PARRY_DAMAGE_IFRAMES runtime=HUMAN_RUNTIME_PENDING", s.id,
                            static_cast<unsigned long long>(b.generation), b.ep.pattern + 1, b.ep.act, b.ep.impact, int(act), pre, post, v.rate, np, d.place.x, d.place.y, d.place.z, center.x, center.y, center.z, cur.r, k->mProcID);
              lnk_write(LOG_LEVEL_INFO, line);
            }
          }
        }
      }
    }
    // The block test uses the sphere's velocity when it has one: a lateral swing or tail sweep in front of a raised shield could read as "from the side". While Link's shield is up the engine decides with the real relative position, exactly its StartCAt state.
    if (k->mTgCyls[0].ChkTgShield() || k->mTgCyls[0].ChkTgSpShield()) {
      liz_native_at_place(a, false, 0.0f, 0.0f, 0.0f, true);
      ++lz.zero_vec;
    }
  }
  ++lz.why[unsigned(why) < unsigned(Why::Count) ? unsigned(why) : 0];
  b.on_sweep(expect, written, off_touch, candidate);
  S.armed = true, S.covered = covered, S.local = b.local, S.impact = b.cur_impact, S.frame = v.frame, S.center = center, S.have = true;
}
void on_liz_exec_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  daAlink_c* k = f1_link();
  liz::View v = liz_view(a, k);
  liz_contact_reads(a, k, v);
  if (s->b.armed_now && s->b.ep.active) liz_sweep(a, k, *s, v);
  else s->sw.armed = false, s->sw.covered = false;
  const std::uint64_t parries_before = s->b.st.enemy_parries;
  const liz::Write w = s->b.on_exec_post(v, liz_link_frame(k));
  liz_apply(a, w);
  if (k && s->b.st.enemy_parries != parries_before) {  // FIX05: what Link's clip really did after this parry (his Execute ran first, so the native transition is already there)
    const bool rev = k->mProcID == daAlink_c::PROC_CUT_REVERSE;
    ++(rev ? lz.rebound_state_ok : lz.rebound_state_other);
    ++lz.cuts[k->getCutType() & 63].parries;
    char st[kLogLineMax];
    std::snprintf(st, sizeof(st), "COMBO_LINK event=REBOUND_STATE actor=%u generation=%llu swing=%llu cut=%d proc=%d reverse=%d tick=%llu", s->id, static_cast<unsigned long long>(s->b.generation), static_cast<unsigned long long>(lz.swing_serial), k->getCutType(), static_cast<int>(k->mProcID), rev ? 1 : 0, static_cast<unsigned long long>(g.p8.tick));
    liz_line(kLogCut, LOG_LEVEL_INFO, st);
  }
  if (s->b.impact_gap() && lz.d1_parry_id == s->id) lz.double_gap_tick = g.p8.tick;
  liz_drain(*s, a);
  pfx_settle(pfx::Kind::LizalfosOnLink, s->id, false);  // FIX03: no EnemyParry in this Execute: a previewed orange spark goes back to its own colour
  pfx_liz_exec_end();
}
HookAction on_liz_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (s) {
    s->b.on_delete();
    if (s->b.admitted) liz_drain(*s, a);
    liz_retire(*s);
    ++lz.deleted;
  }
  return HOOK_CONTINUE;
}
// FIX06 P0 helpers. The ONE HP writer is the audited liz_native_hp_bonus (health - extra): a negative extra raises, so no new store site exists.
void liz_js_hp(e_dn_class* a, int v) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  liz_native_hp_bonus(a, static_cast<std::int16_t>(ac->health - v));
}
void liz_js_settle(LizSlot& s, e_dn_class* a, daAlink_c* k, const char* where) noexcept {
  if (!s.js_raised) return;
  s.js_raised = false;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  const int native = s.js_hp_before + kk::f6::kJsHeadroom - ac->health;
  s.js_credit_now = native > 0 && s.js_credit.take(kk::f6::g_js_episode);
  const int ord = kk::f6::ordinary_of(native, s.js_atp);
  const int dmg = s.js_credit_now ? kk::f6::js_damage(ord) : 0;
  const int target = kk::f6::js_target(s.js_hp_before, dmg, kk::f6::kFloorDn);
  liz_js_hp(a, target);
  s.b.js_call = true, s.b.js_gain = s.js_credit_now ? kk::f6::kGainJsDn : 0, ++s.b.st.f6.js_calls;
  ++kk::f6::g_js_tally[1].contacts, ++(s.js_credit_now ? kk::f6::g_js_tally[1].credited : kk::f6::g_js_tally[1].dupes);
  if (s.js_hp_before - dmg < kk::f6::kFloorDn && s.js_credit_now) ++kk::f6::g_js_tally[1].floored;
  kk::f6::g_js_tally[1].hp_units += static_cast<std::uint64_t>(s.js_hp_before - target), kk::f6::g_js_tally[1].native_units += static_cast<std::uint64_t>(native > 0 ? native : 0);
  if (kk::f6::g_js_tally[1].lines < 120) {
    ++kk::f6::g_js_tally[1].lines;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "JUMP_STRIKE event=CONTACT family=LIZALFOS actor=%u generation=%llu swing=%llu episode=%llu cut=%d proc=%d atp=%d native_damage=%d ordinary=%d credit=%d hp=%d->%d "
                  "floor=%d posture=%d at=%s result=NON_LETHAL tick=%llu", s.id, static_cast<unsigned long long>(s.b.generation), static_cast<unsigned long long>(lz.swing_serial),
                  static_cast<unsigned long long>(kk::f6::g_js_episode), k ? k->getCutType() : -1, k ? static_cast<int>(k->mProcID) : -1, s.js_atp, native, ord, s.js_credit_now ? 1 : 0, s.js_hp_before,
                  ac->health, kk::f6::kFloorDn, s.b.p.points(), where, static_cast<unsigned long long>(g.p8.tick));
    liz_line(kLogCut, LOG_LEVEL_INFO, line);
  } else {
    ++kk::f6::g_js_tally[1].dropped;
  }
}
// FIX07 P1: ONE bounded loss per execution of a spin / great spin / native Mortal Draw (1.5x / 2.25x / 2.5x the ordinary effective cut); lethal only when that
// bounded loss takes the last HP (E_DN dies at health <= 0, through big_damage). Returns true when the contact is lethal.
bool liz_sk_settle(LizSlot& s, e_dn_class* a, daAlink_c* k, const char* where) noexcept {
  if (!s.sk_raised) return false;
  s.sk_raised = false;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  const int native = s.sk_hp_before + kk::f7::kHeadroom - ac->health;
  s.sk_credit_now = native > 0 && s.sk_credit.take(lz.swing_serial);
  const int ord = kk::f6::ordinary_of(native, s.sk_atp);
  const int dmg = s.sk_credit_now ? kk::f7::skill_damage(ord, s.sk_kind) : 0;
  const kk::f7::Settle st = kk::f7::settle(s.sk_hp_before, dmg, 0);
  liz_js_hp(a, st.target);
  s.b.sk_call = true, s.b.sk_gain = s.sk_credit_now ? kk::f7::hit_gain(liz::policy::kGainSword, s.sk_kind) : 0, ++s.b.st.f7.sk_calls;
  kk::f7::Tally& ty = kk::f7::g_tally[1];
  ++(s.sk_credit_now ? ty.credited : ty.dupes)[unsigned(s.sk_kind)];
  if (st.lethal) ++ty.lethal[unsigned(s.sk_kind)], ++s.b.st.f7.sk_lethal;
  ty.hp_units += static_cast<std::uint64_t>(s.sk_hp_before - st.target), ty.native_units += static_cast<std::uint64_t>(native > 0 ? native : 0);
  if (ty.lines < 160) {
    ++ty.lines;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "SPECIAL_SKILL event=HP_COMMITTED family=LIZALFOS actor=%u generation=%llu skill=%s cut=%d proc=%d swing=%llu atp=%d native_damage=%d ordinary=%d credit_once=%d hp_before=%d "
                  "hp_after=%d posture=%d broken=0 at=%s result=%s tick=%llu", s.id, static_cast<unsigned long long>(s.b.generation), kk::f7::name(s.sk_kind), k ? k->getCutType() : -1,
                  k ? static_cast<int>(k->mProcID) : -1, static_cast<unsigned long long>(lz.swing_serial), s.sk_atp, native, ord, s.sk_credit_now ? 1 : 0, s.sk_hp_before, ac->health, s.b.p.points(), where,
                  st.lethal ? "LETHAL_LOW_HP" : s.sk_credit_now ? "BOUNDED" : "DUPLICATE_NO_EFFECT", static_cast<unsigned long long>(g.p8.tick));
    liz_line(kLogCut, LOG_LEVEL_INFO, line);
  } else {
    ++ty.dropped;
  }
  return st.lethal;
}
HookAction on_liz_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  bool shield = false;
  if (a->invulnerability_timer == 0) {
    for (int i = 0; i < 3 && !shield; ++i) {
      if (!a->cc_sph[i].cCcD_ObjHitInf::ChkTgHit()) continue;
      const cCcD_Obj* o = a->cc_sph[i].cCcD_ObjHitInf::GetTgHitObj();
      shield = o && o->ChkAtType(AT_TYPE_SHIELD_ATTACK);
    }
  }
  bool sword = false;
  if (a->invulnerability_timer == 0) {
    for (int i = 0; i < 3 && !sword; ++i) {
      if (!a->cc_sph[i].cCcD_ObjHitInf::ChkTgHit()) continue;
      const cCcD_Obj* o = a->cc_sph[i].cCcD_ObjHitInf::GetTgHitObj();
      sword = o && o->ChkAtType(kMasks.sword);
    }
  }
  daAlink_c* k = f1_link();
  liz::View v = liz_view(a, k);
  liz_contact_reads(a, k, v);  // FIX04: always: a contact on the shield alone has its verdict and its trace too
  const liz::Write w = s->b.on_damage_pre(v, shield, sword);
  liz_apply(a, w);
  liz_drain(*s, a);
  // FIX06 P0: a Jump Strike phase (cut 18/19/20) of Link's sword still on a body sphere after the shield's verdict: HP raised by a headroom larger than any
  // native power, so damage_check never takes its lethal branch for it; small/big_damage PRE (or this POST) settles the bounded, credited loss.
  s->js_raised = false;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  if (sword && k && kk::f6::js_cut(k->getCutType()) && a->invulnerability_timer == 0 && ac->health > 0 && !s->b.dead) {
    const cCcD_Obj* jo = nullptr;
    for (int i = 0; i < 3 && !jo; ++i) {
      if (!a->cc_sph[i].cCcD_ObjHitInf::ChkTgHit()) continue;
      const cCcD_Obj* o = a->cc_sph[i].cCcD_ObjHitInf::GetTgHitObj();
      if (o && o->ChkAtType(kMasks.sword) && const_cast<cCcD_Obj*>(o)->GetAc() == static_cast<fopAc_ac_c*>(k)) jo = o;
    }
    if (jo) {
      s->js_raised = true, s->js_hp_before = ac->health, s->js_atp = jo->GetAtAtp();
      liz_js_hp(a, ac->health + kk::f6::kJsHeadroom);
    }
  }
  // FIX07 P1: a spin / great spin / native Mortal Draw of Link's sword still on a body sphere after the shield's verdict (no standing vulnerability): HP raised
  // above any native power (atp 3 -> 60, atp 4 -> 400 with the Master Sword), so neither the native 3x/20x power nor its choice of reaction sees a lethal
  // result; small/big_damage PRE settles ONE bounded loss (lethal only when it takes the last HP: then the native big_damage death route).
  s->sk_raised = false;
  const kk::f7::Sk skind = k ? kk::f7::skill_of(k->getCutType()) : kk::f7::Sk::None;
  if (sword && k && kk::f7::bounded(skind) && s->b.sk_bounded_ok() && a->invulnerability_timer == 0 && ac->health > 0 && !s->js_raised) {
    const cCcD_Obj* so = nullptr;
    for (int i = 0; i < 3 && !so; ++i) {
      if (!a->cc_sph[i].cCcD_ObjHitInf::ChkTgHit()) continue;
      const cCcD_Obj* o = a->cc_sph[i].cCcD_ObjHitInf::GetTgHitObj();
      if (o && o->ChkAtType(kMasks.sword) && const_cast<cCcD_Obj*>(o)->GetAc() == static_cast<fopAc_ac_c*>(k)) so = o;
    }
    if (so) {
      s->sk_raised = true, s->sk_hp_before = ac->health, s->sk_atp = so->GetAtAtp(), s->sk_kind = skind;
      ++kk::f7::g_tally[1].contacts[unsigned(skind)];
      liz_js_hp(a, ac->health + kk::f7::kHeadroom);
    }
  }
  return HOOK_CONTINUE;
}
void on_liz_damage_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  liz_js_settle(*s, a, f1_link(), "POST");  // FIX06: a raised contact neither reaction consumed (hookshot/stun branches restore it): back to the real HP
  s->b.js_call = false, s->b.js_gain = 0;
  if (s->sk_raised) {  // FIX07: the same for a special no reaction consumed: never left raised, never a death outside the native reaction (kept at 1 at least)
    ++s->b.st.f7.sk_post_settle;
    if (liz_sk_settle(*s, a, f1_link(), "POST")) liz_js_hp(a, 1);
  }
  s->b.sk_call = false, s->b.sk_gain = 0;
  const liz::Write w = s->b.on_damage_post(liz_view(a, f1_link()));
  liz_apply(a, w);
  liz_drain(*s, a);
}
liz::HitKind liz_hit_kind(e_dn_class* a, daAlink_c* k) noexcept {
  const cCcD_Obj* o = a->at_info.mpCollider;
  if (!o || !k || a->at_info.mpActor != static_cast<fopAc_ac_c*>(k)) return o ? liz::HitKind::Other : liz::HitKind::None;
  if (o->ChkAtType(kMasks.iron_ball)) return liz::HitKind::IronBall;
  if (o->ChkAtType(kMasks.sword)) return liz::HitKind::Sword;
  return liz::HitKind::Other;
}
HookAction on_liz_small_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  liz_js_settle(*s, a, k, "SMALL");
  if (liz_sk_settle(*s, a, k, "SMALL")) {  // FIX07: a lethal bounded special: E_DN dies only through big_damage (ACTION_DAMAGE): its native route, never S_DAMAGE at 0 HP
    ++s->b.st.f7.sk_small_redirect;
    s->b.sk_call = false, s->b.sk_gain = 0;
    liz_native_break(a);
    liz_drain(*s, a);
    return HOOK_SKIP_ORIGINAL;
  }
  const bool skip = s->b.on_small_pre(liz_view(a, k), liz_hit_kind(a, k));
  s->b.js_call = false, s->b.js_gain = 0;
  s->b.sk_call = false, s->b.sk_gain = 0;
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction on_liz_big_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  liz_js_settle(*s, a, k, "BIG");
  (void)liz_sk_settle(*s, a, k, "BIG");  // FIX07: lethal or not, the native big_damage reads the committed HP (0 = its own death route)
  const bool skip = s->b.on_big_pre(liz_view(a, k), liz_hit_kind(a, k));
  s->b.js_call = false, s->b.js_gain = 0;
  s->b.sk_call = false, s->b.sk_gain = 0;
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction on_liz_run_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  liz::Write w;
  const bool skip = s->b.on_run_pre(liz_view(a, f1_link()), w);
  liz_apply(a, w);
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
void on_liz_run_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted || s->b.ep.active) return;
  s->b.pending_twirl = lz.swing_twirl;  // FIX06 P4
  const liz::Write w = s->b.on_run_post(liz_view(a, f1_link()), lz.swing_jump, liz_link_frame(f1_link()).at_eta);
  liz_apply(a, w);
  liz_drain(*s, a);
}
void on_liz_guard_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  liz_apply(a, s->b.on_guard_post(liz_view(a, f1_link())));
}
// Link's sword against a Lizalfos shield: the rebound is the signal of a real parry, and only of that. The record itself stays intact: sparks, sound, hitstop and the shield's contact go on as native.
void on_cut_reverse_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!liz_alive() || !args || !ret) return;
  auto* r = static_cast<BOOL*>(ret);
  if (!*r) return;
  auto* o = mods::arg<dCcD_GObjInf*>(args, 1);
  if (!o) return;
  const cCcD_Obj* hit = o->cCcD_ObjHitInf::GetAtHitObj();
  if (!hit) return;
  daAlink_c* k = f1_link();
  for (auto& s : lz.slots) {
    if (!s.on || !s.b.admitted) continue;
    auto* a = reinterpret_cast<e_dn_class*>(s.key);
    // This sword's record is on THIS actor (its shield, or a body sphere of the same pass) and this actor's shield really took Link's sword in that pass. Three sword capsules: the Tg side keeps only the last one.
    bool here = hit == static_cast<const cCcD_Obj*>(&a->tate_sph);
    for (int i = 0; i < 3 && !here; ++i) here = hit == static_cast<const cCcD_Obj*>(&a->cc_sph[i]);
    if (!here) continue;
    liz::View v = liz_view(a, k);
    liz_contact_reads(a, k, v);
    if (!v.tate_hit || !v.tate_link_sword) return;
    const std::uint64_t seen = s.b.st.rebound_kept + s.b.st.rebound_suppressed;
    const bool parry = s.b.sword_rebounds(v, lz.swing_serial);
    if (!parry) *r = FALSE;
    if (parry && k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP) kk::f6::g_land_recoil.arm(kk::f6::g_jump_id, g.p8.tick);  // FIX06 P2: its landing recoils natively
    if (k && s.b.st.rebound_kept + s.b.st.rebound_suppressed != seen) {
      ++lz.cuts[k->getCutType() & 63].contacts;
      char cl[kLogLineMax];
      std::snprintf(cl, sizeof(cl), "COMBO_LINK event=REVERSE_CHECK actor=%u generation=%llu swing=%llu cut=%d proc=%d combo=%u frame=%.1f at=%.1f..%.1f rebound=%d tick=%llu", s.id, static_cast<unsigned long long>(s.b.generation), static_cast<unsigned long long>(lz.swing_serial), k->getCutType(), static_cast<int>(k->mProcID), static_cast<unsigned>(k->getCutCount()), k->mUnderFrameCtrl[0].getFrame(), k->field_0x3478, k->field_0x347c, parry ? 1 : 0, static_cast<unsigned long long>(g.p8.tick));
      liz_line(kLogCut, LOG_LEVEL_INFO, cl);
    }
    if (s.b.st.rebound_kept + s.b.st.rebound_suppressed != seen) {
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "LIZ_DEFLECT event=SWORD_%s actor=%u generation=%llu swing=%llu mode=%d tick=%llu", parry ? "REBOUND_NATIVE" : "NO_REBOUND_ORDINARY_BLOCK", s.id, static_cast<unsigned long long>(s.b.generation), static_cast<unsigned long long>(lz.swing_serial), a->mode, static_cast<unsigned long long>(g.p8.tick));
      liz_line(kLogDeflect, LOG_LEVEL_INFO, line);
    }
    return;
  }
}
HookAction on_liz_attack_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  liz_apply(a, s->b.on_attack_pre(liz_view(a, f1_link())));
  return HOOK_CONTINUE;
}
void on_liz_attack_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  liz_apply(a, s->b.on_attack_post(liz_view(a, f1_link())));
  liz_drain(*s, a);
}
void on_liz_at_hit_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!liz_alive() || !args || !ret) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  auto** r = static_cast<fopAc_ac_c**>(ret);
  const bool is_link = *r && fopAcM_GetName(*r) == fpcNm_ALINK_e;
  if (s->b.on_at_hit_post(liz_view(a, f1_link()), is_link)) *r = nullptr;
}
HookAction on_liz_anm_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  float& rate = mods::arg_ref<float>(args, 4);
  (void)s->b.on_anm_pre(mods::arg<int>(args, 1), rate);
  return HOOK_CONTINUE;
}
void on_liz_anm_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  float frame = 0;
  if (s->b.on_anm_post(mods::arg<int>(args, 1), frame) && a->anm_p) liz_native_anim(a->anm_p, true, frame, false, 0.0f);
  liz_drain(*s, a);
}
void on_liz_cut_post(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return;
  auto* k = mods::arg<daAlink_c*>(args, 0);
  const int t = mods::arg<u8>(args, 1);
  if (!k || k != f1_link() || k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)) return;
  if (t == daPy_py_c::CUT_TYPE_NONE || t == daPy_py_c::CUT_TYPE_GUARD_ATTACK) return;  // the Shield Attack is not a sword swing
  // LFT 0.13.5: procCutJumpLandInit calls setCutType(CUT_TYPE_JUMP) again; that landing is the SAME Jump Attack (one serial, one decision, one gain)
  const bool jump = t == daPy_py_c::CUT_TYPE_JUMP, jump_air = jump && k->mProcID == daAlink_c::PROC_CUT_JUMP;
  if (jump && k->mProcID == daAlink_c::PROC_CUT_JUMP_LAND && lz.jump_open && lz.jump_serial == lz.swing_serial) {
    lz.jump_open = false, ++lz.jump_land_merged;
    return;
  }
  if (lz.swing_tick == g.p8.tick) return;                                             // one distinct swing per tick
  lz.swing_tick = g.p8.tick;
  ++lz.swing_serial;
  lz.swing_jump = jump;
  lz.swing_twirl = t == kk::f6::kCutTwirl;  // FIX06 P4
  lz.jump_open = jump_air, lz.jump_serial = lz.swing_serial;
  bool fighting = false;  // the budget is for swings near a Lizalfos, not for every enemy of the session
  for (const auto& s : lz.slots) fighting = fighting || (s.on && s.b.admitted);
  if (!fighting) return;
  ++lz.cuts[t & 63].swings;
  char line[kLogLineMax];  // FIX05: the real combo as Link's clip runs it: cut, proc, position in the combo, and whether the press was buffered in the previous cut
  std::snprintf(line, sizeof(line), "COMBO_LINK event=SWING serial=%llu cut=%d proc=%d combo=%u buffered=%d tick=%llu", static_cast<unsigned long long>(lz.swing_serial), t, static_cast<int>(k->mProcID), static_cast<unsigned>(k->getCutCount()), k->checkNoResetFlg2(daPy_py_c::FLG2_COMBO_RESERB) ? 1 : 0, static_cast<unsigned long long>(g.p8.tick));
  liz_line(kLogCut, LOG_LEVEL_INFO, line);
}
// The record's own AT object is THIS admitted actor's at_sph: pointer identity against live slots, nothing is dereferenced on the record's side.
LizSlot* liz_slot_of_at(const cCcD_Obj* o) noexcept {
  if (!o) return nullptr;
  for (auto& s : lz.slots)
    if (s.on && s.b.admitted && o == static_cast<const cCcD_Obj*>(&reinterpret_cast<e_dn_class*>(s.key)->at_sph)) return &s;
  return nullptr;
}
// d_a_alink_damage.inc:629 drops every Tg record while Link is ALREADY in GUARD_SLIP. PRE: remember that state for the POST of this same call.
HookAction on_link_damage_pre_liz(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return HOOK_CONTINUE;
  auto* link = mods::arg<daAlink_c*>(args, 0);
  lz.slip_pre = link && link == f1_link() && link->mProcID == daAlink_c::PROC_GUARD_SLIP;
  lz.slip_pre_tick = g.p8.tick;
  return HOOK_CONTINUE;
}
// The one contact native dropped: a REAL shield record of an admitted Lizalfos in the last collision pass, Link still protected, one per impact. Native functions and native values only.
void liz_slip_feedback(daAlink_c* link) noexcept {
  if (!lz.slip_pre || lz.slip_pre_tick != g.p8.tick || link->mProcID != daAlink_c::PROC_GUARD_SLIP || link->mDamageTimer != 0 || !cp04_world_safe()) return;
  for (int i = 0; i < 3; ++i) {
    auto& c = link->mTgCyls[i];
    if (!c.cCcD_ObjHitInf::ChkTgHit() || !c.ChkTgShieldHit()) continue;
    LizSlot* s = liz_slot_of_at(c.cCcD_ObjHitInf::GetTgHitObj());
    if (!s) continue;
    if (s->b.feedback_impact == s->b.impact_counter) return;  // a second record of the same swing never sounds twice
    s->b.feedback_impact = s->b.impact_counter;
    liz_native_link_feedback(link, &c);
    ++lz.se_restored;
    if (s->b.imp.open) ++s->b.imp.feedback_restored;
    else if (s->b.last_imp.impact == s->b.ep.impact) ++s->b.last_imp.feedback_restored;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "LIZ_FEEDBACK event=NATIVE_GUARD_SE_RESTORED_DURING_SLIP actor=%u generation=%llu impact_serial=%llu pattern=%d impact=%d tick=%llu speed=%.1f runtime=HUMAN_RUNTIME_PENDING", s->id, static_cast<unsigned long long>(s->b.generation),
                  static_cast<unsigned long long>(s->b.impact_counter), s->b.ep.pattern + 1, s->b.ep.impact, static_cast<unsigned long long>(g.p8.tick), link->mNormalSpeed);
    liz_line(kLogFeedback, LOG_LEVEL_INFO, line);
    return;
  }
}
// Receipt of Link's own guard SE: the only observable proof of audio this module has. It plays nothing.
void on_guard_se_post(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return;
  auto* o = mods::arg<dCcD_GObjInf*>(args, 1);
  LizSlot* s = o ? liz_slot_of_at(o->cCcD_ObjHitInf::GetTgHitObj()) : nullptr;
  if (!s) return;
  ++lz.se_native;
  if (s->b.imp.open) ++s->b.imp.feedback_native;
}
// Link's native damage receipt for THIS module's direct hits + the only relaxation of the shield re-arm: between two impacts of one episode, and only for a new real press.
void on_link_damage_post_liz(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return;
  auto* link = mods::arg<daAlink_c*>(args, 0);
  if (link) liz_slip_feedback(link);
  if (fx.native.valid && fx.native.link_key == reinterpret_cast<std::uintptr_t>(link) && !fx.native_reentered && link == dComIfGp_getLinkPlayer()) {
    auto n = fx.native;
    n.timer_after = link->mDamageTimer;
    n.life_after = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife();
    n.event = n.event || g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
    n.pending_after = g_dComIfG_gameInfo.play.getItemLifeCount();
    n.electric = link->checkResetFlg1(daPy_py_c::RFLG1_UNK_2) != 0;
    lz.direct.capture(n);
  }
  if (!c4.enabled || lz.double_gap_tick == ~0ull || g.p8.tick > lz.double_gap_tick + 1) return;
  const bool safe = cp04_link_safe(link) && link->mEquipItem == 0x103;
  if (!safe) return;
  const bool slip = link->mProcID == daAlink_c::PROC_GUARD_SLIP && link->mProcVar1.field_0x300a == 0 && g.p8.tick >= c4.shield.permit_tick + liz::policy::kDoubleRearmTicks;
  if (slip && c4.shield.can_restart(g.p8.tick, safe, true)) {
    if (cp04_native_shield_restart(link)) {
      ++c4.shield.c.rearms;
      ++lz.double_rearms;
      cp04_log("SHIELD_CHAIN_ARMED", nullptr, "LIZ_DOUBLE_GAP_NATIVE_RESTART_READBACK");
    } else {
      ++c4.shield.c.failed_native;
      c4.shield.cancel();
    }
  }
}
// ---------------------------------------------------------------- GENERAL: +50% of Link's real damage while his posture break really runs. No actor kind anywhere below.
struct LinkBreakDamage {
  bool enabled = false, in_episode = false;
  lnkdmg::Counters c{};
  std::uint64_t last_applied_tick = ~0ull;
  unsigned lines = 0;
  std::uint64_t dropped = 0;
} ld;
bool ld_episode() noexcept { return lf.enabled && lf.def.extended() && (lf.def.phase() == lnkfx::Phase::NativeBreak || lf.def.phase() == lnkfx::Phase::PostVulnerable); }
void ld_line(const char* line) noexcept {
  if (lnk_take(ld.lines, ld.dropped, 300)) lnk_write(LOG_LEVEL_INFO, line);
}
HookAction on_link_set_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!ld.enabled || !args || !ld_episode()) return HOOK_CONTINUE;
  auto* link = mods::arg<daAlink_c*>(args, 0);
  if (!link || link != f1_link()) return HOOK_CONTINUE;
  int& amount = mods::arg_ref<int>(args, 1);
  lnkdmg::In in;
  in.episode = true, in.amount = amount, in.tick = g.p8.tick, in.last_applied_tick = ld.last_applied_tick;
  in.human = !link->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);
  in.alive = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() > 0;
  in.event = g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
  in.armor_no_hp = link->checkMagicArmorNoDamage() != 0;
  const auto& n = fx.native;
  in.in_action = fx.native_depth == 1 && !fx.native_reentered && n.valid && n.human && n.tick == g.p8.tick;
  in.poly_clear = n.poly_seen && n.poly_result == 0;
  in.has_record = n.cylinder >= 0 && n.cylinder < 3;
  in.blocked = n.blocked;
  std::uint32_t attacker = 0;
  int attacker_name = -1;
  if (in.has_record) {
    cCcD_Obj* o = link->mTgCyls[n.cylinder].cCcD_ObjHitInf::GetTgHitObj();
    fopAc_ac_c* ac = o ? o->GetAc() : nullptr;
    if (ac) {
      in.attacker_known = true;
      attacker = static_cast<std::uint32_t>(fopAcM_GetID(ac)), attacker_name = fopAcM_GetName(ac);
      in.attacker_enemy = fopAcM_GetGroup(ac) == fopAc_ENEMY_e;
      if (!in.attacker_enemy) {  // a projectile: hostile only if the actor that spawned it is an enemy (Link's own bomb is not)
        fopAc_ac_c* parent = nullptr;
        if (fopAcM_SearchByID(fopAcM_GetLinkId(ac), &parent) && parent) in.parent_enemy = fopAcM_GetGroup(parent) == fopAc_ENEMY_e;
      }
    }
  }
  const lnkdmg::Out o = lnkdmg::decide(in);
  ++ld.c.why[unsigned(o.why) < unsigned(lnkdmg::Why::Count) ? unsigned(o.why) : 0];
  if (o.why == lnkdmg::Why::Applied) {
    ld.c.base_units += static_cast<std::uint64_t>(amount), ld.c.extra_units += static_cast<std::uint64_t>(o.extra), ++ld.c.applied;
    ld.last_applied_tick = g.p8.tick;
    amount += o.extra;  // the native argument, before the native code: magnification, armor, pending life and invulnerability stay native
  }
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LINK_BREAK_DAMAGE event=%s base=%d extra=%d final=%d attacker=%u name=%d origin=%s phase=%s life=%d tick=%llu runtime=HUMAN_RUNTIME_PENDING", lnkdmg::name(o.why), in.amount, o.extra, in.amount + o.extra, attacker, attacker_name,
                lnkfx::name(lf.def.origin()), lnkfx::name(lf.def.phase()), g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife(), static_cast<unsigned long long>(g.p8.tick));
  ld_line(line);
  return HOOK_CONTINUE;
}
void ld_maintenance() noexcept {
  if (!ld.enabled) return;
  const bool e = ld_episode();
  if (e == ld.in_episode) return;
  ld.in_episode = e;
  if (e) ++ld.c.episodes;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LINK_BREAK_DAMAGE event=EPISODE_%s origin=%s phase=%s tick=%llu", e ? "BEGIN" : "END", lnkfx::name(lf.def.origin()), lnkfx::name(lf.def.phase()), static_cast<unsigned long long>(g.p8.tick));
  ld_line(line);
}
// ---------------------------------------------------------------- once per mod_update
void liz_maintenance() noexcept {
  ld_maintenance();
  if (!lz.enabled) return;
  const std::uint32_t st = stage_token(dComIfGp_getStartStageName());
  if (st != lz.stage) {  // stage change: no pointer is ever dereferenced again, no state survives
    lz.stage = st;
    for (auto& s : lz.slots) liz_retire(s);
    lz.break_retry_until = 0;
  }
  daAlink_c* k = f1_link();
  const std::uint32_t link_id = k ? static_cast<std::uint32_t>(fopAcM_GetID(k)) : 0;
  for (unsigned i = 0; i < liz::kSlots; ++i) {
    const auto r = lz.direct.take(i, g.p8.tick, k != nullptr, link_id, lg.sim_active);
    if (r.present && r.matched) {
      const int pts = r.k.atp == 2 ? liz::policy::kLinkDirectAtp2 : liz::policy::kLinkDirectAtp1;
      LizSlot& s = lz.slots[i];
      if (s.on && s.id == r.k.id) liz_link_gain(s, pts, lnk05::Kind::Direct, r.k.token);
    }
  }
  if ((g.p8.tick % 9000u) == 0 && lz.admitted) liz_summary();  // every ~5 min of simulation, never per frame
  if (lz.break_retry_until && g.p8.tick <= lz.break_retry_until && (g.p8.tick & 7u) == 0 && lg.posture.milli() >= lnk01::policy::kMaxMilli && lb.m.state() == lnk05::State::Ready) {
    ++lz.break_retries;
    lnk05_request(lnk05::Kind::Direct, lz.retry_attacker, lz.retry_generation, 1, lnk01::policy::kMaxMilli, lnk01::policy::kMaxMilli);
  }
}
// HUD: same 0..100 bar of the other enemies, top hud slots (a Bokoblin ledger slot always wins; then no bar and a counter). Display only. The two lists are
// walked by value; the Delete hook and the stage reset guarantee a slot never outlives its actor.
void liz_hud_samples() noexcept {
  if (!lz.enabled || g.eng.obs.disabled) return;
  auto* player = g_dComIfG_gameInfo.play.getPlayer(0);
  if (!player) return;
  for (unsigned i = 0; i < liz::kSlots; ++i) {
    LizSlot& s = lz.slots[i];
    if (!s.on || !s.b.admitted) continue;
    auto* act = reinterpret_cast<fopAc_ac_c*>(s.key);
    const unsigned h = post07::kSlots - 1u - i;
    if (hg.samples[h].in.present) {
      ++lz.hud_slot_busy;
      continue;
    }
    ehz::DrawEntry life;
    if (!hg.mirror.tracker.presented({s.key, s.id, static_cast<std::int16_t>(fpcNm_E_DN_e)}, life)) {
      ++lz.hud_missing;
      continue;
    }
    post07::Sample& smp = hg.samples[h];
    smp.in.present = true, smp.in.id = s.id, smp.in.generation = (1ull << 40) + s.b.generation;
    smp.in.posture = s.b.p.points(), smp.in.latched = s.b.p.latched, smp.in.dead = s.b.dead || act->health <= 0, smp.in.frozen = false;
    smp.in.phase = smp.in.dead ? post07::Ph::Dead : s.b.phase == liz::Brain::Bk::Punish ? post07::Ph::Punish : s.b.phase == liz::Brain::Bk::Falling ? post07::Ph::Falling : s.b.phase == liz::Brain::Bk::Down ? post07::Ph::Down : s.b.phase == liz::Brain::Bk::GettingUp ? post07::Ph::GettingUp : post07::Ph::Tracking;
    smp.in.opacity = life.alpha;
    smp.anchor = {life.anchor.x, life.anchor.y, life.anchor.z};
    smp.distance = fopAcM_searchActorDistance(player, act);
    hg.slot_interp[h] = life.key;
    ++hg.aligned_samples;
    ++lz.hud_shown;
  }
}
void liz_summary() noexcept {
  liz::Stats t = lz.totals;
  std::uint64_t rolls[3] = {lz.rolls[0], lz.rolls[1], lz.rolls[2]}, atts[3] = {lz.attempts[0], lz.attempts[1], lz.attempts[2]};
  for (auto& s : lz.slots)
    if (s.on && s.b.admitted) liz_merge(t, rolls, atts, s.b);
  char line[kLogLineMax * 4];
  std::snprintf(line, sizeof(line),
                "LIZ_SUMMARY version=%s session=1 admitted=%llu excluded=%llu deleted=%llu overflow=%llu selected=%llu/%llu/%llu/%llu completed=%llu/%llu/%llu/%llu windows=%llu parry=%llu dpair=%llu direct=%llu guard=%llu sword=%llu iron=%llu "
                "blocks=%llu enemy_parry=%llu cancels_suppressed=%llu stagger_small=%llu stagger_big=%llu bash=%llu rolls=%llu/%llu/%llu attempts=%llu/%llu/%llu guards=%llu/%llu/%llu end_posture=%llu end_native=%llu end_adaptive=%llu watchdog=%llu sample_gap=%llu "
                "link_gain=%llu link_sat=%llu break_retries=%llu double_rearms=%llu hud=%llu/%llu/%llu",
                liz::kVersion, static_cast<unsigned long long>(lz.admitted), static_cast<unsigned long long>(lz.excluded), static_cast<unsigned long long>(lz.deleted), static_cast<unsigned long long>(lz.overflow),
                static_cast<unsigned long long>(t.selected[0]), static_cast<unsigned long long>(t.selected[1]), static_cast<unsigned long long>(t.selected[2]), static_cast<unsigned long long>(t.selected[3]),
                static_cast<unsigned long long>(t.completed[0]), static_cast<unsigned long long>(t.completed[1]), static_cast<unsigned long long>(t.completed[2]), static_cast<unsigned long long>(t.completed[3]),
                static_cast<unsigned long long>(t.windows), static_cast<unsigned long long>(t.link_parry), static_cast<unsigned long long>(t.d_pair_parry), static_cast<unsigned long long>(t.contact[unsigned(liz::Hit::Direct)]),
                static_cast<unsigned long long>(t.contact[unsigned(liz::Hit::Guard)]), static_cast<unsigned long long>(t.sword_hits), static_cast<unsigned long long>(t.iron_hits), static_cast<unsigned long long>(t.blocks),
                static_cast<unsigned long long>(t.enemy_parries), static_cast<unsigned long long>(t.cancels_suppressed), static_cast<unsigned long long>(t.stagger_suppressed_small), static_cast<unsigned long long>(t.stagger_suppressed_big),
                static_cast<unsigned long long>(t.shield_bash_cleared), static_cast<unsigned long long>(rolls[0]), static_cast<unsigned long long>(rolls[1]), static_cast<unsigned long long>(rolls[2]), static_cast<unsigned long long>(atts[0]),
                static_cast<unsigned long long>(atts[1]), static_cast<unsigned long long>(atts[2]), static_cast<unsigned long long>(t.guards_native), static_cast<unsigned long long>(t.guards_attempt), static_cast<unsigned long long>(t.guards_boundary),
                static_cast<unsigned long long>(t.ended[unsigned(liz::End::Posture100)]), static_cast<unsigned long long>(t.ended[unsigned(liz::End::NativePriority)]), static_cast<unsigned long long>(t.ended[unsigned(liz::End::AdaptiveDefense)]),
                static_cast<unsigned long long>(t.watchdog), static_cast<unsigned long long>(t.sample_gap), static_cast<unsigned long long>(lz.link_gain), static_cast<unsigned long long>(lz.link_saturations),
                static_cast<unsigned long long>(lz.break_retries), static_cast<unsigned long long>(lz.double_rearms), static_cast<unsigned long long>(lz.hud_shown), static_cast<unsigned long long>(lz.hud_missing),
                static_cast<unsigned long long>(lz.hud_slot_busy));
  lnk_write(LOG_LEVEL_INFO, line);
  const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
  std::snprintf(line, sizeof(line),
                "LIZ_SUMMARY part=FIX02 version=%s swings_detected=%llu pressure_promotions=%llu wounds=%llu sword_duplicates=%llu guard_rolls=%llu/%llu/%llu guard_intents=%llu/%llu/%llu guard_enters=%llu guard_intent_lost=%llu block_pairs=%llu deflect_roll=%llu deflect_armed=%llu deflect_windows=%llu deflect_expired=%llu shield_won_body=%llu block_rear_rejected=%llu rebound_kept=%llu rebound_suppressed=%llu counter_selected=%llu/%llu counter_started=%llu/%llu counter_AT_open=%llu counter_contact=%llu counter_expired=%llu "
                "break_standing=%llu helm_prepared=%llu decisive_sword=%llu decisive_helm=%llu break_bonus_hits=%llu break_bonus_units=%llu break_bonus_lethal=%llu down_finish=%llu getup=%llu punish_end=%llu native_guard=%llu AT_SAMPLE_GAP=%llu collision_gap_fixed=%llu expect_ticks=%llu lost_ticks=%llu no_continuous=%llu immune_touch=%llu "
                "impacts_whiff=%llu impacts_guard=%llu impacts_parry=%llu impacts_body=%llu impacts_immune=%llu impacts_missing=%llu native_feedback_receipt=%llu audio_restored_during_slip=%llu no_receipt=%llu bash_se=%llu gap_arms=%llu zero_vec=%llu probes=%llu probe_mismatch=%llu coverage=%d watchdog=%llu",
                liz::kVersion, u(t.swings_detected), u(lz.pressure_promotions(t)), u(t.wounds), u(t.sword_duplicates), u(t.guard_rolls[0]), u(t.guard_rolls[1]), u(t.guard_rolls[2]), u(t.guard_intents[0]), u(t.guard_intents[1]), u(t.guard_intents[2]), u(t.guard_enters), u(t.guard_intent_lost), u(t.block_pairs), u(t.deflect_rolls), u(t.deflect_armed), u(t.deflect_windows), u(t.deflect_expired), u(t.shield_won_body), u(t.block_rear_rejected), u(t.rebound_kept), u(t.rebound_suppressed), u(t.counter_selected[0]), u(t.counter_selected[1]), u(t.counter_started[0]), u(t.counter_started[1]), u(t.counter_at_open), u(t.counter_contact), u(t.counter_expired),
                u(t.break_standing), u(t.helm_prepared), u(t.decisive_sword), u(t.decisive_helm), u(t.break_bonus_hits), u(t.break_bonus_units), u(t.break_bonus_lethal), u(t.down_finish), u(t.getup), u(t.punish_end), u(t.guards_native), u(t.sample_gap), u(t.gap_fixed), u(t.expect_ticks), u(t.lost_ticks), u(t.nocon_ticks), u(t.off_touch_ticks),
                u(t.impact_class[unsigned(liz::Cls::Whiff)]), u(t.impact_class[unsigned(liz::Cls::LinkGuard)]), u(t.impact_class[unsigned(liz::Cls::LinkParry)]), u(t.impact_class[unsigned(liz::Cls::LinkBody)]), u(t.impact_class[unsigned(liz::Cls::ImmuneValid)]), u(t.impact_class[unsigned(liz::Cls::MissingFeedback)]),
                u(lz.se_native), u(lz.se_restored), u(lz.no_receipt), u(lz.bash_se), u(lz.gap_arms), u(lz.zero_vec), u(lz.probes), u(lz.probe_mismatch), lz.coverage ? 1 : 0, u(t.watchdog));
  lnk_write(LOG_LEVEL_INFO, line);
  // FIX03: the shield chain is a Link mechanism (counters of the general chain, not of this module) and the shield geometry of the guard.
  std::snprintf(line, sizeof(line),
                "LIZ_SUMMARY part=FIX03 version=%s chain_input_new=%llu chain_buffered=%llu chain_rearms=%llu chain_matched=%llu chain_cancelled=%llu chain_expired=%llu double_rearms=%llu gap_arms=%llu dpair=%llu "
                "shield_won_body=%llu shield_not_covering=%llu block_rear_rejected=%llu guard_cover_ticks=%llu guard_open_ticks=%llu guard_turn_ticks=%llu runtime=HUMAN_RUNTIME_PENDING",
                liz::kVersion, u(c4.shield.c.input_new), u(c4.shield.c.buffered), u(c4.shield.c.rearms), u(c4.shield.c.matched), u(c4.shield.c.cancelled), u(c4.shield.c.expired), u(lz.double_rearms), u(lz.gap_arms), u(t.d_pair_parry),
                u(t.shield_won_body), u(t.shield_not_covering), u(t.block_rear_rejected), u(t.guard_cover_ticks), u(t.guard_open_ticks), u(t.guard_turn_ticks));
  lnk_write(LOG_LEVEL_INFO, line);
  // FIX04: the two-block chain end to end, and one final verdict per contact.
  std::snprintf(line, sizeof(line),
                "LIZ_SUMMARY part=FIX04 version=%s blocks=%llu pairs=%llu rolls80=%llu prepared=%llu new_windows=%llu prepared_expired=%llu enemy_parry=%llu rebound_kept=%llu counter_selected=%llu/%llu counter_started=%llu/%llu counter_at_open=%llu counter_contact=%llu "
                "trace_block=%llu trace_parry=%llu trace_uncovered=%llu trace_body=%llu trace_shield_won=%llu trace_repeat=%llu trace_lines_dropped=%llu runtime=HUMAN_RUNTIME_PENDING",
                liz::kVersion, u(t.blocks), u(t.block_pairs), u(t.deflect_rolls), u(t.deflect_armed), u(t.deflect_windows), u(t.deflect_expired), u(t.enemy_parries), u(t.rebound_kept), u(t.counter_selected[0]), u(t.counter_selected[1]), u(t.counter_started[0]), u(t.counter_started[1]), u(t.counter_at_open), u(t.counter_contact),
                u(t.trace_verdict[unsigned(liz::Tv::Block)]), u(t.trace_verdict[unsigned(liz::Tv::EnemyParry)]), u(t.trace_verdict[unsigned(liz::Tv::Uncovered)]), u(t.trace_verdict[unsigned(liz::Tv::BodyDamage)]), u(t.trace_verdict[unsigned(liz::Tv::ShieldWonBody)]), u(t.trace_verdict[unsigned(liz::Tv::Repeat)]), u(lz.dropped[kLogTrace]));
  lnk_write(LOG_LEVEL_INFO, line);
  // FIX05: why no parry happened, contact by contact, and which native cut each outcome belongs to. Truncated telemetry is declared.
  std::snprintf(line, sizeof(line),
                "LIZ_SUMMARY part=FIX05 version=%s parry=%llu chain=%llu rng=%llu cooldown=%llu token_waiting=%llu flank=%llu not_presented=%llu early=%llu late=%llu no_rebound=%llu conflict=%llu represent_synced=%llu represent_unknown=%llu deflect_waits=%llu deflect_recovered=%llu entry_unreachable=%llu rebound_state_ok=%llu rebound_state_other=%llu dropped_cut=%llu dropped_response=%llu dropped_trace=%llu runtime=HUMAN_RUNTIME_PENDING",
                liz::kVersion, u(t.no_parry[unsigned(liz::Np::None)]), u(t.no_parry[unsigned(liz::Np::Chain)]), u(t.no_parry[unsigned(liz::Np::Rng)]), u(t.no_parry[unsigned(liz::Np::Cooldown)]), u(t.no_parry[unsigned(liz::Np::TokenWaiting)]), u(t.no_parry[unsigned(liz::Np::Flank)]), u(t.no_parry[unsigned(liz::Np::NotPresented)]), u(t.no_parry[unsigned(liz::Np::Early)]), u(t.no_parry[unsigned(liz::Np::Late)]), u(t.no_parry[unsigned(liz::Np::NoRebound)]), u(t.no_parry[unsigned(liz::Np::Conflict)]), u(t.represent_synced), u(t.represent_unknown), u(t.deflect_waits), u(t.deflect_recovered), u(t.entry_unreachable), u(lz.rebound_state_ok), u(lz.rebound_state_other), u(lz.dropped[kLogCut]), u(lz.dropped[kLogResponse]), u(lz.dropped[kLogTrace]));
  lnk_write(LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line), "LIZ_FIX06_SUMMARY version=0.13.6 js_contacts=%llu js_credited=%llu js_dupes=%llu js_floored=%llu js_hp_units=%llu js_native_units=%llu js_calls=%llu js_posture_credits=%llu "
                "entry_cover=%llu jump_floor_rolls=%llu jump_floor_hits=%llu chain=%llu/%llu/%llu/%llu/%llu land_recoil_armed=%llu dropped_js=%llu telemetry=MODULE_EXECUTION_COUNTERS runtime=HUMAN_RUNTIME_PENDING",
                u(kk::f6::g_js_tally[1].contacts), u(kk::f6::g_js_tally[1].credited), u(kk::f6::g_js_tally[1].dupes), u(kk::f6::g_js_tally[1].floored), u(kk::f6::g_js_tally[1].hp_units), u(kk::f6::g_js_tally[1].native_units), u(t.f6.js_calls), u(t.f6.js_credited), u(t.f6.entry_cover), u(t.f6.jump_floor_rolls),
                u(t.f6.jump_floor_hits), u(t.f6.chain_rolls), u(t.f6.chain_scheduled), u(t.f6.chain_started), u(t.f6.chain_no), u(t.f6.chain_cancelled), u(kk::f6::g_land_recoil.armed_n), u(kk::f6::g_js_tally[1].dropped));
  lnk_write(LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line), "LIZ_FIX07_SUMMARY version=0.13.7 sk_swings=%llu pre_guard_rolls=%llu pre_guards=%llu entry_cover=%llu entry_frames_cover=%llu parry_rolls=%llu parry_armed=%llu "
                "parry_verdicts=%llu parry_no_route=%llu sk_blocks=%llu sk_calls=%llu sk_credited=%llu sk_lethal=%llu small_redirect=%llu post_settle=%llu defended_hp_leak=%llu dropped_sk=%llu "
                "telemetry=MODULE_EXECUTION_COUNTERS runtime=HUMAN_RUNTIME_PENDING",
                u(t.f7.sk_swings), u(t.f7.pre_guard_rolls), u(t.f7.pre_guards), u(t.f7.entry_cover), u(t.f7.entry_frames_cover), u(t.f7.parry_rolls), u(t.f7.parry_armed), u(t.f7.parry_verdicts),
                u(t.f7.parry_no_route), u(t.f7.sk_blocks), u(t.f7.sk_calls), u(t.f7.sk_credited), u(t.f7.sk_lethal), u(t.f7.sk_small_redirect), u(t.f7.sk_post_settle), u(kk::f7::g_tally[1].leak),
                u(kk::f7::g_tally[1].dropped));
  lnk_write(LOG_LEVEL_INFO, line);
  {
    int cn = std::snprintf(line, sizeof(line), "LIZ_SUMMARY part=FIX05_CUTS version=%s swings/contacts/parries", liz::kVersion);
    for (unsigned c = 0; c < 64 && cn > 0 && cn < int(sizeof(line)) - 48; ++c)
      if (lz.cuts[c].swings || lz.cuts[c].contacts) cn += std::snprintf(line + cn, sizeof(line) - unsigned(cn), " cut%u=%llu/%llu/%llu", c, u(lz.cuts[c].swings), u(lz.cuts[c].contacts), u(lz.cuts[c].parries));
    if (cn > 0 && cn < int(sizeof(line)) - 48) std::snprintf(line + cn, sizeof(line) - unsigned(cn), " jump_land_merged=%llu lft=0.13.7", u(lz.jump_land_merged));  // LFT 0.13.6 (0.13.7: version only)
    lnk_write(LOG_LEVEL_INFO, line);
  }
  std::snprintf(line, sizeof(line), "LINK_BREAK_DAMAGE event=SUMMARY enabled=%d episodes=%llu applied=%llu base_units=%llu extra_units=%llu skip_no_damage=%llu skip_link_state=%llu skip_armor=%llu skip_unattributed=%llu skip_environment=%llu skip_blocked=%llu skip_not_hostile=%llu skip_duplicate=%llu lines_dropped=%llu runtime=HUMAN_RUNTIME_PENDING", ld.enabled ? 1 : 0, u(ld.c.episodes), u(ld.c.applied), u(ld.c.base_units), u(ld.c.extra_units),
                u(ld.c.why[unsigned(lnkdmg::Why::NoDamage)]), u(ld.c.why[unsigned(lnkdmg::Why::LinkState)]), u(ld.c.why[unsigned(lnkdmg::Why::ArmorNoHp)]), u(ld.c.why[unsigned(lnkdmg::Why::Unattributed)]), u(ld.c.why[unsigned(lnkdmg::Why::Environment)]), u(ld.c.why[unsigned(lnkdmg::Why::Blocked)]), u(ld.c.why[unsigned(lnkdmg::Why::NotHostile)]), u(ld.c.why[unsigned(lnkdmg::Why::Duplicate)]), u(ld.dropped));
  lnk_write(LOG_LEVEL_INFO, line);
  int n = std::snprintf(line, sizeof(line), "LIZ_SUMMARY part=PARRY_BY_IMPACT version=%s", liz::kVersion);
  for (unsigned p = 0; p < liz::kPatterns && n > 0 && n < int(sizeof(line)) - 96; ++p) {
    n += std::snprintf(line + n, sizeof(line) - unsigned(n), " P%u=", p + 1);
    for (unsigned i = 0; i < liz::kPattern[p].impacts && n > 0 && n < int(sizeof(line)) - 32; ++i) n += std::snprintf(line + n, sizeof(line) - unsigned(n), "%s%llu", i ? "/" : "", u(t.link_parry_by_impact[p][i]));
  }
  for (unsigned w = 1; w < unsigned(liz::sweep::Why::Count) && n > 0 && n < int(sizeof(line)) - 64; ++w)
    if (lz.why[w]) n += std::snprintf(line + n, sizeof(line) - unsigned(n), " %s=%llu", liz::sweep::name(liz::sweep::Why(w)), u(lz.why[w]));
  lnk_write(LOG_LEVEL_INFO, line);
}
void liz_shutdown() noexcept {  // callbacks inert from here on; nothing of this module persists in the game
  liz_summary();
  lz.enabled = false;
}
void liz_install() noexcept {
  HookOptions late = HOOK_OPTIONS_INIT;
  late.priority = -400;  // after every existing Link damage observer (FIX01 receipt, CP04 re-arm)
  ModResult r[] = {
      mods::hook::add_pre<LizExec>(on_liz_exec_pre),     mods::hook::add_post<LizExec>(on_liz_exec_post),   mods::hook::add_pre<LizDelete>(on_liz_delete_pre),
      mods::hook::add_pre<LizDamage>(on_liz_damage_pre), mods::hook::add_post<LizDamage>(on_liz_damage_post), mods::hook::add_pre<LizSmall>(on_liz_small_pre),
      mods::hook::add_pre<LizBig>(on_liz_big_pre),       mods::hook::add_pre<LizRun>(on_liz_run_pre),       mods::hook::add_post<LizRun>(on_liz_run_post),
      mods::hook::add_pre<LizAttack0>(on_liz_attack_pre), mods::hook::add_post<LizAttack0>(on_liz_attack_post), mods::hook::add_pre<LizAttack>(on_liz_attack_pre),
      mods::hook::add_post<LizAttack>(on_liz_attack_post), mods::hook::add_pre<LizTail>(on_liz_attack_pre),   mods::hook::add_post<LizTail>(on_liz_attack_post),
      mods::hook::add_post<LizAtHit>(on_liz_at_hit_post), mods::hook::add_pre<LizAnm>(on_liz_anm_pre),       mods::hook::add_post<LizAnm>(on_liz_anm_post),
      mods::hook::add_post<LinkCutType>(on_liz_cut_post), mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_liz, &late), mods::hook::add_pre<LinkDamageFix01>(on_link_damage_pre_liz),
      mods::hook::add_post<LinkGuardSe>(on_guard_se_post), mods::hook::add_post<LizGuard>(on_liz_guard_post), mods::hook::add_post<LinkCutReverse>(on_cut_reverse_post)};
  bool ok = c4.enabled;
  unsigned failed = 0;
  for (auto x : r)
    if (x != MOD_OK) {
      ok = false;
      ++failed;
    }
  lz.coverage = ok;
  lz.enabled = ok;  // all or nothing: without every hook the E_DN is vanilla and the rest of the mod is untouched (a callback with lz.enabled=false returns at once)
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LIZ_POSTURE event=INSTALL version=%s enabled=%d hooks=%u failed=%u hp=%d capacity=%d gains=6/12/30/5/25 parry_self=0 regen=10/7/4/2/1 delay=%u combos=3-4,6-7,6-9,5-6 pct=12/35/45 pressure=3in75,4in150 cooldowns=%u/%u/%u wound_guard=45/80/100 two_blocks=80pct_new_window counter=real_parry_only,50_50_N_D window=%u..%u",
                liz::kVersion, ok ? 1 : 0, static_cast<unsigned>(sizeof(r) / sizeof(r[0])), failed, liz::policy::kMaxHp, liz::policy::kMaxPosture, liz::policy::kRegenDelayTicks, liz::policy::kParryCooldownA, liz::policy::kParryCooldownB, liz::policy::kParryCooldownC,
                liz::policy::kParryWindowFirst, liz::policy::kParryWindowLast);
  lnk_write(ok ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  // General and independent: without this one hook the Lizalfos module still runs, and Link's damage stays exactly native.
  HookOptions first = HOOK_OPTIONS_INIT;
  first.priority = 200;
  ld.enabled = lf.enabled && mods::hook::add_pre<LinkSetDamage>(on_link_set_damage_pre, &first) == MOD_OK;
  std::snprintf(line, sizeof(line), "LINK_BREAK_DAMAGE event=INSTALL enabled=%d rule=x1_5_ONE_ATTRIBUTED_CONTACT_PER_CALL episode=LNK05_LNK06_SATURATED route=setDamagePoint_Tg_contact", ld.enabled ? 1 : 0);
  lnk_write(ld.enabled ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}
#undef LIZ_NOINLINE
