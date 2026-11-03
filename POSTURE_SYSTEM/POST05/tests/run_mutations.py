"""Rework + POST07 mutations. Historical 240 are retained at immutable a0a815356394. Each logic mutation names the laboratory that must catch it."""
from pathlib import Path
import subprocess,tempfile,shutil,concurrent.futures,json,sys,os
root=Path(__file__).resolve().parents[1]
logic=[
 ('shield_collider_missing','src/helm_core.hpp','if(selected==guard)return LinkContact::Shield;','if(false)return LinkContact::Shield;'),
 ('shield_is_sword','src/helm_core.hpp','if(selected==guard)return LinkContact::Shield;','if(selected==guard)return LinkContact::Sword;'),
 ('nondecisive_fall','src/break_core.hpp','if(v.hit&&v.native_hp>1&&(v.mode==5||v.mode==6))','if(false)'),
 ('helm_episode_lost','src/helm_core.hpp','if(link_id!=player||!helm_proc)seen=false;','seen=false;'),
 ('helm_stale_contact','src/helm_core.hpp','live&&token&&processed&&fresh&&sword','live&&token&&processed&&sword'),
 ('helm_shield_contact','src/helm_core.hpp','live&&token&&processed&&fresh&&sword','live&&token&&processed&&fresh'),
 ('timed_fall','src/break_core.hpp','a.act=Act::Recover;a.why=v.stun_ended','a.act=Act::Bonus;a.why=v.stun_ended'),
 ('old_120','src/break_core.hpp','a.extra=(base+1)/2','a.extra=base/5'),
 ('shield_bonus','src/break_core.hpp','a.act=Act::Stun;a.why=Why::ShieldPrepared','a.act=Act::Bonus;a.why=Why::ShieldPrepared'),
 ('break_bonus','src/break_core.hpp','a.act=Act::Stun;a.why=Why::BreakStanding','a.act=Act::Bonus;a.why=Why::BreakStanding'),
 ('duplicate_bonus','src/break_core.hpp','||a.token<=s.last_commit',''),
 ('foreign_sword','src/break_core.hpp','v.sword&&v.from_link','v.sword'),
 ('helm_without_shield','src/break_core.hpp','(!v.helm||s.shield_prepared)','true'),
 ('renew_shield_window','src/break_core.hpp','s.shield_prepared=true;++c.shields','s.punish_start=s.exec;s.shield_prepared=true;++c.shields'),
 ('long_timeout','src/break_core.hpp','kPunishMaxExec=90','kPunishMaxExec=180'),
 ('wrong_abort_posture','src/break_core.hpp','a.rearm_to=postbal01::kStandingRearm','a.rearm_to=0'),
 ('wrong_timeout_posture','src/break_core.hpp','Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=postbal01::kStandingRearm','Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=0'),
 ('early_rearm','src/break_core.hpp','s.phase==Phase::GettingUp&&v.mode==3&&v.state==5','s.phase==Phase::Down&&v.mode==3&&v.state==5'),
 ('no_normalize_helm','src/helm_core.hpp','r.hp=pre>r.base?pre-r.base:0','r.hp=native'),
 ('heal_native_loss','src/helm_core.hpp','r.hp=pre>r.base?pre-r.base:0','r.hp=pre>floor?pre-floor:0'),
 ('double_rounding','src/break_core.hpp','a.extra=(base+1)/2','a.extra=(base+2)/2'),
 # POST07 FIX_REARM50
 ('redirect_rearm_50','src/break_core.hpp','{a.act=Act::Rearm;a.rearm_to=0;a.why=Why::RearmAfterGetUpRedirected;}','{a.act=Act::Recover;a.rearm_to=postbal01::kStandingRearm;a.why=Why::TechnicalAbort;}'),
 ('redirect_hit_rearms','src/break_core.hpp','s.phase==Phase::GettingUp&&!v.hit&&v.native_hp>0','s.phase==Phase::GettingUp&&v.native_hp>0'),
 ('redirect_from_down','src/break_core.hpp','s.phase==Phase::GettingUp&&!v.hit&&v.native_hp>0','(s.phase==Phase::GettingUp||s.phase==Phase::Down)&&!v.hit&&v.native_hp>0'),
 ('redirect_uncounted','src/break_core.hpp','if(a.why==Why::RearmAfterGetUpRedirected)++c.rearms_redirected;',''),
]
parry=[
 # POST08 false positives / dedupe / window / bookkeeping / hand-off
 ('parry_no_shield_flag','src/parry_core.hpp','if (in.at_shield[i] && blocked < 0) blocked = i;','if (blocked < 0) blocked = i;'),
 ('parry_proximity_not_link','src/parry_core.hpp','if (in.at_hit[i] && in.at_on_link[i]) {','if (in.at_hit[i]) {'),
 ('parry_any_guard','src/parry_core.hpp','} else if (!in.pass.guard_attack) {','} else if (false) {'),
 ('parry_wide_window','src/parry_core.hpp','inline constexpr float kWindowFrames = 9.0f;','inline constexpr float kWindowFrames = 14.0f;'),
 ('parry_narrow_window_v080','src/parry_core.hpp','inline constexpr float kWindowFrames = 9.0f;','inline constexpr float kWindowFrames = 5.0f;'),
 # FIX01: reintroducir la causa raiz (estado de Link AHORA) o romper el sello de la instantanea debe fallar
 ('fix01_uses_now_state','src/parry_core.hpp','} else if (!in.pass.guard_attack) {','} else if (!in.now.guard_attack) {'),
 ('fix01_window_uses_now','src/parry_core.hpp','    else if (!in_window(in.pass)) {','    else if (!in_window(in.now)) {'),
 ('fix01_no_snapshot_check','src/parry_core.hpp','    else if (!in.pass.valid) v.result = Result::NoPassSnapshot;\n',''),
 ('fix01_snapshot_any_tick','src/parry_core.hpp','if (!link_ok || tick != tick_ || id != id_ || !snap_.valid) return LinkView{};','if (!link_ok || id != id_ || !snap_.valid) return LinkView{};'),
 ('fix01_snapshot_any_link','src/parry_core.hpp','if (!link_ok || tick != tick_ || id != id_ || !snap_.valid) return LinkView{};','if (!link_ok || tick != tick_ || !snap_.valid) return LinkView{};'),
 ('fix01_no_tick_cap','src/parry_core.hpp','d <= policy::kWindowFrames && p.age_ticks <= policy::kWindowTicksCap;','d <= policy::kWindowFrames;'),
 ('fix01_serial_never_closes','src/parry_core.hpp','    } else if (!guard_attack && open_) {\n      t = close(proc);\n    }','    }'),
 ('fix01_link_change_inherits','src/parry_core.hpp','      if (open_) t = close(-1);\n      snap_ = LinkView{};','      snap_ = LinkView{};'),
 ('fix01_hist_wrong_bin','src/parry_core.hpp','    ++bin[d - kMin];','    ++bin[0];'),
 # FIX02: revertir cada parte del arreglo debe hacer fallar el laboratorio
 ('fix02_break_no_wait','src/break_core.hpp','   if(v.parry_break_pending&&v.latched&&!v.parry_crossed)return a;',''),
 ('fix02_no_fresh_gate','src/parry_core.hpp','    if (!in.fresh) {  // FIX02','    if (false) {  // FIX02'),
 ('fix02_always_force_recoil','src/parry_core.hpp','  if (!protected_now) return DeflectPlan::ForceRecoil;','  return DeflectPlan::ForceRecoil;'),
 ('fix02_arm_from_frame0','src/parry_core.hpp','static constexpr float kArmFirst = 14.0f, kArmLast = 22.0f;','static constexpr float kArmFirst = 0.0f, kArmLast = 22.0f;'),
 ('fix02_fresh_any_later_tick','src/parry_core.hpp','s.tick + 1 == tick;','s.tick < tick;'),
 ('fix02_fresh_ignores_armed','src/parry_core.hpp','return s.valid && s.armed && s.id == id','return s.valid && s.id == id'),
 ('fix02_fresh_ignores_identity','src/parry_core.hpp','return s.valid && s.armed && s.id == id && s.generation == generation && s.tick + 1 == tick;','return s.valid && s.armed && s.tick + 1 == tick;'),
 ('fix02_break_counts_protected','src/parry_core.hpp','(now.proc == kProcGuardAttack || now.proc == kProcGuardSlip)','(now.proc == kProcGuardAttack || now.proc == kProcGuardSlip || now.proc == 0x1f)'),
 ('fix02_restore_not_pre','src/parry_core.hpp','  return untouched ? DeflectPlan::NativeContinues : DeflectPlan::RestorePre;','  return DeflectPlan::NativeContinues;'),
 ('fix02_stale_counts_contact','src/parry_core.hpp','      ++c.stale;\n      v.result = Result::StaleContact;\n      return count(v);','      ++c.stale;'),
 ('fix02_watch_blind_aborts','src/parry_core.hpp','      if (!w->swing_armed) ++w->swings_aborted;','      (void)0;'),
 ('fix02_watch_ko_is_stall','src/parry_core.hpp','      if (s.same(w->last) && !ko) {','      if (s.same(w->last)) {'),
 ('fix02_watch_timer_ignored','src/parry_core.hpp','return mode == o.mode && state == o.state && t6c0 == o.t6c0 && t6c2 == o.t6c2 && frame == o.frame && speed == o.speed;','return mode == o.mode && state == o.state && frame == o.frame && speed == o.speed;'),
 ('fix02_watch_any_generation','src/parry_core.hpp','      if (w.on && w.slot == slot && w.id == id && w.generation == generation) return &w;','      if (w.on && w.slot == slot && w.id == id) return &w;'),
 ('parry_late_allowed','src/parry_core.hpp','else if (e.resolved) v.result = Result::MissedLate;','else if (false) v.result = Result::MissedLate;'),
 ('parry_double_sphere','src/parry_core.hpp','else if (e.parried) v.result = Result::Duplicate;','else if (false) v.result = Result::Duplicate;'),
 ('parry_no_restart_token','src/parry_core.hpp','        e.token = ++next_token_;\n','        e.token = 1;\n'),
 ('parry_latched_allowed','src/parry_core.hpp','    } else if (in.latched) v.result = Result::Latched;','    } else if (false) v.result = Result::Latched;'),
 ('parry_fury_allowed','src/parry_core.hpp','    else if (fury_chip_possible(in)) v.result = Result::FuryChipPossible;','    else if (false) v.result = Result::FuryChipPossible;'),
 ('parry_idle_allowed','src/parry_core.hpp','    else if (!swing || !e.active) v.result = Result::NotAttacking;','    else if (false) v.result = Result::NotAttacking;'),
 ('parry_generation_inherit','src/parry_core.hpp','if (!e.bound || e.id != in.id || e.generation != in.generation) {  // identidad','if (!e.bound) {  // identidad'),
 ('parry_commit_twice','src/parry_core.hpp','token == 0 || e.parried) return;','token == 0) return;'),
 ('parry_deflect_any_shape','src/parry_core.hpp','  if (!untouched && !shield_stun) return DeflectPlan::Refuse;',''),
 ('parry_deflect_state','src/parry_core.hpp','inline constexpr int kDeflectState = 3;','inline constexpr int kDeflectState = 1;'),
 ('posture_gain_80','src/posture_core.hpp','inline constexpr int kParryPostureGain = 40;','inline constexpr int kParryPostureGain = 80;'),
 ('posture_no_saturation','src/posture_core.hpp','o.after = e.posture + o.gain > policy::kMaxPosture ? policy::kMaxPosture : e.posture + o.gain;','o.after = e.posture + o.gain;'),
 ('posture_no_regen_reset','src/posture_core.hpp','    e.k = 0;\n    e.regen_acc = 0;\n    e.last_hit_tick = now_;\n    e.last_parry_token = token;','    e.last_hit_tick = now_;\n    e.last_parry_token = token;'),
 ('posture_no_token_dedupe','src/posture_core.hpp','      else if (e.last_parry_token == token) o.reject = ParryReject::DuplicateToken;\n',''),
 ('posture_parry_latched','src/posture_core.hpp','      else if (e.latched) o.reject = ParryReject::Latched;\n      else if (e.last_parry_token','      else if (e.last_parry_token'),
 ('posture_no_crossing_count','src/posture_core.hpp','      ++c.crossings;\n      ++c.parry_crossings;','      ++c.parry_crossings;'),
 ('posture_audit_weakened','src/posture_core.hpp','a.ok = a.e1 && a.e2 && a.e3 && a.e4 && a.e5 && a.e6 && a.invariants&&a.balance_units&&a.balance_events;','a.ok = a.e1 && a.e2 && a.e3 && a.e4 && a.e5 && a.invariants&&a.balance_units&&a.balance_events;'),
 ('break_parry_not_delivered','src/break_core.hpp','else if(v.parry_crossed&&v.latched&&','else if(false&&v.parry_crossed&&v.latched&&'),
 ('break_parry_bonus','src/break_core.hpp','{a.act=Act::Stun;a.why=Why::BreakByParry;}','{a.act=Act::Bonus;a.why=Why::BreakByParry;}'),
 ('break_parry_free_helm','src/break_core.hpp','if(a.why==Why::BreakByParry)++c.breaks_by_parry;s.phase=Phase::Punish;s.punish_start=s.exec;s.shield_prepared=false;','if(a.why==Why::BreakByParry)++c.breaks_by_parry;s.phase=Phase::Punish;s.punish_start=s.exec;s.shield_prepared=a.why==Why::BreakByParry;'),
 ('break_parry_over_fall','src/break_core.hpp','policy::drop_mode_safe(v.pre_mode)&&policy::drop_mode_safe(v.mode)){a.act=Act::Stun;a.why=Why::BreakByParry;}','policy::drop_mode_safe(v.pre_mode)){a.act=Act::Stun;a.why=Why::BreakByParry;}'),
 ('break_latched_any','src/break_core.hpp','else if(v.latched&&!(v.hit&&v.crossed)&&!v.parry_crossed){','else if(v.latched&&!(v.hit&&v.crossed)&&!v.parry_crossed&&false){'),
]
hud=[
 ('hud_fill_one_side','src/hud_core.hpp','q.add(cx - Eb, cy - hc, cx, cy + hc, eT, cT, cB, eB);','q.add(cx, cy - hc, cx + Eb, cy + hc, eT, cT, cB, eB);'),
 ('hud_fill_double','src/hud_core.hpp','inline constexpr float fill_extent(float f) noexcept { return clamp01(f) * layer_end(kCoreInset); }','inline constexpr float fill_extent(float f) noexcept { return clamp01(f) * layer_end(kCoreInset) * 2.0f; }'),
 ('hud_overlaps_life','src/hud_core.hpp','kFrameTop = 3.4f','kFrameTop = 2.2f'),
 ('hud_pulse_every_tick','src/hud_core.hpp','if (in.latched && !s.last_latched) {','if (in.latched) {'),
 ('hud_pulse_on_bind','src/hud_core.hpp','s.last_latched = in.latched;  // first observation','s.last_latched = false;  // first observation'),
 ('hud_no_identity_reset','src/hud_core.hpp','if (!s.bound || s.id != in.id || s.generation != in.generation) {','if (!s.bound) {'),
 ('hud_ghost_kept','src/hud_core.hpp','if (!in.present || in.id == 0 || dead) {','if (in.id == 0) {'),
 ('hud_dead_phase_shown','src/hud_core.hpp','const bool dead = in.dead || in.phase == Ph::Dead;','const bool dead = in.dead;'),
 ('hud_pause_animates','src/hud_core.hpp','    if (!advance) {\n      ++c.paused_ticks;\n      return;\n    }','    if (!advance) ++c.paused_ticks;'),
 ('hud_green_ramp','src/hud_core.hpp','kRed = rgb(0xD82E14)','kRed = rgb(0x30D014)'),
 ('hud_opaque_blend','src/hud_core.hpp','to_u8(254.0f * clamp01(a01))','to_u8(255.0f * clamp01(a01))'),
 ('hud_j2d_swap_missing','src/hud_core.hpp','return J2DArgs{q.tl, q.tr, q.bl, q.br};','return J2DArgs{q.tl, q.tr, q.br, q.bl};'),
 ('hud_frozen_shown','src/hud_core.hpp','const float want = (!in.frozen && content)','const float want = (content)'),
 ('hud_zero_shown','src/hud_core.hpp','const bool content = s.display > 0.5f || t > 0 || s.pulse || s.hit || in.latched;','const bool content = true;'),
 ('hud_premature_unbroken','src/hud_core.hpp','    s.broken = in.latched;','    s.broken = in.latched && in.phase == Ph::Punish;'),
 ('hud_budget_ignored','src/hud_core.hpp','select_nearest(ok, dist, kSlots, kMaxDrawn, sel);','select_nearest(ok, dist, kSlots, kSlots > kMaxDrawn ? kMaxDrawn + 1 : kMaxDrawn, sel);'),
 ('hud_farthest_first','src/hud_core.hpp','if (best < 0 || d < (isfin(dist[best]) ? dist[best] : 3.0e38f)) best','if (best < 0 || d > (isfin(dist[best]) ? dist[best] : 3.0e38f)) best'),
 ('hud_scrub_keeps_packet','src/hud_core.hpp','    if (*p != packet) *dst++ = *p;\n    else ++removed;','    *dst++ = *p;\n    if (*p == packet) ++removed;'),
 ('hud_queue_without_hud','src/hud_core.hpp','  if (!found) return Queue::HudNotQueued;\n','  (void)found;\n'),
 ('hud_anchor_drops','src/hud_core.hpp','  return previous;\n}','  return h;\n}'),
 ('hud_head_offset','src/hud_core.hpp','return P3{pos.x, pos.y + head + 8.0f, pos.z};','return P3{pos.x, pos.y + head, pos.z};'),
 ('hud_fade_band','src/hud_core.hpp','return distance <= 3700.0f ? 1.0f : (4500.0f - distance) / 800.0f;','return 1.0f;'),
 ('hud_asym_tip','src/hud_core.hpp','    q.vgrad(cx - ub, cy - h, cx - ua, cy + h, top, bottom);\n  }\n}','  }\n}'),
]

