from analyze_location import *
import math
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def add(a,b):return tuple(x+y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def kcl(d,plc):
    op,on,ot,ob=u(d,0,'IIII');assert 56<=op<on<ot<ob<len(d)
    assert (on-op)%12==0 and (ob-ot)%16==0
    nv=(on-op)//12;nt=(ob-ot)//16-1
    v=[u(d,op+i*12,'fff') for i in range(nv)]
    assert plc[:4]==b'SPLC';size,ni=u(plc,4,'HH');assert size==20
    span(plc,8,ni*20);codes=[u(plc,8+i*20,'IIIII') for i in range(ni)]
    def norm(index):
        o=on+12*index;assert o>=on and o+12<=ot+16
        return u(d,o,'fff')
    tris=[]
    for i in range(1,nt+1):
        h,pi,fn,e1,e2,e3,attr=u(d,ot+16*i,'fHHHHHH')
        assert pi<nv and attr<ni and math.isfinite(h)
        p=v[pi];n=norm(fn);a,b,c=norm(e1),norm(e2),norm(e3)
        r1=cross(n,a);r2=cross(b,n);den1=dot(r1,c);den2=dot(r2,c)
        assert abs(den1)>1e-8 and abs(den2)>1e-8
        pts=[p,add(p,mul(r2,h/den2)),add(p,mul(r1,h/den1))]
        assert all(math.isfinite(x) for q in pts for x in q)
        tris.append(dict(index=i,pts=pts,normal=n,edges=[a,b,c],height=h,attribute=attr,code=codes[attr]))
    return dict(position_count=nv,prism_count=nt,plc_count=ni,offsets=[op,on,ot,ob],bounds=bbox([q for t in tris for q in t['pts']])),tris
def plc_dec(c):
    return dict(exit_id=c[0]&63,object_through=bool(c[0]&(1<<14)),link_through=bool(c[0]&(1<<16)),special=(c[0]>>24)&15,ground_code=(c[1]>>19)&31,room=(c[4]>>20)&255,normal_group=bool(c[4]&(1<<8)),code_words=[hex(x) for x in c])
def hit(t,x,z):
    p=t['pts'][0];n=t['normal']
    if n[1]<0.5 or t['code'][0]&(1<<14) or t['code'][4]&(1<<8):return None
    v=(x-p[0],0,z-p[2]);v=(v[0],-(v[0]*n[0]+v[2]*n[2])/n[1],v[2])
    a,b,c=t['edges']
    if dot(v,a)>0.0075 or dot(v,b)>0.0075 or dot(v,c)>t['height']+0.0075 or dot(v,c)<-0.0075:return None
    return p[1]+v[1]
def supports(tris,x,z,ceiling):
    out=[(h,t) for t in tris if (h:=hit(t,x,z)) is not None and h<ceiling]
    return sorted(out,key=lambda x:x[0],reverse=True)
def clip(poly,tri):
    # Convex polygon clipped against a triangle in XZ; ensure CCW.
    def det(a,b,p):return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
    if det(tri[0],tri[1],tri[2])<0:tri=list(reversed(tri))
    for a,b in zip(tri,tri[1:]+tri[:1]):
        result=[]
        for p,q in zip(poly,poly[1:]+poly[:1]):
            dp,dq=det(a,b,p),det(a,b,q)
            if dp>=-1e-7:result.append(p)
            if (dp<0)!=(dq<0):
                ratio=dp/(dp-dq);result.append((p[0]+ratio*(q[0]-p[0]),p[1]+ratio*(q[1]-p[1])))
        poly=result
        if not poly:break
    return poly
def area(poly):
    return abs(sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(poly,poly[1:]+poly[:1])))/2 if len(poly)>2 else 0
def bmd(d):
    assert d[:8]==b'J3D2bmd3';length,n=u(d,8,'II');assert length==len(d)
    sections={};o=32
    for _ in range(n):
        tag=span(d,o,4).decode();size=u(d,o+4,'I')[0];assert size>=8
        sections[tag]=span(d,o,size);o+=size
    assert o==len(d)
    vc=u(sections['INF1'],16,'I')[0]
    v=sections['VTX1'];fmt,pos=u(v,8,'II')
    # Validate stored position format via GX attr list (attr9 position, XYZ1, F32=4).
    fm=[]
    for o in range(fmt,len(v),16):
        a,c,t,shift=u(v,o,'IIII')
        if a==255:break
        fm.append((a,c,t,v[o+12]))
    pf=next(x for x in fm if x[0]==9)
    assert pf[1:3]==(1,4),pf
    span(v,pos,vc*12);verts=[u(v,pos+i*12,'fff') for i in range(vc)]
    return dict(sections=[(tag,len(v)) for tag,v in sections.items()],position_format=pf,vertices=vc,position_bounds=bbox(verts)),sections
if __name__=='__main__':
    room=ROOT/'R03_00.arc'
    meta,tris=kcl((room/'kcl/room.kcl').read_bytes(),(room/'plc/room.plc').read_bytes())
    rock,rv,rt,ri=dzb((ROOT/'WRock.arc/dzb/wrock.dzb').read_bytes())
    model,sections=bmd((ROOT/'WRock.arc/bmdr/wrock.bmd').read_bytes())
    p=(-4056.7346,-6717.1538,65135.8281)
    obs=supports(tris,p[0],p[2],p[1]+1000)
    result=dict(room_kcl=meta,rock_dzb=rock,rock_bmd=model,observed_supports=[dict(index=t['index'],y=h,normal=t['normal'],slope_degrees=math.degrees(math.acos(t['normal'][1])),plc=plc_dec(t['code'])) for h,t in obs],evaluations=[])
    bounds=rock['bounds']
    for dx in [-1500,-1000,-500,0,500,1000,1500]:
      for dz in [-1500,-1000,-500,0,500,1000,1500]:
        x,z=p[0]+dx,p[2]+dz;hits=supports(tris,x,z,p[1]+1000)
        if not hits:continue
        y,t=hits[0]
        poly=[(x+bounds[0][0],z+bounds[0][2]),(x+bounds[1][0],z+bounds[0][2]),(x+bounds[1][0],z+bounds[1][2]),(x+bounds[0][0],z+bounds[1][2])]
        covered=0;heights=[];ti=[];max_slope=0
        for f in tris:
            n=f['normal'];c=f['code']
            if n[1]<0.5 or c[0]&(1<<14) or c[4]&(1<<8):continue
            cp=clip(poly,[(a[0],a[2]) for a in f['pts']])
            if area(cp)<1e-5:continue
            hs=[f['pts'][0][1]-((a-f['pts'][0][0])*n[0]+(b-f['pts'][0][2])*n[2])/n[1] for a,b in cp]
            if max(hs)>p[1]+1000 or min(hs)<y-1000:continue
            covered+=area(cp);heights+=hs;ti.append(f['index'])
            max_slope=max(max_slope,math.degrees(math.acos(min(1,n[1]))))
        result['evaluations'].append(dict(offset_xz=[dx,dz],position=[x,y,z],triangle=t['index'],normal=t['normal'],plc=plc_dec(t['code']),footprint_rectangle_area=area(poly),coverage_area_sum=covered,coverage_ratio=covered/area(poly),terrain_height_minmax=[min(heights),max(heights)] if heights else None,height_span=max(heights)-min(heights) if heights else None,max_slope_degrees=max_slope,overlap_triangles=ti))
    (ROOT/'geometry.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS bounded KCL/PLC and BMD; geometry metadata:',ROOT/'geometry.json')
    print('Evaluated locations:',len(result['evaluations']),'(not activation coordinates)')
