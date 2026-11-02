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
 ('wrong_abort_posture','src/break_core.hpp','a.rearm_to=50','a.rearm_to=0'),
 ('wrong_timeout_posture','src/break_core.hpp','Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=50','Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=0'),
 ('early_rearm','src/break_core.hpp','s.phase==Phase::GettingUp&&v.mode==3&&v.state==5','s.phase==Phase::Down&&v.mode==3&&v.state==5'),
 ('no_normalize_helm','src/helm_core.hpp','r.hp=pre>r.base?pre-r.base:0','r.hp=native'),
 ('heal_native_loss','src/helm_core.hpp','r.hp=pre>r.base?pre-r.base:0','r.hp=pre>floor?pre-floor:0'),
 ('double_rounding','src/break_core.hpp','a.extra=(base+1)/2','a.extra=(base+2)/2'),
 # POST07 FIX_REARM50
 ('redirect_rearm_50','src/break_core.hpp','{a.act=Act::Rearm;a.rearm_to=0;a.why=Why::RearmAfterGetUpRedirected;}','{a.act=Act::Recover;a.rearm_to=50;a.why=Why::TechnicalAbort;}'),
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
 ('parry_late_allowed','src/parry_core.hpp','else if (e.resolved) v.result = Result::MissedLate;','else if (false) v.result = Result::MissedLate;'),
 ('parry_double_sphere','src/parry_core.hpp','else if (e.parried) v.result = Result::Duplicate;','else if (false) v.result = Result::Duplicate;'),
 ('parry_no_restart_token','src/parry_core.hpp','        e.token = ++next_token_;\n','        e.token = 1;\n'),
 ('parry_latched_allowed','src/parry_core.hpp','    } else if (in.latched) v.result = Result::Latched;','    } else if (false) v.result = Result::Latched;'),
 ('parry_fury_allowed','src/parry_core.hpp','    else if (fury_chip_possible(in)) v.result = Result::FuryChipPossible;','    else if (false) v.result = Result::FuryChipPossible;'),
 ('parry_idle_allowed','src/parry_core.hpp','    else if (!swing || !e.active) v.result = Result::NotAttacking;','    else if (false) v.result = Result::NotAttacking;'),
 ('parry_generation_inherit','src/parry_core.hpp','if (!e.bound || e.id != in.id || e.generation != in.generation) {  // identidad','if (!e.bound) {  // identidad'),
 ('parry_commit_twice','src/parry_core.hpp','token == 0 || e.parried) return;','token == 0) return;'),
 ('parry_deflect_any_shape','src/parry_core.hpp','  return untouched || shield_stun;','  return true;'),
 ('parry_deflect_state','src/parry_core.hpp','inline constexpr int kDeflectState = 3;','inline constexpr int kDeflectState = 1;'),
 ('posture_gain_80','src/posture_core.hpp','inline constexpr int kParryPostureGain = 40;','inline constexpr int kParryPostureGain = 80;'),
 ('posture_no_saturation','src/posture_core.hpp','o.after = e.posture + policy::kParryPostureGain > policy::kMaxPosture ? policy::kMaxPosture : e.posture + policy::kParryPostureGain;','o.after = e.posture + policy::kParryPostureGain;'),
 ('posture_no_regen_reset','src/posture_core.hpp','    e.k = 0;\n    e.regen_acc = 0;\n    e.last_hit_tick = now_;\n    e.last_parry_token = token;','    e.last_hit_tick = now_;\n    e.last_parry_token = token;'),
 ('posture_no_token_dedupe','src/posture_core.hpp','      else if (e.last_parry_token == token) o.reject = ParryReject::DuplicateToken;\n',''),
 ('posture_parry_latched','src/posture_core.hpp','      else if (e.latched) o.reject = ParryReject::Latched;\n      else if (e.last_parry_token','      else if (e.last_parry_token'),
 ('posture_no_crossing_count','src/posture_core.hpp','      ++c.crossings;\n      ++c.parry_crossings;','      ++c.parry_crossings;'),
 ('posture_audit_weakened','src/posture_core.hpp','a.ok = a.e1 && a.e2 && a.e3 && a.e4 && a.e5 && a.e6 && a.invariants;','a.ok = a.e1 && a.e2 && a.e3 && a.e4 && a.e5 && a.invariants;'),
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

