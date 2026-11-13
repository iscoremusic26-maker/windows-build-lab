"""CP06 0.11.11 additive barrier. Every pre-existing production file must equal the
compiled 0.11.10 bytes after the exact, counted CP06 inverse patch; new CP06
modules are hash-pinned; the only new native writer is the attributed chip
restore. No repinning of older combat, Link, HUD or foreign sources.
"""
from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import cp06f1_audit  # 0.11.12: FIX01 inverse patch first (chained), then this CP06 layer
def policy():return json.loads((HERE/'cp06_policy.json').read_text(encoding='utf-8'))
def sha(b):return hashlib.sha256(b).hexdigest()
def legacy_source(name,text):
 text=cp06f1_audit.legacy_source(name,text)
 for e in reversed(policy()['inverse_patch'].get(name,[])):
  if text.count(e['after'])==e.get('count',1):text=text.replace(e['after'],e['before'],e.get('count',1))
 return text
def audit(project):
 p=Path(project);pin=policy();errors=[]
 expected={'src/cp06_combat_core.hpp','src/cp06_combat_adapter.inc'}
 if set(pin['files'])!=expected|{'tests/reference/cp06_expansion_cp19f2_chip.inc'}:errors.append('CP06 exact two-module + Expansion chip reference inventory required')
 if len(pin['baseline_files'])!=pin['baseline_count']:errors.append('CP06 frozen 0.11.10 production inventory count changed')
 present={f.relative_to(p).as_posix() for f in (p/'src').rglob('*') if f.is_file()}-{'src/'+n for n in cp06f1_audit.NEW|cp06f1_audit.liz_audit.NEW|cp06f1_audit.liz_audit.kk_audit.NEW|cp06f1_audit.liz_audit.kk_audit.dkr_audit.NEW}
 if present!=set(pin['baseline_files'])|expected:errors.append('CP06 production file set differs from 0.11.10 + two CP06 modules: %s'%sorted(present^(set(pin['baseline_files'])|expected)))
 for name,edits in pin['inverse_patch'].items():
  t=cp06f1_audit.legacy_source(name,(p/'src'/name).read_text(encoding='utf-8'))
  for e in reversed(edits):
   if t.count(e['after'])!=e.get('count',1):errors.append('CP06 exact integration changed: '+name)
   else:t=t.replace(e['after'],e['before'],e.get('count',1))
 for rel,want in pin['baseline_files'].items():
  f=p/rel
  if not f.is_file():errors.append('CP06 0.11.10 production missing: '+rel);continue
  if sha(legacy_source(f.name,f.read_text(encoding='utf-8')).encode())!=want:errors.append('CP06 0.11.10 production changed outside exact authorized edits: '+rel)
 for rel,want in pin['files'].items():
  f=p/rel
  if not f.is_file() or sha(f.read_bytes())!=want:errors.append('CP06 reviewed new module changed: '+rel)
 adapter=(p/'src/cp06_combat_adapter.inc').read_text(encoding='utf-8')
 for bad in ['restore_reaction(','write_vitality(','setActionMode(','setPlaySpeed(','setBck(','setFrame(','OnAtSetBit(','OffAtSetBit(','ClrAtHit(','ClrTgHit(','SetC(','SetR(','HOOK_SKIP_ORIGINAL','VirtualProtect','WriteProcessMemory','get_service(','procGuard','mItemTrigger','mProcID','health','clearItemLifeCount','dComIfGp_setItemLifeCount','setLife(','getPlayerStatusA().set']:
  if bad in adapter:errors.append('CP06 forbidden native operation: '+bad)
 if re.search(r'->[\w.\[\]]+\s*=(?!=)',adapter):errors.append('CP06 native pointer assignment forbidden')
 if adapter.count('setItemLifeCount(')!=1 or adapter.count('g_dComIfG_gameInfo.play.setItemLifeCount(units,type);')!=1:errors.append('CP06 exactly one attributed pending-life restore writer required')
 body=adapter[adapter.index('CP06_NOINLINE bool cp06_native_chip_restore('):]
 body=body[:body.index('\n}\n')]
 if 'if(units!=cp06::kChipUnits)return false;' not in body or 'getItemLifeCount()==expected' not in body:errors.append('CP06 restore writer lost its exact-unit guard or readback')
 for token in ['before.priority=90;after.priority=-250;','add_post<Cp04EocExecute>(on_eoc_post_cp06_capture,&before)','add_post<Cp04EocExecute>(on_eoc_post_cp06_chip,&after)','c6.enabled=c5.enabled&&','if(!c6.enabled||!ok||!in.fury_verified','cp06::fury_parry_authenticated(','profile.variant==postbal01::Variant::BlackFury','cp06::fury_parry_authenticated(e2,r.aux.atp,r.aux.anim_ok!=0,r.aux.anim_rate,fury);','c.cCcD_ObjHitInf::ChkTgHit()&&c.ChkTgShieldHit()','g.p8.layout!=1||!cp06_native_chip_restore']:
  if token not in adapter:errors.append('CP06 production proof/binding missing: '+token)
 if adapter.count('add_pre<')or adapter.count('add_post<')!=2:errors.append('CP06 must add exactly two POST callbacks on the existing E_OC execute target')
 core=(p/'src/cp06_combat_core.hpp').read_text(encoding='utf-8')
 for bad in ['uintptr_t','void*','daE_OC_c','daAlink_c','cM_rnd','g_dComIfG']:
  if bad in core:errors.append('CP06 value core contains native address/RNG/global: '+bad)
 for token in ['return e_oc2&&atp==kFuryAtp&&anim_ok&&anim_rate==kFuryRate&&variant_fury_verified;','if(!x.before.shield_pair||now.pending!=x.before.pending-kChipUnits)return record(Chip::DeltaMismatch);','if(now.life!=x.before.life)return record(Chip::LifeChanged);','if(now.type!=x.before.type)return record(Chip::TypeChanged);']:
  if token not in core:errors.append('CP06 attribution rule missing: '+token)
 c5=(p/'src/cp05_combo_core.hpp').read_text(encoding='utf-8')
 for token in ['constexpr float r4[]={1.05f,1.35f,1.35f,1.05f},b8[]={1.05f,1.05f,1.30f,1.30f,1.30f,1.30f,1.30f,1.05f},b7[]={1.30f,1.30f,1.05f,1.05f,1.20f,1.20f,1.20f};','constexpr unsigned r[]={60,36,62},b8[]={62,62,38,38,38,38,64},b7[]={38,62,64,47,47,47};','static_assert(kBlackMaxRate<1.5f,']:
  if token not in c5:errors.append('CP06 approved Red/Black profile changed: '+token)
 c4=(p/'src/cp04_combo_core.hpp').read_text(encoding='utf-8')
 if 'kModerateGap=8, kShortGap=2' not in c4 or 'kOpenRate=1.10f,kFastRate=1.5f;' not in c4:errors.append('CP06 approved Blue profile changed')
 pc=(p/'src/parry_core.hpp').read_text(encoding='utf-8')
 for token in ['inline constexpr float kWindowFrames = 7.0f;','inline constexpr float kPreStartSlack = 1.0f;','inline constexpr std::uint32_t kWindowTicksCap = 9;','inline constexpr float kFuryRateThreshold = 1.5f;']:
  if token not in pc:errors.append('CP06 parry window/legacy fury bound changed: '+token)
 return errors
if __name__=='__main__':
 e=audit(Path(sys.argv[1]));print(json.dumps({'status':'SOURCE_AUDIT_FAIL' if e else 'SOURCE_AUDIT_PASS','base_source':policy()['base_source'],'frozen_prior_files':policy()['baseline_count'],'new_native_writers':['cp06_native_chip_restore'],'errors':e},indent=2));sys.exit(bool(e))
