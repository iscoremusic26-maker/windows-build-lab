import json,os,re,shutil,subprocess,sys,zipfile
from pathlib import Path
from common import *
import audit,provider_audit,source_gate,abi_probe
WORK=Path(os.environ['RUNNER_TEMP'])/'devf-cp20';BUILD=WORK/'build';EVIDENCE=WORK/'evidence';DIST=WORK/'deliver';OUT=WORK/'artifact'
for p in [WORK,BUILD,EVIDENCE,DIST,OUT]:p.mkdir(parents=True,exist_ok=True)
pin=json.loads((TASK/'SOURCE_CI_CP20.json').read_text());source=pin['source_commit']
result=dict(status='FAILED',source_commit=source,ci_commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),run_id=os.environ['GITHUB_RUN_ID'],game_executed=False,phases=[])
def run(label,args,cwd=None):
 p=subprocess.run([str(x) for x in args],cwd=cwd,capture_output=True,text=True,encoding='utf-8',errors='replace');s=p.stdout+p.stderr
 (EVIDENCE/(label+'.log')).write_text(s,encoding='utf-8');result['phases'].append(dict(label=label,exit=p.returncode));print(label,p.returncode,flush=True)
 if p.returncode:
  fail=WORK/'app-tests/FAIL.txt'
  if fail.exists():print(fail.read_text(),flush=True)
  print(s[-16000:],flush=True);raise RuntimeError(label)
 return s
