// BARS203 TT-FIX01 -- Tektite (E_TT, profile 518) health as the bars must show it. PURE (no game types).
//
// The Tektite is defeated by a HIT COUNTER, not by `health` (Dusklight 2.0.3 daE_TT_c::damage_check, verified in the x64 machine code): every hit adds 10 (sword, shield attack, other) or 40 (arrow,
// bomb, iron ball) to the counter (int, daE_TT_c::field_0x6f0), resets `health` to 100 and sets it to 0 once the counter reaches 40; arrows/bombs/iron balls and sword finishers also start the death
// ACTION immediately whatever the counter is. So `health` is meaningless for a bar (80 at creation, 100 - power after a hit, reset on the next one, > 0 while the death action plays).
// The honest native progress is the counter: remaining = 40 - counter (maximum 40); the defeat shown is the game's own: counter >= 40, health <= 0, or the death action (4).
#pragma once
#include "tracker.h"
namespace ehz { namespace tt {
constexpr int kBudget = 40, kActionDeath = 4, kProfile = 518;
inline bool dead(int counter, int health, int action) { return counter >= kBudget || health <= 0 || action == kActionDeath; }
// Alive: {40 - counter, 40, 1}; defeated or unreadable counter: {0, 0, 1} (the tracker neither follows nor draws a defeated actor).
inline HealthView native_view(int counter, int health, int action) {
    if (counter < 0 || counter > kBudget + 40 || dead(counter, health, action)) return {0, 0, 1};
    return {kBudget - counter, kBudget, 1};
}
}}  // namespace ehz::tt
