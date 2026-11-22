"""LIGHT FADES TWICE 0.13.10 DKN01 FIX01 (base 0.13.9) + 0.13.11 FIX02 + 0.13.12 FIX03 + 0.13.13 FIX04 + 0.13.14 FIX05 + 0.13.15 FIX06: executed mutants of the PRODUCTION Darknut miniboss core (src/dkn_core.hpp) against tests/dkn_lab.cpp,
and (0.13.14) of the LGC01 Link guard cancel core (src/lgc_core.hpp) against tests/lgc_lab.cpp (each mutant runs the lab of the file it mutates).
A mutant counts as detected only when the laboratory reports RESULT=LAB_FAIL. Compile errors and timeouts are FAIL (never credited). The positive control
runs first; every anchor must exist exactly once. Optional sharding: env LIZ_MUTATION_SHARD=i/n."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
C = 'src/dkn_core.hpp'
G = 'src/lgc_core.hpp'  # 0.13.14 LGC01
cases = [
    # approved numbers
    ('hp1_400', C, 'kHp1Max = 600;', 'kHp1Max = 400;'),
    ('f1_hp_420_residual', C, 'kHp1Max = 600;', 'kHp1Max = 420;'),
    ('res2_native_360', C, 'kRes2Max = 480;', 'kRes2Max = 360;'),
    ('f1_posture_120_residual', C, 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;'),
    ('posture1_100', C, 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 100, kPosture2Max = 160, kPosture3Max = 180;'),
    ('posture2_old_130', C, 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 200, kPosture2Max = 130, kPosture3Max = 180;'),
    ('posture3_old_130', C, 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 130;'),
    ('f3_uses_f2_max', C, 'inline int posture_max(Ph p) noexcept { return p == Ph::Revived ? policy::kPosture3Max :', 'inline int posture_max(Ph p) noexcept { return p == Ph::Revived ? policy::kPosture2Max :'),
    ('regen_delay_30', C, 'kRegenDelayTicks = 60;', 'kRegenDelayTicks = 30;'),
    ('regen1_table', C, 'kRegen1[5] = {10, 7, 4, 2, 1};', 'kRegen1[5] = {12, 7, 4, 2, 1};'),
    ('regen2_table', C, 'kRegen2[5] = {12, 8, 5, 2, 1};', 'kRegen2[5] = {12, 8, 4, 2, 1};'),
    ('band_border_upper', C, '  if (r > static_cast<long long>(max) * 75) return 0;', '  if (r >= static_cast<long long>(max) * 75) return 0;'),
    ('band_border_low', C, '  if (r > static_cast<long long>(max) * 10) return 3;', '  if (r >= static_cast<long long>(max) * 10) return 3;'),
    ('break_60', C, 'kBreakTicks = 90;', 'kBreakTicks = 60;'),
    ('down_ticks_150', C, 'kDownTicks = 180;', 'kDownTicks = 150;'),
    ('passive_shield_20', C, 'kPassiveShieldTicks = 38;', 'kPassiveShieldTicks = 20;'),
    ('passive_idle_60', C, 'kPassiveIdleTicks = 75;', 'kPassiveIdleTicks = 60;'),
    # Table A / Table B
    ('kick_parry_14', C, '{15, 0}, 38, 45, 3.0f},', '{14, 0}, 38, 45, 3.0f},'),
    ('vertical_block_25', C, '{14, 0}, 26, 36, 8.0f},', '{14, 0}, 25, 36, 8.0f},'),
    ('f1_vertical_parry_old_11', C, '{14, 0}, 26, 36, 8.0f},', '{11, 0}, 26, 36, 8.0f},'),
    ('f1_lateral_parry_old_9', C, '{12, 0}, 20, 30, 8.0f},', '{9, 0}, 20, 30, 8.0f},'),
    ('f1_blocked_old_2', C, 'kGainSwordHp1 = 6, kGainBlocked1 = 4, kGainIron1 = 25;', 'kGainSwordHp1 = 6, kGainBlocked1 = 2, kGainIron1 = 25;'),
    ('f1_direct_old_4', C, 'kGainSwordHp1 = 6, kGainBlocked1 = 4, kGainIron1 = 25;', 'kGainSwordHp1 = 4, kGainBlocked1 = 4, kGainIron1 = 25;'),
    ('shield_direct_45', C, '{14, 0}, 42, 50, 3.0f},', '{14, 0}, 42, 45, 3.0f},'),
    ('b_cut2_parry_11', C, '{9, 9}, 12, 22, 3.0f},', '{9, 11}, 12, 22, 3.0f},'),
    ('b1_like_b', C, '{11, 0}, 23, 33, 3.0f},\n    // F2 C kick', '{9, 0}, 12, 22, 3.0f},\n    // F2 C kick'),
    ('parry_gives_link_posture', C, '  return o == Outcome::Parry ? 0 : o == Outcome::Block ? i.link_block : i.link_direct;',
     '  return o == Outcome::Parry ? i.link_block : o == Outcome::Block ? i.link_block : i.link_direct;'),
    # combos and rhythm
    ('f2c4_ends_b1', C, '{Move::F2C, Move::F2A, Move::F2B, Move::F2A}', '{Move::F2C, Move::F2A, Move::F2B, Move::F2B1}'),
    ('f2c1_no_b1', C, '{Move::F2B, Move::F2B, Move::F2B, Move::F2B1}', '{Move::F2B, Move::F2B, Move::F2B, Move::F2A}'),
    ('f2c2_double_b_instead_of_two_thrusts', C, '{Move::F2A, Move::F2A, Move::F2A, Move::F2B, Move::F2B}', '{Move::F2A, Move::F2B, Move::F2B, Move::F2B}'),
    ('b1_reaches_second_cut', C, '1.0f, 11.0f, 20.0f, 20.0f, 55.0f', '1.0f, 11.0f, 31.0f, 31.0f, 55.0f'),
    ('b_one_window', C, '{13.0f, 21.0f}, {21.0f, 38.0f}', '{13.0f, 38.0f}, {38.0f, 38.0f}'),
    ('mid_rate_ignored', C, '    if (u <= t + t1) return f + (u - t) * s.rate_mid;', '    if (u <= t + t1) return f + (u - t) * s.rate_in;'),
    ('timeline_drops_out_rate', C, '  const float out = f + (u - t) * s.rate_out, end = seg_end_frame(s);', '  const float out = f + (u - t) * 1.0f, end = seg_end_frame(s);'),
    ('rate_clamp_loose', C, 'inline constexpr float kRateMin = 0.55f, kRateMax = 2.40f;', 'inline constexpr float kRateMin = 0.95f, kRateMax = 1.05f;'),
    # posture model
    ('no_saturation', C, '      units = static_cast<std::int64_t>(max_pts) * kUnits, latched = true;', '      latched = true;'),
    ('gain_while_broken', C, '    if (latched) return Gain::Ignored;\n', ''),
    ('regen_while_broken', C, '    if (latched || units <= 0) return false;', '    if (units <= 0) return false;'),
    ('gain_keeps_delay', C, '    units += static_cast<std::int64_t>(pts) * kUnits, since_gain = 0;', '    units += static_cast<std::int64_t>(pts) * kUnits;'),
    # armour
    ('armor_reaches_12_early', C, '  if (hp <= 0) return 12;', '  if (hp <= 35) return 12;'),
    ('armor_old_420_scale', C, '  int p = (policy::kHp1Max - hp) * 12 / policy::kHp1Max;', '  int p = (420 - hp) * 12 / 420;'),
    ('armor_call_below_broken', C, 'return progress > broken && progress <= 11 ? progress : -1;', 'return progress >= broken && progress <= 11 ? progress : -1;'),
    ('armor_next_ignores_bump', C, '  const int v = progress > broken_after ? progress : broken_after;', '  const int v = progress;'),
    # break controller
    # FIX01 break: no hit cap, executions only by contact, one bounded extension, phase 1 has no executor
    ('break_falls_by_itself', C, '      if (ticks < policy::kBreakTicks) return false;', '      if (ticks < policy::kBreakTicks) return ticks == 40 ? (st = Bk::Exec, false) : false;'),
    ('extension_unbounded', C, '    if (finisher && ext < policy::kFinisherExtMax) return', '    if (finisher) return'),
    ('extension_without_finisher', C, '    if (finisher && ext < policy::kFinisherExtMax) return', '    if (ext < policy::kFinisherExtMax) return'),
    ('helm_prep_twice', C, '    if (st != Bk::StaggerA) return false;\n    st = Bk::StaggerB,', '    if (st != Bk::StaggerA && st != Bk::StaggerB) return false;\n    st = Bk::StaggerB,'),
    # FIX06: the phase-1 Helm Splitter preparation is APPROVED now; the old 'helm_prep_phase1' mutant (allowing it) became the inverse: the FIX05 rule again
    ('helm_prep_f2_only_again', C, '    if (st != Bk::StaggerA) return false;\n    st = Bk::StaggerB,', '    if (!p2 || st != Bk::StaggerA) return false;\n    st = Bk::StaggerB,'),
    ('exec_phase1', C, '    if (!p2 || !(st == Bk::StaggerA || st == Bk::StaggerB)) return false;\n    return st = Bk::Exec,', '    if (!(st == Bk::StaggerA || st == Bk::StaggerB)) return false;\n    return st = Bk::Exec,'),
    ('helm_window_long', C, 'kHelmWindowTicks = 60;', 'kHelmWindowTicks = 600;'),
    ('recover_never', C, '    if (clip_done || ++recover_ticks >= policy::kRecoverMaxTicks) return st = Bk::Combat, true;', '    if (++recover_ticks >= 100000u) return st = Bk::Combat, true;'),
    ('phase_waits_stagger', C, '    if (st != Bk::Combat) why = e;\n    st = Bk::Combat, ticks = b_ticks', '    if (st != Bk::Combat) why = e;\n    if (false) st = Bk::Combat, ticks = b_ticks'),
    # FIX02 0.13.11: invariants, slip caps, timing regression
    ('guardl0_allowed', C, '{ return act == kActGuardL && mode == 0; }', '{ return false; }'),
    ('leap_kept', C, '{ return act == kActChaseL && (mode == 8 || mode == 17); }', '{ return act == kActChaseL && mode == 99; }'),
    ('slip_native', C, '    default: return 10.0f;', '    default: return 20.0f;'),
    ('slip_zero', C, '    case Move::F1A: case Move::F1B: return 12.0f;', '    case Move::F1A: case Move::F1B: return 2.0f;'),
    ('timing_changed', C, '    {144, 0, {{73, 4, 6}, {57, 16, 10},', '    {145, 0, {{73, 4, 6}, {57, 16, 10},'),
    ('b_window_shrunk', C, '{15.5f, 29.5f}, {13.0f, 21.0f}, {21.0f, 38.0f}', '{15.5f, 29.5f}, {15.0f, 21.0f}, {29.0f, 38.0f}'),
    ('hold_rate_zero', C, 'inline constexpr float kHoldCreep = 0.02f;', 'inline constexpr float kHoldCreep = 0.0f;'),
    ('stagger_clip_ends_early', C, '  const float left_frames = (end - 1.0f) - frame;', '  const float left_frames = (end + 6.0f) - frame;'),
    # passivity, selection, ledger
    ('hold_counts', C, '    if (a == Act::Hold || dk_busy) return Pv::None;', '    if (dk_busy) return Pv::None;'),
    ('reading_counts', C, '    if (a == Act::Hold || dk_busy) return Pv::None;', '    if (a == Act::Hold) return Pv::None;'),
    ('parry_is_passive', C, '    case kProcGuardSlip: case kProcGuardBreak: case kProcDamage: case kProcLargeDamageUp: case kProcLandDamage:\n      return Act::Hold;',
     '    case kProcGuardSlip: case kProcGuardBreak: case kProcDamage: case kProcLargeDamageUp: case kProcLandDamage: case kProcGuardAttack:\n      return Act::Hold;'),
    # FIX01 own offensive controller
    ('single_share_100', C, 'inline constexpr int kSinglePct = 25;', 'inline constexpr int kSinglePct = 100;'),
    ('combo_biased', C, '        acc += static_cast<unsigned>(policy::kComboPick[k]);', '        acc += static_cast<unsigned>(policy::kComboPick[k]) + 5u;'),
    ('native_share_back', C, '    ++decisions;\n', '    ++decisions;\n    if (rng.pct(60)) return reject(d, NoInit::Gap);\n'),
    ('no_gap', C, '    if (v.now < gap_until) return reject(d, NoInit::Gap);', '    if (false) return reject(d, NoInit::Gap);'),
    ('busy_starts', C, '    if (v.busy) return reject(d, NoInit::Busy);', '    if (false) return reject(d, NoInit::Busy);'),
    ('attack_from_behind', C, '    if (!v.front) return reject(d, NoInit::NotFront);', '    if (false) return reject(d, NoInit::NotFront);'),
    ('no_reentry', C, '    if (v.now < reentry_until) return reject(d, NoInit::Reentry);', '    if (false) return reject(d, NoInit::Reentry);'),
    ('gap_huge', C, 'kGap2Min = 22, kGap2Max = 46;', 'kGap2Min = 22, kGap2Max = 460;'),
    ('phase1_wrong_plans', C, '    const unsigned base = v.phase2 ? 4u : 0u;', '    const unsigned base = 4u;'),
    # FIX01 Shield Attack authority
    ('sa_any_state_helm', C, '  if (st == Bk::StaggerA && sa_new) return Sa::HelmPrep;', '  if (sa_new) return Sa::HelmPrep;'),  # FIX06: every phase
    ('sa_not_new_helm', C, '  if (st == Bk::StaggerA && sa_new) return Sa::HelmPrep;', '  if (st == Bk::StaggerA) return Sa::HelmPrep;'),
    # FIX01 adaptive defense
    ('parry_outside_window', C, '    if (!window_open(now)) return ++rebound_suppressed, 0;', '    if (!armed) return ++rebound_suppressed, 0;'),
    ('parry_twice_per_window', C, '    parried = true, armed = false, ++parries,', '    parried = false, ++parries,'),
    ('counter_without_rebound', C, '    if ((cp != Cp::Await && cp != Cp::CreditOnly) || !link_rebounding || now < parry_tick) return false;', '    if ((cp != Cp::Await && cp != Cp::CreditOnly) || now < parry_tick) return false;'),
    ('counter_twice', C, '    cp = Cp::None;\n    return counter_idx;', '    return counter_idx;'),
    ('counter_always_kick', C, '      const unsigned k = rng.below(3);', '      const unsigned k = 0;'),
    ('no_parry_cooldown', C, '    if (can_parry && !armed && cp == Cp::None && now >= cooldown_until && chance > 0) {', '    if (can_parry && !armed && cp == Cp::None && chance > 0) {'),
    ('no_prep_delay', C, '      window_from = now + policy::kParryPrepTicks,', '      window_from = now,'),
    ('pressure_ignored', C, '    const int v = base + step * static_cast<int>(pressure);', '    const int v = base + 0 * static_cast<int>(pressure);'),
    # FIX01 executions
    ('exec_outside_break', C, '  const bool hit = (st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm);', '  const bool hit = md || helm;'),
    ('helm_without_shield_attack', C, '  const bool hit = (st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm);', '  const bool hit = (st == Bk::StaggerA && (md || helm)) || (st == Bk::StaggerB && helm);'),
    ('lethal_exec_gets_up', C, '  return lethal ? Ex::Lethal : Ex::Fall;', '  return Ex::Fall;'),
    ('receipt_any_generation', C, '  bool valid(std::uint64_t gen, std::uint64_t now) const noexcept { return on && generation == gen &&', '  bool valid(std::uint64_t gen, std::uint64_t now) const noexcept { return on && (gen || true) &&'),
    # FIX01 heavy phase 1
    ('f1_fast_windup', C, '    p.seg[i].cut = q.cut, p.seg[i].rate_out = r;\n    p.seg[i + 1].start = q.start, p.seg[i + 1].rate_in = 1.0f,', '    p.seg[i].cut = q.cut, p.seg[i].rate_out = r;\n    p.seg[i + 1].start = q.start, p.seg[i + 1].rate_in = 1.6f,'),
    ('f1_old_cut', C, '    {259, 0, {{54, 12, 10}, {69, 4, 6}, {54, 12, 10}, {70, 4, 6}}},', '    {259, 0, {{34, 12, 10}, {69, 4, 6}, {54, 12, 10}, {70, 4, 6}}},'),
    ('f1_no_scale', C, '    {259, 0, {{54, 12, 10}, {69, 4, 6}, {54, 12, 10}, {70, 4, 6}}},', '    {100, 0, {{54, 12, 10}, {69, 4, 6}, {54, 12, 10}, {70, 4, 6}}},'),
    ('f1_short_morph', C, '    {144, 0, {{73, 4, 6}, {57, 16, 10},', '    {144, 0, {{73, 4, 6}, {57, 16, 2},'),
    ('f1_rhythm_ratio_broken', C, '    const float T = (c.peak_ms[i + 1] - c.peak_ms[i]) * 0.03f * p.scale;', '    const float T = (c.peak_ms[i + 1] - c.peak_ms[i]) * 0.03f * p.scale + 9.0f;'),
    ('double_contact', C, '    resolved = serial;\n', ''),
    # FIX03 0.13.12: defense by streak in every phase, counters by thirds, tempo, slips, second stagger
    ('streak_flat', C, 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'inline constexpr int kParryOrdinalPct[6] = {25, 25, 25, 25, 25, 25};'),
    ('ordinal_old_fix03_table', C, 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'inline constexpr int kParryOrdinalPct[6] = {10, 25, 45, 60, 60, 60};'),
    ('ordinal_third_swing_parry', C, 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 25, 25, 45, 60};'),
    ('ordinal_60_read_as_75', C, 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 75};'),
    ('streak_kept_after_parry', C, "    chain = 0, whiffs = 0, last_swing = 0;  // the parry ends Link's chain", ''),
    ('streak_gap_ignored', C, '    if (!last_swing || now < last_swing || now - last_swing > policy::kStreakGapTicks) {', '    if (!last_swing) {'),
    ('guard_x_parry_double_roll', C, '      if (rng.pct(chance)) {', '      if (rng.pct(chance) && rng.pct(50)) {'),
    ('f1_parry_without_shield', C, '    if (can_parry && !armed && cp == Cp::None && now >= cooldown_until && chance > 0) {', '    if (!armed && cp == Cp::None && now >= cooldown_until && chance > 0) {'),
    ('f1_ordinary_guard_roll', C, '    if (p2 && rng.pct(pct(', '    if (rng.pct(pct('),
    ('same_tick_parry_counted_twice', C, '    if (parried && parry_tick == now) return ++repeats, 2;\n', ''),
    ('f1_bash_counter_without_shield', C, '      const unsigned k = shield ? rng.below(3) : 1u + rng.below(2);', '      const unsigned k = rng.below(3);'),
    ('counter_without_kick', C, '      idx = k == 0 ? kIdxF2Kick : k == 1 ? kIdxF2Thrust : kIdxF2Double;', '      idx = k == 0 ? kIdxF2Thrust : k == 1 ? kIdxF2Thrust : kIdxF2Double;'),
    ('link_parry_gain_f2_20', C, 'kLinkEnemyParry1 = 20, kLinkEnemyParry2 = 25;', 'kLinkEnemyParry1 = 20, kLinkEnemyParry2 = 20;'),
    # 0.13.14: 'f2_single_b_back' (FIX03: an ordinary F2 single that is the full double) now mutates a path the approved FIX05 change made unreachable (F2/F3 draw no
    # single: kF2Singles false); its reachable form is the double drawn as an F2 own offensive:
    ('f2_double_as_own_offensive', C, '      d.pick = Pick::Combo, d.idx = base + k, ++combos, ++picked[k];', '      d.pick = Pick::Combo, d.idx = v.phase2 && k == 3 ? kIdxF2Double : base + k, ++combos, ++picked[k];'),
    ('tempo_f1_scaled', C, '  if (!c.phase2) return 1.0f;', '  if (!c.phase2) return 1.25f;'),
    ('tempo_combo_110', C, 'inline constexpr float kF2ComboScale = 1.40f;', 'inline constexpr float kF2ComboScale = 1.10f;'),
    ('tempo_fix03_125', C, 'inline constexpr float kF2ComboScale = 1.40f;', 'inline constexpr float kF2ComboScale = 1.25f;'),
    ('tempo_x140_on_top_of_fix03', C, 'inline constexpr float kF2ComboScale = 1.40f;', 'inline constexpr float kF2ComboScale = 1.75f;'),
    ('singles_fix03_120', C, 'inline constexpr float kF2SingleScale = 1.34f;', 'inline constexpr float kF2SingleScale = 1.20f;'),
    ('kick_fix03_112', C, 'inline constexpr float kKickScale = 1.25f;', 'inline constexpr float kKickScale = 1.12f;'),
    ('tempo_b_inner_unscaled', C, '      const float dt = pair_dt(imp);', '      const float dt = pair_dt(imp) / S;'),  # FIX06: the inner pair through pair_dt
    ('tempo_single_native', C, '    p.seg[0].rate_in = p.seg[0].rate_mid = p.seg[0].rate_out = 1.0f / S;', '    p.seg[0].rate_in = p.seg[0].rate_mid = p.seg[0].rate_out = 1.0f;'),
    ('tempo_kick_like_single', C, 'return c.moves[0] == Move::F2C ? policy::kKickScale : policy::kF2SingleScale;', 'return policy::kF2SingleScale;'),
    ('kick_cut_old_range', C, '0.0f, 3.0f, 12.0f, 28.0f, 29.0f', '0.0f, 3.0f, 12.0f, 20.0f, 29.0f'),
    ('thrust_ranges_fix03', C, '1.0f, 6.0f, 12.0f, 35.0f, 36.0f', '3.0f, 6.0f, 12.0f, 24.0f, 36.0f'),
    ('thrust_start_zero_freeze', C, '1.0f, 6.0f, 12.0f, 35.0f, 36.0f', '0.0f, 6.0f, 12.0f, 35.0f, 36.0f'),
    ('double_ranges_fix03', C, '1.0f, 11.0f, 33.0f, 54.0f, 55.0f', '5.0f, 11.0f, 33.0f, 50.0f, 55.0f'),
    ('slip_no_chain', C, '  const float f = chain == 0 ? 1.0f : chain == 1 ? 0.80f : 0.65f;', '  const float f = 1.0f;'),
    ('slip_chain_zero', C, '  const float f = chain == 0 ? 1.0f : chain == 1 ? 0.80f : 0.65f;', '  const float f = chain == 0 ? 1.0f : 0.0f;'),
    ('slip_mace_old_15', C, '    case Move::F1A: case Move::F1B: return 12.0f;', '    case Move::F1A: case Move::F1B: return 15.0f;'),
    ('helm_clip_guard_knockback', C, 'inline constexpr int kHelmClip = native::kBckTnbDamageR;', 'inline constexpr int kHelmClip = native::kBckTnbDown;'),
    ('helm_hold_sword_rising', C, 'kHelmStart = 0.0f, kHelmMorph = 5.0f, kHelmHold = 12.0f;', 'kHelmStart = 0.0f, kHelmMorph = 5.0f, kHelmHold = 20.0f;'),
    ('expired_window_no_retry', C, '    if (armed && !parried && now >= window_to) armed = false, ++expired;', '    if (armed && !parried && now >= window_to) armed = false, ++expired, cooldown_until = now + 75;'),
    # FIX04 0.13.13: the exchange, the ordinal chain, the confirmed parry, the landing counter, the slip per impact
    ('exchange_ignored', C, '    if (v.defending) return reject(d, NoInit::Defending);', '    if (false) return reject(d, NoInit::Defending);'),
    ('hold_forever', C, '    return last_swing && now >= last_swing && now - last_swing <= policy::kStreakGapTicks && whiffs < policy::kChainWhiffs;', '    return last_swing != 0;'),
    ('hold_ignores_whiffs', C, '    return last_swing && now >= last_swing && now - last_swing <= policy::kStreakGapTicks && whiffs < policy::kChainWhiffs;', '    return last_swing && now >= last_swing && now - last_swing <= policy::kStreakGapTicks;'),
    ('whiff_advances_chain', C, '    if (last_swing && !cur_contacted) ++whiffs;', '    if (last_swing && !cur_contacted) ++chain;'),
    ('swing_counts_without_contact', C, '    cur_key = key, cur_contacted = false, last_swing = now, cur_ord = ordinal();', '    cur_key = key, cur_contacted = false, last_swing = now, ++chain, cur_ord = ordinal();'),
    ('contact_counted_twice', C, '    if (key == parried_key || key != cur_key || cur_contacted || !last_swing) return false;', '    if (key == parried_key || key != cur_key || !last_swing) return false;'),
    ('parried_contact_counted', C, '    if (key == parried_key || key != cur_key || cur_contacted || !last_swing) return false;', '    if (key != cur_key || cur_contacted || !last_swing) return false;'),
    ('same_swing_new_ordinal', C, '    if (last_swing && key == cur_key) return cur_ord;', '    if (false) return cur_ord;'),
    ('reroll_same_swing', C, '    if (rolled_key == cur_key) return ++rerolls_refused, Dv::None;', '    if (false) return ++rerolls_refused, Dv::None;'),
    ('window_survives_next_swing', C, '    if (armed && !parried && window_key != key) armed = false, ++expired;', ''),
    ('counter_in_rebound_tick', C, '  return fi + 1 >= land ? 1u : land - fi;', '  return 0u;'),
    ('counter_lands_in_recoil', C, 'inline constexpr unsigned kCounterLandTicks = 24;', 'inline constexpr unsigned kCounterLandTicks = 10;'),
    ('counter_dead_wait', C, '  return fi + 1 >= land ? 1u : land - fi;', '  return fi + 1 >= land ? 1u : land - fi + 20u;'),
    ('break_keeps_counter', C, '    cp = cp == Cp::Await ? Cp::CreditOnly : Cp::None;', '    cp = cp == Cp::Await ? Cp::Await : cp;'),
    ('break_drops_credit', C, '    cp = cp == Cp::Await ? Cp::CreditOnly : Cp::None;', '    cp = Cp::None;'),
    ('unconfirmed_keeps_credit', C, '    if ((cp == Cp::Await || cp == Cp::CreditOnly) && now > counter_by) cp = Cp::None, ++unconfirmed;', ''),
    ('late_counter_kept', C, '    if (cp == Cp::Scheduled && now > counter_at + policy::kCounterLateTicks) cp = Cp::None, ++counter_dropped;', ''),
    ('parry_contact_becomes_hit', C, '  bool parry_hold(std::uint64_t now) const noexcept { return parried && now >= parry_tick && now <= parry_tick + 1; }', '  bool parry_hold(std::uint64_t now) const noexcept { return false && now; }'),
    ('slip_double_count_fixed_upward', C, 'return link_slip_cap(m, ordinal == 0 ? 1u : 2u); }', 'return link_slip_cap(m, ordinal); }'),
    ('slip_double_count_kept', C, 'return link_slip_cap(m, ordinal == 0 ? 1u : 2u); }', 'return link_slip_cap(m, ordinal * 2u + 2u); }'),
    # FIX05 0.13.14: every defended contact feeds the chain; the exchange verdict; four combos only in F2/F3; shield-held special only; persisting rebound;
    # the counter double on the F2-C1 reference (everything else FIX04 bit for bit)
    ('f2_singles_back', C, 'inline constexpr bool kF2Singles = false;', 'inline constexpr bool kF2Singles = true;'),
    ('f2_single_gate_removed', C, '    if ((!v.phase2 || policy::kF2Singles) && rng.pct(policy::kSinglePct)) {', '    if (rng.pct(policy::kSinglePct)) {'),
    ('f1_singles_removed', C, '    if ((!v.phase2 || policy::kF2Singles) && rng.pct(policy::kSinglePct)) {', '    if ((v.phase2 && policy::kF2Singles) && rng.pct(policy::kSinglePct)) {'),
    ('hold_reach_old_300', C, 'inline float hold_reach(bool phase2) noexcept { return phase2 ? kEngage2 : kEngage1; }', 'inline float hold_reach(bool phase2) noexcept { return phase2 ? kGuardReach : kGuardReach; }'),
    ('hold_reach_unbounded', C, '  if (!(dist < policy::hold_reach(phase2))) return Hold::Far;', '  if (false) return Hold::Far;'),
    ('hold_flank_kept', C, '  if (!front) return Hold::NotFront;', '  if (false) return Hold::NotFront;'),
    ('hold_whiffs_kept', C, '  if (d.last_swing && d.whiffs >= policy::kChainWhiffs) return Hold::EmptySwings;', '  if (false) return Hold::EmptySwings;'),
    ('hold_target_ignored', C, '  if (!link_ok) return Hold::NoTarget;', '  if (false) return Hold::NoTarget;'),
    ('hold_forever', C, '  return d.holds(now) ? Hold::Holds : Hold::Pause;', '  return d.last_swing ? Hold::Holds : Hold::Pause;'),
    ('contact_registration_off', C, '    if (!key || key == parried_key || (last_swing && key <= cur_key)) return false;', '    return false;'),
    ('contact_registration_same_key', C, '    if (!key || key == parried_key || (last_swing && key <= cur_key)) return false;', '    if (!key || key == parried_key || (last_swing && key < cur_key)) return false;'),
    ('contact_registration_parried', C, '    if (!key || key == parried_key || (last_swing && key <= cur_key)) return false;', '    if (!key || (last_swing && key <= cur_key)) return false;'),
    ('contact_registration_rolls', C, '    rolled_key = key;\n    ++late_registered;', '    ++late_registered;'),
    ('track_reach_300', C, 'inline constexpr float kSwingTrackReach = 500.0f;', 'inline constexpr float kSwingTrackReach = 300.0f;'),
    ('track_behind', C, '  if (!front || !(dist < policy::kSwingTrackReach)) return Sw::Ignore;', '  if (!(dist < policy::kSwingTrackReach)) return Sw::Ignore;'),
    ('roll_late_unbounded', C, 'return now >= registered && now - registered <= policy::kRollLateTicks; }', 'return now >= registered; }'),
    ('rebound_init_read_credits', C, '    if (!reverse_seen) return reverse_seen = true, reverse_tick = now, reverse_frame = frame, Rb::Seen;',
     '    if (!reverse_seen) return confirm(now, true, p2, shield) ? Rb::Confirmed : Rb::None;'),
    ('rebound_same_tick_credits', C, '    if (now <= reverse_tick) return Rb::Seen;\n', ''),
    ('rebound_frame_unchecked', C, '    if (!(frame > reverse_frame)) {', '    if (false) {'),
    ('rebound_lost_kept_pending', C, '      if (!reverse_seen) return Rb::Waiting;\n      cp = Cp::None, reverse_seen = false, ++rebound_lost;', '      if (!reverse_seen) return Rb::Waiting;\n      ++rebound_lost;'),
    ('special_idle_kick_f2', C, 'inline bool special_allowed(Pv v, bool phase2) noexcept { return v == Pv::Shield || (v == Pv::Idle && !phase2); }',
     'inline bool special_allowed(Pv v, bool phase2) noexcept { return v != Pv::None && phase2 == phase2; }'),
    ('special_shield_removed', C, 'inline bool special_allowed(Pv v, bool phase2) noexcept { return v == Pv::Shield || (v == Pv::Idle && !phase2); }',
     'inline bool special_allowed(Pv v, bool phase2) noexcept { return v == Pv::Idle && !phase2; }'),
    ('counter_double_native_x134', C, '    if (idx == kIdxF2Double && c.phase2 && !(scale > 0.0f)) return plan_counter_double();', '    if (false) return plan_counter_double();'),
    ('counter_double_ref_c2', C, 'inline constexpr unsigned kCounterDoubleRef = 4;', 'inline constexpr unsigned kCounterDoubleRef = 5;'),
    ('counter_double_slow_windup', C, '  p.seg[0].rate_in = 1.0f, p.seg[0].rate_out = 1.0f;\n  float r = (mi.impact[1]', '  p.seg[0].rate_in = 0.746f, p.seg[0].rate_out = 1.0f;\n  float r = (mi.impact[1]'),
    ('counter_double_one_cut', C, '  p.seg[0].m = Move::F2B, p.seg[0].start = mi.s_min, p.seg[0].cut = -1.0f, p.seg[0].morph = mi.morph;',
     '  p.seg[0].m = Move::F2B, p.seg[0].start = mi.s_min, p.seg[0].cut = 22.0f, p.seg[0].morph = mi.morph;'),
    ('counter_double_scale_twice', C, '  const float dt = (ref.peak_ms[1] - ref.peak_ms[0]) * 0.03f * policy::kF2ComboScale;', '  const float dt = (ref.peak_ms[1] - ref.peak_ms[0]) * 0.03f * policy::kF2ComboScale * 1.40f;'),
    ('combos_retimed', C, 'inline constexpr float kF2ComboScale = 1.40f;', 'inline constexpr float kF2ComboScale = 1.25f;'),
    # LGC01 0.13.14: Link's guard cancel of an ordinary cut (tests/lgc_lab.cpp)
    ('lgc_cancel_inside_at', G, '    if (o.region == Region::Active) return ++buffered, o.v = Verdict::Buffer, o;\n', ''),
    ('lgc_r_before_cut', G, '    if (v.r_held && v.r_down_tick >= cut_tick && !intent)', '    if (v.r_held && !intent)'),
    ('lgc_release_kept', G, '      intent = bash = false;\n    }\n    if (intent && v.b_trig', '    }\n    if (intent && v.b_trig'),
    ('lgc_no_lock_ok', G, '    else if (!v.lock) bad = Why::NoLock;\n', ''),
    ('lgc_unsafe_ok', G, '    else if (!v.safe) bad = Why::Unsafe;\n', ''),
    ('lgc_airborne_ok', G, '    else if (!v.ground) bad = Why::Airborne;\n', ''),
    ('lgc_not_sword_ok', G, '    else if (!v.sword) bad = Why::NotSword;\n', ''),
    ('lgc_cancel_on_b_tick', G, '    if (v.b_trig) {\n      if (o.region == Region::Recovery && holds < policy::kHoldMax)', '    if (false) {\n      if (o.region == Region::Recovery && holds < policy::kHoldMax)'),
    ('lgc_no_hold', G, 'inline constexpr unsigned kHoldMax = 1;', 'inline constexpr unsigned kHoldMax = 0;'),
    ('lgc_bash_without_r', G, '    if (intent && v.b_trig && !bash) bash = true, bash_tick = v.tick;', '    if (v.b_trig && !bash) bash = true, bash_tick = v.tick;'),
    ('lgc_bash_never_expires', G, '    if (bash && v.tick > bash_tick + policy::kBashBufferTicks) bash = false, ++bash_expired;\n', ''),
    ('lgc_lunge_ignored', G, '    if (o.region == Region::Recovery && (v.speed > policy::kRecoverSpeedMax || v.speed < -policy::kRecoverSpeedMax))', '    if (false)'),
    ('lgc_hitstop_resets', G, 'v.frame + 0.01f < last_frame || v.tick > last_tick + policy::kInstanceGapTicks)', 'v.frame + 0.01f < last_frame || v.tick != last_tick + 1)'),
    ('lgc_extra_cut', G, 'return cut == kCutNmVertical || cut == kCutNmStab', 'return cut == 0x10 || cut == kCutNmVertical || cut == kCutNmStab'),
    ('lgc_region_late', G, 'frame < at_start ? Region::Prep : frame < at_end ? Region::Active : Region::Recovery; }', 'frame < at_start ? Region::Prep : frame <= at_end ? Region::Active : Region::Recovery; }'),
    ('lgc_prep_waits', G, 'inline Region region(float frame, float at_start, float at_end) noexcept { return frame < at_start ? Region::Prep :',
     'inline Region region(float frame, float at_start, float at_end) noexcept { return frame < at_start ? Region::Active :'),
    ('lgc_first_recovery_cancels', G, '    if (o.region == Region::Recovery && !native_recovery_ran) return ++buffered, o.v = Verdict::Buffer, o.why = Why::LastAtContact, o;\n', ''),
    ('lgc_not_ordinary_kept', G, '    if (!v.cut_normal || !ordinary_cut(v.cut)) {', '    if (!ordinary_cut(v.cut)) {'),
    # FIX06 0.13.15: the five retimed intervals (only them, as final ticks after x1.40), the parry routes (one roll, 60 / 40, 25 x 4), the continuation (one roll,
    # 80 / 20), the sequence (never stacked, ended once, nothing late), its pure gate (no input of Link's ordinary actions), phase-1 executions (100 total, once,
    # its stagger only), the sway, the reposition verdict and the native jump physics, the sword darkening (definitive death only)
    ('retime_c4_old_value', C, '    {7, 1, 25.5f},', '    {7, 1, 33.18f},'),
    ('retime_scale_confused', C, '    return t > 0.0f ? t : (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03f * S;', '    return t > 0.0f ? t * S : (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03f * S;'),
    ('retime_protected_pair', C, '    {5, 2, 30.0f},', '    {5, 1, 30.0f},'),
    ('retime_on_c1', C, '    {5, 0, 27.0f},', '    {4, 0, 27.0f},'),
    ('retime_double_inner_cut', C, '    {7, 3, 30.0f},', '    {7, 2, 30.0f},'),
    ('retime_leaks_into_scaled_plan', C, '  const bool fix06 = !(scale > 0.0f) && idx < 8;', '  const bool fix06 = idx < 8;'),
    ('retime_ignored', C, '    const float t = fix06 ? iv_target(idx, j) : -1.0f;', '    const float t = -1.0f;'),
    ('route_always_combo', C, '      if (route.pct(policy::kParryComboPct)) {', '      if (route.pct(100)) {'),
    ('route_40_combo', C, 'inline constexpr int kParryComboPct = 60;', 'inline constexpr int kParryComboPct = 40;'),
    ('route_reroll', C, '      if (route.pct(policy::kParryComboPct)) {', '      if (route.pct(policy::kParryComboPct) || route.pct(policy::kParryComboPct)) {'),
    ('route_pick_skewed', C, '        const unsigned k = route.below(4);', '        const unsigned k = route.below(3);'),
    ('route_wrong_phase', C, '        const unsigned cidx = (p2 ? 4u : 0u) + k;', '        const unsigned cidx = (p2 ? 0u : 4u) + k;'),
    ('route_off_in_core', C, '    if (routes) {\n      ++route_rolls;', '    if (false) {\n      ++route_rolls;'),
    ('direct_combo_early', C, 'inline constexpr unsigned kParryComboLandTicks = 27;', 'inline constexpr unsigned kParryComboLandTicks = 20;'),
    ('direct_combo_counter_landing', C, 'inline unsigned parry_combo_delay(unsigned idx) noexcept { return land_delay(idx, policy::kParryComboLandTicks); }',
     'inline unsigned parry_combo_delay(unsigned idx) noexcept { return land_delay(idx, policy::kCounterLandTicks); }'),
    ('direct_combo_dead_wait', C, 'inline unsigned parry_combo_delay(unsigned idx) noexcept { return land_delay(idx, policy::kParryComboLandTicks); }',
     'inline unsigned parry_combo_delay(unsigned idx) noexcept { return land_delay(idx, policy::kParryComboLandMax + 4); }'),
    ('continue_always', C, '    if (!rng.pct(policy::kCounterComboPct)) return ++cont_exit, finish(SqEnd::CounterOnly), false;', '    if (false) return ++cont_exit, finish(SqEnd::CounterOnly), false;'),
    ('continue_50', C, 'inline constexpr int kCounterComboPct = 80;', 'inline constexpr int kCounterComboPct = 50;'),
    ('continue_twice', C, '    if (st != Sq::Counter) return false;\n    ++counters_done', '    if (st != Sq::Counter && st != Sq::Bridge) return false;\n    ++counters_done'),
    ('continue_pick_skewed', C, '    const unsigned k = rng.below(4);\n    ++cont_pick[k], ++cont_combo;', '    const unsigned k = rng.below(2);\n    ++cont_pick[k], ++cont_combo;'),
    ('sequence_stacks', C, '    if (st != Sq::None || plan_idx < 0) return false;  // never stacked over a live sequence', '    if (plan_idx < 0) return false;  // never stacked over a live sequence'),
    ('sequence_lease_leak', C, '    ++combos_done;\n    finish(SqEnd::Done);', '    ++combos_done;'),
    ('sequence_end_keeps_owner', C, '    why = e, ++ended[unsigned(e) < unsigned(SqEnd::Count) ? unsigned(e) : 0], st = Sq::None, direct = false, idx = -1;',
     '    why = e, ++ended[unsigned(e) < unsigned(SqEnd::Count) ? unsigned(e) : 0], direct = false, idx = -1;'),
    ('bridge_starts_on_counter_end', C, '  const std::uint64_t a = counter_end + policy::kBridgeSettleTicks;', '  const std::uint64_t a = counter_end - 20;'),
    ('bridge_waits_forever', C, '      return now > counter_end + policy::kBridgeHoldMax ? -1 : 0;', '      return 0;'),
    ('bridge_ignores_held_link', C, '    if (link_held) {\n      held = true, free_since = 0;', '    if (false) {\n      held = true, free_since = 0;'),
    ('bridge_no_free_margin', C, '    return !held || now >= free_since + policy::kBridgeFreeTicks ? 1 : 0;', '    return 1;'),
    ('gate_reads_distance_as_pressure', C, '  const int b = q.bridge_tick(in.now, in.link_held);', '  const int b = q.bridge_tick(in.now, in.link_held || in.dist < 300.0f);'),
    ('gate_prep_without_schedule', C, '  if (q.st == Sq::Prep) return in.cp == Cp::Scheduled ? (o.g = Gate::Own, o) : (o.why = SqEnd::Dropped, o);', '  if (q.st == Sq::Prep) return o.g = Gate::Own, o;'),
    ('gate_orphan_owner', C, '  if (q.st != Sq::Bridge) return o.why = SqEnd::Refused, o;', '  if (q.st != Sq::Bridge) return o.g = Gate::Own, o;'),
    ('gate_ignores_context', C, '  if (e != SqEnd::None) return o.why = e, o;', '  (void)e;'),
    ('gate_no_watchdog', C, '  if (q.watchdog(in.now)) return o.why = SqEnd::Watchdog, o;', '  (void)0;'),
    ('seq_end_action_lost_natural', C, '  return SqEnd::Refused;  // ACTION_LOST and anything else: the native took the actor', '  return SqEnd::None;  // ACTION_LOST and anything else: the native took the actor'),
    ('f1_exec_140', C, 'inline int f1_exec_hp(int hp) noexcept { return hp > policy::kF1ExecDamage ? hp - policy::kF1ExecDamage : 0; }', 'inline int f1_exec_hp(int hp) noexcept { return hp > 140 ? hp - 140 : 0; }'),
    ('f1_exec_underflow', C, 'inline int f1_exec_hp(int hp) noexcept { return hp > policy::kF1ExecDamage ? hp - policy::kF1ExecDamage : 0; }', 'inline int f1_exec_hp(int hp) noexcept { return hp - policy::kF1ExecDamage; }'),
    ('f1_exec_any_state', C, 'return !phase2 && ((st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm)); }', 'return !phase2 && (md || helm); }'),
    ('f1_exec_in_phase2', C, 'return !phase2 && ((st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm)); }', 'return ((st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm)); }'),
    ('f1_exec_twice', C, '    if (p2 || !(st == Bk::StaggerA || st == Bk::StaggerB)) return false;\n    return st = Bk::Recover,', '    if (p2) return false;\n    return st = Bk::Recover,'),
    ('f1_exec_is_a_fall', C, '    return st = Bk::Recover, recover_ticks = 0, why = BkEnd::Execution, ++f1_execs, true;', '    return st = Bk::Exec, recover_ticks = 0, why = BkEnd::Execution, ++f1_execs, true;'),
    ('sway_frozen', C, 'inline constexpr float kSwayAmp = 3.5f, kSwayPeriod = 24.0f, kSwayRateMax = 1.25f;', 'inline constexpr float kSwayAmp = 0.01f, kSwayPeriod = 24.0f, kSwayRateMax = 1.25f;'),
    ('sway_jolt', C, 'inline constexpr float kSwayAmp = 3.5f, kSwayPeriod = 24.0f, kSwayRateMax = 1.25f;', 'inline constexpr float kSwayAmp = 3.5f, kSwayPeriod = 4.0f, kSwayRateMax = 1.25f;'),
    ('sway_no_reverse_rate_zero', C, '  r = clampf(r, -kSwayRateMax, kSwayRateMax);', '  r = clampf(r, 0.0f, kSwayRateMax);'),
    ('sway_f1_on_armourless_clip', C, 'inline constexpr HelmClip kHelmClipF1 = {native::kBckTnaDamageR, native::kEndTnaDamageR, native::kActDamageH, 2};',
     'inline constexpr HelmClip kHelmClipF1 = {native::kBckTnbDamageR, native::kEndTnbDamageR, native::kActDamageH, 2};'),
    ('repo_back_band_edge', C, '  if (dist < policy::kRepoNear) return !safe', '  if (dist <= policy::kRepoNear) return !safe'),
    ('repo_forward_lands_on_link', C, '  return dist - jump_model(false).dist >= policy::kRepoNear ? Rp::Forward : Rp::ForwardTooClose;', '  return Rp::Forward;'),
    ('repo_unsafe_ignored', C, '  if (dist < policy::kRepoNear) return !safe || wall || (dy < 0 ? -dy : dy) > policy::kRepoMaxDy ? Rp::Unsafe : Rp::Back;', '  if (dist < policy::kRepoNear) return Rp::Back;'),
    ('repo_hold_band_jumps', C, '  if (dist <= policy::kRepoFar) return Rp::Hold;', '  if (dist <= policy::kRepoNear + 1.0f) return Rp::Hold;'),
    ('jump_physics_wrong', C, '  const float sf = back ? 50.0f : 35.0f, g = back ? -4.0f : -1.0f;', '  const float sf = back ? 35.0f : 35.0f, g = back ? -4.0f : -1.0f;'),
    ('tint_any_death', C, 'inline bool sword_tint(bool definitive, float material_color) noexcept { return definitive && material_color != 0.0f; }',
     'inline bool sword_tint(bool definitive, float material_color) noexcept { return material_color != 0.0f; }'),
    ('tint_fixed_black', C, 'inline short tint_value(float material_color) noexcept { return static_cast<short>(material_color); }', 'inline short tint_value(float material_color) noexcept { return material_color != 0.0f ? -50 : 0; }'),
]
TOTAL = len(cases)
shard = os.environ.get('LIZ_MUTATION_SHARD')
if shard:
    i, n = (int(x) for x in shard.split('/'))
    cases = cases[i::n]
LABS = ('dkn_lab', 'lgc_lab')
LAB_OF = {C: ('dkn_lab',), G: ('lgc_lab',)}  # 0.13.14: a mutant runs the lab of the core it mutates


def build(p, lab):
    exe = p / (lab + ('.exe' if os.name == 'nt' else ''))
    src = p / 'tests' / (lab + '.cpp')
    cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', str(src), '/Fe' + str(exe), '/Fo' + str(p / (lab + '.obj'))] if os.name == 'nt' \
        else ['g++', '-std=c++20', '-O1', '-w', '-o', str(exe), str(src)]
    return exe, subprocess.run(cmd, capture_output=True, text=True, timeout=600)


def run(p, labs=LABS):
    out = ''
    for lab in labs:
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
    shutil.copy(root / 'tests' / 'dkn_fix06_lab.inc', p / 'tests' / 'dkn_fix06_lab.inc')  # 0.13.15 FIX06: the dkn_lab section of the FIX06 deltas


for name, rel, old, new in cases:
    assert (root / rel).read_text(encoding='utf-8').count(old) == 1, 'anchor not unique: ' + name
with tempfile.TemporaryDirectory() as td:
    p = Path(td)
    copy(p)
    ok, out = run(p)
    assert ok and out.count('RESULT=LAB_PASS') == len(LABS), out[-2000:]


def one(case):
    name, rel, old, new = case
    with tempfile.TemporaryDirectory() as td:
        p = Path(td)
        copy(p)
        f = p / rel
        f.write_text(f.read_text(encoding='utf-8').replace(old, new, 1), encoding='utf-8')
        ok, out = run(p, LAB_OF[rel])
        detected = (not ok) and 'COMPILE_ERROR' not in out and 'RESULT=LAB_FAIL' in out
        return name, detected, ('' if detected else out[-600:])


with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:
    res = list(ex.map(one, cases))
killed = [n for n, d, _ in res if d]
survived = [(n, o) for n, d, o in res if not d]
# the keys the FULL finalize adds up across shards (as tests/test_dkr_mutations.py): total, count, detected, timeouts
out = {'status': 'DKN_MUTATIONS_PASS' if not survived else 'DKN_MUTATIONS_FAIL', 'positive_control': 'DKN_LAB_PASS', 'total': TOTAL, 'count': len(res), 'detected': len(killed), 'timeouts': 0, 'shard': shard or 'all',
       'survived': [n for n, _ in survived], 'detail': {n: o for n, o in survived}}
print(json.dumps(out, indent=1))
sys.exit(0 if not survived else 1)
