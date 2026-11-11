// Enemy Spawner (Standalone) 0.1.1: laboratory of the pure core (catalog, presses, scene and ground rules, tracking, Expansion observation).
// Also checks the spawner's reading of Bokoblin Expansion 0.17.0 against Expansion's own identity code (vendored, unmodified).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/spawner_core.hpp"
#include "vendor/bokoblin_expansion_0_17_0/variant_policy.hpp"

static unsigned long long g_checks = 0;
#define CHECK(x)                                              \
  do {                                                        \
    ++g_checks;                                               \
    if (!(x)) {                                               \
      std::printf("ES_LAB_FAIL line=%d %s\n", __LINE__, #x);   \
      std::exit(1);                                           \
    }                                                         \
  } while (0)

using namespace es;

static int index_of(const char* label) {
  for (std::size_t i = 0; i < kCatalogSize; ++i)
    if (!std::strcmp(kCatalog[i].label, label)) return int(i);
  return -1;
}
static Scene good_scene() {
  Scene s;
  s.link = s.link_layer = s.actor_service = true;
  return s;
}

static void lab_catalog() {
  CHECK(kCatalogSize == 45);
  unsigned black = 0, blue = 0, red = 0;
  for (std::size_t i = 0; i < kCatalogSize; ++i) {
    const Entry& e = kCatalog[i];
    CHECK(e.label && e.label[0]);
    CHECK(unsigned(e.profile) < unsigned(Profile::Count));
    for (std::size_t j = 0; j < i; ++j) CHECK(std::strcmp(kCatalog[j].label, e.label) != 0);  // a label names one entry
    CHECK(kLabels.v[i] == e.label);
    black += e.route == Route::BlackUnavailable, blue += e.route == Route::ExpansionBlue, red += e.route == Route::ExpansionRed;
    if (e.route != Route::Native) CHECK(e.profile == Profile::E_OC);
    // E_OC's subtype lives in the high byte of angle.x: 0 -> E_OC, else E_OC2.
    if (e.route == Route::ExpansionBlue) CHECK((e.angle_x >> 8) == 0);
    if (e.route == Route::ExpansionRed || e.route == Route::BlackUnavailable) CHECK((e.angle_x >> 8) != 0 && ((e.angle_x >> 8) & 0xff) != 0xff);
    if (e.adapt == Adapt::ToadpoliWater) CHECK(e.profile == Profile::E_TK);
    if (e.adapt == Adapt::ToadpoliLava) CHECK(e.profile == Profile::E_TK2);
  }
  CHECK(black == 1 && blue == 1 && red == 1);
  CHECK(std::strstr(kCatalog[index_of("Bokoblin Black (Expansion): integration unavailable")].label, "integration unavailable") != nullptr);
  CHECK(index_of("Goron") < 0 && index_of("Staltroop") < 0);
  CHECK(index_of("Lizalfos") == 6);  // the default of the persisted "enemy" index  // an NPC and an arena-owned enemy are not offered
  // Lizalfos / Dynalfos: low byte 0xFF (native patrol), never 0 (ACTION_STAY); the defeated switch byte stays the 0xFF sentinel.
  CHECK((kCatalog[index_of("Lizalfos")].parameters & 0xff) == 0xff && (kCatalog[index_of("Lizalfos")].parameters >> 24) == 0xff);
  CHECK((kCatalog[index_of("Dynalfos")].parameters & 0xff) == 0xff);
  CHECK(kCatalog[index_of("Helmasaurus")].argument == 1 && kCatalog[index_of("Helmasaur")].argument == -1);
  CHECK(kCatalog[index_of("Stalfos")].angle_z == 0xff && kCatalog[index_of("Kargarok")].angle_z == 0xff);
}

static void lab_scene_and_ground() {
  CHECK(judge_scene(good_scene()) == Wait::Ready);
  Scene s = good_scene();
  s.actor_service = false;
  CHECK(judge_scene(s) == Wait::NoService);
  s = good_scene(), s.link = false;
  CHECK(judge_scene(s) == Wait::NoLink);
  s = good_scene(), s.link_layer = false;
  CHECK(judge_scene(s) == Wait::BadLayer);
  s = good_scene(), s.next_stage = true;
  CHECK(judge_scene(s) == Wait::Transition);
  s = good_scene(), s.event = true;
  CHECK(judge_scene(s) == Wait::Event);
  s = good_scene(), s.paused = true;
  CHECK(judge_scene(s) == Wait::Paused);
  GroundSample g;
  CHECK(judge_ground(g) == Ground::NoGround);
  g.found = true, g.ground_y = 10, g.link_y = 0;
  CHECK(judge_ground(g) == Ground::Ok);
  g.ground_y = policy::kMaxStep + 1;
  CHECK(judge_ground(g) == Ground::TooHigh);
  g.ground_y = -policy::kMaxStep - 1;
  CHECK(judge_ground(g) == Ground::TooLow);
  g.ground_y = 0, g.normal_y = 0.5f;
  CHECK(judge_ground(g) == Ground::Slope);
  g.normal_y = 1, g.water = true, g.water_y = 30;
  CHECK(judge_ground(g) == Ground::Water);
  g.water_y = -5;
  CHECK(judge_ground(g) == Ground::Ok);
  g.blocked = true;
  CHECK(judge_ground(g) == Ground::Blocked);
}

static void lab_presses() {
  Spawner sp;
  CHECK(sp.press(-1, 1) == Press::InvalidIndex && sp.press(std::int64_t(kCatalogSize), 1) == Press::InvalidIndex && sp.press(1LL << 40, 1) == Press::InvalidIndex);
  // Black is never creatable, with or without Expansion, confirmed or not.
  const int b = index_of("Bokoblin Black (Expansion): integration unavailable");
  for (Expansion x : {Expansion::Absent, Expansion::Possible, Expansion::Confirmed, Expansion::Refuted}) {
    sp.expansion = x;
    CHECK(sp.press(b, 1) == Press::BlackUnavailable && !sp.pending());
  }
  // Expansion entries need the framework; a refutation closes them for the session.
  const int blue = index_of("Bokoblin Blue (Expansion)"), red = index_of("Bokoblin Red (Expansion)");
  sp.expansion = Expansion::Absent;
  CHECK(sp.press(blue, 1) == Press::ExpansionAbsent && sp.press(red, 1) == Press::ExpansionAbsent && !sp.pending());
  sp.expansion = Expansion::Refuted;
  CHECK(sp.press(blue, 1) == Press::ExpansionRefuted);
  sp.expansion = Expansion::Possible;
  CHECK(sp.press(blue, 1) == Press::Accepted && sp.pending());
  CHECK(sp.press(index_of("Lizalfos"), 1) == Press::Busy);  // double click: one request
  // A press waits for a safe frame, then expires.
  Scene paused = good_scene();
  paused.paused = true;
  CHECK(sp.ready(paused, 2) == Wait::Paused && sp.pending());
  CHECK(sp.ready(paused, 2 + policy::kPendingTicks) == Wait::Paused && !sp.pending());
  Event ev[Spawner::kEvents];
  unsigned n = sp.drain(ev, Spawner::kEvents);
  CHECK(n == 1 && ev[0].kind == Ev::Expired && ev[0].why == std::uint8_t(Wait::Paused));
  // No safe ground: nothing created, the press ends.
  CHECK(sp.press(index_of("Lizalfos"), 10) == Press::Accepted);
  CHECK(sp.ready(good_scene(), 11) == Wait::Ready);
  sp.reject(Ground::Water, 11);
  n = sp.drain(ev, Spawner::kEvents);
  CHECK(!sp.pending() && n == 1 && ev[0].kind == Ev::Rejected && sp.live() == 0);
  // A refused service call is not tracked.
  CHECK(sp.press(index_of("Lizalfos"), 12) == Press::Accepted);
  sp.created(false, 0xFFFFFFFFu, 12);
  n = sp.drain(ev, Spawner::kEvents);
  CHECK(sp.live() == 0 && n == 1 && ev[0].kind == Ev::CreateFailed);
  // The limit counts requests and live actors; it never frees capacity.
  for (unsigned i = 0; i < policy::kMaxLive; ++i) {
    CHECK(sp.press(index_of("Keese"), 20 + i) == Press::Accepted);
    sp.created(true, 1000 + i, 20 + i);
  }
  CHECK(sp.live() == policy::kMaxLive && sp.press(index_of("Keese"), 40) == Press::Full);
}

static void lab_lifecycle() {
  Spawner sp;
  Event ev[Spawner::kEvents];
  CHECK(sp.press(index_of("Lizalfos"), 1) == Press::Accepted);
  sp.created(true, 77, 1);
  CHECK(sp.t[0].life == Life::Creating && sp.t[0].id == 77);
  Probe p;
  p.creating = true;
  sp.observe(0, p, 2);
  CHECK(sp.t[0].life == Life::Creating);
  p.found = true;  // in the actor queue while its create still runs: not visible yet
  sp.observe(0, p, 3);
  CHECK(sp.t[0].life == Life::Creating);
  p.creating = false;
  sp.observe(0, p, 4);
  CHECK(sp.t[0].life == Life::Alive);
  p.found = false;  // death, deletion or scene teardown
  sp.observe(0, p, 5);
  CHECK(sp.t[0].life == Life::Free);
  unsigned n = sp.drain(ev, Spawner::kEvents);
  CHECK(n == 3 && ev[0].kind == Ev::Created && ev[1].kind == Ev::Appeared && ev[2].kind == Ev::Vanished);
  // A request that ends without an actor (its own create refused it, or its scene went away).
  CHECK(sp.press(index_of("Lizalfos"), 10) == Press::Accepted);
  sp.created(true, 78, 10);
  Probe none;
  sp.observe(0, none, 11);
  n = sp.drain(ev, Spawner::kEvents);
  CHECK(sp.t[0].life == Life::Free && ev[n - 1].kind == Ev::CreateFailed);
  // A creation that never ends is reported and forgotten.
  CHECK(sp.press(index_of("Lizalfos"), 20) == Press::Accepted);
  sp.created(true, 79, 20);
  Probe stuck;
  stuck.creating = true;
  sp.observe(0, stuck, 20 + policy::kCreateTicks + 1);
  n = sp.drain(ev, Spawner::kEvents);
  CHECK(sp.t[0].life == Life::Free && ev[n - 1].kind == Ev::CreateTimeout);
  // An id the engine hands out again is never two actors of ours.
  CHECK(sp.press(index_of("Lizalfos"), 30) == Press::Accepted);
  sp.created(true, 90, 30);
  CHECK(sp.press(index_of("Lizalfos"), 31) == Press::Accepted);
  sp.created(true, 90, 31);
  unsigned with90 = 0;
  for (const auto& x : sp.t) with90 += x.life != Life::Free && x.id == 90;
  CHECK(with90 == 1 && sp.c.id_reused == 1);
  sp.forget_all();
  CHECK(sp.live() == 0 && !sp.pending());
}

// What Expansion 0.17.0 itself decides for a fresh Bokoblin created by this mod: no DEVF registration matches it (Registration::None).
static void lab_expansion_contract() {
  namespace dv = devf_variant;
  const dv::Plan blue = dv::make_plan("E_OC", 40, 40, true, dv::Registration::None, 0);
  CHECK(blue.variant == dv::Variant::Blue && blue.initial_hp == 220 && blue.write_hp);
  const dv::Plan red = dv::make_plan("E_OC2", 220, 220, true, dv::Registration::None, 0);
  CHECK(red.variant == dv::Variant::Red && red.initial_hp == 330 && red.write_hp);
  // Black exists only through an exact registration that Expansion builds at its own initialization from its packaged encounters.
  for (int hp = 1; hp <= 999; hp += 37) CHECK(dv::make_plan("E_OC2", 220, 220, true, dv::Registration::None, hp).variant != dv::Variant::Black);
  CHECK(dv::make_plan("E_OC2", 220, 220, true, dv::Registration::Black, 440).variant == dv::Variant::Black);
  // What the spawner reads back, from the same numbers.
  CHECK(es::classify(false, 40, blue.initial_hp) == es::Variant::Blue && es::classify(true, 220, red.initial_hp) == es::Variant::Red);
  CHECK(es::classify(false, 40, 40) == es::Variant::Native && es::classify(true, 220, 220) == es::Variant::Native);
  CHECK(es::classify(true, 220, 440) == es::Variant::Unexpected);  // a 440 HP E_OC2 is not taken for anything the spawner can name
  CHECK(es::classify(false, 80, 220) == es::Variant::Unexpected);
  CHECK(es::classify(true, 300, 330) == es::Variant::Unexpected && es::classify(false, 40, 330) == es::Variant::Unexpected);  // the creation value is part of the signature  // another mod changed the creation value: not Expansion's signature

  // Observation through the tracker.
  Spawner sp;
  Event ev[Spawner::kEvents];
  sp.set_framework_present(true);
  CHECK(sp.expansion == Expansion::Possible);
  CHECK(sp.press(index_of("Bokoblin Red (Expansion)"), 1) == Press::Accepted);
  sp.created(true, 5, 1);
  Probe p;
  p.found = true, p.bokoblin = true, p.e_oc2 = true, p.field_0x560 = 220, p.health = 220;
  sp.observe(0, p, 2);
  p.health = 330;
  sp.observe(0, p, 3);
  CHECK(sp.expansion == Expansion::Confirmed && sp.c.classified == 1);
  unsigned n = sp.drain(ev, Spawner::kEvents);
  CHECK(ev[n - 1].kind == Ev::Classified && ev[n - 1].why == std::uint8_t(es::Variant::Red));
  // Without Expansion's write, the asked route is refuted, and closes.
  Spawner s2;
  s2.set_framework_present(true);
  CHECK(s2.press(index_of("Bokoblin Blue (Expansion)"), 1) == Press::Accepted);
  s2.created(true, 6, 1);
  Probe q;
  q.found = true, q.bokoblin = true, q.e_oc2 = false, q.field_0x560 = 40, q.health = 40;
  for (std::uint64_t t = 2; t < 2 + policy::kVerifyTicks + 2; ++t) s2.observe(0, q, t);
  CHECK(s2.expansion == Expansion::Refuted && s2.c.not_classified == 1);
  CHECK(s2.press(index_of("Bokoblin Blue (Expansion)"), 50) == Press::ExpansionRefuted);
  n = s2.drain(ev, Spawner::kEvents);
  CHECK(ev[n - 1].kind == Ev::NotClassified && ev[n - 1].why == std::uint8_t(es::Variant::Native));
  // A native entry spawned while Expansion is loaded: observed, never a refutation of the Expansion entries.
  Spawner s3;
  s3.set_framework_present(true);
  CHECK(s3.press(index_of("Bokoblin (E_OC, native)"), 1) == Press::Accepted);
  s3.created(true, 7, 1);
  for (std::uint64_t t = 2; t < 2 + policy::kVerifyTicks + 2; ++t) s3.observe(0, q, t);
  CHECK(s3.expansion == Expansion::Possible);
  // The framework disappearing makes the Expansion entries unavailable again.
  s3.set_framework_present(false);
  CHECK(s3.expansion == Expansion::Absent && s3.press(index_of("Bokoblin Red (Expansion)"), 99) == Press::ExpansionAbsent);
}

int main() {
  lab_catalog();
  lab_scene_and_ground();
  lab_presses();
  lab_lifecycle();
  lab_expansion_contract();
  std::printf("ES_CORE_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PURE_CORE_AND_EXPANSION_0_17_0_IDENTITY_SOURCE runtime=HUMAN_RUNTIME_PENDING\n", kVersion, g_checks);
  return 0;
}
