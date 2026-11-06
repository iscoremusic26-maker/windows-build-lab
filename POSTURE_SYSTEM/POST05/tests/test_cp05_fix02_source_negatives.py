"""Complete positive clone followed by targeted source/write/reference negatives."""
from pathlib import Path
import shutil,tempfile,subprocess,sys,json
root=Path(__file__).resolve().parents[1]
def clone(p):
 for x in ['src','tests','ci']:shutil.copytree(root/x,p/x,ignore=shutil.ignore_patterns('__pycache__'))
def audit(p):
 r=subprocess.run([sys.executable,'-I',str(p/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=60);return r,json.loads(r.stdout)
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);r,d=audit(p);assert r.returncode==0 and d['status']=='SOURCE_AUDIT_PASS',(r.stdout,r.stderr)
cases=[
 ('new_native_HP_writer','src/cp05_fix02_adapter.inc','struct Cp05Fix02Runtime {','void bad(daE_OC_c* a){a->health=220;}\nstruct Cp05Fix02Runtime {','native pointer assignment forbidden'),
 ('new_Link_proc_writer','src/cp05_fix02_adapter.inc','struct Cp05Fix02Runtime {','void bad(daAlink_c* a){a->mProcID=0;}\nstruct Cp05Fix02Runtime {','native pointer assignment forbidden'),
 ('new_forced_attack','src/cp05_fix02_adapter.inc','struct Cp05Fix02Runtime {','void bad(daE_OC_c* a){a->setActionMode(4,0);}\nstruct Cp05Fix02Runtime {','no new native writer allowed'),
 ('new_radius_writer','src/cp05_fix02_adapter.inc','struct Cp05Fix02Runtime {','void bad(daE_OC_c* a){a->mSphs_at[0].SetR(450);}\\nstruct Cp05Fix02Runtime {','no new native writer allowed'),
 ('synthetic_AT_writer','src/cp05_fix02_adapter.inc','struct Cp05Fix02Runtime {','void bad(daE_OC_c* a){a->mSphs_at[0].OnAtSetBit();}\\nstruct Cp05Fix02Runtime {','no new native writer allowed'),
 ('new_rate_writer','src/cp05_fix02_adapter.inc','struct Cp05Fix02Runtime {','void bad(daE_OC_c* a){a->mpMorf->setPlaySpeed(2);}\nstruct Cp05Fix02Runtime {','no new native writer allowed'),
 ('unconditional_terminal_safety','src/mod.cpp','if(fix02_terminal)g.cur.continuation=lnks1fix::Continuation::Safe;','g.cur.continuation=lnks1fix::Continuation::Safe;','exact integration changed'),
 ('legacy_parry_receipt_corrupted','src/mod.cpp','if(ok)fix01_cleanup_request();','if(true)fix01_cleanup_request();','old production changed'),
 ('guard_cleanup_binding_removed','src/mod.cpp','  fix02_guard_contact(a);','  (void)0;','exact integration changed'),
 ('terminal_live_readback_removed','src/cp05_fix02_adapter.inc','a->mpMorf->getFrame()!=r.aux.anim_frame','false','production proof/binding missing'),
 ('terminal_Link_origin_removed','src/cp05_fix02_adapter.inc','!cp05fix02::ordinary_link_sword(rep,g.cur.from_link)','false','production proof/binding missing'),
 ('monitor_adds_engine_mutation','src/cp05_fix02_adapter.inc','void fix02_maintenance()noexcept{','void fix02_maintenance()noexcept{setActionMode(4,0);','no new native writer allowed'),
 ('native_register_oracle_tampered','tests/reference/fix02_native_cc_register.inc','if (obj->ChkAtSet())','if (true)','reviewed new source/reference changed'),
 ('native_clear_oracle_tampered','tests/reference/fix02_native_cc_clear.inc','(*obj)->GetGObjInf()->ClrAtHit();','(void)obj;','reviewed new source/reference changed'),
 ('combo_rate_changed','src/cp05_combo_core.hpp','kBlackMaxRate=1.30f','kBlackMaxRate=1.45f','old production changed'),
 ('Blue_shieldchain_changed','src/shield_chain_core.hpp','!trigger)return','false)return','old production changed'),
]
rows=[]
for name,rel,a,b,want in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/rel;s=f.read_text(encoding='utf-8');assert s.count(a)==1,name;f.write_text(s.replace(a,b,1),encoding='utf-8');r,d=audit(p);assert r.returncode==1 and any(want in e for e in d['errors']),(name,r.stdout,r.stderr);rows.append({'case':name,'rejected':True,'expected_cause':want})
for field,want in [('files','exact six-file reference/module inventory required'),('baseline_files','frozen production inventory must contain34')]:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/'ci/cp05_fix02_policy.json';d=json.loads(f.read_text(encoding='utf-8'));d[field]={};f.write_text(json.dumps(d));r,d=audit(p);assert r.returncode==1 and any(want in e for e in d['errors']);rows.append({'case':field+'_inventory_removed','rejected':True,'expected_cause':want})
print(json.dumps({'status':'NEGATIVES_PASS','positive_clone':'SOURCE_AUDIT_PASS','count':len(rows),'timeouts':0,'cases':rows},indent=2))
