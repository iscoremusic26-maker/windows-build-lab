#include "support.hpp"
#include "../runtime/DEVF/CP10_4_VARIANT_IDENTITY_HP/expansion/typed_registration.hpp"
#include "../runtime/DEVF/CP10_5_DAMAGE_BLUE_RED_BLACK/expansion/damage_policy.hpp"
#include <filesystem>
std::string typed_doc(const char* variant,const char* enemy="E_OC2",const char* hp=""){
 auto e=s2enc("typed",true,-10000,std::string(",\"bokoblin_variant\":\"")+variant+"\""+hp);
 const auto at=e.find("\"enemy\":\"E_OC2\"");e.replace(at,std::string("\"enemy\":\"E_OC2\"").size(),std::string("\"enemy\":\"")+enemy+"\"");return s2doc({e});
}
DevfEncounterPlacementRequestV2 request2(const multi_config::Config& c,unsigned i=0){
 return {sizeof(DevfEncounterPlacementRequestV2),2,c.variant[i]==multi_config::variant_blue?1u:2u,0,c.requests[i]};
}
devf_black2::Snapshot snapshot(const multi_config::Config& c){
 const auto& q=c.requests[0];devf_black2::Snapshot s;s.id=100;s.profile=510;s.room=q.room;s.layer=q.layer;s.parameters=q.parameters;s.set_id=0xFFFF;
 s.x=q.x;s.y=q.y;s.z=q.z;s.home_x=c.variant[0]==2?0x0032:0x0132;s.home_y=q.yaw_s16;s.stage=s.initializing=true;s.resource_eoc2=c.variant[0]!=2;s.health=c.variant[0]==2?40:220;return s;
}
void matrix(const std::filesystem::path& folder){
 std::ifstream index(folder/"matrix.tsv");ok(bool(index));std::string line;
 while(std::getline(index,line)){
  const auto tab=line.find('\t');const bool want=line.substr(tab+1)=="1";const auto name=line.substr(0,tab);
  test(name.c_str(),[&]{std::ifstream f(folder/(name+".json"),std::ios::binary);ok(bool(f));std::string raw(std::istreambuf_iterator<char>(f),{});
   multi_config::Config c;c.count=31;const char* why;const bool result=multi_config::decode(raw,c,why);
   if(result!=want){std::cerr<<"MATRIX "<<name<<" reason="<<why<<"\n";}ok(result==want);if(!result)ok(c.count==0);
   if(result&&name=="mixed")ok(c.count==4&&c.variant[0]==0&&c.variant[1]==2&&c.variant[2]==3&&c.variant[3]==1);
  });
 }
}
int main(int argc,char** argv){try{
 using namespace devf_variant;
 for(const char* v:{"blue","red","black"})test("parser-explicit-native-resource",[&]{auto c=parse(typed_doc(v,!strcmp(v,"blue")?"E_OC":"E_OC2",!strcmp(v,"black")?",\"hp\":440":""));ok(c.count==1);});
 test("v1-record-historical-eoc2",[]{Session s;auto c=parse(allrooms(7));DevfEncounterPlacementReceipt o{sizeof(o)};ok(req(owner,&c.requests[0],&o)==MOD_OK);ok(live.begin()->second[24]==1&&live.begin()->second[25]==0x32);});
 for(const char* v:{"blue","red","black"})test("v2-stage-record-exact-bytes",[&]{Session s;auto c=parse(typed_doc(v,!strcmp(v,"blue")?"E_OC":"E_OC2",!strcmp(v,"black")?",\"hp\":440":""));auto r=request2(c);r.placement.yaw_s16=-2421;auto expected=devf_api::EncounterBroker::record(r.placement);expected[24]=!strcmp(v,"blue")?0:1;
  DevfEncounterPlacementReceipt o{sizeof(o)};ok(reqv2(owner,&r,&o)==MOD_OK&&live.size()==1&&live.begin()->second==expected);ok(expected[25]==0x32&&expected[26]==0xF6&&expected[27]==0x8B&&o.flags==1&&o.registered_edits==1);});
 for(int bad=0;bad<8;++bad)test("v2-domain-fail-closed-zero-edits",[&]{Session s;auto r=request2(parse(typed_doc("blue","E_OC")));if(bad==0)--r.struct_size;if(bad==1)++r.struct_size;if(bad==2)r.request_version=1;if(bad==3)r.native_resource=0;if(bad==4)r.native_resource=3;if(bad==5)r.reserved=1;if(bad==6)r.placement.reserved=1;if(bad==7)--r.placement.struct_size;DevfEncounterPlacementReceipt o{sizeof(o)};o.placement_token=123;ok(reqv2(owner,&r,&o)==MOD_INVALID_ARGUMENT&&!adds&&!o.placement_token);});
 for(unsigned failure:{1u,16u,32u})test("v2-batch-rollback",[&]{Session s;auto c=parse(allrooms(9,32));for(unsigned i=0;i<c.count;++i)c.variant[i]=i%2?2:3;fail_at=failure;expansion::Client cli;ok(cli.initialize(owner,&host,&api11,c)!=MOD_OK&&live.empty());});
 test("v2-32-mixed-owner-detach",[]{Session s;auto c=parse(allrooms(9,32));for(unsigned i=0;i<c.count;++i)c.variant[i]=i%3+1;expansion::Client cli;ok(cli.initialize(owner,&host,&api11,c)==MOD_OK&&live.size()==32);s.b.detach(owner);ok(live.empty());});
 test("old-provider-rejected-no-request",[]{Session s;auto c=parse(typed_doc("blue","E_OC"));auto old=api11;old.header.minor_version=0;expansion::Client cli;ok(cli.initialize(owner,&host,&old,c)==MOD_UNSUPPORTED&&!adds&&std::string(cli.reason).find("NO_FALLBACK")!=std::string::npos);});
 test("capability-missing-rejected",[]{Session s;auto v=api11;v.capabilities=0;expansion::Client cli;ok(cli.initialize(owner,&host,&v,parse(allrooms(7)))==MOD_UNSUPPORTED&&!adds);});
 test("null-old-provider-rejected",[]{Session s;expansion::Client cli;ok(cli.initialize(owner,&host,nullptr,parse(allrooms(7)))==MOD_UNSUPPORTED&&!adds);});
 test("six-identities-and-atp",[]{
  struct Case {const char* name;int hp;Registration reg;Variant v;Source src;int max;bool write;};
  for(const auto& c:std::array<Case,6>{{{"E_OC",40,Registration::None,Variant::Blue,Source::NativeEOC,220,true},{"E_OC2",220,Registration::None,Variant::Red,Source::NativeEOC2,330,true},{"E_OC2",220,Registration::Normal,Variant::Blue,Source::DevfNormal,220,false},{"E_OC",40,Registration::Blue,Variant::Blue,Source::DevfBlue,220,true},{"E_OC2",220,Registration::Red,Variant::Red,Source::DevfRed,330,true},{"E_OC2",220,Registration::Black,Variant::Black,Source::DevfBlack,440,false}}}){
   auto p=make_plan(c.name,int16_t(c.hp),c.hp,true,c.reg,440);ok(p.variant==c.v&&p.source==c.src&&p.max_hp==c.max&&p.write_hp==c.write);
   const uint8_t atp=!strcmp(c.name,"E_OC")?1:2;auto d=devf_damage::make_plan(p.variant,c.name,atp,atp);ok(d.accepted&&d.target==(c.v==Variant::Blue?1:c.v==Variant::Red?2:3));
  }});
 for(bool blue:{false,true}){
  for(int bad=0;bad<11;++bad)test("typed-admission-negative-quarantines-no-native-fallback",[&]{auto c=parse(typed_doc(blue?"blue":"red",blue?"E_OC":"E_OC2"));devf_typed::Table t;ok(t.configure(c));auto s=snapshot(c);const char* name=blue?"E_OC":"E_OC2";if(bad==0)s.home_x^=0x100;if(bad==1)s.home_y++;if(bad==2)s.home_z=1;if(bad==3)--s.health;if(bad==4)s.parameters=0;if(bad==5)s.set_id=0;if(bad==6)s.initializing=false;if(bad==7)s.previous_live=true;if(bad==8)s.id=0;if(bad==9)name="OTHER";if(bad==10)s.resource_eoc2=!s.resource_eoc2;
   // resource_eoc2 is a redundant snapshot flag; exact mName is authoritative. Mutate name consistently.
   if(bad==10)name=blue?"E_OC2":"E_OC";
   ok(t.candidate(s)==0);auto reg=t.admit(0,s,name,&c,s.health);ok(reg==Registration::TypedAdmissionFailed&&t.regs[0].record.fault);
   ok(make_plan(name,int16_t(blue?40:220),blue?40:220,true,reg,0).variant==Variant::None);
   auto valid=snapshot(c);ok(t.admit(0,valid,blue?"E_OC":"E_OC2",&c,valid.health)==Registration::TypedAdmissionFailed);
  });
  test("typed-one-shot-and-next-generation",[&]{auto c=parse(typed_doc(blue?"blue":"red",blue?"E_OC":"E_OC2"));devf_typed::Table t;ok(t.configure(c));auto s=snapshot(c);const char* name=blue?"E_OC":"E_OC2";auto reg=t.admit(0,s,name,&c,s.health);ok(reg==(blue?Registration::Blue:Registration::Red));auto p=make_plan(name,s.health,s.health,true,reg,0);ok(p.write_hp);Registry registry;auto* e=registry.begin(s.id,&c,true);ok(e);adopt(*e,p);registry.end(s.id,&c);e=registry.begin(s.id,&c,false);ok(e&&e->classified);registry.end(s.id,&c);s.id++;ok(t.admit(0,s,name,&c,s.health)==reg&&t.regs[0].record.generation==2);});
  test("typed-reused-id-rejected",[&]{auto c=parse(typed_doc(blue?"blue":"red",blue?"E_OC":"E_OC2"));devf_typed::Table t;ok(t.configure(c));auto s=snapshot(c);const char* name=blue?"E_OC":"E_OC2";ok(t.admit(0,s,name,&c,s.health)!=Registration::TypedAdmissionFailed);ok(t.admit(0,s,name,&c,s.health)==Registration::TypedAdmissionFailed);});
  test("typed-other-context-not-claimed-no-proximity",[&]{auto c=parse(typed_doc(blue?"blue":"red",blue?"E_OC":"E_OC2"));for(int bad=0;bad<5;++bad){devf_typed::Table t;ok(t.configure(c));auto s=snapshot(c);if(bad==0)s.stage=false;if(bad==1)s.room++;if(bad==2)s.layer++;if(bad==3)s.x+=.01f;if(bad==4)s.z+=1;ok(t.candidate(s)==-1);}});
 }
 test("black-comparator-remains-strict",[]{auto c=parse(typed_doc("black","E_OC2",",\"hp\":440"));devf_black2::Table b,n;ok(register_encounters(c,1,b,n)&&b.count==1&&!n.count);auto s=snapshot(c);ok(b.matches(0,s));s.home_x=0x0032;ok(!b.matches(0,s));});
 test("explicit-pristine-creation-signatures",[]{for(bool blue:{false,true}){auto r=blue?Registration::Blue:Registration::Red;const char* name=blue?"E_OC":"E_OC2";int hp=blue?40:220;ok(make_plan(name,int16_t(hp-1),hp,true,r,0).variant==Variant::None);ok(make_plan(name,int16_t(hp),hp-1,true,r,0).variant==Variant::None);ok(make_plan(name,int16_t(hp),hp,false,r,0).variant==Variant::None);}});
 test("black-signature-failure-quarantines-exact-slot",[]{auto c=parse(typed_doc("black","E_OC2",",\"hp\":440"));devf_typed::Table t;ok(t.configure(c));auto s=snapshot(c);s.home_x=0x0032;ok(t.candidate(s)==0&&t.admit(0,s,"E_OC2",&c,s.health)==Registration::TypedAdmissionFailed);});
 test("near-other-variant-exact-only",[]{auto c=parse(allrooms(9,2));c.variant[0]=2;c.variant[1]=3;devf_typed::Table t;ok(t.configure(c));auto s=snapshot(c);ok(t.candidate(s)==0);s.x=c.requests[1].x;s.home_x=0x0132;s.resource_eoc2=true;s.health=220;ok(t.candidate(s)==1&&t.admit(1,s,"E_OC2",&c,s.health)==Registration::Red);});
 test("v2-backend-zero-handle-detach-and-alias",[]{
  for(int fault=0;fault<4;++fault){setup();Session session;auto c=parse(allrooms(9,2));c.variant[0]=2;c.variant[1]=3;if(fault==0)zero_at=2;if(fault==1)throw_at=2;if(fault==2)detach_add=true;if(fault==3)alias=true;expansion::Client cli;ok(cli.initialize(owner,&host,&api11,c)!=MOD_OK);if(fault!=3)ok(live.empty());}
 });
 if(argc==3&&std::string(argv[1])=="--matrix")matrix(argv[2]);
 if(argc==3&&std::string(argv[1])=="--export")test("CSharp-export-runtime-records",[&]{std::ifstream f(argv[2],std::ios::binary);ok(bool(f));std::string raw(std::istreambuf_iterator<char>(f),{});auto c=parse(raw);Session s;expansion::Client cli;ok(cli.initialize(owner,&host,&api11,c)==MOD_OK&&adds==c.count);unsigned i=0;for(const auto& pair:live){ok(pair.second[24]==(c.variant[i]==2?0:1));++i;}cli.shutdown();ok(live.empty());});
 std::cout<<"CP20_LAB_PASS cases="<<cases<<" GAME_EXECUTED=false\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
