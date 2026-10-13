"""Seven pilot-specific semantic mutants: compile and fail assertions, never count crashes."""
import shutil,subprocess
MUTANTS={
 'omit_XZ_guard':('src/policy.cpp','if(d.link.xz<link_min_xz)','if(false)'),
 'use_d3_instead_of_XZ':('src/policy.cpp','if(d.link.xz<link_min_xz)','if(d.link.d3<link_min_xz)'),
 'allow_session_without_permission':('overlay/src/experiment_core.cpp','if(!permission.location_admitted||!permission.session_authorized)','if(!permission.location_admitted)'),
 'admit_truncated_census':('src/policy.cpp','!v.actors_complete||v.actors_truncated||v.duplicates','!v.actors_complete||v.duplicates'),
 'omit_final_C_guard':('overlay/src/experiment_core.cpp','else if(!pilot_check(selected,&fresh))','else if(false)'),
 'critical_logs_share_normal_budget':('src/trace.cpp','case Event::ReadyMonitorBudgetExhausted:return 1;','case Event::ReadyMonitorBudgetExhausted:return 0;'),
 'relax_single_attempt_latch':('overlay/src/experiment_core.cpp','if(consumed_||terminal()||state_==State::ActorReadyObserved)return;','if(terminal()||state_==State::ActorReadyObserved)return;')
}
def run_mutations(project,overlay,work,evidence):
 results=[]
 for name,(rel,before,after) in MUTANTS.items():
  folder=work/'mutants'/name;folder.mkdir(parents=True,exist_ok=True)
  for sub in ['src','tests']:shutil.copytree(project/sub,folder/sub,dirs_exist_ok=True)
  shutil.copytree(overlay,folder/'overlay',dirs_exist_ok=True)
  f=folder/rel;s=f.read_text()
  if s.count(before)!=1:raise RuntimeError('Unique mutation anchor missing: '+name)
  f.write_text(s.replace(before,after));exe=folder/'mutant.exe'
  cmd=['cl','/nologo','/O2','/std:c++20','/EHsc','/DDEVF_FIRST_OFFLINE=1','/I'+str(folder/'src'),'/I'+str(folder/'overlay/src'),'/I'+str(folder/'overlay/tests'),str(folder/'tests/pilot_lab.cpp'),str(folder/'overlay/src/experiment_core.cpp'),str(folder/'overlay/src/admission.cpp'),str(folder/'src/policy.cpp'),str(folder/'src/trace.cpp'),str(folder/'src/census_values.cpp'),'/Fe:'+str(exe),'/link','/STACK:8388608']
  with (evidence/f'mutant-{name}-build.log').open('w') as log:cp=subprocess.run(cmd,cwd=folder,stdout=log,stderr=subprocess.STDOUT)
  if cp.returncode:raise RuntimeError('Mutant must compile: '+name)
  cp=subprocess.run([str(exe)],capture_output=True,text=True)
  (evidence/f'mutant-{name}.log').write_text(cp.stdout+cp.stderr)
  if cp.returncode!=1 or 'FAIL ' not in cp.stdout:raise RuntimeError('Mutant survived or crashed: '+name)
  results.append({'mutation':name,'compiled':True,'killed':True,'exit':cp.returncode,'failed_checks':[s for s in cp.stdout.splitlines() if s.startswith('FAIL ')]})
 return results
