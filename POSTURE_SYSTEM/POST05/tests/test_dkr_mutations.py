"""LIGHT FADES TWICE 0.13.8 DKR01 (+ 0.13.10 FIX01 execution fall): executed mutants of the PRODUCTION Darknut core and adapter against tests/dkr_lab.cpp and tests/dkr_world_lab.cpp.
A mutant counts as detected only when a laboratory reports DKR_LAB_FAIL / DKR_WORLD_LAB_FAIL. Compile errors and timeouts are FAIL (never credited). The
positive control runs first; every anchor must exist exactly once. Optional sharding: env LIZ_MUTATION_SHARD=i/n."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
C = 'src/dkr_core.hpp'
A = 'src/dkr_adapter.inc'
cases = [
 # FIX01 0.13.10: the EXECUTION fall (DKN01 request), the 180-tick getup, the phase-3 definitive Finish
 ('exec_down_300', C, 'kExecDownTicks = 180;', 'kExecDownTicks = 300;'),
 ('exec_down_179', C, 'kExecDownTicks = 180;', 'kExecDownTicks = 179;'),
 ('exec_timer_is_revival', C, '    phase = Phase::Down, timer = exec ? kExecDownTicks : kReviveDelayTicks,', '    phase = Phase::Down, timer = kReviveDelayTicks,'),
 ('exec_getup_consumes_revival', C, '      if (exec && hp_left) return phase = Phase::GettingUp, getup_ticks = 0, Step::GetUp;', '      if (exec && hp_left) return start_rise(Path::Timer);'),
 ('exec_f3_timer_kills', C, '      if (exec && hp_left) return phase = Phase::GettingUp, getup_ticks = 0, Step::GetUp;', '      if (exec && hp_left && consumed()) return phase = Phase::Final, ++finals, Step::FinalDeath;\n      if (exec && hp_left) return phase = Phase::GettingUp, getup_ticks = 0, Step::GetUp;'),
 ('exec_f3_finish_revives', C, '      if (exec && consumed()) return phase = Phase::Final, exec = false, ++finals, Step::FinalDeath;  // FIX01: phase-3', '      if (false) return phase = Phase::Final, exec = false, ++finals, Step::FinalDeath;  // FIX01: phase-3'),
 ('exec_needs_unconsumed', C, '    if (!usable || phase == Phase::Native) return Step::None;\n    phase = Phase::Falling, path = Path::None, hit = false, exec = true,', '    if (!usable || phase == Phase::Native || consumed()) return Step::None;\n    phase = Phase::Falling, path = Path::None, hit = false, exec = true,'),
 ('getup_loses_revival_state', C, '      phase = consumed() ? Phase::Consumed : Phase::Armed, exec = false, ++getups;', '      phase = Phase::Consumed, exec = false, ++getups;'),
 ('getup_without_fix02', A, '        dkr_native_rise(a, dkr_clip());  // the same writer and binding as the resurrection', '        (void)0;  // the same writer and binding as the resurrection'),
 ('getup_consumes_on_clip', A, '        dkr_log("GETUP_START clip=FIX02', '        ++c->revivals;\n        dkr_log("GETUP_START clip=FIX02'),
 ('getup_restores_resistance', A, '      if (s == dkr::Step::GetUpDone) {\n        if (c->watchdog || !ours) ++dz.getup_watchdogs;', '      if (s == dkr::Step::GetUpDone) {\n        a->field_0x6fc = 0;\n        if (c->watchdog || !ours) ++dz.getup_watchdogs;'),
 ('getup_resets_resistance', A, '  a->setBck(0x2A, 2, dkr::kGetupWaitMorph, 1.0f);  // BCK_TNB_WAIT', '  a->setBck(0x2A, 2, dkr::kGetupWaitMorph, 1.0f);  // BCK_TNB_WAIT\n  a->field_0x6fc = 0;'),
 ('final_vanish_mid_finish', A, '      } else if (s == dkr::Step::FinalDeath) {', '      } else if (s == dkr::Step::FinalDeath || c->phase == dkr::Phase::FinishHit) {'),
 ('exec_pending_ignored', A, '  if (dz.exec_pending[ci] == id) {', '  if (false) {'),
 ('exec_request_any_variant', A, '  if (!dz.enabled || !a || !dkr::eligible(dkr_identity(a))) return ++dz.exec_refused, false;', '  if (!dz.enabled || !a) return ++dz.exec_refused, false;'),
 ('unload_kills_living', A, '      if (exec_alive) {  // FIX01', '      if (false) {  // FIX01'),
 ('exec_finish_no_resistance_check', A, '      const dkr::Step s = c->held(dkr_link_view(a), receipt, a->field_0x6fc < kDkrResistance);', '      const dkr::Step s = c->held(dkr_link_view(a), receipt, true);'),
 # the request's numbers and the ONE resurrection
 ('revive_delay_short', C, 'kReviveDelayTicks = 300;', 'kReviveDelayTicks = 299;'),
 ('two_resurrections', C, 'kMaxRevivals = 1;', 'kMaxRevivals = 2;'),
 ('consumed_ignored', C, '    if (phase == Phase::Consumed || consumed()) return phase = Phase::Consumed, Step::NativeDeath;', '    if (false) return phase = Phase::Consumed, Step::NativeDeath;'),
 ('lethal_not_idempotent', C, '    if (phase == Phase::Falling || phase == Phase::Down || phase == Phase::Committed || phase == Phase::FinishHit || phase == Phase::Rising) return Step::None;  // idempotent\n', ''),
 # identity
 ('any_param', C, 'return i.param == kTargetParam && i.type == 1', 'return i.type == 1'),
 ('sword_darknut_taken', C, ' && i.type2 == 0', ''),
 ('switch_darknut_taken', C, ' && i.switch_no == 0xFF', ''),
 ('castle_darknut_taken', C, ' && !i.stay_no2;', ';'),
 ('ineligible_gets_slot', A, '    if (!dkr::eligible(dkr_identity(a))) {', '    if (false) {'),
 # fall, down, timer
 ('down_too_early', C, '    if (native_mode == kModeVanish) {', '    if (native_mode != kModeStart) {'),
 ('timer_not_frozen', C, '    if (l.finishing) return Step::HoldDown;  // the timer freezes while ANY Ending Blow is in flight: the rise never starts mid-jump\n', ''),
 ('commit_any_target', C, '    if (l.finishing && l.on_me && !watchdog) {', '    if (l.finishing && !watchdog) {'),
 ('receipt_ignored', C, '    if (cut_down_hit && phase != Phase::FinishHit) {', '    if (false) {'),
 ('rise_during_back_flip', C, '      if (l.finishing || l.back_jump) {', '      if (l.finishing) {'),
 ('rise_right_after_receipt', C, '        if (++hit_wait_ticks < kFinishWatchdogTicks) return Step::HoldDown;\n', ''),
 ('miss_resets_timer', C, '      phase = Phase::Down, ++misses;', '      phase = Phase::Down, ++misses, timer = kReviveDelayTicks;'),
 ('miss_never_resolves', C, '      phase = Phase::Down, ++misses;  // the Ending Blow missed', '      return Step::HoldDown;  // the Ending Blow missed'),
 ('rise_never_ends', C, '    if (!still_ours || stopped) {', '    if (!still_ours) {'),
 ('rise_cut_short', C, '    if (!still_ours || stopped) {', '    if (!still_ours || stopped || rise_ticks > 40) {'),
 ('retire_keeps_slot', C, '        on[i] = false, slot[i] = Ctl{};\n        return true;', '        slot[i] = Ctl{};\n        return true;'),
 ('table_overflow_reuses', C, '    ++full;\n    return nullptr;', '    ++full;\n    return &slot[0];'),
 # the clip check
 ('crc_skipped', C, '  if (crc32(d, n) != kBckCrc32Value) return Bck::Crc;\n', ''),
 ('loop_rule_lost', C, '  if (b[8] != 0) return Bck::Loop;', '  (void)b;'),
 ('table_bounds_loose', C, '        if (keys == 0 || type > 1 || off + keys * per > pool) return Bck::Tables;', '        if (keys == 0 || type > 1 || off > pool) return Bck::Tables;'),
 # adapter: the body kept, the native left alone
 ('darkening_kept', A, '  a->mMaterialColor = color;\n}', '  (void)color;\n}'),
 ('sword_drop_kept', A, '  if (a->mStates[15] != static_cast<u16>(sword)) a->mStates[15] = static_cast<u16>(sword), ++dz.fall_fix_sword;\n', '  (void)sword;\n'),
 ('lying_sword_live', A, '  a->setSwordAtBit(0);\n  a->mSphC.OffAtSetBit();', '  a->mSphC.OffAtSetBit();'),
 ('lying_shield_live', A, '  a->setSwordAtBit(0);\n  a->mSphC.OffAtSetBit();', '  a->setSwordAtBit(0);'),
 ('lock_lost', A, '  a->mSphC.OffAtSetBit();\n  a->attention_info.flags = attn;', '  a->mSphC.OffAtSetBit();\n  (void)attn;'),
 ('held_mode_is_vanish', A, '  a->mActionMode2 = dkr::kHeldMode;', '  a->mActionMode2 = dkr::kModeVanish;'),
 ('no_down_flag', A, '  a->setDownPos(&pos);\n  a->onDownFlg();', '  a->setDownPos(&pos);'),
 ('down_pos_on_floor', A, '  p.y += 45.0f;', '  p.y += 0.0f;'),
 ('down_pos_not_toward_link', A, '    if (d > 1.0f) p.x += to.x / d * 30.0f, p.z += to.z / d * 30.0f;', '    (void)d;'),
 ('receipt_not_consumed', A, 'DKR_NOINLINE void dkr_native_receipt(daB_TN_c* a) noexcept { a->offDownFlg(); }', 'DKR_NOINLINE void dkr_native_receipt(daB_TN_c* a) noexcept { (void)a; }'),
 ('rise_keeps_down_flag', A, '  a->offDownFlg();\n  a->speedF = 0.0f;\n  a->mpModelMorf2->setAnm(clip,', '  a->speedF = 0.0f;\n  a->mpModelMorf2->setAnm(clip,'),
 ('rise_with_morph', A, '  a->mpModelMorf2->setAnm(clip, 0, 0.0f, 1.0f, 0.0f, -1.0f);', '  a->mpModelMorf2->setAnm(clip, 2, 0.0f, 1.0f, 0.0f, -1.0f);'),
 ('resistance_not_restored', A, '  a->field_0x6fc = 0;\n', ''),
 ('health_not_restored', A, '  a->health = 100;\n', ''),
 ('stays_ending', A, '  a->mActionMode1 = dkr::kActChaseL;\n  a->mActionMode2 = 0;', '  (void)0;\n  a->mActionMode2 = 0;'),
 ('neck_not_restored', A, '  a->mUpdateNeckAngle = neck;\n  a->offHeadLockFlg();\n  a->mActionMode1 = dkr::kActChaseL;\n  a->mActionMode2 = 0;', '  a->offHeadLockFlg();\n  a->mActionMode1 = dkr::kActChaseL;\n  a->mActionMode2 = 0;'),
 ('unload_keeps_clip', A, '  if (a->mpModelMorf2->getAnm() == dkr_clip() && die) {', '  if (false) {'),
 ('unload_immortal', A, '  a->mActionMode2 = dkr::kModeVanish;\n  a->mTimer1 = 10;', '  a->mTimer1 = 10;'),
 ('clip_bound_twice', A, '  if (dz.clip_bound) return true;\n', ''),
 ('bas_kept', A, '  std::memcpy(dkr_clip_store + kDkrBasOffset, &no_bas, sizeof(no_bas));\n', '  (void)no_bas;\n'),
 ('back_flip_unseen', A, '  v.back_jump = p == daAlink_c::PROC_BACK_JUMP || p == daAlink_c::PROC_BACK_JUMP_LAND;', '  v.back_jump = false;'),
 ('commit_never_seen', A, '  v.on_me = v.finishing && k->field_0x280c.getActor() == static_cast<const fopAc_ac_c*>(a);', '  v.on_me = false;'),
 ('delete_keeps_slot', A, '    dz.t.retire(id);\n', ''),
 ('install_half', A, '    (void)mods::hook::uninstall<DkrEnding>(), (void)mods::hook::uninstall<DkrDelete>();\n', ''),
 ('bad_clip_installed', A, '  dz.clip_ok = loaded && dz.check == dkr::Bck::Ok;', '  dz.clip_ok = loaded;'),
 ('second_death_held', A, '    if (c->phase == dkr::Phase::Consumed) ++dz.native_final, dkr_log(',
  '    if (c->phase == dkr::Phase::Consumed) c->phase = dkr::Phase::Falling, ++dz.native_final, dkr_log('),
]
TOTAL = len(cases)
shard = os.environ.get('LIZ_MUTATION_SHARD')
if shard:
    i, n = (int(x) for x in shard.split('/'))
    cases = cases[i::n]
LABS = ('dkr_lab', 'dkr_world_lab')


def build(p, lab):
    exe = p / (lab + ('.exe' if os.name == 'nt' else ''))
    src = p / 'tests' / (lab + '.cpp')
    clip = (p / 'res' / 'darknut' / 'resurrection_A_agil_natural_FIX02.bck').as_posix()
    cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', '/DDKR_BCK_FILE="%s"' % clip, str(src), '/Fe' + str(exe), '/Fo' + str(p / (lab + '.obj'))] if os.name == 'nt' \
        else ['g++', '-std=c++20', '-O1', '-w', '-DDKR_BCK_FILE="%s"' % clip, '-o', str(exe), str(src)]
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
    shutil.copytree(root / 'res', p / 'res')
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
            killed = not ok and ('DKR_LAB_FAIL line=' in out or 'DKR_WORLD_LAB_FAIL line=' in out)
            return {'mutation': name, 'detected': killed, 'status': 'KILLED' if killed else 'FAIL', 'timeout': False, 'tail': None if killed else out[-300:]}
        except subprocess.TimeoutExpired:
            return {'mutation': name, 'detected': False, 'status': 'FAIL', 'timeout': True}


with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('LIZ_MUTATION_WORKERS', '4'))) as pool:
    result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'DKR_LABS_PASS', 'total': TOTAL, 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
