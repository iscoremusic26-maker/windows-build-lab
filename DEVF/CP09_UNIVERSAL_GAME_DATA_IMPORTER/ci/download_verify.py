import io,json,os,urllib.request,urllib.error,zipfile,hashlib,struct
from pathlib import Path
from audit import *
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
artifact=next(a for a in arts if a["name"]=="DEVF_CP09_IMPORTER_TEST_"+run)
evidence=next(a for a in arts if a["name"]=="DEVF_CP09_TEXT_"+run)
with safezip(download(evidence)) as z:result=json.loads(z.read("result.json"))
with safezip(download(artifact)) as outer:
 assert outer.namelist()==["DEVF_CP09_IMPORTER_TEST.zip"];bundle=outer.read(outer.namelist()[0])
assert len(bundle)==result["zip"]["bytes"] and sha(bundle)==result["zip"]["sha256"]
with safezip(bundle) as z:
 manifest=json.loads(z.read("MANIFEST.json"))
 pin=json.loads((TASK/"SOURCE_CI.json").read_text())
 assert manifest["source_commit"]==result["source_commit"]==pin["source_commit"]
 assert manifest["ci_commit"]==result["ci_commit"]==os.environ["EXPECTED_CI_SHA"]
 for name,m in manifest["files"].items():
  raw=z.read(name);assert len(raw)==m["bytes"] and sha(raw)==m["sha256"]
 assert set(z.namelist())==set(manifest["files"])|{"MANIFEST.json","SHA256SUMS.txt"}
 assert z.read("SHA256SUMS.txt").decode().splitlines()==[m["sha256"]+"  "+n for n,m in manifest["files"].items()]
 assert pe(z.read("DEVF Map Editor CP09.exe"))==manifest["app"]
 assert not any(n.lower().endswith((".arc",".kcl",".plc",".ciso",".iso",".dusk",".dll")) for n in z.namelist())
 with safezip(z.read("SOURCE_COMPILED.zip")) as src:
  assert set(src.namelist())==set(result["source_ledger"])==set(pin["source_blobs"])
  for name,h in result["source_ledger"].items():
   raw=src.read(name);assert sha(raw)==h==sha((REPO/name).read_bytes())
   assert hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()==pin["source_blobs"][name]
 report=dict(status="DOWNLOAD_VERIFIED",run_id=run,artifact_id=artifact["id"],artifact_name=artifact["name"],expires_at=artifact["expires_at"],source_commit=result["source_commit"],ci_commit=result["ci_commit"],zip=result["zip"],app=manifest["app"],source_ledger_verified=True,CRC="PASS",runtime_mods_changed=False,game_executed=False)
out=Path(os.environ["RUNNER_TEMP"])/"cp09-download-evidence";out.mkdir();(out/"download-result.json").write_text(json.dumps(report,indent=2)+"\n")
print("CP09_DOWNLOAD_RESULT="+json.dumps(report,separators=(",",":")),flush=True)
