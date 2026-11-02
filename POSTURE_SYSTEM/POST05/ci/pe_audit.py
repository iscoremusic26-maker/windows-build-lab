"""POST05: auditoria de solo lectura de un mod.dll (PE32+ AMD64). Sin ejecutar nada.
Reglas duras: seis HOOK_MEM revisados (cinco enlazados a su thunk de importacion del EXE; el sexto, LNK01, un thunk vcall local de slot 0); sin exports de servicio,
sin TLS/delay-imports; ningun import mutador; imports del juego y de CRT/sistema EXACTAMENTE iguales a ci/expected_pe.json
(revisado a mano la primera vez que se genero). Uso: pe_audit.py <dll> [--expected ci/expected_pe.json] -> JSON en stdout."""
import json
import struct
import sys
from pathlib import Path

HOOK_NAMES = ['??_7daE_OC_c@@6B@', 'daE_OC_c::damage_check']
HOOK_VTABLE = {'daAlink_c':'??_7daAlink_c@@6B@', 'daE_OC_c': '??_7daE_OC_c@@6B@', 'dMeter2_c': '??_7dMeter2_c@@6B@', 'dMeterButton_c': '??_7dMeterButton_c@@6B@'}
# LNK01: the ONLY virtual hook target. Its PMF points to a LOCAL MSVC vcall thunk (no import); the host follows it to the vtable slot
# (src/dusk/mods/svc/hook.cpp vcall_slot_offset) and resolves the overrider by display name (manifest) or by the vtable symbol. Slot must be 0 (draw).
VCALL_HOOKS = {'dMeterButton_c::draw': 0}
HOOK_IMPORT = '?damage_check@daE_OC_c@@QEAAXXZ'
# Imports del juego REVISADOS y su razon. Cualquier otro simbolo del EXE es un fallo. Ninguno muta estado del juego:
GAME_IMPORT_REASONS = {
    '?checkDamageAction@daAlink_c@@QEAAHXZ':'FIX01 readonly PRE/POST native damage receipt',
    '?checkPolyDamage@daAlink_c@@QEAAHXZ':'FIX01 scoped readonly native environmental-branch result',
    '?cc_set@daE_OC_c@@QEAAXXZ':'FIX01 paired own At registration cleanup',
    '?executeAttack@daE_OC_c@@QEAAXXZ':'FIX01 readonly POST actual native AtSet evidence',

    '?executeDamage@daE_OC_c@@QEAAXXZ':'POST only; suppress premature HeadLock until real Shield Attack',
    '?checkMasterSwordEquip@daPy_py_c@@SAHXZ':'Read-only sword identity for CP21 Helm 100/50 rule',
    '?dComIfGp_getLinkPlayer@@YAPEAVdaPy_py_c@@XZ':'Read-only live Link lookup; no cached player pointer',
    '?dComIfGs_getSelectEquipSword@@YAEXZ':'Read-only selected sword id for Wood exclusion',
    '?damage_check@daE_OC_c@@QEAAXXZ': 'thunk del unico hook (PRE/POST observacionales)',
    '?fopAcM_SearchByID@@YAHIPEAPEAVfopAc_ac_c@@@Z': 'consulta de existencia de solo lectura (fpcM_IsCreating + recorrido de g_fopAcTg_Queue), solo en mod_update',
    '?dComIfGp_getStartStageName@@YAPEBDXZ': 'lectura del nombre del stage de inicio para el epoch de contexto (puntero a miembro)',
    # POST07 HUD (display only): the same native 2D path Enemy Health Bars CP06 imports from the same EXE (checked against its shipped DLL).
    '?_execute@dMeter2_c@@QEAAHXZ': 'POST07 hook thunk: HUD meter execute POST = simulation tick counter',
    '?_draw@dMeter2_c@@QEAAHXZ': 'POST07 hook thunk: HUD meter draw POST = sample + queue our 2D packet',
    '?_delete@dMeter2_c@@QEAAHXZ': 'POST07 hook thunk: HUD meter delete PRE = scrub our packet (always continues)',
    '??0J2DOrthoGraph@@QEAA@MMMMMM@Z': 'POST07 painter: local ortho context (stack object)',
    '?setOrtho@J2DOrthoGraph@@QEAAXAEBU?$TBox2@M@JGeometry@@MM@Z': 'POST07 painter: logical widescreen bounds of the local context',
    '?setPort@J2DOrthoGraph@@UEAAXXZ': 'POST07 painter: viewport/projection of the local context',
    '?setColor@J2DGrafContext@@QEAAXUTColor@JUtility@@000@Z': 'POST07 painter: per-corner colours of the local context',
    '?fillBox@J2DGrafContext@@QEAAXAEBU?$TBox2@M@JGeometry@@@Z': 'POST07 painter: one quad of the posture bar',
    '?dComIfGp_getCurrentGrafPort@@YAPEAVJ2DGrafContext@@XZ': 'POST07 painter: restore the native 2D port after painting',
    '?mDoLib_project@@YAXPEAUVec@@0@Z': 'POST07: world anchor -> ortho point (pure projection)',
    '?dComIfGd_getView@@YAPEAUview_class@@XZ': 'POST07: projection precondition (view present)',
    '?dCam_getBody@@YAPEAVdCamera_c@@XZ': 'POST07: camera for the read-only line-of-sight query',
    '?lineBGCheck@dCamera_c@@QEAA_NPEAUcXyz@@0I@Z': 'POST07: read-only camera line of sight to the anchor (same flags as EHB)',
    '?fopAcM_searchActorDistance@@YAMPEBVfopAc_ac_c@@0@Z': 'POST07: player distance for the 3700-4500 fade (pure)',
    '?dMeter2Info_getMeterClass@@YAPEAVdMeter2_c@@XZ': 'POST07 painter: HUD meter state re-check inside the painter',
    '?dMeter2Info_getWindowStatus@@YAEXZ': 'POST07: menu window open => no HUD',
    '?g_dComIfG_gameInfo@@3VdComIfG_inf_c@@A': 'POST07: game info (pause/event/player life/camera; draw list for our packet)',
    '?g_fopAcTg_Queue@@3Unode_list_class@@A': 'POST07: ONE read-only walk of the live actor queue per simulation tick (hud_sim_pass)',
    '?m_widthF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport width (read)',
    '?m_heightF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport height (read)',
    '?m_minXF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport left (read)',
    '?m_minYF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport top (read)',
    '?set@dDlst_list_c@@QEAAHAEAPEAPEAVdDlst_base_c@@0PEAV2@@Z': 'POST07: native 2D list insertion (inline set2DXlu) of OUR packet only',
    'PSVECSquareMag': 'POST07: cXyz::abs() of the camera-to-anchor vector (pure)',
    # POST07, found on the first MSVC link (run 41392200339, module map): /Zc:inline- + the inline virtual ~J2DOrthoGraph make MSVC emit
    # ??_7J2DGrafContext@@6B@ in mod.cpp.obj; its slots reference these EXE virtuals. Our code never calls them directly (the local ortho object
    # lives on the stack; setPort is called through J2DOrthoGraph::setPort, which the EXE implements by calling J2DGrafContext::setPort itself).
    '?place@J2DGrafContext@@UEAAXAEBU?$TBox2@M@JGeometry@@@Z': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
    '?setPort@J2DGrafContext@@UEAAXXZ': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
    '?setScissor@J2DGrafContext@@UEAAXXZ': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
    '?setup2D@J2DGrafContext@@UEAAXXZ': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
}
KINDS = {0: 'PAD', 1: 'HEADER', 2: 'IMPORT', 3: 'EXPORT', 4: 'HOOK_FN', 5: 'HOOK_MEM', 6: 'HOOK_NAME', 7: 'HOOK_MEM_EXT'}
# Simbolos cuyo IMPORT o presencia indicaria un uso que modifica el juego o parchea memoria (el patron exacto lo fija expected_pe.json).
FORBIDDEN = ['setActionMode', 'offTgSph', 'cc_at_check', 'at_power_check', 'ClrTgHit', 'ResetTgHit', 'onDownFlg', 'offDownFlg',
             'setStabPos', 'executeBigDamage', 'executeDeath', 'OffTgSetBit', 'OnTgSetBit', 'OffCoSetBit',
             'OnCoSetBit', 'ClrCcMove', 'fopAcM_delete', 'dComIfGp_setHitMark', 'WriteProcessMemory', 'VirtualProtect', 'funchook']


