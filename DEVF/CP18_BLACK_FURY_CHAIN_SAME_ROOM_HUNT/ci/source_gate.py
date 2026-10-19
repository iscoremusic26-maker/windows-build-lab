"""CP18 closed delta against CP17 FIX01: Black adapter/policy only plus seven marked integration lines and the version/header; Red, Blue FIX01, CP14,
CP10.6 and every other runtime file byte-preserved; the reviewed game import set is exactly CP17 FIX01's."""
import re,json,hashlib
N='CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT';O='CP17_FIX01_RED_X2_JUMP_INTERCEPT'
NEW_A='// ---- CP18 Black fury chain / same-room hunt:';NEW_B='// ---- end CP18 Black ----\n'
# The only lines outside the Black block that differ from CP17 FIX01 (each removed exactly once, then the remainder must equal CP17 FIX01 byte for byte).
LINES=['#include "black_ai_policy.hpp"\n',
 'bool black_ai_hunting(const devf_dyn::Entry* d)noexcept;void black_ai_damage(daE_OC_c* a,std::uint32_t id,const devf_dyn::Entry* d,bool start)noexcept;   // CP18: defined in the Black block\n',
 ' if(d->variant==devf_variant::Variant::Black&&(ev.kind==devf_dyn::Ev::FuryStart||ev.kind==devf_dyn::Ev::FuryRefresh))black_ai_damage(a,id,d,ev.kind==devf_dyn::Ev::FuryStart);   // CP18: read-only forwarding of CP10.6\'s own HP-loss event\n',
 ' if(r.order.active&&black_ai_hunting(d))devf_recruit::abort(r,recruit_log,"BLACK_HUNT_LOCKED_SAME_ROOM_PRIORITY_OVER_RECRUIT_ORDER");   // CP18: a provoked Black is not a callable companion\n',
 ' if(entry&&!fresh)black_ai_pre(a,id,entry);   // CP18: Black-only; after recruitment, so a released SOUND_WATCH lease is already visible\n',
 ' black_ai_post(a,fopAcM_GetID(a),e);         // CP18: Black-only fresh-blow rate, chain readback and hunt measurement after CP10.6\n']
