"""Fail closed on observer services, native call surface and linked PE metadata/imports.
This is a scoped review aid, not a general proof of arbitrary C++ non-mutation.
"""
import json
import re
import struct
from pathlib import Path

NATIVE_CALLS = {
    'dComIfGp_isEnableNextStage', 'dComIfGp_getStartStageName', 'getStayNo',
    'fpcLy_RootLayer', 'fpcLy_CurrentLayer', 'dComIfGp_getPlayer', 'fopAcIt_Judge',
    'fpcM_IsCreating', 'getStatusProcID', 'fopScnM_SearchByID', 'getLayerNo',
    'dComIfG_getObjectResInfo',
}
GAME_IMPORT_BASES = {
    'dComIfGp_isEnableNextStage', 'dComIfGp_getStartStageName', 'dComIfGp_getPlayer',
    'fpcLy_RootLayer', 'fpcLy_CurrentLayer', 'fopAcIt_Judge', 'fpcM_IsCreating',
    'fopScnM_SearchByID', 'getLayerNo', 'dComIfG_getObjectResInfo', 'mStayNo', 'mStatus',
}

def source_audit(project):
    sources = list((project / 'src').glob('*'))
    text = '\n'.join(p.read_text(encoding='utf-8') for p in sources)
    for pattern in (r'\b(?:ActorService|StageService|HookService|ResourceService)\b',
                    r'\b(?:add_actor|register_profile|create_actor|delete_actor|remove_actor)\s*\(',
                    r'\b(?:DEFINE_HOOK|EXPORT_SERVICE|IMPORT_OPTIONAL_SERVICE|std::thread)\b',
                    r'\b(?:fopen|ofstream|CreateFile|WriteFile|VirtualProtect|LoadLibrary)\s*\('):
        if re.search(pattern, text): raise RuntimeError(f'Forbidden source surface: {pattern}')
    native = (project / 'src/native_reader.cpp').read_text(encoding='utf-8')
    calls = set(re.findall(r'\b((?:dComIf|fpc|fop)[A-Za-z0-9_]+|getStayNo|getStatusProcID|getLayerNo)\s*\(', native))
    if calls != NATIVE_CALLS: raise RuntimeError(f'Unexpected/missing native calls: {calls ^ NATIVE_CALLS}')
    if re.findall(r'IMPORT_SERVICE\((\w+),', text) != ['HostService', 'LogService']:
        raise RuntimeError('Only explicit Host/Log services permitted')
    return {'native_calls': sorted(calls), 'explicit_services': ['Host 2.2', 'Log 1.0'],
            'automatic_service': 'Game epoch 2', 'source_surface': 'PASS_STATIC'}

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
                for id,major,minor in [('host',2,2),('log',1,0),('game',2,0)]}
    actual = {(r['id'],r['major'],r['minor']) for r in records if r['kind']=='import'}
    if actual != expected or len(records) != 4: raise RuntimeError('Unexpected metadata services')
    return {'machine':'AMD64', 'imports':imports, 'metadata':records, 'status':'PASS_STATIC'}

if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(); p.add_argument('--project', type=Path, required=True)
    a = p.parse_args(); print(json.dumps(source_audit(a.project), indent=2))
