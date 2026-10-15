"""CP06: reuse G2/G4B strict PE+ZIP readers, extend only reviewed typed targets/import surface."""
import hashlib,json,stat,struct,zipfile
from pathlib import Path
EXPECTED_FILES={'mod.json','lib/windows-amd64/mod.dll'}
ALLOWED_DIRECTORIES={'lib/','lib/windows-amd64/'}
EXPECTED_EXPORTS={'mod_ctx','mod_meta','mod_initialize','mod_update','mod_shutdown'}
GAME_IMPORT_BASES={'execute','drawFast','GXCallDisplayList','dComIfGp_getStartStageName','getLayerNo','fpcM_IsCreating','fopAcIt_Judge','fpcSch_JudgeByID','dComIfGp_getPlayer','dComIfGp_event_runCheck','ChkTgHit'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def safe_members(z,expected_files=EXPECTED_FILES,allowed_dirs=ALLOWED_DIRECTORIES):
    infos=z.infolist();names=[i.filename for i in infos]
    if len(names)!=len(set(names)) or len(names)!=len({n.casefold() for n in names}):raise RuntimeError('Duplicate/case-alias ZIP entries')
    if {i.filename for i in infos if not i.is_dir()}!=expected_files:raise RuntimeError('Unexpected/missing functional files')
    if any(i.is_dir() and i.filename not in allowed_dirs for i in infos):raise RuntimeError('Unexpected directory')
    if sum(i.file_size for i in infos)>4*1024*1024:raise RuntimeError('Unexpected expanded size')
    for i in infos:
        n=i.filename;mode=i.external_attr>>16
        if i.file_size>2*1024*1024 or i.flag_bits&1 or stat.S_ISLNK(mode) or stat.S_IFMT(mode) not in (0,stat.S_IFREG,stat.S_IFDIR):raise RuntimeError('Encrypted/link/special/oversized member')
        if '\\' in n or ':' in n or n.startswith('/') or any(s in ('','.','..') for s in n.rstrip('/').split('/')):raise RuntimeError('Unsafe member path')
        if i.is_dir() and i.file_size!=0:raise RuntimeError('Nonempty directory')
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

def pe_exports(path):
    raw=path.read_bytes();pe,opt,offset=pe_parts(raw)
    er,es=struct.unpack_from('<II',raw,opt+112);at=offset(er)
    functions,names,fr,nr,orr=struct.unpack_from('<IIIII',raw,at+20)
    if functions!=5 or names!=5:raise RuntimeError('Extra/ordinal-only export')
    result=[]
    for i in range(names):
        name_at=offset(struct.unpack_from('<I',raw,offset(nr)+4*i)[0]);end=raw.index(b'\0',name_at)
        name=raw[name_at:end].decode('ascii');ordinal=struct.unpack_from('<H',raw,offset(orr)+2*i)[0]
        if ordinal>=functions:raise RuntimeError('Bad export ordinal')
        address=struct.unpack_from('<I',raw,offset(fr)+4*ordinal)[0]
        if address==0 or er<=address<er+es:raise RuntimeError('Null/forwarded export')
        result.append(name)
    if set(result)!=EXPECTED_EXPORTS:raise RuntimeError('Unexpected export names')
    return sorted(result)


def pe_audit(path):
    b = path.read_bytes()
    u16 = lambda n: struct.unpack_from('<H', b, n)[0]
    u32 = lambda n: struct.unpack_from('<I', b, n)[0]
    pe = u32(0x3c)
    if b[:2] != b'MZ' or b[pe:pe+4] != b'PE\0\0' or u16(pe+4) != 0x8664:
        raise RuntimeError('Windows AMD64 PE required')
    opt = pe + 24
    if u16(opt) != 0x20b: raise RuntimeError('PE32+ required')
    sections = []
    for i in range(u16(pe+6)):
        n = opt + u16(pe+20) + 40*i
        sections.append((b[n:n+8].rstrip(b'\0').decode(), u32(n+12), u32(n+8), u32(n+20), u32(n+16)))
    def offset(rva):
        for _, addr, virtual, raw, size in sections:
            if addr <= rva < addr + max(virtual, size):
                pos = raw + rva - addr
                if pos >= raw + size: raise RuntimeError('RVA outside raw data')
                return pos
        raise RuntimeError(f'Unmapped RVA {rva:x}')
    def cstr(n): return b[n:b.index(b'\0', n)].decode('ascii')
    imports = {}
    n = offset(u32(opt+112+8))
    while any(b[n:n+20]):
        ilt, _, _, name, iat = struct.unpack_from('<IIIII', b, n)
        dll = cstr(offset(name)); syms = []; t = offset(ilt or iat)
        while (v := struct.unpack_from('<Q', b, t)[0]):
            if v >> 63: raise RuntimeError('Ordinal import is outside reviewed surface')
            syms.append(cstr(offset(v)+2)); t += 8
        imports[dll] = syms; n += 20
    game = imports.get('dusklight.exe', [])
    bases = {s.split('@', 1)[0].lstrip('?') for s in game}
    if bases != GAME_IMPORT_BASES: raise RuntimeError(f'Unreviewed/missing game imports: {bases ^ GAME_IMPORT_BASES}')
    for s in game:
        if s.startswith('?mStayNo@') or s.startswith('?mStatus@'):
            if '@dStage_roomControl_c@@' not in s: raise RuntimeError('Wrong data owner')
        if s.startswith('?getLayerNo@') and '@dComIfG_play_c@@' not in s:
            raise RuntimeError('Wrong logical layer getter owner')
    meta = next(s for s in sections if s[0] == 'modmeta')
    records = []; n = meta[3]; end = n + min(meta[2], meta[4])
    while n + 8 <= end:
        if not any(b[n:n+8]): n += 8; continue
        size, kind, flags = struct.unpack_from('<HBB', b, n)
        if size < 8 or size % 8 or n + size > end: raise RuntimeError('Invalid metadata')
        if kind == 1:
            if size != 8 or u32(n+4) != 1: raise RuntimeError('Wrong module ABI')
            records.append({'kind':'header','abi':1})
        elif kind == 2:
            if size != 80 or flags != 0: raise RuntimeError('Required service record expected')
            major, minor = struct.unpack_from('<HH', b, n+4)
            records.append({'kind':'import','id':cstr(n+16),'major':major,'minor':minor})
        elif kind == 5:
            if flags != 0 or u32(n+4) != 0 or any(b[n+16:n+32]): raise RuntimeError('Unexpected PMF adjustment/resolved slot/flags')
            names = [s.decode('ascii') for s in b[n+32:n+size].split(b'\0') if s]
            expected_names = {
                'daE_OC_c::execute': ['??_7daE_OC_c@@6B@', 'daE_OC_c::execute'],
                'J3DShapePacket::drawFast': ['??_7J3DShapePacket@@6B@', 'J3DShapePacket::drawFast'],
            }
            if len(names)!=2 or expected_names.get(names[1])!=names:raise RuntimeError('Unexpected hook metadata '+str(names))
            expected_size=(32+sum(len(s)+1 for s in names)+7)//8*8
            if size!=expected_size:raise RuntimeError('Hook metadata record size')
            records.append({'kind':'hook_mem','target':names[1],'size':size,'pointer_to_member_bytes':8})
        else: raise RuntimeError('Forbidden metadata kind '+str(kind))
        n += size
    expected = {(f'dev.twilitrealm.dusklight.{id}', major, minor)
                for id,major,minor in [('host',2,2),('log',1,0),('game',2,0),('stage',1,0),('hook',1,1),('file',1,0)]}
    actual = {(r['id'],r['major'],r['minor']) for r in records if r['kind']=='import'}
    if actual != expected or len(records) != 9: raise RuntimeError('Unexpected metadata services')
    return {'machine':'AMD64', 'imports':imports, 'metadata':records, 'status':'PASS_STATIC'}

def inspect_pe(path):
    r=pe_audit(path);r["exports"]=pe_exports(path)
    raw=path.read_bytes();_,opt,at=pe_parts(raw)
    for directory in [9,13]:
        if any(struct.unpack_from("<II",raw,opt+112+8*directory)):raise RuntimeError("TLS/delay import directory")
    expected=json.loads((Path(__file__).parent/"reviewed-imports.json").read_text())
    actual={k:sorted(v) for k,v in r["imports"].items()}
    if any(len(v)!=len(set(v)) for v in actual.values()):raise RuntimeError("Duplicate import")
    if actual!=expected:raise RuntimeError("EXACT_IMPORT_REVIEW_PENDING "+json.dumps(actual,sort_keys=True))
    targets={x["target"] for x in r["metadata"] if x["kind"]=="hook_mem"}
    if targets!={"daE_OC_c::execute","J3DShapePacket::drawFast"}:raise RuntimeError("Typed target surface")
    bound=[]
    # Bind each compact PMF to the exact IAT import, not merely a symbol name in a string.
    pe=struct.unpack_from("<I",raw,0x3c)[0];base=struct.unpack_from("<Q",raw,opt+24)[0]
    slots={};n=at(struct.unpack_from("<I",raw,opt+120)[0])
    while any(raw[n:n+20]):
        ilt,_,_,name,iat=struct.unpack_from("<5I",raw,n);t=at(ilt or iat);index=0
        while (v:=struct.unpack_from("<Q",raw,t+8*index)[0]):
            p=at(v)+2;slots[iat+8*index]=raw[p:raw.index(0,p)].decode();index+=1
        n+=20
    meta=None
    for i in range(struct.unpack_from("<H",raw,pe+6)[0]):
        q=opt+struct.unpack_from("<H",raw,pe+20)[0]+40*i
        if raw[q:q+8].rstrip(b"\0")==b"modmeta":meta=struct.unpack_from("<4I",raw,q+8)
    if not meta:raise RuntimeError("Missing metadata")
    vs,vr,rs,off=meta;n=off
    while n+8<=off+min(vs,rs):
        if not any(raw[n:n+8]):n+=8;continue
        size,kind,flags=struct.unpack_from("<HBB",raw,n)
        if kind==5:
            pointer=struct.unpack_from("<Q",raw,n+8)[0]-base;q=at(pointer)
            if raw[q:q+2]!=b"\xff\x25":raise RuntimeError("PMF not import thunk")
            slot=pointer+6+struct.unpack_from("<i",raw,q+2)[0];bound.append(slots.get(slot))
        n+=size
    if set(bound)!={"?execute@daE_OC_c@@QEAAHXZ","?drawFast@J3DShapePacket@@QEAAXXZ"}:raise RuntimeError("Typed PMF import binding mismatch")
    r.update(status="CP06_PE_ABI_PASS",hook_bound_imports=sorted(bound),tls_or_delay_imports=False,game_or_dll_executed=False)
    return r

def verify_package(path,dll,project,work):
    pe=inspect_pe(dll)
    with zipfile.ZipFile(path) as z:
        members=safe_members(z);manifest=z.read("mod.json");inside=z.read("lib/windows-amd64/mod.dll")
        if {i.filename for i in z.infolist() if i.is_dir()}!=ALLOWED_DIRECTORIES:raise RuntimeError("Official package directories")
        if manifest!=(project/"mod.json").read_bytes() or inside!=dll.read_bytes():raise RuntimeError("Package manifest or DLL mismatch")
    parsed=json.loads(manifest)
    if parsed["id"]!="dev.devf.cp06_single_site" or parsed["version"]!="0.6.0":raise RuntimeError("Package identity/version")
    copy=work/"internal-audit.dll";copy.write_bytes(inside)
    if inspect_pe(copy)!=pe:raise RuntimeError("Internal PE changed")
    return {"status":"PACKAGE_VERIFIED","package_bytes":path.stat().st_size,"package_sha256":sha(path.read_bytes()),"dll_bytes":len(inside),"dll_sha256":sha(inside),"manifest_sha256":sha(manifest),"members":members,"pe_audit":pe,"original_assets_included":False,"game_or_dll_executed":False}

def negatives(dll,work):
    result=[];raw=dll.read_bytes()
    def reject(name,b):
        p=work/("negative-"+name+".dll");p.write_bytes(b)
        try:inspect_pe(p)
        except (RuntimeError,ValueError,StopIteration,struct.error):result.append({"case":name,"rejected":True})
        else:raise RuntimeError("Negative survived: "+name)
    pe,_,_=pe_parts(raw);mut=bytearray(raw);struct.pack_into("<H",mut,pe+4,0x14c);reject("x86",mut)
    for name,before,after in [("wrong_packet_hook",b"J3DShapePacket::drawFast",b"J3DShapePacket::drawFasX"),("wrong_hook_service",b"dev.twilitrealm.dusklight.hook",b"dev.twilitrealm.dusklight.hoox"),("wrong_export",b"mod_update",b"mod_updatX")]:
        if before not in raw:raise RuntimeError("Negative marker missing")
        reject(name,raw.replace(before,after))
    def bad_zip(name,entries):
        p=work/("negative-"+name+".zip")
        with zipfile.ZipFile(p,"w") as z:
            for n,b in entries:z.writestr(n,b)
        try:
            with zipfile.ZipFile(p) as z:safe_members(z)
        except (RuntimeError,ValueError,zipfile.BadZipFile):result.append({"case":name,"rejected":True})
        else:raise RuntimeError("ZIP negative survived")
    bad_zip("original_arc_forbidden",[("mod.json",b"{}"),("lib/windows-amd64/mod.dll",raw),("E_oc2.arc",b"dummy-not-an-asset")])
    bad_zip("path_escape",[("../mod.json",b"{}"),("lib/windows-amd64/mod.dll",raw)])
    return result
