"""CP05 actual MSVC scopes; two inline stores and call to frozen mode writer.
Static bounded inspection, not a whole-program formal proof or DLL execution.
"""
from pathlib import Path
import sys,re,struct,json,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import pe_audit,store_audit,cp04_pe_audit
def timer_spans(code):
 return [m.span() for m in re.finditer(rb'\x66[\x40-\x47]?\x89[\x80-\xbf]\xe2\x08\x00\x00',code) if (code[m.end()-5]&7)!=4]
def stores(scopes):
 rate=cp04_pe_audit.rate_store_spans(scopes['cp05_native_rate']);timer=timer_spans(scopes['cp05_native_timer']);errors=[]
 if len(rate)!=1:errors.append('CP05 expected exactly one32bit morf+0x28 float store')
 if len(timer)!=1:errors.append('CP05 expected exactly one16bit E_OC+0x8e2 compare-write timer store')
 return errors,rate,timer
def addresses(mp,name):return [int(m[1],16) for m in re.finditer(r'^\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+\?'+name+r'@\S+\s+([0-9a-fA-F]{16})\s+f\s',mp,re.M)]
def main():
 dll,mapfile=map(Path,sys.argv[1:3]);raw=dll.read_bytes();mp=mapfile.read_text();scopes={n:store_audit.scoped_bytes(raw,mp,n) for n in ['cp05_native_rate','cp05_native_timer','cp05_native_mode']};errors,rate,timer=stores(scopes);negative=0
 for name in ['cp05_native_rate','cp05_native_timer']:
  changed=dict(scopes);code=bytearray(scopes[name])
  if name.endswith('rate') and len(rate)==1:code[rate[0][2]]=0x2c
  if name.endswith('timer') and len(timer)==1:code[timer[0][1]-4]=0xe0
  changed[name]=bytes(code)
  if changed[name]!=scopes[name] and stores(changed)[0]:negative+=1
 if negative!=2:errors.append('CP05 actual instruction negatives did not both reject')
 pe=pe_audit.PE(raw);imports,slots=pe.imports();old=addresses(mp,'cp04_native_mode');new=addresses(mp,'cp05_native_mode');bound=False
 if len(old)==len(new)==1:
  code=scopes['cp05_native_mode']
  for i in range(len(code)-4):
   if code[i] in (0xe8,0xe9) and new[0]+i+5+struct.unpack_from('<i',code,i+1)[0]==old[0]:bound=True
  # /OPT:ICF may coalesce an equivalent wrapper; the frozen destination still
  # has to bind the exact native import, checked independently below.
  if old[0]==new[0]:bound=True
 else:errors.append('CP05/frozen CP04 unique mode writer map scopes missing')
 if not bound:errors.append('CP05 mode call is not bound to frozen cp04_native_mode')
 oldcode=store_audit.scoped_bytes(raw,mp,'cp04_native_mode')
 native=len(old)==1 and cp04_pe_audit.calls_import(pe,raw,oldcode,old[0],slots,('dusklight.exe','?setActionMode@daE_OC_c@@QEAAXHH@Z'))
 if not native:errors.append('Frozen mode writer does not bind typed native setActionMode')
 result={'status':'CP05_PE_SCOPE_PASS' if not errors else 'CP05_PE_SCOPE_FAIL','scope_bytes':{n:len(v) for n,v in scopes.items()},'scope_sha256':{n:hashlib.sha256(v).hexdigest() for n,v in scopes.items()},'scope_hex':{n:v.hex() for n,v in scopes.items()},'stores':{'fresh_rate_32bit':len(rate),'native_timer_16bit':len(timer),'rate_encodings':[x[3] for x in rate]},'mode_call_bound_to_frozen_writer':bound,'frozen_writer_bound_to_typed_import':native,'actual_instruction_negatives_rejected':negative,'errors':errors,'native_runtime':False}
 print(json.dumps(result,indent=2));return bool(errors)
if __name__=='__main__':sys.exit(main())