# LNK01: each must make lnk_lab fail (prompt suppression, Link posture source/dedup/regeneration, HUD geometry)
lnk=[
 ('lnk_hide_first_object_only','src/lnk_core.hpp','    if (in.visible) {\n      d.hide = true;','    if (in.visible && c.hide_writes == 0) {\n      d.hide = true;'),
 ('lnk_mask_never_restored','src/lnk_core.hpp','    e.restore = true, e.s0 = s0_, e.s1 = s1_;','    e.restore = false, e.s0 = s0_, e.s1 = s1_;'),
 ('lnk_mask_foreign_prompt','src/lnk_core.hpp','    if (!live_prompt) {\n      ++c.mask_foreign;','    if (false) {\n      ++c.mask_foreign;'),
 ('lnk_no_vptr_check','src/lnk_core.hpp','    if (!in.screen || !in.aligned || !in.vptr_ok) {','    if (!in.screen || !in.aligned) {'),
 ('lnk_stale_counts','src/lnk_core.hpp','      case R::StaleContact: return Verdict::RejectedStale;','      case R::StaleContact: break;'),
 ('lnk_guard_after_parry_counts','src/lnk_core.hpp','    if (k.episode_parried || e->parried == k.token) return Verdict::RejectedParry;','    if (false) return Verdict::RejectedParry;'),
 ('lnk_parry_gives_gain','src/lnk_core.hpp','    if (o.v != Verdict::Confirmed) return o;','    if (o.v != Verdict::Confirmed && o.v != Verdict::RejectedParry) return o;'),
 ('lnk_no_dedup','src/lnk_core.hpp','    if (e->credited == k.token || e->direct == k.token) return Verdict::Duplicate;','    if (e->direct == k.token) return Verdict::Duplicate;'),
 ('lnk_confuse_attackers','src/lnk_core.hpp','    if (!e.bound || e.id != k.id || e.generation != k.generation) {  // identidad nueva','    if (!e.bound) {  // identidad nueva'),
 ('lnk_block_without_shield_flag','src/lnk_core.hpp','    if (direct ? k.blocked : !k.blocked) return Verdict::RejectedNonBlock;','    if (direct ? k.blocked : false) return Verdict::RejectedNonBlock;'),
 ('lnk_regen_assumes_60hz','src/lnk_core.hpp','inline constexpr int kTicksPerSecond = 30;','inline constexpr int kTicksPerSecond = 60;'),
 ('lnk_regen_no_delay','src/lnk_core.hpp','inline constexpr std::uint32_t kRegenDelayTicks = 30;','inline constexpr std::uint32_t kRegenDelayTicks = 0;'),
 ('lnk_regen_in_pause','src/lnk_core.hpp','    if (!active) {\n      ++c.paused_ticks;','    if (false) {\n      ++c.paused_ticks;'),
 ('lnk_regen_negative','src/lnk_core.hpp','    if (d > cur_) d = cur_;\n',''),
 ('lnk_no_reset_on_link_change','src/lnk_core.hpp','    if (!bound_ || id != link_id_) {','    if (!bound_) {'),
 ('lnk_pulse_every_tick','src/lnk_core.hpp','pulse_ = policy::kPulseTicks, pulse_armed_ = false, ++c.pulses;','pulse_ = policy::kPulseTicks, ++c.pulses;'),
 ('lnk_asym_fill','src/lnk_core.hpp','    q.add(cx - E, cy - F, cx, cy + F, eT, cT, cB, eB);','    q.add(cx - E * 0.5f, cy - F, cx, cy + F, eT, cT, cB, eB);'),
 ('lnk_not_centred','src/lnk_core.hpp','  L.cx = minx + width * policy::kCenterX;','  L.cx = width * policy::kCenterX;'),
 ('lnk_absolute_units','src/lnk_core.hpp','  L.u = height / policy::kRefHeightPx;','  L.u = 1.0f;'),
 ('lnk_packet_only_enemies','src/lnk_core.hpp','return allowed && (enemy_bars > 0 || player_on); }','return allowed && enemy_bars > 0; }'),
 ('lnk_no_hold_at_zero','src/lnk_core.hpp','inline constexpr std::uint32_t kHoldTicks = 21;','inline constexpr std::uint32_t kHoldTicks = 0;'),
 ('lnk_gain_40','src/lnk_core.hpp','inline constexpr Weight kWeights[5] = {{20, 30},','inline constexpr Weight kWeights[5] = {{40, 30},'),
]

