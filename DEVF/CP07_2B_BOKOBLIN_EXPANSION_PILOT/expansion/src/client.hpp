#pragma once
#include "mode.hpp"
#include "generated_encounters.hpp"
#include "../../shared/devf_encounter_api.h"
#include <cstring>
namespace expansion {
inline bool compatible(const DevfEncounterPlacementService* s)noexcept{return service(s,1,0)&&s->request_placement&&s->remove_placement;}
inline DevfEncounterPlacementRequest request_for(unsigned i)noexcept{
 DevfEncounterPlacementRequest r{};if(i>=captured.size())return r;const auto& s=captured[i];r.struct_size=sizeof(r);r.native_profile=s.profile;std::memcpy(r.placement_id,s.id,std::strlen(s.id)+1);std::memcpy(r.stage,s.stage,std::strlen(s.stage)+1);
 r.room=s.room;r.layer=s.layer;r.x=s.x;r.y=s.y;r.z=s.z;r.yaw_s16=s.yaw;r.parameters=0xFFFF0100u;return r;
}
struct Client {
 ModContext* ctx=nullptr;const HostService* host=nullptr;const DevfEncounterPlacementService* api=nullptr;
 std::array<uint64_t,2> tokens{};bool attempted=false;unsigned expected=0;const char* reason="EXPANSION_NOT_STARTED";
 using Emit=void(*)(const char*,unsigned,uint64_t,ModResult)noexcept;Emit emit=nullptr;
 void event(const char* e,unsigned i,uint64_t t,ModResult r)noexcept{if(emit)emit(e,i,t,r);}
 ModResult remove_all(bool query)noexcept{
  const DevfEncounterPlacementService* live=api;ModResult failed=MOD_OK;
  if(query){const void* current=nullptr;try{if(!host||!host->get_service||host->get_service(ctx,DEVF_ENCOUNTER_SERVICE_ID,1,0,&current)!=MOD_OK||current!=api){tokens={};reason="PROVIDER_UNAVAILABLE_SKIP_STALE_CALL";return MOD_OK;}live=static_cast<const DevfEncounterPlacementService*>(current);}catch(...){tokens={};reason="PROVIDER_LOOKUP_FAILED_DETACH_FALLBACK";return MOD_ERROR;}}
  if(!compatible(live)){tokens={};return MOD_UNSUPPORTED;}
  for(unsigned i=2;i>0;--i){auto& token=tokens[i-1];if(!token)continue;const auto t=token;token=0;ModResult r=MOD_ERROR;try{r=live->remove_placement(ctx,t);}catch(...){}event("REMOVE_EDIT_NOT_LIVE_ACTOR",i-1,t,r);if(r!=MOD_OK)failed=r;}
  return failed;
 }
 ModResult initialize(ModContext* c,const HostService* h,const DevfEncounterPlacementService* s,Mode mode,Emit log=nullptr)noexcept{
  if(attempted)return MOD_CONFLICT;attempted=true;ctx=c;host=h;api=s;emit=log;
  if(!c||!service(h,2,2)||!h->get_service||!compatible(s)){reason="EXPANSION_REQUIRED_API_INCOMPATIBLE";return MOD_UNSUPPORTED;}
  if(mode!=Mode::None&&mode!=Mode::One&&mode!=Mode::Both){reason="EXPANSION_MODE_INVALID";return MOD_INVALID_ARGUMENT;}
  expected=mode==Mode::Both?2:mode==Mode::One?1:0;
  for(unsigned i=0;i<expected;++i){
   auto request=request_for(i);DevfEncounterPlacementReceipt receipt{};receipt.struct_size=sizeof(receipt);ModResult r=MOD_ERROR;
   try{r=s->request_placement(c,&request,&receipt);}catch(...){reason="EXPANSION_REQUEST_EXCEPTION";remove_all(false);return MOD_ERROR;}
   if(r!=MOD_OK||receipt.struct_size<sizeof(receipt)||!receipt.placement_token||receipt.flags!=DEVF_ENCOUNTER_EDIT_REGISTERED||receipt.registered_edits!=1||receipt.reserved||(i&&receipt.placement_token==tokens[0])){
    event("REQUEST_REJECTED_OR_BAD_RECEIPT",i,receipt.placement_token,r==MOD_OK?MOD_ERROR:r);
    if(receipt.placement_token&&receipt.placement_token!=tokens[0]){try{s->remove_placement(c,receipt.placement_token);}catch(...){}}
    remove_all(false);reason="EXPANSION_BATCH_REJECTED_OWN_ROLLBACK_DETACH_FALLBACK";return r==MOD_OK?MOD_ERROR:r;
   }
   tokens[i]=receipt.placement_token;event("REQUEST_ACCEPTED_EDIT_ONLY",i,tokens[i],MOD_OK);
  }
  reason=expected?"EXPANSION_REQUESTS_ACCEPTED_NOT_ACTORS_OBSERVED":"EXPANSION_NONE_ZERO_REQUESTS";return MOD_OK;
 }
 ModResult shutdown()noexcept{if(!tokens[0]&&!tokens[1])return MOD_OK;const auto r=remove_all(true);reason=r==MOD_OK?"EXPANSION_OWN_EDITS_REMOVED_NOT_LIVE_ACTORS":"EXPANSION_REMOVE_FAILED_OWNER_DETACH_FALLBACK";return r;}
};
}
