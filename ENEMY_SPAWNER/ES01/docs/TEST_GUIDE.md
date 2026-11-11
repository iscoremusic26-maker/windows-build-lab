# Human test (Windows / DX12)

Status: **HUMAN_RUNTIME_PENDING**. CI checks the host API contract with a model; only the game shows the real menu.

## Install (game closed)
Replace **only** the Enemy Spawner `.dusk` 0.1.0 in your mods folder with the 0.1.1 one. Never keep both. Do not touch the other mods.

## 1. The panel (the FIX01 failure)
1. Start the game and open the Mods window, then the Enemy Spawner tab.
2. Expected: no `Failed` / `mod UI panel build failed`. You see the section "Enemy Spawner", the **Enemy** dropdown, the **Spawn Enemy** button and the Black notice, all in English.
3. Open the dropdown: 45 entries, Lizalfos selected by default. Choose another entry, close and reopen the tab: the choice is kept.
4. In the log, there must be no `UI_BUILD_FAILED` and no `SELECT controls need a help pane`.

## 2. Only Enemy Spawner
1. In an open, flat area, choose Lizalfos and press **Spawn Enemy**. Expected: "Spawn request submitted", then "Enemy spawned: Lizalfos". A Lizalfos in front of Link, facing him, that fights.
2. Repeat with several enemies (Keese, Stalfos, a Chu, Bokoblin E_OC native).
3. Kill one, change room and come back, reload, close the game: no crash and no duplicated enemies.
4. Try the refusals: at a cliff edge, in water, against a wall, during a cutscene. Expected: an English notice, nothing spawned.
5. Expansion entries without Expansion: "Bokoblin Expansion is not loaded". Black: "Bokoblin Black integration is unavailable".

## 3. With Bokoblin Expansion 0.17.0 + Enemy Variant Framework 0.15.0
- **Bokoblin Blue (Expansion)**: "classified the spawned enemy as BLUE". In Expansion's log, `VARIANT_ADMITTED` and `HP_INITIALIZED` with 220.
- **Bokoblin Red (Expansion)**: "as RED", 330.
- **Black**: refused, with the notice.

## 4. With Light Fades Twice 0.12.5
Spawn a Lizalfos. The log shows `LIZ_POSTURE event=ADMITTED` for its actor. Blocks, parries, counters and posture must behave like a Lizalfos placed in the map.

## 5. With all your mods
Repeat sections 1 and 2 with everything loaded.

In the log, look for `ENEMY_SPAWNER` and, at shutdown, `ENEMY_SPAWNER_SUMMARY`.

## Rollback
Close the game and remove the 0.1.1 `.dusk`. The 0.1.0 package is not a working rollback: its panel fails to build.
