"""LIGHT FADES TWICE 0.13.0 CHILFOS01: source rules of the Chilfos module, each proven by negatives (the rule PASSES on the tree and FAILS on every
mutated copy). Usage: test_kk_source.py  (run from anywhere; reads POSTURE_SYSTEM/POST05)."""
import hashlib, json, re, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
NAMES = ('src/kk_core.hpp', 'src/kk_adapter.inc', 'src/kk_bridge.inc', 'src/kk_pfx.inc', 'src/mod.cpp', 'src/parry_fx_core.hpp', 'src/parry_fx_adapter.inc',
         'src/liz_core.hpp', 'src/liz_adapter.inc', 'src/lnk_core.hpp', 'src/parry_core.hpp', 'src/shield_chain_core.hpp', 'src/link_break_core.hpp', 'mod.json')
SRC = {n: (root / n).read_text(encoding='utf-8') for n in NAMES}
# FIX05 0.12.5 (compiled eade22d6b94d3dc3cfcad94812463aed76c18e5d): the Lizalfos and the shared Link systems are byte-identical.
FROZEN = {
    'src/liz_core.hpp': '2ab38be42d8e671586ef72861784a4ee68894518e6c21ab3cfe02b5c119b4351',
    'src/liz_adapter.inc': '56f3773224c8e92c5bc505b17be86f728647fc6a503bded95aa570311eef600d',
    'src/lnk_core.hpp': 'dd19de60db69cf13e248cd532ddc608dbb0569fc8b7f617e209bed3d829f2fc6',
    'src/parry_core.hpp': '457f1871f3d15e668b4606fac6f11e4cafe76546840951b492e8233b0c77ed32',
    'src/shield_chain_core.hpp': '91c0ae587ca495faca450916f7fc18cc606da77f48f28fceac7169b9659bb3a3',
    'src/link_break_core.hpp': '8eadaf197ffeeebf204ca3ce1c215e1ae6a50c7aea36899ced4ec969b34596fe',
}
KK = ('src/kk_core.hpp', 'src/kk_adapter.inc', 'src/kk_bridge.inc', 'src/kk_pfx.inc')


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


POLICY = json.loads((root / 'ci/kk_policy.json').read_text(encoding='utf-8'))


def fix05_text(n, t):  # LFT 0.13.5: liz_adapter.inc minus the exact counted Jump Attack hunks of ci/kk_policy.json must be FIX05 byte for byte
    # LFT 0.13.6: liz_core.hpp too (FIX06 Jump Strike credit, guard-entry cover, jump/Back Slice streak floors, counter -> combo chain), same exact counted hunks
    for h in reversed(POLICY['inverse_patch'].get(n.split('/')[-1], []) if n in ('src/liz_adapter.inc', 'src/liz_core.hpp') else []):
        if t.count(h['after']) != 1:
            return None
        t = t.replace(h['after'], h['before'], 1)
    return t


