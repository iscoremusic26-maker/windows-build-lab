// BARS203 CP06 -- Armos (E_AI, profile 431) health as the bars must show it. PURE (no game types).
//
// The Armos is defeated by a HIT COUNTER, not by `health` (Dusklight 2.0.3 e_ai_class::damage_check, verified in the x64 machine code, HM evidence/cp05_ai_exe_damage_check.txt): `health` is reset to the
// decoy 1000 at every check and never read for the defeat; every COUNTED hit (the Armos is not shielding towards the player, hit type != 0x10) adds one to the signed byte e_ai_class::m_hitCount
// (x64 offset 0x1211), the third counted hit is the defeat, and the special routes (large cuts, iron ball, bomb from the front, AtType 0x2000000) assign 3 (then count once more: 4).
// So the honest native progress is the counter: remaining = 3 - m_hitCount (maximum 3). Hits on the shield do not change it, so the bar does not move for them.
// A negative counter exists only INSIDE one damage_check call (Enemy Health Multiplier's transient compensation, restored in the same call): the bars read at the end of the simulation tick, so it is
// never visible; if it ever were, it is not a real life value and the view is "unreadable" (hidden), never a refilled bar.
#pragma once
#include "tracker.h"
namespace ehz { namespace ai {
constexpr int kBudget = 3, kProfile = 431, kMaxCounter = 4;
inline bool dead(int count) { return count >= kBudget; }
// Alive: {3 - count, 3, 1}; defeated or unreadable counter: {0, 0, 1} (the tracker neither follows nor draws it).
inline HealthView native_view(int count) {
    if (count < 0 || count > kMaxCounter || dead(count)) return {0, 0, 1};
    return {kBudget - count, kBudget, 1};
}
}}  // namespace ehz::ai
