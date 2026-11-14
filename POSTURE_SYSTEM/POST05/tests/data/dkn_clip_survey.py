# DKN01 FIX01: survey of native B_tn clips on the mType-1 rig (tn2.bmd): per frame pelvis height, head height, sword-tip/mace-tip speed.
import os, sys, math
W=os.environ.get("DUSK_WORK", ".")
sys.path.insert(0, W+"/darknut_fable52/DARKNUT_NORMAL_RESURRECTION_ANIMATIONS/A_AGIL_NATURAL_PULIDA_FIX02/scripts")
import j3d
A=W+"/darknut_fable52/assets_private/"
M=j3d.Bmd.parse(open(A+"B_tnp2/bmdr/tn2.bmd","rb").read())
names=[getattr(j,'name',str(i)) for i,j in enumerate(M.joints)] if hasattr(M,'joints') else []
def app(G,p): return [G[i][0]*p[0]+G[i][1]*p[1]+G[i][2]*p[2]+G[i][3] for i in range(3)]
def pt(G,j,p=(0,0,0)): return app(G[j],p)
clips=sys.argv[1:]
if not clips: print(names); sys.exit()
step=float(os.environ.get("STEP","1"))
for c in clips:
    b=j3d.Bck.parse(open(A+"B_tn/bck/%s.bck"%c,"rb").read())
    N=b.duration
    print("== %s frames=%d loop=%d"%(c,N,b.loop))
    prev=None; t=0.0
    while t<=N+1e-6:
        G=j3d.fk(M,j3d.bck_pose(b,t))
        P=[pt(G,int(os.environ.get("PELVIS","1"))), pt(G,int(os.environ.get("HEAD","4"))), pt(G,15,(110,0,0)), pt(G,15,(46,-1,88))]
        v=[math.dist(P[i],prev[i]) if prev else 0 for i in range(4)]
        print(" f%5.1f pelvis=(%5.0f,%5.0f,%5.0f) head_y=%5.0f tip=(%5.0f,%5.0f,%5.0f) v_sword=%6.1f v_mace=%6.1f v_head=%5.1f"%(t,P[0][0],P[0][1],P[0][2],P[1][1],P[2][0],P[2][1],P[2][2],v[2],v[3],v[1]))
        prev=P; t+=step
