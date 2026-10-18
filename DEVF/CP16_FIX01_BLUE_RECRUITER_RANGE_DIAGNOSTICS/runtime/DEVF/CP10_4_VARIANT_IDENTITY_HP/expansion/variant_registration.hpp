#pragma once
// CP10.4: builds the two exact-match registration tables from the decoded encounters.json (enabled encounters only).
// Black encounters go to `black` (hp from JSON, unchanged CP10.2/FIX01 admission); normal / legacy encounters (no variant) go to
// `normal`, whose only purpose is IDENTITY: such an actor is BLUE even though the broker spawns it with the E_OC2 resource.
// Matching is the same exact float32/record/room/layer/stage rule as the Black table (black_policy.hpp), never proximity.
#include "../../CP10_2_BLACK_BOKOBLIN/expansion/black_policy.hpp"
namespace devf_variant {
// Config: multi_config::Config (or a test double) with requests[i].{placement_id,room,layer,x,y,z,yaw_s16}, variant[i], hp[i], count.
// `black_variant` is multi_config::variant_black. Returns false if any registration is refused (caller removes its own edits).
template<class Config>
inline bool register_encounters(const Config& c,unsigned black_variant,devf_black2::Table& black,devf_black2::Table& normal) noexcept {
 for(unsigned i=0;i<c.count;++i){
  const auto& q=c.requests[i];
  if(c.variant[i]==black_variant){
   if(!black.add(q.placement_id,q.room,q.layer,q.x,q.y,q.z,q.yaw_s16,c.hp[i]))return false;
  }else{
   // hp is irrelevant for identity; 220 only satisfies Table::add's 1..999 guard.
   if(!normal.add(q.placement_id,q.room,q.layer,q.x,q.y,q.z,q.yaw_s16,220))return false;
  }
 }
 return true;
}
// Which registration (if any) does this snapshot match? Black has precedence over Normal. The match deliberately ignores fault
// state: an encounter registered as Black whose admission failed must never fall through to RED/BLUE by name.
struct Match {int black=-1,normal=-1;};
inline Match match(const devf_black2::Table& black,const devf_black2::Table& normal,const devf_black2::Snapshot& s) noexcept {
 Match m;
 for(unsigned i=0;i<black.count;++i)if(black.matches(i,s)){m.black=static_cast<int>(i);return m;}
 for(unsigned i=0;i<normal.count;++i)if(normal.matches(i,s)){m.normal=static_cast<int>(i);return m;}
 return m;
}
} // namespace devf_variant
