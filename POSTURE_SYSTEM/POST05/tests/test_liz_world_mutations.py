"""LIZALFOS 0.12.1 executed mutants of the PRODUCTION ADAPTER and brain against liz_world_lab (adapter included verbatim, transcribed E_DN + Link + collider).
Compile errors and timeouts are FAIL (never credited). The positive control runs first."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
A = 'src/liz_adapter.inc'; C = 'src/liz_core.hpp'
cases = [
 # P0-4 the defect: mute contact during Link's GUARD_SLIP
 ('slip_feedback_removed', A, 'liz_native_link_feedback(link, &c);', ''),
 ('slip_feedback_every_record', A, 'if (s->b.feedback_impact == s->b.impact_counter) return;  // a second record of the same swing never sounds twice', ''),
 ('slip_feedback_without_slip', A, "lz.slip_pre_tick != g.p8.tick || link->mProcID != daAlink_c::PROC_GUARD_SLIP || link->mDamageTimer != 0", "lz.slip_pre_tick != g.p8.tick || link->mDamageTimer != 0"),
 ('slip_pre_always_true', A, 'lz.slip_pre = link && link == f1_link() && link->mProcID == daAlink_c::PROC_GUARD_SLIP;', 'lz.slip_pre = true;'),
 ('slip_feedback_on_body_contact', A, 'if (!c.cCcD_ObjHitInf::ChkTgHit() || !c.ChkTgShieldHit()) continue;', 'if (!c.cCcD_ObjHitInf::ChkTgHit()) continue;'),
 ('slip_foreign_attacker', A, 'if (!s) continue;\n', 'if (!s) s = &lz.slots[0];\n'),
 ('no_speed_kick', A, 'if (k->mNormalSpeed < liz::native::kLinkSlipSpeed) k->mNormalSpeed = liz::native::kLinkSlipSpeed;', ''),
 ('receipt_hook_lost', A, '  ++lz.se_native;\n', ''),
 # re-arm between impacts: two real presses
 ('no_rearm_after_block', A, 'c4.shield.armed = true;\n', ''),
 ('rearm_outside_gap', A, 'if (s->b.impact_gap() && lz.d1_parry_id == s->id) lz.double_gap_tick = g.p8.tick;', 'lz.double_gap_tick = g.p8.tick;'),
 ('rearm_never', A, 'if (s->b.impact_gap() && lz.d1_parry_id == s->id) lz.double_gap_tick = g.p8.tick;', ''),
 ('rearm_after_no_press', A, "if (slip && c4.shield.can_restart(g.p8.tick, safe, true)) {", "if (slip) {"),
 ('rearm_too_early', C, 'kDoubleRearmTicks = 4', 'kDoubleRearmTicks = 12'),
 ('arm_when_nothing_owed', A, '&& s->b.owes_impact()) {', ') {'),
 ('impact_gap_inside_window', C, 'return ep.active && ep.impact > 0 && !ep.window_on && ep.impact < kPattern[ep.pattern].impacts;', 'return ep.active && ep.impact > 0 && ep.impact < kPattern[ep.pattern].impacts;'),
 ('shield_provenance_removed', A, 'if (L.pass.guard_attack && !cp04_guard_pass()) L.pass.guard_attack = false;', ''),
 ('parry_confirm_removed', A, '(void)c4.shield.confirm(g.p8.tick);', ''),
 # front range
 ('velocity_kept', A, 'if (k->mTgCyls[0].ChkTgShield() || k->mTgCyls[0].ChkTgSpShield()) {', 'if (false) {'),
 ('velocity_zero_without_shield', A, 'if (k->mTgCyls[0].ChkTgShield() || k->mTgCyls[0].ChkTgSpShield()) {', 'if (false && k) {'),
 # coverage
 ('coverage_off', A, 'lz.coverage = ok;', 'lz.coverage = false;'),
 ('sphere_not_written', A, 'liz_native_at_place(a, true, d.place.x, d.place.y, d.place.z, false);', ''),
 ('frame_not_restored', A, '  liz_native_pose_frame(m, restore);\n', ''),
 ('restore_not_verified', A, 'for (int c = 0; c < 4 && same; ++c) same = q[r][c] == lz.snap[i][r][c];', 'for (int c = 0; c < 4 && same; ++c) (void)q;'),
 ('wrong_joint', A, 'tail ? liz::native::kJntTail04 : liz::native::kJntHandR1', 'liz::native::kJntHandR1'),
 ('sword_offset_lost', A, 'const float ly = tail ? 0.0f : liz::native::kSwordY;', 'const float ly = 0.0f;'),
 ('radius_widened', A, 'cand[i].r = cur.r;', 'cand[i].r = cur.r + 60.0f;'),
 ('expect_ignores_link_off', A, 'expect = cp06f1::touches(cy, cur, cp06f1::kWriteSlack, true);', 'expect = cp06f1::touches(cy, cur, cp06f1::kWriteSlack, false);'),
 ('sweep_with_stale_link', A, 'const bool current = k && f1.link_set_tick == g.p8.tick && f1.link_set_key == reinterpret_cast<std::uintptr_t>(k);', 'const bool current = k != nullptr;'),
 # break through the writers
 ('stagger_writer_lost', A, 'if (w.stagger) liz_native_stagger(a);', ''),
 ('stagger_wrong_action', A, 'a->action = liz::native::kDrawback;', 'a->action = liz::native::kDamage;'),
 ('hold_writer_lost', A, 'if (w.set_timer0 || w.head_lock_off) liz_native_hold(a, w.set_timer0, w.timer0, w.head_lock_off);', ''),
 ('head_lock_not_cleared', A, 'if (head_off) a->actor.offHeadLockFlg();', ''),
 ('shield_serial_lost', A, 'L.shield_serial = c4.shield.native_serial;', ''),
 ('cut_helm_lost', A, 'v.cut_helm = k && k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP;', ''),
 ('bash_se_every_contact', C, 'if (shield_serial != bash_se_serial) {', 'if (true) {'),
 ('bash_se_writer_lost', A, 'liz_native_bash_se(a, o);', ''),
 ('swing_guard_attack_counts', A, 'if (t == daPy_py_c::CUT_TYPE_NONE || t == daPy_py_c::CUT_TYPE_GUARD_ATTACK) return;', ''),
 # counter through the writers
 ('run_post_binding_lost', A, 'const liz::Write w = s->b.on_run_post(liz_view(a, f1_link()), lz.swing_jump);', 'const liz::Write w{};'),
 # cancellation of the double
 ('native_cancel_back', A, 'if (s->b.on_at_hit_post(liz_view(a, f1_link()), is_link)) *r = nullptr;', '(void)is_link;'),
 # lifecycle
 ('all_or_nothing_broken', A, 'lz.enabled = ok;', 'lz.enabled = true;'),
 ('address_reuse_inherits', A, 'if (s && s->id != id) {', 'if (false) {'),
 ('shutdown_keeps_running', A, "  liz_summary();\n  lz.enabled = false;", "  liz_summary();"),
 ('summary_counters_lost', A, 'priority_armed=%llu', 'priority_armd=%llu'),
]
def build(p):
 exe = p / ('w.exe' if os.name == 'nt' else 'w')
 src = p / 'tests/liz_world_lab.cpp'
 cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', '/bigobj', str(src), '/Fe' + str(exe), '/Fo' + str(p / 'w.obj')] if os.name == 'nt' else ['g++', '-std=c++20', '-O1', '-w', '-o', str(exe), str(src)]
 return exe, subprocess.run(cmd, capture_output=True, text=True, timeout=420)
def run(p):
 exe, c = build(p)
 if c.returncode:
  return False, 'COMPILE_ERROR ' + c.stdout[-1500:] + c.stderr[-1500:]
 c = subprocess.run([str(exe)], capture_output=True, text=True, timeout=240)
 return c.returncode == 0, c.stdout + c.stderr
def copy(p):
 (p / 'tests').mkdir()
 shutil.copytree(root / 'src', p / 'src')
 shutil.copy(root / 'tests/liz_world_lab.cpp', p / 'tests/liz_world_lab.cpp')
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
   return {'mutation': name, 'detected': killed, 'status': 'KILLED' if killed else 'FAIL', 'timeout': False, 'tail': None if killed else out[-300:]}
  except subprocess.TimeoutExpired:
   return {'mutation': name, 'detected': False, 'status': 'FAIL', 'timeout': True}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('LIZ_MUTATION_WORKERS', '4'))) as pool:
 result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'LIZ_WORLD_LAB_PASS', 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
