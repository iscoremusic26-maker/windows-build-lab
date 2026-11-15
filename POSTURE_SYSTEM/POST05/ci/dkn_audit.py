"""LIGHT FADES TWICE 0.13.9 DARKNUT MINIBOSS (DKN01) + 0.13.10 FIX01 + 0.13.11 FIX02 + 0.13.12 FIX03: the OUTERMOST layer of the source wall.
* 0.13.12 FIX03: twenty writers (+ dkn_native_guard_enter_h, the phase-1 shield guard at ACT_GUARDH 1); the rebound verdict covers every phase (an ordinary
  block never rebounds, a real parry gives Link +20/+25 exactly once); F2-SINGLE-B is not an ordinary single; the second stagger is TNB_DAMAGE_R; the
  action POST tells a suppressed native ENTRY from an executed unauthorized attack.
* 0.13.10 FIX01: same three modules, mod.cpp unchanged (the new callbacks live on EXISTING hook targets: Link's checkCutAction / checkCutReverseAt);
  sixteen writers (+ headlock, exec_fall, neutral); Link is driven only through the Chilfos writer kk_native_mortal_draw (the native Mortal Draw).
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
           'dkn_native_spl', 'dkn_native_stagger_hold', 'dkn_native_guard', 'dkn_native_attack_flags', 'dkn_native_motion', 'dkn_native_timer1',
           'dkn_native_headlock', 'dkn_native_exec_fall', 'dkn_native_neutral', 'dkn_native_guard_enter', 'dkn_native_walk', 'dkn_link_slip_speed',
           'dkn_native_guard_enter_h']
FORBIDDEN = ['HOOK_SKIP_ORIGINAL', 'fopAcM_delete', 'fopAcM_createDisappear', 'JKR_NEW', 'malloc(', 'operator new', 'mProcID =', 'mProcID=', 'procCut', 'procGuardBreak',
             'onDownFlg', 'offDownFlg', 'setDownPos', 'dComIfGs_onSwitch', 'l_HIO', 'replace<', 'VirtualProtect', 'field_0x6fc =', 'field_0x6fc +=', 'field_0x6fc -=',
             'executeZakoChangeDemo(', 'executeChangeDemo(', 'executeEnding(', 'setActionMode(', 'mMaterialColor =', 'health =', 'svc_resource', 'mod_ctx']
CORE_NEEDS = ['kHp1Max = 420;', 'kRes2Max = 480;', 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;', 'kRegenDelayTicks = 60;', 'kRegen1[5] = {10, 7, 4, 2, 1};',
              'kRegen2[5] = {12, 8, 5, 2, 1};', 'kBreakTicks = 90;', 'kDownTicks = 180;', 'kLinkMax = 100;', 'kComboPick[4] = {25, 25, 25, 25};',
              'kParryStreakPct[4] = {10, 25, 45, 60};', 'kLinkEnemyParry1 = 20, kLinkEnemyParry2 = 25;', 'kF2ComboScale = 1.25f;', 'kF2SingleScale = 1.20f;', 'kKickScale = 1.12f;', 'kGainSwordHp1 = 4, kGainBlocked1 = 2, kGainIron1 = 25;',
              'kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;', 'kPassiveShieldTicks = 38;', 'kPassiveIdleTicks = 75;']
PROVISIONAL = ['kSinglePct', 'kGap1Min', 'kGap2Min', 'kReentryTicks', 'kGuardLeaveTicks', 'kEngage1', 'kPassiveCooldownTicks', 'kPressureWindow', 'kGuardPctBase',
               'kStreakGapTicks', 'kParryPrepTicks', 'kParryWindowTicks', 'kParryCooldownTicks', 'kFinisherExtMax', 'kHelmWindowTicks', 'kSlipChainTicks']


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
        e.append('DKN01 writer inventory is not exactly the twenty writers')
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
    if re.search(r'\bk->\w+\s*=(?!=)|\bk->proc\w*\(|\bk->set\w*\(|\blink->\w+\s*=(?!=)', o):
        e.append('DKN01 drives Link')
    # 0.13.11 FIX02: the ONE Link write: a guard slip speed capped, never raised, only in its writer
    if 'if (k->mNormalSpeed > v) k->mNormalSpeed = v;' not in body(raw, 'DKN_NOINLINE void dkn_link_slip_speed(') or len(re.findall(r'\bk->\w+\s*=(?!=)', a)) != 1:
        e.append('the Link slip writer is not exactly one capped write')
    # 0.13.11 FIX02: the 0.13.10 crash class: ACT_GUARDL 0 (native getCutType on this pass' collider) is never written by the module
    if re.search(r'kActGuardL\s*,\s*0\s*\)', a) or 'dkn::native::needs_hit_context(act, mode)' not in body(raw, 'DKN_NOINLINE void dkn_native_action(') \
            or 'a->mActionMode2 = 1;' not in body(raw, 'DKN_NOINLINE void dkn_native_guard_enter(') \
            or 'a->mActionMode2 = 1;' not in body(raw, 'DKN_NOINLINE void dkn_native_guard_enter_h('):
        e.append('ACT_GUARDL 0 can be written without a hit (the 0.13.10 crash) or the writer invariant is gone')
    if raw.count('dkn_native_action(a, s.pre_act1, s.pre_act2)') or raw.count('dkn_native_action(a, s.sd_act1, s.sd_act2)'):
        e.append('a snapshot is restored without the hit-context invariant (dkn_restore)')
    adm = body(raw, 'DknSlot* dkn_admit(daB_TN_c* a)')
    if 'if (!dkr::eligible(dkr_identity(a))) {' not in adm or adm.index('dkr::eligible') > adm.index('s.on = true'):
        e.append('a slot can be taken before the DKR01 identity check')
    inst = body(raw, 'void dkn_install()')
    if len(re.findall(r'mods::hook::add_(pre|post)<', inst)) != 16 or 'dn.enabled = dn.hooks && dknb_dkr_enabled();' not in inst or 'uninstall<DknAction>' not in inst:
        e.append('install is not the sixteen PRE/POST records, all or nothing, only with DKR01')
    if a.count('kk_native_mortal_draw(') != 1 or 'kk_native_mortal_draw(' not in body(raw, 'void on_dkn_cut_action_post('):
        e.append('Link is driven outside the ONE native Mortal Draw request')
    md = body(raw, 'void on_dkn_cut_action_post(')
    if 's.brk.st != dkn::Bk::StaggerA' not in md or '!dkn::phase2(s.ph)' not in md:
        e.append('the Mortal Draw request is not limited to the first stagger of phases 2-3')
    rv = body(raw, 'void on_dkn_cut_reverse_post(')
    if 'const int rb = s.def.rebound(now);' not in rv or '} else if (rb == 0) {\n      *r = FALSE;' not in rv or 'phase2(s.ph)) continue' in rv \
            or rv.count('dknb_link_gain(') != 1 or rv.index('dknb_link_gain(') < rv.index('if (rb == 1) {'):
        e.append('the rebound verdict is not: armed window = REAL parry (Link +20/+25 once), otherwise no rebound, in EVERY phase (FIX03)')
    core0 = (p / 'src' / 'dkn_core.hpp').read_text(encoding='utf-8')
    if 'd.idx = v.phase2 ? kIdxF2Thrust :' not in core0 or 'rng.below(2) : (v.dist' in core0:
        e.append('FIX03: an ordinary phase-2 single can be the full double (F2-SINGLE-B)')
    if 'inline constexpr int kHelmClip = native::kBckTnbDamageR;' not in core0 or 'dkn_native_clip(a, kHelmClip, kHelmMorph, kHelmStart, 1.0f);' not in raw:
        e.append('FIX03: the second stagger is not TNB_DAMAGE_R (the TNB_DOWN hold reads as a guard)')
    po = body(raw, 'void on_dkn_action_post(')
    if 'const bool entry = native::native_attack_entry(a->mActionMode1, a->mActionMode2);' not in po or 'executed=1' not in raw or 'NATIVE_ENTRY_SUPPRESSED' not in po:
        e.append('FIX03: the action POST does not tell a suppressed native entry from an executed unauthorized attack')
    dp = body(raw, 'void on_dkn_damage_post(')
    if 'const Sa v = sa_verdict(' not in dp or 'dkn_restore(s, a, s.pre_act1, s.pre_act2);  // the native ACT_GUARDL/GUARDH the Shield Attack forced is taken back' not in dp:
        e.append('the Shield Attack authority (take the forced guard back) is lost')
    ap = body(raw, 'HookAction on_dkn_action_pre(')
    if 'native::native_attack_entry(act0, mode0)' not in ap or ap.index('native::native_attack_entry(act0, mode0)') > ap.index('s.ini.decide(iv)'):
        e.append('native attack entries are not suppressed before the own initiative decides')
    if 'if (dkr_exec_request(a)) {' not in body(raw, 'void on_dkn_setdamage_post(') or raw.count('dkn_native_exec_fall(a);') != 1:
        e.append('an execution fall without the DKR01 acceptance')
    sd = body(raw, 'void on_dkn_setdamage_post(')
    if sd.index('kActChangeDemo, 0);') > sd.index('dkn_posture_gain(') or 'dkn_restore(s, a, s.sd_act1, s.sd_act2);' not in sd:
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
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.12':
        e.append('mod id / version')
    return e


NEGATIVES = [
    ('src/dkn_core.hpp', 'kHp1Max = 420;', 'kHp1Max = 400;'),
    ('src/dkn_core.hpp', 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 120, kPosture2Max = 130, kPosture3Max = 130;'),
    ('src/dkn_adapter.inc', '      kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);'.replace('      ', '  '), '  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);\n  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);'),
    ('src/dkn_adapter.inc', '    } else if (rb == 0) {\n      *r = FALSE;', '    } else if (rb == 0) {\n      (void)r;'),
    ('src/dkn_adapter.inc', '    if (!s.on || !dkn_combat(s)) continue;\n    fopAc_ac_c* p = nullptr;\n    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;\n    auto* a = static_cast<daB_TN_c*>(p);\n    bool here',
     '    if (!s.on || !dkn_combat(s) || !dkn::phase2(s.ph)) continue;\n    fopAc_ac_c* p = nullptr;\n    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;\n    auto* a = static_cast<daB_TN_c*>(p);\n    bool here'),
    ('src/dkn_adapter.inc', '      if (g.confirmed) ++s.st_parry_gain;', '      if (g.confirmed) ++s.st_parry_gain;\n      (void)dknb_link_gain(pts, false, s.id, s.generation, 0);'),
    ('src/dkn_core.hpp', '      d.pick = Pick::Single, d.idx = v.phase2 ? kIdxF2Thrust :', '      d.pick = Pick::Single, d.idx = v.phase2 ? kIdxF2Thrust + rng.below(2) :'),
    ('src/dkn_core.hpp', 'inline constexpr int kHelmClip = native::kBckTnbDamageR;', 'inline constexpr int kHelmClip = native::kBckTnbDown;'),
    ('src/dkn_adapter.inc', '    const bool entry = native::native_attack_entry(a->mActionMode1, a->mActionMode2);', '    const bool entry = true;'),
    ('src/dkn_core.hpp', 'inline constexpr int kParryStreakPct[4] = {10, 25, 45, 60};', 'inline constexpr int kParryStreakPct[4] = {10, 25, 45, 75};'),
    ('src/dkn_adapter.inc', '        else dkn_native_guard_enter_h(a, dkn_guard_facing(a, k));', '        else dkn_native_action(a, native::kActGuardL, 0);'),
    ('src/dkn_adapter.inc', '    dkn_restore(s, a, s.pre_act1, s.pre_act2);  // the native ACT_GUARDL/GUARDH the Shield Attack forced is taken back', '    (void)0;  // the native ACT_GUARDL/GUARDH'),
    ('src/dkn_core.hpp', '#include <cstdint>\n', '#include <cstdint>\n#include <vector>\n'),
    ('src/dkn_core.hpp', 'inline constexpr int kSinglePct = 25;                 // [PROVISIONAL]', 'inline constexpr int kSinglePct = 25;                 //'),
    ('src/dkn_core.hpp', '{675, 1045, 1905, 2300, 3195}', '{680, 1050, 1900, 2300, 3190}'),
    ('src/dkn_adapter.inc', 'DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept { a->mTimer1 = t; }',
     'DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept { a->mTimer1 = t; }\nvoid dkn_extra(daB_TN_c* a) noexcept { a->mTimer3 = 0; }'),
    ('src/dkn_adapter.inc', '  dkn_at_authority(s, a);\n}', '  a->field_0x6fc = 0;\n  dkn_at_authority(s, a);\n}'),
    ('src/dkn_adapter.inc', '  if (!dkr::eligible(dkr_identity(a))) {', '  if (false) {'),
    ('src/dkn_adapter.inc', '  dn.enabled = dn.hooks && dknb_dkr_enabled();', '  dn.enabled = true;'),
    ('src/dkn_adapter.inc', '  dknb_direct_capture(link);\n  for (DknSlot& s : dn.s) {', '  link->mProcID = 3;\n  for (DknSlot& s : dn.s) {'),
    ('src/mod.cpp', '  dkn_shutdown();\n', ''),
    ('src/mod.cpp', 'DEFINE_MOD();', 'DEFINE_MOD(); '),
    ('mod.json', '"version": "0.13.12"', '"version": "0.13.11"'),
    ('src/dkn_adapter.inc', '        if (p2) dkn_native_guard_enter(a, dkn_guard_bck(s, k), dkn_guard_facing(a, k));', '        if (p2) dkn_native_action(a, native::kActGuardL, 0);'),
    ('src/dkn_adapter.inc', '  if (k->mNormalSpeed > v) k->mNormalSpeed = v;', '  k->mNormalSpeed = v;'),
    ('src/dkn_adapter.inc', '  if (dkn::native::needs_hit_context(act, mode)) {\n    ++dn.unsafe_entries;', '  if (false) {\n    ++dn.unsafe_entries;'),
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
