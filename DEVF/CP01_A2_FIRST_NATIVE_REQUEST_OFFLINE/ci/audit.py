"""Scoped experimental source/PE review. Not proof of arbitrary C++ safety."""
import json,re,struct,hashlib
from pathlib import Path
GAME_IMPORT_BASES={'mStatus', 'GetSpecialCode', 'fpcLy_CurrentLayer', 'fopAcIt_Judge', 'dComIfGp_getPlayer', '1dBgS_ObjGndChk_Wtr', 'GroundCross', 'dComIfGp_getStartStageName', 'getLayerNo', 'fpcLy_SetCurrentLayer', 'GetPolyColor', 'dComIfGp_isEnableNextStage', 'SetObj', 'mNextStayNo', 'ChkPolySafe', 'dComIfG_getObjectResInfo', '0dBgS_GndChk', 'fpcLy_IsDeletingMesg', 'dComIfG_Bgsp', 'SetPos', '1dBgS_GndChk', 'mProcID', 'fopScnM_SearchByID', 'fpcCtIt_Judge', '0dBgS_ObjGndChk_Wtr', 'dComIfGp_event_runCheck', 'fpcLy_RootLayer', 'dComIfGp_roomControl_checkStatusFlag', 'mStayNo', 'fpcM_IsCreating', 'GetRoomId', 'GetGroundCode'}

def source_audit(project):
 native=(project/'src/native_backend.cpp').read_text();core=(project/'src/experiment_core.cpp').read_text();mod=(project/'src/experimental_mod.cpp').read_text()
 text='\n'.join(f.read_text() for f in (project/'src').glob('*'))
 for pattern in [r'\b(?:delete_actor|register_actor|unregister_actor|create_child_actor|create_actor_from_name|add_actor|fpcLy_Cancel|fpcCtRq_Cancel|fopAcM_delete|fopAcM_create|get_actor_id)\s*\(',r'\b(?:StageService|HookService|ResourceService|DEFINE_HOOK|EXPORT_SERVICE|std::thread|std::async)\b',r'\b(?:CreateThread|VirtualProtect|CreateFile|WriteFile|LoadLibrary|system|fopen)\s*\(']:
  if re.search(pattern,text):raise RuntimeError('Forbidden experimental surface '+pattern)
 if native.count('svc_actor->create_actor(')!=1 or core.count('b.request(preset)')!=1:raise RuntimeError('Not exactly one request site')
 if re.findall(r'IMPORT_SERVICE\((\w+),',mod)!=['HostService','LogService','ActorService']:raise RuntimeError('Wrong service imports')
 if native.count('fpcLy_SetCurrentLayer(')!=1 or native.count('fpcCtIt_Judge(')!=1 or native.count('fopAcIt_Judge(')!=1:raise RuntimeError('Wrong native census/setter graph')
 if 'Admission current_admission() noexcept { return {}; }' not in (project/'src/admission.cpp').read_text():raise RuntimeError('Native admission must deny')
 cmake=re.sub(r'#[^\n]*','',(project/'CMakeLists.txt').read_text())
 if re.search(r'\badd_mod\s*\(',cmake) or (project/'mod.json').exists():raise RuntimeError('Packaging is forbidden')
 if 'consumed_=true;state_=State::AttemptConsumed;' not in core or 'if(!scope.restore())stop(Reason::Restore);' not in core:raise RuntimeError('Missing latch/scope')
 return {'status':'PASS_STATIC','single_request_site':True,'current_admission':'NONE_ALWAYS','native_services':['Host2.2','Log1','Game2','Actor1'],'new_game_imports':['fpcLy_SetCurrentLayer','fpcCtIt_Judge'],'reuse':'unchanged read-only bridge; own minimal guarded core','no_package_target':True,'scope':'reviewed source graph, not runtime lifetime guarantee'}

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
        else: raise RuntimeError(f'Forbidden metadata kind {kind}, including hooks/exports')
        n += size
    expected = {(f'dev.twilitrealm.dusklight.{id}', major, minor)
                for id,major,minor in [('host',2,2),('log',1,0),('game',2,0),('actor',1,0)]}
    actual = {(r['id'],r['major'],r['minor']) for r in records if r['kind']=='import'}
    if actual != expected or len(records) != 5: raise RuntimeError('Unexpected metadata services')
    return {'machine':'AMD64', 'imports':imports, 'metadata':records, 'status':'PASS_STATIC'}

