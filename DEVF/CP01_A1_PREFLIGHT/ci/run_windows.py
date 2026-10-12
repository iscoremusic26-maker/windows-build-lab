"""Build only the new observer/lab. Never invoke packaging, install or game targets."""
import argparse
import datetime
import hashlib
import json
import os
import subprocess
import time
import zipfile
from pathlib import Path
from audit import source_audit, pe_audit

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--inputs', type=Path, required=True)
p.add_argument('--work', type=Path, required=True)
a = p.parse_args()
project = Path(__file__).resolve().parents[1]
source = a.source.resolve(); work = a.work.resolve()
build = work / 'build'; evidence = work / 'evidence'
evidence.mkdir(parents=True, exist_ok=True); build.mkdir(parents=True, exist_ok=True)
ROOT = '40457c6adb381928e4b5fef6ed459ed291edd5e2'
AURORA = '3227d76c60e1e782ca576610bce61c9e7744d8be'
LIB_SHA = '4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4'
result = {'status':'BLOCKED','started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
          'run_id':os.getenv('GITHUB_RUN_ID'), 'workflow_sha':os.getenv('GITHUB_SHA'),
          'run_attempt':os.getenv('GITHUB_RUN_ATTEMPT'),
          'toolchain_env':{k:os.getenv(k) for k in ['VCToolsVersion','WindowsSDKVersion','VSCMD_ARG_TGT_ARCH']},
          'policy':'DISARMED_NO_REGISTRATION', 'game_executed':False,'host_built':False,
          'packaged_or_installed':False,'phases':[]}
def run(label, cmd, expected_fail=False):
    start = time.monotonic()
    with (evidence / f'{label}.log').open('w', encoding='utf-8') as log:
        cp = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, text=True)
    result['phases'].append({'label':label,'command':cmd,'seconds':round(time.monotonic()-start,3),'exit':cp.returncode})
    print(f'{label}: exit={cp.returncode}', flush=True)
    if (not expected_fail and cp.returncode) or (expected_fail and not cp.returncode):
        print((evidence / f'{label}.log').read_text(encoding='utf-8')[-16000:], flush=True)
        raise RuntimeError(f'{label} failed expectation')
def git(*args): return subprocess.check_output(['git','-C',str(source),*args],text=True).strip()
try:
    if os.name != 'nt': raise RuntimeError('Windows MSVC environment required')
    if git('rev-parse','HEAD') != ROOT: raise RuntimeError('Incorrect official root')
    aurora = subprocess.check_output(['git','-C',str(source/'extern/aurora'),'rev-parse','HEAD'],text=True).strip()
    if aurora != AURORA or git('status','--porcelain','--untracked-files=no'):
        raise RuntimeError('Official source or Aurora differs')
    source_snapshot = git('diff','--exit-code')
    archive = a.inputs / 'Dusklight-v2.0.3-win32-x86_64.zip'
    if archive.stat().st_size != 56037777 or archive.read_bytes()[:4] != b'PK\x03\x04':
        raise RuntimeError('Full original private ZIP required; LFS pointer rejected')
    with zipfile.ZipFile(archive) as z:
        candidates = [i for i in z.infolist() if i.filename.endswith('sdk/windows-amd64.lib')]
        if len(candidates) != 1: raise RuntimeError('Unique exact import library required')
        info = candidates[0]; data = z.read(info) # selected member CRC verified by zipfile
    if len(data) != 9400986 or hashlib.sha256(data).hexdigest() != LIB_SHA:
        raise RuntimeError('Selected CP00 import library hash/size differs')
    lib = work / 'windows-amd64.lib'; lib.write_bytes(data)
    result['dependencies'] = {'official_root':ROOT,'aurora':AURORA,'import_library_sha256':LIB_SHA,
        'import_library_bytes':len(data),'selected_member_crc32':f'{info.CRC:08x}',
        'archive_verification':'CP00 evidence reused; LFS object identity and selected library checked only',
        'expected_zip_lfs_sha256':'ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf'}
    result['source_audit'] = source_audit(project)
    # Exact own-code ledger: enables comparison of CI mirror with final main repository source.
    result['own_code'] = [{'path':str(f.relative_to(project)).replace('\\','/'),
                           'sha256':hashlib.sha256(f.read_bytes().replace(b'\r\n',b'\n')).hexdigest()}
                          for folder in ['src','tests','ci'] for f in sorted((project/folder).glob('*')) if f.is_file()]
    version = build / 'compiler-version.cpp'; version.write_text('int devf_observer_compiler_only;\n',encoding='utf-8')
    run('compiler',['cl','/Bv','/c',str(version),f'/Fo{build / "compiler-version.obj"}'])
    run('cmake-version',['cmake','--version'])
    run('configure',['cmake','-S',str(project),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
                     '-DCMAKE_C_COMPILER=cl','-DCMAKE_CXX_COMPILER=cl',f'-DDUSK_SOURCE={source}',f'-DDUSK_GAME_EXE={lib}'])
    run('build',['cmake','--build',str(build),'--target','devf_a1_observer','a1_observer_lab','--parallel','2'])
    run('lab',[str(build/'a1_observer_lab.exe')])
    guard = build / 'guard.cpp'; guard.write_text('#include "core.hpp"\nint guard_only;\n',encoding='utf-8')
    for label, defines, fail in [('guard-positive',['/DDEVF_READ_ONLY=1'],False),
            ('guard-mutation',['/DDEVF_READ_ONLY=0'],True),
            ('guard-hidden-spawn',['/DDEVF_READ_ONLY=1','/DDEVF_ENABLE_SPAWN=1'],True),
            ('guard-hidden-mutation',['/DDEVF_READ_ONLY=1','/DDEVF_ENABLE_MUTATION=1'],True)]:
        run(label,['cl','/nologo','/c','/std:c++20',f'/I{project/"src"}',*defines,str(guard),f'/Fo{build/"guard.obj"}'],fail)
    dll = build / 'devf_a1_observer.dll'
    run('module-imports',['dumpbin','/imports',str(dll)])
    run('module-exports',['dumpbin','/exports',str(dll)])
    exports = (evidence/'module-exports.log').read_text(encoding='utf-8')
    for name in ['mod_initialize','mod_update','mod_shutdown','mod_ctx','mod_meta']:
        if name not in exports: raise RuntimeError(f'Missing module export {name}')
    result['pe_audit'] = pe_audit(dll)
    result['own_dll'] = {'bytes':dll.stat().st_size,'sha256':hashlib.sha256(dll.read_bytes()).hexdigest(),
                         'uploaded':False,'executed':False}
    if list(build.rglob('*.dusk')): raise RuntimeError('Unexpected packaged output')
    if git('status','--porcelain','--untracked-files=no'): raise RuntimeError('Official source modified')
    result['official_source_unchanged'] = True
    result['lab_summary'] = (evidence/'lab.log').read_text(encoding='utf-8').splitlines()[-1]
    result['status'] = 'PASS'
finally:
    result['ended_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    (evidence/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
