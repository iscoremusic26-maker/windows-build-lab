"""Apply only to verified copied originals; extract actual patched function bodies for a lab.
Synthetic queue/layout adapters are NOT ABI declarations. No original resources in output.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
here=Path(__file__).resolve().parent.parent
manifest=json.loads((here/'SOURCE_MANIFEST.json').read_text())
for x in manifest['files']:
 f=a.source/x['path'];assert hashlib.sha256(f.read_bytes()).hexdigest()==x['before_sha256'],x['path']
patch=(here/'host-lifetime.patch').read_bytes().replace(b'\r\n',b'\n')
assert hashlib.sha256(patch).hexdigest()==manifest['patch_sha256'],'Patch hash mismatch'
# Windows checkout may use CRLF; feed verified LF bytes and preserve source LF.
for args in [['--check','-'],['-']]:
 subprocess.run(['git','-c','core.autocrlf=false','-c','core.eol=lf','apply',*args],input=patch,cwd=a.source,check=True)
for x in manifest['files']:
 assert hashlib.sha256((a.source/x['path']).read_bytes()).hexdigest()==x['after_sha256'],x['path']
a.out.mkdir(parents=True,exist_ok=True)
groups={
 'ownership_exact.inc':('src/dusk/mods/svc/actor.cpp',['profile_owner','submission_owner','creation_allowed','allocate_profile_name']),
 'pending_exact.inc':('src/f_pc/f_pc_stdcreate_req.cpp',['fpcSCtRq_HasPendingForOwner']),
 'actor_exact.inc':('src/dusk/mods/svc/actor.cpp',['actor_delete','request_delete_all_actors_of_name','finish_delete_all_actors_of_name','has_actor_of_name','unregister_actor','actor_drain_mod','actor_remove_mod']),
 'loader_exact.inc':('src/dusk/mods/loader/loader.cpp',['ModLoader::deactivate_mod','ModLoader::collect_lifecycle_set','ModLoader::shutdown']),
 'native_exact.inc':('src/dusk/mods/loader/natives.cpp',['ModLoader::unload_native','ModLoader::drain_retired_natives']),
}
report=[]
for out,(path,names) in groups.items():
 text=(a.source/path).read_text();functions=[]
 for name in names:
  m=re.search(r'(?m)^(?:[\w:<>,*]+\s+)'+re.escape(name)+r'\([^;]*?\)\s*(?:const\s*)?\{',text)
  assert m,name
  start=m.start();i=text.index('{',start)+1;depth=1
  while depth:
   if text[i]=='{':depth+=1
   elif text[i]=='}':depth-=1
   i+=1
  code=text[start:i];functions.append(code);report.append({'name':name,'path':path,'line':text[:start].count('\n')+1,'sha256':hashlib.sha256(code.encode()).hexdigest()})
 (a.out/out).write_text('\n\n'.join(functions)+'\n')
(a.out/'functions.json').write_text(json.dumps({'functions':report,'scope':'actual patched function bodies with synthetic layout, queues and service backends; NOT host/game runtime'},indent=2)+'\n')
print(f'PATCH_AND_FUNCTION_EXTRACTION_PASS functions={len(report)} host_runtime=NOT_TESTED')
