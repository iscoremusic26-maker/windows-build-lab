#pragma once
#include <cstddef>
#include <cstdint>
// LIGHT FADES TWICE 0.13.10 DARKNUT MINIBOSS FIX01 (DKN01 FIX01, base 0.13.9 DKN01 2bbc7c8). PURE core: standard library only, no game type, no SDK, no address.
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
// FIX01 (0.13.10) native facts:
//  * The native OFFENSIVE authority lives in executeChaseL/checkNextMove (ACT_ATTACKL 0/10, ACT_ATTACKSHIELDL 0/10 at < 200), executeChaseH/checkStartAttackH,
//    the landings of the jumps (ChaseL 7/8/12/15/17), executeGuardL mode 3 (TNB_DOWN -> ATTACKL 11 or the kick), the end of the kick/bash (ATTACKL 20 /
//    ATTACKH 0) and the B->A chain of executeAttackL (mActionMode2Copy == 0). checkNormalAttackAble() refuses EVERY start while Link's cut count != 0:
//    under pressure the native Darknut only guards (the 0.13.9 human log: 574 blocked contacts, 9000 ticks without one attack decision).
//    Every one of those starts is a setActionMode(ATTACK*, entry mode) executed by the NEXT action(); the action PRE sees it first.
//  * damage_check phase 2: a Shield Attack on mSphA ALWAYS sets ACT_GUARDL 0 (no front check); a frontal sword with field_0xa91 -> ACT_GUARDL 0 (block),
//    mCutFlag -> ACT_GUARDL 11 (TNB_DOWN + automatic attack). Phase 1: a Shield Attack on the guarding (ACT_GUARDH) Darknut -> GUARD_DAMAGE + onHeadLockFlg
//    every executeGuardH tick (the native Helm Splitter opening).
//  * setBodyShield phase 2: mSphA TgShield only with (field_0xa91 && mCutFlag) or mTimer10 != 0 -> Link's sword is REBOUNDED natively (checkCutReverseAt
//    -> checkAtShieldHit -> procCutReverseInit) only then. An ordinary frontal block (no TgShield) is a guard without rebound.
//  * Link: checkCutHeadState = the LOCKED enemy has HeadLockFlg (+ the skill); procCutHeadInit jumps to getHeadLockPos (action() writes eyePos + 300 every
//    tick). checkDownAttackState / procCutDownLandInit -> onCutDownHitFlg (DKR01). procCutFinishInit(CUT_FINISH_PARAM_MORTAL_DRAW_B) = the native Mortal
//    Draw (cut types MORTAL_DRAW_A/B), already driven for Chilfos (kk_native_mortal_draw).
//  * J3DFrameCtrl::update, mode NONE (once): mFrame += mRate; below mStart -> stop at mStart: a NEGATIVE rate plays a once-clip backwards natively.
// FIX02 (0.13.11) native facts, measured in dusklight.exe 2.0.3 (ffa99203..., the user's crash build 40457c6):
//  * THE 0.13.10 CRASH: daB_TN_c::getCutType (0x140cd1f70) starts `mov rax,[rcx+0x5ba8]; test byte [rax+0x20],2` = mAtInfo.mpCollider->ChkAtType
//    WITHOUT a null check; executeGuardL mode 0 (0x140ccedf4..fe) calls it unconditionally. damage_check writes mAtInfo.mpCollider = NULL
//    (0x140cc7ad3/7d39) whenever no hurt sphere took a hit. The native only enters ACT_GUARDL 0 from damage_check WITH a collider; FIX01's proactive
//    guard wrote ACT_GUARDL 0 in the action PRE with no hit -> the same tick: collider NULL -> read at 0x20 (the three crash logs: rva 0xcd1f86 <-
//    0xccee03 <- 0xcc311f (action's ACT_GUARDL case) <- mod.dll hook <- 0xcc8b57 execute). needs_hit_context() below is the invariant every writer obeys.
//  * Link's guard slip (procGuardSlipInit): mNormalSpeed = mGuardSpeedLarge 20 for spl 1 (the Darknut's), 14 for spl 0, decelerated 1.25 per tick
//    in procGuardSlip: ~160 units for every blocked Darknut impact (the user: Link pushed far away). FIX02 caps the speed of a slip THIS Darknut's
//    contact caused (link_slip_cap), in Link's checkDamageAction POST: same reaction, sound, shock and spl (a direct hit keeps the large reaction).
//  * executeChaseL modes 8/17 are the native forward LEAP (35-45 speed) that ends in a native attack: an offensive pattern of the native AI.
namespace dkn {
inline constexpr const char* kVersion = "0.13.11";
inline constexpr const char* kTag = "DKN01_FIX02";
inline constexpr unsigned kSlots = 8;

// ---------------------------------------------------------------- approved numbers (do not change)
namespace policy {
inline constexpr int kHp1Max = 420;          // phase 1 own HP reserve (option B)
inline constexpr int kRes2Max = 480;         // phase 2 / 3 resistance per life (native accumulator units), again 480 after the ONE resurrection
inline constexpr int kPosture1Max = 120, kPosture2Max = 160, kPosture3Max = 180;  // FIX01: phase 2 160 (was 130), phase 3 180 (was 130)
inline constexpr unsigned kRegenDelayTicks = 60;  // eligible ticks after the last gain
inline constexpr int kRegen1[5] = {10, 7, 4, 2, 1};  // pts/s by band (>75, 50-75, 25-50, 10-25, <=10 %)
inline constexpr int kRegen2[5] = {12, 8, 5, 2, 1};  // phases 2 and 3
inline constexpr unsigned kBreakTicks = 90;   // vulnerability window of a break (eligible ticks). FIX01: NO hit cap in any phase.
inline constexpr unsigned kDownTicks = 180;   // FIX01: 6 s on the ground after an execution fall, counted from the END of the fall clip
inline constexpr int kLinkMax = 100;          // Link's posture maximum (unchanged)
// Table A (Darknut gains). Phase 1 / phases 2-3.
inline constexpr int kGainSwordHp1 = 4, kGainBlocked1 = 2, kGainIron1 = 25;
inline constexpr int kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;
// Passivity thresholds.
inline constexpr unsigned kPassiveShieldTicks = 38;  // 1.25 s
inline constexpr unsigned kPassiveIdleTicks = 75;    // 2.5 s
inline constexpr unsigned kReviveFrames = 88;        // FIX02, measured
inline constexpr int kCounterPct = 50;               // after a REAL parry: 50 % thrust, 50 % double (approved proposal, parametrizable)
inline constexpr int kComboPick[4] = {25, 25, 25, 25};  // conditional equity once a combo is chosen
// ---------------------------------------------------------------- [PROVISIONAL] not approved numerically: configurable, logged as provisional
inline constexpr int kSinglePct = 25;                 // [PROVISIONAL] share of the OWN initiatives that are one isolated native strike instead of a combo
inline constexpr unsigned kGap1Min = 36, kGap1Max = 66;  // [PROVISIONAL] phase 1: ticks between the end of an offensive and the next initiative
inline constexpr unsigned kGap2Min = 22, kGap2Max = 46;  // [PROVISIONAL] phases 2-3
inline constexpr unsigned kReentryTicks = 45;         // [PROVISIONAL] after a recovery / getup / resurrection / transformation: no initiative, no special
inline constexpr unsigned kGuardLeaveTicks = 8;       // [PROVISIONAL] a native guard clip may give the initiative back after this many ticks
inline constexpr float kEngage1 = 330.0f, kEngage2 = 370.0f;  // [PROVISIONAL] initiative reach (wind-up distance correction covers the rest)
inline constexpr unsigned kPassiveCooldownTicks = 240;  // [PROVISIONAL] 8 s between two passivity responses of one Darknut
inline constexpr unsigned kRecoverMaxTicks = 60;      // watchdog of the visual recovery
inline constexpr unsigned kComboWatchdogTicks = 600;
inline constexpr float kComboLeaveDist = 700.0f;      // Link farther than this at a move boundary: the combo ends at that boundary
inline constexpr float kPassiveReach1 = 220.0f, kPassiveReach2 = 230.0f;  // shield bash / kick reach (model z 148 / 164 x scale 1.3 + bodies)
inline constexpr int kFrontArc = 0x3000;
// adaptive defense (phases 2-3)
inline constexpr unsigned kPressureWindow = 90;       // [PROVISIONAL] Link's sword contacts on this Darknut counted over the last 3 s
inline constexpr int kGuardPctBase = 20, kGuardPctStep = 15, kGuardPctMax = 75;   // [PROVISIONAL] proactive guard chance by pressure
inline constexpr int kParryPctBase = 15, kParryPctStep = 10, kParryPctMax = 50;   // [PROVISIONAL] of a guard, the share that arms a real parry
inline constexpr unsigned kParryPrepTicks = 3;        // [PROVISIONAL] guard raised -> parry window armed
inline constexpr unsigned kParryWindowTicks = 20;     // [PROVISIONAL] armed window (TgShield on the frontal body spheres)
inline constexpr unsigned kParryCooldownTicks = 150;  // [PROVISIONAL] after a REAL parry (or an armed window that expired: kParryCooldownTicks / 2)
inline constexpr unsigned kCounterWaitTicks = 8;      // Link must be in his native rebound (PROC_CUT_REVERSE) within this many ticks, else no counter
inline constexpr float kGuardReach = 300.0f;
// executions (phases 2-3)
inline constexpr unsigned kFinisherExtMax = 30;       // [PROVISIONAL] a Mortal Draw / Helm Splitter really in flight may outlast its window by this much, once
inline constexpr unsigned kHelmWindowTicks = 60;      // [PROVISIONAL] second stagger (Shield Attack -> Helm Splitter ready): its own window
inline constexpr float kMdReach = 350.0f;             // the Chilfos Mortal Draw reach (kk::policy::kMdReach)
inline constexpr int kMdArc = 0x2800;                 // Link facing the Darknut
inline constexpr unsigned kMdReceiptTtl = 75;         // a Mortal Draw request is a receipt for this actor/generation for at most this long
inline constexpr unsigned kGetupMaxTicks = 120;       // watchdog of the getup clip
}  // namespace policy

// ---------------------------------------------------------------- native constants (pinned by ci/dkn_native_audit.py)
namespace native {
inline constexpr int kActWaitH = 2, kActChaseH = 3, kActAttackH = 4, kActAttackShieldH = 5, kActGuardH = 6, kActDamageH = 7, kActChangeDemo = 8, kActChaseL = 9,
                     kActAttackL = 0xA, kActAttackShieldL = 0xB, kActGuardL = 0xC, kActDamageL = 0xD, kActEnding = 0xE, kActYoroke = 0xF;
inline constexpr int kBckTn2bDie = 0x04, kBckTnaAtackA = 0x05, kBckTnaAtackB = 0x06, kBckTnaAtackShield = 0x08, kBckTnaDamageL = 0x09, kBckTnaDamageLast = 0x0A,
                     kBckTnbAtackA = 0x14, kBckTnbAtackB = 0x15, kBckTnbAtackShield = 0x16, kBckTnbDown = 0x1A, kBckTnbDownSippu = 0x1B, kBckTnbGuardA = 0x1C,
                     kBckTnbGuardB = 0x1D, kBckTnbWait = 0x2A;
inline constexpr float kEndTnaDamageL = 37.0f, kEndTnbDownSippu = 34.0f, kEndTnbDown = 37.0f, kEndTn2bDie = 79.0f;  // frame counts (B_tn archive, measured)
inline constexpr int kSplLarge = 1;  // dCcG_At_Spl 1: checkLargeAttack, not a guard break
inline constexpr int kArmorParts = 13;
inline constexpr int kBreakPartNo[13] = {6, 11, 8, 1, 10, 0, 9, 7, 4, 5, 12, 2, 3};  // setBreakPart order
// Link procedures (daAlink_c::daAlink_PROC, pinned)
inline constexpr int kProcServiceWait = 1, kProcTiredWait = 2, kProcWait = 3, kProcMove = 4, kProcAtnMove = 5, kProcAtnActorWait = 6, kProcAtnActorMove = 7,
                     kProcWaitTurn = 8, kProcMoveTurn = 9, kProcStepMove = 0x1B, kProcCrouch = 0x1C, kProcGuardSlip = 0x1D, kProcGuardAttack = 0x1E,
                     kProcGuardBreak = 0x1F, kProcDamage = 0x32, kProcLargeDamageUp = 0x33, kProcLandDamage = 0x34;
// ChaseL / ChaseH modes that are "neutral" (walking, strafing, waiting): an own initiative may start there.
inline bool neutral_chase(bool phase2, int act, int mode) noexcept {
  if (phase2) return act == kActChaseL && (mode <= 5 || mode == 16);
  return act == kActChaseH;  // every ChaseH mode is walking / turning / waiting (its only attack start is checkStartAttackH -> ACT_ATTACKH 0)
}
// The native attack entries (what the native AI writes with setActionMode): every one of them is suppressed unless the controller owns the move.
inline bool native_attack_entry(int act, int mode) noexcept {
  if (act == kActAttackL) return mode == 0 || mode == 10 || mode == 11 || mode == 20;
  if (act == kActAttackShieldL) return mode == 0 || mode == 10;
  if (act == kActAttackH || act == kActAttackShieldH) return mode == 0;
  return false;
}
inline bool attack_action(int act) noexcept { return act == kActAttackH || act == kActAttackShieldH || act == kActAttackL || act == kActAttackShieldL; }
// FIX02: an (action, mode) whose native entry code reads the collision of THIS damage_check pass (executeGuardL mode 0 -> getCutType ->
// mAtInfo.mpCollider). Only damage_check itself may enter it; the module never writes it (it enters its guard at mode 1 with its own clip).
inline bool needs_hit_context(int act, int mode) noexcept { return act == kActGuardL && mode == 0; }
// FIX02: the native forward leap of ChaseL (initChaseL 8 / 17: JUMP_F, speed 35/45, landing -> attack) is not movement the controller asked for.
inline bool native_leap(int act, int mode) noexcept { return act == kActChaseL && (mode == 8 || mode == 17); }
inline constexpr int kBckTnbWalk = 0x2D;
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
// tests/data/dkn_swing_profile.txt), the AT window of each impact, the start/cut frame ranges the phase-2 rhythm planner may use, the reach for distance
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
    // F1 A lateral (TNA_ATACK_A): sweep 19-24, full reach at 22, AT 18..29 (native), follow-through ends 26. FIX01: phase-1 transitions come from kF1Plan.
    {Move::F1A, native::kBckTnaAtackA, false, 1, {22.0f, 0}, {18.0f, 0}, {29.0f, 0}, 0.0f, 16.0f, 48.0f, 80.0f, 82.0f, 260.0f, native::kActAttackH, 1, {9, 0}, 20, 30, 8.0f},
    // F1 B vertical (TNA_ATACK_B): smash 23-25, ground contact 26 (native SWD_IMPCT + shock at 26), AT 22..30
    {Move::F1B, native::kBckTnaAtackB, false, 1, {26.0f, 0}, {22.0f, 0}, {30.0f, 0}, 0.0f, 16.0f, 48.0f, 80.0f, 82.0f, 280.0f, native::kActAttackH, 2, {11, 0}, 26, 36, 8.0f},
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
// FIX02 [PROVISIONAL]: the highest speed Link's guard slip may keep when THIS Darknut's impact caused it (native: 20 = ~160 units at 1.25/tick).
// Mace 15 (~90 units), shield bash / kick 16 (~102), sword 13 (~68): a short, firm step back, never the native long slide.
inline float link_slip_cap(Move m) noexcept {
  switch (m) {
    case Move::F1A: case Move::F1B: return 15.0f;
    case Move::F1Shield: case Move::F2C: return 16.0f;
    default: return 13.0f;
  }
}
inline constexpr float kLinkSlipNative = 20.0f, kLinkSlipDecel = 1.25f;
inline float slip_distance(float v) noexcept {  // procGuardSlip: cLib_chaseF(&mNormalSpeed, 0, 1.25) after each move of mNormalSpeed
  float d = 0;
  while (v > 0.0f) d += v, v -= kLinkSlipDecel;
  return d;
}

// ---------------------------------------------------------------- the eight approved combos (sequence + the user's eight recordings) + the isolated strikes
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
// One isolated native strike chosen by the OWN controller (never the native AI): the move plays whole at its native rate. Indices 8.. of the plan table.
inline constexpr Combo kSingles[4] = {
    {"F1-SINGLE-A", false, 1, {Move::F1A}, 1, {0}, {0}},
    {"F1-SINGLE-B", false, 1, {Move::F1B}, 1, {0}, {0}},
    {"F2-SINGLE-A", true, 1, {Move::F2A}, 1, {0}, {0}},
    {"F2-SINGLE-B", true, 1, {Move::F2B}, 2, {0}, {0}},
};
inline constexpr unsigned kPlans = 12;  // 8 combos + 4 singles
inline const Combo& combo_of(unsigned idx) noexcept { return idx < 8 ? kCombos[idx] : kSingles[(idx - 8) & 3]; }
inline bool is_single(unsigned idx) noexcept { return idx >= 8; }
inline unsigned impacts_of(const Combo& c) noexcept {
  unsigned n = 0;
  for (unsigned i = 0; i < c.n_moves; ++i) n += info(c.moves[i]).impacts;
  return n;
}

// ---------------------------------------------------------------- FIX01 phase 1: the HEAVY plan (tests/data/dkn_f1_heavy.json, scripts in tests/data)
// The user (0.13.9 test): the mace combos were far too fast for such a weapon; FIX01 instruction: slower, but the SEPARATION between the attacks keeps the
// rhythm of the recordings. Measured on the real rig (tn2.bmd + TNA_ATACK_A/B, forward kinematics over the 29 joints + the mace tip): every 0.13.9 cut
// (frames 28-40) left a pose gap of 126-141 model units (mean per joint) blended in 4 frames = the mace "teleported". FIX01:
//  * ONE uniform time scale S per combo: every interval = audio interval x S (the ratios between the separations are EXACTLY the recording's);
//  * the wind-up and the swing of every move play at the NATIVE rate 1.0 (anticipation, impulse, contact, as the original clip);
//  * only the recovery of the previous move is paced, rate in [0.7, 1.5]; cut/start chosen on the rig so the pose gap is <= 90 (most <= 30),
//    blended over 10 frames (gap > 60) or 6 frames: S is the smallest value meeting all that (f1_scale_split.py, DMAX 90, RMAX 1.5).
struct F1Pair { std::uint8_t cut, start, morph; };
struct F1Plan { std::uint16_t s_x100; std::uint8_t first_start; F1Pair pair[4]; };
inline constexpr F1Plan kF1Plan[4] = {
    {259, 0, {{54, 12, 10}, {69, 4, 6}, {54, 12, 10}, {70, 4, 6}}},                      // F1-C1 B A B A B   2.52 s -> 6.53 s
    {144, 0, {{73, 4, 6}, {57, 16, 10}, {0, 0, 0}, {0, 0, 0}}},                           // F1-C2 B B B       2.31 s -> 3.33 s
    {256, 0, {{61, 16, 10}, {80, 0, 6}, {80, 0, 6}, {76, 10, 6}}},                        // F1-C3 A B B B A   3.54 s -> 9.05 s
    {145, 0, {{68, 4, 6}, {57, 16, 10}, {54, 12, 10}, {62, 16, 10}}},                     // F1-C4 A B B A B   4.12 s -> 5.97 s
};
inline constexpr float kF1RecoveryRateMin = 0.70f, kF1RecoveryRateMax = 1.51f;  // 1.5 + rounding of S to 1/100

// ---------------------------------------------------------------- the rhythm planner (pure, deterministic)
// One segment per move. The model of the adapter is exact: every tick the executeAttack POST (1) switches to the next move when the frame reached the
// cut, then sets the rate of the current region; action() then plays (frame += rate). An impact is the tick whose play crosses its impact frame.
inline constexpr float kRateMin = 0.55f, kRateMax = 2.40f;
struct Seg {
  Move m = Move::F2A;
  float start = 0, cut = 0;        // cut < 0: no cut (the last move ends natively / B1 ends at its fixed cut)
  float rate_in = 1, rate_mid = 1, rate_out = 1;
  float morph = 3;                 // blend frames of the switch INTO this segment
};
struct Plan {
  Seg seg[7];
  std::uint8_t n = 0;
  float target_ticks[8] = {};      // audio intervals in ticks (30 Hz), x S in phase 1
  bool clamped[8] = {};            // a rate hit its range: the interval cannot be met (reported, never hidden)
  float scale = 1.0f;              // phase 1: the uniform time scale S of this combo
};
inline float clampf(float v, float lo, float hi) noexcept { return v < lo ? lo : v > hi ? hi : v; }
inline Plan plan_f1(const Combo& c, const F1Plan& f) noexcept {
  Plan p;
  p.n = c.n_moves;
  p.scale = f.s_x100 / 100.0f;
  for (unsigned i = 0; i < c.n_moves; ++i) p.seg[i].m = c.moves[i], p.seg[i].cut = -1.0f, p.seg[i].morph = info(c.moves[i]).morph;
  p.seg[0].start = f.first_start;
  for (unsigned i = 0; i + 1 < c.n_moves; ++i) {
    const MoveInfo& a = info(c.moves[i]);
    const MoveInfo& b = info(c.moves[i + 1]);
    const F1Pair& q = f.pair[i];
    const float T = (c.peak_ms[i + 1] - c.peak_ms[i]) * 0.03f * p.scale;
    p.target_ticks[i] = T;
    const float wind = b.impact[0] - q.start;  // the next move's wind-up + swing at the native rate
    float r = (q.cut - a.impact[0]) / (T - wind);
    if (!(T > wind) || r < kF1RecoveryRateMin || r > kF1RecoveryRateMax) p.clamped[i] = true, r = clampf(r, kF1RecoveryRateMin, kF1RecoveryRateMax);
    p.seg[i].cut = q.cut, p.seg[i].rate_out = r;
    p.seg[i + 1].start = q.start, p.seg[i + 1].rate_in = 1.0f, p.seg[i + 1].morph = q.morph;
  }
  return p;
}
inline Plan plan(const Combo& c, unsigned idx = 99) noexcept {
  if (!c.phase2 && idx < 4) return plan_f1(c, kF1Plan[idx]);
  Plan p;
  p.n = c.n_moves;
  for (unsigned i = 0; i < c.n_moves; ++i) {
    const MoveInfo& mi = info(c.moves[i]);
    p.seg[i].m = c.moves[i];
    p.seg[i].start = c.n_moves == 1 ? (c.phase2 ? mi.s_min : 0.0f) : mi.s_min;  // the first move keeps its whole planned wind-up
    p.seg[i].cut = -1.0f;
    p.seg[i].morph = mi.morph;
  }
  if (c.n_moves == 1) return p;  // an isolated strike: native clip from its start to its native end, native rate
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
  // One ELIGIBLE tick (the adapter never calls it during stagger, transformation, fall, Finish, getup, resurrection or death).
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

// ---------------------------------------------------------------- the life of one eligible Darknut (three STAGES; not the native phase)
// Armor = FASE 1 (heavy armour + mace), Light = FASE 2 (sword, before the ONE resurrection), Revived = FASE 3 (sword, after FIX02).
enum class Ph : std::uint8_t { Armor = 0, Transform, Light, Dkr, Revived, Dead, Count };
inline const char* name(Ph p) noexcept {
  static constexpr const char* n[] = {"F1_ARMOR", "F1_TRANSFORM", "F2_SWORD", "DKR01_OWNED", "F3_SWORD_REVIVED", "DEAD"};
  return unsigned(p) < 6 ? n[unsigned(p)] : "?";
}
inline bool phase2(Ph p) noexcept { return p == Ph::Light || p == Ph::Revived; }
inline int posture_max(Ph p) noexcept { return p == Ph::Revived ? policy::kPosture3Max : p == Ph::Light ? policy::kPosture2Max : policy::kPosture1Max; }
inline unsigned stage(Ph p) noexcept { return p == Ph::Revived ? 2u : p == Ph::Light ? 1u : 0u; }  // telemetry index F1/F2/F3
// Remaining life fraction for the regeneration band: phase 1 own HP, phases 2-3 the native accumulator against 480.
inline int res2_remaining(int acc) noexcept {
  const int r = policy::kRes2Max - acc;
  return r < 0 ? 0 : r > policy::kRes2Max ? policy::kRes2Max : r;
}

// ---------------------------------------------------------------- the break controller (FIX01: no hit cap; phases 2-3 = execution states)
// Phase 1:   Pending -> StaggerA (TNA_DAMAGE_L held) -> 90 ticks -> Recover -> Combat (0/120). HP 0 at any time: the native transformation preempts.
// Phases 2-3: Pending -> StaggerA (TNB_DOWN_SIPPU held, vulnerable, NO fall) ->
//              (a) Link's REAL Mortal Draw contact (its receipt)               -> Exec (the native fall, owned by DKR01 until the getup / Finish)
//              (b) a NEW real Shield Attack contact -> StaggerB (TNB_DOWN, a different pose, HeadLock on) -> the REAL Helm Splitter contact -> Exec
//              the window ends without a confirmed execution -> Recover -> Combat (0/160 or 0/180). A finisher already in flight extends it ONCE, bounded.
enum class Bk : std::uint8_t { Combat = 0, Pending, StaggerA, StaggerB, Recover, Exec, Count };
inline const char* name(Bk b) noexcept {
  static constexpr const char* n[] = {"COMBAT", "BREAK_PENDING", "STAGGER_A", "STAGGER_B_HELM_READY", "RECOVERY", "EXECUTION"};
  return unsigned(b) < unsigned(Bk::Count) ? n[unsigned(b)] : "?";
}
enum class BkEnd : std::uint8_t { None = 0, Timeout, Phase, Death, Execution, Watchdog, Count };
inline const char* name(BkEnd e) noexcept {
  static constexpr const char* n[] = {"NONE", "TIMEOUT", "PHASE_CHANGE", "DEFEAT", "EXECUTION", "WATCHDOG"};
  return unsigned(e) < unsigned(BkEnd::Count) ? n[unsigned(e)] : "?";
}
struct Break {
  Bk st = Bk::Combat;
  bool p2 = false;
  unsigned ticks = 0, b_ticks = 0, recover_ticks = 0, ext = 0, hits = 0;
  std::uint64_t start = 0, b_start = 0, breaks = 0, helm_preps = 0, timeouts = 0;
  bool ext_used = false;
  BkEnd why = BkEnd::None;
  bool active() const noexcept { return st == Bk::Pending || st == Bk::StaggerA || st == Bk::StaggerB || st == Bk::Recover; }
  bool vulnerable() const noexcept { return st == Bk::StaggerA || st == Bk::StaggerB; }
  void latch() noexcept {
    if (st == Bk::Combat) st = Bk::Pending;
  }
  void begin(std::uint64_t now, bool phase2) noexcept {
    if (st != Bk::Pending) return;
    st = Bk::StaggerA, p2 = phase2, ticks = b_ticks = recover_ticks = ext = hits = 0, ext_used = false, why = BkEnd::None, start = now, ++breaks;
  }
  // A NEW, real Shield Attack contact during the first stagger of phases 2-3: the second stagger (Helm Splitter ready). Never twice, never in phase 1.
  bool helm_prep(std::uint64_t now) noexcept {
    if (!p2 || st != Bk::StaggerA) return false;
    st = Bk::StaggerB, b_start = now, b_ticks = 0, ext_used = false, ext = 0, ++helm_preps;
    return true;
  }
  // One eligible tick of a stagger. finisher = Link's Mortal Draw (StaggerA) / Helm Splitter (StaggerB) really in flight toward THIS actor.
  // true = the recovery starts now.
  bool tick(bool finisher) noexcept {
    if (st == Bk::StaggerA) {
      ++ticks;
      if (ticks < policy::kBreakTicks) return false;
    } else if (st == Bk::StaggerB) {
      ++ticks, ++b_ticks;
      if (b_ticks < policy::kHelmWindowTicks) return false;
    } else {
      return false;
    }
    if (finisher && ext < policy::kFinisherExtMax) return ext_used = true, ++ext, false;  // ONE bounded extension, only while that finisher flies
    return st = Bk::Recover, recover_ticks = 0, why = BkEnd::Timeout, ++timeouts, true;
  }
  // The execution contact (Mortal Draw in StaggerA, Helm Splitter in StaggerB) was confirmed: the fall belongs to DKR01 now.
  bool exec() noexcept {
    if (!p2 || !(st == Bk::StaggerA || st == Bk::StaggerB)) return false;
    return st = Bk::Exec, why = BkEnd::Execution, true;
  }
  // The visual recovery: done when the native clip ended (the native exit) or by the watchdog. true = back to COMBAT (posture reset by the caller).
  bool recover(bool clip_done) noexcept {
    if (st != Bk::Recover) return false;
    if (clip_done || ++recover_ticks >= policy::kRecoverMaxTicks) return st = Bk::Combat, true;
    return false;
  }
  void abort(BkEnd e) noexcept {
    if (st != Bk::Combat) why = e;
    st = Bk::Combat, ticks = b_ticks = recover_ticks = ext = hits = 0, ext_used = false;
  }
};
// The stagger clip is played ONCE across its window: the first kJolt frames at the native rate, the rest stretched so that its hold frame is reached
// when the window ends (never earlier: the native clip end is the native exit, a loop would jump back to frame 0). During the recovery: native speed+.
inline constexpr float kJoltFrames = 10.0f, kStretchMin = 0.25f, kRecoverRate = 1.25f;
// FIX01: a HELD clip never plays at rate 0: mDoExt_morf_c::isStop() is `checkState(1) || rate == 0`, and executeYoroke / executeDamageH end the stagger
// (setActionMode(ACT_CHASEL/CHASEH, 0)) at isStop(). The hold creeps 0.02 frame per tick instead (<= 2 frames over the longest window + extension).
inline constexpr float kHoldCreep = 0.02f;
inline float stagger_rate(unsigned age, float frame, float end, unsigned window = policy::kBreakTicks) noexcept {
  if (frame < kJoltFrames) return 1.0f;
  const float left_ticks = static_cast<float>(window) - static_cast<float>(age);
  const float left_frames = (end - 1.0f) - frame;
  if (left_frames <= 0.0f) return kHoldCreep;  // hold the last-but-one frame until the window ends (never rate 0: that is the native isStop)
  if (left_ticks <= 1.0f) return kHoldCreep;
  return clampf(left_frames / left_ticks, kStretchMin, 1.0f);
}
// Second stagger (TNB_DOWN, measured): frames 0-4 the shield bash knocks him back (head 211 -> 173), 6-16 he stays stunned with the sword up, 18-30 the
// recovery swing to the ready pose (its end = TNB_WAIT 0 within 1.3 units). Start frame 1 (pose gap from the SIPPU hold ~81, morph 5); hold frame 15.
inline constexpr float kHelmStart = 1.0f, kHelmMorph = 5.0f, kHelmHold = 15.0f;
inline float helm_rate(float frame) noexcept { return frame < kHelmHold ? 1.0f : kHoldCreep; }

// ---------------------------------------------------------------- the OWN offensive controller (FIX01: the native AI never starts an attack)
enum class Pick : std::uint8_t { None = 0, Combo, Single, Count };
enum class NoInit : std::uint8_t { Ok = 0, Busy, Unsafe, NoLink, Far, NotFront, Gap, Reentry, NotNeutral, Count };
inline const char* name(NoInit r) noexcept {
  static constexpr const char* n[] = {"OK", "BUSY", "UNSAFE", "NO_LINK", "FAR", "NOT_FRONT", "GAP", "REENTRY", "NOT_NEUTRAL"};
  return unsigned(r) < unsigned(NoInit::Count) ? n[unsigned(r)] : "?";
}
struct InitView {
  bool phase2 = false, busy = false, safe = true, link = true, front = true, neutral = true;
  float dist = 0;
  std::uint64_t now = 0;
};
struct Decision {
  Pick pick = Pick::None;
  unsigned idx = 0;  // plan index (0..7 combo, 8..11 single)
  NoInit why = NoInit::Ok;
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
struct Initiative {
  Rng rng;
  std::uint64_t gap_until = 0, reentry_until = 0, idle_since = 0;
  std::uint64_t opportunities = 0, decisions = 0, combos = 0, singles = 0, picked[4] = {}, rejects[unsigned(NoInit::Count)] = {};
  std::uint64_t idle_total = 0, idle_max = 0, idle_n = 0;
  // A decision point (every tick the Darknut stands in a neutral native state, and every suppressed native attack): the OWN choice, or why not.
  Decision decide(const InitView& v) noexcept {
    Decision d;
    if (v.busy) return reject(d, NoInit::Busy);
    if (!v.safe) return reject(d, NoInit::Unsafe);
    if (!v.link) return reject(d, NoInit::NoLink);
    if (!v.neutral) return reject(d, NoInit::NotNeutral);
    if (v.now < reentry_until) return reject(d, NoInit::Reentry);
    if (v.dist > (v.phase2 ? policy::kEngage2 : policy::kEngage1)) return reject(d, NoInit::Far);
    if (!v.front) return reject(d, NoInit::NotFront);
    ++opportunities;
    if (v.now < gap_until) return reject(d, NoInit::Gap);
    ++decisions;
    const unsigned base = v.phase2 ? 4u : 0u;
    if (rng.pct(policy::kSinglePct)) {
      // phase 1: the native single choice by distance (A < 250, else B); phase 2: thrust / double 50/50
      const unsigned which = v.phase2 ? rng.below(2) : (v.dist < 250.0f ? 0u : 1u);
      d.pick = Pick::Single, d.idx = 8 + (v.phase2 ? 2u : 0u) + which, ++singles;
    } else {
      unsigned r = rng.below(100), k = 0, acc = 0;
      for (; k < 3; ++k) {
        acc += static_cast<unsigned>(policy::kComboPick[k]);
        if (r < acc) break;
      }
      d.pick = Pick::Combo, d.idx = base + k, ++combos, ++picked[k];
    }
    return d;
  }
  Decision reject(Decision d, NoInit r) noexcept {
    d.why = r, ++rejects[unsigned(r)];
    return d;
  }
  void started(std::uint64_t now) noexcept {
    if (idle_since && now > idle_since) {
      const std::uint64_t idle = now - idle_since;
      idle_total += idle, ++idle_n;
      if (idle > idle_max) idle_max = idle;
    }
    idle_since = 0;
  }
  // An offensive (combo, single, special, counter) ended: the next initiative waits a randomized gap.
  void ended(std::uint64_t now, bool phase2) noexcept {
    const unsigned lo = phase2 ? policy::kGap2Min : policy::kGap1Min, hi = phase2 ? policy::kGap2Max : policy::kGap1Max;
    gap_until = now + lo + rng.below(hi - lo + 1);
    idle_since = now;
  }
  void reentry(std::uint64_t now) noexcept {
    reentry_until = now + policy::kReentryTicks;
    if (gap_until < reentry_until) gap_until = reentry_until;
    idle_since = now;
  }
};

// ---------------------------------------------------------------- adaptive defense (phases 2-3): proactive guard, REAL parry, one counter
// Inspired by the Lizalfos / Chilfos defense, re-built on the Darknut's own native facts: the guard is the native ACT_GUARDL 0 (TNB_GUARD_A/B); a parry
// window = mTimer10 kept alive (setBodyShield -> mSphA TgShield) while Link stays in front, so Link's sword is rebounded by HIS native route
// (checkCutReverseAt TRUE -> procCutReverseInit). Outside an armed window the adapter answers checkCutReverseAt FALSE for this Darknut's spheres: an
// ordinary block never rebounds. Parry = the rebound route really taken (TRUE kept on THIS actor's sphere) + Link really in PROC_CUT_REVERSE.
struct Defense {
  Rng rng;
  std::uint64_t contact[8] = {};
  unsigned next = 0;
  std::uint64_t guard_tick = 0, window_from = 0, window_to = 0, cooldown_until = 0, parry_tick = 0, counter_by = 0;
  bool armed = false, parried = false, counter_pending = false;
  std::uint64_t guards = 0, armed_n = 0, expired = 0, parries = 0, counters[2] = {}, counter_expired = 0, ordinary_blocks = 0, rebound_suppressed = 0;
  void note_contact(std::uint64_t now) noexcept { contact[next++ & 7] = now ? now : 1; }
  unsigned pressure(std::uint64_t now) const noexcept {
    unsigned n = 0;
    for (std::uint64_t c : contact) n += c && now >= c && now - c <= policy::kPressureWindow;
    return n;
  }
  static int pct(int base, int step, int mx, unsigned pressure) noexcept {
    const int v = base + step * static_cast<int>(pressure);
    return v > mx ? mx : v;
  }
  // Link starts a NEW swing in front, in reach, the Darknut free (no own attack, no stagger). true = raise the native guard now; armed = parry prepared.
  bool on_swing(std::uint64_t now, bool& arm) noexcept {
    arm = false;
    const unsigned p = pressure(now);
    if (!rng.pct(pct(policy::kGuardPctBase, policy::kGuardPctStep, policy::kGuardPctMax, p))) return false;
    ++guards, guard_tick = now;
    if (now >= cooldown_until && !armed && rng.pct(pct(policy::kParryPctBase, policy::kParryPctStep, policy::kParryPctMax, p))) {
      arm = armed = true, parried = false, ++armed_n;
      window_from = now + policy::kParryPrepTicks, window_to = window_from + policy::kParryWindowTicks;
    }
    return true;
  }
  bool window_open(std::uint64_t now) const noexcept { return armed && !parried && now >= window_from && now < window_to; }
  // Per tick: an armed window that ran out without a parry closes (half cooldown).
  void tick(std::uint64_t now) noexcept {
    if (armed && !parried && now >= window_to) armed = false, ++expired, cooldown_until = now + policy::kParryCooldownTicks / 2;
    if (counter_pending && now > counter_by) counter_pending = false, ++counter_expired;
  }
  // checkCutReverseAt POST answered TRUE on THIS actor's sphere: keep it (a real parry) only inside the armed window; once per window.
  bool rebound(std::uint64_t now) noexcept {
    if (!window_open(now)) return ++rebound_suppressed, false;
    parried = true, armed = false, ++parries, parry_tick = now, cooldown_until = now + policy::kParryCooldownTicks;
    counter_pending = true, counter_by = now + policy::kCounterWaitTicks;
    return true;
  }
  // Link really entered his native rebound: the ONE counter (0 = thrust, 1 = double), approved 50/50.
  int counter(bool link_rebounding) noexcept {
    if (!counter_pending || !link_rebounding) return -1;
    counter_pending = false;
    const int k = rng.pct(policy::kCounterPct) ? 0 : 1;
    ++counters[k];
    return k;
  }
  void cancel() noexcept { armed = false, counter_pending = false; }
};

// ---------------------------------------------------------------- Shield Attack authority (FIX01 bug: the native ACT_GUARDL / GUARDH it forces)
enum class Sa : std::uint8_t { Reject = 0, HelmPrep, RejectRepeat, Count };
inline const char* name(Sa s) noexcept {
  static constexpr const char* n[] = {"SHIELD_ATTACK_REJECTED", "HELM_PREP", "SHIELD_ATTACK_REJECTED_REPEAT"};
  return unsigned(s) < unsigned(Sa::Count) ? n[unsigned(s)] : "?";
}
// Only a NEW real Shield Attack (Link entered PROC_GUARD_ATTACK after the break began, never used before) on a phase-2/3 Darknut in its FIRST stagger
// prepares the Helm Splitter. Everything else is rejected: the native guard it forced is taken back, nothing else changes.
inline Sa sa_verdict(bool phase2, Bk st, bool sa_new) noexcept {
  if (phase2 && st == Bk::StaggerA && sa_new) return Sa::HelmPrep;
  if (st == Bk::StaggerB) return Sa::RejectRepeat;
  return Sa::Reject;
}

// ---------------------------------------------------------------- execution receipts and the contact verdict
struct Receipt {
  std::uint64_t generation = 0, tick = 0;
  bool on = false;
  void issue(std::uint64_t gen, std::uint64_t now) noexcept { generation = gen, tick = now, on = true; }
  bool valid(std::uint64_t gen, std::uint64_t now) const noexcept { return on && generation == gen && now >= tick && now - tick <= policy::kMdReceiptTtl; }
  void clear() noexcept { on = false; }
};
enum class Ex : std::uint8_t { None = 0, Fall, Lethal, Count };
inline const char* name(Ex e) noexcept {
  static constexpr const char* n[] = {"NONE", "EXECUTION_FALL", "EXECUTION_LETHAL_PRIORITY"};
  return unsigned(e) < unsigned(Ex::Count) ? n[unsigned(e)] : "?";
}
// A setDamage of THIS actor: is it the confirmed execution contact? md = Link's cut is a Mortal Draw with a valid receipt of this actor and generation in
// the first stagger; helm = Link's cut is the Helm Splitter (HEAD_JUMP) aimed at this actor in the second stagger. lethal = the native setDamage already
// chose ACT_ENDING (the resistance reached 480): then the real defeat has priority (DKR01 / native death), never a recoverable fall.
inline Ex exec_verdict(bool phase2, Bk st, bool md, bool helm, bool lethal) noexcept {
  if (!phase2) return Ex::None;
  const bool hit = (st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm);
  if (!hit) return Ex::None;
  return lethal ? Ex::Lethal : Ex::Fall;
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
  // One tick. dk_busy = the Darknut is attacking / staggered / transforming / re-entering (counters frozen: reading an attack is not passivity).
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
  void reset() noexcept { shield_ticks = idle_ticks = 0; }
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
}  // namespace dkn
