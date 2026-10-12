"""Extract exact 2.0.3 actor cleanup functions into an explicitly simulated laboratory."""
import argparse
from pathlib import Path
import hashlib,json
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=a.source/'src/dusk/mods/svc/actor.cpp';text=source.read_text()
names=['actor_delete','request_delete_all_actors_of_name','finish_delete_all_actors_of_name','has_actor_of_name','unregister_actor']
functions=[]; report=[]
for name in names:
 import re
 m=re.search(r'(?m)^(?:int|void|bool|ModResult) '+name+r'\([^;]*?\)\s*\{',text)
 assert m,name
 start=m.start();body_start=text.index('{',m.start());depth=1;i=body_start+1
 while depth:
  if text[i]=='{':depth+=1
  elif text[i]=='}':depth-=1
  i+=1
 code=text[start:i];functions.append(code)
 report.append({'name':name,'line':text[:start].count('\n')+1,'sha256':hashlib.sha256(code.encode()).hexdigest()})
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'host_cleanup_exact.inc').write_text('\n\n'.join(functions)+'\n')
(a.out/'host-functions.json').write_text(json.dumps({'source':'dusklight-2.0.3/src/dusk/mods/svc/actor.cpp','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'functions':report,'execution':'exact function bodies with synthetic queues, slot and deletion backends; NOT the real engine'},indent=2)+'\n')
print('Extracted five exact cleanup functions; engine queues/allocator are explicit laboratory substitutes')
