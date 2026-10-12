"""Strict inspection of the SDK-produced package. Does not create or load a mod package."""
import hashlib
import json
import stat
import zipfile
from pathlib import Path
from audit import pe_audit

EXPECTED_FILES = {'mod.json', 'lib/windows-amd64/mod.dll'}
ALLOWED_DIRECTORIES = {'lib/', 'lib/windows-amd64/'}
EXPECTED_EXPORTS = {'mod_ctx','mod_meta','mod_initialize','mod_update','mod_shutdown'}
EXPECTED_RUNTIME = {
 'dusklight.exe', 'VCRUNTIME140.dll', 'api-ms-win-crt-stdio-l1-1-0.dll',
 'api-ms-win-crt-runtime-l1-1-0.dll', 'KERNEL32.dll',
}

def verify(package, dll, project, exports_log, work):
    outside = pe_audit(dll)
    if set(outside['imports']) != EXPECTED_RUNTIME: raise RuntimeError('Unexpected runtime DLL dependency')
    import re
    exports=set(re.findall(r'^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\w+)\s*$', exports_log, re.M))
    if exports != EXPECTED_EXPORTS: raise RuntimeError(f'Unexpected exports: {exports}')
    original=dll.read_bytes(); members=[]
    with zipfile.ZipFile(package) as z:
        names=z.namelist()
        if len(set(names)) != len(names): raise RuntimeError('Duplicate ZIP names')
        if {i.filename for i in z.infolist() if not i.is_dir()} != EXPECTED_FILES:
            raise RuntimeError('Package must contain exactly manifest and one AMD64 own DLL')
        if any(i.is_dir() and i.filename not in ALLOWED_DIRECTORIES for i in z.infolist()):
            raise RuntimeError('Unexpected directory')
        if z.testzip() is not None: raise RuntimeError('Member CRC failure')
        for i in z.infolist():
            mode=i.external_attr>>16
            if i.flag_bits&1 or stat.S_ISLNK(mode) or '\\' in i.filename or '..' in i.filename.split('/') or i.filename.startswith('/'):
                raise RuntimeError('Encrypted/link/unsafe package entry')
            raw=z.read(i)
            members.append({'path':i.filename,'directory':i.is_dir(),'bytes':len(raw),
                            'crc32':f'{i.CRC:08x}','sha256':hashlib.sha256(raw).hexdigest()})
        manifest=z.read('mod.json')
        # Only line endings may differ between Windows checkout and main-source Git blob.
        expected=(project/'observer.json').read_bytes()
        if manifest!=expected: raise RuntimeError('SDK copied an unexpected manifest')
        parsed=json.loads(manifest)
        if parsed['id']!='dev.devf.a1_readonly_preflight' or parsed['version']!='0.0.2':
            raise RuntimeError('Module identity/version differs')
        if set(parsed)!={'id','name','version','description'}: raise RuntimeError('Unexpected manifest options')
        inside=z.read('lib/windows-amd64/mod.dll')
        if inside!=original: raise RuntimeError('Packaged DLL differs from inspected build')
    packaged_copy=work/'packaged-inspection.dll'; packaged_copy.write_bytes(inside)
    inside_audit=pe_audit(packaged_copy)
    if inside_audit!=outside: raise RuntimeError('Packaged PE audit differs')
    return {'status':'PASS_STATIC','sdk_package_target':'devf_a1_observer_package',
            'package_bytes':package.stat().st_size,'package_sha256':hashlib.sha256(package.read_bytes()).hexdigest(),
            'dll_bytes':len(original),'dll_sha256':hashlib.sha256(original).hexdigest(),
            'built_and_packaged_dll_byte_identical':True,'all_members_crc_pass':True,
            'strict_contents_pass':True,'manifest':parsed,'members':members,
            'exports':sorted(exports),'pe_audit':outside,'packaged_pe_audit_identical':True,
            'dll_executed':False,'game_executed':False}
