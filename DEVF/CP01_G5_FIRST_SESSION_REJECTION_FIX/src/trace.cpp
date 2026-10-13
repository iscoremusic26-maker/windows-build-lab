#include "trace.hpp"
#include "policy.hpp"
#include "experiment_core.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
namespace devf::pilot {
const char* event_name(Event e) noexcept {
 switch(e){
#define E(x,s) case Event::x:return s
 E(DiagnosticObservation,"DIAGNOSTIC_OBSERVATION_NO_ADMISSION");E(ModuleInit,"MODULE_INIT");E(AbiVerified,"ABI_VERIFIED");E(AbiFailed,"ABI_FAILED");
 E(SessionAdmissionDenied,"SESSION_ADMISSION_DENIED");E(SessionGateDenied,"SESSION_GATE_DENIED");
 E(PreflightReject,"PREFLIGHT_REJECT");E(LocationPolicyPass,"LOCATION_POLICY_PASS");E(LocationPolicyFail,"LOCATION_POLICY_FAIL");
 E(FirstPrecheckPass,"FIRST_PRECHECK_PASS");E(FreshPrecheckPass,"FRESH_PRECHECK_PASS");E(AttemptConsumed,"ATTEMPT_CONSUMED");
 E(LayerSelected,"LAYER_SELECTED");E(CreateRequestBegin,"CREATE_REQUEST_BEGIN");E(CreateRequestResult,"CREATE_REQUEST_RESULT");
 E(LayerRestoreOk,"LAYER_RESTORE_OK");E(RestoreFailed,"RESTORE_FAILED");E(Requested,"REQUESTED");E(Pending,"PENDING");E(PendingPhase,"PENDING_PHASE");
 E(ActorReadyObserved,"ACTOR_READY_OBSERVED");E(ReadyMonitorBudgetExhausted,"READY_MONITOR_BUDGET_EXHAUSTED");E(StopSession,"STOP_SESSION");
 E(FullExitRequired,"FULL_EXIT_REQUIRED_ON_UNCERTAINTY");E(Shutdown,"SHUTDOWN");E(LogBudgetExhausted,"LOG_BUDGET_EXHAUSTED");E(LogSinkFailed,"LOG_SINK_FAILED");
#undef E
 }return "UNKNOWN";
}
namespace {
unsigned category(Event e) noexcept {
 switch(e){case Event::AbiFailed:case Event::RestoreFailed:case Event::StopSession:case Event::FullExitRequired:
 case Event::Shutdown:case Event::LogBudgetExhausted:case Event::LogSinkFailed:return 2;
 case Event::AttemptConsumed:case Event::LayerSelected:case Event::CreateRequestBegin:case Event::CreateRequestResult:
 case Event::LayerRestoreOk:case Event::Requested:case Event::Pending:case Event::ActorReadyObserved:
 case Event::ReadyMonitorBudgetExhausted:return 1;
 default:return 0;}
}
void increment(unsigned& n)noexcept{if(n<std::numeric_limits<unsigned>::max())++n;}
unsigned axis_class(double x)noexcept {return !std::isfinite(x)?3u:x==0?0u:x<0?1u:2u;}
bool normal_signature(const Record& a,const Record& b)noexcept {
 return a.context==b.context&&a.stage==b.stage&&a.event==b.event&&a.reason==b.reason&&a.guard==b.guard&&a.node==b.node&&a.child==b.child&&a.previous==b.previous&&
 a.offender.id==b.offender.id&&a.offender.profile==b.offender.profile&&a.link_id==b.link_id&&a.link_profile==b.link_profile&&a.link_owner==b.link_owner&&
 a.known_global==b.known_global&&a.unknown_room==b.unknown_room&&a.speed_observed==b.speed_observed&&a.delta.state==b.delta.state&&
 axis_class(a.speed[0])==axis_class(b.speed[0])&&axis_class(a.speed[1])==axis_class(b.speed[1])&&axis_class(a.speed[2])==axis_class(b.speed[2])&&axis_class(a.speed_forward)==axis_class(b.speed_forward);
}
bool same_message(const Record& a,const Record& b) noexcept {
 // update/capture/distance drift is not a new rejection category; actual reasons/identities/phases are.
 const bool policy_event=a.event==Event::LocationPolicyPass||a.event==Event::LocationPolicyFail;
 return a.event==b.event&&(policy_event||a.state==b.state)&&a.reason==b.reason&&a.guard==b.guard&&a.request==b.request&&
        a.offender.id==b.offender.id&&a.actor_profile==b.actor_profile&&a.actor_owner==b.actor_owner&&
        a.phase==b.phase&&a.queued==b.queued&&a.actor_initialized==b.actor_initialized&&a.result==b.result&&a.node==b.node&&a.child==b.child;
}
const char* state_name(unsigned s) noexcept {
 using first::State;switch(State(s)){case State::Blocked:return "BLOCKED";case State::Rejected:return "REJECTED";
 case State::AttemptConsumed:return "ATTEMPT_CONSUMED";case State::Requested:return "REQUESTED";case State::Pending:return "PENDING";
 case State::ActorReadyObserved:return "ACTOR_READY_OBSERVED";case State::Failed:return "FAILED";case State::StopSession:return "STOP_SESSION";}return "UNKNOWN";
}
const char* reason_name(unsigned r) noexcept {
 using first::Reason;switch(Reason(r)){
#define R(x) case Reason::x:return #x
 R(None);R(Disarmed);R(Host);R(Thread);R(Context);R(Registry);R(Player);R(Node);R(Binding);R(Transition);R(PendingContext);R(Previous);
 R(Position);R(Revalidation);R(Reentry);R(Consumed);R(Selection);R(Service);R(InvalidId);R(Exception);R(Restore);R(Identity);R(Phase);
 R(MissingActor);R(Resources);R(Timeout);R(Removed);R(Ambiguous);R(Admission);R(Medium);R(Queue);R(CounterAttribution);R(ObservationBudget);R(Shutdown);R(PilotSafety);
#undef R
 }return "UNKNOWN";
}
}
bool Trace::append(Record r,bool deduplicate) noexcept {
 const unsigned idx=unsigned(r.event);if(idx>=last_.size())return false;
 const unsigned category_now=category(r.event);
 if(r.event==Event::PreflightReject&&r.reason==unsigned(first::Reason::PilotSafety)&&seen_[unsigned(Event::LocationPolicyFail)]){
  const auto& prev=last_[unsigned(Event::LocationPolicyFail)];
  if(prev.update==r.update&&prev.capture==r.capture&&prev.guard==r.guard){increment(coalesced_);return true;}
 }
 if(deduplicate&&category_now==0){
  for(unsigned i=0;i<normal_keys_count_;++i)if(normal_signature(normal_keys_[i],r)){last_[idx]=r;seen_[idx]=true;increment(dedup_skipped_);return true;}
  if(normal_keys_count_<normal_keys_.size())normal_keys_[normal_keys_count_++]=r;
 }else if(deduplicate&&seen_[idx]&&same_message(last_[idx],r)){increment(dedup_skipped_);return true;}
 last_[idx]=r;seen_[idx]=true;const unsigned cat=category(r.event);
 const bool room=cat==0?normal_<normal_limit:cat==1?critical_<critical_limit:terminal_<terminal_limit;
 if(!room||size_==capacity){increment(suppressed_);increment(suppressed_class_[cat]);budget_marker();return false;}
 if(cat==0)++normal_;else if(cat==1)++critical_;else ++terminal_;
 r.event_sequence=++sequence_;r.suppressed=suppressed_;r.sink_failures=failed_;r.suppressed_class=suppressed_class_;r.coalesced=coalesced_;r.dedup_skipped=dedup_skipped_;records_[size_++]=r;return true;
}
void Trace::budget_marker() noexcept {
 if(marker_)return;marker_=true;Record r;r.event=Event::LogBudgetExhausted;r.update=update_;append(r,false);
}
void Trace::record(Event e,const first::Controller& c,const first::Frame* f,const first::Observation* o,int result) noexcept {
 Record r;r.event=e;r.update=update_;r.state=unsigned(c.state());r.reason=unsigned(c.reason());r.guard=c.policy_reason();
 r.attempt=c.consumed()?1u:0u;r.request=c.id();r.polls=c.polls();r.result=result;
 r.node=c.origin().node;r.child=c.origin().child;r.previous=c.origin().previous;r.room=c.origin().room;
 if(f){r.context={f->owner.node,f->owner.child,f->owner.play,f->owner.parent,f->owner.room,f->owner.logical};r.stage=f->owner.stage;r.capture=f->pilot.sequence==unknown?0:f->pilot.sequence;r.node=f->owner.node;r.child=f->owner.child;
  r.previous=c.consumed()?c.origin().previous:f->owner.previous;r.room=f->owner.room;
  const auto d=assess(*f);r.link=d.link;r.offender=d.offender;r.offender_distance=d.offender_metrics;
  r.speed_observed=f->pilot.link_read&&f->pilot.link_profile_checked&&f->pilot.link.initialized&&f->pilot.link.init==2;
  r.speed=f->pilot.link.speed;r.speed_forward=f->pilot.link.speed_forward;
  if(temporal_update_!=f->pilot.update_index){delta_=temporal_.observe(*f);temporal_update_=f->pilot.update_index;}
  r.delta=delta_;
  for(unsigned i=0;i<f->pilot.count&&i<actor_cap;++i){auto scope=room_scope(f->pilot.actors[i],*f);if(scope==RoomScope::KnownStageKYTAG04&&f->pilot.actors_complete&&!f->pilot.actors_truncated&&!f->pilot.duplicates&&f->pilot.visited==f->pilot.count&&f->pilot.actors[i].id!=0&&f->pilot.actors[i].id!=unknown&&f->pilot.actors[i].init==2&&f->pilot.actors[i].initialized&&metrics(f->pilot.actors[i].xyz).valid&&metrics(f->pilot.actors[i].xyz).xz>=actor_min_xz)++r.known_global;else if(scope==RoomScope::Invalid)++r.unknown_room;}
  if(e==Event::DiagnosticObservation){r.guard=d.reason;r.reason=unsigned(first::validate_owner(f->owner));}
  r.link_id=f->pilot.link.id;r.link_profile=f->pilot.link.profile;r.link_owner=f->pilot.link.owner;r.link_room=f->pilot.link.room;
  r.policy_approved=d.allowed&&r.guard==GuardReason::None&&c.reason()==first::Reason::None&&e!=Event::PreflightReject&&e!=Event::DiagnosticObservation;
 }
 if(o){r.actor_profile=o->profile;r.actor_owner=o->owner;r.phase=o->init;r.room=o->room;
  r.queued=(e==Event::Pending||e==Event::Requested)&&o->creating&&o->request_present;
  r.actor_initialized=(e==Event::ActorReadyObserved||e==Event::ReadyMonitorBudgetExhausted)&&c.state()==first::State::ActorReadyObserved;}
 if(e==Event::Requested)r.queued=true;
 append(r,true);
}
void Trace::capture(const first::Frame& f)noexcept {first::Controller inert;record(Event::DiagnosticObservation,inert,&f);}
bool format_record(const Record& r,char* out,std::size_t cap) noexcept {
 if(!cap)return false;
 const char* evidence=r.event==Event::PendingPhase?"UNVALIDATED_PHASE_SCALARS":r.actor_initialized?"ACTOR_INITIALIZED":r.queued?"REQUEST_QUEUED":r.policy_approved?"APPROVED_BY_POLICY":"NO_ADMISSION_PROOF";
 char raw[4][32];const double values[4]={r.speed[0],r.speed[1],r.speed[2],r.speed_forward};unsigned zero_mask=0;
 for(unsigned i=0;i<4;++i){if(r.speed_observed&&std::isfinite(values[i])){std::snprintf(raw[i],sizeof(raw[i]),"%.9g",values[i]);if(values[i]==0)zero_mask|=1u<<i;}else std::snprintf(raw[i],sizeof(raw[i]),"UNAVAILABLE");}
 const int n=std::snprintf(out,cap,
  "DEVF_PILOT event=%s state=%s reason=%s policy_reason=%s event_seq=%llu update_index=%llu capture_sequence=%u attempt_id=%u request_id=%u node=%u child=%u previous=%u room=%d mod_result=%d phase=%d actor_profile=%u actor_owner=%u actor_INIT=%d queue=%d polls=%u measurement=%s link_id=%u link_profile=%u link_room=%d link_owner=%u link_xz=%.6f link_d3=%.6f link_dy=%.6f offender_id=%u offender_profile=%u offender_room=%d offender_owner=%u offender_xz=%.6f evidence=%s candidate=LOCATION_CANDIDATE_CONDITIONAL location_safe=UNVERIFIED resource_ready=NOT_VERIFIED render=NOT_TESTED profile=510 parameters=4294967295 argument=-1 C_f32=%.7f,%.7f,%.7f suppressed=%u sink_failures=%u speed_observed=%d speed_source=fopAc_ac_c link_speed_x=%s link_speed_y=%s link_speed_z=%s link_speedF=%s exact_zero_mask=%u axis_class=%u,%u,%u,%u delta_state=%u delta_valid=%d delta_previous_update=%llu delta_gap=%llu delta_xyz=%.9g,%.9g,%.9g actor_global_scope_allowed=%d known_global_count=%u unknown_actor_rejected=%d suppressed_normal=%u suppressed_critical=%u suppressed_terminal=%u coalesced=%u dedup_skipped=%u suppression_reason=CLASS_QUOTA_OR_CAP link_ground=NOT_OBSERVED_SAFE_API idle_policy=EXACT_ZERO_UNCALIBRATED",
  event_name(r.event),state_name(r.state),reason_name(r.reason),guard_name(r.guard),
  static_cast<unsigned long long>(r.event_sequence),static_cast<unsigned long long>(r.update),r.capture,r.attempt,r.request,r.node,r.child,r.previous,r.room,
  r.result,r.phase,r.actor_profile,r.actor_owner,r.actor_initialized,r.queued,r.polls,r.link.valid?"OBSERVED_SCALARS":"NOT_VERIFIED",r.link_id,r.link_profile,r.link_room,r.link_owner,
  r.link.xz,r.link.d3,r.link.dy,r.offender.id,r.offender.profile,r.offender.room,r.offender.owner,r.offender_distance.xz,evidence,spawn[0],spawn[1],spawn[2],r.suppressed,r.sink_failures,r.speed_observed,raw[0],raw[1],raw[2],raw[3],zero_mask,
 axis_class(values[0]),axis_class(values[1]),axis_class(values[2]),axis_class(values[3]),unsigned(r.delta.state),r.delta.valid,
 static_cast<unsigned long long>(r.delta.previous_update),static_cast<unsigned long long>(r.delta.gap),r.delta.xyz[0],r.delta.xyz[1],r.delta.xyz[2],r.known_global>0,r.known_global,r.unknown_room>0,r.suppressed_class[0],r.suppressed_class[1],r.suppressed_class[2],r.coalesced,r.dedup_skipped);
 if(n<0||std::size_t(n)>=cap){out[cap-1]=0;return false;}return true;
}
void Trace::drain(Sink& sink) noexcept {
 if(draining_)return;draining_=true;struct End{bool& b;~End(){b=false;}} done{draining_};
 const unsigned end=size_;bool failed_now=false;
 while(read_<end){char line[2304];const auto& r=records_[read_++];bool ok=false;
  if(format_record(r,line,sizeof(line))){try{ok=sink.write(line);}catch(...){ok=false;}}
  if(!ok){increment(failed_);failed_now=true;}
 }
 if(failed_now&&!sink_marker_){sink_marker_=true;Record r;r.event=Event::LogSinkFailed;r.update=update_;append(r,false);}
 // Failure marker is attempted on a later drain; no retry loop, exceptions never escape.
}
}
