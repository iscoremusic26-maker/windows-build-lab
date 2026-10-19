#pragma once
// CP10.2 pure policy (no game headers): which actor is a configured Black Bokoblin, and when its one-shot HP write is allowed.
// Derived from the human-observed CP03/CP04.4 rules (exact record match, first-execute only, monotonic ids, no ambiguity),
// generalized to up to 8 registrations. Every decision is per registration: one rejection never disables the others.
#include <array>
#include <cstdint>
#include <cstring>
namespace devf_black2 {
constexpr unsigned capacity=32;
constexpr std::uint32_t kParameters=0xFFFF0100u;
constexpr int kRangeCode=0x0132;          // angle.x written by the encounter broker (native E_OC2 range)
constexpr std::int16_t kNativeHp=220;     // observed initial E_OC2 health (CP03)
struct Snapshot {
 std::uint32_t id=0,parameters=0;int profile=0,room=-1,layer=-1,set_id=0;
 float x=0,y=0,z=0;int home_x=0,home_y=0,home_z=0;
 bool stage=false,initializing=false,resource_eoc2=false,previous_live=false;std::int16_t health=0;
};
struct Reg {
 char id[32]{};int room=-1,layer=-1,yaw=0;float x=0,y=0,z=0;std::int16_t hp=0;
 std::uint32_t last_id=0,admitted_id=0;std::uint64_t generation=0,ready_generation=0,applied_generation=0;
 bool used=false,fault=false,fault_logged=false;const char* reason="NONE";
};
enum class Decision {Ignore,Applied,Fault};
struct Table {
 std::array<Reg,capacity> regs{};unsigned count=0;bool armed=false;std::uint64_t next_generation=0;
 bool add(const char* id,int room,int layer,float x,float y,float z,int yaw,int hp) noexcept {
  if(count>=capacity||!id||!id[0]||hp<1||hp>999)return false;
  for(unsigned i=0;i<count;++i)if(regs[i].room==room&&regs[i].layer==layer&&regs[i].x==x&&regs[i].y==y&&regs[i].z==z)return false;
  Reg& r=regs[count++];r={};for(unsigned i=0;i+1<sizeof(r.id)&&id[i];++i)r.id[i]=id[i]; // no CRT strncpy: keeps the reviewed import surface unchanged
  r.room=room;r.layer=layer;r.x=x;r.y=y;r.z=z;r.yaw=yaw;r.hp=static_cast<std::int16_t>(hp);r.used=true;return true;
 }
 // Exact float32 equality, zero tolerance, no spatial/proximity selection.
 static bool record_matches(const Reg& r,const Snapshot& s) noexcept {
  return s.profile==510&&s.room==r.room&&s.parameters==kParameters&&s.set_id==0xFFFF&&s.x==r.x&&s.y==r.y&&s.z==r.z&&
   s.home_x==kRangeCode&&s.home_y==r.yaw&&s.home_z==0;
 }
 static bool context_matches(const Reg& r,const Snapshot& s) noexcept {return s.stage&&s.layer==r.layer;}
 bool matches(unsigned i,const Snapshot& s) const noexcept {return i<count&&regs[i].used&&record_matches(regs[i],s)&&context_matches(regs[i],s);}
 int find(const Snapshot& s) const noexcept {
  for(unsigned i=0;i<count;++i)if(regs[i].used&&!regs[i].fault&&matches(i,s))return static_cast<int>(i);
  return -1;
 }
 bool any_admitted() const noexcept {for(unsigned i=0;i<count;++i)if(regs[i].used&&!regs[i].fault&&regs[i].admitted_id)return true;return false;}
 Decision reject(Reg& r,const char* why) noexcept {r.reason=why;r.fault=true;r.admitted_id=0;return Decision::Fault;}
 Decision apply(unsigned i,const Snapshot& s) noexcept {
  if(!armed||i>=count)return Decision::Ignore;
  Reg& r=regs[i];
  if(!r.used||r.fault||!matches(i,s))return Decision::Ignore;
  if(s.id==r.last_id&&!s.initializing)return Decision::Ignore;       // regular execute of an already processed generation
  if(!s.id||s.id==0xFFFFFFFFu)return reject(r,"INVALID_ID");
  if(!s.initializing)return reject(r,"FIRST_EXECUTE_NOT_OBSERVED");
  if(s.id<=r.last_id)return reject(r,"REUSED_OR_REENTERED_ID");
  if(s.previous_live)return reject(r,"AMBIGUOUS_CONCURRENT_INSTANCE");
  if(!s.resource_eoc2||s.health!=kNativeHp)return reject(r,"INITIAL_HP_OR_RESOURCE_MISMATCH");
  r.last_id=s.id;r.admitted_id=s.id;r.generation=++next_generation; // before the single write
  return Decision::Applied;
 }
};
}