def rules(s):
    e = []
    for n, h in FROZEN.items():
        t = fix05_text(n, s[n])
        if t is None or hashlib.sha256(t.encode('utf-8')).hexdigest() != h:
            e.append('Lizalfos/shared file changed vs FIX05: ' + n)
    kk = {n: code(s[n]) for n in KK}
    a = kk['src/kk_adapter.inc']
    allk = ''.join(kk.values())
    # 1. Link is never driven: no procedure, demo mode or Link field written by this module
    if re.search(r'mProcID\s*=(?!=)|setProc\w*\(|changeDemoMode|->mProcVar\w*\s*=(?!=)|mNormalSpeed\s*=(?!=)', allk):
        e.append('the Chilfos module drives Link')
    # 1b. FIX03: Link's native Mortal Draw entry is the ONLY proc init, once, in its KK_NOINLINE writer, called once from the checkCutAction POST
    inits = re.findall(r'procCut\w*Init\(([^)]*)\)', allk)
    if inits != ['type'] or 'KK_NOINLINE int kk_native_mortal_draw(daAlink_c* k, int type) noexcept { return k->procCutFinishInit(type); }' not in s['src/kk_adapter.inc'] \
            or a.count('kk_native_mortal_draw(') != 2 or 'kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);' not in body(s['src/kk_adapter.inc'], 'void on_kk_cut_action_post('):
        e.append('Link proc init outside the single audited Mortal Draw entry: %s' % inits)
    # 2. no Lizalfos code copied into the adapter; the bridge alone reads the two shared fields of the D1->D2 re-arm
    if re.search(r'\blz\.|liz::|e_dn_class', a):
        e.append('Lizalfos code or state in the Chilfos adapter')
    if set(re.findall(r'\blz\.(\w+)', kk['src/kk_bridge.inc'])) - {'d1_parry_id', 'double_gap_tick'}:
        e.append('the bridge touches Lizalfos state beyond the shield re-arm pair')
    # 3. native writes of daE_KK_c only inside the KK_NOINLINE writers
    outside = s['src/kk_adapter.inc']
    for w in re.findall(r'KK_NOINLINE void (kk_native_\w+)\(', outside):
        outside = outside.replace(body(outside, 'KK_NOINLINE void ' + w + '('), '', 1)
    outside = code(outside)
    if re.search(r'a->(health|field_0x\w+|mActionMode|mMoveMode|eyePos\.\w+|mTimer|speedF|current\.angle\.\w+)\s*=(?!=)', outside) or re.search(r'mFrameCtrl\.mFrame\s*=(?!=)', outside) \
            or re.search(r'ClrTgHit\(|startCollisionSE\(', outside):
        e.append('a native field written (or a Tg mark cleared, or a native SE played) outside the KK writers')
    # 10. FIX01: below posture 100 no Shield Attack staggers and no non-lethal sword contact keeps its native reaction; the break at 100 is immediate
    if 'kk_native_tg_clear(a);' not in body(s['src/kk_adapter.inc'], 'HookAction on_kk_damage_pre(') or 'kk_resume(*s, a);' not in body(s['src/kk_adapter.inc'], 'void on_kk_damage_post(') \
            or 'kk_try_break(*s, a, "DAMAGE_POST")' not in body(s['src/kk_adapter.inc'], 'void on_kk_damage_post('):
        e.append('FIX01 anti-stagger wiring lost')
    # 11. FIX02: no posture credit for a broken spear on a block survives, the guard reaction replaces the native spear break, offensive episodes are committed
    post = body(s['src/kk_adapter.inc'], 'void on_kk_damage_post(')
    if '"SPEAR_BREAK"' in s['src/kk_adapter.inc'] or 'kk_native_action(a, kk::kGuard, 0);\n    ++s->st.spear_break_prevented;' not in post or 'if (s.ep.active) return false;' not in body(s['src/kk_adapter.inc'], 'bool kk_guard_ready('):
        e.append('FIX02 spear-intact or committed-episode wiring lost')
    # 4. no spear fabricated, no actor deleted, no child created by this module, no ice particle touched
    if re.search(r'fopAcM_createChild|fopAcM_delete|fopAcM_create\(|0x85BA|0x85ba|particle_set', allk):
        e.append('the module creates, deletes or emits by itself')
    # 5. HP: only admission, temporary decisive cap, and rollback without native death
    hp = re.findall(r'kk_native_hp\(a\)|kk_native_health\(a, ([^)]*)\)', a)
    # FIX06: + the Jump Strike headroom raise (captured up to its first ')'), its settled target, and the defended-contact restore (a second s->hp_before)
    # FIX07: + the special-skill headroom raise (a second static_cast<int>(a->health), its settled st.target, and 0 ONLY with the native death action of a
    # bounded loss that took the last HP (the same two fields damage_check's own `health <= 1` branch leaves: health 0, action 10)
    if a.count('kk_native_hp(a)') != 1 or sorted(x for x in hp if x) != ['0', '2', 's->hp_before', 's->hp_before', 'st.target', 'static_cast<int>(a->health', 'static_cast<int>(a->health', 'target'] \
            or 'kk_native_health(a, static_cast<int>(a->health) + kk::f6::kJsHeadroom);' not in a or 'kk_native_health(a, target);' not in a \
            or 'kk_native_health(a, static_cast<int>(a->health) + kk::f7::kHeadroom);' not in a or a.count('kk_native_health(a, 0);') != 1 \
            or 'kk_native_health(a, 0);\n      if (a->mActionMode != kk::kDead) kk_native_action(a, kk::kDead, 0);' not in a:
        e.append('HP written by another route: %s' % hp)
    if 'a->health == kk::policy::kNativeHp && a->field_0x560 == kk::policy::kNativeHp' not in s['src/kk_adapter.inc']:
        e.append('the 340 HP write lost its exact native 140/140 gate')
    # 6. parry FX: the two kinds are appended, the Lizalfos exec defaults kept, the Chilfos clash is map 7 of its own sound and its own setHitMark(2, this)
    if 'enum class Kind : std::uint8_t { None = 0, LinkOnLizalfos, LinkOnEoc, LizalfosOnLink, LinkOnChilfos, ChilfosOnLink, Count };' not in s['src/parry_fx_core.hpp']:
        e.append('parry FX kinds reordered')
    px = s['src/parry_fx_adapter.inc']
    if 'pf.exec_kind = pfx::Kind::LizalfosOnLink, pf.exec_map = 40, pf.exec_mark_actor = nullptr;' not in px or \
       'pf.exec_kind = pfx::Kind::ChilfosOnLink, pf.exec_map = 7, pf.exec_mark_actor = static_cast<const void*>(static_cast<fopAc_ac_c*>(a));' not in px:
        e.append('parry FX exec contract changed')
    if px.count('kk_preview_link_parry(') != 1 or px.count('kk_preview_enemy_parry(') != 1:
        e.append('parry FX previews of the Chilfos changed')
    # 7. hooks: ten E_KK targets, PRE/POST only, all-or-nothing install
    m = s['src/mod.cpp']
    targets = re.findall(r'DEFINE_HOOK\(&daE_KK_c::(\w+), (\w+)\);', m)
    if sorted(t for t, _ in targets) != sorted(['execute', '_delete', 'damage_check', 'executeAttack', 'setBck', 'setWeaponBck', 'executeBackWalk', 'executeSpearThrow', 'executeDamage', 'nextActionCheck', 'mCutTypeCheck']) \
            or m.count('DEFINE_HOOK(&daAlink_c::checkCutAction, LinkCutAction);') != 1:
        e.append('E_KK hook targets changed: %s' % targets)
    inst = body(s['src/kk_adapter.inc'], 'void kk_install()')
    if len(re.findall(r'mods::hook::add_(pre|post)<', inst)) != 21 or 'replace' in inst or 'kz.enabled = ok;' not in inst:
        e.append('install is not the 21 PRE/POST records, all or nothing')
    for call in ('kk_install();', 'kk_hud_samples();', 'kk_shutdown();'):
        if code(m).count(call.replace('();', '()')) < 1 or m.count('  ' + call) != 1:
            e.append('wiring lost: ' + call)
    # 8. identity of the mod and of the Lizalfos module
    mj = json.loads(s['mod.json'])
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.7':
        e.append('mod id/version')
    if 'inline constexpr const char* kVersion = "0.12.5";' not in s['src/liz_core.hpp']:
        e.append('Lizalfos module version string changed')
    # 9. the request's numbers
    core = s['src/kk_core.hpp']
    for need in ('kNativeHp = 140, kMaxHp = 340, kMaxPosture = 100', 'kGainLinkParry = 13, kGainSwordHp = 5, kGainBlock = 3, kGainSpearBreak = 5, kGainIron = 30, kGainEnemyParrySelf = 0',
                 'kLinkBlock = 20, kLinkDirect = 30, kLinkEnemyParry = 25', 'kRegenDelayTicks = 60', 'kBreakTicks = 90;', 'kPctA = 12, kPctB = 35, kPctC = 45, kPctTwoBlocks = 80',
                 'kWoundPct[3] = {45, 80, 100}', 'kRateD = 1.25f, kRateE = 1.30f, kRateEBurst = 1.10f, kRateDFast = 1.40f, kRateBack = 1.25f, kRateThrow = 1.45f, kRateRestore = 1.60f'):
        if need not in core:
            e.append('policy value changed: ' + need[:40])
    # 10. FIX04: the C3 wind-up never parks the eye at a giant height (the native BG line is cast from it); the hit-mark PRE only acts on an E_KK spear
    ad = code(s['src/kk_adapter.inc'])
    if 'e9f' in ad or 'kk::throw_eye(a->eyePos.y, k->current.pos.y)' not in ad:
        e.append('FIX04: C3 wind-up eye height not the Link-relative one')
    # 11. FIX05: the stagger clip is driven only through kk_native_bow (rate + loop frame) from the post-Execute step; the head-lock lift only raises the
    #     point; the eye hold is bounded by Link's own Mortal Draw on that target; the Jump Attack is a rebound proc and its landing never opens a new swing
    fa = s['src/kk_adapter.inc']
    f5 = body(fa, 'void kk_fix05_post(')
    if 'KK_NOINLINE void kk_native_bow(mDoExt_McaMorfSO* m, float rate, std::int16_t loop) noexcept { m->mFrameCtrl.mRate = rate, m->mFrameCtrl.mLoop = loop; }' not in fa \
            or code(fa).count('kk_native_bow(') != 3 or code(f5).count('kk_native_bow(') != 2 or 'mFrameCtrl.mLoop' in code(fa).replace('m->mFrameCtrl.mRate = rate, m->mFrameCtrl.mLoop = loop', ''):
        e.append('FIX05: stagger clip written outside its single writer / step')
    if code(fa).count('kk_native_head(') != 2 or 'if (y > native) {\n      kk_native_head(a, y);' not in f5 or 'a->checkHeadLockFlg()' not in f5:
        e.append('FIX05: head-lock lift not bounded to a raised point of a broken, head-locked Chilfos')
    if 'k->mTargetedActor == static_cast<fopAc_ac_c*>(a)' not in f5 or 'now <= s.md_hold_until' not in f5 or 'kk::mortal_draw_cut(k->getCutType())' not in f5:
        e.append('FIX05: Mortal Draw eye hold not bounded to that Mortal Draw on that target')
    if 'proc == daAlink_c::PROC_CUT_JUMP || proc == daAlink_c::PROC_CUT_JUMP_LAND' not in fa or 'kz.jump_open && kz.jump_serial == kz.swing_serial' not in fa:
        e.append('FIX05: Jump Attack rebound route or one-serial landing lost')
    hm = body(s['src/kk_adapter.inc'], 'HookAction on_kk_hitmark_pre(')
    if 'fopAcM_GetName(aa) != fpcNm_E_KK_e' not in hm or hm.count('HOOK_SKIP_ORIGINAL') != 1 or hm.index('fopAcM_GetName(aa)') > hm.index('HOOK_SKIP_ORIGINAL'):
        e.append('FIX04: the hit-mark PRE is not scoped to the Chilfos spear')
    return e


