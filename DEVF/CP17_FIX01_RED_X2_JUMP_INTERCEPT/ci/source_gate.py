"""CP17 FIX01 closed delta against CP17: Red adapter/policy only, one read-only CP14 forwarding line, version/header; Blue FIX01/CP14/Black byte-preserved."""
import re,json,hashlib
N='CP17_FIX01_RED_X2_JUMP_INTERCEPT';O='CP17_RED_AGGRESSIVE_AI'
OLD_A='// ---- CP17 Red aggressive AI:';OLD_B='// ---- end CP17 Red aggressive AI ----'
NEW_A='// ---- CP17 FIX01 Red x2 / jump interception:';NEW_B='// ---- end CP17 FIX01 Red ----'
FORWARD='   red_ai_damage_confirmed(d->id,d->generation);   // CP17 FIX01: read-only forwarding of CP14\'s own attribution\n'
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def cut(text,a,b):i=text.index(a);j=text.index(b,i)+len(b);return text[i:j],text[:i]+text[j:]
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 new=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8');old=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8')
 adapter,normal=cut(new,NEW_A,NEW_B);_,oldnormal=cut(old,OLD_A,OLD_B)
 assert normal.count(FORWARD)==1,'exactly one CP14 forwarding line';normal=normal.replace(FORWARD,'')
 header=normal.index('// (CP16 FIX01 header follows.)\n');normal=normal[header+len('// (CP16 FIX01 header follows.)\n'):]
 normal=normal.replace('kVersion="0.14.1"','kVersion="0.14.0"')
 oldnormal=oldnormal.replace('#include <Z2AudioLib/Z2SeMgr.h>\n','')   # only the removed 0.14.0 B SFX compensation used it
 assert normal==oldnormal,'Non-Red CP17 runtime changed'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel in [N+'/expansion/mod.cpp',N+'/expansion/red_ai_policy.hpp']:continue
  prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==24,identical
 oldimp=json.loads((repo/'DEVF'/O/'ci/reviewed-imports.json').read_text(encoding='utf-8'));imp=json.loads((task/'ci/reviewed-imports.json').read_text(encoding='utf-8'))
 added=['?SetC@cM3dGSph@@QEAAXAEBUcXyz@@@Z','?SetR@cM3dGSph@@QEAAXM@Z'];removed=['?startCreatureSound@Z2CreatureEnemy@@UEAAPEAVZ2SoundHandlePool@@VJAISoundID@@IC@Z']
 for key in oldimp:assert sorted(imp[key])==sorted([s for s in oldimp[key] if s not in removed]+(added if key=='dusklight.exe' else [])),key
 assert set(imp)==set(oldimp)
 active=code(new);ad=code(adapter)
 # Closed write surface of the Red adapter (all on the executing Red `a`): exactly these assignments/calls, nothing on Link/HP/flags/frames/timers/position.
 writes=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',ad))
 assert writes==['a->speedF=','a->speedF=','a->speedF='],writes
 assert ad.count('a->setActionMode(devf_variant::A_ATTACK,0)')==1 and ad.count('setActionMode(')==1
 assert ad.count('->setPlaySpeed(')==2 and ad.count('.SetC(')==2 and ad.count('.SetR(')==2 and ad.count('mSphs_at[0].SetC(')==1 and ad.count('mSphs_at[1].SetC(')==1
 assert not re.findall(r'\b(?:p|k|v)->[\w\.\[\]]+\s*=(?!=)',ad),'Link writes'
 for x in ['mMoveRange=','home.pos','current.pos.x=','current.pos.y=','current.pos.z=','SetAtAtp(','OnAtSetBit(','OffAtSetBit(','setFrame(','setFrameF(','setRate(','field_0x6c0=','field_0x6c2=','mProcID=','a->health','ClrAtHit','SetAtHit','OnAtHit','startCreatureSound','setDamagePoint','dComIfGs_setLife','mSphs_cc']:assert x not in ad,x
 oldwrites=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(oldnormal)));newwrites=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(normal)));assert newwrites==oldwrites
 assert active.count('DEFINE_HOOK(')==1 and active.count('hook::add_pre<')==1 and active.count('hook::add_post<')==1
 assert active.count('setPlaySpeed(')==5
 assert 'recruits.find(' not in ad,'Red must use its aligned slot, not scan recruitment ledger per actor'
 assert ad.count('e->variant!=devf_variant::Variant::Red')==1 and ad.count('d->generation!=e->generation')==1
 assert 'LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED' in new and 'kVersion="0.14.1"' in new
 assert 'kBurst' in ad and '3.0f' not in ad and 'Z2SE_' not in new
 policy=(rt/N/'expansion/red_ai_policy.hpp').read_text(encoding="utf-8");pc=code(policy)
 assert not re.search(r'\b(daAlink_c|daPy_py_c|daE_OC_c|fopAc_ac_c)\s*\*',pc),'game pointer in value policy'
 for token in ['kNormal=1.15f,kBurst=2.0f,kRush=1.5f,kFindSpeed=20.0f','kRandomPercent=12,kRandomMin=120,kRandomMax=210,kReactiveCooldown=45','kPassive=105,kPassiveMin=180,kPassiveMax=300,kIntent=90','kOpen=8,kClose=12','kProcCutJump=0x26,kProcCutJumpLand=0x27,kProcLargeJump=0x30,kProcLargeJumpLand=0x31','kReactRadius=900.','kHeight=200.','kLeashMargin=100.','kTipR=45.f,kHiltR=30.f,kLinkR=35.f']:assert token in pc,token
 for gone in ['kChargePercent','kBurst=3.0f','lost_b_sfx','before_blow','chance(s,kChargePercent)','std::sqrt','std::atan2','std::lround','<cmath>','std::sin','std::cos']:assert gone not in pc and gone not in policy,gone   # no CRT math imports beyond the reviewed set
 assert re.search(r'inline Jump jump_view\(',pc) and 'proc==kProcCutJump' in pc and 'proc==kProcLargeJump' in pc
 official=repo/'.official';facts={}
 for rel,expected in json.loads((task/'ci/official-source-blobs.json').read_text(encoding="utf-8")).items():
  p=official/rel
  if not p.exists():continue
  raw=p.read_text(encoding='utf-8').encode();blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest();assert blob==expected,rel
  facts[rel]=dict(git_blob=blob,normalized_sha256=hashlib.sha256(raw).hexdigest())
 if official.exists():assert len(facts)==17
 if facts:
  rd=lambda rel:(official/rel).read_text(encoding="utf-8")
  src=rd('src/d/actor/d_a_e_oc.cpp');j3d=rd('libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp');alink=rd('include/d/actor/d_a_alink.h');cut_inc=rd('src/d/actor/d_a_alink_cut.inc')
  for text in ['if (mpMorf->checkFrame(10.0f))','else if (mpMorf->checkFrame(12.5f))','mpMorf->getFrame() >= 14.0f && mpMorf->getFrame() <= 22.0f','mpMorf->setPlaySpeed(-1.0);','current.pos.x += (my_float - field_0x6a0)','mpMorf->play(0,','mSphs_at[0].OffAtSetBit();','cLib_chaseF(&speedF, 20.0f + nREG_F(0), 1.0f);',
               'cLib_chaseF(&speedF, -3.0f, 1.0f);','if (pl_dist < 400.0f && pl_dist > 200.0f) {','if (cLib_chaseF(&speedF, 0.0f, 1.0f)) {','if (mOcState < 3 || !setWatchMode()) {','if (checkBeforeFloorBg(200.0f)) {',
               'my_vec_lhs.set(0.0f, 0.0f, 70.0f);','mSphs_at[0].SetR(45.0f);','my_vec_lhs.set(0.0f, 0.0f, 20.0f);','mSphs_at[1].SetR(30.0f);','dComIfG_Ccsp()->Set(&mSphs_at[1]);','fopAcM_posMoveF(this, mStts.GetCCMoveP());','field_0x6c0 = field_0x6c2 = 0;']:assert text in src,text
  assert src.index('damage_check();')<src.index('checkFall();')<src.index('executeFind();')<src.index('fopAcM_posMoveF(this, mStts.GetCCMoveP());')<src.index('mpMorf->play(0, dComIfGp_getReverb')
  for text in ['mFrame <= passFrame && passFrame < next_frame','f32 next_frame = mFrame + mRate;']:assert text in j3d,text
  for text in ['virtual BOOL checkCutCharge() const { return mProcID == PROC_CUT_TURN_MOVE; }','u16 mProcID;','mProcID == PROC_CUT_TURN_MOVE && mProcVar2.field_0x300c == 0','mProcID == PROC_CUT_TURN_MOVE && mProcVar2.field_0x300c != 0','virtual BOOL checkCutJumpMode() const { return mProcID == PROC_CUT_JUMP; }','fopAc_ac_c* mTargetedActor;','dBgS_LinkAcch mLinkAcch;','/* 0x026 */ PROC_CUT_JUMP,','/* 0x030 */ PROC_CUT_LARGE_JUMP,']:assert text in alink,text
  for text in ['speed.y = mpHIO->mCut.mCutJump.m.mBaseJumpSpeedV;','if (mLinkAcch.ChkGroundHit()) {\n        procCutJumpLandInit(field_0x3198);','!checkModeFlg(2) && frameCtrl->getFrame() >= 5.0f','speed.y = mpHIO->mCut.mCutLargeJump.m.mCutSpeedV;','procCutLargeJumpLandInit(field_0x3198);','if (squareDist > 500.0f) {']:assert text in cut_inc,text
  assert 'void cM3dGSph::SetC(const cXyz& p) {' in rd('src/SSystem/SComponent/c_m3d_g_sph.cpp') and 'mRadius = r;' in rd('src/SSystem/SComponent/c_m3d_g_sph.cpp')
  ccs=rd('src/SSystem/SComponent/c_cc_s.cpp');assert 'void cCcS::ClrAtHitInf() {\n    for (cCcD_Obj** obj = mpObjAt; obj < mpObjAt + mObjAtCount; ++obj) {' in ccs and 'CalcArea();\n    ChkAtTg();' in ccs
  assert 'if (step * (*pvalue - target) >= 0) {' in rd('src/SSystem/SComponent/c_lib.cpp')
  hio=rd('src/d/actor/d_a_alink_HIO_data.inc');assert '    25.0f,\n    27.0f,\n    15.0f,\n    13.0f,' in hio and '    20.0f,\n    33.0f,\n    2.0f,\n    6.0f,' in hio and '    -3.4f,' in hio
  assert '35.0f,\n            180.0f,' in rd('src/d/actor/d_a_alink.cpp')
 return dict(identical_runtime_files_to_cp17=identical,outside_red_adapter_unchanged=True,cp14_forwarding_lines=1,red_adapter_assignments=writes,added_game_imports=added,removed_game_imports=removed,official_source=facts),dict(
  cp17_fix01='Red fast blow exactly x2 (decided at fresh frame 1, applied at frame 2: even phase, five native AT registrations); normal x1.15. Per tick of an owned x2 blow the two native AT spheres cover the skipped native pose (one collision pass). Interception of PROC26/PROC30 only by a bounded native ATTACK start; every eligible charge episode reacts; separate reactive/random cooldowns; FIND state-4 rush exact x1.5 of the native chase, restored after posMoveF; transition-only Link sample telemetry with a critical log reserve.',
  cp17='Non-Red runtime and 24 runtime files byte-identical to CP17; CP17 64-case lab kept as historical regression on its own compiled snapshot.',
  cp14='Lifesteal/status/collider canary, gold/group regen unchanged; one read-only line forwards a CP14-confirmed hit to CP17 telemetry.',
  budget='64 entries, fixed-size state, no heap per frame, O1 per own execute; logs 700 routine + 300 critical reserve per session, 40 + 24 per generation; 40 summaries.',
  contact='Lab geometry on synthetic swings only; not engine collision, audio or animation. Human runtime pending.')
