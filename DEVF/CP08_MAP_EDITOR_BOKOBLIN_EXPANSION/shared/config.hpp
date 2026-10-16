#pragma once
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/provider/src/config_core.hpp"
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/shared/devf_encounter_api.h"
#include <cstring>
namespace map_config {
constexpr unsigned capacity=8;
constexpr const char* geometry_sha="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af";
struct Config {std::array<DevfEncounterPlacementRequest,capacity> requests{};unsigned count=0,total=0;};
inline bool decode(std::string_view bytes,Config& out,const char*& reason)noexcept{
 out={};reason="INVALID_MAP_CONFIG";
 try{
  using namespace native_config;Parser p{bytes};Node root;
  if(!p.parse(root)){reason=p.error.code;return false;}
  if(!keys(root,{"document_type","schema_version","geometry_sha256","encounters"})||
     !text(get(root,"document_type"),"devf_map_encounters")||!integer(get(root,"schema_version"),1,1)||
     !text(get(root,"geometry_sha256"),geometry_sha))return false;
  const auto* list=get(root,"encounters");if(list->type!=Node::Array||list->array.size()>capacity){reason="LIMIT_EIGHT";return false;}
  Config draft;std::array<DevfEncounterPlacementRequest,capacity> all{};std::array<bool,capacity> enabled{};
  for(const auto& n:list->array){
   if(!keys(n,{"id","enabled","enemy","stage","room","layer","x","y","z","yaw_s16"})||!id(get(n,"id"))||get(n,"id")->text.size()>31||get(n,"enabled")->type!=Node::Bool||
      !text(get(n,"enemy"),"E_OC2")||!text(get(n,"stage"),"F_SP121")||!integer(get(n,"room"),3,3)||!integer(get(n,"layer"),6,6)||
      !num(get(n,"x"),-2250,-250)||!num(get(n,"y"),-7500,-5000)||!num(get(n,"z"),59000,63000)||!integer(get(n,"yaw_s16"),-32768,32767)){reason="INVALID_ENTRY_OR_REGION";return false;}
   DevfEncounterPlacementRequest r{};r.struct_size=sizeof(r);r.native_profile=510;
   std::memcpy(r.placement_id,get(n,"id")->text.c_str(),get(n,"id")->text.size()+1);std::memcpy(r.stage,"F_SP121",8);r.room=3;r.layer=6;
   r.x=float(get(n,"x")->number);r.y=float(get(n,"y")->number);r.z=float(get(n,"z")->number);r.yaw_s16=int(get(n,"yaw_s16")->number);r.parameters=0xFFFF0100u;
   const bool on=get(n,"enabled")->boolean;
   for(unsigned i=0;i<draft.total;++i){
    const auto& a=all[i];const double dx=double(a.x)-r.x,dz=double(a.z)-r.z;
    if(!std::strcmp(a.placement_id,r.placement_id)||(a.x==r.x&&a.y==r.y&&a.z==r.z)){reason="DUPLICATE_ID_OR_POSITION";return false;}
    if(on&&enabled[i]&&dx*dx+dz*dz<300.*300.){reason="ACTIVE_SPACING_MIN_300";return false;}
   }
   all[draft.total]=r;enabled[draft.total++]=on;if(on)draft.requests[draft.count++]=r;
  }
  out=draft;reason="MAP_CONFIG_VALID_COLD_START";return true;
 }catch(...){out={};reason="CONFIG_EXCEPTION";return false;}
}
}
