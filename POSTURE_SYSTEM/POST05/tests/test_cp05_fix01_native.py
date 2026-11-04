"""Full native damage_check + Attack/Find, actual POST03/04/05 transaction,
and complete production CP04/CP05 callbacks. SDK I/O remains modeled.
The old-source control must reproduce premature cancellation; no visual claim.
"""
from pathlib import Path
import os,subprocess,tempfile,json,sys
root=Path(__file__).resolve().parents[1]
ns={'__file__':str(root/'tests/test_cp05_adapter.py')}
exec((root/'tests/test_cp05_adapter.py').read_text(encoding='utf-8').split('mod=(root')[0],ns)
head=ns['head']
def edit(old,new):
 global head
 assert head.count(old)==1,old
 head=head.replace(old,new,1)
edit('#include <algorithm>','#include <algorithm>\n#include "'+str(root/'src/reaction_core.hpp')+'"\n#include "'+str(root/'src/fix01_core.hpp')+'"')
edit('struct HitObj{HitActor* GetAc(){return &hit_actor;}};','struct HitObj{unsigned type=0x10;HitActor* GetAc(){return &hit_actor;}bool ChkAtType(unsigned t){return (type&t)!=0;}int GetAtSpl(){return 0;}};using dCcD_GObjInf=HitObj;HitObj* modeled_tg_obj();')
edit('bool ChkTgHit()const{return tg;}','bool ChkTgHit()const{return tg;}HitObj* GetTgHitObj(){return modeled_tg_obj();}cXyz* GetTgHitPosP(){static cXyz p;return &p;}')
edit('void executeAttack();void executeFind();','int field_0x6cc=0,field_0x6d6=0,field_0x6dc=0;bool cutdown=false;struct{HitObj* mpCollider=nullptr;int mAttackPower=2;Angle mHitDirection;}mAtInfo;struct{void Move(){}}mStts;struct{int flags=15;}attention_info;bool checkCutDownHitFlg(){return cutdown;}void offCutDownHitFlg(){cutdown=false;}void offTgSph();bool head_lock=false;void offHeadLockFlg(){head_lock=false;}int getCutType(){return 0;}void damage_check();void executeAttack();void executeFind();')
edit('unsigned calls[6]{};','unsigned calls[6]{};void startCollisionSE(int,int){}')
edit('void setActionMode(int m,int s){mOldActionMode=mActionMode;mActionMode=m;mOcState=s;field_0x6c2=field_0x6c0=0;}', 'void setActionMode(int,int);')
edit('bool ChkTgHit()const{return tg;}', 'bool tg_set=true,co_set=true;void ClrTgHit(){tg=false;}void OnTgSetBit(){tg_set=true;}void OnCoSetBit(){co_set=true;}void OffTgSetBit(){tg_set=false;}void OffCoSetBit(){co_set=false;}bool ChkTgHit()const{return tg;}')
edit('struct Py {bool checkPlayerGuard(){return player_guard;}};','struct Py {bool checkPlayerGuard(){return player_guard;}int getCutType(){return 0;}bool checkCutJumpCancelTurn(){return false;}};')
head+='''
enum{AT_TYPE_UNK=0x80000000u,AT_TYPE_MASTER_SWORD=0x4000000u,AT_TYPE_NORMAL_SWORD=2,AT_TYPE_SHIELD_ATTACK=0x10,AT_TYPE_ARROW=0x2000,AT_TYPE_BOMB=0x20,AT_TYPE_IRON_BALL=0x400000,AT_TYPE_BOOMERANG=0x10000,AT_TYPE_40=0x40,AT_TYPE_SLINGSHOT=0x80,E_OC_ACTION_DEATH=9,E_OC_ACTION_DAMAGE=5,E_OC_ACTION_BIG_DAMAGE=6};
struct daPy_py_c{enum{CUT_TYPE_HEAD_JUMP=55,CUT_TYPE_JUMP=66};static bool checkWoodSwordEquip(){return false;}};
#define S16_ADD(x,y) ((x)+=(y))
#define S16_SUB(x,y) ((x)-=(y))
enum{E_OC_ACTION_WALK=1,E_OC_ACTION_TALK=2};float cM_rnd(){return .5f;}int KREG_S(int){return 0;}int NREG_S(int){return 0;}bool dComIfGp_checkPlayerStatus0(int,int){return false;}
template<class... T>void dComIfGp_setHitMark(T...){}
static HitObj incoming_obj;static unsigned incoming_type=0;static int incoming_power=0;static unsigned native_damage_calls=0,suppression_commits=0;static bool suppression_enabled=true;
HitObj* modeled_tg_obj(){incoming_obj.type=incoming_type;return &incoming_obj;}
template<class T>void cc_at_check(daE_OC_c* a,T*){a->health-=incoming_power;}
void native_damage_transaction(daE_OC_c& a);
'''
prefix=(root/'tests/fixtures/cp05_adapter_scenario.inc').read_text(encoding='utf-8').split('void all_patterns(){')[0].replace('ROOT',root.as_posix())
prefix=prefix.replace('Admission::E_OC2_CANDIDATE,0,1,ev','Admission::E_OC2_CANDIDATE,g.eng.obs.epoch.epoch,1,ev')
prefix=prefix.replace('if(priority>=0)a.setActionMode(priority,0);','if(a.field_0x6cc>0)--a.field_0x6cc;native_damage_transaction(a);if(priority>=0)a.setActionMode(priority,0);')
helpers=(root/'tests/fixtures/cp05_fix01_native_scenario.inc').read_text(encoding='utf-8')
results=[]
for old in [True,False]:
 h=head
 if old:h=h.replace(str(root/'src/cp05_combo_core.hpp'),str(root/'tests/reference/cp05_0_11_8/cp05_combo_core.hpp'))
 adapter=root/('tests/reference/cp05_0_11_8/cp05_adapter.inc' if old else 'src/cp05_adapter.inc')
 mod=(root/'src/mod.cpp').read_text(encoding='utf-8');import re
 old_input=re.search(r'^void on_stick_post_fx\(.*?^\}\n',mod,re.M|re.S)[0]
 source=h+(root/'src/cp04_adapter.inc').read_text(encoding='utf-8')+adapter.read_text(encoding='utf-8')+''.join((root/('tests/reference/'+n)).read_text(encoding='utf-8') for n in ['cp05_native_set_action.inc','cp05_native_off_tg.inc','cp04_native_attack.inc','cp05_native_find.inc','cp05_native_damage.inc'])+old_input+prefix+helpers
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);f=p/'native.cpp';f.write_text(source,encoding='utf-8');exe=p/('native.exe' if os.name=='nt' else 'native')
  flags=['/I'+str(root/'src'),'/nologo','/std:c++20','/EHsc','/utf-8','/DAVOID_UB=1','/DOLD_SOURCE='+str(int(old))]
  cmd=['cl',*flags,str(f),'/Fe'+str(exe),'/Fo'+str(p/'native.obj')] if os.name=='nt' else ['g++','-I'+str(root/'src'),'-std=c++20','-O1','-DAVOID_UB=1','-DOLD_SOURCE='+str(int(old)),'-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
  c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
  if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
  env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=90,env=env)
  print(c.stdout+c.stderr)
  if c.returncode:raise SystemExit(c.returncode)
  results.append((old,c.stdout))
# R release must not unlock a CP05 timer: the Black normal first interval is
# identical with held/released R in the otherwise identical source controls.
for old,output in results:
 rows=[dict(re.findall(r'(\w+)=([^ ]+)',s)) for s in output.splitlines() if s.startswith('CP05_FIX01_GUARD_AB ')]
 assert len(rows)==18
 for kind in ['BLACK8','BLACK7']:
  same=[r for r in rows if r['kind']==kind and r['controller']=='CP05_PLUS_EXPANSION']
  assert len(same)==2 and same[0]['interval']==same[1]['interval'],same
  if not old:assert int(same[0]['interval'])==({'BLACK8':65,'BLACK7':46}[kind]),same
