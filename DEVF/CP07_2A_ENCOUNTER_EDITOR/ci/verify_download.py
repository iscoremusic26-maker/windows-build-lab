"""Independent real download and all nested integrity checks. No build/DLL/game execution."""
import io,json,os,urllib.error,urllib.parse,urllib.request,zipfile
from pathlib import Path
from audit_api import inspect_pe,safe_members,sha
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1];WORK=Path(os.environ["RUNNER_TEMP"])/"native-api-download";WORK.mkdir(parents=True,exist_ok=True)
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):return None
def download(item):
 req=urllib.request.Request("https://api.github.com/repos/iscoremusic26-maker/codex-cp00-inputs/actions/artifacts/"+str(item["id"])+"/zip",headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json"})
 try:response=urllib.request.build_opener(NoRedirect()).open(req,timeout=60)
 except urllib.error.HTTPError as e:
  if e.code not in [301,302,303,307,308]:raise RuntimeError("Artifact HTTP "+str(e.code)) from None
  url=e.headers.get("Location","");p=urllib.parse.urlparse(url)
  if p.scheme!="https" or not p.hostname or not any(p.hostname.endswith(s) for s in [".blob.core.windows.net",".githubusercontent.com",".amazonaws.com"]):raise RuntimeError("Unexpected redirect")
  response=urllib.request.urlopen(url,timeout=60)
 with response:raw=response.read(4*1024*1024+1)
 if len(raw)!=item["bytes"] or sha(raw)!=item["sha256"]:raise RuntimeError("Artifact bytes/SHA")
 return raw
spec=json.loads((ROOT/"ci/download-spec.json").read_text());text=download(spec["text"])
with zipfile.ZipFile(io.BytesIO(text)) as z:
 if z.testzip() or len(z.namelist())!=len(set(z.namelist())) or any("/" in n or not n.endswith((".log",".json")) for n in z.namelist()):raise RuntimeError("Evidence ZIP")
 build=json.loads(z.read("result.json"))
if build["status"]!="DEVF_API_PAIR_BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING" or build["source_commit"]!=spec["source_commit"] or build["ci_head"]!=spec["ci_commit"] or str(build["run_id"])!=str(spec["run_id"]):raise RuntimeError("Build provenance")
result={"status":"DEVF_API_PAIR_DOWNLOAD_VERIFIED","compiled_source":spec["source_commit"],"build_ci":spec["ci_commit"],"build_run":spec["run_id"],"verification_run":os.environ["GITHUB_RUN_ID"],"verification_ci":os.environ["DEVF_VERIFY_SHA"],"game_executed":False,"DLL_executed":False,"new_build":False,"packages":{}}
for mode in ["provider","probe"]:
 raw=download(spec[mode]);package=build["packages"][mode]
 with zipfile.ZipFile(io.BytesIO(raw)) as z:
  manifest=json.loads(z.read("MANIFEST.json"));name=manifest["filename"]
  expected={name,"MANIFEST.json","SHA256SUMS.txt","SOURCE_COMPILED.zip","README.md","HUMAN_TEST.md"}|set(package["examples"])
  outer=safe_members(z,expected,{"examples/"})
  dusk=z.read(name);source_zip=z.read("SOURCE_COMPILED.zip")
  if manifest!=package["manifest"] or len(dusk)!=manifest["package_bytes"] or sha(dusk)!=manifest["package_sha256"]:raise RuntimeError("Manifest/dusk mismatch")
  checks={name:sha(dusk),**{p:e["sha256"] for p,e in package["examples"].items()}}
  if z.read("SHA256SUMS.txt").decode().replace("\r\n","\n")!="".join(h+"  "+p+"\n" for p,h in checks.items()):raise RuntimeError("Checksum text")
  for p,e in package["examples"].items():
   b=z.read(p)
   if len(b)!=e["bytes"] or sha(b)!=e["sha256"] or b!=(ROOT/p).read_bytes():raise RuntimeError("Example bytes")
  for p in ["README.md","HUMAN_TEST.md"]:
   if z.read(p)!=(ROOT/p).read_bytes():raise RuntimeError("Packaged instructions")
 with zipfile.ZipFile(io.BytesIO(source_zip)) as z:
  expected=set(build["source_ledger"])|{"DEVF/NATIVE_API_PILOT/SOURCE_CI.json"};members=safe_members(z,expected,set())
  for rel,digest in build["source_ledger"].items():
   if sha(z.read(rel).replace(b"\r\n",b"\n"))!=digest or sha((REPO/rel).read_bytes().replace(b"\r\n",b"\n"))!=digest:raise RuntimeError("Compiled source mismatch "+rel)
  if json.loads(z.read("DEVF/NATIVE_API_PILOT/SOURCE_CI.json"))["source_commit"]!=spec["source_commit"]:raise RuntimeError("Source SHA")
 with zipfile.ZipFile(io.BytesIO(dusk)) as z:
  inside=safe_members(z);mod_json=z.read("mod.json");dll=z.read("lib/windows-amd64/mod.dll")
  origin=ROOT/("provider" if mode=="provider" else "consumer")/"mod.json"
  if mod_json!=origin.read_bytes() or sha(mod_json)!=manifest["manifest_sha256"]:raise RuntimeError("Internal manifest")
 if len(dll)!=manifest["dll_bytes"] or sha(dll)!=manifest["dll_sha256"]:raise RuntimeError("Internal DLL")
 p=WORK/(mode+"-audit-only.dll");p.write_bytes(dll);pe=inspect_pe(p,mode)
 if pe!=package["audit"]["pe"]:raise RuntimeError("Independent PE mismatch")
 result["packages"][mode]={"artifact":spec[mode],"manifest":manifest,"outer_members":outer,"source_zip_bytes":len(source_zip),"source_zip_sha256":sha(source_zip),"source_ledger_verified":True,"source_members":len(members),"inner_members":inside,"pe":pe,"examples_verified":list(package["examples"]),"all_SHA_CRC_verified":True}
print("DEVF_API_REAL_DOWNLOAD_RESULT="+json.dumps(result,separators=(",",":")),flush=True)
