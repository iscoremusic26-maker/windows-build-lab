"""Read-only verification of two calls in the pinned official Windows PE; never run it.

Requires objdump and zstandard only when the embedded manifest uses Zstd.
Outputs derived names/flags/call edges/hashes, never original code bytes.
"""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
HOST_SHA='ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049'
def verify(path):
 b=path.read_bytes()
 if len(b)!=52309504 or hashlib.sha256(b).hexdigest()!=HOST_SHA:raise RuntimeError('Different official host')
 pe=struct.unpack_from('<I',b,0x3c)[0];opt=pe+24
 if struct.unpack_from('<H',b,pe+4)[0]!=0x8664:raise RuntimeError('AMD64 required')
 size=struct.unpack_from('<H',b,pe+20)[0];base=struct.unpack_from('<Q',b,opt+24)[0];secs=[]
 for i in range(struct.unpack_from('<H',b,pe+6)[0]):
  q=opt+size+i*40;name=b[q:q+8].rstrip(b'\0').decode();vs,vr,rs,off=struct.unpack_from('<4I',b,q+8);secs.append((name,vs,vr,rs,off))
 def at(v):
  for name,vs,vr,rs,off in secs:
   if vr<=v<vr+max(vs,rs) and v-vr<rs:return off+v-vr
  raise RuntimeError('Unmapped RVA')
 def follow(v):
  chain=[]
  for _ in range(8):
   q=at(v)
   if b[q]!=0xE9:return v,chain
   dest=v+5+struct.unpack_from('<i',b,q+1)[0];chain.append([v,dest]);v=dest
  raise RuntimeError('Unexpected jump chain')
 er=struct.unpack_from('<I',b,opt+112)[0];_,count,fa,na,oa=struct.unpack_from('<5I',b,at(er)+20);exports={}
 for i in range(count):
  q=at(struct.unpack_from('<I',b,at(na)+4*i)[0]);name=b[q:b.index(0,q)].decode()
  if name in ['?create@daE_OC_c@@QEAAHXZ','?execute@daE_OC_c@@QEAAHXZ','?action@daE_OC_c@@QEAAXXZ']:
   ordinal=struct.unpack_from('<H',b,at(oa)+2*i)[0];exports[name]=struct.unpack_from('<I',b,at(fa)+4*ordinal)[0]
 sec=next(x for x in secs if x[0]=='.symdb');q=sec[4]
 magic,ver,comp,ulen,clen,bilen,bid,n=struct.unpack_from('<8sIIQQI32sI',b,q)
 if magic!=b'SYMGEN\0\0' or ver!=2:raise RuntimeError('Manifest format changed')
 data=b[q+72:q+72+clen]
 if comp==1:
  import zstandard;data=zstandard.ZstdDecompressor().decompress(data,max_output_size=ulen)
 elif comp!=0:raise RuntimeError('Unknown compression')
 if len(data)!=ulen:raise RuntimeError('Truncated manifest')
 strings=data[n*24:];entries=[];code_addresses=set()
 wanted={'daE_OC_c::create','daE_OC_c::execute','daE_OC_c::action',*exports}
 for i in range(n):
  _,v,off,flags=struct.unpack_from('<QQII',data,i*24)
  if flags&1:code_addresses.add(v)
  name=strings[off:strings.index(0,off)].decode(errors='strict')
  if name in wanted:entries.append({'name':name,'rva':v,'flags':flags,'has_inline_sites':bool(flags&32)})
 if any(x['has_inline_sites'] for x in entries):raise RuntimeError('Critical target inlined')
 body,chain=follow(exports['?execute@daE_OC_c@@QEAAHXZ'])
 create,_=follow(exports['?create@daE_OC_c@@QEAAHXZ'])
 action,_=follow(exports['?action@daE_OC_c@@QEAAXXZ'])
 def calls(start):
  end=min(v for v in code_addresses if v>start)
  if end-start>16384:raise RuntimeError('Unexpected target extent')
  text=subprocess.check_output(['objdump','-d','--start-address='+str(base+start),'--stop-address='+str(base+end),str(path)],text=True)
  return [(int(m.group(1),16)-base,int(m.group(2),16)-base) for m in re.finditer(r'^\s*([0-9a-f]+):.*\bcall\s+0x([0-9a-f]+)',text,re.M)],end
 cc,ce=calls(create);ec,ee=calls(body)
 create_edges=[{'call_rva':site,'callee_rva':dest,'jump_chain':follow(dest)[1],'resolved_rva':body} for site,dest in cc if follow(dest)[0]==body]
 action_edges=[{'call_rva':site,'callee_rva':dest,'jump_chain':follow(dest)[1],'resolved_rva':action} for site,dest in ec if follow(dest)[0]==action]
 if len(create_edges)!=1 or len(action_edges)!=1:raise RuntimeError('Critical call edge absent/ambiguous')
 return {'status':'SOURCE_AND_WINDOWS_PE_CALL_EDGES_VERIFIED','host_bytes':len(b),'host_sha256':HOST_SHA,'manifest_version':ver,'build_id':bid[:bilen].hex(),'target_entries':entries,'execute_export_jump_chain':chain,'create_extent_rva':[create,ce],'execute_extent_rva':[body,ee],'create_to_execute':create_edges,'execute_to_action':action_edges,'manual_review_source_lines':'d_a_e_oc.cpp2771-2779,2829,2642-2644,2631','original_bytes_or_disassembly_published':False,'game_or_hook_backend_executed':False,'runtime':'HUMAN_RUNTIME_NOT_TESTED'}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--host',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 r=verify(a.host);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+'\n');print('PASS static official Windows call chain; game/hook not executed')
