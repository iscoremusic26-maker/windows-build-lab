#pragma once
#include <array>
#include <cstdint>
#include <type_traits>
#if !defined(DEVF_FIRST_OFFLINE) || DEVF_FIRST_OFFLINE != 1
#error "Intentional experimental offline target only; never a READ_ONLY observer"
#endif
namespace devf::first {
using Id=std::uint32_t;inline constexpr Id unknown=0xffffffffu;
inline constexpr std::array<double,3> candidate{-4806.7346,-6729.480961460304,65135.8281};
enum class Reason { None,Disarmed,Host,Thread,Context,Registry,Player,Node,Binding,Transition,PendingContext,Previous,Position,Revalidation,Reentry,Consumed,Selection,Service,InvalidId,Exception,Restore,Identity,Phase,MissingActor,Resources,Timeout,Removed,Ambiguous,Admission,Medium,Queue,CounterAttribution,ObservationBudget,Shutdown };
struct Owner {
    bool host_ok=false, thread_ok=false, registry_ok=false, player_ok=false;
    bool node_present=false, node_profile=false, node_complete=false, node_id_ok=false;
    bool child_registered=false, binding_ok=false, play_ok=false;
    bool previous_registered=false, transition=false, retiring=false;
    std::array<char,9> stage{};
    int room=-1, logical=-1, next_room=-1;
    Id node=unknown, child=unknown, parent=unknown, play=unknown, play_layer=unknown;
    Id previous=unknown, root=unknown, link_owner=unknown;
    int creates=-1, deletes=-1, parent_creates=-1, parent_deletes=-1;
};

struct Frame {
 Owner owner{};bool event_read=false,event_active=false,stable=false,medium_observed=false;
 bool registered_eoc=false,registered_weapon=false; // never means resource/GPU READY
};
struct Key { Id node=unknown,child=unknown,play=unknown,parent=unknown;int room=-1,logical=-1;bool operator==(const Key&) const=default; };
Key key(const Owner&) noexcept;
Reason validate_owner(const Owner&) noexcept;
bool context_valid(const Frame&) noexcept;
bool same_context(const Owner&,const Owner&) noexcept;
struct Admission {
 bool location_admitted=false,session_authorized=false;
 Key context{};std::array<double,3> xyz=candidate;
}; // No runtime setter/config/UI exists. Native provider in admission.cpp always returns NONE.
Admission current_admission() noexcept;
struct Preset {
 Id profile=0x1fe,parameters=0xffffffffu;int argument=-1,room=3;
 std::array<float,3> xyz{},scale{1,1,1};std::array<std::int16_t,3> angle{-236,0,0};
};
bool convert_candidate(const std::array<double,3>&,Preset&) noexcept;
struct RequestResult { int result=-1;Id id=unknown; }; // exact MOD_OK numeric0
struct Observation {
 Frame frame{};Id id=unknown,profile=unknown,owner=unknown,request_owner=unknown;int room=-1,init=-1;
 bool creating=false,request_present=false,actor_present=false,actor_init=false,derived_read=false;
 bool actor_scan_complete=false,request_scan_complete=false,ambiguous=false,reused=false,removed=false;
 bool resources_registered=false,resources_ready=false; // no inference from registered
 bool room_api_ok=false,request_cancelled=false;
};
class Backend {
public:
 virtual ~Backend()=default;
 virtual bool service_available() const=0;
 virtual Frame preflight()=0;
 virtual Id current()=0;
 virtual bool registered(Id)=0;
 virtual void select(Id)=0;
 virtual RequestResult request(const Preset&)=0;
 virtual Observation observe(Id)=0;
};
class LayerScope {
 Backend& backend_;Id previous_;bool restored_=false,ok_=false;
public:
 LayerScope(Backend& b,Id previous):backend_(b),previous_(previous){}
 bool select(Id target){backend_.select(target);return backend_.current()==target;}
 bool restore() noexcept;
 ~LayerScope(){restore();}
};
enum class State { Blocked,Rejected,AttemptConsumed,Requested,Pending,ActorReadyObserved,Failed,StopSession };
class Controller {
 State state_=State::Blocked;Reason reason_=Reason::Admission;
 bool consumed_=false,busy_=false,ready_seen_=false;Id id_=unknown;Owner origin_{};
 unsigned polls_=0,missing_=0,timeout_,ready_polls_=0;
 bool terminal() const noexcept { return state_==State::StopSession||state_==State::Failed; }
public:
 explicit Controller(unsigned timeout=120):timeout_(timeout?timeout:1){}
 State state()const{return state_;}Reason reason()const{return reason_;}
 bool consumed()const{return consumed_;}bool busy()const{return busy_;}Id id()const{return id_;}
 unsigned polls()const{return polls_;}const Owner& origin()const{return origin_;}
 void stop(Reason r) noexcept {state_=State::StopSession;reason_=r;}
 void attempt(Backend&,const Admission&) noexcept;
 void poll(Backend&) noexcept;
 void shutdown() noexcept {stop(Reason::Shutdown);}
};
static_assert(std::is_trivially_copyable_v<Frame> && std::is_trivially_copyable_v<Observation>);
Frame read_only_preflight(); // compiled behind unchanged historical READ_ONLY/A2 guards
}
