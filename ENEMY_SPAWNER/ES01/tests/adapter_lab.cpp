// Enemy Spawner (Standalone) 0.1.0: the PRODUCTION adapter (src/spawner_adapter.inc, included verbatim) over a model of the engine rules it depends on.
// Model, from TwilitRealm/dusklight@40457c6 (inspection, not execution):
//  * ActorService::create_actor -> fopAcM_create -> fpcM_Create records fpcLy_CurrentLayer() in the asynchronous request (src/dusk/mods/svc/actor.cpp,
//    f_pc_manager). The actor is later owned by that layer.
//  * A layer's actors are deleted with it; a root-owned actor survives the play scene. The play scene draws its actors through the actor queue,
//    and the root traversal draws root-owned processes too: a root-owned enemy is drawn twice (Dawnlight docs/darknut-arena-spawner.md).
//  * fopAcM_SearchByID finds a process once it is in the actor queue (first create step); fpcM_IsCreating stays true until its create completes.
// Declared limits: no renderer, no collision, no real actor: a model of the rules above, not a run of the game.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
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

// ---------------------------------------------------------------- engine model
using s16 = std::int16_t;
using u32 = std::uint32_t;
using BOOL = int;
enum ModResult { MOD_OK = 0, MOD_ERROR = 1, MOD_UNAVAILABLE = 2, MOD_INVALID_ARGUMENT = 5 };
struct ModContext {};
static ModContext g_ctx;
static ModContext* mod_ctx = &g_ctx;
using ProfileName = std::int16_t;
using ActorId = std::uint32_t;
using ConfigVarHandle = std::uint64_t;
struct ActorSpawnParams {
  std::uint32_t parameters;
  std::int8_t argument, room_num;
  struct { float x, y, z; } position;
  struct { std::int16_t x, y, z; } angle;
  struct { float x, y, z; } scale;
  int (*create_function)(void*);
};
struct layer_class { bool deleting = false; int name = 0; };
static layer_class g_root{false, 0}, g_play{false, 1};
static layer_class* g_current = &g_root;
static layer_class* fpcLy_CurrentLayer() { return g_current; }
static layer_class* fpcLy_RootLayer() { return &g_root; }
static void fpcLy_SetCurrentLayer(layer_class* l) { g_current = l; }
static BOOL fpcLy_IsDeletingMesg(layer_class* l) { return l->deleting; }
struct cXyz {
  float x = 0, y = 0, z = 0;
  cXyz() = default;
  cXyz(float a, float b, float c) : x(a), y(b), z(c) {}
};
static float cM_ssin(s16 a) { return std::sin(a * 3.14159265f / 32768.0f); }
static float cM_scos(s16 a) { return std::cos(a * 3.14159265f / 32768.0f); }
struct csXyz { s16 x = 0, y = 0, z = 0; };
struct fopAc_ac_c {
  s16 name = 0;
  std::uint32_t id = 0;
  cXyz pos;
  struct { cXyz pos; } current;
  csXyz shape_angle;
  int room = 3;
  virtual ~fopAc_ac_c() = default;
};
struct layer_tag_t { layer_class* layer = &g_play; };
struct daAlink_c : fopAc_ac_c { layer_tag_t layer_tag; };
struct e_tk_class : fopAc_ac_c { s16 mExecuteState = 20; };
struct e_tk2_class : fopAc_ac_c { s16 mActionTimer[4] = {0, 0, 0, 20}; };
struct daE_OC_c : fopAc_ac_c { const char* mName = "E_OC"; s16 field_0x560 = 40, health = 40; };
constexpr s16 fpcNm_E_OC_e = 510, fpcNm_E_TK_e = 600, fpcNm_E_TK2_e = 601, fpcNm_E_DN_e = 435;
static daAlink_c g_link_obj;
static daAlink_c* g_link = &g_link_obj;
static daAlink_c* daAlink_getAlinkActorClass() { return g_link; }
static bool g_next_stage = false, g_event = false, g_pause = false;
static BOOL dComIfGp_isEnableNextStage() { return g_next_stage; }
static BOOL dComIfGp_event_runCheck() { return g_event; }
static std::uint8_t dComIfGp_isPauseFlag() { return g_pause; }
static int fopAcM_GetRoomNo(const fopAc_ac_c* a) { return a->room; }
static s16 fopAcM_GetName(const fopAc_ac_c* a) { return a->name; }
// Requests and actors
struct Request { ActorId id; ProfileName profile; ActorSpawnParams p; layer_class* owner; };
static std::vector<Request> g_requests;
struct Live { fopAc_ac_c* a; layer_class* owner; bool creating; };
static std::vector<Live> g_live;
static std::uint32_t g_next_id = 100;
static ModResult g_service_result = MOD_OK;
static bool g_service_throws = false;
static ModResult create_actor(ModContext*, ProfileName profile, const ActorSpawnParams* p, ActorId* id) {
  if (g_service_throws) throw std::runtime_error("service failure");
  if (g_service_result != MOD_OK) return g_service_result;
  *id = ++g_next_id;
  g_requests.push_back({*id, profile, *p, fpcLy_CurrentLayer()});  // fpcM_Create: the CURRENT layer, now
  return MOD_OK;
}
struct ActorService { ModResult (*create_actor)(ModContext*, ProfileName, const ActorSpawnParams*, ActorId*); };
static ActorService g_actor_service{&create_actor};
static const ActorService* svc_actor = &g_actor_service;
static int fopAcM_SearchByID(std::uint32_t id, fopAc_ac_c** out) {
  *out = nullptr;
  for (auto& l : g_live)
    if (l.a->id == id) *out = l.a;
  return *out != nullptr;
}
static BOOL fpcM_IsCreating(std::uint32_t id) {
  for (auto& l : g_live)
    if (l.a->id == id) return l.creating;
  for (auto& r : g_requests)
    if (r.id == id) return 1;
  return 0;
}
// The engine turns a request into an actor of the requested profile, in the owner layer.
static fopAc_ac_c* g_made = nullptr;
static fopAc_ac_c* materialize(const Request& r, bool creating) {
  fopAc_ac_c* a = nullptr;
  if (r.profile == fpcNm_E_OC_e) {
    auto* b = new daE_OC_c;
    const bool e_oc2 = ((r.p.angle.x >> 8) & 0xff) != 0 && ((r.p.angle.x >> 8) & 0xff) != 0xff;  // d_a_e_oc.cpp create()
    b->mName = e_oc2 ? "E_OC2" : "E_OC";
    b->field_0x560 = b->health = e_oc2 ? 220 : 40;
    a = b;
  } else if (r.profile == fpcNm_E_TK_e) {
    a = new e_tk_class;
  } else if (r.profile == fpcNm_E_TK2_e) {
    a = new e_tk2_class;
  } else {
    a = new fopAc_ac_c;
  }
  a->name = r.profile, a->id = r.id;
  g_live.push_back({a, r.owner, creating});
  g_made = a;
  return a;
}
static void engine_create_all(bool finish = true) {
  for (auto& r : g_requests) materialize(r, !finish);
  g_requests.clear();
}
static void delete_layer(layer_class* l) {  // scene teardown: its actors and its pending requests go with it
  for (auto it = g_live.begin(); it != g_live.end();)
    if (it->owner == l) delete it->a, it = g_live.erase(it);
    else ++it;
  for (auto it = g_requests.begin(); it != g_requests.end();)
    if (it->owner == l) it = g_requests.erase(it);
    else ++it;
}
static unsigned draws_of(const fopAc_ac_c* a) {  // actor queue of the play scene + root traversal of root-owned processes
  for (auto& l : g_live)
    if (l.a == a) return (l.owner == &g_play ? 1u : 0u) + (l.owner == &g_root ? 2u : 0u);
  return 0;
}
// Includer-provided names of the production adapter
#define ES_NOINLINE
static s16 es_profile(es::Profile p) { return p == es::Profile::E_OC ? fpcNm_E_OC_e : p == es::Profile::E_TK ? fpcNm_E_TK_e : p == es::Profile::E_TK2 ? fpcNm_E_TK2_e : fpcNm_E_DN_e; }
static es::GroundSample g_ground;
static int g_ground_calls = 0;
static es::GroundSample es_sample_ground(daAlink_c* link, const cXyz&) {
  ++g_ground_calls;
  es::GroundSample g = g_ground;
  g.link_y = link->current.pos.y;
  return g;
}
static bool g_framework = false;
static bool es_framework_present() { return g_framework; }
static std::vector<std::string> g_log, g_toasts;
static void es_log(const char* l) { g_log.push_back(l); }
static void es_toast(const char*, const char* body, bool) { g_toasts.push_back(body); }

