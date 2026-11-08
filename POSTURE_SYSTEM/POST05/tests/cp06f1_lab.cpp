// CP06 FIX01 0.11.12 value laboratory. Pure core only: geometry against the transcribed
// engine predicate, vanilla pose coverage per rate in float32 J3DFrameCtrl steps, write
// decision properties, swing classification, chip observation and the Expansion flash
// mirror DIFFERENTIALLY against Bokoblin Expansion 0.17.0's real cp19f2_policy.hpp.
// No game, no DLL, no render: pixels and runtime stay HUMAN_RUNTIME_PENDING.
#include "../src/cp06f1_core.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <initializer_list>
#include "reference/cp06f1_native_cross.inc"
#include "cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19f2_policy.hpp"
static unsigned long long checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"CP06F1_FAIL line=%d %s\n",__LINE__,#x);std::exit(1);}}while(false)
using namespace cp06f1;
struct Rng{std::uint64_t s=0x9e3779b97f4a7c15ull;std::uint32_t u(){s^=s<<13;s^=s>>7;s^=s<<17;return std::uint32_t(s>>11);}float f(float a,float b){return a+(b-a)*float(u()%1000003u)/1000003.0f;}bool b(unsigned pct){return u()%100u<pct;}};
static bool native(const Cyl& k,const Sph& s){float pen=0;return ref_cM3d_Cross_CylSph(&k,&s,&pen);}
// ------------------------------------------------------------------ geometry
void geometry(){
 Rng r;unsigned strict_true=0,native_true=0,zero_diff=0;
 for(unsigned n=0;n<2000000;++n){
  Cyl k;k.c={r.f(-300,300),r.f(-100,100),r.f(-300,300)};k.r=r.f(20,50);k.h=r.f(40,200);k.on=true;
  Sph s;s.c={k.c.x+r.f(-160,160),k.c.y+r.f(-120,260),k.c.z+r.f(-160,160)};s.r=n%3==0?kTipR:n%3==1?kHiltR:r.f(5,60);
  const bool nat=native(k,s),st=cross(k,s,kWriteSlack),lo=cross(k,s,kKeepSlack),z=cross(k,s,0.0f);
  CHECK(!st||nat);     // a WRITTEN sphere always crosses natively
  CHECK(!nat||lo);     // a native crossing is always KEPT
  if(z!=nat)++zero_diff;strict_true+=st;native_true+=nat;
 }
 CHECK(zero_diff<=40);CHECK(strict_true>100000&&native_true>strict_true);   // non-vacuous, last-ulp disagreements only
 // exact boundaries: tangent in XZ and in Y
 Cyl k;k.c={0,0,0};k.r=35;k.h=180;k.on=true;Sph s;s.r=45;
 s.c={80,90,0};CHECK(native(k,s)&&cross(k,s,0)&&!cross(k,s,kWriteSlack)&&cross(k,s,kKeepSlack));
 s.c={80.4f,90,0};CHECK(!native(k,s)&&!cross(k,s,0)&&cross(k,s,kKeepSlack));s.c={80.6f,90,0};CHECK(!native(k,s)&&!cross(k,s,kKeepSlack));
 s.c={0,-45,0};CHECK(native(k,s)&&!cross(k,s,kWriteSlack));s.c={0,-45.4f,0};CHECK(!native(k,s)&&cross(k,s,kKeepSlack));
 s.c={0,225,0};CHECK(native(k,s));s.c={0,225.4f,0};CHECK(!native(k,s));
 s.r=-1;s.c={0,90,0};CHECK(cross(k,s,0)==native(k,s));   // degenerate radius: same answer as the engine
 const float nan=std::nanf("");s.r=45;s.c={nan,90,0};CHECK(!cross(k,s,kWriteSlack)&&!cross(k,s,kKeepSlack)&&!cross(k,s,0));
 Cyl ks[3]={k,k,k};ks[0].on=ks[1].on=ks[2].on=false;s.c={0,90,0};CHECK(!touches(ks,s,kWriteSlack,true)&&touches(ks,s,kWriteSlack,false));
 ks[2].on=true;CHECK(touches(ks,s,kWriteSlack,true));CHECK(any_on(ks));
}
// ---------------------------------------------- vanilla pose coverage per rate
void frames(){
 const float rates[]={1.0f,1.05f,1.10f,1.15f,1.20f,1.30f,1.35f,1.5f,2.0f};
 const float phases[]={0.0f,0.25f,0.5f,0.75f,1.0f,1.15f,1.3f,1.35f,1.5f,2.0f,0.001f,0.999f,0.6f,1.7f,1.99f,0.37f};
 unsigned probes_total=0,multi=0,three=0,cases=0,exact_root=0,refused_branch=0;Rng rr;
 for(float rate:rates)for(float ph:phases)for(float end:{42.0f,47.0f})for(int refuse_pct:{0,35}){
  float frame=ph,rt=rate;float cov[128];unsigned n=0;bool any_at=false,prev_armed=false,prev_covered=false;
  for(int t=0;t<80&&rt>0;++t){
   const float pre=frame;const bool at=ref_at_on(pre);ref_frame_update(frame,rt,0.0f,end);const float post=frame;
   float fr[kMaxProbes]={-1,-1,-1};const unsigned np=at?probe_frames(pre,post,prev_armed,prev_covered,fr):0;
   if(rate<=1.0f)CHECK(np==0);
   if(at&&prev_armed&&!prev_covered)++refused_branch;
   if(at){any_at=true;cov[n++]=post;
    for(unsigned i=0;i<np;++i){++probes_total;CHECK(fr[i]>=kPoseFirst&&fr[i]<=kPoseLast);
     CHECK(fr[i]<post||(fr[i]==kPoseLast&&post>kAtLast&&post<kPoseLast));
     if(i)CHECK(fr[i]>fr[i-1]);cov[n++]=fr[i];
     if(fr[i]==pre+1.0f||fr[i]==kPoseLast){++exact_root;for(bool bb:{true,false}){const V3 o=lunge_offset(bb,pre,fr[i],0.6f,0.8f);CHECK(o.x==0.0f&&o.z==0.0f);}}
     {const V3 o=lunge_offset(true,pre,fr[i],0.0f,1.0f);CHECK(o.x==0.0f&&o.z==lunge(true,fr[i]-1.0f)-lunge(true,pre)&&std::fabs(o.z)<=49.0f);}}
    if(np>=2)++multi;if(np==3)++three;
   }
   prev_armed=at;prev_covered=at&&!rr.b(unsigned(refuse_pct));   // adapter: covered unless the evaluation was refused
  }
  CHECK(any_at);++cases;
  // never coarser than vanilla x1 inside the vanilla window: every pose of [15,23] lies within half a frame
  if(rate>1.0f)for(float v=15.0f;v<=23.0f;v+=0.01f){float best=1e9f;for(unsigned i=0;i<n;++i){const float d=std::fabs(cov[i]-v);if(d<best)best=d;}CHECK(best<=0.5f+2e-3f);}
 }
 CHECK(cases==9*16*2*2&&probes_total>2000&&multi>100&&three>4&&exact_root>1000&&refused_branch>100);
 float f[kMaxProbes];
 CHECK(probe_frames(14,15,false,false,f)==0);CHECK(probe_frames(14,16.01f,false,false,f)==0);CHECK(probe_frames(13.9f,15.9f,false,false,f)==0);CHECK(probe_frames(22.1f,24.1f,true,true,f)==0);CHECK(probe_frames(std::nanf(""),16,true,true,f)==0);CHECK(probe_frames(16,14,true,true,f)==0);
 CHECK(probe_frames(16,18,true,true,f)==1&&f[0]==17.0f);CHECK(probe_frames(22,24,true,true,f)==1&&f[0]==23.0f);
 CHECK(probe_frames(21.0f,22.05f,true,true,f)==2&&f[0]==22.0f&&f[1]==23.0f);CHECK(probe_frames(21.5f,22.55f,true,true,f)==1&&f[0]==22.5f);CHECK(probe_frames(22.0f,23.05f,true,true,f)==1&&f[0]==23.0f);
 CHECK(probe_frames(14,16,false,false,f)==1&&f[0]==15.0f);   // x2 even phase first tick: exactly the vanilla pose 15
 CHECK(probe_frames(15.7f,17.7f,false,false,f)==3);CHECK(probe_frames(16,18,true,false,f)==2);   // armed but undecided: [pre,post)
 for(float p=21.0f;p<=22.0f;p+=0.01f)for(bool bb:{true,false}){const V3 o=lunge_offset(bb,p,23.0f,1.0f,0.0f);CHECK(o.x==0.0f);}
 // s16 facing exactly like executeAttack's (cM_ssin,cM_scos)(shape_angle.y), every angle
 for(int a=-32768;a<=32767;++a){float sn,cs;sincos16(std::int16_t(a),sn,cs);const double r=double(a)*3.14159265358979323846/32768.0;CHECK(std::fabs(sn-std::sin(r))<1e-6&&std::fabs(cs-std::cos(r))<1e-6);}
 for(float p=0;p<30;p+=0.05f)for(bool b:{true,false})CHECK(lunge(b,p)==ref_lunge(b,p));
}
// ------------------------------------------------------------ write decision
void decision(){
 Rng r;unsigned writes=0,already=0,invul=0,nomid=0,partial=0,from_current=0,from_probe=0,ambiguous_replaced=0;
 for(unsigned n=0;n<400000;++n){
  Cyl k[3];const V3 feet{r.f(-50,50),r.f(-20,20),r.f(-50,50)};
  k[0].c=feet;k[0].r=35;k[0].h=r.f(60,180);k[1].c={feet.x+r.f(-8,8),feet.y+r.f(40,70),feet.z+r.f(-8,8)};k[1].r=35;k[1].h=90;k[2].c={feet.x+r.f(-10,10),feet.y+r.f(70,100),feet.z+r.f(-10,10)};k[2].r=35;k[2].h=90;
  const bool on=!r.b(15);for(auto& c:k)c.on=on;
  Sph cur[2];Cand c[1+kMaxProbes];const unsigned nc=1+r.u()%(kMaxProbes+1);
  for(int j=0;j<2;++j){cur[j].c={r.f(-200,200),r.f(-40,240),r.f(-200,200)};cur[j].r=j?kHiltR:(r.b(20)?r.f(33,45):kTipR);
   for(unsigned i=0;i<nc;++i){c[i].s[j].c={cur[j].c.x+r.f(-120,120),cur[j].c.y+r.f(-60,60),cur[j].c.z+r.f(-120,120)};c[i].s[j].r=j?kHiltR:kTipR;}}
  if(r.b(30))for(int j=0;j<2;++j)c[0].s[j]=cur[j];   // native current == registered (no foreign sweep)
  const Decision d=decide(k,cur,c,nc);
  const bool keep=touches(k,cur[0],kWriteSlack,true)||touches(k,cur[1],kWriteSlack,true);
  if(keep){CHECK(d.why==Why::AlreadyTouching&&!d.write[0]&&!d.write[1]);++already;continue;}
  for(int j=0;j<2;++j){
   if(d.write[j]){++writes;CHECK(on&&d.from[j]<nc);const Cand& w=c[d.from[j]];CHECK(d.place[j].r==w.s[j].r&&same(d.place[j].c,w.s[j].c));
    if(d.from[j]==0)++from_current;else ++from_probe;if(touches(k,cur[j],0.0f,true))++ambiguous_replaced;
    for(unsigned i=0;i<d.from[j];++i)CHECK(!touches(k,c[i].s[j],kWriteSlack,true));   // first reaching candidate in order
    bool reach=false;for(auto& q:k)reach=reach||(q.on&&native(q,d.place[j]));CHECK(reach);   // written pose really crosses natively
   }else for(unsigned i=0;i<nc;++i)CHECK(!touches(k,c[i].s[j],kWriteSlack,true));
  }
  if(d.write[0]!=d.write[1])++partial;
  if(d.why==Why::LinkInvulnerable){++invul;CHECK(!on);}
  if(d.why==Why::NoMidContact){++nomid;CHECK(!d.write[0]&&!d.write[1]);}
  CHECK((d.why==Why::Written)==(d.write[0]||d.write[1]));
  // after the decision, if any candidate crossed an ON cylinder the registered set crosses with margin
  bool cand_cross=false;for(unsigned i=0;i<nc;++i)for(int j=0;j<2;++j)cand_cross=cand_cross||touches(k,c[i].s[j],kWriteSlack,true);
  if(cand_cross){Sph fin[2]={d.write[0]?d.place[0]:cur[0],d.write[1]?d.place[1]:cur[1]};CHECK(touches(k,fin[0],kWriteSlack,true)||touches(k,fin[1],kWriteSlack,true));}
 }
 CHECK(writes>5000&&already>5000&&invul>500&&nomid>50000&&partial>500&&from_current>1000&&from_probe>1000&&ambiguous_replaced>10);
}
// --------------------------------------------------------------- swings
void swings(){
 auto open=[](Kind k){Swing s;s.open=true;s.kind=k;return s;};
 Tick armed;armed.armed=true;armed.link_current=true;armed.link_any_on=true;armed.rate=2.0f;armed.pre=14;armed.post=16;
 {Swing s=open(Kind::BlackFury);Tick t=armed;t.expect=true;feed_tick(s,t);PassView pv;pv.registered=true;pv.link=true;pv.shield=true;feed_pass(s,pv,7);CHECK(classify(s)==Result::ValidShieldBlock&&s.contact_tick==7);}
 {Swing s=open(Kind::BlackFury);Tick t=armed;t.expect=true;t.guard_attack=true;feed_tick(s,t);PassView pv;pv.registered=true;pv.link=true;pv.shield=true;feed_pass(s,pv,7);CHECK(classify(s)==Result::ValidParry);}
 {Swing s=open(Kind::Red);Tick t=armed;t.expect=true;feed_tick(s,t);CHECK(s.kind==Kind::RedSpecialX2);PassView pv;pv.registered=true;pv.link=true;feed_pass(s,pv,3);CHECK(classify(s)==Result::ValidHitBody);}
 {Swing s=open(Kind::Red);Tick t=armed;t.rate=1.35f;feed_tick(s,t);CHECK(s.kind==Kind::Red);}
 {Swing s=open(Kind::Blue);Tick t=armed;feed_tick(s,t);PassView pv;pv.registered=true;feed_pass(s,pv,3);CHECK(classify(s)==Result::LegitimateMiss);}
 {Swing s=open(Kind::Black);Tick t=armed;t.link_any_on=false;t.geom_touch=true;feed_tick(s,t);CHECK(classify(s)==Result::LegitimateInvulnerability);}
 // expected contact, nothing recorded, no crossing in the pass before: CONTACT LOST (an anomaly line)
 {Swing s=open(Kind::Black);Tick t=armed;t.expect=true;feed_tick(s,t);PassView pv;pv.registered=true;feed_pass(s,pv,3);CHECK(classify(s)==Result::ContactLostConfirmed&&s.lost_ticks==1);}
 // ... but with a crossing in the previous pass the engine's TgOldApid rule explains it
 {Swing s=open(Kind::Black);Tick t=armed;t.expect=true;t.link_any_on=true;feed_tick(s,t);PassView hit;hit.registered=true;hit.link=true;hit.shield=true;feed_pass(s,hit,3);feed_tick(s,t);PassView none;none.registered=true;feed_pass(s,none,4);CHECK(s.nocon_ticks==1&&s.lost_ticks==0&&classify(s)==Result::ValidShieldBlock);}
 {Swing s=open(Kind::Black);Tick t=armed;t.expect=true;feed_tick(s,t);PassView none;none.registered=true;feed_pass(s,none,3);feed_tick(s,t);feed_pass(s,none,4);CHECK(s.lost_ticks==1&&s.nocon_ticks==1);}
 // a Link not registered this tick cannot be an expected contact
 {Swing s=open(Kind::Black);Tick t=armed;t.expect=true;t.link_current=false;feed_tick(s,t);PassView none;none.registered=true;feed_pass(s,none,3);CHECK(classify(s)==Result::LegitimateMiss);}
 {Swing s=open(Kind::Black);Tick t=armed;t.probe_refused=true;feed_tick(s,t);CHECK(classify(s)==Result::GeometryUndetermined);}
 {Swing s=open(Kind::Black);PassView pv;pv.registered=false;pv.link=true;feed_pass(s,pv,3);CHECK(classify(s)==Result::CleanupStale);}
 {Swing s=open(Kind::Black);Tick t=armed;feed_tick(s,t);PassView pv;pv.registered=true;pv.other=true;feed_pass(s,pv,3);CHECK(classify(s)==Result::NativeBlockedOther);}
 {Swing s=open(Kind::Black);Tick t=armed;t.written=1;t.expect=true;feed_tick(s,t);CHECK(s.backfill_ticks==1&&s.backfill_slots==1);PassView pv;pv.registered=true;pv.link=true;feed_pass(s,pv,3);CHECK(s.backfilled_contact&&classify(s)==Result::ValidHitBody);}
 // first contact wins; later contacts of the same swing never change it
 {Swing s=open(Kind::Black);Tick t=armed;t.expect=true;feed_tick(s,t);PassView a;a.registered=true;a.link=true;feed_pass(s,a,3);feed_tick(s,t);PassView b2;b2.registered=true;b2.link=true;b2.shield=true;feed_pass(s,b2,4);CHECK(!s.shield&&s.contact_tick==3);}
 Swing closed;Tick t=armed;feed_tick(closed,t);CHECK(closed.at_ticks==0);
 for(unsigned i=0;i<unsigned(Result::Count);++i)CHECK(name(Result(i))[0]!=0);for(unsigned i=0;i<unsigned(Why::Count);++i)CHECK(name(Why(i))[0]!=0);
}
// ------------------------------------------------ chip observation truth table
void chips(){
 CHECK(observe(0,0,5,6)==ChipSeen::ParryNeutralized);CHECK(observe(-2,-2,5,6)==ChipSeen::ParryNeutralized);
 CHECK(observe(0,-1,5,5)==ChipSeen::Legit);CHECK(observe(-3,-4,0,0)==ChipSeen::Legit);
 CHECK(observe(0,0,5,5)==ChipSeen::None);
 CHECK(observe(0,-2,5,5)==ChipSeen::Foreign);CHECK(observe(0,1,5,5)==ChipSeen::Foreign);CHECK(observe(0,-1,5,6)==ChipSeen::Foreign);CHECK(observe(0,0,5,7)==ChipSeen::Foreign);CHECK(observe(0,-0.5f,5,5)==ChipSeen::Foreign);
}
// ----------------------------- flash mirror vs Expansion's REAL schedule header
Rgb rgb(const devf_cp19f2::Rgb& c){return Rgb{c.r,c.g,c.b};}
void flash_schedule(){
 // schedule and colour, every state, against the real header
 devf_cp19f2::Flash ex;Flash ours;devf_cp19f2::flash_start(ex);start(ours);
 for(int i=0;i<40;++i){CHECK(ex.dmg==ours.dmg&&ex.col==ours.col);CHECK(rgb(devf_cp19f2::flash_color(ex))==color(ours));CHECK(devf_cp19f2::flash_on(ex)==on(ours));devf_cp19f2::flash_tick(ex);tick(ours);}
 for(int n=0;n<256;++n)for(int m2=0;m2<256;m2+=7){devf_cp19f2::Rgb a;a.r=std::uint8_t(n);a.g=std::uint8_t(m2);a.b=std::uint8_t(255-m2);devf_cp19f2::Rgb b;b.r=std::uint8_t(m2);b.g=std::uint8_t(n);b.b=0;CHECK(rgb(devf_cp19f2::flash_merge(a,b))==merge(rgb(a),rgb(b)));}
}
struct World{devf_cp19f2::Flash shown,legit;FlashMirror mirror;};
void flash_differential(){
 Rng r;unsigned removed=0,kept=0,native_kept=0,legit_only_untouched=0,draws=0;
 for(unsigned seq=0;seq<4000;++seq){
  World w;const unsigned mode=seq%4;   // 0 mixed, 1 parry-only, 2 legit-only, 3 bursts of two Blacks
  for(unsigned pass=0;pass<160;++pass){
   // chips charged during this tick's executes (0..2), each judged by Expansion and observed by us
   const unsigned nchips=mode==3?r.u()%3:(r.b(8)?1u:0u);
   for(unsigned c=0;c<nchips;++c){const bool parry=mode==1?true:mode==2?false:r.b(50);
    devf_cp19f2::flash_start(w.shown);if(!parry)devf_cp19f2::flash_start(w.legit);
    w.mirror.chip(parry?ChipSeen::ParryNeutralized:ChipSeen::Legit);}
   // draws of this tick: body/face/hat/hand with random native register (native damage red sometimes)
   for(unsigned d=0;d<4;++d){
    Rgb orig{std::uint8_t(r.b(20)?r.u()%40:0),std::uint8_t(r.b(5)?r.u()%30:0),0};
    const Rgb now=devf_cp19f2::flash_on(w.shown)?merge(orig,rgb(devf_cp19f2::flash_color(w.shown))):orig;
    const Rgb want=devf_cp19f2::flash_on(w.legit)?merge(orig,rgb(devf_cp19f2::flash_color(w.legit))):orig;
    Rgb out{};const Draw dr=w.mirror.decide(orig,now,out);++draws;
    CHECK(dr!=Draw::Mismatch&&dr!=Draw::Uncertain);
    CHECK(out==want);                       // exactly what Expansion shows without the neutralized chips
    CHECK(out.r>=orig.r&&out.g==orig.g&&out.b==orig.b);   // native colour never lowered or tinted
    if(dr==Draw::Counterfactual){if(out==orig)++removed;else ++kept;CHECK(now!=out);}
    if(orig.r>0&&out.r>=orig.r)++native_kept;
    if(mode==2)CHECK(dr!=Draw::Counterfactual&&(++legit_only_untouched,true));
   }
   devf_cp19f2::flash_tick(w.shown);devf_cp19f2::flash_tick(w.legit);w.mirror.pass();
  }
 }
 CHECK(removed>1000&&kept>100&&native_kept>1000&&legit_only_untouched>1000&&draws==4000u*160u*4u);
}
void flash_safety(){
 // a misaligned mirror (one pass late) is detected on the first visible frame and never writes afterwards
 {devf_cp19f2::Flash shown;FlashMirror m;devf_cp19f2::flash_start(shown);m.chip(ChipSeen::ParryNeutralized);devf_cp19f2::flash_tick(shown);
  unsigned writes=0,mism=0;for(unsigned p=0;p<40;++p){devf_cp19f2::flash_tick(shown);const Rgb orig{};const Rgb now=devf_cp19f2::flash_on(shown)?merge(orig,rgb(devf_cp19f2::flash_color(shown))):orig;Rgb out{};const Draw d=m.decide(orig,now,out);
   if(d==Draw::Counterfactual)++writes;if(d==Draw::Mismatch)++mism;if(mism)CHECK(d!=Draw::Counterfactual);m.pass();}
  CHECK(mism>=1&&m.c.mismatch>=1);CHECK(!m.uncertain());}
 // a foreign writer between the two PRE callbacks is never overwritten
 {FlashMirror m;m.chip(ChipSeen::ParryNeutralized);m.pass();Rgb out{};const Rgb orig{0,0,0};CHECK(m.decide(orig,Rgb{0,40,0},out)==Draw::Mismatch&&out==(Rgb{0,40,0}));
  CHECK(m.uncertain());CHECK(m.decide(orig,merge(orig,color(m.expansion())),out)==Draw::Uncertain&&out==merge(orig,color(m.expansion())));}
 // a foreign pending-life change suspends suppression until both shadows expire
 {FlashMirror m;m.chip(ChipSeen::ParryNeutralized);m.chip(ChipSeen::Foreign);CHECK(m.uncertain()&&!m.suppressing());for(int i=0;i<30;++i)m.pass();CHECK(!m.uncertain());}
 // nothing on: never touches anything (foreign red, native damage red)
 {FlashMirror m;Rgb out{};CHECK(m.decide(Rgb{0,0,0},Rgb{12,0,0},out)==Draw::Untouched&&out==(Rgb{12,0,0}));}
 // Expansion hook absent: expected red but register unchanged => nothing written
 {FlashMirror m;m.chip(ChipSeen::ParryNeutralized);for(int i=0;i<3;++i)m.pass();Rgb out{};CHECK(color(m.expansion()).r>0);CHECK(m.decide(Rgb{0,0,0},Rgb{0,0,0},out)==Draw::NoMirroredWrite&&out==(Rgb{0,0,0}));}
 // the 30-pass window is not a suppression window: a legit chip at any pass restores the full legit flash
 for(int at=0;at<31;++at){FlashMirror m;devf_cp19f2::Flash shown,legit;m.chip(ChipSeen::ParryNeutralized);devf_cp19f2::flash_start(shown);
  for(int p=0;p<at;++p){m.pass();devf_cp19f2::flash_tick(shown);}
  m.chip(ChipSeen::Legit);devf_cp19f2::flash_start(shown);devf_cp19f2::flash_start(legit);
  for(int p=0;p<32;++p){const Rgb orig{};const Rgb now=devf_cp19f2::flash_on(shown)?merge(orig,rgb(devf_cp19f2::flash_color(shown))):orig;Rgb out{};m.decide(orig,now,out);CHECK(out==now);m.pass();devf_cp19f2::flash_tick(shown);devf_cp19f2::flash_tick(legit);}}
 // parry right after a legit chip keeps the legit remainder exactly, adds nothing
 {FlashMirror m;devf_cp19f2::Flash shown,legit;m.chip(ChipSeen::Legit);devf_cp19f2::flash_start(shown);devf_cp19f2::flash_start(legit);
  for(int p=0;p<5;++p){m.pass();devf_cp19f2::flash_tick(shown);devf_cp19f2::flash_tick(legit);}
  m.chip(ChipSeen::ParryNeutralized);devf_cp19f2::flash_start(shown);unsigned shown_red=0;
  for(int p=0;p<34;++p){const Rgb orig{};const Rgb now=devf_cp19f2::flash_on(shown)?merge(orig,rgb(devf_cp19f2::flash_color(shown))):orig;Rgb out{};m.decide(orig,now,out);
   CHECK(out==(devf_cp19f2::flash_on(legit)?rgb(devf_cp19f2::flash_color(legit)):Rgb{}));if(out.r)++shown_red;m.pass();devf_cp19f2::flash_tick(shown);devf_cp19f2::flash_tick(legit);}
  CHECK(shown_red>0);}
}
int main(){
 geometry();frames();decision();swings();chips();flash_schedule();flash_differential();flash_safety();
 std::printf("CP06F1_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PURE_CORE_TRANSCRIBED_ENGINE_PREDICATE_REAL_EXPANSION_SCHEDULE runtime=HUMAN_RUNTIME_PENDING\n",kVersion,checks);
 return 0;
}
