#pragma once
// Enemy Spawner (Standalone) 0.1.1: the list of enemies this mod can create. PURE: no game header, no address.
//
// Adapted from the Enemy Spawner of Dawnlight by BeZide93 (https://github.com/BeZide93/dawnlight, CC0 1.0 Universal),
// commit b5e9046cd98bdd8593ab2cf219d72a0f61ef7213, src/enemy_spawner.{hpp,cpp}: kEnemySpawnerProfileLabels, kEnemySpawnerProfiles,
// kEnemySpawnerParameters, the Helmasaurus argument, the angle.z switch sentinel of Stalfos/Kargarok and the flying height list.
// Differences from Dawnlight (each one checked in the native source of Dusklight 2.0.3, TwilitRealm/dusklight@40457c6):
//  * Goron (an NPC) and Staltroop (retires itself without Stallord; Dawnlight replaces its idle AI) are not offered.
//  * Lizalfos and Dynalfos use low byte 0xFF instead of 0: with 0 the native create selects ACTION_STAY (d_a_e_dn.cpp L3482,
//    d_a_e_mf.cpp L3243), an idle that only reacts to noise; 0xFF is the ordinary patrol that finds Link and fights.
//  * Bokoblin is split by the native subtype byte of angle.x (d_a_e_oc.cpp create(): 0 -> resource E_OC, 1 -> E_OC2), and into
//    entries that ask Bokoblin Expansion for its classification. The Black entry is never creatable (see docs/BLACK_API_PROPOSAL.md).
#include <cstddef>
#include <cstdint>

