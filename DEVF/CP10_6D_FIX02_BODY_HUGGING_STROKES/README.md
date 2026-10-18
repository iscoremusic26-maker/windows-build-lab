# DEVF CP10.6D-FIX02 — BODY_HUGGING_STROKES (Expansion 0.8.8)

`BokoblinExpansion_0.8.8_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** 0.8.7 (never two Expansion files). Rollback: 0.8.7. Provider 0.10.0, editor 0.10.2, `encounters.json`, schema and saves are untouched.

**Problem (HUMAN, 0.8.7):** the strokes were visible but faint, and they floated as a loose ring around the enemy instead of covering it. The target is thin, bright, near-vertical lines on and around the body, like Dark Souls' Grass Crest Shield.

**Changes:** same resource `0x0B9D`, same single shared route for Blue and Black, **one emitter per actor**.
- **Placement:** the ring is centred on the Bokoblin's own torso and head collision spheres, following the hunched pose. Its radius is now 57.6 instead of 91.2, and the strokes are born 41.5–57.6 from that centre (before 58.4–91.2), straddling the body surface.
- **Presence:** alpha cap 255; the emitter's own base colour becomes opaque white; it emits on every tick; strokes are 10 % longer and keep the same width.
- **Unchanged:** the non-degenerate draw basis of 0.8.7.
- **Colours:** Blue (80,190,255) blue/cyan; Black (190,100,255) violet/purple, plus purple eyes.

Log prefix `DEVF_CP10_6D`, `route=SHARED_BODY_HUGGING_STROKES` on every aura line for both variants. `TUNING_APPLIED` reports the centre source, radius, band and presence values read back from the emitter. None of this proves pixels. Status: **GAME_VISUAL_HUMAN: PENDING**.
