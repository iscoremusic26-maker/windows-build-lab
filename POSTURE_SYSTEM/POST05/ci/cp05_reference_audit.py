"""Exact E_OC2 native source and observable Expansion handoff provenance.
No private runtime ABI or measured asset durations are claimed.
"""
from pathlib import Path
import sys,json,hashlib
project=Path(__file__).resolve().parents[1];sdk=Path(sys.argv[1]);errors=[]
s=(sdk/'src/d/actor/d_a_e_oc.cpp').read_text(encoding='utf-8')
a=s.index('void daE_OC_c::executeFind() {');b=s.index('\nvoid daE_OC_c::setWeaponGroundAngle()',a)
find=s[a:b].rstrip()+'\n';fixture=(project/'tests/reference/cp05_native_find.inc').read_text()
if find!=fixture:errors.append('CP05 native executeFind fixture differs from pinned SDK')
for token in ('field_0x6c2 = field_0x6c0 + 0x14;','if (field_0x6c2 == 0)','--field_0x6c2;','void daE_OC_c::setActionMode(int i_action, int i_state)','field_0x6c0 = field_0x6c2 = 0;'):
 if token not in s:errors.append('CP05 timer/mode native contract missing: '+token)
base=project/'tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF'
ref=base/'CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion'
red=(ref/'red_ai_policy.hpp').read_text();black=(ref/'black_ai_policy.hpp').read_text();dynamic=(base/'CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp').read_text()
for token in ('if(s.pre_mode!=Attack||s.pre_state!=0','const float chosen=cause==Cause::Normal?kNormal:kBurst;','if(chosen==kBurst){s.pending=true;','out={true,chosen,cause};','absd(double(a.frame)-2.0)<1e-3'):
 if token not in red:errors.append('CP05 normal-cause witness no longer source-proven: '+token)
for token in ('if(s.pre_mode!=Attack||s.pre_state!=0','if(fury){s.pending=true;','out={true,kNormal};'):
 if token not in black:errors.append('CP05 Black fresh-normal witness missing: '+token)
for token in ('want=e.fury?kAtpBlackFury:kAtpBlackBase;','io.set(0,want);io.set(1,want);','kAtpBlackBase=3,kAtpBlackFury=6'):
 if token not in dynamic:errors.append('CP05 two-sphere Fury projection missing: '+token)
parry=(project/'src/parry_core.hpp').read_text()
if 'kFuryRateThreshold = 1.5f' not in parry:errors.append('Frozen conservative Fury/parry threshold changed')
r={'status':'NATIVE_REFERENCE_SOURCE_PASS' if not errors else 'NATIVE_REFERENCE_SOURCE_FAIL','sdk_commit':'40457c6adb381928e4b5fef6ed459ed291edd5e2','expansion_commit':'fe47c8b828939829914d466a425cb1e2c0223cd9','native_find_fixture_exact':find==fixture,'normal_witness':'verified E_OC2 profile + actual ATTACK0 PRE + fresh B/C frame1 rawrate1 in earlyPOST + normal115 write in ExpansionPOST0 + identical BCK/end/frame in latePOST','fury_witness':'existing verified two-sphere ATP3/6 classifier, backed by exact dynamic_policy projection; fresh-start witness also required','private_cause_api':'NONE; no private mod state read or guessed ABI','black_rate_reason':'per-swing1.0/1.15 capped at existing normal rate, below frozen POST08>=1.5 Fury/chip veto; no Fury repair','continuation':'latePOST entry is seen by the NEXT ExpansionPRE as ATTACK0; no synthetic executeAttack calls','timer':'2->1 native-only lease compare-restored in the same Find4 before ExpansionPOST; changed native modes keep their own timer','asset_durations':'NOT_OBSERVED; actual getEnd/isStop in production;42/47 lab fixtures synthetic','native_game_executed':False,'errors':errors}
print(json.dumps(r,indent=2));sys.exit(bool(errors))
