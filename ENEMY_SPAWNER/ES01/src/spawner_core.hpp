#pragma once
// Enemy Spawner (Standalone) 0.1.1: the decisions of the spawner. PURE: no game header. mod.cpp reads the engine into these
// small structs, calls the core and acts on its answer. tests/spawner_lab.cpp drives the same core.
//
// The only job of this mod is to CREATE actors. What happens afterwards (AI, HP, parry, posture, effects, death, deletion) belongs to
// the engine and to the mods that own those systems. The core therefore never decides anything about an actor after its creation,
// except the two Toadpoli adaptations of the catalog and reading, without writing, whether Bokoblin Expansion classified a Bokoblin.
#include <cstddef>
#include <cstdint>
#include "catalog.hpp"

namespace es {
inline constexpr const char* kVersion = "0.1.1";

namespace policy {
inline constexpr unsigned kMaxLive = 12;                 // live actors created by this mod (requests included); never frees capacity by deleting
inline constexpr std::uint32_t kPendingTicks = 300;      // a press waits at most 10 s (30 ticks/s) for a safe moment, then expires
inline constexpr std::uint32_t kCreateTicks = 600;       // a creation request that neither appears nor fails in 20 s is reported and forgotten
inline constexpr std::uint32_t kVerifyTicks = 30;        // Expansion classifies on the first Execute; 1 s later the answer is final
inline constexpr float kDistances[] = {300.0f, 220.0f, 160.0f};  // in front of Link (Dawnlight: 300)
inline constexpr float kProbeAbove = 500.0f;             // ground probe starts this far above the point (Dawnlight)
inline constexpr float kMaxStep = 150.0f;                // ground no further than this above or below Link's feet
inline constexpr float kMinNormalY = 0.70f;              // slopes steeper than about 45 degrees are refused
inline constexpr float kFlyingLift = 200.0f;             // Dawnlight's lift for flying enemies
inline constexpr float kSightHeight = 60.0f;             // the line from Link to the point is tested this high above both
}  // namespace policy

// ---------------------------------------------------------------- where to put it
struct GroundSample {
  bool found = false;      // GroundCross hit a polygon
  float ground_y = 0;      // its height
  float link_y = 0;        // Link's feet
  float normal_y = 1;      // the polygon's normal
  bool water = false;      // a water or lava surface above that ground at this point
  float water_y = 0;
  bool blocked = false;    // a wall between Link and the point
};
enum class Ground : std::uint8_t { Ok, NoGround, TooHigh, TooLow, Slope, Water, Blocked, Count };
inline const char* name(Ground g) noexcept {
  static constexpr const char* n[] = {"OK", "NO_GROUND", "GROUND_TOO_HIGH", "GROUND_TOO_LOW_OR_EDGE", "SLOPE", "WATER_OR_LAVA", "WALL_BETWEEN"};
  return unsigned(g) < unsigned(Ground::Count) ? n[unsigned(g)] : "?";
}
inline Ground judge_ground(const GroundSample& s) noexcept {
  if (!s.found) return Ground::NoGround;
  if (s.blocked) return Ground::Blocked;
  if (s.ground_y > s.link_y + policy::kMaxStep) return Ground::TooHigh;
  if (s.ground_y < s.link_y - policy::kMaxStep) return Ground::TooLow;
  if (!(s.normal_y >= policy::kMinNormalY)) return Ground::Slope;
  if (s.water && s.water_y > s.ground_y) return Ground::Water;
  return Ground::Ok;
}

// ---------------------------------------------------------------- when it is safe to create
struct Scene {
  bool link = false;         // daAlink_getAlinkActorClass() exists
  bool link_layer = false;   // Link's owner layer exists, is not the root layer and is not being deleted
  bool next_stage = false;   // a stage change is requested
  bool event = false;        // a cutscene or event runs
  bool paused = false;       // the pause menu
  bool actor_service = false;
};
enum class Wait : std::uint8_t { Ready, NoService, NoLink, BadLayer, Transition, Event, Paused, Count };
inline const char* name(Wait w) noexcept {
  static constexpr const char* n[] = {"READY", "ACTOR_SERVICE_ABSENT", "NO_LINK", "LINK_LAYER_INVALID", "SCENE_TRANSITION", "EVENT", "PAUSED"};
  return unsigned(w) < unsigned(Wait::Count) ? n[unsigned(w)] : "?";
}
inline Wait judge_scene(const Scene& s) noexcept {
  if (!s.actor_service) return Wait::NoService;
  if (!s.link) return Wait::NoLink;
  if (!s.link_layer) return Wait::BadLayer;
  if (s.next_stage) return Wait::Transition;
  if (s.event) return Wait::Event;
  if (s.paused) return Wait::Paused;
  return Wait::Ready;
}

// ---------------------------------------------------------------- Bokoblin Expansion, observed and never written
// Its classification of a fresh Bokoblin (Expansion 0.17.0, variant_policy.hpp make_plan): native E_OC 40 -> BLUE 220 HP, native E_OC2 220 -> RED 330 HP.
// It writes `health` only; the native creation value field_0x560 stays. The spawner only READS both.
enum class Expansion : std::uint8_t { Absent, Possible, Confirmed, Refuted };  // Absent: the framework it requires is not loaded
inline const char* name(Expansion e) noexcept {
  static constexpr const char* n[] = {"ABSENT", "POSSIBLE_NOT_YET_OBSERVED", "CONFIRMED_BY_OBSERVATION", "NOT_OBSERVED_ON_A_FRESH_BOKOBLIN"};
  return unsigned(e) <= 3 ? n[unsigned(e)] : "?";
}
enum class Variant : std::uint8_t { None, Blue, Red, Native, Unexpected };
inline Variant classify(bool e_oc2, int field_0x560, int health) noexcept {
  if (!e_oc2 && field_0x560 == 40) return health == 220 ? Variant::Blue : health == 40 ? Variant::Native : Variant::Unexpected;
  if (e_oc2 && field_0x560 == 220) return health == 330 ? Variant::Red : health == 220 ? Variant::Native : Variant::Unexpected;
  return Variant::Unexpected;  // another mod changed the native creation value, or the actor is not what was asked
}

// ---------------------------------------------------------------- the presses and the actors
enum class Press : std::uint8_t { Accepted, InvalidIndex, Busy, Full, BlackUnavailable, ExpansionAbsent, ExpansionRefuted, Count };
inline const char* name(Press p) noexcept {
  static constexpr const char* n[] = {"ACCEPTED", "INVALID_INDEX", "REQUEST_ALREADY_PENDING", "LIMIT_REACHED", "BLACK_INTEGRATION_UNAVAILABLE", "EXPANSION_NOT_LOADED", "EXPANSION_NOT_OBSERVED"};
  return unsigned(p) < unsigned(Press::Count) ? n[unsigned(p)] : "?";
}
enum class Ev : std::uint8_t {
  None, Requested, Expired, Rejected, Created, CreateFailed, Appeared, Vanished, CreateTimeout, Classified, NotClassified, IdReused, Count
};
inline const char* name(Ev e) noexcept {
  static constexpr const char* n[] = {"NONE", "REQUEST_SENT", "PRESS_EXPIRED", "SPAWN_REJECTED", "CREATE_REQUEST_ACCEPTED", "CREATE_REQUEST_FAILED", "ACTOR_APPEARED",
                                      "ACTOR_GONE", "CREATION_TIMEOUT", "EXPANSION_CLASSIFIED", "EXPANSION_DID_NOT_CLASSIFY", "ID_REUSED"};
  return unsigned(e) < unsigned(Ev::Count) ? n[unsigned(e)] : "?";
}
struct Event {
  Ev kind = Ev::None;
  std::uint16_t entry = 0;
  std::uint32_t id = 0;
  std::uint8_t why = 0;    // Wait or Ground or Variant, by kind
  std::uint64_t tick = 0;
};
// What mod_update learned about one tracked id this tick (from the engine, read only).
struct Probe {
  bool found = false;      // fopAcM_SearchByID found the process in the actor queue
  bool creating = false;   // fpcM_IsCreating(id)
  bool bokoblin = false;   // found, profile E_OC: the two values below are valid
  bool e_oc2 = false;      // its native resource name is "E_OC2"
  int field_0x560 = 0, health = 0;
};
enum class Life : std::uint8_t { Free, Creating, Alive };
struct Tracked {
  Life life = Life::Free;
  std::uint32_t id = 0;
  std::uint16_t entry = 0;
  std::uint64_t since = 0, generation = 0;
  bool verify = false;     // an Expansion route still waiting for its answer
  std::uint64_t alive_tick = 0;
};

class Spawner {
 public:
  static constexpr unsigned kEvents = 16;
  Expansion expansion = Expansion::Absent;
  Tracked t[policy::kMaxLive];
  struct Counters {
    std::uint64_t presses = 0, accepted = 0, refused = 0, expired = 0, rejected = 0, created = 0, failed = 0, appeared = 0, gone = 0, timeouts = 0, classified = 0, not_classified = 0, id_reused = 0, events_dropped = 0;
  } c;

