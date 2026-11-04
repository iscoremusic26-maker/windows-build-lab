"""CP05 exact authorized insertions + frozen CP04 source. No broad repinning."""
from pathlib import Path
import json,hashlib,re,sys
HERE=Path(__file__).resolve().parent
def policy():return json.loads((HERE/'cp05_policy.json').read_text())
def legacy_source(name,text):
 for edit in policy()['inverse_patch'].get(name,[]):
  if text.count(edit['after'])==1:text=text.replace(edit['after'],edit['before'],1)
 return text
def audit(project):
 p=Path(project);pin=policy();errors=[]
 if len(pin['baseline_pins'])!=32:errors.append('CP05 frozen production inventory must contain32 exact base files')
 for rel,want in {**pin['files'],**pin['baseline_pins']}.items():
  f=p/rel
  if not f.is_file():errors.append('CP05 reviewed/frozen source missing: '+rel);continue
  text=f.read_text(encoding='utf-8')
  if rel in pin['baseline_pins']:text=legacy_source(f.name,text)
  if hashlib.sha256(text.encode()).hexdigest()!=want:errors.append('CP05 reviewed/frozen source changed: '+rel)
 for name,edits in pin['inverse_patch'].items():
  s=(p/'src'/name).read_text(encoding='utf-8')
  for e in edits:
   if s.count(e['after'])!=1:errors.append('CP05 exact integration delta changed: '+name)
 s=(p/'src/cp05_adapter.inc').read_text(encoding='utf-8')
 # FIX01 adds one reviewed READ of Link's procedure for diagnosis, guarded by
 # the live ALINK profile. Preserve the global veto for every other occurrence.
 readonly='v.link_proc=static_cast<daAlink_c*>(p)->mProcID;'
 if s.count(readonly)!=1:errors.append('CP05 reviewed Link procedure read changed')
 checked=s.replace(readonly,'/* one reviewed procedure read */',1)
 for bad in ('HOOK_SKIP_ORIGINAL','mProcID =','setBck(','setFrame(','SetAtAtp','setDamagePoint','VirtualProtect','GetAc(', 'get_service(', 'mod_ctx->','mItemTrigger','mItemButton','mProcID','mDamageTimer','mUnderFrameCtrl','OnAtSetBit','OffAtSetBit','SetC(','SetR(','a->setActionMode('):
  if bad in checked:errors.append('CP05 forbidden native operation: '+bad)
 if re.search(r'\bhealth\s*=(?!=)',s):errors.append('CP05 forbidden native operation: health assignment')
 for token in ('a->mpMorf->setPlaySpeed(s.rate_value);','a->field_0x6c2=static_cast<s16>(value);','return cp04_native_mode(a,mode);'):
  if s.count(token)!=1:errors.append('CP05 exact native writer inventory changed: '+token)
 writes=re.findall(r'a->([\w.\[\]]+)\s*=(?!=)',s)
 if writes!=['field_0x6c2']:errors.append('CP05 game-pointer assignment outside the single scoped timer writer')
 # Every new module is fully hash-audited, including the .inc production unit.
 c=(p/'src/cp05_combo_core.hpp').read_text(encoding='utf-8')
 for bad in ('uintptr_t','void*','daE_OC','daAlink','cM_rndF'):
  if bad in c:errors.append('CP05 sidecar retains native addresses/types or RNG: '+bad)
 if 'late.priority=-200;early.priority=100;' not in s:errors.append('CP05 documented native timer-lease ordering missing')
 return errors
if __name__=='__main__':
 e=audit(Path(sys.argv[1]));print(json.dumps({'status':'SOURCE_AUDIT_FAIL' if e else 'SOURCE_AUDIT_PASS','compiled_base':policy()['compiled_base'],'errors':e,'native_runtime':False},indent=2));sys.exit(bool(e))
