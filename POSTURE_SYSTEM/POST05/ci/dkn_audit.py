"""LIGHT FADES TWICE 0.13.9 DARKNUT MINIBOSS (DKN01) + 0.13.10 FIX01 + ... + 0.13.16 FIX07 + 0.13.17 FIX08 + 0.13.18 FIX09: the OUTERMOST layer.
* 0.13.18 FIX09: three more writers (dkn_native_timer7: mTimer7 back to its pre-action value after a native evasion entry taken back in the POST; dkn_native_no_guard:
  the airborne maneuvers never guard; dkn_native_guard_ready: the phase-2/3 front rule for ONE damage_check), one more callback on the existing LDG01 crossing
  target (the evasion's 6-tick i-frames, the rejected Shield Attack), the voluntary end after a confirmed life hit (move-end reason LINK_HP_HIT), Table B by band,
  Table A; no new hook target, no shared file.
* 0.13.17 FIX08: two more writers (dkn_native_neck: the native neck tracking of the F1 Helm recovery look, back OFF at its end; dkn_native_recover_clip: B1's
  eased return to TNB_WAIT_B_2), dkn_native_jump also writes the native side jumps 11 / 14 (the evasion); Table B F1 24/36, 31/43, 50/60; F1-C3 36/56/57/40 with
  its pair-1 row 61/16/8 (no rate exception); the evasion (0/10/15/20 by ordinal after the parry roll, cooldown 90, one roll per swing, own lease, the native
  evasion entries taken back); the neutral receipt + the swing threat for the ordinary initiative (a native attempt no longer bypasses neutral); the sword's
  perceptual darkening value (Darknut only). Shared files: mod.cpp / parry_fx_adapter.inc / link_break_core.hpp / liz_adapter.inc hunks tagged dkn01_fix08.
* 0.13.16 FIX07: the sword darkening is written where the game diffs it (the draw PRE only ARMS the sword morf; ONE new hook target mDoExt_McaMorfSO::entryDL
  PRE writes after the native MAJI of the sword, its POST restores); F1-C3 four approved final targets (24 / 56 / 57 / 40, its own kF1Plan row); phase-1
  executions 120 total; the offensive leap (90 / 10 per opportunity, one of the four combos, native leaps 8 / 17 after a trajectory check, a lease of its own,
  the landing attack taken back); the reposition reviewed (geometry of every jump, the native turn, the long leap); the thrown mace (+50 / +60 / 0 / 0 for Link,
  never the Darknut, one resolution per throw). LDG01 (ldg_core.hpp / ldg_adapter.inc): Link's dodge i-frames 9 / 6 / 6 through ONE new hook target
  dCcS::ChkAtTgHitAfterCross POST (its only effect: the return value of that call) and one more callback on daAlink_c::setCollision PRE. No new writer function;
  mod.cpp 6 / parry_fx_core 3 / parry_fx_adapter 18 hunks dkn01_fix07.
* 0.13.15 FIX06: five approved final intervals of F2-C2 / F2-C4 (kIvTargets); the SEQUENCE of an authentic enemy parry (Defense::confirm routes 60 / 40,
  Seq: the counter's natural end rolls 80 / 20) owns the initiative through the pure seq_gate (Link's ordinary actions are not among its inputs) placed
  before every defense / special / initiative decision, ended once by the legitimate exceptions; phase-1 Mortal Draw / Helm Splitter (100 HP total,
  natural recovery, no fall); the second stagger sways (helm_rate_at); ONE native jump after the FIX02 rise / a getup (initChaseL 6 / 8 / 9 replicated,
  the landing attack taken back); the sword's TEV color 0 mirrors the body's darkening only in the definitive death, through ONE new hook target
  daB_TN_c::draw PRE / POST (written in the PRE, restored in the POST). Three new writers (jump, jump landing, sword color); mod.cpp: 2 hunks dkn01_fix06.
* 0.13.14 FIX05: every real defended contact feeds Link's chain (register_at_contact), the exchange verdict is the pure hold_verdict (distance alone never
  releases a valid chain within the initiative reach), F2/F3 own offensives are the four combos only (kF2Singles false), the F2/F3 passivity special only
  for a held shield (special_allowed), the parry credit / counter only on a PERSISTING rebound (observe_rebound), the counter double on the F2-C1 reference,
  the transformation's one-tick stale capsule re-anchored (dkn_transform_post through dkn_native_at), the orphan F2/F3 weapon AT off. Shared files: the
  exact counted hunks tagged dkn01_fix05 (mod.cpp 9: LGC01 wiring + R edge tick + buffered R+B; parry_fx_core 4 / parry_fx_adapter 6: the Darknut FX
  transaction). LGC01 (lgc_core.hpp / lgc_adapter.inc, Link's guard cancel of an ordinary cut): ONE new hook target daAlink_c::procCutNormal (PRE), ONE
  LGC_NOINLINE writer (resetCombo(TRUE) + checkNextAction(0): the cut's own native end), no input write, no other Link write.
* 0.13.13 FIX04: F1 600 / 200 and Table A F1 (+4 / +6 / +12 / +14); parry by ordinal 0/0/0/25/45/60; the exchange (Link's valid chain refuses every
  offensive decision); the parry contact kept a block (dkn_native_guard on parry_hold); the rebound verdict needs a PROC_CUT_REVERSE route and gives NO
  credit itself: the credit (+20/+25, once) and the ONE counter come from the action PRE after PROC_CUT_REVERSE is read; the Lizalfos parry FX for the
  Darknut through the shared parry_fx files, as an exact counted inverse layer (8 hunks tagged dkn01_fix04, nothing else of those files changes).
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
LGC_NEW = {'lgc_core.hpp', 'lgc_adapter.inc'}  # 0.13.14 FIX05: LGC01 (Link guard cancel), audited below
LDG_NEW = {'ldg_core.hpp', 'ldg_adapter.inc'}  # 0.13.16 FIX07: LDG01 (Link's dodge i-frames), audited below
NEW = {'dkn_core.hpp', 'dkn_bridge.inc', 'dkn_adapter.inc'} | LGC_NEW | LDG_NEW
HOOKS = [('action', 'DknAction'), ('damage_check', 'DknDamageCheck'), ('setDamage', 'DknSetDamage'), ('executeAttackH', 'DknAttackH'), ('executeAttackL', 'DknAttackL'),
         ('executeAttackShieldH', 'DknShieldH'), ('executeAttackShieldL', 'DknShieldL'), ('draw', 'DknDraw')]  # FIX06: + draw (the sword darkening)
DKR01_MOD_CPP = '0fb88a1be182c0a2beee84c3eca6f1108ac25ae94f9a65ed183eb747123f03de'  # runtime-experiments 0079b6f (0.13.8 DKR01)
WRITERS = ['dkn_native_threshold', 'dkn_native_action', 'dkn_native_armor', 'dkn_native_clip', 'dkn_native_wait_clip', 'dkn_native_rate', 'dkn_native_at',
           'dkn_native_spl', 'dkn_native_stagger_hold', 'dkn_native_guard', 'dkn_native_attack_flags', 'dkn_native_motion', 'dkn_native_timer1',
           'dkn_native_headlock', 'dkn_native_exec_fall', 'dkn_native_neutral', 'dkn_native_guard_enter', 'dkn_native_walk', 'dkn_link_slip_speed',
           'dkn_native_guard_enter_h', 'dkn_native_jump', 'dkn_native_jump_land', 'dkn_native_sword_color',  # FIX06: + the native jump, its landing, the sword color
           'dkn_native_neck', 'dkn_native_recover_clip',  # FIX08: + the Helm recovery look's native neck flag, B1's eased recovery clip
           'dkn_native_timer7', 'dkn_native_no_guard', 'dkn_native_guard_ready']  # FIX09: + mTimer7's pre-action value, the airborne no-guard, the front rule
FORBIDDEN = ['HOOK_SKIP_ORIGINAL', 'fopAcM_delete', 'fopAcM_createDisappear', 'JKR_NEW', 'malloc(', 'operator new', 'mProcID =', 'mProcID=', 'procCut', 'procGuardBreak',
             'onDownFlg', 'offDownFlg', 'setDownPos', 'dComIfGs_onSwitch', 'l_HIO', 'replace<', 'VirtualProtect', 'field_0x6fc =', 'field_0x6fc +=', 'field_0x6fc -=',
             'executeZakoChangeDemo(', 'executeChangeDemo(', 'executeEnding(', 'setActionMode(', 'mMaterialColor =', 'health =', 'svc_resource', 'mod_ctx']
CORE_NEEDS = ['kHp1Max = 600;', 'kRes2Max = 480;', 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kRegenDelayTicks = 60;', 'kRegen1[5] = {10, 7, 4, 2, 1};',
              'kRegen2[5] = {12, 8, 5, 2, 1};', 'kBreakTicks = 90;', 'kDownTicks = 180;', 'kLinkMax = 100;', 'kComboPick[4] = {25, 25, 25, 25};',
              'kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'kLinkEnemyParry1 = 20, kLinkEnemyParry2 = 25;', 'kF2ComboScale = 1.40f;', 'kF2SingleScale = 1.34f;', 'kKickScale = 1.25f;', 'kGainSwordHp1 = 6, kGainBlocked1 = 4, kGainIron1 = 25;',
              'kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;', 'kPassiveShieldTicks = 38;', 'kPassiveIdleTicks = 75;']
PROVISIONAL = ['kSinglePct', 'kGap1Min', 'kGap2Min', 'kReentryTicks', 'kGuardLeaveTicks', 'kEngage1', 'kPassiveCooldownTicks', 'kPressureWindow', 'kGuardPctBase',
               'kStreakGapTicks', 'kParryPrepTicks', 'kParryWindowTicks', 'kParryCooldownTicks', 'kFinisherExtMax', 'kHelmWindowTicks', 'kSlipChainTicks', 'kChainWhiffs',
               'kCounterLandTicks', 'kCounterLateTicks']
FIX04_SHARED = {'parry_fx_core.hpp': 2, 'parry_fx_adapter.inc': 6}  # the exact counted dkn01_fix04 hunks (Darknut kinds, previews, exec scope, summary)
FIX05_SHARED = {'mod.cpp': 9, 'parry_fx_core.hpp': 4, 'parry_fx_adapter.inc': 6}  # the exact counted dkn01_fix05 hunks (LGC01 wiring; the Darknut FX transaction)
FIX06_SHARED = {'mod.cpp': 2, 'parry_fx_core.hpp': 1, 'parry_fx_adapter.inc': 4}  # the exact counted dkn01_fix06 hunks (the draw hook target + its signature assertion; read-only FX_TXN flash telemetry)
FIX08_SHARED = {'parry_fx_adapter.inc': 12, 'liz_adapter.inc': 2, 'link_break_core.hpp': 3}  # the exact counted dkn01_fix08 hunks (parry clash same-tick restart + cut ages; Link's break damage x2)
FIX07_SHARED = {'mod.cpp': 6, 'parry_fx_core.hpp': 3, 'parry_fx_adapter.inc': 18}  # the exact counted dkn01_fix07 hunks (entryDL / crossing hooks + LDG01 wiring; parry flash / throttle / clash follow-up)
FIX06_CORE_NEEDS = ['kParryComboPct = 60;', 'kCounterComboPct = 80;', 'kParryComboLandMin = 27, kParryComboLandMax = 36;',
                    'kRepoNear = 240.0f, kRepoFar = 370.0f;', '{5, 0, 27.0f},', '{5, 2, 30.0f},', '{7, 0, 27.0f},', '{7, 1, 25.5f},', '{7, 3, 30.0f},',
                    'inline constexpr IvTarget kIvTargets[5] = {']
FIX06_PROVISIONAL = ['kBridgeSettleTicks', 'kBridgeFreeTicks', 'kBridgeHoldMax', 'kSeqWatchdogTicks', 'kRepoMaxDy', 'kRepoWaitTicks', 'kRepoSettleTicks']
FIX07_CORE_NEEDS = ['inline constexpr int kF1ExecDamage = 120;', 'kF1C3Targets[4] = {36.0f, 56.0f, 57.0f, 40.0f};', '    {256, 0, {{61, 16, 8}, {64, 8, 6}, {64, 7, 6}, {56, 12, 10}}},',  # FIX08 pair 1
                    'inline constexpr int kLeapPct = 90;', 'inline constexpr int kMaceBlockLink = 50, kMaceDirectLink = 60;', 'inline constexpr unsigned kF1C3 = 2;',
                    'inline int mace_darknut_points(Mace) noexcept { return 0; }', 'const float T = f1c3 ? kF1C3Targets[i] :', 'const bool f1c3 = idx == kF1C3;']
FIX07_PROVISIONAL = ['kLeapMaxDrop', 'kLeapMaxRise', 'kLeapLandMin', 'kLeapLandIdeal', 'kLeapDebounceTicks', 'kLeapCooldownTicks', 'kLeapEpisodeTicks', 'kLeapApproachTicks',
                     'kLeapWatchdogTicks', 'kRepoTurnTicks', 'kRepoLandIdeal']


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
    mine = [h for h in pol['inverse_patch'].get('mod.cpp', []) if h.get('dkn01') or h.get('dkn01_fix05') or h.get('dkn01_fix06') or h.get('dkn01_fix07')]
    if sum(1 for h in mine if h.get('dkn01')) != 7 or sum(1 for h in mine if h.get('dkn01_fix05')) != FIX05_SHARED['mod.cpp'] \
            or sum(1 for h in mine if h.get('dkn01_fix06')) != FIX06_SHARED['mod.cpp'] or sum(1 for h in mine if h.get('dkn01_fix07')) != FIX07_SHARED['mod.cpp']:
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
    fx4 = {k: sum(1 for h in v if h.get('dkn01_fix04')) for k, v in pol['inverse_patch'].items()}
    if {k: n for k, n in fx4.items() if n} != FIX04_SHARED:
        e.append('DKN01 FIX04 shared-file layer changed (only the parry FX files, exact counted hunks): %s' % {k: n for k, n in fx4.items() if n})
    fx5 = {k: sum(1 for h in v if h.get('dkn01_fix05')) for k, v in pol['inverse_patch'].items()}
    if {k: n for k, n in fx5.items() if n} != FIX05_SHARED:
        e.append('DKN01 FIX05 shared-file layer changed (mod.cpp LGC01 wiring + the parry FX transaction, exact counted hunks): %s' % {k: n for k, n in fx5.items() if n})
    fx6 = {k: sum(1 for h in v if h.get('dkn01_fix06')) for k, v in pol['inverse_patch'].items()}
    if {k: n for k, n in fx6.items() if n} != FIX06_SHARED:
        e.append('DKN01 FIX06 shared-file layer changed (mod.cpp draw hook target; parry_fx read-only flash telemetry): %s' % {k: n for k, n in fx6.items() if n})
    fx7 = {k: sum(1 for h in v if h.get('dkn01_fix07')) for k, v in pol['inverse_patch'].items()}
    if {k: n for k, n in fx7.items() if n} != FIX07_SHARED:
        e.append('DKN01 FIX07 shared-file layer changed (mod.cpp hook targets + LDG01 wiring; parry flash / throttle / clash follow-up): %s' % {k: n for k, n in fx7.items() if n})
    fx8 = {k: sum(1 for h in v if h.get('dkn01_fix08')) for k, v in pol['inverse_patch'].items()}
    if {k: n for k, n in fx8.items() if n} != FIX08_SHARED:
        e.append('DKN01 FIX08 shared-file layer changed (parry clash same-tick restart / cut ages; Link break damage x2): %s' % {k: n for k, n in fx8.items() if n})
    for k, v in pol['inverse_patch'].items():  # FIX08: the outermost layer of its files (after every older tag)
        t8 = [bool(h.get('dkn01_fix08')) for h in v]
        if True in t8 and t8[t8.index(True):] != [True] * (len(t8) - t8.index(True)):
            e.append('DKN01 FIX08 hunks are not the outermost layer of ' + k)
    for k, v in pol['inverse_patch'].items():  # the FIX07 layer is the OUTERMOST one: its hunks come after every older tag of the same file
        tags = [bool(h.get('dkn01_fix07')) for h in v if not h.get('dkn01_fix08')]  # FIX08: the newer layer sits outside it
        if any(tags) and tags.index(True) != len(tags) - sum(tags):
            e.append('DKN01 FIX07 hunks are not the outermost layer of ' + k)
    fxa = (p / 'src' / 'parry_fx_adapter.inc').read_text(encoding='utf-8')
    if 'parry = shield_contact && dkn_preview_enemy_parry(static_cast<daB_TN_c*>(ta), k, tg);' not in fxa or 'fopAcM_GetName(ta) == fpcNm_B_TN_e && pfx_link_sword(k, at)' not in fxa \
            or 'pf.exec_kind = pfx::Kind::DarknutOnLink, pf.exec_map = 42,' not in fxa:
        e.append('FIX04: the Darknut parry FX are not scoped to the B_TN contact / its own damage_check (map 42)')
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
        e.append('DKN01 writer inventory is not exactly the twenty-eight writers (FIX08 neck flag + B1 recovery clip; FIX09 timer7 restore, no-guard, front rule)')
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
    if len(re.findall(r'mods::hook::add_(pre|post)<', inst)) != 21 or 'add_post<LdgAfterCross>(on_dkn_after_cross_post)' not in inst or 'dn.enabled = dn.hooks && dknb_dkr_enabled();' not in inst or 'uninstall<DknAction>' not in inst \
            or 'add_pre<DknDraw>(on_dkn_draw_pre)' not in inst or 'add_post<DknDraw>(on_dkn_draw_post)' not in inst or 'uninstall<DknDraw>' not in inst \
            or 'add_pre<DknMorfEntryDL>(on_dkn_morf_entry_pre)' not in inst or 'add_post<DknMorfEntryDL>(on_dkn_morf_entry_post)' not in inst or 'uninstall<DknMorfEntryDL>' not in inst:
        e.append('install is not the twenty-one PRE/POST records (FIX06 draw, FIX07 sword morf entryDL, FIX09 one crossing callback on the LDG01 target), all or nothing, only with DKR01')
    if a.count('kk_native_mortal_draw(') != 1 or 'kk_native_mortal_draw(' not in body(raw, 'void on_dkn_cut_action_post('):
        e.append('Link is driven outside the ONE native Mortal Draw request')
    md = body(raw, 'void on_dkn_cut_action_post(')
    if 's.brk.st != dkn::Bk::StaggerA' not in md or 's.md_active) continue;' not in md or 'kk::native_instant_cut(cut)) return;' not in md:
        e.append('the Mortal Draw request is not limited to the first stagger (FIX06: phases 1, 2 and 3), one in flight, ordinary cuts only')
    rv = body(raw, 'void on_dkn_cut_reverse_post(')
    if 'const bool route = k == dknb_link() && dkn_link_reverse_route(k);' not in rv or '? s.def.rebound(now) : 0;' not in rv or '} else if (rb == 0) {\n      *r = FALSE;' not in rv \
            or 'phase2(s.ph)) continue' in rv or 'dknb_link_gain(' in rv:
        e.append('the rebound verdict is not: armed window + a PROC_CUT_REVERSE route = rebound kept (no credit here), otherwise no rebound, in EVERY phase (FIX04)')
    pre = body(raw, 'HookAction on_dkn_action_pre(')
    anchor = 'const Rb rb = s.def.observe_rebound(now, rev, rf, p2, shield_ok);'
    cf = pre[pre.index(anchor):] if anchor in pre else ''
    if not cf or a.count('dknb_link_gain(') != 4 or 'dknb_link_gain(' not in body(cf, 'if (rb == Rb::Confirmed) {') or pre.count('dknb_link_gain(') != 1 \
            or body(raw, 'void dkn_mace_contact(').count('dknb_link_gain(') != 1 \
            or 'const bool rev = lp == daAlink_c::PROC_CUT_REVERSE;' not in pre or 's.def.confirm(' in pre:
        e.append('FIX04/FIX05: the enemy-parry credit is not given exactly once, on a PERSISTING PROC_CUT_REVERSE read in the action PRE (observe_rebound)')
    if '(s.def.window_open(now) || s.def.parry_hold(now)) && front && shield_ok) dkn_native_guard(a, 2);' not in pre or 'iv.defending = hold;' not in pre \
            or 'const bool neutral = !def_busy && !hold &&' not in pre or 'int c = s.def.due(now);' not in pre:
        e.append('FIX04: the parry contact is not kept a block, or the exchange / the scheduled counter lost their authority in the action PRE')
    core0 = (p / 'src' / 'dkn_core.hpp').read_text(encoding='utf-8')
    # FIX05: the four combos only in F2/F3, the shield-held special only, the exchange verdict, every defended contact in the chain, the counter double ref
    if 'inline constexpr bool kF2Singles = false;' not in core0 or 'if ((!v.phase2 || policy::kF2Singles) && rng.pct(policy::kSinglePct)) {' not in core0:
        e.append('FIX05: phases 2-3 may draw an autonomous isolated strike')
    if 'inline bool special_allowed(Pv v, bool phase2) noexcept { return v == Pv::Shield || (v == Pv::Idle && !phase2); }' not in core0 \
            or 'if (pv != Pv::None && !special_allowed(pv, p2)) {' not in pre:
        e.append('FIX05: the F2/F3 passivity special is not limited to a held shield')
    if 'const Hold hv = hold_verdict(s.def, now, can_defend, k && dknb_link_alive(), dknb_link_broken(), dist, front, p2);' not in pre or 'const bool hold = hv == Hold::Holds;' not in pre \
            or 'if (!(dist < policy::hold_reach(phase2))) return Hold::Far;' not in core0:
        e.append('FIX05: the exchange is not the pure hold_verdict within the initiative reach')
    dp5 = body(raw, 'void dkn_damage_post_body(void* args) noexcept {')
    if 'const bool late = s.def.register_at_contact(now, ck);' not in dp5 or 'if (s.def.on_contact(ck)) {' not in dp5 or dp5.index('register_at_contact') > dp5.index('on_contact(ck)'):
        e.append('FIX05: a real defended contact of an unregistered swing does not feed the chain')
    if 'if (idx == kIdxF2Double && c.phase2 && !(scale > 0.0f)) return plan_counter_double();' not in core0 or 'inline constexpr unsigned kCounterDoubleRef = 4;' not in core0:
        e.append('FIX05: the counter double is not the F2-C1 reference double')
    # FIX05: an offensive already running is independent of Link's B / blocks / parries: the ONLY ways an owned move ends are its own end, a native
    # takeover, an approved interruption (break, phase, defeat, no target / Link broken / event / Link > 700 at a move boundary, watchdog). Exact inventory.
    # FIX09 (approved): + LINK_HP_HIT, the voluntary end after a CONFIRMED life hit on Link (reached only through s.hit_ending, set only by dkn_hit_end_apply)
    ends = sorted(set(re.findall(r'dkn_move_end\(s, a, (?:s\.hit_ending \? "\w+" : )?"(\w+)"', raw)) | set(re.findall(r'dkn_move_end\(s, a, s\.hit_ending \? "(\w+)"', raw))
                  | set(re.findall(r'"(NO_TARGET|LINK_GUARD_BROKEN|EVENT|LINK_FAR)"', body(raw, 'void dkn_combo_post('))))
    if ends != sorted(['ACTION_LOST', 'B1_RECOVERY', 'DEFEAT', 'EVENT', 'LINK_FAR', 'LINK_GUARD_BROKEN', 'LINK_HP_HIT', 'NATIVE_END', 'NO_TARGET', 'PHASE_CHANGE', 'POSTURE_BREAK', 'WATCHDOG']) \
            or len(re.findall(r'dkn_move_end\(', raw)) != 14 or 'dkn_move_end(' in body(raw, 'void dkn_damage_post_body(void* args) noexcept {') \
            or 'dkn_move_end(' in body(raw, 'void dkn_link_contact(') or 'dkn_move_end(' in body(raw, 'void on_dkn_cut_reverse_post('):
        e.append('FIX05: an owned move can end for a reason outside the approved inventory (Link B / block / parry must never end it): %s' % ends)
    tf = body(raw, 'void dkn_transform_post(DknSlot& s, daB_TN_c* a) noexcept {')
    at_writer = body(raw, 'DKN_NOINLINE void dkn_native_at(')
    if 'dkn_native_at(a, true, 0, true);' not in tf or 'dkn_native_at(a, true, 1);' not in tf or 'a->mActionMode2 != 5' not in tf or 'a->mStates[15] == 1' not in tf \
            or 'if (in_hand && !a->mCps.ChkAtSet())' not in tf \
            or 'if (sword && capsule_only)' not in at_writer or 'a->mCps.OffAtSetBit();' not in at_writer or 'a->mCps.ClrAtHit();' not in at_writer \
            or 'dkn_transform_post(*sp, a);' not in body(raw, 'void on_dkn_action_post('):
        e.append('FIX05: the transformation must disable only the stale capsule for one tick, preserve close sword spheres and restore the capsule natively')
    if 'd.idx = v.phase2 ? kIdxF2Thrust :' not in core0 or 'rng.below(2) : (v.dist' in core0:
        e.append('FIX03: an ordinary phase-2 single can be the full double (F2-SINGLE-B)')
    if 'inline constexpr int kHelmClip = native::kBckTnbDamageR;' not in core0 or 'dkn_native_clip(a, hc.bck, kHelmMorph, kHelmStart, 1.0f);' not in raw \
            or 'inline constexpr HelmClip kHelmClipF2 = {kHelmClip, native::kEndTnbDamageR, native::kActYoroke, 10};' not in core0 \
            or 'inline constexpr HelmClip kHelmClipF1 = {native::kBckTnaDamageR, native::kEndTnaDamageR, native::kActDamageH, 2};' not in core0:
        e.append('FIX03/FIX06: the second stagger is not TNB_DAMAGE_R (F2/F3) / the native TNA_DAMAGE_R (F1) (the TNB_DOWN hold reads as a guard)')
    po = body(raw, 'void on_dkn_action_post(')
    if 'const bool entry = native::native_attack_entry(a->mActionMode1, a->mActionMode2);' not in po or 'executed=1' not in raw or 'NATIVE_ENTRY_SUPPRESSED' not in po:
        e.append('FIX03: the action POST does not tell a suppressed native entry from an executed unauthorized attack')
    dp = body(raw, 'void dkn_damage_post_body(void* args) noexcept {')  # FIX04: on_dkn_damage_post = this body + the end of the Darknut FX scope
    if 'dkn_damage_post_body(args);\n  pfx_dkn_exec_end();' not in body(raw, 'void on_dkn_damage_post('):
        e.append('FIX04: the damage_check POST does not close the Darknut parry FX scope')
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
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.18':
        e.append('mod id / version')
    e += audit_lgc(p, m)
    try:
        e += audit_fix06(p, raw, core, m)
    except ValueError as x:  # an anchor of a FIX06 rule is gone: that is a failed rule, never a crash
        e.append('FIX06 rule anchor lost: %s' % x)
    try:
        e += audit_fix07(p, raw, core, m)
        e += audit_ldg(p, m)
    except ValueError as x:
        e.append('FIX07 rule anchor lost: %s' % x)
    try:
        e += audit_fix08(p, raw, core, m)
    except ValueError as x:
        e.append('FIX08 rule anchor lost: %s' % x)
    try:
        e += audit_fix09(p, raw, core, m)
    except ValueError as x:
        e.append('FIX09 rule anchor lost: %s' % x)
    return e


def audit_fix06(p, raw, core, m):
    """0.13.15 FIX06: the approved numbers; the sequence's authority (pure gate, before every other decision, Link's actions not among its inputs, ended once
    by the legitimate exceptions); phase-1 executions; the sway; the reposition jump; the sword darkening in the definitive death only, written / restored
    inside ONE draw call."""
    e = []
    for need in FIX06_CORE_NEEDS:
        if need not in core:
            e.append('FIX06 approved number changed: ' + need)
    for name in FIX06_PROVISIONAL:
        line = next((l for l in core.splitlines() if ' ' + name + ' = ' in l), '')
        if '[PROVISIONAL]' not in line:
            e.append('FIX06 provisional value not marked as such: ' + name)
    if 'const bool fix06 = !(scale > 0.0f) && idx < 8;' not in core or 'const float t = fix06 ? iv_target(idx, j) : -1.0f;' not in core:
        e.append('FIX06: the five targets are not applied by the planner only for the eight combos with the default tempo')
    conf = body(core, '  bool confirm(std::uint64_t now, bool link_rebounding, bool p2, bool shield) noexcept {')
    if 'if (routes) {' not in conf or 'if (route.pct(policy::kParryComboPct)) {' not in conf or 'const unsigned k = route.below(4);' not in conf \
            or conf.index('if (cp == Cp::CreditOnly) return cp = Cp::None, true;') > conf.index('if (routes) {'):
        e.append('FIX06: the parry route is not ONE roll (60 / 40, 25 / 25 / 25 / 25) on its own stream after the credit-only exit')
    cd = body(core, '  bool counter_done(std::uint64_t now, std::uint64_t last, bool p2) noexcept {')
    if 'if (st != Sq::Counter) return false;' not in cd or 'if (!rng.pct(policy::kCounterComboPct))' not in cd or 'const unsigned k = rng.below(4);' not in cd:
        e.append('FIX06: the continuation is not ONE roll (80 / 20) per concluded counter')
    gate = body(core, 'inline GateOut seq_gate(Seq& q, const GateIn& in) noexcept {')
    gin = body(core, 'struct GateIn {')
    if re.search(r'cut|swing|guard|parry|shield|jump|press|button|proc', gin) or 'const int b = q.bridge_tick(in.now, in.link_held);' not in gate:
        e.append('FIX06: the sequence gate reads something of Link\'s ordinary actions (only a forced state may delay the bridge)')
    if 's.def.routes = true;' not in body(raw, 'DknSlot* dkn_admit(daB_TN_c* a)'):
        e.append('FIX06: the production Defense does not route (routes = true at admission)')
    pre = body(raw, 'HookAction on_dkn_action_pre(')
    gp = pre.find('const GateOut go = seq_gate(s.seq, gi);')
    if gp < 0 or not (pre.index('int c = s.def.due(now);') < gp < pre.index('s.def.on_swing(') < pre.index('s.pas.step(') < pre.index('s.ini.decide(iv)')) \
            or 'if (s.seq.owns()) return HOOK_CONTINUE;' not in pre or pre.index('if (s.seq.owns()) return HOOK_CONTINUE;') > pre.index('s.def.on_swing('):
        e.append('FIX06: the sequence gate does not run before (and stop) the defense verdict, the special and the common initiative')
    gi = pre[pre.index('GateIn gi;'):gp] if 'GateIn gi;' in pre and gp > 0 else ''
    if 'gi.link_held = classify(lp) == Act::Hold;' not in gi or re.search(r'swing|cutting|guarding|getCutType|PROC_GUARD|sa_serial|lp ==', gi):
        e.append('FIX06: the gate inputs include Link\'s ordinary actions')
    cf = body(pre[pre.index('const Rb rb = s.def.observe_rebound('):], 'if (rb == Rb::Confirmed) {')
    if raw.count('s.seq.begin(') != 1 or 's.seq.begin(now, s.def.parry_tick, s.def.route_combo, s.def.counter_idx)' not in cf or 'if (!credit_only && s.def.cp == Cp::Scheduled) {' not in cf:
        e.append('FIX06: a sequence starts outside the authentic confirmed parry (or over a credit-only confirmation)')
    if 'combo ? DknLease::Combo : DknLease::Counter, combo ? "PARRY_DIRECT_COMBO" : "AFTER_ENEMY_PARRY_REAL"' not in pre or 's.seq.started(combo);' not in pre:
        e.append('FIX06: the scheduled move of a parry is not the routed combo / counter of the sequence')
    me = body(raw, 'void dkn_move_end(DknSlot& s, daB_TN_c* a, const char* why, bool cancel) noexcept {')
    if 'const dkn::SqEnd e = dkn::seq_end_of(why);' not in me or 's.seq.counter_done(now, last, dkn::phase2(s.ph))' not in me or 's.seq.combo_done();' not in me:
        e.append('FIX06: an owned move of a sequence does not end it / continue it by its own end reason')
    for fn, need in (('void dkn_begin_break(DknSlot& s, daB_TN_c* a) noexcept {', 'dkn_seq_end(s, a, SqEnd::Break,'), ('void on_dkn_setdamage_post(', 'dkn_seq_end(s, a, SqEnd::Phase, "HP1_ZERO");'),
                     ('void on_dkn_setdamage_post(', 'dkn_seq_end(s, a, SqEnd::Defeat, "RESISTANCE_480");'), ('void dkn_phase_track(DknSlot& s, daB_TN_c* a) noexcept {', 'dkn_seq_end(s, a, SqEnd::Phase, "ACT_CHANGEDEMO");'),
                     ('void dkn_phase_track(DknSlot& s, daB_TN_c* a) noexcept {', 'dkn_seq_end(s, a, SqEnd::Defeat, "ACT_ENDING");'), ('void dkn_back_to_combat(', 'dkn_seq_end(s, a, dkn::SqEnd::Reset, ev);')):
        if need not in body(raw, fn):
            e.append('FIX06: a legitimate exception does not end the sequence: ' + need)
    se = body(raw, 'void dkn_seq_end(DknSlot& s, daB_TN_c* a, dkn::SqEnd e, const char* detail) noexcept {')
    if 'if (s.def.cp == dkn::Cp::Scheduled) s.def.cp = dkn::Cp::None' not in se or 's.seq.finish(e);' not in se:
        e.append('FIX06: a sequence can end and leave its scheduled move behind (a late attack)')
    if 'if (s.lease != DknLease::None || s.brk.active() || s.seq.owns()) {' not in body(raw, 'void dkn_damage_post_body(void* args) noexcept {'):
        e.append('FIX06: a block inside a sequence can take the Darknut out of it (its native guard is not taken back)')
    sd = body(raw, 'void on_dkn_setdamage_post(')
    if 'const bool f1x = exec_verdict_f1(false, s.brk.st, md1, helm1);' not in sd or 's.hp1 = f1_exec_hp(s.hp1);' not in sd or 'if (f1x && s.brk.exec_f1()) {' not in sd \
            or sd.index('if (s.hp1 <= 0) {') > sd.index('if (f1x && s.brk.exec_f1()) {') or sd.count('dkr_exec_request(') != 1 or sd.index('dkr_exec_request(') < sd.index('} else {  // ---- phases 2 / 3'):
        e.append('FIX06/FIX07: the phase-1 punishment is not the approved total (FIX07 120) with the transformation first and never the DKR01 fall')
    if 'return exec_verdict_f1' in core or 'inline bool exec_verdict_f1(bool phase2, Bk st, bool md, bool helm) noexcept { return !phase2 && ((st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm)); }' not in core \
            or 'inline int f1_exec_hp(int hp) noexcept { return hp > policy::kF1ExecDamage ? hp - policy::kF1ExecDamage : 0; }' not in core:
        e.append('FIX06: the phase-1 verdict / reserve rule changed')
    po = body(raw, 'void on_dkn_action_post(')
    if 'dkn_native_rate(a, helm_rate_at(s.brk.b_ticks, dkn_frame(a)));' not in po or 'inline float helm_rate(' in core or 'if (r > -kHoldCreep && r < kHoldCreep)' not in core:
        e.append('FIX06: the second stagger can be held still (the 0.13.14 creep) or play at rate 0')
    if raw.count('dkn_native_jump(a, ') != 3 or raw.count('dkn_native_jump(a, dkn::evade_mode(dir));') != 1 or 'dkn_native_jump(a, leap_mode(l));' not in body(pre[pre.index('if (s.repo_pending) {'):], 'if (s.repo_pending) {') \
            or raw.count('s.repo_pending = true') != 2 or 'dkn_back_to_combat(s, a, "GETUP_DONE", ex);\n      s.repo_pending = true' not in raw \
            or 'dkn_back_to_combat(s, a, "FIX02_RISE_DONE", ex);\n    s.repo_pending = true' not in raw:
        e.append('FIX06: the reposition jump is not ONE decision after FIX02_RISE_DONE / GETUP_DONE only')
    if 'if (landing_attack && !s.repo_back) {\n      dkn_native_jump_land(a);' not in po or raw.count('dkn_native_jump_land(a)') != 2 or 's.ini.settle(now + policy::kRepoSettleTicks);' not in po:
        e.append('FIX06: the landing attack of the reposition jump is not taken back (or the settle is lost)')
    if 'if (s.lease == DknLease::None && native::native_leap(act0, mode0)) {' not in pre:
        e.append('FIX06: the native leap suppression is not kept outside the reposition lease')
    dpre = body(raw, 'HookAction on_dkn_draw_pre(')
    if 'if (!s || !dkn::sword_tint(s->ph == dkn::Ph::Dead, a->mMaterialColor)) return HOOK_CONTINUE;' not in dpre \
            or 'inline bool sword_tint(bool definitive, float material_color) noexcept { return definitive && material_color != 0.0f; }' not in core:
        e.append('FIX06: the sword darkening is not limited to the definitive death (FIX07 writes it in the sword morf entryDL, audit_fix07)')
    if m.count('DEFINE_HOOK(&daB_TN_c::draw, DknDraw);') != 1 or 'std::is_same_v<decltype(&daB_TN_c::draw), int (daB_TN_c::*)()>' not in m:
        e.append('FIX06: the draw hook target / signature in mod.cpp changed')
    return e


def audit_fix07(p, raw, core, m):
    """0.13.16 FIX07: the approved numbers; the sword darkening written where it is diffed (after the native MAJI of the sword) and only there; F1-C3's own
    targets; the offensive leap (one roll per opportunity, its own stream, the trajectory read before every jump, a lease the ordinary actions of Link do not
    enter, the landing attack taken back); the reviewed reposition; the thrown mace (one contact, never the Darknut); 120 total in phase 1."""
    e = []
    for need in FIX07_CORE_NEEDS:
        if need not in core:
            e.append('FIX07 approved number / rule changed: ' + need)
    for name in FIX07_PROVISIONAL:
        line = next((l for l in core.splitlines() if ' ' + name + ' = ' in l), '')
        if '[PROVISIONAL]' not in line:
            e.append('FIX07 provisional value not marked as such: ' + name)
    # the sword: the draw PRE only arms; the entryDL PRE of the ARMED morf writes; its POST restores; the draw POST restores anything left and disarms
    dpre, dpost = body(raw, 'HookAction on_dkn_draw_pre('), body(raw, 'void on_dkn_draw_post(')
    epre, epost, rst = body(raw, 'HookAction on_dkn_morf_entry_pre('), body(raw, 'void on_dkn_morf_entry_post('), body(raw, 'void dkn_tint_restore(')
    if 'dkn_native_sword_color' in dpre or 'dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor)' not in dpre \
            or 'if (!dn.tint_morf || !args || mods::arg<const mDoExt_McaMorfSO*>(args, 0) != dn.tint_morf) return HOOK_CONTINUE;' not in epre or 'dkn_native_sword_color(mat, v, v, v);' not in epre \
            or 'if (!dn.tint_data || !args || mods::arg<const mDoExt_McaMorfSO*>(args, 0) != dn.tint_morf) return;' not in epost or 'dkn_tint_restore();' not in epost \
            or 'dkn_native_sword_color(mat, dn.tint_saved[i][0], dn.tint_saved[i][1], dn.tint_saved[i][2]);' not in rst or 'dkn_tint_restore();' not in dpost \
            or 'dn.tint_morf = nullptr, dn.tint_slot = nullptr;' not in dpost or raw.count('dkn_native_sword_color(mat,') != 2:
        e.append('FIX07: the sword darkening is not armed in the draw PRE, written only in the armed morf\'s entryDL PRE and restored in its POST (and the draw POST)')
    if m.count('DEFINE_HOOK(&mDoExt_McaMorfSO::entryDL, DknMorfEntryDL);') != 1 or 'std::is_same_v<decltype(&mDoExt_McaMorfSO::entryDL), void (mDoExt_McaMorfSO::*)()>' not in m:
        e.append('FIX07: the sword morf entryDL hook target / signature in mod.cpp changed')
    # F1-C3: the planner uses its targets for idx 2 only, the one rate exception for its pair 1 only
    pf1 = body(core, 'inline Plan plan_f1(const Combo& c, const F1Plan& f, unsigned idx = 99) noexcept {')
    if 'const float rmax = f1c3 && i == 0 ? kF1C3Pair1RateMax : kF1RecoveryRateMax;' not in pf1 or 'if (!c.phase2 && idx < 4) return plan_f1(c, kF1Plan[idx], idx);' not in core:
        e.append('FIX07: F1-C3 targets / its pair-1 recovery exception leak to another plan')
    # the offensive leap
    pre = body(raw, 'HookAction on_dkn_action_pre(')
    dec = pre.find('const GapOut go = s.gap.decide(gv);')
    if dec < 0 or not (pre.index('s.pas.step(') < dec < pre.index('s.ini.decide(iv)')) \
            or 'gv.phase2 = p2, gv.free = neutral && !def_busy && !hold && now >= s.ini.reentry_until && !s.repo_pending && !evade_hold && !reacting && !s.pht.pending && now >= s.ph_hold_until;' not in pre:
        e.append('FIX07: the offensive leap is not an ordinary, free decision point (after the defense / special, before the common initiative, never in a re-entry / reposition)')
    lp = pre[dec:pre.index('s.ini.decide(iv)')] if dec >= 0 else ''
    if lp.index('dkn_geo(') > lp.index('dkn_native_jump(a, leap_mode(go.leap));') if 'dkn_geo(' in lp and 'dkn_native_jump(a, leap_mode(go.leap));' in lp else True:
        e.append('FIX07: the offensive leap jumps without reading its trajectory first')
    if 'if (g.g != Geo::Ok) {' not in lp or 's.gap.refused()' not in lp:
        e.append('FIX07: a refused trajectory does not consume the roll (a reroll per tick)')
    rp = body(pre[pre.index('if (s.repo_pending) {'):], 'if (s.repo_pending) {')
    if 'gb = dkn_geo(a, dkn_dir_to(a, k, true), leap_model(Leap::Back).dist);' not in rp or 'repo_verdict7(dist, dy, safe, front, turn_left, gb.g == Geo::Ok, g8.g == Geo::Ok, g17.g == Geo::Ok)' not in rp:
        e.append('FIX07: the reposition jumps (back included) without its trajectory check')
    if 's.gap.rng.seed(' not in body(raw, 'DknSlot* dkn_admit(daB_TN_c* a)') or 'rng.pct(policy::kLeapPct)' not in body(core, '  GapOut decide(const GapView& v) noexcept {'):
        e.append('FIX07: the 90 / 10 roll is not on its own stream per actor / generation')
    if 'if (s.lease == DknLease::Leap) {  // FIX07: the offensive leap owns the Darknut' not in pre or pre.index('if (s.lease == DknLease::Leap) {') < pre.index('if (s.brk.st != Bk::Combat) return HOOK_CONTINUE;'):
        e.append('FIX07: the leap lease is not handled after the break priority')
    po = body(raw, 'void on_dkn_action_post(')
    if 'if (s.leap_phase == Lp::Air && entry) {\n      dkn_native_jump_land(a);' not in po or 'dkn_native_neutral(a, true);\n      ++s.st_leap_native_cut;' not in po:
        e.append('FIX07: a native landing attack / native attack entry can run inside the leap lease')
    br = body(raw, 'void dkn_begin_break(DknSlot& s, daB_TN_c* a) noexcept {')
    if 's.lease == DknLease::Leap || s.lease == DknLease::Evade) s.lease = DknLease::None;' not in br:
        e.append('FIX07: a posture break does not end the leap lease')
    lpre = body(raw, 'void dkn_leap_pre(DknSlot& s, daB_TN_c* a, daAlink_c* k, std::uint64_t now) noexcept {')
    if re.search(r'swing|cutting|guarding|getCutType|PROC_GUARD|sa_serial|link_key', lpre):
        e.append('FIX07: Link\'s ordinary actions are inputs of the leap phase')
    # the thrown mace
    dpr = body(raw, 'HookAction on_dkn_damage_pre(')
    mc = body(raw, 'void dkn_mace_contact(')
    if 'if (sp && sp->ph == dkn::Ph::Transform) {' not in dpr or dpr.index('dkn_mace_contact(*sp, a);') > dpr.index('if (!sp || !dkn_combat(*sp)) return HOOK_CONTINUE;') \
            or 'if (!s.mace.resolve(m)) {' not in mc or 'dkn_posture_gain' in mc or 'const bool anchor = a->mStates[13] == 3 && a->mStates[15] != 1;' not in body(raw, 'void dkn_mace_post('):
        e.append('FIX07: the thrown mace is not ONE contact per throw, only while cc_set anchors the colliders on it, never the Darknut\'s posture')
    return e


FIX08_CORE_NEEDS = ['{12, 0}, 24, 36, 8.0f},', '{14, 0}, 31, 43, 8.0f},', '{14, 0}, 50, 60, 3.0f},', '36.0f, 270.0f, native::kActAttackL, 1, {11, 0}, 23, 33, 3.0f},', '55.0f, 240.0f, native::kActAttackL, 2, {11, 0}, 23, 33, 3.0f},', '{9, 9}, 12, 22, 3.0f},', '{15, 0}, 38, 45, 3.0f},',
                    'kEvadeOrdinalPct[6] = {0, 10, 15, 20, 20, 20};', 'inline constexpr unsigned kEvadeCooldownTicks = 90;', 'inline constexpr float kF1C3Pair1RateMax = kF1RecoveryRateMax;',
                    'inline constexpr int kB1RecoverBck = 0x2C;', 'inline constexpr float kB1RecoverMorph = -20.0f;', 'inline constexpr int kB1RecoverTicks = 18;',
                    'inline constexpr int kHelmTurnScale = 4, kHelmTurnMax = 0x400, kHelmTurnMin = 16;', 'inline constexpr float kHelmTurnFromFrame = 24.0f;',
                    'inline bool native_evade_entry(int act, int mode) noexcept { return act == native::kActChaseL && (mode == 6 || mode == 11 || mode == 14); }']
FIX08_PROVISIONAL = ['kHelmLookDelayTicks', 'kHelmLookTailTicks', 'kEvadeLeapHoldTicks', 'kEvadeBackDist', 'kSwingReachLong', 'kSwingActiveTicks']


def audit_fix08(p, raw, core, m):
    """0.13.17 FIX08: the approved numbers; the evasion after the parry roll and only through the native jumps; the neutral receipt and the swing threat on the
    ordinary initiative only; the F1 Helm look (native neck flag ON in the recovery, OFF at its end); B1's eased recovery; the sword value; Darknut-only scope."""
    e = []
    for need in FIX08_CORE_NEEDS:
        if core.count(need) != 1:
            e.append('FIX08 approved number / rule changed: ' + need)
    for name in FIX08_PROVISIONAL:
        mm = re.search(r'inline constexpr [^;]*\b' + name + r'\b[^;]*;[^\n]*', core)
        if not mm or '[PROVISIONAL]' not in mm.group(0):
            e.append('FIX08 provisional value not marked as such: ' + name)
    pre = body(raw, 'HookAction on_dkn_action_pre(')
    i_roll, i_ev, i_guard = pre.find('const Dv v = s.def.on_swing('), pre.find('if (v != Dv::Parry && p2 && k) {'), pre.find('if (v != Dv::None) {')
    if min(i_roll, i_ev, i_guard) < 0 or not (i_roll < i_ev < i_guard):
        e.append('FIX08: the evasion is not decided after the parry roll of the same swing (the parry has priority) and before its guard entry')
    ev = pre[i_ev:i_guard] if i_ev >= 0 and i_guard > i_ev else ''
    st9 = body(raw, 'void dkn_evade_start(DknSlot& s, daB_TN_c* a, dkn::Ed dir, float dist, std::uint64_t now, bool retreat) noexcept {')  # FIX09: the one maneuver start
    if 'const Ev er = s.evade.decide(ev);' not in ev or 'dkn_geo_evade(a, pl)' not in ev or 'dkn_evade_start(s, a, dir, dist, now, false);' not in ev \
            or 'dkn_native_jump(a, dkn::evade_mode(dir));' not in st9 or 's.lease = DknLease::Evade' not in st9 \
            or ev.index('dkn_geo_evade(a, pl)') > ev.index('dkn_evade_start(s, a, dir, dist, now, false);') or 'if (dir == Ed::None) {' not in ev \
            or 'evade_pick_band(ev.band, dist, side, (s.def.cur_key & 1) != 0, gl.g == Geo::Ok, gr.g == Geo::Ok, gb.g == Geo::Ok)' not in ev:
        e.append('FIX08: the evasion jumps without its one roll / its trajectory read first / its own lease, or rerolls on a refused direction')
    if 'human && dknb_link_alive() && dknb_world_safe() && !dknb_link_broken() && a->mAcch.ChkGroundHit() && !s.repo_pending && !s.exec && !knock && !jumping' not in ev:
        e.append('FIX08: the evasion eligibility gates changed (human Link, ground, no reposition / execution / knockback / jump, no armed window or counter)')
    if 'if (s.lease == DknLease::None && p2 && evade_native_entry_any(act0, mode0)) {' not in pre:
        e.append('FIX08: the native evasion entries can run (an uncontrolled second evasion source)')
    if 'iv.neutral = neutral && gate_ok' not in pre or '|| from_native, iv.dist' in pre or 'iv.link_swing = link_swing;' not in pre:
        e.append('FIX08: the ordinary initiative ignores the neutral receipt / Link\'s running swing, or a native attempt bypasses neutral')
    if raw.count('s.ngate.defense(now, ') != 6:
        e.append('FIX08/FIX09: the neutral receipt is not armed by exactly: a guard verdict, an ordinary block, the exchange release, a maneuver start and its end, Link free after a life hit')
    if 'seq' in body(core, 'struct NeutralGate {'):
        e.append('FIX08: the neutral receipt touches the parry sequence (an authentic parry never waits for it)')
    po = body(raw, 'void on_dkn_action_post(')
    if 'if (s.brk.st == Bk::Recover && s.helm_look.on()) {' not in po or 'dkn_native_neck(a, true), s.neck_on = true;' not in po \
            or 'dkn_native_motion(a, 0.0f, turn_step(a->shape_angle.y, dkn_angle_to(a, k)));' not in po or 'if (k && dkn_stagger_action(s, a) && s.helm_look.turn(dkn_frame(a)))' not in po:
        e.append('FIX08: the F1 Helm recovery look is not the native neck after the hit + the native ChaseH turn of the body from frame 24 only')
    if raw.count('dkn_native_neck(a, false)') != 1 or 'dkn_native_neck(static_cast<daB_TN_c*>(p), false)' not in body(raw, 'void dkn_shutdown()') \
            or raw.count('dkn_helm_look_end(') < 5:
        e.append('FIX08: the native neck flag can stay ON (look end / phase change / HP 0 / new break / unload)')
    sd = body(raw, 'void on_dkn_setdamage_post(')
    if 'if (helm1) {' not in sd or 's.helm_look.begin(now);' not in sd or sd.index('if (helm1) {') < sd.index('if (f1x && s.brk.exec_f1()) {'):
        e.append('FIX08: the Helm look is not limited to the non-lethal phase-1 Helm Splitter contact (Mortal Draw / phases 2-3 untouched)')
    cp = body(raw, 'void dkn_combo_post(DknSlot& s, daB_TN_c* a, daAlink_c* k) noexcept {')
    if 'dkn_native_recover_clip(a, policy::kB1RecoverBck, policy::kB1RecoverMorph);' not in cp or 'dkn_native_wait_clip(a);' in cp or 'dkn_native_timer1(a, policy::kB1RecoverTicks);' not in cp:
        e.append('FIX08: B1 does not end with the native eased recovery into the combat stance')
    if 'a->setBck(bck, 2, morph, 1.0f);' not in body(raw, 'DKN_NOINLINE void dkn_native_recover_clip(') or 'a->mUpdateNeckAngle = on;' not in body(raw, 'DKN_NOINLINE void dkn_native_neck('):
        e.append('FIX08: a new writer writes more than its one native field / call')
    jw = body(raw, 'DKN_NOINLINE void dkn_native_jump(')
    if 'right ? 0x24 : left ? 0x22' not in jw or 'if (!back && !right && !left && mode != dkn::native::kChaseJumpFwd && mode != dkn::native::kChaseJumpFwdLong) return;' not in jw:
        e.append('FIX08: the jump writer accepts a mode / clip that is not a native ChaseL jump')
    dpre = body(raw, 'HookAction on_dkn_draw_pre(')
    if 'if (!a || a->mType != 1) return HOOK_CONTINUE;' not in dpre or 's->ph == dkn::Ph::Dead' not in dpre:
        e.append('FIX08: the sword darkening leaves the definitive death of the admitted Darknut')
    for other in ('liz_adapter.inc', 'kk_adapter.inc', 'cp06_combat_adapter.inc'):
        t = (p / 'src' / other).read_text(encoding='utf-8')
        if 'mMaterialColor' in t or 'sword_tint' in t or 'getTevColor' in t:
            e.append('FIX08: a weapon darkening appeared outside the Darknut (' + other + ')')
    return e


