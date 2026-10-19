"""CP19 FIX01 mutation check: each mutant of the CP19/FIX01 policies must make cp19f1_lab fail. Local evidence (g++), not a CI gate.
Usage: python3 -I tests/cp19f1_mutations.py <work-dir>   (from the CP19 folder)"""
import shutil,subprocess,sys
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];WORK=Path(sys.argv[1]).resolve()
X='runtime/DEVF/CP19_FIX01_STARTUP_AUDIO_READINESS/expansion/'
FILES=[X+'cp19_policy.hpp',X+'red_ai_policy.hpp',X+'black_ai_policy.hpp',X+'blue_recruit_policy.hpp',X+'blue_ai_policy.hpp','runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp',
 'runtime/DEVF/CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp','runtime/DEVF/CP10_5_DAMAGE_BLUE_RED_BLACK/expansion/damage_policy.hpp']
C,R,B,Q=X+'cp19_policy.hpp',X+'red_ai_policy.hpp',X+'black_ai_policy.hpp',X+'blue_recruit_policy.hpp'
M=[(R,'strike_distance_400','constexpr double kStrike=240.,','constexpr double kStrike=400.,'),
 (R,'red_close_in_removed',' if(close_band(dist)&&a.mode==Find&&a.state==4&&s.prev_find4',' if(false&&close_band(dist)&&a.mode==Find&&a.state==4&&s.prev_find4'),
 (R,'red_force_back_at_400',' if(dist<=kStrike&&!force_block(s,a,l,kForceFacing,true)){',' if(dist<kForceDist&&!force_block(s,a,l,kForceFacing,true)){'),
 (R,'close_speed_not_given_back','{p.speed=true;p.speed_value=c.v0;','{p.speed=false;p.speed_value=c.v0;'),
 (R,'close_lease_not_given_back',' if(c.lease&&lockout==1)p.lockout=true;',''),
 (B,'black_strike_ignores_distance','  if(dist<=ra::kStrike&&dy<=kChainHeight&&gap<kChainFacing&&!a.wall&&a.los&&(leash==0||hunting)){','  if(dy<=kChainHeight&&gap<kChainFacing&&!a.wall&&a.los&&(leash==0||hunting)){'),
 (B,'black_chain_back_at_400','   if(dist<=ra::kStrike&&dy<=kChainHeight&&gap<kChainFacing&&!a.wall&&a.los){','   if(dist<kChainReach&&dy<=kChainHeight&&gap<kChainFacing&&!a.wall&&a.los){'),
 (B,'black_close_in_removed','  if(ra::close_band(dist)&&prev==Find&&','  if(false&&ra::close_band(dist)&&prev==Find&&'),
 (C,'wave_erase_during_load','  if(status!=2){c.why="ERASE_DEFERRED_WHILE_LOADING";return c;}',''),
 (C,'wave_erases_scene_requested','  if(want||scene_requests(w,kOcWave))return c;','  if(want)return c;'),
 (C,'wave_ignores_transition',' const bool want=field_stage&&!transition&&',' const bool want=field_stage&&'),
 (C,'wave_no_retry_backoff','k.retry_after=k.ticks+kRetry;','k.retry_after=k.ticks;'),
 (C,'wave_loads_without_bokoblin',' const bool need_now=field_stage&&!transition&&field_without_oc(w)&&bokoblin;',' const bool need_now=field_stage&&!transition&&field_without_oc(w);'),
 (C,'attention_at_one_hp','return flags==0&&health>=2&&','return flags==0&&health>=1&&'),
 (C,'attention_during_flight','return flags==0&&health>=2&&live_mode(mode)&&','return flags==0&&health>=2&&(live_mode(mode)||mode==BigDamage)&&'),
 (C,'zombie_overrides_native_death','inline bool complete_zombie(int health,int mode,bool hidden,bool deleting)noexcept{return health<=0&&live_mode(mode)&&','inline bool complete_zombie(int health,int mode,bool hidden,bool deleting)noexcept{return health<=0&&mode!=Death&&'),
 (C,'residual_cleared_in_damage','if(a3==0||health<1||!live_mode(mode)||mode==Damage){','if(a3==0||health<1||!live_mode(mode)){'),
 (C,'helm_mask_missing',' h.mask=true;h.pre_health=v.health;',' h.mask=false;h.pre_health=v.health;'),
 (C,'helm_heals',' if(target<health){p.write=true;p.health=target;}else p.health=health;',' p.write=true;p.health=target;'),
 (C,'helm_values_swapped','kHelmMaster=100,kHelmOrdon=50;','kHelmMaster=50,kHelmOrdon=100;'),
 (C,'helm_duplicate_applies',' if(h.episode){++h.duplicates;',' if(false){++h.duplicates;'),
 (C,'helm_restores_foreign',' p.unmask=same_link&&cut_now==kCutTurnRight;',' p.unmask=true;'),
 (C,'helm_masks_under_invulnerability','||v.cutdown||v.invuln>1||','||v.cutdown||'),
 (C,'helm_lethal_masked',' if(v.health<=d){++h.lethal;',' if(false){++h.lethal;'),
 (C,'permit_never_expires',' if(p.caller_dead&&tick-p.last_combat>=kPermitCalm)return end(',' if(false)return end('),
 (C,'permit_expires_with_caller_alive',' if(!v.caller_alive&&!p.caller_dead){p.caller_dead=true;',' if(!p.caller_dead){p.caller_dead=true;'),
 (C,'permit_overrides_black_hunt',' if(v.hunting_black||!v.safe||',' if(!v.safe||'),
 (C,'permit_acts_without_leash','||!v.link_same_room||v.leash==0)return c;','||!v.link_same_room)return c;'),
 (C,'permit_no_bounce_limit','  if(p.bounce>=kPermitBounceLimit){','  if(false){'),
 (C,'f1_gate_removed','\n if(g!=AudioGate::Ready)return g;\n','\n'),
 (C,'f1_gate_ignores_init',' if(!f.init)return AudioGate::NotInitialized;',''),
 (C,'f1_gate_ignores_audio_heap',' if(!f.heap)return AudioGate::NoAudioHeap;',''),
 (C,'f1_gate_ignores_reset',' if(f.reset_flag||f.resetting)return AudioGate::AudioResetting;',''),
 (C,'f1_gate_ignores_shutdown',' if(f.shutdown)return AudioGate::GameShutdown;',''),
 (C,'f1_gate_ignores_return_to_menu',' if(f.return_to_menu)return AudioGate::ReturnToMenu;',''),
 (C,'f1_gate_ignores_reset_data',' if(!f.reset_data)return AudioGate::NoResetData;',''),
 (C,'f1_shutdown_ungated','  if(g!=AudioGate::Ready)emit("FIELD_SE_WAVE_SHUTDOWN_RELEASE_SKIPPED"','  if(false)emit("FIELD_SE_WAVE_SHUTDOWN_RELEASE_SKIPPED"'),
 (C,'f1_shutdown_erases_while_loading','   if(status==2&&!scene_requests(w,kOcWave)){const bool ok=io.erase(kOcWave);','   if(status!=0&&!scene_requests(w,kOcWave)){const bool ok=io.erase(kOcWave);'),
 (C,'f1_gate_lines_unbounded',' if(g.logs>=kGateLogs)return nullptr;++g.logs;',' ++g.logs;'),
 (C,'f1_gate_logs_every_pass',' if(g.known&&g.last==now)return nullptr;',' if(false)return nullptr;'),
 (Q,'receiver_leash_refusal_back',' if(f.hit||!f.ready)return Skip::Unsafe;\n',' if(f.hit||!f.ready)return Skip::Unsafe;\n if(!leash(f,s.f.pos))return Skip::NativeLeash;\n')]
