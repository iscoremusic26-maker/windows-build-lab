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


def rules(s):
    e = []
    for n, h in FROZEN.items():
        if hashlib.sha256(s[n].encode('utf-8')).hexdigest() != h:
            e.append('Lizalfos/shared file changed vs FIX05: ' + n)
    kk = {n: code(s[n]) for n in KK}
    a = kk['src/kk_adapter.inc']
    allk = ''.join(kk.values())
    # 1. Link is never driven: no procedure, demo mode or Link field written by this module
    if re.search(r'mProcID\s*=(?!=)|setProc\w*\(|changeDemoMode|procCut\w*Init|->mProcVar\w*\s*=(?!=)|mNormalSpeed\s*=(?!=)', allk):
        e.append('the Chilfos module drives Link')
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
    if re.search(r'a->(health|field_0x\w+|mActionMode|mMoveMode|eyePos\.\w+|mTimer)\s*=(?!=)', outside) or re.search(r'mFrameCtrl\.mFrame\s*=(?!=)', outside):
        e.append('a native field written outside the KK writers')
    # 4. no spear fabricated, no actor deleted, no child created by this module, no ice particle touched
    if re.search(r'fopAcM_createChild|fopAcM_delete|fopAcM_create\(|0x85BA|0x85ba|particle_set', allk):
        e.append('the module creates, deletes or emits by itself')
    # 5. HP: only admission, temporary decisive cap, and rollback without native death
    hp = re.findall(r'kk_native_hp\(a\)|kk_native_health\(a, ([^)]*)\)', a)
    if a.count('kk_native_hp(a)') != 1 or sorted(x for x in hp if x) != ['2', 's->hp_before']:
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
    if sorted(t for t, _ in targets) != sorted(['execute', '_delete', 'damage_check', 'executeAttack', 'setBck', 'setWeaponBck', 'executeBackWalk', 'executeSpearThrow', 'executeDamage', 'nextActionCheck']):
        e.append('E_KK hook targets changed: %s' % targets)
    inst = body(s['src/kk_adapter.inc'], 'void kk_install()')
    if len(re.findall(r'mods::hook::add_(pre|post)<', inst)) != 17 or 'replace' in inst or 'kz.enabled = ok;' not in inst:
        e.append('install is not the 17 PRE/POST records, all or nothing')
    for call in ('kk_install();', 'kk_hud_samples();', 'kk_shutdown();'):
        if code(m).count(call.replace('();', '()')) < 1 or m.count('  ' + call) != 1:
            e.append('wiring lost: ' + call)
    # 8. identity of the mod and of the Lizalfos module
    mj = json.loads(s['mod.json'])
    if mj.get('id') != 'dev.posture.post05_selective_reactions' or mj.get('version') != '0.13.0':
        e.append('mod id/version')
    if 'inline constexpr const char* kVersion = "0.12.5";' not in s['src/liz_core.hpp']:
        e.append('Lizalfos module version string changed')
    # 9. the request's numbers
    core = s['src/kk_core.hpp']
    for need in ('kNativeHp = 140, kMaxHp = 340, kMaxPosture = 100', 'kGainLinkParry = 13, kGainSwordHp = 5, kGainBlock = 3, kGainSpearBreak = 5, kGainIron = 30, kGainEnemyParrySelf = 0',
                 'kLinkBlock = 20, kLinkDirect = 30, kLinkEnemyParry = 25', 'kRegenDelayTicks = 60', 'kBreakTicks = 90;', 'kPctA = 12, kPctB = 35, kPctC = 45, kPctTwoBlocks = 80',
                 'kWoundPct[3] = {45, 80, 100}', 'kRateD = 1.25f, kRateE = 1.30f, kRateEBurst = 1.50f, kRateDFast = 1.40f, kRateBack = 1.25f, kRateThrow = 1.45f, kRateRestore = 1.60f'):
        if need not in core:
            e.append('policy value changed: ' + need[:40])
    return e


NEG = [
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
    ('mod.json', '"version": "0.13.0"', '"version": "0.12.5"'),
    ('src/kk_core.hpp', 'kGainLinkParry = 13,', 'kGainLinkParry = 12,'),
    ('src/kk_core.hpp', 'kBreakTicks = 90;', 'kBreakTicks = 120;'),
    ('src/kk_core.hpp', 'kRateEBurst = 1.50f', 'kRateEBurst = 1.30f'),
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
