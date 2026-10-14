#pragma once
#include <d/d_com_inf_game.h>
#include <JSystem/JParticle/JPAEmitterManager.h>
#include <JSystem/JParticle/JPAResourceManager.h>
#include <JSystem/JParticle/JPAResource.h>
#include <JSystem/JParticle/JPAKeyBlock.h>
#include <JSystem/JParticle/JPAExTexShape.h>
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
 // Same branch sequence is instantiated with native SDK types in production and
 // bounded test doubles in the lab. No pointers from this attempt escape.
 template<class Control,class Manager>
 static CreateResult create_checked(const VisualProbe& p,Control* c,Manager* manager)noexcept{
  CreateResult result;
  auto fail=[&](ParticleGate gate)noexcept{result.gate=gate;return result;};
  if(!c)return fail(ParticleGate::ControlNull);result.probe_mask|=1;
  if(!manager)return fail(ParticleGate::ManagerNull);result.probe_mask|=2;
  if(!manager->pResMgrAry)return fail(ParticleGate::ArrayNull);result.probe_mask|=4;
  if(manager->ridMax<1)return fail(ParticleGate::BankCountInvalid);
  auto* resources=manager->pResMgrAry[0];if(!resources)return fail(ParticleGate::Bank0Null);result.probe_mask|=8;
  auto* resource=resources->getResource(kAura);
  if(!resource)return fail(ParticleGate::ResourceMissing);result.probe_mask|=16;
  const auto* dynamics=resource->getDyn();
  if(!dynamics)return fail(ParticleGate::DynamicsNull);result.probe_mask|=32;
  auto& safety=result.resource;
  safety.child=resource->getCsp()!=nullptr;safety.indirect=resource->getEts()!=nullptr;
  result.dynamics_flags=dynamics->getFlag();result.rate_step=dynamics->getRateStep();
  result.indirect_active=safety.indirect&&resource->getEts()->isUseIndirect();
  safety.fixed_interval=(result.dynamics_flags&JPADynFlag_FixedInterval)!=0;
  safety.rate_random=dynamics->getRateRndm();safety.life_random=dynamics->getLifetimeRndm();
  result.key_count=resource->keyNum;
  for(unsigned i=0;i<resource->keyNum;++i){
   if(!resource->ppKey)return fail(ParticleGate::KeyTableNull);
   if(!resource->ppKey[i])return fail(ParticleGate::KeyEntryNull);
   if(resource->ppKey[i]->getID()==0||resource->ppKey[i]->getID()==4)safety.rate_or_life_key=true;
  }
  const auto work=resources->getResUserWork(kAura);safety.work=work;result.probe_mask|=64;
  const auto gate=resource_gate(safety);if(gate!=ParticleGate::Ready)return fail(gate);
  const std::uint8_t group=work&1?0:work&2?1:work&4?2:work&8?3:0;
  const JGeometry::TVec3<float> pos(p.x,p.y,p.z);
  auto* emitter=manager->createSimpleEmitterID(pos,kAura,group,0,nullptr,nullptr);
  if(!emitter)return fail(ParticleGate::PoolCreateFailed);
  emitter->setUserWork(owner_stamp(p.id));
  result.key=c->field_0x210.entry(kAura,emitter,nullptr);
  if(!result.key){emitter->stopDrawParticle();return fail(ParticleGate::LevelEntryFailed);}
  return result;
 }
 CreateResult create(const VisualProbe& p)noexcept{
  // Control guard precedes the static manager read. No retries or asynchronous callbacks.
  auto* c=control();if(!c){CreateResult r;r.gate=ParticleGate::ControlNull;return r;}
  return create_checked(p,c,dPa_control_c::getEmitterManager());
 }
 bool refresh(std::uint32_t key,const VisualProbe& p)noexcept{
  auto* c=control();if(!c)return false;auto* r=c->field_0x210.get(key);
  if(!tagged(r,key,p.id))return false;
  auto* e=r->getEmitter();r->onActive();
  e->setGlobalTranslation(p.x,p.y,p.z);
  const JGeometry::TVec3<float> scale(0.6f,0.6f,0.6f);e->setGlobalScale(scale);
  e->setGlobalPrmColor(155,38,230);e->setGlobalEnvColor(45,8,90);e->setGlobalAlpha(144);
  e->becomeContinuousParticle();e->setRate(0.4f);e->setRateStep(0);e->setLifeTime(18);
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
