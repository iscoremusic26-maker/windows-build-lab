"""CP17 FIX01: read the actual MSVC COFF probe (CP17 layout + FIX01 fields and checkCutJumpMode); no game DLL is loaded, no native function is stubbed or executed."""
import hashlib,struct

def virtual_slot(body):
 for i in range(min(9,len(body)-1)):
  if body[i:i+2]==b'\xff\xa0' and i+6<=len(body):return struct.unpack_from('<i',body,i+2)[0]
  if body[i:i+2]==b'\xff\x60' and i+3<=len(body):return body[i+2]
 return None

def verify(path):
 # Boundary regression from the actual MSVC ten-byte stub in run39183741446. These are reader tests, not fake game ABI evidence.
 observed=bytes.fromhex('488b0148ffa068010000')
 assert virtual_slot(observed)==0x168 and virtual_slot(observed[:-1]) is None and virtual_slot(bytes(10)) is None

 raw=path.read_bytes()
 def unpack(fmt,off):
  size=struct.calcsize(fmt);assert 0<=off<=len(raw)-size
  return struct.unpack_from(fmt,raw,off)
 machine,count,_,symptr,nsyms,optsize,_=unpack('<HHIIIHH',0)
 assert machine==0x8664 and 0<count<32768 and optsize==0 and 0<nsyms<1000000,'AMD64 standard COFF probe required'
 assert 20+40*count<=len(raw) and symptr+nsyms*18+4<=len(raw)
 stringptr=symptr+nsyms*18;stringlen=unpack('<I',stringptr)[0];assert 4<=stringlen<=len(raw)-stringptr
 sections=[]
 for n in range(count):
  at=20+n*40;size,pos=unpack('<II',at+16);assert pos<=len(raw) and size<=len(raw)-pos
  sections.append((pos,size))
 def name(at):
  b=raw[at:at+8]
  if b[:4]==b'\0'*4:
   off=struct.unpack_from('<I',b,4)[0];assert 4<=off<stringlen;end=raw.index(b'\0',stringptr+off,stringptr+stringlen);return raw[stringptr+off:end].decode('ascii')
  return b.split(b'\0')[0].decode('ascii')
 symbols={};n=0
 while n<nsyms:
  at=symptr+n*18;value,section,typ,storage,aux=unpack('<IhHBB',at+8);s=name(at)
  if section>0 and storage==2:symbols[s]=(section,value)
  n+=1+aux
 assert n==nsyms
 def data(s,size):
  section,value=symbols[s];assert 1<=section<=len(sections);pos,limit=sections[section-1];assert 0<=value<=limit-size
  return raw[pos+value:pos+value+size]
 vals=struct.unpack('<14Q',data('cp17_abi_layout',14*8))
 assert vals[0]==0x4350313741424931 and vals[1]==8 and vals[5]==2
 keys=['magic','pointer_bytes','alink_bytes','player_bytes','eoc_bytes','proc_width','proc_offset','procvar2_offset','player_damage_timer_offset','eoc_mode_offset','eoc_morf_offset','morf_framectrl_offset','const_pmf_bytes','proc_max']
 layout=dict(zip(keys,vals));layout['magic']='0x4350313741424931';assert layout['proc_offset']+2<=layout['alink_bytes'] and layout['procvar2_offset']>layout['proc_offset'] and layout['proc_max']>0x30
 f1=struct.unpack('<11Q',data('cp17f1_abi_layout',11*8))
 assert f1[0]==0x4350313746314142
 k1=['magic','alink_targeted_actor_offset','alink_link_acch_offset','actor_speed_offset','actor_speedF_offset','actor_gravity_offset','actor_shape_angle_offset','eoc_lockout_6c2_offset','eoc_at_spheres_offset','sphere_bytes','alink_tg_cylinders_offset']
 fix01=dict(zip(k1,f1));fix01['magic']='0x4350313746314142'
 assert fix01['alink_targeted_actor_offset']+8<=layout['alink_bytes'] and fix01['alink_link_acch_offset']<layout['alink_bytes'] and fix01['alink_tg_cylinders_offset']<layout['alink_bytes']
 assert fix01['actor_gravity_offset']==fix01['actor_speedF_offset']+4,'speedF/gravity adjacency of the official fopAc_ac_c header'
 assert fix01['eoc_lockout_6c2_offset']+2<=layout['eoc_bytes'] and fix01['eoc_at_spheres_offset']+2*fix01['sphere_bytes']<=layout['eoc_bytes'] and fix01['sphere_bytes']>0
 layout['cp17f1']=fix01
 stubs={}
 for s in ['cp17_const_charge','cp17_const_spin','cp17_const_jump','cp17f1_const_cut_jump_mode','cp17_typed_proc']:
  section,value=symbols[s];pos,size=sections[section-1];body=raw[pos+value:pos+min(size,value+40)];assert body
  stubs[s]={'code_hex':body.hex()}
  if s!='cp17_typed_proc':
   # Compiler may choose disp8 or disp32; record the slot derived from its code, never inject any slot into the mod.
   slot=virtual_slot(body)
   assert slot is not None and slot>=0 and slot%8==0,('Unrecognized const virtual probe code',s,body.hex())
   stubs[s]['compiler_virtual_slot_bytes']=slot
  else:
   at=body.find(b'\x0f\xb7\x81');assert 0<=at<=4,('Expected typed u16 load',body.hex())
   offset=struct.unpack_from('<i',body,at+3)[0];assert offset==layout['proc_offset'];stubs[s]['compiler_u16_offset']=offset
 slots=[stubs[s]['compiler_virtual_slot_bytes'] for s in ['cp17_const_charge','cp17_const_spin','cp17_const_jump']]
 assert slots[1]==slots[0]+8 and slots[2]==slots[1]+8,'Consecutive source virtual declarations must have distinct compiler-generated slots'
 jm=stubs['cp17f1_const_cut_jump_mode']['compiler_virtual_slot_bytes'];assert jm not in slots and jm>slots[2],'checkCutJumpMode is a later, distinct const virtual of daPy_py_c'
 return dict(status='CP17F1_MSVC_COFF_SOURCE_ABI_PASS',machine='AMD64',coff_reader_boundary_tests=3,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),layout=layout,compiler_probes=stubs,game_DLL_loaded=False,game_object_read=False,probe_functions_executed=False,meaning='Actual official SDK headers compiled by MSVC; layout/const virtual codegen verified, not physical runtime Link data')
