#pragma once
#include <cstdint>
namespace post06 {
enum class LinkContact:std::uint8_t{None,Sword,Shield};
inline LinkContact classify_link_contact(std::uintptr_t selected,std::uintptr_t cyl,std::uintptr_t cps0,std::uintptr_t cps1,std::uintptr_t cps2,std::uintptr_t guard) noexcept {
 if(!selected)return LinkContact::None;
 if(selected==guard)return LinkContact::Shield;
 if(selected==cyl||selected==cps0||selected==cps1||selected==cps2)return LinkContact::Sword;
 return LinkContact::None;
}
struct HelmEpisode {
 std::uint32_t id=0,link_id=0;std::uint64_t generation=0;bool seen=false;
 void observe(std::uint32_t actor,std::uint64_t gen,std::uint32_t player,bool helm_proc,bool known=true) noexcept {
  if(id!=actor||generation!=gen){id=actor;generation=gen;link_id=0;seen=false;}
  if(!known)return;
  if(link_id!=player||!helm_proc)seen=false;
  link_id=player;
 }
};
// Matches CP21: floor 100 Master / 50 Ordon, never heal a larger native/HM loss.
inline bool helm_contact_verified(bool live,bool token,bool processed,bool fresh,bool sword) noexcept {return live&&token&&processed&&fresh&&sword;}
struct HelmDamage {bool write=false,lethal=false;int hp=0,base=0;};
inline HelmDamage normalize_helm(int pre,int native,int floor,bool confirmed) noexcept {
 HelmDamage r;r.hp=native;if(!confirmed||pre<=0||native<=0||native>=pre||(floor!=50&&floor!=100))return r;
 const int loss=pre-native;r.base=loss>floor?loss:floor;r.hp=pre>r.base?pre-r.base:0;r.write=r.hp<native;r.lethal=r.hp==0;return r;
}
}
