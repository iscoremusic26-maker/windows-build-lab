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
 ('run_post_binding_lost', A, 'const liz::Write w = s->b.on_run_post(liz_view(a, f1_link()), lz.swing_jump, liz_link_frame(f1_link()).at_eta);', 'const liz::Write w{};'),
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
 ('fix04_cover_ignores_shield_side', C, 'return rel >= 0 ? rel <= policy::kShieldSideLimit : rel >= static_cast<std::int16_t>(-policy::kOpenSideLimit);', 'return facing(v.shape_y, v.search_y, policy::kFrontHalfAngle);'),
 ('fix04_cover_always', C, 'pass_cover = contact_is_parry(v, last_swing, local + 1) || (presented(v) && covers(v));', 'pass_cover = true;'),
 ('fix04_shield_side_too_wide', C, 'kShieldSideLimit = 0x5000', 'kShieldSideLimit = 0x7000'),
 ('fix04_shield_side_too_narrow', C, 'kShieldSideLimit = 0x5000', 'kShieldSideLimit = 0x3000'),
 ('fix04_open_side_too_wide', C, 'kOpenSideLimit = 0x3000', 'kOpenSideLimit = 0x6000'),
 ('fix04_open_side_too_narrow', C, 'kOpenSideLimit = 0x3000', 'kOpenSideLimit = 0x1000'),
 ('fix04_held_contact_rejected', C, 'pass_cover = contact_is_parry(v, last_swing, local + 1) || (presented(v) && covers(v));', 'pass_cover = contact_is_parry(v, last_swing, local + 1) || (presented(v) && covers(v) && v.mode != 7);'),
 ('fix04_held_credit_rejected', C, 'const bool front = judged ? pass_cover : presented(v) && covers(v);', 'const bool front = v.mode != 7 && (judged ? pass_cover : presented(v) && covers(v));'),
 ('fix04_not_presented_blocks', C, 'pass_cover = contact_is_parry(v, last_swing, local + 1) || (presented(v) && covers(v));', 'pass_cover = contact_is_parry(v, last_swing, local + 1) || covers(v);'),
 ('fix04_verdict_not_reused', C, 'const bool judged = pass_cover_local + 1 == local;', 'const bool judged = pass_cover_local == local;'),
 ('fix04_parry_uses_block_arc', C, 'parry_contact(attempt_armed, guard_tick, now, facing(v.shape_y, v.search_y, policy::kFrontHalfAngle), def.cooled(now));', 'parry_contact(attempt_armed, guard_tick, now, covers(v), def.cooled(now));'),
 ('fix04_sphere_only_for_aimed_swing', C, 'if (new_swing && v.ut2 > 0) w.any = true, w.set_ut2 = true;', 'if (aimed_swing && v.ut2 > 0) w.any = true, w.set_ut2 = true;'),
 ('fix04_shield_alone_not_read', A, '  liz_contact_reads(a, k, v);  // FIX04: always', '  if (sword) liz_contact_reads(a, k, v);  // FIX04: always'),
 ('fix04_trace_lost', C, '    push(EvK::Trace, int(tr.verdict), 0, 0);\n', ''),
 ('fix04_trace_block_as_repeat', C, 'st.blocks != tr.blocks ? Tv::Block : ', ''),
 ('fix04_trace_damage_as_repeat', C, 'v.hp < tr.hp_before ? Tv::BodyDamage : ', ''),
 ('fix03_turn_uncapped', C, 'const std::int16_t d = step > policy::kGuardTurnMax ? policy::kGuardTurnMax : step < -policy::kGuardTurnMax ? static_cast<std::int16_t>(-policy::kGuardTurnMax) : step;', 'const std::int16_t d = static_cast<std::int16_t>(diff);'),
 ('fix03_turn_removed', C, '    if (v.mode >= 5 && v.mode <= 8) guard_turn(v, w);\n', ''),
 ('fix04_held_body_open', C, 'const bool front = covers(v);', 'const bool front = covers(v) && v.mode != 7;'),
 # FIX03 P0-1/P1-3: the pass-time decision equals the verdict (the peek mutants run against lft03_lab, the HitIn dedupe against liz_lab)
 ('fix03_peek_age_off_by_one', 'src/parry_core.hpp', '    v.age_ticks = open ? static_cast<std::uint32_t>(tick - st) : 0;', '    v.age_ticks = open ? static_cast<std::uint32_t>(tick - st + 1) : 0;'),
 ('fix03_settle_inverted', A, 'pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry != parries);', 'pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry == parries);'),
 ('fix03_exec_scope_left_open', A, '  pfx_liz_exec_end();\n', ''),
 ('fix03_enemy_parry_unconfirmed', A, '        pfx_settle(pfx::Kind::LizalfosOnLink, s.id, true);', '        (void)0;'),
 # FIX03 P1-3 through the production adapter
 ('fx_orange_on_ordinary_blocks', FX, '  if (parry) {\n    pf.ctx_pass = pf.book.open(kind, id, g.p8.tick);', '  if (kind != pfx::Kind::None) {\n    pf.ctx_pass = pf.book.open(kind, id, g.p8.tick);'),
 ('fx_emitter_colour_not_applied', FX, '    if (orange || s->colored) {', '    if (false) {'),
 ('fx_particles_left_blue', FX, '      pfx_native_colors(e, s->prm, s->env, true);\n      if (s->child_static', '      if (s->child_static'),
 ('fx_native_callback_dropped', FX, '    if (s->prev) s->prev->executeAfter(e);\n', ''),
 ('fx_alpha_changed', FX, '  return GXColor{o.r, o.g, o.b, c.a};', '  return GXColor{o.r, o.g, o.b, 255};'),
 ('fx_revert_keeps_orange', FX, '  if (!orange) return GXColor{original.r, original.g, original.b, c.a};\n', ''),
 ('fx_high_clash_on_every_block', FX, "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r) && !pf.book.r[r].sound) pf.ctx_sound = r;", "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  pf.ctx_sound = 0;"),
 ('fx_clash_pitched_twice', FX, "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r) && !pf.book.r[r].sound) pf.ctx_sound = r;", "  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);\n  if (r >= 0 && pf.book.orange_now(r)) pf.ctx_sound = r;"),
 ('fx_clash_not_pitched', FX, '  pfx_native_pitch(h, base * pfx::kPitch);\n', ''),
 ('fx_pitch_not_reverted', FX, '    pfx_native_pitch(pf.last_handle, pf.last_pitch);\n', ''),
 ('fx_enemy_shield_se_not_pitched', FX, '    if (e >= 0 && pf.book.orange_now(e) && !pf.book.r[e].sound) pf.ctx_sound = e;\n', ''),
 ('fx_enemy_clash_any_owner', FX, 'static_cast<const void*>(mods::arg<Z2SoundObjBase*>(args, 0)) == pf.exec_sound && ', ''),
 ('fx_bash_se_taken_for_shield', FX, ' && mods::arg<u32>(args, 2) == 40) {', ') {'),
 ('fx_enemy_own_spark_missed', FX, '    pf.ctx_mark = pf.book.find(pfx::Kind::LizalfosOnLink, pf.exec_id, g.p8.tick - 1);\n', ''),
 ('fx_no_dedupe_in_pass', 'src/parry_fx_core.hpp', '    const int f = find(k, actor, pass);\n    if (f >= 0) {', '    const int f = -1;\n    if (f >= 0) {'),
 ('fx_scene_change_keeps_pointers', FX, '  if (ep != pf.epoch) {', '  if (false) {'),
 ('fx_shutdown_leaves_callback', FX, 'x.e->mpEmtrCallBack == g_pfx_cb) pfx_native_callback(x.e, x.prev);', 'x.e->mpEmtrCallBack == g_pfx_cb) (void)x;'),
 ('fx_stale_record_never_freed', FX, '      if (x.rec == rec) x = PfxEmitter{};', '      (void)x;'),
 ('fx_oc_latched_previewed', FX, ' || e.latched || e.frozen', ' || e.frozen'),
 ('fx_oc_not_swinging_previewed', FX, '  if (mode != post08::policy::kModeAttack || (state != 1 && state != 2)) return false;\n', ''),
 ('fx_oc_outside_window_previewed', FX, '  if (!post08::Parry::in_window(p)) return false;\n', ''),
 ('fx_decision_hook_failure_ignored', FX, 'const bool decide = lz.enabled && r[kPfxHitmarkPre] == MOD_OK && r[kPfxHitmarkPost] == MOD_OK;', 'const bool decide = lz.enabled;'),
 ('fx_spark_hook_failure_ignored', FX, 'pf.spark = decide && r[kPfxMarkPre] == MOD_OK && r[kPfxMarkPost] == MOD_OK && r[kPfxParticlePost] == MOD_OK;', 'pf.spark = decide;'),
 ('fx_sound_hook_failure_ignored', FX, 'pf.sound = decide && r[kPfxGuardPre] == MOD_OK && r[kPfxGuardPost] == MOD_OK && r[kPfxCollisionPost] == MOD_OK;', 'pf.sound = decide;'),
 ('fx_all_or_nothing_again', FX, '  pf.enabled = pf.spark || pf.sound;', '  pf.enabled = pf.spark && pf.sound;'),
 ('fx_hook_failure_not_named', FX, 'kPfxHookNames[i], kPfxHookKinds[i], int(r[i]), tg[i] ? 1 : 0,', 'kPfxHookNames[i], kPfxHookKinds[i], 0, tg[i] ? 1 : 0,'),
 ('fx_callback_not_counted', FX, '  ++pf.calls[kPfxCollisionPost];\n', ''),
 ('fx_readback_after_owner_deleted', FX, 'bool alive = pf.readback_owner == 0;', 'bool alive = true;'),
 ('fx_oc_first_contact_no_preview', FX, '(g.p8.layout < 0 ? !parry_layout_ok(a) : g.p8.layout != 1)', 'g.p8.layout != 1'),
 ('fx_pitch_readback_lost', FX, '  pf.readback = h, pf.readback_tick = g.p8.tick,', '  pf.readback_tick = g.p8.tick,'),
 # FIX03 P1-4 through the production adapter
 ('lk_no_persistence', K, '  lk.enabled = value;\n', '  lk.enabled = true;\n'),
 ('lk_hardcoded_on', K, '  in.enabled = lk.enabled;\n', '  in.enabled = true;\n'),
 ('lk_change_not_seen', K, '  if (svc_config->subscribe) (void)svc_config->subscribe(mod_ctx, h, lockon_option_changed, nullptr, nullptr);\n', ''),
 ('lk_default_off', K, '  desc.default_bool = true;', '  desc.default_bool = false;'),
 ('lk_toggle_not_bound', K, '  d.binding = UI_BINDING_CONFIG_VAR;', '  d.binding = UI_BINDING_CALLBACKS;'),
 ('lk_wrong_label', K, 'objetivos estilo Sekiro', 'objetivos'),
 # FIX05: the combo reaches the shield
 ('fix05_rearm_at_swing_start', C, '(L.at_eta < 0 || L.at_eta <= policy::kRepresentLeadTicks)', 'true'),
 ('fix05_rearm_never_fires', C, 'kRepresentLeadTicks = 2', 'kRepresentLeadTicks = -1'),
 ('fix05_rearm_too_early', C, 'kRepresentLeadTicks = 2', 'kRepresentLeadTicks = 6'),
 ('fix05_rearm_late_lead_zero_loses_sweep', C, 'kParryWindowLast = 7', 'kParryWindowLast = 4'),
 ('fix05_parry_not_presented', C, 'verdict_parry = presented(v) && parry_contact(', 'verdict_parry = parry_contact('),
 ('fix05_at_done_spends_token', C, '      if (L.at_done) {\n        deflect.wait_serial = 0;', '      if (false) {\n        deflect.wait_serial = 0;'),
 ('fix05_spin_spends_token', C, 'else if (L.can_rebound && (L.at_eta', 'else if ((L.at_eta'),
 ('fix05_no_rebound_still_parry', C, 'if (parry && rebound_seen_swing != L.swing_serial) {', 'if (false) {'),
 ('fix05_conflict_ignored', C, '} else if (parry && v.hp < hp_pre) {', '} else if (false) {'),
 ('fix05_token_not_returned', C, '        deflect = spent;\n', ''),
 ('fix05_rearm_presented_lost', C, '(v.mode == 5 && guard_rearm) || (v.mode == 6', '(v.mode == 6'),
 ('fix05_prepare_certain', C, 'const bool armed = rng.below(100) < static_cast<std::uint32_t>(policy::kDeflectPct);', 'const bool armed = true;'),
 ('fix05_entry_not_timed', C, 'if (deflect_swing && link_eta > policy::kEntryLeadMax) return w;', ''),
 ('fix05_entry_opens_unreachable', C, 'if (deflect_swing && (link_eta < 0 || link_eta >= policy::kEntryLeadMin)) {', 'if (deflect_swing) {'),
 ('fix05_eta_rounds_down', C, 'return static_cast<float>(i) < d ? i + 1 : i;', 'return i;'),
 ('fix05_eta_not_read', A, '      L.at_eta = liz::eta_ticks(f, k->mUnderFrameCtrl[0].getRate(), k->field_0x3478);\n', ''),
 ('fix05_at_done_not_read', A, '      L.at_done = f >= k->field_0x347c;\n', ''),
 ('fix05_finish_proc_not_read', A, 'if (proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH) {', 'if (proc == daAlink_c::PROC_CUT_NORMAL) {'),
 ('fix05_spin_may_rebound', A, '      L.can_rebound = proc == daAlink_c::PROC_CUT_REVERSE;', ''),
 ('fix05_entry_eta_not_passed', A, 'lz.swing_jump, liz_link_frame(f1_link()).at_eta);', 'lz.swing_jump);'),
 # FIX05: the whole clash is orange
 ('fx_copy_not_used', FX, '    if (orange && s->clone) pfx_native_work(e, reinterpret_cast<JPAResource*>(s->clone->res));\n', ''),
 ('fx_prm_table_not_tinted', FX, '    bc->mpPrmClrAnmTbl = x.prm;\n', ''),
 ('fx_env_table_not_tinted', FX, '    bc->mpEnvClrAnmTbl = x.env;\n', ''),
 ('fx_shared_table_written', FX, 'x.prm[i] = tint(b->mpPrmClrAnmTbl[i]);', 'x.prm[i] = tint(b->mpPrmClrAnmTbl[i]), b->mpPrmClrAnmTbl[i] = x.prm[i];'),
 ('fx_copy_on_ordinary_block', FX, '    if (orange && s->clone) pfx_native_work(', '    if (s->clone) pfx_native_work('),
 ('fx_child_born_blue', FX, '    rc->pCsp = cc;\n', ''),
 ('fx_child_data_not_tinted', FX, '    cd->mPrmClr = tint(cd->mPrmClr), cd->mEnvClr = tint(cd->mEnvClr);\n', ''),
 ('fx_child_tev_lost', FX, '        pfx_native_child_tev(e);\n', ''),
 ('fx_child_tev_on_parent', FX, 'if (s->child_static && e->checkStatus(0x80)) {', 'if (s->child_static) {'),
 ('fx_flash_stays_blue', FX, '  g_env_light.eflight.mLightType = 1;\n', ''),
 ('fx_flash_on_block', FX, '  if (pf.ctx_mark >= 0 && pf.book.orange_now(pf.ctx_mark) && pfx_native_flash()) ++pf.book.c.flashes;', '  (void)pfx_native_flash();'),
 ('fx_retro_lost', FX, '        if (pfx_attach(pf.pending[i].e, pf.ctx_pass, pf.pending[i].res, kPfxOriginRetro)) ++pf.book.c.retro;\n', ''),
 ('fx_retro_any_actor', FX, '        if (pf.pending[i].actor != id || !pf.pending[i].e) continue;', '        if (!pf.pending[i].e) continue;'),
 ('fx_sibling_lost', FX, '  const int ex = pf.book.exchange(id, g.p8.tick);', '  const int ex = -1;'),
 ('fx_sibling_damage_spark_recoloured', FX, 'if (pf.ctx_pass >= 0 && (pf.ctx_origin == kPfxOriginPass || pfx::hitng_mark(type))) {', 'if (pf.ctx_pass >= 0) {'),
 ('fx_tex_index_through_animation_table', FX, 'r->getTexIdx(b->getTexIdx());', 'r->getTexIdx(b->getTexIdx(0));'),
 ('fx_clash_too_long', 'src/parry_fx_core.hpp', 'kClashTicks = 4', 'kClashTicks = 40'),
 ('fx_pitch_too_low', 'src/parry_fx_core.hpp', 'kPitch = 1.22f', 'kPitch = 1.12f'),
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
total_cases = len(cases)
if os.environ.get('LIZ_MUTATION_SHARD'):  # FIX05 CI: this job runs one slice; the finalize job adds the slices up and compares with the total
 _i, _n = map(int, os.environ['LIZ_MUTATION_SHARD'].split('/'))
 cases = cases[_i::_n]
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('LIZ_MUTATION_WORKERS', '4'))) as pool:
 result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'LIZ_WORLD_LAB_PASS', 'total': total_cases, 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
