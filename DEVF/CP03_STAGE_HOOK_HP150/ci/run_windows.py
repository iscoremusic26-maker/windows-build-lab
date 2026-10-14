import argparse,datetime,json,os,shutil,subprocess,time,zipfile
from pathlib import Path
from audit import sha,inspect_pe,verify_package,negatives
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args();project=Path(__file__).resolve().parents[1];source=a.source.resolve();work=a.work.resolve();build=work/'build';ev=work/'evidence';ev.mkdir(parents=True,exist_ok=True);build.mkdir(parents=True,exist_ok=True)
auth=json.loads((project/'SOURCE_MANIFEST.json').read_text());provenance=json.loads((project/'CI_SOURCE.json').read_text());r={'status':'BLOCKED','source_commit':provenance['source_commit'],'ci_head':os.getenv('DEVF_CI_HEAD'),'run_id':os.getenv('GITHUB_RUN_ID'),'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phases':[],'game_executed':False,'DLL_loaded':False,'host_built':False,'mode':'ONE_STAGE_ADDITION_OFFICIAL_HOOK_HP150','toolchain_env':{k:os.getenv(k) for k in ['VCToolsVersion','WindowsSDKVersion','VSCMD_ARG_TGT_ARCH']}}
def git(at,*args):return subprocess.check_output(['git','-C',str(at),*args],text=True).strip()
def run(label,cmd):
 start=time.monotonic()
 with (ev/(label+'.log')).open('w') as f:cp=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,text=True)
 r['phases'].append({'label':label,'command':cmd,'seconds':round(time.monotonic()-start,3),'exit':cp.returncode});print(label+': exit='+str(cp.returncode),flush=True)
 if cp.returncode:print((ev/(label+'.log')).read_text()[-10000:]);raise RuntimeError(label)
