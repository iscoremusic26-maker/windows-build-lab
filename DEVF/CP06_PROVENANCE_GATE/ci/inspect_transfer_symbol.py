"""CP06 focal: read official PE/symdb only; never load EXE, hook or inspect private STL."""
import os,sys,struct,json,hashlib,zipfile,uuid,bisect,subprocess,shutil,time,urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
WORK=Path(os.environ["RUNNER_TEMP"])/"devf-cp06-provenance-gate";WORK.mkdir(parents=True,exist_ok=True)
EXE_SHA="ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049"
ZIP_SHA="ac63514eeb13cd4e9bd22be02dabee4952d1331a78f0496326bba2b577b12caf"
result={"status":"BLOCKED","kind":"OFFLINE_EXACT_BINARY_SYMBOL_GATE","game_executed":False,"host_loaded":False,"host_modified":False,"host_built":False,"mod_built":False,"hooks_installed":False,"ci_sha":os.environ.get("GITHUB_SHA"),"run_id":os.environ.get("GITHUB_RUN_ID"),"official_source":"40457c6adb381928e4b5fef6ed459ed291edd5e2"}
def digest(b):return hashlib.sha256(b).hexdigest()
def require(ok,why):
    if not ok:raise RuntimeError(why)
def read(d,o,fmt):
    n=struct.calcsize(fmt);require(0<=o<=len(d)-n,"READ_BOUNDS")
    return struct.unpack_from(fmt,d,o)
def fnv(s):
    h=0xcbf29ce484222325
    for b in s.encode("utf-8"):h=((h^b)*0x100000001b3)&0xffffffffffffffff
    return h
def zstd_decode(raw,size):
    try:
        from compression import zstd
        result["decompressor"]={"name":"Python compression.zstd","python":sys.version.split()[0]}
        output=zstd.decompress(raw)
    except ImportError:
        version="0.25.0"
        wheels=WORK/"wheel";wheels.mkdir(exist_ok=True)
        p=subprocess.run([sys.executable,"-m","pip","download","--disable-pip-version-check","--only-binary=:all:","--no-deps","--dest",str(wheels),"zstandard=="+version],capture_output=True,text=True)
        require(p.returncode==0,"PINNED_ZSTANDARD_DOWNLOAD_FAILED")
        paths=list(wheels.glob("zstandard-*.whl"));require(len(paths)==1,"WHEEL_AMBIGUOUS")
        wheel=paths[0];data=wheel.read_bytes()
        meta=json.load(urllib.request.urlopen("https://pypi.org/pypi/zstandard/"+version+"/json",timeout=30))
        candidates=[x for x in meta["urls"] if x["filename"]==wheel.name]
        require(len(candidates)==1 and digest(data)==candidates[0]["digests"]["sha256"],"WHEEL_HASH_MISMATCH")
        p=subprocess.run([sys.executable,"-m","pip","install","--disable-pip-version-check","--no-index","--no-deps",str(wheel)],capture_output=True,text=True)
        require(p.returncode==0,"PINNED_ZSTANDARD_INSTALL_FAILED")
        import zstandard
        require(zstandard.__version__==version,"DECOMPRESSOR_VERSION")
        output=zstandard.ZstdDecompressor().decompress(raw,max_output_size=size)
        result["decompressor"]={"name":"zstandard","version":version,"wheel":wheel.name,"wheel_sha256":digest(data),"hash_source":"PyPI release JSON via TLS"}
    require(len(output)==size,"DECOMPRESSED_LENGTH")
    return output
