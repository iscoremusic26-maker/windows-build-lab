import struct, hashlib, zipfile, json, zlib, os, argparse
from pathlib import Path
ROOT=Path(os.environ['DEVF_CP01_LOCATION_WORK']).resolve()
CODE_ROOT=Path(__file__).resolve().parents[3]
assert CODE_ROOT != ROOT and CODE_ROOT not in ROOT.parents, 'Private output must be outside the code repository'
ROOT.mkdir(parents=True,exist_ok=True)
def u(d,o,fmt): return struct.unpack_from('>'+fmt,d,o)
def span(d,o,n):
    assert 0<=o<=len(d) and 0<=n<=len(d)-o,(o,n,len(d))
    return d[o:o+n]
def yaz(d):
    if d[:4]!=b'Yaz0': return d
    n=u(d,4,'I')[0];assert n<100_000_000
    out=bytearray();i=16
    while len(out)<n:
        code=span(d,i,1)[0];i+=1
        for k in range(8):
            if len(out)==n:break
            if code & (0x80>>k):out.append(span(d,i,1)[0]);i+=1
            else:
                a,b=span(d,i,2);i+=2;dist=((a&15)<<8)|b;count=a>>4
                if count==0:count=span(d,i,1)[0]+18;i+=1
                else:count+=2
                assert dist<len(out) and len(out)+count<=n
                for _ in range(count):out.append(out[-dist-1])
    return bytes(out)
def rarc(d):
    d=yaz(d);assert d[:4]==b'RARC'
    length,header,dat=u(d,4,'III');assert length==len(d) and header==32
    nodes,no,files,fo,ss,so=u(d,32,'IIIIII')
    span(d,32+no,nodes*16);span(d,32+fo,files*20);strings=span(d,32+so,ss)
    def name(o):
        assert 0<=o<len(strings)
        end=strings.index(0,o);text=strings[o:end].decode('ascii');assert '/' not in text and chr(92) not in text;return text
    entries=[]
    for i in range(files):
        fid,nh,flags,off,size,ptr=u(d,32+fo+i*20,'HHIIII')
        nm=name(flags&0xffffff);flags>>=24
        raw=None if flags&2 else span(d,32+dat+off,size)
        entries.append(dict(index=i,id=fid,name=nm,flags=flags,size=size,raw=raw))
    def walk(node,prefix='',parents=()):
        assert node not in parents and node<nodes
        typ,nm,nh,count,first=u(d,32+no+node*16,'IIHHI')
        assert first+count<=files
        result=[]
        for e in entries[first:first+count]:
            if e['name'] in ['.','..']:continue
            path=prefix+e['name']
            if e['flags']&2:result+=walk(u(d,32+fo+e['index']*20+8,'I')[0],path+'/',parents+(node,))
            else:result.append(dict(e,path=path))
        return result
    return d,entries,walk(0)
def dzs(d):
    n=u(d,0,'I')[0];assert n<1000;span(d,4,n*12)
    nodes=[(span(d,4+i*12,4).decode('ascii'),*u(d,8+i*12,'II')) for i in range(n)]
    out=[]
    for tag,count,off in nodes:
        assert off>=4+n*12 and off<=len(d)
        end=min([o for _,_,o in nodes if o>off]+[len(d)])
        size={'ACTR':32,'TGOB':32,'PLYR':32,'TGSC':36,'SCOB':36,'TGDR':36,'Door':36,'MULT':12,'SCLS':12,'RPAT':12,'RPPN':16}.get(tag)
        if tag[:3] in ['ACT','TRE']:size=32
        if tag[:3] in ['SCO','Doo']:size=36
        if size:span(d,off,count*size);assert off+count*size<=end,(tag,count,size,end-off)
        records=[]
        if tag in ['ACTR','TGOB','PLYR','TGSC','SCOB','TGDR','Door'] or tag[:3] in ['ACT','SCO','Doo','TRE']:
            for i in range(count):
                o=off+i*size
                records.append(dict(index=i,name=span(d,o,8).split(b'\0')[0].decode('ascii'),parameters=hex(u(d,o+8,'I')[0]),position=u(d,o+12,'fff'),angle=u(d,o+24,'hhh'),set_id=u(d,o+30,'H')[0],scale_bytes=list(span(d,o+32,3)) if size==36 else None))
        if tag=='MULT':
            records=[dict(translation_xz=u(d,off+i*12,'ff'),angle=u(d,off+i*12+8,'h')[0],room=d[off+i*12+10]) for i in range(count)]
        out.append(dict(tag=tag,count=count,offset=off,bounded_span=end-off,record_size=size,records=records))
    return out
