"""Semantic mutations of own core only. Must compile, fail named assertions, never crash."""
import shutil,subprocess
MUTANTS={
 'omit_restore': [('if(!backend_.registered(previous_))return false;backend_.select(previous_);ok_=backend_.current()==previous_;','ok_=true;')],
 'double_enqueue': [('const auto result=b.request(preset);','const auto result=b.request(preset);b.request(preset);')],
 'use_Link_owner': [('scope.select(origin_.child)','scope.select(origin_.link_owner)')],
 'ready_from_MOD_OK': [('state_=State::Requested;reason_=Reason::None;','state_=State::ActorReadyObserved;reason_=Reason::None;')],
 'ignore_wrong_ID': [('if(a.id!=id_){stop(Reason::Identity);return;}','if(false){stop(Reason::Identity);return;}')],
 'timeout_rearm': [('if(polls_>=timeout_)stop(Reason::Timeout);','if(polls_>=timeout_){consumed_=false;state_=State::Blocked;}')],
 'ignore_transition': [('if (o.transition || o.retiring || o.next_room!=o.room)','if (o.retiring || o.next_room!=o.room)')]
}
def run_mutations(project,work,evidence):
 results=[]
 for name,edits in MUTANTS.items():
  folder=work/'mutants'/name;folder.mkdir(parents=True,exist_ok=True)
  for sub in ['src','tests']:shutil.copytree(project/sub,folder/sub,dirs_exist_ok=True)
  f=folder/'src/experiment_core.cpp';s=f.read_text()
  for old,new in edits:
   if s.count(old)!=1:raise RuntimeError('Unique mutation anchor missing: '+name)
   s=s.replace(old,new)
  f.write_text(s);exe=folder/'mutant.exe'
  cmd=['cl','/nologo','/std:c++20','/EHsc','/DDEVF_FIRST_OFFLINE=1',str(folder/'tests/request_lab.cpp'),str(f),'/Fe:'+str(exe)]
  with (evidence/f'mutant-{name}-build.log').open('w') as log:cp=subprocess.run(cmd,cwd=folder,stdout=log,stderr=subprocess.STDOUT)
  if cp.returncode:raise RuntimeError('Mutant must compile: '+name)
  cp=subprocess.run([str(exe)],capture_output=True,text=True)
  (evidence/f'mutant-{name}.log').write_text(cp.stdout+cp.stderr)
  if cp.returncode!=1 or 'FAIL ' not in cp.stdout:raise RuntimeError('Mutant survived or crashed: '+name)
  results.append({'mutation':name,'compiled':True,'killed':True,'exit':cp.returncode,'failed_checks':[s for s in cp.stdout.splitlines() if s.startswith('FAIL ')]})
 return results
