"""Separate executables: untouched authentic CP06 tracker/renderer vs production mirror/geometry.
No game/DLL execution. Synthetic public interpolation inputs, actual source algorithms.
"""
from pathlib import Path
import time
import subprocess,tempfile,os,struct,json,hashlib,math
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
 p=Path(td); traces=[]; outputs=[]
 for oracle in (True,False):
  name='oracle' if oracle else 'production';exe=p/(name+('.exe' if os.name=='nt' else ''));src=root/'tests/alignment_lab.cpp'
  cmd=['cl','/nologo','/std:c++20','/EHsc','/O2','/utf-8',str(src),'/Fe'+str(exe),'/Fo'+str(p/(name+'.obj'))] if os.name=='nt' else ['g++','-std=c++20','-O1',str(src),'-o',str(exe)]
  cmd+=['/DFIX02_ORACLE' if os.name=='nt' else '-DFIX02_ORACLE'] if oracle else []
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
  if c.returncode:raise RuntimeError(c.stdout+c.stderr)
  trace=p/(name+'.bin');started=time.perf_counter();c=subprocess.run([str(exe),str(trace)],capture_output=True,text=True,timeout=60)
  if c.returncode:raise RuntimeError(c.stdout+c.stderr)
  outputs.append({"output":c.stdout.strip(),"model_seconds":time.perf_counter()-started});traces.append(trace.read_bytes())
 a,b=traces;at=bt=count=frames=0;maximum=0.;legacy=0
 while at<len(a):
  ah=struct.unpack_from('<III',a,at);bh=struct.unpack_from('<III',b,bt);at+=12;bt+=12
  assert ah==bh,('header mismatch',ah,bh);frames+=1
  for i in range(ah[2]):
   ar=struct.unpack_from('<Qffff',a,at);br=struct.unpack_from('<Qffff',b,bt);at+=24;bt+=24
   assert ar[0]==br[0] and abs(ar[3]-br[3])<1e-6 and abs(ar[4]-br[4])<1e-6,('identity/alpha/anchor mismatch',ar,br)
   error=max(abs(ar[1]-br[1]),abs((br[2]-ar[2])-4.7));maximum=max(maximum,error)
   assert error<0.0001,('real geometry centre mismatch',ar,br,error)
   if frames<200 and ah[0]==80 and ah[1]==0:assert ar[4]==68;legacy+=1
   count+=1
 assert at==len(a) and bt==len(b) and count>100000 and legacy==1
 print(json.dumps({'status':'LAB_PASS','checks':count*4+frames,'paired_bar_presentations':count,'presentation_frames':frames,'max_relative_logical_error':maximum,'constant_life_posture_centre_delta':4.7,'old_max_height_regression_error':80,'old_regression_rejected':True,'oracle_trace_sha256':hashlib.sha256(a).hexdigest(),'production_trace_sha256':hashlib.sha256(b).hexdigest(),'source_outputs':outputs,'native_runtime':False}))