FIX09_CORE_NEEDS = ['inline constexpr unsigned kEvadeIframeTicks = 6;', 'kEvadeBandPct[4][4] = {{0, 10, 15, 20}, {0, 12, 18, 25}, {0, 15, 22, 30}, {0, 18, 27, 35}};',
                    'kRetreatBandPct[4] = {20, 35, 50, 65};', 'inline constexpr unsigned kEvadeCooldownTicks = 90;', '  if (band == 0) return policy::kEvadeOrdinalPct[o - 1];',
                    '  return bh > bq ? bh : bq;  // min(H, Q) = the worse band', '  if (4 * num > 3 * den) return 0;', '  if (2 * num > den) return 1;', '  if (4 * num > den) return 2;',
                    '    const int chance = evade_pct(v.band, b + 1u);', '    if (used >= policy::kEvadeIframeTicks) {']
FIX09_PROVISIONAL = ['kEvadeBackReach', 'kRepoEvadeGapTicks', 'kPostHitHoldTicks']


def audit_fix09(p, raw, core, m):
    """0.13.18 FIX09: the approved numbers (6 i-frame ticks, Table B by band, Table A, the common 90-tick cooldown); the three new writers write exactly their own
    fields; the crossing callback only ever answers "no hit" for Link's melee on THIS admitted Darknut's hurt spheres; the native evasion is taken back in the POST
    before cc_set with mTimer7's pre-action value; the airborne maneuvers never guard; the front rule; the hit-end only from a confirmed receipt's own snapshot; the
    post-hit decision; the streams seeded per actor / generation; no new hook target, no shared file."""
    e = []
    for need in FIX09_CORE_NEEDS:
        if core.count(need) != 1:
            e.append('FIX09 approved number / rule changed: ' + need)
    for name in FIX09_PROVISIONAL:
        mm = re.search(r'inline constexpr [^;]*\b' + name + r'\b[^;]*;[^\n]*', core)
        if not mm or '[PROVISIONAL]' not in mm.group(0):
            e.append('FIX09 provisional value not marked as such: ' + name)
    if 'a->mTimer7 = v;' not in body(raw, 'DKN_NOINLINE void dkn_native_timer7(') or body(raw, 'DKN_NOINLINE void dkn_native_timer7(').count('=') != 1:
        e.append('FIX09: the timer7 writer writes more than mTimer7')
    ng = body(raw, 'DKN_NOINLINE void dkn_native_no_guard(')
    if ng.count('=') != 3 or 'a->field_0xa91 = false;' not in ng or 'a->mTimer10 = 0;' not in ng or 'a->mCutFlag = false;' not in ng:
        e.append('FIX09: the no-guard writer is not exactly field_0xa91 / mTimer10 / mCutFlag off')
    gr = body(raw, 'DKN_NOINLINE void dkn_native_guard_ready(')
    if gr.count('=') != 1 or 'a->field_0xa91 = on;' not in gr:
        e.append('FIX09: the front-rule writer writes more than field_0xa91')
    if raw.count('dkn_native_timer7(a, s.pa_t7)') != 1 or 'dkn_native_timer7(' in body(raw, 'HookAction on_dkn_action_pre('):
        e.append('FIX09: mTimer7 is written outside the POST restore of a native evasion entry (never 0 by default, never in the PRE)')
    xp = body(raw, 'void on_dkn_after_cross_post(ModContext*, void* args, void* ret, void*) noexcept {')
    xc = code(xp)
    writes = re.findall(r'(\b\w+(?:->|\.)[\w.\[\]>-]+|\*\w+)\s*(=|\+=|-=|\|=|&=)(?!=)', xc)
    bad = [w for w, _ in writes if not (w == '*r' or w.startswith('dn.') or w.startswith('s->st_') or w.startswith('s->evade.') or w.startswith('x.'))]
    if bad or xc.count('*r = true') != 1 or re.search(r'\*r\s*=\s*false', xc) or 's->ifh.note(' not in xc:
        e.append('FIX09: the crossing callback writes outside its own state / the "no hit" answer: %s' % sorted(set(bad))[:6])
    if 'if (*r) return;' not in xp or 'if (!sword && !sa) return;' not in xp or "if (!ac || fopAcM_GetName(ac) != fpcNm_B_TN_e) return;" not in xp \
            or 'if (!s || !dkn_combat(*s)) return;' not in xp or 'if (!hurt) return;' not in xp or 'if (v == dkn::Xv::Native) return;' not in xp:
        e.append('FIX09: the crossing callback is not limited to Link\'s melee on an admitted, combat-stage Darknut\'s hurt spheres')
    if 'inline Xv cross_verdict(const XIn& x) noexcept {' not in core or 'if (x.shield_attack && x.phase2 && !x.helm_route) return Xv::NoFalseGuard;' not in core:
        e.append('FIX09: the crossing verdict (window / same swing / rejected Shield Attack, Helm route native) changed')
    po = body(raw, 'void on_dkn_action_post(ModContext*, void* args, void*, void*) noexcept {')
    ne = body(raw, 'void dkn_native_evade_post(DknSlot& s, daB_TN_c* a, std::uint64_t now) noexcept {')
    if 'dkn_native_evade_post(s, a, now);' not in po or 'native_evade_written(s.pa_act, s.pa_mode, a->mActionMode1, a->mActionMode2)' not in ne or 's.pa_tick == now' not in ne \
            or 's.lease != DknLease::Evade' not in ne or 'dkn_native_neutral(a, true);' not in ne:
        e.append('FIX09: a native evasion written by the action() is not taken back in its own POST (before cc_set) from the PRE snapshot')
    if 's->pa_act = a->mActionMode1, s->pa_mode = a->mActionMode2, s->pa_t7 = a->mTimer7, s->pa_tick = dknb_tick();' not in body(raw, 'HookAction on_dkn_action_pre_fix09('):
        e.append('FIX09: the pre-action snapshot is not taken at the end of the PRE')
    if 'dkn_iframe_post(s, a, now);' not in po or po.index('dkn_iframe_post(s, a, now);') > po.index('dkn_at_authority(s, a);'):
        e.append('FIX09: the i-frame window is not counted in the POST of the executed tick')
    ipo = body(raw, 'void dkn_iframe_post(DknSlot& s, daB_TN_c* a, std::uint64_t now) noexcept {')
    if 'native_evade_entry(a->mActionMode1, a->mActionMode2) && a->speedF > 0.0f) s.evade_takeoff = true;' not in ipo or 's.ifr.tick(now, s.evade_takeoff, s.lease == DknLease::Evade)' not in ipo:
        e.append('FIX09: the i-frames are not opened by the native take-off of 6 / 11 / 14 only')
    st = body(raw, 'void dkn_evade_start(DknSlot& s, daB_TN_c* a, dkn::Ed dir, float dist, std::uint64_t now, bool retreat) noexcept {')
    if st.index('dkn_native_no_guard(a);') > st.index('dkn_native_jump(a, dkn::evade_mode(dir));') or 's.ifr.begin(++s.evade_token)' not in st or 's.evade.started(now, dir);' not in st:
        e.append('FIX09: a maneuver starts guard-ready / without its own i-frame token / outside the common cooldown')
    if 's.ifr.end();' not in body(raw, 'void dkn_evade_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why, bool cancel) noexcept {'):
        e.append('FIX09: the i-frames can outlive the maneuver')
    pre = body(raw, 'HookAction on_dkn_action_pre(')
    if pre.count('dkn_native_no_guard(a)') != 3 or 'if (s.leap_phase == Lp::Air) dkn_native_no_guard(a)' not in pre:
        e.append('FIX09: an airborne maneuver (evasion / leap in the air / reposition jump) can guard')
    dpre = body(raw, 'HookAction on_dkn_damage_pre(')
    if 'if (dkn::phase2(s.ph) && s.dc_sword && k) {' not in dpre or 'guard_needs_front_clear(true, a->field_0xa91,' not in dpre or 'dkn::policy::kFrontGuardArc' not in dpre:
        e.append('FIX09: the phase-2/3 front rule changed (a guard from the flank / back)')
    he = body(raw, 'void dkn_hit_end_apply(DknSlot& s, daB_TN_c* a, std::uint64_t now) noexcept {')
    if 'if (s.brk.st != Bk::Combat || s.exec || !dkn_combat(s)) {' not in he or 'h.offensive != s.off_id' not in he or he.index('s.brk.st != Bk::Combat') > he.index('hit_end_route('):
        e.append('FIX09: the hit-end can override a break / another offensive')
    if raw.count('s.hit_ending = true') != 2 or 's.hit_ending = true' in body(raw, 'void dkn_link_contact(') or raw.count('s.hit_pending = s.off') != 1 \
            or 'const bool snap = s.off.live && s.off.serial == token && s.off.generation == s.generation && !mace;' not in body(raw, 'void dkn_direct_maintenance() noexcept {') \
            or 'if (took < 0) {' not in body(raw, 'void dkn_direct_maintenance() noexcept {'):
        e.append('FIX09: the voluntary end is reachable without a CONFIRMED life receipt of the impact\'s own snapshot (offer / block / parry / unmatched offer)')
    if 'if (fresh_window && (s.hit_ending || (s.off.live && s.off.tick == dknb_tick()))) {' not in body(raw, 'void dkn_at_authority(DknSlot& s, daB_TN_c* a) noexcept {'):
        e.append('FIX09: a new impact window may open while the offensive ends / while its offer waits for the receipt')
    ph = body(raw, 'bool dkn_posthit_pre(DknSlot& s, daB_TN_c* a, daAlink_c* k, std::uint64_t now, bool p2, int act, int mode, float dist) noexcept {')
    if 'now >= s.evade.cooldown_until' not in ph or 'gb.g == Geo::Ok' not in ph or 'dkn_evade_start(s, a, Ed::Back, dist, now, true);' not in ph or 'dkn_native_jump' in ph:
        e.append('FIX09: the retreat is not the one native back jump through the common cooldown and its path check')
    adm = body(raw, 'DknSlot* dkn_admit(daB_TN_c* a)')
    if 's.evade.rng.seed(' not in adm or 's.pht.rng.seed(' not in adm:
        e.append('FIX09: the evasion / retreat streams are not seeded per actor / generation')
    if re.findall(r'DEFINE_HOOK\(&daB_TN_c::(\w+), (\w+)\);', m) != [('executeZakoEnding', 'DkrEnding'), ('_delete', 'DkrDelete')] + HOOKS:
        e.append('FIX09: a hook target changed (FIX09 adds none)')
    pol = json.loads((p / 'ci' / 'kk_policy.json').read_text(encoding='utf-8'))
    if any(h.get('dkn01_fix09') for v in pol['inverse_patch'].values() for h in v):
        e.append('FIX09 touches a shared file (it adds none)')
    return e


