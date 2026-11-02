"""POST05: build MSVC x64 REAL + laboratorio + auditorias + paquete .dusk privado. Nunca carga la DLL ni ejecuta el juego.
Uso (runner windows-latest tras msvc-dev-cmd):
  python build_windows.py --source .official --inputs .private-inputs --ci-source POSTURE_SYSTEM/POST05_CI_SOURCE.json --work <dir>"""
import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent
sys.path.insert(0, str(HERE))
import pe_audit  # noqa: E402

PINS = {
    'official_commit': '40457c6adb381928e4b5fef6ed459ed291edd5e2',
    'aurora_commit': '3227d76c60e1e782ca576610bce61c9e7744d8be',
    'win_zip_bytes': 56037777,
    'win_zip_sha256': 'ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf',
    'importlib_bytes': 9400986,
    'importlib_sha256': '4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4',
    'importlib_crc': 0x70C35731,
    'host_exe_sha256': 'ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049',
}
TARGET = 'post05_selective_reactions'


def sha(b):
    return hashlib.sha256(b).hexdigest()


def lf(b):
    return b.replace(b'\r\n', b'\n')


def git(at, *args):
    return subprocess.check_output(['git', '-C', str(at), *args], text=True).strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--ci-source', type=Path, required=True)
    ap.add_argument('--work', type=Path, required=True)
    a = ap.parse_args()
    work, ev, build = a.work.resolve(), a.work.resolve() / 'evidence', a.work.resolve() / 'build'
    ev.mkdir(parents=True, exist_ok=True)
    build.mkdir(parents=True, exist_ok=True)
    source = a.source.resolve()
    r = {'status': 'BLOCKED', 'labels': [], 'phases': [], 'game_executed': False, 'dll_loaded': False, 'host_built': False,
         'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'run_id': os.getenv('GITHUB_RUN_ID'),
         'ci_head': os.getenv('GITHUB_SHA'),
         'toolchain_env': {k: os.getenv(k) for k in ['VCToolsVersion', 'WindowsSDKVersion', 'VSCMD_ARG_TGT_ARCH', 'VisualStudioVersion']}}

    def run(label, cmd, cwd=None, check=True):
        t0 = time.monotonic()
        with (ev / (label + '.log')).open('w', encoding='utf-8') as f:
            cp = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, text=True, cwd=cwd)
        r['phases'].append({'label': label, 'command': [str(c) for c in cmd], 'seconds': round(time.monotonic() - t0, 3), 'exit': cp.returncode})
        print('%s: exit=%d' % (label, cp.returncode), flush=True)
        if cp.returncode and check:
            print((ev / (label + '.log')).read_text(encoding='utf-8', errors='replace')[-12000:])
            raise RuntimeError('phase failed: ' + label)
        return cp.returncode

    try:
        if os.name != 'nt':
            raise RuntimeError('Windows required')
        if os.getenv('GITHUB_REF') != 'refs/heads/ci/posture-post06-rework':
            raise RuntimeError('Exclusive CI branch required')
        # --- procedencia del codigo compilado -----------------------------------------------------------------
        cs = json.loads(a.ci_source.read_text())
        r['source_commit'] = cs['source_commit']
        r['source_repo'] = cs['source_repo']
        seen = set()
        for rel, h in cs['files'].items():
            if sha(lf((PROJECT / rel).read_bytes())) != h:
                raise RuntimeError('Source drift vs CI_SOURCE ledger: ' + rel)
            seen.add(rel)
        build_files = {p.relative_to(PROJECT).as_posix() for p in PROJECT.rglob('*') if p.is_file() and '__pycache__' not in p.parts and
                       (p.parent == PROJECT and p.name in ('CMakeLists.txt', 'mod.json') or p.parts[len(PROJECT.parts)] in ('src', 'tests', 'ci'))}
        if build_files != seen:
            raise RuntimeError('Files outside the ledger: %s' % sorted(build_files ^ seen))
        r['ledger_files'] = len(seen)
        # --- SDK oficial y biblioteca de importacion privada ---------------------------------------------------
        if git(source, 'rev-parse', 'HEAD') != PINS['official_commit'] or git(source / 'extern/aurora', 'rev-parse', 'HEAD') != PINS['aurora_commit']:
            raise RuntimeError('SDK pin drift')
        archive = a.inputs / 'Dusklight-v2.0.3-win32-x86_64.zip'
        data = archive.read_bytes()
        if len(data) != PINS['win_zip_bytes'] or sha(data) != PINS['win_zip_sha256']:
            raise RuntimeError('Windows ZIP is not the pinned original (LFS pointer?)')
        with zipfile.ZipFile(archive) as z:
            info = z.getinfo('sdk/windows-amd64.lib')
            lib_bytes = z.read(info)
            host_sha = sha(z.read('dusklight.exe'))
        if len(lib_bytes) != PINS['importlib_bytes'] or sha(lib_bytes) != PINS['importlib_sha256'] or info.CRC != PINS['importlib_crc']:
            raise RuntimeError('Import library mismatch')
        if host_sha != PINS['host_exe_sha256']:
            raise RuntimeError('Host exe mismatch')
        lib = work / 'windows-amd64.lib'
        lib.write_bytes(lib_bytes)
        r['dependencies'] = {'official_sdk': PINS['official_commit'], 'aurora': PINS['aurora_commit'], 'importlib_sha256': sha(lib_bytes),
                             'host_exe_sha256': host_sha, 'win_zip_sha256': PINS['win_zip_sha256']}
        # --- herramientas ------------------------------------------------------------------------------------
        probe = build / 'version.cpp'
        probe.write_text('int compiler_only;\n')
        run('compiler', ['cl', '/Bv', '/c', str(probe), '/Fo' + str(build / 'version.obj')])
        run('cmake-version', ['cmake', '--version'])
        run('ninja-version', ['ninja', '--version'])
        # --- auditoria estatica de pasividad (texto) ----------------------------------------------------------
        run('tool-tests', [sys.executable, '-I', str(PROJECT / 'tests' / 'test_post04_tools.py')])
        run('source-audit', [sys.executable, '-I', str(HERE / 'source_audit.py'), str(PROJECT)])
        r['source_audit'] = json.loads((ev / 'source-audit.log').read_text())
        # --- configurar, laboratorio, modulo ------------------------------------------------------------------
        run('configure', ['cmake', '-S', str(PROJECT), '-B', str(build), '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                          '-DDUSK_SOURCE=' + str(source), '-DDUSK_GAME_EXE=' + str(lib), '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                          '-DPOST05_SOURCE_COMMIT=' + cs['source_commit']])
        for n, lab in enumerate(['hit_lab', 'lifecycle_lab', 'collider_lab', 'posture_lab', 'reaction_lab', 'compat_lab', 'rb_lab', 'general_lab', 'break_lab'], 1):
            run('build-lab%d' % n, ['cmake', '--build', str(build), '--target', 'post05_' + lab, '--parallel', '2'])
            run('run-lab%d' % n, [str(build / ('post05_' + lab + '.exe'))])
        for lab_log in ('run-lab1', 'run-lab2', 'run-lab3', 'run-lab4', 'run-lab5', 'run-lab6', 'run-lab7', 'run-lab8', 'run-lab9'):
            lab = (ev / (lab_log + '.log')).read_text()
            if 'RESULT=LAB_PASS' not in lab or 'failures=0' not in lab:
                raise RuntimeError('Laboratory did not pass: ' + lab_log)
        run('rework-mutations', [sys.executable, '-I', str(PROJECT / 'tests/run_mutations.py')])
        # --- el formateador real compilado con MSVC debe producir EXACTAMENTE la muestra versionada (generada con g++) y el extractor debe aceptarla ---
        sample = ev / 'sample-msvc.log'
        run('emit-sample', [str(build / 'post05_posture_lab.exe'), '--emit-sample', str(sample)])
        committed = lf((PROJECT / 'tests' / 'data' / 'sample_post04_formatter_output.log').read_bytes())
        if lf(sample.read_bytes()) != committed:
            raise RuntimeError('MSVC formatter output differs from the committed sample (formatter drift or compiler-dependent output)')
        sx = work / 'sample-extract'  # fuera de evidence/: ahi solo hay archivos planos
        run('extract-sample', [sys.executable, '-I', str(PROJECT / 'tests' / 'extract_post04_evidence.py'), '--log', str(sample), '--output-dir', str(sx)])
        for fx in sorted(sx.iterdir()):
            shutil.copyfile(fx, ev / ('sample-extract-' + fx.name))
        r['formatter_sample_identical_msvc_gpp'] = True
        r['labels'].append('LAB_PASS_MSVC')
        run('build-mod', ['cmake', '--build', str(build), '--target', TARGET, '--parallel', '2'])
        r['labels'].append('WINDOWS_MSVC_BUILD_OK')
        dll = build / (TARGET + '.dll')
        for label, arg in [('headers', '/headers'), ('imports', '/imports'), ('exports', '/exports')]:
            run('dumpbin-' + label, ['dumpbin', arg, str(dll)])
        mapfile = build / (TARGET + '.map')
        shutil.copyfile(mapfile, ev / 'module-map.log')
        mp = mapfile.read_text(errors='replace')
        for bad in ['setActionMode', 'offTgSph', 'cc_at_check', 'VirtualProtect', 'WriteProcessMemory', 'funchook_install']:
            if bad in mp:
                raise RuntimeError('Forbidden symbol in link map: ' + bad)
        cc = json.loads((build / 'compile_commands.json').read_text())
        mine = [x for x in cc if TARGET + '.dir' in x['command'].replace('\\', '/')]
        files = sorted(Path(x['file']).name for x in mine)
        r['production_units'] = files
        if 'mod.cpp' not in files or any('core_lab' in f for f in files) or any('/tests/' in x['file'].replace('\\', '/') for x in mine):
            raise RuntimeError('Production graph unexpected: %s' % files)
        (ev / 'compile-commands.json').write_text(json.dumps(mine, indent=2) + '\n')
        # --- auditoria PE -------------------------------------------------------------------------------------
        before = dll.read_bytes()
        r['loose_dll'] = {'bytes': len(before), 'sha256': sha(before)}
        expected = HERE / 'expected_pe.json'
        audit = pe_audit.audit(dll, expected if expected.is_file() else None)
        (ev / 'pe-audit.json').write_text(json.dumps(audit, indent=2, sort_keys=True) + '\n')
        r['pe_audit'] = {k: audit[k] for k in ('status', 'errors', 'pinned', 'hook_thunk_bound_import')}
        if audit['status'] != 'PE_AUDIT_PASS':
            r['status'] = 'BLOCKED_' + audit['status']
            raise RuntimeError('PE audit: %s %s' % (audit['status'], audit['errors']))
        run('pe-negatives', [sys.executable, '-I', str(HERE / 'pe_negatives.py'), str(dll), str(expected)])
        r['pe_negatives'] = json.loads((ev / 'pe-negatives.log').read_text())
        r['labels'].append('PE_NEGATIVES_REJECTED')
        # --- paquete oficial .dusk ----------------------------------------------------------------------------
        run('build-package', ['cmake', '--build', str(build), '--target', TARGET + '_package', '--parallel', '2'])
        if dll.read_bytes() != before:
            raise RuntimeError('DLL changed while packaging')
        pkg = build / 'mods' / (TARGET + '.dusk')
        with zipfile.ZipFile(pkg) as z:
            members = [i.filename for i in z.infolist()]
            files_in = sorted(m for m in members if not m.endswith('/'))
            if files_in != ['lib/windows-amd64/mod.dll', 'mod.json'] or any(m.startswith('/') or '..' in m.split('/') for m in members):
                raise RuntimeError('Unexpected package members: %s' % members)
            raw_json = z.read('mod.json')
            inside = z.read('lib/windows-amd64/mod.dll')
        if raw_json != (PROJECT / 'mod.json').read_bytes() or inside != before:
            raise RuntimeError('Package content differs from source manifest / loose DLL')
        manifest = json.loads(raw_json)
        pb = pkg.read_bytes()
        r['package'] = {'bytes': len(pb), 'sha256': sha(pb), 'members': members, 'mod_id': manifest['id'], 'version': manifest['version'],
                        'dll_in_package_equals_loose': True, 'mod_json_equals_source': True}
        r['labels'].append('WINDOWS_MSVC_PASS')
        # --- entrega privada ----------------------------------------------------------------------------------
        name = 'POSTURE_POST06_1_REWORK_PRIVATE_TEST_windows-amd64_%s.dusk' % cs['source_commit'][:12]
        delivery = work / 'delivery'
        delivery.mkdir(exist_ok=True)
        shutil.copyfile(pkg, delivery / name)
        meta = {'filename': name, 'package_bytes': len(pb), 'package_sha256': sha(pb), 'dll_sha256': sha(before), 'source_repo': cs['source_repo'],
                'source_commit': cs['source_commit'], 'ci_head': r['ci_head'], 'workflow_run_id': r['run_id'], 'official_sdk': PINS['official_commit'],
                'aurora': PINS['aurora_commit'], 'mod_id': manifest['id'], 'version': manifest['version'], 'private_test_build': True,
                'installed': False, 'runtime_tested': False, 'runtime_status': 'HUMAN_RUNTIME_PENDING', 'gameplay_changes': True,
                'game_writes': 'Audited actor reaction, vitality, duplicate-Helm cooldown1; scoped live Link cut5/8 only',
                'hook_targets': ['daE_OC_c::damage_check','daE_OC_c::executeDamage'], 'posture': 'POST06_NUMERIC_TRANSACTIONAL_REARM_50_OR_0', 'reaction_policy_id': 'POST05_TEST_POLICY_V4_GENERAL_ANTI_STAGGER', 'stagger_policy': 'GENERAL_POSTURE_BELOW_100', 'write_set': 'actor reaction+health+lethal attention; duplicate Helm cooldown1; scoped Link cut5/8', 'admission_policy': 'V2_COMPAT', 'e2_policy': 'MATURE_BC_OR_BRAKE', 'e_oc2': 'BC_MATURE_ATTACK0_BRAKE_PLUS_EARLY_TICKS_IF_NATIVE_ONLY_OR_RATE_PROVEN', 'e2_env_probe': 'get_service existence only, mod_update only', 'game_actions': 'DAMAGE_AND_BIG_DAMAGE_REACTION_UNDO_BELOW_100+POST06_STUN_DECISIVE_X1_50_NATIVE_FINISH_REARM', 'post06': 'break->native shield stun; future decisive contact x1.50 once->native fall/Finish/getup; timeout on feet posture50; no timed drop', 'lookup': 'fopAcM_SearchByID in mod_update only'}
        (delivery / 'DOWNLOAD_MANIFEST.json').write_text(json.dumps(meta, indent=2) + '\n')
        (delivery / 'SHA256SUMS.txt').write_text('%s  %s\n' % (sha(pb), name))
        shutil.copyfile(delivery / 'DOWNLOAD_MANIFEST.json', ev / 'DOWNLOAD_MANIFEST.json')
        r['status'] = 'POST05_PACKAGE_PASS_DOWNLOAD_PENDING'
    except Exception as ex:  # noqa: BLE001
        r['error'] = str(ex)
        print('ERROR:', ex, flush=True)
        raise
    finally:
        r['ended_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (ev / 'result.json').write_text(json.dumps(r, indent=2, sort_keys=True) + '\n')
        for f in ev.iterdir():
            if not f.is_file() or f.suffix not in ('.log', '.json', '.tsv'):
                continue
            s = f.read_text(encoding='utf-8', errors='replace')
            for old, new in [(str(work), '<WORK>'), (str(source), '<OFFICIAL>'), (str(PROJECT.parents[1]), '<WORKSPACE>')]:
                s = s.replace(old, new).replace(old.replace('\\', '/'), new).replace(old.replace('\\', '\\\\'), new)
            f.write_text(s, encoding='utf-8')
        (ev / 'evidence-sha256.json').write_text(json.dumps({f.name: {'bytes': f.stat().st_size, 'sha256': sha(f.read_bytes())}
                                                             for f in sorted(ev.iterdir()) if f.is_file() and f.name != 'evidence-sha256.json'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
