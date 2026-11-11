"""FIX05 CI: every text anchor of every mutation suite must exist in the tree, checked in seconds in the gate, before any slow suite is scheduled. A stale anchor used to cost a whole run.
Reads the suites without running them: only their string constants and their `cases` list are evaluated. run_mutations.py checks its own anchors at start. Usage: mutant_anchor_check.py <project>"""
import ast, json, sys
from pathlib import Path

root = Path(sys.argv[1])
errors, checked = [], 0
for f in sorted((root / 'tests').glob('test_*mutations.py')):
    tree = ast.parse(f.read_text(encoding='utf-8'))
    env = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and all(isinstance(t, (ast.Name, ast.Tuple)) for t in node.targets):
            names = [n.id for t in node.targets for n in ([t] if isinstance(t, ast.Name) else t.elts) if isinstance(n, ast.Name)]
            if 'cases' in names or isinstance(node.value, ast.Constant) or (isinstance(node.value, ast.Tuple) and all(isinstance(e, ast.Constant) for e in node.value.elts)):
                try:
                    exec(compile(ast.Module([node], []), str(f), 'exec'), env)
                except Exception as ex:  # noqa: BLE001
                    errors.append('%s: %s' % (f.name, ex))
    for c in env.get('cases', []):
        if len(c) < 4 or not isinstance(c[1], str) or not isinstance(c[2], str) or '/' not in c[1]:
            continue  # shapes without a file path are validated by their own suite
        p = root / c[1]
        checked += 1
        if not p.is_file() or c[2] not in p.read_text(encoding='utf-8'):
            errors.append('%s: anchor of %s missing in %s' % (f.name, c[0], c[1]))
print(json.dumps({'status': 'MUTANT_ANCHORS_FAIL' if errors else 'MUTANT_ANCHORS_PASS', 'checked': checked, 'errors': errors}, indent=1))
sys.exit(1 if errors else 0)
