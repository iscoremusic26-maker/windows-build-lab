import datetime,hashlib,json,os,re,shutil,subprocess,zipfile,sys
from pathlib import Path
from audit import *
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp10";BUILD=WORK/"build";EVIDENCE=WORK/"evidence";DIST=WORK/"deliver";OUT=WORK/"artifact"
for p in [WORK,BUILD,EVIDENCE,DIST,OUT]:p.mkdir(parents=True,exist_ok=True)
pin=json.loads((TASK/"SOURCE_CI.json").read_text());source=pin["source_commit"]
result=dict(status="FAILED",source_commit=source,ci_commit=subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),run_id=os.environ["GITHUB_RUN_ID"],game_executed=False,toolchain={k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},phases=[])
def run(label,args):
 p=subprocess.run([str(x) for x in args],capture_output=True,text=True,encoding="utf-8",errors="replace",cwd=TASK/"app" if str(args[0])=="dotnet" else None);text=p.stdout+p.stderr
 for val,tag in [(str(REPO),"<CHECKOUT>"),(str(WORK),"<WORK>"),(os.environ.get("USERPROFILE",""),"<RUNNER>")]:
  if val:text=text.replace(val,tag).replace(val.replace("\\","/"),tag)
 (EVIDENCE/(label+".log")).write_text(text,encoding="utf-8")
 result["phases"].append(dict(label=label,exit=p.returncode));print(label,p.returncode,flush=True)
 if p.returncode:
  fail=WORK/"app-tests/FAIL.txt"
  if fail.exists():text+="\n"+fail.read_text()
  print(text[-18000:],flush=True);raise RuntimeError(label)
 return text
