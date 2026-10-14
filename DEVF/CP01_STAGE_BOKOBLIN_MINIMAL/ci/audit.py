"""StageService-only contract; reuses historical PE/ZIP inspection, never loads DLL."""
import inspect,json,struct,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
import importlib.util
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
old=load('g5_helpers',ROOT/'CP01_G5_FIRST_SESSION_REJECTION_FIX/ci/audit.py')
sha=old.sha;safe_members=old.safe_members;pe_exports=old.pe_exports
# Reuse reviewed structural parser, changing only this module's exact service/native contract.
source=inspect.getsource(old.pe.g2.pe_audit)
before="[('host',2,2),('log',1,0),('game',2,0)]"
assert source.count(before)==1 and source.count('len(records) != 4')==1
source=source.replace(before,"[('host',2,2),('log',1,0),('game',2,0),('stage',1,0)]").replace('len(records) != 4','len(records) != 5')
namespace={'struct':struct,'GAME_IMPORT_BASES':set()};exec(source,namespace)
def inspect_pe(path):
 result=namespace['pe_audit'](path);exports=pe_exports(path)
 raw=path.read_bytes();_,opt,_=old.package.pe_parts(raw)
 for directory in [9,13]:
  if any(struct.unpack_from('<II',raw,opt+112+directory*8)):raise RuntimeError('Unexpected TLS/delay directory')
 allowed=json.loads((Path(__file__).parent/'reviewed-crt-surface.json').read_text())
 for dll,symbols in result['imports'].items():
  if dll not in allowed or len(set(symbols))!=len(symbols) or not set(symbols)<=set(allowed[dll]):raise RuntimeError('Unreviewed native/CRT import '+dll+': '+str(set(symbols)-set(allowed.get(dll,[]))))
 # CRT review is an upper bound from the previous SDK/MSVC module, not an assumption of identity.
 # The complete actual imported identity is preserved and linked to this build's DLL SHA.
 result.update(exports=exports,status='STAGE_ONLY_PE_PASS',game_imports=0,required_services='Host2.2 Log1.0 Stage1.0 Game2.0',permitted_game_mutation='StageService.add_actor via resolved function pointer only')
 return result

def verify_package(path,dll,project,work):
 pe=inspect_pe(dll)
 with __import__('zipfile').ZipFile(path) as z:
  members=safe_members(z);raw=z.read('mod.json');inside=z.read('lib/windows-amd64/mod.dll')
  if {i.filename for i in z.infolist() if i.is_dir()}!={'lib/','lib/windows-amd64/'}:raise RuntimeError('Official SDK directory shape changed')
  if raw!=(project/'mod.json').read_bytes() or inside!=dll.read_bytes():raise RuntimeError('Manifest/DLL byte mismatch')
 manifest=json.loads(raw)
 if manifest['id']!='dev.devf.cp01_stage_bokoblin_minimal' or manifest['version']!='0.1.0':raise RuntimeError('Wrong package identity')
 f=work/'internal-inspection.dll';f.write_bytes(inside)
 if inspect_pe(f)!=pe:raise RuntimeError('Internal PE mismatch')
 return {'status':'STAGE_PACKAGE_PASS','package_bytes':path.stat().st_size,'package_sha256':sha(path.read_bytes()),'dll_bytes':len(inside),'dll_sha256':sha(inside),'manifest_sha256':sha(raw),'manifest':manifest,'members':members,'pe_audit':pe,'dll_internal_equals_loose':True,'runtime_tested':False}

def negatives(dll,work):
 raw=dll.read_bytes();r=[]
 def bad(name,b):
  f=work/('negative-'+name+'.dll');f.write_bytes(b)
  try:inspect_pe(f)
  except (RuntimeError,ValueError,StopIteration,struct.error):r.append({'case':name,'rejected':True,'DLL_loaded':False})
  else:raise RuntimeError('Surviving negative '+name)
 pe,opt,_=old.package.pe_parts(raw);b=bytearray(raw);struct.pack_into('<H',b,pe+4,0x14c);bad('x86-machine',b)
 for old_name,new in [(b'dev.twilitrealm.dusklight.stage',b'dev.twilitrealm.dusklight.actor'),(b'mod_update',b'mod_updatX')]:
  assert raw.count(old_name+b'\0')==1 and len(new)==len(old_name);b=raw.replace(old_name+b'\0',new+b'\0');bad('ActorService' if b'actor' in new else 'export-name',b)
 return r
