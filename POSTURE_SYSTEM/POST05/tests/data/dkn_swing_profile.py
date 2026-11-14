import os
import sys, math
sys.path.insert(0, os.path.join(os.environ.get("DUSK_WORK", "."), "darknut_fable52/DARKNUT_NORMAL_RESURRECTION_ANIMATIONS/A_AGIL_NATURAL_PULIDA_FIX02/scripts"))
import j3d
A=os.path.join(os.environ.get("DUSK_WORK", "."), "darknut_fable52/assets_private/")
M=j3d.Bmd.parse(open(A+"B_tnp2/bmdr/tn2.bmd","rb").read())
def app(G,p): return [G[i][0]*p[0]+G[i][1]*p[1]+G[i][2]*p[2]+G[i][3] for i in range(3)]
clips={"tna_atack_a":[(15,(46,-1,88)),(16,(0,0,0))],"tna_atack_b":[(15,(46,-1,88)),(16,(0,0,0))],"tna_atack_shield":[(9,(10,-10,0))],
"tnb_atack_a":[(15,(110,0,0)),(15,(0,0,110)),(16,(0,0,0))],"tnb_atack_b":[(15,(110,0,0)),(15,(0,0,110)),(16,(0,0,0))],"tnb_atack_shield":[(24,(0,0,0))]}
for c,pts in clips.items():
    b=j3d.Bck.parse(open(A+"B_tn/bck/%s.bck"%c,"rb").read())
    N=b.duration; step=1.0
    prev=None; rows=[]
    for k in range(0,N+1):
        t=k*step; G=j3d.fk(M,j3d.bck_pose(b,t))
        P=[app(G[j],p) for j,p in pts]
        if prev: rows.append((t,[math.dist(P[i],prev[i]) for i in range(len(P))],P))
        prev=P
    print("==",c,"frames",N)
    for t,v,P in rows:
        bar="#"*int(v[0]/2)
        print("  f%5.1f  v=%s  pos0=(%6.0f,%6.0f,%6.0f) %s"%(t," ".join("%6.1f"%x for x in v),P[0][0],P[0][1],P[0][2],bar))
