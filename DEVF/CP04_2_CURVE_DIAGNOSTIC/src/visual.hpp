#pragma once
#include <cstdint>
#include <limits>
namespace devf {
constexpr std::uint16_t kAura=0x29D;
constexpr unsigned kMaxPoints=255,kExpectedKeys=2;
enum class Gate : unsigned { Complete,ControlNull,ManagerNull,ArrayNull,BankMissing,ResourceMissing,DynamicsNull,IdentityChanged,WorkChanged,GroupUnavailable,KeyCountChanged,KeyArrayNull,KeyNull,HeaderNull,HeaderMagic,HeaderSize,PointsLimit,DataPointer,NonFinite,ScalarRange,TimeOrder };
constexpr const char* gate_reason(Gate g){switch(g){
case Gate::Complete:return "DIAGNOSTIC_CURVES_COMPLETE_NO_EMITTER";
case Gate::ControlNull:return "PARTICLE_CONTROL_NULL";case Gate::ManagerNull:return "EMITTER_MANAGER_NULL";
case Gate::ArrayNull:return "RESOURCE_ARRAY_NULL";case Gate::BankMissing:return "RESOURCE_BANK0_MISSING";
case Gate::ResourceMissing:return "RESOURCE_029D_MISSING";case Gate::DynamicsNull:return "RESOURCE_DYNAMICS_OR_DATA_NULL";
case Gate::IdentityChanged:return "RESOURCE_IDENTITY_CHANGED";case Gate::WorkChanged:return "RESOURCE_USERWORK_CHANGED_NO_CURVE_PROBE";
case Gate::GroupUnavailable:return "NATIVE_GROUP9_UNAVAILABLE";case Gate::KeyCountChanged:return "RESOURCE_TWO_KEY_COUNT_CHANGED";
case Gate::KeyArrayNull:return "RESOURCE_KEY_ARRAY_NULL";case Gate::KeyNull:return "RESOURCE_KEY_NULL";
case Gate::HeaderNull:return "CURVE_HEADER_NULL";case Gate::HeaderMagic:return "CURVE_HEADER_NOT_KFA1";
case Gate::HeaderSize:return "CURVE_BLOCK_SIZE_INVALID";case Gate::PointsLimit:return "CURVE_POINT_LIMIT_EXCEEDED";
case Gate::DataPointer:return "CURVE_DATA_POINTER_MISMATCH";case Gate::NonFinite:return "CURVE_NONFINITE_SCALAR";
case Gate::ScalarRange:return "CURVE_DIAGNOSTIC_SCALAR_RANGE_EXCEEDED";case Gate::TimeOrder:return "CURVE_TIMES_INVALID";
}return "CURVE_GATE_INVALID";}
constexpr bool finite(double v){return v>=-std::numeric_limits<double>::max()&&v<=std::numeric_limits<double>::max();}
// Strictly the measured resource. Unknown flags do not acquire a safe-group classification.
constexpr bool known_group(std::uint32_t work,unsigned& group){
 if(work!=0x12u)return false;
 const unsigned base=work&1?0:work&2?1:work&4?2:work&8?3:0;
 group=(work&0x10)?base+8:base;return true;
}
struct CurveInfo {
 unsigned id=0,points=0,loop=0,block_bytes=0;bool summarized=false;
 double first=0,last=0,value_min=0,value_max=0,hull_min=0,hull_max=0;
 Gate status=Gate::Complete;
};
inline void include_value(double v,double& lo,double& hi){if(v<lo)lo=v;if(v>hi)hi=v;}
// Source-defined KFA1 layout: each point is time,value,incoming slope,outgoing slope.
// This envelope is exact-arithmetic Bezier hull information, NOT a float32 runtime certificate.
template<class Key>Gate summarize_key(const Key* key,CurveInfo& out)noexcept{
 if(!key)return out.status=Gate::KeyNull;
 const auto* bytes=key->mDataStart;if(!bytes)return out.status=Gate::HeaderNull;
 if(bytes[0]!='K'||bytes[1]!='F'||bytes[2]!='A'||bytes[3]!='1')return out.status=Gate::HeaderMagic;
 out.block_bytes=(std::uint32_t(bytes[4])<<24)|(std::uint32_t(bytes[5])<<16)|(std::uint32_t(bytes[6])<<8)|bytes[7];
 if(out.block_bytes<12||out.block_bytes>4096)return out.status=Gate::HeaderSize;
 out.id=key->getID();out.points=key->getKeyNum();out.loop=key->isLoop()?1:0;
 if(!out.points||out.points>kMaxPoints)return out.status=Gate::PointsLimit;
 if(out.block_bytes<12+16*out.points)return out.status=Gate::HeaderSize;
 const auto* data=key->getKeyData();
 if(!data||reinterpret_cast<const void*>(data)!=reinterpret_cast<const void*>(bytes+12))return out.status=Gate::DataPointer;
 double previous_time=0,previous_value=0,previous_out=0;
 for(unsigned i=0;i<out.points;++i){
  const double t=float(data[4*i]),v=float(data[4*i+1]),in=float(data[4*i+2]),slope=float(data[4*i+3]);
  if(!finite(t)||!finite(v)||!finite(in)||!finite(slope))return out.status=Gate::NonFinite;
  if(t<0||t>65535||v<-1e6||v>1e6||in<-1e6||in>1e6||slope<-1e6||slope>1e6)return out.status=Gate::ScalarRange;
  if(i&&t-previous_time<0.0001)return out.status=Gate::TimeOrder;
  if(!i){out.first=t;out.value_min=out.value_max=out.hull_min=out.hull_max=v;}
  else{
   const double dt=t-previous_time;
   include_value(v,out.value_min,out.value_max);include_value(v,out.hull_min,out.hull_max);
   include_value(previous_value+dt*previous_out/3.0,out.hull_min,out.hull_max);
   include_value(v-dt*in/3.0,out.hull_min,out.hull_max);
  }
  out.last=t;previous_time=t;previous_value=v;previous_out=slope;
 }
 out.summarized=true;return out.status=Gate::Complete;
}
struct AuditResult {
 Gate gate=Gate::Complete;unsigned probe_mask=0,work=0,group=0,gid_max=0,dyn_flags=0,key_count=0,rate_step=0;
 bool child=false,indirect=false,fixed_interval=false,random_finite=false,base_rate_finite=false;
 float rate_random=0,life_random=0,base_rate=0;int base_life=0,start_frame=0,max_frame=0;
 CurveInfo curves[kExpectedKeys];
};
struct VisualProbe {std::uint32_t id=0;bool eligible=false,live=false,model_ready=false;float x=0,y=0,z=0;};
struct Visual {
 std::uint32_t id=0,key=0;bool fault=false,once=false;AuditResult snapshot;
 template<class Access,class Emit>void step(const VisualProbe& p,Access& a,Emit emit)noexcept{
  if(!p.eligible||fault||once)return;once=true;id=p.id;
  if(!p.live||!p.model_ready){fault=true;emit("CP04_VISUAL_REJECTED",id,0,"MODEL_OR_ACTOR_NOT_READY_NO_PROBE","NO_EMITTER_NO_EFFECT");return;}
  snapshot=a.inspect(p);fault=true;
  emit("CP04_VISUAL_REJECTED",id,0,gate_reason(snapshot.gate),"CURVE_READ_ONLY_NO_EMITTER_NO_EFFECT");
 }
 template<class Emit>void shutdown(Emit)noexcept{id=key=0;}
};
}
