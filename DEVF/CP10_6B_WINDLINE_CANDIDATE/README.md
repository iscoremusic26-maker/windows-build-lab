# DEVF CP10.6B — WindLine aura candidate (Expansion 0.8.2)

`BokoblinExpansion_0.8.2_TEST.dusk`, mod id `dev.bokoblin.expansion` (replaces 0.8.1, which stays the rollback; never install both). Provider 0.10.0, editor 0.10.2, `encounters.json` and schema are **unchanged**. Identity, HP, regeneration, damage (Blue 1 -> 2 group bonus, Black 3 -> 6 fury, Red 2), the approved Black body texture, the purple Black eyes (seven private variants, ownership-checked) and the fade controller of CP10.6A are exactly the same code.

**What changes:** the aura that follows the Blue group bonus and the Black fury is now the native common-bank particle `ID_AK_JN_WINDLINE00` (`AK_JN_windLine00.jpa`, **0x0046**) instead of the CP10.6A glow `0x072F`, which is retired from the aura path. One emitter per actor, moved to the actor every tick, tinted per emitter (Blue cyan/blue, Black violet), faded with the existing 10-tick-in / 15-tick-out controller.

**Also fixed:** the real CP10.6A log showed `BLUE_VISUAL_REJECTED PARTICLE_SET_RETURNED_NO_EMITTER`. The engine's `dPa_control_c::set()` returns no emitter when the actor's `tevStr` is not initialised yet (`Material_use_fg != 0x7C`), which happens at the first execute of a freshly created actor, and when the emitter pool is full. 0.8.1 treated the first refusal as permanent for that actor. Now each refusal is a **bounded retry** (every tick, up to 60 ticks = 2 s of game time), then a clean rejection for that generation; a failure of the aura never touches the eyes.

## What this is and is not
- A **candidate for a visual test**. WindLine is not known to look like thin vertical strokes (the source never spawns it; its shape, direction and emission are unknown until the first `RESOURCE_PROBE` log line is read). Colour is applied per emitter (the draw code multiplies the particle colour by the emitter's global colour and alpha), but whether the pixels actually take the tint is **not verified**. Nothing here claims a Dark Souls look.
- Not implemented: more than one emitter per actor (a 3-emitter layout is a possible next step, only if the probe and the first test justify it), Red fire, ice/electric, shield stall, AI, editor, schema.

Status: LAB_PASS / CI per the pull request / **GAME_VISUAL_HUMAN: PENDING**.
