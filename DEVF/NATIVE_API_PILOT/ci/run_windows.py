"""MSVC two separate modules, real shared core/API tests, no host/game/DLL execution."""
import datetime,hashlib,json,os,re,shutil,struct,subprocess,time,zipfile
from pathlib import Path
from audit_api import inspect_pe,verify_package,negatives,sha
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1]
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-api";BUILD=WORK/"build";EVIDENCE=WORK/"evidence"
for p in [WORK,BUILD,EVIDENCE,WORK/"PROVIDER",WORK/"PROBE"]:p.mkdir(parents=True,exist_ok=True)
source=json.loads((ROOT/"SOURCE_CI.json").read_text())["source_commit"]
r={"status":"BLOCKED","source_commit":source,"ci_head":subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),"event_merge_sha":os.environ.get("GITHUB_SHA"),"run_id":os.environ["GITHUB_RUN_ID"],"started_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"toolchain":{k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},"phases":[],"game_executed":False,"mod_DLL_executed":False,"host_built":False}
def run(label,args):
 start=time.monotonic();p=subprocess.run(args,capture_output=True,text=True);text=p.stdout+p.stderr
 for value,tag in [(str(WORK),"<WORK>"),(str(REPO),"<CHECKOUT>"),(os.environ.get("USERPROFILE",""),"<RUNNER>")]:
  if value:text=text.replace(value,tag).replace(value.replace("\\","/"),tag)
 (EVIDENCE/(label+".log")).write_text(text,encoding="utf-8")
 r["phases"].append({"label":label,"exit":p.returncode,"seconds":round(time.monotonic()-start,3)})
 print(label+": exit="+str(p.returncode),flush=True)
 if p.returncode:print(text[-16000:],flush=True);raise RuntimeError(label)
 return text
