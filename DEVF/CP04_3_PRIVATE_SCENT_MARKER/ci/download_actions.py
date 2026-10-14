"""Independent ZIP download on a fresh Windows runner; never load DLL/game."""
import json,os,urllib.request,urllib.error,hashlib
from pathlib import Path
from verify_download import verify
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,*args,**kwargs):return None
def api(path):
 req=urllib.request.Request("https://api.github.com/repos/"+os.environ["GITHUB_REPOSITORY"]+path,headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json","X-GitHub-Api-Version":"2022-11-28"})
 with urllib.request.urlopen(req,timeout=60) as r:return json.load(r)
def download(item,path):
 req=urllib.request.Request("https://api.github.com/repos/"+os.environ["GITHUB_REPOSITORY"]+"/actions/artifacts/"+str(item["id"])+"/zip",headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json"})
 try:urllib.request.build_opener(NoRedirect()).open(req,timeout=60)
 except urllib.error.HTTPError as e:
  if e.code!=302:raise RuntimeError("Artifact redirect unavailable") from None
  target=e.headers.get("Location")
 else:raise RuntimeError("Expected signed artifact redirect")
 if not target or not target.startswith("https://"):raise RuntimeError("Artifact redirect invalid")
 # Separate request carries NO bearer token; signed URI is never printed/stored in reports.
 with urllib.request.urlopen(target,timeout=60) as r:path.write_bytes(r.read())
 digest=hashlib.sha256(path.read_bytes()).hexdigest()
 if item.get("digest")!="sha256:"+digest:raise RuntimeError("GitHub digest mismatch")
 return digest
run=os.environ["GITHUB_RUN_ID"];project=Path(__file__).resolve().parents[1];work=Path(os.environ["RUNNER_TEMP"])/"cp043-independent-download";work.mkdir()
art=api("/actions/runs/"+run+"/artifacts?per_page=100")["artifacts"]
def one(kind):
 name="DEVF_CP04_3_PRIVATE_SCENT_MARKER_"+kind+"_"+run+"_"+os.environ["GITHUB_RUN_ATTEMPT"]
 matches=[x for x in art if x["name"]==name and not x["expired"]]
 if len(matches)!=1:raise RuntimeError("Expected one current artifact "+kind)
 return matches[0]
delivery,text=one("TEST"),one("TEXT")
dp,tp=work/"TEST.zip",work/"TEXT.zip";dh=download(delivery,dp);th=download(text,tp)
result=verify(dp,tp,project,work/"inspection",dh,th)
result["independent_job"]=True
result["artifact_ids"]={"TEST":delivery["id"],"TEXT":text["id"]}
result["expiry_utc"]={"TEST":delivery["expires_at"],"TEXT":text["expires_at"]}
out=work/"verification";out.mkdir()
(out/"download-verification.json").write_text(json.dumps(result,indent=2)+"\n")
# Text-only independent proof; no original binaries, assets, credentials or signed URLs.
print(json.dumps(result))
