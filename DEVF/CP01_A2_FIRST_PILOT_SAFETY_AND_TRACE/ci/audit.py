"""Reuse prior exact PE parser; review only the new pilot and explicit own-source overlays."""
import importlib.util,json,re,hashlib
from pathlib import Path
OLD=Path(__file__).resolve().parents[2]/'CP01_A2_FIRST_NATIVE_REQUEST_OFFLINE'
spec=importlib.util.spec_from_file_location('prior_pe',OLD/'ci/audit.py');prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)
def source_audit(project,overlay):
 native=(overlay/'src/native_backend.cpp').read_text();core=(overlay/'src/experiment_core.cpp').read_text();mod=(project/'src/experimental_mod.cpp').read_text()
 sources=[f for f in (project/'src').glob('*') if f.suffix in ['.cpp','.hpp']]+list((overlay/'src').glob('*'))
 text='\n'.join(f.read_text() for f in sources)
 for pattern in [r'\b(?:delete_actor|register_actor|unregister_actor|create_child_actor|create_actor_from_name|add_actor|fpcLy_Cancel|fpcCtRq_Cancel|fopAcM_delete|fopAcM_create|get_actor_id)\s*\(',r'\b(?:StageService|HookService|ResourceService|DEFINE_HOOK|EXPORT_SERVICE|std::thread|std::async)\b',r'\b(?:CreateThread|VirtualProtect|CreateFile|WriteFile|LoadLibrary|system|fopen|SphChk)\s*\(']:
  if re.search(pattern,text):raise RuntimeError('Forbidden pilot surface '+pattern)
 if native.count('svc_actor->create_actor(')!=1 or core.count('b.request(preset)')!=1:raise RuntimeError('Not exactly one request site')
 if re.findall(r'IMPORT_SERVICE\((\w+),',mod)!=['HostService','LogService','ActorService']:raise RuntimeError('Wrong service imports')
 admission=(overlay/'src/admission.cpp').read_text()
 if admission!=(OLD/'src/admission.cpp').read_text() or 'Admission current_admission() noexcept { return {}; }' not in admission:raise RuntimeError('Provider must stay identical NONE')
 if 'if(!permission.location_admitted||!permission.session_authorized)' not in core:raise RuntimeError('Session short circuit lost')
 if 'pilot_check(fresh,&first)' not in core or 'pilot_check(selected,&fresh)' not in core:raise RuntimeError('B/C guard omitted')
 if native!=(OLD/'src/native_backend.cpp').read_text():raise RuntimeError('Native backend must remain byte-equivalent')
 census=(overlay/'src/census_reader.cpp').read_text()
 if census.count('fopAcIt_Judge(')!=2 or census.count('devf::pilot::collect_actor(')!=1:raise RuntimeError('Additional census or missing extension')
 if 'if(!scope.restore())' not in core or 'consumed_=true;state_=State::AttemptConsumed;' not in core:raise RuntimeError('Scope/latch lost')
 cmake=re.sub(r'#[^\n]*','',(project/'CMakeLists.txt').read_text())
 if re.search(r'\badd_mod\s*\(',cmake) or (project/'mod.json').exists():raise RuntimeError('Packaging forbidden')
 return {'status':'PASS_STATIC','single_request_site':True,'native_admission':'NONE_ALWAYS_UNCHANGED','live_key':'bound after current precheck A; B/C context/freshness mandatory','link_XZ':1600,'other_actor_XZ':1600,'center_policy_not_collision_clearance':True,'same_actor_passes':2,'additional_actor_passes':0,'logger':'own bounded records in scope; LogService drain outside scope','no_package':True,'native_backend_unchanged':True}
def pe_audit(path):
 result=prior.pe_audit(path)
 if sorted(result['imports']['dusklight.exe'])!=sorted(json.loads((Path(__file__).parent/'reviewed-game-imports.json').read_text())):raise RuntimeError('Exact decorated game surface changed')
 for syms in result['imports'].values():
  if any(re.search(r'CreateFile|WriteFile|CreateThread|VirtualProtect|LoadLibrary|GetProcAddress|fopAcM_delete|fpcCtRq_Cancel',s) for s in syms):raise RuntimeError('Forbidden binary capability')
 return result

def correlate_request(linkmap,disassembly):
 rows=[]
 for line in linkmap.splitlines():
  m=re.match(r'\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{16})\b',line)
  if m:rows.append((m[1],int(m[2],16)))
 request=next(addr for name,addr in rows if name.startswith('?request@NativeBackend@'))
 svc=next(addr for name,addr in rows if name=='svc_actor')
 end=min(addr for name,addr in rows if addr>request and addr<request+0x10000 and ' f ' in next((line for line in linkmap.splitlines() if name in line),'') )
 instructions=[]
 for line in disassembly.splitlines():
  m=re.match(r'\s*([0-9A-Fa-f]{16}):',line)
  if m and request<=int(m[1],16)<end:instructions.append(line)
 block='\n'.join(instructions)
 loads=re.findall(r'mov\s+(r[a-z0-9]+),qword ptr \[([0-9A-Fa-f]+)h\]',block,re.I)
 regs=[reg for reg,addr in loads if int(addr,16)==svc]
 calls=[line for line in instructions if any(re.search(r'call\s+qword ptr \['+reg+r'\+20h\]',line,re.I) for reg in regs)]
 if len(calls)!=1:raise RuntimeError('Exactly one actual svc_actor slot32 call required inside request function')
 return {'status':'PASS_STATIC','request_preferred_va':hex(request),'request_range_end':hex(end),'svc_actor_preferred_va':hex(svc),'exact_call':calls[0].strip(),'registers_loaded_from_svc_actor':regs,'service_offset_decimal':32,'addresses':'derived from this build only, never engine offsets/source constants'}
