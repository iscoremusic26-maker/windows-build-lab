"""New two-point STATIC query only; reuse unchanged CP01 decoders, no runtime/build."""
import os,sys,json,zipfile,hashlib,subprocess,math
from pathlib import Path
root=Path(__file__).resolve().parents[3]
cache=Path(os.environ["RUNNER_TEMP"])/"devf-cp06-site-review"
cache.mkdir(parents=True,exist_ok=True)
os.environ["DEVF_CP01_LOCATION_WORK"]=str(cache)
sys.path.insert(0,str(root/"DEVF/CP01/analysis"))
from analyze_location import rarc,dzs
arc_sha="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af"
with zipfile.ZipFile(root/"DEVF_CP01_EVIDENCIA_F_SP121.zip") as z:
    members=[n for n in z.namelist() if Path(n).name=="R03_00.arc"]
    assert len(members)==1
    raw=z.read(members[0]) # ZIP reader checks this member CRC, no original output.
assert hashlib.sha256(raw).hexdigest()==arc_sha
_,_,entries=rarc(raw)
def member(suffix):
    matches=[e for e in entries if e["path"].endswith(suffix)]
    assert len(matches)==1
    return matches[0]["raw"]
for suffix in ["kcl/room.kcl","plc/room.plc"]:
    dest=cache/"R03_00.arc"/suffix
    dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(member(suffix))
(cache/"inventory.json").write_text(json.dumps({"archives":{"R03_00.arc":{"files":[{"chunks":dzs(member("dzr/room.dzr"))}]}}}))
prior=json.loads((Path(__file__).parent/"previous-volume-bounds.json").read_text())
results=[]
for name,x,z in [("north_of_site1",-1250.,61950.),("south_of_site1",-1250.,59550.)]:
    output=cache/(name+".json")
    env=dict(os.environ,DEVF_STAGE_LOCATION_CACHE=str(cache),DEVF_STAGE_LOCATION_OUTPUT=str(output),DEVF_QUERY_X=str(x),DEVF_QUERY_Z=str(z))
    p=subprocess.run([sys.executable,str(Path(__file__).parent/"site_query.py")],env=env,capture_output=True,text=True)
    if p.returncode:
        results.append({"id":name,"xz":[x,z],"status":"QUERY_FAILED","returncode":p.returncode})
        continue
    r=json.loads(output.read_text())
    margins=[]
    for v in prior["volumes"]:
        # Same conservative circle bound used previously; author-defined volume only.
        half=v.get("half_xz")
        if half:
            radius=math.hypot(max(half[:2]),max(half[2:]))
        elif v.get("radius") is not None:
            radius=v["radius"]
        else:
            margins.append({"name":v["name"],"tag":v["tag"],"index":v["index"],"status":"BOUND_NOT_AVAILABLE"})
            continue
        distance=math.hypot(x-v["center"][0],z-v["center"][2])
        margins.append({"name":v["name"],"tag":v["tag"],"index":v["index"],"circle_margin_minus501":distance-radius-501})
    r["id"]=name;r["previous_volume_bounds"]=margins
    r["status"]="STATIC_QUERY_ONLY_NOT_SITE_APPROVED"
    r["site2_approved"]=False;r["runtime"]="NOT_TESTED"
    results.append(r)
out={"status":"SITE2_PENDING","room_arc_sha256":arc_sha,"candidates":results,"originals_published":False,"game_executed":False,"build_performed":False,"source_base":"3ab8ff2fdd42128ab75157338699b21ae6d768bd","note":"New ±1200 Z queries from observed site1, not arbitrary whitelist additions. No reread of WRock/Digpl/render; old unsafe point excluded."}
report=root/"site-review-result.json";report.write_text(json.dumps(out,indent=2)+"\n")
print("CP06_SITE_REVIEW="+json.dumps(out,separators=(",",":")))
