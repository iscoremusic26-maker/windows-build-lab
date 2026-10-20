"""Fail the build if any frozen input changed; embed the LF document without reserializing it."""
import sys,shutil
from pathlib import Path
TASK=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(TASK/'inputs'))
from verify_inputs import verify

def generate(destination):
 evidence=verify(TASK/'inputs')
 raw=(TASK/'inputs/encounters.json').read_bytes().replace(b'\r\n',b'\n')
 h=14695981039346656037
 for c in raw:h=((h^c)*1099511628211)&0xffffffffffffffff
 text='''#pragma once
#include <cstdint>
#include <cstddef>
namespace cp21_frozen {
inline constexpr char json[]=R"CP21JSON('''+raw.decode()+''')CP21JSON";
constexpr uint64_t digest(){uint64_t h=14695981039346656037ull;for(size_t i=0;i<sizeof(json)-1;++i)h=(h^static_cast<unsigned char>(json[i]))*1099511628211ull;return h;}
static_assert(sizeof(json)-1==18449,"Frozen encounter length changed");
static_assert(digest()=='''+str(h)+'''ull,"Frozen encounter bytes changed");
}
'''
 destination=Path(destination);destination.parent.mkdir(parents=True,exist_ok=True)
 if not destination.exists() or destination.read_bytes()!=text.encode():destination.write_bytes(text.encode())
 for role in ['provider','expansion']:
  res=TASK/role/'res';res.mkdir(exist_ok=True)
  for n in ['icon.png','banner.png']:
   src=TASK/'inputs/assets'/role/n;dst=res/n
   if not dst.exists() or dst.read_bytes()!=src.read_bytes():shutil.copyfile(src,dst)
 return evidence
if __name__=='__main__':
 import json
 print(json.dumps(generate(sys.argv[1]),indent=2))
