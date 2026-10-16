import importlib.util,json,zipfile,struct,io,hashlib
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];REPO=TASK.parents[1]
OLD=REPO/"DEVF/CP07_2B_BOKOBLIN_EXPANSION_PILOT"
spec=importlib.util.spec_from_file_location("prior_api_audit",OLD/"ci/audit_api.py")
old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha=old.sha
def inspect_pe(path,mode):
 # The public ABI depends on set membership, not linker section ordering.
 original=old.__dict__.get("inspect_pe")
 return original(path,mode)
def verify_package(package,dll,mode):
 with zipfile.ZipFile(package) as z:
  members=old.safe_members(z);assert z.testzip() is None
  manifest=z.read("mod.json");inside=z.read("lib/windows-amd64/mod.dll")
 assert manifest==(TASK/mode/"mod.json").read_bytes() and inside==dll.read_bytes()
 m=json.loads(manifest);assert (m["id"],m["version"])==(("dev.devf.cp06_native_config_dual","0.10.0") if mode=="provider" else ("dev.bokoblin.expansion","0.3.0"))
 return dict(bytes=package.stat().st_size,sha256=sha(package.read_bytes()),dll_bytes=len(inside),dll_sha256=sha(inside),manifest_sha256=sha(manifest),pe=inspect_pe(dll,mode))
def app_pe(path):
 b=path.read_bytes();assert b[:2]==b"MZ";p=struct.unpack_from("<I",b,60)[0]
 assert b[p:p+4]==b"PE\0\0" and struct.unpack_from("<H",b,p+4)[0]==0x8664
 assert struct.unpack_from("<H",b,p+24)[0]==0x20b
 return dict(machine="AMD64",bytes=len(b),sha256=sha(b),self_contained=True)

def safezip(b):
 z=zipfile.ZipFile(io.BytesIO(b));names=z.namelist()
 assert len(names)==len(set(names)) and z.testzip() is None
 assert all(not n.startswith(("/","\\")) and "\\" not in n and ".." not in n.split("/") for n in names)
 assert sum(i.file_size for i in z.infolist())<500_000_000
 return z
