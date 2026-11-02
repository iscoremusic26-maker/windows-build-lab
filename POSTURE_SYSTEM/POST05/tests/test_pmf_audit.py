"""Static audit adversarial cases; no DLL or native helper is executed."""
from pathlib import Path
import sys,struct,json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'ci'))
from pmf_audit import decode_materializer
class Image:
 image_base=0x100000
 def __init__(self,code):self.raw=code+bytes(512-len(code))
 def off(self,rva):
  if not 0<=rva<len(self.raw):raise ValueError('unmapped')
  return rva
target=0x100180
code=b'\x48\xb8'+struct.pack('<Q',target)+b'\x48\x89\x01\x33\xc0\x48\x89\x41\x08\x48\x89\x41\x10\xc3'
expected=struct.pack('<Q',target)+bytes(16)
assert decode_materializer(Image(code),0x100000,24,{})==expected
alias=b'\x48\x8b\xd1'+code.replace(b'\x48\x89\x01',b'\x48\x89\x02').replace(b'\x48\x89\x41',b'\x48\x89\x42')
assert decode_materializer(Image(alias),0x100000,24,{})==expected
vec=b'\xf3\x0f\x6f\x05'+struct.pack('<i',0x80-8)+b'\xf3\x0f\x7f\x01\x48\x8b\x05'+struct.pack('<i',0x90-19)+b'\x48\x89\x41\x10\xc3'
im=Image(vec+bytes(0x80-len(vec))+expected)
assert decode_materializer(im,0x100000,24,{})==expected
native=b'\x48\x83\xec\x28'+code[:-1]+b'\x8b\x44\x24\x14\x89\x41\x14\x48\x83\xc4\x28\xc3'
assert decode_materializer(Image(native),0x100000,24,{})==expected
observed=bytes.fromhex('4883ec28488d05f780000048890133c0488941088941108b4424148941144883c428c3')
assert decode_materializer(Image(observed),0x100000,24,{})[:20]==struct.pack('<Q',0x100000+11+0x80f7)+bytes(12)
bad=[('unproven outside store',code.replace(b'\x48\x89\x01',b'\x48\x89\x02')),
 ('nonzero adjustment',code.replace(b'\x33\xc0',b'\xb8\x01\x00\x00\x00')),
 ('uninitialized return',b'\xc3'),('arbitrary code',b'\xcc'),
 ('stack side effect',b'\x48\x83\xec\x28'),('loop',b'\xe9\xfb\xff\xff\xff'),
 ('unknown branch',b'\xff\x25\x10\x00\x00\x00')]
for name,b in bad:
 try:decode_materializer(Image(b),0x100000,24,{})
 except (ValueError,KeyError):pass
 else:raise AssertionError(name)
for n in (8,16,32):
 try:decode_materializer(Image(code),0x100000,n,{})
 except ValueError:pass
 else:raise AssertionError('wrong native PMF size')
print(json.dumps({'status':'PMF_STATIC_AUDIT_TEST_PASS','valid':5,'rejected':len(bad)+3,'native_code_executed':False}))
