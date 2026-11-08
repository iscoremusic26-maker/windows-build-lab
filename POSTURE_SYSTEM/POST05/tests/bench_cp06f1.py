"""CP06 FIX01 synthetic cost of the new callbacks in the modeled world (same scenarios with and
without FIX01). Reports idle E_OC execute and Link draw callback cost and the extra engine model
calcs per armed tick, the dominant real cost. NOT an FPS or game measurement.
Usage: bench_cp06f1.py <POST05 tree> [reps]"""
from pathlib import Path
import os,subprocess,sys,tempfile
tree=Path(sys.argv[1]).resolve();reps=sys.argv[2] if len(sys.argv)>2 else '3'
with tempfile.TemporaryDirectory() as td:
 p=Path(td);exe=p/('bench.exe' if os.name=='nt' else 'bench');src=tree/'tests/cp06f1_world_lab.cpp'
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/O2','/bigobj','/I'+str(tree/'tests'),str(src),'/Fe'+str(exe),'/Fo'+str(p/'bench.obj')] if os.name=='nt' else ['g++','-std=c++20','-O2','-I'+str(tree/'tests'),str(src),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=600)
 if c.returncode:print(c.stdout[-3000:]+c.stderr[-3000:]);sys.exit(c.returncode)
 c=subprocess.run([str(exe),'--bench',reps],capture_output=True,text=True,timeout=600);print(c.stdout+c.stderr[-2000:]);sys.exit(c.returncode)
