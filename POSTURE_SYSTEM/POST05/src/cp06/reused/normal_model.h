#pragma once
#include "health_model.h"
namespace ehz {
struct NormalPoint {float x,y,z;};
struct NormalOptions { bool enabled=true,showNumbers=false; };
struct NormalSample { int profile,group,hp;uint32_t status,condition,attention;bool wolfNoLock,senses,boss,allied; };
inline bool normal_finite(float x){return x==x&&x<=3.402823466e38f&&x>=-3.402823466e38f;}
inline bool normal_ghost(int p){switch(p){case 432:case 474:case 481:case 487:case 489:case 500:case 502:case 505:case 506:case 507:return true;default:return false;}}
// Actor lifetime/HP tracking is independent of temporary presentation filters.
inline bool normal_trackable(const NormalSample&s){
 const unsigned p=(uint16_t)s.profile;
 const bool special=p>=238&&p<=268&&((0x55008001u>>(p-238))&1);
 if(special||s.group==3||s.group==4||s.hp<=0||s.boss||s.allied)return false;
 if(s.group!=2&&!(p>=560&&p<633)&&!(p>=431&&p<545)&&p!=210&&!(p>=228&&p<=245))return false;
 return true;
}
inline bool normal_eligible(const NormalSample&s){
 return normal_trackable(s)&&!(s.status&0x1000000)&&!(s.condition&4)&&
  (s.senses||!normal_ghost(s.profile));
}
inline float normal_height(const NormalPoint&pos,const NormalPoint&eye,const NormalPoint&attention){
 auto mag=[](const NormalPoint&v){return v.x*v.x+v.y*v.y+v.z*v.z;};
 float h=attention.y-pos.y;
 if(!(mag(attention)>.001f&&h>=20&&h<=150))h=mag(eye)>.001f?eye.y-pos.y:0;
 return h>=20&&h<=150?h:55;
}
inline bool normal_on_screen(const NormalPoint&out,float width,float height){
 if(!normal_finite(out.x)||!normal_finite(out.y)||!normal_finite(out.z))return false;
 const float right=width>0?width+80:720,bottom=height>0?height+80:560;
 return out.z<400000&&out.x>=-80&&out.x<=right&&out.y>=-80&&out.y<=bottom;
}
inline float normal_opacity(bool onScreen,bool attention,bool clear,float distance){
 if(!onScreen||!attention||!clear||!normal_finite(distance)||distance<0||distance>=4500)return 0;
 return distance<=3700?1:(4500-distance)/800;
}
inline void normal_step(HealthTrack&t,uint32_t tick,int hp,float target,bool onScreen){
 if(t.lastTick==tick)return;
 t.lastTick=tick;
 t.maximumHealth=maximum(t.maximumHealth,hp);
 t.fill=clamp01(float(maximum(0,hp))/maximum(1,t.maximumHealth));
 t.alpha+=(clamp01(target)-t.alpha)*.2f;
 if(t.alpha<.005f){t.alpha=0;t.anchorHeight=0;t.trail=-1;return;}
 if(!onScreen)return;
 t.trail=t.trail<0||t.fill>=t.trail?t.fill:t.trail+(t.fill-t.trail)*.12f;
}
// Published draw values never contain an actor identity/address or resource pointer.
struct NormalDrawState {float x=0,y=0,fill=0,trail=0,alpha=0;int hp=0,maximumHealth=1;};
static_assert(sizeof(NormalDrawState)==28);
inline char* normal_decimal(char*out,unsigned x){char b[10];unsigned n=0;do{b[n++]=char('0'+x%10);x/=10;}while(x);while(n)*out++=b[--n];return out;}
inline void normal_hp_text(char*out,int hp,int max){out=normal_decimal(out,unsigned(maximum(0,hp)));*out++='/';out=normal_decimal(out,unsigned(maximum(1,max)));*out=0;}
void normal_set_options(bool enabled,bool showNumbers);
NormalOptions normal_get_options();
const NormalDrawState* normal_snapshot(unsigned* count);
}
