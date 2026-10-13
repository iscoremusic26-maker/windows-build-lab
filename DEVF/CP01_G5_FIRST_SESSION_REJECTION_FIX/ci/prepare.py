"""Generate pinned own-source overlays and a non-mutating Frame predicate unit."""
import argparse,subprocess,json,hashlib,difflib,shutil
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--project',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();project=a.project.resolve();out=a.output.resolve();pilot=project.parent/'CP01_A2_FIRST_PILOT_SAFETY_AND_TRACE'
subprocess.run(['python',str(pilot/'ci/build_overlay.py'),'--project',str(pilot),'--output',str(out)],check=True)
ledger={};diff=[]
def edit(rel,changes):
 f=out/rel;before=f.read_text();s=before
 for x,y in changes:
  if s.count(x)!=1:raise RuntimeError('Nonunique G5 overlay '+rel)
  s=s.replace(x,y)
 f.write_text(s,encoding='utf-8',newline='\n');h=lambda s:hashlib.sha256(s.encode()).hexdigest();ledger[rel]={'input_lf_sha256':h(before),'output_lf_sha256':h(s),'output_normalized_sha256':h(s.replace(project.parent.as_posix(),'<DEVF>')),'edits':len(changes)}
 diff.extend(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='pilot/'+rel,tofile='g5/'+rel))
edit('src/census_reader.cpp',[('pilot_link?double(a->speedF):0.0);','pilot_link?double(a->speedF):0.0,int(a->group));')])
# No native offset or SDK modification. Same visitor after init2/actorINIT, one additional scalar.
bridge=(pilot/'src/readonly_bridge.cpp').read_text().replace('v.link.speed=v.actors[i].speed;','v.link.group=v.actors[i].group;v.link.speed=v.actors[i].speed;');(out/'src/readonly_bridge.cpp').write_text(bridge)
for name in ['policy_values.hpp','policy.hpp','census_values.hpp']:shutil.copyfile(project/'src'/name,out/'src'/name)
# Exact predicate slice from reviewed real core, omitting Controller/LayerScope/request.
core=(out/'src/experiment_core.cpp').read_text();part=core[core.index('Key key('):core.index('bool LayerScope::restore()')]
(out/'src/frame_validation.cpp').write_text('#include "experiment_core.hpp"\n#include <cstring>\n#include <cmath>\n#include <limits>\nnamespace devf::first {\n'+part+'\n}\n')
terminal=core[core.index('void Controller::stop('):core.index('Key key(')]
note=core[core.index('void Controller::note('):core.index('bool Controller::pilot_check(')]
f=out/'src/frame_validation.cpp';s=f.read_text().replace('#include "experiment_core.hpp"','#include "experiment_core.hpp"\n#include "trace.hpp"');s=s[:-2]+note+terminal+'}\n';f.write_text(s)
(out/'g5-overlay-ledger.json').write_text(json.dumps(ledger,indent=2)+'\n');(out/'g5-overlay-diff.log').write_text(''.join(diff))
