"""Native MSVC build of new mod only; no host, old mod, DLL load or game execution."""
import datetime,hashlib,json,os,re,shutil,subprocess,time,zipfile
from pathlib import Path
from audit import inspect_pe,verify_package,negatives,sha,safe_members
ROOT=Path(__file__).resolve().parents[1];WORK=Path(os.environ["RUNNER_TEMP"])/"cp044-black";EVIDENCE=WORK/"evidence";BUILD=WORK/"build";DELIVERY=WORK/"delivery"
for p in [EVIDENCE,BUILD,DELIVERY]:p.mkdir(parents=True,exist_ok=True)
r={"status":"BLOCKED","source_commit":json.loads((ROOT/"SOURCE_CI.json").read_text())["source_commit"],"ci_head":os.environ.get("DEVF_CI_HEAD"),"run_id":os.environ.get("GITHUB_RUN_ID"),"started_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"toolchain":{k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]},"phases":[],"game_executed":False,"DLL_loaded":False,"host_or_CP03_built":False,"runtime":"HUMAN_RUNTIME_NOT_TESTED"}
def run(label,args):
    start=time.monotonic();p=subprocess.run(args,capture_output=True,text=True)
    text=p.stdout+p.stderr
    # Avoid runner/user paths in published build evidence.
    for value,tag in [(str(WORK),"<WORK>"),(str(Path.cwd()),"<CHECKOUT>"),(os.environ.get("USERPROFILE",""),"<RUNNER>")]:
        if value:text=text.replace(value,tag).replace(value.replace("\\","/"),tag)
    (EVIDENCE/(label+".log")).write_text(text,encoding="utf-8")
    r["phases"].append({"label":label,"exit":p.returncode,"seconds":round(time.monotonic()-start,3),"command":[str(x).replace(str(WORK),"<WORK>").replace(str(Path.cwd()),"<CHECKOUT>") for x in args]})
    print(label+": exit="+str(p.returncode),flush=True)
    if p.returncode:print(text[-10000:],flush=True);raise RuntimeError(label)
try:
    if os.name!="nt" or os.environ.get("VSCMD_ARG_TGT_ARCH")!="x64":raise RuntimeError("MSVC Windows AMD64 required")
    official=Path(".official").resolve()
    if subprocess.check_output(["git","-C",str(official),"rev-parse","HEAD"],text=True).strip()!="40457c6adb381928e4b5fef6ed459ed291edd5e2":raise RuntimeError("Official root drift")
    if subprocess.check_output(["git","-C",str(official/"extern/aurora"),"rev-parse","HEAD"],text=True).strip()!="3227d76c60e1e782ca576610bce61c9e7744d8be":raise RuntimeError("Aurora drift")
    source=(ROOT/"src/mod.cpp").read_text()
    if re.findall(r"->\w+\s*=(?!=)",source)!=["->health="]:raise RuntimeError("New native object field assignment")
    for forbidden in ["TextureService","ParticleAccess","VirtualProtect","WriteProcessMemory","CreateThread","funchook","std::thread","new J3D"]:
        if forbidden in source:raise RuntimeError("Forbidden new surface")
    if source.count("a->health=devf::kHp;")!=1 or "field_0x560=" in source:raise RuntimeError("HP write policy changed")
    for name,expected in json.loads((ROOT/"reused-cp03.json").read_text()).items():
        f=ROOT.parent/"CP03_HP_DIAGNOSTIC/src"/name
        if sha(f.read_bytes().replace(b"\r\n",b"\n"))!=expected:raise RuntimeError("Immutable CP03 header changed")
    r["source_ledger"]={f.relative_to(ROOT).as_posix():sha(f.read_bytes().replace(b"\r\n",b"\n")) for f in ROOT.rglob("*") if f.is_file() and not any(x in f.parts for x in ["evidence","__pycache__"]) and f.name!="SOURCE_CI.json"}
    with zipfile.ZipFile("Dusklight-v2.0.3-win32-x86_64.zip") as z:
        item=z.getinfo("sdk/windows-amd64.lib");lib=z.read(item);host=z.read("dusklight.exe")
    if len(lib)!=9400986 or sha(lib)!="4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4" or item.CRC!=0x70c35731 or sha(host)!="ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049":raise RuntimeError("Private official input drift")
    library=WORK/"windows-amd64.lib";library.write_bytes(lib)
    r["dependencies"]={"official_source":"40457c6adb381928e4b5fef6ed459ed291edd5e2","aurora":"3227d76c60e1e782ca576610bce61c9e7744d8be","library_sha256":sha(lib),"library_bytes":len(lib),"library_crc32":f"{item.CRC:08x}"}
    f=WORK/"version.cpp";f.write_text("int compiler_only;\n")
    run("compiler",["cl","/Bv","/c",str(f),"/Fo"+str(WORK/"version.obj")])
    run("cmake",["cmake","--version"])
    run("configure",["cmake","-S",str(ROOT),"-B",str(BUILD),"-G","Ninja","-DCMAKE_BUILD_TYPE=Release","-DDUSK_SOURCE="+str(official),"-DDUSK_GAME_EXE="+str(library),"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"])
    run("build",["cmake","--build",str(BUILD),"--target","devf_cp04_eoc2_black","cp044_black_lab","--parallel","2"])
    run("focused-lab",[str(BUILD/"cp044_black_lab.exe")])
    if "LAB_PASS cases=20" not in (EVIDENCE/"focused-lab.log").read_text():raise RuntimeError("Focused laboratory incomplete")
    r["offline_cases"]=20
    dll=BUILD/"devf_cp04_eoc2_black.dll"
    for name,arg in [("headers","/headers"),("imports","/imports"),("exports","/exports")]:
        run("module-"+name,["dumpbin",arg,str(dll)])
    r["pe"]=inspect_pe(dll);r["negatives"]=negatives(dll,WORK)
    before=dll.read_bytes()
    run("official-package",["cmake","--build",str(BUILD),"--target","devf_cp04_eoc2_black_package"])
    if dll.read_bytes()!=before:raise RuntimeError("Package rebuilt DLL differently")
    package=BUILD/"mods/devf_cp04_eoc2_black.dusk";r["package"]=verify_package(package,dll,ROOT,WORK)
    name="DEVF_CP04_4_EOC2_BLACK_windows-amd64_"+r["source_commit"]+".dusk";shutil.copyfile(package,DELIVERY/name)
    r["status"]="BUILD_PASS_PACKAGE_VERIFIED_DOWNLOAD_PENDING"
    manifest={"filename":name,"id":"dev.devf.cp04_eoc2_black","version":"0.4.4","source_commit":r["source_commit"],"ci_head":r["ci_head"],"run_id":r["run_id"],"package_bytes":r["package"]["package_bytes"],"package_sha256":r["package"]["package_sha256"],"dll_bytes":r["package"]["dll_bytes"],"dll_sha256":r["package"]["dll_sha256"],"runtime":"HUMAN_RUNTIME_NOT_TESTED","private_test":True,"original_assets_included":False,"install_alone":True,"no_hot_reload":True}
    (DELIVERY/"MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
    (DELIVERY/"SHA256SUMS.txt").write_text(manifest["package_sha256"]+"  "+name+"\n")
    # ZIP embeds only newly compiled own sources and the three unchanged reused headers, never assets.
    with zipfile.ZipFile(DELIVERY/"SOURCE_COMPILED.zip","w",zipfile.ZIP_DEFLATED) as z:
        for f in ROOT.rglob("*"):
            if f.is_file() and not any(x in f.parts for x in ["evidence","__pycache__"]):z.write(f,"DEVF/CP04_4_EOC2_BLACK_CONTINUATION/"+f.relative_to(ROOT).as_posix())
        for name in ["policy.hpp","observer.hpp","registration.hpp"]:
            f=ROOT.parent/"CP03_HP_DIAGNOSTIC/src"/name;z.write(f,"DEVF/CP03_HP_DIAGNOSTIC/src/"+name)
    r["delivery_manifest"]=manifest
except Exception as e:
    r["error"]=str(e);print("CP04_BUILD_ERROR="+str(e),flush=True);raise
finally:
    (EVIDENCE/"result.json").write_text(json.dumps(r,indent=2)+"\n")
    print("CP04_BUILD_RESULT="+json.dumps(r,sort_keys=True),flush=True)
