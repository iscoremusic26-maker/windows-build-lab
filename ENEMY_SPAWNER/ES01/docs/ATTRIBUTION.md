# Attribution and provenance

**Origin:** the Enemy Spawner of Dawnlight by BeZide93.
- Repository: https://github.com/BeZide93/dawnlight
- Commit read: `b5e9046cd98bdd8593ab2cf219d72a0f61ef7213` (2026-11-01)
- License: CC0 1.0 Universal (`LICENSE.md` of the same commit)

**Reused, adapted:**
- `src/enemy_spawner.hpp`/`.cpp`: the enemy list (`kEnemySpawnerProfileLabels`, `kEnemySpawnerProfiles`, `kEnemySpawnerParameters`), the Helmasaurus argument, the 0xFF angle.z sentinel of Stalfos/Kargarok, the flying height, the 300-unit distance and the ground probe from 500 units above.
- `create_test_actor_in_player_layer`: Link's layer is current during the creation call, and the previous layer is always restored. This is the scene-ownership fix BeZide93 confirmed in game.
- The water and lava Toadpoli adaptations.

**Not reused:** the hooks on `fpcMtd_Method` and `fopAcM_createChild`, child tracking, the Boss Rush arena gate, Dodongo's imported fire effect, the Staltroop with replaced AI, the Goron, Dawnlight's UI and everything else. No art or resource is copied.

**Differences in this adaptation:**
- Requests are processed in `mod_update`, at the start of a frame, never inside the UI callback.
- Ground checks: step height, slope, water or lava, and a wall between Link and the point.
- At most 12 live enemies created by the mod.
- Lizalfos and Dynalfos use low byte 0xFF instead of 0, which is the idle `ACTION_STAY` state in the native code.
- Bokoblin entries are split by native resource and by Expansion's classification.
- The chooser is a dropdown (FIX01): a Mods panel cannot host a SELECT.

New changes are not BeZide93's work and imply no endorsement.

**Validated against:** Dusklight 2.0.3 (`TwilitRealm/dusklight@40457c6adb381928e4b5fef6ed459ed291edd5e2`), and Bokoblin Expansion 0.17.0 / Enemy Variant Framework 0.15.0 (source `fe47c8b828939829914d466a425cb1e2c0223cd9`). This is code inspection and laboratory work, not execution in the game.
