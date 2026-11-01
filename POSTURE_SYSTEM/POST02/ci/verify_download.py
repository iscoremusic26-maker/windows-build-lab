"""POST02: verificacion INDEPENDIENTE del artefacto descargado (no del directorio de compilacion).
Uso: verify_download.py --download <dir> --project <POST02> --ci-source <json> --out <dir>"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pe_audit  # noqa: E402


def sha(b):
    return hashlib.sha256(b).hexdigest()


def lf(b):
    return b.replace(b'\r\n', b'\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--download', type=Path, required=True)
    ap.add_argument('--project', type=Path, required=True)
    ap.add_argument('--ci-source', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    res = {'status': 'ARTIFACT_VERIFY_FAIL', 'errors': []}
    try:
        files = sorted(p for p in a.download.rglob('*') if p.is_file())
        names = sorted(p.name for p in files)
        duskfiles = [p for p in files if p.suffix == '.dusk']
        if len(duskfiles) != 1 or 'DOWNLOAD_MANIFEST.json' not in names or 'SHA256SUMS.txt' not in names or len(files) != 3:
            raise RuntimeError('Unexpected artifact files: %s' % names)
        dusk = duskfiles[0]
        data = dusk.read_bytes()
        meta = json.loads((dusk.parent / 'DOWNLOAD_MANIFEST.json').read_text())
        cs = json.loads(a.ci_source.read_text())
        sums = (dusk.parent / 'SHA256SUMS.txt').read_text().split()
        if sha(data) != meta['package_sha256'] or sums != [meta['package_sha256'], dusk.name] or meta['package_bytes'] != len(data):
            raise RuntimeError('Hash/size mismatch between .dusk, manifest and SHA256SUMS')
        if meta['source_commit'] != cs['source_commit'] or meta['runtime_tested'] or meta['gameplay_changes']:
            raise RuntimeError('Manifest provenance/claims mismatch')
        with zipfile.ZipFile(dusk) as z:
            members = [i.filename for i in z.infolist()]
            if sorted(m for m in members if not m.endswith('/')) != ['lib/windows-amd64/mod.dll', 'mod.json']:
                raise RuntimeError('Unexpected members %s' % members)
            if any(m.startswith('/') or '..' in m.split('/') for m in members):
                raise RuntimeError('Unsafe member path')
            mod_json = z.read('mod.json')
            dll = z.read('lib/windows-amd64/mod.dll')
        if lf(mod_json) != lf((a.project / 'mod.json').read_bytes()):
            raise RuntimeError('mod.json differs from source manifest')
        if sha(dll) != meta['dll_sha256']:
            raise RuntimeError('DLL hash differs from manifest')
        tmp = a.out / 'extracted-mod.dll'
        tmp.write_bytes(dll)
        audit = pe_audit.audit(tmp, a.project / 'ci' / 'expected_pe.json')
        tmp.unlink()
        if audit['status'] != 'PE_AUDIT_PASS':
            raise RuntimeError('PE audit on downloaded DLL: %s %s' % (audit['status'], audit['errors']))
        res.update(status='ARTIFACT_DOWNLOADED_VERIFIED', package_sha256=sha(data), package_bytes=len(data), dll_sha256=sha(dll),
                   mod_id=json.loads(mod_json)['id'], version=json.loads(mod_json)['version'], members=members,
                   pe_audit_status=audit['status'], hook_thunk_bound_import=audit['hook_thunk_bound_import'], filename=dusk.name,
                   runtime_tested=False, errors=[])
    except Exception as ex:  # noqa: BLE001
        res['errors'].append(str(ex))
        (a.out / 'download-verification.json').write_text(json.dumps(res, indent=2) + '\n')
        print('ERROR:', ex)
        return 1
    (a.out / 'download-verification.json').write_text(json.dumps(res, indent=2) + '\n')
    print(json.dumps(res, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
