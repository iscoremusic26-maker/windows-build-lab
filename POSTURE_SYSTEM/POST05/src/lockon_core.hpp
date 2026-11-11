#pragma once
// LIGHT FADES TWICE 0.12.3 FIX03 P1-4: "Fijacion de objetivos estilo Sekiro" (mod option, persisted by the host's ConfigService).
// PURE: standard library only, no game type, no address. lockon_adapter.inc reads the frame's pad and the native attention (dAttention_c) into In,
// calls step() ONCE per mod_update (= once per simulation tick, after mDoCPd_c::read and before every process) and applies Out.
//
// Native facts [S src/d/d_attention.cpp, src/m_Do/m_Do_controller_pad.cpp @40457c6; B dusklight.exe 2.0.3 import library]:
//  * The lock button is the LOGICAL L: mDoCPd_c::LRlockCheck turns the analog trigger into mHoldLockL/mTrigLockL with hysteresis 0.9 on / 0.6 off,
//    storing the hold; the NEXT read derives the trigger edge from that stored hold. Whatever physical button the user mapped to L is this signal.
//  * dAttention_c::Run (once per tick, in the play scene's execute) reads padLockButton() = mHoldLockL. HOLD option: LOCK while held, RELEASE on
//    release (10 ticks, a new press inside it switches with nextAttention), then NONE. SWITCH option: a press locks, the lock stays released; a new
//    press cycles (nextAttention) and holding 15 ticks releases.
//  * In LOCK the target is mLockOnList[mLockOnOffset] (LockonTarget(0)); a list rebuild (makeList) moves that entry to index 0. nextAttention itself
//    switches by moving mLockOnOffset. mType == fopAc_attn_BATTLE_e marks an enemy entry.
// What this module does with the option ON, and ONLY while an ENEMY is locked: the lock survives the release of L (HOLD option: the attention sees
// L still held), a new press of L releases it (never a cycle), and a deliberate flick of the right stick moves the lock to the nearest valid enemy on
// that side of the screen (one target per flick: engage 0.70, re-arm below 0.30). Option OFF, menus, events, pause, wolf, horse: nothing is written.
#include <cstdint>

namespace lockon {
inline constexpr const char* kVersion = "0.12.3";
inline constexpr float kOn = 0.9f, kOff = 0.6f;                    // fapGmHIO mLROnValue / mLROffValue (f_ap_game.cpp)
inline constexpr float kStickEngage = 0.70f, kStickRearm = 0.30f;  // one target per deliberate right-stick flick
inline constexpr float kMinScreenGap = 0.02f;                      // a candidate must really be on that side of the current target
inline constexpr std::uint32_t kTapFrames = 10;                    // a quick tap: the attention locks on the press frame and sees the release next
inline constexpr unsigned kMaxCand = 8;                            // dAttention_c::mLockOnList[8]

enum class St : std::uint8_t { Idle = 0, Held, Toggled, Release, Count };
inline const char* name(St s) noexcept {
  static constexpr const char* n[] = {"IDLE", "HELD", "TOGGLED", "RELEASE"};
  return unsigned(s) < unsigned(St::Count) ? n[unsigned(s)] : "?";
}
enum class Mode : std::uint8_t { Hold = 0, Switch = 1 };

struct In {
  bool enabled = false;  // the mod option (ConfigService var), as last read
  bool safe = false;     // gameplay: human Link on foot, alive, no event / pause / menu window, attention on PAD_1
  float trigger = 0;     // analog L of this frame (mTriggerLeft)
  float cx = 0;          // right stick X of this frame (mCStickPosX)
  bool locked = false;   // dAttention_c::mAttnStatus == EState_LOCK (the previous Run)
  bool enemy = false;    // the locked entry is a BATTLE target (never an NPC, an object, a talk target)
  Mode mode = Mode::Hold;
};
struct Out {
  bool write = false, hold = false, trig = false;  // the logical L the attention reads THIS frame (only when write)
  bool mute_stick = false;                         // the right stick is consumed by the target change: neutral for this frame
  int switch_dir = 0;                              // -1 left, +1 right (one per flick)
  bool force_release = false;                      // SWITCH option: the cancelling press would cycle natively; the lock is released instead
};
struct Counters {
  std::uint64_t frames = 0, held = 0, toggled = 0, cancelled = 0, lost = 0, flicks = 0, switched = 0, no_candidate = 0, option_off = 0, unsafe = 0;
  std::uint64_t synth_frames = 0, muted_frames = 0, forced_releases = 0;
};

class Toggle {
 public:
  Counters c{};
  St st = St::Idle;
  bool real = false;        // own hysteresis of the REAL analog L: the stored mHoldLockL may be the one this module wrote last frame
  bool stick_armed = true;  // a flick moves once; the stick must come back near the centre first
  std::uint32_t since_edge = ~0u;  // frames since the last real press (a tap whose lock is seen only after the release)

