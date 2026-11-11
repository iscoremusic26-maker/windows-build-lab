"""Enemy Spawner 0.1.0: executed mutants of the PRODUCTION core and adapter against tests/core_lab.cpp and tests/adapter_lab.cpp.
A mutant counts as detected only when a lab reports ES_LAB_FAIL. Compile errors and timeouts are failures, never credited. The positive control runs first."""
from pathlib import Path
import concurrent.futures, json, os, shutil, subprocess, sys, tempfile
root = Path(__file__).resolve().parents[1]
C, A, K = 'src/spawner_core.hpp', 'src/spawner_adapter.inc', 'src/catalog.hpp'
cases = [
 ('black_creatable', C, '    if (e.route == Route::BlackUnavailable) return Press::BlackUnavailable;  // never a Red with a black label\n', ''),
 ('black_as_red', K, 'Route::BlackUnavailable, Adapt::None},', 'Route::ExpansionRed, Adapt::None},'),
 ('expansion_absent_allowed', C, 'route == Route::ExpansionRed) && expansion == Expansion::Absent) return Press::ExpansionAbsent;', 'route == Route::ExpansionRed) && false) return Press::ExpansionAbsent;'),
 ('refutation_ignored', C, 'route == Route::ExpansionRed) && expansion == Expansion::Refuted) return Press::ExpansionRefuted;', 'route == Route::ExpansionRed) && false) return Press::ExpansionRefuted;'),
 ('double_click_twice', C, '    if (pending_) return Press::Busy;\n', ''),
 ('no_limit', C, '    if (live() >= policy::kMaxLive) return Press::Full;\n', ''),
 ('index_unchecked', C, 'if (index < 0 || static_cast<std::uint64_t>(index) >= kCatalogSize) return Press::InvalidIndex;', 'if (index < 0) return Press::InvalidIndex;'),
 ('pending_never_expires', C, 'if (w != Wait::Ready && now - pending_since_ > policy::kPendingTicks) {', 'if (false) {'),
 ('paused_is_ready', C, '  if (s.paused) return Wait::Paused;\n', ''),
 ('event_is_ready', C, '  if (s.event) return Wait::Event;\n', ''),
 ('transition_is_ready', C, '  if (s.next_stage) return Wait::Transition;\n', ''),
 ('water_accepted', C, '  if (s.water && s.water_y > s.ground_y) return Ground::Water;\n', ''),
 ('wall_ignored', C, '  if (s.blocked) return Ground::Blocked;\n', ''),
 ('edge_ignored', C, '  if (s.ground_y < s.link_y - policy::kMaxStep) return Ground::TooLow;\n', ''),
 ('visible_while_creating', C, '      if (p.found && !p.creating) {', '      if (p.found) {'),
 ('gone_never_forgotten', C, '      if (!p.found) {  // death', '      if (false) {  // death'),
 ('creation_never_times_out', C, '} else if (now - x.since > policy::kCreateTicks) {', '} else if (false) {'),
 ('reused_id_twice', C, '        ++c.id_reused;\n        emit(Ev::IdReused, x.entry, id, 0, now);\n        x = Tracked{};', '        ++c.id_reused;'),
 ('any_hp_is_red', C, 'return health == 330 ? Variant::Red :', 'return health >= 220 ? Variant::Red :'),
 ('creation_value_ignored', C, '  if (e_oc2 && field_0x560 == 220) return', '  if (e_oc2) return'),
 ('native_refutes', C, 'if ((wanted_blue || wanted_red) && expansion != Expansion::Confirmed) expansion = Expansion::Refuted;', 'if (expansion != Expansion::Confirmed) expansion = Expansion::Refuted;'),
 ('layer_not_restored', A, '      es_native_layer(previous);\n', ''),
 ('root_layer_accepted', A, ' || layer == fpcLy_RootLayer() || fpcLy_IsDeletingMesg(layer)) return MOD_UNAVAILABLE;\n  struct', ' || fpcLy_IsDeletingMesg(layer)) return MOD_UNAVAILABLE;\n  struct'),
 ('created_in_caller_layer', A, '  es_native_layer(layer);\n  return svc_actor', '  return svc_actor'),
 ('not_facing_link', A, 'p.angle.y = static_cast<s16>(facing + 0x8000)', 'p.angle.y = facing'),
 ('flying_not_lifted', A, '(e.flying ? es::policy::kFlyingLift : 0.0f)', '0.0f'),
 ('wrong_room', A, 'p.room_num = static_cast<std::int8_t>(fopAcM_GetRoomNo(link));', 'p.room_num = -1;'),
 ('e_oc2_misread', A, "n[3] == 'C' && n[4] == '2' && n[5] == 0;", "n[3] == 'C';"),
 ('adapt_foreign_profile', A, '  if (fopAcM_GetName(a) != want) return;', ''),
 ('adapt_lost', A, '  es_native_keep_ground(a, how);\n', ''),
 ('ui_creates_directly', A, '  const es::Press r = esr.sp.press(index, esr.tick);', '  const es::Press r = esr.sp.press(index, esr.tick);\n  if (r == es::Press::Accepted) es_spawn_now(esr.tick);'),
 ('appeared_announced_early', A, '    case es::Ev::Created: std::snprintf(body, sizeof(body), "%s: solicitud enviada.", label);', '    case es::Ev::Created: std::snprintf(body, sizeof(body), "%s ha aparecido.", label);'),
 ('lizalfos_stay', K, '{"Lizalfos", Profile::E_DN, 0xff0000ffu', '{"Lizalfos", Profile::E_DN, 0xff000000u'),
 ('e_oc2_subtype_lost', K, '{"Bokoblin Red (Expansion)", Profile::E_OC, 0x00ff0000u, -1, 0x0100,', '{"Bokoblin Red (Expansion)", Profile::E_OC, 0x00ff0000u, -1, 0x0000,'),
]
def build(p, lab):
    exe = p / (lab + ('.exe' if os.name == 'nt' else ''))
    src = p / 'tests' / (lab + '.cpp')
    cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O1', str(src), '/Fe' + str(exe), '/Fo' + str(p / (lab + '.obj'))] if os.name == 'nt' else ['g++', '-std=c++20', '-O1', '-w', '-o', str(exe), str(src)]
    return exe, subprocess.run(cmd, capture_output=True, text=True, timeout=300)
