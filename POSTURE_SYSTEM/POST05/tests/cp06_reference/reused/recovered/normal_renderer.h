#pragma once
#include "../normal_model.h"
#include "renderer.h"
namespace ehz {
struct NormalOutput {recovered::Output out;void(*frame)(void*,float,float,float,float,recovered::Color,uint8_t);};
inline void normal_render(const NormalOutput&o,const NormalDrawState&s,bool numbers){
 if(!normal_finite(s.x)||!normal_finite(s.y)||!normal_finite(s.fill)||!normal_finite(s.trail)||!normal_finite(s.alpha)||s.alpha<.005f)return;
 const float alpha=clamp01(s.alpha),fill=clamp01(s.fill),trail=recovered::bounded(s.trail,fill,1);
 auto c=[alpha](uint32_t rgb,int a){return recovered::color(rgb,int(alpha*a));};
 const auto&p=o.out.painter;float x=s.x-12.5f,y=s.y-.5f;
 p.solid(x-1.5f,y-1.5f,28,4,c(0,160));o.frame(p.context,x-1,y-1,27,3,c(0x3787aa,230),1);
 p.solid(x,y,25,1,c(0x100c0c,230));
 if(fill>0){
  float r,g,b;
  if(fill>.5f){float v=(fill-.5f)+(fill-.5f),inv=1-v;r=v*30+inv*230;g=inv*170+v*190;b=inv*30+v*70;}
  else{float v=fill+fill,inv=1-v;r=inv*220+v*230;g=170*v;b=inv*40+v*30;}
  recovered::Color color{uint8_t(r),uint8_t(g),uint8_t(b),uint8_t(alpha*240)};
  p.solid(x,y,25*fill,1,color);p.solid(x,y,25*fill,1,c(0xffffff,80));
 }
 if(trail>fill+.001f)p.solid(x+fill*25,y,(trail-fill)*25,1,c(0xc8e2ec,150));
 if(!numbers||!o.out.font.available)return;
 char text[32]{};normal_hp_text(text,s.hp,s.maximumHealth);
 float tx=x+(25-recovered::measure(o.out.font,text,7))*.5f,ty=y-3.5f;
 const uint8_t a=uint8_t(alpha*255);
 recovered::shadow(p,tx,ty,7,8.5f,text,recovered::color(0,a),.6f*1.7f);
 p.text(p.context,tx,ty,7,8.5f,text,recovered::color(0xf0fcff,a),recovered::color(0xa8d0e2,a),true);
}
}
