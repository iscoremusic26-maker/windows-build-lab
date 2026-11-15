#pragma once
#include <cstddef>
#include <cstdint>
// LIGHT FADES TWICE 0.13.8 DARKNUT RESURRECTION (DKR01). PURE core: standard library only, no game type, no SDK, no address.
// dkr_adapter.inc feeds it what the game says (the native ending mode of daB_TN_c::executeZakoEnding, Link's procedure, the native Ending Blow receipt)
// and applies its verdicts through a handful of audited writers. Every number of the request is here.
//
// Native facts [S = src/d/actor/d_a_b_tn.cpp, src/d/actor/d_a_alink_cut.inc @40457c6; B = dusklight.exe 2.0.3, sha ffa99203...]:
//  * Target: the normal Darknut Enemy Spawner creates, B_TN with parameters 0x000001FF: mSwitchNo 0xFF, mType 1 (archive B_tnp2), mBrkAnmFrame 0, mType2 0
//    (the mace body). Every other B_TN (Temple of Time mType 0, the 0x010001FF "Darknut (Sword)", placed Darknuts with a switch or a Hyrule Castle
//    stay-no-2 flag) is never touched.
//  * Second phase defeat: setDamage adds the hit to field_0x6fc and, once it reaches field_0x700 (l_HIO.field_0x24 = 360, rewritten every action()),
//    calls setActionMode(ACT_ENDING, 0). health is reset to 100 before every hit, so health==0 is never the death signal. For mType 1, action() calls
//    executeZakoEnding() (S L4526, B: a real call through the 0x4c11d thunk) every tick while mActionMode1 == ACT_ENDING, and damage_check returns at
//    once in ACT_ENDING (S L1267): the ending cannot be hit again, restarted or duplicated.
//  * executeZakoEnding: mode 0 = attention off, BCK_TN2B_DIE once, DEAD2 voice, speed 0, neck off (falls into mode 1 the same call); mode 1 = frame 3
//    mStates[15]=2 (the sword falls and disappears), frames 33/68 knee and body-fall shakes and SEs, mMaterialColor chased to -50 (darkening), at the
//    clip's stop mode 2 + mTimer1 10; mode 2 = disappear effect + switch + fopAcM_delete. Its switch has no default: any other mode is a no-op.
//  * Link's Ending Blow: checkDownAttackState needs the LOCKED target to be an enemy with checkDownFlg() and getDownPos() within 800; the press re-checks
//    it (checkDoCutAction). procCutDownLandInit gives onCutDownHitFlg() to field_0x280c's actor only if it still has the down flag, the sword tip lands
//    within 100 (XZ) of the down position and the heights are within 100: that flag IS the native success receipt. Success ends with procBackJumpInit(1).
// LFT 0.13.10 (DKN01 FIX01): a second kind of fall, the EXECUTION fall (Mortal Draw / Shield Attack + Helm Splitter on a broken Darknut that still has
// resistance). DKN01 asks for it (exec_request) and writes ACT_ENDING 0 itself; the native executeZakoEnding mode 0 then plays the same TN2B_DIE fall,
// which this module keeps exactly like the first defeat (body, sword, colour, attention, down flag, the native Ending Blow). What differs:
//    * the wait on the ground is kDownTicks (180 = 6 s) counted from the END of the fall clip, frozen while an Ending Blow flies (never a rise mid-Finish);
//    * no Finish in that time and resistance left -> GET UP. 0.13.11 (FIX02, user decision): the getup PLAYS THE FIX02 CLIP (the same approved
//      bytes, the same once/no-morph binding as the resurrection: frame 0 = TN2B_DIE's last pose, frame 88 = TNB_WAIT 0). ANIMATION shared, LOGIC
//      separate: the SAME resistance (field_0x6fc untouched), NO resurrection consumed (revivals unchanged), no stage change; back to ACT_CHASEL 2
//      (a guarded wait). (0.13.10 played TN2B_DIE backwards: removed.)
//    * Finish (native receipt) BEFORE the one resurrection was used (phase 2) -> exactly the first-defeat path: FIX02 once, 100 % resistance, phase 3;
//    * Finish AFTER it (phase 3) -> the DEFINITIVE native death: once Link's whole Ending Blow is over, the native vanish (mode 2: darkening, disappear,
//      delete) of the body already on the ground: no second fall, no second resurrection.
//    The real lethal path (resistance 480 reached by ordinary damage) is unchanged: first -> fall/Finish/timer 300/FIX02; after the resurrection -> native death.
namespace dkr {
inline constexpr const char* kVersion = "0.13.11";
inline constexpr const char* kTag = "DKR01";

// ---------------------------------------------------------------- the request's numbers
inline constexpr unsigned kReviveDelayTicks = 300;      // 10 s at the 30 Hz simulation: counted in the actor's own execute, after the native fall ends
inline constexpr unsigned kFinishWatchdogTicks = 600;   // 20 s: a Link procedure that never ends cannot leave the Darknut down forever
inline constexpr unsigned kMaxRevivals = 1;             // ONE resurrection per actor, ever
inline constexpr unsigned kMaxSlots = 16;
inline constexpr std::uint32_t kTargetParam = 0x000001FFu;  // Enemy Spawner "Darknut"
inline constexpr int kHeldMode = 0x5D;                   // executeZakoEnding's mode while we hold the body (native cases are 0..3)
inline constexpr unsigned kExecDownTicks = 180;          // DKN01 FIX01: 6 s on the ground after an EXECUTION fall (from the end of the fall clip)
inline constexpr unsigned kGetupMaxTicks = 120;          // watchdog of the getup clip (FIX02: 88 frames)
inline constexpr unsigned kFinalVanishTicks = 45;        // phase-3 Finish: the native mode 2 (darkening chase) runs this long before the disappear + delete
// FIX02: the FIX02 clip ends on TNB_WAIT 0 (Fable QA_FIX02): the native wait loop takes over with a 3-frame blend.
inline constexpr float kGetupWaitMorph = 3.0f;

// ---------------------------------------------------------------- the resurrection clip (LFT 0.13.9: resurrection_A_agil_natural_FIX02.bck, Fable 5.2, replaces FIX01)
// FIX02, measured on the shipped bytes: 45024 B, SHA-256 f96356fb...1524, J3D1/bck1/ANK1, loop 0 (once), rotation shift 1, 88 frames (2.933 s at 30 Hz),
// 29 joints = the 29 joints of B_tnp2 tn2.bmd (the mType 1 rig); frame 0 = BCK_TN2B_DIE last pose, frame 88 = BCK_TNB_WAIT(0) (Fable QA_FIX02).
inline constexpr const char* kBckPath = "darknut/resurrection_A_agil_natural_FIX02.bck";
inline constexpr std::uint32_t kBckBytes = 45024;
inline constexpr unsigned kBckFrames = 88, kBckJoints = 29;

// Native ending modes (executeZakoEnding) and actions (daB_TN_c::daB_TN_ACTION1)
inline constexpr int kActEnding = 0xE, kActChaseL = 0x9;
inline constexpr int kModeStart = 0, kModeFall = 1, kModeVanish = 2;

enum class Phase : std::uint8_t { Armed = 0, Falling, Down, Committed, FinishHit, Rising, Consumed, Native, GettingUp, Final, Count };
inline const char* name(Phase p) noexcept {
  static constexpr const char* n[] = {"ARMED", "FALLING", "DOWN", "FINISH_COMMITTED", "FINISH_HIT", "RISING", "CONSUMED", "NATIVE", "GETTING_UP", "FINAL_DEATH"};
  return unsigned(p) < unsigned(Phase::Count) ? n[unsigned(p)] : "?";
}
enum class Path : std::uint8_t { None = 0, Finish, Timer };
inline const char* name(Path p) noexcept { return p == Path::Finish ? "FINISH" : p == Path::Timer ? "TIMER" : "NONE"; }

// ---------------------------------------------------------------- identity
struct Identity {
  std::uint32_t param = 0;
  int type = -1, type2 = -1, switch_no = -1;
  bool stay_no2 = false;
};
inline bool eligible(const Identity& i) noexcept {
  return i.param == kTargetParam && i.type == 1 && i.type2 == 0 && i.switch_no == 0xFF && !i.stay_no2;
}

// ---------------------------------------------------------------- Link, as the adapter reads it once per Darknut tick
struct LinkView {
  bool finishing = false;  // mProcID is PROC_CUT_DOWN or PROC_CUT_DOWN_LAND (whatever its target)
  bool on_me = false;      // ... and field_0x280c (the Ending Blow's enemy) is this Darknut
  bool back_jump = false;  // PROC_BACK_JUMP / PROC_BACK_JUMP_LAND (the native exit of a successful Ending Blow)
};

// ---------------------------------------------------------------- verdicts
enum class Step : std::uint8_t {
  None = 0,
  TakeFall,       // first lethal transition: let the native fall run, keep the body (no darkening, no sword drop, no vanish)
  NativeDeath,    // nothing of ours: the original death runs (second defeat, ineligible, no slot, no clip)
  FallFixups,     // native fall in progress: undo only the darkening, the sword drop and the attention reset
  EnterDown,      // the fall clip stopped: hold the body (kHeldMode), down flag + position, start the timer
  HoldDown,       // keep the down flag/position fresh
  Rise,           // play the resurrection clip once (down flag off, the one resurrection consumed)
  RiseHold,       // the clip is playing
  BackToCombat,   // clip ended: 100 % resistance, native second-phase chase (ACT_CHASEL mode 0), nothing else
  GetUp,          // FIX01/FIX02: execution fall, no Finish, 180 ticks over, resistance left: the FIX02 clip starts as a GETUP (no resurrection)
  GetUpHold,      // the getup is playing
  GetUpDone,      // standing: the SAME resistance, a guarded native wait, back to the controller
  FinalDeath,     // FIX01: phase-3 execution Finish confirmed and Link's Ending Blow over: the native vanish of the body on the ground
};
inline const char* name(Step s) noexcept {
  static constexpr const char* n[] = {"NONE", "TAKE_FALL", "NATIVE_DEATH", "FALL_FIXUPS", "ENTER_DOWN", "HOLD_DOWN", "RISE", "RISE_HOLD", "BACK_TO_COMBAT",
                                      "GETUP", "GETUP_HOLD", "GETUP_DONE", "FINAL_DEATH"};
  return unsigned(s) < 13 ? n[unsigned(s)] : "?";
}

// One Darknut, keyed by its process id (never an address). Lives from its first lethal transition to its _delete.
struct Ctl {
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  Phase phase = Phase::Armed;
  Path path = Path::None;
  unsigned revivals = 0;
  unsigned timer = 0;            // ticks left before the timer path may start the rise
  unsigned down_ticks = 0, commit_ticks = 0, hit_wait_ticks = 0, rise_ticks = 0;
  unsigned commits = 0, misses = 0;
  bool hit = false;              // the native Ending Blow receipt was seen on this actor
  bool watchdog = false;
  bool exec = false;             // FIX01: this fall is an EXECUTION fall (resistance left), not a defeat
  unsigned exec_falls = 0, getups = 0, getup_ticks = 0, finals = 0;
  // What the native ending changes and the first fall must keep, saved at the lethal transition (restored on the way back to combat)
  int saved_sword = 1;           // mStates[15]: 1 = the second-phase sword in the hand
  float saved_color = 0.0f;      // mMaterialColor: 0 = no darkening
  std::uint32_t saved_attn = 0;  // attention_info.flags (BATTLE)
  bool saved_neck = false;       // mUpdateNeckAngle

