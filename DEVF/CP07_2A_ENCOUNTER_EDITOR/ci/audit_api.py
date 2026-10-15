"""Strict StageService-only QA audit; reuse unchanged baseline PE/ZIP readers."""
import hashlib,importlib.util,json,struct,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1]
BASE=REPO/"DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci"
spec=importlib.util.spec_from_file_location("baseline_reader",BASE/"audit.py");old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha=old.sha;safe_members=old.safe_members;pe_parts=old.pe_parts
def inspect_pe(path,mode="provider"):
 b=path.read_bytes();pe,opt,offset=pe_parts(b)
 def u32(n):return struct.unpack_from("<I",b,n)[0]
 def cstr(n):return b[n:b.index(b"\0",n)].decode("ascii")
 imports={};n=offset(u32(opt+120))
 while any(b[n:n+20]):
  ilt,_,_,name,iat=struct.unpack_from("<5I",b,n);dll=cstr(offset(name));syms=[];q=offset(ilt or iat)
  while (v:=struct.unpack_from("<Q",b,q)[0]):
   if v>>63:raise RuntimeError("Ordinal import")
   syms.append(cstr(offset(v)+2));q+=8
  imports[dll]=sorted(syms);n+=20
 allowed=json.loads((BASE/"reviewed-imports.json").read_text())
 delta=json.loads((ROOT/"ci/reviewed-imports-delta.json").read_text())["imports"]
 if delta!={"api-ms-win-crt-string-l1-1-0.dll":["strcmp","strlen","strncmp"]}:raise RuntimeError("Unexpected CRT delta")
 for module,names in delta.items():allowed[module]=sorted(set(allowed.get(module,[]))|set(names))
 if mode=="provider":
  allowed["dusklight.exe"]=json.loads((ROOT/"ci/native-read-imports.json").read_text())["dusklight.exe"]
 else:
  if "dusklight.exe" in imports:raise RuntimeError("Probe game import forbidden")
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
 expected={(host+s,a,c) for s,a,c in ([("host",2,2),("file",1,0),("log",1,0),("stage",1,0),("ui",2,3),("game",2,0)] if mode=="provider" else [("host",2,2),("log",1,0)])}
 if mode=="probe":expected.add((api,1,0))
 if {(r["id"],r["major"],r["minor"]) for r in records if r["kind"]=="import"}!=expected:raise RuntimeError("Unexpected imports")
 exports=[r for r in records if r["kind"]=="export"]
 if mode=="provider":
  if exports!=[{"kind":"export","id":api,"major":1,"minor":0,"deferred":True}]:raise RuntimeError("Missing exact deferred API export")
 elif mode=="probe":
  if exports:raise RuntimeError("Consumer must not export")
 else:raise RuntimeError("Unknown role")
 if len(records)!=1+len(expected)+len(exports):raise RuntimeError("Duplicate metadata")
 if any(r["optional"]!=(mode=="provider" and r["id"]==host+"ui") for r in records if r["kind"]=="import"):raise RuntimeError("Required dependency/optional UI flags")
 return {"status":"DEVF_NATIVE_API_PE_ABI_PASS","role":mode,"machine":"AMD64","imports":imports,"metadata":records,"exports":old.pe_exports(path),"hooks":0,"game_imports":len(imports.get("dusklight.exe",[])),"tls_or_delay_imports":False,"DLL_executed":False}
def verify_package(package,dll,mode):
 with zipfile.ZipFile(package) as z:
  members=safe_members(z);manifest=z.read("mod.json");inside=z.read("lib/windows-amd64/mod.dll")
 expected=ROOT/("provider" if mode=="provider" else "consumer")/"mod.json"
 if manifest!=expected.read_bytes() or inside!=dll.read_bytes():raise RuntimeError("Package identity")
 parsed=json.loads(manifest)
 identity=("dev.devf.cp06_native_config_dual","0.7.2") if mode=="provider" else ("dev.devf.native_api_probe","0.1.0")
 if (parsed["id"],parsed["version"])!=identity:raise RuntimeError("Mod identity/version")
 return {"status":"PACKAGE_VERIFIED","package_bytes":package.stat().st_size,"package_sha256":sha(package.read_bytes()),"dll_bytes":len(inside),"dll_sha256":sha(inside),"manifest_sha256":sha(manifest),"members":members,"pe":inspect_pe(dll,mode)}
def metadata_offsets(raw):
 pe,opt,_=pe_parts(raw)
 for i in range(struct.unpack_from("<H",raw,pe+6)[0]):
  q=opt+struct.unpack_from("<H",raw,pe+20)[0]+40*i
  if raw[q:q+8].rstrip(b"\0")==b"modmeta":vs,vr,rs,n=struct.unpack_from("<4I",raw,q+8);break
 end=n+min(vs,rs);found=[]
 while n+8<=end:
  if not any(raw[n:n+8]):n+=8;continue
  size,kind=struct.unpack_from("<HB",raw,n)
  found.append((n,kind,raw[n+16:n+size].split(b"\0",1)[0] if kind in [2,3] else b""));n+=size
 return found
def negatives(dll,work,mode):
 raw=dll.read_bytes();pe,opt,_=pe_parts(raw);rows=[]
 def reject(name,mut):
  p=work/(mode+"-negative-"+name+".dll");p.write_bytes(mut)
  try:inspect_pe(p,mode)
  except (RuntimeError,ValueError,StopIteration,struct.error):rows.append({"case":name,"rejected":True})
  else:raise RuntimeError("Negative survived "+name)
 m=bytearray(raw);struct.pack_into("<H",m,pe+4,0x14c);reject("x86",m)
 reject("missing_export",raw.replace(b"mod_update",b"mod_updatX"))
 offsets=metadata_offsets(raw)
 for role in (["host","file","log","stage","ui","game"] if mode=="provider" else ["host","log"]):
  target=("dev.twilitrealm.dusklight."+role).encode();at=next(n for n,k,s in offsets if k==2 and s==target)
  m=bytearray(raw);struct.pack_into("<H",m,at+4,9);reject("wrong_"+role+"_major",m)
 target=b"dev.devf.native_placement";at=next(n for n,k,s in offsets if k==(3 if mode=="provider" else 2) and s==target)
 m=bytearray(raw);struct.pack_into("<H",m,at+4,2);reject("wrong_api_major",m)
 m=bytearray(raw);m[at+3]=0 if mode=="provider" else 1;reject("api_deferred_or_required_flag",m)
 m=bytearray(raw);m[at+16]=ord("x");reject("foreign_api_id",m)
 if mode=="provider":
  m=bytearray(raw);struct.pack_into("<Q",m,at+8,1);reject("non_null_deferred_pointer",m)
 for name,entries in [("extra_asset",[("mod.json",b"{}"),("lib/windows-amd64/mod.dll",raw),("E_oc2.arc",b"not-original")]),("traversal",[("../mod.json",b"{}"),("lib/windows-amd64/mod.dll",raw)])]:
  p=work/(mode+"-bad-"+name+".zip")
  with zipfile.ZipFile(p,"w") as z:
   for n,data in entries:z.writestr(n,data)
  try:
   with zipfile.ZipFile(p) as z:safe_members(z)
  except (RuntimeError,ValueError,zipfile.BadZipFile):rows.append({"case":name,"rejected":True})
  else:raise RuntimeError("ZIP negative survived")
 return rows
