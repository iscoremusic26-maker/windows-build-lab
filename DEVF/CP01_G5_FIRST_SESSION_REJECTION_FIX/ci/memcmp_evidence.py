"""Correlate only our own linked DLL/map/disassembly, never inspect process memory."""
import re,struct
from audit import package,sha

def verify(dll,linkmap,disasm,predicate):
 raw=dll.read_bytes();_,opt,offset=package.pe_parts(raw);imagebase=struct.unpack_from('<Q',raw,opt+24)[0]
 match=re.search(r'\bmemcmp\s+([0-9A-Fa-f]{16})\b',linkmap)
 if not match:raise RuntimeError('Missing memcmp map thunk')
 thunk=int(match.group(1),16);lines=disasm.splitlines();sites=[]
 funcs=[]
 for line in linkmap.splitlines():
  m=re.search(r'\s(\?\S+)\s+([0-9A-Fa-f]{16})\s',line)
  if m:funcs.append((int(m.group(2),16),m.group(1)))
 for i,line in enumerate(lines):
  m=re.search(r'^\s*([0-9A-Fa-f]{16}):.*call\s+([0-9A-Fa-f]{16})',line)
  if not m or int(m.group(2),16)!=thunk:continue
  va=int(m.group(1),16);block='\n'.join(lines[max(0,i-6):i+3]);const=re.search(r'lea\s+rdx,\[([0-9A-Fa-f]+)h\]',block)
  if not const or not re.search(r'mov\s+r8d,8\b',block) or not re.search(r'lea\s+rcx,\[rsp\+',block):raise RuntimeError('Unreviewed memcmp call arguments')
  constant_va=int(const.group(1),16);at=offset(constant_va-imagebase);value=raw[at:at+8];owner=max((x for x in funcs if x[0]<=va),default=(0,''))[1]
  if value!=b'F_SP121\0' or not owner.startswith('?context_valid@first@devf@@'):raise RuntimeError('Unreviewed memcmp call owner/constant')
  sites.append({'caller':owner,'call_VA':hex(va),'thunk_VA':hex(thunk),'bytes_compared':8,'constant_ASCII':'F_SP121\\0','constant_hex':value.hex(),'stack_stage_buffer':True,'disassembly':block})
 if len(sites)!=1 or 'std::memcmp(o.stage.data(),"F_SP121\\0",8)' not in predicate:raise RuntimeError('Unexpected memcmp call count/source')
 return {'status':'PASS_STATIC','call_sites':sites,'predicate_lf_sha256':sha(predicate.replace('\r\n','\n').encode()),'buffers_read_only':True,'source':'validate_owner inlined into context_valid; existing stage predicate','engine_offsets_used':False}
