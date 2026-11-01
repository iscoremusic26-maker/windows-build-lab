"""POST04: extractor DETERMINISTA y SANEADO de un log real de POST04 (para el cierre de la sesion humana A/B). El log crudo NUNCA se versiona.
Solo emite campos de una lista blanca de las lineas POST04_* y de los contadores SHUTDOWN de POST03 (para comprobar que el consumo concuerda con el observador).
Comprueba invariantes: ecuaciones de conservacion, coherencia aritmetica de cada golpe ponderado, un solo BREAK_READY por ciclo, delete_claim solo con ausencia verificada.
Uso: extract_post04_evidence.py --log L --output-dir DIR [--expect-sha H --expect-bytes N --expect-lines N]   (salida != 0 si hay violaciones)"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

FAMILIES = ['SWORD', 'SHIELD_ATTACK', 'ARROW', 'BOMB', 'IRON_BALL', 'BOOMERANG', 'AT_TYPE_40', 'SLINGSHOT', 'OTHER_VERIFIED', 'AMBIGUOUS', 'UNAVAILABLE']
WEIGHT = {'SWORD': 25, 'IRON_BALL': 50}
SKIPS = ['STALE_GENERATION', 'NOT_ADMITTED_OR_UNBOUND', 'ACTOR_DEAD', 'LETHAL', 'UNKNOWN_FAMILY', 'AMBIGUOUS', 'ZERO_WEIGHT_POLICY', 'FROZEN_UNVERIFIED', 'BREAK_READY_LATCHED']
TAGS = {'POST04_INIT', 'POST04_POSTURE_BIND', 'POST04_POSTURE_HIT', 'POST04_POSTURE_REGEN', 'POST04_BREAK_READY', 'POST04_POSTURE_RETIRE', 'POST04_SUMMARY', 'POST04_CONSERVATION'}
TAG_RE = re.compile(r'\] (POST04\w*)\b')
KV_RE = re.compile(r'(\w+(?:\([^)\s]*\))?)=(\S+)')
VAL_OK = re.compile(r'^[A-Za-z0-9_+.\-/>]+(?:::[A-Za-z0-9_]+)?$')  # sin ':' ni '\\' ni espacios: no pueden aparecer rutas
ITEM_FIELDS = {
    'POST04_POSTURE_BIND': {'id', 'generation', 'initial', 'tick', 'reason'},
    'POST04_POSTURE_HIT': {'id', 'generation', 'event', 'family', 'at', 'eligible', 'lethal', 'before', 'delta', 'after', 'action', 'reason', 'tick'},
    'POST04_POSTURE_REGEN': {'id', 'generation', 'from', 'to', 'ticks', 'cause', 'tick'},
    'POST04_BREAK_READY': {'id', 'generation', 'before', 'after', 'threshold', 'cause', 'game_action', 'tick'},
    'POST04_POSTURE_RETIRE': {'id', 'generation', 'reason', 'final', 'latched', 'dead', 'delete_claim', 'tick'},
}
NUMERIC = {'id', 'generation', 'event', 'lethal', 'before', 'delta', 'after', 'tick', 'initial', 'from', 'to', 'ticks', 'threshold', 'final', 'latched', 'dead', 'delete_claim'}
PRIVACY = [re.compile(p) for p in (r'[A-Za-z]:\\', r'\\Users\\', r'OneDrive', r'AppData', r'Documentos', r'/mnt/', r'/home/', r'~\\', r'\b[0-9a-fA-F]{12,}\b',
                                   r'\b\d{1,3}(?:\.\d{1,3}){3}\b', r'0x[0-9a-fA-F]{9,}', r'\.dusk\b')]
TSV_COLS = ['ord', 'kind', 'id', 'generation', 'tick', 'event', 'family', 'at', 'lethal', 'before', 'delta', 'after', 'action', 'reason', 'from', 'to', 'ticks', 'final', 'latched', 'dead', 'delete_claim']


def sha(b):
    return hashlib.sha256(b).hexdigest()


def vec(v):
    return [int(x) for x in v.split('/')] if re.fullmatch(r'\d+(/\d+)*', v) else None


def parse_kv(body):
    out = {}
    for k, v in KV_RE.findall(body):
        if not VAL_OK.match(v):
            raise ValueError('value outside allowlist pattern: %s' % k)
        out[k] = v
    return out


def parse(path, expect):
    raw = Path(path).read_bytes()
    text = raw.decode('utf-8', errors='replace')
    lines = text.splitlines()
    for key, val in (('sha256', sha(raw)), ('bytes', len(raw)), ('splitlines', len(lines))):
        if expect.get(key) is not None and expect[key] != val:
            raise SystemExit('%s mismatch: %s' % (key, val))
    out = {'sha256': sha(raw), 'bytes': len(raw), 'splitlines': len(lines), 'items': [], 'summaries': {}, 'conservation': {}, 'post03': {}}
    for l in lines:
        m = TAG_RE.search(l)
        if m and 'post04_numeric_observer' in l:
            tag = m.group(1)
            if tag not in TAGS:
                raise ValueError('POST04 tag outside allowlist: ' + tag)
            body = l.split(tag, 1)[1]
            kv = parse_kv(body)
            if tag in ITEM_FIELDS:
                if set(kv) != ITEM_FIELDS[tag]:
                    raise ValueError('unexpected fields in %s: %s' % (tag, sorted(set(kv) ^ ITEM_FIELDS[tag])))
                row = {'kind': tag.replace('POST04_', '')}
                for k, v in kv.items():
                    row[k] = int(v) if k in NUMERIC else v
                if tag == 'POST04_POSTURE_HIT' and row['family'] not in FAMILIES:
                    raise ValueError('family outside allowlist')
                out['items'].append(row)
            elif tag in ('POST04_SUMMARY', 'POST04_CONSERVATION'):
                d = {}
                for k, v in kv.items():
                    d[k] = vec(v) if vec(v) is not None and ('(' in k or '/' in v) else (int(v) if v.isdigit() else v)
                (out['summaries'] if tag == 'POST04_SUMMARY' else out['conservation'])[d.get('tag', '?')] = d
            elif tag == 'POST04_INIT':
                out['init'] = {k: v for k, v in kv.items() if k in ('version', 'source_commit', 'policy_id', 'passive', 'writes_to_game', 'game_actions', 'capacity', 'key', 'max_posture',
                                                                   'threshold', 'recover_delay_ticks', 'recover_period_ticks', 'timebase', 'balance', 'log_budget_posture')}
                out['init']['weights'] = kv.get('weights(sword/iron/all_other)')
                out['init']['pause_unverified'] = 'TIMEBASE_PAUSE_UNVERIFIED' in l
            continue
        for tag, key in (('POST03_HIT_SUMMARY', 'hit_summary'), ('POST03_SUMMARY', 'summary')):
            if tag in l and 'tag=SHUTDOWN' in l and 'post04_numeric_observer' in l:
                out['post03'].setdefault(key, {}).update({k: int(v) for k, v in re.findall(r'\b(confirmed|finish|events_emitted|calls)=(\d+)', l.split(tag, 1)[1])})
        if 'POST03_CONSERVATION' in l and 'tag=SHUTDOWN' in l and 'post04_numeric_observer' in l:
            out['post03']['conservation_ok'] = 'ok=1' in l
        if 'inline_sites_flag=' in l and 'POST03_TARGET' in l:
            out['inline_sites_flag'] = re.search(r'inline_sites_flag=(\d)', l).group(1)
        if 'POST03_SHUTDOWN' in l and 'post04_numeric_observer' in l:
            out['shutdown'] = True
    return out


def check(sess):
    v = []
    items = sess['items']
    sm = sess['summaries'].get('SHUTDOWN')
    cs = sess['conservation'].get('SHUTDOWN')
    if not sm or not cs:
        v.append('missing SHUTDOWN summary/conservation')
        return v
    if cs.get('ok') != 1 or any(cs.get(k) != 1 for k in ('e1_consumable', 'e2_reports', 'e3_binds', 'e4_threshold', 'e5_units', 'invariants')):
        v.append('POST04_CONSERVATION not ok')
    for k in ('underflow', 'overflow', 'generation_leak', 'multi_consume'):
        if sm.get(k) != 0:
            v.append('invariant counter %s != 0' % k)
    if sess['init'].get('writes_to_game') != '0' or sess['init'].get('passive') != '1' or sess['init'].get('game_actions') != 'NONE':
        v.append('init does not declare a passive mod')
    skipped = sm.get('skipped(stale/unbound/dead/lethal/unknown_fam/ambiguous/zero_weight/frozen/latched)', [])
    if sum(skipped) + sm['weighted'] != sm['confirmed_consumable']:
        v.append('summary: confirmed_consumable != weighted + skipped')
    p3 = sess['post03'].get('hit_summary', {})
    if p3 and p3.get('confirmed') != sm['confirmed_consumable']:
        v.append('POST04 confirmed_consumable != POST03 confirmed (%s vs %s)' % (sm['confirmed_consumable'], p3.get('confirmed')))
    if p3 and p3.get('finish') != sm['finish']:
        v.append('POST04 finish != POST03 finish')
    if sess['post03'].get('conservation_ok') is False:
        v.append('POST03 conservation not ok')
    complete = sm.get('log(written/dropped)', [0, 0])[1] == 0
    applies = [i for i in items if i['kind'] == 'POSTURE_HIT' and i['action'] == 'APPLY']
    breaks = [i for i in items if i['kind'] == 'BREAK_READY']
    if complete and len(applies) != sm['weighted']:
        v.append('APPLY lines (%d) != weighted (%d)' % (len(applies), sm['weighted']))
    if complete and len(breaks) != sm['crossings']:
        v.append('BREAK_READY lines (%d) != crossings (%d)' % (len(breaks), sm['crossings']))
    gen_id = {}
    last_after = {}
    broke = set()
    for it in items:
        key = (it['id'], it['generation'])
        if gen_id.setdefault(it['generation'], it['id']) != it['id']:
            v.append('generation %d used by two ids' % it['generation'])
        if it['kind'] == 'POSTURE_HIT' and it['action'] == 'APPLY':
            if it['before'] + it['delta'] != it['after'] or not 0 <= it['after'] <= 100 or it['delta'] <= 0:
                v.append('bad apply arithmetic at event %d' % it['event'])
            w = WEIGHT.get(it['family'])
            if w is None or it['delta'] > w or (it['delta'] < w and it['after'] != 100):
                v.append('weight mismatch at event %d (%s +%d)' % (it['event'], it['family'], it['delta']))
            if key in last_after and it['before'] > last_after[key]:
                v.append('posture rose between hits without a hit (event %d)' % it['event'])
            if key in broke:
                v.append('weighted hit after BREAK_READY on the same generation (event %d)' % it['event'])
            last_after[key] = it['after']
        elif it['kind'] == 'POSTURE_HIT':
            if it['reason'] not in SKIPS or it['delta'] != 0:
                v.append('bad skip line at event %d' % it['event'])
            if it['reason'] in ('ZERO_WEIGHT_POLICY',) and it['family'] in WEIGHT:
                v.append('weighted family skipped as zero weight at event %d' % it['event'])
        elif it['kind'] == 'BREAK_READY':
            if it['game_action'] != 'NONE' or it['after'] != 100 or it['before'] >= 100:
                v.append('bad BREAK_READY')
            if key in broke:
                v.append('second BREAK_READY on one generation')
            broke.add(key)
        elif it['kind'] == 'POSTURE_REGEN':
            if not it['from'] > it['to'] >= 0:
                v.append('bad regen line')
            if key in last_after:
                last_after[key] = it['to']
        elif it['kind'] == 'POSTURE_RETIRE':
            if it['delete_claim'] == 1 and it['reason'] != 'LOOKUP_ABSENT_VERIFIED':
                v.append('delete_claim without verified absence')
    return v


def privacy_check(blob, allowed=(), words=()):
    for a in sorted(allowed, key=len, reverse=True):
        blob = blob.replace(a, '')
    for w in words:
        if w and re.search(r'\b' + re.escape(w) + r'\b', blob, re.I):
            raise SystemExit('privacy scan failed on a forbidden word')
    for rx in PRIVACY:
        if rx.search(blob):
            raise SystemExit('privacy scan failed on pattern %s' % rx.pattern)


def run(log, outdir, expect=None, words=()):
    sess = parse(log, expect or {})
    viol = check(sess)
    items = sess.pop('items')
    tsv = '\t'.join(TSV_COLS) + '\n' + ''.join('\t'.join(str(it.get(c, '')) if c != 'ord' else str(n) for c in TSV_COLS) + '\n' for n, it in enumerate(items, 1))
    summary = {'schema_version': 1, 'session': sess, 'item_counts': {k: sum(1 for i in items if i['kind'] == k) for k in sorted({i['kind'] for i in items})}, 'violations': viol}
    blob = json.dumps(summary, sort_keys=True, indent=1) + '\n'
    allowed = [sess['sha256'], sess.get('init', {}).get('source_commit', '')]
    for t in (tsv, blob):
        privacy_check(t, [a for a in allowed if a], words)
    outdir = Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    (outdir / 'POST04_EVENTS_SANITIZED.tsv').write_text(tsv, encoding='utf-8', newline='\n')
    (outdir / 'POST04_SUMMARY_SANITIZED.json').write_text(blob, encoding='utf-8', newline='\n')
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--log', required=True)
    ap.add_argument('--output-dir', required=True)
    ap.add_argument('--expect-sha')
    ap.add_argument('--expect-bytes', type=int)
    ap.add_argument('--expect-lines', type=int)
    ap.add_argument('--forbid-word', action='append', default=[])
    a = ap.parse_args()
    s = run(a.log, a.output_dir, {'sha256': a.expect_sha, 'bytes': a.expect_bytes, 'splitlines': a.expect_lines}, a.forbid_word)
    print(json.dumps({'item_counts': s['item_counts'], 'violations': len(s['violations'])}, sort_keys=True))
    return 1 if s['violations'] else 0


if __name__ == '__main__':
    sys.exit(main())
