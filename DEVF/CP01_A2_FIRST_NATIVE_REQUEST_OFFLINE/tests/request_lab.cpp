#include "fake.hpp"
#include <iostream>
#include <limits>
using namespace lab;
int count=0,failures=0;
void check(const char* name,bool result){++count;if(!result)++failures;std::cout<<(result?"PASS ":"FAIL ")<<name<<"\n";}
void start(Controller& c,Fake& b){c.attempt(b,permit(b.frame));}
int main(){
 {Controller c;Fake b;c.attempt(b,{});check("no-admission-no-reads-or-writes",b.calls==0&&b.sets==0&&b.reads==0&&c.state()==State::Blocked);}
 {Controller c;Fake b;auto a=permit(b.frame);a.session_authorized=false;c.attempt(b,a);check("no-session",b.calls==0&&b.sets==0);}
 const std::vector<std::pair<const char*,std::function<void(Fake&)>>> rejects={
 {"host",[](Fake& b){b.frame.owner.host_ok=false;}},{"thread",[](Fake& b){b.frame.owner.thread_ok=false;}},
 {"stage",[](Fake& b){b.frame.owner.stage[0]='X';}},{"room",[](Fake& b){b.frame.owner.room=4;}},{"logical",[](Fake& b){b.frame.owner.logical=5;}},
 {"registry",[](Fake& b){b.frame.owner.registry_ok=false;}},{"player",[](Fake& b){b.frame.owner.player_ok=false;}},
 {"node",[](Fake& b){b.frame.owner.node_complete=false;}},{"binding",[](Fake& b){b.frame.owner.binding_ok=false;}},
 {"play",[](Fake& b){b.frame.owner.play_ok=false;}},{"transition",[](Fake& b){b.frame.owner.transition=true;}},
 {"retiring",[](Fake& b){b.frame.owner.retiring=true;}},{"foreign-create",[](Fake& b){b.frame.owner.creates=1;}},
 {"delete",[](Fake& b){b.frame.owner.deletes=1;}},{"parent-create",[](Fake& b){b.frame.owner.parent_creates=1;}},
 {"parent-delete",[](Fake& b){b.frame.owner.parent_deletes=1;}},{"previous",[](Fake& b){b.frame.owner.previous_registered=false;}},
 {"event-active",[](Fake& b){b.frame.event_active=true;}},{"event-unreadable",[](Fake& b){b.frame.event_read=false;}},
 {"unstable",[](Fake& b){b.frame.stable=false;}},{"medium",[](Fake& b){b.frame.medium_observed=false;}},
 {"missing-E_OC",[](Fake& b){b.frame.registered_eoc=false;}},{"missing-E_ocb",[](Fake& b){b.frame.registered_weapon=false;}},
 {"missing-service",[](Fake& b){b.service=false;}},{"child-is-root",[](Fake& b){b.frame.owner.child=0;}},
 {"child-is-parent",[](Fake& b){b.frame.owner.child=5;}}};
 for(auto& [name,mutate]:rejects){Controller c;Fake b;mutate(b);start(c,b);check(name,b.calls==0&&b.sets==0&&!c.consumed());}
 for(double bad:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),std::numeric_limits<double>::max(),1.0}){Controller c;Fake b;auto a=permit(b.frame);a.xyz[0]=bad;c.attempt(b,a);check("float-or-candidate-rejected",b.calls==0&&b.sets==0);}
 {Preset p;check("explicit-float32-exact-preset",convert_candidate(candidate,p)&&p.xyz[1]==static_cast<float>(candidate[1])&&p.angle[0]==-236&&p.argument==-1&&p.profile==0x1fe&&p.parameters==unknown);}
 {Controller c;Fake b;auto a=permit(b.frame);a.context.child=20;c.attempt(b,a);check("admission-bound-to-context",b.calls==0);}
 for(int tick:{2,3}){Controller c;Fake b;b.before_read=[tick](Fake& x){if(x.reads==tick)x.frame.owner.node++;};start(c,b);check("fresh-context-drift",b.calls==0&&c.state()==State::StopSession&&b.layer==0);}
 {Controller c;Fake b;b.before_read=[](Fake& x){if(x.reads==2)x.layer=42;};start(c,b);check("previous-drift",b.calls==0&&!c.consumed());}
 {Controller c;Fake b;bool latched=false;b.on_select=[&](Fake&){latched=c.consumed();};start(c,b);check("latch-before-first-setter",latched);check("exact-child-and-restored-previous",b.selections==std::vector<Id>{6,0}&&b.layer==0);check("one-real-backend-request",b.calls==1);check("MOD_OK-is-REQUESTED-not-ready",c.state()==State::Requested);check("native-preset",b.last.profile==0x1fe&&b.last.xyz[0]==static_cast<float>(candidate[0])&&b.last.room==3&&b.last.scale[0]==1);start(c,b);start(c,b);check("never-second-attempt",b.calls==1&&b.sets==2);}
 {Controller c;Fake b;b.frame.owner.child=46;b.frame.owner.parent=b.frame.owner.play_layer=b.frame.owner.link_owner=45;b.pending();start(c,b);check("ids-not-hardcoded-or-Link-owner",b.selections==std::vector<Id>{46,0}&&b.calls==1);}
 for(int mode=0;mode<7;++mode){Controller c;Fake b;
  if(mode==0)b.throw_select=true;if(mode==1)b.misselect=true;if(mode==2)b.throw_request=true;if(mode==3)b.result.result=-1;if(mode==4)b.result.id=unknown;if(mode==5)b.result.id=0;if(mode==6)b.result.id=0x7fffffff;
  start(c,b);start(c,b);check("restore-after-failed-consumed-attempt",b.layer==0&&c.consumed()&&c.state()==State::StopSession&&b.calls<=1&&b.sets==2);}
 for(int mode=0;mode<3;++mode){Controller c;Fake b;b.on_request=[mode](Fake& x){if(mode==0)x.previous_exists=false;if(mode==1)x.throw_restore=true;if(mode==2)x.misrestore=true;};start(c,b);start(c,b);check("restoration-failure-no-root-fallback",c.reason()==Reason::Restore&&b.calls==1&&c.consumed());}
 {Controller c;Fake b;b.on_request=[&](Fake& x){c.attempt(x,permit(x.frame));};start(c,b);check("reentry-sticky-restored",c.reason()==Reason::Reentry&&b.calls==1&&b.layer==0);}
 {Controller c;Fake b;b.on_select=[&](Fake& x){if(x.layer==0)c.attempt(x,permit(x.frame));};start(c,b);check("setter-reentry-no-request",c.state()==State::StopSession&&b.calls==0&&b.layer==0);}
 {Controller c;Fake b;start(c,b);c.poll(b);check("own-pending-count-not-foreign",c.state()==State::Pending);b.observation.resources_registered=true;b.observation.resources_ready=true;c.poll(b);check("registered-or-ready-resources-not-actor-ready",c.state()==State::Pending);b.ready();c.poll(b);check("complete-ID-phase-owner-room-ready",c.state()==State::ActorReadyObserved);}
 const std::vector<std::pair<const char*,std::function<void(Observation&)>>> badobs={
 {"wrong-id",[](Observation& a){a.id++;}},{"wrong-profile",[](Observation& a){a.profile++;}},
 {"wrong-owner",[](Observation& a){a.owner=5;}},{"wrong-room",[](Observation& a){a.room=4;}},{"room-API-failure",[](Observation& a){a.room_api_ok=false;}},
 {"duplicate-ID",[](Observation& a){a.ambiguous=true;}},{"recycled-ID",[](Observation& a){a.reused=true;}},
 {"truncated-actor-scan",[](Observation& a){a.actor_scan_complete=false;}},{"truncated-request-scan",[](Observation& a){a.request_scan_complete=false;}},
 {"removed",[](Observation& a){a.removed=true;}},{"cancelled-request",[](Observation& a){a.request_cancelled=true;}},
 {"deleting-phase",[](Observation& a){a.init=3;}},{"changed-room-context",[](Observation& a){a.frame.owner.room=4;}},
 {"changed-node",[](Observation& a){a.frame.owner.node++;}},{"changed-child",[](Observation& a){a.frame.owner.child++;}},
 {"transition-during-pending",[](Observation& a){a.frame.owner.transition=true;}},{"event-during-pending",[](Observation& a){a.frame.event_active=true;}},
 {"foreign-count-after-complete",[](Observation& a){a.frame.owner.creates=1;}},{"parent-count-after-complete",[](Observation& a){a.frame.owner.parent_creates=1;}},
 {"delete-after-complete",[](Observation& a){a.frame.owner.deletes=1;}}};
 for(auto& [name,mutate]:badobs){Controller c;Fake b;start(c,b);b.ready();mutate(b.observation);c.poll(b);start(c,b);check(name,c.state()==State::StopSession&&b.calls==1&&b.layer==0);}
 for(int mode=0;mode<4;++mode){Controller c;Fake b;start(c,b);b.ready();if(mode==0)b.observation.init=1;if(mode==1)b.observation.actor_init=false;if(mode==2)b.observation.derived_read=false;if(mode==3){b.pending();b.observation.actor_present=true;b.observation.profile=0x1fe;b.observation.init=1;}c.poll(b);check("incomplete-header-no-ready",c.state()==State::Pending);}
 for(int mode=0;mode<3;++mode){Controller c;Fake b;start(c,b);if(mode==0)b.observation.request_owner=5;if(mode==1)b.observation.request_present=false;if(mode==2)b.observation.frame.owner.creates=2;c.poll(b);check("pending-queue-attribution-fail-closed",c.state()==State::StopSession&&b.calls==1);}
 {Controller c;Fake b;start(c,b);b.ready();b.observation.actor_present=false;for(int i=0;i<3;++i)c.poll(b);check("bounded-missing-grace",c.state()==State::Pending);c.poll(b);start(c,b);check("missing-expired-no-retry",c.reason()==Reason::MissingActor&&b.calls==1);}
 {Controller c(4);Fake b;start(c,b);for(int i=0;i<4;++i)c.poll(b);start(c,b);check("timeout-no-retry",c.reason()==Reason::Timeout&&b.calls==1);}
 {Controller c;Fake b;start(c,b);b.ready();c.poll(b);b.observation.actor_present=false;c.poll(b);check("early-destruction-after-ready-stop",c.state()==State::StopSession);}
 {Controller c;Fake b;start(c,b);b.ready();for(int i=0;i<61;++i)c.poll(b);check("finite-ready-observation",c.reason()==Reason::ObservationBudget&&b.calls==1);}
 {Controller c;Fake b;start(c,b);b.throw_observe=true;c.poll(b);start(c,b);check("monitor-exception-stop",c.reason()==Reason::Exception&&b.calls==1);}
 {Controller c;Fake b;start(c,b);c.shutdown();start(c,b);c.poll(b);check("shutdown-no-cancel-delete-or-rearm",c.reason()==Reason::Shutdown&&b.calls==1);}
 std::cout<<"SUMMARY checks="<<count<<" failures="<<failures<<" runtime=NOT_TESTED\n";return failures?1:0;
}
