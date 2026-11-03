"""Independent exact-rational oracle, current production + authentic compiled-base headers.
Both compile/run; base is expected to violate new policy (never a compile-error kill).
"""
from pathlib import Path
from fractions import Fraction
import subprocess,tempfile,shutil,json,hashlib,os
root=Path(__file__).resolve().parents[1]
base=json.loads((root/'tests/fixtures/cp0203_baseline.json').read_text())
def compare(output):
 errors=0;checks=0;fraction={};remaining={}
 for row in output.splitlines():
  p=row.split(',');checks+=1
  if p[0]=='T':
   key,tick,rate,accepted,posture,acc=map(int,p[1:]);fraction.setdefault(key,Fraction(0));remaining.setdefault(key,75)
   if tick>60 and remaining[key]>0:
    fraction[key]+=Fraction(rate,30);units=int(fraction[key]);remaining[key]=max(0,remaining[key]-units);fraction[key]-=units
   errors+=int(not accepted or posture!=remaining[key] or (remaining[key]>0 and acc!=fraction[key]*30))
  elif p[0]=='B':
   hp,rate=map(int,p[1:]);ratio=Fraction(hp,100);wanted=10 if ratio>Fraction(3,4) else 7 if ratio>Fraction(1,2) else 4 if ratio>Fraction(1,4) else 2 if ratio>Fraction(1,10) else 1;errors+=int(rate!=wanted)
  else:
   frame,age,result=float(p[1]),int(p[2]),int(p[3]);errors+=int(result!=(-1<=frame-1<=7 and age<=9))
 return checks,errors
results=[]
with tempfile.TemporaryDirectory() as td:
 td=Path(td)
 for old in (True,False):
  p=td/('base' if old else 'current');shutil.copytree(root/'src',p/'src');
  if old:
   for n,h in base['headers'].items():
    assert hashlib.sha256(h['source'].encode()).hexdigest()==h['sha256'];(p/'src'/n).write_text(h['source'],encoding='utf-8')
  code=(root/'tests/fixtures/cp0203_trace.cpp').read_text().replace('ROOT',p.as_posix());f=p/'trace.cpp';f.write_text('#include "'+(p/'src/parry_core.hpp').as_posix()+'"\n'+code,encoding='utf-8');exe=p/('trace.exe' if os.name=='nt' else 'trace')
  cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(f),'/Fe'+str(exe),'/Fo'+str(p/'trace.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(f),'-o',str(exe)]
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=180);assert c.returncode==0,c.stdout+c.stderr
  c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60);assert c.returncode==0,c.stdout+c.stderr
  checks,errors=compare(c.stdout);results.append({'source':'BASE_0_11_5' if old else 'CURRENT','checks':checks,'policy_mismatches':errors,'trace_sha256':hashlib.sha256(c.stdout.encode()).hexdigest()})
assert results[0]['policy_mismatches']>0 and results[1]['policy_mismatches']==0,results
print(json.dumps({'status':'LAB_PASS','base_compiled_and_executed_model':True,'old_policy_rejected':True,'reference':'Python fractions.Fraction, independent input boundaries','results':results,'native_runtime':False},indent=2))
