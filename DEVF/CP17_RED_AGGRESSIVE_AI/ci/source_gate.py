"""CP17 closed delta: Red adapter only, immutable Blue FIX01/CP14/Black, exact official source."""
import re,json,hashlib
N='CP17_RED_AGGRESSIVE_AI';O='CP16_FIX01_BLUE_RECRUITER_RANGE_DIAGNOSTICS'
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF';new=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8');old=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8')
 a=new.index('// ---- CP17 Red aggressive AI:');b=new.index('// ---- end CP17 Red aggressive AI ----',a)+len('// ---- end CP17 Red aggressive AI ----')
 adapter=new[a:b];normal=new[:a]+new[b:]
 normal=normal.replace('#include "red_ai_policy.hpp"\n','').replace('#include <Z2AudioLib/Z2SeMgr.h>\n','').replace('kVersion="0.14.0"','kVersion="0.13.1"')
 normal='\n'.join(l for l in normal.splitlines() if 'if(entry&&!fresh)red_ai_pre(' not in l and ' red_ai_post(a,fopAcM_GetID(a),e);' not in l)
 normal=normal.replace('red_ai_poll();','').replace('red_ai_shutdown();','')
 assert code(normal)==code(old),'Non-Red FIX01 runtime changed'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel in [N+'/expansion/mod.cpp',N+'/expansion/red_ai_policy.hpp']:continue
  prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==24,identical
 oldimp=json.loads((repo/'DEVF'/O/'ci/reviewed-imports.json').read_text());imp=json.loads((task/'ci/reviewed-imports.json').read_text());added=['?fopAcM_otherBgCheck@@YAHPEBVfopAc_ac_c@@0@Z','?startCreatureSound@Z2CreatureEnemy@@UEAAPEAVZ2SoundHandlePool@@VJAISoundID@@IC@Z']
 for key in oldimp:assert sorted(imp[key])==sorted(oldimp[key]+(added if key=='dusklight.exe' else [])),key
 active=code(new);ad=code(adapter)
 writes=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',active));oldwrites=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(old)));assert writes==oldwrites
 assert not re.findall(r'\b(?:p|k|v)->[\w\.\[\]]+\s*=(?!=)',ad),'Link writes'
 for x in ['setActionMode(','mMoveRange=','home.pos','current.pos.x=','SetAtAtp(','OnAtSetBit(','OffAtSetBit(','setFrame(','setFrameF(','setRate(','speedF=','field_0x6c0=','field_0x6c2=','mProcID='] :assert x not in ad,x
 assert ad.count('setPlaySpeed(')==2 and ad.count('Z2CreatureEnemy::startCreatureSound(')==1
 assert active.count('DEFINE_HOOK(')==1 and active.count('hook::add_pre<')==1 and active.count('hook::add_post<')==1
 assert active.count('setPlaySpeed(')==5
 assert 'recruits.find(' not in ad,'Red must use its aligned slot, not scan recruitment ledger per actor'
 assert ad.count('e->variant!=devf_variant::Variant::Red')==2 and ad.count('d->generation!=e->generation')==2
 assert 'LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED' in new
 policy=(rt/N/'expansion/red_ai_policy.hpp').read_text();pc=code(policy)
 assert not re.search(r'\b(daAlink_c|daPy_py_c|daE_OC_c|fopAc_ac_c)\s*\*',pc),'game pointer in value policy'
 for token in ['kNormal=1.15f,kBurst=3.0f','kRandomPercent=12,kChargePercent=70','kBurstMin=120,kBurstMax=210','kPassive=105,kPassiveMin=180,kPassiveMax=300','kIntent=90','kReactRadius=900.','kHeight=200.','kLeashMargin=100.'] :assert token in pc,token
 official=repo/'.official';facts={}
 for rel,expected in json.loads((task/'ci/official-source-blobs.json').read_text()).items():
  p=official/rel
  if not p.exists():continue
  raw=p.read_text(encoding='utf-8').encode();blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest();assert blob==expected,rel
  facts[rel]=dict(git_blob=blob,normalized_sha256=hashlib.sha256(raw).hexdigest())
 if official.exists():assert len(facts)==8
 if facts:
  src=(official/'src/d/actor/d_a_e_oc.cpp').read_text();j3d=(official/'libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp').read_text();alink=(official/'include/d/actor/d_a_alink.h').read_text()
  for text in ['if (mpMorf->checkFrame(10.0f))','else if (mpMorf->checkFrame(12.5f))','mpMorf->getFrame() >= 14.0f && mpMorf->getFrame() <= 22.0f','mpMorf->setPlaySpeed(-1.0);','current.pos.x += (my_float - field_0x6a0)','mpMorf->play(0,','mSphs_at[0].OffAtSetBit();','cLib_chaseF(&speedF, 20.0f + nREG_F(0), 1.0f);'] :assert text in src,text
  for text in ['mFrame <= passFrame && passFrame < next_frame','f32 next_frame = mFrame + mRate;']:assert text in j3d,text
  for text in ['virtual BOOL checkCutCharge() const { return mProcID == PROC_CUT_TURN_MOVE; }','u16 mProcID;','mProcID == PROC_CUT_TURN_MOVE && mProcVar2.field_0x300c == 0','mProcID == PROC_CUT_TURN_MOVE && mProcVar2.field_0x300c != 0']:assert text in alink,text
 return dict(identical_runtime_files_to_fix01=identical,outside_red_adapter_unchanged=True,closed_native_actor_assignments=writes,additional_game_imports=added,native_speed_boost=False,native_frame_or_collision_writes=False,official_source=facts),dict(cp17='Red shared policy64 cases; compiler-generated Windows Link layout/const virtual dispatch. x1.15/3 fixed at fresh B/C; one missing native B SFX compensated only with exact PRE/POST ownership. Physical contact/audio/Link state need human runtime.',cp16_fix01='Entire Blue adapter and policy headers byte-preserved; original53 and historical56 suites.',cp14='Lifesteal/status/collider canary, gold/group regen unchanged; known Link canary fault remains independent.',budget='64 entries,128B each,8192B total. O1 Red policy per own execute,4 slot cleanup/update. Logs900/session and48/generation. No global actor census, growing maps or new hooks.',approach='Native FIND only, speed/timers/home/range/physics untouched. PRE speedF boost rejected because action() can switch WATCH/damage/water before FIND; no claim of1.5 movement boost.',contact='Native frame3 stepping source port exercises16/19/22 active pre-frames, cc_set model19/22/25. No extra collision bit at25. This does not prove a physical engine hit, shield or SFX heard.')
