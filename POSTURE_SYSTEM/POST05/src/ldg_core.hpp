#pragma once
#include <cstdint>
// LIGHT FADES TWICE 0.13.16 LDG01 (DKN01 FIX07 change 6): directional invulnerability frames of the HUMAN Link's evasive moves, inspired by Sekiro (approved
// values, independent of any Sekiro version: forward 9 ticks = 0.30 s, side 6 ticks = 0.20 s, back 6 ticks = 0.20 s at the 30 Hz simulation).
// PURE core: standard library only. src/ldg_adapter.inc feeds it Link's real procedure once per executed Link tick (daAlink_c::setCollision PRE, the end of
// Link's execute: the procedure of THIS tick is final, its collision cylinders are about to be registered for THIS tick's collision pass) and asks it, inside
// that pass (dCcS::ChkAtTgHitAfterCross POST, the commit of one AT/TG crossing), whether one enemy contact on Link's Tg cylinders is evaded.
//
// Native facts [S = TwilitRealm/dusklight v2.0.3 40457c6, B = dusklight.exe 2.0.3 ffa99203..., measured]:
//  * daAlink_c::checkMoveDoAction: the Do button with status JUMP (Z-target) -> procSideStepInit(direction): DIR_LEFT / DIR_RIGHT = PROC_SIDESTEP (side jump,
//    field_0x2f98 = the direction), DIR_BACKWARD = procBackJumpInit (PROC_BACK_JUMP, the backflip) or, in a heavy state, PROC_SIDESTEP with DIR_BACKWARD (the
//    native back jump clip); status 0x79 -> procFrontRollInit (PROC_FRONT_ROLL). PROC_SIDE_ROLL = checkSideRollAction (the Back Slice preparation, a hidden skill)
//    or a fall landing: never mapped. A front / side roll that ends a fall (procFall / procLand) is a recovery, never a dodge.
//  * cCcS::SetAtTgCommonHitInf commits a crossing only when the virtual ChkAtTgHitAfterCross returns false (B: SetAtTgCommonHitInf `call [rbp+0x28]`): then
//    SetAtHit / SetTgHit / the damage / dCcS::SetAtTgGObjInf (hit position, shield flags, the hit callbacks) and, in dCcS, ProcAtTgHitmark (the spark). A true
//    answer = the crossing never existed for either actor: no life, no posture receipt, no guard, no parry, no hit mark, no clash, nothing to undo.
//  * An evaded contact keeps being a non-contact while the SAME AT collider keeps crossing Link pass after pass (a blade still sweeping through him): no
//    deferred fictitious damage from the contact already evaded. The Darknut's impact keeps its exact identity (its impact-window serial) for the whole window.
namespace ldg {
inline constexpr const char* kVersion = "0.13.16";
inline constexpr const char* kTag = "LDG01";
// ---------------------------------------------------------------- approved numbers (do not change)
namespace policy {
inline constexpr unsigned kForwardTicks = 9;  // approved: forward roll 0.30 s
inline constexpr unsigned kSideTicks = 6;     // approved: side jump 0.20 s
inline constexpr unsigned kBackTicks = 6;     // approved: back jump / backflip 0.20 s
inline constexpr float kCombatReach = 800.0f; // [PROVISIONAL] an eligible enemy this close at the dodge start = a combat dodge (or Link Z-targets an enemy)
inline constexpr unsigned kHeld = 8;          // evaded contacts tracked at once (continuity of one blade through Link)
}  // namespace policy
// ---------------------------------------------------------------- native constants (pinned by ci/ldg_audit.py against the SDK headers)
namespace native {
inline constexpr int kProcSideStep = 0x0A, kProcSideStepLand = 0x0B, kProcSlide = 0x0C, kProcSlideLand = 0x0D, kProcFrontRoll = 0x0E, kProcSideRoll = 0x11,
                     kProcBackJump = 0x12, kProcBackJumpLand = 0x13, kProcSlip = 0x14, kProcAutoJump = 0x15, kProcDiveJump = 0x16, kProcRollJump = 0x17, kProcFall = 0x18,
                     kProcLand = 0x19;
inline constexpr int kDirForward = 0, kDirBackward = 1, kDirLeft = 2, kDirRight = 3;  // daAlink_c DIR_* (getDirectionFromShapeAngle)
}  // namespace native
enum class Dir : std::uint8_t { None = 0, Forward, Side, Back, Count };
inline const char* name(Dir d) noexcept {
  static constexpr const char* n[] = {"NONE", "FORWARD_ROLL", "SIDE_JUMP", "BACK_JUMP"};
  return unsigned(d) < unsigned(Dir::Count) ? n[unsigned(d)] : "?";
}
// The evasive move of a native procedure (side = the side jump's own direction field). Everything else (jumps, rolls of a landing, the Back Slice side roll,
// attacks, guard, wolf / horse / swim procedures) is not an evasion.
inline Dir dir_of(int proc, int side_dir) noexcept {
  if (proc == native::kProcFrontRoll) return Dir::Forward;
  if (proc == native::kProcBackJump) return Dir::Back;
  if (proc == native::kProcSideStep) return side_dir == native::kDirBackward ? Dir::Back : side_dir == native::kDirLeft || side_dir == native::kDirRight ? Dir::Side : Dir::None;
  return Dir::None;
}
inline unsigned window_of(Dir d) noexcept { return d == Dir::Forward ? policy::kForwardTicks : d == Dir::Side ? policy::kSideTicks : d == Dir::Back ? policy::kBackTicks : 0u; }
// A procedure the evasion may NOT come from (it would be a fall / jump recovery, not a voluntary evasion in combat).
inline bool from_recovery(int prev_proc) noexcept {
  switch (prev_proc) {
    case native::kProcSlide: case native::kProcSlideLand: case native::kProcSlip: case native::kProcAutoJump: case native::kProcDiveJump: case native::kProcRollJump:
    case native::kProcFall: case native::kProcLand:
      return true;
    default:
      return false;
  }
}
// The attack classes the exclusions read (verified mapping only, see ldg_adapter.inc): Ordinary = every eligible enemy contact; Thrust = the Darknut's
// TNB_ATACK_A lunge (Move F2A: the only verified genuine thrust of the supported enemies); Sweep = a genuine special sweep: NONE of the supported enemies has one
// (no class is invented), kept so the rule is complete and tested.
enum class Atk : std::uint8_t { Ordinary = 0, Thrust, Sweep, Count };
inline const char* name(Atk a) noexcept {
  static constexpr const char* n[] = {"ORDINARY", "THRUST", "SWEEP"};
  return unsigned(a) < unsigned(Atk::Count) ? n[unsigned(a)] : "?";
}
// Sekiro-inspired exclusions: the forward roll is no protection against thrusts and special sweeps; side / back none against special sweeps.
inline bool excluded(Dir d, Atk a) noexcept { return a == Atk::Sweep || (d == Dir::Forward && a == Atk::Thrust); }
enum class Res : std::uint8_t { Contact = 0, Evaded, Excluded, OutsideWindow, NotEligible, Held, Count };
inline const char* name(Res r) noexcept {
  static constexpr const char* n[] = {"CONTACT", "EVADED", "EXCLUDED_ATTACK_CLASS", "OUTSIDE_WINDOW", "DODGE_NOT_ELIGIBLE", "EVADED_SAME_CONTACT_HELD"};
  return unsigned(r) < unsigned(Res::Count) ? n[unsigned(r)] : "?";
}
// ---------------------------------------------------------------- one evasive action of Link (instance, generation, serial)
struct Dodge {
  bool on = false, eligible = false;
  Dir dir = Dir::None;
  std::uint64_t serial = 0, start_tick = 0;
  unsigned age = 0;  // executed Link ticks since its start (0 = the tick it began)
  float last_frame = 0;
  int proc = -1;
  bool protects() const noexcept { return on && eligible && age < window_of(dir); }
};
struct Tracker {
  Dodge d;
  int prev_proc = -1;
  std::uint64_t protect_pass = ~0ull;  // the collision pass (simulation tick) this tick's window covers
  std::uint64_t starts[unsigned(Dir::Count)] = {}, eligible_n = 0, not_combat = 0, from_recovery_n = 0, not_human = 0, restarts = 0, ticks_protected = 0;
  // Once per EXECUTED Link tick, at the end of his execute (the procedure is final; its cylinders go to THIS tick's pass). frame = his main clip frame.
  // A NEW action = a dodge procedure entered from another procedure, or the same procedure restarted (its clip frame went back). The same action seen again
  // (input held, the same procedure on later ticks, a second callback of the same tick) never reopens a window.
  void observe(std::uint64_t now, int proc, int side_dir, float frame, bool human, bool combat) noexcept {
    const Dir dr = human ? dir_of(proc, side_dir) : Dir::None;
    if (observed && now == last_tick) return;  // a second observation of the same tick (a doubled callback): nothing advances, nothing reopens
    observed = true, last_tick = now;
    const bool same = d.on && dr != Dir::None && proc == d.proc && dr == d.dir && !(frame + 0.5f < d.last_frame);
    if (same) {
      ++d.age, d.last_frame = frame;
    } else if (dr != Dir::None) {
      if (d.on && proc == d.proc) ++restarts;
      const bool rec = from_recovery(prev_proc == proc ? -1 : prev_proc);
      d = Dodge{};
      d.on = true, d.dir = dr, d.serial = ++serial_n, d.start_tick = now, d.age = 0, d.last_frame = frame, d.proc = proc;
      d.eligible = combat && !rec;
      ++starts[unsigned(dr)];
      if (d.eligible) ++eligible_n;
      else if (rec) ++from_recovery_n;
      else ++not_combat;
    } else {
      if (!human && dir_of(proc, side_dir) != Dir::None) ++not_human;
      d = Dodge{};
    }
    protect_pass = d.protects() ? now : ~0ull;
    if (d.protects()) ++ticks_protected;
    prev_proc = proc;
  }
  // Inside the collision pass of tick `pass`: may THIS action evade a contact of class `a`? (the identity / continuity rules are the caller's: Held)
  Res verdict(std::uint64_t pass, Atk a) const noexcept {
    if (!d.on) return Res::Contact;
    if (!d.eligible) return Res::NotEligible;
    if (protect_pass != pass) return Res::OutsideWindow;
    if (excluded(d.dir, a)) return Res::Excluded;
    return Res::Evaded;
  }
  void reset() noexcept { d = Dodge{}, prev_proc = -1, protect_pass = ~0ull, observed = false, last_tick = 0; }
  std::uint64_t serial_n = 0;
  bool observed = false;
  std::uint64_t last_tick = 0;
};
// ---------------------------------------------------------------- evaded contacts held (one blade sweeping through Link pass after pass)
// key = the AT collider (opaque), attacker = its actor id, impact = the attacker's own impact identity when it has one (the Darknut's impact-window serial,
// 0 otherwise). A contact is the SAME evaded contact while its collider crosses Link again in the very next pass (or, with an impact identity, inside that
// same impact window): it stays a non-contact. A gap of one pass, or another impact, = a new contact (judged again: inside a window or not).
struct Held {
  std::uintptr_t key = 0;
  std::uint32_t attacker = 0;
  std::uint64_t impact = 0, pass = 0, dodge = 0;
};
struct HeldSet {
  Held h[policy::kHeld];
  unsigned next = 0;
  std::uint64_t held_hits = 0;
  // true = this crossing continues an evaded contact (and the record is advanced to this pass)
  bool continues(std::uintptr_t key, std::uint32_t attacker, std::uint64_t impact, std::uint64_t pass) noexcept {
    for (Held& x : h) {
      if (!x.key || x.key != key || x.attacker != attacker) continue;
      const bool same = impact ? x.impact == impact : (pass == x.pass || pass == x.pass + 1);
      if (!same) continue;
      x.pass = pass, ++held_hits;
      return true;
    }
    return false;
  }
  void add(std::uintptr_t key, std::uint32_t attacker, std::uint64_t impact, std::uint64_t pass, std::uint64_t dodge) noexcept {
    for (Held& x : h)
      if (x.key == key && x.attacker == attacker) {
        x = Held{key, attacker, impact, pass, dodge};
        return;
      }
    h[next++ % policy::kHeld] = Held{key, attacker, impact, pass, dodge};
  }
  // records nobody continued for more than one pass are forgotten (impact identities: their window closed long ago when the attacker reuses the slot)
  void prune(std::uint64_t pass) noexcept {
    for (Held& x : h)
      if (x.key && pass > x.pass + (x.impact ? 90u : 1u)) x = Held{};
  }
  void clear() noexcept {
    for (Held& x : h) x = Held{};
  }
};
}  // namespace ldg
