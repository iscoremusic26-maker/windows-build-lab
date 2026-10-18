# DEVF CP10.6C — light-sword strokes, visual PILOT (Expansion 0.8.4)

`BokoblinExpansion_0.8.4_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** whichever Expansion you have installed for one test round (0.8.3 DIAG or 0.8.2; never install two). Rollbacks: 0.8.2 (visual base), 0.8.3 DIAG (inventory tool), 0.8.1. Provider 0.10.0, editor 0.10.2, `encounters.json` and schema are untouched.

**What it is:** a single, minimal visual experiment. The aura of 0.8.2 (the missing WindLine `0x0046`) is replaced by **one native emitter per actor from Link's light-sword glow family**, both present in the common bank of your real session inventory (`bank=0`, ids without 0x8000):

| Actor / state | Resource | Tint |
|---|---|---|
| Blue with the group bonus active | `0x0A58` `ID_ZI_J_LK_SWL_A` | cyan / blue |
| Black during the fury | `0x0B9D` `ID_ZI_J_LK_SWL_C` | violet (the existing purple eyes stay) |
| Blue without group, Black without fury, Red | none | — |

`SWL_B` (`0x0A59`) is not used. Logic, HP, regeneration, damage, the Blue group bonus, the Black fury, the Black texture and eyes, the CP10.6A fade (10 in / 15 out) and the CP10.6B bounded retry (tevStr not ready / pool full) are the 0.8.2 code.

**Why this is a hypothesis, not a result:** the family is originally a *sword* effect (Link attaches three emitters to the sword matrix). The resource itself does not need the sword; this pilot gives it the actor's position and the engine's default identity rotation, with **no sword matrix, sword model or bone**. From the resource data (circle volume in the emitter's horizontal plane, directional quads 20x280 units moving along +Y) it *may* read as thin vertical strokes rising from a ring around the Bokoblin, or it may look like sword streaks or be too big/small. **Nothing here claims how it looks.** Using one emitter instead of the original three is also a deliberate limit of this first test.

Look for in the log: `DEVF_CP10_6C_STROKES` (`RESOURCE_PROBE`, `RESOURCE_FOUND`/`MISSING`, `ACTOR_STATE_ON`, `REQUESTED`, `EMITTER_ACQUIRED`, `EMITTER_REJECTED`, `COLOR_APPLIED`/`UNAVAILABLE`, `EMITTER_RELEASED`). `ACTOR_STATE_ON` is the logic state and `EMITTER_ACQUIRED` the engine handing over an emitter: neither proves pixels. Status: **GAME_VISUAL_HUMAN: PENDING**.
