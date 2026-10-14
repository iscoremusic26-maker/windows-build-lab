"""Focused static PE call/inlining gate for a selected packet's native draw, never execute host."""
import hashlib, json, os, re, struct, subprocess, sys, tempfile, zipfile
from pathlib import Path
PIN="ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049"
z=Path("Dusklight-v2.0.3-win32-x86_64.zip")
if z.stat().st_size!=56037777:raise RuntimeError("Original LFS Windows ZIP missing")
with zipfile.ZipFile(z) as archive:
    b=archive.read("dusklight.exe")
if len(b)!=52309504 or hashlib.sha256(b).hexdigest()!=PIN:raise RuntimeError("Pinned host differs")
pe=struct.unpack_from("<I",b,0x3c)[0];opt=pe+24
if struct.unpack_from("<H",b,pe+4)[0]!=0x8664:raise RuntimeError("AMD64 required")
ss=struct.unpack_from("<H",b,pe+20)[0];base=struct.unpack_from("<Q",b,opt+24)[0];sections=[]
for i in range(struct.unpack_from("<H",b,pe+6)[0]):
    q=opt+ss+i*40;name=b[q:q+8].rstrip(b"\0").decode();vs,vr,rs,off=struct.unpack_from("<4I",b,q+8);sections.append((name,vs,vr,rs,off))
def at(v):
    for name,vs,vr,rs,off in sections:
        if vr<=v<vr+max(vs,rs) and v-vr<rs:return off+v-vr
    raise RuntimeError("Unmapped symbol")
def follow(v):
    for _ in range(8):
        q=at(v)
        if b[q]!=0xe9:return v
        v=v+5+struct.unpack_from("<i",b,q+1)[0]
    raise RuntimeError("Unexpected thunk chain")
sec=next(x for x in sections if x[0]==".symdb")
q=sec[4];magic,ver,comp,ulen,clen,bilen,bid,n=struct.unpack_from("<8sIIQQI32sI",b,q)
if magic!=b"SYMGEN\0\0" or ver!=2:raise RuntimeError("Manifest mismatch")
data=b[q+72:q+72+clen]
if comp==1:
    import zstandard
    data=zstandard.ZstdDecompressor().decompress(data,max_output_size=ulen)
elif comp!=0:raise RuntimeError("Unexpected compression")
if len(data)!=ulen:raise RuntimeError("Truncated manifest")
strings=data[n*24:];wanted={"J3DShapePacket::drawFast","J3DMatPacket::draw","J3DShape::drawFast","J3DShapeDraw::draw"};found={};all_code=set()
for i in range(n):
    _,v,off,flags=struct.unpack_from("<QQII",data,i*24)
    if flags&1:all_code.add(v)
    name=strings[off:strings.index(0,off)].decode()
    if name in wanted:found.setdefault(name,[]).append((v,flags))
if any(len(found.get(name,[]))!=1 for name in wanted):raise RuntimeError("Missing or ambiguous native draw symbol")
report={"status":"STATIC_GATE_PENDING","host_sha256":PIN,"host_bytes":len(b),"manifest_version":ver,"targets":[],"game_executed":False,"original_bytes_or_addresses_published":False}
for name,values in found.items():
    v,flags=values[0];report["targets"].append({"symbol":name,"flags":flags,"inline_sites":bool(flags&32),"folded_or_alias":bool(flags&8)})
target=follow(found["J3DShapePacket::drawFast"][0][0]);caller=follow(found["J3DMatPacket::draw"][0][0]);end=min(v for v in all_code if v>caller)
if end-caller>32768:raise RuntimeError("Unexpected caller extent")
path=Path(tempfile.gettempdir())/"devf-cp044-static-host.exe";path.write_bytes(b)
try:
    raw=subprocess.check_output(["objdump","-d","--start-address="+str(base+caller),"--stop-address="+str(base+end),str(path)],text=True)
    dests=[int(m.group(1),16)-base for m in re.finditer(r"\bcall\s+0x([0-9a-f]+)",raw)]
    count=sum(follow(v)==target for v in dests)
finally:path.unlink(missing_ok=True)
report["matpacket_to_shapepacket_drawfast_direct_calls"]=count
flags=found["J3DShapePacket::drawFast"][0][1]
report["packet_hook_admitted"]=bool(count>=1 and flags&1 and not(flags&(8|32)))
report["status"]="PACKET_CALL_GATE_PASS" if report["packet_hook_admitted"] else "PACKET_CALL_GATE_BLOCKED"
report["objdump_version"]=subprocess.check_output(["objdump","--version"],text=True).splitlines()[0]
print("EOC2_HOST_DRAW_GATE="+json.dumps(report,sort_keys=True))