def audit_ldg(p, m):
    """0.13.16 FIX07 LDG01: Link's dodge i-frames. The adapter writes NO actor field: its only effect is `*r = true` (no hit) in the POST of the crossing commit,
    for an eligible enemy's AT on Link's own Tg cylinders inside a real dodge window (or the continuation of a contact already evaded), never during an event;
    the observer runs on the existing setCollision PRE target; all or nothing; the core includes only <cstdint>."""
    e = []
    for n in LDG_NEW:
        if not (p / 'src' / n).is_file():
            return ['LDG01 module missing: ' + n]
    raw = (p / 'src' / 'ldg_adapter.inc').read_text(encoding='utf-8')
    a = code(raw)
    writes = re.findall(r'(\b\w+(?:->|\.)[\w.\[\]>-]+|\*\w+)\s*(=|\+=|-=|\|=|&=)(?!=)', a)
    bad = [w for w, _ in writes if not (w.startswith('lq.') or w.startswith('lq ') or w == '*r' or w.startswith('s->st_ldg'))]
    if bad:
        e.append('LDG01 writes outside its own state / the return value: %s' % sorted(set(bad))[:6])
    if a.count('*r = true') != 2 or re.search(r'\*r\s*=\s*false', a):
        e.append('LDG01 changes the crossing answer outside its two "no hit" cases (or turns a native refusal into a hit)')
    post = body(raw, 'void on_ldg_after_cross_post(')
    if 'if (*r) return;' not in post or 'if (!link_tg) return;' not in post or 'if (!kind) return;' not in post or 'if (!cp04_world_safe()) return;' not in post \
            or post.index('if (!cp04_world_safe()) return;') > post.index('const ldg::Res v = lq.tr.verdict(pass, cls);'):
        e.append('LDG01: the veto is not limited to an eligible enemy on Link\'s Tg outside events')
    for tok in ('HOOK_SKIP_ORIGINAL', 'setDamage', 'mDamageTimer', 'getLife() =', 'setLife', 'procDamage', 'onNoResetFlg', 'offNoResetFlg', 'mProcID =', 'JKR_NEW', 'malloc('):
        if tok in a:
            e.append('LDG01 forbidden operation: ' + tok)
    inst = body(raw, 'void ldg_install()')
    if 'lq.enabled = r0 == MOD_OK && r1 == MOD_OK;' not in inst or 'add_pre<LinkCollisionFx>(on_ldg_collision_pre)' not in inst or 'add_post<LdgAfterCross>(on_ldg_after_cross_post)' not in inst:
        e.append('LDG01 install is not all or nothing on its two callbacks')
    core = (p / 'src' / 'ldg_core.hpp').read_text(encoding='utf-8')
    for inc in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]', core):
        if inc != 'cstdint':
            e.append('the LDG01 core includes a non-standard header: ' + inc)
    for need in ('inline constexpr unsigned kForwardTicks = 9;', 'inline constexpr unsigned kSideTicks = 6;', 'inline constexpr unsigned kBackTicks = 6;',
                 'bool protects() const noexcept { return on && eligible && age < window_of(dir); }', '{ return a == Atk::Sweep || (d == Dir::Forward && a == Atk::Thrust); }'):
        if need not in core:
            e.append('LDG01 approved number / rule changed: ' + need[:60])
    line = next((l for l in core.splitlines() if ' kCombatReach = ' in l), '')
    if '[PROVISIONAL]' not in line:
        e.append('LDG01 provisional value not marked as such: kCombatReach')
    if m.count('DEFINE_HOOK(&dCcS::ChkAtTgHitAfterCross, LdgAfterCross);') != 1 or m.count('  ldg_install();') != 1 or m.count('  ldg_shutdown();') != 1 \
            or m.count('#include "ldg_adapter.inc"') != 1 or m.count('#include "ldg_core.hpp"') != 1 or m.index('dkn_install();') > m.index('ldg_install();') \
            or m.index('ldg_shutdown();') > m.index('dkn_shutdown();'):
        e.append('LDG01 wiring in mod.cpp changed')
    return e


