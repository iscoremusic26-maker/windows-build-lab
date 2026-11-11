// Enemy Spawner (Standalone) 0.1.0 for Dusklight 2.0.3. One job: create real native enemies in front of Link from the Mods window.
//
// Based on the Enemy Spawner of Dawnlight by BeZide93 (https://github.com/BeZide93/dawnlight, CC0 1.0 Universal), commit
// b5e9046cd98bdd8593ab2cf219d72a0f61ef7213. Not Dawnlight, not affiliated with it, none of its other features. See docs/ATTRIBUTION.md.
//
// No hook. No other mod is called or written. Bokoblin Expansion is only detected through the presence of the framework service it
// requires and through what its classification leaves in a Bokoblin this mod created (read only).
#include <mods/service.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/actor.h>
#include <mods/svc/config.h>
#include <mods/svc/ui.h>
#include "SSystem/SComponent/c_math.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_e_tk.h"
#include "d/actor/d_a_e_tk2.h"
#include "d/d_com_inf_game.h"
#include "d/d_bg_s_gnd_chk.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_manager.h"
#include "f_pc/f_pc_name.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#include "spawner_core.hpp"

DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ActorService, svc_actor);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(UiService, svc_ui);

#if defined(_MSC_VER)
#define ES_NOINLINE __declspec(noinline)
#else
#define ES_NOINLINE __attribute__((noinline))
#endif

namespace {
// The SDK layouts this mod relies on (Dusklight 2.0.3, TwilitRealm/dusklight@40457c6). A different host fails the build, never the game.
static_assert(std::is_same_v<decltype(&fpcLy_SetCurrentLayer), void (*)(layer_class*)> && std::is_same_v<decltype(&fpcLy_CurrentLayer), layer_class* (*)()> &&
                  std::is_same_v<decltype(&fpcLy_RootLayer), layer_class* (*)()> && std::is_same_v<decltype(&fpcLy_IsDeletingMesg), BOOL (*)(layer_class*)>,
              "fpcLy layer API");
static_assert(std::is_same_v<decltype(ActorService::create_actor), ModResult (*)(ModContext*, ProfileName, const ActorSpawnParams*, ActorId*)>, "ActorService::create_actor");
static_assert(sizeof(ActorSpawnParams::parameters) == 4 && sizeof(ActorSpawnParams::argument) == 1 && sizeof(ActorSpawnParams::room_num) == 1, "ActorSpawnParams");
// Field accesses go by name through the SDK headers: the header comments give GameCube offsets, the x64 layout is the compiler's. Types only:
static_assert(std::is_same_v<decltype(fopEn_enemy_c::field_0x560), s16> && std::is_same_v<decltype(fopEn_enemy_c::health), s16>, "enemy health fields");
static_assert(std::is_same_v<decltype(daE_OC_c::mName), const char*>, "daE_OC_c::mName");
static_assert(std::is_same_v<decltype(e_tk_class::mExecuteState), s16> && std::is_same_v<decltype(e_tk2_class::mActionTimer), s16[4]>, "Toadpoli fields");

// es::Profile -> native profile name, in enum order.
constexpr s16 kProfileName[] = {fpcNm_B_TN_e, fpcNm_E_OC_e, fpcNm_E_FZ_e, fpcNm_E_BA_e, fpcNm_E_TT_e, fpcNm_E_GI_e, fpcNm_B_GG_e, fpcNm_E_KK_e, fpcNm_E_FB_e, fpcNm_E_BS_e, fpcNm_E_BU_e,
                                fpcNm_E_MS_e, fpcNm_E_WW_e, fpcNm_E_FS_e, fpcNm_E_CR_e, fpcNm_E_SH_e, fpcNm_E_TK_e, fpcNm_E_TK2_e, fpcNm_E_RD_e, fpcNm_E_DN_e, fpcNm_E_DD_e, fpcNm_E_MF_e,
                                fpcNm_E_ST_e, fpcNm_E_HB_e, fpcNm_E_GB_e, fpcNm_E_DB_e, fpcNm_E_SF_e, fpcNm_E_MM_e, fpcNm_E_KR_e, fpcNm_E_GE_e, fpcNm_E_AI_e, fpcNm_E_SM2_e};
static_assert(sizeof(kProfileName) / sizeof(kProfileName[0]) == std::size_t(es::Profile::Count), "one native profile per catalog profile");
s16 es_profile(es::Profile p) noexcept { return kProfileName[std::size_t(p)]; }

// The framework Bokoblin Expansion 0.17.0 requires (Enemy Variant Framework 0.15.0). Its presence is necessary, not sufficient.
constexpr const char* kFrameworkService = "dev.devf.encounter_placement.v11";
bool es_framework_present() noexcept {
  const void* s = nullptr;
  return svc_host->get_service && svc_host->get_service(mod_ctx, kFrameworkService, 1, 0, &s) == MOD_OK && s != nullptr;
}

void es_log(const char* line) noexcept;
void es_toast(const char* title, const char* body, bool warning) noexcept;

// Ground at a point in front of Link: the native ground probe from 500 above (Dawnlight), its slope, water/lava there, and a wall in between.
es::GroundSample es_sample_ground(daAlink_c* link, const cXyz& at) noexcept {
  es::GroundSample g;
  g.link_y = link->current.pos.y;
  cXyz probe = at;
  probe.y += es::policy::kProbeAbove;
  dBgS_ObjGndChk ground;
  ground.SetPos(&probe);
  const float y = dComIfG_Bgsp().GroundCross(&ground);
  g.found = y != -1.0e9f && y > -1.0e8f;
  if (!g.found) return g;
  g.ground_y = y;
  cM3dGPla plane;
  if (dComIfG_Bgsp().GetTriPla(ground, &plane)) g.normal_y = plane.mNormal.y;
  cXyz on = at;
  on.y = y + 1.0f;
  if (fopAcM_wt_c::waterCheck(&on)) g.water = true, g.water_y = fopAcM_wt_c::getWaterY();
  cXyz from = link->current.pos, to = at;
  from.y += es::policy::kSightHeight;
  to.y = y + es::policy::kSightHeight;
  g.blocked = fopAcM_lc_c::lineCheck(&from, &to, link);
  return g;
}

#include "spawner_adapter.inc"

unsigned es_lines = 0;
std::uint64_t es_lines_dropped = 0;
void es_log(const char* line) noexcept {
  if (es_lines >= 400) {
    ++es_lines_dropped;
    return;
  }
  ++es_lines;
  svc_log->write(mod_ctx, LOG_LEVEL_INFO, line);
}
void es_toast(const char* title, const char* body, bool warning) noexcept {
  if (!esr.ui || !svc_ui->push_toast) return;
  UiToastDesc d = UI_TOAST_DESC_INIT;
  d.type = warning ? "warning" : nullptr;
  d.title_rml = title;
  d.body_rml = body;
  (void)svc_ui->push_toast(mod_ctx, &d);
}

void on_invoke(ModContext*, void*) noexcept {
  std::int64_t index = -1;
  if (!esr.var || svc_config->get_int(mod_ctx, esr.var, &index) != MOD_OK) index = -1;
  es_press(index);
}

ModResult build_panel(ModContext* ctx, UiElementHandle pane, void*, ModError*) {
  if (svc_ui->pane_add_section(ctx, pane, "Enemy Spawner") != MOD_OK) return MOD_ERROR;
  UiControlDesc sel = UI_CONTROL_DESC_INIT;
  sel.kind = UI_CONTROL_SELECT;
  sel.label = "Enemigo";
  sel.binding = UI_BINDING_CONFIG_VAR;
  sel.config_var = esr.var;
  sel.options = es::kLabels.v;
  sel.option_count = es::kCatalogSize;
  if (svc_ui->pane_add_control(ctx, pane, &sel, nullptr) != MOD_OK) return MOD_ERROR;
  UiControlDesc btn = UI_CONTROL_DESC_INIT;
  btn.kind = UI_CONTROL_BUTTON;
  btn.label = "Invocar enemigo";
  btn.on_pressed = on_invoke;
  if (svc_ui->pane_add_control(ctx, pane, &btn, nullptr) != MOD_OK) return MOD_ERROR;
  return svc_ui->pane_add_text(ctx, pane, "Black: integracion no disponible (Bokoblin Expansion 0.17.0 no ofrece un registro dinamico de Black).", nullptr);
}
}  // namespace

