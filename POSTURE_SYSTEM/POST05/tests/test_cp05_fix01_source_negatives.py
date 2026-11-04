"""Additive source/write/reference negatives with a complete positive clone."""
from pathlib import Path
import tempfile,subprocess,shutil,json,sys
root=Path(__file__).resolve().parents[1]
def clone(p):
 for n in ['src','tests','ci']:shutil.copytree(root/n,p/n,ignore=shutil.ignore_patterns('__pycache__'))
def audit(p):
 c=subprocess.run([sys.executable,'-I',str(p/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=60)
 return c,json.loads(c.stdout)
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);c,d=audit(p);assert c.returncode==0 and d['status']=='SOURCE_AUDIT_PASS',(c.stdout,c.stderr)
cases=[
 ('RNG_changed','src/cp05_combo_core.hpp','rng+=0x9e3779b97f4a7c15ULL','rng+=1ULL','CP05 FIX01 frozen RNG changed'),
 ('old_RNG_oracle_tampered','tests/reference/cp05_0_11_8/cp05_combo_core.hpp','word%3','word%2','CP05 FIX01 reviewed reference/source changed'),
 ('native_damage_oracle_tampered','tests/reference/cp05_native_damage.inc','if (field_0x6cc)','if (false)','CP05 FIX01 reviewed reference/source changed'),
 ('native_geometry_oracle_tampered','tests/reference/cp05_native_cc_set.inc','mSphs_at[0].SetR(45.0f);','mSphs_at[0].SetR(80.0f);','CP05 FIX01 reviewed reference/source changed'),
 ('Link_procedure_write','src/cp05_adapter.inc','#undef CP05_NOINLINE','void wrong(daAlink_c* link){link->mProcID=0;}\n#undef CP05_NOINLINE','CP05 forbidden native operation: mProcID'),
 ('pose_reader_moves_actor','src/cp05_adapter.inc','v.centre[n][0]=c.x;','a->current.pos.x=0;v.centre[n][0]=c.x;','CP05 game-pointer assignment outside'),
 ('raw_Tg_premature_veto','src/cp05_adapter.inc','&&!a->mHide;','&&!a->mHide&&!a->mSphs_cc[0].ChkTgHit();','CP05 FIX01 premature raw Tg veto restored'),
 ('midcall_PRE_receipt_removed','src/cp05_adapter.inc','if(s->last_execute!=g.p8.tick)return HOOK_CONTINUE;','if(false)return HOOK_CONTINUE;','CP05 FIX01 matching EOC PRE receipt missing'),
 ('critical_reserve_removed','src/cp05_adapter.inc','const bool reserve=cls==0','const bool reserve=false','CP05 FIX01 critical example reserve missing'),
 ('sampling_cap_changed','src/cp05_combo_core.hpp','kFastRate=1.15f','kFastRate=1.45f','CP05 FIX01 native normal sampling cap changed'),
]
results=[]
for name,rel,old,new,expected in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/rel;s=f.read_text(encoding='utf-8');assert old in s,name;f.write_text(s.replace(old,new,1),encoding='utf-8');c,d=audit(p)
  assert c.returncode==1 and d['status']=='SOURCE_AUDIT_FAIL' and any(expected in e for e in d['errors']),(name,c.stdout,c.stderr)
  results.append({'case':name,'rejected':True,'expected_cause':expected})
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);f=p/'ci/cp05_fix01_policy.json';d=json.loads(f.read_text());d['files']={};f.write_text(json.dumps(d));c,d=audit(p);assert c.returncode==1 and any('inventory must contain9' in e for e in d['errors']);results.append({'case':'reference_inventory_emptied','rejected':True,'expected_cause':'inventory must contain9'})
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);(p/'tests/reference/cp05_native_damage.inc').unlink();c,d=audit(p);assert c.returncode==1 and any('reviewed reference/source changed' in e for e in d['errors']);results.append({'case':'native_reference_missing','rejected':True,'expected_cause':'reviewed reference/source changed'})
print(json.dumps({'status':'NEGATIVES_PASS','positive_clone':'SOURCE_AUDIT_PASS','count':len(results),'cases':results,'timeouts':0},indent=2))
