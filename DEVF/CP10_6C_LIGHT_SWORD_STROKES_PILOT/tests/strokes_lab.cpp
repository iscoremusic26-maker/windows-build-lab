// CP10.6C offline lab: the light-sword strokes aura path (Blue 0x0A58, Black 0x0B9D, common bank). Pure logic, no game, no GPU, no pixels.
// The REAL devf_vis::step (CP10.6A, unchanged) is driven through the REAL devf_stk::AuraPart against a fake engine that models the contract read from the
// source: dPa_control_c::set returns 0 when the tevStr is not ready or the pool is exhausted, an emitter nobody refreshes during a tick is removed by the
// engine, colour/alpha/scale are per-emitter, and a resource id exists in the bank only if the fake says so. It proves id selection, per-id probing, retry,
// fades, cleanup and isolation; it does NOT prove that the strokes look like anything in particular (human Windows round).
#include "../expansion/strokes_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>
namespace vv=devf_vis;
namespace sk=devf_stk;
using vv::Variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
struct Em {bool alive=false,refreshed=false,stopped=false,continuous=false;int alpha=0,r=0,g=0,b=0;float x=0,y=0,z=0,vscale=0,pscale=0;unsigned refreshes=0;std::uint16_t id=0;};
struct FakeEngine {
 bool bank_ok=true,pool_full=false,lookup_fail=false,color_mismatch=false;unsigned not_ready=0;std::set<std::uint16_t> missing;
 std::vector<Em> ems{Em{}};std::uint32_t next=1;std::vector<std::uint16_t> set_ids,probe_ids;unsigned set_calls=0,probe_calls=0,created=0,stops=0,orphans=0;int max_frame=0;
 bool probe(std::uint16_t id,const char*& why){++probe_calls;probe_ids.push_back(id);if(!bank_ok){why="COMMON_PARTICLE_BANK_NOT_LOADED";return false;}if(missing.count(id)){why="RESOURCE_MISSING_FROM_COMMON_BANK";return false;}return true;}
 bool ready(){if(not_ready){--not_ready;return false;}return true;}
 std::uint32_t set(std::uint16_t id,std::uint32_t key,float x,float y,float z){
  ++set_calls;set_ids.push_back(id);
  if(key&&key<ems.size()&&ems[key].alive){Em& m=ems[key];m.refreshed=true;m.x=x;m.y=y;m.z=z;++m.refreshes;return key;}
  if(pool_full)return 0;
  const std::uint32_t k=next++;ems.resize(k+1);Em& m=ems[k];m.alive=true;m.refreshed=true;m.x=x;m.y=y;m.z=z;m.id=id;++created;return k;
 }
 bool configure(std::uint32_t key,const sk::Tuning& t,int alpha,bool created_now,sk::Readback& rb){
  if(lookup_fail||key>=ems.size()||!ems[key].alive)return false;
  Em& m=ems[key];m.r=t.r;m.g=t.g;m.b=t.b;m.alpha=alpha;m.vscale=t.volume_scale;m.pscale=t.particle_scale;if(created_now&&max_frame!=0)m.continuous=true;
  rb.ok=!color_mismatch;rb.r=m.r;rb.g=m.g;rb.b=m.b;rb.a=m.alpha;return true;
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
 FakeEngine own;FakeEngine* en;vv::Ledger L;std::array<sk::Retry,vv::kCapacity> retries{};FakeEye eyes;vv::Entry* e=nullptr;std::uint32_t id=1,gen=1;std::vector<sk::Note> notes;std::vector<vv::Event> events;
 explicit Run(Variant v,std::uint32_t id_=1,FakeEngine* shared=nullptr):en(shared?shared:&own){id=id_;e=L.record(id,this,gen,v);require(e!=nullptr,"record");}
 void tick(bool aura,bool eyes_want=false){
  sk::Retry& rt=sk::retry_for(retries,std::size_t(e-L.entries.data()),e->id,e->generation);
  sk::AuraPart<FakeEngine> ap(*en,rt);ap.x=10;ap.y=20;ap.z=30;Fx fx{ap,eyes};
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
  const auto& em=r.en->ems[r.e->key];require(em.id==0x0A58&&em.r==70&&em.g==170&&em.b==255&&em.vscale==3.0f&&em.pscale==0.45f&&em.alpha==198,"Blue cyan/blue tint, dynamics 3.0, particle 0.45, boosted alpha cap");
  require(r.notes_of(sk::Note::Requested)==1&&r.notes_of(sk::Note::Acquired)==1&&r.notes_of(sk::Note::ColorApplied)==1&&r.notes_of(sk::Note::ColorUnavailable)==0,"requested once, acquired once, colour read back once");
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
  r.run(3,true);require(r.e->key!=k&&r.en->live()==1&&r.en->created==2&&r.notes_of(sk::Note::ColorApplied)==2&&r.notes_of(sk::Note::Acquired)==2&&r.notes_of(sk::Note::Requested)==1,"expired handle: exactly one new emitter, acquired and coloured again, never two alive (no second request: the state did not change)");
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
  require(sk::kBlue.volume_scale==3.0f&&sk::kBlack.volume_scale==3.0f&&sk::kBlue.particle_scale==0.45f&&sk::kBlack.particle_scale==0.45f&&sk::kEmittersPerActor==1,"first-round scales (ring ~72 units, strokes ~9x126) and one emitter per actor");
  pass("tuning_table_sane_distinct_and_documented");}
 std::printf("STROKES_LAB_PASS cases=%u POLICY_ONLY; STROKES_LOOK_NOT_VERIFIED; ENGINE_AND_PIXELS_NOT_EXECUTED\n",cases);
}
