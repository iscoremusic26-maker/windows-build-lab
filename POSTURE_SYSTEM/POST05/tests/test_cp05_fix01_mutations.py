"""Executed FIX01 behavior mutants. Compile errors and timeouts are FAIL.
Controls exercise the actual current core, callbacks, native transaction and
native geometric predicates before any mutant is credited as detected.
"""
from pathlib import Path
import subprocess,tempfile,shutil,json,sys,os,concurrent.futures
root=Path(__file__).resolve().parents[1]
cases=[
 ('premature_raw_Tg_veto','src/cp05_adapter.inc','&&!a->mHide;','&&!a->mHide&&!a->mSphs_cc[0].ChkTgHit();','native',1),
 ('HP_loss_guards_removed','src/cp05_adapter.inc','a->health<s->execute_hp||cp05_weighted_native_hit(*s)','false||cp05_weighted_native_hit(*s)','native',2),
 ('HP_snapshot_lost','src/cp05_adapter.inc','s->execute_hp=a->health;','s->execute_hp=0;','native',1),
 ('weighted_report_generation_ignored','src/cp05_adapter.inc','h.generation==s.key.generation','true','native',1),
 ('weighted_report_freshness_ignored','src/cp05_adapter.inc','h.event!=s.execute_event','true','native',1),
 ('uncommitted_zero_weight_report_is_priority','src/cp05_adapter.inc','h.applied&&h.weight>0','true','native',1),
 ('ordinary_Find_recovery_rejected','src/cp05_adapter.inc','a->mOcState<0||a->mOcState>4','a->mOcState!=0','native',1),
 ('new_native_mode_timer_clobbered','src/cp05_adapter.inc','if(a->mActionMode!=3||a->mOcState!=4){++stats.lease_native_mode;','if(false){++stats.lease_native_mode;','native',1),
 ('midcall_context_acquires_without_PRE','src/cp05_adapter.inc','if(s->last_execute!=g.p8.tick)return HOOK_CONTINUE;','if(s->last_execute!=g.p8.tick)(void)s->execute(g.p8.tick,c5.c);','native',1),
 ('retained_native_reaction_misclassified','src/cp05_adapter.inc','if(!s->will_finish&&a->mActionMode!=4)','if(false)','native',1),
 ('all_swings_flat_fast_rate','src/cp05_combo_core.hpp','?kFastRate:kSlowRate','?kFastRate:kFastRate','core',1),
 ('normal_sampling_cap_exceeded','src/cp05_combo_core.hpp','kFastRate=1.15f','kFastRate=1.45f','geometry',1),
 ('six_second_Red_gap_restored','src/cp05_combo_core.hpp','r[]={68,46,72}','r[]={180,46,72}','core',1),
 ('four_second_Black_gap_restored','src/cp05_combo_core.hpp','b8[]={65,67,46,46,46,46,70}','b8[]={134,67,46,46,46,46,70}','core',1),
 ('Black7_closing_loses_its_contrast','src/cp05_combo_core.hpp','b7[]={46,68,70,56,56,56}','b7[]={46,68,70,74,74,74}','core',1),
 ('critical_case_example_reserve_lost','src/cp05_adapter.inc','const bool reserve=cls==0&&s&&critical<32','const bool reserve=false&&s&&critical<32','native',1),
 ('complete_summary_hidden_by_detail_cap','src/cp05_adapter.inc','void cp05_report(const char* reason)noexcept{','void cp05_report(const char* reason)noexcept{if(c5.lines>=800)return;','adapter',1),
 ('native_no_guard_is_ignored','tests/reference/cp05_native_shield.inc','if (i_atObjInf->ChkAtNoGuard())','if (false)','geometry',1),
 ('native_far_overlap_is_fabricated','tests/reference/cp05_native_cyl_sph.inc','if (radius_sum < dist)','if (false)','geometry',1),
 ('native_weapon_radius_inflated','tests/reference/cp05_native_cc_set.inc','mSphs_at[0].SetR(45.0f);','mSphs_at[0].SetR(450.0f);','geometry',1),
 ('interval_histogram_lies','src/cp05_combo_core.hpp','x<32?0:x<40?1','x<40?0:x<40?1','core',1),
 ('foreign_PRE_timer_is_borrowed','src/cp05_adapter.inc','if(s->gap_timer_known&&a->field_0x6c2!=s->gap_timer_expected)','if(false)','native',1),
 ('cancelled_execution_rerolls','src/cp05_adapter.inc','if(s->cancelled_this_execute){','if(false){','native',1),
]
for name,rel,old,new,lab,count in cases:assert (root/rel).read_text().count(old)==count,(name,count)
def run(p,lab):
 if lab=='core':
  exe=p/('core.exe' if os.name=='nt' else 'core');src=p/'tests/cp05_lab.cpp'
  cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(src),'/Fe'+str(exe),'/Fo'+str(p/'core.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(src),'-o',str(exe)]
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=120)
  if c.returncode:return False,'COMPILE_ERROR '+c.stdout+c.stderr
  c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
 else:
  script={'native':'test_cp05_fix01_native.py','geometry':'test_cp05_fix01_geometry.py','adapter':'test_cp05_adapter.py'}[lab]
  c=subprocess.run([sys.executable,'-I',str(p/'tests'/script)],capture_output=True,text=True,timeout=300)
 return c.returncode==0,c.stdout+c.stderr
with tempfile.TemporaryDirectory() as td:
 p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests')
 for lab in ['core','native','geometry','adapter']:
  ok,out=run(p,lab);assert ok and 'RESULT=LAB_PASS' in out,(lab,out[-1800:])
def one(case):
 name,rel,old,new,lab,count=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');f=p/rel;f.write_text(f.read_text().replace(old,new,count),encoding='utf-8')
  try:
   ok,out=run(p,lab);killed=not ok and ('CP05_FAIL line=' in out or 'CP05_ADAPTER_FAIL ' in out)
   return {'mutation':name,'detected':killed,'status':'KILLED' if killed else 'FAIL','timeout':False,'failure_tail':None if killed else out[-1800:]}
  except subprocess.TimeoutExpired:return {'mutation':name,'detected':False,'status':'FAIL','timeout':True}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:result=list(pool.map(one,cases))
print(json.dumps({'positive_controls':'CORE_NATIVE_TRANSACTION_GEOMETRY_ADAPTER_PASS','count':len(result),'detected':sum(x['detected'] for x in result),'timeouts':sum(x['timeout'] for x in result),'mutations':result},indent=2));sys.exit(not all(x['detected'] for x in result))
