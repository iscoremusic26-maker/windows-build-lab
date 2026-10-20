#define broker lab_broker
#include "support.hpp"
#undef broker
#include "../expansion/packaged_config.hpp"
#include "../runtime/DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/mod.cpp"
#include <filesystem>
#include <bit>
unsigned io_calls=0,published=0;
ModResult no_io_dir(ModContext*,const char**){++io_calls;return MOD_ERROR;}
ModResult no_io_join(ModContext*,const char*,const char*,const char**){++io_calls;return MOD_ERROR;}
ModResult no_io_open(ModContext*,const char*,FileOpenMode,FileStreamHandle*){++io_calls;return MOD_ERROR;}
ModResult capture_publish(ModContext*,const char* id,uint16_t major,const void* service){
 ok(major==1&&service);
 if(!strcmp(id,DEVF_NATIVE_SERVICE_ID))ok(static_cast<const DevfNativePlacementService*>(service)->request_placement==api_request);
 else if(!strcmp(id,DEVF_ENCOUNTER_SERVICE_ID))ok(static_cast<const DevfEncounterPlacementService*>(service)->request_placement==encounter_request);
 else {ok(!strcmp(id,DEVF_ENCOUNTER_V11_SERVICE_ID));ok(static_cast<const DevfEncounterPlacementServiceV11*>(service)->request_placement_v2==encounter_request_v2);}
 ++published;return MOD_OK;
}
ModResult public_getsvc(ModContext*,const char* id,uint16_t major,uint16_t minor,const void** out){*out=nullptr;if(!strcmp(id,DEVF_ENCOUNTER_V11_SERVICE_ID)&&major==1&&minor<=1)*out=&encounter_service_v11;return *out?MOD_OK:MOD_UNAVAILABLE;}
void public_setup(){
 setup();core={};::broker={};io_calls=published=0;host.data_dir=no_io_dir;host.publish_service=capture_publish;host.get_service=public_getsvc;
 static FileService file{};file.header={sizeof(file),1,0};file.join=no_io_join;file.open=no_io_open;
 mod_ctx=provider;svc_host=&host;svc_file=&file;svc_stage=&stage;svc_log=nullptr;
}
multi_config::Config frozen(){multi_config::Config c;const char* why;ok(packaged_config::load(c,why)==MOD_OK);return c;}
int main(int argc,char** argv){try{
 ok(argc==2);std::filesystem::path root=argv[1];
 test("frozen-document-exact-decoder-and-float32-identity",[&]{
  auto c=frozen();ok(c.count==32);native_config::Parser parser{cp21_frozen::json};native_config::Node doc;ok(parser.parse(doc));
  auto* es=native_config::get(doc,"encounters");ok(es&&es->array.size()==32);unsigned types[4]={};
  for(unsigned i=0;i<32;++i){auto& q=c.requests[i];const auto& e=es->array[i];
   ok(!strcmp(q.placement_id,native_config::get(e,"id")->text.c_str()));
   for(const auto& v:std::array<std::pair<const char*,float>,3>{{{"x",q.x},{"y",q.y},{"z",q.z}}})ok(std::bit_cast<uint32_t>(v.second)==std::bit_cast<uint32_t>(float(native_config::get(e,v.first)->number)));
   ok(q.room==native_config::get(e,"room")->number&&q.layer==6&&q.yaw_s16==native_config::get(e,"yaw_s16")->number&&q.parameters==0xFFFF0100u);
   ++types[c.variant[i]];if(c.variant[i]==1)ok(c.hp[i]==440);
  }ok(types[2]==15&&types[3]==7&&types[1]==10&&!types[0]);
 });
 for(bool placements:{false,true})for(bool encounters:{false,true})test("public-entrypoints-stale-files-ignored-zero-legacy-32-user-edits",[&]{
  public_setup();std::filesystem::create_directories(root);auto pf=root/"placements.json",ef=root/"encounters.json";
  std::filesystem::remove(pf);std::filesystem::remove(ef);
  if(placements){std::ofstream f(pf);f<<"{\"placements\":[\"FIELD\",\"NORTH\"]}";}
  if(encounters){std::ofstream f(ef);f<<doc(8);}
  const auto oldp=placements?std::filesystem::file_size(pf):0,olde=encounters?std::filesystem::file_size(ef):0;
  ok(mod_initialize(nullptr)==MOD_OK&&published==3&&core.accepted&&core.registration.ready&&core.registration.expected==0&&!adds&&!io_calls);
  auto c=frozen();expansion::Client client;ok(client.initialize(owner,&host,&encounter_service_v11,c)==MOD_OK&&adds==32&&live.size()==32&&!io_calls);
  for(unsigned i=0;i<c.count;++i){auto& r=c.requests[i];const auto& b=live.at(101+i);ok(b==devf_api::EncounterBroker::record(r,c.variant[i]==2?1:2));ok(b[24]==(c.variant[i]==2?0:1)&&b[25]==0x32);}
  // One accepted ID has one edit; a repeated admission cannot spawn another.
  DevfEncounterPlacementReceipt out{sizeof(out)};DevfEncounterPlacementRequestV2 request{sizeof(request),2,1,0,c.requests[0]};
  ok(encounter_request_v2(owner,&request,&out)==MOD_CONFLICT&&adds==32);
  auto foreign=out;ok(api_remove(other,client.tokens[0])==MOD_INVALID_ARGUMENT&&live.size()==32);
  ok(mod_update(nullptr)==MOD_OK&&adds==32); // room reentry is host StageService persistence, never a new request
  ok(client.shutdown()==MOD_OK&&live.empty());ok(mod_shutdown(nullptr)==MOD_OK&&!watch_cb&&!io_calls);
  ok((!placements||std::filesystem::file_size(pf)==oldp)&&(!encounters||std::filesystem::file_size(ef)==olde));
 });
 test("cold-reinstall-and-owner-detach-cleanup",[]{for(int install=0;install<2;++install){public_setup();ok(mod_initialize(nullptr)==MOD_OK);auto c=frozen();expansion::Client cli;ok(cli.initialize(owner,&host,&encounter_service_v11,c)==MOD_OK);watch_cb(provider,owner,"dev.bokoblin.expansion",MOD_LIFECYCLE_DETACHED,watch_data);ok(live.empty());ok(mod_shutdown(nullptr)==MOD_OK);}});
 test("framework-unavailable-zero-edits",[]{auto c=frozen();expansion::Client cli;ok(cli.initialize(owner,&host,nullptr,c)!=MOD_OK&&!adds);});
 for(unsigned fail:{1u,16u,32u})test("frozen-batch-rollback",[&]{public_setup();ok(mod_initialize(nullptr)==MOD_OK);fail_at=fail;auto c=frozen();expansion::Client cli;ok(cli.initialize(owner,&host,&encounter_service_v11,c)!=MOD_OK&&live.empty());ok(mod_shutdown(nullptr)==MOD_OK);});
 test("empty-core-host-version-fails-closed",[]{public_setup();host.version="2.0.4";ok(mod_initialize(nullptr)==MOD_UNSUPPORTED&&!adds&&!published);});
 std::cout<<"CP21_LAB_PASS cases="<<cases<<" GAME_EXECUTED=false\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