  void set_framework_present(bool present) noexcept {
    if (!present) expansion = Expansion::Absent;
    else if (expansion == Expansion::Absent) expansion = Expansion::Possible;
  }
  unsigned live() const noexcept {
    unsigned n = pending_ ? 1u : 0u;
    for (const auto& x : t) n += x.life != Life::Free;
    return n;
  }
  bool pending() const noexcept { return pending_; }
  std::uint16_t pending_entry() const noexcept { return pending_entry_; }

  // UI button. Nothing touches the engine here: the request is taken by the next mod_update, at the start of a frame.
  Press press(std::int64_t index, std::uint64_t now) noexcept {
    ++c.presses;
    const Press r = check(index);
    if (r != Press::Accepted) {
      ++c.refused;
      return r;
    }
    ++c.accepted;
    pending_ = true, pending_entry_ = static_cast<std::uint16_t>(index), pending_since_ = now;
    return r;
  }
  // mod_update, each frame: is the waiting press to be created now? (`Wait` explains a no)
  Wait ready(const Scene& s, std::uint64_t now) noexcept {
    if (!pending_) return Wait::Ready;
    const Wait w = judge_scene(s);
    if (w != Wait::Ready && now - pending_since_ > policy::kPendingTicks) {
      pending_ = false;
      ++c.expired;
      emit(Ev::Expired, pending_entry_, 0, std::uint8_t(w), now);
    }
    return w;
  }
  // No valid ground at any distance: the press ends here, nothing was created.
  void reject(Ground g, std::uint64_t now) noexcept {
    if (!pending_) return;
    pending_ = false;
    ++c.rejected;
    emit(Ev::Rejected, pending_entry_, 0, std::uint8_t(g), now);
  }
  // The ActorService answer. ok with a real id: tracked from now on (its process exists before it is an actor).
  void created(bool ok, std::uint32_t id, std::uint64_t now) noexcept {
    if (!pending_) return;
    pending_ = false;
    if (!ok || id == 0 || id == 0xFFFFFFFFu) {
      ++c.failed;
      emit(Ev::CreateFailed, pending_entry_, id, 0, now);
      return;
    }
    for (auto& x : t)
      if (x.life != Life::Free && x.id == id) {  // the engine never reuses a live id; a stale entry of ours is forgotten, counted
        ++c.id_reused;
        emit(Ev::IdReused, x.entry, id, 0, now);
        x = Tracked{};
      }
    for (auto& x : t)
      if (x.life == Life::Free) {
        x = Tracked{};
        x.life = Life::Creating, x.id = id, x.entry = pending_entry_, x.since = now, x.generation = ++generation_;
        x.verify = kCatalog[pending_entry_].route == Route::ExpansionBlue || kCatalog[pending_entry_].route == Route::ExpansionRed ||
                   (kCatalog[pending_entry_].profile == Profile::E_OC && expansion != Expansion::Absent);
        ++c.created;
        emit(Ev::Created, pending_entry_, id, 0, now);
        return;
      }
  }
  // mod_update, each frame, for each tracked slot.
  void observe(unsigned slot, const Probe& p, std::uint64_t now) noexcept {
    if (slot >= policy::kMaxLive) return;
    Tracked& x = t[slot];
    if (x.life == Life::Free) return;
    if (x.life == Life::Creating) {
      if (p.found && !p.creating) {
        x.life = Life::Alive, x.alive_tick = now;
        ++c.appeared;
        emit(Ev::Appeared, x.entry, x.id, 0, now);
      } else if (!p.found && !p.creating) {  // the request ended without an actor: refused by its own create, or its scene went away
        ++c.failed;
        emit(Ev::CreateFailed, x.entry, x.id, 1, now);
        x = Tracked{};
        return;
      } else if (now - x.since > policy::kCreateTicks) {
        ++c.timeouts;
        emit(Ev::CreateTimeout, x.entry, x.id, 0, now);
        x = Tracked{};
        return;
      }
    }
    if (x.life == Life::Alive) {
      if (!p.found) {  // death, deletion or the scene it belonged to: the engine's business, we only forget the id
        ++c.gone;
        emit(Ev::Vanished, x.entry, x.id, 0, now);
        x = Tracked{};
        return;
      }
      if (x.verify && p.bokoblin && now - x.alive_tick >= 1) {
        const Variant v = classify(p.e_oc2, p.field_0x560, p.health);
        const bool wanted_blue = kCatalog[x.entry].route == Route::ExpansionBlue, wanted_red = kCatalog[x.entry].route == Route::ExpansionRed;
        if (v == Variant::Blue || v == Variant::Red) {
          x.verify = false;
          expansion = Expansion::Confirmed;
          ++c.classified;
          emit(Ev::Classified, x.entry, x.id, std::uint8_t(v), now);
        } else if (now - x.alive_tick >= policy::kVerifyTicks) {
          x.verify = false;
          if ((wanted_blue || wanted_red) && expansion != Expansion::Confirmed) expansion = Expansion::Refuted;
          ++c.not_classified;
          emit(Ev::NotClassified, x.entry, x.id, std::uint8_t(v), now);
        }
      } else if (x.verify && !p.bokoblin) {
        x.verify = false;
      }
    }
  }
  // Scene teardown or mod shutdown: ids of a gone scene are never looked up again.
  void forget_all() noexcept {
    for (auto& x : t) x = Tracked{};
    pending_ = false;
  }
  // Events for the log and the toasts, drained by mod_update.
  unsigned drain(Event* out, unsigned cap) noexcept {
    const unsigned n = nev_ < cap ? nev_ : cap;
    for (unsigned i = 0; i < n; ++i) out[i] = ev_[i];
    nev_ = 0;
    return n;
  }