def run(p):
    out = ''
    for lab in ('core_lab', 'adapter_lab'):
        exe, c = build(p, lab)
        if c.returncode:
            return False, 'COMPILE_ERROR ' + c.stdout[-800:] + c.stderr[-800:]
        r = subprocess.run([str(exe)], capture_output=True, text=True, timeout=120)
        out += r.stdout + r.stderr
        if r.returncode:
            return False, out
    return True, out
def copy(p):
    shutil.copytree(root / 'src', p / 'src')
    shutil.copytree(root / 'tests', p / 'tests', ignore=shutil.ignore_patterns('*.py', '__pycache__'))
with tempfile.TemporaryDirectory() as td:
    p = Path(td); copy(p)
    ok, out = run(p)
    assert ok and out.count('RESULT=LAB_PASS') == 2, out[-2000:]
for name, rel, old, new in cases:
    assert (root / rel).read_text(encoding='utf-8').count(old) == 1, (name, old)
def one(case):
    name, rel, old, new = case
    with tempfile.TemporaryDirectory() as td:
        p = Path(td); copy(p)
        f = p / rel; f.write_text(f.read_text(encoding='utf-8').replace(old, new, 1), encoding='utf-8')
        try:
            ok, out = run(p)
        except subprocess.TimeoutExpired:
            return {'mutation': name, 'detected': False, 'status': 'TIMEOUT', 'timeout': True}
        killed = not ok and 'ES_LAB_FAIL line=' in out
        return {'mutation': name, 'detected': killed, 'status': 'KILLED' if killed else 'FAIL', 'timeout': False, 'tail': None if killed else out[-300:]}
with concurrent.futures.ThreadPoolExecutor(max_workers=int(os.environ.get('ES_MUTATION_WORKERS', '4'))) as pool:
    result = list(pool.map(one, cases))
print(json.dumps({'positive_control': 'BOTH_LABS_PASS', 'count': len(result), 'detected': sum(x['detected'] for x in result), 'timeouts': sum(x['timeout'] for x in result), 'mutations': result}, indent=1))
sys.exit(0 if all(x['detected'] for x in result) else 1)
