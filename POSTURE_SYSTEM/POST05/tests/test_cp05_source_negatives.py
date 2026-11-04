"""Review/write barriers, separate from executed behavior mutation kills.
Positive full clone first; each negative must report its named CP05 cause.
"""
from pathlib import Path
import tempfile,subprocess,shutil,sys,json
root=Path(__file__).resolve().parents[1]
def clone(p):
 for name in ['src','ci','tests']:shutil.copytree(root/name,p/name,ignore=shutil.ignore_patterns('__pycache__'))
def run(p):
 c=subprocess.run([sys.executable,'-I',str(p/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=60)
 return c,json.loads(c.stdout)
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);c,d=run(p);assert c.returncode==0 and d['status']=='SOURCE_AUDIT_PASS',(c.stdout,c.stderr)
cases=[
 ('native_HP_write','src/cp05_adapter.inc','a->mpMorf->setPlaySpeed(s.rate_value);','a->health = 0;a->mpMorf->setPlaySpeed(s.rate_value);','CP05 forbidden native operation'),
 ('native_position_write','src/cp05_adapter.inc','a->mpMorf->setPlaySpeed(s.rate_value);','a->current.pos.z=0;a->mpMorf->setPlaySpeed(s.rate_value);','CP05 game-pointer assignment'),
 ('native_hitbox_write','src/cp05_adapter.inc','a->mpMorf->setPlaySpeed(s.rate_value);','a->mSphs_at[0].OnAtSetBit();a->mpMorf->setPlaySpeed(s.rate_value);','CP05 forbidden native operation'),
 ('new_Link_input_write','src/cp05_adapter.inc','#undef CP05_NOINLINE','void wrong(daAlink_c* link){link->mItemTrigger=0;}\n#undef CP05_NOINLINE','CP05 forbidden native operation'),
 ('direct_mode_write','src/cp05_adapter.inc','return cp04_native_mode(a,mode);','a->setActionMode(mode,0);return true;','CP05 forbidden native operation'),
 ('C5_mode_readback_removed','src/cp05_adapter.inc','return cp04_native_mode(a,mode);','return true;','CP05 exact native writer inventory'),
 ('Fury_parry_changed','src/parry_core.hpp','kFuryRateThreshold = 1.5f','kFuryRateThreshold = 9.f','CP05 reviewed/frozen source changed: src/parry_core.hpp'),
 ('Blue_rate_changed','src/cp04_combo_core.hpp','kFastRate=1.5f','kFastRate=2.f','CP05 reviewed/frozen source changed: src/cp04_combo_core.hpp'),
 ('HUD_root_changed','src/mod.cpp','ctx.owned[i]=ctx.mask[i]=false;','ctx.owned[i]=ctx.mask[i]=true;','CP05 reviewed/frozen source changed: src/mod.cpp'),
 ('balance_changed','src/balance_core.hpp','{{25,40,50}','{{25,20,50}','CP05 reviewed/frozen source changed: src/balance_core.hpp'),
 ('native_Find_oracle_changed','tests/reference/cp05_native_find.inc','field_0x6c2 == 0','field_0x6c2 < 0','CP05 reviewed/frozen source changed: tests/reference/cp05_native_find.inc'),
 ('Expansion_Random_odds_changed','tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/red_ai_policy.hpp','kRandomPercent=12','kRandomPercent=20','CP05 reviewed/frozen source changed: tests/cp21'),
 ('Expansion_Fury_duration_changed','tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp','kFuryTicks=300','kFuryTicks=30','CP05 reviewed/frozen source changed: tests/cp21'),
 ('C5_receipt_guard_removed','src/cp05_adapter.inc','!s.native_fresh||','false||','CP05 reviewed/frozen source changed: src/cp05_adapter.inc'),
 ('committed_contact_integration_omitted','src/mod.cpp','  cp05_contact(in,v,ok,pb,pa);','  (void)ok;','CP05 exact integration delta changed: mod.cpp'),
]
results=[]
for name,rel,old,new,expected in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/rel;s=f.read_text(encoding='utf-8');assert old in s,name;f.write_text(s.replace(old,new,1),encoding='utf-8');c,d=run(p)
  assert c.returncode==1 and d['status']=='SOURCE_AUDIT_FAIL' and any(expected in e for e in d['errors']),(name,c.stdout,c.stderr)
  results.append({'case':name,'rejected':True,'expected_cause':expected})
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);f=p/'ci/cp05_policy.json';d=json.loads(f.read_text());d['baseline_pins']={};f.write_text(json.dumps(d));c,d=run(p);assert c.returncode==1 and any('must contain32 exact base files' in x for x in d['errors']);results.append({'case':'frozen_inventory_emptied','rejected':True,'expected_cause':'must contain32 exact base files'})
print(json.dumps({'status':'NEGATIVES_PASS','positive_clone':'SOURCE_AUDIT_PASS','count':len(results),'cases':results,'timeouts':0},indent=2))
