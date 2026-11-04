#include "../src/cp05_fix02_monitor.hpp"
#include <cstdio>
#include <limits>
#include <initializer_list>
using namespace cp05fix02;
static unsigned long long checks=0,failures=0;
void ck(bool v,const char* s){++checks;if(!v){++failures;std::printf("FIX02_FAIL %s\n",s);}}
post03::NativeAux aux(int state,bool c){post03::NativeAux a;a.valid=a.anim_ok=1;a.anim_attr=0;a.anim_end=c?47:42;a.anim_frame=state==3?0:float(a.anim_end)-.001f;a.anim_rate=0;a.set_bits=15;return a;}
int main(){
 // The exact observed frame/rate pairs for all five logged unsafe decisions.
 for(int n=0;n<5;++n){const int state=n==1?1:n==3?2:3;const bool c=n==3;auto a=aux(state,c);
  ck(lnks1fix::continuation(4,state,a,a)!=lnks1fix::Continuation::Safe,"0.11.9 reproduces logged terminal exclusion");
  ck(terminal(4,state,a,a,!c,c,true)==(state==3?Terminal::RecoilEnd:Terminal::ForwardEnd),"five genuine native terminal receipts accepted");
 }
 for(int variant=0;variant<4;++variant)for(bool c:{false,true})for(int state:{1,2,3})for(float f:{0.f,.001f,3.f,13.f,22.f,41.999f,42.f,46.999f,47.f})
 for(float rate:{-2.f,-1.f,0.f,1.f,1.15f,1.3f,1.5f,2.f})for(bool stop:{false,true}){
  auto a=aux(state,c);a.anim_frame=f;a.anim_rate=rate;a.resource=unsigned(variant?post03::NameKind::E_OC2:post03::NameKind::E_OC);a.atp=variant==0?1:variant==1?2:variant==2?3:6;
  const bool expected=stop&&rate==0&&f<=a.anim_end&&((state==3&&f<=.01f)||(state!=3&&((state==1)==!c)&&f>=float(a.anim_end)-.01f));
  ck((terminal(4,state,a,a,!c,c,stop)!=Terminal::None)==expected,"only authenticated native stopped endpoint, all variants/rates/states");
 }
 auto a=aux(1,false);
 for(int fault=0;fault<13;++fault){auto b=a;bool cb=true,cc=false,stop=true;int mode=4,state=1;
  if(fault==0)b.anim_frame-=1;if(fault==1)b.anim_rate=1;if(fault==2)b.anim_end=47;if(fault==3)b.anim_attr=2;if(fault==4)b.anim_ok=0;
  if(fault==5)cb=cc=false;if(fault==6)cb=cc=true;if(fault==7)stop=false;if(fault==8)mode=6;if(fault==9)state=5;
  if(fault==10)b.anim_frame=std::numeric_limits<float>::quiet_NaN();if(fault==11)b.speed=1;if(fault==12)b.valid=0;
  ck(terminal(mode,state,a,b,cb,cc,stop)==Terminal::None,"changed animation, physical priority or missing readback remains closed");
 }
 post08::Input p;p.fresh=p.tracked=p.link_ok=p.pass.valid=true;p.id=1;p.generation=1;p.hp=100;p.mode=4;p.state=1;p.at_hit[0]=p.at_on_link[0]=true;post08::Verdict v;v.first_contact=true;
 ck(cleanup_contact(p,v),"fresh actual own contact eligible for existing cleanup");
 for(int fault=0;fault<13;++fault){auto q=p;auto w=v;if(fault==0)q.fresh=false;if(fault==1)q.tracked=false;if(fault==2)q.dead=true;if(fault==3)q.frozen=true;if(fault==4)q.hp=1;if(fault==5)q.event_running=true;if(fault==6)q.link_ok=false;if(fault==7)q.pass.valid=false;if(fault==8)w.first_contact=false;if(fault==9)q.mode=5;if(fault==10)q.state=3;if(fault==11)q.at_hit[0]=false;if(fault==12)q.at_on_link[0]=false;ck(!cleanup_contact(q,w),"unsafe, duplicate, stale or foreign contact cannot acquire a lease");}

 for(unsigned actors:{1u,8u,64u})for(int variant=0;variant<4;++variant){
  GuardMonitor m;
  for(unsigned tick=1;tick<=360;++tick)for(unsigned i=0;i<actors;++i){
   Sample pre,post;post.valid=post.sim=post.input=post.target=post.held=true;post.guard=true;post.key.id=100+i;post.key.generation=i+1;post.key.stage=2;post.tick=tick;post.variant=variant;post.mode=3;post.state=4;post.t2=20-int(tick%20);post.range2=90000;pre=post;pre.t2=post.t2+1;
   const auto event=m.feed(i,pre,post);ck(event==(tick==1?GuardEvent::Start:tick==121||tick==241?GuardEvent::Persist:GuardEvent::None),"bounded continuous guard samples and genuine countdown");
   ck(m.feed(i,pre,post)==GuardEvent::None,"same frame duplicate never advances observation");
  }
  ck(m.c.starts==actors&&m.c.early==0&&m.c.persistent==actors*2,"normal countdown is not a guard stall");
  for(unsigned i=0;i<actors;++i){auto post=m.row(i).last;post.tick=361;post.held=false;ck(m.feed(i,post,post)==GuardEvent::End,"R release terminates observed episode");}
  ck(m.c.ends==actors,"all group episodes close without writes");
 }
 {GuardMonitor m;Sample a;a.valid=a.input=a.target=a.sim=a.held=true;a.key.id=1;a.key.generation=1;a.tick=1;m.feed(0,a,a);a.tick=2;auto pre=a;pre.mode=4;pre.state=1;pre.frame=1;pre.rate=1;pre.stale_link_hit=true;a.mode=4;a.state=3;
  ck(m.feed(0,pre,a)==GuardEvent::EarlyRebound&&m.c.early==1,"actual stale pre-window rebound classified separately");
  a.tick=3;a.key.generation=2;ck(m.feed(0,a,a)==GuardEvent::Start&&m.c.contexts==1,"generation reset retains no stale progression");
  ck(m.expire(0,64)&&m.c.expired==1,"culling expires observation as inconclusive");
 }
 {GuardMonitor m;Sample a;a.valid=a.input=a.target=a.sim=a.held=true;a.key.id=1;a.key.generation=1;a.tick=1;m.feed(0,a,a);a.tick=2;a.sim=false;ck(m.feed(0,a,a)==GuardEvent::Expired,"pause ends diagnostic only");}
 std::printf("CP05_FIX02_CORE checks=%llu failures=%llu RESULT=%s\n",checks,failures,failures?"FAIL":"LAB_PASS");return failures?1:0;
}
