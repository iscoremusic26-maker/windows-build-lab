// Reuse the exact prior fake/fixtures; NEVER call the prior 104-check main.
#define main historical_lab_not_executed
#include "../../CP01_A2_FIRST_PILOT_SAFETY_AND_TRACE/tests/pilot_lab.cpp"
#undef main
#include "package_compatibility.hpp"
#include <cstddef>
static_assert(sizeof(ActorSpawnParams)==48&&sizeof(ActorService)==80&&sizeof(ServiceHeader)==8&&sizeof(ProfileName)==2&&sizeof(ActorId)==4&&sizeof(ModContext*)==8);
static_assert(offsetof(ActorService,create_actor)==32&&offsetof(ActorService,get_actor_room_num)==64);
int main(){
 auto permit=current_admission();
 check("real-provider-technically-armed-live-key",permit.location_admitted&&permit.session_authorized&&permit.bind_context_live&&permit.xyz==candidate&&permit.context==Key{});
 {Controller c;Fake b;c.attempt(b,permit);check("real-provider-one-request-three-fresh-captures",b.calls==1&&b.reads==3&&b.sets==2&&c.state()==State::Requested);check("exact-preset-C-profile-parameters-angle-scale",b.last.profile==510&&b.last.parameters==unknown&&b.last.argument==-1&&b.last.room==3&&b.last.angle==std::array<std::int16_t,3>{-236,0,0}&&b.last.scale==std::array<float,3>{1,1,1}&&std::array<double,3>{b.last.xyz[0],b.last.xyz[1],b.last.xyz[2]}==p::spawn);check("MOD_OK-ID-is-not-READY",c.id()==200&&c.state()!=State::ActorReadyObserved);c.attempt(b,permit);check("real-provider-no-second-request",b.calls==1);c.shutdown();c.attempt(b,permit);check("same-image-shutdown-never-rearms",b.calls==1&&c.reason()==Reason::Shutdown);}
 for(int k=0;k<2;++k){Controller c;Fake b;auto a=permit;if(k)a.location_admitted=false;else a.session_authorized=false;c.attempt(b,a);check("denied-zero-reads-setters-requests-"+std::to_string(k),b.reads==0&&b.sets==0&&b.calls==0);}
 for(int k=0;k<3;++k){Controller c;Fake b;if(k==0)b.frame.owner.stage[0]='X';if(k==1)b.frame.owner.room=4;if(k==2)b.frame.owner.logical=5;c.attempt(b,permit);check("wrong-stage-room-layer-"+std::to_string(k),b.sets==0&&b.calls==0);}
 for(double x:{1599.999,1600.0,1600.001}){Controller c;Fake b;b.move(x);c.attempt(b,permit);check("real-provider-Link-XZ-"+std::to_string(x),b.calls==(x>=1600?1:0));}
 {Controller c;Fake b;b.move(100,9000);c.attempt(b,permit);check("real-provider-d3-cannot-replace-XZ",b.calls==0);}
 {Controller c;Fake b;b.frame.pilot.link.speed_forward=1;c.attempt(b,permit);check("moving-Link-denied",b.calls==0&&b.sets==0);}
 {Controller c;Fake b;b.add_actor(5000);b.frame.pilot.actors[1].owner_valid=false;c.attempt(b,permit);check("unknown-global-actor-denied",b.calls==0);}
 {Controller c;Fake b;b.frame.pilot.actors_truncated=true;c.attempt(b,permit);check("truncated-global-census-denied",b.calls==0);}
 for(int phase:{2,3}){Controller c;Fake b;b.before_read=[phase](lab::Fake& b){if(b.reads==phase)b.frame.owner.child++;};c.attempt(b,permit);check("changed-Key-phase-"+std::to_string(phase),b.calls==0&&c.state()==State::StopSession&&b.layer==0);}
 {Controller c;Fake b;b.stale=true;c.attempt(b,permit);check("stale-A-B-denied",b.calls==0&&c.state()==State::StopSession);}
 {Controller c;Fake b;b.add_actor(5000);b.before_read=[](lab::Fake& b){if(b.reads==3)b.frame.pilot.actors[1].xyz[0]++;};c.attempt(b,permit);check("C-actor-drift-STOP",b.calls==0&&b.layer==0&&c.state()==State::StopSession);}
 for(int phase:{1,2,3}){Controller c;Fake b;b.before_read=[&](lab::Fake& b){if(b.reads==phase)c.attempt(b,permit);};c.attempt(b,permit);check("reentry-phase-"+std::to_string(phase),b.calls==0&&c.state()==State::StopSession&&c.reason()==Reason::Reentry&&b.layer==0);}
 {Controller c;Fake b;b.on_select=[&](lab::Fake&){c.attempt(b,permit);};c.attempt(b,permit);check("reentry-after-latch-no-request",b.calls==0&&c.consumed()&&b.layer==0);}
 {Controller c;Fake b;b.on_request=[&](lab::Fake&){c.attempt(b,permit);};c.attempt(b,permit);check("request-reentry-no-double",b.calls==1&&b.layer==0&&c.state()==State::StopSession);}
 for(int k=0;k<3;++k){Controller c;Fake b;if(k==0)b.throw_select=true;if(k==1)b.previous_exists=false;if(k==2)b.on_request=[](lab::Fake& b){b.throw_restore=true;};c.attempt(b,permit);c.attempt(b,permit);check("scope-error-terminal-"+std::to_string(k),b.calls<=(k==2?1:0)&&c.state()==State::StopSession);}
 {Controller c(2);Fake b;c.attempt(b,permit);c.poll(b);c.poll(b);c.attempt(b,permit);check("timeout-no-retry",b.calls==1&&c.reason()==Reason::Timeout);}
 {Controller c;Fake b;b.result.result=-1;c.attempt(b,permit);c.attempt(b,permit);check("service-error-no-retry",b.calls==1&&c.reason()==Reason::Service);}
 {Controller c;Fake b;p::Trace t;c.attach_trace(&t);t.record(p::Event::PackageProviderArmed,c);c.attempt(b,permit);Sink sink;sink.fail=true;t.drain(sink);check("arming-trace-honest-and-sink-failure-no-bypass",has(t,p::Event::PackageProviderArmed)&&t.sink_failures()>0&&b.calls==1&&b.layer==0);}
 {Controller c;Fake b;p::Trace t;c.attach_trace(&t);for(unsigned i=0;i<40;++i){b.move(100);b.frame.pilot.link.id=100+i;t.record(p::Event::PreflightReject,c,&b.frame);}b.move(2000);c.attempt(b,permit);check("normal-log-saturation-reserves-request-restore",has(t,p::Event::LogBudgetExhausted)&&has(t,p::Event::CreateRequestBegin)&&has(t,p::Event::LayerRestoreOk)&&b.calls==1);}
 HostService h{};LogService l{};GameService g{};ActorService a{};
 h.header=SERVICE_HEADER(HostService,2,2);l.header=SERVICE_HEADER(LogService,1,0);g.header=SERVICE_HEADER(GameService,2,0);a.header=SERVICE_HEADER(ActorService,1,0);
 h.version="2.0.3";l.write=+[](ModContext*,LogLevel,const char*){};
 a.create_actor=+[](ModContext*,ProfileName,const ActorSpawnParams*,ActorId*){return MOD_ERROR;};a.get_actor_room_num=+[](ModContext*,ActorId,int8_t*){return MOD_ERROR;};
 check("actual-production-service-contract-positive",devf::package::contracts_available(&h,&l,&g,&a));
 for(int k=0;k<10;++k){auto hh=h;auto ll=l;auto gg=g;auto aa=a;switch(k){case 0:hh.version="2.0.4";break;case 1:hh.header.minor_version=1;break;case 2:ll.header.major_version=2;break;case 3:gg.header.major_version=1;break;case 4:aa.header.major_version=2;break;case 5:aa.create_actor=nullptr;break;case 6:aa.get_actor_room_num=nullptr;break;case 7:ll.write=nullptr;break;case 8:aa.header.struct_size=8;break;case 9:hh.version=nullptr;break;}check("production-contract-fail-closed-"+std::to_string(k),!devf::package::contracts_available(&hh,&ll,&gg,&aa));}
 check("null-service-fail-closed",!devf::package::contracts_available(nullptr,&l,&g,&a));
 std::cout<<"SUMMARY active-provider-incremental-checks="<<checks<<" failures="<<failures<<" historical104-not-executed native-runtime=NOT_TESTED\n";return failures?1:0;
}
