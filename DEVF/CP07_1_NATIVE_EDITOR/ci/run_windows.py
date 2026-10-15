"""Build one native JSON QA, actual shared core/IO tests; never host/game/DLL execution."""
import datetime,hashlib,json,os,re,shutil,struct,subprocess,time,zipfile
from pathlib import Path
from audit_native import inspect_pe,verify_package,negatives,sha
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1]
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp07-editor";BUILD=WORK/"build";EVIDENCE=WORK/"evidence";DELIVERY=WORK/"TEST"
for p in [WORK,BUILD,EVIDENCE,DELIVERY]:p.mkdir(parents=True,exist_ok=True)
source=json.loads((ROOT/"SOURCE_CI.json").read_text())["source_commit"]
r={"status":"BLOCKED","source_commit":source,"ci_head":subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),"event_merge_sha":os.environ.get("GITHUB_SHA"),"run_id":os.environ["GITHUB_RUN_ID"],"started_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"toolchain":{k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},"phases":[],"game_executed":False,"mod_DLL_executed":False,"host_built":False,"hooks":0,"personalization":False}
def run(label,args):
 start=time.monotonic();p=subprocess.run(args,capture_output=True,text=True);text=p.stdout+p.stderr
 for value,tag in [(str(WORK),"<WORK>"),(str(REPO),"<CHECKOUT>"),(os.environ.get("USERPROFILE",""),"<RUNNER>")]:
  if value:text=text.replace(value,tag).replace(value.replace("\\","/"),tag)
 (EVIDENCE/(label+".log")).write_text(text,encoding="utf-8")
 r["phases"].append({"label":label,"exit":p.returncode,"seconds":round(time.monotonic()-start,3)})
 print(label+": exit="+str(p.returncode),flush=True)
 if p.returncode:print(text[-12000:],flush=True);raise RuntimeError(label)
 return text
