"""Verify the actual MSVC CP01 package and Windows synthetic laboratory; no game execution."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zipfile

def sha(path):
    with path.open('rb') as f: return hashlib.file_digest(f,'sha256').hexdigest()

parser=argparse.ArgumentParser()
parser.add_argument('--root',type=Path,required=True)
parser.add_argument('--work',type=Path,required=True)
a=parser.parse_args(); root=a.root.resolve(); work=a.work.resolve()
assert os.name=='nt', 'Real Windows required'
evidence=work/'evidence'; package=work/'build/mods/devf_cp01.dusk'
with zipfile.ZipFile(package) as z:
    assert z.testzip() is None
    members=[n for n in z.namelist() if not n.endswith('/')]
    assert set(members)=={'mod.json','lib/windows-amd64/mod.dll','res/safety-policy.txt'},members
    manifest=json.loads(z.read('mod.json')); assert manifest['id']=='dev.devf.cp01_lab'
    assert 'BUILD ONLY' in manifest['name']
    assert z.read('res/safety-policy.txt')==b'DISARMED_NO_REGISTRATION\n'
    dll=evidence/'mod.dll';dll.write_bytes(z.read('lib/windows-amd64/mod.dll'))
data=dll.read_bytes();assert data[:2]==b'MZ';pe=struct.unpack_from('<I',data,0x3c)[0]
assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x8664
assert struct.unpack_from('<H',data,pe+24)[0]==0x20b and struct.unpack_from('<H',data,pe+22)[0]&0x2000
compiler_files=list((work/'build/CMakeFiles').glob('*/CMakeCXXCompiler.cmake'));assert len(compiler_files)==1
compiler_text=compiler_files[0].read_text(encoding='utf-8')
def cv(name):
    m=re.search(r'set\('+name+r' "([^"]*)"\)',compiler_text);assert m,name;return m.group(1)
assert cv('CMAKE_CXX_COMPILER_ID')=='MSVC'
dumpbin=Path(cv('CMAKE_CXX_COMPILER')).with_name('dumpbin.exe')
for flag,name in [('/EXPORTS','exports.txt'),('/IMPORTS','imports.txt'),('/HEADERS','headers.txt')]:
    p=subprocess.run([str(dumpbin),flag,str(dll)],capture_output=True,text=True,check=True)
    (evidence/name).write_text(p.stdout,encoding='utf-8')
exports=(evidence/'exports.txt').read_text(encoding='utf-8')
for symbol in ['mod_ctx','mod_meta','mod_initialize','mod_update','mod_shutdown']:
    assert re.search(r'\b'+symbol+r'\b',exports),symbol
imports=(evidence/'imports.txt').read_text(encoding='utf-8')
for required in ['dusklight.exe','mDoExt_J3DModel__create','dBgW']:
    assert required.lower() in imports.lower(),required
records={}
for service,version in {'actor':(1,0),'stage':(1,0),'resource':(1,1),'host':(2,2),'log':(1,0),'game':(2,0)}.items():
    key=('dev.twilitrealm.dusklight.'+service).encode()+b'\0'
    observed=[struct.unpack_from('<HBBHH',data,m.start()-16) for m in re.finditer(re.escape(key),data) if m.start()>=16]
    assert (80,2,0,*version) in observed,(service,observed)
    records[service]={'record_size':80,'major':version[0],'minimum_minor':version[1],'required':True}
lab=(evidence/'lifecycle-lab.log').read_text(encoding='utf-8-sig')
collision=re.search(r'CP01_COLLISION_PASS cases=(\d+) platform=Windows engine_runtime=NOT_TESTED',lab)
assert collision and int(collision.group(1))==6, 'Focused collision ownership cases did not pass'
assert 'CP01_LAB_PASS' not in lab and 'CP01_SAFETY_PASS' not in lab, 'Unaffected suites repeated'
shutil.copy2(work/'build/audit/host-functions.json',evidence/'host-functions.json')
configure=(evidence/'configure.log').read_text(encoding='utf-8-sig')
sdk=re.search(r'CP01 Windows SDK: ([\d.]+)',configure);assert sdk
build_only=evidence/'DEVF_CP01_BUILD_ONLY.dusk';shutil.copy2(package,build_only)
lab_binary=work/'build/RelWithDebInfo/devf_cp01_safety_lab.exe'
report={'status':'PASS_BUILD_AND_SYNTHETIC_LAB','compiler_id':'MSVC','compiler_version':cv('CMAKE_CXX_COMPILER_VERSION'),
    'windows_sdk':sdk.group(1),'configuration':'RelWithDebInfo','dll_sha256':sha(dll),'dusk_sha256':sha(build_only),
    'safety_lab_exe_sha256':sha(lab_binary),'new_collision_ownership_cases':int(collision.group(1)),
    'previously_passed_suites_not_rerun':['original_16','affected_regression_9','safety_14'],
    'service_records':records,'package_members':members,
    'runtime_game':'NOT_TESTED','dx12':'NOT_TESTED','safe_location':'NOT_VERIFIED','automatic_spawns':False,
    'release_classification':'DISARMED_BUILD_ONLY_NOT_APPROVED_FOR_TEST','profiles_registered':False,'cp02':'NOT_STARTED',
    'run_url':f"https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}"}
(evidence/'cp01-verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,sort_keys=True))
