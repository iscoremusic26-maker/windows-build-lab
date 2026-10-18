import datetime,json,os,re,shutil,subprocess,sys,zipfile
from pathlib import Path
from common import *
import audit
# CP10.6D: Expansion 0.8.6 shared corrected strokes route (Blue = Black geometry, colour only) (.dusk) only. Provider 0.10.0 and editor 0.10.2 are reused (editor rebuilt here only for regression tests, not shipped).
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp10-6d";BUILD=WORK/"build";EVIDENCE=WORK/"evidence";DIST=WORK/"deliver";OUT=WORK/"artifact"
for p in [WORK,BUILD,EVIDENCE,DIST,OUT]:p.mkdir(parents=True,exist_ok=True)
pin=json.loads((TASK/"SOURCE_CI_CP106D.json").read_text());source=pin["source_commit"]
assert re.fullmatch("[0-9a-f]{40}",source)
result=dict(status="FAILED",source_commit=source,ci_commit=subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),run_id=os.environ["GITHUB_RUN_ID"],game_executed=False,provider="REUSED CP10 DEVF_Provider_0.10.0_TEST.dusk (not rebuilt)",toolchain={k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},phases=[])
def run(label,args,cwd=None):
 p=subprocess.run([str(x) for x in args],capture_output=True,text=True,encoding="utf-8",errors="replace",cwd=cwd or ((REPO/EDITOR_CSPROJ).parent if str(args[0])=="dotnet" else None));text=p.stdout+p.stderr
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
 # Source-level gates of this PILOT (static, on the pinned source, comments stripped): pilot ids only, one emitter path, no sword matrix, ONE execute hook.
 mod=(TASK/"expansion/mod.cpp").read_text(encoding="utf-8")
 pol=(TASK/"expansion/strokes_policy.hpp").read_text(encoding="utf-8")
 code="\n".join(l.split("//")[0] for l in mod.splitlines())
 pcode="\n".join(l.split("//")[0] for l in pol.splitlines())
 assert "ID_ZI_J_LK_SWL_C" in code and "ID_ZI_J_LK_SWL_A" not in code and "0x0A58" not in code   # Blue no longer has its own route
 for f in ["ID_ZI_J_LK_SWL_B","ID_AK_JN_WINDLINE00","WINDLINE","ID_ZF_J_FAIRY00_GLOW","FAIRY00","0x0A59","0x8046","0x072F","setGlobalRTMatrix","mSwordModel","getBaseTRMtx","setGlobalRotation","setGlobalTranslation","createSimpleEmitterID","GfxService","setRate(","setLifeTime(","setVolumeSize("]:
  assert f not in code,("forbidden in mod.cpp: "+f)
 assert "kResource=0x0B9D" in pcode and "en.set(res," in pcode and "never_used" in pcode and "0x0A58" in pcode.split("kNeverUsed[]=")[1].split(";")[0]
 assert code.count("DEFINE_HOOK(")==1 and code.count("hook::add_pre<")==1 and code.count("hook::add_post<")==1 and code.count("dComIfGp_particle_set(")==1
 assert "0.8.6" in code
 # CP10.6D shared-route gates (static, comments stripped): ONE geometry for every variant, canonical frame written once per configure on the emitter.
 assert pcode.count("kGeometry{3.8f,0.70f,0.55f,1.0f,640,650}")==1 and "geometry_for(Variant) noexcept {return kGeometry;}" in pcode and "kAlphaCap=234" in pcode
 assert code.count("setLocalTranslation(zero)")==1 and code.count("setLocalScale(one)")==1 and code.count("setDirection(up)")==1 and code.count("mLocalRot.x=0;")==1
 assert len(re.findall(r"mVolumeSweep\s*=(?!=)",code))==1 and len(re.findall(r"mVolumeMinRad\s*=(?!=)",code))==1 and "route=SHARED_CANONICAL_RING" in code
 result["source_checks"]=dict(shared_route="Blue and Black: 0x0B9D SWL_C, ONE geometry 3.8/0.70/0.55/sweep 1.0/min_rad 0.64/height 0.65, canonical emitter frame (trs 0, rot 0, scl 1, dir up); only colour differs",pilot_ids="0x0B9D SWL_C for both, bank 0; 0x0A58 absent",forbidden_ids_and_sword_matrix_absent=True,define_hook_count=1,pre_hooks=1,post_hooks=1,particle_set_call_sites=1)
 # Editor reuse proof: every CP10.2 editor/app source blob is identical to the CP10.2 pin.
 cp102=json.loads((REPO/"DEVF/CP10_2_BLACK_BOKOBLIN/SOURCE_CI_CP102.json").read_text())["source_blobs"]
 app_keys=[k for k in cp102 if k.startswith("DEVF/CP10_2_BLACK_BOKOBLIN/app/") or k.startswith("DEVF/CP10_1_ROOM_NAMES/app/")]
 assert app_keys and all(pin["source_blobs"].get(k)==cp102[k] for k in app_keys)
 result["editor_source_identical_to_cp102"]=True
 official=REPO/".official"
 assert subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()==SOURCE_HOST
 assert subprocess.check_output(["git","-C",str(official/"extern/aurora"),"rev-parse","HEAD"],text=True).strip()==SOURCE_AURORA
 archive=REPO/".private-inputs/Dusklight-v2.0.3-win32-x86_64.zip"
 assert archive.stat().st_size==56037777 and sha(archive.read_bytes())=="ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf"
 with zipfile.ZipFile(archive) as z:
  member=next(n for n in z.namelist() if n.endswith("sdk/windows-amd64.lib"));lib=z.read(member)
 assert len(lib)==9400986 and sha(lib)=="4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4"
 libpath=WORK/"windows-amd64.lib";libpath.write_bytes(lib)
 run("configure",["cmake","-S",TASK,"-B",BUILD,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDEVF_SOURCE_SHA="+source,"-DDUSK_GAME_EXE="+str(libpath)])
 run("msvc",["cmake","--build",BUILD,"--target","bokoblin_expansion","map_lab","legacy_lab","black_lab","texture_lab","variant_lab","damage_lab","dynamic_damage_lab","visual_lab","eye_lab","strokes_lab","api_c_abi","--parallel","2"])
 run("legacy-runtime-lab",[BUILD/"legacy_lab.exe"])
 lab=run("runtime-lab",[BUILD/"map_lab.exe"])
 assert "CP10_RUNTIME_LAB_PASS" in lab
 result["runtime_lab_cases"]=len(re.findall("CASE_PASS ",lab));assert result["runtime_lab_cases"]==66
 blab=run("black-lab",[BUILD/"black_lab.exe"])
 assert "BLACK_LAB_PASS cases=26" in blab
 result["black_lab_cases"]=len(re.findall("CASE_PASS ",blab))
 rlab=run("variant-lab",[BUILD/"variant_lab.exe"])
 assert "VARIANT_LAB_PASS cases=30" in rlab
 result["variant_lab_cases"]=len(re.findall("CASE_PASS ",rlab))
 dlab=run("damage-lab",[BUILD/"damage_lab.exe"])
 assert "DAMAGE_LAB_PASS cases=22" in dlab
 result["damage_lab_cases"]=len(re.findall("CASE_PASS ",dlab));assert result["damage_lab_cases"]==22
 ylab=run("dynamic-damage-lab",[BUILD/"dynamic_damage_lab.exe"])
 assert "DYNAMIC_DAMAGE_LAB_PASS cases=28" in ylab
 result["dynamic_lab_cases"]=len(re.findall("CASE_PASS ",ylab));assert result["dynamic_lab_cases"]==28
 klab=run("strokes-lab",[BUILD/"strokes_lab.exe"])
 assert "STROKES_LAB_PASS cases=28" in klab
 result["strokes_lab_cases"]=len(re.findall("CASE_PASS ",klab));assert result["strokes_lab_cases"]==28
 vlab=run("visual-lab",[BUILD/"visual_lab.exe"])
 assert "VISUAL_LAB_PASS cases=22" in vlab
 result["visual_lab_cases"]=len(re.findall("CASE_PASS ",vlab));assert result["visual_lab_cases"]==22
 elab=run("eye-lab",[BUILD/"eye_lab.exe"])
 assert "EYE_LAB_PASS cases=9" in elab
 result["eye_lab_cases"]=len(re.findall("CASE_PASS ",elab));assert result["eye_lab_cases"]==9
 tlab=run("texture-lab",[BUILD/"texture_lab.exe"])
 assert "TEXTURE_LAB_PASS cases=20" in tlab and "GOLDEN src=923196cf9a7bedff out=1f01aafe6dd92478" in tlab
 result["texture_lab_cases"]=len(re.findall("CASE_PASS ",tlab))
 app=WORK/"app"
 run("dotnet-publish",["dotnet","publish",REPO/EDITOR_CSPROJ,"-c","Release","-r","win-x64","--self-contained","true","-p:PublishSingleFile=true","-p:IncludeNativeLibrariesForSelfExtract=true","-p:DebugType=None","-p:DebugSymbols=false","-o",app])
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
 # No-regression gates vs CP10 run 38988277707 (42/71/59) and CP10.1 (names 25); CP10.2 only adds cases.
 assert result["app_tests"]["status"]=="CP08_APP_LAB_PASS" and result["app_tests"]["cases"]>=42
 assert result["importer_tests"]["status"]=="CP09_IMPORTER_LAB_PASS" and result["importer_tests"]["cases"]>=71
 assert result["multi_tests"]["status"]=="CP10_STATIC_LAB_PASS" and result["multi_tests"]["cases"]>=59+25+7+18
 assert result["name_tests"]["status"]=="CP10_1_NAME_LAB_PASS" and result["name_tests"]["cases"]==25
 run("CSharp-schema1-to-Cxx",[BUILD/"map_lab.exe",tests/"encounters.json"])
 run("CSharp-schema2-to-Cxx",[BUILD/"map_lab.exe",tests/"multi-encounters.json"])
 run("CSharp-schema2-black-to-Cxx",[BUILD/"map_lab.exe",tests/"multi-encounters-black.json"])
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
 result["app_pe_rebuilt_not_shipped"]=app_pe(exe)
 dll=BUILD/"bokoblin_expansion.dll";before=dll.read_bytes()
 neg=audit.negatives(dll,WORK)
 run("expansion-package",["cmake","--build",BUILD,"--target","bokoblin_expansion_package"])
 assert dll.read_bytes()==before
 pkg=BUILD/"mods/bokoblin_expansion.dusk";verified=audit.verify_package(pkg,dll,TASK)
 shutil.copyfile(pkg,DIST/DUSK_NAME);result["package"]=dict(filename=DUSK_NAME,**{k:v for k,v in verified.items() if k!="members"},negatives=neg)
 for what in ["headers","imports","exports"]:run("expansion-"+what,["dumpbin","/"+what,dll])
 source_paths=[REPO/p for p in pin["source_blobs"]]
 result["source_ledger"]={p.relative_to(REPO).as_posix():sha(p.read_bytes()) for p in source_paths}
 with zipfile.ZipFile(DIST/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
  for p in source_paths:z.write(p,p.relative_to(REPO).as_posix())
 for name in ["README.md","IMPLEMENTATION.md","HUMAN_TEST.md"]:shutil.copyfile(TASK/name,DIST/name)
 shutil.copyfile(EVIDENCE/"app-self-test.log",DIST/"SELF_TEST_LOG.txt")
 (DIST/"TEST_RESULTS.json").write_text(json.dumps({k:result[k] for k in ["app_tests","importer_tests","multi_tests","name_tests","runtime_lab_cases","black_lab_cases","texture_lab_cases","variant_lab_cases","damage_lab_cases","dynamic_lab_cases","visual_lab_cases","eye_lab_cases","strokes_lab_cases","source_checks","independent_geometry_height","editor_source_identical_to_cp102"]},indent=2)+"\n")
 for p in DIST.rglob("*"):
  if p.is_file() and p.suffix.lower() in [".arc",".kcl",".plc",".bmd",".bti",".png",".dds",".cmpr",".dll",".exe",".ciso",".iso"]:raise RuntimeError("Original/derived asset, image, loose DLL or EXE forbidden")
 manifest=dict(source_commit=source,ci_commit=result["ci_commit"],run_id=result["run_id"],sdk_headers=SOURCE_HOST,status="WINDOWS_BUILD_VERIFIED_PACKAGE_VERIFIED_DOWNLOAD_PENDING",human="HUMAN_RUNTIME_PENDING",map_data_included=False,provider="REUSE CP10 DEVF_Provider_0.10.0_TEST.dusk (dev.devf.cp06_native_config_dual 0.10.0), not included",editor="REUSE CP10.2 DEVF Map Editor CP10.2.exe 0.10.2 (source identical; run 39060071547), not included",package=result["package"],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in DIST.rglob("*") if p.is_file()})
 (DIST/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
 (DIST/"SHA256SUMS.txt").write_text("".join(v["sha256"]+"  "+k+"\n" for k,v in manifest["files"].items()))
 bundle=OUT/"DEVF_Bokoblin_Strokes_CP10.6D_TEST.zip"
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
 print("CP10_6D_BUILD_RESULT="+json.dumps({k:v for k,v in result.items() if k!="source_ledger"},separators=(",",":")),flush=True)
