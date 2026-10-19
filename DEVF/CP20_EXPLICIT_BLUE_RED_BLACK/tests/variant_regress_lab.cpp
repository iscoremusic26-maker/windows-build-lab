// CP10.4 offline lab: variant identity (Blue/Red/Black), per-variant HP, regeneration. Pure logic: no game, no GPU.
// Includes the CP10.3 timing / death / cap / registry cases, ported to the new maxima (Blue 220, Red 330, Black configured) without relaxing them.
#include "../runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
#include "../runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace devf_variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
using R=Registration;
Plan P(const char* name,std::int16_t f560,int health,bool first,R reg=R::None,int bh=0){return make_plan(name,f560,health,first,reg,bh);}
Entry E(const Plan& p){require(p.variant!=Variant::None,"plan accepted");Entry e;e.used=true;e.id=1;adopt(e,p);return e;}
std::int16_t run(Entry& e,std::int16_t h,unsigned ticks,int action=A_FIND){for(unsigned i=0;i<ticks;++i)h=step(e,h,action).new_health;return h;}
struct Req {char placement_id[32];int room,layer;float x,y,z;int yaw_s16;};
struct FakeConfig {unsigned count=0;Req requests[8]{};unsigned variant[8]{};int hp[8]{};};
FakeConfig cfg(std::initializer_list<std::pair<const char*,int>> items){FakeConfig c;for(auto& it:items){Req& r=c.requests[c.count];std::snprintf(r.placement_id,32,"%s",it.first);r.room=1;r.layer=6;r.x=-25050.2812f+float(c.count)*1000;r.y=-8097.86523f;r.z=80595.0703f;r.yaw_s16=0;c.variant[c.count]=it.second?1u:0u;c.hp[c.count]=it.second;++c.count;}return c;}
devf_black2::Snapshot snap_of(const Req& r){devf_black2::Snapshot s;s.profile=510;s.room=r.room;s.layer=r.layer;s.parameters=devf_black2::kParameters;s.set_id=0xFFFF;s.x=r.x;s.y=r.y;s.z=r.z;s.home_x=devf_black2::kRangeCode;s.home_y=r.yaw_s16;s.home_z=0;s.stage=true;s.initializing=true;s.resource_eoc2=true;s.health=220;return s;}
int main(){
 static_assert(kRateBlue==2&&kRateRed==3&&kRateBlack==4&&kTicksPerSecond==30,"rates");
 static_assert(kMaxHpBlue==220&&kMaxHpRed==330&&kNativeHpE_OC==40&&kNativeHpE_OC2==220,"hp table");
 // ---------- A. identity and precedence ----------
 {auto p=P("E_OC",40,40,true);require(p.variant==Variant::Blue&&p.source==Source::NativeEOC&&p.native_base==40&&p.initial_hp==220&&p.max_hp==220&&p.rate==2&&p.write_hp,"E_OC");pass("native_E_OC_40_becomes_BLUE_220_2hps");}
 {auto p=P("E_OC2",220,220,true);require(p.variant==Variant::Red&&p.source==Source::NativeEOC2&&p.native_base==220&&p.initial_hp==330&&p.max_hp==330&&p.rate==3&&p.write_hp,"E_OC2");pass("native_E_OC2_220_becomes_RED_330_3hps");}
 {auto p=P("E_OC2",220,220,true,R::Normal);require(p.variant==Variant::Blue&&p.source==Source::DevfNormal&&p.max_hp==220&&p.rate==2&&!p.write_hp,"normal");pass("registered_DEVF_normal_E_OC2_is_BLUE_not_RED");}
 {auto p=P("E_OC2",220,220,true,R::Black,440);require(p.variant==Variant::Black&&p.source==Source::DevfBlack&&p.max_hp==440&&p.initial_hp==440&&p.rate==4&&!p.write_hp,"black440");auto q=P("E_OC2",220,220,true,R::Black,150);require(q.max_hp==150&&q.rate==4,"black150 custom");pass("registered_black_uses_configured_hp_440_or_150");}
 {require(P("E_OC2",220,220,true,R::BlackAdmissionFailed,440).why==Why::BlackAdmissionRejected,"failed black must not fall through to RED");require(P("E_OC2",220,220,true,R::BlackAdmissionFailed,440).variant==Variant::None,"rejected");pass("failed_black_admission_never_becomes_red_or_blue");}
 {const auto c=cfg({{"bokoblin-001",0},{"bokoblin-004",150}});devf_black2::Table black,normal;require(devf_variant::register_encounters(c,1u,black,normal)&&black.count==1&&normal.count==1,"tables");
  const auto n=snap_of(c.requests[0]);const auto b=snap_of(c.requests[1]);
  auto m=devf_variant::match(black,normal,n);require(m.normal==0&&m.black==-1,"normal matched");m=devf_variant::match(black,normal,b);require(m.black==0&&m.normal==-1,"black matched");
  // exact-match only
  auto s=n;s.x=std::nextafter(s.x,1e9f);require(devf_variant::match(black,normal,s).normal==-1,"1 ulp different position is NOT a registration");
  s=n;s.z+=0.5f;require(devf_variant::match(black,normal,s).normal==-1,"close but different");
  s=n;s.room=2;require(devf_variant::match(black,normal,s).normal==-1,"other room");s=n;s.layer=5;require(devf_variant::match(black,normal,s).normal==-1,"other layer");s=n;s.stage=false;require(devf_variant::match(black,normal,s).normal==-1,"other stage");
  s=n;s.home_y=9;require(devf_variant::match(black,normal,s).normal==-1,"other yaw");s=n;s.home_x=0x0106;require(devf_variant::match(black,normal,s).normal==-1,"other range code (provider legacy site)");s=n;s.parameters=0;require(devf_variant::match(black,normal,s).normal==-1,"other params");s=n;s.profile=511;require(devf_variant::match(black,normal,s).normal==-1,"other profile");
  require(P("E_OC2",220,220,true).variant==Variant::Red,"unmatched native E_OC2 is RED");pass("near_but_not_identical_actor_is_never_blue_or_black_exact_match_only");}
 {devf_black2::Table black,normal;auto c=cfg({{"a",150},{"b",150}});c.requests[1]=c.requests[0];std::snprintf(c.requests[1].placement_id,32,"%s","b");require(!devf_variant::register_encounters(c,1u,black,normal),"two Black registrations with the identical record are refused");
  devf_black2::Table b2,n2;auto d=cfg({{"x",0},{"y",0}});d.requests[1]=d.requests[0];require(!devf_variant::register_encounters(d,1u,b2,n2),"duplicate normal record refused");pass("registration_builder_refuses_duplicate_records");}
 // precedence when the same record is somehow in both tables: Black wins
 {devf_black2::Table black,normal;black.add("bk",1,6,1,2,3,0,150);normal.add("nm",1,6,1,2,3,0,220);auto s=snap_of(Req{"x",1,6,1,2,3,0});auto m=devf_variant::match(black,normal,s);require(m.black==0&&m.normal==-1,"Black > Normal");pass("black_registration_has_priority_over_normal");}
 {for(auto cfgcase:{0,1,2,3}){FakeConfig c;if(cfgcase==1)c=cfg({{"n",0}});if(cfgcase==2)c=cfg({{"b",150}});if(cfgcase==3)c=cfg({{"n",0},{"b",440}});devf_black2::Table black,normal;require(devf_variant::register_encounters(c,1u,black,normal),"register");
   require(P("E_OC",40,40,true).variant==Variant::Blue&&P("E_OC2",220,220,true).variant==Variant::Red,"natives classified regardless of the file contents");}
  pass("native_blue_and_red_work_without_file_empty_file_only_normal_only_black_and_mixed");}
 {require(P("E_OC2",220,220,true,R::Black,440).max_hp==440&&P("E_OC2",220,220,true,R::Black,150).max_hp==150,"hp from registration");const auto b150=P("E_OC2",220,220,true,R::Black,150);require(b150.max_hp!=440,"150 stays 150 until the JSON says 440");pass("black_hp150_stays_150_hp440_is_440_no_automatic_migration");}
 // ---------- B. rejection / fail-closed ----------
 {require(P("E_GOB",220,220,true).why==Why::UnknownName&&P(nullptr,40,40,true).why==Why::UnknownName&&P("E_OC3",220,220,true).why==Why::UnknownName,"other names");
  require(P("E_OC2",300,220,true).why==Why::NativeSignature&&P("E_OC",220,40,true).why==Why::NativeSignature,"modified f560 signature");
  require(P("E_OC2",220,200,true).why==Why::NativeHealthNotInitial&&P("E_OC",40,39,true).why==Why::NativeHealthNotInitial&&P("E_OC2",220,330,true).why==Why::NativeHealthNotInitial,"already-modified or damaged health");
  require(P("E_OC2",220,220,false).why==Why::FirstExecuteNotObserved&&P("E_OC",40,40,false).why==Why::FirstExecuteNotObserved,"no first execute: no mid-combat assignment");
  require(P("E_OC",40,40,true,R::Black,440).why==Why::BadRegistration&&P("E_OC",40,40,true,R::Normal).why==Why::BadRegistration,"registration incompatible with resource");
  require(P("E_OC2",220,220,true,R::Black,0).why==Why::BlackHpRange&&P("E_OC2",220,220,true,R::Black,1000).why==Why::BlackHpRange,"black hp range");
  for(auto p:{P("E_GOB",220,220,true),P("E_OC2",300,220,true),P("E_OC2",220,220,false)}){require(p.variant==Variant::None&&!p.write_hp&&p.max_hp==0,"rejected: no write");}
  pass("rejections_fail_closed_no_hp_write_no_regeneration");}
 {Entry e;e.used=true;adopt(e,P("E_OC2",300,220,true));require(e.rejected&&step(e,100,A_FIND).heal==0&&step(e,10,A_WAIT).heal==0,"rejected entry never heals");pass("rejected_instance_never_regenerates");}
 // ---------- one-time initialisation ----------
 {Registry r;int actor=0;Entry* e=r.begin(5,&actor,true);require(e&&!e->classified,"fresh");int writes=0;
  auto classify_once=[&](Entry* x,int health){if(x->classified)return;const Plan p=P("E_OC2",220,health,true);adopt(*x,p);if(p.write_hp)++writes;};
  classify_once(e,220);r.end(5,&actor);
  for(unsigned t=0;t<200;++t){Entry* x=r.begin(5,&actor,false);require(x!=nullptr,"entry stays");classify_once(x,330);r.end(5,&actor);}
  require(writes==1&&e->variant==Variant::Red,"exactly one initial write across 200 later ticks (hits, regen, reentrancy)");
  Entry* again=r.begin(5,&actor,false);require(r.begin(5,&actor,false)==nullptr,"reentrant PRE does not reclassify");(void)again;pass("hp_written_once_at_first_execute_never_again");}
 {Registry r;int a1=0,a2=0;Entry* e=r.begin(8,&a1,true);adopt(*e,P("E_OC2",220,220,true));e->acc=17;e->healed=33;r.end(8,&a1);
  bool stale=false;Entry* n=r.begin(8,&a2,true,&stale);require(stale&&n&&!n->classified&&n->acc==0&&n->healed==0&&n->variant==Variant::None,"respawn / id reuse: fresh, unclassified, no inherited state");pass("reentry_and_id_reuse_start_unclassified");}
 // ---------- C. HP / regeneration (ported from CP10.3) ----------
 {Entry b=E(P("E_OC",40,40,true)),r=E(P("E_OC2",220,220,true)),k=E(P("E_OC2",220,220,true,R::Black,440));
  require(run(b,100,30)==102&&run(r,100,30)==103&&run(k,100,30)==104,"2/3/4 HP in one game second");require(run(b,100,300)==120&&run(r,100,300)==130&&run(k,100,300)==140,"10 s");
  pass("exact_2_3_4_hp_per_30_ticks");}
 {Entry r=E(P("E_OC2",220,220,true));std::int16_t h=run(r,10,9);require(h==10,"no heal before tick 10");h=step(r,h,A_WAIT).new_health;require(h==11&&r.acc==0,"RED: 1 HP every 10 ticks");
  Entry b=E(P("E_OC",40,40,true));h=run(b,10,14);require(h==10&&b.acc==28,"BLUE 14 ticks");h=step(b,h,A_WAIT).new_health;require(h==11,"BLUE: 1 HP every 15 ticks");
  Entry k=E(P("E_OC2",220,220,true,R::Black,440));h=run(k,10,7);require(h==10&&k.acc==28,"BLACK 7 ticks");h=step(k,h,A_WAIT).new_health;require(h==11&&k.acc==2,"BLACK ~1 HP every 7-8 ticks");pass("fractional_accumulators_per_variant");}
 {for(int rate:{2,3,4}){Entry e;e.used=true;e.variant=Variant::Blue;e.max_hp=30000;e.rate=std::uint8_t(rate);std::int16_t h=1;h=run(e,h,1000*30);require(h==1+1000*rate,"zero drift over 1000 s");
   Entry g;g.used=true;g.variant=Variant::Blue;g.max_hp=30000;g.rate=std::uint8_t(rate);int total=0;for(unsigned i=1;i<=9999;++i){total+=step(g,1,A_WAIT).heal;require(total==int(i*unsigned(rate)/30),"floor-exact");}}pass("integer_accumulator_exact_no_drift_2_3_4");}
 {auto bursty=[](int mode){Entry e=E(P("E_OC2",220,220,true));std::int16_t h=50;for(unsigned f=0;f<600;++f){unsigned n=mode==0?1:(f%4==0?4:0);for(unsigned i=0;i<n;++i)h=step(e,h,A_FIND).new_health;}return h;};require(bursty(0)==bursty(1)&&bursty(0)==50+600*3/30,"render pattern independent");pass("render_fps_pattern_independent");}
 {Entry e=E(P("E_OC2",220,220,true));std::int16_t h=run(e,100,15);const auto frozen=e;const auto h_frozen=h;/* pause / loading / event stop: the actor executes zero times, whatever the wall time */require(e.ticks==frozen.ticks&&e.acc==frozen.acc&&h==h_frozen&&h==101&&e.acc==15,"15 ticks of RED: 1 HP at tick 10, 15/30 kept; nothing changes without executes");pass("pause_and_loading_grant_nothing");}
 {Registry r;int actor=0;Entry* e=r.begin(77,&actor,false);require(e&&r.begin(77,&actor,false)==nullptr,"second PRE not counted");require(r.end(77,&actor)==e&&r.end(77,&actor)==nullptr,"POST once");int other=0;require(r.end(77,&other)==nullptr&&r.end(78,&actor)==nullptr,"foreign POST ignored");pass("pre_post_pair_counts_exactly_one_tick");}
 {Entry e=E(P("E_OC2",220,220,true));std::int16_t h=200;unsigned healed=0;for(unsigned t=1;t<=90;++t){if(t%20==0)h=std::int16_t(h-10);auto s=step(e,h,t%20==0?A_DAMAGE:A_FIND);healed+=unsigned(s.heal);h=s.new_health;}require(healed==9&&h==200-40+9,"hits never pause RED regeneration (90 ticks = 9 HP)");pass("damage_does_not_pause_regeneration");}
 {Entry e=E(P("E_OC2",220,220,true));e.acc=29;require(step(e,0,A_DEATH).heal==0&&step(e,-5,A_BIG_DAMAGE).heal==0,"no revive");pass("health_zero_or_below_never_revived");}
 {for(int a:{int(A_DEATH),int(A_WATER_DEATH),int(A_FALL_DEAD),int(A_FALL),int(A_DEMO_MASTER),int(A_DEMO_CHILD),-1,int(A_COUNT),99}){Entry e=E(P("E_OC2",220,220,true));e.acc=29;auto s=step(e,100,a);require(s.heal==0&&e.acc==0,"blocked action");}
  for(int a:{int(A_WAIT),int(A_WALK),int(A_TALK),int(A_FIND),int(A_ATTACK),int(A_DAMAGE),int(A_BIG_DAMAGE),int(A_WATCH),int(A_SOUND_WATCH),int(A_FIND_STAY),int(A_MOVE_OUT)}){require(!blocks_regen(a),"live action");}
  pass("death_water_fall_demo_states_never_heal_even_with_positive_hp");}
 {Entry b=E(P("E_OC",40,40,true));require(run(b,218,300)==220,"BLUE cap 220");Entry r=E(P("E_OC2",220,220,true));require(run(r,328,300)==330,"RED cap 330");Entry k=E(P("E_OC2",220,220,true,R::Black,440));require(run(k,438,300)==440,"BLACK cap 440");Entry c=E(P("E_OC2",220,220,true,R::Black,150));require(run(c,146,600)==150,"custom 150 cap");pass("caps_220_330_440_and_custom_150");}
 {Entry r=E(P("E_OC2",220,220,true));auto s=step(r,400,A_FIND);require(s.heal==0&&s.new_health==400&&r.acc==0,"above max untouched");Entry k=E(P("E_OC2",220,220,true,R::Black,150));s=step(k,200,A_FIND);require(s.heal==0&&s.new_health==200,"never normalised down");
  Entry f=E(P("E_OC2",220,220,true));std::int16_t h=run(f,329,100);require(h==330,"exactly full");auto t=step(f,h,A_FIND);require(t.heal==0&&f.acc==0&&!t.milestone,"no banking at full");pass("max_never_exceeded_never_lowered_no_banking_at_full");}
 {Entry e=E(P("E_OC2",220,220,true,R::Black,999));require(run(e,998,10)==999,"999");Entry big;big.used=true;big.variant=Variant::Black;big.max_hp=32767;big.rate=4;require(run(big,32766,100)==32767,"s16");pass("s16_overflow_guard");}
 {Entry a=E(P("E_OC",40,40,true)),b=E(P("E_OC2",220,220,true)),c=E(P("E_OC2",220,220,true,R::Black,440));a.id=1;b.id=2;c.id=3;std::int16_t ha=10,hb=100,hc=100;for(unsigned i=0;i<60;++i){ha=step(a,ha,A_FIND).new_health;hb=step(b,hb,A_FIND).new_health;hc=step(c,hc,A_FIND).new_health;}
  require(ha==14&&hb==106&&hc==108,"2 s: +4 +6 +8");require(a.max_hp==220&&b.max_hp==330&&c.max_hp==440,"separate maxima");ha=step(a,ha,A_DEATH).new_health;require(ha==14&&step(b,hb,A_FIND).new_health>=hb,"independent");pass("simultaneous_blue_red_black_independent");}
 {Registry r;std::vector<int> actors(kCapacity+8);for(unsigned i=0;i<kCapacity;++i){Entry* e=r.begin(100+i,&actors[i],true);require(e!=nullptr,"fill");r.end(100+i,&actors[i]);}
  Why why=Why::None;require(r.begin(9999,&actors[kCapacity],true,nullptr,&why)==nullptr&&why==Why::RegistryFull&&r.used==kCapacity,"full: refused");r.release(*r.find(101));require(r.begin(9999,&actors[kCapacity],true)!=nullptr,"reusable");pass("registry_bounded_fail_closed");}
 {Registry r;int a=0;require(!r.begin(0,&a,true)&&!r.begin(0xFFFFFFFFu,&a,true)&&!r.begin(1,nullptr,true),"invalid identity");pass("invalid_identity_refused");}
 {Entry e=E(P("E_OC",40,40,true));unsigned logs=0;std::int16_t h=1;for(unsigned i=0;i<30*40;++i){auto s=step(e,h,A_FIND);h=s.new_health;if(s.milestone)++logs;}require(logs<=kHealLogMaxPerActor+1&&logs>=1,"bounded telemetry");pass("telemetry_milestones_not_per_frame");}
 std::printf("VARIANT_LAB_PASS cases=%u OFFLINE_ONLY; TICK_TIME_BASE=1/30s; NATIVE_HOOKS_AND_GAME_NOT_EXECUTED\n",cases);
}
