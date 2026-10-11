#include "controller.hpp"
#include "lifecycle.hpp"
#include <cstdio>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace devf::cp01;
static void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message); // active in RelWithDebInfo; not assert/NDEBUG
}
struct Ops {
    static constexpr int complete_step = 4, error_step = 5; // synthetic lab constants, NOT game ABI
    int phase = 1;
    bool heap_ok = true, collision_ok = true, scene_ok = true;
    int loads = 0, heaps = 0, registrations = 0, scenes = 0;
    std::vector<std::string> events;
    int load_archive() { ++loads; return phase; }
    bool create_heap() { ++heaps; return heap_ok; }
    bool register_collision() { ++registrations; return collision_ok; }
    bool initialize_scene() { ++scenes; return scene_ok; }
    void release_collision() { events.push_back("release_collision"); }
    void release_archive() { events.push_back("release_archive"); }
    void clear_heap_references() { events.push_back("clear_heap_references"); }
};
struct LabActor {
    ResourceLifecycle lifecycle;
    Ops ops;
    int ticks = 0, draws = 0, deletes = 0;
};
static int create_callback(void* p) { auto& a = *static_cast<LabActor*>(p); return a.lifecycle.advance(a.ops); }
static int delete_callback(void* p) { auto& a = *static_cast<LabActor*>(p); a.lifecycle.release(a.ops); ++a.deletes; return 1; }
static int execute_callback(void* p) { auto& a = *static_cast<LabActor*>(p); if(a.lifecycle.ready()) ++a.ticks; return 1; }
static int draw_callback(void* p) { auto& a = *static_cast<LabActor*>(p); if(a.lifecycle.ready()) ++a.draws; return 1; }
static int may_delete(void*) { return 1; }
static ActorProfileDesc profile() {
    return {.name = "dvfrock", .process_size = sizeof(LabActor),
        .create_function = create_callback, .delete_function = delete_callback,
        .execute_function = execute_callback, .is_delete_function = may_delete,
        .draw_function = draw_callback};
}
struct Host {
    ActorService actors{};
    StageService stages{};
    std::vector<std::string> events;
    ModResult register_result = MOD_OK, add_result = MOD_OK, remove_result = MOD_OK;
    ModResult unregister_result = MOD_OK, create_result = MOD_OK;
    bool owned_profile = false, owned_stage = false, requested = false;
    std::optional<ActorProfileDesc> desc;
    LabActor object;
    Host() {
        actors.register_actor = [](ModContext* ctx, const ActorProfileDesc* d, ProfileName* n, ActorHandle* h) {
            auto& host = *reinterpret_cast<Host*>(ctx); host.events.push_back("register");
            if(host.register_result != MOD_OK) return host.register_result;
            host.desc.emplace(*d); host.owned_profile = true; *n = 2000; *h = 42; return MOD_OK;
        };
        actors.unregister_actor = [](ModContext* ctx, ActorHandle h) {
            auto& host = *reinterpret_cast<Host*>(ctx); host.events.push_back("unregister");
            check(h == 42, "invalid profile ownership");
            if(!host.owned_profile) return MOD_INVALID_ARGUMENT;
            if(host.unregister_result != MOD_OK) return host.unregister_result;
            if(host.requested) host.desc->delete_function(&host.object);
            host.owned_profile = false; host.requested = false; return MOD_OK;
        };
        actors.create_actor = [](ModContext* ctx, ProfileName n, const ActorSpawnParams*, ActorId* id) {
            auto& host = *reinterpret_cast<Host*>(ctx); host.events.push_back("request_runtime");
            check(n == 2000 && host.owned_profile && !host.owned_stage, "wrong runtime authority");
            if(host.create_result != MOD_OK) return host.create_result;
            host.requested = true; *id = 101; return MOD_OK;
        };
        stages.add_actor = [](ModContext* ctx, const char* stage, uint8_t room, int8_t layer,
                              const void* record, size_t size, StageActorHandle* h) {
            auto& host = *reinterpret_cast<Host*>(ctx); host.events.push_back("add_stage");
            check(std::strcmp(stage, "LABONLY") == 0 && room == 1 && layer == -1, "fixture changed");
            check(size == 32 && std::memcmp(record, "dvfrock", 8) == 0, "wrong stage record");
            if(host.add_result != MOD_OK) return host.add_result;
            host.owned_stage = true; *h = 51; return MOD_OK;
        };
        stages.remove_actor_edit = [](ModContext* ctx, StageActorHandle h) {
            auto& host = *reinterpret_cast<Host*>(ctx); host.events.push_back("remove_stage");
            check(h == 51 && host.owned_stage, "invalid stage ownership");
            if(host.remove_result != MOD_OK) return host.remove_result;
            host.owned_stage = false; return MOD_OK;
        };
    }
    Controller controller() { return Controller(reinterpret_cast<ModContext*>(this), actors, stages); }
};
static Placement fixture() {
    // LABONLY is a synthetic service fixture and is NEVER passed to the real game module.
    Placement p; std::memcpy(p.stage.data(), "LABONLY", 8); p.room = 1;
    std::memcpy(p.record.data(), "dvfrock", 8); p.location_verified = true; return p;
}
static ActorSpawnParams spawn() { ActorSpawnParams p{}; p.room_num = 1; p.scale = {1,1,1}; return p; }
static void resource_success() {
    ResourceLifecycle state; Ops ops;
    check(state.advance(ops)==1 && !state.ready(), "pending load incorrectly ready");
    check(ops.heaps==0 && ops.registrations==0, "pending load allocated heap");
    ops.phase = Ops::complete_step;
    check(state.advance(ops)==4 && state.ready(), "load did not finish");
    check(state.advance(ops)==4 && ops.loads==2 && ops.heaps==1 && ops.registrations==1 && ops.scenes==1, "duplicate initialization");
    state.release(ops); state.release(ops);
    check(ops.events==std::vector<std::string>{"release_collision","release_archive","clear_heap_references"}, "cleanup order/ownership incorrect");
    check(!state.ready() && state.advance(ops)==5, "destroyed actor reused");
}
static void resource_failure(int failure) {
    ResourceLifecycle state; Ops ops; ops.phase = Ops::complete_step;
    if(failure==0) ops.phase=Ops::error_step;
    if(failure==1) ops.heap_ok=false;
    if(failure==2) ops.collision_ok=false;
    if(failure==3) ops.scene_ok=false;
    check(state.advance(ops)==5 && !state.ready(), "early failure reported ready");
    const int loads=ops.loads; check(state.advance(ops)==5 && ops.loads==loads,"terminal error retried");
    state.release(ops); state.release(ops);
    const auto expected = failure==3 ? std::vector<std::string>{"release_collision","release_archive","clear_heap_references"}
        : std::vector<std::string>{"release_archive","clear_heap_references"};
    check(ops.events==expected, "partial failure freed unowned collision or leaked archive");
}
static void never_started_cleanup() {
    ResourceLifecycle state; Ops ops; state.release(ops);
    check(ops.events==std::vector<std::string>{"clear_heap_references"}, "unrequested archive released");
}
static void stage_success() {
    Host h; auto c=h.controller(); auto p=fixture();
    check(c.start(profile(),&p)==MOD_OK,"stage registration failed");
    check(c.start(profile(),&p)==MOD_CONFLICT,"duplicate profile accepted");
    ActorId id=0; check(c.request_runtime(spawn(),&id)==MOD_CONFLICT,"mixed spawn authorities");
    check(c.stop()==MOD_OK && c.stop()==MOD_OK,"idempotent stop failed");
    check(h.events==std::vector<std::string>{"register","add_stage","remove_stage","unregister"},"stage/profile cleanup order incorrect");
}
static void location_validation() {
    for(int failure=0;failure<6;++failure) {
        Host h; auto c=h.controller(); auto p=fixture();
        if(failure==0) p.location_verified=false;
        if(failure==1) p.stage.fill('x');
        if(failure==2) p.room=0xff;
        if(failure==3) p.layer=16;
        if(failure==4) p.record[0]='x';
        if(failure==5) p.stage[0]='\0';
        const auto expected=failure==0?MOD_UNSUPPORTED:MOD_INVALID_ARGUMENT;
        check(c.start(profile(),&p)==expected && h.events.empty(),"unsafe location caused side effects");
    }
}
static void registration_conflict() {
    Host h; auto c=h.controller(); h.register_result=MOD_CONFLICT;
    check(c.start(profile(),nullptr)==MOD_CONFLICT,"name collision swallowed");
    check(c.stop()==MOD_OK && h.events.size()==1 && !c.registered(),"failed registration owns profile");
}
static void stage_failure_rollback() {
    Host h; auto c=h.controller(); auto p=fixture(); h.add_result=MOD_ERROR;
    check(c.start(profile(),&p)==MOD_ERROR && !c.registered(),"stage failure did not rollback profile");
    check(h.events==std::vector<std::string>{"register","add_stage","unregister"},"unexpected rollback sequence");
}
static void rollback_failure_retry() {
    Host h; auto c=h.controller(); auto p=fixture(); h.add_result=MOD_ERROR; h.unregister_result=MOD_UNAVAILABLE;
    check(c.start(profile(),&p)==MOD_ERROR && c.registered() && c.last_cleanup()==MOD_UNAVAILABLE,"rollback failure lost ownership");
    check(c.start(profile(),nullptr)==MOD_CONFLICT,"re-register during rollback allowed");
    h.unregister_result=MOD_OK; check(c.stop()==MOD_OK && !c.registered(),"rollback retry failed");
}
static void shutdown_failure_retry() {
    Host h; auto c=h.controller(); auto p=fixture(); check(c.start(profile(),&p)==MOD_OK,"setup failed");
    h.remove_result=MOD_UNAVAILABLE;
    check(c.stop()==MOD_UNAVAILABLE && c.has_stage_edit() && c.registered(),"stage removal failure lost handles");
    check(h.events.back()=="remove_stage","unregister despite live stage edit");
    h.remove_result=MOD_OK; h.unregister_result=MOD_UNAVAILABLE;
    check(c.stop()==MOD_UNAVAILABLE && !c.has_stage_edit() && c.registered(),"profile removal failure lost handle");
    h.unregister_result=MOD_OK; check(c.stop()==MOD_OK,"stop retry failed");
}
static void host_deactivating_before_shutdown() {
    Host h; auto c=h.controller(); auto p=fixture(); check(c.start(profile(),&p)==MOD_OK,"setup failed");
    // Exact host order: ActorService attempts unregister before the mod's shutdown callback;
    // StageService removes edits only at detach, after shutdown.
    check(h.actors.unregister_actor(reinterpret_cast<ModContext*>(&h),42)==MOD_OK,"pre-hook failed");
    check(c.stop(true)==MOD_OK && !c.registered() && !c.has_stage_edit(),"native pre-clean misreported as failed ownership");
    Host failed; auto d=failed.controller(); check(d.start(profile(),nullptr)==MOD_OK,"setup failed");
    failed.unregister_result=MOD_UNAVAILABLE;
    check(d.stop(true)==MOD_UNAVAILABLE && d.registered(),"native unregister failure swallowed");
    Host unexpected; auto e=unexpected.controller(); check(e.start(profile(),nullptr)==MOD_OK,"setup failed");
    unexpected.owned_profile=false;
    check(e.stop()==MOD_INVALID_ARGUMENT && e.registered(),"invalid handle outside shutdown swallowed");
}
static void runtime_lifecycle() {
    Host h; auto c=h.controller(); check(c.start(profile(),nullptr)==MOD_OK,"setup failed");
    ActorId id=0; h.create_result=MOD_ERROR;
    check(c.request_runtime(spawn(),&id)==MOD_ERROR,"failed request misreported");
    h.create_result=MOD_OK;
    check(c.request_runtime(spawn(),&id)==MOD_OK && id==101,"runtime request failed");
    check(!h.object.lifecycle.ready(),"request treated as completed actor");
    check(c.request_runtime(spawn(),&id)==MOD_CONFLICT,"duplicate runtime request accepted");
    h.desc->execute_function(&h.object); h.desc->draw_function(&h.object);
    check(h.object.ticks==0 && h.object.draws==0,"pending actor drawn/executed");
    check(h.desc->create_function(&h.object)==1,"pending phase lost");
    h.object.ops.phase=Ops::complete_step;
    check(h.desc->create_function(&h.object)==4,"ready phase lost");
    h.desc->execute_function(&h.object); h.desc->draw_function(&h.object);
    check(h.object.ticks==1 && h.object.draws==1,"callbacks not delivered");
    check(c.stop()==MOD_OK && h.object.deletes==1 && !h.object.lifecycle.ready(),"delete callback did not clean up");
    check(c.stop()==MOD_OK && h.object.deletes==1,"double destruction");
}
static void invalid_runtime() {
    Host h; auto c=h.controller(); ActorId id=0;
    check(c.request_runtime(spawn(),&id)==MOD_UNAVAILABLE,"unregistered runtime request allowed");
    check(c.start(profile(),nullptr)==MOD_OK,"setup failed");
    auto p=spawn(); p.scale.x=0;
    check(c.request_runtime(p,&id)==MOD_INVALID_ARGUMENT,"zero scale accepted");
    p=spawn();p.position.y=std::numeric_limits<float>::quiet_NaN();
    check(c.request_runtime(p,&id)==MOD_INVALID_ARGUMENT,"NaN accepted");
    check(c.request_runtime(spawn(),nullptr)==MOD_INVALID_ARGUMENT,"null output accepted");
    check(c.stop()==MOD_OK,"cleanup failed");
}
static void pending_delete() {
    Host h; auto c=h.controller(); check(c.start(profile(),nullptr)==MOD_OK,"setup failed");
    ActorId id=0;check(c.request_runtime(spawn(),&id)==MOD_OK,"request failed");
    check(h.desc->create_function(&h.object)==1,"phase fixture failed");
    check(c.stop()==MOD_OK && h.object.deletes==1,"pending fake actor not cleaned");
    check(h.object.ops.events==std::vector<std::string>{"release_archive","clear_heap_references"},"pending fake actor cleanup incorrect");
    // This fake host can destroy pending objects. The real host's pending queue is NOT validated.
}
static int cases=0;
template<class Function> void test(const char* name, Function function) {
    function(); ++cases; std::printf("PASS %s\n",name);
}
int main() {
    try {
        test("resource_pending_ready_owned_cleanup",resource_success);
        for(int failure=0;failure<4;++failure) test(("resource_partial_failure_"+std::to_string(failure)).c_str(),[=]{resource_failure(failure);});
        test("cleanup_before_resource_request",never_started_cleanup);
        test("stage_registration_authority_cleanup_order",stage_success);
        test("unverified_and_invalid_locations_no_side_effects",location_validation);
        test("profile_collision",registration_conflict);
        test("stage_failure_rolls_back_profile",stage_failure_rollback);
        test("rollback_failure_keeps_handles_for_retry",rollback_failure_retry);
        test("shutdown_failure_keeps_handles_for_retry",shutdown_failure_retry);
        test("host_deactivating_before_shutdown",host_deactivating_before_shutdown);
        test("runtime_request_and_synthetic_callbacks",runtime_lifecycle);
        test("invalid_runtime_params",invalid_runtime);
        test("pending_fake_actor_destruction",pending_delete);
        std::printf("CP01_LAB_PASS cases=%d platform=Windows synthetic_services=true real_game=NOT_TESTED\n",cases);
        return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"FAIL %s\n",error.what()); return 1; }
}
