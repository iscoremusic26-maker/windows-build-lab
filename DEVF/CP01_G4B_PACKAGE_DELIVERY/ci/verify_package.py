"""G2 strict package inspection adapted to G4B; never creates or loads deliverable packages."""
import hashlib,importlib.util,json,stat,struct,zipfile
from pathlib import Path
HERE=Path(__file__).resolve().parents[1]
OBS=HERE.parent/'CP01_G4B_LOCAL_READONLY_OBSERVER'
spec=importlib.util.spec_from_file_location('approved_g4b_audit',OBS/'ci/audit.py');audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
EXPECTED_FILES={'mod.json','lib/windows-amd64/mod.dll'}
ALLOWED_DIRECTORIES={'lib/','lib/windows-amd64/'}
EXPECTED_EXPORTS={'mod_ctx','mod_meta','mod_initialize','mod_update','mod_shutdown'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def safe_members(z,expected_files=EXPECTED_FILES,allowed_dirs=ALLOWED_DIRECTORIES):
    infos=z.infolist();names=[i.filename for i in infos]
    if len(names)!=len(set(names)) or len(names)!=len({n.casefold() for n in names}):raise RuntimeError('Duplicate/case-alias ZIP entries')
    if {i.filename for i in infos if not i.is_dir()}!=expected_files:raise RuntimeError('Unexpected/missing functional files')
    if any(i.is_dir() and i.filename not in allowed_dirs for i in infos):raise RuntimeError('Unexpected directory')
    if sum(i.file_size for i in infos)>4*1024*1024:raise RuntimeError('Unexpected expanded size')
    for i in infos:
        n=i.filename;mode=i.external_attr>>16
        if i.file_size>2*1024*1024 or i.flag_bits&1 or stat.S_ISLNK(mode) or stat.S_IFMT(mode) not in (0,stat.S_IFREG,stat.S_IFDIR):raise RuntimeError('Encrypted/link/special/oversized member')
        if '\\' in n or ':' in n or n.startswith('/') or any(s in ('','.','..') for s in n.rstrip('/').split('/')):raise RuntimeError('Unsafe member path')
        if i.is_dir() and i.file_size!=0:raise RuntimeError('Nonempty directory')
    if z.testzip() is not None:raise RuntimeError('CRC mismatch')
    return [{'path':i.filename,'directory':i.is_dir(),'bytes':i.file_size,'crc32':f'{i.CRC:08x}','sha256':sha(z.read(i))} for i in infos]
def pe_parts(raw):
    pe=struct.unpack_from('<I',raw,0x3c)[0];opt=pe+24
    if raw[:2]!=b'MZ' or raw[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',raw,pe+4)[0]!=0x8664 or struct.unpack_from('<H',raw,opt)[0]!=0x20b:raise RuntimeError('PE32+ AMD64 required')
    count=struct.unpack_from('<H',raw,pe+6)[0];table=opt+struct.unpack_from('<H',raw,pe+20)[0]
    def offset(rva):
        for i in range(count):
            vsize,va,size,pos=struct.unpack_from('<IIII',raw,table+40*i+8)
            if va<=rva<va+max(vsize,size):
                at=pos+rva-va
                if at>=pos+size or at>=len(raw):raise RuntimeError('Unmapped raw RVA')
                return at
        raise RuntimeError('Unmapped RVA')
    return pe,opt,offset

def pe_exports(path):
    raw=path.read_bytes();pe,opt,offset=pe_parts(raw)
    er,es=struct.unpack_from('<II',raw,opt+112);at=offset(er)
    functions,names,fr,nr,orr=struct.unpack_from('<IIIII',raw,at+20)
    if functions!=5 or names!=5:raise RuntimeError('Extra/ordinal-only export')
    result=[]
    for i in range(names):
        name_at=offset(struct.unpack_from('<I',raw,offset(nr)+4*i)[0]);end=raw.index(b'\0',name_at)
        name=raw[name_at:end].decode('ascii');ordinal=struct.unpack_from('<H',raw,offset(orr)+2*i)[0]
        if ordinal>=functions:raise RuntimeError('Bad export ordinal')
        address=struct.unpack_from('<I',raw,offset(fr)+4*ordinal)[0]
        if address==0 or er<=address<er+es:raise RuntimeError('Null/forwarded export')
        result.append(name)
    if set(result)!=EXPECTED_EXPORTS:raise RuntimeError('Unexpected export names')
    return sorted(result)

def historical_difference(path,approval):
    """Analyze an in-memory copy only; never rewrite the DLL. A matching historical SHA proves all other bytes equal."""
    raw=path.read_bytes();old=approval['historical_dll'];r={'new_sha256':sha(raw),'historical_sha256':old['sha256'],'new_bytes':len(raw),'historical_bytes':old['bytes'],'binary_rewritten':False}
    if r['new_sha256']==old['sha256']:r.update(status='IDENTICAL_HISTORICAL_BINARY',changed_fields=[]);return r
    pe,opt,offset=pe_parts(raw);current=struct.unpack_from('<I',raw,pe+8)[0];copy=bytearray(raw);fields=[]
    def replace(at,value,name):
        before=struct.unpack_from('<I',raw,at)[0]
        if before==value:return
        struct.pack_into('<I',copy,at,value);fields.append({'field':name,'file_offset':at,'new':before,'historical':value})
    replace(pe+8,approval['historical_coff_timestamp'],'COFF.TimeDateStamp')
    er,es=struct.unpack_from('<II',raw,opt+112)
    if er and struct.unpack_from('<I',raw,offset(er)+4)[0]==current:replace(offset(er)+4,approval['historical_coff_timestamp'],'Export.TimeDateStamp')
    dr,ds=struct.unpack_from('<II',raw,opt+112+6*8)
    if dr:
        if ds%28:raise RuntimeError('Invalid debug directory size')
        for i in range(ds//28):
            at=offset(dr)+28*i+4
            if struct.unpack_from('<I',raw,at)[0]==current:replace(at,approval['historical_debug_timestamp'],f'Debug[{i}].TimeDateStamp')
    r.update(changed_fields=fields,normalized_sha256=sha(copy),historical_equal_after_documented_timestamp_normalization=sha(copy)==old['sha256'])
    if sha(copy)!=old['sha256']:raise RuntimeError('Unexplained difference from historical DLL; delivery blocked')
    r['status']='PROVEN_ONLY_DOCUMENTED_PE_TIMESTAMPS_DIFFER';return r

def verify(package,dll,project=OBS,work=None):
    outside=audit.pe_audit(dll);exports=pe_exports(dll);original=dll.read_bytes()
    with zipfile.ZipFile(package) as z:
        members=safe_members(z);manifest=z.read('mod.json');expected=(project/'mod.json').read_bytes()
        if manifest!=expected:raise RuntimeError('Manifest byte mismatch')
        parsed=json.loads(manifest)
        if parsed.get('id')!='dev.devf.g4b_local_readonly' or parsed.get('version')!='0.0.1' or set(parsed)!={'id','name','version','description'}:raise RuntimeError('Wrong identity/options')
        inside=z.read('lib/windows-amd64/mod.dll')
        if inside!=original or sha(inside)!=sha(original):raise RuntimeError('Packaged DLL differs byte-for-byte')
    if work is None:raise RuntimeError('Inspection scratch required')
    copy=work/'packaged-inspection.dll';copy.write_bytes(inside)
    if audit.pe_audit(copy)!=outside or pe_exports(copy)!=exports:raise RuntimeError('Packaged DLL audit differs')
    return {'status':'G4B_PACKAGE_STRUCTURE_PASS','sdk_package_target':'devf_g4b_readonly_package','package_bytes':package.stat().st_size,'package_sha256':sha(package.read_bytes()),'dll_bytes':len(inside),'dll_sha256':sha(inside),'built_and_packaged_dll_byte_identical':True,'all_members_crc_pass':True,'manifest_byte_identical':True,'strict_contents_pass':True,'manifest':parsed,'members':members,'exports':exports,'pe_audit':outside,'packaged_pe_audit_identical':True,'dll_executed':False,'game_executed':False}
