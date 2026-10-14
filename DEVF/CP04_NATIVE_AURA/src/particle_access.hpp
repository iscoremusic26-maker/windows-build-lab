#pragma once
#include <d/d_com_inf_game.h>
#include <JSystem/JParticle/JPAEmitterManager.h>
#include <JSystem/JParticle/JPAResourceManager.h>
#include <JSystem/JParticle/JPAResource.h>
#include <JSystem/JParticle/JPAKeyBlock.h>
#include "visual.hpp"
namespace devf {
struct ParticleAccess {
 static dPa_control_c* control()noexcept{return g_dComIfG_gameInfo.play.getParticle();}
 static bool tagged(dPa_control_c::level_c::emitter_c* r,std::uint32_t key,std::uint32_t id)noexcept{
  if(!r)return false;
  auto* e=r->getEmitter();
  return r->getId()==key&&r->getNameId()==kAura&&r->getCallback()==nullptr&&e&&e->pRes&&e->pRes->getUsrIdx()==kAura&&
   e->getUserWork()==owner_stamp(id)&&e->getEmitterCallBackPtr()==nullptr&&e->getParticleCallBackPtr()==nullptr;
 }
 std::uint32_t create(const VisualProbe& p)noexcept{
  auto* c=control();auto* manager=dPa_control_c::getEmitterManager();
  if(!c||!manager||!manager->pResMgrAry||manager->ridMax<1)return 0;
  auto* resources=manager->pResMgrAry[0];if(!resources)return 0;
  auto* resource=resources->getResource(kAura);
  // No child emitters, indirect texture block, light/model/trace resource callbacks or overlay group.
  if(!resource||!resource->getDyn())return 0;
  const auto* dynamics=resource->getDyn();
  ResourceProbe safety;safety.child=resource->getCsp()!=nullptr;safety.indirect=resource->getEts()!=nullptr;
  safety.fixed_interval=(dynamics->getFlag()&JPADynFlag_FixedInterval)!=0;safety.rate_random=dynamics->getRateRndm();safety.life_random=dynamics->getLifetimeRndm();
  for(unsigned i=0;i<resource->keyNum;++i){
   if(!resource->ppKey||!resource->ppKey[i])return 0;
   if(resource->ppKey[i]->getID()==0||resource->ppKey[i]->getID()==4)safety.rate_or_life_key=true;
  }
  const auto work=resources->getResUserWork(kAura);
  safety.work=work;if(!resource_safe(safety))return 0;
  const std::uint8_t group=work&1?0:work&2?1:work&4?2:work&8?3:0;
  const JGeometry::TVec3<float> pos(p.x,p.y,p.z);
  // Pool allocates a fresh emitter, reading the shared resource; both callbacks are host-null.
  auto* emitter=manager->createSimpleEmitterID(pos,kAura,group,0,nullptr,nullptr);
  if(!emitter)return 0;
  emitter->setUserWork(owner_stamp(p.id));
  const auto key=c->field_0x210.entry(kAura,emitter,nullptr);
  if(!key){emitter->stopDrawParticle();return 0;} // Native entry already invalidates on pool-table failure.
  return key;
 }
 bool refresh(std::uint32_t key,const VisualProbe& p)noexcept{
  auto* c=control();if(!c)return false;auto* r=c->field_0x210.get(key);
  if(!tagged(r,key,p.id))return false;
  auto* e=r->getEmitter();r->onActive();
  e->setGlobalTranslation(p.x,p.y,p.z);
  const JGeometry::TVec3<float> scale(0.6f,0.6f,0.6f);e->setGlobalScale(scale);
  e->setGlobalPrmColor(155,38,230);e->setGlobalEnvColor(45,8,90);e->setGlobalAlpha(144);
  e->becomeContinuousParticle();e->setRate(0.4f);e->setLifeTime(18);
  GXColor color{};e->getGlobalPrmColor(color);
  return color.r==155&&color.g==38&&color.b==230&&color.a==144&&e->getUserWork()==owner_stamp(p.id);
 }
 int release(std::uint32_t key,std::uint32_t id)noexcept{
  auto* c=control();if(!c)return 0;auto* r=c->field_0x210.get(key);if(!r)return 0;
  if(!tagged(r,key,id))return -1;
  r->getEmitter()->stopDrawParticle(); // Hide surviving particles; pool frees after native lifetime.
  c->field_0x210.cutTable(r); // Unlinks record, invalidates emitter, clears native key, quits immortal.
  return 1;
 }
};
}
