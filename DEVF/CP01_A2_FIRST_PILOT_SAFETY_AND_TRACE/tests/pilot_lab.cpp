#include "fake.hpp" // generated, byte-identical fixture from checkpoint #16
#include "policy.hpp"
#include "trace.hpp"
#include "census_values.hpp"
#include <iostream>
#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <vector>
using namespace devf::first;
namespace p=devf::pilot;
namespace {
unsigned checks=0,failures=0;
void check(const std::string& name,bool ok){++checks;if(!ok)++failures;std::cout<<(ok?"PASS ":"FAIL ")<<name<<'\n';}
struct Fake:lab::Fake {
 bool stale=false,stamp_valid=true;Id sequence=0;
 Fake(){auto& v=frame.pilot;v.sequence=1;v.update_index=1;v.coherent=true;v.link_read=v.link_profile_checked=true;
  v.actors_complete=v.bg_complete=v.resources_read=true;v.water_status=2;
  v.ground={1,p::spawn[1],0,2174,3,0,0,false};
  v.link.id=105;v.link.profile=253;v.link.owner=frame.owner.link_owner;v.link.room=3;v.link.init=2;
  v.link.initialized=v.link.owner_valid=v.link.is_link=true;move(2000);v.count=v.visited=1;
  v.context={frame.owner.node,frame.owner.child,frame.owner.play,frame.owner.parent,3,6};}
 void move(double xz,double dy=0){frame.pilot.link.xyz={p::spawn[0]+xz,p::spawn[1]+dy,p::spawn[2]};frame.pilot.actors[0]=frame.pilot.link;}
 void add_actor(double xz,double dy=0){auto& v=frame.pilot;auto a=v.link;a.id=305;a.profile=100;a.is_link=false;a.xyz={p::spawn[0]+xz,p::spawn[1]+dy,p::spawn[2]};v.actors[v.count++]=a;v.visited=v.count;}
 Frame preflight()override{auto f=lab::Fake::preflight();f.pilot.sequence=stale?1:++sequence;
  if(stamp_valid)f.pilot.context={f.owner.node,f.owner.child,f.owner.play,f.owner.parent,f.owner.room,f.owner.logical};return f;}
};
Admission allowed(){Admission a;a.location_admitted=a.session_authorized=a.bind_context_live=true;return a;}
struct Sink:p::Sink {
 std::vector<std::string> lines;bool fail=false,throw_write=false;
 bool write(const char* s)override{if(throw_write)throw std::runtime_error("synthetic sink failure");if(fail)return false;lines.emplace_back(s);return true;}
};
std::vector<p::Event> events(const p::Trace& t){std::vector<p::Event> out;for(unsigned i=0;i<t.emitted_records();++i)out.push_back(t.at(i).event);return out;}
bool has(const p::Trace& t,p::Event e){auto v=events(t);return std::find(v.begin(),v.end(),e)!=v.end();}
bool subsequence(const p::Trace& t,const std::vector<p::Event>& expected){unsigned j=0;for(auto e:events(t))if(j<expected.size()&&e==expected[j])++j;return j==expected.size();}
void start(Controller& c,Fake& b){c.attempt(b,allowed());}
}
int main(){
 {Controller c;Fake b;auto a=current_admission();c.attempt(b,a);check("native-provider-session-denied-with-zero-reads-setters-requests",!a.session_authorized&&b.reads==0&&b.sets==0&&b.calls==0);}
 {Controller c;Fake b;auto a=allowed();a.session_authorized=false;c.attempt(b,a);check("candidate-policy-never-grants-session",b.reads==0&&b.sets==0&&b.calls==0);}
 {Controller c;Fake b;start(c,b);check("positive-live-context-single-request",b.calls==1&&b.sets==2&&b.layer==0&&c.state()==State::Requested);check("preset-float32-angle-parameters-unchanged",b.last.profile==0x1fe&&b.last.parameters==unknown&&b.last.argument==-1&&b.last.room==3&&b.last.angle==std::array<std::int16_t,3>{-236,0,0}&&b.last.scale==std::array<float,3>{1,1,1}&&double(b.last.xyz[1])==p::spawn[1]);}
 {Controller c;Fake b;b.frame.owner.node=202;b.frame.owner.child=46;b.frame.owner.play=160;b.frame.owner.parent=b.frame.owner.play_layer=b.frame.owner.link_owner=45;b.frame.pilot.link.owner=45;b.frame.pilot.actors[0]=b.frame.pilot.link;b.pending();start(c,b);check("key-bound-from-fresh-A-not-historical-ids",b.calls==1&&c.origin().node==202&&c.origin().child==46&&b.selections==std::vector<Id>{46,0});}
 {Controller c;Fake b;auto a=allowed();a.bind_context_live=false;a.context={102,6,60,5,3,6};b.frame.owner.node=202;c.attempt(b,a);check("explicit-key-mismatch-not-bypassed",b.calls==0&&b.sets==0);}
 for(double xz:{1599.999,1600.0,1600.001}){Controller c;Fake b;b.move(xz);start(c,b);check("horizontal-threshold-"+std::to_string(xz),b.calls==(xz>=1600?1:0));}
 {Controller c;Fake b;b.move(100,9000);start(c,b);check("large-deltaY-does-not-admit-small-XZ",b.calls==0&&c.policy_reason()==p::GuardReason::LinkTooClose);auto m=p::metrics(b.frame.pilot.link.xyz);check("metrics-XZ-d3-deltaY-independent",m.valid&&m.xz==100&&m.d3>1600&&m.dy==9000);}
 const std::vector<std::pair<const char*,std::function<void(Fake&)>>> invalid={
  {"link-absent",[](Fake& b){b.frame.pilot.link_read=false;}},
  {"link-profile-unchecked",[](Fake& b){b.frame.pilot.link_profile_checked=false;}},
  {"link-init-incomplete",[](Fake& b){b.frame.pilot.link.init=1;}},
  {"link-actorINIT-absent",[](Fake& b){b.frame.pilot.link.initialized=false;}},
  {"link-owner-invalid",[](Fake& b){b.frame.pilot.link.owner_valid=false;}},
  {"link-owner-mismatch",[](Fake& b){b.frame.pilot.link.owner=42;}},
  {"link-room-wrong",[](Fake& b){b.frame.pilot.link.room=4;}},
  {"link-ID-unknown",[](Fake& b){b.frame.pilot.link.id=unknown;}},
  {"link-NaN",[](Fake& b){b.frame.pilot.link.xyz[0]=std::numeric_limits<double>::quiet_NaN();}},
  {"link-Inf",[](Fake& b){b.frame.pilot.link.xyz[2]=std::numeric_limits<double>::infinity();}},
  {"link-speed-unknown",[](Fake& b){b.frame.pilot.link.speed_forward=std::numeric_limits<double>::quiet_NaN();}},
  {"link-speed-nonzero",[](Fake& b){b.frame.pilot.link.speed[2]=2;}},
  {"link-forward-speed",[](Fake& b){b.frame.pilot.link.speed_forward=1;}},
  {"census-incomplete",[](Fake& b){b.frame.pilot.actors_complete=false;}},
  {"census-truncated",[](Fake& b){b.frame.pilot.actors_truncated=true;}},
  {"census-duplicate-flag",[](Fake& b){b.frame.pilot.duplicates=true;}},
  {"census-empty",[](Fake& b){b.frame.pilot.count=0;}},
  {"census-count-mismatch",[](Fake& b){b.frame.pilot.visited=2;}},
  {"BG-unavailable",[](Fake& b){b.frame.pilot.bg_complete=false;}},
  {"BG-truncated",[](Fake& b){b.frame.pilot.bg_truncated=true;}},
  {"BG-near-C",[](Fake& b){b.frame.pilot.bg_candidates=1;}},
  {"water-hit",[](Fake& b){b.frame.pilot.water_status=1;}},
  {"water-unavailable",[](Fake& b){b.frame.pilot.water_status=4;}},
  {"ground-poly",[](Fake& b){b.frame.pilot.ground.poly++;}},
  {"ground-Y",[](Fake& b){b.frame.pilot.ground.y+=1;}},
  {"ground-room",[](Fake& b){b.frame.pilot.ground.room=4;}},
  {"ground-code",[](Fake& b){b.frame.pilot.ground.ground=1;}},
  {"ground-special",[](Fake& b){b.frame.pilot.ground.special=1;}},
  {"ground-moving",[](Fake& b){b.frame.pilot.ground.moving=true;}},
  {"ground-BG-not-zero",[](Fake& b){b.frame.pilot.ground.bg=1;}},
  {"resources-unreadable",[](Fake& b){b.frame.pilot.resources_read=false;}},
  {"E_OC-unregistered",[](Fake& b){b.frame.registered_eoc=false;}},
  {"E_ocb-unregistered",[](Fake& b){b.frame.registered_weapon=false;}},
  {"event",[](Fake& b){b.frame.event_active=true;}},
  {"next-room",[](Fake& b){b.frame.owner.next_room=4;}},
  {"capture-not-coherent",[](Fake& b){b.frame.pilot.coherent=false;}},
  {"capture-stamp-mismatch",[](Fake& b){b.stamp_valid=false;b.frame.pilot.context.node=999;}},
  {"unidentified-far-actor",[](Fake& b){b.add_actor(5000);b.frame.pilot.actors[1].owner_valid=false;}},
  {"far-actor-NaN",[](Fake& b){b.add_actor(5000);b.frame.pilot.actors[1].xyz[1]=std::numeric_limits<double>::quiet_NaN();}},
  {"actor-with-duplicate-ID",[](Fake& b){b.add_actor(5000);b.frame.pilot.actors[1].id=b.frame.pilot.link.id;}}
 };
 for(const auto& [name,mutate]:invalid){Controller c;Fake b;mutate(b);start(c,b);check(name,b.calls==0&&b.sets==0&&!c.consumed());}
 for(double xz:{200.0,1599.0,1600.0,5000.0}){Controller c;Fake b;b.add_actor(xz,8000);start(c,b);check("other-actor-XZ-"+std::to_string(xz),b.calls==(xz>=1600?1:0));}
 for(int phase:{2,3}){Controller c;Fake b;b.before_read=[phase](lab::Fake& base){auto& x=static_cast<Fake&>(base);if(x.reads==phase)x.move(200);};start(c,b);check("Link-far-then-near-phase-"+std::to_string(phase),b.calls==0&&b.layer==0&&c.state()==State::StopSession&&c.consumed()==(phase==3));}
 for(int phase:{2,3}){Controller c;Fake b;b.before_read=[phase](lab::Fake& base){auto& x=static_cast<Fake&>(base);if(x.reads==phase)x.move(2500);};start(c,b);check("Link-position-drift-even-if-still-far-"+std::to_string(phase),b.calls==0&&c.policy_reason()==p::GuardReason::LinkChanged);}
 {Controller c;Fake b;b.stale=true;start(c,b);check("reused-capture-sequence-denied-before-latch",b.calls==0&&!c.consumed()&&c.policy_reason()==p::GuardReason::CaptureStale);}
 {Controller c;Fake b;b.before_read=[](lab::Fake& x){if(x.reads==2)x.frame.pilot.update_index++;};start(c,b);check("mixed-update-captures-denied",b.calls==0&&c.policy_reason()==p::GuardReason::CaptureStale);}
 {Controller c;Fake b;b.add_actor(5000);b.before_read=[](lab::Fake& x){if(x.reads==3)x.frame.pilot.actors[1].xyz[0]+=1;};start(c,b);check("far-actor-motion-between-B-and-C-denied",b.calls==0&&b.layer==0&&c.policy_reason()==p::GuardReason::ActorChanged);}
 for(int phase:{2,3}){Controller c;Fake b;b.before_read=[phase](lab::Fake& x){if(x.reads==phase)x.frame.owner.child++;};start(c,b);check("scene-child-drift-phase-"+std::to_string(phase),b.calls==0&&b.layer==0);}
 for(double x:{1.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),std::numeric_limits<double>::max()}){Controller c;Fake b;auto a=allowed();a.xyz[0]=x;c.attempt(b,a);check("candidate-nonexact-or-nonfinite-denied",b.calls==0&&b.sets==0);}
 {Controller c;Fake b;bool latched=false;b.on_select=[&](lab::Fake&){latched=c.consumed();};start(c,b);start(c,b);check("latch-before-setter-and-never-second-request",latched&&b.calls==1&&b.selections==std::vector<Id>{6,0});}
 for(int mode=0;mode<4;++mode){Controller c;Fake b;if(mode==0)b.throw_select=true;if(mode==1)b.throw_request=true;if(mode==2)b.result.result=-1;if(mode==3)b.result.id=unknown;start(c,b);start(c,b);check("incremental-restore-error-"+std::to_string(mode),c.consumed()&&b.layer==0&&c.state()==State::StopSession&&b.calls<=1);}
 {Controller c;Fake b;b.frame.owner.previous=77;b.layer=77;b.pending();start(c,b);check("previous-not-root-and-not-Link-owner-restored",b.calls==1&&b.selections==std::vector<Id>{6,77}&&b.layer==77);}
 for(int phase:{1,2}){Controller c;Fake b;b.before_read=[&](lab::Fake& x){if(x.reads==phase)c.attempt(b,allowed());};start(c,b);check("reentry-during-preflight-cannot-revive-STOP-"+std::to_string(phase),c.reason()==Reason::Reentry&&c.state()==State::StopSession&&b.calls==0&&b.sets==0&&!c.consumed());}
 {Controller c;Fake b;b.on_request=[&](lab::Fake&){c.poll(b);};start(c,b);check("reentry-sticky-scope-restored",b.calls==1&&b.layer==0&&c.reason()==Reason::Reentry);}
 {Controller c;Fake b;b.on_request=[](lab::Fake& x){x.throw_restore=true;};start(c,b);start(c,b);check("failed-restore-no-root-fallback-no-retry",b.calls==1&&c.reason()==Reason::Restore&&b.layer==6);}
 {Controller c;Fake b;start(c,b);c.poll(b);check("own-pending-queue-not-conflict",c.state()==State::Pending);b.ready();c.poll(b);check("ready-needs-identity-phase-owner-room",c.state()==State::ActorReadyObserved);b.observation.actor_present=false;c.poll(b);check("early-removal-after-ready-stop",c.state()==State::StopSession);}
 for(int mode=0;mode<5;++mode){Controller c;Fake b;start(c,b);b.ready();if(mode==0)b.observation.id++;if(mode==1)b.observation.owner=5;if(mode==2)b.observation.room=4;if(mode==3)b.observation.ambiguous=true;if(mode==4)b.observation.frame.owner.transition=true;c.poll(b);check("incremental-ID-monitor-failure-"+std::to_string(mode),c.state()==State::StopSession&&b.calls==1);}
 {Controller c(2);Fake b;start(c,b);c.poll(b);c.poll(b);b.ready();c.poll(b);start(c,b);check("late-actor-after-timeout-not-rearmed",c.reason()==Reason::Timeout&&b.calls==1);}
 {Controller c;p::Trace t;c.attach_trace(&t);t.set_update(1);Fake b;start(c,b);c.poll(b);b.ready();c.poll(b);c.shutdown();
  check("exact-critical-event-order-positive",subsequence(t,{p::Event::FirstPrecheckPass,p::Event::FreshPrecheckPass,p::Event::AttemptConsumed,p::Event::LayerSelected,p::Event::CreateRequestBegin,p::Event::CreateRequestResult,p::Event::LayerRestoreOk,p::Event::Requested,p::Event::Pending,p::Event::ActorReadyObserved,p::Event::Shutdown}));
  Sink sink;t.drain(sink);check("all-lines-parseable-prefix-render-not-tested",!sink.lines.empty()&&std::all_of(sink.lines.begin(),sink.lines.end(),[](const std::string& s){return s.starts_with("DEVF_PILOT ")&&s.find("render=NOT_TESTED")!=s.npos&&s.find("resource_ready=NOT_VERIFIED")!=s.npos&&s.find("RENDERED") == s.npos&&s.find("0x")==s.npos&&s.find("\\") == s.npos;}));
  check("trace-cannot-alter-latch-or-restore",b.calls==1&&b.layer==0&&c.consumed());
  for(const auto& line:sink.lines)std::cout<<"SYNTHETIC_LAB "<<line<<'\n';
 }
 {Controller c;p::Trace t;c.attach_trace(&t);Fake b;b.move(100);for(unsigned i=0;i<100;++i){t.set_update(i+1);start(c,b);}check("stable-rejections-deduplicated",t.emitted_records()==2&&b.calls==0&&b.sets==0);}
 {Controller c;p::Trace t;c.attach_trace(&t);t.set_update(1);Fake b;auto variants=invalid;
  for(auto& [name,mutate]:variants){Fake bad;mutate(bad);start(c,bad);}
  check("normal-truncation-signalled",has(t,p::Event::LogBudgetExhausted)&&t.suppressed()>0);
  start(c,b);c.shutdown();check("critical-request-restoration-reserved-after-normal-saturation",has(t,p::Event::CreateRequestBegin)&&has(t,p::Event::CreateRequestResult)&&has(t,p::Event::LayerRestoreOk)&&has(t,p::Event::StopSession)&&has(t,p::Event::Shutdown));
  check("trace-total-budget-finite",t.emitted_records()<=p::Trace::capacity);
 }
 for(int mode=0;mode<2;++mode){Controller c;p::Trace t;c.attach_trace(&t);Fake b;start(c,b);Sink sink;sink.fail=mode==0;sink.throw_write=mode==1;t.drain(sink);check("sink-failure-does-not-change-controller-"+std::to_string(mode),b.calls==1&&b.layer==0&&c.state()==State::Requested&&t.sink_failures()>0&&has(t,p::Event::LogSinkFailed));}
 {Controller c;p::Trace t;c.attach_trace(&t);Fake b;b.result={-1,201};start(c,b);check("MOD_ERROR-traced-with-returned-ID",has(t,p::Event::CreateRequestResult)&&c.id()==201&&c.reason()==Reason::Service&&has(t,p::Event::FullExitRequired));bool exact=false;for(unsigned i=0;i<t.emitted_records();++i)if(t.at(i).event==p::Event::CreateRequestResult)exact=t.at(i).result==-1&&t.at(i).request==201;check("exact-ModResult-and-ID-in-result-record",exact);}
 {Controller c;p::Trace t;c.attach_trace(&t);Fake b;b.on_request=[](lab::Fake& x){x.previous_exists=false;};start(c,b);check("failed-restore-record-and-full-exit-instruction",has(t,p::Event::RestoreFailed)&&has(t,p::Event::StopSession)&&has(t,p::Event::FullExitRequired)&&b.calls==1);}
 {Controller c;p::Trace t;c.attach_trace(&t);Fake b;start(c,b);b.ready();for(unsigned i=0;i<61;++i)c.poll(b);check("ready-monitor-budget-transition-traced",has(t,p::Event::ActorReadyObserved)&&has(t,p::Event::ReadyMonitorBudgetExhausted)&&c.reason()==Reason::ObservationBudget);c.shutdown();start(c,b);check("same-image-shutdown-does-not-reset-attempt",b.calls==1&&c.reason()==Reason::Shutdown);}
 {Controller c;p::Trace t;t.record(p::Event::ModuleInit,c);struct RecursiveSink:p::Sink{p::Trace& trace;unsigned calls=0;explicit RecursiveSink(p::Trace& t):trace(t){}bool write(const char*)override{++calls;trace.drain(*this);return true;}} sink(t);t.drain(sink);check("recursive-sink-drain-bounded",sink.calls==1&&t.sink_failures()==0);}
 {p::CensusValues v;v.reset(1,1);p::ActorSample a;a.id=10;v.add(a);v.add(a);check("same-visitor-scalar-collector-detects-ID-duplicates",v.values().duplicates);for(unsigned i=2;i<257;++i){a.id=i+20;v.add(a);}check("same-visitor-scalar-collector-cap256",v.values().count==256&&v.values().actors_truncated);}
 std::cout<<"SUMMARY pilot-new-and-affected-checks="<<checks<<" failures="<<failures<<" native-runtime=NOT_TESTED historical93-35-not-rerun\n";return failures?1:0;
}
