"""CP19 FIX01 closed delta against CP19 (compiled c0166f0): only mod.cpp (cp19_poll/cp19_shutdown become a thin engine adapter of the gated policy, two
includes, version/header, summary fields) and cp19_policy.hpp (pure additions: readiness gate + audio pass/shutdown templates) change. Every other runtime
file is byte-identical to CP19, so fixes A-G, CP18, CP17 FIX01 and earlier code are untouched. Ordering asserts: no audio call before the gate."""
import re,json,hashlib,difflib
N='CP19_FIX01_STARTUP_AUDIO_READINESS';O='CP19_BUGFIX'
def code(s):return '\n'.join(l.split('//')[0].strip() for l in s.splitlines() if l.split('//')[0].strip())
def delta(old,new):
 rem=[];hunks=[]
 for op,a1,a2,b1,b2 in difflib.SequenceMatcher(None,old,new,autojunk=False).get_opcodes():
  if op in('replace','delete'):rem+=old[a1:a2]
  if op in('replace','insert'):hunks.append(new[b1:b2])
 return rem,hunks
def body(text,head):i=text.index(head);j=text.index('\n}\n',i)+3;return text[i:j]
def check(repo,task):
 rt=task/'runtime/DEVF';oldrt=repo/'DEVF'/O/'runtime/DEVF'
 identical=0
 for p in rt.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(rt).as_posix()
  if rel in [N+'/expansion/mod.cpp',N+'/expansion/cp19_policy.hpp']:continue
  prior=oldrt/(rel.replace(N+'/',O+'/',1) if rel.startswith(N+'/') else rel)
  assert p.read_bytes()==prior.read_bytes(),rel;identical+=1
 assert identical==26,identical
 # cp19_policy.hpp: additions only, each hunk marked FIX01 or part of the readiness block.
 op=(oldrt/O/'expansion/cp19_policy.hpp').read_text(encoding='utf-8');np_=(rt/N/'expansion/cp19_policy.hpp').read_text(encoding='utf-8')
 rem,hunks=delta(op.splitlines(),np_.splitlines());assert rem==[],('policy lines removed',rem[:3])
 assert len(hunks)==2 and all(any('FIX01' in l for l in h) for h in hunks),[h[0][:60] for h in hunks]
 # mod.cpp: removed lines are the 0.16.0 cp19_poll/cp19_shutdown bodies plus five reviewed lines.
 om=(oldrt/O/'expansion/mod.cpp').read_text(encoding='utf-8');nm=(rt/N/'expansion/mod.cpp').read_text(encoding='utf-8')
 oldfuncs=(body(om,'void cp19_poll()noexcept{')+body(om,'void cp19_shutdown()noexcept{')).splitlines()
 allowed=set(oldfuncs)|{'constexpr const char* kVersion="0.16.0";','devf_cp19::Keeper cp19_wave;unsigned cp19_passes=0,cp19_last_bokoblin=0,cp19_logs=0,cp19_summaries=0,cp19_bgm_logs=0;','struct Cp19Bgm{int dist=-1,seq=-1,status=-1;unsigned enemies=0,lasthit=0;}cp19_bgm;'}
 rem,hunks=delta(om.splitlines(),nm.splitlines())
 for l in rem:
  if l in allowed:continue
  assert ('cp19_wave.owned' in l) or l.startswith(' permits=0,permit_ended=0,permit_find=0,permit_blocked=0;}cp19_counts;') or l.startswith(' char text[900];std::snprintf(text,sizeof(text),"DEVF_CP19 event=CP19_SUMMARY') or l.startswith('  kVersion,c.wave_loads,c.wave_load_failures'),('unreviewed removed line',l[:120])
 for h in hunks:assert any(re.search(r'FIX01|cp19_audio|Cp19AudioIo|audio_gate|0\.16\.1|AudioPass|cp19_poll|cp19_shutdown|audio_ready|gate',l) for l in h),('unmarked hunk',h[0][:100])
 # Every Z2 audio access of the mod lives in Cp19AudioIo; the policy calls it only after the gate.
 i0=nm.index('struct Cp19AudioIo{');io=nm[i0:nm.index('\n};\n',i0)+4]
 outside=nm.replace(io,'')
 for tok in ['getWaveLoadStatus(','loadSeWave(','eraseSeWave(','getEnemyList(','mBattleDistState','requestSeWave_1','isSceneExist(','isResetting(']:
  assert tok in io and tok not in code(outside),('audio access outside the gated adapter',tok)
 assert code(outside).count('Z2AudioMgr::getInterface()')==0,'no direct audio-manager use outside the adapter'
 assert code(nm).count('Cp19AudioIo io;')==2 and 'devf_cp19::audio_pass(cp19_audio,io,recent,cp19_audio_emit);' in nm and 'devf_cp19::audio_shutdown(cp19_audio,io,cp19_audio_emit);' in nm
 ap=body(np_,'template<class Io,class Emit>inline AudioGate audio_pass(');sd=body(np_,'template<class Io,class Emit>inline void audio_shutdown(')
 g=ap.index('if(g!=AudioGate::Ready)return g;')
 for call in ['io.waves()','io.status(','io.load(','io.erase(','io.bgm()','io.field()','io.transition()']:assert ap.index(call)>g,('audio call before the gate',call)
 assert ap.index('audio_gate(io.flags())')<g
 sg=sd.index('if(g!=AudioGate::Ready)emit(');e=sd.index('else{')
 for call in ['io.waves()','io.status(','io.erase(']:assert sd.index(call)>e>sg,('shutdown call before the gate',call)
 f0=nm.index(' devf_cp19::AudioFlags flags()const{');fl=nm[f0:nm.index('\n }\n',f0)+4]
 for bad in ['getWaveLoadStatus','getEnemyList','requestSeWave','isSceneExist','JAUSectionHeap','sInstance']:assert bad not in fl,bad
 assert 'isInitFlag()' in fl and 'g_mDoAud_audioHeap' in fl and 'isResetFlag()' in fl and 'mDoRst::mResetData' in fl and 'f.manager&&f.init&&f.heap&&m->isResetting()' in fl
 assert 'kVersion="0.16.1"' in nm and code(nm).count('DEFINE_HOOK(')==1 and code(nm).count('hook::add_pre<')==1 and code(nm).count('hook::add_post<')==1
 ow=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(om)));nw=sorted(re.findall(r'\ba->[A-Za-z_0-9\.\[\]]+\s*=(?!=)',code(nm)));assert ow==nw,'actor write surface unchanged'
 assert len(re.findall(r'\b(?:p|k|v)->[\w\.\[\]]+\s*=(?!=)',code(nm)))==2
 oldimp=json.loads((repo/'DEVF'/O/'ci/reviewed-imports.json').read_text(encoding='utf-8'));imp=json.loads((task/'ci/reviewed-imports.json').read_text(encoding='utf-8'))
 added=['?mInitFlag@mDoAud_zelAudio_c@@2EA','?mResetFlag@mDoAud_zelAudio_c@@2EA','?g_mDoAud_audioHeap@@3PEAVJKRSolidHeap@@EA','?mResetData@mDoRst@@2PEAUmDoRstData@@EA']
 assert set(imp)==set(oldimp)
 for k in oldimp:assert sorted(imp[k])==sorted(oldimp[k]+(added if k=='dusklight.exe' else [])),k
 official=repo/'.official';facts={}
 for rel,expected in json.loads((task/'ci/official-source-blobs.json').read_text(encoding="utf-8")).items():
  p=official/rel
  if not p.exists():continue
  raw=p.read_text(encoding='utf-8').encode();blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest();assert blob==expected,rel
  facts[rel]=dict(git_blob=blob,normalized_sha256=hashlib.sha256(raw).hexdigest())
 if official.exists():assert len(facts)==33
 if facts:
  rd=lambda rel:(official/rel).read_text(encoding="utf-8")
  am=rd('src/Z2AudioLib/Z2AudioMgr.cpp');assert 'Z2AudioMgr::Z2AudioMgr() : mSoundStarter(true) {\n    mAudioMgrPtr = this;' in am and '    JAUSectionHeap* sectionHeap = JAUNewSectionHeap(true);' in am and 'bool Z2AudioMgr::hasReset() const {' in am
  ma=rd('src/m_Do/m_Do_audio.cpp')
  assert 'static mDoAud_zelAudio_c g_mDoAud_zelAudio;' in ma,'static audio object: mAudioMgrPtr is set at process start'
  assert ma.index('g_mDoAud_zelAudio.init(g_mDoAud_audioHeap, audioMemSize')<ma.index('        mDoAud_zelAudio_c::onInitFlag();'),'init (section heap) precedes the init flag'
  assert '    if (!mDoAud_zelAudio_c::isInitFlag()) {\n        if (!mDoRst::isShutdown() && !mDoRst::isReturnToMenu()) {\n            mDoAud_Create();' in ma
  assert '        if (g_mDoAud_audioHeap != NULL) {' in ma,'the init flag is raised even when the audio heap is missing: heap checked separately'
  assert '        mDoAud_zelAudio_c::onResetFlag();' in ma and '        mDoAud_zelAudio_c::offResetFlag();' in ma
  scn=rd('src/Z2AudioLib/Z2SceneMgr.cpp');assert 's32 Z2SceneMgr::getWaveLoadStatus(u32 wave, u32 bank) {\n    JAUSectionHeap* sectionHeap = JASGlobalInstance<JAUSectionHeap>::getInstance();\n    JUT_ASSERT(3001, sectionHeap);\n\n    JASWaveBank* wave_bank = sectionHeap->getWaveBankTable().getWaveBank(bank);' in scn
  gad=rd('libs/JSystem/include/JSystem/JAudio2/JASGadget.h');assert 'static T* getInstance() { return sInstance; }' in gad
  hd=rd('include/m_Do/m_Do_audio.h');assert 'static u8 isInitFlag() { return mInitFlag; }' in hd and 'DUSK_GAME_EXTERN JKRSolidHeap* g_mDoAud_audioHeap;' in hd
  rs=rd('include/m_Do/m_Do_Reset.h');assert 'static BOOL isShutdown() { return mResetData->mShutdown; }' in rs and 'static BOOL isReturnToMenu() { return mResetData->mReturnToMenu; }' in rs
 return dict(identical_runtime_files_to_cp19=identical,policy_additions_only=True,audio_access_only_in_gated_adapter=True,added_game_imports=added,official_source=facts),dict(
  cp19_fix01='0.16.0 crashed in Z2SceneMgr::getWaveLoadStatus called from cp19_poll before the JAUSectionHeap existed (Z2AudioMgr::mAudioMgrPtr is set at process start). All audio access now sits behind audio_gate (mDoAud init flag, audio heap, no reset, no shutdown/return-to-menu); shutdown idempotent and gated.',
  cp19='26 runtime files byte-identical to CP19: fixes A-G unchanged; CP19 65-case lab kept as regression on its own folder.',
  evidence='Lab: fake engine with the source audio lifecycle and exact call counters; not the game. Human boot pending.')
