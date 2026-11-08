"""CP06 FIX01 0.11.12 executed behaviour mutants against the value core and the PRODUCTION
adapter (world lab). Compile errors and timeouts are FAIL (never credited). Positive controls
run both unmodified labs first."""
from pathlib import Path
import subprocess,tempfile,shutil,json,sys,os,concurrent.futures
root=Path(__file__).resolve().parents[1]
C='src/cp06f1_core.hpp';A='src/cp06f1_adapter.inc'
cases=[
 # geometry / engine predicate
 ('cross_ignores_write_slack',C,'const float rs=k.r+s.r-slack;','const float rs=k.r+s.r;','core'),
 ('cross_without_vertical_test',C,'return s.c.y+rv>=k.c.y&&s.c.y-rv<=k.c.y+k.h;','return true;','core'),
 ('touches_ignores_link_tg_off',C,'if((k[i].on||!require_on)&&cross(k[i],s,slack))','if(cross(k[i],s,slack))','core'),
 ('write_margin_zero',C,'kWriteSlack=0.5f','kWriteSlack=0.0f','core'),
 # vanilla pose coverage
 ('no_pre_plus_one_pose',C,'if(a==0){if(n<kMaxProbes&&(!n||out[n-1]<one))out[n++]=one;else if(n>=kMaxProbes)break;}','if(a==0){}','core'),
 ('no_window_start_poses',C,'float cur=prev_armed&&prev_covered?pre+0.5f:prev_armed?pre:kPoseFirst;','float cur=pre+0.5f;','core'),
 ('no_gap_fill',C,'while(at-cur>0.5f+1e-4f&&n<kMaxProbes){','while(false&&at-cur>0.5f){','core'),
 ('no_window_end_pose',C,'if(n<kMaxProbes&&post>kAtLast&&kPoseLast-post>0.5f&&(!n||out[n-1]<kPoseLast))out[n++]=kPoseLast;','','core'),
 ('window_end_test_inexact',C,'if(n<kMaxProbes&&post>kAtLast&&','if(n<kMaxProbes&&post>kAtLast+0.01f&&','core'),
 ('slow_rates_probed',C,'kMinFastRate=1.0001f','kMinFastRate=0.5f','core'),
 # vanilla root
 ('lunge_offset_sign',C,'const float d=lunge(clip_b,m-1.0f)-lunge(clip_b,pre);','const float d=lunge(clip_b,pre)-lunge(clip_b,m-1.0f);','core'),
 ('lunge_table_index_shift',C,'int k=int(pre-9.0f);','int k=int(pre-8.0f);','core'),
 ('facing_quadrant_wrong',C,'else if(q==1){sn=c;cs=-s;}','else if(q==1){sn=c;cs=s;}','core'),
 # decision
 ('already_touching_loose',C,"if(touches(k,cur[0],kWriteSlack,true)||touches(k,cur[1],kWriteSlack,true)){d.why=Why::AlreadyTouching;return d;}","if(touches(k,cur[0],kKeepSlack,true)||touches(k,cur[1],kKeepSlack,true)){d.why=Why::AlreadyTouching;return d;}",'core'),
 ('overwrites_a_touching_sphere',C,"if(touches(k,cur[0],kWriteSlack,true)||touches(k,cur[1],kWriteSlack,true)){d.why=Why::AlreadyTouching;return d;}",'','core'),
 ('last_candidate_wins',C,'for(int j=0;j<2;++j)for(unsigned i=0;i<n&&!d.write[j];++i){','for(int j=0;j<2;++j)for(unsigned i=0;i<n;++i){','core'),
 # swing classification
 ('lost_contact_reported_as_engine_rule',C,'if(s.crossed_before)++s.nocon_ticks;else ++s.lost_ticks;','++s.nocon_ticks;','core'),
 ('parry_without_guard_attack',C,'s.parry=pv.shield&&s.ga_pending;','s.parry=pv.shield;','core'),
 # Expansion flash mirror
 ('flash_colour_timer_start',C,'f.col=32-(kFlashTimer%16);','f.col=32;','core'),
 ('flash_colour_band',C,'t>32?kR0:t>16?kR1:kR2;','t>32?kR0:kR1;','core'),
 ('merge_lowers_native',C,'c.r=n.r>o.r?n.r:o.r;','c.r=o.r;','core'),
 ('parry_chip_counts_as_legit',C,'if(s==ChipSeen::ParryNeutralized){start(e_);++c.parry_starts;}','if(s==ChipSeen::ParryNeutralized){start(e_);start(l_);++c.parry_starts;}','core'),
 ('legit_chip_not_mirrored',C,'else if(s==ChipSeen::Legit){start(e_);start(l_);++c.legit_starts;}','else if(s==ChipSeen::Legit){start(e_);++c.legit_starts;}','core'),
 ('foreign_value_overwritten',C,'if(now!=expected){++c.mismatch;uncertain_=true;return Draw::Mismatch;}','','core'),
 ('uncertain_window_ignored',C,'if(uncertain_){++c.uncertain_draws;return Draw::Uncertain;}','','core'),
 ('uncertain_never_cleared',C,'if(!on(e_)&&!on(l_))uncertain_=false;','','core'),
 ('neutralized_seen_as_legit',C,'if(comp_after==comp_before+1&&after==before)return ChipSeen::ParryNeutralized;','if(comp_after==comp_before+1&&after==before)return ChipSeen::Legit;','core'),
 ('any_drop_is_a_legit_chip',C,'if(comp_after==comp_before&&after==before-1.0f)return ChipSeen::Legit;','if(comp_after==comp_before&&after<before)return ChipSeen::Legit;','core'),
 # production adapter (world lab)
 ('probe_frame_not_restored',A,' cp06f1_native_pose_frame(m,restore);\n',' m->modelCalc();\n','world'),
 ('probe_restore_unverified',A,' bool same=m->getFrame()==restore;\n',' bool same=true;return same;\n','world'),
 ('link_not_registered_ignored',A,' if(!t.link_current){t.why=Why::LinkNotCurrent;return;}\n','','world'),
 ('native_current_candidate_dropped',A,' Cand c[1+kMaxProbes];unsigned n=0;c[n].s[0]=tip(now);c[n].s[1]=hilt(now);++n;',' Cand c[1+kMaxProbes];unsigned n=0;','world'),
 ('probe_at_current_root',A,'c[n].s[0]=moved(tip(pm[i]),o);c[n].s[1]=moved(hilt(pm[i]),o);','c[n].s[0]=tip(pm[i]);c[n].s[1]=hilt(pm[i]);(void)o;','world'),
 ('written_radius_not_native',A,'cp06f1_native_sphere_place(a,j,d.place[j].c.x,d.place[j].c.y,d.place[j].c.z,d.place[j].r)','cp06f1_native_sphere_place(a,j,d.place[j].c.x,d.place[j].c.y,d.place[j].c.z,d.place[j].r-5.0f)','world'),
 ('no_probes',A,' const bool near=S.valid&&reachable(any,hand,S.hand);',' const bool near=false;','world'),
 ('coverage_before_expansion_sweep',A,'cov.priority=-300;','cov.priority=10;','world'),
 ('flash_capture_after_expansion',A,'dc.priority=90;','dc.priority=-10;','world'),
 ('flash_apply_before_expansion',A,'da.priority=-250;','da.priority=10;','world'),
 ('chip_observed_before_cp06',A,'ca.priority=-251;','ca.priority=-249;','world'),
 ('flash_mirror_not_ticked',A,' f1.flash.pass();   // Expansion','// f1.flash.pass();   // Expansion','world'),
 ('flash_applied_to_any_link',A,"if(!k||!m||k!=f1_link()||!f1_flash_model(k,m))return HOOK_CONTINUE;","if(!k||!m)return HOOK_CONTINUE;",'world'),
]
for name,rel,old,new,lab in cases:assert (root/rel).read_text(encoding='utf-8').count(old)==1,(name,(root/rel).read_text(encoding='utf-8').count(old))
def build(p,lab):
 src=p/('tests/cp06f1_lab.cpp' if lab=='core' else 'tests/cp06f1_world_lab.cpp');exe=p/(lab+('.exe' if os.name=='nt' else ''))
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/O2','/bigobj','/I'+str(p/'tests'),str(src),'/Fe'+str(exe),'/Fo'+str(p/(lab+'.obj'))] if os.name=='nt' else ['g++','-std=c++20','-O2','-I'+str(p/'tests'),str(src),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=600)
 return exe,c
