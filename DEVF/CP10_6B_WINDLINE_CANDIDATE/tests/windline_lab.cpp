// CP10.6B offline lab: the WindLine (0x0046) aura path. Pure logic, no game, no GPU, no pixels.
// The REAL devf_vis::step (CP10.6A, unchanged) is driven through the REAL devf_wl::AuraPart against a fake engine that models the contract
// read from the source: dPa_control_c::set returns 0 when the tevStr is not ready or the pool is exhausted, an emitter nobody refreshes during
// a tick is removed by the engine, and colour/alpha are per-emitter. It proves id selection, retry/recovery, fades, cleanup and isolation;
// it does NOT prove that WindLine looks like anything in particular (human Windows round).
#include "../expansion/windline_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
namespace vv=devf_vis;
namespace wl=devf_wl;
using vv::Variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
struct Em {bool alive=false,refreshed=false,stopped=false,continuous=false;int alpha=0,r=0,g=0,b=0;float x=0,y=0,z=0,scale=0;unsigned refreshes=0;};
struct FakeEngine {
 bool bank_ok=true,pool_full=false,lookup_fail=false,color_mismatch=false;unsigned not_ready=0;
 std::vector<Em> ems{Em{}};std::uint32_t next=1;std::vector<std::uint16_t> ids;unsigned set_calls=0,probe_calls=0,created=0,stops=0,orphans=0;bool resource_finite=true;
 bool probe(const char*& why){++probe_calls;if(!bank_ok){why="COMMON_PARTICLE_BANK_NOT_LOADED";return false;}return true;}
 bool ready(){if(not_ready){--not_ready;return false;}return true;}
 std::uint32_t set(std::uint16_t id,std::uint32_t key,float x,float y,float z){
  ++set_calls;ids.push_back(id);
  if(key&&key<ems.size()&&ems[key].alive){Em& m=ems[key];m.refreshed=true;m.x=x;m.y=y;m.z=z;++m.refreshes;return key;}   // keyed refresh also moves it
  if(pool_full)return 0;
  const std::uint32_t k=next++;ems.resize(k+1);Em& m=ems[k];m.alive=true;m.refreshed=true;m.x=x;m.y=y;m.z=z;++created;return k;
 }
 bool configure(std::uint32_t key,const wl::Tuning& t,int alpha,bool created_now,wl::Readback& rb){
  if(lookup_fail||key>=ems.size()||!ems[key].alive)return false;
  Em& m=ems[key];m.r=t.r;m.g=t.g;m.b=t.b;m.alpha=alpha;m.scale=t.scale;if(created_now&&resource_finite)m.continuous=true;
  rb.ok=!color_mismatch;rb.r=m.r;rb.g=m.g;rb.b=m.b;rb.a=m.alpha;return true;
 }
 void stop(std::uint32_t key){if(key<ems.size()){ems[key].stopped=true;ems[key].alpha=0;}++stops;}
 void engine_tick(){for(auto& m:ems){if(m.alive&&!m.refreshed){m.alive=false;++orphans;}m.refreshed=false;}}
 unsigned live()const{unsigned n=0;for(auto& m:ems)if(m.alive)++n;return n;}
};
struct FakeEye {unsigned tex=0;bool foreign=false;unsigned writes=0;};
struct Fx {
 wl::AuraPart<FakeEngine>& ap;FakeEye& eyes;const char* last="NONE";
 bool aura(vv::Entry& e,int l,int a){const bool ok=ap.aura(e,l,a);last=ap.why;return ok;}
 void aura_release(vv::Entry& e){ap.aura_release(e);}
 bool eye(vv::Entry&,unsigned idx){if(eyes.foreign){last="EYE_PACKET_TEXTURE_NOT_OURS";return false;}eyes.tex=idx;++eyes.writes;return true;}
 const char* reason()const{return last;}
};
struct Run {
 FakeEngine en;vv::Ledger L;std::array<wl::Retry,vv::kCapacity> retries{};FakeEye eyes;vv::Entry* e=nullptr;std::uint32_t id=1,gen=1;int notes_color=0;std::vector<wl::Note> notes;std::vector<vv::Event> events;
 explicit Run(Variant v,std::uint32_t id_=1){id=id_;e=L.record(id,this,gen,v);require(e!=nullptr,"record");}
 void tick(bool aura,bool eyes_want=false){
  wl::Retry& rt=wl::retry_for(retries,std::size_t(e-L.entries.data()),e->id,e->generation);
  wl::AuraPart<FakeEngine> ap{en,rt};ap.x=10;ap.y=20;ap.z=30;Fx fx{ap,eyes};
  const auto out=vv::step(L,*e,fx,aura,eyes_want);
  for(unsigned i=0;i<out.n;++i){events.push_back(out.ev[i]);}
  for(unsigned i=0;i<ap.notes.count;++i){notes.push_back(ap.notes.n[i]);}
  en.engine_tick();
 }
 void run(unsigned n,bool a,bool ey=false){for(unsigned i=0;i<n;++i)tick(a,ey);}
 unsigned count(vv::Ev k){unsigned n=0;for(auto& x:events)if(x.kind==k)++n;return n;}
 unsigned notes_of(wl::Note::Kind k){unsigned n=0;for(auto& x:notes)if(x.kind==k)++n;return n;}
};
int main(){
 // ---------- resource selection ----------
 {static_assert(wl::kResource==0x0046&&wl::kRetiredGlow==0x072F&&wl::kBank==0,"ids");require(wl::kResource!=0x8046&&(wl::kResource&0x8000)==0,"bank 0, not the scene-bank id");
  Run r(Variant::Blue);r.run(40,true);r.run(40,false);require(!r.en.ids.empty(),"engine was called");for(auto id:r.en.ids)require(id==0x0046&&id!=0x072F,"every set() uses 0x0046, never the retired 0x072F glow");
  Run k(Variant::Black);k.run(20,true,true);for(auto id:k.en.ids)require(id==0x0046,"Black too");
  pass("only_resource_0x0046_is_ever_passed_to_the_engine_never_0x072F_nor_0x8046");}
 // ---------- Blue lifecycle ----------
 {Run r(Variant::Blue);r.run(5,false);require(r.en.set_calls==0&&r.en.live()==0&&r.events.empty(),"inactive: engine untouched");
  r.run(12,true);require(r.e->aura_level==100&&r.en.live()==1&&r.en.created==1,"faded in over 10 ticks with ONE emitter");
  const auto& em=r.en.ems[r.e->key];require(em.r==110&&em.g==190&&em.b==255&&em.scale==1.5f&&em.alpha==wl::boosted_alpha(Variant::Blue,110)&&em.alpha==198,"Blue cyan tint, boosted alpha cap");
  require(r.notes_of(wl::Note::ColorApplied)==1&&r.notes_of(wl::Note::ColorUnavailable)==0,"colour readback reported once");
  r.run(20,true);require(r.en.created==1&&r.en.ems[r.e->key].refreshes>=30,"refreshed every tick, never recreated");
  unsigned ticks=0;while(r.e->aura_level>0&&ticks<40){r.tick(false);++ticks;}require(ticks==15,"15 ticks out");require(r.count(vv::Ev::AuraOff)==1&&r.e->key==0&&r.en.stops==1,"stopped once");
  r.run(3,false);require(r.en.live()==0&&r.en.orphans==1,"the engine removed the emitter nobody refreshes");
  pass("blue_windline_fades_in_10_out_15_one_emitter_cyan_tint_then_released");}
 {Run r(Variant::Blue);r.run(30,true);r.run(7,false);const int mid=r.e->aura_level;require(mid>0&&mid<100,"mid fade-out");const unsigned created=r.en.created;r.run(2,true);require(r.en.created==created&&r.e->aura_level>mid-14,"resume from the current level, same emitter");
  r.run(20,true);require(r.e->aura_level==100&&r.count(vv::Ev::AuraOff)==0,"never released in between");pass("blue_reactivation_during_fade_out_resumes_same_emitter");}
 // ---------- Black lifecycle ----------
 {Run r(Variant::Black);r.run(15,true,true);require(r.en.ems[r.e->key].r==185&&r.en.ems[r.e->key].b==255&&r.en.ems[r.e->key].alpha==234&&r.eyes.tex==7,"Black purple tint, stronger alpha, eyes purple");
  const unsigned created=r.en.created,ev=r.events.size();r.run(600,true,true);   // fury refreshes: the target simply stays on
  require(r.en.created==created&&r.events.size()==ev&&r.eyes.tex==7&&r.en.stops==0,"refresh: no new emitter, no event, no stop, no flicker");
  r.run(40,false,false);require(r.e->aura_level==0&&r.eyes.tex==0&&r.en.stops==1&&r.count(vv::Ev::EyesOff)==1,"fury over: aura stopped, eyes native");r.run(3,false);require(r.en.live()==0,"no residue");
  pass("black_windline_and_eyes_on_with_fury_stable_through_refresh_then_clean_off");}
 // ---------- transient engine refusals ----------
 {Run r(Variant::Blue);r.en.not_ready=5;r.run(3,true);require(r.en.live()==0&&r.e->aura_fault==false&&r.e->aura_live,"tevStr not ready: no emitter, no fault, logic stays on");
  r.run(10,true);require(r.en.live()==1&&r.notes_of(wl::Note::RetryStart)==1&&r.notes_of(wl::Note::Acquired)==1&&r.count(vv::Ev::AuraRejected)==0,"acquired after the retries: one RetryStart, one Acquired, no rejection");
  require(r.count(vv::Ev::AuraOn)==1&&r.en.ids.size()>0,"one ON event");require(r.en.set_calls<=r.en.created+14,"bounded attempts");
  pass("tevstr_not_ready_is_retried_not_a_permanent_fault");}
 {Run r(Variant::Blue);r.en.pool_full=true;r.run(100,true);
  require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault&&r.e->aura_level==0&&r.en.live()==0,"pool exhausted for good: rejected once, level 0");
  require(r.en.set_calls==wl::kRetryTicks+1&&r.notes_of(wl::Note::Exhausted)==1,"exactly kRetryTicks+1 attempts, then no more calls");
  const unsigned calls=r.en.set_calls;r.run(200,true);require(r.en.set_calls==calls&&r.count(vv::Ev::AuraRejected)==1,"no retries after the rejection, no log spam");
  pass("pool_exhaustion_is_bounded_then_rejected_once_for_the_generation");}
 {Run r(Variant::Blue);r.en.pool_full=true;r.run(30,true);require(r.en.live()==0&&!r.e->aura_fault,"still retrying");r.en.pool_full=false;r.run(3,true);
  require(r.en.live()==1&&r.notes_of(wl::Note::Acquired)==1&&r.count(vv::Ev::AuraRejected)==0&&wl::retry_for(r.retries,std::size_t(r.e-r.L.entries.data()),1,1).fails==0,"recovered: acquired, retry count reset");
  pass("recovery_when_the_pool_frees_up_within_the_retry_budget");}
 {Run r(Variant::Blue);r.en.bank_ok=false;r.run(100,true);require(r.en.ids.empty()&&r.en.set_calls==0,"bank not loaded: set() is never called");require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault,"rejected after the budget");
  Run q(Variant::Blue);q.en.bank_ok=false;q.run(10,true);q.en.bank_ok=true;q.run(5,true);require(q.en.live()==1,"bank appeared in time: works");
  pass("missing_bank_never_reaches_set_and_is_rejected_cleanly_or_recovers");}
 {Run r(Variant::Blue);r.en.lookup_fail=true;r.run(100,true);require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault,"lookup always failing: bounded, rejected once");
  Run q(Variant::Blue);q.en.color_mismatch=true;q.run(15,true);require(q.notes_of(wl::Note::ColorUnavailable)==1&&q.notes_of(wl::Note::ColorApplied)==0&&q.en.live()==1,"colour readback mismatch is reported (COLOR_UNAVAILABLE) but the emitter still works");
  pass("lookup_failure_bounded_and_colour_unavailable_reported_honestly");}
 {Run r(Variant::Blue);r.run(20,true);const auto k=r.e->key;r.en.ems[k].alive=false;   // the engine/another system removed it (event, scene change)
  r.run(3,true);require(r.e->key!=k&&r.en.live()==1&&r.en.created==2&&r.notes_of(wl::Note::ColorApplied)==2,"expired handle: exactly one new emitter, colour re-applied, never two alive");
  pass("expired_emitter_is_recreated_once_never_duplicated");}
 // ---------- ownership / cleanup / bounds / isolation ----------
 {Run a(Variant::Blue);a.run(10,true);std::array<wl::Retry,4> t{};auto& x=wl::retry_for(t,0,7,1);x.fails=9;require(wl::retry_for(t,0,7,1).fails==9,"same generation keeps it");require(wl::retry_for(t,0,7,2).fails==0&&wl::retry_for(t,0,8,2).fails==0,"new generation or id starts clean");
  pass("retry_state_is_stamped_by_id_and_generation");}
 {FakeEngine shared;vv::Ledger L;std::array<wl::Retry,vv::kCapacity> rt{};FakeEye eyes;unsigned rejected=0;std::vector<vv::Entry*> es;
  for(unsigned i=0;i<10;++i){static int dummy[16];es.push_back(L.record(100+i,&dummy[i],1,Variant::Blue));require(es.back(),"record");}
  for(unsigned t=0;t<40;++t){for(auto* e:es){wl::AuraPart<FakeEngine> ap{shared,wl::retry_for(rt,std::size_t(e-L.entries.data()),e->id,e->generation)};Fx fx{ap,eyes};const auto o=vv::step(L,*e,fx,true,false);for(unsigned i=0;i<o.n;++i)if(o.ev[i].kind==vv::Ev::AuraRejected)++rejected;}shared.engine_tick();}
  require(shared.live()==vv::kMaxAuras&&rejected==10-vv::kMaxAuras,"capacity is explicit: at most 8 emitters, the rest rejected cleanly");pass("emitter_capacity_is_bounded_and_explicit");}
 {vv::Ledger L;int d=0;require(L.record(1,&d,1,Variant::Red)==nullptr&&L.record(2,&d,1,Variant::None)==nullptr,"Red and unclassified never get a visual record");FakeEngine en;require(en.set_calls==0,"so the engine is never called for them");
  pass("red_and_unclassified_actors_get_no_aura");}
 {Run r(Variant::Black);r.en.pool_full=true;r.run(100,true,true);require(r.e->aura_fault&&r.eyes.tex==7&&r.count(vv::Ev::EyesRejected)==0,"WindLine failure: the eyes are not disabled, not touched");r.run(40,false,false);require(r.eyes.tex==0&&r.count(vv::Ev::EyesOff)==1,"and still return to native");
  Run q(Variant::Black);q.eyes.foreign=true;q.run(30,true,true);require(q.count(vv::Ev::EyesRejected)==1&&q.e->eye_fault&&q.en.live()==1&&!q.e->aura_fault,"eye failure: the aura keeps working");
  pass("aura_and_eye_faults_are_independent");}
 {Run r(Variant::Blue);r.run(20,true);vv::Entry copy=*r.e;require(r.en.live()==1,"live");   // destruction / room exit: nobody steps the entry any more
  r.en.engine_tick();r.en.engine_tick();require(r.en.live()==0&&r.en.orphans==1,"actor gone: the engine removes the emitter within one tick, nothing to unwind");(void)copy;
  Run s(Variant::Black);s.run(20,true,true);wl::Retry& rt=s.retries[0];FakeEye& ey=s.eyes;wl::AuraPart<FakeEngine> ap{s.en,rt};Fx fx{ap,ey};vv::cleanup(*s.e,fx,true);require(s.en.stops==1&&s.e->key==0&&ey.tex==0,"shutdown cleanup: aura stopped, eyes native");s.en.engine_tick();s.en.engine_tick();require(s.en.live()==0,"nothing left");
  pass("actor_gone_and_shutdown_leave_no_emitter");}
 {require(wl::boosted_alpha(Variant::Blue,0)==0&&wl::boosted_alpha(Variant::Blue,110)==198&&wl::boosted_alpha(Variant::Black,130)==234&&wl::boosted_alpha(Variant::Black,1000)==255,"alpha boost clamps");
  require(wl::kBlue.b==255&&wl::kBlue.r<wl::kBlue.b&&wl::kBlack.r>wl::kBlack.g&&wl::kBlack.b==255&&wl::kBlue.g!=wl::kBlack.g,"cyan-blue vs violet, distinct");pass("tuning_table_sane_and_distinct");}
 std::printf("WINDLINE_LAB_PASS cases=%u POLICY_ONLY; WINDLINE_LOOK_NOT_VERIFIED; ENGINE_AND_PIXELS_NOT_EXECUTED\n",cases);
}
