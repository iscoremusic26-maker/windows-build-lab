import argparse,datetime,json,os,shutil,subprocess,time,zipfile
from pathlib import Path
from audit import sha,inspect_pe,verify_package,negatives
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args();project=Path(__file__).resolve().parents[1];source=a.source.resolve();work=a.work.resolve();build=work/'build';ev=work/'evidence';ev.mkdir(parents=True,exist_ok=True);build.mkdir(parents=True,exist_ok=True)
auth=json.loads((project/'SOURCE_MANIFEST.json').read_text());provenance=json.loads((project/'CI_SOURCE.json').read_text());r={'status':'BLOCKED','source_commit':provenance['source_commit'],'ci_head':os.getenv('DEVF_CI_HEAD'),'run_id':os.getenv('GITHUB_RUN_ID'),'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phases':[],'game_executed':False,'DLL_loaded':False,'host_built':False,'mode':'ONE_NATIVE_STAGE_ADDITION_NOT_READ_ONLY','toolchain_env':{k:os.getenv(k) for k in ['VCToolsVersion','WindowsSDKVersion','VSCMD_ARG_TGT_ARCH']}}
def git(at,*args):return subprocess.check_output(['git','-C',str(at),*args],text=True).strip()
def run(label,cmd):
 start=time.monotonic()
 with (ev/(label+'.log')).open('w') as f:cp=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,text=True)
 r['phases'].append({'label':label,'command':cmd,'seconds':round(time.monotonic()-start,3),'exit':cp.returncode});print(label+': exit='+str(cp.returncode),flush=True)
 if cp.returncode:print((ev/(label+'.log')).read_text()[-10000:]);raise RuntimeError(label)
