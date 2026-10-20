import json,os,shutil,subprocess,sys,zipfile
from pathlib import Path
from common import *
import source_gate,input_negatives,package_audit,abi_probe
from generate_config import generate
WORK=Path(os.environ['RUNNER_TEMP'])/'devf-cp21';BUILD=WORK/'build';EVIDENCE=WORK/'evidence';DIST=WORK/'deliver';OUT=WORK/'artifact'
for p in [WORK,BUILD,EVIDENCE,DIST,OUT]:p.mkdir(parents=True,exist_ok=True)
pin=json.loads((TASK/'SOURCE_CI_CP21.json').read_text());source=pin['source_commit']
result=dict(status='FAILED',source_commit=source,ci_commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),run_id=os.environ['GITHUB_RUN_ID'],game_executed=False,human='HUMAN_RUNTIME_PENDING',phases=[])
def run(label,args,cwd=None):
 p=subprocess.run([str(x) for x in args],cwd=cwd,capture_output=True,text=True,encoding='utf-8',errors='replace');s=p.stdout+p.stderr
 (EVIDENCE/(label+'.log')).write_text(s,encoding='utf-8');result['phases'].append(dict(label=label,exit=p.returncode));print(label,p.returncode,flush=True)
 if p.returncode:print(s[-18000:],flush=True);raise RuntimeError(label)
 return s
