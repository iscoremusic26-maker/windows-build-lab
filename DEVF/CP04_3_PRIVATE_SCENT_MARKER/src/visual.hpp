#pragma once
#include <cstdint>
namespace devf {
constexpr std::uint16_t kAura=0x84ED;
constexpr unsigned kBank=1,kParticleBudget=20;
constexpr std::uintptr_t kOwnerPrefix=std::uintptr_t{0x4456464300000000ULL};
constexpr std::uintptr_t owner_stamp(std::uint32_t id){return kOwnerPrefix|id;}
constexpr bool in_range(float x,float low,float high){return x>=low&&x<=high;}
// Translate ONLY source-supported ordinary/fog groups, never silently strip bits.
constexpr bool native_group(std::uint32_t work,unsigned& group){
 if(work&~0x00EF007Fu)return false;
 group=work&1?0:work&2?1:work&4?2:work&8?3:0;
 if(work&0x10)group+=8;
 return true;
}
enum class Gate:std::uint8_t {
 Ready,ControlNull,ManagerNull,ArrayNull,BankMissing,ResourceMissing,IdentityChanged,
 DynamicsNull,ShapeNull,GroupUnsafe,GroupUnavailable,ChildUnsafe,IndirectUnsafe,
 FixedIntervalUnsafe,RandomUnsafe,StartFrameUnsafe,VolumeTypeUnsafe,KeysUnsupported,
 FieldLimit,FieldNull,ShapeUnsafe,ScaleUnsafe,TextureUnsafe,PoolFailed,EntryFailed
};
constexpr const char* gate_reason(Gate g){
 switch(g){
 case Gate::Ready:return "ONCE_NATIVE_PREFLIGHT_ACCEPTED";
 case Gate::ControlNull:return "PARTICLE_CONTROL_NULL";
 case Gate::ManagerNull:return "EMITTER_MANAGER_NULL";
 case Gate::ArrayNull:return "RESOURCE_ARRAY_NULL";
 case Gate::BankMissing:return "SCENE_BANK1_NOT_LOADED";
 case Gate::ResourceMissing:return "RESOURCE_84ED_MISSING_NO_FALLBACK";
 case Gate::IdentityChanged:return "RESOURCE_IDENTITY_CHANGED";
 case Gate::DynamicsNull:return "DYNAMICS_OR_DATA_NULL";
 case Gate::ShapeNull:return "BASE_SHAPE_OR_DATA_NULL";
 case Gate::GroupUnsafe:return "USERWORK_CALLBACK_OR_SPECIAL_GROUP_UNSUPPORTED";
 case Gate::GroupUnavailable:return "NATIVE_GROUP_UNAVAILABLE";
 case Gate::ChildUnsafe:return "CHILD_SHAPE_UNSUPPORTED";
 case Gate::IndirectUnsafe:return "EXTRA_TEXTURE_BLOCK_UNSUPPORTED";
 case Gate::FixedIntervalUnsafe:return "FIXED_INTERVAL_UNBOUNDED_FOR_THIS_TEST";
 case Gate::RandomUnsafe:return "RATE_OR_LIFETIME_RANDOM_OUT_OF_RANGE";
 case Gate::StartFrameUnsafe:return "DELAYED_START_UNSUPPORTED";
 case Gate::VolumeTypeUnsafe:return "VOLUME_FUNCTION_INVALID";
 case Gate::KeysUnsupported:return "KEY_ANIMATION_UNSUPPORTED_NO_CURVE_OVERRIDE";
 case Gate::FieldLimit:return "FIELD_COUNT_EXCEEDS_FOUR";
 case Gate::FieldNull:return "FIELD_TABLE_OR_ENTRY_NULL";
 case Gate::ShapeUnsafe:return "BASE_SHAPE_SIZE_OR_DRAW_UNSUPPORTED";
 case Gate::ScaleUnsafe:return "EXTRA_SCALE_BOUND_UNSUPPORTED";
 case Gate::TextureUnsafe:return "SCENE_TEXTURE_DEPENDENCY_INVALID";
 case Gate::PoolFailed:return "EMITTER_POOL_EXHAUSTED";
 case Gate::EntryFailed:return "NATIVE_LEVEL_ENTRY_FAILED";
 }
 return "GATE_INVALID";
}
struct CreateResult {
 std::uint32_t key=0,probe_mask=0,work=0,dyn_flags=0,key_count=0,fields=0;
 unsigned group=255,gid_max=0;float rate_random=0,life_random=0;
 bool child=false,indirect=false;Gate gate=Gate::Ready;
 constexpr explicit operator bool()const{return key&&gate==Gate::Ready;}
};
struct VisualProbe {std::uint32_t id=0;bool eligible=false,live=false,model_ready=false;float x=0,y=0,z=0;};
// IDs, handle and scalar preflight evidence only. No persistent engine pointers.
struct Visual {
 std::uint32_t id=0,key=0,attempted_id=0;bool fault=false,applied=false;CreateResult last_create;
 template<class Emit>void reject(const char* why,Emit emit)noexcept{
  if(!fault)emit("CP04_VISUAL_REJECTED",id,key,why,"NO_SHARED_RESOURCE_WRITE");fault=true;
 }
 template<class Access,class Emit>void release(Access& access,const char* why,Emit emit)noexcept{
  if(!key){id=0;applied=false;return;}
  const auto previous=id,handle=key;const auto result=access.release(handle,previous);
  key=id=0;applied=false;
  emit("CP04_VISUAL_RELEASED",previous,handle,why,result==1?"OWN_NATIVE_HANDLE_CUT":result==0?"HANDLE_ALREADY_GONE":"NOT_OWNED_NO_WRITE");
  if(result<0){id=previous;reject("REUSED_EMITTER_HANDLE",emit);}
 }
 template<class Access,class Emit>void step(const VisualProbe& p,Access& access,Emit emit)noexcept{
  if(fault||!p.eligible)return;
  if(!p.live){release(access,"NATIVE_DEATH_OR_DELETE_QUEUE",emit);return;}
  if(key&&id!=p.id){release(access,"NEXT_GENERATION",emit);if(fault)return;}
  if(!in_range(p.x,-1e7f,1e7f)||!in_range(p.y,-1e7f,1e7f)||!in_range(p.z,-1e7f,1e7f)){
   id=p.id;reject("ANCHOR_NONFINITE_OR_OUT_OF_RANGE",emit);return;
  }
  if(!p.model_ready){id=p.id;reject("MODEL_NOT_READY_OR_NOT_OWNED",emit);return;}
  if(!key){
   // Never retry if the host has expired this generation's handle while execute was absent.
   if(attempted_id==p.id){reject("GENERATION_HANDLE_EXPIRED_NO_RETRY",emit);return;}
   id=p.id;attempted_id=p.id;last_create=access.create(p);
   if(!last_create){reject(gate_reason(last_create.gate),emit);return;}
   key=last_create.key;
   if(!access.refresh(key,p)){release(access,"CONFIGURATION_READBACK_FAILED",emit);id=p.id;reject("PRIVATE_CONFIG_OR_OWNERSHIP_FAILED",emit);return;}
   applied=true;emit("CP04_VISUAL_APPLIED",id,key,"PRIVATE_PURPLE_SCENT_MARKER","PRIVATE_READBACK_VISUAL_PIXELS_HUMAN_NOT_VERIFIED");
  }else if(!access.refresh(key,p)){
   release(access,"REFRESH_OR_OWNERSHIP_FAILED",emit);id=p.id;reject("HANDLE_EXPIRED_OR_PRIVATE_CONFIG_FAILED_NO_RETRY",emit);
  }
 }
 template<class Emit>void shutdown(Emit emit)noexcept{
  // Stop refresh, no engine dereference/unload callback. Host level expiry or full process exit.
  if(key)emit("CP04_VISUAL_RELEASED",id,key,"SHUTDOWN_REFRESH_RELINQUISHED","NATIVE_EXPIRY_OR_FULL_PROCESS_EXIT_NOT_HOT_DELETE");
  id=key=0;applied=false;
 }
};
}
