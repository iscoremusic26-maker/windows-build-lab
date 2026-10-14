"""CP03: reuse reviewed PE/ZIP readers; permit only one typed SDK Hook MEM target."""
import inspect,json,struct,hashlib,importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
old=load('g5_cp03_helpers',ROOT/'CP01_G5_FIRST_SESSION_REJECTION_FIX/ci/audit.py')
sha=old.sha;safe_members=old.safe_members;pe_exports=old.pe_exports
source=inspect.getsource(old.pe.g2.pe_audit)
source=source.replace("[('host',2,2),('log',1,0),('game',2,0)]","[('host',2,2),('log',1,0),('game',2,0),('stage',1,0),('hook',1,1)]")
source=source.replace('len(records) != 4','len(records) != 7')
before="else: raise RuntimeError(f'Forbidden metadata kind {kind}, including hooks/exports')"
after="""elif kind == 5:
            if size != 72 or flags != 0 or u32(n+4) != 0: raise RuntimeError('Unexpected typed hook record')
            if any(b[n+16:n+32]): raise RuntimeError('PMF adjustment/resolved slot must be initially zero')
            names = [s.decode('ascii') for s in b[n+32:n+size].split(b'\\0') if s]
            if names != ['??_7daE_OC_c@@6B@', 'daE_OC_c::execute']: raise RuntimeError('Unexpected hook target names '+str(names))
            records.append({'kind':'hook_mem','target':'daE_OC_c::execute','size':size,'pointer_to_member_bytes':8})
        else: raise RuntimeError('Forbidden metadata kind '+str(kind))"""
assert before in source;source=source.replace(before,after)
namespace={'struct':struct,'GAME_IMPORT_BASES':{'execute','dComIfGp_getStartStageName','getLayerNo','fpcM_IsCreating','fopAcIt_Judge','fpcSch_JudgeByID'}}
exec(source,namespace)
def inspect_pe(path):
 r=namespace['pe_audit'](path);r['exports']=pe_exports(path)
 raw=path.read_bytes();_,opt,_=old.package.pe_parts(raw)
 for directory in [9,13]:
  if any(struct.unpack_from('<II',raw,opt+112+8*directory)):raise RuntimeError('TLS/delay import directory forbidden')
 native=json.loads((Path(__file__).parent/'reviewed-game-exports.json').read_text())
 if sorted(r['imports'].get('dusklight.exe',[]))!=native:raise RuntimeError('Unreviewed game import identity '+str(r['imports']))
 allowed=json.loads((Path(__file__).parent/'reviewed-crt-surface.json').read_text())
 for dll,names in r['imports'].items():
  if dll=='dusklight.exe':continue
  if dll not in allowed or len(names)!=len(set(names)) or not set(names)<=set(allowed[dll]):raise RuntimeError('Unreviewed CRT/system imports '+str(r['imports']))
 exact=Path(__file__).parent/'reviewed-exact-imports.json'
 actual={k:sorted(v) for k,v in r['imports'].items()}
 if not exact.is_file() or actual!=json.loads(exact.read_text()):raise RuntimeError('NEW_IMPORT_IDENTITY_REVIEW_REQUIRED '+json.dumps(actual,sort_keys=True))
 # The compact PMF must point to the linker-generated import thunk for the exact execute symbol.
 def offset(v):
  pe=struct.unpack_from('<I',raw,0x3c)[0]
  for i in range(struct.unpack_from('<H',raw,pe+6)[0]):
   q=opt+struct.unpack_from('<H',raw,pe+20)[0]+i*40;vs,vr,rs,off=struct.unpack_from('<4I',raw,q+8)
   if vr<=v<vr+max(vs,rs) and v-vr<rs:return off+v-vr
  raise RuntimeError('Unmapped hook RVA')
 pe=struct.unpack_from('<I',raw,0x3c)[0];meta=None
 for i in range(struct.unpack_from('<H',raw,pe+6)[0]):
  q=opt+struct.unpack_from('<H',raw,pe+20)[0]+i*40
  if raw[q:q+8].rstrip(b'\0')==b'modmeta':meta=struct.unpack_from('<4I',raw,q+8)
 if not meta:raise RuntimeError('Missing metadata')
 vs,vr,rs,off=meta;n=off;hookptr=None
 while n+8<=off+min(vs,rs):
  if not any(raw[n:n+8]):n+=8;continue
  size,kind,flags=struct.unpack_from('<HBB',raw,n)
  if kind==5:hookptr=struct.unpack_from('<Q',raw,n+8)[0]
  n+=size
 base=struct.unpack_from('<Q',raw,opt+24)[0];at=offset(hookptr-base)
 if raw[at:at+2]!=b'\xff\x25':raise RuntimeError('Hook PMF does not point to an import thunk')
 slot_rva=hookptr-base+6+struct.unpack_from('<i',raw,at+2)[0]
 # Bind that IAT slot to its import name, independent of the mutable function pointer at runtime.
 imp_rva=struct.unpack_from('<I',raw,opt+120)[0];q=offset(imp_rva);bound=None
 while any(raw[q:q+20]):
  ilt,_,_,name,iat=struct.unpack_from('<5I',raw,q);t=offset(ilt or iat);index=0
  while (v:=struct.unpack_from('<Q',raw,t+8*index)[0]):
   if iat+8*index==slot_rva:
    p=offset(v)+2;bound=raw[p:raw.index(0,p)].decode()
   index+=1
  q+=20
 if bound!='?execute@daE_OC_c@@QEAAHXZ':raise RuntimeError('Hook thunk bound to wrong symbol '+str(bound))
 r.update(status='CP03_HOOK_PE_PASS',hook_target_bound_import=bound,manual_detour_imports=False,metadata_records=7)
 return r