source=[('extra_hp','r->health=7;'),('power','r->mSphs_at[0].SetAtAtp(6);'),('anim_rate','r->mpMorf->setPlaySpeed(2);'),('cooldown_elsewhere','r->field_0x6cc=0;'),('second_writer','r->mActionMode=6;')]
def one(item,lab='break_lab'):
 name,fn,old,new=item
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests',td/'tests');p=td/fn;s=p.read_text();assert old in s,name;p.write_text(s.replace(old,new,1))
  exe=td/('lab.exe' if os.name=='nt' else 'lab');cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(td/('tests/'+lab+'.cpp')),'/Fe'+str(exe),'/Fo'+str(td/'lab.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(td/('tests/'+lab+'.cpp')),'-o',str(exe)]
  r=subprocess.run(cmd,capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
  r=subprocess.run([str(exe)],capture_output=True,text=True);return {'mutation':name,'detected':r.returncode!=0}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as e:results=list(e.map(one,logic))+list(e.map(lambda it:one(it,'hud_lab'),hud))+list(e.map(lambda it:one(it,'parry_lab'),parry))
for name,code in source:
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');p=td/'src/mod.cpp';s=p.read_text();p.write_text(s.replace('  r->health = static_cast<s16>(hp);','  r->health = static_cast<s16>(hp);\n  '+code,1))
  r=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(td)],capture_output=True,text=True);results.append({'mutation':name,'detected':r.returncode!=0})
# POST07: source-audit negatives for the display-only HUD block (each must be rejected by ci/source_audit.py)
hud_src=[
 ('hud_writes_meter','  ++hg.passes;\n','  ++hg.passes;\n  if (meter) meter->mStatus = 0;\n'),
 ('hud_writes_health','      smp.in.frozen = e.frozen;\n','      smp.in.frozen = e.frozen;\n      act->health = 1;\n'),
 ('hud_rearms_posture','      smp.in.frozen = e.frozen;\n','      smp.in.frozen = e.frozen;\n      g.pe.px.rearm(slot, L.generation, id, 0);\n'),
 ('hud_reads_breaker_internals','      smp.in.frozen = e.frozen;\n','      smp.in.frozen = e.frozen;\n      (void)g.br.c.calls;\n'),
 ('hud_second_queue_walk','void hud_drop_all() noexcept {\n','void hud_drop_all() noexcept {\n  (void)g_fopAcTg_Queue.mpHead;\n'),
 ('hud_direct_set','  hg.visible = allowed && hg.frame.n > 0;\n','  hg.visible = allowed && hg.frame.n > 0;\n  g_dComIfG_gameInfo.drawlist.set2DXlu(&hud_packet);\n'),
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
 ('parry_target_not_pre','        post05::LiveMode t = g.cur.pre_live;\n','        post05::LiveMode t = now;\n'),
 ('parry_no_shape_check','  else if (!post08::deflect_allowed(pre_s, now_s)) stage = "REACTION_UNEXPECTED";\n',''),
 ('parry_apply_outside','  post08_update();\n','  post08_update();\n  (void)g.pe.px.apply_parry(0, 0, 0, 1, g.eng.obs.ledger);\n'),
 ('parry_disabled_twice','  g.parry.enabled = true;','  g.parry.enabled = true;\n  g.parry.enabled = true;'),
 ('fix01_judge_uses_now','  in.pass = g.link_track.pass_for(g.p8.tick, in.link_ok, link_id);','  in.pass = in.now;'),
 ('fix01_observe_twice','  post08_update();\n  post08_observe_link();','  post08_update();\n  post08_observe_link();\n  post08_observe_link();'),
 ('fix01_observe_writes','    frame = link->mUnderFrameCtrl[0].getFrame();\n','    frame = link->mUnderFrameCtrl[0].getFrame();\n    link->field_0x3478 = 0.0f;\n'),
]
for item in hud_src:
 name=item[0]
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src')
  if item[1]=='src/hud_core.hpp':
   p=td/'src/hud_core.hpp';s=p.read_text();p.write_text(s.replace('namespace post07 {','namespace post07 {\nusing marker = post04::Entry;',1))
  else:
   p=td/'src/mod.cpp';s=p.read_text();assert item[1] in s,name;p.write_text(s.replace(item[1],item[2],1))
  r=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(td)],capture_output=True,text=True);results.append({'mutation':name,'detected':r.returncode!=0})
print(json.dumps({'mutations':results,'count':len(results),'detected':sum(x['detected'] for x in results)},indent=2));sys.exit(0 if all(x['detected'] for x in results) else 1)
