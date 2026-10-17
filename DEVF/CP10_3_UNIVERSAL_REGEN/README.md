# DEVF CP10.3 — passive regeneration for every Bokoblin (Expansion 0.5.0)

`BokoblinExpansion_0.5.0_TEST.dusk`, mod id `dev.bokoblin.expansion` (replaces 0.4.1; never install two versions). Provider `DEVF_Provider_0.10.0_TEST.dusk`, editor `DEVF Map Editor CP10.2.exe`, schema, placements, the four observed encounters and the approved Black texture are **unchanged**.

- **Every** `daE_OC_c` regenerates: native `E_OC` (40 HP), native `E_OC2` (220 HP), DEVF Normal (it is a native E_OC2), and DEVF Black (its configured HP, e.g. 150).
- Rate: **2 HP/s** (Normal/native), **4 HP/s** (Black). Fixed HP per second of game time, not a percentage.
- Starts as soon as the Bokoblin is below its maximum, also in combat; no delay or cooldown after damage.
- Never above the maximum, never lowers a higher value, never revives (health ≤ 0) and never heals in death / water-death / fall / cutscene states.
- Works with or without Black encounters in `encounters.json`.
- Future Red 3, Silver 6, White 4, Golden 8 (16 with an ally) HP/s are **specification only**, not implemented.

Source evidence, time base and design: `IMPLEMENTATION.md`. QA: `HUMAN_TEST.md`. Status: CI per the pull request; **HUMAN_RUNTIME_PENDING**.