try:
 if os.name!='nt' or os.getenv('GITHUB_HEAD_REF')!='ci/devf-cp03-stage-hook-hp150' or os.getenv('GITHUB_BASE_REF')!='ci/devf-cp02-eoc2-native-elite':raise RuntimeError('Exclusive Windows PR required')
 if git(source,'rev-parse','HEAD')!=auth['official_root'] or git(source/'extern/aurora','rev-parse','HEAD')!=auth['aurora']:raise RuntimeError('SDK pin drift')
 for field,root in [('sdk_lf_sha256',source),('directed_sdk_source_sha256',source),('reuse_lf_sha256',project.parent)]:
  for rel,h in auth[field].items():
   if sha((root/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Pinned source drift '+rel)
 r['source_ledger']={f.relative_to(project).as_posix():sha(f.read_bytes().replace(b'\r\n',b'\n')) for f in project.rglob('*') if f.is_file() and f.suffix in ['.cpp','.hpp','.py','.json','.txt','.yml'] and 'evidence' not in f.parts and '__pycache__' not in f.parts and f.name!='CI_SOURCE.json'}
 archive=a.inputs/'Dusklight-v2.0.3-win32-x86_64.zip'
 if archive.stat().st_size!=56037777:raise RuntimeError('Complete original LFS ZIP required')
 with zipfile.ZipFile(archive) as z:entry=z.getinfo('sdk/windows-amd64.lib');data=z.read(entry);host=z.read('dusklight.exe')
 if len(data)!=9400986 or sha(data)!=auth['importlib_sha256'] or entry.CRC!=0x70c35731:raise RuntimeError('Private library hash/CRC/size mismatch')
 if sha(host)!=auth['host_sha256']:raise RuntimeError('Pinned official host mismatch; do not use other 2.0.3 binaries')
 lib=work/'windows-amd64.lib';lib.write_bytes(data);r['dependencies']={'SDK':auth['official_root'],'Aurora':auth['aurora'],'lib_bytes':len(data),'lib_sha256':sha(data),'lib_CRC':'70c35731'}
 f=build/'version.cpp';f.write_text('int compiler_only;\n');run('compiler',['cl','/Bv','/c',str(f),'/Fo'+str(build/'version.obj')]);run('cmake-version',['cmake','--version'])
 run('configure',['cmake','-S',str(project),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',f'-DDUSK_SOURCE={source}',f'-DDUSK_GAME_EXE={lib}','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 run('build',['cmake','--build',str(build),'--target','devf_cp03_stage_hook_hp150','--parallel','2'])
 cases=['registration_success','reentry','stage_fail','zero_handle','install_fail','install_throw','missing_hook','hook_minor0','bad_host','timing_and_damage','originals','room_reload','ambiguous_or_recycled','incomplete_and_late','detach_disarm']
 proof=json.loads((project/'ci/lab-reuse.json').read_text())
 if proof['cases']!=cases or proof['each_exit']!=0 or proof['toolchain_env']!=r['toolchain_env'] or proof['sdk']!=auth['official_root'] or proof['aurora']!=auth['aurora'] or proof['importlib_sha256']!=auth['importlib_sha256']:raise RuntimeError('Unchanged laboratory provenance drift')
 for rel,h in proof['file_lf_sha256'].items():
  if sha((project/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Laboratory source changed; reuse forbidden '+rel)
 for case in cases:
  log=project/'evidence/attempt-38511192326'/('fake-'+case+'.log')
  if 'PASS TEST_FAKE case='+case not in log.read_text():raise RuntimeError('Missing positive prior test '+case)
  shutil.copyfile(log,ev/log.name)
 (ev/'lab-reuse.json').write_text(json.dumps(proof,indent=2)+'\n')
 print('15 focused TEST_FAKE cases reused from38511192326; identical C++/headers/flags/SDK/toolchain',flush=True)
 r['fake_scenarios']={'cases':cases,'passed':len(cases),'each_separate_process':True,'game_or_DLL_runtime':False,'actual_policy_and_registration_headers':True,'reused_from_windows_run':38511192326,'unchanged_test_graph_verified':True,'actual_DLL_callback_not_executed':True,'payload_bytes':'32 BE ACTR, E_oc/FFFF0100/(-1250,-6304.0083,60750)/(262,0,0)/FFFF','reason_reexecuted':'New HookService ordering, registration rollback and own-instance HP policy; no historical suites repeated'}
 dll=build/'devf_cp03_stage_hook_hp150.dll'
 for label,arg in [('headers','/headers'),('imports','/imports'),('exports','/exports')]:run('module-'+label,['dumpbin',arg,str(dll)])
 linkmap=(build/'devf_cp03_stage_hook_hp150.map').read_text()
 for bad in ['NativeBackend','ActorService','fopAcM_create','fpcSCtRq_Request','fpcLy_SetCurrentLayer','VirtualProtect','WriteProcessMemory','funchook_install','Fake']:
  if bad in linkmap:raise RuntimeError('Forbidden module symbol '+bad)
 shutil.copyfile(build/'devf_cp03_stage_hook_hp150.map',ev/'module-map.log');r['binary_audit']=inspect_pe(dll);r['PE_negatives']=negatives(dll,work)
 (ev/'binary-audit.json').write_text(json.dumps(r['binary_audit'],indent=2)+'\n')
 commands=json.loads((build/'compile_commands.json').read_text());units=[x for x in commands if 'devf_cp03_stage_hook_hp150.dir' in x['command']]
 if len(units)!=2 or any('/tests/' in x['file'].replace('\\','/') for x in units):raise RuntimeError('Production graph changed')
 r['production_units']=units;(ev/'compile-commands.json').write_text(json.dumps(commands,indent=2)+'\n');before=dll.read_bytes();r['loose_dll']={'bytes':len(before),'sha256':sha(before)}
 run('official-package',['cmake','--build',str(build),'--target','devf_cp03_stage_hook_hp150_package','--parallel','2'])
 if dll.read_bytes()!=before:raise RuntimeError('Package rebuild drift')
 package=build/'mods/devf_cp03_stage_hook_hp150.dusk';r['package_verification']=verify_package(package,dll,project,work)
 if git(source,'status','--porcelain','--untracked-files=no') or git(source/'extern/aurora','status','--porcelain','--untracked-files=no'):raise RuntimeError('Official sources changed')
 name='DEVF_CP03_STAGE_HOOK_HP150_windows-amd64_'+r['source_commit']+'.dusk';pv=r['package_verification'];meta={'filename':name,'source_commit':r['source_commit'],'workflow_commit':r['ci_head'],'workflow_run_id':r['run_id'],'official_sdk':auth['official_root'],'aurora':auth['aurora'],'importlib_sha256':auth['importlib_sha256'],'module_id':pv['manifest']['id'],'version':pv['manifest']['version'],'parameters':'0xFFFF0100','angle_x':'0x0106','resource_selected_by_source':'E_OC2','package_bytes':pv['package_bytes'],'package_sha256':pv['package_sha256'],'dll_bytes':pv['dll_bytes'],'dll_sha256':pv['dll_sha256'],'adds_native_actor_entry':True,'read_only':False,'ActorService_used':False,'registered_custom_profiles':False,'installed':False,'runtime_tested':False,'execution_authorized':False,'CP03_RUNTIME':'HUMAN_RUNTIME_NOT_TESTED','CP04':'NOT_AUTHORIZED','HookService_used':True,'expected_initial_hp_by_code':150,'confirmed_CP01_location_user_report':'F_SP121 room3 layer6 (-1250,-6304.0083,60750); see HUMAN_TEST.md'}
 delivery=work/'delivery';delivery.mkdir();shutil.copyfile(package,delivery/name);(delivery/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(meta,indent=2)+'\n');(ev/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(meta,indent=2)+'\n');r['status']='CP03_PACKAGE_PASS_DOWNLOAD_PENDING'
except Exception as ex:r['error']=str(ex);raise
finally:
 r['ended_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(ev/'result.json').write_text(json.dumps(r,indent=2)+'\n')
 for f in ev.iterdir():
  if f.suffix not in ['.log','.json']:raise RuntimeError('Text evidence only')
  s=f.read_text(errors='replace')
  for old,new in [(str(work),'<WORK>'),(str(source),'<OFFICIAL>'),(str(project.parents[1]),'<WORKSPACE>')]:s=s.replace(old,new).replace(old.replace('\\','/'),new).replace(old.replace('\\','\\\\'),new)
  f.write_text(s)
 (ev/'evidence-sha256.json').write_text(json.dumps({f.name:{'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())} for f in ev.iterdir()},indent=2)+'\n')
