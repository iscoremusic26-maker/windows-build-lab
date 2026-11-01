"""POST04: genera tests/real_trace.inc a partir del TSV SANEADO de los 36 eventos reales A1/B1 (nunca de un log crudo).
Calcula con una implementacion INDEPENDIENTE (reglas en prosa, sin compartir codigo con posture_core.hpp) la decision esperada para cada evento.
Es una repeticion de ACUMULACION solamente: los logs no registran ticks, asi que NO verifica recuperacion.
Uso: gen_real_trace.py <events.tsv> <salida.inc>   (determinista)"""
import csv
import hashlib
import sys

FAM = {'UNAVAILABLE': 0, 'SWORD': 1, 'SHIELD_ATTACK': 2, 'ARROW': 3, 'BOMB': 4, 'IRON_BALL': 5, 'BOOMERANG': 6, 'AT_TYPE_40': 7, 'SLINGSHOT': 8, 'OTHER_VERIFIED': 9, 'AMBIGUOUS': 10}
WEIGHT = {'SWORD': 25, 'IRON_BALL': 50}
# 0 = ponderado; indices de Skip (0 stale,1 unbound,2 dead,3 lethal,4 unknown_fam,5 ambiguous,6 zero_weight,7 frozen,8 latched); 20 = finish
ACT = {'APPLY': 0, 'LETHAL': 3, 'UNKNOWN': 4, 'AMBIGUOUS': 5, 'ZERO': 6, 'LATCHED': 8, 'FINISH': 20}


def main(src, out):
    raw = open(src, 'rb').read()
    rows = list(csv.DictReader(raw.decode().splitlines(), delimiter='\t'))
    st = {}
    lines = []
    totals = {}
    for r in rows:
        key = (r['session'], r['id'], r['generation'])
        s = st.setdefault(key, {'p': 0, 'latched': False, 'dead': False})
        if r['kind'] != 'HIT_CONFIRMED':
            act, p = 'FINISH', s['p']
        elif int(r['lethal']) == 1:
            act, p = 'LETHAL', s['p']
            s['dead'] = True
        elif r['family'] in ('UNAVAILABLE',):
            act, p = 'UNKNOWN', s['p']
        elif r['family'] == 'AMBIGUOUS':
            act, p = 'AMBIGUOUS', s['p']
        elif r['family'] not in WEIGHT:
            act, p = 'ZERO', s['p']
        elif s['latched']:
            act, p = 'LATCHED', s['p']
        else:
            before = s['p']
            s['p'] = min(100, before + WEIGHT[r['family']])
            p, act = s['p'], 'APPLY'
            if before < 100 and s['p'] >= 100:
                s['latched'] = True
        totals[(r['session'], act)] = totals.get((r['session'], act), 0) + 1
        lines.append('{%d,%s,%s,%s,%d,%d,%d,%d,%s,%d,%d,%d},' % (0 if r['session'] == 'A1' else 1, r['ord'], r['id'], r['generation'], 1 if r['life'] == 'DEATH_SEEN' else 0,
                     0 if r['kind'] == 'HIT_CONFIRMED' else 1, int(r['lethal']), int(r['hp_post']), r['at'] if r['at'].startswith('0x') else '0', FAM[r['family']], ACT[act], p))
    body = '// GENERADO por tests/gen_real_trace.py desde data/real_trace_a1_b1_events.tsv (sha256 %s). No editar.\n' % hashlib.sha256(raw).hexdigest()
    body += '// {session(0=A1,1=B1), ord, id, generation, life_death, finish, lethal, hp_post, at, family, expected_action, expected_posture_after}\n'
    body += 'static const RealRow kRealRows[] = {\n' + '\n'.join(lines) + '\n};\n'
    body += 'static const unsigned kRealRowCount = %d;\n' % len(lines)
    for sess in ('A1', 'B1'):
        for act in ACT:
            body += 'static const unsigned kExp_%s_%s = %d;\n' % (sess, act, totals.get((sess, act), 0))
    open(out, 'w', newline='\n').write(body)
    print({k: v for k, v in sorted(totals.items())})


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
