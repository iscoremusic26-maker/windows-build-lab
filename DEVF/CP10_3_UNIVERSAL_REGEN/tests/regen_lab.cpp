// CP10.3 offline lab: regeneration policy, timing, caps, death exclusion, isolation, registry. No game, no GPU.
// "Tick" = one completed daE_OC_c::execute pair = 1/30 s of game time (see regen_policy.hpp header for the host evidence).
#include "../expansion/regen_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace devf_regen;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
Entry make(const char* name,std::int16_t base,int black){const auto c=classify(name,base,black);require(c.kind!=Kind::None,"classified");Entry e;e.used=true;e.id=1;e.kind=c.kind;e.max_hp=c.max_hp;e.rate=c.rate;return e;}
// Runs `ticks` execute pairs with the actor in `action`; returns the health. Optional per-tick damage hook models native damage inside execute.
std::int16_t run(Entry& e,std::int16_t health,unsigned ticks,int action=A_FIND){for(unsigned i=0;i<ticks;++i)health=step(e,health,action).new_health;return health;}
int main(){
 static_assert(kTicksPerSecond==30&&kRateNormalHpPerSecond==2&&kRateBlackHpPerSecond==4,"spec");
 static_assert(kMaxHpE_OC==40&&kMaxHpE_OC2==220,"native maxima (d_a_e_oc.cpp create)");
 // 1/2/3 baselines and exact rates
 {Entry e=make("E_OC",40,0);require(e.max_hp==40&&e.rate==2,"E_OC");Entry f=make("E_OC2",220,0);require(f.max_hp==220&&f.rate==2,"E_OC2");pass("native_E_OC_40_and_E_OC2_220_classified_separately");}
 {Entry e=make("E_OC2",220,0);std::int16_t h=run(e,100,30);require(h==102,"2 HP in one game second");h=run(e,100,300);require(h==120,"20 HP in 10 s");pass("normal_exactly_2_hp_per_game_second");}
 {Entry e=make("E_OC2",220,150);require(e.kind==Kind::Black&&e.rate==4&&e.max_hp==150,"black classified");std::int16_t h=run(e,50,30);require(h==54,"4 HP in 1 s");h=run(e,50,300);require(h==90,"40 HP in 10 s");pass("black_exactly_4_hp_per_game_second");}
 {for(int rate:{2,4}){Entry e;e.used=true;e.kind=Kind::Native;e.max_hp=30000;e.rate=std::uint8_t(rate);const std::int16_t start=1;std::int16_t h=start;const unsigned seconds=1000;h=run(e,h,seconds*30);require(h==start+int(seconds)*rate,"zero drift over 1000 s");
   Entry g=e;g=Entry{};g.used=true;g.kind=Kind::Native;g.max_hp=30000;g.rate=std::uint8_t(rate);int total=0;for(unsigned i=1;i<=9999;++i){auto s=step(g,1,A_WAIT);total+=s.heal;require(total==int((i*unsigned(rate))/30),"exact floor at every tick");}}
  pass("integer_accumulator_exact_no_drift");}
 // fractions
 {Entry e=make("E_OC2",220,0);std::int16_t h=run(e,10,14);require(h==10&&e.acc==28,"14 ticks: no heal yet, fraction kept");h=step(e,h,A_WAIT).new_health;require(h==11&&e.acc==0,"15th tick heals 1");
  Entry b=make("E_OC2",220,150);h=run(b,10,7);require(h==10&&b.acc==28,"black 7 ticks");h=step(b,h,A_WAIT).new_health;require(h==11&&b.acc==2,"black 8th tick heals 1, 2/30 kept");pass("fraction_accumulation_no_early_rounding");}
 // time base: ticks, not frames. Interpolation bursts / dropped frames do not change totals.
 {auto bursty=[](unsigned schedule){Entry e=make("E_OC2",220,0);std::int16_t h=50;unsigned ticks=0;for(unsigned frame=0;frame<600;++frame){unsigned n=schedule==0?1:(frame%4==0?4:0);for(unsigned i=0;i<n;++i){h=step(e,h,A_FIND).new_health;++ticks;}}return std::pair<int,unsigned>{h,ticks};};
  auto a=bursty(0),b=bursty(1);require(a.second==600&&b.second==600&&a.first==b.first&&a.first==50+600*2/30,"same totals for 1 tick/frame and 4-tick bursts");pass("render_fps_pattern_independent");}
 // pause / loading: no execute => no ticks => nothing healed
 {Entry e=make("E_OC2",220,0);std::int16_t h=run(e,100,15);const auto snapshot=e;/* pause: zero executes for any wall time */h=h;require(e.ticks==snapshot.ticks&&e.acc==snapshot.acc&&h==101,"no executes, no change");pass("pause_and_loading_grant_nothing");}
 // pair semantics
 {Registry r;int actor=0;bool stale=false;Why why=Why::None;Entry* e=r.begin(77,&actor,false,&stale,&why);require(e&&e->pending&&!stale,"begin");
  require(r.begin(77,&actor,false,&stale,&why)==nullptr&&why==Why::Reentrant,"second PRE before POST is not counted");
  require(r.end(77,&actor)==e&&!e->pending,"POST completes");require(r.end(77,&actor)==nullptr,"second POST ignored");
  int other=0;require(r.end(77,&other)==nullptr,"POST of another actor ignored");require(r.end(78,&actor)==nullptr,"POST without PRE ignored");pass("pre_post_pair_counts_exactly_one_tick");}
 // damage during regeneration: no cooldown, native processing untouched
 {Entry e=make("E_OC2",220,0);std::int16_t h=100;unsigned healed=0;for(unsigned t=1;t<=90;++t){if(t%20==0)h=std::int16_t(h-10); // native hit inside the tick, before POST
   auto s=step(e,h,t%20==0?A_DAMAGE:A_FIND);healed+=unsigned(s.heal);h=s.new_health;}
  require(healed==6&&h==100-4*10+6,"hits never pause or cancel regeneration (90 ticks = 6 HP)");pass("damage_does_not_pause_regeneration");}
 // post-hook ordering: lethal hit is never pre-empted by a heal
 {Entry e=make("E_OC2",220,0);e.acc=29;std::int16_t after_native_damage=0;auto s=step(e,after_native_damage,A_DEATH);require(s.heal==0&&s.new_health==0,"dead stays dead even with a full accumulator");
  Entry f=make("E_OC2",220,0);f.acc=29;s=step(f,-5,A_BIG_DAMAGE);require(s.heal==0&&s.new_health==-5,"negative health untouched");pass("health_zero_or_below_never_revived");}
 // irreversible / scripted actions
 {for(int a:{int(A_DEATH),int(A_WATER_DEATH),int(A_FALL_DEAD),int(A_FALL),int(A_DEMO_MASTER),int(A_DEMO_CHILD),-1,int(A_COUNT),99}){Entry e=make("E_OC2",220,0);e.acc=29;auto s=step(e,100,a);require(s.heal==0&&s.new_health==100&&e.acc==0,"blocked action");}
  for(int a:{int(A_WAIT),int(A_WALK),int(A_TALK),int(A_FIND),int(A_ATTACK),int(A_DAMAGE),int(A_BIG_DAMAGE),int(A_WATCH),int(A_SOUND_WATCH),int(A_FIND_STAY),int(A_MOVE_OUT)}){Entry e=make("E_OC2",220,0);auto s=step(e,100,a);require(!blocks_regen(a)&&s.skip[0]!='I',"live action allowed");}
  pass("death_water_fall_demo_states_never_heal_even_with_positive_hp");}
 // caps
 {Entry e=make("E_OC",40,0);std::int16_t h=run(e,38,300);require(h==40,"E_OC clamps at 40");Entry f=make("E_OC2",220,0);h=run(f,218,300);require(h==220,"E_OC2 clamps at 220");pass("native_caps_40_and_220");}
 {Entry e=make("E_OC2",220/*native field_0x560*/,150);std::int16_t h=run(e,146,600);require(h==150,"Black stops at configured 150, never 220");require(e.max_hp==150,"max from registration");
  const auto n=classify("E_OC2",220,0);require(n.kind==Kind::Native&&n.max_hp==220&&n.rate==2,"same name without registration is Normal 220");pass("black_cap_is_configured_hp_not_native_field_0x560");}
 {Entry e=make("E_OC2",220,0);auto s=step(e,300,A_FIND);require(s.heal==0&&s.new_health==300&&e.acc==0,"above max untouched");Entry b=make("E_OC2",220,150);s=step(b,200,A_FIND);require(s.heal==0&&s.new_health==200,"never normalizes a higher valid value");
  Entry f=make("E_OC2",220,0);std::int16_t h=run(f,219,100);require(h==220,"exactly full");auto t=step(f,h,A_FIND);require(t.heal==0&&f.acc==0&&!t.milestone,"full: no write, no banked fraction");pass("max_never_exceeded_never_lowered_no_banking_at_full");}
 {Entry e=make("E_OC2",220,999);require(e.max_hp==999,"black 999");std::int16_t h=run(e,998,10);require(h==999,"s16 safe");Entry big;big.used=true;big.kind=Kind::Native;big.max_hp=32767;big.rate=4;h=run(big,32766,100);require(h==32767,"no s16 overflow");pass("s16_overflow_guard");}
 // classification contract / fail-closed
 {require(classify("E_OC2",300,0).why==Why::MaxHpContract,"modified native max rejected");require(classify("E_OC",220,0).why==Why::MaxHpContract,"wrong base for name");require(classify("E_GOB",220,0).why==Why::UnknownName&&classify(nullptr,40,0).why==Why::UnknownName&&classify("E_OC3",220,0).why==Why::UnknownName,"other enemy names rejected");
  require(classify("E_OC2",220,1000).why==Why::BadBlackHp&&classify("E_OC2",220,150).kind==Kind::Black,"black hp bound");Entry r;r.used=true;r.rejected=true;r.kind=Kind::None;auto s=step(r,10,A_FIND);require(s.heal==0,"rejected never heals");pass("classification_fail_closed_no_false_positives");}
 // independence of simultaneous actors of different kinds
 {Entry a=make("E_OC",40,0),b=make("E_OC2",220,0),c=make("E_OC2",220,150);a.id=1;b.id=2;c.id=3;std::int16_t ha=10,hb=100,hc=100;for(unsigned i=0;i<60;++i){ha=step(a,ha,A_FIND).new_health;hb=step(b,hb,A_FIND).new_health;hc=step(c,hc,A_FIND).new_health;}
  require(ha==14&&hb==104&&hc==108,"2s: +4, +4, +8");require(a.acc==0&&b.acc==0&&c.acc==0,"separate accumulators");
  ha=step(a,ha,A_DEATH).new_health;require(ha==14&&step(b,hb,A_FIND).new_health>=hb,"one death does not affect others");pass("simultaneous_actors_independent");}
 // identity: id reuse, re-entry, no inherited fraction or maximum
 {Registry r;int a1=0,a2=0;Entry* e=r.begin(10,&a1,false);require(e!=nullptr,"entry");e->kind=Kind::Black;e->max_hp=150;e->rate=4;e->acc=29;e->healed=40;r.end(10,&a1);
  bool stale=false;Entry* n=r.begin(10,&a2,false,&stale);require(stale&&n&&n->kind==Kind::None&&n->acc==0&&n->healed==0&&n->max_hp==0,"same id, other address: fresh state");
  r.end(10,&a2);Entry* m=r.begin(10,&a2,true,&stale);require(stale&&m&&m->acc==0&&m->kind==Kind::None,"first-execute marker on a tracked id resets state");pass("id_reuse_and_reentry_never_transfer_state");}
 {Registry r;int a=0;Entry* e=r.begin(5,&a,false);e->kind=Kind::Native;e->acc=17;r.end(5,&a);const auto gen=e->generation;r.release(*e);require(!r.find(5)&&r.used==0,"released");Entry* f=r.begin(5,&a,false);require(f&&f->acc==0&&f->generation!=gen,"new generation after release");pass("release_then_respawn_starts_clean");}
 // bounded registry
 {Registry r;std::vector<int> actors(kCapacity+8);for(unsigned i=0;i<kCapacity;++i){Entry* e=r.begin(100+i,&actors[i],false);require(e!=nullptr,"fill");r.end(100+i,&actors[i]);}
  Why why=Why::None;require(r.begin(9999,&actors[kCapacity],false,nullptr,&why)==nullptr&&why==Why::RegistryFull&&r.used==kCapacity,"full: refused, no growth");
  Entry* e0=r.find(100);require(e0!=nullptr,"find");e0->kind=Kind::Native;e0->max_hp=220;e0->rate=2;require(step(*e0,100,A_FIND).skip[0]!='R',"existing keeps working");
  r.release(*r.find(101));Entry* again=r.begin(9999,&actors[kCapacity],false);require(again&&r.used==kCapacity,"slot reusable after release");pass("registry_bounded_fail_closed_no_unbounded_growth");}
 // invalid ids
 {Registry r;int a=0;require(!r.begin(0,&a,false)&&!r.begin(0xFFFFFFFFu,&a,false)&&!r.begin(1,nullptr,false)&&r.used==0,"invalid identity refused");pass("invalid_identity_refused");}
 // log throttling constants
 {Entry e=make("E_OC2",220,0);unsigned logs=0;std::int16_t h=1;for(unsigned i=0;i<30*40;++i){auto s=step(e,h,A_FIND);h=s.new_health;if(s.milestone)++logs;}require(logs>=1&&logs<=1+e.healed/kHealLogEveryHp,"milestone logs bounded");require(logs<=kHealLogMaxPerActor+1,"well below the cap in 40 s");pass("telemetry_milestones_not_per_frame");}
 // no Black registration anywhere: native regeneration still works
 {Entry e=make("E_OC",40,0);std::int16_t h=run(e,1,15*10);require(h==11&&e.kind==Kind::Native,"regen without any DEVF Black");pass("regeneration_independent_of_black_registration");}
 std::printf("REGEN_LAB_PASS cases=%u OFFLINE_ONLY; TICK_TIME_BASE=1/30s; NATIVE_HOOKS_AND_GAME_NOT_EXECUTED\n",cases);
}
