#pragma once
#include <mods/svc/stage.h>
#include "native_records.hpp"
namespace native_qa {
template<class T>bool compatible(const T* s,unsigned major,unsigned minor){
 return s&&s->header.struct_size>=sizeof(T)&&s->header.major_version==major&&s->header.minor_version>=minor;
}
struct Registration {
 std::array<StageActorHandle,2> handles{};bool attempted=false,ready=false;unsigned expected=0;
 const char* reason="NOT_STARTED";ModResult cleanup_result=MOD_OK;
 ModResult cleanup(ModContext* ctx,const StageService* svc) noexcept {
  ready=false;ModResult failure=MOD_OK;
  for(unsigned i=2;i>0;--i){
   auto& h=handles[i-1];if(!h)continue;
   // Duplicate handles are an invalid backend response, not a second authority.
   for(unsigned j=0;j<i-1;++j)if(handles[j]==h)handles[j]=0;
   ModResult result=MOD_UNAVAILABLE;
   try {if(ctx&&compatible(svc,1,0)&&svc->remove_actor_edit)result=svc->remove_actor_edit(ctx,h);}
   catch(...){result=MOD_ERROR;}
   if(result==MOD_OK)h=0;else if(failure==MOD_OK)failure=result;
  }
  cleanup_result=failure;return failure;
 }
 ModResult initialize(ModContext* ctx,const StageService* svc,const std::array<bool,2>& active) noexcept {
  if(attempted){reason="NO_REINJECTION";return MOD_CONFLICT;}attempted=true;
  expected=unsigned(active[0])+unsigned(active[1]);
  if(!ctx||!compatible(svc,1,0)||!svc->add_actor||!svc->remove_actor_edit){reason="STAGE_API_UNAVAILABLE";return MOD_UNSUPPORTED;}
  unsigned i=0;
  for(unsigned site=0;site<2;++site){
   if(!active[site])continue;
   const Record bytes=record(site==1);
   ModResult result=MOD_ERROR;StageActorHandle h=0;
   try {result=svc->add_actor(ctx,stage,room,layer,bytes.data(),bytes.size(),&h);}
   catch(...){handles[i]=h;reason="ADD_EXCEPTION";cleanup(ctx,svc);return MOD_ERROR;}
   handles[i]=h; // Only handles produced by this mod's own service call.
   if(result!=MOD_OK||!h||(i&&h==handles[0])){
    reason=result!=MOD_OK?"ADD_FAILED":!h?"ZERO_HANDLE":"DUPLICATE_HANDLE";
    cleanup(ctx,svc);return result==MOD_OK?MOD_ERROR:result;
   }
   ++i;
  }
  ready=true;reason="STAGE_EDITS_REGISTERED_NOT_ACTOR_OBSERVED";return MOD_OK;
 }
};
}
