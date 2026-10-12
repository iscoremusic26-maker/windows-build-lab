"""Native directed integration: real headers/queue TUs plus hash-recorded engine functions.
Profile ownership, phase dispatch, layer bookkeeping, allocator and process deletion
are explicit controlled adapters. Not a game/scene/resource-mount runtime test.
"""
import argparse,ctypes,hashlib,json,re,subprocess,time,datetime
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--evidence',type=Path,required=True);p.add_argument('--temp',type=Path,required=True);a=p.parse_args()
a.source=a.source.resolve();a.evidence=a.evidence.resolve();a.temp=a.temp.resolve();a.temp.mkdir(parents=True,exist_ok=True);a.evidence.mkdir(parents=True,exist_ok=True)
here=Path(__file__).resolve().parents[1]
entries=json.loads((a.source/'build/x-windows-ci-msvc/compile_commands.json').read_text())
def function(text,name):
 m=re.search(r'(?m)^(?:[\w:<>,*]+\s+)'+re.escape(name)+r'\([^;]*?\)\s*(?:const\s*)?\{',text);assert m,name
 start=m.start();i=text.index('{',start)+1;depth=1
 while depth:
  if text[i]=='{':depth+=1
  elif text[i]=='}':depth-=1
  i+=1
 return text[start:i]
s=(a.source/'src/f_pc/f_pc_stdcreate_req.cpp').read_text();start=s.index('typedef struct standard_create_request_class {');end=s.index('} standard_create_request_class;',start)+len('} standard_create_request_class;');layout=s[start:end]
req=(a.source/'src/f_pc/f_pc_create_req.cpp').read_text()
base=(a.source/'src/f_pc/f_pc_base.cpp').read_text();dstart=base.index('base_process_class::~base_process_class() {');dend=base.index('\n}',dstart)+2;destructor=base[dstart:dend]
actor=(a.source/'src/dusk/mods/svc/actor.cpp').read_text()
parts=[destructor,layout,function(s,'fpcSCtRq_HasPendingForOwner'),function(actor,'finish_delete_all_actors_of_name')]+[function(req,n)for n in ['fpcCtRq_CreateQTo','fpcCtRq_ToCreateQ','fpcCtRq_Delete','fpcCtRq_Cancel']]
(a.temp/'real_functions.inc').write_text('\n\n'.join(parts)+'\n')
def argv_windows(command):
 argc=ctypes.c_int();api=ctypes.windll.shell32.CommandLineToArgvW;api.argtypes=[ctypes.c_wchar_p,ctypes.POINTER(ctypes.c_int)];api.restype=ctypes.POINTER(ctypes.c_wchar_p)
 ptr=api(command,ctypes.byref(argc));assert ptr;result=[ptr[i]for i in range(argc.value)];free=ctypes.windll.kernel32.LocalFree;free.argtypes=[ctypes.c_void_p];free.restype=ctypes.c_void_p;free(ptr);return result
entry=next(e for e in entries if e['file'].replace('\\','/').endswith('/src/f_pc/f_pc_create_req.cpp'))
args=entry.get('arguments')or argv_windows(entry['command']);idx=next(i for i,x in enumerate(args)if Path(x).name.lower()=='cl.exe');compiler=args[idx];flags=[]
for x in args[idx+1:]:
 if x in ['-c','/c'] or x.startswith(('/Fo','/Fd','/Fp','/Yu','/Yc')) or x.replace('\\','/').lower()==entry['file'].replace('\\','/').lower():continue
 flags.append(x)
units=['src/SSystem/SComponent/c_node.cpp','src/SSystem/SComponent/c_list.cpp','src/SSystem/SComponent/c_tree.cpp','src/SSystem/SComponent/c_tag.cpp','src/f_pc/f_pc_create_tag.cpp','src/f_pc/f_pc_method.cpp','src/f_pc/f_pc_delete_tag.cpp']
exe=a.temp/'directed_queue.exe'
cmd=[compiler,*flags,'/Od','/I'+str(a.temp),str(here/'tests/real_queue.cpp'),*[str(a.source/u)for u in units],'/Fe:'+str(exe),'/link','/INCREMENTAL:NO']
r={'scope':'real engine headers/layouts and seven original queue/method translation units; exact extracted scanner/cancel/delete functions; explicit engine-edge adapters; NOT game runtime','command':cmd,'engine_execution':'NOT_TESTED','source_function_sha256':[hashlib.sha256(x.encode()).hexdigest()for x in parts],'original_translation_units':[{'path':u,'sha256':hashlib.sha256((a.source/u).read_bytes()).hexdigest()}for u in units]}
r['started_at_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();start=time.monotonic();proc=subprocess.run(cmd,cwd=a.temp,capture_output=True,text=True,encoding='utf-8',errors='replace');r['compile_exit']=proc.returncode
(a.evidence/'directed-compile.log').write_text(proc.stdout+proc.stderr)
if not proc.returncode:
 proc=subprocess.run([str(exe)],cwd=a.temp,capture_output=True,text=True,encoding='utf-8',errors='replace');r['test_exit']=proc.returncode;r['output']=proc.stdout+proc.stderr;(a.evidence/'directed-tests.log').write_text(r['output'])
r['ended_at_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();r['elapsed_seconds']=round(time.monotonic()-start,3);r['status']='INTEGRATION_TEST_PASS'if r.get('test_exit')==0 else 'BLOCKED';(a.evidence/'directed-result.json').write_text(json.dumps(r,indent=2)+'\n');print(r['status'],r.get('output','See directed-compile.log'));raise SystemExit(proc.returncode)
