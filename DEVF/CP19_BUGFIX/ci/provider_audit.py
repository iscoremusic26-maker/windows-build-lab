import importlib.util,json,zipfile
from pathlib import Path
REPO=Path(__file__).resolve().parents[3];ROOT=REPO/"DEVF/CP13_HYRULE_FIELD_ALL_ROOMS"   # CP14: provider 0.13.0 source and mod.json are CP13's, unchanged
spec=importlib.util.spec_from_file_location("cp10_provider_audit",REPO/"DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/ci/audit.py")
prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)
def verify(package,dll):
 with zipfile.ZipFile(package) as z:
  prior.old.safe_members(z);assert z.testzip() is None
  assert z.read("mod.json")== (ROOT/"provider/mod.json").read_bytes() and z.read("lib/windows-amd64/mod.dll")==dll.read_bytes()
 m=json.loads((ROOT/"provider/mod.json").read_bytes());assert m["id"]=="dev.devf.cp06_native_config_dual" and m["version"]=="0.13.0"
 return dict(bytes=package.stat().st_size,sha256=prior.sha(package.read_bytes()),dll_bytes=dll.stat().st_size,dll_sha256=prior.sha(dll.read_bytes()),pe=prior.inspect_pe(dll,"provider"))
