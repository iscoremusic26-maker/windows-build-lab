"""FIX01: compare against compiled CP16; closed native write/import/hook sets."""
import re,hashlib
N='CP16_FIX01_BLUE_RECRUITER_RANGE_DIAGNOSTICS';O='CP16_BLUE_RECRUITER_NATIVE_LEASH'
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 new=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8');old=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8')
 start='// ---- CP16 Blue recruiter';end='void blue_ai_post('
 def outside(s):return s[:s.index(start)]+s[s.index(end):]
 normal=outside(new).replace('kVersion="0.13.1"','kVersion="0.13.0"').replace('BLUE_RECRUITER_FIX01_RADIUS_FROM_POLICY_NATIVE_LEASH_GROUP_1_2_3_RATE_1_15_1_30_LOCKOUT_17_15','BLUE_RECRUITER_1800_250_NATIVE_LEASH_GROUP_1_2_3_RATE_1_15_1_30_LOCKOUT_17_15')
 assert code(normal)==code(outside(old)),'Changes outside recruitment adapter/version/init telemetry'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix();prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  if rel in [N+'/expansion/mod.cpp',N+'/expansion/blue_recruit_policy.hpp']:continue
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==23,identical
 assert (task/'ci/reviewed-imports.json').read_bytes()==(repo/'DEVF'/O/'ci/reviewed-imports.json').read_bytes()
 active=code(new);adapter=code(new[new.index(start):new.index(end)])
 forbidden=['step_evade','veto_attack_start','EVADE_START','field_0x6ce=','field_0x6da=','shape_angle.y=','speedF=','mActionMode=','mOldActionMode=','mOcState=','mMoveRange=','mPlayerRange=','home.pos.x=','home.pos.y=','home.pos.z=','current.pos.x=','current.pos.y=','current.pos.z=','fopAcM_gc_c::gndCheck','checkBeforeBg(','checkBeforeFloorBg(']
 for f in forbidden:
  if f.endswith('='):assert not re.search(re.escape(f)+r'(?!=)',active),f
  else:assert f not in active,f
 writes=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',active))
 expected=sorted(['a->health=']*4+['a->field_0x6c0=','a->field_0x6c2=']+[f'a->mWatchPos.{c}=' for c in 'xyz']*5)
 assert writes==expected,writes
 setters=re.findall(r'a->setActionMode\(([^;]+)\);',adapter)
 assert sorted(setters)==sorted(['devf_recruit::Wait,0','devf_recruit::Sound,0','devf_recruit::Watch,3','devf_recruit::Find,2','devf_recruit::Watch,0'])
 assert 'if(a->mOcState==2&&a->field_0x6c0<2)a->field_0x6c0=2;' in adapter
 assert active.count('setPlaySpeed(')==3 and active.count('DEFINE_HOOK(')==1 and active.count('hook::add_pre<')==1 and active.count('hook::add_post<')==1
 assert 'LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED' in new
 policy=(rt/N/'expansion/blue_recruit_policy.hpp').read_text(encoding='utf-8');pc=code(policy)
 assert not re.search(r'\b(daE_OC_c|fopAc_ac_c)\s*\*',pc)
 for token in ['kRecall=150','kCalm=150','kBudget=180','kMaxBudget=240','kNoProgress=60','kBackoff=90','kRefresh=15','kCallerGrace=10','kRadius=2800.,kHeight=250.,kMargin=100.','f.range==0'] :assert token in pc,token
 assert 'kRadius' in adapter and '2800' not in adapter
 # Native gesture, bounded wait, scratch timer restoration and progress algorithm are preserved.
 oldpc=(oldrt/O/'expansion/blue_recruit_policy.hpp').read_text(encoding='utf-8')
 assert policy[policy.index('template<class Emit>inline bool waiting'):]==oldpc[oldpc.index('template<class Emit>inline bool waiting'):]
 a=' if(fighting(r.f.mode)&&!o.engaged)'
 assert policy[policy.index(a):policy.index('template<class Emit>inline bool waiting')]==oldpc[oldpc.index(a):oldpc.index('template<class Emit>inline bool waiting')]
 official=repo/'.official';cpp=official/'src/d/actor/d_a_e_oc.cpp';native={}
 if cpp.exists():
  source=cpp.read_text(encoding='utf-8');header=(official/'include/d/actor/d_a_e_oc.h').read_text(encoding='utf-8')
  for text in ['void daE_OC_c::setActionMode(int i_action, int i_state)','void daE_OC_c::executeSoundWatch()','void daE_OC_c::executeWatch()','bool daE_OC_c::searchPlayer()','void daE_OC_c::executeDamage()','field_0x6cc = 10;','setBck(7, 0, 5.0f, 1.0f);','mSound.startCreatureVoice(Z2SE_EN_OC_V_CRY, -1);'] :assert text in source,text
  for text in ['mWatchPos','mMoveRange','searchPlayer()','setActionMode(int']:assert text in header,text
  native=dict(revision='40457c6adb381928e4b5fef6ed459ed291edd5e2',cpp_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest(),cpp_normalized_sha256=hashlib.sha256(source.encode()).hexdigest(),typed_members_source_verified=True,damage_grace='10 recipient executes; native normal hit debounce reference, not a measured animation duration')
 return dict(identical_runtime_files_to_cp16=identical,outside_recruit_adapter_unchanged=True,closed_actor_assignments=writes,native_setter_calls=setters,reviewed_imports_byte_identical=True,native_source=native),dict(fix01='Shared policy53 tests plus unchanged historical CP16 56. Radius2800, active combat recalls without claiming LOS, bounded diagnostics, consumed normal caller hit grace10. Physical game/audio pending.',cp15='Levels/rates/lockout header byte-identical to CP16; historical CP15 lab remains historical.',cp14='All non-Blue runtime code unchanged. Red Link canary remains pending.',budget='64 value-only entries; one bounded64-slot scan per call150 own executes. Diagnostic windows16call/16summary/8detail per30 management passes; session600/600/240 separate lifecycle1200. No global census or growing containers.')
