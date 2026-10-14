#pragma once
#include <cstdint>
namespace devf {
enum class EventKind { Delta, Increase, Suppressed, Summary, Rejected };
struct Observation {
 EventKind kind; std::uint32_t id; int before,after;
 std::uint64_t ordinal; unsigned emitted;
 const char* source; const char* classification; const char* reason;
};
// Scalars only: never stores an actor pointer, collider, callback or engine resource.
struct Observer {
 static constexpr unsigned kLimit=32,kDepth=8;
 struct Frame {std::uint32_t id=0; int before=0; bool eligible=false;};
 Frame frames[kDepth]{}; unsigned depth=0;
 std::uint32_t own_id=0; int last_hp=0; std::uint64_t changes=0;
 unsigned emitted=0; bool suppressed=false,active=false,fault=false;
 const char* fault_reason="NONE";
 template<class Emit> void reject(const char* reason,Emit emit) noexcept {
  if(!fault){fault=true;fault_reason=reason;emit(Observation{EventKind::Rejected,own_id,last_hp,last_hp,changes,emitted,"OBSERVER","UNKNOWN",reason});}
  active=false;
 }
 template<class Emit> bool push(Emit emit) noexcept {
  if(depth>=kDepth){++depth;reject("PAIR_DEPTH_EXCEEDED",emit);return false;}
  frames[depth++]={};return !fault;
 }
 Frame pending() const noexcept {return depth&&depth<=kDepth&&!fault?frames[depth-1]:Frame{};}
 template<class Emit> void summary(const char* reason,Emit emit) noexcept {
  if(active)emit(Observation{EventKind::Summary,own_id,last_hp,last_hp,changes,emitted,"OBSERVER","UNKNOWN",reason});
 }
 template<class Emit> void start(std::uint32_t id,int hp,Emit emit) noexcept {
  if(fault)return;
  for(unsigned i=0;i<depth&&i<kDepth;++i)if(frames[i].eligible){reject("GENERATION_CHANGED_INSIDE_PAIR",emit);return;}
  summary("NEXT_GENERATION",emit);
  if(fault)return;
  own_id=id;last_hp=hp;changes=0;emitted=0;suppressed=false;active=true;
 }
 template<class Emit> void change(int before,int after,const char* source,Emit emit) noexcept {
  if(!active||fault||before==after)return;
  if(changes!=UINT64_MAX)++changes;
  last_hp=after;
  if(emitted<kLimit){
   ++emitted;
   emit(Observation{after>before?EventKind::Increase:EventKind::Delta,own_id,before,after,changes,emitted,source,
    after>before?"UNKNOWN_INCREASE":after==0?"UNKNOWN_OR_SPECIAL":"UNATTRIBUTED_HP_LOSS","NET_FIELD_CHANGE"});
  }else if(!suppressed){
   suppressed=true;emit(Observation{EventKind::Suppressed,own_id,before,after,changes,emitted,source,"UNKNOWN","DELTA_LIMIT_32"});
  }
 }
 template<class Emit> void capture(std::uint32_t id,int hp,Emit emit) noexcept {
  if(!active||fault||id!=own_id)return;
  if(!depth||depth>kDepth){reject("PRE_PAIR_MISSING",emit);return;}
  for(unsigned i=0;i+1<depth;++i)if(frames[i].eligible&&frames[i].id==id){reject("OWN_EXECUTE_REENTRANCY",emit);return;}
  // Catch changes outside our execute window honestly; never call them individual hits.
  change(last_hp,hp,"OBSERVED_BETWEEN_EXECUTES",emit);
  if(!fault)frames[depth-1]={id,hp,true};
 }
 template<class Emit> void complete(std::uint32_t id,int hp,bool matches,Emit emit) noexcept {
  if(!depth){reject("POST_PAIR_MISSING",emit);return;}
  if(depth>kDepth){--depth;return;}
  const Frame frame=frames[--depth];frames[depth]={};
  if(!frame.eligible||!active||fault)return;
  if(!matches||id!=frame.id||id!=own_id){reject("POST_IDENTITY_MISMATCH",emit);return;}
  change(frame.before,hp,"EXECUTE_WINDOW",emit);last_hp=hp;
 }
 template<class Emit> void shutdown(Emit emit) noexcept {
  summary("SHUTDOWN",emit);active=false;own_id=0;depth=0;
  for(auto& frame:frames)frame={};
 }
};
}
