"""Additive FIX02 barrier: exact original production bytes after narrow inserts.
No repinning old combat functions, headers, combo profiles or foreign sources.
"""
from pathlib import Path
import hashlib,json,re,sys
import cp06_audit
HERE=Path(__file__).resolve().parent
def policy():return json.loads((HERE/'cp05_fix02_policy.json').read_text(encoding='utf-8'))
def sha(b):return hashlib.sha256(b).hexdigest()
def legacy_source(name,text):
 text=cp06_audit.legacy_source(name,text)
 for e in reversed(policy()['inverse_patch'].get(name,[])):
  if text.count(e['after'])==e.get('count',1):text=text.replace(e['after'],e['before'],e.get('count',1))
 return text
def audit(project):
 p=Path(project);pin=policy();errors=[]
 expected={'src/cp05_fix02_core.hpp','src/cp05_fix02_monitor.hpp','src/cp05_fix02_adapter.inc','tests/reference/cp05_0_11_9/fix01_core.hpp','tests/reference/fix02_native_cc_register.inc','tests/reference/fix02_native_cc_clear.inc'}
 if set(pin['files'])!=expected:errors.append('FIX02 exact six-file reference/module inventory required')

 for rel,want in pin['baseline_files'].items():
  f=p/rel
  if not f.is_file():errors.append('FIX02 old production missing: '+rel);continue
  if sha(legacy_source(f.name,f.read_text(encoding='utf-8')).encode())!=want:errors.append('FIX02 old production changed outside exact authorized insertions: '+rel)
 for name,edits in pin['inverse_patch'].items():
  t=cp06_audit.legacy_source(name,(p/'src'/name).read_text(encoding='utf-8'))
  for e in reversed(edits):
   if t.count(e['after'])!=e.get('count',1):errors.append('FIX02 exact integration changed: '+name)
   else:t=t.replace(e['after'],e['before'],e.get('count',1))
 for rel,want in pin['files'].items():
  f=p/rel
  if not f.is_file() or sha(f.read_bytes())!=want:errors.append('FIX02 reviewed new source/reference changed: '+rel)
 adapter=(p/'src/cp05_fix02_adapter.inc').read_text(encoding='utf-8')
 for bad in ['restore_reaction(', 'write_vitality(', 'setActionMode(', 'setPlaySpeed(', 'setBck(', 'setFrame(', 'OnAtSetBit(', 'OffAtSetBit(', 'ClrAtHit(', 'SetC(', 'SetR(', 'HOOK_SKIP_ORIGINAL','VirtualProtect','WriteProcessMemory','get_service(']:
  if bad in adapter:errors.append('FIX02 no new native writer allowed: '+bad)
 if re.search(r'->[\w.\[\]]+\s*=(?!=)',adapter):errors.append('FIX02 native pointer assignment forbidden')
 for token in ['!g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation)','!cp05fix02::ordinary_link_sword(rep,g.cur.from_link)','!post05::same_anim(g.cur.pre_aux,r.aux)','a->mpMorf->getFrame()!=r.aux.anim_frame','a->mpMorf->getPlaySpeed()!=r.aux.anim_rate','a->mpMorf->isStop()','cp05fix02::cleanup_contact(g.cur.pin,g.cur.pv)','last.priority=-300','f2.monitor.shutdown()','g.layout=layout_matches_binary(a)?1:0;','if(g.layout!=1||g.rx.session_disabled())return;','cCcD_ObjHitInf::ChkAtHit()','cCcD_ObjHitInf::GetAtHitObj()']:
  if token not in adapter:errors.append('FIX02 production proof/binding missing: '+token)
 if adapter.count('fix01_cleanup_request();')!=1:errors.append('FIX02 must use the single existing paired registration cleanup')
 core=(p/'src/cp05_fix02_core.hpp').read_text(encoding='utf-8')
 for bad in ['uintptr_t','void*','daE_OC_c','daAlink_c','cM_rnd']:
  if bad in core:errors.append('FIX02 value core contains native address/RNG: '+bad)
 if len(pin['baseline_files'])!=34:errors.append('FIX02 frozen production inventory must contain34 prior files')
 return errors
if __name__=='__main__':
 e=audit(Path(sys.argv[1]));print(json.dumps({'status':'SOURCE_AUDIT_FAIL' if e else 'SOURCE_AUDIT_PASS','base_source':policy()['base_source'],'frozen_prior_files':34,'new_native_writers':0,'errors':e},indent=2));sys.exit(bool(e))
