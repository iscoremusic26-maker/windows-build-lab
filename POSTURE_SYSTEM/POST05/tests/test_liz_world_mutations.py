"""LIZALFOS 0.12.2 + FIX03 0.12.3 executed mutants of the PRODUCTION ADAPTER and brain against liz_world_lab (adapter included verbatim, transcribed E_DN + Link + collider).
Compile errors and timeouts are FAIL (never credited). The positive control runs first."""
from pathlib import Path
import subprocess, tempfile, shutil, json, sys, os, concurrent.futures
root = Path(__file__).resolve().parents[1]
A = 'src/liz_adapter.inc'; C = 'src/liz_core.hpp'; FX = 'src/parry_fx_adapter.inc'; K = 'src/lockon_adapter.inc'
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
 ('hold_writer_lost', A, 'if (w.set_timer0 || w.head_lock_off || w.set_ut2) liz_native_hold(a, w.set_timer0, w.timer0, w.head_lock_off, w.set_ut2);', ''),
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
 # 0.12.2 rebound through the real hook
 ('every_block_rebounds', A, 'if (!parry) *r = FALSE;', ''),
 ('reverse_hook_lost', A, 'mods::hook::add_post<LinkCutReverse>(on_cut_reverse_post)', 'mods::hook::add_post<LinkCutReverse>(on_guard_se_post)'),
 ('reverse_touches_foreign_shield', A, "    if (!here) continue;\n", ''),
 ('reverse_only_last_capsule', A, "    bool here = hit == static_cast<const cCcD_Obj*>(&a->tate_sph);\n    for (int i = 0; i < 3 && !here; ++i) here = hit == static_cast<const cCcD_Obj*>(&a->cc_sph[i]);\n", "    bool here = hit == static_cast<const cCcD_Obj*>(&a->tate_sph) && a->tate_sph.cCcD_ObjHitInf::GetTgHitObj() == static_cast<const cCcD_Obj*>(o);\n"),
 # 0.12.2 guard through the real hooks
 ('guard_hook_lost', A, 'liz_apply(a, s->b.on_guard_post(liz_view(a, f1_link())));', '(void)a;'),
 ('learn_writer_lost', A, 'if (w.clear_learn) liz_native_learn(a);', ''),
 ('learn_writer_all_bits', A, 'a->learn &= ~liz::policy::kLearnSwordMask;', 'a->learn = 0;'),
 ('sword_mark_not_read', A, 'sword = o && o->ChkAtType(kMasks.sword);', ''),
 # 0.12.2 decisive through the real writer
 ('bonus_writer_lost', A, 'if (w.hp_extra > 0) liz_native_hp_bonus(a, w.hp_extra);', ''),
 ('bonus_after_fall', A, "  if (w.hp_extra > 0) liz_native_hp_bonus(a, w.hp_extra);\n", ''),
 # 0.12.2 Link's damage through the real hook
 ('link_bonus_not_applied', A, 'amount += o.extra;', ''),
 ('link_bonus_x2_25', A, 'amount += o.extra;', 'amount += o.extra; amount += amount / 2;'),
 ('link_bonus_only_lizalfos', A, 'in.attacker_enemy = fopAcM_GetGroup(ac) == fopAc_ENEMY_e;', 'in.attacker_enemy = fopAcM_GetName(ac) == fpcNm_E_DN_e;'),
 ('link_episode_any_break', A, 'return lf.enabled && lf.def.extended() && (', 'return lf.enabled && ('),
 ('link_episode_native_only', A, '(lf.def.phase() == lnkfx::Phase::NativeBreak || lf.def.phase() == lnkfx::Phase::PostVulnerable)', '(lf.def.phase() == lnkfx::Phase::NativeBreak)'),
 ('link_episode_all_phases', A, '(lf.def.phase() == lnkfx::Phase::NativeBreak || lf.def.phase() == lnkfx::Phase::PostVulnerable)', 'lf.def.phase() != lnkfx::Phase::Ready'),
 ('link_outside_action', A, 'in.in_action = fx.native_depth == 1 && !fx.native_reentered && n.valid && n.human && n.tick == g.p8.tick;', 'in.in_action = true;'),
 ('link_poly_blind', A, 'in.poly_clear = n.poly_seen && n.poly_result == 0;', 'in.poly_clear = true;'),
 ('link_block_blind', A, 'in.blocked = n.blocked;', ''),
 ('link_parent_blind', A, 'if (fopAcM_SearchByID(fopAcM_GetLinkId(ac), &parent) && parent) in.parent_enemy = fopAcM_GetGroup(parent) == fopAc_ENEMY_e;', ''),
 ('link_dup_tick_lost', A, 'ld.last_applied_tick = g.p8.tick;', ''),
 ('link_hook_in_lizalfos_gate', A, 'ld.enabled = lf.enabled && mods::hook::add_pre', 'ld.enabled = ok && lf.enabled && mods::hook::add_pre'),
 # lifecycle
 ('orphan_slot_after_delete', A, '    liz_retire(*s);\n    ++lz.deleted;', '    ++lz.deleted;'),
 ('orphan_events_lost_on_delete', A, '    if (s->b.admitted) liz_drain(*s, a);\n    liz_retire(*s);', '    liz_retire(*s);'),
 ('ghost_attacker_is_enemy', A, 'in.attacker_enemy = fopAcM_GetGroup(ac) == fopAc_ENEMY_e;', 'in.attacker_enemy = true;'),
 ('all_or_nothing_broken', A, 'lz.enabled = ok;', 'lz.enabled = true;'),
 ('address_reuse_inherits', A, 'if (s && s->id != id) {', 'if (false) {'),
 ('shutdown_keeps_running', A, "  liz_summary();\n  lz.enabled = false;", "  liz_summary();"),
 ('summary_counters_lost', A, 'deflect_armed=%llu', 'deflect_armd=%llu'),
 # FIX03 0.12.3 P0-2: the shield covers what its sphere faces; the guard turns at the native rate
 ('fix03_cover_body_yaw', C, 'pass_cover = tate_covers(v);', 'pass_cover = facing(v.shape_y, v.search_y, policy::kFrontHalfAngle);'),
 ('fix03_cover_always', C, 'pass_cover = tate_covers(v);', 'pass_cover = true;'),
 ('fix03_ahead_min_zero', C, 'kShieldAheadMin = 10.0f', 'kShieldAheadMin = 0.0f'),
 ('fix03_no_back_limit', C, '    if (!facing(v.shape_y, v.search_y, policy::kShieldBackLimit)) return false;\n', ''),
 ('fix03_turn_uncapped', C, 'const std::int16_t d = step > policy::kGuardTurnMax ? policy::kGuardTurnMax : step < -policy::kGuardTurnMax ? static_cast<std::int16_t>(-policy::kGuardTurnMax) : step;', 'const std::int16_t d = static_cast<std::int16_t>(diff);'),
 ('fix03_turn_removed', C, '    if (v.mode >= 5 && v.mode <= 8) guard_turn(v, w);\n', ''),
 ('fix03_invuln_body_yaw', C, 'const bool front = v.shield_ok ? covers(v, v.shield_c) : facing(v.shape_y, v.search_y, policy::kFrontHalfAngle);', 'const bool front = facing(v.shape_y, v.search_y, policy::kFrontHalfAngle);'),
 ('fix03_credit_body_yaw', C, 'const bool front = pass_cover_local == local ? pass_cover : tate_covers(v);', 'const bool front = facing(v.shape_y, v.search_y, policy::kFrontHalfAngle);'),
 ('fix03_shield_geometry_not_read', A, '    v.shield_ok = true;\n', ''),
 # FIX03 P0-1/P1-3: the pass-time decision equals the verdict (the peek mutants run against lft03_lab, the HitIn dedupe against liz_lab)
 ('fix03_peek_age_off_by_one', 'src/parry_core.hpp', '    v.age_ticks = open ? static_cast<std::uint32_t>(tick - st) : 0;', '    v.age_ticks = open ? static_cast<std::uint32_t>(tick - st + 1) : 0;'),
 ('fix03_settle_inverted', A, 'pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry != parries);', 'pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry == parries);'),
 ('fix03_exec_scope_left_open', A, '  pfx_liz_exec_end();\n', ''),
 ('fix03_enemy_parry_unconfirmed', A, '        pfx_settle(pfx::Kind::LizalfosOnLink, s.id, true);', '        (void)0;'),
 # FIX03 P1-3 through the production adapter
 ('fx_orange_on_ordinary_blocks', FX, '  if (!parry || !other) return HOOK_CONTINUE;', '  if (!other) return HOOK_CONTINUE;'),
 ('fx_emitter_colour_not_applied', FX, '    if (orange || s->colored) {', '    if (false) {'),
 ('fx_particles_left_blue', FX, '    if (pf.enabled && pf.book.orange_now(s->rec)) pfx_native_colors(e, s->prm, s->env, true);\n', ''),
 ('fx_native_callback_dropped', FX, '    if (s->prev) s->prev->executeAfter(e);\n', ''),
 ('fx_alpha_changed', FX, '  return GXColor{o.r, o.g, o.b, c.a};', '  return GXColor{o.r, o.g, o.b, 255};'),
 ('fx_revert_keeps_orange', FX, '  if (!orange) return GXColor{original.r, original.g, original.b, c.a};\n', ''),
 ('fx_high_clash_on_every_block', FX, "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r) && !pf.book.r[r].sound) pf.ctx_sound = r;", "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  pf.ctx_sound = 0;"),
 ('fx_clash_pitched_twice', FX, "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r) && !pf.book.r[r].sound) pf.ctx_sound = r;", "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r)) pf.ctx_sound = r;"),
 ('fx_clash_not_pitched', FX, '  pfx_native_pitch(h, base * pfx::kPitch);\n', ''),
 ('fx_pitch_not_reverted', FX, '    pfx_native_pitch(pf.last_handle, pf.last_pitch);\n', ''),
 ('fx_enemy_shield_se_not_pitched', FX, "  const int r = pf.book.find(pfx::Kind::LizalfosOnLink, pf.exec_id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r) && !pf.book.r[r].sound) pf.ctx_sound = r;", "  const int r = -1;\n  (void)r;"),
 ('fx_bash_se_taken_for_shield', FX, ' || mods::arg<u32>(args, 2) != 40) return HOOK_CONTINUE;', ') return HOOK_CONTINUE;'),
 ('fx_enemy_own_spark_missed', FX, '    pf.ctx_mark = pf.book.find(pfx::Kind::LizalfosOnLink, pf.exec_id, g.p8.tick - 1);\n', ''),
 ('fx_no_dedupe_in_pass', 'src/parry_fx_core.hpp', '    const int f = find(k, actor, pass);\n    if (f >= 0) {', '    const int f = -1;\n    if (f >= 0) {'),
 ('fx_scene_change_keeps_pointers', FX, '  if (ep != pf.epoch) {', '  if (false) {'),
 ('fx_shutdown_leaves_callback', FX, 'x.e->mpEmtrCallBack == g_pfx_cb) pfx_native_callback(x.e, x.prev);', 'x.e->mpEmtrCallBack == g_pfx_cb) (void)x;'),
 ('fx_stale_record_never_freed', FX, '      if (x.rec == rec) x = PfxEmitter{};', '      (void)x;'),
 ('fx_oc_latched_previewed', FX, ' || e.latched || e.frozen', ' || e.frozen'),
 ('fx_oc_not_swinging_previewed', FX, '  if (mode != post08::policy::kModeAttack || (state != 1 && state != 2)) return false;\n', ''),
 ('fx_oc_outside_window_previewed', FX, '  if (!post08::Parry::in_window(p)) return false;\n', ''),
 ('fx_install_not_all_or_nothing', FX, '  for (auto x : r) ok = ok && x == MOD_OK;\n  pf.enabled = ok;', '  pf.enabled = true;'),
 # FIX03 P1-4 through the production adapter
 ('lk_no_persistence', K, '  lk.enabled = value;\n', '  lk.enabled = true;\n'),
 ('lk_hardcoded_on', K, '  in.enabled = lk.enabled;\n', '  in.enabled = true;\n'),
 ('lk_change_not_seen', K, '  if (svc_config->subscribe) (void)svc_config->subscribe(mod_ctx, h, lockon_option_changed, nullptr, nullptr);\n', ''),
 ('lk_default_off', K, '  desc.default_bool = true;', '  desc.default_bool = false;'),
 ('lk_toggle_not_bound', K, '  d.binding = UI_BINDING_CONFIG_VAR;', '  d.binding = UI_BINDING_CALLBACKS;'),
 ('lk_wrong_label', K, 'objetivos estilo Sekiro', 'objetivos'),
 ('lk_unsafe_writes', K, '  in.safe = lockon_safe(att);', '  in.safe = att != nullptr;'),
 ('lk_wolf_allowed', K, '  return !link->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF) && !link->checkHorseRide();', '  return !link->checkHorseRide();'),
 ('lk_horse_allowed', K, '  return !link->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF) && !link->checkHorseRide();', '  return !link->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);'),
 ('lk_camera_side_flipped', K, 'side = rx * -fz + rz * fx;', 'side = rx * fz - rz * fx;'),
 ('lk_npc_candidate', K, '    out[i].valid = att->mLockOnList[i].mType == fopAc_attn_BATTLE_e;', '    out[i].valid = true;'),
 ('lk_npc_locked_sekiro', K, 'att->mLockOnList[att->mLockOnOffset % n].mType == fopAc_attn_BATTLE_e;', 'true;'),
 ('lk_stick_not_muted', K, '  if (o.mute_stick) {', '  if (false) {'),
 ('lk_flick_not_applied', K, '      lockon_native_attention(att, false, best, pid[best]);\n', ''),
 ('lk_hold_not_written', K, '    p.mHoldLockL = o.hold ? 1 : 0;\n', ''),
 ('lk_switch_release_lost', K, '  if (pad && o.force_release) lockon_native_attention(att, true, 0, 0), ++lk.writes_attention;\n', ''),
 ('lk_runs_unavailable', K, '  if (!lk.available) return;\n  dAttention_c* att', '  dAttention_c* att'),
 ('lk_shutdown_keeps_running', K, '  lk.available = false;  // nothing is synthesized', '  // nothing is synthesized'),
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
