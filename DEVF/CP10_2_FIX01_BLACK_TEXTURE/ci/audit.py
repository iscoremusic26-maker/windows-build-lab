"""CP10.2 FIX01: strict PE/ZIP audit (one execute hook, no drawFast/XF; private J3DTexture import reviewed). Base: CP10.2 for the Black-variant Expansion (hooks + game imports), self-contained.
Derived from the human-verified CP04.4 audit (hook metadata, PMF import binding) and the CP10 expansion audit (services)."""
import hashlib,json,stat,struct,zipfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
EXPECTED_FILES={'mod.json','lib/windows-amd64/mod.dll'}
EXPECTED_EXPORTS={'mod_ctx','mod_meta','mod_initialize','mod_update','mod_shutdown'}
HOST='dev.twilitrealm.dusklight.'
# 'game' (2,0) is added by add_mod(... FEATURES game) and was also part of the human-verified CP04.4 service set.
EXPECTED_SERVICES={(HOST+'host',2,2),(HOST+'file',1,0),(HOST+'log',1,0),(HOST+'hook',1,1),(HOST+'game',2,0),('dev.devf.encounter_placement',1,0)}
HOOK_NAMES={'daE_OC_c::execute':['??_7daE_OC_c@@6B@','daE_OC_c::execute']}
HOOK_IMPORTS={'?execute@daE_OC_c@@QEAAHXZ'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def safe_members(z):
    infos=z.infolist();names=[i.filename for i in infos]
    if len(names)!=len(set(names)) or len(names)!=len({n.casefold() for n in names}):raise RuntimeError('Duplicate/case-alias ZIP entries')
    if {i.filename for i in infos if not i.is_dir()}!=EXPECTED_FILES:raise RuntimeError('Unexpected/missing functional files')
    if any(i.is_dir() and i.filename not in {'lib/','lib/windows-amd64/'} for i in infos):raise RuntimeError('Unexpected directory')
    if sum(i.file_size for i in infos)>4*1024*1024:raise RuntimeError('Unexpected expanded size')
    for i in infos:
        n=i.filename;mode=i.external_attr>>16
        if i.file_size>2*1024*1024 or i.flag_bits&1 or stat.S_ISLNK(mode) or stat.S_IFMT(mode) not in (0,stat.S_IFREG,stat.S_IFDIR):raise RuntimeError('Encrypted/link/special/oversized member')
        if '\\' in n or ':' in n or n.startswith('/') or any(s in ('','.','..') for s in n.rstrip('/').split('/')):raise RuntimeError('Unsafe member path')
    if z.testzip() is not None:raise RuntimeError('CRC mismatch')
    return [{'path':i.filename,'directory':i.is_dir(),'bytes':i.file_size,'crc32':f'{i.CRC:08x}','sha256':sha(z.read(i))} for i in infos]
def pe_parts(raw):
    pe=struct.unpack_from('<I',raw,0x3c)[0];opt=pe+24
    if raw[:2]!=b'MZ' or raw[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',raw,pe+4)[0]!=0x8664 or struct.unpack_from('<H',raw,opt)[0]!=0x20b:raise RuntimeError('PE32+ AMD64 required')
    count=struct.unpack_from('<H',raw,pe+6)[0];table=opt+struct.unpack_from('<H',raw,pe+20)[0]
    def offset(rva):
        for i in range(count):
            vsize,va,size,pos=struct.unpack_from('<IIII',raw,table+40*i+8)
            if va<=rva<va+max(vsize,size):
                at=pos+rva-va
                if at>=pos+size or at>=len(raw):raise RuntimeError('Unmapped raw RVA')
                return at
        raise RuntimeError('Unmapped RVA')
    return pe,opt,offset
def pe_exports(raw):
    pe,opt,offset=pe_parts(raw);er,es=struct.unpack_from('<II',raw,opt+112);at=offset(er)
    functions,names,fr,nr,orr=struct.unpack_from('<IIIII',raw,at+20)
    if functions!=5 or names!=5:raise RuntimeError('Extra/ordinal-only export')
    result=[]
    for i in range(names):
        name_at=offset(struct.unpack_from('<I',raw,offset(nr)+4*i)[0]);name=raw[name_at:raw.index(b'\0',name_at)].decode('ascii')
        ordinal=struct.unpack_from('<H',raw,offset(orr)+2*i)[0]
        if ordinal>=functions:raise RuntimeError('Bad export ordinal')
        address=struct.unpack_from('<I',raw,offset(fr)+4*ordinal)[0]
        if address==0 or er<=address<er+es:raise RuntimeError('Null/forwarded export')
        result.append(name)
    if set(result)!=EXPECTED_EXPORTS:raise RuntimeError('Unexpected export names')
    return sorted(result)
def inspect_pe(path):
    raw=path.read_bytes();pe,opt,offset=pe_parts(raw)
    u32=lambda n:struct.unpack_from('<I',raw,n)[0]
    cstr=lambda n:raw[n:raw.index(b'\0',n)].decode('ascii')
    imports={};slots={};n=offset(u32(opt+120))
    while any(raw[n:n+20]):
        ilt,_,_,name,iat=struct.unpack_from('<5I',raw,n);dll=cstr(offset(name));syms=[];q=offset(ilt or iat);index=0
        while (v:=struct.unpack_from('<Q',raw,q)[0]):
            if v>>63:raise RuntimeError('Ordinal import')
            symbol=cstr(offset(v)+2);syms.append(symbol);slots[iat+8*index]=symbol;q+=8;index+=1
        imports[dll]=sorted(syms);n+=20
    allowed=json.loads((HERE/'reviewed-imports.json').read_text())
    for dll,syms in imports.items():
        if dll not in allowed or len(syms)!=len(set(syms)):raise RuntimeError('UNREVIEWED_IMPORT_MODULE '+json.dumps(imports,sort_keys=True))
        extra=[s for s in syms if s not in allowed[dll]]
        if extra:raise RuntimeError('UNREVIEWED_IMPORTS '+json.dumps({dll:extra})+' ALL='+json.dumps(imports,sort_keys=True))
    if imports.get('dusklight.exe')!=sorted(allowed['dusklight.exe']):raise RuntimeError('GAME_IMPORT_SET_MISMATCH '+json.dumps(imports.get('dusklight.exe')))
    for directory in [9,13]:
        if any(struct.unpack_from('<II',raw,opt+112+8*directory)):raise RuntimeError('TLS/delay imports')
    base=struct.unpack_from('<Q',raw,opt+24)[0]
    sections=[]
    for i in range(struct.unpack_from('<H',raw,pe+6)[0]):
        at=opt+struct.unpack_from('<H',raw,pe+20)[0]+40*i;sections.append((raw[at:at+8].rstrip(b'\0').decode(),*struct.unpack_from('<4I',raw,at+8)))
    meta=next(s for s in sections if s[0]=='modmeta');_,vs,vr,rs,off=meta;n=off;end=off+min(vs,rs);records=[];bound=[]
    while n+8<=end:
        if not any(raw[n:n+8]):n+=8;continue
        size,kind,flags=struct.unpack_from('<HBB',raw,n)
        if size<8 or size%8 or n+size>end:raise RuntimeError('Invalid metadata')
        if kind==1:
            if size!=8 or flags or u32(n+4)!=1:raise RuntimeError('Module ABI')
            records.append({'kind':'header'})
        elif kind==2:
            major,minor=struct.unpack_from('<HH',raw,n+4)
            if size!=80 or flags!=0:raise RuntimeError('Required service record expected')
            records.append({'kind':'import','id':cstr(n+16),'major':major,'minor':minor})
        elif kind==5:
            if flags!=0 or u32(n+4)!=0 or any(raw[n+16:n+32]):raise RuntimeError('Unexpected PMF adjustment/resolved slot/flags')
            names=[s.decode('ascii') for s in raw[n+32:n+size].split(b'\0') if s]
            if len(names)!=2 or HOOK_NAMES.get(names[1])!=names:raise RuntimeError('Unexpected hook metadata '+str(names))
            if size!=(32+sum(len(s)+1 for s in names)+7)//8*8:raise RuntimeError('Hook metadata record size')
            pointer=struct.unpack_from('<Q',raw,n+8)[0]-base;q=offset(pointer)
            if raw[q:q+2]!=b'\xff\x25':raise RuntimeError('PMF not import thunk')
            slot=pointer+6+struct.unpack_from('<i',raw,q+2)[0];bound.append(slots.get(slot))
            records.append({'kind':'hook_mem','target':names[1]})
        else:raise RuntimeError('Forbidden metadata kind '+str(kind))
        n+=size
    actual={(r['id'],r['major'],r['minor']) for r in records if r['kind']=='import'}
    if actual!=EXPECTED_SERVICES:raise RuntimeError('Unexpected metadata services '+json.dumps(sorted(actual)))
    if sorted(r['target'] for r in records if r['kind']=='hook_mem')!=sorted(HOOK_NAMES) or set(bound)!=HOOK_IMPORTS:raise RuntimeError('Typed hook surface/PMF import binding mismatch')
    if len(records)!=1+len(EXPECTED_SERVICES)+len(HOOK_NAMES):raise RuntimeError('Duplicate or extra metadata')
    if b'J3DShapePacket::drawFast' in raw or 'GXCallDisplayList' in imports.get('dusklight.exe',[]):raise RuntimeError('Old XF/drawFast tint route must be absent')
    return {'status':'CP10_2_FIX01_PE_ABI_PASS','machine':'AMD64','imports':imports,'metadata':records,'exports':pe_exports(raw),'hook_bound_imports':sorted(bound),'tls_or_delay_imports':False,'game_or_dll_executed':False}
def verify_package(package,dll,project):
    pe=inspect_pe(dll)
    with zipfile.ZipFile(package) as z:
        members=safe_members(z);manifest=z.read('mod.json');inside=z.read('lib/windows-amd64/mod.dll')
    if manifest!=(project/'expansion/mod.json').read_bytes() or inside!=dll.read_bytes():raise RuntimeError('Package manifest or DLL mismatch')
    parsed=json.loads(manifest)
    if (parsed['id'],parsed['version'])!=('dev.bokoblin.expansion','0.4.1'):raise RuntimeError('Package identity/version')
    return {'status':'PACKAGE_VERIFIED','package_bytes':package.stat().st_size,'package_sha256':sha(package.read_bytes()),'dll_bytes':len(inside),'dll_sha256':sha(inside),'manifest_sha256':sha(manifest),'members':members,'pe':pe,'original_assets_included':False}
def negatives(dll,work):
    result=[];raw=dll.read_bytes()
    def reject(name,b):
        p=work/('negative-'+name+'.dll');p.write_bytes(b)
        try:inspect_pe(p)
        except (RuntimeError,ValueError,StopIteration,struct.error):result.append({'case':name,'rejected':True})
        else:raise RuntimeError('Negative survived: '+name)
    inspect_pe(dll)
    pe,_,_=pe_parts(raw);mut=bytearray(raw);struct.pack_into('<H',mut,pe+4,0x14c);reject('x86',bytes(mut))
    for name,before,after in [('wrong_execute_hook',b'daE_OC_c::execute',b'daE_OC_c::executX'),('wrong_hook_service',b'dev.twilitrealm.dusklight.hook',b'dev.twilitrealm.dusklight.hoox'),('wrong_export',b'mod_update',b'mod_updatX'),('wrong_encounter_service',b'dev.devf.encounter_placement',b'dev.devf.encounter_placemenX')]:
        if before not in raw:raise RuntimeError('Negative marker missing: '+name)
        reject(name,raw.replace(before,after))
    def bad_zip(name,entries):
        p=work/('negative-'+name+'.zip')
        with zipfile.ZipFile(p,'w') as z:
            for n,b in entries:z.writestr(n,b)
        try:
            with zipfile.ZipFile(p) as z:safe_members(z)
        except (RuntimeError,ValueError,zipfile.BadZipFile):result.append({'case':name,'rejected':True})
        else:raise RuntimeError('ZIP negative survived')
    bad_zip('original_arc_forbidden',[('mod.json',b'{}'),('lib/windows-amd64/mod.dll',raw),('E_oc2.arc',b'dummy-not-an-asset')])
    bad_zip('path_escape',[('../mod.json',b'{}'),('lib/windows-amd64/mod.dll',raw)])
    return result
