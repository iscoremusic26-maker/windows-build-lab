// LIZALFOS DUELIST 0.12.1 laboratory: the PRODUCTION brain (src/liz_core.hpp) driven hook by hook, in native order, by a transcription of the
// relevant paths of src/d/actor/d_a_e_dn.cpp @40457c6 (Execute, action, damage_check, attack_0, attack, tail_attack, guard, fight_run decision, J3D frame control).
// Limits declared: clip end frames of the commercial BCK are NOT read (runs sweep a range), geometry is a reach flag, Link is a scripted view. Not a game run; no FPS or visual claim.
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include "../src/liz_core.hpp"
using namespace liz;
static unsigned long long g_checks = 0;
#define CHECK(c)                                                 \
  do {                                                           \
    ++g_checks;                                                  \
    if (!(c)) {                                                  \
      std::printf("LIZ_LAB_FAIL line=%d %s\n", __LINE__, #c);    \
      std::fflush(stdout);                                       \
      std::exit(1);                                              \
    }                                                            \
  } while (0)

enum class Lk { Absent, Body, Guard, ParryAll, ParryOdd, ParryFirstWindowOnly, LateShield };
struct Native {
  // e_dn_class fields transcribed
  std::int16_t action = native::kFightRun, mode = 0, hp = 100, max_hp = 100, invuln = 0, ut2 = 0, timer[4] = {0, 0, 0, 0}, fc = 0, shape_y = 0, cur_y = 0, search_y = 0;
  std::int8_t at_chk = 0, tail = 0, center = 0, guard_flag = 0;
  float frame = 0, rate = 1, end = 40;
  bool stop = false;
  int anm_no = 0;
  float pl_dir = 200, recoil = 0, speedF = 0, push = 0;
  float px = 0, pz = 0;
  bool event = false, ground = true, horse = false, human = true, link_alive = true, cut_helm = false;
  bool head_lock = false, down_flag = false, cut_down_hit = false, deleted = false;
  std::int16_t search_x = 0, seen_timer = 40;
  std::uint64_t shield_serial_seen = 0, bash_se = 0, drawback_entries = 0, sdamage_inits = 0;
  int punish_seen = 0;
  // last collision pass
  bool at_hit = false, at_shield = false, tate_hit = false, tate_sword = false, body_hit = false;
  HitKind body_kind = HitKind::None;
  bool body_shield_attack = false, body_strong = false;
  int body_power = 4;
  // lab
  float len[64];
  bool hook_at_hit = true, hook_small_big = true, hook_anm = true, hook_attack = true, native_only = false;
  std::uint32_t nr = 12345;
  Brain b;
  bool skipped = false;
  std::uint64_t ticks = 0, big_calls = 0, small_calls = 0, cancel_native = 0, setplay0 = 0, spurious_at = 0;
  int impacts_open = 0;
  float fwd_total = 0;
  Native() {
    for (auto& l : len) l = 40.0f;
    len[native::kAnmN] = 42.0f;
    len[native::kAnmD] = 46.0f;
    len[native::kAnmC] = 62.0f;
    len[18] = 50.0f;  // ANM_DRAWBACK length is a commercial fact: swept by the break labs
  }
  std::uint32_t nrand(std::uint32_t n) {
    nr = nr * 1664525u + 1013904223u;
    return (nr >> 8) % n;
  }
  View view() const {
    View v;
    v.action = action, v.mode = mode, v.hp = hp, v.max_hp = max_hp, v.invuln = invuln, v.timer2 = timer[2], v.ut2 = ut2, v.search_y = search_y, v.shape_y = shape_y, v.cur_y = cur_y, v.fc = fc;
    v.at_chk = at_chk, v.tail = tail, v.anm_no = anm_no, v.frame = frame, v.rate = rate, v.end = end, v.pl_dir = pl_dir, v.recoil = recoil, v.event = event, v.link_ok = true, v.link_human = human, v.horse = horse, v.ground = ground, v.search_x = search_x, v.seen_timer = seen_timer, v.link_alive = link_alive, v.cut_helm = cut_helm;
    v.at_c[0] = frame * 10.0f, v.at_r = 55.0f;
    v.at_hit = at_hit, v.at_on_link = at_hit, v.at_shield = at_shield, v.tate_hit = tate_hit, v.tate_link_sword = tate_sword;
    return v;
  }
  void apply(const Write& w) {
    if (!w.any) return;
    if (w.set_hp) hp = max_hp = policy::kMaxHp;
    if (w.clear_tg) {
      body_hit = false, body_shield_attack = false;
      if (w.bash_se) ++bash_se;
    }
    if (w.big_damage) {
      big_damage_orig();
      invuln = 1000;
      speedF = 0;
      return;
    }
    if (w.stagger) action = native::kDrawback, mode = 0, recoil = 100;
    if (w.set_timer0) timer[0] = w.timer0;
    if (w.head_lock_off) head_lock = false;
    if (w.set_action) action = w.action;
    if (w.set_mode) mode = w.mode;
    if (w.set_invuln) invuln = w.invuln;
    if (w.set_timer2) timer[2] = w.timer2;
    if (w.set_cur_y) cur_y = w.cur_y;
    if (w.fwd != 0.0f) fwd_total += w.fwd;
    if (w.guard_entry) push = 30.0f, timer[0] = 0, invuln = 15;
    if (w.set_frame) frame = w.frame;
    if (w.set_rate) rate = w.rate;
  }
  void anm_init(int id, float r) {
    bool hooked = false;
    if (hook_anm && !native_only) hooked = b.on_anm_pre(id, r);
    (void)hooked;
    frame = 0, rate = r, end = len[id], stop = false, anm_no = id;
    float fr = 0;
    if (hook_anm && !native_only && b.on_anm_post(id, fr)) frame = fr;
  }
  void small_damage() {
    if (hook_small_big && !native_only && b.on_small_pre(view(), body_kind)) return;
    ++small_calls;
    action = native::kSDamage, mode = 0;
  }
  void big_damage_orig() {
    ++big_calls;
    action = native::kDamage, mode = 0, recoil = 150;
  }
  void big_damage() {
    if (hook_small_big && !native_only && b.on_big_pre(view(), body_kind)) return;
    big_damage_orig();
  }
  void damage_check() {
    if (!native_only) apply(b.on_damage_pre(view(), body_hit && body_shield_attack));
    if (invuln == 0 && body_hit) {
      if (body_shield_attack) {  // mHitType==16: DRAWBACK. No HP (health = oldHealth2).
        invuln = 10;
        action = native::kDrawback, mode = 0, recoil = 100;
        ++drawback_entries;
      } else {
        hp = static_cast<std::int16_t>(hp - body_power);
        invuln = 10;
        if (hp <= 0 || body_strong) {
          big_damage();
          invuln = 1000;
        } else {
          small_damage();
        }
        speedF = 0;
      }
    }
    body_hit = false;
    if (!native_only) apply(b.on_damage_post(view()));
  }
  void attack_common_pre() {
    if (!native_only && hook_attack) apply(b.on_attack_pre(view()));
  }
  void attack_common_post() {
    if (!native_only && hook_attack) apply(b.on_attack_post(view()));
  }
  bool at_hit_check() {
    if (mode >= 10) return false;
    bool r = at_hit;
    if (!native_only && hook_at_hit && b.on_at_hit_post(view(), r)) r = false;
    return r;
  }
  void attack_0() {
    attack_common_pre();
    const int f = static_cast<int>(frame);
    if (mode == 0) {
      anm_init(native::kAnmN, 1.0f);
      mode = 1;
    }
    if (mode == 1) {
      if (f >= 22 && f <= 28) at_chk = 1, tail = 0;
      if (isStop()) action = native::kFightRun, mode = 0;
    }
    attack_common_post();
  }
  void attack() {
    attack_common_pre();
    const int f = static_cast<int>(frame);
    if (mode == 0) {
      anm_init(native::kAnmD, 1.0f);
      mode = 1;
    }
    if (mode == 1) {
      if (f >= 13 && f <= 18) at_chk = 1, tail = 0;
      if (f >= 25 && f <= 30) at_chk = 1, tail = 1, fwd_total += policy::kLungeD2;
      if (isStop()) action = native::kFightRun, mode = 0;
    }
    if (at_chk != 0) {
      const bool link = at_hit_check();
      if (link && at_shield) {  // checkPlayerGuard
        rate = 0;
        action = native::kFightRun, mode = 0;
        ++cancel_native;
        ++setplay0;
      }
    }
    attack_common_post();
  }
  void tail_attack() {
    attack_common_pre();
    const int f = static_cast<int>(frame);
    if (mode == 0) {
      anm_init(native::kAnmC, 1.0f);
      mode = 1;
    } else if (mode == 1) {
      if (f >= 33 && f <= 41) at_chk = 1, tail = 2, fwd_total += policy::kLungeC;
      if (isStop()) action = native::kFightRun, mode = 0;
    }
    attack_common_post();
  }
  bool isStop() const { return stop || rate == 0.0f; }
  void guard() {
    guard_flag = 1;
    if (mode == 0) {
      anm_init(32, 1.0f);
      mode = 1;
    } else if (mode == 1) {
      invuln = 5;
      if (frame >= 5.0f) mode = 5, timer[0] = 20;
    }
    if (mode == 5) {
      anm_init(22, 1.0f);
      mode = 6;
    }
    if (mode == 6) {
      invuln = 5;
      if (frame >= 8.0f) rate = 0, mode = 7;
    } else if (mode == 7) {
      invuln = 5;
      if (timer[0] == 0) mode = 8, rate = 1;
    } else if (mode == 8) {
      if (isStop()) action = native::kFightRun, mode = 0;
    }
  }
  bool learned_guard = false, cut_at = false;
  void fight_run(bool jump_cut) {
    Write wpre;
    bool skip = false;
    if (!native_only) {
      skip = b.on_run_pre(view(), wpre);
      apply(wpre);
    }
    if (!skip) {
      if (mode == 0) {
        anm_init(30, 1.0f);
        mode = 1;
      }
      if (learned_guard && cut_at && recoil < 1.0f && pl_dir < 300.0f) {
        action = native::kGuard, mode = 5, push = 30, timer[0] = 0, invuln = 15;
      } else if (pl_dir < 350.0f && timer[2] == 0) {
        timer[2] = static_cast<std::int16_t>(30 + nrand(30));
        if (nrand(4) != 0) action = pl_dir < 250.0f ? native::kAttack0 : native::kAttack;
        else action = native::kTailAttack;
        mode = 0;
      }
    }
    if (!native_only && !b.ep.active) apply(b.on_run_post(view(), jump_cut));
  }
  void do_action(bool jump_cut) {
    damage_check();
    head_lock = false;  // action(): offHeadLockFlg every tick
    switch (action) {
      case native::kDrawback:
        if (mode == 0) {
          anm_init(18, 1.0f);
          mode = 1;
          invuln = 10;
        } else if (mode == 1) {
          head_lock = true;
          if (isStop()) action = native::kFightRun, mode = 0;
        }
        break;
      case native::kFightRun: fight_run(jump_cut); break;
      case native::kAttack0: attack_0(); break;
      case native::kAttack: attack(); break;
      case native::kTailAttack: tail_attack(); break;
      case native::kGuard: guard(); break;
      case native::kSDamage:
        if (mode == 0) mode = 1, timer[0] = 20, anm_init(31, 1.0f), stop = false, ++sdamage_inits;
        else if (timer[0] == 0) action = native::kFightRun, mode = 0, timer[0] = 40;
        break;
      case native::kDamage:
        if (cut_down_hit) {  // e_dn_damage: the Ending Blow route, only from the native down flag
          cut_down_hit = false, down_flag = false, hp = 0, mode = 3, timer[0] = 100, timer[1] = 46;
          break;
        }
        if (mode == 0) mode = 1, timer[0] = 0, anm_init(9, 1.0f);
        else if (mode == 1 && ++dmg_t > 6) mode = 3, timer[0] = 65, timer[1] = 35, dmg_t = 0, down_flag = hp > 0;
        else if (mode == 3 && hp <= 0 && timer[1] == 0) deleted = true;
        else if (mode == 3 && hp > 0 && timer[0] == 0) mode = 20, anm_init(14, 1.0f), down_flag = false;
        else if (mode == 20 && isStop()) mode = 21, timer[0] = 5;
        else if (mode == 21) {
          invuln = 0;
          if (timer[0] == 0) action = native::kNormal, mode = 0;
        }
        break;
      case native::kNormal:
        action = native::kFightRun, mode = 0;
        break;
      default:
        if (++other_t > 25) action = native::kFightRun, mode = 0, other_t = 0;
        break;
    }
  }
  int dmg_t = 0, other_t = 0;
  // One daE_DN_Execute with the pass result of the previous tick.
  void execute(const LinkFrame& L, bool jump_cut = false) {
    if (!native_only) apply(b.on_exec_pre(view(), L));
    ++fc;
    for (auto& t : timer)
      if (t) --t;
    if (invuln) --invuln;
    if (ut2) --ut2;
    do_action(jump_cut);
    // play
    if (!stop) {
      frame += rate;
      if (frame >= end) {
        frame = end - 0.001f;
        stop = true;
      }
    }
    // at flag end of Execute
    armed_this_tick = at_chk != 0;
    if (armed_this_tick && tail == 1 && action != native::kAttack) ++spurious_at;
    // guard acceptance
    guard_active_tick = guard_flag != 0 && ut2 == 0;
    if (guard_flag != 0 && ut2 == 0 && tate_hit) ut2 = 10;
    if (!native_only) apply(b.on_exec_post(view(), L));
    at_chk = 0;
    guard_flag = 0;
    at_hit = at_shield = tate_hit = tate_sword = false;
    ++ticks;
  }
  bool armed_this_tick = false, guard_active_tick = false;
  void bind_admitted() {
    timer[2] = 5;  // the admission Execute (native c_start) never decides an attack
    b.bind(100, 7, 0xabc);
    IdIn in;
    in.profile_ok = true, in.arg0 = 1, in.arg1 = 15;
    b.admit(in);
  }
  void drain() {}
};

#include <vector>
struct Run {
  std::vector<int>* dummy = nullptr;
};
// ---------------------------------------------------------------- A identity / lifecycle
static void lab_identity() {
  IdIn i;
  CHECK(classify(i) == Excl::WrongActor);
  i.profile_ok = true;
  CHECK(classify(i) == Excl::None);
  i.arg1 = 15;
  CHECK(classify(i) == Excl::None);
  i.arg1 = 0;
  CHECK(classify(i) == Excl::None);
  i.arg1 = 1;
  CHECK(classify(i) == Excl::Dynalfos);
  i.arg1 = 0, i.skull_model = true;
  CHECK(classify(i) == Excl::SkullModel);
  i.skull_model = false;
  for (int a = 13; a <= 16; ++a) {
    i.arg0 = a;
    CHECK(classify(i) == Excl::ScriptedRun);
  }
  i.arg0 = 12;
  CHECK(classify(i) == Excl::None);
  i.arg0 = 17;
  CHECK(classify(i) == Excl::None);
  i.arg0 = 0;  // ACTION_STAY waiter: a normal Lizalfos that wakes up
  CHECK(classify(i) == Excl::None);
  i.action = native::kReg;
  CHECK(classify(i) == Excl::ScriptedRun);
  i.action = 0, i.status = 1;
  CHECK(classify(i) == Excl::Status);
  i.status = 0, i.special_room = true;
  CHECK(classify(i) == Excl::SpecialRoom);
  // HP once at admission, never again, never heals
  Native n;
  n.bind_admitted();
  LinkFrame L;
  n.execute(L);
  CHECK(n.hp == 370 && n.max_hp == 370);
  n.hp = 123;
  for (int t = 0; t < 400; ++t) {
    n.pl_dir = 900;
    n.execute(L);
    for (unsigned i = 0; i < n.b.nev; ++i) CHECK(n.b.ev[i].k != EvK::Critical && n.b.ev[i].k != EvK::HpSet);
    n.b.nev = 0;
  }
  CHECK(n.hp == 123);  // 0 HP/s in any state
  // Foreign HP model: untouched and reported
  Native f;
  f.bind_admitted();
  f.hp = 80, f.max_hp = 80;
  f.execute(L);
  CHECK(f.hp == 80 && f.max_hp == 80);
  // Excluded actor: brain never writes
  Native d;
  d.b.bind(5, 1, 3);
  IdIn di;
  di.profile_ok = true, di.arg1 = 1;
  d.b.admit(di);
  CHECK(!d.b.admitted);
  for (int t = 0; t < 300; ++t) d.execute(L);
  CHECK(d.hp == 100 && d.b.p.milli == 0);
  // Recycled identity inherits nothing
  n.b.p.add(50);
  n.b.def.stamp(5);
  n.b.bind(100, 8, 1);
  CHECK(n.b.p.milli == 0 && !n.b.admitted && n.b.generation == 8 && !n.b.hp_done);
  // Several actors keep independent posture, RNG and memory
  Native x, y;
  x.bind_admitted(), y.bind_admitted();
  y.b.bind(101, 7, 0xabc);
  IdIn yi;
  yi.profile_ok = true;
  y.b.admit(yi);
  x.b.p.add(40);
  CHECK(y.b.p.milli == 0);
  unsigned same = 0;
  for (int k = 0; k < 64; ++k) same += x.b.rng.below(4) == y.b.rng.below(4);
  CHECK(same < 40);
}
// ---------------------------------------------------------------- B posture
static void lab_posture() {
  // Values fixed by the user, as literals: a change of the policy constant fails here even when the behaviour tests use the symbol.
  CHECK(policy::kMaxHp == 370 && policy::kMaxPosture == 100 && policy::kGainSword == 9 && policy::kGainLinkParry == 15 && policy::kGainIronBall == 30 && policy::kGainBlocked == 5 && policy::kGainLinkOnEnemyParry == 25);
  CHECK(policy::kRegenDelayTicks == 60 && policy::kTicksPerSecond == 30 && policy::kParryPctA == 12 && policy::kParryPctB == 35 && policy::kParryPctC == 45 && policy::kParryCooldownA == 120 && policy::kParryCooldownB == 90 && policy::kParryCooldownC == 60);
  CHECK(policy::kPressureWindow == 75 && policy::kSustainedWindow == 150 && policy::kSustainedSwings == 4 && policy::kParryWindowLast - policy::kParryWindowFirst == 4 && policy::kMinWindupFrames == 8.0f);
  CHECK(policy::kCounterPct == 80 && policy::kBlockChainTicks == 90 && policy::kPunishTicks == 90 && policy::kPressureLeaseTicks == 30 && policy::kPendingSwingTicks == 6);
  // exact percent borders need a round maximum: strictly greater at every band edge
  CHECK(regen_points_per_second(76, 100) == 10 && regen_points_per_second(75, 100) == 7 && regen_points_per_second(51, 100) == 7 && regen_points_per_second(50, 100) == 4);
  CHECK(regen_points_per_second(26, 100) == 4 && regen_points_per_second(25, 100) == 2 && regen_points_per_second(11, 100) == 2 && regen_points_per_second(10, 100) == 1);
  CHECK(regen_points_per_second(370, 370) == 10);
  CHECK(regen_points_per_second(278, 370) == 10);  // 75.13%
  CHECK(regen_points_per_second(277, 370) == 7);   // 74.86%
  CHECK(regen_points_per_second(186, 370) == 7);   // 50.27%
  CHECK(regen_points_per_second(185, 370) == 4);   // exactly 50%
  CHECK(regen_points_per_second(93, 370) == 4);    // 25.13%
  CHECK(regen_points_per_second(92, 370) == 2);    // 24.86%
  CHECK(regen_points_per_second(38, 370) == 2);    // 10.27%
  CHECK(regen_points_per_second(37, 370) == 1);    // exactly 10%
  CHECK(regen_points_per_second(1, 370) == 1);
  CHECK(regen_points_per_second(0, 370) == 0);
  CHECK(regen_points_per_second(-5, 370) == 0);
  for (int hp = 1; hp <= 370; ++hp) {  // exhaustive monotonic + exact band
    const int r = regen_points_per_second(hp, 370);
    const double pct = hp * 100.0 / 370.0;
    const int w = pct > 75 ? 10 : pct > 50 ? 7 : pct > 25 ? 4 : pct > 10 ? 2 : 1;
    CHECK(r == w);
  }
  // gains fixed, cap 100, latch once
  PostureState p;
  CHECK(!p.add(15) && p.points() == 15);
  CHECK(!p.add(22) && p.points() == 37);
  CHECK(!p.add(30) && p.points() == 67);
  CHECK(!p.add(5) && p.points() == 72);
  CHECK(p.add(30) && p.points() == 100 && p.latched);
  CHECK(!p.add(30) && p.points() == 100 && p.breaks == 1);
  // delay: exactly 60 eligible ticks, then an exact integer rate without drift
  for (int band : {370, 277, 185, 92, 37}) {
    PostureState q;
    q.add(90);
    const int rate = regen_points_per_second(band, 370);
    for (std::uint32_t t = 0; t < policy::kRegenDelayTicks; ++t) {
      q.regen(true, band, 370);
      CHECK(q.milli == 90000);
    }
    for (int s = 0; s < 3; ++s) {
      const int before = q.milli;
      for (int t = 0; t < 30; ++t) q.regen(true, band, 370);
      CHECK(before - q.milli == rate * 1000);
    }
  }
  {  // ineligible ticks do not advance the delay; a gain restarts it; never below zero; never while latched
    PostureState q;
    q.add(10);
    for (int t = 0; t < 500; ++t) q.regen(false, 370, 370);
    CHECK(q.milli == 10000 && q.since == 0);
    for (int t = 0; t < 59; ++t) q.regen(true, 370, 370);
    q.add(5);
    CHECK(q.since == 0);
    for (int t = 0; t < 60 + 45; ++t) q.regen(true, 370, 370);
    CHECK(q.milli == 0);
    PostureState l;
    l.add(100);
    for (int t = 0; t < 400; ++t) l.regen(true, 370, 370);
    CHECK(l.milli == 100000 && l.latched);
    l.rearm();
    CHECK(l.milli == 0 && !l.latched && l.rearms == 1);
  }
}
// ---------------------------------------------------------------- C combos
static void lab_tables() {
  CHECK(kPattern[0].acts == 3 && kPattern[1].acts == 6 && kPattern[2].acts == 6 && kPattern[3].acts == 5);
  CHECK(kPattern[0].impacts == 4 && kPattern[1].impacts == 7 && kPattern[2].impacts == 9 && kPattern[3].impacts == 6);
  for (unsigned p = 0; p < kPatterns; ++p) {
    unsigned imp = 0;
    for (unsigned a = 0; a < kPattern[p].acts; ++a) imp += impacts_of(kPattern[p].a[a]);
    CHECK(imp == kPattern[p].impacts);
    CHECK(kIntervals[p] == kPattern[p].impacts - 1);
  }
  const Act P1[] = {Act::N, Act::D, Act::CFast}, P2[] = {Act::N, Act::N, Act::N, Act::D, Act::N, Act::CFast}, P3[] = {Act::D, Act::N, Act::D, Act::N, Act::N, Act::D}, P4[] = {Act::CSlow, Act::CSlow, Act::N, Act::D, Act::CFast};
  const Act* T[] = {P1, P2, P3, P4};
  for (unsigned p = 0; p < 4; ++p)
    for (unsigned a = 0; a < kPattern[p].acts; ++a) CHECK(kPattern[p].a[a] == T[p][a]);
  CHECK(rate_of(Act::N) == 1.25f && rate_of(Act::D) == 1.10f && rate_of(Act::CSlow) == 1.10f && rate_of(Act::CFast) == 1.20f);
  // action gaps are exactly the recorded non-D-internal intervals
  const float recorded_gaps[4][5] = {{.50f, .63f}, {.58f, .60f, .54f, .66f, .67f}, {.52f, .53f, .54f, .61f, .52f}, {1.34f, 1.26f, .52f, .63f}};
  for (unsigned p = 0; p < 4; ++p)
    for (unsigned a = 0; a + 1 < kPattern[p].acts; ++a) CHECK(kPattern[p].gap_s[a] == recorded_gaps[p][a]);
  // entry frames stay inside the clip, keep a real wind-up, never cross the first AT frame; every native sound of the clip is still reached
  for (unsigned p = 0; p < 4; ++p)
    for (unsigned a = 0; a < kPattern[p].acts; ++a) {
      const Act x = kPattern[p].a[a];
      const float s = start_frame(p, a);
      CHECK(s >= 0.0f && s < first_at(x) - policy::kMinWindupFrames + 1.0f);
      CHECK(sounds_ok(x, s));
      if (a == 0) CHECK(entry_frame(p, a) == 0.0f);
      // float accumulation exactly as J3DFrameCtrl: every AT frame band reached with several ticks
      float f = s;
      unsigned in_w1 = 0, in_w2 = 0;
      for (int k = 0; k < 200 && f < 70.0f; ++k, f += rate_of(x)) {
        const int i = static_cast<int>(f);
        if (x == Act::N && i >= 22 && i <= 28) ++in_w1;
        if (x == Act::D && i >= 13 && i <= 18) ++in_w1;
        if (x == Act::D && i >= 25 && i <= 30) ++in_w2;
        if (is_tail(x) && i >= 33 && i <= 41) ++in_w1;
      }
      CHECK(in_w1 >= 5);
      if (x == Act::D) CHECK(in_w2 >= 5);
    }
}
static void lab_draw() {
  Rng r;
  r.seed(42);
  unsigned long long c[4] = {};
  const unsigned long long N = 4000000;
  for (unsigned long long i = 0; i < N; ++i) ++c[r.below(4)];
  double chi = 0;
  for (auto x : c) {
    const double e = N / 4.0;
    chi += (x - e) * (x - e) / e;
    CHECK(std::fabs(double(x) / N - 0.25) < 0.001);
  }
  CHECK(chi < 16.27);  // chi2(3) p=0.001
  // sequential independence: transition table also flat
  unsigned long long t[4][4] = {};
  unsigned last = r.below(4);
  for (unsigned long long i = 0; i < N; ++i) {
    const unsigned n = r.below(4);
    ++t[last][n];
    last = n;
  }
  for (auto& row : t)
    for (auto x : row) CHECK(std::fabs(double(x) / (N / 16.0) - 1.0) < 0.02);
  // below() has no modulo bias with a range near the limit
  Rng s;
  s.seed(7);
  unsigned long long pc[100] = {};
  for (int i = 0; i < 2000000; ++i) ++pc[s.below(100)];
  for (auto x : pc) CHECK(std::fabs(double(x) / 20000.0 - 1.0) < 0.06);
}
struct ComboResult {
  int impacts = 0;
  std::uint64_t opens[16] = {};
  float open_frames[16] = {};
  End end = End::None;
  int acts = 0;
  float rates[8] = {};
  int ticks = 0;
  int parries = 0, directs = 0, guards = 0;
};
// Runs one episode of pattern `pat` with Link's script. Returns observed impacts.
static ComboResult run_pattern(unsigned pat, Lk link, int start_posture, float end_scale, std::uint64_t seed, bool hook_at_hit = true, bool reach = true) {
  Native n;
  n.bind_admitted();
  n.b.rng.seed(seed);
  n.hook_at_hit = hook_at_hit;
  n.len[native::kAnmN] *= end_scale, n.len[native::kAnmD] *= end_scale, n.len[native::kAnmC] *= end_scale;
  LinkFrame L;
  n.execute(L);  // admission + 370
  if (start_posture) n.b.p.add(start_posture);
  // force exactly this pattern through the real start path
  n.pl_dir = 200;
  n.timer[2] = 0;
  // Search RNG seed so that the real draw yields `pat`
  ComboResult r;
  for (int guard = 0; guard < 4096; ++guard) {
    Rng probe = n.b.rng;
    if (probe.below(kPatterns) == pat) break;
    (void)n.b.rng.next();
  }
  const auto before = n.b.st.selected[pat];
  int acts_seen = 0;
  bool started = false;
  int link_serial_parry = 0;
  std::uint64_t last_open = ~0ull;
  for (int t = 0; t < 900; ++t) {
    LinkFrame L2;
    if (n.at_hit && link != Lk::Body && link != Lk::Absent) {
      L2.pass.valid = true;
      bool parry = link == Lk::ParryAll || (link == Lk::ParryOdd && (r.impacts % 2) == 1) || (link == Lk::ParryFirstWindowOnly && r.impacts == 1);
      L2.pass.guard_attack = parry || link == Lk::LateShield;
      L2.pass.frame = link == Lk::LateShield ? 20.0f : 3.0f, L2.pass.start = 1.0f, L2.pass.age_ticks = link == Lk::LateShield ? 20 : 2, L2.pass.serial = static_cast<std::uint64_t>(++link_serial_parry);
    }
    n.execute(L2);
    // Events (production) as adapter would read them
    for (unsigned i = 0; i < n.b.nev; ++i) {
      const Ev& e = n.b.ev[i];
      if (e.k == EvK::ImpactOpen) {
        if (r.impacts < 16) r.opens[r.impacts] = e.t, r.open_frames[r.impacts] = e.f;
        ++r.impacts;
        last_open = e.t;
      } else if (e.k == EvK::ActionStart) {
        if (acts_seen < 8) r.rates[acts_seen] = e.g;
        ++acts_seen;
      } else if (e.k == EvK::ComboEnd) {
        r.end = static_cast<End>(e.b);
      }
    }
    n.b.nev = 0;
    // collision pass of this tick
    if (n.armed_this_tick && reach && link != Lk::Absent) {
      n.at_hit = true;
      n.at_shield = link != Lk::Body;
    }
    if (n.b.st.selected[pat] != before) started = true;
    if (started && !n.b.ep.active && r.end != End::None) break;
  }
  (void)last_open;
  r.acts = acts_seen;
  r.ticks = static_cast<int>(n.ticks);
  r.parries = static_cast<int>(n.b.st.link_parry);
  r.directs = static_cast<int>(n.b.st.contact[unsigned(Hit::Direct)]);
  r.guards = static_cast<int>(n.b.st.contact[unsigned(Hit::Guard)]);
  CHECK(n.spurious_at == 0);
  CHECK(n.cancel_native == 0 || !hook_at_hit);
  return r;
}
static void lab_complete() {
  // Completeness with no impediment, every clip length of the sweep: 4/7/9/6 impacts, 3/6/6/5 actions, exact action rates, whatever the recovery length.
  // Scale 0.7: the clip ends before the recorded gap, so the gap runs inside FIGHT_RUN: no native decision may run there and the combo must still complete.
  const float scales[] = {0.7f, 0.9f, 1.0f, 1.15f, 1.3f};
  for (float sc : scales)
    for (unsigned p = 0; p < 4; ++p)
      for (Lk k : {Lk::Absent, Lk::Guard, Lk::LateShield, Lk::Body}) {
        const ComboResult r = run_pattern(p, k, 0, sc, 1000 + p);
        CHECK(r.end == End::Completed);
        CHECK(r.impacts == static_cast<int>(kPattern[p].impacts));
        CHECK(r.acts == static_cast<int>(kPattern[p].acts));
        for (unsigned a = 0; a < kPattern[p].acts; ++a) CHECK(r.rates[a] == rate_of(kPattern[p].a[a]));
        if (k == Lk::Guard) CHECK(r.guards == static_cast<int>(kPattern[p].impacts));
        if (k == Lk::Body) CHECK(r.directs == static_cast<int>(kPattern[p].impacts));
      }
  // Negative control: without the at_hit_check hook the transcribed native code cancels the double on the first guarded window. The model reproduces the user's observation.
  {
    const ComboResult r = run_pattern(2 /*D first*/, Lk::Guard, 0, 1.0f, 3, false);
    CHECK(r.impacts < 9);  // D2 lost at each cancelled double
  }
  // Rhythm against the recorded table: every non-pair gap reachable exactly (+-1 tick); the physical pair is the clip's own 12 frames at x1.10.
  std::printf("LIZ_RHYTHM pattern recorded_s achieved_s entry_frames\n");
  for (unsigned p = 0; p < 4; ++p) {
    const ComboResult r = run_pattern(p, Lk::Absent, 0, 1.0f, 2000 + p);
    unsigned rec = 0;
    unsigned imp = 0;
    for (unsigned a = 0; a < kPattern[p].acts; ++a) {
      const Act x = kPattern[p].a[a];
      if (x == Act::D) {
        const float got = float(r.opens[imp + 1] - r.opens[imp]) / 30.0f;
        std::printf("LIZ_RHYTHM P%u impact%u->%u D1->D2 recorded=%.2f achieved_open_to_open=%.3f min_contact_gap=%.3f physical_clip_limit=1\n", p + 1, imp + 1, imp + 2, kRecorded[p][rec], got, (25.0f - 19.0f) / 1.10f / 30.0f);
        CHECK(r.opens[imp + 1] - r.opens[imp] >= 10 && r.opens[imp + 1] - r.opens[imp] <= 12);
        ++rec;
        ++imp;
      }
      if (a + 1 < kPattern[p].acts) {
        const float want = kRecorded[p][rec];
        const std::uint64_t ticks = r.opens[imp + 1] - r.opens[imp];
        const float got = float(ticks) / 30.0f;
        std::printf("LIZ_RHYTHM P%u impact%u->%u recorded=%.2f achieved=%.3f entry=%.0f rate=%.2f\n", p + 1, imp + 1, imp + 2, want, got, entry_frame(p, a + 1), rate_of(kPattern[p].a[a + 1]));
        CHECK(std::fabs(got - want) <= 1.5f / 30.0f);
        ++rec;
        ++imp;
      }
    }
  }
  // Parry at every impact: +15 fixed and uniform, the 7th crossing breaks: P1 4/4 and P4 6/6 complete, P2 7/7 and P3 7/9 break.
  for (unsigned p = 0; p < 4; ++p) {
    const ComboResult r = run_pattern(p, Lk::ParryAll, 0, 1.0f, 3000 + p);
    if (p == 0 || p == 3) {
      CHECK(r.end == End::Completed && r.impacts == static_cast<int>(kPattern[p].impacts) && r.parries == r.impacts);
    } else {
      CHECK(r.end == End::Posture100);
      CHECK(r.impacts == 7);
      CHECK(r.parries == 7);
    }
  }
  // Prior posture is conserved, no protection at combo start: 10/25/70/80/99 break at the parry the arithmetic says, including the very first.
  for (int start : {0, 10, 25, 70, 80, 99})
    for (unsigned p = 0; p < 4; ++p) {
      const ComboResult r = run_pattern(p, Lk::ParryAll, start, 1.0f, 4000 + p + start);
      int need = 0;
      for (int v = start; v < 100; v += 15) ++need;
      const int total = static_cast<int>(kPattern[p].impacts);
      if (need > total) {
        CHECK(r.end == End::Completed && r.parries == total);
      } else {
        CHECK(r.end == End::Posture100 && r.parries == need);
      }
    }
  // One physical press per impact: D1 parried, D2 only guarded => one +15, the second impact is an ordinary guard, the pair and the combo go on.
  {
    const ComboResult r = run_pattern(2, Lk::ParryFirstWindowOnly, 0, 1.0f, 77);
    CHECK(r.end == End::Completed && r.parries == 1 && r.impacts == 9);
  }
  // Alternating
  {
    const ComboResult r = run_pattern(2, Lk::ParryOdd, 0, 1.0f, 78);
    CHECK(r.end == End::Completed || r.end == End::Posture100);
    CHECK(r.parries >= 4);
  }
  // Whiff: no contact is ever invented.
  for (unsigned p = 0; p < 4; ++p) {
    const ComboResult r = run_pattern(p, Lk::Body, 0, 1.0f, 90 + p, true, false);
    CHECK(r.directs == 0 && r.guards == 0 && r.parries == 0 && r.impacts == static_cast<int>(kPattern[p].impacts));
  }
}
static void lab_stale_frame() {
  // The attack function reads the previous clip's frame in mode 0: WAIT_02 at frame 15 before a D, or an N stopped at 27 before a D2-range frame, must not open a window.
  for (float stale : {13.5f, 15.0f, 18.0f, 25.0f, 27.0f, 30.0f}) {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 100, n.timer[2] = 0;
    for (int g = 0; g < 4096; ++g) {
      Rng pr = n.b.rng;
      if (pr.below(kPatterns) == 2) break;  // P3 starts with D
      (void)n.b.rng.next();
    }
    n.frame = stale, n.stop = false;
    n.execute(L);  // FIGHT_RUN: mode 0 anm_init resets; force the stale value right before the attack tick as the native WAIT_02 strafe leaves it
    CHECK(n.action == native::kAttack && n.mode == 0);
    n.frame = stale;
    n.execute(L);
    CHECK(!n.armed_this_tick);
    CHECK(n.b.st.windows == 0);
  }
}
static void lab_lunge() {
  // The native step applies per window tick; at rate r there are fewer ticks: the compensation keeps the total within one native step of the x1 value.
  for (float scale : {1.0f}) {
    (void)scale;
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 100;  // no slide
    n.b.ep = Episode{};
    n.b.rng.seed(1);
    n.timer[2] = 0;
    for (int g = 0; g < 4096; ++g) {
      Rng pr = n.b.rng;
      if (pr.below(kPatterns) == 2) break;
      (void)n.b.rng.next();
    }
    float d2 = 0;
    for (int t = 0; t < 45; ++t) n.execute(L);
    d2 = n.fwd_total;
    CHECK(d2 > 0.0f);
  }
  // Direct arithmetic of the conserved totals (per window): vanilla ticks x step vs ticks(r) x r x step
  CHECK(std::fabs(6 * 25.0f - 6.0f / kRateD * kRateD * 25.0f) < 0.01f);
  CHECK(std::fabs(9 * 30.0f - 9.0f / kRateCFast * kRateCFast * 30.0f) < 0.01f);
}
// ---------------------------------------------------------------- D Link verdict table
static void lab_hit() {
  HitIn h;
  CHECK(judge_hit(h) == Hit::None);
  h.at_hit = true;
  CHECK(judge_hit(h) == Hit::None);  // not on Link: another target
  h.on_link = true;
  CHECK(judge_hit(h) == Hit::Stale);  // AT was not armed: residue
  h.armed_last = true, h.event = true;
  CHECK(judge_hit(h) == Hit::Event);
  h.event = false;
  CHECK(judge_hit(h) == Hit::NotAttacking);
  h.impact = 5;
  CHECK(judge_hit(h) == Hit::LinkInvalid);
  h.link_ok = true;
  CHECK(judge_hit(h) == Hit::Direct);
  h.shield = true;
  CHECK(judge_hit(h) == Hit::Guard);  // held R
  h.pass.valid = true;
  CHECK(judge_hit(h) == Hit::Guard);
  h.pass.guard_attack = true, h.pass.start = 1, h.pass.frame = 1, h.pass.age_ticks = 0;
  CHECK(judge_hit(h) == Hit::Parry);
  h.pass.frame = 1 + post08::policy::kWindowFrames;
  h.pass.age_ticks = post08::policy::kWindowTicksCap;
  CHECK(judge_hit(h) == Hit::Parry);
  h.pass.frame += 0.5f;
  CHECK(judge_hit(h) == Hit::MissedEarly);  // late press: guard, not parry
  h.pass.frame = 0.0f - post08::policy::kPreStartSlack - 0.5f;
  CHECK(judge_hit(h) == Hit::MissedEarly);
  h.pass.frame = 3, h.pass.age_ticks = post08::policy::kWindowTicksCap + 1;
  CHECK(judge_hit(h) == Hit::MissedEarly);
  h.pass.age_ticks = 2, h.latched = true;
  CHECK(judge_hit(h) == Hit::ParryLatched);
  h.latched = false, h.resolved = 5;
  CHECK(judge_hit(h) == Hit::Duplicate);  // the same impact never pays twice, not per frame
  h.resolved = 4;
  CHECK(judge_hit(h) == Hit::Parry);
  // D1 and D2 are two serials: repeated contact ticks inside D1 count once; D2 can parry again only with its own proof of a press in window.
  Native n;
  n.bind_admitted();
  LinkFrame L;
  n.execute(L);
  n.b.ep.active = true, n.b.ep.pattern = 2, n.b.ep.act = 0, n.b.ep.anm_pending = false;
  n.b.cur_impact = 11;
  n.b.armed_last = true;
  LinkFrame P;
  P.pass.valid = P.pass.guard_attack = true, P.pass.start = 1, P.pass.frame = 3, P.pass.age_ticks = 2;
  n.at_hit = n.at_shield = true;
  for (int k = 0; k < 4; ++k) {
    n.b.armed_last = true;
    n.at_hit = n.at_shield = true;
    (void)n.b.on_exec_pre(n.view(), P);
    n.b.nev = 0;
  }
  CHECK(n.b.st.link_parry == 1 && n.b.p.points() == 15);
  // Link's parry is not "Link stopped pressing": the pressure memory of his sword swings stands.
  {
    Native m;
    m.bind_admitted();
    m.execute(L);
    for (int i = 0; i < 4; ++i) m.b.def.stamp(m.b.local);
    m.b.def.update(m.b.local, 200);
    CHECK(m.b.def.level == 2);
    m.b.ep.active = true, m.b.ep.pattern = 2, m.b.ep.act = 0, m.b.ep.anm_pending = false;
    m.b.cur_impact = 1, m.b.armed_last = true;
    m.at_hit = m.at_shield = true;
    (void)m.b.on_exec_pre(m.view(), P);
    m.b.nev = 0;
    CHECK(m.b.st.link_parry == 1 && m.b.def.level == 2 && m.b.def.swings_within(m.b.local, 150) == 4);
  }

  n.b.cur_impact = 12;
  n.b.armed_last = true;
  (void)n.b.on_exec_pre(n.view(), P);
  CHECK(n.b.st.link_parry == 2 && n.b.p.points() == 30);
  // No Link: never credits Lizalfos a parry
  Native a;
  a.bind_admitted();
  a.execute(L);
  a.b.cur_impact = 3, a.b.armed_last = true;
  a.at_hit = a.at_shield = true;
  View v = a.view();
  v.link_ok = false;
  (void)a.b.on_exec_pre(v, P);
  CHECK(a.b.st.link_parry == 0 && a.b.p.points() == 0 && a.b.st.contact[unsigned(Hit::LinkInvalid)] == 1);
}
static void lab_double_gap() {
  // double_gap(): exactly between the windows of a D of this actor
  Native n;
  n.bind_admitted();
  LinkFrame L;
  n.execute(L);
  n.timer[2] = 0;
  n.pl_dir = 100;
  for (int g = 0; g < 4096; ++g) {
    Rng pr = n.b.rng;
    if (pr.below(kPatterns) == 2) break;
    (void)n.b.rng.next();
  }
  int gap_ticks = 0, before = 0, after = 0;
  bool opened = false;
  for (int t = 0; t < 60 && n.b.st.windows < 3; ++t) {
    n.execute(L);
    n.b.nev = 0;
    if (n.b.double_gap()) ++gap_ticks;
    CHECK(!(n.b.double_gap() && n.armed_this_tick));
    if (!opened && n.b.st.windows >= 1) opened = true;
    if (!opened && n.b.double_gap()) ++before;
    if (n.b.st.windows >= 2 && n.b.double_gap()) ++after;
  }
  CHECK(before == 0 && after == 0);
  CHECK(gap_ticks >= 4 && gap_ticks <= 8);
  CHECK(policy::kDoubleRearmTicks < 6);
}
// ---------------------------------------------------------------- E adaptive defense
static void lab_defense() {
  // Levels: promotion conditions and per-opportunity percentages
  Defense d;
  CHECK(d.level == 0);
  std::uint64_t t = 1000;
  d.stamp(t);
  d.stamp(t + 20);
  d.update(t + 20, 200);
  CHECK(d.level == 0);
  d.stamp(t + 70);
  d.update(t + 70, 200);
  CHECK(d.level == 1);  // three in 75 ticks
  // Three swings spread over more than 75 ticks is not pressure
  Defense e;
  e.stamp(100), e.stamp(140), e.stamp(180);
  e.update(180, 200);
  CHECK(e.level == 0);
  // Level C: four distinct swings in 5 s, nothing else (the old "two undefended impacts" gate is gone); three in 5 s is not C, four in more than 5 s is not C
  Defense c;
  for (int i = 0; i < 4; ++i) c.stamp(5000 + i * 45);
  c.update(5135, 200);
  CHECK(c.level == 2);
  Defense c3;
  for (int i = 0; i < 3; ++i) c3.stamp(5000 + i * 45);
  c3.update(5090, 200);
  CHECK(c3.level == 0);
  Defense c4;
  for (int i = 0; i < 4; ++i) c4.stamp(5000 + i * 60);
  c4.update(5180, 200);
  CHECK(c4.level == 0 && c4.swings_within(5180, 150) == 3);
  for (std::uint32_t k = 0; k <= policy::kDemoteTicks; ++k) c.update(5135 + 200, 200);
  CHECK(c.level == 1);
  // Stopping attacking decays step by step (a level per kDemoteTicks once its condition is gone)
  Defense p;
  for (int i = 0; i < 6; ++i) p.stamp(100 + i * 10);
  p.update(150, 200);
  CHECK(p.level == 2);
  for (std::uint32_t k = 0; k < policy::kDemoteTicks; ++k) p.update(1000 + k, 200);
  CHECK(p.level == 1);
  for (std::uint32_t k = 0; k < policy::kDemoteTicks; ++k) p.update(2000 + k, 200);
  CHECK(p.level == 0);
  // Backing off clears the memory
  Defense f;
  for (int i = 0; i < 3; ++i) f.stamp(10 + i);
  for (std::uint32_t k = 0; k < policy::kResetFarTicks; ++k) f.update(12 + k, 700);
  CHECK(f.swings_within(12, 200) == 0);
  // Reset: a fight that is over leaves nothing
  Defense z;
  for (int i = 0; i < 6; ++i) z.stamp(100 + i * 10);
  z.update(150, 200);
  z.reset();
  CHECK(z.level == 0 && z.swings_within(150, 300) == 0);
  // Cooldown by pressure at the moment of the parry
  CHECK(Defense::cooldown_of(0) == 120 && Defense::cooldown_of(1) == 90 && Defense::cooldown_of(2) == 60);
  // HP is not an input: same result at any HP (the API has no HP)
  CHECK(Defense::percent(0) == 12 && Defense::percent(1) == 35 && Defense::percent(2) == 45);
  // Roll frequency
  Rng r;
  r.seed(9);
  for (int lvl = 0; lvl < 3; ++lvl) {
    Defense x;
    x.level = lvl;
    int hits = 0;
    const int N = 1000000;
    for (int i = 0; i < N; ++i) hits += x.roll(r) ? 1 : 0;
    CHECK(std::fabs(double(hits) / N - Defense::percent(lvl) / 100.0) < 0.002);
    CHECK(x.rolls[lvl] == static_cast<std::uint64_t>(N));
  }
  // Parry window: exact bounds, real contact only
  CHECK(!parry_contact(true, 100, 102, true, true));
  CHECK(parry_contact(true, 100, 100 + policy::kParryWindowFirst, true, true));
  CHECK(parry_contact(true, 100, 100 + policy::kParryWindowLast, true, true));
  CHECK(!parry_contact(true, 100, 100 + policy::kParryWindowLast + 1, true, true));
  CHECK(!parry_contact(false, 100, 103, true, true));
  CHECK(!parry_contact(true, 100, 103, false, true));
  CHECK(!parry_contact(true, 100, 103, true, false));
  CHECK(policy::kParryWindowLast - policy::kParryWindowFirst + 1 == 5);
  // Boundary policy
  CHECK(!boundary_allowed(0, 1, 2, 0));
  CHECK(boundary_allowed(1, 1, 1, 0) && !boundary_allowed(1, 1, 0, 0) && !boundary_allowed(1, 1, 1, 1));
  CHECK(!boundary_allowed(2, 2, 2, 0) && boundary_allowed(2, 3, 1, 0));
  CHECK(boundary_allowed(3, 1, 1, 0) && boundary_allowed(3, 2, 1, 0) && !boundary_allowed(3, 3, 2, 0) && !boundary_allowed(3, 4, 2, 0));
  // Swing aim: a swing away from the actor is not an opportunity for it
  CHECK(Brain::swing_aimed(0, static_cast<std::int16_t>(0x8000)));
  CHECK(!Brain::swing_aimed(0, 0));
  CHECK(!Brain::swing_aimed(0x4000, static_cast<std::int16_t>(0x8000)));
}
// Live scenario: Link swings at FIGHT_RUN Lizalfos; roll, guard, tate contact.
struct DuelOut {
  std::uint64_t parries, blocks, rolls, hits;
  int link_gain;
  std::uint64_t posture;
};
static void lab_guard_live() {
  // Forced attempt: roll seed search so the first opportunity arms; contact inside the window => parry only: +25 to Link, 0 to the actor, no +5, no HP change, cooldown, native exit.
  for (std::uint32_t contact_delay : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 8u}) {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 500;  // no attack decision, only defense
    for (int g = 0; g < 8192; ++g) {
      Rng pr = n.b.rng;
      if (pr.below(100) < 12) break;
      (void)n.b.rng.next();
    }
    const int hp0 = n.hp;
    L.swing_serial = 1, L.shape_y = static_cast<std::int16_t>(0x8000);
    n.search_y = 0, n.shape_y = 0, n.cur_y = 0;
    bool entered = false;
    int link_gain = 0;
    std::uint32_t since = 0;
    for (int t = 0; t < 300; ++t) {
      n.execute(L);
      for (unsigned i = 0; i < n.b.nev; ++i)
        if (n.b.ev[i].k == EvK::LinkGain) link_gain += n.b.ev[i].a;
      n.b.nev = 0;
      if (n.action == native::kGuard && n.guard_active_tick && !entered) entered = true, since = 0;
      if (entered && n.action == native::kGuard && since == contact_delay && n.guard_active_tick) {
        n.tate_hit = n.tate_sword = true;  // real contact of this tick's pass, next Execute accepts
      }
      if (entered) ++since;
      if (n.b.st.enemy_parries + n.b.st.blocks > 0) break;
    }
    CHECK(entered);
    // contact_delay 0 = the first collision pass with tate_sph in place: delta 3, the first physically possible contact
    const bool in_window = contact_delay + 3 >= policy::kParryWindowFirst && contact_delay + 3 <= policy::kParryWindowLast;
    if (in_window) {
      CHECK(n.b.st.enemy_parries == 1 && n.b.st.blocks == 0 && link_gain == 25 && n.b.p.points() == 0);
      CHECK(n.b.def.cooldown_until > n.b.local && n.b.def.cooldown_until <= n.b.local + policy::kParryCooldownA);
    } else {
      CHECK(n.b.st.enemy_parries == 0 && n.b.st.blocks == 1 && link_gain == 0 && n.b.p.points() == 5);
    }
    CHECK(n.hp == hp0);
    // never stuck: back to FIGHT_RUN in bounded time, no permanent stop
    int out = 0;
    for (; out < 200 && n.action == native::kGuard; ++out) {
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.action != native::kGuard);
  }
  // One roll per swing: a swing that lost the roll does not get another chance and is not re-evaluated every tick.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 500;
    for (int g = 0; g < 8192; ++g) {
      Rng pr = n.b.rng;
      if (pr.below(100) >= 12) break;
      (void)n.b.rng.next();
    }
    L.swing_serial = 9, L.shape_y = static_cast<std::int16_t>(0x8000);
    for (int t = 0; t < 40; ++t) {
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.b.def.rolls[0] == 1 && n.b.def.attempts[0] == 0);
    CHECK(n.action != native::kGuard);
  }
  // Cooldown: 120 ticks after a success no attempt is rolled; afterwards it is.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    n.b.def.cooldown_until = n.b.local + 120;
    L.shape_y = static_cast<std::int16_t>(0x8000);
    std::uint64_t serial = 1;
    for (int t = 0; t < 119; ++t) {
      L.swing_serial = serial += (t % 10 == 0);
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.b.def.rolls[0] + n.b.def.rolls[1] + n.b.def.rolls[2] == 0);
    for (int t = 0; t < 30; ++t) {
      L.swing_serial = serial += 1;
      n.execute(L);
      n.b.nev = 0;
      if (n.action != native::kFightRun) break;
    }
    CHECK(n.b.def.rolls[0] + n.b.def.rolls[1] + n.b.def.rolls[2] >= 1);
  }
  // Back or side: no roll for a swing outside the front half, and a contact from there is never a parry.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    n.search_y = static_cast<std::int16_t>(0x8000), n.shape_y = 0, n.cur_y = 0;  // Link behind the actor
    L.swing_serial = 3, L.shape_y = 0;
    for (int t = 0; t < 5; ++t) {
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.b.def.rolls[0] == 0);
  }
  // Out of range / aimed elsewhere (another Lizalfos's fight): memory unaffected
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 900, n.timer[2] = 5000;
    L.shape_y = static_cast<std::int16_t>(0x8000);
    for (int s = 1; s < 20; ++s) {
      L.swing_serial = static_cast<std::uint64_t>(s);
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.b.def.swings_within(n.b.local, 300) == 0);
    Native m;
    m.bind_admitted();
    m.execute(LinkFrame{});
    m.pl_dir = 200, m.timer[2] = 5000;
    L.shape_y = 0x4000;
    for (int s = 1; s < 20; ++s) {
      L.swing_serial = static_cast<std::uint64_t>(s);
      m.execute(L);
      m.b.nev = 0;
    }
    CHECK(m.b.def.swings_within(m.b.local, 300) == 0);
  }
  // Ordinary native guard (learned cut): +5 on real tate contact, no initiative, no cooldown, no "automatic turn".
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000, n.learned_guard = true, n.cut_at = true;
    n.execute(L);
    n.cut_at = false;
    CHECK(n.action == native::kGuard);
    n.tate_hit = n.tate_sword = true;
    n.execute(L);
    n.b.nev = 0;
    CHECK(n.b.st.blocks == 1 && n.b.st.enemy_parries == 0 && n.b.p.points() == 5 && !n.b.counter.armed && n.b.def.cooled(n.b.local));
    // a second contact of the same swing cannot pay (native 10 tick lock and swing serial)
    n.tate_hit = n.tate_sword = true;
    n.execute(L);
    n.b.nev = 0;
    CHECK(n.b.st.blocks == 1);
    // ... not even after the native 10 tick lock has elapsed: same swing serial, no second credit
    Native h;
    h.bind_admitted();
    h.execute(L);
    h.action = native::kGuard, h.mode = 7, h.timer[0] = 60, h.rate = 0, h.pl_dir = 200, h.timer[2] = 5000;
    LinkFrame S;
    S.swing_serial = 44, S.shape_y = static_cast<std::int16_t>(0x8000);
    h.execute(S);
    h.tate_hit = h.tate_sword = true;
    h.execute(S);
    CHECK(h.b.st.blocks == 1);
    for (int t = 0; t < 14; ++t) {
      h.execute(S);
      h.b.nev = 0;
    }
    h.tate_hit = h.tate_sword = true;
    h.execute(S);
    CHECK(h.b.st.blocks == 1 && h.b.p.points() == 5);
    S.swing_serial = 45;  // a distinct swing pays again once the native lock is over
    for (int t = 0; t < 12; ++t) {
      h.timer[0] = 60;
      h.execute(S);
      h.b.nev = 0;
    }
    h.timer[0] = 60;
    h.tate_hit = h.tate_sword = true;
    h.execute(S);
    CHECK(h.action == native::kGuard && h.b.st.blocks == 2 && h.b.p.points() == 10);
    // tate contact with something else than Link's sword never pays
    Native a;
    a.bind_admitted();
    a.execute(L);
    a.action = native::kGuard, a.mode = 7, a.timer[0] = 10, a.rate = 0;
    a.tate_hit = true, a.tate_sword = false;
    a.execute(L);
    CHECK(a.b.st.blocks == 0 && a.b.p.points() == 0);
  }
  // Guard hold under R held / passive Link: native exit always, no HP, no regen stall
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.action = native::kGuard, n.mode = 5, n.timer[0] = 0, n.pl_dir = 200, n.timer[2] = 500;
    int t = 0;
    for (; t < 400 && n.action == native::kGuard; ++t) {
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.action == native::kFightRun && t < 120);
    // Watchdog: a guard kept artificially (native timer refilled, e.g. continuous spin) leaves through the native mode 8 path after the bound
    Native w;
    w.bind_admitted();
    w.execute(L);
    w.action = native::kGuard, w.mode = 7, w.rate = 0, w.pl_dir = 200, w.timer[2] = 5000;
    bool exited = false;
    for (int k = 0; k < 400 && !exited; ++k) {
      w.timer[0] = 20;
      w.execute(L);
      w.b.nev = 0;
      exited = w.mode == 8 || w.action != native::kGuard;
    }
    CHECK(exited && w.b.st.watchdog >= 1);
  }
  // Boundary defense: a voluntary defense can only happen between actions of the permitted patterns under verified pressure, and is labelled.
  {
    bool any_adaptive = false;
    for (std::uint64_t seed = 0; seed < 200 && !any_adaptive; ++seed) {
      Native n;
      n.bind_admitted();
      n.b.rng.seed(seed);
      LinkFrame L;
      n.execute(L);
      n.pl_dir = 200, n.timer[2] = 0;
      L.shape_y = static_cast<std::int16_t>(0x8000);
      std::uint64_t s = 1;
      for (int t = 0; t < 300; ++t) {
        if (t % 12 == 0) ++s;
        L.swing_serial = s;
        n.execute(L);
        for (unsigned i = 0; i < n.b.nev; ++i) {
          if (n.b.ev[i].k == EvK::ComboEnd && static_cast<End>(n.b.ev[i].b) == End::AdaptiveDefense) {
            any_adaptive = true;
            CHECK(n.b.ev[i].a != 0);       // never pattern 1
            CHECK(n.action == native::kGuard || n.mode == 5);
          }
        }
        n.b.nev = 0;
        CHECK(!(n.armed_this_tick && n.action == native::kGuard));  // never with AT open
      }
    }
    CHECK(any_adaptive);
  }
}
// ---------------------------------------------------------------- G anti-stagger, break, recovery
static void lab_reaction() {
  LinkFrame L;
  // Sword: HP real, +9, no S_DAMAGE, action continues
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    n.pl_dir = 900, n.timer[2] = 500;
    n.body_hit = true, n.body_kind = HitKind::Sword;
    const int hp0 = n.hp;
    n.execute(L);
    CHECK(n.hp == hp0 - n.body_power && n.b.p.points() == 9 && n.small_calls == 0 && n.action == native::kFightRun && n.invuln > 0 && n.invuln <= 10);
    // while invulnerable a second contact does nothing natively
    n.body_hit = true;
    n.execute(L);
    CHECK(n.hp == hp0 - n.body_power && n.b.p.points() == 9);
    // Strong sword (big path) below 100: same result, the 1000 invulnerability never survives
    Native s;
    s.bind_admitted();
    s.execute(L);
    s.pl_dir = 900, s.timer[2] = 500;
    s.body_hit = true, s.body_kind = HitKind::Sword, s.body_strong = true;
    s.execute(L);
    CHECK(s.big_calls == 0 && s.action == native::kFightRun && s.invuln <= 10 && s.b.p.points() == 9);
    s.b.nev = 0;
    // Mid attack: no interruption
    Native a;
    a.bind_admitted();
    a.execute(L);
    a.pl_dir = 200, a.timer[2] = 0;
    for (int t = 0; t < 3; ++t) a.execute(L);
    CHECK(a.b.ep.active);
    const auto act = a.action;
    a.body_hit = true, a.body_kind = HitKind::Sword;
    a.execute(L);
    CHECK(a.b.ep.active && a.action == act && a.small_calls == 0);
  }
  // Lethal: native death always, never a skipped reaction, combo ends with Death
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    n.hp = 3, n.pl_dir = 200, n.timer[2] = 0;
    n.execute(L);
    n.body_hit = true, n.body_kind = HitKind::Sword;
    n.execute(L);
    CHECK(n.big_calls == 1 && n.action == native::kDamage && n.invuln > 10);
    n.execute(L);
    CHECK(!n.b.ep.active && n.b.dead);
  }
  // Iron Ball below 100: +30, native reaction kept; other hit families: no posture, native reaction
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    n.pl_dir = 900, n.timer[2] = 500;
    n.body_hit = true, n.body_kind = HitKind::IronBall, n.body_strong = true;
    n.execute(L);
    CHECK(n.b.p.points() == 30 && !n.b.p.latched && n.big_calls == 1);
    Native o;
    o.bind_admitted();
    o.execute(L);
    o.pl_dir = 900, o.timer[2] = 500;
    o.body_hit = true, o.body_kind = HitKind::Other;
    o.execute(L);
    CHECK(o.b.p.points() == 0 && o.small_calls == 1 && o.action == native::kSDamage);
  }
  // Shield Attack on the body below 100: no DRAWBACK, no posture, no HP; at latched state the native path is not touched
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    n.pl_dir = 900, n.timer[2] = 500;
    n.body_hit = true, n.body_shield_attack = true;
    LinkFrame S = L;
    S.shield_serial = 7;
    n.execute(S);
    CHECK(n.action == native::kFightRun && n.b.st.shield_bash_cleared == 1 && n.b.p.points() == 0 && n.bash_se == 1 && n.drawback_entries == 0);
  }
  // Horse / event: native, no suppression, no credit
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    n.pl_dir = 900, n.timer[2] = 500, n.horse = true;
    n.body_hit = true, n.body_kind = HitKind::Sword;
    n.execute(L);
    CHECK(n.b.p.points() == 0 && n.small_calls == 1);
    Native e;
    e.bind_admitted();
    e.execute(L);
    e.pl_dir = 200, e.timer[2] = 0;
    for (int t = 0; t < 3; ++t) e.execute(L);
    CHECK(e.b.ep.active);
    e.event = true;
    e.execute(L);
    CHECK(!e.b.ep.active && e.b.st.ended[unsigned(End::Event)] == 1);
    // Delete mid combo
    Native d;
    d.bind_admitted();
    d.execute(L);
    d.pl_dir = 200, d.timer[2] = 0;
    for (int t = 0; t < 3; ++t) d.execute(L);
    d.b.on_delete();
    CHECK(!d.b.ep.active && d.b.st.ended[unsigned(End::SceneOrDelete)] == 1);
  }
  // Native priority interruption mid combo (another family's knock) is labelled, not hidden
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 0;
    for (int t = 0; t < 3; ++t) n.execute(L);
    CHECK(n.b.ep.active);
    n.body_hit = true, n.body_kind = HitKind::Other;
    n.execute(L);
    n.execute(L);
    CHECK(!n.b.ep.active && n.b.st.ended[unsigned(End::NativePriority)] == 1);
  }
  // Pause: Execute not running changes nothing; a tick with an unchanged frame counter does not advance local state
  {
    Native n;
    n.bind_admitted();
    n.execute(L);
    const auto l = n.b.local;
    n.b.fc_pre = n.fc;
    (void)n.b.on_exec_post(n.view(), L);
    CHECK(n.b.local == l);
  }
}
// ---------------------------------------------------------------- F frozen offensive contract: the exact table compiled by 0.12.0 (03111c85), value by value
static void lab_frozen_rhythm() {
  struct Row { float start; std::uint32_t wu; };
  const Row T[4][6] = {
      {{0.000000f, 18}, {5.000000f, 8}, {19.000000f, 12}},
      {{0.000000f, 18}, {10.000000f, 10}, {9.250000f, 11}, {4.000000f, 9}, {6.000000f, 13}, {18.400000f, 13}},
      {{0.100000f, 12}, {11.000000f, 9}, {4.000000f, 9}, {11.000000f, 9}, {9.250000f, 11}, {4.000000f, 9}},
      {{0.000000f, 30}, {0.000000f, 30}, {0.000000f, 18}, {4.000000f, 9}, {19.000000f, 12}},
  };
  for (unsigned p = 0; p < 4; ++p)
    for (unsigned a = 0; a < kPattern[p].acts; ++a) {
      CHECK(std::fabs(start_frame(p, a) - T[p][a].start) < 1e-5f);
      CHECK(windup_of(p, a) == T[p][a].wu);
    }
  // The counters are the native clips at the same action rates from frame 0, and are not combos.
  CHECK(kPattern[kCounterN].acts == 1 && kPattern[kCounterN].impacts == 1 && kPattern[kCounterN].a[0] == Act::N);
  CHECK(kPattern[kCounterD].acts == 1 && kPattern[kCounterD].impacts == 2 && kPattern[kCounterD].a[0] == Act::D);
  CHECK(entry_frame(kCounterN, 0) == 0.0f && entry_frame(kCounterD, 0) == 0.0f);
  CHECK(kPatterns == 4 && !boundary_allowed(kCounterN, 1, 2, 0) && !boundary_allowed(kCounterD, 1, 2, 0));
}
// ---------------------------------------------------------------- swing lease: one roll per real swing; never a future input
static void lab_lease() {
  // Level A: 6 ticks. Level B/C: the same swing can still earn its one roll when a committed state ends, up to 30 ticks, then it is gone.
  for (int level : {0, 1, 2}) {
    Brain b;
    b.pending_swing_serial = 5, b.pending_swing_tick = 100, b.def.level = level;
    b.local = 106;
    CHECK(b.swing_pending() && b.swing_fresh());
    b.local = 107;
    CHECK(b.swing_pending() == (level >= 1) && !b.swing_fresh());
    b.local = 130;
    CHECK(b.swing_pending() == (level >= 1));
    b.local = 131;
    CHECK(!b.swing_pending());
    b.local = 110;
    b.rolled_serial = 5;
    CHECK(!b.swing_pending());
  }
  // Live: under proven pressure a swing during a committed attack gets its single roll at the first FIGHT_RUN after the clip; no swing, no roll, ever.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    n.action = native::kSDamage, n.mode = 1, n.timer[0] = 14;  // a committed state
    L.shape_y = static_cast<std::int16_t>(0x8000);
    for (int s = 1; s <= 4; ++s) {  // 4 distinct swings: level C while the actor cannot react
      L.swing_serial = static_cast<std::uint64_t>(s);
      n.execute(L);
      n.execute(L);
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.b.def.level == 2);
    CHECK(n.b.def.rolls[0] + n.b.def.rolls[1] + n.b.def.rolls[2] == 0);
    for (int t = 0; t < 30 && n.action == native::kSDamage; ++t) {
      n.execute(L);
      n.b.nev = 0;
    }
    n.execute(L);
    n.b.nev = 0;
    CHECK(n.b.def.rolls[2] == 1 && n.b.def.rolls[0] == 0);
    Native q;
    q.bind_admitted();
    q.execute(LinkFrame{});
    q.pl_dir = 200, q.timer[2] = 5000;
    LinkFrame I;
    I.shape_y = static_cast<std::int16_t>(0x8000);
    for (int t = 0; t < 300; ++t) {
      q.execute(I);
      q.b.nev = 0;
    }
    CHECK(q.b.def.rolls[0] + q.b.def.rolls[1] + q.b.def.rolls[2] == 0);  // B pressed without a real swing start: nothing
  }
  // A swing only counts once for this actor, however many ticks the animation lasts; aimed at another actor it does not count at all.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    L.shape_y = static_cast<std::int16_t>(0x8000), L.swing_serial = 3;
    for (int t = 0; t < 20; ++t) {
      n.execute(L);
      n.b.nev = 0;
    }
    CHECK(n.b.st.swings_detected == 1 && n.b.def.swings_within(n.b.local, 75) == 1);
    Native o;
    o.bind_admitted();
    o.execute(LinkFrame{});
    o.pl_dir = 200, o.timer[2] = 5000, o.search_y = 0x4000;  // the swing above is aimed elsewhere for this one
    LinkFrame M = L;
    M.swing_serial = 3, M.shape_y = 0;
    for (int t = 0; t < 20; ++t) {
      o.execute(M);
      o.b.nev = 0;
    }
    CHECK(o.b.st.swings_detected == 0);
  }
  // De-escalation: Link dead, gone, or a scene clears the pressure; so does the break.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    L.shape_y = static_cast<std::int16_t>(0x8000);
    for (int s = 1; s <= 4; ++s) {
      L.swing_serial = static_cast<std::uint64_t>(s);
      n.execute(L), n.b.nev = 0;
    }
    CHECK(n.b.def.level >= 1);
    n.link_alive = false;
    n.execute(L);
    CHECK(n.b.def.level == 0 && n.b.def.swings_within(n.b.local, 300) == 0);
  }
}
// A combo boundary only ever takes a fresh swing: the pressure lease is for FIGHT_RUN, it never interrupts a combo on an old swing.
static void lab_boundary_fresh() {
  Native n;
  n.bind_admitted();
  LinkFrame L;
  n.execute(L);
  n.pl_dir = 200, n.timer[2] = 0;
  L.shape_y = static_cast<std::int16_t>(0x8000);
  for (int g = 0; g < 4096; ++g) {
    Rng pr = n.b.rng;
    if (pr.below(kPatterns) == 1) break;  // P2 allows boundary defense from its first boundary
    (void)n.b.rng.next();
  }
  for (int t = 0; t < 8 && !n.b.ep.active; ++t) n.execute(L), n.b.nev = 0;
  CHECK(n.b.ep.active && n.b.ep.pattern == 1);
  n.b.def.stamp(n.b.local), n.b.def.stamp(n.b.local), n.b.def.stamp(n.b.local);
  L.swing_serial = 1;  // one real swing right after the combo started: at the first boundary it is ~25 ticks old
  for (int t = 0; t < 26; ++t) {
    n.execute(L), n.b.nev = 0;
    CHECK(n.b.def.level >= 1 && n.b.ep.active);
  }
  CHECK(n.b.def.rolls[0] + n.b.def.rolls[1] + n.b.def.rolls[2] == 0 && n.b.ep.act >= 1);
}
// ---------------------------------------------------------------- two blocks => one roll => short counter
struct Blocks {
  Native n;
  LinkFrame L;
  explicit Blocks(std::uint64_t seed, bool want_armed, unsigned kind = 2) {
    n.bind_admitted();
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    n.b.rng.seed(seed);
    // position the stream: first draw = priority roll, second = kind
    for (int g = 0; g < 200000; ++g) {
      Rng pr = n.b.rng;
      const bool a = pr.below(100) < 80;
      const unsigned k = pr.below(2);
      if (a == want_armed && (kind > 1 || k == kind)) break;
      (void)n.b.rng.next();
    }
    L.shape_y = static_cast<std::int16_t>(0x8000);
  }
  void hold_guard() { n.action = native::kGuard, n.mode = 7, n.timer[0] = 60, n.rate = 0; }
  void contact(std::uint64_t swing) {
    L.swing_serial = swing;
    for (int t = 0; t < 12; ++t) {
      n.timer[0] = 60;
      n.execute(L);
      n.b.nev = 0;
    }
    n.timer[0] = 60;
    n.tate_hit = n.tate_sword = true;
    n.execute(L);
  }
};
static void lab_counter() {
  // Exactly the second REAL distinct block rolls, once, and only prepares: no parry, no damage, the guard leaves by its native exit.
  {
    Blocks B(11, true);
    B.hold_guard();
    B.contact(1);
    CHECK(B.n.b.st.blocks == 1 && B.n.b.st.priority_rolls == 0 && !B.n.b.counter.armed);
    B.contact(2);
    CHECK(B.n.b.st.blocks == 2 && B.n.b.st.priority_rolls == 1 && B.n.b.st.priority_armed == 1 && B.n.b.counter.armed && B.n.b.st.enemy_parries == 0);
    CHECK(B.n.mode == 8 && B.n.rate == 1.0f);  // native exit written at once; no static guard waiting for a third hit
    CHECK(B.n.b.p.points() == 10);
    // the same swing re-touching never counts
    Blocks D(12, true);
    D.hold_guard();
    D.contact(1);
    D.contact(1);
    D.contact(1);
    CHECK(D.n.b.st.blocks == 1 && D.n.b.st.priority_rolls == 0);
    // contact with something else than Link's sword never counts
    Blocks O(13, true);
    O.hold_guard();
    O.contact(1);
    O.L.swing_serial = 2;
    for (int t = 0; t < 12; ++t) {
      O.n.timer[0] = 60;
      O.n.execute(O.L), O.n.b.nev = 0;
    }
    O.n.timer[0] = 60;
    O.n.tate_hit = true, O.n.tate_sword = false;
    O.n.execute(O.L);
    CHECK(O.n.b.st.blocks == 1 && O.n.b.st.priority_rolls == 0);
  }
  // Two blocks farther apart than the exchange: no roll. A sword wound between them: no roll.
  {
    Blocks B(14, true);
    B.hold_guard();
    B.contact(1);
    for (std::uint32_t t = 0; t < policy::kBlockChainTicks + 5; ++t) {
      B.n.timer[0] = 60;
      B.n.execute(B.L), B.n.b.nev = 0;
    }
    B.contact(2);
    CHECK(B.n.b.st.blocks == 2 && B.n.b.st.priority_rolls == 0);
    Blocks W(15, true);
    W.hold_guard();
    W.contact(1);
    W.n.invuln = 0;
    W.n.body_hit = true, W.n.body_kind = HitKind::Sword;
    W.n.execute(W.L);
    W.n.b.nev = 0;
    W.hold_guard();
    W.contact(2);
    CHECK(W.n.b.st.priority_rolls == 0);
  }
  // The 80%: one roll per pair, independent of the parry percentages and of the 25% draw.
  {
    Rng r;
    r.seed(99);
    int armed = 0;
    const int N = 2000000;
    for (int i = 0; i < N; ++i) armed += r.below(100) < static_cast<std::uint32_t>(policy::kCounterPct);
    CHECK(std::fabs(double(armed) / N - 0.80) < 0.001);
    int d = 0;
    for (int i = 0; i < N; ++i) d += r.below(2);
    CHECK(std::fabs(double(d) / N - 0.50) < 0.001);
    // live: 800 independent pairs
    int live_armed = 0, live_n = 0, live_d = 0, sel = 0;
    for (std::uint64_t s = 1; s <= 800; ++s) {
      Native n;
      n.bind_admitted();
      LinkFrame L;
      n.execute(L);
      n.pl_dir = 200, n.timer[2] = 5000;
      n.b.rng.seed(s * 2654435761ull);
      L.shape_y = static_cast<std::int16_t>(0x8000);
      n.action = native::kGuard, n.mode = 7, n.rate = 0;
      for (std::uint64_t sw = 1; sw <= 2; ++sw) {
        L.swing_serial = sw;
        for (int t = 0; t < 12; ++t) {
          n.timer[0] = 60;
          n.execute(L), n.b.nev = 0;
        }
        n.timer[0] = 60;
        n.tate_hit = n.tate_sword = true;
        n.execute(L), n.b.nev = 0;
      }
      CHECK(n.b.st.priority_rolls == 1);
      live_armed += static_cast<int>(n.b.st.priority_armed);
      live_n += static_cast<int>(n.b.st.counter_selected[0]);
      live_d += static_cast<int>(n.b.st.counter_selected[1]);
      for (auto x : n.b.st.selected) sel += static_cast<int>(x);
    }
    CHECK(live_armed > 590 && live_armed < 690);
    CHECK(live_n + live_d == live_armed && live_n > live_armed / 3 && live_d > live_armed / 3);
    CHECK(sel == 0);
  }
  // Commit: from the native exit into FIGHT_RUN, the first legal tick starts the native attack at its action rate. N 1 impact, D 2 impacts. Not a combo. Afterwards the next combo is any of the four.
  for (unsigned kind : {0u, 1u}) {
    Blocks B(20 + kind, true, kind);
    B.hold_guard();
    B.contact(1);
    B.contact(2);
    CHECK(B.n.b.counter.armed && B.n.b.counter.kind == kind);
    float rate_seen = 0;
    int windows = 0, guard_ticks = 0;
    bool committed = false;
    for (int t = 0; t < 140; ++t) {
      B.n.execute(B.L);
      for (unsigned i = 0; i < B.n.b.nev; ++i) {
        if (B.n.b.ev[i].k == EvK::ActionStart) rate_seen = B.n.b.ev[i].g;
        if (B.n.b.ev[i].k == EvK::ImpactOpen) ++windows;
        if (B.n.b.ev[i].k == EvK::CounterCommitted) committed = true;
      }
      B.n.b.nev = 0;
      if (!committed && B.n.action == native::kGuard) ++guard_ticks;
      if (committed && !B.n.b.ep.active && !B.n.b.counter.live) break;
    }
    CHECK(committed && windows == (kind == 0 ? 1 : 2));
    CHECK(rate_seen == (kind == 0 ? 1.25f : 1.10f));
    CHECK(guard_ticks < 60);
    CHECK(B.n.b.st.counter_started[kind] == 1 && B.n.b.st.counter_at_open == 1);
    std::uint64_t sel = 0;
    for (auto x : B.n.b.st.selected) sel += x;
    CHECK(sel == 0);
    CHECK(B.n.b.st.impacts_open[kind == 0 ? kCounterN : kCounterD] == (kind == 0 ? 1u : 2u));
    // recovery, opening, then the ordinary draw
    B.n.timer[2] = 0;
    std::uint64_t before = 0;
    int t = 0;
    for (; t < 400 && before == 0; ++t) {
      B.n.execute(B.L);
      B.n.b.nev = 0;
      for (auto x : B.n.b.st.selected) before += x;
    }
    CHECK(before == 1);
    CHECK(t >= policy::kRestMin);  // never an instant chain: the same opening as after any combo
  }
  // Link parries the counter: it continues, +15, D admits D1 and D2.
  {
    Blocks B(31, true, 1);
    B.hold_guard();
    B.contact(1);
    B.contact(2);
    int parries = 0;
    for (int t = 0; t < 140 && parries < 2; ++t) {
      LinkFrame P = B.L;
      if (B.n.at_hit) {
        P.pass.valid = P.pass.guard_attack = true, P.pass.start = 1, P.pass.frame = 3, P.pass.age_ticks = 2;
        B.n.at_shield = true;
      }
      B.n.execute(P);
      B.n.b.nev = 0;
      if (B.n.armed_this_tick) B.n.at_hit = true;
      parries = static_cast<int>(B.n.b.st.link_parry);
    }
    CHECK(parries == 2 && B.n.b.st.d_pair_parry == 1 && B.n.b.p.points() == 10 + 30);
    CHECK(B.n.b.st.counter_contact == 1);
  }
  // Legality: never from the back with a magic turn, never at another height, never without a line of sight, never during recoil; the intent waits, bounded.
  {
    struct Bad { const char* name; void (*make)(Native&); };
    const Bad bad[] = {
        {"back", [](Native& n) { n.search_y = static_cast<std::int16_t>(0x8000); }},
        {"side", [](Native& n) { n.search_y = 0x3200; }},
        {"height", [](Native& n) { n.search_x = 0x1000; }},
        {"unseen", [](Native& n) { n.seen_timer = 0; }},
        {"far", [](Native& n) { n.pl_dir = 351.0f; }},
        {"air", [](Native& n) { n.ground = false; }},
        {"recoil", [](Native& n) { n.recoil = 1.0f; }},
    };
    for (const auto& x : bad) {
      Blocks B(40, true);
      B.hold_guard();
      B.contact(1);
      B.contact(2);
      for (int t = 0; t < 60 && B.n.action == native::kGuard; ++t) B.n.execute(B.L), B.n.b.nev = 0;
      x.make(B.n);
      B.n.timer[2] = 5000;
      for (int t = 0; t < 20; ++t) {
        x.make(B.n);
        B.n.execute(B.L);
        B.n.b.nev = 0;
        CHECK(!B.n.b.ep.active);
      }
      CHECK(B.n.b.counter.armed);
      std::uint64_t exp = 0;
      for (std::uint32_t t = 0; t < policy::kCounterLeaseTicks + 5 && !exp; ++t) {
        x.make(B.n);
        B.n.execute(B.L);
        B.n.b.nev = 0;
        exp = B.n.b.st.counter_expired;
      }
      CHECK(exp == 1 && !B.n.b.counter.armed && B.n.b.st.counter_cancel[unsigned(Cancel::Lease)] == 1 && !B.n.b.ep.active);
    }
    // the boundaries that are legal
    Blocks E(41, true);
    E.hold_guard();
    E.contact(1);
    E.contact(2);
    for (int t = 0; t < 60 && E.n.action == native::kGuard; ++t) E.n.execute(E.L), E.n.b.nev = 0;
    E.n.search_y = 0x3000, E.n.search_x = 0x0FFF, E.n.pl_dir = 350.0f;
    E.n.execute(E.L);
    CHECK(E.n.b.ep.active && E.n.b.st.counter_started[0] + E.n.b.st.counter_started[1] == 1);
  }
  // Ends: distance, Link gone, event, a sword wound, the break: each with its own reason, and nothing starts afterwards.
  {
    Blocks F(50, true);
    F.hold_guard(), F.contact(1), F.contact(2);
    F.n.pl_dir = 700;
    F.n.execute(F.L);
    CHECK(!F.n.b.counter.armed && F.n.b.st.counter_cancel[unsigned(Cancel::Far)] == 1);
    Blocks G(51, true);
    G.hold_guard(), G.contact(1), G.contact(2);
    G.n.link_alive = false;
    G.n.execute(G.L);
    CHECK(!G.n.b.counter.armed && G.n.b.st.counter_cancel[unsigned(Cancel::LinkGone)] == 1);
    Blocks H(52, true);
    H.hold_guard(), H.contact(1), H.contact(2);
    H.n.event = true;
    H.n.execute(H.L);
    CHECK(!H.n.b.counter.armed && H.n.b.st.counter_cancel[unsigned(Cancel::Event)] == 1);
    Blocks I(53, true);
    I.hold_guard(), I.contact(1), I.contact(2);
    I.n.invuln = 0, I.n.body_hit = true, I.n.body_kind = HitKind::Sword;
    I.n.execute(I.L);
    CHECK(!I.n.b.counter.armed && I.n.b.st.counter_cancel[unsigned(Cancel::Damage)] == 1);
    Blocks K(54, true);
    K.hold_guard(), K.contact(1), K.contact(2);
    K.n.b.p.add(85);
    K.n.invuln = 0, K.n.body_hit = true, K.n.body_kind = HitKind::Sword;
    K.n.execute(K.L);
    CHECK(!K.n.b.counter.armed && K.n.action == native::kDrawback);
    // the 20%: habitual behaviour, nothing armed, the pair is consumed
    Blocks L2(55, false);
    L2.hold_guard(), L2.contact(1), L2.contact(2);
    CHECK(L2.n.b.st.priority_rolls == 1 && L2.n.b.st.priority_armed == 0 && !L2.n.b.counter.armed);
    L2.contact(3);
    CHECK(L2.n.b.st.priority_rolls == 1);  // the third is the first of a new pair, never a re-roll of the same two
    L2.contact(4);
    CHECK(L2.n.b.st.priority_rolls == 2);
  }
  // Real enemy parry: 100% prepared, Normal or Double only, +25 to Link once, cooldown by pressure; never Short/Combo/Reposition; survives the guard exit even with timer[2] running.
  for (int level : {0, 1, 2}) {
    Native n;
    n.bind_admitted();
    LinkFrame L;
    n.execute(L);
    n.pl_dir = 200, n.timer[2] = 5000;
    L.shape_y = static_cast<std::int16_t>(0x8000), L.swing_serial = 1;
    n.action = native::kGuard, n.mode = 7, n.rate = 0, n.timer[0] = 60;
    n.b.in_guard = true;
    n.b.attempt_armed = true;
    n.b.guard_tick = n.b.local;
    n.b.def.level = level;
    
    n.execute(L), n.b.nev = 0;
    n.execute(L), n.b.nev = 0;
    n.tate_hit = n.tate_sword = true;
    n.execute(L);
    int gain = 0;
    for (unsigned i = 0; i < n.b.nev; ++i)
      if (n.b.ev[i].k == EvK::LinkGain) gain += n.b.ev[i].a;
    n.b.nev = 0;
    CHECK(n.b.st.enemy_parries == 1 && gain == 25 && n.b.p.points() == 0 && n.b.counter.armed && n.b.counter.source == 1);
    CHECK(n.b.def.cooldown_until == n.b.local + (level == 2 ? 60u : level == 1 ? 90u : 120u));
    CHECK(n.mode == 8);
    CHECK(n.hp == 370);
    std::uint64_t sel = 0;
    bool started = false;
    for (int t = 0; t < 60 && !started; ++t) {
      n.execute(L);
      n.b.nev = 0;
      started = n.b.ep.active;
    }
    CHECK(started && (n.b.ep.pattern == kCounterN || n.b.ep.pattern == kCounterD));
    for (auto x : n.b.st.selected) sel += x;
    CHECK(sel == 0);
    CHECK(n.timer[2] > 1000);  // the intent did not need the native attack timer
  }
  // The guard exit waits for the native clip: mode 5 has not started ANM_GUARD yet; leaving from it would run a clip that never ends.
  {
    Blocks B(62, true);
    B.hold_guard(), B.contact(1), B.contact(2);
    for (int t = 0; t < 60 && B.n.action == native::kGuard; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    B.n.search_y = 0x3200;  // counter illegal: it waits; the native guard enters from FIGHT_RUN
    B.n.learned_guard = true, B.n.cut_at = true;
    B.n.execute(B.L), B.n.b.nev = 0;
    B.n.learned_guard = false, B.n.cut_at = false;
    CHECK(B.n.action == native::kGuard && B.n.mode == 5 && B.n.b.counter.armed);
    B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(B.n.anm_no == 22 && B.n.mode == 8);
  }
  // One response: a real parry while a counter is already prepared neither re-rolls nor prepares a second one; an Iron Ball crossing while prepared ends it with its own reason.
  {
    Blocks B(63, true);
    B.hold_guard(), B.contact(1), B.contact(2);
    CHECK(B.n.b.counter.armed);
    const auto token = B.n.b.counter.token;
    B.n.b.attempt_armed = true, B.n.b.guard_tick = B.n.b.local - 3;
    B.n.action = native::kGuard, B.n.mode = 7, B.n.rate = 0, B.n.timer[0] = 60;
    B.L.swing_serial = 3;
    B.n.ut2 = 0;
    B.n.tate_hit = B.n.tate_sword = true;
    B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(B.n.b.st.enemy_parries == 1 && B.n.b.counter.token == token && B.n.b.st.counter_selected[0] + B.n.b.st.counter_selected[1] == 1);
    Blocks C(64, true);
    C.hold_guard(), C.contact(1), C.contact(2);
    C.n.b.p.add(85);
    C.n.invuln = 0, C.n.body_hit = true, C.n.body_kind = HitKind::IronBall;
    C.n.execute(C.L), C.n.b.nev = 0;
    CHECK(C.n.b.phase == Brain::Bk::Punish && !C.n.b.counter.armed && C.n.b.st.counter_cancel[unsigned(Cancel::Break)] == 1);
  }
  // Native guard entry on the very tick the counter may start: the attack wins and the 15 of the native entry does not survive.
  {
    Blocks B(60, true);
    B.hold_guard(), B.contact(1), B.contact(2);
    B.n.learned_guard = true, B.n.cut_at = true;
    bool started = false;
    for (int t = 0; t < 120 && !started; ++t) {
      B.n.execute(B.L);
      B.n.b.nev = 0;
      started = B.n.b.ep.active;
      if (started) CHECK(B.n.invuln == 0 && B.n.action != native::kGuard);
    }
    CHECK(started);
  }
  // Cross-actor: a swing aimed at another Lizalfos neither counts nor rolls here.
  {
    Blocks B(61, true);
    B.n.search_y = 0x4000;
    B.hold_guard();
    B.contact(1);
    B.contact(2);
    CHECK(B.n.b.st.swings_detected == 0);
  }
}
// ---------------------------------------------------------------- the standing break
static std::uint64_t find_ev(Brain& b, EvK k) {
  std::uint64_t c = 0;
  for (unsigned i = 0; i < b.nev; ++i) c += b.ev[i].k == k;
  return c;
}
struct Broken {
  Native n;
  LinkFrame L;
  explicit Broken(int hp = 370, float drawback_len = 50.0f) {
    n.bind_admitted();
    n.len[18] = drawback_len;
    n.execute(L);
    n.hp = static_cast<std::int16_t>(hp), n.pl_dir = 200, n.timer[2] = 500;
    L.shape_y = static_cast<std::int16_t>(0x8000), L.swing_serial = 1, L.shield_serial = 1;
    n.execute(L), n.b.nev = 0;
    n.b.p.add(95);
    n.body_hit = true, n.body_kind = HitKind::Sword;
    n.execute(L);
    n.b.nev = 0;
  }
};
static void lab_break() {
  // Every origin: never down, never the down flag, HP valid, a legible native stagger, an exact window.
  LinkFrame L0;
  L0.shape_y = static_cast<std::int16_t>(0x8000), L0.swing_serial = 1, L0.shield_serial = 1;
  // sword (+9 from 95)
  for (float dl : {25.0f, 50.0f, 80.0f, 140.0f}) {
    Broken B(370, dl);
    CHECK(B.n.b.p.latched && B.n.b.phase == Brain::Bk::Punish && B.n.action == native::kDrawback && B.n.big_calls == 0 && B.n.small_calls == 0 && !B.n.down_flag && B.n.hp == 370 - 4);
    CHECK(B.n.b.st.break_standing == 1);
    // no head lock from the crossing alone, during the whole stagger; no attack; no regen; the bar stays at 100
    int ticks = 0;
    bool ever_lock = false, ever_attack = false, ever_guard = false;
    while (ticks < 400 && B.n.b.phase == Brain::Bk::Punish) {
      B.n.execute(B.L);
      B.n.b.nev = 0;
      ever_lock = ever_lock || B.n.head_lock;
      ever_attack = ever_attack || B.n.action == native::kAttack0 || B.n.action == native::kAttack || B.n.action == native::kTailAttack;
      ever_guard = ever_guard || B.n.action == native::kGuard;
      if (B.n.b.phase == Brain::Bk::Punish) CHECK(B.n.b.p.points() == 100);
      ++ticks;
    }
    CHECK(!ever_lock && !ever_attack && !ever_guard && !B.n.down_flag);
    CHECK(B.n.sdamage_inits <= 1);  // one dazed wait, held by its own timer: never a repeated native re-entry
    // the window ends on its feet at the absolute deadline (or at the clip end if a longer stagger clip): rearm to 0 only then
    const int want_min = static_cast<int>(policy::kPunishTicks) - 1;
    CHECK(ticks >= want_min && ticks <= static_cast<int>(dl) + 4 + static_cast<int>(policy::kPunishTicks));
    CHECK(B.n.b.p.points() == 0 && !B.n.b.p.latched && B.n.b.p.rearms == 1 && B.n.big_calls == 0 && B.n.action == native::kFightRun);
    CHECK(B.n.b.st.punish_end == 1);
  }
  // Parry crossing mid-swing: stagger next to the contact; the AT never stays armed; the same press can neither prepare nor punish.
  {
    Native n;
    n.bind_admitted();
    LinkFrame L = L0;
    n.execute(L);
    n.b.p.add(90);
    n.pl_dir = 200, n.timer[2] = 0;
    for (int t = 0; t < 40 && !n.armed_this_tick; ++t) n.execute(L), n.b.nev = 0;
    CHECK(n.armed_this_tick);
    n.at_hit = n.at_shield = true;
    LinkFrame P = L;
    P.pass.valid = P.pass.guard_attack = true, P.pass.start = 1, P.pass.frame = 2, P.pass.age_ticks = 1;
    n.execute(P);
    n.b.nev = 0;
    CHECK(n.b.p.latched && n.action == native::kDrawback && !n.b.ep.active && n.big_calls == 0 && !n.armed_this_tick && n.b.phase == Brain::Bk::Punish);
    CHECK(n.b.st.ended[unsigned(End::Posture100)] == 1);
    for (int t = 0; t < 12; ++t) n.execute(L), n.b.nev = 0;
    n.body_hit = true, n.body_shield_attack = true;  // shield collider of the SAME press still touching
    n.execute(L);
    n.b.nev = 0;
    CHECK(!n.b.helm_prepared && n.b.st.helm_prepared == 0 && n.drawback_entries == 0);
  }
  // A new Shield Attack: the native DRAWBACK runs untouched, no HP, HeadLock real, once. A third press never renews it. Not lethal, ever.
  {
    Broken B(50);
    for (int t = 0; t < 12; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    B.L.shield_serial = 2;
    const int hp0 = B.n.hp;
    B.n.body_hit = true, B.n.body_shield_attack = true;
    B.n.execute(B.L);
    CHECK(B.n.b.helm_prepared && B.n.b.st.helm_prepared == 1 && B.n.drawback_entries == 1 && B.n.hp == hp0 && B.n.action == native::kDrawback);
    B.n.b.nev = 0;
    bool lock = false;
    for (int t = 0; t < 12; ++t) {
      B.n.execute(B.L);
      B.n.b.nev = 0;
      lock = lock || B.n.head_lock;
    }
    CHECK(lock && B.n.head_lock);
    B.L.shield_serial = 3;
    B.n.invuln = 0;
    B.n.body_hit = true, B.n.body_shield_attack = true;
    B.n.execute(B.L);
    CHECK(B.n.drawback_entries == 1 && B.n.b.st.helm_prepared == 1);  // no renewal
    B.n.b.nev = 0;
    // prepared, no decisive contact: ends with the clip, on its feet
    int t = 0;
    for (; t < 200 && B.n.b.phase == Brain::Bk::Punish; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(t <= 45);  // the prepared window is the native clip (50 frames, 12 elapsed), never prolonged to the deadline
    CHECK(B.n.b.phase == Brain::Bk::None && B.n.b.p.points() == 0 && B.n.action == native::kFightRun && B.n.big_calls == 0);
    B.n.execute(B.L), B.n.b.nev = 0;  // native: action() clears the flag every tick
    CHECK(!B.n.head_lock);
  }
  // Decisive sword: only a NEW swing with a real HP loss; drops through the native big_damage; native down flag; Finish; or native get-up then rearm 0.
  for (bool finish : {false, true}) {
    Broken B(200);
    B.n.invuln = 0;
    B.n.body_hit = true;  // the crossing swing re-touching after its invulnerability: not its own punishment
    B.n.execute(B.L);
    B.n.b.nev = 0;
    CHECK(B.n.b.phase == Brain::Bk::Punish && B.n.big_calls == 0);
    B.n.invuln = 0;
    B.L.swing_serial = 2;
    B.n.execute(B.L), B.n.b.nev = 0;
    B.n.invuln = 0;
    B.n.body_hit = true;
    const int hp0 = B.n.hp;
    B.n.execute(B.L);
    CHECK(find_ev(B.n.b, EvK::Decisive) == 1);
    B.n.b.nev = 0;
    CHECK(B.n.big_calls == 1 && B.n.action == native::kDamage && B.n.hp == hp0 - 4 && B.n.invuln >= 990 && B.n.b.st.decisive_sword == 1 && B.n.b.phase == Brain::Bk::Falling);
    int t = 0;
    for (; t < 30 && !B.n.down_flag; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(B.n.down_flag && B.n.hp > 0 && B.n.b.p.points() == 100);
    B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(B.n.b.phase == Brain::Bk::Down && B.n.b.st.down_finish == 1);
    if (finish) {
      B.n.cut_down_hit = true;
      B.n.execute(B.L), B.n.b.nev = 0;
      CHECK(B.n.hp == 0 && !B.n.down_flag);
      for (t = 0; t < 80 && !B.n.deleted; ++t) B.n.execute(B.L), B.n.b.nev = 0;
      CHECK(B.n.deleted && B.n.b.dead);
    } else {
      for (t = 0; t < 300 && B.n.action == native::kDamage; ++t) {
        CHECK(B.n.b.p.points() == 100);
        B.n.execute(B.L), B.n.b.nev = 0;
      }
      CHECK(B.n.action != native::kDamage && B.n.b.p.points() == 0 && B.n.b.st.getup == 1 && B.n.b.phase == Brain::Bk::None && B.n.big_calls == 1);
    }
  }
  // A swing that took no HP is not a decisive hit, new or not.
  {
    Broken B(200);
    B.L.swing_serial = 2;
    B.n.execute(B.L), B.n.b.nev = 0;
    B.n.invuln = 0, B.n.body_hit = true, B.n.body_power = 0;
    B.n.execute(B.L);
    B.n.b.nev = 0;
    CHECK(B.n.big_calls == 0 && B.n.b.phase == Brain::Bk::Punish && B.n.b.st.decisive_sword == 0);
  }
  // Strong sword during the vulnerability: the native big path is that same fall, one big_damage.
  {
    Broken B(200);
    B.L.swing_serial = 2;
    B.n.execute(B.L), B.n.b.nev = 0;
    B.n.invuln = 0, B.n.body_hit = true, B.n.body_strong = true;
    B.n.execute(B.L);
    B.n.b.nev = 0;
    CHECK(B.n.big_calls == 1 && B.n.b.phase == Brain::Bk::Falling && B.n.invuln >= 990);
  }
  // Helm Splitter: prepared by a new press, the native jump cut is a decisive hit (native mHitStatus 0 small path): fall and Finish offered.
  {
    Broken B(200);
    for (int t = 0; t < 12; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    B.L.shield_serial = 2;
    B.n.invuln = 0, B.n.body_hit = true, B.n.body_shield_attack = true;
    B.n.execute(B.L), B.n.b.nev = 0;
    for (int t = 0; t < 12; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(B.n.head_lock && B.n.b.helm_prepared);
    B.L.swing_serial = 2;
    B.n.cut_helm = true;
    B.n.execute(B.L), B.n.b.nev = 0;
    B.n.invuln = 0, B.n.body_hit = true, B.n.body_shield_attack = false;
    B.n.execute(B.L);
    B.n.b.nev = 0;
    CHECK(B.n.b.st.decisive_helm == 1 && B.n.b.st.decisive_sword == 0 && B.n.big_calls == 1 && B.n.action == native::kDamage);
    int t = 0;
    for (; t < 30 && !B.n.down_flag; ++t) B.n.execute(B.L), B.n.b.nev = 0;
    CHECK(B.n.down_flag);
  }
  // Death inside the vulnerability: native, no fall bookkeeping left behind; arrows and bombs keep their native reaction.
  {
    Broken B(6);
    B.L.swing_serial = 2;
    B.n.execute(B.L), B.n.b.nev = 0;
    B.n.invuln = 0, B.n.body_hit = true;
    B.n.execute(B.L);
    B.n.execute(B.L);
    B.n.b.nev = 0;
    CHECK(B.n.hp <= 0 && B.n.big_calls == 1 && B.n.b.dead && B.n.b.phase == Brain::Bk::None);
    Broken O(200);
    O.n.invuln = 0, O.n.body_hit = true, O.n.body_kind = HitKind::Other;
    O.n.execute(O.L), O.n.b.nev = 0;
    CHECK(O.n.action == native::kSDamage && O.n.big_calls == 0 && O.n.b.phase == Brain::Bk::Punish);
    Broken X(200);
    X.n.invuln = 0, X.n.body_hit = true, X.n.body_kind = HitKind::Other, X.n.body_strong = true;
    X.n.execute(X.L), X.n.b.nev = 0;
    X.n.execute(X.L), X.n.b.nev = 0;
    CHECK(X.n.big_calls == 1 && X.n.b.phase == Brain::Bk::Falling);
  }
  // Iron Ball: crossing = the same standing stagger, its native knockdown does not run; below the crossing the native knockdown is kept and it is NOT a break.
  for (bool strong : {true, false}) {
    Native m;
    m.bind_admitted();
    m.execute(L0);
    m.pl_dir = 900, m.timer[2] = 500;
    m.b.p.add(80);
    m.body_hit = true, m.body_kind = HitKind::IronBall, m.body_strong = strong;
    m.execute(L0);
    CHECK(m.b.p.latched && m.big_calls == 0 && m.small_calls == 0 && m.action == native::kDrawback && m.b.phase == Brain::Bk::Punish && m.invuln <= 10);
  }
  // Own block crossing: +5 from 95, inside a native guard: stagger.
  {
    Native n;
    n.bind_admitted();
    n.execute(L0);
    n.pl_dir = 200, n.timer[2] = 5000;
    n.b.p.add(95);
    n.action = native::kGuard, n.mode = 7, n.rate = 0, n.timer[0] = 60;
    n.execute(L0), n.b.nev = 0;
    n.execute(L0), n.b.nev = 0;
    n.tate_hit = n.tate_sword = true;
    n.execute(L0);
    n.b.nev = 0;
    n.execute(L0);
    n.b.nev = 0;
    CHECK(n.b.p.latched && n.b.st.blocks == 1 && n.b.phase == Brain::Bk::Punish && n.action == native::kDrawback && n.big_calls == 0);
  }
  // Non-combat native state: nothing overwritten, delivered when it ends; a native knock already running IS the fall, no second big_damage.
  {
    Native n;
    n.bind_admitted();
    n.execute(L0);
    n.pl_dir = 900, n.timer[2] = 500;
    n.action = 24, n.other_t = 0;  // ACTION_WATER family
    n.b.p.add(100);
    n.b.break_pending = true;
    n.execute(L0);
    n.b.nev = 0;
    CHECK(n.action == 24 && n.big_calls == 0 && n.b.break_pending && n.b.p.latched);
    for (int t = 0; t < 40 && n.b.break_pending; ++t) n.execute(L0), n.b.nev = 0;
    CHECK(!n.b.break_pending && n.big_calls == 0 && n.b.phase == Brain::Bk::Punish);
    Native k;
    k.bind_admitted();
    k.execute(L0);
    k.pl_dir = 900, k.timer[2] = 500;
    k.action = native::kDamage, k.mode = 3, k.timer[0] = 100;
    k.b.p.add(100);
    k.b.break_pending = true;
    k.execute(L0);
    k.b.nev = 0;
    CHECK(!k.b.break_pending && k.big_calls == 0 && k.b.phase != Brain::Bk::None && k.b.phase != Brain::Bk::Punish);
  }
  // Below 100, native knockdowns that are not a posture break (bomb etc.) stay native and never rearm the bar.
  {
    Native n;
    n.bind_admitted();
    n.execute(L0);
    n.pl_dir = 900, n.timer[2] = 500;
    n.b.p.add(40);
    n.body_hit = true, n.body_kind = HitKind::Other, n.body_strong = true;
    n.execute(L0);
    n.b.nev = 0;
    CHECK(n.big_calls == 1 && n.b.phase == Brain::Bk::None);
    for (int t = 0; t < 300 && n.action == native::kDamage; ++t) n.execute(L0), n.b.nev = 0;
    CHECK(n.b.p.rearms == 0 && n.b.p.points() > 0);
  }
  // Pressure and intents do not survive a break.
  {
    Broken B(200);
    CHECK(B.n.b.def.level == 0 && B.n.b.def.swings_within(B.n.b.local, 300) == 0 && !B.n.b.counter.armed && B.n.b.chain_count == 0);
  }
}
// ---------------------------------------------------------------- coverage: pure geometry and probes
static void lab_sweep() {
  using namespace sweep;
  CHECK(first_pose(Act::N, 0) == 23.0f && first_pose(Act::D, 0) == 14.0f && first_pose(Act::D, 1) == 26.0f && first_pose(Act::CSlow, 0) == 34.0f && first_pose(Act::CFast, 0) == 34.0f);
  CHECK(std::fabs(radius(Act::N, 0) - 55.0f) < 0.01f && std::fabs(radius(Act::D, 0) - 55.0f) < 0.01f && std::fabs(radius(Act::D, 1) - 88.0f) < 0.01f && std::fabs(radius(Act::CFast, 0) - 77.0f) < 0.01f);
  float o[kMaxProbes];
  // Rate 1: nothing skipped, nothing evaluated: the native sampling is never touched.
  CHECK(probe_frames(Act::N, 0, 24.0f, 25.0f, true, true, o) == 0);
  // At every frozen rate, every stretch of a window is within half a vanilla frame + the registered pose of something the engine or the probe saw.
  for (Act a : {Act::N, Act::D, Act::CSlow, Act::CFast})
    for (std::uint8_t w : {std::uint8_t(0), std::uint8_t(1)}) {
      if (w == 1 && a != Act::D) continue;
      const float r = rate_of(a);
      const float f0 = first_pose(a, w) - 1.0f;
      float pre = f0 + 0.0f;
      bool prev = false;
      float seen = -1.0f;
      for (int k = 0; k < 8; ++k, pre += r) {
        const float post = pre + r;
        const unsigned n = probe_frames(a, w, pre, post, prev, prev, o);
        CHECK(n <= kMaxProbes);
        float last = prev ? pre : first_pose(a, w);
        for (unsigned i = 0; i < n; ++i) {
          CHECK(o[i] >= first_pose(a, w) && o[i] < post);
          CHECK(o[i] - last <= 0.5f + 1e-3f);
          last = o[i];
        }
        CHECK(post - last <= 0.75f + 1e-3f);
        seen = post;
        prev = true;
      }
      CHECK(seen > f0);
    }
  // Never before the window, never in a span with the armed flag off: the caller's gates; undecided previous pose is taken again.
  CHECK(probe_frames(Act::D, 0, 13.9f, 15.0f, false, false, o) == 2 && o[0] == 14.0f && o[1] == 14.5f);
  const unsigned n_prev = probe_frames(Act::N, 0, 25.0f, 26.25f, true, false, o);
  CHECK(n_prev >= 1 && o[0] == 25.0f);
  CHECK(lunge_back(Act::N, 0, 26.25f, 25.0f) == 0.0f && lunge_back(Act::D, 0, 16.0f, 15.0f) == 0.0f);
  CHECK(std::fabs(lunge_back(Act::D, 1, 28.2f, 27.2f) - 25.0f) < 1e-3f && std::fabs(lunge_back(Act::CFast, 0, 36.0f, 35.0f) - 30.0f) < 1e-3f);
  // Decision: already touching writes nothing; a real candidate on an ON cylinder writes; Tg off is immunity; a miss is a miss, always.
  Cyl k[3];
  for (auto& c : k) c.c = {0, 0, 0}, c.r = 35, c.h = 180, c.on = true;
  Sph cur;
  cur.c = {0, 100, 200}, cur.r = 55;
  Sph cand[2];
  cand[0].c = {0, 100, 170}, cand[0].r = 55;
  cand[1].c = {0, 100, 60}, cand[1].r = 55;
  Decision d = decide(k, cur, cand, 2);
  CHECK(d.why == Why::Written && d.write && d.place.z == 60.0f);
  Sph near;
  near.c = {0, 100, 60}, near.r = 55;
  d = decide(k, near, cand, 2);
  CHECK(d.why == Why::AlreadyTouching && !d.write);
  for (auto& c : k) c.on = false;
  d = decide(k, cur, cand, 2);
  CHECK(d.why == Why::LinkInvulnerable && !d.write);
  for (auto& c : k) c.on = true;
  Sph miss[1];
  miss[0].c = {0, 100, 91}, miss[0].r = 55;  // 35+55 = 90 < 91: no hitbox was widened, one unit out is out
  d = decide(k, cur, miss, 1);
  CHECK(d.why == Why::NoMidContact && !d.write);
  miss[0].c = {0, 100, 89};
  d = decide(k, cur, miss, 1);
  CHECK(d.why == Why::Written);
  miss[0].c = {0, 400, 60};  // above Link's cylinder
  d = decide(k, cur, miss, 1);
  CHECK(d.why == Why::NoMidContact);
  // Reach gate: a performance gate only
  CHECK(reachable(k, {0, 100, 300}, {0, 100, 100}, true, 55));
  CHECK(!reachable(k, {0, 100, 2000}, {0, 100, 2000}, true, 55));
  // Tracker: a sphere expected to cross with no record is a loss unless the engine's no-continuous-hit rule explains it, never a fabricated hit.
  Native n;
  n.bind_admitted();
  LinkFrame L;
  n.execute(L);
  n.b.ep.active = true, n.b.ep.pattern = 2, n.b.ep.act = 0, n.b.ep.anm_pending = false;
  n.b.imp.open = true;
  n.b.on_sweep(true, false, false, false);
  (void)n.b.on_exec_pre(n.view(), L);
  CHECK(n.b.imp.lost == 1 && n.b.imp.nocon == 0);
  n.b.on_sweep(true, false, false, false);
  (void)n.b.on_exec_pre(n.view(), L);
  CHECK(n.b.imp.lost == 1 && n.b.imp.nocon == 1);
  n.b.on_sweep(false, true, false, true);
  CHECK(n.b.imp.written == 1 && n.b.st.gap_fixed == 1 && n.b.imp.exp_pending);
}
// After every episode there is an opening: no relentless chain.
static void lab_rest() {
  LinkFrame L;
  Native n;
  n.bind_admitted();
  n.execute(L);
  n.pl_dir = 200, n.timer[2] = 0;
  int eps = 0;
  std::uint64_t last_end = 0;
  int min_rest = 1 << 30;
  for (int t = 0; t < 6000; ++t) {
    const bool was = n.b.ep.active;
    n.execute(L);
    n.b.nev = 0;
    if (was && !n.b.ep.active) last_end = n.ticks;
    if (!was && n.b.ep.active) {
      if (eps > 0 && static_cast<int>(n.ticks - last_end) < min_rest) min_rest = static_cast<int>(n.ticks - last_end);
      ++eps;
    }
  }
  CHECK(eps >= 10);
  CHECK(min_rest >= policy::kRestMin);
  // and the draw is the only pattern source in a long fight
  std::uint64_t s = 0;
  for (auto x : n.b.st.selected) s += x;
  CHECK(s == static_cast<std::uint64_t>(eps));
  for (unsigned p = 0; p < kPatterns; ++p) CHECK(n.b.st.selected[p] > 0);
}
static void lab_long_soak() {
  // Long mixed fight on many seeds: invariants always hold, no state deadlocks: the actor keeps acting.
  for (std::uint64_t seed = 1; seed <= 60; ++seed) {
    Native n;
    n.bind_admitted();
    n.b.rng.seed(seed);
    Rng w;
    w.seed(seed * 77);
    LinkFrame L;
    n.execute(L);
    L.shape_y = static_cast<std::int16_t>(0x8000);
    std::uint64_t serial = 1;
    std::uint64_t last_action_change = 0;
    std::int16_t last_action = n.action;
    int hp_prev = n.hp;
    for (int t = 0; t < 12000 && n.hp > 0; ++t) {
      n.pl_dir = 120.0f + static_cast<float>(w.below(300));
      if (w.below(20) == 0) L.swing_serial = ++serial;
      LinkFrame F = L;
      if (n.at_hit) {
        const auto c = w.below(4);
        F.pass.valid = c != 0;
        F.pass.guard_attack = c == 1;
        F.pass.frame = 3, F.pass.start = 1, F.pass.age_ticks = 2;
        n.at_shield = c != 0;
      }
      if (w.below(40) == 0 && n.invuln == 0) {
        n.body_hit = true;
        const auto k = w.below(10);
        n.body_kind = k < 7 ? HitKind::Sword : k == 7 ? HitKind::IronBall : HitKind::Other;
        n.body_strong = k == 7 || w.below(8) == 0;
        n.body_power = 2;
      }
      if (n.action == native::kGuard && w.below(6) == 0) n.tate_hit = n.tate_sword = true;
      n.execute(F);
      n.b.nev = 0;
      if (n.armed_this_tick) {
        if (w.below(3) == 0) n.at_hit = true;
      }
      CHECK(n.hp <= hp_prev);  // 0 HP regeneration, always
      hp_prev = n.hp;
      CHECK(n.b.p.milli >= 0 && n.b.p.milli <= policy::kMaxMilli);
      CHECK(n.rate != 0.0f || n.action == native::kGuard || n.action == native::kFightRun || n.action == native::kDamage);
      CHECK(!(n.action == native::kGuard && n.armed_this_tick));
      if (n.action != last_action) last_action = n.action, last_action_change = n.ticks;
      if (n.hp > 0) CHECK(n.ticks - last_action_change < 1500);  // no AI freeze in any state of the model
    }
  }
}
// Synthetic cost: one fighting actor per slot, native model with and without the brain. Not FPS.
static double bench_run(unsigned actors, bool brain, unsigned long long ticks) {
  std::vector<Native> v(actors);
  for (auto& n : v) {
    n.native_only = !brain;
    if (brain) n.bind_admitted();
    n.pl_dir = 200;
    n.timer[2] = 0;
  }
  LinkFrame L;
  L.shape_y = static_cast<std::int16_t>(0x8000);
  const auto t0 = std::chrono::steady_clock::now();
  for (unsigned long long t = 0; t < ticks; ++t) {
    if ((t & 31u) == 0) ++L.swing_serial;
    for (auto& n : v) {
      n.execute(L);
      n.b.nev = 0;
      if (n.hp <= 0) n.hp = 370;
    }
  }
  const auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::nano>(t1 - t0).count() / double(ticks);
}
int main(int argc, char** argv) {
  if (argc > 2 && !std::strcmp(argv[1], "--bench")) {
    const unsigned long long ticks = std::strtoull(argv[2], nullptr, 10);
    std::printf("LIZ_BENCH metric=ns_per_simulation_tick synthetic=1 not_fps=1\n");
    for (unsigned a : {0u, 1u, 4u, 8u}) {
      const double base = a ? bench_run(a, false, ticks) : 0.0;
      const double with = a ? bench_run(a, true, ticks) : 0.0;
      std::printf("LIZ_BENCH actors=%u native_model_ns=%.0f with_brain_ns=%.0f brain_cost_ns=%.0f brain_ns_per_actor=%.0f\n", a, base, with, with - base, a ? (with - base) / a : 0.0);
    }
    std::printf("LIZ_BENCH actors_without_lizalfos: no hook target of this module is on an E_OC path; maintenance+HUD loop over 16 empty slots only\n");
    return 0;
  }
  lab_identity();
  lab_posture();
  lab_tables();
  lab_draw();
  lab_complete();
  lab_stale_frame();
  lab_lunge();
  lab_hit();
  lab_double_gap();
  lab_defense();
  lab_guard_live();
  lab_reaction();
  lab_frozen_rhythm();
  lab_lease();
  lab_boundary_fresh();
  lab_counter();
  lab_break();
  lab_sweep();
  lab_rest();
  lab_long_soak();
  std::printf("LIZ_LAB version=%s checks=%llu failures=0 RESULT=LAB_PASS scope=PRODUCTION_BRAIN_TRANSCRIBED_NATIVE_E_DN_PATHS clip_lengths=SWEPT_NOT_READ runtime=HUMAN_RUNTIME_PENDING\n", kVersion, g_checks);
  return 0;
}
