"""Execute original build commands, preserving phase exit code, timing and text logs."""
import argparse,datetime,json,subprocess,time,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--phase',required=True);p.add_argument('--source',type=Path,required=True);p.add_argument('--evidence',type=Path,required=True);p.add_argument('command',nargs=argparse.REMAINDER);a=p.parse_args();cmd=a.command
if cmd and cmd[0]=='--':cmd=cmd[1:]
assert cmd
a.evidence.mkdir(parents=True,exist_ok=True);now=lambda:datetime.datetime.now(datetime.timezone.utc).isoformat();r={'phase':a.phase,'command':cmd,'started_at_utc':now(),'state':'RUNNING'};f=a.evidence/(a.phase+'.json');f.write_text(json.dumps(r,indent=2)+'\n');start=time.monotonic()
try:
 with (a.evidence/(a.phase+'.log')).open('w',encoding='utf-8',newline='\n') as out:
  proc=subprocess.Popen(cmd,cwd=a.source,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace')
  for line in proc.stdout:out.write(line);out.flush();sys.stdout.write(line);sys.stdout.flush()
  code=proc.wait()
except Exception as e:
 r['exception']=str(e);code=1
r.update({'finished_at_utc':now(),'elapsed_seconds':round(time.monotonic()-start,3),'exit_code':code,'state':'PASS'if code==0 else'FAIL'});f.write_text(json.dumps(r,indent=2)+'\n');sys.exit(code)
