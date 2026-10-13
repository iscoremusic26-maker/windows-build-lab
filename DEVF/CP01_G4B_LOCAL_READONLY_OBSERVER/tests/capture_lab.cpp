#include "../src/capture.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <limits>
using namespace devf::g4b;
unsigned checks=0;
void check(bool v,const char* name){if(!v){std::cerr<<"FAIL "<<name<<'\n';std::exit(1);}++checks;std::cout<<"PASS "<<name<<'\n';}
Capture fixture(){
 Capture c;c.id=1;auto& o=c.owner;o.host_ok=o.thread_ok=o.registry_ok=o.player_ok=true;
 o.node_present=o.node_profile=o.node_complete=o.node_id_ok=true;o.child_registered=o.binding_ok=o.play_ok=o.previous_registered=true;
 std::memcpy(o.stage.data(),"F_SP121",8);o.room=o.next_room=3;o.logical=6;
 o.root=10;o.previous=99;o.link_owner=10;o.node=100;o.child=55;o.parent=o.play_layer=20;o.play=200;
 o.creates=o.deletes=o.parent_creates=o.parent_deletes=0;
 c.context_stable=c.event_read=c.actors_complete=c.bg_scan_complete=true;c.ground.status=Status::Observed;
 c.ground.y=C[1];c.ground.bg=0;c.ground.poly=42;c.ground.room=3;c.water.status=Status::NoHitAtProbe;return c;
}
Actor actor(){Actor a;a.id=7;a.owner=10;a.profile=0x2fd;a.room=3;a.initialized=a.owner_valid=true;a.xyz=C;return a;}
int main(){
 auto c=fixture();evaluate(c);check(c.capture_valid && c.session==Session::CompatibleObserved,"coherent_scalars_only_not_admission");
 auto reject=[](auto change,const char* name){auto t=fixture();change(t);evaluate(t);check(!t.capture_valid,name);};
 reject([](auto& t){t.owner.stage[0]='X';},"wrong_stage");reject([](auto& t){t.owner.room=4;},"wrong_room");
 reject([](auto& t){t.owner.logical=5;},"wrong_logical");reject([](auto& t){t.owner.node_present=false;},"missing_room_scene");
 reject([](auto& t){t.owner.play_ok=false;},"invalid_play_scene");reject([](auto& t){t.owner.child_registered=false;},"unregistered_child");
 reject([](auto& t){t.owner.transition=true;},"stage_transition");reject([](auto& t){t.owner.next_room=4;},"room_transition");
 reject([](auto& t){t.owner.retiring=true;},"retiring_room");reject([](auto& t){t.owner.parent_deletes=1;},"parent_pending_delete");
 reject([](auto& t){t.owner.creates=1;},"child_pending_create");reject([](auto& t){t.context_stable=false;},"changed_mid_capture");
 reject([](auto& t){t.actors_complete=false;},"incomplete_actor_walk");reject([](auto& t){t.event_read=false;},"incomplete_session");
 reject([](auto& t){t.event_active=true;},"active_event");reject([](auto& t){t.budget_exceeded=true;},"budget_exceeded");
 reject([](auto& t){t.ground.status=Status::Invalid;},"ground_incompatible");
 reject([](auto& t){t.ground.y=std::numeric_limits<double>::quiet_NaN();},"ground_nan");
 c=fixture();c.ground.status=Status::NoHitAtProbe;evaluate(c);check(c.ground.status==Status::NoHitAtProbe,"no_support_preserved_never_location_pass");
 c=fixture();c.ground.special=7;c.ground.ground=31;evaluate(c);check(c.ground.special==7 && c.ground.ground==31,"material_preserved_no_admission_policy");
 c=fixture();c.water=c.ground;c.water.y=C[1]+100;evaluate(c);check(c.water.status==Status::Observed && c.water.y>C[1],"water_group_surface_present");
 c=fixture();evaluate(c);check(c.water.status==Status::NoHitAtProbe && c.water_volume==Status::UnavailableSafeApi,"probe_no_hit_is_not_volume_absence");
 reject([](auto& t){t.water.status=Status::UnavailableSafeApi;},"unavailable_water_not_absence");
 reject([](auto& t){t.bg_scan_complete=false;},"partial_bg_scan");
 double d;auto lo=C,hi=C;for(auto& v:lo)v-=10;for(auto& v:hi)v+=10;
 check(sphere_box_candidate(lo,hi,d)&&d==0,"bg_aabb_candidate_not_exact_intersection");
 lo[0]+=1000;hi[0]+=1000;check(!sphere_box_candidate(lo,hi,d)&&d>radius,"distant_aabb_filtered_only");
 lo=hi;lo[0]+=1;check(!sphere_box_candidate(lo,hi,d)&&d<0,"reversed_aabb_invalid");
 c=fixture();c.bg_count=1;auto& b=c.bg[0];b.index=1;b.ready=b.moving=true;b.lo=b.hi=C;evaluate(c);
 check(c.exact_bg==Status::UnavailableSafeApi && c.bg[0].distance==0,"bg_overlap_not_claimed");
 reject([](auto& t){t.bg_truncated=true;},"bg_output_truncation");
 c=fixture();c.actor_count=1;c.actors[0]=actor();evaluate(c);check(c.actors[0].occupancy==Occupancy::Proximate,"link_or_actor_center_proximate");
 c=fixture();c.actor_count=1;c.actors[0]=actor();c.actors[0].xyz[0]+=1000;evaluate(c);check(c.actors[0].occupancy==Occupancy::OverlapNotVerified,"far_center_unknown_volume");
 c.actors[0].bound_proven=true;c.actors[0].bound=50;evaluate(c);check(c.actors[0].occupancy==Occupancy::SeparatedUnderBound,"fixture_only_proven_bound_separation");
 c.actors[0].xyz=C;evaluate(c);check(c.actors[0].occupancy==Occupancy::OverlapUnderBound,"fixture_only_proven_bound_overlap");
 reject([](auto& t){t.actor_count=1;t.actors[0]=actor();t.actors[0].initialized=false;},"partial_initialization");
 reject([](auto& t){t.actor_count=2;t.actors[0]=t.actors[1]=actor();},"duplicate_local_id");
 reject([](auto& t){t.actor_count=1;t.actors[0]=actor();t.actors[0].owner_valid=false;},"unknown_actor_owner");
 reject([](auto& t){t.actors_truncated=true;},"actor_output_truncation");
 reject([](auto& t){t.actor_count=1;t.actors[0]=actor();t.actors[0].xyz[0]=std::numeric_limits<double>::quiet_NaN();},"actor_nan");
 reject([](auto& t){t.owner.next_room=4;},"room_change_during_sample");
 reject([](auto& t){t.actor_count=99;},"oversized_count_clamped_invalid");
 reject([](auto& t){t.visited=257;},"bounded_walk_exceeded");
 c=fixture();c.resources_read=c.e_oc_registered=c.e_ocb_registered=true;evaluate(c);char text[8192];
 check(format(c,"2.0.3","NOT_VERIFIED",text,sizeof(text)) && std::strstr(text,"READY=NOT_VERIFIED") && std::strstr(text,"no_location_admission=1"),"registered_not_ready_or_admission");
 char tiny[16];check(!format(c,"2.0.3","x",tiny,sizeof(tiny)) && tiny[15]==0,"log_truncation_fail_closed");
 c=fixture();c.actor_count=max_actors;c.bg_count=max_bg;
 for(unsigned i=0;i<max_actors;++i){c.actors[i]=actor();c.actors[i].id=i;}
 for(unsigned i=0;i<max_bg;++i){c.bg[i].lo=c.bg[i].hi=C;c.bg[i].index=i;c.bg[i].ready=c.bg[i].moving=true;}
 evaluate(c);check(format(c,"2.0.3.0","0123456789012345678901234567890123456789012345678901234567890123",text,sizeof(text)),"maximum_fixture_output_fits_finite_line");
 Budget budget;budget.active=true;unsigned calls=0;for(unsigned i=0;i<4000;++i)if(budget.due()){++budget.lines;++calls;}
 check(calls==16,"sixteen_records_budget");budget.busy=true;check(!budget.due(),"reentrant_budget_blocks");
 std::cout<<"SUMMARY "<<checks<<" PASS 0 FAIL; synthetic scalar capture only; no engine or DX12\n";
}