class PE:
    def __init__(self, raw):
        self.raw = raw
        if raw[:2] != b'MZ':
            raise ValueError('not MZ')
        self.pe = struct.unpack_from('<I', raw, 0x3c)[0]
        if raw[self.pe:self.pe + 4] != b'PE\0\0':
            raise ValueError('not PE')
        self.machine, self.nsec = struct.unpack_from('<HH', raw, self.pe + 4)
        optsize = struct.unpack_from('<H', raw, self.pe + 20)[0]
        self.opt = self.pe + 24
        if struct.unpack_from('<H', raw, self.opt)[0] != 0x20B:
            raise ValueError('not PE32+')
        self.image_base = struct.unpack_from('<Q', raw, self.opt + 24)[0]
        self.sections = []
        s = self.opt + optsize
        for i in range(self.nsec):
            o = s + 40 * i
            name = raw[o:o + 8].rstrip(b'\0').decode('ascii', 'replace')
            vs, va, rs, ro = struct.unpack_from('<IIII', raw, o + 8)
            self.sections.append((name, vs, va, rs, ro))

    def datadir(self, i):
        return struct.unpack_from('<II', self.raw, self.opt + 112 + 8 * i)

    def off(self, rva):
        for _, vs, va, rs, ro in self.sections:
            if va <= rva < va + max(vs, rs) and rva - va < rs:
                return ro + rva - va
        raise ValueError('unmapped rva %#x' % rva)

    def cstr(self, rva):
        o = self.off(rva)
        return self.raw[o:self.raw.index(b'\0', o)].decode('ascii', 'replace')

    def imports(self):
        out, slots = {}, {}
        rva, size = self.datadir(1)
        if not rva:
            return out, slots
        q = self.off(rva)
        while any(self.raw[q:q + 20]):
            oft, _, _, name, ft = struct.unpack_from('<5I', self.raw, q)
            dll = self.cstr(name)
            t = self.off(oft or ft)
            i = 0
            names = []
            while True:
                v = struct.unpack_from('<Q', self.raw, t + 8 * i)[0]
                if not v:
                    break
                if v >> 63:
                    n = 'ORDINAL#%d' % (v & 0xFFFF)
                else:
                    n = self.cstr((v & 0x7FFFFFFF) + 2)
                names.append(n)
                slots[ft + 8 * i] = (dll, n)
                i += 1
            out[dll] = sorted(names)
            q += 20
        return out, slots

    def exports(self):
        rva, size = self.datadir(0)
        if not rva:
            return []
        o = self.off(rva)
        count = struct.unpack_from('<I', self.raw, o + 24)[0]
        names_rva = struct.unpack_from('<I', self.raw, o + 32)[0]
        no = self.off(names_rva)
        return sorted(self.cstr(struct.unpack_from('<I', self.raw, no + 4 * i)[0]) for i in range(count))

    def meta_records(self):
        sec = [s for s in self.sections if s[0] == 'modmeta']
        if len(sec) != 1:
            raise ValueError('modmeta section count %d' % len(sec))
        _, vs, _, rs, ro = sec[0]
        end = ro + min(vs, rs)
        n = ro
        recs = []
        while n + 8 <= end:
            if not any(self.raw[n:n + 8]):
                n += 8
                continue
            size, kind, flags = struct.unpack_from('<HBB', self.raw, n)
            if size < 8 or size % 8 or n + size > end:
                raise ValueError('bad meta record size %d at %#x' % (size, n))
            rec = {'kind': KINDS.get(kind, 'KIND_%d' % kind), 'size': size, 'flags': flags}
            if kind == 1:
                rec['abi'] = struct.unpack_from('<I', self.raw, n + 4)[0]
            elif kind == 2:
                rec['major'], rec['min_minor'] = struct.unpack_from('<HH', self.raw, n + 4)
                rec['service'] = self.raw[n + 16:n + 80].split(b'\0')[0].decode('ascii', 'replace')
            elif kind == 5:
                rec['pmf'] = self.raw[n + 8:n + 24]
                rec['resolved'] = struct.unpack_from('<Q', self.raw, n + 24)[0]
                rec['names'] = [s.decode('ascii') for s in self.raw[n + 32:n + size].split(b'\0') if s]
            elif kind == 7:
                rec['pmf_size'] = struct.unpack_from('<I', self.raw, n+4)[0]
                rec['materialize'] = struct.unpack_from('<Q', self.raw, n+8)[0]
                rec['resolved'] = struct.unpack_from('<Q', self.raw, n+16)[0]
                rec['names'] = [s.decode('ascii') for s in self.raw[n+24:n+size].split(b'\0') if s]
            recs.append(rec)
            n += size
        return recs


