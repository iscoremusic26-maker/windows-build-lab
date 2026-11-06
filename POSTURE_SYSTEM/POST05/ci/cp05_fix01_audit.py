"""FIX01 reviewed code and exact retained per-actor RNG; additive barrier."""
from pathlib import Path
import hashlib,json,sys
import cp06_audit
HERE=Path(__file__).resolve().parent
def sha(s):return hashlib.sha256(s.encode()).hexdigest()
def body(s,anchor):
 start=s.index(anchor);opening=s.index('{',start);depth=1;i=opening+1
 while depth:
  if s[i]=='{':depth+=1
  elif s[i]=='}':depth-=1
  i+=1
 return s[start:i]
def audit(project):
 p=Path(project);policy=json.loads((HERE/'cp05_fix01_policy.json').read_text(encoding='utf-8'));errors=[]
 expected={'tests/reference/cp05_0_11_8/'+n for n in ['cp05_combo_core.hpp','cp05_adapter.inc']}|{'tests/reference/'+n for n in ['cp05_native_find.inc','cp05_native_damage.inc','cp05_native_set_action.inc','cp05_native_off_tg.inc','cp05_native_cc_set.inc','cp05_native_cyl_sph.inc','cp05_native_shield.inc']}
 if set(policy['files'])!=expected:errors.append('CP05 FIX01 native/RNG reference inventory must contain9 exact files')
 for rel,want in policy['files'].items():
  f=p/rel
  if not f.is_file() or sha(f.read_text(encoding='utf-8'))!=want:errors.append('CP05 FIX01 reviewed reference/source changed: '+rel)
 old=(p/'tests/reference/cp05_0_11_8/cp05_combo_core.hpp').read_text(encoding='utf-8');new=cp06_audit.legacy_source('cp05_combo_core.hpp',(p/'src/cp05_combo_core.hpp').read_text(encoding='utf-8'))
 for anchor in ['inline bool reduce3(',' void bind(',' Kind decide(']:
  try:
   if body(old,anchor)!=body(new,anchor):errors.append('CP05 FIX01 frozen RNG changed: '+anchor)
  except (ValueError,IndexError):errors.append('CP05 FIX01 frozen RNG scope missing: '+anchor)
 a=(p/'src/cp05_adapter.inc').read_text(encoding='utf-8')
 if 'ChkTgHit' in body(a,'bool cp05_safe('):errors.append('CP05 FIX01 premature raw Tg veto restored')
 if 'if(s->last_execute!=g.p8.tick)return HOOK_CONTINUE;' not in a:errors.append('CP05 FIX01 matching EOC PRE receipt missing')
 if 'cp05_weighted_native_hit(*s)' not in a or 'a->health<s->execute_hp' not in a:errors.append('CP05 FIX01 actual native injury priority missing')
 if 'const bool reserve=cls==0' not in a:errors.append('CP05 FIX01 critical example reserve missing')
 if 'kSlowRate=1.f,kFastRate=1.15f;' not in new:errors.append('CP05 FIX01 native normal sampling cap changed')
 return errors
if __name__=='__main__':
 errors=audit(Path(sys.argv[1]));print(json.dumps({'status':'SOURCE_AUDIT_FAIL' if errors else 'SOURCE_AUDIT_PASS','frozen_rng':'bind/decide/reduce3 byte-identical to compiled0.11.8','errors':errors,'native_runtime':False},indent=2));sys.exit(bool(errors))
