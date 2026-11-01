#include "black_ai_policy.hpp"
#include "cp19f2_policy.hpp"
#include "../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include <cstdio>
#include <cstring>
namespace ra=devf_red_ai; namespace ba=devf_black_ai; namespace dy=devf_dyn;
unsigned checks=0,failures=0;
void check(bool b,const char* w){++checks;if(!b){++failures;std::printf("FAIL %s\n",w);}}
auto re=[](const char*,ra::Cause,const char*){};
auto be=[](const char*,const char*){};
ra::Actor red(int state,float frame,float rate,bool hit){ra::Actor a;a.id=1;a.gen=1;a.variant=ra::Variant::Red;std::strcpy(a.stage,"TEST");a.room=0;a.layer=0;a.health=200;a.ready=true;a.mode=ra::Attack;a.state=state;a.frame=frame;a.rate=rate;a.end=35;a.b=state==1;a.c=state==2;a.hit=hit;return a;}
ba::Actor black(int state,float frame,float rate,bool hit){ba::Actor a;a.id=1;a.gen=1;a.stage=1;a.room=0;a.variant=ba::Variant::Black;a.health=400;a.mode=ba::Attack;a.state=state;a.frame=frame;a.rate=rate;a.end=35;a.b=state==1;a.c=state==2;a.hit=hit;return a;}
struct Io{unsigned char p[2]={3,3};int writes=0;unsigned char get(int i){return p[i];}void set(int i,unsigned char x){p[i]=x;++writes;}};
int main(){
 for(bool hit:{false,true}){
  ra::State s;auto a=red(0,0,1,hit);ra::Link l;ra::pre(s,a,l,re);a=red(1,1,1,false);auto out=ra::post(s,a,re);
  check(hit?(!out.write&&!s.owned):(out.write&&out.value==1.15f),"Red startup twin");
  for(unsigned fury:{0u,300u}){ba::State b;auto x=black(0,0,1,hit);ba::Rooms g;ba::Link k;ba::pre(b,x,k,g,fury,be);x=black(1,1,1,false);auto q=ba::post(b,x,fury,be);check(hit?(!q.write&&!b.pending&&!b.owned):(fury?b.pending:(q.write&&q.value==1.15f)),"Black startup twin");}
 }
 for(bool hit:{false,true}){
  ra::State s;s.decided=true;s.pending=true;s.clip=5;s.own_rate=1;s.attack_seq=7;auto a=red(1,1,1,hit);ra::Link l;ra::pre(s,a,l,re);a=red(1,2,1,false);auto out=ra::post(s,a,re);check(hit?(!out.write&&!s.pending&&!s.owned):(out.write&&out.value==2),"Red frame12 twin");
  ba::State b;b.decided=true;b.pending=true;b.clip=5;b.own_rate=1;b.seq=7;auto x=black(1,1,1,hit);ba::Rooms g;ba::Link k;ba::pre(b,x,k,g,300,be);x=black(1,2,1,false);auto q=ba::post(b,x,300,be);check(hit?(!q.write&&!b.pending&&!b.owned):(q.write&&q.value==2),"Black frame12 twin");
 }
 for(int state:{1,2})for(float rate:{1.15f,2.0f})for(int f=3;f<30;++f)for(bool hit:{false,true}){
  ra::State s;s.decided=true;s.owned=true;s.clip=state==1?5:6;s.own_rate=rate;s.attack_seq=7;s.last.seq=7;s.last_frame=float(f)-rate;auto a=red(state,float(f),rate,hit);ra::Link l;ra::pre(s,a,l,re);a.frame+=rate;auto out=ra::post(s,a,re);check(!out.write&&s.owned&&s.attack_seq==7&&s.own_rate==rate,"Red mature same episode/rate");
  ba::State b;b.decided=true;b.owned=true;b.clip=state==1?5:6;b.own_rate=rate;b.seq=7;b.last_frame=float(f)-rate;auto x=black(state,float(f),rate,hit);ba::Rooms g;ba::Link k;ba::pre(b,x,k,g,300,be);x.frame+=rate;auto q=ba::post(b,x,300,be);check(!q.write&&b.owned&&b.seq==7&&b.own_rate==rate,"Black mature same episode/rate");
 }
 for(unsigned fury:{0u,1u,299u,300u})for(bool loss:{false,true}){
  dy::Ledger L;Io io;io.p[0]=io.p[1]=fury?6:3;int identity=0;auto* e=L.record(1,&identity,1,dy::Variant::Black,3);e->owned=true;e->applied=fury?6:3;e->fury=fury;dy::capture_pre(*e,400);dy::Post p;p.post_health=loss?380:400;p.action=ba::Attack;auto ev=dy::post_update(L,*e,io,p,[](const dy::Entry&){return true;});unsigned want=loss?300:(fury?fury-1:0);check(e->fury==want&&io.get(0)==(want?6:3),"Black power timeline");if(loss)check(ev.kind==(fury?dy::Ev::FuryRefresh:dy::Ev::FuryStart),"Black distinct start/refresh");
 }
 for(unsigned gen=1;gen<=10;++gen){devf_cp19f2::ChipBlow b;devf_cp19f2::ChipView v;v.id=1;v.gen=gen;v.seq=1;v.fury=true;v.at_set=true;v.live=true;
  check(devf_cp19f2::chip_step(b,v)==devf_cp19f2::ChipOp::None,"register before shield");v.shield=true;
  check(devf_cp19f2::chip_step(b,v)==devf_cp19f2::ChipOp::Charge,"first shield chip");
  for(int i=0;i<100;++i)check(devf_cp19f2::chip_step(b,v)==devf_cp19f2::ChipOp::Duplicate,"persistent flags no duplicate chip");
 }
 std::printf("CP21_INTERACTION_PROBE checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
