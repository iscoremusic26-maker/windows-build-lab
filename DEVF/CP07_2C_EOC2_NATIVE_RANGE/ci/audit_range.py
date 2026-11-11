"""Reuse exact PE/import reader; new provider manifest only. No DLL execution."""
import importlib.util,json,zipfile
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];REPO=TASK.parents[1]
ROOT=REPO/"DEVF/CP07_2B_BOKOBLIN_EXPANSION_PILOT"
spec=importlib.util.spec_from_file_location("cp072b_pe",ROOT/"ci/audit_api.py");old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
inspect_pe=old.inspect_pe;negatives=old.negatives;sha=old.sha;safe_members=old.safe_members
def verify_package(package,dll,mode):
 if mode!="provider":raise RuntimeError("PROVIDER_ONLY")
 with zipfile.ZipFile(package) as z:
  members=safe_members(z);manifest=z.read("mod.json");inside=z.read("lib/windows-amd64/mod.dll")
 if manifest!=(ROOT/"provider/mod.json").read_bytes() or inside!=dll.read_bytes():raise RuntimeError("Package identity")
 m=json.loads(manifest)
 if (m["id"],m["version"])!=("dev.devf.cp06_native_config_dual","0.7.4"):raise RuntimeError("Mod identity/version")
 return {"status":"PACKAGE_VERIFIED","package_bytes":package.stat().st_size,"package_sha256":sha(package.read_bytes()),"dll_bytes":len(inside),"dll_sha256":sha(inside),"manifest_sha256":sha(manifest),"members":members,"pe":inspect_pe(dll,mode)}
