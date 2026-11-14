import os
import aud, numpy as np, glob, os, sys
root=os.path.join(os.environ.get("DUSK_WORK", "."), "darknut_combo_audios")
out=sys.argv[-1]
for f in sorted(glob.glob(root+"/FASE_*/*.m4a")):
    s=aud.Sound(f)
    rate=s.specs[0]
    d=s.data()
    tag=f.split("/")[-2]+"__"+os.path.basename(f)[:40].replace(" ","_")
    np.save(os.path.join(out,tag+".npy"), d.astype(np.float32))
    print(tag, rate, d.shape, d.shape[0]/rate)
