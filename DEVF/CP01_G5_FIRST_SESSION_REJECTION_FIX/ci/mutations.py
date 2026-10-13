import subprocess,shutil
MUTANTS={
 'any_invalid_room':('policy.cpp','return RoomScope::Invalid;','return RoomScope::Ordinary;'),
 'ky_without_owner_distance':('policy.cpp','if(a.room==-1&&a.profile==686&&a.group==0&&!a.is_link&&a.owner_valid&&a.owner==o.parent&&','if(a.room==-1&&a.profile==686&&'),
 'far_unknown_profile':('policy.cpp','a.profile==686&&a.group==0','a.group==0'),
 'd3_replaces_XZ':('policy.cpp','if(d.link.xz<link_min_xz)','if(d.link.d3<link_min_xz)'),
 'movement_admitted':('policy.cpp','if(v.link.speed!=std::array<double,3>{}||v.link.speed_forward!=0)','if(false)'),
 'C_freshness_missing':('experiment_core.cpp','else if(!pilot_check(selected,&fresh))','else if(false)'),
 'reject_consumes_latch':('experiment_core.cpp','state_=State::Rejected;note(pilot::Event::PreflightReject,&first);return;','consumed_=true;state_=State::Rejected;note(pilot::Event::PreflightReject,&first);return;'),
 'critical_shares_normal':('trace.cpp','case Event::ReadyMonitorBudgetExhausted:return 1;','case Event::ReadyMonitorBudgetExhausted:return 0;')}
def run(project,overlay,work,ev,source):
 result=[]
 for name,(file,before,after) in MUTANTS.items():
  d=work/'mutants'/name;d.mkdir(parents=True,exist_ok=True)
  for f in (project/'src').glob('*'):shutil.copyfile(f,d/f.name)
  for f in (overlay/'src').glob('*'):
   if not (d/f.name).exists():shutil.copyfile(f,d/f.name)
  # Quoted relative include in Fake must select this mutant's own real header.
  (d/'tests').mkdir(exist_ok=True);(d/'src').mkdir(exist_ok=True);shutil.copyfile(overlay/'tests/fake.hpp',d/'tests/fake.hpp');shutil.copyfile(d/'experiment_core.hpp',d/'src/experiment_core.hpp')
  f=d/file;s=f.read_text()
  if s.count(before)!=1:raise RuntimeError('Mutant anchor '+name)
  f.write_text(s.replace(before,after))
  exe=d/'mutant.exe';cmd=['cl','/nologo','/O2','/std:c++20','/EHsc','/DDEVF_FIRST_OFFLINE=1',*['/I'+str(x) for x in [d,d/'tests',project/'src',source/'sdk/include']],str(project/'tests/g5_lab.cpp'),*[str(d/x) for x in ['experiment_core.cpp','admission.cpp','policy.cpp','trace.cpp','temporal.cpp','census_values.cpp']],'/Fe:'+str(exe),'/link','/STACK:8388608']
  with (ev/f'mutant-{name}-build.log').open('w') as log:cp=subprocess.run(cmd,cwd=d,stdout=log,stderr=subprocess.STDOUT)
  if cp.returncode:raise RuntimeError('Mutant did not compile '+name)
  cp=subprocess.run([str(exe)],capture_output=True,text=True);(ev/f'mutant-{name}.log').write_text(cp.stdout+cp.stderr)
  if cp.returncode!=1 or 'FAIL ' not in cp.stdout:raise RuntimeError('Mutant survived/crashed '+name)
  result.append({'name':name,'compiled':True,'killed':True,'exit':1,'failed_checks':[l for l in cp.stdout.splitlines() if l.startswith('FAIL ')]})
 return result
