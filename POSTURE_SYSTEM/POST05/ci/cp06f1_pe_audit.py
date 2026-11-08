"""CP06 FIX01 0.11.12 actual compiled writer scopes (static, bounded; never executes the DLL).
  cp06f1_native_kcolor        exactly three byte stores at +0x550/+0x551/+0x552 of ONE base register
                              (daAlink_c::tevStr.TevKColor r/g/b), no call, no other non-stack store.
  cp06f1_native_pose_frame    exactly one float store at +0x2c (mDoExt_morf_c::mFrameCtrl.mFrame) and
                              exactly one call, resolved through the import thunk to
                              ?modelCalc@mDoExt_McaMorfSO; no other non-stack store.
  cp06f1_native_sphere_place  no non-stack store at all; exactly one call to ?SetC@cM3dGSph and one to
                              ?SetR@cM3dGSph (the engine setters cc_set itself uses).
And across the whole .text: those three imports are called ONLY from inside these writer scopes.
Stack stores (SIB/rsp, rbp with negative displacement) are excluded; the scan is a bounded
encoding check, not a general proof of every x64 memory effect. Real instruction negatives follow.
"""
from pathlib import Path
import re,sys,json,struct,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import pe_audit
KCOLOR=0x550;FRAME=0x2c
IMPORTS={'modelCalc':'?modelCalc@mDoExt_McaMorfSO@@QEAAXXZ','SetC':'?SetC@cM3dGSph@@QEAAXAEBUcXyz@@@Z','SetR':'?SetR@cM3dGSph@@QEAAXM@Z'}
def functions(maptext):
 out=[]
 for line in maptext.splitlines():
  m=re.search(r'^\s*0001:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{16})\b',line)
  if m:out.append((int(m[2],16),m[1]))
 return sorted(out)
def scope(raw,pe,funcs,name):
 hits=[(a,n) for a,n in funcs if n.startswith('?'+name+'@')]
 if len(hits)!=1:raise ValueError('expected one map function '+name)
 a=hits[0][0];b=min(x for x,_ in funcs if x>a)
 return a,raw[pe.off(a-pe.image_base):pe.off(b-pe.image_base)]
# ModRM stores to memory with a displacement: 88 (r/m8,r8) 89 (r/m32/64) C6/C7 (imm) 66 89 (16) F3/F2 0F 11 (movss/movsd) 0F 11/0F 29 (movups/movaps)
STORE=[(rb'(?P<p>[\x40-\x4f]?)\x88(?P<m>[\x40-\xbf])','MOV8'),(rb'\x66(?P<p>[\x40-\x4f]?)\x89(?P<m>[\x40-\xbf])','MOV16'),(rb'(?P<p>[\x40-\x4f]?)\x89(?P<m>[\x40-\xbf])','MOV32_64'),
       (rb'(?P<p>[\x40-\x4f]?)\xc6(?P<m>[\x40-\x47\x80-\x87])','MOVIMM8'),(rb'(?P<p>[\x40-\x4f]?)\xc7(?P<m>[\x40-\x47\x80-\x87])','MOVIMM32'),
       (rb'\xf3(?P<p>[\x40-\x4f]?)\x0f\x11(?P<m>[\x40-\xbf])','MOVSS'),(rb'\xf2(?P<p>[\x40-\x4f]?)\x0f\x11(?P<m>[\x40-\xbf])','MOVSD'),
       (rb'(?P<p>[\x40-\x4f]?)\x0f[\x11\x29](?P<m>[\x40-\xbf])','MOVUPS_APS')]
def stores(code):
 cand=[]
 for pat,kind in STORE:
  for m in re.finditer(pat,code):
   i=m.start('m');modrm=code[i];mod=modrm>>6;rm=modrm&7;rex=code[m.start('p')] if m.group('p') else 0
   if mod==1:disp=struct.unpack_from('<b',code,i+1)[0];end=i+2
   elif mod==2:disp=struct.unpack_from('<i',code,i+1)[0];end=i+5
   else:continue
   if kind=='MOVIMM8':end+=1
   elif kind=='MOVIMM32':end+=4
   start=m.start()
   if kind in('MOV8','MOV32_64','MOVIMM8','MOVIMM32','MOVUPS_APS') and start>0 and code[start-1] in(0x66,0xf2,0xf3):start-=1   # mandatory/size prefix belongs to it
   cand.append({'start':start,'end':end,'at':m.start(),'kind':kind,'rm':rm,'base':rm|((rex&1)<<3),'disp':disp})
 out=[];last=-1
 for c in sorted(cand,key=lambda x:(x['start'],-(x['end']-x['start']))):
  if c['start']<last:continue                         # starts inside an accepted instruction: not a real boundary
  last=c['end']
  if c['rm']==4:continue                              # SIB: rsp-based locals / indexed own buffers
  if c['base']==5 and c['disp']<0:continue             # rbp frame locals
  out.append({'at':c['at'],'kind':c['kind'],'base':c['base'],'disp':c['disp']})
 return out
def call_targets(raw,pe,slots,start_va,code):
 out=[]
 for m in re.finditer(rb'\xe8(....)',code,re.S):
  va=start_va+m.start()+5+struct.unpack('<i',m.group(1))[0]
  try:o=pe.off(va-pe.image_base)
  except ValueError:continue   # not a call into the image (data bytes inside .text)
  name=None
  if raw[o:o+2]==b'\xff\x25':name=slots.get(va+6+struct.unpack_from('<i',raw,o+2)[0]-pe.image_base,(None,None))[1]
  out.append({'at':m.start(),'target':va,'import':name})
 for m in re.finditer(rb'\xff\x15(....)',code,re.S):
  slot=start_va+m.start()+6+struct.unpack('<i',m.group(1))[0]-pe.image_base;out.append({'at':m.start(),'target':None,'import':slots.get(slot,(None,None))[1]})
 return out
