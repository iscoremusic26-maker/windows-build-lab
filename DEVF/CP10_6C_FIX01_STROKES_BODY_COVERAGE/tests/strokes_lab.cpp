// CP10.6C-FIX01 offline lab: the light-sword strokes aura path (Blue 0x0A58, Black 0x0B9D, common bank) with the body-coverage geometry. Pure logic, no game, no GPU, no pixels.
// The REAL devf_vis::step (CP10.6A, unchanged) is driven through the REAL devf_stk::AuraPart against a fake engine that models the contract read from the
// source: dPa_control_c::set returns 0 when the tevStr is not ready or the pool is exhausted, an emitter nobody refreshes during a tick is removed by the
// engine, colour/alpha/scale are per-emitter, and a resource id exists in the bank only if the fake says so. It proves id selection, per-id probing, retry,
// fades, cleanup and isolation; it does NOT prove that the strokes look like anything in particular (human Windows round).
#include "../expansion/strokes_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
namespace vv=devf_vis;
namespace sk=devf_stk;
using vv::Variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
struct Em {bool alive=false,refreshed=false,stopped=false,continuous=false,dir_locked=false;int alpha=0,r=0,g=0,b=0;float x=0,y=0,z=0,vscale=0,wscale=0,lscale=0,min_rad=0;unsigned refreshes=0;std::uint16_t id=0;};
struct FakeEngine {
 bool bank_ok=true,pool_full=false,lookup_fail=false,color_mismatch=false,geo_mismatch=false;unsigned not_ready=0;std::set<std::uint16_t> missing;sk::Facts fct;
 std::vector<Em> ems{Em{}};std::uint32_t next=1;std::vector<std::uint16_t> set_ids,probe_ids;unsigned set_calls=0,probe_calls=0,created=0,stops=0,orphans=0;int max_frame=0;
 bool probe(std::uint16_t id,const char*& why){++probe_calls;probe_ids.push_back(id);if(!bank_ok){why="COMMON_PARTICLE_BANK_NOT_LOADED";return false;}if(missing.count(id)){why="RESOURCE_MISSING_FROM_COMMON_BANK";return false;}return true;}
 bool ready(){if(not_ready){--not_ready;return false;}return true;}
 const sk::Facts& facts(std::uint16_t){return fct;}
 std::uint32_t set(std::uint16_t id,std::uint32_t key,float x,float y,float z){
  ++set_calls;set_ids.push_back(id);
  if(key&&key<ems.size()&&ems[key].alive){Em& m=ems[key];m.refreshed=true;m.x=x;m.y=y;m.z=z;++m.refreshes;return key;}
  if(pool_full)return 0;
  const std::uint32_t k=next++;ems.resize(k+1);Em& m=ems[k];m.alive=true;m.refreshed=true;m.x=x;m.y=y;m.z=z;m.id=id;++created;return k;
 }
 bool configure(std::uint32_t key,const sk::Tuning& t,const sk::Plan& pl,int alpha,bool created_now,sk::Readback& rb){
  if(lookup_fail||key>=ems.size()||!ems[key].alive)return false;
  Em& m=ems[key];m.r=t.r;m.g=t.g;m.b=t.b;m.alpha=alpha;m.vscale=t.volume_scale;m.wscale=t.width_scale;m.lscale=t.length_scale;m.min_rad=pl.min_rad;m.dir_locked=pl.lock_dir;if(created_now&&max_frame!=0)m.continuous=true;
  rb.ok=!color_mismatch;rb.geo_ok=!geo_mismatch;rb.min_rad=m.min_rad;rb.r=m.r;rb.g=m.g;rb.b=m.b;rb.a=m.alpha;return true;
 }
 void stop(std::uint32_t key){if(key<ems.size()){ems[key].stopped=true;ems[key].alpha=0;}++stops;}
 void engine_tick(){for(auto& m:ems){if(m.alive&&!m.refreshed){m.alive=false;++orphans;}m.refreshed=false;}}
 unsigned live()const{unsigned n=0;for(auto& m:ems)if(m.alive)++n;return n;}
};
struct FakeEye {unsigned tex=0;bool foreign=false;unsigned writes=0;};
struct Fx {
 sk::AuraPart<FakeEngine>& ap;FakeEye& eyes;const char* last="NONE";
 bool aura(vv::Entry& e,int l,int a){const bool ok=ap.aura(e,l,a);last=ap.why;return ok;}
 void aura_release(vv::Entry& e){ap.aura_release(e);}
 bool eye(vv::Entry&,unsigned idx){if(eyes.foreign){last="EYE_PACKET_TEXTURE_NOT_OURS";return false;}eyes.tex=idx;++eyes.writes;return true;}
 const char* reason()const{return last;}
};
struct Run {
 FakeEngine own;FakeEngine* en;vv::Ledger L;std::array<sk::Retry,vv::kCapacity> retries{};FakeEye eyes;vv::Entry* e=nullptr;std::uint32_t id=1,gen=1;std::vector<sk::Note> notes;std::vector<vv::Event> events;float px=10,py=20,pz=30,eye_dy=200;
 explicit Run(Variant v,std::uint32_t id_=1,FakeEngine* shared=nullptr):en(shared?shared:&own){id=id_;e=L.record(id,this,gen,v);require(e!=nullptr,"record");}
 void tick(bool aura,bool eyes_want=false){
  sk::Retry& rt=sk::retry_for(retries,std::size_t(e-L.entries.data()),e->id,e->generation);
  sk::AuraPart<FakeEngine> ap(*en,rt);ap.fx=px;ap.fy=py;ap.fz=pz;ap.eye_dy=eye_dy;Fx fx{ap,eyes};
  const auto out=vv::step(L,*e,fx,aura,eyes_want);
  for(unsigned i=0;i<out.n;++i){events.push_back(out.ev[i]);}
  for(unsigned i=0;i<ap.notes.count;++i){notes.push_back(ap.notes.n[i]);}
  en->engine_tick();
 }
 void run(unsigned n,bool a,bool ey=false){for(unsigned i=0;i<n;++i)tick(a,ey);}
 unsigned count(vv::Ev k){unsigned n=0;for(auto& x:events)if(x.kind==k)++n;return n;}
 unsigned notes_of(sk::Note::Kind k){unsigned n=0;for(auto& x:notes)if(x.kind==k)++n;return n;}
};
int main(){
 // ---------- resource selection ----------
 {static_assert(sk::kResourceBlue==0x0A58&&sk::kResourceBlack==0x0B9D&&sk::kBank==0&&sk::kEmittersPerActor==1,"pilot table");
  require(sk::resource_for(Variant::Blue)==0x0A58&&sk::resource_for(Variant::Black)==0x0B9D&&sk::resource_for(Variant::Red)==0&&sk::resource_for(Variant::None)==0,"A for Blue, C for Black, nothing for the rest");
  for(std::uint16_t bad:{0x0A59,0x0046,0x8046,0x072F}){require(sk::never_used(bad),"forbidden ids");}
  require(!sk::never_used(0x0A58)&&!sk::never_used(0x0B9D),"pilot ids are not forbidden");
  Run b(Variant::Blue);b.run(40,true);b.run(40,false);require(!b.en->set_ids.empty(),"engine was called");for(auto id:b.en->set_ids)require(id==0x0A58,"Blue only ever passes 0x0A58");for(auto id:b.en->probe_ids)require(id==0x0A58,"Blue probes only 0x0A58");
  Run k(Variant::Black);k.run(20,true,true);for(auto id:k.en->set_ids)require(id==0x0B9D,"Black only ever passes 0x0B9D");for(auto id:k.en->probe_ids)require(id==0x0B9D,"Black probes only 0x0B9D");
  for(const Run* r:{&b,&k}){for(auto id:r->en->set_ids)require(id!=0x0A59&&id!=0x0046&&id!=0x8046&&id!=0x072F,"never SWL_B, WindLine, 0x8046 nor the retired glow");}
  pass("blue_uses_only_0x0A58_black_only_0x0B9D_nothing_else_is_ever_probed_or_requested");}
 {Run r(Variant::Blue);vv::Entry fake=*r.e;fake.variant=Variant::Red;sk::Retry rt;sk::AuraPart<FakeEngine> ap(*r.en,rt);require(!ap.aura(fake,10,100)&&r.en->set_calls==0&&r.en->probe_calls==0,"a variant without a pilot resource is refused before any engine call");
  vv::Ledger L;int d=0;require(L.record(1,&d,1,Variant::Red)==nullptr&&L.record(2,&d,1,Variant::None)==nullptr,"Red and unclassified never get a visual record");
  pass("red_and_unclassified_never_request_an_emitter");}
 // ---------- Blue lifecycle ----------
 {Run r(Variant::Blue);r.run(5,false);require(r.en->set_calls==0&&r.en->probe_calls==0&&r.en->live()==0&&r.events.empty(),"inactive: engine untouched, nothing probed");
  r.run(12,true);require(r.e->aura_level==100&&r.en->live()==1&&r.en->created==1,"faded in over 10 ticks with ONE emitter");
  const auto& em=r.en->ems[r.e->key];require(em.id==0x0A58&&em.r==70&&em.g==170&&em.b==255&&em.vscale==4.2f&&em.wscale==0.70f&&em.lscale==0.55f&&em.min_rad==0.6f&&em.alpha==198,"Blue cyan/blue tint, dynamics 4.2, width 0.70, length 0.55, min radius 0.6, boosted alpha cap");
  require(r.notes_of(sk::Note::Requested)==1&&r.notes_of(sk::Note::Acquired)==1&&r.notes_of(sk::Note::ColorApplied)==1&&r.notes_of(sk::Note::ColorUnavailable)==0&&r.notes_of(sk::Note::TuningApplied)==1,"requested once, acquired once, colour read back once, tuning reported once");
  require(r.count(vv::Ev::AuraOn)==1,"one ACTOR_STATE_ON");r.run(20,true);require(r.en->created==1&&r.en->ems[r.e->key].refreshes>=30,"refreshed every tick, never recreated");
  unsigned ticks=0;while(r.e->aura_level>0&&ticks<40){r.tick(false);++ticks;}require(ticks==15,"15 ticks out");require(r.count(vv::Ev::AuraOff)==1&&r.e->key==0&&r.en->stops==1,"released once");
  r.run(3,false);require(r.en->live()==0&&r.en->orphans==1,"the engine removed the emitter nobody refreshes");
  pass("blue_strokes_fade_in_10_out_15_one_emitter_tint_scale_alpha_then_released");}
 {Run r(Variant::Blue);r.en->not_ready=4;r.run(2,true);
  require(r.count(vv::Ev::AuraOn)==1&&r.en->created==0&&r.notes_of(sk::Note::Acquired)==0,"ACTOR_STATE_ON is logged while no emitter exists: state is not an emitter");
  r.run(10,true);require(r.en->created==1&&r.notes_of(sk::Note::Acquired)==1,"the emitter is acquired later: a separate event");
  pass("actor_state_on_is_distinct_from_emitter_acquired");}
 {Run r(Variant::Blue);r.run(30,true);r.run(7,false);const int mid=r.e->aura_level;require(mid>0&&mid<100,"mid fade-out");const unsigned created=r.en->created;r.run(2,true);require(r.en->created==created&&r.e->aura_level>mid-14,"resume from the current level, same emitter");
  r.run(20,true);require(r.e->aura_level==100&&r.count(vv::Ev::AuraOff)==0,"never released in between");pass("blue_reactivation_during_fade_out_resumes_same_emitter");}
 // ---------- Black lifecycle ----------
 {Run r(Variant::Black);r.run(15,true,true);const auto& em=r.en->ems[r.e->key];require(em.id==0x0B9D&&em.r==185&&em.g==90&&em.b==255&&em.alpha==234&&r.eyes.tex==7,"Black violet tint, stronger alpha, eyes purple");
  const unsigned created=r.en->created,ev=r.events.size(),nn=r.notes.size();r.run(600,true,true);   // fury refreshes: the target simply stays on
  require(r.en->created==created&&r.events.size()==ev&&r.notes.size()==nn&&r.eyes.tex==7&&r.en->stops==0,"refresh: no new emitter, no event, no note, no stop, no flicker");
  r.run(40,false,false);require(r.e->aura_level==0&&r.eyes.tex==0&&r.en->stops==1&&r.count(vv::Ev::EyesOff)==1,"fury over: aura stopped, eyes native");r.run(3,false);require(r.en->live()==0,"no residue");
  pass("black_strokes_and_eyes_on_with_fury_stable_through_refresh_then_clean_off");}
 // ---------- transient engine refusals ----------
 {Run r(Variant::Blue);r.en->not_ready=5;r.run(3,true);require(r.en->live()==0&&r.e->aura_fault==false&&r.e->aura_live,"tevStr not ready: no emitter, no fault, logic stays on");
  r.run(10,true);require(r.en->live()==1&&r.notes_of(sk::Note::RetryStart)==1&&r.notes_of(sk::Note::Acquired)==1&&r.notes_of(sk::Note::Requested)==1&&r.count(vv::Ev::AuraRejected)==0,"acquired after the retries: one request, one retry start, one acquisition, no rejection");
  bool late=false;for(auto& n:r.notes)if(n.kind==sk::Note::Acquired&&n.fails>0)late=true;require(late,"the acquisition records that it came after retries");
  pass("tevstr_not_ready_is_retried_not_a_permanent_fault");}
 {Run r(Variant::Blue);r.en->pool_full=true;r.run(100,true);
  require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault&&r.e->aura_level==0&&r.en->live()==0,"pool exhausted for good: rejected once, level 0");
  require(r.en->set_calls==sk::kRetryTicks+1&&r.notes_of(sk::Note::Exhausted)==1,"exactly kRetryTicks+1 attempts, then no more calls");
  const unsigned calls=r.en->set_calls;r.run(200,true);require(r.en->set_calls==calls&&r.count(vv::Ev::AuraRejected)==1,"no retries after the rejection, no log spam");
  pass("pool_exhaustion_is_bounded_then_rejected_once_for_the_generation");}
 {Run r(Variant::Blue);r.en->pool_full=true;r.run(30,true);require(r.en->live()==0&&!r.e->aura_fault,"still retrying");r.en->pool_full=false;r.run(3,true);
  require(r.en->live()==1&&r.notes_of(sk::Note::Acquired)==1&&r.count(vv::Ev::AuraRejected)==0&&sk::retry_for(r.retries,std::size_t(r.e-r.L.entries.data()),1,1).fails==0,"recovered: acquired, retry count reset");
  pass("recovery_when_the_pool_frees_up_within_the_retry_budget");}
 {Run r(Variant::Blue);r.en->bank_ok=false;r.run(100,true);require(r.en->set_ids.empty()&&r.en->set_calls==0,"bank not loaded: set() is never called");require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault,"rejected after the budget");
  Run q(Variant::Blue);q.en->bank_ok=false;q.run(10,true);q.en->bank_ok=true;q.run(5,true);require(q.en->live()==1,"bank appeared in time: works");
  pass("missing_bank_never_reaches_set_and_is_rejected_cleanly_or_recovers");}
 {FakeEngine shared;shared.missing.insert(0x0A58);Run blue(Variant::Blue,1,&shared);Run black(Variant::Black,2,&shared);blue.run(100,true);black.run(100,true,true);
  require(blue.count(vv::Ev::AuraRejected)==1&&blue.e->aura_fault&&blue.en->created==1,"Blue: 0x0A58 missing -> rejected once, no emitter of its own");
  require(black.en->ems[black.e->key].id==0x0B9D&&black.e->aura_live&&!black.e->aura_fault&&black.eyes.tex==7,"Black: 0x0B9D present -> works, eyes purple, independent of the missing Blue resource");
  bool blue_probe_only_a=true;for(auto id:shared.probe_ids)if(id!=0x0A58&&id!=0x0B9D)blue_probe_only_a=false;require(blue_probe_only_a,"only the two pilot ids are ever probed");
  FakeEngine s2;s2.missing.insert(0x0B9D);Run bl(Variant::Blue,1,&s2);Run bk(Variant::Black,2,&s2);bl.run(30,true);bk.run(100,true,true);
  require(bl.e->aura_live&&!bl.e->aura_fault&&bk.e->aura_fault&&bk.count(vv::Ev::AuraRejected)==1&&bk.eyes.tex==7,"the other way round: C missing rejects only Black's aura, its eyes still work");
  pass("a_missing_resource_is_detected_per_id_and_never_affects_the_other_variant_or_the_eyes");}
 {Run r(Variant::Blue);r.en->lookup_fail=true;r.run(100,true);require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault,"lookup always failing: bounded, rejected once");
  Run q(Variant::Blue);q.en->color_mismatch=true;q.run(15,true);require(q.notes_of(sk::Note::ColorUnavailable)==1&&q.notes_of(sk::Note::ColorApplied)==0&&q.en->live()==1,"colour readback mismatch is reported (COLOR_UNAVAILABLE) but the emitter still works");
  pass("lookup_failure_bounded_and_colour_unavailable_reported_honestly");}
 {Run r(Variant::Blue);r.run(20,true);const auto k=r.e->key;r.en->ems[k].alive=false;   // the engine/another system removed it (event, scene change)
  r.run(3,true);require(r.e->key!=k&&r.en->live()==1&&r.en->created==2&&r.notes_of(sk::Note::ColorApplied)==2&&r.notes_of(sk::Note::TuningApplied)==2&&r.notes_of(sk::Note::Acquired)==2&&r.notes_of(sk::Note::Requested)==1,"expired handle: exactly one new emitter, acquired and coloured again, never two alive (no second request: the state did not change)");
  pass("expired_emitter_is_recreated_once_never_duplicated");}
 // ---------- ownership / cleanup / bounds / isolation ----------
 {std::array<sk::Retry,4> t{};auto& x=sk::retry_for(t,0,7,1);x.fails=9;require(sk::retry_for(t,0,7,1).fails==9,"same generation keeps it");require(sk::retry_for(t,0,7,2).fails==0&&sk::retry_for(t,0,8,2).fails==0,"new generation or id starts clean");
  pass("retry_state_is_stamped_by_id_and_generation");}
 {FakeEngine shared;vv::Ledger L;std::array<sk::Retry,vv::kCapacity> rt{};FakeEye eyes;unsigned rejected=0;std::vector<vv::Entry*> es;
  for(unsigned i=0;i<10;++i){
   static int dummy[16];es.push_back(L.record(100+i,&dummy[i],1,i%2?Variant::Black:Variant::Blue));require(es.back(),"record");}
  for(unsigned t=0;t<40;++t){for(auto* e:es){sk::AuraPart<FakeEngine> ap(shared,sk::retry_for(rt,std::size_t(e-L.entries.data()),e->id,e->generation));Fx fx{ap,eyes};const auto o=vv::step(L,*e,fx,true,false);for(unsigned i=0;i<o.n;++i)if(o.ev[i].kind==vv::Ev::AuraRejected)++rejected;}shared.engine_tick();}
  require(shared.live()==vv::kMaxAuras&&rejected==10-vv::kMaxAuras,"capacity is explicit: at most 8 emitters, the rest rejected cleanly");
  std::set<std::uint32_t> owners;for(auto& m:shared.ems)if(m.alive)owners.insert(m.id);require(owners.size()==2,"the 8 live emitters are of the two pilot resources only");
  pass("emitter_capacity_is_bounded_and_explicit_one_emitter_per_actor");}
 {Run r(Variant::Black);r.en->pool_full=true;r.run(100,true,true);require(r.e->aura_fault&&r.eyes.tex==7&&r.count(vv::Ev::EyesRejected)==0,"strokes failure: the eyes are not disabled, not touched");r.run(40,false,false);require(r.eyes.tex==0&&r.count(vv::Ev::EyesOff)==1,"and still return to native");
  Run q(Variant::Black);q.eyes.foreign=true;q.run(30,true,true);require(q.count(vv::Ev::EyesRejected)==1&&q.e->eye_fault&&q.en->live()==1&&!q.e->aura_fault,"eye failure: the aura keeps working");
  pass("aura_and_eye_faults_are_independent");}
 {Run r(Variant::Blue);r.run(20,true);require(r.en->live()==1,"live");r.en->engine_tick();r.en->engine_tick();require(r.en->live()==0&&r.en->orphans==1,"actor gone: the engine removes the emitter within one tick, nothing to unwind");
  Run s(Variant::Black);s.run(20,true,true);sk::Retry& rt=s.retries[0];FakeEye& ey=s.eyes;sk::AuraPart<FakeEngine> ap(*s.en,rt);Fx fx{ap,ey};vv::cleanup(*s.e,fx,true);require(s.en->stops==1&&s.e->key==0&&ey.tex==0,"shutdown cleanup: aura stopped, eyes native");s.en->engine_tick();s.en->engine_tick();require(s.en->live()==0,"nothing left");
  pass("actor_gone_and_shutdown_leave_no_emitter");}
 {Run a(Variant::Blue,1);Run b(Variant::Blue,2);a.run(20,true);b.run(20,true);vv::Entry copy=*a.e;(void)copy;   // new generation of the same id after despawn: nothing inherited
  Run g(Variant::Blue,1);g.gen=2;g.run(3,false);require(g.en->set_calls==0&&g.e->aura_level==0&&g.e->key==0,"a fresh record starts inactive with no handle");
  pass("new_generation_starts_clean");}
 {require(sk::boosted_alpha(Variant::Blue,0)==0&&sk::boosted_alpha(Variant::Blue,110)==198&&sk::boosted_alpha(Variant::Black,130)==234&&sk::boosted_alpha(Variant::Black,1000)==255,"alpha boost clamps");
  require(sk::kBlue.b==255&&sk::kBlue.r<sk::kBlue.g&&sk::kBlack.r>sk::kBlack.g&&sk::kBlack.b==255&&sk::kBlue.g!=sk::kBlack.g,"cyan-blue vs violet, distinct");
  require(sk::kBlue.volume_scale==4.2f&&sk::kBlack.volume_scale==4.2f&&sk::kBlue.width_scale==0.70f&&sk::kBlack.width_scale==0.70f&&sk::kBlue.length_scale==0.55f&&sk::kBlack.length_scale==0.55f&&sk::kBlue.min_rad_pm==600&&sk::kBlack.min_rad_pm==600&&sk::kBlue.height_pm==580&&sk::kBlack.height_pm==580&&sk::kBlue.alpha_boost_pm==1800&&sk::kBlack.alpha_boost_pm==1800&&sk::kEmittersPerActor==1,"FIX01 table: ring 4.2, width 0.70, length 0.55, min radius 0.60, height 0.58, alpha boost unchanged, one emitter per actor");
  pass("tuning_table_sane_distinct_and_documented");}

 // ---------- FIX01: geometry, placement, compensation, direction, min radius, alpha progression ----------
 {const unsigned vs=24;const float R=sk::nominal_radius(vs,sk::kBlue),in=sk::nominal_inner(vs,sk::kBlue);
  require(R>100.7f&&R<100.9f&&in>60.4f&&in<60.6f,"nominal ring radius 100.8, spawn band starts at 60.5");
  require(in>sk::kTorsoColliderRadius*1.2f&&in<R,"the inner edge clears the 50-unit torso collider with a margin and stays inside the ring");
  require(sk::nominal_radius(vs,sk::kBlack)==R&&sk::nominal_inner(vs,sk::kBlack)==in,"same spatial criterion for Blue and Black");
  const float wB=20.0f*sk::kBlue.width_scale,lB=280.0f*sk::kBlue.length_scale,lK=300.0f*sk::kBlack.length_scale;
  require(wB>13.9f&&wB<14.1f&&lB>153.9f&&lB<154.1f&&lK>164.9f&&lK<165.1f&&lB>wB*10.0f&&lK>wB*10.0f,"nominal quads stay thin: 14 wide, 154 / 165 long (more than 10:1)");
  require(sk::kBlue.width_scale<1.0f&&sk::kBlue.length_scale<1.0f&&sk::kBlue.width_scale>0.45f&&sk::kBlue.length_scale>0.45f,"wider and longer than CP10.6C (0.45) but smaller than the resource's native size");
  pass("nominal_geometry_clears_the_torso_collider_and_strokes_stay_thin");}
 {Run r(Variant::Blue);r.px=100;r.py=-50;r.pz=7;r.eye_dy=200;r.run(12,true);auto& m=r.en->ems[r.e->key];
  require(m.x==100&&m.z==7&&m.y>-50.0f+115.9f&&m.y<-50.0f+116.1f,"emitter at feet + 0.58 x measured eye offset (200 -> 116)");
  r.px=140;r.py=0;r.pz=-33;r.eye_dy=260;r.tick(true);require(r.en->ems[r.e->key].x==140&&r.en->ems[r.e->key].z==-33&&r.en->ems[r.e->key].y>150.7f&&r.en->ems[r.e->key].y<150.9f&&r.en->created==1,"the actor moved: the SAME emitter follows on the next tick (no new emitter, no trail left)");
  Run lo(Variant::Black);lo.eye_dy=-3000;lo.run(12,true,true);require(lo.en->ems[lo.e->key].y>20.0f+34.7f&&lo.en->ems[lo.e->key].y<20.0f+34.9f,"uninitialised / glitched eye offset is clamped to 60 -> 34.8 above the feet");
  Run hi(Variant::Black);hi.eye_dy=99999;hi.run(12,true,true);require(hi.en->ems[hi.e->key].y>20.0f+289.9f&&hi.en->ems[hi.e->key].y<20.0f+290.1f,"absurdly large eye offset is clamped to 500 -> 290");
  pass("emitter_height_follows_the_measured_body_and_the_actor_every_tick_with_sane_clamps");}
 {Run r(Variant::Blue);r.en->fct.tx=0;r.en->fct.ty=0;r.en->fct.tz=0;r.run(12,true);require(r.en->ems[r.e->key].x==10&&r.en->ems[r.e->key].z==30,"zero resource translation: no compensation");
  Run c(Variant::Blue);c.en->fct.tz=20;c.en->fct.tx=-5;c.run(12,true);const Em& m=c.en->ems[c.e->key];const float s=sk::kBlue.volume_scale;
  const float cx=m.x+s*(-5.0f),cz=m.z+s*20.0f;   // engine rule: ring centre = translation + global scale x resource local translation (JPAResource::calcWorkData_c)
  require(cx>9.99f&&cx<10.01f&&cz>29.99f&&cz<30.01f,"ring centre lands on the requested body point even with a resource-local translation");
  Run big(Variant::Black);big.en->fct.tz=200;big.run(12,true,true);require(big.en->ems[big.e->key].z==30.0f-sk::kCompLimit,"a huge offset is clamped (400), never an unbounded shift");
  const sk::Plan pb=sk::plan_for(big.en->fct,sk::kBlack);require(pb.comp_clamped&&!sk::plan_for(sk::Facts{},sk::kBlue).comp_clamped,"the clamp is reported");
  pass("resource_local_translation_is_compensated_so_the_ring_is_centred_on_the_body");}
 {sk::Facts v;require(!sk::plan_for(v,sk::kBlue).lock_dir&&!sk::plan_for(v,sk::kBlue).dir_nonvertical_kept,"vertical +Y direction: untouched");
  v.dy=-1;require(!sk::plan_for(v,sk::kBlue).lock_dir,"vertical -Y (downward) direction: untouched");
  v.dx=1;v.dy=0.2f;v.dz=0;v.rot_zero=true;require(sk::plan_for(v,sk::kBlue).lock_dir&&!sk::plan_for(v,sk::kBlue).dir_nonvertical_kept,"unrotated resource with a horizontal direction: vertical lock");
  v.rot_zero=false;require(!sk::plan_for(v,sk::kBlue).lock_dir&&sk::plan_for(v,sk::kBlue).dir_nonvertical_kept,"rotated resource: left alone, reported");
  v=sk::Facts{};v.dx=v.dy=v.dz=0;require(sk::plan_for(v,sk::kBlue).lock_dir,"a zero vector is not vertical");
  v=sk::Facts{};v.dy=0.95f;v.dx=0.3f;require(!sk::plan_for(v,sk::kBlue).lock_dir,"0.95 vertical component: still vertical");
  Run r(Variant::Blue);r.en->fct.dx=0;r.en->fct.dy=0;r.en->fct.dz=1;r.run(12,true);require(r.en->ems[r.e->key].dir_locked,"engine adapter receives the lock for a horizontal resource");
  Run q(Variant::Blue);q.run(12,true);require(!q.en->ems[q.e->key].dir_locked,"and not for a vertical one");
  pass("emitter_direction_is_locked_vertical_only_for_an_unrotated_non_vertical_resource");}
 {sk::Facts f;f.min_rad=0.0f;require(sk::plan_for(f,sk::kBlue).min_rad==0.6f,"resource without a hole gets 0.60");f.min_rad=0.8f;require(sk::plan_for(f,sk::kBlue).min_rad==0.8f,"a wider resource hole is kept");
  Run r(Variant::Black);r.en->fct.min_rad=0.1f;r.run(12,true,true);require(r.en->ems[r.e->key].min_rad==0.6f,"applied to the emitter (per emitter, not the shared resource)");
  pass("min_radius_is_per_emitter_and_never_narrower_than_the_resource_own_hole");}
 {Run b(Variant::Blue);int prev=0;std::vector<int> bl;for(int i=0;i<12;++i){b.tick(true);if(b.e->key)bl.push_back(b.en->ems[b.e->key].alpha);}
  require(bl.size()==12&&bl[0]==19&&bl[9]==198&&bl[11]==198,"Blue: first fade-in sample 19 (as in the log), peak 198 at tick 10, then stays");for(int a:bl){require(a>=prev,"alpha never decreases while fading in");prev=a;}
  Run k(Variant::Black);prev=0;std::vector<int> kl;for(int i=0;i<12;++i){k.tick(true,true);if(k.e->key)kl.push_back(k.en->ems[k.e->key].alpha);}
  require(kl.size()==12&&kl[0]==23&&kl[9]==234&&kl[11]==234,"Black: first sample 23 (as in the log), peak 234");for(int a:kl){require(a>=prev,"monotonic");prev=a;}
  int down=234;unsigned n=0;while(k.e->aura_level>0&&n<40){k.tick(false,false);if(k.e->key){require(k.en->ems[k.e->key].alpha<=down,"fade-out never brightens");down=k.en->ems[k.e->key].alpha;}++n;}require(n==15,"15 ticks out");
  pass("alpha_progression_is_monotonic_19_23_samples_are_tick_one_of_a_ten_tick_fade_in");}
 {Run r(Variant::Blue);r.en->geo_mismatch=true;r.run(15,true);bool mism=false;for(auto& n:r.notes)if(n.kind==sk::Note::TuningApplied&&!n.color.geo_ok&&std::string(n.why)=="GEOMETRY_READBACK_MISMATCH")mism=true;
  require(mism&&r.en->live()==1&&!r.e->aura_fault,"a geometry readback mismatch is REPORTED (not hidden) and does not kill the emitter");
  Run q(Variant::Blue);q.run(15,true);bool good=false;for(auto& n:q.notes)if(n.kind==sk::Note::TuningApplied&&n.color.geo_ok&&n.applied.y_off>115.9f&&n.applied.y_off<116.1f&&n.applied.radius>100.7f)good=true;require(good,"the applied parameters are carried to the log: height 116, radius 100.8");
  pass("tuning_applied_reports_the_real_readback_honestly");}
 {Run bk(Variant::Black);bk.run(100,false,false);require(bk.en->set_calls==0&&bk.en->probe_calls==0&&bk.notes.empty()&&bk.events.empty(),"Black without fury: nothing probed, requested or spawned");
  Run bl(Variant::Blue);bl.run(100,false);require(bl.en->set_calls==0&&bl.en->live()==0&&bl.notes.empty(),"Blue without a group: nothing");
  pass("inactive_states_spawn_and_log_nothing");}
 {Run r(Variant::Blue);r.run(20,true);const auto k=r.e->key;vv::Entry* e=r.e;sk::Retry& rt=r.retries[0];sk::AuraPart<FakeEngine> ap(*r.en,rt);FakeEye ey;Fx fx{ap,ey};vv::cleanup(*e,fx,true);
  r.en->engine_tick();r.en->engine_tick();require(r.en->live()==0&&k!=0,"room change / unload: emitter stopped and gone");
  FakeEngine fresh;Run again(Variant::Blue,1,&fresh);again.gen=2;again.run(12,true);require(fresh.created==1&&fresh.live()==1,"re-entry: exactly one new emitter for the new generation, none inherited");
  pass("reentry_after_cleanup_creates_exactly_one_new_emitter");}
 std::printf("STROKES_LAB_PASS cases=%u POLICY_ONLY; STROKES_LOOK_NOT_VERIFIED; ENGINE_AND_PIXELS_NOT_EXECUTED\n",cases);
}
