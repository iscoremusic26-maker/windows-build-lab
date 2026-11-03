#pragma once
// General Link mechanism. Deliberately no enemy kind, ID or combo dependency.
#include <cstdint>
namespace shieldchain {
inline constexpr unsigned kBufferTicks=6,kPermitTicks=30,kVisibleAdvance=1;
inline constexpr float kRecoveryFrame=12.f;
struct Counters {
 std::uint64_t input_new=0,buffered=0,native_starts=0,untrusted=0,motion=0,matched=0,rearms=0;
 std::uint64_t duplicate=0,rejected_motion=0,expired=0,cancelled=0,failed_native=0;
};
struct Episode {
 std::uint64_t serial=0,input=0,tick=0;float initial_frame=0;bool consumed=false;
};
class Chain {
 public:
 Counters c{};
 std::uint32_t id=0,epoch=0,stage=0;std::uint64_t input_serial=0,native_serial=0,input_tick=0,permit_tick=0,buffer_tick=0,permit_serial=0;
 bool input_r=false,armed=false,buffer=false,pass_valid=false;std::uint64_t pass_tick=0,pass_serial=0;
 std::uint64_t last_sample=0,last_used_input=0;Episode episodes[4]{};
 void context(std::uint32_t link,std::uint32_t ep,std::uint32_t st)noexcept{
  if(id!=link||epoch!=ep||stage!=st){cancel();id=link;epoch=ep;stage=st;for(auto& e:episodes)e=Episode{};pass_valid=false;}
 }
 void cancel()noexcept{if(armed||buffer)++c.cancelled;armed=buffer=false;}
 void sample(std::uint64_t tick,bool r,bool trigger,bool safe)noexcept{
  if(last_sample==tick)return;last_sample=tick;input_r=r;
  if(!safe||!r){cancel();return;}
  if(armed&&tick-permit_tick>kPermitTicks){cancel();++c.expired;}
  if(buffer&&tick-buffer_tick>kBufferTicks){buffer=false;++c.expired;}
  if(!trigger)return;++input_serial;input_tick=tick;++c.input_new;
  if(armed){buffer=true;buffer_tick=tick;++c.buffered;}
 }
 bool native_start(std::uint64_t tick,float initial,bool real_clip,bool safe)noexcept{
  if(!safe||!real_clip||!input_r||(input_tick!=tick&&!(armed&&buffer&&tick-buffer_tick<=kBufferTicks))||!input_serial||input_serial==last_used_input){++c.untrusted;return false;}
  last_used_input=input_serial;++native_serial;
  episodes[native_serial%4]=Episode{native_serial,input_serial,tick,initial,false};
  ++c.native_starts;armed=buffer=false;return true;
 }
 void observe(std::uint64_t tick,bool real_clip,float frame,float rate,bool safe)noexcept{
  pass_tick=tick;pass_valid=false;pass_serial=native_serial;
  auto& e=episodes[native_serial%4];
  if(!safe||!real_clip||!e.serial||e.serial!=native_serial)return;
  // Positive animation progression in the ACTUAL native clip is required;
  // merely proc=GUARD_ATTACK or an input edge never supplies motion provenance.
  pass_valid=rate==1.f&&frame>=e.initial_frame+kVisibleAdvance&&frame<=18.f&&tick>e.tick;
  if(pass_valid)++c.motion;
 }
 bool permits(std::uint64_t tick)noexcept{
  auto& e=episodes[pass_serial%4];
  if(!pass_valid||pass_tick!=tick||!e.serial||e.serial!=pass_serial){++c.rejected_motion;return false;}
  if(e.consumed){++c.duplicate;return false;}return true;
 }
 bool confirm(std::uint64_t tick)noexcept{
  if(!permits(tick))return false;episodes[pass_serial%4].consumed=true;
  armed=pass_serial==native_serial;permit_serial=pass_serial;permit_tick=tick;if(!armed)buffer=false;++c.matched;return true;
 }
 bool can_restart(std::uint64_t tick,bool safe,bool recovering)noexcept{
  if(!safe){cancel();return false;}
  if(armed&&tick-permit_tick>kPermitTicks){cancel();++c.expired;}
  if(buffer&&tick-buffer_tick>kBufferTicks){buffer=false;++c.expired;}
  return armed&&permit_serial==native_serial&&buffer&&input_r&&input_serial!=last_used_input&&recovering;
 }
};
}
