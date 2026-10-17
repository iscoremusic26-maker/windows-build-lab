# DEVF CP10.2 FIX01 — Black Bokoblin with its own albedo (no "paint" filter)

Expansion **0.4.1** (`BokoblinExpansion_0.4.1_TEST.dusk`, id `dev.bokoblin.expansion`). Replaces 0.4.0; never install both. Provider `DEVF_Provider_0.10.0_TEST.dusk` and editor `DEVF Map Editor CP10.2.exe` (0.10.2) are **reused unchanged** (editor source is byte-identical, checked in CI). JSON, schema, HP and placement rules are exactly CP10.2.

What changed: CP10.2 darkened the whole body with a MATCOLOR0 multiplier (90/255) during each body draw, which multiplies every texel, color channel and material equally — a uniform black film. FIX01 removes that route completely (no `drawFast` hook, no XF command) and gives the Black actor **its own texture**: a private copy of the E_OC2 atlas, recolored by region, bound only to that actor's two body material packets.

- Skin (mauve/purple hues) → cool charcoal slate at ~80% of its own luminance, keeping 12% of the original chroma.
- Chain mail, cloth, leather straps keep their hue (8% darker). Eyes use the untouched original texture (their packet is never re-bound).
- Recolor runs in the compressed domain: only the two RGB565 endpoints of each 4×4 block change; the per-texel indices are kept, so every edge, fold, scale and muscle line stays exactly where it was.
- Lighting, material colors, TEV, alpha and draw order are native, so the dark albedo is lit like any other surface.

Generated at runtime from your own game data in memory; no game texture or derivative is stored in Git or the artifact.

Status: CI build/tests/independent download — see the PR. **HUMAN_VISUAL_PENDING.** Details: `IMPLEMENTATION.md`; QA: `HUMAN_TEST.md`.
