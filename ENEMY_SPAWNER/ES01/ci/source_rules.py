"""Enemy Spawner 0.1.0: source rules, each proven by negatives (the rule must PASS on the tree and FAIL on every mutated copy).
Usage: source_rules.py <project>"""
import json, re, sys
from pathlib import Path
root = Path(sys.argv[1])
NAMES = ('src/mod.cpp', 'src/spawner_adapter.inc', 'src/spawner_core.hpp', 'src/catalog.hpp', 'mod.json')
SRC = {n: (root / n).read_text(encoding='utf-8') for n in NAMES}

def code(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    t = re.sub(r'"(?:\\.|[^"\\])*"', '""', t)
    return re.sub(r'//[^\n]*', '', t)

def body(t, sig):
    i = t.index(sig); j = t.index('{', i); d = 0
    for k in range(j, len(t)):
        d += t[k] == '{'; d -= t[k] == '}'
        if d == 0:
            return t[i:k + 1]
    raise ValueError(sig)

def rules(s):
    e = []
    allc = ''.join(code(s[n]) for n in NAMES if n != 'mod.json')
    m, a = code(s['src/mod.cpp']), code(s['src/spawner_adapter.inc'])
    # 1. no hook of any kind, no hook service
    if re.search(r'DEFINE_HOOK|HookService|mods::hook|svc_hook', allc):
        e.append('a hook or the hook service')
    # 2. exactly these services
    imps = sorted(re.findall(r'IMPORT_(?:OPTIONAL_)?SERVICE[A-Z_]*\((\w+)', m))
    if imps != sorted(['HostService', 'LogService', 'ActorService', 'ConfigService', 'UiService']):
        e.append('service imports changed: %s' % imps)
    # 3. the layer is only set by its writer, which is only called by the creation helper, which restores it
    if allc.count('fpcLy_SetCurrentLayer(') != 1 or 'fpcLy_SetCurrentLayer(layer)' not in body(a, 'void es_native_layer('):
        e.append('fpcLy_SetCurrentLayer outside es_native_layer')
    h = body(a, 'ModResult es_create_in_link_layer(')
    if '~Restore()' not in h or 'es_native_layer(previous)' not in h or 'fpcLy_CurrentLayer()' not in h or h.index('restore{') > h.index('es_native_layer(layer)'):
        e.append('layer not restored around the creation call')
    for g in ('layer == fpcLy_RootLayer()', 'fpcLy_IsDeletingMesg(layer)', 'dComIfGp_isEnableNextStage()', 'link == nullptr'):
        if g not in h:
            e.append('creation guard lost: ' + g)
    if a.count('svc_actor->create_actor(') != 1 or 'svc_actor->create_actor(' not in h:
        e.append('create_actor called outside the layer helper')
    # 4. the only actor field writes are the Toadpoli adaptations, inside their writer
    w = body(a, 'void es_native_keep_ground(')
    pat = r'->\s*([\w.]+(?:\[\d+\])?)\s*=(?!=)'
    writes = re.findall(pat, a.replace(w, '') + m)
    if writes or sorted(re.findall(pat, w)) != ['mActionTimer[3]', 'mExecuteState'] or 'mExecuteState = 2' not in w or 'mActionTimer[3] = 2' not in w:
        e.append('native field written outside es_native_keep_ground: %s' % writes)
    if re.search(r'(->|\)\.)\s*(health|field_0x560)\s*=(?!=)', allc):
        e.append('HP written')
    # 5. nothing deletes or creates by other routes, nothing calls another mod
    for bad in ('delete_actor', 'create_child_actor', 'create_actor_from_name', 'fopAcM_create(', 'fopAcM_delete', 'publish_service', 'dlsym', 'GetProcAddress'):
        if bad in allc:
            e.append('forbidden call: ' + bad)
    if len(re.findall(r'get_service\(', m)) != 1 or 'kFrameworkService' not in body(m, 'bool es_framework_present('):
        e.append('get_service used for something else than the framework presence')
    # 6. Black: one entry, never creatable, labelled unavailable
    cat = s['src/catalog.hpp']
    blacks = re.findall(r'\{"([^"]*Black \(Expansion\)[^"]*)", Profile::\w+, [^}]*Route::(\w+)', cat)
    if len(blacks) != 1 or blacks[0][1] != 'BlackUnavailable' or 'no disponible' not in blacks[0][0]:
        e.append('Black entry must be the single unavailable one: %s' % blacks)
    core = code(s['src/spawner_core.hpp'])
    if 'if (e.route == Route::BlackUnavailable) return Press::BlackUnavailable;' not in core:
        e.append('Black refusal lost')
    # 7. identity of this mod, never another mod's
    mj = json.loads(s['mod.json'])
    if mj.get('id') in ('dev.bezide.dawnlight', 'dev.bokoblin.expansion', 'dev.devf.cp06_native_config_dual', 'dev.posture.post05_selective_reactions') or mj.get('id') != 'dev.enemyspawner.standalone' or mj.get('version') != '0.1.0':
        e.append('mod id/version')
    if 'Goron' in code(cat).replace('""', '') or re.search(r'\{"(Goron|Staltroop)"', cat):
        e.append('an excluded entry is offered')
    # 8. the UI only presses; the engine is only touched from mod_update
    if 'es_update' in body(m, 'void on_invoke(') or 'es_spawn_now' in body(a, 'void es_press('):
        e.append('the UI callback creates directly')
    return e

NEG = [
    ('src/mod.cpp', 'IMPORT_SERVICE(UiService, svc_ui);', 'IMPORT_SERVICE(UiService, svc_ui);\nIMPORT_SERVICE(HookService, svc_hook);'),
    ('src/mod.cpp', 'DEFINE_MOD();', 'DEFINE_MOD();\nDEFINE_HOOK(&fpcMtd_Method, SpawnerProcessHook);'),
    ('src/spawner_adapter.inc', '  es_native_layer(layer);\n  return svc_actor', '  fpcLy_SetCurrentLayer(layer);\n  return svc_actor'),
    ('src/spawner_adapter.inc', '      es_native_layer(previous);\n', ''),
    ('src/spawner_adapter.inc', ' || layer == fpcLy_RootLayer() || fpcLy_IsDeletingMesg(layer)) return MOD_UNAVAILABLE;\n  struct', ' || fpcLy_IsDeletingMesg(layer)) return MOD_UNAVAILABLE;\n  struct'),
    ('src/spawner_adapter.inc', 'if (link == nullptr || dComIfGp_isEnableNextStage()) return MOD_UNAVAILABLE;', 'if (link == nullptr) return MOD_UNAVAILABLE;'),
    ('src/spawner_adapter.inc', '  es_native_keep_ground(a, how);', '  es_native_keep_ground(a, how);\n  a->health = 1;'),
    ('src/spawner_adapter.inc', '    p.field_0x560 = b->field_0x560, p.health = b->health;', '    p.field_0x560 = b->field_0x560, p.health = b->health;\n    const_cast<daE_OC_c*>(b)->health = 330;'),
    ('src/spawner_adapter.inc', '  if (how == es::Adapt::ToadpoliWater) static_cast', '  if (how == es::Adapt::ToadpoliWater) a->current.pos.y = 0;\n  if (how == es::Adapt::ToadpoliWater) static_cast'),
    ('src/spawner_adapter.inc', 'void es_press(std::int64_t index) noexcept {', 'void es_kill(std::uint32_t id) noexcept { (void)svc_actor->delete_actor(mod_ctx, id); }\nvoid es_press(std::int64_t index) noexcept {'),
    ('src/spawner_adapter.inc', '  const es::Press r = esr.sp.press(index, esr.tick);', '  const es::Press r = esr.sp.press(index, esr.tick);\n  es_spawn_now(esr.tick);'),
    ('src/mod.cpp', '  es_press(index);\n}', '  es_press(index);\n  es_update();\n}'),
    ('src/mod.cpp', '  es_update();\n  return MOD_OK;', '  es_update();\n  const void* x = nullptr;\n  (void)svc_host->get_service(mod_ctx, "dev.bokoblin.expansion", 1, 0, &x);\n  return MOD_OK;'),
    ('src/catalog.hpp', 'Route::BlackUnavailable, Adapt::None},', 'Route::ExpansionRed, Adapt::None},'),
    ('src/catalog.hpp', '"Bokoblin Black (Expansion): integracion no disponible"', '"Bokoblin Black (Expansion)"'),
    ('src/catalog.hpp', '    {"Lizalfos",', '    {"Goron", Profile::E_DN, 0u, -1, 0, 0, false, Route::Native, Adapt::None},\n    {"Lizalfos",'),
    ('src/spawner_core.hpp', '    if (e.route == Route::BlackUnavailable) return Press::BlackUnavailable;  // never a Red with a black label\n', ''),
    ('mod.json', '"dev.enemyspawner.standalone"', '"dev.bezide.dawnlight"'),
    ('mod.json', '"dev.enemyspawner.standalone"', '"dev.bokoblin.expansion"'),
    ('mod.json', '"version": "0.1.0"', '"version": "5.4.2"'),
]

def main():
    out = {'status': 'SOURCE_RULES_PASS', 'positive': rules(SRC), 'negatives': [], 'errors': []}
    out['errors'] += out['positive']
    for name, old, new in NEG:
        if SRC[name].count(old) != 1:
            out['errors'].append('negative anchor missing: %s %r' % (name, old[:60]))
            continue
        s2 = dict(SRC); s2[name] = SRC[name].replace(old, new, 1)
        try:
            rejected = bool(rules(s2))
        except (ValueError, json.JSONDecodeError):
            rejected = True
        out['negatives'].append({'file': name, 'rejected': rejected})
        if not rejected:
            out['errors'].append('negative accepted: %s %r' % (name, old[:60]))
    out['count'] = len(out['negatives'])
    if out['errors']:
        out['status'] = 'SOURCE_RULES_FAIL'
    print(json.dumps(out, indent=1))
    sys.exit(1 if out['errors'] else 0)

main()
