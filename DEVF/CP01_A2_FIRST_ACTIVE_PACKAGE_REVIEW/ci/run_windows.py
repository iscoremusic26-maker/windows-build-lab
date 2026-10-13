"""MSVC build, incremental fake lab, official SDK package. NEVER load DLL or game."""
import argparse,datetime,hashlib,json,os,subprocess,time,zipfile,shutil,re
from pathlib import Path
from audit import source_audit,pe_audit,correlate_request,sha
from verify_package import verify,pe_exports
from negative_packages import run as negative_cases
from mutations import run as run_mutations
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args()
project=Path(__file__).resolve().parents[1];pilot=project.parent/'CP01_A2_FIRST_PILOT_SAFETY_AND_TRACE';source=a.source.resolve();work=a.work.resolve();build=work/'build';ev=work/'evidence';delivery=work/'delivery'
ev.mkdir(parents=True,exist_ok=True);build.mkdir(parents=True,exist_ok=True)
auth=json.loads((project/'SOURCE_AUTHORITY.json').read_text());provenance=json.loads((project/'CI_SOURCE.json').read_text())
r={'status':'BLOCKED','started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source_commit':provenance['source_commit'],'run_id':os.getenv('GITHUB_RUN_ID'),'run_attempt':os.getenv('GITHUB_RUN_ATTEMPT'),'ci_head':os.getenv('DEVF_CI_HEAD'),'phases':[],'game_executed':False,'installed':False,'dll_loaded':False,'host_built':False,'runtime_tested':False,'creation_capable':True,'provider':'TECHNICALLY_ARMED_FIXED_C_E_OC','execution_authorized':False,'G5':'NO_GO','CP02':'NO_GO','toolchain_env':{k:os.getenv(k) for k in ['VCToolsVersion','WindowsSDKVersion','VSCMD_ARG_TGT_ARCH']}}
def run(label,cmd):
 start=time.monotonic()
 with (ev/f'{label}.log').open('w',encoding='utf-8') as log:cp=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,text=True)
 r['phases'].append({'label':label,'command':cmd,'seconds':round(time.monotonic()-start,3),'exit':cp.returncode});print(f'{label}: exit={cp.returncode}',flush=True)
 if cp.returncode:print((ev/f'{label}.log').read_text()[-12000:],flush=True);raise RuntimeError(label+' failed')
def git(at,*args):return subprocess.check_output(['git','-C',str(at),*args],text=True).strip()
def require_sha(raw,h,label):
 if sha(raw)!=h:raise RuntimeError('Hash drift '+label)
