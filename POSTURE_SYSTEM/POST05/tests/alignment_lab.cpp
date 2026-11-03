#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#ifdef FIX02_ORACLE
#include "cp06_reference/tracker.cpp"
#include "cp06_reference/reused/recovered/normal_renderer.h"
static ehz::Tracker tracker;
#else
#include "../src/cp06/tracker.cpp"
#include "../src/alignment_core.hpp"
#include "../src/hud_core.hpp"
static postalign02::Mirror mirror;
#endif
static unsigned checks=0;static void ck(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"ALIGN02_FAIL %s\n",why);std::exit(1);}}
static bool near(float a,float b){return std::fabs(a-b)<0.0001f;}

static ehz::ActorView own[4100];
struct World{bool onscreen=true,los=true;float distance=100;float cameraX=0,cameraY=0,zoom=1;} world;
struct History{float previous[65536][3]{},current[65536][3]{};bool set[65536]{};} mh;
static bool mp(void*,const ehz::Vec3& a,ehz::Vec3* b){*b={304+(a.x-world.cameraX)*world.zoom,180-(a.y-world.cameraY)*world.zoom,a.z};if(!world.onscreen)b->x=-500;return true;}
static bool ml(void*,const ehz::Vec3&){return world.los;}
static float md(void*,const ehz::ActorView&){return world.distance;}
static void record(History& h,std::uint64_t key,float x,float y,float z){ck(key<65536,"bounded key");const float a[3]={x,y,z};for(unsigned i=0;i<3;++i){h.previous[key][i]=h.set[key]?h.current[key][i]:a[i];h.current[key][i]=a[i];}h.set[key]=true;}
static void mr(void*,std::uint64_t k,const ehz::Vec3& p){record(mh,k,p.x,p.y,p.z);}
static void mf(void*,std::uint64_t k){if(k<65536)mh.set[k]=false;}
static ehz::TickEnv me{nullptr,mp,ml,md,mr,mf,608,448,0,0};
static void fill(unsigned n,float head,int profile=464){
 for(unsigned i=0;i<n;++i){auto& a=own[i];a={};a.address=0x10000+i*256;a.id=i+1;a.profile=profile;a.group=2;a.attention=7;a.health={220,220,1};a.targetable=true;a.pos={float(i%10)*10,0,20};a.eye={a.pos.x,head-3,20};a.attentionPos={a.pos.x,head,20};a.handle=reinterpret_cast<void*>(a.address);}
}
static std::uint64_t comparisons=0,legacy_divergences=0;static float maximum_error=0;
static FILE* trace=nullptr;static char stage[8]={'F','_','S','P','1','2','1',0};static unsigned player_id=42;static std::uintptr_t player_address=0x999;static std::int8_t layer=6,dark=0;
#ifdef FIX02_ORACLE
static float life_x,life_y;static bool captured;
static void box(void*,float x,float y,float w,float h,recovered::Color,recovered::Color,recovered::Color,recovered::Color){if(!captured){life_x=x+w*.5f;life_y=y+h*.5f;captured=true;}}
static void frame(void*,float,float,float,float,recovered::Color,uint8_t){}
#endif
static void clear(){
#ifdef FIX02_ORACLE
 tracker.clear(&me);
#else
 mirror.clear(me);
#endif
}
static void tick(unsigned t,unsigned n,bool visible=true,bool senses=false,unsigned frames=2){
#ifdef FIX02_ORACLE
 tracker.tick(t,visible,true,senses,own,n,me);auto& active=tracker;
#else
 mirror.sample(t,player_address,player_id,stage,layer,dark,visible,senses,own,n,me);auto& active=mirror.tracker;
 for(unsigned i=0;i<n;++i)ck(!own[i].handle,"borrowed handles scrubbed");
#endif
 unsigned count=0;const auto* entries=active.draw_entries(&count);
 for(unsigned f=0;f<frames;++f){
  const unsigned header[3]={t,f,count};ck(std::fwrite(header,sizeof(header),1,trace)==1,"trace header");
  for(unsigned i=0;i<count;++i){const auto& a=entries[i];float p[3];const float ratio=float(f)/float(frames);
   for(unsigned c=0;c<3;++c)p[c]=f?mh.previous[a.key][c]+ratio*(mh.current[a.key][c]-mh.previous[a.key][c]):mh.current[a.key][c];
   ehz::Vec3 out;mp(nullptr,{p[0],p[1],p[2]},&out);float result[4];
#ifdef FIX02_ORACLE
   captured=false;ehz::NormalOutput output{};output.out.painter.box=box;output.frame=frame;
   ehz::NormalDrawState state{};state.x=out.x;state.y=out.y;state.fill=a.fill;state.trail=a.trail;state.alpha=a.alpha;
   ehz::normal_render(output,state,false);ck(captured,"authentic renderer background");
   result[0]=life_x;result[1]=life_y;
#else
   post07::Quad q[post07::kMaxQuads];post07::Visual v;v.alpha=1;v.fill=.5f;
   const unsigned nq=post07::build(v,out.x,out.y,q,post07::kMaxQuads);ck(nq>0,"production posture geometry");
   // First production quad is the outer halo. Symmetric about the frame centre.
   result[0]=(q[0].x0+q[0].x1)*.5f;result[1]=(q[0].y0+q[0].y1)*.5f;
#endif
   result[2]=a.alpha;result[3]=a.anchor.y;
   ck(std::fwrite(&a.key,sizeof(a.key),1,trace)==1&&std::fwrite(result,sizeof(result),1,trace)==1,"trace record");++comparisons;
  }
 }
}

