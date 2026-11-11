#pragma once
#include <cstdint>
#include "config_core.hpp"
namespace devf {
constexpr std::int16_t kHp=150;
constexpr float kX=-1250.f,kY=-6304.0083f,kZ=60750.f;
struct Snapshot {
 std::uint32_t id=0,parameters=0; int profile=0,room=-1,layer=-1,set_id=0;
 float x=0,y=0,z=0; int home_x=0,home_y=0,home_z=0;
 bool stage=false,initializing=false,resource_eoc2=false,previous_live=false;
 std::int16_t health=0;
};
inline bool record_matches(const Snapshot& s) {
 // Exact float32 equality: zero tolerance, no spatial/proximity selection.
 const auto* placement=cp6::active();
 return placement&&s.profile==510&&s.room==3&&s.parameters==0xFFFF0100u&&s.set_id==0xFFFF&&
  s.x==placement->x&&s.y==placement->y&&s.z==placement->z&&s.home_x==0x0106&&s.home_y==placement->yaw&&s.home_z==0;
}
inline bool context_matches(const Snapshot& s){return s.stage&&s.layer==6;}
enum class Decision { Ignore,Applied,Fault };
struct Policy {
 bool armed=false,fault=false; std::uint32_t last_id=0; const char* reason="NONE";
 Decision inspect(const Snapshot& s) {
  if(!armed||fault||!record_matches(s)||!context_matches(s))return Decision::Ignore;
  // A regular execute of an already processed generation never writes health.
  if(s.id==last_id&&!s.initializing)return Decision::Ignore;
  if(!s.id||s.id==0xFFFFFFFFu)return reject("INVALID_ID");
  if(!s.initializing)return reject("FIRST_EXECUTE_NOT_OBSERVED");
  if(s.id<=last_id)return reject("REUSED_OR_REENTERED_ID");
  if(s.previous_live)return reject("AMBIGUOUS_CONCURRENT_INSTANCE");
  if(!s.resource_eoc2||s.health!=220)return reject("INITIAL_HP_OR_RESOURCE_MISMATCH");
  last_id=s.id; // Before the single write: reentry cannot authorize a second assignment.
  return Decision::Applied;
 }
 Decision reject(const char* why){reason=why;fault=true;armed=false;return Decision::Fault;}
};
}