#include "../src/spawner_adapter.inc"

// ---------------------------------------------------------------- scenarios
static int index_of(const char* label) {
  for (std::size_t i = 0; i < es::kCatalogSize; ++i)
    if (!std::strcmp(es::kCatalog[i].label, label)) return int(i);
  return -1;
}
static void reset() {
  for (auto& l : g_live) delete l.a;
  g_live.clear(), g_requests.clear(), g_log.clear(), g_toasts.clear();
  esr.~EsRuntime();
  new (&esr) EsRuntime();
  g_current = &g_root;
  g_play.deleting = false;
  g_link = &g_link_obj;
  g_link->layer_tag.layer = &g_play;
  g_link->current.pos = cXyz(100, 50, -20);
  g_link->shape_angle.y = 0x4000;
  g_next_stage = g_event = g_pause = false;
  g_service_result = MOD_OK, g_service_throws = false;
  g_ground = es::GroundSample{};
  g_ground.found = true, g_ground.ground_y = 50, g_ground.normal_y = 1;
  g_framework = false;
}
static bool toasted(const char* part) {
  for (auto& t : g_toasts)
    if (t.find(part) != std::string::npos) return true;
  return false;
}
static bool logged(const char* part) {
  for (auto& t : g_log)
    if (t.find(part) != std::string::npos) return true;
  return false;
}

