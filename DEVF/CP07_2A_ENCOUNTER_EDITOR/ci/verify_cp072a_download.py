"""Independent real download and all nested integrity checks. No build/DLL/game execution."""
import io,json,os,urllib.error,urllib.parse,urllib.request,zipfile
from pathlib import Path
from audit_api import inspect_pe,safe_members,sha
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1];WORK=Path(os.environ["RUNNER_TEMP"])/"cp072a-download";WORK.mkdir(parents=True,exist_ok=True)
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
spec=json.loads((ROOT/"ci/encounter-download-spec.json").read_text());text=download(spec["text"])
with zipfile.ZipFile(io.BytesIO(text)) as z:
 if z.testzip() or len(z.namelist())!=len(set(z.namelist())) or any("/" in n or not n.endswith((".log",".json")) for n in z.namelist()):raise RuntimeError("Evidence ZIP")
 build=json.loads(z.read("result.json"))
if build["status"]!="DEVF_CP072A_BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING" or build["source_commit"]!=spec["source_commit"] or build["ci_head"]!=spec["ci_commit"] or str(build["run_id"])!=str(spec["run_id"]):raise RuntimeError("Build provenance")
result={"status":"DEVF_CP072A_DOWNLOAD_VERIFIED","compiled_source":spec["source_commit"],"build_ci":spec["ci_commit"],"build_run":spec["run_id"],"verification_run":os.environ["GITHUB_RUN_ID"],"verification_ci":os.environ["DEVF_VERIFY_SHA"],"game_executed":False,"DLL_executed":False,"new_build":False,"packages":{}}
for mode in ["provider"]:
 raw=download(spec[mode]);package=build["packages"][mode]
 with zipfile.ZipFile(io.BytesIO(raw)) as z:
  manifest=json.loads(z.read("MANIFEST.json"));name=manifest["filename"]
  expected={name,"MANIFEST.json","SHA256SUMS.txt","SOURCE_COMPILED.zip","README.md","HUMAN_TEST.md"}|set(package["examples"])
  outer=safe_members(z,expected,{"examples/","tools/"})
  dusk=z.read(name);source_zip=z.read("SOURCE_COMPILED.zip")
  if manifest!=package["manifest"] or len(dusk)!=manifest["package_bytes"] or sha(dusk)!=manifest["package_sha256"]:raise RuntimeError("Manifest/dusk mismatch")
  checks={name:sha(dusk),**{p:e["sha256"] for p,e in package["examples"].items()}}
  if z.read("SHA256SUMS.txt").decode().replace("\r\n","\n")!="".join(h+"  "+p+"\n" for p,h in checks.items()):raise RuntimeError("Checksum text")
  for p,e in package["examples"].items():
   b=z.read(p)
   if len(b)!=e["bytes"] or sha(b)!=e["sha256"]:raise RuntimeError("Example/tool bytes")
   if p.startswith("examples/") and b!=(ROOT/p).read_bytes():raise RuntimeError("Example source bytes")
   if p=="tools/encounter_validate.exe":
    import struct
    pe_at=struct.unpack_from("<I",b,0x3c)[0]
    if b[:2]!=b"MZ" or struct.unpack_from("<H",b,pe_at+4)[0]!=0x8664:raise RuntimeError("Validator architecture")
  for p in ["README.md","HUMAN_TEST.md"]:
   if z.read(p)!=(ROOT/p).read_bytes():raise RuntimeError("Packaged instructions")
 with zipfile.ZipFile(io.BytesIO(source_zip)) as z:
  expected=set(build["source_ledger"])|{"DEVF/CP07_2A_ENCOUNTER_EDITOR/SOURCE_CI.json"};members=safe_members(z,expected,set())
  for rel,digest in build["source_ledger"].items():
   if sha(z.read(rel).replace(b"\r\n",b"\n"))!=digest or sha((REPO/rel).read_bytes().replace(b"\r\n",b"\n"))!=digest:raise RuntimeError("Compiled source mismatch "+rel)
  if json.loads(z.read("DEVF/CP07_2A_ENCOUNTER_EDITOR/SOURCE_CI.json"))["source_commit"]!=spec["source_commit"]:raise RuntimeError("Source SHA")
 with zipfile.ZipFile(io.BytesIO(dusk)) as z:
  inside=safe_members(z);mod_json=z.read("mod.json");dll=z.read("lib/windows-amd64/mod.dll")
  origin=ROOT/("provider" if mode=="provider" else "consumer")/"mod.json"
  if mod_json!=origin.read_bytes() or sha(mod_json)!=manifest["manifest_sha256"]:raise RuntimeError("Internal manifest")
 if len(dll)!=manifest["dll_bytes"] or sha(dll)!=manifest["dll_sha256"]:raise RuntimeError("Internal DLL")
 p=WORK/(mode+"-audit-only.dll");p.write_bytes(dll);pe=inspect_pe(p,mode)
 if pe!=package["audit"]["pe"]:raise RuntimeError("Independent PE mismatch")
 result["packages"][mode]={"artifact":spec[mode],"manifest":manifest,"outer_members":outer,"source_zip_bytes":len(source_zip),"source_zip_sha256":sha(source_zip),"source_ledger_verified":True,"source_members":len(members),"inner_members":inside,"pe":pe,"examples_verified":list(package["examples"]),"all_SHA_CRC_verified":True}