  bool consumed() const noexcept { return revivals >= kMaxRevivals; }

  // executeZakoEnding PRE with the native mode 0 (the lethal transition). usable = eligible actor + clip ready + a slot.
  Step on_lethal(bool usable) noexcept {
    if (phase == Phase::Consumed || consumed()) return phase = Phase::Consumed, Step::NativeDeath;
    if (phase == Phase::Falling || phase == Phase::Down || phase == Phase::Committed || phase == Phase::FinishHit || phase == Phase::Rising) return Step::None;  // idempotent
    if (!usable || phase != Phase::Armed) return phase = Phase::Native, Step::NativeDeath;
    phase = Phase::Falling, path = Path::None, hit = false;
    return Step::TakeFall;
  }
  // FIX01: executeZakoEnding PRE with mode 0 that DKN01 requested as an EXECUTION fall (the broken Darknut still has resistance). Allowed before and after
  // the one resurrection; never on a body already owned. usable = eligible + the FIX02 clip bound (needed if this fall ends in the phase-2 Finish).
  Step on_execution(bool usable) noexcept {
    if (phase == Phase::Falling || phase == Phase::Down || phase == Phase::Committed || phase == Phase::FinishHit || phase == Phase::Rising ||
        phase == Phase::GettingUp || phase == Phase::Final)
      return Step::None;
    if (!usable || phase == Phase::Native) return Step::None;
    phase = Phase::Falling, path = Path::None, hit = false, exec = true, watchdog = false, ++exec_falls;
    return Step::TakeFall;
  }
  // executeZakoEnding POST while falling: native mode after the call.
  Step after_fall(int native_mode) noexcept {
    if (phase != Phase::Falling) return Step::None;
    if (native_mode == kModeVanish) {
      phase = Phase::Down, timer = exec ? kExecDownTicks : kReviveDelayTicks, down_ticks = commit_ticks = hit_wait_ticks = 0;
      return Step::EnterDown;
    }
    return Step::FallFixups;
  }
  // executeZakoEnding POST while held (Down / Committed / FinishHit). cut_down_hit = the native receipt on this actor this tick.
  // hp_left (FIX01) = the native accumulator is still below the threshold: an execution fall may get up only then.
  Step held(const LinkView& l, bool cut_down_hit, bool hp_left = true) noexcept {
    if (phase != Phase::Down && phase != Phase::Committed && phase != Phase::FinishHit) return Step::None;
    ++down_ticks;
    if (cut_down_hit && phase != Phase::FinishHit) {
      phase = Phase::FinishHit, hit = true, hit_wait_ticks = 0;
    }
    if (phase == Phase::FinishHit) {
      // Path A: rise only once Link's whole Ending Blow is over (jump, stab, hold, pull-out back flip and its landing)
      if (l.finishing || l.back_jump) {
        if (++hit_wait_ticks < kFinishWatchdogTicks) return Step::HoldDown;
        watchdog = true;
      }
      if (exec && consumed()) return phase = Phase::Final, exec = false, ++finals, Step::FinalDeath;  // FIX01: phase-3 execution Finish = definitive death
      exec = false;                                                                                  // phase-2 execution Finish = the first defeat
      return start_rise(Path::Finish);
    }
    if (phase == Phase::Committed) {
      if (l.finishing && l.on_me) {
        if (++commit_ticks < kFinishWatchdogTicks) return Step::HoldDown;
        watchdog = true;  // never stuck: fall back to the timer rules
      }
      phase = Phase::Down, ++misses;  // the Ending Blow missed or was interrupted: no receipt, the timer resumes where it froze
    }
    if (l.finishing && l.on_me && !watchdog) {
      phase = Phase::Committed, ++commits, commit_ticks = 0;
      return Step::HoldDown;
    }
    if (l.finishing) return Step::HoldDown;  // the timer freezes while ANY Ending Blow is in flight: the rise never starts mid-jump
    if (timer > 0) --timer;
    if (timer == 0) {
      if (exec && hp_left) return phase = Phase::GettingUp, getup_ticks = 0, Step::GetUp;  // FIX01: 180 ticks, no Finish: get up, nothing consumed
      if (exec && consumed()) return phase = Phase::Final, exec = false, ++finals, Step::FinalDeath;  // no resistance after the resurrection: never up
      exec = false;
      return start_rise(Path::Timer);
    }
    return Step::HoldDown;
  }
  // FIX01/FIX02: executeZakoEnding POST while getting up: reached = the FIX02 clip stopped (or is not on the model any more).
  Step getting_up(bool reached) noexcept {
    if (phase != Phase::GettingUp) return Step::None;
    ++getup_ticks;
    if (reached || getup_ticks >= kGetupMaxTicks) {
      watchdog = watchdog || !reached;
      phase = consumed() ? Phase::Consumed : Phase::Armed, exec = false, ++getups;  // the ONE resurrection is exactly as it was before the fall
      return Step::GetUpDone;
    }
    return Step::GetUpHold;
  }
  Step start_rise(Path p) noexcept {
    phase = Phase::Rising, path = p, ++revivals, rise_ticks = 0;
    return Step::Rise;
  }
  // executeZakoEnding POST while rising: still_ours = the model plays our clip; stopped = the once-clip reached its end.
  Step rising(bool still_ours, bool stopped) noexcept {
    if (phase != Phase::Rising) return Step::None;
    ++rise_ticks;
    if (!still_ours || stopped) {
      phase = Phase::Consumed;
      return Step::BackToCombat;
    }
    return Step::RiseHold;
  }
};

struct Table {
  Ctl slot[kMaxSlots];
  bool on[kMaxSlots] = {};
  std::uint64_t next_generation = 1;
  unsigned full = 0;

