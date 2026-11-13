#pragma once
// BALANCE SESSION01 0.11.2: normalized enemy resistance ONLY. No game headers,
// pointers, HP identification, or writes. Runtime maps the existing LNK07 result.
#include <cstdint>
namespace postbal01 {
inline constexpr const char* kVersion="0.11.2";
inline constexpr int kStandingRearm=25;
enum class Variant:std::uint8_t {Blue,Red,Black,BlackFury,Unknown,Count};
enum class Kind:std::uint8_t {Sword,Parry,Iron,Other};
enum class Reason:std::uint8_t {Verified,LegacyReference,VariantUnavailable,IdentityMismatch,EpochMismatch,UnknownResource};
struct Weights {int sword=25,parry=40,iron=50;};
inline constexpr Weights kWeights[5]={{25,40,50},{17,27,33},{13,20,25},{13,20,25},{25,40,50}};
inline unsigned index(Variant v) noexcept {return static_cast<unsigned>(v)<5?static_cast<unsigned>(v):4u;}
inline const char* name(Variant v) noexcept {const char* n[]={"BLUE","RED","BLACK","BLACK_FURY","UNKNOWN"};return n[index(v)];}
inline const char* name(Reason r) noexcept {const char* n[]={"VERIFIED_LNK07","LEGACY_REFERENCE","VARIANT_UNAVAILABLE","IDENTITY_MISMATCH","EPOCH_MISMATCH","UNKNOWN_RESOURCE"};return n[static_cast<unsigned>(r)<6?static_cast<unsigned>(r):2];}
struct Snapshot {
 bool requested=false,verified=false;int slot=-1;std::uint32_t id=0,epoch=0;std::uint64_t generation=0;
 Variant variant=Variant::Unknown;std::uint8_t method=6;Reason reason=Reason::VariantUnavailable;
};
struct Selection {bool requested=false,verified=false;Variant variant=Variant::Unknown;std::uint8_t method=6;Reason reason=Reason::LegacyReference;};
inline Selection select(const Snapshot& s,int slot,std::uint32_t id,std::uint64_t gen,std::uint32_t epoch) noexcept {
 Selection o;o.requested=s.requested;if(!s.requested)return o;
 o.reason=s.reason;
 if(s.slot!=slot||s.id!=id||s.generation!=gen){o.reason=Reason::IdentityMismatch;return o;}
 if(s.epoch!=epoch){o.reason=Reason::EpochMismatch;return o;}
 if(!s.verified||s.variant==Variant::Unknown||index(s.variant)==4)return o;
 o.verified=true;o.variant=s.variant;o.method=s.method;o.reason=Reason::Verified;return o;
}
inline int gain(const Selection& s,Kind k,int reference) noexcept {
 if(!s.verified)return reference;
 const auto&w=kWeights[index(s.variant)];return k==Kind::Sword?w.sword:k==Kind::Parry?w.parry:k==Kind::Iron?w.iron:0;
}
// LFT 0.13.6 FIX06 P0: ONE posture credit per Jump Strike, 1.6x a sword hit (user table: Blue 40, Red 27, Black 21; Black Fury keeps Black's 21: its
// weights are Black's, and no separate Fury identity is invented; an unverified variant gets 1.6x its reference weight).
inline constexpr int kJumpWeights[5]={40,27,21,21,40};
inline int jump_gain(const Selection& s,int reference) noexcept {
 if(!s.verified)return (reference*8+2)/5;
 return kJumpWeights[index(s.variant)];
}
inline bool same(const Selection& a,const Selection& b) noexcept {return a.requested==b.requested&&a.verified==b.verified&&a.variant==b.variant&&a.method==b.method&&a.reason==b.reason;}
struct Counters {
 std::uint64_t sword[5]{},iron[5]{},parries[5]{},units[5]{},crossings[5]{},delivered[5]{},standing25[5]{},getup0[5]{},rate_changes[5]{};
 std::uint64_t unavailable=0,stale=0,preview_mismatch=0,delivery_duplicate=0;
 void credit(const Selection& s,Kind k,int delta,bool crossed) noexcept {
  const unsigned i=index(s.variant);if(k==Kind::Sword)++sword[i];else if(k==Kind::Iron)++iron[i];else if(k==Kind::Parry)++parries[i];
  units[i]+=static_cast<std::uint64_t>(delta);if(crossed)++crossings[i];
  if(s.requested&&!s.verified){++unavailable;if(s.reason==Reason::IdentityMismatch||s.reason==Reason::EpochMismatch)++stale;}
 }
};
}
