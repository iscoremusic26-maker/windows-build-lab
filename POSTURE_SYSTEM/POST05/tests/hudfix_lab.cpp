#include "../src/hudfix_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <initializer_list>
using namespace hudfix01;
static unsigned checks=0;
static void ck(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"HUD_FIX01 FAIL %s\n",why);std::exit(1);}}
static bool near(float a,float b){return absf(a-b)<0.002f;}
int main(){
  // All original player quads, including the final fading/empty frame, must
  // remain inside one maximum envelope. This reads the ACTUAL inherited builder.
  for(float width:{608.0f,796.444f,1066.667f})for(float minx:{0.0f,-94.222f}) {
    const auto l=lnk01::layout(minx,0,width,448);const Rect band=exclusion(l),view{minx,0,minx+width,448};
    const Move nearest=relocate({l.cx-20,band.y0-20,l.cx+20,band.y0+5},band,view);
    ck(nearest.ok&&near(nearest.dx,0)&&near(nearest.dy,-5),"nearest free strip keeps contextual hint close");
    const Rect oversized{view.x0-10,band.y0-1,view.x1+10,band.y1+1};
    const Move cannot_fit=relocate(oversized,band,view);
    ck(cannot_fit.needed&&!cannot_fit.ok,"never relocate a hint beyond viewport");
    Scene wide;wide.n=1;wide.nodes[0].own=oversized;
    ck(wide.plan(band,view)==0,"occupied-space planner refuses oversized hints atomically");
    for(unsigned alpha=1;alpha<=100;alpha+=3)for(unsigned pulse=0;pulse<=100;++pulse) {
      lnk01::Visual v;v.alpha=alpha/100.0f;v.fill=pulse/100.0f;v.pulse=pulse/100.0f;v.hit=1;v.recover=1;v.saturated=true;
      post07::Quad q[lnk01::kMaxQuads];const unsigned n=lnk01::build_bar(v,l,q,lnk01::kMaxQuads);
      for(unsigned i=0;i<n;++i)ck(contains(band,{q[i].x0,q[i].y0,q[i].x1,q[i].y1}),"pulse/fade envelope");
    }
    // Prompt names never enter the algorithm: the same geometry covers Scoop,
    // A/B/R, several bottle slots, technique names, text, auxiliary and sparkles.
    for(int y=280;y<448;++y)for(int w=12;w<=300;w+=12) {
      Rect box{l.cx-w/2.0f,float(y),l.cx+w/2.0f,float(y+24)};
      const Move m=relocate(box,band,view);
      if(intersects(box,band)){ck(m.needed&&m.ok,"native prompt can move");ck(!intersects(m.result,band)&&contains(view,m.result),"disjoint in viewport");}
      else ck(!m.needed&&near(m.result.y0,box.y0),"unaffected noncompeting hint");
    }
    // Main HUD: health occupies the upper region. The screen cannot move as a
    // whole, so ONLY the bottom prompt subtree moves, keeping its child layout.
    Scene s;s.n=6;
    s.nodes[0].parent=-1;
    s.nodes[1].parent=0;s.nodes[1].own={minx+10,10,minx+200,50};
    s.nodes[2].parent=0;
    s.nodes[3].parent=2;s.nodes[3].own={l.cx-130,l.cy-10,l.cx+110,l.cy+10};
    s.nodes[4].parent=2;s.nodes[4].own={l.cx+110,l.cy-14,l.cx+140,l.cy+14};
    s.nodes[5].parent=0;s.nodes[5].own={minx+width-120,55,minx+width-10,110};
    ck(s.plan(band,view)==1,"move whole bottom subtree");
    ck(!s.nodes[0].selected&&!s.nodes[1].selected&&s.nodes[2].selected&&!s.nodes[3].selected,"health and child positions preserved");
    for(unsigned i=3;i<=4;++i)ck(!intersects(shift(s.nodes[i].own,s.nodes[2].dx,s.nodes[2].dy),band),"text and icon remain together");
    s.complete=false;ck(s.plan(band,view)==0,"overflow performs no writes");
    // Previously reproduced adapter failure: clearing Link must not move a
    // hint over an unrelated native hint above it. Include two independent
    // competing groups, and a viewport where the whole root WOULD fit above.
    Scene occupied;occupied.n=5;occupied.nodes[0].parent=-1;
    occupied.nodes[1].parent=0;occupied.nodes[1].own={l.cx-70,330,l.cx+70,360};
    occupied.nodes[2].parent=0;occupied.nodes[2].own={l.cx-70,375,l.cx+70,400};
    occupied.nodes[3].parent=0;occupied.nodes[3].own={l.cx-70,400,l.cx+70,420};
    occupied.nodes[4].parent=0;occupied.nodes[4].own={minx+15,30,minx+160,60};
    occupied.plan(band,view);
    ck(!occupied.nodes[0].selected&&!occupied.nodes[1].selected&&!occupied.nodes[4].selected,"unrelated hints and health never move with root");
    ck(!intersects(occupied.placed(1),occupied.placed(2)),"relocated hint avoids existing native hint");
    ck(!intersects(occupied.placed(2),occupied.placed(3)),"independent relocated hints avoid one another");
    ck(!intersects(occupied.placed(2),band),"occupied-space placement still clears bar");
    ck(contains(view,occupied.placed(2)),"occupied-space relocation remains in viewport");
    Scene root_guard;root_guard.n=3;root_guard.nodes[0].parent=-1;
    root_guard.nodes[1].parent=0;root_guard.nodes[1].own={minx+15,40,minx+160,60};
    root_guard.nodes[2].parent=0;root_guard.nodes[2].own={l.cx-70,375,l.cx+70,395};
    root_guard.plan(band,view);
    ck(!root_guard.nodes[0].selected&&root_guard.placed(1).y0==40,"whole-root fitting elsewhere must not move native health");
  }
  // Rotation/scale parent inverse; moving text through a transformed parent
  // must produce the requested SCREEN displacement, not an accumulated drift.
  Matrix parent;parent.v[0][0]=0.8f;parent.v[0][1]=-0.4f;parent.v[1][0]=0.4f;parent.v[1][1]=0.8f;
  for(int dy=-180;dy<=180;++dy){float lx=0,ly=0;ck(local_delta(parent,3,float(dy),lx,ly),"inverse exists");ck(near(parent.v[0][0]*lx+parent.v[0][1]*ly,3)&&near(parent.v[1][0]*lx+parent.v[1][1]*ly,float(dy)),"correct affine displacement");}
  parent.v[0][0]=parent.v[1][1]=parent.v[0][1]=parent.v[1][0]=0;float lx,ly;ck(!local_delta(parent,1,1,lx,ly),"singular transform refuses before lease");
  // 30/60/irregular presentation, no enemies, 20 consecutive sword contacts,
  // queue refusal, enemy identity reuse and empty enemy packet do not appear in
  // the player presentation predicate. Native pause/menu/hide still do.
  for(unsigned fps=0;fps<3;++fps){lnk01::PlayerHud hud;Presentation p;unsigned rendered=0;
    for(unsigned tick=0;tick<240;++tick){hud.tick(tick<90?50000:0,true,true);const auto before=hud.visual();const unsigned frames=fps==0?1:fps==1?2:1+(tick%4);
      for(unsigned f=0;f<frames;++f){const bool allowed=!(tick>=40&&tick<45);const auto layout=lnk01::layout(0,0,796.444f,448);p.begin(allowed,true,before,layout,{0,0,796.444f,448});ck(p.on==(allowed&&hud.drawable()),"same-frame independent player visibility");if(p.on){++p.painted;++rendered;}p.end();ck(!p.inside,"paired frame");ck(near(hud.visual().alpha,before.alpha),"presentation never advances fade");}
    }
    ck(p.frames==p.painted&&rendered==p.painted,"exactly once per required frame");ck(!hud.drawable(),"legitimate zero hold/fade finishes");
  }
  // PRE/POST leases restore bit-exact authored values for a million frames.
  float ty=319.125f,my=319.125f;
  for(unsigned i=0;i<1000000;++i){const float a=ty,b=my;ty+=float(i%80)-40;my+=float(i%80)-40;ty=a;my=b;ck(ty==319.125f&&my==319.125f,"no accumulation");}
  Presentation p;lnk01::Visual value;value.alpha=1;auto l=lnk01::layout(0,0,796.444f,448);p.begin(true,false,value,l,{0,0,796.444f,448});ck(!p.on,"minimal HUD/no native packet");
  value.alpha=0.009f;p.begin(true,true,value,l,{0,0,796.444f,448});ck(!p.on&&!p.band.valid(),"native UI unaffected without actual bar");
  std::printf("HUD_FIX01 checks=%u failures=0 RESULT=LAB_PASS model_only=1 EHB_ALIGNMENT=NOT_TESTED NATIVE_DRAW=HUMAN_RUNTIME_PENDING\n",checks);
}
