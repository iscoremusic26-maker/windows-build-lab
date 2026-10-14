#pragma once
#include <d/d_com_inf_game.h>
#include <JSystem/JParticle/JPAEmitterManager.h>
#include <JSystem/JParticle/JPAResourceManager.h>
#include <JSystem/JParticle/JPAResource.h>
#include <JSystem/JParticle/JPABaseShape.h>
#include <JSystem/JParticle/JPAExtraShape.h>
#include "visual.hpp"
namespace devf {
struct ParticleAccess {
 unsigned approved_group=255;
 static dPa_control_c* control()noexcept{return g_dComIfG_gameInfo.play.getParticle();}
 template<class Extra>static bool scale_safe(const Extra* e)noexcept{
  if(!e||!e->isEnableScaleAnm())return true;
  const float in=e->getScaleInTiming(),out=e->getScaleOutTiming();
  if(!in_range(in,0,1)||!in_range(out,in,1)||!in_range(e->getScaleRndm(),0,1))return false;
  const float vi[]={e->getScaleInValueX(),e->getScaleInValueY()},vo[]={e->getScaleOutValueX(),e->getScaleOutValueY()};
  const float si[]={e->getScaleIncRateX(),e->getScaleIncRateY()},so[]={e->getScaleDecRateX(),e->getScaleDecRateY()};
  for(unsigned i=0;i<2;++i){
   if(!in_range(vi[i],0,2)||!in_range(vo[i],0,2)||!in_range(si[i],-64,64)||!in_range(so[i],-64,64))return false;
   // Check cached native piecewise linear endpoints, not a promised resource rebuild.
   const double end_in=double(si[i])*in+vi[i],end_out=double(so[i])*(1.0-out)+1;
   if(end_in<0||end_in>2||end_out<0||end_out>2)return false;
  }
  const unsigned tx=e->getScaleAnmTypeX(),ty=e->getScaleAnmTypeY();
  return tx<=2&&ty<=2&&(!tx||e->getScaleAnmCycleX()>0)&&(!ty||e->getScaleAnmCycleY()>0);
 }
 template<class Control,class Manager>static CreateResult create_checked(const VisualProbe& p,Control* c,Manager* m)noexcept{
  CreateResult r;auto fail=[&](Gate gate){r.gate=gate;return r;};
  if(!c)return fail(Gate::ControlNull);r.probe_mask|=1;
  if(!m)return fail(Gate::ManagerNull);r.probe_mask|=2;r.gid_max=m->gidMax;
  if(!m->pResMgrAry)return fail(Gate::ArrayNull);r.probe_mask|=4;
  if(m->ridMax<=kBank||!m->pResMgrAry[kBank])return fail(Gate::BankMissing);r.probe_mask|=8;
  auto* bank=m->pResMgrAry[kBank];auto* res=bank->getResource(kAura);
  if(!res)return fail(Gate::ResourceMissing);r.probe_mask|=16;
  if(res->getUsrIdx()!=kAura)return fail(Gate::IdentityChanged);
  const auto* dyn=res->getDyn();if(!dyn||!dyn->mpData)return fail(Gate::DynamicsNull);r.probe_mask|=32;
  auto* shape=res->getBsp();if(!shape||!shape->pBsd)return fail(Gate::ShapeNull);
  r.work=bank->getResUserWork(kAura);r.probe_mask|=64;
  if(!native_group(r.work,r.group))return fail(Gate::GroupUnsafe);
  if(r.group>=m->gidMax)return fail(Gate::GroupUnavailable);
  r.dyn_flags=dyn->getFlag();r.child=res->getCsp()!=nullptr;r.indirect=res->getEts()!=nullptr;
  r.key_count=res->keyNum;r.fields=res->fldNum;r.rate_random=dyn->getRateRndm();r.life_random=dyn->getLifetimeRndm();
  if(r.child)return fail(Gate::ChildUnsafe);
  if(r.indirect)return fail(Gate::IndirectUnsafe);
  if(r.dyn_flags&JPADynFlag_FixedInterval)return fail(Gate::FixedIntervalUnsafe);
  if(!in_range(r.rate_random,0,1)||!in_range(r.life_random,0,1))return fail(Gate::RandomUnsafe);
  if(dyn->getStartFrame()!=0)return fail(Gate::StartFrameUnsafe);
  if(dyn->getVolumeType()>6||!dyn->mpCalcVolumeFunc)return fail(Gate::VolumeTypeUnsafe);
  // ALL KFA curves rejected by count, including malformed/extreme/looped RATE/LIFE/VOLUME.
  // Never traverse a possibly malformed key table; do not mutate curves or reset native ticks.
  if(r.key_count)return fail(Gate::KeysUnsupported);
  if(r.fields>4)return fail(Gate::FieldLimit);
  for(unsigned i=0;i<r.fields;++i)if(!res->ppFld||!res->ppFld[i])return fail(Gate::FieldNull);
  if(shape->isNoDrawParent()||!in_range(shape->getBaseSizeX(),0.001f,128)||!in_range(shape->getBaseSizeY(),0.001f,128)||
     (shape->getType()!=2&&shape->getType()!=9&&shape->getType()!=10))return fail(Gate::ShapeUnsafe);
  if(!scale_safe(res->getEsp()))return fail(Gate::ScaleUnsafe);
  // Only dependencies of the selected resource, at most 8; no bank census.
  if(!res->texNum||res->texNum>8||!res->mpTDB1||!bank->pTexAry)return fail(Gate::TextureUnsafe);
  for(unsigned i=0;i<res->texNum;++i){
   const unsigned idx=res->getTexIdx(i);
   if(idx>=bank->texRegNum||!bank->pTexAry[idx]||!bank->pTexAry[idx]->mpData)return fail(Gate::TextureUnsafe);
  }
  auto* e=m->createSimpleEmitterID(JGeometry::TVec3<float>(p.x,p.y,p.z),kAura,r.group,kBank,nullptr,nullptr);
  if(!e)return fail(Gate::PoolFailed);
  e->setUserWork(owner_stamp(p.id));r.key=c->field_0x210.entry(kAura,e,nullptr);
  if(!r.key){e->stopDrawParticle();return fail(Gate::EntryFailed);}return r;
 }
 CreateResult create(const VisualProbe& p)noexcept{
  auto* c=control();if(!c){CreateResult r;r.gate=Gate::ControlNull;return r;}
  auto r=create_checked(p,c,dPa_control_c::getEmitterManager());approved_group=r.group;return r;
 }
 template<class Record>static bool tagged(Record* r,std::uint32_t key,std::uint32_t id,unsigned group)noexcept{
  if(!r||r->getId()!=key||r->getNameId()!=kAura||r->getCallback()!=nullptr)return false;
  auto* e=r->getEmitter();
  // BEFORE any pRes dereference: scene clear can leave this pointer stale in the resident pool.
  if(!e||e->checkStatus(0x300)||e->getUserWork()!=owner_stamp(id)||
     e->getResourceManagerID()!=kBank||e->getGroupID()!=group||
     e->getEmitterCallBackPtr()!=nullptr||e->getParticleCallBackPtr()!=nullptr)return false;
  return e->pRes&&e->pRes->getUsrIdx()==kAura;
 }
 template<class Control>static bool refresh_checked(Control* c,std::uint32_t key,const VisualProbe& p,unsigned group)noexcept{
  if(!c)return false;auto* r=c->field_0x210.get(key);if(!tagged(r,key,p.id,group))return false;
  auto* e=r->getEmitter();if(e->getParticleNumber()>kParticleBudget)return false;
  r->onActive();e->playCalcEmitter();e->playCreateParticle();
  e->setGlobalTranslation(p.x,p.y,p.z);e->setGlobalScale(JGeometry::TVec3<float>(0.6f,0.6f,0.6f));
  e->setLocalTranslation(JGeometry::TVec3<float>(0,0,0));e->setLocalScale(JGeometry::TVec3<float>(1,1,1));
  e->setVolumeSize(20);e->setVolumeMiniRadius(0);
  e->setGlobalPrmColor(155,38,230);e->setGlobalEnvColor(45,8,90);e->setGlobalAlpha(144);
  e->becomeContinuousParticle();e->setRate(0.4f);e->setRateStep(0);e->setLifeTime(18);e->setDrawTimes(1);
  GXColor color{};e->getGlobalPrmColor(color);
  return color.r==155&&color.g==38&&color.b==230&&color.a==144&&e->getRate()==0.4f&&
   e->mLifeTime==18&&e->mRateStep==0&&e->mMaxFrame==0&&e->getDrawTimes()==1&&
   e->mGlobalTrs.x==p.x&&e->mGlobalTrs.y==p.y&&e->mGlobalTrs.z==p.z&&e->getUserWork()==owner_stamp(p.id);
 }
 bool refresh(std::uint32_t key,const VisualProbe& p)noexcept{return refresh_checked(control(),key,p,approved_group);}
 template<class Control>static int release_checked(Control* c,std::uint32_t key,std::uint32_t id,unsigned group)noexcept{
  if(!c)return 0;auto* r=c->field_0x210.get(key);if(!r)return 0;
  if(!tagged(r,key,id,group))return -1;
  r->getEmitter()->stopDrawParticle();c->field_0x210.cutTable(r);return 1;
 }
 int release(std::uint32_t key,std::uint32_t id)noexcept{return release_checked(control(),key,id,approved_group);}
};
}
