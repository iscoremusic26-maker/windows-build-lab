"""CP19 closed delta against CP18 (compiled 71bfcb3): four runtime files change (mod.cpp, red/black/recruit policies), one is new (cp19_policy.hpp); every
other runtime file is byte-identical. Removed lines are an exact reviewed list; added lines outside the CP19 block carry a CP19 marker; closed write surface."""
import re,json,hashlib,difflib
N='CP19_BUGFIX';O='CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT'
A='// ---- CP19 fixes:';B='// ---- end CP19 ----\n'
CHANGED=['mod.cpp','red_ai_policy.hpp','black_ai_policy.hpp','blue_recruit_policy.hpp']
# sha256 prefixes of the exact CP18 lines each changed file may lose (reviewed one by one; see IMPLEMENTATION.md "Source delta").
def h(s):return hashlib.sha256(s.encode()).hexdigest()[:16]
REMOVED={
 'mod.cpp':['constexpr const char* kVersion="0.15.0";',' const bool safe=devf_recruit::usable(r.f)&&!r.f.hit&&!r.f.leash_out;','   if(a->mActionMode==devf_recruit::Sound&&a->mOcState==0)devf_recruit::consumed(r,recruit_log);',
  ' if(f.mode==devf_variant::A_ATTACK&&f.stopped&&d->fury&&p)f.los=fopAcM_otherBgCheck(a,p)==0;',' red_ai_shutdown();black_ai_shutdown();recruit_shutdown();'],
 'blue_recruit_policy.hpp':[' if(!leash(f,s.f.pos))return Skip::NativeLeash;',' if(!leash(r.f,s->f.pos))return reject("RECEIVER_NATIVE_LEASH_REJECTED");'],
 'red_ai_policy.hpp':[' if(dist<kForceDist&&!force_block(s,a,l,kForceFacing,true)){','  return forced(s,a,s.intent_cause,"REACTION_NATIVE_ATTACK_STARTED_IN_RANGE",emit);','struct Command{bool force=false,zero_speed=false,rush=false;float speed=0;Cause cause=Cause::Normal;const char* why="NONE";};'],
 'black_ai_policy.hpp':['enum class Op:std::uint8_t{None,Chain,Pursue,FuryFind3,KeepFind0,Redirect};','   if(dist<kChainReach&&dy<=kChainHeight&&gap<kChainFacing&&!a.wall&&a.los){',' if(!fury_on)s.lockout_logged=false;','  if(op==Op::Chain){',
  ' s.decided=true;++s.seq;s.blow_start=s.ticks;s.contact=s.link_contact=s.shield=s.registered=false;',
  ' else if(a.mode==Find&&a.state==4&&a.lockout>0){c.lockout=true;c.lockout_value=0;if(!s.lockout_logged){s.lockout_logged=true;emit("BLACK_FURY_LOCKOUT_CLEARED","FIND4_POST_ATTACK_LOCKOUT_OF_THIS_BLACK_ONLY_WHILE_FURY");}}'],
}
# Lines that change only by appending a CP19 token to a list or a log format (prefix match, the CP19 version must contain the marker).
PREFIX={
 'mod.cpp':[(' char text[1700];std::snprintf(text,sizeof(text),"DEVF_CP18_BLACK_AI event=%s','active_min_dist'),('  event,kVersion,a.id,a.gen,a.room,a.stage,s.ticks,','s.active_min'),('MOD_EXPORT ModResult mod_update(ModError*){','cp19_poll()')],
 'red_ai_policy.hpp':[(' for(const char* p:{"JUMP_","INTERCEPT",','CLOSE_IN_STARTED'),('template<class Emit>inline void cancel(State&s,Emit&&emit,const char*why)noexcept{','CLOSE_IN_UNREACHABLE')],
 'black_ai_policy.hpp':[(' ra::State sweep;Point link_pos{};bool link_pos_valid=false;','ra::State sweep'),(' for(const char* p:{"BLACK_HUNT","BLACK_CHAIN",','BLACK_STRIKE'),('inline bool chain_detail(const char*e)noexcept{','BLACK_STRIKE_STARTED'),
  ('struct Command{bool set_mode=false,zero_speed=false,timer=false,lockout=false','close=false'),('   }else set(Find,3,Op::Pursue,dist>','FURY_OUTSIDE_STRIKE_DISTANCE')],
 'blue_recruit_policy.hpp':[(' for(auto& r:t.entries){const auto&o=r.order;if(!o.active||o.caller!=s.f.id','!context(r.f,s.f))continue;')],
}
MARK=re.compile(r'CP19|cp19|close|Close|CLOSE|kStrike|Strike|STRIKE|active_min|0\.16\.0|ra::kStrike')
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def delta(old,new):
 rem=[];add=[];hunks=[]
 for op,a1,a2,b1,b2 in difflib.SequenceMatcher(None,old,new,autojunk=False).get_opcodes():
  if op in('replace','delete'):rem+=old[a1:a2]
  if op in('replace','insert'):add+=new[b1:b2];hunks.append(new[b1:b2])
 return rem,add,hunks
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel in [N+'/expansion/'+f for f in CHANGED]+[N+'/expansion/cp19_policy.hpp']:continue
  prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==23,identical
 summary={}
 for f in CHANGED:
  old=(oldrt/O/'expansion'/f).read_text(encoding='utf-8');new=(rt/N/'expansion'/f).read_text(encoding='utf-8')
  if f=='mod.cpp':
   i=new.index(A);j=new.index(B,i)+len(B);block=new[i:j];newc=new[:i]+new[j:]
  else:block='';newc=new
  rem,add,hunks=delta(old.splitlines(),newc.splitlines())
  allowed=set(REMOVED[f]);pre=PREFIX[f]
  for l in rem:
   if l in allowed:allowed.discard(l);continue
   m=[q for q in pre if l.startswith(q[0])];assert len(m)==1,(f,'unreviewed removed line',l[:100])
   assert sum(1 for x in add if x.startswith(m[0][0]) and m[0][1] in x)==1,(f,'replacement not marked',l[:100])
  assert not allowed,(f,'expected removals missing',sorted(allowed))
  for hk in hunks:assert any(MARK.search(l) for l in hk) or all(any(l.startswith(q[0]) for q in pre) for l in hk),(f,'added hunk without a CP19 marker',hk[0][:120])   # every inserted/replaced hunk is marked
  summary[f]=dict(removed=len(rem),added=len(add))
  if f=='mod.cpp':modblock=block;modnew=new
 ad=code(modblock);full=code(modnew)
 writes=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',ad))
 assert writes==['a->attention_info.flags=','a->field_0x6cc=','a->health=','a->mSound.field_0xa3='],writes
 assert ad.count('a->setActionMode(')==3 and ad.count('p->mCutType=')==2 and ad.count('sm->loadSeWave(')==1 and ad.count('sm->eraseSeWave(')==2
 assert len(re.findall(r'\b(?:p|k|v)->[\w\.\[\]]+\s*=(?!=)',full))==2,'the only Link writes are the HEAD_JUMP mask and its restore'
 for x in ['mMoveRange=','home.pos.x=','current.pos.x=','field_0x6e3=','SetAtAtp(','OnAtSetBit(','ClrAtHit','SetAtHit','setDamagePoint','dComIfGs_setLife','unMuteSceneBgm','bgmAllUnMute','muteSceneBgm','setBattle','stopBattleBgm','startCreature','startSound(']:assert x not in ad,x
 oldfull=code((oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8'))
 ow=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',oldfull));nw=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',full))
 extra=list(nw)
 for w in ow:extra.remove(w)
 assert sorted(extra)==sorted(writes+['a->field_0x6c2=','a->field_0x6c2=','a->field_0x6c2=','a->speedF=','a->speedF=','a->speedF=','a->speedF=']),extra   # + Red/Black close-in speed and lease
 assert full.count('DEFINE_HOOK(')==1 and full.count('hook::add_pre<')==1 and full.count('hook::add_post<')==1 and 'kVersion="0.16.0"' in full
 oldimp=json.loads((repo/'DEVF'/O/'ci/reviewed-imports.json').read_text(encoding='utf-8'));imp=json.loads((task/'ci/reviewed-imports.json').read_text(encoding='utf-8'))
 added=['?mAudioMgrPtr@Z2AudioMgr@@2PEAV1@EA','?loadSeWave@Z2SceneMgr@@QEAA_NI@Z','?eraseSeWave@Z2SceneMgr@@QEAA_NI@Z','?getWaveLoadStatus@Z2SceneMgr@@QEAAHII@Z','?checkMasterSwordEquip@daPy_py_c@@SAHXZ','?dComIfGs_getSelectEquipSword@@YAEXZ']
 assert set(imp)==set(oldimp)
 for k in oldimp:assert sorted(imp[k])==sorted(oldimp[k]+(added if k=='dusklight.exe' else [])),k
 pol=(rt/N/'expansion/cp19_policy.hpp').read_text(encoding='utf-8');pc=code(pol)
 assert not re.search(r'\b(daAlink_c|daPy_py_c|daE_OC_c|fopAc_ac_c|Z2SceneMgr|Z2AudioMgr)\s*\*',pc),'game pointer in value policy'
 for t in ['kFieldSeWave1=0x4a,kOcWave=0x5e','kHelmMaster=100,kHelmOrdon=50','kCutHeadJump=5,kCutTurnRight=8','kPermitCalm=150','kResidualTicks=30']:assert t in pc,t
 assert 'constexpr double kStrike=240.,kNativeAttackMax=400.' in (rt/N/'expansion/red_ai_policy.hpp').read_text(encoding='utf-8')
 for gone in ['std::sqrt','<cmath>','std::atan2']:assert gone not in pc,gone
 official=repo/'.official';facts={}
 for rel,expected in json.loads((task/'ci/official-source-blobs.json').read_text(encoding="utf-8")).items():
  p=official/rel
  if not p.exists():continue
  raw=p.read_text(encoding='utf-8').encode();blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest();assert blob==expected,rel
  facts[rel]=dict(git_blob=blob,normalized_sha256=hashlib.sha256(raw).hexdigest())
 if official.exists():assert len(facts)==28
 if facts:
  rd=lambda rel:(official/rel).read_text(encoding="utf-8")
  src=rd('src/d/actor/d_a_e_oc.cpp')
  for t in ['        if (daPy_getPlayerActorClass()->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP) {\n            health = 0;\n        }','    if (health <= 1) {\n        attention_info.flags = 0;',
            '            setActionMode(E_OC_ACTION_DAMAGE, my_val);\n            field_0x6cc = 3 + NREG_S(7);\n            return;','        case daPy_py_c::CUT_TYPE_TURN_RIGHT:\n        case daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT:\n        case daPy_py_c::CUT_TYPE_HEAD_JUMP:\n            return 6;',
            'attention_info.flags = fopAc_AttnFlag_BATTLE_e;','mSound.setEnemyName("E_oc");','    if (field_0x6cc) {\n        return;\n    }','                if (health < 1) {\n                    setActionMode(E_OC_ACTION_DEATH, 0);']:assert t in src,t[:70]
  assert src.count('attention_info.flags = ')==2,'create() BATTLE and the damage_check health<=1 clear are the only writers'
  cc=rd('src/d/d_cc_uty.cpp')
  for t in ['if (player_p->checkMasterSwordEquip()) {\n                    i_AtInfo->mAttackPower *= 2;','if (i_AtInfo->mAttackPower != 0 && i_enemy->health <= 0) {\n            i_AtInfo->mHitStatus = 2;\n            i_enemy->health = 0;','dusk::AchievementSystem::get().signal("enemy_killed");','i_AtInfo->mpSound->startCollisionSE(getHitId(i_AtInfo->mpCollider, uvar8),\n                                                    getMapInfo(i_AtInfo->mHitStatus));','    } else if (param_0 == 2) {\n        map_info = 32;']:assert t in cc,t[:70]
  scn=rd('src/Z2AudioLib/Z2SceneMgr.cpp')
  for t in ['    case Z2SCENE_HYRULE_FIELD:\n        se_wave1 = 0x4a;','            case 2:\n            case 3:\n            case 4:\n                se_wave2 = 0x5e;','            case 0:\n            case 5:\n            case 7:\n                se_wave2 = 0x5f;','            case 9:\n            case 10:\n            case 11:\n                se_wave2 = 0x60;','            case 1:\n            case 6:\n            case 15:\n                se_wave2 = 0x4b;','bool Z2SceneMgr::loadSeWave(u32 wave) {','s32 Z2SceneMgr::getWaveLoadStatus(u32 wave, u32 bank) {','        if (requestSeWave_2 != 0 && loadSeWave(requestSeWave_2)) {\n            loadedSeWave_2 = requestSeWave_2;\n        } else {\n            loadedSeWave_2 = 0;']:assert t in scn,t[:70]
  arc=rd('libs/JSystem/src/JAudio2/JASWaveArcLoader.cpp');assert '    if (mStatus != 0) {\n        return false;\n    }' in arc and 'bool JASWaveArc::erase() {\n    return mHeap.free();' in arc
  assert 'if (strcmp(dComIfGp_getStartStageName(), "F_SP121") == 0) {\n                    mDoAud_load2ndDynamicWave();' in rd('src/d/d_kankyo.cpp')
  cr=rd('src/Z2AudioLib/Z2Creature.cpp');assert '        case 0x20:\n        case 0x24: {\n            field_0xa3 = 1;' in cr and 'void Z2CreatureEnemy::setLinkSearch(bool isLinkSearch) {\n    if (field_0xa3 == 0) {' in cr
  om=rd('src/Z2AudioLib/Z2SoundObjMgr.cpp');assert '                if (enemy->field_0xa3 != 0) {\n                    bVar7 = true;' in om and '    } else if (!bVar7) {\n        Z2GetSeqMgr()->setBattleSearched(local_96);' in om
  assert '                } else if (mBattleDistState <= 2) {' in rd('src/Z2AudioLib/Z2SeqMgr.cpp')
  assert '    if (i_actor->attention_info.flags == 0) {\n        return 0;' in rd('src/d/d_attention.cpp')
  pl=rd('include/d/actor/d_a_player.h');assert '/* 0x05 */ CUT_TYPE_HEAD_JUMP,' in pl and '/* 0x08 */ CUT_TYPE_TURN_RIGHT,' in pl and 'u8 getCutType() const { return mCutType; }' in pl
 return dict(identical_runtime_files_to_cp18=identical,changed_files=summary,cp19_block_assignments=writes,link_writes='mCutType HEAD_JUMP<->TURN_RIGHT only',added_game_imports=added,official_source=facts),dict(
  cp19='A F_SP121 SE wave 0x5e loaded/erased through Z2SceneMgr for field rooms without the E_OC wave; B attention BATTLE restored on survivors (health>=2); C 0-HP live mode completed through BIG_DAMAGE 0, residual last-hit flag cleared; D Blue call leash permission; E/G x2 blows from <=240 after a native final approach; F Helm Splitter exact 100/50.',
  cp18='23 runtime files byte-identical to CP18; CP18 43-case lab kept as regression on its own folder; CP17 FIX01 53-case lab kept.',
  evidence='Lab on source ports (E_OC, Z2SceneMgr/JASWaveArc, damage_check/cc_at_check); not engine audio, collision or animation. Human runtime pending.')