# LNK SESSION01 (LNK02/03/04/07): each must make lnks1_lab fail
s1=[
 ('s1_no_tg_attribution','src/lnk_core.hpp','      if (!k.tg_match) return Verdict::RejectedDirectNotAttributed;\n',''),
 ('s1_no_edge_required','src/lnk_core.hpp','          if (!edge) {\n            finish(e, DirectResult::NoNativeDamage, d, out);','          if (false) {\n            finish(e, DirectResult::NoNativeDamage, d, out);'),
 ('s1_no_life_required','src/lnk_core.hpp','        if (ob.life < e.pend_life_ref || ob.pending_life < 0.0f) {','        if (true) {'),
 ('s1_ambiguous_credited','src/lnk_core.hpp','          if (same_tick != 1) {','          if (same_tick == 0) {'),
 ('s1_edge_any_timer','src/lnk_core.hpp','ob.damage_timer > 0 && ob.damage_timer > prev_timer_;','ob.damage_timer > 0;'),
 ('s1_direct_weight_as_block','src/lnk_core.hpp','          d.weight = w.direct;','          d.weight = w.block;'),
 ('s1_band_inverted','src/lnk_core.hpp','inline constexpr int kBandPointsPerSecond[4] = {10, 8, 5, 2};','inline constexpr int kBandPointsPerSecond[4] = {2, 5, 8, 10};'),
 ('s1_band_flat','src/lnk_core.hpp','inline constexpr int kBandPointsPerSecond[4] = {10, 8, 5, 2};','inline constexpr int kBandPointsPerSecond[4] = {10, 10, 10, 10};'),
 ('s1_guard_no_bonus','src/lnk_core.hpp','inline constexpr int kGuardMulNum = 3, kGuardMulDen = 2;','inline constexpr int kGuardMulNum = 2, kGuardMulDen = 2;'),
 ('s1_guard_shortens_delay','src/lnk_core.hpp','    if (cur_ <= 0 || since_ <= policy::kRegenDelayTicks) return ev;','    if (cur_ <= 0 || (!guard_safe && since_ <= policy::kRegenDelayTicks)) return ev;'),
 ('s1_fury_as_black','src/lnk_core.hpp','    r.v = policy::Variant::BlackFury, r.m = IdMethod::E_OC2_Atp6BlackFury;','    r.v = policy::Variant::Black, r.m = IdMethod::E_OC2_Atp6BlackFury;'),
 ('s1_unknown_as_black','src/lnk_core.hpp','{30, 40}, {35, 45}, {20, 30}};','{30, 40}, {35, 45}, {30, 40}};'),
 ('s1_history_inherited','src/lnk_core.hpp','    if (!s.bound || s.id != id || s.generation != generation || s.e_oc2 != e_oc2) s = S{','    if (!s.bound) s = S{'),
 ('s1_legacy_blue_as_red','src/lnk_core.hpp','  } else if (seen & 1u) {','  } else if (false) {'),
 ('s1_no_group_adjust','src/lnk_core.hpp','inline constexpr Weight kGroupPowerAdjust = {5, 5};','inline constexpr Weight kGroupPowerAdjust = {0, 0};'),
 ('s1_invalid_life_fast','src/lnk_core.hpp','  if (permille < 0) return 3;','  if (permille < 0) return 0;'),
 ('s1_life_ref_after_damage','src/lnk_core.hpp','e.pend_life_ref = prev_life_;','e.pend_life_ref = ob.life;'),
 ('s1_parry_then_direct','src/lnk_core.hpp','    if (k.episode_parried || e->parried == k.token) return Verdict::RejectedParry;','    if (!direct && (k.episode_parried || e->parried == k.token)) return Verdict::RejectedParry;'),
]