static void lab_ownership() {
  // What a request on the current (root) layer would do: the bug Dawnlight fixed. Shown with the engine model itself.
  reset();
  ActorId bad = 0;
  ActorSpawnParams p{};
  CHECK(create_actor(mod_ctx, fpcNm_E_DN_e, &p, &bad) == MOD_OK && g_requests.back().owner == &g_root);
  engine_create_all();
  fopAc_ac_c* root_owned = g_made;
  CHECK(draws_of(root_owned) == 2);  // drawn by the play queue AND the root traversal
  delete_layer(&g_play);
  CHECK(draws_of(root_owned) == 2);  // and it survives its scene
  // The production helper: Link's layer during the call, the caller's layer back afterwards.
  reset();
  ActorId id = 0;
  CHECK(es_create_in_link_layer(g_link, fpcNm_E_DN_e, p, id) == MOD_OK);
  CHECK(g_requests.size() == 1 && g_requests[0].owner == &g_play && g_current == &g_root && esr.layer_restores == 1);
  engine_create_all();
  CHECK(draws_of(g_made) == 1);
  delete_layer(&g_play);
  fopAc_ac_c* a = nullptr;
  CHECK(fopAcM_SearchByID(id, &a) == 0);  // gone with its scene
  // Restored on failure and on exception.
  reset();
  g_service_result = MOD_ERROR;
  CHECK(es_create_in_link_layer(g_link, fpcNm_E_DN_e, p, id) == MOD_ERROR && g_current == &g_root);
  reset();
  g_service_throws = true;
  bool thrown = false;
  try {
    (void)es_create_in_link_layer(g_link, fpcNm_E_DN_e, p, id);
  } catch (const std::exception&) {
    thrown = true;
  }
  CHECK(thrown && g_current == &g_root && g_requests.empty());
  // Never on a root, missing or dying layer, never during a stage change; nothing is requested and nothing is changed.
  for (int k = 0; k < 4; ++k) {
    reset();
    if (k == 0) g_link->layer_tag.layer = &g_root;
    if (k == 1) g_link->layer_tag.layer = nullptr;
    if (k == 2) g_play.deleting = true;
    if (k == 3) g_next_stage = true;
    CHECK(es_create_in_link_layer(g_link, fpcNm_E_DN_e, p, id) == MOD_UNAVAILABLE && g_requests.empty() && g_current == &g_root);
  }
  CHECK(es_create_in_link_layer(nullptr, fpcNm_E_DN_e, p, id) == MOD_UNAVAILABLE);
}