results=[]
for path,name,a,b in M:
 base=(TASK/path).read_text();assert base.count(a)==1,(name,base.count(a))
 d=WORK/name;shutil.rmtree(d,ignore_errors=True);(d/'tests').mkdir(parents=True)
 for rel in FILES:(d/rel).parent.mkdir(parents=True,exist_ok=True);(d/rel).write_text((TASK/rel).read_text().replace(a,b) if rel==path else (TASK/rel).read_text())
 shutil.copy(TASK/'tests/cp19f1_lab.cpp',d/'tests/cp19f1_lab.cpp')
 c=subprocess.run(['g++','-std=c++20','-O1','-w',str(d/'tests/cp19f1_lab.cpp'),'-o',str(d/'lab')],capture_output=True,text=True)
 if c.returncode:results.append((name,'COMPILE_ERROR',c.stderr[-300:]));continue
 r=subprocess.run([str(d/'lab')],capture_output=True,text=True,timeout=180)
 results.append((name,'KILLED' if r.returncode else 'SURVIVED',(r.stderr.strip().splitlines() or [''])[-1]))
for n,s,w in results:print(f'MUTANT {n} {s} {w}')
killed=sum(s=='KILLED' for _,s,_ in results);print(f'CP19F1_MUTATIONS killed={killed} total={len(results)}')
sys.exit(0 if killed==len(results) else 1)