LGC_WRITERS = ['lgc_native_cut_end']


def audit_lgc(p, m):
    """0.13.14 FIX05 LGC01: Link's guard cancel. One hook target (procCutNormal PRE), one writer (the cut's own native end), no input write, no other Link
    write, fail-closed context, the manual shield's decide_bash reads the buffered R+B only inside the cancel call; the pure core includes only <cstdint>."""
    e = []
    for n in LGC_NEW:
        if not (p / 'src' / n).is_file():
            e.append('LGC01 module missing: ' + n)
    if e:
        return e
    raw = (p / 'src' / 'lgc_adapter.inc').read_text(encoding='utf-8')
    a = code(raw)
    if len(re.findall(r'^LGC_NOINLINE ', raw, flags=re.M)) != 1 or 'LGC_NOINLINE int lgc_native_cut_end(daAlink_c* k) noexcept {\n  k->resetCombo(1);\n  return k->checkNextAction(0);\n}' not in raw:
        e.append('LGC01 writer inventory is not exactly lgc_native_cut_end = resetCombo(TRUE) + checkNextAction(0)')
    outside = raw.replace(body(raw, 'LGC_NOINLINE int lgc_native_cut_end('), '', 1)
    o = code(outside)
    if re.search(r'\bk->[\w.\[\]]+\s*(=|\+=|-=|\|=|&=)(?!=)|\bk->(proc\w*|set\w*|on\w*|off\w*|reset\w*|checkNextAction|commonProcInit)\(|mItemTrigger|mItemButton|->mProcID\s*=', o):
        e.append('LGC01 writes / drives Link outside its one writer (or touches the input)')
    hp = body(raw, 'HookAction on_lgc_cut_normal_pre(')
    if a.count('HOOK_SKIP_ORIGINAL') != 2 or hp.count('HOOK_SKIP_ORIGINAL') != 2 or hp.count('*static_cast<int*>(ret) = 1;') != 2:
        e.append('LGC01 skips the original outside its two verdicts (HOLD / CANCEL) or without the procedure return value')
    for need in ('v.safe = sample && cp04_link_safe(k);', 'v.sword = k->mEquipItem == 0x103;', 'v.ground = k->mLinkAcch.ChkGroundHit();',
                 'v.lock = v.r_held && k->checkGuardActionChange() != 0;', 'v.cut_normal = static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_NORMAL;',
                 'lgc_rt.bash_now = o.bash;\n    const int r = lgc_native_cut_end(k);\n    lgc_rt.bash_now = false;', 'lgc_rt.enabled = lgc_rt.hooks && lf.enabled && c4.enabled;'):
        if need not in raw:
            e.append('LGC01 gate / scope lost: ' + need.split('\n')[0])
    core = (p / 'src' / 'lgc_core.hpp').read_text(encoding='utf-8')
    for inc in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]', core):
        if inc != 'cstdint':
            e.append('the LGC01 core includes a non-standard header: ' + inc)
    for need in ('inline bool ordinary_cut(int cut) noexcept { return cut == kCutNmVertical || cut == kCutNmStab || cut == kCutNmRight || cut == kCutNmLeft || cut == kCutComboStab; }',
                 'if (o.region == Region::Active) return ++buffered, o.v = Verdict::Buffer, o;', 'if (v.r_held && v.r_down_tick >= cut_tick && !intent)'):
        if need not in core:
            e.append('LGC01 core rule lost: ' + need[:60])
    for name in ('kBashBufferTicks',):
        line = next((l for l in core.splitlines() if ' ' + name + ' = ' in l), '')
        if '[PROVISIONAL]' not in line:
            e.append('LGC01 provisional value not marked as such: ' + name)
    if m.count('DEFINE_HOOK(&daAlink_c::procCutNormal, LinkCutNormalLgc);') != 1 or m.count('  lgc_install();') != 1 or m.count('  lgc_shutdown();') != 1 \
            or m.count('#include "lgc_adapter.inc"') != 1 or m.count('lnkfx::decide_bash(lf.r_held, lf.b_trig || lgc_bash_now(), lf.def.phase());') != 1 \
            or m.count('lgc_bash_now()') != 2 or m.index('dkn_install();') > m.index('lgc_install();'):
        e.append('LGC01 wiring in mod.cpp changed')
    return e


