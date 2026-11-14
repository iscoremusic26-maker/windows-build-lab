"""LIGHT FADES TWICE FIX03 0.12.3: scoped binary audit of the five new native writer functions in the compiled DLL (bounded encoding scan, as
cp06f1_pe_audit / liz_pe_audit; not a proof of every memory effect). Every non-stack store displacement of each writer must be in its reviewed set,
offsets measured in the EXE 2.0.3 (mDoCPd_c::LRlockCheck, dAttention_c::Run/nextAttention/freeAttention, JPARegistPrmEnv, JPARegistPrmAlphaEnv,
JPADrawEmitterCallBackB). movePitch is reached only from pfx_native_pitch. Usage: lft03_pe_audit.py <dll> <map>"""
from pathlib import Path
import json, sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import re, struct
import pe_audit
import cp06f1_pe_audit as h


def import_refs(raw, pe, slots, start_va, code):
    """Imports reached by call/jmp rel32 into an import thunk, or by call/jmp [rip+slot] (tail calls included)."""
    out = set()
    for m in re.finditer(rb'[\xe8\xe9](....)', code, re.S):
        va = start_va + m.start() + 5 + struct.unpack('<i', m.group(1))[0]
        try:
            o = pe.off(va - pe.image_base)
        except ValueError:
            continue
        if raw[o:o + 2] == b'\xff\x25':
            n = slots.get(va + 6 + struct.unpack_from('<i', raw, o + 2)[0] - pe.image_base)
            if n:
                out.add(tuple(n))
    for m in re.finditer(rb'\xff[\x15\x25](....)', code, re.S):
        n = slots.get(start_va + m.start() + 6 + struct.unpack('<i', m.group(1))[0] - pe.image_base)
        if n:
            out.add(tuple(n))
    return out

ALLOWED = {
    # this frame's right stick X/Y/value/angle (0x10..0x1d; a compiler may merge the four zero stores inside that span) and logical L hold / edge
    'lockon_native_pad': set(range(0x10, 0x1d)) | {0x39, 0x3a},
    'lockon_native_attention': {0x451, 0x504, 0x8, 0x456},              # mAttnStatus, mLockOnOffset, mLockTargetID, field_0x32e
    # the emitter's mPrmClr/mEnvClr and its particles' (+0x8c/+0x90 through the particle, +0x9c/+0xa0 through its JPANode: two pointers then mData)
    'pfx_native_colors': {0x14c, 0x150, 0x8c, 0x90, 0x9c, 0xa0},
    'pfx_native_callback': {0x128},                                     # mpEmtrCallBack of a captured emitter (chained, restored at shutdown)
    'pfx_native_pitch': set(),                                          # only the movePitch call of the returned playback
    'pfx_native_work': {0x8},                                           # FIX05: JPAEmitterWorkData::mpRes of the calc in progress
    'pfx_native_child_tev': set(),                                      # FIX05: only the two GXSetTevColor calls of the child pass
    'pfx_native_flash': {0x978},                                        # FIX05: g_env_light.eflight.mLightType (eflight.mState read at 0x974)
}
GX_TEV = ('dusklight.exe', 'GXSetTevColor')
MOVE_PITCH = ('dusklight.exe', '?movePitch@JAISoundParamsMove@@QEAAXMI@Z')


def main():
    raw = Path(sys.argv[1]).read_bytes()
    pe = pe_audit.PE(raw)
    funcs = h.functions(Path(sys.argv[2]).read_text(encoding='utf-8', errors='replace'))
    _, slots = pe.imports()
    res = {'status': 'FIX03_PE_SCOPE_PASS', 'errors': [], 'writers': {}}
    for name, ok in ALLOWED.items():
        try:
            va, code = h.scope(raw, pe, funcs, name)
        except ValueError as ex:
            res['errors'].append(str(ex))
            continue
        st = h.stores(code)
        calls = import_refs(raw, pe, slots, va, code)
        res['writers'][name] = {'bytes': len(code), 'stores': sorted({s['disp'] for s in st}), 'imports': sorted('%s!%s' % c for c in calls)}
        for s in st:
            if s['disp'] not in ok:
                # The scanner matches byte patterns without decoding. `44 0F B6 C6` (movzx r8d, sil: the bool argument; or `45 8A C6`, mov r8b, r14b) followed by a
                # REX `41 8B ..` reads as C6 /0 "mov byte [r9-0x75], imm8": C6 there is the ModRM of opcode 0F B6 / 8A. Only those exact shapes are discounted, with their bytes as evidence.
                raw8 = code[max(0, s['at'] - 3):s['at'] + 4].hex()
                if code[s['at']] == 0xC6 and s['at'] >= 2 and (code[s['at'] - 1] == 0x8A or code[s['at'] - 2:s['at']] == b'\x0f\xb6'):
                    res.setdefault('discounted_modrm_of_load', []).append({'writer': name, 'at': s['at'], 'bytes': raw8})
                    continue
                rip = h.riprel_read_around(code, s['at'])  # LFT 0.13.9: inside the rel32 of a RIP-relative constant READ (layout-dependent bytes)
                if rip:
                    res.setdefault('discounted_inside_riprel_read', []).append({'writer': name, 'at': s['at'], 'bytes': rip})
                    continue
                res['errors'].append('%s: store at unreviewed displacement %#x bytes=%s' % (name, s['disp'], raw8))
        if name == 'pfx_native_pitch' and MOVE_PITCH not in calls:
            res['errors'].append('pfx_native_pitch does not reach movePitch')
        if name == 'pfx_native_child_tev' and GX_TEV not in calls:
            res['errors'].append('pfx_native_child_tev does not reach GXSetTevColor')
    # movePitch only from its writer: no other code references its import thunk.
    others = [n for n in ('lockon_native_pad', 'lockon_native_attention', 'pfx_native_colors', 'pfx_native_callback') if n in res['writers'] and '%s!%s' % MOVE_PITCH in res['writers'][n]['imports']]
    if others:
        res['errors'].append('movePitch reached from %s' % others)
    if res['errors']:
        res['status'] = 'FIX03_PE_SCOPE_FAIL'
    print(json.dumps(res, indent=1))
    sys.exit(1 if res['errors'] else 0)


if __name__ == '__main__':
    main()
