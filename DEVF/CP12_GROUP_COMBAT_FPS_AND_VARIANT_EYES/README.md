# DEVF CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES — Expansion 0.9.0

`BokoblinExpansion_0.9.0_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** 0.8.9 (never two Expansion files). Rollback: 0.8.9. Provider 0.10.0, editor 0.10.2, `encounters.json`, schema and saves are untouched.

- **Black:** aura and eyes are now **intense red**, (255,32,24) / env (130,0,6). Same geometry, alpha, fades and fury logic; the black skin is untouched.
- **Blue:** the aura is unchanged. Its emitter trace is byte-identical to 0.8.9. **New:** its eyes turn the aura's **blue/cyan** (80,190,255) while the group bonus is on, fading 10 / 15 ticks with the aura, and they always return to the native texture.
- **Red:** unchanged; no aura and native eyes.
- **Fix:** a Blue whose group started while it was not being drawn (off camera) used to lose its aura for good after 60 ticks; this happened in the human 0.8.9 log. It now waits for the first draw.
- **Measurement:** one line `DEVF_CP12 event=PERF_WINDOW` every ~10 s with simulation-tick interval percentiles, hitch counts and the mod's own CPU time. It is written at ERROR level only so it survives `--log-level error`, and it says it is not an error.

The 0.8.9 log shows **no evidence that the particles cause the dips**: they are drawn in one batched draw per aura. The engine warning `CalcDivideInfo`, flushed to disk on every frame in the combat windows (up to 300 per 300 ticks) and present even without auras, is a strong external candidate. See `IMPLEMENTATION.md`, section A. Status: **HUMAN_VISUAL and HUMAN_PERFORMANCE: PENDING**.
