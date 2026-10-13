"""Semantic mutations of our own PE bytes only. Never load/execute any DLL."""
import json,struct
from audit import pe_audit,pe_exports,package

def run(dll,work):
 raw=dll.read_bytes();_,opt,offset=package.pe_parts(raw);results=[];base=pe_audit(dll)
 def check(name,changed,review=None):
  f=work/('negative-'+name+'.dll');f.write_bytes(changed)
  try:pe_audit(f,review)
  except (RuntimeError,ValueError,IndexError,struct.error,StopIteration):results.append({'case':name,'rejected':True,'loaded':False})
  else:raise RuntimeError('Surviving PE fixture '+name)
 def replace(name,old,new):
  if len(new)>len(old) or raw.count(old+b'\0')!=1:raise RuntimeError('Ambiguous mutation '+name)
  b=bytearray(raw);at=b.index(old+b'\0');b[at:at+len(old)+1]=new+b'\0'*(len(old)+1-len(new));check(name,b)
 game=base['imports']['dusklight.exe'];symbol=game[0].encode()
 replace('decorated-same-base-changed',symbol,symbol[:-1]+(b'X' if symbol[-1:]!=b'X' else b'Y'))
 replace('prohibited-native-setter',symbol,b'fpcLy_SetCurrentLayer')
 replace('prohibited-actor-create',symbol,b'fopAcM_create')
 replace('prohibited-system-import',symbol,b'CreateFileW')
 replace('changed-CRT-memcmp',b'memcmp',b'memcpy')
 replace('unexpected-DLL',b'dusklight.exe',b'untrusted.dll')
 # Fixed 64-byte metadata field permits longer service ID without changing record length.
 b=bytearray(raw);at=b.index(b'dev.twilitrealm.dusklight.game\0');value=b'dev.twilitrealm.dusklight.actor';b[at:at+64]=value+b'\0'*(64-len(value));check('metadata-ActorService',b)
 table=opt+struct.unpack_from('<H',raw,opt-4)[0];count=struct.unpack_from('<H',raw,opt-18)[0]
 meta=None
 for i in range(count):
  at=table+40*i
  if raw[at:at+8].rstrip(b'\0')==b'modmeta':meta=struct.unpack_from('<I',raw,at+20)[0]
 if meta is None:raise RuntimeError('Missing modmeta')
 b=bytearray(raw);struct.pack_into('<I',b,meta+4,2);check('ABI-version',b)
 b=bytearray(raw);struct.pack_into('<H',b,meta,7);check('metadata-size',b)
 b=bytearray(raw);struct.pack_into('<H',b,opt-20,0x14c);check('machine-x86',b)
 for index,name in [(9,'TLS-directory'),(13,'delay-import-directory')]:
  b=bytearray(raw);struct.pack_into('<II',b,opt+112+index*8,1,1);check(name,b)
 er,es=struct.unpack_from('<II',raw,opt+112);at=offset(er);nf,nn,fr,nr,orr=struct.unpack_from('<IIIII',raw,at+20)
 replace('export-name',b'mod_update',b'mod_updatX')
 b=bytearray(raw);struct.pack_into('<I',b,at+24,nn-1);check('ordinal-only-export',b)
 b=bytearray(raw);struct.pack_into('<I',b,offset(fr),er);check('forwarded-export',b)
 b=bytearray(raw);struct.pack_into('<H',b,offset(orr)+2,struct.unpack_from('<H',raw,offset(orr))[0]);check('aliased-export-ordinal',b)
 manifest=work/'negative-reviewed.json';altered=json.loads(json.dumps(base['imports']));altered['VCRUNTIME140.dll'].remove('memcmp');manifest.write_text(json.dumps(altered));check('review-manifest-missing-symbol',raw,manifest)
 altered['VCRUNTIME140.dll'].append('memcmp');altered['VCRUNTIME140.dll'].append('memcmp');manifest.write_text(json.dumps(altered));check('review-manifest-duplicate',raw,manifest)
 manifest.write_text('[]');check('review-manifest-invalid-schema',raw,manifest)
 check('review-manifest-absent',raw,work/'absent-reviewed.json')
 return results
