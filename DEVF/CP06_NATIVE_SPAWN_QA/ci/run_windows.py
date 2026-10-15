"""Windows MSVC A/B build; native Stage edits only, no host build/game/DLL execution."""
import datetime,hashlib,io,json,os,re,shutil,struct,subprocess,time,zipfile
from pathlib import Path
from audit_native import inspect_pe,verify_package,negatives,sha
from north_context import run as north_context
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1]
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-native-qa";BUILD=WORK/"build";EVIDENCE=WORK/"evidence"
for p in [WORK,BUILD,EVIDENCE,WORK/"A",WORK/"B"]:p.mkdir(parents=True,exist_ok=True)
source=json.loads((ROOT/"SOURCE_CI.json").read_text())["source_commit"]
r={"status":"BLOCKED","source_commit":source,"ci_head":subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),"event_merge_sha":os.environ.get("GITHUB_SHA"),"run_id":os.environ["GITHUB_RUN_ID"],"started_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"toolchain":{k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},"phases":[],"game_executed":False,"mod_DLL_executed":False,"host_built":False,"personalization":False}
def run(label,args):
 start=time.monotonic();p=subprocess.run(args,capture_output=True,text=True)
 text=p.stdout+p.stderr
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
 code="\n".join((ROOT/"src"/x).read_text() for x in ["mod.cpp","registration.hpp","native_records.hpp"])
 for forbidden in ["HookService","DEFINE_HOOK","ActorService","TextureService","mPlayerRange","->health","GXCall","register_edit","VirtualProtect","WriteProcessMemory"]:
  if forbidden in code:raise RuntimeError("FORBIDDEN_SOURCE_SURFACE "+forbidden)
 mod=(ROOT/"src/mod.cpp").read_text()
 if 'MOD_EXPORT ModResult mod_update(ModError*){return MOD_OK;}' not in mod:raise RuntimeError("FRAME_CALLBACK_CHANGED")
 r["north_context"]=north_context()
 dependencies=["DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci/audit.py","DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci/reviewed-imports.json","DEVF/CP01/analysis/analyze_location.py","DEVF/CP01/analysis/location_geometry.py","DEVF/CP01/analysis/location_details.py"]
 paths=[p for p in ROOT.rglob("*") if p.is_file() and not any(s in p.parts for s in ["__pycache__","evidence"]) and p.name not in ["SOURCE_CI.json","download-spec.json"]]+[REPO/p for p in dependencies]
 ledger={p.relative_to(REPO).as_posix():sha(p.read_bytes().replace(b"\r\n",b"\n")) for p in paths}
 r["source_ledger"]=ledger
 version=WORK/"compiler_only.cpp";version.write_text("int compiler_only;\n")
 run("compiler",["cl","/Bv","/c",str(version),"/Fo"+str(WORK/"compiler.obj")])
 run("configure",["cmake","-S",str(ROOT),"-B",str(BUILD),"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDEVF_SOURCE_SHA="+source])
 run("build",["cmake","--build",str(BUILD),"--target","devf_native_qa_north","devf_native_qa_dual","native_registration_lab","--parallel","2"])
 lab=run("registration-lab",[str(BUILD/"native_registration_lab.exe")])
 if "NATIVE_QA_LAB_PASS cases=24" not in lab:raise RuntimeError("LAB_CASES_INCOMPLETE")
 r["offline_cases"]=24
 def record_hex(y,z):return struct.pack(">8sIfffhhhH",b"E_oc",0xFFFF0100,-1250.,y,z,0x0106,0,0,0xFFFF).hex()
 expected={"NORTH":record_hex(-6322.600450915229,61950.),"SITE1":record_hex(-6304.00830078125,60750.)}
 for label,hexdata in expected.items():
  if "RECORD_"+label+"="+hexdata not in lab:raise RuntimeError("NATIVE_RECORD_BYTES")
 r["record_hex"]=expected;r["north_spawn_float32"]=struct.unpack(">f",struct.pack(">f",-6322.600450915229))[0]
 r["packages"]={}
 for mode,key in [("north","A"),("dual","B")]:
  dll=BUILD/("devf_native_qa_"+mode+".dll")
  for what in ["headers","imports","exports"]:run(mode+"-"+what,["dumpbin","/"+what,str(dll)])
  pe=inspect_pe(dll);negative=negatives(dll,WORK)
  before=dll.read_bytes()
  run(mode+"-official-package",["cmake","--build",str(BUILD),"--target","devf_native_qa_"+mode+"_package"])
  if dll.read_bytes()!=before:raise RuntimeError("PACKAGING_REBUILT_DLL")
  pkg=BUILD/("mods/devf_native_qa_"+mode+".dusk");audit=verify_package(pkg,dll,mode)
  folder=WORK/key;name="DEVF_CP06_QA_"+key+"_"+mode.upper()+"_windows-amd64_"+source+".dusk"
  shutil.copyfile(pkg,folder/name)
  manifest={"id":"dev.devf.cp06_qa_"+("north_native" if mode=="north" else "dual_native"),"version":"0.6.1","qa":key,"filename":name,"source_commit":source,"ci_commit":r["ci_head"],"build_run":r["run_id"],"stage":"F_SP121","room":3,"layer":6,"native_records":1 if key=="A" else 2,"north_spawn_y":r["north_spawn_float32"],"health_color_AI":"NATIVE_UNMODIFIED","hooks":0,"JSON_required":False,"status":"NORTH_NATIVE_QA_BUILD_READY_DOWNLOAD_PENDING" if key=="A" else "DUAL_NATIVE_QA_BUILT_DOWNLOAD_PENDING","human_gate":"HUMAN_RUNTIME_NOT_TESTED" if key=="A" else "HUMAN_TEST_BLOCKED_UNTIL_NORTH_PASS",**{k:audit[k] for k in ["package_bytes","package_sha256","dll_bytes","dll_sha256","manifest_sha256"]}}
  (folder/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
  (folder/"SHA256SUMS.txt").write_text(sha(pkg.read_bytes())+"  "+name+"\n")
  with zipfile.ZipFile(folder/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
   for rel in ledger:z.write(REPO/rel,rel)
   z.write(ROOT/"SOURCE_CI.json","DEVF/CP06_NATIVE_SPAWN_QA/SOURCE_CI.json")
  r["packages"][key]={"manifest":manifest,"audit":audit,"negatives":negative,"source_zip_bytes":(folder/"SOURCE_COMPILED.zip").stat().st_size,"source_zip_sha256":sha((folder/"SOURCE_COMPILED.zip").read_bytes())}
 r["status"]="A_B_BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING"
except Exception as e:
 r["error"]=str(e);print("NATIVE_QA_ERROR="+str(e),flush=True);raise
finally:
 (EVIDENCE/"result.json").write_text(json.dumps(r,indent=2)+"\n")
 print("NATIVE_QA_BUILD_RESULT="+json.dumps(r,separators=(",",":")),flush=True)
