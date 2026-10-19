#pragma once
#include "../../CP08_MAP_EDITOR_BOKOBLIN_EXPANSION/shared/config.hpp"
// Schema2 is an explicit local QA authorization, NOT a signed terrain certificate.
// Runtime validates metadata/identity/bounds; only the editor reads and checks KCL.
namespace multi_config {
constexpr unsigned capacity=32;
using namespace native_config;
// CP10.2: optional per-encounter Bokoblin variant. Arrays are aligned with `requests` (enabled encounters only).
constexpr std::uint8_t variant_normal=0,variant_black=1,variant_blue=2,variant_red=3;constexpr int hp_min=1,hp_max=999;
struct Config:map_config::Config{std::array<std::uint8_t,capacity> variant{};std::array<std::int16_t,capacity> hp{};};
// Required keys exactly once + listed optional keys at most once (the parser already rejects duplicate keys).
inline bool keys_optional(const Node& n,std::initializer_list<std::string_view> required,std::initializer_list<std::string_view> optional){
 if(n.type!=Node::Object)return false;
 for(auto k:required)if(!get(n,k))return false;
 for(const auto& x:n.object){bool known=false;for(auto k:required)known=known||x.first==k;for(auto k:optional)known=known||x.first==k;if(!known)return false;}
 return true;
}
inline bool hash(const Node* n){if(!n||n->type!=Node::String||n->text.size()!=64)return false;for(char c:n->text)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;return n->text!=std::string(64,'0');}
inline bool decode(std::string_view raw,Config& out,const char*& why)noexcept{
 out={};why="MULTIROOM_INVALID";try{
 Parser parser{raw};Node root;if(!parser.parse(root))return false;
 if(integer(get(root,"schema_version"),1,1))return map_config::decode(raw,static_cast<map_config::Config&>(out),why);
 if(!keys(root,{"document_type","schema_version","disc_id","revision","resources","encounters"})||!text(get(root,"document_type"),"devf_map_encounters")||!integer(get(root,"schema_version"),2,2)||!text(get(root,"disc_id"),"GZ2E01")||!integer(get(root,"revision"),0,0))return false;
 const auto* rs=get(root,"resources");const auto* es=get(root,"encounters");if(rs->type!=Node::Array||rs->array.size()>64||es->type!=Node::Array||es->array.size()>capacity)return false;
 struct Resource{std::string key,hash,variant;int room;std::array<double,6> bounds;};std::vector<Resource> resources;
 for(const auto& r:rs->array){
 if(!keys(r,{"key","stage","room","variant","path","arc_sha256","kcl_sha256","plc_sha256","plc_room","bounds","authorization"})||
 !id(get(r,"key"))||!text(get(r,"stage"),"F_SP121")||!integer(get(r,"room"),0,63)||!(get(r,"variant")&&get(r,"variant")->type==Node::String&&get(r,"variant")->text.size()==2&&get(r,"variant")->text.find_first_not_of("0123456789")==std::string::npos)||
 !hash(get(r,"arc_sha256"))||!hash(get(r,"kcl_sha256"))||!hash(get(r,"plc_sha256"))||
 !text(get(r,"authorization"),"STATIC_QA_NOT_RUNTIME_VERIFIED"))return false;
 Resource item;item.key=get(r,"key")->text;item.room=int(get(r,"room")->number);item.hash=get(r,"arc_sha256")->text;item.variant=get(r,"variant")->text;
 if(!integer(get(r,"plc_room"),item.room,item.room)||!text(get(r,"path"),"Stage/F_SP121/R"+(item.room<10?std::string("0"):std::string())+std::to_string(item.room)+"_"+item.variant+".arc"))return false;
 const auto* bs=get(r,"bounds");if(bs->type!=Node::Array||bs->array.size()!=6)return false;
 for(unsigned i=0;i<6;++i){if(!num(&bs->array[i],-1000000,1000000))return false;item.bounds[i]=bs->array[i].number;}
 if(item.bounds[0]>=item.bounds[1]||item.bounds[2]>item.bounds[3]||item.bounds[4]>=item.bounds[5])return false;
 for(const auto& old:resources)if(old.key==item.key||(old.room==item.room&&old.variant==item.variant))return false;
 resources.push_back(item);
 }
 Config draft;std::vector<DevfEncounterPlacementRequest> all;std::vector<bool> enabled;
 for(const auto& e:es->array){
 if(!keys_optional(e,{"id","enabled","enemy","resource","stage","room","layer","x","y","z","yaw_s16","native_profile","parameters","qa_confirmed","layer_state"},{"bokoblin_variant","hp"})||
 !id(get(e,"id"))||get(e,"id")->text.size()>31||get(e,"enabled")->type!=Node::Bool||get(e,"enemy")->type!=Node::String||
 !text(get(e,"stage"),"F_SP121")||!integer(get(e,"room"),0,63)||!integer(get(e,"layer"),0,14)||
 !integer(get(e,"yaw_s16"),-32768,32767)||!integer(get(e,"native_profile"),510,510)||!integer(get(e,"parameters"),4294902016.,4294902016.)||
 get(e,"qa_confirmed")->type!=Node::Bool||get(e,"resource")->type!=Node::String)return false;
 const Resource* resource=nullptr;for(const auto& r:resources)if(r.key==get(e,"resource")->text)resource=&r;
 if(!resource||resource->room!=get(e,"room")->number)return false;
 const auto& b=resource->bounds;
 if(!num(get(e,"x"),b[0]+100,b[1]-100)||!num(get(e,"y"),b[2]+5-.02,b[3]+5+.02)||!num(get(e,"z"),b[4]+100,b[5]-100))return false;
 std::uint8_t variant=variant_normal;std::int16_t hp=0;
 {const auto* vv=get(e,"bokoblin_variant");const auto* hv=get(e,"hp");
  if(vv){if(vv->type!=Node::String){why="VARIANT_UNKNOWN";return false;}
   if(vv->text=="black")variant=variant_black;else if(vv->text=="blue")variant=variant_blue;else if(vv->text=="red")variant=variant_red;else if(vv->text!="normal"){why="VARIANT_UNKNOWN";return false;}}
  if(!text(get(e,"enemy"),variant==variant_blue?"E_OC":"E_OC2")){why="VARIANT_ENEMY_MISMATCH";return false;}
  if(variant==variant_black){if(!integer(hv,hp_min,hp_max)){why="BLACK_HP_RANGE_REQUIRED";return false;}hp=std::int16_t(hv->number);}else if(hv){why="HP_ONLY_BLACK";return false;}}
 bool active=get(e,"enabled")->boolean;
 const bool legacy=text(get(e,"layer_state"),"CP08_OBSERVED");
 if(legacy){if(variant==variant_blue||variant==variant_red){why="EXPLICIT_VARIANT_REQUIRES_QA_CANDIDATE";return false;}if(resource->room!=3||resource->hash!=map_config::geometry_sha||get(e,"layer")->number!=6||!num(get(e,"x"),-2150,-350)||!num(get(e,"z"),59100,62900))return false;}
 else if(!text(get(e,"layer_state"),"QA_CANDIDATE")||(active&&!get(e,"qa_confirmed")->boolean))return false;
 DevfEncounterPlacementRequest r{};r.struct_size=sizeof(r);r.native_profile=510;
 std::memcpy(r.placement_id,get(e,"id")->text.c_str(),get(e,"id")->text.size()+1);std::memcpy(r.stage,"F_SP121",8);
 r.room=int(get(e,"room")->number);r.layer=int(get(e,"layer")->number);r.x=float(get(e,"x")->number);r.y=float(get(e,"y")->number);r.z=float(get(e,"z")->number);r.yaw_s16=int(get(e,"yaw_s16")->number);r.parameters=0xFFFF0100u;
 for(size_t i=0;i<all.size();++i){const auto& a=all[i];if(!std::strcmp(a.placement_id,r.placement_id))return false;
 if(a.room==r.room&&a.layer==r.layer&&((a.x==r.x&&a.y==r.y&&a.z==r.z)||(active&&enabled[i]&&std::hypot(double(a.x)-r.x,double(a.z)-r.z)<300.)))return false;}
 all.push_back(r);enabled.push_back(active);++draft.total;if(active){draft.variant[draft.count]=variant;draft.hp[draft.count]=hp;draft.requests[draft.count++]=r;}
 }
 out=draft;why="MULTIROOM_STATIC_QA_ONLY_COLD_START";return true;
 }catch(...){out={};return false;}
}
}
