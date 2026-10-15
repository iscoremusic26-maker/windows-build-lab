import copy,json
from generate_expansion import generate
from pathlib import Path
root=Path(__file__).resolve().parents[1];raw=(root/"inputs/user-encounters.export.json").read_bytes();base=json.loads(raw);count=0
assert generate(raw)==(root/"expansion/src/generated_encounters.hpp").read_text(encoding="utf-8");count+=1
def reject(name,change):
 global count
 d=copy.deepcopy(base);change(d)
 try:generate(json.dumps(d,ensure_ascii=False).encode())
 except (ValueError,KeyError,TypeError,OverflowError):count+=1;print("GENERATOR_NEGATIVE_PASS "+name)
 else:raise RuntimeError("INVALID_SURVIVED "+name)
for key,value in [("stage","D_MN01"),("room",2),("layer",4),("quantity",2),("native_profile",999),("enemy","E_OC"),("variant_id","black_guard"),("terrain_verified",True),("respawn_applied",True),("capture_origin","manual"),("x",float("nan")),("z",float("inf")),("x",0.1),("yaw_s16",32768)]:
 reject(key,lambda d,k=key,v=value:d["encounters"][0].__setitem__(k,v))
reject("third-encounter",lambda d:d["encounters"].append(copy.deepcopy(d["encounters"][0])))
reject("duplicate-id",lambda d:d["encounters"][1].__setitem__("encounter_id",d["encounters"][0]["encounter_id"]))
reject("duplicate-site",lambda d:[d["encounters"][1].__setitem__(k,d["encounters"][0][k]) for k in ["x","y","z"]])
reject("injection-id",lambda d:d["encounters"][0].__setitem__("encounter_id",'bad";'))
reject("unknown-field",lambda d:d.__setitem__("unknown",1))
print("EXPANSION_GENERATOR_PASS cases="+str(count))
