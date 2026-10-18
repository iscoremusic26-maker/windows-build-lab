import hashlib,struct
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];REPO=TASK.parents[1]
SOURCE_HOST="40457c6adb381928e4b5fef6ed459ed291edd5e2"
SOURCE_AURORA="3227d76c60e1e782ca576610bce61c9e7744d8be"
EXE_NAME="DEVF Map Editor CP13.exe" # CP14: rebuilt from the unchanged CP13 source for regression tests only; NOT shipped
DUSK_NAME="BokoblinExpansion_0.11.0_CP14_TEST.dusk"
CP13_DIR="DEVF/CP13_HYRULE_FIELD_ALL_ROOMS"
CP13_PROVIDER=dict(filename="DEVF_Provider_0.13.0_CP13_TEST.dusk",bytes=130335,sha256="3a3c874e71cf8b63c43fb54340e11778011c486026897d6c29d3d6ad2e67634e",dll_sha256="2932b3e2c1b188ae89f22545e94431a81e71d79c96c076038ae286af9a328c83",run_id=39153841948,artifact_id=12146112007)   # the provider the user keeps installed (CP13 VERIFIED_PACKAGE.json)
EDITOR_CSPROJ="DEVF/CP13_HYRULE_FIELD_ALL_ROOMS/app/DEVF.MapEditor.csproj"
def sha(b):return hashlib.sha256(b).hexdigest()
def git_blob(raw):return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()
def app_pe(path):
 b=path.read_bytes();assert b[:2]==b"MZ";p=struct.unpack_from("<I",b,60)[0]
 assert b[p:p+4]==b"PE\0\0" and struct.unpack_from("<H",b,p+4)[0]==0x8664
 assert struct.unpack_from("<H",b,p+24)[0]==0x20b
 return dict(machine="AMD64",bytes=len(b),sha256=sha(b),self_contained=True)
