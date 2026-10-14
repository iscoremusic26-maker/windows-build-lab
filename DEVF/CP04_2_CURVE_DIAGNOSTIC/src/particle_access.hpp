#pragma once
#include <d/d_com_inf_game.h>
#include <JSystem/JParticle/JPAEmitterManager.h>
#include <JSystem/JParticle/JPAResourceManager.h>
#include <JSystem/JParticle/JPAResource.h>
#include <JSystem/JParticle/JPAKeyBlock.h>
#include "visual.hpp"
namespace devf {
struct ParticleAccess {
 template<class Control,class Manager>
 static AuditResult inspect_checked(Control* c,Manager* manager)noexcept{
  AuditResult r;auto fail=[&](Gate g)noexcept{r.gate=g;return r;};
  if(!c)return fail(Gate::ControlNull);r.probe_mask|=1;
  if(!manager)return fail(Gate::ManagerNull);r.probe_mask|=2;r.gid_max=manager->gidMax;
  if(!manager->pResMgrAry)return fail(Gate::ArrayNull);r.probe_mask|=4;
  if(manager->ridMax<1||!manager->pResMgrAry[0])return fail(Gate::BankMissing);r.probe_mask|=8;
  auto* bank=manager->pResMgrAry[0];auto* resource=bank->getResource(kAura);
  if(!resource)return fail(Gate::ResourceMissing);r.probe_mask|=16;
  if(resource->getUsrIdx()!=kAura)return fail(Gate::IdentityChanged);
  auto* dyn=resource->getDyn();if(!dyn||!dyn->mpData)return fail(Gate::DynamicsNull);r.probe_mask|=32;
  r.dyn_flags=dyn->getFlag();r.rate_step=dyn->getRateStep();r.fixed_interval=(r.dyn_flags&JPADynFlag_FixedInterval)!=0;
  r.child=resource->getCsp()!=nullptr;r.indirect=resource->getEts()!=nullptr;
  const auto rate_random=dyn->getRateRndm(),life_random=dyn->getLifetimeRndm(),base_rate=dyn->getRate();
  r.random_finite=finite(rate_random)&&finite(life_random);r.base_rate_finite=finite(base_rate);
  if(r.random_finite){r.rate_random=rate_random;r.life_random=life_random;}if(r.base_rate_finite)r.base_rate=base_rate;
  r.base_life=dyn->getLifetime();r.start_frame=dyn->getStartFrame();r.max_frame=dyn->getMaxFrame();
  r.key_count=resource->keyNum;r.work=bank->getResUserWork(kAura);r.probe_mask|=64;
  if(!known_group(r.work,r.group))return fail(Gate::WorkChanged);
  if(r.group>=r.gid_max)return fail(Gate::GroupUnavailable);
  if(r.key_count!=kExpectedKeys)return fail(Gate::KeyCountChanged);
  if(!resource->ppKey)return fail(Gate::KeyArrayNull);
  for(unsigned i=0;i<kExpectedKeys;++i){const auto g=summarize_key(resource->ppKey[i],r.curves[i]);if(g!=Gate::Complete)return fail(g);}
  // No emitter API, resource write, native calc/callback, or retry exists in this adapter.
  return r;
 }
 AuditResult inspect(const VisualProbe&)noexcept{
  auto* c=g_dComIfG_gameInfo.play.getParticle();
  if(!c){AuditResult r;r.gate=Gate::ControlNull;return r;}
  return inspect_checked(c,dPa_control_c::getEmitterManager());
 }
};
}
