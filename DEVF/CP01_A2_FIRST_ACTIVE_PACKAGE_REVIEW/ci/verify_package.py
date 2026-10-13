"""Strict own package + downloaded artifact inspection; never load any DLL."""
import hashlib,importlib.util,json,stat,zipfile
from pathlib import Path
import audit
OLD=Path(__file__).resolve().parents[2]/'CP01_G4B_PACKAGE_DELIVERY/ci/verify_package.py'
spec=importlib.util.spec_from_file_location('old_package',OLD);old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha=old.sha;pe_exports=old.pe_exports
FILES={'mod.json','lib/windows-amd64/mod.dll'};DIRS={'lib/','lib/windows-amd64/'}
def safe_members(z,expected_files=FILES,allowed_dirs=DIRS):
 result=old.safe_members(z,expected_files,allowed_dirs)
 for i in z.infolist():
  mode=i.external_attr>>16
  if (i.is_dir() and stat.S_IFMT(mode) not in (0,stat.S_IFDIR)) or (not i.is_dir() and stat.S_ISDIR(mode)):raise RuntimeError('False directory attributes')
  if i.file_size and (i.compress_size==0 or i.file_size/max(1,i.compress_size)>1000):raise RuntimeError('Anomalous compressed ratio')
  if i.compress_type not in (zipfile.ZIP_STORED,zipfile.ZIP_DEFLATED):raise RuntimeError('Unreviewed compression')
 return result

def contents(package,expected_manifest,dll_bytes):
 with zipfile.ZipFile(package) as z:
  members=safe_members(z)
  if {i.filename for i in z.infolist() if i.is_dir()}!=DIRS:raise RuntimeError('SDK directory structure differs')
  manifest=z.read('mod.json')
  if manifest!=expected_manifest:raise RuntimeError('Manifest byte drift')
  m=json.loads(manifest)
  if m.get('id')!='dev.devf.cp01_eoc_first_active_pilot' or m.get('version')!='0.0.1' or set(m)!={'id','name','version','description'}:raise RuntimeError('Wrong active identity')
  if 'ACTIVE-CAPABLE' not in m['description'] or 'NATIVE CREATE_ACTOR' not in m['description'] or 'READ_ONLY' in m['description']:raise RuntimeError('False active manifest')
  inside=z.read('lib/windows-amd64/mod.dll')
  if inside!=dll_bytes:raise RuntimeError('DLL byte drift')
 return members,inside,m

def verify(package,dll,project,work):
 raw=dll.read_bytes();outside=audit.pe_audit(dll);exports=pe_exports(dll)
 members,inside,manifest=contents(package,(project/'mod.json').read_bytes(),raw)
 copy=work/'packaged-inspection.dll';copy.write_bytes(inside)
 if audit.pe_audit(copy)!=outside or pe_exports(copy)!=exports:raise RuntimeError('Internal binary audit differs')
 return {'status':'PACKAGE_STRUCTURE_PASS','package_bytes':package.stat().st_size,'package_sha256':sha(package.read_bytes()),'dll_bytes':len(raw),'dll_sha256':sha(raw),'manifest_sha256':sha((project/'mod.json').read_bytes()),'built_and_packaged_dll_byte_identical':True,'members':members,'manifest':manifest,'pe_audit':outside,'exports':exports,'dll_loaded':False}
