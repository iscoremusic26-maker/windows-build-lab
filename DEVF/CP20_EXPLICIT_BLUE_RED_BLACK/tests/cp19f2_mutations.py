"""CP19 FIX02 mutation check: each mutant of cp19f2_policy.hpp (and of the FIX01 gate it reuses) must make cp19f2_lab fail. Local evidence (g++), not a CI gate.
Usage: python3 -I tests/cp19f2_mutations.py <work-dir>   (from the CP19 FIX02 folder)"""
import shutil,subprocess,sys
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];WORK=Path(sys.argv[1]).resolve()
X='runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/'
FILES=[X+'cp19f2_policy.hpp',X+'cp19_policy.hpp',X+'red_ai_policy.hpp',X+'black_ai_policy.hpp','runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp']
F,C=X+'cp19f2_policy.hpp',X+'cp19_policy.hpp'
M=[
 # ---- A: universal residency ----
 (F,'a_resident_before_gate','\n if(g!=AudioGate::Ready)return g;\n resident_pass(st.res,io,need,emit);','\n resident_pass(st.res,io,need,emit);\n if(g!=AudioGate::Ready)return g;'),
 (F,'a_claims_scene_group','  if(!r.foreign_logged){r.foreign_logged=true;++r.foreign;','  if(r.st!=Res::Loading){r.st=Res::Ready;r.used=used;}\n  if(!r.foreign_logged){r.foreign_logged=true;++r.foreign;'),
 (F,'a_ready_on_request',' if(ok){r.st=Res::Loading;',' if(ok){emit("AUDIO_LOAD_READY","X",0,0,0,0);r.st=Res::Loading;'),
 (F,'a_loads_during_transition',' if(io.transition()||r.passes<r.retry_at)return;',' if(r.passes<r.retry_at)return;'),
 (F,'a_no_retry_backoff',' ++r.refused;r.retry_at=r.passes+kRetryPasses;emit("AUDIO_LOAD_REFUSED"',' ++r.refused;r.retry_at=r.passes;emit("AUDIO_LOAD_REFUSED"'),
 (F,'a_never_gives_up','\n if(r.attempts>=kMaxAttempts){r.st=Res::GaveUp;emit("AUDIO_LOAD_GAVE_UP","THREE_REFUSALS_NO_FURTHER_ATTEMPT_THIS_SESSION",kOcFamilyWave,int(r.attempts),0,0);}\n}','\n}'),
 (F,'a_reservation_keep_ignored','constexpr std::uint32_t kGraphKeep=4u<<20;','constexpr std::uint32_t kGraphKeep=0;'),
 (F,'a_wrong_family_wave','constexpr std::uint8_t kOcFamilyWave=0x5e;','constexpr std::uint8_t kOcFamilyWave=0x60;'),
 (F,'a_interferes_with_scene_loading','  return;   // our load in flight, or the scene loading the group itself: never interfere','  if(!own){const char* w="";io.load_own(kOcFamilyWave,w);}return;'),
 (F,'a_need_ignored',' if(!need||r.st==Res::GaveUp)return;',' if(r.st==Res::GaveUp)return;'),
 (F,'a_lost_not_detected',' if(r.st==Res::Ready&&!own){',' if(false){'),
 (F,'a_slow_load_logged_every_pass','&&!r.slow_logged&&r.passes-r.loading_since>=kSlowLoadPasses){r.slow_logged=true;','&&r.passes-r.loading_since>=kSlowLoadPasses){'),
 (F,'a_shutdown_forgets_nothing',' st=AudioPass{};\n}',' \n}'),
 (F,'a_foreign_line_every_pass',' if(!r.foreign_logged){r.foreign_logged=true;++r.foreign;',' if(true){r.foreign_logged=true;++r.foreign;'),
 (F,'a_native_cause_inverted','gf<used?"GAME_AUDIO_ARAM_LARGEST_FREE_BLOCK_SMALLER_THAN_THE_GROUP"','gf>=used?"GAME_AUDIO_ARAM_LARGEST_FREE_BLOCK_SMALLER_THAN_THE_GROUP"'),
 (C,'f1_gate_ignores_init',' if(!f.init)return AudioGate::NotInitialized;',''),
 (C,'f1_gate_ignores_audio_heap',' if(!f.heap)return AudioGate::NoAudioHeap;',''),
 (C,'f1_gate_lines_every_pass',' if(g.known&&g.last==now)return nullptr;',' if(false)return nullptr;'),
 # ---- B: chip ----
 (F,'b_no_fury_check','  else if(!b.fury)op=ChipOp::NotFury;\n',''),
 (F,'b_no_duplicate_check','  else if(b.charged)op=ChipOp::Duplicate;\n',''),
 (F,'b_no_registration_check','  else if(!b.registered)op=ChipOp::NotRegistered;\n',''),
 (F,'b_judged_after_sequence_change',' ChipOp op=ChipOp::None;\n if(v.shield&&v.live){',' ChipOp op=ChipOp::None;\n if(v.seq!=b.seq){b.seq=v.seq;b.fury=v.fury;b.registered=false;b.charged=false;}\n if(v.shield&&v.live){'),
 (F,'b_fury_read_every_post',' if(v.seq!=b.seq){b.seq=v.seq;b.fury=v.fury;b.registered=false;b.charged=false;}\n if(v.at_set',' if(v.seq!=b.seq){b.seq=v.seq;b.registered=false;b.charged=false;}\n b.fury=v.fury;\n if(v.at_set'),
 (F,'b_generation_inherits',' if(b.id!=v.id||b.gen!=v.gen){b=ChipBlow{};b.id=v.id;b.gen=v.gen;}',' if(b.id!=v.id){b=ChipBlow{};b.id=v.id;b.gen=v.gen;}'),
 (F,'b_dead_black_charges',' if(v.shield&&v.live){',' if(v.shield){'),
 (F,'b_two_units','constexpr int kChipLifeUnits=1;','constexpr int kChipLifeUnits=2;'),
 (F,'b_meter_negative','if(n<0)n=0;return n;}','return n;}'),
 (F,'b_at_shield_alone','return at_hit&&at_shield&&at_obj!=nullptr&&at_obj==cyl&&tg_hit&&tg_shield&&','return at_hit&&at_shield&&at_obj!=nullptr&&at_obj==cyl&&tg_hit&&'),
 (F,'b_tg_object_not_checked','tg_hit&&tg_shield&&tg_obj!=nullptr&&tg_obj==sph;','tg_hit&&tg_shield;'),
 (F,'b_at_object_not_checked','return at_hit&&at_shield&&at_obj!=nullptr&&at_obj==cyl&&','return at_hit&&at_shield&&'),
 # ---- B: flash ----
 (F,'b_flash_wrong_colour_time','f.col=32-(kFlashDamageTimer%16);','f.col=32;'),
 (F,'b_flash_lowers_native','c.r=native.r>own.r?native.r:own.r;','c.r=own.r;'),
 (F,'b_flash_never_ends','inline void flash_tick(Flash&f)noexcept{if(f.dmg>0){++f.ticks;if(f.col>0)--f.col;--f.dmg;}}','inline void flash_tick(Flash&f)noexcept{if(f.dmg>0){++f.ticks;if(f.col>0)--f.col;}}'),
 (F,'b_flash_wrong_peak','constexpr std::uint8_t kFlashR0=15,','constexpr std::uint8_t kFlashR0=255,'),
 (F,'b_flash_green_channel','c.r=std::uint8_t(float(base)*kAbsSin16[unsigned(t)&15u]);return c;','c.r=std::uint8_t(float(base)*kAbsSin16[unsigned(t)&15u]);c.g=c.r;return c;'),
]
results=[]
for path,name,a,b in M:
 base=(TASK/path).read_text();assert base.count(a)==1,(name,base.count(a))
 d=WORK/name;shutil.rmtree(d,ignore_errors=True);(d/'tests').mkdir(parents=True)
 for rel in FILES:(d/rel).parent.mkdir(parents=True,exist_ok=True);(d/rel).write_text((TASK/rel).read_text().replace(a,b) if rel==path else (TASK/rel).read_text())
 shutil.copy(TASK/'tests/cp19f2_lab.cpp',d/'tests/cp19f2_lab.cpp')
 c=subprocess.run(['g++','-std=c++20','-O1','-w',str(d/'tests/cp19f2_lab.cpp'),'-o',str(d/'lab')],capture_output=True,text=True)
 if c.returncode:results.append((name,'COMPILE_ERROR',c.stderr[-300:]));continue
 try:r=subprocess.run([str(d/'lab')],capture_output=True,text=True,timeout=60)
 except subprocess.TimeoutExpired:results.append((name,'KILLED','TIMEOUT'));continue
 results.append((name,'KILLED' if r.returncode else 'SURVIVED',(r.stderr.strip().splitlines() or [''])[-1]))
for n,s,w in results:print(f'MUTANT {n} {s} {w}')
killed=sum(s=='KILLED' for _,s,_ in results);print(f'CP19F2_MUTATIONS killed={killed} total={len(results)}')
sys.exit(0 if killed==len(results) else 1)
