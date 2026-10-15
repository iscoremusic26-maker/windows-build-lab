#pragma once
#include <array>
#include <cstdint>
#include <cmath>
namespace devf_ai {
constexpr float kNative=5000.f,kEnter=1200.f,kExit=1400.f,kHome=500.f;
constexpr unsigned kGrace=30;
enum class Phase { Guard, Interest, ReturnNative };
struct View {
 std::uint32_t id=0;std::uint64_t generation=0,epoch=0;std::uintptr_t key=0;
 bool identity=false,context=false,eoc2=false,created=false,event=false,hit=false;
 int action=-1,health=0;float range=0,move=0,player2=0,home2=0;
};
inline bool safe_action(int a){return a==0||a==1||a==3;}
struct Frame {bool selected=false;std::uintptr_t key=0;std::uint32_t id=0;std::uint64_t generation=0,epoch=0;float original=0,applied=0;int health=0;};
struct Pilot {
 std::uint32_t id=0;std::uint64_t generation=0,epoch=0;Phase phase=Phase::Guard;
 unsigned outside=0,depth=0,overflow=0;bool fault=false;const char* reason="NONE";
 std::array<Frame,8> frames{};
 void reject(const char* why){fault=true;reason=why;}
 void admit(std::uint32_t actor,std::uint64_t gen){
  if(depth){reject("ADMISSION_WITH_PENDING_CALLBACK");return;}
  id=actor;generation=gen;++epoch;phase=Phase::Guard;outside=0;
 }
 bool push(){
  if(depth==frames.size()){++overflow;reject("NESTED_STACK_LIMIT");return false;}
  frames[depth++]=Frame{};return true;
 }
 bool selected()const{for(unsigned i=0;i<depth;++i)if(frames[i].selected)return true;return false;}
 bool eligible(const View& v)const{
  return !fault&&id&&v.id==id&&v.generation==generation&&v.epoch==epoch&&v.key&&
   v.identity&&v.context&&v.eoc2&&v.created;
 }
 bool begin(const View& v,float& value){
  if(!v.context&&v.id==id){phase=Phase::ReturnNative;outside=0;}
  if(!depth||overflow||!eligible(v))return false;
  if(selected()){reject("NESTED_SELECTED_EXECUTE");return false;}
  if(!std::isfinite(v.player2)||!std::isfinite(v.home2)||v.player2<0||v.home2<0){reject("NONFINITE_DISTANCE");return false;}
  if(v.range!=kNative||v.move!=600.f){reject("NATIVE_PARAMETER_CONTRACT");return false;}
  if(v.event||v.hit||!safe_action(v.action)||v.health<=0){
   phase=Phase::ReturnNative;outside=0;reason=v.event?"EVENT_NATIVE_PRIORITY":v.hit?"HIT_NATIVE_PRIORITY":"ACTION_NATIVE_PRIORITY";return false;
  }
  if(phase==Phase::ReturnNative&&v.home2<kHome*kHome&&v.player2>=kEnter*kEnter){
   phase=Phase::Guard;outside=0;reason="HOME_GUARD_REARM";
  }
  if(phase==Phase::Interest){
   if(v.player2>=kExit*kExit){if(outside<kGrace)++outside;}
   else outside=0;
   if(outside==kGrace){phase=Phase::ReturnNative;reason="EXIT_RADIUS_GRACE";}
  }
  // Do not force action: perception remains subject to native FOV/LOS/sound/peer logic.
  value=kEnter;
  if(phase==Phase::Interest)value=outside?kNative:kExit;
  auto& f=frames[depth-1];f={true,v.key,v.id,v.generation,v.epoch,v.range,value,v.health};
  return true;
 }
 enum class End { Ignored, Restore, Reject };
 End end(const View& v,float& original){
  if(overflow){--overflow;return End::Ignored;}
  if(!depth){reject("POST_WITHOUT_PRE");return End::Reject;}
  auto f=frames[depth-1];
  if(!f.selected){frames[--depth]=Frame{};return End::Ignored;}
  if(v.key!=f.key||v.id!=f.id||v.generation!=f.generation||v.epoch!=f.epoch||!v.identity||!v.eoc2){
   reject("POST_IDENTITY_MISMATCH");return End::Reject;
  }
  if(v.range!=f.applied){reject("SCOPED_RANGE_CHANGED_BY_OTHER_WRITER");return End::Reject;}
  frames[--depth]=Frame{};
  original=f.original;
  if(v.health<f.health||v.health<=0||v.action==5||v.action==6||v.action>=9){
   phase=Phase::ReturnNative;outside=0;reason="DAMAGE_DEATH_OR_NATIVE_STATE";
  }else if(phase==Phase::Guard&&v.action==3&&v.player2<kEnter*kEnter){
   phase=Phase::Interest;outside=0;reason="NATIVE_FIND_OBSERVED";
  }
  return End::Restore;
 }
 void released(){id=0;phase=Phase::Guard;outside=0;}
};
}
