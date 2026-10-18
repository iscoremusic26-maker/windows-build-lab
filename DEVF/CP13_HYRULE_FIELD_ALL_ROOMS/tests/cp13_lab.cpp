#include "../runtime/DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/encounter_broker.hpp"
#include "../runtime/DEVF/CP10_2_BLACK_BOKOBLIN/expansion/client.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
int provider_tag,owner_tag,other_tag;
auto* provider=reinterpret_cast<ModContext*>(&provider_tag);
auto* owner=reinterpret_cast<ModContext*>(&owner_tag);
auto* other=reinterpret_cast<ModContext*>(&other_tag);
void ok(bool x){if(!x)throw std::runtime_error("assertion");}
unsigned cases=0,adds=0,fail_at=0,throw_at=0,zero_at=0;
bool alias=false,detach_add=false,nested=false;
std::map<StageActorHandle,native_qa::Record> live;std::vector<StageActorHandle> removed;
std::vector<std::pair<int,int>> contexts;
devf_api::EncounterBroker* broker;
ModLifecycleFn watch_cb;void* watch_data;
ModResult add(ModContext* c,const char* stage,uint8_t room,int8_t layer,const void* b,size_t n,StageActorHandle* out){
 ok(c==provider&&!strcmp(stage,"F_SP121")&&room>=0&&room<=63&&layer>=0&&layer<=14&&n==32);++adds;contexts.emplace_back(room,layer);
 if(adds==throw_at)throw std::runtime_error("injected");
 if(adds==fail_at)return MOD_ERROR;if(adds==zero_at)return MOD_OK;
 if(alias&&adds==2){*out=101;return MOD_OK;}
 *out=100+adds;native_qa::Record r;memcpy(r.data(),b,32);live[*out]=r;
 if(detach_add)watch_cb(provider,owner,"owner",MOD_LIFECYCLE_DETACHED,watch_data);
 return MOD_OK;
}
ModResult remove_edit(ModContext* c,StageActorHandle h){ok(c==provider&&live.count(h));removed.push_back(h);live.erase(h);return MOD_OK;}
const char* id(ModContext* c){return c==owner?"dev.bokoblin.expansion":c==other?"other":c==provider?"provider":"";}
ModResult watch(ModContext*,ModLifecycleFn cb,void* data,uint64_t* h){watch_cb=cb;watch_data=data;*h=1;return MOD_OK;}
ModResult unwatch(ModContext*,uint64_t){watch_cb=nullptr;return MOD_OK;}
ModResult publish(ModContext*,const char*,uint16_t,const void*){return MOD_OK;}
ModResult req(ModContext* c,const DevfEncounterPlacementRequest* r,DevfEncounterPlacementReceipt* out){return broker->request_encounter(c,r,out);}
ModResult rem(ModContext* c,uint64_t t){return broker->remove(c,t);}
DevfEncounterPlacementService api{{sizeof(api),1,0},req,rem};
ModResult getsvc(ModContext*,const char*,uint16_t major,uint16_t,const void** out){*out=major==1?&api:nullptr;return *out?MOD_OK:MOD_UNAVAILABLE;}
HostService host{};StageService stage{};
void setup(){adds=fail_at=throw_at=zero_at=0;alias=detach_add=false;live.clear();removed.clear();contexts.clear();host={};host.header={sizeof(host),2,2};host.version="2.0.3";host.mod_id=id;host.publish_service=publish;host.watch_mod_lifecycle=watch;host.unwatch_mod_lifecycle=unwatch;host.get_service=getsvc;stage={};stage.header={sizeof(stage),1,0};stage.add_actor=add;stage.remove_actor_edit=remove_edit;}
struct Session {native_config::Runtime core;devf_api::EncounterBroker b;Session(unsigned legacy=0){broker=&b;core.accepted=true;for(unsigned i=0;i<legacy;++i)core.config.placements[i]={i?"legacy-north":"legacy-field",true,i==1};core.config.count=core.config.enabled_count=legacy;ok(core.registration.initialize(provider,&stage,core.config.active())==MOD_OK);ok(b.initialize(provider,&host,&stage,core));}~Session(){b.shutdown();core.registration.cleanup(provider,&stage);}};
std::string entry(unsigned i,bool enabled=true){return "{\"id\":\"bok-"+std::to_string(i)+"\",\"enabled\":"+(enabled?"true":"false")+",\"enemy\":\"E_OC2\",\"stage\":\"F_SP121\",\"room\":3,\"layer\":6,\"x\":-2000,\"y\":-6300,\"z\":"+std::to_string(59200+i*400)+",\"yaw_s16\":-2421}";}
std::string doc(unsigned n){std::string s="{\"document_type\":\"devf_map_encounters\",\"schema_version\":1,\"geometry_sha256\":\""+std::string(map_config::geometry_sha)+"\",\"encounters\":[";for(unsigned i=0;i<n;++i){if(i)s+=",";s+=entry(i);}return s+"]}";}
multi_config::Config parse(const std::string& s){multi_config::Config c;const char* why;if(!multi_config::decode(s,c,why)){std::cerr<<why<<"\n"<<s<<"\n";throw std::runtime_error("decode");}return c;}
// ---- CP10.2 schema2 helpers: one resource (F_SP121 room 1) + encounters with optional bokoblin_variant/hp ----
std::string s2enc(const std::string& id,bool on,double x,const std::string& extra=""){
 return "{\"id\":\""+id+"\",\"enabled\":"+(on?"true":"false")+",\"enemy\":\"E_OC2\",\"resource\":\"r01-00\",\"stage\":\"F_SP121\",\"room\":1,\"layer\":6,\"x\":"+std::to_string(x)+",\"y\":-8097.86523,\"z\":80595.0703,\"yaw_s16\":0,\"native_profile\":510,\"parameters\":4294902016,\"qa_confirmed\":"+(on?"true":"false")+",\"layer_state\":\"QA_CANDIDATE\""+extra+"}";
}
std::string s2doc(const std::vector<std::string>& encounters){
 std::string h(64,'a'),k(64,'b'),p(64,'c');
 std::string s="{\"document_type\":\"devf_map_encounters\",\"schema_version\":2,\"disc_id\":\"GZ2E01\",\"revision\":0,\"resources\":[{\"key\":\"r01-00\",\"stage\":\"F_SP121\",\"room\":1,\"variant\":\"00\",\"path\":\"Stage/F_SP121/R01_00.arc\",\"arc_sha256\":\""+h+"\",\"kcl_sha256\":\""+k+"\",\"plc_sha256\":\""+p+"\",\"plc_room\":1,\"bounds\":[-30000,0,-9000,-7000,70000,90000],\"authorization\":\"STATIC_QA_NOT_RUNTIME_VERIFIED\"}],\"encounters\":[";
 for(size_t i=0;i<encounters.size();++i){if(i)s+=",";s+=encounters[i];}return s+"]}";
}
bool decodes(const std::string& raw){multi_config::Config c;const char* why;return multi_config::decode(raw,c,why);}
template<class F>void test(const char* name,F fn){setup();try{fn();++cases;std::cout<<"CASE_PASS "<<name<<"\n";}catch(...){std::cerr<<"CASE_FAIL "<<name<<"\n";throw;}}
namespace io {
std::string data,fault;size_t pos;bool opened=false,missing=false;FileService fs{};
ModResult dir(ModContext* c,const char** p){ok(c==owner);*p="OWN";return fault=="dir"?MOD_ERROR:MOD_OK;}
ModResult join(ModContext*,const char*,const char* child,const char** p){ok(!strcmp(child,"encounters.json"));*p="OWN/file";return missing?MOD_UNAVAILABLE:fault=="join"?MOD_ERROR:MOD_OK;}
ModResult open(ModContext*,const char*,FileOpenMode,FileStreamHandle* h){if(fault=="open")return MOD_ERROR;*h=1;pos=0;opened=true;return MOD_OK;}
ModResult size(ModContext*,FileStreamHandle,uint64_t* n){*n=data.size();return fault=="size"?MOD_ERROR:MOD_OK;}
ModResult read(ModContext*,FileStreamHandle,void* dst,uint64_t n,uint64_t* got){if(fault=="read")return MOD_ERROR;*got=std::min<uint64_t>(n,data.size()-pos);memcpy(dst,data.data()+pos,*got);pos+=*got;if(fault=="short"&&n>1&&*got)--*got;return MOD_OK;}
ModResult close(ModContext*,FileStreamHandle){opened=false;return fault=="close"?MOD_ERROR:MOD_OK;}
void init(){fault="";missing=opened=false;data=doc(3);host.data_dir=dir;fs={};fs.header={sizeof(fs),1,0};fs.join=join;fs.open=open;fs.size=size;fs.read=read;fs.close=close;}
}
#include "../runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include <chrono>

