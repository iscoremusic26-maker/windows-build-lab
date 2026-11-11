#pragma once
#include "config_io.hpp"
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/shared/devf_encounter_api.h"
#include <cstring>
#include <algorithm>
namespace expansion {
inline bool compatible(const DevfEncounterPlacementService* s)noexcept{return service(s,1,0)&&s->request_placement&&s->remove_placement;}
struct Client {
 ModContext* ctx=nullptr;const HostService* host=nullptr;const DevfEncounterPlacementService* api=nullptr;
 std::array<uint64_t,map_config::capacity> tokens{};bool attempted=false;unsigned expected=0;const char* reason="EXPANSION_NOT_STARTED";
 using Emit=void(*)(const char*,unsigned,uint64_t,ModResult)noexcept;Emit emit=nullptr;
 void event(const char* e,unsigned i,uint64_t t,ModResult r)noexcept{if(emit)emit(e,i,t,r);}
 ModResult remove_all(bool query)noexcept{
  const DevfEncounterPlacementService* live=api;ModResult failed=MOD_OK;
  if(query){const void* current=nullptr;try{if(!host||!host->get_service||host->get_service(ctx,DEVF_ENCOUNTER_SERVICE_ID,1,0,&current)!=MOD_OK||current!=api){tokens={};reason="PROVIDER_UNAVAILABLE_SKIP_STALE_CALL";return MOD_OK;}live=static_cast<const DevfEncounterPlacementService*>(current);}catch(...){tokens={};reason="PROVIDER_LOOKUP_FAILED_DETACH_FALLBACK";return MOD_ERROR;}}
  if(!compatible(live)){tokens={};return MOD_UNSUPPORTED;}
  for(unsigned i=map_config::capacity;i>0;--i){auto& token=tokens[i-1];if(!token)continue;const auto t=token;token=0;ModResult r=MOD_ERROR;try{r=live->remove_placement(ctx,t);}catch(...){}event("REMOVE_EDIT_NOT_LIVE_ACTOR",i-1,t,r);if(r!=MOD_OK)failed=r;}
  return failed;
 }
 ModResult initialize(ModContext* c,const HostService* h,const DevfEncounterPlacementService* s,const map_config::Config& config,Emit log=nullptr)noexcept{
  if(attempted)return MOD_CONFLICT;attempted=true;ctx=c;host=h;api=s;emit=log;
  if(!c||!service(h,2,2)||!h->get_service||!compatible(s)){reason="EXPANSION_REQUIRED_API_INCOMPATIBLE";return MOD_UNSUPPORTED;}
  if(config.count>map_config::capacity)return MOD_INVALID_ARGUMENT;
  expected=config.count;
  for(unsigned i=0;i<expected;++i){
   auto request=config.requests[i];DevfEncounterPlacementReceipt receipt{};receipt.struct_size=sizeof(receipt);ModResult r=MOD_ERROR;
   try{r=s->request_placement(c,&request,&receipt);}catch(...){reason="EXPANSION_REQUEST_EXCEPTION";remove_all(false);return MOD_ERROR;}
   if(r!=MOD_OK||receipt.struct_size<sizeof(receipt)||!receipt.placement_token||receipt.flags!=DEVF_ENCOUNTER_EDIT_REGISTERED||receipt.registered_edits!=1||receipt.reserved||(std::find(tokens.begin(),tokens.end(),receipt.placement_token)!=tokens.end())){
    event("REQUEST_REJECTED_OR_BAD_RECEIPT",i,receipt.placement_token,r==MOD_OK?MOD_ERROR:r);
    if(receipt.placement_token&&std::find(tokens.begin(),tokens.end(),receipt.placement_token)==tokens.end()){try{s->remove_placement(c,receipt.placement_token);}catch(...){}}
    remove_all(false);reason="EXPANSION_BATCH_REJECTED_OWN_ROLLBACK_DETACH_FALLBACK";return r==MOD_OK?MOD_ERROR:r;
   }
   tokens[i]=receipt.placement_token;event("REQUEST_ACCEPTED_EDIT_ONLY",i,tokens[i],MOD_OK);
  }
  reason=expected?"EXPANSION_REQUESTS_ACCEPTED_NOT_ACTORS_OBSERVED":"EXPANSION_NONE_ZERO_REQUESTS";return MOD_OK;
 }
 ModResult shutdown()noexcept{if(std::none_of(tokens.begin(),tokens.end(),[](auto t){return t!=0;}))return MOD_OK;const auto r=remove_all(true);reason=r==MOD_OK?"EXPANSION_OWN_EDITS_REMOVED_NOT_LIVE_ACTORS":"EXPANSION_REMOVE_FAILED_OWNER_DETACH_FALLBACK";return r;}
};
}
