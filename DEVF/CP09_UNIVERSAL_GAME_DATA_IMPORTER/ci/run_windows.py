import json,os,subprocess,zipfile,shutil,sys
from pathlib import Path
from audit import *
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp09"
for name in ["evidence","app","tests","deliver","artifact"]:(WORK/name).mkdir(parents=True,exist_ok=True)
EVIDENCE=WORK/"evidence";DIST=WORK/"deliver"
pin=json.loads((TASK/"SOURCE_CI.json").read_text());result=dict(status="FAILED",source_commit=pin["source_commit"],ci_commit=subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),game_executed=False)
def run(label,args):
 p=subprocess.run([str(a) for a in args],text=True,capture_output=True,encoding="utf-8",errors="replace");txt=p.stdout+p.stderr
 for val in [str(REPO),str(WORK),os.environ.get("USERPROFILE","")]:
  if val:txt=txt.replace(val,"<WORK>").replace(val.replace("\\","/"),"<WORK>")
 (EVIDENCE/(label+".txt")).write_text(txt,encoding="utf-8")
 print(label,p.returncode,flush=True)
 if p.returncode:
  fail=WORK/"tests/FAIL.txt"
  if fail.exists():txt+="\n"+fail.read_text()
  print(txt[-14000:],flush=True);raise RuntimeError(label)
 return txt
try:
 assert os.name=="nt"
 ledger={}
 for name,h in pin["source_blobs"].items():
  raw=(REPO/name).read_bytes();assert hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()==h,name
  ledger[name]=sha(raw)
 result["dotnet_sdk"]=run("sdk",["dotnet","--version"]).strip()
 assert result["dotnet_sdk"].split(".")[0] in ["8","9","10"] # target framework and runtime fixed net8 by project
 run("publish",["dotnet","publish",TASK/"app/DEVF.MapEditor.csproj","-c","Release","-r","win-x64","--self-contained","true","-p:PublishSingleFile=true","-p:IncludeNativeLibrariesForSelfExtract=true","-p:DebugType=None","-p:DebugSymbols=false","-o",WORK/"app"])
 with zipfile.ZipFile(REPO/".geometry-inputs/DEVF_CP01_EVIDENCIA_F_SP121.zip") as z:
  names=[n for n in z.namelist() if Path(n).name=="R03_00.arc"];assert len(names)==1;raw=z.read(names[0])
 assert sha(raw)=="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af"
 arc=WORK/"R03_00.arc";arc.write_bytes(raw)
 exe=WORK/"app/DEVF Map Editor CP09.exe"
 run("app-tests",[exe,"--self-test",arc,WORK/"tests"])
 result["compatibility"]=json.loads((WORK/"tests/result.json").read_text())
 result["importer"]=json.loads((WORK/"tests/importer-result.json").read_text())
 os.environ["DEVF_CP01_LOCATION_WORK"]=str(WORK/"private-python")
 sys.path.insert(0,str(REPO/"DEVF/CP01/analysis"))
 import analyze_location as al,location_geometry as lg
 _,_,members=al.rarc(raw);lookup={m["path"]:m["raw"] for m in members}
 def asset(s):return al.yaz(next(v for k,v in lookup.items() if k.endswith(s)))
 _,tris=lg.kcl(asset("kcl/room.kcl"),asset("plc/room.plc"))
 for e in json.loads((WORK/"tests/encounters.json").read_text())["encounters"]:
  hits=lg.supports(tris,e["x"],e["z"],-5005);assert hits and abs(e["y"]-(hits[0][0]+5))<.002
 for x,y,z in [(-1334.75342,-6339.34229,60011.2109),(-1805.60547,-6375.58301,60777.1289),(-1039.68616,-6316.24902,61329.5977)]:
  hits=lg.supports(tris,x,z,-5005);assert hits and abs(y-(hits[0][0]+5))<.02
 result["python_independent_geometry"]="PASS CP01 reader; .002 candidate/.02 rounded human-reference tolerance"
 result["app"]=pe(exe.read_bytes());shutil.copyfile(exe,DIST/exe.name)
 negatives=0
 for bad in [b"bad",exe.read_bytes()[:100]]:
  try:pe(bad)
  except (AssertionError,struct.error):negatives+=1
 assert negatives==2;result["PE_negatives"]=negatives
 for name in ["README.md","HUMAN_TEST.md","RECOVERY.md","IMPLEMENTATION.md","CP10_HANDOFF.md"]:shutil.copyfile(TASK/name,DIST/name)
 with zipfile.ZipFile(DIST/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
  for name,h in ledger.items():z.write(REPO/name,name)
 result["source_ledger"]=pin["source_ledger"]
 manifest=dict(source_commit=pin["source_commit"],ci_commit=result["ci_commit"],version="0.9.0",runtime_mods_changed=False,assets_included=False,human="CISO_REAL_IMPORT_HUMAN_PENDING",app=result["app"],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in DIST.rglob("*") if p.is_file()})
 (DIST/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
 (DIST/"SHA256SUMS.txt").write_text("".join(v["sha256"]+"  "+k+"\n" for k,v in manifest["files"].items()),encoding="utf-8")
 bundle=WORK/"artifact/DEVF_CP09_IMPORTER_TEST.zip"
 with zipfile.ZipFile(bundle,"w",zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in DIST.rglob("*"):
   if p.is_file():
    assert p.suffix.lower() not in [".iso",".ciso",".arc",".kcl",".plc",".dusk",".dll"]
    z.write(p,p.relative_to(DIST).as_posix())
 with safezip(bundle.read_bytes()) as z:assert len(z.namelist())==9
 result["zip"]=dict(filename=bundle.name,bytes=bundle.stat().st_size,sha256=sha(bundle.read_bytes()))
 result["status"]="WINDOWS_BUILD_VERIFIED_PACKAGE_VERIFIED_DOWNLOAD_PENDING"
except Exception as e:result["error"]=str(e);raise
finally:
 (EVIDENCE/"result.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
 print("CP09_BUILD_RESULT="+json.dumps({k:v for k,v in result.items() if k!="source_ledger"},separators=(",",":")),flush=True)
