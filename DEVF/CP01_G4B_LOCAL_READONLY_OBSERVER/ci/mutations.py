"""Three focused semantic mutants, each must compile and be rejected by capture_lab."""
import shutil,subprocess
from pathlib import Path
def run_mutations(project,work,ev):
 result=[]
 mutations=[('center_is_clearance','Occupancy::OverlapNotVerified;','Occupancy::SeparatedUnderBound;'),('ignore_context_change','c.context_stable && c.event_read','true && c.event_read'),('drop_duplicate_id','if(c.actors[j].id==a.id)invalid=true;','if(false)invalid=true;'),('unavailable_water_is_absence','s->status==Status::UnavailableSafeApi','false')]
 for name,before,after in mutations:
  root=work/'mutants'/name;src=root/'DEVF';src.mkdir(parents=True,exist_ok=True)
  for folder in [project,project.parent/'CP01_A2_DISARMED_CONTROLLER',project.parent/'CP01_G2_READONLY_PACKAGE']:
   shutil.copytree(folder,src/folder.name,ignore=shutil.ignore_patterns('evidence','__pycache__'))
  p=src/project.name;f=p/'src/capture.cpp';text=f.read_text()
  if text.count(before)!=1:raise RuntimeError('Unanchored mutant '+name)
  f.write_text(text.replace(before,after));exe=root/'lab.exe'
  cmd=['cl','/nologo','/std:c++20','/EHsc','/DDEVF_G4B_READ_ONLY=1','/DDEVF_READ_ONLY=1','/DDEVF_A2_DISARMED=1',str(p/'tests/capture_lab.cpp'),str(f),str(src/'CP01_A2_DISARMED_CONTROLLER/src/controller_core.cpp'),'/Fe'+str(exe),'/Fo'+str(root)+'\\']
  with (ev/(name+'-build.log')).open('w') as log:built=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT).returncode
  if built:raise RuntimeError('Mutant failed to compile '+name)
  with (ev/(name+'-test.log')).open('w') as log:code=subprocess.run([str(exe)],stdout=log,stderr=subprocess.STDOUT).returncode
  if code==0:raise RuntimeError('Surviving mutant '+name)
  result.append({'name':name,'compiled':True,'test_exit':code,'killed':True})
 return result
