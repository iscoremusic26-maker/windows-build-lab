#include "fake.hpp"
#include "../../CP01_A2_DISARMED_CONTROLLER/src/controller_core.hpp"
#include <iostream>
using namespace lab;
devf::a2::Owner project(const Owner& o){devf::a2::Owner a;
#define COPY(x) a.x=o.x
 COPY(host_ok);COPY(thread_ok);COPY(registry_ok);COPY(player_ok);COPY(node_present);COPY(node_profile);COPY(node_complete);COPY(node_id_ok);COPY(child_registered);COPY(binding_ok);COPY(play_ok);COPY(previous_registered);COPY(transition);COPY(retiring);COPY(stage);COPY(room);COPY(logical);COPY(next_room);COPY(node);COPY(child);COPY(parent);COPY(play);COPY(play_layer);COPY(previous);COPY(root);COPY(link_owner);COPY(creates);COPY(deletes);COPY(parent_creates);COPY(parent_deletes);
#undef COPY
 return a;}
int main(){unsigned checks=0,failures=0;auto compare=[&](Owner o){++checks;const bool ok=int(validate_owner(o))==int(devf::a2::validate_owner(project(o)));if(!ok)++failures;};auto o=valid().owner;compare(o);
 bool Owner::* flags[]={&Owner::host_ok,&Owner::thread_ok,&Owner::registry_ok,&Owner::player_ok,&Owner::node_present,&Owner::node_profile,&Owner::node_complete,&Owner::node_id_ok,&Owner::child_registered,&Owner::binding_ok,&Owner::play_ok,&Owner::previous_registered};
 for(auto flag:flags){auto x=o;x.*flag=false;compare(x);}for(auto flag:{&Owner::transition,&Owner::retiring}){auto x=o;x.*flag=true;compare(x);}
 for(auto flag:{&Owner::room,&Owner::logical,&Owner::next_room,&Owner::creates,&Owner::deletes,&Owner::parent_creates,&Owner::parent_deletes}){auto x=o;++(x.*flag);compare(x);}
 for(auto flag:{&Owner::root,&Owner::link_owner,&Owner::node,&Owner::child,&Owner::parent,&Owner::play,&Owner::previous}){auto x=o;x.*flag=unknown;compare(x);}
 auto x=o;x.child=x.root;compare(x);x=o;x.child=x.parent;compare(x);x=o;x.play_layer++;compare(x);x=o;x.stage[0]='X';compare(x);x=o;x.stage[8]='X';compare(x);
 x=o;x.node=202;x.child=106;x.parent=x.play_layer=105;x.play=160;x.previous=50;x.link_owner=105;compare(x);
 std::cout<<"SUMMARY owner-fixture-equivalence="<<checks<<" failures="<<failures<<" old-controller-remains-DISARMED\n";return failures?1:0;}
