#pragma once
#include "normal_model.h"
namespace ehz {
// Runtime presentation differs deliberately from the recovered original model.
// Keep body X/Z (no animated head bob), allow tall valid heads and ease only
// upward changes. Camera/actor movement is never delayed in screen space.
inline float stable_head_height(float previous,const NormalPoint&pos,const NormalPoint&eye,const NormalPoint&attention){
 float h=attention.y-pos.y;
 if(!normal_finite(h)||h<20||h>600){h=eye.y-pos.y;}
 if(!normal_finite(h)||h<20||h>600)h=55;
 if(!(previous>0))return h;
 if(h>previous+3){float step=(h-previous)*.2f;return previous+(step>3?3:step);}
 return previous;
}
inline int baba_display_health(int hp,bool valid,int action,int mode,bool dying){
 if(!valid)return hp;
 if(action==21||(action==20&&(mode>=50||dying)))return 0;
 // Native damage_check sets HP=0 at <=10 while ACTION_ESCAPE can stay alive.
 // A one-unit presentation floor keeps its bar; no write to actor HP occurs.
 return action==20?maximum(1,hp):hp;
}
}
