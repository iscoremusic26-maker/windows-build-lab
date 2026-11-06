"""CP06: complete positive clone, then targeted source/writer/policy negatives against the full source wall."""
from pathlib import Path
import shutil,tempfile,subprocess,sys,json
root=Path(__file__).resolve().parents[1]
def clone(p):
 for x in ['src','tests','ci']:shutil.copytree(root/x,p/x,ignore=shutil.ignore_patterns('__pycache__'))
def audit(p):
 r=subprocess.run([sys.executable,'-I',str(p/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=120);return r,json.loads(r.stdout)
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);r,d=audit(p);assert r.returncode==0 and d['status']=='SOURCE_AUDIT_PASS',(r.stdout,r.stderr)
A='src/cp06_combat_adapter.inc';C='src/cp06_combat_core.hpp';M='src/mod.cpp'
cases=[
 ('fury_bit_not_bound_in_judge',M,'  in.fury_verified = cp06_fury_verified(r);\n','','CP06 exact integration changed: mod.cpp'),
 ('chip_window_not_armed_after_commit',M,'  cp06_parry_committed(in,v,ok);\n','','CP06 exact integration changed: mod.cpp'),
 ('late_contact_not_recorded',M,'    cp06_contact(in,v);\n','','CP06 exact integration changed: mod.cpp'),
 ('install_failure_ignored',M,'  if(!cp06_install())return mods::set_error(err,MOD_ERROR,"CP06 fury chip/timing hooks failed; no partial checkpoint");\n','  (void)cp06_install();\n','CP06 exact integration changed: mod.cpp'),
 ('second_life_writer',A,'void cp06_parry_committed(','void bad(){g_dComIfG_gameInfo.play.setItemLifeCount(4.f,0);}\nvoid cp06_parry_committed(','exactly one attributed pending-life restore writer'),
 ('global_life_clear',A,'void cp06_parry_committed(','void bad(){g_dComIfG_gameInfo.play.clearItemLifeCount();}\nvoid cp06_parry_committed(','forbidden native operation: clearItemLifeCount'),
 ('enemy_rate_writer',A,'void cp06_parry_committed(','void bad(daE_OC_c* a){a->mpMorf->setPlaySpeed(2);}\nvoid cp06_parry_committed(','forbidden native operation: setPlaySpeed('),
 ('enemy_HP_writer',A,'void cp06_parry_committed(','void bad(daE_OC_c* a){a->health=1;}\nvoid cp06_parry_committed(','native pointer assignment forbidden'),
 ('Link_proc_read_or_write',A,'void cp06_parry_committed(','void bad(daAlink_c* k){(void)k->mProcID;}\nvoid cp06_parry_committed(','forbidden native operation: mProcID'),
 ('extra_pre_hook',A,'const auto capture=','(void)mods::hook::add_pre<Cp04EocExecute>(nullptr,&before);const auto capture=','must add exactly two POST callbacks'),
 ('chip_window_order_swapped',A,'before.priority=90;after.priority=-250;','before.priority=-250;after.priority=90;','proof/binding missing'),
 ('restore_unit_guard_removed',A,' if(units!=cp06::kChipUnits)return false;\n','','lost its exact-unit guard'),
 ('restore_readback_removed',A,'return g_dComIfG_gameInfo.play.getItemLifeCount()==expected&&','return true&&','lost its exact-unit guard'),
 ('restore_without_layout',A,'g.p8.layout!=1||!cp06_native_chip_restore','!cp06_native_chip_restore','proof/binding missing'),
 ('fury_variant_ignored',A,'profile.variant==postbal01::Variant::BlackFury','true','proof/binding missing'),
 ('shield_pair_reverse_check_removed',A,'c.cCcD_ObjHitInf::ChkTgHit()&&c.ChkTgShieldHit()','true','proof/binding missing'),
 ('core_atp_check_removed',C,'atp==kFuryAtp&&','','attribution rule missing'),
 ('core_absorbs_any_delta',C,'if(!x.before.shield_pair||now.pending!=x.before.pending-kChipUnits)return record(Chip::DeltaMismatch);','','attribution rule missing'),
 ('core_life_change_ignored',C,'if(now.life!=x.before.life)return record(Chip::LifeChanged);','','attribution rule missing'),
 ('core_native_address',C,'namespace cp06 {','namespace cp06 {\nusing addr=std::uintptr_t;','value core contains native address'),
 ('parry_window_widened','src/parry_core.hpp','inline constexpr float kWindowFrames = 7.0f;','inline constexpr float kWindowFrames = 9.0f;','parry window/legacy fury bound changed'),
 ('parry_age_widened','src/parry_core.hpp','kWindowTicksCap = 9;','kWindowTicksCap = 12;','parry window/legacy fury bound changed'),
 ('legacy_fury_bound_relaxed','src/parry_core.hpp','kFuryRateThreshold = 1.5f;','kFuryRateThreshold = 2.5f;','parry window/legacy fury bound changed'),
 ('fury_gate_unconditional','src/parry_core.hpp','    if (in.fury_verified) return false;','    if (true) return false;','CP06 exact integration changed: parry_core.hpp'),
 ('Red_profile_changed','src/cp05_combo_core.hpp','r4[]={1.05f,1.35f,1.35f,1.05f}','r4[]={1.05f,1.50f,1.50f,1.05f}','approved Red/Black profile changed'),
 ('Black_normal_at_fury_rate','src/cp05_combo_core.hpp','kBlackMaxRate=1.30f;','kBlackMaxRate=1.49f;','0.11.10 production changed outside exact authorized edits'),
 ('AT_targets_changed','src/cp05_combo_core.hpp','b8[]={62,62,38,38,38,38,64}','b8[]={62,62,30,30,30,30,64}','approved Red/Black profile changed'),
 ('Blue_gap_changed','src/cp04_combo_core.hpp','kModerateGap=8,','kModerateGap=4,','approved Blue profile changed'),
 ('e2_rate_known_widened','src/reaction_core.hpp','r == 1.0f || r == 1.15f || r == 2.0f;','r == 1.0f || r == 1.15f || r == 2.0f || r > 0.0f;','0.11.10 production changed outside exact authorized edits: src/reaction_core.hpp'),
 ('anti_stagger_terminal_proof_changed','src/cp05_fix02_core.hpp','a.anim_frame<=.01f?','a.anim_frame<=5.f?','0.11.10 production changed outside exact authorized edits: src/cp05_fix02_core.hpp'),
 ('extra_production_unit','src/cp06_extra.hpp',None,'#pragma once\n','production file set differs'),
 ('chip_reference_tampered','tests/reference/cp06_expansion_cp19f2_chip.inc','dComIfGp_setItemLifeCount(-float(devf_cp19f2::kChipLifeUnits),type);','dComIfGp_setItemLifeCount(-2.f,type);','reviewed new module changed'),
]
rows=[]
for name,rel,a,b,want in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/rel
  if a is None:f.write_text(b,encoding='utf-8')
  else:s=f.read_text(encoding='utf-8');assert s.count(a)==1,name;f.write_text(s.replace(a,b,1),encoding='utf-8')
  r,d=audit(p);assert r.returncode==1 and any(want in e for e in d['errors']),(name,want,d['errors']);rows.append({'case':name,'rejected':True,'expected_cause':want})
for field,want in [('files','CP06 exact two-module + Expansion chip reference inventory required'),('baseline_files','CP06 frozen 0.11.10 production inventory count changed')]:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/'ci/cp06_policy.json';d=json.loads(f.read_text(encoding='utf-8'));d[field]={};f.write_text(json.dumps(d));r,d=audit(p);assert r.returncode==1 and any(want in e for e in d['errors']),(field,d['errors']);rows.append({'case':field+'_inventory_removed','rejected':True,'expected_cause':want})
print(json.dumps({'status':'NEGATIVES_PASS','positive_clone':'SOURCE_AUDIT_PASS','count':len(rows),'timeouts':0,'cases':rows},indent=2))
