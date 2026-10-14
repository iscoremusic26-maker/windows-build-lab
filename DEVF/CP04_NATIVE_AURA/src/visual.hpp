#pragma once
#include <cstdint>
namespace devf {
constexpr std::uint16_t kAura=0x29D;
constexpr std::uintptr_t kOwnerPrefix=std::uintptr_t{0x4456463400000000ULL};
constexpr std::uintptr_t owner_stamp(std::uint32_t id){return kOwnerPrefix|id;}
struct ResourceProbe {std::uint32_t work=0;bool child=false,indirect=false,fixed_interval=false,rate_or_life_key=false;float rate_random=0,life_random=0;};
constexpr bool resource_safe(const ResourceProbe& r){
 return !(r.work&0xFD90u)&&!r.child&&!r.indirect&&!r.fixed_interval&&!r.rate_or_life_key&&
  r.rate_random>=0&&r.rate_random<=1&&r.life_random>=0&&r.life_random<=1;
}
struct VisualProbe {std::uint32_t id=0;bool eligible=false,live=false,model_ready=false;float x=0,y=0,z=0;};
// Handle + actor ID only. No persistent actor/emitter/controller/resource pointer.
struct Visual {
 std::uint32_t id=0,key=0;bool fault=false,applied=false;
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
   id=p.id;const auto created=access.create(p);
   if(!created){reject("PARTICLE_RESOURCE_OR_POOL_GATE_FAILED",emit);return;}
   key=created;
   if(!access.refresh(key,p)){release(access,"CONFIGURATION_READBACK_FAILED",emit);id=p.id;reject("PRIVATE_EMITTER_CONFIG_FAILED",emit);return;}
   applied=true;emit("CP04_VISUAL_APPLIED",id,key,"NATIVE_PARTICLE_MARKER","PRIVATE_EMITTER_CONFIG_READBACK_RENDER_NOT_VERIFIED");
  }else if(!access.refresh(key,p)){
   release(access,"REFRESH_OR_OWNERSHIP_FAILED",emit);id=p.id;reject("EMITTER_NOT_AVAILABLE_OR_NOT_OWNED",emit);
  }
 }
 template<class Emit>void shutdown(Emit emit)noexcept{
  // Never dereference game state at DLL shutdown: native level expiry owns remaining cleanup.
  if(key)emit("CP04_VISUAL_RELEASED",id,key,"SHUTDOWN_REFRESH_RELINQUISHED","NATIVE_EXPIRY_OR_FULL_PROCESS_EXIT_NOT_HOT_DELETE");
  id=key=0;applied=false;
 }
};
}
