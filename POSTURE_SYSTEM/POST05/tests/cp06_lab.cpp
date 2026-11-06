// CP06 0.11.11 value laboratory: fury authentication, attributed chip guard, late-input
// diagnosis and the approved per-index rhythm profiles. Pure logic; no game, no DLL.
#include "../src/cp06_combat_core.hpp"
#include "../src/cp05_combo_core.hpp"
#include "../src/parry_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <initializer_list>
static unsigned long long checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"CP06_FAIL line=%d %s\n",__LINE__,#x);std::exit(1);}}while(false)
using namespace cp06;
void fury_truth_table(){
 const float inf=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN();
 const float rates[]={nan,-inf,-1.f,0.f,1.f,1.05f,1.15f,1.2f,1.3f,1.35f,1.5f,1.999f,2.f,2.001f,inf};
 unsigned yes=0;
 for(bool e2:{false,true})for(int atp:{-2,-1,0,1,2,3,4,5,6,7,100})for(bool ok:{false,true})for(float r:rates)for(bool v:{false,true}){
  const bool want=e2&&atp==6&&ok&&r==2.f&&v;CHECK(fury_parry_authenticated(e2,atp,ok,r,v)==want);yes+=want;
  // the legacy gate is only relaxed for an authenticated fury input
  post08::Input in;in.e_oc2=e2;in.atp=atp;in.anim_ok=ok;in.anim_rate=r;in.fury_verified=want;
  const bool legacy=e2&&atp!=2&&(!ok||!(r<1.5f));
  CHECK(post08::Parry::fury_chip_possible(in)==(legacy&&!want));
 }
 CHECK(yes==1);
}
ChipView view(float p,unsigned char t,int life,bool ok=true,bool pair=true){ChipView v;v.pending=p;v.type=t;v.life=life;v.link_ok=ok;v.shield_pair=pair;return v;}
void chip_cases(){
 struct Case{const char* n;ChipView before,after;Chip want;float restore;};
 const Case cases[]={
  {"exact",view(0,0,40),view(-1,0,40),Chip::Neutralized,1},
  {"exact nonzero pending",view(-24,1,40),view(-25,1,40),Chip::Neutralized,1},
  {"no chip",view(0,0,40),view(0,0,40),Chip::NoChipObserved,0},
  {"double",view(0,0,40),view(-2,0,40),Chip::DeltaMismatch,0},
  {"half",view(0,0,40),view(-.5f,0,40),Chip::DeltaMismatch,0},
  {"heal",view(0,0,40),view(1,0,40),Chip::DeltaMismatch,0},
  {"type",view(0,0,40),view(-1,3,40),Chip::TypeChanged,0},
  {"life",view(0,0,40),view(-1,0,39),Chip::LifeChanged,0},
  {"link after",view(0,0,40),view(-1,0,40,false),Chip::LinkInvalid,0},
  {"no pair",view(0,0,40,true,false),view(-1,0,40),Chip::DeltaMismatch,0},
 };
 for(const auto& k:cases)for(unsigned slot:{0u,17u,63u}){
  ChipGuard g;CHECK(g.arm(slot,9,4,77,100));CHECK(g.capture(slot,9,4,100,k.before));float r=-5;std::uint8_t t=0xee;
  const Chip c=g.decide(slot,9,4,100,k.after,r,t);CHECK(c==k.want);CHECK(r==k.restore);
  if(c==Chip::Neutralized){CHECK(t==k.after.type);CHECK(k.after.pending+r==k.before.pending);}
  CHECK(!g.row(slot).armed);
  // one neutralization per token, ever; a NEW token is a new blow
  CHECK(!g.arm(slot,9,4,77,101));CHECK(g.arm(slot,9,4,78,102));
 }
 {ChipGuard g;float r;std::uint8_t t;CHECK(g.decide(3,1,1,5,view(-1,0,40),r,t)==Chip::None&&r==0);CHECK(!g.capture(3,1,1,5,view(0,0,40)));}
 {ChipGuard g;float r;std::uint8_t t;CHECK(g.arm(3,1,1,9,5));CHECK(g.decide(3,1,1,5,view(-1,0,40),r,t)==Chip::NotCaptured&&r==0);}
 {ChipGuard g;float r;std::uint8_t t;CHECK(g.arm(3,1,1,9,5));CHECK(!g.capture(3,1,2,5,view(0,0,40)));CHECK(!g.capture(3,2,1,5,view(0,0,40)));CHECK(!g.capture(3,1,1,6,view(0,0,40)));CHECK(g.capture(3,1,1,5,view(0,0,40)));CHECK(!g.capture(3,1,1,5,view(0,0,40)));
  CHECK(g.decide(3,1,2,5,view(-1,0,40),r,t)==Chip::ContextChanged&&r==0);}
 {ChipGuard g;float r;std::uint8_t t;CHECK(g.arm(3,1,1,9,5));CHECK(g.capture(3,1,1,5,view(0,0,40)));CHECK(g.decide(3,1,1,6,view(-1,0,40),r,t)==Chip::NotCaptured&&r==0);}
 {ChipGuard g;CHECK(g.arm(4,1,1,9,5));CHECK(!g.arm(4,1,1,9,5));CHECK(!g.arm(5,1,1,9,5));CHECK(g.c.duplicate_arm==2);CHECK(!g.arm(64,1,1,9,5));CHECK(!g.arm(4,0,1,9,5));CHECK(!g.arm(4,1,1,0,5));}
 {ChipGuard g;float r;std::uint8_t t;CHECK(g.arm(1,1,1,9,5));CHECK(!g.capture(1,1,1,5,view(0,0,40,false)));CHECK(!g.row(1).armed);CHECK(g.decide(1,1,1,5,view(-1,0,40),r,t)==Chip::None);}
 // 64 independent slots in one tick (multi-actor)
 {ChipGuard g;CHECK(g.armed_count()==0);for(unsigned s=0;s<64;++s){CHECK(g.arm(s,s+1,2,s+10,7));CHECK(g.armed_count()==s+1);CHECK(g.capture(s,s+1,2,7,view(float(s),0,40)));}
  for(unsigned s=0;s<64;++s){float r;std::uint8_t t;const Chip c=g.decide(s,s+1,2,7,view(float(s)-(s%2?1.f:0.f),0,40),r,t);CHECK(c==(s%2?Chip::Neutralized:Chip::NoChipObserved));CHECK(g.armed_count()==63-s);}
  g.reset();CHECK(g.armed_count()==0);}
}
// Bounded random sequences: global invariants of the guard.
void chip_fuzz(){
 std::uint64_t x=0x9e3779b97f4a7c15ULL;auto rnd=[&](unsigned n){x^=x<<13;x^=x>>7;x^=x<<17;return unsigned(x%n);};
 for(unsigned run=0;run<4000;++run){
  ChipGuard g;unsigned restores[4][4]{};
  for(unsigned step=0;step<64;++step){
   const unsigned slot=rnd(4),id=1+rnd(2),token=1+rnd(4);const std::uint64_t tick=1+rnd(6);
   const unsigned op=rnd(3);
   if(op==0)(void)g.arm(slot,id,1,token,tick);
   else if(op==1)(void)g.capture(slot,id,1,tick,view(float(int(rnd(5))-2),std::uint8_t(rnd(2)),40,rnd(8)!=0,rnd(8)!=0));
   else{const auto row=g.row(slot);float r=0;std::uint8_t t=0;const ChipView now=view(float(int(rnd(5))-3),std::uint8_t(rnd(2)),40-int(rnd(2)),rnd(8)!=0,true);
    const Chip c=g.decide(slot,id,1,tick,now,r,t);
    if(c==Chip::Neutralized){CHECK(row.armed&&row.captured&&row.id==id&&row.tick==tick);CHECK(now.pending==row.before.pending-1.f&&now.type==row.before.type&&now.life==row.before.life&&row.before.shield_pair&&row.before.link_ok&&now.link_ok);CHECK(r==1.f);
     CHECK(row.token<4u+1u);CHECK(++restores[slot][row.token-1]<=4);}
    else CHECK(r==0.f);
    CHECK(!g.row(slot).armed);}
   {unsigned n=0;for(unsigned q=0;q<4;++q)n+=g.row(q).armed;CHECK(g.armed_count()==n);}
  }
 }
}
void late_input(){
 LateInput L;unsigned emitted=0;unsigned last=99;auto emit=[&](const LateInput::Event&,unsigned d){++emitted;last=d;};
 L.contact(5,1,10,100,1,int(post08::Result::NormalGuard),2.f,18.f);
 L.input(99,emit);CHECK(emitted==0);              // before the judged tick: not late
 L.input(100,emit);CHECK(emitted==1&&last==0);    // same tick as the judge = one tick after the pass
 L.input(101,emit);CHECK(emitted==1);             // reported once
 CHECK(L.c.late==1&&L.c.by_delta[0]==1&&L.c.rate2==1&&L.c.by_variant[1]==1);
 L.contact(6,1,11,200,9,int(post08::Result::NotBlocked),1.15f,17.f);L.input(209,emit);CHECK(emitted==1);  // beyond the 8-tick horizon
 L.contact(7,1,12,300,2,0,1.3f,17.f);L.input(308,emit);CHECK(emitted==2&&last==8&&L.c.by_variant[2]==1);
 for(unsigned n=0;n<40;++n)L.contact(n,1,n,400+n,0,0,1.f,17.f);CHECK(L.c.contacts==43);   // bounded ring overwrite
 L.input(440,emit);CHECK(emitted<=2+LateInput::kRing);
 LateInput M;M.contact(1,1,1,10,9,0,1,1);M.input(10,emit);CHECK(M.c.by_variant[4]==1);
}
unsigned armed(float rate,float& first_pose,float& last_pose){
 float f=1.f;unsigned n=0;first_pose=-1;last_pose=-1;
 for(unsigned t=0;t<80&&f<42.f;++t){if(f>=14.f&&f<=22.f){if(first_pose<0)first_pose=f+rate;last_pose=f+rate;++n;}f+=rate;}
 return n;
}
void rhythm(){
 using namespace cp05;
 const float r4[]={1.05f,1.35f,1.35f,1.05f},b8[]={1.05f,1.05f,1.30f,1.30f,1.30f,1.30f,1.30f,1.05f},b7[]={1.30f,1.30f,1.05f,1.05f,1.20f,1.20f,1.20f};
 const unsigned t4[]={60,36,62},t8[]={62,62,38,38,38,38,64},t7[]={38,62,64,47,47,47};
 const unsigned o4[]={68,46,72},o8[]={65,67,46,46,46,46,70},o7[]={46,68,70,56,56,56};
 for(unsigned n=1;n<=4;++n)CHECK(rate(Kind::Red4,n)==r4[n-1]);for(unsigned n=1;n<=8;++n)CHECK(rate(Kind::Black8,n)==b8[n-1]);for(unsigned n=1;n<=7;++n)CHECK(rate(Kind::Black7,n)==b7[n-1]);
 for(unsigned n:{0u,5u,9u,100u})CHECK(rate(Kind::Red4,n)==1.f);CHECK(rate(Kind::Black7,8)==1.f);CHECK(rate(Kind::Normal,1)==1.f);
 for(unsigned n=0;n<3;++n){CHECK(target(Kind::Red4,n)==t4[n]);CHECK(t4[n]<o4[n]);}
 for(unsigned n=0;n<7;++n){CHECK(target(Kind::Black8,n)==t8[n]);CHECK(t8[n]<o8[n]);}
 for(unsigned n=0;n<6;++n){CHECK(target(Kind::Black7,n)==t7[n]);CHECK(t7[n]<o7[n]);}
 CHECK(target(Kind::Red4,3)==0&&target(Kind::Black8,7)==0&&target(Kind::Black7,6)==0&&target(Kind::Normal,0)==0);
 // identities: Red central pair is the closest; Black8 five-hit burst uniform; Black7 closing cadence distinct
 CHECK(t4[1]<t4[0]&&t4[1]<t4[2]);for(unsigned n=3;n<6;++n)CHECK(t8[n]==t8[2]);CHECK(t8[0]>t8[2]+16&&t8[6]>t8[2]+16);
 CHECK(t7[0]<t7[3]&&t7[3]<t7[1]&&t7[4]==t7[3]&&t7[5]==t7[3]&&t7[3]!=t8[2]);
 for(auto k:{Kind::Red4,Kind::Black8,Kind::Black7})for(unsigned n=1;n<=count(k);++n){
  const float r=rate(k,n);CHECK(r>=1.05f&&r<=kRedMaxRate);if(k!=Kind::Red4){CHECK(r<=kBlackMaxRate);CHECK(r<post08::policy::kFuryRateThreshold);}
  float first,last;const unsigned a=armed(r,first,last);CHECK(a>=6&&a<=8);CHECK(first<=15.86f&&last>=22.59f);
  const unsigned d=at_delay(k,n);CHECK(d==(r==1.05f?14u:r==1.20f?12u:11u));
 }
 // Blue CP04: opening pair faster, approved fast closing pair unchanged
 cp04::Combo s;s.swing=1;CHECK(s.wanted_rate()==1.10f);s.swing=2;CHECK(s.wanted_rate()==1.10f);s.swing=3;CHECK(s.wanted_rate()==1.5f);s.swing=4;CHECK(s.wanted_rate()==1.5f);
 CHECK(cp04::normal_rate(1.f)&&cp04::normal_rate(1.15f)&&cp04::normal_rate(1.30f)&&!cp04::normal_rate(1.10f)&&!cp04::normal_rate(1.5f));
 cp04::Counters c;s=cp04::Combo{};s.phase=cp04::Phase::Swing;s.swing=1;s.finish(c);CHECK(s.gap==8);s.phase=cp04::Phase::Swing;s.swing=2;s.finish(c);CHECK(s.gap==8);s.phase=cp04::Phase::Swing;s.swing=3;s.finish(c);CHECK(s.gap==2);
 float first,last;CHECK(armed(1.10f,first,last)>=8);
}
int main(){fury_truth_table();chip_cases();chip_fuzz();late_input();rhythm();
 std::printf("CP06_LAB checks=%llu failures=0 RESULT=LAB_PASS native_runtime=0 visual=HUMAN_RUNTIME_PENDING\n",checks);}
