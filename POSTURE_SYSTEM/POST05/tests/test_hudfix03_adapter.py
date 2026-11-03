"""Same independently traversed scene, actual base/current production callbacks.
Critical native identity is u16 const J2DScreen::getTypeID()==8, verified in EXE.
Not host raster execution; official SDK MSVC/PE audit separately verifies ABI.
"""
from pathlib import Path
import subprocess,tempfile,os,hashlib,json
root=Path(__file__).resolve().parents[1]
mod=(root/'src/mod.cpp').read_text(encoding='utf-8')
head=(root/'tests/fixtures/hudfix03_native_model.hpp').read_text().replace('HUDFIX_HEADER',(root/'src/hudfix_core.hpp').as_posix())
tail=(root/'tests/fixtures/hudfix03_scenario.cpp').read_text()
adapter=mod[mod.index('struct HudFixGlue {'):mod.index('// LECTURA (por valor) del prompt de accion inferior')]
ui=mod[mod.index('// FIX03: suppression requires'):mod.index('// Link viva')]
base=(root/'tests/fixtures/hudfix02_native_base.inc').read_text(encoding='utf-8')
outputs=[]
with tempfile.TemporaryDirectory() as td:
 p=Path(td)
 for old in (True,False):
  name='base' if old else 'fixed';cpp=p/(name+'.cpp');exe=p/(name+('.exe' if os.name=='nt' else ''));cpp.write_text(head+'\n'+(base if old else adapter+ui)+'\n'+tail,encoding='utf-8')
  cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(cpp),'/Fe'+str(exe),'/Fo'+str(p/(name+'.obj'))] if os.name=='nt' else ['g++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(cpp),'-o',str(exe)]
  if old:cmd+=['/DHUD03_BASE' if os.name=='nt' else '-DHUD03_BASE']
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
  if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
  env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0';c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60,env=env)
  print(c.stdout+c.stderr)
  if c.returncode:raise SystemExit(c.returncode)
  outputs.append(c.stdout.strip())
assert 'BASE_0_11_4_REGRESSION_PROVEN' in outputs[0] and 'RESULT=LAB_PASS' in outputs[1]
print(json.dumps({'status':'LAB_PASS','same_scene_base_regression_proven':True,'base_reference_sha256':hashlib.sha256(base.encode()).hexdigest(),'outputs':outputs,'native_runtime':False}))