def run(p,lab):
 exe,c=build(p,lab)
 if c.returncode:return False,'COMPILE_ERROR '+c.stdout[-3000:]+c.stderr[-3000:]
 c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=600)
 return c.returncode==0,c.stdout+c.stderr
def copy(p):
 shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests',ignore=shutil.ignore_patterns('__pycache__','bench'))
with tempfile.TemporaryDirectory() as td:
 p=Path(td);copy(p)
 for lab in['core','world']:
  ok,out=run(p,lab);assert ok and 'RESULT=LAB_PASS' in out,(lab,out[-2000:])
MARK=('CP06F1_FAIL line=','CP06F1_WORLD_FAIL line=')
def one(case):
 name,rel,old,new,lab=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);copy(p);f=p/rel;f.write_text(f.read_text(encoding='utf-8').replace(old,new,1),encoding='utf-8')
  try:
   ok,out=run(p,lab);killed=not ok and any(m in out for m in MARK)
   return {'mutation':name,'lab':lab,'detected':killed,'status':'KILLED' if killed else 'FAIL','timeout':False,'failure_tail':None if killed else out[-1500:]}
  except subprocess.TimeoutExpired:return {'mutation':name,'lab':lab,'detected':False,'status':'FAIL','timeout':True}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('CP06F1_MUTATION_WORKERS','4'))) as pool:result=list(pool.map(one,cases))
print(json.dumps({'positive_controls':'CP06F1_CORE_AND_WORLD_PASS','count':len(result),'detected':sum(x['detected'] for x in result),'timeouts':sum(x['timeout'] for x in result),'mutations':result},indent=2));sys.exit(not all(x['detected'] for x in result))
