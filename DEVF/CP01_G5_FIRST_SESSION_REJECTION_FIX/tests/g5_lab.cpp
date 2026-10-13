#define main historical_104_not_run
#include "../../CP01_A2_FIRST_PILOT_SAFETY_AND_TRACE/tests/pilot_lab.cpp"
#undef main
#include "temporal.hpp"
#include <mods/svc/actor.h>
#include <cstddef>
static_assert(sizeof(ActorSpawnParams)==48&&sizeof(ActorService)==80&&offsetof(ActorService,create_actor)==32);
namespace {
void global(Fake& b,double xz=99755.529511,Id id=662){b.add_actor(xz);auto& a=b.frame.pilot.actors[b.frame.pilot.count-1];a.id=id;a.profile=686;a.room=-1;a.group=0;}
bool deny(Fake& b){Controller c;c.attempt(b,allowed());return b.calls==0&&b.sets==0&&!c.consumed();}
Frame sample(Fake& b,std::uint64_t update){auto f=b.preflight();f.pilot.update_index=update;return f;}
}
int main(){
 {Controller c;Fake b;c.attempt(b,current_admission());check("S01-native-NONE-zero-read-setter-request",b.reads==0&&b.sets==0&&b.calls==0);}
 {Controller c;Fake b;global(b);c.attempt(b,allowed());check("C01-C18-known686-dynamicID-stage-owner-far",b.calls==1&&b.frame.pilot.count==2&&c.origin().parent==b.frame.owner.parent);}
 for(double x:{1599.999,1600.0,1600.001}){Controller c;Fake b;global(b,x);c.attempt(b,allowed());check("C02-C03-known686-XZ-"+std::to_string(x),b.calls==(x>=1600?1:0)&&(x>=1600||c.policy_reason()==p::GuardReason::ActorNearC));}
 for(int k=0;k<13;++k){Fake b;global(b);auto& a=b.frame.pilot.actors[1];switch(k){case 0:a.profile=687;break;case 1:a.profile=510;break;case 2:a.room=-2;break;case 3:a.owner_valid=false;break;case 4:a.owner=0xffffffffu;break;case 5:a.owner=b.frame.owner.child;break;case 6:a.group=2;break;case 7:a.group=-1;break;case 8:a.id=0;break;case 9:a.id=unknown;break;case 10:a.init=1;break;case 11:a.initialized=false;break;case 12:a.is_link=true;break;}check("C04-07-C12-C17-room-metadata-denial-"+std::to_string(k),deny(b));}
 for(int k=0;k<3;++k){Fake b;global(b);if(k==0)b.frame.pilot.actors[1].id=b.frame.pilot.link.id;if(k==1)b.frame.pilot.actors_truncated=true;if(k==2)b.frame.pilot.visited++;check("C08-duplicate-truncated-visited-"+std::to_string(k),deny(b));}
 for(double x:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),double(std::numeric_limits<float>::max())*2}){Fake b;global(b);b.frame.pilot.actors[1].xyz[0]=x;check("C09-invalid-coordinate",deny(b));}
 for(int field=0;field<5;++field){Controller c;Fake b;global(b);b.before_read=[field](lab::Fake& b){if(b.reads==3){auto& a=b.frame.pilot.actors[1];if(field==0)a.xyz[0]++;if(field==1)a.id++;if(field==2)a.profile++;if(field==3)a.owner++;if(field==4)a.group++;}};c.attempt(b,allowed());check("C10-global-row-freshness-"+std::to_string(field),b.calls==0&&b.layer==0&&c.state()==State::StopSession);}
 {Fake b;global(b);global(b,90000,663);b.add_actor(100);check("C11-known-global-never-early-success",deny(b));}
 {Fake b;global(b);b.frame.pilot.link.room=-1;b.frame.pilot.link.profile=686;b.frame.pilot.actors[0]=b.frame.pilot.link;check("C12-Link-no-global-exemption",deny(b));}
 for(unsigned n:{256u,257u}){Fake b;for(unsigned i=1;i<256;++i){b.add_actor(5000+i);b.frame.pilot.actors[i].id=1000+i;}if(n==257)b.frame.pilot.actors_truncated=true;Controller c;c.attempt(b,allowed());check("C13-cap-"+std::to_string(n),b.calls==(n==256?1:0));}
 for(int room:{0,3,63}){Fake b;b.add_actor(5000);b.frame.pilot.actors[1].room=room;Controller c;c.attempt(b,allowed());check("C14-ordinary-room-"+std::to_string(room),b.calls==1);}
 {Fake b;b.add_actor(100);b.frame.pilot.actors[1].room=4;check("C16-other-room-near-world-denied",deny(b));}
 for(int k=0;k<3;++k){Controller c;Fake b;global(b);b.before_read=[k](lab::Fake& b){if(b.reads==2){if(k==0)b.frame.owner.stage[0]='X';if(k==1)b.frame.owner.parent++;if(k==2)b.frame.owner.transition=true;}};c.attempt(b,allowed());check("C15-stage-owner-transition-freshness-"+std::to_string(k),b.calls==0&&c.state()==State::StopSession);}
 {Fake b;global(b);check("L01-exact-zero-not-moving",p::assess(b.preflight()).allowed);}
 for(int k=0;k<4;++k){Fake b;if(k<3)b.frame.pilot.link.speed[k]=0.001;else b.frame.pilot.link.speed_forward=0.001;check("L02-05-nonzero-axis-"+std::to_string(k),deny(b));}
 for(double x:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),double(std::numeric_limits<float>::max())*2}){Fake b;b.frame.pilot.link.speed_forward=x;check("L06-L16-invalid-speedF",deny(b));}
 {Fake b;b.frame.pilot.link.speed[1]=std::numeric_limits<float>::denorm_min();check("L08-no-fabricated-epsilon-for-microresidue",deny(b));}
 for(int phase:{2,3}){Controller c;Fake b;b.before_read=[phase](lab::Fake& b){if(b.reads==phase)b.frame.pilot.link.speed[1]=-3;};c.attempt(b,allowed());check("L11-speed-changes-phase-"+std::to_string(phase),b.calls==0&&b.layer==0&&c.state()==State::StopSession);}
 for(int k=0;k<5;++k){Controller c;Fake b;b.before_read=[k](lab::Fake& b){if(b.reads==3){auto& l=b.frame.pilot.link;if(k==0)l.id++;if(k==1)l.profile++;if(k==2)l.owner++;if(k==3)l.room++;if(k==4)l.xyz[0]++;}};c.attempt(b,allowed());check("L09-10-Link-C-drift-"+std::to_string(k),b.calls==0&&c.state()==State::StopSession&&b.layer==0);}
 for(double x:{1589.428288,1600.0,3027.075419}){Fake b;b.move(x);Controller c;c.attempt(b,allowed());check("L14-G5-distance-"+std::to_string(x),b.calls==(x>=1600?1:0));}
 {Fake b;b.move(100,9000);check("L15-d3-not-XZ",deny(b));}
 {Preset p0;check("L16-exact-float32-C",convert_candidate(candidate,p0)&&std::array<double,3>{p0.xyz[0],p0.xyz[1],p0.xyz[2]}==p::spawn);}
 {p::Temporal t;Fake b;auto f=sample(b,10);auto d=t.observe(f);check("L12-temporal-no-previous",!d.valid);f=sample(b,11);f.pilot.link.xyz[0]+=2;f.pilot.actors[0]=f.pilot.link;d=t.observe(f);check("L07-two-distinct-updates-real-delta",d.valid&&d.gap==1&&d.xyz[0]==2);d=t.observe(f);check("L12-same-update-is-not-idle-window",!d.valid&&d.state==p::DeltaState::SameUpdate);f.pilot.update_index=14;d=t.observe(f);check("L12-skipped-update-no-delta-certification",!d.valid&&d.state==p::DeltaState::Skipped);f.pilot.update_index=13;d=t.observe(f);check("L12-rewind-resets",!d.valid&&d.state==p::DeltaState::Rewind);}
 for(int k=0;k<5;++k){p::Temporal t;Fake b;t.observe(sample(b,10));auto f=sample(b,11);if(k==0)f.owner.transition=true;if(k==1)f.event_active=true;if(k==2)f.pilot.actors_truncated=true;if(k==3)f.pilot.link_read=false;if(k==4)f.owner.stage[0]='X';check("L12-13-invalid-context-reset-"+std::to_string(k),!t.observe(f).valid&&!t.observe(sample(b,12)).valid);}
 {Controller c;Fake b;b.move(100);c.attempt(b,allowed());b.move(2000);c.attempt(b,allowed());check("S02-A-reject-no-consume-later-evaluation",b.calls==1&&c.consumed());}
 for(int phase:{1,2,3}){Controller c;Fake b;b.before_read=[&](lab::Fake& b){if(b.reads==phase)c.attempt(b,allowed());};c.attempt(b,allowed());check("S04-reentry-"+std::to_string(phase),b.calls==0&&b.layer==0&&c.reason()==Reason::Reentry);}
 {Controller c;Fake b;bool latch=false;b.on_select=[&](lab::Fake&){latch=c.consumed();};c.attempt(b,allowed());c.attempt(b,allowed());check("S05-single-request-latch-before-setter",latch&&b.calls==1&&b.sets==2&&b.layer==0);c.shutdown();c.attempt(b,allowed());check("S10-same-image-never-rearms",b.calls==1);}
 for(int k=0;k<4;++k){Controller c;Fake b;if(k==0)b.result.id=unknown;if(k==1)b.result.id=0;if(k==2)b.result.id=0x7ffffffeu;if(k==3)b.throw_request=true;c.attempt(b,allowed());c.attempt(b,allowed());check("S07-request-errors-no-retry-"+std::to_string(k),b.calls==1&&b.layer==0&&c.state()==State::StopSession);}
 for(int k=0;k<3;++k){Controller c;Fake b;c.attempt(b,allowed());b.ready();if(k==0)b.observation.profile++;if(k==1)b.observation.room++;if(k==2)b.observation.owner++;c.poll(b);check("S08-monitor-identity-"+std::to_string(k),c.state()==State::StopSession);}
 for(int k=0;k<6;++k){Fake b;if(k==0)b.frame.registered_eoc=false;if(k==1)b.frame.registered_weapon=false;if(k==2)b.frame.pilot.bg_complete=false;if(k==3)b.frame.pilot.ground.poly++;if(k==4)b.frame.pilot.water_status=0;if(k==5)b.frame.owner.stage[0]='X';check("S09-S11-existing-medium-context-"+std::to_string(k),deny(b));}
 {Controller c;Fake b;b.service=false;c.attempt(b,allowed());check("S12-service-unavailable-denied",b.calls==0&&b.sets==0);}
 {p::Trace t;Controller c;Fake b;c.attach_trace(&t);global(b);for(unsigned i=1;i<=1000;++i){t.set_update(i);b.frame.pilot.update_index=i;b.frame.pilot.link.speed[1]=(i%2)?-1:0;b.frame.pilot.actors[1].room=(i%2)?-1:-2;c.attempt(b,allowed());}check("T01-T04-oscillation-dedup-bounded",t.emitted_records()<12&&b.calls==0&&b.sets==0);check("T04-policy-preflight-pairs-coalesced",t.at(t.emitted_records()-1).coalesced>0);}

 {p::Trace t;Fake b;global(b);b.frame.pilot.link.speed[1]=-3;auto f=sample(b,1);f.pilot.actors[0]=f.pilot.link;t.set_update(1);t.capture(f);f=sample(b,2);f.pilot.link.speed[1]=-3;f.pilot.actors[0]=f.pilot.link;t.set_update(2);t.capture(f);Sink sink;t.drain(sink);check("T02-T03-raw-speed-global-and-delta-fields",!sink.lines.empty()&&sink.lines[0].find("link_speed_y=-3")!=std::string::npos&&sink.lines[0].find("known_global_count=1")!=std::string::npos&&sink.lines[0].find("DIAGNOSTIC_OBSERVATION_NO_ADMISSION")!=std::string::npos);for(auto& l:sink.lines)std::cout<<"SYNTHETIC_LAB "<<l<<'\n';}
 {Controller c;p::Trace t;c.attach_trace(&t);Fake b;for(unsigned i=0;i<100;++i){auto f=b.preflight();f.owner.node+=i;t.record(p::Event::PreflightReject,c,&f);}c.attempt(b,allowed());c.shutdown();check("T05-T07-critical-terminal-reserved",has(t,p::Event::CreateRequestBegin)&&has(t,p::Event::CreateRequestResult)&&has(t,p::Event::LayerRestoreOk)&&has(t,p::Event::StopSession)&&has(t,p::Event::Shutdown));check("T07-suppression-by-class-summary",t.suppressed()>0&&t.at(t.emitted_records()-1).suppressed_class[0]>0&&t.at(t.emitted_records()-1).suppressed_class[1]==0);}
 for(int k=0;k<2;++k){Controller c;p::Trace t;Fake b;c.attach_trace(&t);c.attempt(b,allowed());Sink sink;sink.fail=k==0;sink.throw_write=k==1;t.drain(sink);check("T08-sink-never-changes-control-"+std::to_string(k),b.calls==1&&b.layer==0&&t.sink_failures()>0);}
 {Controller c;p::Trace t;t.record(p::Event::ModuleInit,c);char shortline[8];check("T08-format-truncation-detected",!p::format_record(t.at(0),shortline,sizeof(shortline)));}
 {Controller c;p::Trace t;Fake b;auto f=b.preflight();t.record(p::Event::PreflightReject,c,&f);f.owner.node++;t.record(p::Event::PreflightReject,c,&f);check("T09-distinct-Key-not-hidden",t.emitted_records()==2);}
 std::cout<<"SUMMARY G5-new-affected-checks="<<checks<<" failures="<<failures<<" historical104-not-run game-runtime=NOT_TESTED delivery=DISARMED_DIAGNOSTIC\n";return failures?1:0;
}
