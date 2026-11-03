"""CP02/03 behavior mutations. Positive controls required; compile/timeouts FAIL."""
from pathlib import Path
import tempfile,shutil,subprocess,sys,json,os,concurrent.futures
root=Path(__file__).resolve().parents[1]
cases=[
 ('old_setter_clamp5','src/posture_core.hpp','rate > policy::kRegenRateMax','rate > 5','core'),
 ('old_advance_clamp5','src/posture_core.hpp','e.regen_rate <= policy::kRegenRateMax','e.regen_rate <= 5','core'),
 ('old_delay120','src/posture_core.hpp','kRecoverDelayTicks = 60','kRecoverDelayTicks = 120','core'),
 ('early_regen','src/posture_core.hpp','e.k <= policy::kRecoverDelayTicks','e.k < policy::kRecoverDelayTicks','core'),
 ('top_rate5','src/break_core.hpp','p>75LL*max?10','p>75LL*max?5','core'),
 ('second_rate4','src/break_core.hpp','p>50LL*max?7','p>50LL*max?4','core'),
 ('third_rate3','src/break_core.hpp','p>25LL*max?4','p>25LL*max?3','core'),
 ('boundary75_inclusive','src/break_core.hpp','p>75LL*max','p>=75LL*max','core'),
 ('boundary50_inclusive','src/break_core.hpp','p>50LL*max','p>=50LL*max','core'),
 ('boundary25_inclusive','src/break_core.hpp','p>25LL*max','p>=25LL*max','core'),
 ('boundary10_inclusive','src/break_core.hpp','p>10LL*max','p>=10LL*max','core'),
 ('fraction_lost_on_rate_change','src/posture_core.hpp','    e.regen_rate = rate;','    e.regen_rate = rate; e.regen_acc=0;','core'),
 ('old_parry_frame9','src/parry_core.hpp','kWindowFrames = 7.0f','kWindowFrames = 9.0f','core'),
 ('old_parry_age11','src/parry_core.hpp','kWindowTicksCap = 9','kWindowTicksCap = 11','core'),
 ('parry_negative_slack_lost','src/parry_core.hpp','kPreStartSlack = 1.0f','kPreStartSlack = 0.0f','core'),
 ('parry_fraction_not_reset','src/posture_core.hpp','    e.regen_acc = 0;\n    e.last_hit_tick = now_;','    e.regen_acc = 1;\n    e.last_hit_tick = now_;','core'),
 ('parry_reward_changed','src/balance_core.hpp','{{25,40,50}','{{25,48,50}','core'),
 ('decisive_x2','src/break_core.hpp','a.extra=(base+1)/2','a.extra=base','core'),
 ('Link5to8_applied','src/lnk_core.hpp','kParryCostPoints = 0','kParryCostPoints = 8','core'),
 ('production_no_pause_gate','src/mod.cpp','g.pe.on_update(lg.sim_active)','g.pe.on_update()','adapter'),
 ('production_rate_update_lost','src/mod.cpp','g.pe.px.set_regen_rate(v.slot, v.generation, v.id, balance_new_rate);','(void)balance_new_rate;','adapter'),
 ('stale_max_generation','src/break_core.hpp','s.generation!=v.generation||s.id!=v.id','false','core'),
]
for name,fn,old,new,lab in cases:assert old in (root/fn).read_text(encoding='utf-8'),name
# A positive clone is compiled/run before any mutation is credited.
def core(p):
 exe=p/('core.exe' if os.name=='nt' else 'core');cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(p/'tests/cp0203_lab.cpp'),'/Fe'+str(exe),'/Fo'+str(p/'core.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(p/'tests/cp0203_lab.cpp'),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
 if c.returncode:return False,'COMPILE_ERROR'
 c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60);return c.returncode==0,c.stdout+c.stderr
with tempfile.TemporaryDirectory() as td:
 p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');ok,out=core(p);assert ok and 'LAB_PASS' in out,'positive core must pass'
c=subprocess.run([sys.executable,'-I',str(root/'tests/test_cp0203_adapter.py')],capture_output=True,text=True,timeout=180);assert c.returncode==0 and 'LAB_PASS' in c.stdout,'positive adapter must pass'
def one(case):
 name,fn,old,new,lab=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');f=p/fn;f.write_text(f.read_text(encoding='utf-8').replace(old,new,1),encoding='utf-8')
  try:
   if lab=='core':ok,out=core(p);killed=not ok and 'CP0203_FAIL' in out
   else:
    c=subprocess.run([sys.executable,'-I',str(p/'tests/test_cp0203_adapter.py')],capture_output=True,text=True,timeout=180);out=c.stdout+c.stderr;killed=c.returncode!=0 and 'CP0203_ADAPTER_FAIL' in out
   return {'mutation':name,'status':'KILLED' if killed else 'FAIL','timeout':False}
  except subprocess.TimeoutExpired:return {'mutation':name,'status':'FAIL','timeout':True}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as e:r=list(e.map(one,cases))
print(json.dumps({'positive_controls':'PASS','count':len(r),'detected':sum(x['status']=='KILLED' for x in r),'timeouts':sum(x['timeout'] for x in r),'mutations':r},indent=2));sys.exit(any(x['status']!='KILLED' for x in r))
