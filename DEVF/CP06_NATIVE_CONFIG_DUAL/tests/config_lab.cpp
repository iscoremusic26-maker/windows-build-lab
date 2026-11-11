#include "registration_cases.hpp"
#include "../src/runtime.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
using native_config::Config;
using native_config::Error;
using native_config::Runtime;
namespace disk {
std::string folder,location;std::ifstream stream;HostService host{};FileService files{};
unsigned opens=0,closes=0,reads=0,dirs=0,joins=0;std::string fault;bool threw_close=false;
void check(bool b){::check(b);}
void reset(){
 if(stream.is_open())stream.close();stream.clear();opens=closes=reads=dirs=joins=0;fault.clear();threw_close=false;
 host={};host.header={sizeof(HostService),2,2};host.version="2.0.3";
 files={};files.header={sizeof(FileService),1,0};
}
void write(const std::string& raw){std::ofstream out(std::filesystem::path(folder)/"placements.json",std::ios::binary|std::ios::trunc);out<<raw;check(bool(out));}
ModResult dir(ModContext* c,const char** p){check(c==ctx);++dirs;if(fault=="dir_throw")throw 1;if(fault=="dir_error")return MOD_ERROR;if(fault=="dir_null"){*p=nullptr;return MOD_OK;}*p=folder.c_str();return MOD_OK;}
ModResult join(ModContext* c,const char* base,const char* child,const char** p){
 check(c==ctx&&base==folder.c_str()&&std::string(child)=="placements.json");++joins;
 if(fault=="join_throw")throw 1;if(fault=="join_error")return MOD_ERROR;if(fault=="join_null"){*p=nullptr;return MOD_OK;}
 location=(std::filesystem::path(folder)/child).string();if(!std::filesystem::exists(location))return MOD_UNAVAILABLE;*p=location.c_str();return MOD_OK;
}
ModResult open(ModContext* c,const char*,FileOpenMode mode,FileStreamHandle* h){
 check(c==ctx&&mode==FILE_OPEN_READ);if(fault=="open_throw")throw 1;if(fault=="open_error")return MOD_ERROR;if(fault=="open_zero")return MOD_OK;
 stream.open(location,std::ios::binary);check(bool(stream));++opens;*h=55;
 if(fault=="open_handle_throw")throw 1;return fault=="open_handle_error"?MOD_ERROR:MOD_OK;
}
ModResult size(ModContext* c,FileStreamHandle h,uint64_t* n){check(c==ctx&&h==55);if(fault=="size_throw")throw 1;if(fault=="size_error")return MOD_ERROR;*n=fault=="oversize"?native_config::kMaxBytes+1:fault=="zero_size"?0:std::filesystem::file_size(location);return MOD_OK;}
ModResult read(ModContext* c,FileStreamHandle h,void* p,uint64_t n,uint64_t* actual){
 check(c==ctx&&h==55);++reads;if(fault=="read_throw")throw 1;if(fault=="read_error")return MOD_ERROR;
 if(n==1&&fault=="growth"){*actual=1;return MOD_OK;}if(n==1&&fault=="tail_error")return MOD_ERROR;
 stream.read(static_cast<char*>(p),std::streamsize(n));*actual=uint64_t(stream.gcount());if(n>1&&fault=="short"&&*actual)--*actual;return MOD_OK;
}
ModResult close(ModContext* c,FileStreamHandle h){
 check(c==ctx&&h==55);++closes;if(fault=="close_throw"&&!threw_close){threw_close=true;throw 1;}
 stream.close();return fault=="close_error"?MOD_ERROR:MOD_OK;
}
void setup(){reset();host.data_dir=dir;files.join=join;files.open=open;files.size=size;files.read=read;files.close=close;}
}
std::string read_text(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);check(bool(in));return std::string(std::istreambuf_iterator<char>(in),{});}
std::string replace_one(std::string s,const std::string& a,const std::string& b){auto p=s.find(a);check(p!=s.npos);s.replace(p,a.size(),b);return s;}
std::string both,field,north,disabled;
void rejected(const std::string& raw,const char* reason=nullptr){
 disk::write(raw);Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_OK&&!r.accepted&&fake.adds==0&&r.config.count==0);
 if(reason)check(std::string(r.error.code)==reason);check(r.shutdown(ctx,&service)==MOD_OK&&fake.removes==0&&!disk::stream.is_open());
}
void accepted(const std::string& raw,unsigned n,bool first_north=false){
 disk::write(raw);Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_OK&&r.accepted&&r.config.enabled_count==n&&fake.adds==n);
 if(n)check(fake.records[0]==record(first_north));if(n==2)check(fake.records[1]==record(true));
 check(r.shutdown(ctx,&service)==MOD_OK&&!disk::stream.is_open());
}
template<class F>void config_test(const char* name,F fn){test(name,[&]{disk::setup();fn();});}
int main(int argc,char** argv){
 if(argc!=3)return 2;if(run_registration_cases()!=0)return 1;
 disk::folder=argv[2];std::filesystem::create_directories(disk::folder);
 const std::filesystem::path examples=argv[1];
 both=read_text(examples/"both-enabled.json");field=read_text(examples/"field-only.json");north=read_text(examples/"north-only.json");disabled=read_text(examples/"disabled.json");
 try{
 config_test("json_both_two_exact_records",[]{accepted(both,2);});
 config_test("json_field_only",[]{accepted(field,1);});
 config_test("json_north_only",[]{accepted(north,1,true);});
 config_test("json_empty_placements",[]{accepted(disabled,0);});
 config_test("json_both_disabled",[]{accepted(replace_one(replace_one(both,"\"enabled\": true","\"enabled\": false"),"\"enabled\": true","\"enabled\": false"),0);});
 config_test("json_field_disabled_north_active",[]{accepted(replace_one(both,"\"enabled\": true","\"enabled\": false"),1,true);});
 config_test("json_north_disabled_field_active",[]{auto p=both.rfind("\"enabled\": true");auto b=both;b.replace(p,15,"\"enabled\": false");accepted(b,1);});
 config_test("json_reference_placements_matches_both",[&]{check(read_text(examples/"placements.json")==both);});
 config_test("json_deterministic_order",[]{accepted(R"json({"schema_version":1,"variants":[{"variant_id":"native_elite","enemy":"E_OC2","native_profile":510,"hp":220,"appearance":"native","ai":"native"}],"placements":[{"placement_id":"north_native","variant_id":"native_elite","enabled":true,"stage":"F_SP121","room":3,"layer":6,"x":-1250,"y":-6322.6005859375,"z":61950,"yaw_s16":0,"parameters":4294902016},{"placement_id":"field_native","variant_id":"native_elite","enabled":true,"stage":"F_SP121","room":3,"layer":6,"x":-1250,"y":-6304.00830078125,"z":60750,"yaw_s16":0,"parameters":4294902016}]})json",2);});
 config_test("json_float32_equivalent",[]{accepted(replace_one(both,"-6322.6005859375","-6322.600450915229"),2);});
 config_test("json_invalid_missing",[]{std::filesystem::remove(std::filesystem::path(disk::folder)/"placements.json");Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_OK&&!r.accepted&&!fake.adds&&!disk::opens);});
 config_test("json_invalid_empty",[]{rejected("");});
 config_test("json_invalid_oversize",[]{rejected(std::string(native_config::kMaxBytes+1,' '));check(disk::reads==0);});
 config_test("json_invalid_bom",[]{rejected(std::string("\xef\xbb\xbf")+both);});
 config_test("json_invalid_syntax",[]{rejected("{");});
 config_test("json_invalid_trailing",[]{rejected(both+"[]","JSON_TRAILING");});
 config_test("json_invalid_duplicate_key",[]{rejected(replace_one(both,"\"schema_version\": 1","\"schema_version\": 1, \"schema_version\": 1"),"JSON_DUPLICATE_KEY");});
 config_test("json_invalid_unknown_root_key",[]{rejected(replace_one(both,"\"schema_version\": 1","\"unknown\": 1, \"schema_version\": 1"),"SCHEMA_ROOT_KEYS");});
 config_test("json_invalid_unknown_placement_key",[]{rejected(replace_one(both,"\"enabled\": true","\"other\": 1, \"enabled\": true"),"PLACEMENT_FIELDS");});
 config_test("json_invalid_unknown_variant_key",[]{rejected(replace_one(both,"\"hp\": 220","\"other\": 1, \"hp\": 220"),"VARIANT_FIELDS");});
 config_test("json_invalid_version",[]{rejected(replace_one(both,"\"schema_version\": 1","\"schema_version\": 2"),"SCHEMA_VERSION");});
 config_test("json_invalid_unknown_reference",[]{rejected(replace_one(both,"\"variant_id\": \"native_elite\",\n      \"enabled\"","\"variant_id\": \"absent\",\n      \"enabled\""),"UNKNOWN_VARIANT_ID");});
 config_test("json_invalid_black_guard",[]{rejected(replace_one(both,"\"variant_id\": \"native_elite\"","\"variant_id\": \"black_guard\""),"NATIVE_VARIANT_ONLY");});
 config_test("json_invalid_hp",[]{rejected(replace_one(both,"\"hp\": 220","\"hp\": 150"),"NATIVE_VARIANT_ONLY");});
 config_test("json_invalid_tint",[]{rejected(replace_one(both,"\"appearance\": \"native\"","\"appearance\": \"charcoal\""),"NATIVE_VARIANT_ONLY");});
 config_test("json_invalid_ai",[]{rejected(replace_one(both,"\"ai\": \"native\"","\"ai\": \"bounded_guard\""),"NATIVE_VARIANT_ONLY");});
 config_test("json_invalid_profile",[]{rejected(replace_one(both,"\"native_profile\": 510","\"native_profile\": 511"),"NATIVE_VARIANT_ONLY");});
 config_test("json_invalid_enemy",[]{rejected(replace_one(both,"E_OC2","E_OC"),"NATIVE_VARIANT_ONLY");});
 config_test("json_invalid_x",[]{rejected(replace_one(both,"\"x\": -1250","\"x\": -1251"),"SITE_NOT_ALLOWED");});
 config_test("json_invalid_ground_y",[]{rejected(replace_one(both,"-6322.6005859375","-6327.600450915229"),"SITE_NOT_ALLOWED");});
 config_test("json_invalid_z",[]{rejected(replace_one(both,"61950","61951"),"SITE_NOT_ALLOWED");});
 config_test("json_invalid_nonfinite",[]{rejected(replace_one(both,"-1250","1e999"),"JSON_NUMBER_RANGE");});
 config_test("json_invalid_nan",[]{rejected(replace_one(both,"-1250","NaN"));});
 config_test("json_invalid_yaw",[]{rejected(replace_one(both,"\"yaw_s16\": 0","\"yaw_s16\": 1"),"NATIVE_RECORD_CONTRACT");});
 config_test("json_invalid_parameters",[]{rejected(replace_one(both,"4294902016","0"),"NATIVE_RECORD_CONTRACT");});
 config_test("json_invalid_stage",[]{rejected(replace_one(both,"F_SP121","F_SP122"),"SCENE_NOT_ALLOWED");});
 config_test("json_invalid_room",[]{rejected(replace_one(both,"\"room\": 3","\"room\": 4"),"SCENE_NOT_ALLOWED");});
 config_test("json_invalid_layer",[]{rejected(replace_one(both,"\"layer\": 6","\"layer\": 7"),"SCENE_NOT_ALLOWED");});
 config_test("json_invalid_duplicate_id",[]{rejected(replace_one(both,"north_native","field_native"),"DUPLICATE_PLACEMENT_ID");});
 config_test("json_invalid_duplicate_site",[]{auto b=replace_one(both,"-6322.6005859375","-6304.00830078125");rejected(replace_one(b,"61950","60750"),"DUPLICATE_PHYSICAL_SITE");});
 config_test("json_invalid_disabled_entry",[]{rejected(replace_one(replace_one(both,"\"enabled\": true","\"enabled\": false"),"\"x\": -1250","\"x\": -1251"),"SITE_NOT_ALLOWED");});
 config_test("json_invalid_three_entries",[]{rejected(R"json({"schema_version":1,"variants":[{"variant_id":"native_elite","enemy":"E_OC2","native_profile":510,"hp":220,"appearance":"native","ai":"native"}],"placements":[{"placement_id":"field_native","variant_id":"native_elite","enabled":true,"stage":"F_SP121","room":3,"layer":6,"x":-1250,"y":-6304.00830078125,"z":60750,"yaw_s16":0,"parameters":4294902016},{"placement_id":"north_native","variant_id":"native_elite","enabled":true,"stage":"F_SP121","room":3,"layer":6,"x":-1250,"y":-6322.6005859375,"z":61950,"yaw_s16":0,"parameters":4294902016},{"placement_id":"third","variant_id":"native_elite","enabled":true,"stage":"F_SP121","room":3,"layer":6,"x":-1250,"y":-6304.00830078125,"z":60750,"yaw_s16":0,"parameters":4294902016}]})json","PLACEMENT_LIMIT");});
 config_test("json_invalid_id_path",[]{rejected(replace_one(both,"field_native","../escape"),"PLACEMENT_FIELDS");});
 config_test("json_invalid_types",[]{rejected(replace_one(both,"\"enabled\": true","\"enabled\": 1"),"PLACEMENT_FIELDS");});
 config_test("json_invalid_complexity",[]{rejected(std::string(10,'[')+std::string(10,']'),"JSON_COMPLEXITY");});
 for(const auto* fault:{"dir_error","dir_null","dir_throw","join_error","join_null","join_throw","open_error","open_zero","open_throw","open_handle_error","open_handle_throw","size_error","size_throw","zero_size","oversize","read_error","read_throw","short","growth","tail_error","close_error","close_throw"}){
  config_test(fault,[&]{disk::fault=fault;rejected(both);check(disk::closes>=disk::opens);});
 }
 config_test("null_host",[]{disk::write(both);Runtime r;check(r.initialize(ctx,nullptr,&disk::files,&service)==MOD_OK&&!r.accepted&&!fake.adds&&!disk::dirs);});
 config_test("host_wrong_version",[]{disk::host.version="2.0.2";rejected(both);check(!disk::dirs);});
 config_test("host_short_header",[]{disk::host.header.struct_size=8;rejected(both);check(!disk::dirs);});
 config_test("host_wrong_major",[]{disk::host.header.major_version=1;rejected(both);});
 config_test("host_old_minor",[]{disk::host.header.minor_version=1;rejected(both);});
 config_test("file_wrong_major",[]{disk::files.header.major_version=2;rejected(both);});
 config_test("file_short_header",[]{disk::files.header.struct_size=8;rejected(both);});
 config_test("stage_wrong_major",[]{service.header.major_version=2;rejected(both);});
 config_test("file_missing_read",[]{disk::files.read=nullptr;rejected(both);});
 config_test("file_missing_close",[]{disk::files.close=nullptr;rejected(both);});
 config_test("host_missing_data_dir",[]{disk::host.data_dir=nullptr;rejected(both);});
 config_test("stage_missing_add",[]{service.add_actor=nullptr;rejected(both);check(!disk::dirs);});
 config_test("stage_missing_remove",[]{service.remove_actor_edit=nullptr;rejected(both);check(!disk::dirs);});
 config_test("runtime_stage_fail_second",[]{disk::write(both);fake.fail_at=2;Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_ERROR&&!r.accepted&&fake.adds==2&&fake.removes==1&&!fake.live[0]);r.shutdown(ctx,&service);});
 config_test("runtime_stage_exception_second",[]{disk::write(both);fake.throw_handle_at=2;Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_ERROR&&fake.removes==2);});
 config_test("runtime_no_frame_registration_or_io",[]{disk::write(both);Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_OK);const auto reads=disk::reads;for(unsigned i=0;i<1000;++i)check(r.update()==MOD_OK);check(fake.adds==2&&disk::reads==reads);check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_CONFLICT&&fake.adds==2);r.shutdown(ctx,&service);r.shutdown(ctx,&service);check(fake.removes==2);});
 config_test("restart_config_changes_2_1_1_0",[]{for(const auto& raw:{both,field,north,disabled}){fake={};disk::setup();disk::write(raw);Runtime r;check(r.initialize(ctx,&disk::host,&disk::files,&service)==MOD_OK&&r.accepted);r.shutdown(ctx,&service);}check(fake.adds==0);});
 config_test("no_partial_config_on_late_error",[]{Config c;Error e;check(native_config::decode(both,c,e));check(!native_config::decode(replace_one(both,"north_native","field_native"),c,e)&&c.count==0&&c.enabled_count==0);});
 std::printf("JSON_DUAL_NATIVE_LAB_PASS cases=%u game_executed=false\n",cases);return 0;
 }catch(...){std::printf("JSON_DUAL_NATIVE_LAB_FAIL completed=%u\n",cases);return 1;}
}