static void lab_press_to_actor() {
  reset();
  es_press(index_of("Lizalfos"));
  CHECK(g_requests.empty());  // the UI never touches the engine
  es_update();
  CHECK(g_requests.size() == 1 && g_requests[0].owner == &g_play && g_current == &g_root);
  const Request r = g_requests[0];
  CHECK(r.profile == fpcNm_E_DN_e && r.p.parameters == 0xff0000ffu && r.p.room_num == 3);
  CHECK(std::fabs(r.p.position.x - (100.0f + 300.0f)) < 0.5f && std::fabs(r.p.position.z - (-20.0f)) < 0.5f && r.p.position.y == 50.0f);  // 300 in front, on the ground
  CHECK(r.p.angle.y == static_cast<s16>(0x4000 + 0x8000));  // facing Link
  CHECK(toasted("solicitud enviada") && !toasted("ha aparecido"));
  engine_create_all(false);  // in the actor queue, still creating
  es_update();
  CHECK(!toasted("ha aparecido"));
  for (auto& l : g_live) l.creating = false;
  es_update();
  CHECK(toasted("ha aparecido") && logged("event=ACTOR_APPEARED"));
  delete_layer(&g_play);
  es_update();
  CHECK(logged("event=ACTOR_GONE") && esr.sp.live() == 0);
  // Flying enemies are lifted; a fallback distance is used when the first point is refused.
  reset();
  es_press(index_of("Keese"));
  es_update();
  CHECK(g_requests.size() == 1 && g_requests[0].p.position.y == 250.0f && g_requests[0].p.parameters == 0xffff0f01u);
}

static void lab_refusals() {
  reset();
  g_ground.water = true, g_ground.water_y = 90;
  es_press(index_of("Lizalfos"));
  es_update();
  CHECK(g_requests.empty() && toasted("no hay suelo seguro") && g_ground_calls >= 3);
  reset();
  g_ground.found = false;
  es_press(index_of("Lizalfos"));
  es_update();
  CHECK(g_requests.empty() && logged("why=NO_GROUND"));
  // Unsafe moments: the press waits, then expires; nothing is created meanwhile.
  reset();
  g_event = true;
  es_press(index_of("Lizalfos"));
  for (std::uint32_t k = 0; k <= es::policy::kPendingTicks + 1; ++k) es_update();
  CHECK(g_requests.empty() && toasted("estado seguro") && !esr.sp.pending());
  reset();
  g_pause = true;
  es_press(index_of("Lizalfos"));
  es_update();
  CHECK(g_requests.empty() && esr.sp.pending());
  g_pause = false;
  es_update();
  CHECK(g_requests.size() == 1);
  reset();
  g_link = nullptr;
  es_press(index_of("Lizalfos"));
  es_update();
  CHECK(g_requests.empty() && esr.sp.pending());
  // Black: a refusal with its reason, whatever is loaded. No request, ever.
  for (int fw = 0; fw <= 1; ++fw) {
    reset();
    g_framework = fw != 0;
    es_update();
    es_press(index_of("Bokoblin Black (Expansion): integracion no disponible"));
    es_update();
    CHECK(g_requests.empty() && toasted("Black: integracion no disponible"));
  }
  // Out of range selections from a damaged config value.
  reset();
  es_press(-3);
  es_press(9999);
  es_update();
  CHECK(g_requests.empty() && g_toasts.size() == 2);
}