namespace es {

// Native profile of each entry. mod.cpp maps it to the fpcNm_* value of the SDK and checks the mapping at compile time.
enum class Profile : std::uint8_t { B_TN, E_OC, E_FZ, E_BA, E_TT, E_GI, B_GG, E_KK, E_FB, E_BS, E_BU, E_MS, E_WW, E_FS, E_CR, E_SH, E_TK, E_TK2, E_RD, E_DN, E_DD, E_MF, E_ST, E_HB, E_GB, E_DB, E_SF, E_MM, E_KR, E_GE, E_AI, E_SM2, Count };

// What a press of "Invocar" means for this entry.
enum class Route : std::uint8_t {
  Native,            // create the native actor, nothing else
  ExpansionBlue,     // native E_OC; Bokoblin Expansion must classify it BLUE (40 -> 220 HP) or the result is reported as native
  ExpansionRed,      // native E_OC2; Expansion must classify it RED (220 -> 330 HP)
  BlackUnavailable,  // Expansion 0.17.0 admits Black only for its exact packaged encounters: no public contract exists. Never creates.
};

// A minimal adaptation, applied only to actors this mod created, from mod_update (before the actor's own Execute).
enum class Adapt : std::uint8_t {
  None,
  ToadpoliWater,  // e_tk: mExecuteState kept above 0, so it stays on the chosen ground instead of searching water (Dawnlight's rule)
  ToadpoliLava,   // e_tk2: mActionTimer[3] kept above 0, so it stays on the chosen ground instead of searching lava (Dawnlight's rule)
};

struct Entry {
  const char* label;
  Profile profile;
  std::uint32_t parameters;
  std::int8_t argument;
  std::int16_t angle_x;  // E_OC: high byte = native subtype (0 -> E_OC, else E_OC2)
  std::int16_t angle_z;  // Stalfos/Kargarok: defeated-switch sentinel 0xFF
  bool flying;           // spawned 200 units above the ground
  Route route;
  Adapt adapt;
};

inline constexpr Entry kCatalog[] = {
    {"Darknut", Profile::B_TN, 0x000001ffu, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Bokoblin (E_OC, native)", Profile::E_OC, 0x00ff0000u, -1, 0x0000, 0, false, Route::Native, Adapt::None},
    {"Bokoblin (E_OC2, native)", Profile::E_OC, 0x00ff0000u, -1, 0x0100, 0, false, Route::Native, Adapt::None},
    {"Bokoblin Blue (Expansion)", Profile::E_OC, 0x00ff0000u, -1, 0x0000, 0, false, Route::ExpansionBlue, Adapt::None},
    {"Bokoblin Red (Expansion)", Profile::E_OC, 0x00ff0000u, -1, 0x0100, 0, false, Route::ExpansionRed, Adapt::None},
    {"Bokoblin Black (Expansion): integration unavailable", Profile::E_OC, 0x00ff0000u, -1, 0x0100, 0, false, Route::BlackUnavailable, Adapt::None},
    {"Lizalfos", Profile::E_DN, 0xff0000ffu, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Dynalfos", Profile::E_MF, 0xff0000ffu, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Bulblin", Profile::E_RD, 0xff000100u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Stalfos", Profile::E_SF, 0x0000ff00u, -1, 0, 0xff, false, Route::Native, Adapt::None},
    {"Stalchild", Profile::E_BS, 0x00ffff00u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Stalhound (night only)", Profile::E_SH, 0x00ffff00u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Gibdo", Profile::E_GI, 0x00ff01ffu, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Puppet", Profile::E_FS, 0x00000000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"White Wolfos", Profile::E_WW, 0x03000000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Chilfos", Profile::E_KK, 0x00ff0000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Freezard", Profile::E_FB, 0x00ffff00u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Mini Freezard", Profile::E_FZ, 0x00000000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Tektite", Profile::E_TT, 0x000000ffu, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Keese", Profile::E_BA, 0xffff0f01u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Fire Keese", Profile::E_BA, 0xffff1f01u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Ice Keese", Profile::E_BA, 0xffff2f01u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Bubble", Profile::E_BU, 0xffff0f00u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Fire Bubble", Profile::E_BU, 0xffff1f00u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Ice Bubble", Profile::E_BU, 0xffff2f00u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Aeralfos", Profile::B_GG, 0xffff0002u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Kargarok", Profile::E_KR, 0xffffff00u, -1, 0, 0xff, true, Route::Native, Adapt::None},
    {"Guay", Profile::E_GE, 0x00ffff01u, -1, 0, 0, true, Route::Native, Adapt::None},
    {"Rat", Profile::E_MS, 0xffff0000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Bomskit", Profile::E_CR, 0x00000000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Water Toadpoli", Profile::E_TK, 0x00ff0000u, -1, 0, 0, false, Route::Native, Adapt::ToadpoliWater},
    {"Fire Toadpoli", Profile::E_TK2, 0x00000000u, -1, 0, 0, false, Route::Native, Adapt::ToadpoliLava},
    {"Dodongo", Profile::E_DD, 0xffff0000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Skulltula", Profile::E_ST, 0xff000002u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Baba Serpent", Profile::E_HB, 0xff000000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Big Baba", Profile::E_GB, 0x00ffff00u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Deku Baba", Profile::E_DB, 0xff000000u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Helmasaur", Profile::E_MM, 0x0000ff00u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Helmasaurus", Profile::E_MM, 0x0000ff00u, 1, 0, 0, false, Route::Native, Adapt::None},
    {"Armos", Profile::E_AI, 0x00ff000au, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Red Chu", Profile::E_SM2, 0xffff0010u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Blue Chu", Profile::E_SM2, 0xffff0020u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Yellow Chu", Profile::E_SM2, 0xffff0030u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Purple Chu", Profile::E_SM2, 0xffff0040u, -1, 0, 0, false, Route::Native, Adapt::None},
    {"Black Chu", Profile::E_SM2, 0xffff0060u, -1, 0, 0, false, Route::Native, Adapt::None},
};
inline constexpr std::size_t kCatalogSize = sizeof(kCatalog) / sizeof(kCatalog[0]);

// Labels in catalog order, for the host's select control.
struct Labels {
  const char* v[kCatalogSize];
};
inline constexpr Labels make_labels() noexcept {
  Labels l{};
  for (std::size_t i = 0; i < kCatalogSize; ++i) l.v[i] = kCatalog[i].label;
  return l;
}
inline constexpr Labels kLabels = make_labels();

}  // namespace es
