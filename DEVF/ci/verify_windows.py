"""CP00 Windows input/package verification. Never execute the game or publish originals."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zipfile

EXPECTED = {
    'dusklight-2.0.3.zip': '9dbdc0b068df24c3fbb680c87be25e904978f04b315e2051e3c6203476516008',
    'Dusklight-v2.0.3-win32-x86_64.zip': 'ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf',
}

def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def write_json(path, obj):
    path.write_text(json.dumps(obj, indent=2) + '\n', encoding='utf-8')

def prepare(root, work):
    evidence = work / 'evidence'
    evidence.mkdir(parents=True, exist_ok=True)
    report = {'inputs': [], 'run_url': f"https://github.com/{os.environ.get('GITHUB_REPOSITORY')}/actions/runs/{os.environ.get('GITHUB_RUN_ID')}", 'source_sha': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()}
    for name, expected in EXPECTED.items():
        archive = root / name
        actual = sha(archive)
        if actual != expected:
            raise RuntimeError(f'Input hash mismatch: {name}')
        with zipfile.ZipFile(archive) as z:
            if z.testzip() is not None:
                raise RuntimeError(f'Input CRC failure: {name}')
            report['inputs'].append({'name': name, 'sha256': actual, 'bytes': archive.stat().st_size, 'entries': len(z.infolist()), 'crc': 'PASS'})
            if name.startswith('dusklight-'):
                for entry in z.infolist():
                    destination = (work / entry.filename).resolve()
                    if not destination.is_relative_to(work.resolve()):
                        raise RuntimeError('Unsafe source archive path')
                z.extractall(work)
            else:
                (work / 'windows-amd64.lib').write_bytes(z.read('sdk/windows-amd64.lib'))
    assert sha(work / 'windows-amd64.lib') == '4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4'
    write_json(evidence / 'inputs.json', report)
    print('Exact inputs: SHA256/CRC PASS; originals retained outside artifact directory')

def verify(root, work):
    if os.name != 'nt':
        raise RuntimeError('Verification requires a real Windows runner')
    evidence = work / 'evidence'
    package = work / 'build/mods/devf_cp00_probe.dusk'
    with zipfile.ZipFile(package) as z:
        assert z.testzip() is None
        members = [n for n in z.namelist() if not n.endswith('/')]
        assert set(members) == {'mod.json', 'lib/windows-amd64/mod.dll'}, members
        assert json.loads(z.read('mod.json'))['id'] == 'dev.devf.cp00_probe'
        dll = evidence / 'mod.dll'
        dll.write_bytes(z.read('lib/windows-amd64/mod.dll'))
    data = dll.read_bytes()
    assert data[:2] == b'MZ'
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe + 4] == b'PE\0\0'
    assert struct.unpack_from('<H', data, pe + 4)[0] == 0x8664
    assert struct.unpack_from('<H', data, pe + 24)[0] == 0x20b
    assert struct.unpack_from('<H', data, pe + 22)[0] & 0x2000  # DLL
    compiler_files = list((work / 'build/CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
    assert len(compiler_files) == 1
    compiler_text = compiler_files[0].read_text(encoding='utf-8')
    def cmake_value(name):
        match = re.search(r'set\(' + name + r' "([^"]*)"\)', compiler_text)
        assert match, name
        return match.group(1)
    compiler = cmake_value('CMAKE_CXX_COMPILER')
    compiler_id = cmake_value('CMAKE_CXX_COMPILER_ID')
    assert compiler_id == 'MSVC', compiler_id
    dumpbin = Path(compiler).with_name('dumpbin.exe')
    for option, name in [('/EXPORTS', 'exports.txt'), ('/IMPORTS', 'imports.txt'), ('/HEADERS', 'headers.txt')]:
        result = subprocess.run([str(dumpbin), option, str(dll)], text=True, capture_output=True, check=True)
        (evidence / name).write_text(result.stdout, encoding='utf-8')
    exports = (evidence / 'exports.txt').read_text(encoding='utf-8')
    for symbol in ['mod_ctx', 'mod_meta', 'mod_initialize', 'mod_update', 'mod_shutdown', 'devf_cp00_game_linkage']:
        assert re.search(r'\b' + symbol + r'\b', exports), symbol
    imports = (evidence / 'imports.txt').read_text(encoding='utf-8')
    assert 'dusklight.exe' in imports.lower()
    assert 'fopAcM_SearchByID' in imports
    services = {'actor': (1, 0), 'stage': (1, 0), 'resource': (1, 1),
                'host': (2, 2), 'log': (1, 0), 'game': (2, 0)}
    service_records = {}
    for service, version in services.items():
        service_id = ('dev.twilitrealm.dusklight.' + service).encode() + b'\0'
        matches = [match.start() for match in re.finditer(re.escape(service_id), data)]
        # The verified SDK's Win64 ModMetaImport is 80 bytes; ID is at offset 16.
        records = [struct.unpack_from('<HBBHH', data, at - 16) for at in matches if at >= 16]
        assert (80, 2, 0, *version) in records, (service, records)
        service_records[service] = {'record_size': 80, 'kind': 'required_import',
                                    'major': version[0], 'minimum_minor': version[1]}
    configure = (evidence / 'configure.log').read_text(encoding='utf-8-sig')
    sdk_match = re.search(r'CP00 Windows SDK: ([\d.]+)', configure)
    assert sdk_match, 'Windows SDK version missing'
    shutil.copy2(package, evidence / package.name)
    report = {
        'status': 'PASS', 'platform': 'Windows x64', 'compiler_id': compiler_id,
        'compiler_version': cmake_value('CMAKE_CXX_COMPILER_VERSION'), 'windows_sdk': sdk_match.group(1),
        'configuration': 'RelWithDebInfo', 'package_members': members,
        'dll_sha256': sha(dll), 'dusk_sha256': sha(package), 'native_link': 'dusklight.exe / fopAcM_SearchByID',
        'runtime_game': 'NOT_TESTED', 'dx12': 'NOT_TESTED', 'cp01': 'NOT_STARTED',
        'service_records': service_records,
    }
    write_json(evidence / 'verification.json', report)
    print(json.dumps(report, sort_keys=True))
    print('Windows/MSVC build, package, PE AMD64, lifecycle exports, service records and game import: PASS')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('operation', choices=['prepare', 'verify'])
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    {'prepare': prepare, 'verify': verify}[args.operation](args.root.resolve(), args.work.resolve())
