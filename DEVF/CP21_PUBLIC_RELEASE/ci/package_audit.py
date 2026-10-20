"""Audit actual bundles, including immutable original PNG bytes and public native metadata."""
import importlib.util,json,zipfile,stat,struct,sys
from pathlib import Path
from common import *
sys.path.insert(0,str(TASK/'inputs'))
from verify_inputs import ASSETS,png
import provider_core_audit as provider
spec=importlib.util.spec_from_file_location('cp21_prior_expansion_audit',TASK.parent/'CP20_EXPLICIT_BLUE_RED_BLACK/ci/audit.py');prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)
FORBIDDEN=[b'DEVF Editor',b'EVF Provider',b'DEVF Provider',b'CP20 TEST',b'FIELD / NORTH',b'Diseno de encuentros',b'Dise\xc3\xb1o de encuentros',b'encounter::Ui',b'editor7::Ui',b'dev.twilitrealm.dusklight.ui',b'placements.json',b'encounters.json',b'ENCOUNTERS_IO_UNAVAILABLE']

def inspect(path,role):
 raw=path.read_bytes()
 for word in FORBIDDEN:
  if word in raw or word.decode('utf-8').encode('utf-16-le') in raw:raise RuntimeError('FORBIDDEN_PUBLIC_STRING '+repr(word))
 return prior.inspect_pe(path) if role=='expansion' else provider.inspect_pe(path,'provider')

def verify(package,dll,role):
 with zipfile.ZipFile(package) as z:
  infos=z.infolist();names=[i.filename for i in infos];files={i.filename for i in infos if not i.is_dir()}
  assert len(names)==len(set(names))==len({n.casefold() for n in names})
  assert files=={'mod.json','lib/windows-amd64/mod.dll','res/icon.png','res/banner.png'}
  assert z.testzip() is None and sum(i.file_size for i in infos)<5_000_000
  for i in infos:
   n=i.filename;mode=i.external_attr>>16
   assert not n.startswith(('/','\\')) and not any(x in n for x in ['\\',':']) and '..' not in n.split('/')
   assert not i.flag_bits&1 and not stat.S_ISLNK(mode) and stat.S_IFMT(mode) in [0,stat.S_IFREG,stat.S_IFDIR]
  manifest=z.read('mod.json');raw=z.read('lib/windows-amd64/mod.dll');assert raw==dll.read_bytes()
  assert manifest==(TASK/role/'mod.json').read_bytes()
  m=json.loads(manifest);ident,name,version=('dev.bokoblin.expansion','Bokoblin Expansion','0.17.0') if role=='expansion' else ('dev.devf.cp06_native_config_dual','Enemy Variant Framework','0.15.0')
  assert (m['id'],m['name'],m['version'],m['author'])==(ident,name,version,'Fran')
  for n in ['icon','banner']:
   path='res/'+n+'.png';b=z.read(path);h,dimensions=ASSETS[role+'/'+n+'.png'];assert m[n]==path and sha(b)==h;png(b,dimensions)
   assert b==(TASK/'inputs/assets'/role/(n+'.png')).read_bytes()
  members=[dict(path=i.filename,bytes=i.file_size,crc32=f'{i.CRC:08x}',sha256=sha(z.read(i))) for i in infos]
 return dict(status='PACKAGE_VERIFIED',package_bytes=package.stat().st_size,package_sha256=sha(package.read_bytes()),dll_bytes=len(raw),dll_sha256=sha(raw),manifest=m,members=members,pe=inspect(dll,role),original_PNG_bytes_verified=True)

def negatives(package,dll,role,work):
 inspect(dll,role);rows=[];raw=dll.read_bytes()
 for label,mut in [('wrong-machine',lambda b:b[:]),('forbidden-ui',lambda b:b+b'DEVF Editor'),('forbidden-ui-service',lambda b:b+b'dev.twilitrealm.dusklight.ui')]:
  b=bytearray(mut(raw))
  if label=='wrong-machine':struct.pack_into('<H',b,struct.unpack_from('<I',b,60)[0]+4,0x14c)
  f=work/(role+'-'+label+'.dll');f.write_bytes(b)
  try:inspect(f,role)
  except (AssertionError,RuntimeError,ValueError,struct.error):rows.append(label)
  else:raise AssertionError('Negative binary survived '+label)
 with zipfile.ZipFile(package) as z:entries={n:z.read(n) for n in z.namelist() if not n.endswith('/')}
 for label in ['png-byte-change','png-missing','old-manifest','extra-editor','extra-encounters']:
  changed=dict(entries)
  if label=='png-byte-change':changed['res/icon.png']+=b'x'
  if label=='png-missing':del changed['res/banner.png']
  if label=='old-manifest':m=json.loads(changed['mod.json']);m['version']='0.14.0';changed['mod.json']=json.dumps(m).encode()
  if label=='extra-editor':changed['DEVF Editor.exe']=b'negative'
  if label=='extra-encounters':changed['encounters.json']=b'{}'
  f=work/(role+'-'+label+'.dusk')
  with zipfile.ZipFile(f,'w') as z:
   for n,b in changed.items():z.writestr(n,b)
  try:verify(f,dll,role)
  except (AssertionError,RuntimeError,ValueError):rows.append(label)
  else:raise AssertionError('Negative package survived '+label)
 return rows
