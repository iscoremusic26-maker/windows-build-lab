"""DKN01 FIX07 LDG01: execute the PRODUCTION adapter of Link's dodge i-frames (src/ldg_adapter.inc: LdgState, ldg_line, ldg_enemy_kind, ldg_combat_context,
on_ldg_collision_pre, on_ldg_after_cross_post) with the production pure core (src/ldg_core.hpp), against doubles of Link, his three Tg cylinders, the AT
colliders of enemies / hazards and the module trackers (Darknut, Lizalfos, Chilfos, E_OC ledger).

The order is the game's: once per executed tick Link's setCollision PRE (observe) runs, then that tick's collision pass commits each crossing through
dCcS::ChkAtTgHitAfterCross (whose POST may answer true = no hit). Checks: exact windows (9 / 6 / 6) from the real start, none for a travel roll, a wolf, a
non-combat roll; only Link's Tg; only eligible enemies (never a hazard / an unsupported actor); the forward roll is never protected against the Darknut's thrust
(the adapter's class query); the same blade keeps being evaded after the window (no deferred fictitious damage) while a new contact after a gap hits; an
event is never protected; a crossing the native already refused stays refused. Seven mutants must be detected; a compile error is never a detection."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def body(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += text[end] == '{'
        depth -= text[end] == '}'
        if depth == 0:
            return text[start:end + 1]
    raise ValueError(signature)


PREFIX = r'''
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include "ldg_core.hpp"
typedef std::int16_t s16;
typedef std::uint32_t u32;
struct ModContext {};
enum HookAction { HOOK_CONTINUE = 0 };
enum ModResult { MOD_OK = 0, MOD_ERROR = 1 };
enum { LOG_LEVEL_INFO = 1, LOG_LEVEL_WARN = 2 };
inline constexpr unsigned kLogLineMax = 512;
namespace mods { template <class T> T arg(void* args, int i) { return reinterpret_cast<T>(static_cast<void**>(args)[i]); } }
unsigned g_lines = 0, g_evaded_lines = 0;
void lnk_write(int, const char* line) { ++g_lines; if (std::strstr(line, "result=EVADED")) ++g_evaded_lines; }
enum { fpcNm_B_TN_e = 1, fpcNm_E_DN_e = 2, fpcNm_E_KK_e = 3, fpcNm_E_OC_e = 4, fpcNm_TRAP_e = 9 };
enum { fopAc_ENEMY_e = 2, fopAc_ENV_e = 5 };
struct fopAc_ac_c { std::uint32_t id = 0; int name = 0, group = 0; float x = 0, z = 0; };
std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) { return a->id; }
s16 fopAcM_GetName(const fopAc_ac_c* a) { return static_cast<s16>(a->name); }
int fopAcM_GetGroup(const fopAc_ac_c* a) { return a->group; }
fopAc_ac_c* g_world[16] = {};
bool fopAcM_SearchByID(std::uint32_t id, fopAc_ac_c** p) { for (fopAc_ac_c* a : g_world) if (a && a->id == id) { *p = a; return true; } *p = nullptr; return false; }
float fopAcM_searchActorDistance(const fopAc_ac_c* a, const fopAc_ac_c* b) { return std::sqrt((a->x - b->x) * (a->x - b->x) + (a->z - b->z) * (a->z - b->z)); }
struct cCcD_GObjInf { fopAc_ac_c* ac = nullptr; fopAc_ac_c* GetAc() { return ac; } };
struct daPy_py_c { enum { FLG1_IS_WOLF = 0x1 }; };
struct FrameCtrl { float f = 0; float getFrame() const { return f; } };
struct daAlink_c : fopAc_ac_c { int mProcID = 3; unsigned char field_0x2f98 = 0; FrameCtrl mUnderFrameCtrl[1]; cCcD_GObjInf mTgCyls[3]; fopAc_ac_c* mTargetedActor = nullptr; bool wolf = false;
                                bool checkNoResetFlg1(int) const { return wolf; } };
daAlink_c g_link;
daAlink_c* f1_link() { return &g_link; }
bool g_safe = true;
bool cp04_world_safe() { return g_safe; }
struct Posture { std::uint64_t ep = 1; int m = 30000; std::uint64_t epoch() const { return ep; } int milli() const { return m; } };
struct Lg { Posture posture; } lg;
enum class Life { Free = 0, Active };
namespace post08 { inline constexpr unsigned kSlots = 4; }
struct LedgerSlot { std::uint32_t id = 0; Life life = Life::Free; };
struct Ledger { LedgerSlot s[4]; const LedgerSlot& slot(unsigned i) const { return s[i]; } };
struct Obs { Ledger ledger; };
struct Eng { Obs obs; };
struct P8 { std::uint64_t tick = 0; };
struct G { Eng eng; P8 p8; } g;
struct StatusA { int life = 48; int getLife() const { return life; } };
struct Player { StatusA a; StatusA& getPlayerStatusA() { return a; } };
struct Info { Player p; Player& getPlayer() { return p; } };
struct GameInfo { Info info; } g_dComIfG_gameInfo;
struct DknSlot { bool on = false; std::uint32_t id = 0; bool combat = true; std::uint64_t st_ldg_evaded = 0; };
struct Dn { DknSlot s[8]; } dn;
DknSlot* dkn_find(std::uint32_t id) { for (DknSlot& s : dn.s) if (s.on && s.id == id) return &s; return nullptr; }
bool dkn_combat(const DknSlot& s) { return s.combat; }
struct DknLdg { bool own = false; int cls = 0; std::uint64_t impact = 0; };
int g_dkn_cls = 0;
std::uint64_t g_dkn_impact = 0;
DknLdg dkn_ldg_query(const fopAc_ac_c* ac, const cCcD_GObjInf*) { DknLdg r; r.own = ac && ac->name == fpcNm_B_TN_e; r.cls = g_dkn_cls; r.impact = g_dkn_impact; return r; }
struct Slot { bool on = false; std::uint32_t id = 0; };
struct Lz { Slot slots[4]; } lz;
struct Kz { Slot slots[4]; } kz;
template <class T> ModResult add_dummy(T) { return MOD_OK; }
#include <cstring>
'''

MAIN = r'''
unsigned checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { std::printf("FAIL line=%d: %s\n", __LINE__, #c); return 1; } } while (0)
// one executed tick: Link's setCollision PRE, then the crossings of this tick's pass. ret[i] = the native answer before the POST (false = hit)
void observe() { void* a[1] = {&g_link}; on_ldg_collision_pre(nullptr, a, nullptr, nullptr); }
bool cross(cCcD_GObjInf* at, cCcD_GObjInf* tg, bool native_no_hit = false) {
  bool r = native_no_hit;
  void* a[9] = {nullptr, nullptr, nullptr, at, tg, nullptr, nullptr, nullptr, nullptr};
  on_ldg_after_cross_post(nullptr, a, &r, nullptr);
  return !r;  // true = the contact is committed (a hit)
}
void tick_link(int proc, int side, float frame) { ++g.p8.tick; g_link.mProcID = proc; g_link.field_0x2f98 = static_cast<unsigned char>(side); g_link.mUnderFrameCtrl[0].f = frame; observe(); }
int main() {
  using namespace ldg::native;
  lq.enabled = true;
  g_link.id = 1, g_link.name = 100;
  fopAc_ac_c liz, dkn, oc, trap, far_liz;
  liz.id = 20, liz.name = fpcNm_E_DN_e, liz.group = fopAc_ENEMY_e, liz.x = 200;
  dkn.id = 30, dkn.name = fpcNm_B_TN_e, dkn.group = fopAc_ENEMY_e, dkn.x = 250;
  oc.id = 40, oc.name = fpcNm_E_OC_e, oc.group = fopAc_ENEMY_e, oc.x = 150;
  trap.id = 50, trap.name = fpcNm_TRAP_e, trap.group = fopAc_ENV_e, trap.x = 10;
  g_world[0] = &g_link, g_world[1] = &liz, g_world[2] = &dkn, g_world[3] = &oc, g_world[4] = &trap;
  lz.slots[0] = {true, 20};
  dn.s[0].on = true, dn.s[0].id = 30;
  g.eng.obs.ledger.s[0] = {40, Life::Active};
  cCcD_GObjInf liz_at{&liz}, dkn_at{&dkn}, oc_at{&oc}, trap_at{&trap}, other_tg{&liz};
  for (int i = 0; i < 3; ++i) g_link.mTgCyls[i].ac = &g_link;
  cCcD_GObjInf* tg = &g_link.mTgCyls[1];
  // 1) a side jump near a Lizalfos: exactly 6 passes protected from the start; the 7th contact (a new blade, no previous evasion) hits
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  unsigned evaded = 0;
  static cCcD_GObjInf pool[64];  // a different collider each pass (distinct objects): no continuity, the window alone decides
  unsigned pn = 0;
  auto fresh = [&](fopAc_ac_c* a) { pool[pn % 64].ac = a; return &pool[pn++ % 64]; };
  for (int i = 0; i < 10; ++i) {
    tick_link(kProcSideStep, kDirLeft, static_cast<float>(i));
    evaded += !cross(fresh(&liz), tg);
  }
  CHECK(evaded == 6 && lq.evaded == 6 && lq.by_enemy[1] == 6);
  // 2) a forward roll: 9 passes; against the Darknut's thrust (class 1): none; against its ordinary cut: 9
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  unsigned fwd = 0, fwd_thrust = 0;
  for (int i = 0; i < 12; ++i) {
    tick_link(kProcFrontRoll, 0, static_cast<float>(i));
    g_dkn_cls = 1, g_dkn_impact = 1000 + i;
    fwd_thrust += !cross(fresh(&dkn), tg);
    g_dkn_cls = 0, g_dkn_impact = 2000 + i;
    fwd += !cross(fresh(&dkn), tg);
  }
  CHECK(fwd == 9 && fwd_thrust == 0 && lq.excluded >= 9);
  // 3) the backflip: 6; a side jump against the thrust: protected (exclusion is forward only)
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  unsigned back = 0;
  for (int i = 0; i < 10; ++i) {
    tick_link(kProcBackJump, 0, static_cast<float>(i));
    g_dkn_cls = 1, g_dkn_impact = 3000 + i;
    back += !cross(fresh(&dkn), tg);
  }
  CHECK(back == 6);
  g_dkn_cls = 0;
  // 4) the same blade sweeping through Link after the window: still no contact (held); after a real gap it hits again (outside any window)
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  cCcD_GObjInf blade{&oc};
  unsigned hits = 0;
  for (int i = 0; i < 14; ++i) {
    tick_link(i < 10 ? kProcSideStep : 5, kDirRight, static_cast<float>(i));
    if (i >= 2 && i <= 9) hits += cross(&blade, tg);  // crossing passes 2..9: evaded at 2, held 3..9 (the window ends at 5)
  }
  CHECK(hits == 0 && lq.held_continued >= 7);
  tick_link(5, 0, 0), tick_link(5, 0, 0);
  CHECK(cross(&blade, tg));  // a new contact after a gap of a pass, no dodge: the hit stands
  // 5) never: a hazard / an unsupported actor, another target than Link's Tg, an event, a crossing the native refused, a wolf, a travel roll, a fall recovery
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  tick_link(kProcSideStep, kDirLeft, 0);
  CHECK(cross(&trap_at, tg));              // environment hazard: native
  cCcD_GObjInf liz_tg{&liz};
  CHECK(cross(&dkn_at, &liz_tg));          // a crossing on the Lizalfos' own Tg: native
  g_safe = false;
  CHECK(cross(fresh(&liz), tg));           // an event: never protected
  g_safe = true;
  CHECK(!cross(fresh(&liz), tg, true));    // the native already said "no hit": unchanged
  for (int i = 0; i < 3; ++i) tick_link(5, 0, 0);
  CHECK(!cross(fresh(&trap), tg, true) && !cross(fresh(&liz), tg, true));  // outside any dodge, a hazard / an enemy the native refused: never turned into a hit
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  g_link.wolf = true;
  tick_link(kProcSideStep, kDirLeft, 0);
  CHECK(cross(fresh(&liz), tg));           // wolf: no i-frames
  g_link.wolf = false;
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  for (fopAc_ac_c* a : {&liz, &dkn, &oc}) a->x = 5000;  // nobody within the combat reach, no Z-target: a travel roll
  tick_link(kProcFrontRoll, 0, 0);
  CHECK(cross(fresh(&liz), tg));
  liz.x = 200, dkn.x = 250, oc.x = 150;
  for (int i = 0; i < 5; ++i) tick_link(kProcFall, 0, 0);
  tick_link(kProcFrontRoll, 0, 0);         // a roll that ends a fall: a recovery, never a dodge
  CHECK(cross(fresh(&liz), tg));
  // 6) Z-targeting an enemy far away still makes it a combat dodge
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  liz.x = 5000, dkn.x = 5000, oc.x = 5000, g_link.mTargetedActor = &liz;
  tick_link(kProcSideStep, kDirRight, 0);
  CHECK(!cross(fresh(&liz), tg));
  g_link.mTargetedActor = nullptr;
  // 7) a held input / a doubled observation of the same tick never reopens or lengthens the window
  for (int i = 0; i < 5; ++i) tick_link(5, 0, 0);
  unsigned held = 0;
  liz.x = 200;
  for (int i = 0; i < 12; ++i) {
    tick_link(kProcSideStep, kDirLeft, static_cast<float>(i));
    observe();  // the same tick seen twice
    held += !cross(fresh(&liz), tg);
  }
  CHECK(held == 6);
  // 8) a new Link / scene: nothing carries over
  lg.posture.ep = 2;
  tick_link(5, 0, 0);
  CHECK(!lq.tr.d.on && lq.scene_resets >= 2);
  std::printf("LDG_ADAPTER_PASS checks=%u evaded=%llu held=%llu excluded=%llu outside=%llu not_eligible=%llu lines=%u runtime=HUMAN_RUNTIME_PENDING\n", checks,
              static_cast<unsigned long long>(lq.evaded), static_cast<unsigned long long>(lq.held_continued), static_cast<unsigned long long>(lq.excluded),
              static_cast<unsigned long long>(lq.outside), static_cast<unsigned long long>(lq.not_eligible), g_lines);
  return 0;
}
'''

FUNCS = ['void ldg_line(', 'int ldg_enemy_kind(', 'bool ldg_combat_context(', 'HookAction on_ldg_collision_pre(', 'void on_ldg_after_cross_post(']


def extract(source):
    s0 = source.index('struct LdgState {')
    s1 = source.index('} lq;', s0) + len('} lq;')
    return source[s0:s1] + '\n' + '\n'.join(body(source, f) for f in FUNCS)


def run_variant(source, directory, label):
    cpp = directory / (label + '.cpp')
    cpp.write_text(PREFIX + '\n' + extract(source) + '\n' + MAIN)
    exe = directory / (label + ('.exe' if os.name == 'nt' else ''))
    inc = str(ROOT / 'src')
    if os.name == 'nt':
        cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/I' + inc, str(cpp), '/Fe' + str(exe), '/Fo' + str(directory / (label + '.obj'))]
    else:
        cmd = [shutil.which('g++') or 'clang++', '-std=c++20', '-O2', '-I' + inc, str(cpp), '-o', str(exe)]
    built = subprocess.run(cmd, capture_output=True, text=True, cwd=directory, timeout=120)
    if built.returncode:
        raise RuntimeError('compile failure never counts as detection: ' + built.stdout + built.stderr)
    return subprocess.run([str(exe)], capture_output=True, text=True, timeout=20)


def main():
    source = (ROOT / 'src/ldg_adapter.inc').read_text()
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        positive = run_variant(source, directory, 'positive')
        print(positive.stdout, end='')
        if positive.returncode:
            print(positive.stderr)
            return 1
        mutations = [
            ('any_tg_vetoed', '  if (!link_tg) return;  // every crossing that is not on Link\'s own Tg cylinders', '  if (false) return;  // every crossing that is not on Link\'s own Tg cylinders'),
            ('any_actor_eligible', '  if (!kind) return;  // hazards, environment, other actors, unsupported enemies: native', '  if (!kind) kind_hazard_eligible();'),
            ('no_held_continuity', '  if (lq.held.continues(key, aid, dq.impact, pass)) {', '  if (false) {'),
            ('no_combat_context', '  const bool combat = fresh && cp04_world_safe() && ldg_combat_context(k);', '  const bool combat = fresh;'),
            ('event_protected', '  if (!cp04_world_safe()) return;  // an event / cutscene: never protected', '  (void)0;'),
            ('thrust_ignored', '  const ldg::Atk cls = dq.cls == 1 ? ldg::Atk::Thrust : ldg::Atk::Ordinary;', '  const ldg::Atk cls = ldg::Atk::Ordinary;'),
            ('native_refusal_overridden', '  if (*r) return;  // the native already answered "no hit"', '  if (*r) *r = false;'),
        ]
        for label, old, new in mutations:
            assert source.count(old) == 1, label
            mutated = source.replace(old, new, 1)
            if label == 'any_actor_eligible':  # the mutant: an unsupported actor counts as an eligible enemy (kind 1)
                mutated = mutated.replace('  if (!kind) kind_hazard_eligible();', '  const int kind2 = kind ? kind : 1;\n  if (!kind2) return;').replace(
                    '  ++lq.eligible_contacts;', '  ++lq.eligible_contacts;\n  const int kindx = kind2; (void)kindx;', 1).replace('++lq.by_enemy[kind - 1]', '++lq.by_enemy[kind2 - 1]').replace('kEnemy[kind - 1]', 'kEnemy[kind2 - 1]').replace(
                    'const DknLdg dq = kind == 1 ?', 'const DknLdg dq = kind2 == 1 ?').replace('    if (kind == 1)\n      if (DknSlot* s', '    if (kind2 == 1)\n      if (DknSlot* s')
            result = run_variant(mutated, directory, label)
            if result.returncode != 1 or 'FAIL line=' not in result.stdout:
                raise RuntimeError('mutation survived: ' + label + result.stdout + result.stderr)
            print(label + ': DETECTED ' + result.stdout.strip())
    print(json.dumps({'status': 'LDG_ADAPTER_PASS', 'mutants_detected': len(mutations), 'mutants_total': len(mutations),
                      'scope': 'production setCollision PRE observer + ChkAtTgHitAfterCross POST veto with the production core, doubles of Link / colliders / trackers; '
                               'no game run, no human feel'}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
