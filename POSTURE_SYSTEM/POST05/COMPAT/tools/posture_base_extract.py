"""Extractor DETERMINISTA y SANEADO de logs reales de la serie Enemy Posture System (POST04 y posteriores que conserven sus lineas).

Lee logs crudos (que NUNCA se versionan) y emite solo campos de una lista blanca. Reconstruye cada informe de impacto uniendo
POST03_HIT_EVENT con POST04_POSTURE_HIT por (event, id, generation) y recalcula de forma independiente la contabilidad de POST04.
Cualquier incoherencia (serial duplicado, golpe sin decision de postura, aritmetica, latch repetido, contadores que bajan entre
resumenes periodicos, conservacion falsa...) es un FAIL: se lista en `violations` y la salida del programa es != 0.
Uso: extract_posture_runtime.py --log ETIQUETA=RUTA [--log ...] --expect ETIQUETA=sha256:bytes:lineas --output-dir DIR
     [--module dev.posture.post04_numeric_observer] [--extra-tags POST05_]"""
import argparse
import hashlib
import json
import os
import re
import sys
from pathlib import Path

SCHEMA_VERSION = 1
FAMILIES = ['SWORD', 'SHIELD_ATTACK', 'ARROW', 'BOMB', 'IRON_BALL', 'BOOMERANG', 'AT_TYPE_40', 'SLINGSHOT', 'OTHER_VERIFIED', 'AMBIGUOUS', 'UNAVAILABLE']
WEIGHT = {'SWORD': 25, 'IRON_BALL': 50}
SKIPS = ['STALE_GENERATION', 'NOT_ADMITTED_OR_UNBOUND', 'ACTOR_DEAD', 'LETHAL', 'UNKNOWN_FAMILY', 'AMBIGUOUS', 'ZERO_WEIGHT_POLICY', 'FROZEN_UNVERIFIED', 'BREAK_READY_LATCHED']
BASE_TAGS = {'POST03_1_INIT', 'POST03_TARGET', 'POST03_HOOK_INSTALLED', 'POST03_INSTANCE_FIRST_SEEN', 'POST03_INSTANCE_DEATH_SEEN', 'POST03_INSTANCE_RETIRED',
             'POST03_INSTANCE_UNVERIFIED', 'POST03_INSTANCE_REVALIDATED', 'POST03_ADMISSION_CHANGED', 'POST03_HIT_EVENT', 'POST03_SUMMARY', 'POST03_HIT_SUMMARY',
             'POST03_1_COLLIDER_DIAG', 'POST03_CONSERVATION', 'POST03_SHUTDOWN', 'POST03_DISABLED', 'POST03_HOOK_FAILED', 'POST03_CAPACITY_EXHAUSTED',
             'POST03_INVALID_KEY', 'POST04_INIT', 'POST04_POSTURE_BIND', 'POST04_POSTURE_HIT', 'POST04_POSTURE_REGEN', 'POST04_BREAK_READY',
             'POST04_POSTURE_RETIRE', 'POST04_SUMMARY', 'POST04_CONSERVATION'}
TAG_RE = re.compile(r'\] (POST0\w*)\b')
KV_RE = re.compile(r'(\w+(?:\([^)\s]*\))?)=(\S+)')
VAL_OK = re.compile(r'^[A-Za-z0-9_+.\-/>]+(?:::[A-Za-z0-9_]+)?$')  # sin '\\', ':' sueltos ni espacios: no caben rutas
HIT_FIELDS = {'event', 'call', 'id', 'generation', 'life', 'kind', 'branch', 'cc_at_check', 'origin', 'tg', 'dual', 'distinct', 'collider_src', 'collider', 'sel_class',
              'raw_class', 'family', 'native_first', 'basis', 'at', 'hp', 'lethal', 'mode', 'old_mode', 'state', 'cd', 'cutdown', 'eligible', 'reason'}
