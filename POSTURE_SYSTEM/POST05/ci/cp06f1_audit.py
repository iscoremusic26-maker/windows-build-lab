"""CP06 FIX01 0.11.12 additive barrier. Every 0.11.11 production file must be reproduced
byte for byte by the exact, counted FIX01 inverse patch (then the CP06 layer and the older
chain apply as before); the two FIX01 modules and the engine-predicate reference are
hash-pinned; the adapter has exactly three native writer functions, each the only place of
its native write, and the documented hook set and priorities.
"""
from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent
def policy():return json.loads((HERE/'cp06f1_policy.json').read_text(encoding='utf-8'))
def sha(b):return hashlib.sha256(b).hexdigest()
NEW={'cp06f1_core.hpp','cp06f1_adapter.inc'}
sys.path.insert(0,str(HERE))
import liz_audit  # 0.12.0: the Lizalfos inverse patch first (chained), then this FIX01 layer
def legacy_source(name,text):
 text=liz_audit.legacy_source(name,text)
 for e in reversed(policy()['inverse_patch'].get(name,[])):
  if text.count(e['after'])==e.get('count',1):text=text.replace(e['after'],e['before'],e.get('count',1))
 return text
def body(text,signature):
 i=text.index(signature);j=text.index('{',i);depth=0;k=j
 while True:
  if text[k]=='{':depth+=1
  elif text[k]=='}':
   depth-=1
   if depth==0:return text[i:k+1]
  k+=1
