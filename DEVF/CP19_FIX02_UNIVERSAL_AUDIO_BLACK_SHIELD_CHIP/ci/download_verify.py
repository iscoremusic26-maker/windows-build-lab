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
artifact=next(a for a in arts if a["name"]=="DEVF_CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP_TEST_"+run)
evidence=next(a for a in arts if a["name"]=="DEVF_CP19F2_TEXT_"+run)
with zipfile.ZipFile(io.BytesIO(download(evidence))) as z:
 assert z.testzip() is None
 result=json.loads(z.read("result.json"))
with zipfile.ZipFile(io.BytesIO(download(artifact))) as outer:
 assert outer.testzip() is None and outer.namelist()==["DEVF_CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP_TEST.zip"];bundle=outer.read(outer.namelist()[0])
assert len(bundle)==result["zip"]["bytes"] and sha(bundle)==result["zip"]["sha256"]
pin=json.loads((TASK/"SOURCE_CI_CP19F2.json").read_text(encoding="utf-8"))
work=Path(os.environ["RUNNER_TEMP"])/"cp19f2-independent";work.mkdir()
with zipfile.ZipFile(io.BytesIO(bundle)) as z:
 assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
 assert all(not n.startswith(("/","\\")) and ".." not in Path(n).parts and "\\" not in n for n in z.namelist())
 manifest=json.loads(z.read("MANIFEST.json"))
 assert manifest["source_commit"]==result["source_commit"]==pin["source_commit"]
 assert manifest["ci_commit"]==result["ci_commit"]==os.environ["EXPECTED_CI_SHA"]
 for name,m in manifest["files"].items():
  raw=z.read(name);assert len(raw)==m["bytes"] and sha(raw)==m["sha256"]
 assert set(z.namelist())==set(manifest["files"])|{"MANIFEST.json","SHA256SUMS.txt"}
 assert not any(n.lower().endswith((".dll",".arc",".kcl",".plc",".bmd",".bti",".png",".dds",".cmpr",".ciso",".iso",".exe")) for n in z.namelist())
 assert set(n for n in z.namelist() if n.lower().endswith(".dusk"))=={DUSK_NAME}   # CP16 ships ONLY the Expansion; the user keeps provider/editor CP13
 assert manifest["requires_provider"]==CP13_PROVIDER
 assert z.read("SHA256SUMS.txt").decode().splitlines()==[m["sha256"]+"  "+n for n,m in manifest["files"].items()]
 import audit,io as _io
 pkgdata=z.read(DUSK_NAME);m=manifest["package"];assert len(pkgdata)==m["package_bytes"] and sha(pkgdata)==m["package_sha256"]
 pkgpath=work/DUSK_NAME;pkgpath.write_bytes(pkgdata)
 with zipfile.ZipFile(pkgpath) as pz:
  assert pz.testzip() is None;audit.safe_members(pz);dllraw=pz.read("lib/windows-amd64/mod.dll");manifest_inside=pz.read("mod.json")
 assert sha(dllraw)==m["dll_sha256"] and len(dllraw)==m["dll_bytes"] and sha(manifest_inside)==m["manifest_sha256"]
 dllpath=work/"expansion.dll";dllpath.write_bytes(dllraw);pe=audit.inspect_pe(dllpath);assert pe["status"]=="CP19F2_PE_ABI_PASS"
 expected=json.loads((TASK/"ci/reviewed-imports.json").read_text())["dusklight.exe"]
 assert pe["imports"]["dusklight.exe"]==sorted(expected)
 assert pe["hook_bound_imports"]==["?execute@daE_OC_c@@QEAAHXZ","?modelDraw@daAlink_c@@QEAAXPEAVJ3DModel@@H@Z"]
 with zipfile.ZipFile(io.BytesIO(z.read("SOURCE_COMPILED.zip"))) as src:
  assert src.testzip() is None and set(src.namelist())==set(result["source_ledger"])==set(pin["source_blobs"])
  for name,h in result["source_ledger"].items():
   assert sha(src.read(name))==h and sha((REPO/name).read_bytes())==h and git_blob(src.read(name))==pin["source_blobs"][name]
 tr=json.loads(z.read("TEST_RESULTS.json"))
 assert tr["cp17_link_abi_source_layout"]["status"]=="CP19F2_MSVC_COFF_SOURCE_ABI_PASS" and "cp19f2" in tr["cp17_link_abi_source_layout"]["layout"] and tr["cp17_link_abi_source_layout"]["game_DLL_loaded"] is False and tr["cp17_link_abi_source_layout"]["layout"]["proc_width"]==2
 assert tr["cp15_lab_cases"]==38 and tr["cp16_lab_cases"]==56 and tr["cp14_lab_cases"]==16 and tr["fix01_lab_cases"]==53 and tr["cp17_lab_cases"]==64 and tr["cp17f1_lab_cases"]==53 and tr["cp18_lab_cases"]==43 and tr["cp19_lab_cases"]==65 and tr["cp19f1_lab_cases"]==73 and tr["cp19f2_lab_cases"]==40 and tr["runtime_provenance"]["identical_runtime_files_to_fix01"]==27 and tr["runtime_provenance"]["audio_access_only_in_gated_adapter"] is True and tr["runtime_provenance"]["no_wave_erase"] is True and tr["runtime_provenance"]["single_life_write"] is True and len(tr["runtime_provenance"]["added_game_imports"])==13 and tr["provider_regression"]["shipped"] is False
 assert tr["name_tests"]["cases"]==25 and tr["multi_tests"]["status"]=="CP10_STATIC_LAB_PASS" and tr["runtime_lab_cases"]==66 and tr["black_lab_cases"]==26 and tr["texture_lab_cases"]==20 and tr["variant_lab_cases"]==30 and tr["damage_lab_cases"]==22 and tr["dynamic_lab_cases"]==28 and tr["visual_lab_cases"]==22 and tr["eye_lab_cases"]==9 and tr["strokes_lab_cases"]==36 and tr["perf_lab_cases"]==4 and tr["cp12_lab_cases"]==7 and tr["cp13_runtime_cases"]>=26 and tr["cp13_editor_tests"]["status"]=="CP13_EDITOR_LAB_PASS"
 report=dict(status="DOWNLOAD_VERIFIED",run_id=run,artifact_id=artifact["id"],artifact_name=artifact["name"],expires_at=artifact["expires_at"],source_commit=result["source_commit"],ci_commit=result["ci_commit"],zip=result["zip"],source_ledger_verified=True,bytes_and_sha256_checked_on_downloaded_zip=True,package_audited_pe_abi_hooks_imports=True,cp17_lab_cases=tr["cp17_lab_cases"],cp17f1_lab_cases=tr["cp17f1_lab_cases"],cp18_lab_cases=tr["cp18_lab_cases"],cp19_lab_cases=tr["cp19_lab_cases"],cp19f1_lab_cases=tr["cp19f1_lab_cases"],cp19f2_lab_cases=tr["cp19f2_lab_cases"],cp17_link_abi_source_layout=tr["cp17_link_abi_source_layout"],no_loose_dll_no_game_assets=True,requires_provider=manifest["requires_provider"],expansion=manifest["package"],CRC="PASS",game_executed=False)
out=Path(os.environ["RUNNER_TEMP"])/"cp19f2-download-evidence";out.mkdir();(out/"download-result.json").write_text(json.dumps(report,indent=2)+"\n")
print("CP19F2_DOWNLOAD_RESULT="+json.dumps(report,separators=(",",":")),flush=True)
