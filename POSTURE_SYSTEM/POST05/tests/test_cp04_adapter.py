"""Compile the COMPLETE production CP04 callbacks, actual legacy cores and
unchanged native executeAttack against modeled SDK I/O. Not a game execution.
Native Find/execute/frame/contacts are explicitly modeled; no render claim.
"""
from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
head=(root/'tests/fixtures/cp04_adapter_head.inc').read_text().replace('ROOT',root.as_posix())
adapter=(root/'src/cp04_adapter.inc').read_text()
native=(root/'tests/reference/cp04_native_attack.inc').read_text()
tail=(root/'tests/fixtures/cp04_adapter_scenario.inc').read_text()
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'adapter.cpp';f.write_text(head+adapter+'\n'+native+tail,encoding='utf-8');exe=p/('adapter.exe' if os.name=='nt' else 'adapter')
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/DAVOID_UB=1',str(f),'/Fe'+str(exe),'/Fo'+str(p/'adapter.obj')] if os.name=='nt' else ['g++','-DAVOID_UB=1','-std=c++20','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
 if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
 env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0';c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60,env=env);print(c.stdout+c.stderr);raise SystemExit(c.returncode)
