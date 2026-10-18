# DEVF CP10.6C-FIX01 — strokes body coverage (Expansion 0.8.5)

`BokoblinExpansion_0.8.5_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** the 0.8.4 pilot for one test round (never two Expansion files). Rollbacks: 0.8.4 (observable pilot), 0.8.3 DIAG, 0.8.2, 0.8.1. Provider 0.10.0, editor 0.10.2, `encounters.json`, schema and saves are untouched.

**Human evidence that motivates it (USER_TESTIMONY, not CI):** with 0.8.4 the strokes were seen "only in a very small part of the enemy, right at the end of its back". Everything else (resources `0x0A58` Blue / `0x0B9D` Black, one emitter per actor, fades, retries, cleanup, gameplay, eyes) is unchanged.

**What changes (one emitter per actor, max 8, same two resources):** only *where* and *how big* the same particles are — a larger ring whose spawn band starts outside the Bokoblin's own torso collider, a slightly higher centre, wider and longer (still thin) quads, compensation of the resource's own local offset, a per-emitter vertical direction only if the resource is unrotated and not already vertical, and extra read-only telemetry (`RESOURCE_GEOMETRY`, `CURVE_PROBE`, `TUNING_APPLIED`). Emission rate, lifetime, alpha and colours are not touched. See `IMPLEMENTATION.md` for the exact diff and the proven-versus-hypothesis table.

Log prefix `DEVF_CP10_6C_FIX01`. `ACTOR_STATE_ON` = logic state, `EMITTER_ACQUIRED` = the engine handed over an emitter, `TUNING_APPLIED` = parameters read back from that emitter. **None of them proves pixels; there is no `VISUAL_ACCEPTED` event.** Status: **GAME_VISUAL_HUMAN: PENDING**.
