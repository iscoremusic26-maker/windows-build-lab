// BARS203 CP02 -- per-instance tracker for NORMAL enemy bars. Pure logic: no game headers, no actor pointers.
// Algorithm ported line by line from Bars 1.1.1 health.cpp (health_tick); only the game-facing parts moved to the adapter.
#pragma once
#include <stdint.h>
#include "reused/normal_model.h"
#include "reused/normal_runtime.h"

namespace ehz {
struct Vec3 { float x, y, z; };
struct HealthView { int current, maximum; unsigned units; };

// Copied values for one actor of the native queue. `handle` is an opaque token that is only valid DURING Tracker::tick
// and only meaningful to the adapter's TickEnv callbacks; the tracker never stores it.
struct ActorView {
    uintptr_t address; uint32_t id; int profile, group;
    uint32_t status, condition, attention;
    HealthView health;
    bool boss, allied, targetable;
    Vec3 pos, eye, attentionPos;
    void* handle;
};

// What the draw packet needs; contains no actor address/pointer. `key` identifies the instance for the InterpService.
struct DrawEntry { uint64_t key; Vec3 anchor; float fill, trail, alpha; int hp, maximumHealth; };

struct TickEnv {
    void* user;
    bool (*project)(void*, const Vec3& world, Vec3* ortho);  // 2D-pass (ortho) coordinates exactly as mDoLib_project returns them; false = no view
    bool (*clear_line)(void*, const Vec3& world);
    float (*distance)(void*, const ActorView&);
    void (*record)(void*, uint64_t key, const Vec3& world);  // per-tick anchor sample for presentation interpolation
    void (*forget)(void*, uint64_t key);                      // instance retired
    float width, height;                                      // logical viewport size
    float minX, minY;                                         // ortho-space origin of the logical viewport (negative in widescreen)
};

constexpr unsigned kTrackCapacity = 4096, kDrawCapacity = 256;

class Tracker {
public:
    struct Stats { unsigned ticks = 0, tracked = 0, published = 0, maxTracked = 0, maxPublished = 0, retired = 0, capacityDrops = 0; };
    // One simulation tick. `views` is the native queue snapshot of this tick (excluding the player).
    void tick(uint32_t tick, bool visible, bool enabled, bool senses, const ActorView* views, unsigned count, const TickEnv& env);
    // Scene/player change or shutdown: drops every instance (forgets every key) and invalidates old identities.
    void clear(const TickEnv* env);
    const DrawEntry* draw_entries(unsigned* count) const { *count = drawCount; return drawEntries; }
    void clear_draw() { drawCount = 0; }
    unsigned tracked() const { return entryCount; }
    const Stats& stats() const { return st; }
    bool has_tick() const { return haveTick; }
private:
    struct Entry { HealthTrack track; Vec3 anchor; uint32_t seen; int health; unsigned units; uint64_t key; };
    Entry* find(const Identity& who) {
        for (unsigned i = 0; i < entryCount; ++i) { auto& e = entries[i]; if (e.track.epoch == epoch && same(e.track.identity, who)) return &e; }
        return nullptr;
    }
    Entry* admit(const Identity& who, uint32_t tick, const HealthView& v);
    Entry entries[kTrackCapacity]{};
    unsigned entryCount = 0;
    uint32_t epoch = 1, lastTick = ~0u;
    bool haveTick = false;
    uint64_t nextKey = 0;
    DrawEntry drawEntries[kDrawCapacity]{};
    unsigned drawCount = 0;
    HealthView candidateViews[kDrawCapacity]{};
    ActorView candidates[kDrawCapacity]{};
    Stats st{};
};

// Interpolation key: a non-canonical x64 value, so it can never equal a real object address used as a key by the game.
inline const void* interp_key(uint64_t serial) { return reinterpret_cast<const void*>(0x7BA2000000000000ull | (serial & 0xFFFFFFFFFFFFull)); }
}  // namespace ehz
