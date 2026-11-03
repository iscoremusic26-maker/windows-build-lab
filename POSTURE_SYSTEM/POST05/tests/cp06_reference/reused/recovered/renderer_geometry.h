#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace recovered {
inline float clamp01(float f) { return f<0?0:(f>1?1:f); }
struct Color { uint8_t r,g,b,a; };
inline Color rgba(uint32_t rgb,float alpha) {
    return {uint8_t(rgb>>16),uint8_t(rgb>>8),uint8_t(rgb),uint8_t(255*clamp01(alpha))};
}
// The source binary passes packed little-endian TColor, not hexadecimal RGB.
inline Color nativeColor(uint32_t packed,float alpha) {
    return {uint8_t(packed),uint8_t(packed>>8),uint8_t(packed>>16),uint8_t(255*clamp01(alpha))};
}
struct Painter {
    void* context;
    void (*box)(void*,float,float,float,float,Color,Color,Color,Color);
    void (*text)(void*,float,float,float,float,const char*,Color,Color,bool);
    void solid(float x,float y,float w,float h,Color c) const {
        if(w>0&&h>0)box(context,x,y,w,h,c,c,c,c);
    }
    void gradient(float x,float y,float w,float h,Color top,Color bottom) const {
        if(w>0&&h>0)box(context,x,y,w,h,top,top,bottom,bottom);
    }
};
struct Viewport { float minX=0,minY=0,maxX=608,maxY=448,safeMinX=0,safeMaxX=608,safeMaxY=448; };
struct BossStyle { int style=0;float offsetX=0,offsetY=0; };
inline float bounded(float v,float low,float high) { return v<low?low:(v>high?high:v); }
struct BossLayout { float x,y,w;bool thin; };
inline BossLayout bossLayout(Viewport v,BossStyle style) {
    if(!(v.maxX>v.minX+1)){v.minX=0;v.maxX=640;}
    float safeWidth=v.safeMaxX>v.safeMinX+1?v.safeMaxX-v.safeMinX:v.maxX-v.minX;
    bool thin=style.style==1;
    float w=thin?bounded(safeWidth*.54f,340,600):bounded(safeWidth*.30f,280,440);
    float x=(v.minX+v.maxX-w)*.5f+style.offsetX;
    float top=v.minY>=0&&v.minY<=200?v.minY:0;
    float bottom=v.safeMaxY>top+100?v.safeMaxY:v.maxY;
    float y=(thin?(bottom>top+100?bottom-84:364):top+41)+style.offsetY;
    return {x,y,w,thin};
}
// Both source styles use J2D primitives and the game's own Ruby font.
// Geometry stays in HUD coordinates, independent of output resolution.
inline void paintBoss(const Painter& p,Viewport v,BossStyle style,const char* name,const char* title,
                      float fill,float trail,float alpha) {
    fill=clamp01(fill);trail=bounded(trail,fill,1);alpha=clamp01(alpha);
    if(alpha<.01f)return;
    auto layout=bossLayout(v,style);float x=layout.x,y=layout.y,w=layout.w;bool thin=layout.thin;
    auto c=[alpha](uint32_t packed,int a){return Color{uint8_t(packed),uint8_t(packed>>8),uint8_t(packed>>16),uint8_t(alpha*a)};};
    if(thin){
        p.gradient(x-1,y-1.5f,w+2,7.5f,c(0,120),c(0,150));
        p.gradient(x,y,w,4.5f,c(1315866,200),c(592140,200));
        if(trail>fill+.001f)p.gradient(x+w*fill,y,w*(trail-fill),4.5f,c(8443120,235),c(4626116,235));
        p.gradient(x,y,w*fill,4.5f,c(2499254,250),c(1182828,250));
        p.gradient(x,y,w*fill,1,c(0x7480ff,110),c(0x7480ff,40));
        if(fill>0){
            float gleam=bounded(w*fill,0,10);Color clear=c(0xa0aaff,0),light=c(0xbac4ff,190);
            p.box(p.context,x+w*fill-gleam,y,gleam,4.5f,clear,light,clear,light);
            p.solid(x+w*fill-1.5f,y,1.5f,4.5f,c(0xccd6ff,235));
        }
        p.gradient(x,y+5.1f,w,1.7f,c(12904172,235),c(5011072,235));
        p.text(p.context,x+4,y-7,11.5f,13.5f,title?title:name,c(14346992,255),c(11847374,255),false);
    }else{
        // Exact frame helper (source 0x81270), including the inset highlight.
        auto frame=[&](float bx,float by,float bw,float bh){
            p.gradient(bx,by,bw,bh,c(0x07090c,245),c(0x040506,245));
            p.gradient(bx+1,by+1,bw-2,bh-2,c(0x7cbcdc,245),c(0x2c5874,245));
            p.gradient(bx+1,by+1,bw-2,bh*.35f+.5f,c(0xbae6f8,200),c(0xbae6f8,0));
        };
        frame(x-1.5f,y-4,w+3,4);frame(x-1.5f,y+10,w+3,4);
        frame(x-4,y-1.5f,4,13);frame(x+w,y-1.5f,4,13);
        p.solid(x-1,y-1,w+2,12,c(131587,245));
        p.gradient(x,y,w,10,c(2300703,240),c(854025,240));
        p.gradient(x,y,w,2,c(0,115),c(0,0));
        if(trail>fill+.001f)p.gradient(x+w*fill,y,w*(trail-fill),10,c(13165292,150),c(7774908,85));
        p.gradient(x,y,w*fill,5,c(8287460,245),c(2627794,245));
        p.gradient(x,y+5,w*fill,5,c(2627794,245),c(1313901,245));
        p.gradient(x,y,w*fill,5,c(0xffffff,85),c(0xffffff,0));
        if(fill>0&&fill<.999f)p.solid(x+w*fill-1,y,1.6f,10,c(0xd6f4ff,170));
        for(int i=1;i<4;++i)p.gradient(x+w*i*.25f,y+2,1,6,c(0,70),c(0,30));
        p.text(p.context,x+w*.5f,y-7.7f,16,18.5f,name,c(0xe8faff,255),c(9225442,255),true);
    }
}
}
