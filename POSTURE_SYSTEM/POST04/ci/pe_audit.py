"""POST04: auditoria de solo lectura de un mod.dll (PE32+ AMD64). Sin ejecutar nada.
Reglas duras: un unico registro HOOK_MEM sobre daE_OC_c::damage_check enlazado a su thunk de importacion; sin exports de servicio,
sin TLS/delay-imports; ningun import mutador; imports del juego y de CRT/sistema EXACTAMENTE iguales a ci/expected_pe.json
(revisado a mano la primera vez que se genero). Uso: pe_audit.py <dll> [--expected ci/expected_pe.json] -> JSON en stdout."""
import json
import struct
import sys
from pathlib import Path

HOOK_NAMES = ['??_7daE_OC_c@@6B@', 'daE_OC_c::damage_check']
HOOK_IMPORT = '?damage_check@daE_OC_c@@QEAAXXZ'
# Imports del juego REVISADOS y su razon. Cualquier otro simbolo del EXE es un fallo. Ninguno muta estado del juego:
GAME_IMPORT_REASONS = {
    '?damage_check@daE_OC_c@@QEAAXXZ': 'thunk del unico hook (PRE/POST observacionales)',
    '?fopAcM_SearchByID@@YAHIPEAPEAVfopAc_ac_c@@@Z': 'consulta de existencia de solo lectura (fpcM_IsCreating + recorrido de g_fopAcTg_Queue), solo en mod_update',
    '?dComIfGp_getStartStageName@@YAPEBDXZ': 'lectura del nombre del stage de inicio para el epoch de contexto (puntero a miembro)',
}
KINDS = {0: 'PAD', 1: 'HEADER', 2: 'IMPORT', 3: 'EXPORT', 4: 'HOOK_FN', 5: 'HOOK_MEM', 6: 'HOOK_NAME', 7: 'HOOK_MEM_EXT'}
# Simbolos cuyo IMPORT o presencia indicaria un uso que modifica el juego o parchea memoria (el patron exacto lo fija expected_pe.json).
FORBIDDEN = ['setActionMode', 'offTgSph', 'cc_at_check', 'at_power_check', 'ClrTgHit', 'ResetTgHit', 'onDownFlg', 'offDownFlg',
             'setStabPos', 'executeBigDamage', 'executeDamage', 'executeDeath', 'OffTgSetBit', 'OnTgSetBit', 'OffCoSetBit',
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
            recs.append(rec)
            n += size
        return recs


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
    hooks = [x for x in recs if x['kind'] == 'HOOK_MEM']
    if r['meta_kinds'].get('HOOK_MEM') != 1 or any(k in r['meta_kinds'] for k in ('HOOK_FN', 'HOOK_NAME', 'HOOK_MEM_EXT', 'EXPORT')):
        errors.append('expected exactly one HOOK_MEM and no other hook/export records: %s' % r['meta_kinds'])
    if r['meta_kinds'].get('HEADER') != 1:
        errors.append('expected one HEADER record')
    bound = None
    if len(hooks) == 1:
        h = hooks[0]
        if h['names'] != HOOK_NAMES:
            errors.append('hook names %s' % h['names'])
        if h['resolved'] != 0 or any(h['pmf'][8:]):
            errors.append('PMF slot not a zero-adjustment compact pointer / resolved slot not initially zero')
        va = struct.unpack_from('<Q', h['pmf'], 0)[0]
        try:
            o = p.off(va - p.image_base)
            if raw[o:o + 2] != b'\xff\x25':
                errors.append('PMF does not point to an import thunk')
            else:
                slot = (va - p.image_base) + 6 + struct.unpack_from('<i', raw, o + 2)[0]
                bound = slots.get(slot)
        except ValueError as ex:
            errors.append('PMF target unmapped: %s' % ex)
        r['hook_thunk_bound_import'] = list(bound) if bound else None
        if not bound or bound != ('dusklight.exe', HOOK_IMPORT):
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
