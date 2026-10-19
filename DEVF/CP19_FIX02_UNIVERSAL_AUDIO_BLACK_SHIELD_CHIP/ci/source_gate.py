"""CP19 FIX02 closed delta against CP19 FIX01 (compiled 5c8ce00): only mod.cpp changes and cp19f2_policy.hpp is new; every other runtime file (fixes A-G,
the FIX01 readiness gate in cp19_policy.hpp, CP18 Black, CP17 FIX01 Red, CP16 Blue and earlier) is byte-identical to FIX01. Ordering and surface asserts:
no audio call before the FIX01 gate, every audio/ARAM access inside the engine adapter, no erase of any SE wave group, the loader root heap set only
around one loadSeWave call, exactly one life write (meter pending count), Link written only in the render tint of the modelDraw PRE/POST pair."""
import re,json,hashlib,difflib
N='CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP';O='CP19_FIX01_STARTUP_AUDIO_READINESS'
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def delta(old,new):
 rem=[];hunks=[]
 for op,a1,a2,b1,b2 in difflib.SequenceMatcher(None,old,new,autojunk=False).get_opcodes():
  if op in('replace','delete'):rem+=old[a1:a2]
  if op in('replace','insert'):hunks.append(new[b1:b2])
 return rem,hunks
def body(text,head):i=text.index(head);j=text.index('\n}\n',i)+3;return text[i:j]
def block(text,head,end='\n};\n'):i=text.index(head);return text[i:text.index(end,i)+len(end)]
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel in [N+'/expansion/mod.cpp',N+'/expansion/cp19f2_policy.hpp']:continue
  prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==27,identical
 assert not (oldrt/O/'expansion/cp19f2_policy.hpp').exists()
 om=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8');nm=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8')
 np_=(rt/N/'expansion/cp19f2_policy.hpp').read_text(encoding='utf-8');cn=code(nm)
 # ---- mod.cpp: removed lines are FIX01's audio adapter, cp19_poll/cp19_shutdown bodies and five reviewed lines; every added hunk is FIX02-marked ----
 oldio=block(om,'// FIX01: engine adapter of devf_cp19::audio_pass.').splitlines()
 allowed=set(oldio)|set((body(om,'void cp19_poll()noexcept{')+body(om,'void cp19_shutdown()noexcept{')).splitlines())|{
  'constexpr const char* kVersion="0.16.1";',
  'devf_cp19::AudioPass cp19_audio;unsigned cp19_passes=0,cp19_last_bokoblin=0,cp19_logs=0,cp19_summaries=0;   // FIX01: keeper/BGM state lives in the gated AudioPass'}
 rem,hunks=delta(om.splitlines(),nm.splitlines())
 for l in rem:
  if l in allowed:continue
  assert ('unsigned(devf_cp19::kOcWave),unsigned(cp19_audio.keeper.owned),why);' in l) or l.startswith('  kVersion,c.wave_loads,c.wave_load_failures,c.wave_erases,unsigned(cp19_audio.keeper.owned),') or l.startswith(' if(is("FIELD_SE_WAVE_LOAD_REQUESTED"))++cp19_counts.wave_loads;'),('unreviewed removed line',l[:120])
 for h in hunks:assert any(re.search(r'FIX02|cp19f2|0\.16\.2|LinkModelDraw|JASHeap|JKRAram|sAramHeap|Cp19AudioIo|cp19_audio|AUDIO_|Res::Ready|own_used|reserve|load_own|link_kcolor',l) for l in h),('unmarked hunk',h[0][:100])
 # ---- audio: every engine audio/ARAM access sits in Cp19AudioIo; the policy calls it only after the FIX01 gate; no erase anywhere ----
 io=block(nm,'struct Cp19AudioIo{');outside=nm.replace(io,'')
 for tok in ['getWaveLoadStatus(','loadSeWave(','getEnemyList(','mBattleDistState','isResetting(','JASWaveArcLoader::sAramHeap','JASKernel::getAramFreeSize(','JKRAram::getAramHeap(','initRootHeap(','getTotalFreeSize(','JKRHeap::alloc(']:
  assert tok in io and tok not in code(outside),('audio/ARAM access outside the gated adapter',tok)
 for tok in ['eraseSeWave(','eraseBgmWave(','->erase(','decWaveTable','incWaveTable','JKRHeap::free(mem,mh)==','JKRFreeToAram']:assert tok not in cn or tok=='JKRHeap::free(mem,mh)==',('no release of any wave group',tok)
 assert 'eraseSeWave' not in cn,'FIX02 never erases an SE wave group'
 assert code(io).count('JASWaveArcLoader::sAramHeap=')==2 and 'JASWaveArcLoader::sAramHeap=cp19f2_aram.heap;\n  const bool ok=scene()->loadSeWave(wave);\n  JASWaveArcLoader::sAramHeap=nullptr;' in io,'root heap set around exactly one loadSeWave call'
 assert 'if(JASWaveArcLoader::sAramHeap!=nullptr){why="FOREIGN_WAVE_LOADER_ROOT_HEAP_ALREADY_SET_NOT_TOUCHED";return false;}' in io
 assert code(nm).count('Cp19AudioIo io;')==1 and 'devf_cp19f2::audio_pass(cp19_audio,io,cp19f2_need||recent,cp19_audio_emit);' in nm and 'devf_cp19f2::audio_shutdown(cp19_audio,cp19_audio_emit);' in nm
 oldflags=block(om,' devf_cp19::AudioFlags flags()const{','\n }\n');assert oldflags in io,'FIX01 flags() byte-identical'
 ap=body(np_,'template<class Io,class Emit>inline AudioGate audio_pass(');g=ap.index('if(g!=AudioGate::Ready)return g;')
 assert ap.index('devf_cp19::audio_gate(io.flags())')<g and ap.index('resident_pass(st.res,io,need,emit);')>g and ap.index('io.bgm()')>g,'gate first'
 sd=body(np_,'template<class Emit>inline void audio_shutdown(');assert 'io.' not in sd and 'Io' not in sd,'shutdown makes no audio call'
 rp=body(np_,'template<class Io,class Emit>inline void resident_pass(')
 assert rp.index(' if(!need||r.st==Res::GaveUp)return;')<rp.index('io.status(kOcFamilyWave)')<rp.index('io.reserve(')<rp.index('io.load_own('),'need, status, then reservation, then load'
 assert rp.index(' if(io.transition()||r.passes<r.retry_at)return;')<rp.index('io.reserve(') and 'io.erase' not in rp and 'io.load(' not in rp
 assert 'constexpr std::uint8_t kOcFamilyWave=0x5e;' in np_ and 'kReserveBytes=4u<<20' in np_ and 'kGraphKeep=4u<<20' in np_ and 'kMaxAttempts=3' in np_
 for f in ['F_SP121','room==','req2==0x5f','field_without_oc','kFieldSeWave1']:assert f not in code(rp)+code(ap),('no stage/room list in the residency policy',f)
 assert 'io.field()' not in nm and 'bool field()const' not in io
 # ---- B: exactly one life write, the meter pending count; nothing of the native damage path ----
 assert cn.count('dComIfGp_setItemLifeCount(')==1 and 'dComIfGp_setItemLifeCount(-float(devf_cp19f2::kChipLifeUnits),type);' in nm and 'constexpr int kChipLifeUnits=1;' in np_
 chip=body(nm,'void cp19f2_chip_post(daE_OC_c* a,const devf_black_ai::State& s,const devf_dyn::Entry& d,const devf_black_ai::Actor& f)noexcept{')
 assert 'dComIfGp_setItemLifeCount(' in chip and chip.index('devf_cp19f2::chip_step(b,v)')<chip.index('dComIfGp_setItemLifeCount(') and chip.index('if(op!=ChipOp::Charge){')<chip.index('dComIfGp_setItemLifeCount(')
 for f in ['setDamagePoint','damageMagnification','procDamageInit','procCoLargeDamageInit','procGuardBreakInit','mDamageTimer=','mDamageColorTime','setDamageColorTime','dComIfGs_setLife','dComIfGp_setItemRupeeCount',
           'SetTgHit','ClrTgHit','ClrAtHit','OnTgShieldHit','OnAtShieldHit','OffTgShieldHit','GetTgHitAc','GetAtHitAc']:
  assert f not in cn,('FIX02 must not use the native damage path or write collider state',f)
 assert 'devf_cp19f2::shield_confirmed(at_hit,at_shield,at_obj,' in nm and 'cp19f2_chip_post(a,s,*d,f);' in body(nm,'void black_ai_post(daE_OC_c* a,std::uint32_t id,const devf_variant::Entry* e)noexcept{')
 # ---- B flash: Link written only in the render tint, restored in POST; the exe-verified native model set ----
 assert len(re.findall(r'link_kcolor\.[rgb]=',cn))==6 and len(re.findall(r'\blk->[\w\.\[\]]+\s*=(?!=)',cn))==0
 pre=body(nm,'HookAction on_link_model_draw(ModContext*,void* args,void*,void*)noexcept{');post=body(nm,'void after_link_model_draw(ModContext*,void* args,void*,void*)noexcept{')
 assert len(re.findall(r'link_kcolor\.[rgb]=',code(pre)))==3 and len(re.findall(r'link_kcolor\.[rgb]=cp19f2_flash_save\.[rgb]',code(post)))==3
 assert 'm==k->mpLinkModel||m==k->mpLinkFaceModel||m==k->mpLinkHatModel||m==k->mpLinkHandModel||m==k->mpDemoFCBlendModel||m==k->mpDemoFCTongueModel||m==k->mpDemoHLTmpModel||m==k->mpDemoHRTmpModel;' in nm
 assert cn.count('DEFINE_HOOK(')==2 and cn.count('hook::add_pre<')==2 and cn.count('hook::add_post<')==2 and 'DEFINE_HOOK(&daAlink_c::modelDraw,LinkModelDraw);' in nm
 assert body(nm,'ModResult install_flash_hooks()noexcept{').index('add_post<LinkModelDraw>')<body(nm,'ModResult install_flash_hooks()noexcept{').index('add_pre<LinkModelDraw>')
 assert 'kVersion="0.16.2"' in nm
 ow=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(om)));nw=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',cn));assert ow==nw,'actor write surface unchanged'
 assert len(re.findall(r'\b(?:p|k|v)->[\w\.\[\]]+\s*=(?!=)',cn))==2,'Link mCutType mask/restore stays the only other Link write'
 oldimp=json.loads((repo/'DEVF'/O/'ci/reviewed-imports.json').read_text(encoding='utf-8'));imp=json.loads((task/'ci/reviewed-imports.json').read_text(encoding='utf-8'))
 added=['?sAramHeap@JASWaveArcLoader@@2PEAVJASHeap@@EA','?getAramFreeSize@JASKernel@@YAIXZ','?sAramObject@JKRAram@@0PEAV1@EA','?getFreeSize@JKRAramHeap@@QEAAIXZ','?alloc@JKRAramHeap@@QEAAPEAVJKRAramBlock@@IW4EAllocMode@1@@Z',
  '?alloc@JKRHeap@@SAPEAXIHPEAV1@@Z','?free@JKRHeap@@SAXPEAXPEAV1@@Z','??0JASHeap@@QEAA@PEAUJASDisposer@@@Z','?initRootHeap@JASHeap@@QEAAXPEAXI@Z','?getTotalFreeSize@JASHeap@@QEBAIXZ',
  '?dComIfGp_setItemLifeCount@@YAXME@Z','?dComIfGp_getItemLifeCountType@@YAEXZ','?modelDraw@daAlink_c@@QEAAXPEAVJ3DModel@@H@Z']
 assert set(imp)==set(oldimp)
 for k in oldimp:assert sorted(imp[k])==sorted(oldimp[k]+(added if k=='dusklight.exe' else [])),k
 # ---- official source facts the design relies on ----
 official=repo/'.official';facts={}
 for rel,expected in json.loads((task/'ci/official-source-blobs.json').read_text(encoding="utf-8")).items():
  p=official/rel
  if not p.exists():continue
  raw=p.read_text(encoding='utf-8').encode();blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest();assert blob==expected,rel
  facts[rel]=dict(git_blob=blob,normalized_sha256=hashlib.sha256(raw).hexdigest())
 if official.exists():assert len(facts)==52
 if facts:
  rd=lambda rel:(official/rel).read_text(encoding="utf-8")
  scn=rd('src/Z2AudioLib/Z2SceneMgr.cpp')
  assert '            case 2:\n            case 3:\n            case 4:\n                se_wave2 = 0x5e;' in scn,'native E_OC rooms 2/3/4 load SE group 0x5e'
  assert 'bool Z2SceneMgr::loadSeWave(u32 wave) {' in scn and '            return wave_arc->load(NULL);' in scn
  wl=rd('libs/JSystem/src/JAudio2/JASWaveArcLoader.cpp')
  assert 'JASHeap* JASWaveArcLoader::getRootHeap() {\n    if (JASWaveArcLoader::sAramHeap) {\n        return JASWaveArcLoader::sAramHeap;\n    }\n    return JASKernel::getAramHeap();\n}' in wl,'root heap routing'
  assert '    if (mStatus != 0) {\n        return false;\n    }\n    if (heap == NULL) {\n        heap = JASWaveArcLoader::getRootHeap();\n    }\n    if (mHeap.alloc(heap, mFileLength) == false) {\n        return false;\n    }\n    return sendLoadCmd();' in wl,'load(NULL): status 0 then alloc on the root heap'
  bw=rd('libs/JSystem/src/JAudio2/JASBasicWaveBank.cpp')
  assert '        if (!handle->mHeap) {\n            handle->mHeap = &param_0->mHeap;' in bw and '        if (handle->mHeap == &param_0->mHeap) {\n            handle->mHeap = NULL;' in bw,'incWaveTable first-wins / decWaveTable clears: release unsafe'
  assert 'JFWSystem::setAramAudioBufSize(0xA00000);' in rd('src/m_Do/m_Do_machine.cpp') and 'config.mem2Size = 24 * 1024 * 1024;' in rd('src/m_Do/m_Do_main.cpp'),'audio ARAM fixed 10 MiB of 24 MiB'
  ar=rd('libs/JSystem/src/JKernel/JKRAram.cpp');assert 'mGraphMemorySize = (aramSize - audio_buffer_size) - aramBase;' in ar and 'JKRAramHeap(mGraphMemoryPtr, mGraphMemorySize);' in ar
  dm=rd('src/d/actor/d_a_alink_damage.inc')
  assert 'base_mag *= dusk::getSettings().game.damageMultiplier;' in dm and 'f32 magnified_dmgF = (f32)i_dmgAmount * damageMagnification(i_checkZoraMag, param_3);' in dm,'native path multiplies'
  assert '        if (var_r29->ChkTgShieldHit()) {\n            setGuardSe(var_r29);' in dm,'native shield branch reads the tg shield-hit bit'
  ccs=rd('src/d/d_cc_s.cpp');assert '        if (chk_shield) {\n            tgObjInf->OnTgShieldHit();\n        } else {' in ccs and 'tgObjInf->GetTgHitMark() == 8 && atObjInf->GetAtMtrl() != dCcD_MTRL_ICE' in ccs
  me=rd('src/d/d_meter2.cpp');assert 'new_life = dComIfGs_getLife() + dComIfGp_getItemLifeCount();' in me and '        } else if (new_life < 0) {\n            new_life = 0;' in me,'pending count applied by the meter, clamped'
  ci=rd('include/d/d_com_inf_game.h');assert '        mItemInfo.mItemLifeCount += hearts;' in ci
  ak=rd('src/d/actor/d_a_alink.cpp');assert '    } else if (mDamageTimer != 0) {\n        if (!checkMagicArmorNoDamage()) {\n            int color_timer = mDamageTimer + mDamageColorTime;' in ak and 'f32 var_f2 = fabsf(cM_ssin(color_timer * 0x800));' in ak
  assert '    g_env_light.setLightTevColorType_MAJI(i_model, &tevStr);\n\n    if (param_1 == 0) {' in ak,'modelDraw applies tevStr to the model it draws'
  ky=rd('src/d/d_kankyo.cpp');assert 'material_p->setTevKColor(0, (J3DGXColor*)&tevstr_p->TevKColor);' in ky
  hio=rd('src/d/actor/d_a_alink_HIO_data.inc');assert '    30,\n    30,\n    10,\n    3,\n    7,\n    10,\n    15,\n    0,\n    0,\n    10,\n    0,\n    0,\n    5,' in hio,'mInvincibleTime 30, damage colours 15/10/5'
 return dict(identical_runtime_files_to_fix01=identical,new_policy_file='cp19f2_policy.hpp',audio_access_only_in_gated_adapter=True,no_wave_erase=True,single_life_write=True,link_render_writes_restored=True,added_game_imports=added,official_source=facts),dict(
  cp19_fix02_audio='0.16.1 loadSeWave(0x5e) refused at status 0: by the official source the remaining cause is JASHeap::alloc on the fixed 10 MiB audio ARAM heap. FIX02 loads the E_OC family group through the engine loader into an own block of the JKRAram graph heap (sAramHeap set only around that call), never erases it (decWaveTable), no room/stage list.',
  cp19_fix02_chip='Fury blow (fury at blow start) confirmed by the engine as blocked by Link\'s shield (tg shield-hit bit + both hit-object pointers): exactly 1 life unit through the meter pending count, once per (id, generation, blow); render-only red pulse on the native damage-colour model set, restored after each model.',
  fix01='27 runtime files byte-identical to FIX01 (cp19_policy.hpp included): fixes A-G and the readiness gate unchanged; FIX01 73-case lab kept as regression on its own folder.',
  evidence='Lab: engine ports with call counters, not the game. Sound audibility, flash visibility and guard continuity need the human test.')
