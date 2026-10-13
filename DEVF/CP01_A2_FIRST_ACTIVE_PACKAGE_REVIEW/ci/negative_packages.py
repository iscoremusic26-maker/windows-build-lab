"""Synthetic malformed packages only; does not produce deliverable .dusk."""
import io,json,stat,struct,zipfile,warnings
from verify_package import safe_members,contents

def run(expected_manifest):
 cases=['duplicate','case_alias','extra_binary','traversal','absolute','drive','backslash','symlink','special','oversized','false_directory','nonempty_directory','bad_crc','encrypted','ratio','manifest_absent','dll_absent','manifest_wrong_id','manifest_duplicate_json_id','manifest_drift','dll_substituted','malformed_zip','hash_corrupt']
 results=[]
 for case in cases:
  b=io.BytesIO()
  with warnings.catch_warnings():
   warnings.simplefilter('ignore',UserWarning)
   with zipfile.ZipFile(b,'w') as z:
    if case!='manifest_absent':
     m=expected_manifest
     if case=='manifest_wrong_id':m=m.replace(b'dev.devf.cp01_eoc_first_active_pilot',b'dev.devf.g4b_local_readonly')
     if case=='manifest_duplicate_json_id':m=m[:-2]+b',"id":"other"}\n'
     if case=='manifest_drift':m+=b' '
     z.writestr('mod.json',m)
    z.writestr('lib/',b'not-empty' if case=='nonempty_directory' else b'');z.writestr('lib/windows-amd64/',b'')
    if case!='dll_absent':z.writestr('lib/windows-amd64/mod.dll',b'wrong' if case=='dll_substituted' else b'own-fixture')
    extra={'duplicate':'mod.json','case_alias':'MOD.JSON','extra_binary':'other.dll','traversal':'../other','absolute':'/other','drive':'C:/other','backslash':r'lib\other'}.get(case)
    if extra:z.writestr(extra,b'fixture')
  blob=b.getvalue()
  if case=='malformed_zip':blob=b'not-a-zip'
  if case=='hash_corrupt':blob=blob[:-10]
  try:
   with zipfile.ZipFile(io.BytesIO(blob)) as z:
    if case in ['symlink','special','oversized','false_directory','bad_crc','encrypted','ratio']:
     i=z.getinfo('lib/windows-amd64/mod.dll')
     if case=='symlink':i.external_attr=(stat.S_IFLNK|0o777)<<16
     if case=='special':i.external_attr=(stat.S_IFIFO|0o600)<<16
     if case=='false_directory':i.external_attr=(stat.S_IFDIR|0o755)<<16
     if case=='oversized':i.file_size=3*1024*1024
     if case=='bad_crc':i.CRC^=1
     if case=='encrypted':i.flag_bits|=1
     if case=='ratio':i.compress_size=0
    safe_members(z)
   contents(io.BytesIO(blob),expected_manifest,b'own-fixture')
  except (RuntimeError,zipfile.BadZipFile):results.append({'case':case,'rejected':True})
  else:raise RuntimeError('Negative fixture survived '+case)
 return results
