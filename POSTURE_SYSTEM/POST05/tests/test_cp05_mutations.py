"""Executed behavior mutants of production CP05. Compilation errors/timeouts
never count as kills; both unmodified controls must compile and pass first.
"""
from pathlib import Path
import subprocess,tempfile,shutil,json,sys,os,concurrent.futures
root=Path(__file__).resolve().parents[1]
cases=[
 ('red_three_quarters','src/cp05_combo_core.hpp','word()>>63','word()>>62','core'),
 ('black_biased_mod2','src/cp05_combo_core.hpp','word%3','word%2','core'),
 ('black_rejection_missing','src/cp05_combo_core.hpp','if(word==~std::uint64_t(0))return false;','if(false)return false;','core'),
 ('black_eight_to_seven','src/cp05_combo_core.hpp','k==Kind::Black8?8','k==Kind::Black8?7','core'),
 ('black_seven_to_eight','src/cp05_combo_core.hpp','k==Kind::Black7?7','k==Kind::Black7?8','core'),
 ('red_four_to_three','src/cp05_combo_core.hpp','k==Kind::Red4?4','k==Kind::Red4?3','core'),
 ('red_equal_intervals','src/cp05_combo_core.hpp','r[]={68,46,72}','r[]={46,46,46}','core'),
 ('black8_loses_isolated_finish','src/cp05_combo_core.hpp','46,46,46,46,70','46,46,46,46,46','core'),
 ('black7_loses_initial_pair','src/cp05_combo_core.hpp','b7[]={46,68,70,56,56,56}','b7[]={68,68,70,56,56,56}','core'),
 ('active_combo_rerolled','src/cp05_combo_core.hpp','||active()||last_decision','||false||last_decision','core'),
 ('duplicate_execution_rerolled','src/cp05_combo_core.hpp','||last_decision==stamp){','||false){','core'),
 ('tokens_reused','src/cp05_combo_core.hpp','++swing;++token;','++swing;','core'),
 ('special_rate_accepted','src/cp05_combo_core.hpp','||handoff!=1.15f','||false','core'),
 ('duplicate_parry_counter','src/cp05_combo_core.hpp','if(contact_seen)return;contact_seen=true;','if(false)return;contact_seen=true;','core'),
 ('posture100_cancel_lost','src/cp05_combo_core.hpp','if(!active())return;if(phase','if(why==Cancel::Posture100)return;if(!active())return;if(phase','core'),
 ('black_normal_misread_as_fury','src/cp05_combo_core.hpp','kFastRate=1.15f','kFastRate=1.5f','adapter'),
 ('red_too_slow_for_shield_motion','src/cp05_combo_core.hpp','kSlowRate=1.f','kSlowRate=.5f','adapter'),
 ('deadline_uses_wrong_at_phase','src/cp05_combo_core.hpp','const auto buffer=at_delay(kind,swing+1);','const auto buffer=1u;','adapter'),
 ('context_reuses_old_stream','src/cp05_combo_core.hpp','const auto context=context_serial+1?context_serial+1:1;','const auto context=1;','adapter'),
 ('native_rate_write_removed','src/cp05_adapter.inc','a->mpMorf->setPlaySpeed(s.rate_value);','(void)s.rate_value;','adapter'),
 ('native_timer_write_removed','src/cp05_adapter.inc','a->field_0x6c2=static_cast<s16>(value);','(void)value;','adapter'),
 ('foreign_timer_overwritten','src/cp05_adapter.inc','||a->field_0x6c2!=expected','||false','adapter'),
 ('restore_after_expansion','src/cp05_adapter.inc','early.priority=100','early.priority=-300','adapter'),
 ('normal_handoff_bypassed','src/cp05_adapter.inc','if(a->mpMorf->getPlaySpeed()!=1.15f){','if(false){','adapter'),
 ('native_fresh_provenance_removed','src/cp05_adapter.inc','s->native_fresh=!s->processed&&s->start_pending&&s->attack_entered','s->native_fresh=!s->processed&&true&&true','adapter'),
 ('fury_transition_ignored','src/cp05_adapter.inc','s.fury=profile.variant==postbal01::Variant::BlackFury;','s.fury=false;','adapter'),
 ('full_clip_truncated_at22','src/cp05_adapter.inc','a->mpMorf&&a->mpMorf->isStop();','a->mpMorf&&a->mpMorf->getFrame()>=22;','adapter'),
 ('posture100_future_not_cancelled','src/cp05_adapter.inc','if(after>=100)cp05_cancel','if(false)cp05_cancel','adapter'),
 ('normal_choice_seized','src/cp05_adapter.inc','if(kind==cp05::Kind::Normal)return;','if(kind==cp05::Kind::Normal){s->kind=cp05::Kind::Red4;s->phase=cp05::Phase::Selected;}','adapter'),
 ('foreign_motion_jump_accepted','src/cp05_adapter.inc','||!motion||(!stopped','||false||(!stopped','adapter'),
 ('ghost_commit_telemetry','src/cp05_adapter.inc','s.contact(link,committed,v.blocked,c5.c);','s.contact(link,v.result==post08::Result::Confirmed,v.blocked,c5.c);','adapter'),
 ('other_contact_lost','src/cp05_adapter.inc','cp05_observe_other(a,*s);','(void)a;','adapter'),
 ('early_post_lease_restore_removed','src/cp05_adapter.inc','if(!cp05_native_timer(a,1,restore))','if(false)','adapter'),
 ('pending_continuation_enters_too_early','src/cp05_adapter.inc','s->local_tick+1>=s->launch_at','s->local_tick+20>=s->launch_at','adapter'),
]
for n,rel,old,new,lab in cases:assert old in (root/rel).read_text(),n
def core(p):
 exe=p/('cp05.exe' if os.name=='nt' else 'cp05');src=p/'tests/cp05_lab.cpp'
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(src),'/Fe'+str(exe),'/Fo'+str(p/'cp05.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(src),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=120)
 if c.returncode:return False,'COMPILE_ERROR'
 c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30);return c.returncode==0,c.stdout+c.stderr
with tempfile.TemporaryDirectory() as td:
 p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');ok,out=core(p);assert ok and 'RESULT=LAB_PASS' in out,'positive core must pass'
c=subprocess.run([sys.executable,'-I',str(root/'tests/test_cp05_adapter.py')],capture_output=True,text=True,timeout=210);assert c.returncode==0 and 'RESULT=LAB_PASS' in c.stdout,c.stdout+c.stderr
def one(case):
 n,rel,old,new,lab=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');f=p/rel;f.write_text(f.read_text().replace(old,new,1),encoding='utf-8')
  try:
   if lab=='core':ok,out=core(p);killed=not ok and 'CP05_FAIL line=' in out
   else:
    c=subprocess.run([sys.executable,'-I',str(p/'tests/test_cp05_adapter.py')],capture_output=True,text=True,timeout=210);out=c.stdout+c.stderr;killed=c.returncode!=0 and 'CP05_ADAPTER_FAIL' in out
   return {'mutation':n,'status':'KILLED' if killed else 'FAIL','timeout':False,'failure_tail':None if killed else out[-1200:]}
  except subprocess.TimeoutExpired:return {'mutation':n,'status':'FAIL','timeout':True}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:results=list(pool.map(one,cases))
print(json.dumps({'positive_controls':'PASS','count':len(results),'detected':sum(x['status']=='KILLED' for x in results),'timeouts':sum(x['timeout'] for x in results),'mutations':results},indent=2));sys.exit(any(x['status']!='KILLED' for x in results))
