"""Actual production callbacks integrated with actual cores, modeled native I/O only."""
from pathlib import Path
import subprocess,tempfile,os,re
root=Path(__file__).resolve().parents[1]
s=(root/'src/mod.cpp').read_text(encoding='utf-8')
def definition(n):
 m=re.search(r'^void '+n+r'\(.*?^\}',s,re.M|re.S)
 assert m,n
 return m[0]
head=(root/'tests/fixtures/cp0203_adapter_head.inc').read_text().replace('ROOT',root.as_posix())
# This isolated CP02/03 update harness does not install CP05. Its full native
# callbacks and maintenance are exercised separately by test_cp05_adapter.py.
head+='\nvoid cp05_maintenance()noexcept{}\n'
# FIX02 observation does not affect CP02/03 values. Its production monitor and
# lifecycle are exercised by the dedicated FIX02 core/native/guard suites.
head+='\nvoid fix02_maintenance()noexcept{}\n'
# CP06: read-only late-input/chip summary maintenance is tested by test_cp06_combat.py and cp06_lab.
head+='\nvoid cp06_maintenance()noexcept{}\n'
# CP06 FIX01 0.11.12: contact coverage / flash mirror maintenance is tested by cp06f1_lab and cp06f1_world_lab.
head+='\nvoid cp06f1_maintenance()noexcept{}\n'
# LIZALFOS 0.12.0: its maintenance only touches E_DN slots; exercised by liz_lab.
head+='\nvoid liz_maintenance()noexcept{}\nvoid lockon_update()noexcept{}\nvoid pfx_maintenance()noexcept{}\n'
tail=(root/'tests/fixtures/cp0203_adapter_scenario.inc').read_text()
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'adapter.cpp';f.write_text(head+definition('post06_step')+'\n'+definition('do_update')+tail,encoding='utf-8');exe=p/('adapter.exe' if os.name=='nt' else 'adapter')
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(f),'/Fe'+str(exe),'/Fo'+str(p/'adapter.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
 if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
 env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0';c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60,env=env);print(c.stdout+c.stderr);raise SystemExit(c.returncode)
