"""LIGHT FADES TWICE FIX03 0.12.3: executed mutants of the PRODUCTION lock-on and parry FX cores against tests/lft03_lab.cpp.
Compile errors and timeouts are FAIL (never credited). The positive control runs first; every anchor must exist exactly once."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
L = 'src/lockon_core.hpp'; X = 'src/parry_fx_core.hpp'
cases = [
 # P1-4 lock-on
 ('tap_window_zero', L, 'kTapFrames = 10', 'kTapFrames = 0'),
 ('second_press_ignored', L, '        if (edge) {  // the second press', '        if (false) {  // the second press'),
 ('release_not_hidden', L, '      case St::Release:\n        o.write = true,', '      case St::Release:\n        o.write = false,'),
 ('release_never_ends', L, '        if (!real) st = St::Idle;  // this frame still hides it', '        if (false) st = St::Idle;  // this frame still hides it'),
 ('switch_option_synthetic_hold', L, 'o.hold = in.mode == Mode::Hold;', 'o.hold = true;'),
 ('switch_option_cycles', L, '          if (in.mode == Mode::Switch) o.force_release = true, ++c.forced_releases;\n', ''),
 ('stick_no_rearm', L, '    if (!stick_armed && ax <= kStickRearm) stick_armed = true;', '    stick_armed = true;'),
 ('stick_engage_low', L, 'kStickEngage = 0.70f', 'kStickEngage = 0.60f'),
 ('stick_outside_lock', L, '        if (st == St::Idle) return o;\n', ''),
 ('option_off_still_writes', L, '    if (!in.enabled || !in.safe) {', '    if (!in.safe) {'),
 ('unsafe_still_writes', L, '    if (!in.enabled || !in.safe) {', '    if (!in.enabled) {'),
 ('npc_made_sekiro', L, '        if (in.locked && in.enemy) {', '        if (in.locked) {'),
 ('lost_keeps_hold', L, '        if (!in.locked || !in.enemy) {  // target gone', '        if (false) {  // target gone'),
 ('hysteresis_removed', L, 'real = in.trigger > kOn ? true : (in.trigger < kOff ? false : real);', 'real = in.trigger > kOff;'),
 ('choose_wrong_side', L, 'const float d = (c[i].x - c[cur].x) * float(dir);', 'const float d = (c[cur].x - c[i].x) * float(dir);'),
 ('choose_takes_invalid', L, 'if (int(i) == cur || !c[i].valid) continue;', 'if (int(i) == cur) continue;'),
 ('choose_farthest', L, 'if (best < 0 || d < bestd) best = int(i), bestd = d;', 'if (best < 0 || d > bestd) best = int(i), bestd = d;'),
 ('choose_no_gap', L, 'if (d <= kMinScreenGap) continue;', 'if (d <= 0.0f) continue;'),
 # P1-3 colour and bookkeeping
 ('orange_drops_min', X, '  o.b = static_cast<std::uint8_t>(mn);', '  o.b = 0;'),
 ('orange_too_yellow', X, 'kHueNum = 47', 'kHueNum = 90'),
 ('orange_too_red', X, 'kHueNum = 47', 'kHueNum = 5'),
 ('orange_drops_value', X, '  o.r = static_cast<std::uint8_t>(mx);', '  o.r = static_cast<std::uint8_t>(mx * 3 / 4);'),
 ('book_no_dedupe', X, '    const int f = find(k, actor, pass);\n    if (f >= 0) {', '    const int f = -1;\n    if (f >= 0) {'),
 ('book_reverts_confirmed', X, 'if (i < 0 || unsigned(i) >= kRecords || r[i].st != St::Preview) return;', 'if (i < 0 || unsigned(i) >= kRecords) return;'),
 ('book_preview_never_expires', X, '      if (x.st == St::Preview && now > x.pass + kConfirmTicks) {', '      if (false) {'),
 ('book_reverted_still_orange', X, 'return i >= 0 && unsigned(i) < kRecords && (r[i].st == St::Preview || r[i].st == St::Confirmed);', 'return i >= 0 && unsigned(i) < kRecords && r[i].st != St::Free;'),
 ('book_unbounded', X, '    ++c.full;\n    return -1;', '    ++c.full;\n    return 0;'),
 ('pitch_out_of_range', X, 'kPitch = 1.12f', 'kPitch = 1.30f'),
 # the const peeks of the collision-pass decision must equal what observe/permits then produce
 ('peek_ignores_restart', 'src/parry_core.hpp', '    if (native_restart_) open = false;\n', ''),
 ('peek_ignores_link_change', 'src/parry_core.hpp', '    if (snap_.valid && id != id_) open = false;\n', ''),
 ('peek_ignores_consumed', 'src/shield_chain_core.hpp', '  return !e.consumed;\n', '  return true;\n'),
 ('peek_ignores_input', 'src/shield_chain_core.hpp', '  if(!safe||!real_clip||!e.serial||e.serial!=native_serial||!e.input)return false;\n  if(!(rate', '  if(!safe||!real_clip||!e.serial||e.serial!=native_serial)return false;\n  if(!(rate'),
]
def build(p):
 exe = p / ('f.exe' if os.name == 'nt' else 'f')
 src = p / 'tests/lft03_lab.cpp'
 cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', str(src), '/Fe' + str(exe), '/Fo' + str(p / 'f.obj')] if os.name == 'nt' else ['g++', '-std=c++20', '-O1', '-w', '-o', str(exe), str(src)]
 return exe, subprocess.run(cmd, capture_output=True, text=True, timeout=300)
def run(p):
 exe, c = build(p)
 if c.returncode:
  return False, 'COMPILE_ERROR ' + c.stdout[-1500:] + c.stderr[-1500:]
 c = subprocess.run([str(exe)], capture_output=True, text=True, timeout=120)
 return c.returncode == 0, c.stdout + c.stderr
def copy(p):
 (p / 'tests').mkdir()
 shutil.copytree(root / 'src', p / 'src')
 shutil.copy(root / 'tests/lft03_lab.cpp', p / 'tests/lft03_lab.cpp')
for name, rel, old, new in cases:
 assert (root / rel).read_text(encoding='utf-8').count(old) == 1, 'anchor not unique: ' + name
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
  f.write_text(f.read_text(encoding='utf-8').replace(old, new, 1), encoding='utf-8')
  try:
   ok, out = run(p)
   killed = not ok and 'LIZ_LAB_FAIL line=' in out
   return {'mutation': name, 'detected': killed, 'status': 'KILLED' if killed else 'FAIL', 'timeout': False, 'tail': None if killed else out[-300:]}
  except subprocess.TimeoutExpired:
   return {'mutation': name, 'detected': False, 'status': 'FAIL', 'timeout': True}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('LIZ_MUTATION_WORKERS', '4'))) as pool:
 result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'LFT03_LAB_PASS', 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
