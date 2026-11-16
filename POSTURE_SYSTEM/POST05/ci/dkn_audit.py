"""LIGHT FADES TWICE 0.13.9 DARKNUT MINIBOSS (DKN01) + 0.13.10 FIX01 + 0.13.11 FIX02 + 0.13.12 FIX03 + 0.13.13 FIX04 + 0.13.14 FIX05: the OUTERMOST layer of the source wall.
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
NEW = {'dkn_core.hpp', 'dkn_bridge.inc', 'dkn_adapter.inc'} | LGC_NEW
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
CORE_NEEDS = ['kHp1Max = 600;', 'kRes2Max = 480;', 'kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;', 'kRegenDelayTicks = 60;', 'kRegen1[5] = {10, 7, 4, 2, 1};',
              'kRegen2[5] = {12, 8, 5, 2, 1};', 'kBreakTicks = 90;', 'kDownTicks = 180;', 'kLinkMax = 100;', 'kComboPick[4] = {25, 25, 25, 25};',
              'kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};', 'kLinkEnemyParry1 = 20, kLinkEnemyParry2 = 25;', 'kF2ComboScale = 1.40f;', 'kF2SingleScale = 1.34f;', 'kKickScale = 1.25f;', 'kGainSwordHp1 = 6, kGainBlocked1 = 4, kGainIron1 = 25;',
              'kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;', 'kPassiveShieldTicks = 38;', 'kPassiveIdleTicks = 75;']
PROVISIONAL = ['kSinglePct', 'kGap1Min', 'kGap2Min', 'kReentryTicks', 'kGuardLeaveTicks', 'kEngage1', 'kPassiveCooldownTicks', 'kPressureWindow', 'kGuardPctBase',
               'kStreakGapTicks', 'kParryPrepTicks', 'kParryWindowTicks', 'kParryCooldownTicks', 'kFinisherExtMax', 'kHelmWindowTicks', 'kSlipChainTicks', 'kChainWhiffs',
               'kCounterLandTicks', 'kCounterLateTicks']
FIX04_SHARED = {'parry_fx_core.hpp': 2, 'parry_fx_adapter.inc': 6}  # the exact counted dkn01_fix04 hunks (Darknut kinds, previews, exec scope, summary)
FIX05_SHARED = {'mod.cpp': 9, 'parry_fx_core.hpp': 4, 'parry_fx_adapter.inc': 6}  # the exact counted dkn01_fix05 hunks (LGC01 wiring; the Darknut FX transaction)


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
    mine = [h for h in pol['inverse_patch'].get('mod.cpp', []) if h.get('dkn01') or h.get('dkn01_fix05')]
    if sum(1 for h in mine if h.get('dkn01')) != 7 or sum(1 for h in mine if h.get('dkn01_fix05')) != FIX05_SHARED['mod.cpp']:
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
    if 'const bool route = k == dknb_link() && dkn_link_reverse_route(k);' not in rv or '? s.def.rebound(now) : 0;' not in rv or '} else if (rb == 0) {\n      *r = FALSE;' not in rv \
            or 'phase2(s.ph)) continue' in rv or 'dknb_link_gain(' in rv:
        e.append('the rebound verdict is not: armed window + a PROC_CUT_REVERSE route = rebound kept (no credit here), otherwise no rebound, in EVERY phase (FIX04)')
    pre = body(raw, 'HookAction on_dkn_action_pre(')
    anchor = 'const Rb rb = s.def.observe_rebound(now, rev, rf, p2, shield_ok);'
    cf = pre[pre.index(anchor):] if anchor in pre else ''
    if not cf or a.count('dknb_link_gain(') != 3 or 'dknb_link_gain(' not in body(cf, 'if (rb == Rb::Confirmed) {') or pre.count('dknb_link_gain(') != 1 \
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
    ends = sorted(set(re.findall(r'dkn_move_end\(s, a, "(\w+)"', raw)) | set(re.findall(r'"(NO_TARGET|LINK_GUARD_BROKEN|EVENT|LINK_FAR)"', body(raw, 'void dkn_combo_post('))))
    if ends != sorted(['ACTION_LOST', 'B1_RECOVERY', 'DEFEAT', 'EVENT', 'LINK_FAR', 'LINK_GUARD_BROKEN', 'NATIVE_END', 'NO_TARGET', 'PHASE_CHANGE', 'POSTURE_BREAK', 'WATCHDOG']) \
            or len(re.findall(r'dkn_move_end\(', raw)) != 12 or 'dkn_move_end(' in body(raw, 'void dkn_damage_post_body(void* args) noexcept {') \
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
    if 'inline constexpr int kHelmClip = native::kBckTnbDamageR;' not in core0 or 'dkn_native_clip(a, kHelmClip, kHelmMorph, kHelmStart, 1.0f);' not in raw:
        e.append('FIX03: the second stagger is not TNB_DAMAGE_R (the TNB_DOWN hold reads as a guard)')
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
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.14':
        e.append('mod id / version')
    e += audit_lgc(p, m)
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
    ('mod.json', '"version": "0.13.14"', '"version": "0.13.13"'),
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
