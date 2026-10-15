#define main inherited_api_suite_main
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/tests/api_lab.cpp"
#undef main
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/provider/src/encounter_broker.hpp"
#include "../../CP07_2B_BOKOBLIN_EXPANSION_PILOT/expansion/src/client.hpp"
namespace pilot {
devf_api::EncounterBroker* broker=nullptr;bool pub_old=false,pub_new=false;
ModResult req(ModContext* c,const DevfEncounterPlacementRequest* r,DevfEncounterPlacementReceipt* out){return broker->request_encounter(c,r,out);}
ModResult old_req(ModContext* c,const DevfNativePlacementRequest* r,DevfNativePlacementReceipt* out){return broker->request(c,r,out);}
ModResult rem(ModContext* c,uint64_t t){return broker->remove(c,t);}
DevfEncounterPlacementService api{{sizeof(DevfEncounterPlacementService),1,0},req,rem};
DevfNativePlacementService old_api{{sizeof(DevfNativePlacementService),1,0},old_req,rem};
const char* id(ModContext* c){if(c==ctx)return "dev.devf.cp06_native_config_dual";if(c==lab::one)return "dev.bokoblin.expansion";if(c==lab::two)return "devf.second";return "";}
ModResult publish(ModContext* c,const char* name,uint16_t major,const void* service){check(c==ctx&&major==1);if(std::strcmp(name,DEVF_NATIVE_SERVICE_ID)==0&&service==&old_api){pub_old=true;return MOD_OK;}if(std::strcmp(name,DEVF_ENCOUNTER_SERVICE_ID)==0&&service==&api){pub_new=true;return MOD_OK;}return MOD_INVALID_ARGUMENT;}
ModResult get(ModContext*,const char* name,uint16_t major,uint16_t minor,const void** out){*out=nullptr;if(major!=1||minor)return MOD_UNSUPPORTED;if(pub_new&&std::strcmp(name,DEVF_ENCOUNTER_SERVICE_ID)==0)*out=&api;else if(pub_old&&std::strcmp(name,DEVF_NATIVE_SERVICE_ID)==0)*out=&old_api;return *out?MOD_OK:MOD_UNAVAILABLE;}
struct Session{
 native_config::Runtime core;devf_api::EncounterBroker owned;
 void start(const native_config::Config& config){pub_old=pub_new=false;core.config=config;core.accepted=true;check(core.registration.initialize(ctx,&service,config.active())==MOD_OK);broker=&owned;lab::current=broker;lab::host.mod_id=id;lab::host.publish_service=publish;lab::host.get_service=get;check(owned.initialize(ctx,&lab::host,&service,core));check(publish(ctx,DEVF_NATIVE_SERVICE_ID,1,&old_api)==MOD_OK&&publish(ctx,DEVF_ENCOUNTER_SERVICE_ID,1,&api)==MOD_OK);}
 void stop(){owned.shutdown();core.shutdown(ctx,&service);pub_old=pub_new=false;}
};
DevfEncounterPlacementReceipt receipt(){DevfEncounterPlacementReceipt r{};r.struct_size=sizeof(r);return r;}
uint32_t be32(const native_qa::Record& b,unsigned at){return uint32_t(b[at])<<24|uint32_t(b[at+1])<<16|uint32_t(b[at+2])<<8|b[at+3];}
}

native_qa::Record previous_encounter(const DevfEncounterPlacementRequest& r){
 auto b=native_qa::record(false);
 native_qa::be32(b,12,std::bit_cast<uint32_t>(r.x));native_qa::be32(b,16,std::bit_cast<uint32_t>(r.y));native_qa::be32(b,20,std::bit_cast<uint32_t>(r.z));
 native_qa::be16(b,26,static_cast<uint16_t>(r.yaw_s16));return b;
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 auto read=[&](const char* name){std::ifstream in(std::string(argv[1])+"/"+name,std::ios::binary);std::string raw(std::istreambuf_iterator<char>(in),{});native_config::Config c;native_config::Error e;check(native_config::decode(raw,c,e));return c;};
 lab::none=read("disabled.json");lab::field=read("field-only.json");lab::north=read("north-only.json");lab::both=read("both-enabled.json");
 try{
 for(unsigned n=0;n<2;++n)scenario(n?"B-only-byte25":"A-only-byte25",[&]{
  const auto r=expansion::request_for(n);const auto old=previous_encounter(r),now=devf_api::EncounterBroker::record(r);
  check(old.size()==32&&now.size()==32&&old[24]==1&&now[24]==1&&old[25]==6&&now[25]==0x32);
  unsigned changed=0;for(unsigned i=0;i<32;++i){if(old[i]!=now[i]){check(i==25);++changed;}}check(changed==1);
  std::printf("RANGE_PAIR_%u_OLD=",n);for(auto v:old)std::printf("%02x",v);std::printf("\nRANGE_PAIR_%u_NEW=",n);for(auto v:now)std::printf("%02x",v);std::printf("\n");
 });
 for(unsigned n=0;n<2;++n)scenario(n?"legacy-north-all-bytes":"legacy-field-all-bytes",[&]{
  pilot::Session s;s.start(n?lab::north:lab::field);check(fake.adds==1&&fake.records[0]==native_qa::record(n!=0)&&fake.records[0][24]==1&&fake.records[0][25]==6);s.stop();
 });
 scenario("legacy-probe-all-bytes",[]{pilot::Session s;s.start(lab::none);devf_probe::Probe p;check(p.initialize(lab::two,&lab::host,&pilot::old_api)==MOD_OK&&fake.records[0]==native_qa::record(false));p.shutdown();s.stop();});
 for(auto mode:{expansion::Mode::None,expansion::Mode::One,expansion::Mode::Both})scenario("real-client-registers-only-new-range",[&]{
  pilot::Session s;s.start(lab::none);expansion::Client c;check(c.initialize(lab::one,&lab::host,&pilot::api,mode)==MOD_OK);
  unsigned count=mode==expansion::Mode::Both?2:mode==expansion::Mode::One?1:0;check(fake.adds==count);
  for(unsigned n=0;n<count;++n)check(fake.records[n]==devf_api::EncounterBroker::record(expansion::request_for(n))&&fake.records[n][25]==0x32);
  check(c.shutdown()==MOD_OK&&fake.removes==count);s.stop();
 });
 std::printf("NATIVE_RANGE_LAB_PASS cases=%u\n",cases);return 0;
 }catch(const std::exception& e){std::printf("NATIVE_RANGE_LAB_FAIL %s\n",e.what());return 1;}
}
