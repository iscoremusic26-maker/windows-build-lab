"""FIX04: can the host's funchook backend really patch every exported hook target of this mod?
0.12.3 shipped PARRY_FX disabled because def_se_set starts `test rcx,rcx; je short ...`: a rel8 branch inside the 5 bytes funchook relocates ("Could not fix ip-relative
address. The size is not 32."). The MSVC build and every lab were green: only the real host binary shows it. This audit reads that binary (the pinned private input, never
uploaded), follows the export's incremental-link jumps exactly as the host does, and walks the instructions covering the first 5 bytes of each target.
The known bad target is checked too: if the scanner stopped seeing it, the audit fails, so a PASS can never be a blind scanner.
By-name targets (file-local statics, absent from the export table) are outside this audit; for those the real log of 0.12.3 shows no funchook failure.
Usage: hook_prologue_audit.py <dusklight.exe>   (Windows runner, dumpbin in PATH)"""
import json
import re
import struct
import subprocess
import sys

JUMP_WINDOW = 5
KNOWN_UNHOOKABLE = '?def_se_set@@YAXPEAVZ2Creature@@PEAVcCcD_Obj@@IPEAVfopAc_ac_c@@@Z'
TARGETS = [
    '?ProcAtTgHitmark@dCcS@@QEAAX_N0PEAVcCcD_Obj@@1PEAVdCcD_GObjInf@@2PEAVcCcD_Stts@@3PEAVdCcD_GStts@@4PEAUcXyz@@0@Z',
    '?setHitMark@dPa_control_c@@QEAAXGPEAVfopAc_ac_c@@PEBUcXyz@@PEBVcsXyz@@1I@Z',
    '?set@dPa_control_c@@QEAAPEAVJPABaseEmitter@@EGPEBUcXyz@@PEBVdKy_tevstr_c@@PEBVcsXyz@@0EPEAVdPa_levelEcallBack@@CPEBUGXColor@@40M@Z',
    '?setGuardSe@daAlink_c@@QEAAXPEAVdCcD_GObjInf@@@Z',
    '?startCollisionSE@Z2SoundObjBase@@QEAAPEAVZ2SoundHandlePool@@IIPEAV1@@Z',
    '?checkCutReverseAt@daAlink_c@@QEAAHPEAVdCcD_GObjInf@@@Z',
    '?setDamagePoint@daAlink_c@@QEAAHHHHH@Z',
    '?setCutType@daAlink_c@@QEAAXE@Z',
    '?checkDamageAction@daAlink_c@@QEAAHXZ',
    '?checkMoveDoAction@daAlink_c@@QEAAHXZ',
    '?setCollision@daAlink_c@@QEAAXXZ',
    '?setRStatus@daAlink_c@@QEAAXE@Z',
    '?setShieldGuard@daAlink_c@@QEAAXXZ',
    '?setStickData@daAlink_c@@QEAAXXZ',
    '?spActionTrigger@daAlink_c@@QEAAHXZ',
    '?modelDraw@daAlink_c@@QEAAXPEAVJ3DModel@@H@Z',
    '?damage_check@daE_OC_c@@QEAAXXZ',
    '?execute@daE_OC_c@@QEAAHXZ',
    '?executeAttack@daE_OC_c@@QEAAXXZ',
    '?executeDamage@daE_OC_c@@QEAAXXZ',
    '?_draw@dMeter2_c@@QEAAHXZ',
    '?_execute@dMeter2_c@@QEAAHXZ',
    '?_delete@dMeter2_c@@QEAAHXZ',
    # CHILFOS01 0.13.0: the ten daE_KK_c targets
    '?execute@daE_KK_c@@QEAAHXZ', '?_delete@daE_KK_c@@QEAAHXZ', '?damage_check@daE_KK_c@@QEAAXXZ', '?executeAttack@daE_KK_c@@QEAAXXZ',
    '?setBck@daE_KK_c@@QEAAXHEMM@Z', '?setWeaponBck@daE_KK_c@@QEAAXHEMM@Z', '?executeBackWalk@daE_KK_c@@QEAAXXZ', '?executeSpearThrow@daE_KK_c@@QEAAXXZ',
    '?executeDamage@daE_KK_c@@QEAAXXZ', '?nextActionCheck@daE_KK_c@@QEAAXXZ',
]


