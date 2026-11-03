#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <type_traits>
#include "HUDFIX_HEADER"
using Mtx=float[3][4];using GfxStageHookHandle=unsigned long long;using u32=unsigned;using u16=unsigned short;using u8=unsigned char;using f32=float;
struct ModContext{};enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,int n)noexcept{return *static_cast<T*>(static_cast<void**>(a)[n]);}template<class T>T& arg_ref(void* a,int n)noexcept{return *static_cast<T*>(static_cast<void**>(a)[n]);}}
struct Point{float x=0,y=0;};struct Box{Point i,f;};struct J2DPane;struct Child{J2DPane* p;};
template<class T>struct JSUTreeIterator{Child* p;JSUTreeIterator(Child* q):p(q){}bool operator!=(Child* q)const{return p!=q;}void operator++(){++p;}T* getObject(){return p->p;}};
struct Tree{Tree* parent=nullptr;J2DPane* object=nullptr;std::vector<Child> children;Tree* getParent(){return parent;}J2DPane* getObject(){return object;}Child* getFirstChild(){return children.data();}Child* getEndChild(){return children.empty()?children.data():children.data()+children.size();}};
struct J2DPane{
 Box mBounds{{0,0},{608,448}};Mtx mPositionMtx{},mGlobalMtx{};float mScaleX=1,mScaleY=1,mTranslateX=0,mTranslateY=0;std::uint64_t mInfoTag=0,mUserInfoTag=0;bool mVisible=true,mIsInfluencedAlpha=true;unsigned mAlpha=255;u16 type=16;Tree mPaneTree;int draw_id=-1;
 J2DPane(){mPaneTree.object=this;mPositionMtx[0][0]=mPositionMtx[1][1]=mPositionMtx[2][2]=1;}
 virtual ~J2DPane()=default;virtual u16 getTypeID()const{return type;}
 virtual void makeMatrix(float x,float y){for(auto& r:mPositionMtx)for(auto& v:r)v=0;mPositionMtx[0][0]=mScaleX;mPositionMtx[1][1]=mScaleY;mPositionMtx[2][2]=1;mPositionMtx[0][3]=x;mPositionMtx[1][3]=y;}
 void place(float x,float y,float w,float h,u16 kind=16,int id=-1){mTranslateX=x;mTranslateY=y;mBounds={{0,0},{w,h}};type=kind;draw_id=id;makeMatrix(x,y);}
};
struct J2DScreen:J2DPane{bool mScissor=false;u16 getTypeID()const override{return 8;}};
using J2DPicture=J2DPane;using J2DTextBox=J2DPane;
struct J2DGrafContext{virtual int getGrafType()const{return 1;}};struct J2DOrthoGraph:J2DGrafContext{Box ortho{{0,0},{608,448}};const Box* getOrtho()const{return &ortho;}};
namespace JUtility{struct TColor{unsigned char r=12,g=34,b=56,a=255;};}
struct JUTFont{struct TWidth{int field_0x0=0,field_0x1=8;};JUtility::TColor mColor1,mColor2,mColor3,mColor4;int getCellWidth(){return 8;}int getHeight(){return 12;}int getAscent(){return 9;}int getDescent(){return 3;}bool isFixed(){return false;}int getFixedWidth(){return 8;}bool isLeadByte(int){return false;}void getWidthEntry(int,TWidth* w){*w={0,8};}};
struct CPaneMgr{J2DPane* pane=nullptr;J2DPane* getPanePtr(){return pane;}};
struct dMeterButton_c{
 J2DScreen* mpButtonScreen=nullptr;u8 field_0x4be[2]{1,21};float field_0x2e8[2]{1,1};CPaneMgr* mpText[2]{};
 CPaneMgr *mpButtonA=nullptr,*mpButtonB=nullptr,*mpButtonR=nullptr,*mpButtonZ=nullptr,*mpButton3D=nullptr,*mpButtonC=nullptr,*mpButtonS=nullptr,*mpButtonX=nullptr,*mpButtonY=nullptr,*mpButtonNun=nullptr,*mpButtonRemo=nullptr,*mpButtonRemo2=nullptr,*mpButtonAR=nullptr,*mpButton3DB=nullptr,*mpButtonNURE=nullptr,*mpButtonReel=nullptr,*mpButtonReel2=nullptr,*mpButtonAB=nullptr,*mpButtonTate=nullptr,*mpButtonNunZ=nullptr,*mpButtonNunC=nullptr,*mpButtonBin=nullptr;
};
struct dMeter2Draw_c{J2DScreen *mpScreen=nullptr,*mpPikariScreen=nullptr;CPaneMgr* mpPikariParent=nullptr;};struct dMeter2_c{dMeter2Draw_c* mpMeterDraw=nullptr;dMeterButton_c* mpEmpButton=nullptr;};static dMeter2_c* meter=nullptr;dMeter2_c* dMeter2Info_getMeterClass(){return meter;}
static_assert(std::is_same_v<decltype(&J2DScreen::getTypeID),u16(J2DScreen::*)()const>);
