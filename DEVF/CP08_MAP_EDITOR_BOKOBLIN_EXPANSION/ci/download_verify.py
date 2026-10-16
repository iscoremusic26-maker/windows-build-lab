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
artifact=next(a for a in arts if a["name"]=="DEVF_CP08_MAP_EDITOR_TEST_"+run)
evidence=next(a for a in arts if a["name"]=="DEVF_CP08_TEXT_"+run)
with zipfile.ZipFile(io.BytesIO(download(evidence))) as z:
 assert z.testzip() is None
 result=json.loads(z.read("result.json"))
with zipfile.ZipFile(io.BytesIO(download(artifact))) as outer:
 assert outer.testzip() is None and outer.namelist()==["DEVF_CP08_MAP_EDITOR_TEST.zip"];bundle=outer.read(outer.namelist()[0])
assert len(bundle)==result["zip"]["bytes"] and sha(bundle)==result["zip"]["sha256"]
work=Path(os.environ["RUNNER_TEMP"])/"cp08-independent";work.mkdir()
with zipfile.ZipFile(io.BytesIO(bundle)) as z:
 assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
 assert all(not n.startswith(("/","\\")) and ".." not in Path(n).parts and "\\" not in n for n in z.namelist())
 manifest=json.loads(z.read("MANIFEST.json"))
 assert manifest["source_commit"]==result["source_commit"]==json.loads((TASK/"SOURCE_CI.json").read_text())["source_commit"]
 for name,m in manifest["files"].items():
  raw=z.read(name);assert len(raw)==m["bytes"] and sha(raw)==m["sha256"]
 assert set(z.namelist())==set(manifest["files"])|{"MANIFEST.json","SHA256SUMS.txt"}
 assert z.read("SHA256SUMS.txt").decode()=="".join(m["sha256"]+"  "+n+"\n" for n,m in manifest["files"].items())
 for mode,m in manifest["packages"].items():
  data=z.read(m["filename"]);assert len(data)==m["bytes"] and sha(data)==m["sha256"]
  pkg=work/(mode+".dusk");pkg.write_bytes(data)
  with zipfile.ZipFile(io.BytesIO(data)) as p:
   assert p.testzip() is None;raw=p.read("lib/windows-amd64/mod.dll")
  dll=work/(mode+".dll");dll.write_bytes(raw);checked=verify_package(pkg,dll,mode)
  assert checked["dll_sha256"]==m["dll_sha256"] and checked["dll_bytes"]==m["dll_bytes"]
 app=work/"DEVF Map Editor.exe";app.write_bytes(z.read(app.name));assert app_pe(app)==manifest["app"]
 with zipfile.ZipFile(io.BytesIO(z.read("SOURCE_COMPILED.zip"))) as src:
  assert src.testzip() is None and set(src.namelist())==set(result["source_ledger"])
  for name,h in result["source_ledger"].items():
   assert sha(src.read(name))==h and sha((REPO/name).read_bytes())==h
 report=dict(status="DOWNLOAD_VERIFIED",run_id=run,artifact_id=artifact["id"],artifact_name=artifact["name"],expires_at=artifact["expires_at"],source_commit=result["source_commit"],ci_commit=result["ci_commit"],zip=result["zip"],packages=manifest["packages"],app=manifest["app"],source_ledger_verified=True,CRC="PASS",game_executed=False)
out=Path(os.environ["RUNNER_TEMP"])/"cp08-download-evidence";out.mkdir();(out/"download-result.json").write_text(json.dumps(report,indent=2)+"\n")
print("CP08_DOWNLOAD_RESULT="+json.dumps(report,separators=(",",":")),flush=True)