 private:
  Press check(std::int64_t index) const noexcept {
    if (index < 0 || static_cast<std::uint64_t>(index) >= kCatalogSize) return Press::InvalidIndex;
    const Entry& e = kCatalog[index];
    if (e.route == Route::BlackUnavailable) return Press::BlackUnavailable;  // never a Red with a black label
    if ((e.route == Route::ExpansionBlue || e.route == Route::ExpansionRed) && expansion == Expansion::Absent) return Press::ExpansionAbsent;
    if ((e.route == Route::ExpansionBlue || e.route == Route::ExpansionRed) && expansion == Expansion::Refuted) return Press::ExpansionRefuted;
    if (pending_) return Press::Busy;
    if (live() >= policy::kMaxLive) return Press::Full;
    return Press::Accepted;
  }
  void emit(Ev k, std::uint16_t entry, std::uint32_t id, std::uint8_t why, std::uint64_t now) noexcept {
    if (nev_ >= kEvents) {
      ++c.events_dropped;
      return;
    }
    ev_[nev_++] = Event{k, entry, id, why, now};
  }
  bool pending_ = false;
  std::uint16_t pending_entry_ = 0;
  std::uint64_t pending_since_ = 0, generation_ = 0;
  Event ev_[kEvents];
  unsigned nev_ = 0;
};

}  // namespace es
