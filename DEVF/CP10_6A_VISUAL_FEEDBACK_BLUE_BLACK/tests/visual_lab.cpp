// CP10.6A offline lab: visual feedback (aura + Black eyes) driven by the approved CP10.6 logic. Pure logic, no game, no GPU, no pixels.
// The world runs the REAL CP10.4 identity/regeneration, CP10.5/6 admission and dynamic POST code (devf_dyn) plus the REAL devf_vis state
// machine against fake effects that model the engine contract: an emitter that is not refreshed during a tick is removed by the engine,
// and a texture swap only succeeds on an eye packet that still holds one of our textures. It proves decisions, timing, ownership and
// cleanup; it does NOT prove how anything looks (human Windows round).
#include "../expansion/visual_policy.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
namespace vp=devf_variant;
namespace vv=devf_vis;
using vp::Variant;
using R=vp::Registration;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
struct Fake {
 std::uint32_t id=0,stage=7;const char* name="E_OC2";float x=0,y=0,z=0;int room=3;int health=220;std::int16_t f560=220;int action=vp::A_WAIT;
 std::uint8_t atp[2]{2,2};bool alive=true;
 unsigned eye_tex=0;bool eye_foreign=false;unsigned eye_writes=0;   // eye packet: 0 = native, 1..7 = our variants, foreign = somebody else's texture
};
struct FakeIo {Fake* f;std::uint8_t get(unsigned i)const{return f->atp[i];}void set(unsigned i,std::uint8_t v){f->atp[i]=v;}};
struct Emitter {bool alive=false,refreshed=false;int alpha=0;unsigned refreshes=0;std::uint32_t owner=0;};
struct FakeFx {
 Fake* cur=nullptr;std::vector<Emitter> ems{Emitter{}};std::uint32_t next_key=1;bool fail_aura=false;const char* why="NONE";
 unsigned creates=0,releases=0,eye_calls=0,orphans_expired=0;
 bool aura(vv::Entry& e,int level,int alpha){
  (void)level;if(fail_aura){why="AURA_RESOURCE_UNAVAILABLE";return false;}
  if(!e.aura_live){e.key=next_key++;if(ems.size()<=e.key)ems.resize(e.key+1);ems[e.key].alive=true;ems[e.key].owner=e.id;++creates;}
  else if(!ems[e.key].alive){why="EMITTER_HANDLE_EXPIRED";return false;}
  ems[e.key].refreshed=true;ems[e.key].alpha=alpha;++ems[e.key].refreshes;return true;
 }
 void aura_release(vv::Entry& e){if(e.key&&e.key<ems.size()){ems[e.key].alpha=0;}++releases;e.key=0;}   // stop drawing + forget; the engine cuts it when nobody refreshes it
 bool eye(vv::Entry& e,unsigned idx){
  (void)e;++eye_calls;if(cur->eye_foreign){why="EYE_PACKET_TEXTURE_NOT_OURS";return false;}
  cur->eye_tex=idx;++cur->eye_writes;return true;
 }
 const char* reason()const{return why;}
 void engine_tick(){for(auto& m:ems){if(m.alive&&!m.refreshed){m.alive=false;++orphans_expired;}m.refreshed=false;}}
 unsigned live()const{unsigned n=0;for(auto& m:ems)if(m.alive)++n;return n;}
};
struct Log {vv::Ev kind;std::uint32_t id;const char* reason;int level,alpha;unsigned eye;};
struct World {
 vp::Registry reg;devf_dyn::Ledger led;vv::Ledger vled;FakeFx fx;std::uint32_t seq=0;std::vector<std::unique_ptr<Fake>> actors;std::vector<Log> log;
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
    if(plan.source==vp::Source::DevfBlack)f->health=black_hp;
    FakeIo io{f};devf_dyn::admit(led,f->id,f,e->generation,e->variant,f->name,io);
   }
  }
  if(auto* d=dyn(f))devf_dyn::capture_pre(*d,f->health);
 }
 void vis_post(Fake* f,devf_dyn::Entry* d){
  const bool aura=(d->variant==Variant::Blue&&d->group)||(d->variant==Variant::Black&&d->fury>0);
  const bool eyes=d->variant==Variant::Black&&d->fury>0;
  auto* v=vled.find(f->id);
  if(v&&v->identity!=f){vled.release(*v);v=nullptr;}
  if(!v){if(!aura&&!eyes)return;v=vled.record(f->id,f,d->generation,d->variant);if(!v)return;}
  fx.cur=f;const auto ev=vv::step(vled,*v,fx,aura,eyes);
  for(unsigned i=0;i<ev.n;++i)log.push_back({ev.ev[i].kind,f->id,ev.ev[i].reason,ev.ev[i].level,ev.ev[i].alpha,ev.ev[i].eye_idx});
 }
 void post(Fake* f){
  vp::Entry* e=reg.end(f->id,f);if(!e)return;
  const int before=f->health;++seq;
  if(auto* d=dyn(f)){
   FakeIo io{f};devf_dyn::Post in;in.post_health=before;in.action=f->action;in.room=f->room;in.x=f->x;in.y=f->y;in.z=f->z;in.stage=f->stage;in.seq=seq;in.window=devf_dyn::window_for(reg.used);
   devf_dyn::post_update(led,*d,io,in,[&](const devf_dyn::Entry& c){Fake* o=by_id(c.id);return o&&o==c.identity;});
   vis_post(f,d);
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
  fx.engine_tick();
 }
 void run(unsigned n){for(unsigned i=0;i<n;++i)tick();}
 void destroy(Fake* f){f->alive=false;if(auto* e=reg.find(f->id))reg.release(*e);if(auto* d=led.find(f->id))led.release(*d);if(auto* v=vled.find(f->id))vled.release(*v);}   // regen_poll / stale path
 unsigned count(vv::Ev k,std::uint32_t id=0){unsigned n=0;for(auto& l:log)if(l.kind==k&&(!id||l.id==id))++n;return n;}
 int aura_level(Fake* f){auto* v=vled.find(f->id);return v?v->aura_level:0;}
 int alpha_of(Fake* f){auto* v=vled.find(f->id);return v&&v->key&&v->key<fx.ems.size()?fx.ems[v->key].alpha:0;}
};
int main(){
 using Ev=vv::Ev;
 // ===================== pure state machine =====================
 {require(vv::next_level(0,true)==10&&vv::next_level(95,true)==100&&vv::next_level(100,true)==100&&vv::next_level(100,false)==93&&vv::next_level(5,false)==0&&vv::next_level(0,false)==0,"steps and clamps");
  int l=0;unsigned up=0;while(l<100){l=vv::next_level(l,true);++up;}require(up==10,"10 ticks to fade in");
  unsigned down=0;while(l>0){l=vv::next_level(l,false);++down;}require(down==15,"15 ticks to fade out");
  require(vv::state_of(0,false)==vv::State::Inactive&&vv::state_of(40,true)==vv::State::FadingIn&&vv::state_of(100,true)==vv::State::Active&&vv::state_of(40,false)==vv::State::FadingOut&&vv::state_of(0,true)==vv::State::FadingIn,"state mapping");
  pass("fade_10_ticks_in_15_ticks_out_with_four_states");}
 {require(vv::eye_index(0)==0&&vv::eye_index(100)==7&&vv::eye_index(-5)==0&&vv::eye_index(500)==7,"ends");unsigned last=0;for(int l=0;l<=100;++l){const unsigned k=vv::eye_index(l);require(k>=last&&k<=7&&k-last<=1,"monotonic, one step at a time");last=k;}
  require(vv::alpha_for(Variant::Blue,0)==0&&vv::alpha_for(Variant::Blue,100)==110&&vv::alpha_for(Variant::Black,100)==130&&vv::alpha_for(Variant::Black,100)>vv::alpha_for(Variant::Blue,100),"alpha caps: Black a little stronger, both subtle");
  for(int l=1;l<=100;++l)require(vv::alpha_for(Variant::Black,l)>=vv::alpha_for(Variant::Black,l-1)&&vv::alpha_for(Variant::Black,l)<=130,"alpha monotonic and capped");
  pass("eye_index_and_alpha_ramps_monotonic_and_capped");}
 // ===================== BLUE =====================
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",5000,0,0);w.create(b);w.create(o);w.run(5);
  require(w.vled.used==0&&w.fx.creates==0&&w.fx.live()==0,"alone: no visual, no emitter");
  o->x=100;w.run(2);   // companion arrives: BLUE_GROUP_ON this tick or the next
  unsigned first=0;for(unsigned t=0;t<40&&!first;++t){w.tick();if(w.count(Ev::AuraOn,1))first=t+1;}
  require(first>=1&&w.count(Ev::AuraOn,1)==1&&w.fx.live()==1,"aura on once");
  const int l0=w.aura_level(b);require(l0>0&&l0<=100,"already fading in on the tick the logic switched");
  w.run(12);require(w.aura_level(b)==100&&w.alpha_of(b)==110,"fully active at the Blue alpha cap");
  w.run(100);require(w.count(Ev::AuraOn,1)==1&&w.fx.creates==1&&w.fx.eye_calls==0,"steady: nothing new, never any eye call for Blue");
  pass("blue_aura_fades_in_when_group_bonus_turns_on_never_touches_eyes");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);w.create(b);w.create(o);w.run(30);require(w.aura_level(b)==100,"on");
  o->x=5000;unsigned ticks=0;while(w.vled.find(1)&&ticks<80){w.tick();++ticks;}
  require(!w.vled.find(1)||w.vled.find(1)->aura_level==0,"faded out");require(w.count(Ev::AuraOff,1)==1&&w.fx.live()==0&&w.fx.releases==1,"released once, emitter gone");
  w.run(3);require(w.fx.orphans_expired>=1&&w.fx.live()==0,"the engine removed the emitter nobody refreshes");
  pass("blue_aura_fades_out_15_ticks_after_group_off_and_leaves_no_emitter");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);w.create(b);w.create(o);w.run(30);
  o->x=5000;w.run(7);const int mid=w.aura_level(b);require(mid>0&&mid<100,"mid fade-out");const unsigned creates=w.fx.creates;
  o->x=100;w.run(2);   // reactivated during FADING_OUT
  require(w.aura_level(b)>mid-14&&w.fx.creates==creates,"continues from the current level, same emitter, no restart from zero");
  w.run(20);require(w.aura_level(b)==100&&w.fx.creates==creates&&w.count(Ev::AuraOff,1)==0,"back to active, never released in between");
  bool resumed=false;for(auto& l:w.log)if(l.kind==Ev::AuraOn&&l.id==1&&std::strstr(l.reason,"RESUMED"))resumed=true;require(resumed,"the resume is logged with its reason");
  pass("blue_reactivation_during_fade_out_resumes_smoothly");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);w.create(b);w.create(o);w.run(3);
  // border flicker: companion crossing the radius every tick
  int prev=w.aura_level(b);unsigned maxjump=0;for(int i=0;i<200;++i){o->x=(i%2)?5000.f:100.f;w.tick();const int l=w.aura_level(b);const unsigned d=unsigned(l>prev?l-prev:prev-l);if(d>maxjump)maxjump=d;prev=l;}
  require(maxjump<=10,"no jump larger than one fade step");require(w.fx.creates<=1,"the emitter is never recreated by flicker");pass("border_flicker_never_recreates_the_emitter_and_never_jumps");}
 {World w;Fake* r=w.add(1,"E_OC2",0,0,0);Fake* b=w.add(2,"E_OC",100,0,0);w.create(r);w.create(b);w.run(60);
  require(w.vled.find(1)==nullptr&&w.fx.eye_calls==0,"Red never has a visual record and never gets eye calls");require(r->eye_tex==0&&r->eye_writes==0,"Red untouched");
  require(w.vled.find(2)!=nullptr,"the Blue next to a Red does get the aura (Red counts as a companion)");pass("red_gets_no_visual_but_still_counts_as_blue_companion");}
 // ===================== BLACK =====================
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(1000);
  require(w.vled.used==0&&w.fx.creates==0&&w.fx.eye_calls==0&&k->eye_writes==0&&w.log.empty(),"HP init 220->440, 1000 ticks of regeneration: no visual at all");
  Fake* c=w.add(2,"E_OC2",5000,0,0);w.create(c,R::Black,150);w.run(300);require(w.vled.find(2)==nullptr,"220->150 neither");
  k->action=vp::A_DAMAGE;w.tick();k->action=vp::A_BIG_DAMAGE;w.tick();w.run(5);require(w.vled.used==0,"stagger/guard without HP loss neither");
  pass("black_visual_not_triggered_by_hp_init_regeneration_or_stagger");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,30}});
  require(w.count(Ev::AuraOn,1)==1&&w.aura_level(k)==10&&k->atp[0]==6,"aura starts the same tick as the logic (fury 6)");
  std::vector<unsigned> eyes;for(int i=0;i<12;++i){w.tick();eyes.push_back(k->eye_tex);}
  require(w.count(Ev::EyesOn,1)==1&&eyes.back()==7&&k->eye_tex==7,"eyes fully purple after the fade-in");for(size_t i=1;i<eyes.size();++i)require(eyes[i]>=eyes[i-1]&&eyes[i]-eyes[i-1]<=1,"eyes step through the variants, never jump");
  require(k->eye_writes==7,"seven texture swaps for a whole fade (only on change of variant)");require(w.alpha_of(k)==130,"aura at the Black cap");
  pass("black_fury_start_fades_in_purple_eyes_and_aura_together");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(20);const unsigned creates=w.fx.creates,writes=k->eye_writes;const unsigned ons=w.count(Ev::AuraOn,1);
  unsigned before_log=w.log.size();const unsigned refreshes=w.fx.ems[w.vled.find(1)->key].refreshes;
  for(int i=0;i<10;++i){w.run(20);w.tick({{k,5}});}                    // hit every ~21 ticks: BLACK_FURY_REFRESH ten times
  require(w.count(Ev::AuraOn,1)==ons&&w.fx.creates==creates&&k->eye_writes==writes&&k->eye_tex==7,"refreshes never restart, flicker or recreate anything");
  require(w.log.size()==before_log,"and log nothing from the visual layer");require(w.aura_level(k)==100&&w.alpha_of(k)==130,"stable full level");
  const unsigned per_tick=w.fx.ems[w.vled.find(1)->key].refreshes-refreshes;require(per_tick==210,"the emitter is refreshed exactly once per tick (210 ticks) so the engine keeps it alive");
  pass("black_fury_refresh_keeps_the_visual_perfectly_stable");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});
  for(int i=0;i<299;++i){w.tick();}
  require(k->atp[0]==6&&w.aura_level(k)==100,"still in fury at tick 299");
  w.tick();require(k->atp[0]==3,"logic: fury over at tick 300");require(w.aura_level(k)<100&&w.aura_level(k)>0,"visual starts fading the same tick");
  std::vector<unsigned> eyes;for(int i=0;i<20;++i){w.tick();eyes.push_back(k->eye_tex);}
  for(size_t i=1;i<eyes.size();++i)require(eyes[i]<=eyes[i-1]&&eyes[i-1]-eyes[i]<=1,"eyes step back down smoothly");
  require(k->eye_tex==0,"back to native");
  w.run(5);require(k->eye_tex==0&&w.fx.live()==0&&w.count(Ev::EyesOff,1)==1&&w.count(Ev::AuraOff,1)==1,"eyes native, emitter gone, one OFF each");
  pass("black_fury_end_fades_eyes_and_aura_out_and_restores_the_native_eye_texture");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(30);   // fully on
  for(int i=0;i<300-31+8;++i){w.tick();}   // fury ends -> fading out
  const int mid=w.aura_level(k);require(mid>0&&mid<100,"fading out");w.tick({{k,5}});   // hit during FADING_OUT: the logic starts a NEW fury
  require(k->atp[0]==6&&w.aura_level(k)>mid-8,"the visual resumes from its current level");w.run(15);require(w.aura_level(k)==100&&k->eye_tex==7&&w.count(Ev::AuraOff,1)==0&&w.count(Ev::EyesOff,1)==0,"never fully released in between");
  pass("black_new_fury_during_fade_out_resumes_from_the_current_level");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(12);require(k->eye_tex==7&&w.fx.live()==1,"on");
  w.tick({{k,100000}});require(k->health==0&&k->atp[0]==3,"lethal: logic ended");w.run(25);
  require(k->eye_tex==0&&w.fx.live()==0&&w.count(Ev::EyesOff,1)==1&&w.count(Ev::AuraOff,1)==1,"a dying Black still executing fades out completely and releases everything");
  pass("black_death_ends_the_visual_without_residues");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(12);const unsigned writes=k->eye_writes;
  w.destroy(k);w.run(3);   // room exit / despawn mid-fury
  require(w.vled.used==0&&w.fx.live()==0&&w.fx.orphans_expired==1,"emitter expires natively within one tick of the last refresh");require(k->eye_writes==writes,"the dead actor's packet is never touched");
  Fake* n=w.add(1,"E_OC2",0,0,0);w.create(n,R::Black,440);w.run(30);require(w.vled.find(1)==nullptr&&n->eye_tex==0&&n->atp[0]==3&&w.fx.live()==0,"new generation of the same id: nothing inherited, no accidental start");
  pass("destruction_room_change_and_id_reuse_leave_nothing_behind_and_inherit_nothing");}
 // ===================== OWNERSHIP / SAFETY =====================
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);k->eye_foreign=true;w.run(5);w.tick({{k,10}});w.run(30);
  require(w.count(Ev::EyesRejected,1)==1&&k->eye_tex==0&&k->eye_writes==0,"a texture we do not own is never written");require(w.count(Ev::AuraOn,1)==1&&w.aura_level(k)==100,"the aura still works on its own");
  const unsigned calls=w.fx.eye_calls;w.run(400);require(w.fx.eye_calls==calls&&w.count(Ev::EyesRejected,1)==1,"no retries, no log spam");
  pass("third_party_eye_texture_is_rejected_cleanly_without_touching_it");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(4);require(k->eye_tex>0&&k->eye_tex<7,"mid fade");
  k->eye_foreign=true;const unsigned writes=k->eye_writes;w.run(10);
  require(w.count(Ev::EyesRejected,1)==1&&k->eye_writes==writes,"changed under us mid-fade: stop, never overwrite, never restore over it");pass("third_party_change_mid_fade_stops_eye_management");}
 {World w;FakeFx& fx=w.fx;fx.fail_aura=true;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(40);
  require(w.count(Ev::AuraRejected,1)==1&&fx.live()==0&&fx.creates==0,"aura refused: one rejection, no emitter");require(k->eye_tex==7&&w.count(Ev::EyesOn,1)==1,"eyes independent of the aura");
  const unsigned calls=fx.releases;w.run(400);require(fx.releases==calls&&w.count(Ev::AuraRejected,1)==1,"no retry for this generation");
  pass("aura_unavailable_is_rejected_once_and_eyes_still_work");}
 {World w;std::vector<Fake*> bl;for(unsigned i=0;i<10;++i){bl.push_back(w.add(100+i,"E_OC",10.f*i,0,0));w.create(bl.back());}
  w.run(40);require(w.fx.live()<=vv::kMaxAuras&&w.fx.live()>0,"bounded number of simultaneous auras");unsigned rej=w.count(Ev::AuraRejected);require(rej==10-vv::kMaxAuras,"the rest are refused cleanly");
  require(w.vled.live_auras()==vv::kMaxAuras,"ledger agrees");pass("simultaneous_aura_emitters_are_bounded");}
 {World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);w.create(b);w.create(o);b->atp[0]=9;w.run(20);   // third party touched a collider: the dynamic layer disowns the pair
  require(!w.dyn(b)->owned&&w.vled.find(1)==nullptr&&w.fx.live()==0,"no managed ability (not owned) -> no visual");
  pass("an_actor_whose_colliders_are_not_owned_never_shows_an_ability_visual");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);Fake* b=w.add(2,"E_OC",5000,0,0);Fake* o=w.add(3,"E_OC2",5100,0,0);w.create(k,R::Black,440);w.create(b);w.create(o);w.run(5);w.tick({{k,10}});w.run(12);w.run(15);
  const Variant seen[]{Variant::Black,Variant::Blue};for(Variant v:seen){(void)v;}
  require(w.vled.find(1)&&w.vled.find(2)&&w.vled.find(3)==nullptr,"Black and Blue have independent records, the Red none");
  // shutdown while both are on and the Black eyes are purple
  vv::Entry* ke=w.vled.find(1);vv::Entry* be=w.vled.find(2);k->eye_foreign=false;
  w.fx.cur=k;vv::cleanup(*ke,w.fx,true);require(k->eye_tex==0&&w.vled.find(1)->aura_live==false,"live Black: eyes back to native, aura released");
  w.fx.cur=b;vv::cleanup(*be,w.fx,true);require(!be->aura_live,"Blue: aura released");for(int i=0;i<3;++i){w.fx.engine_tick();}   // the mod is gone: nobody refreshes anything any more
  require(w.fx.live()==0,"nothing left in the engine after the next engine tick");
  pass("shutdown_cleanup_restores_eyes_releases_auras_independently");}
 {World w;Fake* k=w.add(1,"E_OC2",0,0,0);w.create(k,R::Black,440);w.run(5);w.tick({{k,10}});w.run(12);const unsigned writes=k->eye_writes;
  vv::Entry* ke=w.vled.find(1);w.fx.cur=k;vv::cleanup(*ke,w.fx,false);require(k->eye_writes==writes,"actor not live at unload: the packet is not touched");require(!ke->aura_live,"the aura handle is still dropped");
  Fake* f=w.add(2,"E_OC2",5000,0,0);w.create(f,R::Black,440);w.run(5);w.tick({{f,10}});w.run(12);f->eye_foreign=true;vv::Entry* fe=w.vled.find(2);const unsigned w2=f->eye_writes;w.fx.cur=f;vv::cleanup(*fe,w.fx,true);require(f->eye_writes==w2,"foreign texture at unload: never restored over it");
  pass("shutdown_cleanup_never_touches_dead_or_foreign_packets");}
 {require(vv::kEventsPerActor==24&&vv::kCapacity==64,"bounds");World w;Fake* b=w.add(1,"E_OC",0,0,0);Fake* o=w.add(2,"E_OC2",100,0,0);w.create(b);w.create(o);
  for(int i=0;i<2000;++i){o->x=(i%40<20)?100.f:5000.f;w.tick();}
  require(w.log.size()<=2000/40*4+8,"visual events only on real on/off transitions, never per frame");pass("visual_events_only_on_transitions");}
 std::printf("VISUAL_LAB_PASS cases=%u POLICY_ONLY; PIXELS_AND_ENGINE_NOT_EXECUTED; HUMAN_VISUAL_PENDING\n",cases);
}
