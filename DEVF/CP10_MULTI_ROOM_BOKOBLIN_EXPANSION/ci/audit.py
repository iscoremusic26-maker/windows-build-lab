import importlib.util,json,zipfile,struct,io,hashlib
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];REPO=TASK.parents[1]
OLD=REPO/"DEVF/CP07_2B_BOKOBLIN_EXPANSION_PILOT"
spec=importlib.util.spec_from_file_location("prior_api_audit",OLD/"ci/audit_api.py")
old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha=old.sha
def inspect_pe(path,mode="provider"):
 b=path.read_bytes();pe,opt,offset=old.pe_parts(b)
 def u32(n):return struct.unpack_from("<I",b,n)[0]
 def cstr(n):return b[n:b.index(b"\0",n)].decode("ascii")
 imports={};n=offset(u32(opt+120))
 while any(b[n:n+20]):
  ilt,_,_,name,iat=struct.unpack_from("<5I",b,n);dll=cstr(offset(name));syms=[];q=offset(ilt or iat)
  while (v:=struct.unpack_from("<Q",b,q)[0]):
   if v>>63:raise RuntimeError("Ordinal import")
   syms.append(cstr(offset(v)+2));q+=8
  imports[dll]=sorted(syms);n+=20
 allowed=json.loads((old.BASE/"reviewed-imports.json").read_text())
 delta=json.loads((OLD/"ci/reviewed-imports-delta.json").read_text())["imports"]
 if delta!={"api-ms-win-crt-string-l1-1-0.dll":["strcmp","strlen","strncmp"]}:raise RuntimeError("Unexpected CRT delta")
 for module,names in delta.items():allowed[module]=sorted(set(allowed.get(module,[]))|set(names))
 if mode=="provider":
  allowed["dusklight.exe"]=json.loads((OLD/"ci/native-read-imports.json").read_text())["dusklight.exe"]
 else:
  if "dusklight.exe" in imports:raise RuntimeError("Probe game import forbidden")
 # CP10 uses std::hypot(float,float) for the existing 300-unit separation checks.
 # MSVC emits UCRT _hypotf; permit only this named math import, no wildcard.
 allowed["api-ms-win-crt-math-l1-1-0.dll"]=sorted(set(allowed.get("api-ms-win-crt-math-l1-1-0.dll",[]))|{"_hypotf"})
 for dll,syms in imports.items():
  if dll not in allowed or len(syms)!=len(set(syms)) or any(s not in allowed[dll] for s in syms):raise RuntimeError("UNREVIEWED_CRT_IMPORTS "+json.dumps(imports,sort_keys=True))
 for directory in [9,13]:
  if any(struct.unpack_from("<II",b,opt+112+8*directory)):raise RuntimeError("TLS/delay imports")
 sections=[]
 for i in range(struct.unpack_from("<H",b,pe+6)[0]):
  at=opt+struct.unpack_from("<H",b,pe+20)[0]+40*i
  sections.append((b[at:at+8].rstrip(b"\0").decode(),*struct.unpack_from("<4I",b,at+8)))
 meta=next(s for s in sections if s[0]=="modmeta")
 _,vs,vr,rs,n=meta;end=n+min(vs,rs);records=[]
 while n+8<=end:
  if not any(b[n:n+8]):n+=8;continue
  size,kind,flags=struct.unpack_from("<HBB",b,n)
  if size<8 or size%8 or n+size>end:raise RuntimeError("Invalid metadata")
  if kind==1:
   if size!=8 or flags or u32(n+4)!=1:raise RuntimeError("Module ABI")
   records.append({"kind":"header","abi":1})
  elif kind==2:
   major,minor=struct.unpack_from("<HH",b,n+4)
   if size!=80 or flags not in [0,1]:raise RuntimeError("Service record")
   records.append({"kind":"import","id":cstr(n+16),"major":major,"minor":minor,"optional":bool(flags)})
  elif kind==3:
   major,minor=struct.unpack_from("<HH",b,n+4)
   if size!=80 or flags!=1 or struct.unpack_from("<Q",b,n+8)[0]!=0:raise RuntimeError("Deferred export contract")
   records.append({"kind":"export","id":cstr(n+16),"major":major,"minor":minor,"deferred":True})
  else:raise RuntimeError("Hook metadata forbidden")
  n+=size
 host="dev.twilitrealm.dusklight."
 api="dev.devf.native_placement"
 expected={(host+s,a,c) for s,a,c in ([("host",2,2),("file",1,0),("log",1,0),("stage",1,0),("ui",2,3),("game",2,0)] if mode=="provider" else [("host",2,2),("file",1,0),("log",1,0)] if mode=="expansion" else [("host",2,2),("log",1,0)])}
 if mode=="probe":expected.add((api,1,0))
 if mode=="expansion":expected.add(("dev.devf.encounter_placement",1,0))
 if {(r["id"],r["major"],r["minor"]) for r in records if r["kind"]=="import"}!=expected:raise RuntimeError("Unexpected imports")
 exports=[r for r in records if r["kind"]=="export"]
 if mode=="provider":
  if exports!=[{"kind":"export","id":api,"major":1,"minor":0,"deferred":True},{"kind":"export","id":"dev.devf.encounter_placement","major":1,"minor":0,"deferred":True}]:raise RuntimeError("Missing exact deferred API export")
 elif mode in ["probe","expansion"]:
  if exports:raise RuntimeError("Consumer must not export")
 else:raise RuntimeError("Unknown role")
 if len(records)!=1+len(expected)+len(exports):raise RuntimeError("Duplicate metadata")
 if any(r["optional"]!=(mode=="provider" and r["id"]==host+"ui") for r in records if r["kind"]=="import"):raise RuntimeError("Required dependency/optional UI flags")
 return {"status":"DEVF_NATIVE_API_PE_ABI_PASS","role":mode,"machine":"AMD64","imports":imports,"metadata":records,"exports":old.old.pe_exports(path),"hooks":0,"game_imports":len(imports.get("dusklight.exe",[])),"tls_or_delay_imports":False,"DLL_executed":False}
