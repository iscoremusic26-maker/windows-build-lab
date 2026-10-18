#pragma once
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/provider/src/runtime.hpp"
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/shared/devf_native_api.h"
namespace devf_api {
struct Entry {
 ModContext* owner=nullptr;StageActorHandle handle=0;uint64_t token=0;unsigned site=0;
 std::array<char,32> id{};
};
struct Broker {
 ModContext* provider=nullptr;const HostService* host=nullptr;const StageService* stage=nullptr;
 static constexpr unsigned capacity=32;
 std::array<Entry,capacity> entries{};std::array<bool,2> json_active{};
 std::array<StageActorHandle,2> json_handles{};std::array<std::array<char,33>,2> json_ids{};
 uint64_t watch=0,next_token=0;bool alive=false,open=false,ready=false,busy=false,fault=false;
 const char* reason="API_NOT_STARTED";ModResult cleanup_result=MOD_OK;
 using Emit=void(*)(const char*,const char*,const char*,uint64_t,unsigned,ModResult);
 Emit emit=nullptr;ModContext* pending=nullptr;bool canceled=false;
 void event(const char* e,const char* owner,const char* id,uint64_t t,unsigned site,ModResult r) noexcept {
  if(emit){try{emit(e,owner,id,t,site,r);}catch(...){}}
 }
 static bool id_valid(const char* id) noexcept {
  if(!id||!id[0])return false;
  for(unsigned i=0;i<32;++i){const char c=id[i];if(!c)return i>0;
   if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;}return false;
 }
 static bool scene(const char* s) noexcept {static constexpr char expected[8]={'F','_','S','P','1','2','1',0};return std::memcmp(s,expected,8)==0;}
 unsigned external_count() const noexcept {unsigned n=0;for(const auto& e:entries)if(e.handle)++n;return n;}
 bool initialize(ModContext* ctx,const HostService* h,const StageService* s,const native_config::Runtime& core,Emit output=nullptr) noexcept {
  if(alive||watch){reason="API_ALREADY_INITIALIZED";return false;}
  provider=ctx;host=h;stage=s;emit=output;
  if(!ctx||!native_qa::compatible(h,2,2)||!h->mod_id||!h->publish_service||!h->watch_mod_lifecycle||!h->unwatch_mod_lifecycle||
     !native_qa::compatible(s,1,0)||!s->add_actor||!s->remove_actor_edit){reason="API_HOST_STAGE_UNAVAILABLE";return false;}
  json_active=core.config.active();json_handles=core.registration.handles;
  for(unsigned i=0;i<core.config.count;++i){const auto& p=core.config.placements[i];const auto n=p.id.size();if(n>32){reason="JSON_ID_INVALID";return false;}std::memcpy(json_ids[p.north?1:0].data(),p.id.c_str(),n+1);}
  ready=core.accepted;alive=true;
  try{
   const auto r=h->watch_mod_lifecycle(ctx,on_detach,this,&watch);
   if(r!=MOD_OK||!watch){reason="API_WATCH_FAILED";if(watch){try{h->unwatch_mod_lifecycle(ctx,watch);}catch(...){}}watch=0;alive=false;return false;}
  }catch(...){if(watch){try{h->unwatch_mod_lifecycle(ctx,watch);}catch(...){}}watch=0;alive=false;reason="API_WATCH_EXCEPTION";return false;}
  open=true;reason=ready?"API_BOOTSTRAP_OPEN":"API_CONFIG_NOT_ACCEPTED";return true;
 }
 void close_admission() noexcept {if(open){open=false;reason="API_BOOTSTRAP_CLOSED";}}
 bool owned_handle(StageActorHandle h) const noexcept {
  for(auto v:json_handles)if(v&&v==h)return true;
  for(const auto& e:entries)if(e.handle&&e.handle==h)return true;
  return false;
 }
 ModResult erase(Entry& e) noexcept {
  ModResult r=MOD_ERROR;
  try{r=stage->remove_actor_edit(provider,e.handle);}catch(...){}
  if(r==MOD_OK)e=Entry{};else{fault=true;cleanup_result=r;}return r;
 }
 ModResult request(ModContext* caller,const DevfNativePlacementRequest* r,DevfNativePlacementReceipt* out) noexcept {
  if(!out||out->struct_size<sizeof(*out))return MOD_INVALID_ARGUMENT;
  out->site=0;out->placement_token=0;out->registered_edits=0;out->reserved=0;
  if(!alive||!open||!ready||fault){reason="API_ADMISSION_CLOSED_OR_CONFIG_INVALID";return MOD_UNAVAILABLE;}
  if(busy)return MOD_CONFLICT;
  if(!caller||caller==provider||!r||r->struct_size<sizeof(*r)||!id_valid(r->placement_id)||!scene(r->stage)||
     r->native_profile!=510||r->room!=3||r->layer!=6||r->yaw_s16||r->parameters!=0xFFFF0100u||r->reserved)return MOD_INVALID_ARGUMENT;
  unsigned site=2;
  if(r->x==-1250.f&&r->y==native_qa::site1_spawn&&r->z==60750.f)site=0;
  if(r->x==-1250.f&&r->y==native_qa::north_spawn&&r->z==61950.f)site=1;
  if(site==2)return MOD_INVALID_ARGUMENT;
  const char* owner_id=nullptr;
  try{owner_id=host->mod_id(caller);}catch(...){return MOD_INVALID_ARGUMENT;}
  if(!owner_id||!*owner_id)return MOD_INVALID_ARGUMENT;
  if(json_active[site]){reason="API_JSON_SITE_CONFLICT";return MOD_CONFLICT;}
  for(unsigned i=0;i<2;++i){
   if(json_active[i]&&std::strcmp(json_ids[i].data(),r->placement_id)==0)return MOD_CONFLICT;
  }
  for(unsigned i=0;i<capacity;++i){
   const auto& e=entries[i];
   if(e.handle&&(e.site==site||e.owner==caller||std::strcmp(e.id.data(),r->placement_id)==0)){reason="API_DUPLICATE_OR_OWNER_CONFLICT";return MOD_CONFLICT;}
  }
  if(external_count()+unsigned(json_active[0])+unsigned(json_active[1])>=capacity||next_token==UINT64_MAX)return MOD_CONFLICT;
  unsigned slot=0;while(slot<capacity&&entries[slot].handle)++slot;if(slot==capacity)return MOD_CONFLICT;
  const auto bytes=native_qa::record(site==1);StageActorHandle h=0;ModResult result=MOD_ERROR;busy=true;pending=caller;canceled=false;
  try{result=stage->add_actor(provider,native_qa::stage,native_qa::room,native_qa::layer,bytes.data(),bytes.size(),&h);}catch(...){result=MOD_ERROR;}
  pending=nullptr;
  const bool duplicate=h&&owned_handle(h);
  if(result!=MOD_OK||!h||duplicate||canceled){
   reason=canceled?"API_OWNER_DETACHED_DURING_REQUEST":duplicate?"API_STAGE_DUPLICATE_HANDLE":!h?"API_STAGE_FAILED_OR_ZERO_HANDLE":"API_STAGE_FAILED_WITH_HANDLE";
   if(duplicate)fault=true; // cannot delete an alias of an existing JSON/external edit
   if(h&&!duplicate){
    auto& e=entries[slot];e.handle=h;e.site=site; // quarantine own handle if rollback fails
    erase(e);
   }
   busy=false;return canceled?MOD_UNAVAILABLE:result==MOD_OK?MOD_ERROR:result;
  }
  auto& e=entries[slot];e.owner=caller;e.handle=h;e.token=++next_token;e.site=site;
  std::memcpy(e.id.data(),r->placement_id,32);out->site=site;out->placement_token=e.token;out->registered_edits=1;
  reason="API_STAGE_EDIT_REGISTERED_NOT_ACTOR_OBSERVED";
  event("REQUEST_ACCEPTED",owner_id,e.id.data(),e.token,site,MOD_OK);
  if(e.owner!=caller||!e.handle){out->placement_token=0;out->registered_edits=0;busy=false;return MOD_UNAVAILABLE;}
  busy=false;return MOD_OK;
 }
 ModResult remove(ModContext* caller,uint64_t token) noexcept {
  if(!alive||!caller||!token)return MOD_INVALID_ARGUMENT;if(busy)return MOD_CONFLICT;
  for(auto& e:entries)if(e.handle&&e.owner==caller&&e.token==token){
   busy=true;const auto site=e.site;const auto result=erase(e);reason=result==MOD_OK?"API_OWN_EDIT_REMOVED_NOT_LIVE_ACTOR":"API_OWN_EDIT_REMOVE_FAILED";
   event("REMOVE_RESULT","CALLER", "OWN_PLACEMENT",token,site,result);busy=false;return result;
  }
  return MOD_INVALID_ARGUMENT;
 }
 void detach(ModContext* subject) noexcept {
  if(!alive||!subject)return;if(subject==pending){pending=nullptr;canceled=true;}const bool nested=busy;busy=true;
  for(unsigned i=capacity;i>0;--i){auto& e=entries[i-1];if(e.owner!=subject)continue;
   e.owner=nullptr;e.token=0; // never retain/dereference context after detach callback returns
   if(e.handle)erase(e);
  }
  busy=nested;
 }
 static void on_detach(ModContext* ctx,ModContext* subject,const char*,ModLifecycleEvent event,void* p) noexcept {
  auto* b=static_cast<Broker*>(p);if(b&&ctx==b->provider&&event==MOD_LIFECYCLE_DETACHED)b->detach(subject);
 }
 ModResult shutdown() noexcept {
  open=false;alive=false;pending=nullptr;canceled=true;busy=true;ModResult failure=MOD_OK;
  for(unsigned i=capacity;i>0;--i){auto& e=entries[i-1];e.owner=nullptr;e.token=0;if(e.handle){const auto r=erase(e);if(r!=MOD_OK)failure=r;}}
  if(watch){try{if(host->unwatch_mod_lifecycle(provider,watch)!=MOD_OK)failure=MOD_ERROR;}catch(...){failure=MOD_ERROR;}watch=0;}
  busy=false;cleanup_result=failure;return failure;
 }
};
}