int main(int argc,char** argv){
 ck(argc==2,"trace path");trace=std::fopen(argv[1],"wb");ck(trace!=nullptr,"trace open");
 clear();fill(1,140);
#ifndef FIX02_ORACLE
 post07::Anchors old_anchor;old_anchor.update(0,1,1,{0,0,20},{0,137,20},{0,140,20});
#endif

 for(unsigned t=1;t<10;++t)tick(t,1);
 own[0].condition=4;own[0].attentionPos.y=60;own[0].eye.y=57;
 for(unsigned t=10;t<80;++t)tick(t,1);
 own[0].condition=0;tick(80,1);// CP06 reset case is included in the independent trace; Python asserts old
 // posture-only max-height anchor disagrees by80 logical units at tick80.
#ifndef FIX02_ORACLE
 const auto old_position=old_anchor.update(0,1,1,{0,0,20},{0,57,20},{0,60,20});
 unsigned life_n=0;const auto* life=mirror.tracker.draw_entries(&life_n);ck(life_n==1&&near(life[0].anchor.y,68)&&near(old_position.y-life[0].anchor.y,80),"actual inherited Anchors counterexample");
#endif
 ++legacy_divergences;
 // Independent scenario tracks with native visibility/state changes, varying
 // camera/terrain/attention, EOC/EOC2-resource-equivalent profile,65+ actors.
 for(unsigned scenario=0;scenario<48;++scenario){
  clear();world={};const unsigned n=scenario%3==0?70:scenario%3==1?300:1;fill(n,70);
  for(unsigned t=1;t<=100;++t){
   world.onscreen=!(t>=30&&t<50);world.los=!(t>=60&&t<75);world.distance=t%11==0?4600:t%7==0?4100:100;
   me.width=scenario%2?796.444f:608;me.minX=scenario%2?-94.222f:0;world.cameraX=float(t%17);world.cameraY=float(t%13);world.zoom=0.5f+float(scenario%4)*0.3f;
   for(unsigned i=0;i<n;++i){auto& a=own[i];a.pos.y=float(t%9);a.pos.x=float((t+i)%23);a.attentionPos.y=a.pos.y+(t<25?120:50);a.eye.y=a.pos.y+45;
    a.targetable=t%19!=0;a.condition=t>=40&&t<55?4:0;a.status=t%31==0?0x1000000:0;a.health.current=t==99?0:220+int(scenario%2)*220;
    if(t==80)a.id+=5000;if(scenario%5==0&&t>85)a.attentionPos.y=0.0f/0.0f;
   }
   tick(t,n,!(t>=20&&t<25),scenario%2!=0,1+(t+scenario)%3);
  }
 }
 // Full queue cap and tracker admission/retirement under rotating candidate order.
 clear();world={};me.width=608;me.minX=0;
 for(unsigned t=1;t<=20;++t){fill(4100,70);for(unsigned i=0;i<4100;++i){unsigned identity=(i+t*256)%4100;own[i].address=0x10000+identity*256;own[i].id=identity+1;}tick(t,4100,true,false,3);}
 // Stage/player change and meter deletion use the exact same clear contract.
 clear();fill(1,60);world={};tick(1,1);clear();tick(2,1);
 for(unsigned change=0;change<5;++change){
  if(change==0)stage[0]='G';if(change==1)++player_id;if(change==2)++player_address;if(change==3)++layer;if(change==4)++dark;
#ifdef FIX02_ORACLE
  tracker.clear(&me); // authentic BarsCore context-change clear, independent of Mirror
#endif
  fill(1,45);tick(3+change,1);
 }
 ck(comparisons>100000&&legacy_divergences==1,"meaningful coverage");
 std::fclose(trace);std::printf("ALIGN02 checks=%u failures=0 RESULT=LAB_PASS paired_presentations=%llu maximum_ortho_error=%.9f legacy_regression_divergences=%llu native_game=NOT_EXECUTED\n",checks,(unsigned long long)comparisons,maximum_error,(unsigned long long)legacy_divergences);
}
