"""Real independent artifact download on a fresh runner; no build/DLL/game execution."""
import io,json,os,struct,urllib.error,urllib.parse,urllib.request,zipfile
from pathlib import Path
from audit_native import inspect_pe,safe_members,sha
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1];WORK=Path(os.environ["RUNNER_TEMP"])/"native-qa-actual-download";WORK.mkdir(parents=True,exist_ok=True)
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):return None
def download(item):
 req=urllib.request.Request("https://api.github.com/repos/iscoremusic26-maker/codex-cp00-inputs/actions/artifacts/"+str(item["id"])+"/zip",headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json"})
 try:response=urllib.request.build_opener(NoRedirect()).open(req,timeout=60)
 except urllib.error.HTTPError as e:
  if e.code not in [301,302,303,307,308]:raise RuntimeError("Artifact HTTP "+str(e.code)) from None
  url=e.headers.get("Location","");p=urllib.parse.urlparse(url)
  if p.scheme!="https" or not p.hostname or not any(p.hostname.endswith(s) for s in [".blob.core.windows.net",".githubusercontent.com",".amazonaws.com"]):raise RuntimeError("Unexpected redirect")
  response=urllib.request.urlopen(url,timeout=60) # intentionally no authorization on redirect
 with response:raw=response.read(4*1024*1024+1)
 if len(raw)!=item["bytes"] or sha(raw)!=item["sha256"]:raise RuntimeError("Artifact byte/SHA mismatch")
 return raw
spec=json.loads((ROOT/"ci/download-spec.json").read_text());text=download(spec["text"])
with zipfile.ZipFile(io.BytesIO(text)) as z:
 if z.testzip() or len(z.namelist())!=len(set(z.namelist())) or any("/" in n or not n.endswith((".log",".json")) for n in z.namelist()):raise RuntimeError("Evidence ZIP")
 build=json.loads(z.read("result.json"))
if build["status"]!="A_B_BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING" or build["source_commit"]!=spec["source_commit"] or build["ci_head"]!=spec["ci_commit"] or str(build["run_id"])!=str(spec["run_id"]):raise RuntimeError("Build provenance mismatch")
result={"status":"A_B_DOWNLOAD_VERIFIED","compiled_source":spec["source_commit"],"build_ci":spec["ci_commit"],"build_run":spec["run_id"],"verification_run":os.environ["GITHUB_RUN_ID"],"verification_ci":os.environ["DEVF_VERIFY_SHA"],"game_executed":False,"DLL_executed":False,"new_build":False,"packages":{}}
for key,mode in [("A","north"),("B","dual")]:
 raw=download(spec[key])
 with zipfile.ZipFile(io.BytesIO(raw)) as z:
  manifest=json.loads(z.read("MANIFEST.json"));name=manifest["filename"]
  outer=safe_members(z,{name,"MANIFEST.json","SHA256SUMS.txt","SOURCE_COMPILED.zip"},set())
  dusk=z.read(name);source_zip=z.read("SOURCE_COMPILED.zip")
  if manifest!=build["packages"][key]["manifest"] or sha(dusk)!=manifest["package_sha256"] or len(dusk)!=manifest["package_bytes"]:raise RuntimeError("Delivery manifest/dusk mismatch")
  if z.read("SHA256SUMS.txt").decode().replace("\r\n","\n")!=sha(dusk)+"  "+name+"\n":raise RuntimeError("Checksum text")
 with zipfile.ZipFile(io.BytesIO(source_zip)) as z:
  expected=set(build["source_ledger"])|{"DEVF/CP06_NATIVE_SPAWN_QA/SOURCE_CI.json"}
  source_members=safe_members(z,expected,set())
  for rel,digest in build["source_ledger"].items():
   if sha(z.read(rel).replace(b"\r\n",b"\n"))!=digest or sha((REPO/rel).read_bytes().replace(b"\r\n",b"\n"))!=digest:raise RuntimeError("Compiled source mismatch "+rel)
  if json.loads(z.read("DEVF/CP06_NATIVE_SPAWN_QA/SOURCE_CI.json"))["source_commit"]!=spec["source_commit"]:raise RuntimeError("Source SHA mismatch")
 with zipfile.ZipFile(io.BytesIO(dusk)) as z:
  inside=safe_members(z)
  if z.read("mod.json")!=(ROOT/"manifests"/(mode+".json")).read_bytes():raise RuntimeError("Internal manifest")
  dll=z.read("lib/windows-amd64/mod.dll")
 if sha(dll)!=manifest["dll_sha256"] or len(dll)!=manifest["dll_bytes"]:raise RuntimeError("Internal DLL bytes/hash")
 p=WORK/(key+"-audit-only.dll");p.write_bytes(dll);pe=inspect_pe(p)
 if pe!=build["packages"][key]["audit"]["pe"]:raise RuntimeError("Independent PE differs")
 result["packages"][key]={"artifact":spec[key],"manifest":manifest,"outer_members":outer,"source_zip_bytes":len(source_zip),"source_zip_sha256":sha(source_zip),"source_ledger_verified":True,"source_members":len(source_members),"inner_members":inside,"pe":pe,"all_SHA_CRC_verified":True,"status":"NORTH_NATIVE_QA_BUILD_READY" if key=="A" else "DUAL_NATIVE_QA_BUILT_HUMAN_TEST_BLOCKED_UNTIL_NORTH_PASS"}
print("NATIVE_QA_REAL_DOWNLOAD_RESULT="+json.dumps(result,separators=(",",":")),flush=True)