  Out step(const In& in) noexcept {
    Out o;
    ++c.frames;
    const bool prev = real;
    real = in.trigger > kOn ? true : (in.trigger < kOff ? false : real);
    const bool edge = real && !prev;
    since_edge = edge ? 0u : (since_edge == ~0u ? ~0u : since_edge + 1u);
    if (!in.enabled || !in.safe) {
      if (st == St::Held || st == St::Toggled) ++(in.enabled ? c.unsafe : c.option_off);
      st = St::Idle;  // native from this very frame: nothing synthesized, nothing muted, no retained target
      stick_armed = true;
      return o;
    }
    switch (st) {
      case St::Idle:
        // Acquisition is always native (the real press reached the attention untouched). Only an ENEMY lock becomes Sekiro-style.
        if (in.locked && in.enemy) {
          if (real) {
            st = St::Held, ++c.held;
          } else if (in.mode == Mode::Switch || since_edge <= kTapFrames) {
            st = St::Toggled, ++c.toggled;  // a tap (or the SWITCH option's own lock): from here the lock stays and the next press releases
          }
        }
        if (st == St::Idle) return o;
        break;
      case St::Held:
        if (!in.locked || !in.enemy) {
          st = St::Idle;
          return o;
        }
        if (!real) st = St::Toggled, ++c.toggled;  // released: the lock stays
        break;
      case St::Toggled:
        if (!in.locked || !in.enemy) {  // target gone / out of reach / event: the attention decided; nothing is held for it
          ++c.lost;
          st = St::Idle;
          stick_armed = true;
          return o;
        }
        if (edge) {  // the second press: release, and hide that press until L comes back up (it must never relock or cycle)
          ++c.cancelled;
          st = St::Release;
          o.write = true, o.hold = false, o.trig = false;
          if (in.mode == Mode::Switch) o.force_release = true, ++c.forced_releases;
          ++c.synth_frames;
          return o;
        }
        break;
      case St::Release:
        o.write = true, o.hold = false, o.trig = false;
        ++c.synth_frames;
        if (!real) st = St::Idle;  // this frame still hides it; LRlockCheck then reads a released L
        return o;
      default:
        st = St::Idle;
        return o;
    }
    // Held or Toggled: the right stick belongs to the target change.
    if (st == St::Toggled) {
      o.write = true, o.trig = false;
      o.hold = in.mode == Mode::Hold;  // HOLD option: the attention sees L still held; SWITCH option: native lock already persists released
      ++c.synth_frames;
    }
    o.mute_stick = true;
    ++c.muted_frames;
    const float ax = in.cx < 0 ? -in.cx : in.cx;
    if (!stick_armed && ax <= kStickRearm) stick_armed = true;
    if (stick_armed && ax >= kStickEngage) {
      stick_armed = false;
      o.switch_dir = in.cx > 0 ? 1 : -1;
      ++c.flicks;
    }
    return o;
  }
  void reset() noexcept {
    st = St::Idle;
    real = false;
    stick_armed = true;
    since_edge = ~0u;
  }
};

// One candidate of the native lock list as seen from the camera: x = horizontal screen coordinate (side / depth), valid = an enemy in front.
struct Cand {
  bool valid = false, current = false;
  float x = 0;
};
// The nearest valid enemy strictly on the requested side of the current target; -1 when there is none (the current lock then stays, no jerk).
inline int choose(const Cand* c, unsigned n, int dir) noexcept {
  int cur = -1;
  for (unsigned i = 0; i < n && i < kMaxCand; ++i)
    if (c[i].current) cur = int(i);
  if (cur < 0 || dir == 0) return -1;
  int best = -1;
  float bestd = 0;
  for (unsigned i = 0; i < n && i < kMaxCand; ++i) {
    if (int(i) == cur || !c[i].valid) continue;
    const float d = (c[i].x - c[cur].x) * float(dir);
    if (d <= kMinScreenGap) continue;
    if (best < 0 || d < bestd) best = int(i), bestd = d;
  }
  return best;
}
}  // namespace lockon
