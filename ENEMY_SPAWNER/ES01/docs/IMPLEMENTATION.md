# Implementation

- `src/catalog.hpp`, `src/spawner_core.hpp`: pure logic (catalog, presses, safe moment, ground, tracking, Expansion observation).
- `src/spawner_adapter.inc`: what touches the engine. Included by `mod.cpp` and `tests/adapter_lab.cpp`.
- `src/ui_panel.inc`: the Mods panel (FIX01). Included by `mod.cpp` and `tests/panel_lab.cpp`.
- `src/mod.cpp`: services, toasts, the ground probe and the SDK type checks.

## Flow
1. The button only records the press. It refuses Black, an absent or refuted Expansion, a press already pending, and the limit.
2. At the start of the next frame (`mod_update`), if the scene is safe (Link, a valid layer, no scene change, event or pause), points at 300, 220 and 160 in front of Link are tried. After 10 s without a safe moment, the press expires.
3. At the first valid point, `create_actor` is called with Link's layer current (Dawnlight's rule), and the previous layer is always restored.
4. The returned ID is tracked read-only with `fopAcM_SearchByID` and `fpcM_IsCreating`: request submitted, spawned, or gone. No actor is ever deleted.
5. Bokoblin with Expansion: the spawned actor's `health` and `field_0x560` are read. 40→220 is Blue and 220→330 is Red, the signature of Expansion 0.17.0. If that does not appear, the result is reported as native, and the Expansion entries are disabled for that session.

## Native writes
- `fpcLy_SetCurrentLayer`: around a single `create_actor` call.
- Toadpoli: `mExecuteState` / `mActionTimer[3]` of an actor the mod created, every frame before its Execute.

No HP, AI, state or position of any actor is written. No hook is installed.

## Compatibility
- Light Fades Twice: the spawned Lizalfos is a native E_DN. LFT admits it through its own hook, with no code from this mod.
- Bokoblin Expansion: it classifies Blue and Red. This mod only observes the result.
- Detection: the framework service being present is necessary but not sufficient. The confirmation is what Expansion leaves on a freshly spawned Bokoblin.