old=download(spec["old_probe"])
with zipfile.ZipFile(io.BytesIO(old)) as z:
 if z.testzip() or len(z.namelist())!=len(set(z.namelist())):raise RuntimeError("Historical probe CRC")
 m=json.loads(z.read("MANIFEST.json"));dusk=z.read(m["filename"]);src=z.read("SOURCE_COMPILED.zip")
 if m["source_commit"]!="ae4fa410e4543c032fccfe761b2a817ae17479b2" or m["id"]!="dev.devf.native_api_probe" or m["version"]!="0.1.0" or len(dusk)!=7439 or sha(dusk)!="6556ffe9253f7c0a0e4c678e8293f85a6c4430295895dabf18e518fcc6e25a15":raise RuntimeError("Historical probe identity")
 if len(src)!=57122 or sha(src)!="ebda436b20ee208e7c075043622e3bc883a80bca0cd5e454936bcb8af9005839":raise RuntimeError("Historical source ZIP")
with zipfile.ZipFile(io.BytesIO(src)) as z:
 if z.testzip():raise RuntimeError("Historical source CRC")
 for path in ["shared/devf_native_api.h","consumer/src/probe.hpp"]:
  if z.read("DEVF/NATIVE_API_PILOT/"+path)!=(ROOT/path).read_bytes():raise RuntimeError("ABI/client source drift")
with zipfile.ZipFile(io.BytesIO(dusk)) as z:
 safe_members(z);raw=z.read("lib/windows-amd64/mod.dll")
 if len(raw)!=14336 or sha(raw)!="b407aec64a4d4d79ae4ea54b94a91e0d8b3f904da724e82402b7787812e1c4ec":raise RuntimeError("Historical DLL")
p=WORK/"historical-probe-not-executed.dll";p.write_bytes(raw);old_pe=inspect_pe(p,"probe")
result["old_probe"]={"artifact":spec["old_probe"],"source":"ae4fa410e4543c032fccfe761b2a817ae17479b2","version":"0.1.0","recompiled":False,"republished":False,"DLL_executed":False,"ABI_header_and_client_source_unchanged":True,"PE":old_pe,"status":"HISTORICAL_CLIENT_DOWNLOAD_ABI_VERIFIED_NOT_GAME_EXECUTED"}
print("DEVF_CP072A_REAL_DOWNLOAD_RESULT="+json.dumps(result,separators=(",",":")),flush=True)
