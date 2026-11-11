"""Enemy Spawner 0.1.0: audit of the built DLL (pure Python PE reader). Windows AMD64 DLL; the mod exports; the exact reviewed set of game imports;
system/CRT imports from a closed list; nothing that patches code, loads libraries or starts threads; no hook service id in the binary.
Usage: pe_audit.py <dll> <expected_pe.json>   -> JSON on stdout, exit 1 on any error."""
import json, struct, sys

def rd(fmt, b, o):
    return struct.unpack_from(fmt, b, o)

class PE:
    def __init__(self, raw):
        self.raw = raw
        if raw[:2] != b'MZ':
            raise ValueError('not MZ')
        self.pe = rd('<I', raw, 0x3C)[0]
        if raw[self.pe:self.pe + 4] != b'PE\0\0':
            raise ValueError('not PE')
        self.machine, self.nsec = rd('<HH', raw, self.pe + 4)
        self.chars = rd('<H', raw, self.pe + 22)[0]
        opt = self.pe + 24
        self.magic = rd('<H', raw, opt)[0]
        self.dirs = opt + 112
        osz = rd('<H', raw, self.pe + 20)[0]
        self.secs = []
        for i in range(self.nsec):
            s = opt + osz + 40 * i
            name = raw[s:s + 8].rstrip(b'\0').decode('ascii', 'replace')
            vs, va, rs, ro = rd('<IIII', raw, s + 8)
            self.secs.append((name, va, max(vs, rs), ro))

    def off(self, rva):
        for _, va, size, ro in self.secs:
            if va <= rva < va + size:
                return rva - va + ro
        raise ValueError(hex(rva))

    def cstr(self, rva):
        o = self.off(rva)
        return self.raw[o:self.raw.index(b'\0', o)].decode('ascii', 'replace')

    def exports(self):
        rva = rd('<I', self.raw, self.dirs)[0]
        if not rva:
            return []
        o = self.off(rva)
        n_names, _, names = rd('<I', self.raw, o + 24)[0], rd('<I', self.raw, o + 28)[0], rd('<I', self.raw, o + 32)[0]
        return sorted(self.cstr(rd('<I', self.raw, self.off(names) + 4 * i)[0]) for i in range(n_names))

    def imports(self):
        rva = rd('<I', self.raw, self.dirs + 8)[0]
        out = {}
        o = self.off(rva)
        while True:
            ilt, _, _, name, iat = rd('<IIIII', self.raw, o)
            if not name:
                break
            dll = self.cstr(name)
            p = self.off(ilt or iat)
            fns = []
            while True:
                v = rd('<Q', self.raw, p)[0]
                if not v:
                    break
                fns.append('#%d' % (v & 0xFFFF) if v & (1 << 63) else self.cstr((v & 0x7FFFFFFF) + 2))
                p += 8
            out.setdefault(dll, []).extend(fns)
            o += 20
        return {k: sorted(v) for k, v in out.items()}

def audit(dll, expected):
    raw = open(dll, 'rb').read()
    exp = json.load(open(expected))
    p = PE(raw)
    r = {'status': 'PE_AUDIT_FAIL', 'errors': []}
    e = r['errors']
    if p.machine != 0x8664 or p.magic != 0x20B:
        e.append('not a PE32+ AMD64 image')
    if not p.chars & 0x2000:
        e.append('not a DLL')
    r['exports'] = p.exports()
    for x in exp['exports']:
        if x not in r['exports']:
            e.append('missing export ' + x)
    imps = p.imports()
    r['imports'] = imps
    game = imps.get('dusklight.exe', [])
    if sorted(game) != sorted(exp['game_imports']):
        e.append('game imports differ: added %s removed %s' % (sorted(set(game) - set(exp['game_imports'])), sorted(set(exp['game_imports']) - set(game))))
    for dll_name, fns in imps.items():
        if dll_name == 'dusklight.exe':
            continue
        allowed = exp['system_imports'].get(dll_name)
        if allowed is None:
            e.append('unexpected import library ' + dll_name)
            continue
        for f in fns:
            if f not in allowed:
                e.append('unexpected import %s!%s' % (dll_name, f))
    flat = [f for fns in imps.values() for f in fns]
    for bad in exp['forbidden']:
        if any(bad in f for f in flat):
            e.append('forbidden import ' + bad)
    for s in exp['forbidden_strings']:
        if s.encode() in raw:
            e.append('forbidden string in binary: ' + s)
    for s in exp['required_strings']:
        if s.encode() not in raw:
            e.append('required string missing: ' + s)
    r['game_import_count'] = len(game)
    if not e:
        r['status'] = 'PE_AUDIT_PASS'
    return r

if __name__ == '__main__':
    res = audit(sys.argv[1], sys.argv[2])
    print(json.dumps(res, indent=1))
    sys.exit(0 if res['status'] == 'PE_AUDIT_PASS' else 1)
