// LIZALFOS DUELIST 0.12.1 production adapter. Included once inside mod.cpp's anonymous namespace after the CP06 FIX01 adapter.
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
  bool swing_jump = false, coverage = false, slip_pre = false;
  std::uint64_t promotions = 0;
  std::uint64_t pressure_promotions(const liz::Stats&) const noexcept { return promotions; }
  std::uint64_t slip_pre_tick = ~0ull, se_native = 0, se_restored = 0, se_ignored_foreign = 0, gap_arms = 0, zero_vec = 0, probes = 0, probe_mismatch = 0, place_failed = 0, foreign_sphere = 0, bash_se = 0, no_receipt = 0;
  std::uint64_t why[unsigned(liz::sweep::Why::Count)] = {};
  float snap[kF1Joints][3][4];
  lnks1fix::DirectJoin direct;
  liz::Stats totals{};
  std::uint64_t rolls[3] = {}, attempts[3] = {};
  unsigned lines[12] = {};
  std::uint64_t dropped[12] = {};
} lz;
enum LizLog : unsigned { kLogAdmit = 0, kLogCombo, kLogSwing, kLogDefense, kLogParry, kLogTurn, kLogBreak, kLogPosture, kLogCritical, kLogLink, kLogImpact, kLogFeedback };
inline constexpr unsigned kLizCaps[12] = {64, 240, 400, 240, 240, 240, 240, 120, 80, 160, 700, 240};
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
  t.swings_detected += x.swings_detected, t.block_pairs += x.block_pairs, t.priority_rolls += x.priority_rolls, t.priority_armed += x.priority_armed, t.counter_at_open += x.counter_at_open, t.counter_contact += x.counter_contact, t.counter_expired += x.counter_expired;
  for (unsigned i = 0; i < 2; ++i) t.counter_selected[i] += x.counter_selected[i], t.counter_started[i] += x.counter_started[i];
  for (unsigned i = 0; i < unsigned(liz::Cancel::Count); ++i) t.counter_cancel[i] += x.counter_cancel[i];
  t.break_standing += x.break_standing, t.helm_prepared += x.helm_prepared, t.decisive_sword += x.decisive_sword, t.decisive_helm += x.decisive_helm, t.punish_end += x.punish_end, t.down_finish += x.down_finish, t.getup += x.getup;
  for (unsigned p = 0; p < liz::kPatterns; ++p)
    for (unsigned i = 0; i < 9; ++i) t.link_parry_by_impact[p][i] += x.link_parry_by_impact[p][i];
  for (unsigned i = 0; i < unsigned(liz::Cls::Count); ++i) t.impact_class[i] += x.impact_class[i];
  t.expect_ticks += x.expect_ticks, t.candidate_ticks += x.candidate_ticks, t.nocon_ticks += x.nocon_ticks, t.lost_ticks += x.lost_ticks, t.off_touch_ticks += x.off_touch_ticks, t.gap_fixed += x.gap_fixed;
  for (unsigned i = 0; i < 3; ++i) rolls[i] += b.def.rolls[i], atts[i] += b.def.attempts[i];
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
LIZ_NOINLINE cCcD_Obj* liz_native_tg_clear(e_dn_class* a) noexcept {
  cCcD_Obj* first = nullptr;
  for (int i = 0; i < 3; ++i) {
    auto& s = a->cc_sph[i];
    if (!s.cCcD_ObjHitInf::ChkTgHit()) continue;
    cCcD_Obj* o = s.cCcD_ObjHitInf::GetTgHitObj();
    if (o && o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
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
LIZ_NOINLINE void liz_native_hold(e_dn_class* a, bool set_timer, std::int16_t timer0, bool head_off) noexcept {
  if (set_timer) a->timer[0] = timer0;
  if (head_off) a->actor.offHeadLockFlg();
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
    cCcD_Obj* o = liz_native_tg_clear(a);
    if (w.bash_se && o) {
      liz_native_bash_se(a, o);
      ++lz.bash_se;
    }
  }
  if (w.big_damage) {
    liz_native_break(a);
    return;
  }
  if (w.stagger) liz_native_stagger(a);
  if (w.set_timer0 || w.head_lock_off) liz_native_hold(a, w.set_timer0, w.timer0, w.head_lock_off);
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
    v.tate_link_sword = sword && own;
  }
}
liz::LinkFrame liz_link_frame(daAlink_c* k) noexcept {
  liz::LinkFrame L;
  L.swing_serial = lz.swing_serial;
  L.shield_serial = c4.shield.native_serial;
  L.jump_cut = lz.swing_jump;
  if (k) L.shape_y = static_cast<fopAc_ac_c*>(k)->shape_angle.y;
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
        std::snprintf(line, sizeof(line), "LIZ_DEFENSE event=ORDINARY_BLOCK actor=%u generation=%llu gain=+%d posture_before=%d hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, ac->health,
                      static_cast<unsigned long long>(e.t));
        liz_line(kLogDefense, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::EnemyParry:
        std::snprintf(line, sizeof(line), "LIZ_PARRY event=ENEMY_PARRY actor=%u generation=%llu link_gain=+%d level=%d guard_age=%d hp=%d tick=%llu cooldown_until=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c,
                      ac->health, static_cast<unsigned long long>(e.t), static_cast<unsigned long long>(b.def.cooldown_until));
        liz_line(kLogParry, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::PriorityRoll:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=PRIORITY_ROLL actor=%u generation=%llu armed=%d pct=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterSelected:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=COUNTER_SELECTED actor=%u generation=%llu token=%llu kind=%s source=%s tick=%llu dist=%.1f", s.id, static_cast<unsigned long long>(b.generation), static_cast<unsigned long long>(b.counter.token), e.a == 0 ? "NORMAL_X1_25" : "DOUBLE_X1_10",
                      e.b == 0 ? "TWO_BLOCKS" : "ENEMY_PARRY", static_cast<unsigned long long>(e.t), a->pl_dir);
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterGuardExit:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=GUARD_EXIT actor=%u token=%llu from_mode=%d tick=%llu", s.id, static_cast<unsigned long long>(b.counter.token), e.a, static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::CounterCommitted:
        std::snprintf(line, sizeof(line), "LIZ_COUNTER event=ATTACK_COMMITTED actor=%u token=%llu kind=%s source=%s waited_ticks=%d tick=%llu dist=%.1f", s.id, static_cast<unsigned long long>(b.counter.token), e.a == 0 ? "NORMAL_X1_25" : "DOUBLE_X1_10", e.b == 0 ? "TWO_BLOCKS" : "ENEMY_PARRY", e.c,
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
          std::snprintf(line, sizeof(line), "LIZ_IMPACT actor=%u generation=%llu pattern=%d act=%d impact=%d class=%s link_proc=%d open=%llu contact=%llu expect=%u candidate_written=%u immune_touch=%u lost=%u no_continuous=%u audio_native_receipt=%u audio_restored_call=%u effect=ENGINE_NOT_OBSERVED runtime=HUMAN_RUNTIME_PENDING", s.id,
                        static_cast<unsigned long long>(b.generation), m.pattern + 1, m.act, m.impact, liz::name(m.cls), m.link_proc, static_cast<unsigned long long>(m.open_tick), static_cast<unsigned long long>(m.contact_tick), m.expect, m.written, m.off_touch, m.lost, m.nocon, m.feedback_native, m.feedback_restored);
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
    s->b.bind(id, ++lz.generations, g.p8.tick ^ (static_cast<std::uint64_t>(lz.generations) << 20));
    liz::IdIn in;
    in.profile_ok = ac->profname == fpcNm_E_DN_e && ac->name == fpcNm_E_DN_e;
    in.skull_model = a->skull_model != nullptr, in.arg0 = a->arg0, in.arg1 = a->arg1, in.status = a->status, in.action = a->action;
    in.special_room = liz_stage_is(dComIfGp_getStartStageName(), "D_MN07") && fopAcM_GetRoomNo(ac) == 8;
    s->b.admit(in);
    liz_drain(*s, a);
  }
  if (!s->b.admitted) return HOOK_CONTINUE;
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
  const liz::Write w = s->b.on_exec_post(v, liz_link_frame(k));
  liz_apply(a, w);
  if (s->b.impact_gap() && lz.d1_parry_id == s->id) lz.double_gap_tick = g.p8.tick;
  liz_drain(*s, a);
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
  const liz::Write w = s->b.on_damage_pre(liz_view(a, f1_link()), shield);
  liz_apply(a, w);
  liz_drain(*s, a);
  return HOOK_CONTINUE;
}
void on_liz_damage_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
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
  const bool skip = s->b.on_small_pre(liz_view(a, k), liz_hit_kind(a, k));
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction on_liz_big_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  const bool skip = s->b.on_big_pre(liz_view(a, k), liz_hit_kind(a, k));
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
  const liz::Write w = s->b.on_run_post(liz_view(a, f1_link()), lz.swing_jump);
  liz_apply(a, w);
  liz_drain(*s, a);
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
  if (lz.swing_tick == g.p8.tick) return;                                             // one distinct swing per tick
  lz.swing_tick = g.p8.tick;
  ++lz.swing_serial;
  lz.swing_jump = t == daPy_py_c::CUT_TYPE_JUMP;
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
// ---------------------------------------------------------------- once per mod_update
void liz_maintenance() noexcept {
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
  char line[kLogLineMax * 2];
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
                "LIZ_SUMMARY part=FIX01 version=%s swings_detected=%llu pressure_promotions=%llu block_pairs=%llu priority_roll=%llu priority_armed=%llu counter_selected=%llu/%llu counter_started=%llu/%llu counter_AT_open=%llu counter_contact=%llu counter_expired=%llu "
                "break_standing=%llu helm_prepared=%llu decisive_sword=%llu decisive_helm=%llu down_finish=%llu getup=%llu punish_end=%llu native_guard=%llu AT_SAMPLE_GAP=%llu collision_gap_fixed=%llu expect_ticks=%llu lost_ticks=%llu no_continuous=%llu immune_touch=%llu "
                "impacts_whiff=%llu impacts_guard=%llu impacts_parry=%llu impacts_body=%llu impacts_immune=%llu impacts_missing=%llu native_feedback_receipt=%llu audio_restored_during_slip=%llu no_receipt=%llu bash_se=%llu gap_arms=%llu zero_vec=%llu probes=%llu probe_mismatch=%llu coverage=%d watchdog=%llu",
                liz::kVersion, u(t.swings_detected), u(lz.pressure_promotions(t)), u(t.block_pairs), u(t.priority_rolls), u(t.priority_armed), u(t.counter_selected[0]), u(t.counter_selected[1]), u(t.counter_started[0]), u(t.counter_started[1]), u(t.counter_at_open), u(t.counter_contact), u(t.counter_expired),
                u(t.break_standing), u(t.helm_prepared), u(t.decisive_sword), u(t.decisive_helm), u(t.down_finish), u(t.getup), u(t.punish_end), u(t.guards_native), u(t.sample_gap), u(t.gap_fixed), u(t.expect_ticks), u(t.lost_ticks), u(t.nocon_ticks), u(t.off_touch_ticks),
                u(t.impact_class[unsigned(liz::Cls::Whiff)]), u(t.impact_class[unsigned(liz::Cls::LinkGuard)]), u(t.impact_class[unsigned(liz::Cls::LinkParry)]), u(t.impact_class[unsigned(liz::Cls::LinkBody)]), u(t.impact_class[unsigned(liz::Cls::ImmuneValid)]), u(t.impact_class[unsigned(liz::Cls::MissingFeedback)]),
                u(lz.se_native), u(lz.se_restored), u(lz.no_receipt), u(lz.bash_se), u(lz.gap_arms), u(lz.zero_vec), u(lz.probes), u(lz.probe_mismatch), lz.coverage ? 1 : 0, u(t.watchdog));
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
      mods::hook::add_post<LinkGuardSe>(on_guard_se_post)};
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
  std::snprintf(line, sizeof(line), "LIZ_POSTURE event=INSTALL version=%s enabled=%d hooks=%u failed=%u hp=%d capacity=%d gains=9/15/30/5/25 regen=10/7/4/2/1 delay=%u combos=3-4,6-7,6-9,5-6 pct=12/35/45 pressure=3in75,4in150 cooldowns=%u/%u/%u counter=2blocks80pct,50_50_N_D window=%u..%u",
                liz::kVersion, ok ? 1 : 0, static_cast<unsigned>(sizeof(r) / sizeof(r[0])), failed, liz::policy::kMaxHp, liz::policy::kMaxPosture, liz::policy::kRegenDelayTicks, liz::policy::kParryCooldownA, liz::policy::kParryCooldownB, liz::policy::kParryCooldownC,
                liz::policy::kParryWindowFirst, liz::policy::kParryWindowLast);
  lnk_write(ok ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}
#undef LIZ_NOINLINE