def verify_package(path,dll,project,work):
 pe=inspect_pe(dll)
 with __import__('zipfile').ZipFile(path) as z:
  members=safe_members(z);raw=z.read('mod.json');inside=z.read('lib/windows-amd64/mod.dll')
  if {i.filename for i in z.infolist() if i.is_dir()}!={'lib/','lib/windows-amd64/'}:raise RuntimeError('Official SDK package shape changed')
  if raw!=(project/'mod.json').read_bytes() or inside!=dll.read_bytes():raise RuntimeError('Manifest/DLL byte mismatch')
 manifest=json.loads(raw)
 if manifest['id']!='dev.devf.cp03_stage_hook_hp150' or manifest['version']!='0.3.0':raise RuntimeError('Wrong CP03 identity')
 f=work/'internal-inspection.dll';f.write_bytes(inside)
 if inspect_pe(f)!=pe:raise RuntimeError('Internal PE mismatch')
 return {'status':'CP03_PACKAGE_VERIFIED','package_bytes':path.stat().st_size,'package_sha256':sha(path.read_bytes()),'dll_bytes':len(inside),'dll_sha256':sha(inside),'manifest_sha256':sha(raw),'manifest':manifest,'members':members,'pe_audit':pe,'dll_internal_equals_loose':True,'runtime_tested':False}
def negatives(dll,work):
 raw=dll.read_bytes();out=[]
 def bad(name,b):
  f=work/('negative-'+name.replace('::','-')+'.dll');f.write_bytes(b)
  try:inspect_pe(f)
  except (RuntimeError,ValueError,StopIteration,struct.error):out.append({'case':name,'rejected':True,'DLL_loaded':False})
  else:raise RuntimeError('Surviving negative '+name)
 pe,_,_=old.package.pe_parts(raw);b=bytearray(raw);struct.pack_into('<H',b,pe+4,0x14c);bad('x86',b)
 for old_name,new in [(b'dev.twilitrealm.dusklight.hook',b'dev.twilitrealm.dusklight.hoox'),(b'daE_OC_c::execute',b'daE_OC_c::executX'),(b'mod_update',b'mod_updatX')]:
  assert len(old_name)==len(new) and old_name in raw;bad(new.decode(),raw.replace(old_name,new))
 return out