fix01=[
 ('f01_native_completion_ignored','src/fix01_core.hpp','if(s.native_completed&&s.armed&&!s.interrupted','if(s.armed&&!s.interrupted'),
 ('f01_original_late_tg','src/fix01_core.hpp','r.n.collider!=0&&(r.n.collider==o.sphere[0]||r.n.collider==o.sphere[1])','false'),
 ('f01_wrong_tick','src/fix01_core.hpp','if(z.k.tick+1!=tick)','if(false)'),
 ('f01_foreign_source','src/fix01_core.hpp','r.n.collider!=0&&(r.n.collider==o.sphere[0]||r.n.collider==o.sphere[1])','r.n.collider!=0'),
 ('f01_native_timer','src/fix01_core.hpp','timer_before==0&&timer_after>0','timer_after>0'),
 ('f01_environment','src/fix01_core.hpp','poly_result==0','true'),
 ('f01_unseen_poly','src/fix01_core.hpp','!blocked&&poly_seen&&','!blocked&&'),
 ('f01_electric','src/fix01_core.hpp','poly_result==0&&!electric','poly_result==0'),
 ('f01_magic_armor','src/fix01_core.hpp','(life_after<life_before||(pending_after<0&&pending_after<pending_before))','true'),
 ('f01_block_chip','src/fix01_core.hpp','!event&&!blocked&&','!event&&'),
 ('f01_identity','src/fix01_core.hpp','!identity_ok||!active||z.k.link_id!=link_id','!active||z.k.link_id!=link_id'),
 ('f01_stale_contact','src/fix01_core.hpp','if(!fresh||k.result','if(k.result'),
 ('f01_parry_contact','src/fix01_core.hpp','k.result!=post08::Result::NotBlocked||',''),
 ('f01_ambiguous','src/fix01_core.hpp','&&!r.ambiguous&&!r.used','&&!r.used'),
 ('f01_loop_clip_restored','src/fix01_core.hpp','!a.anim_ok||a.anim_attr!=0||','!a.anim_ok||'),
 ('f01_animation_changed','src/fix01_core.hpp','if(!post05::same_anim(a,b))','if(false)'),
 ('f01_reverse_attack','src/fix01_core.hpp','if(a.anim_rate<=0)','if(false)'),
 ('f01_recoil_positive','src/fix01_core.hpp','||a.anim_rate>=0',''),
 ('f01_physical_chain','src/fix01_core.hpp','if(mode==6)return Continuation::PhysicalChain;',''),
 ('f01_guard_ignored','src/reaction_core.hpp','    if (!in.continuation_safe) return Reason::UnsafeContinuation;',''),
 ('f01_historical_arm_blind','src/fix01_core.hpp','if(!s.suspected&&eligible&&(s.abort_run','if(!s.suspected&&s.arms==0&&eligible&&(s.abort_run'),
 ('f01_dead_is_recovery','src/fix01_core.hpp','return result(AiVerdict::DeadBeforeRecovery);','return result(AiVerdict::NormalRecovery);'),
 ('f01_no_actual_arm','src/fix01_core.hpp','if(s.native_completed&&s.armed&&!s.interrupted','if(s.native_completed&&!s.interrupted'),
 ('f01_no_native_arm_observation','src/fix01_core.hpp','if(actual_armed&&!s.armed)','if(false)'),
 ('f01_ignores_restarts','src/fix01_core.hpp','++s.restarts;++c.restarts;++s.restart_run;','++s.restarts;++c.restarts;'),
 ('f01_aborts_not_diagnosed','src/fix01_core.hpp','s.abort_run>=4||',''),
 ('f01_expired_is_pass','src/fix01_core.hpp','return result(AiVerdict::ExpiredInconclusive);','return result(AiVerdict::NormalRecovery);'),
 ('f01_cleanup_ignores_fresh','src/fix01_core.hpp','||!fresh||!attack||','||!attack||'),
 ('f01_cleanup_ignores_attack','src/fix01_core.hpp','||!fresh||!attack||','||!fresh||'),
 ('f01_cleanup_changes_native_on','src/fix01_core.hpp','r.added=r.mask&~native_bits&3','r.added=r.mask&3'),
 ('f01_cleanup_wrong_identity','src/fix01_core.hpp','r.id!=id||r.gen!=gen||r.tick!=tick','r.gen!=gen||r.tick!=tick'),
 ('f01_cleanup_wrong_generation','src/fix01_core.hpp','r.id!=id||r.gen!=gen||r.tick!=tick','r.id!=id||r.tick!=tick'),
 ('f01_cleanup_wrong_tick','src/fix01_core.hpp','r.id!=id||r.gen!=gen||r.tick!=tick','r.id!=id||r.gen!=gen'),
 ('f01_cleanup_false_verify','src/fix01_core.hpp','else if(hit_bits&r.added)','else if(false)'),
 ('f01_dead_legacy_false_pass','src/parry_core.hpp','if(why&&std::strcmp(why,"ACTOR_DEAD")==0)','if(false)'),
]
s2=[
 ('s2_queue_requests','src/lnks2_core.hpp','    if (s_ != State::Ready || e.link_id == 0) {','    if (e.link_id == 0) {'),
 ('s2_block_long_deadline','src/lnks2_core.hpp','kBlockTriggerTicks = 3;','kBlockTriggerTicks = 24;'),
 ('s2_direct_short_deadline','src/lnks2_core.hpp','kDirectTriggerTicks = 24;','kDirectTriggerTicks = 3;'),
 ('s2_gate_no_expiry','src/lnks2_core.hpp','    if (g.tick > ep_.deadline) return Gate::Expired;\n',''),
 ('s2_gate_any_link','src/lnks2_core.hpp','    if (!g.valid || g.link_id != ep_.link_id || g.epoch != ep_.epoch) return Gate::LinkMismatch;\n',''),
 ('s2_gate_wolf','src/lnks2_core.hpp','    if (!g.human) return Gate::NotHuman;\n',''),
 ('s2_gate_dead','src/lnks2_core.hpp','    if (!g.alive) return Gate::Dead;\n',''),
 ('s2_gate_pause','src/lnks2_core.hpp','    if (!g.sim_active) return Gate::SimInactive;\n',''),
 ('s2_gate_event','src/lnks2_core.hpp','    if (g.event) return Gate::EventRunning;\n',''),
 ('s2_rebreak','src/lnks2_core.hpp','    if (g.proc == policy::kProcGuardBreak) return Gate::AlreadyBreaking;','    if (false) return Gate::AlreadyBreaking;'),
 ('s2_any_origin','src/lnks2_core.hpp','    if (!allowed_origin(g.proc)) return Gate::ProcNotAllowed;\n',''),
 ('s2_shield_attack_origin','src/lnks2_core.hpp','0x1c /*CROUCH*/,    0x1d /*GUARD_SLIP*/};','0x1c /*CROUCH*/,    0x1d /*GUARD_SLIP*/, 0x1e};'),
 ('s2_damage_origin','src/lnks2_core.hpp','0x1c /*CROUCH*/,    0x1d /*GUARD_SLIP*/};','0x1c /*CROUCH*/,    0x1d /*GUARD_SLIP*/, 0x32};'),
 ('s2_no_slip_origin','src/lnks2_core.hpp','0x1c /*CROUCH*/,    0x1d /*GUARD_SLIP*/};','0x1c /*CROUCH*/};'),
 ('s2_mode_ignored','src/lnks2_core.hpp','    if ((g.mode_flags & policy::kModeBlockedMask) != 0 || (g.mode_flags & policy::kModeSpecialMask) != 0) return Gate::ModeFlags;\n',''),
 ('s2_special_mode_ignored','src/lnks2_core.hpp','|| (g.mode_flags & policy::kModeSpecialMask) != 0) return Gate::ModeFlags;',') return Gate::ModeFlags;'),
 ('s2_damage_veto_ignored','src/lnks2_core.hpp','    if (g.damage_this_call) return Gate::DamageThisCall;',''),
 ('s2_entry_unverified','src/lnks2_core.hpp','    if (!entered) {','    if (false) {'),
 ('s2_no_parry_cancel','src/lnks2_core.hpp','    (void)tick;\n    if (s_ == State::TriggerPending) {','    (void)tick;\n    if (false) {'),
 ('s2_no_expiry_observe','src/lnks2_core.hpp','    if (s_ == State::TriggerPending && in.tick > ep_.deadline) {','    if (false) {'),
 ('s2_no_adoption','src/lnks2_core.hpp','      if (s_ == State::TriggerPending || posture_saturated) {','      if (false) {'),
 ('s2_adopt_unsaturated','src/lnks2_core.hpp','      if (s_ == State::TriggerPending || posture_saturated) {','      if (true) {'),
 ('s2_same_link_ignored','src/lnks2_core.hpp','    const bool same_link = in.valid && (s_ == State::Ready || (in.link_id == ep_.link_id && in.epoch == ep_.epoch));','    const bool same_link = in.valid;'),
 ('s2_pause_counts','src/lnks2_core.hpp','        if (in.sim_active) ++active_ticks_, ++c.active_ticks;','        if (true) ++active_ticks_, ++c.active_ticks;'),
 ('s2_stuck_forces_exit','src/lnks2_core.hpp','++c.stuck, o.ev = Ev::Stuck;','++c.stuck, o.ev = Ev::Stuck, s_ = State::Ready;'),
 ('s2_dead_recovers','src/lnks2_core.hpp','        if (!in.alive) {','        if (false) {'),
 ('s2_end_as_cancel','src/lnks2_core.hpp','        } else if (last_frame_ >= policy::kEndFrame - policy::kFrameSlack) {','        } else if (false) {'),
 ('s2_early_exit_is_recovery','src/lnks2_core.hpp','        } else if (last_frame_ >= policy::kCancelFrame - policy::kFrameSlack) {','        } else if (true) {'),
 ('s2_cancel_frame','src/lnks2_core.hpp','kCancelFrame = 28.0f','kCancelFrame = 20.0f'),
 ('s2_reset_on_abort','src/lnks2_core.hpp','          ++c.aborted;\n          o.ev = Ev::Aborted;\n','          ++c.aborted;\n          o.ev = Ev::Aborted;\n          o.reset_posture = true;\n'),
 ('s2_hold_always','src/lnks2_core.hpp','  bool hold() const noexcept { return s_ != State::Ready; }','  bool hold() const noexcept { return true; }'),
 ('s2_hold_never','src/lnks2_core.hpp','  bool hold() const noexcept { return s_ != State::Ready; }','  bool hold() const noexcept { return false; }'),
 ('s2_hold_ignored','src/lnk_core.hpp','    if (hold_) {  //','    if (false) {  //'),
 ('s2_reset_not_accounted','src/lnk_core.hpp','    c.break_reset_milli += static_cast<std::uint64_t>(cur_);\n',''),
 ('s2_reset_keeps_value','src/lnk_core.hpp','    cur_ = 0, acc_ = 0, since_ = 0xFFFFFFFFu, regenerating_ = false;','    acc_ = 0, since_ = 0xFFFFFFFFu, regenerating_ = false;'),
]
s2f=[
 ('fx_auto_shield_L','src/lnks2fix_core.hpp','  if (!r_held) return Guard::DeniedNoR;\n',''),
 ('fx_mandatory_cleared','src/lnks2fix_core.hpp','  if (mandatory) return Guard::Mandatory;\n',''),
 ('fx_guard_in_penalty','src/lnks2fix_core.hpp','  if (p == Phase::NativeBreak || p == Phase::PostVulnerable) return Guard::DeniedPenalty;\n',''),
 ('fx_guard_before_rearm','src/lnks2fix_core.hpp','  if (p == Phase::RearmRequired) return Guard::DeniedRearm;\n',''),
 ('fx_rearm_while_held','src/lnks2fix_core.hpp','    if (need_release_) return o;  // R mantenido desde antes del desbloqueo\n',''),
 ('fx_rearm_during_penalty','src/lnks2fix_core.hpp','    if (phase_ != Phase::RearmRequired || link_id != link_id_ || epoch != epoch_) return o;','    if ((phase_ != Phase::RearmRequired && phase_ != Phase::PostVulnerable) || link_id != link_id_ || epoch != epoch_) return o;'),
 ('fx_no_release_needed','src/lnks2fix_core.hpp','    phase_ = Phase::RearmRequired;\n    need_release_ = true;','    phase_ = Phase::RearmRequired;\n    need_release_ = false;'),
 ('fx_bash_in_penalty','src/lnks2fix_core.hpp','  if (p != Phase::Ready) return Bash::DeniedPenalty;\n',''),
 ('fx_bash_without_r','src/lnks2fix_core.hpp','  if (!r_held) return Bash::DeniedNoR;\n',''),
 ('fx_dodge_free','src/lnks2fix_core.hpp','  bool dodge_blocked() const noexcept { return extended_ && (phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable); }','  bool dodge_blocked() const noexcept { return false; }'),
 ('fx_dodge_native_only','src/lnks2fix_core.hpp','  bool dodge_blocked() const noexcept { return extended_ && (phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable); }','  bool dodge_blocked() const noexcept { return extended_ && phase_ == Phase::NativeBreak; }'),
 ('fx_backflip_free','src/lnks2fix_core.hpp','  if (backward(move_minus_shape)) return Dodge::Backflip;','  if (backward(move_minus_shape)) return Dodge::None;'),
 ('fx_frontroll_free','src/lnks2fix_core.hpp','  if (do_status == policy::kStatusFrontRoll) return Dodge::FrontRoll;','  if (do_status == policy::kStatusFrontRoll) return Dodge::None;'),
 ('fx_sidehop_free','src/lnks2fix_core.hpp','  return side_roll_armed ? Dodge::SideRollCandidate : Dodge::Sidehop;','  return Dodge::None;'),
 ('fx_window_native_only','src/lnks2fix_core.hpp','      if (extended_ && active_ < policy::kTotalPenaltyTicks) {','      if (false) {'),
 ('fx_window_short','src/lnks2fix_core.hpp','kTotalPenaltyTicks = 75;','kTotalPenaltyTicks = 45;'),
 ('fx_window_off_by_one','src/lnks2fix_core.hpp','    } else if (phase_ == Phase::PostVulnerable && active_ >= policy::kTotalPenaltyTicks) {','    } else if (phase_ == Phase::PostVulnerable && active_ + 1 >= policy::kTotalPenaltyTicks) {'),
 ('fx_pause_counts','src/lnks2fix_core.hpp','    if (!in.sim_active) {  // pausa','    if (false) {  // pausa'),
 ('fx_latch_extended','src/lnks2fix_core.hpp','    origin_ = b.origin, extended_ = b.extended && policy::kExtendedPenalty;','    origin_ = b.origin, extended_ = true;'),
 ('fx_clock_restart','src/lnks2fix_core.hpp','    if (phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable) {  // nunca una cola','    if (false) {  // nunca una cola'),
 ('fx_transplant','src/lnks2fix_core.hpp','    const bool same = in.valid && in.link_id == link_id_ && in.epoch == epoch_;','    const bool same = in.valid;'),
 ('fx_event_no_abort','src/lnks2fix_core.hpp','    if (!same || !in.human || !in.alive || in.event) {','    if (!same || !in.human || !in.alive) {'),
 ('fx_abort_leaves_latch','src/lnks2fix_core.hpp','      o.reason = !same ? "LINK_CHANGED_OR_ABSENT" : (!in.human ? "NOT_HUMAN" : (!in.alive ? "DEAD" : "EVENT"));\n      reset_phase();','      o.reason = !same ? "LINK_CHANGED_OR_ABSENT" : (!in.human ? "NOT_HUMAN" : (!in.alive ? "DEAD" : "EVENT"));\n      phase_ = Phase::RearmRequired, need_release_ = true;'),
 ('fx_watchdog_forces_exit','src/lnks2fix_core.hpp','++c.watchdog, o.ev = Ev::Watchdog,','++c.watchdog, phase_ = Phase::RearmRequired, o.ev = Ev::Watchdog,'),
 ('fx_requests_open','src/lnks2fix_core.hpp','  bool request_blocked() const noexcept { return phase_ == Phase::NativeBreak || phase_ == Phase::PostVulnerable; }','  bool request_blocked() const noexcept { return false; }'),
 ('fx_consume_b_global','src/lnks2fix_core.hpp','  return b_trig && r_held && p == Phase::Ready && bash_offered_prev;','  return b_trig && r_held;'),
 ('fx_consume_b_unoffered','src/lnks2fix_core.hpp','  return b_trig && r_held && p == Phase::Ready && bash_offered_prev;','  return b_trig && r_held && p == Phase::Ready;'),
 ('fx_bonus_r_only','src/lnks2fix_core.hpp','  return native_guard_safe && (!fx_enabled || (manual_accepted && p == Phase::Ready));','  return native_guard_safe;'),
 ('fx_bonus_in_rearm','src/lnks2fix_core.hpp','  return native_guard_safe && (!fx_enabled || (manual_accepted && p == Phase::Ready));','  return native_guard_safe && (!fx_enabled || manual_accepted);'),
 ('fx_backward_threshold','src/lnks2fix_core.hpp','static_cast<int>(rel)) > 0x6000;','static_cast<int>(rel)) > 0x2000;'),
 ('lnk05_cancel_speed','src/lnks2_core.hpp','kCancelFrame = 28.0f','kCancelFrame = 14.0f'),
]
balance=[
 ('bal_red_sword','src/balance_core.hpp','{17,27,33}','{25,27,33}'),
 ('bal_red_parry','src/balance_core.hpp','{17,27,33}','{17,40,33}'),
 ('bal_red_iron34','src/balance_core.hpp','{17,27,33}','{17,27,34}'),
 ('bal_black_sword','src/balance_core.hpp','{13,20,25}','{25,20,25}'),
 ('bal_black_parry','src/balance_core.hpp','{13,20,25}','{13,40,25}'),
 ('bal_black_iron','src/balance_core.hpp','{13,20,25}','{13,20,50}'),
 ('bal_fury_different','src/balance_core.hpp','{13,20,25},{13,20,25}','{13,20,25},{17,27,33}'),
 ('bal_standing50','src/balance_core.hpp','kStandingRearm=25','kStandingRearm=50'),
 ('bal_snapshot_id','src/balance_core.hpp','s.slot!=slot||s.id!=id||s.generation!=gen','s.slot!=slot||s.generation!=gen'),
 ('bal_snapshot_gen','src/balance_core.hpp','s.slot!=slot||s.id!=id||s.generation!=gen','s.slot!=slot||s.id!=id'),
 ('bal_snapshot_epoch','src/balance_core.hpp','if(s.epoch!=epoch)','if(false)'),
 ('bal_no_fallback','src/balance_core.hpp','if(!s.verified)return reference;','if(!s.verified)return 13;'),
 ('bal_capture_ignores_valid','src/balance_adapter.hpp','if(!a.valid||','if('),
 ('bal_classifier_forces_blue','src/balance_adapter.hpp','s.variant=static_cast<Variant>(v.v);','s.variant=Variant::Blue;'),
 ('bal_consume_legacy','src/posture_core.hpp','postbal01::select(profile,r.slot,r.id,r.generation,','postbal01::select({},r.slot,r.id,r.generation,'),
 ('bal_preview_legacy','src/posture_core.hpp','o.gain=postbal01::gain(o.balance_profile,postbal01::Kind::Parry,policy::kParryPostureGain);','o.gain=policy::kParryPostureGain;'),
 ('bal_commit_legacy','src/posture_core.hpp','preview_parry(slot, generation, id, token, L, profile);','preview_parry(slot, generation, id, token, L);'),
 ('bal_commit_ignores_receipt','src/posture_core.hpp','if(o.reject==ParryReject::None&&expected&&!same_preview(*expected,o))','if(false)'),
 ('bal_preview_id_unbound','src/posture_core.hpp','&&a.id==b.id',''),
 ('bal_preview_epoch_unbound','src/posture_core.hpp','&&a.epoch==b.epoch',''),
 ('bal_preview_token_unbound','src/posture_core.hpp','&&a.token==b.token',''),
 ('bal_preview_gain_unbound','src/posture_core.hpp','&&a.gain==b.gain',''),
 ('bal_preview_before_unbound','src/posture_core.hpp','&&a.before==b.before',''),
 ('bal_preview_source_unbound','src/balance_core.hpp','&&a.method==b.method',''),
 ('bal_rearm_accepts50','src/posture_core.hpp','target != 0 && target != postbal01::kStandingRearm','target != 0 && target != postbal01::kStandingRearm && target != 50'),
 ('bal_rearmed_wrong_units','src/posture_core.hpp','c.units_rearmed += e.posture - target;','c.units_rearmed += e.posture;'),
 ('bal_delivered_twice','src/posture_core.hpp','if(e.balance_break_delivered){','if(false){'),
]
hf01=[
 ('hf_ignore_occupied','src/hudfix_core.hpp','if(!intersects(candidate,obstacle))continue;','continue;'),
 ('hf_move_unrelated_root','src/hudfix_core.hpp','||!competing_only(i,band)',''),
 ('hf_band_no_halo','src/hudfix_core.hpp','l.half_w-48*l.u','l.half_w+2*l.u'),
 ('hf_band_no_pulse','src/hudfix_core.hpp','l.frame_h-17*l.u','l.frame_h+2*l.u'),
 ('hf_overlap_ignored','src/hudfix_core.hpp','a.x0<b.x1&&a.x1>b.x0&&a.y0<b.y1&&a.y1>b.y0','false'),
 ('hf_collision_not_moved','src/hudfix_core.hpp','m.needed=true;','return m;'),
 ('hf_move_wrong_sign','src/hudfix_core.hpp','band.y0-box.y1','box.y1-band.y0'),
 ('hf_viewport_ignored','src/hudfix_core.hpp','contains(viewport,r)&&!intersects(r,band)','!intersects(r,band)'),
 ('hf_overflow_writes','src/hudfix_core.hpp','if(!complete)return 0;',''),
 ('hf_inverse_wrong','src/hudfix_core.hpp','parent.v[1][1]*dx-parent.v[0][1]*dy','parent.v[1][1]*dx+parent.v[0][1]*dy'),
 ('hf_hide_player_without_enemy','src/hudfix_core.hpp','const bool next=allowed&&native_queued&&value.alpha>=0.01f&&l.ok;','const bool next=false;'),
 ('hf_minimal_hud_ignored','src/hudfix_core.hpp','allowed&&native_queued&&value.alpha','allowed&&value.alpha'),
 ('hf_fade_stuck_on','src/hudfix_core.hpp','value.alpha>=0.01f','value.alpha>=0.0f'),
]
source=[('extra_hp','r->health=7;'),('power','r->mSphs_at[0].SetAtAtp(6);'),('anim_rate','r->mpMorf->setPlaySpeed(2);'),('cooldown_elsewhere','r->field_0x6cc=0;'),('second_writer','r->mActionMode=6;'),('at_set_outside_helper','r->mSphs_at[0].OnAtSetBit();')]
# Fail before any compiler starts if a source evolution invalidated a mutant.
for suite in (logic,hud,parry,lnk,s1,fix01,s2,s2f,balance,hf01):
 for name,filename,anchor,replacement in suite:
  if anchor not in (root/filename).read_text(encoding='utf-8'):
   raise RuntimeError('MUTATION_ANCHOR_ERROR: '+name+' in '+filename)
