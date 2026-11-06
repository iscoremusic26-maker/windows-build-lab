"""CP06 executed behaviour mutants. Compile errors and timeouts are FAIL (not credited).
Positive controls run the actual core lab and the production combat harness first."""
from pathlib import Path
import subprocess,tempfile,shutil,json,sys,os,concurrent.futures
root=Path(__file__).resolve().parents[1]
C='src/cp06_combat_core.hpp';A='src/cp06_combat_adapter.inc';P='src/parry_core.hpp'
cases=[
 ('fury_without_power6',C,'atp==kFuryAtp&&','','core',1),
 ('fury_any_rate',C,'anim_rate==kFuryRate&&','','core',1),
 ('fury_without_LNK07_class',C,'&&variant_fury_verified;',';','core',1),
 ('fury_without_E_OC2',C,'return e_oc2&&atp==kFuryAtp','return atp==kFuryAtp','core',1),
 ('chip_absorbs_double_damage',C,'now.pending!=x.before.pending-kChipUnits','now.pending>x.before.pending','combat',1),
 ('chip_ignores_life_change',C,'if(now.life!=x.before.life)return record(Chip::LifeChanged);','','combat',1),
 ('chip_ignores_type_change',C,'if(now.type!=x.before.type)return record(Chip::TypeChanged);','','combat',1),
 ('chip_without_engine_shield_pair',C,'!x.before.shield_pair||','','core',1),
 ('chip_without_capture',C,'if(!x.captured)return record(Chip::NotCaptured);','','core',1),
 ('chip_capture_any_tick',C,'||r.generation!=generation||r.tick!=tick)return false;','||r.generation!=generation)return false;','core',1),
 ('chip_repeat_same_token',C,'if(token<=last_token_[slot]&&','if(false&&','core',1),
 ('chip_cross_slot_duplicate',C,'if(n!=slot&&rows_[n].armed&&rows_[n].id==id','if(false&&rows_[n].armed&&rows_[n].id==id','core',1),
 ('chip_restores_two_units',C,'restore=kChipUnits;type=now.type;','restore=2*kChipUnits;type=now.type;','core',1),
 ('chip_link_invalid_accepted',C,'if(!now.link_ok||!x.before.link_ok)return record(Chip::LinkInvalid);','','core',1),
 ('late_reported_twice',C,'e.reported=true;const unsigned d','const unsigned d','core',1),
 ('late_horizon_unbounded',C,'tick-e.tick>kHorizon','tick-e.tick>kHorizon*100','core',1),
 ('legacy_fury_gate_disabled',P,'    if (in.fury_verified) return false;  // CP06','    if (true) return false;  // CP06','core',1),
 ('fury_gate_ignored',P,'    if (in.fury_verified) return false;  // CP06','    if (false) return false;  // CP06','combat',1),
 ('chip_window_armed_without_commit',A,'if(!c6.enabled||!ok||!in.fury_verified','if(!c6.enabled||!in.fury_verified','combat',1),
 ('chip_window_for_unverified',A,'if(!c6.enabled||!ok||!in.fury_verified','if(!c6.enabled||!ok','combat',1),
 ('capture_after_expansion',A,'before.priority=90;after.priority=-250;','before.priority=-1;after.priority=-250;','combat',1),
 ('restore_written_twice',A,' g_dComIfG_gameInfo.play.setItemLifeCount(units,type);\n',' g_dComIfG_gameInfo.play.setItemLifeCount(units,type);g_dComIfG_gameInfo.play.setItemLifeCount(units,type);\n','combat',1),
 ('fury_verified_ignores_rate',A,'r.aux.anim_ok!=0,r.aux.anim_rate,fury','r.aux.anim_ok!=0,2.f,fury','combat',1),
 ('late_contact_any_verdict',A,'if(v.result!=post08::Result::NormalGuard&&v.result!=post08::Result::NotBlocked)return;','','combat',1),
 ('Blue_opening_pair_unchanged','src/cp04_combo_core.hpp','kOpenRate=1.10f','kOpenRate=1.f','cp04',1),
 ('Blue_gap_back_to_twelve','src/cp04_combo_core.hpp','kModerateGap=8,','kModerateGap=12,','cp04',1),
 ('Red_pair_slowed','src/cp05_combo_core.hpp','r4[]={1.05f,1.35f,1.35f,1.05f}','r4[]={1.05f,1.15f,1.15f,1.05f}','core',1),
 ('Black7_closing_unaccelerated','src/cp05_combo_core.hpp','1.05f,1.05f,1.20f,1.20f,1.20f}','1.05f,1.05f,1.05f,1.05f,1.05f}','core',1),
 ('Black8_burst_target_old','src/cp05_combo_core.hpp','b8[]={62,62,38,38,38,38,64}','b8[]={62,62,46,46,46,46,64}','core',1),
]
for name,rel,old,new,lab,count in cases:assert (root/rel).read_text(encoding='utf-8').count(old)==count,(name,count)
def run(p,lab):
 if lab in('core','cp04'):
  src=p/('tests/cp06_lab.cpp' if lab=='core' else 'tests/cp04_lab.cpp');exe=p/(lab+('.exe' if os.name=='nt' else ''))
  cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(src),'/Fe'+str(exe),'/Fo'+str(p/(lab+'.obj'))] if os.name=='nt' else ['g++','-std=c++20','-O1',str(src),'-o',str(exe)]
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
  if c.returncode:return False,'COMPILE_ERROR '+c.stdout+c.stderr
  c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=120)
 else:c=subprocess.run([sys.executable,'-I',str(p/'tests/test_cp06_combat.py')],capture_output=True,text=True,timeout=900)
 return c.returncode==0,c.stdout+c.stderr
def copy(p):
 shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests',ignore=shutil.ignore_patterns('__pycache__'))
with tempfile.TemporaryDirectory() as td:
 p=Path(td);copy(p)
 for lab in['core','cp04','combat']:
  ok,out=run(p,lab);assert ok and 'RESULT=LAB_PASS' in out,(lab,out[-2000:])
MARK=('CP06_FAIL line=','CP05_ADAPTER_FAIL ','FAIL line')
def one(case):
 name,rel,old,new,lab,count=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);copy(p);f=p/rel;f.write_text(f.read_text(encoding='utf-8').replace(old,new,count),encoding='utf-8')
  try:
   ok,out=run(p,lab);killed=not ok and any(m in out for m in MARK)
   return {'mutation':name,'lab':lab,'detected':killed,'status':'KILLED' if killed else 'FAIL','timeout':False,'failure_tail':None if killed else out[-1500:]}
  except subprocess.TimeoutExpired:return {'mutation':name,'lab':lab,'detected':False,'status':'FAIL','timeout':True}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('CP06_MUTATION_WORKERS','4'))) as pool:result=list(pool.map(one,cases))
print(json.dumps({'positive_controls':'CP06_CORE_CP04_COMBAT_PASS','count':len(result),'detected':sum(x['detected'] for x in result),'timeouts':sum(x['timeout'] for x in result),'mutations':result},indent=2));sys.exit(not all(x['detected'] for x in result))
