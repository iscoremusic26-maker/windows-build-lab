"""Native Windows/MSVC targets only; no package target, game execution or original upload."""
import argparse,datetime,hashlib,json,os,subprocess,time,zipfile,shutil,re
from pathlib import Path
from audit import source_audit,pe_audit,correlate_request
from mutations import run_mutations
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args()
project=Path(__file__).resolve().parents[1];source=a.source.resolve();work=a.work.resolve();build=work/'build';ev=work/'evidence'
ev.mkdir(parents=True,exist_ok=True);build.mkdir(parents=True,exist_ok=True)
ROOT='40457c6adb381928e4b5fef6ed459ed291edd5e2';AURORA='3227d76c60e1e782ca576610bce61c9e7744d8be';LIB='4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4'
r={'status':'BLOCKED','started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'run_id':os.getenv('GITHUB_RUN_ID'),'run_attempt':os.getenv('GITHUB_RUN_ATTEMPT'),'ci_head':os.getenv('DEVF_CI_HEAD'),'phases':[],'game_executed':False,'packaged':False,'host_built':False,'runtime':'NOT_TESTED','policy':'PILOT_CONDITIONAL_LOCATION_SESSION_DENIED_NO_REGISTRATION','toolchain_env':{k:os.getenv(k) for k in ['VCToolsVersion','WindowsSDKVersion','VSCMD_ARG_TGT_ARCH']}}
def run(label,cmd,fail=False):
 start=time.monotonic()
 with (ev/f'{label}.log').open('w',encoding='utf-8') as log:cp=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,text=True)
 r['phases'].append({'label':label,'command':cmd,'seconds':round(time.monotonic()-start,3),'exit':cp.returncode})
 print(f'{label}: exit={cp.returncode}',flush=True)
 if bool(cp.returncode)!=fail:
  print((ev/f'{label}.log').read_text(encoding='utf-8')[-12000:],flush=True);raise RuntimeError(label+' failed expectation')
