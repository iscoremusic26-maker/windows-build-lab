"""LIGHT FADES TWICE FIX03 0.12.3 source rules, each proven by a negative: the rule must PASS on the production text and FAIL on a mutated copy.
Text checks complement the executed labs (they pin what a lab cannot compile: cp04_adapter.inc, the lock-on and parry FX adapters)."""
from pathlib import Path
import json, re, sys
root = Path(__file__).resolve().parents[1]
SRC = {n: (root / 'src' / n).read_text(encoding='utf-8') for n in ('cp04_adapter.inc', 'lockon_adapter.inc', 'parry_fx_adapter.inc', 'mod.cpp', 'liz_adapter.inc')}
LAB = (root / 'tests/liz_world_lab.cpp').read_text(encoding='utf-8')


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


CHAIN = ' if(!cp04_world_safe())c4.shield.cancel();\n if(!cp04_sim_safe()){for(auto& s:c4.slots)cp04_cancel(s,"PAUSE_OR_EVENT");}\n'


def r_chain(s):
    e = []
    m = body(s['cp04_adapter.inc'], 'void cp04_maintenance()noexcept{')
    if CHAIN not in m:
        e.append('P0-1: the shield chain must be cancelled by Link world safety only, the Blue slots by the simulation gate')
    if re.search(r'cp04_sim_safe\(\)\)\{[^}]*c4\.shield\.cancel', code(m)):
        e.append('P0-1: the Bokoblin-only layout gate cancels the Link shield chain again')
    if 'if(!cp04_world_safe())c4.shield.cancel();' not in LAB:
        e.append('P0-1: the world lab no longer runs the production chain rule verbatim')
    return e


PAD_FIELDS = r'(mHoldLockL|mTrigLockL|mCStickPosX|mCStickPosY|mCStickValue|mCStickAngle)\s*=(?!=)'
ATT_FIELDS = r'->(mAttnStatus|mLockOnOffset|mLockTargetID|field_0x32e)\s*=(?!=)'


def r_lockon(s):
    e = []
    a = s['lockon_adapter.inc']
    pad = body(a, 'LOCKON_NOINLINE void lockon_native_pad(')
    att = body(a, 'LOCKON_NOINLINE void lockon_native_attention(')
    rest = code(a.replace(pad, '').replace(att, ''))
    if re.search(PAD_FIELDS, rest):
        e.append('P1-4: pad written outside lockon_native_pad')
    if re.search(ATT_FIELDS, rest):
        e.append('P1-4: native lock written outside lockon_native_attention')
    for tok in ('desc.name = "sekiro_lockon";', 'desc.type = CONFIG_VAR_BOOL;', 'desc.default_bool = true;', 'd.kind = UI_CONTROL_TOGGLE;', 'd.binding = UI_BINDING_CONFIG_VAR;',
                'd.config_var = lk.var;', 'd.label = "Fijaci\\xC3\\xB3n de objetivos estilo Sekiro";', 'lk.enabled = value;', 'lk.enabled = value->bool_value;', 'in.enabled = lk.enabled;'):
        if tok not in a:
            e.append('P1-4: option binding missing: ' + tok)
    if re.search(r'lk\.enabled\s*=\s*true', code(a)) or re.search(r'in\.enabled\s*=\s*true', code(a)):
        e.append('P1-4: a hardcoded option value')
    if 'lockon_update();  // FIX03 P1-4' not in s['mod.cpp'] or s['mod.cpp'].index('lockon_update();  // FIX03 P1-4') > s['mod.cpp'].index('probe_variant_ai_env();\n  post08_update();'):
        e.append('P1-4: the lock-on input layer must run first in do_update')
    if 'game_info' in a or 'setOptAttentionType' in a or 'dComIfGs_setOpt' in a:
        e.append('P1-4: the user HOLD/SWITCH option is never written')
    return e


def r_pfx(s):
    e = []
    a = s['parry_fx_adapter.inc']
    w = [body(a, 'PFX_NOINLINE void pfx_native_colors('), body(a, 'PFX_NOINLINE void pfx_native_callback('), body(a, 'PFX_NOINLINE void pfx_native_pitch(')]
    rest = a
    for x in w:
        rest = rest.replace(x, '')
    rest = code(rest)
    if re.search(r'(mPrmClr|mEnvClr|mpEmtrCallBack|mpPtclCallBack|mGlobalPrmClr|mGlobalEnvClr)\s*=(?!=)', rest):
        e.append('P1-3: emitter/particle colour or callback written outside its writer')
    if 'movePitch' in rest:
        e.append('P1-3: pitch changed outside pfx_native_pitch')
    for bad in ('particle_set', 'createSimpleEmitter', 'setGlobalPrmColor', 'setGlobalEnvColor', 'getBsp', 'JPAResource', 'startSound', 'startCollisionSE(', 'setHitMark(', 'def_se_set(', 'deleteEmitter', 'setUserWork', 'getResourceManager'):
        if bad in rest:
            e.append('P1-3: forbidden (a new effect, a shared resource or a new sound): ' + bad)
    if 'pfx::Book::' in a or 'r[i].st = St::Confirmed' in a:
        e.append('P1-3: the adapter bypasses the bookkeeping core')
    if 'pf.enabled = ok;  // all or nothing' not in a:
        e.append('P1-3: all-or-nothing install lost')
    if 'pfx_settle(pfx::Kind::LinkOnEoc, in.id, ok);' not in s['mod.cpp'] or 'pfx_settle(pfx::Kind::LinkOnEoc, in.id, false);' not in s['mod.cpp']:
        e.append('P1-3: the POST08 verdict no longer settles the E_OC preview')
    for tok in ('pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry != parries);', 'pfx_settle(pfx::Kind::LizalfosOnLink, s.id, true);', 'pfx_settle(pfx::Kind::LizalfosOnLink, s->id, false);'):
        if tok not in s['liz_adapter.inc']:
            e.append('P1-3: Lizalfos verdict wiring missing: ' + tok)
    return e