try:
 assert os.name=='nt' and os.environ['VSCMD_ARG_TGT_ARCH']=='x64'
 for name,h in pin['source_blobs'].items():assert git_blob((REPO/name).read_bytes())==h,('PINNED_BLOB',name)
 result['pinned_blobs_verified']=len(pin['source_blobs']);result['freeze']=source_gate.check(REPO,TASK)
 baseline=Path(os.environ['RUNNER_TEMP'])/'devf-cp19f2';prior=json.loads((baseline/'evidence/result.json').read_text())
 assert prior['source_commit']=='a0dd9e5a780c413a27fd165c9d3133b14d1e8613' and prior['status']=='WINDOWS_BUILD_VERIFIED_PACKAGE_VERIFIED_DOWNLOAD_PENDING' and prior['game_executed'] is False
 assert all(p['exit']==0 for p in prior['phases']);result['baseline']=prior
 shutil.copytree(baseline/'evidence',EVIDENCE/'frozen-fix02',dirs_exist_ok=True)
 official=REPO/'.official';lib=baseline/'windows-amd64.lib'
 assert subprocess.check_output(['git','-C',official,'rev-parse','HEAD'],text=True).strip()==SOURCE_HOST
 run('configure',['cmake','-S',TASK,'-B',BUILD,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DDUSK_SOURCE='+str(official),'-DDEVF_SOURCE_SHA='+source,'-DDUSK_GAME_EXE='+str(lib)])
 targets=['bokoblin_expansion','devf_map_provider','cp20_lab','cp20_abi','cp20_variant_regress_lab','cp20_legacy_compat_lab','cp19f2_lab','cp14_regress_lab','cp19f2_abi']
 text=run('msvc',['cmake','--build',BUILD,'--target',*targets,'--parallel','2']);assert 'C4477' not in text and 'C4473' not in text
 matrix=WORK/'matrix';run('matrix-generation',[sys.executable,TASK/'tests/matrix.py',matrix])
 lab=run('cp20-lab',[BUILD/'cp20_lab.exe','--matrix',matrix]);assert 'CP20_LAB_PASS' in lab;result['cp20_lab_cases']=lab.count('CASE_PASS ')
 legacy=run('old-expansion-compatibility',[BUILD/'cp20_legacy_compat_lab.exe',matrix]);assert 'CP20_LEGACY_COMPAT_PASS cases=9' in legacy
 for target,marker in [('cp20_variant_regress_lab','VARIANT_LAB_PASS cases=30'),('cp19f2_lab','CP19F2_LAB_PASS cases=40'),('cp14_regress_lab','CP14_LAB_PASS cases=16')]:assert marker in run(target,[BUILD/(target+'.exe')])
 obj=list(BUILD.rglob('cp19f2_abi.cpp.obj'));assert len(obj)==1;result['source_abi']=abi_probe.verify(obj[0])
 app=WORK/'app';project=TASK/'app/DEVF.MapEditor.csproj'
 run('dotnet-publish',['dotnet','publish',project,'-c','Release','-r','win-x64','--self-contained','true','-p:PublishSingleFile=true','-p:IncludeNativeLibrariesForSelfExtract=true','-p:DebugType=None','-p:DebugSymbols=false','-o',app],project.parent)
 tests=WORK/'app-tests';run('editor-ui-and-parsers',[app/EXE_NAME,'--self-test',baseline/'R03_00.arc',tests,matrix])
 result['editor_tests']={name:json.loads((tests/(name+'-result.json')).read_text()) for name in ['cp20','cp13','multi','importer','names']}
 assert result['editor_tests']['cp20']['status']=='CP20_EDITOR_LAB_PASS'
 assert result['editor_tests']['cp13']['status']=='CP13_EDITOR_LAB_PASS' and result['editor_tests']['multi']['cases']>=109
 shutil.copyfile(tests/'cp20-synthetic-map-markers.png',EVIDENCE/'cp20-synthetic-map-markers.png')
 result['editor_pe']=app_pe(app/EXE_NAME)
 for export in ['multi-encounters.json','cp20-mixed-export.json']:run('CSharp-export-'+export,[BUILD/'cp20_lab.exe','--export',tests/export])
 dll=BUILD/'bokoblin_expansion.dll';pdll=BUILD/'devf_map_provider.dll'
 result['expansion_negatives']=audit.negatives(dll,WORK);result['provider_negatives']=provider_audit.prior.negatives(pdll,WORK,'provider')
 run('package',['cmake','--build',BUILD,'--target','bokoblin_expansion_package','devf_map_provider_package'])
 result['expansion']=audit.verify_package(BUILD/'mods/bokoblin_expansion.dusk',dll,TASK)
 result['provider']=provider_audit.verify(BUILD/'mods/devf_map_provider.dusk',pdll)
 for name,p in [(DUSK_NAME,BUILD/'mods/bokoblin_expansion.dusk'),(PROVIDER_NAME,BUILD/'mods/devf_map_provider.dusk'),(EXE_NAME,app/EXE_NAME)]:shutil.copyfile(p,DIST/name)
 for label,path in [('expansion',dll),('provider',pdll)]:
  for what in ['headers','imports','exports']:run(label+'-'+what,['dumpbin','/'+what,path])
 for name in ['README.md','IMPLEMENTATION.md','SOURCE_AUDIT.md','HUMAN_TEST.md','RECOVERY.md','TEST_RESULTS.md']:shutil.copyfile(TASK/name,DIST/name)
 shutil.copytree(TASK/'examples',DIST/'examples')
 result['source_ledger']={name:sha((REPO/name).read_bytes()) for name in pin['source_blobs']}
 with zipfile.ZipFile(DIST/'SOURCE_COMPILED.zip','w',zipfile.ZIP_DEFLATED) as z:
  for name in pin['source_blobs']:z.write(REPO/name,name)
 shutil.copyfile(TASK/'SOURCE_CI_CP20.json',DIST/'SOURCE_PIN.json')
 result['status']='WINDOWS_MSVC_PASS_DOWNLOAD_PENDING'
 (DIST/'TEST_RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 for p in DIST.rglob('*'):
  if p.is_file():assert p.suffix.lower() not in ['.dll','.arc','.kcl','.plc','.bmd','.bti','.iso','.ciso'],p
 manifest=dict(source_commit=source,ci_commit=result['ci_commit'],run_id=result['run_id'],host=SOURCE_HOST,human='HUMAN_RUNTIME_PENDING',game_executed=False,required_pair={'provider':'0.14.0','expansion':'0.16.3','editor':'0.14.0'},expansion=result['expansion'],provider=result['provider'],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in sorted(DIST.rglob('*')) if p.is_file()})
 (DIST/'VERIFICATION_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n');(DIST/'SHA256SUMS.txt').write_text(''.join(m['sha256']+'  '+n+'\n' for n,m in manifest['files'].items()))
 bundle=OUT/BUNDLE_NAME
 with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in sorted(DIST.rglob('*')):
   if p.is_file():z.write(p,p.relative_to(DIST).as_posix())
 with zipfile.ZipFile(bundle) as z:assert z.testzip() is None
 result['zip']=dict(filename=bundle.name,bytes=bundle.stat().st_size,sha256=sha(bundle.read_bytes()))
 result['status']='WINDOWS_MSVC_PASS_DOWNLOAD_PENDING'
 assert not subprocess.check_output(['git','-C',official,'status','--porcelain','--untracked-files=no'],text=True).strip()
except Exception as e:result['error']=str(e);raise
finally:
 (EVIDENCE/'result.json').write_text(json.dumps(result,indent=2)+'\n');print('CP20_BUILD_RESULT='+json.dumps({k:v for k,v in result.items() if k not in ['baseline','source_ledger','freeze','editor_tests']},separators=(',',':')),flush=True)
