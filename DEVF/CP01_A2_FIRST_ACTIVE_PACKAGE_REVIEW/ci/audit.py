"""Focused integration audit, reuses exact existing pilot PE/parser/correlation."""
import importlib.util,json,re,hashlib
from pathlib import Path
PILOT=Path(__file__).resolve().parents[2]/'CP01_A2_FIRST_PILOT_SAFETY_AND_TRACE'
spec=importlib.util.spec_from_file_location('pilot_audit',PILOT/'ci/audit.py');pilot=importlib.util.module_from_spec(spec);spec.loader.exec_module(pilot)
pe_audit=pilot.pe_audit;correlate_request=pilot.correlate_request
def sha(b):return hashlib.sha256(b).hexdigest()
def source_audit(project,overlay):
 auth=json.loads((project/'SOURCE_AUTHORITY.json').read_text())
 for rel,h in auth['reuse_lf_sha256'].items():
  if sha((project.parent/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Historical source drift '+rel)
 ledger=json.loads((overlay/'overlay-ledger.json').read_text())
 for rel,old in auth['prior_overlay'].items():
  if any(ledger[rel][k]!=old[k] for k in ['input_lf_sha256','output_normalized_sha256','reviewed_edits']):raise RuntimeError('Seven-overlay drift '+rel)
 native=(overlay/'src/native_backend.cpp').read_text();core=(overlay/'src/experiment_core.cpp').read_text();provider=(project/'src/active_admission.cpp').read_text();mod=(project/'src/active_mod.cpp').read_text()
 text='\n'.join(f.read_text() for d in [project/'src',overlay/'src'] for f in d.glob('*') if f.suffix in ['.cpp','.hpp'])
 for pattern in [r'\b(?:delete_actor|register_actor|unregister_actor|create_child_actor|create_actor_from_name|add_actor|fpcLy_Cancel|fpcCtRq_Cancel|fopAcM_delete|fopAcM_create|get_actor_id)\s*\(',r'\b(?:StageService|HookService|ResourceService|DEFINE_HOOK|EXPORT_SERVICE|std::thread|std::async)\b',r'\b(?:CreateThread|VirtualProtect|CreateFile|WriteFile|LoadLibrary|system|fopen|SphChk)\s*\(']:
  if re.search(pattern,text):raise RuntimeError('Forbidden module surface')
 if native.count('svc_actor->create_actor(')!=1 or core.count('b.request(preset)')!=1:raise RuntimeError('Request site drift')
 for s in ['a.location_admitted=true; a.session_authorized=true;','a.bind_context_live=true; a.xyz=candidate;']:
  if s not in provider:raise RuntimeError('Actual fixed provider missing')
 for s in ['if(!permission.location_admitted||!permission.session_authorized)','pilot_check(fresh,&first)','pilot_check(selected,&fresh)','if(terminal())return;','consumed_=true;state_=State::AttemptConsumed;','if(!scope.restore())']:
  if s not in core:raise RuntimeError('Historical guard omitted '+s)
 if re.findall(r'IMPORT_SERVICE\((\w+),',mod)!=['HostService','LogService','ActorService'] or 'PackageProviderArmed' not in mod or 'SessionAdmissionDenied' in mod:raise RuntimeError('False module identity/admission trace')
 if mod.count('contracts_available(')!=2 or 'initialized_once=true;' not in mod:raise RuntimeError('Production contract/lifetime gate lost')
 cmake=(project/'CMakeLists.txt').read_text()
 if cmake.count('add_mod(')!=1 or '"${OVERLAY}/src/admission.cpp"' in cmake or 'src/active_admission.cpp' not in cmake:raise RuntimeError('Wrong provider target')
 return {'status':'PASS_STATIC','provider':'TECHNICALLY_ARMED_FIXED_C_E_OC_LIVE_KEY','human_execution_authorized':False,'seven_overlays_identical':True,'native_backend_unchanged':True,'A_B_C_reentry_scope_latch_preserved':True,'request_sites':1,'runtime':'NOT_TESTED'}
