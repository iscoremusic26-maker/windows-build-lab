#include "fake_engine.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <utility>
using namespace devf::a2;
using namespace devf::a2::lab;
static unsigned passed=0,failed=0;
void require(bool b) { if(!b) throw std::runtime_error("expectation failed"); }
template<class F> void test(const char* name,F fn) {
    try { fn(); ++passed; std::cout<<"PASS "<<name<<'\n'; }
    catch(const std::exception& e) { ++failed; std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n'; }
}
int main() {
    using Edit=std::function<void(Owner&)>;
    const std::vector<std::pair<const char*,Edit>> guards={
        {"host",[](Owner&o){o.host_ok=false;}},{"thread",[](Owner&o){o.thread_ok=false;}},
        {"missing_node",[](Owner&o){o.node_present=false;}},{"wrong_profile",[](Owner&o){o.node_profile=false;}},
        {"root_target",[](Owner&o){o.child=o.root;}},{"link_owner_confusion",[](Owner&o){o.child=o.link_owner;}},
        {"missing_child",[](Owner&o){o.child=unknown;}},{"unregistered_child",[](Owner&o){o.child_registered=false;}},
        {"incoherent_node_child",[](Owner&o){o.binding_ok=false;}},{"node_creating",[](Owner&o){o.node_complete=false;}},
        {"node_destroying",[](Owner&o){o.retiring=true;}},{"creates",[](Owner&o){o.creates=1;}},
        {"deletes",[](Owner&o){o.deletes=1;}},{"parent_creates",[](Owner&o){o.parent_creates=1;}},
        {"parent_deletes",[](Owner&o){o.parent_deletes=1;}},{"wrong_stage",[](Owner&o){o.stage[0]='X';}},
        {"wrong_room",[](Owner&o){o.room=2;}},{"wrong_logical",[](Owner&o){o.logical=5;}},
        {"unregistered_previous",[](Owner&o){o.previous_registered=false;}},{"stage_transition",[](Owner&o){o.transition=true;}},
        {"room_transition",[](Owner&o){o.next_room=4;}},{"node_id_mismatch",[](Owner&o){o.node_id_ok=false;}},
        {"missing_play",[](Owner&o){o.play_ok=false;}},{"wrong_parent_scene",[](Owner&o){o.play_layer=21;}},
        {"registry_incomplete",[](Owner&o){o.registry_ok=false;}},{"player_absent",[](Owner&o){o.player_ok=false;}}
    };
    for(const auto& [name,edit]:guards) test(name,[&] {
        FakeEngine f; edit(f.owner); Controller c;
        attempt(c,f,true,true); require(f.requests==0 && f.selections==0 && !c.consumed() && c.state()==State::ValidationRejected);
    });
    test("default_zero",[]{FakeEngine f;Controller c;attempt(c,f,false,true);require(f.requests==0 && c.state()==State::Disarmed);});
    test("no_position",[]{FakeEngine f;Controller c;attempt(c,f,true,false);require(f.requests==0 && c.reason()==Reason::Position);});
    test("single_request_owner_and_restore",[]{FakeEngine f;Controller c;attempt(c,f,true,true);require(f.requests==1 && f.captured_owner==16 && f.current()==9 && c.state()==State::Requested && c.request_id()==700);});
    test("previous_changed_before_select",[]{FakeEngine f;f.current_layer=22;Controller c;attempt(c,f,true,true);require(f.requests==0 && c.reason()==Reason::Previous);});
    test("previous_root_restored",[]{FakeEngine f;f.owner.previous=f.current_layer=0;Controller c;attempt(c,f,true,true);require(f.requests==1 && f.current()==0);});
    test("repeated_signals",[]{FakeEngine f;Controller c;attempt(c,f,true,true);attempt(c,f,true,true);attempt(c,f,true,true);require(f.requests==1);});
    test("service_error_no_retry",[]{FakeEngine f;f.result_ok=false;Controller c;attempt(c,f,true,true);attempt(c,f,true,true);require(f.requests==1 && f.current()==9 && c.state()==State::StopSession);});
    test("invalid_id_no_retry",[]{FakeEngine f;f.result_id=unknown;Controller c;attempt(c,f,true,true);attempt(c,f,true,true);require(f.requests==1 && c.state()==State::StopSession);});
    test("exception_before_enqueue",[]{FakeEngine f;f.throw_request=true;Controller c;attempt(c,f,true,true);attempt(c,f,true,true);require(f.requests==0 && f.current()==9 && c.consumed() && c.state()==State::StopSession);});
    test("exception_after_enqueue",[]{FakeEngine f;f.throw_after_enqueue=true;Controller c;attempt(c,f,true,true);attempt(c,f,true,true);require(f.requests==1 && f.current()==9 && c.state()==State::StopSession);});
    test("resolver_exception_no_continuation",[]{FakeEngine f;f.throw_resolve=true;Controller c;attempt(c,f,true,true);f.throw_resolve=false;attempt(c,f,true,true);require(f.requests==0 && c.state()==State::StopSession);});
    test("selection_throw_restore",[]{FakeEngine f;f.throw_select=true;Controller c;attempt(c,f,true,true);require(f.requests==0 && f.current()==9 && c.consumed());});
    test("selection_mismatch",[]{FakeEngine f;f.select_mismatch=true;Controller c;attempt(c,f,true,true);require(f.requests==0 && f.current()==9 && c.reason()==Reason::Selection);});
    test("target_changed_before_scope",[]{FakeEngine f;f.on_resolve=[](FakeEngine& e){if(e.resolves==2)e.owner.child=20;};Controller c;attempt(c,f,true,true);require(f.requests==0 && c.reason()==Reason::Revalidation);});
    test("target_unregistered_after_selection",[]{FakeEngine f;f.on_resolve=[](FakeEngine&e){if(e.resolves==3)e.owner.child_registered=false;};Controller c;attempt(c,f,true,true);require(f.requests==0 && f.current()==9);});
    test("early_exit_scope_restore",[]{FakeEngine f;{LayerScope s(f);require(s.select(16));}require(f.current()==9);});
    test("nested_lifo",[]{FakeEngine f;{LayerScope a(f);a.select(16);{LayerScope b(f);b.select(19);}require(f.current()==16);}require(f.current()==9);});
    test("reentry_blocked",[]{FakeEngine f;Controller c;f.on_request=[&](FakeEngine&e){attempt(c,e,true,true);};attempt(c,f,true,true);require(f.requests==1 && f.current()==9 && c.state()==State::StopSession && !c.busy());});
    test("dead_previous_fail_closed",[]{FakeEngine f;Controller c;f.on_request=[](FakeEngine&e){e.dead_previous=true;};attempt(c,f,true,true);require(f.requests==1 && c.reason()==Reason::Restore && c.state()==State::StopSession);});
    test("restore_failure_no_retry",[]{FakeEngine f;f.fail_restore=true;Controller c;attempt(c,f,true,true);attempt(c,f,true,true);require(f.requests==1 && c.reason()==Reason::Restore);});
    test("latch_consumed_before_request",[]{FakeEngine f;Controller c;f.on_request=[&](FakeEngine&){require(c.consumed() && c.state()==State::AttemptConsumed);};attempt(c,f,true,true);});
    test("pending_then_ready",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto a=ready();a.pending=true;a.visible=false;c.observe(f.owner,a);require(c.state()==State::Pending);c.observe(f.owner,ready());require(c.state()==State::ReadyObserved);});
    test("visible_incomplete",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto a=ready();a.initialized=false;c.observe(f.owner,a);require(c.state()==State::Pending);});
    test("actor_init_missing",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto a=ready();a.actor_init=false;c.observe(f.owner,a);require(c.state()==State::Pending);});
    test("own_pending_counts",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto o=f.owner;o.creates=1;auto a=ready();a.pending=true;c.observe(o,a);require(c.state()==State::Pending);});
    using Change=std::function<void(Observation&)>;
    const std::vector<std::pair<const char*,Change>> obs={
        {"foreign_id",[](Observation&a){a.id=701;}},{"wrong_actor_profile",[](Observation&a){a.profile=0x213;}},
        {"wrong_actor_owner",[](Observation&a){a.owner=0;}},{"wrong_actor_room",[](Observation&a){a.room=4;}},
        {"phase_failed",[](Observation&a){a.failed=true;}},{"request_vanished",[](Observation&a){a.visible=false;}},
        {"actor_removed",[](Observation&a){a.removed=true;}},{"missing_resources",[](Observation&a){a.resources=false;}},
        {"ambiguous_creation",[](Observation&a){a.ambiguous=true;}},{"reuse_reported",[](Observation&a){a.reused=true;}}
    };
    for(const auto& [name,edit]:obs) test(name,[&]{FakeEngine f;Controller c;attempt(c,f,true,true);auto a=ready();edit(a);c.observe(f.owner,a);require(c.state()==State::StopSession || c.state()==State::Failed);attempt(c,f,true,true);require(f.requests==1);});
    test("ready_then_destroyed",[]{FakeEngine f;Controller c;attempt(c,f,true,true);c.observe(f.owner,ready());auto a=ready();a.removed=true;c.observe(f.owner,a);require(c.state()==State::StopSession);});
    test("ready_regresses",[]{FakeEngine f;Controller c;attempt(c,f,true,true);c.observe(f.owner,ready());auto a=ready();a.initialized=false;c.observe(f.owner,a);require(c.state()==State::StopSession);});
    test("timeout_no_retry",[]{FakeEngine f;Controller c(2);attempt(c,f,true,true);auto a=ready();a.pending=true;c.observe(f.owner,a);c.observe(f.owner,a);attempt(c,f,true,true);require(f.requests==1 && c.reason()==Reason::Timeout);});
    test("context_lost",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto o=f.owner;o.node=999;c.observe(o,ready());require(c.state()==State::StopSession);});
    test("room_change_no_rearm",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto o=f.owner;o.room=4;c.observe(o,ready());attempt(c,f,true,true);require(f.requests==1 && c.state()==State::StopSession);});
    test("deactivate_reactivate_no_rearm",[]{FakeEngine f;Controller c;attempt(c,f,true,true);c.disarm();attempt(c,f,true,true);require(f.requests==1 && c.state()==State::StopSession);});
    test("error_terminal_monitor",[]{FakeEngine f;Controller c;attempt(c,f,true,true);auto a=ready();a.failed=true;c.observe(f.owner,a);c.observe(f.owner,ready());require(c.state()==State::Failed);});
    test("preset_no_changes",[]{require(Preset::profile==0x1fe && Preset::parameters==0xffffffffu && Preset::angle_x==-236 && Preset::argument==-1 && Preset::set_id==0xffff && !native_requests_allowed);});
    // Opaque registry behavior reuses the historical G2 template in a separate audit lab.
    std::cout<<"SUMMARY "<<passed<<" PASS "<<failed<<" FAIL; synthetic controller only; no engine or DX12\n";
    return failed?1:0;
}
