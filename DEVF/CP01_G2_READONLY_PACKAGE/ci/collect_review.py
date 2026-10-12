"""Record the completed scoped human/source review; not an automated runtime safety proof."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
project=Path(__file__).resolve().parents[1]
root='40457c6adb381928e4b5fef6ed459ed291edd5e2'
if subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()!=root:
    raise SystemExit('Wrong official source')
references={
 'src/m_Do/m_Do_main.cpp':[302,326,493,932], 'src/f_ap/f_ap_game.cpp':[807,824,828],
 'src/dusk/mods/loader/loader.cpp':[210,228,269,296,334,1100,1237,1268],
 'src/dusk/mods/loader/natives.cpp':[29,128,496,563,575],
 'src/dusk/mods/loader/native_module.cpp':[11,18],
 'src/dusk/mods/svc/host.cpp':[177,204], 'src/dusk/mods/svc/log.cpp':[9,34],
 'src/dusk/mods/svc/game.cpp':[8], 'src/dusk/mods/svc/hook.cpp':[993],
 'src/dusk/mods/log_buffer.cpp':[33,57], 'src/f_pc/f_pc_manager.cpp':[48,52,90],
 'src/f_pc/f_pc_layer.cpp':[95,105,111,135,139], 'include/f_pc/f_pc_layer.h':[14],
 'src/f_pc/f_pc_base.cpp':[102,165], 'src/f_pc/f_pc_executor.cpp':[55,68],
 'src/f_pc/f_pc_deletor.cpp':[90,100], 'src/f_op/f_op_actor.cpp':[404,421,453],
 'src/f_op/f_op_actor_iter.cpp':[28], 'src/SSystem/SComponent/c_list_iter.cpp':[19],
 'src/SSystem/SComponent/c_node_iter.cpp':[28], 'src/SSystem/SComponent/c_tag_iter.cpp':[14],
 'src/f_op/f_op_scene.cpp':[32,48], 'src/f_op/f_op_scene_iter.cpp':[13],
 'src/f_op/f_op_scene_mng.cpp':[13], 'src/d/d_com_inf_game.cpp':[642,880,890],
 'src/d/d_resorce.cpp':[874], 'cmake/ModSDK.cmake':[173,311,400],
 'sdk/include/mods/svc/log.h':[28], 'sdk/include/mods/svc/game.h':[17],
 'sdk/src/game_feature.cpp':[5], 'src/dusk/mods/loader/bundle_zip.cpp':[7],
}
def record(path, relative, lines=None):
    raw=path.read_bytes().replace(b'\r\n',b'\n')
    if lines and max(lines)>len(raw.splitlines()): raise SystemExit(f'Invalid source anchor: {relative}')
    result={'path':relative,'lf_sha256':hashlib.sha256(raw).hexdigest()}
    if lines: result['anchor_lines']=lines
    return result
own=[]
for folder in ['src','tests','ci']:
    for f in sorted((project/folder).glob('*')):
        if f.is_file(): own.append(record(f,f.relative_to(project).as_posix()))
for name in ['CMakeLists.txt','observer.json','SAFETY_AUDIT.md']:
    own.append(record(project/name,name))
result={'decision':'PASS_STATIC_FOR_PACKAGING_REVIEW','unresolved_critical_risks':[],
        'runtime_safety':'NOT_VERIFIED','game_executed':False,'official_root':root,
        'base_checkpoint':'2124adddae1e4433f7523e777cf0a773b2119bb2',
        'review_scope':'Only existing read-only observer lifetimes, engine borrows and official package contract',
        'conditions':['Unmodified original 2.0.3 and epoch2','Synchronous original game-thread tick',
                      'Host-owned lists healthy; no conflicting actor/scene/hook mods',
                      'No native pointers, ActorIds or callbacks retained after capture',
                      'Complete successful Windows and packaged binary verification before publication'],
        'own_reviewed_files':own,'official_references':[
            record(a.source/path,path,lines) for path,lines in references.items()]}
(project/'evidence/pre-package-review.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('PASS_STATIC: reviewed code/source hashes fixed; runtime remains NOT_VERIFIED')
