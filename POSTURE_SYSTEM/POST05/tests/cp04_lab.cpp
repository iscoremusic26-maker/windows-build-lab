#include "../src/cp04_combo_core.hpp"
#include "../src/shield_chain_core.hpp"
#include "../src/parry_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static unsigned long long checks=0,failures=0;
#define CHECK(x) do{++checks;if(!(x)){++failures;if(failures<16)std::printf("FAIL line%d %s\n",__LINE__,#x);}}while(0)
using cp04::Phase;
cp04::Key key(unsigned id=1,unsigned gen=1){cp04::Key k;k.id=id;k.generation=gen;k.stage=123;k.epoch=2;k.room=0;k.layer=1;return k;}
void choose(cp04::Combo& s,cp04::Counters& c){for(unsigned i=1;i<200&&!s.active();++i)(void)s.decide(i,true,c);CHECK(s.phase==Phase::Selected);}
void chance(){
 unsigned total=0,disagree=0;cp04::Counters a,b;cp04::Combo x,y;x.bind(key(1));y.bind(key(2));
 for(unsigned t=1;t<=200000;++t){
  bool xx=x.decide(t,true,a),yy=y.decide(t,true,b);total+=xx;disagree+=xx!=yy;
  const auto n=a.opportunities;(void)x.decide(t,true,a);CHECK(a.opportunities==n);
  x.cancel(a);y.cancel(b);
 }
 CHECK(total>98000&&total<102000);CHECK(disagree>98000&&disagree<102000);
 CHECK(a.opportunities==200000&&a.normal+a.selected==a.opportunities);
 cp04::Counters c;cp04::Combo s;s.bind(key());
 for(unsigned i=1;i<10000;++i){CHECK(!s.decide(i,false,c));CHECK(c.opportunities==0);}
 for(unsigned g=1;g<=64;++g){s.bind(key(g,g));auto seed=s.seed;cp04::Combo same;same.bind(key(g,g));CHECK(seed==same.seed);same.bind(key(g,g+1));CHECK(seed!=same.seed);}
}
void combo(){
 for(int clip=5;clip<=6;++clip)for(int end=23;end<=120;++end)for(int group=0;group<3;++group){
  cp04::Combo s;cp04::Counters c;s.bind(key(unsigned(end)));choose(s,c);
  auto rolled=c.opportunities;std::uint64_t token=0;
  for(unsigned i=1;i<=4;++i){
   CHECK(s.execute(s.last_execute+1));CHECK(s.begin(clip,float(end),1.f,group==0?1.f:group==1?1.15f:1.3f,c));
   CHECK(s.swing==i&&s.token>token);token=s.token;CHECK(s.rate==(i<=2?1.10f:1.5f));
   s.rate_owned=true;CHECK(!s.decide(s.last_execute+1,true,c));CHECK(c.opportunities==rolled);
   s.finish(c);CHECK(!s.rate_owned);
   if(i<4){CHECK(s.phase==Phase::Gap);CHECK(s.remaining()==(i==3?2:8));
    for(unsigned n=s.gap;n;--n){CHECK(s.remaining()==n);auto old=s.local_tick;CHECK(!s.execute(s.last_execute));CHECK(s.local_tick==old);CHECK(s.execute(s.last_execute+1));}
    CHECK(s.remaining()==0);
   }else CHECK(s.phase==Phase::Recovery&&c.completed==1);
  }
  CHECK(!s.begin(clip,float(end),1.f,1.f,c));CHECK(c.starts[0]==1&&c.starts[1]==1&&c.starts[2]==1&&c.starts[3]==1);
 }
 for(unsigned swing=1;swing<=4;++swing){cp04::Combo s;cp04::Counters c;s.bind(key());choose(s,c);
  for(unsigned i=1;i<=swing;++i){CHECK(s.begin(5,42.f,1.f,1.f,c));if(i<swing)s.finish(c);}
  s.cancel(c,true);CHECK(!s.active()&&c.breaks==1);CHECK(!s.begin(6,47.f,1.f,1.f,c));
 }
 for(float foreign:{0.f,-1.f,1.4f,2.f,1.001f,1.151f,1.301f}){cp04::Combo s;cp04::Counters c;s.bind(key());choose(s,c);CHECK(!s.begin(5,42,1,foreign,c));}
}
void shield(){
 using shieldchain::Chain;Chain s;s.context(1,2,3);
 s.sample(1,true,true,true);CHECK(!s.permits(1));CHECK(s.native_start(1,1,true,true));
 s.observe(2,true,1,1,true);CHECK(!s.permits(2)); // proc/input without actual motion
 s.observe(3,true,2,1,true);CHECK(s.permits(3));CHECK(s.confirm(3));CHECK(!s.confirm(3));
 CHECK(!s.can_restart(4,true,true)); // held B supplies no input
 s.sample(4,true,false,true);CHECK(!s.can_restart(4,true,true));
 s.sample(5,true,true,true);CHECK(s.buffer);CHECK(!s.can_restart(5,true,false));
 CHECK(s.can_restart(8,true,true));CHECK(s.native_start(8,1,true,true));CHECK(s.native_serial==2); // real buffered press, not frame reset
 s.observe(9,true,2,1,true);CHECK(s.confirm(9));CHECK(s.c.matched==2&&s.c.native_starts==2);
 s.sample(10,true,true,true);CHECK(s.can_restart(10,true,true));CHECK(!s.native_start(10,1,false,true));
 CHECK(!s.native_start(10,1,true,false));
 s.observe(11,false,2,1,true);CHECK(!s.permits(11));s.observe(11,true,2,2,true);CHECK(!s.permits(11));
 s.context(1,3,3);CHECK(!s.armed&&!s.buffer);s.observe(12,true,2,1,true);CHECK(!s.permits(12));
 for(unsigned i=0;i<10;++i){Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));
  q.observe(2,true,2,1,true);CHECK(q.confirm(2));q.sample(3,true,true,true);
  CHECK(!q.can_restart(10,true,true));CHECK(!q.buffer);
  q.sample(11,false,false,true);CHECK(!q.armed);
 }
 {Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));q.observe(2,true,2,1,true);CHECK(q.confirm(2));q.sample(3,true,true,true);CHECK(!q.can_restart(3,false,true));CHECK(!q.armed&&!q.buffer);}
 {Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));q.observe(2,true,2,2,true);CHECK(!q.permits(2));q.observe(2,true,2,1,true);CHECK(q.permits(2));}
 {Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));q.observe(2,true,2,1,true);q.sample(2,true,true,true);CHECK(q.native_start(2,1,true,true));CHECK(q.confirm(2));CHECK(!q.armed);q.sample(3,true,true,true);CHECK(!q.can_restart(3,true,true));}
 {Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));q.observe(2,true,2,1,true);CHECK(q.confirm(2));q.context(1,0x100000001ULL,1);CHECK(!q.armed&&!q.pass_valid);}
 {Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));q.observe(2,true,2,1,true);CHECK(q.permits(2));CHECK(!q.native_start(2,1,true,true));CHECK(q.permits(2));CHECK(q.confirm(2));CHECK(!q.armed);q.observe(3,true,2,1,true);CHECK(!q.permits(3));}
 {Chain q;q.context(1,1,1);q.sample(1,true,true,true);CHECK(q.native_start(1,1,true,true));CHECK(!q.native_start(1,1,true,true));q.observe(2,true,2,1,true);CHECK(!q.permits(2));}
 // Each native init restarts the real POST08 serial, even if proc stays GUARD_ATTACK.
 post08::LinkTrack track;track.native_start();auto t=track.observe(1,true,1,30,true,2,1,12);CHECK(t.started);auto first=t.serial;
 track.native_start();t=track.observe(2,true,1,30,true,2,1,12);CHECK(t.started&&t.serial!=first);
 CHECK(track.pass_for(2,true,1).age_ticks==0);CHECK(track.pass_for(2,true,1).serial==t.serial);
}
// Native frame crossing / else-if path. 1.5 cannot cross both adjacent B cues
// (10->12.5 separation2.5), or C cues (7->11.5 separation4.5) in one update.
// Endpoints are deliberately a range, NOT a claim of asset lengths42/47.
void timing(){
 for(int clip=5;clip<=6;++clip)for(int end=23;end<=120;++end)for(int rate100:{100,115,130,150}){
  float rate=rate100/100.f,prev=0,frame=1;unsigned cues[3]{},active=0;
  const float times[3]={clip==5?10.f:7.f,clip==5?12.5f:11.5f,19.f};
  while(frame<end){bool fired=false;for(int i=0;i<(clip==5?3:2);++i){if(frame<=times[i]&&times[i]<frame+rate&&!fired){++cues[i];fired=true;}}
   if(frame>=14&&frame<=22)++active;prev=frame;frame+=rate;
  }
  CHECK(cues[0]==1&&cues[1]==1&&(clip==6||cues[2]==1));CHECK(active>=5);
 }
 // Consecutive full-clip fast swings leave >=18ticks even at the shortest
 // validated runtime end>22, enough for actual native shield18/cancel14/end12.
 CHECK((23-1)/1.5f+2+14/1.5f>18.f);
}
int main(){chance();combo();shield();timing();std::printf("CP04 checks=%llu failures=%llu RESULT=%s native_game_executed=false visual=VISUAL_SHIELD_MOTION_HUMAN_PENDING\n",checks,failures,failures?"LAB_FAIL":"LAB_PASS");return failures?1:0;}
