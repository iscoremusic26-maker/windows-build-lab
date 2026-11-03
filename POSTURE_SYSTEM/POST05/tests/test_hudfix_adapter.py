"""Execute the ACTUAL production render adapter against a fake native J2D tree.
This tests leases/callback pairing, not the game ABI or actual raster output.
The Windows PE audit separately checks the real SDK ABI. Never loads a DLL.
"""
from pathlib import Path
import subprocess,tempfile,os,json
root=Path(__file__).resolve().parents[1]
mod=(root/'src/mod.cpp').read_text(encoding='utf-8')
adapter=mod[mod.index('struct HudFixGlue {'):mod.index('// LECTURA (por valor) del prompt de accion inferior')]
head=r'''
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include "HUDFIX_HEADER"
using Mtx=float[3][4];using GfxStageHookHandle=unsigned long long;
struct ModContext{};enum HookAction {HOOK_CONTINUE};
namespace mods {
template<class T>T arg(void* a,int n) noexcept{return *static_cast<T*>(static_cast<void**>(a)[n]);}
template<class T>T& arg_ref(void* a,int n) noexcept{return *static_cast<T*>(static_cast<void**>(a)[n]);}
}
struct Point{float x=0,y=0;};struct Box{Point i,f;};
struct J2DPane;
struct Child {J2DPane* p;};
template<class T>struct JSUTreeIterator{Child* p;JSUTreeIterator(Child* q):p(q){}bool operator!=(Child* q)const{return p!=q;}void operator++(){++p;}T* getObject(){return p->p;}};
struct Tree {std::vector<Child> children;Child* getFirstChild(){return children.data();}Child* getEndChild(){return children.empty()?children.data():children.data()+children.size();}};
struct J2DPane {
  Box mBounds{{0,0},{608,448}};Mtx mPositionMtx{},global{};
  float mScaleX=1,mScaleY=1,mTranslateX=0,mTranslateY=0;unsigned long long mUserInfoTag=0;
  bool mVisible=true,mIsInfluencedAlpha=true;unsigned mAlpha=255,type=16;Tree mPaneTree;
  J2DPane(){mPositionMtx[0][0]=mPositionMtx[1][1]=mPositionMtx[2][2]=1;}
  virtual unsigned getTypeID(){return type;}
  virtual void makeMatrix(float x,float y){for(auto& row:mPositionMtx)for(float& v:row)v=0;mPositionMtx[0][0]=mScaleX;mPositionMtx[1][1]=mScaleY;mPositionMtx[2][2]=1;mPositionMtx[0][3]=x;mPositionMtx[1][3]=y;}
  void place(float x,float y,float w,float h,unsigned kind=16){mTranslateX=x;mTranslateY=y;mBounds={{0,0},{w,h}};type=kind;makeMatrix(x,y);}
};
struct J2DScreen:J2DPane {bool mScissor=false;};
struct J2DGrafContext {virtual int getGrafType()const{return 1;}};
struct J2DOrthoGraph:J2DGrafContext {Box ortho{{0,0},{608,448}};const Box* getOrtho()const{return &ortho;}};
'''.replace('HUDFIX_HEADER',(root/'src/hudfix_core.hpp').as_posix())
tail=r'''
unsigned checks=0;
void ck(bool b,const char* s){++checks;if(!b){std::fprintf(stderr,"ADAPTER FAIL %s\n",s);std::exit(1);}}
// Separate implementation of the native J2D traversal. It DOES NOT call the
// planner or prediction functions under test when generating actual draw bounds.
void draw(J2DPane& p,const Mtx* parent,float x,float y,float width){
  if(!parent){const float scale=p.mScaleX,tx=p.mTranslateX;p.mScaleX*=width/608.0f;p.makeMatrix((tx-304)*width/608+304+x,p.mTranslateY+y);p.mScaleX=scale;}
  for(int r=0;r<3;++r)for(int c=0;c<4;++c){float v=parent?(c==3?(*parent)[r][3]:0):p.mPositionMtx[r][c];if(parent)for(int k=0;k<3;++k)v+=(*parent)[r][k]*p.mPositionMtx[k][c];p.global[r][c]=v;}
  for(auto child:p.mPaneTree.children)draw(*child.p,&p.global,0,0,width);
}
hudfix01::Rect actual(J2DPane& p){float x0=1e20f,y0=1e20f,x1=-1e20f,y1=-1e20f;for(int i=0;i<4;++i){float x=(i&1)?p.mBounds.f.x:p.mBounds.i.x,y=(i&2)?p.mBounds.f.y:p.mBounds.i.y;float a=p.global[0][0]*x+p.global[0][1]*y+p.global[0][3],b=p.global[1][0]*x+p.global[1][1]*y+p.global[1][3];if(a<x0)x0=a;if(a>x1)x1=a;if(b<y0)y0=b;if(b>y1)y1=b;}return{x0,y0,x1,y1};}
int main(){
  hf.enabled=true;
  for(int w=0;w<2;++w)for(int frame=0;frame<1000;++frame){
    const float width=w?796.444f:608.0f;J2DScreen screen;screen.type=17;
    J2DPane life,group,text,icon;life.place(10,10,180,30,18);group.place(200,375,240,45);text.place(0,0,180,20,19);icon.place(180,0,30,30,18);
    screen.mPaneTree.children={{&life},{&group}};group.mPaneTree.children={{&text},{&icon}};
    J2DOrthoGraph graph;graph.ortho.f.x=width;const J2DGrafContext* port=&graph;J2DScreen* self=&screen;float x=0,y=0;void* args[]={&self,&x,&y,&port};
    lnk01::Visual v;v.alpha=(frame%3)?1:0;const auto l=lnk01::layout(0,0,width,448);hf.present.begin(true,true,v,l,{0,0,width,448});
    ck(on_screen_pre_hudfix(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE,"never skips native draw");
    draw(screen,nullptr,x,y,width);
    if(v.alpha>=0.01f){ck(!hudfix01::intersects(actual(text),hf.present.band),"actual text clear");ck(!hudfix01::intersects(actual(icon),hf.present.band),"actual icon clear");}
    ck(life.global[1][3]==10,"native life stays in place");
    on_screen_post_hudfix(nullptr,args,nullptr,nullptr);
    ck(group.mTranslateX==200&&group.mTranslateY==375&&group.mPositionMtx[0][3]==200&&group.mPositionMtx[1][3]==375,"authored fields restored bit exact");
    ck(hf.saved_n==0&&hf.depth==0,"no lease escapes draw");
    for(unsigned i=0;i<hf.scene.n;++i)ck(hf.panes[i]==nullptr,"no pointer escapes draw");
    // Root prediction itself is an immediate lease, including the native matrix
    // left by the previous widescreen draw. It must not accumulate scale.
    const auto before=hudfix_matrix(screen.mPositionMtx);const float scale=screen.mScaleX,tx=screen.mTranslateX;
    (void)hudfix_root(&screen,0,0,width);
    ck(screen.mScaleX==scale&&screen.mTranslateX==tx,"root scalar restore");
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)ck(screen.mPositionMtx[r][c]==before.v[r][c],"root matrix restore");
    hf.present.end();
  }
  // Capacity refusal must happen before ANY live pane layout write.
  J2DScreen screen;J2DOrthoGraph graph;const J2DGrafContext* port=&graph;J2DScreen* self=&screen;float x=0,y=0;void* args[]={&self,&x,&y,&port};
  std::vector<J2DPane> many(600);for(auto& p:many){p.place(250,380,20,20,18);screen.mPaneTree.children.push_back({&p});}
  lnk01::Visual v;v.alpha=1;hf.present.begin(true,true,v,lnk01::layout(0,0,608,448),{0,0,608,448});
  const auto overflow=hf.overflow;on_screen_pre_hudfix(nullptr,args,nullptr,nullptr);ck(hf.overflow==overflow+1&&hf.saved_n==0,"capacity refusal atomic");on_screen_post_hudfix(nullptr,args,nullptr,nullptr);
  for(auto& p:many)ck(p.mTranslateY==380,"overflow untouched");
  screen.mScissor=true;const auto refused=hf.unsupported;on_screen_pre_hudfix(nullptr,args,nullptr,nullptr);on_screen_post_hudfix(nullptr,args,nullptr,nullptr);ck(hf.unsupported==refused+1,"unknown clipping context refused");
  ck(hf.unpaired==0,"all pairs balanced");
  std::printf("HUD_FIX01_ADAPTER checks=%u failures=0 RESULT=LAB_PASS actual_production_callbacks=1 native_game_executed=0\n",checks);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp=Path(tmp);cpp=tmp/'adapter.cpp';exe=tmp/('adapter.exe' if os.name=='nt' else 'adapter');cpp.write_text(head+'\n'+adapter+'\n'+tail,encoding='utf-8')
    cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(cpp),'/Fe'+str(exe),'/Fo'+str(tmp/'adapter.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(cpp),'-o',str(exe)]
    result=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
    if result.returncode:print(result.stdout+result.stderr);raise SystemExit(result.returncode)
    env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0'
    result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60,env=env);print(result.stdout+result.stderr);raise SystemExit(result.returncode)
