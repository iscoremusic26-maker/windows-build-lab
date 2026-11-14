import os
import numpy as np, glob, os, sys
d=sys.argv[-1]
exp={"FASE_1__Darknut_combo_1":[0.68,1.05,1.90,2.30,3.19],"FASE_1__Combo_2":[0.58,2.13,2.84],"FASE_1__Darknut_combo_3":[0.57,0.98,2.10,3.23,4.04],
"FASE_1__Combo_4":[0.52,2.04,2.76,3.47,4.63],"FASE_2__Combo1":[0.42,0.70,0.99,1.25,1.54,1.85,2.68],"FASE_2__Comb_2":[0.60,1.49,1.70,2.84,3.10,3.38,3.68],
"FASE_2__Combo_fase":[0.63,1.28,1.49,2.32,2.95,3.16,3.98,4.29,4.63],"FASE_2__Comb_4":[0.64,1.52,2.31,2.52,3.53]}
sr=44100; hop=int(sr*0.005)
for f in sorted(glob.glob(d+"/*.npy")):
    x=np.load(f)[:,0].astype(np.float64)
    k=[v for kk,v in exp.items() if os.path.basename(f).startswith(kk)][0]
    n=len(x)//hop
    env=np.array([np.sqrt(np.mean(x[i*hop:(i+1)*hop]**2)) for i in range(n)])
    db=20*np.log10(env+1e-6)
    # onset: rise of envelope
    t=np.arange(n)*0.005
    floor=np.percentile(db,20)
    # for each expected peak find local max of envelope in +-0.08 s and onset (first frame > max-12dB going backwards)
    res=[]
    for e in k:
        i0=int((e-0.10)/0.005); i1=int((e+0.10)/0.005)
        j=i0+int(np.argmax(env[i0:i1]))
        # onset: walk back until env < 0.25*peak
        o=j
        while o>0 and env[o]>0.25*env[j]: o-=1
        res.append((t[j],t[o],db[j]))
    # global peak picking: local maxima of env, prominence threshold
    pk=[]
    thr=np.max(env)*0.18
    for i in range(1,n-1):
        if env[i]>=thr and env[i]==env[max(0,i-30):i+30].max(): pk.append(round(t[i],3))
    print(os.path.basename(f)[:45], "dur=%.3f floor=%.1fdB"%(len(x)/sr,floor))
    print("  expected:", k)
    print("  envmax  :", [round(r[0],3) for r in res])
    print("  onset   :", [round(r[1],3) for r in res])
    print("  levels  :", [round(r[2],1) for r in res])
    print("  auto pk :", pk)
    iv=np.diff([r[0] for r in res]); ie=np.diff(k)
    print("  intervals measured:", [round(v,3) for v in iv], " expected:", [round(v,2) for v in ie])
    print("  intervals ticks(30Hz):", [round(v*30,1) for v in iv])
