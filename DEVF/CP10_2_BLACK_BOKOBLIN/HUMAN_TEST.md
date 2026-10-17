# CP10.2 — human QA (Windows / Dusklight 2.0.3 / DX12)
Keep your CP10 provider `.dusk`. Never run Expansion 0.3.0 and 0.4.0 together. Gameplay of Normal Bokoblins must not change.

## Install
1. Close Dusklight. Back up `BokoblinExpansion_0.3.0_TEST.dusk` and your `encounters.json`.
2. Replace the Expansion with `BokoblinExpansion_0.4.0_TEST.dusk` (from the TEST zip). Keep `DEVF_Provider_0.10.0_TEST.dusk`.
3. Run `DEVF Map Editor CP10.2.exe` (does not install anything).

## Test 1 — the four existing Bokoblins still work
1. Install your current schema2 file unchanged (all Normal) and start the game.
2. F_SP121: the three original Bokoblins in room 3 / layer 6 and `bokoblin-004` in room 1 / layer 6 appear and behave as before. Log: 4 `REQUEST_ACCEPTED`, **no** `DEVF_CP10_2_BLACK` lines.

## Test 2 — one Black Bokoblin placed with the editor
1. Editor → CP10 multi-room: open your schema2 file. Select `bokoblin-004`, set **Variant = Black Bokoblin**, HP **150** (the value already observed in earlier tests). The list should read `bokoblin-004 [Black · HP 150]`. (Or add a new marker in a F_SP121 room with the variant set to Black before clicking.)
2. Save a test copy, then Install JSON (game closed). Start the game.
3. Go to the Black one. Check: **dark charcoal gray, not pure black**; face, clothing, relief and silhouette still readable; eyes keep their native look; the three Normal Bokoblins keep native colors. Compare with a Normal one.
4. Fight it: normal behavior; it dies with fewer hits than a Normal one (HP 150 vs 220).
5. Optional: leave the room and come back; also try night/day. Optional: HP 300 (never run before).

## Report
What it looks like (screenshot welcome), whether details are legible, whether it is too dark/too light, combat behavior, and the log lines `DEVF_CP10_2_BLACK event=HP_APPLIED / BLACK_APPLIED` (extract only those lines). `BLACK_REJECTED` means the actor stayed Normal (the reason is in the line).

## Rollback
Close the game, restore the 0.3.0 Expansion and your backed-up `encounters.json`.