try:
 assert os.name=="nt" and os.environ["VSCMD_ARG_TGT_ARCH"]=="x64"
 for name,h in pin["source_blobs"].items():
  raw=(REPO/name).read_bytes();assert hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()==h,name
 official=REPO/".official";assert subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()=="40457c6adb381928e4b5fef6ed459ed291edd5e2"
 archive=REPO/".private-inputs/Dusklight-v2.0.3-win32-x86_64.zip"
 assert archive.stat().st_size==56037777 and sha(archive.read_bytes())=="ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf"
 with zipfile.ZipFile(archive) as z:
  member=next(n for n in z.namelist() if n.endswith("sdk/windows-amd64.lib"));lib=z.read(member)
 assert len(lib)==9400986 and sha(lib)=="4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4"
 libpath=WORK/"windows-amd64.lib";libpath.write_bytes(lib)
 run("configure",["cmake","-S",TASK,"-B",BUILD,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDEVF_SOURCE_SHA="+source,"-DDUSK_GAME_EXE="+str(libpath)])
 run("msvc",["cmake","--build",BUILD,"--target","devf_map_provider","bokoblin_expansion","map_lab","legacy_lab","api_c_abi","--parallel","2"])
 run("legacy-runtime-lab",[BUILD/"legacy_lab.exe"])
 lab=run("runtime-lab",[BUILD/"map_lab.exe"])
 assert "CP10_RUNTIME_LAB_PASS" in lab
 result["runtime_lab_cases"]=len(re.findall("CASE_PASS ",lab))
 app=WORK/"app"
 run("dotnet-publish",["dotnet","publish",TASK/"app/DEVF.MapEditor.csproj","-c","Release","-r","win-x64","--self-contained","true","-p:PublishSingleFile=true","-p:IncludeNativeLibrariesForSelfExtract=true","-p:DebugType=None","-p:DebugSymbols=false","-o",app])
 with zipfile.ZipFile(REPO/".geometry-inputs/DEVF_CP01_EVIDENCIA_F_SP121.zip") as z:
  members=[n for n in z.namelist() if Path(n).name=="R03_00.arc"];assert len(members)==1;raw=z.read(members[0])
 assert sha(raw)=="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af"
 arc=WORK/"R03_00.arc";arc.write_bytes(raw)
 tests=WORK/"app-tests";exe=app/"DEVF Map Editor CP10.exe"
 run("app-real-geometry-tests",[exe,"--self-test",arc,tests])
 result["app_tests"]=json.loads((tests/"result.json").read_text())
 result["importer_tests"]=json.loads((tests/"importer-result.json").read_text())
 result["multi_tests"]=json.loads((tests/"multi-result.json").read_text())
 run("CSharp-schema1-to-Cxx",[BUILD/"map_lab.exe",tests/"encounters.json"])
 run("CSharp-schema2-to-Cxx",[BUILD/"map_lab.exe",tests/"multi-encounters.json"])
 # Independent Python reader from previous checkpoint, only this room, compare new candidate heights.
 os.environ["DEVF_CP01_LOCATION_WORK"]=str(WORK/"private-query")
 sys.path.insert(0,str(REPO/"DEVF/CP01/analysis"))
 import analyze_location as al,location_geometry as lg
 _,_,members=al.rarc(raw);lookup={m["path"]:m["raw"] for m in members}
 def asset(s):return al.yaz(next(v for k,v in lookup.items() if k.endswith(s)))
 _,tris=lg.kcl(asset("kcl/room.kcl"),asset("plc/room.plc"))
 for e in json.loads((tests/"encounters.json").read_text())["encounters"]:
  hits=lg.supports(tris,e["x"],e["z"],-5005);assert hits
  assert abs(float(e["y"])-(hits[0][0]+5))<.002
 result["independent_geometry_height"]="PASS Python prior reader vs new CSharp parser; static only"
 result["app_pe"]=app_pe(exe);shutil.copyfile(exe,DIST/exe.name)
 result["packages"]={}
 for mode,target in [("provider","devf_map_provider"),("expansion","bokoblin_expansion")]:
  dll=BUILD/(target+".dll");before=dll.read_bytes();neg=negatives(dll,WORK,mode)
  run(mode+"-package",["cmake","--build",BUILD,"--target",target+"_package"])
  assert dll.read_bytes()==before
  pkg=BUILD/("mods/"+target+".dusk");audit=verify_package(pkg,dll,mode)
  name=("DEVF_Provider_0.10.0_TEST" if mode=="provider" else "BokoblinExpansion_0.3.0_TEST")+".dusk"
  shutil.copyfile(pkg,DIST/name);result["packages"][mode]=dict(filename=name,**audit,negatives=neg)
  for what in ["headers","imports","exports"]:run(mode+"-"+what,["dumpbin","/"+what,dll])
 source_paths=[REPO/p for p in pin["source_blobs"]]
 ledger={p.relative_to(REPO).as_posix():sha(p.read_bytes()) for p in source_paths}
 result["source_ledger"]=ledger
 with zipfile.ZipFile(DIST/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
  for p in source_paths:z.write(p,p.relative_to(REPO).as_posix())
 for name in ["README.md","HUMAN_TEST.md","RECOVERY.md","IMPLEMENTATION.md","CP11_HANDOFF.md"]:shutil.copyfile(TASK/name,DIST/name)
 shutil.copytree(TASK/"examples",DIST/"examples",dirs_exist_ok=True)
 # Derived coordinates only, no map assets. This is a QA candidate collection, not default activation.
 pass # No synthetic positions or proprietary geometry distributed.
 for p in DIST.rglob("*"):
  if p.is_file() and p.suffix.lower() in [".arc",".kcl",".plc",".bmd",".dll"]:raise RuntimeError("Original asset/loose DLL forbidden")
 manifest=dict(source_commit=source,ci_commit=result["ci_commit"],run_id=result["run_id"],sdk="40457c6adb381928e4b5fef6ed459ed291edd5e2",status="WINDOWS_BUILD_VERIFIED_PACKAGE_VERIFIED_DOWNLOAD_PENDING",human="HUMAN_RUNTIME_PENDING",map_data_included=False,capacity=8,packages=result["packages"],app=result["app_pe"],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in DIST.rglob("*") if p.is_file()})
 (DIST/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
 (DIST/"SHA256SUMS.txt").write_text("".join(v["sha256"]+"  "+k+"\n" for k,v in manifest["files"].items()))
 bundle=OUT/"DEVF_CP10_MAP_EDITOR_TEST.zip"
 with zipfile.ZipFile(bundle,"w",zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in DIST.rglob("*"):
   if p.is_file():z.write(p,p.relative_to(DIST).as_posix())
 with zipfile.ZipFile(bundle) as z:assert z.testzip() is None
 result["zip"]=dict(filename=bundle.name,bytes=bundle.stat().st_size,sha256=sha(bundle.read_bytes()))
 result["status"]="WINDOWS_BUILD_VERIFIED_PACKAGE_VERIFIED_DOWNLOAD_PENDING"
 assert not subprocess.check_output(["git","-C",str(official),"status","--porcelain","--untracked-files=no"],text=True).strip()
except Exception as e:
 result["error"]=str(e);raise
finally:
 (EVIDENCE/"result.json").write_text(json.dumps(result,indent=2)+"\n")
 print("CP10_BUILD_RESULT="+json.dumps({k:v for k,v in result.items() if k!="source_ledger"},separators=(",",":")),flush=True)
