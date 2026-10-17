# CP10.2 — implementation notes

## Architecture
Editor → `encounters.json` (schema2, optional `bokoblin_variant`/`hp`) → Expansion 0.4.0 → (unchanged) EncounterBroker 0.10.0 → StageService `add_actor` → native E_OC2. The provider ABI (`DevfEncounterPlacementRequest` 80 bytes, receipt 24, service ids @1.0) is not touched: the variant never crosses the API. The Expansion already knows the exact record it requested (room, layer, XYZ, yaw, params `0xFFFF0100`, range code `0x0132`), so it recognizes its own actor in the game by exact record equality and applies HP/tint itself.

## Why in the Expansion
No provider rebuild, no ABI or mod-id change, one `.dusk` to replace, and a Normal-only configuration installs no hook at all. If hook installation or registration fails, the Expansion registers **zero** placements (same fail-closed rule as before) instead of spawning a Normal Bokoblin where a Black one was configured.

## Reused, human-observed mechanism (CP03 → CP04.4, 2026-10-14)
- HP: one write of `daE_OC_c::health` during the **first native execute** of the matching actor. Rules per registration (`expansion/black_policy.hpp`): exact record match (profile 510, room, params, setID `0xFFFF`, home XYZ equal as float32, home angles `0x0132/yaw/0`), stage `F_SP121`, layer equal, initializing state, id strictly greater than the last one, no live previous instance, resource `E_OC2`, native HP 220. A violation faults **that registration only** (it stays Normal; a `BLACK_REJECTED` line is logged).
- Tint: private 9-byte XF `MATCOLOR0` command pair around the body shapes (0 and 2) of the admitted actor's own model during its own `drawFast` call, restored immediately after; eyes (shape 1), the accessory model and the original Bokoblins are excluded. Effective-resource contract checks (materials, TEV, textures, display list) are the CP04.4 ones, unchanged.
- Differences from CP04.4: (1) up to 8 simultaneous registrations instead of one fixed record, (2) HP value configurable per encounter instead of the constant 150, (3) tint strength raised from ~3% to ~35% for legibility (`expansion/xf_scope.hpp`; the only changed constant set), (4) no HP observer/telemetry, (5) records come from the JSON, not a hard-coded site.

## Invariants kept
Mod ids, ABI, services, E_OC2 profile 510, capacity 8, separation 300 per room/layer, cold start, no hot reload, schema1 untouched, legacy `CP08_OBSERVED` validation (no `FLOOR_AMBIGUOUS` bypass for new points; a legacy marker may be set Black without changing its provenance).

## Limits / not verified
- Appearance, HP effect and combat are **not** verified in game. HP values other than 150 have never been run (150 is the value observed in CP03/CP04.4).
- Tint strength is a judgement call (35%); it is a single constant if it needs tuning.
- Black is only for the existing F_SP121 rooms 1–5 scope. No multi-stage, no other colors, no damage/speed fields, no fire/ice/electric effects.
- A 0.3.0 Expansion rejects a file containing the new keys (strict parser, zero requests): do not mix versions.
