#include "tracker.h"
namespace ehz {
Tracker::Entry* Tracker::admit(const Identity& who, uint32_t tick, const HealthView& v) {
    if (auto* e = find(who)) return e;
    if (entryCount == kTrackCapacity) { ++st.capacityDrops; return nullptr; }
    auto* e = &entries[entryCount++];
    e->track.reset(who, epoch, tick - 1, v.current);
    e->units = v.units;
    e->key = ++nextKey;
    if (v.maximum > 0) e->track.maximumHealth = v.maximum;
    return e;
}

void Tracker::clear(const TickEnv* env) {
    if (env && env->forget) for (unsigned i = 0; i < entryCount; ++i) env->forget(env->user, entries[i].key);
    drawCount = 0; haveTick = false;
    for (unsigned i = 0; i < entryCount; ++i) entries[i] = {};
    entryCount = 0; lastTick = ~0u; ++epoch;
}

void Tracker::tick(uint32_t tick, bool visible, bool enabled, bool senses, const ActorView* views, unsigned count, const TickEnv& env) {
    if (haveTick && lastTick == tick) return;
    lastTick = tick; haveTick = true;
    drawCount = 0;
    ++st.ticks;
    unsigned candidateCount = 0, scanned = 0;
    for (unsigned i = 0; i < entryCount; ++i) entries[i].seen = tick - 1;
    for (unsigned n = 0; n < count && scanned++ < kTrackCapacity; ++n) {
        const ActorView& a = views[n];
        NormalSample sample{a.profile, a.group, a.health.current, a.status, a.condition, a.attention, false, senses, a.boss, a.allied};
        // Match metadata against the CURRENT native queue snapshot; a saved address is never dereferenced.
        if (normal_trackable(sample)) {
            const Identity who{a.address, a.id, int16_t(a.profile)};
            if (auto* e = find(who)) {
                e->seen = tick; e->health = sample.hp;
                if (a.health.maximum > 0) e->track.maximumHealth = a.health.maximum;
                else if (e->units != a.health.units) e->track.maximumHealth = maximum(1, e->track.maximumHealth / int(maximum(1, e->units)) * int(a.health.units));
                e->units = a.health.units;
            }
        }
        if (normal_eligible(sample) && candidateCount < kDrawCapacity) { candidates[candidateCount] = a; candidateViews[candidateCount] = a.health; ++candidateCount; }
    }
    // Retire missing/reused identities BEFORE admitting new actors, so a full previous pool cannot suppress newcomers.
    unsigned kept = 0;
    for (unsigned i = 0; i < entryCount; ++i) {
        if (entries[i].seen == tick) { if (kept != i) entries[kept] = entries[i]; ++kept; }
        else { if (env.forget) env.forget(env.user, entries[i].key); ++st.retired; }
    }
    for (unsigned i = kept; i < entryCount; ++i) entries[i] = {};
    entryCount = kept;
    for (unsigned j = 0; j < candidateCount; ++j) {
        const ActorView& a = candidates[j]; const HealthView hv = candidateViews[j];
        const Identity who{a.address, a.id, int16_t(a.profile)};
        auto* e = admit(who, tick, hv);
        if (!e) continue;
        e->seen = tick; e->health = hv.current;
        if (hv.maximum > 0) e->track.maximumHealth = hv.maximum;
        e->units = hv.units;
        e->track.maximumHealth = maximum(e->track.maximumHealth, e->health);
        if (!visible || !enabled) continue;
        const NormalPoint pos{a.pos.x, a.pos.y, a.pos.z}, eye{a.eye.x, a.eye.y, a.eye.z}, attention{a.attentionPos.x, a.attentionPos.y, a.attentionPos.z};
        if (!normal_finite(pos.x) || !normal_finite(pos.y) || !normal_finite(pos.z)) continue;
        e->track.anchorHeight = stable_head_height(e->track.anchorHeight, pos, eye, attention);
        e->anchor = {pos.x, pos.y + e->track.anchorHeight + 8, pos.z};
        Vec3 out{};
        const bool projected = env.project && env.project(env.user, e->anchor, &out);
        const bool onScreen = projected && normal_on_screen({out.x - env.minX, out.y - env.minY, out.z}, env.width, env.height);
        const bool clear = onScreen ? (!env.clear_line || env.clear_line(env.user, e->anchor)) : false;
        const float distance = onScreen && a.targetable && clear && env.distance ? env.distance(env.user, a) : 4500;
        normal_step(e->track, tick, e->health, normal_opacity(onScreen, a.targetable, clear, distance), onScreen);
        if (onScreen && e->track.alpha >= .005f && drawCount < kDrawCapacity) {
            drawEntries[drawCount++] = {e->key, e->anchor, e->track.fill, e->track.trail, e->track.alpha, e->health, e->track.maximumHealth};
            if (env.record) env.record(env.user, e->key, e->anchor);
        }
    }
    // HP/fill remains current while hidden, off-camera, paused or disabled; presentation suppression publishes no bar.
    for (unsigned i = 0; i < entryCount; ++i) normal_step(entries[i].track, tick, entries[i].health, 0, false);
    st.tracked = entryCount; st.published = drawCount;
    if (entryCount > st.maxTracked) st.maxTracked = entryCount;
    if (drawCount > st.maxPublished) st.maxPublished = drawCount;
}
}  // namespace ehz
