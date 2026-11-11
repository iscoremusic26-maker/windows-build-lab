"""Enemy Spawner 0.1.0: every catalog entry is either Dawnlight's (same profile, parameters, argument, angle.z, flying) or a documented change.
Reads src/catalog.hpp as text against ci/dawnlight_reference.json (extracted from Dawnlight b5e9046). Usage: catalog_check.py <project>"""
import json, re, sys
from pathlib import Path
root = Path(sys.argv[1])
ref = json.loads((root / 'ci/dawnlight_reference.json').read_text())
txt = (root / 'src/catalog.hpp').read_text()
rows = re.findall(r'\{"([^"]+)", Profile::(\w+), (0x[0-9a-f]+)u, (-?\d+), (0x[0-9a-f]+|0), (0x[0-9a-f]+|0), (true|false), Route::(\w+), Adapt::(\w+)\}', txt)
errors = []
byname = {e['label']: e for e in ref['entries']}
alias = {'Stalhound (solo de noche)': 'Stalhound (night only)'}
seen = set()
for label, prof, par, arg, ax, az, fly, route, adapt in rows:
    if label.startswith('Bokoblin'):
        e = byname['Bokoblin']
        if prof != 'E_OC' or int(par, 16) != int(e['parameters'], 16):
            errors.append('Bokoblin entry differs: ' + label)
        seen.add('Bokoblin')
        continue
    key = alias.get(label, label)
    if key not in byname:
        errors.append('not a Dawnlight entry: ' + label)
        continue
    e = byname[key]
    seen.add(key)
    same = prof == e['profile'] and int(par, 16) == int(e['parameters'], 16) and int(arg) == e['argument'] and int(az, 0) == e['angle_z'] and (fly == 'true') == e['flying']
    if not same and key not in ref['changed']:
        errors.append('undocumented change: ' + label)
    if same and key in ref['changed'] and key != 'Stalhound (night only)':
        errors.append('documented change not present: ' + label)
missing = [e['label'] for e in ref['entries'] if e['label'] not in seen and e['label'] not in ref['not_offered']]
if missing:
    errors.append('Dawnlight entries dropped without a reason: %s' % missing)
for name in ref['not_offered']:
    if name in seen:
        errors.append('not offered but present: ' + name)
out = {'status': 'CATALOG_FAIL' if errors else 'CATALOG_PASS', 'rows': len(rows), 'errors': errors}
print(json.dumps(out, indent=1))
sys.exit(1 if errors else 0)
