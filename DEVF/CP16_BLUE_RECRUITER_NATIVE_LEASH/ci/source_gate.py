"""Closed write/import/provenance gate. CP15 evasion tests remain historical; actual CP16 may not contain their side effects."""
import json,re,hashlib
from pathlib import Path
N='CP16_BLUE_RECRUITER_NATIVE_LEASH';O='CP15_BLUE_AI_GROUP_SPEED_SOLO_EVASION'
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 new=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8');old=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8')
 start='// ---- CP15 Blue AI adapter';end='// CP10.4 universal tracking'
 oldout=old[:old.index(start)]+old[old.index(end):]
 newout=new[:new.index('// ---- CP16 Blue recruiter')]+new[new.index(end):]
 newout=newout.replace('#include "blue_recruit_policy.hpp"\n','').replace('kVersion="0.13.0"','kVersion="0.12.0"').replace('recruit_post(a,fopAcM_GetID(a));','').replace('recruit_poll();','').replace('recruit_shutdown();','')
 newout=newout.replace('x.ra=devf_blue::kRatePair;probe_state.id=0;','x.ra=devf_blue::kRatePair;x.d1=devf_blue::kEvadeRadius;probe_state.id=0;').replace('BLUE_RECRUITER_1800_250_NATIVE_LEASH_GROUP_1_2_3_RATE_1_15_1_30_LOCKOUT_17_15','BLUE_ONLY_GROUP_1_2_3_SOLO_EVADE_RATE_1_15_1_30_LOCKOUT_17_15') # explicit init-telemetry replacement only
 assert code(newout)==code(oldout),'Non-Blue CP15 runtime changed'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel.startswith(N+'/'):continue
  assert p.read_bytes()==(oldrt/rel).read_bytes(),rel;identical+=1
 assert identical==22
 active=code(new);adapter=code(new[new.index('// ---- CP16 Blue recruiter'):new.index(end)])
 forbidden=['step_evade','veto_attack_start','EVADE_START','field_0x6ce=','field_0x6da=','shape_angle.y=','speedF=','mActionMode=','mOldActionMode=','mOcState=','mMoveRange=','mPlayerRange=','home.pos.x=','home.pos.y=','home.pos.z=','current.pos.x=','current.pos.y=','current.pos.z=','fopAcM_gc_c::gndCheck','checkBeforeBg(','checkBeforeFloorBg(']
 # Fields can be read or compared; only direct assignments are forbidden.
 for f in forbidden:
  if f.endswith('='):assert not re.search(re.escape(f)+r'(?!=)',active),f
  else:assert f not in active,f
 writes=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',active))
 expected=sorted(['a->health=']*4+['a->field_0x6c0=','a->field_0x6c2=']+[f'a->mWatchPos.{c}=' for c in 'xyz']*5)
 assert writes==expected,writes
 setters=re.findall(r'a->setActionMode\(([^;]+)\);',adapter)
 assert sorted(setters)==sorted(['devf_recruit::Wait,0','devf_recruit::Sound,0','devf_recruit::Watch,3','devf_recruit::Find,2','devf_recruit::Watch,0']),setters
 assert 'if(a->mOcState==2&&a->field_0x6c0<2)a->field_0x6c0=2;' in adapter
 assert active.count('setPlaySpeed(')==3 and active.count('DEFINE_HOOK(')==1 and active.count('hook::add_pre<')==1 and active.count('hook::add_post<')==1
 assert 'LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED' in new,'prior CP14 known fault hidden'
 policy=(rt/N/'expansion/blue_recruit_policy.hpp').read_text(encoding='utf-8');pcode=code(policy)
 assert not re.search(r'\b(daE_OC_c|fopAc_ac_c)\s*\*',pcode),'new pointer state'
 assert 'kRecall=150' in pcode and 'kBudget=180' in pcode and 'kMaxBudget=240' in pcode and 'kNoProgress=60' in pcode
 assert 'kRadius=1800.,kHeight=250.,kMargin=100.' in pcode and 'f.range==0' in pcode
 blue=(rt/N/'expansion/blue_ai_policy.hpp').read_text(encoding='utf-8')
 oldblue=(oldrt/O/'expansion/blue_ai_policy.hpp').read_text(encoding='utf-8')
 # Group filter / tracker / rates / lockout implementations themselves must remain byte-identical.
 for a,b in [('template<class S','// What the PRE'),('constexpr bool in_blow','// ---------------------------------------------------------------- 3.')]:
  chunk=oldblue[oldblue.index(a):oldblue.index(b)] if b in oldblue else oldblue[oldblue.index(a):]
  if a=='constexpr bool in_blow':chunk=chunk[:chunk.index('struct Rollback')] if 'struct Rollback' in chunk else chunk
  # lockout function compared explicitly below; CP15 veto is intentionally deleted.
  if a=='template<class S':assert code(chunk)==code(blue[blue.index(a):blue.index('// What the PRE')])
 for a,b in [('constexpr bool in_blow','// executeFind state 0'),('constexpr int lockout_write','\n}')]:
  def part(s):
   i=s.index(a);j=s.index(b,i+len(a));return s[i:j+len(b)]
  assert part(blue)==part(oldblue),a
 # Header code is typed against official source; actual link + exact importlib membership and PE imports remain separate Windows gates.
 official=repo/'.official';cpp=official/'src/d/actor/d_a_e_oc.cpp';native={}
 if cpp.exists():
  source=cpp.read_text(encoding='utf-8');header=(official/'include/d/actor/d_a_e_oc.h').read_text(encoding='utf-8')
  for text in ['void daE_OC_c::setActionMode(int i_action, int i_state)', 'void daE_OC_c::executeSoundWatch()', 'void daE_OC_c::executeWatch()', 'bool daE_OC_c::searchPlayer()']:
   assert text in source,text
  for text in ['setBck(7, 0, 5.0f, 1.0f);','mSound.startCreatureVoice(Z2SE_EN_OC_V_CRY, -1);','current.pos.abs(mWatchPos) < 300.0f','field_0x6c0 = 0x3c;','mOldActionMode = mActionMode;','field_0x6c0 = field_0x6c2 = 0;']:
   assert text in source,text
  for n in ['mWatchPos','mMoveRange','searchPlayer()','setActionMode(int'] :assert n in header,n
  native={'revision':'40457c6adb381928e4b5fef6ed459ed291edd5e2','cpp_sha256':hashlib.sha256(cpp.read_bytes()).hexdigest(),'typed_members_source_verified':True}
 return dict(identical_to_cp14=identical,identical_to_cp15=identical,outside_cp16_adapter_unchanged=True,closed_actor_assignments=writes,native_setter_calls=setters,native_source=native),dict(cp16='Own-execute native SoundWatch recruitment; direct native detection, own leash, exact context, bounded progress and PRE waiting. Solo evasion and POST veto absent. Human navigation/audio pending.',cp15='Levels/rates/lockout byte-preserved; historical evasion suite executed on historical CP15 only, not presented as CP16 behavior.',cp14='Every non-Blue runtime line unchanged; prior Link layout canary remains a known issue.',budget='64 value-only records; calls once/150 actor simulation ticks, 15-tick caller identity refresh, four stale-slot checks/update; no global actor census; logs capped1200.')
