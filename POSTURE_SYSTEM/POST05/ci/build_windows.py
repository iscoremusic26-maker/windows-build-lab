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
# FIX04: per-process limits from the durations measured in run 42932174664 (about 3x the slowest; the default is more than 4x anything not listed). A hang now fails by name.
DEFAULT_LIMIT = 420
LIMITS = {'hook-prologue-audit': 900, 'liz-world-mutations': 3300, 'liz-mutations': 2700, 'rework-mutations': 2400, 'cp06f1-mutations': 1000}
BUILD_LABELS = {'compiler', 'cmake-version', 'ninja-version', 'configure', 'build-mod', 'dumpbin-headers', 'dumpbin-imports', 'dumpbin-exports'}
UNIT_PLAN = {'build': 8, 'labs': 118, 'package': 2}  # checked against what really ran before the run can pass


def stage_of(label):
    return 'build' if label in BUILD_LABELS else 'package' if label in ('build-package', 'hook-prologue-audit') else 'labs'


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

    done = {'all': 0, 'build': 0, 'labs': 0, 'package': 0}

    def run(label, cmd, cwd=None, check=True):
        # FIX04: live progress. Units are phases really finished, over a plan verified at the end of the run: never an ETA.
        stage = stage_of(label)
        limit = LIMITS.get(label, DEFAULT_LIMIT)
        print('::group::[%d/%d] %s (%s %d/%d, limit %ds)' % (done['all'] + 1, sum(UNIT_PLAN.values()), label, stage, done[stage] + 1, UNIT_PLAN[stage], limit), flush=True)
        print('START %s' % label, flush=True)
        t0 = time.monotonic()
        timed_out = False
        with (ev / (label + '.log')).open('w', encoding='utf-8') as f:
            proc = subprocess.Popen(cmd, stdout=f, stderr=subprocess.STDOUT, text=True, cwd=cwd)
            try:
                code = proc.wait(timeout=limit)
            except subprocess.TimeoutExpired:
                timed_out = True
                subprocess.run(['taskkill', '/T', '/F', '/PID', str(proc.pid)], capture_output=True)  # the whole tree: a test's cl.exe or lab child holds the log otherwise
                code = proc.wait()
        seconds = round(time.monotonic() - t0, 3)
        r['phases'].append({'label': label, 'command': [str(c) for c in cmd], 'seconds': seconds, 'exit': 'TIMEOUT' if timed_out else code})
        done['all'] += 1
        done[stage] += 1
        total = sum(UNIT_PLAN.values())
        print('END %s exit=%s seconds=%.1f | %s %d/%d %.1f%% | total %d/%d %.1f%% of finished units' % (label, 'TIMEOUT' if timed_out else code, seconds, stage, done[stage], UNIT_PLAN[stage], 100.0 * done[stage] / UNIT_PLAN[stage], done['all'], total, 100.0 * done['all'] / total), flush=True)
        print('::endgroup::', flush=True)
        if timed_out:
            print('::error::TIMEOUT %s after %ds' % (label, limit), flush=True)
            print((ev / (label + '.log')).read_text(encoding='utf-8', errors='replace')[-12000:])
            raise RuntimeError('TIMEOUT ' + label)
        if code and check:
            print('::error::phase failed: %s exit=%s' % (label, code), flush=True)
            print((ev / (label + '.log')).read_text(encoding='utf-8', errors='replace')[-12000:])
            raise RuntimeError('phase failed: ' + label)
        return code

    try:
        if os.name != 'nt':
            raise RuntimeError('Windows required')
        if os.getenv('GITHUB_REF') != 'refs/heads/ci/light-fades-twice-lizalfos-0-12-4':
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
        host = work / 'host_for_prologue_audit.exe'  # outside evidence/: never uploaded
        with zipfile.ZipFile(archive) as z:
            host.write_bytes(z.read('dusklight.exe'))
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
        run('pmf-static-tests', [sys.executable, '-I', str(PROJECT / 'tests/test_pmf_audit.py')])
        run('hudfix-production-adapter', [sys.executable, '-I', str(PROJECT / 'tests/test_hudfix_adapter.py')])
        run('fix03-parity', [sys.executable, '-I', str(HERE / 'fix03_audit.py'), str(PROJECT)])
        run('fix03-source-negatives', [sys.executable, '-I', str(PROJECT / 'tests/test_fix03_source_negatives.py')])
        run('cp04-pe-name-policy',[sys.executable,'-I',str(PROJECT/'tests/test_cp04_pe_names.py')])
        run('cp04-native-reference',[sys.executable,'-I',str(HERE/'cp04_reference_audit.py'),str(source)])
        run('cp05-pe-policy',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_pe_policy.py')])
        run('cp05-native-reference',[sys.executable,'-I',str(HERE/'cp05_reference_audit.py'),str(source)])
        run('cp05-fix01-native-reference',[sys.executable,'-I',str(HERE/'cp05_fix01_reference_audit.py'),str(source)])
        run('cp05-fix02-native-reference',[sys.executable,'-I',str(HERE/'cp05_fix02_reference_audit.py'),str(source)])
        run('source-barrier', [sys.executable, '-I', str(HERE / 'store_audit.py'), str(PROJECT)])
        run('source-audit', [sys.executable, '-I', str(HERE / 'source_audit.py'), str(PROJECT)])
        r['source_audit'] = json.loads((ev / 'source-audit.log').read_text())
        # --- configurar, laboratorio, modulo ------------------------------------------------------------------
        run('configure', ['cmake', '-S', str(PROJECT), '-B', str(build), '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                          '-DDUSK_SOURCE=' + str(source), '-DDUSK_GAME_EXE=' + str(lib), '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                          '-DPOST05_SOURCE_COMMIT=' + cs['source_commit']])
        run('build-mod', ['cmake', '--build', str(build), '--target', TARGET, '--parallel', '2'])
        r['labels'].append('WINDOWS_MSVC_BUILD_OK')
        dll = build / (TARGET + '.dll')
        for label, arg in [('headers', '/headers'), ('imports', '/imports'), ('exports', '/exports')]:
            run('dumpbin-' + label, ['dumpbin', arg, str(dll)])
        mapfile = build / (TARGET + '.map')
        shutil.copyfile(mapfile, ev / 'module-map.log')
        mp = mapfile.read_text(errors='replace')
        for bad in ['offTgSph', 'cc_at_check', 'VirtualProtect', 'WriteProcessMemory', 'funchook_install']:
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
        run('hook-prologue-audit', [sys.executable, '-I', str(HERE / 'hook_prologue_audit.py'), str(host)])
        r['hook_prologue'] = {k: v for k, v in json.loads((ev / 'hook-prologue-audit.log').read_text()).items() if k in ('status', 'errors', 'window')}
        host.unlink()
        run('store-audit', [sys.executable, '-I', str(HERE / 'store_audit.py'), str(PROJECT), '--dll', str(dll), '--map', str(mapfile)])
        run('cp04-native-writer-scopes',[sys.executable,'-I',str(HERE/'cp04_pe_audit.py'),str(dll),str(mapfile)])
        run('cp05-native-writer-scopes',[sys.executable,'-I',str(HERE/'cp05_pe_audit.py'),str(dll),str(mapfile)])
        run('cp06-native-writer-scope',[sys.executable,'-I',str(HERE/'cp06_pe_audit.py'),str(dll),str(mapfile)])
        run('cp06f1-native-writer-scopes',[sys.executable,'-I',str(HERE/'cp06f1_pe_audit.py'),str(dll),str(mapfile)])
        run('liz-native-contract',[sys.executable,'-I',str(HERE/'liz_native_audit.py'),str(source)])
        run('liz-native-writer-scopes',[sys.executable,'-I',str(HERE/'liz_pe_audit.py'),str(dll),str(mapfile)])
        r['liz_pe_scope']=json.loads((ev/'liz-native-writer-scopes.log').read_text())
        run('lft03-native-writer-scopes',[sys.executable,'-I',str(HERE/'lft03_pe_audit.py'),str(dll),str(mapfile)])
        r['lft03_pe_scope']=json.loads((ev/'lft03-native-writer-scopes.log').read_text())
        r['cp06f1_pe_scope']=json.loads((ev/'cp06f1-native-writer-scopes.log').read_text())
        r['cp06_pe_scope']=json.loads((ev/'cp06-native-writer-scope.log').read_text())
        run('pe-negatives', [sys.executable, '-I', str(HERE / 'pe_negatives.py'), str(dll), str(expected)])
        r['pe_negatives'] = json.loads((ev / 'pe-negatives.log').read_text())
        r['labels'].append('PE_NEGATIVES_REJECTED')
        run('hudfix02-production-adapter', [sys.executable, '-I', str(PROJECT / 'tests/test_hudfix02_adapter.py')])
        run('cp04-production-adapter',[sys.executable,'-I',str(PROJECT/'tests/test_cp04_adapter.py')])
        run('cp04-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp04_mutations.py')])
        run('cp04-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_cp04_source_negatives.py')])
        run('cp05-production-adapter',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_adapter.py')])
        run('cp05-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_mutations.py')])
        run('cp05-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_source_negatives.py')])
        run('cp05-fix01-native',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix01_native.py')])
        run('cp05-fix01-geometry',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix01_geometry.py')])
        run('cp05-fix01-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix01_mutations.py')])
        run('cp05-fix01-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix01_source_negatives.py')])
        run('cp05-fix02-native',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix02_native.py')])
        run('cp05-fix02-guard',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix02_guard.py')])
        run('cp05-fix02-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix02_mutations.py')])
        run('cp05-fix02-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_cp05_fix02_source_negatives.py')])
        run('cp06-combat',[sys.executable,'-I',str(PROJECT/'tests/test_cp06_combat.py')])
        run('cp06-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp06_mutations.py')])
        run('cp06-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_cp06_source_negatives.py')])
        run('cp06f1-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp06f1_mutations.py')])
        run('cp06f1-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_cp06f1_source_negatives.py')])
        run('cp06f1-bench',[sys.executable,'-I',str(PROJECT/'tests/bench_cp06f1.py'),str(PROJECT)],check=False)
        run('liz-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_liz_mutations.py')])
        run('liz-world-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_liz_world_mutations.py')])
        run('liz-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_liz_source_negatives.py')])
        run('lft03-source-negatives',[sys.executable,'-I',str(PROJECT/'tests/test_lft03_source.py')])
        run('lft03-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_lft03_mutations.py')])
        run('liz-bench',[sys.executable,'-I',str(PROJECT/'tests/bench_liz.py'),str(PROJECT)],check=False)
        run('cp06-bench-with-cp06',[sys.executable,'-I',str(PROJECT/'tests/bench_cp06.py'),str(PROJECT),'--cp06','16','3000','15'],check=False)
        run('cp06-bench-without-cp06-callbacks',[sys.executable,'-I',str(PROJECT/'tests/bench_cp06.py'),str(PROJECT),'16','3000','15'],check=False)

        run('cp0203-production-adapter',[sys.executable,'-I',str(PROJECT/'tests/test_cp0203_adapter.py')])
        run('cp0203-independent-reference',[sys.executable,'-I',str(PROJECT/'tests/test_cp0203_reference.py')])
        run('cp0203-mutations',[sys.executable,'-I',str(PROJECT/'tests/test_cp0203_mutations.py')])
        run('cp06-authentic-pair', [sys.executable, '-I', str(PROJECT / 'tests/test_alignment_pair.py')])

        for n, lab in enumerate(['hit_lab', 'lifecycle_lab', 'collider_lab', 'posture_lab', 'reaction_lab', 'compat_lab', 'rb_lab', 'general_lab', 'break_lab', 'hud_lab', 'parry_lab', 'lnk_lab', 'lnks1_lab', 'fix01_lab', 'lnks2_lab', 'lnks2fix_lab', 'balance_lab', 'hudfix_lab','cp0203_lab','cp04_lab','cp05_lab','cp05_fix02_lab','cp06_lab','cp06f1_lab','cp06f1_world_lab','liz_lab','liz_world_lab','lft03_lab'], 1):
            run('build-lab%d' % n, ['cmake', '--build', str(build), '--target', 'post05_' + lab, '--parallel', '2'])
            run('run-lab%d' % n, [str(build / ('post05_' + lab + '.exe'))])
        for lab_log in ('run-lab1', 'run-lab2', 'run-lab3', 'run-lab4', 'run-lab5', 'run-lab6', 'run-lab7', 'run-lab8', 'run-lab9', 'run-lab10', 'run-lab11', 'run-lab12', 'run-lab13', 'run-lab14', 'run-lab15', 'run-lab16', 'run-lab17', 'run-lab18','run-lab19','run-lab20','run-lab21','run-lab22','run-lab23','run-lab24','run-lab25','run-lab26', 'run-lab27', 'run-lab28'):
            lab = (ev / (lab_log + '.log')).read_text()
            if 'RESULT=LAB_PASS' not in lab or 'failures=0' not in lab:
                raise RuntimeError('Laboratory did not pass: ' + lab_log)
        run('fix03-mutations', [sys.executable, '-I', str(PROJECT / 'tests/test_fix03_mutations.py')])
        run('fix02-mutations', [sys.executable, '-I', str(PROJECT / 'tests/test_fix02_mutations.py')])
        run('rework-mutations', [sys.executable, '-I', str(PROJECT / 'tests/run_mutations.py')])
        # POST07: LAB reference renders rasterised from the REAL quad list compiled with MSVC (not game screenshots)
        run('hud-reference-renders', [str(build / 'post05_hud_lab.exe'), '--render', str(ev)])
        r['hud_reference_renders'] = sorted(f.name for f in ev.glob('POST07_REFERENCE_RENDER_*.png'))
        if len(r['hud_reference_renders']) != 3:
            raise RuntimeError('HUD reference renders missing')
        # LNK01: LAB reference renders of Link's bar (1080p, 720p, ultrawide) rasterised from the REAL quad list compiled with MSVC
        run('lnk01-reference-renders', [str(build / 'post05_lnk_lab.exe'), '--render', str(ev)])
        r['lnk01_reference_renders'] = sorted(f.name for f in ev.glob('LNK01_REFERENCE_RENDER_*.png'))
        if len(r['lnk01_reference_renders']) != 3:
            raise RuntimeError('LNK01 reference renders missing')
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
        if {k: done[k] for k in UNIT_PLAN} != UNIT_PLAN:
            raise RuntimeError('Progress plan is not what ran: %s vs %s' % (done, UNIT_PLAN))
        r['labels'].append('WINDOWS_MSVC_PASS')
        # --- entrega privada ----------------------------------------------------------------------------------
        name = 'LIGHT_FADES_TWICE_LIZALFOS_FIX04_0_12_4_PRIVATE_TEST_windows-amd64_%s.dusk' % cs['source_commit'][:12]
        delivery = work / 'delivery'
        delivery.mkdir(exist_ok=True)
        shutil.copyfile(pkg, delivery / name)
        meta = {'filename': name, 'package_bytes': len(pb), 'package_sha256': sha(pb), 'dll_sha256': sha(before), 'source_repo': cs['source_repo'],
                'source_commit': cs['source_commit'], 'ci_head': r['ci_head'], 'workflow_run_id': r['run_id'], 'official_sdk': PINS['official_commit'],
                'aurora': PINS['aurora_commit'], 'mod_id': manifest['id'], 'version': manifest['version'], 'private_test_build': True,
                'installed': False, 'runtime_tested': False, 'runtime_status': 'HUMAN_RUNTIME_PENDING', 'gameplay_changes': True,
                'cp02_cp03': {'regen_rates':[10,7,4,2,1],'regen_delay_eligible_ticks':60,'simulation_hz':30,'invalid_hp_fallback':5,'hp_max_source':'OBSERVED_MAX_THIS_GENERATION_LOWER_BOUND','pause_gate':'existing lg.sim_active, enemy-only','parry_frame_delta':[-1,7],'parry_age_max_ticks':9,'parry_rewards_unchanged':[40,27,20],'fury_policy':'UNCHANGED_REFUSED_IF_CHIP_POSSIBLE','decisive_bonus':'UNCHANGED_X1_5_ONCE','link5to8':'NOT_APPROVED_NOT_APPLIED','human_runtime':'PENDING'},
                'hud_fix03': {'version':'0.11.5','prompts':'LIVE_EMP_OWNER_EXACT_SLOT_SUBTREES_SCOPED_FX_FOREIGN_PRESERVED','alignment':'AUTHENTIC_CP06_FULL_TRACKER_MIRROR','reference_commit':'7bb569a9cdf047034dc940fa5733c0f339a1e962','runtime':'HUMAN_RUNTIME_PENDING'},
                'game_writes': 'BALANCE SESSION01 0.11.2 NEW: private posture sidecar weights and standing25 ONLY, no new native writes; INHERITED LNK SESSION02 FIX01 (0.11.1): Link FLG2_UNK_8000000 switched OFF only (setShieldGuard POST on a denied lock-on guard; setCollision PRE during penalty/rearm), BTN_B cleared from mItemTrigger only for an offered R+B Shield Attack (setStickData POST), forced returns at the Shield Attack site (spActionTrigger POST = R+B) and for a vetoed voluntary dodge (checkMoveDoAction PRE = FALSE), setRStatus(0x3a) skipped, setBStatus HUD label; LNK SESSION02: one native Link transition daAlink_c::procGuardBreakInit() called from the existing checkDamageAction POST (same call site as the game), entry verified by mProcID readback, no mProcID/field_0x2fd5/mModeFlg/mDamageTimer/life write; FIX01: paired own AtSet bit0 lease in cc_set PRE/POST (+0x1038/+0x11f0), engine clears interrupted contacts with no added collision; INHERITED: audited actor reaction, vitality, duplicate-Helm cooldown1; scoped live Link cut5/8 only; HUD: own packet pointer in the 2D XLU list only; HUD FIX03 HUD_VISUAL_ONLY: live mpEmpButton owner + exact slot identity + geometry; roots and foreign draws always continue; scoped draw-enable and font alpha leases; Pikari original always executes; original action/status/input unchanged; root prediction restores exact fields; no native pointers persist after paired scopes. Legacy root hiding/sparkle masking DISABLED',
                'hook_targets': sorted(audit['hook_thunks']),
                'hook_meta_kinds': audit['meta_kinds'], 'game_import_count': len(audit['imports']['dusklight.exe']),
                'fix01_ai': 'Native engine clears fresh contacts interrupted by a posture intervention via paired own AtSet registration; verify next-pass clear; safe continuation gate; recovery requires real arming and native terminal exit; no state watchdog',
                'balance_session01': {'weights_sword_parry_iron': {'BLUE':[25,40,50],'RED':[17,27,33],'BLACK':[13,20,25],'BLACK_FURY':[13,20,25]},'standing_rearm':25,'getup_rearm':0,'cap':100,'threshold':100,'regen_delay':60,'regen_rates':[10,7,4,2,1],'classification':'same LNK07 resource, both-sphere At power, per-generation history; no HP heuristic','preview_commit':'same immutable snapshot plus checked preview receipt','new_native_writes':False,'new_hooks':0,'new_game_imports':0,'runtime':'HUMAN_RUNTIME_PENDING'},
                'lnk01': 'Inherited bottom-centre posture HUD and visual prompt masking; same private counter and attack-token deduplication. Current weights and recovery are specified by lnk_session01; the native Link guard break at posture 100 is specified by lnk_session02',
                'hud': 'POST07 display-only posture bar below the EHB-equivalent life-bar slot; centre-out fill; once-per-crossing pulse; optional InterpService',
                'rearm_fix': 'FIX_REARM50: unhit live normal mode after observed BIG state 6 rearms 0 (native FIND 5 exit redirected by another AI hook)', 'posture': 'POST06_NUMERIC_TRANSACTIONAL_REARM_25_OR_0', 'reaction_policy_id': 'POST05_TEST_POLICY_V4_GENERAL_ANTI_STAGGER', 'stagger_policy': 'GENERAL_POSTURE_BELOW_100', 'write_set': 'actor reaction+health+lethal attention; duplicate Helm cooldown1; scoped Link cut5/8; FIX01 paired own AtSet registration mask only', 'admission_policy': 'V2_COMPAT', 'e2_policy': 'MATURE_BC_OR_BRAKE', 'e_oc2': 'BC_MATURE_ATTACK0_BRAKE_PLUS_EARLY_TICKS_IF_NATIVE_ONLY_OR_RATE_PROVEN', 'e2_env_probe': 'get_service existence only, mod_update only', 'game_actions': 'DAMAGE_AND_BIG_DAMAGE_REACTION_UNDO_BELOW_100+POST06_STUN_DECISIVE_X1_50_NATIVE_FINISH_REARM', 'post06': 'break->native shield stun; future decisive contact x1.50 once->native fall/Finish/getup; timeout on feet posture25; no timed drop', 'version_line': 'LIGHT FADES TWICE CP06 0.11.11 (base compiled 0.11.10 d6e4c217; all37 0.11.10 production files frozen outside the exact counted CP06 inverse patch)', 'lnk_session02_fix01': 'Manual shield from Lazy Shield Rework 1.0.0 (Kamilink, github.com/kamilink7/lazytweaks-modapi@62620e7, hooks.cpp a0c7a320) - manual shielding and R+B Shield Attack ONLY, no crouch shielding/attacks: hold R (with lock-on) to guard, L alone never guards, R+B = native procGuardAttackInit (POST08 parry kept). Extended penalty: from the confirmed native guard-break entry, 75 active simulation ticks (2.5 s at 30 Hz, pause frozen) without guard, Shield Attack/parry or voluntary dodges (checkMoveDoAction JUMP/0x79 admission veto), native animation untouched (speed 1.0, cancel 28, end 35), posture reset once at the native exit; shield rearm only by a new R press after the unlock (release then press); LNK04 x1.5 only with an accepted manual guard; native breaks without saturation: rearm latch only; six new Link hook targets, all-or-nothing', 'rollback_0_11_0': '6f880aff5d589931e0100879685b6c1efad9b76bb7478a8b1b891b48daecbcc2', 'lnk_session02': 'LNK05: a confirmed block or confirmed direct hit that leaves Link posture at 100 requests ONE native guard break bound to that episode (block: same/next Link execute, <=3 ticks; direct: waits for the native damage reaction, <=24 ticks); gate: same Link/epoch, human, alive, no pause/menu/event, allowed free ground origin or GUARD_SLIP, !mModeFlg(0x70C52|0x4000000), no native damage this execute, not already GUARD_BREAK; parry cancels; native breaks adopted, never doubled. LNK06: native vulnerability; exit observed per tick and classified by last frame vs mGuardBreakAnm cancel 28/end 35; posture reset once after the native exit; regeneration held while pending/active; HUD pulse reused', 'rollback_0_11_5':'1dbf22a4c985e2ff334bd774faba86ac6bef717e55d135dbca7755103cd2f660', 'rollback_0_11_2': '2aa715980bc81af0db13622fc9e210100bcd973cd1255044d570a91f092eba0a', 'rollback': '0.11.1 8325dcf92d1ad7f39b330cef55713930d4dfce39c7baba27f64c8ec7d23e93ac; second0.11.0 6f880aff5d589931e0100879685b6c1efad9b76bb7478a8b1b891b48daecbcc2', 'lnk_session01': 'single Link posture counter: block (variant weight), confirmed direct hit (POST08 NOT_BLOCKED fresh joined to same-pass native checkDamageAction PRE/POST receipt: first cylinder, no native poly/electric branch, scoped timer and real life delta, then next-update native evidence; else not credited), parry +0; regen by life band 10/8/5/2 pts/s x1.5 with a real upper guard; variants from resource + Expansion attack power (1/2 Blue, 2 Red, 3 Black, 6 Black fury) with per-generation history; FIX01:4 additional hook targets, three read-only observers and one paired own AtSet registration lease; no new Link writes', 'fix02': 'fresh armed-pass At contacts only (ArmTrack); parry deflection lets native executeAttack arm/rebound (no ATTACK/3 write while Link is guarded); BREAK_BY_PARRY deferred one call; read-only AI watch (POST08_AI_TRACE)', 'fix01': 'Link state captured once per sim tick in mod_update (= state during the previous collision pass); verdict uses that snapshot, not the post-block GUARD_SLIP state; CP03 window [-1,+7] frames and <=9 ticks; dedicated POST08 log budgets', 'parry': 'Shield Attack precision parry: same collision pass enemy At hit on live Link Tg + native AtShieldHit + PROC_GUARD_ATTACK window; verified variant gain40/27/20 via apply_parry; deflect uses audited NativeContinues/RestorePre/ForceRecoil plan; crossing -> deferred POST06 BREAK_BY_PARRY; fury Black refused (Expansion chip)', 'lookup': 'fopAcM_SearchByID in mod_update only'}
        meta['cp04']={'selection':'1/2_per_genuine_native_attack_opportunity','rates':[1.1,1.1,1.5,1.5],'gaps_sim_executes':[8,8,2],'rates_0_11_10':[1,1,1.5,1.5],'gaps_0_11_10':[12,12,2],'native_shield_init':True,'one_native_shield_action_per_parry':True,'fast_shield_component':'GENERAL_REUSABLE_NEW_R_B_INPUT','visual':'VISUAL_SHIELD_MOTION_HUMAN_PENDING','runtime':'HUMAN_RUNTIME_PENDING','base_compiled_source':'8c72aec291a7473f1752c702137229cc2224c998'}
        meta['rollback_0_11_6']='0aa8e8d2941d644a33765fe288bc3f9867cc1e188506fed691caf069c63fb019'
        meta['rollback_0_11_6_run']=41703645193
        meta['rollback_0_11_6_artifact']=13264179241
        meta['cp04']['one_parry_per_native_shield_action_global']=True
        meta['cp04']['initial_frame_without_native_progress_rejected']=True
        meta['cp04']['untrusted_init_never_borrows_input']=True
        meta['game_writes']+='; CP04 selected Blue fresh-clip setPlaySpeed, typed setActionMode at native full-clip exit/gap expiry, one-shot own field_0x6c2 gap; confirmed-parry/new-input Link procGuardAttackInit native animation/voice/collider reset, no HP/position/animation-frame writes'
        meta['rollback_0_11_7']='e446ed3ee1111837a9feed628adf939942a006e60913fe66b00d48986b71826e'
        meta['rollback_0_11_7_run']=41746570392
        meta['rollback_0_11_7_artifact']=13283289812
        meta['cp05']={'base_compiled_source': '2d103c5562f7199b875d5c4c7de7f00a32922a61', 'red_probabilities': ['1/2', '1/2'], 'black_normal_probabilities': ['1/3', '1/3', '1/3'], 'black_reduction': 'UNBIASED_UINT64_REJECTION', 'kinds': {'RED4': 4, 'BLACK8': 8, 'BLACK7': 7}, 'per_swing_rates': {'RED4': [1.05, 1.35, 1.35, 1.05], 'BLACK8': [1.05, 1.05, 1.3, 1.3, 1.3, 1.3, 1.3, 1.05], 'BLACK7': [1.3, 1.3, 1.05, 1.05, 1.2, 1.2, 1.2]}, 'maximum_rate': 1.35, 'black_normal_maximum_rate': 1.3, 'rhythm_policy': 'SEGMENT_NATIVE_END_AT_DEADLINES', 'at_begin_targets_ticks': {'RED4': [60, 36, 62], 'BLACK8': [62, 62, 38, 38, 38, 38, 64], 'BLACK7': [38, 62, 64, 47, 47, 47]}, 'at_begin_targets_0_11_10': {'RED4': [68, 46, 72], 'BLACK8': [65, 67, 46, 46, 46, 46, 70], 'BLACK7': [46, 68, 70, 56, 56, 56]}, 'native_end_recoil_braking_may_extend_targets': True, 'native_timer_lease_only_during_execute': True, 'fury_and_red_specials_unchanged': True, 'blue_and_shield_source_frozen': True, 'new_native_hook_targets': 0, 'runtime': 'HUMAN_RUNTIME_PENDING'}
        meta['cp05_fix01']={'previous_defective_source':'e226415ebb4c5ad1d488898357072dc7c503a57d','previous_human_runtime':'REJECTED_1_OF_197_COMBOS_COMPLETED','raw_Tg_is_not_committed_priority':True,'committed_native_injury_still_cancels':True,'changed_native_mode_timer_never_restored':True,'AT_centres_and_radii':'READ_ONLY','native_geometry_or_spark_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING'}
        meta['preserved_defective_0_11_8']={'run':41768178836,'artifact':13293813585,'sha256':'8905dbee957d43b4dbb2cb2211daf0dc66b8517a3f2a131ec8d1240f3123a7c6'}
        meta['game_writes']+='; CP05 FIX01: fresh normal115 one-shot per-swing1.0/1.15, native full-clip mode calls, compare-write16bit timer only in the same Find4 native execute lease. Raw Tg is deferred until actual damage/suppression; committed HP/weighted injury and native priority still cancel. AT centres/radii read-only; no new Link/HP/position/damage/hitbox/animation-frame writer.'
        meta['cp05_fix02']={'base_compiled_source':'785f7044a09588ea9f570f09b4b74580e4f7ee93','ordinary_Link_sword_terminal_native_receipt':True,'exact_PRE_POST_and_live_animation_readback':True,'old_POST05_transaction_and_writer_preserved':True,'all_fresh_native_Link_contacts_use_existing_paired_cleanup':True,'native_inactive_AT_rejected_before_collision':True,'fresh_guard_can_initialize_existing_layout_receipt':True,'guard_observer_all_variants_read_only':True,'guard_observer_callback_priority':-300,'new_native_writers':0,'new_hook_targets':0,'combo_profiles_RNG_Blue_shield_Link_HUD_unchanged':True,'real_game_or_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING'}
        meta['preserved_0_11_9']={'source':'785f7044a09588ea9f570f09b4b74580e4f7ee93','run':41787837430,'artifact':13302951716,'sha256':'875e049ebb76f2deee50e7d2c62d73f3fe762d1f936925b4d008648b11cad4e6','human':'COMBOS_CONFIRMED_TWO_REACTION_GUARD_DEFECTS_REPORTED'}
        meta['game_writes']+='; CP05 FIX02 adds no native writer: strict authenticated stopped-B/C or stopped-recoil ordinary Link sword receipt uses existing POST05 compare/readback restore; all fresh native Link contacts use existing paired cc_set registration lease with native AtSet off again before the collision pass. Continuous guard observers are read-only.'
        meta['cp06']={'base_compiled_source':'d6e4c2179c2c42a7aa807ce862290de728abb396','version':'0.11.11','blue_rates':[1.1,1.1,1.5,1.5],'blue_gaps':[8,8,2],'red4_rates':[1.05,1.35,1.35,1.05],'black8_rates':[1.05,1.05,1.3,1.3,1.3,1.3,1.3,1.05],'black7_rates':[1.3,1.3,1.05,1.05,1.2,1.2,1.2],'at_targets':{'RED4':[60,36,62],'BLACK8':[62,62,38,38,38,38,64],'BLACK7':[38,62,64,47,47,47]},'parry_window_frames':[-1,7],'parry_age_max_ticks':9,'new_R_B_per_parry':True,'fury_parry':'ATP6_BOTH_SPHERES+LNK07_BLACK_FURY+EXPANSION_X2_EXACT','fury_chip':'EXPANSION_CP19F2_EXACT_ONE_UNIT_IN_SAME_EXECUTE_ATTRIBUTED_AND_RETURNED_ONCE','fury_chip_flash':'EXPANSION_RENDER_ONLY_NOT_CONTROLLED','red_special_x2':'UNCHANGED_SELECTION_RATE_SWEEP_PARRY_WITH_TIMELY_NATIVE_SHIELD_ATTACK','late_input_telemetry':'READ_ONLY','new_native_writers':1,'new_native_writer_functions':['cp06_native_chip_restore'],'new_hook_targets':0,'new_callbacks_on_existing_targets':2,'new_game_imports':0,'real_game_or_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING'}
        meta['rollback_0_11_10']='9c8f0074962386d7f46984cab43c0cff8fb429911d731e846aa52bef44e4bb4f'
        meta['rollback_0_11_10_run']=41856406121
        meta['rollback_0_11_10_artifact']=13338164623
        meta['game_writes']+='; CP06 adds ONE native writer: cp06_native_chip_restore returns exactly one pending life unit (g_dComIfG_gameInfo.play.setItemLifeCount(+1,type)) only when Expansion CP19F2 charged exactly -1 between the own POST(90) and POST(-250) of the SAME daE_OC_c::execute that committed an authenticated Black-fury parry (ATP6, LNK07 BLACK_FURY, rate2.0, engine shield pair); exact readback; one per blow token. CP06 rhythm changes reuse the existing CP04/CP05 rate and mode writers with new per-index values only.'
        meta['cp06f1']={'base_compiled_source':'0ef0ad2824832c9e03d899c298b5f9a8e860d590','version':'0.11.12','p0a':'EXACT_VANILLA_POSES_ENGINE_MODELCALC_RESTORE_VERIFIED_ONLY_ONTO_LINK_REGISTERED_TG_CYLINDERS','vanilla_window_poses':[15,23],'max_pose_gap_frames':0.5,'write_margin':0.5,'rates_covered':'GT1_LE2','vanilla_root_offset':'LUNGE_TABLE_ALONG_SHAPE_ANGLE','foreign_sweep_native_pose_restored':True,'p0b':'EXPANSION_CP19F2_SCHEDULE_MIRROR_E_L_EXACT_VALUE_ONLY_COUNTERFACTUAL','new_native_writers':3,'new_native_writer_functions':['cp06f1_native_sphere_place','cp06f1_native_pose_frame','cp06f1_native_kcolor'],'new_hook_targets':1,'new_hook_target_names':['daAlink_c::modelDraw'],'new_callbacks_on_existing_targets':5,'new_game_imports':4,'rhythms_parry_chip_unchanged':True,'real_game_or_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING'}
        meta['rollback_0_11_11']='7bcebf6ff442147d6741daf65d9b1e2128b524af8219296272ab64f236d15be3'
        meta['version_line']='LIGHT FADES TWICE CP06 FIX01 0.11.12 (base compiled 0.11.11 0ef0ad28; all 39 0.11.11 production files frozen outside the exact counted FIX01 inverse patch)'
        meta['rollback_0_11_11_run']=42298979283
        meta['rollback_0_11_11_artifact']=13543119520
        meta['game_writes']+='; CP06 FIX01 adds THREE native writers: cp06f1_native_sphere_place (one own AT sphere centre+radius via the engine setters SetC/SetR, only onto an exact vanilla/native pose that reaches one of Link\'s registered ON Tg cylinders with 0.5 margin while no registered sphere does; cc_set rewrites it next execute), cp06f1_native_pose_frame (own morf frame field + engine modelCalc, then the exact frame back and modelCalc; every joint matrix verified equal, else coverage disabled), cp06f1_native_kcolor (Link tevStr.TevKColor r/g/b during one body/face/hat/hand draw, only when the value equals Expansion\'s exact expected flash write, replaced by the counterfactual without CP06-neutralized chips; restored only if it is still exactly ours)'
        meta['liz']={'base_compiled_source':'5c8ac17e75ed2c8d2b9995277edec59e3ea8360d','base_global_0_11_12':'2a57ebeeafe6ec48ee5b63b5f614b6a0fff95b5e','version':'0.12.2','actor':'E_DN e_dn_class 435; excluded: arg1==1 (Dynalfos skull), skull model, arg0 13..16/ACTION_REG, status!=0, D_MN07 room 8',
                     'hp':370,'hp_regen':0,'posture_cap':100,'gains_sword_linkparry_iron_block':[6,12,30,5],'gain_to_self_on_enemy_parry':0,'wound_guard_pct':[45,80,100],'decisive_hit_extra':'(base+1)/2 once','link_break_damage':'x1.5 on attributed Tg contacts during a saturated posture break episode; route setDamagePoint argument','gain_to_link_on_enemy_parry':25,'posture_regen':[10,7,4,2,1],'regen_delay':60,'rearm_after_recovery':0,
                     'patterns':{'P1':['N','D','C'],'P2':['N','N','N','D','N','C'],'P3':['D','N','D','N','N','D'],'P4':['C','C','N','D','C']},'actions':[3,6,6,5],'impacts':[4,7,9,6],'selection':'25/25/25/25 UNBIASED_REJECTION_ONE_DRAW_PER_EPISODE',
                     'rates':{'N':1.25,'D':1.10,'C_slow':1.10,'C_fast':1.20},'shield_cancel':'at_hit_check POST returns NULL inside ACTION_ATTACK; the native setPlaySpeed(0)/FIGHT_RUN branch never runs','parry_pct':[12,35,45],'pressure_swings':{'B':[3,75],'C':[4,150]},'parry_window_ticks':[3,7],'parry_cooldown_ticks':{'A':120,'B':90,'C':60},'two_blocks_new_parry_window_pct':80,'counter_only_after_real_parry':True,'ordinary_block':'no HP, +5, no rebound','parry':'+0 self, +25 Link, native rebound','counter_kinds':{'N':1.25,'D':1.10},'counter_split':'50/50','break':'STANDING_DRAWBACK_THEN_DECISIVE_NATIVE_FALL','punish_ticks':90,
                     'd1_d2_physical_gap_s':0.367,'new_hook_targets':16,'by_name_hook_records':12,'new_game_imports':10,'new_native_writers':14,'new_native_writer_functions':['liz_native_anim','liz_native_at_place','liz_native_bash_se','liz_native_break','liz_native_fields','liz_native_guard','liz_native_hold','liz_native_hp','liz_native_hp_bonus','liz_native_learn','liz_native_link_feedback','liz_native_pose_frame','liz_native_stagger','liz_native_tg_clear'],'combo_timing_changed':False,
                     'real_game_or_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING','bokoblin_paths_changed':False}
        meta['liz']['version']='0.12.4'
        meta['lft03']={'base_compiled_source':'c242770e4f155e206590be978a800c461ae4ebc6','version':'0.12.3',
                       'p0_1':'Link shield chain cancelled by Link world safety only (0.12.2: also by the E_OC-only layout gate, -1 until a sword hit an E_OC: dpair=0, buffered=0, rearms=0, matched+gap_arms==chain_cancelled in both human logs); E_OC parry commit evaluates the pure layout check itself',
                       'p0_2':'shield cover = real tate_sph / JNT_ARML2*(50,0,30) centre at least 10 units toward Link from the body centre, never beyond +-112.5 deg of the body facing; one verdict per pass reused by the credit; guard turns toward Link with cLib_addCalcAngleS2(cur, search, 2, 0x800)',
                       'p1_3':'the parry contact own native hit-mark emitters (setHitMark inside ProcAtTgHitmark, and E_DN own shield spark) recoloured per instance in executeAfter: HSV hue 28 deg, S/V/alpha kept; the same contact collision SE playback movePitch x1.12; decided in the collision pass with the same pure judges, settled by the next verdict (mismatch reverted and counted)',
                       'p1_4':'mod option sekiro_lockon (ConfigService bool, default ON, toggle bound in the Mods panel): enemy lock kept after L release, second press releases, right stick flick (0.70/0.30) moves to the nearest enemy on that side; option OFF, menus, events, wolf, horse: nothing written',
                       'shield_attack_rate':1.0,'combo_timing_changed':False,'balance_changed':False,'new_hook_targets':5,'new_hook_records':{'HOOK_MEM':3,'HOOK_NAME':1,'LinkGuardSe_pre':1},
                       'new_game_imports':9,'new_services':['dev.twilitrealm.dusklight.config 1.0 (optional)','dev.twilitrealm.dusklight.ui 2.3 (optional)'],'new_native_writers':5,
                       'new_native_writer_functions':['lockon_native_pad','lockon_native_attention','pfx_native_colors','pfx_native_callback','pfx_native_pitch'],
                       'real_game_or_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING'}
        meta['lft04']={'base_compiled_source':'2aa4abe2e3b4c420dc67c3588e7ba8427eab9b71','version':'0.12.4','hook_removed':'def_se_set',
                       'p0_shield':'cover = presented by the guard clip + Link inside the body arc (-67.5/+112.5 deg, shield on the left arm) + the engine own tate_sph contact; 0.12.3 demanded the arm anchor 10 units toward Link and the held pose failed it',
                       'p1_fx':'PARRY_FX was disabled because def_se_set has a rel8 branch inside the 5 bytes funchook relocates; the Lizalfos clash is now recognised in Z2SoundObjBase::startCollisionSE POST by owner and map info 40',
                       'combo_timing_changed':False,'balance_changed':False,'lockon_changed':False,'shield_attack_rate':1.0,'new_native_writers':0,'new_game_imports':0,'real_game_or_visual_certification':False,'runtime':'HUMAN_RUNTIME_PENDING'}
        meta['rollback_0_12_3']='f3bab58159aa270ad72cec721523bee8ec675cef476aad8f7b151f7d9b4c94a8'
        meta['rollback_0_12_3_run']=42932174664
        meta['rollback_0_12_3_artifact']=13820291574
        meta['rollback_0_12_2']='87725c176e595b7c69e20fff5bdd4b44764d050f2cb6b248a8b8dd32092f94ad'
        meta['rollback_0_12_2_run']=42556628271
        meta['rollback_0_12_2_artifact']=13649337870
        meta['rollback_0_12_1']='08b18329128041b9b4bec99d10e2736fbd942fe58d4a1b037e7b88aef4480e23'
        meta['rollback_0_12_1_run']=42531603399
        meta['rollback_0_12_1_artifact']=13639325429
        meta['rollback_0_12_0']='0565555c45f683684a60b6c796f18c474604751468b682e8596c4396136b1b99'
        meta['rollback_0_12_0_run']=42506745010
        meta['rollback_0_12_0_artifact']=13628437919
        meta['rollback_0_11_12']='82f678a8ffacf197d7828721513597fe1b4d3c6752ca07ce2d6a6c9c2797d2ed'
        meta['rollback_0_11_12_run']=42482972682
        meta['rollback_0_11_12_artifact']=13618428357
        meta['version_line']='LIGHT FADES TWICE LIZALFOS FIX04 0.12.4 (base compiled 0.12.3 2aa4abe2; all 41 0.11.12 production files frozen outside the exact counted inverse patch)'
        meta['game_writes']+='; LIZ 0.12.2 adds TWO native writers: liz_native_hp_bonus (own health: +50% of the real loss of the confirmed decisive hit), liz_native_learn (sword bits of own learn field); liz_native_tg_clear also clears Link sword marks on own body spheres when the raised shield took that swing from the front; liz_native_hold also zeroes own unk_timer_2 on a NEW swing. Hook-argument raises, no memory write: setDamagePoint i_dmgAmount (+50%, attributed Tg contact, posture break episode). Hook-return change: checkCutReverseAt FALSE on an ordinary block of an admitted Lizalfos shield; LIZ 0.12.1 adds SIX more native writers: liz_native_stagger (ACTION_DRAWBACK mode 0 + native recoil fields at posture 100), liz_native_hold (timer[0] of the native S_DAMAGE wait, HeadLock off until a real new Shield Attack), liz_native_pose_frame (own morf frame + engine modelCalc, verified restore), liz_native_at_place (own at_sph centre onto an exact skipped native pose reaching an ON Tg cylinder; its velocity zeroed while Link shield is up), liz_native_bash_se (native def_se_set of a real Shield Attack contact below 100), liz_native_link_feedback (native setGuardSe + StartShock + native slip speed for a real shield record dropped by GUARD_SLIP); LIZ 0.12.0 SIX native writers on the executing admitted E_DN only: liz_native_fields (action/mode/invulnerability/timer[2]/current.angle.y/current.pos before CrrPos), liz_native_anim (morf frame field, play speed), liz_native_hp (health+max once from native 100/100), liz_native_guard (native guard entry fields), liz_native_break (mHitDirection + original big_damage + invulnerability), liz_native_tg_clear (inline Tg hit clear of Shield Attack marks)'
        meta['game_writes']+='; FIX03 0.12.3 adds FIVE native writers: lockon_native_pad (PAD_1 logical L hold/edge and right stick of THIS frame, option ON and an enemy locked), lockon_native_attention (dAttention_c mAttnStatus NONE on a SWITCH-option release; mLockOnOffset/mLockTargetID/field_0x32e on a right-stick target change), pfx_native_colors (mPrmClr/mEnvClr of the captured native spark emitters of a parry contact and of their particles, hue only), pfx_native_callback (mpEmtrCallBack of those emitters, chained to the native one), pfx_native_pitch (movePitch of the parry contact clash playback)'
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
