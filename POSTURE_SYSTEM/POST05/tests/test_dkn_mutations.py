"""LIGHT FADES TWICE 0.13.10 DKN01 FIX01 (base 0.13.9): executed mutants of the PRODUCTION Darknut miniboss core (src/dkn_core.hpp) against tests/dkn_lab.cpp.
A mutant counts as detected only when the laboratory reports RESULT=LAB_FAIL. Compile errors and timeouts are FAIL (never credited). The positive control
runs first; every anchor must exist exactly once. Optional sharding: env LIZ_MUTATION_SHARD=i/n."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
C = 'src/dkn_core.hpp'
cases = [
    # approved numbers
    ('hp1_400', C, 'kHp1Max = 420;', 'kHp1Max = 400;'),
    ('res2_native_360', C, 'kRes2Max = 480;', 'kRes2Max = 360;'),
    ('posture1_100', C, 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 100, kPosture2Max = 160, kPosture3Max = 180;'),
    ('posture2_old_130', C, 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 120, kPosture2Max = 130, kPosture3Max = 180;'),
    ('posture3_old_130', C, 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 130;'),
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
    ('vertical_block_25', C, '{11, 0}, 26, 36, 8.0f},', '{11, 0}, 25, 36, 8.0f},'),
    ('shield_direct_45', C, '{14, 0}, 42, 50, 3.0f},', '{14, 0}, 42, 45, 3.0f},'),
    ('b_cut2_parry_11', C, '{9, 9}, 12, 22, 3.0f},', '{9, 11}, 12, 22, 3.0f},'),
    ('b1_like_b', C, '{11, 0}, 23, 33, 3.0f},\n    // F2 C kick', '{9, 0}, 12, 22, 3.0f},\n    // F2 C kick'),
    ('parry_gives_link_posture', C, '  return o == Outcome::Parry ? 0 : o == Outcome::Block ? i.link_block : i.link_direct;',
     '  return o == Outcome::Parry ? i.link_block : o == Outcome::Block ? i.link_block : i.link_direct;'),
    # combos and rhythm
    ('f2c4_ends_b1', C, '{Move::F2C, Move::F2A, Move::F2B, Move::F2A}', '{Move::F2C, Move::F2A, Move::F2B, Move::F2B1}'),
    ('f2c1_no_b1', C, '{Move::F2B, Move::F2B, Move::F2B, Move::F2B1}', '{Move::F2B, Move::F2B, Move::F2B, Move::F2A}'),
    ('f2c2_double_b_instead_of_two_thrusts', C, '{Move::F2A, Move::F2A, Move::F2A, Move::F2B, Move::F2B}', '{Move::F2A, Move::F2B, Move::F2B, Move::F2B}'),
    ('b1_reaches_second_cut', C, '5.0f, 11.0f, 20.0f, 20.0f, 55.0f', '5.0f, 11.0f, 31.0f, 31.0f, 55.0f'),
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
    ('armor_call_below_broken', C, 'return progress > broken && progress <= 11 ? progress : -1;', 'return progress >= broken && progress <= 11 ? progress : -1;'),
    ('armor_next_ignores_bump', C, '  const int v = progress > broken_after ? progress : broken_after;', '  const int v = progress;'),
    # break controller
    # FIX01 break: no hit cap, executions only by contact, one bounded extension, phase 1 has no executor
    ('break_falls_by_itself', C, '      if (ticks < policy::kBreakTicks) return false;', '      if (ticks < policy::kBreakTicks) return ticks == 40 ? (st = Bk::Exec, false) : false;'),
    ('extension_unbounded', C, '    if (finisher && ext < policy::kFinisherExtMax) return', '    if (finisher) return'),
    ('extension_without_finisher', C, '    if (finisher && ext < policy::kFinisherExtMax) return', '    if (ext < policy::kFinisherExtMax) return'),
    ('helm_prep_twice', C, '    if (!p2 || st != Bk::StaggerA) return false;\n    st = Bk::StaggerB,', '    if (!p2 || (st != Bk::StaggerA && st != Bk::StaggerB)) return false;\n    st = Bk::StaggerB,'),
    ('helm_prep_phase1', C, '    if (!p2 || st != Bk::StaggerA) return false;\n    st = Bk::StaggerB,', '    if (st != Bk::StaggerA) return false;\n    st = Bk::StaggerB,'),
    ('exec_phase1', C, '    if (!p2 || !(st == Bk::StaggerA || st == Bk::StaggerB)) return false;\n    return st = Bk::Exec,', '    if (!(st == Bk::StaggerA || st == Bk::StaggerB)) return false;\n    return st = Bk::Exec,'),
    ('helm_window_long', C, 'kHelmWindowTicks = 60;', 'kHelmWindowTicks = 600;'),
    ('recover_never', C, '    if (clip_done || ++recover_ticks >= policy::kRecoverMaxTicks) return st = Bk::Combat, true;', '    if (++recover_ticks >= 100000u) return st = Bk::Combat, true;'),
    ('phase_waits_stagger', C, '    if (st != Bk::Combat) why = e;\n    st = Bk::Combat, ticks = b_ticks', '    if (st != Bk::Combat) why = e;\n    if (false) st = Bk::Combat, ticks = b_ticks'),
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
    ('sa_any_state_helm', C, '  if (phase2 && st == Bk::StaggerA && sa_new) return Sa::HelmPrep;', '  if (phase2 && sa_new) return Sa::HelmPrep;'),
    ('sa_not_new_helm', C, '  if (phase2 && st == Bk::StaggerA && sa_new) return Sa::HelmPrep;', '  if (phase2 && st == Bk::StaggerA) return Sa::HelmPrep;'),
    # FIX01 adaptive defense
    ('parry_outside_window', C, '    if (!window_open(now)) return ++rebound_suppressed, false;', '    if (!armed) return ++rebound_suppressed, false;'),
    ('parry_twice_per_window', C, '    parried = true, armed = false, ++parries,', '    parried = false, ++parries,'),
    ('counter_without_rebound', C, '    if (!counter_pending || !link_rebounding) return -1;', '    if (!counter_pending) return -1;'),
    ('counter_twice', C, '    counter_pending = false;\n    const int k = rng.pct', '    const int k = rng.pct'),
    ('counter_always_thrust', C, '    const int k = rng.pct(policy::kCounterPct) ? 0 : 1;', '    const int k = 0;'),
    ('no_parry_cooldown', C, '    if (now >= cooldown_until && !armed && rng.pct(', '    if (!armed && rng.pct('),
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
]
TOTAL = len(cases)
shard = os.environ.get('LIZ_MUTATION_SHARD')
if shard:
    i, n = (int(x) for x in shard.split('/'))
    cases = cases[i::n]
LABS = ('dkn_lab',)


def build(p, lab):
    exe = p / (lab + ('.exe' if os.name == 'nt' else ''))
    src = p / 'tests' / (lab + '.cpp')
    cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', str(src), '/Fe' + str(exe), '/Fo' + str(p / (lab + '.obj'))] if os.name == 'nt' \
        else ['g++', '-std=c++20', '-O1', '-w', '-o', str(exe), str(src)]
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
    assert ok and out.count('RESULT=LAB_PASS') == 1, out[-2000:]


def one(case):
    name, rel, old, new = case
    with tempfile.TemporaryDirectory() as td:
        p = Path(td)
        copy(p)
        f = p / rel
        f.write_text(f.read_text(encoding='utf-8').replace(old, new, 1), encoding='utf-8')
        ok, out = run(p)
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
