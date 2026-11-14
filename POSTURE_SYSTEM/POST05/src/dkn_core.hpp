#pragma once
#include <cstddef>
#include <cstdint>
// LIGHT FADES TWICE 0.13.9 DARKNUT MINIBOSS (DKN01). PURE core: standard library only, no game type, no SDK, no address.
// src/dkn_adapter.inc feeds it what the native daB_TN_c says and applies its verdicts through audited writers; tests/dkn_lab.cpp runs this very file.
// Every number the user approved lives here, in one place. Numbers that were NOT approved are marked [PROVISIONAL] and are configurable constants.
//
// Native facts this core is built on [S = TwilitRealm/dusklight@40457c6 src/d/actor/d_a_b_tn.cpp, B = dusklight.exe 2.0.3 ffa99203..., measured]:
//  * action() runs damage_check() FIRST and only then writes field_0x700 = l_HIO.field_0x24 (360); setDamage (called only from damage_check, B: 3 real
//    call sites) is the only reader of field_0x700. A per-actor threshold written in the damage_check PRE is therefore exactly the one setDamage reads.
//  * setDamage: health = 100; cc_at_check (health -= attack power); phase 1 (mActionMode1 < 8): mNextBreakPart += 1 (sword) or 2 (bomb / iron ball) and
//    ACT_DAMAGEH; phase 2: field_0x6fc += 100 - health (or +50 near the threshold with mCutFlag), ACT_ENDING at field_0x6fc >= field_0x700, else ACT_DAMAGEL.
//  * executeDamageH: mNextBreakPart >= 12 -> ACT_CHANGEDEMO; else setBreakPart(mNextBreakPart). setBreakPart(i) breaks parts until (i - already broken)
//    reaches 0; for mType 1 the head part (4) also bumps mNextBreakPart and breaks part 5: a call with i BELOW the broken count never reaches 0 and breaks
//    EVERY remaining part. armor_call() below never asks for that.
//  * The weapon AT spl the Darknut uses (setSwordAtBreak(1) = 0xA, the shield/kick source = 9) is a native GUARD BREAK for Link (procGuardBreakInit in
//    daAlink_c::checkDamageAction for spl 9/10/11). The approved Table B wants posture, not an instant break: the eligible Darknut uses spl 1 (a LARGE
//    attack: native guard slip, POWER4 shock) and the break comes from Link's posture at 100 (LNK05).
namespace dkn {
inline constexpr const char* kVersion = "0.13.9";
inline constexpr const char* kTag = "DKN01";
inline constexpr unsigned kSlots = 8;

// ---------------------------------------------------------------- approved numbers (do not change)
namespace policy {
inline constexpr int kHp1Max = 420;          // phase 1 own HP reserve (option B)
inline constexpr int kRes2Max = 480;         // phase 2 resistance per life (native accumulator units), again 480 after the ONE resurrection
inline constexpr int kPosture1Max = 120, kPosture2Max = 130;
inline constexpr unsigned kRegenDelayTicks = 60;  // eligible ticks after the last gain
inline constexpr int kRegen1[5] = {10, 7, 4, 2, 1};  // pts/s by band (>75, 50-75, 25-50, 10-25, <=10 %)
inline constexpr int kRegen2[5] = {12, 8, 5, 2, 1};
inline constexpr unsigned kBreakTicks = 90;   // maximum stagger window (eligible ticks)
inline constexpr unsigned kBreakHits = 3;     // ... or this many unique effective hits of Link
inline constexpr int kLinkMax = 100;          // Link's posture maximum (unchanged)
// Table A (Darknut gains). Phase 1 / phase 2.
inline constexpr int kGainSwordHp1 = 4, kGainBlocked1 = 2, kGainIron1 = 25;
inline constexpr int kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;
// Passivity thresholds.
inline constexpr unsigned kPassiveShieldTicks = 38;  // 1.25 s
inline constexpr unsigned kPassiveIdleTicks = 75;    // 2.5 s
inline constexpr unsigned kReviveFrames = 88;        // FIX02, measured
// ---------------------------------------------------------------- [PROVISIONAL] not approved numerically: configurable, logged as provisional
inline constexpr int kComboPct = 40;                  // [PROVISIONAL] share of the native AI's ordinary attack starts that become a combo
inline constexpr unsigned kComboCooldownTicks = 120;  // [PROVISIONAL] after a combo, no new combo for 4 s (native single attacks still allowed)
inline constexpr unsigned kPassiveCooldownTicks = 240;  // [PROVISIONAL] 8 s between two passivity responses of one Darknut
inline constexpr unsigned kGuardRecoverTicks = 45;    // [PROVISIONAL] native forced guard (mTimer10) after a stagger: the next frontal hits are blocked
inline constexpr unsigned kRecoverMaxTicks = 60;      // watchdog of the visual recovery
inline constexpr unsigned kComboWatchdogTicks = 600;
inline constexpr float kComboLeaveDist = 700.0f;      // Link farther than this at a move boundary: the combo ends at that boundary
inline constexpr float kPassiveReach1 = 220.0f, kPassiveReach2 = 230.0f;  // shield bash / kick reach (model z 148 / 164 x scale 1.3 + bodies)
inline constexpr int kFrontArc = 0x3000;
}  // namespace policy

// ---------------------------------------------------------------- native constants (pinned by ci/dkn_native_audit.py)
namespace native {
inline constexpr int kActWaitH = 2, kActChaseH = 3, kActAttackH = 4, kActAttackShieldH = 5, kActGuardH = 6, kActDamageH = 7, kActChangeDemo = 8, kActChaseL = 9,
                     kActAttackL = 0xA, kActAttackShieldL = 0xB, kActGuardL = 0xC, kActDamageL = 0xD, kActEnding = 0xE, kActYoroke = 0xF;
inline constexpr int kBckTnaAtackA = 0x05, kBckTnaAtackB = 0x06, kBckTnaAtackShield = 0x08, kBckTnaDamageL = 0x09, kBckTnaDamageLast = 0x0A,
                     kBckTnbAtackA = 0x14, kBckTnbAtackB = 0x15, kBckTnbAtackShield = 0x16, kBckTnbDownSippu = 0x1B, kBckTnbWait = 0x2A;
inline constexpr float kEndTnaDamageL = 37.0f, kEndTnbDownSippu = 34.0f;  // frame counts (B_tn archive, measured)
inline constexpr int kSplLarge = 1;  // dCcG_At_Spl 1: checkLargeAttack, not a guard break
inline constexpr int kArmorParts = 13;
inline constexpr int kBreakPartNo[13] = {6, 11, 8, 1, 10, 0, 9, 7, 4, 5, 12, 2, 3};  // setBreakPart order
// Link procedures (daAlink_c::daAlink_PROC, pinned)
inline constexpr int kProcServiceWait = 1, kProcTiredWait = 2, kProcWait = 3, kProcMove = 4, kProcAtnMove = 5, kProcAtnActorWait = 6, kProcAtnActorMove = 7,
                     kProcWaitTurn = 8, kProcMoveTurn = 9, kProcStepMove = 0x1B, kProcCrouch = 0x1C, kProcGuardSlip = 0x1D, kProcGuardAttack = 0x1E,
                     kProcGuardBreak = 0x1F, kProcDamage = 0x32, kProcLargeDamageUp = 0x33, kProcLandDamage = 0x34;
}  // namespace native

// ---------------------------------------------------------------- the moves (one per animated action)
enum class Move : std::uint8_t { F1A = 0, F1B, F1Shield, F2A, F2B, F2B1, F2C, Count };
inline const char* name(Move m) noexcept {
  static constexpr const char* n[] = {"F1_A_LATERAL_0x05", "F1_B_VERTICAL_0x06", "F1_SHIELD_0x08", "F2_A_THRUST_0x14", "F2_B_DOUBLE_0x15", "F2_B1_FIRST_CUT_0x15",
                                      "F2_C_KICK_0x16"};
  return unsigned(m) < unsigned(Move::Count) ? n[unsigned(m)] : "?";
}
enum class Outcome : std::uint8_t { Parry = 0, Block, Direct, Count };
inline const char* name(Outcome o) noexcept {
  static constexpr const char* n[] = {"PARRY", "BLOCK", "DIRECT"};
  return unsigned(o) < 3 ? n[unsigned(o)] : "?";
}
// Per move: clip, the impact frames (weapon tip / foot speed peak at full reach, measured on tn2.bmd + B_tn BCKs by the forward kinematics in
// tests/data/dkn_swing_profile.txt), the AT window of each impact, the start/cut frame ranges the rhythm planner may use, the reach for distance
// correction, the native action/mode that plays it, and the approved Table A / Table B values.
struct MoveInfo {
  Move m;
  int bck;
  bool phase2;
  std::uint8_t impacts;
  float impact[2];
  float at_open[2], at_close[2];
  float s_min, s_max, c_min, c_max;  // start frame range (wind-up kept), cut frame range (follow-through kept)
  float end;                          // clip frame count
  float reach;                        // distance the impact reaches (combo distance correction)
  int act, mode;                      // native action / mode that plays the clip
  int dk_parry[2];                    // Table A: Darknut posture when Link PARRIES impact 0 / 1
  int link_block, link_direct;        // Table B: Link posture on a normal block / a direct impact (parry: 0)
  float morph;                        // blend frames when the move starts inside a combo
};
inline constexpr MoveInfo kMoves[unsigned(Move::Count)] = {
    // F1 A lateral (TNA_ATACK_A): sweep 19-24, full reach at 22, AT 18..29 (native), follow-through ends 26
    {Move::F1A, native::kBckTnaAtackA, false, 1, {22.0f, 0}, {18.0f, 0}, {29.0f, 0}, 6.0f, 13.0f, 28.0f, 40.0f, 82.0f, 260.0f, native::kActAttackH, 1, {9, 0}, 20, 30, 4.0f},
    // F1 B vertical (TNA_ATACK_B): smash 23-25, ground contact 26 (native SWD_IMPCT + shock at 26), AT 22..30
    {Move::F1B, native::kBckTnaAtackB, false, 1, {26.0f, 0}, {22.0f, 0}, {30.0f, 0}, 8.0f, 17.0f, 31.0f, 50.0f, 82.0f, 280.0f, native::kActAttackH, 2, {11, 0}, 26, 36, 4.0f},
    // F1 shield bash (TNA_ATACK_SHIELD): thrust 5-7, AT 5..9 (mSphC)
    {Move::F1Shield, native::kBckTnaAtackShield, false, 1, {7.0f, 0}, {5.0f, 0}, {9.0f, 0}, 0.0f, 2.0f, 10.0f, 14.0f, 21.0f, 190.0f, native::kActAttackShieldH, 1, {14, 0}, 42, 50, 3.0f},
    // F2 A thrust (TNB_ATACK_A): lunge 8-10, full extension 10-11, AT 7..14
    {Move::F2A, native::kBckTnbAtackA, true, 1, {10.0f, 0}, {7.0f, 0}, {14.0f, 0}, 3.0f, 6.0f, 12.0f, 24.0f, 36.0f, 270.0f, native::kActAttackL, 1, {11, 0}, 23, 33, 3.0f},
    // F2 B double (TNB_ATACK_B): cut 1 peak 15-16 (follow-through ends at 20, speed minimum), re-cock 21-25, cut 2 peak 29-30. Native AT 13..38 split
    // into one window per cut so each cut is ONE independent block/impact/parry opportunity.
    {Move::F2B, native::kBckTnbAtackB, true, 2, {15.5f, 29.5f}, {13.0f, 21.0f}, {21.0f, 38.0f}, 5.0f, 11.0f, 33.0f, 50.0f, 55.0f, 240.0f, native::kActAttackL, 2, {9, 9}, 12, 22, 3.0f},
    // F2 B1: ONLY the first cut of TNB_ATACK_B. Cut at frame 20 (end of the cut-1 follow-through, before the re-cock of cut 2 starts at 21); AT closed at 20.
    {Move::F2B1, native::kBckTnbAtackB, true, 1, {15.5f, 0}, {13.0f, 0}, {20.0f, 0}, 5.0f, 11.0f, 20.0f, 20.0f, 55.0f, 240.0f, native::kActAttackL, 2, {11, 0}, 23, 33, 3.0f},
    // F2 C kick (TNB_ATACK_SHIELD): foot out 9-10, AT 8..12 (mSphC on joint 24)
    {Move::F2C, native::kBckTnbAtackShield, true, 1, {10.0f, 0}, {8.0f, 0}, {12.0f, 0}, 0.0f, 3.0f, 12.0f, 20.0f, 29.0f, 200.0f, native::kActAttackShieldL, 1, {15, 0}, 38, 45, 3.0f},
};
inline const MoveInfo& info(Move m) noexcept { return kMoves[unsigned(m) < unsigned(Move::Count) ? unsigned(m) : 0u]; }
// Table B: Link posture of one impact. A confirmed parry gives Link +0.
inline int link_gain(Move m, Outcome o) noexcept {
  const MoveInfo& i = info(m);
  return o == Outcome::Parry ? 0 : o == Outcome::Block ? i.link_block : i.link_direct;
}
// Table A: Darknut posture when Link parries impact k of move m.
inline int dk_parry_gain(Move m, unsigned k) noexcept { return info(m).dk_parry[k < 2 ? k : 0]; }

// ---------------------------------------------------------------- the eight approved combos (sequence + the user's eight recordings)
// peak_ms: the impact peaks of the ORIGINAL recordings (darknut_combo_audios, decoded without modification, envelope maxima at 5 ms; see
// tests/data/dkn_audio_peaks.txt). Only the DIFFERENCES between peaks are used: the silence before the first peak is never played.
struct Combo {
  const char* name;
  bool phase2;
  std::uint8_t n_moves;
  Move moves[7];
  std::uint8_t n_impacts;
  std::uint16_t peak_ms[9];
  std::uint16_t prompt_ms[9];  // the request's rounded marks (documentation / deviation report)
};
inline constexpr Combo kCombos[8] = {
    {"F1-C1", false, 5, {Move::F1B, Move::F1A, Move::F1B, Move::F1A, Move::F1B}, 5, {675, 1045, 1905, 2300, 3195}, {680, 1050, 1900, 2300, 3190}},
    {"F1-C2", false, 3, {Move::F1B, Move::F1B, Move::F1B}, 3, {530, 2130, 2840}, {580, 2130, 2840}},
    {"F1-C3", false, 5, {Move::F1A, Move::F1B, Move::F1B, Move::F1B, Move::F1A}, 5, {510, 980, 2100, 3240, 4045}, {570, 980, 2100, 3230, 4040}},
    {"F1-C4", false, 5, {Move::F1A, Move::F1B, Move::F1B, Move::F1A, Move::F1B}, 5, {520, 2035, 2740, 3480, 4635}, {520, 2040, 2760, 3470, 4630}},
    {"F2-C1", true, 4, {Move::F2B, Move::F2B, Move::F2B, Move::F2B1}, 7, {420, 700, 990, 1255, 1540, 1850, 2680}, {420, 700, 990, 1250, 1540, 1850, 2680}},
    {"F2-C2", true, 5, {Move::F2A, Move::F2A, Move::F2A, Move::F2B, Move::F2B}, 7, {605, 1485, 1695, 2815, 3105, 3390, 3690}, {600, 1490, 1700, 2840, 3100, 3380, 3680}},
    {"F2-C3", true, 7, {Move::F2A, Move::F2B, Move::F2A, Move::F2B, Move::F2A, Move::F2A, Move::F2A}, 9, {625, 1280, 1485, 2325, 2950, 3165, 3985, 4300, 4640},
     {630, 1280, 1490, 2320, 2950, 3160, 3980, 4290, 4630}},
    {"F2-C4", true, 4, {Move::F2C, Move::F2A, Move::F2B, Move::F2A}, 5, {640, 1520, 2310, 2520, 3535}, {640, 1520, 2310, 2520, 3530}},
};
inline unsigned impacts_of(const Combo& c) noexcept {
  unsigned n = 0;
  for (unsigned i = 0; i < c.n_moves; ++i) n += info(c.moves[i]).impacts;
  return n;
}

// ---------------------------------------------------------------- the rhythm planner (pure, deterministic)
// One segment per move. The model of the adapter is exact: every tick the executeAttack POST (1) switches to the next move when the frame reached the
// cut, then sets the rate of the current region; action() then plays (frame += rate). An impact is the tick whose play crosses its impact frame.
inline constexpr float kRateMin = 0.55f, kRateMax = 2.40f;
struct Seg {
  Move m = Move::F2A;
  float start = 0, cut = 0;        // cut < 0: no cut (the last move ends natively / B1 ends at its fixed cut)
  float rate_in = 1, rate_mid = 1, rate_out = 1;
};
struct Plan {
  Seg seg[7];
  std::uint8_t n = 0;
  float target_ticks[8] = {};      // audio intervals in ticks (30 Hz)
  bool clamped[8] = {};            // a rate hit [kRateMin, kRateMax]: the interval cannot be met (reported, never hidden)
};
inline float clampf(float v, float lo, float hi) noexcept { return v < lo ? lo : v > hi ? hi : v; }
inline Plan plan(const Combo& c) noexcept {
  Plan p;
  p.n = c.n_moves;
  for (unsigned i = 0; i < c.n_moves; ++i) {
    const MoveInfo& mi = info(c.moves[i]);
    p.seg[i].m = c.moves[i];
    p.seg[i].start = mi.s_min;  // the first move keeps its whole planned wind-up
    p.seg[i].cut = -1.0f;
  }
  unsigned imp = 0;
  for (unsigned i = 0; i < c.n_moves; ++i) {
    const MoveInfo& a = info(c.moves[i]);
    if (a.impacts == 2 && imp + 1 < c.n_impacts) {  // the two cuts of one B
      const float dt = (c.peak_ms[imp + 1] - c.peak_ms[imp]) * 0.03f;
      p.target_ticks[imp] = dt;
      float r = (a.impact[1] - a.impact[0]) / dt;
      if (r < kRateMin || r > kRateMax) p.clamped[imp] = true, r = clampf(r, kRateMin, kRateMax);
      p.seg[i].rate_mid = r;
    }
    imp += a.impacts;
    if (i + 1 >= c.n_moves) break;
    const MoveInfo& b = info(c.moves[i + 1]);
    const unsigned j = imp - 1;  // pair (impact j -> impact j+1) crosses the move boundary
    const float dt = (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03f;
    p.target_ticks[j] = dt;
    const float last = a.impact[a.impacts - 1], first = b.impact[0];
    const float fmin = (a.c_min - last) + (first - b.s_max), fmax = (a.c_max - last) + (first - b.s_min);
    float f = clampf(dt, fmin, fmax);  // frames to play at rate 1 if possible
    const float t = fmax > fmin ? (f - fmin) / (fmax - fmin) : 0.0f;
    p.seg[i].cut = a.c_min + t * (a.c_max - a.c_min);
    p.seg[i + 1].start = b.s_max - t * (b.s_max - b.s_min);
    float r = f / dt;
    if (r < kRateMin || r > kRateMax) p.clamped[j] = true, r = clampf(r, kRateMin, kRateMax);
    p.seg[i].rate_out = r, p.seg[i + 1].rate_in = r;
  }
  // B1: always ends at its fixed cut (frame 20), never reaching the second cut
  for (unsigned i = 0; i < p.n; ++i)
    if (p.seg[i].m == Move::F2B1) p.seg[i].cut = info(Move::F2B1).c_min;
  return p;
}
// ---------------------------------------------------------------- the continuous timeline of a plan (no drift, no quantization bias)
// Each segment is a piecewise-linear frame(u) (u = ticks since the segment started): rate_in to the first impact, rate_mid between the two cuts of a B,
// rate_out to the cut (or to the clip end for the last move). Every tick the adapter asks for the frame the timeline wants AFTER this tick's play and
// sets the J3D rate to exactly (wanted - current): impact ticks are the continuous impact times rounded up, never accumulated.
inline float seg_end_frame(const Seg& s) noexcept { return s.cut >= 0 ? s.cut : info(s.m).end; }
inline float seg_duration(const Seg& s) noexcept {
  const MoveInfo& m = info(s.m);
  const float last = m.impact[m.impacts - 1];
  float d = (m.impact[0] - s.start) / s.rate_in;
  if (m.impacts == 2) d += (m.impact[1] - m.impact[0]) / s.rate_mid;
  return d + (seg_end_frame(s) - last) / s.rate_out;
}
inline float seg_frame(const Seg& s, float u) noexcept {
  const MoveInfo& m = info(s.m);
  if (u <= 0) return s.start;
  const float t0 = (m.impact[0] - s.start) / s.rate_in;
  if (u <= t0) return s.start + u * s.rate_in;
  float f = m.impact[0], t = t0;
  if (m.impacts == 2) {
    const float t1 = (m.impact[1] - m.impact[0]) / s.rate_mid;
    if (u <= t + t1) return f + (u - t) * s.rate_mid;
    f = m.impact[1], t += t1;
  }
  const float out = f + (u - t) * s.rate_out, end = seg_end_frame(s);
  return out < end ? out : end;
}
struct Timeline {
  float T[8] = {};  // T[k] = start time of segment k; T[n] = end of the combo
  unsigned n = 0;
};
inline Timeline timeline(const Plan& p) noexcept {
  Timeline tl;
  tl.n = p.n;
  for (unsigned k = 0; k < p.n; ++k) tl.T[k + 1] = tl.T[k] + seg_duration(p.seg[k]);
  return tl;
}
// Where the timeline is at time t (ticks since the combo started). false = past the end of the combo.
inline bool locate(const Plan& p, const Timeline& tl, float t, unsigned& seg, float& frame) noexcept {
  if (t >= tl.T[tl.n]) return seg = tl.n ? tl.n - 1 : 0, frame = tl.n ? seg_end_frame(p.seg[tl.n - 1]) : 0, false;
  unsigned k = 0;
  while (k + 1 < tl.n && t >= tl.T[k + 1]) ++k;
  return seg = k, frame = seg_frame(p.seg[k], t - tl.T[k]), true;
}
inline constexpr float kTickRateMax = 3.0f;  // safety cap of one tick's J3D rate
// The frame a segment starts at when the switch tick began u ticks (u <= 0) before the segment's own start: its wind-up extrapolated backwards, so the
// first tick of the new clip plays at exactly rate_in (no partial tick, no visible hold at the switch).
inline float switch_frame(const Seg& s, float u) noexcept {
  if (u >= 0) return seg_frame(s, u);
  const float f = s.start + u * s.rate_in;
  return f > 0 ? f : 0.0f;
}

// ---------------------------------------------------------------- posture (integer units: 1 point = 30000, n pts/s = n*1000 per 30 Hz tick, no drift)
inline constexpr std::int64_t kUnits = 30000;
enum class Gain : std::uint8_t { None = 0, Added, Reached, Ignored };
struct Posture {
  int max_pts = policy::kPosture1Max;
  std::int64_t units = 0;
  unsigned since_gain = policy::kRegenDelayTicks;  // eligible ticks since the last genuine gain
  bool latched = false;                            // reached max: stays full until the break recovery resets it
  int points() const noexcept { return static_cast<int>(units / kUnits); }
  void reset(int max) noexcept { max_pts = max, units = 0, since_gain = policy::kRegenDelayTicks, latched = false; }
  Gain gain(int pts) noexcept {
    if (pts <= 0) return Gain::None;
    if (latched) return Gain::Ignored;
    units += static_cast<std::int64_t>(pts) * kUnits, since_gain = 0;
    if (units >= static_cast<std::int64_t>(max_pts) * kUnits) {
      units = static_cast<std::int64_t>(max_pts) * kUnits, latched = true;
      return Gain::Reached;
    }
    return Gain::Added;
  }
  // One ELIGIBLE tick (the adapter never calls it during stagger, transformation, fall, Finish, resurrection or death).
  bool regen(int pts_per_s) noexcept {
    if (latched || units <= 0) return false;
    if (since_gain < policy::kRegenDelayTicks) return ++since_gain, false;
    units -= static_cast<std::int64_t>(pts_per_s) * 1000;
    if (units < 0) units = 0;
    return true;
  }
};
// Deterministic bands of the remaining life fraction. Borders belong to the LOWER band: exactly 75 % -> "50-75", 50 % -> "25-50", 25 % -> "10-25",
// 10 % -> "<=10". Integer comparisons only: no oscillation (life never regenerates).
inline unsigned band(int remaining, int max) noexcept {
  if (max <= 0) return 4;
  const long long r = static_cast<long long>(remaining) * 100;
  if (r > static_cast<long long>(max) * 75) return 0;
  if (r > static_cast<long long>(max) * 50) return 1;
  if (r > static_cast<long long>(max) * 25) return 2;
  if (r > static_cast<long long>(max) * 10) return 3;
  return 4;
}
inline int regen_rate(bool phase2, int remaining, int max) noexcept { return (phase2 ? policy::kRegen2 : policy::kRegen1)[band(remaining, max)]; }

// ---------------------------------------------------------------- phase 1 HP and armour
// Armour progress for a remaining HP: 12 steps over 420 (one per 35 HP), capped at 11 while any HP remains (11 = the native final defence, shield
// gone); 12 ONLY at exactly 0 (the transition). The native mNextBreakPart is driven to this value, never below what is already broken.
inline int armor_progress(int hp) noexcept {
  if (hp <= 0) return 12;
  int p = (policy::kHp1Max - hp) * 12 / policy::kHp1Max;
  return p < 0 ? 0 : p > 11 ? 11 : p;
}
// The setBreakPart argument for a wanted progress, or -1 for "no call". Never below the parts already broken (that native call would break them all).
inline int armor_call(int progress, int broken) noexcept { return progress > broken && progress <= 11 ? progress : -1; }
// mNextBreakPart after our call: the larger of the wanted progress and what the native call really broke (the head bump counts 2 parts), capped at 11.
inline int armor_next(int progress, int broken_after) noexcept {
  const int v = progress > broken_after ? progress : broken_after;
  return v > 11 ? 11 : v;
}

// ---------------------------------------------------------------- break / stagger controller
enum class Bk : std::uint8_t { Combat = 0, Pending, Stagger, Recover, Count };
inline const char* name(Bk b) noexcept {
  static constexpr const char* n[] = {"COMBAT", "BREAK_PENDING", "STAGGER_ACTIVE", "RECOVERY_GUARD"};
  return unsigned(b) < 4 ? n[unsigned(b)] : "?";
}
enum class BkEnd : std::uint8_t { None = 0, Timeout, Hits, Phase, Death, Count };
inline const char* name(BkEnd e) noexcept {
  static constexpr const char* n[] = {"NONE", "TIMEOUT_90", "HITS_3", "PHASE_CHANGE", "DEFEAT"};
  return unsigned(e) < 5 ? n[unsigned(e)] : "?";
}
struct Break {
  Bk st = Bk::Combat;
  unsigned ticks = 0, hits = 0, recover_ticks = 0;
  BkEnd why = BkEnd::None;
  std::uint64_t breaks = 0;
  bool active() const noexcept { return st == Bk::Pending || st == Bk::Stagger || st == Bk::Recover; }
  void latch() noexcept {
    if (st == Bk::Combat) st = Bk::Pending;
  }
  void begin() noexcept {
    if (st == Bk::Pending) st = Bk::Stagger, ticks = hits = recover_ticks = 0, why = BkEnd::None, ++breaks;
  }
  // One eligible tick of the stagger. true = the recovery starts now (90 ticks).
  bool tick() noexcept {
    if (st != Bk::Stagger) return false;
    if (++ticks >= policy::kBreakTicks) return st = Bk::Recover, why = BkEnd::Timeout, true;
    return false;
  }
  // One unique effective hit of Link during the stagger. true = the recovery starts now (3 hits). Hits never restart the window.
  bool hit() noexcept {
    if (st != Bk::Stagger) return false;
    if (++hits >= policy::kBreakHits) return st = Bk::Recover, why = BkEnd::Hits, true;
    return false;
  }
  // The visual recovery: done when the native clip ended (the native exit) or by the watchdog. true = back to COMBAT (posture reset by the caller).
  bool recover(bool clip_done) noexcept {
    if (st != Bk::Recover) return false;
    if (clip_done || ++recover_ticks >= policy::kRecoverMaxTicks) return st = Bk::Combat, true;
    return false;
  }
  void abort(BkEnd e) noexcept {
    if (st != Bk::Combat) why = e;
    st = Bk::Combat, ticks = hits = recover_ticks = 0;
  }
};
// The stagger clip is played ONCE across the window: the first kJolt frames at the native rate, the rest stretched so that its last frame is reached
// when the window ends (never earlier: the native clip end is the native exit, a loop would jump back to frame 0). During the recovery: native speed+.
inline constexpr float kJoltFrames = 10.0f, kStretchMin = 0.25f, kRecoverRate = 1.25f;
inline float stagger_rate(unsigned age, float frame, float end) noexcept {
  if (frame < kJoltFrames) return 1.0f;
  const float left_ticks = static_cast<float>(policy::kBreakTicks) - static_cast<float>(age);
  const float left_frames = (end - 1.0f) - frame;
  if (left_frames <= 0.0f) return 0.0f;     // hold the last-but-one frame until the window ends
  if (left_ticks <= 1.0f) return 0.0f;
  return clampf(left_frames / left_ticks, kStretchMin, 1.0f);
}

// ---------------------------------------------------------------- Link passivity
enum class Act : std::uint8_t { Passive = 0, Hold, Active };
// Passive = standing, walking, turning, crouching (guard or not); Hold = forced states that neither count nor reset (guard slip, guard break, damage);
// Active = everything Link chooses to DO (cuts, Shield Attack/parry, rolls, side steps, back flips, items...).
inline Act classify(int proc) noexcept {
  using namespace native;
  switch (proc) {
    case kProcServiceWait: case kProcTiredWait: case kProcWait: case kProcMove: case kProcAtnMove: case kProcAtnActorWait: case kProcAtnActorMove:
    case kProcWaitTurn: case kProcMoveTurn: case kProcStepMove: case kProcCrouch:
      return Act::Passive;
    case kProcGuardSlip: case kProcGuardBreak: case kProcDamage: case kProcLargeDamageUp: case kProcLandDamage:
      return Act::Hold;
    default:
      return Act::Active;
  }
}
enum class Pv : std::uint8_t { None = 0, Shield, Idle };
struct Passive {
  unsigned shield_ticks = 0, idle_ticks = 0;
  std::uint64_t cooldown_until = 0, fired = 0;
  // One tick. dk_busy = the Darknut is attacking / staggered / transforming (a guard while reading an attack is not passivity: counters frozen).
  Pv step(Act a, bool guarding, bool dk_busy) noexcept {
    if (a == Act::Active) return shield_ticks = idle_ticks = 0, Pv::None;
    if (a == Act::Hold || dk_busy) return Pv::None;
    if (guarding) ++shield_ticks, idle_ticks = 0;
    else ++idle_ticks, shield_ticks = 0;
    if (shield_ticks >= policy::kPassiveShieldTicks) return Pv::Shield;
    if (idle_ticks >= policy::kPassiveIdleTicks) return Pv::Idle;
    return Pv::None;
  }
  bool ready(std::uint64_t now) const noexcept { return now >= cooldown_until; }
  void fire(std::uint64_t now) noexcept { cooldown_until = now + policy::kPassiveCooldownTicks, shield_ticks = idle_ticks = 0, ++fired; }
};

// ---------------------------------------------------------------- own RNG (cM_rnd is never consumed)
struct Rng {
  std::uint64_t s = 0x9E3779B97F4A7C15ull;
  void seed(std::uint64_t v) noexcept {
    v += 0x9E3779B97F4A7C15ull;
    v = (v ^ (v >> 30)) * 0xBF58476D1CE4E5B9ull;
    v = (v ^ (v >> 27)) * 0x94D049BB133111EBull;
    v ^= v >> 31;
    s = v ? v : 0x9E3779B97F4A7C15ull;
  }
  std::uint64_t next() noexcept {
    s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
    return s * 0x2545F4914F6CDD1Dull;
  }
  unsigned below(unsigned n) noexcept {
    if (n <= 1) return 0;
    const std::uint64_t lim = (~0ull / n) * n;
    std::uint64_t x;
    do x = next();
    while (x >= lim);
    return static_cast<unsigned>(x % n);
  }
  bool pct(int p) noexcept { return p >= 100 || (p > 0 && static_cast<int>(below(100)) < p); }
};
// Combo selection: the native AI decides WHEN to attack; at that decision, [PROVISIONAL] kComboPct % become a combo, chosen 25/25/25/25.
struct Selector {
  Rng rng;
  std::uint64_t cooldown_until = 0, rolls = 0, combos = 0, singles = 0, picked[4] = {};
  int pick(std::uint64_t now) noexcept {  // -1 = keep the native single attack
    ++rolls;
    if (now < cooldown_until || !rng.pct(policy::kComboPct)) return ++singles, -1;
    const unsigned k = rng.below(4);
    ++combos, ++picked[k];
    return static_cast<int>(k);
  }
  void ended(std::uint64_t now) noexcept { cooldown_until = now + policy::kComboCooldownTicks; }
};

// ---------------------------------------------------------------- the impact ledger: ONE resolution per impact window, whatever the native passes
struct Ledger {
  std::uint64_t serial = 0, resolved = 0;  // serial of the open impact window, last resolved serial
  Move move = Move::F2A;
  std::uint8_t k = 0;                       // impact index inside the move
  bool open = false;
  void open_window(Move m, std::uint8_t idx) noexcept { ++serial, move = m, k = idx, open = true; }
  void close() noexcept { open = false; }
  bool fresh() const noexcept { return open && resolved != serial; }
  bool resolve() noexcept {
    if (!fresh()) return false;
    resolved = serial;
    return true;
  }
};
// Which impact window of a move a frame is in (-1: none).
inline int window_of(Move m, float frame) noexcept {
  const MoveInfo& i = info(m);
  for (unsigned k = 0; k < i.impacts; ++k)
    if (frame >= i.at_open[k] && frame < i.at_close[k]) return static_cast<int>(k);
  return -1;
}

// ---------------------------------------------------------------- the life of one eligible Darknut
enum class Ph : std::uint8_t { Armor = 0, Transform, Light, Dkr, Revived, Dead, Count };
inline const char* name(Ph p) noexcept {
  static constexpr const char* n[] = {"PHASE1_ARMOR", "TRANSFORMATION", "PHASE2", "DKR01_EVENT", "PHASE2_REVIVED", "DEAD"};
  return unsigned(p) < 6 ? n[unsigned(p)] : "?";
}
inline bool phase2(Ph p) noexcept { return p == Ph::Light || p == Ph::Revived; }
// Remaining life fraction for the regeneration band: phase 1 own HP, phase 2 the native accumulator against 480.
inline int res2_remaining(int acc) noexcept {
  const int r = policy::kRes2Max - acc;
  return r < 0 ? 0 : r > policy::kRes2Max ? policy::kRes2Max : r;
}
}  // namespace dkn
