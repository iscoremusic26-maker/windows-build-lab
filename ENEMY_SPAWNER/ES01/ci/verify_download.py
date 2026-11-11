"""Enemy Spawner 0.1.1 FIX01: independent verification of the artifact downloaded through the GitHub API.
Usage: verify_download.py --download <dir> --project <ENEMY_SPAWNER/ES01> --ci-source <ledger> --out <dir>"""
import argparse, hashlib, json, sys, zipfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import pe_audit  # noqa: E402
def sha(b):
    return hashlib.sha256(b).hexdigest()
ap = argparse.ArgumentParser()
ap.add_argument('--download', type=Path, required=True)
ap.add_argument('--project', type=Path, required=True)
ap.add_argument('--ci-source', type=Path, required=True)
ap.add_argument('--out', type=Path, required=True)
a = ap.parse_args()
a.out.mkdir(parents=True, exist_ok=True)
res = {'status': 'DOWNLOAD_VERIFY_FAIL', 'errors': []}
try:
    dusks = sorted(a.download.rglob('*.dusk'))
    if len(dusks) != 1:
        raise RuntimeError('expected exactly one .dusk, found %d' % len(dusks))
    d = dusks[0]
    data = d.read_bytes()
    meta = json.loads((d.parent / 'DOWNLOAD_MANIFEST.json').read_text())
    sums = (d.parent / 'SHA256SUMS.txt').read_text().split()
    cs = json.loads(a.ci_source.read_text())
    if sha(data) != meta['package_sha256'] or sums != [sha(data), d.name] or len(data) != meta['package_bytes'] or d.name != meta['filename']:
        raise RuntimeError('package hash / size / name differ from the manifest')
    if not d.name.startswith('ENEMY_SPAWNER_STANDALONE_FIX01_0_1_1_PRIVATE_TEST_windows-amd64_') or meta['source_commit'] != cs['source_commit'] or meta['ci_profile'] != 'full':
        raise RuntimeError('name, source or profile mismatch')
    with zipfile.ZipFile(d) as z:
        members = [i.filename for i in z.infolist()]
        if sorted(m for m in members if not m.endswith('/')) != ['lib/windows-amd64/mod.dll', 'mod.json']:
            raise RuntimeError('members: %s' % members)
        mj = z.read('mod.json'); dll = z.read('lib/windows-amd64/mod.dll')
    if mj.replace(b'\r\n', b'\n') != (a.project / 'mod.json').read_bytes().replace(b'\r\n', b'\n'):
        raise RuntimeError('mod.json differs from source')
    m = json.loads(mj)
    if m['id'] != 'dev.enemyspawner.standalone' or m['version'] != '0.1.1':
        raise RuntimeError('mod id/version')
    if sha(dll) != meta['dll_sha256']:
        raise RuntimeError('DLL hash')
    # FIX01: the shipped DLL carries the English panel and none of the 0.1.0 Spanish texts; the manifest declares the dropdown.
    for s in (b'Spawn Enemy', b'Enemy Spawner', b'Bokoblin Black (Expansion): integration unavailable', b'UI_BUILD_FAILED element=%s result=%d', b'Spawn request submitted: %s.'):
        if s not in dll:
            raise RuntimeError('English text missing from the DLL: %r' % s)
    for s in (b'Enemigo', b'Invocar', b'integracion', b'no disponible', b'Solicitud', b'nativo'):
        if s in dll:
            raise RuntimeError('Spanish text left in the DLL: %r' % s)
    if not meta.get('mods_panel_chooser', '').startswith('UI_CONTROL_DROPDOWN'):
        raise RuntimeError('manifest does not declare the dropdown')
    tmp = a.out / 'extracted.dll'; tmp.write_bytes(dll)
    audit = pe_audit.audit(tmp, a.project / 'ci/expected_pe.json'); tmp.unlink()
    if audit['status'] != 'PE_AUDIT_PASS':
        raise RuntimeError('PE audit: %s' % audit['errors'])
    if meta['variants']['black'] != 'UNAVAILABLE_NO_PUBLIC_CONTRACT_IN_0_17_0' or meta['hooks'] != 0 or meta['mutants']['count'] != meta['mutants']['detected']:
        raise RuntimeError('manifest claims')
    res.update(status='ARTIFACT_DOWNLOADED_VERIFIED', filename=d.name, package_bytes=len(data), package_sha256=sha(data), dll_sha256=sha(dll), members=members, mod_id=m['id'], version=m['version'],
               pe=audit['status'], game_imports=audit['game_import_count'], runtime_tested=False)
except Exception as ex:  # noqa: BLE001
    res['errors'].append(str(ex))
(a.out / 'download-verification.json').write_text(json.dumps(res, indent=1) + '\n')
print(json.dumps(res, indent=1))
sys.exit(0 if res['status'] == 'ARTIFACT_DOWNLOADED_VERIFIED' else 1)