def check(raw,pe,slots,scopes):
 errors=[];facts={}
 va,code=scopes['cp06f1_native_kcolor'];st=stores(code);calls=call_targets(raw,pe,slots,va,code)
 kc=[s for s in st if s['kind'] in('MOV8','MOVIMM8')and s['disp'] in(KCOLOR,KCOLOR+1,KCOLOR+2)]
 if sorted(s['disp'] for s in kc)!=[KCOLOR,KCOLOR+1,KCOLOR+2] or len({s['base'] for s in kc})!=1:errors.append('kcolor writer: not exactly r/g/b byte stores at +0x550..+0x552 of one base')
 if len(st)!=3:errors.append('kcolor writer: another non-stack store present')
 if calls:errors.append('kcolor writer: unexpected call')
 facts['kcolor']={'stores':st,'calls':len(calls)}
 va,code=scopes['cp06f1_native_pose_frame'];st=stores(code);calls=call_targets(raw,pe,slots,va,code)
 fr=[s for s in st if s['kind']=='MOVSS' and s['disp']==FRAME]
 if len(fr)!=1 or len(st)!=1:errors.append('pose frame writer: not exactly one float store at +0x2c and nothing else')
 if [c['import'] for c in calls]!=[IMPORTS['modelCalc']]:errors.append('pose frame writer: calls are not exactly the imported modelCalc')
 facts['pose_frame']={'stores':st,'calls':[c['import'] for c in calls]}
 va,code=scopes['cp06f1_native_sphere_place'];st=stores(code);calls=call_targets(raw,pe,slots,va,code)
 if st:errors.append('sphere writer: a non-stack store is present')
 if sorted(c['import'] or '?' for c in calls)!=sorted([IMPORTS['SetC'],IMPORTS['SetR']]):errors.append('sphere writer: calls are not exactly the imported SetC and SetR')
 facts['sphere_place']={'stores':st,'calls':[c['import'] for c in calls]}
 return errors,facts
def main():
 dll,mapfile=map(Path,sys.argv[1:3]);raw=dll.read_bytes();mp=mapfile.read_text(errors='replace');pe=pe_audit.PE(raw);_,slots=pe.imports();funcs=functions(mp)
 errors=[]
 try:scopes={n:scope(raw,pe,funcs,n) for n in('cp06f1_native_kcolor','cp06f1_native_pose_frame','cp06f1_native_sphere_place')}
 except ValueError as e:print(json.dumps({'status':'CP06F1_PE_SCOPE_FAIL','errors':[str(e)]}));return 1
 e,facts=check(raw,pe,slots,scopes);errors.extend(e)
 # whole-image: the writer imports are called only from the writer scopes
 text_va=min(a for a,_ in funcs);text_end=max(a for a,_ in funcs)
 whole=raw[pe.off(text_va-pe.image_base):pe.off(text_end-pe.image_base)]
 allowed=[(a,a+len(c)) for a,c in scopes.values()]
 outside=[c for c in call_targets(raw,pe,slots,text_va,whole) if c['import'] in IMPORTS.values() and not any(lo<=text_va+c['at']<hi for lo,hi in allowed)]
 if outside:errors.append('writer imports called outside the writer scopes: %d sites'%len(outside))
 negative=0
 if not e:
  va,code=scopes['cp06f1_native_kcolor'];i=code.find(struct.pack('<i',KCOLOR+1));bad=bytearray(code);bad[i]=0x54  # +0x554 instead of +0x551
  s2=dict(scopes);s2['cp06f1_native_kcolor']=(va,bytes(bad));negative+=bool(check(raw,pe,slots,s2)[0])
  s2=dict(scopes);s2['cp06f1_native_kcolor']=(va,code[:-1]+b'\x88\x91\x00\x02\x00\x00'+code[-1:]);negative+=bool(check(raw,pe,slots,s2)[0])        # extra byte store
  va,code=scopes['cp06f1_native_pose_frame'];i=code.find(b'\x0f\x11');bad=bytearray(code);bad[i+3]=0x30                                        # +0x30 instead of +0x2c
  s2=dict(scopes);s2['cp06f1_native_pose_frame']=(va,bytes(bad));negative+=bool(check(raw,pe,slots,s2)[0])
  va,code=scopes['cp06f1_native_sphere_place'];s2=dict(scopes);s2['cp06f1_native_sphere_place']=(va,code+b'\x89\x81\x00\x01\x00\x00');negative+=bool(check(raw,pe,slots,s2)[0])
  if negative!=4:errors.append('CP06F1 actual instruction negatives did not all reject')
 print(json.dumps({'status':'CP06F1_PE_SCOPE_PASS' if not errors else 'CP06F1_PE_SCOPE_FAIL','scopes':{n:{'va':hex(a),'bytes':len(c),'sha256':hashlib.sha256(c).hexdigest(),'hex':c.hex()} for n,(a,c) in scopes.items()},'facts':facts,'writer_import_calls_outside':len(outside),'actual_instruction_negatives_rejected':negative,'errors':errors,'native_runtime':False},indent=2,default=str))
 return bool(errors)
if __name__=='__main__':sys.exit(main())