NEG = [
    ('src/kk_adapter.inc', '        kk_native_bow(m, s.bow_rate, kk::bow_loop(static_cast<std::int16_t>(s.bow_end)));', '        m->mFrameCtrl.mRate = s.bow_rate;'),
    ('src/kk_adapter.inc', '    if (y > native) {\n      kk_native_head(a, y);', '    if (true) {\n      kk_native_head(a, y);'),
    ('src/kk_adapter.inc', '    const bool on = k && now <= s.md_hold_until &&', '    const bool on = k &&'),
    ('src/kk_adapter.inc', ' || proc == daAlink_c::PROC_CUT_JUMP || proc == daAlink_c::PROC_CUT_JUMP_LAND;', ';'),
    ('src/liz_adapter.inc', 'inline constexpr float kLizJumpAtStartFrame = 13.0f;', 'inline constexpr float kLizJumpAtStartFrame = 12.0f;'),
    ('src/liz_adapter.inc', 'void on_liz_cut_post(', 'void liz_extra() noexcept {}\nvoid on_liz_cut_post('),
    ('src/kk_adapter.inc', '      const float y = kk::throw_eye(a->eyePos.y, k->current.pos.y);', '      const float y = -1.0e9f;'),
    ('src/kk_adapter.inc', '  if (!aa || fopAcM_GetName(aa) != fpcNm_E_KK_e) return HOOK_CONTINUE;', '  if (!aa) return HOOK_CONTINUE;'),
    ('src/liz_core.hpp', 'inline constexpr const char* kVersion = "0.12.5";', 'inline constexpr const char* kVersion = "0.13.0";'),
    ('src/liz_adapter.inc', 'liz_native_hp', 'liz_native_hp2'),
    ('src/parry_core.hpp', 'namespace', 'namespace  '),
    ('src/kk_adapter.inc', 'KK_NOINLINE void kk_native_back(daE_KK_c* a) noexcept { a->field_0x76c = 0; }', 'KK_NOINLINE void kk_native_back(daE_KK_c* a) noexcept { a->field_0x76c = 0; }\nvoid kk_drive(daAlink_c* k) noexcept { k->mProcID = 3; }'),
    ('src/kk_adapter.inc', 'bool kk_alive() noexcept { return kz.enabled; }', 'bool kk_alive() noexcept { return kz.enabled && lz.enabled; }'),
    ('src/kk_bridge.inc', '  lz.d1_parry_id = id;\n  g.link_track.note', '  lz.d1_parry_id = id, lz.swing_jump = false;\n  g.link_track.note'),
    ('src/kk_adapter.inc', '  kkb_pfx_begin(a, s->id);\n  if (a->mActionMode == kk::kDead) {', '  kkb_pfx_begin(a, s->id);\n  a->mTimer = 0;\n  if (a->mActionMode == kk::kDead) {'),
    ('src/kk_adapter.inc', 'void kk_shutdown() noexcept {', 'void kk_spawn_spear(daE_KK_c* a) noexcept { fopAcM_createChild(0, 0, 0, nullptr, 0, nullptr, nullptr, -1, nullptr); }\nvoid kk_shutdown() noexcept {'),
    ('src/kk_adapter.inc', '      if (a->health > 2) kk_native_health(a, 2);', '      if (a->health > 2) kk_native_health(a, 1);'),
    ('src/kk_adapter.inc', '    if (a->health == kk::policy::kNativeHp && a->field_0x560 == kk::policy::kNativeHp) {', '    if (a->field_0x560 == kk::policy::kNativeHp) {'),
    ('src/parry_fx_core.hpp', 'LizalfosOnLink, LinkOnChilfos, ChilfosOnLink, Count', 'LinkOnChilfos, ChilfosOnLink, LizalfosOnLink, Count'),
    ('src/parry_fx_adapter.inc', 'pf.exec_kind = pfx::Kind::ChilfosOnLink, pf.exec_map = 7,', 'pf.exec_kind = pfx::Kind::ChilfosOnLink, pf.exec_map = 40,'),
    ('src/mod.cpp', 'DEFINE_HOOK(&daE_KK_c::nextActionCheck, KkNext);', 'DEFINE_HOOK(&daE_KK_c::nextActionCheck, KkNext);\nDEFINE_HOOK(&daE_KK_c::draw, KkDraw);'),
    ('src/kk_adapter.inc', '  kz.enabled = ok;  // all or nothing', '  kz.enabled = true;  // all or nothing'),
    ('src/mod.cpp', '  kk_hud_samples();\n', ''),
    ('mod.json', '"version": "0.13.7"', '"version": "0.13.2"'),
    ('src/kk_core.hpp', 'kGainLinkParry = 13,', 'kGainLinkParry = 12,'),
    ('src/kk_core.hpp', 'kBreakTicks = 90;', 'kBreakTicks = 120;'),
    ('src/kk_core.hpp', 'kRateEBurst = 1.10f', 'kRateEBurst = 1.50f'),
    ('src/kk_adapter.inc', '  kkb_pfx_begin(a, s->id);\n  if (a->mActionMode == kk::kDead) {', '  kkb_pfx_begin(a, s->id);\n  a->speedF = 0.0f;\n  if (a->mActionMode == kk::kDead) {'),
    ('src/kk_adapter.inc', '  kkb_pfx_begin(a, s->id);\n  if (a->mActionMode == kk::kDead) {', '  kkb_pfx_begin(a, s->id);\n  a->mCyl.cCcD_ObjHitInf::ClrTgHit();\n  if (a->mActionMode == kk::kDead) {'),
    ('src/kk_adapter.inc', '    else if (!s->brk.active() && s->cur_v == kk::V::Damage) {\n      kk_resume(*s, a);', '    else if (!s->brk.active() && s->cur_v == kk::V::Damage) {\n      (void)0;'),
    ('src/kk_adapter.inc', '    kk_native_action(a, kk::kGuard, 0);\n    ++s->st.spear_break_prevented;', '    ++s->st.spear_break_prevented;'),
    ('src/kk_adapter.inc', 'void kk_shutdown() noexcept {', 'void kk_force(daAlink_c* k) noexcept { k->procCutFinishInit(3); }\nvoid kk_shutdown() noexcept {'),
    ('src/kk_adapter.inc', '  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);', '  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);\n  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawA);'),
    ('src/mod.cpp', 'DEFINE_HOOK(&daE_KK_c::mCutTypeCheck, KkCutCheck);\n', ''),
    ('src/mod.cpp', 'DEFINE_HOOK(&daAlink_c::checkCutAction, LinkCutAction);\n', ''),
    ('src/kk_adapter.inc', '        ++s->st.blocks;\n        s->def.on_block(now, s->cur_serial);', '        kk_posture_gain(*s, a, 5, "SPEAR_BREAK", s->cur_serial);\n        ++s->st.blocks;\n        s->def.on_block(now, s->cur_serial);'),
]


def main():
    out = {'status': 'KK_SOURCE_PASS', 'positive': rules(SRC), 'negatives': [], 'errors': []}
    out['errors'] += out['positive']
    for name, old, new in NEG:
        if SRC[name].count(old) < 1:
            out['errors'].append('negative anchor missing: %s %r' % (name, old[:50]))
            continue
        s2 = dict(SRC)
        s2[name] = SRC[name].replace(old, new, 1)
        try:
            rejected = bool(rules(s2))
        except (ValueError, json.JSONDecodeError):
            rejected = True
        out['negatives'].append({'file': name, 'rejected': rejected})
        if not rejected:
            out['errors'].append('negative accepted: %s %r' % (name, old[:50]))
    out['count'] = len(out['negatives'])
    if out['errors']:
        out['status'] = 'KK_SOURCE_FAIL'
    print(json.dumps(out, indent=1))
    sys.exit(1 if out['errors'] else 0)


main()
