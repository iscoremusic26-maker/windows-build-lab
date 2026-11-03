#include "hm_bridge.h"
#include <stdio.h>

namespace ehz {
namespace {
const HmVitalsService* g_svc = nullptr;
HmBridgeStats g_st;
// bounded warning queue: one slot per distinct unexpected class
constexpr unsigned kWarnKinds = 40;
bool g_warned[kWarnKinds];
char g_warnText[200];
unsigned g_warnPending = kWarnKinds;  // index of a not-yet-delivered warning (kWarnKinds = none)
unsigned g_warnCount = 0;
constexpr unsigned kMaxWarnings = 12;
void note(unsigned kind, const char* what, unsigned detail) {
    if (kind >= kWarnKinds || g_warned[kind] || g_warnCount >= kMaxWarnings || g_warnPending != kWarnKinds) return;
    g_warned[kind] = true; ++g_warnCount; g_warnPending = kind;
    snprintf(g_warnText, sizeof g_warnText, "BARS203 HM bridge: unexpected provider answer (%s %u): native health is used for that actor; counted, not repeated", what, detail);
}
constexpr uint32_t kMax = 32767u * 8u;
}  // namespace

void hm_bridge_attach(const HmVitalsService* service) {
    g_svc = nullptr; g_st = HmBridgeStats{}; for (auto& w : g_warned) w = false; g_warnPending = kWarnKinds; g_warnCount = 0;
    if (service == nullptr) return;                                                   // provider absent: expected, silent
    if (!SERVICE_HAS(service, HmVitalsService, get_vitals) || service->get_vitals == nullptr || service->header.major_version != HM_VITALS_SERVICE_MAJOR) { g_st.incompatible = true; return; }
    g_svc = service; g_st.attached = true;
}
void hm_bridge_detach() { g_svc = nullptr; g_st.attached = false; }
bool hm_bridge_present() { return g_svc != nullptr; }
const HmBridgeStats& hm_bridge_stats() { return g_st; }
const char* hm_bridge_next_warning() {
    if (g_warnPending == kWarnKinds) return nullptr;
    g_warnPending = kWarnKinds;
    return g_warnText;
}

HmRejectCause hm_validate(const HmVitals& v, uint32_t id, int profile, int nativeHealth, HealthView* view) {
    if (v.struct_size != sizeof(HmVitals)) return HMR_STRUCT_SIZE;
    if (v.actor_id != id) return HMR_ECHO_ID;
    if (v.profile != (uint16_t)profile) return HMR_ECHO_PROFILE;
    if (v.reason != HM_VR_NONE) return HMR_REASON;
    if (v.multiplier < 1 || v.multiplier > 8) return HMR_MULTIPLIER;
    if (v.current > kMax || v.maximum > kMax) return HMR_RANGE;
    if (v.kind == HM_VITALS_KIND_EFFECTIVE_HP) {
        if (nativeHealth <= 0 || v.current < 1) return HMR_RANGE;
        if (v.maximum != 0 && v.current > v.maximum) return HMR_MAX_RELATION;
        // the effective life can never exceed what the native health allows at that factor (HM never creates HP): h*M
        if ((uint64_t)v.current > (uint64_t)nativeHealth * v.multiplier) return HMR_ABOVE_NATIVE;
    } else if (v.kind == HM_VITALS_KIND_BUDGET) {
        if (v.maximum == 0 || v.current > v.maximum) return HMR_MAX_RELATION;
        if (profile == 483 && (nativeHealth <= 0 || v.current != (uint32_t)nativeHealth)) return HMR_MASK_MISMATCH;   // the Zant-mask budget IS its native health
        if (profile == tt::kProfile) {   // Tektite: nativeHealth = 40 - counter (alive); the effective budget is exactly (40 - counter)*M - rem with 0 <= rem < M, maximum 40*M
            if (nativeHealth <= 0 || nativeHealth > tt::kBudget || v.maximum != (uint32_t)(tt::kBudget * v.multiplier)) return HMR_TT_MISMATCH;
            const uint32_t top = (uint32_t)nativeHealth * v.multiplier;
            if (v.current > top || v.current + v.multiplier <= top) return HMR_TT_MISMATCH;
        }
        if (profile == ai::kProfile) {   // Armos: nativeHealth = 3 - m_hitCount (alive); the effective budget is exactly (3 - count)*M - rem with 0 <= rem < M, maximum 3*M
            if (nativeHealth <= 0 || nativeHealth > ai::kBudget || v.maximum != (uint32_t)(ai::kBudget * v.multiplier)) return HMR_AI_MISMATCH;
            const uint32_t top = (uint32_t)nativeHealth * v.multiplier;
            if (v.current < 1 || v.current > top || v.current + v.multiplier <= top) return HMR_AI_MISMATCH;
        }
    } else return HMR_KIND;
    if (profile == tt::kProfile && v.kind != HM_VITALS_KIND_BUDGET) return HMR_TT_MISMATCH;
    if (profile == ai::kProfile && v.kind != HM_VITALS_KIND_BUDGET) return HMR_AI_MISMATCH;
    *view = HealthView{(int)v.current, (int)v.maximum, v.multiplier};
    return HMR_NONE;
}

namespace {
HealthView health_view_impl(ModContext* caller, const void* actor, uint32_t id, int profile, int nativeHealth, const HealthView& native);
}
HealthView hm_health_view(ModContext* caller, const void* actor, uint32_t id, int profile, int nativeHealth) {
    return health_view_impl(caller, actor, id, profile, nativeHealth, HealthView{nativeHealth, 0, 1});
}
namespace {
HealthView health_view_impl(ModContext* caller, const void* actor, uint32_t id, int profile, int nativeHealth, const HealthView& native) {
    if (g_svc == nullptr) return native;
    if (profile == 446) { ++g_st.skippedChuchu; return native; }   // Chuchus keep the inherited native bar (Bars 1.1.1 rule), whatever the provider says
    ++g_st.queries;
    HmVitals v{};
    v.struct_size = sizeof(HmVitals);
    const uint32_t status = g_svc->get_vitals(caller, actor, id, (uint16_t)profile, &v);
    switch (status) {
    case HM_VITALS_OK: {
        HealthView view{};
        const HmRejectCause c = hm_validate(v, id, profile, nativeHealth, &view);
        if (c != HMR_NONE) { ++g_st.rejected; ++g_st.rejectCause[c]; if (profile == tt::kProfile) ++g_st.ttRejected; if (profile == ai::kProfile) ++g_st.aiRejected; note(8 + c, "rejected cause", (unsigned)c); return native; }
        ++g_st.ok; if (v.kind == HM_VITALS_KIND_BUDGET) ++g_st.okBudget; else ++g_st.okEffective;
        if (profile == tt::kProfile) ++g_st.ttEffectiveViews;
        if (profile == ai::kProfile) ++g_st.aiEffectiveViews;
        if (v.multiplier != g_st.lastMultiplier) { ++g_st.scaleChanges; g_st.lastMultiplier = v.multiplier; }
        return view;
    }
    case HM_VITALS_USE_NATIVE:
        ++g_st.useNative;
        if (v.reason >= 1 && v.reason < 8) ++g_st.nativeReason[v.reason];
        else { ++g_st.rejected; ++g_st.rejectCause[HMR_REASON]; note(8 + HMR_REASON, "use-native with reason", v.reason); }
        if (g_st.lastMultiplier != 1) { ++g_st.scaleChanges; g_st.lastMultiplier = 1; }
        return native;
    case HM_VITALS_UNAVAILABLE:
        ++g_st.unavailable;
        if (v.reason >= 16 && v.reason < 24) ++g_st.unavailReason[v.reason - 16];
        note(1 + (v.reason & 7), "unavailable reason", v.reason);
        return native;
    case HM_VITALS_INVALID:
        ++g_st.invalid; note(0, "invalid status, reason", v.reason);
        return native;
    default:
        ++g_st.rejected; ++g_st.rejectCause[HMR_STATUS_UNKNOWN]; note(8 + HMR_STATUS_UNKNOWN, "unknown status", status);
        return native;
    }
}
}  // namespace
HealthView hm_normal_view(ModContext* caller, const void* actor, uint32_t id, int profile, int group, int nativeHealth, uint32_t status, uint32_t condition, uint32_t attention, bool senses, bool boss, bool allied) {
    const HealthView native{nativeHealth, 0, 1};
    if (g_svc == nullptr) return native;
    const NormalSample sample{profile, group, nativeHealth, status, condition, attention, false, senses, boss, allied};
    if (!normal_trackable(sample)) return native;
    return hm_health_view(caller, actor, id, profile, nativeHealth);
}
HealthView hm_tektite_view(ModContext* caller, const void* actor, uint32_t id, int counter, int health, int action, int group, uint32_t status, uint32_t condition, uint32_t attention, bool senses, bool boss, bool allied) {
    const HealthView nv = tt::native_view(counter, health, action);
    ++g_st.ttSeen;
    { bool known = false; for (unsigned i = 0; i < g_st.ttIdCount; ++i) if (g_st.ttIds[i] == id) { known = true; break; }
      if (!known) { if (g_st.ttIdCount < 32) g_st.ttIds[g_st.ttIdCount++] = id; else ++g_st.ttIdOverflow; } }
    if (nv.current <= 0) { ++g_st.ttDead; return nv; }                                   // defeated (counter / health / death action): nothing to draw, nothing to ask
    ++g_st.ttNativeViews;
    if (g_svc == nullptr) return nv;
    const NormalSample sample{tt::kProfile, group, nv.current, status, condition, attention, false, senses, boss, allied};
    if (!normal_trackable(sample)) return nv;
    return health_view_impl(caller, actor, id, tt::kProfile, nv.current, nv);
}
namespace {
// Counts reflected steps per instance (statistics only: the stored value is never used to build a view).
void ai_note(uint32_t id, int current) {
    for (unsigned i = 0; i < g_st.aiIdCount; ++i) if (g_st.aiIds[i] == id) { if ((uint32_t)current < g_st.aiLast[i]) ++g_st.aiSteps; else ++g_st.aiSteadyRepeats; g_st.aiLast[i] = (uint32_t)current; return; }
    if (g_st.aiIdCount < 32) { g_st.aiIds[g_st.aiIdCount] = id; g_st.aiLast[g_st.aiIdCount++] = (uint32_t)current; } else ++g_st.aiIdOverflow;
}
}  // namespace
HealthView hm_armos_view(ModContext* caller, const void* actor, uint32_t id, int count, int group, uint32_t status, uint32_t condition, uint32_t attention, bool senses, bool boss, bool allied) {
    const HealthView nv = ai::native_view(count);
    ++g_st.aiSeen;
    if (count < 0 || count > ai::kMaxCounter) ++g_st.aiUnreadable;
    if (nv.current <= 0) { ++g_st.aiDead; return nv; }                                   // defeated (or unreadable counter): nothing to draw, nothing to ask
    ++g_st.aiNativeViews;
    HealthView out = nv;
    if (g_svc != nullptr) {
        const NormalSample sample{ai::kProfile, group, nv.current, status, condition, attention, false, senses, boss, allied};
        if (normal_trackable(sample)) out = health_view_impl(caller, actor, id, ai::kProfile, nv.current, nv);
    }
    ai_note(id, out.current);
    return out;
}
}  // namespace ehz
