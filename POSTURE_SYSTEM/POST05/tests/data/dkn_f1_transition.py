# DKN01 FIX01: phase-1 heavy transitions. For every pair X->Y of mace clips (TNA_ATACK_A lateral / TNA_ATACK_B vertical), the pose distance between
# frame c of X (follow-through / recovery) and frame s of Y (wind-up): mean joint-position distance over the 29 joints of tn2.bmd (+ the mace tip),
# model units. Small = the morph between the two clips is short and invisible; large = the mace "teleports".
import os, sys, math
W=os.environ.get("DUSK_WORK", ".")
sys.path.insert(0, W+"/darknut_fable52/DARKNUT_NORMAL_RESURRECTION_ANIMATIONS/A_AGIL_NATURAL_PULIDA_FIX02/scripts")
import j3d
A=W+"/darknut_fable52/assets_private/"
M=j3d.Bmd.parse(open(A+"B_tnp2/bmdr/tn2.bmd","rb").read())
def app(G,p): return [G[i][0]*p[0]+G[i][1]*p[1]+G[i][2]*p[2]+G[i][3] for i in range(3)]
cl={k:j3d.Bck.parse(open(A+"B_tn/bck/%s.bck"%k,"rb").read()) for k in ("tna_atack_a","tna_atack_b","tna_wait")}
cache={}
def pose(k,f):
    key=(k,f)
    if key not in cache:
        G=j3d.fk(M,j3d.bck_pose(cl[k],f))
        cache[key]=[app(G[j],(0,0,0)) for j in range(29)]+[app(G[15],(46,-1,88))]
    return cache[key]
def dist(a,b): return sum(math.dist(x,y) for x,y in zip(a,b))/len(a)
imp={"tna_atack_a":22,"tna_atack_b":26}
for X in ("tna_atack_a","tna_atack_b"):
    for Y in ("tna_atack_a","tna_atack_b"):
        print("== %s -> %s"%(X,Y))
        best=[]
        for c in range(imp[X]+4, 72, 2):
            for s in range(0, imp[Y]-9):
                best.append((dist(pose(X,c),pose(Y,s)),c,s))
        best.sort()
        for d,c,s in best[:6]: print("   cut=%2d start=%2d pose_dist=%5.1f frames_to_play=%d"%(c,s,d,(c-imp[X])+(imp[Y]-s)))
        # the best start for a few fixed cuts
        for c in (30,34,38,42,48,54,60):
            r=min((dist(pose(X,c),pose(Y,s)),s) for s in range(0,imp[Y]-9))
            print("   cut=%2d best_start=%2d pose_dist=%5.1f frames=%d"%(c,r[1],r[0],(c-imp[X])+(imp[Y]-r[1])))
print("== reference: consecutive native frames (one frame of motion at the impact)", round(dist(pose("tna_atack_a",21),pose("tna_atack_a",22)),1), round(dist(pose("tna_atack_b",25),pose("tna_atack_b",26)),1))
print("== reference: wait pose vs a/b start", round(dist(pose("tna_wait",0),pose("tna_atack_a",0)),1), round(dist(pose("tna_wait",0),pose("tna_atack_b",0)),1))