RULES = {'chain': r_chain, 'lockon': r_lockon, 'pfx': r_pfx}
NEG = [
    ('chain', 'cp04_adapter.inc', CHAIN, ' if(!cp04_sim_safe()){c4.shield.cancel();for(auto& s:c4.slots)cp04_cancel(s,"PAUSE_OR_EVENT");}\n'),
    ('chain', 'cp04_adapter.inc', ' if(!cp04_world_safe())c4.shield.cancel();\n', ''),
    ('lockon', 'lockon_adapter.inc', '  lockon_update_marker', ''),
    ('lockon', 'lockon_adapter.inc', '  if (pad && o.force_release) lockon_native_attention(att, true, 0, 0), ++lk.writes_attention;', '  if (pad && o.force_release) att->mAttnStatus = 0;'),
    ('lockon', 'lockon_adapter.inc', '    in.trigger = pad->mTriggerLeft;', '    in.trigger = pad->mTriggerLeft;\n    pad->mHoldLockL = 1;'),
    ('lockon', 'lockon_adapter.inc', '  desc.default_bool = true;', '  desc.default_bool = false;'),
    ('lockon', 'lockon_adapter.inc', '  in.enabled = lk.enabled;', '  in.enabled = true;'),
    ('lockon', 'lockon_adapter.inc', '  d.binding = UI_BINDING_CONFIG_VAR;', '  d.binding = UI_BINDING_CALLBACKS;'),
    ('pfx', 'parry_fx_adapter.inc', '  pfx_native_callback(e, g_pfx_cb);', '  e->mpEmtrCallBack = g_pfx_cb;'),
    ('pfx', 'parry_fx_adapter.inc', '  pfx_native_pitch(h, base * pfx::kPitch);', '  (*h)->getAuxiliary().movePitch(base * pfx::kPitch, 0);'),
    ('pfx', 'parry_fx_adapter.inc', '  if (!e) {\n    ++pf.book.c.no_emitter;', '  if (!e) {\n    dComIfGp_particle_set(0, nullptr, nullptr, nullptr);\n    ++pf.book.c.no_emitter;'),
    ('pfx', 'parry_fx_adapter.inc', '    if (s->prev) s->prev->executeAfter(e);', '    if (s->prev) s->prev->executeAfter(e);\n    e->setGlobalPrmColor(255, 138, 32);'),
    ('pfx', 'parry_fx_adapter.inc', '  pf.enabled = ok;  // all or nothing', '  pf.enabled = true;  // all or nothing'),
    ('pfx', 'liz_adapter.inc', 'pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, s->b.st.link_parry != parries);', 'pfx_settle(pfx::Kind::LinkOnLizalfos, s->id, true);'),
    ('pfx', 'mod.cpp', 'pfx_settle(pfx::Kind::LinkOnEoc, in.id, ok);', '(void)ok;'),
]


def main():
    out = {'status': 'LFT03_SOURCE_PASS', 'positive': {}, 'negatives': [], 'errors': []}
    for k, f in RULES.items():
        e = f(SRC)
        out['positive'][k] = e
        out['errors'].extend(e)
    for k, name, old, new in NEG:
        if name == 'lockon_adapter.inc' and old == '  lockon_update_marker':  # do_update order: the call moved after the observer
            s2 = dict(SRC)
            s2['mod.cpp'] = SRC['mod.cpp'].replace('  lockon_update();  // FIX03 P1-4: before every process of this tick, right after mDoCPd_c::read\n', '').replace(
                '  liz_maintenance();\n  pfx_maintenance();', '  liz_maintenance();\n  pfx_maintenance();\n  lockon_update();  // FIX03 P1-4')
        else:
            if SRC[name].count(old) != 1:
                out['errors'].append('negative anchor missing: %s %r' % (name, old[:60]))
                continue
            s2 = dict(SRC)
            s2[name] = SRC[name].replace(old, new, 1)
        try:
            rejected = bool(RULES[k](s2))
        except ValueError:
            rejected = True
        out['negatives'].append({'rule': k, 'file': name, 'rejected': rejected})
        if not rejected:
            out['errors'].append('negative accepted: %s %s %r' % (k, name, old[:60]))
    if out['errors']:
        out['status'] = 'LFT03_SOURCE_FAIL'
    print(json.dumps(out, indent=1))
    sys.exit(1 if out['errors'] else 0)


if __name__ == '__main__':
    main()
