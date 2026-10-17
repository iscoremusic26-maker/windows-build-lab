# CP10.3 — human QA (Windows / Dusklight 2.0.3 / DX12), one round
1. Close Dusklight. Back up `BokoblinExpansion_0.4.1_TEST.dusk` and `encounters.json`. Put ONLY `BokoblinExpansion_0.5.0_TEST.dusk` in its place; keep the provider 0.10.0, the editor and your JSON (bokoblin-004 Black, HP 150). Never keep both Expansion files.
2. **Native Bokoblin** (not placed by DEVF): damage it without killing it (a native E_OC has 40 HP), then stop attacking and stay out of its reach. It should recover about 2 HP per second of game time (full 40 HP in ~20 s). There may be no visible bar: the proof is the log.
3. **Black** `bokoblin-004` (F_SP121 / room 1 / layer 6): wound it without killing it. It should recover about 4 HP/s and stop exactly at 150. Colour and animation must stay as approved.
4. Kill one Bokoblin of each kind: it must stay dead (no revival, no healing during the death animation). Leave and re-enter the room: new Bokoblins start with their own state.
5. Optional: pause/open the menu for a while mid-regeneration: the HP must not jump when you resume.
6. Send only these lines: `DEVF_CP10_3_REGEN event=REGEN_TRACKED / REGEN_APPLIED / REGEN_RELEASED` (and `REGEN_REJECTED` if present), `DEVF_CP10_2_BLACK event=HP_APPLIED`, `DEVF_CP10_2_BLACK_TEXTURE event=BLACK_TEXTURE_BOUND`.

**What the log proves:** each `REGEN_APPLIED` carries `sim_ticks` (1 tick = 1/30 s of game time), `healed_total`, hp before/after and the maximum. For an undisturbed wounded Normal Bokoblin, consecutive lines (every 10 HP) are ~150 ticks / 5.000 s apart; for Black ~75 ticks / 2.500 s. A hit, a scripted state or time spent at full health makes the interval longer (ticks keep counting). **What you observe:** that it heals while idle/in combat, stops at its maximum, never revives and keeps its look. Without a numeric health display the 2/4 HP/s are not measurable by eye.

Rollback: close the game, restore `BokoblinExpansion_0.4.1_TEST.dusk` and your backed-up `encounters.json`.
