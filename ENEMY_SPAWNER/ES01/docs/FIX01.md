# FIX01 (0.1.1): the Mods panel failed to build

## The incident
Dusklight 2.0.3 (build rev `40457c6`), log `dusklight-20261111-091855`: the mod initialized (`ENEMY_SPAWNER event=INSTALL version=0.1.0`) and then
`pane_add_control: SELECT controls need a help pane (mod window tabs)` followed by `failed: mod UI panel build failed (1)`. No enemy was ever requested.

## Root cause (host source, TwilitRealm/dusklight@40457c6)
- `src/dusk/mods/svc/ui.cpp`, `ui_register_mods_panel`: the Mods panel is wrapped with `wrap_pane(mod, pane, nullptr)`. It has **no help pane**.
- `ui_pane_add_control`, case `UI_CONTROL_SELECT`: `if (slot->helpPane == nullptr) return MOD_UNSUPPORTED;`. The check runs before the options and before the binding.
- `help_rml` is only text the host would display in that pane. Setting it does not create a pane, so a SELECT can never be built in a Mods panel, with or without `help_rml`.
- Case `UI_CONTROL_DROPDOWN` (UI service minor 3) builds a `DropdownButton` from the option labels without a help pane. `wire_config_var_binding` accepts it with the same int config var. `src/dusk/ui/mod_window.cpp` `build_mod_control` builds the dropdown without a help pane, and `docs/modding.md` states that SELECT is only available in window tabs.
- The mod imports UiService 2.3. The user's log shows the mod was activated, so the host provided UI ≥ 2.3, where DROPDOWN exists.

## The change
- `src/ui_panel.inc`: the panel builder, now a separate unit. The chooser is `UI_CONTROL_DROPDOWN` with the same config var (`enemy`, default 6 = Lizalfos), the same 45 options and the same order. Every `pane_add_*` result is checked, and a failure logs `ENEMY_SPAWNER event=UI_BUILD_FAILED element=<section|enemy_dropdown|spawn_button|black_notice> result=<n>`.
- The button still only queues the request. The next `mod_update` decides and spawns, exactly as in 0.1.0.
- Every user-visible text is English: catalog labels, panel, toasts, errors, documents.
- Version 0.1.1, same mod ID. Spawning behavior, the catalog contents and order, the limits and the compatibility rules are unchanged.

## Regression tests
- `tests/panel_lab.cpp` compiles the production builder against the real SDK `mods/svc/ui.h` and a model of the host's `ui_pane_add_control`. It reproduces the 0.1.0 chooser being refused in a Mods panel, with and without `help_rml`, and accepted in a window tab. It then checks that the production panel builds with a dropdown, a button and a notice, that every option can be chosen and pressed, and that each failure is named.
- `ci/english_audit.py`: every production string literal, the manifest and the documents, with its own negatives.
- New mutants of the panel (for example the chooser back to SELECT, with or without `help_rml`).
