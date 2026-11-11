"""LIGHT FADES TWICE 0.13.0 CHILFOS01: executed mutants of the PRODUCTION Chilfos core and adapter against tests/kk_lab.cpp and tests/kk_world_lab.cpp.
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
 ('snap_not_restored', A, '  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;\n  if (!s->ep.active || s->ep.ph != KkEp::Melee) return;', '  s->clip.snapped = false;\n  if (!s->ep.active || s->ep.ph != KkEp::Melee) return;'),
 ('crossing_strict', C, '    if (a < e && e < b && !(fired_mask >> i & 1u)) {', '    if (false) {'),
 ('duplicate_never_flagged', C, '    if (b == e && (fired_mask >> i & 1u)) c.duplicate = true;', ''),
 # C3 tail (D)
 ('frame23_omitted', C, 'inline constexpr int kThrowEvents[4] = {9, policy::kThrowSpawnFrame, 0x22, 0x3D};', 'inline constexpr int kThrowEvents[4] = {9, 22, 0x22, 0x3D};'),
 ('throw_emitted_twice', C, '    if (a < e && e < b && !(fired_mask >> i & 1u)) {', '    if (a <= e && e < b) {'),
 ('melee_cancel_not_held', A, '    kk_native_eye(a, -1.0e9f);', '    (void)0;'),
 ('eye_not_restored', A, '  if (s->eye_saved) kk_native_eye(a, s->eye_y), s->eye_saved = false;\n  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;\n  if (s->ep.ph == KkEp::Throw',
  '  s->eye_saved = false;\n  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;\n  if (s->ep.ph == KkEp::Throw'),
 ('spearless_transition_broken', A, '  if (s->ep.ph == KkEp::Back && idx == kk::policy::kWeaponRestore) return HOOK_SKIP_ORIGINAL;\n', ''),
 ('restore_clip_while_armed', A, '    if (idx == kk::policy::kBckRestoreB) return HOOK_SKIP_ORIGINAL;  // the spear is still in the hand: no restore clip\n', ''),
 ('restore_not_completed', A, '      s->ep.ph = KkEp::Restore, s->ep.last_progress = kkb_tick();\n      ++s->st.restores;', '      ++s->st.restores;'),
 ('wall_ignored', A, '  if (!s || !s->admitted || !s->ep.active || s->ep.ph != KkEp::Back || a->mMoveMode != 3) return;\n  ++s->st.early_throw', '  if (true) return;\n  ++s->st.early_throw'),
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
 ('helm_open_outside_break', A, '      ++s->st.headlock_stripped;\n      kk_native_headlock(a);', '      ++s->st.headlock_stripped;'),
 ('break_never_entered', A, '  if (s->brk.st == kk::Bk::Pending && a->health > 1 && a->mActionMode != kk::kIcicle && a->mActionMode != kk::kWeaponMove) kk_enter_break(*s, a);', '  if (false) kk_enter_break(*s, a);'),
 ('break_exit_keeps_headlock', A, '  s.p.reset();\n  kk_native_headlock(a);', '  s.p.reset();'),
 # defense (H, I)
 ('block_forever', C, '    if (wound_streak == 0 || now - last_wound > policy::kWoundMemory) return policy::kBlockBase;', '    return 100;'),
 ('parry_and_damage', A, '    case kk::V::EnemyParry:\n      kkb_pfx_settle(false, s->id, true);', '    case kk::V::EnemyParry:\n      kk_native_spear(a, 1), s->conv = true;\n      kkb_pfx_settle(false, s->id, true);'),
 ('parry_from_flank', A, '  else if (s.shield_on && s.def.presenting(now, serial) && frontal && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;', '  else if (s.shield_on && s.def.presenting(now, serial) && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;'),
 ('block_from_flank', A, '  else if (kk_guard_ready(s, a) && frontal && s.rng.pct(s.def.block_pct(now))) v = kk::V::Block;', '  else if (kk_guard_ready(s, a) && s.rng.pct(s.def.block_pct(now))) v = kk::V::Block;'),
 ('rebound_never_suppressed', A, '      *r = FALSE;  // an ordinary block: no rebound', '      (void)r;  // an ordinary block: no rebound'),
 ('counter_never', A, '  s.counter_pending = true, s.counter_until = now + kk::policy::kCounterTtl;', '  s.counter_until = now + kk::policy::kCounterTtl;'),
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
 ('event_keeps_combo', A, '    if (s->ep.active) kk_end_episode(*s, a, kk::End::Event);\n    return HOOK_CONTINUE;', '    return HOOK_CONTINUE;'),
 ('rest_never', A, '    if (!s->counter_pending && now < s->rest_until) {  // the rest after an episode', '    if (false) {  // the rest after an episode'),
 ('poise_lost', A, '            ++s->st.poise;\n            kk_native_action(a, kk::kAttack, 3);', '            ++s->st.poise;'),
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
