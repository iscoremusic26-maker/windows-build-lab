import hashlib,struct,zipfile,io,json
from pathlib import Path
REPO=Path(__file__).resolve().parents[3]
TASK=REPO/"DEVF/CP09_UNIVERSAL_GAME_DATA_IMPORTER"
def sha(b):return hashlib.sha256(b).hexdigest()
def pe(b):
 assert b[:2]==b"MZ" and len(b)>256
 p=struct.unpack_from("<I",b,60)[0]
 assert b[p:p+4]==b"PE\0\0" and struct.unpack_from("<H",b,p+4)[0]==0x8664
 assert struct.unpack_from("<H",b,p+24)[0]==0x20b
 return dict(bytes=len(b),sha256=sha(b),machine="AMD64",optional_magic="PE32+")
def safezip(b):
 z=zipfile.ZipFile(io.BytesIO(b));names=z.namelist()
 assert len(names)==len(set(names)) and z.testzip() is None
 assert all(not n.startswith(("/","\\")) and "\\" not in n and ".." not in n.split("/") for n in names)
 assert sum(i.file_size for i in z.infolist())<500_000_000
 return z
