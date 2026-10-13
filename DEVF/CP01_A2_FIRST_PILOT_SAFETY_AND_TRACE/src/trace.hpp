#pragma once
#include "policy_values.hpp"
#include <array>
#include <cstddef>
namespace devf::first {class Controller;struct Frame;struct Observation;}
namespace devf::pilot {
enum class Event:unsigned { ModuleInit,AbiVerified,AbiFailed,SessionAdmissionDenied,SessionGateDenied,
 PreflightReject,LocationPolicyPass,LocationPolicyFail,FirstPrecheckPass,FreshPrecheckPass,
 AttemptConsumed,LayerSelected,CreateRequestBegin,CreateRequestResult,LayerRestoreOk,RestoreFailed,
 Requested,Pending,PendingPhase,ActorReadyObserved,ReadyMonitorBudgetExhausted,StopSession,
 FullExitRequired,Shutdown,LogBudgetExhausted,LogSinkFailed };
const char* event_name(Event) noexcept;
struct Record {
 Event event=Event::ModuleInit;unsigned state=0,reason=0;GuardReason guard=GuardReason::None;
 std::uint64_t update=0,event_sequence=0;Id capture=0,request=unknown,node=unknown,child=unknown,previous=unknown;
 unsigned attempt=0,polls=0,suppressed=0,sink_failures=0;int result=-1,phase=-1,room=-1;
 bool queued=false,actor_initialized=false,policy_approved=false;
 Id actor_profile=unknown,actor_owner=unknown,link_id=unknown,link_profile=unknown,link_owner=unknown;int link_room=-1;Metrics link{},offender_distance{};ActorSample offender{};
};
class Sink {public:virtual ~Sink()=default;virtual bool write(const char*)=0;};
class Trace {
public:
 static constexpr unsigned normal_limit=24,critical_limit=32,terminal_limit=8,capacity=64;
private:
 std::array<Record,capacity> records_{};unsigned size_=0,read_=0,normal_=0,critical_=0,terminal_=0,suppressed_=0,failed_=0;
 std::uint64_t update_=0,sequence_=0;bool marker_=false,sink_marker_=false,draining_=false;
 std::array<Record,26> last_{};std::array<bool,26> seen_{};
 bool append(Record,bool deduplicate) noexcept;
 void budget_marker() noexcept;
public:
 void set_update(std::uint64_t u) noexcept{update_=u;}
 void record(Event,const first::Controller&,const first::Frame* =nullptr,const first::Observation* =nullptr,int result=-1) noexcept;
 void drain(Sink&) noexcept; // outside critical scope; failures never change controller behavior
 unsigned emitted_records()const noexcept{return size_;}
 unsigned suppressed()const noexcept{return suppressed_;}
 unsigned sink_failures()const noexcept{return failed_;}
 const Record& at(unsigned i)const noexcept{return records_[i];}
};
bool format_record(const Record&,char*,std::size_t) noexcept;
}
