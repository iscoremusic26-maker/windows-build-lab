"""LIGHT FADES TWICE 0.13.6 CHILFOS FIX06 (over FIX05): executed mutants of the PRODUCTION Chilfos core and adapter against tests/kk_lab.cpp and tests/kk_world_lab.cpp.
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
 ('melee_cancel_not_held', A, '        kk_native_eye(a, y);', '        (void)y;'),
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
 ('stagger_not_held', A, '    attr = 2;  // EMode_LOOP never sets', '    attr = attr;  // EMode_LOOP never sets'),
 ('decisive_not_lethal', A, '      kk_native_spear(a, 1);\n      if (a->health > 2) kk_native_health(a, 2);\n      break;', '      break;'),
 ('break_never_entered', A, '  kk_enter_break(s, a, origin);\n  return true;', '  (void)origin;\n  return false;'),
 ('break_exit_keeps_headlock', A, '  s.p.reset();\n  kk_native_headlock(a);', '  s.p.reset();'),
 # defense (H, I)
 ('block_forever', C, '    if (wound_streak == 0 || now - last_wound > policy::kWoundMemory) return policy::kBlockBase;', '    return 100;'),
 ('parry_and_damage', A, '      if (kkb_strong_hit(k, o)) kkb_pfx_clash_map(kk::policy::kSeMapClash);', '      kk_native_spear(a, 1), s->conv = true;'),
 ('parry_from_flank', A, '  else if (s.shield_on && s.def.presenting(now, serial) && frontal && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;', '  else if (s.shield_on && s.def.presenting(now, serial) && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;'),
 ('block_from_flank', A, '  else if (kk_guard_ready(s, a) && frontal && s.rng.pct(s.def.block_pct(now, serial))) v = kk::V::Block, s.guard_state = "ROLL";', '  else if (kk_guard_ready(s, a) && s.rng.pct(s.def.block_pct(now, serial))) v = kk::V::Block, s.guard_state = "ROLL";'),
 ('rebound_never_suppressed', A, '      *r = FALSE;  // an ordinary block: no rebound', '      (void)r;  // an ordinary block: no rebound'),
 ('counter_never', A, '  s.counter_pending = true, s.counter_until = now + kk::policy::kCounterTtl, s.parry_tick = now', '  s.counter_until = now + kk::policy::kCounterTtl, s.parry_tick = now'),
 ('two_blocks_ignored', C, '    const int lpct = pair ? policy::kPctTwoBlocks : lv == 2', '    const int lpct = false ? policy::kPctTwoBlocks : lv == 2'),
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
 ('link_contact_gated_by_timer', A, '  if (ro.r != kk::Round::Stale) kk_link_contact(*s, a, k);', '  if (ro.r != kk::Round::Stale && a->mDamageTimer == 0) kk_link_contact(*s, a, k);'),
 ('episode_guard_allowed', A, '  if (s.ep.active) return false;\n', ''),
 ('back_speed_native', A, '    kk_native_speed(a, kk::policy::kBackSpeedNative * kk::policy::kRateBack);', '    (void)0;'),
 ('tip_sweep_strict_0_13_0', C, '  if (lv > 100.0f && lv >= 0.6f * fz) return Tip::Diagonal;', '  if (lv > 30.0f && lv > 1.2f * fz) return Tip::Diagonal;'),
 # FIX02 0.13.2: the spear never breaks on a block or parry; a defense that holds under repeated B; the counter; no slide; x1.10 bursts
 ('spear_break_branch_restored', A, '    kk_native_action(a, kk::kGuard, 0);\n    ++s->st.spear_break_prevented;', '    (void)0;\n    ++s->st.spear_break_prevented;'),
 ('spear_intact_block_only', A, '(s->cur_v == kk::V::Block || s->cur_v == kk::V::EnemyParry) && s->saved_67d == 0 && a->mActionMode == kk::kDamage', 's->cur_v == kk::V::Block && s->saved_67d == 0 && a->mActionMode == kk::kDamage'),
 ('pressure_ignored_0_13_1', C, '    const int w = wound_pct(now), pr = pressure_pct(now), j = jump_floor(policy::kJumpBlockPct, now, serial);\n    const int m = w > pr ? w : pr;', '    const int j = jump_floor(policy::kJumpBlockPct, now, serial);\n    const int m = wound_pct(now);'),
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
 ('perf_window_never_opens', A, '  s.thr_max = s.thr_sum = 0, s.thr_calls = 0, s.thr_len_max = 0, s.thr_t0 = 0;\n  kk_perf_open();', '  s.thr_max = s.thr_sum = 0, s.thr_calls = 0, s.thr_len_max = 0, s.thr_t0 = 0;'),
 ('perf_slow_frames_wrong', A, 'slow += f.post[i] * 2 > m0 * 3 ? 1u : 0u;', 'slow += f.post[i] > m0 * 3 ? 1u : 0u;'),
 ('death_cause_lumped', A, '                      : (s->cur_v == kk::V::Damage || s->cur_v == kk::V::None) ? "HP_ZERO" : "OTHER";', '                      : "OTHER";'),
 ('rearm_not_logged', A, '  if (s->ep.ph == KkEp::Restore && s->d_before != 0 && a->field_0x67d == 0) {', '  if (false) {'),
 ('install_partial', A, '    if (x != MOD_OK) ok = false, ++failed;', '    if (x != MOD_OK) ++failed;'),
 # FIX04 P0: the C3 wind-up's giant BG line (each restores one 0.13.3 path)
 ('fix04_eye_at_minus_1e9', A, '      const float y = kk::throw_eye(a->eyePos.y, k->current.pos.y);', '      const float y = -1.0e9f;'),
 ('fix04_throw_eye_core_1e9', C, '  return eye < want ? eye : want;\n}', '  return -1.0e9f + 0.0f * (eye + want);\n}'),
 ('fix04_eye_raised_not_lowered', C, '  const float want = link_y - policy::kThrowEyeBelow;', '  const float want = link_y + policy::kThrowEyeBelow;'),
 ('fix04_throw_cost_not_logged', A, '    kk_throw_cost(*s);\n', ''),
 # FIX04 P1: one clash, one spark, one receipt per spear attack
 ('fix04_stale_replayed', C, '  if (!live) return o.r = Round::Stale, o.clear = any_hit, o;', '  if (false) return o.r = Round::Stale, o.clear = any_hit, o;'),
 ('fix04_echo_kept', C, '  if (token == sounded) return o.r = Round::Echo, o.clear = shield_hit, o;', '  if (false) return o.r = Round::Echo, o.clear = shield_hit, o;'),
 ('fix04_spheres_not_merged', C, '  o.r = Round::Keep, o.keep = low_bit(shield_hit), o.clear = shield_hit & ~o.keep;', '  o.r = Round::Keep, o.keep = shield_hit, o.clear = 0;'),
 ('fix04_first_contact_silenced', C, '  o.r = Round::Keep, o.keep = low_bit(shield_hit), o.clear = shield_hit & ~o.keep;', '  o.r = Round::Keep, o.keep = low_bit(shield_hit), o.clear = shield_hit;'),
 ('fix04_self_se_missing', A, '    kk_native_bash_se(&a->mCreatureSound);\n    ++s.st.se_self, ++s.tok_self;', '    ++s.st.se_self, ++s.tok_self;'),
 ('fix04_marks_not_deduped', A, '  ++s->st.marks_suppressed, ++s->tok_marks_cut;\n  return HOOK_SKIP_ORIGINAL;', '  ++s->st.marks_suppressed, ++s->tok_marks_cut;\n  return HOOK_CONTINUE;'),
 ('fix04_hitmark_hook_missing', A, ', mods::hook::add_pre<PfxProcHitmark>(on_kk_hitmark_pre, &after_liz)};', '};'),
 ('fix04_contact_from_stale', A, '  if (ro.r != kk::Round::Stale) kk_link_contact(*s, a, k);', '  kk_link_contact(*s, a, k);'),
 ('fix04_token_never_opens', A, '    if (live && !s->cc_live) kk_token_close(*s, a), kk_token_open(*s);', '    if (false) kk_token_close(*s, a), kk_token_open(*s);'),
 ('fix04_token_per_tick', A, '    if (live && !s->cc_live) kk_token_close(*s, a), kk_token_open(*s);', '    if (live) kk_token_close(*s, a), kk_token_open(*s);'),
 # FIX04 P2: the guard spark of the strong branch, the settle after the spark, the clash map
 ('fix04_guard_mark_missing', A, '    kkb_guard_mark(a);  // FIX04 P2', '    (void)0;  // FIX04 P2'),
 ('fix04_parry_settled_in_pre', A, '      if (kkb_strong_hit(k, o)) kkb_pfx_clash_map(kk::policy::kSeMapClash);', '      kkb_pfx_settle(false, s->id, true);'),
 ('fix04_clash_map_not_restored', A, '    kkb_pfx_clash_map(kk::policy::kSeMapGuard);\n', ''),
 ('fix04_guard_mark_on_damage', A, '  if (!died && (s->cur_v == kk::V::Block || s->cur_v == kk::V::EnemyParry)) s->def_tick = now;', '  if (!died && s->cur_v == kk::V::Damage) kkb_guard_mark(a);\n  if (!died && (s->cur_v == kk::V::Block || s->cur_v == kk::V::EnemyParry)) s->def_tick = now;'),
 # FIX04 P3: facing, the guard raised at the swing start, one roll, wound -> guard, never inside its own attack
 ('fix04_no_face', A, '      if (h != a->shape_angle.y) kk_native_face(a, h), ++s->st.face_ticks;', '      (void)h;'),
 ('fix04_face_instant', C, '  int step = diff / scale;', '  int step = diff;\n  max_step = 0x8000;'),
 ('fix04_no_swing_raise', A, '    kk_raise_guard(s, a, k, why, now);\n', ''),
 ('fix04_second_roll_at_contact', A, '    else s.guard_state = s.raise_ok ? "RAISED_FLANKED" : "RAISE_FAILED";', '    else if (kk_guard_ready(s, a) && frontal && s.rng.pct(s.def.block_pct(now))) v = kk::V::Block, s.guard_state = "ROLL";'),
 ('fix04_raise_during_attack', A, '  if (s.dead || !kk_guard_ready(s, a) || !kk_frontal(a, k)) return;\n  const int act = a->mActionMode;\n  if (act != kk::kWait && act != kk::kWalk && act != kk::kGuard) return;', '  if (s.dead) return;\n  const int act = a->mActionMode;'),
 ('fix04_raised_guard_covers_flank', A, '    if (s.raise_ok && kk_guard_ready(s, a) && frontal) v = kk::V::Block, s.guard_state = "RAISED";', '    if (s.raise_ok && kk_guard_ready(s, a)) v = kk::V::Block, s.guard_state = "RAISED";'),
 ('fix04_wound_no_guard', A, '        kk_native_action(a, kk::kGuard, 0);\n        ++s->st.guard_after_wound;', '        ++s->st.guard_after_wound;'),
 # FIX05 P0: the stagger plays once across the window, ends in its own stance, never jumps back to the bow
 ('fix05_bow_not_stretched', A, '        kk_native_bow(m, s.bow_rate, kk::bow_loop(static_cast<std::int16_t>(s.bow_end)));', '        (void)0;'),
 ('fix05_bow_loop_frame_zero', C, '{ return end > 1 ? std::int16_t(end - 1) : std::int16_t(0); }', '{ return std::int16_t(0 * end); }'),
 ('fix05_bow_rate_native', C, '  float r = left / float(ticks_left ? ticks_left : 1);', '  float r = policy::kBowRateMax + 0.0f * left * float(ticks_left);'),
 ('fix05_bow_no_replan', A, '    s->bow_pending = true;  // FIX05 P0', '    (void)0;  // FIX05 P0'),
 ('fix05_bow_off_by_one', A, 'kk::bow_rate(f, s.bow_end, end_tick - now > 1 ? end_tick - now - 1 : 1)', 'kk::bow_rate(f, s.bow_end, end_tick - now + 2)'),
 # FIX05 P1: Jump Attacks: native rebound routes, one serial per jump, adaptive floor, one gain
 ('fix05_jump_no_rebound', A, ' || proc == daAlink_c::PROC_CUT_JUMP || proc == daAlink_c::PROC_CUT_JUMP_LAND;\n}', ';\n}'),
 ('fix05_jump_land_new_serial', A, '  const bool land = jump && proc == daAlink_c::PROC_CUT_JUMP_LAND && kz.jump_open && kz.jump_serial == kz.swing_serial;', '  const bool land = false;'),
 ('fix05_landing_rolls_again', A, '    if (land && s.rolled_serial == kz.swing_serial) {  // already decided at the takeoff', '    if (false) {  // already decided at the takeoff'),
 ('fix05_jump_floor_ignored', C, '    const int pct = jf > lpct ? jf : lpct;', '    const int pct = lpct + 0 * jf;'),
 ('fix05_jump_lease_short', C, '(jump ? policy::kJumpParryLease : policy::kParryLease)', 'policy::kParryLease'),
 ('fix05_jump_parry_always', C, 'inline constexpr int kJumpParryPct[4] = {0, 40, 60, 75};', 'inline constexpr int kJumpParryPct[4] = {100, 100, 100, 100};'),
 ('fix05_block_gain_twice', A, '        if (s->cur_serial != 0 && s->cur_serial == s->block_credit_serial) {', '        if (false) {'),
 # FIX05 P2: the burst gap feeds the shared fast re-arm, only a real pair's gap
 ('fix05_burst_gap_not_fed', A, '    kkb_burst_gap(s.id);\n    ++s.st.burst_gap_ticks;', '    ++s.st.burst_gap_ticks;'),
 ('fix05_burst_any_e_pair', A, ' && s.ep.plan.a[i].recorded_s < kk::policy::kBurstMax;\n}', ';\n}'),
 # FIX05 P3: the Helm Splitter aims at its upright head, never lower than native
 ('fix05_head_not_lifted', A, '      kk_native_head(a, y);', '      (void)y;'),
 ('fix05_head_lowered', A, '    if (y > native) {\n      kk_native_head(a, y);', '    if (y != native) {\n      kk_native_head(a, y);'),
 ('fix05_head_never_sampled', A, '      if (s.head_up == 0.0f) ++s.st.head_samples;\n      s.head_up = rel;', '      if (s.head_up == 0.0f) ++s.st.head_samples;'),
 # FIX05 P4: the eye of the Chilfos the Mortal Draw killed keeps its kill height while that Mortal Draw aims Link's torso, then is native again
 ('fix05_md_no_hold', A, '      if (a->eyePos.y < s.md_hold_y) kk_native_eye(a, s.md_hold_y), ++s.md_hold_ticks;', '      ++s.md_hold_ticks;'),
 ('fix05_md_hold_forever', A, '    const bool on = k && now <= s.md_hold_until && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH && kk::mortal_draw_cut(k->getCutType()) &&\n                    k->mTargetedActor == static_cast<fopAc_ac_c*>(a);', '    const bool on = k != nullptr;'),
 ('fix05_md_hold_any_death', A, '    if (md && s->brk.active() && k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH) {  // FIX05 P4', '    if (k) {  // FIX05 P4'),

 # ---- LFT 0.13.6 FIX06: each one is a 0.13.5 behaviour (or a weaker fix) the FIX06 laboratory sections reject
 ('fix06_js_no_headroom', A, '    kk_native_health(a, static_cast<int>(a->health) + kk::f6::kJsHeadroom);', '    (void)0;'),
 ('fix06_js_native_loss_kept', A, '    kk_native_health(a, target);\n    if (a->mActionMode == kk::kDead)', '    (void)target;\n    if (a->mActionMode == kk::kDead)'),
 ('fix06_js_no_floor', C, '  return hp_before <= floor ? hp_before : (hp_before - damage < floor ? floor : hp_before - damage);', '  return hp_before - damage + 0 * floor;'),
 ('fix06_js_credit_every_phase', C, '    if (ep == episode) return false;', '    if (false) return false;'),
 ('fix06_js_cut_gain', A, 'kk_posture_gain(*s, a, kk::f6::kGainJsKk, "JUMP_STRIKE", s->cur_serial);', 'kk_posture_gain(*s, a, kk::policy::kGainSwordHp, "JUMP_STRIKE", s->cur_serial);'),
 ('fix06_js_broken_instant_kill', A, '  if (param == 2 && s->brk.active() && kkb_link() && kk::f6::js_cut(kkb_link()->getCutType())) {', '  if (false) {'),
 ('fix06_js_native_power', C, 'inline constexpr int kJsNum = 7, kJsDen = 4;', 'inline constexpr int kJsNum = 20, kJsDen = 1;'),
 ('fix06_js_ordinary_cut_damage', C, '  return ordinary <= 0 ? 0 : ((ordinary * kJsNum + kJsDen / 2) / kJsDen > ordinary ? (ordinary * kJsNum + kJsDen / 2) / kJsDen : ordinary + 1);', '  return ordinary;'),
 ('fix06_js_phases_unbounded', C, 'inline constexpr bool js_cut(int c) noexcept { return c == kCutLargeJumpInit || c == kCutLargeJump || c == kCutLargeJumpFinish; }', 'inline constexpr bool js_cut(int c) noexcept { return c == kCutLargeJump; }'),
 ('fix06_land_recoil_never', A, '      *r = TRUE;\n      kz.lr_take_tick', '      *r = FALSE;\n      kz.lr_take_tick'),
 ('fix06_land_recoil_not_armed', A, '        kk::f6::g_land_recoil.arm(kk::f6::g_jump_id, kkb_tick());  // FIX06 P2: and its landing will recoil natively', '        (void)0;'),
 ('fix06_twirl_parry_needs_rebound', A, '  else if (s.shield_on && s.def.presenting(now, serial) && frontal && kk_spear(a) && k && k->getCutType() == kk::f6::kCutTwirl) v = kk::V::EnemyParry;', '  else if (false) v = kk::V::EnemyParry;'),
 ('fix06_twirl_parry_waits_rebound', A, '        if (k && k->getCutType() == kk::f6::kCutTwirl) {  // FIX06 P4', '        if (false) {  // FIX06 P4'),
 ('fix06_chain_always', C, 'inline constexpr int kChainPct = 65;', 'inline constexpr int kChainPct = 100;'),
 ('fix06_chain_pattern_bias', C, '    pattern = rng.below(4), at = now', '    pattern = rng.below(3), at = now'),
 ('fix06_chain_twice', C, '    if (!completed || counter_token == 0 || counter_token == token) {', '    if (!completed || counter_token == 0) {'),
 ('fix06_chain_after_cut_counter', C, '    if (!completed || counter_token == 0 || counter_token == token) {', '    if (counter_token == 0 || counter_token == token) {'),
 ('fix06_spark_never_moved', C, '  if (o.proj >= kShieldFront) return o;  // already on the attacker', '  if (true) return o;  // already on the attacker'),
 ('fix06_spark_inside_link', C, '  o.p.x = link.x + dx * kShieldFront, o.p.z = link.z + dz * kShieldFront;', '  o.p.x = hit.x, o.p.z = hit.z;'),
 ('fix06_spark_height_unclamped', C, '  o.p.y = link.y + (y < kShieldYMin ? kShieldYMin : y > kShieldYMax ? kShieldYMax : y);', '  o.p.y = link.y + y;'),
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
