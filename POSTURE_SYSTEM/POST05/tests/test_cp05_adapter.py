"""Full CP04+CP05 production callbacks + unchanged native Attack AND Find.
Exact Expansion Red/Black value policies, modeled SDK I/O; no native game/render.
"""
from pathlib import Path
import re,subprocess,os,tempfile
root=Path(__file__).resolve().parents[1]
head=(root/'tests/fixtures/cp04_adapter_head.inc').read_text().replace('ROOT',root.as_posix())
def edit(old,new):
 global head
 assert head.count(old)==1,old
 head=head.replace(old,new,1)
edit('#include <algorithm>','#include <algorithm>\n#include "'+str(root/'src/cp05_combo_core.hpp')+'"')
edit('CP04_ADAPTER_FAIL','CP05_ADAPTER_FAIL')
edit('struct HitObj{HitActor* GetAc(){return &hit_actor;}};', 'struct HitObj{HitActor* GetAc(){return &hit_actor;}};using cCcD_Obj=HitObj;HitObj* modeled_hit_obj();')
edit('HitObj* GetAtHitObj(){static HitObj h;return &h;}', 'HitObj* GetAtHitObj(){return modeled_hit_obj();}')
edit('void* mShieldModel=this;', 'HitObj mTgCyls[3];void* mShieldModel=this;')

edit('void lnk_write(int,const char* text){','void lnk_write(int,const char* text){if(std::strncmp(text,"CP05_SUMMARY ",13)==0)++cp05_summary_lines;if(std::getenv("CP05_TRACE")&&std::strncmp(text,"CP05_",5)==0)std::fprintf(stderr,"%s\\n",text);')
edit('registration_priorities[6]','registration_priorities[16]')
edit('static unsigned summary_lines=0;', 'static unsigned cp05_summary_lines=0;static unsigned summary_lines=0;')
edit('struct fopAc_ac_c{unsigned id=1;int profname=fpcNm_E_OC_e;State state;};','struct fopAc_ac_c{unsigned id=1;int profname=fpcNm_E_OC_e;State state;struct{struct{float x=0,y=0,z=0;}pos;}current;};')
edit('struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}};','struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){};template<class T>cXyz& operator=(const T& v){x=v.x;y=v.y;z=v.z;return *this;}};')
edit('bool ChkAtSet()const{return on;}','bool tg=false;bool ChkTgHit()const{return tg;}bool ChkAtSet()const{return on;}')
edit('bool water=false,blue=true;','bool water=false,blue=true;int atp=1,mHide=0;bool watch=false,perceived=true;int field_0x6b4=0,field_0x6de=0,field_0x6e2=0,field_0x6ce=0,field_0x6da=0,field_0x6c4=0;cXyz mWatchPos;struct{bool ChkWallHit(){return false;}}mAcch;')
edit('void executeAttack();','void executeAttack();void executeFind();bool setWatchMode(){return watch;}bool checkBeforeBgFind(){return false;}bool searchPlayer2(){return perceived;}bool searchPlayerShakeHead(){return false;}')
edit('cCcD_ObjHitInf mSphs_at[2];','cCcD_ObjHitInf mSphs_at[2],mSphs_cc[2];')
edit('x.atp=a->blue?1:2;','x.atp=a->atp;')
head+='''
HitObj* modeled_hit_obj(){static HitObj other;return hit_actor.profname==fpcNm_ALINK_e?&player.mTgCyls[0]:&other;}
daAlink_c* dComIfGp_getLinkPlayer(){return &player;}
enum{E_OC_ACTION_ATTACK=4,E_OC_ACTION_FIND_STAY=15,E_OC_ACTION_WAIT=0,fopAcStts_UNK_0x4000_e=1,fpcNm_NPC_KS_e=30};
enum{Z2SE_EN_OC_V_FIND=5,Z2SE_EN_OC_V_WALK_ST=5,Z2SE_EN_OC_V_DASH=5,Z2SE_EN_OC_V_SEARCH=5};
static struct{int demo_waiting_time=20;}l_HIO;
bool dComIfGp_event_runCheck(){return event;}int dComIfGp_getStartStageRoomNo(){return 0;}
void fopAcM_OffStatus(daE_OC_c*,int){}void fopAcM_SearchByName(int,fopAc_ac_c** a){*a=nullptr;}
int cLib_distanceAngleS(short a,short b){return std::abs(int(short(a-b)));}
void cLib_addCalcAngleS(short* a,short b,int,int max,int){int d=int(short(b-*a));*a=short(*a+std::clamp(d,-max,max));}
int cLib_targetAngleY(cXyz*,cXyz*){return 0;}float nREG_F(int){return 0;}
'''
mod=(root/'src/mod.cpp').read_text();old_input=re.search(r'^void on_stick_post_fx\(.*?^\}\n',mod,re.M|re.S)[0]
source=head+(root/'src/cp04_adapter.inc').read_text()+'\n'+(root/'src/cp05_adapter.inc').read_text()+'\n'+(root/'tests/reference/cp04_native_attack.inc').read_text()+'\n'+(root/'tests/reference/cp05_native_find.inc').read_text()+old_input+(root/'tests/fixtures/cp05_adapter_scenario.inc').read_text().replace('ROOT',root.as_posix())
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'adapter.cpp';f.write_text(source,encoding='utf-8');exe=p/('adapter.exe' if os.name=='nt' else 'adapter')
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/DAVOID_UB=1',str(f),'/Fe'+str(exe),'/Fo'+str(p/'adapter.obj')] if os.name=='nt' else ['g++','-DAVOID_UB=1','-std=c++20','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
 if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
 env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0';c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=90,env=env);print(c.stdout+c.stderr);raise SystemExit(c.returncode)
