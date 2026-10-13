"""Scoped source/call surface + PE metadata/IAT audit, reusing historical G2 parser."""
import importlib.util,json,re,hashlib
from pathlib import Path
BASE=Path(__file__).resolve().parents[2]/'CP01_G2_READONLY_PACKAGE'
spec=importlib.util.spec_from_file_location('historical_g2_audit',BASE/'ci/audit.py')
g2=importlib.util.module_from_spec(spec);spec.loader.exec_module(g2)
EXTRA={'fpcLy_IsDeletingMesg','mProcID','mNextStayNo','dComIfGp_roomControl_checkStatusFlag'}
def source_audit(project):
 sources=list((project/'src').glob('*'))+[BASE/'src/core.hpp',BASE/'src/core.cpp',BASE/'src/native_reader.cpp']
 text='\n'.join(p.read_text(encoding='utf-8') for p in sources)
 for pattern in [r'\b(?:ActorService|StageService|HookService|ActorSpawnParams)\b',r'\b(?:create_actor|add_actor|delete_actor|register_actor|fopAcM_create|fpcSCtRq_Request|fpcLy_SetCurrentLayer|fpcCtRq_Cancel)\s*\(',r'\b(?:IMPORT_OPTIONAL_SERVICE|DEFINE_HOOK|EXPORT_SERVICE|std::thread|std::async)\b',r'\b(?:fopen|ofstream|CreateFile|WriteFile|CreateThread|VirtualProtect|setenv|getenv|system)\s*\(']:
  if re.search(pattern,text):raise RuntimeError('Forbidden production call surface: '+pattern)
 cmake=(project/'CMakeLists.txt').read_text(encoding='utf-8')
 module=cmake.split('add_mod(devf_a2_disarmed',1)[1].split('MOD_JSON mod.json)',1)[0]
 if 'tests/' in module or 'simulation' in module:raise RuntimeError('Fake backend linked into module')
 imports=re.findall(r'IMPORT_SERVICE\((\w+),',text)
 if imports!=['HostService','LogService']:raise RuntimeError('Unreviewed services')
 mod=(project/'src/disarmed_mod.cpp').read_text(encoding='utf-8')
 if any(x in mod for x in ['begin_simulation','requested(','attempt(','observe(']):raise RuntimeError('Simulation entered host call graph')
 # Historical source must remain byte-normalized identical to its reviewed ledger.
 review=json.loads((BASE/'evidence/pre-package-review.json').read_text())
 for i in review['own_reviewed_files']:
  p=BASE/i['path']
  if hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest()!=i['lf_sha256']:raise RuntimeError('Historical G2 changed: '+i['path'])
 return {'status':'PASS_STATIC','production_units':['disarmed_mod','controller_core','room_owner_resolver','G2/core','G2/native_reader','SDK/game_feature'],
 'only_service_callbacks':['Host metadata','Log.write'],'synchronous_visitors':['G2 match_actor','G2 census_actor'],
 'entry_graph':'mod_initialize -> service/header/version checks; mod_update -> capture_owner -> historical G2 capture + scalar/link checks; Controller.disarm; validate_owner; Log.write; mod_shutdown -> scalar disarm',
 'simulation_linked':False,'native_request_backend':False,'native_layer_setter':False,'historical_G2_unchanged':True}
def pe_audit(path):
 g2.GAME_IMPORT_BASES |= EXTRA
 result=g2.pe_audit(path)
 for dll,syms in result['imports'].items():
  for s in syms:
   if any(x in s for x in ['CreateThread','WriteFile','CreateFile','VirtualProtect','LoadLibrary','fopAcM_create','fpcSCtRq_Request','fpcLy_SetCurrentLayer']):raise RuntimeError('Forbidden native import '+s)
 result['status']='A2_DISARMED_BINARY_AUDIT_PASS'
 return result
