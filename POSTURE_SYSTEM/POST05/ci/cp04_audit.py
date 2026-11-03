"""CP04 authorized deltas, narrow inverse patch for legacy barriers and
new writer inventory. The original wall is retained, never repinned wholesale.
Exact hashes supplement behavior tests; neither is a runtime certification.
"""
from pathlib import Path
import json,hashlib,re,sys
HERE=Path(__file__).resolve().parent
def policy():return json.loads((HERE/'cp04_policy.json').read_text())
def legacy_source(name,text):
 for edit in policy()['inverse_patch'].get(name,[]):
  # A changed insertion is NOT silently erased: the legacy gate then fails.
  if text.count(edit['after'])==1:text=text.replace(edit['after'],edit['before'],1)
 return text
def audit(project):
 p=Path(project);pin=policy();errors=[]
 for rel,want in pin['files'].items():
  f=p/rel
  if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest()!=want:errors.append('CP04 reviewed source hash changed: '+rel)
 for name,edits in pin['inverse_patch'].items():
  s=(p/'src'/name).read_text(encoding='utf-8')
  for edit in edits:
   if s.count(edit['after'])!=1:errors.append('CP04 integration delta changed: '+name)
 a=(p/'src/cp04_adapter.inc').read_text(encoding='utf-8')
 for token in ('HOOK_SKIP_ORIGINAL','mProcID =','health=','health =','setFrame(','setBck(','mAttackPower','VirtualProtect','GetTgHitAc','get_service','mod_ctx->'):
  if token in a:errors.append('CP04 forbidden native operation: '+token)
 if a.count('a->mpMorf->setPlaySpeed(s.rate);')!=1 or a.count('a->setActionMode(mode,0);')!=1 or a.count('a->field_0x6c2=static_cast<s16>(ticks);')!=1 or a.count('(void)link->procGuardAttackInit();')!=1:errors.append('CP04 native writer inventory is not four scoped functions')
 if 'late.priority=-100' not in a or 'add_post<LinkStickFx>(on_stick_post_cp04,&late)' not in a:errors.append('CP04 documented hook priority missing')
 # No retained actor pointer: sidecars are in the standard-only cores. Native
 # pointers are callback locals; actual actor identity uses ledger generation.
 for rel in ('src/cp04_combo_core.hpp','src/shield_chain_core.hpp'):
  s=(p/rel).read_text(encoding='utf-8')
  if 'uintptr_t' in s or 'void*' in s or 'daE_OC' in s or 'daAlink' in s:errors.append('CP04 sidecar retains native addresses/types: '+rel)
 return errors
if __name__=='__main__':
 errors=audit(Path(sys.argv[1]));print(json.dumps({'status':'SOURCE_AUDIT_FAIL' if errors else 'SOURCE_AUDIT_PASS','compiled_base':policy()['compiled_base'],'legacy_wall':'exact inverse patch + original0.11.2 and CP0203 approved pins','new_native_writer_functions':['cp04_native_rate','cp04_native_mode','cp04_native_gap','cp04_native_shield_restart'],'errors':errors,'native_runtime':False},indent=2));sys.exit(bool(errors))
