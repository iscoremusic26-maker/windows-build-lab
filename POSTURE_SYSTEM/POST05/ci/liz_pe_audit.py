"""LIZALFOS 0.12.2 scoped binary audit of the fourteen native writer functions in the compiled DLL (bounded encoding scan, as cp06f1_pe_audit; not a proof of every memory effect).
Every non-stack store displacement of each writer must be in its reviewed set, and the original big_damage is reached only from liz_native_break. Usage: liz_pe_audit.py <dll> <map>"""
from pathlib import Path
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
                res['errors'].append('%s: store at unreviewed displacement %#x' % (name, s['disp']))
    if res['errors']:
        res['status'] = 'LIZ_PE_SCOPE_FAIL'
    print(json.dumps(res, indent=1))
    sys.exit(1 if res['errors'] else 0)
main()
