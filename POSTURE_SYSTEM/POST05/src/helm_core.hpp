#pragma once
#include <cstdint>
namespace post06 {
// Matches CP21: floor 100 Master / 50 Ordon, never heal a larger native/HM loss.
struct HelmDamage {bool write=false,lethal=false;int hp=0,base=0;};
inline HelmDamage normalize_helm(int pre,int native,int floor,bool confirmed) noexcept {
 HelmDamage r;r.hp=native;if(!confirmed||pre<=0||native<=0||native>=pre||(floor!=50&&floor!=100))return r;
 const int loss=pre-native;r.base=loss>floor?loss:floor;r.hp=pre>r.base?pre-r.base:0;r.write=r.hp<native;r.lethal=r.hp==0;return r;
}
}
