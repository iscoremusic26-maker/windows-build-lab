# DEVF CP10.6A — visual feedback for the Blue group bonus and the Black fury (Expansion 0.8.1)

`BokoblinExpansion_0.8.1_TEST.dusk`, mod id `dev.bokoblin.expansion` (replaces 0.8.0; never install both). Provider `DEVF_Provider_0.10.0_TEST.dusk`, editor `DEVF Map Editor CP10.2.exe`, `encounters.json` (Black `bokoblin-004` = 440 HP) and schema are **unchanged**. HP, regeneration, base attack power, the Blue 650 XZ / 250 Y group bonus (1 -> 2), the Black 300-tick fury (3 -> 6), Red = 2 and the approved Black body texture are exactly CP10.2 FIX01 / CP10.3 / CP10.4 / CP10.5 / CP10.6.

| What | When | Look |
|---|---|---|
| **Blue aura** | exactly while `BLUE_GROUP_ON` is in effect (damage 1 -> 2) | soft blue/cyan glow around the body; no change to the eyes |
| **Black aura** | exactly while the fury is active (damage 3 -> 6) | soft purple glow around the body |
| **Black eyes** | same fury state | the eyes fade to purple, and back to their native colour |

- **Smooth:** each effect has a fade state machine (INACTIVE / FADING_IN / ACTIVE / FADING_OUT): **10 ticks in, 15 ticks out** (1/3 s and 1/2 s). Re-activation during a fade-out continues from the current level, nothing restarts. A fury renewal (`BLACK_FURY_REFRESH`) changes nothing visually: it only renews the logical timer.
- **Reuses a game resource:** the aura is the engine's own common-bank glow `ID_ZF_J_FAIRY00_GLOW` (0x072F), driven through the engine's own level-emitter API exactly like Link's bottle fairy. No particle system of our own. See `IMPLEMENTATION.md` for why this one and what was discarded.
- **Eyes** use seven private purple variants of the E_OC2 atlas, bound only to the eye packet of that one Black (the same per-actor mechanism the approved body texture uses). Nothing global is touched.
- **Clean:** nothing is started by the HP initialisation, by regeneration or by a stagger. Death, room change, id reuse and unload leave no residue (the engine removes an emitter nobody refreshes; eyes are restored only on a still-live, still-ours actor).
- Not implemented (specification only): Red fire, ice/electric, vision, attack speed, shield stall, AI, Silver/White/Golden, new editor options.

## What this does and does not prove
Lab + source reading prove the state machine, timing, ownership and cleanup. **How anything looks is not verified** (colours, size, position, intensity are first-round values in one table, `visual_policy.hpp` `kBlue` / `kBlack`, and `eye_recolor.hpp` `kPurple`). If the aura resource is unavailable or rejected at runtime the log says exactly why and only the eyes work. Status: **HUMAN_VISUAL_PENDING**.
