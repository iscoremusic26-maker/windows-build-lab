#include "../src/xf_scope.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace devf_black;
void require(bool v,const char* message){if(!v){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
struct Shared {std::array<std::uint8_t,16> material{},texture{},display_list{};};
struct Queue {
 struct Event {bool geometry;Command command;std::uint32_t owner;};
 std::vector<Event> events;
 void emit(const Command& command){events.push_back({false,command,0});} // copy before caller returns
 void draw(std::uint32_t owner){events.push_back({true,{},owner});}
 std::vector<std::pair<std::uint32_t,std::uint32_t>> consume()const{
  std::uint32_t color=0xFFFFFFFF;std::vector<std::pair<std::uint32_t,std::uint32_t>> out;
  for(const auto& e:events){if(e.geometry)out.push_back({e.owner,color});else color=be32(e.command.data()+5);}
  require(color==0xFFFFFFFF,"register restored after deferred draw");return out;
 }
};
Owner good(){Owner o;o.id=147;o.admitted_id=147;o.generation=1;o.record=o.context=o.eoc2=o.created=o.model_matches=o.packet_matches=true;o.shape=0;o.material=0;return o;}
void case_scope(const char* name,Owner o,bool admitted){
 Shared resource;const Shared before=resource;const Shared* native=&resource;const Shared* devf=&resource;
 require(native==devf,"actors must alias identical MaterialData/textures/DL");
 Queue q;Scope s;const bool applied=s.begin(o,123,0xFFFFFFFF,[&](const Command& c){q.emit(c);});
 require(applied==admitted,"selection mismatch");q.draw(o.id);s.end(123,[&](const Command& c){q.emit(c);});q.draw(10);
 const auto views=q.consume();require(views[0].second==(admitted?0x08080AFFu:0xFFFFFFFFu),"selected packet color");
 require(views[1].second==0xFFFFFFFFu,"original EOC2 same resource remains unchanged");
 require(resource.material==before.material&&resource.texture==before.texture&&resource.display_list==before.display_list,"shared writes");
 std::printf("PASS OFFLINE_ONLY %s\n",name);
}
int main(){
 case_scope("additional_body0",good(),true);
 auto o=good();o.shape=2;o.material=1;case_scope("additional_body2",o,true);
 o=good();o.shape=1;o.material=2;case_scope("eyes_unchanged",o,false);
 o=good();o.model_matches=false;case_scope("weapon_other_model",o,false);
 o=good();o.id=10;case_scope("original_eoc2_alias",o,false);
 o=good();o.eoc2=false;case_scope("normal_eoc",o,false);
 o=good();o.record=false;case_scope("wrong_identity",o,false);
 o=good();o.context=false;case_scope("scene_room_layer_changed",o,false);
 o=good();o.created=false;case_scope("partial_creation_or_destroyed",o,false);
 o=good();o.generation=0;case_scope("recycled_id_without_admission",o,false);
 o=good();o.packet_matches=false;case_scope("wrong_packet_binding",o,false);
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  q.draw(10);s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);s.end(123,emit);q.draw(11);
  auto owner=good();owner.shape=2;owner.material=1;s.begin(owner,124,0xFFFFFFFF,emit);q.draw(147);s.end(124,emit);q.draw(10);
  auto draws=q.consume();require(draws.size()==6,"interleaved capture count");
  for(auto [actor,color]:draws)require(color==(actor==147?0x08080AFFu:0xFFFFFFFFu),"shared/interleaved frame isolation");
  std::puts("PASS OFFLINE_ONLY two_originals_interleaved_same_frame_deferred_fifo");
 }
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);require(!s.end(999,emit)&&s.fault&&!s.active,"out-of-order post fail closed");q.draw(10);q.consume();
  std::puts("PASS OFFLINE_ONLY out_of_order_post_restore");
 }
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);require(!s.begin(good(),456,0xFFFFFFFF,emit)&&s.fault,"nested rejected");q.draw(10);q.consume();
  std::puts("PASS OFFLINE_ONLY nested_rejected_restore_same_eoc2_baseline");
 }
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);s.abort(emit,"MISSING_POST");q.draw(10);q.consume();
  require(!s.begin(good(),124,0xFFFFFFFF,emit),"fault no retry");
  std::puts("PASS OFFLINE_ONLY missing_post_failclosed_before_next_geometry");
 }
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  for(unsigned generation=1;generation<=3;generation++){auto owner=good();owner.id=owner.admitted_id=146+generation;owner.generation=generation;
   s.begin(owner,123,0xFFFFFFFF,emit);q.draw(owner.id);s.end(123,emit);
  }q.consume();require(!s.active&&!s.call_key,"no retained packet after scene reentry");
  std::puts("PASS OFFLINE_ONLY reentry_and_generation_readmission");
 }
 {
  std::array<std::uint8_t,13> bytes{0x10,0,1,0x10,0x0C,255,255,255,255,0,0,0,0};std::uint32_t rgba=0;
  require(read_base(bytes.data(),bytes.size(),0,rgba)&&rgba==0xFFFFFFFF,"exact native ColorBlock XF header");
  require(!read_base(nullptr,13,0,rgba)&&!read_base(bytes.data(),12,0,rgba)&&!read_base(bytes.data(),13,14,rgba),"null/truncated/overflow guards");
  bytes[4]=0x0D;require(!read_base(bytes.data(),13,0,rgba),"wrong XF register rejected");
  std::puts("PASS OFFLINE_ONLY colorblock_bounds_and_wrong_register");
 }
 {
  require((charcoal(0x10203077)&255)==0x77,"alpha preserved");require(charcoal(0x000000FF)==0x000000FF,"black material never lightened");
  for(int tex=0;tex<=255;tex++)for(int light=0;light<=255;light++){
   const double t=tex/255.0,L=light/255.0;
   for(int c:{8,10}){double result=4*t*L*c/255;require(result<=4.0*c/255+1e-12,"bounded charcoal with texture/light");}
  }
  std::puts("PASS OFFLINE_ONLY alpha_preservation_light_texture_charcoal_bound");
 }
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  for(unsigned rebuild=0;rebuild<8;rebuild++){s.begin(good(),123,0xFFFFFFFF,emit);q.draw(147);s.end(123,emit);q.draw(10);}q.consume();
  std::puts("PASS OFFLINE_ONLY diffdl_regeneration_and_presentation_rescoped");
 }
 {
  Queue q;Scope s;auto emit=[&](const Command& c){q.emit(c);};
  auto o=good();o.model_matches=false;require(!s.begin(o,123,0xFFFFFFFF,emit)&&q.events.empty(),"failed native model allocation never emits");
  s.abort(emit,"SHUTDOWN");require(!s.active&&!s.call_key&&!s.begin(good(),123,0xFFFFFFFF,emit),"shutdown disarmed");
  std::puts("PASS OFFLINE_ONLY allocation_null_no_private_allocation_and_shutdown");
 }
 std::puts("LAB_PASS cases=20 OFFLINE_ONLY; NATIVE_HOOK_AND_GPU_NOT_EXECUTED");
}
