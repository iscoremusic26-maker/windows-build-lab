#pragma once
#include "variant_policy.hpp"
#include "../../CP10_2_BLACK_BOKOBLIN/expansion/black_policy.hpp"
namespace devf_typed {
// Separate from Black's frozen exact 0x0132 comparator. No distance matching.
// An exact stage/room/layer/home-XYZ slot is reserved for this DEVF placement:
// any signature/admission failure quarantines that slot from name-based fallback.
// Quarantine means no Expansion HP, damage, AI, eyes or regeneration writes;
// the engine's actor is not deleted or mutated. Other contexts remain unclaimed.
struct Reg {devf_black2::Reg record;bool blue=false,black=false;const void* identity=nullptr;};
struct Table {
 std::array<Reg,32> regs{};unsigned count=0;std::uint64_t next_generation=0;
 template<class Config> bool configure(const Config& c) noexcept {
  *this={};
  for(unsigned i=0;i<c.count;++i){
   if(c.variant[i]==0)continue;
   if((c.variant[i]!=1&&c.variant[i]!=2&&c.variant[i]!=3)||count>=regs.size())return false;
   const auto& q=c.requests[i];auto& t=regs[count++];t.blue=c.variant[i]==2;t.black=c.variant[i]==1;
   auto& r=t.record;r.used=true;r.room=q.room;r.layer=q.layer;r.x=q.x;r.y=q.y;r.z=q.z;r.yaw=q.yaw_s16;
   for(unsigned j=0;j+1<sizeof(r.id)&&q.placement_id[j];++j)r.id[j]=q.placement_id[j];
   for(unsigned j=0;j+1<count;++j){const auto& a=regs[j].record;
    if(!std::strcmp(a.id,r.id)||(a.room==r.room&&a.layer==r.layer&&a.x==r.x&&a.y==r.y&&a.z==r.z))return false;
   }
  }
  return true;
 }
 bool possible(const devf_black2::Snapshot& s)const noexcept {
  for(unsigned i=0;i<count;++i){const auto& r=regs[i].record;if(s.room==r.room&&s.x==r.x&&s.y==r.y&&s.z==r.z)return true;}return false;
 }
 int candidate(const devf_black2::Snapshot& s)const noexcept {
  if(!s.stage)return -1;
  for(unsigned i=0;i<count;++i){const auto& r=regs[i].record;
   if(s.room==r.room&&s.layer==r.layer&&s.x==r.x&&s.y==r.y&&s.z==r.z)return int(i);
  }return -1;
 }
 devf_variant::Registration admit(unsigned i,const devf_black2::Snapshot& s,const char* name,const void* identity,int native_base)noexcept {
  using R=devf_variant::Registration;
  if(i>=count)return R::TypedAdmissionFailed;
  auto& t=regs[i];auto& r=t.record;
  auto reject=[&](const char* reason){r.fault=true;r.reason=reason;r.admitted_id=0;return R::TypedAdmissionFailed;};
  if(r.fault)return R::TypedAdmissionFailed;
  if(candidate(s)!=int(i)||s.profile!=510||s.parameters!=devf_black2::kParameters||s.set_id!=0xFFFF||
     s.home_y!=r.yaw||s.home_z||s.home_x!=(t.blue?0x0032:0x0132))return reject("TYPED_EXACT_RECORD_MISMATCH_QUARANTINED");
  if(!identity||!s.id||s.id==0xFFFFFFFFu||s.id<=r.last_id)return reject("TYPED_INVALID_REUSED_ID_QUARANTINED");
  if(!s.initializing||s.previous_live)return reject("TYPED_LATE_OR_CONCURRENT_GENERATION_QUARANTINED");
  if(!name||std::strcmp(name,t.blue?"E_OC":"E_OC2")||s.health!=(t.blue?40:220)||native_base!=(t.blue?40:220))return reject("TYPED_RESOURCE_OR_NATIVE_HP_MISMATCH_QUARANTINED");
  r.last_id=r.admitted_id=s.id;r.generation=++next_generation;t.identity=identity;
  return t.black?R::Black:t.blue?R::Blue:R::Red;
 }
};
}
