// CP10.5 offline lab: attack-power POLICY and its configuration flow. Pure logic: no game, no GPU, no Link.
// EVIDENCE SCOPE: this proves which values the module decides to write into the two attack spheres, when, how often and under which
// ownership rules. It does NOT prove how many hearts Link loses; that needs the human Windows round. The "Link model" cases only mirror
// the host source (d_cc_s.cpp / c_cc_d.cpp / d_a_alink_damage.inc) to document the contract; they are labelled MODEL_OF_SOURCE.
#include "../expansion/damage_policy.hpp"
#include "../../CP10_4_VARIANT_IDENTITY_HP/expansion/variant_registration.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace devf_damage;
namespace vp=devf_variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
// Fake actor: the two spheres, the native creation values, and write counters (the "game" side of the contract).
struct Actor {
 std::uint32_t id=1;const char* name="E_OC2";std::uint8_t atp[2]{2,2};int health=220;std::int16_t f560=220;unsigned writes=0;
 void set(std::uint8_t v){atp[0]=atp[1]=v;++writes;}
};
Actor native(const char* name,std::uint32_t id=1){Actor a;a.id=id;a.name=name;const bool oc=!std::strcmp(name,"E_OC");a.atp[0]=a.atp[1]=oc?1:2;a.health=a.f560=oc?40:220;return a;}
struct Outcome {vp::Plan identity;Plan damage;bool applied=false;};
// Mirror of the module's first-execute flow: CP10.4 classification, then CP10.5 on an accepted actor only (damage_apply in mod.cpp).
Outcome first_execute(Actor& a,vp::Registry& reg,Ledger& led,vp::Registration r,int black_hp,bool initializing=true,bool force_readback_fail=false){
 Outcome o;
 vp::Entry* e=reg.begin(a.id,&a,initializing);
 require(e!=nullptr,"registry entry");
 if(e->classified){reg.end(a.id,&a);return o;}        // later executes: classification (and so damage) is never repeated
 o.identity=vp::make_plan(a.name,a.f560,a.health,initializing,r,black_hp);vp::adopt(*e,o.identity);
 if(o.identity.variant!=Variant::None){
  if(auto* old=led.find(a.id))led.release(*old);
  o.damage=make_plan(e->variant,a.name,a.atp[0],a.atp[1]);
  if(o.damage.accepted){
   Entry* rec=led.record(a.id,&a,e->generation,e->variant,o.damage);
   if(!rec){o.damage.accepted=false;o.damage.why=Why::LedgerFull;}
   else{
    if(o.damage.write)a.set(o.damage.target);
    if(force_readback_fail){a.atp[1]=77;}
    if(a.atp[0]!=o.damage.target||a.atp[1]!=o.damage.target){a.atp[0]=a.atp[1]=o.damage.native;led.release(*rec);o.damage.accepted=false;o.damage.why=Why::ReadbackMismatch;}
    else{rec->written=o.damage.write;o.applied=true;}
   }
  }
 }else o.damage=make_plan(Variant::None,a.name,a.atp[0],a.atp[1]);
 reg.end(a.id,&a);
 return o;
}
// MODEL_OF_SOURCE (not the game): the Link-side contract read from the host source.
int model_link_units(std::initializer_list<int> atps_hitting_in_one_frame,float magnification){
 int dmg=0;for(int a:atps_hitting_in_one_frame)dmg=std::max(dmg,a);        // cCcD_Stts::PlusDmg keeps the maximum (c_cc_d.cpp L263)
 float m=float(dmg)*magnification;int units=int(m);if(int(m*10.0f)%10!=0)++units;   // daAlink_c::setDamagePoint (d_a_alink_damage.inc L178-190)
 return units;
}
int main(){
 static_assert(kAtpBlue==1&&kAtpRed==2&&kAtpBlack==3&&kNativeAtpE_OC==1&&kNativeAtpE_OC2==2&&kSpheres==2,"attack table");
 static_assert(target_for(Variant::Blue)==1&&target_for(Variant::Red)==2&&target_for(Variant::Black)==3&&target_for(Variant::None)==0,"target_for");
 using R=vp::Registration;
 // ---------- A. value policy per identity row ----------
 {Actor a=native("E_OC");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::None,0);
  require(o.identity.source==vp::Source::NativeEOC&&o.identity.variant==Variant::Blue&&o.applied&&o.damage.native==1&&o.damage.target==1&&!o.damage.write&&a.atp[0]==1&&a.atp[1]==1&&a.writes==0,"native Blue");pass("native_E_OC_BLUE_reference_1_no_write_needed");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::Normal,0);
  require(o.identity.source==vp::Source::DevfNormal&&o.identity.variant==Variant::Blue&&o.applied&&o.damage.native==2&&o.damage.target==1&&o.damage.write&&a.atp[0]==1&&a.atp[1]==1&&a.writes==1,"DEVF Normal");pass("DEVF_NORMAL_physically_E_OC2_is_normalised_from_2_to_BLUE_1_on_both_spheres");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::None,0);
  require(o.identity.source==vp::Source::NativeEOC2&&o.identity.variant==Variant::Red&&o.applied&&o.damage.native==2&&o.damage.target==2&&!o.damage.write&&a.atp[0]==2&&a.atp[1]==2&&a.writes==0,"native Red");pass("native_E_OC2_RED_reference_2_stays_2_no_second_multiplier");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::Black,440);
  require(o.identity.source==vp::Source::DevfBlack&&o.identity.variant==Variant::Black&&o.applied&&o.damage.native==2&&o.damage.target==3&&o.damage.write&&a.atp[0]==3&&a.atp[1]==3&&a.atp[0]!=6,"Black");pass("DEVF_BLACK_physically_E_OC2_goes_from_2_to_3_not_6");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::Black,150);require(o.applied&&a.atp[0]==3&&a.atp[1]==3,"hp does not change damage");
  Actor b=native("E_OC2",2);auto p=first_execute(b,reg,led,R::Black,999);require(p.applied&&b.atp[0]==3,"black hp 999");pass("black_damage_is_independent_of_the_configured_hp");}
 {require(target_for(Variant::Red)==2*target_for(Variant::Blue)&&target_for(Variant::Black)==3*target_for(Variant::Blue),"1:2:3");pass("targets_are_1_2_3_relative_to_BLUE");}
 // ---------- B. fail closed ----------
 {struct C{const char* name;std::uint8_t a0,a1;Variant v;Why why;};
  const C table[]{{"E_OC",1,1,Variant::None,Why::NoVariant},{"E_GOB",2,2,Variant::Blue,Why::UnknownName},{nullptr,1,1,Variant::Blue,Why::UnknownName},{"E_OC3",2,2,Variant::Blue,Why::UnknownName},
   {"E_OC",1,1,Variant::Red,Why::VariantResourceMismatch},{"E_OC",1,1,Variant::Black,Why::VariantResourceMismatch},
   {"E_OC2",2,3,Variant::Red,Why::SpheresDisagree},{"E_OC2",3,2,Variant::Black,Why::SpheresDisagree},{"E_OC",1,2,Variant::Blue,Why::SpheresDisagree},
   {"E_OC2",3,3,Variant::Black,Why::NativeAtpMismatch},{"E_OC2",1,1,Variant::Blue,Why::NativeAtpMismatch},{"E_OC",2,2,Variant::Blue,Why::NativeAtpMismatch},{"E_OC2",0,0,Variant::Red,Why::NativeAtpMismatch},{"E_OC2",255,255,Variant::Red,Why::NativeAtpMismatch}};
  for(const auto& c:table){const auto p=make_plan(c.v,c.name,c.a0,c.a1);require(!p.accepted&&!p.write&&p.target==0&&p.why==c.why,"rejected plan has no target and no write");}
  pass("rejections_fail_closed_no_target_no_write");}
 {// CP10.4 rejections never reach CP10.5: colliders byte-identical, ledger empty
  struct C{const char* name;std::int16_t f560;int health;bool init;R reg;int bh;};
  const C table[]{{"E_GOB",220,220,true,R::None,0},{"E_OC2",300,220,true,R::None,0},{"E_OC2",220,200,true,R::None,0},{"E_OC2",220,220,false,R::None,0},{"E_OC2",220,220,true,R::BlackAdmissionFailed,440},{"E_OC",40,40,true,R::Black,440},{"E_OC2",220,220,true,R::Black,0}};
  for(const auto& c:table){Actor a=native("E_OC2");a.name=c.name;a.f560=c.f560;a.health=c.health;vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,c.reg,c.bh,c.init);
   require(o.identity.variant==Variant::None&&!o.applied&&a.writes==0&&led.used==0&&a.atp[0]==2&&a.atp[1]==2,"colliders untouched");}
  pass("rejected_by_CP10_4_never_modified_and_never_degraded_to_RED_or_BLUE");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::BlackAdmissionFailed,440);require(o.identity.why==vp::Why::BlackAdmissionRejected&&a.atp[0]==2&&a.writes==0,"failed Black keeps native 2");pass("failed_black_admission_keeps_native_collider_power");}
 {Actor a=native("E_OC2");a.atp[1]=5;vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::None,0);require(o.identity.variant==Variant::Red&&!o.applied&&o.damage.why==Why::SpheresDisagree&&a.atp[0]==2&&a.atp[1]==5&&a.writes==0,"third-party sphere untouched");
  Actor b=native("E_OC2");b.atp[0]=b.atp[1]=9;auto p=first_execute(b,reg,led,R::None,0);require(!p.applied&&p.damage.why==Why::NativeAtpMismatch&&b.atp[0]==9&&b.writes==0,"modified native power untouched");pass("pre_modified_colliders_are_not_overwritten_but_hp_identity_unaffected");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;auto o=first_execute(a,reg,led,R::Black,440,true,true);require(!o.applied&&o.damage.why==Why::ReadbackMismatch&&a.atp[0]==2&&a.atp[1]==2&&led.used==0,"readback failure restores native");pass("readback_mismatch_restores_exact_native_values_and_drops_record");}
 {vp::Registry reg;Ledger led;std::vector<Actor> actors;actors.reserve(kCapacity+2);for(unsigned i=0;i<kCapacity;++i)actors.push_back(native("E_OC2",100+i));
  for(unsigned i=0;i<kCapacity;++i){auto o=first_execute(actors[i],reg,led,R::None,0);require(o.applied,"fills");}
  // registry (64) is the first gate; ledger alone is also bounded: fill it directly then ask for one more
  Ledger full;Plan p=make_plan(Variant::Red,"E_OC2",2,2);int dummy[kCapacity+1];for(unsigned i=0;i<kCapacity;++i)require(full.record(500+i,&dummy[i],i+1,Variant::Red,p)!=nullptr,"ledger fill");
  require(full.record(9999,&dummy[kCapacity],99,Variant::Red,p)==nullptr&&full.used==kCapacity,"ledger full refuses");
  Actor extra=native("E_OC2",9000);vp::Registry reg2;auto o=first_execute(extra,reg2,full,R::Black,440);require(!o.applied&&o.damage.why==Why::LedgerFull&&extra.atp[0]==2&&extra.writes==0,"full ledger: nothing written");pass("ledger_bounded_fail_closed_no_write_when_full");}
 // ---------- C. when and how often ----------
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;first_execute(a,reg,led,R::Black,440);const unsigned w=a.writes;
  for(unsigned t=0;t<600;++t){auto o=first_execute(a,reg,led,R::Black,440,false);require(!o.applied&&a.atp[0]==3&&a.atp[1]==3,"unchanged");}
  require(a.writes==w&&w==1,"exactly one write across 600 later executes");pass("idempotent_one_write_over_hundreds_of_executes_no_accumulation");}
 {Actor a=native("E_OC2");vp::Registry reg;Ledger led;vp::Entry* e=reg.begin(a.id,&a,false);vp::adopt(*e,vp::make_plan("E_OC2",220,220,true,R::None,0));reg.end(a.id,&a); // classified earlier, now mid-combat
  a.atp[0]=a.atp[1]=2;const unsigned w=a.writes;auto o=first_execute(a,reg,led,R::Black,440,false);require(!o.applied&&a.writes==w&&a.atp[0]==2,"already classified actor is not rewritten");
  Actor b=native("E_OC2",2);vp::Registry reg2;Ledger led2;auto q=first_execute(b,reg2,led2,R::Black,440,false);require(!q.applied&&b.writes==0&&b.atp[0]==2&&led2.used==0,"never seen at creation: not touched mid-combat");pass("actor_not_seen_at_creation_or_already_classified_is_never_rewritten");}
 {vp::Registry reg;Ledger led;Actor a=native("E_OC2",7);first_execute(a,reg,led,R::Black,440);require(a.atp[0]==3&&led.used==1,"gen1");
  // same id, new creation (stale release path): no inheritance of ledger or collider state
  Actor b=native("E_OC2",7);auto o=first_execute(b,reg,led,R::None,0);require(o.identity.variant==Variant::Red&&o.applied&&b.atp[0]==2&&b.atp[1]==2&&b.writes==0&&led.used==1,"gen2 evaluated alone: native RED keeps 2");
  Actor c=native("E_OC2",7);auto q=first_execute(c,reg,led,R::Normal,0);require(q.applied&&c.atp[0]==1&&led.used==1&&led.find(7)->target==1&&led.find(7)->identity==&c,"gen3: BLUE 1");pass("two_generations_of_the_same_actor_id_never_inherit_the_modification");}
 {vp::Registry reg;Ledger led;Actor blue=native("E_OC",1),blue2=native("E_OC2",2),red=native("E_OC2",3),black=native("E_OC2",4);
  first_execute(blue,reg,led,R::None,0);first_execute(blue2,reg,led,R::Normal,0);first_execute(red,reg,led,R::None,0);first_execute(black,reg,led,R::Black,440);
  require(blue.atp[0]==1&&blue2.atp[0]==1&&red.atp[0]==2&&black.atp[0]==3&&led.used==4,"four variants at once");
  black.atp[1]=4;                                                   // someone else changes ONE sphere of Black: nothing else is affected
  require(blue.atp[0]==1&&blue2.atp[1]==1&&red.atp[1]==2&&black.atp[0]==3,"independent");pass("simultaneous_actors_independent_third_party_change_affects_only_that_actor");}
 {// HP / regeneration / death states never touch attack power (CP10.4 step() is unchanged and only returns health)
  Actor a=native("E_OC2");vp::Registry reg;Ledger led;first_execute(a,reg,led,R::Black,440);vp::Entry* e=reg.find(a.id);require(e!=nullptr,"entry");std::int16_t h=100;
  for(int act:{int(vp::A_WAIT),int(vp::A_ATTACK),int(vp::A_DAMAGE),int(vp::A_BIG_DAMAGE),int(vp::A_DEATH),int(vp::A_WATER_DEATH),int(vp::A_FALL_DEAD),int(vp::A_FALL),int(vp::A_DEMO_MASTER),int(vp::A_DEMO_CHILD),int(vp::A_MOVE_OUT)})
   for(unsigned t=0;t<40;++t){h=vp::step(*e,h,act).new_health;require(a.atp[0]==3&&a.atp[1]==3&&a.writes==1,"state change leaves atp alone");}
  h=0;vp::step(*e,h,vp::A_DEATH);require(a.atp[0]==3,"dead actor keeps its value, no resurrection of HP either");pass("death_fall_event_and_regeneration_states_do_not_write_attack_power");}
 // ---------- D. release / rollback ----------
 {vp::Registry reg;Ledger led;Actor a=native("E_OC2",10);first_execute(a,reg,led,R::Black,440);Entry rec=*led.find(10);
  auto rp=restore_plan(rec,true,3,3);require(rp.write&&!std::strcmp(rp.why,"RESTORE_NATIVE_ATTACK_POWER"),"owned: restore native");
  rp=restore_plan(rec,true,3,2);require(!rp.write,"one sphere not ours");rp=restore_plan(rec,true,4,4);require(!rp.write,"third party value");rp=restore_plan(rec,true,0,0);require(!rp.write,"reset to 0 by others");
  rp=restore_plan(rec,false,3,3);require(!rp.write&&!std::strcmp(rp.why,"ACTOR_GONE_OR_REPLACED_NOTHING_READ"),"gone / replaced: nothing read");
  Entry unwritten=rec;unwritten.written=false;rp=restore_plan(unwritten,true,2,2);require(!rp.write,"native Red had no write");rp=restore_plan(Entry{},true,3,3);require(!rp.write,"empty slot");
  Actor blue=native("E_OC2",11);first_execute(blue,reg,led,R::Normal,0);const Entry b=*led.find(11);require(b.native==2&&b.target==1&&restore_plan(b,true,1,1).write,"restore Blue DEVF to the exact native 2");
  pass("rollback_restores_exact_native_values_only_if_owned_and_same_live_actor");}
 {Ledger led;Plan p=make_plan(Variant::Black,"E_OC2",2,2);int x=0;auto* r=led.record(5,&x,1,Variant::Black,p);require(r&&led.used==1&&!led.record(5,&x,2,Variant::Black,p)&&led.used==1,"one record per id");led.release(*r);require(led.used==0&&!led.find(5),"released");
  require(!led.record(0,&x,1,Variant::Black,p)&&!led.record(6,nullptr,1,Variant::Black,p),"invalid identity refused");pass("ledger_identity_and_release");}
 // ---------- E. MODEL_OF_SOURCE: what the host does with these values (documentation, NOT a measurement of the game) ----------
 {require(model_link_units({1},1.0f)==1&&model_link_units({2},1.0f)==2&&model_link_units({3},1.0f)==3,"atp -> life units 1:2:3 at magnification 1.0");pass("MODEL_OF_SOURCE_base_hit_is_linear_1_2_3_at_magnification_1");}
 {require(model_link_units({3,3},1.0f)==3&&model_link_units({3,3,3},1.0f)==3&&model_link_units({1,3},1.0f)==3,"max rule: two spheres or repeated hits in one frame never add up");pass("MODEL_OF_SOURCE_both_spheres_in_one_frame_do_not_stack");}
 {require(model_link_units({1},2.0f)==2&&model_link_units({2},2.0f)==4&&model_link_units({3},2.0f)==6,"wolf x2 stays proportional");
  require(model_link_units({1},1.5f)==2&&model_link_units({2},1.5f)==3&&model_link_units({3},1.5f)==5,"x1.5 rounds up per hit: NOT exactly 1:2:3");pass("MODEL_OF_SOURCE_non_integer_magnification_rounds_up_so_the_ratio_is_not_exact");}
 std::printf("DAMAGE_LAB_PASS cases=%u POLICY_ONLY; LINK_HEART_LOSS_NOT_MEASURED; NATIVE_HOOKS_AND_GAME_NOT_EXECUTED\n",cases);
}
