// CP11 offline lab: proves that each CPU optimisation of 0.8.9 is EXACT against the 0.8.8 behaviour, using the real policy headers.
//  1. lazy registration context (stage / layer computed only when a registration's record fields match) vs the eager 0.8.8 snapshot;
//  2. early-exit ledger lookup vs Ledger::find on the real devf_vis / devf_dyn ledgers;
//  3. telemetry budget rules (detail for the first 2 acquisitions per variant, anomalies always).
// Pure logic: no game, no GPU, no FPS. Timing evidence lives in tests/bench (local) and is reported separately.
#include "../expansion/perf_policy.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include "../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
namespace b2=devf_black2;
using devf_variant::Variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
struct Rng {std::uint32_t s;explicit Rng(std::uint32_t seed):s(seed){}std::uint32_t u(){s^=s<<13;s^=s>>17;s^=s<<5;return s;}unsigned below(unsigned n){return u()%n;}bool coin(){return u()&1u;}};
// The game context (stage name F_SP121? layer) for a test "world".
struct World {bool stage=true;int layer=6;unsigned context_calls=0;};
void eager(b2::Snapshot& s,World& w){++w.context_calls;s.stage=w.stage;s.layer=w.layer;}              // 0.8.8: always
void lazy(const b2::Table& a,const b2::Table& b,b2::Snapshot& s,World& w){devf_perf::complete_snapshot(a,b,s,[&](b2::Snapshot& c){++w.context_calls;c.stage=w.stage;c.layer=w.layer;});}
float pick_f(Rng& r){static const float v[]{-4321.5f,0.0f,1.0f,1529.25f,-90.125f};return v[r.below(5)];}
void make_tables(Rng& r,b2::Table& black,b2::Table& normal){
 black=b2::Table{};normal=b2::Table{};
 const unsigned nb=r.below(4),nn=r.below(4);char id[8]="B0";
 for(unsigned i=0;i<nb;++i){id[1]=char('0'+i);black.add(id,int(r.below(4)),int(r.below(8)),pick_f(r),pick_f(r)+float(i),pick_f(r),int(r.below(3))*0x4000,220+int(r.below(200)));}
 id[0]='N';for(unsigned i=0;i<nn;++i){id[1]=char('0'+i);normal.add(id,int(r.below(4)),int(r.below(8)),pick_f(r)+100.0f,pick_f(r)+float(i),pick_f(r),int(r.below(3))*0x4000,220);}
 for(unsigned i=0;i<black.count;++i){if(r.below(8)==0)black.regs[i].fault=true;if(r.below(10)==0)black.regs[i].used=false;}
 black.armed=r.below(4)!=0;
}
b2::Snapshot make_snapshot(Rng& r,const b2::Table& black,const b2::Table& normal){
 b2::Snapshot s;s.profile=510;s.parameters=b2::kParameters;s.set_id=0xFFFF;s.home_x=b2::kRangeCode;s.home_z=0;s.room=int(r.below(4));s.home_y=int(r.below(3))*0x4000;
 s.x=pick_f(r);s.y=pick_f(r);s.z=pick_f(r);
 const b2::Table* t=r.coin()?&black:&normal;
 if(t->count&&r.below(3)){const auto& g=t->regs[r.below(t->count)];s.room=g.room;s.x=g.x;s.y=g.y;s.z=g.z;s.home_y=g.yaw;}   // an exact record match, often
 switch(r.below(8)){case 0:s.profile=509;break;case 1:s.parameters=0;break;case 2:s.set_id=3;break;case 3:s.home_x=0;break;case 4:s.x+=1.0f;break;default:break;}
 s.id=1+r.below(50);s.health=r.coin()?220:200;s.initializing=r.coin();s.resource_eoc2=r.below(5)!=0;s.previous_live=r.below(6)==0;
 return s;
}
int main(){
 // ---------- 1. lazy context is exact ----------
 {Rng r(0xC0FFEEu);unsigned compared=0,skipped=0;
  for(unsigned it=0;it<200000;++it){
   b2::Table black,normal;make_tables(r,black,normal);
   World w;w.stage=r.below(5)!=0;w.layer=int(r.below(8));
   b2::Snapshot base=make_snapshot(r,black,normal);
   b2::Snapshot e=base,l=base;World we=w,wl=w;eager(e,we);lazy(black,normal,l,wl);
   if(wl.context_calls==0)++skipped;
   require(black.find(e)==black.find(l),"Table::find identical");
   for(unsigned i=0;i<black.count;++i)require(black.matches(i,e)==black.matches(i,l),"black Table::matches identical");
   for(unsigned i=0;i<normal.count;++i)require(normal.matches(i,e)==normal.matches(i,l),"normal Table::matches identical");
   const auto me=devf_variant::match(black,normal,e),ml=devf_variant::match(black,normal,l);require(me.black==ml.black&&me.normal==ml.normal,"devf_variant::match identical (classification)");
   const int idx=black.find(e);
   if(idx>=0){b2::Table te=black,tl=black;const auto de=te.apply(unsigned(idx),e),dl=tl.apply(unsigned(idx),l);
    require(de==dl&&te.regs[idx].fault==tl.regs[idx].fault&&te.regs[idx].admitted_id==tl.regs[idx].admitted_id&&te.regs[idx].last_id==tl.regs[idx].last_id&&te.regs[idx].generation==tl.regs[idx].generation,"Table::apply decision and state identical (HP_APPLIED / BLACK_REJECTED paths)");}
   ++compared;
  }
  require(compared==200000&&skipped>20000&&skipped<180000,"both branches exercised (context needed / not needed)");
  pass("lazy_registration_context_is_exact_against_the_eager_0_8_8_snapshot");}
 {// call counts: 8 Bokoblins on screen, one registered Black among them, 30 ticks
  b2::Table black,normal;black.add("BLACK",3,6,1529.25f,-90.125f,-4321.5f,0x4000,440);normal.add("BLUE",3,6,100.0f,0.0f,0.0f,0,220);black.armed=true;
  World we,wl;unsigned matched_e=0,matched_l=0;
  for(unsigned tick=0;tick<30;++tick)for(unsigned a=0;a<8;++a){
   b2::Snapshot s;s.profile=510;s.parameters=a==0?b2::kParameters:0x12345678u;s.set_id=a==0?0xFFFF:a;s.home_x=a==0?b2::kRangeCode:0;s.room=3;
   if(a==0){s.x=1529.25f;s.y=-90.125f;s.z=-4321.5f;s.home_y=0x4000;}else{s.x=float(a)*10.0f;s.y=0;s.z=0;}
   b2::Snapshot e=s,l=s;eager(e,we);lazy(black,normal,l,wl);if(black.find(e)>=0)++matched_e;if(black.find(l)>=0)++matched_l;}
  require(we.context_calls==240&&wl.context_calls==30,"context (getStartStageName + getLayerNo) calls: 240 -> 30 per 30 ticks of 8 Bokoblins");
  require(matched_e==30&&matched_l==30,"the registered Black is still matched on every tick");
  pass("lazy_context_cuts_layer_lookups_to_the_actors_that_can_match");}
 // ---------- 2. early-exit lookup is exact ----------
 {Rng r(0xBADA55u);devf_vis::Ledger vl;devf_dyn::Ledger dl;static int ids[64];
  for(unsigned it=0;it<300000;++it){
   const std::uint32_t id=r.below(90);
   switch(r.below(3)){
    case 0:{vl.record(id,&ids[id%64],1,r.coin()?Variant::Blue:Variant::Black);dl.record(id,&ids[id%64],1,Variant::Red,2);break;}
    case 1:{if(auto* e=vl.find(id))vl.release(*e);if(auto* e=dl.find(id))dl.release(*e);break;}
    default:break;
   }
   const std::uint32_t q=r.below(100);
   require(devf_perf::find_used(vl,q)==vl.find(q),"visual ledger: find_used == find");
   require(devf_perf::find_used(dl,q)==dl.find(q),"dynamic ledger: find_used == find");
  }
  require(devf_perf::find_used(vl,0)==nullptr&&vl.find(0)==nullptr,"id 0 never found");
  devf_vis::Ledger empty;require(devf_perf::find_used(empty,5)==nullptr,"empty ledger");
  pass("early_exit_lookup_returns_exactly_what_ledger_find_returns");}
 // ---------- 3. telemetry budget ----------
 {devf_perf::LogBudget b;using F=devf_perf::LineFilter;
  require(b.detail(Variant::Blue)&&b.detail(Variant::Black),"detail at session start");
  require(!b.consume(Variant::Blue)&&b.detail(Variant::Blue),"1st Blue acquisition: still detailed");
  require(b.consume(Variant::Blue)&&!b.detail(Variant::Blue),"2nd Blue acquisition: budget exhausted, reported once");
  require(!b.consume(Variant::Blue)&&!b.consume(Variant::Blue),"never reported again");
  require(b.detail(Variant::Black),"Black has its own budget");
  require(F::requested(true)&&!F::requested(false),"REQUESTED only while detailed");
  require(F::colour(false,false)&&!F::colour(false,true)&&F::colour(true,true),"a colour mismatch is always logged");
  require(F::tuning(false,false,true)&&F::tuning(false,true,false)&&!F::tuning(false,true,true)&&F::tuning(true,true,true),"a geometry or presence mismatch is always logged");
  require(F::basis(false,false)&&!F::basis(false,true)&&F::basis(true,true),"a degenerate draw basis is always logged");
  pass("telemetry_budget_keeps_two_detailed_acquisitions_per_variant_and_every_anomaly");}
 std::printf("PERF_LAB_PASS cases=%u EXACTNESS_OF_CPU_OPTIMISATIONS_ONLY; FPS_NOT_MEASURED; GAME_NOT_EXECUTED\n",cases);
}
