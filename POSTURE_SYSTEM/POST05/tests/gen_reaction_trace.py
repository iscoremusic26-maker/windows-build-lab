"""POST05: genera tests/reaction_trace.inc desde el TSV SANEADO de los 27 informes reales de POST04 A1/B1 (nunca desde un log crudo).
La decision esperada de POST05 se calcula aqui con reglas escritas de forma INDEPENDIENTE a reaction_core.hpp (politica en prosa):
elegible <=> SWORD, ponderado por POST04 (APPLY) y sin llegar a 100, no letal, PRE ATTACK(4) en estado 0/1/2, POST DAMAGE(5) en estado 0/1/2.
Supuestos declarados: cd=0>10 en los golpes procesados (verificado en los bytes reales) y precondiciones auxiliares cumplidas (no registradas en POST04).
Uso: gen_reaction_trace.py <events.tsv> <salida.inc>   (determinista)"""
import csv
import hashlib
import sys

FAM = {'UNAVAILABLE': 0, 'SWORD': 1, 'SHIELD_ATTACK': 2, 'ARROW': 3, 'BOMB': 4, 'IRON_BALL': 5, 'BOOMERANG': 6, 'AT_TYPE_40': 7, 'SLINGSHOT': 8, 'OTHER_VERIFIED': 9, 'AMBIGUOUS': 10}


def main(src, out):
    raw = open(src, 'rb').read()
    rows = list(csv.DictReader(raw.decode().splitlines(), delimiter='\t'))
    lines, eligible = [], 0
    for r in rows:
        finish = r['kind'] != 'HIT_CONFIRMED'
        apply_ = r['posture_action'] == 'APPLY'
        after = int(r['after']) if r['after'] != '' else 0
        e = (not finish and r['family'] == 'SWORD' and apply_ and after < 100 and r['lethal'] == '0' and r['mode_pre'] == '4' and r['state_pre'] in ('0', '1', '2')
             and r['mode_post'] == '5' and r['state_post'] in ('0', '1', '2'))
        eligible += e
        lines.append('{%d,%s,%s,%s,%d,%s,%d,%s,%s,%s,%s,%s,%s,%d,%d,%d,%s},' % (0 if r['session'] == 'A1' else 1, r['event'], r['id'], r['generation'], finish, r['lethal'],
                     FAM[r['family']], r['at'] if r['at'].startswith('0x') else '0', r['hp_post'], r['mode_pre'], r['mode_post'], r['state_pre'], r['state_post'],
                     1 if apply_ else 0, after, 1 if e else 0, r['tick'] or '0'))
    body = '// GENERADO por tests/gen_reaction_trace.py desde data/post04_a1_b1_events.tsv (sha256 %s). No editar.\n' % hashlib.sha256(raw).hexdigest()
    body += '// {session, event, id, generation, finish, lethal, family, at, hp_post, mode_pre, mode_post, state_pre, state_post, posture_apply, after, post05_eligible, posture_tick}\n'
    body += 'static const TraceRow kTrace[] = {\n' + '\n'.join(lines) + '\n};\nstatic const unsigned kTraceCount = %d;\nstatic const unsigned kTraceExpectedEligible = %d;\n' % (len(lines), eligible)
    open(out, 'w', newline='\n').write(body)
    print({'rows': len(lines), 'eligible': eligible, 'eligible_events': [(r['session'], r['event']) for r, l in zip(rows, lines) if l.split(',')[15] == '1']})


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