def vcall_slot(raw, p, va):
    """Python replica of the host's x64 vcall_slot_offset: follow E9 stubs, then match mov rax,[rcx]; jmp [rax(+imm)] (MSVC) or
    mov rax,[rcx]; mov rax,[rax(+imm)]; jmp rax (clang-cl). Returns the slot byte offset or None."""
    o = p.off(va - p.image_base)
    for _ in range(8):
        if raw[o] != 0xE9:
            break
        va = va + 5 + struct.unpack_from('<i', raw, o + 1)[0]
        o = p.off(va - p.image_base)
    w = raw[o:o + 32]
    q = None
    for i in range(13):
        if w[i:i + 3] == b'\x48\x8b\x01':
            q = i + 3
            break
    if q is None:
        return None
    if w[q:q + 2] == b'\xff\x20':
        return 0
    if w[q:q + 2] == b'\xff\x60':
        return struct.unpack_from('<b', w, q + 2)[0]
    if w[q:q + 2] == b'\xff\xa0':
        return struct.unpack_from('<i', w, q + 2)[0]
    if w[q:q + 2] == b'\x48\x8b' and w[q + 2] in (0x00, 0x40, 0x80):
        off, r = 0, q + 3
        if w[q + 2] == 0x40:
            off, r = struct.unpack_from('<b', w, q + 3)[0], q + 4
        elif w[q + 2] == 0x80:
            off, r = struct.unpack_from('<i', w, q + 3)[0], q + 7
        if any(w[r + i:r + i + 2] == b'\xff\xe0' for i in range(9)):
            return off
    return None


