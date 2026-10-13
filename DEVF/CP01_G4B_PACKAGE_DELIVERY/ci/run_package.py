"""Reuse approved G4B Windows validator, audit fresh DLL, then invoke official SDK package target explicitly."""
import argparse,datetime,hashlib,json,os,shutil,subprocess,time,re
from pathlib import Path
from verify_package import OBS,audit,verify,historical_difference,sha,pe_exports
from verify_negative_cases import run as negative_cases
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args()
source=a.source.resolve();work=a.work.resolve();project=Path(__file__).resolve().parents[1];ev=work/'package-evidence';ev.mkdir(parents=True,exist_ok=True);delivery=work/'delivery'
approval=json.loads((project/'SOURCE_APPROVAL.json').read_text());provenance=json.loads((project/'CI_SOURCE.json').read_text());build=work/'build'
r={'status':'BLOCKED','started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'approved_observer_checkpoint':approval['approved_checkpoint'],'packaging_source_commit':provenance['packaging_source_commit'],'ci_head':os.getenv('DEVF_CI_HEAD'),'run_id':os.getenv('GITHUB_RUN_ID'),'run_attempt':os.getenv('GITHUB_RUN_ATTEMPT'),'game_executed':False,'installed':False,'dll_loaded':False,'phases':[]}
def git(at,*args):return subprocess.check_output(['git','-C',str(at),*args],text=True).strip()
def run(label,cmd):
    start=time.monotonic()
    with (ev/(label+'.log')).open('w',encoding='utf-8') as out:cp=subprocess.run(cmd,stdout=out,stderr=subprocess.STDOUT,text=True)
    r['phases'].append({'label':label,'command':cmd,'exit':cp.returncode,'seconds':round(time.monotonic()-start,3)});print(label+': exit='+str(cp.returncode),flush=True)
    if cp.returncode:print((ev/(label+'.log')).read_text()[-12000:]);raise RuntimeError(label)
try:
    if os.name!='nt':raise RuntimeError('Real Windows/MSVC required')
    if os.getenv('GITHUB_EVENT_NAME')!='pull_request' or os.getenv('GITHUB_HEAD_REF')!='ci/devf-cp01-g4b-package' or os.getenv('GITHUB_BASE_REF')!='ci/devf-cp01-g4b-readonly':raise RuntimeError('Unexpected CI trigger/base')
    if git(project.parents[1],'rev-parse','HEAD')!=r['ci_head']:raise RuntimeError('Wrong checkout identity')
    r['source_ledger']={}
    for rel,h in approval['files_lf_sha256'].items():
        f=project.parent/rel;value=sha(f.read_bytes().replace(b'\r\n',b'\n'))
        if value!=h:raise RuntimeError('Approved source changed '+rel)
        r['source_ledger'][rel]=value
    if git(source,'rev-parse','HEAD')!=approval['official_root'] or git(source/'extern/aurora','rev-parse','HEAD')!=approval['aurora']:raise RuntimeError('SDK dependency differs')
    for rel,h in approval['sdk_files_lf_sha256'].items():
        if sha((source/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('SDK packager changed '+rel)
    run('approved-g4b-validation',['python',str(OBS/'ci/run_windows.py'),'--source',str(source),'--inputs',str(a.inputs),'--work',str(work)])
    prior=json.loads((work/'evidence/result.json').read_text())
    if prior.get('status')!='G4B_LOCAL_OBSERVER_PASS_LAB' or prior.get('binary')!='G4B_LOCAL_OBSERVER_BINARY_AUDIT_PASS':raise RuntimeError('Pre-package audit failed')
    r['msvc']='G4B_PACKAGE_MSVC_BUILD_PASS';r['fresh_build_validation']=prior
    compiler=re.search(r'Compiler Version ([0-9.]+) for x64',(work/'evidence/compiler.log').read_text())
    if not compiler:raise RuntimeError('Unidentified compiler version')
    linkmap=(work/'evidence/module-map.log').read_text()
    if any(x in linkmap for x in ['begin_simulation','Controller@','fake_capture','FakeEngine','LayerScope']):raise RuntimeError('Unexpected dormant/fake map symbols')
    dll=build/'devf_g4b_readonly.dll'
    r['new_dll_audit']=audit.pe_audit(dll);r['exports']=pe_exports(dll);r['historical_difference']=historical_difference(dll,approval)
    r['negative_package_cases']=negative_cases()
    if list(build.rglob('*.dusk')):raise RuntimeError('Premature package')
    before=dll.read_bytes()
    run('official-package',['cmake','--build',str(build),'--target','devf_g4b_readonly_package','--parallel','2'])
    if dll.read_bytes()!=before:raise RuntimeError('Packaging rebuilt/changed audited DLL')
    package=build/'mods/devf_g4b_readonly.dusk';r['package_verification']=verify(package,dll,work=work)
    if git(source,'status','--porcelain','--untracked-files=no'):raise RuntimeError('Official tracked source modified')
    r['dll_audit']='G4B_PACKAGE_DLL_AUDIT_PASS';r['structure']='G4B_PACKAGE_STRUCTURE_PASS';r['runner_status']='RUNNER_PACKAGE_VERIFIED_DOWNLOAD_PENDING';r['official_source_unchanged']=True
    name=f'DEVF_CP01_G4B_READ_ONLY_windows-amd64_{approval["approved_checkpoint"]}.dusk';r['filename']=name
    manifest={'filename':name,'module_id':'dev.devf.g4b_local_readonly','module_version':'0.0.1','approved_source_commit':approval['approved_checkpoint'],'prior_equivalent_compiled_source_commit':approval['prior_compiled_commit'],'packaging_source_commit':r['packaging_source_commit'],'workflow_commit':r['ci_head'],'workflow_run_id':r['run_id'],'official_sdk':approval['official_root'],'aurora':approval['aurora'],'import_library':prior['dependencies'],'compiler':'MSVC '+compiler[1],'toolchain_env':prior['toolchain_env'],'generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'package_sha256':r['package_verification']['package_sha256'],'package_bytes':r['package_verification']['package_bytes'],'dll_sha256':r['package_verification']['dll_sha256'],'dll_bytes':r['package_verification']['dll_bytes'],'manifest_sha256':sha((OBS/'mod.json').read_bytes()),'audits':{'msvc':r['msvc'],'dll':r['dll_audit'],'structure':r['structure']},'read_only':True,'policy':'DISARMED_NO_REGISTRATION','runtime':'RUNTIME_NOT_TESTED','installation_authorized':False,'execution_authorized':False,'historical_difference':r['historical_difference']}
    # Never expose a native output for artifact upload before all critical runner gates pass.
    if delivery.exists():raise RuntimeError('Unexpected existing delivery folder')
    delivery.mkdir();shutil.copyfile(package,delivery/name)
    (delivery/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n');(ev/'DOWNLOAD_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n');r['status']='RUNNER_PACKAGE_VERIFIED_DOWNLOAD_PENDING'
    (ev/'package-verification.json').write_text(json.dumps(r['package_verification'],indent=2)+'\n')
finally:
    r['ended_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(ev/'package-result.json').write_text(json.dumps(r,indent=2)+'\n')
    # Original validation evidence is text only; retain as a subdirectory without native binaries.
    if (work/'evidence').is_dir():shutil.copytree(work/'evidence',ev/'build-audit',dirs_exist_ok=True)
    replacements=[(str(work),'<WORK>'),(str(project.parents[1]),'<WORKSPACE>'),(str(source),'<OFFICIAL>')]
    for f in ev.rglob('*'):
        if not f.is_file():continue
        if f.suffix not in ('.json','.log'):raise RuntimeError('Unexpected non-text evidence')
        text=f.read_text(encoding='utf-8',errors='replace')
        for old,new in replacements:text=text.replace(old,new).replace(old.replace('\\','/'),new).replace(old.replace('\\','\\\\'),new)
        f.write_text(text,encoding='utf-8')
