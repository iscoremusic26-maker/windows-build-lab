"""CP20 integration boundary: freeze FIX02 policies and all nonidentity adapter code."""
import hashlib,json,difflib
from pathlib import Path
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(repo,task):
 old=repo/'DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP';rt=task/'runtime/DEVF';base=old/'runtime/DEVF'
 allowed={'CP10_2_BLACK_BOKOBLIN/shared/config.hpp','CP10_2_BLACK_BOKOBLIN/expansion/client.hpp','CP10_2_BLACK_BOKOBLIN/expansion/config_io.hpp','CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp','CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp','CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/mod.cpp'}
 frozen={}
 for p in base.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(base).as_posix()
  if rel in allowed:continue
  assert p.read_bytes()==(rt/rel).read_bytes(),('FROZEN_RUNTIME_CHANGED',rel);frozen[rel]=sha(p)
 def section(s,start,end):return s[s.index(start):s.index(end,s.index(start))]
 def remove(s,start,end):return s.replace(section(s,start,end),'')
 rel='CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/mod.cpp';a=(base/rel).read_text();b=(rt/rel).read_text()
 for start,end in [('devf_black2::Snapshot record_snapshot(', '// Same read-only effective-resource contract'),('MOD_EXPORT ModResult mod_initialize(', 'MOD_EXPORT ModResult mod_update(')]:
  a=remove(a,start,end);b=remove(b,start,end)
 b=remove(b,'  const int ti=typed_table.candidate(s);','  const auto plan=devf_variant::make_plan(')
 b=b.replace('plan.source==devf_variant::Source::DevfBlue?"EXACT_DEVF_BLUE_E_OC_REGISTRATION":plan.source==devf_variant::Source::DevfRed?"EXACT_DEVF_RED_E_OC2_REGISTRATION":','')
 b=b.replace('if(table.armed&&have_snapshot&&a->field_0x560==devf_variant::kNativeHpE_OC2){','if(table.armed&&have_snapshot){')
 b=b.replace('#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/typed_registration.hpp"\n','').replace('devf_typed::Table typed_table;\n','').replace('IMPORT_SERVICE(DevfEncounterPlacementServiceV11,svc_devf_encounter_v11);','IMPORT_SERVICE(DevfEncounterPlacementService,svc_devf_encounter);').replace('kVersion="0.16.3"','kVersion="0.16.2"')
 assert a==b,'ADAPTER_CHANGE_OUTSIDE_IDENTITY_BOUNDARY'
 # Full function bodies for universal audio, shield, flash, all AI, lifesteal,
 # hooks, polling, collision reads, unload and render remain byte-identical.
 cp13=repo/'DEVF/CP13_HYRULE_FIELD_ALL_ROOMS/runtime/DEVF'
 provider_frozen={}
 for p in cp13.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(cp13).as_posix()
  if '/provider/' in rel and not rel.startswith('CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/encounter_broker') and not rel.endswith('CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/mod.cpp'):
   assert p.read_bytes()==(rt/rel).read_bytes(),rel;provider_frozen[rel]=sha(p)
 return dict(status='CP20_SOURCE_FREEZE_PASS',baseline='a0dd9e5a780c413a27fd165c9d3133b14d1e8613',frozen_runtime=frozen,frozen_provider=provider_frozen,adapter_changes_only=['record_snapshot','on_execute_body identity classification','mod_initialize service and registrations','version/import/globals'],host_modified=False,game_executed=False)
if __name__=='__main__':
 task=Path(__file__).resolve().parents[1];print(json.dumps(check(task.parents[1],task),indent=2))