MOD_EXPORT ModResult mod_initialize(ModError* error) {
  ConfigVarDesc d = CONFIG_VAR_DESC_INIT;
  d.name = "enemy";
  d.type = CONFIG_VAR_INT;
  d.default_int = 6;  // Lizalfos
  if (svc_config->register_var(mod_ctx, &d, &esr.var) != MOD_OK || esr.var == 0) return mods::set_error(error, MOD_ERROR, "Enemy Spawner: config var rejected");
  UiModsPanelDesc p = UI_MODS_PANEL_DESC_INIT;
  p.build = build_panel;
  if (svc_ui->register_mods_panel(mod_ctx, &p) != MOD_OK) return mods::set_error(error, MOD_ERROR, "Enemy Spawner: Mods panel rejected");
  esr.ui = true;
  char line[200];
  std::snprintf(line, sizeof(line), "ENEMY_SPAWNER event=INSTALL version=%s entries=%u max_live=%u hooks=0 black=UNAVAILABLE_NO_PUBLIC_CONTRACT runtime=HUMAN_RUNTIME_PENDING", es::kVersion,
                unsigned(es::kCatalogSize), es::policy::kMaxLive);
  es_log(line);
  return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
  es_update();
  return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
  const auto& c = esr.sp.c;
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  char line[400];
  std::snprintf(line, sizeof(line),
                "ENEMY_SPAWNER_SUMMARY version=%s presses=%llu accepted=%llu refused=%llu expired=%llu rejected_ground=%llu created=%llu failed=%llu appeared=%llu gone=%llu timeouts=%llu "
                "expansion_classified=%llu expansion_not_classified=%llu id_reused=%llu layer_restores=%llu adapt_writes=%llu expansion=%s lines_dropped=%llu runtime=HUMAN_RUNTIME_PENDING",
                es::kVersion, u(c.presses), u(c.accepted), u(c.refused), u(c.expired), u(c.rejected), u(c.created), u(c.failed), u(c.appeared), u(c.gone), u(c.timeouts), u(c.classified),
                u(c.not_classified), u(c.id_reused), u(esr.layer_restores), u(esr.adapt_writes), es::name(esr.sp.expansion), u(es_lines_dropped + c.events_dropped));
  es_lines = 0;
  es_log(line);
  esr.sp.forget_all();  // no id is looked up after this; the actors stay in their scene, owned by the engine
  esr.ui = false;
  return MOD_OK;
}