static void lab_expansion_observation() {
  // Expansion loaded: the framework service is present, and Expansion's own classification writes 220 / 330 at the first Execute.
  for (int red = 1; red >= 0; --red) {
    for (int works = 1; works >= 0; --works) {
      reset();
      g_framework = true;
      es_update();
      CHECK(esr.sp.expansion == es::Expansion::Possible);
      es_press(index_of(red ? "Bokoblin Red (Expansion)" : "Bokoblin Blue (Expansion)"));
      es_update();
      CHECK(g_requests.size() == 1 && g_requests[0].profile == fpcNm_E_OC_e && ((g_requests[0].p.angle.x >> 8) & 0xff) == (red ? 1 : 0));
      engine_create_all();
      auto* b = static_cast<daE_OC_c*>(g_made);
      CHECK(!std::strcmp(b->mName, red ? "E_OC2" : "E_OC") && b->health == (red ? 220 : 40));
      if (works) b->health = red ? 330 : 220;  // what Expansion 0.17.0 writes for a native E_OC2 / E_OC (variant_policy.hpp)
      for (std::uint32_t k = 0; k < es::policy::kVerifyTicks + 3; ++k) es_update();
      if (works) {
        CHECK(esr.sp.expansion == es::Expansion::Confirmed && toasted(red ? "clasifico como RED" : "clasifico como BLUE") && b->health == (red ? 330 : 220));
      } else {
        CHECK(esr.sp.expansion == es::Expansion::Refuted && toasted("no lo clasifico") && b->health == (red ? 220 : 40));  // never written by the spawner
      }
    }
  }
  // Without the framework: refused, nothing created.
  reset();
  es_update();
  es_press(index_of("Bokoblin Blue (Expansion)"));
  es_update();
  CHECK(g_requests.empty() && toasted("no esta cargado"));
  // Native Bokoblin entries stay available and honest.
  reset();
  es_press(index_of("Bokoblin (E_OC, nativo)"));
  es_update();
  CHECK(g_requests.size() == 1 && ((g_requests[0].p.angle.x >> 8) & 0xff) == 0);
}

static void lab_adaptations() {
  reset();
  es_press(index_of("Water Toadpoli"));
  es_update();
  engine_create_all();
  auto* tk = static_cast<e_tk_class*>(g_made);
  // An actor of the same profile that this mod did not create: never written.
  e_tk_class foreign;
  foreign.name = fpcNm_E_TK_e, foreign.id = 9999, foreign.mExecuteState = 1;
  g_live.push_back({&foreign, &g_play, false});
  for (int k = 0; k < 40; ++k) {
    if (foreign.mExecuteState > 0) --foreign.mExecuteState;
    if (tk->mExecuteState > 0) --tk->mExecuteState;  // the native countdown of its Execute
    es_update();
  }
  CHECK(tk->mExecuteState == 2 && foreign.mExecuteState == 0 && esr.adapt_writes > 0);
  g_live.pop_back();
  // An id of ours that now names another profile: nothing is written.
  reset();
  es_press(index_of("Fire Toadpoli"));
  es_update();
  g_requests[0].profile = fpcNm_E_DN_e;
  engine_create_all();
  es_update();
  CHECK(esr.adapt_writes == 0);
}

static void lab_limit_and_shutdown() {
  reset();
  for (unsigned i = 0; i < es::policy::kMaxLive; ++i) {
    es_press(index_of("Rat"));
    es_update();
  }
  CHECK(g_requests.size() == es::policy::kMaxLive);
  es_press(index_of("Rat"));
  es_update();
  CHECK(g_requests.size() == es::policy::kMaxLive && toasted("Limite"));
  engine_create_all();
  es_update();
  const std::size_t live = g_live.size();
  esr.sp.forget_all();
  es_update();
  CHECK(g_live.size() == live && esr.sp.live() == 0);  // forgetting deletes nothing: the actors belong to their scene
}

int main() {
  lab_ownership();
  lab_press_to_actor();
  lab_refusals();
  lab_expansion_observation();
  lab_adaptations();
  lab_limit_and_shutdown();
  reset();
  std::printf("ES_ADAPTER_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_ADAPTER_OVER_ENGINE_LAYER_MODEL runtime=HUMAN_RUNTIME_PENDING\n", es::kVersion, g_checks);
  return 0;
}