NEGATIVES = [
    ('src/dkn_core.hpp', 'kHp1Max = 600;', 'kHp1Max = 420;'),
    ('src/dkn_core.hpp', 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;'),
    ('src/dkn_adapter.inc', '      kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);'.replace('      ', '  '), '  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);\n  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);'),
    ('src/dkn_adapter.inc', '    } else if (rb == 0) {\n      *r = FALSE;', '    } else if (rb == 0) {\n      (void)r;'),
    ('src/dkn_adapter.inc', '    if (!s.on || !dkn_combat(s)) continue;\n    fopAc_ac_c* p = nullptr;\n    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;\n    auto* a = static_cast<daB_TN_c*>(p);\n    bool here',
     '    if (!s.on || !dkn_combat(s) || !dkn::phase2(s.ph)) continue;\n    fopAc_ac_c* p = nullptr;\n    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;\n    auto* a = static_cast<daB_TN_c*>(p);\n    bool here'),
    ('src/dkn_adapter.inc', '      if (gn.confirmed) ++s.st_parry_gain;', '      if (gn.confirmed) ++s.st_parry_gain;\n      (void)dknb_link_gain(pts, false, s.id, s.generation, 0);'),
    ('src/dkn_adapter.inc', '      pfx_settle(pfx::Kind::DarknutOnLink, s.id, true);  // the orange spark', '      (void)dknb_link_gain(25, false, s.id, s.generation, 0);  // the orange spark'),
    ('src/dkn_adapter.inc', '    const bool route = k == dknb_link() && dkn_link_reverse_route(k);', '    const bool route = true;'),
    ('src/dkn_adapter.inc', '  if ((s.def.window_open(now) || s.def.parry_hold(now)) && front && shield_ok) dkn_native_guard(a, 2);', '  if (s.def.window_open(now) && front && shield_ok) dkn_native_guard(a, 2);'),
    ('src/dkn_adapter.inc', '  iv.defending = hold;', '  iv.defending = false;'),
    ('src/dkn_adapter.inc', '  const bool neutral = !def_busy && !hold &&', '  const bool neutral = !def_busy &&'),
    ('src/parry_fx_adapter.inc', '  pf.exec_kind = pfx::Kind::DarknutOnLink, pf.exec_map = 42,', '  pf.exec_kind = pfx::Kind::DarknutOnLink, pf.exec_map = 40,'),
    ('src/parry_fx_adapter.inc', 'fopAcM_GetName(ta) == fpcNm_B_TN_e && pfx_link_sword(k, at)', 'pfx_link_sword(k, at)'),
    ('src/dkn_core.hpp', '      d.pick = Pick::Single, d.idx = v.phase2 ? kIdxF2Thrust :', '      d.pick = Pick::Single, d.idx = v.phase2 ? kIdxF2Thrust + rng.below(2) :'),
    ('src/dkn_core.hpp', 'inline constexpr int kHelmClip = native::kBckTnbDamageR;', 'inline constexpr int kHelmClip = native::kBckTnbDown;'),
    ('src/dkn_adapter.inc', '    const bool entry = native::native_attack_entry(a->mActionMode1, a->mActionMode2);', '    const bool entry = true;'),
    ('src/dkn_core.hpp', 'inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'inline constexpr int kParryOrdinalPct[6] = {10, 25, 45, 60, 60, 60};'),
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
    ('mod.json', '"version": "0.13.18"', '"version": "0.13.17"'),
    # FIX05
    ('src/dkn_adapter.inc', '  dkn_native_at(a, true, 0, true);', '  dkn_native_at(a, true, 0);'),
    ('src/dkn_adapter.inc', 'if (in_hand && !a->mCps.ChkAtSet())', 'if (in_hand && !at_on)'),
    ('src/dkn_core.hpp', 'inline constexpr bool kF2Singles = false;', 'inline constexpr bool kF2Singles = true;'),
    ('src/dkn_adapter.inc', '    if (s.def.on_contact(ck)) {  // FIX04: a REAL', '    if (dkn_owned_move(s)) dkn_move_end(s, a, "LINK_BLOCKED", true);\n    if (s.def.on_contact(ck)) {  // FIX04: a REAL'),
    ('src/dkn_adapter.inc', '      dkn_log("PARRY", s, a, ex);', '      dkn_log("PARRY", s, a, ex);\n      dkn_move_end(s, a, "NATIVE_END", true);'),
    ('src/dkn_adapter.inc', '    if (pv != Pv::None && !special_allowed(pv, p2)) {', '    if (false) {'),
    ('src/dkn_adapter.inc', '    const bool late = s.def.register_at_contact(now, ck);', '    const bool late = false;'),
    ('src/dkn_core.hpp', '  if (!(dist < policy::hold_reach(phase2))) return Hold::Far;', '  if (!(dist < policy::kGuardReach)) return Hold::Far;'),
    ('src/dkn_adapter.inc', '    const Rb rb = s.def.observe_rebound(now, rev, rf, p2, shield_ok);', '    const Rb rb = s.def.confirm(now, rev, p2, shield_ok) ? Rb::Confirmed : Rb::None;'),
    ('src/dkn_core.hpp', '    if (idx == kIdxF2Double && c.phase2 && !(scale > 0.0f)) return plan_counter_double();', '    if (false) return plan_counter_double();'),
    ('src/dkn_adapter.inc', '    dkn_transform_post(*sp, a);', '    (void)0;'),
    ('src/lgc_adapter.inc', '  k->resetCombo(1);\n  return k->checkNextAction(0);', '  return k->checkNextAction(0);'),
    ('src/lgc_adapter.inc', '    v.lock = v.r_held && k->checkGuardActionChange() != 0;', '    v.lock = v.r_held;'),
    ('src/lgc_adapter.inc', '    v.safe = sample && cp04_link_safe(k);', '    v.safe = sample;'),
    ('src/lgc_adapter.inc', '    const bool sample = lf.sample_ok', '    k->mItemTrigger = 0;\n    const bool sample = lf.sample_ok'),
    ('src/lgc_core.hpp', '    if (o.region == Region::Active) return ++buffered, o.v = Verdict::Buffer, o;', '    if (false) return ++buffered, o.v = Verdict::Buffer, o;'),
    ('src/mod.cpp', 'lnkfx::decide_bash(lf.r_held, lf.b_trig || lgc_bash_now(), lf.def.phase());', 'lnkfx::decide_bash(lf.r_held, true || lgc_bash_now(), lf.def.phase());'),
    ('src/dkn_adapter.inc', '        if (p2) dkn_native_guard_enter(a, dkn_guard_bck(s, k), dkn_guard_facing(a, k));', '        if (p2) dkn_native_action(a, native::kActGuardL, 0);'),
    ('src/dkn_adapter.inc', '  if (k->mNormalSpeed > v) k->mNormalSpeed = v;', '  k->mNormalSpeed = v;'),
    ('src/dkn_adapter.inc', '  if (dkn::native::needs_hit_context(act, mode)) {\n    ++dn.unsafe_entries;', '  if (false) {\n    ++dn.unsafe_entries;'),
    # FIX06
    ('src/dkn_core.hpp', '    {7, 1, 25.5f},', '    {7, 1, 33.18f},'),
    ('src/dkn_core.hpp', 'inline constexpr int kParryComboPct = 60;', 'inline constexpr int kParryComboPct = 92;'),
    ('src/dkn_core.hpp', 'inline constexpr int kCounterComboPct = 80;', 'inline constexpr int kCounterComboPct = 100;'),
    ('src/dkn_core.hpp', 'inline constexpr unsigned kBridgeHoldMax = 60;        // [PROVISIONAL]', 'inline constexpr unsigned kBridgeHoldMax = 60;        //'),
    ('src/dkn_core.hpp', '  bool link_ok = true, link_broken = false, safe = true, link_held = false;', '  bool link_ok = true, link_broken = false, safe = true, link_held = false, link_cutting = false;'),
    ('src/dkn_adapter.inc', '      s.def.routes = true;', '      s.def.routes = false;'),
    ('src/dkn_adapter.inc', '    if (s.seq.owns()) return HOOK_CONTINUE;\n  }', '  }'),
    ('src/dkn_adapter.inc', '    gi.link_ok = k && dknb_link_alive(), gi.link_broken = dknb_link_broken(), gi.safe = dknb_world_safe(), gi.link_held = classify(lp) == Act::Hold;',
     '    gi.link_ok = k && dknb_link_alive(), gi.link_broken = dknb_link_broken(), gi.safe = dknb_world_safe(), gi.link_held = classify(lp) == Act::Hold || dkn_link_cutting(lp);'),
    ('src/dkn_adapter.inc', '  dkn_seq_end(s, a, SqEnd::Break, "POSTURE_BREAK");', '  (void)0;'),
    ('src/dkn_adapter.inc', '  if (s.def.cp == dkn::Cp::Scheduled) s.def.cp = dkn::Cp::None, ++s.def.counter_dropped;', '  (void)0;'),
    ('src/dkn_adapter.inc', '  if (s.lease != DknLease::None || s.brk.active() || s.seq.owns()) {', '  if (s.lease != DknLease::None || s.brk.active()) {'),
    ('src/dkn_adapter.inc', '      s.hp1 = f1_exec_hp(s.hp1);', '      s.hp1 = f1_exec_hp(s.hp1 - dmg);'),
    ('src/dkn_adapter.inc', '      dkn_native_rate(a, helm_rate_at(s.brk.b_ticks, dkn_frame(a)));', '      dkn_native_rate(a, dkn_frame(a) < kHelmHold ? 1.0f : kHoldCreep);'),
    ('src/dkn_adapter.inc', '    if (landing_attack && !s.repo_back) {\n      dkn_native_jump_land(a);', '    if (false) {\n      dkn_native_jump_land(a);'),
    ('src/dkn_adapter.inc', '  if (!s || !dkn::sword_tint(s->ph == dkn::Ph::Dead, a->mMaterialColor)) return HOOK_CONTINUE;', '  if (!s || a->mMaterialColor == 0.0f) return HOOK_CONTINUE;'),
    ('src/dkn_adapter.inc', '    if (mat && mat->getTevColor(0)) dkn_native_sword_color(mat, dn.tint_saved[i][0], dn.tint_saved[i][1], dn.tint_saved[i][2]);', '    (void)mat;'),
    ('src/dkn_adapter.inc', '    s.repo_pending = true, s.repo_since = dknb_tick(), s.repo_turn_since = 0;  // FIX06: ONE reposition decision for this completed resurrection', '    (void)0;'),
    ('src/mod.cpp', 'DEFINE_HOOK(&daB_TN_c::draw, DknDraw);', '// (no draw hook)'),
    # FIX07
    ('src/dkn_core.hpp', 'inline constexpr int kF1ExecDamage = 120;', 'inline constexpr int kF1ExecDamage = 100;'),
    ('src/dkn_core.hpp', 'kF1C3Targets[4] = {36.0f, 56.0f, 57.0f, 40.0f};', 'kF1C3Targets[4] = {36.0f, 56.0f, 57.0f, 41.0f};'),
    ('src/dkn_core.hpp', 'inline constexpr int kLeapPct = 90;', 'inline constexpr int kLeapPct = 80;'),
    ('src/dkn_core.hpp', 'inline constexpr int kMaceBlockLink = 50, kMaceDirectLink = 60;', 'inline constexpr int kMaceBlockLink = 50, kMaceDirectLink = 50;'),
    ('src/dkn_core.hpp', 'inline constexpr float kLeapMaxDrop = 80.0f;     // [PROVISIONAL]', 'inline constexpr float kLeapMaxDrop = 80.0f;     //'),
    ('src/dkn_core.hpp', '    const float rmax = f1c3 && i == 0 ? kF1C3Pair1RateMax : kF1RecoveryRateMax;', '    const float rmax = kF1C3Pair1RateMax;'),
    ('src/dkn_adapter.inc', '  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;',
     '  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;\n  dkn_native_sword_color(nullptr, 0, 0, 0);'),
    ('src/dkn_adapter.inc', '  if (!dn.tint_morf || !args || mods::arg<const mDoExt_McaMorfSO*>(args, 0) != dn.tint_morf) return HOOK_CONTINUE;', '  if (!dn.tint_morf || !args) return HOOK_CONTINUE;'),
    ('src/dkn_adapter.inc', '  dkn_tint_restore();\n  ++dn.tint_entry_restores;', '  ++dn.tint_entry_restores;'),
    ('src/mod.cpp', 'DEFINE_HOOK(&mDoExt_McaMorfSO::entryDL, DknMorfEntryDL);', '// (no entryDL hook)'),
    ('src/dkn_adapter.inc', '    gv.phase2 = p2, gv.free = neutral && !def_busy && !hold && now >= s.ini.reentry_until && !s.repo_pending && !evade_hold && !reacting && !s.pht.pending && now >= s.ph_hold_until;  // FIX09', '    gv.phase2 = p2, gv.free = true;'),
    ('src/dkn_adapter.inc', '        if (g.g != Geo::Ok) {  // the roll stays consumed', '        if (false) {  // the roll stays consumed'),
    ('src/dkn_adapter.inc', '      if (v0 == Rp::Back) gb = dkn_geo(a, dkn_dir_to(a, k, true), leap_model(Leap::Back).dist);', '      (void)0;'),
    ('src/dkn_adapter.inc', '      s.gap.rng.seed(', '      s.ini.rng.seed('),
    ('src/dkn_adapter.inc', '    if (s.leap_phase == Lp::Air && entry) {\n      dkn_native_jump_land(a);', '    if (false) {\n      dkn_native_jump_land(a);'),
    ('src/dkn_adapter.inc', '  if (s.lease == DknLease::Special || s.lease == DknLease::Reposition || s.lease == DknLease::Leap || s.lease == DknLease::Evade) s.lease = DknLease::None;',
     '  if (s.lease == DknLease::Special || s.lease == DknLease::Reposition || s.lease == DknLease::Evade) s.lease = DknLease::None;'),
    ('src/dkn_adapter.inc', '  if (!s.mace.resolve(m)) {', '  if (!s.mace.resolve(m) && false) {'),
    ('src/dkn_adapter.inc', '  const bool anchor = a->mStates[13] == 3 && a->mStates[15] != 1;', '  const bool anchor = a->mStates[13] == 3;'),
    ('src/dkn_adapter.inc', '  const LpStep st = leap_ground(dist, dkn_front(a, k), s.leap_ground, total);', '  const LpStep st = leap_ground(dist, dkn_front(a, k) || s.swing_new, s.leap_ground, total);'),
    ('src/ldg_adapter.inc', '  if (!link_tg) return;  // every crossing', '  if (false) return;  // every crossing'),
    ('src/ldg_adapter.inc', '  if (!cp04_world_safe()) return;  // an event / cutscene: never protected', '  (void)0;'),
    ('src/ldg_adapter.inc', '  if (*r) return;  // the native already answered "no hit"', '  if (*r) *r = false;'),
    ('src/ldg_adapter.inc', '  if (!kind) return;  // hazards', '  if (!kind) k->mDamageTimer = 30;  // hazards'),
    ('src/ldg_adapter.inc', '  lq.enabled = r0 == MOD_OK && r1 == MOD_OK;', '  lq.enabled = true;'),
    ('src/ldg_core.hpp', 'inline constexpr unsigned kForwardTicks = 9;', 'inline constexpr unsigned kForwardTicks = 12;'),
    ('src/ldg_core.hpp', '#include <cstdint>\n', '#include <cstdint>\n#include <vector>\n'),
    ('src/mod.cpp', 'DEFINE_HOOK(&dCcS::ChkAtTgHitAfterCross, LdgAfterCross);', '// (no crossing hook)'),
    # FIX08
    ('src/dkn_core.hpp', '{12, 0}, 24, 36, 8.0f},', '{12, 0}, 20, 30, 8.0f},'),
    ('src/dkn_core.hpp', '{14, 0}, 50, 60, 3.0f},', '{14, 0}, 42, 50, 3.0f},'),
    ('src/dkn_core.hpp', 'kEvadeOrdinalPct[6] = {0, 10, 15, 20, 20, 20};', 'kEvadeOrdinalPct[6] = {0, 20, 20, 20, 20, 20};'),
    ('src/dkn_core.hpp', 'inline constexpr unsigned kEvadeCooldownTicks = 90;', 'inline constexpr unsigned kEvadeCooldownTicks = 45;'),
    ('src/dkn_core.hpp', 'inline constexpr float kF1C3Pair1RateMax = kF1RecoveryRateMax;', 'inline constexpr float kF1C3Pair1RateMax = 1.76f;'),
    ('src/dkn_core.hpp', 'inline constexpr unsigned kHelmLookDelayTicks = 6;   // [PROVISIONAL]', 'inline constexpr unsigned kHelmLookDelayTicks = 6;   //'),
    ('src/dkn_adapter.inc', '    if (v != Dv::Parry && p2 && k) {', '    if (p2 && k) {'),
    ('src/dkn_adapter.inc', '        const Ed dir = evade_pick_band(ev.band, dist, side, (s.def.cur_key & 1) != 0, gl.g == Geo::Ok, gr.g == Geo::Ok, gb.g == Geo::Ok);',
     '        const Ed dir = evade_pick_band(ev.band, dist, side, (s.def.cur_key & 1) != 0, true, true, true);'),
    ('src/dkn_adapter.inc', '  if (s.lease == DknLease::None && p2 && evade_native_entry_any(act0, mode0)) {', '  if (false && s.lease == DknLease::None && p2 && evade_native_entry_any(act0, mode0)) {'),
    ('src/dkn_adapter.inc', 'iv.neutral = neutral && gate_ok', 'iv.neutral = neutral || from_native'),
    ('src/dkn_adapter.inc', '  iv.link_swing = link_swing;', '  (void)link_swing;'),
    ('src/dkn_adapter.inc', '    s.ngate.defense(now, 2);  // FIX08: an ordinary block', '    // FIX08: an ordinary block'),
    ('src/dkn_adapter.inc', '      dkn_native_neck(a, true), s.neck_on = true;', '      s.neck_on = true;'),
    ('src/dkn_adapter.inc', '  if (s.neck_on) dkn_native_neck(a, false), s.neck_on = false;\n  const int err', '  const int err'),
    ('src/dkn_adapter.inc', '      dkn_native_recover_clip(a, policy::kB1RecoverBck, policy::kB1RecoverMorph);\n      dkn_native_timer1(a, policy::kB1RecoverTicks);\n      ++s.st_b1_recover;',
     '      dkn_native_wait_clip(a);\n      dkn_native_timer1(a, policy::kB1RecoverTicks);\n      ++s.st_b1_recover;'),
    ('src/dkn_adapter.inc', '      if (helm1) {  // FIX08-01', '      if (true) {  // FIX08-01'),
    ('src/dkn_adapter.inc', 'DKN_NOINLINE void dkn_native_neck(daB_TN_c* a, bool on) noexcept { a->mUpdateNeckAngle = on; }',
     'DKN_NOINLINE void dkn_native_neck(daB_TN_c* a, bool on) noexcept { a->mUpdateNeckAngle = on, a->mUpdateWaistAngle = on; }'),
    ('src/dkn_adapter.inc', '  if (!s || !dkn::sword_tint(s->ph == dkn::Ph::Dead, a->mMaterialColor)) return HOOK_CONTINUE;', '  if (!s || !dkn::sword_tint(true, a->mMaterialColor)) return HOOK_CONTINUE;'),
    # FIX09
    ('src/dkn_core.hpp', 'inline constexpr unsigned kEvadeIframeTicks = 6;', 'inline constexpr unsigned kEvadeIframeTicks = 10;'),
    ('src/dkn_core.hpp', 'kRetreatBandPct[4] = {20, 35, 50, 65};', 'kRetreatBandPct[4] = {20, 35, 50, 80};'),
    ('src/dkn_core.hpp', '{0, 18, 27, 35}}', '{0, 18, 27, 40}}'),
    ('src/dkn_core.hpp', 'inline constexpr unsigned kRepoEvadeGapTicks = 90;   // [PROVISIONAL]', 'inline constexpr unsigned kRepoEvadeGapTicks = 90;   //'),
    ('src/dkn_adapter.inc', 'DKN_NOINLINE void dkn_native_timer7(daB_TN_c* a, int v) noexcept { a->mTimer7 = v; }', 'DKN_NOINLINE void dkn_native_timer7(daB_TN_c* a, int v) noexcept { a->mTimer7 = v, a->mInvincibilityTimer = 0; }'),
    ('src/dkn_adapter.inc', '  a->mCutFlag = false;\n}\n// 27 (FIX09-01)', '  a->mCutFlag = false;\n  a->mInvincibilityTimer = 6;\n}\n// 27 (FIX09-01)'),
    ('src/dkn_adapter.inc', '  *r = true;\n  if (v == dkn::Xv::Evaded || v == dkn::Xv::EvadedHeld) {', '  *r = true;\n  a->mInvincibilityTimer = 1;\n  if (v == dkn::Xv::Evaded || v == dkn::Xv::EvadedHeld) {'),
    ('src/dkn_adapter.inc', '  if (!sword && !sa) return;  // every other crossing of the game: pointer comparisons only', '  (void)sa;'),
    ('src/dkn_adapter.inc', '  dkn_native_evade_post(s, a, now);  // FIX09-01', '  // FIX09-01'),
    ('src/dkn_adapter.inc', '    if (s.leap_phase == Lp::Air) dkn_native_no_guard(a), ++s.st_no_guard;  // FIX09: airborne: never guard-ready', '    // FIX09: airborne: never guard-ready'),
    ('src/dkn_adapter.inc', '  dkn_native_no_guard(a);\n  dkn_native_jump(a, dkn::evade_mode(dir));', '  dkn_native_jump(a, dkn::evade_mode(dir));'),
    ('src/dkn_adapter.inc', '  if (dkn::phase2(s.ph) && s.dc_sword && k) {', '  if (false && dkn::phase2(s.ph) && s.dc_sword && k) {'),
    ('src/dkn_adapter.inc', '  } else if (h.offensive != s.off_id) {', '  } else if (false) {'),
    ('src/dkn_adapter.inc', '      s.hit_pending = s.off, s.off.live = false, s.ph_receipt = now, s.ph_free_armed = false;', '      s.hit_pending = s.off, s.off.live = false, s.ph_receipt = now, s.ph_free_armed = false;\n      s.hit_ending = true;'),
    ('src/dkn_adapter.inc', '    if (fresh_window && (s.hit_ending || (s.off.live && s.off.tick == dknb_tick()))) {', '    if (fresh_window && s.hit_ending) {'),
    ('src/dkn_adapter.inc', '                        now >= s.evade.cooldown_until && !repo_gap && !s.def.busy() && !s.seq.owns();', '                        !repo_gap && !s.def.busy() && !s.seq.owns();'),
    ('src/dkn_adapter.inc', '      s.evade.rng.seed(', '      s.gap.rng.seed('),
    ('src/dkn_adapter.inc', '  s.ifr.end();  // FIX09: the i-frames never outlive the maneuver\n', ''),
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
