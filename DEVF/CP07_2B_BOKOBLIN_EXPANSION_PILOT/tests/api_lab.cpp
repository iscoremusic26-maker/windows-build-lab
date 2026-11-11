#include "../provider/tests/registration_cases.hpp"
#include "../provider/src/broker.hpp"
#include "../provider/src/persistence.hpp"
#include "../consumer/src/probe.hpp"
#include <fstream>
namespace lab {
int c1,c2,invalid;auto* one=reinterpret_cast<ModContext*>(&c1);auto* two=reinterpret_cast<ModContext*>(&c2);auto* unknown=reinterpret_cast<ModContext*>(&invalid);
devf_api::Broker* current=nullptr;ModLifecycleFn watcher=nullptr;void* watch_data=nullptr;
HostService host{};bool published=false,watch_fail=false,watch_zero=false,publish_fail=false,unwatch_fail=false,detach_during_add=false,reenter=false;unsigned watch_adds=0,watch_removes=0,publishes=0;ModResult nested_result=MOD_OK;
ModResult request(ModContext* c,const DevfNativePlacementRequest* r,DevfNativePlacementReceipt* out){return current->request(c,r,out);}
ModResult remove(ModContext* c,uint64_t t){return current->remove(c,t);}
DevfNativePlacementService api{{sizeof(DevfNativePlacementService),1,0},request,remove};
const char* id(ModContext* c){if(c==ctx)return "dev.devf.cp06_native_config_dual";if(c==one)return "dev.devf.native_api_probe";if(c==two)return "dev.devf.test_second_consumer";return "";}
ModResult watch(ModContext* c,ModLifecycleFn fn,void* data,uint64_t* out){check(c==ctx);++watch_adds;if(watch_fail)return MOD_ERROR;*out=watch_zero?0:9;watcher=fn;watch_data=data;return MOD_OK;}
ModResult unwatch(ModContext* c,uint64_t h){check(c==ctx&&h==9);++watch_removes;watcher=nullptr;watch_data=nullptr;return unwatch_fail?MOD_ERROR:MOD_OK;}
ModResult publish(ModContext* c,const char* name,uint16_t major,const void* s){check(c==ctx&&std::strcmp(name,DEVF_NATIVE_SERVICE_ID)==0&&major==1&&s==&api);++publishes;if(publish_fail)return MOD_ERROR;published=true;return MOD_OK;}
ModResult get(ModContext*,const char* name,uint16_t major,uint16_t minor,const void** out){*out=nullptr;if(published&&std::strcmp(name,DEVF_NATIVE_SERVICE_ID)==0&&major==api.header.major_version&&minor<=api.header.minor_version){*out=&api;return MOD_OK;}return MOD_UNAVAILABLE;}
void detach(ModContext* subject){if(watcher)watcher(ctx,subject,id(subject),MOD_LIFECYCLE_DETACHED,watch_data);}
ModResult add_advanced(ModContext* c,const char* s,uint8_t r,int8_t l,const void* bytes,size_t n,StageActorHandle* out){
 if(reenter){reenter=false;auto req=devf_probe::field();DevfNativePlacementReceipt receipt{};receipt.struct_size=sizeof(receipt);nested_result=current->request(two,&req,&receipt);}
 const auto result=add(c,s,r,l,bytes,n,out);
 if(detach_during_add){detach_during_add=false;detach(one);}return result;
}
void setup(){
 host={};host.header={sizeof(HostService),2,2};host.version="2.0.3";host.mod_id=id;host.watch_mod_lifecycle=watch;host.unwatch_mod_lifecycle=unwatch;host.publish_service=publish;host.get_service=get;
 current=nullptr;watcher=nullptr;watch_data=nullptr;published=watch_fail=watch_zero=publish_fail=unwatch_fail=detach_during_add=reenter=false;watch_adds=watch_removes=publishes=0;nested_result=MOD_OK;
 api={{sizeof(DevfNativePlacementService),1,0},request,remove};service.add_actor=add_advanced;
}
native_config::Config both,field,north,none;
struct Session {
 native_config::Runtime core;devf_api::Broker broker;
 bool start(const native_config::Config& c,bool valid=true){
  core.config=c;core.accepted=valid;
  check(core.registration.initialize(ctx,&service,valid?c.active():std::array<bool,2>{})==MOD_OK);
  current=&broker;
  if(!broker.initialize(ctx,&host,&service,core))return false;
  const auto r=host.publish_service(ctx,DEVF_NATIVE_SERVICE_ID,1,&api);if(r!=MOD_OK){broker.shutdown();core.shutdown(ctx,&service);return false;}return true;
 }
 void stop(){broker.shutdown();published=false;core.shutdown(ctx,&service);}
};
DevfNativePlacementRequest north_request(){auto r=devf_probe::field();std::memcpy(r.placement_id,"api_probe_north",16);r.y=native_qa::north_spawn;r.z=61950.f;return r;}
DevfNativePlacementReceipt receipt(){DevfNativePlacementReceipt r{};r.struct_size=sizeof(r);return r;}
}
template<class F>void scenario(const char* name,F fn){test(name,[&]{lab::setup();fn();});}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 auto read=[&](const char* file){std::ifstream s(std::string(argv[1])+"/"+file,std::ios::binary);check(bool(s));std::string text(std::istreambuf_iterator<char>(s),{});native_config::Config c;native_config::Error e;check(native_config::decode(text,c,e));return c;};
 lab::both=read("both-enabled.json");lab::field=read("field-only.json");lab::north=read("north-only.json");lab::none=read("disabled.json");
 using namespace lab;
 try{
 scenario("provider_alone_four_json_modes",[]{for(const auto* c:{&both,&field,&north,&none}){fake={};Session s;check(s.start(*c));check(fake.adds==c->enabled_count&&published);s.stop();check(!fake.live[0]&&!fake.live[1]);}});
 scenario("required_consumer_real_request_none_field",[]{Session s;check(s.start(none));devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_OK&&p.token&&fake.adds==1&&fake.records[0]==record(false)&&s.broker.entries[0].owner==one);check(p.shutdown()==MOD_OK&&fake.removes==1);s.stop();});
 scenario("provider_remains_independent_without_consumer",[]{Session s;check(s.start(none)&&fake.adds==0);s.broker.close_admission();check(s.broker.external_count()==0);s.stop();});
 scenario("json_field_conflict_no_duplicate",[]{Session s;s.start(field);devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_CONFLICT&&fake.adds==1&&s.broker.external_count()==0);s.stop();});
 scenario("json_north_plus_consumer_field_two",[]{Session s;s.start(north);const auto before=editor7::serialize(s.core.config,s.core.config.active());devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_OK&&fake.adds==2&&fake.records[0]==record(true)&&fake.records[1]==record(false));check(before==editor7::serialize(s.core.config,s.core.config.active()));p.shutdown();check(s.core.registration.handles[0]&&fake.live[0]);s.stop();});
 scenario("none_after_consumer_removed_cold_start",[]{Session s;s.start(none);devf_probe::Probe p;p.initialize(one,&host,&api);p.shutdown();s.stop();fake={};Session next;next.start(none);check(fake.adds==0);next.stop();});
 scenario("manifest_required_provider_absent_model",[]{devf_probe::Probe p;check(p.initialize(one,&host,nullptr)==MOD_UNSUPPORTED&&fake.adds==0);});
 scenario("wrong_major_rejected_before_request",[]{api.header.major_version=2;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_UNSUPPORTED&&fake.adds==0);});
 scenario("short_service_rejected",[]{api.header.struct_size=8;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_UNSUPPORTED);});
 scenario("null_request_pointer_rejected",[]{api.request_placement=nullptr;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_UNSUPPORTED);});
 scenario("unpublished_deferred_service_model",[]{Session s;s.start(none);published=false;const void* out=nullptr;check(host.get_service(one,DEVF_NATIVE_SERVICE_ID,1,0,&out)==MOD_UNAVAILABLE&&!out);s.stop();});
 scenario("two_consumers_distinct_sites_owned",[]{Session s;s.start(none);auto a=devf_probe::field(),b=north_request();auto ra=receipt(),rb=receipt();check(s.broker.request(one,&a,&ra)==MOD_OK&&s.broker.request(two,&b,&rb)==MOD_OK&&fake.adds==2&&ra.placement_token!=rb.placement_token);check(s.broker.remove(two,ra.placement_token)==MOD_INVALID_ARGUMENT&&fake.removes==0);s.stop();check(fake.removed[0]==102&&fake.removed[1]==101);});
 scenario("same_owner_second_request_rejected",[]{Session s;s.start(none);auto a=devf_probe::field(),b=north_request();auto ra=receipt(),rb=receipt();s.broker.request(one,&a,&ra);check(s.broker.request(one,&b,&rb)==MOD_CONFLICT&&fake.adds==1);s.stop();});
 scenario("same_site_other_owner_rejected",[]{Session s;s.start(none);auto a=devf_probe::field();auto ra=receipt(),rb=receipt();s.broker.request(one,&a,&ra);a.placement_id[0]='b';check(s.broker.request(two,&a,&rb)==MOD_CONFLICT&&fake.adds==1);s.stop();});
 scenario("duplicate_external_id_different_site_rejected",[]{Session s;s.start(none);auto a=devf_probe::field(),b=north_request();std::memcpy(b.placement_id,a.placement_id,32);auto ra=receipt(),rb=receipt();s.broker.request(one,&a,&ra);check(s.broker.request(two,&b,&rb)==MOD_CONFLICT);s.stop();});
 scenario("duplicate_json_id_rejected",[]{Session s;s.start(north);auto r=devf_probe::field();std::memcpy(r.placement_id,"north_native",13);auto out=receipt();check(s.broker.request(one,&r,&out)==MOD_CONFLICT&&fake.adds==1);s.stop();});
 scenario("bootstrap_closed_before_consumer_update",[]{Session s;s.start(none);s.broker.close_admission();devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_UNAVAILABLE&&fake.adds==0);s.stop();});
 scenario("invalid_json_disarms_api_not_only_core",[]{Session s;s.start(none,false);devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_UNAVAILABLE&&fake.adds==0);s.stop();});
 for(const auto* which:{"profile","scene","room","layer","x","y","z","yaw","parameters","reserved","id","unterminated","short"}){
  const std::string name=std::string("invalid_request_")+which;
  scenario(name.c_str(),[&]{Session s;s.start(none);auto r=devf_probe::field();auto out=receipt();const std::string kind=which;
   if(kind=="profile")r.native_profile=511;if(kind=="scene")r.stage[0]='X';if(kind=="room")r.room=4;if(kind=="layer")r.layer=0;
   if(kind=="x")r.x=-1251;if(kind=="y")r.y=-6327;if(kind=="z")r.z=0;if(kind=="yaw")r.yaw_s16=1;
   if(kind=="parameters")r.parameters=0;if(kind=="reserved")r.reserved=1;if(kind=="id")r.placement_id[0]='/';
   if(kind=="unterminated")for(auto& c:r.placement_id)c='a';if(kind=="short")r.struct_size=8;
   check(s.broker.request(one,&r,&out)==MOD_INVALID_ARGUMENT&&fake.adds==0&&out.placement_token==0);s.stop();});
 }
 scenario("caller_context_null_self_unknown",[]{Session s;s.start(none);auto r=devf_probe::field();auto out=receipt();for(auto* c:{static_cast<ModContext*>(nullptr),ctx,unknown})check(s.broker.request(c,&r,&out)==MOD_INVALID_ARGUMENT);check(fake.adds==0);s.stop();});
 scenario("output_missing_or_short_no_add",[]{Session s;s.start(none);auto r=devf_probe::field();auto out=receipt();out.struct_size=4;check(s.broker.request(one,&r,nullptr)==MOD_INVALID_ARGUMENT&&s.broker.request(one,&r,&out)==MOD_INVALID_ARGUMENT&&!fake.adds);s.stop();});
 scenario("stage_failure_rollback_only_new_owner",[]{Session s;s.start(north);fake.error_handle_at=2;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_ERROR&&fake.removes==1&&fake.removed[0]==102&&fake.live[0]==101);s.stop();});
 scenario("stage_exception_with_handle_rollback",[]{Session s;s.start(none);fake.throw_handle_at=1;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_ERROR&&fake.removes==1&&s.broker.external_count()==0);s.stop();});
 scenario("stage_zero_handle_no_receipt",[]{Session s;s.start(none);fake.zero_at=1;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_ERROR&&!p.token);s.stop();});
 scenario("stage_duplicate_core_handle_quarantined",[]{Session s;s.start(north);fake.duplicate_at=2;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_ERROR&&s.broker.fault&&fake.removes==0&&fake.live[0]==101);s.stop();});
 scenario("detach_after_consumer_initialization_failed",[]{Session s;s.start(none);auto r=devf_probe::field();auto out=receipt();check(s.broker.request(one,&r,&out)==MOD_OK);detach(one);check(fake.removes==1&&s.broker.entries[0].owner==nullptr&&!s.broker.entries[0].handle);s.stop();});
 scenario("detach_only_correct_owner",[]{Session s;s.start(none);auto r=devf_probe::field(),n=north_request();auto a=receipt(),b=receipt();s.broker.request(one,&r,&a);s.broker.request(two,&n,&b);detach(one);check(fake.removes==1&&s.broker.entries[1].owner==two);s.stop();});
 scenario("detach_remove_failure_drops_pointer_quarantines",[]{Session s;s.start(none);devf_probe::Probe p;p.initialize(one,&host,&api);fake.remove_fail=true;detach(one);check(s.broker.entries[0].owner==nullptr&&s.broker.entries[0].token==0&&s.broker.entries[0].handle&&s.broker.fault);fake.remove_fail=false;s.stop();check(!fake.live[0]);});
 scenario("detach_during_add_cancels_receipt",[]{Session s;s.start(none);detach_during_add=true;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_UNAVAILABLE&&!p.token&&fake.removes==1&&!s.broker.entries[0].owner&&!s.broker.pending);s.stop();});
 scenario("nested_request_rejected_no_second_add",[]{Session s;s.start(none);reenter=true;devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_OK&&nested_result==MOD_CONFLICT&&fake.adds==1);p.shutdown();s.stop();});
 scenario("watch_failure_no_publication",[]{watch_fail=true;Session s;check(!s.start(none)&&!publishes&&!published&&fake.adds==0);s.stop();});
 scenario("watch_zero_no_publication",[]{watch_zero=true;Session s;check(!s.start(none)&&!published);s.stop();});
 scenario("publish_failure_cleanup_watch_and_core",[]{publish_fail=true;Session s;check(!s.start(north)&&!published&&watch_removes==1&&!fake.live[0]);});
 scenario("consumer_no_reinjection",[]{Session s;s.start(none);devf_probe::Probe p;check(p.initialize(one,&host,&api)==MOD_OK&&p.initialize(one,&host,&api)==MOD_CONFLICT&&fake.adds==1);p.shutdown();p.shutdown();check(fake.removes==1);s.stop();});
 scenario("provider_unavailable_consumer_shutdown_no_stale_call",[]{Session s;s.start(none);devf_probe::Probe p;p.initialize(one,&host,&api);s.stop();const auto n=fake.removes;check(p.shutdown()==MOD_OK&&fake.removes==n);});
 scenario("stale_ticket_rejected_after_owner_detach",[]{Session s;s.start(none);auto r=devf_probe::field();auto a=receipt(),b=receipt();s.broker.request(one,&r,&a);detach(one);check(s.broker.request(one,&r,&b)==MOD_OK&&b.placement_token>a.placement_token);check(s.broker.remove(one,a.placement_token)==MOD_INVALID_ARGUMENT);s.stop();});
 scenario("provider_shutdown_without_consumer_shutdown",[]{Session s;s.start(none);devf_probe::Probe p;p.initialize(one,&host,&api);s.stop();check(!watcher&&!s.broker.alive&&!s.broker.open&&!s.broker.entries[0].owner&&!fake.live[0]);});
 scenario("external_placement_not_serialized_in_editor_json",[]{Session s;s.start(none);const auto before=editor7::serialize(s.core.config,s.core.config.active());devf_probe::Probe p;p.initialize(one,&host,&api);const auto after=editor7::serialize(s.core.config,s.core.config.active());check(before==after&&after.find("api_probe_field")==std::string::npos);p.shutdown();s.stop();});
 std::printf("DEVF_API_LAB_PASS cases=%u game_executed=false loader_model_only=true\n",cases);return 0;
 }catch(...){std::printf("DEVF_API_LAB_FAIL completed=%u\n",cases);return 1;}
}
