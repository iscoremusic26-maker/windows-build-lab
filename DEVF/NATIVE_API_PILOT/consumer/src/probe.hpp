#pragma once
#include "../../shared/devf_native_api.h"
#include <mods/svc/host.h>
#include <cstring>
namespace devf_probe {
inline bool compatible(const DevfNativePlacementService* s) noexcept {
 return s&&s->header.struct_size>=sizeof(*s)&&s->header.major_version==DEVF_NATIVE_MAJOR&&s->header.minor_version>=DEVF_NATIVE_MINOR&&s->request_placement&&s->remove_placement;
}
inline DevfNativePlacementRequest field() noexcept {
 DevfNativePlacementRequest r{};r.struct_size=sizeof(r);r.native_profile=510;
 constexpr char id[]="api_probe_field";constexpr char stage[]="F_SP121";
 std::memcpy(r.placement_id,id,sizeof(id));std::memcpy(r.stage,stage,sizeof(stage));r.room=3;r.layer=6;
 r.x=-1250.f;r.y=-6304.00830078125f;r.z=60750.f;r.yaw_s16=0;r.parameters=0xFFFF0100u;return r;
}
struct Probe {
 ModContext* caller=nullptr;const HostService* host=nullptr;const DevfNativePlacementService* imported=nullptr;
 uint64_t token=0;bool attempted=false;const char* reason="PROBE_NOT_STARTED";
 ModResult initialize(ModContext* c,const HostService* h,const DevfNativePlacementService* s) noexcept {
  if(attempted)return MOD_CONFLICT;attempted=true;caller=c;host=h;imported=s;
  if(!c||!h||h->header.struct_size<sizeof(*h)||h->header.major_version!=2||h->header.minor_version<2||!h->get_service||!compatible(s)){reason="PROBE_REQUIRED_SERVICE_INCOMPATIBLE";return MOD_UNSUPPORTED;}
  auto request=field();DevfNativePlacementReceipt receipt{};receipt.struct_size=sizeof(receipt);
  ModResult result=MOD_ERROR;try{result=s->request_placement(c,&request,&receipt);}catch(...){reason="PROBE_SERVICE_EXCEPTION";return MOD_ERROR;}
  if(result!=MOD_OK){reason="PROBE_REQUEST_REJECTED";if(receipt.placement_token){try{s->remove_placement(c,receipt.placement_token);}catch(...){}}return result;}
  if(!receipt.placement_token||receipt.site!=DEVF_SITE_FIELD||receipt.registered_edits!=1||receipt.reserved){
   reason="PROBE_RECEIPT_INVALID";if(receipt.placement_token){try{s->remove_placement(c,receipt.placement_token);}catch(...){}}return MOD_ERROR;
  }
  token=receipt.placement_token;reason="PROBE_REQUEST_ACCEPTED_EDIT_ONLY";return MOD_OK;
 }
 ModResult shutdown() noexcept {
  if(!token)return MOD_OK;const auto mine=token;token=0;
  const void* current=nullptr;
  try{
   // Never dereference a cached pointer into an unavailable/retired provider image.
   if(!host||!host->get_service||host->get_service(caller,DEVF_NATIVE_SERVICE_ID,DEVF_NATIVE_MAJOR,DEVF_NATIVE_MINOR,&current)!=MOD_OK||current!=imported){
    reason="PROBE_PROVIDER_UNAVAILABLE_SKIP_STALE_CALL";return MOD_OK;
   }
   const auto* live=static_cast<const DevfNativePlacementService*>(current);
   if(!compatible(live)){reason="PROBE_PROVIDER_INCOMPATIBLE_SKIP";return MOD_OK;}
   const auto result=live->remove_placement(caller,mine);reason=result==MOD_OK?"PROBE_OWN_EDIT_REMOVED":"PROBE_REMOVE_FAILED_DETACH_FALLBACK";return result;
  }catch(...){reason="PROBE_CLEANUP_EXCEPTION_DETACH_FALLBACK";return MOD_ERROR;}
 }
};
}
