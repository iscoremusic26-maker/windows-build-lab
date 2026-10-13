"""Six semantic mutants in temporary lab copies, never in the native module."""
import shutil,subprocess,json
from pathlib import Path
MUTANTS={
 'double_attempt': [('src/controller_core.cpp','if (consumed_ || state_==State::StopSession || state_==State::Failed) return false;','if (false) return false;'),('tests/simulation.hpp','if (c.consumed()) return;','if (false) return;')],
 'omit_restore': [('tests/simulation.hpp','try { f_.select(previous_); ok_=f_.current()==previous_; } catch (...) { ok_=false; }','ok_=true;')],
 'allow_root': [('src/controller_core.cpp','o.child==o.root || ','')],
 'foreign_id': [('src/controller_core.cpp',' || a.id!=id_','')],
 'continue_after_error': [('src/controller_core.cpp','if (state_!=State::Requested && state_!=State::Pending && state_!=State::ReadyObserved) return;','if (state_==State::Disarmed) return;')],
 'ignore_transition': [('src/controller_core.cpp','if (o.transition || o.retiring || o.next_room!=o.room)','if (o.retiring)')]
}
def run_mutations(project,work,evidence,compiler='cl'):
 results=[]
 for name,edits in MUTANTS.items():
  folder=work/'mutants'/name;folder.mkdir(parents=True,exist_ok=True)
  for sub in ['src','tests']:shutil.copytree(project/sub,folder/sub,dirs_exist_ok=True)
  for file,old,new in edits:
   p=folder/file;s=p.read_text(encoding='utf-8');assert s.count(old)==1,(name,file);p.write_text(s.replace(old,new),encoding='utf-8')
  exe=folder/('mutant.exe' if compiler=='cl' else 'mutant')
  cmd=([compiler,'/nologo','/std:c++20','/EHsc','/DDEVF_A2_DISARMED=1','/DDEVF_A2_LAB_ONLY=1',str(folder/'tests/controller_lab.cpp'),str(folder/'src/controller_core.cpp'),'/Fe:'+str(exe)] if compiler=='cl' else [compiler,'-std=c++20','-DDEVF_A2_DISARMED=1','-DDEVF_A2_LAB_ONLY=1',str(folder/'tests/controller_lab.cpp'),str(folder/'src/controller_core.cpp'),'-o',str(exe)])
  with (evidence/f'mutant-{name}-build.log').open('w',encoding='utf-8') as log:cp=subprocess.run(cmd,cwd=folder,stdout=log,stderr=subprocess.STDOUT)
  if cp.returncode:raise RuntimeError(f'Mutant must compile: {name}')
  cp=subprocess.run([str(exe)],capture_output=True,text=True)
  (evidence/f'mutant-{name}.log').write_text(cp.stdout+cp.stderr,encoding='utf-8')
  if cp.returncode!=1 or 'FAIL ' not in cp.stdout:raise RuntimeError(f'Mutant survived or crashed: {name}')
  results.append({'mutation':name,'detected':True,'exit':cp.returncode,'failed_checks':[s for s in cp.stdout.splitlines() if s.startswith('FAIL ')]})
 return results
