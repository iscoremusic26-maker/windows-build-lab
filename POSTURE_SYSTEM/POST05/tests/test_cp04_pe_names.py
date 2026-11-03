"""Only the two exact reviewed native transitions escape the mutator ban."""
from pathlib import Path
import sys,json,struct
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'ci'))
import pe_audit
import cp04_pe_audit
checks=0
for symbol in pe_audit.CP04_NATIVE_TRANSITIONS:
 bad=symbol.split('@')[0][1:]
 for raw,want in [(symbol.encode()+b'\0',False),(symbol.replace('@daE_OC_c@','@daE_DB_c@').replace('@daAlink_c@','@daOther_c@').encode()+b'\0',True),(symbol.encode()+b'EXTRA\0',True),(symbol.encode()+b'\0?'+bad.encode()+b'@WrongClass@@QEAAXXZ\0',True)]:
  assert pe_audit.forbidden_decorated(raw,bad)==want,(bad,raw);checks+=1
for bad in ('setDamagePoint','procGuardSlipInit','commonProcInit','VirtualProtect','WriteProcessMemory'):
 assert pe_audit.forbidden_decorated(b'?'+bad.encode()+b'@Wrong@@\0',bad);checks+=1
class SyntheticPE:
 image_base=0
 def off(self,rva):
  if not 0<=rva<512:raise ValueError('outside synthetic image')
  return rva
pe=SyntheticPE();raw=bytearray(512);raw[128:134]=b'\xff\x25'+struct.pack('<i',256-134)
wanted=('dusklight.exe','native-reviewed');slots={256:wanted}
code=b'\xe8'+struct.pack('<i',128-5)+b'\x90\xc3'
assert cp04_pe_audit.calls_import(pe,bytes(raw),code,0,slots,wanted);checks+=1
assert not cp04_pe_audit.calls_import(pe,bytes(raw),code,0,slots,('dusklight.exe','wrong'));checks+=1
for direct in (b'\xff\x15'+struct.pack('<i',256-6),b'\xff\x25'+struct.pack('<i',256-6),b'\xe9'+struct.pack('<i',128-5)):
 assert cp04_pe_audit.calls_import(pe,bytes(raw),direct,0,slots,wanted);checks+=1
raw[128:133]=b'\xe9'+struct.pack('<i',-5)
assert cp04_pe_audit.resolve_import(pe,bytes(raw),128,slots) is None;checks+=1
assert cp04_pe_audit.resolve_import(pe,bytes(raw),4096,slots) is None;checks+=1
for code in (b'\xf3\x0f\x11\x40\x28',b'\xf3\x41\x0f\x11\x40\x28',b'\x89\x50\x28',b'\x41\x89\x50\x28'):
 assert len(cp04_pe_audit.rate_store_spans(code))==1;checks+=1
 assert not cp04_pe_audit.rate_store_spans(code[:-1]+b'\x2c');checks+=1
assert not cp04_pe_audit.rate_store_spans(b'\x48\x89\x50\x28');checks+=1
print(json.dumps({'status':'PE_NAME_POLICY_PASS','checks':checks,'authorized_exact_transitions':sorted(pe_audit.CP04_NATIVE_TRANSITIONS),'native_runtime':False},indent=2))
