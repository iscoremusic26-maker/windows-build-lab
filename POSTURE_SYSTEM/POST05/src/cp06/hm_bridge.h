// BARS203 HM-integration -- optional consumer of the Enemy Health Multiplier vitals service (contract: hm_vitals_api.h v1.0).
// Game-type free on purpose (testable on its own): the adapter passes the actor's copied identity and native health; this layer asks the provider, VALIDATES the answer and returns the
// HealthView the tracker consumes. It is a pure reader: the only call it ever makes into HM is get_vitals(), which cannot change anything, and any doubt yields the native view.
#pragma once
#include <stdint.h>
#include "hm_vitals_api.h"
#include "tracker.h"
#include "reused/normal_model.h"
#include "tektite_view.h"
#include "armos_view.h"

namespace ehz {
// Why an answer was not used (for the bounded statistics). Expected native answers are not errors.
enum HmRejectCause : unsigned {
    HMR_NONE = 0, HMR_STATUS_UNKNOWN, HMR_ECHO_ID, HMR_ECHO_PROFILE, HMR_STRUCT_SIZE, HMR_REASON, HMR_MULTIPLIER, HMR_KIND, HMR_RANGE, HMR_MAX_RELATION, HMR_ABOVE_NATIVE, HMR_MASK_MISMATCH, HMR_TT_MISMATCH, HMR_AI_MISMATCH, HMR_COUNT
};
struct HmBridgeStats {
    bool attached = false, incompatible = false;
    uint64_t queries = 0, ok = 0, okEffective = 0, okBudget = 0, useNative = 0, unavailable = 0, invalid = 0, rejected = 0, skippedChuchu = 0;
    uint64_t nativeReason[8] = {};     // HM_VR_FACTOR_ONE.. HM_VR_NO_BUDGET (1..6)
    uint64_t unavailReason[8] = {};    // HM_VR_NOT_READY.. (16..21) -> index reason-16
    uint64_t rejectCause[HMR_COUNT] = {};
    uint64_t scaleChanges = 0;         // times the multiplier reported for an OK answer differs from the previous OK answer
    unsigned lastMultiplier = 1;
    // Tektite (TT-FIX01): counter-based view, with or without a provider
    uint64_t ttSeen = 0, ttDead = 0, ttNativeViews = 0, ttEffectiveViews = 0, ttRejected = 0, ttIdOverflow = 0;
    uint32_t ttIds[32] = {}; unsigned ttIdCount = 0;
    // Armos (CP06): counter-based view, with or without a provider
    uint64_t aiSeen = 0, aiDead = 0, aiNativeViews = 0, aiEffectiveViews = 0, aiRejected = 0, aiIdOverflow = 0, aiSteps = 0, aiSteadyRepeats = 0, aiUnreadable = 0;
    uint32_t aiIds[32] = {}, aiLast[32] = {}; unsigned aiIdCount = 0;   // per-instance last published value (only to COUNT reflected steps; never used to compute a view)
};
// Attach the imported service (null = no provider / incompatible). Called from mod_initialize; detach from mod_shutdown, before anything else is torn down.
void hm_bridge_attach(const HmVitalsService* service);
void hm_bridge_detach();
bool hm_bridge_present();
// The view of one actor: the provider's effective view when it answers OK and the answer passes every check, else {nativeHealth, 0, 1}. `actor` is a borrowed pointer used only during this call.
HealthView hm_health_view(ModContext* caller, const void* actor, uint32_t id, int profile, int nativeHealth);
// The single entry used by the normal-bar gatherer: native view unless a provider is attached AND the actor is one the tracker would follow (normal_trackable on the native sample:
// enemy-type groups only, alive, not a boss, not an ally), in which case hm_health_view() is consulted. Bosses, allies, the dead and non-enemies never cause a query.
HealthView hm_normal_view(ModContext* caller, const void* actor, uint32_t id, int profile, int group, int nativeHealth, uint32_t status, uint32_t condition, uint32_t attention, bool senses, bool boss, bool allied);
// Tektite entry (TT-FIX01): the bar of a Tektite follows its HIT COUNTER, never `health` (see tektite_view.h). counter/health/action are the actor's copied fields. Defeated actors return {0,0,1} without
// any query; alive ones return the native counter view {40-counter, 40, 1}, or -- with a provider attached that answers OK and passes the Tektite validation (BUDGET, maximum == 40*M, current within
// one unit of (40-counter)*M) -- the provider's effective view. Same trackable filter as hm_normal_view.
HealthView hm_tektite_view(ModContext* caller, const void* actor, uint32_t id, int counter, int health, int action, int group, uint32_t status, uint32_t condition, uint32_t attention, bool senses, bool boss, bool allied);
// Armos entry (CP06): the bar of an Armos follows its HIT COUNTER, never `health` (the 1000 decoy; see armos_view.h). count is the actor's copied m_hitCount. Defeated / unreadable counters return {0,0,1} with no
// query; alive ones return the native counter view {3-count, 3, 1}, or -- with a provider attached that answers OK and passes the Armos validation (BUDGET, maximum == 3*M, current within one unit of
// (3-count)*M) -- the provider's effective view. Same trackable filter as hm_normal_view.
HealthView hm_armos_view(ModContext* caller, const void* actor, uint32_t id, int count, int group, uint32_t status, uint32_t condition, uint32_t attention, bool senses, bool boss, bool allied);
// Pure validation of an OK answer (no provider needed): exposed for the lab. Returns HMR_NONE when acceptable and fills `view`.
HmRejectCause hm_validate(const HmVitals& v, uint32_t id, int profile, int nativeHealth, HealthView* view);
const HmBridgeStats& hm_bridge_stats();
// One-shot descriptions of UNEXPECTED provider behaviour (unavailable / invalid / rejected), each distinct class at most once per attach: the caller logs it. nullptr when nothing new.
const char* hm_bridge_next_warning();
}  // namespace ehz
