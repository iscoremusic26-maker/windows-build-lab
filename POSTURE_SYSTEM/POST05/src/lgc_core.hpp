#pragma once
#include <cstdint>
// LIGHT FADES TWICE 0.13.14 (DKN01 FIX05) LGC01: Link's guard cancel of an ORDINARY sword cut. PURE core: standard library only, no game type, no address.
// src/lgc_adapter.inc feeds it what the live Link says (procedure, cut type, frame, the cut's own AT frames, R / B as the manual shield sampled them) and
// executes its verdict with Link's OWN native end-of-cut transition. tests/lgc_lab.cpp runs this very file.
//
// Native facts [S = dusklight 2.0.3 40457c6 src/d/actor/d_a_alink_cut.inc, d_a_alink.cpp, d_a_alink_guard.inc, d_a_alink_HIO_data.inc; B = dusklight.exe 2.0.3]:
//  * procCutNormalInit (cut.inc:912): the ordinary cuts NM_VERTICAL / NM_LEFT / NM_RIGHT / COMBO_STAB / NM_STAB; field_0x3478 = mAttackStartFrame,
//    field_0x347c = mAttackEndFrame, field_0x3484 = mCutAnm.mCancelFrame. HIO (HIO_data.inc:251-326): vertical clip 2..27 at 1.2, AT 5..10, cancel 16;
//    left 2..27 at 1.2, AT 7..13; right 3..27 at 1.2, AT 7..12.5; stabs 3/4..31/27 at 1.1, AT 3..10, cancel 17. The lunge (mNormalSpeed = field_0x3480,
//    3..10) is set at the AT start and decelerates 2.2 per tick (move HIO mDeceleration).
//  * procCutNormal (cut.inc:1033): the AT (RFLG0_UNK_2 + the swing sound + the lunge) only while field_0x3478 <= frame < field_0x347c. Before the cancel
//    frame nothing but a buffered combo can leave the cut; after it, checkCutCancelNextMode(3) -> checkNextAction(1) leaves it ONLY for a button action
//    or a stick input (d_a_alink.cpp:12326: param 1 without stick returns 0). With R held and the stick still, the cut runs to its clip end (frame 27 at
//    1.2 = ~21 ticks): THE R -> guard latency of the human log (R_DOWN -> GUARD_ENABLED 7-14 ticks after a cut). At the end: resetCombo(TRUE);
//    checkNextAction(0) -> procAtnActorWaitInit / procAtnMoveInit ... (the native morph of the next clip from the current pose).
//  * setShieldGuard (guard.inc:186): the guard flag needs checkGuardAccept() = mode flag 0x80 (none inside the cut) and checkGuardActionChange() = the
//    lock-on (LazyTweaks + this mod's manual shield add R). daAlink_c::execute: setStickData -> checkDamageAction -> setShieldGuard -> procedure -> setCollision.
//  * The rebound (PROC_CUT_REVERSE), the guard break, the finishers (PROC_CUT_FINISH: combo finisher / Mortal Draw), the jump / spin / head cuts are other
//    procedures: never touched here.
// LGC01 (approved scope: Link in general, not tied to any enemy):
//  * Region PREP (frame < AT start): the swing has not been committed: a NEW R press (after the cut began) cancels it now. No AT ever existed: no damage,
//    no posture, no impact sound, no enemy parry can come from it. The queued combo (FLG2_COMBO_RESERB) is dropped by resetCombo(TRUE): never replayed.
//  * Region ACTIVE (AT start <= frame < AT end): committed. The R intent is kept (bounded by R staying held) and executed at the first recovery tick.
//  * Region RECOVERY (frame >= AT end, the lunge spent): the AT is over; the cancel happens now instead of at the clip end (the dead tail). The FIRST
//    recovery call always runs natively: it is the one that evaluates the collision of the last AT tick (changeCutReverseProc -> checkCutReverseAt:
//    an enemy's parry rebound, PROC_CUT_REVERSE); a cancel there would erase that rebound. Only a later recovery call may cancel (rule E).
//  * The transition is the cut's OWN natural end, earlier: resetCombo(TRUE) + checkNextAction(0) (no pose, frame, position or angle written; the native morph
//    blends from the current pose). Fail-closed: a context that is not plainly safe (penalty, damage, event, no shield drawn, no lock-on, airborne, not the
//    sword, a non-ordinary cut) keeps the native cut untouched.
//  * R+B (a new B while R, pressed during this cut, is held): the Shield Attack. The B is never executed as a cut: in RECOVERY its tick skips the cut's own
//    procedure once (no combo chain), and the cancel of the next tick offers the native Shield Attack site (setRStatus -> spActionTrigger, the manual shield's
//    own decide_bash) with that buffered B, ONCE, within kBashBufferTicks. Without the native offer (no skill, no lock...) the B is dropped, never a cut.
namespace lgc {
inline constexpr const char* kVersion = "0.13.14";
inline constexpr const char* kTag = "LGC01";

namespace policy {
inline constexpr unsigned kBashBufferTicks = 8;     // [PROVISIONAL] a buffered R+B lives at most this long (covers the longest AT: stab 3..10 at 1.1 = 6.4 ticks)
inline constexpr float kRecoverSpeedMax = 0.5f;     // the lunge must be spent before a recovery cancel (native 2.2/tick deceleration: <= 5 ticks after the AT start)
inline constexpr unsigned kInstanceGapTicks = 30;   // a cut procedure not seen for this long is another cut (hitstop pauses are 2-5 ticks)
inline constexpr unsigned kHoldMax = 1;             // ticks a cut's own procedure may be skipped for one R+B (never a frozen cut)
}  // namespace policy

// daPy_py_c::CUT_TYPE_* (pinned by static_assert in the adapter) of the five ordinary cuts procCutNormalInit sets.
inline constexpr int kCutNmVertical = 0x01, kCutNmStab = 0x02, kCutNmRight = 0x03, kCutNmLeft = 0x04, kCutComboStab = 0x21;
inline bool ordinary_cut(int cut) noexcept { return cut == kCutNmVertical || cut == kCutNmStab || cut == kCutNmRight || cut == kCutNmLeft || cut == kCutComboStab; }

enum class Region : std::uint8_t { Prep = 0, Active, Recovery };
inline const char* name(Region r) noexcept {
  static constexpr const char* n[] = {"PREP_BEFORE_AT", "AT_ACTIVE_COMMITTED", "RECOVERY_AT_OVER"};
  return unsigned(r) < 3 ? n[unsigned(r)] : "?";
}
inline Region region(float frame, float at_start, float at_end) noexcept { return frame < at_start ? Region::Prep : frame < at_end ? Region::Active : Region::Recovery; }

// What the adapter reads, once per call of the cut's procedure (PRE).
struct View {
  bool cut_normal = false;   // mProcID == PROC_CUT_NORMAL
  int cut = -1;              // getCutType()
  int combo = 0;             // getCutCount(): a new combo cut is a new cut instance
  bool safe = false;         // world, manual shield READY, shield drawn, no damage timer, no break penalty, no special mode
  bool human = false, sword = false, ground = false, lock = false;
  bool r_held = false;       // the manual shield's R sample of THIS execute
  std::uint64_t r_down_tick = 0;  // tick of the last R rising edge (the manual shield's sampler)
  bool b_trig = false;       // a NEW B this execute (before any consumption)
  float frame = 0, at_start = 0, at_end = 0, speed = 0;
  std::uint64_t tick = 0;
};
enum class Verdict : std::uint8_t { Native = 0, Buffer, Hold, Cancel, Count };
inline const char* name(Verdict v) noexcept {
  static constexpr const char* n[] = {"NATIVE", "BUFFER_COMMITTED", "HOLD_ONE_TICK_RB", "CANCEL_TO_GUARD"};
  return unsigned(v) < unsigned(Verdict::Count) ? n[unsigned(v)] : "?";
}
enum class Why : std::uint8_t { None = 0, NotOrdinary, NoIntent, Unsafe, NoLock, Airborne, NotSword, NotHuman, Lunge, LastAtContact, Count };
inline const char* name(Why w) noexcept {
  static constexpr const char* n[] = {"NONE", "NOT_AN_ORDINARY_CUT", "NO_R_INTENT", "UNSAFE_CONTEXT", "NO_LOCK_ON", "AIRBORNE", "NOT_THE_SWORD", "NOT_HUMAN_LINK", "LUNGE_NOT_SPENT", "LAST_AT_CONTACT_NATIVE_CALL"};
  return unsigned(w) < unsigned(Why::Count) ? n[unsigned(w)] : "?";
}
struct Out {
  Verdict v = Verdict::Native;
  Why why = Why::None;
  Region region = Region::Prep;
  bool bash = false;             // Cancel: offer the native Shield Attack site with the buffered B, once
  std::uint64_t intent_age = 0;  // ticks since the R press that asked for the guard
};
struct Ctl {
  bool seen = false, intent = false, bash = false, seen_recovery = false;
  int cut = -1, combo = -1;
  std::uint64_t last_tick = 0, cut_tick = 0, intent_tick = 0, bash_tick = 0;
  float last_frame = 0;
  unsigned holds = 0;
  std::uint64_t cuts = 0, intents = 0, cancels[3] = {}, buffered = 0, holds_n = 0, bashes = 0, bash_expired = 0, refused[unsigned(Why::Count)] = {}, released = 0;
  void reset() noexcept { seen = intent = bash = seen_recovery = false, holds = 0; }
  // One call of the cut's procedure. Never cancels twice (the cut is gone after a Cancel); never acts on anything but an ordinary cut.
  Out step(const View& v) noexcept {
    Out o;
    if (!v.cut_normal || !ordinary_cut(v.cut)) {
      reset();
      o.why = Why::NotOrdinary;
      return o;
    }
    // a new cut instance: a new cut type / combo step, the clip restarted, or a cut not seen for a long time (a native hitstop freezes Link 2-5 ticks while
    // the simulation tick runs on: a pause alone never starts a new instance)
    if (!seen || v.cut != cut || v.combo != combo || v.frame + 0.01f < last_frame || v.tick > last_tick + policy::kInstanceGapTicks) {
      seen = true, intent = bash = false, holds = 0, cut = v.cut, combo = v.combo, cut_tick = v.tick, seen_recovery = false, ++cuts;
    }
    last_tick = v.tick, last_frame = v.frame;
    o.region = region(v.frame, v.at_start, v.at_end);
    const bool native_recovery_ran = seen_recovery;  // a previous call of THIS cut already ran natively in the recovery (the last AT contact evaluated)
    if (o.region == Region::Recovery) seen_recovery = true;  // this call runs natively unless it is a later recovery call (cancel / hold need one before)
    // the R intent: a NEW R press after this cut began (an R already held when the B was pressed asked for this very cut), alive while R stays held
    if (v.r_held && v.r_down_tick >= cut_tick && !intent) intent = true, intent_tick = v.r_down_tick, ++intents;
    if (!v.r_held) {
      if (intent) ++released;
      intent = bash = false;
    }
    if (intent && v.b_trig && !bash) bash = true, bash_tick = v.tick;  // R+B: the Shield Attack wish (never a cut)
    if (bash && v.tick > bash_tick + policy::kBashBufferTicks) bash = false, ++bash_expired;
    o.intent_age = intent && v.tick >= intent_tick ? v.tick - intent_tick : 0;
    if (!intent) return o.why = Why::NoIntent, o;
    Why bad = Why::None;
    if (!v.human) bad = Why::NotHuman;
    else if (!v.safe) bad = Why::Unsafe;
    else if (!v.sword) bad = Why::NotSword;
    else if (!v.ground) bad = Why::Airborne;
    else if (!v.lock) bad = Why::NoLock;
    if (bad != Why::None) {  // the context does not admit a guard: the intent dies, the native cut goes on untouched
      ++refused[unsigned(bad)];
      intent = bash = false;
      return o.why = bad, o;
    }
    if (o.region == Region::Active) return ++buffered, o.v = Verdict::Buffer, o;
    if (o.region == Region::Recovery && !native_recovery_ran) return ++buffered, o.v = Verdict::Buffer, o.why = Why::LastAtContact, o;
    if (o.region == Region::Recovery && (v.speed > policy::kRecoverSpeedMax || v.speed < -policy::kRecoverSpeedMax))
      return ++buffered, o.v = Verdict::Buffer, o.why = Why::Lunge, o;
    // a tick carrying a NEW B never cancels (checkNextAction(0) would read that B as a cut): in RECOVERY the cut's own procedure is skipped once (no combo
    // chain from that B), in PREP it runs (the B only reserves a combo: resetCombo drops it); the next tick cancels with the buffered B (the Shield Attack)
    if (v.b_trig) {
      if (o.region == Region::Recovery && holds < policy::kHoldMax) return ++holds, ++holds_n, o.v = Verdict::Hold, o;
      return ++buffered, o.v = Verdict::Buffer, o;
    }
    o.v = Verdict::Cancel, o.bash = bash;
    ++cancels[unsigned(o.region)];
    bashes += bash;
    reset();
    return o;
  }
};
}  // namespace lgc