try:
    require(os.name=="nt","WINDOWS_REQUIRED")
    archive=ROOT/"Dusklight-v2.0.3-win32-x86_64.zip"
    raw_zip=archive.read_bytes()
    require(not raw_zip.startswith(b"version https://git-lfs.github.com"),"LFS_POINTER")
    require(digest(raw_zip)==ZIP_SHA,"ORIGINAL_WINDOWS_ZIP_HASH")
    with zipfile.ZipFile(archive) as z:
        info=z.getinfo("dusklight.exe");exe=z.read(info) # CRC checked by ZipFile
    require(digest(exe)==EXE_SHA,"REFERENCE_EXE_HASH")
    result["inputs"]={"zip_bytes":len(raw_zip),"zip_sha256":digest(raw_zip),"exe_bytes":len(exe),"exe_sha256":digest(exe),"exe_zip_crc32":f"{info.CRC:08x}","crc_checked":True,"origin":"private existing inputs Git LFS; no new user upload"}
    require(exe[:2]==b"MZ","MZ");pe=read(exe,0x3c,"<I")[0]
    require(exe[pe:pe+4]==b"PE\0\0","PE")
    machine,nsections,_,_,_,optional_size,_=read(exe,pe+4,"<HHIIIHH")
    require(machine==0x8664,"NOT_AMD64")
    opt=pe+24;require(read(exe,opt,"<H")[0]==0x20b,"NOT_PE32PLUS")
    sections=[]
    for i in range(nsections):
        off=opt+optional_size+40*i
        name=exe[off:off+8].rstrip(b"\0").decode("ascii")
        virtual_size,rva,raw_size,raw_offset=read(exe,off+8,"<IIII")
        sections.append((name,rva,virtual_size,raw_offset,raw_size))
    def rva_off(rva,count=1):
        for name,start,vs,raw,rs in sections:
            if start<=rva and rva+count<=start+rs:
                require(raw+rva-start+count<=len(exe),"SECTION_FILE_BOUNDS")
                return raw+rva-start
        raise RuntimeError("RVA_NOT_BACKED_BY_FILE")
    desc=[s for s in sections if s[0]==".symdbh"];require(len(desc)==1,"SYMDB_DESCRIPTOR_SECTION")
    _,_,_,descriptor_offset,descriptor_size=desc[0]
    magic,manifest_rva,manifest_size=read(exe,descriptor_offset,"<QQQ")
    require(magic==0x52444842444d5953 and manifest_rva and manifest_size>=72,"DESCRIPTOR_INVALID")
    mo=rva_off(manifest_rva,manifest_size);blob=exe[mo:mo+manifest_size]
    magic,version,compression,usize,csize,buildlen,buildid,count=read(blob,0,"<8sIIQQI32sI")
    require(magic==b"SYMGEN\0\0" and version==2 and compression in [0,1],"HEADER_VERSION")
    require(buildlen<=32 and csize<=len(blob)-72 and usize<=200_000_000,"HEADER_BOUNDS")
    # Exact host build-id logic: RSDS GUID RFC4122 byte order + little-endian age.
    debug_rva,debug_size=read(exe,opt+112+6*8,"<II");debug_off=rva_off(debug_rva,debug_size)
    ids=[]
    for i in range(debug_size//28):
        _,_,_,_,kind,size,addr,ptr=read(exe,debug_off+28*i,"<IIHHIIII")
        if kind==2 and size>=24:
            o=rva_off(addr,size)
            if exe[o:o+4]==b"RSDS":
                ids.append(uuid.UUID(bytes_le=exe[o+4:o+20]).bytes+exe[o+20:o+24])
    require(buildid[:buildlen] in ids,"STALE_SYMBOL_MANIFEST")
    payload=blob[72:72+csize]
    if compression:payload=zstd_decode(payload,usize)
    else:require(len(payload)==usize,"RAW_LENGTH")
    require(count*24<=len(payload),"ENTRY_COUNT")
    strings=payload[count*24:]
    entries=[];hashes=[]
    for i in range(count):
        h,rva,no,flags=read(payload,i*24,"<QQII")
        require(no<len(strings),"NAME_BOUNDS")
        end=strings.find(b"\0",no);require(end>=0,"NAME_TERMINATION")
        name=strings[no:end].decode("utf-8")
        entries.append({"name":name,"hash":h,"rva":rva,"flags":flags});hashes.append(h)
    require(hashes==sorted(hashes),"MANIFEST_NOT_SORTED")
    def resolve(name):
        h=fnv(name);i=bisect.bisect_left(hashes,h)
        while i<len(entries) and entries[i]["hash"]==h:
            e=entries[i]
            if e["name"]==name:
                return {"status":"Ambiguous" if e["flags"]&16 else "Ok","rva":e["rva"],"flags":e["flags"]}
            i+=1
        return {"status":"NotFound"}
    selected=[e for e in entries if ("register_edit" in e["name"] or "stage_add_actor" in e["name"]) and e["flags"]&1]
    require(all(fnv(e["name"])==e["hash"] for e in selected),"TARGET_NAME_HASH")
    undname=shutil.which("undname.exe") or shutil.which("llvm-undname.exe")
    require(undname is not None,"MSVC_DEMANGLER_MISSING")
    records=[]
    for e in selected:
        demangled=None
        if e["name"].startswith("?"):
            p=subprocess.run([undname,e["name"]],capture_output=True,text=True)
            require(p.returncode==0,"UNDECORATE_FAILED")
            demangled=p.stdout.strip()
            # No paths/host code/memory bytes are emitted, only target type names.
        peers=[a for a in entries if a["rva"]==e["rva"] and a["flags"]&1]
        aliases=[a for a in peers if a["flags"]&32]
        flags=e["flags"]
        records.append({"name":e["name"],"demangler_output":demangled,"rva":hex(e["rva"]),"rva_is_static_not_runtime_pointer":True,"flags":flags,"code":bool(flags&1),"local":bool(flags&4),"multi_name":bool(flags&8),"dup_name":bool(flags&16),"inline_sites_at_target":bool(aliases),"flag_inline_sites":bool(flags&32),"resolver":resolve(e["name"]),"names_at_same_rva":len(peers)})
    result["manifest"]={"version":version,"compression":compression,"entries":count,"build_id":buildid[:buildlen].hex(),"identity_matches_rsds":True,"target_records":records,"target_records_count":len(records),"resolver_model":"ported from exact manifest.cpp; binary read only, not in-process HookService invocation"}
    result["toolchain"]={k:os.environ.get(k) for k in ["VCToolsVersion","WindowsSDKVersion","VSCMD_ARG_TGT_ARCH"]}
    result["python"]=sys.version.split()[0]
    result["status"]="EXACT_BINARY_SYMBOL_MEASUREMENT_COMPLETE"
    result["bridge_proved"]=False
    result["P2"]="NO_GO_UNTIL_OWNER_HANDLE_TO_BUFFER_PROVED"
except Exception as e:
    result["error"]=str(e) if isinstance(e,RuntimeError) else type(e).__name__
    raise
finally:
    (WORK/"symbol-result.json").write_text(json.dumps(result,indent=2)+"\n")
    print("CP06_PROVENANCE_SYMBOL_RESULT="+json.dumps(result,separators=(",",":")),flush=True)
