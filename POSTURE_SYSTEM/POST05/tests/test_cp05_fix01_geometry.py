"""Exact native cc_set, cylinder/sphere pair and shield-front predicates.
Joint matrices, Link cylinder and broadphase I/O are synthetic fixtures.
This does NOT authenticate BCK poses, visible sparks or Windows runtime.
"""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1];ns={'__file__':str(root/'tests/test_cp05_adapter.py')}
exec((root/'tests/test_cp05_adapter.py').read_text(encoding='utf-8').split('mod=(root')[0],ns);head=ns['head']
def edit(old,new):
 global head
 assert head.count(old)==1,old
 head=head.replace(old,new,1)
edit('cXyz()=default;','cXyz()=default;static const cXyz BaseZ;void set(float a,float b,float c){x=a;y=b;z=c;}float inprod(const cXyz& v)const{return x*v.x+y*v.y+z*v.z;}')
edit('struct Model{float m[3][4]{};MtxP getAnmMtx(int){return m;}};','struct Model{float m[32][3][4]{};MtxP getAnmMtx(int n){return m[n];}};')
edit('struct HitObj{HitActor* GetAc(){return &hit_actor;}};', '''struct cCcD_Stts{};
struct HitObj{HitActor* owner=nullptr;cCcD_Stts stts;bool no_guard=false,shield=false,sp=false,small=false,front=true;short heading=0,range=0x2000,particle_y=short(0x8000);HitActor* GetAc(){return owner?owner:&hit_actor;}HitObj* GetGObjInf(){return this;}cCcD_Stts* GetStts(){return &stts;}bool ChkAtNoGuard(){return no_guard;}bool ChkTgShield(){return shield;}bool ChkTgSpShield(){return sp;}bool ChkTgSmallShield(){return small;}int GetAtSpl(){return 0;}bool ChkTgShieldFrontRange(){return front;}short* GetTgShieldFrontRangeYAngle(){return &heading;}short GetTgShieldRange(){return range;}};''')
edit('struct cCcD_ObjHitInf{','struct cCcD_ObjHitInf:HitObj{')
edit('float GetR()const{return radius;}','float GetR()const{return radius;}void SetC(const cXyz& p){centre=p;}void SetR(float x){radius=x;}')
edit('void executeAttack();void executeFind();','cXyz eyePos;struct{cXyz position;}attention_info;void cc_set();void executeAttack();void executeFind();')
head+='''
using J3DModel=Model;
const cXyz cXyz::BaseZ{0,0,1};using cCcD_GObjInf=HitObj;using dCcD_GObjInf=HitObj;struct csXyz{short x=0,y=0,z=0;};
struct dJntCol_c{int getHitmarkPosAndAngle(const cXyz*,csXyz*,cXyz*,csXyz*,int){return -1;}bool checkShieldType(int){return false;}};
dJntCol_c* fopAcM_GetJntCol(fopAc_ac_c*){return nullptr;}csXyz* fopAcM_GetShapeAngle_p(fopAc_ac_c*){static csXyz p;return &p;}
struct mDoMtx_stack_c{static inline MtxP matrix=nullptr;static void copy(MtxP m){matrix=m;}static void multVec(const cXyz* s,cXyz* d){cXyz v;v.x=matrix[0][0]*s->x+matrix[0][1]*s->y+matrix[0][2]*s->z+matrix[0][3];v.y=matrix[1][0]*s->x+matrix[1][1]*s->y+matrix[1][2]*s->z+matrix[1][3];v.z=matrix[2][0]*s->x+matrix[2][1]*s->y+matrix[2][2]*s->z+matrix[2][3];*d=v;}static void ZXYrotS(csXyz){}static void multVecSR(const cXyz* a,cXyz* b){*b=*a;}};
class dCcS{public:std::vector<cCcD_ObjHitInf*> registered;void Set(cCcD_ObjHitInf* s){registered.push_back(s);}void CalcParticleAngle(HitObj* a,cCcD_Stts*,cCcD_Stts*,csXyz* p){p->y=a->particle_y;}bool ChkShieldFrontRange(cCcD_Obj*,cCcD_Obj*,int,const cXyz*);bool ChkShield(cCcD_Obj*,cCcD_Obj*,dCcD_GObjInf*,dCcD_GObjInf*,const cXyz*);};
dCcS collision_world;dCcS* dComIfG_Ccsp(){return &collision_world;}
struct cM3dGSph{cXyz centre;float r;const cXyz* GetCP()const{return &centre;}float GetR()const{return r;}};
struct cM3dGCyl{cXyz centre;float r,h;const cXyz* GetCP()const{return &centre;}float GetR()const{return r;}float GetH()const{return h;}};
float cM3d_Len2dSq(float x,float z,float a,float b){return (x-a)*(x-a)+(z-b)*(z-b);}
'''
source=head+(root/'src/cp04_adapter.inc').read_text(encoding='utf-8')+(root/'src/cp05_adapter.inc').read_text(encoding='utf-8')+''.join((root/('tests/reference/'+n)).read_text(encoding='utf-8') for n in ['cp04_native_attack.inc','cp05_native_find.inc','cp05_native_cc_set.inc','cp05_native_cyl_sph.inc','cp05_native_shield.inc'])
source+=(root/'tests/fixtures/cp05_fix01_geometry.inc').read_text(encoding='utf-8')
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'geometry.cpp';f.write_text(source,encoding='utf-8');exe=p/('geometry.exe' if os.name=='nt' else 'geometry')
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/DAVOID_UB=1',str(f),'/Fe'+str(exe),'/Fo'+str(p/'geometry.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1','-DAVOID_UB=1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
 if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=90,env=env);print(c.stdout+c.stderr);raise SystemExit(c.returncode)
