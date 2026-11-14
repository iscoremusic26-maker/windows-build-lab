"""LIGHT FADES TWICE 0.13.9 DARKNUT MINIBOSS (DKN01): the OUTERMOST layer of the source wall.
* Inventory: three new production modules (dkn_core.hpp, dkn_bridge.inc, dkn_adapter.inc); no new resource (the FIX02 clip belongs to DKR01).
* mod.cpp minus its exact, counted DKN01 hunks (ci/kk_policy.json, entries tagged dkn01) is 0.13.8 DKR01 (runtime-experiments 0079b6f) byte for byte;
  the older chain (dkr_audit and below) then removes its own hunks from that reconstruction. No other shared production file changes.
* The adapter writes the native Darknut ONLY inside its thirteen DKN_NOINLINE writers; never Link (the bridge reaches Link's posture only through the
  shared LNK API); never a deletion, a skipped original, an allocation, the native accumulator field_0x6fc, l_HIO, a switch or a down flag.
* The eligibility filter is DKR01's (dkr::eligible) and runs before any slot is taken; ineligible actors are only remembered by id to log them once.
* The approved numbers are present in the pure core; the eight combos' peaks are the ones measured from the user's recordings (tests/data/dkn_audio_peaks.txt).
Every rule is shown to reject a mutated copy (negatives). Usage: dkn_audit.py [<project POST05 dir>]"""
from pathlib import Path
import hashlib, json, re, shutil, sys, tempfile, ast

HERE = Path(__file__).resolve().parent
NEW = {'dkn_core.hpp', 'dkn_bridge.inc', 'dkn_adapter.inc'}
HOOKS = [('action', 'DknAction'), ('damage_check', 'DknDamageCheck'), ('setDamage', 'DknSetDamage'), ('executeAttackH', 'DknAttackH'), ('executeAttackL', 'DknAttackL'),
         ('executeAttackShieldH', 'DknShieldH'), ('executeAttackShieldL', 'DknShieldL')]
DKR01_MOD_CPP = '0fb88a1be182c0a2beee84c3eca6f1108ac25ae94f9a65ed183eb747123f03de'  # runtime-experiments 0079b6f (0.13.8 DKR01)
WRITERS = ['dkn_native_threshold', 'dkn_native_action', 'dkn_native_armor', 'dkn_native_clip', 'dkn_native_wait_clip', 'dkn_native_rate', 'dkn_native_at',
           'dkn_native_spl', 'dkn_native_stagger_hold', 'dkn_native_guard', 'dkn_native_attack_flags', 'dkn_native_motion', 'dkn_native_timer1']
FORBIDDEN = ['HOOK_SKIP_ORIGINAL', 'fopAcM_delete', 'fopAcM_createDisappear', 'JKR_NEW', 'malloc(', 'operator new', 'mProcID =', 'mProcID=', 'procCut', 'procGuardBreak',
             'onDownFlg', 'offDownFlg', 'setDownPos', 'dComIfGs_onSwitch', 'l_HIO', 'replace<', 'VirtualProtect', 'field_0x6fc =', 'field_0x6fc +=', 'field_0x6fc -=',
             'executeZakoChangeDemo(', 'executeChangeDemo(', 'executeEnding(', 'setActionMode(', 'mMaterialColor =', 'health =', 'svc_resource', 'mod_ctx']
CORE_NEEDS = ['kHp1Max = 420;', 'kRes2Max = 480;', 'kPosture1Max = 120, kPosture2Max = 130;', 'kRegenDelayTicks = 60;', 'kRegen1[5] = {10, 7, 4, 2, 1};',
              'kRegen2[5] = {12, 8, 5, 2, 1};', 'kBreakTicks = 90;', 'kBreakHits = 3;', 'kLinkMax = 100;', 'kGainSwordHp1 = 4, kGainBlocked1 = 2, kGainIron1 = 25;',
              'kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;', 'kPassiveShieldTicks = 38;', 'kPassiveIdleTicks = 75;']
PROVISIONAL = ['kComboPct', 'kComboCooldownTicks', 'kPassiveCooldownTicks', 'kGuardRecoverTicks']


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


def audio_peaks(p):
    """envmax rows of tests/data/dkn_audio_peaks.txt, in milliseconds, keyed by the file tag."""
    rows, tag = {}, None
    for line in (p / 'tests' / 'data' / 'dkn_audio_peaks.txt').read_text(encoding='utf-8').splitlines():
        if line.startswith('FASE_'):
            tag = line.split()[0]
        elif tag and line.strip().startswith('envmax'):
            rows[tag] = [int(round(x * 1000)) for x in ast.literal_eval(line.split(':', 1)[1].strip())]
    return rows


AUDIO_OF = {'F1-C1': 'FASE_1__Darknut_combo_1', 'F1-C2': 'FASE_1__Combo_2', 'F1-C3': 'FASE_1__Darknut_combo_3', 'F1-C4': 'FASE_1__Combo_4',
            'F2-C1': 'FASE_2__Combo1', 'F2-C2': 'FASE_2__Comb_2', 'F2-C3': 'FASE_2__Combo_fase', 'F2-C4': 'FASE_2__Comb_4'}


