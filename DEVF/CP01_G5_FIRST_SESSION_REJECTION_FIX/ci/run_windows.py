"""Windows-only new lab + DISARMED diagnostic DLL, never host or DLL execution."""
import argparse,datetime,json,os,re,shutil,subprocess,time,zipfile
from pathlib import Path
from audit import pe_audit,parse_diagnostic,pe_exports,verify_package,sha
from mutations import run as mutations
from zip_negatives import run as zip_negatives
from pe_negatives import run as pe_negatives
from memcmp_evidence import verify as verify_memcmp
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args();project=Path(__file__).resolve().parents[1];source=a.source.resolve();work=a.work.resolve();build=work/'build';ev=work/'evidence';ev.mkdir(parents=True,exist_ok=True);build.mkdir(parents=True,exist_ok=True)
auth=json.loads((project/'SOURCE_MANIFEST.json').read_text());provenance=json.loads((project/'CI_SOURCE.json').read_text())
r={'status':'BLOCKED','source_commit':provenance['source_commit'],'ci_head':os.getenv('DEVF_CI_HEAD'),'run_id':os.getenv('GITHUB_RUN_ID'),'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phases':[],'game_executed':False,'dll_loaded':False,'host_built':False,'creation_capable':False,'mode':'DISARMED_READ_ONLY_DIAGNOSTIC','next_G5_authorized':False,'CP02':'NO_GO','toolchain_env':{k:os.getenv(k) for k in ['VCToolsVersion','WindowsSDKVersion','VSCMD_ARG_TGT_ARCH']}}
def git(at,*args):return subprocess.check_output(['git','-C',str(at),*args],text=True).strip()
def run(label,cmd):
 start=time.monotonic()
 with (ev/(label+'.log')).open('w') as log:cp=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,text=True)
 r['phases'].append({'label':label,'command':cmd,'seconds':round(time.monotonic()-start,3),'exit':cp.returncode});print(label+': exit='+str(cp.returncode),flush=True)
 if cp.returncode:print((ev/(label+'.log')).read_text()[-10000:]);raise RuntimeError(label)