def bbox(v):return [list(map(min,zip(*v))),list(map(max,zip(*v)))]
def dzb(d):
    nv,ov,nt,ot,nb,ob,nn,on,ng,og,ni,oi,flags=u(d,0,'I'*13)
    for c,o,s in [(nv,ov,12),(nt,ot,10),(nb,ob,2),(nn,on,20),(ng,og,52),(ni,oi,16)]:
        assert c<1_000_000;span(d,o,c*s)
    v=[u(d,ov+i*12,'fff') for i in range(nv)]
    t=[u(d,ot+i*10,'HHHHH') for i in range(nt)]
    for a,b,c,info,grp in t:assert max(a,b,c)<nv and info<ni and grp<ng
    groups=[]
    for i in range(ng):
        o=og+i*52;no=u(d,o,'I')[0];assert no<len(d)
        groups.append(dict(index=i,name=d[no:d.index(0,no)].decode('ascii'),scale=u(d,o+4,'fff'),angle=u(d,o+16,'hhh'),translation=u(d,o+24,'fff'),parent=u(d,o+36,'H')[0],room=u(d,o+42,'H')[0],info=hex(u(d,o+48,'I')[0])))
    infos=[u(d,oi+i*16,'IIII') for i in range(ni)]
    return dict(vertices=nv,triangles=nt,groups=groups,info_count=ni,flags=flags,bounds=bbox(v)),v,t,infos
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--inputs',type=Path,required=True);args=parser.parse_args()
    p=args.inputs/'DEVF_CP01_EVIDENCIA_F_SP121.zip'
    expected=(args.inputs/'CP01_EVIDENCE_SHA256SUMS.txt').read_text().split()[0]
    sha=hashlib.sha256(p.read_bytes()).hexdigest();assert sha==expected
    report=dict(zip_sha256=sha,zip_size=p.stat().st_size,zip_crc='PASS',members=[],archives={})
    with zipfile.ZipFile(p) as z:
        assert z.testzip() is None
        assert sorted(z.namelist())==sorted(['STG_00.arc','R03_00.arc','WRock.arc','SHA256SUMS.txt'])
        sums={line.split()[1]:line.split()[0] for line in z.read('SHA256SUMS.txt').decode().splitlines() if line.strip()}
        for e in z.infolist():
            d=z.read(e);h=hashlib.sha256(d).hexdigest()
            assert len(d)==e.file_size and zlib.crc32(d)==e.CRC
            if e.filename in sums:assert sums[e.filename]==h
            report['members'].append(dict(name=e.filename,size=len(d),compressed_size=e.compress_size,crc32=f'{e.CRC:08x}',sha256=h,manifest_match=e.filename in sums))
            if e.filename.endswith('.arc'):
                raw,entries,internal=rarc(d)
                dest=ROOT/e.filename;dest.mkdir(exist_ok=True)
                arc=dict(compression=d[:4].decode('ascii',errors='replace'),decompressed_size=len(raw),files=[])
                for f in internal:
                    data=yaz(f['raw']);out=dest/f['path'];out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(data)
                    item={k:f[k] for k in ['index','id','path','flags','size']}
                    item['decoded_size']=len(data);item['sha256']=hashlib.sha256(data).hexdigest()
                    if f['name'].endswith(('.dzs','.dzr')):item['chunks']=dzs(data)
                    if f['name'].endswith('.dzb'):item['collision']=dzb(data)[0]
                    arc['files'].append(item)
                report['archives'][e.filename]=arc
    (ROOT/'inventory.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS zip SHA256, all CRCs, sizes, individual hashes and bounded RARC/DZS/DZR/DZB; private inventory:',ROOT/'inventory.json')