# POST07: source-audit negatives for the display-only HUD block (each must be rejected by ci/source_audit.py)
hud_src=[
 ('f01_adapter_guard_removed','ap, g.cur.continuation==lnks1fix::Continuation::Safe);','ap, true);'),
 ('f01_capture_no_first_priority','n.blocked=link->mTgCyls[k].ChkTgShieldHit()!=0;break;','n.blocked=link->mTgCyls[k].ChkTgShieldHit()!=0;'),
 ('f01_native_source_fabricated','n.collider=reinterpret_cast<std::uintptr_t>(link->mTgCyls[k].cCcD_ObjHitInf::GetTgHitObj());','n.collider=1;'),
 ('f01_native_hook_writes','n.timer_after=link->mDamageTimer;','link->mDamageTimer=0;n.timer_after=link->mDamageTimer;'),
 ('f01_actual_arm_fabricated','const bool actual=a->mSphs_at[0].ChkAtSet()||a->mSphs_at[1].ChkAtSet();','const bool actual=true;'),
 ('f01_poly_result_ignored','fx.native.poly_seen=true;fx.native.poly_result=*static_cast<const int*>(ret);','fx.native.poly_seen=true;fx.native.poly_result=0;'),

 ('hud_writes_meter','  ++hg.passes;\n','  ++hg.passes;\n  if (meter) meter->mStatus = 0;\n'),
 ('hud_writes_health','      smp.in.frozen = e.frozen;\n','      smp.in.frozen = e.frozen;\n      act->health = 1;\n'),
 ('hud_rearms_posture','      smp.in.frozen = e.frozen;\n','      smp.in.frozen = e.frozen;\n      g.pe.px.rearm(slot, L.generation, id, 0);\n'),
 ('hud_reads_breaker_internals','      smp.in.frozen = e.frozen;\n','      smp.in.frozen = e.frozen;\n      (void)g.br.c.calls;\n'),
 ('hud_second_queue_walk','void hud_drop_all() noexcept {\n','void hud_drop_all() noexcept {\n  (void)g_fopAcTg_Queue.mpHead;\n'),
 ('hud_direct_set','  hg.visible = lnk01::packet_needed(allowed, hg.frame.n, hg.player_on);\n','  hg.visible = lnk01::packet_needed(allowed, hg.frame.n, hg.player_on);\n  g_dComIfG_gameInfo.drawlist.set2DXlu(&hud_packet);\n'),
 ('hud_painter_reads_posture','    post07::Quad quads[post07::kMaxQuads];\n','    post07::Quad quads[post07::kMaxQuads];\n    (void)g.pe.px.entry(0);\n'),
 ('hud_no_scrub_on_shutdown','  hg.active = false;\n  hud_drop_all();\n  // SESSION_SHUTDOWN','  hg.active = false;\n  // SESSION_SHUTDOWN'),
 ('hud_extra_service','IMPORT_OPTIONAL_SERVICE(InterpService, svc_interp);','IMPORT_OPTIONAL_SERVICE(InterpService, svc_interp);\nIMPORT_OPTIONAL_SERVICE(LogService, svc_log2);'),
 ('hud_extra_hook','DEFINE_HOOK(&dMeter2_c::_delete, MeterDeletePost07);','DEFINE_HOOK(&dMeter2_c::_delete, MeterDeletePost07);\nDEFINE_HOOK(&dMeter2_c::_create, MeterCreatePost07);'),
 ('hud_core_reads_posture','src/hud_core.hpp',None),
 # POST08: parry adapter negatives
 ('parry_writes_hp','  ++g.p8.attempts;\n','  ++g.p8.attempts;\n  a->health = g.cur.pre_hp;\n'),
 ('parry_writes_link','    in.now.guard_attack = link->mProcID == daAlink_c::PROC_GUARD_ATTACK;\n','    in.now.guard_attack = link->mProcID == daAlink_c::PROC_GUARD_ATTACK;\n    link->mProcID = 0;\n'),
 ('parry_cached_actor_read','    in.at_shield[i] = a->mSphs_at[i].ChkAtShieldHit();\n','    in.at_shield[i] = a->mSphs_at[i].ChkAtShieldHit() && a->mSphs_at[i].GetAtHitAc();\n'),
 ('parry_judge_writes','  g.cur.pv = g.parry.judge(in);\n','  g.cur.pv = g.parry.judge(in);\n  restore_reaction(a, g.cur.pre_live);\n'),
 ('parry_second_deflect','        deflected = post05::same_live(read_live(a), t);\n','        deflected = post05::same_live(read_live(a), t);\n        post08_deflect(a, t);\n'),
 ('parry_target_not_pre','        post05::LiveMode t = g.cur.pre_live;  //','        post05::LiveMode t = now;  //'),
 ('parry_no_shape_check','    if (plan == post08::DeflectPlan::Refuse) stage = "REACTION_UNEXPECTED";\n    else if','    if'),
 ('parry_apply_outside','  post08_update();\n','  post08_update();\n  (void)g.pe.px.apply_parry(0, 0, 0, 1, g.eng.obs.ledger);\n'),
 ('parry_disabled_twice','  g.parry.enabled = true;','  g.parry.enabled = true;\n  g.parry.enabled = true;'),
 ('fix01_judge_uses_now','  in.pass = g.link_track.pass_for(g.p8.tick, in.link_ok, link_id);','  in.pass = in.now;'),
 ('fix01_observe_twice','  post08_update();\n  post08_observe_link();','  post08_update();\n  post08_observe_link();\n  post08_observe_link();'),
 ('fix02_judge_ignores_fresh','  in.fresh = in.tracked && g.arm.fresh(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick);','  in.fresh = in.tracked;'),
 ('fix02_state3_always','        if (plan == post08::DeflectPlan::ForceRecoil) t.state = post08::kDeflectState;','        t.state = post08::kDeflectState;'),
 ('fix02_note_before_writes','  bool parry_crossed = false;\n','  bool parry_crossed = false;\n  g.arm.note_post(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick, true);\n'),
 ('fix02_watch_writes','  const bool dead = r.snap.health <= 0','  restore_reaction(nullptr, post05::LiveMode{});\n  const bool dead = r.snap.health <= 0'),
 ('fix01_observe_writes','    frame = link->mUnderFrameCtrl[0].getFrame();\n','    frame = link->mUnderFrameCtrl[0].getFrame();\n    link->field_0x3478 = 0.0f;\n'),
 # LNK01: source-audit negatives (each must be rejected)
 ('lnk_hides_whole_hud','    action_ui_hide(s);\n','    action_ui_hide(s);\n    meter->mpMeterDraw->mpScreen->hide();\n'),
 ('lnk_shows_again','    action_ui_hide(s);\n','    action_ui_hide(s);\n    s->show();\n'),
 ('lnk_global_alpha','    action_ui_hide(s);\n','    action_ui_hide(s);\n    g_drawHIO.mParentAlpha = 0.0f;\n'),
 ('lnk_writes_link_proc','  lg.posture.observe_link(l.valid, l.id);\n','  lg.posture.observe_link(l.valid, l.id);\n  static_cast<daAlink_c*>(dComIfGp_getLinkPlayer())->mProcID = 0;\n'),
 ('lnk_writes_guard_counter','  lg.posture.observe_link(l.valid, l.id);\n','  lg.posture.observe_link(l.valid, l.id);\n  (void)static_cast<daAlink_c*>(dComIfGp_getLinkPlayer())->field_0x2fd5;\n'),
 ('lnk_writes_life','  const lnk01::Outcome o = lg.posture.on_contact(k);\n','  const lnk01::Outcome o = lg.posture.on_contact(k);\n  g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().setLife(1);\n'),
 ('lnk_contact_twice','  lnk01_contact();  //','  lnk01_contact();\n  lnk01_contact();  //'),
 ('lnk_contact_before_parry','  bool parry_crossed = false;\n','  lnk01_contact();\n  bool parry_crossed = false;\n'),
 ('lnk_regen_in_draw','  if (!hg.active || !args) return;\n  ++hg.meter_draws;','  if (!hg.active || !args) return;\n  lg.posture.tick(true);\n  ++hg.meter_draws;'),
 ('lnk_painter_reads_posture','    if (hg.player_on && !hf.enabled) {\n      post07::Quad lq','    if (hg.player_on && !hf.enabled) {\n      (void)lg.posture.milli();\n      post07::Quad lq'),
 ('lnk_pre_installed_first','    hq = mods::hook::add_post<PromptDrawLnk01>(on_prompt_draw_post);\n    hp = hq == MOD_OK ? mods::hook::add_pre<PromptDrawLnk01>(on_prompt_draw_pre) : MOD_ERROR;','    hp = mods::hook::add_pre<PromptDrawLnk01>(on_prompt_draw_pre);\n    hq = hp == MOD_OK ? mods::hook::add_post<PromptDrawLnk01>(on_prompt_draw_post) : MOD_ERROR;'),
 ('lnk_mask_any_prompt','    const bool live = self && aligned8(meter) && meter->mpEmpButton == self;','    const bool live = self && aligned8(meter);'),
 ('lnk_no_vptr_guard','  if (!in.vptr_ok) return in;\n','  if (false) return in;\n'),
 ('lnk_hide_outside_step','  if (!lnk_take(lg.ui_lines, lg.ui_dropped, 40)) return;\n','  if (!lnk_take(lg.ui_lines, lg.ui_dropped, 40)) return;\n  action_ui_hide(nullptr);\n'),
 ('lnk_regen_ignores_pause','  lg.sim_active = l.valid && life > 0 && !g_dComIfG_gameInfo.play.isPauseFlag() &&','  lg.sim_active = l.valid && life > 0 &&'),
 ('lnk_extra_slot_write','  r->field_0x4be[1] = s1;\n}','  r->field_0x4be[1] = s1;\n  r->mMsgID = 0;\n}'),
 # LNK SESSION01: source-audit negatives
 ('s1_writes_damage_timer','  l.damage_timer = static_cast<int>(lp->mDamageTimer);','  lp->mDamageTimer = 0;\n  l.damage_timer = static_cast<int>(lp->mDamageTimer);'),
 ('s1_pre_clears_tg','    g.cur.lnk_tg_cyl = k;\n','    g.cur.lnk_tg_cyl = k;\n    const_cast<daAlink_c*>(link)->mTgCyls[k].ClrTgHit();\n'),
 ('s1_guard_ignores_invulnerability','ob.human && l.upper_guard && l.damage_timer == 0 && l.proc','ob.human && l.upper_guard && l.proc'),
 ('s1_confirm_twice','  lg.posture.confirm_direct(ob, g.p8.tick, de);','  lg.posture.confirm_direct(ob, g.p8.tick, de);\n  lg.posture.confirm_direct(ob, g.p8.tick, de);'),
 ('s1_any_cylinder','    g.cur.lnk_tg_match = hit != 0 && (hit == s0 || hit == s1);\n    break;\n','    g.cur.lnk_tg_match = g.cur.lnk_tg_match || (hit != 0 && (hit == s0 || hit == s1));\n'),
 ('s1_tg_assumed','  k.tg_match = g.cur.lnk_tg_match;','  k.tg_match = true;'),
 ('s1_writes_pending_life','  ob.pending_life = g_dComIfG_gameInfo.play.getItemLifeCount();','  g_dComIfG_gameInfo.play.setItemLifeCount(0.0f, 0);\n  ob.pending_life = g_dComIfG_gameInfo.play.getItemLifeCount();'),
 ('s1_regen_before_confirm','  lg.posture.confirm_direct(ob, g.p8.tick, de);  //','  (void)0;  //'),
]
s2_src=[
 # LNK SESSION02: source-audit negatives (each must be rejected)
 ('s2_writes_proc','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  link->mProcID = daAlink_c::PROC_GUARD_BREAK;\n'),
 ('s2_writes_counter','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  link->field_0x2fd5 = 0;\n'),
 ('s2_writes_mode','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  link->mModeFlg |= 4;\n'),
 ('s2_on_mode_flag','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  link->onModeFlg(4);\n'),
 ('s2_writes_damage_timer','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  link->mDamageTimer = 30;\n'),
 ('s2_slip_route','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardSlipInit();\n'),
 ('s2_damage_route','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  link->setDamagePoint(1, 0, 0, 0);\n'),
 ('s2_double_call','  (void)link->procGuardBreakInit();\n','  (void)link->procGuardBreakInit();\n  (void)link->procGuardBreakInit();\n'),
 ('s2_entry_assumed','  return link->mProcID == daAlink_c::PROC_GUARD_BREAK;\n}','  return true;\n}'),
 ('s2_call_before_gate','    const lnk05::Gate gate = lb.m.check(gi);\n','    (void)lnk05_native_break(link);\n    const lnk05::Gate gate = lb.m.check(gi);\n'),
 ('s2_call_from_update','    const lnk05::Out bo = lb.m.observe(','    (void)lnk05_native_break(nullptr);\n    const lnk05::Out bo = lb.m.observe('),
 ('s2_no_pending_check','    if (!lb.enabled || !args || lb.m.state() != lnk05::State::TriggerPending) return;','    if (!lb.enabled || !args) return;'),
 ('s2_damage_veto_blind','                          static_cast<int>(link->mDamageTimer) > fx.native.timer_before;','                          false;'),
 ('s2_second_hook','  const ModResult lnk05_hook = mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_lnk05);','  const ModResult lnk05_hook = mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_lnk05);\n  (void)mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_lnk05);'),
 ('s2_reset_unconditional','    if (bo.reset_posture) lg.posture.break_reset();','    lg.posture.break_reset();'),
 ('s2_reset_at_request','  lg.posture.hold(lb.m.hold());\n}','  lg.posture.hold(lb.m.hold());\n  lg.posture.break_reset();\n}'),
 ('s2_regen_request','    const lnk05::Out bo = lb.m.observe(','    lnk05_request(lnk05::Kind::Block, 0, 0, 0, 0, lg.posture.milli());\n    const lnk05::Out bo = lb.m.observe('),
 ('s2_parry_requests','  if (o.v == lnk01::Verdict::Confirmed) lnk05_request(lnk05::Kind::Block,','  if (o.v != lnk01::Verdict::None) lnk05_request(lnk05::Kind::Block,'),
 ('s2_no_parry_cancel','    lnk05_log(lb.m.parry(g.p8.tick), "LNK05_EVENT", lb.event_lines, lb.event_dropped, 150);','    (void)0;'),
 ('s2_observe_twice','    if (bo.reset_posture) lg.posture.break_reset();','    if (bo.reset_posture) lg.posture.break_reset();\n    (void)lb.m.observe(bi, false);'),
 ('s2_hold_forced','    lg.posture.hold(lb.m.hold());\n    if (lf.enabled && (bo.ev','    lg.posture.hold(true);\n    if (lf.enabled && (bo.ev'),
]
hud_src+=s2_src
s2f_src=[
 # LNK SESSION02 FIX01: source-audit negatives (each must be rejected)
 ('s2f_hook_crouch_procwait','DEFINE_HOOK(&daAlink_c::setCollision, LinkCollisionFx);','DEFINE_HOOK(&daAlink_c::setCollision, LinkCollisionFx);\nDEFINE_HOOK(&daAlink_c::procWait, LinkWaitCrouch);'),
 ('s2f_hook_dodge_proc','DEFINE_HOOK(&daAlink_c::setCollision, LinkCollisionFx);','DEFINE_HOOK(&daAlink_c::setCollision, LinkCollisionFx);\nDEFINE_HOOK(&daAlink_c::procFrontRollInit, LinkRollVeto);'),
 ('s2f_hook_anim','DEFINE_HOOK(&daAlink_c::setCollision, LinkCollisionFx);','DEFINE_HOOK(&daAlink_c::setCollision, LinkCollisionFx);\nDEFINE_HOOK(&daAlink_c::setSingleAnimeParam, LinkAnimRewrite);'),
 ('s2f_flag_on','      link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);  // ESCRITURA ACOTADA (owner=MANUAL_SHIELD)','      link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);  // ESCRITURA ACOTADA (owner=MANUAL_SHIELD)'),
 ('s2f_third_flag_off','    lf.guard_accepted = gd == lnkfx::Guard::Accepted;\n','    lf.guard_accepted = gd == lnkfx::Guard::Accepted;\n    if (lf.r_held) link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);\n'),
 ('s2f_flag_off_unconditional','    if (lnkfx::clears_flag(gd)) {\n      link->offNoResetFlg2','    if (true) {\n      link->offNoResetFlg2'),
 ('s2f_b_global','    if (lnkfx::consume_b(lf.b_trig, lf.r_held, lf.def.phase(), lf.bash_offered_prev)) {','    if (lf.r_held) {'),
 ('s2f_second_input_write','    lf.sample_ok = true;\n','    lf.sample_ok = true;\n    link->mItemTrigger |= daAlink_c::BTN_R;\n'),
 ('s2f_raw_pad','    lf.r_held = (link->mItemButton & daAlink_c::BTN_R) != 0;','    lf.r_held = mDoCPd_c::getHoldLockR(PAD_1) != 0;'),
 ('s2f_writes_proc','    const int proc = static_cast<int>(link->mProcID);\n','    const int proc = static_cast<int>(link->mProcID);\n    link->mProcID = daAlink_c::PROC_WAIT;\n'),
 ('s2f_anim_rate','    const int proc = static_cast<int>(link->mProcID);\n','    const int proc = static_cast<int>(link->mProcID);\n    link->mUnderFrameCtrl[0].setRate(0.5f);\n'),
 ('s2f_damage_artificial','    ++lf.dodge_by[static_cast<unsigned>(d)];\n','    ++lf.dodge_by[static_cast<unsigned>(d)];\n    link->setDamagePoint(1, 0, 0, 0);\n'),
 ('s2f_reset_on_complete','    lnkfx_penalty_log(lf.def.observe(fo));\n','    lnkfx_penalty_log(lf.def.observe(fo));\n    if (lf.def.phase() == lnkfx::Phase::RearmRequired) lg.posture.break_reset();\n'),
 ('s2f_skip_any_status','    if (mods::arg<u8>(args, 1) != BUTTON_STATUS_SHIELD_ATTACK || !lnkfx_link(link, id)) return HOOK_CONTINUE;','    if (!lnkfx_link(link, id)) return HOOK_CONTINUE;'),
 ('s2f_dodge_always','    if (!lf.enabled || !args || !ret || !lf.def.dodge_blocked()) return HOOK_CONTINUE;','    if (!lf.enabled || !args || !ret) return HOOK_CONTINUE;'),
 ('s2f_collision_in_break','    if (proc == daAlink_c::PROC_GUARD_BREAK) return HOOK_CONTINUE;\n',''),
 ('s2f_collision_always','    if (!lf.enabled || !args || !lf.def.guard_blocked()) return HOOK_CONTINUE;','    if (!lf.enabled || !args) return HOOK_CONTINUE;'),
 ('s2f_partial_install','    (void)mods::hook::uninstall<LinkSpActionFx>(), (void)mods::hook::uninstall<LinkMoveDoFx>(), (void)mods::hook::uninstall<LinkCollisionFx>();\n',''),
 ('s2f_enable_early','  const ModResult fr[6] = {','  lf.enabled = true;\n  const ModResult fr[6] = {'),
 ('s2f_bonus_r_held','  ob.guard_safe = lnkfx::regen_bonus(ob.guard_safe, lf.enabled, lf.guard_accepted, lf.def.phase());','  ob.guard_safe = ob.guard_safe || lf.r_held;'),
 ('s2f_request_unblocked','  if (lf.enabled && lf.def.request_blocked()) {','  if (false) {'),
 ('s2f_second_native_call','    if (entered && lf.enabled) {','    if (entered && lf.enabled && lnk05_native_break(link)) {'),
 ('s2f_begin_on_request','  lnk05_log(lb.m.request(e), "LNK05_EVENT", lb.event_lines, lb.event_dropped, 150);\n','  lnk05_log(lb.m.request(e), "LNK05_EVENT", lb.event_lines, lb.event_dropped, 150);\n  lnkfx_penalty_log(lf.def.begin(lnkfx::Begin{}));\n'),
 ('s2f_lose_fix01_hook','     mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_fix01)!=MOD_OK ||\n',''),
 ('s2f_const_dup','  const ModResult fr[6] = {','  constexpr std::uint64_t kTotalPenaltyTicks = 45;\n  (void)kTotalPenaltyTicks;\n  const ModResult fr[6] = {'),
 ('s2f_bstatus_always','    if (lf.r_held && lf.def.phase() == lnkfx::Phase::Ready) {\n      link->setBStatus','    if (true) {\n      link->setBStatus'),
 ('s2f_bash_forced','    *static_cast<BOOL*>(ret) = b == lnkfx::Bash::Accepted ? TRUE : FALSE;','    *static_cast<BOOL*>(ret) = TRUE;'),
 ('s2f_crouch_shield','    lf.sample_ok = true;\n','    lf.sample_ok = true;\n    (void)daAlink_c::ANM_CROUCH_DEF;\n'),
]
hud_src+=s2f_src
hud_src.append(('s2f_core_impure','src/lnks2fix_core.hpp',None))
hud_src.append(('s2_core_impure','src/lnks2_core.hpp',None))
for item in hud_src:
 if item[2] is not None and item[1] not in (root/'src/mod.cpp').read_text(encoding='utf-8'):
  raise RuntimeError('SOURCE_MUTATION_ANCHOR_ERROR: '+item[0])
