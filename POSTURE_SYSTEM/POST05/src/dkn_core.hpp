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
// FIX03 (0.13.12) native facts [S, measured in the user's logs dusklight-20261114-195050 / -200429]:
//  * setBodyShield phase 1 (mActionMode1 < 8): mSphC (the shield) is TgShield whenever the shield exists (mNextBreakPart < 11), mSphA when
//    field_0xa91 + Link in front, or mTimer10 != 0: EVERY phase-1 frontal sword contact was a native rebound (checkCutReverseAt TRUE). FIX03 answers
//    FALSE outside an armed window in phase 1 too: an ordinary block never rebounds in any phase. A phase-1 parry needs the shield (no parry at >= 11).
//  * executeGuardH mode 0 never reads the collider (TNA_GUARD, field_0xaa8 = Shield Attack -> GUARD_DAMAGE): the module still enters at mode 1.
//  * The 0.13.11 "guard pose" during the Helm Splitter: the second stagger held TNB_DOWN frame 15 = the native guard KNOCKBACK of executeGuardL 10/11:
//    sword raised above the head (tip - head +3..+34 at frames 6-16, tests/data/dkn_stagger_b_survey.txt). Log 195050: STAGGER_B frame 1 -> HELM_START
//    frame 15.1 -> HELM_CONTACT 36 ticks later on that very pose. FIX03 holds TNB_DAMAGE_R (the native phase-2 damage reaction, 0x18): head down, sword
//    low (tip - head -150..-192 at frames 4-18), 31-35 units from the SIPPU hold (distinct second stagger).
//  * The 0.13.11 POST audit counted 233 UNAUTHORIZED attacks: every one was a native ENTRY (setActionMode(ATTACK*, 0/10/11/20)) written during action()
//    and caught in the same POST, i.e. an attempt that never ran. FIX03 neutralizes and counts those as NATIVE_ENTRY_SUPPRESSED; only a running
//    (non-entry) attack mode without a lease is UNAUTHORIZED.
// FIX04 (0.13.13) native facts [S, measured in the user's log dusklight-20261114-231220]:
//  * damage_check phase 2 blocks a frontal sword on mSphA ONLY while field_0xa91 is true (else setDamage). FIX03 started the counter in the action PRE of
//    the very action() whose damage_check processed the parry contact; dkn_native_attack_flags wrote field_0xa91 = false first, so the parry contact
//    became an HP hit (log ticks 7640 / 38910: ENEMY_PARRY_REAL, COUNTER_START, HP2_DAMAGE +40 in one tick: damage spark instead of a parry).
//  * Link's rebound (procCutReverseInit, mCut.mRecoilAnm {45 frames, rate 1, cancel frame 15}): Link may act again only after frame 15. FIX03 counters
//    landed 6-13 ticks after the rebound (the bash at frame 7): the rebound was cut by a hit Link could not answer (F1 "intermittent").
//  * cc_at_check: a Link normal attack that damages calls dScnPly_c::setPauseTimer(2) (5 on a kill): every actor freezes 2 ticks (native hitstop). The
//    +2..+5 interval errors of the log are those pauses (each one follows an HP1/HP2_DAMAGE between two impacts), not the planner.
//  * The FIX03 F2/F3 plans played whole recovery + wind-up regions at 0.58-0.72 (F2-C2 / F2-C4 openings) next to 1.2-1.8: the cut / start ranges of
//    the thrust (cut <= 24, start >= 3) and of the kick (cut <= 26) left too few clip frames for the recorded gaps (the visible slow motion).
//  * Link's slip: the claim re-armed after a cap made Link's checkDamageAction POST of the next tick cap THE SAME slip again with chain + 1: every real
//    impact advanced the chain twice (log: chain 0 -> 1 on one impact, then 2, 4, 6 ...). The profile the user approved is therefore, per REAL impact,
//    x0.80 (first) and x0.65 (every later one); FIX04 counts once per impact and keeps exactly that profile.
// FIX05 (0.13.14) facts [S, measured in the user's log dusklight-20261115-104941, 5 Darknuts]:
//  * Swings were registered for the chain only when they STARTED within kGuardReach (300) of the Darknut, in front. A swing started farther (a step-in cut,
//    a jump attack, a cut right after the native TNB_DOWN guard knockback of executeGuardL 11/3: speedF 17 decelerated 1/tick, ~150 units backwards) was
//    blocked natively but never credited: actor 242 (all its chain contacts logged) had 15/49 F2/F3 blocks without a chain step, every one right after a
//    COMBO_END or a LINK_LEFT_REACH release; 8 ticks later (kGuardLeaveTicks) the guard clip counted as neutral and OWN_INITIATIVE fired (ticks 9731,
//    12966, 20811, distances 321/285/269, link_cutting=1). FIX05: every real defended contact feeds the chain (its swing is registered at the contact if it
//    was not yet), swings are tracked within kSwingTrackReach, and the exchange holds within the initiative reach (kEngage1/2): distance alone never hands
//    the initiative over while Link's chain is valid (only a pause, two empty swings, flank, Link down / broken, no defense).
//  * F2/F3 drew 25 % isolated thrusts (32 in the log) and the idle passivity kick: neither is approved. F2/F3 own offensives are the four combos only; the
//    passivity special in F2/F3 is the SHIELD_HELD kick only (38 ticks of a real held guard, cooldown 240): F1 keeps its singles and its idle bash.
//  * The counter double played the whole TNB_ATACK_B at 1/1.34 = 0.746: its two cuts 18.8 ticks apart (log ~19) while every combo double plays them
//    8.6-13.0 ticks apart (rate 1.08-1.63). FIX05 plays the counter double as the F2-C1 opener double (wind-up at 1.0 from frame 1, cuts 11.76 ticks apart =
//    280 ms x 1.40, the tail at 1.0 as a combo's last move); it still lands >= kCounterLandTicks after the rebound.
//  * The credit / counter read PROC_CUT_REVERSE in the same tick as procCutReverseInit (Link executes first): an init, not a persisting rebound. FIX05 needs a
//    second read one tick later with the recoil frame advanced (observe_rebound); a rebound gone by then gives neither credit nor counter.
// FIX06 (0.13.15) facts [S = 40457c6 = the v2.0.3 tag, measured in the user's log dusklight-20261121-234432 (sha256 b5ef18b0...), 3 Darknuts]:
//  * Every authentic enemy parry (68) started exactly ONE counter; dkn_move_end / Initiative::ended then handed the turn back to the common initiative, and
//    Link's valid chain (hold_verdict / InitView.defending) refused every new offensive while he kept pressing (4738 refusals): B -> parry -> counter ->
//    Link parries -> B was a loop. FIX06: the confirmed parry opens a SEQUENCE owned by this actor/generation (Seq): 60 % a combo directly, 40 % a counter;
//    a counter concluded legitimately continues with a combo 80 % of the time. The sequence owns the initiative (no defense verdict, no special, no common
//    initiative, no native attack) from the confirmation until its last recovery; only the legitimate exceptions end it.
//  * F2-C2 / F2-C4 (log ticks): 37,9,47,12,12,13 / 37,34,8,43. Five intervals retimed to approved FINAL targets (after x1.40); the others untouched.
//  * The second stagger froze: helm_rate() = 1.0 to frame 12, then kHoldCreep 0.02 (log 4358 -> 4423: frame 12 -> 13.0 in 65 ticks). FIX06 plays a continuous
//    sway around the head-down region instead (helm_frame).
//  * Phase 1 never had an executor (STAGGER_A executor=NONE_IN_F1). Native damage_check phase 1 lets HEAD_JUMP / MORTAL_DRAW_A/B through the shield sphere
//    (`cut_type != HEAD_JUMP && != MORTAL_DRAW_B && != MORTAL_DRAW_A` before the guard branch), setDamage phase 1 adds the part count and ACT_DAMAGEH; FIX06
//    lets both finishers punish a phase-1 break: 100 HP TOTAL, natural recovery, no DKR fall.
//  * draw(): mMaterialColor is written into TEV color 0 of the BODY materials before mpModelMorf2->entryDL(); the sword (mpModelMorf1, part 15) is drawn
//    with its own model data and no such write. tn_sword_b (B_tnp2 tn2_sword_b.bmd) has the very TEV stage of tn2_body (stage 0: KONST, TEXC, RASC, C0):
//    the same write darkens it the same way. Both models are created with diff flags 0x11000084 and mDoExt_modelEntryDL diffs the material state into the
//    instance at entryDL time (every presentation frame): a write before the sword's entryDL, undone after draw(), is this instance's only.
//  * initChaseL(6): speedF 0, TNB_JUMP_B_1 (once, morph 3); executeChaseL 6: frame 4 speedF 50 / speed.y 19 / gravity -4, JUMP_B_2 held until the ground,
//    7: landing (initChaseL zeroes speedF), at its end ATTACKSHIELDL 0 / ATTACKL 10 or initChaseL(1). initChaseL(8): TNB_JUMP_F_1; 8: frame 4 speedF 35 /
//    speed.y 4 / gravity -1, at clip end on the ground ATTACK* entries or initChaseL(9) (TNB_JUMP_F_2 landing, then initChaseL(1)).
namespace dkn {
inline constexpr const char* kVersion = "0.13.16";
inline constexpr const char* kTag = "DKN01_FIX07";
inline constexpr unsigned kSlots = 8;

// ---------------------------------------------------------------- approved numbers (do not change)
namespace policy {
inline constexpr int kHp1Max = 600;          // phase 1 own HP reserve (option B). FIX04: 600 (was 420)
inline constexpr int kRes2Max = 480;         // phase 2 / 3 resistance per life (native accumulator units), again 480 after the ONE resurrection
inline constexpr int kPosture1Max = 200, kPosture2Max = 160, kPosture3Max = 180;  // FIX04: phase 1 200 (was 120); FIX01: phase 2 160, phase 3 180
inline constexpr unsigned kRegenDelayTicks = 60;  // eligible ticks after the last gain
inline constexpr int kRegen1[5] = {10, 7, 4, 2, 1};  // pts/s by band (>75, 50-75, 25-50, 10-25, <=10 %)
inline constexpr int kRegen2[5] = {12, 8, 5, 2, 1};  // phases 2 and 3
inline constexpr unsigned kBreakTicks = 90;   // vulnerability window of a break (eligible ticks). FIX01: NO hit cap in any phase.
inline constexpr unsigned kDownTicks = 180;   // FIX01: 6 s on the ground after an execution fall, counted from the END of the fall clip
inline constexpr int kLinkMax = 100;          // Link's posture maximum (unchanged)
// Table A (Darknut gains). Phase 1 / phases 2-3.
inline constexpr int kGainSwordHp1 = 6, kGainBlocked1 = 4, kGainIron1 = 25;  // FIX04: +6 direct (was 4), +4 blocked by the shield (was 2)
inline constexpr int kGainSwordHp2 = 5, kGainBlocked2 = 3, kGainIron2 = 25;
// Passivity thresholds.
inline constexpr unsigned kPassiveShieldTicks = 38;  // 1.25 s
inline constexpr unsigned kPassiveIdleTicks = 75;    // 2.5 s
inline constexpr unsigned kReviveFrames = 88;        // FIX02, measured
// FIX03: after a REAL parry ONE counter, equal thirds among the eligible: phases 2-3 kick / thrust / full double; phase 1 shield bash (only with the
// shield) / lateral / vertical mace.
inline constexpr int kCounterThirds = 3;
inline constexpr int kLinkEnemyParry1 = 20, kLinkEnemyParry2 = 25;  // FIX03: Link's posture when the Darknut REALLY parries him (exactly once per parry)
inline constexpr int kComboPick[4] = {25, 25, 25, 25};  // conditional equity once a combo is chosen
// ---------------------------------------------------------------- [PROVISIONAL] not approved numerically: configurable, logged as provisional
inline constexpr int kSinglePct = 25;                 // [PROVISIONAL] PHASE 1 ONLY: share of the OWN initiatives that are one isolated native strike instead of a combo
inline constexpr bool kF2Singles = false;             // FIX05 (approved): phases 2-3 own offensives are the four combos only (no autonomous thrust)
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
inline constexpr int kGuardPctBase = 20, kGuardPctStep = 15, kGuardPctMax = 75;   // [PROVISIONAL] phases 2-3: ORDINARY proactive guard (no parry) by pressure
// FIX04 (approved): the parry ATTEMPT probability of one new, distinct Link swing (in front, in reach, the Darknut free), by its ORDINAL in Link's chain:
// ordinal = the defended contacts this chain really produced + 1 (1st, 2nd, 3rd, 4th, 5th, 6th+). The first three are never parried. ONE roll per
// swing key (never per frame / hook / collider), never a guard x parry double roll: these are the effective attempt probabilities.
inline constexpr int kParryOrdinalPct[6] = {0, 0, 0, 25, 45, 60};
inline constexpr unsigned kStreakGapTicks = 45;       // [PROVISIONAL] FIX03 reference: a swing more than 1.5 s after the previous one starts a new chain
inline constexpr unsigned kChainWhiffs = 2;           // [PROVISIONAL] FIX04: two swings in a row without a defended contact end the chain (no guard-lock)
inline constexpr unsigned kParryPrepTicks = 2;        // [PROVISIONAL] guard raised -> parry window armed
inline constexpr unsigned kParryWindowTicks = 20;     // [PROVISIONAL] armed window (TgShield on the frontal body spheres / the phase-1 shield)
inline constexpr unsigned kParryCooldownTicks = 60;   // [PROVISIONAL] after a REAL parry (the counter runs meanwhile); an expired window: none
inline constexpr unsigned kCounterWaitTicks = 8;      // Link must be in his native rebound (PROC_CUT_REVERSE) within this many ticks, else no parry, no counter
// FIX04: the counter's FIRST impact lands at least this long after the rebound: Link's recoil is cancelable after frame 15 (mRecoilAnm, native), plus a
// reaction margin to guard / parry it. The counter starts as early as that allows (its own wind-up plays), never in the rebound tick itself.
inline constexpr unsigned kRecoilCancelFrames = 15;   // native daAlinkHIO_cut_c0::m.mRecoilAnm.mCancelFrame
inline constexpr unsigned kCounterLandTicks = 24;     // [PROVISIONAL] rebound -> first counter impact (>= recoil cancel 15 + 9 ticks to answer)
inline constexpr unsigned kCounterLateTicks = 10;     // [PROVISIONAL] a scheduled counter that cannot start within this many ticks is dropped
inline constexpr float kGuardReach = 300.0f;          // a swing is ROLLED (parry / ordinary guard verdict) only within this reach, in front
inline constexpr float kSwingTrackReach = 500.0f;     // [PROVISIONAL] FIX05: a swing in front within this reach belongs to Link's chain (ordinal, whiff)
inline constexpr unsigned kRollLateTicks = 4;         // [PROVISIONAL] FIX05: a registered swing entering kGuardReach this young may still get its ONE verdict
inline float hold_reach(bool phase2) noexcept { return phase2 ? kEngage2 : kEngage1; }  // FIX05: the exchange holds within the initiative reach (kEngage1/2)
// executions (phases 2-3)
inline constexpr unsigned kFinisherExtMax = 45;       // [PROVISIONAL] a Mortal Draw / Helm Splitter really in flight may outlast its window by this much, once
inline constexpr unsigned kHelmWindowTicks = 60;      // [PROVISIONAL] second stagger (Shield Attack -> Helm Splitter ready): its own window
inline constexpr float kMdReach = 350.0f;             // the Chilfos Mortal Draw reach (kk::policy::kMdReach)
inline constexpr int kMdArc = 0x2800;                 // Link facing the Darknut
inline constexpr unsigned kMdReceiptTtl = 75;         // a Mortal Draw request is a receipt for this actor/generation for at most this long
inline constexpr unsigned kGetupMaxTicks = 120;       // watchdog of the getup clip
// FIX03 tempo (phases 2-3 only; phase 1 is frozen: kF1Plan, kCombos F1, rates, cuts, morphs untouched, pinned by tests/dkn_lab.cpp):
// FIX04 (approved): F2/F3 combos x1.40 against the FIX02 reference (FIX03 x1.25: durations +12 %, playback ~ -10.7 %). Singles / counters / kick: the
// same +12 % over FIX03 (x1.20 -> x1.34, x1.12 -> x1.25): one coherent, slower song.
inline constexpr float kF2ComboScale = 1.40f;         // every audio interval of the four F2/F3 combos x 1.40: Δt_i/Δt_j of the recording kept exactly
inline constexpr float kF2SingleScale = 1.34f;        // an isolated thrust / double (single or counter) x 1.34 (whole clip at rate 1/1.34)
inline constexpr float kKickScale = 1.25f;            // the kick (passivity special and counter) x 1.25
// FIX03 Link guard slip chain [PROVISIONAL]: a slip that starts within kSlipChainTicks of the previous one keeps less speed (no accumulated drift)
inline constexpr unsigned kSlipChainTicks = 45;       // [PROVISIONAL] 1.5 s
// ---------------------------------------------------------------- FIX06 (0.13.15)
// The initiative exchange after an AUTHENTIC enemy parry (approved): one roll per confirmed parry, one roll per legitimately concluded counter.
inline constexpr int kParryComboPct = 60;             // approved: 60 % one of the phase's four combos directly, 40 % one counter (60 + 40 x 80 = 92 % theoretical)
inline constexpr int kCounterComboPct = 80;           // approved: after the counter, 80 % one of the four combos, 20 % back to the common combat
inline constexpr unsigned kParryComboLandMin = 27, kParryComboLandMax = 36;  // approved window: rebound -> FIRST impact of the direct combo (0.90-1.20 s)
inline constexpr unsigned kParryComboLandTicks = 27;  // the target inside that window: its lower bound (3 ticks more than the counter's 24, no dead wait)
inline constexpr unsigned kBridgeSettleTicks = 3;     // [PROVISIONAL] counter clip end (its natural recovery played) -> continuation combo start, at least
inline constexpr unsigned kBridgeLandTicks = 24;      // last counter impact -> first combo impact, at least (= kCounterLandTicks: the same answer time)
inline constexpr unsigned kBridgeFreeTicks = 6;       // [PROVISIONAL] Link held by a forced state after the counter: the combo waits this long after he is free
inline constexpr unsigned kBridgeHoldMax = 60;        // [PROVISIONAL] Link still held this long after the counter ended: the continuation is dropped
inline constexpr unsigned kSeqWatchdogTicks = 900;    // [PROVISIONAL] safety: a sequence never owns the initiative longer than 30 s
// Phase-1 executions (approved): Mortal Draw / Helm Splitter on a broken phase-1 Darknut take a TOTAL (never added to the native damage).
// FIX07 (the user's final decision): 120 HP TOTAL each (20 % of the 600 reserve, three reference sword cuts of 40), saturated at the HP left. F2/F3 unchanged.
inline constexpr int kF1ExecDamage = 120;
// Reposition after the FIX02 rise / a recoverable getup (approved provisional design reference, XZ distance to Link): < 240 jump back, 240..370 hold, > 370
// jump forward if safe. The forward jump also needs its predicted landing >= kRepoNear (native displacement, see jump_model): a leap that would land on Link
// worsens the separation (documented adjustment, measured on the native physics, not on an animation).
inline constexpr float kRepoNear = 240.0f, kRepoFar = 370.0f;
inline constexpr float kRepoMaxDy = 120.0f;           // [PROVISIONAL] Link this much higher / lower than the Darknut: no jump (ledges, slopes)
inline constexpr unsigned kRepoWaitTicks = 15;        // [PROVISIONAL] the ONE decision waits at most this long for a neutral native chase after the rise / getup
inline constexpr unsigned kRepoSettleTicks = 15;      // [PROVISIONAL] after the landing: no initiative for at least this long (the 45-tick re-entry is kept too)
inline constexpr unsigned kRepoWatchdogTicks = 90;    // safety of one jump (the native back jump lasts ~20 ticks, the forward one ~20)
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
inline constexpr int kBckTnaGuard = 0x0C, kBckTnbDamageR = 0x18;  // FIX03: phase-1 shield guard; the second stagger (native damage reaction)
inline constexpr float kEndTnbDamageR = 37.0f;
// FIX06: the phase-1 damage reaction of the other side (executeDamageH mode 1: setBck(BCK_TNA_DAMAGE_R)), the native jumps of ChaseL (initChaseL 6 / 8)
inline constexpr int kBckTnaDamageR = 0x0B;
inline constexpr float kEndTnaDamageR = 42.0f;
inline constexpr int kBckTnbJumpB1 = 0x1E, kBckTnbJumpB2 = 0x1F, kBckTnbJumpF1 = 0x20, kBckTnbJumpF2 = 0x21;
inline constexpr float kEndTnbJump1 = 9.0f, kEndTnbJump2 = 7.0f;  // B_tn archive, measured (tnb_jump_b_1 / f_1: 9 frames; b_2 / f_2: 7)
inline constexpr int kChaseJumpBack = 6, kChaseJumpBackLand = 7, kChaseJumpFwd = 8, kChaseJumpFwdLand = 9;
inline constexpr int kChaseJumpFwdLong = 17;  // FIX07: initChaseL(17) (the native long forward leap: TNB_JUMP_F_1, 45 / 5 / -1)
inline bool jump_mode(int mode) noexcept {
  return mode == kChaseJumpBack || mode == kChaseJumpBackLand || mode == kChaseJumpFwd || mode == kChaseJumpFwdLand || mode == kChaseJumpFwdLong;
}
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
    {Move::F1A, native::kBckTnaAtackA, false, 1, {22.0f, 0}, {18.0f, 0}, {29.0f, 0}, 0.0f, 16.0f, 48.0f, 80.0f, 82.0f, 260.0f, native::kActAttackH, 1, {12, 0}, 20, 30, 8.0f},
    // F1 B vertical (TNA_ATACK_B): smash 23-25, ground contact 26 (native SWD_IMPCT + shock at 26), AT 22..30
    {Move::F1B, native::kBckTnaAtackB, false, 1, {26.0f, 0}, {22.0f, 0}, {30.0f, 0}, 0.0f, 16.0f, 48.0f, 80.0f, 82.0f, 280.0f, native::kActAttackH, 2, {14, 0}, 26, 36, 8.0f},
    // F1 shield bash (TNA_ATACK_SHIELD): thrust 5-7, AT 5..9 (mSphC)
    {Move::F1Shield, native::kBckTnaAtackShield, false, 1, {7.0f, 0}, {5.0f, 0}, {9.0f, 0}, 0.0f, 2.0f, 10.0f, 14.0f, 21.0f, 190.0f, native::kActAttackShieldH, 1, {14, 0}, 42, 50, 3.0f},
    // F2 A thrust (TNB_ATACK_A): lunge 8-10, full extension 10-11, AT 7..14. FIX04: start 1..6 (was 3..6: almost the native start from the ready pose,
    // the whole native preparation; frame 1, not 0, so a switch tick extrapolated backwards never starts below the clip) and cut 12..35 (was 12..24: the
    // natural recovery to the ready pose, clip end 36) so the x1.40 gaps are filled by clip frames at ~native speed instead of slow motion (FIX03 0.58-0.72).
    {Move::F2A, native::kBckTnbAtackA, true, 1, {10.0f, 0}, {7.0f, 0}, {14.0f, 0}, 1.0f, 6.0f, 12.0f, 35.0f, 36.0f, 270.0f, native::kActAttackL, 1, {11, 0}, 23, 33, 3.0f},
    // F2 B double (TNB_ATACK_B): cut 1 peak 15-16 (follow-through ends at 20, speed minimum), re-cock 21-25, cut 2 peak 29-30. Native AT 13..38 split
    // into one window per cut so each cut is ONE independent block/impact/parry opportunity.
    // FIX04: start 1..11 (was 5..11: almost the native start from the ready pose), cut 33..54 (was 33..50: the natural recovery, clip end 55).
    {Move::F2B, native::kBckTnbAtackB, true, 2, {15.5f, 29.5f}, {13.0f, 21.0f}, {21.0f, 38.0f}, 1.0f, 11.0f, 33.0f, 54.0f, 55.0f, 240.0f, native::kActAttackL, 2, {9, 9}, 12, 22, 3.0f},
    // F2 B1: ONLY the first cut of TNB_ATACK_B. Cut at frame 20 (end of the cut-1 follow-through, before the re-cock of cut 2 starts at 21); AT closed at 20.
    {Move::F2B1, native::kBckTnbAtackB, true, 1, {15.5f, 0}, {13.0f, 0}, {20.0f, 0}, 1.0f, 11.0f, 20.0f, 20.0f, 55.0f, 240.0f, native::kActAttackL, 2, {11, 0}, 23, 33, 3.0f},
    // F2 C kick (TNB_ATACK_SHIELD): foot out 9-10, AT 8..12 (mSphC on joint 24). FIX03: cut range 12..26 (was 12..20): F2-C4 kick -> thrust at x1.25
    // needs 33 ticks; a cut at 20 forced a 0.515 recovery (< kRateMin). Rig (tests/data/dkn_kick_cut_survey.txt): cuts 22-28 meet the thrust start with a
    // SMALLER pose gap (42.6-46.0) than the 0.13.11 cut 20 (52.0). FIX04: cut 12..28 (the surveyed range's end; clip end 29) for the x1.40 gap.
    {Move::F2C, native::kBckTnbAtackShield, true, 1, {10.0f, 0}, {8.0f, 0}, {12.0f, 0}, 0.0f, 3.0f, 12.0f, 28.0f, 29.0f, 200.0f, native::kActAttackShieldL, 1, {15, 0}, 38, 45, 3.0f},
};
inline const MoveInfo& info(Move m) noexcept { return kMoves[unsigned(m) < unsigned(Move::Count) ? unsigned(m) : 0u]; }
// Table B: Link posture of one impact. A confirmed parry gives Link +0.
inline int link_gain(Move m, Outcome o) noexcept {
  const MoveInfo& i = info(m);
  return o == Outcome::Parry ? 0 : o == Outcome::Block ? i.link_block : i.link_direct;
}
// Table A: Darknut posture when Link parries impact k of move m.
inline int dk_parry_gain(Move m, unsigned k) noexcept { return info(m).dk_parry[k < 2 ? k : 0]; }
// FIX02/FIX03 [PROVISIONAL]: the highest speed Link's guard slip may keep when THIS Darknut's impact caused it (native: 20 = ~160 units at 1.25/tick).
// FIX03: mace 12 (~64 units), shield bash / kick 12 (~64), sword 10 (~45); a slip chained within kSlipChainTicks of the previous one keeps x0.80
// (2nd) and x0.65 (3rd and later): a 7-impact blocked combo moves Link ~175 units instead of ~476 (0.13.11) or ~1120 (native).
inline float link_slip_base(Move m) noexcept {
  switch (m) {
    case Move::F1A: case Move::F1B: return 12.0f;
    case Move::F1Shield: case Move::F2C: return 12.0f;
    default: return 10.0f;
  }
}
inline float link_slip_cap(Move m, unsigned chain = 0) noexcept {
  const float f = chain == 0 ? 1.0f : chain == 1 ? 0.80f : 0.65f;
  return link_slip_base(m) * f;
}
// FIX04: the cap of the slip of ONE real impact by its ordinal in the slip chain (0 = first). FIX03 advanced the chain twice per impact (measured, see the
// header): the user-approved profile per real impact is the nominal table at 1 (x0.80) then at >= 2 (x0.65). One count per impact keeps it exactly.
inline float link_slip_cap_impact(Move m, unsigned ordinal) noexcept { return link_slip_cap(m, ordinal == 0 ? 1u : 2u); }
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
// FIX06 (approved): FIVE intervals of F2-C2 and F2-C4 get a FINAL effective target (ticks at 30 Hz AFTER the x1.40 tempo: never audio_ms x 1.40, never a
// change of the global scale). Pair j = impact j+1 -> impact j+2 of the combo. F2-C1 / F2-C3, phase 1, the doubles' inner cuts (C2 2->3, 4->5, 5->6, 6->7,
// C4 3->4), singles, counters and specials keep audio x S exactly. Phases 2 and 3 share these plans.
struct IvTarget { std::uint8_t combo, pair; float ticks; };
inline constexpr IvTarget kIvTargets[5] = {
    {5, 0, 27.0f},  // F2-C2 1->2  thrust -> thrust   was 37 (1.23 s) -> 0.90 s
    {5, 2, 30.0f},  // F2-C2 3->4  thrust -> double   was 47 (1.57 s) -> 1.00 s
    {7, 0, 27.0f},  // F2-C4 1->2  kick -> thrust     was 37 (1.23 s) -> 0.90 s
    {7, 1, 25.5f},  // F2-C4 2->3  thrust -> double   was 34 (1.13 s) -> 0.85 s (25.5 ticks: the planner is continuous; impacts quantize to 25 / 26)
    {7, 3, 30.0f},  // F2-C4 4->5  double -> thrust   was 43 (1.43 s) -> 1.00 s
};
inline float iv_target(unsigned combo, unsigned pair) noexcept {
  for (const IvTarget& t : kIvTargets)
    if (t.combo == combo && t.pair == pair) return t.ticks;
  return -1.0f;
}
// One isolated native strike chosen by the OWN controller (never the native AI): the move plays whole at its native rate. Indices 8.. of the plan table.
// FIX03: F2-SINGLE-B (the full double) is no longer an ordinary single: it stays inside the combos and as a counter. 12 / 13: counter-only plans.
inline constexpr Combo kSingles[6] = {
    {"F1-SINGLE-A", false, 1, {Move::F1A}, 1, {0}, {0}},
    {"F1-SINGLE-B", false, 1, {Move::F1B}, 1, {0}, {0}},
    {"F2-SINGLE-A", true, 1, {Move::F2A}, 1, {0}, {0}},
    {"F2-SINGLE-B", true, 1, {Move::F2B}, 2, {0}, {0}},
    {"F2-COUNTER-KICK", true, 1, {Move::F2C}, 1, {0}, {0}},
    {"F1-COUNTER-BASH", false, 1, {Move::F1Shield}, 1, {0}, {0}},
};
inline constexpr unsigned kPlans = 14;  // 8 combos + 4 singles + 2 counter-only
inline constexpr unsigned kIdxF1SingleA = 8, kIdxF1SingleB = 9, kIdxF2Thrust = 10, kIdxF2Double = 11, kIdxF2Kick = 12, kIdxF1Bash = 13;
inline const Combo& combo_of(unsigned idx) noexcept { return idx < 8 ? kCombos[idx] : kSingles[idx - 8 < 6 ? idx - 8 : 0]; }
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
    {256, 0, {{57, 22, 10}, {64, 8, 6}, {64, 7, 6}, {56, 12, 10}}},                       // F1-C3 A B B B A   FIX07: 24 / 56 / 57 / 40 ticks = 5.90 s (FIX06 9.05 s)
    {145, 0, {{68, 4, 6}, {57, 16, 10}, {54, 12, 10}, {62, 16, 10}}},                     // F1-C4 A B B A B   4.12 s -> 5.97 s
};
inline constexpr float kF1RecoveryRateMin = 0.70f, kF1RecoveryRateMax = 1.51f;  // 1.5 + rounding of S to 1/100
// FIX07 (approved, F1-C3 ONLY): the four FINAL effective intervals between its five impacts (ticks at 30 Hz), ~35 % shorter with the recording's contrast kept:
// 1->2 24 (was 36.10), 2->3 56 (86.02), 3->4 57 (87.55), 4->5 40 (61.82): 177 ticks = 5.90 s. Order, impacts, moves, the first start and the natural end are
// the FIX06 ones. Cut / start re-chosen on the same rig with the same FIX01 metric and rules (wind-up + swing at the native rate 1.0, only the previous move's
// recovery paced, pose gap <= 90; tools: dkn_fix07_evidence/tools/f1c3_retime.py, f1c3_pair1.py): 2->3 64/8 gap 17.2, 3->4 64/7 gap 19.0, 4->5 56/12 gap 73.5,
// all three at recovery 1.00. Pair 1->2 has NO solution under the FIX01 recovery cap 1.51 (its minimum there is 35.8 ticks, the FIX06 plan): the approved 24 is
// met exactly by starting the vertical at frame 22 (4 wind-up frames at 1.0, the smash 23-26 whole) after the lateral cut at 57 recovered at 1.75 (pose gap 68.0,
// below FIX06's own 89.2 for this pair). kF1C3Pair1RateMax is that one scoped exception; every other F1 pair keeps kF1RecoveryRateMax.
inline constexpr unsigned kF1C3 = 2;  // kCombos[2] = F1-C3
inline constexpr float kF1C3Targets[4] = {24.0f, 56.0f, 57.0f, 40.0f};
inline constexpr float kF1C3Pair1RateMax = 1.76f;

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
inline Plan plan_f1(const Combo& c, const F1Plan& f, unsigned idx = 99) noexcept {
  Plan p;
  p.n = c.n_moves;
  p.scale = f.s_x100 / 100.0f;
  for (unsigned i = 0; i < c.n_moves; ++i) p.seg[i].m = c.moves[i], p.seg[i].cut = -1.0f, p.seg[i].morph = info(c.moves[i]).morph;
  p.seg[0].start = f.first_start;
  for (unsigned i = 0; i + 1 < c.n_moves; ++i) {
    const MoveInfo& a = info(c.moves[i]);
    const MoveInfo& b = info(c.moves[i + 1]);
    const F1Pair& q = f.pair[i];
    const bool f1c3 = idx == kF1C3;  // FIX07: the approved final targets of F1-C3 only (every other F1 combo: audio x S, untouched)
    const float T = f1c3 ? kF1C3Targets[i] : (c.peak_ms[i + 1] - c.peak_ms[i]) * 0.03f * p.scale;
    p.target_ticks[i] = T;
    const float wind = b.impact[0] - q.start;  // the next move's wind-up + swing at the native rate
    const float rmax = f1c3 && i == 0 ? kF1C3Pair1RateMax : kF1RecoveryRateMax;
    float r = (q.cut - a.impact[0]) / (T - wind);
    if (!(T > wind) || r < kF1RecoveryRateMin || r > rmax) p.clamped[i] = true, r = clampf(r, kF1RecoveryRateMin, rmax);
    p.seg[i].cut = q.cut, p.seg[i].rate_out = r;
    p.seg[i + 1].start = q.start, p.seg[i + 1].rate_in = 1.0f, p.seg[i + 1].morph = q.morph;
  }
  return p;
}
// FIX03: the time scale of a phase-2/3 plan (phase 1: 1, its own kF1Plan scales).
inline float f2_scale(const Combo& c) noexcept {
  if (!c.phase2) return 1.0f;
  if (c.n_moves > 1) return policy::kF2ComboScale;
  return c.moves[0] == Move::F2C ? policy::kKickScale : policy::kF2SingleScale;
}
// FIX05: the counter double (F2-SINGLE-B, counter only) plays as the approved opener double of F2-C1 (its first move: TNB_ATACK_B from s_min 1, wind-up at
// the native rate 1.0, the two cuts 280 ms x 1.40 = 11.76 ticks apart = the recording of F2-C1 impacts 1-2), and its recovery at 1.0 as the last move of a
// combo plays its tail (F2-C2 ends on a double). Every other single / counter keeps its FIX04 tempo (thrust x1.34, kick x1.25); the eight combos untouched.
inline constexpr unsigned kCounterDoubleRef = 4;  // kCombos[4] = F2-C1, impacts 1-2
inline Plan plan_counter_double() noexcept {
  Plan p;
  p.n = 1;
  const Combo& ref = kCombos[kCounterDoubleRef];
  const MoveInfo& mi = info(Move::F2B);
  const float dt = (ref.peak_ms[1] - ref.peak_ms[0]) * 0.03f * policy::kF2ComboScale;
  p.scale = policy::kF2ComboScale;
  p.seg[0].m = Move::F2B, p.seg[0].start = mi.s_min, p.seg[0].cut = -1.0f, p.seg[0].morph = mi.morph;
  p.seg[0].rate_in = 1.0f, p.seg[0].rate_out = 1.0f;
  float r = (mi.impact[1] - mi.impact[0]) / dt;
  if (r < kRateMin || r > kRateMax) p.clamped[0] = true, r = clampf(r, kRateMin, kRateMax);
  p.seg[0].rate_mid = r, p.target_ticks[0] = dt;
  return p;
}
// scale > 0 overrides the phase-2/3 tempo (tests/dkn_lab.cpp: plan(c, i, 1.0f) is the 0.13.11 plan, bit for bit).
inline Plan plan(const Combo& c, unsigned idx = 99, float scale = 0.0f) noexcept {
  if (!c.phase2 && idx < 4) return plan_f1(c, kF1Plan[idx], idx);
  Plan p;
  p.n = c.n_moves;
  const float S = c.phase2 && scale > 0.0f ? scale : f2_scale(c);
  p.scale = S;
  for (unsigned i = 0; i < c.n_moves; ++i) {
    const MoveInfo& mi = info(c.moves[i]);
    p.seg[i].m = c.moves[i];
    p.seg[i].start = c.n_moves == 1 ? (c.phase2 ? mi.s_min : 0.0f) : mi.s_min;  // the first move keeps its whole planned wind-up
    p.seg[i].cut = -1.0f;
    p.seg[i].morph = mi.morph;
  }
  if (c.n_moves == 1) {  // an isolated strike: native clip from its start to its native end; phases 2-3 uniformly slower (FIX03)
    if (idx == kIdxF2Double && c.phase2 && !(scale > 0.0f)) return plan_counter_double();  // FIX05: the counter double = the F2-C1 opener double
    p.seg[0].rate_in = p.seg[0].rate_mid = p.seg[0].rate_out = 1.0f / S;
    return p;
  }
  // FIX06: the approved final targets of kIvTargets, only with the default tempo (plan(c, i, 1.0f) stays the 0.13.11 plan bit for bit)
  const bool fix06 = !(scale > 0.0f) && idx < 8;
  auto pair_dt = [&](unsigned j) noexcept {
    const float t = fix06 ? iv_target(idx, j) : -1.0f;
    return t > 0.0f ? t : (c.peak_ms[j + 1] - c.peak_ms[j]) * 0.03f * S;
  };
  unsigned imp = 0;
  for (unsigned i = 0; i < c.n_moves; ++i) {
    const MoveInfo& a = info(c.moves[i]);
    if (a.impacts == 2 && imp + 1 < c.n_impacts) {  // the two cuts of one B
      const float dt = pair_dt(imp);
      p.target_ticks[imp] = dt;
      float r = (a.impact[1] - a.impact[0]) / dt;
      if (r < kRateMin || r > kRateMax) p.clamped[imp] = true, r = clampf(r, kRateMin, kRateMax);
      p.seg[i].rate_mid = r;
    }
    imp += a.impacts;
    if (i + 1 >= c.n_moves) break;
    const MoveInfo& b = info(c.moves[i + 1]);
    const unsigned j = imp - 1;  // pair (impact j -> impact j+1) crosses the move boundary
    const float dt = pair_dt(j);
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
// Armour progress for a remaining HP: 12 steps over kHp1Max (FIX04: 600, one per 50 HP; 0.13.12: 420, one per 35), capped at 11 while any HP remains
// (11 = the native final defence, shield gone); 12 ONLY at exactly 0 (the transition). mNextBreakPart is driven to this value, never below what is broken.
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
  std::uint64_t f1_execs = 0;  // FIX06: phase-1 punishments (Mortal Draw / Helm Splitter contact, 100 HP total, natural recovery)
  // A NEW, real Shield Attack contact during the first stagger: the second stagger (Helm Splitter ready). Never twice per break. FIX06: phase 1 too.
  bool helm_prep(std::uint64_t now) noexcept {
    if (st != Bk::StaggerA) return false;
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
  // FIX06: the phase-1 punishment contact (Mortal Draw in StaggerA / Helm Splitter in StaggerB): ONE per break; the vulnerability ends and the natural
  // recovery plays (never a fall, never DKR01). Phases 2-3 keep exec().
  bool exec_f1() noexcept {
    if (p2 || !(st == Bk::StaggerA || st == Bk::StaggerB)) return false;
    return st = Bk::Recover, recover_ticks = 0, why = BkEnd::Execution, ++f1_execs, true;
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
// Second stagger, FIX03: TNB_DAMAGE_R (the native phase-2 damage reaction, measured): frames 0-4 the recoil, 6-16 bent forward, head down
// (171-173), sword LOW (tip 150-192 under the head: no guard-like pose, min gap to TNB_GUARD_A/B 44-58), 20-36 back to the ready pose. Distinct from
// the SIPPU hold (31-35 units). Start 0, morph 5, hold frame 12 (lowest head and sword). 0.13.11 held TNB_DOWN 15 = sword above the head (guard look).
inline constexpr float kHelmStart = 0.0f, kHelmMorph = 5.0f, kHelmHold = 12.0f;
inline constexpr int kHelmClip = native::kBckTnbDamageR;
// FIX06: the second stagger is never held still (0.13.14 crept 0.02 frame/tick from frame 12: the frozen pose of log tick 4358 -> 4423). The damage
// reaction plays at the native rate 1.0 up to kHelmHold (its head-down centre), then SWAYS around it: frame = 12 + 3.5 sin(2 pi v / 24), two whole periods in
// the 60-tick window (it leaves the centre moving forward at 0.92 frame/tick, close to the entry's 1.0; at the window end it is back at the centre, moving
// forward, and the recovery continues forward at kRecoverRate: no jolt, no restart, no loop jump). Frames 8.5..15.5 are the measured head-down / weapon-low
// span of both clips (tn2.bmd rig, tests/data/dkn_helm_sway_survey.txt): TNB_DAMAGE_R (phase 2-3, sword tip 150-190 under the head) and TNA_DAMAGE_R
// (phase 1, executeDamageH mode 1's own clip: pelvis 95-105, head 147-160, mace on the ground). The rate is wanted - current each tick (no drift), never 0
// (isStop), never below the clip start, never at its end before the recovery.
inline constexpr float kSwayAmp = 3.5f, kSwayPeriod = 24.0f, kSwayRateMax = 1.25f;
inline constexpr float kSwayPi = 3.14159265358979f;
struct HelmClip { int bck; float end; int act, mode; };
inline constexpr HelmClip kHelmClipF2 = {kHelmClip, native::kEndTnbDamageR, native::kActYoroke, 10};  // FIX03 kHelmClip = TNB_DAMAGE_R
inline constexpr HelmClip kHelmClipF1 = {native::kBckTnaDamageR, native::kEndTnaDamageR, native::kActDamageH, 2};
inline const HelmClip& helm_clip(bool phase2) noexcept { return phase2 ? kHelmClipF2 : kHelmClipF1; }
// The frame the second stagger wants u ticks after its preparation (the clip starts at kHelmStart in the preparation tick).
inline float helm_frame(float u) noexcept {
  const float entry = kHelmHold - kHelmStart;
  if (u <= entry) return kHelmStart + (u < 0 ? 0.0f : u);
  const float v = u - entry;
  float s = 2.0f * kSwayPi * v / kSwayPeriod;  // series sine on a reduced angle: no <cmath> difference between compilers / runtimes
  s -= 2.0f * kSwayPi * static_cast<float>(static_cast<int>(s / (2.0f * kSwayPi)));
  if (s > kSwayPi) s -= 2.0f * kSwayPi;
  float x = s, term = s, sum = s;
  for (int n = 1; n < 9; ++n) term *= -x * x / static_cast<float>((2 * n) * (2 * n + 1)), sum += term;
  return kHelmHold + kSwayAmp * sum;
}
// The J3D rate of the next play: lands on helm_frame(age + 1) (age = ticks of the second stagger already counted), bounded, never 0.
inline float helm_rate_at(unsigned age, float frame) noexcept {
  float r = helm_frame(static_cast<float>(age) + 1.0f) - frame;
  r = clampf(r, -kSwayRateMax, kSwayRateMax);
  if (r > -kHoldCreep && r < kHoldCreep) r = r < 0.0f ? -kHoldCreep : kHoldCreep;
  return r;
}

// ---------------------------------------------------------------- the OWN offensive controller (FIX01: the native AI never starts an attack)
enum class Pick : std::uint8_t { None = 0, Combo, Single, Count };
enum class NoInit : std::uint8_t { Ok = 0, Busy, Unsafe, NoLink, Far, NotFront, Gap, Reentry, NotNeutral, Defending, Count };
inline const char* name(NoInit r) noexcept {
  static constexpr const char* n[] = {"OK", "BUSY", "UNSAFE", "NO_LINK", "FAR", "NOT_FRONT", "GAP", "REENTRY", "NOT_NEUTRAL", "DEFENDING"};
  return unsigned(r) < unsigned(NoInit::Count) ? n[unsigned(r)] : "?";
}
struct InitView {
  bool phase2 = false, busy = false, safe = true, link = true, front = true, neutral = true;
  bool defending = false;  // FIX04: Link's valid chain against this Darknut's real defense owns the exchange (no new offensive of any kind)
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
    if (v.defending) return reject(d, NoInit::Defending);
    if (!v.neutral) return reject(d, NoInit::NotNeutral);
    if (v.now < reentry_until) return reject(d, NoInit::Reentry);
    if (v.dist > (v.phase2 ? policy::kEngage2 : policy::kEngage1)) return reject(d, NoInit::Far);
    if (!v.front) return reject(d, NoInit::NotFront);
    ++opportunities;
    if (v.now < gap_until) return reject(d, NoInit::Gap);
    ++decisions;
    const unsigned base = v.phase2 ? 4u : 0u;
    // FIX05: phases 2-3 never draw an isolated strike (kF2Singles false): their own offensive is one of the four combos, 25/25/25/25. Phase 1 unchanged.
    if ((!v.phase2 || policy::kF2Singles) && rng.pct(policy::kSinglePct)) {
      // phase 1: the native single choice by distance (A < 250, else B); phases 2-3 (FIX03): always the thrust (the double is a combo / counter move)
      d.pick = Pick::Single, d.idx = v.phase2 ? kIdxF2Thrust : (v.dist < 250.0f ? kIdxF1SingleA : kIdxF1SingleB), ++singles;
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
  // FIX06: after a reposition jump's landing: no initiative before `until` (never shortens a re-entry already running)
  void settle(std::uint64_t until) noexcept {
    if (reentry_until < until) reentry_until = until;
    if (gap_until < until) gap_until = until;
  }
};

// ---------------------------------------------------------------- adaptive defense (FIX03/FIX04: phases 1, 2 and 3): parry by ordinal, REAL parry, one counter
// Inspired by the Lizalfos / Chilfos defense, re-built on the Darknut's own native facts. A parry window = mTimer10 + field_0xa91 kept alive while Link stays
// in front (setBodyShield -> TgShield: phase 2-3 mSphA, phase 1 mSphA + the shield mSphC), so Link's sword is rebounded by HIS native route
// (checkCutReverseAt TRUE -> procCutReverseInit). Outside an armed window the adapter answers checkCutReverseAt FALSE for this Darknut's spheres in EVERY
// phase: an ordinary block never rebounds.
// FIX04: Link's CHAIN against this Darknut = consecutive swings (<= kStreakGapTicks apart) whose contacts the Darknut really defended. Each new swing key
// gets ONE verdict prepared before its contact by its ordinal (defended contacts so far + 1): 0/0/0/25/45/60 %; only a REAL defended contact advances the
// chain (a whiff, a direct hit or a Shield Attack never does); two swings in a row without one end it. While the chain lives the Darknut keeps the
// exchange (holds(): no offensive decision of any kind starts over it). A parry = the native rebound route taken inside the window bound to that swing;
// it is CREDITED (Link +20/+25, once) and gives ONE counter only when Link's PROC_CUT_REVERSE is read afterwards (confirm). The counter starts as soon as
// its first impact lands >= kCounterLandTicks after the rebound (Link's recoil is cancelable after frame 15): never in the rebound tick.
enum class Dv : std::uint8_t { None = 0, Guard, Parry };
// FIX05: what one read of Link's procedure says about a parry rebound waiting for its credit (observe_rebound).
enum class Rb : std::uint8_t { None = 0, Waiting, Seen, Lost, Confirmed };
inline const char* name(Rb r) noexcept {
  static constexpr const char* n[] = {"NONE", "WAITING", "SEEN_INIT", "LOST_OVERWRITTEN", "CONFIRMED_PERSISTING"};
  return unsigned(r) < 5 ? n[unsigned(r)] : "?";
}
enum class Cp : std::uint8_t { None = 0, Await, Scheduled, CreditOnly };
inline const char* name(Cp c) noexcept {
  static constexpr const char* n[] = {"NONE", "AWAIT_LINK_REBOUND", "SCHEDULED", "CREDIT_ONLY"};
  return unsigned(c) < 4 ? n[unsigned(c)] : "?";
}
// Ticks from the start of a single / counter plan to its first impact under the adapter's play model (the move starts with its first play in the start
// tick: impact on play k = the first k with start + k * rate_in >= impact; tick index k - 1).
inline unsigned first_impact_tick(unsigned idx) noexcept {
  const Plan p = plan(combo_of(idx), idx);
  const Seg& s = p.seg[0];
  const float need = info(s.m).impact[0] - s.start;
  unsigned k = 1;
  while (static_cast<float>(k) * s.rate_in < need - 1e-4f && k < 200) ++k;
  return k - 1;
}
// Ticks between the reference tick and the start of plan idx so that its first impact lands `land` ticks after the reference (at least 1: never in it).
inline unsigned land_delay(unsigned idx, unsigned land) noexcept {
  const unsigned fi = first_impact_tick(idx);
  return fi + 1 >= land ? 1u : land - fi;
}
inline unsigned counter_delay(unsigned idx) noexcept { return land_delay(idx, policy::kCounterLandTicks); }
// FIX06: the direct combo after an authentic parry lands its first impact kParryComboLandTicks after the rebound (inside the approved 27-36).
inline unsigned parry_combo_delay(unsigned idx) noexcept { return land_delay(idx, policy::kParryComboLandTicks); }
// FIX06: the continuation combo after a counter: never before the counter's natural recovery ended (+ settle), its first impact >= kBridgeLandTicks after
// the counter's LAST impact.
inline std::uint64_t bridge_due(std::uint64_t counter_end, std::uint64_t last_impact, unsigned idx) noexcept {
  const unsigned fi = first_impact_tick(idx);
  const std::uint64_t a = counter_end + policy::kBridgeSettleTicks;
  const std::uint64_t b = last_impact + policy::kBridgeLandTicks > fi ? last_impact + policy::kBridgeLandTicks - fi : 0;
  return a > b ? a : b;
}
struct Defense {
  Rng rng;
  std::uint64_t contact[8] = {};
  unsigned next = 0;
  unsigned chain = 0, whiffs = 0;  // defended contacts of the current chain; swings in a row without one
  std::uint64_t last_swing = 0, cur_key = 0, rolled_key = 0, window_key = 0, parried_key = 0;
  bool cur_contacted = false;
  std::uint64_t guard_tick = 0, window_from = 0, window_to = 0, cooldown_until = 0, parry_tick = 0, counter_by = 0, counter_at = 0;
  bool armed = false, parried = false;
  bool reverse_seen = false;           // FIX05: Link's PROC_CUT_REVERSE read once for the pending rebound (its init)
  std::uint64_t reverse_tick = 0;
  float reverse_frame = 0;
  Cp cp = Cp::None;
  int counter_idx = -1;
  unsigned parry_ordinal = 0, cur_ord = 0;
  std::uint64_t guards = 0, armed_n = 0, expired = 0, parries = 0, confirmed = 0, unconfirmed = 0, counters[3] = {}, counter_dropped = 0, ordinary_blocks = 0,
                rebound_suppressed = 0;
  std::uint64_t swings[6] = {}, rolls[6] = {}, attempts[6] = {}, contacts[6] = {}, parries_at[6] = {}, no_shield = 0, repeats = 0, gap_breaks = 0, whiff_breaks = 0,
                rerolls_refused = 0;
  std::uint64_t late_registered = 0, rebound_lost = 0;  // FIX05
  // FIX06: the route of a confirmed parry (production: routes = true, set once at admission). ONE roll on its own stream per confirmed parry: 60 % one of the
  // phase's four combos (25/25/25/25), 40 % the FIX05 counter (equal thirds). routes = false keeps the FIX05 counter-only rule (labs of earlier versions).
  bool routes = false, route_combo = false;
  Rng route;
  std::uint64_t route_rolls = 0, route_combos = 0, route_counters = 0, route_pick[4] = {};
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
  unsigned ordinal() const noexcept { return chain + 1 < 6 ? chain + 1 : 6; }
  // A new distinct Link swing (its key) in front and in reach, whatever the Darknut is doing: its ordinal (1..6, 6 = sixth and later).
  unsigned on_link_swing(std::uint64_t now, std::uint64_t key) noexcept {
    if (last_swing && key == cur_key) return cur_ord;  // the same swing seen again: its own ordinal, nothing else changes
    if (last_swing && !cur_contacted) ++whiffs;          // the previous swing produced no defended contact
    if (!last_swing || now < last_swing || now - last_swing > policy::kStreakGapTicks) {
      if (chain) ++gap_breaks;
      chain = 0, whiffs = 0;
    } else if (whiffs >= policy::kChainWhiffs && chain) {
      chain = 0, ++whiff_breaks;
    }
    if (armed && !parried && window_key != key) armed = false, ++expired;  // that window belonged to the previous swing
    cur_key = key, cur_contacted = false, last_swing = now, cur_ord = ordinal();
    ++swings[cur_ord - 1];
    return cur_ord;
  }
  // The Darknut is free for the current swing: ONE verdict per swing key. can_parry = the parry route exists (phase 1: the shield is still there);
  // p2 = an ordinary guard may be raised when no parry is attempted.
  Dv on_swing(std::uint64_t now, unsigned ord, bool can_parry, bool p2) noexcept {
    if (rolled_key == cur_key) return ++rerolls_refused, Dv::None;
    rolled_key = cur_key;
    const unsigned b = (ord < 1 ? 1u : ord > 6 ? 6u : ord) - 1u;
    if (!can_parry) ++no_shield;
    const int chance = policy::kParryOrdinalPct[b];
    if (can_parry && !armed && cp == Cp::None && now >= cooldown_until && chance > 0) {
      ++rolls[b];
      if (rng.pct(chance)) {
        armed = true, parried = false, ++armed_n, ++attempts[b], ++guards, guard_tick = now, window_key = cur_key;
        window_from = now + policy::kParryPrepTicks, window_to = window_from + policy::kParryWindowTicks;
        return Dv::Parry;
      }
    }
    if (p2 && rng.pct(pct(policy::kGuardPctBase, policy::kGuardPctStep, policy::kGuardPctMax, pressure(now)))) return ++guards, guard_tick = now, Dv::Guard;
    return Dv::None;
  }
  // A contact of swing `key` the Darknut REALLY defended: one chain step per swing; never the parried swing, never an older swing.
  bool on_contact(std::uint64_t key) noexcept {
    if (key == parried_key || key != cur_key || cur_contacted || !last_swing) return false;
    cur_contacted = true, whiffs = 0, ++contacts[ordinal() - 1];
    ++chain;
    return true;
  }
  // FIX05: a REAL defended contact of a swing the chain has not registered (it started beyond the tracking reach, outside the front arc, or before the
  // tracking saw it: a step-in cut, a jump attack, a cut right after the native guard knockback). It is registered now, never rolled (its contact already
  // happened: no verdict afterwards), so EVERY defended contact feeds the chain. Keys only grow; the parried swing never re-enters.
  bool register_at_contact(std::uint64_t now, std::uint64_t key) noexcept {
    if (!key || key == parried_key || (last_swing && key <= cur_key)) return false;
    on_link_swing(now, key);
    rolled_key = key;
    ++late_registered;
    return true;
  }
  // The exchange belongs to Link's chain: a recent swing and not two empty ones in a row.
  bool holds(std::uint64_t now) const noexcept {
    return last_swing && now >= last_swing && now - last_swing <= policy::kStreakGapTicks && whiffs < policy::kChainWhiffs;
  }
  bool window_open(std::uint64_t now) const noexcept { return armed && !parried && now >= window_from && now < window_to; }
  bool parry_hold(std::uint64_t now) const noexcept { return parried && now >= parry_tick && now <= parry_tick + 1; }  // the parry contact stays a block
  bool busy() const noexcept { return armed || cp == Cp::Await || cp == Cp::Scheduled; }
  // Per tick: an armed window that ran out without a parry closes (no cooldown: the next swing rolls again); unconfirmed rebounds / stale counters go.
  void tick(std::uint64_t now) noexcept {
    if (armed && !parried && now >= window_to) armed = false, ++expired;
    if ((cp == Cp::Await || cp == Cp::CreditOnly) && now > counter_by) cp = Cp::None, ++unconfirmed;
    if (cp == Cp::Scheduled && now > counter_at + policy::kCounterLateTicks) cp = Cp::None, ++counter_dropped;
  }
  // checkCutReverseAt POST answered TRUE on THIS actor's sphere: 1 = a NEW parry rebound (inside the armed window, once per window), 2 = the same parry seen
  // again in the same tick (another of Link's colliders: kept TRUE, never counted twice), 0 = not a parry (the caller answers FALSE).
  int rebound(std::uint64_t now) noexcept {
    if (parried && parry_tick == now) return ++repeats, 2;
    if (!window_open(now)) return ++rebound_suppressed, 0;
    parry_ordinal = ordinal();
    parried = true, armed = false, ++parries, ++parries_at[parry_ordinal - 1], parry_tick = now, cooldown_until = now + policy::kParryCooldownTicks;
    cp = Cp::Await, counter_by = now + policy::kCounterWaitTicks, counter_idx = -1, parried_key = window_key, reverse_seen = false;
    chain = 0, whiffs = 0, last_swing = 0;  // the parry ends Link's chain
    return 1;
  }
  // Link's native rebound is established (PROC_CUT_REVERSE read after procCutReverseInit): true ONCE = the credit. Unless a break preempted it, the ONE
  // counter is chosen (equal thirds among the eligible) and scheduled so its first impact lands kCounterLandTicks after the rebound.
  //   phases 2-3: kick (12) / thrust (10) / full double (11);  phase 1: shield bash (13, only with the shield) / lateral (8) / vertical (9).
  bool confirm(std::uint64_t now, bool link_rebounding, bool p2, bool shield) noexcept {
    if ((cp != Cp::Await && cp != Cp::CreditOnly) || !link_rebounding || now < parry_tick) return false;
    ++confirmed;
    if (cp == Cp::CreditOnly) return cp = Cp::None, true;
    route_combo = false;
    if (routes) {
      ++route_rolls;
      if (route.pct(policy::kParryComboPct)) {
        const unsigned k = route.below(4);
        const unsigned cidx = (p2 ? 4u : 0u) + k;
        ++route_combos, ++route_pick[k], route_combo = true;
        counter_idx = static_cast<int>(cidx), counter_at = parry_tick + parry_combo_delay(cidx), cp = Cp::Scheduled;
        return true;
      }
      ++route_counters;
    }
    unsigned idx;
    if (p2) {
      const unsigned k = rng.below(3);
      idx = k == 0 ? kIdxF2Kick : k == 1 ? kIdxF2Thrust : kIdxF2Double;
      ++counters[k];
    } else {
      const unsigned k = shield ? rng.below(3) : 1u + rng.below(2);
      idx = k == 0 ? kIdxF1Bash : k == 1 ? kIdxF1SingleA : kIdxF1SingleB;
      ++counters[k];
    }
    counter_idx = static_cast<int>(idx), counter_at = parry_tick + counter_delay(idx), cp = Cp::Scheduled;
    return true;
  }
  // FIX05: one read of Link (action PRE) for a pending rebound. The first PROC_CUT_REVERSE read is procCutReverseInit itself (Link executes before the
  // Darknut in the rebound tick): only SEEN. The credit and the counter (confirm) need a later read with the recoil clip ADVANCED: the native rebound really
  // persists. Anything else after it was seen (another procedure, a restarted / held frame) = LOST: no credit, no counter, nothing pending.
  Rb observe_rebound(std::uint64_t now, bool link_rebounding, float frame, bool p2, bool shield) noexcept {
    if (cp != Cp::Await && cp != Cp::CreditOnly) return Rb::None;
    if (now < parry_tick) return Rb::Waiting;
    if (!link_rebounding) {
      if (!reverse_seen) return Rb::Waiting;
      cp = Cp::None, reverse_seen = false, ++rebound_lost;
      return Rb::Lost;
    }
    if (!reverse_seen) return reverse_seen = true, reverse_tick = now, reverse_frame = frame, Rb::Seen;
    if (now <= reverse_tick) return Rb::Seen;
    if (!(frame > reverse_frame)) {
      cp = Cp::None, reverse_seen = false, ++rebound_lost;
      return Rb::Lost;
    }
    reverse_seen = false;
    return confirm(now, true, p2, shield) ? Rb::Confirmed : Rb::None;
  }
  // The scheduled counter is due: its plan index, ONCE (the adapter starts it, or drops it when the context changed).
  int due(std::uint64_t now) noexcept {
    if (cp != Cp::Scheduled || now < counter_at) return -1;
    cp = Cp::None;
    return counter_idx;
  }
  // A break / defeat preempts: no window, no counter; a rebound still awaiting its confirmation keeps only its credit (the parry was real).
  void cancel() noexcept {
    armed = false;
    if (cp == Cp::Scheduled) ++counter_dropped;
    cp = cp == Cp::Await ? Cp::CreditOnly : Cp::None;
  }
  void reset_exchange() noexcept { armed = false, cp = Cp::None, chain = 0, whiffs = 0, last_swing = 0, cur_contacted = false, reverse_seen = false; }
};

// ---------------------------------------------------------------- FIX05: who owns the exchange, from where a swing counts, when it may still be rolled
// A NEW Link swing in front: tracked (ordinal / whiff of Link's chain) within kSwingTrackReach, rolled (its ONE verdict) within kGuardReach.
enum class Sw : std::uint8_t { Ignore = 0, Track, Roll };
inline Sw swing_reach(float dist, bool front) noexcept {
  if (!front || !(dist < policy::kSwingTrackReach)) return Sw::Ignore;
  return dist < policy::kGuardReach ? Sw::Roll : Sw::Track;
}
// A registered, not yet rolled swing that comes into kGuardReach while still young may get its one verdict (a step-in cut); older: never (no late window).
inline bool roll_late_ok(std::uint64_t now, std::uint64_t registered) noexcept { return now >= registered && now - registered <= policy::kRollLateTicks; }
// The exchange Link's chain owns (no offensive decision over it), with the reason it does not. Distance alone never releases a valid chain inside the
// initiative reach: the Darknut could not start an offensive farther away anyway (NoInit::Far), so the gap between kGuardReach and kEngage (the native
// guard knockback, a step-in) can no longer hand the initiative over 8 ticks after a block.
enum class Hold : std::uint8_t { Holds = 0, NoDefense, NoTarget, LinkBroken, Far, NotFront, EmptySwings, Pause, Count };
inline const char* name(Hold h) noexcept {
  static constexpr const char* n[] = {"HOLDS", "NO_DEFENSE_LEFT", "NO_TARGET", "LINK_GUARD_BROKEN", "LINK_LEFT_REACH", "NOT_FRONT", "EMPTY_SWINGS", "PAUSE_OVER_1_5S"};
  return unsigned(h) < unsigned(Hold::Count) ? n[unsigned(h)] : "?";
}
inline Hold hold_verdict(const Defense& d, std::uint64_t now, bool can_defend, bool link_ok, bool link_broken, float dist, bool front, bool phase2) noexcept {
  if (!can_defend) return Hold::NoDefense;
  if (!link_ok) return Hold::NoTarget;
  if (link_broken) return Hold::LinkBroken;
  if (!(dist < policy::hold_reach(phase2))) return Hold::Far;
  if (!front) return Hold::NotFront;
  if (d.last_swing && d.whiffs >= policy::kChainWhiffs) return Hold::EmptySwings;
  return d.holds(now) ? Hold::Holds : Hold::Pause;
}

// ---------------------------------------------------------------- FIX06: the initiative SEQUENCE an authentic enemy parry opens (one per actor/generation)
// Prep    the parry is confirmed (PROC_CUT_REVERSE persisting): the route was rolled ONCE (Defense::confirm, routes) and the move is scheduled (Cp::Scheduled)
// Counter the counter plays (owned move)            -> concluded legitimately: ONE roll, 80 % Bridge, 20 % the sequence ends (common combat)
// Bridge  the counter's natural recovery is over; the continuation combo starts at bridge_due (Link held by a forced state: it waits, bounded)
// Combo   the combo plays (the direct 60 % one or the 80 % continuation) -> its legitimate end closes the sequence
// While it lives the sequence OWNS the initiative: Link's B, ordinary block, parry, Shield Attack or jump never end, delay or replace it (no defense verdict,
// no exchange hold, no special, no common initiative over it). Only the legitimate exceptions end it (SqEnd), once, with everything it scheduled.
enum class Sq : std::uint8_t { None = 0, Prep, Counter, Bridge, Combo, Count };
inline const char* name(Sq s) noexcept {
  static constexpr const char* n[] = {"NONE", "PREP_AFTER_PARRY", "COUNTER", "BRIDGE_COUNTER_TO_COMBO", "COMBO"};
  return unsigned(s) < unsigned(Sq::Count) ? n[unsigned(s)] : "?";
}
enum class SqEnd : std::uint8_t { None = 0, Done, CounterOnly, Break, Phase, Defeat, NoTarget, Event, LinkBroken, LinkFar, Watchdog, Incapacitated, Refused, Dropped, Reset, Count };
inline const char* name(SqEnd e) noexcept {
  static constexpr const char* n[] = {"NONE", "DONE_NATURAL_RECOVERY", "COUNTER_ONLY_20", "POSTURE_BREAK", "PHASE_CHANGE", "DEFEAT", "NO_TARGET", "EVENT", "LINK_GUARD_BROKEN",
                                      "LINK_FAR", "WATCHDOG", "LINK_INCAPACITATED", "START_REFUSED", "SCHEDULE_DROPPED", "RESET"};
  return unsigned(e) < unsigned(SqEnd::Count) ? n[unsigned(e)] : "?";
}
// The exception an owned move's end reason means for the sequence (None = a legitimate, natural end).
inline SqEnd seq_end_of(const char* why) noexcept {
  auto eq = [](const char* a, const char* b) noexcept {
    while (*a && *a == *b) ++a, ++b;
    return *a == *b;
  };
  if (!why || eq(why, "NATIVE_END") || eq(why, "B1_RECOVERY")) return SqEnd::None;
  if (eq(why, "POSTURE_BREAK")) return SqEnd::Break;
  if (eq(why, "PHASE_CHANGE")) return SqEnd::Phase;
  if (eq(why, "DEFEAT")) return SqEnd::Defeat;
  if (eq(why, "NO_TARGET")) return SqEnd::NoTarget;
  if (eq(why, "EVENT")) return SqEnd::Event;
  if (eq(why, "LINK_GUARD_BROKEN")) return SqEnd::LinkBroken;
  if (eq(why, "LINK_FAR")) return SqEnd::LinkFar;
  if (eq(why, "WATCHDOG")) return SqEnd::Watchdog;
  return SqEnd::Refused;  // ACTION_LOST and anything else: the native took the actor
}
struct Seq {
  Rng rng;  // its own stream (the continuation roll + its combo pick): never cM_rnd, never the defense / route / initiative streams
  Sq st = Sq::None;
  bool direct = false;  // the 60 % route
  bool held = false;    // Link was held by a forced state during the bridge (then the combo waits kBridgeFreeTicks after he is free)
  int idx = -1;         // the scheduled / running plan of the sequence
  std::uint64_t parry_tick = 0, since = 0, due_at = 0, counter_end = 0, last_impact = 0, free_since = 0;
  SqEnd why = SqEnd::None;
  std::uint64_t begun = 0, direct_n = 0, counter_n = 0, counters_done = 0, cont_rolls = 0, cont_combo = 0, cont_exit = 0, combos_started = 0, combos_done = 0, cont_pick[4] = {},
                ended[unsigned(SqEnd::Count)] = {};
  bool owns() const noexcept { return st != Sq::None; }
  // The authentic parry was confirmed and its ONE route rolled (Defense::confirm): from now on the sequence owns the initiative.
  bool begin(std::uint64_t now, std::uint64_t parry, bool route_combo, int plan_idx) noexcept {
    if (st != Sq::None || plan_idx < 0) return false;  // never stacked over a live sequence
    st = Sq::Prep, direct = route_combo, idx = plan_idx, parry_tick = parry, since = now, due_at = 0, counter_end = last_impact = free_since = 0, why = SqEnd::None;
    ++begun, ++(route_combo ? direct_n : counter_n);
    return true;
  }
  // The scheduled move started (Prep -> Counter / Combo) or the continuation combo started (Bridge -> Combo).
  bool started(bool combo) noexcept {
    if (!(st == Sq::Prep || (st == Sq::Bridge && combo))) return false;
    st = combo ? Sq::Combo : Sq::Counter;
    combos_started += combo;
    return true;
  }
  // The counter concluded LEGITIMATELY (its natural end): ONE roll. true = the continuation (Bridge, idx = one of the phase's four combos, due_at), false = the
  // sequence ends here (20 %: the common combat takes over).
  bool counter_done(std::uint64_t now, std::uint64_t last, bool p2) noexcept {
    if (st != Sq::Counter) return false;
    ++counters_done, ++cont_rolls, counter_end = now, last_impact = last ? last : now;
    if (!rng.pct(policy::kCounterComboPct)) return ++cont_exit, finish(SqEnd::CounterOnly), false;
    const unsigned k = rng.below(4);
    ++cont_pick[k], ++cont_combo;
    idx = static_cast<int>((p2 ? 4u : 0u) + k), due_at = bridge_due(counter_end, last_impact, static_cast<unsigned>(idx)), free_since = 0, held = false, st = Sq::Bridge;
    return true;
  }
  // Bridge, one tick: may the continuation start now? link_held = Link in a forced state (slip, damage, knock-down). 1 = start, 0 = wait, -1 = drop.
  int bridge_tick(std::uint64_t now, bool link_held) noexcept {
    if (st != Sq::Bridge) return -1;
    if (link_held) {
      held = true, free_since = 0;
      return now > counter_end + policy::kBridgeHoldMax ? -1 : 0;
    }
    if (held && !free_since) free_since = now;
    if (now < due_at) return 0;
    return !held || now >= free_since + policy::kBridgeFreeTicks ? 1 : 0;
  }
  void combo_done() noexcept {
    if (st != Sq::Combo) return;
    ++combos_done;
    finish(SqEnd::Done);
  }
  void finish(SqEnd e) noexcept {
    if (st == Sq::None) return;
    why = e, ++ended[unsigned(e) < unsigned(SqEnd::Count) ? unsigned(e) : 0], st = Sq::None, direct = false, idx = -1;
  }
  bool watchdog(std::uint64_t now) const noexcept { return st != Sq::None && now > since + policy::kSeqWatchdogTicks; }
};
// The per-tick authority of the sequence in the action PRE (after the break / lease checks and the scheduled start of Defense::due). Its ONLY inputs are
// the sequence, the scheduled move and the legitimate context: nothing Link does with his sword or shield (B, block, parry, Shield Attack, jump) is an input.
// Link held by a forced state (slip / damage / knock-down) only delays the continuation, bounded by kBridgeHoldMax.
enum class Gate : std::uint8_t { Open = 0, Own, StartContinuation, End, Count };
inline const char* name(Gate g) noexcept {
  static constexpr const char* n[] = {"OPEN", "OWN", "START_CONTINUATION", "END"};
  return unsigned(g) < unsigned(Gate::Count) ? n[unsigned(g)] : "?";
}
struct GateIn {
  bool link_ok = true, link_broken = false, safe = true, link_held = false;
  float dist = 0;
  std::uint64_t now = 0;
  Cp cp = Cp::None;
};
struct GateOut {
  Gate g = Gate::Open;
  SqEnd why = SqEnd::None;
};
inline SqEnd context_exception(const GateIn& in) noexcept {
  if (!in.link_ok) return SqEnd::NoTarget;
  if (in.link_broken) return SqEnd::LinkBroken;
  if (!in.safe) return SqEnd::Event;
  if (!(in.dist <= policy::kComboLeaveDist)) return SqEnd::LinkFar;
  return SqEnd::None;
}
inline GateOut seq_gate(Seq& q, const GateIn& in) noexcept {
  GateOut o;
  if (!q.owns()) return o;
  o.g = Gate::End;
  if (q.watchdog(in.now)) return o.why = SqEnd::Watchdog, o;
  if (q.st == Sq::Prep) return in.cp == Cp::Scheduled ? (o.g = Gate::Own, o) : (o.why = SqEnd::Dropped, o);
  if (q.st != Sq::Bridge) return o.why = SqEnd::Refused, o;  // Counter / Combo whose owned move is gone without its end: never an orphan owner
  const SqEnd e = context_exception(in);
  if (e != SqEnd::None) return o.why = e, o;
  const int b = q.bridge_tick(in.now, in.link_held);
  if (b < 0) return o.why = SqEnd::Incapacitated, o;
  return o.g = b > 0 ? Gate::StartContinuation : Gate::Own, o;
}

// ---------------------------------------------------------------- FIX06: reposition after the FIX02 rise / a recoverable getup (phases 2-3)
// The native jump physics, flat ground (fopAcM_posMoveF: speed.y += gravity then pos += speed; initChaseL zeroes speedF at the landing): the horizontal
// displacement and the ticks from the jump's start. Back: speedF 50 / speed.y 19 / gravity -4 from frame 4; forward: 35 / 4 / -1 from frame 4, landing
// accepted only once its 9-frame take-off clip stopped.
struct Jump { float dist = 0; unsigned air = 0, ticks = 0; };
inline Jump jump_model(bool back) noexcept {
  Jump j;
  const float sf = back ? 50.0f : 35.0f, g = back ? -4.0f : -1.0f;
  float vy = back ? 19.0f : 4.0f, y = 0;
  const unsigned takeoff = 4, clip_end = static_cast<unsigned>(native::kEndTnbJump1);
  unsigned t = takeoff;
  do {
    vy += g, y += vy, j.dist += sf, ++j.air, ++t;
  } while ((y > 0.0f || (!back && t < clip_end)) && j.air < 60);
  j.ticks = t + static_cast<unsigned>(native::kEndTnbJump2);  // + the landing clip (7 frames at the native rate)
  return j;
}
// FIX07 appends ForwardLong (native leap 17), WaitFront (the native turn first), GeoUnsafe (the trajectory check refused every jump).
enum class Rp : std::uint8_t { None = 0, Back, Hold, Forward, ForwardTooClose, Unsafe, ForwardLong, WaitFront, GeoUnsafe, Count };
inline const char* name(Rp r) noexcept {
  static constexpr const char* n[] = {"NONE", "JUMP_BACK", "HOLD", "JUMP_FORWARD", "HOLD_FORWARD_WOULD_LAND_ON_LINK", "HOLD_UNSAFE", "JUMP_FORWARD_LONG",
                                      "WAIT_FRONT_NATIVE_TURN", "HOLD_GROUND_UNSAFE"};
  return unsigned(r) < unsigned(Rp::Count) ? n[unsigned(r)] : "?";
}
// ONE decision per completed rise / getup. dist = XZ to Link (dkn_dist), dy = Link's height - the Darknut's, safe = no event, Link alive, on the ground,
// no wall contact for a back jump (front = Link in the front arc for a forward one).
inline Rp repo_verdict(float dist, float dy, bool safe, bool wall, bool front) noexcept {
  if (dist < policy::kRepoNear) return !safe || wall || (dy < 0 ? -dy : dy) > policy::kRepoMaxDy ? Rp::Unsafe : Rp::Back;
  if (dist <= policy::kRepoFar) return Rp::Hold;
  if (!safe || !front || (dy < 0 ? -dy : dy) > policy::kRepoMaxDy) return Rp::Unsafe;
  return dist - jump_model(false).dist >= policy::kRepoNear ? Rp::Forward : Rp::ForwardTooClose;
}

// ---------------------------------------------------------------- FIX07 (0.13.16): native jump physics of every leap the module may ask for
// [S executeChaseL 6 / 8 / 17, 2.0.3]: back (6) speedF 50 / speed.y 19 / gravity -4 from the take-off frame 4 of TNB_JUMP_B_1; forward (8) 35 / 4 / -1 and long
// forward (17) 45 / 5 / -1 from frame 4 of TNB_JUMP_F_1, the landing accepted only once that 9-frame clip stopped. Flat ground, displacement of the Darknut itself.
enum class Leap : std::uint8_t { None = 0, Back, Fwd, FwdLong, Count };
inline const char* name(Leap l) noexcept {
  static constexpr const char* n[] = {"NONE", "BACK_MODE6", "FORWARD_MODE8", "FORWARD_LONG_MODE17"};
  return unsigned(l) < unsigned(Leap::Count) ? n[unsigned(l)] : "?";
}
inline int leap_mode(Leap l) noexcept { return l == Leap::Back ? native::kChaseJumpBack : l == Leap::Fwd ? native::kChaseJumpFwd : l == Leap::FwdLong ? native::kChaseJumpFwdLong : -1; }
inline Jump leap_model(Leap l) noexcept {
  Jump j;
  if (l == Leap::None) return j;
  const bool back = l == Leap::Back;
  const float sf = back ? 50.0f : l == Leap::Fwd ? 35.0f : 45.0f, g = back ? -4.0f : -1.0f;
  float vy = back ? 19.0f : l == Leap::Fwd ? 4.0f : 5.0f, y = 0;
  const unsigned takeoff = 4, clip_end = static_cast<unsigned>(native::kEndTnbJump1);
  unsigned t = takeoff;
  do {
    vy += g, y += vy, j.dist += sf, ++j.air, ++t;
  } while ((y > 0.0f || (!back && t < clip_end)) && j.air < 60);
  j.ticks = t + static_cast<unsigned>(native::kEndTnbJump2);
  return j;
}
// The trajectory of one leap, read from the world by the adapter with the native queries (fopAcM_gc_c::gndCheck at three points of the path + the landing,
// fopAcM_lc_c::lineCheck along it): ground under every sample, no drop / rise beyond the bounds, no wall or obstacle, no water over the landing.
enum class Geo : std::uint8_t { Ok = 0, NoGround, Drop, Rise, Wall, Water, NotChecked, Count };
inline const char* name(Geo g) noexcept {
  static constexpr const char* n[] = {"OK", "NO_GROUND", "DROP", "RISE", "WALL", "WATER", "NOT_CHECKED"};
  return unsigned(g) < unsigned(Geo::Count) ? n[unsigned(g)] : "?";
}
namespace policy {
inline constexpr float kLeapMaxDrop = 80.0f;     // [PROVISIONAL] FIX07: a sample whose ground lies this far under the Darknut = a ledge / pit: no leap
inline constexpr float kLeapMaxRise = 50.0f;     // [PROVISIONAL] FIX07: ground this much higher = a step / slope the native leap would hit
inline constexpr float kLeapProbeUp = 100.0f;    // the ground probe starts this much above the Darknut's feet (fopAcM_gc_c::gndCheck casts downward)
inline constexpr float kLeapBodyR = 50.0f;       // the landing sample is taken this much beyond the predicted landing (the Darknut's body)
inline constexpr float kLeapLineY = 60.0f;       // the obstacle line runs at waist height
}  // namespace policy
// One sample of the path: ground height (no_ground = the native query found none), against the Darknut's feet.
inline Geo geo_sample(bool no_ground, float ground_y, float feet_y) noexcept {
  if (no_ground) return Geo::NoGround;
  if (feet_y - ground_y > policy::kLeapMaxDrop) return Geo::Drop;
  if (ground_y - feet_y > policy::kLeapMaxRise) return Geo::Rise;
  return Geo::Ok;
}
// The verdict of a whole path: the first failing sample, then the obstacle line, then water over the landing.
inline Geo geo_path(const Geo* samples, unsigned n, bool wall, bool water) noexcept {
  for (unsigned i = 0; i < n; ++i)
    if (samples[i] != Geo::Ok) return samples[i];
  if (wall) return Geo::Wall;
  if (water) return Geo::Water;
  return Geo::Ok;
}

// ---------------------------------------------------------------- FIX07: the OFFENSIVE leap (approved): Link far, ONE decision per opportunity
// Phases 2-3 only (phase 1 has no native leap clip for the armour / mace: TNB_JUMP_* are the sword-phase clips; executeChaseH has no jump: see
// IMPLEMENTATION_0_13_16.md, F1 keeps its native walking approach). When Link is beyond the initiative reach, at a distance where one native forward leap
// lands the Darknut inside it without landing on Link, the opportunity is rolled ONCE: 90 % the leap followed by one of the phase's four existing combos
// (25 / 25 / 25 / 25), 10 % the normal approach (nothing is changed: the native walk and the ordinary initiative). No roll per tick, no reroll on the
// threshold: an opportunity needs kLeapDebounceTicks in a row, and after a roll the next one waits for the episode to close (Link came within the
// initiative reach, or kLeapEpisodeTicks passed) AND kLeapCooldownTicks.
namespace policy {
inline constexpr int kLeapPct = 90;                  // approved: 90 % leap + combo, 10 % normal approach
inline constexpr float kLeapLandMin = 200.0f;        // [PROVISIONAL] never land closer to Link than this (bodies 40 + 60, the attack needs room)
inline constexpr float kLeapLandIdeal = 285.0f;      // [PROVISIONAL] when both native leaps qualify: the one landing nearer this (inside kEngage2)
inline constexpr unsigned kLeapDebounceTicks = 5;    // [PROVISIONAL] the eligible distance / state held this long before the ONE roll
inline constexpr unsigned kLeapCooldownTicks = 150;  // [PROVISIONAL] 5 s between two rolls of one Darknut
inline constexpr unsigned kLeapEpisodeTicks = 300;   // [PROVISIONAL] 10 s without coming close also closes an episode
inline constexpr unsigned kLeapApproachTicks = 45;   // [PROVISIONAL] after the landing: Link out of reach / not in front this long = he escaped: cancel
inline constexpr unsigned kLeapWatchdogTicks = 120;  // [PROVISIONAL] safety of the whole leap phase (take-off, air, landing, approach)
inline constexpr float kLeapMaxDy = 120.0f;          // = kRepoMaxDy: Link this much higher / lower: no leap
inline constexpr int kLeapArc = 0x2000;              // Link within 45 degrees of the facing (the native leap turns 0x400 per tick at most)
}  // namespace policy
// The native leap (forward 8 / long 17) whose predicted landing lands inside [kLeapLandMin, kEngage2], the one nearer kLeapLandIdeal; None if neither does.
inline Leap leap_pick(float dist) noexcept {
  Leap best = Leap::None;
  float bd = 1.0e9f;
  for (unsigned li = 0; li < 2; ++li) {
    const Leap l = li == 0 ? Leap::Fwd : Leap::FwdLong;
    const float land = dist - leap_model(l).dist;
    if (land < policy::kLeapLandMin || land > policy::kEngage2) continue;
    const float e = land > policy::kLeapLandIdeal ? land - policy::kLeapLandIdeal : policy::kLeapLandIdeal - land;
    if (e < bd) bd = e, best = l;
  }
  return best;
}
enum class Gc : std::uint8_t { Wait = 0, NotEligible, Roll, Count };
enum class GcPick : std::uint8_t { None = 0, LeapCombo, Normal, Count };
inline const char* name(GcPick p) noexcept {
  static constexpr const char* n[] = {"NONE", "LEAP_COMBO_90", "NORMAL_APPROACH_10"};
  return unsigned(p) < unsigned(GcPick::Count) ? n[unsigned(p)] : "?";
}
struct GapView {
  bool phase2 = true, free = true, link = true, safe = true, front = true;
  float dist = 0, dy = 0;
  std::uint64_t now = 0;
};
struct GapOut {
  Gc g = Gc::NotEligible;
  GcPick pick = GcPick::None;
  Leap leap = Leap::None;
  int combo = -1;
};
struct GapCloser {
  Rng rng;  // its own stream per actor / generation: never cM_rnd, never the defense / route / sequence / initiative streams
  bool armed = true, closed = false;
  unsigned held = 0;
  std::uint64_t last_roll = 0;
  std::uint64_t rolls = 0, leaps = 0, normals = 0, pick[4] = {}, rearmed = 0, geo_refused = 0, f1_refused = 0;
  // Every tick of an admitted Darknut (any state): the episode bookkeeping. close = Link within the initiative reach.
  void observe(std::uint64_t now, float dist) noexcept {
    if (armed) return;
    if (dist <= policy::kEngage2 || now >= last_roll + policy::kLeapEpisodeTicks) closed = true;
    if (closed && now >= last_roll + policy::kLeapCooldownTicks) armed = true, closed = false, held = 0, ++rearmed;
  }
  // A decision point (the adapter's ordinary, unowned neutral state). Roll = the ONE roll of this opportunity happened now (its pick, leap, combo).
  GapOut decide(const GapView& v) noexcept {
    GapOut o;
    const bool eligible = v.phase2 && v.free && v.link && v.safe && v.front && armed && (v.dy < 0 ? -v.dy : v.dy) <= policy::kLeapMaxDy && v.dist > policy::kEngage2;
    const Leap l = eligible ? leap_pick(v.dist) : Leap::None;
    if (l == Leap::None) {
      held = 0;
      return o;
    }
    if (++held < policy::kLeapDebounceTicks) return o.g = Gc::Wait, o;
    o.g = Gc::Roll, armed = false, closed = false, held = 0, last_roll = v.now, ++rolls;
    if (!rng.pct(policy::kLeapPct)) return ++normals, o.pick = GcPick::Normal, o;
    const unsigned k = rng.below(4);
    ++leaps, ++pick[k];
    o.pick = GcPick::LeapCombo, o.leap = l, o.combo = static_cast<int>(4u + k);
    return o;
  }
  // The geometry refused the leap the roll chose: the roll stays consumed (no reroll), the Darknut approaches normally.
  void refused() noexcept { ++geo_refused; }
};
// The leap phase owned by the actor / generation: Air (take-off, flight, the native landing clip), Ground (the native chase faces Link; bounded) -> the combo.
// The last step-in is the combo's own wind-up distance correction (the same as every own initiative started within kEngage2).
enum class Lp : std::uint8_t { None = 0, Air, Ground, Count };
inline const char* name(Lp l) noexcept {
  static constexpr const char* n[] = {"NONE", "AIR", "GROUND_APPROACH"};
  return unsigned(l) < unsigned(Lp::Count) ? n[unsigned(l)] : "?";
}
// Ground phase, one tick: start the combo (Link within the initiative reach and in front), keep approaching, or cancel (Link escaped, timeout, watchdog).
enum class LpStep : std::uint8_t { Wait = 0, StartCombo, CancelFar, CancelTimeout, CancelWatchdog, Count };
inline const char* name(LpStep s) noexcept {
  static constexpr const char* n[] = {"WAIT_NATIVE_CHASE", "START_COMBO", "CANCEL_LINK_FAR", "CANCEL_APPROACH_TIMEOUT", "CANCEL_WATCHDOG"};
  return unsigned(s) < unsigned(LpStep::Count) ? n[unsigned(s)] : "?";
}
inline LpStep leap_ground(float dist, bool front, unsigned ground_ticks, unsigned total_ticks) noexcept {
  if (total_ticks > policy::kLeapWatchdogTicks) return LpStep::CancelWatchdog;
  if (!(dist <= policy::kComboLeaveDist)) return LpStep::CancelFar;
  if (dist <= policy::kEngage2 && front) return LpStep::StartCombo;
  if (ground_ticks >= policy::kLeapApproachTicks) return LpStep::CancelTimeout;
  return LpStep::Wait;
}

// ---------------------------------------------------------------- FIX07: the reposition after the rise / getup, reviewed (same bands: < 240 back,
// 240..370 hold, > 370 forward). Additions: the trajectory of EVERY jump (back included: FIX06 never checked ground behind) must pass geo_path; Link not in
// front first gets a short native turn (the ChaseL chase / strafe faces him) instead of a refusal; the long native leap (17) is used where the short one would
// stop too far. 370..485: every native leap would land on Link: no jump (FIX06's hold, the native chase goes on): a "short walk" there is not native
// (executeChaseL mode 3 hands over to mode 1 as soon as Link is within 700, and fopAcM_posMoveF runs inside action(): only a non-native per-tick push could
// do it). One decision per rise / getup.
namespace policy {
inline constexpr unsigned kRepoTurnTicks = 20;      // [PROVISIONAL] Link not in front: wait at most this long for the native turn, then decide once
inline constexpr float kRepoLandIdeal = 305.0f;     // [PROVISIONAL] the middle of the hold band: the forward leap landing nearer it
}  // namespace policy
inline Leap repo_forward_pick(float dist, bool geo8, bool geo17) noexcept {
  Leap best = Leap::None;
  float bd = 1.0e9f;
  for (unsigned li = 0; li < 2; ++li) {
    const Leap l = li == 0 ? Leap::Fwd : Leap::FwdLong;
    if (!(l == Leap::Fwd ? geo8 : geo17)) continue;
    const float land = dist - leap_model(l).dist;
    if (land < policy::kRepoNear) continue;
    const float e = land > policy::kRepoLandIdeal ? land - policy::kRepoLandIdeal : policy::kRepoLandIdeal - land;
    if (e < bd) bd = e, best = l;
  }
  return best;
}
// turn_left = the native turn may still run (Link not in front yet): WAIT_FRONT; at its end a Link still not in front = HOLD (no jump sideways / backwards).
inline Rp repo_verdict7(float d, float dy, bool safe, bool front, bool turn_left, bool geo_back, bool geo8, bool geo17) noexcept {
  const float ady = dy < 0 ? -dy : dy;
  if (d < policy::kRepoNear) return !safe || ady > policy::kRepoMaxDy ? Rp::Unsafe : geo_back ? Rp::Back : Rp::GeoUnsafe;
  if (d <= policy::kRepoFar) return Rp::Hold;
  if (!safe || ady > policy::kRepoMaxDy) return Rp::Unsafe;
  if (!front) return turn_left ? Rp::WaitFront : Rp::Unsafe;
  if (d - leap_model(Leap::Fwd).dist < policy::kRepoNear) return Rp::ForwardTooClose;  // 370..485: any native leap would land on Link
  const Leap l = repo_forward_pick(d, geo8, geo17);
  if (l == Leap::None) return Rp::GeoUnsafe;
  return l == Leap::Fwd ? Rp::Forward : Rp::ForwardLong;
}

// ---------------------------------------------------------------- FIX07: the mace thrown in the transformation (approved, ACT_CHANGEDEMO only)
// [S executeZakoChangeDemo mode 4 frame 23: mStates[13] = 2, setSwordAtBit(1); calcSwordMoveA 2 -> 3 (80 units/tick toward Link + 200), 3 -> 7 at its ground /
// wall hit (mType 1: disappear); cc_set anchors mSwordSphs / mCps on the flying mace ONLY while mStates[13] == 3 and the sword is not in hand (mStates[15] != 1:
// from TNB_PULL frame 10 the same colliders follow the drawn sword)]. One contact of that projectile with Link, ONE resolution per throw:
//   block (ordinary shield) +50 Link, direct +60 Link (native life damage kept), parry (Link's Shield Attack in the shared window) +0 and no life damage,
//   a dodge with valid i-frames never makes a contact (src/ldg_core.hpp), miss 0. The Darknut's posture never moves for it; no initiative, no reaction.
namespace policy {
inline constexpr int kMaceBlockLink = 50, kMaceDirectLink = 60;  // approved
}
enum class Mace : std::uint8_t { None = 0, Block, Direct, Parry, Count };
inline const char* name(Mace m) noexcept {
  static constexpr const char* n[] = {"NONE", "BLOCK", "DIRECT", "PARRY"};
  return unsigned(m) < unsigned(Mace::Count) ? n[unsigned(m)] : "?";
}
inline Mace mace_verdict(bool contact, bool shield, bool parry_window) noexcept {
  if (!contact) return Mace::None;
  if (!shield) return Mace::Direct;
  return parry_window ? Mace::Parry : Mace::Block;
}
inline int mace_link_points(Mace m) noexcept { return m == Mace::Block ? policy::kMaceBlockLink : m == Mace::Direct ? policy::kMaceDirectLink : 0; }
inline int mace_darknut_points(Mace) noexcept { return 0; }  // the mace was thrown: never the Darknut's posture
// The projectile identity: a throw serial (the 2 -> 3 transition of mStates[13] in the transformation), live while cc_set anchors the colliders on it.
struct MaceLedger {
  std::uint64_t serial = 0, resolved = 0;
  bool live = false;
  std::uint64_t throws = 0, blocks = 0, directs = 0, parries = 0, repeats = 0;
  void thrown() noexcept { ++serial, live = true, ++throws; }
  void landed() noexcept { live = false; }
  // ONE resolution per throw: true = this contact is the throw's one verdict; a second contact of the same throw (bounce, another collider) = false.
  bool resolve(Mace m) noexcept {
    if (!serial || m == Mace::None) return false;
    if (resolved == serial) return ++repeats, false;
    resolved = serial, ++(m == Mace::Block ? blocks : m == Mace::Direct ? directs : parries);
    return true;
  }
};

// ---------------------------------------------------------------- FIX06: the sword darkens with the body, ONLY in the definitive death
// draw() writes mMaterialColor into the body's TEV color 0 when it is non-zero (executeZakoEnding chases it to -50, 0.3 per tick). The sword gets the very
// same value at the same moment of draw() only when this instance's death is DEFINITIVE (stage DEAD: F3 HP0 or the F3 Finish); a recoverable fall / the
// first defeat (DKR01 keeps mMaterialColor at its saved value) never.
inline bool sword_tint(bool definitive, float material_color) noexcept { return definitive && material_color != 0.0f; }
inline short tint_value(float material_color) noexcept { return static_cast<short>(material_color); }  // the body's own conversion (float -> s16)

// ---------------------------------------------------------------- Shield Attack authority (FIX01 bug: the native ACT_GUARDL / GUARDH it forces)
enum class Sa : std::uint8_t { Reject = 0, HelmPrep, RejectRepeat, Count };
inline const char* name(Sa s) noexcept {
  static constexpr const char* n[] = {"SHIELD_ATTACK_REJECTED", "HELM_PREP", "SHIELD_ATTACK_REJECTED_REPEAT"};
  return unsigned(s) < unsigned(Sa::Count) ? n[unsigned(s)] : "?";
}
// Only a NEW real Shield Attack (Link entered PROC_GUARD_ATTACK after the break began, never used before) on a Darknut in its FIRST stagger prepares the
// Helm Splitter. Everything else is rejected: the native guard it forced is taken back, nothing else changes. FIX06: every phase (phase 1 approved).
inline Sa sa_verdict(bool phase2, Bk st, bool sa_new) noexcept {
  (void)phase2;
  if (st == Bk::StaggerA && sa_new) return Sa::HelmPrep;
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
// FIX06: the phase-1 verdict of the same contact (md / helm as above). true = the ONE punishment of this break: kF1ExecDamage HP TOTAL instead of the native
// damage of that hit; the remaining HP decides (0 = the native transformation, with absolute priority), never a fall.
inline bool exec_verdict_f1(bool phase2, Bk st, bool md, bool helm) noexcept { return !phase2 && ((st == Bk::StaggerA && md) || (st == Bk::StaggerB && helm)); }
// The phase-1 reserve after that punishment: exactly kF1ExecDamage less, saturated at 0 (no underflow).
inline int f1_exec_hp(int hp) noexcept { return hp > policy::kF1ExecDamage ? hp - policy::kF1ExecDamage : 0; }

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
// FIX05 (approved): phases 2-3 answer ONLY a really held shield (the kick, 38 ticks, cooldown 240); the idle response (no shield, 75 ticks) stays the
// phase-1 shield bash. An idle Link in F2/F3 is met by the ordinary initiative (a combo), never by an independent kick.
inline bool special_allowed(Pv v, bool phase2) noexcept { return v == Pv::Shield || (v == Pv::Idle && !phase2); }
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
