#ifndef POST_HUDFIX_CORE_HPP
#define POST_HUDFIX_CORE_HPP
#include "lnk_core.hpp"

// 0.11.3: presentation only. No action names, actor pointers, input or combat.
namespace hudfix01 {
inline float absf(float a) noexcept {return a<0?-a:a;}
constexpr unsigned kNodes = 512, kDepth = 48, kStack = 4;
struct Rect {
  float x0=0,y0=0,x1=0,y1=0;
  bool valid() const noexcept { return post07::isfin(x0)&&post07::isfin(y0)&&post07::isfin(x1)&&post07::isfin(y1)&&x1>x0&&y1>y0; }
};
inline bool intersects(Rect a,Rect b) noexcept { return a.valid()&&b.valid()&&a.x0<b.x1&&a.x1>b.x0&&a.y0<b.y1&&a.y1>b.y0; }
inline bool contains(Rect a,Rect b) noexcept { return a.valid()&&b.valid()&&b.x0>=a.x0&&b.y0>=a.y0&&b.x1<=a.x1&&b.y1<=a.y1; }
inline Rect shift(Rect a,float x,float y) noexcept { return {a.x0+x,a.y0+y,a.x1+x,a.y1+y}; }
inline Rect grow(Rect a,float m) noexcept { return a.valid()?Rect{a.x0-m,a.y0-m,a.x1+m,a.y1+m}:Rect{}; }
inline Rect unite(Rect a,Rect b) noexcept {
  if(!a.valid())return b;if(!b.valid())return a;
  return {post07::minf(a.x0,b.x0),post07::minf(a.y0,b.y0),post07::maxf(a.x1,b.x1),post07::maxf(a.y1,b.y1)};
}
// Constant maximum envelope includes every quad of the unchanged Link painter,
// the expanding saturation pulse, end caps and an 8-reference-pixel margin.
inline Rect exclusion(const lnk01::Layout& l) noexcept {
  if(!l.ok)return {};
  return {l.cx-l.half_w-48*l.u,l.cy-l.frame_h-17*l.u,l.cx+l.half_w+48*l.u,l.cy+l.frame_h+17*l.u};
}
struct Matrix {
  float v[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
};
inline Matrix multiply(const Matrix& a,const Matrix& b) noexcept {
  Matrix r;
  for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x) {
    r.v[y][x]=x==3?a.v[y][3]:0;
    for(unsigned k=0;k<3;++k)r.v[y][x]+=a.v[y][k]*b.v[k][x];
  }
  return r;
}
inline Rect transformed(Rect a,const Matrix& m) noexcept {
  if(!a.valid())return {};
  Rect r;
  for(unsigned i=0;i<4;++i) {
    float x=(i&1)?a.x1:a.x0,y=(i&2)?a.y1:a.y0;
    float u=m.v[0][0]*x+m.v[0][1]*y+m.v[0][3],v=m.v[1][0]*x+m.v[1][1]*y+m.v[1][3];
    if(!post07::isfin(u)||!post07::isfin(v))return {};
    if(i==0)r={u,v,u,v};else {r.x0=post07::minf(r.x0,u);r.y0=post07::minf(r.y0,v);r.x1=post07::maxf(r.x1,u);r.y1=post07::maxf(r.y1,v);}
  }
  return r;
}
inline bool local_delta(const Matrix& parent,float dx,float dy,float& lx,float& ly) noexcept {
  const float d=parent.v[0][0]*parent.v[1][1]-parent.v[0][1]*parent.v[1][0];
  if(!post07::isfin(d)||absf(d)<0.00001f)return false;
  lx=(parent.v[1][1]*dx-parent.v[0][1]*dy)/d;
  ly=(-parent.v[1][0]*dx+parent.v[0][0]*dy)/d;
  return post07::isfin(lx)&&post07::isfin(ly);
}
struct Move { bool needed=false,ok=false;float dx=0,dy=0;Rect result; };
inline Move relocate(Rect box,Rect band,Rect viewport) noexcept {
  Move m;m.result=box;
  if(!intersects(box,band))return m;
  m.needed=true;
  // Preserve a whole prompt group whenever it fits. Nearest disjoint free strip
  // first; never hide a contextual action or invent a replacement label.
  const float dx[4]={0,0,band.x0-box.x1,band.x1-box.x0};
  const float dy[4]={band.y0-box.y1,band.y1-box.y0,0,0};
  float best=3.0e38f;
  for(unsigned i=0;i<4;++i) {
    Rect r=shift(box,dx[i],dy[i]);
    float cost=absf(dx[i])+absf(dy[i]);
    if(contains(viewport,r)&&!intersects(r,band)&&cost<best) {best=cost;m.ok=true;m.dx=dx[i];m.dy=dy[i];m.result=r;}
  }
  return m;
}
struct Node { int parent=-1;Matrix world;Rect own,all;float dx=0,dy=0;bool selected=false; };
struct Scene {
  Node nodes[kNodes]{};unsigned n=0;bool complete=true;
  void clear() noexcept { n=0;complete=true; }
  void aggregate() noexcept { for(unsigned i=n;i-->0;) {nodes[i].all=unite(nodes[i].all,nodes[i].own);if(nodes[i].parent>=0)nodes[nodes[i].parent].all=unite(nodes[nodes[i].parent].all,nodes[i].all);} }
  unsigned plan(Rect band,Rect viewport) noexcept {
    if(!complete)return 0;aggregate();unsigned chosen=0;
    for(unsigned i=0;i<n;++i) {
      bool ancestor=false;for(int p=nodes[i].parent;p>=0;p=nodes[p].parent)if(nodes[p].selected){ancestor=true;break;}
      if(ancestor)continue;
      const Move m=relocate(nodes[i].all,band,viewport);
      if(m.needed&&m.ok){nodes[i].selected=true;nodes[i].dx=m.dx;nodes[i].dy=m.dy;++chosen;}
    }
    return chosen;
  }
};
struct Presentation {
  bool on=false,inside=false;lnk01::Visual player;lnk01::Layout layout;Rect band,viewport;
  std::uint64_t serial=0,frames=0,painted=0,transitions=0;
  void begin(bool allowed,bool native_queued,const lnk01::Visual& value,const lnk01::Layout& l,Rect view) noexcept {
    const bool next=allowed&&native_queued&&value.alpha>=0.01f&&l.ok;
    if(next!=on)++transitions;
    on=next;inside=true;player=value;layout=l;viewport=view;band=next?exclusion(l):Rect{};++serial;if(next)++frames;
  }
  void end() noexcept {inside=false;}
};
} // namespace hudfix01
#endif
