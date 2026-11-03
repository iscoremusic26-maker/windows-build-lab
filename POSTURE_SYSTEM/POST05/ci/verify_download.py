"""POST05: verificacion INDEPENDIENTE del artefacto descargado (no del directorio de compilacion).
Uso: verify_download.py --download <dir> --project <POST04> --ci-source <json> --out <dir>"""
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
        if meta['source_commit'] != cs['source_commit'] or meta['runtime_tested'] or meta['gameplay_changes'] is not True or 'game_writes' not in meta:
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
        if meta.get('hook_targets') != sorted(audit['hook_thunks']) or meta.get('hook_meta_kinds') != audit['meta_kinds'] or meta.get('game_import_count') != len(audit['imports']['dusklight.exe']):
            raise RuntimeError('Download manifest hook/import claims differ from actual audited DLL')
        if 'paired own AtSet' not in meta['game_writes'] or 'no new game writes' in meta.get('lnk_session01',''):
            raise RuntimeError('Download manifest omits the scoped FIX01 enemy writer')
        if 'procGuardBreakInit' not in meta['game_writes'] or 'lnk_session02' not in meta or meta.get('version') != '0.11.4' \
                or '?procGuardBreakInit@daAlink_c@@QEAAHXZ' not in audit['imports']['dusklight.exe']:
            raise RuntimeError('Download manifest/DLL omit the LNK SESSION02 native Link transition (procGuardBreakInit)')
        fx_imports = ('?setStickData@daAlink_c@@QEAAXXZ', '?setShieldGuard@daAlink_c@@QEAAXXZ', '?setRStatus@daAlink_c@@QEAAXE@Z', '?spActionTrigger@daAlink_c@@QEAAHXZ',
                      '?checkMoveDoAction@daAlink_c@@QEAAHXZ', '?setCollision@daAlink_c@@QEAAXXZ')
        if 'lnk_session02_fix01' not in meta or 'FLG2_UNK_8000000 switched OFF only' not in meta['game_writes'] or 'crouch' not in meta['lnk_session02_fix01'] \
                or any(i not in audit['imports']['dusklight.exe'] for i in fx_imports) or meta.get('hook_meta_kinds', {}).get('HOOK_MEM_EXT') != 8:
            raise RuntimeError('Download manifest/DLL omit the LNK SESSION02 FIX01 manual-shield/penalty hooks and writes')
        if meta.get('hud_fix02', {}).get('version') != '0.11.4' or 'DISABLED' not in meta['game_writes']:
            raise RuntimeError('HUD FIX01 policy missing from package manifest')
        balance = meta.get('balance_session01', {})
        if balance.get('weights_sword_parry_iron') != {'BLUE':[25,40,50],'RED':[17,27,33],'BLACK':[13,20,25],'BLACK_FURY':[13,20,25]} or balance.get('standing_rearm') != 25 or balance.get('getup_rearm') != 0 or balance.get('new_native_writes') is not False:
            raise RuntimeError('Download manifest omits or changes BALANCE SESSION01 approved policy')
        if json.loads(mod_json).get('version') != '0.11.3' or not dusk.name.startswith('POSTURE_HUD_FIX02_0_11_4_'):
            raise RuntimeError('Wrong HUD FIX01 package version/name')
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
