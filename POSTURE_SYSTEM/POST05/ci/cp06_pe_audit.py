"""CP06 actual compiled writer scope: the attributed chip restore performs exactly one
32-bit float store at g_dComIfG_gameInfo+0x98f0 (mItemInfo.mItemLifeCount) and one
byte store at +0x99e6 (mItemLifeCountType), through the imported game global, and
no other displaced store. Static bounded inspection; never executes the DLL.
"""
from pathlib import Path
import re,sys,json,struct,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import pe_audit,store_audit
GLOBAL=('dusklight.exe','?g_dComIfG_gameInfo@@3VdComIfG_inf_c@@A')
def spans(code,patterns):
 out=[]
 for pattern,kind in patterns:
  for m in re.finditer(pattern,code,re.S):
   modrm=code[m.start()+m.group(0).index(m.group('m'))]
   if (modrm&7)==4:continue  # SIB form: not a plain field displacement
   if kind.startswith('MOV32') and m.start()>0 and 0x48<=code[m.start()-1]<=0x4f:continue
   out.append((m.start(),m.end(),kind))
 return out
def life_stores(code):
 return spans(code,[(rb'\xf3[\x40-\x4f]?\x0f\x11(?P<m>[\x80-\xbf])\xf0\x98\x00\x00','MOVSS_F32'),(rb'[\x40-\x47]?\x89(?P<m>[\x80-\xbf])\xf0\x98\x00\x00','MOV32_FLOAT_BITS')])
def type_stores(code):
 return spans(code,[(rb'[\x40-\x4f]?\x88(?P<m>[\x80-\xbf])\xe6\x99\x00\x00','MOV8_REG'),(rb'[\x40-\x4f]?\xc6(?P<m>[\x80-\x87])\xe6\x99\x00\x00.','MOV8_IMM')])
def all_displaced_stores(code):
 pats=[(rb'\xf3[\x40-\x4f]?\x0f\x11(?P<m>[\x80-\xbf])....','MOVSS'),(rb'\xf2[\x40-\x4f]?\x0f\x11(?P<m>[\x80-\xbf])....','MOVSD'),(rb'[\x40-\x4f]?\x89(?P<m>[\x80-\xbf])....','MOV32_64'),
       (rb'[\x40-\x4f]?\x88(?P<m>[\x80-\xbf])....','MOV8'),(rb'\x66[\x40-\x4f]?\x89(?P<m>[\x80-\xbf])....','MOV16'),(rb'[\x40-\x4f]?\xc7(?P<m>[\x80-\x87])....','MOVIMM32'),(rb'[\x40-\x4f]?\xc6(?P<m>[\x80-\x87])....','MOVIMM8')]
 return spans(code,pats)
def global_loads(pe,code,start_va,slots):
 n=0
 for m in re.finditer(rb'[\x48-\x4f]\x8b([\x05\x0d\x15\x1d\x25\x2d\x35\x3d])',code):
  i=m.start()
  if i+7>len(code):continue
  slot=start_va-pe.image_base+i+7+struct.unpack_from('<i',code,i+3)[0]
  if slots.get(slot)==GLOBAL:n+=1
 return n
def check(code,pe,start_va,slots):
 errors=[];life=life_stores(code);kind=type_stores(code);every=all_displaced_stores(code);loads=global_loads(pe,code,start_va,slots)
 if len(life)!=1:errors.append('CP06 expected exactly one32bit store at gameInfo+0x98f0 (pending life count)')
 if len(kind)!=1:errors.append('CP06 expected exactly one byte store at gameInfo+0x99e6 (pending life type)')
 if len(every)!=len(life)+len(kind):errors.append('CP06 restore writer contains another displaced store')
 if loads<1:errors.append('CP06 restore writer base is not the imported g_dComIfG_gameInfo')
 return errors,{'life_store':len(life),'type_store':len(kind),'displaced_stores':len(every),'imported_global_loads':loads,'encodings':[x[2] for x in life+kind]}
def addresses(mp,name):return [int(m[1],16) for m in re.finditer(r'^\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+\?'+name+r'@\S+\s+([0-9a-fA-F]{16})\s+f\s',mp,re.M)]
def main():
 dll,mapfile=map(Path,sys.argv[1:3]);raw=dll.read_bytes();mp=mapfile.read_text();pe=pe_audit.PE(raw);_,slots=pe.imports()
 name='cp06_native_chip_restore';code=store_audit.scoped_bytes(raw,mp,name);va=addresses(mp,name)
 errors=[] if len(va)==1 else ['CP06 restore writer map scope missing or duplicated']
 e,counts=check(code,pe,va[0] if va else 0,slots);errors.extend(e)
 negative=0
 if not e:
  life=life_stores(code)[0];bad=bytearray(code);bad[life[1]-4]=0xf4;negative+=bool(check(bytes(bad),pe,va[0],slots)[0])  # other displacement
  t=type_stores(code)[0];bad=bytearray(code);bad[t[1]-4]=0xe7;negative+=bool(check(bytes(bad),pe,va[0],slots)[0])
  extra=code+b'\x89\x81\x00\x01\x00\x00';negative+=bool(check(extra,pe,va[0],slots)[0])
  if negative!=3:errors.append('CP06 actual instruction negatives did not all reject')
 print(json.dumps({'status':'CP06_PE_SCOPE_PASS' if not errors else 'CP06_PE_SCOPE_FAIL','scope_bytes':len(code),'scope_sha256':hashlib.sha256(code).hexdigest(),'scope_hex':code.hex(),'stores':counts,'actual_instruction_negatives_rejected':negative,'errors':errors,'native_runtime':False},indent=2))
 return bool(errors)
if __name__=='__main__':sys.exit(main())
