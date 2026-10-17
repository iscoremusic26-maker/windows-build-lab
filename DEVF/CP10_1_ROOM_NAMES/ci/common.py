import hashlib,struct
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];REPO=TASK.parents[1]
SOURCE_HOST="40457c6adb381928e4b5fef6ed459ed291edd5e2"
EXE_NAME="DEVF Map Editor CP10.1.exe"
def sha(b):return hashlib.sha256(b).hexdigest()
def git_blob(raw):return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()
def app_pe(path):
 b=path.read_bytes();assert b[:2]==b"MZ";p=struct.unpack_from("<I",b,60)[0]
 assert b[p:p+4]==b"PE\0\0" and struct.unpack_from("<H",b,p+4)[0]==0x8664
 assert struct.unpack_from("<H",b,p+24)[0]==0x20b
 return dict(machine="AMD64",bytes=len(b),sha256=sha(b),self_contained=True)
