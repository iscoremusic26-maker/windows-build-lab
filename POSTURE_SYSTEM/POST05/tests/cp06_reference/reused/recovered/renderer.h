#pragma once
// Stage 1 only: platform-neutral command plan. No hooks, game pointers or J2D calls.
#include "renderer_geometry.h"
extern "C" float powf(float,float);
namespace recovered {
struct FontMetrics {
    void* context;
    int (*width)(void*);
    uint8_t (*entry)(void*,signed char);
    bool available;
};
inline float measure(const FontMetrics& f,const char* s,float size) {
    if(!s)return 0;
    if(!f.available)return float(std::strlen(s))*size;
    float denominator=float(std::max(1,f.width(f.context))), total=0;
    for(;*s;++s){int w=f.entry(f.context,*s);total+=float(w?w:int(denominator))*(size/denominator);}
    return total;
}
struct OrnamentState {
    // root, background A, background B, left, right. Engine adapter owns pane lifetimes.
    float alpha[5], scales[5][2], bounds[5][4], globalLeft[4],globalRight[4];
    bool available;
};
struct Output {
    Painter painter;
    FontMetrics font;
    OrnamentState ornaments;
    void (*screen)(void*,float,float,const OrnamentState&);
};
inline uint32_t pack(Color c){return c.r|(uint32_t(c.g)<<8)|(uint32_t(c.b)<<16)|(uint32_t(c.a)<<24);}
inline Color color(uint32_t rgb,int a){return {uint8_t(rgb),uint8_t(rgb>>8),uint8_t(rgb>>16),uint8_t(a)};}
inline void frame(const Painter&p,float x,float y,float w,float h,float a) {
    p.gradient(x,y,w,h,color(0x07090c,int(245*a)),color(0x040506,int(245*a)));
    p.gradient(x+1,y+1,w-2,h-2,color(0x7cbcdc,int(245*a)),color(0x2c5874,int(245*a)));
    p.gradient(x+1,y+1,w-2,h*.35f+.5f,color(0xbae6f8,int(200*a)),color(0xbae6f8,0));
}
inline void ornaments(const Output& o,float x,float width,float y,float height,float alpha,float scale,float left,float right,float vertical) {
    if(!o.ornaments.available||!o.screen)return;
    // Work on a value snapshot. Original helper saves/restores the actual pane state.
    auto s=o.ornaments;
    s.alpha[0]=alpha;s.scales[0][0]=s.scales[0][1]=1;
    for(int n=1;n<=2;++n){s.alpha[n]=0;s.bounds[n][2]=s.bounds[n][0];s.bounds[n][3]=s.bounds[n][1];}
    for(int n=3;n<=4;++n){s.alpha[n]=0;s.scales[n][0]=s.scales[n][1]=scale;}
    o.screen(o.painter.context,0,0,s);
    float factor=scale/1.15f, extra2=factor+factor,extra23=factor*23;
    float halfWidth=-(s.globalLeft[2]-s.globalLeft[0])*scale*.5f;
    float center=y+height*.5f-(s.globalLeft[3]-s.globalLeft[1])*scale*.5f+extra2+vertical;
    float lx=(halfWidth+x-extra2+extra23+left)-s.globalLeft[0];
    float rx=(x+width+halfWidth+extra2+extra23+right)-s.globalRight[0];
    s.alpha[3]=alpha;o.screen(o.painter.context,lx,center-s.globalLeft[1],s);
    s.alpha[3]=0;s.alpha[4]=alpha;o.screen(o.painter.context,rx,center-s.globalRight[1],s);
}
inline const char* title(const char* n) {
    if(!n)return "Boss";
    struct Pair{const char*n;const char*t;};
    static constexpr Pair table[]={
        {"Diababa","Diababa, the Twilit Parasite"},{"Fyrus","Fyrus, the Twilit Igniter"},
        {"Morpheel","Morpheel, the Twilit Aquatic"},{"Stallord","Stallord, the Twilit Fossil"},
        {"Blizzeta","Blizzeta, the Twilit Ice Mass"},{"Armogohma","Armogohma, the Twilit Arachnid"},
        {"Argorok","Argorok, the Twilit Dragon"},{"Zant","Zant, the Usurper King"},
        {"Phantom Zant","Phantom Zant, the False King"},{"Puppet Zelda","Zelda, the Puppet Princess"},
        {"Ganondorf","Ganondorf, the Dark Lord"},{"Dark Beast Ganon","Ganon, the Dark Beast"},
        {"King Bulblin","King Bulblin, the Warlord"},{"Death Sword","Death Sword, the Cursed Blade"},
        {"Darkhammer","Darkhammer, the Iron Wall"},{"Deku Toad","Deku Toad, the Tunnel Dweller"},
        {"Dangoro","Dangoro, the Goron Guardian"},{"Ook","Ook, the Forest Thief"},
        {"Darknut","Darknut, the Iron Knight"},{"Aeralfos","Aeralfos, the Sky Guard"}
    };
    for(const auto& e:table)if(std::strcmp(n,e.n)==0)return e.t;
    return n;
}
inline void shadow(const Painter& p,float x,float y,float w,float h,const char* text,Color c,float radius) {
    if(!text||!c.a||radius<=0)return;
    float a=1-::powf(1-float(c.a)/255,1.f/3.f);
    c.a=uint8_t(a*255+.5f);
    static constexpr float dirs[18][2]={
        {1,0},{.866025388f,.5f},{.499999970f,.866025448f},{-4.37113883e-8f,1},
        {-.50000006f,.866025388f},{-.866025388f,.50000006f},{-1,-8.74227766e-8f},
        {-.866025269f,-.500000179f},{-.499999911f,-.866025448f},{1.192488e-8f,-1},
        {.499999911f,-.866025448f},{.866025567f,-.499999762f},
        {.476313978f,.275000006f},{-2.404126e-8f,.550000012f},{-.476313978f,.275000036f},
        {-.476313919f,-.275000095f},{6.558685e-9f,-.550000012f},{.476314068f,-.274999887f}};
    for(auto& d:dirs)p.text(p.context,x+d[0]*radius,y+d[1]*radius,w,h,text,c,c,true);
}
struct TextContext { const Output* out; float alpha; };
inline void text_plan(void* ctx,float x,float y,float w,float h,const char* name,Color top,Color bottom,bool smallcaps) {
    const auto& o=*static_cast<TextContext*>(ctx)->out;const auto&p=o.painter;
    char text[64]{};size_t max=smallcaps?47:63;
    if(!name)name="Boss";
    for(size_t i=0;i<max&&name[i];++i){char c=name[i];text[i]=smallcaps&&c>='a'&&c<='z'?char(c-32):c;}
    if(!smallcaps){
        if(!o.font.available)return;
        shadow(p,x,y,w,h,text,color(0,int(static_cast<TextContext*>(ctx)->alpha*150)),1);
        p.text(p.context,x,y,w,h,text,top,bottom,true);return;
    }
    auto large=[&](size_t i){return i==0||text[i]==' '||text[i-1]==' ';};
    float total=0;
    for(size_t i=0;text[i];++i){char letter[2]={text[i],0};total+=measure(o.font,letter,large(i)?16.f:11.52f);}
    x+=total*-.5f;
    for(size_t i=0;text[i];){
        size_t start=i;bool big=large(i++);while(text[i]&&large(i)==big)++i;
        char run[48]{};std::memcpy(run,text+start,i-start);
        float cw=big?16.f:11.52f,ch=big?18.5f:13.320000648498535f;
        if(o.font.available){
            shadow(p,x,y,cw,ch,run,color(0x0c1420,int(float(top.a)*.9f)),1.1f);
            p.text(p.context,x,y,cw,ch,run,top,bottom,true);
        }
        x+=measure(o.font,run,cw);
    }
}
inline void rect_plan(void* ctx,float x,float y,float w,float h,Color a,Color b,Color c,Color d) {
    const auto&p=static_cast<TextContext*>(ctx)->out->painter;p.box(p.context,x,y,w,h,a,b,c,d);
}
inline void render(const Output& o,Viewport v,BossStyle s,const char* name,float fill,float trail,float alpha) {
    if(alpha<.01f)return;
    alpha=clamp01(alpha);TextContext ctx{&o,alpha};Painter p{&ctx,rect_plan,text_plan};
    paintBoss(p,v,s,name,title(name),fill,trail,alpha);
    auto layout=bossLayout(v,s);float x=layout.x,y=layout.y,w=layout.w;bool thin=layout.thin;
    // Preserve original operation order for the lower rail's height.
    ornaments(o,x,w,y,thin?((y+5.1f)+1.7f-y):10,alpha,thin?.62f:1.15f,thin?-27.f:0,thin?-34.f:0,thin?-9.f:0);
}
}
