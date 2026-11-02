"""POST05: pruebas NEGATIVAS del auditor PE sobre COPIAS alteradas de la DLL real. Cada alteracion debe hacer que pe_audit NO devuelva PE_AUDIT_PASS.
Demuestra que la auditoria detectaria: cambio de arquitectura, import nuevo, import mutador, export alterado, hook apuntando a otro metodo,
dependencia alterada, archivo truncado y cabecera corrupta. Sin ejecutar nada. Uso: pe_negatives.py <dll> <expected_pe.json> -> JSON; salida != 0 si algun caso pasa."""
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pe_audit  # noqa: E402


def swap(raw, old, new, allow_many=False):
    """Sustituye la primera (o todas) las apariciones de old por new (misma longitud: relleno con NUL si new es mas corto)."""
    assert len(new) <= len(old), (old, new)
    new = new + b'\0' * (len(old) - len(new))
    n = raw.count(old)
    if n == 0:
        raise RuntimeError('pattern not found: %r' % old)
    return raw.replace(old, new) if allow_many else raw.replace(old, new, 1)


def patch_in_modmeta(raw, old, new):
    """Cambia old por new SOLO dentro de la seccion `modmeta` (donde vive el registro HOOK_MEM), no en cadenas de log de .rdata."""
    sec = [x for x in pe_audit.PE(raw).sections if x[0] == 'modmeta']
    assert len(sec) == 1
    _, vs, _, rs, ro = sec[0]
    end = ro + min(vs, rs)
    i = raw.find(old, ro, end)
    if i < 0:
        raise RuntimeError('pattern not found in modmeta: %r' % old)
    assert len(new) == len(old)
    return raw[:i] + new + raw[i + len(old):]


def ext_patch(raw, which):
    p=pe_audit.PE(raw);sec=next(x for x in p.sections if x[0]=='modmeta');n=sec[4];end=n+min(sec[1],sec[3])
    while n+8<=end:
        size,kind,flags=struct.unpack_from('<HBB',raw,n)
        if not size:n+=8;continue
        if kind==7:
            b=bytearray(raw)
            if which=='size':struct.pack_into('<I',b,n+4,16)
            elif which=='null':struct.pack_into('<Q',b,n+8,0)
            elif which=='flags':b[n+3]=1
            elif which=='code':b[p.off(struct.unpack_from('<Q',raw,n+8)[0]-p.image_base)]=0xcc
            return bytes(b)
        n+=size
    raise ValueError('extended hook missing')


