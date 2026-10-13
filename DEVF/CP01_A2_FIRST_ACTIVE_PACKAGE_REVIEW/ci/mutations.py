"""Only four integration mutants. Reuse historical fake and new provider checks."""
import subprocess,shutil
MUTANTS={
 'provider_accidentally_NONE':('src/active_admission.cpp','a.location_admitted=true; a.session_authorized=true;','a.location_admitted=false; a.session_authorized=false;'),
 'threshold_relaxed':('policy.cpp','link_min_xz','link_min_xz'),
 'C_check_omitted':('experiment_core.cpp','else if(!pilot_check(selected,&fresh))','else if(false)'),
 'multi_request_latch':('experiment_core.cpp','if(consumed_||terminal()||state_==State::ActorReadyObserved)return;','if(terminal()||state_==State::ActorReadyObserved)return;')}
def run(project,pilot,overlay,trace,work,evidence,source):
 results=[]
 for name,(rel,before,after) in MUTANTS.items():
  d=work/'mutants'/name;d.mkdir(parents=True,exist_ok=True)
  f=project/rel if rel.startswith('src/') else pilot/'src'/rel if rel=='policy.cpp' else overlay/'src'/rel
  text=f.read_text()
  if name=='threshold_relaxed':
   # The production threshold is in the shared policy_values header; override through a generated private copy.
   f=pilot/'src/policy_values.hpp';text=f.read_text();before='link_min_xz=1600.0';after='link_min_xz=1200.0'
  if text.count(before)!=1:raise RuntimeError('Mutation anchor '+name)
  changed=d/f.name;changed.write_text(text.replace(before,after))
  # Private copies only where a quoted include would otherwise select the original header.
  for x in ['policy.hpp','policy_values.hpp','census_values.hpp']:
   if not (d/x).exists():shutil.copyfile(pilot/'src'/x,d/x)
  shutil.copyfile(trace/'trace.hpp',d/'trace.hpp')
  policy=d/'policy.cpp'
  if not policy.exists():shutil.copyfile(pilot/'src/policy.cpp',policy)
  core=changed if rel=='experiment_core.cpp' else overlay/'src/experiment_core.cpp'
  provider=changed if rel=='src/active_admission.cpp' else project/'src/active_admission.cpp'
  exe=d/'mutant.exe'
  cmd=['cl','/nologo','/O2','/std:c++20','/EHsc','/DDEVF_FIRST_OFFLINE=1',*['/I'+str(x) for x in [d,trace,overlay/'src',overlay/'tests',project/'src',pilot/'src',source/'sdk/include']],str(project/'tests/provider_lab.cpp'),str(core),str(provider),str(policy),str(trace/'trace.cpp'),str(pilot/'src/census_values.cpp'),'/Fe:'+str(exe),'/link','/STACK:8388608']
  with (evidence/f'mutant-{name}-build.log').open('w') as log:cp=subprocess.run(cmd,cwd=d,stdout=log,stderr=subprocess.STDOUT)
  if cp.returncode:raise RuntimeError('Mutant must compile '+name)
  cp=subprocess.run([str(exe)],capture_output=True,text=True);(evidence/f'mutant-{name}.log').write_text(cp.stdout+cp.stderr)
  if cp.returncode!=1 or 'FAIL ' not in cp.stdout:raise RuntimeError('Mutant survived/crashed '+name)
  results.append({'mutation':name,'compiled':True,'killed':True,'exit':1,'failed_checks':[s for s in cp.stdout.splitlines() if s.startswith('FAIL ')]})
 return results
