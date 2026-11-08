"""LIZALFOS DUELIST 0.12.0 additive barrier. Every 0.11.12 production file must be reproduced byte for byte
by the exact, counted inverse patch in liz_policy.json (the older chain then applies as before); the two new
modules are hash-pinned; native writes live only in the six LIZ writers of the adapter; the core is pure.
"""
from pathlib import Path
import hashlib, json, re, sys
HERE = Path(__file__).resolve().parent
NEW = {'liz_core.hpp', 'liz_adapter.inc'}


def policy():
    return json.loads((HERE / 'liz_policy.json').read_text(encoding='utf-8'))


def sha(b):
    return hashlib.sha256(b).hexdigest()


def legacy_source(name, text):
    for e in reversed(policy()['inverse_patch'].get(name, [])):
        if text.count(e['after']) == e.get('count', 1):
            text = text.replace(e['after'], e['before'], e.get('count', 1))
    return text


def body(text, signature):
    i = text.index(signature)
    j = text.index('{', i)
    depth = 0
    k = j
    while True:
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                return text[i:k + 1]
        k += 1


WRITERS = {
    'fields': 'LIZ_NOINLINE void liz_native_fields(',
    'anim': 'LIZ_NOINLINE void liz_native_anim(',
    'hp': 'LIZ_NOINLINE void liz_native_hp(',
    'guard': 'LIZ_NOINLINE void liz_native_guard(',
    'break': 'LIZ_NOINLINE void liz_native_break(',
    'tg': 'LIZ_NOINLINE void liz_native_tg_clear(',
}
OWN_NATIVE_WRITES = [('a->action = ', 'fields'), ('a->mode = ', 'fields'), ('a->timer[2] = ', 'fields'), ('ac->current.angle.y = ', 'fields'), ('ac->current.pos.x += ', 'fields'),
                     ('m->mFrameCtrl.mFrame = ', 'anim'), ('m->setPlaySpeed(', 'anim'), ('ac->health = ', 'hp'), ('ac->field_0x560 = ', 'hp'),
                     ('a->field_0x708 = ', 'guard'), ('a->field_0x70c = ', 'guard'), ('a->timer[0] = ', 'guard'), ('a->at_info.mHitDirection.y = ', 'break'), ('LizBig::g_orig(', 'break'),
                     ('.cCcD_ObjHitInf::ClrTgHit()', 'tg')]
FORBIDDEN_ADAPTER = ['setActionMode', 'VirtualProtect', 'WriteProcessMemory', 'get_service(', 'cM_rnd', 'setBck(', 'OnAtSetBit', 'OffAtSetBit', 'ClrAtHit', 'ResetAtHit', 'OnTgSetBit', 'OffTgSetBit', 'OnCoSetBit',
                     'OffCoSetBit', 'setItemLifeCount', 'setLife(', 'procGuardBreakInit', 'mItemTrigger', 'daE_OC_c', 'mSphs_at', 'write_vitality', 'restore_reaction']
FORBIDDEN_CORE = ['uintptr_t', 'void*', 'e_dn_class', 'daAlink_c', 'cM_rnd', 'g_dComIfG', 'sqrt(', 'sinf(', 'cosf(', '#include <cmath>']
REQUIRED_CORE = [
    'kMaxHp = 370', 'kGainSword = 15, kGainLinkParry = 22, kGainIronBall = 30, kGainBlocked = 5, kGainLinkOnEnemyParry = 25', 'kRegenDelayTicks = 60',
    'if (h > 75 * m) return 10;', 'if (h > 50 * m) return 7;', 'if (h > 25 * m) return 4;', 'if (h > 10 * m) return 2;',
    'kParryPctA = 12, kParryPctB = 35, kParryPctC = 45', 'kParryCooldownTicks = 120', 'kRateN = 1.25f, kRateD = 1.10f, kRateCSlow = 1.10f, kRateCFast = 1.20f',
    'pat = rng.below(kPatterns);  // exactly one draw per episode start, exact 25% each',
    '{3, 4, {Act::N, Act::D, Act::CFast}, {0.50f, 0.63f}}',
    '{6, 7, {Act::N, Act::N, Act::N, Act::D, Act::N, Act::CFast}, {0.58f, 0.60f, 0.54f, 0.66f, 0.67f}}',
    '{6, 9, {Act::D, Act::N, Act::D, Act::N, Act::N, Act::D}, {0.52f, 0.53f, 0.54f, 0.61f, 0.52f}}',
    '{5, 6, {Act::CSlow, Act::CSlow, Act::N, Act::D, Act::CFast}, {1.34f, 1.26f, 0.52f, 0.63f}}',
    'if (!admitted || !ret_is_link || v.action != native::kAttack) return false;',
    'if (r == Hit::Parry && p.add(policy::kGainLinkParry)) request_break(v);',
    'const bool crossed = !lethal && p.add(policy::kGainIronBall);',
]


