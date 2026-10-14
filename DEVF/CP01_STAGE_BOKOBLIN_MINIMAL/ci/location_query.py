import os,sys,importlib.util,json,math
from pathlib import Path
base=Path(__file__).resolve().parents[2];cache=Path(os.environ['DEVF_STAGE_LOCATION_CACHE']);os.environ['DEVF_CP01_LOCATION_WORK']=str(cache);sys.path.insert(0,str(base/'CP01/analysis'))
spec=importlib.util.spec_from_file_location('query',base/'CP01_G4_EOC_RESOURCE_CONTINUATION/analysis/reassess_position.py');q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
from analyze_location import u
from location_geometry import dot,cross,add,mul,hit,plc_dec,area
from verify_footprint import constraints,halfclip,subtract
kcl=(cache/'R03_00.arc/kcl/room.kcl').read_bytes();plc=(cache/'R03_00.arc/plc/room.plc').read_bytes()
x,z=-1250.,60750.;rough_y=-6201.49169921875
ids,counts=q.query_cells(kcl,[x-550,rough_y-2000,z-550],[x+550,rough_y+2000,z+550],u);tris=q.selected_prisms(kcl,plc,ids,u,dot,cross,add,mul);supports=sorted([(h,t) for t in tris if (h:=hit(t,x,z)) is not None and h<rough_y+1500],key=lambda v:v[0],reverse=True)
print('support',[(h,t['index'],plc_dec(t['code'])) for h,t in supports[:4]],counts)
assert supports;y=supports[0][0];center=[x,y,z];results={}
for radius in [100,501]:
 poly=[(x-radius,z-radius),(x+radius,z-radius),(x+radius,z+radius),(x-radius,z+radius)];remainder=[poly];heights=[];contrib=[];codes=[]
 for t in tris:
  if t['normal'][1]<.5 or t['code'][0]&(1<<14) or t['code'][4]&(1<<8):continue
  clipped=poly
  for a,b,c in constraints(t):clipped=halfclip(clipped,a,b,c)
  if area(clipped)<1e-7:continue
  n,p=t['normal'],t['pts'][0];ys=[p[1]-((xx-p[0])*n[0]+(zz-p[2])*n[2])/n[1] for xx,zz in clipped]
  if max(ys)>y+501 or min(ys)<y-501:continue
  heights+=ys;contrib.append(t['index']);codes.append(plc_dec(t['code']))
  nextparts=[]
  for piece in remainder:nextparts+=subtract(piece,constraints(t))
  remainder=nextparts
 results[str(radius)]={'missing_area':sum(area(part) for part in remainder),'full_area':4*radius**2,'heights':[min(heights),max(heights)] if heights else [],'prisms':contrib,'PLC':codes}
dist=[{'index':t['index'],'distance':q.point_triangle_distance(center,t['pts']),'normal':t['normal']} for t in tris if t['normal'][1]<.5];dist.sort(key=lambda t:t['distance'])
inv=json.loads((cache/'inventory.json').read_text());actors=[]
for f in inv['archives']['R03_00.arc']['files']:
 for chunk in f.get('chunks',[]):
  if chunk['tag'] in ['ACTR','ACT6','SCOB','SCO6']:
   for r in chunk['records']:actors.append(dict(tag=chunk['tag'],index=r['index'],name=r['name'],position=r['position'],xz=math.hypot(x-r['position'][0],z-r['position'][2])))
actors.sort(key=lambda r:r['xz']);report={'candidate':center,'query':counts,'support_prism':supports[0][1]['index'],'normal':supports[0][1]['normal'],'PLC':plc_dec(supports[0][1]['code']),'local_coverage':results,'nearest_static_walls':dist[:4],'nearest_authored_room_records':actors[:14],'native_Eoc_separation':[a for a in actors if a['name']=='E_oc'],'runtime':'NOT_TESTED'}
Path(os.environ['DEVF_STAGE_LOCATION_OUTPUT']).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['native_Eoc_separation','local_coverage']},indent=2));print({k:{x:y for x,y in v.items() if x!='PLC'} for k,v in results.items()})
