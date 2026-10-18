# DEVF CP10.6D-FIX01 — non-degenerate draw basis (Expansion 0.8.7)

`BokoblinExpansion_0.8.7_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** 0.8.6 (never two Expansion files). Rollback: 0.8.6. Provider 0.10.0, editor 0.10.2, `encounters.json`, schema and saves are untouched.

**Problem (HUMAN, 0.8.6):** Blue and Black strokes completely invisible, although resource, triggers and emitters were all present in the log.

**Cause (engine source):** 0.8.6 set the stroke direction to (0,1,0). With our emitters' identity rotation the particle base axis is also (0,1,0). The direction-cross draw (`JPADrawDirection`) takes `cross(base, direction)`, which is 0, and returns **without drawing**, for every particle and frame.

**Fix:** the single shared stroke direction becomes (0.10, 0.99499, 0), 5.7° from vertical. `|cross| = 0.10` in every frame, far above the engine threshold. Everything else in 0.8.6 is kept: full ring around the torso (radius 91.2, band 58.4–91.2, chest height), canonical frame, thin quads 14×165, one emitter per actor, resource `0x0B9D` for **both** variants. Only colour differs: Blue cyan, Black violet, and the Black keeps its purple eyes.

Log prefix `DEVF_CP10_6D`. New: `DRAW_BASIS_CHECK ... status=NON_DEGENERATE evidence=MATHEMATICAL_READBACK_NOT_RENDERED` per created emitter. It is arithmetic on read-back values, not a pixel claim. Status: **GAME_VISUAL_HUMAN: PENDING**.
