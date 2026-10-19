"""CP17 FIX01 mutation check: each mutant of red_ai_policy.hpp must make cp17f1_lab fail. Local evidence (g++), not a CI gate.
Usage: python3 -I tests/cp17f1_mutations.py <work-dir>   (from the FIX01 folder)"""
import shutil,subprocess,sys
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];WORK=Path(sys.argv[1]).resolve()
POL='runtime/DEVF/CP17_FIX01_RED_X2_JUMP_INTERCEPT/expansion/red_ai_policy.hpp';VAR='runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp'
M=[('fast_rate_x3','kBurst=2.0f','kBurst=3.0f'),('normal_rate_1_3','kNormal=1.15f','kNormal=1.3f'),('rush_1_4','kRush=1.5f','kRush=1.4f'),
 ('charge_70_roll_reintroduced','  if(!defer){s.episode_selected=true;','  if(!defer&&chance(s,70)){s.episode_selected=true;'),
 ('random_blocked_by_reactive_cooldown','random_ready(s)&&chance(s,kRandomPercent)','random_ready(s)&&reactive_ready(s)&&chance(s,kRandomPercent)'),
 ('odd_phase_x2_at_frame1','if(chosen==kBurst){s.pending=true;s.owned=false;s.own_rate=1;}else{','if(false){}else{'),
 ('pose24_left_registered',' if(last){\n',' if(false&&last){\n'),
 ('sweep_radius_grows','rt=double(kTipR)-err_t;','rt=double(kTipR)+err_t;'),
 ('interception_not_directed','  else if(!directed(a,l)){','  else if(false){'),
 ('no_landing_time_check','   else if(p.land>=0&&p.land-1<kOpen){','   else if(false){'),
 ('rush_speed_not_restored','if(absd(double(a.speed)-s.rush_r)<=1e-3){out={true,s.rush_n};','if(absd(double(a.speed)-s.rush_r)<=1e-3){out={false,s.rush_n};'),
 ('sample_spam','if(s.sample_id==l.id&&s.sample_flags==f)return false;','if(s.sample_id==l.id&&s.sample_flags==f)return true;'),
 ('no_critical_reserve','  if(is_critical){if(crit<kCritGlobal','  if(false){if(crit<kCritGlobal'),
 ('helm_splitter_intercepted','if(proc==kProcLargeJump&&!charge_any)return Jump::JumpStrike;','if((proc==kProcLargeJump||proc==0x2d)&&!charge_any)return Jump::JumpStrike;'),
 ('episode_consumed_by_failed_tick','  else if(!s.episode_deferred_logged){s.episode_deferred_logged=true;','  else if(!s.episode_deferred_logged){s.episode_reacted=true;s.episode_deferred_logged=true;'),
 ('force_ignores_facing',' if(angle_gap(a.angle,yaw(a.pos,l.pos))>facing_limit)return "RED_NOT_FACING_LINK";',''),
 ('force_ignores_lockout',' if(find4&&lockout_matters&&a.lockout!=0)return "NATIVE_POST_ATTACK_LOCKOUT";',''),
 ('stale_contact_flags',' if(s.decided&&s.last.seq==s.attack_seq&&!s.last.contact&&s.registered&&a.at_hit){',' if(s.decided&&s.last.seq==s.attack_seq&&!s.last.contact&&a.at_hit){'),
 ('force_from_find_stay','const bool find4=a.mode==Find&&a.state==4&&s.prev_find4,','const bool find4=(a.mode==Find||a.mode==FindStay)&&s.prev_find4,'),
 ('jump_strike_too_high','kReachXZ=260.,kReachY=110.,','kReachXZ=260.,kReachY=400.,'),
 ('double_reaction_charge_to_jump','    if(continued&&s.episode_reacted)jump_final(','    if(false)jump_final('),
 ('preempted_force_consumes_episode','  else if(s.forced_cause==Cause::ChargeSpin||s.forced_cause==Cause::ChargeJump){s.episode_reacted=false;s.episode_selected=false;}','')]
base=(TASK/POL).read_text();results=[]
for name,a,b in M:
 assert base.count(a)==1,(name,base.count(a))
 d=WORK/name;shutil.rmtree(d,ignore_errors=True);(d/'tests').mkdir(parents=True)
 for rel,text in [(POL,base.replace(a,b)),(VAR,(TASK/VAR).read_text())]:(d/rel).parent.mkdir(parents=True,exist_ok=True);(d/rel).write_text(text)
 shutil.copy(TASK/'tests/cp17f1_lab.cpp',d/'tests/cp17f1_lab.cpp')
 c=subprocess.run(['g++','-std=c++20','-O1','-w',str(d/'tests/cp17f1_lab.cpp'),'-o',str(d/'lab')],capture_output=True,text=True)
 if c.returncode:results.append((name,'COMPILE_ERROR',c.stderr[-300:]));continue
 r=subprocess.run([str(d/'lab')],capture_output=True,text=True,timeout=120)
 results.append((name,'KILLED' if r.returncode else 'SURVIVED',(r.stderr.strip().splitlines() or [''])[-1]))
for n,s,w in results:print(f'MUTANT {n} {s} {w}')
killed=sum(s=='KILLED' for _,s,_ in results);print(f'CP17F1_MUTATIONS killed={killed} total={len(results)}')
sys.exit(0 if killed==len(results) else 1)
