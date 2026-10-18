import datetime,json,os,re,shutil,subprocess,sys,zipfile
from pathlib import Path
from common import *
import audit
# CP16: native recruitment; no evasion. Historical CP15 lab remains on historical headers only.
# Provider 0.13.0 and editor 0.13.0 are CP13's: rebuilt here from their unchanged CP13 sources for the regression labs and audits, NOT shipped.
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp16-fix01";BUILD=WORK/"build";EVIDENCE=WORK/"evidence";DIST=WORK/"deliver";OUT=WORK/"artifact"
for p in [WORK,BUILD,EVIDENCE,DIST,OUT]:p.mkdir(parents=True,exist_ok=True)
pin=json.loads((TASK/"SOURCE_CI_CP16_FIX01.json").read_text());source=pin["source_commit"]
assert re.fullmatch("[0-9a-f]{40}",source)
result=dict(status="FAILED",source_commit=source,ci_commit=subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),run_id=os.environ["GITHUB_RUN_ID"],game_executed=False,provider="CP13 provider 0.13.0 source rebuilt for regression only, not shipped",toolchain={k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},phases=[])
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
def strip(text):return "\n".join(l.split("//")[0] for l in text.splitlines())
try:
 assert os.name=="nt" and os.environ["VSCMD_ARG_TGT_ARCH"]=="x64"
 for name,h in pin["source_blobs"].items():
  raw=(REPO/name).read_bytes();assert git_blob(raw)==h,name
 result["pinned_blobs_verified"]=len(pin["source_blobs"])
 RT=TASK/"runtime/DEVF";C14=REPO/CP14_DIR/"runtime/DEVF"
 mod=(RT/"CP16_FIX01_BLUE_RECRUITER_RANGE_DIAGNOSTICS/expansion/mod.cpp").read_text(encoding="utf-8")
 blue=(RT/"CP16_FIX01_BLUE_RECRUITER_RANGE_DIAGNOSTICS/expansion/blue_ai_policy.hpp").read_text(encoding="utf-8")
 red=(RT/"CP14_RED_BOKOBLIN_LIFESTEAL_GROUP_REGEN_GOLD/expansion/red_policy.hpp").read_text(encoding="utf-8")
 pol=(RT/"CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/strokes_policy.hpp").read_text(encoding="utf-8")
 code=strip(mod);pcode=strip(pol);rcode=strip(red);bcode=strip(blue)
 # ---- CP13/CP12/CP11/FIX02/FIX01 static gates, unchanged in substance (paths now in the CP14 runtime) ----
 assert "ID_ZI_J_LK_SWL_C" in code and "ID_ZI_J_LK_SWL_A" not in code and "0x0A58" not in code
 for f in ["ID_ZI_J_LK_SWL_B","ID_AK_JN_WINDLINE00","WINDLINE","ID_ZF_J_FAIRY00_GLOW","FAIRY00","0x0A59","0x8046","0x072F","setGlobalRTMatrix","mSwordModel","getBaseTRMtx","setGlobalRotation","setGlobalTranslation","createSimpleEmitterID","GfxService","setRate(","setLifeTime(","setVolumeSize("]:
  assert f not in code,("forbidden in mod.cpp: "+f)
 assert "kResource=0x0B9D" in pcode and "en.set(res," in pcode and "never_used" in pcode and "0x0A58" in pcode.split("kNeverUsed[]=")[1].split(";")[0]
 assert code.count("DEFINE_HOOK(")==1 and code.count("hook::add_pre<")==1 and code.count("hook::add_post<")==1 and code.count("dComIfGp_particle_set(")==1
 assert 'kVersion="0.13.1"' in code
 assert pcode.count("kGeometry{2.4f,0.70f,0.60f,1.0f,720,650,0.10f,0.99498744f,0.0f}")==1 and "geometry_for(Variant) noexcept {return kGeometry;}" in pcode and "kAlphaCap=255" in pcode
 assert code.count("setLocalTranslation(zero)")==1 and code.count("setLocalScale(one)")==1 and code.count("setDirection(dir)")==1 and "up(0.0f,1.0f,0.0f)" not in code and code.count("mLocalRot.x=0;")==1
 eyes=(RT/"CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/variant_eyes.hpp").read_text(encoding="utf-8");ecode=strip(eyes)
 clock=(RT/"CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/perf_clock.cpp").read_text(encoding="utf-8")
 assert "kRedEyes{70,170,25,12,32,devf_stk::kBlackColour.r,devf_stk::kBlackColour.g,devf_stk::kBlackColour.b}" in ecode and "kCyanEyesAnyHue{0,360,0,12,32,devf_stk::kBlueColour.r" in ecode
 assert code.count("prepare_blue_eyes(a,entry);")==1 and code.count("devf_eyes12::step_eyes(")==1 and code.count("std::strcmp(mat->mMaterialName,\"oc_eye_mat\")")==1 and "devf_eye::kPurple" not in code
 assert "TEVSTR_NOT_READY_ACTOR_NOT_DRAWN_YET" in pcode and "++rt.waits==1" in pcode and "transient(\"TEVSTR_NOT_READY_YET\")" not in pcode
 assert code.count("LOG_LEVEL_ERROR")==1 and "event=PERF_WINDOW" in code and "level_note=MEASUREMENT_NOT_AN_ERROR" in code and "<windows.h>" not in code
 assert clock.count("#include <windows.h>")==1 and clock.count("extern \"C\"")==2 and "QueryPerformanceCounter" in clock and "GetSystemTimeAsFileTime" in clock
 assert "devf_perf::complete_snapshot(table,normal_table,s," in code and code.count("getLayerNo(0)")==2 and "devf_perf::LogBudget log_budget;" in code
 assert code.count("devf_perf::find_used(")==14 and "vledger.find(" not in code and "ledger.find(" not in code   # CP13: 5; CP14 adds regen_multiplier + the claim confirmation
 assert "  if(created){" in code and code.count("setGlobalPrmColor(")==1 and code.count("setGlobalEnvColor(")==1 and code.count("setGlobalAlpha(")==1
 assert code.count("setGlobalDynamicsScale(")==1 and code.count("setGlobalParticleScale(")==1
 assert "kGeometry{2.4f,0.70f,0.60f,1.0f,720,650,0.10f,0.99498744f,0.0f}" in pcode and "kBlueColour{80,190,255,30,90,170}" in pcode and "kBlackColour{255,32,24,130,0,6}" in pcode
 assert "mSphs_cc[0].GetCX()" in code and "mSphs_cc[1].GetCZ()" in code and "ring_centre(" in pcode and "kMaxTorsoOffset=80.0f" in pcode
 assert code.count("em->mPrmClr.a=")==1 and code.count("setRateStep(")==1 and "kBasePrm{255,255,255,255}" in pcode and "kRateStep=0" in pcode
 assert "mSphs_cc[0].Set" not in code and "mSphs_cc[1].Set" not in code
 assert "static_assert(configured_basis(kGeometry).drawable" in pcode and "!draw_basis(V3{0.0f,1.0f,0.0f},V3{0.0f,1.0f,0.0f}).drawable" in pcode
 assert code.count("event=DRAW_BASIS_CHECK")==1 and "evidence=MATHEMATICAL_READBACK_NOT_RENDERED" in code and "em->mGlobalRot" in code
 for f in ["VISUAL_PASS","VISUAL_ACCEPTED","PIXELS_RENDERED","HUMAN_PASS","RUNTIME_PASS"]:
  assert f not in code and f not in rcode,("no pixel / human claims: "+f)
 assert len(re.findall(r"mVolumeSweep\s*=(?!=)",code))==1 and len(re.findall(r"mVolumeMinRad\s*=(?!=)",code))==1 and "route=SHARED_BODY_HUGGING_STROKES" in code and "SHARED_CANONICAL_RING" not in code
 # ---- CP14 gates: Red route contained, Link read-only, one lifesteal write, x2 never more ----
 assert code.count("dComIfGp_getLinkPlayer()")==2 and code.count("dComIfGs_getLife()")==1 and code.count("dComIfGp_getItemLifeCount()")==1   # CP14 red watch + CP15 Blue AI
 for f in ["setDamagePoint","setItemLifeCount","dComIfGs_setLife","SetTgHit","ClrTgHit","ClrAtHit","OnTgShieldHit","OnAtShieldHit","GetTgHitAc","GetAtHitAc"]:
  assert f not in code,("CP14 must only READ Link / collider state: "+f)
 assert not re.findall(r"\bk->[\w\.\[\]]+\s*=(?!=)",code) and not re.findall(r"\bra->[\w\.\[\]]+\s*=(?!=)",code) and not re.findall(r"\bk->mTgCyls\[i\]\.Set",code)
 assert code.count("a->health=o.hp_after")==1 and code.count("a->health=")==4 and code.count("devf_variant::step(*e,from,a->mActionMode,mult)")==1
 assert "kLifestealPm=250;" in rcode and "kWindowTicks=120;" in rcode and "kRegenMultiplier=2;" in rcode and "static_assert(kWindowTicks==4*devf_variant::kTicksPerSecond" in rcode
 assert "own_window||ally_in_range?kRegenMultiplier:1u;" in rcode and "s.window=kWindowTicks;" in rcode and "s.window+=" not in rcode
 assert code.count("red_watch();")==1 and code.count("devf_red::make_claim(")==1 and code.count("devf_red::confirm(")==1 and code.count("devf_red::red_post(")==1
 assert "kRedGoldColour{255,190,30,150,90,0}" in pcode and "kGoldEyes{70,170,25,12,32,devf_stk::kRedGoldColour.r" in rcode and code.count("prepare_gold_eyes(a,entry);")==1
 # CP16 gates additionally prove the entire non-Blue runtime and every other mod.cpp line is unchanged from the actually compiled CP15.
 import source_gate
 result["runtime_provenance"],result["source_checks"]=source_gate.check(REPO,TASK)
 official=REPO/".official"
 assert subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()==SOURCE_HOST
 assert subprocess.check_output(["git","-C",str(official/"extern/aurora"),"rev-parse","HEAD"],text=True).strip()==SOURCE_AURORA
 archive=REPO/".private-inputs/Dusklight-v2.0.3-win32-x86_64.zip"
 assert archive.stat().st_size==56037777 and sha(archive.read_bytes())=="ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf"
 with zipfile.ZipFile(archive) as z:
  member=next(n for n in z.namelist() if n.endswith("sdk/windows-amd64.lib"));lib=z.read(member)
 assert len(lib)==9400986 and sha(lib)=="4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4"
 for symbol in json.loads((TASK/"ci/reviewed-imports.json").read_text())["dusklight.exe"]:
  assert symbol.encode("ascii") in lib,("MISSING_EXACT_OFFICIAL_IMPORT_SYMBOL",symbol)
 result["official_import_symbols_verified"]=True
 libpath=WORK/"windows-amd64.lib";libpath.write_bytes(lib)
 run("configure",["cmake","-S",TASK,"-B",BUILD,"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDEVF_SOURCE_SHA="+source,"-DDUSK_GAME_EXE="+str(libpath)])
 msvc_text=run("msvc",["cmake","--build",BUILD,"--target","bokoblin_expansion","devf_map_provider","cp15_lab","cp16_lab","fix01_lab","cp14_regress_lab","cp13_lab","map_lab","legacy_lab","black_lab","texture_lab","variant_lab","damage_lab","dynamic_damage_lab","visual_lab","eye_lab","strokes_lab","perf_lab","cp12_lab","api_c_abi","--parallel","2"])
 assert "C4477" not in msvc_text and "C4473" not in msvc_text,"printf format mismatch reported by MSVC"
 result["printf_format_warnings"]=0
 lab15=run("cp15-blue-ai-lab",[BUILD/"cp15_lab.exe"]);assert "CP15_LAB_PASS cases=38" in lab15;result["cp15_lab_cases"]=len(re.findall("CASE_PASS ",lab15));assert result["cp15_lab_cases"]==38
 lab16=run("cp16-blue-recruit-lab",[BUILD/"cp16_lab.exe"]);assert "CP16_LAB_PASS cases=56" in lab16;result["cp16_lab_cases"]=len(re.findall("CASE_PASS ",lab16));assert result["cp16_lab_cases"]==56
 labfix=run("cp16-fix01-lab",[BUILD/"fix01_lab.exe"]);assert "FIX01_LAB_PASS cases=53" in labfix;result["fix01_lab_cases"]=len(re.findall("CASE_PASS ",labfix));assert result["fix01_lab_cases"]==53
 lab14=run("cp14-red-lab-on-cp16-runtime",[BUILD/"cp14_regress_lab.exe"]);assert "CP14_LAB_PASS cases=16" in lab14;result["cp14_lab_cases"]=len(re.findall("CASE_PASS ",lab14));assert result["cp14_lab_cases"]==16
 lab13=run("cp13-runtime-lab",[BUILD/"cp13_lab.exe"]);assert "CP13_LAB_PASS" in lab13;result["cp13_runtime_cases"]=len(re.findall("CASE_PASS ",lab13));assert result["cp13_runtime_cases"]>=26
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
 assert "STROKES_LAB_PASS cases=36" in klab
 result["strokes_lab_cases"]=len(re.findall("CASE_PASS ",klab));assert result["strokes_lab_cases"]==36
 plab=run("perf-lab",[BUILD/"perf_lab.exe"])
 assert "PERF_LAB_PASS cases=4" in plab
 result["perf_lab_cases"]=len(re.findall("CASE_PASS ",plab));assert result["perf_lab_cases"]==4
 qlab=run("cp12-lab",[BUILD/"cp12_lab.exe"])
 assert "CP12_LAB_PASS cases=7" in qlab
 result["cp12_lab_cases"]=len(re.findall("CASE_PASS ",qlab));assert result["cp12_lab_cases"]==7
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
 result["cp13_editor_tests"]=json.loads((tests/"cp13-result.json").read_text())
 result["app_tests"]=json.loads((tests/"result.json").read_text())
 result["importer_tests"]=json.loads((tests/"importer-result.json").read_text())
 result["multi_tests"]=json.loads((tests/"multi-result.json").read_text())
 result["name_tests"]=json.loads((tests/"names-result.json").read_text())
 assert result["cp13_editor_tests"]["status"]=="CP13_EDITOR_LAB_PASS"
 assert result["app_tests"]["status"]=="CP08_APP_LAB_PASS" and result["app_tests"]["cases"]>=42
 assert result["importer_tests"]["status"]=="CP09_IMPORTER_LAB_PASS" and result["importer_tests"]["cases"]>=71
 assert result["multi_tests"]["status"]=="CP10_STATIC_LAB_PASS" and result["multi_tests"]["cases"]>=59+25+7+18
 assert result["name_tests"]["status"]=="CP10_1_NAME_LAB_PASS" and result["name_tests"]["cases"]==25
 run("CSharp-CP13-32-to-Cxx",[BUILD/"cp13_lab.exe",tests/"cp13-encounters.json"])
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
 result["editor_regression_pe"]=app_pe(exe)   # NOT shipped: the user keeps the CP13 editor
 dll=BUILD/"bokoblin_expansion.dll";before=dll.read_bytes()
 neg=audit.negatives(dll,WORK)
 run("expansion-package",["cmake","--build",BUILD,"--target","bokoblin_expansion_package"])
 assert dll.read_bytes()==before
 pkg=BUILD/"mods/bokoblin_expansion.dusk";verified=audit.verify_package(pkg,dll,TASK)
 assert verified["pe"]["status"]=="CP16_FIX01_PE_ABI_PASS"
 shutil.copyfile(pkg,DIST/DUSK_NAME);result["package"]=dict(filename=DUSK_NAME,**{k:v for k,v in verified.items() if k!="members"},negatives=neg)
 for what in ["headers","imports","exports"]:run("expansion-"+what,["dumpbin","/"+what,dll])
 import provider_audit
 pdll=BUILD/"devf_map_provider.dll";pneg=provider_audit.prior.negatives(pdll,WORK,"provider")
 run("provider-package",["cmake","--build",BUILD,"--target","devf_map_provider_package"])
 ppkg=BUILD/"mods/devf_map_provider.dusk";paudit=provider_audit.verify(ppkg,pdll)
 result["provider_regression"]=dict(shipped=False,**paudit,negatives=pneg,same_dll_bytes_as_cp13_shipped=paudit["dll_sha256"]==CP13_PROVIDER["dll_sha256"],user_keeps=CP13_PROVIDER)
 source_paths=[REPO/p for p in pin["source_blobs"]]
 result["source_ledger"]={p.relative_to(REPO).as_posix():sha(p.read_bytes()) for p in source_paths}
 with zipfile.ZipFile(DIST/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
  for p in source_paths:z.write(p,p.relative_to(REPO).as_posix())
 for name in ["README.md","IMPLEMENTATION.md","HUMAN_TEST.md","RECOVERY.md"]:shutil.copyfile(TASK/name,DIST/name)
 (DIST/"TEST_RESULTS.json").write_text(json.dumps({k:result[k] for k in ["cp15_lab_cases","cp16_lab_cases","fix01_lab_cases","cp14_lab_cases","cp13_editor_tests","cp13_runtime_cases","app_tests","importer_tests","multi_tests","name_tests","runtime_lab_cases","black_lab_cases","texture_lab_cases","variant_lab_cases","damage_lab_cases","dynamic_lab_cases","visual_lab_cases","eye_lab_cases","strokes_lab_cases","perf_lab_cases","cp12_lab_cases","source_checks","runtime_provenance","independent_geometry_height","provider_regression"]},indent=2)+"\n")
 for p in DIST.rglob("*"):
  if p.is_file() and p.suffix.lower() in [".arc",".kcl",".plc",".bmd",".bti",".png",".dds",".cmpr",".dll",".ciso",".iso",".exe"]:raise RuntimeError("Original/derived asset, image, loose DLL or EXE forbidden")
 manifest=dict(source_commit=source,ci_commit=result["ci_commit"],run_id=result["run_id"],sdk_headers=SOURCE_HOST,status="WINDOWS_BUILD_VERIFIED_PACKAGE_VERIFIED_DOWNLOAD_PENDING",human="HUMAN_RUNTIME_PENDING",map_data_included=False,requires_provider=CP13_PROVIDER,package=result["package"],files={p.relative_to(DIST).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in DIST.rglob("*") if p.is_file()})
 (DIST/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
 (DIST/"SHA256SUMS.txt").write_text("".join(v["sha256"]+"  "+k+"\n" for k,v in manifest["files"].items()))
 bundle=OUT/"DEVF_CP16_FIX01_TEST.zip"
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
 print("CP16_FIX01_BUILD_RESULT="+json.dumps({k:v for k,v in result.items() if k!="source_ledger"},separators=(",",":")),flush=True)