SWAPS=[('recruit_poll();red_ai_poll();black_ai_poll();red_watch();','recruit_poll();red_ai_poll();red_watch();'),(' red_ai_shutdown();black_ai_shutdown();recruit_shutdown();\n',' red_ai_shutdown();recruit_shutdown();\n'),('kVersion="0.15.0"','kVersion="0.14.1"')]
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def cut(text,a,b):i=text.index(a);j=text.index(b,i)+len(b);return text[i:j],text[:i]+text[j:]
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 new=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8');old=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8')
 adapter,normal=cut(new,NEW_A,NEW_B)
 assert normal.count(NEW_A)==0 and adapter.count('\n')>100
 for l in LINES:assert normal.count(l)==1,('integration line',l[:60]);normal=normal.replace(l,'')
 for a,b in SWAPS:assert normal.count(a)==1,a;normal=normal.replace(a,b)
 h='// (CP17 FIX01 header follows.)\n';assert normal.count(h)==1;normal=normal[normal.index(h)+len(h):]
 assert normal==old,'Non-Black runtime changed against CP17 FIX01'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel in [N+'/expansion/mod.cpp',N+'/expansion/black_ai_policy.hpp']:continue
  prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==25,identical
 assert (rt/N/'expansion/red_ai_policy.hpp').read_bytes()==(oldrt/O/'expansion/red_ai_policy.hpp').read_bytes(),'Red policy must be byte-identical'
 oldimp=json.loads((repo/'DEVF'/O/'ci/reviewed-imports.json').read_text(encoding='utf-8'));imp=json.loads((task/'ci/reviewed-imports.json').read_text(encoding='utf-8'))
 assert imp==oldimp,'CP18 adds no import'
 ad=code(adapter);active=code(new)
 # Closed write surface of the Black adapter (all on the executing Black `a`): exactly these, nothing on Link/HP/ATP/flags/frames/home/range/leash/position.
 writes=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',ad))
 assert writes==['a->field_0x6c0=','a->field_0x6c2=','a->speedF='],writes
 assert ad.count('a->setActionMode(cmd.mode,cmd.state)')==1 and ad.count('setActionMode(')==1
 assert ad.count('->setPlaySpeed(')==2 and ad.count('mSphs_at[0].SetC(')==1 and ad.count('mSphs_at[1].SetC(')==1 and ad.count('.SetR(')==2
 assert not re.findall(r'\b(?:p|k|v)->[\w\.\[\]]+\s*=(?!=)',ad),'Link writes'
 for x in ['mMoveRange=','home.pos.x=','home.pos.y=','home.pos.z=','current.pos.x=','current.pos.y=','current.pos.z=','field_0x6e3=','SetAtAtp(','OnAtSetBit(','OffAtSetBit(','setFrame(','setFrameF(','setRate(','mProcID=','ClrAtHit','SetAtHit','OnAtHit','startCreature','setDamagePoint','dComIfGs_setLife','mSphs_cc[0].Set','mSphs_cc[1].Set','->fury=','.fury=','setBck(','mWatchPos','a->health=']:assert x not in ad,x
 assert ad.count('e->variant!=devf_variant::Variant::Black')==2 and 'recruits.find(' not in ad
 oldwrites=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(old)));newwrites=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(normal)));assert newwrites==oldwrites
 assert active.count('DEFINE_HOOK(')==1 and active.count('hook::add_pre<')==1 and active.count('hook::add_post<')==1 and active.count('setPlaySpeed(')==7
 assert 'kVersion="0.15.0"' in new and 'Z2SE_' not in new
 policy=(rt/N/'expansion/black_ai_policy.hpp').read_text(encoding='utf-8');pc=code(policy)
 assert not re.search(r'\b(daAlink_c|daPy_py_c|daE_OC_c|fopAc_ac_c)\s*\*',pc),'game pointer in value policy'
 for token in ['kNormal=1.15f,kFury=2.0f','kChainReach=400.,kChainHeight=200.','kChainFacing=0x1000','kBounceWindow=45,kBounceLimit=3,kBlockedTicks=90','#include "red_ai_policy.hpp"','ra::sweep(s.sweep,r,l)']:assert token in pc or token in policy,token
 for gone in ['std::sqrt','std::atan2','std::lround','<cmath>','std::sin','std::cos','kFuryTicks=','->fury=','.fury=']:assert gone not in pc,gone   # no CRT math; never writes the CP10.6 fury
 official=repo/'.official';facts={}
 for rel,expected in json.loads((task/'ci/official-source-blobs.json').read_text(encoding="utf-8")).items():
  p=official/rel
  if not p.exists():continue
  raw=p.read_text(encoding='utf-8').encode();blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest();assert blob==expected,rel
  facts[rel]=dict(git_blob=blob,normalized_sha256=hashlib.sha256(raw).hexdigest())
 if official.exists():assert len(facts)==19
 if facts:
  rd=lambda rel:(official/rel).read_text(encoding="utf-8")
  src=rd('src/d/actor/d_a_e_oc.cpp')
  # The end branch the chain pre-empts, the FIND state 0 pause, the FIND state 4 abandon checks, the leash update and the native entries reused.
  for text in ['            if (mpMorf->isStop()) {\n                setBck(0x1c, 2, 0.0f, 1.0f);\n                mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);\n                if (field_0x6e3) {\n                    setActionMode(E_OC_ACTION_MOVE_OUT, 0);',
               '            setBck(0x1c, 2, 5.0f, 1.0f);\n            mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);\n            if (field_0x6e3) {\n                setActionMode(E_OC_ACTION_MOVE_OUT, 0);\n                break;',
               '                    field_0x6c2 = field_0x6c0 + 0x14;\n                    if (checkBck(0x1c) == 0) {\n                        setBck(0x1c, 2, 5.0f, 1.0f);\n                        mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);',
               '                if (field_0x6c0 == 0) {\n                    if (!searchPlayer2()) {\n                        setActionMode(E_OC_ACTION_WAIT, 0);\n                    } else if (field_0x6e3) {\n                        setActionMode(E_OC_ACTION_MOVE_OUT, 0);',
               'if (home.pos.absXZ(current.pos) > mMoveRange + 300.0f) {','} else if (home.pos.absXZ(current.pos) < mMoveRange - 100.0f) {',
               '                if (cM_rndF(1.0f) < 0.5f) {\n                    setBck(5, 0, 5.0f, 1.0f);','if (cLib_chaseF(&speedF, 0.0f, 1.0f)) {','mOldActionMode = mActionMode;\n    mActionMode = i_action;\n    mOcState = i_state;\n    field_0x6c0 = field_0x6c2 = 0;',
               'if (mOcState < 3 || !setWatchMode()) {','if (checkBeforeFloorBg(200.0f)) {','mpMorf->getFrame() >= 14.0f && mpMorf->getFrame() <= 22.0f','mpMorf->setPlaySpeed(-1.0);',
               '    if (field_0x6c0) {\n        --field_0x6c0;\n    }\n    if (field_0x6c2) {\n        --field_0x6c2;\n    }']:assert text in src,text[:80]
  assert src.index('damage_check();')<src.index('    if (mMoveRange) {')<src.index('checkWaterSurface()) {')<src.index('checkFall();\n    field_0x566 = 0;')<src.index('        executeAttack();')<src.index('fopAcM_posMoveF(this, mStts.GetCCMoveP());')<src.index('mpMorf->play(0, dComIfGp_getReverb')
  assert src.count('Z2SE_EN_OC_V_WAIT_ST')==9,'every V_WAIT_ST source site accounted for: attack end x2, FIND x4, WATCH x2, setWaitStSound x1'
  assert 'void daE_OC_c::setWaitStSound() {\n    if (checkBck(0x1c)) {\n        if (mpMorf->checkFrame(0.0f)) {\n            mSound.startCreatureVoice(Z2SE_EN_OC_V_WAIT_ST, -1);' in src and 'void daE_OC_c::setFootNoteSound() {' in src,'the per-tick idle-clip voice only exists while BCK 0x1C is the clip'
  assert 'bool isStop() { return mFrameCtrl.checkState(1) || mFrameCtrl.getRate() == 0.0f; }' in rd('include/m_Do/m_Do_ext.h')
  j3d=rd('libs/JSystem/src/J3DGraphAnimator/J3DAnimation.cpp');assert '        if (mFrame >= mEnd) {\n            mFrame = mEnd - 0.001f;\n            mRate = 0.0f;\n            mState |= (u8)1;' in j3d and 'mFrame <= passFrame && passFrame < next_frame' in j3d
  al=rd('src/d/actor/d_a_alink.cpp');assert 'roomID = dComIfG_Bgsp().GetRoomId(mLinkAcch.m_gnd);\n\n        if (roomID != fopAcM_GetRoomNo(this)) {' in al and '            fopAcM_SetRoomNo(this, roomID);' in al
  mng=rd('include/f_op/f_op_actor_mng.h');assert 'inline void fopAcM_SetRoomNo(fopAc_ac_c* actor, s8 roomNo) {\n    actor->current.roomNo = roomNo;' in mng and 'inline s8 fopAcM_GetRoomNo(const fopAc_ac_c* i_actor) {' in mng
  assert 'cXyz dist = (*fopAcM_GetPosition_p(i_actorB) - *fopAcM_GetPosition_p(i_actorA));\n    return dist.abs();' in rd('src/f_op/f_op_actor_mng.cpp')
  assert 'fopAcM_SetRoomNo' not in src,'E_OC never changes its own room: the Black native room is constant'
  hdr=rd('include/d/actor/d_a_e_oc.h')
  for t in ['/* 0x690 */ f32 mMoveRange;','/* 0x6c0 */ s16 field_0x6c0;','/* 0x6c2 */ s16 field_0x6c2;','/* 0x6e3 */ u8 field_0x6e3;','/* 0x6b0 */ int mOcState;']:assert t in hdr,t
  assert 'void cM3dGSph::SetC(const cXyz& p) {' in rd('src/SSystem/SComponent/c_m3d_g_sph.cpp')
  assert 'if (step * (*pvalue - target) >= 0) {' in rd('src/SSystem/SComponent/c_lib.cpp')
 return dict(identical_runtime_files_to_cp17_fix01=identical,outside_black_adapter_unchanged=True,integration_lines=len(LINES)+len(SWAPS)-1,red_policy_byte_identical=True,black_adapter_assignments=writes,added_game_imports=[],official_source=facts),dict(
  cp18='Black native B/C x1.15; blows started during the CP10.6 fury x2 (even phase, CP17 FIX01 sweep reused). Fury chain: at the PRE of a complete clip the native ATTACK state 0 (in reach) or FIND state 3 (out of reach) replaces the executeAttack end branch (setBck 0x1C + V_WAIT_ST); FIND state 0 becomes FIND state 3 and the FIND state 4 lockout is cleared while fury>0. Same-room hunt armed by the forwarded CP10.6 HP-loss event, FIND4 timer held at 2, abandon modes re-enter the native FIND, reset on Link room/stage epoch change or actor end.',
  cp17_fix01='Red adapter, Red policy (byte-identical), Blue FIX01, CP14 and 25 runtime files byte-identical to CP17 FIX01; CP17 FIX01 53-case lab kept as regression.',
  budget='64 entries, fixed-size state, no heap per frame, O1 per own execute plus one Link read and four slot checks per management pass; 600 routine + 800 critical lines per session, 40 + 120 per generation; chained-blow detail 6 per fury episode; 40 summaries.',
  evidence='Lab on a source port of the daE_OC_c paths; not engine animation, audio or collision. Human runtime pending.')
