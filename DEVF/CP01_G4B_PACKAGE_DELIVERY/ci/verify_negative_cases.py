"""Malicious ZIP fixtures exercise inspection only; no deliverable .dusk is generated."""
import io,json,stat,zipfile,warnings
from verify_package import safe_members

def run():
    result=[]
    cases=['duplicate','case_alias','unexpected','traversal','absolute','drive','backslash','symlink','special','oversized','nonempty_directory','bad_crc','encrypted']
    for case in cases:
        stream=io.BytesIO()
        with warnings.catch_warnings():
            warnings.simplefilter('ignore',UserWarning)
            with zipfile.ZipFile(stream,'w') as z:
                z.writestr('mod.json',b'{}');z.writestr('lib/windows-amd64/mod.dll',b'own-fixture')
                extra={'duplicate':'mod.json','case_alias':'MOD.JSON','unexpected':'other.dll','traversal':'../other','absolute':'/other','drive':'C:/other','backslash':'lib\\other'}.get(case)
                if extra:z.writestr(extra,b'fixture')
                if case in ['symlink','special']:
                    # replace DLL member, not just an unexpected extra filename
                    pass
                if case=='nonempty_directory':z.writestr('lib/',b'not-empty')
        blob=stream.getvalue()
        with zipfile.ZipFile(io.BytesIO(blob)) as z:
            if case in ['symlink','special','oversized','encrypted','bad_crc']:
                i=z.getinfo('lib/windows-amd64/mod.dll')
                if case=='symlink':i.external_attr=(stat.S_IFLNK|0o777)<<16
                if case=='special':i.external_attr=(stat.S_IFIFO|0o600)<<16
                if case=='oversized':i.file_size=3*1024*1024
                if case=='encrypted':i.flag_bits|=1
                if case=='bad_crc':i.CRC^=1
            try:safe_members(z)
            except (RuntimeError,zipfile.BadZipFile):result.append({'case':case,'rejected':True})
            else:raise RuntimeError('Surviving unsafe fixture '+case)
    return result
if __name__=='__main__':print(json.dumps(run(),indent=2))
