"""LIGHT FADES TWICE 0.13.3 CHILFOS FIX03: executed mutants of the PRODUCTION Chilfos core and adapter against tests/kk_lab.cpp and tests/kk_world_lab.cpp.
A mutant counts as detected only when a laboratory reports *_LAB_FAIL. Compile errors and timeouts are FAIL (never credited). The positive control runs
first; every anchor must exist exactly once. Optional sharding: env LIZ_MUTATION_SHARD=i/n."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
C = 'src/kk_core.hpp'
A = 'src/kk_adapter.inc'
cases = [
 # identity and patterns (A)
 ('clip_d_e_inverted', C, 'inline constexpr int kBckDiagonal = policy::kBckAttack01, kBckThrust = policy::kBckAttack03;', 'inline constexpr int kBckDiagonal = policy::kBckAttack03, kBckThrust = policy::kBckAttack01;'),
 ('identity_settles_on_one_vote', C, '    if (t >= 2 && t > 2 * d) return Tip::Thrust;', '    if (t >= 1) return Tip::Thrust;'),
 ('identity_swap_ignored', C, '    swapped = (a == Tip::Thrust);', '    swapped = false;'),
 ('pattern_c1_type_swapped', C, '{6, {K::D, K::D, K::D, K::E, K::E, K::D}', '{6, {K::D, K::D, K::E, K::E, K::E, K::D}'),
 ('pattern_c4_shortened', C, '    {7, {K::D, K::D, K::E, K::D, K::E, K::E, K::E}', '    {6, {K::D, K::D, K::E, K::D, K::E, K::E, K::E}'),
 ('c3_tail_dropped', C, '{0.9966f, 1.2100f, 2.6556f, 2.8884f, 4.3105f, 4.9375f, 5.5415f}, true, 6.9487f}', '{0.9966f, 1.2100f, 2.6556f, 2.8884f, 4.3105f, 4.9375f, 5.5415f}, false, 6.9487f}'),
 ('reroll_mid_combo', A, '        const int bck = kz.ident.bck(p.kind);', '        const int bck = kz.ident.bck(s->rng.below(2) ? kk::K::D : kk::K::E);'),
 ('pattern_bias', A, '  s.ep.pattern = s.rng.below(kk::kPatterns);', '  s.ep.pattern = s.rng.below(kk::kPatterns - 1);'),
 # rhythm, rates, windows (B, C)
 ('rate_d_wrong', C, 'kRateD = 1.25f', 'kRateD = 1.10f'),
 ('rate_burst_everywhere', C, 'a.rate = a.kind == K::D ? policy::kRateD : (a.burst ? policy::kRateEBurst : policy::kRateE);', 'a.rate = a.kind == K::D ? policy::kRateD : policy::kRateEBurst;'),
 ('rate_restore_native', A, '      rate = kk::policy::kRateRestore;\n      s->ep.ph = KkEp::Restore', '      rate = 1.0f;\n      s->ep.ph = KkEp::Restore'),
 ('rate_throw_native', A, '      rate = kk::policy::kRateThrow;', '      rate = 1.0f;'),
 ('burst_prep_cut', C, 'kMinPrep = 8, kBurstPrep = 6', 'kMinPrep = 8, kBurstPrep = 1'),
 ('second_at_fabricated', A, 'KK_NOINLINE void kk_native_close(daE_KK_c* a) noexcept { a->field_0x67e = 1; }', 'KK_NOINLINE void kk_native_close(daE_KK_c* a) noexcept { a->field_0x67e = 0; }'),
 ('window_merged', A, '  kk_native_close(a);\n  kz.self_bck = true;', '  kz.self_bck = true;'),
 ('restart_one_tick_early', A, '      if (now >= due) {', '      if (now + 1 >= due) {'),
 ('gap_runs_native', A, '      return HOOK_SKIP_ORIGINAL;  // between two planned actions', '      return HOOK_CONTINUE;  // between two planned actions'),
 ('frame24_snap_lost', A, '      kk_native_frame(a->mpMorfSO, c.frame);\n      s->clip.last_read = c.frame;\n    } else {\n      s->clip.last_read = f;\n    }\n  }\n  return HOOK_CONTINUE;\n}\nvoid on_kk_attack_post(',
  '      s->clip.last_read = c.frame;\n    } else {\n      s->clip.last_read = f;\n    }\n  }\n  return HOOK_CONTINUE;\n}\nvoid on_kk_attack_post('),
 ('snap_not_restored', A, '  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;\n  // FIX02: the native frame-24 lunge', '  s->clip.snapped = false;\n  // FIX02: the native frame-24 lunge'),
 ('crossing_strict', C, '    if (a < e && e < b && !(fired_mask >> i & 1u)) {', '    if (false) {'),
 ('duplicate_never_flagged', C, '    if (b == e && (fired_mask >> i & 1u)) c.duplicate = true;', ''),
 # C3 tail (D)
 ('frame23_omitted', C, 'inline constexpr int kThrowEvents[4] = {9, policy::kThrowSpawnFrame, 0x22, 0x3D};', 'inline constexpr int kThrowEvents[4] = {9, 22, 0x22, 0x3D};'),
 ('throw_emitted_twice', C, '    if (a < e && e < b && !(fired_mask >> i & 1u)) {', '    if (a <= e && e < b) {'),
 ('melee_cancel_not_held', A, '    kk_native_eye(a, -1.0e9f);', '    (void)0;'),
 ('eye_not_restored', A, '  if (s->eye_saved) kk_native_eye(a, s->eye_y), s->eye_saved = false;  // FIX01: restored even when the episode ended inside the native call',
  '  s->eye_saved = false;  // FIX01: restored even when the episode ended inside the native call'),
 ('spearless_transition_broken', A, '  if (s->ep.ph == KkEp::Back && idx == kk::policy::kWeaponRestore) return HOOK_SKIP_ORIGINAL;\n', ''),
 ('restore_clip_while_armed', A, '    if (idx == kk::policy::kBckRestoreB) return HOOK_SKIP_ORIGINAL;  // the spear is still in the hand: no restore clip\n', ''),
 ('restore_not_completed', A, '      s->ep.ph = KkEp::Restore, s->ep.last_progress = kkb_tick();\n      ++s->st.restores;', '      ++s->st.restores;'),
 ('wall_ignored', A, '  if (a->mMoveMode != 3) return;\n  ++s->st.early_throw', '  if (true) return;\n  ++s->st.early_throw'),
 # HP and identity (E)
 ('hp_on_projectile', C, '  if (in.sub == 1) return Excl::Spear;', '  if (in.sub == 11) return Excl::Spear;'),
 ('hp_foreign_overwritten', A, '    if (a->health == kk::policy::kNativeHp && a->field_0x560 == kk::policy::kNativeHp) {', '    if (true) {'),
 ('hp_set_every_execute', A, '  if (!s->hp_set) {\n    s->hp_set = true;', '  if (true) {\n    s->hp_set = true;'),
 ('hp_value_wrong', C, 'kNativeHp = 140, kMaxHp = 340', 'kNativeHp = 140, kMaxHp = 370'),
 ('hidden_never_joins', A, '    else if (s->excl == kk::Excl::HiddenIronBall) kk_admit(*s, a);\n', ''),
 # posture (F)
 ('link_parry_gain_wrong', C, 'kGainLinkParry = 13', 'kGainLinkParry = 12'),
 ('iron_gain_lost', A, '    kk_posture_gain(*s, a, kk::policy::kGainIron, "IRON_BALL", kz.swing_serial);', '    (void)0;'),
 ('sword_gain_wrong', C, 'kGainSwordHp = 5', 'kGainSwordHp = 6'),
 ('block_gain_wrong', C, 'kGainBlock = 3', 'kGainBlock = 5'),
 ('enemy_parry_self_gain', A, '  kk_link_gain(s, kk::policy::kLinkEnemyParry, false, s.parry_serial, "ENEMY_PARRY");', '  kk_link_gain(s, kk::policy::kLinkEnemyParry, false, s.parry_serial, "ENEMY_PARRY");\n  kk_posture_gain(s, a, 13, "WRONG", s.parry_serial);'),
 ('link_enemy_parry_wrong', C, 'kLinkEnemyParry = 25', 'kLinkEnemyParry = 20'),
 ('link_block_wrong', C, 'kLinkBlock = 20, kLinkDirect = 30', 'kLinkBlock = 25, kLinkDirect = 30'),
 ('link_parry_duplicated', A, '  s.imp_resolved = s.imp_serial;\n', ''),
 ('link_parry_also_block', A, '    kk_line(kKkLink, false, line);\n    return;\n  }\n  kkb_pfx_settle(true, s.id, false);', '    kk_line(kKkLink, false, line);\n  }\n  kkb_pfx_settle(true, s.id, false);'),
 ('latch_twice', C, '    if (pts <= 0 || latched) return false;', '    if (pts <= 0) return false;'),
 ('regen_while_latched', C, '    if (latched || u <= 0 || hp <= 0 || !eligible_now) return;', '    if (u <= 0 || hp <= 0 || !eligible_now) return;'),
 ('regen_no_delay', C, '    if (++eligible <= policy::kRegenDelayTicks) return;', '    ++eligible;'),
 ('regen_band_wrong', C, '  if (hp * 2 > policy::kMaxHp) return 8;', '  if (hp * 2 > policy::kMaxHp) return 10;'),
 ('regen_band_boundary', C, '  if (hp * 4 > 3 * policy::kMaxHp) return 12;', '  if (hp * 4 >= 3 * policy::kMaxHp - 4) return 12;'),
 ('regen_gain_no_restart', C, '    last_gain = tick, eligible = 0;', '    last_gain = tick;'),
 ('iron_without_receipt', A, 'if (!died && s->iron_hit && a->mDamageTimer == 8 && a->mActionMode == kk::kDamage) {', 'if (!died && s->iron_hit) {'),
 ('fallback_without_damage', A, '      kk_native_health(a, s->hp_before);', '      kk_native_health(a, 0); kk_native_action(a, kk::kDead, 0);'),
 ('decisive_rollback_lost', A, '      kk_native_health(a, s->hp_before);', '      (void)0;'),
 ('stagger_reuses_crossing_verdict', A, '  if (had != kk::V::None && (!s.brk.active() || had == kk::V::Decisive || had == kk::V::Ignored)) return had;', '  if (had != kk::V::None) return had;'),
 ('decisive_no_receipt_leaves_stagger', A, '      if (s->brk.active() && (a->mActionMode != s->act_before || a->mMoveMode != s->mode_before)) kk_native_action(a, s->act_before, s->mode_before);\n', ''),
 # break and finishers (G)
 ('same_swing_kills', C, '  bool sword_finishes(std::uint64_t serial) const noexcept { return st == Bk::Broken && serial != latch_serial && serial > latch_serial; }', '  bool sword_finishes(std::uint64_t serial) const noexcept { return st == Bk::Broken && serial >= latch_serial; }'),
 ('shield_attack_kills', A, '      s->brk.shield_attack();\n      ++s->st.shield_preps;', '      s->brk.shield_attack();\n      ++s->st.shield_preps;\n      kk_native_health(a, 0), kk_native_action(a, kk::kDead, 0);'),
 ('window_renewed', A, '      s->brk.shield_attack();\n      ++s->st.shield_preps;', '      s->brk.shield_attack();\n      s->brk.start = kkb_tick();\n      ++s->st.shield_preps;'),
 ('break_ticks_wrong', C, 'kBreakTicks = 90;', 'kBreakTicks = 150;'),
 ('posture_not_reset', A, '  s.brk.reset();\n  s.p.reset();', '  s.brk.reset();'),
 ('stagger_not_held', A, '    attr = 2;  // J3D loop', '    attr = attr;  // J3D loop'),
 ('decisive_not_lethal', A, '      kk_native_spear(a, 1);\n      if (a->health > 2) kk_native_health(a, 2);\n      break;', '      break;'),
 ('break_never_entered', A, '  kk_enter_break(s, a, origin);\n  return true;', '  (void)origin;\n  return false;'),
 ('break_exit_keeps_headlock', A, '  s.p.reset();\n  kk_native_headlock(a);', '  s.p.reset();'),
 # defense (H, I)
 ('block_forever', C, '    if (wound_streak == 0 || now - last_wound > policy::kWoundMemory) return policy::kBlockBase;', '    return 100;'),
 ('parry_and_damage', A, '    case kk::V::EnemyParry:\n      kkb_pfx_settle(false, s->id, true);', '    case kk::V::EnemyParry:\n      kk_native_spear(a, 1), s->conv = true;\n      kkb_pfx_settle(false, s->id, true);'),
 ('parry_from_flank', A, '  else if (s.shield_on && s.def.presenting(now, serial) && frontal && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;', '  else if (s.shield_on && s.def.presenting(now, serial) && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;'),
 ('block_from_flank', A, '  else if (kk_guard_ready(s, a) && frontal && s.rng.pct(s.def.block_pct(now))) v = kk::V::Block, s.guard_state = "ROLL";', '  else if (kk_guard_ready(s, a) && s.rng.pct(s.def.block_pct(now))) v = kk::V::Block, s.guard_state = "ROLL";'),
 ('rebound_never_suppressed', A, '      *r = FALSE;  // an ordinary block: no rebound', '      (void)r;  // an ordinary block: no rebound'),
 ('counter_never', A, '  s.counter_pending = true, s.counter_until = now + kk::policy::kCounterTtl, s.parry_tick = now', '  s.counter_until = now + kk::policy::kCounterTtl, s.parry_tick = now'),
 ('two_blocks_ignored', C, '    const int pct = pair ? policy::kPctTwoBlocks : lv == 2', '    const int pct = false ? policy::kPctTwoBlocks : lv == 2'),
 ('pct_a_wrong', C, 'kPctA = 12, kPctB = 35', 'kPctA = 20, kPctB = 35'),
 ('no_parry_cooldown', C, '    if (now < cooldown_until) return Np::Cooldown;\n', ''),
 ('wound_streak_ignored', C, '    return policy::kWoundPct[wound_streak >= 3 ? 2 : wound_streak - 1];', '    return policy::kBlockBase;'),
 ('verdict_not_cached', C, '  V of(std::uint64_t s) const noexcept { return s == serial ? v : V::None; }', '  V of(std::uint64_t) const noexcept { return V::None; }'),
 ('cooldown_fixed_120', C, 'level(now) == 2 ? policy::kParryCooldownC : level(now) == 1 ? policy::kParryCooldownB : policy::kParryCooldown', 'policy::kParryCooldown'),
 ('pressure_requires_wounds', C, 'within(now, policy::kSustainedWindow) >= 4', 'within(now, policy::kSustainedWindow) >= 6 && hits_since_defense >= 2'),
 ('repeated_attack_event', A, '      return HOOK_SKIP_ORIGINAL;  // repeated event frame: no second AT opening or attack sound', '      (void)0;'),
 # lifecycle (J)
 ('id_reuse_inherits', A, '  if (s && s->id != id) kk_retire(*s), s = nullptr;  // address reused', '  if (false) kk_retire(*s), s = nullptr;  // address reused'),
 ('event_keeps_combo', A, '    if (s->ep.active) kk_end_episode(*s, a, kk::End::Event, "EXECUTE_PRE_EVENT");\n    return HOOK_CONTINUE;', '    return HOOK_CONTINUE;'),
 ('rest_never', A, '    if (!s->counter_pending && now < s->rest_until) {  // the rest after an episode', '    if (false) {  // the rest after an episode'),
 # FIX01 0.13.1: posture below 100 never staggers; combos and the C3 tail survive Link's contacts; the break at 100 is immediate
 ('resume_lost', A, '  kk_native_resume(a, s.act_before, s.mode_before, s.heading_before);', '  (void)0;'),
 ('resume_heading_lost', A, '  kk_native_resume(a, s.act_before, s.mode_before, s.heading_before);', '  kk_native_resume(a, s.act_before, s.mode_before, a->current.angle.y);'),
 ('resume_melee_only_0_13_0', A, '  if (a->mActionMode != kk::kDamage) return;\n  if (s.act_before == kk::kDamage', '  if (a->mActionMode != kk::kDamage || !s.ep.active || s.ep.ph != KkEp::Melee) return;\n  if (s.act_before == kk::kDamage'),
 ('shield_attack_staggers', A, '    if (!s->brk.active()) {\n      // FIX01 (FIX05 rule)', '    if (false) {\n      // FIX01 (FIX05 rule)'),
 ('bash_se_every_contact', A, 'const bool fresh = s->bash_last_tick == 0 || now_dc > s->bash_last_tick + 1;', 'const bool fresh = true;'),
 ('break_deferred_one_tick', A, '    if (kk_try_break(*s, a, "DAMAGE_POST")) ++s->st.immediate_breaks;\n    else if', '    if (false) ++s->st.immediate_breaks;\n    else if'),
 ('sword_credit_every_contact', A, 'if (s->cur_serial != 0 && s->cur_serial == s->sword_credit_serial) {', 'if (false) {'),
 ('link_contact_gated_by_timer', A, '  kk_link_contact(*s, a, k);\n  if (a->mDamageTimer != 0) return HOOK_CONTINUE;  // the native ignores every contact of this call',
  '  if (a->mDamageTimer != 0) return HOOK_CONTINUE;  // the native ignores every contact of this call\n  kk_link_contact(*s, a, k);'),
 ('episode_guard_allowed', A, '  if (s.ep.active) return false;\n', ''),
 ('back_speed_native', A, '    kk_native_speed(a, kk::policy::kBackSpeedNative * kk::policy::kRateBack);', '    (void)0;'),
 ('tip_sweep_strict_0_13_0', C, '  if (lv > 100.0f && lv >= 0.6f * fz) return Tip::Diagonal;', '  if (lv > 30.0f && lv > 1.2f * fz) return Tip::Diagonal;'),
 # FIX02 0.13.2: the spear never breaks on a block or parry; a defense that holds under repeated B; the counter; no slide; x1.10 bursts
 ('spear_break_branch_restored', A, '    kk_native_action(a, kk::kGuard, 0);\n    ++s->st.spear_break_prevented;', '    (void)0;\n    ++s->st.spear_break_prevented;'),
 ('spear_intact_block_only', A, '(s->cur_v == kk::V::Block || s->cur_v == kk::V::EnemyParry) && s->saved_67d == 0 && a->mActionMode == kk::kDamage', 's->cur_v == kk::V::Block && s->saved_67d == 0 && a->mActionMode == kk::kDamage'),
 ('pressure_ignored_0_13_1', C, '    const int w = wound_pct(now), pr = pressure_pct(now);\n    return w > pr ? w : pr;', '    return wound_pct(now);'),
 ('guarantee_ignored', C, '    if (guaranteed(now)) return 100;\n', ''),
 ('uncovered_not_counted', A, '        if (s->dec_ready_frontal) s->def.on_uncovered(now);\n', ''),
 ('cooldown_old_120_90_60', C, 'kParryCooldown = 45, kParryCooldownB = 35, kParryCooldownC = 25;', 'kParryCooldown = 120, kParryCooldownB = 90, kParryCooldownC = 60;'),
 ('counter_never_dispatched', A, '      kk_native_action(a, kk::kAttack, 0);\n      ++s->st.counter_dispatched;', '      ++s->st.counter_dispatched;'),
 ('counter_cancel_silent', A, '    if (now > s->counter_until) kk_counter_cancel(*s, s->counter_block);', '    if (now > s->counter_until) s->counter_pending = false;'),
 ('burst_rate_150', C, 'kRateEBurst = 1.10f', 'kRateEBurst = 1.50f'),
 ('recoil_not_fixed', A, '    kk_native_speed(a, 0.0f);\n    ++s->st.recoil_fixed;', '    ++s->st.recoil_fixed;'),
 ('lunge_uncapped', A, '        kk_native_speed(a, v);\n        ++s->st.lunge_clamped;', '        ++s->st.lunge_clamped;'),
 ('spear_monitor_blind', A, '    if (s->spear_prev && !spear && !s->dead && !(s->ep.active && (s->ep.ph == KkEp::Throw || s->ep.ph == KkEp::Restore))) {', '    if (false) {'),
 # FIX03 0.13.3: HP and posture are independent; the guard that is up covers; the stagger shows no guard; the native Mortal Draw finisher; the throw's pacing
 ('skill_instant_kill_restored', A, '  if (param == 2 && !s->brk.active()) {', '  if (false) {'),
 ('hp_zero_death_blocked', A, '      if (kk_spear(a)) s->conv = true, kk_native_spear(a, 1);\n      break;', '      if (kk_spear(a)) s->conv = true, kk_native_spear(a, 1);\n      if (a->health <= 20) kk_native_health(a, 21);\n      break;'),
 ('guard_active_roll_restored', A, '  else if (a->mActionMode == kk::kGuard && kk_guard_ready(s, a) && frontal) v = kk::V::Block, s.guard_state = "ACTIVE", ++s.st.guard_active_blocks;\n', ''),
 ('guard_active_from_back', A, '  else if (a->mActionMode == kk::kGuard && kk_guard_ready(s, a) && frontal)', '  else if (a->mActionMode == kk::kGuard && kk_guard_ready(s, a))'),
 ('stagger_shield_reopened', A, '  if (param == 0 && s->brk.active()) {', '  if (false) {'),
 ('stagger_drawback_shown', A, '      idx = kk_spear(a) ? kk::policy::kBckDamageBow : kk::policy::kBckDamageBowB;\n      ++s->st.stagger_clip_kept;', '      ++s->st.stagger_clip_kept;'),
 ('md_not_started', A, '  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);', '  (void)0;'),
 ('md_index_swapped', C, 'kFinishMortalDrawA = 3, kFinishMortalDrawB = 4;', 'kFinishMortalDrawA = 4, kFinishMortalDrawB = 3;'),
 ('md_steals_helm', A, '!s.brk.active() || s.brk.shield_prep || s.md_active) continue;', '!s.brk.active() || s.md_active) continue;'),
 ('md_any_actor', A, '!s.brk.active() || s.brk.shield_prep || s.md_active) continue;', 's.brk.shield_prep || s.md_active) continue;'),
 ('md_double_request', A, '!s.brk.active() || s.brk.shield_prep || s.md_active) continue;', '!s.brk.active() || s.brk.shield_prep) continue;'),
 ('md_kill_on_press', A, '  if (started) best->md_active = true, best->md_tick = kkb_tick(), ++best->st.md_started;', '  if (started) best->md_active = true, best->md_tick = kkb_tick(), ++best->st.md_started, kk_native_health(a, 0), kk_native_action(a, kk::kDead, 0);'),
 ('md_from_behind', A, ' && kk::angle_diff(k->shape_angle.y, to_kk) <= kk::policy::kMdArc && d < best_d)', ' && d < best_d)'),
 ('md_no_extension', A, '    if (s->md_active && now < s->brk.start + kk::policy::kBreakTicks + kk::policy::kMdExtendMax) {', '    if (false) {'),
 ('md_never_cancelled', A, '  if (on || s.dead) return;\n  s.md_active = false, ++s.st.md_cancelled;', '  return;\n  s.md_active = false, ++s.st.md_cancelled;'),
 ('spear_child_takes_slot', A, '  if (!s && a->field_0x679 == 1 && fopAcM_GetName(a) == fpcNm_E_KK_e) {', '  if (false) {'),
 ('spear_child_logged_each', A, '  if (kz.spear_children == 1) {', '  if (true) {'),
 ('perf_window_never_opens', A, '    kk_perf_open();\n', ''),
 ('perf_slow_frames_wrong', A, 'slow += f.post[i] * 2 > m0 * 3 ? 1u : 0u;', 'slow += f.post[i] > m0 * 3 ? 1u : 0u;'),
 ('death_cause_lumped', A, '                      : (s->cur_v == kk::V::Damage || s->cur_v == kk::V::None) ? "HP_ZERO" : "OTHER";', '                      : "OTHER";'),
 ('rearm_not_logged', A, '  if (s->ep.ph == KkEp::Restore && s->d_before != 0 && a->field_0x67d == 0) {', '  if (false) {'),
 ('install_partial', A, '    if (x != MOD_OK) ok = false, ++failed;', '    if (x != MOD_OK) ++failed;'),
]
TOTAL = len(cases)
shard = os.environ.get('LIZ_MUTATION_SHARD')
if shard:
    i, n = (int(x) for x in shard.split('/'))
    cases = cases[i::n]
LABS = ('kk_lab', 'kk_world_lab')
def build(p, lab):
    exe = p / (lab + ('.exe' if os.name == 'nt' else ''))
    src = p / 'tests' / (lab + '.cpp')
    cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', str(src), '/Fe' + str(exe), '/Fo' + str(p / (lab + '.obj'))] if os.name == 'nt' else ['g++', '-std=c++20', '-O1', '-w', '-o', str(exe), str(src)]
    return exe, subprocess.run(cmd, capture_output=True, text=True, timeout=600)
def run(p):
    out = ''
    for lab in LABS:
        exe, c = build(p, lab)
        if c.returncode:
            return False, 'COMPILE_ERROR ' + c.stdout[-1500:] + c.stderr[-1500:]
        r = subprocess.run([str(exe)], capture_output=True, text=True, timeout=300)
        out += r.stdout + r.stderr
        if r.returncode:
            return False, out
    return True, out
def copy(p):
    shutil.copytree(root / 'src', p / 'src')
    (p / 'tests').mkdir()
    for lab in LABS:
        shutil.copy(root / 'tests' / (lab + '.cpp'), p / 'tests' / (lab + '.cpp'))
for name, rel, old, new in cases:
    assert (root / rel).read_text(encoding='utf-8').count(old) == 1, 'anchor not unique: ' + name
with tempfile.TemporaryDirectory() as td:
    p = Path(td)
    copy(p)
    ok, out = run(p)
    assert ok and out.count('RESULT=LAB_PASS') == 2, out[-2000:]
def one(case):
    name, rel, old, new = case
    with tempfile.TemporaryDirectory() as td:
        p = Path(td)
        copy(p)
        f = p / rel
        f.write_text(f.read_text(encoding='utf-8').replace(old, new, 1), encoding='utf-8')
        try:
            ok, out = run(p)
            killed = not ok and ('KK_LAB_FAIL line=' in out or 'KK_WORLD_LAB_FAIL line=' in out)
            return {'mutation': name, 'detected': killed, 'status': 'KILLED' if killed else 'FAIL', 'timeout': False, 'tail': None if killed else out[-300:]}
        except subprocess.TimeoutExpired:
            return {'mutation': name, 'detected': False, 'status': 'FAIL', 'timeout': True}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('LIZ_MUTATION_WORKERS', '4'))) as pool:
    result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'KK_LABS_PASS', 'total': TOTAL, 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