try:
 if os.name!='nt' or os.getenv('GITHUB_HEAD_REF')!='ci/devf-cp01-stage-bokoblin-minimal' or os.getenv('GITHUB_BASE_REF')!='ci/devf-cp01-g5-rejections':raise RuntimeError('Exclusive Windows PR required')
 if git(source,'rev-parse','HEAD')!=auth['official_root'] or git(source/'extern/aurora','rev-parse','HEAD')!=auth['aurora']:raise RuntimeError('SDK pin drift')
 for field,root in [('sdk_lf_sha256',source),('directed_sdk_source_sha256',source),('reuse_lf_sha256',project.parent)]:
  for rel,h in auth[field].items():
   if sha((root/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Pinned source drift '+rel)
 r['source_ledger']={f.relative_to(project).as_posix():sha(f.read_bytes().replace(b'\r\n',b'\n')) for f in project.rglob('*') if f.is_file() and f.suffix in ['.cpp','.py','.json','.txt','.yml'] and 'evidence' not in f.parts and '__pycache__' not in f.parts and f.name!='CI_SOURCE.json'}
 archive=a.inputs/'Dusklight-v2.0.3-win32-x86_64.zip'
 if archive.stat().st_size!=56037777:raise RuntimeError('Complete original LFS ZIP required')
 with zipfile.ZipFile(archive) as z:entry=z.getinfo('sdk/windows-amd64.lib');data=z.read(entry)
 if len(data)!=9400986 or sha(data)!=auth['importlib_sha256'] or entry.CRC!=0x70c35731:raise RuntimeError('Private library hash/CRC/size mismatch')
 lib=work/'windows-amd64.lib';lib.write_bytes(data);r['dependencies']={'SDK':auth['official_root'],'Aurora':auth['aurora'],'lib_bytes':len(data),'lib_sha256':sha(data),'lib_CRC':'70c35731'}
 f=build/'version.cpp';f.write_text('int compiler_only;\n');run('compiler',['cl','/Bv','/c',str(f),'/Fo'+str(build/'version.obj')]);run('cmake-version',['cmake','--version'])
 run('configure',['cmake','-S',str(project),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',f'-DDUSK_SOURCE={source}',f'-DDUSK_GAME_EXE={lib}','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 run('build',['cmake','--build',str(build),'--target','devf_stage_bokoblin_minimal','--parallel','2'])
 reuse=json.loads((project/'REUSE_FAKE_LAB.json').read_text())
 for rel,h in reuse['input_lf_sha256'].items():
  data=(project/'CMakeLists.txt').read_text().split('add_executable(stage_bokoblin_lab',1)[1].encode() if rel=='lab_CMake_tail' else (project/rel).read_bytes().replace(b'\r\n',b'\n')
  if sha(data)!=h:raise RuntimeError('Lab reuse input changed '+rel)
 if r['toolchain_env']!=reuse['toolchain_env']:raise RuntimeError('Lab reuse toolchain changed')
 r['fake_scenarios']=reuse['fake_scenarios'];r['fake_reused_from_run']=reuse['origin_run'];r['fake_not_reexecuted']=True
 dll=build/'devf_stage_bokoblin_minimal.dll'
 for label,arg in [('headers','/headers'),('imports','/imports'),('exports','/exports')]:run('module-'+label,['dumpbin',arg,str(dll)])
 linkmap=(build/'devf_stage_bokoblin_minimal.map').read_text()
 for bad in ['NativeBackend','LayerScope','ActorService','HookService','fopAcM_create','fpcSCtRq_Request','fpcLy_SetCurrentLayer','historical_104','Fake']:
  if bad in linkmap:raise RuntimeError('Forbidden module symbol '+bad)
 shutil.copyfile(build/'devf_stage_bokoblin_minimal.map',ev/'module-map.log');r['binary_audit']=inspect_pe(dll);r['PE_negatives']=negatives(dll,work)
 (ev/'binary-audit.json').write_text(json.dumps(r['binary_audit'],indent=2)+'\n')
 commands=json.loads((build/'compile_commands.json').read_text());units=[x for x in commands if 'devf_stage_bokoblin_minimal.dir' in x['command']]
 if len(units)!=2 or any('/tests/' in x['file'].replace('\\','/') for x in units):raise RuntimeError('Production graph changed')
 r['production_units']=units;(ev/'compile-commands.json').write_text(json.dumps(commands,indent=2)+'\n');before=dll.read_bytes();r['loose_dll']={'bytes':len(before),'sha256':sha(before)}
 run('official-package',['cmake','--build',str(build),'--target','devf_stage_bokoblin_minimal_package','--parallel','2'])
 if dll.read_bytes()!=before:raise RuntimeError('Package rebuild drift')
 package=build/'mods/devf_stage_bokoblin_minimal.dusk';r['package_verification']=verify_package(package,dll,project,work)
 if git(source,'status','--porcelain','--untracked-files=no') or git(source/'extern/aurora','status','--porcelain','--untracked-files=no'):raise RuntimeError('Official sources changed')
 name='DEVF_CP01_STAGE_BOKOBLIN_MINIMAL_windows-amd64_'+r['source_commit']+'.dusk';pv=r['package_verification'];meta={'filename':name,'source_commit':r['source_commit'],'workflow_commit':r['ci_head'],'workflow_run_id':r['run_id'],'official_sdk':auth['official_root'],'aurora':auth['aurora'],'importlib_sha256':auth['importlib_sha256'],'module_id':pv['manifest']['id'],'version':pv['manifest']['version'],'package_bytes':pv['package_bytes'],'package_sha256':pv['package_sha256'],'dll_bytes':pv['dll_bytes'],'dll_sha256':pv['dll_sha256'],'adds_native_actor_entry':True,'read_only':False,'ActorService_used':False,'registered_custom_profiles':False,'installed':False,'runtime_tested':False,'execution_authorized':False,'CP02':'NO_GO','location_proposal':'F_SP121 room3 layer6 (-1250,-6304.0083,60750); see HUMAN_TEST.md'}
 delivery=work/'delivery';delivery.mkdir();shutil.copyfile(package,delivery/name);(delivery/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(meta,indent=2)+'\n');(ev/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(meta,indent=2)+'\n');r['status']='STAGE_PACKAGE_PASS_DOWNLOAD_PENDING'
except Exception as ex:r['error']=str(ex);raise
finally:
 r['ended_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(ev/'result.json').write_text(json.dumps(r,indent=2)+'\n')
 for f in ev.iterdir():
  if f.suffix not in ['.log','.json']:raise RuntimeError('Text evidence only')
  s=f.read_text(errors='replace')
  for old,new in [(str(work),'<WORK>'),(str(source),'<OFFICIAL>'),(str(project.parents[1]),'<WORKSPACE>')]:s=s.replace(old,new).replace(old.replace('\\','/'),new).replace(old.replace('\\','\\\\'),new)
  f.write_text(s)
 (ev/'evidence-sha256.json').write_text(json.dumps({f.name:{'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())} for f in ev.iterdir()},indent=2)+'\n')
