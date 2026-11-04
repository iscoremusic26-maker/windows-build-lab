"""Executed FIX02 mutants: valid positive controls first; compile errors and
timeouts are failures, never evidence of a behavioral kill."""
from pathlib import Path
import os,subprocess,tempfile,shutil,json,sys,concurrent.futures
root=Path(__file__).resolve().parents[1]
cases=[
 ('terminal_midclip_accepted','src/cp05_fix02_core.hpp','a.anim_frame>=float(end)-.01f','a.anim_frame>=0','core'),
 ('terminal_stop_ignored','src/cp05_fix02_core.hpp','!stopped||a.anim_rate!=0','false||a.anim_rate!=0','core'),
 ('terminal_recoil_positive_frame','src/cp05_fix02_core.hpp','a.anim_frame<=.01f','a.anim_frame<=14.f','core'),
 ('terminal_unknown_BCK_accepted','src/cp05_fix02_core.hpp','bck5==bck6','false','core'),
 ('terminal_changed_animation_accepted','src/cp05_fix02_core.hpp','!post05::same_anim(a,b)','false','core'),
 ('terminal_Link_origin_ignored','src/cp05_fix02_core.hpp','return from_link&&','return true&&','native'),
 ('terminal_generation_ignored','src/cp05_fix02_adapter.inc','rep.generation!=g.cur.generation','false','native'),
 ('terminal_live_frame_ignored','src/cp05_fix02_adapter.inc','a->mpMorf->getFrame()!=r.aux.anim_frame','false','native'),
 ('terminal_live_rate_ignored','src/cp05_fix02_adapter.inc','a->mpMorf->getPlaySpeed()!=r.aux.anim_rate','false','native'),
 ('terminal_pause_ignored','src/cp05_fix02_adapter.inc','||!cp04_world_safe()||','||false||','native'),
 ('terminal_lethal_accepted','src/cp05_fix02_core.hpp','&&!r.j.lethal','&&true','native'),
 ('ordinary_guard_cleanup_omitted','src/cp05_fix02_adapter.inc','void fix02_guard_contact()noexcept{','void fix02_guard_contact()noexcept{if(g.cur.pv.result==post08::Result::NormalGuard)return;','guard'),
 ('stale_contact_cleanup_allowed','src/cp05_fix02_core.hpp','return p.fresh&&','return true&&','core'),
 ('foreign_contact_cleanup_allowed','src/cp05_fix02_core.hpp','p.at_hit[0]&&p.at_on_link[0]','p.at_hit[0]','core'),
 ('duplicate_contact_cleanup_allowed','src/cp05_fix02_core.hpp','&&v.first_contact&&','&&true&&','core'),
 ('cc_set_registration_lease_not_restored','src/mod.cpp','fix01_at_mask(a,fx.cc_added,false);','(void)fx.cc_added;','guard'),
 ('native_registers_inactive_AT','tests/reference/fix02_native_cc_register.inc','if (obj->ChkAtSet())','if (true)','guard'),
 ('engine_clear_omitted','tests/reference/fix02_native_cc_clear.inc','(*obj)->GetGObjInf()->ClrAtHit();','(void)obj;','guard'),
 ('guard_duplicate_sample_advances','src/cp05_fix02_monitor.hpp','if(r.last_tick==post.tick)','if(false)','core'),
 ('guard_early_rebound_unobserved','src/cp05_fix02_monitor.hpp','pre.stale_link_hit&&post.mode==4','false&&post.mode==4','core'),
 ('guard_R_release_never_ends','src/cp05_fix02_monitor.hpp','if(!post.held)','if(false)','core'),
 ('guard_context_inherits_old_episode','src/cp05_fix02_monitor.hpp','if(r.bound&&!(r.key==post.key))','if(false)','core'),
 ('guard_culling_report_lost','src/cp05_fix02_monitor.hpp','tick>r.last_tick+60','tick>r.last_tick+6000','core'),
]
for name,rel,a,b,lab in cases:assert (root/rel).read_text(encoding='utf-8').count(a)==1,name
def run(p,lab):
 if lab=='core':
  exe=p/('core.exe' if os.name=='nt' else 'core');f=p/'tests/cp05_fix02_lab.cpp';cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(f),'/Fe'+str(exe),'/Fo'+str(p/'core.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(f),'-o',str(exe)]
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=120)
  if c.returncode:return False,'COMPILE_ERROR '+c.stdout+c.stderr
  c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
 else:c=subprocess.run([sys.executable,'-I',str(p/'tests'/('test_cp05_fix02_'+lab+'.py'))],capture_output=True,text=True,timeout=300)
 return c.returncode==0,c.stdout+c.stderr
def clone(p):
 for x in ['src','tests']:shutil.copytree(root/x,p/x,ignore=shutil.ignore_patterns('__pycache__'))
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p)
 for lab in ['core','native','guard']:
  ok,out=run(p,lab);assert ok and 'RESULT=LAB_PASS' in out,(lab,out[-1800:])
def one(case):
 name,rel,a,b,lab=case
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/rel;f.write_text(f.read_text(encoding='utf-8').replace(a,b,1),encoding='utf-8')
  try:
   ok,out=run(p,lab);killed=not ok and ('FIX02_FAIL ' in out or 'CP05_ADAPTER_FAIL ' in out)
   return {'mutation':name,'detected':killed,'timeout':False,'status':'KILLED' if killed else 'FAIL','failure_tail':None if killed else out[-1800:]}
  except subprocess.TimeoutExpired:return {'mutation':name,'detected':False,'timeout':True,'status':'FAIL'}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:rows=list(pool.map(one,cases))
print(json.dumps({'count':len(rows),'detected':sum(r['detected'] for r in rows),'timeouts':sum(r['timeout'] for r in rows),'positive_controls':'ACTUAL_TERMINAL_PROOF_NATIVE_TRANSACTION_NATIVE_COLLISION_LEASE_PASS','mutations':rows},indent=2));sys.exit(not all(r['detected'] for r in rows))
