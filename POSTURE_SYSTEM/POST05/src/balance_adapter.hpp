#pragma once
// Shared read-only adapter for the ALREADY VERIFIED LNK07 classifier. No second
// identity heuristic, HP input, actor pointer or additional native getter.
#include "balance_core.hpp"
#include "engine.hpp"
#include "lnk_core.hpp"
namespace postbal01 {
static_assert(static_cast<unsigned>(Variant::Blue)==static_cast<unsigned>(lnk01::policy::Variant::Blue)&&static_cast<unsigned>(Variant::Red)==static_cast<unsigned>(lnk01::policy::Variant::Red)&&static_cast<unsigned>(Variant::Black)==static_cast<unsigned>(lnk01::policy::Variant::Black)&&static_cast<unsigned>(Variant::BlackFury)==static_cast<unsigned>(lnk01::policy::Variant::BlackFury)&&static_cast<unsigned>(Variant::Unknown)==static_cast<unsigned>(lnk01::policy::Variant::Unknown),"LNK07 variant tags must match");
inline Snapshot capture(const post03::Ledger& L,const lnk01::VariantTrack& variants,int slot,std::uint32_t id,std::uint64_t generation,const post03::NativeAux& a) noexcept {
 Snapshot s;s.requested=true;s.slot=slot;s.id=id;s.generation=generation;
 if(slot<0||static_cast<unsigned>(slot)>=post03::kCapacity||!L.valid(slot,generation)||L.slot(static_cast<unsigned>(slot)).id!=id){s.reason=Reason::IdentityMismatch;return s;}
 const auto& in=L.slot(static_cast<unsigned>(slot));s.epoch=in.epoch_last;
 if(in.life!=post03::Life::Active||!post03::is_candidate(in.adm)){s.reason=Reason::IdentityMismatch;return s;}
 if(!a.valid||(a.resource!=static_cast<unsigned>(post03::NameKind::E_OC)&&a.resource!=static_cast<unsigned>(post03::NameKind::E_OC2))){s.reason=Reason::UnknownResource;return s;}
 const auto v=variants.classify(slot,id,generation,a.resource==static_cast<unsigned>(post03::NameKind::E_OC2),a.atp);
 s.variant=static_cast<Variant>(v.v);s.method=static_cast<std::uint8_t>(v.m);s.verified=v.v!=lnk01::policy::Variant::Unknown;
 s.reason=s.verified?Reason::Verified:Reason::VariantUnavailable;return s;
}
}