def source_result(name,r):
 try:j=json.loads(r.stdout)
 except (ValueError,TypeError):return {'mutation':name,'detected':False,'status':'ERROR','stage':'AUDIT_INVALID_JSON'}
 killed=r.returncode==1 and j.get('status')=='SOURCE_AUDIT_FAIL' and bool(j.get('errors'))
 return {'mutation':name,'detected':killed,'status':'KILLED' if killed else 'SURVIVED','audit_status':j.get('status'),'audit_error_count':len(j.get('errors',[]))}

def one(item,lab='break_lab'):
 name,fn,old,new=item
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests',td/'tests');p=td/fn;s=p.read_text(encoding='utf-8');assert old in s,name;p.write_text(s.replace(old,new,1),encoding='utf-8')
  exe=td/('lab.exe' if os.name=='nt' else 'lab');cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(td/('tests/'+lab+'.cpp')),'/Fe'+str(exe),'/Fo'+str(td/'lab.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(td/('tests/'+lab+'.cpp')),'-o',str(exe)]
  try:r=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
  except subprocess.TimeoutExpired:return {'mutation':name,'detected':False,'status':'ERROR','stage':'COMPILE_TIMEOUT'}
  if r.returncode:return {'mutation':name,'detected':False,'status':'ERROR','stage':'COMPILE','error':(r.stdout+r.stderr)[-1600:]}
  try:r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60)
  except subprocess.TimeoutExpired:return {'mutation':name,'detected':False,'timeout':True,'status':'TIMEOUT'}  # a hang is NEVER counted as a detection: labs must be bounded
  return {'mutation':name,'detected':r.returncode!=0}
