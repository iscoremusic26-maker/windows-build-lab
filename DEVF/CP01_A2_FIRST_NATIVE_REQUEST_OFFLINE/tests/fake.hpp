#pragma once
#include "../src/experiment_core.hpp"
#include <cstring>
#include <functional>
#include <stdexcept>
#include <vector>
namespace lab {
using namespace devf::first;
inline Frame valid(){Frame f;auto& o=f.owner;
 o.host_ok=o.thread_ok=o.registry_ok=o.player_ok=o.node_present=o.node_profile=o.node_complete=o.node_id_ok=o.child_registered=o.binding_ok=o.play_ok=o.previous_registered=true;
 std::memcpy(o.stage.data(),"F_SP121",8);o.room=o.next_room=3;o.logical=6;
 o.root=o.previous=0;o.node=102;o.child=6;o.parent=o.play_layer=o.link_owner=5;o.play=60;
 o.creates=o.deletes=o.parent_creates=o.parent_deletes=0;
 f.stable=f.event_read=f.medium_observed=f.registered_eoc=f.registered_weapon=true;return f;}
inline Admission permit(Frame f){Admission a;a.location_admitted=a.session_authorized=true;a.context=key(f.owner);return a;}
struct Fake:Backend {
 Frame frame=valid();Observation observation{};Id layer=0;RequestResult result{0,200};
 bool service=true,previous_exists=true,child_exists=true,throw_select=false,throw_request=false,throw_restore=false,throw_observe=false,misselect=false,misrestore=false;
 int reads=0,calls=0,sets=0,observes=0;std::vector<Id> selections;Preset last{};
 std::function<void(Fake&)> before_read,on_request,on_select;
 Fake(){pending();}
 void pending(){observation={};observation.id=200;observation.frame=frame;observation.frame.owner.creates=1;observation.creating=observation.request_present=observation.actor_scan_complete=observation.request_scan_complete=true;observation.request_owner=frame.owner.child;}
 void ready(){observation={};observation.id=200;observation.frame=frame;observation.actor_scan_complete=observation.request_scan_complete=observation.actor_present=observation.actor_init=observation.derived_read=observation.room_api_ok=true;observation.profile=0x1fe;observation.init=2;observation.owner=frame.owner.child;observation.room=3;}
 bool service_available()const override{return service;}
 Frame preflight()override{++reads;if(before_read)before_read(*this);auto f=frame;f.owner.previous=layer;return f;}
 Id current()override{return layer;}
 bool registered(Id id)override{return id==frame.owner.previous?previous_exists:id==frame.owner.child&&child_exists;}
 void select(Id id)override{++sets;selections.push_back(id);if(on_select)on_select(*this);if((id==frame.owner.child&&throw_select)||(id==frame.owner.previous&&throw_restore))throw std::runtime_error("select");if((id==frame.owner.child&&misselect)||(id==frame.owner.previous&&misrestore))return;layer=id;}
 RequestResult request(const Preset& p)override{++calls;last=p;if(on_request)on_request(*this);if(throw_request)throw std::runtime_error("request");return result;}
 Observation observe(Id)override{++observes;if(throw_observe)throw std::runtime_error("observe");return observation;}
};
}