try:
 if os.name!='nt' or os.getenv('GITHUB_HEAD_REF')!='ci/devf-cp01-g5-rejections' or os.getenv('GITHUB_BASE_REF')!='ci/devf-cp01-first-active-package':raise RuntimeError('Windows/exclusive event required')
 if git(source,'rev-parse','HEAD')!=auth['official_root'] or git(source/'extern/aurora','rev-parse','HEAD')!=auth['aurora']:raise RuntimeError('SDK pin drift')
 for fields,root in [('sdk_lf_sha256',source),('directed_sdk_source_sha256',source),('reuse_lf_sha256',project.parent)]:
  for rel,h in auth[fields].items():
   if sha((root/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Pinned source drift '+rel)
 r['source_ledger']={f.relative_to(project).as_posix():sha(f.read_bytes().replace(b'\r\n',b'\n')) for f in project.rglob('*') if f.is_file() and f.suffix in ['.cpp','.hpp','.py','.json','.txt','.yml'] and 'evidence' not in f.parts and '__pycache__' not in f.parts and f.name!='CI_SOURCE.json'}
 archive=a.inputs/'Dusklight-v2.0.3-win32-x86_64.zip'
 if archive.stat().st_size!=56037777:raise RuntimeError('Full original LFS ZIP required')
 with zipfile.ZipFile(archive) as z:
  entry=z.getinfo('sdk/windows-amd64.lib');data=z.read(entry)
 if len(data)!=9400986 or sha(data)!=auth['importlib_sha256']:raise RuntimeError('Library pin drift')
 lib=work/'windows-amd64.lib';lib.write_bytes(data);r['dependencies']={'SDK':auth['official_root'],'Aurora':auth['aurora'],'lib_bytes':len(data),'lib_sha256':sha(data),'lib_CRC32':f'{entry.CRC:08x}'}
 v=build/'version.cpp';v.write_text('int compiler_only;\n');run('compiler',['cl','/Bv','/c',str(v),'/Fo'+str(build/'version.obj')]);run('cmake-version',['cmake','--version'])
 run('configure',['cmake','-S',str(project),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',f'-DDUSK_SOURCE={source}',f'-DDUSK_GAME_EXE={lib}','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 overlay=build/'overlay'
 for n in ['overlay-ledger.json','overlay-diff.log','g5-overlay-ledger.json','g5-overlay-diff.log']:shutil.copyfile(overlay/n,ev/n)
 reuse=json.loads((project/'REUSE_WINDOWS_LAB.json').read_text())
 for rel,h in reuse['same_input_lf_sha256'].items():
  if sha((project/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Reuse input changed '+rel)
 r['lab_summary']=reuse['lab_summary'];r['mutations']=reuse['mutations'];r['historical_zip_negative_cases']=reuse['zip_negative_cases'];r['lab_reused_from_run']=reuse['origin_run'];r['lab_not_reexecuted']=True
 run('build',['cmake','--build',str(build),'--target','devf_g5_rejection_diagnostic','--parallel','2'])
 dll=build/'devf_g5_rejection_diagnostic.dll';r['parsed_pe_before_review']=parse_diagnostic(dll);r['exports']=pe_exports(dll)
 (ev/'candidate-imports.json').write_text(json.dumps(r['parsed_pe_before_review'],indent=2)+'\n')
 for label,arg in [('headers','/headers'),('imports','/imports'),('exports','/exports'),('disassembly','/disasm')]:run('module-'+label,['dumpbin',arg,str(dll)])
 linkmap=(build/'devf_g5_rejection_diagnostic.map').read_text()
 for bad in ['?attempt@Controller@','?poll@Controller@','?request@NativeBackend@','LayerScope','fpcLy_SetCurrentLayer','fpcCtIt_Judge','Fake@','historical_104_not_run','ActorService','StageService','HookService']:
  if bad in linkmap:raise RuntimeError('Mutation/test symbol in diagnostic '+bad)
 shutil.copyfile(build/'devf_g5_rejection_diagnostic.map',ev/'module-map.log')
 r['binary_audit']=pe_audit(dll)
 r['memcmp_review']=verify_memcmp(dll,linkmap,(ev/'module-disassembly.log').read_text(),(overlay/'src/frame_validation.cpp').read_text())
 r['pe_negative_cases']=pe_negatives(dll,work)
 r['zip_negative_cases']=zip_negatives() # Verifier now checks directory type consistency; rerun affected fixtures.
 r['zip_negatives_reexecuted']=True
 (ev/'semantic-pe-negative-results.json').write_text(json.dumps(r['pe_negative_cases'],indent=2)+'\n')
 (ev/'memcmp-review.json').write_text(json.dumps(r['memcmp_review'],indent=2)+'\n')
 cmds=json.loads((build/'compile_commands.json').read_text());units=[x for x in cmds if 'devf_g5_rejection_diagnostic.dir' in x['command']];bridge=[x for x in cmds if 'pilot_readonly_bridge.dir' in x['command']]
 if len(units)!=6 or len(bridge)!=8 or any('native_backend.cpp' in x['file'] or 'experiment_core.cpp' in x['file'] or '/tests/' in x['file'].replace('\\','/') for x in units):raise RuntimeError('Diagnostic graph drift')
 r['native_units']=units;r['bridge_units']=bridge;(ev/'compile-commands.json').write_text(json.dumps(cmds,indent=2)+'\n')
 r['diagnostic_binary_excludes']={'ActorService':True,'NativeBackend':True,'Controller_attempt_poll':True,'LayerScope_and_native_setter':True,'request_slot32':'ABSENT_INTENTIONALLY_DIAGNOSTIC','game_imports':30}
 before=dll.read_bytes();r['loose_dll']={'bytes':len(before),'sha256':sha(before)}
 run('official-package',['cmake','--build',str(build),'--target','devf_g5_rejection_diagnostic_package','--parallel','2'])
 if dll.read_bytes()!=before:raise RuntimeError('Package rebuilt DLL')
 package=build/'mods/devf_g5_rejection_diagnostic.dusk';r['package_verification']=verify_package(package,dll,project,work)
 if git(source,'status','--porcelain','--untracked-files=no') or git(source/'extern/aurora','status','--porcelain','--untracked-files=no'):raise RuntimeError('Official sources changed')
 name='DEVF_CP01_G5_DISARMED_DIAGNOSTIC_windows-amd64_'+r['source_commit']+'.dusk';pv=r['package_verification'];m={'filename':name,'source_commit':r['source_commit'],'workflow_commit':r['ci_head'],'workflow_run_id':r['run_id'],'official_sdk':auth['official_root'],'aurora':auth['aurora'],'importlib_sha256':auth['importlib_sha256'],'module_id':pv['manifest']['id'],'version':pv['manifest']['version'],'package_bytes':pv['package_bytes'],'package_sha256':pv['package_sha256'],'dll_bytes':pv['dll_bytes'],'dll_sha256':pv['dll_sha256'],'creation_capable':False,'provider_armed':False,'read_only_diagnostic':True,'installed':False,'runtime_tested':False,'execution_authorized':False,'NEXT_G5_SESSION_AUTHORIZED':False,'LINK_IDLE_POLICY_EVIDENCE':'UNVERIFIED','LOCATION_SAFE_VERIFIED':'NOT_VERIFIED','G6':'NOT_VERIFIED','CP02':'NO_GO','CUSTOM_PROFILE_DLL_UNLOADING':'BLOCKED_INDEPENDENT'}
 delivery=work/'delivery';delivery.mkdir();shutil.copyfile(package,delivery/name);(delivery/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(m,indent=2)+'\n');(ev/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(m,indent=2)+'\n');r['status']='DIAGNOSTIC_PACKAGE_PASS_DOWNLOAD_PENDING'
finally:
 r['ended_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(ev/'result.json').write_text(json.dumps(r,indent=2)+'\n')
 for f in ev.iterdir():
  if f.suffix not in ['.log','.json']:raise RuntimeError('Text evidence only')
  s=f.read_text(errors='replace')
  for old,new in [(str(work),'<WORK>'),(str(source),'<OFFICIAL>'),(str(project.parents[1]),'<WORKSPACE>')]:s=s.replace(old,new).replace(old.replace('\\','/'),new).replace(old.replace('\\','\\\\'),new)
  f.write_text(s)
 (ev/'evidence-sha256.json').write_text(json.dumps({f.name:{'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())} for f in ev.iterdir()},indent=2)+'\n')
