// Enemy Spawner (Standalone) 0.1.1 FIX01: the panel in this mod's tab of the host Mods window. Included once by mod.cpp, and by tests/panel_lab.cpp
// against the real SDK UI header and a model of the host's ui_pane_add_control.
// The includer provides: svc_ui, svc_config, mod_ctx, es_press(), es_log().
//
// Why a DROPDOWN (Dusklight 2.0.3, TwilitRealm/dusklight@40457c6, read, not guessed):
//  * src/dusk/mods/svc/ui.cpp ui_register_mods_panel wraps the Mods panel with wrap_pane(mod, pane, nullptr): a Mods panel has NO help pane.
//  * ui_pane_add_control, case UI_CONTROL_SELECT: `if (slot->helpPane == nullptr) return MOD_UNSUPPORTED;` before anything else. help_rml is only
//    text the host would show in that pane; setting it creates no pane, so a SELECT can never be built here. 0.1.0 failed exactly there
//    ("pane_add_control: SELECT controls need a help pane (mod window tabs)" -> "mod UI panel build failed").
//  * case UI_CONTROL_DROPDOWN (UI service minor 3, which this mod already imports) builds a DropdownButton with the option labels and no help pane,
//    and wire_config_var_binding accepts it with an int config var, exactly like SELECT. Same var, same 45 indices, same order.

struct EsUi {
  ConfigVarHandle var = 0;  // the persisted index of the chosen enemy ("enemy", default 6 = Lizalfos)
} esui;

inline constexpr const char* kUiSection = "Enemy Spawner";
inline constexpr const char* kUiChooser = "Enemy";
inline constexpr const char* kUiButton = "Spawn Enemy";
inline constexpr const char* kUiBlackNotice = "Bokoblin Black: integration unavailable. Bokoblin Expansion 0.17.0 has no API to register a Black at runtime.";

// The button only records the press: the next mod_update, at the start of a frame, decides and creates. Nothing touches the engine here.
void on_invoke(ModContext*, void*) {
  std::int64_t index = -1;
  if (esui.var == 0 || svc_config->get_int(mod_ctx, esui.var, &index) != MOD_OK) index = -1;
  es_press(index);
}

ModResult es_ui_failed(ModError* error, const char* element, ModResult r) {
  char line[160];
  std::snprintf(line, sizeof(line), "ENEMY_SPAWNER event=UI_BUILD_FAILED element=%s result=%d", element, static_cast<int>(r));
  es_log(line);
  if (error) (void)mods::set_error(error, r, "Enemy Spawner: the Mods panel could not be built (see ENEMY_SPAWNER event=UI_BUILD_FAILED in the log)");
  return r;
}

ModResult build_panel(ModContext* ctx, UiElementHandle pane, void*, ModError* error) {
  ModResult r = svc_ui->pane_add_section(ctx, pane, kUiSection);
  if (r != MOD_OK) return es_ui_failed(error, "section", r);
  UiControlDesc chooser = UI_CONTROL_DESC_INIT;
  chooser.kind = UI_CONTROL_DROPDOWN;  // never UI_CONTROL_SELECT: see the top of this file
  chooser.label = kUiChooser;
  chooser.binding = UI_BINDING_CONFIG_VAR;
  chooser.config_var = esui.var;
  chooser.options = es::kLabels.v;  // static storage: valid for the life of the module
  chooser.option_count = es::kCatalogSize;
  r = svc_ui->pane_add_control(ctx, pane, &chooser, nullptr);
  if (r != MOD_OK) return es_ui_failed(error, "enemy_dropdown", r);
  UiControlDesc button = UI_CONTROL_DESC_INIT;
  button.kind = UI_CONTROL_BUTTON;
  button.label = kUiButton;
  button.on_pressed = on_invoke;
  r = svc_ui->pane_add_control(ctx, pane, &button, nullptr);
  if (r != MOD_OK) return es_ui_failed(error, "spawn_button", r);
  r = svc_ui->pane_add_text(ctx, pane, kUiBlackNotice, nullptr);
  if (r != MOD_OK) return es_ui_failed(error, "black_notice", r);
  return MOD_OK;
}
