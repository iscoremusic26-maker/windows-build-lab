# DKN01 FIX01: second stagger (TNB_DOWN) entry from the first stagger hold (TNB_DOWN_SIPPU), and the getup end (TN2B_DIE reversed) into TNB_WAIT.
import os, sys, math
W=os.environ.get("DUSK_WORK", ".")
sys.path.insert(0, W+"/darknut_fable52/DARKNUT_NORMAL_RESURRECTION_ANIMATIONS/A_AGIL_NATURAL_PULIDA_FIX02/scripts")
import j3d
A=W+"/darknut_fable52/assets_private/"
M=j3d.Bmd.parse(open(A+"B_tnp2/bmdr/tn2.bmd","rb").read())
def app(G,p): return [G[i][0]*p[0]+G[i][1]*p[1]+G[i][2]*p[2]+G[i][3] for i in range(3)]
cl={k:j3d.Bck.parse(open(A+"B_tn/bck/%s.bck"%k,"rb").read()) for k in ("tnb_down_sippu","tnb_down","tn2b_die","tnb_wait","tnb_guard_a","tnb_guard_b","tnb_damage_l","tnb_damage_r")}
P={}
def pose(k,f):
    if (k,f) not in P:
        G=j3d.fk(M,j3d.bck_pose(cl[k],f)); P[(k,f)]=[app(G[j],(0,0,0)) for j in range(29)]+[app(G[15],(110,0,0))]
    return P[(k,f)]
def dist(a,b): return sum(math.dist(x,y) for x,y in zip(a,b))/len(a)
for k in cl: print(k,"frames",cl[k].duration)
print("== first stagger hold (SIPPU f10..f18) -> TNB_DOWN start frame")
for h in (10,12,14,16,18):
    print("  sippu f%d:"%h, " ".join("d%d=%.0f"%(s,dist(pose("tnb_down_sippu",h),pose("tnb_down",s))) for s in range(0,9)))
print("== distinctness: TNB_DOWN hold frames vs SIPPU hold frames (min over pairs)")
for f in (6,8,10,12,14,16):
    print("  down f%d min_dist_to_sippu=%.0f head_y=%.0f"%(f,min(dist(pose("tnb_down",f),pose("tnb_down_sippu",g)) for g in range(0,35,2)), app(j3d.fk(M,j3d.bck_pose(cl['tnb_down'],f))[5],(0,0,0))[1]))
print("== TNB_DOWN recovery end vs TNB_WAIT 0:", round(dist(pose("tnb_down",36),pose("tnb_wait",0)),1), " SIPPU end vs WAIT 0:", round(dist(pose("tnb_down_sippu",33),pose("tnb_wait",0)),1))
print("== getup end: TN2B_DIE frame f (reversed playback ends there) vs TNB_WAIT 0")
for f in (0,4,8,12,16,20,24,26,28,30,32,34):
    print("  die f%d -> wait d=%.0f"%(f,dist(pose("tn2b_die",f),pose("tnb_wait",0))))
