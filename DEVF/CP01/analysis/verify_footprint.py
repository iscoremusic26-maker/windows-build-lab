from location_details import *
def halfclip(poly,a,b,c):
    out=[]
    for p,q in zip(poly,poly[1:]+poly[:1]):
        fp=a*p[0]+b*p[1]-c;fq=a*q[0]+b*q[1]-c
        if fp<=1e-9:out.append(p)
        if (fp<=0)!=(fq<=0):
            ratio=fp/(fp-fq);out.append((p[0]+ratio*(q[0]-p[0]),p[1]+ratio*(q[1]-p[1])))
    return out
def constraints(t):
    n=t['normal'];p=t['pts'][0]
    def coeff(e):
        a=e[0]-e[1]*n[0]/n[1];b=e[2]-e[1]*n[2]/n[1]
        return a,b,a*p[0]+b*p[2]
    a,b,c=[coeff(e) for e in t['edges']]
    return [(a[0],a[1],a[2]+.0075),(b[0],b[1],b[2]+.0075),(c[0],c[1],c[2]+t['height']+.0075),(-c[0],-c[1],-c[2]+.0075)]
def subtract(poly,conditions):
    remainder=[]
    for a,b,c in conditions:
        outside=halfclip(poly,-a,-b,-c)
        if area(outside)>1e-7:remainder.append(outside)
        poly=halfclip(poly,a,b,c)
        if area(poly)<1e-7:break
    return remainder
if __name__=='__main__':
    r=ROOT/'R03_00.arc';d=(r/'kcl/room.kcl').read_bytes()
    meta,tris=kcl(d,(r/'plc/room.plc').read_bytes());tree,query=octree(d,meta)
    p=(-4806.7346,-6729.480961460304,65135.8281)
    rock,rv,rt,ri=dzb((ROOT/'WRock.arc/dzb/wrock.dzb').read_bytes())
    bm,sections=bmd((ROOT/'WRock.arc/bmdr/wrock.bmd').read_bytes())
    bounds=[list(map(min,zip(rock['bounds'][0],bm['position_bounds'][0]))),list(map(max,zip(rock['bounds'][1],bm['position_bounds'][1])))]
    all_results={}
    for label,b in [('collision',rock['bounds']),('model_and_collision',bounds)]:
        poly=[(p[0]+b[0][0],p[2]+b[0][2]),(p[0]+b[1][0],p[2]+b[0][2]),(p[0]+b[1][0],p[2]+b[1][2]),(p[0]+b[0][0],p[2]+b[1][2])]
        remaining=[poly];contributors=[];hs=[]
        for index in [2174,2175]:
            t=tris[index-1];cp=poly
            for a,b1,c in constraints(t):cp=halfclip(cp,a,b1,c)
            if area(cp)>1e-7:
                contributors.append(index);n=t['normal'];base=t['pts'][0]
                hs += [base[1]-((a-base[0])*n[0]+(b1-base[2])*n[2])/n[1] for a,b1 in cp]
                remaining=[r for q in remaining for r in subtract(q,constraints(t))]
        uncovered=sum(area(q) for q in remaining);assert uncovered<1e-5,(label,uncovered)
        # Independent XZ barycentric support and octree reachability over a 21x21 lattice.
        samples=0;maxdiff=0;indices=set()
        for ix in range(21):
            for iz in range(21):
                x=poly[0][0]+(poly[1][0]-poly[0][0])*ix/20
                z=poly[0][1]+(poly[2][1]-poly[0][1])*iz/20
                hits=supports(tris,x,z,p[1]+1000);assert hits
                h,t=hits[0];assert t['index'] in query((x,h+10,z));indices.add(t['index'])
                q=[(a[0],a[2]) for a in t['pts']]
                den=(q[1][1]-q[2][1])*(q[0][0]-q[2][0])+(q[2][0]-q[1][0])*(q[0][1]-q[2][1])
                wa=((q[1][1]-q[2][1])*(x-q[2][0])+(q[2][0]-q[1][0])*(z-q[2][1]))/den
                wb=((q[2][1]-q[0][1])*(x-q[2][0])+(q[0][0]-q[2][0])*(z-q[2][1]))/den
                hb=wa*t['pts'][0][1]+wb*t['pts'][1][1]+(1-wa-wb)*t['pts'][2][1]
                maxdiff=max(maxdiff,abs(h-hb));assert abs(h-hb)<1e-5;samples+=1
        all_results[label]=dict(rectangle=poly,uncovered_area_engine_halfplanes=uncovered,contributing_prisms=contributors,terrain_height_minmax=[min(hs),max(hs)],grid_samples=samples,grid_prisms=sorted(indices),barycentric_max_difference=maxdiff)
    result=dict(candidate=list(p),yaw_s16=0,scale=[1,1,1],analysis_only=True,footprints=all_results)
    (ROOT/'footprint-verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
