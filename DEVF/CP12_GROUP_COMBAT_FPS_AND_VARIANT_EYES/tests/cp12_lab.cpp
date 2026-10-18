// CP12 offline lab: variant-matched eyes (Black red, Blue cyan, Red none) and the PERF_WINDOW instrument. Real policy headers, no game, no GPU.
//  * colour rules on synthetic CMPR atlases (compressed domain: endpoints only, indices and block mode kept);
//  * step_eyes == the eye block of devf_vis::step (exact equivalence on random sequences), then the Blue lifecycle incl. the restore bug;
//  * PERF_WINDOW percentiles / thresholds / pause exclusion / clock calibration.
#include "../expansion/variant_eyes.hpp"
#include "../expansion/perf_window.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
namespace ey=devf_eyes12;namespace vv=devf_vis;using devf_variant::Variant;using devf_tex::RGB;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
// A 128x128 CMPR atlas (2048 blocks... 1024 8-byte blocks of 4x4 texels = 8 KB) filled with a background colour, with an "eye" patch of blocks.
std::vector<std::uint8_t> atlas(RGB bg0,RGB bg1,RGB eye0,RGB eye1,unsigned eye_blocks){
 std::vector<std::uint8_t> a(devf_tex::cmpr_level_bytes(128,128));
 for(std::size_t o=0,i=0;o<a.size();o+=8,++i){
  const bool eye=i<eye_blocks;RGB c0=eye?eye0:bg0,c1=eye?eye1:bg1;
  std::uint16_t p0=devf_tex::pack565(c0),p1=devf_tex::pack565(c1);if(p0<=p1){if(p0==p1)p0=std::uint16_t(p1+1);else{const auto t=p0;p0=p1;p1=t;}}   // 4-colour mode
  devf_tex::wr16(a.data()+o,p0);devf_tex::wr16(a.data()+o+2,p1);devf_tex::wr32(a.data()+o+4,0x1B1B1B1Bu);
 }
 return a;
}
int luminance(RGB c){return (299*c.r+587*c.g+114*c.b)/1000;}
struct FakeEye {unsigned tex=0;bool foreign=false;unsigned writes=0,fail_at=99;std::vector<unsigned> seq;};
struct Fx {FakeEye& e;const char* last="NONE";
 bool eye(vv::Entry&,unsigned idx){if(e.foreign){last="EYE_PACKET_TEXTURE_NOT_OURS";return false;}if(idx==e.fail_at){last="INJECTED";return false;}e.tex=idx;++e.writes;e.seq.push_back(idx);return true;}
 bool aura(vv::Entry&,int,int){return true;}void aura_release(vv::Entry&){}const char* reason()const{return last;}};
