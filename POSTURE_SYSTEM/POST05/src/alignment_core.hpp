#pragma once
#include <cstdint>
#include "cp06/tracker.h"
namespace postalign02 {
// A completely separate tracker, fed the same full queue snapshot as CP06.
// Default enabled is immutable in the supplied CP06 mod (no options setter is
// implemented or called). Identity/lifecycle is independent of posture credit.
class Mirror {
 public:
  ehz::Tracker tracker;
  bool scene=false;
  std::uintptr_t player=0;std::uint32_t player_id=0;char stage[8]{};std::int8_t layer=0,dark=0;
  std::uint64_t clears=0;
  void clear(const ehz::TickEnv& env) noexcept {tracker.clear(&env);scene=false;player=0;player_id=0;++clears;}
  void sample(std::uint32_t tick,std::uintptr_t p,std::uint32_t id,const char* name,std::int8_t l,std::int8_t d,bool allowed,bool senses,
              ehz::ActorView* views,unsigned count,const ehz::TickEnv& env) noexcept {
    if(!p){clear(env);return;}
    bool changed=!scene||p!=player||id!=player_id||l!=layer||d!=dark;
    for(unsigned i=0;i<8;++i)changed=changed||name[i]!=stage[i];
    if(changed){clear(env);scene=true;player=p;player_id=id;layer=l;dark=d;for(unsigned i=0;i<8;++i)stage[i]=name[i];}
    tracker.tick(tick,allowed,true,senses,views,count,env);
    // Handles are native borrowed pointers, never usable after this simulation pass.
    for(unsigned i=0;i<count;++i)views[i].handle=nullptr;
  }
};
}
