import datetime,json,os,re,shutil,subprocess,sys,zipfile
from pathlib import Path
from common import *
# CP10.1: editor-only build. No .dusk is built or packaged; CP10 provider 0.10.0 / expansion 0.3.0 are reused unchanged.
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp10-1";BUILD=WORK/"build";EVIDENCE=WORK/"evidence";DIST=WORK/"deliver";OUT=WORK/"artifact"
for p in [WORK,BUILD,EVIDENCE,DIST,OUT]:p.mkdir(parents=True,exist_ok=True)
pin=json.loads((TASK/"SOURCE_CI_CP101.json").read_text());source=pin["source_commit"]
assert re.fullmatch("[0-9a-f]{40}",source)
result=dict(status="FAILED",source_commit=source,ci_commit=subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),run_id=os.environ["GITHUB_RUN_ID"],game_executed=False,dusk_packages="NONE (CP10 provider 0.10.0 / expansion 0.3.0 reused)",toolchain={k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},phases=[])
def run(label,args,cwd=None):
 p=subprocess.run([str(x) for x in args],capture_output=True,text=True,encoding="utf-8",errors="replace",cwd=cwd or (TASK/"app" if str(args[0])=="dotnet" else None));text=p.stdout+p.stderr
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
  raw=(REPO/name).read_bytes();assert git_blob(raw)==h,name
 result["pinned_blobs_verified"]=len(pin["source_blobs"])
 official=REPO/".official";assert subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()==SOURCE_HOST
 # C++ parity labs (unchanged CP10/CP08 sources) compiled directly against the pinned public SDK headers; same flags as the CP10 CMake test targets.
 flags=["/nologo","/std:c++20","/EHsc","/utf-8","/O2","/DNDEBUG","/MD","/I"+str(official/"sdk/include")]
 cp10=REPO/"DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION";cp08=REPO/"DEVF/CP08_MAP_EDITOR_BOKOBLIN_EXPANSION"
 run("msvc-map_lab",["cl",*flags,"/Fo"+str(BUILD)+"\\map_lab.obj","/Fe"+str(BUILD)+"\\map_lab.exe",cp10/"tests/map_lab.cpp"])
 run("msvc-legacy_lab",["cl",*flags,"/Fo"+str(BUILD)+"\\legacy_lab.obj","/Fe"+str(BUILD)+"\\legacy_lab.exe",cp08/"tests/map_lab.cpp"])
 run("legacy-runtime-lab",[BUILD/"legacy_lab.exe"])
 lab=run("runtime-lab",[BUILD/"map_lab.exe"])
 assert "CP10_RUNTIME_LAB_PASS" in lab
 result["runtime_lab_cases"]=len(re.findall("CASE_PASS ",lab));assert result["runtime_lab_cases"]==43
 app=WORK/"app"
 run("dotnet-publish",["dotnet","publish",TASK/"app/DEVF.MapEditor.csproj","-c","Release","-r","win-x64","--self-contained","true","-p:PublishSingleFile=true","-p:IncludeNativeLibrariesForSelfExtract=true","-p:DebugType=None","-p:DebugSymbols=false","-o",app])
 with zipfile.ZipFile(REPO/".geometry-inputs/DEVF_CP01_EVIDENCIA_F_SP121.zip") as z:
  members=[n for n in z.namelist() if Path(n).name=="R03_00.arc"];assert len(members)==1;raw=z.read(members[0])
 assert sha(raw)=="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af"
 arc=WORK/"R03_00.arc";arc.write_bytes(raw)
 tests=WORK/"app-tests";exe=app/EXE_NAME
 run("app-self-test",[exe,"--self-test",arc,tests])
 result["app_tests"]=json.loads((tests/"result.json").read_text())
 result["importer_tests"]=json.loads((tests/"importer-result.json").read_text())
 result["multi_tests"]=json.loads((tests/"multi-result.json").read_text())
 result["name_tests"]=json.loads((tests/"names-result.json").read_text())
 # No-regression gates against the CP10 run 38988277707 baseline (42 / 71 / 59 cases); CP10.1 adds cases, never removes.
 assert result["app_tests"]["status"]=="CP08_APP_LAB_PASS" and result["app_tests"]["cases"]>=42
 assert result["importer_tests"]["status"]=="CP09_IMPORTER_LAB_PASS" and result["importer_tests"]["cases"]>=71
 assert result["multi_tests"]["status"]=="CP10_STATIC_LAB_PASS" and result["multi_tests"]["cases"]>=59+25+7
 assert result["name_tests"]["status"]=="CP10_1_NAME_LAB_PASS" and result["name_tests"]["cases"]==25
 run("CSharp-schema1-to-Cxx",[BUILD/"map_lab.exe",tests/"encounters.json"])
 run("CSharp-schema2-to-Cxx",[BUILD/"map_lab.exe",tests/"multi-encounters.json"])
 os.environ["DEVF_CP01_LOCATION_WORK"]=str(WORK/"private-query")
 sys.path.insert(0,str(REPO/"DEVF/CP01/analysis"))
 import analyze_location as al,location_geometry as lg
 _,_,members=al.rarc(raw);lookup={m["path"]:m["raw"] for m in members}
 def asset(s):return al.yaz(next(v for k,v in lookup.items() if k.endswith(s)))
 _,tris=lg.kcl(asset("kcl/room.kcl"),asset("plc/room.plc"))
 for e in json.loads((tests/"encounters.json").read_text())["encounters"]:
  hits=lg.supports(tris,e["x"],e["z"],-5005);assert hits
  assert abs(float(e["y"])-(hits[0][0]+5))<.002
 result["independent_geometry_height"]="PASS Python prior reader vs C# parser; static only"
 result["app_pe"]=app_pe(exe);shutil.copyfile(exe,DIST/exe.name)
 source_paths=[REPO/p for p in pin["source_blobs"]]
 result["source_ledger"]={p.relative_to(REPO).as_posix():sha(p.read_bytes()) for p in source_paths}
 with zipfile.ZipFile(DIST/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
  for p in source_paths:z.write(p,p.relative_to(REPO).as_posix())
 for name in ["README.md","HUMAN_TEST.md","NAMES_PROVENANCE.md","ALIASES.tsv"]:shutil.copyfile(TASK/name,DIST/name)
 shutil.copyfile(EVIDENCE/"app-self-test.log",DIST/"SELF_TEST_LOG.txt")
 (DIST/"TEST_RESULTS.json").write_text(json.dumps({k:result[k] for k in ["app_tests","importer_tests","multi_tests","name_tests","runtime_lab_cases","independent_geometry_height"]},indent=2)+"\n")
 for p in DIST.rglob("*"):
  if p.is_file() and p.suffix.lower() in [".arc",".kcl",".plc",".bmd",".dll",".dusk",".ciso",".iso"]:raise RuntimeError("Original asset, loose DLL or .dusk forbidden in CP10.1 bundle")
 manifest=dict(source_commit=source,ci_commit=result["ci_commit"],run_id=result["run_id"],sdk_headers=SOURCE_HOST,status="WINDOWS_BUILD_VERIFIED_EDITOR_UI_TEST_READY_DOWNLOAD_PENDING",human="USER_VISUAL_TEST_PENDING",map_data_included=False,dusk_packages="NONE; reuse CP10 provider 0.10.0 and expansion 0.3.0",app=result["app_pe"],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in DIST.rglob("*") if p.is_file()})
 (DIST/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
 (DIST/"SHA256SUMS.txt").write_text("".join(v["sha256"]+"  "+k+"\n" for k,v in manifest["files"].items()))
 bundle=OUT/"DEVF_Map_Editor_CP10.1_TEST.zip"
 with zipfile.ZipFile(bundle,"w",zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in DIST.rglob("*"):
   if p.is_file():z.write(p,p.relative_to(DIST).as_posix())
 with zipfile.ZipFile(bundle) as z:assert z.testzip() is None
 result["zip"]=dict(filename=bundle.name,bytes=bundle.stat().st_size,sha256=sha(bundle.read_bytes()))
 result["status"]="WINDOWS_BUILD_VERIFIED_EDITOR_UI_TEST_READY_DOWNLOAD_PENDING"
 assert not subprocess.check_output(["git","-C",str(official),"status","--porcelain","--untracked-files=no"],text=True).strip()
except Exception as e:
 result["error"]=str(e);raise
finally:
 (EVIDENCE/"result.json").write_text(json.dumps(result,indent=2)+"\n")
 print("CP10_1_BUILD_RESULT="+json.dumps({k:v for k,v in result.items() if k!="source_ledger"},separators=(",",":")),flush=True)