  Ctl* find(std::uint32_t id) noexcept {
    for (unsigned i = 0; i < kMaxSlots; ++i)
      if (on[i] && slot[i].id == id) return &slot[i];
    return nullptr;
  }
  Ctl* take(std::uint32_t id) noexcept {
    if (Ctl* c = find(id)) return c;
    for (unsigned i = 0; i < kMaxSlots; ++i)
      if (!on[i]) {
        on[i] = true, slot[i] = Ctl{}, slot[i].id = id, slot[i].generation = next_generation++;
        return &slot[i];
      }
    ++full;
    return nullptr;
  }
  bool retire(std::uint32_t id) noexcept {
    for (unsigned i = 0; i < kMaxSlots; ++i)
      if (on[i] && slot[i].id == id) {
        on[i] = false, slot[i] = Ctl{};
        return true;
      }
    return false;
  }
  unsigned live() const noexcept {
    unsigned n = 0;
    for (unsigned i = 0; i < kMaxSlots; ++i) n += on[i] ? 1u : 0u;
    return n;
  }
};

// ---------------------------------------------------------------- the clip bytes (J3D1 bck1, one ANK1 block), checked before the native loader sees them
inline std::uint32_t be32(const unsigned char* p) noexcept { return std::uint32_t(p[0]) << 24 | std::uint32_t(p[1]) << 16 | std::uint32_t(p[2]) << 8 | p[3]; }
inline std::uint16_t be16(const unsigned char* p) noexcept { return std::uint16_t(p[0] << 8 | p[1]); }
inline std::uint32_t crc32(const unsigned char* p, std::size_t n) noexcept {
  std::uint32_t c = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < n; ++i) {
    c ^= p[i];
    for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}
inline constexpr std::uint32_t kBckCrc32Value = 0xF92A19F6u;  // CRC-32 (IEEE) of the 45024 shipped FIX02 bytes; SHA-256 f96356fb...1524 is checked by the CI

enum class Bck : std::uint8_t { Ok = 0, Size, Crc, Magic, Blocks, Tag, Loop, Frames, Joints, Tables, Count };
inline const char* name(Bck b) noexcept {
  static constexpr const char* n[] = {"OK", "SIZE", "CRC", "MAGIC", "BLOCKS", "TAG", "LOOP", "FRAMES", "JOINTS", "TABLES"};
  return unsigned(b) < unsigned(Bck::Count) ? n[unsigned(b)] : "?";
}
// Structure of J3DAnmTransformKeyData (libs/JSystem J3DAnimation.h): +0x08 loop, +0x09 rotation shift, +0x0A frames, +0x0C joints, +0x0E/10/12 S/R/T
// counts, +0x14/18/1C/20 table offsets (from the block). Every key table of every joint must stay inside its pool.
inline Bck check_bck_layout(const unsigned char* d, std::size_t n) noexcept {
  if (!d || n < 0x44u) return Bck::Size;
  if (be32(d) != 0x4A334431u || be32(d + 4) != 0x62636B31u || be32(d + 8) != n) return Bck::Magic;  // "J3D1" "bck1" file size
  if (be32(d + 0x0C) != 1u) return Bck::Blocks;
  const unsigned char* b = d + 0x20;
  if (be32(b) != 0x414E4B31u || 0x20u + be32(b + 4) != n) return Bck::Tag;  // "ANK1" filling the file
  if (b[8] != 0) return Bck::Loop;                                           // once, like BCK_TN2B_DIE
  if (be16(b + 0x0A) != kBckFrames) return Bck::Frames;
  const unsigned joints = be16(b + 0x0C);
  if (joints != kBckJoints) return Bck::Joints;
  const std::uint32_t sc = be16(b + 0x0E), rc = be16(b + 0x10), tc = be16(b + 0x12);
  const std::uint32_t jt = be32(b + 0x14), so = be32(b + 0x18), ro = be32(b + 0x1C), to = be32(b + 0x20);
  const std::uint32_t blk = be32(b + 4);
  if (jt + joints * 54u > blk || so + sc * 4u > blk || ro + rc * 2u > blk || to + tc * 4u > blk || (so & 3u) || (ro & 1u) || (to & 3u)) return Bck::Tables;
  for (unsigned j = 0; j < joints; ++j)
    for (unsigned k = 0; k < 3; ++k)  // scale x,y,z / rotation x,y,z / translation x,y,z share one J3DAnmKeyTableBase each (max frame, offset, type)
      for (unsigned c = 0; c < 3; ++c) {
        const unsigned char* t = b + jt + j * 54u + c * 18u + k * 6u;
        if (jt + j * 54u + c * 18u + k * 6u + 6u > blk) return Bck::Tables;
        const std::uint32_t keys = be16(t), off = be16(t + 2), type = be16(t + 4);
        const std::uint32_t per = keys == 1 ? 1u : (type == 0 ? 3u : 4u);
        const std::uint32_t pool = k == 0 ? sc : k == 1 ? rc : tc;
        if (keys == 0 || type > 1 || off + keys * per > pool) return Bck::Tables;
      }
  return Bck::Ok;
}
// The production check: exactly the shipped file (size, CRC-32), then its J3D structure.
inline Bck check_bck(const unsigned char* d, std::size_t n) noexcept {
  if (!d || n != kBckBytes) return Bck::Size;
  if (crc32(d, n) != kBckCrc32Value) return Bck::Crc;
  return check_bck_layout(d, n);
}
}  // namespace dkr
