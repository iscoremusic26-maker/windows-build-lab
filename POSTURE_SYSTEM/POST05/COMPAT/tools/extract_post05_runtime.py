"""POST05 / POST05-COMPAT: extractor DETERMINISTA y SANEADO de logs reales de POST05 (0.5.0 y 0.5.1).
Reutiliza la reconstruccion POST03<->POST04 de `posture_base_extract.py` (copia de POST04/evidence_tools/extract_posture_runtime.py) y anade:
  * POST05_DECISION (formato 0.5.0 y 0.5.1, lista blanca por campo), POST05_SUMMARY/LAYOUT/CONSERVATION;
  * POST03_ADMISSION_CHANGED por actor;
  * de Bokoblin Expansion SOLO: event in {VARIANT_ADMITTED, BLUE_GROUP_ON, BLUE_GROUP_OFF} con campos actor_id/source/classified_variant/atp_from/atp_to.
FALLA (violations) si: un HIT_CONFIRMED no tiene exactamente una decision; un Finish tiene decision; familia, modos o postura de la decision no
coinciden con POST03/POST04; SUPPRESSION_APPLIED sin SUPPRESS_ELIGIBLE; contadores de POST05_SUMMARY distintos de lo reconstruido; conservacion != ok.
Uso: extract_post05_runtime.py --log A1=ruta --log B1=ruta --expect A1=sha:bytes:lineas ... --output-dir DIR"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import posture_base_extract as B  # noqa: E402

MODULE = 'dev.posture.post05_selective_reactions'
DEC_050 = {'id', 'generation', 'event', 'family', 'decision', 'reason', 'pre_mode', 'pre_state', 'post_mode', 'post_state', 'cd', 'posture', 'action'}
DEC_051 = DEC_050 | {'resource', 'atp', 'admission'}
ARROW = re.compile(r'^-?\d+>-?\d+$')
ENUM = re.compile(r'^[A-Z][A-Z0-9_]*$')
NUM = re.compile(r'^-?\d+$')
DEC_NUM = {'id', 'generation', 'event', 'pre_mode', 'pre_state', 'post_mode', 'post_state'}
POST05_TAGS = {'POST05_INIT', 'POST05_DECISION', 'POST05_SUMMARY', 'POST05_LAYOUT', 'POST05_CONSERVATION'}
EXP_EVENTS = {'VARIANT_ADMITTED', 'BLUE_GROUP_ON', 'BLUE_GROUP_OFF'}
EXP_FIELDS = {'actor_id': NUM, 'source': ENUM, 'classified_variant': ENUM, 'atp_from': NUM, 'atp_to': NUM}
TSV_COLS = ['session', 'event', 'id', 'generation', 'kind', 'family', 'resource', 'expansion_variant', 'expansion_source', 'admission_changes_before', 'pre_mode', 'pre_state',
            'post_mode', 'post_state', 'cd', 'hp_pre', 'hp_post', 'posture', 'decision', 'reason', 'action']


def parse_extra(path):
    lines = Path(path).read_bytes().decode('utf-8', errors='replace').splitlines()
    out = {'decisions': [], 'admission': [], 'expansion': [], 'summary': {}, 'conservation': {}, 'layout': None, 'init': {}}
    mtag = '| %s] ' % MODULE
    for n, l in enumerate(lines, 1):
        if mtag in l:
            t = re.search(r'\] (POST05_\w*)', l)
            if t and t.group(1) not in POST05_TAGS:  # lista EXACTA (la base solo admite el prefijo POST05_)
                raise ValueError('POST05 tag outside allowlist (line %d): %s' % (n, t.group(1)))
            if '] POST05_DECISION ' in l:
                kv = dict(re.findall(r'(\w+)=(\S+)', l.split('POST05_DECISION', 1)[1]))
                if set(kv) not in (DEC_050, DEC_051):
                    raise ValueError('POST05_DECISION fields outside allowlist (line %d): %s' % (n, sorted(set(kv) ^ DEC_050)))
                for k, v in kv.items():
                    rx = ARROW if k in ('cd', 'posture', 'atp') else NUM if k in DEC_NUM else ENUM
                    if not rx.match(v):
                        raise ValueError('POST05_DECISION field %s outside pattern (line %d)' % (k, n))
                d = {k: (int(v) if k in DEC_NUM else v) for k, v in kv.items()}
                d['line'] = n
                out['decisions'].append(d)
            elif '] POST03_ADMISSION_CHANGED ' in l:
                kv = dict(re.findall(r'(\w+)=(\S+)', l.split('POST03_ADMISSION_CHANGED', 1)[1]))
                out['admission'].append({'call': int(kv['call']), 'id': int(kv['id']), 'generation': int(kv['generation']), 'from': kv['from'], 'to': kv['to'], 'line': n})
            elif '] POST05_SUMMARY ' in l and 'tag=SHUTDOWN' in l:
                body = l.split('POST05_SUMMARY', 1)[1]
                out['summary'] = {k: int(v) for k, v in re.findall(
                    r'\b(decisions|duplicates|not_applicable|allow_vanilla|skip_unsafe|eligible|confirmed|attempted|applied|recheck_failed|verify_failed|not_attempted|session_disabled)=(\d+)', body)}
            elif '] POST05_CONSERVATION ' in l and 'tag=SHUTDOWN' in l:
                out['conservation'] = {k: int(v) for k, v in re.findall(r'\b(ok|r\d_\w+)=(\d+)', l)}
            elif '] POST05_LAYOUT ' in l and 'tag=SHUTDOWN' in l:
                m = re.search(r'layout=([A-Z_]+)', l)
                out['layout'] = m.group(1) if m else None
            elif '] POST05_INIT ' in l:
                out['init'] = {k: v for k, v in re.findall(r'(\w+)=(\S+)', l.split('POST05_INIT', 1)[1])
                               if k in ('version', 'policy_id', 'writes_to_game', 'admission_policy', 'hook_skip_original')}
        elif '| dev.bokoblin.expansion] ' in l:
            m = re.search(r'\bevent=([A-Z_]+)', l)
            if m and m.group(1) in EXP_EVENTS:
                kv = {k: v for k, v in re.findall(r'(\w+)=(\S+)', l) if k in EXP_FIELDS}
                for k, v in kv.items():
                    if not EXP_FIELDS[k].match(v):
                        raise ValueError('Expansion field %s outside pattern (line %d)' % (k, n))
                kv = {k: (int(v) if EXP_FIELDS[k] is NUM else v) for k, v in kv.items()}
                kv['event'] = m.group(1)
                kv['line'] = n
                out['expansion'].append(kv)
    return out


def check(base, extra):
    v = []
    hits = {h['event']: h for h in base['hits']}
    phit = {it['event']: it for it in base['items'] if it['kind'] == 'POSTURE_HIT'}
    decs = {}
    for d in extra['decisions']:
        if d['event'] in decs:
            v.append('two POST05_DECISION lines for event %d' % d['event'])
        decs[d['event']] = d
    for ev, h in sorted(hits.items()):
        d = decs.get(ev)
        if h['kind'] == 'HIT_CONFIRMED':
            if not d:
                v.append('HIT_CONFIRMED %d without POST05_DECISION' % ev)
                continue
            if (d['id'], d['generation'], d['family']) != (h['id'], h['generation'], h['family']):
                v.append('event %d: decision identity/family differs from POST03' % ev)
            if (d['pre_mode'], d['post_mode'], d['pre_state'], d['post_state']) != (h['mode_pre'], h['mode_post'], h['state_pre'], h['state_post']):
                v.append('event %d: decision modes differ from POST03 snapshot' % ev)
            p = phit.get(ev)
            if p and d['posture'] != '%d>%d' % (p['before'], p['after']):
                v.append('event %d: decision posture %s != POST04 %d>%d' % (ev, d['posture'], p['before'], p['after']))
        elif d:
            v.append('non-confirmed event %d has a decision line' % ev)
    for ev, d in decs.items():
        if ev not in hits:
            v.append('decision for unknown event %d' % ev)
        if d['action'] == 'SUPPRESSION_APPLIED' and d['decision'] != 'SUPPRESS_ELIGIBLE':
            v.append('event %d: SUPPRESSION_APPLIED without SUPPRESS_ELIGIBLE' % ev)
    s = extra['summary']
    if not s:
        v.append('missing POST05_SUMMARY SHUTDOWN')
    else:
        n_conf = sum(1 for h in hits.values() if h['kind'] == 'HIT_CONFIRMED')
        n_fin = len(hits) - n_conf
        mine = {'decisions': n_conf + n_fin, 'confirmed': n_conf, 'not_applicable': n_fin,
                'allow_vanilla': sum(d['decision'] == 'ALLOW_VANILLA' for d in decs.values()), 'skip_unsafe': sum(d['decision'] == 'SKIP_UNSAFE' for d in decs.values()),
                'eligible': sum(d['decision'] == 'SUPPRESS_ELIGIBLE' for d in decs.values()), 'applied': sum(d['action'] == 'SUPPRESSION_APPLIED' for d in decs.values())}
        for k, val in mine.items():
            if s.get(k) != val:
                v.append('POST05_SUMMARY %s=%s but reconstructed %s' % (k, s.get(k), val))
        if s.get('verify_failed', 0) or s.get('recheck_failed', 0):
            v.append('write recheck/verify failures present')
    if extra['conservation'].get('ok') != 1:
        v.append('POST05_CONSERVATION not ok')
    return decs, v


def run(logs, outdir, expects=None, words=()):
    expects = expects or {}
    words = tuple(words) + tuple(w for w in os.environ.get('POSTURE_PRIVACY_WORDS', '').split(',') if w)
    sessions, rows, viol = {}, [], []
    for label in sorted(logs):
        s = B.parse(logs[label], expects.get(label, {}), MODULE, ('POST05_',))
        s['label'] = label
        s['module'] = MODULE
        _, der, bv = B.check(s)
        extra = parse_extra(logs[label])
        decs, ev = check(s, extra)
        viol += ['%s: %s' % (label, x) for x in bv + ev]
        variant, source = {}, {}
        for x in extra['expansion']:
            if x['event'] == 'VARIANT_ADMITTED' and 'actor_id' in x:
                variant.setdefault(x['actor_id'], []).append(x.get('classified_variant', 'NONE'))
                source.setdefault(x['actor_id'], []).append(x.get('source', 'NONE'))
        for h in s['hits']:
            d = decs.get(h['event'], {})
            n_adm = sum(1 for a in extra['admission'] if a['id'] == h['id'] and a['generation'] == h['generation'] and a['line'] < h['line'])
            rows.append([label, h['event'], h['id'], h['generation'], h['kind'], h['family'], d.get('resource', 'E_OC'), '/'.join(variant.get(h['id'], ['NO_EXPANSION_LINE'])),
                         '/'.join(source.get(h['id'], ['NO_EXPANSION_LINE'])), n_adm, h['mode_pre'], h['state_pre'], h['mode_post'], h['state_post'],
                         '%d>%d' % (h['cd_pre'], h['cd_post']), h['hp_pre'], h['hp_post'], d.get('posture', ''), d.get('decision', ''), d.get('reason', ''), d.get('action', '')])
        grp = {}
        for x in extra['expansion']:
            if x['event'] in ('BLUE_GROUP_ON', 'BLUE_GROUP_OFF'):
                grp.setdefault(str(x.get('actor_id')), []).append('%s:%s>%s' % (x['event'][11:], x.get('atp_from'), x.get('atp_to')))
        keep = {k: s[k] for k in ('file_name', 'sha256', 'bytes', 'splitlines', 'mods', 'mods_active', 'warnings', 'inline_sites_flag', 'init', 'hm', 'bars_drawn',
                                  'module_shutdown', 'all_mods_unloaded') if k in s}
        keep['post04_derived'] = der
        keep['post05'] = {'init': extra['init'], 'summary': extra['summary'], 'conservation': extra['conservation'], 'layout': extra['layout'],
                          'decisions_by_reason': {r: sum(1 for d in decs.values() if d['reason'] == r) for r in sorted({d['reason'] for d in decs.values()})},
                          'admission_changes': [{k: a[k] for k in ('id', 'generation', 'from', 'to')} for a in extra['admission']],
                          'expansion_variants': {str(k): {'variant': variant[k], 'source': source[k]} for k in sorted(variant)}, 'blue_group_transitions': grp}
        sessions[label] = keep
    summary = {'schema_version': 1, 'module': MODULE, 'sessions': sessions, 'violations': viol}
    tsv = '\t'.join(TSV_COLS) + '\n' + ''.join('\t'.join(str(x) for x in r) + '\n' for r in rows)
    manifest = {'schema_version': 1, 'sources': {k: {f: sessions[k][f] for f in ('file_name', 'sha256', 'bytes', 'splitlines')} for k in sessions},
                'note': 'raw logs are NOT versioned; only these metadata and the sanitized derivatives'}
    blobs = {'POST05_A1_B1_EVENTS_SANITIZED.tsv': tsv, 'POST05_A1_B1_SUMMARY_SANITIZED.json': json.dumps(summary, sort_keys=True, indent=1) + '\n',
             'POST05_A1_B1_SOURCE_MANIFEST.json': json.dumps(manifest, sort_keys=True, indent=1) + '\n'}
    allowed = [sessions[k]['sha256'] for k in sessions] + [sessions[k].get('init', {}).get(f, '') for k in sessions for f in ('source_commit', 'host')]
    for b in blobs.values():
        B.privacy_check(b, [a for a in allowed if a], words)
    outdir = Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    for name, b in blobs.items():
        (outdir / name).write_text(b, encoding='utf-8', newline='\n')
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--log', action='append', required=True)
    ap.add_argument('--expect', action='append', default=[])
    ap.add_argument('--output-dir', required=True)
    a = ap.parse_args()
    logs = dict(x.split('=', 1) for x in a.log)
    exp = {}
    for x in a.expect:
        k, val = x.split('=', 1)
        h, b, n = val.split(':')
        exp[k] = {'sha256': h, 'bytes': int(b), 'splitlines': int(n)}
    s = run(logs, a.output_dir, exp)
    print(json.dumps({'violations': s['violations'], 'summaries': {k: v['post05']['summary'] for k, v in s['sessions'].items()}}, sort_keys=True))
    return 1 if s['violations'] else 0


if __name__ == '__main__':
    sys.exit(main())