ITEM_FIELDS = {
    'POST04_POSTURE_BIND': {'id', 'generation', 'initial', 'tick', 'reason'},
    'POST04_POSTURE_HIT': {'id', 'generation', 'event', 'family', 'at', 'eligible', 'lethal', 'before', 'delta', 'after', 'action', 'reason', 'tick'},
    'POST04_POSTURE_REGEN': {'id', 'generation', 'from', 'to', 'ticks', 'cause', 'tick'},
    'POST04_BREAK_READY': {'id', 'generation', 'before', 'after', 'threshold', 'cause', 'game_action', 'tick'},
    'POST04_POSTURE_RETIRE': {'id', 'generation', 'reason', 'final', 'latched', 'dead', 'delete_claim', 'tick'},
}
# patrones EXACTOS por campo (la lista blanca es por campo, no generica)
ENUM = re.compile(r'^[A-Z][A-Z0-9_]*$')
NUM = re.compile(r'^\d+$')
CLS = '(?:null|too_low|mod8_0|mod8_4|mod4_bad)'
FIELD_RX = {'at': re.compile(r'^(?:0x[0-9a-f]{8}|UNAVAILABLE)$'), 'tg': re.compile(r'^(?:none|cc0|cc1|cc0\+cc1)$'), 'collider_src': re.compile(r'^(?:none|cc0|cc1)$'),
            'sel_class': re.compile('^' + CLS + '$'), 'raw_class': re.compile('^' + CLS + '/' + CLS + '$'), 'hp': re.compile(r'^-?\d+>-?\d+$'),
            'mode': re.compile(r'^-?\d+>-?\d+$'), 'state': re.compile(r'^-?\d+>-?\d+$'), 'cd': re.compile(r'^-?\d+>-?\d+$'), 'old_mode': re.compile(r'^-?\d+$')}
NUMERIC_FIELDS = {'event', 'call', 'id', 'generation', 'dual', 'distinct', 'lethal', 'cutdown', 'initial', 'tick', 'before', 'delta', 'after', 'from', 'to', 'ticks',
                  'threshold', 'final', 'latched', 'dead', 'delete_claim'}


def validate_fields(kv, where):
    for k, val in kv.items():
        rx = FIELD_RX.get(k) or (NUM if k in NUMERIC_FIELDS else ENUM)
        if not rx.match(val):
            raise ValueError('%s: field %s has a value outside its allowlist pattern' % (where, k))


MOD_RE = re.compile(r"\[INFO \| ([\w.]+)\] found '([^']{1,80})' v([\w.\-]{1,40})(?: by |$)")
PRIVACY = [re.compile(p) for p in (r'[A-Za-z]:\\', r'\\Users\\', r'OneDrive', r'AppData', r'Documentos', r'/mnt/', r'/home/', r'~\\', r'\b[0-9a-fA-F]{12,}\b',
                                   r'\b\d{1,3}(?:\.\d{1,3}){3}\b', r'0x[0-9a-fA-F]{9,}', r'\.dusk\b')]
# valores instantaneos (pueden bajar); todo lo demas de un resumen es acumulativo y nunca puede disminuir
GAUGES = {'active', 'bound_now', 'latched_now', 'dead_now', 'frozen_now', 'pending', 'units_live', 'epoch', 'max_active'}
SUM_KEYS_SKIP = 'skipped(stale/unbound/dead/lethal/unknown_fam/ambiguous/zero_weight/frozen/latched)'
TSV_COLS = ['session', 'event', 'id', 'generation', 'kind', 'family', 'at', 'tg', 'sel_class', 'hp_pre', 'hp_post', 'lethal', 'mode_pre', 'mode_post', 'state_pre',
            'state_post', 'posture_action', 'posture_reason', 'before', 'delta', 'after', 'break_ready', 'tick']


def sha(b):
    return hashlib.sha256(b).hexdigest()


def kv_of(body):
    d = {}
    for k, v in KV_RE.findall(body):
        if not VAL_OK.match(v):
            raise ValueError('value outside allowlist pattern in field %s' % k)
        d[k] = v
    return d


def vec(v):
    return [int(x) for x in v.split('/')] if re.fullmatch(r'\d+(/\d+)*', v) else None


