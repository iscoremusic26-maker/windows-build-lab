"""New north context only: paths/discrete Digpl points/entries; reuse existing readers."""
import os,sys,json,zipfile,hashlib,math
from pathlib import Path
REPO=Path(__file__).resolve().parents[3]
os.environ["DEVF_CP01_LOCATION_WORK"]=str(Path(os.environ["RUNNER_TEMP"])/"native-north-context")
sys.path.insert(0,str(REPO/"DEVF/CP01/analysis"))
from analyze_location import rarc,dzs
from location_details import paths,point_distance_segment
def run():
 with zipfile.ZipFile(REPO/"DEVF_CP01_EVIDENCIA_F_SP121.zip") as z:
  names=[n for n in z.namelist() if Path(n).name=="R03_00.arc"];assert len(names)==1
  raw=z.read(names[0])
 assert hashlib.sha256(raw).hexdigest()=="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af"
 _,_,files=rarc(raw);room=next(f["raw"] for f in files if f["path"].endswith("dzr/room.dzr"))
 chunks=dzs(room);routes=paths(room);p=(-1250.,-6327.600450915229,61950.)
 active=[dict(r,chunk=c["tag"]) for c in chunks if c["tag"] in ["ACTR","ACT6","SCOB","SCO6","PLYR"] for r in c["records"]]
 distance=lambda a:math.hypot(a[0]-p[0],a[2]-p[2])
 records=sorted([{"chunk":r["chunk"],"index":r["index"],"name":r["name"],"distance_xz":distance(r["position"])} for r in active],key=lambda r:r["distance_xz"])
 near_routes=[]
 for r in routes:
  segs=list(zip(r["points"],r["points"][1:]))+([(r["points"][-1],r["points"][0])] if r["closed"] and r["points"] else [])
  near_routes.append({"index":r["index"],"count":r["count"],"closed":r["closed"],"polyline_distance_xz":min([point_distance_segment(p,a,b) for a,b in segs]+[distance(q) for q in r["points"]],default=None)})
 consumer=next(r for r in active if r["chunk"]=="ACTR" and r["index"]==5 and r["name"]=="Digpl")
 assert int(consumer["parameters"],16)==0xf07ff5ff
 route=routes[7];assert route["count"]==27
 dig=sorted([{"index":i,"distance_xz":distance(q),"delta_y":q[1]-p[1],"distance_to_square501_xz":math.hypot(max(abs(q[0]-p[0])-501,0),max(abs(q[2]-p[2])-501,0))} for i,q in enumerate(route["points"])],key=lambda r:r["distance_xz"])
 entries=[{"index":r["index"],"distance_xz":distance(r["position"]),"angle_z":r["angle"][2]} for r in active if r["chunk"]=="PLYR"]
 result={"status":"NORTH_CONTEXT_SOURCE_SCREEN","candidate_ground":p,"nearest_records":records[:8],"native_Eoc_homes":[r for r in records if r["name"]=="E_oc"],"nearest_routes":sorted(near_routes,key=lambda r:r["polyline_distance_xz"] or 0)[:4],"digpl":{"semantics":"27 discrete wolf-player digging points; not enemy paths","nearest_points":dig[:3],"look_radius_player_only":500},"entries":entries,"room_arc_sha256":hashlib.sha256(raw).hexdigest(),"no_geometry_redecode":True,"human_terrain_visibility_collision":"NOT_TESTED"}
 print("NORTH_CONTEXT_RESULT="+json.dumps(result,separators=(",",":")));return result
