#pragma once
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/provider/src/runtime.hpp"
namespace public_core {
// Empty accepted core preserves the broker bootstrap contract. No file service call,
// legacy placement decoder, native capture, or UI is reachable from this path.
inline ModResult initialize(native_config::Runtime& core,ModContext* ctx,const HostService* host,const StageService* stage) noexcept {
 if(core.attempted){core.error.code="NO_REINJECTION";return MOD_CONFLICT;}
 core.attempted=true;
 if(!ctx||!native_qa::compatible(host,2,2)||!host->version||
    (std::strcmp(host->version,"2.0.3")&&std::strcmp(host->version,"2.0.3.0"))||
    !native_qa::compatible(stage,1,0)||!stage->add_actor||!stage->remove_actor_edit){
  core.error.code="PUBLIC_HOST_STAGE_UNAVAILABLE";return MOD_UNSUPPORTED;
 }
 core.config={};core.accepted=false;
 const auto result=core.registration.initialize(ctx,stage,core.config.active());
 core.accepted=result==MOD_OK;core.error.code=core.accepted?"PUBLIC_EMPTY_CORE_READY":core.registration.reason;
 return result;
}
}