def audit(project):
 p=Path(project);pin=policy();errors=[]
 if set(pin['files'])!={'src/'+n for n in NEW}|{'tests/reference/cp06f1_native_cross.inc'}:errors.append('CP06F1 exact two-module + reference inventory required')
 if len(pin['baseline_files'])!=pin['baseline_count']:errors.append('CP06F1 frozen 0.11.11 production inventory count changed')
 present={f.relative_to(p).as_posix() for f in (p/'src').rglob('*') if f.is_file()}-{'src/'+n for n in liz_audit.NEW|liz_audit.kk_audit.NEW|liz_audit.kk_audit.dkr_audit.NEW}
 want=set(pin['baseline_files'])|{'src/'+n for n in NEW}
 if present!=want:errors.append('CP06F1 production file set differs from 0.11.11 + two FIX01 modules: %s'%sorted(present^want))
 for name,edits in pin['inverse_patch'].items():
  t=liz_audit.legacy_source(name,(p/'src'/name).read_text(encoding='utf-8'))
  for e in reversed(edits):
   if t.count(e['after'])!=e.get('count',1):errors.append('CP06F1 exact integration changed: '+name)
   else:t=t.replace(e['after'],e['before'],e.get('count',1))
 for rel,w in pin['baseline_files'].items():
  f=p/rel
  if not f.is_file():errors.append('CP06F1 0.11.11 production missing: '+rel);continue
  if sha(legacy_source(f.name,f.read_text(encoding='utf-8')).encode() if f.suffix in('.cpp','.hpp','.inc','.h') else f.read_bytes())!=w:errors.append('CP06F1 0.11.11 production changed outside exact authorized edits: '+rel)
 for rel,w in pin['files'].items():
  f=p/rel
  if not f.is_file() or sha(f.read_bytes())!=w:errors.append('CP06F1 reviewed new module changed: '+rel)
 a=(p/'src/cp06f1_adapter.inc').read_text(encoding='utf-8');core=(p/'src/cp06f1_core.hpp').read_text(encoding='utf-8')
 core_code=re.sub(r'//[^\n]*','',re.sub(r'/\*.*?\*/','',core,flags=re.S))
 # --- adapter: forbidden native operations anywhere
 for bad in ['setActionMode(','setPlaySpeed(','setBck(','OnAtSetBit(','OffAtSetBit(','ClrAtHit(','ClrTgHit(','ResetTgHit(','OnTgSetBit(','OffTgSetBit(','OnTgShield','OffTgShield','restore_reaction(','write_vitality(',
             'setItemLifeCount(','setLife(','clearItemLifeCount','HOOK_SKIP_ORIGINAL','VirtualProtect','WriteProcessMemory','get_service(','procGuard','mItemTrigger','setFrame(','setFrameF(','setDamagePoint','fopAcM_SearchByID','calc()']:
  if bad in a:errors.append('CP06F1 forbidden native operation: '+bad)
 for pat in [r'mDamageTimer\s*=(?!=)',r'mProcID\s*=(?!=)',r'\bhealth\s*=(?!=)',r'mActionMode\s*=(?!=)',r'mOcState\s*=(?!=)']:
  if re.search(pat,a):errors.append('CP06F1 forbidden native assignment: '+pat)
 # --- exactly three writers; every native write inside its writer and nowhere else
 writers={'sphere':'CP06F1_NOINLINE bool cp06f1_native_sphere_place(','frame':'CP06F1_NOINLINE void cp06f1_native_pose_frame(','kcolor':'CP06F1_NOINLINE bool cp06f1_native_kcolor('}
 if len(re.findall(r'^CP06F1_NOINLINE (?:bool|void) ',a,re.M))!=3 or a.count('#define CP06F1_NOINLINE')!=2 or a.count('#undef CP06F1_NOINLINE')!=1:errors.append('CP06F1 exactly three noinline writer functions required')
 try:wb={k:body(a,s) for k,s in writers.items()}
 except ValueError:errors.append('CP06F1 writer function missing');wb={}
 if wb:
  rest=a
  for v in wb.values():rest=rest.replace(v,'')
  for tok,k in [('.SetC(','sphere'),('.SetR(','sphere'),('mFrameCtrl.mFrame=','frame'),('modelCalc()','frame'),('TevKColor;kc.r=','kcolor')]:
   if tok not in wb[k]:errors.append('CP06F1 writer lost its native write: '+tok)
   if tok in rest:errors.append('CP06F1 native write outside its writer: '+tok)
  if wb['sphere'].count('.SetC(')!=1 or wb['sphere'].count('.SetR(')!=1 or wb['frame'].count('mFrameCtrl.mFrame=')!=1 or wb['frame'].count('modelCalc()')!=1:errors.append('CP06F1 writer native write count changed')
  if 'if(j!=0&&j!=1)return false;' not in wb['sphere'] or 'return c.x==x&&c.y==y&&c.z==z&&a->mSphs_at[j].GetR()==r;' not in wb['sphere']:errors.append('CP06F1 sphere writer lost its slot guard or readback')
  if 'return kc.r==r&&kc.g==g&&kc.b==b;' not in wb['kcolor']:errors.append('CP06F1 kcolor writer lost its readback')
  for m in re.finditer(r'(?<![=!<>])=(?!=)',rest):
   left=rest[max(0,m.start()-60):m.start()]
   if re.search(r'(->|\bk\.|\bkc\.|\bm\.|\bmodel\.)[\w\[\]\.]*\s*$',left)and not re.search(r'\b(f1|S|s|d|t|c|pv|out|line|o|e)\.[\w\[\]\.]*\s*$',left):errors.append('CP06F1 assignment through a game pointer outside the writers: '+left[-40:])
  if wb['frame'].count('cp06f1_native_pose_frame(')!=1 or a.count('cp06f1_native_pose_frame(')!=1+2:errors.append('CP06F1 pose frame writer must be called exactly twice, by the probe')
  pb=body(a,'bool cp06f1_pose_probe(')
  for tok in ['same=q[r][c]==f1_snap[i][r][c];','bool same=m->getFrame()==restore;','cp06f1_native_pose_frame(m,restore);','for(unsigned i=0;i<n;++i)std::memcpy(f1_snap[i],model->getAnmMtx(int(i)),sizeof(f1_snap[i]));']:
   if tok not in pb:errors.append('CP06F1 probe lost its exact restore verification: '+tok)
 # --- production bindings: gates, scope, priorities, attribution
 for tok in ['t.link_current=f1.link_set_tick==g.p8.tick&&f1.link_set_key==reinterpret_cast<std::uintptr_t>(k);','if(!t.link_current){t.why=Why::LinkNotCurrent;return;}',
             'if(t.expect){t.why=Why::AlreadyTouching;covered=true;return;}','if(!(m->getMorfRatio()>=1.0f||!m->getOldTransInfo()||!m->getOldQuaternion()))',
             '++f1.probe_mismatch;f1.coverage=false;','sincos16(a->shape_angle.y,sn,cs);','const V3 o=lunge_offset(clip_b,t.pre,fr[i],sn,cs);',
             'return m==k->mpLinkModel||m==k->mpLinkFaceModel||m==k->mpLinkHatModel||m==k->mpLinkHandModel||m==k->mpDemoFCBlendModel||m==k->mpDemoFCTongueModel||m==k->mpDemoHLTmpModel||m==k->mpDemoHRTmpModel;',
             'if(!k||!m||k!=f1_link()||!f1_flash_model(k,m))return HOOK_CONTINUE;','else if(now==d.written){','if(r==cp06f1::Draw::Counterfactual){if(cp06f1_native_kcolor(k,out.r,out.g,out.b))',
             'cov.priority=-300;cb.priority=91;ca.priority=-251;dc.priority=90;da.priority=-250;dv.priority=-250;lc.priority=-300;',
             'const auto seen=cp06f1::observe(f1.chip.pending,g_dComIfG_gameInfo.play.getItemLifeCount(),f1.chip.comp,c6.compensated);','bool ok=c6.enabled;for(auto x:r)ok=ok&&x==MOD_OK;',
             'f1.flash.pass();']:
  if tok not in a:errors.append('CP06F1 production proof/binding missing: '+tok)
 regs=re.findall(r'mods::hook::add_(pre|post)<(\w+)>\((\w+),&(\w+)\)',a)
 if sorted(regs)!=sorted([('post','LinkModelDrawF1','on_model_draw_post_cp06f1','dv'),('pre','LinkModelDrawF1','on_model_draw_pre_cp06f1_capture','dc'),('pre','LinkModelDrawF1','on_model_draw_pre_cp06f1_apply','da'),
   ('post','Cp04EocExecute','on_eoc_post_cp06f1_chip_before','cb'),('post','Cp04EocExecute','on_eoc_post_cp06f1_chip_after','ca'),('post','LinkCollisionFx','on_collision_post_cp06f1','lc'),('post','Cp04EocExecute','on_eoc_post_cp06f1_coverage','cov')]) or a.count('mods::hook::add_')!=7:
  errors.append('CP06F1 hook registrations changed: %s'%regs)
 for m in re.finditer(r'return\s+(HOOK_\w+)\s*;',a):
  if m.group(1)!='HOOK_CONTINUE':errors.append('CP06F1 hook returns '+m.group(1))
 # --- pure core
 for bad in ['uintptr_t','void*','daE_OC_c','daAlink_c','cM_rnd','g_dComIfG','#include <cmath>','sqrt(','sinf(','cosf(']:
  if bad in core_code:errors.append('CP06F1 value core contains native address/RNG/global/CRT math: '+bad)
 for tok in ['const float rs=k.r+s.r-slack;if(!(rs>=0.0f))return false;','return s.c.y+rv>=k.c.y&&s.c.y-rv<=k.c.y+k.h;','kWriteSlack=0.5f,kKeepSlack=-0.5f;',
             'if(!(pre>=kAtFirst&&pre<=kAtLast))return 0;','if(n<kMaxProbes&&post>kAtLast&&kPoseLast-post>0.5f&&(!n||out[n-1]<kPoseLast))out[n++]=kPoseLast;',
             "if(touches(k,cur[0],kWriteSlack,true)||touches(k,cur[1],kWriteSlack,true)){d.why=Why::AlreadyTouching;return d;}",
             'const float d=lunge(clip_b,m-1.0f)-lunge(clip_b,pre);return V3{d*sn,0.0f,d*cs};',
             'inline constexpr float kLungeB[10]={0.0f,0.0f,0.0f,0.0f,15.3f,38.4f,63.3f,86.2f,103.3f,112.0f};','inline constexpr float kLungeC[10]={0.0f,31.3f,72.7f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f};',
             'inline void start(Flash& f)noexcept{f.dmg=kFlashTimer;f.col=32-(kFlashTimer%16);}','if(now!=expected){++c.mismatch;uncertain_=true;return Draw::Mismatch;}',
             'if(s==ChipSeen::ParryNeutralized){start(e_);++c.parry_starts;}','else if(s==ChipSeen::Legit){start(e_);start(l_);++c.legit_starts;}',
             'if(comp_after==comp_before+1&&after==before)return ChipSeen::ParryNeutralized;','if(comp_after==comp_before&&after==before-1.0f)return ChipSeen::Legit;']:
  if tok not in core:errors.append('CP06F1 core rule missing: '+tok)
 # --- CP06 rhythms, parry window and chip attribution untouched (byte-identical files already pinned by the 0.11.11 inventory)
 return errors
if __name__=='__main__':
 e=audit(Path(sys.argv[1]));print(json.dumps({'status':'SOURCE_AUDIT_FAIL' if e else 'SOURCE_AUDIT_PASS','base_source':policy()['base_source'],'frozen_0_11_11_files':policy()['baseline_count'],'new_native_writers':['cp06f1_native_sphere_place','cp06f1_native_pose_frame','cp06f1_native_kcolor'],'errors':e},indent=2));sys.exit(bool(e))