def audit(project):
    p = Path(project)
    e = []
    for n in NEW:
        if not (p / 'src' / n).is_file():
            e.append('DKN01 module missing: ' + n)
    if e:
        return e
    # mod.cpp: the exact, counted DKN01 hunks give back 0.13.8 DKR01
    pol = json.loads((p / 'ci' / 'kk_policy.json').read_text(encoding='utf-8'))
    mine = [h for h in pol['inverse_patch'].get('mod.cpp', []) if h.get('dkn01')]
    if len(mine) != 7:
        e.append('DKN01 hunk inventory changed (%d)' % len(mine))
    t = (p / 'src' / 'mod.cpp').read_text(encoding='utf-8')
    for h in reversed(mine):
        if t.count(h['after']) != h.get('count', 1):
            e.append('DKN01 exact integration changed in mod.cpp')
            break
        t = t.replace(h['after'], h['before'], h.get('count', 1))
    if sha(t.encode('utf-8')) != DKR01_MOD_CPP:
        e.append('mod.cpp changed outside the exact DKN01 hunks (not 0.13.8 DKR01 after removal)')
    if any(h.get('dkn01') for k, v in pol['inverse_patch'].items() if k != 'mod.cpp' for h in v):
        e.append('DKN01 touches a shared file other than mod.cpp')
    m = (p / 'src' / 'mod.cpp').read_text(encoding='utf-8')
    if re.findall(r'DEFINE_HOOK\(&daB_TN_c::(\w+), (\w+)\);', m) != [('executeZakoEnding', 'DkrEnding'), ('_delete', 'DkrDelete')] + HOOKS:
        e.append('B_TN hook targets changed')
    for need in ('  dkn_install();', '  dkn_shutdown();', '  dkn_hud_samples();', '#include "dkn_bridge.inc"\n#include "dkn_adapter.inc"\n', '#include "dkn_core.hpp"\n'):
        if m.count(need) != 1:
            e.append('DKN01 wiring lost: ' + need.strip())
    if m.index('dkr_install();') > m.index('dkn_install();'):
        e.append('DKN01 installs before DKR01 (it needs the DKR01 state)')
    # adapter: writes only in its writers; nothing forbidden; Link never driven
    raw = (p / 'src' / 'dkn_adapter.inc').read_text(encoding='utf-8')
    a = code(raw)
    for w in WRITERS:
        if len(re.findall(r'^DKN_NOINLINE void ' + w + r'\(', raw, flags=re.M)) != 1:
            e.append('DKN01 writer inventory changed: ' + w)
    if len(re.findall(r'^DKN_NOINLINE ', raw, flags=re.M)) != len(WRITERS):
        e.append('DKN01 writer inventory is not exactly the thirteen writers')
    outside = raw
    for w in WRITERS:
        outside = outside.replace(body(outside, 'DKN_NOINLINE void ' + w + '('), '', 1)
    o = code(outside)
    if re.search(r'\ba->[\w.\[\]]+\s*(=|\+=|-=|\|=|&=)(?!=)|\ba->(offHeadLockFlg|setBck|setBreakPart|setBreakHeadPart|setSwordAtBit|setSwordAtBreak)\(|->mFrameCtrl|'
                 r'OnAtSetBit|OffAtSetBit|SetAtSpl|->setFrame\(|->setAnm\(', o):
        e.append('a native Darknut field / call written outside the DKN01 writers')
    for tok in FORBIDDEN:
        if tok in a:
            e.append('DKN01 forbidden operation in the adapter: ' + tok)
    if re.search(r'\bk->\w+\s*=(?!=)|\bk->proc\w*\(|\bk->set\w*\(|\blink->\w+\s*=(?!=)', a):
        e.append('DKN01 drives Link')
    adm = body(raw, 'DknSlot* dkn_admit(daB_TN_c* a)')
    if 'if (!dkr::eligible(dkr_identity(a))) {' not in adm or adm.index('dkr::eligible') > adm.index('s.on = true'):
        e.append('a slot can be taken before the DKR01 identity check')
    inst = body(raw, 'void dkn_install()')
    if len(re.findall(r'mods::hook::add_(pre|post)<', inst)) != 14 or 'dn.enabled = dn.hooks && dknb_dkr_enabled();' not in inst or 'uninstall<DknAction>' not in inst:
        e.append('install is not the fourteen PRE/POST records, all or nothing, only with DKR01')
    sd = body(raw, 'void on_dkn_setdamage_post(')
    if sd.index('kActChangeDemo, 0);') > sd.index('dkn_posture_gain(') or 'dkn_native_action(a, s.sd_act1, s.sd_act2);' not in sd:
        e.append('setDamage POST lost its order: transition first, then posture, then the ordinary stagger taken back')
    # bridge: no writes, no hooks
    br = code((p / 'src' / 'dkn_bridge.inc').read_text(encoding='utf-8'))
    if re.search(r'\b(a|act|k|link)->\w+\s*=(?!=)', br) or 'HOOK_' in br or 'setAnm' in br:
        e.append('the DKN01 bridge writes an actor or hooks')
    # core: numbers, provisional marks, purity, the recorded peaks
    core = (p / 'src' / 'dkn_core.hpp').read_text(encoding='utf-8')
    for need in CORE_NEEDS:
        if need not in core:
            e.append('DKN01 approved number changed: ' + need)
    for name in PROVISIONAL:
        line = next((l for l in core.splitlines() if ' ' + name + ' = ' in l), '')
        if '[PROVISIONAL]' not in line:
            e.append('DKN01 provisional value not marked as such: ' + name)
    for inc in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]', core):
        if inc not in ('cstddef', 'cstdint'):
            e.append('the DKN01 core includes a non-standard header: ' + inc)
    peaks = audio_peaks(p)
    for name, tag in AUDIO_OF.items():
        mm = re.search(r'\{"' + re.escape(name) + r'", (?:true|false), \d+, \{[^}]*\}, \d+, \{([0-9, ]+)\}', core)
        rec = next((v for k, v in peaks.items() if k.startswith(tag)), None)
        if not mm or rec is None or [int(x) for x in mm.group(1).split(',')] != rec:
            e.append('combo %s peaks differ from the recording measurement' % name)
    mj = json.loads((p / 'mod.json').read_text(encoding='utf-8'))
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.9':
        e.append('mod id / version')
    return e