def summary_of(kv):
    out = {}
    for k, v in kv.items():
        if k in ('v', 'tag'):
            continue
        vv = vec(v)
        if vv is not None and ('(' in k or '/' in v):
            out[k] = vv
        elif v.isdigit():
            out[k] = int(v)
    return out


def parse(path, expect, module, extra_tags):
    raw = Path(path).read_bytes()
    lines = raw.decode('utf-8', errors='replace').splitlines()
    got = {'sha256': sha(raw), 'bytes': len(raw), 'splitlines': len(lines)}
    for k, v in got.items():
        if expect.get(k) is not None and expect[k] != v:
            raise SystemExit('%s mismatch for %s: %s' % (k, Path(path).name, v))
    s = {'file_name': Path(path).name, **got, 'hits': [], 'items': [], 'periodic': [], 'shutdown': {}, 'mods': [], 'warnings': {}}
    mtag = '| %s] ' % module
    cdi = tex = perf = 0
    for n, l in enumerate(lines, 1):
        m = TAG_RE.search(l)
        if m and mtag in l:
            tag = m.group(1)
            if tag not in BASE_TAGS and not any(tag.startswith(p) for p in extra_tags):
                raise ValueError('tag outside allowlist (line %d): %s' % (n, tag))
            if tag in ('POST03_1_INIT', 'POST04_INIT'):  # identificadores de build: solo campos seguros
                kv = {k: v for k, v in KV_RE.findall(l.split(tag, 1)[1]) if k in ('version', 'source_commit', 'host', 'passive', 'writes_to_game', 'game_actions', 'policy_id', 'capacity')}
                s.setdefault('init', {}).update(kv)
                continue
            body = l.split(tag, 1)[1]
            kv = kv_of(body)
            if tag == 'POST03_HIT_EVENT':
                if set(kv) != HIT_FIELDS:
                    raise ValueError('unexpected POST03_HIT_EVENT fields (line %d): %s' % (n, sorted(set(kv) ^ HIT_FIELDS)))
                validate_fields(kv, 'line %d' % n)
                if kv['family'] not in FAMILIES:
                    raise ValueError('family outside allowlist (line %d)' % n)
                h = {k: kv[k] for k in ('kind', 'family', 'at', 'tg', 'sel_class', 'life', 'eligible')}
                for k in ('event', 'id', 'generation', 'lethal', 'dual'):
                    h[k] = int(kv[k])
                for k in ('hp', 'mode', 'state', 'cd'):
                    a, b = kv[k].split('>')
                    h[k + '_pre'], h[k + '_post'] = int(a), int(b)
                h['line'] = n
                s['hits'].append(h)
            elif tag in ITEM_FIELDS:
                if set(kv) != ITEM_FIELDS[tag]:
                    raise ValueError('unexpected %s fields (line %d): %s' % (tag, n, sorted(set(kv) ^ ITEM_FIELDS[tag])))
                validate_fields(kv, 'line %d' % n)
                it = {'kind': tag[7:], 'line': n}
                for k, v in kv.items():
                    it[k] = int(v) if re.fullmatch(r'-?\d+', v) and k not in ('at',) else v
                s['items'].append(it)
            elif tag in ('POST03_SUMMARY', 'POST03_HIT_SUMMARY', 'POST03_1_COLLIDER_DIAG', 'POST03_CONSERVATION', 'POST04_SUMMARY', 'POST04_CONSERVATION'):
                t = kv.get('tag')
                rec = summary_of(kv)
                if t == 'SHUTDOWN':
                    if tag in s['shutdown']:
                        raise ValueError('duplicate SHUTDOWN %s' % tag)
                    s['shutdown'][tag] = rec
                elif t == 'PERIODIC':
                    s['periodic'].append((tag, rec))
                else:
                    raise ValueError('summary with unknown tag')
            elif tag == 'POST03_TARGET':
                s['inline_sites_flag'] = re.search(r'inline_sites_flag=(\d)', body).group(1)  # 1.a aparicion: la leyenda final no cuenta
                s['resolve'] = kv.get('resolve')
            elif tag == 'POST03_SHUTDOWN':
                s['module_shutdown'] = True
            elif tag in ('POST03_DISABLED', 'POST03_HOOK_FAILED'):
                s['module_disabled_or_failed'] = True
            continue
        mm = MOD_RE.search(l)
        if mm:
            s['mods'].append({'id': mm.group(1), 'version': mm.group(3)})
        mm = re.search(r'\[INFO \| dusk::mods::loader\] (\d+)/(\d+) mod\(s\) active', l)
        if mm:
            s['mods_active'] = '%s/%s' % mm.groups()
        if 'all mods unloaded' in l:
            s['all_mods_unloaded'] = True
        if l.startswith('[WARNING | dusk] CalcDivideInfo'):
            cdi += 1
        if l.startswith('[WARNING | aurora::texture] texture_replacement:'):
            tex += 1
        if 'event=PERF_WINDOW' in l:
            perf += 1
        if 'HM203C SUMMARY(shutdown): events=' in l:
            mm = re.search(r'events=(\d+) valid=(\d+).*HP relation: match=(\d+) diff=(\d+)', l)
            if mm:
                s['hm'] = dict(zip(('events', 'valid', 'hp_match', 'hp_diff'), map(int, mm.groups())))
        if 'BARS203 CP03 shutdown' in l:
            mm = re.search(r'barsDrawn=(\d+)', l)
            s['bars_drawn'] = int(mm.group(1)) if mm else None
    s['warnings'] = {'CalcDivideInfo_lines': cdi, 'texture_replacement_lines': tex, 'PERF_WINDOW_lines_telemetry': perf}
    return s