try:
 if os.name!="nt" or os.environ.get("VSCMD_ARG_TGT_ARCH")!="x64":raise RuntimeError("MSVC_AMD64_REQUIRED")
 official=REPO/".official"
 if subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()!="40457c6adb381928e4b5fef6ed459ed291edd5e2":raise RuntimeError("SDK_ROOT_DRIFT")
 code="\n".join(p.read_text() for p in (ROOT/"src").glob("*"))
 for forbidden in ["HookService","DEFINE_HOOK","ActorService","TextureService","mPlayerRange","->health","GXCall","register_edit","VirtualProtect","WriteProcessMemory"]:
  if forbidden in code:raise RuntimeError("FORBIDDEN_SOURCE_SURFACE "+forbidden)
 if 'ModResult update() const noexcept {return MOD_OK;}' not in code:raise RuntimeError("FRAME_CALLBACK_CHANGED")
 # Unchanged native records from the tested A/B path; no game resources needed.
 if (ROOT/"src/native_records.hpp").read_bytes()!=(REPO/"DEVF/CP06_NATIVE_SPAWN_QA/src/native_records.hpp").read_bytes():raise RuntimeError("NATIVE_RECORD_SOURCE_DRIFT")
 dependencies=["DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci/audit.py","DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci/reviewed-imports.json"]
 paths=[p for p in ROOT.rglob("*") if p.is_file() and not any(s in p.parts for s in ["__pycache__","evidence"]) and p.name not in ["SOURCE_CI.json","download-spec.json"]]+[REPO/p for p in dependencies]
 ledger={p.relative_to(REPO).as_posix():sha(p.read_bytes().replace(b"\r\n",b"\n")) for p in paths};r["source_ledger"]=ledger
 version=WORK/"compiler_only.cpp";version.write_text("int compiler_only;\n")
 run("compiler",["cl","/Bv","/c",str(version),"/Fo"+str(WORK/"compiler.obj")])
 run("configure",["cmake","-S",str(ROOT),"-B",str(BUILD),"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDEVF_SOURCE_SHA="+source])
 run("build",["cmake","--build",str(BUILD),"--target","devf_cp07_1_editor","native_config_lab","editor_lab","--parallel","2"])
 lab=run("config-io-registration-lab",[str(BUILD/"native_config_lab.exe"),str(ROOT/"examples"),str(WORK/"fake-data-dir")])
 m=re.search(r"JSON_DUAL_NATIVE_LAB_PASS cases=(\d+)",lab)
 if not m or int(m[1])<100:raise RuntimeError("LAB_CASES_INCOMPLETE")
 r["offline_cases"]=int(m[1]);r["case_names"]=re.findall(r"CASE_PASS ([^\r\n]+)",lab)
 if len(r["case_names"])!=r["offline_cases"]:raise RuntimeError("LAB_CASE_LEDGER")
 expected={k:struct.pack(">8sIfffhhhH",b"E_oc",0xFFFF0100,-1250.,y,z,0x0106,0,0,0xFFFF).hex() for k,y,z in [("NORTH",-6322.6005859375,61950.),("SITE1",-6304.00830078125,60750.)]}
 for label,h in expected.items():
  if "RECORD_"+label+"="+h not in lab:raise RuntimeError("NATIVE_RECORD_BYTES")
 editor_lab=run("editor-persistence-ui-lab",[str(BUILD/"editor_lab.exe"),str(ROOT/"examples"),str(WORK/"editor-lab-data")])
 editor_match=re.search(r"CP07_EDITOR_LAB_PASS cases=(\d+)",editor_lab)
 if not editor_match or int(editor_match[1])<50:raise RuntimeError("EDITOR_CASES_INCOMPLETE")
 r["editor_cases"]=int(editor_match[1]);r["editor_case_names"]=re.findall(r"CASE_PASS ([^\r\n]+)",editor_lab)
 if len(r["editor_case_names"])!=r["editor_cases"]:raise RuntimeError("EDITOR_CASE_LEDGER")
 r["record_hex"]=expected
 dll=BUILD/"devf_cp07_1_editor.dll"
 for what in ["headers","imports","exports"]:run(what,["dumpbin","/"+what,str(dll)])
 pe=inspect_pe(dll);negative=negatives(dll,WORK)
 if len(negative)!=11:raise RuntimeError("NEGATIVES_INCOMPLETE")
 before=dll.read_bytes()
 run("official-package",["cmake","--build",str(BUILD),"--target","devf_cp07_1_editor_package"])
 if dll.read_bytes()!=before:raise RuntimeError("PACKAGING_REBUILT_DLL")
 pkg=BUILD/"mods/devf_cp07_1_editor.dusk";audit=verify_package(pkg,dll)
 name="DEVF_CP07_1_NATIVE_EDITOR_windows-amd64_"+source+".dusk";shutil.copyfile(pkg,DELIVERY/name)
 manifest={"id":"dev.devf.cp06_native_config_dual","version":"0.7.0","filename":name,"source_commit":source,"ci_commit":r["ci_head"],"build_run":r["run_id"],"stage":"F_SP121","room":3,"layer":6,"native_records":"CONFIGURED_0_TO_2","health_color_AI":"NATIVE_UNMODIFIED","hooks":0,"JSON_required":True,"JSON_read":"COLD_START_ONLY","editor":"UiService2.3 optional menu/window, verified backup before save","status":"CP07_EDITOR_QA_BUILD_PASS_DOWNLOAD_PENDING","human_gate":"HUMAN_RUNTIME_PENDING",**{k:audit[k] for k in ["package_bytes","package_sha256","dll_bytes","dll_sha256","manifest_sha256"]}}
 (DELIVERY/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
 examples={}
 for p in sorted((ROOT/"examples").glob("*.json")):
  rel="examples/"+p.name;dest=DELIVERY/rel;dest.parent.mkdir(exist_ok=True);shutil.copyfile(p,dest);examples[rel]={"bytes":p.stat().st_size,"sha256":sha(p.read_bytes())}
 for doc in ["HUMAN_TEST.md","README.md"]:shutil.copyfile(ROOT/doc,DELIVERY/doc)
 checks={name:sha(pkg.read_bytes()),**{p:e["sha256"] for p,e in examples.items()}}
 (DELIVERY/"SHA256SUMS.txt").write_text("".join(digest+"  "+p+"\n" for p,digest in checks.items()))
 with zipfile.ZipFile(DELIVERY/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
  for rel in ledger:z.write(REPO/rel,rel)
  z.write(ROOT/"SOURCE_CI.json",ROOT.relative_to(REPO).as_posix()+"/SOURCE_CI.json")
 r["manifest"]=manifest;r["audit"]=audit;r["negatives"]=negative;r["examples"]=examples
 r["source_zip_bytes"]=(DELIVERY/"SOURCE_COMPILED.zip").stat().st_size;r["source_zip_sha256"]=sha((DELIVERY/"SOURCE_COMPILED.zip").read_bytes())
 r["status"]="CP07_EDITOR_BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING"
except Exception as e:
 r["error"]=str(e);print("CP07_EDITOR_ERROR="+str(e),flush=True);raise
finally:
 (EVIDENCE/"result.json").write_text(json.dumps(r,indent=2)+"\n")
 print("CP07_EDITOR_BUILD_RESULT="+json.dumps(r,separators=(",",":")),flush=True)
