#include "../src/core.hpp"
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <cstdlib>
#include <cstring>
namespace devf::a1 { extern unsigned capture_calls; extern Snapshot fixture; }
namespace {
unsigned checks = 0;
std::vector<std::string> logs;
void check(bool ok, const char* label) {
    if (!ok) { std::cerr << "FAIL " << label << '\n'; std::exit(1); }
    ++checks; std::cout << "PASS " << label << '\n';
}
void write(ModContext*, LogLevel, const char* text) { logs.emplace_back(text); }
}
int main() {
    using namespace devf::a1;
    HostService host{}; host.header = SERVICE_HEADER(HostService, 2, 2);
    LogService log{}; log.header = SERVICE_HEADER(LogService, 1, 0); log.write = write;
    GameService game{}; game.header = SERVICE_HEADER(GameService, 2, 0);
    ModError error = MOD_ERROR_INIT;
    svc_host = &host; svc_log = &log; svc_game = &game;
    check(mod_update(&error) == MOD_OK && capture_calls == 0, "update_before_initialize_skips_native");
    svc_host = nullptr;
    check(mod_initialize(&error) == MOD_UNSUPPORTED, "missing_host_rejected"); svc_host = &host;
    for (const char* version : {"2.0.2", "2.0.3-custom", "2.0.30", "", "4.0.3"}) {
        host.version = version;
        check(mod_initialize(&error) == MOD_UNSUPPORTED && capture_calls == 0, "inexact_version_rejected");
    }
    host.version = "2.0.3"; game.header.major_version = 1;
    check(mod_initialize(&error) == MOD_UNSUPPORTED, "wrong_game_epoch_rejected"); game.header.major_version = 2;
    log.write = nullptr;
    check(mod_initialize(&error) == MOD_UNSUPPORTED, "missing_log_function_rejected"); log.write = write;
    host.header.struct_size = sizeof(ServiceHeader);
    check(mod_initialize(&error) == MOD_UNSUPPORTED, "short_host_header_rejected"); host.header.struct_size = sizeof(host);
    host.header.minor_version = 1;
    check(mod_initialize(&error) == MOD_UNSUPPORTED, "old_host_minor_rejected"); host.header.minor_version = 2;
    game.header.struct_size = sizeof(game) - 1;
    check(mod_initialize(&error) == MOD_UNSUPPORTED, "short_game_header_rejected"); game.header.struct_size = sizeof(game);
    check(logs.empty() && capture_calls == 0, "rejections_have_no_logs_or_native_reads");
    host.build_id = reinterpret_cast<const uint8_t*>(1); host.build_id_len = 33;
    check(mod_initialize(&error) == MOD_OK && logs.back().find("build=NOT_VERIFIED") != std::string::npos,
          "oversized_build_id_never_dereferenced");
    check(mod_initialize(&error) == MOD_CONFLICT && logs.size() == 1, "duplicate_init_does_not_reset_active_state");
    fixture.status = Observation::Transition;
    mod_update(&error);
    check(capture_calls == 1 && logs.back().find("SKIP_TRANSITION") != std::string::npos, "transition_values_skipped");
    for (unsigned i = 0; i < 119; ++i) mod_update(&error);
    check(capture_calls == 1, "cadence_119_updates_without_capture");
    fixture.status = Observation::Captured;
    std::memcpy(fixture.stage.data(), "F_SP121", 8); fixture.room = 3; fixture.logical_layer = 6;
    fixture.current_layer = 0; fixture.link_owner = 7; fixture.room_node = 9;
    fixture.room_owner = 5; fixture.room_layer = 8; fixture.room_creates = fixture.room_deletes = 0;
    mod_update(&error);
    check(capture_calls == 2 && logs.back().find("logical=6") != std::string::npos, "scalar_snapshot_formatted");
    auto bad = fixture; bad.logical_layer = 15;
    check(!valid_values(bad), "logical_layer_15_rejected");
    bad = fixture; bad.room = 64; check(!valid_values(bad), "room_64_rejected");
    bad = fixture; bad.stage[0] = '/'; check(!valid_values(bad), "invalid_stage_rejected");
    bad = fixture; bad.stage[8] = 'X'; check(!valid_values(bad), "unterminated_stage_rejected");
    bad = fixture; bad.link_xyz[0] = std::numeric_limits<float>::quiet_NaN();
    check(!valid_values(bad), "NaN_link_rejected");
    bad = fixture; bad.census_count = 9; check(!valid_values(bad), "oversized_census_rejected");
    bad = fixture; bad.census_count = 1; bad.census[0].xyz[2] = std::numeric_limits<float>::infinity();
    bad.census[0].id = 10; bad.census[0].owner = 7; bad.census[0].room = 3;
    check(!valid_values(bad), "infinite_census_rejected");
    char small[7]; format_snapshot(bad, small, sizeof(small));
    check(small[6] == '\0', "small_log_buffer_terminated");
    bad = fixture; bad.census_truncated = true;
    check(!valid_values(bad), "truncated_census_never_accepted");
    bad = fixture; bad.room_owner = kUnknown;
    check(!valid_values(bad), "unknown_room_parent_owner_rejected");
    bad = fixture; bad.link_owner = kUnknown;
    check(!valid_values(bad), "unknown_link_owner_rejected");
    bad = fixture; bad.current_layer = kUnknown;
    check(!valid_values(bad), "unknown_current_layer_rejected");
    bad = fixture; bad.room_creates = 1;
    check(!valid_values(bad), "room_pending_create_rejected");
    bad = fixture; bad.room_deletes = 1;
    check(!valid_values(bad), "room_pending_delete_rejected");
    bad = fixture; bad.census_count = 1; bad.census[0].id = 10; bad.census[0].room = 3;
    check(!valid_values(bad), "unknown_census_owner_rejected");
    bad.census[0].owner = 7; bad.census[0].room = 4;
    check(!valid_values(bad), "wrong_census_room_rejected");
    bad.census[0].room = 3; bad.census_count = 2; bad.census[1] = bad.census[0];
    check(!valid_values(bad), "duplicate_census_id_rejected");
    check(valid_values(fixture), "distinct_registered_owners_not_required_equal");
    bad = fixture; bad.status = Observation::IncompleteCensus; char text[2048];
    format_snapshot(bad, text, sizeof(text));
    check(std::strstr(text, "NOT_VERIFIED_CENSUS_INCOMPLETE") != nullptr,
          "incomplete_census_has_explicit_not_verified_log");
    for (unsigned i = 0; i < 100000; ++i) mod_update(&error);
    check(logs.size() == 63 && capture_calls == 62, "bounded_62_samples_and_no_reads_after_cap");
    check(mod_shutdown(&error) == MOD_OK && logs.size() == 64, "shutdown_reserved_log_and_scalar_reset");
    const auto calls = capture_calls;
    mod_update(&error); mod_shutdown(&error);
    check(capture_calls == calls && logs.size() == 64, "shutdown_idempotent_no_pending_capture_or_log");
    host.version = "2.0.3.0"; host.build_id = nullptr; host.build_id_len = 0;
    check(mod_initialize(&error) == MOD_OK, "official_four_component_version_accepted");
    mod_shutdown(&error);
    struct Node { const Node* next; std::uint32_t id; };
    Node b{nullptr, 7}, a{&b, 0}; Registry<Node, 3> registry;
    auto next = [](const Node* p) { return p->next; };
    unsigned id_reads = 0;
    auto id = [&](const Node* p) { ++id_reads; return p->id; };
    registry.collect(&a, next); std::uint32_t result;
    check(registry.id_of(&b, result, id) && result == 7, "registered_layer_identity_read");
    const auto before = id_reads;
    check(!registry.id_of(reinterpret_cast<const Node*>(1), result, id) && result == kUnknown && id_reads == before,
          "unregistered_poison_layer_never_dereferenced");
    b.next = &a; registry.collect(&a, next);
    check(!registry.complete && !registry.id_of(&a, result, id), "cyclic_registry_rejected_even_matching_head");
    b.next = nullptr; Registry<Node, 1> short_registry; short_registry.collect(&a, next);
    check(!short_registry.complete, "truncated_registry_rejected");
    registry.collect(nullptr, next); check(!registry.complete, "empty_registry_rejected");
    check(!kRegistrationAllowed && !kGameMutationAllowed, "permanent_disarm_constants");
    std::cout << "SUMMARY " << checks << " PASS; synthetic observer lifecycle only; no native backend or game execution\n";
}
