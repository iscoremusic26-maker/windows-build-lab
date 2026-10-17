// CP10.2 offline lab: tint scope + multi-registration HP/identity policy. Pure logic, no game, no GPU.
#include "../expansion/xf_scope.hpp"
#include "../expansion/black_policy.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace devf_black;
unsigned cases=0;
void require(bool v,const char* message){if(!v){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
void pass(const char* name){++cases;std::printf("CASE_PASS %s\n",name);}
struct Queue {
 struct Event {bool geometry;Command command;std::uint32_t owner;};
 std::vector<Event> events;
 void emit(const Command& command){events.push_back({false,command,0});}
 void draw(std::uint32_t owner){events.push_back({true,{},owner});}
 std::vector<std::pair<std::uint32_t,std::uint32_t>> consume()const{
  std::uint32_t color=0xFFFFFFFF;std::vector<std::pair<std::uint32_t,std::uint32_t>> out;
  for(const auto& e:events){if(e.geometry)out.push_back({e.owner,color});else color=be32(e.command.data()+5);}
  require(color==0xFFFFFFFF,"register restored after deferred draw");return out;
 }
};
Owner good(std::uint32_t id=147){Owner o;o.id=id;o.admitted_id=id;o.generation=1;o.record=o.context=o.eoc2=o.created=o.model_matches=o.packet_matches=true;o.shape=0;o.material=0;return o;}
constexpr std::uint32_t kExpected=(kTintR<<24)|(kTintG<<16)|(kTintB<<8)|0xFFu;
void scope_case(const char* name,Owner o,bool admitted){
 Queue q;Scope s;const bool applied=s.begin(o,123,0xFFFFFFFF,[&](const Command& c){q.emit(c);});
 require(applied==admitted,"selection mismatch");q.draw(o.id);s.end(123,[&](const Command& c){q.emit(c);});q.draw(10);
 const auto views=q.consume();require(views[0].second==(admitted?kExpected:0xFFFFFFFFu),"selected packet color");
 require(views[1].second==0xFFFFFFFFu,"other geometry stays native");pass(name);
}
// ---- policy helpers ----
using namespace devf_black2;
Snapshot snap(const Reg& r,std::uint32_t id,bool initializing=true,std::int16_t health=220){
 Snapshot s;s.id=id;s.parameters=kParameters;s.profile=510;s.room=r.room;s.layer=r.layer;s.set_id=0xFFFF;s.x=r.x;s.y=r.y;s.z=r.z;
 s.home_x=kRangeCode;s.home_y=r.yaw;s.home_z=0;s.stage=true;s.initializing=initializing;s.resource_eoc2=true;s.health=health;return s;
}
Table two(){Table t;require(t.add("black-a",1,6,-25050.2812f,-8097.86523f,80595.0703f,0,150),"add a");require(t.add("black-b",3,6,-1334.75342f,-6339.34229f,60011.2109f,-2421,321),"add b");t.armed=true;return t;}
int main(){
 // tint strength: legible charcoal, never crushed black
 {
  require(kExpected==0x5A5A62FFu,"documented tint constants");
  require((charcoal(0x10203077)&255)==0x77,"alpha preserved");require(charcoal(0x000000FF)==0x000000FF,"black material never lightened");
  require(((charcoal(0xFFFFFFFF)>>24)&255)>=60&&((charcoal(0xFFFFFFFF)>>24)&255)<=120,"not crushed black and clearly darker than native");
  for(int tex=0;tex<=255;tex++)for(int light=0;light<=255;light++){
   const double t=tex/255.0,L=light/255.0;
   for(int c:{int(kTintR),int(kTintB)}){double result=4*t*L*c/255;require(result<=4.0*c/255+1e-12,"bounded charcoal with texture/light");}
  }
  pass("tint_legible_not_crushed_alpha_preserved");
 }
 scope_case("body0_tinted",good(),true);
 {auto o=good();o.shape=2;o.material=1;scope_case("body2_tinted",o,true);}
 {auto o=good();o.shape=1;o.material=2;scope_case("eyes_unchanged",o,false);}
 {auto o=good();o.model_matches=false;scope_case("weapon_other_model",o,false);}
 {auto o=good();o.id=10;scope_case("normal_bokoblin_alias_untouched",o,false);}
 {auto o=good();o.eoc2=false;scope_case("normal_eoc",o,false);}
 {auto o=good();o.record=false;scope_case("wrong_identity",o,false);}
 {auto o=good();o.generation=0;scope_case("recycled_id_without_admission",o,false);}
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  q.draw(10);s.begin(good(147),123,0xFFFFFFFF,emit);q.draw(147);s.end(123,emit);q.draw(11);
  s.begin(good(148),124,0xFFFFFFFF,emit);q.draw(148);s.end(124,emit);q.draw(10);
  auto draws=q.consume();require(draws.size()==5,"five interleaved geometries");
  for(auto [actor,color]:draws)require(color==((actor==147||actor==148)?kExpected:0xFFFFFFFFu),"two black owners and normals do not bleed");
  pass("two_black_owners_interleaved_with_normals");
 }
 {Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);require(!s.end(999,emit)&&s.fault&&!s.active,"out-of-order post fail closed");q.draw(10);q.consume();pass("out_of_order_post_restore");}
 {Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);require(!s.begin(good(),456,0xFFFFFFFF,emit)&&s.fault,"nested rejected");q.draw(10);q.consume();pass("nested_rejected_restore");}
 {std::array<std::uint8_t,13> bytes{0x10,0,1,0x10,0x0C,255,255,255,255,0,0,0,0};std::uint32_t rgba=0;
  require(read_base(bytes.data(),bytes.size(),0,rgba)&&rgba==0xFFFFFFFF,"exact native ColorBlock XF header");
  require(!read_base(nullptr,13,0,rgba)&&!read_base(bytes.data(),12,0,rgba)&&!read_base(bytes.data(),13,14,rgba),"guards");bytes[4]=0x0D;require(!read_base(bytes.data(),13,0,rgba),"wrong XF register");pass("colorblock_bounds_and_wrong_register");}
 // ---- policy: records, first execute, HP, isolation ----
 {Table t=two();auto s=snap(t.regs[0],200);int i=t.find(s);require(i==0,"find a");require(t.apply(0,s)==Decision::Applied&&t.regs[0].admitted_id==200&&t.regs[0].hp==150,"applied a");pass("policy_first_execute_applies_registered_hp");}
 {Table t=two();auto s=snap(t.regs[1],300);require(t.find(s)==1&&t.apply(1,s)==Decision::Applied&&t.regs[1].hp==321,"b own hp");require(t.regs[0].admitted_id==0,"a untouched");pass("policy_second_registration_has_own_hp");}
 {Table t=two();auto s=snap(t.regs[0],200);t.apply(0,s);auto again=snap(t.regs[0],200,false,150);require(t.apply(0,again)==Decision::Ignore&&!t.regs[0].fault,"regular execute ignored");pass("policy_regular_execute_never_rewrites_hp");}
 {Table t=two();auto s=snap(t.regs[0],200);s.x+=0.001f;require(t.find(s)<0,"one-ulp-ish position difference");s=snap(t.regs[0],200);s.parameters=0;require(t.find(s)<0,"params");s=snap(t.regs[0],200);s.home_x=0x0106;require(t.find(s)<0,"legacy range code not matched");s=snap(t.regs[0],200);s.home_y=7;require(t.find(s)<0,"yaw");s=snap(t.regs[0],200);s.room=2;require(t.find(s)<0,"room");s=snap(t.regs[0],200);s.layer=5;require(t.find(s)<0,"layer");s=snap(t.regs[0],200);s.stage=false;require(t.find(s)<0,"stage");s=snap(t.regs[0],200);s.profile=511;require(t.find(s)<0,"profile");s=snap(t.regs[0],200);s.set_id=3;require(t.find(s)<0,"setid");pass("policy_exact_record_context_match_only");}
 {Table t=two();auto s=snap(t.regs[0],200);s.initializing=false;require(t.apply(0,s)==Decision::Fault&&t.regs[0].fault&&!t.regs[0].admitted_id,"not first execute");require(!t.regs[1].fault,"other registration unaffected");require(t.find(snap(t.regs[0],201))<0,"faulted registration ignored");pass("policy_fault_is_per_registration");}
 {Table t=two();auto s=snap(t.regs[0],200,true,150);require(t.apply(0,s)==Decision::Fault,"hp not native");pass("policy_non_native_hp_rejected");}
 {Table t=two();auto s=snap(t.regs[0],200);s.resource_eoc2=false;require(t.apply(0,s)==Decision::Fault,"resource");pass("policy_non_E_OC2_resource_rejected");}
 {Table t=two();t.apply(0,snap(t.regs[0],200));auto s=snap(t.regs[0],201);s.previous_live=true;require(t.apply(0,s)==Decision::Fault,"ambiguous concurrent");pass("policy_ambiguous_concurrent_instance_rejected");}
 {Table t=two();t.apply(0,snap(t.regs[0],200));auto s=snap(t.regs[0],150);require(t.apply(0,s)==Decision::Fault,"reused id");pass("policy_reused_or_older_id_rejected");}
 {Table t=two();t.apply(0,snap(t.regs[0],200));auto s=snap(t.regs[0],260);s.previous_live=false;require(t.apply(0,s)==Decision::Applied&&t.regs[0].generation==2&&t.regs[0].admitted_id==260,"re-entry generation");pass("policy_reentry_readmission_new_generation");}
 {Table t=two();t.armed=false;require(t.apply(0,snap(t.regs[0],200))==Decision::Ignore,"disarmed");pass("policy_disarmed_never_writes");}
 {Table t;require(t.add("a",1,6,1,2,3,0,150)&&!t.add("dup",1,6,1,2,3,0,150),"duplicate context position");require(t.add("other-layer",1,7,1,2,3,0,150),"same xyz other layer");require(!t.add("bad-hp",1,6,9,9,9,0,0)&&!t.add("bad-hp",1,6,9,9,9,0,1000)&&!t.add("",1,6,9,9,9,0,10),"hp/id guards");
  Table full;for(unsigned i=0;i<capacity;++i)require(full.add("x",1,6,float(i),0,0,0,10),"fill");require(!full.add("x",1,6,99,0,0,0,10),"capacity eight");pass("policy_registration_guards_capacity_eight");}
 {Table t;require(!t.any_admitted(),"none admitted");t.add("a",1,6,1,2,3,0,150);t.armed=true;require(!t.any_admitted(),"registered but not admitted");pass("policy_no_admission_means_no_hot_path_work");}
 std::printf("BLACK_LAB_PASS cases=%u OFFLINE_ONLY; NATIVE_HOOK_AND_GPU_NOT_EXECUTED\n",cases);
}
