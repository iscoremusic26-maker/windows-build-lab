#define main inherited_api_suite_main
#include "api_lab.cpp"
#undef main
#include "../provider/src/encounter_broker.hpp"
#include "../expansion/src/client.hpp"
#include <limits>
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
namespace mem{
std::string raw;bool exists=true,opened=false;size_t pos=0;std::string fault;FileService file{};
ModResult dir(ModContext* c,const char** out){check(c==lab::one);if(fault=="dir")return MOD_ERROR;*out="<OWN_DATA>";return MOD_OK;}
ModResult join(ModContext* c,const char*,const char* child,const char** out){check(c==lab::one&&std::string(child)=="qa.json");if(fault=="join")return MOD_ERROR;if(!exists)return MOD_UNAVAILABLE;*out="<OWN_DATA>/qa.json";return MOD_OK;}
ModResult open(ModContext* c,const char*,FileOpenMode mode,FileStreamHandle* h){check(c==lab::one&&mode==FILE_OPEN_READ);if(fault=="open")return MOD_ERROR;opened=true;pos=0;*h=1;return MOD_OK;}
ModResult size(ModContext*,FileStreamHandle,uint64_t* out){if(fault=="size")return MOD_ERROR;*out=raw.size();return MOD_OK;}
ModResult read(ModContext*,FileStreamHandle,void* out,uint64_t n,uint64_t* got){if(fault=="read")return MOD_ERROR;const auto count=std::min<size_t>(size_t(n),raw.size()-pos);std::memcpy(out,raw.data()+pos,count);pos+=count;*got=count;if(fault=="short"&&n>1&&*got)--*got;return MOD_OK;}
ModResult close(ModContext*,FileStreamHandle){opened=false;return fault=="close"?MOD_ERROR:MOD_OK;}
void setup(){exists=true;opened=false;pos=0;fault.clear();raw=R"({"schema_version":1,"mode":"NONE"})";file={};file.header={sizeof(file),1,0};file.join=join;file.open=open;file.size=size;file.read=read;file.close=close;lab::host.data_dir=dir;}
}
}
template<class F>void cut(const char* name,F fn){scenario(name,[&]{pilot::mem::setup();fn();check(!pilot::mem::opened);});}
int main(int argc,char** argv){
 if(argc!=2)return 2;auto read=[&](const char* name){std::ifstream in(std::string(argv[1])+"/"+name,std::ios::binary);std::string raw(std::istreambuf_iterator<char>(in),{});native_config::Config c;native_config::Error e;check(native_config::decode(raw,c,e));return c;};
 lab::none=read("disabled.json");lab::field=read("field-only.json");lab::north=read("north-only.json");lab::both=read("both-enabled.json");
 using namespace pilot;
 try{
 cut("new-ABI-old-header-unchanged",[]{check(sizeof(DevfNativePlacementService)==24&&sizeof(DevfEncounterPlacementService)==24&&sizeof(DevfEncounterPlacementReceipt)==24&&sizeof(DevfEncounterPlacementRequest)==80);});
 for(auto mode:{expansion::Mode::None,expansion::Mode::One,expansion::Mode::Both})cut("real-client-mode-0-1-2",[&]{Session s;s.start(lab::none);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,mode)==MOD_OK);unsigned count=mode==expansion::Mode::Both?2:mode==expansion::Mode::One?1:0;check(fake.adds==count&&s.owned.external_count()==count);if(count)check(s.owned.entries[0].owner==lab::one);if(count==2)check(c.tokens[0]!=c.tokens[1]&&s.owned.entries[1].owner==lab::one);check(c.shutdown()==MOD_OK);check(fake.removes==count);if(count==2)check(fake.removed[0]==102&&fake.removed[1]==101);s.stop();});
 cut("exact-user-A-native-record",[]{auto r=expansion::request_for(0);auto b=devf_api::EncounterBroker::record(r);check(r.x==5394.84716796875f&&r.y==-5603.69873046875f&&r.z==51442.3046875f&&r.yaw_s16==-2421);check(be32(b,8)==0xFFFF0100u&&be32(b,12)==std::bit_cast<uint32_t>(r.x)&&be32(b,16)==std::bit_cast<uint32_t>(r.y)&&be32(b,20)==std::bit_cast<uint32_t>(r.z));check(b[24]==1&&b[25]==6&&b[26]==0xF6&&b[27]==0x8B&&b[28]==0&&b[29]==0&&b[30]==255&&b[31]==255);std::printf("RECORD_USER_A=");for(auto n:b)std::printf("%02x",n);std::printf("\n");});
 cut("exact-user-B-native-record",[]{auto r=expansion::request_for(1);auto b=devf_api::EncounterBroker::record(r);check(r.x==3528.7822265625f&&r.y==-5850.8798828125f&&r.z==51201.09375f&&r.yaw_s16==-18402);check(b[26]==0xB8&&b[27]==0x1E);std::printf("RECORD_USER_B=");for(auto n:b)std::printf("%02x",n);std::printf("\n");});
 cut("not-A-B-hardcoded-provider-whitelist",[]{Session s;s.start(lab::none);auto r=expansion::request_for(0);r.x+=37.f;r.z+=81.f;auto out=receipt();check(req(lab::one,&r,&out)==MOD_OK&&out.flags==1);s.stop();});
 cut("same-owner-two-distinct",[]{Session s;s.start(lab::none);auto a=expansion::request_for(0),b=expansion::request_for(1);auto x=receipt(),y=receipt();check(req(lab::one,&a,&x)==MOD_OK&&req(lab::one,&b,&y)==MOD_OK&&x.placement_token!=y.placement_token);s.stop();});
 for(unsigned k=0;k<3;++k)cut("cross-owner-id-position-duplicates",[&]{Session s;s.start(lab::none);auto a=expansion::request_for(0);auto x=receipt();check(req(lab::one,&a,&x)==MOD_OK);auto b=expansion::request_for(1);if(k==0)std::memcpy(b.placement_id,a.placement_id,32);if(k==1){b.x=a.x;b.y=a.y;b.z=a.z;}if(k==2)b=a;auto y=receipt();check(req(lab::two,&b,&y)==MOD_CONFLICT&&fake.adds==1);s.stop();});
 cut("two-owners-distinct-positive",[]{Session s;s.start(lab::none);auto a=expansion::request_for(0),b=expansion::request_for(1);auto x=receipt(),y=receipt();check(req(lab::one,&a,&x)==MOD_OK&&req(lab::two,&b,&y)==MOD_OK);s.owned.detach(lab::one);check(s.owned.external_count()==1&&s.owned.entries[1].owner==lab::two);s.stop();});
 for(const auto* config:{&lab::field,&lab::north})cut("json-one-plus-A-capacity-two",[&]{Session s;s.start(*config);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::One)==MOD_OK&&fake.adds==2);c.shutdown();check(s.core.registration.ready);s.stop();});
 cut("json-one-plus-BOTH-atomic-consumer-rollback",[]{Session s;s.start(lab::field);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)==MOD_CONFLICT&&s.owned.external_count()==0&&s.core.registration.ready&&fake.live[0]==101&&!fake.live[1]);s.stop();});
 cut("json-two-rejects-all-new",[]{Session s;s.start(lab::both);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::One)==MOD_CONFLICT&&fake.adds==2);s.stop();});
 cut("json-physical-conflict",[]{Session s;s.start(lab::field);auto r=expansion::request_for(0);r.x=-1250;r.y=native_qa::site1_spawn;r.z=60750;auto out=receipt();check(req(lab::one,&r,&out)==MOD_CONFLICT&&fake.adds==1);s.stop();});
 cut("json-id-conflict",[]{Session s;s.start(lab::field);auto r=expansion::request_for(0);std::memset(r.placement_id,0,32);std::memcpy(r.placement_id,s.core.config.id_for(0),std::strlen(s.core.config.id_for(0))+1);auto out=receipt();check(req(lab::one,&r,&out)==MOD_CONFLICT);s.stop();});
 cut("old-probe-service1-compatible",[]{Session s;s.start(lab::none);devf_probe::Probe p;check(p.initialize(lab::two,&lab::host,&old_api)==MOD_OK&&fake.records[0]==native_qa::record(false));p.shutdown();s.stop();});
 cut("new-API-does-not-relabel-old-fixed-site",[]{Session s;s.start(lab::none);auto r=expansion::request_for(0);DevfNativePlacementReceipt out{};out.struct_size=sizeof(out);check(old_req(lab::one,&r,&out)==MOD_INVALID_ARGUMENT&&fake.adds==0);s.stop();});
 cut("old-request-conflicts-new-physical-position",[]{Session s;s.start(lab::none);auto r=expansion::request_for(0);r.x=-1250;r.y=native_qa::site1_spawn;r.z=60750;auto out=receipt();check(req(lab::one,&r,&out)==MOD_OK);devf_probe::Probe p;check(p.initialize(lab::two,&lab::host,&old_api)==MOD_CONFLICT&&fake.adds==1);s.stop();});
 cut("new-request-conflicts-old-api-position",[]{Session s;s.start(lab::none);devf_probe::Probe p;check(p.initialize(lab::two,&lab::host,&old_api)==MOD_OK);auto r=expansion::request_for(0);r.x=-1250;r.y=native_qa::site1_spawn;r.z=60750;auto out=receipt();check(req(lab::one,&r,&out)==MOD_CONFLICT);p.shutdown();s.stop();});
 for(unsigned k=0;k<9;++k)cut("invalid-native-scene-params-yaw",[&]{Session s;s.start(lab::none);auto r=expansion::request_for(0);if(k==0)r.native_profile=999;if(k==1)r.room=2;if(k==2)r.layer=5;if(k==3)r.stage[0]='X';if(k==4)r.parameters=0;if(k==5)r.yaw_s16=-32769;if(k==6)r.yaw_s16=32768;if(k==7)r.reserved=1;if(k==8)r.placement_id[0]='/';auto out=receipt();check(req(lab::one,&r,&out)==MOD_INVALID_ARGUMENT&&fake.adds==0&&!out.placement_token);s.stop();});
 for(float n:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),1000001.f})cut("invalid-coordinate",[&]{Session s;s.start(lab::none);auto r=expansion::request_for(0);r.x=n;auto out=receipt();check(req(lab::one,&r,&out)==MOD_INVALID_ARGUMENT&&!fake.adds);s.stop();});
 cut("unknown-and-provider-owner-rejected",[]{Session s;s.start(lab::none);auto r=expansion::request_for(0);auto out=receipt();check(req(lab::unknown,&r,&out)==MOD_INVALID_ARGUMENT&&req(ctx,&r,&out)==MOD_INVALID_ARGUMENT&&fake.adds==0);s.stop();});
 cut("late-bootstrap-closed",[]{Session s;s.start(lab::none);s.owned.close_admission();expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)==MOD_UNAVAILABLE&&!fake.adds);s.stop();});
 cut("owner-detach-removes-both",[]{Session s;s.start(lab::none);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)==MOD_OK);lab::detach(lab::one);check(s.owned.external_count()==0&&fake.removes==2&&fake.removed[0]==102&&fake.removed[1]==101);s.stop();});
 cut("remove-other-owner-ticket-rejected",[]{Session s;s.start(lab::none);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::One)==MOD_OK);check(rem(lab::two,c.tokens[0])==MOD_INVALID_ARGUMENT&&s.owned.external_count()==1);c.shutdown();s.stop();});
 for(unsigned n:{1u,2u})cut("failed-add-rolls-back-consumer",[&]{Session s;s.start(lab::none);fake.fail_at=n;expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)!=MOD_OK&&s.owned.external_count()==0);s.stop();});
 for(unsigned n:{1u,2u})cut("throw-add-rolls-back-consumer",[&]{Session s;s.start(lab::none);fake.throw_at=n;expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)!=MOD_OK&&s.owned.external_count()==0);s.stop();});
 for(unsigned n:{1u,2u})cut("zero-handle-rolls-back-consumer",[&]{Session s;s.start(lab::none);fake.zero_at=n;expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)!=MOD_OK&&s.owned.external_count()==0);s.stop();});
 cut("backend-aliased-handle-quarantine",[]{Session s;s.start(lab::none);fake.duplicate_at=2;expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)!=MOD_OK&&s.owned.fault);s.stop();});
 cut("detach-during-add",[]{Session s;s.start(lab::none);lab::detach_during_add=true;expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::One)!=MOD_OK&&s.owned.external_count()==0);s.stop();});
 cut("nested-request-busy",[]{Session s;s.start(lab::none);lab::reenter=true;expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::One)==MOD_OK&&lab::nested_result==MOD_CONFLICT);c.shutdown();s.stop();});
 cut("remove-fails-owner-dropped-detach",[]{Session s;s.start(lab::none);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)==MOD_OK);fake.remove_fail=true;lab::detach(lab::one);check(s.owned.fault&&s.owned.entries[0].owner==nullptr&&s.owned.entries[1].owner==nullptr);fake.remove_fail=false;s.stop();});
 cut("provider-missing-client-fails",[]{expansion::Client c;check(c.initialize(lab::one,&lab::host,nullptr,expansion::Mode::One)==MOD_UNSUPPORTED&&!fake.adds);});
 cut("new-major-mismatch-client-fails",[]{Session s;s.start(lab::none);auto wrong=api;wrong.header.major_version=2;expansion::Client c;check(c.initialize(lab::one,&lab::host,&wrong,expansion::Mode::Both)==MOD_UNSUPPORTED&&!fake.adds);s.stop();});
 cut("provider-unpublished-shutdown-no-stale-call",[]{Session s;s.start(lab::none);expansion::Client c;check(c.initialize(lab::one,&lab::host,&api,expansion::Mode::Both)==MOD_OK);pub_new=false;check(c.shutdown()==MOD_OK&&fake.removes==0);lab::detach(lab::one);s.stop();});
 for(const auto* name:{"NONE","A_ONLY","BOTH"})cut("own-qa-mode-examples",[&]{std::ifstream in(std::string(argv[1])+"/expansion/"+name+".json");mem::raw=std::string(std::istreambuf_iterator<char>(in),{});expansion::Mode mode;const char* reason;check(expansion::load_mode(lab::one,&lab::host,&mem::file,mode,reason)==MOD_OK);check(mode==(std::string(name)=="BOTH"?expansion::Mode::Both:std::string(name)=="A_ONLY"?expansion::Mode::One:expansion::Mode::None));});
 for(const auto* raw:{"",R"({"schema_version":1,"mode":"BOTH","mode":"NONE"})",R"({"schema_version":2,"mode":"BOTH"})",R"({"schema_version":1,"mode":"BOTH","extra":1})",R"({"schema_version":1,"mode":"FIELD"})","\xEF\xBB\xBF{}"})cut("invalid-mode-zero-requests",[&]{mem::raw=raw;expansion::Mode mode;const char* reason;check(expansion::load_mode(lab::one,&lab::host,&mem::file,mode,reason)!=MOD_OK&&mode==expansion::Mode::None&&!fake.adds);});
 cut("missing-mode-none-no-default-spawn",[]{mem::exists=false;expansion::Mode mode;const char* reason;check(expansion::load_mode(lab::one,&lab::host,&mem::file,mode,reason)==MOD_OK&&mode==expansion::Mode::None);});
 cut("oversized-mode",[]{mem::raw=std::string(257,' ');expansion::Mode mode;const char* reason;check(expansion::load_mode(lab::one,&lab::host,&mem::file,mode,reason)!=MOD_OK&&!fake.adds);});
 for(const auto* op:{"dir","join","open","size","read","short","close"})cut("mode-io-failure-no-requests",[&]{mem::raw=R"({"schema_version":1,"mode":"BOTH"})";mem::fault=op;expansion::Mode mode;const char* reason;check(expansion::load_mode(lab::one,&lab::host,&mem::file,mode,reason)!=MOD_OK&&mode==expansion::Mode::None&&!fake.adds);});
 cut("file-version-mismatch-no-requests",[]{mem::file.header.major_version=2;expansion::Mode mode;const char* reason;check(expansion::load_mode(lab::one,&lab::host,&mem::file,mode,reason)==MOD_UNAVAILABLE);});
 std::printf("EXPANSION_LAB_PASS cases=%u\n",cases);return 0;
 }catch(...){std::printf("EXPANSION_LAB_FAIL completed=%u\n",cases);return 1;}
}
