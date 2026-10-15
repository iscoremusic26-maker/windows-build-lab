#pragma once
#include "registration.hpp"
#include "config_io.hpp"
#include <cstring>
namespace native_config {
struct Runtime {
 Config config;Error error;native_qa::Registration registration;bool attempted=false,accepted=false;
 ModResult initialize(ModContext* ctx,const HostService* host,const FileService* file,const StageService* stage) noexcept {
  if(attempted){error.code="NO_REINJECTION";return MOD_CONFLICT;}attempted=true;
  if(!ctx||!native_qa::compatible(host,2,2)||!host->version||
     (std::strcmp(host->version,"2.0.3")&&std::strcmp(host->version,"2.0.3.0"))||
     !native_qa::compatible(file,1,0)||!native_qa::compatible(stage,1,0)||!stage->add_actor||!stage->remove_actor_edit){
   error.code="HOST_FILE_STAGE_UNAVAILABLE";return MOD_OK;
  }
  if(!load(ctx,host,file,config,error))return MOD_OK; // stable disarmed, zero fallback
  accepted=true;
  const auto result=registration.initialize(ctx,stage,config.active());
  if(result!=MOD_OK){error.code=registration.reason;accepted=false;}
  return result;
 }
 ModResult update() const noexcept {return MOD_OK;} // no IO/add/retry
 ModResult shutdown(ModContext* ctx,const StageService* stage) noexcept {return registration.cleanup(ctx,stage);}
};
}