def audit(path, expected=None):
    raw = Path(path).read_bytes()
    p = PE(raw)
    r = {'file_bytes': len(raw), 'machine': hex(p.machine), 'sections': [s[0] for s in p.sections]}
    errors = []
    if p.machine != 0x8664:
        errors.append('machine is not AMD64')
    for idx, label in [(9, 'TLS'), (13, 'DELAY_IMPORT')]:
        if any(p.datadir(idx)):
            errors.append(label + ' directory present')
    imports, slots = p.imports()
    r['imports'] = imports
    r['exports'] = p.exports()
    recs = p.meta_records()
    r['meta_kinds'] = {}
    for rec in recs:
        r['meta_kinds'][rec['kind']] = r['meta_kinds'].get(rec['kind'], 0) + 1
    r['meta_imports'] = sorted((x['service'], x['major'], x['min_minor']) for x in recs if x['kind'] == 'IMPORT')
    hooks = [x for x in recs if x['kind'] in ('HOOK_MEM','HOOK_MEM_EXT')]
    if r['meta_kinds'].get('HOOK_MEM') != 8 or r['meta_kinds'].get('HOOK_MEM_EXT') != 2 or any(k in r['meta_kinds'] for k in ('HOOK_FN', 'HOOK_NAME', 'EXPORT')):
        errors.append('expected exactly8 HOOK_MEM +2 audited Link HOOK_MEM_EXT and no other hook/export records: %s' % r['meta_kinds'])
    if r['meta_kinds'].get('HEADER') != 1:
        errors.append('expected one HEADER record')
    bound = None
    expected_hooks={'daE_OC_c::damage_check':HOOK_IMPORT,'daE_OC_c::executeDamage':'?executeDamage@daE_OC_c@@QEAAXXZ',
                    'dMeter2_c::_execute':'?_execute@dMeter2_c@@QEAAHXZ','dMeter2_c::_draw':'?_draw@dMeter2_c@@QEAAHXZ','dMeter2_c::_delete':'?_delete@dMeter2_c@@QEAAHXZ',
                    'dMeterButton_c::draw':None,'daAlink_c::checkDamageAction':'?checkDamageAction@daAlink_c@@QEAAHXZ','daAlink_c::checkPolyDamage':'?checkPolyDamage@daAlink_c@@QEAAHXZ','daE_OC_c::executeAttack':'?executeAttack@daE_OC_c@@QEAAXXZ','daE_OC_c::cc_set':'?cc_set@daE_OC_c@@QEAAXXZ'}
    if {h['names'][-1] for h in hooks}!=set(expected_hooks):errors.append('unexpected hook target set')
    for h in hooks:
        if h['kind']=='HOOK_MEM_EXT':
            try:
                if h['names'][-1] not in ('daAlink_c::checkDamageAction','daAlink_c::checkPolyDamage') or h['pmf_size']!=24 or h['flags']!=0:
                    raise ValueError('unapproved extended member target/layout')
                from pmf_audit import decode_materializer
                h['pmf']=decode_materializer(p,h['materialize'],h['pmf_size'],slots)
                r.setdefault('extended_materializers',{})[h['names'][-1]]={'size':24,'static_decode':'READ_ONLY_STORES_TO_OUTPUT_ONLY','target_and_adjustments_hex':h['pmf'][:20].hex(),'padding':'bytes20..23 unspecified native ABI padding; never interpreted'}
            except (ValueError,KeyError,IndexError) as ex:
                errors.append('extended PMF rejected: '+str(ex));continue
        elif h['names'][-1].startswith('daAlink_c::'):
            errors.append('Link hook must preserve SDK native extended PMF representation')
        if h['names'][-1] not in expected_hooks or h['names'] != [HOOK_VTABLE[h['names'][-1].split('::')[0]], h['names'][-1]]:
            errors.append('hook names %s' % h['names'])
        if h['resolved'] != 0 or any(h['pmf'][8:]):
            errors.append('PMF slot not a zero-adjustment compact pointer / resolved slot not initially zero')
        va = struct.unpack_from('<Q', h['pmf'], 0)[0]
        if h['names'][-1] in VCALL_HOOKS:  # LNK01: local vcall thunk, exact slot, never an import
            try:
                slot_off = vcall_slot(raw, p, va)
            except (ValueError, IndexError) as ex:
                slot_off = None
                errors.append('vcall PMF target unmapped: %s' % ex)
            r.setdefault('vcall_hooks', {})[h['names'][-1]] = slot_off
            if slot_off != VCALL_HOOKS[h['names'][-1]]:
                errors.append('virtual hook %s: PMF is not a slot-%d vcall thunk (got %s)' % (h['names'][-1], VCALL_HOOKS[h['names'][-1]], slot_off))
            r.setdefault('hook_thunks', {})[h['names'][-1]] = None
            continue
        try:
            o = p.off(va - p.image_base)
            if raw[o:o + 2] != b'\xff\x25':
                errors.append('PMF does not point to an import thunk')
            else:
                slot = (va - p.image_base) + 6 + struct.unpack_from('<i', raw, o + 2)[0]
                bound = slots.get(slot)
        except ValueError as ex:
            errors.append('PMF target unmapped: %s' % ex)
        r.setdefault('hook_thunks', {})[h['names'][-1]] = list(bound) if bound else None
        r['hook_thunk_bound_import'] = r['hook_thunks'].get('daE_OC_c::damage_check')
        if not bound or bound != ('dusklight.exe', expected_hooks.get(h['names'][-1])):
            errors.append('hook thunk bound to %s' % (bound,))
    flat = [n for names in imports.values() for n in names]
    game = sorted(imports.get('dusklight.exe', []))
    r['game_imports'] = {n: GAME_IMPORT_REASONS.get(n, 'UNREVIEWED') for n in game}
    for n in game:
        if n not in GAME_IMPORT_REASONS:
            errors.append('unreviewed game import ' + n)
    for n in GAME_IMPORT_REASONS:
        if n not in game:
            errors.append('reviewed game import missing: ' + n)
    for bad in FORBIDDEN:
        hit = [n for n in flat if bad in n] + [e for e in r['exports'] if bad in e]
        if hit:
            errors.append('forbidden symbol %s in imports/exports: %s' % (bad, hit))
        if ('?' + bad + '@').encode() in raw:
            # Un nombre MSVC decorado de un mutador (?nombre@...) no debe aparecer en la imagen; el texto libre de un log (p. ej. «cc_at_check=UNDETERMINED») no es un simbolo.
            errors.append('forbidden decorated symbol ?%s@ present in image' % bad)
    r['errors'] = errors
    if expected is not None:
        exp = json.loads(Path(expected).read_text())
        for key in ('imports', 'exports', 'meta_imports', 'meta_kinds'):
            got = json.loads(json.dumps(r[key]))
            if got != exp[key]:
                errors.append('PIN_MISMATCH %s: got %s expected %s' % (key, got, exp[key]))
        r['pinned'] = True
    else:
        r['pinned'] = False
    r['status'] = 'PE_AUDIT_PASS' if not errors and r['pinned'] else ('PE_AUDIT_UNPINNED' if not errors else 'PE_AUDIT_FAIL')
    return r


if __name__ == '__main__':
    exp = None
    if '--expected' in sys.argv:
        exp = sys.argv[sys.argv.index('--expected') + 1]
    res = audit(sys.argv[1], exp)
    print(json.dumps(res, indent=2, sort_keys=True))
    sys.exit(0 if res['status'] == 'PE_AUDIT_PASS' else 2)
