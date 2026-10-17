# CP10.2 FIX01 — visual QA (Windows / Dusklight 2.0.3 / DX12)
Visual only first. HP/combat is a separate later check.

## Install
1. Close Dusklight. Back up `BokoblinExpansion_0.4.0_TEST.dusk` and your `encounters.json`.
2. Replace ONLY the Expansion with `BokoblinExpansion_0.4.1_TEST.dusk`. Keep `DEVF_Provider_0.10.0_TEST.dusk`, keep the CP10.2 editor, keep your JSON (bokoblin-004 Black, HP 150).

## Black next to Normal in the same scene
`bokoblin-004` is alone in F_SP121 room 1 / layer 6. To see a Normal one beside it:
1. Editor CP10.2 → CP10 multi-room → open your schema2 → load F_SP121 R01 from the catalog.
2. Variant = **Normal**, layer 6, add one marker on accepted ground at least ~400 units from bokoblin-004 (the editor rejects closer than 300). Enable it, save a copy, Install JSON (game closed).
3. Afterwards remove that temporary marker (or reinstall your backup JSON).

## Check
- Black: charcoal/graphite skin with visible face, muscles and texture; NOT a uniform black film; not pure black. Cloth, chain mail, straps keep their own colours; eyes look native.
- Normal: completely unchanged, even when both are on screen together.
- Day and night, close and far. Leave the room and come back (re-entry). No flicker, no corrupted or swapped textures.
- Send a screenshot with both, and only these log lines: `DEVF_CP10_2_BLACK_TEXTURE event=...` and `DEVF_CP10_2_BLACK event=HP_APPLIED`.
- `BLACK_TEXTURE_REJECTED` means the Black one kept the native look; the reason is in the line.

## Rollback
Close the game; restore `BokoblinExpansion_0.4.0_TEST.dusk` (or 0.3.0) and your backed-up `encounters.json`.
