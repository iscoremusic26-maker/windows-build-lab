#include "../src/config_io.hpp"
#include "../src/policy.hpp"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <stdexcept>
using namespace cp6;
void need(bool b){if(!b)throw std::runtime_error("assertion");}
const std::string blackJson=R"json({
  "schema_version": 1,
  "variants": [
    {
      "variant_id": "black_guard",
      "enemy": "E_OC2",
      "native_profile": 510,
      "hp": 150,
      "appearance": "charcoal",
      "ai": "bounded_guard"
    },
    {
      "variant_id": "native_elite",
      "enemy": "E_OC2",
      "native_profile": 510,
      "hp": 220,
      "appearance": "native",
      "ai": "native"
    }
  ],
  "placements": [
    {
      "placement_id": "field_black",
      "variant_id": "black_guard",
      "enabled": true,
      "stage": "F_SP121",
      "room": 3,
      "layer": 6,
      "x": -1250,
      "y": -6304.00830078125,
      "z": 60750,
      "yaw_s16": 0,
      "parameters": 4294902016
    },
    {
      "placement_id": "field_native",
      "variant_id": "native_elite",
      "enabled": false,
      "stage": "F_SP121",
      "room": 3,
      "layer": 6,
      "x": -1250,
      "y": -6304.00830078125,
      "z": 60750,
      "yaw_s16": 0,
      "parameters": 4294902016
    }
  ]
}
)json";
const std::string native=R"json({
  "schema_version": 1,
  "variants": [
    {
      "variant_id": "black_guard",
      "enemy": "E_OC2",
      "native_profile": 510,
      "hp": 150,
      "appearance": "charcoal",
      "ai": "bounded_guard"
    },
    {
      "variant_id": "native_elite",
      "enemy": "E_OC2",
      "native_profile": 510,
      "hp": 220,
      "appearance": "native",
      "ai": "native"
    }
  ],
  "placements": [
    {
      "placement_id": "field_black",
      "variant_id": "black_guard",
      "enabled": false,
      "stage": "F_SP121",
      "room": 3,
      "layer": 6,
      "x": -1250,
      "y": -6304.00830078125,
      "z": 60750,
      "yaw_s16": 0,
      "parameters": 4294902016
    },
    {
      "placement_id": "field_native",
      "variant_id": "native_elite",
      "enabled": true,
      "stage": "F_SP121",
      "room": 3,
      "layer": 6,
      "x": -1250,
      "y": -6304.00830078125,
      "z": 60750,
      "yaw_s16": 0,
      "parameters": 4294902016
    }
  ]
}
)json";
const std::string empty=R"json({
  "schema_version": 1,
  "variants": [
    {
      "variant_id": "black_guard",
      "enemy": "E_OC2",
      "native_profile": 510,
      "hp": 150,
      "appearance": "charcoal",
      "ai": "bounded_guard"
    },
    {
      "variant_id": "native_elite",
      "enemy": "E_OC2",
      "native_profile": 510,
      "hp": 220,
      "appearance": "native",
      "ai": "native"
    }
  ],
  "placements": []
}
)json";
std::string replace(std::string s,const std::string& a,const std::string& b){auto p=s.find(a);need(p!=s.npos);s.replace(p,a.size(),b);return s;}
Config good(const std::string& s){Config c;Error e;need(decode(s,c,e));return c;}
void reject(const std::string& s,const char* expected=nullptr){Config c=good(blackJson);Error e;need(!decode(s,c,e)&&c.selected==-1&&c.count==0);if(expected)need(std::string(e.code)==expected);}
namespace disk {
std::string folder,location;std::ifstream stream;int opens=0,closes=0;bool io_error=false,bad_close=false,oversize=false,short_read=false;
void reset(){if(stream.is_open())stream.close();opens=closes=0;io_error=bad_close=oversize=short_read=false;}
void write(const std::string& s){std::ofstream f(std::filesystem::path(folder)/"placements.json",std::ios::binary|std::ios::trunc);f<<s;need(bool(f));}
ModResult dir(ModContext*,const char** p){*p=folder.c_str();return MOD_OK;}
ModResult join(ModContext*,const char*,const char* child,const char** p){need(std::string(child)=="placements.json");location=(std::filesystem::path(folder)/child).string();if(!std::filesystem::exists(location)){*p=nullptr;return MOD_UNAVAILABLE;}*p=location.c_str();return MOD_OK;}
ModResult open(ModContext*,const char* p,FileOpenMode mode,FileStreamHandle* handle){need(mode==FILE_OPEN_READ);stream.open(p,std::ios::binary);if(!stream)return MOD_UNAVAILABLE;++opens;*handle=1;return MOD_OK;}
ModResult size(ModContext*,FileStreamHandle,std::uint64_t* n){*n=oversize?kMaxBytes+1:std::filesystem::file_size(location);return MOD_OK;}
ModResult read(ModContext*,FileStreamHandle,void* p,std::uint64_t n,std::uint64_t* actual){if(io_error)return MOD_ERROR;stream.read(static_cast<char*>(p),static_cast<std::streamsize>(n));*actual=std::uint64_t(stream.gcount());if(short_read&&*actual)--*actual;return MOD_OK;}
ModResult close(ModContext*,FileStreamHandle){stream.close();++closes;return bad_close?MOD_ERROR:MOD_OK;}
HostService host{};FileService files{};
void setup(const char* path){folder=path;std::filesystem::create_directories(folder);host.data_dir=dir;files.join=join;files.open=open;files.size=size;files.read=read;files.close=close;}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;disk::setup(argv[1]);unsigned count=0;const char* current="";
 auto test=[&](const char* name,auto fn){current=name;fn();++count;std::printf("PASS %s\n",name);};
 try{
 test("two_distinct_presets",[]{config=good(blackJson);need(cp6::black()&&guard()&&hp()==150);config=good(native);need(!cp6::black()&&!guard()&&hp()==220);});
 test("stable_placement_and_variant_ids",[]{auto c=good(blackJson);need(c.count==2&&c.placements[0].id=="field_black"&&c.placements[1].id=="field_native"&&c.selected==0);});
 test("zero_placements",[]{config=good(empty);need(!active()&&!active_variant()&&!cp6::black()&&!guard());});
 test("both_active_fail_closed",[]{reject(replace(blackJson,"\"enabled\": false","\"enabled\": true"),"MULTI_ACTIVE_NOT_SUPPORTED");});
 test("duplicate_variant_id",[]{reject(replace(blackJson,"\"variant_id\": \"native_elite\"","\"variant_id\": \"black_guard\""),"DUPLICATE_VARIANT_ID");});
 test("duplicate_placement_id",[]{reject(replace(blackJson,"field_native","field_black"),"DUPLICATE_PLACEMENT_ID");});
 test("unknown_variant",[]{reject(replace(blackJson,"\"variant_id\": \"black_guard\",\n      \"enabled\"","\"variant_id\": \"missing\",\n      \"enabled\""),"UNKNOWN_VARIANT_ID");});
 test("unsupported_combinations",[]{reject(replace(blackJson,"\"hp\": 150","\"hp\": 151"),"UNSUPPORTED_VARIANT_COMBINATION");reject(replace(blackJson,"bounded_guard","fire"),"UNSUPPORTED_VARIANT_COMBINATION");});
 test("wrong_profile_or_enemy",[]{reject(replace(blackJson,"\"native_profile\": 510","\"native_profile\": 511"),"UNSUPPORTED_NATIVE_PROFILE");reject(replace(blackJson,"E_OC2","E_OC"),"UNSUPPORTED_NATIVE_PROFILE");});
 test("unknown_keys_rejected",[]{reject(replace(blackJson,"\"schema_version\": 1","\"future_buff\": true, \"schema_version\": 1"),"SCHEMA_ROOT_KEYS");});
 test("schema_version_and_type",[]{reject(replace(blackJson,"\"schema_version\": 1","\"schema_version\": 2"),"SCHEMA_VERSION");reject(replace(blackJson,"\"schema_version\": 1","\"schema_version\": true"),"SCHEMA_VERSION");});
 test("stage_room_layer",[]{reject(replace(blackJson,"F_SP121","F_SP122"),"SCENE_NOT_VERIFIED");reject(replace(blackJson,"\"room\": 3","\"room\": 4"),"SCENE_NOT_VERIFIED");reject(replace(blackJson,"\"layer\": 6","\"layer\": 5"),"SCENE_NOT_VERIFIED");});
 test("site2_unverified",[]{reject(replace(blackJson,"\"x\": -1250","\"x\": -1251"),"SITE_NOT_VERIFIED");});
 test("coordinate_bounds_and_nonfinite",[]{reject(replace(blackJson,"\"x\": -1250","\"x\": 1000001"),"COORDINATE_RANGE");reject(replace(blackJson,"\"x\": -1250","\"x\": 1e999"),"JSON_NUMBER_RANGE");reject(replace(blackJson,"\"x\": -1250","\"x\": NaN"));});
 test("yaw_and_parameters",[]{need(good(replace(blackJson,"\"yaw_s16\": 0","\"yaw_s16\": -32768")).placements[0].yaw==-32768);reject(replace(blackJson,"\"yaw_s16\": 0","\"yaw_s16\": 32768"),"NATIVE_RECORD_CONTRACT");reject(replace(blackJson,"4294902016","0"),"NATIVE_RECORD_CONTRACT");});
 test("json_syntax_and_duplicate_key",[]{reject("{");reject(replace(blackJson,"\"schema_version\": 1","\"schema_version\": 1, \"schema_version\": 1"),"JSON_DUPLICATE_KEY");reject(blackJson+"garbage","JSON_TRAILING");});
 test("malicious_strings_ids_and_paths",[]{reject(replace(blackJson,"field_black","../escape"),"PLACEMENT_FIELDS");reject(replace(blackJson,"field_black","field\\u0000"));reject(std::string("\xef\xbb\xbf")+blackJson);reject(replace(blackJson,"field_black",std::string(65,'a')));});
 test("oversized_and_deep",[]{reject(std::string(kMaxBytes+1,' '),"CONFIG_SIZE");reject(std::string(10,'[')+std::string(10,']'),"JSON_COMPLEXITY");});
 test("no_partial_config_committed",[]{Config c=good(blackJson);Error e;need(!decode(replace(blackJson,"field_native","field_black"),c,e)&&c.selected==-1&&c.variant_count==0);});
 test("native_preset_no_ai_or_tint",[]{config=good(native);need(!guard()&&!cp6::black()&&hp()==220);});
 test("retail_identity_exclusion",[]{config=good(blackJson);devf::Snapshot s;s.profile=510;s.room=3;s.parameters=0xFFFF0100u;s.set_id=0xffff;s.x=-50;s.y=-6201.491699f;s.z=60750;s.home_x=0x106;need(!devf::record_matches(s));});
 test("admission_once_and_respawn",[]{config=good(blackJson);devf::Snapshot s;s.profile=510;s.room=3;s.parameters=0xFFFF0100u;s.set_id=0xffff;s.x=-1250;s.y=-6304.0083f;s.z=60750;s.home_x=0x106;s.id=147;s.initializing=s.stage=s.resource_eoc2=true;s.layer=6;s.health=220;devf::Policy p;p.armed=true;need(p.inspect(s)==devf::Decision::Applied);s.initializing=false;need(p.inspect(s)==devf::Decision::Ignore);s.initializing=true;s.id=566;need(p.inspect(s)==devf::Decision::Applied);});
 test("recycled_or_duplicate_actor_rejected",[]{config=good(blackJson);devf::Snapshot s;s.profile=510;s.room=3;s.parameters=0xFFFF0100u;s.set_id=0xffff;s.x=-1250;s.y=-6304.0083f;s.z=60750;s.home_x=0x106;s.id=147;s.initializing=s.stage=s.resource_eoc2=true;s.layer=6;s.health=220;devf::Policy p;p.armed=true;need(p.inspect(s)==devf::Decision::Applied);need(p.inspect(s)==devf::Decision::Fault);p=devf::Policy{};p.armed=true;s.previous_live=true;need(p.inspect(s)==devf::Decision::Fault);});
 test("windows_real_file_read_then_restart_edit",[]{disk::reset();disk::write(blackJson);Config a;Error e;need(load(nullptr,&disk::host,&disk::files,a,e)&&a.selected==0);disk::write(native);Config b;need(load(nullptr,&disk::host,&disk::files,b,e)&&b.selected==1);need(disk::opens==2&&disk::closes==2);});
 test("windows_file_remove_placement_restart",[]{disk::reset();disk::write(empty);Config c;Error e;need(load(nullptr,&disk::host,&disk::files,c,e)&&c.selected==-1);});
 test("windows_missing_file",[]{disk::reset();std::filesystem::remove(std::filesystem::path(disk::folder)/"placements.json");Config c;Error e;need(!load(nullptr,&disk::host,&disk::files,c,e)&&c.selected==-1&&disk::opens==0);});
 test("io_oversize_no_read",[]{disk::reset();disk::write(blackJson);disk::oversize=true;Config c;Error e;need(!load(nullptr,&disk::host,&disk::files,c,e)&&disk::closes==1&&c.selected==-1);});
 test("io_short_error_close_fail",[]{for(int kind=0;kind<3;++kind){disk::reset();disk::write(blackJson);disk::short_read=kind==0;disk::io_error=kind==1;disk::bad_close=kind==2;Config c;Error e;need(!load(nullptr,&disk::host,&disk::files,c,e)&&disk::closes==1&&c.selected==-1);}});
 test("empty_file_and_null_api",[]{disk::reset();disk::write("");Config c;Error e;need(!load(nullptr,&disk::host,&disk::files,c,e)&&disk::closes==1);need(!load(nullptr,nullptr,nullptr,c,e));});
 test("config_changed_yaw_without_recompile",[]{disk::reset();disk::write(replace(blackJson,"\"yaw_s16\": 0","\"yaw_s16\": 120"));Config c;Error e;need(load(nullptr,&disk::host,&disk::files,c,e)&&c.placements[0].yaw==120);});
 std::printf("LAB_PASS cases=%u CP06_PARTIAL max_active=1 game_executed=false\n",count);
 }catch(...){std::printf("LAB_FAIL case=%s completed=%u\n",current,count);return 1;}
}
