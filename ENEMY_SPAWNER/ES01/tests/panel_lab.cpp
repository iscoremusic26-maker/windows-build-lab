// Enemy Spawner (Standalone) 0.1.1 FIX01: the PRODUCTION Mods panel (src/ui_panel.inc, included verbatim) built against the REAL SDK UI header
// (sdk/include/mods/svc/ui.h of TwilitRealm/dusklight@40457c6) and a model of the host's ui_pane_add_control (src/dusk/mods/svc/ui.cpp, same commit).
// Modelled host facts, read in the source:
//  * ui_register_mods_panel: wrap_pane(mod, pane, nullptr) -> the Mods panel has no help pane. A window tab's left pane has one (wrap_pane(.., &right)).
//  * UI_CONTROL_SELECT: helpPane == nullptr -> log "SELECT controls need a help pane (mod window tabs)", MOD_UNSUPPORTED. help_rml is not consulted.
//  * UI_CONTROL_DROPDOWN: options copied, no help pane needed. SELECT/DROPDOWN/NUMBER bind an INT config var (wire_config_var_binding);
//    an unknown handle or a var of another type -> MOD_INVALID_ARGUMENT.
// Stricter than the host on purpose (a host-accepted value that would fail later is a failure here): a button without on_pressed, a chooser
// without options, a descriptor that is not UI_CONTROL_DESC_INIT-sized.
// Declared limit: a model of the host contract, not the real RmlUi window. Only the game shows the real menu.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <mods/svc/ui.h>
#include <mods/svc/config.h>
#include "../src/spawner_core.hpp"

