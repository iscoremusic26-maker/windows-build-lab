# DEVF CP11_PERFORMANCE_OPTIMIZATION — Expansion 0.8.9

`BokoblinExpansion_0.8.9_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** 0.8.8 (never two Expansion files). Rollback: 0.8.8. Provider 0.10.0, editor 0.10.2, `encounters.json`, schema and saves are untouched.

**Same behaviour and look as 0.8.8:** gameplay, classification, HP, regeneration, damage, group bonus, fury, eyes and texture are unchanged. The shared body-hugging strokes of Blue (cyan) and Black (violet) are unchanged too: same resource, geometry, colours, alpha and one emitter per actor.

**What is faster (mod CPU only, exact by construction and tested):**
- The per-tick registration check no longer recomputes the stage layer (about 40 string compares) for Bokoblins that cannot match a registration.
- Emitter read-backs run once at creation instead of every tick.
- Ledger lookups stop early.
- After the first 2 detailed acquisitions per variant, activation logs are compact; anomalies are always logged in full.

Local microbench of the mod's own code: activation tick −86 %, active tick −86 %, inactive tick −45 %, log bytes per ON/OFF cycle −71 %. **This is not an FPS figure.** The particles' GPU cost is unchanged, because reducing it would change the approved look. Status: **HUMAN_FPS_AND_VISUAL_PARITY: PENDING**. Details: `IMPLEMENTATION.md`.
