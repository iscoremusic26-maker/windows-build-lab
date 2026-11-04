"""Exact native collision-list lifecycle and stop predicate used by FIX02."""
from pathlib import Path
import json,sys
root=Path(__file__).resolve().parents[1];sdk=Path(sys.argv[1]);errors=[]
s=(sdk/'src/SSystem/SComponent/c_cc_s.cpp').read_text(encoding='utf-8')
refs=[('void cCcS::Set(cCcD_Obj* obj) {','\nvoid cCcS::ClrCoHitInf()','fix02_native_cc_register.inc'),('void cCcS::ClrAtHitInf() {','\nbool cCcS::ChkNoHitAtTg','fix02_native_cc_clear.inc')]
for a,b,n in refs:
 i=s.index(a);j=s.index(b,i)
 if s[i:j].rstrip()+'\n'!=(root/'tests/reference'/n).read_text(encoding='utf-8'):errors.append('FIX02 native list reference mismatch '+n)
part=s[s.index('void cCcS::ChkAtTg() {'):s.index('void cCcS::ChkCo()') if 'void cCcS::ChkCo()' in s else len(s)]
if not part.index('ClrAtHitInf();')<part.index('!(*pat_obj)->ChkAtSet()'):errors.append('FIX02 cleanup must occur before inactive-AT rejection')
m=(sdk/'include/m_Do/m_Do_ext.h').read_text(encoding='utf-8')
if 'return mFrameCtrl.checkState(1) || mFrameCtrl.getRate() == 0.0f;' not in m:errors.append('Native isStop contract changed')
a=(sdk/'src/d/actor/d_a_e_oc.cpp').read_text(encoding='utf-8')
for token in ['if (mpMorf->getFrame() >= 14.0f && mpMorf->getFrame() <= 22.0f)', 'if (my_bool && daPy_getPlayerActorClass()->checkPlayerGuard())', 'mpMorf->setPlaySpeed(-1.0);','if (!mpMorf->isStop())']:
 if token not in a:errors.append('FIX02 native boundary/recoil contract changed: '+token)
print(json.dumps({'status':'NATIVE_REFERENCE_SOURCE_PASS' if not errors else 'NATIVE_REFERENCE_SOURCE_FAIL','sdk_commit':'40457c6adb381928e4b5fef6ed459ed291edd5e2','exact_new_references':2,'native_stop':'state1 OR rate0; terminal proof additionally requires exact B/C endpoint','native_guard_failure':'last-pass contact at22 read after22: no fresh AtSet registration, residual hit survives and rebounds subsequent starts before14 while Link guards','inactive_registration_safety':'native ClrAtHitInf precedes native ChkAtSet rejection','native_game_executed':False,'errors':errors},indent=2));sys.exit(bool(errors))
