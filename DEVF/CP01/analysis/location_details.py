from location_geometry import *
def octree(d,meta):
    ob=meta['offsets'][3];minimum=u(d,20,'fff');masks=u(d,32,'III');shift,sx,sxy=u(d,44,'III')
    widths=[(~m)&0xffffffff for m in masks];dims=[(w>>shift)+1 for w in widths]
    assert sx==int(math.log2(dims[0])) and sxy==int(math.log2(dims[0]*dims[1]))
    seen=set();refs=set();leaves=set()
    def descend(base,offset,level,trail=()):
        p=base+offset;span(d,p,4);val=u(d,p,'I')[0]
        if val&0x80000000:
            leaf=base+(val&0x7fffffff);span(d,leaf,4);leaves.add(leaf);i=leaf+2;items=[]
            while True:
                idx=u(d,i,'H')[0];i+=2
                if idx==0:break
                assert 1<=idx<=meta['prism_count'];refs.add(idx);items.append(idx)
            return items
        child=base+val;assert level>0 and child not in trail;span(d,child,32)
        if child in seen:return []
        seen.add(child)
        for i in range(8):descend(child,i*4,level-1,trail+(child,))
        return []
    for i in range(math.prod(dims)):descend(ob,i*4,shift)
    def point_list(p):
        coords=[int(p[i]-minimum[i]) for i in range(3)]
        assert all(0<=coords[i]<=widths[i] for i in range(3))
        base=ob;level=shift;idx=4*((coords[2]>>level)<<sxy|(coords[1]>>level)<<sx|(coords[0]>>level))
        while True:
            val=u(d,base+idx,'I')[0]
            if val&0x80000000:
                i=base+(val&0x7fffffff)+2;out=[]
                while (a:=u(d,i,'H')[0])!=0:out.append(a);i+=2
                return out
            base+=val;level-=1;assert level>=0
            idx=4*(((coords[2]>>level)&1)<<2|((coords[1]>>level)&1)<<1|((coords[0]>>level)&1))
    return dict(root_dimensions=dims,area_minimum=minimum,block_width_shift=shift,internal_nodes=len(seen),leaf_lists=len(leaves),distinct_prism_references=len(refs)),point_list
def point_distance_segment(p,a,b):
    v=(b[0]-a[0],b[2]-a[2]);w=(p[0]-a[0],p[2]-a[2]);den=dot(v,v);s=max(0,min(1,dot(w,v)/den)) if den else 0
    return math.hypot(w[0]-s*v[0],w[1]-s*v[1])
def event_data(d):
    sizes=[176,80,80,64,4,4,1];tables=[]
    for i,s in enumerate(sizes):
        off,n=u(d,i*8,'Ii');assert n>=0;span(d,off,n*s);tables.append((off,n,s))
    def name(o,n=32):return span(d,o,n).split(b'\0')[0].decode('ascii')
    ev,st,cu,da,fd,ii,sd=tables
    events=[name(ev[0]+i*176) for i in range(ev[1])]
    staff=[dict(name=name(st[0]+i*80,8),start_cut=u(d,st[0]+i*80+48,'i')[0],type=u(d,st[0]+i*80+44,'i')[0]) for i in range(st[1])]
    cuts=[name(cu[0]+i*80) for i in range(cu[1])]
    data=[]
    for i in range(da[1]):
        o=da[0]+i*64;typ,index,n,nxt=u(d,o+36,'iiii')
        vals=None
        if typ in [0,1,3,4] and n>=0:
            table=fd if typ in [0,1] else ii if typ==3 else sd
            count=n*3 if typ==1 else n
            assert index>=0 and index+count<=table[1],(i,typ,index,n,table)
            raw=span(d,table[0]+index*table[2],count*table[2])
            vals=u(raw,0,('f' if typ in [0,1] else 'i')*count) if typ!=4 else raw.decode('ascii',errors='replace')
        data.append(dict(name=name(o),type=typ,values=vals))
    return dict(table_counts=[n for _,n,_ in tables],events=events,staff=staff,cut_names=cuts,data=data)
def paths(d):
    cs=dzs(d);rp=next(x for x in cs if x['tag']=='RPAT');pp=next(x for x in cs if x['tag']=='RPPN');out=[]
    for i in range(rp['count']):
        o=rp['offset']+i*12;n,nextid=u(d,o,'HH');off=pp['offset']+u(d,o+8,'I')[0]
        assert off>=pp['offset'] and off+n*16<=pp['offset']+pp['count']*16 and (off-pp['offset'])%16==0
        points=[u(d,off+j*16+4,'fff') for j in range(n)]
        out.append(dict(index=i,count=n,next_id=nextid,closed=bool(d[o+5]&1),points=points))
    return out
if __name__=='__main__':
    r=ROOT/'R03_00.arc';d=(r/'kcl/room.kcl').read_bytes()
    m,tris=kcl(d,(r/'plc/room.plc').read_bytes());tree,query=octree(d,m)
    candidate=(-4556.7346,-6725.720543062609,65135.8281);observed=(-4056.7346,-6717.1538,65135.8281)
    bmdmeta,secs=bmd((ROOT/'WRock.arc/bmdr/wrock.bmd').read_bytes());j=secs['JNT1'];jn=u(j,8,'H')[0];jo=u(j,12,'I')[0]
    assert jn==1
    joint=dict(count=jn,scale=u(j,jo+4,'fff'),rotation=u(j,jo+16,'hhh'),translation=u(j,jo+24,'fff'),radius=u(j,jo+36,'f')[0],bounds=[u(j,jo+40,'fff'),u(j,jo+52,'fff')])
    assert joint['scale']==(1,1,1) and joint['rotation']==(0,0,0) and joint['translation']==(0,0,0)
    result=dict(kcl_octree=tree,observed_octree_contains_2494=2494 in query((observed[0],observed[1]+10,observed[2])),candidate_octree_contains_2174=2174 in query((candidate[0],candidate[1]+10,candidate[2])),rock_joint=joint,events={},paths={})
    for arc,name in [('R03_00.arc','dzr/room.dzr'),('STG_00.arc','dzs/stage.dzs')]:
        result['events'][arc]=event_data((ROOT/arc/'evt/event_list.dat').read_bytes())
        ps=paths((ROOT/arc/name).read_bytes());summaries=[]
        for path in ps:
            pp=path['points'];edges=list(zip(pp,pp[1:]))+([(pp[-1],pp[0])] if path['closed'] and pp else [])
            distance=min([point_distance_segment(candidate,a,b) for a,b in edges]+[math.hypot(q[0]-candidate[0],q[2]-candidate[2]) for q in pp],default=None)
            summaries.append({k:path[k] for k in ['index','count','next_id','closed']}|dict(candidate_distance_xz=distance))
        result['paths'][arc]=sorted(summaries,key=lambda x:x['candidate_distance_xz'] if x['candidate_distance_xz'] is not None else 1e99)
    (ROOT/'details.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS complete reachable KCL octree, event-table bounds, relative route offsets and identity joint; private details:',ROOT/'details.json')