std::string resource(int room,const std::string& variant="00"){
 auto nn=(room<10?std::string("0"):std::string())+std::to_string(room);
 return "{\"key\":\"r"+nn+"-"+variant+"\",\"stage\":\"F_SP121\",\"room\":"+std::to_string(room)+",\"variant\":\""+variant+"\",\"path\":\"Stage/F_SP121/R"+nn+"_"+variant+".arc\",\"arc_sha256\":\""+std::string(63,'a')+"1\",\"kcl_sha256\":\""+std::string(64,'b')+"\",\"plc_sha256\":\""+std::string(64,'c')+"\",\"plc_room\":"+std::to_string(room)+",\"bounds\":[-100000,100000,-10000,1000,-100000,100000],\"authorization\":\"STATIC_QA_NOT_RUNTIME_VERIFIED\"}";
}
std::string allrooms(int room,unsigned n=1){
 std::string s="{\"document_type\":\"devf_map_encounters\",\"schema_version\":2,\"disc_id\":\"GZ2E01\",\"revision\":0,\"resources\":["+resource(room)+"],\"encounters\":[";
 for(unsigned i=0;i<n;++i){if(i)s+=",";auto e=s2enc("p-"+std::to_string(i),true,-50000.+i*400.);auto replace=[&](const std::string& a,const std::string& b){e.replace(e.find(a),a.size(),b);};
 replace("r01-00","r"+(room<10?std::string("0"):std::string())+std::to_string(room)+"-00");replace("\"room\":1","\"room\":"+std::to_string(room));s+=e;
 }return s+"]}";
}
int main(int argc,char** argv){try{
 for(int room:{0,1,3,5,6,7,9,10,15,63})test("all-room-parser-provider-context",[&]{Session session;auto c=parse(allrooms(room));expansion::Client client;ok(client.initialize(owner,&host,&api,c)==MOD_OK&&adds==1&&contexts[0].first==room&&contexts[0].second==6);ok(live.begin()->second[24]==1&&live.begin()->second[25]==0x32);client.shutdown();ok(live.empty());});
 test("32-registered-and-33-rejected",[]{Session s;auto c=parse(allrooms(9,32));expansion::Client cli;ok(cli.initialize(owner,&host,&api,c)==MOD_OK&&live.size()==32&&cli.tokens[31]);auto r=c.requests[0];strcpy(r.placement_id,"thirty-three");r.x=50000;DevfEncounterPlacementReceipt o{};o.struct_size=sizeof(o);ok(req(owner,&r,&o)==MOD_CONFLICT&&adds==32);cli.shutdown();ok(live.empty()&&removed[0]==132);});
 test("parser-no-33-total",[]{ok(!decodes(allrooms(9,33)));});
 test("legacy-schema1-still-eight-limit",[]{ok(decodes(doc(8))&&!decodes(doc(9)));});
 for(unsigned failure:{1u,16u,32u})test("32-partial-failure-own-inverse-rollback",[&]{Session s;fail_at=failure;expansion::Client cli;ok(cli.initialize(owner,&host,&api,parse(allrooms(15,32)))!=MOD_OK&&live.empty());if(failure>1)ok(removed.front()==100+failure-1);});
 test("32-detach",[]{Session s;expansion::Client cli;ok(cli.initialize(owner,&host,&api,parse(allrooms(0,32)))==MOD_OK);s.b.detach(owner);ok(live.empty());});
 test("legacy-two-reserve-capacity",[]{Session s(2);expansion::Client cli;ok(cli.initialize(owner,&host,&api,parse(allrooms(10,31)))==MOD_CONFLICT&&live.size()==2);});
 test("global-id-and-context-separation",[]{Session s;auto r=parse(allrooms(7)).requests[0];DevfEncounterPlacementReceipt o{};o.struct_size=sizeof(o);ok(req(owner,&r,&o)==MOD_OK);strcpy(r.placement_id,"other-room");r.room=9;ok(req(owner,&r,&o)==MOD_OK);strcpy(r.placement_id,"same-room-near");r.x+=100;ok(req(owner,&r,&o)==MOD_CONFLICT);r.layer=5;ok(req(owner,&r,&o)==MOD_OK);});
 for(int room:{-1,64,255})test("native-room-domain-reject",[&]{Session s;auto r=parse(allrooms(7)).requests[0];r.room=room;DevfEncounterPlacementReceipt o{};o.struct_size=sizeof(o);ok(req(owner,&r,&o)==MOD_INVALID_ARGUMENT&&!adds);});
 test("late-admission-reject",[]{Session s;s.b.close_admission();expansion::Client cli;ok(cli.initialize(owner,&host,&api,parse(allrooms(7)))==MOD_UNAVAILABLE&&!adds);});
 test("32-exact-variant-registration",[]{auto c=parse(allrooms(9,32));for(unsigned i=0;i<32;++i){c.variant[i]=i%2;c.hp[i]=i%2?440:0;}devf_black2::Table black,normal;ok(devf_variant::register_encounters(c,multi_config::variant_black,black,normal));ok(black.count==16&&normal.count==16);});
 test("bounded-state-memory",[]{ok(sizeof(devf_api::EncounterBroker)<32768&&sizeof(multi_config::Config)<8192&&sizeof(devf_black2::Table)<8192);std::cout<<"MEMORY broker="<<sizeof(devf_api::EncounterBroker)<<" config="<<sizeof(multi_config::Config)<<" exact_table="<<sizeof(devf_black2::Table)<<"\n";});
 test("parser-128KiB-bound",[]{auto s=allrooms(9,32);s.resize(131073,' ');ok(!decodes(s));});
 if(argc==2)test("CSharp-CP13-export-real-client",[&]{std::ifstream in(argv[1],std::ios::binary);std::string raw(std::istreambuf_iterator<char>(in),{});auto c=parse(raw);ok(c.count==32);Session s;expansion::Client cli;ok(cli.initialize(owner,&host,&api,c)==MOD_OK&&adds==32);cli.shutdown();ok(live.empty());});
 auto start=std::chrono::steady_clock::now();for(int i=0;i<100;++i)ok(decodes(allrooms(15,32)));auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();std::cout<<"BENCH_100_PARSERS_US "<<us<<" LAB_NOT_GAME_FPS\n";
 std::cout<<"CP13_LAB_PASS cases="<<cases<<"\n";return 0;
 }catch(...){return 1;}}