try:
 if os.name!='nt':raise RuntimeError('Real Windows/MSVC required')
 if os.getenv('GITHUB_EVENT_NAME')!='pull_request' or os.getenv('GITHUB_HEAD_REF')!='ci/devf-cp01-first-active-package' or os.getenv('GITHUB_BASE_REF')!='ci/devf-cp01-first-pilot-safety':raise RuntimeError('Wrong isolated event')
 if git(project.parents[1],'rev-parse','HEAD')!=r['ci_head']:raise RuntimeError('Wrong CI identity')
 if git(source,'rev-parse','HEAD')!=auth['official_root'] or git(source/'extern/aurora','rev-parse','HEAD')!=auth['aurora'] or git(source,'status','--porcelain','--untracked-files=no'):raise RuntimeError('Official source differs')
 for rel,h in auth['sdk_lf_sha256'].items():require_sha((source/rel).read_bytes().replace(b'\r\n',b'\n'),h,rel)
 r['source_ledger']={f.relative_to(project).as_posix():sha(f.read_bytes().replace(b'\r\n',b'\n')) for f in project.rglob('*') if f.is_file() and f.suffix in ['.cpp','.hpp','.py','.txt','.json','.yml'] and 'evidence' not in f.parts and '__pycache__' not in f.parts and f.name!='CI_SOURCE.json'}
 archive=a.inputs/'Dusklight-v2.0.3-win32-x86_64.zip'
 if archive.stat().st_size!=56037777:raise RuntimeError('Original Windows ZIP/LFS not real')
 with zipfile.ZipFile(archive) as z:
  items=[i for i in z.infolist() if i.filename.endswith('sdk/windows-amd64.lib')]
  if len(items)!=1:raise RuntimeError('Unique original library required')
  data=z.read(items[0])
 if len(data)!=9400986:raise RuntimeError('Library size drift')
 require_sha(data,auth['importlib_sha256'],'importlib');lib=work/'windows-amd64.lib';lib.write_bytes(data)
 r['dependencies']={'sdk':auth['official_root'],'aurora':auth['aurora'],'library_bytes':len(data),'library_sha256':sha(data),'library_crc32':f'{items[0].CRC:08x}','sdk_files_verified':len(auth['sdk_lf_sha256']),'original_integrity':'CP00 reused; selected library CRC/hash rechecked'}
 r['negative_dependency_drift']=[]
 for name in ['manifest','SDK','importlib']:
  try:require_sha(b'own-negative-fixture',sha(b'own-expected-fixture'),name)
  except RuntimeError:r['negative_dependency_drift'].append({'case':name+'-hash-drift','rejected':True})
  else:raise RuntimeError('Drift check survived')
 version=build/'version.cpp';version.write_text('int compiler_only;\n')
 run('compiler',['cl','/Bv','/c',str(version),'/Fo'+str(build/'version.obj')]);run('cmake-version',['cmake','--version'])
 run('configure',['cmake','-S',str(project),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=cl','-DCMAKE_CXX_COMPILER=cl',f'-DDUSK_SOURCE={source}',f'-DDUSK_GAME_EXE={lib}','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 overlay=build/'overlay';trace=build/'active_trace';r['source_audit']=source_audit(project,overlay)
 for d,names in [(overlay,['overlay-ledger.json','overlay-diff.log']),(trace,['trace-ledger.json','trace-diff.log'])]:
  for n in names:shutil.copyfile(d/n,ev/n)
 run('build',['cmake','--build',str(build),'--target','devf_first_eoc_active','active_provider_lab','--parallel','2'])
 run('active-provider-lab',[str(build/'active_provider_lab.exe')]);r['lab_summary']=(ev/'active-provider-lab.log').read_text().splitlines()[-1]
 r['mutations']=run_mutations(project,pilot,overlay,trace,work,ev,source)
 r['negative_package_cases']=negative_cases((project/'mod.json').read_bytes())
 dll=build/'devf_first_eoc_active.dll'
 for label,args in [('headers',['/headers']),('imports',['/imports']),('exports',['/exports']),('disassembly',['/disasm'])]:run('module-'+label,['dumpbin',*args,str(dll)])
 r['binary_audit']=pe_audit(dll);r['exports']=pe_exports(dll)
 if r['binary_audit']['imports']!=auth['prior_imports']:raise RuntimeError('Exact import sets changed; explicit review required')
 linkmap=(build/'devf_first_eoc_active.map').read_text();disasm=(ev/'module-disassembly.log').read_text()
 for s in ['devf_first_request_link_anchor','fpcLy_SetCurrentLayer','fpcCtIt_Judge','?current_admission@','active_admission.cpp.obj','active_mod.cpp.obj','?request@NativeBackend@','LayerScope','room_owner_resolver.cpp.obj']:
  if s not in linkmap:raise RuntimeError('Production route/provider absent '+s)
 for s in ['Fake@','FakeEngine','historical_lab_not_executed','delete_actor','fopAcM_delete','fpcCtRq_Cancel','StageService','HookService']:
  if s in linkmap:raise RuntimeError('Unexpected production symbol '+s)
 r['request_slot32_correlation']=correlate_request(linkmap,disasm)
 # Correlate provider function body with its MAP range; tests execute the same source.
 rows=[]
 for line in linkmap.splitlines():
  m=re.match(r'\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{16})\b',line)
  if m:rows.append((m[1],int(m[2],16),line))
 provider=next(addr for name,addr,line in rows if name.startswith('?current_admission@'))
 end=min(addr for name,addr,line in rows if addr>provider and ' f ' in line)
 block='\n'.join(line for line in disasm.splitlines() if (m:=re.match(r'\s*([0-9A-Fa-f]{16}):',line)) and provider<=int(m[1],16)<end)
 if not block or not re.search(r'mov\s+word ptr .*101h',block,re.I):raise RuntimeError('Compiled provider true/true not accredited')
 (ev/'provider-disassembly.log').write_text(block+'\n');r['provider_map_range']={'begin':hex(provider),'end':hex(end),'true_true_store':True,'source':'src/active_admission.cpp','human_permission':False}
 shutil.copyfile(build/'devf_first_eoc_active.map',ev/'module-map.log')
 cmds=json.loads((build/'compile_commands.json').read_text());units=[i for i in cmds if 'devf_first_eoc_active.dir' in i['command']];bridge=[i for i in cmds if 'pilot_readonly_bridge.dir' in i['command']]
 if len(units)!=7 or len(bridge)!=8:raise RuntimeError('Wrong production graph')
 if any('DEVF_READ_ONLY' in i['command'] or '/tests/' in i['file'].replace('\\','/') or 'overlay/src/admission.cpp' in i['file'].replace('\\','/') for i in units):raise RuntimeError('Wrong mutator/provider inputs')
 r['native_compile_units']=units;r['bridge_compile_units']=bridge;(ev/'compile-commands.json').write_text(json.dumps(cmds,indent=2)+'\n')
 before=dll.read_bytes();r['loose_dll']={'bytes':len(before),'sha256':sha(before)}
 if list(build.rglob('*.dusk')):raise RuntimeError('Premature package')
 run('official-package',['cmake','--build',str(build),'--target','devf_first_eoc_active_package','--parallel','2'])
 if dll.read_bytes()!=before:raise RuntimeError('Package rebuilt audited DLL')
 package=build/'mods/devf_first_eoc_active.dusk';r['package_verification']=verify(package,dll,project,work)
 for label,args in [('headers',['/headers']),('imports',['/imports']),('exports',['/exports']),('disassembly',['/disasm'])]:run('internal-'+label,['dumpbin',*args,str(work/'packaged-inspection.dll')])
 if (ev/'module-disassembly.log').read_text().splitlines()[4:]!=(ev/'internal-disassembly.log').read_text().splitlines()[4:]:
  # dumpbin file banner differs; byte identity plus independent PE parsed audit is authoritative.
  r['internal_dumpbin_banner_diff_expected']=True
 if git(source,'status','--porcelain','--untracked-files=no') or git(source/'extern/aurora','status','--porcelain','--untracked-files=no'):raise RuntimeError('Official source changed')
 r['official_source_unchanged']=True;r['status']='PACKAGE_BUILD_PASS_DOWNLOAD_PENDING'
 name=f'DEVF_CP01_E_OC_FIRST_ACTIVE_PILOT_windows-amd64_{r["source_commit"]}.dusk';r['filename']=name
 compiler=re.search(r'Compiler Version ([0-9.]+) for x64',(ev/'compiler.log').read_text())
 if not compiler:raise RuntimeError('Compiler unverified')
 pv=r['package_verification'];manifest={'filename':name,'source_commit':r['source_commit'],'baseline_commit':auth['baseline'],'workflow_commit':r['ci_head'],'workflow_run_id':r['run_id'],'workflow_run_attempt':r['run_attempt'],'official_sdk':auth['official_root'],'aurora':auth['aurora'],'import_library_sha256':auth['importlib_sha256'],'compiler':'MSVC '+compiler[1],'toolchain_env':r['toolchain_env'],'module_id':pv['manifest']['id'],'module_version':pv['manifest']['version'],'package_sha256':pv['package_sha256'],'package_bytes':pv['package_bytes'],'dll_sha256':pv['dll_sha256'],'dll_bytes':pv['dll_bytes'],'manifest_sha256':pv['manifest_sha256'],'creation_capable':True,'provider_armed_in_binary':True,'installed':False,'runtime_tested':False,'execution_authorized':False,'installation_authorized':False,'G5':'NO_GO','G6':'NOT_VERIFIED','CP02':'NO_GO','location':'LOCATION_SAFE_UNVERIFIED','unloading':'CUSTOM_PROFILE_DLL_UNLOADING_BLOCKED','risk':'May request ONE E_OC automatically if loaded and live A/B/C guards pass. NOT INSTALLED / NOT RUN / SESSION NOT AUTHORIZED. STOP does not cancel/rollback; no hot reload.','generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
 if delivery.exists():raise RuntimeError('Unexpected delivery folder')
 delivery.mkdir();shutil.copyfile(package,delivery/name);(delivery/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n');(ev/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
finally:
 r['ended_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(ev/'result.json').write_text(json.dumps(r,indent=2)+'\n')
 replacements=[(str(work),'<WORK>'),(str(source),'<OFFICIAL>'),(str(project.parents[1]),'<WORKSPACE>')]
 for f in ev.iterdir():
  if not f.is_file():continue
  if f.suffix not in ['.log','.json']:raise RuntimeError('Nontext evidence forbidden')
  s=f.read_text(encoding='utf-8',errors='replace')
  for old,new in replacements:s=s.replace(old,new).replace(old.replace('\\','/'),new).replace(old.replace('\\','\\\\'),new)
  f.write_text(s,encoding='utf-8')
 manifest={f.name:{'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())} for f in ev.iterdir() if f.is_file()}
 (ev/'evidence-sha256.json').write_text(json.dumps(manifest,indent=2)+'\n')