def cases(raw):
    pe = struct.unpack_from('<I', raw, 0x3c)[0]
    b = bytearray(raw)
    machine = bytearray(raw)
    struct.pack_into('<H', machine, pe + 4, 0x14c)  # i386 en lugar de AMD64
    notpe = bytearray(raw)
    notpe[0:2] = b'ZM'
    return [
        ('N1 arquitectura i386 en vez de AMD64', bytes(machine)),
        ('N2 import nuevo del juego (fopAcM_SearchByID alterado)', swap(raw, b'?fopAcM_SearchByID@@', b'?fopAcM_SearchByIE@@')),
        ('N3 import mutador del sistema (WriteProcessMemory)', swap(raw, b'GetCurrentProcessId', b'WriteProcessMemory')),
        ('N4 import CRT nuevo (memset -> memmov)', swap(raw, b'memset\0', b'memmov')),
        ('N5 export alterado (mod_update)', swap(raw, b'mod_update\0', b'mod_updatf', allow_many=True)),
        ('N6 el registro HOOK_MEM apunta a otro metodo (nombre en modmeta)', patch_in_modmeta(raw, b'daE_OC_c::damage_check', b'daE_OC_c::damage_chec!')),
        ('N6b hook reenlazado a otro simbolo importado (thunk)', swap(raw, b'?damage_check@daE_OC_c@@', b'?damage_chexk@daE_OC_c@@')),
        ('N7 dependencia alterada (KERNEL32.dll)', swap(raw, b'KERNEL32.dll', b'KERNEL33.dll')),
        ('N8 import del juego no revisado (getStartStageName -> otro)', swap(raw, b'?dComIfGp_getStartStageName@@', b'?dComIfGp_getStartStageNamX@@')),
        ('N9 archivo truncado', bytes(raw[: len(raw) // 2])),
        ('N10 cabecera MZ corrupta', bytes(notpe)),
        ('N15 native Link PMF size corrupted',ext_patch(raw,'size')),
        ('N16 null Link materializer',ext_patch(raw,'null')),
        ('N17 arbitrary materializer code',ext_patch(raw,'code')),
        ('N18 unexpected extended hook flags',ext_patch(raw,'flags')),
        ('N19 extended target name changed',patch_in_modmeta(raw,b'daAlink_c::checkPolyDamage',b'daAlink_c::checkPolyDamagf')),

        # LNK01: el unico hook virtual (prompt de accion inferior) debe seguir siendo EXACTAMENTE dMeterButton_c::draw, por thunk vcall de slot 0
        ('N14 transicion nativa de Link no revisada (procGuardBreakInit -> otro metodo)', swap(raw, b'?procGuardBreakInit@daAlink_c@@', b'?procGuardBreakInix@daAlink_c@@')),
        ('N11 hook virtual renombrado (dMeterButton_c::draw -> otro metodo)', patch_in_modmeta(raw, b'dMeterButton_c::draw', b'dMeterButton_c::drax')),
        ('N12 vtable del hook virtual alterada', patch_in_modmeta(raw, b'??_7dMeterButton_c@@6B@', b'??_7dMeterButtoN_c@@6B@')),
        ('N13 thunk vcall del hook virtual alterado (otro slot / salto directo)', vcall_patch(raw)),
    ]


def vcall_patch(raw):
    """Corrompe el thunk local al que apunta el PMF del hook virtual (jmp [rax] -> jmp [rax+8] o salto directo): ya no es el slot 0."""
    p = pe_audit.PE(raw)
    h = [r for r in p.meta_records() if r['kind'] == 'HOOK_MEM' and r['names'][-1] == 'dMeterButton_c::draw']
    assert len(h) == 1
    va = struct.unpack_from('<Q', h[0]['pmf'], 0)[0]
    o = p.off(va - p.image_base)
    b = bytearray(raw)
    i = bytes(b[o:o + 16]).find(b'\x48\x8b\x01')
    assert i >= 0
    b[o + i + 2] = 0x02  # mov rax,[rdx]: el host ya no lo reconoce como vcall => direccion directa al thunk, no al overrider
    return bytes(b)


def main(dll, expected):
    raw = Path(dll).read_bytes()
    base = pe_audit.audit(Path(dll), Path(expected))
    out = {'baseline': base['status'], 'cases': [], 'passed_unexpectedly': 0}
    if base['status'] != 'PE_AUDIT_PASS':
        print(json.dumps(out | {'error': 'baseline does not pass'}))
        return 2
    with tempfile.TemporaryDirectory() as td:
        for name, data in cases(raw):
            p = Path(td) / 'x.dll'
            p.write_bytes(data)
            try:
                r = pe_audit.audit(p, Path(expected))
                status, err = r['status'], (r.get('errors') or [''])[0][:90]
            except Exception as ex:  # noqa: BLE001  una DLL ilegible tambien es un rechazo
                status, err = 'REJECTED_BY_EXCEPTION', type(ex).__name__
            ok = status != 'PE_AUDIT_PASS'
            out['passed_unexpectedly'] += 0 if ok else 1
            out['cases'].append({'case': name, 'status': status, 'rejected': ok, 'first_error': err})
    print(json.dumps(out, indent=1))
    return 1 if out['passed_unexpectedly'] else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2]))