def check(s):
    """Reconstruccion independiente. Devuelve (filas, derivados, violaciones)."""
    v = []
    sd = s['shutdown']
    for t in ('POST03_SUMMARY', 'POST03_HIT_SUMMARY', 'POST03_CONSERVATION', 'POST04_SUMMARY', 'POST04_CONSERVATION'):
        if t not in sd:
            v.append('missing SHUTDOWN %s (truncated or crashed session)' % t)
    if v:
        return [], {}, v
    p3, p4, c3, c4 = sd['POST03_HIT_SUMMARY'], sd['POST04_SUMMARY'], sd['POST03_CONSERVATION'], sd['POST04_CONSERVATION']
    # --- resumenes acumulativos: ningun contador de un PERIODIC posterior puede ser menor, ni superar al SHUTDOWN ---
    last = {}
    for tag, rec in s['periodic'] + [(t, r) for t, r in sd.items()]:
        prev = last.get(tag, {})
        for k, val in rec.items():
            if isinstance(val, int) and k in prev and isinstance(prev[k], int) and val < prev[k] and k not in GAUGES:
                v.append('%s counter %s decreased between summaries (%d -> %d)' % (tag, k, prev[k], val))
        last[tag] = rec
    # --- serials unicos y correlacion POST03 <-> POST04 ---
    hits = {}
    for h in s['hits']:
        if h['event'] in hits:
            v.append('duplicate POST03_HIT_EVENT serial %d' % h['event'])
        hits[h['event']] = h
    if sorted(hits) != list(range(1, len(hits) + 1)):
        v.append('POST03_HIT_EVENT serials are not contiguous from 1 (lost or reordered events)')
    phit = {}
    for it in s['items']:
        if it['kind'] != 'POSTURE_HIT':
            continue
        if it['event'] in phit:
            v.append('two POST04_POSTURE_HIT for event %d (double consumption)' % it['event'])
        phit[it['event']] = it
    complete = p4.get('log(written/dropped)', [0, 0])[1] == 0
    rows, latched, last_after, broke, apply_count, skip_count = [], set(), {}, {}, 0, {k: 0 for k in SKIPS}
    for ev in sorted(hits):
        h = hits[ev]
        key = (h['id'], h['generation'])
        p = phit.get(ev)
        if h['kind'] in ('FINISH_NATIVE_OBSERVED', 'FINISH_UNEXPECTED_STATE'):
            if p:
                v.append('Finish event %d has a posture decision' % ev)
        elif h['kind'] == 'HIT_CONFIRMED':
            if not p:
                if complete:
                    v.append('HIT_CONFIRMED %d without POST04_POSTURE_HIT' % ev)
            else:
                if (p['id'], p['generation'], p['family'], p['at']) != (h['id'], h['generation'], h['family'], h['at']):
                    v.append('event %d: POST03/POST04 identity or family mismatch' % ev)
                if p['lethal'] != h['lethal']:
                    v.append('event %d: lethal flag mismatch' % ev)
                if p['before'] + p['delta'] != p['after'] or not 0 <= p['after'] <= 100:
                    v.append('event %d: bad arithmetic' % ev)
                w = WEIGHT.get(h['family'], 0)
                expect = None
                if h['lethal']:
                    expect = 'LETHAL'
                elif w == 0:
                    expect = 'ZERO_WEIGHT_POLICY'
                elif key in latched:
                    expect = 'BREAK_READY_LATCHED'
                if p['action'] == 'APPLY':
                    apply_count += 1
                    if expect is not None:
                        v.append('event %d applied but policy says %s' % (ev, expect))
                    if p['delta'] != min(w, 100 - p['before']) or p['delta'] <= 0:
                        v.append('event %d: delta %d does not match weight %d' % (ev, p['delta'], w))
                    if key in last_after and p['before'] > last_after[key]:
                        v.append('event %d: posture rose without a hit' % ev)
                    last_after[key] = p['after']
                    if p['before'] < 100 <= p['after']:
                        latched.add(key)
                else:
                    if p['reason'] in skip_count:
                        skip_count[p['reason']] += 1
                    else:
                        v.append('event %d: unknown skip reason' % ev)
                    if expect is not None and p['reason'] != expect:
                        v.append('event %d: skip reason %s, expected %s' % (ev, p['reason'], expect))
                    if p['delta'] != 0 or p['before'] != p['after']:
                        v.append('event %d: skip with non-zero delta' % ev)
        rows.append([s['label'], ev, h['id'], h['generation'], h['kind'], h['family'], h['at'], h['tg'], h['sel_class'], h['hp_pre'], h['hp_post'], h['lethal'], h['mode_pre'],
                     h['mode_post'], h['state_pre'], h['state_post'], p['action'] if p else '', p['reason'] if p else '', p['before'] if p else '', p['delta'] if p else '',
                     p['after'] if p else '', 0, p['tick'] if p else ''])
    for ev in phit:
        if ev not in hits:
            v.append('POST04_POSTURE_HIT %d without POST03_HIT_EVENT' % ev)
    # --- marcas de ruptura, regeneracion, retiros ---
    rec_units = 0
    for it in s['items']:
        key = (it['id'], it['generation'])
        if it['kind'] == 'BREAK_READY':
            if it['game_action'] != 'NONE' or it['after'] != 100 or it['before'] >= 100:
                v.append('bad BREAK_READY line')
            broke[key] = broke.get(key, 0) + 1
            for r in rows:
                if (r[2], r[3]) == key and r[16] == 'APPLY' and r[18] < 100 <= r[20] and r[0] == s['label']:
                    r[21] = 1
        elif it['kind'] == 'POSTURE_REGEN':
            if not it['from'] > it['to'] >= 0:
                v.append('bad regen line')
            rec_units += it['from'] - it['to']
        elif it['kind'] == 'POSTURE_RETIRE':
            if it['delete_claim'] == 1 and it['reason'] != 'LOOKUP_ABSENT_VERIFIED':
                v.append('delete_claim without verified absence')
    if any(c != 1 for c in broke.values()):
        v.append('more than one BREAK_READY on one generation')
    if set(broke) != latched:
        v.append('BREAK_READY lines do not match reconstructed threshold crossings')
    # --- contabilidad recalculada vs. declarada ---
    sk = p4.get(SUM_KEYS_SKIP, [])
    units = p4.get('units(added/recovered/closed/live)', [0, 0, 0, 0])
    added = sum(r[19] for r in rows if r[16] == 'APPLY')
    closed = sum(it['final'] for it in s['items'] if it['kind'] == 'POSTURE_RETIRE')
    checks = [('confirmed', sum(1 for h in hits.values() if h['kind'] == 'HIT_CONFIRMED'), p3.get('confirmed')),
              ('finish', sum(1 for h in hits.values() if h['kind'].startswith('FINISH')), p3.get('finish')),
              ('POST04.confirmed_consumable==POST03.confirmed', p4.get('confirmed_consumable'), p3.get('confirmed')),
              ('POST04.finish==POST03.finish', p4.get('finish'), p3.get('finish')),
              ('events_emitted', len(hits), p3.get('events_emitted'))]
    if complete:
        checks += [('weighted', apply_count, p4.get('weighted')), ('skips', [skip_count[k] for k in SKIPS], sk), ('crossings', len(latched), p4.get('crossings')),
                   ('units_added', added, units[0]), ('units_recovered', rec_units, units[1]), ('units_closed', closed, units[2])]
    for name, mine, decl in checks:
        if mine != decl:
            v.append('reconstructed %s=%s but summary says %s' % (name, mine, decl))
    if c3.get('ok') != 1 or c3.get('pending') != 0 or c3.get('started') != c3.get('judged'):
        v.append('POST03_CONSERVATION not ok')
    if c4.get('ok') != 1 or any(c4.get(k) != 1 for k in ('e1_consumable', 'e2_reports', 'e3_binds', 'e4_threshold', 'e5_units', 'invariants')):
        v.append('POST04_CONSERVATION not ok')
    # E1..E5 recalculadas desde el resumen (un ok=1 falso no basta)
    if p4['confirmed_consumable'] != p4['weighted'] + sum(sk):
        v.append('E1 does not hold on summary numbers')
    if p4['reports_new'] != p4['finish'] + p4['not_confirmed'] + p4['confirmed_consumable']:
        v.append('E2 does not hold on summary numbers')
    if p4['binds'] != p4['retired'] + p4['bound_now']:
        v.append('E3 does not hold on summary numbers')
    if p4['crossings'] > p4['weighted']:
        v.append('E4 does not hold on summary numbers')
    if units[0] != units[1] + units[2] + units[3]:
        v.append('E5 does not hold on summary numbers')
    for k in ('underflow', 'overflow', 'generation_leak', 'multi_consume', 'ring_dropped'):
        if p4.get(k) != 0:
            v.append('invariant %s != 0' % k)
    if s.get('inline_sites_flag') != '0':
        v.append('inline_sites_flag is not 0')
    if s.get('module', '').endswith('post04_numeric_observer') and (s.get('init', {}).get('writes_to_game') != '0' or s.get('init', {}).get('passive') != '1'):
        v.append('POST04 build does not declare passive=1 writes_to_game=0')
    if not s.get('module_shutdown') or not s.get('all_mods_unloaded'):
        v.append('no normal shutdown (POST03_SHUTDOWN and all mods unloaded)')
    der = {'events': len(hits), 'hit_confirmed': checks[0][1], 'finish': checks[1][1], 'weighted': apply_count, 'skips': skip_count, 'crossings': len(latched),
           'break_ready_lines': sum(broke.values()), 'units_added': added, 'units_recovered_from_regen_lines': rec_units, 'units_closed_from_retire_lines': closed,
           'regen_runs': sum(1 for it in s['items'] if it['kind'] == 'POSTURE_REGEN'), 'binds': sum(1 for it in s['items'] if it['kind'] == 'POSTURE_BIND'),
           'retires': sum(1 for it in s['items'] if it['kind'] == 'POSTURE_RETIRE'), 'log_complete': complete}
    return rows, der, v


