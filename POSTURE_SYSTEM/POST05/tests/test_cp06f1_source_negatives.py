"""CP06 FIX01: complete positive clone, then targeted source/writer/policy negatives against the full source wall."""
from pathlib import Path
import shutil,tempfile,subprocess,sys,json
root=Path(__file__).resolve().parents[1]
def clone(p):
 for x in ['src','tests','ci']:shutil.copytree(root/x,p/x,ignore=shutil.ignore_patterns('__pycache__'))
def audit(p):
 r=subprocess.run([sys.executable,'-I',str(p/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=120);return r,json.loads(r.stdout)
with tempfile.TemporaryDirectory() as td:
 p=Path(td);clone(p);r,d=audit(p);assert r.returncode==0 and d['status']=='SOURCE_AUDIT_PASS',(r.stdout,r.stderr)
A='src/cp06f1_adapter.inc';C='src/cp06f1_core.hpp';M='src/mod.cpp'
cases=[
 ('sphere_write_outside_writer',A,' if(!k){t.why=Why::LinkMissing;return;}\n',' if(!k){t.why=Why::LinkMissing;a->mSphs_at[0].SetR(1.0f);return;}\n','native write outside its writer: .SetR('),
 ('frame_write_outside_writer',A,' if(!k){t.why=Why::LinkMissing;return;}\n',' if(!k){t.why=Why::LinkMissing;a->mpMorf->mFrameCtrl.mFrame=0;return;}\n','native write outside its writer: mFrameCtrl.mFrame='),
 ('fourth_writer',A,'// ---------------------------------------------------------------- A. coverage\n','CP06F1_NOINLINE void cp06f1_native_extra(daE_OC_c* a)noexcept{(void)a;}\n// ---------------------------------------------------------------- A. coverage\n','exactly three noinline writer functions'),
 ('rate_written',A,' if(!k){t.why=Why::LinkMissing;return;}\n',' if(!k){t.why=Why::LinkMissing;a->mpMorf->setPlaySpeed(1.0f);return;}\n','forbidden native operation: setPlaySpeed('),
 ('link_proc_written',A,' if(!k){t.why=Why::LinkMissing;return;}\n',' if(!k){t.why=Why::LinkMissing;return;}\n if(false)k->mProcID=3;\n','forbidden native assignment'),
 ('at_bit_forced',A,' if(!k){t.why=Why::LinkMissing;return;}\n',' if(!k){t.why=Why::LinkMissing;a->mSphs_at[0].OnAtSetBit();return;}\n','forbidden native operation: OnAtSetBit('),
 ('link_registration_gate_removed',A,' if(!t.link_current){t.why=Why::LinkNotCurrent;return;}\n','','production proof/binding missing: if(!t.link_current)'),
 ('morph_gate_removed',A,'if(!(m->getMorfRatio()>=1.0f||!m->getOldTransInfo()||!m->getOldQuaternion()))','if(false)','production proof/binding missing: if(!(m->getMorfRatio()'),
 ('coverage_before_expansion',A,'cov.priority=-300;','cov.priority=10;','production proof/binding missing: cov.priority=-300;'),
 ('extra_hook_callback',A,'mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06f1_coverage,&cov)};','mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06f1_coverage,&cov),mods::hook::add_pre<Cp04EocExecute>(on_eoc_post_cp06f1_skip,&cov)};','hook registrations changed'),
 ('skip_original',A,'if(!k||!m||k!=f1_link()||!f1_flash_model(k,m))return HOOK_CONTINUE;','if(!k||!m||k!=f1_link()||!f1_flash_model(k,m))return HOOK_SKIP_ORIGINAL;','forbidden native operation: HOOK_SKIP_ORIGINAL'),
 ('probe_not_verified',A,'same=q[r][c]==f1_snap[i][r][c];','same=true;','probe lost its exact restore verification'),
 ('flash_scope_widened',A,'||m==k->mpLinkHandModel||','||true||','production proof/binding missing: return m==k->mpLinkModel'),
 ('restore_foreign_value',A,'else if(now==d.written){','else if(true){','production proof/binding missing: else if(now==d.written){'),
 ('write_margin_zero',C,'kWriteSlack=0.5f,kKeepSlack=-0.5f;','kWriteSlack=0.0f,kKeepSlack=-0.5f;','core rule missing: kWriteSlack=0.5f'),
 ('core_crt_math',C,'#include <cstdint>\n','#include <cstdint>\n#include <cmath>\n','CRT math: #include <cmath>'),
 ('core_reads_globals',C,'inline constexpr unsigned kSlots=64;','inline constexpr unsigned kSlots=64;inline void* g_dComIfG_probe=nullptr;','native address/RNG/global/CRT math'),
 ('mirror_overwrites_foreign',C,'if(now!=expected){++c.mismatch;uncertain_=true;return Draw::Mismatch;}','','core rule missing: if(now!=expected)'),
 ('maintenance_not_called',M,'  cp06_maintenance();\n  cp06f1_maintenance();\n  liz_maintenance();\n  pfx_maintenance();\n}\n','  cp06_maintenance();\n  liz_maintenance();\n  pfx_maintenance();\n}\n','exact integration changed: mod.cpp'),
 ('install_dropped',M,'  if(!cp06f1_install())return mods::set_error(err,MOD_ERROR,"CP06 FIX01 contact coverage/flash hooks failed; no partial checkpoint");\n','','exact integration changed: mod.cpp'),
 ('writer_offset_assert_dropped',M,' && offsetof(daE_OC_c,mpMorf)==0x6d8,',',','exact integration changed: mod.cpp'),
 ('version_changed','src/lifecycle_core.hpp','kModVersion = "0.12.4"','kModVersion = "0.12.5"','production changed outside exact authorized edits'),
 ('reference_tampered','tests/reference/cp06f1_native_cross.inc','    if (radius_sum < dist) {','    if (radius_sum + 10.0f < dist) {','reviewed new module changed: tests/reference/cp06f1_native_cross.inc'),
 ('extra_production_unit','src/cp06f1_extra.hpp',None,'#pragma once\n','production file set differs'),
]
rows=[]
for name,rel,a,b,want in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/rel
  if a is None:f.write_text(b,encoding='utf-8')
  else:s=f.read_text(encoding='utf-8');assert s.count(a)==1,(name,s.count(a));f.write_text(s.replace(a,b,1),encoding='utf-8')
  r,d=audit(p);assert r.returncode==1 and any(want in e for e in d['errors']),(name,want,d['errors']);rows.append({'case':name,'rejected':True,'expected_cause':want})
for field,want in [('files','CP06F1 exact two-module + reference inventory required'),('baseline_files','CP06F1 frozen 0.11.11 production inventory count changed')]:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);clone(p);f=p/'ci/cp06f1_policy.json';d=json.loads(f.read_text(encoding='utf-8'));d[field]={};f.write_text(json.dumps(d));r,d=audit(p);assert r.returncode==1 and any(want in e for e in d['errors']),(field,d['errors']);rows.append({'case':field+'_inventory_removed','rejected':True,'expected_cause':want})
print(json.dumps({'status':'NEGATIVES_PASS','positive_clone':'SOURCE_AUDIT_PASS','count':len(rows),'timeouts':0,'cases':rows},indent=2))
