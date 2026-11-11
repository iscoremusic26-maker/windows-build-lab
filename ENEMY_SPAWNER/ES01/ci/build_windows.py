"""Enemy Spawner (Standalone) 0.1.0 CI: MSVC x64 build, audits, laboratories, mutants and the private .dusk. Never loads the DLL, never runs the game.
Profile: the committed file ENEMY_SPAWNER/ES01_CI_PROFILE.txt next to the ledger: fast | integration | full (docs/CI_PROFILES.md). Only full names a package.
Usage (windows-latest after msvc-dev-cmd):
  python build_windows.py --source .official --inputs .private-inputs --ci-source ENEMY_SPAWNER/ES01_CI_SOURCE.json --work <dir>"""
import argparse, datetime, hashlib, json, os, shutil, subprocess, sys, time, zipfile
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
    'importlib_sha256': '4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4',
    'host_exe_sha256': 'ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049',
}
CI_REF = 'refs/heads/ci/enemy-spawner-standalone-v0-1-0'
TARGET = 'enemy_spawner_standalone'
LIMITS = {'build-mod': 900, 'mutants': 1800}
DEFAULT_LIMIT = 300
PLAN = {
    'fast': ['compiler', 'cmake-version', 'source-rules', 'configure', 'build-mod', 'dumpbin-headers', 'dumpbin-imports', 'dumpbin-exports', 'pe-audit', 'build-core_lab', 'run-core_lab'],
}
PLAN['integration'] = PLAN['fast'] + ['catalog-check', 'vendor-check', 'build-adapter_lab', 'run-adapter_lab']
PLAN['full'] = PLAN['integration'] + ['mutants', 'build-package']

def sha(b):
    return hashlib.sha256(b).hexdigest()