def audit(project):
    p = Path(project)
    pin = policy()
    errors = []
    if set(pin['files']) != {'src/' + n for n in NEW}:
        errors.append('LIZ exact two-module inventory required')
    if len(pin['baseline_files']) != pin['baseline_count']:
        errors.append('LIZ frozen 0.11.12 production inventory count changed')
    present = {f.relative_to(p).as_posix() for f in (p / 'src').rglob('*') if f.is_file()}
    want = set(pin['baseline_files']) | {'src/' + n for n in NEW}
    if present != want:
        errors.append('LIZ production file set differs from 0.11.12 + two LIZ modules: %s' % sorted(present ^ want))
    for name, edits in pin['inverse_patch'].items():
        t = (p / 'src' / name).read_text(encoding='utf-8')
        for e in reversed(edits):
            if t.count(e['after']) != e.get('count', 1):
                errors.append('LIZ exact integration changed: ' + name)
            else:
                t = t.replace(e['after'], e['before'], e.get('count', 1))
    for rel, w in pin['baseline_files'].items():
        f = p / rel
        if not f.is_file():
            errors.append('LIZ 0.11.12 production missing: ' + rel)
            continue
        b = legacy_source(f.name, f.read_text(encoding='utf-8')).encode() if f.suffix in ('.cpp', '.hpp', '.inc', '.h') else f.read_bytes()
        if sha(b) != w:
            errors.append('LIZ 0.11.12 production changed outside exact authorized edits: ' + rel)
    for rel, w in pin['files'].items():
        f = p / rel
        if not f.is_file() or sha(f.read_bytes()) != w:
            errors.append('LIZ reviewed new module changed: ' + rel)
    a = (p / 'src/liz_adapter.inc').read_text(encoding='utf-8')
    core = (p / 'src/liz_core.hpp').read_text(encoding='utf-8')
    code = re.sub(r'//[^\n]*', '', re.sub(r'/\*.*?\*/', '', core, flags=re.S))
    acode = re.sub(r'//[^\n]*', '', re.sub(r'"(?:\\.|[^"\\])*"', '""', a))
    for bad in FORBIDDEN_ADAPTER:
        if bad in acode:
            errors.append('LIZ forbidden native operation: ' + bad)
    for bad in FORBIDDEN_CORE:
        if bad in code:
            errors.append('LIZ core is not pure: ' + bad)
    for tok in REQUIRED_CORE:
        if tok not in core:
            errors.append('LIZ fixed value or rule missing: ' + tok)
    if len(re.findall(r'^LIZ_NOINLINE void ', a, re.M)) != len(WRITERS) or a.count('#define LIZ_NOINLINE') != 2 or a.count('#undef LIZ_NOINLINE') != 1:
        errors.append('LIZ exactly six noinline writer functions required')
    try:
        wb = {k: body(a, s) for k, s in WRITERS.items()}
    except ValueError:
        errors.append('LIZ writer function missing')
        wb = {}
    if wb:
        rest = a
        for v in wb.values():
            rest = rest.replace(v, '')
        for tok, k in OWN_NATIVE_WRITES:
            if tok not in wb[k]:
                errors.append('LIZ writer lost its native write: ' + tok)
            if tok in rest:
                errors.append('LIZ native write outside its writer: ' + tok)
        for pat in [r'a->\w+(\[\w+\])?\s*=(?!=)', r'ac->[\w\.]+\s*(=(?!=)|\+=|-=)', r'k->\w+\s*=(?!=)', r'link->\w+\s*=(?!=)',
                    r'->(health|field_0x560|action|mode|invulnerability_timer|speedF|mFrameCtrl\.mFrame)\s*(=(?!=)|\+=|-=)', r'->setPlaySpeed\(', r'->setFrame\(']:
            for m in re.finditer(pat, rest):
                errors.append('LIZ assignment through a game pointer outside a writer: ' + m.group(0))
    # at_hit_check suppression: only through the POST return slot; attack_0 vibration branch and tail attack untouched by construction (action test in the core).
    for tok in ['if (s->b.on_at_hit_post(liz_view(a, f1_link()), is_link)) *r = nullptr;', 'if (!s->b.admitted) return HOOK_CONTINUE;', 'lz.enabled = ok;',
                'if (L.pass.guard_attack && !cp04_guard_pass()) L.pass.guard_attack = false;', '(void)c4.shield.confirm(g.p8.tick);',
                'g.p8.tick >= c4.shield.permit_tick + liz::policy::kDoubleRearmTicks', 'if (s && s->id != id) {']:
        if tok not in a:
            errors.append('LIZ production binding missing: ' + tok)
    regs = re.findall(r'mods::hook::add_(pre|post)<(\w+)>\((\w+)', a)
    want_regs = sorted([('pre', 'LizExec', 'on_liz_exec_pre'), ('post', 'LizExec', 'on_liz_exec_post'), ('pre', 'LizDelete', 'on_liz_delete_pre'), ('pre', 'LizDamage', 'on_liz_damage_pre'), ('post', 'LizDamage', 'on_liz_damage_post'),
                        ('pre', 'LizSmall', 'on_liz_small_pre'), ('pre', 'LizBig', 'on_liz_big_pre'), ('pre', 'LizRun', 'on_liz_run_pre'), ('post', 'LizRun', 'on_liz_run_post'), ('pre', 'LizAttack0', 'on_liz_attack_pre'),
                        ('post', 'LizAttack0', 'on_liz_attack_post'), ('pre', 'LizAttack', 'on_liz_attack_pre'), ('post', 'LizAttack', 'on_liz_attack_post'), ('pre', 'LizTail', 'on_liz_attack_pre'), ('post', 'LizTail', 'on_liz_attack_post'),
                        ('post', 'LizAtHit', 'on_liz_at_hit_post'), ('pre', 'LizAnm', 'on_liz_anm_pre'), ('post', 'LizAnm', 'on_liz_anm_post'), ('post', 'LinkCutType', 'on_liz_cut_post'), ('post', 'LinkDamageFix01', 'on_link_damage_post_liz')])
    if sorted(regs) != want_regs:
        errors.append('LIZ hook registrations changed: %s' % regs)
    return errors


if __name__ == '__main__':
    e = audit(Path(sys.argv[1]))
    print(json.dumps({'status': 'SOURCE_AUDIT_FAIL' if e else 'SOURCE_AUDIT_PASS', 'base_source': policy()['base_source'], 'frozen_0_11_12_files': policy()['baseline_count'], 'errors': e}, indent=2))
    sys.exit(1 if e else 0)
