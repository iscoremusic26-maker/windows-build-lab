#include "shadow_policy.hpp"
#include <cstdio>
#include <limits>
#include <initializer_list>
using namespace rb_shadow;
unsigned checks=0,failures=0;void check(bool b){++checks;if(!b)++failures;}
Input good(){Input x;x.common_verified=x.profile_verified=x.clip_b=true;x.state=1;x.frame=16;x.rate=2;x.end=35;return x;}
// Independent truth-table oracle: conjunction of requirements, no call to phase().
bool oracle(const Input& x){
 bool clip=(x.state==1 && x.clip_b && !x.clip_c)||(x.state==2 && x.clip_c && !x.clip_b);
 bool rate=x.rate==1||x.rate==1.15f||x.rate==1.3f||x.rate==2;
 bool power=x.power_pre==x.power_post && (x.power_pre==1||x.power_pre==2||x.power_pre==6);
 return x.common_verified && x.profile_verified && x.mode==4 && clip && x.frame>2 && !x.stopped && x.playmode==0 && rate && power && x.end>25 && x.end<1000 && x.frame<x.end && x.frame+x.rate<x.end;
}
int main(){
 auto x=good();check(phase(x)==Gate::MaturePhaseCandidate);check(!release_enabled);
 for(int state=-1;state<5;++state)for(int f=0;f<40;++f)for(int p=0;p<8;++p)for(int rate=0;rate<5;++rate)for(unsigned mask=0;mask<16;++mask){
  x=good();x.state=state;x.clip_b=state==1;x.clip_c=state==2;x.frame=float(f);x.power_pre=x.power_post=p;
  const float rates[]={-1,1,1.15f,1.3f,2};x.rate=rates[rate];x.common_verified=mask&1;x.profile_verified=mask&2;x.stopped=mask&4;x.playmode=(mask&8)?1:0;
  check((phase(x)==Gate::MaturePhaseCandidate)==oracle(x));
 }
 for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-1.0f}){x=good();x.frame=bad;check(phase(x)!=Gate::MaturePhaseCandidate);x=good();x.rate=bad;check(phase(x)!=Gate::MaturePhaseCandidate);}
 x=good();x.power_post=6;check(phase(x)==Gate::PowerChanged);x=good();x.power_pre=x.power_post=3;check(phase(x)==Gate::Power3Posthook);
 x=good();x.state=0;check(phase(x)==Gate::AttackRateInitPending);x=good();x.mode=5;check(phase(x)!=Gate::MaturePhaseCandidate);x=good();x.clip_c=true;check(phase(x)!=Gate::MaturePhaseCandidate);
 std::printf("RB_SHADOW_LAB checks=%u failures=%u release_enabled=%d\n",checks,failures,int(release_enabled));return failures?1:0;
}
