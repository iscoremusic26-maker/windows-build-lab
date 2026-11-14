"""LIGHT FADES TWICE 0.13.8 DARKNUT RESURRECTION (DKR01): the OUTERMOST layer of the source wall.
* Inventory: three new production modules (dkr_core.hpp, dkr_bridge.inc, dkr_adapter.inc) and ONE resource. LFT 0.13.9 replaced the clip by the Fable 5.2
  FIX02 file res/darknut/resurrection_A_agil_natural_FIX02.bck (45024 bytes, SHA-256 f96356fb...1524; FIX01 7476e89e...29a8 is no longer shipped).
* 0.13.9: the DKN01 miniboss layer (ci/dkn_audit.py) sits OUTSIDE this one: its modules count in this inventory and its exact mod.cpp hunks are removed first.
* mod.cpp minus its exact, counted DKR01 hunks (ci/kk_policy.json, entries tagged dkr01) is FIX07 0.13.7 (runtime-experiments 501a26d) byte for byte;
  the older chain (kk_audit and below) then applies to that reconstruction unchanged. No other shared production file changes.
* The adapter writes the native Darknut ONLY inside its seven DKR_NOINLINE writers and its clip binder; never Link; never a deletion, a disappear effect,
  a skipped original or an allocation; the native loader is called once, in the binder; the clip goes to the model only in the rise and unload writers.
* The request's numbers: 300 ticks (10 s), ONE resurrection, the Enemy Spawner parameters 0x000001FF.
Every rule is shown to reject a mutated copy (negatives). Usage: dkr_audit.py [<project POST05 dir>]"""
from pathlib import Path
import hashlib, json, re, shutil, sys, tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import dkn_audit  # noqa: E402  LFT 0.13.9: the DKN01 layer
NEW = {'dkr_core.hpp', 'dkr_bridge.inc', 'dkr_adapter.inc'} | dkn_audit.NEW
RES = {'res/darknut/resurrection_A_agil_natural_FIX02.bck': ('f96356fb6096f1ced7db8e6e3b4d21b39f9c4ea6b7a30de0cd46b1f2a2321524', 45024)}
FIX07_MOD_CPP = '5dbefb8b5ea21db920a003b06303000984594c85e7fa80db8bfc3b39dc450cf0'  # runtime-experiments 501a26d (0.13.7)
WRITERS = ['dkr_native_keep_body', 'dkr_native_disarm', 'dkr_native_hold', 'dkr_native_receipt', 'dkr_native_rise', 'dkr_native_combat', 'dkr_native_unload']
FORBIDDEN = ['HOOK_SKIP_ORIGINAL', 'fopAcM_delete', 'fopAcM_createDisappear', 'JKR_NEW', 'malloc(', 'operator new', 'mProcID =', 'mProcID=', 'changeDemoMode',
             'setActionMode(', 'setBck(', 'procCut', 'procBackJump', 'onCutDownHitFlg', 'dComIfGs_onSwitch', 'replace<', 'VirtualProtect', 'svc_resource', 'mod_ctx']


