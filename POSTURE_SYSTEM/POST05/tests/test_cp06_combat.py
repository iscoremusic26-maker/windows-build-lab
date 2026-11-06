"""CP06 0.11.11 combat laboratory: production CP04/CP05/CP06/FIX02 callbacks, unchanged
native Attack/Find/cc_set/cCcS register+clear, actual POST08 judge, shield chain, posture
engine, real Expansion Red/Black/CP10.6 policies and the transcribed Expansion CP19F2 chip.
Link motion, broadphase contacts and rendering are MODELED; no visual/runtime claim.
"""
from pathlib import Path
import os,re,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1];ns={'__file__':str(root/'tests/test_cp05_adapter.py')}
exec((root/'tests/test_cp05_adapter.py').read_text(encoding='utf-8').split('mod=(root')[0],ns);head=ns['head']
def edit(a,b):
 global head
 assert head.count(a)==1,(a,head.count(a));head=head.replace(a,b,1)
edit('#include <algorithm>','#include <algorithm>\n#include <array>\n#include "'+str(root/'src/cp05_fix02_core.hpp')+'"\n#include "'+str(root/'src/cp06_combat_core.hpp')+'"')
edit('struct HitObj{HitActor* GetAc(){return &hit_actor;}};', '''struct cCcD_Stts{void ClrAt(){}void ClrTg(){}void ClrCo(){}};
struct HitObj{bool on=false,hit=false,shield=false;bool ChkAtSet(){return on;}bool ChkTgSet(){return false;}bool ChkCoSet(){return false;}bool ChkAtShieldHit(){return shield;}HitObj* GetGObjInf(){return this;}cCcD_Stts* GetStts(){return nullptr;}void ClrAtHit(){hit=shield=false;}HitActor* GetAc(){return &hit_actor;}};''')
edit('struct cCcD_ObjHitInf{bool on=false,hit=false;','struct cCcD_ObjHitInf:HitObj{bool tg_shield=false;const void* tg_obj=nullptr;bool ChkTgShieldHit(){return tg_shield;}HitObj* GetTgHitObj(){return static_cast<HitObj*>(const_cast<void*>(tg_obj));}')
edit('float GetR()const{return radius;}','float GetR()const{return radius;}void SetC(const cXyz& c){centre=c;}void SetR(float r){radius=r;}')
edit('void OnAtSetBit(){on=true;}','void OnAtSetBit(){on=true;}void OffAtSetBit(){on=false;}')
edit('cXyz()=default;','cXyz()=default;void set(float a,float b,float c){x=a;y=b;z=c;}')
edit('void executeAttack();void executeFind();','cXyz eyePos;struct{cXyz position;}attention_info;void cc_set();void executeAttack();void executeFind();')
edit('bool session_disabled(){return off;}','bool session_disabled(){return off;}void close_session(){off=true;}')
edit('struct{post04::PostureEngine pe;','struct{bool reentered=false;struct{std::uintptr_t actor=0;std::uint32_t id=0;std::uint64_t generation=0;int slot=-1;bool tracked=false;post08::Input pin;post08::Verdict pv;postbal01::Snapshot balance_profile;}cur;post04::PostureEngine pe;')
edit('struct{std::uint64_t tick=1;}p8;','struct{std::uint64_t tick=1;int layout=1;}p8;')
edit('HitObj mTgCyls[3];void* mShieldModel=this;','cCcD_ObjHitInf mTgCyls[3];void* mShieldModel=this;')
edit('struct{struct{bool isPauseFlag(){return pause;}','struct{struct{float item_life=0;unsigned char item_type=0;float getItemLifeCount(){return item_life;}unsigned char getItemLifeCountType(){return item_type;}void setItemLifeCount(float h,unsigned char t){item_life+=h;item_type=t;}bool isPauseFlag(){return pause;}')
edit('enum{LOG_LEVEL_INFO,MOD_OK,MOD_ERROR};','enum{LOG_LEVEL_INFO,MOD_OK,MOD_ERROR,LOG_LEVEL_WARN};')
edit('x.atp=a->atp;','x.atp=a->atp;x.anim_ok=1;x.anim_attr=0;x.anim_rate=a->morf.getPlaySpeed();x.anim_frame=a->morf.getFrame();x.anim_end=short(a->morf.mFrameCtrl.getEnd());')
head+='''
static bool abi_layout_valid=true;static unsigned layout_checks=0;bool layout_matches_binary(daE_OC_c*){++layout_checks;return abi_layout_valid;}
#define ARRAY_SIZEU(a) (sizeof(a)/sizeof((a)[0]))
#define OS_REPORT(...) ((void)0)
class cCcS{public:cCcD_Obj* mpObjAt[256]{},*mpObjTg[256]{},*mpObjCo[256]{},*mpObj[256]{};unsigned mObjAtCount=0,mObjTgCount=0,mObjCoCount=0,mObjCount=0;void Set(cCcD_Obj*);void ClrAtHitInf();void reset(){mObjAtCount=mObjTgCount=mObjCoCount=mObjCount=0;}};
cCcS collision_world;cCcS* dComIfG_Ccsp(){return &collision_world;}using J3DModel=Model;
struct mDoMtx_stack_c{static inline MtxP matrix=nullptr;static void copy(MtxP m){matrix=m;}static void multVec(const cXyz* s,cXyz* d){*d=*s;}};
struct{lnks1fix::CollisionCleanup cleanup;unsigned cc_depth=0,cc_added=0;std::uintptr_t cc_key=0;int cc_slot=-1;}fx;
struct{std::uint64_t cleanup_requested=0,contact_dropped=0;unsigned contact_lines=0;}f2;
'''
mod=(root/'src/mod.cpp').read_text(encoding='utf-8')
lease=mod[mod.index('void fix01_at_mask('):mod.index('// POST06: segundo escritor')]
new=(root/'src/cp05_fix02_adapter.inc').read_text(encoding='utf-8');contact=re.search(r'^void fix02_guard_contact\(.*?^\}\n',new,re.M|re.S)[0]
input_fn=re.search(r'^void on_stick_post_fx\(.*?^\}\n',mod,re.M|re.S)[0]
prefix=(root/'tests/fixtures/cp05_adapter_scenario.inc').read_text(encoding='utf-8').split('void native_tick(')[0].replace('ROOT',root.as_posix())
native=''.join((root/'tests/reference'/n).read_text(encoding='utf-8') for n in ['cp04_native_attack.inc','cp05_native_find.inc','cp05_native_cc_set.inc','fix02_native_cc_register.inc','fix02_native_cc_clear.inc'])
expansion='#include "'+(root/'tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19f2_policy.hpp').as_posix()+'"\n'
source=head+(root/'src/cp04_adapter.inc').read_text(encoding='utf-8')+(root/'src/cp05_adapter.inc').read_text(encoding='utf-8')+input_fn+prefix+lease+contact+(root/'src/cp06_combat_adapter.inc').read_text(encoding='utf-8')+native+expansion+(root/'tests/fixtures/cp06_combat_scenario.inc').read_text(encoding='utf-8').replace('ROOT',root.as_posix())
if os.environ.get('CP06_SOURCE_OUT'):Path(os.environ['CP06_SOURCE_OUT']).write_text(source,encoding='utf-8')
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'combat.cpp';f.write_text(source,encoding='utf-8');exe=p/('combat.exe' if os.name=='nt' else 'combat')
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/bigobj','/DAVOID_UB=1',str(f),'/Fe'+str(exe),'/Fo'+str(p/'combat.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1','-DAVOID_UB=1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=300)
 if c.returncode:print(c.stdout[-6000:]+c.stderr[-6000:]);raise SystemExit(c.returncode)
 c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=300,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));print(c.stdout+c.stderr[-4000:]);raise SystemExit(c.returncode)
