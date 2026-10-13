import importlib.util,json,re,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
pe=load('g4_pe',ROOT/'CP01_G4B_LOCAL_READONLY_OBSERVER/ci/audit.py')
package=load('g4_package',ROOT/'CP01_G4B_PACKAGE_DELIVERY/ci/verify_package.py')
safe_members=package.safe_members;pe_exports=package.pe_exports
def parse_diagnostic(path):
 # Historical structural parser kept intact. Do NOT treat this as whole-surface audit.
 pe.g2.GAME_IMPORT_BASES |= pe.EXTRA
 return pe.g2.pe_audit(path)
def pe_audit(path):
 result=parse_diagnostic(path)
 expected_path=Path(__file__).parent/'reviewed-diagnostic-imports.json'
 if not expected_path.exists():raise RuntimeError('DIAGNOSTIC_IMPORT_REVIEW_PENDING; no distribution permitted')
 expected=json.loads(expected_path.read_text())
 if {k:sorted(v) for k,v in result['imports'].items()}!={k:sorted(v) for k,v in expected.items()}:raise RuntimeError('Exact diagnostic import identity mismatch')
 for symbols in result['imports'].values():
  for symbol in symbols:
   if any(x in symbol for x in ['CreateThread','WriteFile','CreateFile','VirtualProtect','LoadLibrary','GetProcAddress','fopAcM_create','fpcSCtRq_Request','fpcLy_SetCurrentLayer','fpcCtRq_Cancel','fopAcM_delete']):raise RuntimeError('Forbidden diagnostic import')
 result['status']='G5_DIAGNOSTIC_EXACT_PE_PASS';return result
sha=lambda b:hashlib.sha256(b).hexdigest()
def verify_package(path,dll,project,work):
 outside=pe_audit(dll);exports=pe_exports(dll)
 with __import__('zipfile').ZipFile(path) as z:
  members=safe_members(z);raw=z.read('mod.json');inside=z.read('lib/windows-amd64/mod.dll')
  if {i.filename for i in z.infolist() if i.is_dir()}!={'lib/','lib/windows-amd64/'}:raise RuntimeError('SDK structure mismatch')
  if raw!=(project/'mod.json').read_bytes() or inside!=dll.read_bytes():raise RuntimeError('Package manifest/DLL byte drift')
 manifest=json.loads(raw)
 if manifest['id']!='dev.devf.cp01_g5_rejection_diagnostic' or manifest['version']!='0.0.2' or 'READ_ONLY_DIAGNOSTIC' not in manifest['description']:raise RuntimeError('Wrong diagnostic identity')
 f=work/'internal-inspection.dll';f.write_bytes(inside)
 if pe_audit(f)!=outside or pe_exports(f)!=exports:raise RuntimeError('Internal PE mismatch')
 return {'package_bytes':path.stat().st_size,'package_sha256':sha(path.read_bytes()),'dll_bytes':len(inside),'dll_sha256':sha(inside),'manifest_sha256':sha(raw),'manifest':manifest,'members':members,'pe_audit':outside,'exports':exports,'byte_identical':True,'status':'DIAGNOSTIC_PACKAGE_PASS'}
