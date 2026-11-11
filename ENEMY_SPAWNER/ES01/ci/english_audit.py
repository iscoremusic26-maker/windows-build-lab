"""Enemy Spawner 0.1.1 FIX01: everything this mod shows or ships for people to read is English.
Enumerates every string literal of the production source (catalog labels, panel, toasts, errors, logs), the manifest, and every first-party
document under ES01/ (README, docs). Fails on any non-ASCII character in production strings, any Spanish word from the 0.1.0 texts, and any
missing expected English label. Proven by its own negatives. Usage: english_audit.py <project>"""
import json, re, sys
from pathlib import Path
root = Path(sys.argv[1])
PRODUCTION = ['src/catalog.hpp', 'src/spawner_core.hpp', 'src/spawner_adapter.inc', 'src/ui_panel.inc', 'src/mod.cpp']
DOCS = ['README.md'] + sorted(p.relative_to(root).as_posix() for p in (root / 'docs').glob('*.md'))
SPANISH = ['solicitud', 'enviada', 'aparecido', 'invoco', 'invocar', 'invocacion', 'invocación', 'creacion', 'creación', 'clasifico', 'clasificó', 'cargado', 'integracion',
           'integración', 'disponible', 'ya hay', 'limite', 'límite', 'seleccion', 'selección', 'valida', 'válida', 'enemigo', 'enemigos', 'suelo', 'seguro', 'sesion', 'sesión',
           'nativo', 'noche', 'juego', 'delante', 'prueba', 'pruebas', 'instalacion', 'instalación', 'estado', 'entrada', 'entradas', 'aviso', 'cerrado', 'carpeta', 'borra',
           'pestaña', 'busca', 'repite', 'esperado', 'depende', 'sin garantia', 'sin garantía', 'qué', 'por qué', 'también', 'según', 'hasta', 'cuando', 'porque', 'mientras']
EXPECTED = ['"Enemy Spawner"', '"Enemy"', '"Spawn Enemy"', '"Bokoblin Black (Expansion): integration unavailable"', 'Spawn request submitted', 'Enemy spawned',
            'no safe spawn location', 'not in a safe state', 'did not classify the spawned enemy', 'Bokoblin Black integration is unavailable', 'A spawn request is already pending',
            'active spawned enemies reached', 'Invalid enemy selection', 'Bokoblin Expansion is not loaded']
LIT = re.compile(r'"(?:\\.|[^"\\])*"')

def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    out = []
    for line in t.splitlines():
        # drop // comments outside string literals
        s, q, i = line, False, 0
        while i < len(s):
            if s[i] == '"' and (i == 0 or s[i - 1] != '\\'):
                q = not q
            if not q and s.startswith('//', i):
                s = s[:i]
                break
            i += 1
        out.append(s)
    return '\n'.join(out)

def words(t):
    low = t.lower()
    return [w for w in SPANISH if re.search(r'(?<![a-záéíóúñ])' + re.escape(w) + r'(?![a-záéíóúñ])', low)]

def audit(files):
    e = []
    literals = []
    for rel in PRODUCTION:
        for lit in LIT.findall(strip_comments(files[rel])):
            literals.append((rel, lit))
    for rel, lit in literals:
        if any(ord(c) > 0x7e for c in lit):
            e.append('non-ASCII text in %s: %s' % (rel, lit[:60]))
        bad = words(lit)
        if bad:
            e.append('Spanish in %s: %s %s' % (rel, bad, lit[:70]))
    joined = '\n'.join(files[r] for r in PRODUCTION)
    for x in EXPECTED:
        if x not in joined:
            e.append('expected English text missing: ' + x)
    mj = json.loads(files['mod.json'])
    for k in ('name', 'description'):
        if words(mj.get(k, '')) or any(ord(c) > 0x7e for c in mj.get(k, '')):
            e.append('manifest %s is not English' % k)
    for rel in DOCS:
        bad = words(files[rel])
        if bad:
            e.append('Spanish in %s: %s' % (rel, bad[:6]))
    return e, len(literals)

def main():
    files = {r: (root / r).read_text(encoding='utf-8') for r in PRODUCTION + DOCS + ['mod.json']}
    errors, n = audit(files)
    negs = [('src/spawner_adapter.inc', '"A spawn request is already pending."', '"Ya hay una invocacion en curso."'),
            ('src/catalog.hpp', '"Bokoblin (E_OC, native)"', '"Bokoblin (E_OC, nativo)"'),
            ('src/catalog.hpp', '"Bokoblin Black (Expansion): integration unavailable"', '"Bokoblin Black (Expansion): integracion no disponible"'),
            ('src/ui_panel.inc', '"Spawn Enemy"', '"Invocar enemigo"'),
            ('src/ui_panel.inc', '"Enemy";', '"Enemigo";'),
            ('src/spawner_adapter.inc', '"Enemy spawned: %s."', '"%s ha aparecido."'),
            ('src/catalog.hpp', '"Stalhound (night only)"', '"Stalhound (solo de noche)"'),
            ('mod.json', '"Creates real native enemies', '"Crea enemigos nativos reales'),
            ('docs/TEST_GUIDE.md', '# ', '# Prueba humana\n# '),
            ('src/ui_panel.inc', '"Enemy Spawner";', '"Enemy Spawner á";')]
    rejected = 0
    for rel, old, new in negs:
        if files[rel].count(old) < 1:
            errors.append('negative anchor missing: %s %r' % (rel, old))
            continue
        f2 = dict(files)
        f2[rel] = files[rel].replace(old, new, 1)
        if audit(f2)[0]:
            rejected += 1
        else:
            errors.append('negative accepted: %s %r' % (rel, new))
    out = {'status': 'ENGLISH_AUDIT_FAIL' if errors else 'ENGLISH_AUDIT_PASS', 'production_literals': n, 'documents': len(DOCS), 'negatives_rejected': rejected, 'errors': errors}
    print(json.dumps(out, indent=1))
    sys.exit(1 if errors else 0)

main()