try:
 if os.name!="nt" or os.environ.get("VSCMD_ARG_TGT_ARCH")!="x64":raise RuntimeError("MSVC_AMD64_REQUIRED")
 official=REPO/".official"
 if subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()!="40457c6adb381928e4b5fef6ed459ed291edd5e2":raise RuntimeError("SDK_DRIFT")
 code="\n".join(p.read_text() for p in (ROOT/"provider/src").glob("*"))
 consumer="\n".join(p.read_text() for p in (ROOT/"consumer/src").glob("*"))
 for token in ["HookService","DEFINE_HOOK","ActorService","TextureService","mPlayerRange","->health","GXCall","register_edit","VirtualProtect","WriteProcessMemory"]:
  if token in code or token in consumer:raise RuntimeError("FORBIDDEN_SURFACE "+token)
 if "StageService" in consumer or "add_actor" in consumer:raise RuntimeError("CONSUMER_SPAWN_AUTHORITY")
 old=REPO/"DEVF/CP07_1_NATIVE_EDITOR"
 copied=["src/config_core.hpp","src/config_io.hpp","src/native_records.hpp","src/registration.hpp","src/runtime.hpp","src/persistence.hpp","src/editor_ui.hpp","tests/config_lab.cpp","tests/registration_cases.hpp","tests/editor_lab.cpp"]
 for p in copied:
  if (ROOT/"provider"/p).read_bytes()!=(old/p).read_bytes():raise RuntimeError("CP07_COPY_DRIFT "+p)
 dependencies=["DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci/audit.py","DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci/reviewed-imports.json"]
 paths=[p for p in ROOT.rglob("*") if p.is_file() and not any(s in p.parts for s in ["__pycache__","evidence"]) and p.name not in ["SOURCE_CI.json","download-spec.json"]]+[REPO/p for p in dependencies]
 ledger={p.relative_to(REPO).as_posix():sha(p.read_bytes().replace(b"\r\n",b"\n")) for p in paths};r["source_ledger"]=ledger
 version=WORK/"compiler_only.cpp";version.write_text("int compiler_only;\n")
 run("compiler",["cl","/Bv","/c",str(version),"/Fo"+str(WORK/"compiler.obj")])
 run("configure",["cmake","-S",str(ROOT),"-B",str(BUILD),"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDEVF_SOURCE_SHA="+source])
 run("build",["cmake","--build",str(BUILD),"--target","devf_native_api_provider","devf_native_api_probe","native_config_lab","editor_lab","api_lab","api_c_abi","--parallel","2"])
 base=run("core109",[str(BUILD/"native_config_lab.exe"),str(ROOT/"examples"),str(WORK/"core-data")])
 editor=run("editor55",[str(BUILD/"editor_lab.exe"),str(ROOT/"examples"),str(WORK/"editor-data")])
 api=run("api-owner-lifecycle",[str(BUILD/"api_lab.exe"),str(ROOT/"examples")])
 for key,text,marker,minimum in [("core",base,"JSON_DUAL_NATIVE_LAB_PASS",109),("editor",editor,"CP07_EDITOR_LAB_PASS",55),("api",api,"DEVF_API_LAB_PASS",45)]:
  m=re.search(marker+r" cases=(\d+)",text)
  if not m or int(m[1])<minimum:raise RuntimeError("LAB_INCOMPLETE "+key)
  names=re.findall(r"CASE_PASS ([^\r\n]+)",text)
  if len(names)!=int(m[1]):raise RuntimeError("LAB_LEDGER "+key)
  r[key+"_cases"]=int(m[1]);r[key+"_case_names"]=names
 r["C_ABI_compile"]="PASS MSVC C11 and C++20";r["loader_execution"]="SOURCE_CONTRACT_AND_FAKE_ONLY_NOT_REAL_HOST"
 expected={k:struct.pack(">8sIfffhhhH",b"E_oc",0xFFFF0100,-1250.,y,z,0x0106,0,0,0xFFFF).hex() for k,y,z in [("NORTH",-6322.6005859375,61950.),("SITE1",-6304.00830078125,60750.)]}
 for label,h in expected.items():
  if "RECORD_"+label+"="+h not in base:raise RuntimeError("RECORD_BYTES")
 r["record_hex"]=expected;r["packages"]={}
 for mode in ["provider","probe"]:
  target="devf_native_api_"+mode;dll=BUILD/(target+".dll")
  for what in ["headers","imports","exports"]:run(mode+"-"+what,["dumpbin","/"+what,str(dll)])
  negative=negatives(dll,WORK,mode);before=dll.read_bytes()
  run(mode+"-official-package",["cmake","--build",str(BUILD),"--target",target+"_package"])
  if dll.read_bytes()!=before:raise RuntimeError("REBUILT_DURING_PACKAGE")
  pkg=BUILD/("mods/"+target+".dusk");audit=verify_package(pkg,dll,mode)
  dest=WORK/mode.upper();name="DEVF_NATIVE_API_"+mode.upper()+"_windows-amd64_"+source+".dusk";shutil.copyfile(pkg,dest/name)
  manifest={"id":"dev.devf.cp06_native_config_dual" if mode=="provider" else "dev.devf.native_api_probe","version":"0.7.1" if mode=="provider" else "0.1.0","role":mode,"filename":name,"source_commit":source,"ci_commit":r["ci_head"],"build_run":r["run_id"],"api":"dev.devf.native_placement@1.0","scope":"NATIVE_STAGE_EDIT_ONLY","status":"API_BUILD_PASS_DOWNLOAD_PENDING","human":"HUMAN_PENDING",**{k:audit[k] for k in ["package_bytes","package_sha256","dll_bytes","dll_sha256","manifest_sha256"]}}
  (dest/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
  for doc in ["README.md","HUMAN_TEST.md"]:shutil.copyfile(ROOT/doc,dest/doc)
  examples={}
  if mode=="provider":
   for p in (ROOT/"examples").glob("*.json"):
    rel="examples/"+p.name;(dest/"examples").mkdir(exist_ok=True);shutil.copyfile(p,dest/rel);examples[rel]={"bytes":p.stat().st_size,"sha256":sha(p.read_bytes())}
  checks={name:sha(pkg.read_bytes()),**{p:e["sha256"] for p,e in examples.items()}}
  (dest/"SHA256SUMS.txt").write_text("".join(h+"  "+p+"\n" for p,h in checks.items()))
  with zipfile.ZipFile(dest/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
   for rel in ledger:z.write(REPO/rel,rel)
   z.write(ROOT/"SOURCE_CI.json",ROOT.relative_to(REPO).as_posix()+"/SOURCE_CI.json")
  r["packages"][mode]={"manifest":manifest,"audit":audit,"negatives":negative,"examples":examples,"source_zip_bytes":(dest/"SOURCE_COMPILED.zip").stat().st_size,"source_zip_sha256":sha((dest/"SOURCE_COMPILED.zip").read_bytes())}
 r["status"]="DEVF_API_PAIR_BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING"
except Exception as e:
 r["error"]=str(e);print("API_BUILD_ERROR="+str(e),flush=True);raise
finally:
 (EVIDENCE/"result.json").write_text(json.dumps(r,indent=2)+"\n")
 print("DEVF_API_BUILD_RESULT="+json.dumps(r,separators=(",",":")),flush=True)
