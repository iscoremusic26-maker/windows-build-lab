// CP10.6 offline lab: dynamic attack power (Blue group bonus, Black fury). Pure logic, no game, no GPU, no Link.
// It runs the SAME templates the mod runs (devf_dyn::admit / post_update / reconcile / restore_plan) against fake actors whose two attack
// spheres are plain bytes, driven by a world that mirrors the mod's PRE / native / POST order. It proves decisions, timing and ownership;
// it does NOT prove hearts lost by Link, nor that the real game honours the spheres: that is the human Windows round.
#include "../expansion/dynamic_policy.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
namespace vp=devf_variant;
using devf_dyn::Ev;
using vp::Variant;
using R=vp::Registration;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
struct Fake {
 std::uint32_t id=0,stage=7;const char* name="E_OC2";float x=0,y=0,z=0;int room=3;int health=220;std::int16_t f560=220;int action=vp::A_WAIT;
 std::uint8_t atp[2]{2,2};bool alive=true,stuck1=false;unsigned writes=0;
};
struct FakeIo {
 Fake* f;
 std::uint8_t get(unsigned i) const {return f->atp[i];}
 void set(unsigned i,std::uint8_t v){if(i==1&&f->stuck1)return;f->atp[i]=v;++f->writes;}
};
struct Log {Ev kind;std::uint32_t id;std::uint8_t from,to;const char* reason;std::uint32_t companion;};
struct World {
 vp::Registry reg;devf_dyn::Ledger led;std::uint32_t seq=0;std::vector<std::unique_ptr<Fake>> actors;std::vector<Log> log;std::vector<devf_dyn::Admit> admits;
 Fake* add(std::uint32_t id,const char* name,float x,float y,float z,int room=3){
  actors.push_back(std::make_unique<Fake>());Fake* f=actors.back().get();f->id=id;f->name=name;f->x=x;f->y=y;f->z=z;f->room=room;
  const bool oc=!std::strcmp(name,"E_OC");f->health=f->f560=oc?40:220;f->atp[0]=f->atp[1]=oc?1:2;return f;}
 Fake* by_id(std::uint32_t id){for(auto& a:actors)if(a->id==id&&a->alive)return a.get();return nullptr;}
 devf_dyn::Entry* dyn(Fake* f){auto* d=led.find(f->id);return d&&d->identity==f?d:nullptr;}
 void pre(Fake* f,R r=R::None,int black_hp=0,bool initializing=false){
  vp::Entry* e=reg.begin(f->id,f,initializing);if(!e)return;
  if(!e->classified){
   const auto plan=vp::make_plan(f->name,f->f560,f->health,initializing,r,black_hp);vp::adopt(*e,plan);
   if(plan.variant!=Variant::None){
    if(plan.write_hp)f->health=plan.initial_hp;
    if(plan.source==vp::Source::DevfBlack)f->health=black_hp;               // HP_APPLIED (CP10.2) happens before anything is captured
    FakeIo io{f};admits.push_back(devf_dyn::admit(led,f->id,f,e->generation,e->variant,f->name,io));
   }
  }
  if(auto* d=dyn(f))devf_dyn::capture_pre(*d,f->health);
 }
 void post(Fake* f){
  vp::Entry* e=reg.end(f->id,f);if(!e)return;
  const int before=f->health;++seq;                                         // health right after the native execute, BEFORE regeneration
  if(auto* d=dyn(f)){
   FakeIo io{f};devf_dyn::Post in;in.post_health=before;in.action=f->action;in.room=f->room;in.x=f->x;in.y=f->y;in.z=f->z;in.stage=f->stage;in.seq=seq;in.window=devf_dyn::window_for(reg.used);
   const auto ev=devf_dyn::post_update(led,*d,io,in,[&](const devf_dyn::Entry& c){Fake* o=by_id(c.id);return o&&o==c.identity;});
   if(ev.kind!=Ev::None)log.push_back({ev.kind,f->id,ev.from,ev.to,ev.reason,ev.companion});
  }
  const auto st=vp::step(*e,std::int16_t(before),f->action);f->health=st.new_health;
 }
 void create(Fake* f,R r=R::None,int black_hp=0){pre(f,r,black_hp,true);post(f);}
 void tick(const std::vector<std::pair<Fake*,int>>& hits={}){
  for(auto& a:actors){
   if(!a->alive)continue;
   Fake* f=a.get();pre(f);
   for(auto& h:hits)if(h.first==f&&h.second){f->health-=h.second;if(f->health<=0){f->health=0;f->action=vp::A_DEATH;}else f->action=vp::A_DAMAGE;}
   post(f);if(f->action==vp::A_DAMAGE)f->action=vp::A_WAIT;
  }
 }
 void run(unsigned n){for(unsigned i=0;i<n;++i)tick();}
 void destroy(Fake* f){f->alive=false;if(auto* e=reg.find(f->id))reg.release(*e);if(auto* d=led.find(f->id))led.release(*d);}   // regen_poll / stale path
 unsigned count(Ev k,std::uint32_t id=0){unsigned n=0;for(auto& l:log)if(l.kind==k&&(!id||l.id==id))++n;return n;}
};
bool eq(const Fake* f){return f->atp[0]==f->atp[1];}
bool lone(World& w,Fake* b){(void)w;return b->atp[0]==1&&b->atp[1]==1;}
int main(){
 static_assert(devf_dyn::kFuryTicks==300&&devf_dyn::kGroupRadiusXZ==650.0&&devf_dyn::kGroupRadiusY==250.0,"constants");
 // ===================== BLUE: companion bonus =====================
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* d=w.add(2,"E_OC2",5000,0,0);w.create(b);w.create(d,R::Normal);
  w.run(100);require(lone(w,b)&&b->writes==0,"native E_OC alone: 1, never written");require(d->atp[0]==1&&d->atp[1]==1&&d->writes==2,"DEVF Normal alone: 2->1 once");require(w.log.empty(),"no events when alone");
  pass("blue_alone_is_1_for_native_E_OC_and_DEVF_Normal");}
 {for(int blue=0;blue<2;++blue)for(int kind=0;kind<4;++kind){World w;Fake* b=blue?w.add(1,"E_OC2",0,0,0):w.add(1,"E_OC",0,0,0);w.create(b,blue?R::Normal:R::None);
   Fake* c=kind==0?w.add(2,"E_OC",100,0,0):w.add(2,"E_OC2",100,0,0);
   w.create(c,kind==2?R::Normal:kind==3?R::Black:R::None,kind==3?440:0);w.run(5);
   require(b->atp[0]==2&&b->atp[1]==2&&w.count(Ev::BlueOn,1)==1,"ON with a live companion of any kind");
   const unsigned expect=kind==0?2u:kind==1?2u:kind==2?2u:3u;   // a Blue companion evaluates ITSELF (it also has a companion); Red keeps 2, Black keeps 3
   require(c->atp[0]==expect&&c->atp[1]==expect,"the companion keeps its own rule (Blue never buffs Red or Black)");}
  pass("blue_native_and_DEVF_Normal_go_to_2_with_blue_red_or_black_companion_red_black_unaffected");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);w.create(b);for(unsigned i=0;i<5;++i)w.create(w.add(10+i,i%2?"E_OC":"E_OC2",50.f*(i+1),0,0));w.run(50);
  require(b->atp[0]==2&&b->atp[1]==2&&w.count(Ev::BlueOn,1)==1&&b->writes==2,"five companions: still 2, a single transition");
  for(unsigned i=0;i<4;++i){w.destroy(w.by_id(10+i));}
  w.run(10);require(b->atp[0]==2,"one companion left");w.destroy(w.by_id(14));w.run(10);require(lone(w,b)&&w.count(Ev::BlueOff,1)==1,"last one gone: 1");
  pass("two_or_five_companions_never_stack_last_one_leaving_restores_1");}
 {struct C{float dx,dy,dz;bool on;const char* what;};
  const C t[]{{649,0,0,true,"649"},{650,0,0,true,"650 inclusive"},{651,0,0,false,"651"},{0,0,650,true,"z 650"},{0,0,651,false,"z 651"},
   {0,249,0,true,"y 249"},{0,250,0,true,"y 250 inclusive"},{0,251,0,false,"y 251"},{0,-250,0,true,"y -250"},{0,-251,0,false,"y -251"},
   {390,0,520,true,"3-4-5 diagonal exactly 650"},{391,0,520,false,"diagonal just over"},{-390,0,-520,true,"negative diagonal exactly 650"},
   {460,0,460,false,"Chebyshev would accept (460,460) but euclidean is 650.54"},{459,0,459,true,"euclidean 649.1"},{650,250,0,true,"both limits at once"},{650,251,0,false,"y over with xz at limit"},{651,250,0,false,"xz over with y at limit"}};
  for(const auto& c:t){World w;Fake* b=w.add(1,"E_OC",1000,2000,3000);Fake* o=w.add(2,"E_OC2",1000+c.dx,2000+c.dy,3000+c.dz);w.create(b);w.create(o);w.run(3);
   require((b->atp[0]==2)==c.on&&eq(b),c.what);}
  {const double d=devf_dyn::sqrt_d(422500.0);require(d>649.999999&&d<650.000001&&devf_dyn::sqrt_d(0)==0&&devf_dyn::abs_d(-3.0)==3.0,"telemetry helpers without CRT");}
  pass("radius_650_xz_euclidean_250_y_inclusive_boundaries_exact");}
 {{World w;Fake* b=w.add(1,"E_OC",0,0,0);w.create(b);w.run(20);require(lone(w,b),"self never counts");}
  {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0,5);w.create(b);w.create(o);w.run(5);require(lone(w,b),"other room");}
  {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0);o->stage=99;w.create(b);w.create(o);w.run(5);require(lone(w,b),"other stage");}
  {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0);o->f560=300;w.create(b);w.create(o);w.run(5);require(lone(w,b)&&w.led.find(2)==nullptr,"actor rejected by CP10.4 is neither modified nor a companion");require(o->atp[0]==2&&o->writes==0,"rejected actor colliders untouched");}
  {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0);o->atp[1]=7;w.create(b);w.create(o);w.run(5);
   require(w.dyn(o)&&!w.dyn(o)->owned&&b->atp[0]==2,"pre-modified spheres: not owned, never written, but a real Bokoblin that still counts as companion");require(o->atp[0]==2&&o->atp[1]==7&&o->writes==0,"untouched");}
  {World w;Fake* b=w.add(1,"E_OC",0,0,0);w.create(b);const unsigned seq0=w.seq;w.run(3);require(w.seq==seq0+3&&lone(w,b),"no registry entry besides self -> nothing");}
  pass("exclusions_self_room_stage_rejected_and_unowned_candidates");}
 {for(int st:{vp::A_DEATH,vp::A_WATER_DEATH,vp::A_FALL_DEAD,vp::A_FALL,vp::A_DEMO_MASTER,vp::A_DEMO_CHILD}){World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0);w.create(b);w.create(o);w.run(3);require(b->atp[0]==2,"on");
   o->action=st;w.run(2);require(lone(w,b),"terminal companion state: off");o->action=vp::A_WAIT;w.run(2);require(b->atp[0]==2,"recovers when no longer terminal");}
  {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0);w.create(b);w.create(o);w.run(3);w.tick({{o,1000}});w.run(2);require(o->health==0&&lone(w,b)&&w.count(Ev::BlueOff,1)==1,"companion killed: off");}
  pass("dead_dying_falling_water_and_demo_companions_do_not_count");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",900,0,0);w.create(b);w.create(o);w.run(3);require(lone(w,b),"far");
  o->x=640;w.run(2);require(b->atp[0]==2,"arrives");o->x=700;w.run(2);require(lone(w,b),"leaves");o->x=100;w.run(2);require(b->atp[0]==2,"returns");
  o->room=8;w.run(2);require(lone(w,b),"changes room");o->room=3;w.run(2);require(b->atp[0]==2,"back in room");
  w.destroy(o);w.run(5);require(lone(w,b),"destroyed");Fake* n=w.add(2,"E_OC2",50,0,0);w.create(n);w.run(2);require(b->atp[0]==2,"respawn with the same id: counts as a fresh companion");
  w.destroy(n);w.run(2);require(lone(w,b)&&w.count(Ev::BlueOn,1)==4&&w.count(Ev::BlueOff,1)==4,"never stuck at 2");
  pass("companion_arrives_leaves_dies_respawns_changes_room_blue_follows_without_accumulating");}
 {// stale record: the companion vanished WITHOUT its record being released (poll has not run): freshness window + authoritative verify
  World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",10,0,0);w.create(b);w.create(o);w.run(3);require(b->atp[0]==2,"on");
  o->alive=false;                                  // no longer executes, record still in the ledger
  const unsigned window=devf_dyn::window_for(w.reg.used);w.run(window+2);require(lone(w,b),"stale companion expires by freshness: Blue back to 1 without waiting for any poll");
  World v;Fake* b2=v.add(1,"E_OC",0,0,0);Fake* o2=v.add(2,"E_OC2",10,0,0);v.create(b2);v.create(o2);
  // a record that looks fresh but whose actor no longer exists: ON must be refused by the authoritative check
  o2->alive=false;v.tick();require(lone(v,b2)&&v.count(Ev::BlueOn)==0&&v.led.find(2)&&!v.led.find(2)->snap,"authoritative verification refuses a vanished actor at the ON transition and drops its snapshot");
  pass("stale_or_vanished_companion_never_keeps_or_turns_the_bonus_on");}
 {World w;Fake* a=w.add(1,"E_OC",0,0,0);Fake* b=w.add(2,"E_OC",100,0,0);w.create(a);w.create(b);w.run(3);require(a->atp[0]==2&&b->atp[0]==2,"two Blues enable each other");
  w.tick({{b,100000}});w.run(2);require(lone(w,a)&&b->action==vp::A_DEATH,"one dies: the other falls back");require(b->atp[0]==1,"the dying Blue itself returns to base");
  pass("two_blues_enable_each_other_and_both_fall_back_when_one_dies");}
 {World w;Fake* r=w.add(1,"E_OC2",0,0,0);Fake* k=w.add(2,"E_OC2",50,0,0);Fake* b=w.add(3,"E_OC",90,0,0);w.create(r);w.create(k,R::Black,440);w.create(b);w.run(300);
  require(r->atp[0]==2&&r->writes==0&&k->atp[0]==3&&k->writes==2&&b->atp[0]==2,"Blue 2; Red untouched 2; Black base 3");
  pass("blue_ability_never_alters_red_or_black_red_counts_as_companion");}
 // ===================== BLACK: fury =====================
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);require(k->health==440&&k->atp[0]==3&&k->atp[1]==3,"Black admitted at 440 and base 3");w.run(1000);require(k->atp[0]==3&&w.log.empty()&&k->writes==2,"base 3 forever without damage");
  pass("black_base_3_without_damage_no_events_no_writes");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(10);
  w.tick({{k,30}});   // tick 0: the POST that observes the loss
  require(k->atp[0]==6&&k->atp[1]==6&&w.count(Ev::FuryStart,1)==1,"tick 0: 6");
  std::vector<unsigned> v;for(int i=1;i<=305;++i){w.tick();v.push_back(k->atp[0]);require(eq(k),"spheres equal");}
  require(v[0]==6,"tick 1: 6");require(v[298]==6,"tick 299: still 6 (remaining 1)");require(v[299]==3,"tick 300: 3");require(v[300]==3&&v[301]==3,"tick 301: 3");
  unsigned sixes=1;for(auto x:v)if(x==6)++sixes;require(sixes==300,"exactly 300 ticks at 6 counting the hit tick");
  require(w.count(Ev::FuryEnd,1)==1&&w.count(Ev::FuryRefresh,1)==0,"one start, one end");
  pass("black_fury_exactly_300_ticks_boundaries_0_1_299_300_301");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});unsigned six=1;
  for(int i=0;i<100;++i){w.tick();if(k->atp[0]==6)++six;}w.tick({{k,10}});++six;require(k->atp[0]==6&&w.count(Ev::FuryRefresh,1)==1,"renewed");
  unsigned maxv=0;for(int i=0;i<400;++i){w.tick();if(k->atp[0]==6)++six;if(k->atp[0]>maxv)maxv=k->atp[0];}
  require(maxv<=6&&k->atp[0]==3,"never above 6 and back to 3");require(six==1+100+1+299,"renewal restarts at 300: 1+100 ticks, then 300 more including the new hit tick");
  require(w.count(Ev::FuryStart,1)==1&&w.count(Ev::FuryEnd,1)==1,"no stacking: one start, one end");
  pass("black_second_hit_renews_to_300_never_600_never_12");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,5}});for(int i=0;i<299;++i)w.tick();require(k->atp[0]==6,"tick 299 still 6");
  w.tick({{k,5}});require(k->atp[0]==6&&w.count(Ev::FuryRefresh,1)==1&&w.count(Ev::FuryEnd,1)==0,"a hit in the would-be expiry tick renews without a gap");
  for(int i=0;i<299;++i){w.tick();}
  require(k->atp[0]==6,"and lasts 300 more");w.tick();require(k->atp[0]==3,"ends");
  pass("black_hit_on_the_expiry_tick_renews_without_gap");}
 {// HP initialisation, regeneration, stagger and lethal hits never start the fury
  for(int cfg:{440,150}){World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,cfg);w.run(30);require(w.count(Ev::FuryStart)==0&&k->atp[0]==3,"220->440 / 220->150 is not a hit");}
  {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,100}});w.run(300);require(w.count(Ev::FuryStart,1)==1,"only the real hit");
   const unsigned starts=w.count(Ev::FuryStart,1);w.run(300);require(w.count(Ev::FuryStart,1)==starts&&k->atp[0]==3&&k->health>300,"regeneration (4 HP/s) is never a hit");}
  {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);k->action=vp::A_DAMAGE;w.tick();k->action=vp::A_BIG_DAMAGE;w.tick();w.run(5);require(w.count(Ev::FuryStart)==0&&k->atp[0]==3,"stagger/guard reaction without HP loss");}
  {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,150);w.tick({{k,150}});require(k->health==0&&w.count(Ev::FuryStart)==0&&k->atp[0]==3,"lethal hit: no fury");}
  {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,10}});require(k->atp[0]==6,"fury");w.tick({{k,100000}});require(k->atp[0]==3&&w.count(Ev::FuryEnd,1)==1&&w.count(Ev::FuryRefresh)==0,"lethal blow during fury ends it: no 6 left on a dying actor");}
  pass("hp_init_regen_stagger_and_lethal_hits_never_start_fury_lethal_ends_it");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,10},{k,10},{k,10}});require(k->atp[0]==6&&w.count(Ev::FuryStart,1)==1&&k->health==410+0,"three contacts in one native execute");w.run(299);require(k->atp[0]==6,"still 6");w.tick();require(k->atp[0]==3,"one 300-tick window only");
  pass("several_contacts_in_one_execute_are_one_trigger");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,10}});
  const unsigned seq=w.seq;for(int wall=0;wall<10000;++wall){/* pause / loading screen / event stop: the actor executes zero times */}
  require(w.seq==seq&&k->atp[0]==6,"nothing consumed");
  for(int i=0;i<100;++i){/* render at 144 fps: more draws than sim ticks, still one tick per completed pair */w.tick();}
  require(k->atp[0]==6,"100 ticks");for(int i=0;i<199;++i){w.tick();}
  require(k->atp[0]==6,"tick 299");w.tick();require(k->atp[0]==3,"tick 300");
  pass("pause_and_render_rate_do_not_consume_or_grant_fury_time");}
 {// PRE/POST pairing: a lost POST or a reentrant PRE never counts a tick
  World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,10}});
  for(int i=0;i<500;++i){w.pre(k);/* POST lost */ w.pre(k);/* reentrant PRE: refused */ }
  require(k->atp[0]==6,"unpaired PRE do not consume ticks");
  w.post(k);w.post(k);require(k->atp[0]==6,"POST without a pending PRE is ignored");
  pass("unpaired_or_reentrant_pre_post_never_count_a_tick");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});require(k->atp[0]==6,"fury on");
  w.destroy(k);Fake* n=w.add(1,"E_OC2",0,0,0);w.create(n,R::Black,440);require(n->atp[0]==3&&w.dyn(n)->fury==0&&w.count(Ev::FuryStart,1)==1,"new generation of id 1 starts at base 3, nothing inherited");w.run(10);require(n->atp[0]==3,"and stays");
  pass("respawn_and_id_reuse_never_inherit_fury");}
 {for(int st:{vp::A_DEATH,vp::A_WATER_DEATH,vp::A_FALL_DEAD,vp::A_FALL,vp::A_DEMO_MASTER,vp::A_DEMO_CHILD}){World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,10}});require(k->atp[0]==6,"on");k->action=st;w.run(1);require(k->atp[0]==3&&w.dyn(k)->fury==0,"terminal state ends fury and releases the counter");k->action=vp::A_WAIT;w.run(400);require(k->atp[0]==3&&w.count(Ev::FuryStart,1)==1,"does not come back");}
  pass("death_fall_water_demo_end_fury_without_counter_leak");}
 {// detection order: a hit followed by regeneration in the same tick, regeneration alone, and the next tick's PRE baseline
  World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.tick({{k,100}});w.run(299);w.tick();require(k->atp[0]==3,"fury over");
  k->health=300;w.run(1);k->health=300;w.tick({{k,1}});require(k->atp[0]==6,"a 1 HP loss is a hit even though regeneration may give it back in the same tick (read before step())");
  World v;Fake* k2=v.add(1,"E_OC2",0,0,0);v.create(k2,R::Black,440);v.tick({{k2,50}});for(int i=0;i<300;++i)v.tick();const unsigned s=v.count(Ev::FuryStart,1);for(int i=0;i<600;++i)v.tick();require(v.count(Ev::FuryStart,1)==s,"600 ticks of regeneration add no start");
  pass("damage_is_read_before_regeneration_and_regeneration_never_looks_like_damage");}
 // ===================== OWNERSHIP / INTEGRITY =====================
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* d=w.add(2,"E_OC2",100,0,0);Fake* r=w.add(3,"E_OC2",5000,0,0);Fake* k=w.add(4,"E_OC2",9000,0,0);
  w.create(b);w.create(d,R::Normal);w.create(r);w.create(k,R::Black,440);
  for(int i=0;i<1500;++i){w.tick(i%300==5?std::vector<std::pair<Fake*,int>>{{k,3}}:std::vector<std::pair<Fake*,int>>{});if(i==600)d->x=4000;if(i==900)d->x=100;
   for(Fake* f:{b,d,r,k})require(eq(f),"both spheres always identical");
   require(b->atp[0]==1||b->atp[0]==2,"Blue only 1/2");require(d->atp[0]==1||d->atp[0]==2,"DEVF Normal only 1/2");require(r->atp[0]==2,"Red always 2");require(k->atp[0]==3||k->atp[0]==6,"Black only 3/6");}
  pass("spheres_equal_and_values_confined_to_1_2_2_3_6_never_4_9_12");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);w.create(b);w.create(o);w.run(3);const unsigned w0=b->writes,wo=o->writes;w.run(2000);require(b->writes==w0&&o->writes==wo,"steady state: zero writes per tick");
  o->x=5000;w.run(2);o->x=100;w.run(2);require(b->writes==w0+4,"two transitions = two pairs of writes, nothing else");
  pass("writes_only_on_a_change_of_target_never_per_frame");}
 {for(int which=0;which<3;++which){World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",5000,0,0);w.create(b);w.create(o);w.run(3);
   if(which==0){b->atp[0]=9;}
   if(which==1){b->atp[1]=9;}
   if(which==2){b->atp[0]=5;b->atp[1]=6;}
   const std::uint8_t a0=b->atp[0],a1=b->atp[1];o->x=100;w.run(3);
   require(b->atp[0]==a0&&b->atp[1]==a1&&w.count(Ev::Rejected,1)==1&&!w.dyn(b)->owned,"third party value kept, one rejection, ownership dropped");const unsigned wr=b->writes;o->x=5000;w.run(5);o->x=100;w.run(5);require(b->writes==wr&&w.count(Ev::Rejected,1)==1,"no further attempts, no log spam");}
  {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(3);k->atp[1]=4;w.tick({{k,10}});require(k->atp[0]==3&&k->atp[1]==4&&w.count(Ev::Rejected,1)==1&&w.count(Ev::FuryStart,1)==0,"Black: third-party sphere not overwritten, fury not started");}
  pass("third_party_change_is_never_overwritten_and_stops_management_of_that_generation");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",5000,0,0);w.create(b);w.create(o);w.run(3);b->stuck1=true;o->x=100;w.run(3);
  require(b->atp[0]==1&&b->atp[1]==1&&!w.dyn(b)->owned&&w.count(Ev::Rejected,1)==1,"partial write: sphere 0 put back, ownership dropped");
  {World v;Fake* d=v.add(1,"E_OC2",0,0,0);d->stuck1=true;v.create(d,R::Normal);require(!v.admits.back().plan.accepted&&v.admits.back().plan.why==devf_damage::Why::ReadbackMismatch&&d->atp[0]==2&&d->atp[1]==2,"admission readback failure restores native");}
  pass("partial_write_failures_restore_and_drop_ownership");}
 {World w;Fake* o=w.add(1,"E_OC2",0,0,0);o->atp[0]=o->atp[1]=9;w.create(o);require(!w.dyn(o)->owned&&o->atp[0]==9&&w.admits.back().plan.why==devf_damage::Why::NativeAtpMismatch,"non pristine");
  Fake* p=w.add(2,"E_OC2",5000,0,0);w.create(p,R::Black,440);require(w.dyn(p)->owned,"normal Black still owned");
  World f;std::vector<Fake*> all;for(unsigned i=0;i<devf_dyn::kCapacity;++i){Fake* x=f.add(100+i,"E_OC2",100000.f*i,0,0);f.create(x);all.push_back(x);}
  Fake* extra=f.add(999,"E_OC",0,0,0);const auto before=f.reg.used;f.create(extra);require(f.reg.used==before&&f.led.find(999)==nullptr&&extra->atp[0]==1&&extra->writes==0,"table full: refused, untouched");
  pass("pristine_check_and_bounded_ledger_fail_closed");}
 // ===================== SHUTDOWN / RESTORE =====================
 {auto restore=[&](devf_dyn::Entry e,bool live,std::uint8_t a,std::uint8_t b){return devf_dyn::restore_plan(e,live,a,b);};
  struct Row{const char* name;Variant v;std::uint8_t native,applied;bool write;};
  const Row rows[]{{"native Blue alone",Variant::Blue,1,1,false},{"native Blue grouped",Variant::Blue,1,2,true},{"DEVF Normal alone",Variant::Blue,2,1,true},{"DEVF Normal grouped",Variant::Blue,2,2,false},
   {"Red",Variant::Red,2,2,false},{"Black base",Variant::Black,2,3,true},{"Black fury",Variant::Black,2,6,true}};
  for(const auto& r:rows){devf_dyn::Entry e;e.used=true;e.owned=true;e.variant=r.v;e.native=r.native;e.applied=r.applied;
   const auto p=restore(e,true,r.applied,r.applied);require(p.write==r.write,r.name);
   if(r.write){require(!restore(e,true,r.applied,std::uint8_t(r.applied+1)).write&&!restore(e,true,std::uint8_t(r.applied+1),r.applied).write,"one sphere changed by others");require(!restore(e,false,r.applied,r.applied).write,"gone/replaced");}
   e.owned=false;require(!restore(e,true,r.applied,r.applied).write,"not owned");}
  pass("shutdown_restore_matrix_1_2_native_only_when_live_same_and_still_ours");}
 {// end to end: the values on live actors at the moment of unload are restored from the LEDGER state (applied), never from the fixed base
  World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);Fake* d=w.add(3,"E_OC2",5000,0,0);Fake* k=w.add(4,"E_OC2",9000,0,0);
  w.create(b);w.create(o,R::Normal);w.create(d,R::Normal);w.create(k,R::Black,440);w.tick({{k,10}});w.run(3);
  require(b->atp[0]==2&&o->atp[0]==2&&d->atp[0]==1&&k->atp[0]==6,"state at unload: Blue grouped 2, Normal grouped 2, Normal alone 1, Black fury 6");
  for(Fake* f:{b,o,d,k}){auto* e=w.dyn(f);const auto p=devf_dyn::restore_plan(*e,true,f->atp[0],f->atp[1]);if(p.write){f->atp[0]=f->atp[1]=e->native;}}
  require(b->atp[0]==1&&b->atp[1]==1&&o->atp[0]==2&&o->atp[1]==2&&d->atp[0]==2&&d->atp[1]==2&&k->atp[0]==2&&k->atp[1]==2,"all back to native: E_OC 1, E_OC2 2");
  pass("unload_with_live_dynamic_values_restores_native_not_the_old_fixed_base");}
 std::printf("DYNAMIC_DAMAGE_LAB_PASS cases=%u POLICY_ONLY; LINK_HEART_LOSS_NOT_MEASURED; NATIVE_HOOKS_AND_GAME_NOT_EXECUTED\n",cases);
}
