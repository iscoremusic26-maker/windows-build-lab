"""0.11.2 regression wall: baseline source bodies plus scoped x64 stores.
Never loads a DLL. Byte checks intentionally cover the inherited manual-shield
stores; source pins and the unchanged PE import/hook set cover the remaining wall.
Not a general proof of every x64 memory effect or a runtime test.
"""
from pathlib import Path
import re,json,hashlib,sys,argparse
sys.path.insert(0,str(Path(__file__).resolve().parent))
import pe_audit

def canonical(s):
 s=re.sub(r'/\*.*?\*/',' ',s,flags=re.S);s=re.sub(r'//[^\n]*',' ',s)
 return re.sub(r'\s+','',s)
def function(s,name):
 # Named definitions start at column zero; prototypes are excluded by requiring
 # a brace before the first semicolon. Braces in comments/strings are skipped.
 clean=re.sub(r'/\*.*?\*/',' ',s,flags=re.S);clean=re.sub(r'//[^\n]*',' ',clean)
 for m in re.finditer(r'^.*?\b'+re.escape(name)+r'\([^;]*?\)\s*noexcept\s*\{',clean,re.M):
  start=m.start();pos=m.end();depth=1;quoted=False;escape=False
  while pos<len(clean) and depth:
   ch=clean[pos]
   if quoted:
    if escape:escape=False
    elif ch=='\\':escape=True
    elif ch=='"':quoted=False
   elif ch=='"':quoted=True
   elif ch=='{':depth+=1
   elif ch=='}':depth-=1
   pos+=1
  if depth==0:return canonical(clean[start:pos])
 raise ValueError('missing pinned definition '+name)
def sha(s):return hashlib.sha256(s.encode()).hexdigest()
def source(project,pin):
 errors=[];mod=(project/'src/mod.cpp').read_text(encoding='utf-8');approved=json.loads((project/'ci/cp0203_policy.json').read_text())
 for name,want in pin['functions'].items():
  body=function(mod,name)
  if name in approved['functions']:
   rule=approved['functions'][name];assert rule['before_sha256']==want;want=rule['approved_sha256']
  if sha(body)!=want:errors.append('inherited function changed: '+name)
 for name,want in pin['headers'].items():
  s=(project/'src'/name).read_text(encoding='utf-8')
  if name in approved['headers']:
   rule=approved['headers'][name];assert rule['before_sha256']==want;want=rule['approved_sha256']
  if name=='lifecycle_core.hpp':s=s.replace('kModVersion = "0.11.6"','kModVersion = "0.11.2"')
  if sha(s)!=want:errors.append('inherited header changed: '+name)
 for section in approved.get('frozen_sections',[]):
  try:body=mod[mod.index(section['begin']):mod.index(section['end'])]
  except ValueError:errors.append('missing frozen0.11.5 section: '+section['name']);continue
  if sha(body)!=section['sha256']:errors.append('changed frozen0.11.5 section: '+section['name'])
 return errors

def scoped_bytes(raw,maptext,name):
 funcs=[]
 for line in maptext.splitlines():
  m=re.search(r'^\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{16})\s+f\s',line)
  if m:funcs.append((int(m[2],16),m[1]))
 matched=[a for a,n in funcs if n.startswith('?'+name+'@')]
 if len(matched)!=1:raise ValueError('expected one map function '+name)
 a=matched[0];b=min(x for x,_ in funcs if x>a);pe=pe_audit.PE(raw)
 return raw[pe.off(a-pe.image_base):pe.off(b-pe.image_base)]
def check_scopes(scopes):
 errors=[];counts={}
 for name in ['on_shield_post_fx','on_collision_pre_fx']:
  b=scopes[name]
  clear=len(re.findall(rb'\x0f\xba[\xf0-\xf7]\x1b',b))
  store=len(re.findall(rb'\x89[\x80-\x83\x85-\x87\x88-\x8b\x8d-\x8f\x90-\x93\x95-\x97\x98-\x9b\x9d-\x9f\xa0-\xa3\xa5-\xa7\xa8-\xab\xad-\xaf\xb0-\xb3\xb5-\xb7\xb8-\xbb\xbd-\xbf]\x78\x06\x00\x00',b))
  counts[name]={'register_btr_bit27':clear,'guard_flag_store_plus678':store}
  if clear!=1 or store!=1:errors.append(name+': inherited guarded clear shape changed')
 b=scopes['on_stick_post_fx'];n=len(re.findall(rb'\x80[\xa0-\xa3\xa5-\xa7]\xe9\x40\x00\x00\xf7',b));counts['on_stick_post_fx']={'and_buttonB_clear_plus40e9':n}
 if n!=1:errors.append('inherited contextual B clear shape changed')
 return errors,counts

def main():
 a=argparse.ArgumentParser();a.add_argument('project',type=Path);a.add_argument('--dll',type=Path);a.add_argument('--map',type=Path);x=a.parse_args();pin=json.loads((x.project/'ci/baseline_0_11_2_barrier.json').read_text(encoding='utf-8'));errors=source(x.project,pin);r={'source_base':pin['source_commit'],'approved_cp02_functions':2,'approved_cp02_cp03_headers':3,'functions_pinned':len(pin['functions']),'headers_pinned':len(pin['headers']),'native_code_executed':False,'errors':errors}
 if x.dll:
  scopes={n:scoped_bytes(x.dll.read_bytes(),x.map.read_text(encoding='utf-8'),n) for n in ['on_shield_post_fx','on_collision_pre_fx','on_stick_post_fx']};e,counts=check_scopes(scopes);errors.extend(e);r['scoped_native_stores']=counts
  # Negative checks change actual extracted instruction streams, not fixtures.
  mutated=dict(scopes);mutated['on_shield_post_fx']=re.sub(rb'(\x0f\xba[\xf0-\xf7])\x1b',lambda m:m[1]+b'\x1a',scopes['on_shield_post_fx'],count=1);e,_=check_scopes(mutated);assert e
  mutated=dict(scopes);mutated['on_stick_post_fx']=scopes['on_stick_post_fx'].replace(b'\xe9\x40\x00\x00\xf7',b'\xe9\x40\x00\x00\xff',1);e,_=check_scopes(mutated);assert e;r['actual_instruction_negatives_rejected']=2
 r['status']='STORE_AUDIT_PASS' if not errors else 'STORE_AUDIT_FAIL';print(json.dumps(r,indent=2));return 0 if not errors else 1
if __name__=='__main__':sys.exit(main())