NEGATIVES = [
    ('src/dkn_core.hpp', 'kHp1Max = 420;', 'kHp1Max = 400;'),
    ('src/dkn_core.hpp', '#include <cstdint>\n', '#include <cstdint>\n#include <vector>\n'),
    ('src/dkn_core.hpp', 'inline constexpr int kComboPct = 40;                  // [PROVISIONAL]', 'inline constexpr int kComboPct = 40;                  //'),
    ('src/dkn_core.hpp', '{675, 1045, 1905, 2300, 3195}', '{680, 1050, 1900, 2300, 3190}'),
    ('src/dkn_adapter.inc', 'DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept { a->mTimer1 = t; }',
     'DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept { a->mTimer1 = t; }\nvoid dkn_extra(daB_TN_c* a) noexcept { a->mTimer3 = 0; }'),
    ('src/dkn_adapter.inc', '  dkn_at_authority(s, a);\n}', '  a->field_0x6fc = 0;\n  dkn_at_authority(s, a);\n}'),
    ('src/dkn_adapter.inc', '  if (!dkr::eligible(dkr_identity(a))) {', '  if (false) {'),
    ('src/dkn_adapter.inc', '  dn.enabled = dn.hooks && dknb_dkr_enabled();', '  dn.enabled = true;'),
    ('src/dkn_adapter.inc', '  if (daAlink_c* link = dknb_link()) dknb_direct_capture(link);', '  if (daAlink_c* link = dknb_link()) link->mProcID = 3;'),
    ('src/mod.cpp', '  dkn_shutdown();\n', ''),
    ('src/mod.cpp', 'DEFINE_MOD();', 'DEFINE_MOD(); '),
    ('mod.json', '"version": "0.13.9"', '"version": "0.13.8"'),
]


def negatives(src_root):
    out = []
    for rel, old, new in NEGATIVES:
        with tempfile.TemporaryDirectory() as td:
            q = Path(td)
            for d in ('src', 'ci'):
                shutil.copytree(src_root / d, q / d, ignore=shutil.ignore_patterns('__pycache__'))
            (q / 'tests' / 'data').mkdir(parents=True)
            shutil.copy(src_root / 'tests' / 'data' / 'dkn_audio_peaks.txt', q / 'tests' / 'data' / 'dkn_audio_peaks.txt')
            shutil.copy(src_root / 'mod.json', q / 'mod.json')
            f = q / rel
            t = f.read_text(encoding='utf-8')
            assert t.count(old) == 1, 'negative anchor not unique: ' + old
            f.write_text(t.replace(old, new, 1), encoding='utf-8')
            out.append(bool(audit(q)))
    return out


if __name__ == '__main__':
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE.parent
    errs = audit(root)
    neg = negatives(root)
    status = 'DKN_AUDIT_PASS' if not errs and all(neg) else 'DKN_AUDIT_FAIL'
    print(json.dumps({'status': status, 'errors': errs, 'negatives': len(neg), 'negatives_rejected': sum(neg), 'new_modules': sorted(NEW), 'writers': WRITERS,
                      'hooks': ['daB_TN_c::' + h for h, _ in HOOKS], 'base_mod_cpp_dkr01_sha256': DKR01_MOD_CPP}, indent=1))
    sys.exit(0 if status == 'DKN_AUDIT_PASS' else 1)
