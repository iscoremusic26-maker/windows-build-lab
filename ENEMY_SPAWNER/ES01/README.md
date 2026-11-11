# Enemy Spawner (Standalone) 0.1.1 FIX01

Private mod for Dusklight 2.0.3 (Windows x64, D3D12). It spawns real native enemies in front of Link from the Mods window, to test combat without travelling across the map.

In its tab of the Mods window: **Enemy** (a dropdown) and **Spawn Enemy** (a button). Nothing else.

- ID: `dev.enemyspawner.standalone`. No hooks. It does not call or write into any other mod.
- Based on the Enemy Spawner of **Dawnlight** by BeZide93: https://github.com/BeZide93/dawnlight (CC0 1.0 Universal), commit `b5e9046cd98bdd8593ab2cf219d72a0f61ef7213`. It is not Dawnlight, is not affiliated with it, and contains none of its other features. Details in `docs/ATTRIBUTION.md`.
- **Bokoblin Black: integration unavailable.** Bokoblin Expansion 0.17.0 offers no contract to register a Black at runtime. See `docs/BLACK_API_PROPOSAL.md`.
- FIX01 (0.1.1) changes only the panel and the language: the enemy chooser is a host-supported dropdown, and every user-visible text is English. See `docs/FIX01.md`.

Documents: `docs/FIX01.md`, `docs/IMPLEMENTATION.md`, `docs/ENEMY_CATALOG.md`, `docs/TEST_GUIDE.md`, `docs/CI_PROFILES.md`.
