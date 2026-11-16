#pragma once
// CP10.5 pure policy (no game headers): offensive attack power of a classified Bokoblin variant.
//
// HOST CONTRACT (Dusklight 40457c6, verified in source; see IMPLEMENTATION.md):
//  * d_a_e_oc.cpp create() L2786-2802 sets BOTH attack spheres mSphs_at[0] and mSphs_at[1] with SetAtAtp(1) for "E_OC" or SetAtAtp(2)
//    otherwise, and only then calls daE_OC_Execute(this): the first execute (our PRE) sees both colliders fully initialised.
//  * mAtp is written only by cCcD_ObjAt::Set(src) (create) and SetAtp(); nothing else in the host rewrites it, so ONE write per
//    actor generation is enough (no per-frame writes).
//  * Link side: cCcS::CalcTgPlusDmg -> cCcD_Stts::PlusDmg(atp) keeps the MAXIMUM atp among the hits of the frame (two spheres, or two
//    hits, never add up); daAlink_c::procDamage reads mCcStts.GetDmg() and calls setDamagePoint(dmg), which multiplies by
//    damageMagnification() (and rounds up) before dComIfGp_setItemLifeCount(-dmg). A shield hit takes the ChkTgShieldHit branch and
//    never reaches setDamagePoint; that decision does not depend on atp.
//
// TARGET (the base hit, relative to the original Blue E_OC whose native atp is 1):  BLUE 1, RED 2, BLACK 3.
// The target is an ABSOLUTE value that replaces the native one, never a multiplier of it: native E_OC2 already carries 2.
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_policy.hpp"
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_damage {
using devf_variant::Variant;
constexpr std::uint8_t kAtpBlue=1,kAtpRed=2,kAtpBlack=3;
constexpr std::uint8_t kNativeAtpE_OC=1,kNativeAtpE_OC2=2;
constexpr unsigned kSpheres=2,kCapacity=64;
enum class Why:std::uint8_t {None,NoVariant,UnknownName,VariantResourceMismatch,SpheresDisagree,NativeAtpMismatch,LedgerFull,ReadbackMismatch};
inline const char* why_text(Why w) noexcept {
 switch(w){case Why::NoVariant:return "NO_CLASSIFIED_VARIANT_COLLIDERS_UNTOUCHED";case Why::UnknownName:return "UNKNOWN_BOKOBLIN_NAME";
  case Why::VariantResourceMismatch:return "VARIANT_INCOMPATIBLE_WITH_RESOURCE";case Why::SpheresDisagree:return "ATTACK_SPHERES_DISAGREE_NOT_PRISTINE";
  case Why::NativeAtpMismatch:return "NATIVE_ATTACK_POWER_SIGNATURE_MISMATCH";case Why::LedgerFull:return "DAMAGE_LEDGER_FULL";case Why::ReadbackMismatch:return "WRITE_READBACK_MISMATCH_RESTORED";default:return "NONE";}
}
constexpr std::uint8_t target_for(Variant v) noexcept {return v==Variant::Blue?kAtpBlue:v==Variant::Red?kAtpRed:v==Variant::Black?kAtpBlack:std::uint8_t(0);}
struct Plan {bool accepted=false,write=false;std::uint8_t native=0,target=0;Why why=Why::None;};
// `variant` is the CP10.4 classification (None = rejected there). a0/a1 = GetAtAtp() of the two spheres BEFORE any write.
inline Plan make_plan(Variant variant,const char* name,std::uint8_t a0,std::uint8_t a1) noexcept {
 Plan p;
 if(variant==Variant::None){p.why=Why::NoVariant;return p;}
 std::uint8_t native=0;bool e_oc2=false;
 if(name&&!std::strcmp(name,"E_OC"))native=kNativeAtpE_OC;
 else if(name&&!std::strcmp(name,"E_OC2")){native=kNativeAtpE_OC2;e_oc2=true;}
 else {p.why=Why::UnknownName;return p;}
 if((variant==Variant::Red||variant==Variant::Black)&&!e_oc2){p.why=Why::VariantResourceMismatch;return p;}
 if(a0!=a1){p.why=Why::SpheresDisagree;return p;}                    // someone already touched one of them: do not guess
 if(a0!=native){p.why=Why::NativeAtpMismatch;return p;}               // pristine native creation values only
 p.accepted=true;p.native=native;p.target=target_for(variant);p.write=p.target!=native;
 return p;
}
// One record per applied generation, only to allow a safe rollback at unload; keyed by actor id + address + registry generation.
struct Entry {bool used=false,written=false;std::uint32_t id=0,generation=0;const void* identity=nullptr;Variant variant=Variant::None;std::uint8_t native=0,target=0;};
struct Ledger {
 std::array<Entry,kCapacity> entries{};unsigned used=0;
 Entry* find(std::uint32_t id) noexcept {if(!id)return nullptr;for(auto& e:entries)if(e.used&&e.id==id)return &e;return nullptr;}
 void release(Entry& e) noexcept {if(e.used&&used)--used;e=Entry{};}
 // Fresh generation of `id`: the caller has already released any previous record of the same id. nullptr = full (fail closed).
 Entry* record(std::uint32_t id,const void* identity,std::uint32_t generation,Variant v,const Plan& p) noexcept {
  if(!id||!identity||find(id))return nullptr;
  for(auto& slot:entries)if(!slot.used){slot=Entry{true,false,id,generation,identity,v,p.native,p.target};++used;return &slot;}
  return nullptr;
 }
};
// Rollback decision. `live_same_actor` = the id still resolves to the very same, not-destroyed object. The colliders are restored only if
// both still hold the exact value this module wrote (otherwise somebody else changed them and they are not ours to touch).
struct RestorePlan {bool write=false;const char* why="NOTHING_TO_RESTORE";};
inline RestorePlan restore_plan(const Entry& e,bool live_same_actor,std::uint8_t a0,std::uint8_t a1) noexcept {
 RestorePlan r;
 if(!e.used)return r;
 if(!live_same_actor){r.why="ACTOR_GONE_OR_REPLACED_NOTHING_READ";return r;}
 if(!e.written){r.why="NO_WRITE_WAS_NEEDED_NATIVE_VALUE_ALREADY_TARGET";return r;}
 if(a0!=e.target||a1!=e.target){r.why="COLLIDER_CHANGED_BY_OTHERS_NOT_OWNED";return r;}
 r.write=true;r.why="RESTORE_NATIVE_ATTACK_POWER";return r;
}
} // namespace devf_damage