# A negative must fail for its mutation, never missing fixtures, encoding or
# a broken auditor. First execute a byte-identical positive clone.
with tempfile.TemporaryDirectory() as td:
 td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests/cp06_reference',td/'tests/cp06_reference');shutil.copytree(root/'tests/fixtures',td/'tests/fixtures');shutil.copytree(root/'ci',td/'ci')
 r=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(td)],capture_output=True,text=True,timeout=60)
 assert r.returncode==0 and json.loads(r.stdout)['status']=='SOURCE_AUDIT_PASS','source auditor positive clone failed'
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as e:results=list(e.map(one,logic))+list(e.map(lambda it:one(it,'hud_lab'),hud))+list(e.map(lambda it:one(it,'parry_lab'),parry))+list(e.map(lambda it:one(it,'lnk_lab'),lnk))+list(e.map(lambda it:one(it,'lnks1_lab'),s1))+list(e.map(lambda it:one(it,'fix01_lab'),fix01))+list(e.map(lambda it:one(it,'lnks2_lab'),s2))+list(e.map(lambda it:one(it,'lnks2fix_lab'),s2f))+list(e.map(lambda it:one(it,'balance_lab'),balance))+list(e.map(lambda it:one(it,'hudfix_lab'),hf01))
for name,code in source:
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests/cp06_reference',td/'tests/cp06_reference');shutil.copytree(root/'tests/fixtures',td/'tests/fixtures');shutil.copytree(root/'ci',td/'ci');p=td/'src/mod.cpp';s=p.read_text(encoding='utf-8');p.write_text(s.replace('  r->health = static_cast<s16>(hp);','  r->health = static_cast<s16>(hp);\n  '+code,1),encoding='utf-8')
  r=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(td)],capture_output=True,text=True,timeout=60);results.append(source_result(name,r))
