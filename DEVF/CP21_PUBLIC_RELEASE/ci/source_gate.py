import hashlib
from pathlib import Path
TASK=Path(__file__).resolve().parents[1];BASE=TASK.parent/'CP20_EXPLICIT_BLUE_RED_BLACK'
EXP='DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/mod.cpp'
PRO='DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/mod.cpp'

def check():
 frozen=[]
 for f in (BASE/'runtime').rglob('*'):
  if not f.is_file():continue
  n=f.relative_to(BASE/'runtime').as_posix();b=f.read_bytes();p=TASK/'runtime'/n
  assert p.is_file(),n
  if n not in [EXP,PRO]:assert p.read_bytes()==b,('RUNTIME_CHANGED',n);frozen.append(n)
 old=(BASE/'runtime'/EXP).read_text();new=(TASK/'runtime'/EXP).read_text()
 expected='#include "../../../../expansion/packaged_config.hpp"\n'+old.replace('expansion::load_config(mod_ctx,svc_host,svc_file,config,mode_reason)','packaged_config::load(config,mode_reason)').replace('0.16.3','0.17.0').replace('Expansion encounters.json invalid/unreadable; zero requests','Expansion packaged encounters invalid; zero requests')
 assert new==expected,'COMBAT_ADAPTER_CHANGED'
 s=(BASE/'runtime'/PRO).read_text()
 s=s.replace('#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/provider/src/encounter_ui.hpp"','#include "public_core.hpp"')
 s=s.replace('IMPORT_OPTIONAL_SERVICE(UiService,svc_ui);\n','').replace('native_config::Runtime core;encounter::Ui editor;','native_config::Runtime core;')
 s=s.replace('version=0.14.0','version=0.15.0').replace('reason=%s ui=%s evidence=REGISTRATION_AND_UI_LOGICAL_ONLY','reason=%s legacy_pairs=0 evidence=STAGE_EDIT_ONLY').replace(',core.error.code,editor.legacy.reason',',core.error.code')
 s=s.replace('core.initialize(mod_ctx,svc_host,svc_file,svc_stage)','public_core::initialize(core,mod_ctx,svc_host,svc_stage)')
 s=s.replace(' editor.initialize(mod_ctx,svc_host,svc_file,svc_ui);editor.event=log;\n','').replace('editor.shutdown();','').replace('editor.update();','')
 s=s.replace('const auto ui=const auto result=','const auto result=').replace('log("OWN_UI_AND_EDITS_REMOVED_NOT_LIVE_ACTORS",result!=MOD_OK?result:ui)','log("OWN_EDITS_REMOVED_NOT_LIVE_ACTORS",result)')
 s=s.replace(' // UI detach owns residual slots. Never alter actors or write config from shutdown.\n if(ui!=MOD_OK)return mods::set_error(error,ui,"Own UI cleanup failed; host UiService detach removes residual owned slots");\n','').replace(' // No UI IO, no reinjection',' // No IO, no reinjection')
 actual=(TASK/'runtime'/PRO).read_text();assert s==actual,'UNREVIEWED_PROVIDER_DELTA'
 for marker in ['UiService','encounter::Ui','editor7::Ui','editor.','placements.json','encounters.json','core.initialize(']:assert marker not in actual,marker
 helper=(TASK/'runtime/DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/public_core.hpp').read_text()
 assert 'registration.initialize(ctx,stage,core.config.active())' in helper and 'core.config={}' in helper
 assert all(x not in helper for x in ['FileService','data_dir','load(','UiService'])
 packaged=(TASK/'expansion/packaged_config.hpp').read_text();assert 'multi_config::decode(' in packaged
 assert all(x not in packaged for x in ['data_dir','files->','load_config('])
 for role in ['provider','expansion']:assert 'RES_DIR res' in (TASK/role/'CMakeLists.txt').read_text()
 return dict(status='SOURCE_IMPLEMENTED',runtime_files_frozen=len(frozen),combat_adapter='CP20 identical except include, input call and version',ABI_headers='unchanged',legacy_pairs=0,UiService='absent')
if __name__=='__main__':print(check())