int main(){
 const RGB green0{70,190,60},green1{30,120,40},skin0{120,90,70},skin1{80,60,45},black{4,4,4},white{250,250,250};
 // ---------- colour rules ----------
 {auto a=atlas(skin0,skin1,green0,green1,40);const auto c=ey::choose(ey::Scheme::BlackRed,a.data(),a.size());require(c.params==&ey::kRedEyes&&c.endpoints==80,"Black: green eye endpoints found (40 blocks x 2)");
  auto r=a;devf_eye::Stats st;require(devf_eye::recolor_cmpr(r.data(),r.size(),1000,ey::kRedEyes,st)&&st.endpoints_changed==80,"only the 80 eye endpoints change");
  for(std::size_t o=0,i=0;o<r.size();o+=8,++i){const std::uint16_t n0=devf_tex::rd16(r.data()+o),n1=devf_tex::rd16(r.data()+o+2);
   require(n0>n1,"4-colour block mode preserved");
   if(i<40){const RGB e0=devf_tex::expand565(n0);require(e0.r>=200&&e0.g<=60&&e0.b<=60&&e0.r>=4*e0.g&&e0.g+8>=e0.b,"eye endpoint becomes intense red, not pink (b<=g), not orange (g small)");
    require(std::abs(luminance(e0)-luminance(devf_tex::expand565(devf_tex::pack565(green0))))<=40||e0.r==255,"luminance kept unless clipped at full red");}
   else require(n0==devf_tex::rd16(a.data()+o)&&n1==devf_tex::rd16(a.data()+o+2)&&devf_tex::rd32(r.data()+o+4)==devf_tex::rd32(a.data()+o+4),"skin blocks byte-identical");}
  auto k=atlas(black,white,black,white,40);devf_eye::Stats s2;devf_eye::recolor_cmpr(k.data(),k.size(),1000,ey::kRedEyes,s2);require(s2.endpoints_changed==0,"neutral pupil / highlight endpoints never tinted");
  pass("black_eyes_red_on_green_endpoints_only_mode_and_neutrals_kept");}
 {auto a=atlas(skin0,skin1,green0,green1,40);auto c=ey::choose(ey::Scheme::BlueCyan,a.data(),a.size());require(c.params==&ey::kCyanEyesGreen&&!c.fallback,"Blue: green eyes use the proven green rule");
  auto r=a;devf_eye::Stats st;devf_eye::recolor_cmpr(r.data(),r.size(),1000,*c.params,st);const RGB e0=devf_tex::expand565(devf_tex::rd16(r.data()));
  require(e0.b>e0.r&&e0.g>e0.r&&e0.b>=e0.g,"Blue eye endpoint becomes blue/cyan like the aura (80,190,255)");
  const RGB yellow0{230,200,40},yellow1{170,140,20};auto y=atlas(skin0,skin1,yellow0,yellow1,40);c=ey::choose(ey::Scheme::BlueCyan,y.data(),y.size());
  require(c.params==&ey::kCyanEyesAnyHue&&c.fallback,"non-green (yellow) eyes: the any-chromatic fallback is chosen");
  auto ry=y;devf_eye::Stats sy;devf_eye::recolor_cmpr(ry.data(),ry.size(),1000,*c.params,sy);const RGB ye=devf_tex::expand565(devf_tex::rd16(ry.data()));require(ye.b>ye.r&&ye.g>ye.r,"yellow eyes also become cyan");
  auto n=atlas(black,white,black,white,1024);c=ey::choose(ey::Scheme::BlueCyan,n.data(),n.size());require(!c.params&&std::string(c.why)=="NO_CHROMATIC_EYE_ENDPOINTS","an all-neutral atlas is refused (no fake success)");
  c=ey::choose(ey::Scheme::None,a.data(),a.size());require(!c.params,"Red has no scheme");
  require(ey::scheme_for(Variant::Red)==ey::Scheme::None&&ey::scheme_for(Variant::None)==ey::Scheme::None,"Red / unclassified: no eye colour, ever");
  auto few=atlas(skin0,skin1,green0,green1,3);require(!ey::choose(ey::Scheme::BlackRed,few.data(),few.size()).params,"fewer than kMinEyeEndpoints: refused");
  pass("blue_eyes_cyan_with_adaptive_detection_and_honest_refusal");}
 {require(ey::kRedEyes.tint_r==devf_stk::kBlackColour.r&&ey::kRedEyes.tint_g==devf_stk::kBlackColour.g&&ey::kRedEyes.tint_b==devf_stk::kBlackColour.b,"Black eyes share the aura's red");
  require(ey::kCyanEyesGreen.tint_r==devf_stk::kBlueColour.r&&ey::kCyanEyesGreen.tint_g==devf_stk::kBlueColour.g&&ey::kCyanEyesGreen.tint_b==devf_stk::kBlueColour.b&&ey::kCyanEyesAnyHue.tint_b==devf_stk::kBlueColour.b,"Blue eyes share the aura's blue/cyan");
  pass("eye_tints_are_the_aura_colours");}
 // ---------- step_eyes == the Black eye block of devf_vis::step ----------
 {std::uint32_t seed=0x9E3779B9u;auto rnd=[&]{seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
  for(unsigned run=0;run<3000;++run){
   vv::Ledger L;int dummy=0;vv::Entry* a=L.record(1,&dummy,1,Variant::Black);vv::Entry b=*a;
   FakeEye ea,eb;const unsigned fail=rnd()%40==0?1+rnd()%7:99;ea.fail_at=eb.fail_at=fail;Fx fa{ea},fb{eb};
   for(unsigned t=0;t<120;++t){
    const bool want=(rnd()%5)!=0?((t/17)%2==0):(rnd()&1);
    const auto oa=vv::step(L,*a,fa,false,want);const auto ob=ey::step_eyes(b,fb,want);
    require(oa.n==ob.n,"same number of events");
    for(unsigned i=0;i<oa.n;++i)require(oa.ev[i].kind==ob.ev[i].kind&&oa.ev[i].eye_idx==ob.ev[i].eye_idx&&oa.ev[i].level==ob.ev[i].level,"same events");
    require(a->eye_level==b.eye_level&&a->eye_idx==b.eye_idx&&a->eye_fault==b.eye_fault&&a->eye_target==b.eye_target,"same eye state");
   }
   require(ea.seq==eb.seq,"same texture write sequence");
  }
  pass("step_eyes_is_exactly_the_cp10_6a_black_eye_machine");}
 // ---------- Blue lifecycle ----------
 {vv::Ledger L;int d=0;vv::Entry* e=L.record(5,&d,1,Variant::Blue);FakeEye fe;Fx fx{fe};
  require(!ey::eyes_via_policy_step(Variant::Blue)&&ey::eyes_via_step_eyes(Variant::Blue)&&ey::eyes_via_policy_step(Variant::Black)&&!ey::eyes_via_step_eyes(Variant::Black)&&!ey::eyes_via_step_eyes(Variant::Red)&&!ey::eyes_via_policy_step(Variant::Red),"routing: Black via devf_vis::step, Blue via step_eyes, Red never");
  for(int t=0;t<5;++t)ey::step_eyes(*e,fx,false);require(fe.writes==0&&e->eye_idx==0,"Blue alone: native eyes, nothing written");
  unsigned on=0;for(int t=0;t<12;++t){auto o=ey::step_eyes(*e,fx,true);for(unsigned i=0;i<o.n;++i)on+=o.ev[i].kind==vv::Ev::EyesOn;}
  require(e->eye_idx==7&&fe.tex==7&&on==1&&fe.writes==7,"group ON: 7 progressive variants in 10 ticks, one EYES_ON");
  unsigned ticks=0,off=0;while(e->eye_level>0&&ticks<40){auto o=ey::step_eyes(*e,fx,false);for(unsigned i=0;i<o.n;++i)off+=o.ev[i].kind==vv::Ev::EyesOff;++ticks;}
  require(ticks==15&&e->eye_idx==0&&fe.tex==0&&off==1,"group OFF: back to the NATIVE texture in 15 ticks (the 'never restored' bug cannot happen)");
  for(int t=0;t<12;++t)ey::step_eyes(*e,fx,true);for(int t=0;t<6;++t)ey::step_eyes(*e,fx,false);const unsigned mid=e->eye_idx;require(mid>0&&mid<7,"mid fade-out");
  for(int t=0;t<3;++t)ey::step_eyes(*e,fx,true);require(e->eye_idx>=mid&&!e->eye_fault,"re-activation during the fade resumes from the current level");
  vv::cleanup(*e,fx,true);require(fe.tex==0&&e->eye_idx==0,"death / room change / unload with eyes on: native restored (actor still live)");
  vv::Entry g=*e;g.eye_idx=5;FakeEye gone;Fx gx{gone};vv::cleanup(g,gx,false);require(gone.writes==0,"actor already destroyed: nothing touched");
  pass("blue_eyes_follow_the_group_bonus_and_always_return_native");}
 {vv::Ledger L;int d=0;vv::Entry* e=L.record(6,&d,1,Variant::Blue);FakeEye fe;fe.foreign=true;Fx fx{fe};
  for(int t=0;t<12;++t)ey::step_eyes(*e,fx,true);require(e->eye_fault&&fe.writes==0&&e->eye_idx==0,"another mod owns the eye packet: fault, nothing written");
  for(int t=0;t<30;++t)ey::step_eyes(*e,fx,true);require(fe.writes==0,"and no retry spam");
  vv::Entry* f=L.record(7,&d,1,Variant::Blue);FakeEye fe2;fe2.fail_at=4;Fx fx2{fe2};for(int t=0;t<12;++t)ey::step_eyes(*f,fx2,true);
  require(f->eye_fault&&fe2.tex==0&&f->eye_idx==0,"a write failure mid-fade returns to native (best effort) and stops");
  std::array<ey::EyeOwner,4> owners{};auto& o=ey::owner_for(owners,1,7,1);o.bound=true;o.set=2;
  require(ey::owner_for(owners,1,7,1).bound&&!ey::owner_for(owners,1,7,2).bound&&!ey::owner_for(owners,1,8,2).bound,"eye binding stamped by id+generation: never inherited");
  pass("blue_eye_ownership_faults_are_safe_and_never_inherited");}
 // ---------- PERF_WINDOW ----------
 {devf_pw::Window w;for(unsigned i=0;i<1000;++i)w.add_interval(33300);require(w.n==1000&&w.percentile_us(500)==33300&&w.percentile_us(990)==33300&&w.over40==0,"steady 33.3 ms: p50 = p99 = 33.3 ms (bucket edge capped at the observed max)");
  devf_pw::Window h;for(unsigned i=0;i<980;++i)h.add_interval(33300);for(unsigned i=0;i<20;++i)h.add_interval(70000);
  require(h.percentile_us(950)==33500&&h.percentile_us(990)==70000&&h.max_us==70000&&h.over40==20&&h.over50==20&&h.over67==20,"2 % hitches of 70 ms: p95 steady, p99 shows them, counters exact");
  devf_pw::Window q;for(unsigned i=0;i<99;++i)q.add_interval(33300);q.add_interval(70000);q.add_interval(70000);require(q.percentile_us(990)==70000,"nearest-rank: p99 of 101 samples is the 100th (a hitch), not the 99th");
  devf_pw::Window p;p.add_interval(2500000);p.add_interval(33300);require(p.paused==1&&p.n==1&&p.max_us==33300,"a 2.5 s load/pause is reported apart, not as a hitch");
  devf_pw::Window c;c.add_interval(3000);c.add_interval(500000);require(c.catchup==1&&c.percentile_us(1000)==500000,"catch-up and overflow bucket");
  for(unsigned i=0;i<299;++i)c.end_tick(2,5);require(!c.full(),"window still open at 299 ticks");c.end_tick(3,6);require(c.full()&&c.aura_max==3&&c.tracked_max==6,"window closes after 300 ticks");
  devf_pw::Clock k;k.calibrate(1000,0);k.calibrate(1000+3000000000ull*10,100000000ull);require(k.hz==3000000000ull,"GHz counter calibrated over 10 s without overflow");
  require(k.us(3000000)==1000,"3e6 ticks at 3 GHz = 1 ms");
  devf_pw::Clock z;z.calibrate(5,0);z.calibrate(6,1000);require(z.hz==10000000ull,"too short a calibration span keeps the default 10 MHz");
  pass("perf_window_percentiles_thresholds_pause_and_clock_are_exact");}
 std::printf("CP12_LAB_PASS cases=%u EYES_POLICY_AND_PERF_INSTRUMENT_ONLY; PIXELS_AND_FPS_NOT_MEASURED; GAME_NOT_EXECUTED\n",cases);
}