def privacy_check(blob, allowed=(), words=()):
    for a in sorted(allowed, key=len, reverse=True):
        blob = blob.replace(a, '')
    for w in words:
        if w and re.search(r'\b' + re.escape(w) + r'\b', blob, re.I):
            raise SystemExit('privacy scan failed on a forbidden word')
    for rx in PRIVACY:
        if rx.search(blob):
            raise SystemExit('privacy scan failed on pattern %s' % rx.pattern)


def run(logs, outdir, expects=None, module='dev.posture.post04_numeric_observer', extra_tags=(), words=()):
    expects = expects or {}
    words = tuple(words) + tuple(w for w in os.environ.get('POSTURE_PRIVACY_WORDS', '').split(',') if w)
    sessions, rows_all, viol = {}, [], []
    for label in sorted(logs):
        s = parse(logs[label], expects.get(label, {}), module, extra_tags)
        s['label'] = label
        s['module'] = module
        rows, der, v = check(s)
        rows_all += rows
        viol += ['%s: %s' % (label, x) for x in v]
        keep = {k: s[k] for k in ('file_name', 'sha256', 'bytes', 'splitlines', 'mods', 'mods_active', 'warnings', 'inline_sites_flag', 'resolve', 'init', 'hm', 'bars_drawn',
                                  'module_shutdown', 'all_mods_unloaded') if k in s}
        keep['shutdown'] = s['shutdown']
        keep['periodic_summaries'] = len(s['periodic'])
        keep['derived'] = der
        sessions[label] = keep
    agg = {k: sum(sessions[x]['derived'].get(k, 0) for x in sessions) for k in ('events', 'hit_confirmed', 'finish', 'weighted', 'crossings', 'units_recovered_from_regen_lines')}
    summary = {'schema_version': SCHEMA_VERSION, 'module': module, 'sessions': sessions, 'aggregate': agg, 'violations': viol}
    tsv = '\t'.join(TSV_COLS) + '\n' + ''.join('\t'.join(str(x) for x in r) + '\n' for r in rows_all)
    manifest = {'schema_version': SCHEMA_VERSION, 'sources': {k: {f: sessions[k][f] for f in ('file_name', 'sha256', 'bytes', 'splitlines')} for k in sessions},
                'note': 'raw logs are NOT versioned; only these metadata and the sanitized derivatives'}
    schema = {'schema_version': SCHEMA_VERSION, 'tsv_columns': TSV_COLS, 'notes': {'break_ready': '1 on the weighted hit that crossed P<100 -> P=100',
                                                                                  'posture_*': 'POST04 decision for that event; empty for Finish'}}
    blobs = {'EVENTS_SANITIZED.tsv': tsv, 'SUMMARY_SANITIZED.json': json.dumps(summary, sort_keys=True, indent=1) + '\n',
             'SOURCE_MANIFEST.json': json.dumps(manifest, sort_keys=True, indent=1) + '\n', 'SCHEMA.json': json.dumps(schema, sort_keys=True, indent=1) + '\n'}
    allowed = [sessions[k]['sha256'] for k in sessions] + [sessions[k].get('init', {}).get(f, '') for k in sessions for f in ('source_commit', 'host')]
    for b in blobs.values():
        privacy_check(b, [a for a in allowed if a], words)
    outdir = Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    for name, b in blobs.items():
        (outdir / name).write_text(b, encoding='utf-8', newline='\n')
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--log', action='append', required=True, help='ETIQUETA=RUTA')
    ap.add_argument('--expect', action='append', default=[], help='ETIQUETA=sha256:bytes:lineas')
    ap.add_argument('--output-dir', required=True)
    ap.add_argument('--module', default='dev.posture.post04_numeric_observer')
    ap.add_argument('--extra-tags', action='append', default=[])
    a = ap.parse_args()
    logs = dict(x.split('=', 1) for x in a.log)
    exp = {}
    for x in a.expect:
        k, val = x.split('=', 1)
        h, b, n = val.split(':')
        exp[k] = {'sha256': h, 'bytes': int(b), 'splitlines': int(n)}
    s = run(logs, a.output_dir, exp, a.module, tuple(a.extra_tags))
    print(json.dumps({'aggregate': s['aggregate'], 'violations': s['violations']}, sort_keys=True))
    return 1 if s['violations'] else 0


if __name__ == '__main__':
    sys.exit(main())
