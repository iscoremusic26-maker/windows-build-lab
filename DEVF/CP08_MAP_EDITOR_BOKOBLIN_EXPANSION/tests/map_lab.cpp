#include "../provider/encounter_broker.hpp"
#include "../expansion/client.hpp"
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
devf_api::EncounterBroker* broker;
ModLifecycleFn watch_cb;void* watch_data;
ModResult add(ModContext* c,const char* stage,uint8_t room,int8_t layer,const void* b,size_t n,StageActorHandle* out){
 ok(c==provider&&!strcmp(stage,"F_SP121")&&room==3&&layer==6&&n==32);++adds;
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
void setup(){adds=fail_at=throw_at=zero_at=0;alias=detach_add=false;live.clear();removed.clear();host={};host.header={sizeof(host),2,2};host.version="2.0.3";host.mod_id=id;host.publish_service=publish;host.watch_mod_lifecycle=watch;host.unwatch_mod_lifecycle=unwatch;host.get_service=getsvc;stage={};stage.header={sizeof(stage),1,0};stage.add_actor=add;stage.remove_actor_edit=remove_edit;}
struct Session {native_config::Runtime core;devf_api::EncounterBroker b;Session(){broker=&b;core.accepted=true;ok(b.initialize(provider,&host,&stage,core));}~Session(){b.shutdown();}};
std::string entry(unsigned i,bool enabled=true){return "{\"id\":\"bok-"+std::to_string(i)+"\",\"enabled\":"+(enabled?"true":"false")+",\"enemy\":\"E_OC2\",\"stage\":\"F_SP121\",\"room\":3,\"layer\":6,\"x\":-1250,\"y\":-6300,\"z\":"+std::to_string(59200+i*400)+",\"yaw_s16\":-2421}";}
std::string doc(unsigned n){std::string s="{\"document_type\":\"devf_map_encounters\",\"schema_version\":1,\"geometry_sha256\":\""+std::string(map_config::geometry_sha)+"\",\"encounters\":[";for(unsigned i=0;i<n;++i){if(i)s+=",";s+=entry(i);}return s+"]}";}
map_config::Config parse(const std::string& s){map_config::Config c;const char* why;ok(map_config::decode(s,c,why));return c;}
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
int main(int argc,char** argv){try{
 for(unsigned n:{0u,1u,2u,3u,8u})test("JSON-real-client-provider-0-1-2-3-8",[&]{Session s;auto c=parse(doc(n));expansion::Client client;ok(client.initialize(owner,&host,&api,c)==MOD_OK&&adds==n&&live.size()==n);if(n){auto b=live.begin()->second;ok(b[24]==1&&b[25]==0x32&&b[8]==255&&b[9]==255&&b[10]==1&&b[11]==0);ok(client.tokens[n-1]);}ok(client.shutdown()==MOD_OK&&live.empty());if(n)ok(removed.front()==100+n);});
 test("disabled-total-limit",[]{auto raw=doc(8);for(size_t p=0;(p=raw.find(":true",p))!=std::string::npos;)raw.replace(p,5,":false");ok(parse(raw).count==0);map_config::Config c;const char* e;ok(!map_config::decode(doc(9),c,e)&&!c.count);});
 test("duplicate-key-and-id-position",[]{for(auto raw:{doc(3)+"]",std::string("\xEF\xBB\xBF")+doc(3)}){map_config::Config c;const char* e;ok(!map_config::decode(raw,c,e));}auto raw=doc(3);auto pos=raw.find("bok-1");raw.replace(pos,5,"bok-0");map_config::Config c;const char* e;ok(!map_config::decode(raw,c,e));raw=doc(3);raw.replace(raw.find("59600"),5,"59200");ok(!map_config::decode(raw,c,e));});
 for(const auto* bad:{"NaN","1e999","0","-2300"})test("invalid-coordinate",[&]{auto raw=doc(3);raw.replace(raw.find("-1250"),5,bad);map_config::Config c;const char* e;ok(!map_config::decode(raw,c,e)&&!c.count);});
 for(unsigned n:{1u,2u,3u,8u})test("partial-failure-inverse-rollback",[&]{Session s;fail_at=n;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(8)))!=MOD_OK&&live.empty());});
 test("exception-rollback",[]{Session s;throw_at=3;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(8)))!=MOD_OK&&live.empty());});
 test("zero-handle-rollback",[]{Session s;zero_at=3;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(8)))!=MOD_OK&&live.empty());});
 test("aliased-handle-no-double-remove",[]{Session s;alias=true;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(3)))!=MOD_OK&&live.empty()&&removed.size()==1&&s.b.fault);});
 test("detach-eight",[]{Session s;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(8)))==MOD_OK);s.b.detach(owner);ok(live.empty());});
 test("detach-during-add",[]{Session s;detach_add=true;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(3)))!=MOD_OK&&live.empty());});
 test("late-rejected",[]{Session s;s.b.close_admission();expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(3)))==MOD_UNAVAILABLE&&!adds);});
 test("capacity-no-nine",[]{Session s;expansion::Client c;ok(c.initialize(owner,&host,&api,parse(doc(8)))==MOD_OK);auto r=parse(doc(1)).requests[0];strcpy(r.placement_id,"ninth");r.x=-2000;DevfEncounterPlacementReceipt out{};out.struct_size=sizeof(out);ok(req(other,&r,&out)==MOD_CONFLICT&&adds==8);});
 test("legacy-record-bytes-and-ABI",[]{Session s;DevfNativePlacementRequest r{};r.struct_size=sizeof(r);r.native_profile=510;strcpy(r.placement_id,"probe");strcpy(r.stage,"F_SP121");r.room=3;r.layer=6;r.x=-1250;r.y=native_qa::site1_spawn;r.z=60750;r.parameters=0xFFFF0100u;DevfNativePlacementReceipt out{};out.struct_size=sizeof(out);ok(s.b.request(other,&r,&out)==MOD_OK&&live[101]==native_qa::record(false)&&out.site==0);ok(sizeof(api)==24&&sizeof(DevfEncounterPlacementRequest)==80);});
 for(auto fault:{"dir","join","open","size","read","short","close"})test("IO-fail-closed",[&]{io::init();io::fault=fault;map_config::Config c;const char* why;ok(expansion::load_config(owner,&host,&io::fs,c,why)!=MOD_OK&&!c.count&&!io::opened&&!adds);});
 test("IO-missing-zero-valid-three",[]{io::init();map_config::Config c;const char* why;io::missing=true;ok(expansion::load_config(owner,&host,&io::fs,c,why)==MOD_OK&&!c.count);io::missing=false;ok(expansion::load_config(owner,&host,&io::fs,c,why)==MOD_OK&&c.count==3&&!io::opened);});
 if(argc==2)test("CSharp-export-consumed-real-core",[&]{std::ifstream in(argv[1],std::ios::binary);ok(bool(in));std::string raw(std::istreambuf_iterator<char>(in),{});auto c=parse(raw);ok(c.count>=3);Session s;expansion::Client client;ok(client.initialize(owner,&host,&api,c)==MOD_OK&&adds==c.count);client.shutdown();});
 std::cout<<"CP08_RUNTIME_LAB_PASS cases="<<cases<<"\n";return 0;
 }catch(...){return 1;}}
