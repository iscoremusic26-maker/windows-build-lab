"""CP18 mutation check: each mutant of black_ai_policy.hpp must make cp18_lab fail. Local evidence (g++), not a CI gate.
Usage: python3 -I tests/cp18_mutations.py <work-dir>   (from the CP18 folder)"""
import shutil,subprocess,sys
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];WORK=Path(sys.argv[1]).resolve()
POL='runtime/DEVF/CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT/expansion/black_ai_policy.hpp'
DEPS=['runtime/DEVF/CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT/expansion/red_ai_policy.hpp','runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp',
 'runtime/DEVF/CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp','runtime/DEVF/CP10_5_DAMAGE_BLUE_RED_BLACK/expansion/damage_policy.hpp']
M=[('chain_removed_native_wait_branch_runs','  if(fury_on&&(leash==0||hunting)){\n','  if(false){\n'),
 ('chain_ignores_reach','   if(dist<kChainReach&&dy<=kChainHeight','   if(dy<=kChainHeight'),
 ('chain_ignores_facing','&&gap<kChainFacing&&!a.wall','&&!a.wall'),
 ('chain_ignores_los','&&!a.wall&&a.los){','&&!a.wall){'),
 ('chain_ignores_wall','&&!a.wall&&a.los){','&&a.los){'),
 ('chain_before_clip_complete','a.state>=1&&a.state<=3&&a.stopped){','a.state>=1&&a.state<=3&&(a.stopped||a.frame>=23)){'),
 ('fury_find0_wait_kept',' if(fury_on&&a.mode==Find&&a.state==0&&',' if(false&&a.mode==Find&&a.state==0&&'),
 ('fury_lockout_kept','c.lockout=true;c.lockout_value=0;',''),
 ('lockout_cleared_without_fury',' if(!fury_on)s.lockout_logged=false;\n else if(',' if(!fury_on)s.lockout_logged=false;\n if('),
 ('fury_permanent',' const int leash=leash_next(a);const bool fury_on=fury>0;',' const int leash=leash_next(a);const bool fury_on=fury>0||s.episodes>0;'),
 ('x2_never_applied',' if(fury){s.pending=true;s.owned=false;s.own_rate=1;}',' if(false){s.pending=true;s.owned=false;s.own_rate=1;}'),
 ('x2_odd_phase_at_frame1',' if(fury){s.pending=true;s.owned=false;s.own_rate=1;}',' if(fury){s.own_rate=kFury;s.owned=true;out={true,kFury};}'),
 ('normal_rate_not_written','else{s.own_rate=kNormal;s.owned=true;out={true,kNormal};}','else{s.own_rate=kNormal;s.owned=true;}'),
 ('room_epoch_ignored_in_pre','  else if(g.epoch!=s.hunt_epoch)reset(','  else if(false)reset('),
 ('poll_never_resets','{if(s.hunt&&g.epoch!=s.hunt_epoch)reset(','{if(false)reset('),
 ('hunt_reacquired_without_damage',',hunting=s.hunt&&same;',',hunting=(s.hunt||s.episodes>0)&&same;'),
 ('hunt_arms_link_other_room',' if(l.room!=a.room||l.stage!=a.stage){emit(',' if(false){emit('),
 ('hunt_arms_on_lethal_or_terminal',' if(!domain(a)||a.health<=0||terminal_mode(a.mode)){emit(',' if(!domain(a)){emit('),
 ('hunt_double_trigger',' if(s.hunt)return;                                                    // already locked',' // removed'),
 ('room_minus1_resets','  else if(link_valid(l)&&(l.room!=s.hunt_room','  else if(l.present&&(l.room!=s.hunt_room'),
 ('stage_change_ignored_by_observer',' if(l.room==g.room&&l.stage==g.stage)return false;',' if(l.room==g.room)return false;'),
 ('find4_timer_not_held','  if(a.timer<2){c.timer=true;','  if(false){c.timer=true;'),
 ('no_bounce_limit','  if(s.bounce>=kBounceLimit){','  if(false){'),
 ('attack_end_move_out_kept','  }else if(hunting&&leash!=0){set(','  }else if(false){set('),
 ('abandon_modes_not_redirected',' if(a.mode==Wait||a.mode==Walk||a.mode==MoveOut||a.mode==Watch||a.mode==Sound){',' if(false){'),
 ('natural_end_logged_as_foreign','if(!(a.stopped&&a.rate==0))emit("BLACK_RATE_OWNERSHIP_RELEASED"','emit("BLACK_RATE_OWNERSHIP_RELEASED"'),
 ('chain_detail_uncapped','  if(s.fury_detail>kChainDetail&&chain_detail(e)){','  if(false){'),
 ('no_critical_reserve','  if(critical(e)){if(crit<kCritGlobal','  if(false){if(crit<kCritGlobal'),
 ('command_over_pending_hit','&&!a.hit&&!a.cutdown&&!a.event','&&!a.cutdown&&!a.event'),
 ('generation_inherits','if(stale(s,id,gen)){s={};s.id=id;s.gen=gen;}','if(stale(s,id,gen)){const bool h=s.hunt;s={};s.id=id;s.gen=gen;s.hunt=h;}')]
base=(TASK/POL).read_text();results=[]
for name,a,b in M:
 assert base.count(a)==1,(name,base.count(a))
 d=WORK/name;shutil.rmtree(d,ignore_errors=True);(d/'tests').mkdir(parents=True)
 for rel,text in [(POL,base.replace(a,b))]+[(r,(TASK/r).read_text()) for r in DEPS]:(d/rel).parent.mkdir(parents=True,exist_ok=True);(d/rel).write_text(text)
 shutil.copy(TASK/'tests/cp18_lab.cpp',d/'tests/cp18_lab.cpp')
 c=subprocess.run(['g++','-std=c++20','-O1','-w',str(d/'tests/cp18_lab.cpp'),'-o',str(d/'lab')],capture_output=True,text=True)
 if c.returncode:results.append((name,'COMPILE_ERROR',c.stderr[-300:]));continue
 r=subprocess.run([str(d/'lab')],capture_output=True,text=True,timeout=120)
 results.append((name,'KILLED' if r.returncode else 'SURVIVED',(r.stderr.strip().splitlines() or [''])[-1]))
for n,s,w in results:print(f'MUTANT {n} {s} {w}')
killed=sum(s=='KILLED' for _,s,_ in results);print(f'CP18_MUTATIONS killed={killed} total={len(results)}')
sys.exit(0 if killed==len(results) else 1)
