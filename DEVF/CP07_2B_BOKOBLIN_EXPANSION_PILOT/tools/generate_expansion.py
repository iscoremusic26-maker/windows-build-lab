"""Regenerate only own C++ encounter data from a user export; never reads ROM/assets."""
import argparse,json,math
from pathlib import Path
def object_unique(pairs):
 out={}
 for k,v in pairs:
  if k in out:raise ValueError("DUPLICATE_KEY")
  out[k]=v
 return out
def generate(raw):
 if not raw or len(raw)>16384:raise ValueError("EXPORT_SIZE")
 doc=json.loads(raw.decode("utf-8"),object_pairs_hook=object_unique,parse_constant=lambda s:(_ for _ in ()).throw(ValueError("NONFINITE")))
 if set(doc)!={"document_type","schema_version","producer","runtime_applied","encounters"} or doc["document_type"]!="devf_encounter_export" or type(doc["schema_version"]) is not int or doc["schema_version"]!=1 or doc["runtime_applied"] is not False or doc["producer"] not in ["dev.devf.cp06_native_config_dual@0.7.2","dev.devf.cp06_native_config_dual@0.7.3"]:raise ValueError("EXPORT_SCHEMA")
 rows=doc["encounters"]
 if type(rows) is not list or len(rows)!=2:raise ValueError("PILOT_TWO_CAPTURES_REQUIRED")
 ids=set();names=set();sites=set();lines=[]
 fields={"encounter_id","name","enemy","variant_id","native_profile","stage","room","layer","x","y","z","yaw_s16","capture_yaw_s16","quantity","spacing_units","respawn_policy","respawn_applied","capture_origin","capture_tick","capture_validity","terrain_verified"}
 for e in rows:
  if type(e) is not dict or set(e)!=fields:raise ValueError("ENCOUNTER_FIELDS")
  ident=e["encounter_id"]
  if not isinstance(ident,str) or not 1<=len(ident)<=31 or any(c not in "abcdefghijklmnopqrstuvwxyz0123456789_-" for c in ident) or ident in ids:raise ValueError("ENCOUNTER_ID")
  ids.add(ident)
  if not isinstance(e["name"],str) or not 1<=len(e["name"].encode())<=96 or any(ord(c)<32 or ord(c)==127 for c in e["name"]) or e["name"] in names:raise ValueError("ENCOUNTER_NAME")
  names.add(e["name"])
  for k in ["native_profile","room","layer","yaw_s16","capture_yaw_s16","quantity","spacing_units","capture_tick"]:
   if type(e[k]) is not int:raise ValueError("INTEGER_FIELD")
  if e["enemy"]!="E_OC2" or e["variant_id"]!="native_elite" or e["native_profile"]!=510 or e["stage"]!="F_SP121" or e["room"]!=3 or e["layer"]!=6 or e["quantity"]!=1:raise ValueError("PILOT_NATIVE_CONTEXT_ONLY")
  if not -32768<=e["yaw_s16"]<=32767 or not -32768<=e["capture_yaw_s16"]<=32767 or not 0<=e["spacing_units"]<=1000 or not 1<=e["capture_tick"]<2**53 or e["respawn_policy"] not in ["on_zone_reentry","no_respawn_during_session"] or e["respawn_applied"] is not False or e["terrain_verified"] is not False or e["capture_origin"]!="link_readonly_v203" or e["capture_validity"]!="READ_ONLY_SNAPSHOT_NOT_TERRAIN_VERIFIED":raise ValueError("UNSUPPORTED_METADATA")
  numbers=[]
  for k in ["x","y","z"]:
   n=e[k]
   if type(n) not in [int,float] or not math.isfinite(n) or abs(n)>1000000:raise ValueError("COORDINATE")
   import struct
   if struct.unpack("<f",struct.pack("<f",n))[0]!=n:raise ValueError("FLOAT32_CAPTURE_REQUIRED")
   t=format(float(n),".17g")
   if not any(c in t for c in ".eE"):t+=".0"
   numbers.append(t+"f")
  site=tuple(e[k] for k in ["x","y","z"])
  if site in sites:raise ValueError("DUPLICATE_POSITION")
  sites.add(site)
  lines.append(' {"'+ident+'","F_SP121",3,6,'+','.join(numbers)+','+str(e["yaw_s16"])+',510,1}')
 return '#pragma once\n#include <array>\n#include <cstdint>\nnamespace expansion {\nstruct Captured {const char* id;const char* stage;int room,layer;float x,y,z;int yaw;unsigned profile,quantity;};\ninline constexpr std::array<Captured,2> captured{{\n'+',\n'.join(lines)+'\n}};\n}\n'
if __name__=="__main__":
 p=argparse.ArgumentParser();p.add_argument("input",type=Path);p.add_argument("output",type=Path);p.add_argument("--check",action="store_true");a=p.parse_args()
 text=generate(a.input.read_bytes())
 if a.check:
  if a.output.read_text(encoding="utf-8").replace("\r\n","\n")!=text:raise SystemExit("GENERATED_SOURCE_DRIFT")
  print("EXPANSION_GENERATION_VERIFIED")
 else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text,encoding="utf-8",newline="\n")