try:
 assert os.name=='nt' and os.environ['VSCMD_ARG_TGT_ARCH']=='x64'
 assert __debug__,'Python asserts must remain enabled'
 for n,h in pin['source_blobs'].items():assert git_blob((REPO/n).read_bytes())==h,('CP21_PIN',n)
 result['pinned_blobs_verified']=len(pin['source_blobs']);result['freeze']=source_gate.check();result['inputs']=generate(BUILD/'cp21_frozen_encounters.hpp');result['input_negatives']=input_negatives.run()
 baseline=Path(os.environ['RUNNER_TEMP'])/'devf-cp20';prior=json.loads((baseline/'evidence/result.json').read_text())
 assert prior['status']=='WINDOWS_MSVC_PASS_DOWNLOAD_PENDING' and prior['source_commit']=='2ce550395ff69d2c9046a2d3e0b635024307ce1e' and prior['game_executed'] is False
 assert all(x['exit']==0 for x in prior['phases']);result['baseline']=prior
 official=REPO/'.official';lib=Path(os.environ['RUNNER_TEMP'])/'devf-cp19f2/windows-amd64.lib'
 assert subprocess.check_output(['git','-C',official,'rev-parse','HEAD'],text=True).strip()==SOURCE_HOST
 official_doc=(official/'docs/modding.md').read_bytes();assert git_blob(official_doc)=='a138ab329166fd5a9fde0e04bbd15a5d7af9d570'
 assert b'RES_DIR res' in official_doc and b'"icon": "res/icon.png"' in official_doc
 run('configure',['cmake','-S',TASK,'-B',BUILD,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DDUSK_SOURCE='+str(official),'-DDEVF_SOURCE_SHA='+source,'-DDUSK_GAME_EXE='+str(lib)])
 targets=['bokoblin_expansion','devf_map_provider','cp21_lab','cp20_lab','cp20_abi','cp20_variant_regress_lab','cp20_legacy_compat_lab','cp19f2_lab','cp14_regress_lab','cp19f2_abi']
 text=run('msvc',['cmake','--build',BUILD,'--target',*targets,'--parallel','2']);assert 'C4477' not in text and 'C4473' not in text
 lab=run('cp21-public-entrypoints',[BUILD/'cp21_lab.exe',WORK/'stale-player-files']);assert 'CP21_LAB_PASS cases=11' in lab;result['cp21_lab_cases']=lab.count('CASE_PASS ')
 matrix=WORK/'matrix';run('matrix-generation',[sys.executable,TASK/'tests/matrix.py',matrix]);text=run('cp20-regression',[BUILD/'cp20_lab.exe','--matrix',matrix]);assert 'CP20_LAB_PASS cases=141' in text
 for target,args,marker in [('cp20_legacy_compat_lab',[matrix],'CP20_LEGACY_COMPAT_PASS cases=9'),('cp20_variant_regress_lab',[],'VARIANT_LAB_PASS cases=30'),('cp19f2_lab',[],'CP19F2_LAB_PASS cases=40'),('cp14_regress_lab',[],'CP14_LAB_PASS cases=16')]:assert marker in run(target,[BUILD/(target+'.exe'),*args])
 obj=list(BUILD.rglob('cp19f2_abi.cpp.obj'));assert len(obj)==1;result['source_abi']=abi_probe.verify(obj[0])
 run('package',['cmake','--build',BUILD,'--target','bokoblin_expansion_package','devf_map_provider_package'])
 for role,target,filename in [('expansion','bokoblin_expansion',DUSK_NAME),('provider','devf_map_provider',PROVIDER_NAME)]:
  dlls=list(BUILD.rglob(target+'.dll'));packages=list(BUILD.rglob(target+'.dusk'));assert len(dlls)==len(packages)==1,(dlls,packages)
  dll,package=dlls[0],packages[0]
  result[role]=package_audit.verify(package,dll,role);result[role+'_negatives']=package_audit.negatives(package,dll,role,WORK)
  if role=='expansion':result['inherited_expansion_negatives']=package_audit.prior.negatives(dll,WORK)
  else:result['inherited_provider_negatives']=package_audit.provider.negatives(dll,WORK,'provider')
  shutil.copyfile(package,DIST/filename)
  for what in ['headers','imports','exports']:run(role+'-'+what,['dumpbin','/'+what,dll])
 for n in ['README.md','HUMAN_TEST.md','RECOVERY.md']:shutil.copyfile(TASK/n,DIST/n)
 # CP20's exact compiled regression snapshot is included separately from CP21's source ledger.
 inherited=json.loads((TASK.parent/'CP20_EXPLICIT_BLUE_RED_BLACK/SOURCE_CI_CP20.json').read_text())
 ledger=dict(inherited['source_blobs']);ledger.update(pin['source_blobs']);result['source_ledger']={n:sha((REPO/n).read_bytes()) for n in ledger}
 with zipfile.ZipFile(DIST/'SOURCE_COMPILED.zip','w',zipfile.ZIP_DEFLATED) as z:
  for n,h in ledger.items():assert git_blob((REPO/n).read_bytes())==h;z.write(REPO/n,n)
 shutil.copyfile(TASK/'SOURCE_CI_CP21.json',DIST/'SOURCE_PIN.json')
 result['status']='WINDOWS_MSVC_PASS_DOWNLOAD_PENDING';(DIST/'TEST_RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 manifest=dict(source_commit=source,inherited_regression_source_commit=inherited['source_commit'],ci_commit=result['ci_commit'],run_id=result['run_id'],host=SOURCE_HOST,human='HUMAN_RUNTIME_PENDING',game_executed=False,required_pair={'provider':'0.15.0','expansion':'0.17.0'},expansion=result['expansion'],provider=result['provider'],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in sorted(DIST.rglob('*')) if p.is_file()})
 (DIST/'VERIFICATION_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n');(DIST/'SHA256SUMS.txt').write_text(''.join(m['sha256']+'  '+n+'\n' for n,m in manifest['files'].items()))
 bundle=OUT/BUNDLE_NAME
 with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in sorted(DIST.iterdir()):z.write(p,p.name)
 with zipfile.ZipFile(bundle) as z:assert z.testzip() is None
 result['zip']=dict(filename=bundle.name,bytes=bundle.stat().st_size,sha256=sha(bundle.read_bytes()))
 assert not subprocess.check_output(['git','-C',official,'status','--porcelain','--untracked-files=no'],text=True).strip()
except Exception as e:result['error']=str(e);raise
finally:
 (EVIDENCE/'result.json').write_text(json.dumps(result,indent=2)+'\n');print('CP21_BUILD_RESULT='+json.dumps({k:v for k,v in result.items() if k not in ['baseline','source_ledger']},separators=(',',':')),flush=True)
