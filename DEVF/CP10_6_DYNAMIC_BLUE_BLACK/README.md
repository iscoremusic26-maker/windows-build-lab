# DEVF CP10.6 — Blue group bonus and Black fury (Expansion 0.8.0)

`BokoblinExpansion_0.8.0_TEST.dusk`, mod id `dev.bokoblin.expansion` (replaces 0.7.0; never install both). Provider `DEVF_Provider_0.10.0_TEST.dusk`, editor `DEVF Map Editor CP10.2.exe`, `encounters.json` (Black `bokoblin-004` = 440 HP), schema, placements and the approved Black texture are **unchanged**. Identity, HP and regeneration are exactly CP10.4; the base attack power is CP10.5.

Attack power is always an **absolute** value written to **both** attack spheres, never a multiplier:

| Variant | Base (CP10.5) | Dynamic (this CP) |
|---|---|---|
| BLUE (native `E_OC` or DEVF Normal) | **1** | **2** while at least one OTHER live, classified Bokoblin is within **650 XZ / 250 Y**, same room and stage; back to 1 when none is left |
| RED (native `E_OC2`) | **2** | none. Always 2. A live Red counts as a companion for a nearby Blue |
| BLACK (DEVF, exact registration) | **3** | **6** for **300 simulation ticks (10 s)** after real, non-lethal HP loss observed in its native execute; every new loss renews to 300 (no stacking); then back to 3 |

- One group bonus is the same with one companion or five. A Blue never buffs Red or Black.
- Black fury is not triggered by the HP initialisation (220 -> 440), by regeneration, by a stagger/guard without HP loss, or by a lethal hit. Pausing the game consumes none of the 300 ticks.
- Values written are confined to Blue {1,2}, Red {2}, Black {3,6}: never 4, 9 or 12.
- Not implemented here (specification only): Red fire, ice/electric, vision, chase, attack speed, shield-block changes (the native "shield stall" is untouched), AI, new variants/textures/editor options.

## What this does and does not prove
Lab + source reading prove which values are written, when, and under which ownership rules. **Hearts lost by Link and the real-game feel are not measured** (`HUMAN_RUNTIME_PENDING`). The fury trigger is "the actor's HP dropped inside its native execute": the author of the hit (Link, a bomb, the environment) is **not verified**. See `IMPLEMENTATION.md` and `HUMAN_TEST.md`.
