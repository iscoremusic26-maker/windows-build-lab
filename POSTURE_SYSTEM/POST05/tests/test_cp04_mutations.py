"""Behavior mutations of actual compiled CP04 core and complete callbacks.
Only an executed assertion failure kills; compile errors/timeouts are FAIL.
"""
from pathlib import Path
import subprocess,tempfile,shutil,sys,os,json,concurrent.futures
root=Path(__file__).resolve().parents[1]
cases=[
 ('chance75','src/cp04_combo_core.hpp','mix(rng)>>63','mix(rng)>>62','core'),
 ('three_swings','src/cp04_combo_core.hpp','kSwings=4','kSwings=3','core'),
 ('same_execution_speed','src/cp04_combo_core.hpp','kFastRate=1.5f','kFastRate=1.f','core'),
 ('short_is_moderate','src/cp04_combo_core.hpp','kShortGap=2','kShortGap=12','core'),
 ('moderate_gap_lost','src/cp04_combo_core.hpp','kModerateGap=12','kModerateGap=2','core'),
 ('reroll_same_execute','src/cp04_combo_core.hpp','||stamp==last_decision){','||false){','core'),
 ('reuse_swing_token','src/cp04_combo_core.hpp','++swing;++token;','++swing;','core'),
 ('allow_foreign_rate','src/cp04_combo_core.hpp','!normal_rate(observed_rate)','false','core'),
 ('ignore_combo_break','src/cp04_combo_core.hpp','if(!active())return;if(phase','if(broken)return;if(!active())return;if(phase','core'),
 ('motion_not_advanced','src/shield_chain_core.hpp','kVisibleAdvance=1','kVisibleAdvance=0','core'),
 ('input_without_animation','src/shield_chain_core.hpp','!real_clip||!input_r','false||!input_r','core'),
 ('one_action_two_parries','src/shield_chain_core.hpp','if(e.consumed){','if(false){','core'),
 ('new_input_reused','src/shield_chain_core.hpp','||input_serial==last_used_input','||false','core'),
 ('buffer_forever','src/shield_chain_core.hpp','kBufferTicks=6','kBufferTicks=60','core'),
 ('motion_foreign_speed','src/shield_chain_core.hpp','pass_valid=rate==1.f&&','pass_valid=true&&','core'),
 ('native_start_unsafe','src/shield_chain_core.hpp','if(!safe||!real_clip','if(false||!real_clip','core'),
 ('restart_in_unsafe_priority','src/shield_chain_core.hpp','if(!safe){cancel();return false;}','if(false){cancel();return false;}','core'),
 ('shield_serial_not_new','src/shield_chain_core.hpp','++native_serial;','native_serial=1;','core'),
 ('old_pass_rearms_unconfirmed_new_clip','src/shield_chain_core.hpp','armed=pass_serial==native_serial','armed=true','core'),
 ('old_post08_shield_serial','src/parry_core.hpp','void native_start() noexcept { native_restart_=true; }','void native_start() noexcept {}','core'),
 ('production_expansion_race','src/cp04_adapter.inc','late.priority=-100','late.priority=0','adapter'),
 ('production_posture100_veto_lost','src/cp04_adapter.inc','p.dead||p.latched||a->health','p.dead||a->health','adapter'),
 ('production_skip_native_shield_init','src/cp04_adapter.inc','(void)link->procGuardAttackInit();','(void)link;','adapter'),
 ('production_motion_provenance_lost','src/cp04_adapter.inc','bool cp04_guard_pass()noexcept{return !c4.enabled||c4.shield.permits(g.p8.tick);}','bool cp04_guard_pass()noexcept{return true;}','adapter'),
 ('production_parry_receipt_lost','src/cp04_adapter.inc','if(!c4.shield.confirm(g.p8.tick))','if(true)','adapter'),
 ('production_recoil_cancelled','src/cp04_adapter.inc','&&a->mOcState!=3)','&&true)','adapter'),
 ('production_ignore_event','src/cp04_adapter.inc','!g_dComIfG_gameInfo.play.getEvent()->runCheck()','true','adapter'),
 ('production_gap_not_applied','src/cp04_adapter.inc','a->field_0x6c2=static_cast<s16>(ticks);','a->field_0x6c2=20;','adapter'),
 ('production_cancel_native_braking','src/cp04_adapter.inc','if(s->launch_requested&&a->mActionMode==4&&a->mOcState==0)return;','if(false)return;','adapter'),
 ('production_native_rate_not_applied','src/cp04_adapter.inc','a->mpMorf->setPlaySpeed(s.rate);','(void)s.rate;','adapter'),
]
for n,rel,old,new,lab in cases:assert old in (root/rel).read_text(encoding='utf-8'),n
def core(p):
 exe=p/('cp04.exe' if os.name=='nt' else 'cp04');src=p/'tests/cp04_lab.cpp'
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(src),'/Fe'+str(exe),'/Fo'+str(p/'cp04.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(src),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=120)
 if c.returncode:return False,'COMPILE_ERROR'
 c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30);return c.returncode==0,c.stdout+c.stderr
with tempfile.TemporaryDirectory() as td:
 p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');ok,out=core(p);assert ok and 'LAB_PASS' in out,'positive core required'
c=subprocess.run([sys.executable,'-I',str(root/'tests/test_cp04_adapter.py')],capture_output=True,text=True,timeout=180);assert c.returncode==0,c.stdout+c.stderr
def one(case):
 name,rel,old,new,lab=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');f=p/rel;f.write_text(f.read_text(encoding='utf-8').replace(old,new,1),encoding='utf-8')
  try:
   if lab=='core':ok,out=core(p);killed=not ok and 'FAIL line' in out
   else:
    c=subprocess.run([sys.executable,'-I',str(p/'tests/test_cp04_adapter.py')],capture_output=True,text=True,timeout=180);out=c.stdout+c.stderr;killed=c.returncode!=0 and 'CP04_ADAPTER_FAIL' in out
   return {'mutation':name,'status':'KILLED' if killed else 'FAIL','timeout':False}
  except subprocess.TimeoutExpired:return {'mutation':name,'status':'FAIL','timeout':True}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:results=list(pool.map(one,cases))
print(json.dumps({'positive_controls':'PASS','count':len(results),'detected':sum(x['status']=='KILLED' for x in results),'timeouts':sum(x['timeout'] for x in results),'mutations':results},indent=2));sys.exit(any(x['status']!='KILLED' for x in results))