class Pe:
    def __init__(self, raw):
        self.raw = raw
        pe = struct.unpack_from('<I', raw, 0x3C)[0]
        n, opt = struct.unpack_from('<H', raw, pe + 6)[0], struct.unpack_from('<H', raw, pe + 20)[0]
        self.base = struct.unpack_from('<Q', raw, pe + 24 + 24)[0]
        self.export_rva = struct.unpack_from('<I', raw, pe + 24 + 112)[0]
        self.secs = []
        for i in range(n):
            o = pe + 24 + opt + 40 * i
            vs, va, rs, ro = struct.unpack_from('<IIII', raw, o + 8)
            self.secs.append((va, max(vs, rs), ro))

    def off(self, rva):
        for va, size, ro in self.secs:
            if va <= rva < va + size:
                return rva - va + ro
        raise ValueError(hex(rva))

    def exports(self):
        o = self.off(self.export_rva)
        n_names, f_rva, n_rva, o_rva = struct.unpack_from('<IIII', self.raw, o + 24)[0], *struct.unpack_from('<III', self.raw, o + 28)
        out = {}
        fo, no, oo = self.off(f_rva), self.off(n_rva), self.off(o_rva)
        for i in range(n_names):
            p = self.off(struct.unpack_from('<I', self.raw, no + 4 * i)[0])
            name = self.raw[p:self.raw.index(b'\0', p)].decode('ascii', 'replace')
            out[name] = struct.unpack_from('<I', self.raw, fo + 4 * struct.unpack_from('<H', self.raw, oo + 2 * i)[0])[0]
        return out

    def follow(self, rva):  # the host's resolve_import_thunk: E9 chains of incremental linking
        for _ in range(8):
            o = self.off(rva)
            if self.raw[o] != 0xE9:
                break
            rva = rva + 5 + struct.unpack_from('<i', self.raw, o + 1)[0]
        return rva


LINE = re.compile(r'^\s*([0-9A-Fa-f]{8,16}):\s+((?:[0-9A-Fa-f]{2}\s)+)\s*(\S.*)?$')


def parse_dumpbin(text):
    """-> [(va, first_byte, mnemonic_and_operands)], in order."""
    out = []
    for line in text.splitlines():
        m = LINE.match(line)
        if m:
            out.append((int(m.group(1), 16), int(m.group(2).split()[0], 16), (m.group(3) or '').strip()))
    return out


def dumpbin_disasm(exe, va):
    cp = subprocess.run(['dumpbin', '/nologo', '/disasm', '/range:0x%X,0x%X' % (va, va + 40), exe], capture_output=True, text=True, timeout=120)
    if cp.returncode:
        raise RuntimeError('dumpbin failed: ' + cp.stderr[-300:])
    return parse_dumpbin(cp.stdout)


def judge(insns, va):
    """Walks the window. Returns (covered_bytes, text, rel8_branches_leaving_the_window)."""
    bad, texts = [], []
    for i, (a, first, text) in enumerate(insns[:-1]):
        size = insns[i + 1][0] - a
        texts.append(text)
        if size == 2 and (0x70 <= first <= 0x7F or first in (0xEB, 0xE0, 0xE1, 0xE2, 0xE3)):
            bad.append(text)
        if insns[i + 1][0] - va >= JUMP_WINDOW:
            return insns[i + 1][0] - va, texts, bad
    raise RuntimeError('window not covered at 0x%X: %d instructions' % (va, len(insns)))


def audit(exe, disasm=dumpbin_disasm):
    raw = open(exe, 'rb').read()
    p = Pe(raw)
    ex = p.exports()
    res = {'status': 'HOOK_PROLOGUE_FAIL', 'window': JUMP_WINDOW, 'targets': {}, 'errors': [], 'scope': 'exported hook targets only; execution of the hooks is only proven by the human log'}
    for name in TARGETS + [KNOWN_UNHOOKABLE]:
        if name not in ex:
            res['errors'].append('not exported: ' + name)
            continue
        rva = p.follow(ex[name])
        size, texts, bad = judge(disasm(exe, p.base + rva), p.base + rva)
        res['targets'][name] = {'rva': '0x%x' % rva, 'window_bytes': size, 'instructions': texts, 'rel8_in_window': bad}
        if name == KNOWN_UNHOOKABLE:
            if not bad:
                res['errors'].append('blind scanner: the known unhookable target was not flagged')
        elif bad:
            res['errors'].append('rel8 branch in the first %d bytes of %s: %s' % (JUMP_WINDOW, name, bad))
    if not res['errors']:
        res['status'] = 'HOOK_PROLOGUE_PASS'
    return res


if __name__ == '__main__':
    r = audit(sys.argv[1])
    print(json.dumps(r, indent=1))
    sys.exit(0 if r['status'] == 'HOOK_PROLOGUE_PASS' else 1)
