"""LIZALFOS 0.12.1 executed behaviour mutants against the PRODUCTION brain, driven by the native-transcribed lab.
Compile errors and timeouts are FAIL (never credited). Each mutant must be killed by a lab assertion. The positive control runs first."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures

root = Path(__file__).resolve().parents[1]
C = 'src/liz_core.hpp'
cases = [
 # fixed values
 ('hp_370', C, 'kMaxHp = 370', 'kMaxHp = 330'),
 ('gain_sword', C, 'kGainSword = 9', 'kGainSword = 15'),
 ('gain_link_parry', C, 'kGainLinkParry = 15', 'kGainLinkParry = 22'),
 ('gain_iron', C, 'kGainIronBall = 30', 'kGainIronBall = 25'),
 ('gain_block', C, 'kGainBlocked = 5', 'kGainBlocked = 15'),
 ('gain_link_on_parry', C, 'kGainLinkOnEnemyParry = 25', 'kGainLinkOnEnemyParry = 20'),
 ('regen_delay', C, 'kRegenDelayTicks = 60', 'kRegenDelayTicks = 30'),
 ('regen_75', C, 'if (h > 75 * m) return 10;', 'if (h >= 75 * m) return 10;'),
 ('regen_50_edge', C, 'if (h > 50 * m) return 7;', 'if (h >= 50 * m) return 7;'),
 ('regen_25_edge', C, 'if (h > 25 * m) return 4;', 'if (h >= 25 * m) return 4;'),
 ('regen_10_edge', C, 'if (h > 10 * m) return 2;', 'if (h >= 10 * m) return 2;'),
 ('regen_band_value', C, 'if (h > 50 * m) return 7;', 'if (h > 50 * m) return 8;'),
 ('regen_when_latched', C, 'if (!eligible || latched || milli <= 0) return;', 'if (!eligible || milli <= 0) return;'),
 ('regen_ignores_eligibility', C, 'if (!eligible || latched || milli <= 0) return;', 'if (latched || milli <= 0) return;'),
 ('regen_drift', C, 'acc -= d * policy::kTicksPerSecond;', 'acc = 0;'),
 ('gain_keeps_delay', C, "    since = 0;\n    acc = 0;\n    if (milli >= policy::kMaxMilli) {", "    if (milli >= policy::kMaxMilli) {"),
 ('latch_twice', C, 'if (latched || pts <= 0) return false;', 'if (pts <= 0) return false;'),
 ('rearm_partial', C, 'milli = 0, acc = 0, since = 0, latched = false;', 'milli = 10000, acc = 0, since = 0, latched = false;'),
 # composition, rates, draw
 ('p1_missing_tail', C, '{3, 4, {Act::N, Act::D, Act::CFast}, {0.50f, 0.63f}}', '{2, 3, {Act::N, Act::D}, {0.50f}}'),
 ('p2_five_hits', C, '{6, 7, {Act::N, Act::N, Act::N, Act::D, Act::N, Act::CFast}', '{6, 6, {Act::N, Act::N, Act::N, Act::N, Act::N, Act::CFast}'),
 ('p3_six_hits', C, '{6, 9, {Act::D, Act::N, Act::D, Act::N, Act::N, Act::D}', '{6, 6, {Act::N, Act::N, Act::N, Act::N, Act::N, Act::N}'),
 ('p4_swap', C, '{Act::CSlow, Act::CSlow, Act::N, Act::D, Act::CFast}', '{Act::CFast, Act::CSlow, Act::N, Act::D, Act::CSlow}'),
 ('rate_n', C, 'kRateN = 1.25f', 'kRateN = 1.20f'),
 ('rate_d', C, 'kRateD = 1.10f', 'kRateD = 1.25f'),
 ('rate_cfast', C, 'kRateCFast = 1.20f', 'kRateCFast = 1.10f'),
 ('gap_p2', C, '{0.58f, 0.60f, 0.54f, 0.66f, 0.67f}', '{0.70f, 0.60f, 0.54f, 0.66f, 0.67f}'),
 ('gap_p4', C, '{1.34f, 1.26f, 0.52f, 0.63f}', '{1.04f, 1.26f, 0.52f, 0.63f}'),
 ('draw_biased', C, 'pat = rng.below(kPatterns);', 'pat = rng.below(kPatterns - 1);'),
 ('below_modulo_bias', C, 'if (x <= lim) return x % n;', 'if (x <= lim) return (x & 0xFFu) % n;'),
 ('sounds_lost', C, 'const float p = static_cast<float>(k) * 0.05f;', 'const float p = 0.0f;'),
 ('windup_too_short', C, 'kMinWindupFrames = 8.0f', 'kMinWindupFrames = 0.0f'),
 # shield cancellation and continuity
 ('cancel_not_suppressed', C, 'if (!admitted || !ret_is_link || v.action != native::kAttack) return false;', 'return false;'),
 ('stale_frame_not_neutralised', C, 'w.any = true, w.set_frame = true, w.frame = 0;', ''),
 ('combo_stops_at_block', C, 'if (ep.act + 1u >= P.acts) {', 'if (ep.act + 1u >= P.acts || st.contact[unsigned(Hit::Guard)] > 0) {'),
 ('native_decision_in_gap', C, "    if (ep.active) {\n      turn_toward(v, w);\n      return true;\n    }", "    if (ep.active) {\n      turn_toward(v, w);\n      return false;\n    }"),
 ('window_counts_once_for_d', C, 'const std::uint8_t win = (a == Act::D && v.tail == 1) ? 1 : 0;', 'const std::uint8_t win = 0;'),
 ('act_done_after_d1', C, 'if (a != Act::D || ep.window == 1) {\n        ep.act_done = true;', 'if (true) {\n        ep.act_done = true;'),
 ('no_rest', C, 'rest_pending = true;', ''),
 # Link contact
 ('stale_counts', C, 'if (!h.armed_last) return Hit::Stale;', ''),
 ('dup_pays', C, 'if (h.resolved == h.impact) return Hit::Duplicate;', ''),
 ('no_shield_is_guard', C, 'if (!h.shield) return Hit::Direct;', ''),
 ('held_r_parries', C, 'if (!h.pass.valid || !h.pass.guard_attack) return Hit::Guard;', ''),
 ('no_window_check', C, 'if (!post08::Parry::in_window(h.pass)) return Hit::MissedEarly;', ''),
 ('parry_cancels_combo', C, 'if (r == Hit::Parry && p.add(policy::kGainLinkParry)) request_break(v);', 'if (r == Hit::Parry) { p.add(policy::kGainLinkParry); request_break(v); }'),
 ('parry_no_gain', C, 'if (r == Hit::Parry && p.add(policy::kGainLinkParry)) request_break(v);', ''),
 ('double_gap_in_window', C, 'ep.impact > 0 && ep.window == 0 && !ep.window_on && !ep.act_done', 'ep.impact > 0 && ep.window == 0 && !ep.act_done'),
 # reaction / break
 ('stagger_not_suppressed', C, "      if (big) {\n        skipped_big = true;\n        ++st.stagger_suppressed_big;\n      } else {\n        skipped_small = true;\n        ++st.stagger_suppressed_small;\n      }\n      return true;", "      return false;"),
 ('lethal_suppressed', C, 'if (lethal) return false;  // native death, always', ''),
 ('big_invuln_kept', C, 'w.any = true, w.set_invuln = true, w.invuln = 10;', ''),
 ('sword_credit_never', C, 'if (p.add(policy::kGainSword)) request_break(v);', ''),
 ('iron_no_credit', C, 'const bool crossed = !lethal && p.add(policy::kGainIronBall);', 'const bool crossed = false;'),
 ('other_gets_posture', C, 'return false;  // heavy/bomb/arrow/other: native reaction kept (documented native exceptions)', 'p.add(5); return false;'),
 ('bash_not_cleared', C, 'w.any = true, w.clear_tg = true;', ''),
 ('break_never_delivered', C, 'w.any = true, w.stagger = true;', ''),
 ('break_falls_at_crossing', C, 'w.any = true, w.stagger = true;', 'w.any = true, w.big_damage = true;'),
 ('break_overwrites_special_state', C, 'if (!breakable(v.action)) return;', ''),
 ('double_big_on_knockdown', C, "    if (v.action == native::kDamage) {  // already in a native knock-down (a real bomb, ...): no standing window can exist, that fall is the break\n", "    if (false) {\n"),
 ('rearm_early', C, '    if (v.action != native::kDamage) {\n      finish_break(v, false);', '    if (true) {\n      finish_break(v, false);'),
 ('rearm_never', C, '    if (standing) ++st.punish_end;\n    p.rearm();', '    if (standing) ++st.punish_end;'),
 ('horse_suppressed', C, 'if (!admitted || v.event || v.horse || dead) return false;', 'if (!admitted || v.event || dead) return false;'),
 ('event_keeps_combo', C, 'if (ep.active) end_episode(End::Event);', ''),
 ('hp_rewritten', C, "    if (!hp_done) {\n      hp_done = true;", "    if (true) {\n      hp_done = true;"),
 ('hp_foreign_overwritten', C, 'if (v.hp == native::kNativeHp && v.max_hp == native::kNativeHp) {', 'if (true) {'),
 ('dynalfos_admitted', C, 'if (i.arg1 == 1) return Excl::Dynalfos;', ''),
 ('reg_admitted', C, 'if (i.arg0 >= 13 && i.arg0 <= 16) return Excl::ScriptedRun;', ''),
 ('special_room_admitted', C, 'if (i.special_room) return Excl::SpecialRoom;', ''),
 # adaptive
 ('pct_a', C, 'kParryPctA = 12', 'kParryPctA = 20'),
 ('pct_b', C, 'kParryPctB = 35', 'kParryPctB = 30'),
 ('pct_c', C, 'kParryPctC = 45', 'kParryPctC = 60'),
 ('pressure_window', C, 'kPressureWindow = 75', 'kPressureWindow = 200'),
 ('link_parry_clears_pressure', C, "        ++st.link_parry;\n", "        ++st.link_parry;def.clear();\n"),
 ('no_decay', C, '} else if (++since_ok_ >= policy::kDemoteTicks) {', '} else if (false) {'),
 ('far_keeps_memory', C, 'if (++far_ >= policy::kResetFarTicks) {', 'if (false) {'),
 ('cooldown_ignored', C, 'return def.cooled(local) && v.ground', 'return v.ground'),
 ('cooldown_a', C, 'kParryCooldownA = 120', 'kParryCooldownA = 30'),
 ('cooldown_b', C, 'kParryCooldownB = 90', 'kParryCooldownB = 120'),
 ('cooldown_c', C, 'kParryCooldownC = 60', 'kParryCooldownC = 120'),
 ('cooldown_ignores_level', C, 'def.cooldown_until = local + Defense::cooldown_of(def.level);', 'def.cooldown_until = local + policy::kParryCooldownA;'),
 ('window_wider', C, 'kParryWindowLast = 7', 'kParryWindowLast = 12'),
 ('window_earlier', C, 'kParryWindowFirst = 3', 'kParryWindowFirst = 0'),
 ('parry_without_attempt', C, 'if (!attempt_armed || !front || !cooled || now < guard_tick) return false;', 'if (!front || !cooled || now < guard_tick) return false;'),
 ('parry_from_side', C, 'if (!attempt_armed || !front || !cooled || now < guard_tick) return false;', 'if (!attempt_armed || !cooled || now < guard_tick) return false;'),
 ('reroll_every_tick', C, 'rolled_serial = pending_swing_serial;', ''),
 ('swing_any_direction', C, 'if (v.link_human && v.pl_dir < policy::kSwingRange && swing_aimed(L.shape_y, v.search_y)) {', 'if (v.link_human && v.pl_dir < policy::kSwingRange) {'),
 ('swing_any_range', C, 'if (v.link_human && v.pl_dir < policy::kSwingRange && swing_aimed(L.shape_y, v.search_y)) {', 'if (v.link_human && swing_aimed(L.shape_y, v.search_y)) {'),
 ('block_pays_parry_too', C, 'push(EvK::LinkGain, policy::kGainLinkOnEnemyParry);', 'push(EvK::LinkGain, policy::kGainLinkOnEnemyParry);p.add(policy::kGainBlocked);'),
 ('block_gives_turn', C, 'push(EvK::Block, policy::kGainBlocked, p.points(), 0);', 'push(EvK::Block, policy::kGainBlocked, p.points(), 0);arm_counter(0);'),
 ('contact_per_tick', C, 'if (contact_swing == L.swing_serial && L.swing_serial != 0) return;', ''),
 ('counter_counts_as_combo', C, "        ++st.counter_started[counter.kind];", "        ++st.counter_started[counter.kind];++st.selected[0];"),
 ('p1_interrupted', C, 'case 0: return false;', 'case 0: return next_act >= 1;'),
 ('p3_early_interrupt', C, 'case 2: return next_act >= 3 && next_act < kPattern[2].acts;', 'case 2: return next_act >= 1 && next_act < kPattern[2].acts;'),
 ('p4_tail_interrupt', C, 'case 3: return next_act == 1 || next_act == 2;', 'case 3: return next_act >= 1;'),
 ('voluntary_unlimited', C, 'if (voluntary != 0 || level < 1 || pattern >= kPatterns) return false;', 'if (pattern >= kPatterns) return false;'),
 ('guard_watchdog_off', C, 'if (v.action == native::kGuard && in_guard && local - guard_seen_from > policy::kGuardWatchdog) {', 'if (false) {'),
 # --- 0.12.1 pressure
 ('c_needs_six', C, 'kSustainedSwings = 4', 'kSustainedSwings = 6'),
 ('c_window', C, 'kSustainedWindow = 150', 'kSustainedWindow = 300'),
 ('c_needs_old_gate', C, 'return swings_within(now, policy::kSustainedWindow) >= policy::kSustainedSwings; }', 'return swings_within(now, policy::kSustainedWindow) >= policy::kSustainedSwings && false; }'),
 ('lease_short', C, 'kPressureLeaseTicks = 30', 'kPressureLeaseTicks = 6'),
 ('lease_long', C, 'kPressureLeaseTicks = 30', 'kPressureLeaseTicks = 90'),
 ('lease_at_level_a', C, '(def.level >= 1 ? policy::kPressureLeaseTicks : policy::kPendingSwingTicks)', 'policy::kPressureLeaseTicks'),
 ('boundary_uses_lease', C, 'if (swing_fresh() && eligible_boundary(v)', 'if (swing_pending() && eligible_boundary(v)'),
 ('swing_counts_every_tick', C, "      last_swing = L.swing_serial;\n      if (v.link_human", "      if (v.link_human"),
 ('link_gone_keeps_memory', C, "    if (gone && !link_gone) {  // the pressure and the exchange of a Link that is not there never survive\n      def.reset();", "    if (gone && !link_gone) {  // the pressure and the exchange of a Link that is not there never survive\n"),
 ('break_keeps_pressure', C, "    chain_count = 0;\n    def.reset();\n    rest_pending = false;\n    phase = Bk::Punish;", "    chain_count = 0;\n    rest_pending = false;\n    phase = Bk::Punish;"),
 # --- 0.12.1 two blocks and the counter
 ('roll_at_first_block', C, 'if (chain_count < 2) return;', 'if (chain_count < 1) return;'),
 ('roll_needs_three', C, 'if (chain_count < 2) return;', 'if (chain_count < 3) return;'),
 ('chain_never_resets', C, '    chain_count = 0;  // the same two contacts are never counted again, whatever the guard does afterwards\n', ''),
 ('chain_window_ignored', C, 'if (local - chain_tick > policy::kBlockChainTicks) chain_count = 0;', ''),
 ('wound_keeps_chain', C, "          cancel_counter(Cancel::Damage);\n          chain_count = 0;", "          cancel_counter(Cancel::Damage);"),
 ('counter_pct', C, 'kCounterPct = 80', 'kCounterPct = 100'),
 ('counter_pct_low', C, 'kCounterPct = 80', 'kCounterPct = 50'),
 ('counter_kind_biased', C, 'counter.kind = static_cast<std::uint8_t>(rng.below(2));', 'counter.kind = static_cast<std::uint8_t>(rng.below(4) == 0);'),
 ('counter_always_normal', C, 'counter.kind = static_cast<std::uint8_t>(rng.below(2));', 'counter.kind = 0;'),
 ('no_counter_after_parry', C, "      arm_counter(1);\n", ''),
 ('no_guard_exit', C, 'else guard_exit(v, w);', ''),
 ('exit_from_mode5', C, 'if (v.action != native::kGuard || (v.mode != 6 && v.mode != 7)) return;', 'if (v.action != native::kGuard) return;'),
 ('counter_from_back', C, 'facing(v.shape_y, v.search_y, policy::kCounterFaceHalf);', 'true;'),
 ('counter_other_height', C, ' && iabs16(v.search_x) < policy::kCounterHeightHalf &&', ' &&'),
 ('counter_unseen', C, ' && v.seen_timer != 0 && ', ' && '),
 ('counter_during_recoil', C, 'return v.ground && v.recoil < 1.0f && v.hp > 0 && v.link_ok', 'return v.ground && v.hp > 0 && v.link_ok'),
 ('counter_in_air', C, 'return v.ground && v.recoil < 1.0f && v.hp > 0 && v.link_ok', 'return v.recoil < 1.0f && v.hp > 0 && v.link_ok'),
 ('counter_any_range', C, 'v.pl_dir <= native::kAttackInitRange && ', ''),
 ('lease_never_expires', C, 'else if (local - counter.tick > policy::kCounterLeaseTicks) cancel_counter(Cancel::Lease);', ''),
 ('far_keeps_counter', C, 'else if (v.pl_dir > policy::kCounterExpireDistance) cancel_counter(Cancel::Far);', ''),
 ('gone_keeps_counter', C, 'if (!v.link_ok || !v.link_human || !v.link_alive) cancel_counter(Cancel::LinkGone);\n    else if', 'if'),
 ('wound_keeps_counter', C, 'cancel_counter(Cancel::Damage);', ''),
 ('break_keeps_counter', C, "    attempt_armed = false;\n    cancel_counter(Cancel::Break);\n", "    attempt_armed = false;\n"),
 ('event_keeps_counter', C, "      armed_last = armed_now = false;\n      cancel_counter(Cancel::Event);", "      armed_last = armed_now = false;"),
 ('counter_keeps_guard_invuln', C, 'if (v.action == native::kGuard) w.set_invuln = true, w.invuln = 0;', ''),
 ('counter_waits_native_timer', C, 'if ((v.action == native::kFightRun || native_attack || v.action == native::kGuard) && v.mode >= 0 && counter_legal(v)) {', 'if (native_attack && v.mode >= 0 && counter_legal(v)) {'),
 ('double_answer', C, 'if (counter.armed || counter.live) return;  // one response at a time: never a double answer to the same exchange', ''),
 ('counter_is_full_combo', C, 'const unsigned pat = counter.kind == 0 ? kCounterN : kCounterD;', 'const unsigned pat = counter.kind == 0 ? 0u : 2u;'),
 ('counter_wrong_kind', C, 'const unsigned pat = counter.kind == 0 ? kCounterN : kCounterD;', 'const unsigned pat = kCounterN;'),
 ('counter_d_one_impact', C, '{1, 2, {Act::D}, {}},', '{1, 1, {Act::N}, {}},'),
 # --- 0.12.1 standing break
 ('decisive_same_swing', C, 'if (first && v.hp < hp_pre && last_swing != break_swing) {', 'if (first && v.hp < hp_pre) {'),
 ('decisive_no_hp', C, 'if (first && v.hp < hp_pre && last_swing != break_swing) {', 'if (first && last_swing != break_swing) {'),
 ('decisive_never', C, 'if (first && v.hp < hp_pre && last_swing != break_swing) {', 'if (false) {'),
 ('decisive_no_fall', C, 'w.any = true, w.big_damage = true;', ''),
 ('helm_needs_nothing', C, 'if (phase == Bk::Punish && !helm_prepared && !break_pending && shield_serial != 0 && shield_serial != break_shield) {', 'if (phase == Bk::Punish && !helm_prepared && !break_pending) {'),
 ('helm_renews', C, 'if (phase == Bk::Punish && !helm_prepared && !break_pending && shield_serial != 0 && shield_serial != break_shield) {', 'if (phase == Bk::Punish && !break_pending && shield_serial != 0 && shield_serial != break_shield) {'),
 ('bash_always_native_when_latched', C, 'if (phase == Bk::Punish && !helm_prepared && !break_pending && shield_serial != 0 && shield_serial != break_shield) {', 'if (p.latched) {'),
 ('head_lock_from_crossing', C, 'if (!helm_prepared) w.any = true, w.head_lock_off = true;', ''),
 ('head_lock_off_after_prepared', C, 'if (!helm_prepared) w.any = true, w.head_lock_off = true;', 'w.any = true, w.head_lock_off = true;'),
 ('punish_ticks', C, 'kPunishTicks = 90', 'kPunishTicks = 300'),
 ('punish_hold_absent', C, "w.any = true, w.set_timer0 = true;\n", ''),
 ('punish_ends_with_clip', C, 'if (!helm_prepared && local < break_tick + policy::kPunishTicks) {', 'if (false) {'),
 ('prepared_prolongs', C, 'if (!helm_prepared && local < break_tick + policy::kPunishTicks) {', 'if (local < break_tick + policy::kPunishTicks) {'),
 ('iron_cross_keeps_knockdown', C, "        request_break(v);\n        if (big) skipped_big = true;\n        else skipped_small = true;\n        return true;\n      }\n    }\n    return false;", "        request_break(v);\n        return false;\n      }\n    }\n    return false;"),
 ('punish_regens', C, 'if (!eligible || latched || milli <= 0) return;', 'if (!eligible || milli <= 0) return;'),
 ('punish_no_rearm_standing', C, "          finish_break(v, true);\n        }\n        return;", "        }\n        return;"),
 ('down_phase_lost', C, "      phase = Bk::Down;\n      ++st.down_finish;", "      ++st.down_finish;"),
 ('bash_se_never', C, 'w.bash_se = true;', ''),
 # --- 0.12.1 coverage, pure part
 ('probe_every_frame', C, 'p += 0.5f;', 'p += 1.0f;'),
 ('probe_at_rate_one', C, 'if (!(r >= kMinRate && r <= kMaxRate)) return 0;', ''),
 ('first_pose_d2', C, '(win == 1 ? 26.0f : 14.0f)', '(win == 1 ? 25.0f : 14.0f)'),
 ('radius_d2', C, '(win == 1 ? 80.0f : 50.0f)', '(win == 1 ? 50.0f : 50.0f)'),
 ('lunge_back_lost', C, '(win == 1 ? (post - m) * policy::kLungeD2 : 0.0f)', '0.0f'),
 ('sweep_writes_when_touching', C, "  if (cp06f1::touches(k, cur, cp06f1::kWriteSlack, true)) {\n    d.why = Why::AlreadyTouching;\n    return d;\n  }\n", ''),
 ('sweep_writes_off_cylinder', C, 'if (cp06f1::touches(k, c[i], cp06f1::kWriteSlack, true)) {', 'if (true) {'),
 ('tracker_lost_never', C, 'else ++imp.lost, ++st.lost_ticks;', ''),
 ('tracker_counts_nocon_as_lost', C, 'if (imp.crossed_before) ++imp.nocon, ++st.nocon_ticks;\n        else ++imp.lost, ++st.lost_ticks;', '++imp.lost, ++st.lost_ticks;'),
]
def build(p):
 exe = p / ('liz.exe' if os.name == 'nt' else 'liz')
 src = p / 'tests/liz_lab.cpp'
 cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', '/bigobj', str(src), '/Fe' + str(exe), '/Fo' + str(p / 'liz.obj')] if os.name == 'nt' else ['g++', '-std=c++20', '-O1', '-o', str(exe), str(src)]
 return exe, subprocess.run(cmd, capture_output=True, text=True, timeout=300)
def run(p):
 exe, c = build(p)
 if c.returncode:
  return False, 'COMPILE_ERROR ' + c.stdout[-2000:] + c.stderr[-2000:]
 c = subprocess.run([str(exe)], capture_output=True, text=True, timeout=240)
 return c.returncode == 0, c.stdout + c.stderr
def copy(p):
 (p / 'tests').mkdir()
 shutil.copytree(root / 'src', p / 'src')
 shutil.copy(root / 'tests/liz_lab.cpp', p / 'tests/liz_lab.cpp')
with tempfile.TemporaryDirectory() as td:
 p = Path(td)
 copy(p)
 ok, out = run(p)
 assert ok and 'RESULT=LAB_PASS' in out, out[-2000:]
def one(case):
 name, rel, old, new = case
 with tempfile.TemporaryDirectory() as td:
  p = Path(td)
  copy(p)
  f = p / rel
  t = f.read_text(encoding='utf-8')
  if t.count(old) < 1:
   return {'mutation': name, 'detected': False, 'status': 'ANCHOR_MISSING', 'timeout': False}
  f.write_text(t.replace(old, new, 1), encoding='utf-8')
  try:
   ok, out = run(p)
   killed = not ok and 'LIZ_LAB_FAIL line=' in out
   return {'mutation': name, 'detected': killed, 'status': 'KILLED' if killed else 'FAIL', 'timeout': False, 'tail': None if killed else out[-400:]}
  except subprocess.TimeoutExpired:
   return {'mutation': name, 'detected': False, 'status': 'FAIL', 'timeout': True}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('LIZ_MUTATION_WORKERS', '4'))) as pool:
 result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'LIZ_LAB_PASS', 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
