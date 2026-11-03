"""Match the independently compiled native fixture to the pinned official
checkout, and record source origins for shield animation and hook precedence.
No asset durations inferred from a contact window or a synthetic lab fixture.
"""
from pathlib import Path
import sys,json,hashlib,re
project=Path(__file__).resolve().parents[1];sdk=Path(sys.argv[1]);errors=[]
paths=['src/d/d_cc_d.cpp','src/d/d_cc_s.cpp','src/d/actor/d_a_e_oc.cpp','include/d/actor/d_a_e_oc.h','src/d/actor/d_a_alink_guard.inc','src/d/actor/d_a_alink_HIO_data.inc','include/d/actor/d_a_alink.h','src/d/actor/d_a_alink.cpp','libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp','include/m_Do/m_Do_ext.h','sdk/include/mods/svc/hook.h','src/dusk/mods/svc/hook.cpp']
hashes={name:hashlib.sha256((sdk/name).read_bytes().replace(b'\r\n',b'\n')).hexdigest() for name in paths}
s=(sdk/'src/d/actor/d_a_e_oc.cpp').read_text(encoding='utf-8');a=s.index('void daE_OC_c::executeAttack()');b=s.index('\nvoid daE_OC_c::executeDamage()',a)
actual=s[a:b]+'\n';expected=(project/'tests/reference/cp04_native_attack.inc').read_text(encoding='utf-8')
if actual!=expected:errors.append('native executeAttack fixture differs from official checkout')
hook=(sdk/'sdk/include/mods/svc/hook.h').read_text(encoding='utf-8');impl=(sdk/'src/dusk/mods/svc/hook.cpp').read_text(encoding='utf-8')
if 'Higher runs first' not in hook or 'Applies to pre/post ordering' not in hook or 'return a.options.priority > b.options.priority;' not in impl:errors.append('official hook precedence contract differs')
guard=(sdk/'src/d/actor/d_a_alink_guard.inc').read_text(encoding='utf-8')
for x in ['setSingleAnimeParam(anm, &params->mAttackAnm);','daAlink_ANM anm = ANM_SHIELD_ATTACK;','mGuardAtCps.ResetAtHit();','field_0x347c = params->mAttackEndFrame;']:
 if x not in guard:errors.append('native shield route missing: '+x)
print(json.dumps({'status':'NATIVE_REFERENCE_SOURCE_PASS' if not errors else 'NATIVE_REFERENCE_SOURCE_FAIL','sdk_commit':'40457c6adb381928e4b5fef6ed459ed291edd5e2','native_attack_fixture_exact':actual==expected,'official_file_sha256':hashes,'shield_route':'procGuardAttackInit -> setSingleAnimeParam(ANM_SHIELD_ATTACK), native voice, collider reset; actual clip resource identity+positive frame progression','asset_durations':'NOT_OBSERVED_HERE; production reads actual loaded B/C mFrameCtrl.end; fixtures42/47 and23..120 are synthetic durations, never asset evidence','native_game_executed':False,'errors':errors},indent=2));sys.exit(bool(errors))