def verify_package(package,dll,mode):
 with zipfile.ZipFile(package) as z:
  members=old.safe_members(z);assert z.testzip() is None
  manifest=z.read("mod.json");inside=z.read("lib/windows-amd64/mod.dll")
 assert manifest==(TASK/mode/"mod.json").read_bytes() and inside==dll.read_bytes()
 m=json.loads(manifest);assert (m["id"],m["version"])==(("dev.devf.cp06_native_config_dual","0.10.0") if mode=="provider" else ("dev.bokoblin.expansion","0.3.0"))
 return dict(bytes=package.stat().st_size,sha256=sha(package.read_bytes()),dll_bytes=len(inside),dll_sha256=sha(inside),manifest_sha256=sha(manifest),pe=inspect_pe(dll,mode))
def app_pe(path):
 b=path.read_bytes();assert b[:2]==b"MZ";p=struct.unpack_from("<I",b,60)[0]
 assert b[p:p+4]==b"PE\0\0" and struct.unpack_from("<H",b,p+4)[0]==0x8664
 assert struct.unpack_from("<H",b,p+24)[0]==0x20b
 return dict(machine="AMD64",bytes=len(b),sha256=sha(b),self_contained=True)

def safezip(b):
 z=zipfile.ZipFile(io.BytesIO(b));names=z.namelist()
 assert len(names)==len(set(names)) and z.testzip() is None
 assert all(not n.startswith(("/","\\")) and "\\" not in n and ".." not in n.split("/") for n in names)
 assert sum(i.file_size for i in z.infolist())<500_000_000
 return z

def negatives(dll,work,mode):
 # Check the positive binary first so a whitelist failure cannot fake all negatives.
 inspect_pe(dll,mode)
 old.inspect_pe=inspect_pe
 rows=old.negatives(dll,work,mode)
 raw=dll.read_bytes()
 if b"_hypotf\0" in raw:
  p=work/(mode+"-negative-unreviewed-math.dll");p.write_bytes(raw.replace(b"_hypotf\0",b"_hypotx\0"))
  try:inspect_pe(p,mode)
  except RuntimeError as e:
   if not str(e).startswith("UNREVIEWED_CRT_IMPORTS"):raise
   rows.append({"case":"unreviewed-math-import","rejected":True})
  else:raise RuntimeError("Unreviewed math import survived")
 return rows
