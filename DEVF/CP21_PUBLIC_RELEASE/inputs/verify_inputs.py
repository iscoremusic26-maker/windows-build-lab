import hashlib,json,struct,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent
ASSETS={
'expansion/icon.png':('d86ced394d2671152bccf7e37a0fbfc88a2cedeea7feadeac223e44e551d67de',(512,512)),
'expansion/banner.png':('24544c1b511e1e8735a7a34d7b5451ca31ede0fa958415174a351ce57ae18759',(1400,400)),
'provider/icon.png':('fe81d8c585836098559b3a84eba182adf3b7881e7b92e3c2de782f1c186c7f07',(512,512)),
'provider/banner.png':('f94701828dd0014328164da0a56a9e265aaf1671170a46a9328b45b47df6485a',(1400,400))}
def sha(b):return hashlib.sha256(b).hexdigest()
def png(b,dimensions):
 assert b[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',b[16:24])==dimensions
 n=8;types=[]
 while n<len(b):
  length=struct.unpack_from('>I',b,n)[0];kind=b[n+4:n+8];data=b[n+8:n+8+length];crc=struct.unpack_from('>I',b,n+8+length)[0]
  assert zlib.crc32(kind+data)&0xffffffff==crc
  types.append(kind);n+=length+12
 assert n==len(b) and types[0]==b'IHDR' and types[-1]==b'IEND' and b'IDAT' in types

def verify(root=ROOT):
 paths=[root/'assets'/n for n in ASSETS];missing=[str(p) for p in paths if not p.is_file()]
 assert not missing,('STOP_ASSETS_MISSING',missing)
 result={}
 for n,(h,d) in ASSETS.items():
  b=(root/'assets'/n).read_bytes();assert sha(b)==h,(n,sha(b));png(b,d);result[n]={'bytes':len(b),'sha256':h,'dimensions':d}
 b=(root/'encounters.json').read_bytes();lf=b.replace(b'\r\n',b'\n');original=lf.replace(b'\n',b'\r\n')
 assert len(lf)==18449 and sha(lf)=='786afe55dfadd0811cb53012aec7417f0886d2874cab63b0f9ae87ee01487e53'
 assert len(original)==19164 and sha(original)=='67a1d7109949e9a18071ff06592f24c2cf0d54093cf76cc07efd6dad730e74aa'
 j=json.loads(lf);rows=j['encounters'];assert len(rows)==32 and all(r['enabled'] for r in rows)
 from collections import Counter
 assert Counter(r['bokoblin_variant'] for r in rows)=={'blue':15,'red':7,'black':10}
 assert len(j['resources'])==6
 result['encounters']={'bytes':len(b),'sha256':sha(b),'original_crlf_bytes':len(original),'original_crlf_sha256':sha(original),'newline_difference_only':b!=original,'count':32}
 return result
if __name__=='__main__':print(json.dumps(verify(),indent=2))
