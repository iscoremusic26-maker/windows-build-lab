"""Scoped production graph and exact PE import/metadata check; not arbitrary C++ proof."""
import importlib.util,re,hashlib,json
from pathlib import Path
BASE=Path(__file__).resolve().parents[2]/'CP01_G2_READONLY_PACKAGE'
A2=BASE.parent/'CP01_A2_DISARMED_CONTROLLER'
spec=importlib.util.spec_from_file_location('g2_pe',BASE/'ci/audit.py');g2=importlib.util.module_from_spec(spec);spec.loader.exec_module(g2)
EXTRA={'fpcLy_IsDeletingMesg','mProcID','mNextStayNo','dComIfGp_roomControl_checkStatusFlag',
 'GroundCross','ChkPolySafe','GetRoomId','GetGroundCode','GetSpecialCode','GetPolyColor','dComIfG_Bgsp',
 '0dBgS_GndChk','1dBgS_GndChk','SetObj','0dBgS_ObjGndChk_Wtr','1dBgS_ObjGndChk_Wtr','SetPos','dComIfGp_event_runCheck'}
def source_audit(project):
 files=list((project/'src').glob('*'))+[BASE/'src/core.cpp',BASE/'src/core.hpp',A2/'src/room_owner_resolver.cpp',A2/'src/controller_core.cpp']
 text='\n'.join(p.read_text(encoding='utf-8') for p in files)
 for pattern in [r'\b(?:ActorService|StageService|HookService|ResourceService|ActorSpawnParams)\b',r'\b(?:create_actor|add_actor|delete_actor|register_actor|fopAcM_create|fpcSCtRq_Request|fpcLy_SetCurrentLayer|fpcCtRq_Cancel|SphChk|WaterChk)\s*\(',r'\b(?:IMPORT_OPTIONAL_SERVICE|DEFINE_HOOK|EXPORT_SERVICE|std::thread|std::async)\b',r'\b(?:fopen|ofstream|CreateFile|WriteFile|CreateThread|VirtualProtect|setenv|getenv|system)\s*\(']:
  if re.search(pattern,text):raise RuntimeError('Forbidden production surface: '+pattern)
 imports=re.findall(r'IMPORT_SERVICE\((\w+),',text)
 if imports!=['HostService','LogService']:raise RuntimeError('Service surface')
 cmake=(project/'CMakeLists.txt').read_text();module=cmake.split('add_mod(devf_g4b_readonly',1)[1].split('MOD_JSON mod.json)',1)[0]
 if 'tests/' in module:raise RuntimeError('Fake in module graph')
 mod=(project/'src/mod.cpp').read_text()
 if any(word in mod for word in ['Controller','begin_simulation','requested(','attempt(','observe(']):raise RuntimeError('Dormant controller backend reached')
 local=(project/'src/local_queries.cpp').read_text()
 if 'bg.cBgS::GroundCross(&q)' not in local or 'bg.GroundCross' in local:raise RuntimeError('Unsafe debug wrapper')
 native=(project/'src/extended_g2_reader.cpp').read_text()
 if native.count('fopAcIt_Judge(')!=2:raise RuntimeError('Additional global census')
 for i in json.loads((BASE/'evidence/pre-package-review.json').read_text())['own_reviewed_files']:
  p=BASE/i['path']
  if hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest()!=i['lf_sha256']:raise RuntimeError('Historical G2 changed')
 return {'status':'PASS_STATIC','entry_graph':'initialize -> Host/Log/Game validation; update -> A2 resolver -> adapted SAME G2 player match + bounded census + BG owning sidecar -> private ground/water queries -> event/context scalar recheck -> evaluate -> bounded Log.write; shutdown -> own scalar only','historical_G2_unchanged':True,'services':'Host2.2/Log1.0/Game2 only','actor_queue_passes':'one existing membership + one existing census, no new census','new_virtual_calls':['ChkNotReady','ChkMoveBg','GetBnd','ChkLock'],'private_query_writes_only':True}
def pe_audit(path):
 g2.GAME_IMPORT_BASES |= EXTRA
 result=g2.pe_audit(path)
 expected=json.loads((Path(__file__).parent/'reviewed-imports.json').read_text())
 if {k:sorted(v) for k,v in result['imports'].items()}!={k:sorted(v) for k,v in expected.items()}:
  raise RuntimeError('Exact reviewed decorated game/CRT import surface changed')
 for dll,syms in result['imports'].items():
  for s in syms:
   if any(x in s for x in ['CreateThread','WriteFile','CreateFile','VirtualProtect','LoadLibrary','fopAcM_create','fpcSCtRq_Request','fpcLy_SetCurrentLayer','SphChk']):raise RuntimeError('Forbidden native import '+s)
 result['status']='G4B_LOCAL_OBSERVER_BINARY_AUDIT_PASS';return result
