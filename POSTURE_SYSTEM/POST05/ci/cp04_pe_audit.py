"""Actual MSVC writer scopes: two inline field stores and two typed native
transition imports. Never executes the DLL; not a whole-program formal proof.
"""
from pathlib import Path
import re,sys,json,struct,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import pe_audit,store_audit
def resolve_import(pe,raw,target_va,slots):
 # Official headers use unannotated native method declarations too: MSVC
 # may emit E8 -> import thunk -> FF25, rather than an in-scope FF15 call.
 # Follow only the same bounded E9/FF25 forms accepted by the host auditor.
 for _ in range(8):
  rva=target_va-pe.image_base
  try:off=pe.off(rva)
  except (ValueError,IndexError):return None
  if off<0 or off+6>len(raw):return None
  if raw[off]==0xe9:
   target_va+=5+struct.unpack_from('<i',raw,off+1)[0];continue
  if raw[off:off+2]==b'\xff\x25':return slots.get(rva+6+struct.unpack_from('<i',raw,off+2)[0])
  return None
 return None
def calls_import(pe,raw,code,start_va,slots,wanted):
 for i in range(len(code)):
  if i+6<=len(code) and code[i:i+2] in (b'\xff\x15',b'\xff\x25'):
   slot=start_va-pe.image_base+i+6+struct.unpack_from('<i',code,i+2)[0]
   if slots.get(slot)==wanted:return True
  if i+5<=len(code) and code[i] in (0xe8,0xe9):
   target=start_va+i+5+struct.unpack_from('<i',code,i+1)[0]
   if resolve_import(pe,raw,target,slots)==wanted:return True
 return False
def stores(scopes):
 errors=[]
 rate=len(re.findall(rb'\xf3\x0f\x11[\x40-\x7f]\x28',scopes['cp04_native_rate']))
 gap=len(re.findall(rb'\x66\x89[\x80-\xbf]\xe2\x08\x00\x00',scopes['cp04_native_gap']))
 if rate!=1:errors.append('fresh-clip rate: expected one movss store at native morf+0x28')
 if gap!=1:errors.append('owned gap: expected one16bit store at E_OC+0x8e2')
 return errors,{'fresh_clip_rate_store':rate,'own_gap_timer_store':gap}
def main():
 dll,mapfile=map(Path,sys.argv[1:3]);raw=dll.read_bytes();mp=mapfile.read_text(encoding='utf-8');scopes={n:store_audit.scoped_bytes(raw,mp,n) for n in ['cp04_native_rate','cp04_native_gap','cp04_native_mode','cp04_native_shield_restart']}
 errors,counts=stores(scopes);negative=0
 for n,old,new in [('cp04_native_rate',b'\x28',b'\x2c'),('cp04_native_gap',b'\xe2\x08\x00\x00',b'\xe0\x08\x00\x00')]:
  changed=dict(scopes)
  if n.endswith('rate'):changed[n]=re.sub(rb'(\xf3\x0f\x11[\x40-\x7f])\x28',lambda m:m[1]+new,scopes[n],count=1)
  else:changed[n]=scopes[n].replace(old,new,1)
  if changed[n]!=scopes[n] and stores(changed)[0]:negative+=1
 if negative!=2:errors.append('actual instruction negatives failed')
 pe=pe_audit.PE(raw);imports,slots=pe.imports();bounds={}
 for name,symbol in [('cp04_native_mode','?setActionMode@daE_OC_c@@QEAAXHH@Z'),('cp04_native_shield_restart','?procGuardAttackInit@daAlink_c@@QEAAHXZ')]:
  addrs=[int(m[1],16) for m in re.finditer(r'^\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+\?'+name+r'@\S+\s+([0-9a-fA-F]{16})\s+f\s',mp,re.M)]
  if len(addrs)!=1:errors.append('unique writer map scope missing: '+name);continue
  code=scopes[name];found=calls_import(pe,raw,code,addrs[0],slots,('dusklight.exe',symbol))
  bounds[name]=found
  if not found:errors.append('typed native import not bound in writer scope: '+name)
 r={'status':'CP04_PE_SCOPE_PASS' if not errors else 'CP04_PE_SCOPE_FAIL','scope_bytes':{k:len(v) for k,v in scopes.items()},'scope_sha256':{k:hashlib.sha256(v).hexdigest() for k,v in scopes.items()},'stores':counts,'native_transition_imports_bound':bounds,'actual_instruction_negatives_rejected':negative,'errors':errors,'native_runtime':False}
 print(json.dumps(r,indent=2));return bool(errors)
if __name__=='__main__':sys.exit(main())
