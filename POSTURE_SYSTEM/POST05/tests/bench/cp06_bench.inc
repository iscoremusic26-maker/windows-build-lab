// CP06 synthetic CPU benchmark (NOT FPS, NOT a game measurement). Production CP04/CP05/FIX02
// (and CP06 when present) callbacks around the unchanged native Attack/Find/cc_set with the
// real Expansion Red/Black policies; identical driver, compiler and flags for both sources.
#include <chrono>
static bool callbacks_on=true;
void bench_tick(daE_OC_c& a){
 const int slot=slot_of(a);auto& p=policy_ports[slot];auto* self=&a;void* args[]={&self};
 if(callbacks_on){expansion_pre(a,p);on_eoc_pre_cp04(nullptr,args,nullptr,nullptr);on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);}else expansion_pre(a,p);
 if(a.field_0x6ca)--a.field_0x6ca;if(a.field_0x6c0)--a.field_0x6c0;if(a.field_0x6c2)--a.field_0x6c2;a.mSphs_at[0].on=a.mSphs_at[1].on=false;
 if(a.mActionMode==4){if(callbacks_on){on_attack_pre_cp04(nullptr,args,nullptr,nullptr);on_attack_pre_cp05(nullptr,args,nullptr,nullptr);}a.executeAttack();}else if(a.mActionMode==3)a.executeFind();
 native_update(a.morf);
 if(callbacks_on)on_cc_pre_fix01(nullptr,args,nullptr,nullptr);a.cc_set();if(callbacks_on)on_cc_post_fix01(nullptr,args,nullptr,nullptr);
 if(callbacks_on){on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);
#ifdef BENCH_CP06
 on_eoc_post_cp06_capture(nullptr,args,nullptr,nullptr);
#endif
 }
 expansion_post(a,p);
 if(callbacks_on){on_eoc_post_cp04(nullptr,args,nullptr,nullptr);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);
#ifdef BENCH_CP06
 on_eoc_post_cp06_chip(nullptr,args,nullptr,nullptr);
#endif
 }
}
int main(int argc,char** argv){
 const unsigned actors=argc>1?unsigned(std::atoi(argv[1])):16,ticks=argc>2?unsigned(std::atoi(argv[2])):3000,reps=argc>3?unsigned(std::atoi(argv[3])):15;
 static daE_OC_c pool[64];double with_ns[64]{},without_ns[64]{};
 for(unsigned r=0;r<reps;++r)for(int mode=0;mode<2;++mode){
  reset();
#ifdef BENCH_CP06
  c6=Cp06Runtime{};(void)cp06_install();
#endif
  fx={};f2={};g.layout=1;
  for(unsigned i=0;i<actors;++i){auto& a=pool[i];a=daE_OC_c{};a.mpMorf=&a.morf;a.id=500+i;const bool black=i%3==2;admit(a,black);if(i%3==0){a.atp=2;}}
  callbacks_on=mode==0;player_distance=230;
  for(unsigned w=0;w<300;++w){++g.p8.tick;for(unsigned i=0;i<actors;++i)bench_tick(pool[i]);}  // warm-up, combos selected
  const auto t0=std::chrono::steady_clock::now();
  for(unsigned t=0;t<ticks;++t){++g.p8.tick;for(unsigned i=0;i<actors;++i)bench_tick(pool[i]);cp04_maintenance();cp05_maintenance();
#ifdef BENCH_CP06
   cp06_maintenance();
#endif
  }
  const double ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-t0).count()/(double(ticks)*actors);
  (mode==0?with_ns:without_ns)[r]=ns;
 }
 std::sort(with_ns,with_ns+reps);std::sort(without_ns,without_ns+reps);
 const double med_with=with_ns[reps/2],med_without=without_ns[reps/2];
 std::printf("CP06_BENCH actors=%u ticks=%u reps=%u ns_per_actor_tick_with_callbacks(min/med/max)=%.1f/%.1f/%.1f native_model_only(min/med/max)=%.1f/%.1f/%.1f callback_cost_median_ns=%.1f per_tick_us_all_actors=%.3f evidence=SYNTHETIC_LAB_NOT_FPS\n",
  actors,ticks,reps,with_ns[0],med_with,with_ns[reps-1],without_ns[0],med_without,without_ns[reps-1],med_with-med_without,(med_with-med_without)*actors/1000.0);
}
