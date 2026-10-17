# DEVF CP10.2 — Black Bokoblin (first Bokoblin Expansion variant)

Editor **0.10.2** (`DEVF Map Editor CP10.2.exe`) + Expansion **0.4.0** (`BokoblinExpansion_0.4.0_TEST.dusk`, id `dev.bokoblin.expansion`). The CP10 provider (`DEVF_Provider_0.10.0_TEST.dusk`, id `dev.devf.cp06_native_config_dual`) is **reused unchanged and not rebuilt**.

A Bokoblin encounter can now be **Normal** (default, exactly as before) or **Black**: charcoal/dark-slate body (eyes, accessories and every other actor keep native colors) and its **own HP** (1–999, default 150). Everything else about the encounter (record, position rules, layer, capacity 8, QA flow) is unchanged.

- Schema: optional `bokoblin_variant` (`"black"`; `"normal"` accepted) and `hp` per encounter. A document without Black Bokoblins serializes **byte-identically** to CP10. Black requires `hp`; Normal forbids it. Schema1 never carries them.
- Editor: Variant combo (Normal / Black) and HP box in the multi-room editor; list shows `bokoblin-005 [Black · HP 150]` / `[Normal]`; details show the variant; black markers have a dark center dot.
- Runtime (Expansion only): when the file contains a Black Bokoblin, the Expansion installs the same two hooks proven in CP03/CP04.4 (`daE_OC_c::execute` for the one-shot HP write, `J3DShapePacket::drawFast` for the body tint). A Normal-only file installs nothing and behaves like 0.3.0.
- Tint: the native body color is multiplied by ~35% (90/255, 90/255, 98/255) instead of CP04.4's ~3%: readable charcoal, not crushed black.

Details: `IMPLEMENTATION.md`. Human QA: `HUMAN_TEST.md`.

## Status labels
- CI (Windows build, tests, package/PE/ABI audit, independent download): see the pull request / run.
- SOURCE_VERIFIED: code review and offline labs (C# schema/editor rules, C++ decoder and policy labs).
- **HUMAN_RUNTIME_PENDING**: actual appearance, HP and combat in game. Nothing here is a gameplay PASS.
