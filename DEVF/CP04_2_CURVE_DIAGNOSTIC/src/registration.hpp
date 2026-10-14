#pragma once
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/stage.h>
#include <mods/svc/hook.h>
#include <d/d_stage.h>
#include <cstring>
#include "policy.hpp"
namespace devf {
template<class T>bool compatible(const T* p,unsigned major,unsigned minor){
 return p&&p->header.struct_size>=sizeof(T)&&p->header.major_version==major&&p->header.minor_version>=minor;
}
using InstallPre=ModResult(*)(HookPreFn);
using InstallPost=ModResult(*)(HookPostFn);
using RemoveHook=ModResult(*)();
inline const stage_actor_data_class& stage_record(){
 static const stage_actor_data_class r{.name="E_oc",.base={.parameters=0xFFFF0100u,
 .position={kX,kY,kZ},.angle={0x0106,0,0},.setID=0xFFFF}};
 static_assert(sizeof(r)==32);return r;
}
struct Registration {
 bool attempted=false,ready=false; StageActorHandle edit=0; ModResult rollback=MOD_OK;
 ModResult initialize(ModContext* ctx,const HostService* host,const LogService* log,
  const StageService* stage,const HookService* hook,InstallPre install,InstallPost install_post,RemoveHook remove,HookPreFn callback,HookPostFn post_callback){
  if(attempted)return MOD_CONFLICT;
  attempted=true;
  if(!ctx||!compatible(host,2,2)||!host->version||
   (std::strcmp(host->version,"2.0.3")&&std::strcmp(host->version,"2.0.3.0"))||
   !compatible(log,1,0)||!log->write||!compatible(stage,1,0)||!stage->add_actor||
   !compatible(hook,1,1)||!hook->install||!hook->add_pre||!hook->add_post||!hook->dispatch_pre||
   !hook->dispatch_post||!hook->uninstall||!install||!install_post||!remove||!callback||!post_callback)return MOD_UNSUPPORTED;
  ModResult result=MOD_ERROR;
  try {result=install(callback);}catch(...){return MOD_ERROR;}
  if(result!=MOD_OK){rollback=remove();return result;}
  try {result=install_post(post_callback);}catch(...){rollback=remove();return MOD_ERROR;}
  if(result!=MOD_OK){rollback=remove();return result;}
  try {const auto& r=stage_record();result=stage->add_actor(ctx,"F_SP121",3,6,&r,sizeof(r),&edit);}
  catch(...){rollback=remove();return MOD_ERROR;}
  if(result!=MOD_OK||edit==0){rollback=remove();return result==MOD_OK?MOD_ERROR:result;}
  ready=true;return MOD_OK;
 }
};
}