for item in hud_src:
 name=item[0]
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests/cp06_reference',td/'tests/cp06_reference');shutil.copytree(root/'tests/fixtures',td/'tests/fixtures');shutil.copytree(root/'ci',td/'ci')
  if item[1]=='src/hud_core.hpp':
   p=td/'src/hud_core.hpp';s=p.read_text(encoding='utf-8');p.write_text(s.replace('namespace post07 {','namespace post07 {\nusing marker = post04::Entry;',1),encoding='utf-8')
  elif item[1]=='src/lnks2fix_core.hpp':
   p=td/'src/lnks2fix_core.hpp';s=p.read_text(encoding='utf-8');assert 'namespace lnkfx {' in s;p.write_text(s.replace('namespace lnkfx {','namespace lnkfx {\ninline std::uintptr_t key(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }',1),encoding='utf-8')
  elif item[1]=='src/lnks2_core.hpp':
   p=td/'src/lnks2_core.hpp';s=p.read_text(encoding='utf-8');assert 'namespace lnk05 {' in s;p.write_text(s.replace('namespace lnk05 {','namespace lnk05 {\ninline std::uintptr_t key(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }',1),encoding='utf-8')
  else:
   p=td/'src/mod.cpp';s=p.read_text(encoding='utf-8');assert item[1] in s,name;p.write_text(s.replace(item[1],item[2],1),encoding='utf-8')
  r=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(td)],capture_output=True,text=True,timeout=60);results.append(source_result(name,r))
for row in results:
 row.setdefault('status','KILLED' if row['detected'] else 'SURVIVED')
print(json.dumps({'source_audit_positive_clone':'PASS','mutations':results,'count':len(results),'detected':sum(x['detected'] for x in results),'timeouts':sum(1 for x in results if x.get('timeout'))},indent=2));sys.exit(0 if all(x['detected'] for x in results) else 1)
