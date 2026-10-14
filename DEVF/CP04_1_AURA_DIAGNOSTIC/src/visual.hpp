#pragma once
#include <cstdint>
namespace devf {
constexpr std::uint16_t kAura=0x29D;
constexpr std::uintptr_t kOwnerPrefix=std::uintptr_t{0x4456463400000000ULL};
constexpr std::uintptr_t owner_stamp(std::uint32_t id){return kOwnerPrefix|id;}
struct ResourceProbe {std::uint32_t work=0;bool child=false,indirect=false,fixed_interval=false,rate_or_life_key=false;float rate_random=0,life_random=0;};
enum class ParticleGate : std::uint8_t {
 Ready, ControlNull, ManagerNull, ArrayNull, BankCountInvalid, Bank0Null,
 ResourceMissing, DynamicsNull, KeyTableNull, KeyEntryNull, UserWorkUnsafe,
 ChildUnsafe, IndirectUnsafe, FixedIntervalUnsafe, RateLifetimeKeyUnsafe,
 RateRandomUnsafe, LifetimeRandomUnsafe, PoolCreateFailed, LevelEntryFailed
};
constexpr const char* gate_reason(ParticleGate g) {
 switch(g){
 case ParticleGate::Ready:return "PARTICLE_CREATE_READY";
 case ParticleGate::ControlNull:return "PARTICLE_CONTROL_NULL";
 case ParticleGate::ManagerNull:return "EMITTER_MANAGER_NULL";
 case ParticleGate::ArrayNull:return "RESOURCE_ARRAY_NULL";
 case ParticleGate::BankCountInvalid:return "RESOURCE_BANK_COUNT_INVALID";
 case ParticleGate::Bank0Null:return "RESOURCE_BANK0_NULL";
 case ParticleGate::ResourceMissing:return "RESOURCE_029D_MISSING";
 case ParticleGate::DynamicsNull:return "RESOURCE_DYNAMICS_NULL";
 case ParticleGate::KeyTableNull:return "RESOURCE_KEY_TABLE_NULL";
 case ParticleGate::KeyEntryNull:return "RESOURCE_KEY_ENTRY_NULL";
 case ParticleGate::UserWorkUnsafe:return "RESOURCE_USERWORK_FLAGS_UNSAFE";
 case ParticleGate::ChildUnsafe:return "RESOURCE_CHILD_UNSAFE";
 case ParticleGate::IndirectUnsafe:return "RESOURCE_INDIRECT_UNSAFE";
 case ParticleGate::FixedIntervalUnsafe:return "RESOURCE_FIXED_INTERVAL_UNSAFE";
 case ParticleGate::RateLifetimeKeyUnsafe:return "RESOURCE_RATE_OR_LIFETIME_KEY_UNSAFE";
 case ParticleGate::RateRandomUnsafe:return "RESOURCE_RATE_RANDOM_UNSAFE";
 case ParticleGate::LifetimeRandomUnsafe:return "RESOURCE_LIFETIME_RANDOM_UNSAFE";
 case ParticleGate::PoolCreateFailed:return "EMITTER_POOL_CREATE_FAILED";
 case ParticleGate::LevelEntryFailed:return "NATIVE_LEVEL_ENTRY_FAILED";
 }
 return "PARTICLE_GATE_ENUM_INVALID";
}
constexpr ParticleGate resource_gate(const ResourceProbe& r){
 // Identical acceptance and order of the historical v0.4.0 predicate.
 if(r.work&0xFD90u)return ParticleGate::UserWorkUnsafe;
 if(r.child)return ParticleGate::ChildUnsafe;
 if(r.indirect)return ParticleGate::IndirectUnsafe;
 if(r.fixed_interval)return ParticleGate::FixedIntervalUnsafe;
 if(r.rate_or_life_key)return ParticleGate::RateLifetimeKeyUnsafe;
 if(!(r.rate_random>=0&&r.rate_random<=1))return ParticleGate::RateRandomUnsafe;
 if(!(r.life_random>=0&&r.life_random<=1))return ParticleGate::LifetimeRandomUnsafe;
 return ParticleGate::Ready;
}
constexpr bool resource_safe(const ResourceProbe& r){return resource_gate(r)==ParticleGate::Ready;}
// Only copied scalar observations. mask: control1/manager2/array4/bank8/resource16/dyn32/work64.
struct CreateResult {
 std::uint32_t key=0;ParticleGate gate=ParticleGate::Ready;
 std::uint32_t probe_mask=0,dynamics_flags=0,key_count=0,rate_step=0;bool indirect_active=false;ResourceProbe resource;
 constexpr explicit operator bool()const{return key!=0&&gate==ParticleGate::Ready;}
};
struct VisualProbe {std::uint32_t id=0;bool eligible=false,live=false,model_ready=false;float x=0,y=0,z=0;};
// Handle + actor ID only. No persistent actor/emitter/controller/resource pointer.
struct Visual {
 std::uint32_t id=0,key=0;bool fault=false,applied=false;CreateResult last_create;
 template<class Emit>void reject(const char* why,Emit emit)noexcept{
  if(!fault)emit("CP04_VISUAL_REJECTED",id,key,why,"NO_SHARED_RESOURCE_WRITE");fault=true;
 }
 template<class Access,class Emit>void release(Access& access,const char* why,Emit emit)noexcept{
  if(!key){id=0;applied=false;return;}
  const auto previous=id,handle=key;
  const auto result=access.release(handle,previous); // 1 own record cut; 0 already gone; -1 not ours.
  key=0;id=0;applied=false;
  emit("CP04_VISUAL_RELEASED",previous,handle,why,result==1?"OWN_NATIVE_HANDLE_CUT":result==0?"HANDLE_ALREADY_GONE":"NOT_OWNED_NO_WRITE");
  if(result<0){id=previous;reject("REUSED_EMITTER_HANDLE",emit);}
 }
 template<class Access,class Emit>void step(const VisualProbe& p,Access& access,Emit emit)noexcept{
  if(fault||!p.eligible)return;
  if(!p.live){release(access,"NATIVE_DEATH_OR_DELETE_QUEUE",emit);return;}
  if(key&&id!=p.id){release(access,"NEXT_GENERATION",emit);if(fault)return;}
  if(!p.model_ready){id=p.id;reject("MODEL_NOT_READY_OR_NOT_OWNED",emit);return;}
  if(!key){
   id=p.id;last_create=access.create(p);
   if(!last_create){reject(gate_reason(last_create.gate),emit);return;}
   key=last_create.key;
   if(!access.refresh(key,p)){release(access,"CONFIGURATION_READBACK_FAILED",emit);id=p.id;reject("OWNERSHIP_OR_PRIVATE_CONFIG_READBACK_FAILED",emit);return;}
   applied=true;emit("CP04_VISUAL_APPLIED",id,key,"NATIVE_PARTICLE_MARKER","PRIVATE_EMITTER_CONFIG_READBACK_RENDER_NOT_VERIFIED");
  }else if(!access.refresh(key,p)){
   release(access,"REFRESH_OR_OWNERSHIP_FAILED",emit);id=p.id;reject("OWNERSHIP_OR_PRIVATE_CONFIG_READBACK_FAILED",emit);
  }
 }
 template<class Emit>void shutdown(Emit emit)noexcept{
  // Never dereference game state at DLL shutdown: native level expiry owns remaining cleanup.
  if(key)emit("CP04_VISUAL_RELEASED",id,key,"SHUTDOWN_REFRESH_RELINQUISHED","NATIVE_EXPIRY_OR_FULL_PROCESS_EXIT_NOT_HOT_DELETE");
  id=key=0;applied=false;
 }
};
}
