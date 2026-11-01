"""POST03.1: extractor reproducible y SANEADO de evidencia de logs reales de Dusklight (sesiones A/B de POST03).
Lee un log crudo (que NUNCA se versiona) y emite SOLO campos de una lista blanca: eventos POST03_HIT_EVENT, contadores SHUTDOWN de POST03,
versiones de mods, lineas de resumen de HM/Bars y conteos de incidencias con criterio exacto. No copia lineas completas ni rutas.
Uso: extract_runtime_evidence.py --log-a A.log --log-b B.log --output-dir DIR [--expect-sha-a H --expect-sha-b H]"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

HIT_FIELDS = ['event', 'call', 'id', 'generation', 'life', 'kind', 'branch', 'cc_at_check', 'origin', 'tg', 'dual', 'distinct', 'collider_src',
              'collider', 'family', 'native_first', 'basis', 'at', 'hp', 'lethal', 'mode', 'old_mode', 'state', 'cd', 'cutdown', 'eligible', 'reason']
TOKEN = r'[A-Za-z0-9_+>.\-]+'  # sin '/', '\\', ':' ni espacios: no pueden aparecer rutas
HIT_RE = re.compile(r'POST03_HIT_EVENT ((?:\w+=' + TOKEN + r' ?)+)$')
FIELD_RE = re.compile(r'(\w+)=(' + TOKEN + r')')
KV_NUM = re.compile(r'(\w+)=(\d+)')
SUMMARY_LINES = {'POST03_SUMMARY': 'summary', 'POST03_HIT_SUMMARY': 'hit_summary', 'POST03_CONSERVATION': 'conservation'}
FAMILY_RE = re.compile(r'family\(([a-z0-9/]+)\)=(\d+(?:/\d+)*)')
GROUP_RE = re.compile(r'(adm|pre|log)\(([a-z_/ ]+)\)=(\d+(?:/\d+)*)(?: dropped=(\d+(?:/\d+)*))?')
LINK_RE = re.compile(r'fpcBs_Create: pid=(\d+) profname=fpcNm_ALINK_e')
MOD_RE = re.compile(r"\[INFO \| ([\w.]+)\] found '([^']{1,80})' v([\w.\-]{1,40}) by ")
PRIVACY = [re.compile(p) for p in (r'[A-Za-z]:\\', r'\\Users\\', r'OneDrive', r'AppData', r'Documentos', r'\b[0-9a-fA-F]{12,}\b', r'\b\d{1,3}(?:\.\d{1,3}){3}\b', r'0x[0-9a-fA-F]{9,}')]


def sha(b):
    return hashlib.sha256(b).hexdigest()


def parse_hit(line):
    m = HIT_RE.search(line)
    if not m:
        return None
    d = dict(FIELD_RE.findall(m.group(1)))
    if set(d) != set(HIT_FIELDS):
        raise ValueError('unexpected POST03_HIT_EVENT fields: %s' % sorted(set(d) ^ set(HIT_FIELDS)))
    pre_post = {}
    for k in ('hp', 'mode', 'state', 'cd'):
        a, b = d[k].split('>')
        pre_post[k + '_pre'], pre_post[k + '_post'] = int(a), int(b)
    row = {k: d[k] for k in HIT_FIELDS if k not in ('hp', 'mode', 'state', 'cd')}
    row.update(pre_post)
    for k in ('event', 'call', 'id', 'generation', 'dual', 'distinct', 'lethal', 'old_mode', 'cutdown'):
        row[k] = int(row[k])
    return row


def parse_log(path, expect_sha):
    raw = Path(path).read_bytes()
    digest = sha(raw)
    if expect_sha and digest != expect_sha:
        raise SystemExit('SHA-256 mismatch for %s: %s' % (Path(path).name, digest))
    text = raw.decode('utf-8', errors='replace')
    lines = text.splitlines()
    out = {'file_name': Path(path).name, 'sha256': digest, 'bytes': len(raw), 'splitlines': len(lines), 'events': [], 'mods': [], 'incidents': {}}
    cdi = tex = perf = perf_flag = 0
    link_pid = 0
    out['link_creation_pids'] = []
    for l in lines:
        m = LINK_RE.search(l)
        if m:  # creacion de daAlink_c (Link): los colliders de su espada viven dentro de este objeto del heap
            link_pid = int(m.group(1))
            out['link_creation_pids'].append(link_pid)
        row = parse_hit(l)
        if row:
            row['link_pid'] = link_pid
            out['events'].append(row)
            continue
        if 'POST03_HIT_EVENT' in l:  # una linea de evento que no cumple la lista blanca NO se descarta en silencio
            raise ValueError('POST03_HIT_EVENT line rejected by allowlist (line %d)' % (lines.index(l) + 1))
        for tag, key in SUMMARY_LINES.items():
            if tag in l and 'tag=SHUTDOWN' in l:
                body = l.split(tag, 1)[1]
                rec = {k: int(v) for k, v in KV_NUM.findall(body)}
                for g in FAMILY_RE.findall(body):
                    rec['family_' + g[0].replace('/', '_')] = [int(x) for x in g[1].split('/')]
                for name, labels, vals, dropped in GROUP_RE.findall(body):
                    rec[name + '_' + labels.replace('/', '_').replace(' ', '_')] = [int(x) for x in vals.split('/')]
                    if dropped:
                        rec[name + '_dropped'] = [int(x) for x in dropped.split('/')]
                out.setdefault(key, {}).update(rec)
        m = MOD_RE.search(l)
        if m:
            out['mods'].append({'id': m.group(1), 'name': m.group(2), 'version': m.group(3)})
        if 'POST03_TARGET' in l:
            ht = {}
            for k, v in FIELD_RE.findall(l.split('POST03_TARGET', 1)[1]):
                if k in ('resolve', 'same_address', 'inline_sites_flag', 'declared_target_resolved'):
                    ht.setdefault(k, v)  # la primera aparicion es el valor real; el texto final «(inline_sites_flag=1 => NOT_APPROVED)» es solo una leyenda
            out['hook_target'] = ht
        if 'POST03_INIT' in l:
            out['init'] = {k: v for k, v in FIELD_RE.findall(l.split('POST03_INIT', 1)[1]) if k in ('version', 'source_commit', 'host', 'capacity', 'log_cap')}
        if 'POST03_HOOK_INSTALLED' in l:
            out['hook_installed'] = True
        if 'POST03_SHUTDOWN' in l:
            out['post03_shutdown'] = True
        if 'POST03_DISABLED' in l or 'POST03_HOOK_FAILED' in l:
            out['post03_disabled_or_failed'] = True
        m = re.search(r'\[INFO \| dusk::mods::loader\] (\d+)/(\d+) mod\(s\) active', l)
        if m:
            out['mods_active'] = '%s/%s' % m.groups()
        if 'all mods unloaded' in l:
            out['all_mods_unloaded'] = True
        if l.startswith('  Backend: '):
            out['backend'] = l.split(':', 1)[1].strip()
        if 'HM203C SUMMARY(shutdown)' in l:
            out.setdefault('hm_summary', {}).update({k: int(v) for k, v in KV_NUM.findall(l.split('SUMMARY(shutdown)', 1)[1])
                                                     if k in ('pre', 'post', 'paired', 'unmatchedPost', 'orphanPre', 'identityMismatch', 'readFail', 'threadMismatch', 'events', 'valid', 'match', 'diff')})
        if 'BARS203 HM bridge(shutdown)' in l:
            out['bars_bridge'] = {k: int(v) for k, v in KV_NUM.findall(l.split('bridge(shutdown)', 1)[1]) if k in ('queries', 'ok', 'unavailable', 'invalid', 'rejected')}
        if 'BARS203 CP03 shutdown' in l:
            out['bars_draw'] = {k: int(v) for k, v in KV_NUM.findall(l.split('shutdown:', 1)[1]) if k in ('barsDrawn', 'packetDraws', 'refused')}
        if l.startswith('[WARNING | dusk] CalcDivideInfo'):
            cdi += 1
        if l.startswith('[WARNING | aurora::texture] texture_replacement:'):
            tex += 1
        if 'event=PERF_WINDOW' in l:
            perf += 1
            perf_flag += 'MEASUREMENT_NOT_AN_ERROR' in l
    out['incidents'] = {'CalcDivideInfo_warning_lines': cdi, 'texture_replacement_warning_lines': tex, 'PERF_WINDOW_lines': perf, 'PERF_WINDOW_with_MEASUREMENT_NOT_AN_ERROR': perf_flag}
    # HM: numero de golpes por perfil 510 (E_OC) del resumen final de dano efectivo
    m = re.search(r'HM203C CORE\(shutdown\): effective damage by profile \| (.*)$', text, re.M)
    if m:
        out['hm_core_profiles'] = {p: int(h) for p, h in re.findall(r'(\d+): (\d+) hits', m.group(1))}
    return out


def derive(sess):
    ev = sess['events']
    conf = [e for e in ev if e['kind'] == 'HIT_CONFIRMED']
    sword = [e for e in conf if e['family'] == 'SWORD']
    una = [e for e in conf if e['family'] == 'UNAVAILABLE']
    return {'events': len(ev), 'hit_confirmed': len(conf), 'finish_native': sum(e['kind'] == 'FINISH_NATIVE_OBSERVED' for e in ev),
            'family_unavailable': len(una), 'family_sword': len(sword), 'dual_tg_events': sum(e['dual'] for e in conf),
            'pair_distinct_events': sum(e['distinct'] for e in conf), 'implausible_collider': sum(e['collider'] == 'SELECTED_COLLIDER_IMPLAUSIBLE' for e in conf),
            'actors_hit': sorted({e['id'] for e in conf}), 'sword_actor_generations': sorted({(e['id'], e['generation']) for e in sword}),
            'by_link_instance': {str(p): {'confirmed': sum(1 for e in conf if e['link_pid'] == p), 'unavailable': sum(1 for e in una if e['link_pid'] == p),
                                         'sword': sum(1 for e in sword if e['link_pid'] == p)} for p in sorted({e['link_pid'] for e in conf})}}


def privacy_check(blob, allowed=()):
    for a in allowed:  # hashes publicos declarados (SHA-256 de cada log y commit fuente): unicos hex largos permitidos
        blob = blob.replace(a, '')
    for rx in PRIVACY:
        m = rx.search(blob)
        if m:
            raise SystemExit('privacy scan failed on pattern %s' % rx.pattern)


TSV_COLS = ['session', 'ord', 'event', 'call', 'id', 'generation', 'link_pid', 'life', 'kind', 'tg', 'dual', 'distinct', 'collider_src', 'collider', 'family', 'basis', 'at',
            'hp_pre', 'hp_post', 'mode_pre', 'mode_post', 'old_mode', 'state_pre', 'state_post', 'cd_pre', 'cd_post', 'cutdown', 'eligible']


def run(log_a, log_b, outdir, sha_a=None, sha_b=None):
    outdir = Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    res = {'A': parse_log(log_a, sha_a), 'B': parse_log(log_b, sha_b)}
    rows = []
    for s in ('A', 'B'):
        for i, e in enumerate(res[s]['events'], 1):
            rows.append([s, i] + [e[c] for c in TSV_COLS[2:]])
    tsv = '\t'.join(TSV_COLS) + '\n' + ''.join('\t'.join(str(x) for x in r) + '\n' for r in rows)
    summary = {'sessions': {}}
    for s in ('A', 'B'):
        d = {k: v for k, v in res[s].items() if k != 'events'}
        d['derived'] = derive(res[s])
        summary['sessions'][s] = d
    summary['aggregate'] = {'events': len(rows), 'hit_confirmed': sum(summary['sessions'][s]['derived']['hit_confirmed'] for s in 'AB'),
                            'finish_native': sum(summary['sessions'][s]['derived']['finish_native'] for s in 'AB'),
                            'family_unavailable': sum(summary['sessions'][s]['derived']['family_unavailable'] for s in 'AB'),
                            'family_sword': sum(summary['sessions'][s]['derived']['family_sword'] for s in 'AB')}
    blob = json.dumps(summary, sort_keys=True, indent=1) + '\n'
    allowed = [res[s]['sha256'] for s in 'AB'] + [res[s].get('init', {}).get(k, '') for s in 'AB' for k in ('source_commit', 'host')]
    allowed = [a for a in allowed if a]
    privacy_check(tsv, allowed)
    privacy_check(blob, allowed)
    (outdir / 'POST03_AB_EVENTS_SANITIZED.tsv').write_text(tsv, encoding='utf-8', newline='\n')
    (outdir / 'POST03_AB_SUMMARY_SANITIZED.json').write_text(blob, encoding='utf-8', newline='\n')
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--log-a', required=True)
    ap.add_argument('--log-b', required=True)
    ap.add_argument('--output-dir', required=True)
    ap.add_argument('--expect-sha-a')
    ap.add_argument('--expect-sha-b')
    a = ap.parse_args()
    s = run(a.log_a, a.log_b, a.output_dir, a.expect_sha_a, a.expect_sha_b)
    print(json.dumps(s['aggregate']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
