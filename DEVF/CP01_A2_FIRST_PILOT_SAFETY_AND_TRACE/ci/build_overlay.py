"""Small reviewed overlays of own prior sources, generated in build only. No host/SDK patch."""
import argparse,json,hashlib,difflib,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--project',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
project=a.project.resolve();old=project.parent/'CP01_A2_FIRST_NATIVE_REQUEST_OFFLINE';g4=project.parent/'CP01_G4B_LOCAL_READONLY_OBSERVER'
edits=json.loads((project/'ci/overlay-edits.json').read_text());out=a.output.resolve();(out/'src').mkdir(parents=True,exist_ok=True);(out/'tests').mkdir(exist_ok=True);ledger={};diff=[]
files=['src/experiment_core.hpp','src/experiment_core.cpp','src/native_backend.hpp','src/native_backend.cpp','src/admission.cpp','tests/fake.hpp']
for rel in files+['g4b-census']:
 source=g4/'src/extended_g2_reader.cpp' if rel=='g4b-census' else old/rel
 original=source.read_text(encoding='utf-8');updated=original
 for before,after in edits.get(rel,[]):
  if updated.count(before)!=1:raise RuntimeError('Overlay anchor not unique: '+rel+' '+before[:80])
  updated=updated.replace(before,after)
 # Only relative own includes relocated by generation; public SDK headers never edited.
 if rel=='g4b-census':
  updated=updated.replace('"../../CP01_G2_READONLY_PACKAGE/src/core.hpp"','"'+(project.parent/'CP01_G2_READONLY_PACKAGE/src/core.hpp').as_posix()+'"')
 target=out/('src/census_reader.cpp' if rel=='g4b-census' else rel);target.write_text(updated,encoding='utf-8',newline='\n')
 sha=lambda s:hashlib.sha256(s.encode()).hexdigest()
 normalized=updated.replace(project.parent.as_posix(),'<DEVF>')
 ledger[rel]={'input_lf_sha256':sha(original),'output_lf_sha256':sha(updated),'output_normalized_sha256':sha(normalized),'reviewed_edits':len(edits.get(rel,[]))}
 diff.extend(difflib.unified_diff(original.replace(project.parent.as_posix(),'<DEVF>').splitlines(True),normalized.splitlines(True),fromfile='base/'+rel,tofile='pilot-overlay/'+rel))
(out/'overlay-ledger.json').write_text(json.dumps(ledger,indent=2)+'\n')
(out/'overlay-diff.log').write_text(''.join(diff),encoding='utf-8')
print('PASS own-source overlay: '+str(len(ledger))+' inputs; source/SDK untouched')