def code(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    t = re.sub(r'"(?:\\.|[^"\\])*"', '""', t)
    return re.sub(r'//[^\n]*', '', t)


def body(t, sig):
    i = t.index(sig)
    j = t.index('{', i)
    d = 0
    for k in range(j, len(t)):
        d += t[k] == '{'
        d -= t[k] == '}'
        if d == 0:
            return t[i:k + 1]
    raise ValueError(sig)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def audit(project):
    p = Path(project)
    e = []
    for n in NEW:
        if not (p / 'src' / n).is_file():
            e.append('DKR01 module missing: ' + n)
    if e:
        return e
    res = {f.relative_to(p).as_posix() for f in (p / 'res').rglob('*') if f.is_file()} if (p / 'res').is_dir() else set()
    if res != set(RES):
        e.append('DKR01 resource inventory changed: %s' % sorted(res))
    for rel, (h, n) in RES.items():
        f = p / rel
        if not f.is_file() or f.stat().st_size != n or sha(f.read_bytes()) != h:
            e.append('DKR01 clip is not the approved Fable 5.2 file: ' + rel)
    # mod.cpp: the exact, counted DKR01 hunks give back FIX07 0.13.7
    pol = json.loads((p / 'ci' / 'kk_policy.json').read_text(encoding='utf-8'))
    mine = [h for h in pol['inverse_patch'].get('mod.cpp', []) if h.get('dkr01') or h.get('dkn01')]  # 0.13.9: the outer DKN01 hunks first (reverse order)
    t = (p / 'src' / 'mod.cpp').read_text(encoding='utf-8')
    for h in reversed(mine):
        if t.count(h['after']) != h.get('count', 1):
            e.append('DKR01 exact integration changed in mod.cpp')
            break
        t = t.replace(h['after'], h['before'], h.get('count', 1))
    if sha(t.encode('utf-8')) != FIX07_MOD_CPP:
        e.append('mod.cpp changed outside the exact DKR01 hunks (not FIX07 0.13.7 after removal)')
    if any(h.get('dkr01') or h.get('dkn01') for k, v in pol['inverse_patch'].items() if k != 'mod.cpp' for h in v):
        e.append('DKR01 touches a shared file other than mod.cpp')
    m = (p / 'src' / 'mod.cpp').read_text(encoding='utf-8')
    if re.findall(r'DEFINE_HOOK\(&daB_TN_c::(\w+), (\w+)\);', m) != [('executeZakoEnding', 'DkrEnding'), ('_delete', 'DkrDelete')] + dkn_audit.HOOKS:
        e.append('B_TN hook targets changed')
    for need in ('  dkr_install();', '  dkr_shutdown();', '#include "dkr_bridge.inc"\n#include "dkr_adapter.inc"\n', 'IMPORT_OPTIONAL_SERVICE(ResourceService, svc_resource);'):
        if m.count(need) != 1:
            e.append('DKR01 wiring lost: ' + need.strip())
    # adapter: writes only in its writers; nothing forbidden; the loader once; the clip on the model only in two writers
    raw = (p / 'src' / 'dkr_adapter.inc').read_text(encoding='utf-8')
    a = code(raw)
    for w in WRITERS:
        if raw.count('DKR_NOINLINE void ' + w + '(') != 1:
            e.append('DKR01 writer inventory changed: ' + w)
    if len(re.findall(r'^DKR_NOINLINE \w+\*? \w+\(', raw, flags=re.M)) != len(WRITERS) + 1 or raw.count('DKR_NOINLINE bool dkr_bind_clip(') != 1:
        e.append('DKR01 writer inventory is not seven writers + the clip binder')
    outside = raw
    for w in WRITERS + ['dkr_bind_clip']:
        sig = ('DKR_NOINLINE bool ' if w == 'dkr_bind_clip' else 'DKR_NOINLINE void ') + w + '('
        outside = outside.replace(body(outside, sig), '', 1)
    o = code(outside)
    if re.search(r'a->\w+(\[\d+\])?\s*=(?!=)|a->(onDownFlg|offDownFlg|setDownPos|offHeadLockFlg|setSwordAtBit)\(|->setAnm\(|mSphC\.|setResource\(', o):
        e.append('a native Darknut field / flag / clip written outside the DKR01 writers')
    for tok in FORBIDDEN:
        if tok in a:
            e.append('DKR01 forbidden operation in the adapter: ' + tok)
    if a.count('J3DAnmLoaderDataBase::setResource(') != 1 or 'J3DAnmLoaderDataBase::setResource(' not in body(raw, 'DKR_NOINLINE bool dkr_bind_clip('):
        e.append('the native loader is not called exactly once, in the clip binder')
    if a.count('->setAnm(') != 2 or '->setAnm(clip, 0, 0.0f, 1.0f, 0.0f, -1.0f)' not in body(raw, 'DKR_NOINLINE void dkr_native_rise('):
        e.append('the clip is put on the model outside the rise / unload writers, or not once without morph')
    if re.search(r'\bk->\w+\s*=(?!=)|\bk->proc\w*\(|\bk->set\w*\(', a):
        e.append('DKR01 drives Link')
    inst = body(raw, 'void dkr_install()')
    if len(re.findall(r'mods::hook::add_(pre|post)<', inst)) != 3 or 'dz.enabled = dz.hooks;' not in inst or 'uninstall<DkrEnding>' not in inst:
        e.append('install is not the three PRE/POST records, all or nothing')
    pre = body(raw, 'HookAction on_dkr_ending_pre(')
    if 'if (!dkr::eligible(dkr_identity(a)))' not in pre or pre.index('dkr::eligible') > pre.index('dz.t.take(id)'):
        e.append('a slot can be taken before the identity check')
    # core numbers and purity
    core = (p / 'src' / 'dkr_core.hpp').read_text(encoding='utf-8')
    for need in ('kReviveDelayTicks = 300;', 'kMaxRevivals = 1;', 'kTargetParam = 0x000001FFu;', 'kBckBytes = 45024;', 'kBckCrc32Value = 0xF92A19F6u;',
                 'return i.param == kTargetParam && i.type == 1 && i.type2 == 0 && i.switch_no == 0xFF && !i.stay_no2;',
                 'bool consumed() const noexcept { return revivals >= kMaxRevivals; }',
                 'if (phase == Phase::Consumed || consumed()) return phase = Phase::Consumed, Step::NativeDeath;'):
        if need not in core:
            e.append('DKR01 core rule changed: ' + need)
    for inc in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]', core):
        if inc not in ('cstddef', 'cstdint'):
            e.append('the DKR01 core includes a non-standard header: ' + inc)
    br = code((p / 'src' / 'dkr_bridge.inc').read_text(encoding='utf-8'))
    if re.search(r'->\w+\s*=(?!=)', br) or 'setAnm' in br or 'HOOK_' in br:
        e.append('the DKR01 bridge writes or hooks')
    mj = json.loads((p / 'mod.json').read_text(encoding='utf-8'))
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.9':
        e.append('mod id / version')
    return e


