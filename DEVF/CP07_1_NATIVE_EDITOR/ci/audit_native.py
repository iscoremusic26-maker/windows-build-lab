"""Strict StageService-only QA audit; reuse unchanged baseline PE/ZIP readers."""
import hashlib,importlib.util,json,struct,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1]
BASE=REPO/"DEVF/CP06_CONFIGURABLE_PLACEMENTS/ci"
spec=importlib.util.spec_from_file_location("baseline_reader",BASE/"audit.py");old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha=old.sha;safe_members=old.safe_members;pe_parts=old.pe_parts
def inspect_pe(path):
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
 if "dusklight.exe" in imports:raise RuntimeError("Game import forbidden in Stage-only QA")
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
  else:raise RuntimeError("Hook/game/export-service metadata forbidden")
  n+=size
 expected={("dev.twilitrealm.dusklight."+s,a,c) for s,a,c in [("host",2,2),("file",1,0),("log",1,0),("stage",1,0),("ui",2,3)]}
 if {(r["id"],r["major"],r["minor"]) for r in records if r["kind"]=="import"}!=expected or len(records)!=6:raise RuntimeError("Unexpected metadata service graph")
 if any(r["optional"]!=(r["id"]=="dev.twilitrealm.dusklight.ui") for r in records if r["kind"]=="import"):raise RuntimeError("UiService must be optional, others required")
 return {"status":"CP07_EDITOR_PE_ABI_PASS","machine":"AMD64","imports":imports,"metadata":records,"exports":old.pe_exports(path),"hooks":0,"game_imports":0,"tls_or_delay_imports":False,"DLL_executed":False}
def verify_package(package,dll):
 with zipfile.ZipFile(package) as z:
  members=safe_members(z);manifest=z.read("mod.json");inside=z.read("lib/windows-amd64/mod.dll")
  if manifest!=(ROOT/"mod.json").read_bytes() or inside!=dll.read_bytes():raise RuntimeError("Package identity/content mismatch")
 parsed=json.loads(manifest)
 if parsed["id"]!="dev.devf.cp06_native_config_dual" or parsed["version"]!="0.7.0":raise RuntimeError("Wrong mod id/version")
 return {"status":"PACKAGE_VERIFIED","package_bytes":package.stat().st_size,"package_sha256":sha(package.read_bytes()),"dll_bytes":len(inside),"dll_sha256":sha(inside),"manifest_sha256":sha(manifest),"members":members,"pe":inspect_pe(dll)}
def negatives(dll,work):
 raw=dll.read_bytes();pe,opt,_=pe_parts(raw);rows=[]
 def reject(name,mut):
  out=work/("negative-"+name+".dll");out.write_bytes(mut)
  try:inspect_pe(out)
  except (RuntimeError,ValueError,StopIteration,struct.error):rows.append({"case":name,"rejected":True})
  else:raise RuntimeError("Binary negative survived "+name)
 m=bytearray(raw);struct.pack_into("<H",m,pe+4,0x14c);reject("x86",m)
 reject("foreign_stage_service",raw.replace(b"dev.twilitrealm.dusklight.stage",b"dev.twilitrealm.dusklight.stagg"))
 reject("missing_export",raw.replace(b"mod_update",b"mod_updatX"))
 # Mutate a required service's version, not merely an unused string.
 m=bytearray(raw);_,_,at=pe_parts(raw)
 for i in range(struct.unpack_from("<H",raw,pe+6)[0]):
  q=opt+struct.unpack_from("<H",raw,pe+20)[0]+40*i
  if raw[q:q+8].rstrip(b"\0")==b"modmeta":vs,vr,rs,n=struct.unpack_from("<4I",raw,q+8);break
 end=n+min(vs,rs)
 while n+8<=end:
  if not any(raw[n:n+8]):n+=8;continue
  size,kind=struct.unpack_from("<HB",raw,n)
  if kind==2 and raw[n+16:n+size].split(b"\0",1)[0]==b"dev.twilitrealm.dusklight.stage":
   struct.pack_into("<H",m,n+4,2);break
  n+=size
 reject("wrong_stage_major",m)
 for name,entries in [("extra_original_asset",[("mod.json",b"{}"),("lib/windows-amd64/mod.dll",raw),("E_oc2.arc",b"dummy-not-an-asset")]),("traversal",[("../mod.json",b"{}"),("lib/windows-amd64/mod.dll",raw)])]:
  p=work/("bad-"+name+".zip")
  with zipfile.ZipFile(p,"w") as z:
   for n,data in entries:z.writestr(n,data)
  try:
   with zipfile.ZipFile(p) as z:safe_members(z)
  except (RuntimeError,ValueError,zipfile.BadZipFile):rows.append({"case":name,"rejected":True})
  else:raise RuntimeError("ZIP negative survived")
  # FileService is the only new imported service, validate it explicitly.
 reject("foreign_file_service",raw.replace(b"dev.twilitrealm.dusklight.file",b"dev.twilitrealm.dusklight.filx"))
 m=bytearray(raw)
 for i in range(struct.unpack_from("<H",raw,pe+6)[0]):
  q=opt+struct.unpack_from("<H",raw,pe+20)[0]+40*i
  if raw[q:q+8].rstrip(b"\0")==b"modmeta":vs,vr,rs,n=struct.unpack_from("<4I",raw,q+8);break
 end=n+min(vs,rs)
 while n+8<=end:
  if not any(raw[n:n+8]):n+=8;continue
  size,kind=struct.unpack_from("<HB",raw,n)
  if kind==2 and raw[n+16:n+size].split(b"\0",1)[0]==b"dev.twilitrealm.dusklight.file":
   struct.pack_into("<H",m,n+4,2);break
  n+=size
 reject("wrong_file_major",m)
 for label in ["wrong_ui_version","required_ui","foreign_ui"]:
  m=bytearray(raw)
  for i in range(struct.unpack_from("<H",raw,pe+6)[0]):
   q=opt+struct.unpack_from("<H",raw,pe+20)[0]+40*i
   if raw[q:q+8].rstrip(b"\0")==b"modmeta":vs,vr,rs,n=struct.unpack_from("<4I",raw,q+8);break
  end=n+min(vs,rs);found=False
  while n+8<=end:
   if not any(raw[n:n+8]):n+=8;continue
   size,kind=struct.unpack_from("<HB",raw,n)
   if kind==2 and raw[n+16:n+size].split(b"\0",1)[0]==b"dev.twilitrealm.dusklight.ui":
    if label=="wrong_ui_version":struct.pack_into("<H",m,n+6,4)
    elif label=="required_ui":m[n+3]=0
    else:m[n+16+len("dev.twilitrealm.dusklight.")]=ord("x")
    found=True;break
   n+=size
  if not found:raise RuntimeError("Optional UI import missing")
  reject(label,m)
 return rows