def git(at,*args):return subprocess.check_output(['git','-C',str(at),*args],text=True).strip()
try:
 if os.name!='nt':raise RuntimeError('MSVC Windows required')
 if git(source,'rev-parse','HEAD')!=ROOT or git(source/'extern/aurora','rev-parse','HEAD')!=AURORA or git(source,'status','--porcelain','--untracked-files=no'):raise RuntimeError('Official source differs')
 # Audit after configure creates the reviewed overlay; no official source edited.
 r['reused_source_ledger']={str(f.relative_to(project.parent)).replace('\\','/'):hashlib.sha256(f.read_bytes().replace(b'\r\n',b'\n')).hexdigest() for folder in ['CP01_A2_DISARMED_CONTROLLER','CP01_G2_READONLY_PACKAGE','CP01_G4B_LOCAL_READONLY_OBSERVER','CP01_A2_FIRST_NATIVE_REQUEST_OFFLINE'] for f in (project.parent/folder/'src').glob('*') if f.suffix in ['.cpp','.hpp']}
 r['compiled_main_commit']=json.loads((project/'CI_SOURCE.json').read_text())['own_source_commit']
 r['source_ledger']={str(f.relative_to(project)).replace('\\','/'):hashlib.sha256(f.read_bytes().replace(b'\r\n',b'\n')).hexdigest() for f in project.rglob('*') if f.is_file() and f.suffix in ['.cpp','.hpp','.py','.txt','.json','.yml'] and 'evidence' not in f.parts and '__pycache__' not in f.parts and f.name!='CI_SOURCE.json'}
 archive=a.inputs/'Dusklight-v2.0.3-win32-x86_64.zip'
 if archive.stat().st_size!=56037777:raise RuntimeError('Full original Windows ZIP not available')
 with zipfile.ZipFile(archive) as z:
  items=[i for i in z.infolist() if i.filename.endswith('sdk/windows-amd64.lib')]
  if len(items)!=1:raise RuntimeError('Unique CP00 import library required')
  data=z.read(items[0])
 if len(data)!=9400986 or hashlib.sha256(data).hexdigest()!=LIB:raise RuntimeError('Exact import library mismatch')
 lib=work/'windows-amd64.lib';lib.write_bytes(data)
 r['dependencies']={'official_root':ROOT,'aurora':AURORA,'import_library_sha256':LIB,'bytes':len(data),'member_crc32':f'{items[0].CRC:08x}','original_integrity':'CP00 reused; selected library only'}
 version=build/'version.cpp';version.write_text('int compiler_only;\n')
 run('compiler',['cl','/Bv','/c',str(version),'/Fo'+str(build/'version.obj')])
 run('cmake-version',['cmake','--version'])
 run('configure',['cmake','-S',str(project),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=cl','-DCMAKE_CXX_COMPILER=cl',f'-DDUSK_SOURCE={source}',f'-DDUSK_GAME_EXE={lib}','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 overlay=build/'overlay';r['source_audit']=source_audit(project,overlay)
 shutil.copyfile(overlay/'overlay-ledger.json',ev/'overlay-ledger.json');shutil.copyfile(overlay/'overlay-diff.log',ev/'overlay-diff.log')
 run('build',['cmake','--build',str(build),'--target','devf_first_pilot_experiment','pilot_safety_trace_lab','pilot_native_abi_lab','pilot_baseline_reentry_reproducer','--parallel','2'])
 for label in ['pilot_safety_trace_lab','pilot_native_abi_lab','pilot_baseline_reentry_reproducer']:run(label,[str(build/(label+'.exe'))])
 r['lab_summaries']={name:(ev/(name+'.log')).read_text().splitlines()[-1] for name in ['pilot_safety_trace_lab','pilot_native_abi_lab','pilot_baseline_reentry_reproducer']}
 r['mutations']=run_mutations(project,overlay,work,ev)
 dll=build/'devf_first_pilot_experiment.dll'
 for label,args in [('headers',['/headers']),('imports',['/imports']),('exports',['/exports']),('disassembly',['/disasm'])]:run('module-'+label,['dumpbin',*args,str(dll)])
 exports=(ev/'module-exports.log').read_text();names=[]
 import re
 for line in exports.splitlines():
  m=re.match(r'\s*\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\w+)\s*$',line)
  if m:names.append(m[1])
 if sorted(names)!=sorted(['mod_ctx','mod_meta','mod_initialize','mod_update','mod_shutdown']):raise RuntimeError('Unexpected exported API')
 r['binary_audit']=pe_audit(dll);r['exports']=names
 linkmap=(build/'devf_first_pilot_experiment.map').read_text()
 if any(word in linkmap for word in ['Fake@lab','FakeEngine','first_request_lab','fopAcM_delete','fpcCtRq_Cancel','StageService']):raise RuntimeError('Unapproved symbol in module map')
 if 'devf_first_request_link_anchor' not in linkmap or 'fpcLy_SetCurrentLayer' not in linkmap or 'fpcCtIt_Judge' not in linkmap:raise RuntimeError('Real native route absent')
 disasm=(ev/'module-disassembly.log').read_text()
 if not re.search(r'call\s+qword ptr \[r[a-z0-9]+\+20h\]',disasm,re.I):raise RuntimeError('ActorService create slot32 indirect call absent')
 shutil.copyfile(build/'devf_first_pilot_experiment.map',ev/'module-map.log')
 cmds=json.loads((build/'compile_commands.json').read_text())
 r['native_compile_units']=[i for i in cmds if 'devf_first_pilot_experiment.dir' in i['command']]
 r['bridge_compile_units']=[i for i in cmds if 'pilot_readonly_bridge.dir' in i['command']]
 if len(r['native_compile_units'])!=7 or len(r['bridge_compile_units'])!=8:raise RuntimeError('Wrong compilation graph')
 if any('/tests/' in i['file'].replace('\\','/') or 'DEVF_READ_ONLY' in i['command'] or 'DEVF_A2_DISARMED' in i['command'] for i in r['native_compile_units']):raise RuntimeError('Mislabelled creation units')
 (ev/'compile-commands.json').write_text(json.dumps(cmds,indent=2)+'\n')
 r['artifact_hashes']={f.name:{'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in [dll,build/'pilot_safety_trace_lab.exe',build/'pilot_native_abi_lab.exe']}
 r['request_slot32_correlation']=correlate_request(linkmap,disasm)
 r['call_graph_verified']={'entry':'mod_update -> link_anchor -> Controller.attempt -> LayerScope -> NativeBackend.request -> svc_actor.create_actor slot32; Admission NONE blocks entry BEFORE native reads','creation_code_present':True,'services':'Host/Log/Game/Actor','real_setter_import':True,'request_queue_visitor_import':True,'native_units':7,'readonly_bridge_units':8,'disassembly_slot32_call':True,'no_fake_in_module':True,'disassembly_log':'module-disassembly.log','link_map_log':'module-map.log'}
 if list(build.rglob('*.dusk')):raise RuntimeError('Package target unexpectedly built')
 if git(source,'status','--porcelain','--untracked-files=no'):raise RuntimeError('Official source changed')
 r['official_source_unchanged']=True;r['status']='A2_FIRST_PILOT_PASS_LAB';r['msvc']='A2_FIRST_PILOT_NATIVE_ABI_MSVC_LINK_PASS';r['binary']='A2_FIRST_PILOT_BINARY_AUDIT_PASS'
finally:
 r['ended_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 (ev/'result.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
 # Text evidence only, no standalone DLL/EXE/import library in uploaded directory.
 replacements=[(str(work),'<WORK>'),(str(source),'<OFFICIAL>'),(str(project.parent.parent.parent),'<WORKSPACE>')]
 for f in ev.iterdir():
  if f.is_file():
   s=f.read_text(encoding='utf-8',errors='replace')
   for old,new in replacements:s=s.replace(old,new).replace(old.replace('\\','/'),new).replace(old.replace('\\','\\\\'),new)
   f.write_text(s,encoding='utf-8')

manifest={f.name:{'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in ev.iterdir() if f.is_file()}
if any(f.suffix not in ['.log','.json'] for f in ev.iterdir()):raise RuntimeError('Nontext evidence forbidden')
(ev/'evidence-sha256.json').write_text(json.dumps(manifest,indent=2)+'\n')