def lf(b):
    return b.replace(b'\r\n', b'\n')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--ci-source', type=Path, required=True)
    ap.add_argument('--work', type=Path, required=True)
    a = ap.parse_args()
    work = a.work.resolve(); ev = work / 'evidence'; build = work / 'build'
    ev.mkdir(parents=True, exist_ok=True); build.mkdir(parents=True, exist_ok=True)
    source = a.source.resolve()
    profile = (a.ci_source.parent / 'ES01_CI_PROFILE.txt').read_text().strip()
    if profile not in PLAN:
        raise SystemExit('unknown profile ' + profile)
    plan = PLAN[profile]
    r = {'status': 'BLOCKED', 'profile': profile, 'phases': [], 'omitted': [p for p in PLAN['full'] if p not in plan], 'cache': 'none: every result of this run was produced by this run',
         'game_executed': False, 'dll_loaded': False, 'run_id': os.getenv('GITHUB_RUN_ID'), 'ci_head': os.getenv('GITHUB_SHA'),
         'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
    ran = []

    def run(label, cmd, cwd=None, env=None):
        if label not in plan:
            raise RuntimeError('phase outside the plan of profile %s: %s' % (profile, label))
        limit = LIMITS.get(label, DEFAULT_LIMIT)
        print('::group::[%d/%d] %s (limit %ds)' % (len(ran) + 1, len(plan), label, limit), flush=True)
        print('START ' + label, flush=True)
        t0 = time.monotonic(); timed_out = False
        with (ev / (label + '.log')).open('w', encoding='utf-8') as f:
            proc = subprocess.Popen([str(c) for c in cmd], stdout=f, stderr=subprocess.STDOUT, text=True, cwd=cwd, env=env)
            try:
                code = proc.wait(timeout=limit)
            except subprocess.TimeoutExpired:
                timed_out = True
                subprocess.run(['taskkill', '/T', '/F', '/PID', str(proc.pid)], capture_output=True)
                code = proc.wait()
        secs = round(time.monotonic() - t0, 3)
        ran.append(label)
        r['phases'].append({'label': label, 'seconds': secs, 'exit': 'TIMEOUT' if timed_out else code})
        print('END %s exit=%s seconds=%.1f | %d/%d %.1f%% of finished units' % (label, 'TIMEOUT' if timed_out else code, secs, len(ran), len(plan), 100.0 * len(ran) / len(plan)), flush=True)
        print('::endgroup::', flush=True)
        if timed_out or code:
            print((ev / (label + '.log')).read_text(encoding='utf-8', errors='replace')[-8000:])
            raise RuntimeError(('TIMEOUT ' if timed_out else 'phase failed: ') + label)

    try:
        if os.name != 'nt' or os.getenv('GITHUB_REF') != CI_REF:
            raise RuntimeError('Windows runner on the exclusive CI branch required')
        cs = json.loads(a.ci_source.read_text())
        r['source_commit'] = cs['source_commit']
        seen = set()
        for rel, h in cs['files'].items():
            if sha(lf((PROJECT / rel).read_bytes())) != h:
                raise RuntimeError('source drift vs ledger: ' + rel)
            seen.add(rel)
        files = {p.relative_to(PROJECT).as_posix() for p in PROJECT.rglob('*') if p.is_file() and '__pycache__' not in p.parts}
        if files != seen:
            raise RuntimeError('files outside the ledger: %s' % sorted(files ^ seen))
        if subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip() != PINS['official_commit'] or \
           subprocess.check_output(['git', '-C', str(source / 'extern/aurora'), 'rev-parse', 'HEAD'], text=True).strip() != PINS['aurora_commit']:
            raise RuntimeError('SDK pin drift')
        archive = a.inputs / 'Dusklight-v2.0.3-win32-x86_64.zip'
        data = archive.read_bytes()
        if len(data) != PINS['win_zip_bytes'] or sha(data) != PINS['win_zip_sha256']:
            raise RuntimeError('Windows ZIP is not the pinned original')
        with zipfile.ZipFile(archive) as z:
            lib_bytes = z.read('sdk/windows-amd64.lib')
            host = sha(z.read('dusklight.exe'))
        if sha(lib_bytes) != PINS['importlib_sha256'] or host != PINS['host_exe_sha256']:
            raise RuntimeError('import library / host exe mismatch')
        lib = work / 'windows-amd64.lib'
        lib.write_bytes(lib_bytes)
        probe = build / 'version.cpp'
        probe.write_text('int compiler_only;\n')
        run('compiler', ['cl', '/Bv', '/c', probe, '/Fo' + str(build / 'version.obj')])
        run('cmake-version', ['cmake', '--version'])
        run('source-rules', [sys.executable, '-I', HERE / 'source_rules.py', PROJECT])
        run('configure', ['cmake', '-S', PROJECT, '-B', build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DDUSK_SOURCE=' + str(source), '-DDUSK_GAME_EXE=' + str(lib)])
        run('build-mod', ['cmake', '--build', build, '--target', TARGET, '--parallel', '4'])
        dll = build / (TARGET + '.dll')
        for label, arg in (('headers', '/headers'), ('imports', '/imports'), ('exports', '/exports')):
            run('dumpbin-' + label, ['dumpbin', arg, dll])
        run('pe-audit', [sys.executable, '-I', HERE / 'pe_audit.py', dll, HERE / 'expected_pe.json'])
        before = dll.read_bytes()
        r['dll'] = {'bytes': len(before), 'sha256': sha(before)}
        labs = ['core_lab'] + (['adapter_lab'] if profile != 'fast' else [])
        if profile != 'fast':
            run('catalog-check', [sys.executable, '-I', HERE / 'catalog_check.py', PROJECT])
            run('vendor-check', [sys.executable, '-I', HERE / 'vendor_check.py', PROJECT])
        for lab in labs:
            run('build-' + lab, ['cmake', '--build', build, '--target', 'es_' + lab, '--parallel', '4'])
            run('run-' + lab, [build / ('es_' + lab + '.exe')])
            if 'RESULT=LAB_PASS' not in (ev / ('run-' + lab + '.log')).read_text():
                raise RuntimeError('laboratory did not pass: ' + lab)
        if profile == 'full':
            run('mutants', [sys.executable, '-I', PROJECT / 'tests/test_mutations.py'])
            m = (ev / 'mutants.log').read_text()
            r['mutants'] = {k: v for k, v in json.loads(m[m.index('{'):]).items() if k in ('count', 'detected', 'timeouts')}
            run('build-package', ['cmake', '--build', build, '--target', TARGET + '_package', '--parallel', '4'])
            if dll.read_bytes() != before:
                raise RuntimeError('DLL changed while packaging')
            pkg = build / 'mods' / (TARGET + '.dusk')
            with zipfile.ZipFile(pkg) as z:
                members = [i.filename for i in z.infolist()]
                files_in = sorted(x for x in members if not x.endswith('/'))
                if files_in != ['lib/windows-amd64/mod.dll', 'mod.json'] or any(x.startswith('/') or '..' in x.split('/') for x in members):
                    raise RuntimeError('unexpected package members: %s' % members)
                if z.read('mod.json') != (PROJECT / 'mod.json').read_bytes() or z.read('lib/windows-amd64/mod.dll') != before:
                    raise RuntimeError('package content differs from the audited DLL / manifest')
            pb = pkg.read_bytes()
            name = 'ENEMY_SPAWNER_STANDALONE_0_1_0_PRIVATE_TEST_windows-amd64_%s.dusk' % cs['source_commit'][:12]
            delivery = work / 'delivery'
            delivery.mkdir(exist_ok=True)
            (delivery / name).write_bytes(pb)
            meta = {'filename': name, 'package_bytes': len(pb), 'package_sha256': sha(pb), 'dll_sha256': sha(before), 'mod_id': 'dev.enemyspawner.standalone', 'version': '0.1.0',
                    'members': members, 'source_repo': 'iscoremusic26-maker/Mods-Duskligh', 'source_commit': cs['source_commit'], 'ci_head': r['ci_head'], 'workflow_run_id': r['run_id'],
                    'ci_profile': profile, 'official_sdk': PINS['official_commit'], 'host_exe_sha256': PINS['host_exe_sha256'], 'hooks': 0,
                    'dawnlight': {'repo': 'https://github.com/BeZide93/dawnlight', 'commit': 'b5e9046cd98bdd8593ab2cf219d72a0f61ef7213', 'license': 'CC0 1.0 Universal'},
                    'expansion_reference': {'version': '0.17.0', 'id': 'dev.bokoblin.expansion', 'source_commit': 'fe47c8b828939829914d466a425cb1e2c0223cd9'},
                    'variants': {'blue': 'ROUTE_AVAILABLE_IF_FRAMEWORK_LOADED_VERIFIED_BY_OBSERVATION', 'red': 'ROUTE_AVAILABLE_IF_FRAMEWORK_LOADED_VERIFIED_BY_OBSERVATION', 'black': 'UNAVAILABLE_NO_PUBLIC_CONTRACT_IN_0_17_0'},
                    'mutants': r['mutants'], 'runtime_status': 'HUMAN_RUNTIME_PENDING', 'installed': False}
            (delivery / 'DOWNLOAD_MANIFEST.json').write_text(json.dumps(meta, indent=1) + '\n')
            (delivery / 'SHA256SUMS.txt').write_text('%s  %s\n' % (sha(pb), name))
            shutil.copyfile(delivery / 'DOWNLOAD_MANIFEST.json', ev / 'DOWNLOAD_MANIFEST.json')
        if ran != plan:
            raise RuntimeError('phases that ran differ from the plan: %s' % sorted(set(ran) ^ set(plan)))
        r['status'] = 'FULL_PASS' if profile == 'full' else 'PARTIAL_%s_PASS_NOT_DELIVERABLE' % profile.upper()
    except Exception as ex:  # noqa: BLE001
        r['error'] = str(ex)
        print('ERROR:', ex, flush=True)
        raise
    finally:
        r['ended_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (ev / 'result.json').write_text(json.dumps(r, indent=1) + '\n')
        for f in ev.iterdir():
            if f.suffix in ('.log', '.json'):
                s = f.read_text(encoding='utf-8', errors='replace')
                for old, new in ((str(work), '<WORK>'), (str(source), '<OFFICIAL>'), (str(PROJECT.parents[1]), '<WORKSPACE>')):
                    s = s.replace(old, new).replace(old.replace('\\', '/'), new).replace(old.replace('\\', '\\\\'), new)
                f.write_text(s, encoding='utf-8')

if __name__ == '__main__':
    main()
