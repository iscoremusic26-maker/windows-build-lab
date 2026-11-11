#pragma once
#include "broker.hpp"
#include "../../shared/devf_encounter_api.h"
namespace devf_api {
struct EncounterBroker:Broker {
 struct Position{uint64_t token=0;float x=0,y=0,z=0;};
 std::array<Position,2> positions{};
 using EmitEncounter=void(*)(const char*,const char*,const char*,const DevfEncounterPlacementRequest*,uint64_t,ModResult) noexcept;
 EmitEncounter emit_encounter=nullptr;
 // High byte 0x01 keeps E_OC2; low byte 0x32 selects finite native range 50 * 100.
 static constexpr uint16_t encounter_pitch_range_5000=0x0132;
 static native_qa::Record record(const DevfEncounterPlacementRequest& r) noexcept {
  auto b=native_qa::record(false);
  native_qa::be32(b,12,std::bit_cast<uint32_t>(r.x));native_qa::be32(b,16,std::bit_cast<uint32_t>(r.y));native_qa::be32(b,20,std::bit_cast<uint32_t>(r.z));
  native_qa::be16(b,24,encounter_pitch_range_5000);
  native_qa::be16(b,26,static_cast<uint16_t>(r.yaw_s16));return b;
 }
 bool matches(unsigned i,float x,float y,float z)const noexcept{
  const auto& e=entries[i];if(!e.handle)return false;
  if(e.site<2)return x==-1250.f&&y==(e.site?native_qa::north_spawn:native_qa::site1_spawn)&&z==(e.site?61950.f:60750.f);
  const auto& p=positions[i];return p.token==e.token&&p.x==x&&p.y==y&&p.z==z;
 }
 ModResult request(ModContext* caller,const DevfNativePlacementRequest* r,DevfNativePlacementReceipt* out)noexcept{
  if(alive&&open&&ready&&!fault&&!busy&&r&&r->struct_size>=sizeof(*r)&&out&&out->struct_size>=sizeof(*out)){
   const bool fixed=r->x==-1250.f&&((r->y==native_qa::site1_spawn&&r->z==60750.f)||(r->y==native_qa::north_spawn&&r->z==61950.f));
   if(fixed)for(unsigned i=0;i<2;++i)if(entries[i].site==2&&matches(i,r->x,r->y,r->z)){
    out->site=0;out->placement_token=0;out->registered_edits=0;out->reserved=0;reason="LEGACY_REQUEST_CONFLICTS_ENCOUNTER_POSITION";return MOD_CONFLICT;
   }
  }
  return Broker::request(caller,r,out);
 }
 ModResult request_encounter(ModContext* caller,const DevfEncounterPlacementRequest* r,DevfEncounterPlacementReceipt* out) noexcept {
  if(!out||out->struct_size<sizeof(*out))return MOD_INVALID_ARGUMENT;
  out->flags=0;out->placement_token=0;out->registered_edits=0;out->reserved=0;
  if(!alive||!open||!ready||fault){reason="ENCOUNTER_ADMISSION_CLOSED_OR_CONFIG_INVALID";return MOD_UNAVAILABLE;}
  if(busy){reason="ENCOUNTER_BUSY";return MOD_CONFLICT;}
  if(!caller||caller==provider||!r||r->struct_size<sizeof(*r)||!id_valid(r->placement_id)||!scene(r->stage)||r->native_profile!=510||r->room!=3||r->layer!=6||r->yaw_s16< -32768||r->yaw_s16>32767||r->parameters!=0xFFFF0100u||r->reserved||
     !std::isfinite(r->x)||!std::isfinite(r->y)||!std::isfinite(r->z)||std::abs(r->x)>1000000.f||std::abs(r->y)>1000000.f||std::abs(r->z)>1000000.f){reason="ENCOUNTER_INVALID_SCENE_NATIVE_RECORD_OR_BOUNDS";return MOD_INVALID_ARGUMENT;}
  const char* owner=nullptr;try{owner=host->mod_id(caller);}catch(...){reason="ENCOUNTER_OWNER_LOOKUP_EXCEPTION";return MOD_INVALID_ARGUMENT;}
  if(!owner||!*owner){reason="ENCOUNTER_UNKNOWN_OWNER";return MOD_INVALID_ARGUMENT;}
  for(unsigned i=0;i<2;++i){
   if(json_active[i]&&(std::strcmp(json_ids[i].data(),r->placement_id)==0||(r->x==-1250.f&&r->y==(i?native_qa::north_spawn:native_qa::site1_spawn)&&r->z==(i?61950.f:60750.f)))){reason="ENCOUNTER_LEGACY_JSON_CONFLICT";return MOD_CONFLICT;}
   if(entries[i].handle&&(std::strcmp(entries[i].id.data(),r->placement_id)==0||matches(i,r->x,r->y,r->z))){reason="ENCOUNTER_DUPLICATE_ID_OR_POSITION";return MOD_CONFLICT;}
  }
  if(external_count()+unsigned(json_active[0])+unsigned(json_active[1])>=2||next_token==UINT64_MAX){reason="ENCOUNTER_GLOBAL_CAPACITY_TWO";return MOD_CONFLICT;}
  unsigned slot=0;while(slot<2&&entries[slot].handle)++slot;if(slot==2)return MOD_CONFLICT;
  const auto bytes=record(*r);StageActorHandle h=0;ModResult result=MOD_ERROR;busy=true;pending=caller;canceled=false;
  try{result=stage->add_actor(provider,native_qa::stage,native_qa::room,native_qa::layer,bytes.data(),bytes.size(),&h);}catch(...){result=MOD_ERROR;}
  pending=nullptr;const bool duplicate=h&&owned_handle(h);
  if(result!=MOD_OK||!h||duplicate||canceled){
   reason=canceled?"ENCOUNTER_OWNER_DETACHED_DURING_ADD":duplicate?"ENCOUNTER_BACKEND_HANDLE_ALIAS":!h?"ENCOUNTER_ADD_FAILED_OR_ZERO_HANDLE":"ENCOUNTER_ADD_FAILED_WITH_HANDLE";
   if(duplicate)fault=true;
   if(h&&!duplicate){auto& e=entries[slot];e.handle=h;e.site=2;erase(e);}
   busy=false;return canceled?MOD_UNAVAILABLE:result==MOD_OK?MOD_ERROR:result;
  }
  auto& e=entries[slot];e.owner=caller;e.handle=h;e.token=++next_token;e.site=2;std::memcpy(e.id.data(),r->placement_id,32);
  positions[slot]={e.token,r->x,r->y,r->z};out->flags=DEVF_ENCOUNTER_EDIT_REGISTERED;out->placement_token=e.token;out->registered_edits=1;
  reason="ENCOUNTER_NATIVE_EDIT_REGISTERED_NOT_ACTOR";
  if(emit_encounter)emit_encounter("REQUEST_ACCEPTED",owner,e.id.data(),r,e.token,MOD_OK);
  if(e.owner!=caller||!e.handle){out->flags=0;out->placement_token=0;out->registered_edits=0;busy=false;return MOD_UNAVAILABLE;}
  busy=false;return MOD_OK;
 }
};
}