NEGATIVES = [
    ('src/dkr_core.hpp', 'kReviveDelayTicks = 300;', 'kReviveDelayTicks = 200;'),
    ('src/dkr_core.hpp', 'kMaxRevivals = 1;', 'kMaxRevivals = 2;'),
    ('src/dkr_core.hpp', '#include <cstdint>\n', '#include <cstdint>\n#include <vector>\n'),
    ('src/dkr_adapter.inc', '  a->mMaterialColor = color;\n', '  a->mMaterialColor = color;\n}\nvoid dkr_extra(daB_TN_c* a) noexcept {\n  a->mMaterialColor = -50.0f;\n'),
    ('src/dkr_adapter.inc', 'DKR_NOINLINE void dkr_native_receipt(daB_TN_c* a) noexcept { a->offDownFlg(); }', 'DKR_NOINLINE void dkr_native_receipt(daB_TN_c* a) noexcept { a->offDownFlg(); fopAcM_delete(a); }'),
    ('src/dkr_adapter.inc', '  if (!dz.enabled || !args) return;\n', '  if (!dz.enabled || !args) return;\n  if (dz.lines > 1000000) dkrb_link()->mProcID = 1;\n'),
    ('src/dkr_adapter.inc', '  dz.enabled = dz.hooks;', '  dz.enabled = true;'),
    ('src/mod.cpp', '  dkr_shutdown();\n', ''),
    ('src/mod.cpp', 'DEFINE_MOD();', 'DEFINE_MOD(); '),
    ('mod.json', '"version": "0.13.9"', '"version": "0.13.8"'),
]


def negatives(src_root):
    out = []
    for rel, old, new in NEGATIVES:
        with tempfile.TemporaryDirectory() as td:
            q = Path(td)
            for d in ('src', 'ci', 'res'):
                shutil.copytree(src_root / d, q / d, ignore=shutil.ignore_patterns('__pycache__'))
            shutil.copy(src_root / 'mod.json', q / 'mod.json')
            f = q / rel
            t = f.read_text(encoding='utf-8')
            assert t.count(old) == 1, 'negative anchor not unique: ' + old
            f.write_text(t.replace(old, new, 1), encoding='utf-8')
            out.append(bool(audit(q)))
    with tempfile.TemporaryDirectory() as td:  # one byte of the clip
        q = Path(td)
        for d in ('src', 'ci', 'res'):
            shutil.copytree(src_root / d, q / d, ignore=shutil.ignore_patterns('__pycache__'))
        shutil.copy(src_root / 'mod.json', q / 'mod.json')
        f = q / 'res/darknut/resurrection_A_agil_natural_FIX02.bck'
        b = bytearray(f.read_bytes())
        b[0x5000] ^= 1
        f.write_bytes(bytes(b))
        out.append(bool(audit(q)))
    return out


if __name__ == '__main__':
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE.parent
    errs = audit(root)
    neg = negatives(root)
    status = 'DKR_AUDIT_PASS' if not errs and all(neg) else 'DKR_AUDIT_FAIL'
    print(json.dumps({'status': status, 'errors': errs, 'negatives': len(neg), 'negatives_rejected': sum(neg), 'new_modules': sorted(NEW), 'resource': RES,
                      'writers': WRITERS, 'base_mod_cpp_fix07_sha256': FIX07_MOD_CPP}, indent=1))
    sys.exit(0 if status == 'DKR_AUDIT_PASS' else 1)
