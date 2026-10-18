import io,json,os,urllib.request,urllib.error,zipfile
from pathlib import Path
import sys
from common import *
def api(path):
 req=urllib.request.Request("https://api.github.com/repos/iscoremusic26-maker/codex-cp00-inputs/"+path,headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json"})
 return json.load(urllib.request.urlopen(req))
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,*a):return None
def download(artifact):
 url=artifact["archive_download_url"]
 try:
  urllib.request.build_opener(NoRedirect).open(urllib.request.Request(url,headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"]}))
  raise RuntimeError("Expected archive redirect")
 except urllib.error.HTTPError as e:
  if e.code!=302:raise
  signed=e.headers["Location"]
 with urllib.request.urlopen(signed) as response:data=response.read(400_000_001)
 assert len(data)<=400_000_000
 return data
run=os.environ["GITHUB_RUN_ID"];arts=api("actions/runs/"+run+"/artifacts")["artifacts"]
artifact=next(a for a in arts if a["name"]=="DEVF_Bokoblin_Damage_CP10.5_TEST_"+run)
evidence=next(a for a in arts if a["name"]=="DEVF_CP10_5_TEXT_"+run)
with zipfile.ZipFile(io.BytesIO(download(evidence))) as z:
 assert z.testzip() is None
 result=json.loads(z.read("result.json"))
with zipfile.ZipFile(io.BytesIO(download(artifact))) as outer:
 assert outer.testzip() is None and outer.namelist()==["DEVF_Bokoblin_Damage_CP10.5_TEST.zip"];bundle=outer.read(outer.namelist()[0])
assert len(bundle)==result["zip"]["bytes"] and sha(bundle)==result["zip"]["sha256"]
pin=json.loads((TASK/"SOURCE_CI_CP105.json").read_text())
work=Path(os.environ["RUNNER_TEMP"])/"cp10-5-independent";work.mkdir()
with zipfile.ZipFile(io.BytesIO(bundle)) as z:
 assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
 assert all(not n.startswith(("/","\\")) and ".." not in Path(n).parts and "\\" not in n for n in z.namelist())
 manifest=json.loads(z.read("MANIFEST.json"))
 assert manifest["source_commit"]==result["source_commit"]==pin["source_commit"]
 assert manifest["ci_commit"]==result["ci_commit"]==os.environ["EXPECTED_CI_SHA"]
 for name,m in manifest["files"].items():
  raw=z.read(name);assert len(raw)==m["bytes"] and sha(raw)==m["sha256"]
 assert set(z.namelist())==set(manifest["files"])|{"MANIFEST.json","SHA256SUMS.txt"}
 assert not any(n.lower().endswith((".dll",".arc",".kcl",".plc",".bmd",".bti",".png",".dds",".cmpr",".ciso",".iso")) for n in z.namelist())
 assert [n for n in z.namelist() if n.lower().endswith(".dusk")]==[DUSK_NAME]
 assert z.read("SHA256SUMS.txt").decode().splitlines()==[m["sha256"]+"  "+n for n,m in manifest["files"].items()]
 assert EXE_NAME not in z.namelist() and not any(n.lower().endswith(".exe") for n in z.namelist()) # editor reused, not shipped
 import audit,io as _io
 pkgdata=z.read(DUSK_NAME);m=manifest["package"];assert len(pkgdata)==m["package_bytes"] and sha(pkgdata)==m["package_sha256"]
 pkgpath=work/DUSK_NAME;pkgpath.write_bytes(pkgdata)
 with zipfile.ZipFile(pkgpath) as pz:
  assert pz.testzip() is None;audit.safe_members(pz);dllraw=pz.read("lib/windows-amd64/mod.dll");manifest_inside=pz.read("mod.json")
 assert sha(dllraw)==m["dll_sha256"] and len(dllraw)==m["dll_bytes"] and sha(manifest_inside)==m["manifest_sha256"]
 dllpath=work/"expansion.dll";dllpath.write_bytes(dllraw);pe=audit.inspect_pe(dllpath);assert pe["status"]=="CP10_5_PE_ABI_PASS"
 with zipfile.ZipFile(io.BytesIO(z.read("SOURCE_COMPILED.zip"))) as src:
  assert src.testzip() is None and set(src.namelist())==set(result["source_ledger"])==set(pin["source_blobs"])
  for name,h in result["source_ledger"].items():
   assert sha(src.read(name))==h and sha((REPO/name).read_bytes())==h and git_blob(src.read(name))==pin["source_blobs"][name]
 tr=json.loads(z.read("TEST_RESULTS.json"))
 assert tr["name_tests"]["cases"]==25 and tr["multi_tests"]["status"]=="CP10_STATIC_LAB_PASS" and tr["runtime_lab_cases"]==66 and tr["black_lab_cases"]==26 and tr["texture_lab_cases"]==20 and tr["variant_lab_cases"]==30 and tr["damage_lab_cases"]==22 and tr["editor_source_identical_to_cp102"] is True
 report=dict(status="DOWNLOAD_VERIFIED",run_id=run,artifact_id=artifact["id"],artifact_name=artifact["name"],expires_at=artifact["expires_at"],source_commit=result["source_commit"],ci_commit=result["ci_commit"],zip=result["zip"],source_ledger_verified=True,bytes_and_sha256_checked_on_downloaded_zip=True,package_audited_pe_abi_hooks_imports=True,no_loose_dll_no_game_assets=True,CRC="PASS",game_executed=False)
out=Path(os.environ["RUNNER_TEMP"])/"cp10-5-download-evidence";out.mkdir();(out/"download-result.json").write_text(json.dumps(report,indent=2)+"\n")
print("CP10_5_DOWNLOAD_RESULT="+json.dumps(report,separators=(",",":")),flush=True)
