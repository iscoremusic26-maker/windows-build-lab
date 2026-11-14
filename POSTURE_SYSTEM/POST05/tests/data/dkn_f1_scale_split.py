# DKN01 FIX01: per-combo uniform time scale S for the phase-1 mace combos. Every audio interval is multiplied by the SAME S (the rhythm = the ratios
# between the separations is kept exactly). S is the smallest value for which every transition X->Y can be played with a cut c / start s whose pose
# distance (f1_transition.py metric) is <= DMAX, at a clip rate in [RMIN, RMAX] (heavy: never faster than RMAX x the native mace).
import os, sys, math, json
W=os.environ.get("DUSK_WORK", ".")
sys.path.insert(0, W+"/darknut_fable52/DARKNUT_NORMAL_RESURRECTION_ANIMATIONS/A_AGIL_NATURAL_PULIDA_FIX02/scripts")
import j3d
A=W+"/darknut_fable52/assets_private/"
M=j3d.Bmd.parse(open(A+"B_tnp2/bmdr/tn2.bmd","rb").read())
def app(G,p): return [G[i][0]*p[0]+G[i][1]*p[1]+G[i][2]*p[2]+G[i][3] for i in range(3)]
cl={k:j3d.Bck.parse(open(A+"B_tn/bck/%s.bck"%k,"rb").read()) for k in ("tna_atack_a","tna_atack_b")}
P={}
for k in cl:
    for f in range(0,83):
        G=j3d.fk(M,j3d.bck_pose(cl[k],f)); P[(k,f)]=[app(G[j],(0,0,0)) for j in range(29)]+[app(G[15],(46,-1,88))]
def dist(a,b): return sum(math.dist(x,y) for x,y in zip(a,b))/len(a)
imp={"A":22,"B":26}; key={"A":"tna_atack_a","B":"tna_atack_b"}
DMAX=float(os.environ.get("DMAX","70")); RMIN=float(os.environ.get("RMIN","0.7")); RMAX=float(os.environ.get("RMAX","1.25"))
opts={}
for X in "AB":
    for Y in "AB":
        o=[]
        for c in range(imp[X]+4,81):
            for s in range(0,imp[Y]-9):
                d=dist(P[(key[X],c)],P[(key[Y],s)])
                if d<=DMAX: o.append(((c-imp[X])+(imp[Y]-s),c,s,d))
        o.sort(); opts[X+Y]=o
        print("pair %s->%s options=%d min_frames=%s max_frames=%s"%(X,Y,len(o),o[0][0] if o else None,o[-1][0] if o else None))
combos={"F1-C1":("BABAB",[675,1045,1905,2300,3195]),"F1-C2":("BBB",[530,2130,2840]),"F1-C3":("ABBBA",[510,980,2100,3240,4045]),"F1-C4":("ABBAB",[520,2035,2740,3480,4635])}
def pick(pair,T):
    # the option whose rate is closest to 1.0 (natural speed), ties -> smaller pose distance
    best=None
    X,Y=pair[0],pair[1]
    for F,c,s,d in opts[pair]:
        wind=imp[Y]-s                      # Y's wind-up + swing: ALWAYS at the native rate 1.0
        rec=T-wind                         # ticks left for X's follow-through / recovery
        if rec<=0: continue
        r=(c-imp[X])/rec
        if RMIN<=r<=RMAX:
            sc=(abs(r-1.0)+0.004*d,)
            if best is None or sc<best[0]: best=(sc,F,c,s,d,r)
    return best
out={}
for name,(seq,pk) in combos.items():
    S=1.0
    while S<6.0:
        ok=True
        for i in range(len(seq)-1):
            if not pick(seq[i]+seq[i+1],(pk[i+1]-pk[i])*0.03*S): ok=False;break
        if ok: break
        S=round(S+0.01,2)
    rows=[]
    for i in range(len(seq)-1):
        T=(pk[i+1]-pk[i])*0.03*S; b=pick(seq[i]+seq[i+1],T)
        rows.append({"pair":seq[i]+"->"+seq[i+1],"audio_s":(pk[i+1]-pk[i])/1000,"new_s":round(T/30,3),"ticks":round(T,2),"cut":b[2],"start":b[3],"frames":b[1],"rate_recovery":round(b[5],3),"rate_windup":1.0,"pose_dist":round(b[4],1)})
    out[name]={"S":S,"total_audio_s":(pk[-1]-pk[0])/1000,"total_new_s":round((pk[-1]-pk[0])/1000*S,2),"pairs":rows}
    print(name,"S=%.2f"%S,"total %.2fs -> %.2fs"%((pk[-1]-pk[0])/1000,(pk[-1]-pk[0])/1000*S))
    for r in rows: print("   ",r)
json.dump(out,open(W+"/dkn_fix01_evidence/f1_scale_split_D%d_R%.2f.json"%(DMAX,RMAX),"w"),indent=1)
