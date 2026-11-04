#include "../src/cp05_combo_core.hpp"
#include "../src/shield_chain_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static unsigned long long checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"CP05_FAIL line=%d %s\n",__LINE__,#x);std::exit(1);}}while(false)
using namespace cp05;
cp04::Key key(unsigned id=1){cp04::Key k;k.id=id;k.generation=2;k.stage=77;k.epoch=3;k.room=0;k.layer=1;return k;}
void probabilities(){
 unsigned result=99;CHECK(!reduce3(~std::uint64_t(0),result));CHECK(result==99);
 for(std::uint64_t x=0;x<300000;++x){CHECK(reduce3(x,result));CHECK(result==x%3);}
 for(auto variant:{Variant::Red,Variant::Black}){
  Counters c;Combo s;s.bind(key(),variant);
  for(unsigned i=1;i<=120000;++i){s.phase=Phase::Idle;s.execute(i,c);const auto k=s.decide(i,true,c);auto rng=s.rng;auto opp=s.opportunity;(void)s.decide(i,true,c);CHECK(s.opportunity==opp&&s.rng==rng);CHECK(k==Kind::Normal||count(k)>0);}
  auto& d=c.distribution[unsigned(variant)];CHECK(d.opportunities==120000);CHECK(d.normal+d.combo1+d.combo2==d.opportunities);
  if(variant==Variant::Red){CHECK(d.normal>58000&&d.normal<62000);CHECK(d.combo2==0);}
  else{CHECK(d.normal>38000&&d.normal<42000);CHECK(d.combo1>38000&&d.combo1<42000);CHECK(d.combo2>38000&&d.combo2<42000);}
 }
 Counters c;Combo s;s.bind(key(),Variant::Black);for(unsigned i=1;i<10000;++i){s.execute(i,c);s.decide(i,false,c);}CHECK(c.distribution[1].opportunities==0);
 Combo active;active.bind(key(),Variant::Red);active.kind=Kind::Red4;active.phase=Phase::Selected;auto active_rng=active.rng;(void)active.decide(100,true,c);CHECK(active.rng==active_rng&&active.opportunity==0);
 std::uint64_t seeds[64]{};for(unsigned i=0;i<64;++i){s.bind(key(i+1),Variant::Black);seeds[i]=s.seed;for(unsigned j=0;j<i;++j)CHECK(seeds[j]!=seeds[i]);}
 // Force the one rejected mixer output using its proven inverse: UINT64_MAX
 // is a fixed point of xor-right-shift only after inverse multiplication.
 auto unxor=[](std::uint64_t y,unsigned shift){std::uint64_t x=y;for(unsigned n=shift;n<64;n+=shift)x^=y>>n;return x;};
 std::uint64_t x=unxor(~std::uint64_t(0),31);x*=0x319642b2d24d8ec3ULL;x=unxor(x,27);x*=0x96de1b173f119089ULL;x=unxor(x,30);
 CHECK(cp04::mix(x)==~std::uint64_t(0));s.bind(key(),Variant::Black);s.rng=x-0x9e3779b97f4a7c15ULL;s.decide(1,true,c);CHECK(s.last_rejections==1&&s.last_draws==2);
}
void phases(){
 for(Kind kind:{Kind::Red4,Kind::Black8,Kind::Black7})for(int clip:{5,6})for(float end:{26.f,42.f,47.f,80.f,120.f,300.f}){
  Counters c;Combo s;s.bind(key(),kind==Kind::Red4?Variant::Red:Variant::Black);s.kind=kind;s.phase=Phase::Selected;
  std::uint64_t token=0;
  for(unsigned n=1;n<=count(kind);++n){s.local_tick+=2;CHECK(s.begin(clip,end,1,1.15f,c));CHECK(s.swing==n&&s.token==++token);s.local_tick+=10;s.arm(c);s.arm(c);CHECK(c.kind[unsigned(kind)].armed[n-1]==1);
   s.contact(false,false,false,c);s.contact(true,true,true,c);s.contact(true,true,true,c);CHECK(c.kind[unsigned(kind)].other[n-1]==1);CHECK(c.kind[unsigned(kind)].parries[n-1]==1);
   s.local_tick+=unsigned(end/1.5f)+1;s.finish(c);CHECK(!s.rate_owned);
   if(n<count(kind)){CHECK(s.phase==Phase::Gap);CHECK(s.launch_at>=s.local_tick+2);s.local_tick=s.launch_at;}
  }
  CHECK(s.phase==Phase::Recovery&&c.kind[unsigned(kind)].completed==1);CHECK(!s.begin(clip,end,1,1.15f,c));
 }
 for(float bad:{0.f,1.f,1.149f,1.151f,2.f,-1.f}){Counters c;Combo s;s.bind(key(),Variant::Red);s.kind=Kind::Red4;s.phase=Phase::Selected;CHECK(!s.begin(5,42,1,bad,c));}
 for(unsigned i=0;i<unsigned(Cancel::Count);++i){Counters c;Combo s;s.bind(key(),Variant::Black);s.kind=Kind::Black8;s.phase=Phase::Selected;s.cancel(Cancel(i),c);CHECK(s.phase==Phase::Idle);CHECK(c.kind[2].cancel[i]==1&&c.kind[2].not_started==1);s.cancel(Cancel(i),c);CHECK(c.kind[2].cancelled==1);}
 CHECK(count(Kind::Black8)==8&&count(Kind::Black7)==7&&count(Kind::Red4)==4);
 CHECK(target(Kind::Red4,0)>target(Kind::Red4,1)*3&&target(Kind::Red4,2)>target(Kind::Red4,1)*3);
 CHECK(target(Kind::Black8,0)>target(Kind::Black8,3)*3&&target(Kind::Black8,6)>target(Kind::Black8,3)*3);
 CHECK(target(Kind::Black7,0)<target(Kind::Black7,3)&&target(Kind::Black7,3)<target(Kind::Black7,1));
}
int main(){probabilities();phases();std::printf("CP05_LAB checks=%llu failures=0 RESULT=LAB_PASS native_runtime=0 visual=HUMAN_RUNTIME_PENDING\n",checks);}
