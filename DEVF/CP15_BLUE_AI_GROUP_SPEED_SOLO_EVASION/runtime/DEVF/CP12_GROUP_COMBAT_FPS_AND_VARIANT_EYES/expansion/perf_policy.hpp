#pragma once
// CP11 pure helpers (no game headers) for the two behaviour-preserving optimisations that are not inside the aura policy.
//
// 1) LAZY REGISTRATION CONTEXT. The PRE hook builds a devf_black2::Snapshot for every Bokoblin on every tick while a Black is configured
//    (and once per generation for classification). Its two context fields (stage F_SP121?, layer) need dComIfGp_getStartStageName() and
//    dComIfG_play_c::getLayerNo(0); the latter recomputes the layer on every call (d_com_inf_game.cpp getLayerNo_common_common: a twilight
//    table check plus a chain of ~40 stage-name strcmp and event-bit reads). Those two fields are read ONLY by Table::context_matches, and
//    every consumer (Table::find, Table::matches, Table::apply, devf_variant::match) evaluates `record_matches(reg,s) && context_matches(reg,s)`
//    for a USED registration. So when no used registration of either table record-matches, the context can never be read and computing it
//    is dead work: skipping it is exact. When some registration does record-match (in practice the registered Black itself), the context
//    is computed exactly as before.
// 2) TELEMETRY BUDGET. Full-detail acquisition lines (REQUESTED, COLOR_APPLIED, TUNING_APPLIED, DRAW_BASIS_CHECK and the geometry tuple of
//    the state lines) are written for the first kDetailAcquisitions emitters of each variant per session; afterwards the state lines stay
//    (compact) and every anomaly (retry, rejection, colour/geometry/presence mismatch, degenerate basis) is still logged in full.
#include "../../CP10_2_BLACK_BOKOBLIN/expansion/black_policy.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
namespace devf_perf {
inline bool any_record_match(const devf_black2::Table& a,const devf_black2::Table& b,const devf_black2::Snapshot& s) noexcept {
 for(unsigned i=0;i<a.count;++i)if(a.regs[i].used&&devf_black2::Table::record_matches(a.regs[i],s))return true;
 for(unsigned i=0;i<b.count;++i)if(b.regs[i].used&&devf_black2::Table::record_matches(b.regs[i],s))return true;
 return false;
}
// Fills the context only when it can be read. `Ctx` is called at most once and must set s.stage and s.layer.
template<class Ctx> inline void complete_snapshot(const devf_black2::Table& a,const devf_black2::Table& b,devf_black2::Snapshot& s,Ctx&& ctx) noexcept {
 if(any_record_match(a,b,s))ctx(s);
}
// 3) EARLY-EXIT LEDGER LOOKUP. devf_vis::Ledger::find / devf_dyn::Ledger::find scan all 64 slots when the id is absent (the common case for
//    the visual ledger: most Bokoblins have no aura). Both ledgers keep `used` exact (record ++, release --), so the scan can stop once
//    `used` occupied slots were seen: same result as find() (nullptr for id 0, first used slot with that id otherwise).
template<class L> inline auto* find_used(L& ledger,std::uint32_t id) noexcept {
 decltype(&ledger.entries[0]) none=nullptr;
 if(!id||!ledger.used)return none;
 unsigned seen=0;
 for(auto& e:ledger.entries){if(!e.used)continue;if(e.id==id)return &e;if(++seen>=ledger.used)break;}
 return none;
}
constexpr unsigned kDetailAcquisitions=2;
struct LogBudget {
 unsigned used[4]{};   // indexed by devf_variant::Variant
 static constexpr unsigned slot(devf_variant::Variant v) noexcept {return unsigned(v)&3u;}
 bool detail(devf_variant::Variant v) const noexcept {return used[slot(v)]<kDetailAcquisitions;}
 // Call once per created emitter, AFTER its lines were written. Returns true exactly once per variant: when the budget becomes exhausted.
 bool consume(devf_variant::Variant v) noexcept {unsigned& u=used[slot(v)];if(u<kDetailAcquisitions){++u;return u==kDetailAcquisitions;}return false;}
};
// Which acquisition lines to write. Anomalies are never suppressed.
struct LineFilter {
 static constexpr bool requested(bool detail) noexcept {return detail;}
 static constexpr bool colour(bool detail,bool readback_ok) noexcept {return detail||!readback_ok;}
 static constexpr bool tuning(bool detail,bool geo_ok,bool presence_ok) noexcept {return detail||!geo_ok||!presence_ok;}
 static constexpr bool basis(bool detail,bool drawable) noexcept {return detail||!drawable;}
};
} // namespace devf_perf