static unsigned long long g_checks = 0;
#define CHECK(x)                                              \
  do {                                                        \
    ++g_checks;                                               \
    if (!(x)) {                                               \
      std::printf("ES_LAB_FAIL line=%d %s\n", __LINE__, #x);   \
      std::exit(1);                                           \
    }                                                         \
  } while (0)

// ---------------------------------------------------------------- host model
static char g_ctx_storage[8];
ModContext* mod_ctx = reinterpret_cast<ModContext*>(g_ctx_storage);
constexpr UiElementHandle kModsPanel = 0x11, kWindowTabLeft = 0x21;  // the Mods panel (no help pane) and a window tab pane (with one)
struct Elem {
  int kind = -1;  // -1 section, -2 text, else UiControlKind
  std::string label;
  std::vector<std::string> options;
  const char* const* option_ptr = nullptr;
  ConfigVarHandle var = 0;
  UiPressedFn pressed = nullptr;
};
struct Var { ConfigVarHandle h; ConfigVarType type; std::int64_t value; };
static std::vector<Elem> g_elems;
static std::vector<Var> g_vars;
static std::vector<std::string> g_host_log;
static int g_fail_section = 0, g_fail_control_at = 0, g_fail_text = 0, g_controls = 0;
static bool g_get_int_fails = false;
static bool pane_ok(UiElementHandle p) { return p == kModsPanel || p == kWindowTabLeft; }
static bool has_help(UiElementHandle p) { return p == kWindowTabLeft; }
static Var* find_var(ConfigVarHandle h) {
  for (auto& v : g_vars)
    if (v.h == h) return &v;
  return nullptr;
}
static ModResult m_section(ModContext*, UiElementHandle pane, const char* title) {
  if (!pane_ok(pane) || !title) return MOD_INVALID_ARGUMENT;
  if (g_fail_section) return static_cast<ModResult>(g_fail_section);
  Elem e;
  e.kind = -1, e.label = title;
  g_elems.push_back(e);
  return MOD_OK;
}
static ModResult m_text(ModContext*, UiElementHandle pane, const char* text, UiElementHandle*) {
  if (!pane_ok(pane) || !text) return MOD_INVALID_ARGUMENT;
  if (g_fail_text) return static_cast<ModResult>(g_fail_text);
  Elem e;
  e.kind = -2, e.label = text;
  g_elems.push_back(e);
  return MOD_OK;
}
static ModResult m_control(ModContext*, UiElementHandle pane, const UiControlDesc* d, UiElementHandle*) {
  ++g_controls;
  if (!pane_ok(pane) || !d || d->struct_size != sizeof(UiControlDesc) || !d->label) return MOD_INVALID_ARGUMENT;
  if (g_fail_control_at && g_controls == g_fail_control_at) return MOD_ERROR;
  Elem e;
  e.kind = d->kind, e.label = d->label;
  switch (d->kind) {
    case UI_CONTROL_BUTTON:
      if (!d->on_pressed) return MOD_INVALID_ARGUMENT;
      e.pressed = d->on_pressed;
      break;
    case UI_CONTROL_DROPDOWN:
      if (!d->options || d->option_count == 0) return MOD_INVALID_ARGUMENT;
      for (std::size_t i = 0; i < d->option_count; ++i) {
        if (!d->options[i]) return MOD_INVALID_ARGUMENT;
        e.options.emplace_back(d->options[i]);
      }
      e.option_ptr = d->options;
      break;
    case UI_CONTROL_SELECT:
      if (!has_help(pane)) {  // the host's exact branch: before options, before binding, whatever help_rml holds
        g_host_log.push_back("pane_add_control: SELECT controls need a help pane (mod window tabs)");
        return MOD_UNSUPPORTED;
      }
      for (std::size_t i = 0; i < d->option_count; ++i) e.options.emplace_back(d->options[i]);
      break;
    default:
      return MOD_INVALID_ARGUMENT;
  }
  if (d->kind != UI_CONTROL_BUTTON) {
    if (d->binding != UI_BINDING_CONFIG_VAR) return MOD_INVALID_ARGUMENT;  // production binds the persisted var
    Var* v = find_var(d->config_var);
    if (!v || v->type != CONFIG_VAR_INT) {
      g_host_log.push_back("pane_add_control: config var handle is unknown or its type does not match the control kind");
      return MOD_INVALID_ARGUMENT;
    }
    e.var = d->config_var;
  }
  g_elems.push_back(e);
  return MOD_OK;
}
static ModResult m_get_int(ModContext*, ConfigVarHandle h, std::int64_t* out) {
  Var* v = find_var(h);
  if (g_get_int_fails || !v || v->type != CONFIG_VAR_INT) {
    *out = 12345;  // stricter than the host: an output written on failure must still never be pressed
    return MOD_INVALID_ARGUMENT;
  }
  *out = v->value;
  return MOD_OK;
}
static UiService g_ui_service{};
static ConfigService g_config_service{};
const UiService* svc_ui = nullptr;
const ConfigService* svc_config = nullptr;
// Includer-provided names of the production panel
static std::vector<std::int64_t> g_presses;
static std::vector<std::string> g_log;
static void es_press(std::int64_t index) { g_presses.push_back(index); }  // the real one only queues (tests/adapter_lab.cpp)
static void es_log(const char* l) { g_log.push_back(l); }

#include "../src/ui_panel.inc"

// ---------------------------------------------------------------- scenarios
static void reset() {
  g_elems.clear(), g_host_log.clear(), g_log.clear(), g_presses.clear();
  g_fail_section = g_fail_control_at = g_fail_text = g_controls = 0;
  g_get_int_fails = false;
  g_vars.clear();
  g_vars.push_back({0x77, CONFIG_VAR_INT, 6});  // "enemy", as mod_initialize registers it: int, default 6
  g_vars.push_back({0x78, CONFIG_VAR_BOOL, 0});
  esui.var = 0x77;
}
static const Elem* elem_of(int kind) {
  for (auto& e : g_elems)
    if (e.kind == kind) return &e;
  return nullptr;
}
static bool logged(const char* part) {
  for (auto& l : g_log)
    if (l.find(part) != std::string::npos) return true;
  return false;
}
// The 0.1.0 chooser, literally: a SELECT in the Mods panel.
static ModResult add_v010_chooser(UiElementHandle pane, const char* help) {
  UiControlDesc sel = UI_CONTROL_DESC_INIT;
  sel.kind = UI_CONTROL_SELECT;
  sel.label = "Enemigo";
  sel.help_rml = help;
  sel.binding = UI_BINDING_CONFIG_VAR;
  sel.config_var = esui.var;
  sel.options = es::kLabels.v;
  sel.option_count = es::kCatalogSize;
  return svc_ui->pane_add_control(mod_ctx, pane, &sel, nullptr);
}

static void lab_root_cause() {
  // The incident of log 091855, reproduced: the 0.1.0 chooser in the Mods panel is refused by the host, with or without help_rml.
  for (const char* help : {static_cast<const char*>(nullptr), "Choose an enemy."}) {
    reset();
    CHECK(add_v010_chooser(kModsPanel, help) == MOD_UNSUPPORTED);
    CHECK(g_host_log.size() == 1 && g_host_log[0].find("SELECT controls need a help pane") != std::string::npos && g_elems.empty());
  }
  // The same SELECT is fine where a help pane exists: the control type, not its content, is what the Mods panel cannot host.
  reset();
  CHECK(add_v010_chooser(kWindowTabLeft, nullptr) == MOD_OK);
}

static void lab_production_panel() {
  reset();
  ModError err = MOD_ERROR_INIT;
  CHECK(build_panel(mod_ctx, kModsPanel, nullptr, &err) == MOD_OK);
  CHECK(g_host_log.empty() && g_elems.size() == 4);
  CHECK(g_elems[0].kind == -1 && g_elems[0].label == "Enemy Spawner");
  const Elem* chooser = elem_of(UI_CONTROL_DROPDOWN);
  CHECK(chooser && chooser == &g_elems[1] && chooser->label == "Enemy" && chooser->var == esui.var && elem_of(UI_CONTROL_SELECT) == nullptr);
  CHECK(chooser->options.size() == es::kCatalogSize && chooser->options.size() == 45 && chooser->option_ptr == es::kLabels.v);
  for (std::size_t i = 0; i < es::kCatalogSize; ++i) CHECK(chooser->options[i] == es::kCatalog[i].label);
  CHECK(chooser->options[6] == "Lizalfos");  // the default index of the "enemy" var
  CHECK(chooser->options[5] == "Bokoblin Black (Expansion): integration unavailable");
  const Elem* button = elem_of(UI_CONTROL_BUTTON);
  CHECK(button && button == &g_elems[2] && button->label == "Spawn Enemy" && button->pressed == on_invoke);
  CHECK(g_elems[3].kind == -2 && g_elems[3].label.find("Bokoblin Black") != std::string::npos && g_elems[3].label.find("unavailable") != std::string::npos);
  // Reopening the tab rebuilds the panel: the same elements again, MOD_OK again, the same persisted var.
  g_elems.clear();
  CHECK(build_panel(mod_ctx, kModsPanel, nullptr, &err) == MOD_OK && g_elems.size() == 4 && elem_of(UI_CONTROL_DROPDOWN)->var == 0x77);
}

static void lab_selection_and_press() {
  reset();
  ModError err = MOD_ERROR_INIT;
  CHECK(build_panel(mod_ctx, kModsPanel, nullptr, &err) == MOD_OK);
  const Elem* button = elem_of(UI_CONTROL_BUTTON);
  // Default: Lizalfos. Every option, chosen in the dropdown (the host writes the bound var), then pressed: that index and only a queue.
  button->pressed(mod_ctx, nullptr);
  CHECK(g_presses.size() == 1 && g_presses[0] == 6);
  for (std::int64_t i = 0; i < std::int64_t(es::kCatalogSize); ++i) {
    find_var(0x77)->value = i;
    button->pressed(mod_ctx, nullptr);
    CHECK(g_presses.back() == i);
  }
  CHECK(g_presses.size() == es::kCatalogSize + 1);
  // A damaged persisted value or an unreadable var reaches the core as an invalid index (refused there, tests/core_lab.cpp).
  find_var(0x77)->value = 99;
  button->pressed(mod_ctx, nullptr);
  CHECK(g_presses.back() == 99);
  g_get_int_fails = true;
  button->pressed(mod_ctx, nullptr);
  CHECK(g_presses.back() == -1);
  g_get_int_fails = false;
  esui.var = 0;  // no var registered: never read, never a real index
  on_invoke(mod_ctx, nullptr);
  CHECK(g_presses.back() == -1 && g_presses.size() == es::kCatalogSize + 4);
}

static void lab_failures_named() {
  struct F { int section, control_at, text; const char* element; } cases[] = {
      {MOD_ERROR, 0, 0, "element=section"}, {0, 1, 0, "element=enemy_dropdown"}, {0, 2, 0, "element=spawn_button"}, {0, 0, MOD_UNAVAILABLE, "element=black_notice"}};
  for (const F& c : cases) {
    reset();
    g_fail_section = c.section, g_fail_control_at = c.control_at, g_fail_text = c.text;
    ModError err = MOD_ERROR_INIT;
    const ModResult r = build_panel(mod_ctx, kModsPanel, nullptr, &err);
    CHECK(r != MOD_OK && logged(c.element) && logged("UI_BUILD_FAILED") && err.code == r && std::strstr(err.message, "Mods panel") != nullptr);
  }
  // The chooser bound to a var the host does not know, or of the wrong type: refused, and named.
  for (ConfigVarHandle bad : {ConfigVarHandle(0), ConfigVarHandle(0x78), ConfigVarHandle(0x99)}) {
    reset();
    esui.var = bad;
    ModError err = MOD_ERROR_INIT;
    CHECK(build_panel(mod_ctx, kModsPanel, nullptr, &err) == MOD_INVALID_ARGUMENT && logged("element=enemy_dropdown"));
  }
}

static void lab_english() {
  static const char* kSpanish[] = {"solicitud", "enviada", "aparecido", "invoco", "invocar", "invocacion", "creacion", "clasifico", "cargado", "integracion", "disponible",
                                   "ya hay", "limite", "seleccion", "valida", "enemigo", "suelo", "seguro", "sesion", "nativo", "noche", "juego", "delante"};
  std::vector<std::string> visible = {kUiSection, kUiChooser, kUiButton, kUiBlackNotice};
  for (std::size_t i = 0; i < es::kCatalogSize; ++i) visible.emplace_back(es::kCatalog[i].label);
  for (std::string s : visible) {
    for (unsigned char ch : s) CHECK(ch >= 0x20 && ch < 0x7f);
    for (auto& ch : s) ch = static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
    for (const char* w : kSpanish) CHECK(s.find(w) == std::string::npos);
  }
}

int main() {
  g_ui_service.pane_add_section = m_section;
  g_ui_service.pane_add_control = m_control;
  g_ui_service.pane_add_text = m_text;
  g_config_service.get_int = m_get_int;
  svc_ui = &g_ui_service, svc_config = &g_config_service;
  lab_root_cause();
  lab_production_panel();
  lab_selection_and_press();
  lab_failures_named();
  lab_english();
  std::printf("ES_PANEL_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_PANEL_REAL_SDK_UI_HEADER_HOST_CONTROL_MODEL runtime=HUMAN_RUNTIME_PENDING\n", es::kVersion, g_checks);
  return 0;
}
