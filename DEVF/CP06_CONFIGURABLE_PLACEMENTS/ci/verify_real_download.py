"""Independent fresh Actions download: CRC/hash/source/manifest/internal PE, never load/run DLL."""
import io,json,os,urllib.error,urllib.parse,urllib.request,zipfile
from pathlib import Path
from audit import inspect_pe,safe_members,sha
SPEC=Path(__file__).resolve().parent/"download-spec.json"
ROOT=Path(__file__).resolve().parents[1]
WORK=Path(os.environ["RUNNER_TEMP"])/"cp06-real-download";WORK.mkdir(parents=True,exist_ok=True)
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,req,fp,code,msg,headers,newurl):return None
def download(item):
    # Token goes only to api.github.com. A signed artifact redirect receives no auth header.
    req=urllib.request.Request("https://api.github.com/repos/iscoremusic26-maker/codex-cp00-inputs/actions/artifacts/"+str(item["id"])+"/zip",headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json"})
    opener=urllib.request.build_opener(NoRedirect())
    try:
        response=opener.open(req,timeout=60)
    except urllib.error.HTTPError as e:
        if e.code not in (301,302,303,307,308):raise RuntimeError("Artifact API rejected: HTTP "+str(e.code)) from None
        target=e.headers.get("Location","");parsed=urllib.parse.urlparse(target)
        if parsed.scheme!="https" or not parsed.hostname or not any(parsed.hostname.endswith(s) for s in [".blob.core.windows.net",".githubusercontent.com",".amazonaws.com"]):raise RuntimeError("Unexpected artifact redirect host")
        response=urllib.request.urlopen(target,timeout=60) # no Authorization header
    with response:
        data=response.read(4*1024*1024+1)
    if len(data)!=item["bytes"] or sha(data)!=item["sha256"]:raise RuntimeError("Real artifact size/hash mismatch")
    return data
def normalized(data):return data.replace(b"\r\n",b"\n")
spec=json.loads(SPEC.read_text());binary=download(spec["delivery"]);text=download(spec["text"])
with zipfile.ZipFile(io.BytesIO(text)) as z:
    names=z.namelist()
    if len(names)!=len(set(names)) or z.testzip() or any("/" in n or not n.endswith((".log",".json")) for n in names):raise RuntimeError("Unsafe/CRC-failed evidence ZIP")
    build=json.loads(z.read("result.json"))
    compiler=z.read("compiler.log").decode("utf-8",errors="strict")
if build["status"]!="BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING" or build["source_commit"]!=spec["source_commit"] or build["ci_head"]!=spec["ci_head"] or int(build["run_id"])!=spec["run_id"]:raise RuntimeError("Compiled provenance mismatch")
with zipfile.ZipFile(io.BytesIO(binary)) as z:
    manifest=json.loads(z.read("MANIFEST.json"));name=manifest["filename"]
    members=safe_members(z,{name,"MANIFEST.json","SHA256SUMS.txt","SOURCE_COMPILED.zip"},set())
    dusk=z.read(name);source_zip=z.read("SOURCE_COMPILED.zip");checksum=z.read("SHA256SUMS.txt").decode()
checks={"manifest_equal":manifest==build["delivery_manifest"],"package_sha256":sha(dusk)==manifest["package_sha256"],"package_bytes":len(dusk)==manifest["package_bytes"],"checksum_text":checksum.replace("\r\n","\n")==sha(dusk)+"  "+name+"\n"}
if not all(checks.values()):raise RuntimeError("Delivery checks failed: "+json.dumps(checks,sort_keys=True))
with zipfile.ZipFile(io.BytesIO(source_zip)) as z:
    expected={"DEVF/CP06_CONFIGURABLE_PLACEMENTS/"+n for n in build["source_ledger"]}|{"DEVF/CP06_CONFIGURABLE_PLACEMENTS/SOURCE_CI.json"}
    source_members=safe_members(z,expected,set())
    for rel,digest in build["source_ledger"].items():
        packaged=z.read("DEVF/CP06_CONFIGURABLE_PLACEMENTS/"+rel)
        if sha(normalized(packaged))!=digest or sha(normalized((ROOT/rel).read_bytes()))!=digest:raise RuntimeError("Compiled source ledger mismatch")
    reuse=json.loads(z.read("DEVF/CP06_CONFIGURABLE_PLACEMENTS/immutable-helpers.json"))
    for n,digest in reuse.items():
        if sha(normalized(z.read("DEVF/CP06_CONFIGURABLE_PLACEMENTS/"+n)))!=digest:raise RuntimeError("Approved helper source mismatch")
    provenance=json.loads(z.read("DEVF/CP06_CONFIGURABLE_PLACEMENTS/SOURCE_CI.json"))
    if provenance["source_commit"]!=spec["source_commit"]:raise RuntimeError("Source bundle provenance mismatch")
with zipfile.ZipFile(io.BytesIO(dusk)) as z:
    inner_members=safe_members(z)
    if z.read("mod.json")!=(ROOT/"mod.json").read_bytes():raise RuntimeError("Internal official manifest bytes differ")
    dll=z.read("lib/windows-amd64/mod.dll")
if len(dll)!=manifest["dll_bytes"] or sha(dll)!=manifest["dll_sha256"]:raise RuntimeError("Internal DLL hash/size")
dll_path=WORK/"own-internal-inspection.dll";dll_path.write_bytes(dll)
pe=inspect_pe(dll_path)
if pe!=build["pe"] or pe!=build["package"]["pe_audit"]:raise RuntimeError("Internal PE/ABI/import/hook audit differs")
result={"status":"DOWNLOAD_VERIFIED","source_commit":spec["source_commit"],"build_ci_head":spec["ci_head"],"build_run":spec["run_id"],"verification_run":os.environ["GITHUB_RUN_ID"],"verification_ci_head":os.environ["DEVF_CI_HEAD"],"delivery_artifact":spec["delivery"],"text_artifact":spec["text"],"manifest":manifest,"outer_zip":members,"source_zip_members":source_members,"inner_dusk_members":inner_members,"pe":pe,"all_CRC_and_SHA256_verified":True,"DLL_internal_matches_compiled":True,"published_source_matches_compiled_ledger":True,"compiled_source_bundle_verified":True,"runtime":"HUMAN_RUNTIME_NOT_TESTED","game_or_DLL_executed":False,"host_or_CP03_built":False,"compiler_version_log":compiler.strip()}
print("CP06_REAL_DOWNLOAD_RESULT="+json.dumps(result,sort_keys=True))
