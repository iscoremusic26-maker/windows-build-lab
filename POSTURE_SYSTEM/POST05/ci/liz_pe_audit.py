"""LIZALFOS 0.12.2 scoped binary audit of the fourteen native writer functions in the compiled DLL (bounded encoding scan, as cp06f1_pe_audit; not a proof of every memory effect).
Every non-stack store displacement of each writer must be in its reviewed set, and the original big_damage is reached only from liz_native_break. Usage: liz_pe_audit.py <dll> <map>"""
from pathlib import Path
import re, struct
import json, sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import pe_audit
import cp06f1_pe_audit as h
ALLOWED = {
 'liz_native_stagger': {0x8e2, 0x6c8, 0x5c2, 0x960, 0x964},   # action, mode, current.angle.y, field_0x74c, field_0x750 (damage_check's mHitType 16 branch in the EXE)
 'liz_native_hp_bonus': {0x654, 0x656},
 'liz_native_learn': {0x958},
 'liz_native_hold': {0x8f4, 0x69a, 0x900},                           # timer[0], fopEn mFlags (HeadLock bit, native offHeadLockFlg)
 'liz_native_pose_frame': {0x2c},                             # J3DFrameCtrl mFrame of the actor's morf; the call is the engine's modelCalc
 'liz_native_at_place': set(range(0x1318, 0x1318 + 0x1b8)),   # the actor's own at_sph (centre, At vec)
 'liz_native_bash_se': set(),                                 # the native SE call only
 'liz_native_link_feedback': {0x44f8},                        # mNormalSpeed (procGuardSlipInit `movss %xmm0,0x44f8(%rbx)` in the EXE), plus the two native calls
 'liz_native_fields': {0x8e2, 0x6c8, 0x8fc, 0x8f8, 0x5b4, 0x5b8, 0x5bc, 0x5c0, 0x5c2, 0x5c4},
 'liz_native_anim': {0x2c, 0x28},   # J3DFrameCtrl fields of the actor's morf: frame, rate
 'liz_native_hp': {0x654, 0x656},
 'liz_native_guard': {0x91c, 0x920, 0x8f4, 0x8fc},
 'liz_native_break': {0x16a2, 0x8fc, 0x620},
 'liz_native_tg_clear': set(range(0, 0x1800)),   # one sphere's cCcD_ObjHitInf fields through the sphere base register
}
def main():
    dll = Path(sys.argv[1]).read_bytes()
    pe = pe_audit.PE(dll)
    funcs = h.functions(Path(sys.argv[2]).read_text(encoding='utf-8', errors='replace'))
    _, slots = pe.imports()
    res = {'status': 'LIZ_PE_SCOPE_PASS', 'errors': [], 'writers': {}}
    for name, ok in ALLOWED.items():
        try:
            va, code = h.scope(dll, pe, funcs, name)
        except ValueError as ex:
            res['errors'].append(str(ex))
            continue
        st = h.stores(code)
        res['writers'][name] = {'bytes': len(code), 'stores': sorted({s['disp'] for s in st})}
        for s in st:
            if s['disp'] not in ok:
                # FIX05: the scanner matches bytes without decoding; the 4 displacement bytes of a call through an import slot can spell a store. Only that exact shape is discounted, with its bytes as evidence.
                call = next((m for m in re.finditer(rb'\xff[\x15\x25](....)', code, re.S) if m.start() < s['at'] < m.end() and va + m.end() + struct.unpack('<i', m.group(1))[0] - pe.image_base in slots), None)
                # The same with the MSVC stack-cookie epilogue: `48 33 CC` (xor rcx,rsp) then `E8 rel32` (__security_check_cookie).
                call = call or next((m for m in re.finditer(rb'\x48\x33\xcc\xe8....', code, re.S) if m.start() + 3 < s['at'] < m.end()), None)
                if call:
                    res.setdefault('discounted_inside_import_call', []).append({'writer': name, 'at': s['at'], 'bytes': code[call.start():call.end()].hex()})
                    continue
                rip = h.riprel_read_around(code, s['at'])  # LFT 0.13.9: inside the rel32 of a RIP-relative constant READ (layout-dependent bytes)
                if rip:
                    res.setdefault('discounted_inside_riprel_read', []).append({'writer': name, 'at': s['at'], 'bytes': rip})
                    continue
                res['errors'].append('%s: store at unreviewed displacement %#x bytes=%s' % (name, s['disp'], code[max(0, s['at'] - 4):s['at'] + 8].hex()))
    if res['errors']:
        res['status'] = 'LIZ_PE_SCOPE_FAIL'
    print(json.dumps(res, indent=1))
    sys.exit(1 if res['errors'] else 0)
main()
