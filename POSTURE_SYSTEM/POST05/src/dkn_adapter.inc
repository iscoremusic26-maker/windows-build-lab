// LIGHT FADES TWICE 0.13.13 DARKNUT MINIBOSS FIX04 (DKN01 FIX04; FIX03 0.13.12, FIX02 0.13.11, FIX01 0.13.10 base): production adapter of the normal Darknut of Enemy Spawner (daB_TN_c, parameters
// 0x000001FF, mType 1, mType2 0, switch 0xFF, no stay-no-2 flag: the DKR01 identity, dkr::eligible). Included once in mod.cpp's anonymous namespace after
// dkn_bridge.inc. The pure decisions live in dkn_core.hpp (tests/dkn_lab.cpp).
//
// Hook targets (all exported by dusklight.exe 2.0.3 and called for real, out of line, by action()/damage_check() - measured, see DKN01 IMPLEMENTATION.md):
//   daB_TN_c::action PRE/POST        per-tick life of the instance: ONE owner (break / execution / defense / offense), native attack starts suppressed,
//                                    the OWN initiative, adaptive guard + parry window, counter, passivity, regeneration, stagger hold, AT authority, spl
//   daB_TN_c::damage_check PRE/POST  480 threshold for setDamage, the Darknut's impacts on Link, Link's blocked attacks, the Shield Attack authority
//   daB_TN_c::setDamage PRE/POST     the native damage commit: 600 HP (FIX04) + armour (phase 1), the 480 accumulator (phases 2-3), Table A, the execution contact
//   daB_TN_c::executeAttackH/L, executeAttackShieldH/L PRE/POST   the move director (native clips, sounds, AT frames, turning) and the passivity specials
//   daB_TN_c::_delete PRE (a second callback on the DKR01 target)  the slot dies with the actor
//   (FIX01, callbacks on EXISTING targets, no new hook target)
//   daAlink_c::checkCutAction POST (after the Chilfos)  B on a broken Darknut (first stagger): the native Mortal Draw (kk_native_mortal_draw, the Chilfos writer)
//   daAlink_c::checkCutReverseAt POST (after the Chilfos)  this Darknut's spheres: TRUE kept only inside an armed parry window (the REAL parry), else FALSE
//   daAlink_c::checkDamageAction POST (callback, prio -402)  Link's native damage receipt (direct impacts)
// Every write to the native actor goes through the DKN_NOINLINE writers below (ci/dkn_audit.py). Link: only the native Mortal Draw request (the Chilfos
// writer) and, FIX02, dkn_link_slip_speed: the speed of a guard slip THIS Darknut's impact caused, capped (never raised), in checkDamageAction POST.
// FIX02 (0.13.11): the 0.13.10 crash (ACT_GUARDL 0 written without a hit -> native getCutType on a NULL collider) is closed in the writer itself:
// dkn_native_action never writes an (action, mode) that needs this pass' collision (dkn::native::needs_hit_context); the proactive guard enters at
// mode 1 with its own clip (dkn_native_guard_enter), exactly what executeGuardL mode 0 would have done, minus the collider read.
// FIX03 (0.13.12): the parry route in every phase (phase 1 = the shield: dkn_native_guard_enter_h), one roll per new Link swing by streak
// (10/25/45/60 %), Link +20/+25 posture on a REAL enemy parry, counters by thirds, Link's chained guard slips capped lower, F2/F3 tempo x1.25 (F1
// frozen), the native attack ENTRIES seen in the action POST counted as suppressed attempts, the second stagger on TNB_DAMAGE_R.
// FIX04 (0.13.13): F1 600 HP / 200 posture (+4 blocked, +6 direct, Link parries +12 / +14 / +14); Link's chain against a REAL defense owns the exchange
// (no offensive decision over it), parry by ordinal 0/0/0/25/45/60 bound to the swing key, the parry contact stays a block (never an HP hit), the credit
// and the ONE counter only after Link's PROC_CUT_REVERSE is read, the counter landing >= 24 ticks after the rebound; the Lizalfos parry FX (orange spark,
// high clash) in both directions; F2/F3 x1.40 with natural clip ranges; Link's slip counted once per impact (FIX03 realized profile kept).
// FIX05 (0.13.14): every REAL defended contact feeds Link's chain (a swing begun beyond the old 300 reach is registered at its contact); swings tracked
// within 500, rolled within 300 (a young swing stepping in may still get its ONE verdict); the exchange holds within the initiative reach (hold_verdict):
// the 8-tick OWN_INITIATIVE over a block (log 9731 / 12966 / 20811) is gone. F2/F3: the four combos only, the passivity kick only for a held shield. The
// rebound's credit / counter need Link's recoil read again one tick later with its frame advanced. The counter double plays as the F2-C1 opener double.
// Transformation: the first drawn-sword tick of executeZakoChangeDemo (mStates[15] = 1 at TNB_PULL frame 10) would close cc_set's mCps capsule from
// mCpsPos = the last position of the THROWN MACE (the weapon AT is on since TNB_THROW frame 23): a one-tick sweep across the arena (the remote damage; log:
// Link's guard slip 18 ticks before F2_START). That one tick runs with only the capsule AT off (dkn_native_at), the next tick gets the native setSwordAtBit(1)
// back: the capsule is re-anchored on the sword by cc_set itself. In F2/F3 a weapon AT left on outside every move (the transformation's, after F2_START)
// is switched off (the AT authority's own rule: off outside the windows). Closure: on the re-anchor tick only the stale capsule is disabled; the four
// physical sword spheres remain native, including their close contacts and pending receipts. The native whole-weapon off would erase those too.
// Ineligible actors (Darknut (Sword) 0x010001FF, Temple of Time mType 0, placed / Hyrule Castle variants) never get a slot: every callback returns at once.
// FIX06 (0.13.15): (1) F2-C2 / F2-C4: five approved final intervals (dkn::kIvTargets). (2) The SEQUENCE of an authentic enemy parry (dkn::Seq): 60 % a combo
// directly / 40 % a counter, a legitimately concluded counter continues with a combo 80 % of the time; the sequence owns the initiative from the confirmation
// to its last recovery (no defense verdict, hold, special or common initiative over it; Link's blocks / hits keep their real damage and posture); only the
// legitimate exceptions end it, once, with what it scheduled. (3) Phase 1: Mortal Draw (the Chilfos writer) and Shield Attack -> Helm Splitter on a broken
// Darknut, 100 HP TOTAL, natural recovery, the transformation first at HP 0. (4) The second stagger sways (helm_frame) instead of freezing; phase 1 on its
// own native clip TNA_DAMAGE_R. (5) After the FIX02 rise / a recoverable getup: ONE native jump back / hold / jump forward (initChaseL 6 / 8 replicated, the
// landing attack taken back). (6) One more hook target, daB_TN_c::draw PRE / POST: the sword's TEV color 0 gets the body's mMaterialColor only in the
// definitive death, written before draw() and restored after it (this instance only).
// FIX08 (0.13.17): (1) after a non-lethal phase-1 Helm Splitter the recovery looks for Link: the native neck tracking (mUpdateNeckAngle, never set for mType 1)
// follows him after the hit read, the body turns toward him at the native ChaseH rate once the head is back, the neck lets go after a short tail (dkn::HelmLook).
// (2) F1 Table B 24/36, 31/43, 50/60. (4) The defensive evasion of phases 2-3 (dkn::Evade: 0 / 10 / 15 / 20 % by ordinal after the parry roll, cooldown 90, one roll
// per swing, the native side / back jumps 11 / 14 / 6 with their trajectory read first, own lease, landing attack taken back); the native evasion entries 6 / 11 / 14
// the native AI drew by itself are taken back. (5) The sword's darkening value (dkn::sword_tint_value). (7) No ordinary initiative over a running swing that can
// connect (swing_threat), and DEFENSE -> NEUTRAL -> ORDINARY_INIT (dkn::NeutralGate); a native attempt no longer bypasses the neutral state. (8) F1-C3 1->2 36 ticks.
// (9) B1 ends with the native eased morph into the combat stance (TNB_WAIT_B_2) instead of a 10-frame snap into TNB_WAIT.
#if defined(_MSC_VER)
#define DKN_NOINLINE __declspec(noinline)
#else
#define DKN_NOINLINE __attribute__((noinline))
#endif

enum class DknLease : std::uint8_t { None = 0, Combo, Single, Counter, Special, Reposition, Leap, Evade };  // FIX07: Leap = the offensive leap until its combo starts; FIX08: Evade
inline const char* dkn_lease_name(DknLease l) noexcept {
  static constexpr const char* n[] = {"NONE", "COMBO", "SINGLE", "COUNTER", "SPECIAL", "REPOSITION", "LEAP", "EVADE"};
  return n[unsigned(l) < 8 ? unsigned(l) : 0];
}
struct DknStage {  // telemetry per stage (F1 / F2 / F3)
  std::uint64_t opportunities = 0, decisions = 0, combo_sel[4] = {}, combo_start[4] = {}, combo_end[4] = {}, combo_cancel[4] = {}, singles = 0, counters = 0, specials = 0;
  std::uint64_t suppressed = 0, sa_rejected = 0, helm_preps = 0, guards = 0, parries = 0, breaks = 0, executions = 0, idle_ticks = 0, idle_n = 0, idle_max = 0;
};
struct DknSlot {
  bool on = false, admitted_logged = false;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  dkn::Ph ph = dkn::Ph::Armor;
  int hp1 = dkn::policy::kHp1Max;
  dkn::Posture post;
  dkn::Break brk;
  dkn::Passive pas;
  dkn::Initiative ini;
  dkn::Defense def;
  dkn::Ledger led;
  dkn::Receipt md_rcpt;
  // the ONE owned move (combo / single / counter / special)
  DknLease lease = DknLease::None;
  bool tail = false;
  unsigned combo_idx = 0, seg = 0, ct = 0, impacts = 0;
  dkn::Plan plan;
  dkn::Timeline tl;
  dkn::Move special_move = dkn::Move::F2C;
  std::uint64_t combo_tick = 0, impact_tick[9] = {};
  // executions
  bool exec = false, md_active = false, helm_active = false;
  std::uint64_t md_tick = 0, helm_tick = 0;
  unsigned getups_seen = 0;
  // Link tracking (one key per Link action; Shield Attack entries)
  std::uint64_t link_key = 0, blocked_key = ~0ull, swing_key = 0, sa_serial = 0, sa_used = 0, sa_tick = 0, guard_tick = 0, last_suppress_log = 0;
  int link_prev_proc = -1, link_prev_cut = -1, link_prev_count = -1;
  bool swing_new = false, parry_shield = false;
  // FIX02: the guard slip THIS Darknut's impact may cause (Link's checkDamageAction POST caps its speed). FIX04: one chain step per impact serial.
  bool slip_pending = false;
  std::uint64_t slip_tick = 0, slip_serial = 0, slip_done_serial = 0;
  float slip_cap_now = 0;
  dkn::Move slip_move = dkn::Move::F2A;
  // damage_check / setDamage snapshots of THIS call
  int pre_act1 = 0, pre_act2 = 0, sd_nbp = 0, sd_acc = 0, sd_act1 = 0, sd_act2 = 0, sd_cut = -1;
  bool sd_seen = false, dc_sa = false, dc_sword = false, sd_link_sword = false;
  // bookkeeping
  unsigned last_band = 99;
  int last_acc = 0;
  DknStage stg[3];
  std::uint64_t st_hp1 = 0, st_hp2 = 0, st_gain = 0, st_parry = 0, st_block_link = 0, st_direct_link = 0, st_blocked_by_dk = 0, st_reverted = 0, st_yoroke_blocked = 0,
                st_breaks = 0, st_punish = 0, st_combos = 0, st_combo_cancel = 0, st_specials = 0, st_chain_cut = 0, st_impacts = 0, st_armor_calls = 0, st_regen_ticks = 0,
                st_suppressed = 0, st_sa_rejected = 0, st_helm_prep = 0, st_md_start = 0, st_md_contact = 0, st_helm_start = 0, st_helm_contact = 0, st_exec = 0,
                st_exec_lethal = 0, st_getups = 0, st_guards = 0, st_enemy_parry = 0, st_counters = 0, st_ordinary_blocks = 0, st_headlock_off = 0,
                st_unauthorized = 0, st_leaps = 0, st_slip_capped = 0, st_slip_seen = 0, st_unsafe_entry = 0;
  // FIX03
  std::uint64_t last_slip_tick = 0, st_entry_post = 0, st_parry_gain = 0, st_parry_attempts = 0, st_kick_slowed = 0;
  unsigned slip_chain = 0;
  bool native_attempt = false;  // a native attack entry was taken back in the action POST: the next decision point is that attempt
  // FIX04: the exchange Link's chain owns, the parry credit, the tempo telemetry of the owned move (native hitstop pauses told apart)
  bool def_holding = false;
  std::uint64_t st_hold_ticks = 0, st_hold_releases = 0, st_hold_refused = 0, st_parry_unconfirmed = 0, st_counter_dropped = 0, st_rebound_no_route = 0;
  std::uint64_t mv_last_tick = 0, mv_paused = 0, mv_paused_at[9] = {};
  float mv_rate_min = 99.0f, mv_rate_max = 0.0f;
  // FIX05: the registered chain swing (late verdict bound), exchange telemetry, rebound persistence, the transformation re-anchor, orphan AT
  std::uint64_t swing_reg_tick = 0, swing_reg_key = 0;
  std::uint64_t st_late_registered = 0, st_late_rolls = 0, st_tracked_far = 0, st_hold_beyond_300 = 0, st_rebound_seen = 0, st_rebound_lost = 0, st_idle_special_refused = 0,
                st_at_orphan_off = 0, st_tf_reanchor = 0, st_tf_restore = 0;
  bool tf_reanchored = false, tf_restore = false;
  // FIX06: the initiative sequence of an authentic enemy parry, the reposition after a rise / getup, phase-1 executions, sword darkening
  dkn::Seq seq;
  bool repo_pending = false, repo_back = false;
  std::uint64_t repo_since = 0, repo_tick = 0;
  float repo_d0 = 0;
  dkn::Rp repo = dkn::Rp::None;
  std::uint64_t st_seq_bridge_held = 0, st_seq_logs = 0, st_repo_jumps = 0, st_repo_holds = 0, st_repo_land_attack = 0, st_repo_cancel = 0, st_f1_md_contact = 0,
                st_f1_helm_contact = 0, st_f1_helm_prep = 0, st_sword_tint_draws = 0, st_seq_hits_taken = 0, st_seq_blocks = 0;
  // FIX07: the offensive leap (90 / 10), the reviewed reposition, the thrown mace, the sword darkening where entryDL really diffs it
  dkn::GapCloser gap;
  dkn::Lp leap_phase = dkn::Lp::None;
  dkn::Leap leap = dkn::Leap::None, repo_leap = dkn::Leap::None;
  int leap_combo = -1;
  std::uint64_t leap_tick = 0, leap_land_tick = 0, leap_combo_tick = 0, leap_episode = 0, repo_turn_since = 0, repo_walk_tick = 0;
  unsigned leap_ground = 0;
  float leap_d0 = 0, leap_x0 = 0, leap_z0 = 0, leap_land_d = 0, repo_x0 = 0, repo_z0 = 0;
  bool leap_first_impact_logged = false, repo_walk = false, leap_f1_noted = false;
  dkn::MaceLedger mace;
  bool mace_anchor = false, mace_anchor_prev = false;
  std::uint64_t mace_direct_token = 0;
  std::uint64_t st_leap_start = 0, st_leap_land = 0, st_leap_combo = 0, st_leap_cancel = 0, st_leap_land_attack = 0, st_leap_native_cut = 0, st_leap_geo_refused = 0,
                st_repo_long = 0, st_repo_wait_front = 0, st_repo_approach = 0, st_repo_geo_unsafe = 0, st_sword_tint_entries = 0, st_mace_contacts = 0,
                st_mace_repeat = 0, st_ldg_evaded = 0;
  // FIX08: the Helm recovery look, the defensive evasion, the neutral receipt, B1's recovery, the swing seen, the parry FX routes, the sword darkening series
  dkn::HelmLook helm_look;
  bool neck_on = false;
  std::int16_t helm_err0 = 0;
  dkn::Evade evade;
  dkn::Ed evade_dir = dkn::Ed::None;
  std::uint64_t evade_tick = 0, evade_land_tick = 0, evade_key = 0;
  bool evade_link_close = false, evade_hit = false;
  float evade_d0 = 0, evade_x0 = 0, evade_z0 = 0;
  dkn::NeutralGate ngate;
  std::uint64_t swing_seen_tick = 0, st_b1_recover = 0, st_link_swing_refused = 0, st_evade_lines = 0, st_route_lines = 0;
  std::uint64_t fx_route[10] = {};  // FIX08: confirmed parries by route (0 F1 lateral, 1 F1 vertical, 2 F1 bash, 3 thrown mace, 4 F2 thrust, 5 double cut 1, 6 double cut 2, 7 B1, 8 kick, 9 Darknut's)
  std::uint64_t fx_route_enemy[2] = {};  // the Darknut's own parry: F1 shield / F2-F3 sword
  short tint_body_last = 1;
  unsigned tint_samples = 0;
};
struct DknState {
  bool enabled = false, hooks = false;
  DknSlot s[dkn::kSlots];
  std::uint64_t next_generation = 1, admitted = 0, ineligible = 0, full = 0, deletes = 0, unload_fixed = 0, dropped = 0, unsafe_entries = 0;
  std::uint32_t rejected_ids[16] = {};
  unsigned rejected_next = 0, lines = 0;
  std::uint64_t direct_tick = ~0ull;
  dkn::Plan plans[dkn::kPlans];
  dkn::Timeline tls[dkn::kPlans];
  // FIX06: the sword material values saved by the draw PRE of ONE daB_TN_c::draw call, restored by its POST (draw calls never nest)
  J3DModelData* tint_data = nullptr;
  short tint_saved[8][3] = {};
  unsigned tint_n = 0;
  std::uint64_t tint_draws = 0, tint_restores = 0;
  // FIX07: the draw PRE only ARMS the darkening of THIS instance's sword morf; the write happens inside that morf's own entryDL (after the native
  // setLightTevColorType_MAJI of the sword re-set TEV color 0 from tevStr, before mDoExt_modelEntryDL diffs it into the instance) and is undone in its POST.
  const mDoExt_McaMorfSO* tint_morf = nullptr;
  short tint_value = 0;
  DknSlot* tint_slot = nullptr;
  std::uint64_t tint_armed = 0, tint_entry_writes = 0, tint_entry_restores = 0, tint_overwritten_by_maji = 0, tint_unrestored = 0;
} dn;

void dkn_line(bool warn, const char* line) noexcept {
  if (dn.lines >= 9000) {  // FIX08: 6000 -> 9000 (the 0.13.16 human session used 5375 of 6000; FIX08 adds sampled evasion / receipt / route lines)
    ++dn.dropped;
    return;
  }
  ++dn.lines;
  dknb_write(warn, line);
}
unsigned dkn_index(const DknSlot* s) noexcept { return static_cast<unsigned>(s - &dn.s[0]); }
DknSlot* dkn_find(std::uint32_t id) noexcept {
  for (DknSlot& s : dn.s)
    if (s.on && s.id == id) return &s;
  return nullptr;
}
float dkn_frame(const daB_TN_c* a) noexcept { return a->mpModelMorf2->getFrame(); }
float dkn_dist(const daB_TN_c* a, const daAlink_c* k) noexcept {
  const float dx = k->current.pos.x - a->current.pos.x, dz2 = k->current.pos.z - a->current.pos.z;
  return kk::fsqrt(dx * dx + dz2 * dz2);
}
std::int16_t dkn_angle_to(const daB_TN_c* a, const daAlink_c* k) noexcept { return cM_atan2s(k->current.pos.x - a->current.pos.x, k->current.pos.z - a->current.pos.z); }
bool dkn_front(const daB_TN_c* a, const daAlink_c* k) noexcept {
  const int d = static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y);
  return (d < 0 ? -d : d) < dkn::policy::kFrontArc;
}
int dkn_broken_parts(const daB_TN_c* a) noexcept {
  int n = 0;
  for (int i = 0; i < dkn::native::kArmorParts; ++i) n += a->mStates[dkn::native::kBreakPartNo[i]] != 0;
  return n;
}
bool evade_native_entry_any(int act, int mode) noexcept { return dkn::native_evade_entry(act, mode); }
bool dkn_combat(const DknSlot& s) noexcept { return !s.exec && (s.ph == dkn::Ph::Armor || dkn::phase2(s.ph)); }
int dkn_max(const DknSlot& s) noexcept { return dkn::posture_max(s.ph); }
int dkn_remaining(const DknSlot& s, const daB_TN_c* a) noexcept { return dkn::phase2(s.ph) ? dkn::res2_remaining(a->field_0x6fc) : s.hp1; }
int dkn_life_max(const DknSlot& s) noexcept { return dkn::phase2(s.ph) ? dkn::policy::kRes2Max : dkn::policy::kHp1Max; }
DknStage& dkn_stage(DknSlot& s) noexcept { return s.stg[dkn::stage(s.ph)]; }
bool dkn_busy(const DknSlot& s) noexcept { return s.lease != DknLease::None || s.brk.st != dkn::Bk::Combat || s.exec; }
void dkn_log(const char* ev, const DknSlot& s, const daB_TN_c* a, const char* extra = "", bool warn = false) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "DARKNUT_%s actor=%u generation=%llu stage=%s hp1=%d/%d res=%d/%d posture=%d/%d break=%s lease=%s act=%d mode=%d frame=%.1f nbp=%d acc=%d %s tick=%llu", ev,
                s.id, static_cast<unsigned long long>(s.generation), dkn::name(s.ph), s.hp1, dkn::policy::kHp1Max, a ? dkn::res2_remaining(a->field_0x6fc) : -1, dkn::policy::kRes2Max,
                s.post.points(), s.post.max_pts, dkn::name(s.brk.st), dkn_lease_name(s.lease), a ? a->mActionMode1 : -1, a ? a->mActionMode2 : -1,
                a ? static_cast<double>(dkn_frame(a)) : -1.0, a ? a->mNextBreakPart : -1, a ? a->field_0x6fc : -1, extra, static_cast<unsigned long long>(dknb_tick()));
  dkn_line(warn, line);
}

void dkn_slip_cap(DknSlot& s, daAlink_c* k, const daB_TN_c* a, const char* where) noexcept;  // FIX02, defined with the action hooks
// ---------------------------------------------------------------- writers (the ONLY native writes of this module)
// 1/15: the per-actor phase-2/3 threshold, written in the damage_check PRE: setDamage (called only from damage_check) reads it; action() rewrites 360 after.
DKN_NOINLINE void dkn_native_threshold(daB_TN_c* a, int v) noexcept { a->field_0x700 = v; }
// 2/15: the native setActionMode, replicated (offHeadLockFlg + the two modes; ci/dkn_native_audit.py pins that body).
// FIX02 invariant: never an (action, mode) whose native entry reads this pass' collision (ACT_GUARDL 0 -> getCutType -> mAtInfo.mpCollider, NULL
// without a hit: the 0.13.10 crash). Such a request becomes the neutral chase of the stage (counted; never expected from this module any more).
DKN_NOINLINE void dkn_native_action(daB_TN_c* a, int act, int mode) noexcept {
  a->offHeadLockFlg();
  if (dkn::native::needs_hit_context(act, mode)) {
    ++dn.unsafe_entries;
    act = dkn::native::kActChaseL, mode = 1;
  }
  a->mActionMode1 = act;
  a->mActionMode2 = mode;
}
// 3/15: the armour: the native setBreakPart / setBreakHeadPart (sound + part physics) for a progress ABOVE what is broken, then mNextBreakPart.
DKN_NOINLINE void dkn_native_armor(daB_TN_c* a, int call, bool head, int floor) noexcept {
  if (call >= 0) {
    if (head) a->setBreakHeadPart(call);
    else a->setBreakPart(call);
  }
  a->mNextBreakPart = floor >= 12 ? 12 : dkn::armor_next(floor, dkn_broken_parts(a));
}
// 4/15: a native clip of the B_tn archive (setBck), its start frame, its rate.
DKN_NOINLINE void dkn_native_clip(daB_TN_c* a, int bck, float morph, float frame, float rate) noexcept {
  a->setBck(bck, 0, morph, rate);
  a->mpModelMorf2->setFrame(frame);
}
DKN_NOINLINE void dkn_native_wait_clip(daB_TN_c* a) noexcept { a->setBck(dkn::native::kBckTnbWait, 2, 10.0f, 1.0f); }
// 5/15: the J3D play rate of the body clip (the next play of action()).
DKN_NOINLINE void dkn_native_rate(daB_TN_c* a, float rate) noexcept { a->mpModelMorf2->mFrameCtrl.mRate = rate; }
// 6/15: the weapon / shield-kick AT of the open impact window (native setSwordAtBit for the weapon, the native mSphC bits for the shield and the kick).
DKN_NOINLINE void dkn_native_at(daB_TN_c* a, bool sword, int want, bool capsule_only = false) noexcept {
  if (sword && capsule_only) {  // transformation re-anchor only: leave the physical sword spheres and their receipts untouched
    a->mCps.OffAtSetBit();
    a->mCps.ClrAtHit();
  } else if (sword) a->setSwordAtBit(want);
  else if (want) a->mSphC.OnAtSetBit();
  else a->mSphC.OffAtSetBit();
}
// 7/15: spl 1 (a LARGE attack: native guard slip) on the weapon and shield/kick colliders, never 9/10/11 (native guard break before Link's 100).
DKN_NOINLINE void dkn_native_spl(daB_TN_c* a) noexcept {
  for (int i = 0; i < 4; ++i) a->mSwordSphs[i].SetAtSpl(static_cast<dCcG_At_Spl>(dkn::native::kSplLarge));
  a->mCps.SetAtSpl(static_cast<dCcG_At_Spl>(dkn::native::kSplLarge));
  a->mSphC.SetAtSpl(static_cast<dCcG_At_Spl>(dkn::native::kSplLarge));
}
// 8/15: the stagger is ours: no native timed guard (mTimer5 kept > 0, mTimer10 0), no cut-flag guard, no early ACT_YOROKE exit (mCutJumpStatus 0),
// not guard-ready (field_0xa91 false: every hit reaches setDamage, no TgShield).
DKN_NOINLINE void dkn_native_stagger_hold(daB_TN_c* a, bool entry) noexcept {
  if (entry) a->mTimer5 = 90;  // executeDamageH must never see mTimer5 0 (its native timed guard) during OUR stagger
  else if (a->mTimer5 < 2) a->mTimer5 = 2;
  a->mTimer10 = 0;
  a->mCutFlag = false;
  a->mCutJumpStatus = 0;
  a->field_0xa91 = false;
}
// 9/15: the parry window: the native forced-guard timer kept alive (setBodyShield -> mSphA TgShield, Link's sword rebounds by his own route) while the
// armed window lasts and Link stays in front; 0 never written (the native countdown closes it in <= 2 ticks).
DKN_NOINLINE void dkn_native_guard(daB_TN_c* a, unsigned ticks) noexcept {
  a->mTimer10 = static_cast<u8>(ticks > 255 ? 255 : ticks);
  a->field_0xa91 = true;
}
// 10/15: inside an owned move: no native B->A chain (mActionMode2Copy != 0), not guard-ready while attacking.
DKN_NOINLINE void dkn_native_attack_flags(daB_TN_c* a) noexcept {
  a->mActionMode2Copy = 1;
  a->field_0xa91 = false;
}
// 11/15: distance correction of a combo wind-up: forward speed along the facing, and the facing chased toward Link (bounded per tick).
DKN_NOINLINE void dkn_native_motion(daB_TN_c* a, float speed, std::int16_t facing) noexcept {
  a->speedF = speed;
  a->shape_angle.y = facing;
  a->current.angle.y = facing;
}
// 12/15: the native recovery wait after B1 (initChaseL(2): TNB_WAIT, 30 ticks), mTimer1.
DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept { a->mTimer1 = t; }
// 13/15 (FIX01): the Helm Splitter target: the native HeadLock flag ON only in the second stagger (Link's checkCutHeadState reads it), OFF everywhere else
// (the native phase-1 GUARDH-after-Shield-Attack opening included: no Helm Splitter executor in phase 1, no shortcut outside a break).
DKN_NOINLINE void dkn_native_headlock(daB_TN_c* a, bool on) noexcept {
  if (on) a->onHeadLockFlg();
  else a->offHeadLockFlg();
}
// 14/15 (FIX01): the ACT_ENDING 0 of an EXECUTION fall (after dkr_exec_request accepted it): the native executeZakoEnding fall, owned by DKR01.
DKN_NOINLINE void dkn_native_exec_fall(daB_TN_c* a) noexcept {
  a->setSwordAtBit(0);
  a->mSphC.OffAtSetBit();
  a->offHeadLockFlg();
  a->mActionMode1 = dkn::native::kActEnding;
  a->mActionMode2 = 0;
}
// 15/15 (FIX01): a native attack entry the native AI wrote (setActionMode(ATTACK*, entry)) taken back to the neutral chase BEFORE the attack runs
// (phase 2: ACT_CHASEL 1 = the native strafe choice; phase 1: ACT_CHASEH 0 = the native chase dispatch). Nothing of the attack has run yet.
// FIX02: the weapon and shield/kick AT are switched off too (the chase modes switch the sword AT off natively, never mSphC): a neutral body never hurts.
DKN_NOINLINE void dkn_native_neutral(daB_TN_c* a, bool phase2) noexcept {
  a->offHeadLockFlg();
  a->setSwordAtBit(0);
  a->mSphC.OffAtSetBit();
  a->mActionMode1 = phase2 ? dkn::native::kActChaseL : dkn::native::kActChaseH;
  a->mActionMode2 = phase2 ? 1 : 0;
}
// 16 (FIX02): the module's guard entry = executeGuardL mode 0 without its collider read: sword/kick AT off, not moving, guard-ready, the native guard
// clip (TNB_GUARD_A/B, once, morph 3), the facing chased toward Link (0x2000 at most, as the native), then ACT_GUARDL 1 (the native guard hold).
DKN_NOINLINE void dkn_native_guard_enter(daB_TN_c* a, int bck, std::int16_t facing) noexcept {
  a->setSwordAtBit(0);
  a->mSphC.OffAtSetBit();
  a->speedF = 0.0f;
  a->field_0xa91 = true;
  a->setBck(bck, 0, 3.0f, 1.0f);
  a->shape_angle.y = facing;
  a->current.angle.y = static_cast<std::int16_t>(facing + 0x8000);
  a->offHeadLockFlg();
  a->mActionMode1 = dkn::native::kActGuardL;
  a->mActionMode2 = 1;
}
// 19 (FIX03): the phase-1 guard entry = executeGuardH mode 0 without its joint snap: weapon / shield AT off, not moving, guard-ready, the native shield
// guard clip (TNA_GUARD, once, morph 3), the facing chased toward Link (0x2000 at most, as the native), Shield-Attack mark off, then ACT_GUARDH 1.
DKN_NOINLINE void dkn_native_guard_enter_h(daB_TN_c* a, std::int16_t facing) noexcept {
  a->setSwordAtBit(0);
  a->mSphC.OffAtSetBit();
  a->speedF = 0.0f;
  a->field_0xa91 = true;
  a->field_0xaa8 = false;
  a->setBck(dkn::native::kBckTnaGuard, 0, 3.0f, 1.0f);
  a->shape_angle.y = facing;
  a->offHeadLockFlg();
  a->mActionMode1 = dkn::native::kActGuardH;
  a->mActionMode2 = 1;
}
// 17 (FIX02): the native forward leap (ChaseL 8/17) becomes the native walk toward Link (initChaseL(3): TNB_WALK loop, 4.7).
DKN_NOINLINE void dkn_native_walk(daB_TN_c* a) noexcept {
  a->setBck(dkn::native::kBckTnbWalk, 2, 10.0f, 1.0f);
  a->speedF = 4.7f;
  a->mActionMode2 = 3;
}
// 18 (FIX02, Link): a guard slip THIS Darknut's impact caused keeps at most the capped speed (procGuardSlip decelerates it natively from there).
DKN_NOINLINE void dkn_link_slip_speed(daAlink_c* k, float v) noexcept {
  if (k->mNormalSpeed > v) k->mNormalSpeed = v;
}
// 20 (FIX06, FIX07 + mode 17): the native jump of ChaseL exactly as initChaseL(6) / initChaseL(8) / initChaseL(17) writes it (the mode, speedF 0, the
// take-off clip once, morph 3, rate 1): executeChaseL 6 / 8 / 17 does everything else natively (take-off speeds at frame 4: 50 / 35 / 45, the voice, the turn
// toward Link in the air, the landing). Head lock off as setActionMode does. Only the three native jump modes are ever written.
// FIX08: + the native side jumps of initChaseL(0xb) / (0xe) (TNB_JUMP_R_1 0x24 / TNB_JUMP_L_1 0x22, once, morph 3), the defensive evasion.
DKN_NOINLINE void dkn_native_jump(daB_TN_c* a, int mode) noexcept {
  const bool back = mode == dkn::native::kChaseJumpBack, right = mode == 11, left = mode == 14;
  if (!back && !right && !left && mode != dkn::native::kChaseJumpFwd && mode != dkn::native::kChaseJumpFwdLong) return;
  a->offHeadLockFlg();
  a->mActionMode1 = dkn::native::kActChaseL;
  a->mActionMode2 = mode;
  a->speedF = 0.0f;
  a->setBck(back ? dkn::native::kBckTnbJumpB1 : right ? 0x24 : left ? 0x22 : dkn::native::kBckTnbJumpF1, 0, 3.0f, 1.0f);
}
// 21 (FIX06): the forward jump's native landing exactly as initChaseL(9) writes it (mode 9, speedF 0, TNB_JUMP_F_2 once, morph 5), in place of the landing
// attack entry the native AI wrote at that very moment of executeChaseL 8 (whose branch already played the FOOT_M1 step of the landing).
DKN_NOINLINE void dkn_native_jump_land(daB_TN_c* a) noexcept {
  a->mActionMode1 = dkn::native::kActChaseL;
  a->mActionMode2 = dkn::native::kChaseJumpFwdLand;
  a->speedF = 0.0f;
  a->setBck(dkn::native::kBckTnbJumpF2, 0, 5.0f, 1.0f);
}
// 22 (FIX06): TEV color 0 (r, g, b) of one material of the sword's model data: draw()'s own body write, mirrored, and its exact restore.
DKN_NOINLINE void dkn_native_sword_color(J3DMaterial* m, short r, short g, short b) noexcept {
  J3DGXColorS10* c = m->getTevColor(0);
  if (!c) return;
  c->r = r, c->g = g, c->b = b;
}

// 23 (FIX08-01): the native neck tracking of THIS Darknut (calcNeckAngle follows Link while it is set; ctrlJoint applies the smoothed mNeckAngle to the head). mType 1
// never sets it natively (only the boss opening / demo skip): the module writes it ON for the Helm recovery look and back OFF (the native value) when it ends.
DKN_NOINLINE void dkn_native_neck(daB_TN_c* a, bool on) noexcept { a->mUpdateNeckAngle = on; }
// 24 (FIX08-09): the native eased return to the combat stance after B1 (setBck loop, NEGATIVE morph = mDoExt_morf_c's eased step chase, rate 1).
DKN_NOINLINE void dkn_native_recover_clip(daB_TN_c* a, int bck, float morph) noexcept { a->setBck(bck, 2, morph, 1.0f); }

// FIX07 (read only): the trajectory of one native leap from the Darknut along `yaw`, read with the game's own static queries: fopAcM_gc_c::gndCheck
// (dBgS_ObjGndChk down from kLeapProbeUp over three points of the path and the landing + the body radius), fopAcM_lc_c::lineCheck (dBgS_ObjLinChk at
// waist height from the Darknut to the landing, the actor itself excluded), fopAcM_wt_c::waterCheck at the landing. Called once per decision, never per tick.
struct DknGeo { dkn::Geo g = dkn::Geo::NotChecked; float ground[4] = {}; float land_x = 0, land_z = 0; };
// The direction is the unit XZ vector toward Link (forward) or away from him (back), taken from the two positions: no angle table is read.
struct DknDir { float x = 0, z = 1; };
DknDir dkn_dir_to(const daB_TN_c* a, const daAlink_c* k, bool away) noexcept {
  DknDir d;
  if (!k) return d;
  const float dx = k->current.pos.x - a->current.pos.x, dz = k->current.pos.z - a->current.pos.z, l = kk::fsqrt(dx * dx + dz * dz);
  if (l > 1.0f) d.x = (away ? -dx : dx) / l, d.z = (away ? -dz : dz) / l;
  return d;
}
DknGeo dkn_geo(const daB_TN_c* a, DknDir dir, float dist) noexcept {
  using namespace dkn;
  DknGeo r;
  const float sx = dir.x, cz = dir.z, feet = a->current.pos.y;
  const float frac[4] = {1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 1.0f};
  Geo smp[4];
  for (unsigned i = 0; i < 4; ++i) {
    const float d = dist * frac[i] + (i == 3 ? policy::kLeapBodyR : 0.0f);
    cXyz p(a->current.pos.x + sx * d, feet + policy::kLeapProbeUp, a->current.pos.z + cz * d);
    const bool hit = fopAcM_gc_c::gndCheck(&p);
    const float gy = fopAcM_gc_c::mGroundY;
    r.ground[i] = hit ? gy - feet : -9999.0f;
    smp[i] = geo_sample(!hit, gy, feet);
  }
  r.land_x = a->current.pos.x + sx * dist, r.land_z = a->current.pos.z + cz * dist;
  cXyz s0(a->current.pos.x, feet + policy::kLeapLineY, a->current.pos.z), s1(r.land_x + sx * policy::kLeapBodyR, feet + policy::kLeapLineY, r.land_z + cz * policy::kLeapBodyR);
  const bool wall = fopAcM_lc_c::lineCheck(&s0, &s1, a);
  cXyz lw(r.land_x, feet + policy::kLeapProbeUp, r.land_z);
  const bool water = fopAcM_wt_c::waterCheck(&lw) && fopAcM_wt_c::mWaterY > feet + r.ground[2] + 20.0f;
  r.g = geo_path(smp, 4, wall, water);
  return r;
}

// ---------------------------------------------------------------- moves and windows
bool dkn_owned_move(const DknSlot& s) noexcept { return s.lease == DknLease::Combo || s.lease == DknLease::Single || s.lease == DknLease::Counter; }
bool dkn_move_now(const DknSlot& s, const daB_TN_c* a, dkn::Move& m) noexcept {
  using namespace dkn::native;
  if (dkn_owned_move(s) && !s.tail) return m = s.plan.seg[s.seg].m, true;
  const int act = a->mActionMode1, mode = a->mActionMode2;
  if (s.lease == DknLease::Special && (act == kActAttackShieldH || act == kActAttackShieldL)) return m = s.special_move, true;
  if (act == kActAttackH && (mode == 1 || mode == 2)) return m = mode == 1 ? dkn::Move::F1A : dkn::Move::F1B, true;
  if (act == kActAttackShieldH && mode == 1) return m = dkn::Move::F1Shield, true;
  if (act == kActAttackL && (mode == 1 || mode == 101)) return m = dkn::Move::F2A, true;
  if (act == kActAttackL && (mode == 2 || mode == 102)) return m = dkn::Move::F2B, true;
  if (act == kActAttackShieldL && mode == 1) return m = dkn::Move::F2C, true;
  return false;
}
bool dkn_sword_move(dkn::Move m) noexcept { return m != dkn::Move::F1Shield && m != dkn::Move::F2C; }
// The AT of the open window: on while the window is open and its impact not yet resolved; off after the one contact; off outside the windows.
void dkn_at_authority(DknSlot& s, daB_TN_c* a) noexcept {
  dkn::Move m;
  if (!dkn_move_now(s, a, m)) {
    s.led.close();
    // FIX05: F2/F3 combat outside every move: a weapon / kick AT still on (the transformation's draw AT left on by initChaseL(3) after F2_START) is off
    if (dkn::phase2(s.ph) && s.brk.st == dkn::Bk::Combat && (s.lease == DknLease::None || s.lease == DknLease::Reposition || s.lease == DknLease::Leap || s.lease == DknLease::Evade) &&
        (a->mSwordSphs[0].ChkAtSet() || a->mSphC.ChkAtSet())) {  // FIX06: a reposition jump never carries a weapon AT either
      dkn_native_at(a, true, 0);
      dkn_native_at(a, false, 0);
      if (++s.st_at_orphan_off <= 20 || s.st_at_orphan_off % 50 == 0) {
        char ex[96];
        std::snprintf(ex, sizeof(ex), "reason=AT_OUTSIDE_EVERY_MOVE total=%llu", static_cast<unsigned long long>(s.st_at_orphan_off));
        dkn_log("AT_ORPHAN_OFF", s, a, ex);
      }
    }
    return;
  }
  const int w = dkn::window_of(m, dkn_frame(a));
  bool want = false;
  if (w < 0) {
    s.led.close();
  } else {
    if (!s.led.open || s.led.move != m || s.led.k != static_cast<std::uint8_t>(w)) {
      s.led.open_window(m, static_cast<std::uint8_t>(w));
      if (w == 1) dknb_burst_gap(s.id);  // the second cut of a B follows the first within ~0.21-0.31 s: the parry pair re-arm
    }
    want = s.led.fresh();
  }
  const bool sword = dkn_sword_move(m);
  const bool now = sword ? a->mSwordSphs[0].ChkAtSet() != 0 : a->mSphC.ChkAtSet() != 0;
  if (now != want) dkn_native_at(a, sword, want ? 1 : 0);
}

// ---------------------------------------------------------------- the Darknut's impacts on Link: parry / block / direct, ONE per impact window
void dkn_posture_gain(DknSlot& s, daB_TN_c* a, int pts, const char* src) noexcept {
  if (pts <= 0 || !dkn_combat(s)) return;
  const int before = s.post.points();
  const dkn::Gain g = s.post.gain(pts);
  if (g == dkn::Gain::None || g == dkn::Gain::Ignored) return;
  ++s.st_gain;
  char ex[160];
  std::snprintf(ex, sizeof(ex), "source=%s gain=+%d before=%d after=%d", src, pts, before, s.post.points());
  dkn_log("POSTURE_GAIN", s, a, ex);
  if (g == dkn::Gain::Reached) {
    s.brk.latch();
    dkn_log("POSTURE_BREAK", s, a, "event=LATCH_MAX next=BREAK_PENDING");
  }
}
void dkn_link_gain(DknSlot& s, daB_TN_c* a, dkn::Move m, dkn::Outcome o) noexcept {
  const int pts = dkn::link_gain(m, o);
  if (pts <= 0) return;
  const DknGain r = dknb_link_gain(pts, o == dkn::Outcome::Block, s.id, s.generation, s.led.serial);
  char ex[220];
  std::snprintf(ex, sizeof(ex), "move=%s outcome=%s impact_serial=%llu link_points=%d confirmed=%d link_before=%d.%03d link_after=%d.%03d saturated=%d", dkn::name(m), dkn::name(o),
                static_cast<unsigned long long>(s.led.serial), pts, r.confirmed ? 1 : 0, r.before / 1000, r.before % 1000, r.after / 1000, r.after % 1000, r.saturated ? 1 : 0);
  dkn_log("IMPACT", s, a, ex);
}
// FIX08-06: ONE line and one uncapped counter per authentic parry, by route, with the identity the FX transaction carries (actor, generation, impact serial, tick):
// the reconciliation of every parry route with its FX_TXN (pass tick = this tick - 1) never depends on a sampled log.
inline constexpr const char* kDknFxRoute[10] = {"F1_LATERAL", "F1_VERTICAL", "F1_SHIELD_BASH", "THROWN_MACE", "F2_THRUST", "F2_DOUBLE_CUT1", "F2_DOUBLE_CUT2", "F2_B1", "F2_KICK", "DARKNUT_PARRY"};
unsigned dkn_fx_route_of(dkn::Move m, unsigned k) noexcept {
  switch (m) {
    case dkn::Move::F1A: return 0;
    case dkn::Move::F1B: return 1;
    case dkn::Move::F1Shield: return 2;
    case dkn::Move::F2A: return 4;
    case dkn::Move::F2B: return k ? 6u : 5u;
    case dkn::Move::F2B1: return 7;
    case dkn::Move::F2C: return 8;
    default: return 9;
  }
}
void dkn_fx_route(DknSlot& s, const daB_TN_c* a, unsigned route, std::uint64_t token, const char* dir) noexcept {
  ++s.fx_route[route < 10 ? route : 9];
  if (s.st_route_lines >= 120) return;  // sampled; the per-route counters (summary) are complete
  ++s.st_route_lines;
  char ex[200];
  std::snprintf(ex, sizeof(ex), "route=%s direction=%s token=%llu fx_pass_tick=%llu expected=ORANGE_ALL_COMPONENTS+NATIVE_CLASH_x%.2f", kDknFxRoute[route < 10 ? route : 9], dir,
                static_cast<unsigned long long>(token), static_cast<unsigned long long>(dknb_tick() ? dknb_tick() - 1 : 0), static_cast<double>(pfx::kPitch));
  dkn_log("PARRY_ROUTE", s, a, ex);
}
void dkn_link_contact(DknSlot& s, daB_TN_c* a, daAlink_c* k) noexcept {
  if (!k || !s.led.fresh()) return;
  bool hit = false, shield = false;
  const void* sph = nullptr;
  auto look = [&](dCcD_GObjInf& x) noexcept {
    if (!x.cCcD_ObjHitInf::ChkAtHit()) return;
    const cCcD_Obj* o = x.cCcD_ObjHitInf::GetAtHitObj();
    for (int i = 0; i < 3; ++i)
      if (o == static_cast<const cCcD_Obj*>(&k->mTgCyls[i])) {
        hit = true, shield = shield || x.ChkAtShieldHit();
        if (!sph) sph = static_cast<const void*>(&x);
      }
  };
  if (dkn_sword_move(s.led.move)) {
    for (int i = 0; i < 4; ++i) look(a->mSwordSphs[i]);
    look(a->mCps);
  } else {
    look(a->mSphC);
  }
  if (!hit || !s.led.resolve()) return;
  ++s.st_impacts;
  const dkn::Move m = s.led.move;
  const unsigned kidx = s.led.k;
  const bool burst_follows = (m == dkn::Move::F2B && kidx == 0) ||
                             (dkn_owned_move(s) && s.seg + 1 < s.plan.n && s.plan.seg[s.seg + 1].m == dkn::Move::F2A && m == dkn::Move::F2A);
  if (shield) {
    // FIX02: the slip this contact causes is capped (both actor orders). FIX04: the claim names this impact; it stays valid two ticks (Link may enter the
    // slip later this frame, or the engine may rewrite the speed) but the chain advances ONCE per impact and the same cap is re-applied idempotently.
    s.slip_pending = true, s.slip_tick = dknb_tick(), s.slip_move = m, s.slip_serial = s.led.serial;
    dkn_slip_cap(s, k, a, "DARKNUT_DAMAGE_PRE");
    const DknPass pass = dknb_link_pass(k);
    if (pass.in_window) {
      ++s.st_parry;
      dknb_link_parry_confirmed(s.id, pass.serial);
      pfx_settle(pfx::Kind::LinkOnDarknut, s.id, true);  // FIX04: the orange spark / high clash the pass previewed is this parry
      dkn_fx_route(s, a, dkn_fx_route_of(m, kidx), s.led.serial, "LINK_PARRIES_DARKNUT");  // FIX08-06
      char ex[200];
      std::snprintf(ex, sizeof(ex), "move=%s impact=%u impact_serial=%llu link_serial=%llu delta=%.2f link_gain=+0 combo=%s", dkn::name(m), kidx + 1,
                    static_cast<unsigned long long>(s.led.serial), static_cast<unsigned long long>(pass.serial), pass.delta,
                    dkn_owned_move(s) ? dkn::combo_of(s.combo_idx).name : "NONE");
      dkn_log("PARRY", s, a, ex);
      dkn_posture_gain(s, a, dkn::dk_parry_gain(m, kidx), "LINK_PARRY");  // a parry never ends the combo
      return;
    }
    ++s.st_block_link;
    pfx_settle(pfx::Kind::LinkOnDarknut, s.id, false);  // FIX04: an ordinary block: a previewed spark goes back to its own colour
    dknb_link_blocked(s.id, burst_follows);
    dkn_link_gain(s, a, m, dkn::Outcome::Block);
    return;
  }
  ++s.st_direct_link;
  pfx_settle(pfx::Kind::LinkOnDarknut, s.id, false);
  dknb_direct_offer(dkn_index(&s), s.id, s.generation, s.led.serial, sph);
  char ex[120];
  std::snprintf(ex, sizeof(ex), "move=%s impact=%u impact_serial=%llu outcome=DIRECT_OFFERED (credited on Link's native damage receipt)", dkn::name(m), kidx + 1,
                static_cast<unsigned long long>(s.led.serial));
  dkn_log("IMPACT", s, a, ex);
}
// Direct offers resolved by Link's native damage receipt (checkDamageAction POST), once per simulation tick.
void dkn_direct_maintenance() noexcept {
  const std::uint64_t now = dknb_tick();
  if (dn.direct_tick == now) return;
  dn.direct_tick = now;
  for (unsigned i = 0; i < dkn::kSlots; ++i) {
    DknSlot& s = dn.s[i];
    if (!s.on) continue;
    std::uint32_t id = 0;
    std::uint64_t token = 0;
    if (!dknb_direct_take(i, id, token) || id != s.id) continue;
    const dkn::Move m = s.led.move;
    const bool mace = s.mace_direct_token && token == s.mace_direct_token;  // FIX07: the thrown mace's direct contact: +60, never the move table
    const int pts = mace ? dkn::mace_link_points(dkn::Mace::Direct) : dkn::link_gain(m, dkn::Outcome::Direct);
    const DknGain r = dknb_link_gain(pts, false, s.id, s.generation, token);
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "DARKNUT_IMPACT actor=%u generation=%llu move=%s outcome=DIRECT impact_serial=%llu link_points=%d confirmed=%d link_after=%d.%03d saturated=%d tick=%llu", s.id,
                  static_cast<unsigned long long>(s.generation), mace ? "THROWN_MACE_FIX07" : dkn::name(m), static_cast<unsigned long long>(token), pts, r.confirmed ? 1 : 0, r.after / 1000, r.after % 1000,
                  r.saturated ? 1 : 0, static_cast<unsigned long long>(now));
    dkn_line(false, line);
  }
}

// ---------------------------------------------------------------- the owned moves: combos, singles, counters
void dkn_start_move(DknSlot& s, daB_TN_c* a, unsigned seg, float frame, bool first) noexcept {
  const dkn::Seg& sg = s.plan.seg[seg];
  const dkn::MoveInfo& mi = dkn::info(sg.m);
  s.seg = seg;
  dkn_native_action(a, mi.act, mi.mode);
  dkn_native_clip(a, mi.bck, first ? (dkn::phase2(s.ph) ? 3.0f : 6.0f) : sg.morph, frame, sg.rate_in);
  dkn_native_attack_flags(a);
  s.led.close();
}
// FIX06: the sequence ends ONCE (a legitimate exception or its natural end); what it still had scheduled goes with it (never a late move afterwards).
void dkn_seq_end(DknSlot& s, daB_TN_c* a, dkn::SqEnd e, const char* detail) noexcept {
  if (!s.seq.owns()) return;
  const dkn::Sq was = s.seq.st;
  const bool direct = s.seq.direct;
  const int idx = s.seq.idx;
  if (s.def.cp == dkn::Cp::Scheduled) s.def.cp = dkn::Cp::None, ++s.def.counter_dropped;  // the scheduled move of the sequence: dropped with it
  s.seq.finish(e);
  const bool natural = e == dkn::SqEnd::Done || e == dkn::SqEnd::CounterOnly;
  char ex[240];
  std::snprintf(ex, sizeof(ex), "state=%s reason=%s detail=%s route=%s plan=%s owned_ticks=%llu initiative=%s", dkn::name(was), dkn::name(e), detail ? detail : "-",
                direct ? "DIRECT_COMBO_60" : "COUNTER_40", idx >= 0 ? dkn::combo_of(static_cast<unsigned>(idx)).name : "NONE",
                static_cast<unsigned long long>(dknb_tick() - s.seq.since), natural ? "BACK_TO_COMMON_COMBAT" : "EXCEPTION_PRIORITY");
  dkn_log("SEQ_END", s, a, ex, !natural && e != dkn::SqEnd::Break && e != dkn::SqEnd::Phase && e != dkn::SqEnd::Defeat);
}
void dkn_move_end(DknSlot& s, daB_TN_c* a, const char* why, bool cancel) noexcept {
  if (!dkn_owned_move(s)) return;
  const DknLease ended_kind = s.lease;
  const dkn::Combo& c = dkn::combo_of(s.combo_idx);
  char ex[320];
  std::snprintf(ex, sizeof(ex), "combo=%s kind=%s reason=%s impacts=%u/%u moves=%u/%u ticks=%llu native_pause_ticks=%llu rate_min=%.2f rate_max=%.2f", c.name, dkn_lease_name(s.lease),
                why, s.impacts, c.n_impacts, s.seg + 1, s.plan.n, static_cast<unsigned long long>(dknb_tick() - s.combo_tick), static_cast<unsigned long long>(s.mv_paused),
                static_cast<double>(s.mv_rate_min < 50.0f ? s.mv_rate_min : 0.0f), static_cast<double>(s.mv_rate_max));
  if (s.lease == DknLease::Combo && s.combo_idx < 8) {
    DknStage& g = dkn_stage(s);
    ++(cancel ? g.combo_cancel : g.combo_end)[s.combo_idx & 3];
  }
  s.lease = DknLease::None, s.tail = false;
  s.ini.ended(dknb_tick(), dkn::phase2(s.ph));
  if (cancel) {
    ++s.st_combo_cancel;
    dkn_native_at(a, true, 0);
    dkn_native_at(a, false, 0);
    dkn_native_rate(a, 1.0f);
  }
  s.led.close();
  dkn_log(cancel ? "COMBO_CANCEL" : "COMBO_END", s, a, ex);
  // FIX06: the owned move of a sequence ended. An exception ends the sequence; the counter's natural end rolls its ONE continuation (80 / 20); the combo's
  // natural end closes it (back to the common combat after its natural recovery).
  if (!s.seq.owns()) return;
  const dkn::SqEnd e = dkn::seq_end_of(why);
  if (e != dkn::SqEnd::None) {
    dkn_seq_end(s, a, e, why);
  } else if (s.seq.st == dkn::Sq::Counter && ended_kind == DknLease::Counter) {
    const std::uint64_t now = dknb_tick(), last = s.impacts ? s.impact_tick[s.impacts - 1 < 9 ? s.impacts - 1 : 8] : 0;
    if (s.seq.counter_done(now, last, dkn::phase2(s.ph))) {
      char cx[240];
      std::snprintf(cx, sizeof(cx), "roll=CONTINUE_80 combo=%s pick=25/25/25/25 counter_end=%llu last_impact=%llu due=%llu first_impact_after_last>=%u settle=%u(PROVISIONAL)",
                    dkn::combo_of(static_cast<unsigned>(s.seq.idx)).name, static_cast<unsigned long long>(now), static_cast<unsigned long long>(s.seq.last_impact),
                    static_cast<unsigned long long>(s.seq.due_at), dkn::policy::kBridgeLandTicks, dkn::policy::kBridgeSettleTicks);
      dkn_log("SEQ_CONTINUE", s, a, cx);
    } else {
      dkn_log("SEQ_END", s, a, "state=COUNTER reason=COUNTER_ONLY_20 roll=EXIT_20 initiative=BACK_TO_COMMON_COMBAT");
    }
  } else if (s.seq.st == dkn::Sq::Combo && ended_kind == DknLease::Combo) {
    s.seq.combo_done();
    dkn_log("SEQ_END", s, a, "state=COMBO reason=DONE_NATURAL_RECOVERY initiative=BACK_TO_COMMON_COMBAT");
  }
}
bool dkn_move_begin(DknSlot& s, daB_TN_c* a, daAlink_c* k, unsigned idx, DknLease kind, const char* why) noexcept {
  const dkn::Combo& c = dkn::combo_of(idx);
  if (!k || dkn_dist(a, k) > dkn::policy::kComboLeaveDist || !dknb_world_safe() || c.phase2 != dkn::phase2(s.ph)) return false;
  s.lease = kind, s.tail = false, s.combo_idx = idx, s.seg = 0, s.ct = 0, s.impacts = 0, s.combo_tick = dknb_tick();
  s.mv_last_tick = dknb_tick(), s.mv_paused = 0, s.mv_rate_min = 99.0f, s.mv_rate_max = 0.0f;  // FIX04 tempo telemetry of this move
  s.def_holding = false;
  s.plan = dn.plans[idx], s.tl = dn.tls[idx];
  s.ini.started(dknb_tick());
  DknStage& g = dkn_stage(s);
  if (kind == DknLease::Combo) ++s.st_combos, ++g.combo_start[idx & 3];
  else if (kind == DknLease::Single) ++g.singles;
  else ++g.counters, ++s.st_counters;
  dkn_start_move(s, a, 0, s.plan.seg[0].start, true);
  char ex[240];
  std::snprintf(ex, sizeof(ex), "combo=%s kind=%s owner=OWN_CONTROLLER cause=%s impacts=%u moves=%u distance=%.0f scale=%.2f pick=25/25/25/25 single_pct=%d(PROVISIONAL)", c.name,
                dkn_lease_name(kind), why, c.n_impacts, c.n_moves, dkn_dist(a, k), static_cast<double>(s.plan.scale), dkn::policy::kSinglePct);
  dkn_log(kind == DknLease::Counter ? "COUNTER_START" : "COMBO_START", s, a, ex);
  return true;
}
// executeAttack* POST of an owned move: impacts, the switch to the next move, the rate that keeps the timeline, distance correction.
void dkn_combo_post(DknSlot& s, daB_TN_c* a, daAlink_c* k) noexcept {
  using namespace dkn;
  if (s.tail) {  // the last move plays to its native end at its own rate
    const MoveInfo& mi = info(s.plan.seg[s.seg].m);
    if (a->mActionMode1 != mi.act) dkn_move_end(s, a, "NATIVE_END", false);
    return;
  }
  const Seg& cur = s.plan.seg[s.seg];
  const MoveInfo& mi = info(cur.m);
  if (a->mActionMode1 != mi.act) {  // something native took the actor (should not happen inside an owned move): end at once, never fight it
    dkn_move_end(s, a, "ACTION_LOST", true);
    return;
  }
  const Combo& c = combo_of(s.combo_idx);
  // FIX04: ticks this POST did not run (the native hitstop dScnPly_c::setPauseTimer freezes every actor 2 ticks per damaging Link hit; an event / menu)
  {
    const std::uint64_t nt = dknb_tick();
    if (nt > s.mv_last_tick + 1) s.mv_paused += nt - s.mv_last_tick - 1;
    s.mv_last_tick = nt;
  }
  // impacts of the previous play (bookkeeping against the recording)
  const float f = dkn_frame(a);
  unsigned seen = 0;
  for (unsigned i = 0; i < s.seg; ++i) seen += info(s.plan.seg[i].m).impacts;
  for (unsigned kx = 0; kx < mi.impacts; ++kx)
    if (f >= mi.impact[kx] && s.impacts < seen + kx + 1 && s.impacts < 9) {
      s.impact_tick[s.impacts] = dknb_tick();
      s.mv_paused_at[s.impacts] = s.mv_paused;
      double err = 0, want = 0, paused = 0;
      if (s.impacts > 0 && !is_single(s.combo_idx)) {
        want = s.plan.target_ticks[s.impacts - 1];  // FIX06: the planned interval (audio x S, or the approved final target of kIvTargets)
        err = static_cast<double>(s.impact_tick[s.impacts] - s.impact_tick[s.impacts - 1]) - want;
        paused = static_cast<double>(s.mv_paused_at[s.impacts] - s.mv_paused_at[s.impacts - 1]);
      }
      char ex[320];
      std::snprintf(ex, sizeof(ex), "combo=%s impact=%u/%u move=%s sub=%u audio_interval_ms=%d scale=%.2f planned_ticks=%.2f retimed_fix06=%d interval_error_ticks=%+.2f native_pause_ticks=%.0f "
                    "error_ex_pause=%+.2f first_contact_ticks=%llu", c.name, s.impacts + 1, c.n_impacts, name(cur.m), kx + 1,
                    s.impacts && !is_single(s.combo_idx) ? c.peak_ms[s.impacts] - c.peak_ms[s.impacts - 1] : 0, static_cast<double>(s.plan.scale), want,
                    s.impacts && !is_single(s.combo_idx) && iv_target(s.combo_idx, s.impacts - 1) > 0 ? 1 : 0, err, paused, err - paused,
                    static_cast<unsigned long long>(s.impacts ? 0 : dknb_tick() - s.combo_tick));
      dkn_log("COMBO_SWING", s, a, ex);
      if (s.impacts == 0 && s.leap_combo_tick && s.leap_combo_tick == s.combo_tick && !s.leap_first_impact_logged) {  // FIX07: the leap's fairness, measured
        s.leap_first_impact_logged = true;
        char fx[220];
        std::snprintf(fx, sizeof(fx), "combo=%s since_leap_start=%llu since_landing=%llu since_combo_start=%llu distance=%.0f clip_frame=%.1f",
                      c.name, static_cast<unsigned long long>(dknb_tick() - s.leap_tick), static_cast<unsigned long long>(dknb_tick() - s.leap_land_tick),
                      static_cast<unsigned long long>(dknb_tick() - s.combo_tick), k ? static_cast<double>(dkn_dist(a, k)) : -1.0, static_cast<double>(f));
        dkn_log("LEAP_FIRST_IMPACT", s, a, fx);
      }
      ++s.impacts;
    }
  unsigned ts = 0;
  float want = 0;
  const bool on = locate(s.plan, s.tl, static_cast<float>(s.ct + 1), ts, want);
  float now_frame = f;
  if (on && ts != s.seg) {
    if (!k || dkn_dist(a, k) > policy::kComboLeaveDist || !dknb_world_safe() || dknb_link_broken()) {
      dkn_move_end(s, a, !k ? "NO_TARGET" : dknb_link_broken() ? "LINK_GUARD_BROKEN" : !dknb_world_safe() ? "EVENT" : "LINK_FAR", true);
      dkn_native_neutral(a, phase2(s.ph));
      return;
    }
    now_frame = switch_frame(s.plan.seg[ts], static_cast<float>(s.ct) - s.tl.T[ts]);
    dkn_start_move(s, a, ts, now_frame, false);
  }
  if (!on) {
    const Seg& last = s.plan.seg[s.plan.n - 1];
    if (last.m == Move::F2B1) {  // B1: after the first cut only, a natural recovery: no second cut, no AT
      // FIX08-09: FIX07 morphed 10 frames into TNB_WAIT (the sword tip swept 332 units at 120..156 / tick across the front, a second cut's speed, then a 30-tick still
      // idle). Now the native eased morph (negative) into the combat stance TNB_WAIT_B_2 (tip speed bell, peak 52, done in 16 ticks), the native wait 18 ticks, then
      // the native strafe (initChaseL(1) keeps / blends the same stance). The cut, its AT window and the impact timings are untouched.
      const float f20 = dkn_frame(a);
      dkn_native_at(a, true, 0);
      dkn_native_action(a, native::kActChaseL, 2);
      dkn_native_recover_clip(a, policy::kB1RecoverBck, policy::kB1RecoverMorph);
      dkn_native_timer1(a, policy::kB1RecoverTicks);
      ++s.st_b1_recover;
      if (s.st_b1_recover <= 60) {
        char bx[200];
        std::snprintf(bx, sizeof(bx), "combo=%s b1_end_frame=%.1f recovery=TNB_WAIT_B_2_0x2c morph=%.0f(eased) native_wait=%d second_cut=NONE at=OFF", c.name, static_cast<double>(f20),
                      static_cast<double>(policy::kB1RecoverMorph), policy::kB1RecoverTicks);
        dkn_log("B1_RECOVERY_START", s, a, bx);
      }
      dkn_move_end(s, a, "B1_RECOVERY", false);
      return;
    }
    s.tail = true;
    dkn_native_rate(a, 1.0f);
    dkn_native_attack_flags(a);  // the tail keeps mActionMode2Copy: the native B->A chain stays impossible after an owned B
    return;
  }
  float rate = want - now_frame;
  rate = rate < 0 ? 0 : rate > kTickRateMax ? kTickRateMax : rate;
  if (rate < s.mv_rate_min) s.mv_rate_min = rate;
  if (rate > s.mv_rate_max) s.mv_rate_max = rate;
  dkn_native_rate(a, rate);
  dkn_native_attack_flags(a);
  // wind-up only: face Link (bounded) and close the distance the impact needs (phase 1 heavier: slower step, slower turn)
  const Seg& sg = s.plan.seg[s.seg];
  const MoveInfo& m2 = info(sg.m);
  float sp = 0.0f;
  std::int16_t facing = a->shape_angle.y;
  if (k && now_frame < m2.impact[0]) {
    std::int16_t diff = static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y);
    const std::int16_t step = phase2(s.ph) ? 0x500 : 0x300;
    if (diff > step) diff = step;
    if (diff < -step) diff = -step;
    facing = static_cast<std::int16_t>(a->shape_angle.y + diff);
    const float d = dkn_dist(a, k), left = (m2.impact[0] - now_frame) / (rate > 0.2f ? rate : 0.2f);
    sp = d > m2.reach ? (d - m2.reach) / (left > 1 ? left : 1) : 0.0f;
    const float cap = phase2(s.ph) ? 18.0f : 8.0f;
    if (sp > cap) sp = cap;
  }
  dkn_native_motion(a, sp, facing);
  ++s.ct;
}
// A passivity special (shield bash / kick): ONE native move, never a combo, never the native shield->mace / kick->attack chain afterwards.
void dkn_special_post(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn::native;
  const int act = a->mActionMode1;
  if (act == kActAttackShieldL && s.special_move == dkn::Move::F2C) {  // FIX03: the passivity kick x1.12 (the phase-1 shield bash keeps its native rate)
    dkn_native_rate(a, 1.0f / dkn::policy::kKickScale);
    ++s.st_kick_slowed;
    return;
  }
  if (act == kActAttackShieldH || act == kActAttackShieldL) return;
  s.lease = DknLease::None;
  s.ini.ended(dknb_tick(), dkn::phase2(s.ph));
  if (attack_action(act)) {  // the native chain just started at the clip end: back to the neutral chase instead
    ++s.st_chain_cut;
    dkn_native_neutral(a, dkn::phase2(s.ph));
  }
  dkn_log("SPECIAL_END", s, a, attack_action(act) ? "native_chain=CUT NATIVE_CHAIN_SUPPRESSED back=OWN_CONTROLLER" : "back=OWN_CONTROLLER");
}

// ---------------------------------------------------------------- admission and stages
DknSlot* dkn_admit(daB_TN_c* a) noexcept {
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  if (DknSlot* s = dkn_find(id)) return s;
  for (std::uint32_t r : dn.rejected_ids)
    if (r == id) return nullptr;
  if (!dkr::eligible(dkr_identity(a))) {  // the DKR01 filter, reused: only the normal Enemy Spawner Darknut
    ++dn.ineligible;
    dn.rejected_ids[dn.rejected_next++ % 16] = id;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "DARKNUT_ELIGIBILITY actor=%u result=NATIVE param=%08x type=%d type2=%d switch=%d stay_no2=%d tick=%llu", id, static_cast<unsigned>(fopAcM_GetParam(a)),
                  a->mType, a->mType2, a->mSwitchNo, a->mStayNo2Flag ? 1 : 0, static_cast<unsigned long long>(dknb_tick()));
    dkn_line(false, line);
    return nullptr;
  }
  for (DknSlot& s : dn.s)
    if (!s.on) {
      s = DknSlot{};
      s.on = true, s.id = id, s.generation = dn.next_generation++;
      const DkrView v = dkr_view(id);
      s.ph = a->mActionMode1 >= dkn::native::kActChaseL ? (v.revivals ? dkn::Ph::Revived : dkn::Ph::Light) : dkn::Ph::Armor;  // a Darknut admitted already in its sword phase
      s.post.reset(dkn::posture_max(s.ph));
      s.ini.rng.seed((static_cast<std::uint64_t>(id) << 20) ^ s.generation ^ dknb_tick());
      s.def.rng.seed((static_cast<std::uint64_t>(id) << 24) ^ (s.generation * 0x9E37u) ^ (dknb_tick() + 17));
      // FIX06: the parry route (60 / 40) and the continuation (80 / 20) on streams of their own, per actor and generation
      s.def.routes = true;
      s.def.route.seed((static_cast<std::uint64_t>(id) << 28) ^ (s.generation * 0x5851F42Du) ^ (dknb_tick() + 29));
      s.seq.rng.seed((static_cast<std::uint64_t>(id) << 32) ^ (s.generation * 0x2545F491u) ^ (dknb_tick() + 41));
      s.gap.rng.seed((static_cast<std::uint64_t>(id) << 36) ^ (s.generation * 0x1B873593u) ^ (dknb_tick() + 53));  // FIX07: the offensive-leap roll, its own stream
      s.ini.reentry(dknb_tick());
      s.last_acc = a->field_0x6fc;
      ++dn.admitted;
      char cx[200];
      std::snprintf(cx, sizeof(cx), "eligibility=B_TN_000001FF_mType1_mType2_0_switch_ff max_hp1=%d max_res=%d posture_max=%d/%d/%d offense=OWN_CONTROLLER", dkn::policy::kHp1Max,
                    dkn::policy::kRes2Max, dkn::policy::kPosture1Max, dkn::policy::kPosture2Max, dkn::policy::kPosture3Max);
      dkn_log("CREATE", s, a, cx);
      return &s;
    }
  ++dn.full;
  return nullptr;
}
void dkn_helm_look_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why) noexcept;  // FIX08-01, defined with the action hooks
void dkn_back_to_combat(DknSlot& s, daB_TN_c* a, const char* ev, const char* extra) noexcept {
  s.post.reset(dkn_max(s));
  s.brk.abort(dkn::BkEnd::None);
  dkn_seq_end(s, a, dkn::SqEnd::Reset, ev);  // FIX06: a recovery / phase start never inherits a sequence (normally none is alive here)
  s.repo_pending = false, s.repo = dkn::Rp::None;
  s.lease = DknLease::None, s.tail = false, s.md_active = s.helm_active = false, s.md_rcpt.clear();
  s.def.reset_exchange(), s.pas.reset(), s.def_holding = false;
  s.ini.reentry(dknb_tick());
  s.last_acc = a->field_0x6fc;
  dkn_log(ev, s, a, extra);
}
// The stage of this instance, from the native action and the DKR01 table (read only). The execution fall and its outcomes are tracked here too.
void dkn_phase_track(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn;
  const int act = a->mActionMode1;
  const DkrView v = dkr_view(s.id);
  if (s.exec) {
    if (v.phase == dkr::Phase::Rising) {  // phase-2 execution Finish: the ONE resurrection (FIX02) owns the body; phase 3 follows
      s.exec = false, s.ph = Ph::Dkr;
      dkn_log("EXECUTION_FINISH_FIRST_DEFEAT", s, a, "owner=DKR01 fix02=PLAYING next=F3 posture=0/180 res=480/480");
    } else if (v.phase == dkr::Phase::Final) {
      s.exec = false, s.ph = Ph::Dead;
      dkn_log("EXECUTION_FINISH_FINAL", s, a, "owner=NATIVE_VANISH definitive=1 second_resurrection=0");
    } else if (act != native::kActEnding && v.getups > s.getups_seen) {
      s.getups_seen = v.getups, s.exec = false, ++s.st_getups;
      char ex[160];
      std::snprintf(ex, sizeof(ex), "event=GETUP_DONE resistance_kept=%d/480 posture=0/%d resurrection_consumed=%u fix02=NOT_PLAYED stage_kept=1", res2_remaining(a->field_0x6fc),
                    posture_max(s.ph), v.revivals);
      dkn_back_to_combat(s, a, "GETUP_DONE", ex);
      s.repo_pending = true, s.repo_since = dknb_tick(), s.repo_turn_since = 0;  // FIX06: ONE reposition decision for this getup (a recoverable fall without Finish)
    }
    return;
  }
  if (s.ph == Ph::Armor && act == native::kActChangeDemo) {
    dkn_helm_look_end(s, a, dknb_link(), "PHASE_CHANGE");  // FIX08: never a stale neck flag into the transformation
    s.ph = Ph::Transform, s.brk.abort(BkEnd::Phase);
    if (dkn_owned_move(s)) dkn_move_end(s, a, "PHASE_CHANGE", false);
    dkn_seq_end(s, a, SqEnd::Phase, "ACT_CHANGEDEMO");
    s.lease = DknLease::None;
  }
  if (s.ph == Ph::Transform && act >= native::kActChaseL && act != native::kActEnding) {
    s.ph = Ph::Light;
    dkn_back_to_combat(s, a, "PHASE_TRANSITION", "event=F2_START posture=0/160 res=480/480 weapon=LIGHT_SWORD armour_never_rebuilt=1 chains_windows_counters=RESET");
  }
  if (act == native::kActEnding && (s.ph == Ph::Light || s.ph == Ph::Revived)) {
    const bool second = s.ph == Ph::Revived || v.phase == dkr::Phase::Consumed;
    s.ph = second ? Ph::Dead : Ph::Dkr;
    s.brk.abort(BkEnd::Death);
    if (dkn_owned_move(s)) dkn_move_end(s, a, "DEFEAT", false);
    dkn_seq_end(s, a, SqEnd::Defeat, "ACT_ENDING");
    s.repo_pending = false;
    s.lease = DknLease::None, s.def.reset_exchange(), s.def_holding = false;
    dkn_log(second ? "FINAL_NATIVE_DEATH" : "DKR01_REAL_LETHAL", s, a, second ? "cause=HP0_F3 owner=NATIVE_executeZakoEnding definitive=1" : "cause=HP0_F2 owner=DKR01 finish_or_timer300=FIX02");
  }
  if (s.ph == Ph::Dkr && act != native::kActEnding) {  // DKR01 handed the actor back (end of the FIX02 rise): phase 3, full resistance, posture 0/180
    s.ph = Ph::Revived;
    char ex[120];
    std::snprintf(ex, sizeof(ex), "event=F3_START clip=FIX02 dkr_phase=%s res=%d/480 posture=0/180", dkr::name(v.phase), res2_remaining(a->field_0x6fc));
    dkn_back_to_combat(s, a, "FIX02_RISE_DONE", ex);
    s.repo_pending = true, s.repo_since = dknb_tick(), s.repo_turn_since = 0;  // FIX06: ONE reposition decision for this completed resurrection
  }
}
bool dkn_stagger_action(const DknSlot& s, const daB_TN_c* a) noexcept {
  return a->mActionMode1 == (dkn::phase2(s.ph) ? dkn::native::kActYoroke : dkn::native::kActDamageH);
}
void dkn_begin_break(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn;
  if (dkn_owned_move(s)) dkn_move_end(s, a, "POSTURE_BREAK", true);
  if (s.lease == DknLease::Evade) ++s.evade.cancels;  // FIX08: a break during the evasion ends it (the stagger has priority)
  if (s.lease == DknLease::Special || s.lease == DknLease::Reposition || s.lease == DknLease::Leap || s.lease == DknLease::Evade) s.lease = DknLease::None;  // FIX07: + the leap; FIX08: + the evasion
  dkn_helm_look_end(s, a, dknb_link(), "NEW_BREAK");
  s.leap_phase = dkn::Lp::None;
  s.def.cancel();
  dkn_seq_end(s, a, SqEnd::Break, "POSTURE_BREAK");  // FIX06: the break has priority over the sequence (its prep, bridge or scheduled combo)
  s.repo_pending = false;
  dkn_native_at(a, true, 0);
  dkn_native_at(a, false, 0);
  dkn_native_stagger_hold(a, true);
  // phase 1: the native first-phase damage reaction (TNA_DAMAGE_L, voice, facing); phases 2-3: the native ACT_YOROKE (TNB_DOWN_SIPPU). Never DAMAGE_LAST.
  dkn_native_action(a, phase2(s.ph) ? native::kActYoroke : native::kActDamageH, 0);
  s.brk.begin(dknb_tick(), phase2(s.ph));
  ++s.st_breaks, ++dkn_stage(s).breaks;
  s.md_rcpt.clear(), s.md_active = s.helm_active = false;
  dkn_log("STAGGER_A", s, a,
          phase2(s.ph) ? "event=STAGGER_START clip=TNB_DOWN_SIPPU_0x1B window=90 routes=MORTAL_DRAW|SHIELD_ATTACK_HELM_SPLITTER fall=ONLY_ON_EXECUTION_CONTACT hit_cap=NONE"
                       : "event=STAGGER_START clip=TNA_DAMAGE_L_0x09 window=90 hit_cap=NONE routes_f1=MORTAL_DRAW|SHIELD_ATTACK_HELM_SPLITTER damage=120_TOTAL fall=NEVER");
}
bool dkn_link_cutting(int proc) noexcept {
  return proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH || proc == daAlink_c::PROC_CUT_JUMP || proc == daAlink_c::PROC_CUT_TURN ||
         proc == daAlink_c::PROC_CUT_FINISH_JUMP_UP;
}
// Link's Mortal Draw / Helm Splitter really in flight toward THIS actor (the finisher that may extend a window once).
bool dkn_md_flying(const DknSlot& s, daAlink_c* k) noexcept {
  return s.md_active && k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH && kk::mortal_draw_cut(k->getCutType());
}
bool dkn_helm_flying(const DknSlot& s, const daB_TN_c* a, daAlink_c* k) noexcept {
  if (!k) return false;
  const int p = static_cast<int>(k->mProcID);
  return (p == daAlink_c::PROC_CUT_HEAD || p == daAlink_c::PROC_CUT_HEAD_LAND) && k->field_0x280c.getActor() == static_cast<const fopAc_ac_c*>(a) && s.brk.st == dkn::Bk::StaggerB;
}

// FIX02: the guard clip executeGuardL mode 0 would choose (daB_TN_c::getCutType bits 0x10 -> TNB_GUARD_A, 0x20 -> TNB_GUARD_B, else 50/50), read from
// Link's own cut type (never from the Darknut's collider), and the native facing step (cLib_chaseAngleS 0x2000).
int dkn_guard_bck(DknSlot& s, daAlink_c* k) noexcept {
  using P = daPy_py_c;
  switch (k ? k->getCutType() : -1) {
    case P::CUT_TYPE_JUMP: case P::CUT_TYPE_LARGE_JUMP: case P::CUT_TYPE_NM_VERTICAL: case P::CUT_TYPE_NM_RIGHT: case P::CUT_TYPE_FINISH_LEFT:
    case P::CUT_TYPE_FINISH_VERTICAL: case P::CUT_TYPE_DASH_RIGHT:
      return dkn::native::kBckTnbGuardA;
    case P::CUT_TYPE_TURN_LEFT: case P::CUT_TYPE_TURN_RIGHT: case P::CUT_TYPE_LARGE_TURN_LEFT: case P::CUT_TYPE_LARGE_TURN_RIGHT: case P::CUT_TYPE_TWIRL:
    case P::CUT_TYPE_NM_LEFT: case P::CUT_TYPE_DASH_LEFT:
      return dkn::native::kBckTnbGuardB;
    default:
      return s.def.rng.below(2) ? dkn::native::kBckTnbGuardA : dkn::native::kBckTnbGuardB;
  }
}
std::int16_t dkn_guard_facing(const daB_TN_c* a, const daAlink_c* k) noexcept {
  if (!k) return a->shape_angle.y;
  std::int16_t d = static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y);
  if (d > 0x2000) d = 0x2000;
  if (d < -0x2000) d = -0x2000;
  return static_cast<std::int16_t>(a->shape_angle.y + d);
}
// FIX02: every restore of a snapshot goes through the same invariant as the writer (a snapshot taken mid-pass could hold a hit-context entry).
void dkn_restore(DknSlot& s, daB_TN_c* a, int act, int mode) noexcept {
  if (dkn::native::needs_hit_context(act, mode)) {
    ++s.st_unsafe_entry;
    dkn_native_neutral(a, dkn::phase2(s.ph));
    dkn_log("INVARIANT_HIT_CONTEXT_ENTRY_REFUSED", s, a, "entry=ACT_GUARDL_0 reason=NO_COLLIDER_THIS_PASS back=NEUTRAL_CHASE", true);
    return;
  }
  dkn_native_action(a, act, mode);
}
// FIX02: the guard slip THIS Darknut's impact caused (Link's checkDamageAction POST, or the Darknut's damage_check PRE when Link already slipped).
// FIX04: ONE chain step per impact serial (FIX03 advanced it again in the next tick's POST for the same slip); the cap of that impact is re-applied
// idempotently while the claim lives (never lower, never a second step). Per real impact: x0.80 first, x0.65 later = the FIX03 realized profile.
void dkn_slip_cap(DknSlot& s, daAlink_c* k, const daB_TN_c* a, const char* where) noexcept {
  if (!s.slip_pending || !k) return;
  const std::uint64_t now = dknb_tick();
  if (now > s.slip_tick + 2) {
    s.slip_pending = false;
    return;
  }
  if (static_cast<int>(k->mProcID) != daAlink_c::PROC_GUARD_SLIP) return;
  ++s.st_slip_seen;
  const bool fresh = s.slip_done_serial != s.slip_serial;
  if (fresh) {
    s.slip_done_serial = s.slip_serial;
    s.slip_chain = s.last_slip_tick && now - s.last_slip_tick <= dkn::policy::kSlipChainTicks ? s.slip_chain + 1 : 0;
    s.last_slip_tick = now;
    s.slip_cap_now = dkn::link_slip_cap_impact(s.slip_move, s.slip_chain);
  }
  const float cap = s.slip_cap_now, before = k->mNormalSpeed;
  if (before <= cap) return;
  dkn_link_slip_speed(k, cap);
  ++s.st_slip_capped;
  if (s.st_slip_capped <= 60 || s.st_slip_capped % 20 == 0) {
    char ex[260];
    std::snprintf(ex, sizeof(ex), "where=%s move=%s chain=%u impact_serial=%llu reapplied=%d speed_native=%.1f speed_capped=%.2f slide_native=%.0f slide_now=%.0f separation_xz=%.0f", where,
                  dkn::name(s.slip_move), s.slip_chain, static_cast<unsigned long long>(s.slip_serial), fresh ? 0 : 1, static_cast<double>(before), static_cast<double>(cap),
                  static_cast<double>(dkn::slip_distance(before)), static_cast<double>(dkn::slip_distance(cap)), a ? static_cast<double>(dkn_dist(a, k)) : -1.0);
    dkn_log("LINK_SLIP_CAPPED", s, a, ex);
  }
}

// ---------------------------------------------------------------- FIX07: the offensive leap owned by the actor / generation (DknLease::Leap)
// The leap phase ends ONCE: its combo starts (lease Combo: the combo is owned exactly like every other combo), or a legitimate exception cancels it (no target,
// event, Link guard-broken, Link escaped, the bounded wait over, watchdog; the break / HP0 / defeat clear the lease where they always do). Never a late attack:
// a cancelled leap leaves nothing scheduled; a native landing attack is taken back in the action POST (lease Leap) exactly like the reposition's.
void dkn_leap_cancel(DknSlot& s, daB_TN_c* a, const char* why) noexcept {
  if (s.lease != DknLease::Leap) return;
  s.lease = DknLease::None, s.leap_phase = dkn::Lp::None, ++s.st_leap_cancel;
  s.ini.ended(dknb_tick(), dkn::phase2(s.ph));
  char ex[200];
  std::snprintf(ex, sizeof(ex), "reason=%s leap=%s combo=%s owned_ticks=%llu attack_after=NONE initiative=BACK_TO_COMMON_COMBAT", why, dkn::name(s.leap),
                s.leap_combo >= 0 ? dkn::combo_of(static_cast<unsigned>(s.leap_combo)).name : "NONE", static_cast<unsigned long long>(dknb_tick() - s.leap_tick));
  dkn_log("LEAP_CANCEL", s, a, ex, true);
}
bool dkn_move_begin(DknSlot& s, daB_TN_c* a, daAlink_c* k, unsigned idx, DknLease kind, const char* why) noexcept;
void dkn_leap_pre(DknSlot& s, daB_TN_c* a, daAlink_c* k, std::uint64_t now) noexcept {
  using namespace dkn;
  const unsigned total = static_cast<unsigned>(now - s.leap_tick);
  if (!k || !dknb_link_alive()) return dkn_leap_cancel(s, a, "NO_TARGET");
  if (!dknb_world_safe()) return dkn_leap_cancel(s, a, "EVENT");
  if (dknb_link_broken()) return dkn_leap_cancel(s, a, "LINK_GUARD_BROKEN");
  const int act = a->mActionMode1, mode = a->mActionMode2;
  const float dist = dkn_dist(a, k);
  if (s.leap_phase == Lp::Air) {
    if (act == native::kActChaseL && native::jump_mode(mode) && total <= policy::kLeapWatchdogTicks) return;  // take-off, air, the native landing clip
    if (total > policy::kLeapWatchdogTicks) return dkn_leap_cancel(s, a, "WATCHDOG");
    s.leap_phase = Lp::Ground, s.leap_land_tick = now, s.leap_ground = 0, s.leap_land_d = dist, ++s.st_leap_land;
    const float mx = a->current.pos.x - s.leap_x0, mz = a->current.pos.z - s.leap_z0;
    char lx[240];
    std::snprintf(lx, sizeof(lx), "leap=%s distance_start=%.0f distance_landing=%.0f darknut_moved=%.0f predicted=%.0f air_and_landing_ticks=%u native_state=%d/%d", name(s.leap), s.leap_d0,
                  dist, static_cast<double>(kk::fsqrt(mx * mx + mz * mz)), static_cast<double>(leap_model(s.leap).dist), total, act, mode);
    dkn_log("LEAP_LANDED", s, a, lx);
  }
  ++s.leap_ground;
  const LpStep st = leap_ground(dist, dkn_front(a, k), s.leap_ground, total);
  if (st == LpStep::Wait) return;
  if (st != LpStep::StartCombo) return dkn_leap_cancel(s, a, name(st));
  const unsigned ci = static_cast<unsigned>(s.leap_combo);
  s.lease = DknLease::None, s.leap_phase = Lp::None;
  if (!dkn_move_begin(s, a, k, ci, DknLease::Combo, "LEAP_COMBO")) {
    s.lease = DknLease::Leap, s.leap_phase = Lp::Ground;
    return dkn_leap_cancel(s, a, "COMBO_START_REFUSED");
  }
  s.leap_combo_tick = now, s.leap_first_impact_logged = false, ++s.st_leap_combo;
  char cx[240];
  std::snprintf(cx, sizeof(cx), "combo=%s pick=25/25/25/25 since_leap=%u since_landing=%llu distance=%.0f first_impact_in=%u owner=ACTOR_GENERATION", combo_of(ci).name, total,
                static_cast<unsigned long long>(now - s.leap_land_tick), dist, first_impact_tick(ci));
  dkn_log("LEAP_COMBO_START", s, a, cx);
}


// ---------------------------------------------------------------- FIX08-04: the defensive evasion owned by the actor / generation (DknLease::Evade)
// The whole native path of one evasion is read before it starts (never only its landing point): ground under the samples at 1/3, 2/3 and the end of the simulated
// arc (+ the body radius beyond the landing), the obstacle line along the two halves of the arc at waist height, water over the landing. Link's own position is
// never crossed (dkn::evade_path keeps the arc around him). Read once per decision, only for the directions actually considered.
DknGeo dkn_geo_evade(const daB_TN_c* a, const dkn::EvPath& pth) noexcept {
  using namespace dkn;
  DknGeo r;
  if (pth.n < 3) return r.g = Geo::NoGround, r;
  const float feet = a->current.pos.y, x0 = a->current.pos.x, z0 = a->current.pos.z;
  const unsigned idx[3] = {pth.n / 3, (2 * pth.n) / 3, pth.n - 1};
  Geo smp[4];
  for (unsigned i = 0; i < 4; ++i) {
    unsigned j = idx[i < 3 ? i : 2];
    float px = x0 + pth.x[j], pz = z0 + pth.z[j];
    if (i == 3) {  // the body beyond the landing, along the last step
      const float dx = pth.x[pth.n - 1] - pth.x[pth.n - 2], dz = pth.z[pth.n - 1] - pth.z[pth.n - 2], l = kk::fsqrt(dx * dx + dz * dz);
      if (l > 0.5f) px += dx / l * policy::kLeapBodyR, pz += dz / l * policy::kLeapBodyR;
    }
    cXyz q(px, feet + policy::kLeapProbeUp, pz);
    const bool hit = fopAcM_gc_c::gndCheck(&q);
    const float gy = fopAcM_gc_c::mGroundY;
    r.ground[i] = hit ? gy - feet : -9999.0f;
    smp[i] = geo_sample(!hit, gy, feet);
  }
  r.land_x = x0 + pth.x[pth.n - 1], r.land_z = z0 + pth.z[pth.n - 1];
  const unsigned mid = pth.n / 2;
  cXyz s0(x0, feet + policy::kLeapLineY, z0), s1(x0 + pth.x[mid], feet + policy::kLeapLineY, z0 + pth.z[mid]), s2(r.land_x, feet + policy::kLeapLineY, r.land_z);
  const bool wall = fopAcM_lc_c::lineCheck(&s0, &s1, a) || fopAcM_lc_c::lineCheck(&s1, &s2, a);
  cXyz lw(r.land_x, feet + policy::kLeapProbeUp, r.land_z);
  const bool water = fopAcM_wt_c::waterCheck(&lw) && fopAcM_wt_c::mWaterY > feet + r.ground[2] + 20.0f;
  r.g = geo_path(smp, 4, wall, water);
  return r;
}
void dkn_evade_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why, bool cancel) noexcept {
  if (s.lease != DknLease::Evade) return;
  const std::uint64_t now = dknb_tick();
  s.lease = DknLease::None, s.evade_land_tick = now, s.evade_link_close = false;
  if (cancel) ++s.evade.cancels;
  else ++s.evade.lands;
  s.ngate.defense(now, 4);  // the next ordinary initiative after an evasion passes through the neutral chase (never a counter out of the jump)
  const float mx = a->current.pos.x - s.evade_x0, mz = a->current.pos.z - s.evade_z0;
  char ex[260];
  std::snprintf(ex, sizeof(ex), "reason=%s dir=%s swing_key=%llu ticks=%llu distance_start=%.0f distance_end=%.0f darknut_moved=%.0f hit_during=%d next=NEUTRAL_RECEIPT leap_hold=%u(PROVISIONAL)", why,
                dkn::name(s.evade_dir), static_cast<unsigned long long>(s.evade_key), static_cast<unsigned long long>(now - s.evade_tick), static_cast<double>(s.evade_d0),
                k ? static_cast<double>(dkn_dist(a, k)) : -1.0, static_cast<double>(kk::fsqrt(mx * mx + mz * mz)), s.evade_hit ? 1 : 0, dkn::policy::kEvadeLeapHoldTicks);
  dkn_log(cancel ? "EVADE_CANCEL" : "EVADE_LANDED", s, a, ex, cancel);
}
// PRE of a tick inside the evasion lease: the legitimate exceptions end it (no target, event, Link guard-broken, watchdog); the end of the native jump ends it.
void dkn_evade_pre(DknSlot& s, daB_TN_c* a, daAlink_c* k, std::uint64_t now) noexcept {
  if (!k || !dknb_link_alive()) return dkn_evade_end(s, a, k, "NO_TARGET", true);
  if (!dknb_world_safe()) return dkn_evade_end(s, a, k, "EVENT", true);
  if (dknb_link_broken()) return dkn_evade_end(s, a, k, "LINK_GUARD_BROKEN", true);
  if (now > s.evade_tick + dkn::policy::kEvadeWatchdogTicks) {
    dkn_native_neutral(a, true);
    return dkn_evade_end(s, a, k, "WATCHDOG", true);
  }
  if (a->mActionMode1 == dkn::native::kActChaseL && dkn::evade_jump_mode(a->mActionMode2)) return;  // take-off, flight, the native landing clip
  dkn_evade_end(s, a, k, "LANDED_NATIVE", false);
}
// FIX08-01: the Helm recovery look ends (the native neck flag back OFF: its native value for mType 1; mNeckAngle then decays natively to 0).
void dkn_helm_look_end(DknSlot& s, daB_TN_c* a, daAlink_c* k, const char* why) noexcept {
  if (!s.helm_look.on()) return;
  s.helm_look.end();
  if (s.neck_on) dkn_native_neck(a, false), s.neck_on = false;
  const int err = k ? static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y) : 0;
  char ex[200];
  std::snprintf(ex, sizeof(ex), "reason=%s body_error_start=%d body_error_now=%d neck_angle=%d neck_flag=OFF(native) turn_ticks=%llu", why, static_cast<int>(s.helm_err0), err,
                static_cast<int>(a->mNeckAngle), static_cast<unsigned long long>(s.helm_look.turn_ticks));
  dkn_log("HELM_LOOK_END", s, a, ex);
}

// ---------------------------------------------------------------- hooks: action
HookAction on_dkn_action_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a || a->mType != 1) return HOOK_CONTINUE;  // the Temple of Time Darknut (mType 0) never reaches the admission
  DknSlot* sp = dkn_admit(a);
  if (!sp) return HOOK_CONTINUE;
  DknSlot& s = *sp;
  dkn_direct_maintenance();
  dkn_phase_track(s, a);
  if (!dkn_combat(s)) return HOOK_CONTINUE;
  using namespace dkn;
  const std::uint64_t now = dknb_tick();
  const bool p2 = phase2(s.ph);
  daAlink_c* k = dknb_link();
  if (k) s.gap.observe(now, dkn_dist(a, k));  // FIX07: the offensive-leap episode closes when Link comes within the initiative reach (or by time)
  if (s.lease != DknLease::Leap) s.leap_phase = Lp::None;
  // ---- Link's action key (blocked-attack dedupe), his NEW swings and his NEW Shield Attacks
  s.swing_new = false;
  int lp = -1;
  if (k) {
    lp = dknb_link_proc(k);
    const int lc = k->getCutType(), ln = k->getCutCount();
    if (classify(lp) == Act::Active && (lp != s.link_prev_proc || lc != s.link_prev_cut || ln != s.link_prev_count)) {
      ++s.link_key;
      if (dkn_link_cutting(lp)) s.swing_new = true, s.swing_key = s.link_key, s.swing_seen_tick = now;  // FIX08: the age of the running swing (swing_threat)
    }
    if (lp == daAlink_c::PROC_GUARD_ATTACK && s.link_prev_proc != daAlink_c::PROC_GUARD_ATTACK) ++s.sa_serial, s.sa_tick = now;
    s.link_prev_proc = lp, s.link_prev_cut = lc, s.link_prev_count = ln;
  }
  // FIX04: Link's chain against this Darknut: every NEW swing key in front gets its ordinal, whatever the Darknut is doing. FIX05: tracked within
  // kSwingTrackReach (a step-in cut / a jump attack belongs to the chain), its ONE verdict only within kGuardReach (now, or while still young: late roll).
  int swing_ord = 0;  // > 0: the current chain swing may receive its ONE verdict this tick
  bool late_roll = false;
  s.def.tick(now);
  if (k) {
    const float d0 = dkn_dist(a, k);
    const bool f0 = dkn_front(a, k);
    if (s.swing_new) {
      const Sw r = swing_reach(d0, f0);
      if (r != Sw::Ignore) {
        const unsigned o = s.def.on_link_swing(now, s.swing_key);
        s.swing_reg_tick = now, s.swing_reg_key = s.swing_key;
        if (r == Sw::Roll) swing_ord = static_cast<int>(o);
        else ++s.st_tracked_far;
      }
    } else if (s.swing_reg_key && s.swing_reg_key == s.def.cur_key && s.def.rolled_key != s.swing_reg_key && s.link_key == s.swing_reg_key && dkn_link_cutting(lp) &&
               roll_late_ok(now, s.swing_reg_tick) && swing_reach(d0, f0) == Sw::Roll) {
      swing_ord = static_cast<int>(s.def.cur_ord), late_roll = true;
    }
  }
  const bool shield_ok = p2 || (a->mNextBreakPart < 11 && a->mStates[12] == 0);  // phase 1: no parry / no shield defense once the shield is gone (native >= 11)
  // FIX04: a parry rebound is CREDITED (Link +20 / +25, exactly once) only when Link's native PROC_CUT_REVERSE is read here, after procCutReverseInit ran;
  // the ONE counter is chosen then and scheduled (never in the rebound tick). A break that preempted it keeps only the credit.
  // FIX05: that read must PERSIST: the first one (the init, same tick) is only SEEN; the credit / counter come from a later read with the recoil frame
  // advanced; if the rebound is gone by then (another procedure, a restarted clip) nothing is credited and no counter is ever scheduled.
  if ((s.def.cp == Cp::Await || s.def.cp == Cp::CreditOnly) && k) {
    const bool credit_only = s.def.cp == Cp::CreditOnly;
    const bool rev = lp == daAlink_c::PROC_CUT_REVERSE;
    const float rf = rev ? k->mUnderFrameCtrl[0].getFrame() : 0.0f;
    const std::uint64_t seen_tick = s.def.reverse_tick;
    const float seen_frame = s.def.reverse_frame;
    const Rb rb = s.def.observe_rebound(now, rev, rf, p2, shield_ok);
    if (rb == Rb::Seen && s.def.reverse_tick == now) {
      ++s.st_rebound_seen;
      char ex[160];
      std::snprintf(ex, sizeof(ex), "rebound_tick=%llu link_proc=PROC_CUT_REVERSE read=INIT recoil_frame=%.1f credit=PENDING_PERSISTENCE", static_cast<unsigned long long>(s.def.parry_tick),
                    static_cast<double>(rf));
      dkn_log("ENEMY_PARRY_REBOUND_SEEN", s, a, ex);
    } else if (rb == Rb::Lost) {
      ++s.st_rebound_lost;
      char ex[200];
      std::snprintf(ex, sizeof(ex), "rebound_tick=%llu seen_tick=%llu seen_frame=%.1f link_proc_now=%d frame_now=%.1f credit=0 counter=0 reason=REBOUND_NOT_PERSISTING",
                    static_cast<unsigned long long>(s.def.parry_tick), static_cast<unsigned long long>(seen_tick), static_cast<double>(seen_frame), lp, static_cast<double>(rf));
      dkn_log("ENEMY_PARRY_REBOUND_LOST", s, a, ex, true);
    }
    if (rb == Rb::Confirmed) {
      ++s.st_enemy_parry, ++dkn_stage(s).parries;
      const int pts = p2 ? policy::kLinkEnemyParry2 : policy::kLinkEnemyParry1;
      const DknGain gn = dknb_link_gain(pts, false, s.id, s.generation, 0x4000000000000000ull | s.def.parries);
      if (gn.confirmed) ++s.st_parry_gain;
      char ex[320];
      std::snprintf(ex, sizeof(ex), "rebound=%llu proc=CUT_REVERSE persisted=1 seen=%llu@%.1f read=%llu@%.1f ord=%u link_gain=+%d ok=%d link=%d.%03d->"
                    "%d.%03d sat=%d counter=%s at=%llu land>=%u", static_cast<unsigned long long>(s.def.parry_tick),
                    static_cast<unsigned long long>(seen_tick), static_cast<double>(seen_frame), static_cast<unsigned long long>(now), static_cast<double>(rf), s.def.parry_ordinal, pts,
                    gn.confirmed ? 1 : 0, gn.before / 1000,
                    gn.before % 1000, gn.after / 1000, gn.after % 1000, gn.saturated ? 1 : 0, credit_only ? "NONE_BREAK_PRIORITY" : combo_of(static_cast<unsigned>(s.def.counter_idx)).name,
                    static_cast<unsigned long long>(credit_only ? 0 : s.def.counter_at), policy::kCounterLandTicks);
      dkn_log("ENEMY_PARRY_REAL", s, a, ex);
      // FIX06: the authentic parry opens the SEQUENCE of this actor / generation (its ONE route was rolled by the confirmation): it owns the initiative now
      if (!credit_only && s.def.cp == Cp::Scheduled) {
        if (s.seq.begin(now, s.def.parry_tick, s.def.route_combo, s.def.counter_idx)) {
          const unsigned pi = static_cast<unsigned>(s.def.counter_idx);
          char sx[260];
          std::snprintf(sx, sizeof(sx), "route=%s roll=ONE_PER_CONFIRMED_PARRY pct=%d/%d plan=%s start=%llu first_impact_after_rebound=%llu window=%u..%u owner=ACTOR_GENERATION",
                        s.def.route_combo ? "DIRECT_COMBO_60" : "COUNTER_40", policy::kParryComboPct, 100 - policy::kParryComboPct, combo_of(pi).name,
                        static_cast<unsigned long long>(s.def.counter_at),
                        static_cast<unsigned long long>(s.def.counter_at - s.def.parry_tick + first_impact_tick(pi)), s.def.route_combo ? policy::kParryComboLandMin : policy::kCounterLandTicks,
                        s.def.route_combo ? policy::kParryComboLandMax : 999u);
          dkn_log("SEQ_BEGIN", s, a, sx);
        } else {  // never stacked over a live sequence: nothing of this confirmation is scheduled
          s.def.cp = Cp::None, ++s.def.counter_dropped;
          dkn_log("SEQ_REFUSED", s, a, "reason=SEQUENCE_ALREADY_OWNS_THE_INITIATIVE scheduled=DROPPED", true);
        }
      }
    }
  }
  // ---- priority 4: a posture break preempts everything below (the native transformation / defeat preempt it: they are not combat)
  if (s.brk.st == Bk::Pending) {
    const int act = a->mActionMode1;
    if (act == native::kActChangeDemo || act == native::kActEnding) s.brk.abort(BkEnd::Phase);
    else dkn_begin_break(s, a);
  }
  if (s.brk.vulnerable()) dkn_native_stagger_hold(a, false);
  if (s.md_active && !dkn_md_flying(s, k) && now > s.md_tick + 2) {
    s.md_active = false;
    dkn_log("MORTAL_END", s, a, "reason=LINK_LEFT_MORTAL_DRAW fall=0");
  }
  // ---- owned-move safety
  if (dkn_owned_move(s) && !s.tail && (!k || !dknb_link_alive())) {
    dkn_move_end(s, a, "NO_TARGET", true);
    dkn_native_neutral(a, p2);
  }
  if (dkn_owned_move(s) && s.ct > policy::kComboWatchdogTicks) {
    dkn_move_end(s, a, "WATCHDOG", true);
    dkn_native_neutral(a, p2);
  }
  if (s.brk.st != Bk::Combat) return HOOK_CONTINUE;  // stagger / recovery: nothing below runs
  if (s.lease == DknLease::Leap) {  // FIX07: the offensive leap owns the Darknut until its combo starts (Link's ordinary actions are not inputs)
    dkn_leap_pre(s, a, k, now);
    if (s.lease != DknLease::None) return HOOK_CONTINUE;
  }
  if (s.lease == DknLease::Evade) {  // FIX08: the defensive evasion owns the Darknut until its native landing (Link's ordinary actions are not inputs)
    dkn_evade_pre(s, a, k, now);
    if (s.lease != DknLease::None) return HOOK_CONTINUE;
  }
  if (s.evade_land_tick && !s.evade_link_close && k && dkn_dist(a, k) <= policy::kEngage2) s.evade_link_close = true;  // FIX08: Link closed in after the evasion
  // ---- the native AI never starts an attack: any attack entry it wrote without our lease is taken back BEFORE it runs
  const int act0 = a->mActionMode1, mode0 = a->mActionMode2;
  bool from_native = s.native_attempt;  // FIX03: an entry already taken back in the last action POST is that native attempt
  s.native_attempt = false;
  if (s.lease == DknLease::None && native::native_leap(act0, mode0)) {  // FIX02: the native forward leap (an attack approach) becomes the native walk
    dkn_native_walk(a);
    ++s.st_leaps;
    if (s.st_leaps <= 30 || s.st_leaps % 20 == 0) {
      char ex[96];
      std::snprintf(ex, sizeof(ex), "native_mode=%d back=TNB_WALK_mode3 total=%llu", mode0, static_cast<unsigned long long>(s.st_leaps));
      dkn_log("NATIVE_LEAP_SUPPRESSED", s, a, ex);
    }
  }
  // FIX08: the native AI's own evasion entries (checkNextMove initChaseL 6 / 11 / 14: Link attacking / too close / a slow jump attack) are taken back before they
  // run: the only evasion of phases 2-3 is the approved one below (FIX07 log 14562: an uncontrolled side jump opened distance and an ordinary combo followed).
  if (s.lease == DknLease::None && p2 && evade_native_entry_any(act0, mode0)) {
    dkn_native_neutral(a, true);
    ++s.evade.native_suppressed;
    if (s.evade.native_suppressed <= 30 || s.evade.native_suppressed % 25 == 0) {
      char ex[120];
      std::snprintf(ex, sizeof(ex), "native_mode=%d executed=0 total=%llu back=NEUTRAL_CHASE evasion=APPROVED_POLICY_ONLY", mode0, static_cast<unsigned long long>(s.evade.native_suppressed));
      dkn_log("NATIVE_EVADE_SUPPRESSED", s, a, ex);
    }
  }
  if (s.lease == DknLease::None && native::native_attack_entry(act0, mode0)) {
    dkn_native_neutral(a, p2);
    ++s.st_suppressed, ++dkn_stage(s).suppressed, from_native = true;
    if (s.st_suppressed <= 40 || now - s.last_suppress_log >= 300) {
      s.last_suppress_log = now;
      char ex[120];
      std::snprintf(ex, sizeof(ex), "native_act=%d native_mode=%d total=%llu back=NEUTRAL_CHASE authority=OWN_CONTROLLER", act0, mode0, static_cast<unsigned long long>(s.st_suppressed));
      dkn_log("NATIVE_CHAIN_SUPPRESSED", s, a, ex);
    }
  }
  if (s.lease != DknLease::None) return HOOK_CONTINUE;
  const int act = a->mActionMode1, mode = a->mActionMode2;
  const float dist = k ? dkn_dist(a, k) : 1.0e9f;
  const bool front = k && dkn_front(a, k);
  // ---- priority 6 (FIX03/FIX04: every phase): the parry window, the ONE counter after a CONFIRMED parry, ONE defense verdict per new Link swing
  // the armed window, and the parry contact itself (its damage_check may run this tick): the native guard stays up, so that contact is a block, never an
  // HP hit (FIX03 log 7640 / 38910: the counter's own flags dropped field_0xa91 before that damage_check)
  if ((s.def.window_open(now) || s.def.parry_hold(now)) && front && shield_ok) dkn_native_guard(a, 2);
  {  // the ONE move a confirmed parry scheduled, at its tick: FIX05 the counter (first impact >= 24 ticks after the rebound); FIX06 the counter (40 %) or the
     // direct combo (60 %, first impact 27 ticks after the rebound). Dropped (with its sequence) when the context changed meanwhile.
    int c = s.def.due(now);
    if (c >= 0) {
      const bool combo = !is_single(static_cast<unsigned>(c));
      if (!combo && c == static_cast<int>(kIdxF1Bash) && !shield_ok) c = static_cast<int>(s.def.rng.below(2) ? kIdxF1SingleA : kIdxF1SingleB);  // the shield is gone: a mace alternative
      const char* why = !k || !dknb_link_alive() ? "NO_TARGET" : dknb_link_broken() ? "LINK_GUARD_BROKEN" : !dknb_world_safe() ? "EVENT" : dist > policy::kComboLeaveDist ? "LINK_FAR" : nullptr;
      if (!why && dkn_move_begin(s, a, k, static_cast<unsigned>(c), combo ? DknLease::Combo : DknLease::Counter, combo ? "PARRY_DIRECT_COMBO" : "AFTER_ENEMY_PARRY_REAL")) {
        s.seq.started(combo);
        return HOOK_CONTINUE;
      }
      ++s.st_counter_dropped;
      char ex[120];
      std::snprintf(ex, sizeof(ex), "%s=%s reason=%s late_counter=0", combo ? "combo" : "counter", combo_of(static_cast<unsigned>(c)).name, why ? why : "START_REFUSED");
      dkn_log("COUNTER_DROPPED", s, a, ex);
      dkn_seq_end(s, a, why ? seq_end_of(why) : SqEnd::Refused, why ? why : "START_REFUSED");
    }
  }
  // FIX06: the sequence owns the Darknut from the confirmed parry to its last recovery. Nothing below runs over it: no defense verdict (no parry window
  // stacked on the committed attack), no exchange hold, no passivity special, no common initiative. Link's contacts keep their real damage / block /
  // posture (damage_check / setDamage hooks). The bridge after the counter starts the continuation combo at bridge_due; a Link held by a forced state
  // (slip / damage / knock-down) is waited for (then kBridgeFreeTicks more), never forever.
  if (s.seq.owns()) {
    GateIn gi;
    gi.link_ok = k && dknb_link_alive(), gi.link_broken = dknb_link_broken(), gi.safe = dknb_world_safe(), gi.link_held = classify(lp) == Act::Hold;
    gi.dist = dist, gi.now = now, gi.cp = s.def.cp;
    const GateOut go = seq_gate(s.seq, gi);  // the pure authority: Link's ordinary actions are not among its inputs
    if (go.g == Gate::End) {
      dkn_seq_end(s, a, go.why, "SEQ_GATE");
    } else if (go.g == Gate::StartContinuation) {
      const unsigned ci = static_cast<unsigned>(s.seq.idx);
      const std::uint64_t ce = s.seq.counter_end, li = s.seq.last_impact;
      const bool was_held = s.seq.held;
      if (dkn_move_begin(s, a, k, ci, DknLease::Combo, "COUNTER_TO_COMBO")) {
        s.seq.started(true);
        char bx[220];
        std::snprintf(bx, sizeof(bx), "combo=%s since_counter_end=%llu since_last_counter_impact=%llu first_impact_after_last=%llu link_was_held=%d", combo_of(ci).name,
                      static_cast<unsigned long long>(now - ce), static_cast<unsigned long long>(now - li), static_cast<unsigned long long>(now - li + first_impact_tick(ci)),
                      was_held ? 1 : 0);
        dkn_log("SEQ_COMBO_START", s, a, bx);
        return HOOK_CONTINUE;
      }
      dkn_seq_end(s, a, SqEnd::Refused, "CONTINUATION_START_REFUSED");
    } else if (gi.link_held && s.seq.st == Sq::Bridge) {
      ++s.st_seq_bridge_held;
    }
    if (s.seq.owns()) return HOOK_CONTINUE;
  }
  // FIX06 / FIX07: the ONE reposition decision after the FIX02 rise / a recoverable getup (phases 2-3): jump back / hold / jump forward on the native jumps.
  // FIX07: every jump's trajectory is read first (dkn_geo: ground, drop, rise, wall, water; the back jump too); Link not in front gets the native turn first
  // (at most kRepoTurnTicks, the decision stays pending); the long native leap (17) where the short one stops too far. Never a second decision.
  if (s.repo_pending) {
    const std::uint64_t deadline = s.repo_turn_since ? s.repo_turn_since + policy::kRepoTurnTicks : s.repo_since + policy::kRepoWaitTicks;
    if (!p2 || now > deadline) {
      s.repo_pending = false, ++s.st_repo_cancel;
      dkn_log("REPOSITION_SKIPPED", s, a, !p2 ? "reason=NOT_A_SWORD_STAGE" : s.repo_turn_since ? "reason=LINK_NOT_IN_FRONT_AFTER_NATIVE_TURN jump=0" : "reason=NO_NEUTRAL_NATIVE_STATE_IN_TIME jump=0");
    } else if (native::neutral_chase(true, act, mode)) {
      const float dy = k ? k->current.pos.y - a->current.pos.y : 0.0f;
      const bool safe = k && dknb_link_alive() && dknb_world_safe() && a->mAcch.ChkGroundHit();
      const bool turn_left = !s.repo_turn_since || now < s.repo_turn_since + policy::kRepoTurnTicks;
      const std::int16_t to_link = k ? dkn_angle_to(a, k) : a->shape_angle.y;
      const int arc = static_cast<std::int16_t>(to_link - a->shape_angle.y);
      // the native verdict with the geometry assumed clear tells which trajectories have to be read (one decision, a few native queries)
      const Rp v0 = k ? repo_verdict7(dist, dy, safe, front, turn_left, true, true, true) : Rp::Unsafe;
      DknGeo gb, g8, g17;
      if (v0 == Rp::Back) gb = dkn_geo(a, dkn_dir_to(a, k, true), leap_model(Leap::Back).dist);
      if (v0 == Rp::Forward || v0 == Rp::ForwardLong)
        g8 = dkn_geo(a, dkn_dir_to(a, k, false), leap_model(Leap::Fwd).dist), g17 = dkn_geo(a, dkn_dir_to(a, k, false), leap_model(Leap::FwdLong).dist);
      const Rp v = k ? repo_verdict7(dist, dy, safe, front, turn_left, gb.g == Geo::Ok, g8.g == Geo::Ok, g17.g == Geo::Ok) : Rp::Unsafe;
      if (v == Rp::WaitFront) {
        if (!s.repo_turn_since) {
          s.repo_turn_since = now, ++s.st_repo_wait_front;
          char wx[160];
          std::snprintf(wx, sizeof(wx), "verdict=WAIT_FRONT_NATIVE_TURN distance=%.0f angle=%d max_wait=%u(PROVISIONAL) decision=PENDING", dist, arc, policy::kRepoTurnTicks);
          dkn_log("REPOSITION_WAIT", s, a, wx);
        }
      } else {
        s.repo_pending = false;
        const Leap l = v == Rp::Back ? Leap::Back : v == Rp::Forward ? Leap::Fwd : v == Rp::ForwardLong ? Leap::FwdLong : Leap::None;
        const Jump jm = leap_model(l);
        const DknGeo& gu = l == Leap::Back ? gb : l == Leap::Fwd ? g8 : g17;
        s.repo = v, s.repo_d0 = dist, s.repo_leap = l, s.repo_x0 = a->current.pos.x, s.repo_z0 = a->current.pos.z;
        char rx[400];
        std::snprintf(rx, sizeof(rx), "verdict=%s distance=%.0f angle=%d dy=%.0f front=%d turned_ticks=%llu bands=<%.0f_back,%.0f..%.0f_hold,>%.0f_forward leap=%s predicted_jump=%.0f "
                      "predicted_distance=%.0f geo_back=%s geo_fwd8=%s geo_fwd17=%s ground_dy=%.0f/%.0f/%.0f/%.0f safe=%d",
                      name(v), dist, arc, dy, front ? 1 : 0, static_cast<unsigned long long>(s.repo_turn_since ? now - s.repo_turn_since : 0), policy::kRepoNear, policy::kRepoNear,
                      policy::kRepoFar, policy::kRepoFar, name(l), l == Leap::None ? 0.0f : jm.dist, l == Leap::Back ? dist + jm.dist : l == Leap::None ? dist : dist - jm.dist,
                      name(gb.g), name(g8.g), name(g17.g), static_cast<double>(gu.ground[0]), static_cast<double>(gu.ground[1]), static_cast<double>(gu.ground[2]),
                      static_cast<double>(gu.ground[3]), safe ? 1 : 0);
        dkn_log("REPOSITION_DECISION", s, a, rx);
        if (l != Leap::None) {
          dkn_native_jump(a, leap_mode(l));
          s.lease = DknLease::Reposition, s.repo_back = l == Leap::Back, s.repo_tick = now, ++s.st_repo_jumps;
          s.st_repo_long += l == Leap::FwdLong;
          return HOOK_CONTINUE;
        }
        ++s.st_repo_holds;
        s.st_repo_geo_unsafe += v == Rp::GeoUnsafe;
      }
    }
  }
  // FIX04: the exchange Link's chain owns. A valid chain (recent swings, defended contacts) against a REAL defense (phase 1: the shield) in front and in
  // reach: no offensive decision of any kind (combo, single, special, a native attempt) starts over it. It lets go naturally: a pause > 1.5 s, two empty
  // swings, Link far / behind / down / guard-broken, no defense left. An offensive already running is never cut by it.
  const bool can_defend = p2 || shield_ok;
  // FIX05: the pure verdict (dkn::hold_verdict): within the initiative reach, distance alone never releases a valid chain
  const Hold hv = hold_verdict(s.def, now, can_defend, k && dknb_link_alive(), dknb_link_broken(), dist, front, p2);
  const bool hold = hv == Hold::Holds;
  if (hold) ++s.st_hold_ticks, s.st_hold_beyond_300 += !(dist < policy::kGuardReach);
  if (s.def_holding && !hold) {
    ++s.st_hold_releases;
    s.ini.ended(now, p2);  // the controller takes the initiative back with its ordinary gap, never with a timer that ran out during the defense
    s.ngate.defense(now, 3);  // FIX08: and only through the neutral chase (DEFENSE -> NEUTRAL -> ORDINARY_INIT)
    char ex[200];
    std::snprintf(ex, sizeof(ex), "reason=%s chain=%u whiffs=%u distance=%.0f last_swing_age=%llu gap_after=%llu", name(hv), s.def.chain, s.def.whiffs, dist,
                  static_cast<unsigned long long>(s.def.last_swing && now >= s.def.last_swing ? now - s.def.last_swing : 0),
                  static_cast<unsigned long long>(s.ini.gap_until > now ? s.ini.gap_until - now : 0));
    dkn_log("DEFENSE_RELEASE", s, a, ex);
  }
  s.def_holding = hold;
  if (swing_ord > 0 && !s.def.busy()) {
    const Dv v = s.def.on_swing(now, static_cast<unsigned>(swing_ord), shield_ok, p2);
    s.st_late_rolls += late_roll;
    // FIX08-04: the defensive evasion. Decided ONLY when the parry roll of this very swing armed no window (the parry has priority), once per swing key, in phases
    // 2-3, for a human Link in combat, on the ground, free of every lease / window / sequence / break / reposition, out of the guard knockback and any jump.
    if (v != Dv::Parry && p2 && k) {
      const bool human = !k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);
      const bool knock = act == native::kActGuardL && (mode == 2 || mode == 3);
      const bool jumping = act == native::kActChaseL && (native::jump_mode(mode) || evade_jump_mode(mode));
      EvadeView ev;
      ev.phase2 = p2, ev.ordinal = static_cast<unsigned>(swing_ord), ev.key = s.def.cur_key, ev.now = now;
      ev.eligible = human && dknb_link_alive() && dknb_world_safe() && !dknb_link_broken() && a->mAcch.ChkGroundHit() && !s.repo_pending && !s.exec && !knock && !jumping &&
                    s.def.cp == Cp::None && !s.def.armed;
      const Ev er = s.evade.decide(ev);
      if (er == Ev::Won) {
        const float lx = k->current.pos.x - a->current.pos.x, lz = k->current.pos.z - a->current.pos.z;
        const int side = static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y);
        const EvPath pl = evade_path(Ed::Left, lx, lz, a->shape_angle.y), pr = evade_path(Ed::Right, lx, lz, a->shape_angle.y), pb = evade_path(Ed::Back, lx, lz, a->shape_angle.y);
        const DknGeo gl = dkn_geo_evade(a, pl), gr = dkn_geo_evade(a, pr), gb = dkn_geo_evade(a, pb);
        const Ed dir = evade_pick(dist, side, (s.def.cur_key & 1) != 0, gl.g == Geo::Ok, gr.g == Geo::Ok, gb.g == Geo::Ok);
        const EvPath& pu = dir == Ed::Left ? pl : dir == Ed::Right ? pr : pb;
        char ex[360];
        std::snprintf(ex, sizeof(ex), "swing_key=%llu ordinal=%d chance=%d roll=WON dir=%s distance=%.0f link_side=%d geo=L:%s,R:%s,B:%s predicted_move=%.0f samples=%u cooldown=%u parry_roll=%s",
                      static_cast<unsigned long long>(s.def.cur_key), swing_ord, policy::kEvadeOrdinalPct[swing_ord - 1], name(dir), dist, side, name(gl.g), name(gr.g), name(gb.g),
                      static_cast<double>(pu.dist), pu.n, policy::kEvadeCooldownTicks, v == Dv::Guard ? "ORDINARY_GUARD" : "NONE");
        if (dir == Ed::None) {  // no safe direction: the ordinary defense below, the roll stays consumed (no reroll)
          ++s.evade.no_direction;
          dkn_log("EVADE_REFUSED_GEOMETRY", s, a, ex);
        } else {
          dkn_native_jump(a, evade_mode(dir));
          s.lease = DknLease::Evade, s.evade_dir = dir, s.evade_tick = now, s.evade_key = s.def.cur_key, s.evade_hit = false, s.evade_d0 = dist;
          s.evade_x0 = a->current.pos.x, s.evade_z0 = a->current.pos.z, s.evade_land_tick = 0, s.evade_link_close = false;
          s.evade.started(now, dir);
          s.def.cur_contacted = true;  // the evaded swing is a defended one, never an "empty swing" that would hand the initiative over (no chain step either)
          s.ngate.defense(now, 4);
          dkn_log("EVADE_START", s, a, ex);
          return HOOK_CONTINUE;
        }
      } else if (er != Ev::NotEligible && er != Ev::Rerolled && s.st_evade_lines < 120) {  // sampled; the summary counts every decision
        ++s.st_evade_lines;
        char ex[200];
        std::snprintf(ex, sizeof(ex), "swing_key=%llu ordinal=%d chance=%d result=%s parry_roll=%s cooldown_left=%llu", static_cast<unsigned long long>(s.def.cur_key), swing_ord,
                      policy::kEvadeOrdinalPct[swing_ord - 1], name(er), v == Dv::Guard ? "ORDINARY_GUARD" : "NONE",
                      static_cast<unsigned long long>(s.evade.cooldown_until > now ? s.evade.cooldown_until - now : 0));
        dkn_log("EVADE_DECISION", s, a, ex);
      }
    }
    if (v != Dv::None) {
      const bool guarding = act == (p2 ? native::kActGuardL : native::kActGuardH) && mode == 1;
      if (!guarding) {  // FIX02/FIX03: entered at mode 1 with the native guard clip: never ACT_GUARDL 0 without a hit
        if (p2) dkn_native_guard_enter(a, dkn_guard_bck(s, k), dkn_guard_facing(a, k));
        else dkn_native_guard_enter_h(a, dkn_guard_facing(a, k));
      }
      s.guard_tick = now;
      s.ngate.defense(now, 1);  // FIX08: a real defense: the next ordinary initiative needs the neutral chase first
      ++s.st_guards, ++dkn_stage(s).guards;
      if (v == Dv::Parry) ++s.st_parry_attempts;
      char ex[240];
      std::snprintf(ex, sizeof(ex), "verdict=%s ordinal=%d attempt_pct=%d pressure=%u window=%llu..%llu link_cut=%d guard_clip=%s shield=%d exchange=%s distance=%.0f late=%d",
                    v == Dv::Parry ? "PARRY_ATTEMPT" : "ORDINARY_GUARD", swing_ord, policy::kParryOrdinalPct[swing_ord - 1], s.def.pressure(now), static_cast<unsigned long long>(s.def.window_from),
                    static_cast<unsigned long long>(s.def.window_to), k->getCutType(), guarding ? "KEPT" : p2 ? "TNB_GUARD" : "TNA_GUARD", shield_ok ? 1 : 0, hold ? "LINK_CHAIN" : "OPEN", dist,
                    late_roll ? 1 : 0);
      dkn_log("ADAPTIVE_GUARD", s, a, ex);
      return HOOK_CONTINUE;
    }
  }
  // ---- neutral native states the controller may leave: chase / strafe / wait; a native guard clip after kGuardLeaveTicks (never during an armed window)
  const bool guard_state = act == (p2 ? native::kActGuardL : native::kActGuardH) && mode == 1;
  // FIX03: an armed parry window or a counter still waiting owns the Darknut; FIX04: so does Link's valid chain (hold)
  const bool def_busy = s.def.busy();
  const bool neutral = !def_busy && !hold && (native::neutral_chase(p2, act, mode) || (guard_state && now >= s.guard_tick + policy::kGuardLeaveTicks && !s.def.window_open(now)));
  // ---- passivity: independent special (bash / kick), only from neutral, never during the re-entry
  if (k) {
    const bool reentry = now < s.ini.reentry_until;
    const Pv pv = s.pas.step(classify(lp), dknb_link_guarding(k), !neutral || reentry);
    if (pv != Pv::None && !special_allowed(pv, p2)) {  // FIX05: F2/F3 never kick an idle Link (only a held shield): the idle count restarts, nothing starts
      ++s.st_idle_special_refused;
      s.pas.reset();
    } else if (pv != Pv::None && s.pas.ready(now) && neutral && !reentry && dknb_world_safe() && dknb_link_alive()) {
      const bool bash_ok = p2 || a->mStates[12] == 0;  // phase 1: the bash needs the shield (part 12 breaks at armour progress 11)
      const float reach = p2 ? policy::kPassiveReach2 : policy::kPassiveReach1;
      if (bash_ok && dist < reach && front) {
        s.pas.fire(now);
        s.lease = DknLease::Special, s.special_move = p2 ? Move::F2C : Move::F1Shield;
        ++s.st_specials, ++dkn_stage(s).specials;
        s.ini.started(now);
        dkn_native_action(a, p2 ? native::kActAttackShieldL : native::kActAttackShieldH, 0);
        char ex[160];
        std::snprintf(ex, sizeof(ex), "kind=%s move=%s distance=%.0f cooldown_ticks=%u(PROVISIONAL)", pv == Pv::Shield ? "SHIELD_HELD_38" : "NO_SHIELD_75", name(s.special_move), dist,
                      policy::kPassiveCooldownTicks);
        dkn_log("SPECIAL_START", s, a, ex);
        return HOOK_CONTINUE;
      }
      if (s.pas.fired < 1000) {
        char ex[120];
        std::snprintf(ex, sizeof(ex), "kind=%s rejected=%s distance=%.0f", pv == Pv::Shield ? "SHIELD_HELD_38" : "NO_SHIELD_75", !bash_ok ? "NO_SHIELD_LEFT" : !front ? "NOT_FRONT" : "FAR", dist);
        if (now % 30 == 0) dkn_log("SPECIAL_REJECTED", s, a, ex);
      }
    }
  }
  // ---- FIX07: the offensive leap (approved 90 / 10): Link beyond the initiative reach at a distance one native forward leap can close without landing on him.
  // ONE roll per opportunity (debounced, then the episode must close + a cooldown), never over a sequence, a defense, a re-entry or a pending reposition.
  if (k) {
    const float dy = k->current.pos.y - a->current.pos.y;
    const std::int16_t to_link = dkn_angle_to(a, k);
    const int arc = static_cast<std::int16_t>(to_link - a->shape_angle.y);
    if (!p2 && dist > policy::kEngage1 && !s.leap_f1_noted) {
      s.leap_f1_noted = true, ++s.gap.f1_refused;
      dkn_log("LEAP_F1_NOT_AVAILABLE", s, a, "reason=NO_NATIVE_PHASE1_LEAP(executeChaseH_has_no_jump;TNB_JUMP_are_sword_phase_clips) approach=NATIVE_CHASEH_WALK");
    }
    GapView gv;
    const bool evade_hold = leap_hold_after_evade(now, s.evade_land_tick, s.evade_link_close);  // FIX08: the distance the evasion created is not Link's retreat
    gv.phase2 = p2, gv.free = neutral && !def_busy && !hold && now >= s.ini.reentry_until && !s.repo_pending && !evade_hold;
    gv.link = dknb_link_alive() && !dknb_link_broken(), gv.safe = dknb_world_safe() && a->mAcch.ChkGroundHit(), gv.front = (arc < 0 ? -arc : arc) <= policy::kLeapArc;
    gv.dist = dist, gv.dy = dy, gv.now = now;
    const GapOut go = s.gap.decide(gv);
    if (go.g == Gc::Roll) {
      ++s.leap_episode;
      if (go.pick == GcPick::Normal) {
        char nx[200];
        std::snprintf(nx, sizeof(nx), "episode=%llu roll=ONE_PER_OPPORTUNITY result=%s pct=%d/%d distance=%.0f approach=NATIVE initiative=ORDINARY next_roll_after=EPISODE_CLOSE+%u(PROVISIONAL)",
                      static_cast<unsigned long long>(s.leap_episode), name(go.pick), policy::kLeapPct, 100 - policy::kLeapPct, dist, policy::kLeapCooldownTicks);
        dkn_log("LEAP_ROLL", s, a, nx);
      } else {
        const DknGeo g = dkn_geo(a, dkn_dir_to(a, k, false), leap_model(go.leap).dist);
        char gx[400];
        std::snprintf(gx, sizeof(gx), "episode=%llu roll=ONE_PER_OPPORTUNITY result=%s combo=%s leap=%s distance=%.0f angle=%d dy=%.0f predicted_jump=%.0f predicted_landing=%.0f geo=%s "
                      "ground_dy=%.0f/%.0f/%.0f/%.0f", static_cast<unsigned long long>(s.leap_episode), name(go.pick), combo_of(static_cast<unsigned>(go.combo)).name, name(go.leap), dist, arc,
                      dy, static_cast<double>(leap_model(go.leap).dist), static_cast<double>(dist - leap_model(go.leap).dist), name(g.g), static_cast<double>(g.ground[0]),
                      static_cast<double>(g.ground[1]), static_cast<double>(g.ground[2]), static_cast<double>(g.ground[3]));
        if (g.g != Geo::Ok) {  // the roll stays consumed (no reroll): the Darknut approaches normally
          s.gap.refused(), ++s.st_leap_geo_refused;
          dkn_log("LEAP_REFUSED_GEOMETRY", s, a, gx);
        } else {
          dkn_log("LEAP_START", s, a, gx);
          dkn_native_jump(a, leap_mode(go.leap));
          s.lease = DknLease::Leap, s.leap_phase = Lp::Air, s.leap = go.leap, s.leap_combo = go.combo, s.leap_tick = now, s.leap_d0 = dist;
          s.leap_x0 = a->current.pos.x, s.leap_z0 = a->current.pos.z, ++s.st_leap_start;
          s.ini.started(now);
          return HOOK_CONTINUE;
        }
      }
    }
  }
  // ---- priority 7: the OWN initiative (combo 25/25/25/25 or one isolated strike)
  // FIX08-07: (a) the neutral receipt after a defense: a guard clip / knockback / jump never starts an ordinary offensive (DEFENSE -> NEUTRAL -> ORDINARY_INIT); a
  // native attempt is only a decision point in that same neutral state (FIX07 let `from_native` bypass it). (b) Never over Link's running swing that can connect.
  const std::uint64_t receipts0 = s.ngate.receipts;
  const int gate_cause = s.ngate.cause;
  const std::uint64_t gate_since = s.ngate.since;
  const bool gate_ok = s.ngate.ready(now, native::neutral_chase(p2, act, mode));
  if (s.ngate.receipts != receipts0 && s.ngate.receipts <= 80) {  // sampled; the summary counts every receipt
    char gx[200];
    std::snprintf(gx, sizeof(gx), "cause=%s defense_tick=%llu neutral_ticks=%u receipt_tick=%llu next=ORDINARY_INIT_ALLOWED gap_left=%llu", nr_cause(gate_cause),
                  static_cast<unsigned long long>(gate_since), policy::kGuardLeaveTicks, static_cast<unsigned long long>(now),
                  static_cast<unsigned long long>(s.ini.gap_until > now ? s.ini.gap_until - now : 0));
    dkn_log("DEFENSE_NEUTRAL_RECEIPT", s, a, gx);
  }
  const unsigned swing_age = s.swing_seen_tick && now >= s.swing_seen_tick ? static_cast<unsigned>(now - s.swing_seen_tick) : 9999u;
  const bool link_swing = k && swing_threat(dkn_link_cutting(lp) && s.swing_key == s.link_key, front, dist, k->getCutType(), swing_age);
  InitView iv;
  iv.phase2 = p2, iv.busy = dkn_busy(s) || def_busy, iv.safe = dknb_world_safe(), iv.link = k && dknb_link_alive(), iv.front = front, iv.neutral = neutral && gate_ok, iv.dist = dist, iv.now = now;
  iv.defending = hold;
  iv.link_swing = link_swing;
  const std::uint64_t opp0 = s.ini.opportunities;
  const Decision d = s.ini.decide(iv);
  if (d.why == NoInit::Defending && (neutral || from_native || guard_state)) ++s.st_hold_refused;  // an offensive the exchange refused (telemetry)
  if (d.why == NoInit::LinkSwinging) ++s.st_link_swing_refused;
  if (s.ini.opportunities != opp0) ++dkn_stage(s).opportunities;
  if (d.pick != dkn::Pick::None) {
    DknStage& g = dkn_stage(s);
    ++g.decisions;
    if (d.pick == dkn::Pick::Combo) ++g.combo_sel[d.idx & 3];
    char ex[260];
    std::snprintf(ex, sizeof(ex), "result=%s plan=%s cause=%s initiative=ORDINARY idle_ticks=%llu distance=%.0f link_cutting=%d swing_age=%u exchange=OPEN open_reason=%s chain=%u whiffs=%u "
                  "last_swing_age=%llu neutral_receipt=%llu", d.pick == dkn::Pick::Combo ? "COMBO" : "SINGLE", combo_of(d.idx).name, from_native ? "NATIVE_ATTEMPT_TAKEN" : "OWN_INITIATIVE",
                  static_cast<unsigned long long>(s.ini.idle_since ? now - s.ini.idle_since : 0), dist, dkn_link_cutting(lp) ? 1 : 0, swing_age > 999u ? 999u : swing_age, name(hv), s.def.chain,
                  s.def.whiffs, static_cast<unsigned long long>(s.def.last_swing && now >= s.def.last_swing ? now - s.def.last_swing : 0),
                  static_cast<unsigned long long>(s.ngate.last_receipt));
    dkn_log("COMBO_SELECT", s, a, ex);
    if (!dkn_move_begin(s, a, k, d.idx, d.pick == dkn::Pick::Combo ? DknLease::Combo : DknLease::Single, from_native ? "NATIVE_ATTEMPT_TAKEN" : "OWN_INITIATIVE"))
      s.ini.ended(now, p2);
  }
  return HOOK_CONTINUE;
}
// FIX07: the thrown mace. cc_set (after this POST) anchors mSwordSphs / mCps on the flying mace this tick iff mStates[13] == 3 and the sword is not in
// hand (mStates[15] != 1; from TNB_PULL frame 10 they follow the drawn sword). That anchor is what the NEXT damage_check's AT-hit flags were produced with.
void dkn_mace_post(DknSlot& s, daB_TN_c* a) noexcept {
  const bool anchor = a->mStates[13] == 3 && a->mStates[15] != 1;
  if (anchor && !s.mace.live) {
    s.mace.thrown();
    char ex[160];
    std::snprintf(ex, sizeof(ex), "throw_serial=%llu mace_state=3 at=%d spl=%d owner=NATIVE_calcSwordMoveA darknut_posture=NEVER", static_cast<unsigned long long>(s.mace.serial),
                  a->mSwordSphs[0].ChkAtSet() ? 1 : 0, static_cast<int>(a->mSwordSphs[0].GetAtSpl()));
    dkn_log("MACE_THROWN", s, a, ex);
  } else if (!anchor && s.mace.live) {
    s.mace.landed();
    char ex[160];
    std::snprintf(ex, sizeof(ex), "throw_serial=%llu mace_state=%u sword_state=%u resolved=%d reason=%s", static_cast<unsigned long long>(s.mace.serial), static_cast<unsigned>(a->mStates[13]),
                  static_cast<unsigned>(a->mStates[15]), s.mace.resolved == s.mace.serial ? 1 : 0, a->mStates[15] == 1 ? "SWORD_DRAWN_COLLIDERS_ON_SWORD" : "MACE_LANDED");
    dkn_log("MACE_PROJECTILE_END", s, a, ex);
  }
  s.mace_anchor = anchor && (a->mSwordSphs[0].ChkAtSet() || a->mCps.ChkAtSet());
}
// The ONE contact of a throw with Link (damage_check PRE of the next tick, before the native reads the same flags): block +50 / direct +60 (on Link's native
// damage receipt) / parry +0 (Link's Shield Attack in the shared window: orange spark, x1.22 clash, no life damage); never the Darknut's posture.
void dkn_mace_contact(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn;
  if (!s.mace_anchor || !s.mace.serial) return;
  daAlink_c* k = dknb_link();
  if (!k) return;
  bool hit = false, shield = false;
  const void* sph = nullptr;
  auto look = [&](dCcD_GObjInf& x) noexcept {
    if (!x.cCcD_ObjHitInf::ChkAtHit()) return;
    const cCcD_Obj* o = x.cCcD_ObjHitInf::GetAtHitObj();
    for (int i = 0; i < 3; ++i)
      if (o == static_cast<const cCcD_Obj*>(&k->mTgCyls[i])) {
        hit = true, shield = shield || x.ChkAtShieldHit();
        if (!sph) sph = static_cast<const void*>(&x);
      }
  };
  for (int i = 0; i < 4; ++i) look(a->mSwordSphs[i]);
  look(a->mCps);
  if (!hit) return;
  const DknPass pass = dknb_link_pass(k);
  const Mace m = mace_verdict(true, shield, shield && pass.in_window);
  ++s.st_mace_contacts;
  if (!s.mace.resolve(m)) {
    ++s.st_mace_repeat;
    if (s.st_mace_repeat <= 10) dkn_log("MACE_CONTACT_REPEAT", s, a, "same_throw=1 credit=0 reason=ONE_RESOLUTION_PER_THROW");
    return;
  }
  const int pts = mace_link_points(m);
  char ex[260];
  if (m == Mace::Parry) {
    dknb_link_parry_confirmed(s.id, pass.serial);
    pfx_settle(pfx::Kind::LinkOnDarknut, s.id, true);  // the orange spark / high clash the pass previewed is this parry
    dkn_fx_route(s, a, 3, s.mace.serial, "LINK_PARRIES_THROWN_MACE");  // FIX08-06
    std::snprintf(ex, sizeof(ex), "throw_serial=%llu verdict=PARRY link_serial=%llu delta=%.2f link_points=+0 darknut_points=+0 life_damage=0 initiative=NONE", static_cast<unsigned long long>(s.mace.serial),
                  static_cast<unsigned long long>(pass.serial), pass.delta);
  } else if (m == Mace::Block) {
    pfx_settle(pfx::Kind::LinkOnDarknut, s.id, false);
    dknb_link_blocked(s.id, false);
    const DknGain r = dknb_link_gain(pts, true, s.id, s.generation, 0x2000000000000000ull | s.mace.serial);
    std::snprintf(ex, sizeof(ex), "throw_serial=%llu verdict=BLOCK link_points=+%d confirmed=%d link=%d.%03d->%d.%03d saturated=%d darknut_points=+0 life=NATIVE_GUARD", static_cast<unsigned long long>(s.mace.serial),
                  pts, r.confirmed ? 1 : 0, r.before / 1000, r.before % 1000, r.after / 1000, r.after % 1000, r.saturated ? 1 : 0);
  } else {
    pfx_settle(pfx::Kind::LinkOnDarknut, s.id, false);
    s.mace_direct_token = 0x2000000000000000ull | s.mace.serial;
    dknb_direct_offer(dkn_index(&s), s.id, s.generation, s.mace_direct_token, sph);
    std::snprintf(ex, sizeof(ex), "throw_serial=%llu verdict=DIRECT link_points=+%d (credited on Link's native damage receipt) darknut_points=+0 life=NATIVE", static_cast<unsigned long long>(s.mace.serial), pts);
  }
  dkn_log("MACE_CONTACT", s, a, ex);
}
// FIX05: the native transformation's sword draw (executeZakoChangeDemo mode 5). At TNB_PULL frame 10 the native sets mStates[15] = 1 (sword in hand) in
// this action(); cc_set (after it) then closes mCps from mCpsPos, which still holds the last position of the THROWN mace (mStates[13] 3 -> 7), while the
// weapon AT is on since TNB_THROW frame 23: a capsule from the mace's landing point to the hand, for one tick (the remote damage). That tick runs with the
// capsule AT off; the four physical sword spheres keep their legitimate close contacts even on this tick. cc_set re-anchors mCpsPos on the sword;
// the next tick the native setSwordAtBit(1) restores the capsule (the spheres were never disabled).
void dkn_transform_post(DknSlot& s, daB_TN_c* a) noexcept {
  if (a->mActionMode1 != dkn::native::kActChangeDemo) return;
  const bool in_hand = a->mStates[15] == 1, at_on = a->mSwordSphs[0].ChkAtSet() != 0;
  if (s.tf_restore) {
    s.tf_restore = false;
    if (in_hand && !a->mCps.ChkAtSet()) {
      dkn_native_at(a, true, 1);
      ++s.st_tf_restore;
      dkn_log("TRANSFORM_DRAW_AT_RESTORED", s, a, "native=setSwordAtBit(1) capsule=ANCHORED_ON_SWORD");
    }
    return;
  }
  if (s.tf_reanchored || !in_hand || !at_on || a->mActionMode2 != 5) return;
  s.tf_reanchored = s.tf_restore = true;
  ++s.st_tf_reanchor;
  const cXyz stale = a->mCpsPos;
  const float sx = stale.x - a->current.pos.x, sz = stale.z - a->current.pos.z;
  daAlink_c* k = dknb_link();
  float lk = -1.0f;
  if (k) {  // Link's XZ distance to the segment the stale capsule would have swept (stale anchor -> the Darknut), the remote-hit evidence
    const float ax = stale.x, az = stale.z, bx = a->current.pos.x, bz = a->current.pos.z, px = k->current.pos.x, pz = k->current.pos.z;
    const float vx = bx - ax, vz = bz - az, l2 = vx * vx + vz * vz;
    float u = l2 > 1.0f ? ((px - ax) * vx + (pz - az) * vz) / l2 : 0.0f;
    u = u < 0 ? 0 : u > 1 ? 1 : u;
    const float qx = ax + u * vx - px, qz = az + u * vz - pz;
    lk = kk::fsqrt(qx * qx + qz * qz);
  }
  dkn_native_at(a, true, 0, true);
  char ex[240];
  std::snprintf(ex, sizeof(ex), "tnb_pull_frame=%.1f stale_anchor_from_body=%.0f link_to_stale_segment=%.0f link_distance=%.0f mace_state=%u capsule_at=OFF_THIS_TICK_ONLY sword_spheres=NATIVE capsule=REANCHOR",
                static_cast<double>(dkn_frame(a)), static_cast<double>(kk::fsqrt(sx * sx + sz * sz)), static_cast<double>(lk), k ? static_cast<double>(dkn_dist(a, k)) : -1.0,
                static_cast<unsigned>(a->mStates[13]));
  dkn_log("TRANSFORM_SWEEP_REANCHORED", s, a, ex);
}
void on_dkn_action_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp) return;
  if (sp->ph == dkn::Ph::Transform) {
    dkn_mace_post(*sp, a);  // FIX07: the thrown mace as a projectile (read only: where cc_set anchors the weapon colliders this tick)
    dkn_transform_post(*sp, a);
    return;
  }
  if (!dkn_combat(*sp)) return;
  DknSlot& s = *sp;
  using namespace dkn;
  const int act = a->mActionMode1;
  if (act == native::kActChangeDemo || act == native::kActEnding) return;  // the native transformation / DKR01 own the actor
  dkn_native_spl(a);
  const std::uint64_t now = dknb_tick();
  daAlink_c* k = dknb_link();
  // HeadLock: ON only in the second stagger; any other HeadLock (the native phase-1 Shield Attack opening) is taken back before Link can read it
  if (s.brk.st == Bk::StaggerB) {
    if (!a->checkHeadLockFlg()) dkn_native_headlock(a, true);
    if (!s.helm_active && dkn_helm_flying(s, a, k)) {
      s.helm_active = true, s.helm_tick = now, ++s.st_helm_start;
      dkn_log("HELM_START", s, a, "link=PROC_CUT_HEAD target=THIS_ACTOR native=procCutHeadInit");
    }
  } else if (a->checkHeadLockFlg()) {
    dkn_native_headlock(a, false);
    ++s.st_headlock_off;
  }
  // the staggers: one eligible tick, the clip held across the window, then the natural recovery
  if (s.brk.st == Bk::StaggerA) {
    const float end = phase2(s.ph) ? native::kEndTnbDownSippu : native::kEndTnaDamageL;
    if (!dkn_stagger_action(s, a)) {  // the native clip ended early (should not happen while held): recover now
      s.brk.st = Bk::Recover, s.brk.why = BkEnd::Watchdog;
    } else if (s.brk.tick(dkn_md_flying(s, k))) {
      dkn_log("BREAK_TIMEOUT", s, a, "event=RECOVERY_START reason=TIMEOUT_90 fall=0 execution=NONE");
    } else {
      if (s.brk.ext_used && s.brk.ext == 1) dkn_log("BREAK_EXTENDED", s, a, "reason=MORTAL_DRAW_IN_FLIGHT max=30(PROVISIONAL)");
      dkn_native_rate(a, stagger_rate(s.brk.ticks < policy::kBreakTicks ? s.brk.ticks : policy::kBreakTicks - 1, dkn_frame(a), end));
    }
  } else if (s.brk.st == Bk::StaggerB) {
    if (!dkn_stagger_action(s, a)) {
      s.brk.st = Bk::Recover, s.brk.why = BkEnd::Watchdog;
    } else if (s.brk.tick(dkn_helm_flying(s, a, k))) {
      dkn_log("BREAK_TIMEOUT", s, a, "event=RECOVERY_START reason=HELM_WINDOW_OVER fall=0 execution=NONE");
    } else {
      if (s.brk.ext_used && s.brk.ext == 1) dkn_log("BREAK_EXTENDED", s, a, "reason=HELM_SPLITTER_IN_FLIGHT max=30(PROVISIONAL)");
      dkn_native_rate(a, helm_rate_at(s.brk.b_ticks, dkn_frame(a)));  // FIX06: the continuous sway (0.13.14 held frame 12 at 0.02)
    }
  }
  if (s.brk.st == Bk::Recover && s.helm_look.on()) {  // FIX08-01: the recovery after the phase-1 Helm Splitter looks for Link
    if (s.helm_look.neck(now) && !s.neck_on) {
      dkn_native_neck(a, true), s.neck_on = true;
      char nx[160];
      std::snprintf(nx, sizeof(nx), "neck_flag=ON(native calcNeckAngle) body_error=%d clip_frame=%.1f since_contact=%llu", k ? static_cast<int>(static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y)) : 0,
                    static_cast<double>(dkn_frame(a)), static_cast<unsigned long long>(now - s.helm_look.contact));
      dkn_log("HELM_LOOK_FOLLOW", s, a, nx);
    }
    if (k && dkn_stagger_action(s, a) && s.helm_look.turn(dkn_frame(a)))  // the body: the native ChaseH turn rate, never while the hit demands the original facing
      dkn_native_motion(a, 0.0f, turn_step(a->shape_angle.y, dkn_angle_to(a, k)));
  }
  if (s.brk.st == Bk::Recover) {
    const bool done = !dkn_stagger_action(s, a);
    if (!done) dkn_native_rate(a, kRecoverRate);
    if (s.brk.recover(done)) {
      if (!done) dkn_native_neutral(a, phase2(s.ph));
      dkn_native_headlock(a, false);
      char ex[160];
      std::snprintf(ex, sizeof(ex), "event=RECOVERED reason=%s posture=0/%d forced_guard=NONE reentry_ticks=%u(PROVISIONAL) auto_kick=BLOCKED", name(s.brk.why), dkn_max(s),
                    policy::kReentryTicks);
      dkn_back_to_combat(s, a, "GUARD_RECOVER", ex);
      if (s.helm_look.on()) {  // FIX08-01: the tail: the neck keeps following while the native ChaseH turns the body
        s.helm_look.recovered_at(now);
        char rx[160];
        std::snprintf(rx, sizeof(rx), "body_error_start=%d body_error_now=%d neck_angle=%d tail=%u(PROVISIONAL)", static_cast<int>(s.helm_err0),
                      k ? static_cast<int>(static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y)) : 0, static_cast<int>(a->mNeckAngle), policy::kHelmLookTailTicks);
        dkn_log("HELM_LOOK_RECOVERED", s, a, rx);
      }
    }
  }
  if (s.brk.st == Bk::Combat && s.helm_look.on() && (!s.helm_look.tail(now) || s.lease != DknLease::None || !k))  // FIX08-01: the look ends (never stale)
    dkn_helm_look_end(s, a, k, !k ? "NO_TARGET" : s.lease != DknLease::None ? "OWN_MOVE_STARTED" : "TAIL_OVER");
  // regeneration: only in plain combat (never during a stagger, the transformation, the fall, the Finish, the getup, the rise or death)
  if (s.brk.st == Bk::Combat) {
    const int rem = dkn_remaining(s, a), mx = dkn_life_max(s);
    const unsigned b = band(rem, mx);
    if (s.post.regen(regen_rate(phase2(s.ph), rem, mx))) ++s.st_regen_ticks;
    if (b != s.last_band) {
      s.last_band = b;
      char ex[96];
      std::snprintf(ex, sizeof(ex), "event=BAND band=%u rate_pts_s=%d remaining=%d/%d", b, regen_rate(phase2(s.ph), rem, mx), rem, mx);
      dkn_log("REGEN", s, a, ex);
    }
  }
  // FIX06: the reposition jump (native executeChaseL 6 -> 7 back, 8 -> 9 forward). The landing attack the native writes at the end of mode 7 / 8
  // (ATTACKSHIELDL 0 / ATTACKL 0 / 10) never runs: the forward jump gets its native landing (initChaseL(9): TNB_JUMP_F_2) instead, the back jump (its
  // landing clip already played) the neutral chase. Then: no initiative before kRepoSettleTicks (and the 45-tick re-entry of the rise / getup).
  if (s.lease == DknLease::Reposition) {
    const int m1 = a->mActionMode1, m2 = a->mActionMode2;
    const bool landing_attack = native::native_attack_entry(m1, m2);
    const bool jumping = m1 == native::kActChaseL && native::jump_mode(m2);
    if (landing_attack && !s.repo_back) {
      dkn_native_jump_land(a);
      ++s.st_repo_land_attack;
      dkn_log("REPOSITION_LANDING_ATTACK_TAKEN_BACK", s, a, "native_entry=ATTACK landing=TNB_JUMP_F_2_mode9 attack_executed=0");
    } else if (!jumping || landing_attack || now > s.repo_tick + policy::kRepoWatchdogTicks) {  // landed (mode 1 / any other action), back landing attack, watchdog
      if (landing_attack) ++s.st_repo_land_attack;
      const bool watchdog = jumping && now > s.repo_tick + policy::kRepoWatchdogTicks;
      if (landing_attack || watchdog) dkn_native_neutral(a, true);
      s.lease = DknLease::None;
      s.ini.settle(now + policy::kRepoSettleTicks);
      char rx[220];
      std::snprintf(rx, sizeof(rx), "jump=%s distance_start=%.0f distance_end=%.0f ticks=%llu landing_attack_taken_back=%d watchdog=%d next_initiative_not_before=%llu",
                    s.repo_back ? "BACK" : "FORWARD", static_cast<double>(s.repo_d0), k ? static_cast<double>(dkn_dist(a, k)) : -1.0,
                    static_cast<unsigned long long>(now - s.repo_tick), landing_attack ? 1 : 0, watchdog ? 1 : 0, static_cast<unsigned long long>(s.ini.reentry_until));
      dkn_log("REPOSITION_LANDED", s, a, rx, watchdog);
    }
  }
  // FIX07: the offensive leap. Air: the native landing attack (mode 8 / 17 end) becomes the native landing (initChaseL(9): TNB_JUMP_F_2), never an attack.
  // Ground: any native attack entry / native leap the chase writes meanwhile is taken back to the neutral chase (the lease is ours; the combo comes from us).
  if (s.lease == DknLease::Leap) {
    const int m1 = a->mActionMode1, m2 = a->mActionMode2;
    const bool entry = native::native_attack_entry(m1, m2);
    if (s.leap_phase == Lp::Air && entry) {
      dkn_native_jump_land(a);
      ++s.st_leap_land_attack;
      dkn_log("LEAP_LANDING_ATTACK_TAKEN_BACK", s, a, "native_entry=ATTACK landing=TNB_JUMP_F_2_mode9 attack_executed=0");
    } else if (entry || native::attack_action(m1) || (s.leap_phase == Lp::Ground && native::native_leap(m1, m2))) {
      dkn_native_neutral(a, true);
      ++s.st_leap_native_cut;
      if (s.st_leap_native_cut <= 30) {
        char nx[120];
        std::snprintf(nx, sizeof(nx), "native_act=%d native_mode=%d phase=%s executed=0 back=NEUTRAL_CHASE", m1, m2, name(s.leap_phase));
        dkn_log("LEAP_NATIVE_ENTRY_SUPPRESSED", s, a, nx);
      }
    }
  }
  // FIX08: the evasion. Its native landing (12 / 15 / 7) ends in a native attack entry when Link is attackable: taken back to the neutral chase (never an attack
  // out of an evasion); the lease ends there, the next ordinary initiative waits for the neutral receipt.
  if (s.lease == DknLease::Evade) {
    const int m1 = a->mActionMode1, m2 = a->mActionMode2;
    if (native::native_attack_entry(m1, m2) || native::attack_action(m1) || native::native_leap(m1, m2)) {
      dkn_native_neutral(a, true);
      ++s.evade.landing_attacks;
      dkn_evade_end(s, a, k, "LANDING_ATTACK_TAKEN_BACK", false);
    }
  }
  // FIX02/FIX03: the offensive belongs to the controller only. An attack action without a lease in the POST is either a native ENTRY the native AI
  // wrote during this action() (setActionMode(ATTACK*, 0/10/11/20): nothing of it has run, it would start at the NEXT action) = a suppressed
  // ATTEMPT, or a running attack mode = a real integration failure (UNAUTHORIZED, executed). Both are taken back to the neutral chase (AT off) now.
  if (s.lease == DknLease::None && native::attack_action(a->mActionMode1) && s.brk.st == Bk::Combat) {
    const bool entry = native::native_attack_entry(a->mActionMode1, a->mActionMode2);
    char ex[160];
    if (entry) {
      ++s.st_entry_post, ++s.st_suppressed, ++dkn_stage(s).suppressed;
      s.native_attempt = true;
      if (s.st_entry_post <= 40 || now - s.last_suppress_log >= 300) {
        s.last_suppress_log = now;
        std::snprintf(ex, sizeof(ex), "native_act=%d native_mode=%d executed=0 total=%llu back=NEUTRAL_CHASE authority=OWN_CONTROLLER", a->mActionMode1, a->mActionMode2,
                      static_cast<unsigned long long>(s.st_entry_post));
        dkn_log("NATIVE_ENTRY_SUPPRESSED", s, a, ex);
      }
    } else {
      ++s.st_unauthorized;
      std::snprintf(ex, sizeof(ex), "native_act=%d native_mode=%d executed=1 total=%llu back=NEUTRAL_CHASE", a->mActionMode1, a->mActionMode2, static_cast<unsigned long long>(s.st_unauthorized));
      dkn_log("UNAUTHORIZED_NATIVE_ATTACK", s, a, ex, true);
    }
    dkn_native_neutral(a, phase2(s.ph));
  }
  dkn_at_authority(s, a);
}

// ---------------------------------------------------------------- hooks: damage_check
// Link's colliders on THIS Darknut's hurt spheres in this collision pass (pointer identity only: never a stale object dereferenced).
void dkn_tg_scan(DknSlot& s, daB_TN_c* a, daAlink_c* k) noexcept {
  s.dc_sa = s.dc_sword = false;
  if (!k) return;
  auto look = [&](dCcD_GObjInf& x) noexcept {
    if (!x.cCcD_ObjHitInf::ChkTgHit()) return;
    const cCcD_Obj* o = x.cCcD_ObjHitInf::GetTgHitObj();
    if (o == static_cast<const cCcD_Obj*>(&k->mGuardAtCps)) s.dc_sa = true;
    for (int i = 0; i < 3; ++i)
      if (o == static_cast<const cCcD_Obj*>(&k->mAtCps[i])) s.dc_sword = true;
  };
  for (int i = 0; i < 3; ++i) look(a->mSphA[i]), look(a->mSphB[i]);
  look(a->mSphC);
}
HookAction on_dkn_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (sp && sp->ph == dkn::Ph::Transform) {  // FIX07: the thrown mace's one contact with Link (projectile phase of ACT_CHANGEDEMO only)
    dkn_mace_contact(*sp, a);
    return HOOK_CONTINUE;
  }
  if (!sp || !dkn_combat(*sp)) return HOOK_CONTINUE;
  DknSlot& s = *sp;
  s.pre_act1 = a->mActionMode1, s.pre_act2 = a->mActionMode2, s.sd_seen = false;
  pfx_dkn_exec_begin(a, s.id);  // FIX04: this damage_check's own shield spark / clash (setShieldEffect) may be a previewed parry contact
  if (dkn::phase2(s.ph)) dkn_native_threshold(a, dkn::policy::kRes2Max);
  daAlink_c* k = dknb_link();
  dkn_tg_scan(s, a, k);
  dkn_link_contact(s, a, k);
  return HOOK_CONTINUE;
}
void dkn_damage_post_body(void* args) noexcept;
void on_dkn_damage_post(ModContext*, void* args, void*, void*) noexcept {
  dkn_damage_post_body(args);
  pfx_dkn_exec_end();  // FIX04: the Darknut's damage_check is over: no later mark / clash belongs to it
}
void dkn_damage_post_body(void* args) noexcept {
  if (!dn.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return;
  DknSlot& s = *sp;
  using namespace dkn;
  const int act = a->mActionMode1, mode = a->mActionMode2;
  if (act == native::kActChangeDemo || act == native::kActEnding || s.sd_seen) return;  // damage resolved by the setDamage hooks
  const bool changed = act != s.pre_act1 || mode != s.pre_act2;
  const std::uint64_t now = dknb_tick();
  // ---- the Shield Attack authority: outside the ONE legitimate route (a new Shield Attack in the first stagger of phases 2-3) it changes nothing
  if (s.dc_sa && !s.dc_sword) {  // (the preparation never depends on WHICH hurt sphere took the Shield Attack: mSphB has no native Shield Attack branch)
    const bool sa_new = s.sa_serial != s.sa_used && s.sa_tick >= s.brk.start && s.brk.st == Bk::StaggerA;
    const Sa v = sa_verdict(phase2(s.ph), s.brk.st, sa_new);
    if (v == Sa::HelmPrep && s.brk.helm_prep(now)) {
      s.sa_used = s.sa_serial, s.md_active = false, s.md_rcpt.clear();
      ++s.st_helm_prep, ++dkn_stage(s).helm_preps;
      s.st_f1_helm_prep += !phase2(s.ph);
      // FIX03: TNB_DAMAGE_R (head down, sword low), never the TNB_DOWN guard knockback. FIX06: phase 1 plays its own native reaction TNA_DAMAGE_R
      // (executeDamageH mode 1's clip) in executeDamageH's hold mode 2; both sway around their head-down centre (helm_frame), never held still.
      const HelmClip& hc = helm_clip(phase2(s.ph));
      dkn_native_action(a, hc.act, hc.mode);
      dkn_native_clip(a, hc.bck, kHelmMorph, kHelmStart, 1.0f);
      dkn_native_stagger_hold(a, false);
      dkn_native_headlock(a, true);
      char ex[220];
      std::snprintf(ex, sizeof(ex), "link_sa_serial=%llu age_in_break=%llu clip=%s centre=12 sway=+-%.1f/%.0f_ticks (continuous, never held) headlock=ON window=%u(PROVISIONAL)",
                    static_cast<unsigned long long>(s.sa_serial), static_cast<unsigned long long>(now - s.brk.start), phase2(s.ph) ? "TNB_DAMAGE_R_0x18" : "TNA_DAMAGE_R_0x0B",
                    static_cast<double>(kSwayAmp), static_cast<double>(kSwayPeriod), policy::kHelmWindowTicks);
      dkn_log("STAGGER_B", s, a, ex);
      dkn_log("HELM_PREP", s, a, "route=SHIELD_ATTACK_THEN_HELM_SPLITTER");
      return;
    }
    if (!changed) return;  // the native ignored it (no forced guard): nothing to take back
    ++s.st_sa_rejected, ++dkn_stage(s).sa_rejected;
    ++s.st_reverted;
    dkn_restore(s, a, s.pre_act1, s.pre_act2);  // the native ACT_GUARDL/GUARDH the Shield Attack forced is taken back: combo, guard, recovery untouched
    if (s.brk.st == Bk::StaggerB) dkn_native_headlock(a, true);
    if (s.st_sa_rejected <= 60 || s.st_sa_rejected % 20 == 0) {
      char ex[180];
      std::snprintf(ex, sizeof(ex), "verdict=%s forced_act=%d forced_mode=%d kept_act=%d kept_mode=%d lease=%s link_sa_serial=%llu posture_gain=0", name(v), act, mode, s.pre_act1, s.pre_act2,
                    dkn_lease_name(s.lease), static_cast<unsigned long long>(s.sa_serial));
      dkn_log("SHIELD_ATTACK_REJECTED", s, a, ex);
    }
    return;
  }
  if (!changed) return;
  // Link's attack met the Darknut's defence: the native guard (blocked), or the native ACT_YOROKE of a strong cut on a guarding phase-2 Darknut,
  // which becomes a plain block (only the posture break may stagger him).
  const bool yoroke = act == native::kActYoroke && !(s.brk.active());
  const bool guard = act == native::kActGuardL || act == native::kActGuardH;
  if (act == native::kActDamageH || act == native::kActDamageL) {  // a reaction without damage (hookshot pull): no ordinary stagger, no broken stagger lost
    ++s.st_reverted;
    dkn_restore(s, a, s.pre_act1, s.pre_act2);
    return;
  }
  if (!yoroke && !guard) return;
  if (yoroke) ++s.st_yoroke_blocked;
  if (s.dc_sword) {
    s.def.note_contact(now);
    // FIX05: the contact belongs to Link's latest SWING (swing_key: a roll / step right after the cut never steals it). A swing the chain had not registered
    // (begun beyond the tracking reach, outside the front arc) is registered at this real contact, without a verdict: every defended contact feeds the chain.
    const std::uint64_t ck = s.swing_key ? s.swing_key : s.link_key;
    daAlink_c* kc = dknb_link();
    const float cd = kc ? dkn_dist(a, kc) : -1.0f;
    const bool late = s.def.register_at_contact(now, ck);
    if (late) ++s.st_late_registered;
    if (s.def.on_contact(ck)) {  // FIX04: a REAL defended contact of the current swing: ONE step of Link's chain (never the parried swing)
      if ((late && s.st_late_registered <= 60) || s.def.contacts[0] + s.def.contacts[1] + s.def.contacts[2] + s.def.contacts[3] + s.def.contacts[4] + s.def.contacts[5] <= 60) {
        char cx[160];
        std::snprintf(cx, sizeof(cx), "chain=%u next_ordinal=%u swing_key=%llu distance=%.0f registered=%s", s.def.chain, s.def.ordinal(), static_cast<unsigned long long>(ck), cd,
                      late ? "AT_CONTACT" : "AT_SWING");
        dkn_log("CHAIN_CONTACT", s, a, cx);
      }
    }
    if (s.link_key != s.blocked_key) {  // ONE +2/+3 per Link sword action (procedure + cut type + cut count), never per frame, never a Shield Attack
      s.blocked_key = s.link_key;
      ++s.st_blocked_by_dk;
      dkn_posture_gain(s, a, phase2(s.ph) ? policy::kGainBlocked2 : policy::kGainBlocked1, yoroke ? "LINK_BLOCKED_STRONG_CUT" : "LINK_BLOCKED");
    }
  }
  if (s.lease != DknLease::None || s.brk.active() || s.seq.owns()) {  // a block never cuts an owned move, a special, a stagger or (FIX06) a sequence
    s.st_seq_blocks += s.seq.owns() && s.dc_sword;
    ++s.st_reverted;
    dkn_restore(s, a, s.pre_act1, s.pre_act2);
    if (s.brk.st == Bk::StaggerB) dkn_native_headlock(a, true);
  } else {
    if (yoroke) dkn_native_guard_enter(a, dkn_guard_bck(s, dknb_link()), dkn_guard_facing(a, dknb_link()));  // FIX02: same safe entry
    s.guard_tick = now;
    s.ngate.defense(now, 2);  // FIX08: an ordinary block: the next ordinary initiative passes through the neutral chase
    if (!s.def.window_open(now) && !(s.def.parried && now <= s.def.parry_tick + 2)) ++s.st_ordinary_blocks;
  }
}

// ---------------------------------------------------------------- hooks: setDamage (the native damage commit)
HookAction on_dkn_setdamage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return HOOK_CONTINUE;
  sp->sd_nbp = a->mNextBreakPart, sp->sd_acc = a->field_0x6fc, sp->sd_act1 = a->mActionMode1, sp->sd_act2 = a->mActionMode2, sp->sd_seen = true;
  daAlink_c* k = dknb_link();
  sp->sd_cut = k ? k->getCutType() : -1;
  return HOOK_CONTINUE;
}
void on_dkn_setdamage_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp) || !sp->sd_seen) return;
  DknSlot& s = *sp;
  using namespace dkn;
  const std::uint64_t now = dknb_tick();
  daAlink_c* k = dknb_link();
  const cCcD_Obj* o = a->mAtInfo.mpCollider;
  bool link_sword = false;
  for (int i = 0; i < 3 && k; ++i) link_sword = link_sword || o == static_cast<const cCcD_Obj*>(&k->mAtCps[i]);
  const bool sword = o && const_cast<cCcD_Obj*>(o)->ChkAtType(AT_TYPE_NORMAL_SWORD);
  const bool iron = o && const_cast<cCcD_Obj*>(o)->ChkAtType(AT_TYPE_IRON_BALL);
  const bool head_jump = k && k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP;
  if (s.sd_act1 < native::kActChangeDemo) {  // ---- phase 1: the 600 HP reserve (FIX04) and the armour
    int dmg = 100 - a->health;
    if (dmg < 0) dmg = 0;
    // FIX06: Link's REAL Mortal Draw (its receipt of this actor / generation, first stagger) or Helm Splitter (HEAD_JUMP, second stagger) on the broken
    // phase-1 Darknut: the ONE punishment of this break, exactly kF1ExecDamage HP TOTAL (the native damage of that hit is replaced, never added).
    const bool md1 = kk::mortal_draw_cut(s.sd_cut) && link_sword && s.md_rcpt.valid(s.generation, now);
    const bool helm1 = s.sd_cut == daPy_py_c::CUT_TYPE_HEAD_JUMP && link_sword;
    const bool f1x = exec_verdict_f1(false, s.brk.st, md1, helm1);
    const int hp_before = s.hp1;
    if (f1x) {
      s.hp1 = f1_exec_hp(s.hp1);
    } else {
      s.hp1 -= dmg;
      if (s.hp1 < 0) s.hp1 = 0;
    }
    ++s.st_hp1;
    if (f1x) {
      ++(md1 ? s.st_f1_md_contact : s.st_f1_helm_contact);
      s.md_rcpt.clear(), s.md_active = s.helm_active = false;
      char fx[260];
      std::snprintf(fx, sizeof(fx), "route=%s target_damage=%d effective_damage=%d saturated=%d native_damage_replaced=%d total_damage=%d hp1=%d->%d stage=%s fall=0 dkr=0 next=%s",
                    md1 ? "MORTAL_DRAW_F1" : "SHIELD_ATTACK_HELM_SPLITTER_F1", policy::kF1ExecDamage, hp_before - s.hp1, hp_before - s.hp1 < policy::kF1ExecDamage ? 1 : 0, dmg,
                    hp_before - s.hp1, hp_before, s.hp1, name(s.brk.st), s.hp1 > 0 ? "NATURAL_RECOVERY" : "TRANSFORMATION_PRIORITY");
      dkn_log("F1_EXECUTION_CONTACT", s, a, fx);
    }
    const int prog = armor_progress(s.hp1);
    const int broken = dkn_broken_parts(a);
    const int call = armor_call(prog, broken);
    char ex[200];
    std::snprintf(ex, sizeof(ex), "damage=%d source=%s native_nbp_added=%d armor_progress=%d broken_before=%d in_break=%d", dmg, sword ? "SWORD" : iron ? "IRON_BALL" : "OTHER",
                  a->mNextBreakPart - s.sd_nbp, prog, broken, s.brk.vulnerable() ? 1 : 0);
    if (s.hp1 > 0) {
      if (call >= 0) ++s.st_armor_calls;
      dkn_native_armor(a, call, head_jump, prog > s.sd_nbp ? prog : s.sd_nbp);  // never the native +1/+2: our progress, never below what really broke
      if (call >= 0) {
        char ax[96];
        std::snprintf(ax, sizeof(ax), "progress=%d parts_broken=%d hp1=%d/%d", a->mNextBreakPart, dkn_broken_parts(a), s.hp1, dkn::policy::kHp1Max);
        dkn_log("ARMOR_PROGRESS", s, a, ax);
      }
    }
    dkn_log("HP1_DAMAGE", s, a, ex);
    if (s.hp1 <= 0) {  // priority 2: the reserve is exhausted: the native mType-1 transformation (executeZakoChangeDemo), over everything (a stagger included)
      if (dkn_owned_move(s)) dkn_move_end(s, a, "PHASE_CHANGE", true);
      dkn_seq_end(s, a, SqEnd::Phase, "HP1_ZERO");  // FIX06: nothing of a sequence survives the transformation
      s.def.cancel();
      s.lease = DknLease::None;
      s.md_rcpt.clear(), s.md_active = s.helm_active = false;
      s.brk.abort(BkEnd::Phase);
      dkn_native_armor(a, -1, false, 12);
      dkn_helm_look_end(s, a, k, "HP1_ZERO");  // FIX08
      dkn_native_action(a, native::kActChangeDemo, 0);
      s.ph = Ph::Transform;
      dkn_log("PHASE_TRANSITION", s, a, "event=ACT_CHANGEDEMO executeZakoChangeDemo=1 armour=NATIVE_LAST_PARTS mace_throw=NATIVE sword_draw=NATIVE");
      return;
    }
    if (f1x && s.brk.exec_f1()) {  // FIX06: HP left: the vulnerability ends, the stagger clip finishes as the natural recovery (GUARD_RECOVER: posture 0/200)
      if (a->mActionMode1 != s.sd_act1 || a->mActionMode2 != s.sd_act2) ++s.st_reverted, dkn_restore(s, a, s.sd_act1, s.sd_act2);  // the native ACT_DAMAGEH reaction taken back
      dkn_native_headlock(a, false);
      dkn_log("F1_EXECUTION_RECOVERY", s, a, "event=RECOVERY_START reason=EXECUTION one_punishment_per_break=1 headlock=OFF");
      if (helm1) {  // FIX08-01: the non-lethal phase-1 Helm Splitter: the recovery looks for Link (head first after the hit reads, then the body; Mortal Draw untouched)
        s.helm_look.begin(now);
        s.helm_err0 = k ? static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y) : 0;
        char hx[260];
        std::snprintf(hx, sizeof(hx), "route=HELM_SPLITTER_F1 body_error=%d link_distance=%.0f clip_frame=%.1f neck_flag=%d neck_angle=%d look_after=%u body_turn_from_frame=%.0f turn=native_ChaseH(4,0x400,16)",
                      static_cast<int>(s.helm_err0), k ? static_cast<double>(dkn_dist(a, k)) : -1.0, static_cast<double>(dkn_frame(a)), a->mUpdateNeckAngle ? 1 : 0,
                      static_cast<int>(a->mNeckAngle), policy::kHelmLookDelayTicks, static_cast<double>(policy::kHelmTurnFromFrame));
        dkn_log("HELM_LOOK_START", s, a, hx);
      }
      return;
    }
  } else {  // ---- phases 2 / 3: the native accumulator against 480
    ++s.st_hp2;
    const bool lethal = a->mActionMode1 == native::kActEnding;
    const bool md = kk::mortal_draw_cut(s.sd_cut) && link_sword && s.md_rcpt.valid(s.generation, now);
    const bool helm = s.sd_cut == daPy_py_c::CUT_TYPE_HEAD_JUMP && link_sword && s.brk.st == Bk::StaggerB;
    const Ex ex_v = exec_verdict(phase2(s.ph), s.brk.st, md, helm, lethal);
    char ex[200];
    std::snprintf(ex, sizeof(ex), "added=%d acc=%d/480 remaining=%d source=%s cut=%d lethal=%d execution=%s", a->field_0x6fc - s.sd_acc, a->field_0x6fc, res2_remaining(a->field_0x6fc),
                  sword ? "SWORD" : iron ? "IRON_BALL" : "OTHER", s.sd_cut, lethal ? 1 : 0, name(ex_v));
    dkn_log("HP2_DAMAGE", s, a, ex);
    if (ex_v != Ex::None) {
      ++(md ? s.st_md_contact : s.st_helm_contact);
      dkn_log(md ? "MORTAL_CONTACT" : "HELM_CONTACT", s, a, md ? "native_cut=MORTAL_DRAW receipt=THIS_ACTOR_GENERATION contact=REAL" : "native_cut=HEAD_JUMP after=NEW_SHIELD_ATTACK contact=REAL");
      s.md_rcpt.clear(), s.md_active = s.helm_active = false;
    }
    if (lethal) {  // priority 2/3: the real defeat (the resistance reached 480), whatever the stagger or the execution: DKR01 (F2) / the native death (F3)
      if (ex_v == Ex::Lethal) ++s.st_exec_lethal, dkn_log("EXECUTION_LETHAL_PRIORITY", s, a, "execution_contact=1 resistance=0 outcome=REAL_DEFEAT getup=IMPOSSIBLE");
      if (dkn_owned_move(s)) dkn_move_end(s, a, "DEFEAT", true);
      dkn_seq_end(s, a, SqEnd::Defeat, "RESISTANCE_480");  // FIX06: the defeat has priority over a sequence
      s.repo_pending = false;
      s.lease = DknLease::None, s.def.cancel();
      s.brk.abort(BkEnd::Death);
      return;
    }
    if (ex_v == Ex::Fall) {  // priority 3/5: the confirmed execution with resistance left: the native fall, owned by DKR01 (Finish or the 6 s getup)
      if (dkr_exec_request(a)) {
        s.brk.exec();
        s.exec = true, s.getups_seen = dkr_view(s.id).getups;
        ++s.st_exec, ++dkn_stage(s).executions;
        dkn_native_exec_fall(a);
        char fx[200];
        std::snprintf(fx, sizeof(fx), "route=%s resistance_left=%d/480 clip=TN2B_DIE owner=DKR01 down_ticks=180_from_fall_end finish=%s", md ? "MORTAL_DRAW" : "SHIELD_ATTACK_HELM_SPLITTER",
                      res2_remaining(a->field_0x6fc), s.ph == Ph::Light ? "FIRST_DEFEAT_FIX02_TO_F3" : "DEFINITIVE_DEATH");
        dkn_log("EXECUTION_FALL", s, a, fx);
        return;
      }
      dkn_log("EXECUTION_REFUSED", s, a, "reason=DKR01_REFUSED (slot / clip / body owned) fall=0 stagger_continues=1", true);
    }
  }
  if (s.lease == DknLease::Evade) ++s.evade.hits_taken, s.evade_hit = true;  // FIX08: a real hit during the evasion keeps its damage / posture (no invulnerability)
  // Table A for an effective hit outside a break; inside a break the hit is ordinary native damage (no cap, no posture: the posture is full)
  if (s.brk.vulnerable()) {
    ++s.st_punish;
    char ex[64];
    std::snprintf(ex, sizeof(ex), "hit_in_break=%llu cap=NONE", static_cast<unsigned long long>(s.st_punish));
    dkn_log("PUNISH_HIT", s, a, ex);
  } else if (sword) {
    s.def.note_contact(now);
    s.st_seq_hits_taken += s.seq.owns();  // FIX06: a hit inside a sequence keeps its real damage and posture (counted, never suppressed)
    dkn_posture_gain(s, a, phase2(s.ph) ? policy::kGainSwordHp2 : policy::kGainSwordHp1, "LINK_SWORD_HP");
  } else if (iron) {
    dkn_posture_gain(s, a, phase2(s.ph) ? policy::kGainIron2 : policy::kGainIron1, "LINK_IRON_BALL");
  }
  // no ordinary stagger: the native ACT_DAMAGEH / ACT_DAMAGEL of this hit is taken back (hit mark, sounds, invulnerability and armour kept)
  if (a->mActionMode1 != s.sd_act1 || a->mActionMode2 != s.sd_act2) {
    ++s.st_reverted;
    dkn_restore(s, a, s.sd_act1, s.sd_act2);
    if (s.brk.st == Bk::StaggerB) dkn_native_headlock(a, true);
  }
}

// ---------------------------------------------------------------- hooks: the attacks (move director and specials)
HookAction on_dkn_attack_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return HOOK_CONTINUE;
  // The native attack entries are taken back in the action PRE, before damage_check and before this call: an unowned entry here would mean a native
  // start inside the same action() (none exists: setActionMode only changes the NEXT dispatch). Counted, never fought inside the running attack.
  if (sp->lease == DknLease::None && dkn::native::native_attack_entry(a->mActionMode1, a->mActionMode2)) ++sp->st_chain_cut;
  return HOOK_CONTINUE;
}
void on_dkn_attack_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return;
  if (dkn_owned_move(*sp)) dkn_combo_post(*sp, a, dknb_link());
  else if (sp->lease == DknLease::Special) dkn_special_post(*sp, a);
}

// ---------------------------------------------------------------- Link: the Mortal Draw request and the rebound verdict (callbacks on existing targets)
// B on a broken Darknut in its FIRST stagger (FIX06: phases 1, 2 and 3): the native checkCutAction has just chosen Link's ordinary sword cut. If that Darknut stands in
// front of Link and in reach, the native Mortal Draw (the Chilfos writer: procCutFinishInit(MORTAL_DRAW_B), its animation, AT, sound, cut type) replaces it
// in the same call; an explicit receipt of THIS actor/generation is issued. The fall only comes from its REAL contact (setDamage POST).
void on_dkn_cut_action_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!dn.enabled || !args || !ret || *static_cast<int*>(ret) == 0) return;
  auto* k = mods::arg<daAlink_c*>(args, 0);
  if (!k || k != dknb_link() || k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)) return;
  const int proc = static_cast<int>(k->mProcID), cut = k->getCutType();
  if ((proc != daAlink_c::PROC_CUT_NORMAL && proc != daAlink_c::PROC_CUT_FINISH) || kk::native_instant_cut(cut)) return;  // a hidden skill / a finisher (or the Chilfos took it)
  DknSlot* best = nullptr;
  daB_TN_c* best_a = nullptr;
  float best_d = dkn::policy::kMdReach + 1.0f;
  for (DknSlot& s : dn.s) {
    if (!s.on || !dkn_combat(s) || s.brk.st != dkn::Bk::StaggerA || s.md_active) continue;  // FIX06: phases 1, 2 and 3 (phase 1: 100 HP total, no fall)
    fopAc_ac_c* p = nullptr;
    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;
    auto* a = static_cast<daB_TN_c*>(p);
    const float d = dkn_dist(a, k);
    const int to = static_cast<std::int16_t>(cM_atan2s(a->current.pos.x - k->current.pos.x, a->current.pos.z - k->current.pos.z) - k->shape_angle.y);
    if (d <= dkn::policy::kMdReach && (to < 0 ? -to : to) <= dkn::policy::kMdArc && d < best_d) best = &s, best_a = a, best_d = d;
  }
  if (!best) return;
  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);
  const bool started = static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH && kk::mortal_draw_cut(k->getCutType());
  char ex[160];
  std::snprintf(ex, sizeof(ex), "native_proc=%d native_cut=%d dist=%.0f age_in_break=%llu result=%s", proc, cut, best_d, static_cast<unsigned long long>(dknb_tick() - best->brk.start),
                started ? "MORTAL_DRAW_NATIVE" : "NATIVE_REFUSED");
  if (started) best->md_active = true, best->md_tick = dknb_tick(), best->md_rcpt.issue(best->generation, dknb_tick()), ++best->st_md_start;
  dkn_log(started ? "MORTAL_START" : "MORTAL_REFUSED", *best, best_a, ex, !started);
}
// Link's sword against THIS Darknut's spheres (FIX03: every phase; phase 1 = the shield mSphC too): TRUE (the native rebound) is kept ONLY inside an
// armed parry window; any other TRUE (a native mCutFlag guard, the phase-1 shield TgShield, a stray TgShield) becomes FALSE: an ordinary block never
// rebounds. FIX04: the window is bound to the swing it was prepared for, Link's procedure must be one whose rebound is PROC_CUT_REVERSE (otherwise no
// parry: FALSE, an ordinary block), and the credit + counter wait for that PROC_CUT_REVERSE to be read in the action PRE (no gain in this callback).
bool dkn_link_reverse_route(const daAlink_c* k) noexcept {  // changeCutReverseProc -> procCutReverseInit(PROC_CUT_REVERSE): ground cuts and the jump-cut landing
  const int proc = k ? static_cast<int>(k->mProcID) : -1;
  return proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH || proc == daAlink_c::PROC_CUT_JUMP_LAND;
}
void on_dkn_cut_reverse_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!dn.enabled || !args || !ret) return;
  auto* r = static_cast<BOOL*>(ret);
  if (!*r) return;
  auto* o = mods::arg<dCcD_GObjInf*>(args, 1);
  if (!o) return;
  const cCcD_Obj* hit = o->cCcD_ObjHitInf::GetAtHitObj();
  if (!hit) return;
  const std::uint64_t now = dknb_tick();
  daAlink_c* k = mods::arg<daAlink_c*>(args, 0);
  for (DknSlot& s : dn.s) {
    if (!s.on || !dkn_combat(s)) continue;
    fopAc_ac_c* p = nullptr;
    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;
    auto* a = static_cast<daB_TN_c*>(p);
    bool here = hit == static_cast<const cCcD_Obj*>(&a->mSphC);
    for (int i = 0; i < 3 && !here; ++i) here = hit == static_cast<const cCcD_Obj*>(&a->mSphA[i]) || hit == static_cast<const cCcD_Obj*>(&a->mSphB[i]);
    if (!here) continue;
    const bool route = k == dknb_link() && dkn_link_reverse_route(k);
    const int rb = route || (s.def.parried && s.def.parry_tick == now) ? s.def.rebound(now) : 0;
    if (rb == 1) {
      pfx_settle(pfx::Kind::DarknutOnLink, s.id, true);  // the orange spark / high clash the pass previewed is this parry
      ++s.fx_route_enemy[dkn::phase2(s.ph) ? 1 : 0];
      dkn_fx_route(s, a, 9, s.def.parried_key, dkn::phase2(s.ph) ? "DARKNUT_SWORD_PARRIES_LINK" : "DARKNUT_SHIELD_PARRIES_LINK");  // FIX08-06
      char ex[200];
      std::snprintf(ex, sizeof(ex), "rebound=NATIVE_checkCutReverseAt_TRUE window=ARMED swing_key=%llu ordinal=%u credit=PENDING(link PROC_CUT_REVERSE read later) counter=PENDING",
                    static_cast<unsigned long long>(s.def.parried_key), s.def.parry_ordinal);
      dkn_log("ENEMY_PARRY_REBOUND", s, a, ex);
      dkn_log("SWORD_REBOUND", s, a, "link=procCutReverseInit native=1 confirmation=NEXT_READ");
    } else if (rb == 0) {
      *r = FALSE;
      pfx_settle(pfx::Kind::DarknutOnLink, s.id, false);
      if (!route && s.def.window_open(now)) {
        ++s.st_rebound_no_route;
        dkn_log("ENEMY_PARRY_REFUSED", s, a, "reason=LINK_PROC_WITHOUT_PROC_CUT_REVERSE (aerial / spin) parry=0 rebound=0 credit=0");
      } else if (s.def.rebound_suppressed <= 40 || s.def.rebound_suppressed % 25 == 0) {
        dkn_log("ORDINARY_BLOCK_NO_REBOUND", s, a, "window=NOT_ARMED parry=0");
      }
    }
    return;
  }
}
// FIX04: the two previews the parry FX pass asks about a Darknut contact (read-only: no RNG, no state). (A) Link's Shield Attack on THIS Darknut's
// weapon / shield-kick AT: an open, unresolved impact window and Link's pass of the NEXT tick inside the shared window (the verdict dkn_link_contact
// gives). (C) Link's sword on THIS Darknut's guard sphere (mSphA, or the phase-1 shield mSphC): the armed window of the NEXT tick, a frontal Link with a
// PROC_CUT_REVERSE route (the verdict on_dkn_cut_reverse_post gives).
bool dkn_preview_link_parry(daB_TN_c* a, daAlink_c* k, const cCcD_Obj* at) noexcept {
  if (!dn.enabled || !a || !k) return false;
  DknSlot* s = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (s && s->ph == dkn::Ph::Transform) {  // FIX07: the thrown mace (anchored this tick, its throw not yet resolved): the same shared window
    if (!s->mace_anchor || !s->mace.serial || s->mace.resolved == s->mace.serial) return false;
    bool mace_own = at == static_cast<const cCcD_Obj*>(&a->mCps);
    for (int i = 0; i < 4 && !mace_own; ++i) mace_own = at == static_cast<const cCcD_Obj*>(&a->mSwordSphs[i]);
    return mace_own && post08::Parry::in_window(pfx_link_pass(k, g.p8.tick + 1));
  }
  if (!s || !dkn_combat(*s) || !s->led.fresh()) return false;
  bool own = false;
  if (dkn_sword_move(s->led.move)) {
    for (int i = 0; i < 4 && !own; ++i) own = at == static_cast<const cCcD_Obj*>(&a->mSwordSphs[i]);
    own = own || at == static_cast<const cCcD_Obj*>(&a->mCps);
  } else {
    own = at == static_cast<const cCcD_Obj*>(&a->mSphC);
  }
  if (!own) return false;
  const post08::LinkView v = pfx_link_pass(k, g.p8.tick + 1);
  return post08::Parry::in_window(v);
}
bool dkn_preview_enemy_parry(daB_TN_c* a, daAlink_c* k, const cCcD_Obj* tg) noexcept {
  if (!dn.enabled || !a || !k) return false;
  DknSlot* s = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!s || !dkn_combat(*s) || s->brk.st != dkn::Bk::Combat) return false;
  bool guard_sph = tg == static_cast<const cCcD_Obj*>(&a->mSphC);
  for (int i = 0; i < 3 && !guard_sph; ++i) guard_sph = tg == static_cast<const cCcD_Obj*>(&a->mSphA[i]);
  const std::uint64_t next = g.p8.tick + 1;
  const bool shield_ok = dkn::phase2(s->ph) || (a->mNextBreakPart < 11 && a->mStates[12] == 0);
  return guard_sph && shield_ok && s->def.window_open(next) && s->def.window_key == s->swing_key && dkn_front(a, k) && dkn_link_reverse_route(k);
}

// ---------------------------------------------------------------- FIX07: what LDG01 (Link's dodge i-frames) asks about a Darknut contact (read only)
// class 1 = the thrust (Move F2A, TNB_ATACK_A: the only verified genuine thrust of the supported enemies) on a sword collider; 0 = ordinary (every other
// move, the kick, the mace, the thrown mace). impact = the exact identity of the contact: the impact-window serial (or the throw serial of the mace).
struct DknLdg { bool own = false; int cls = 0; std::uint64_t impact = 0; };
DknLdg dkn_ldg_query(const fopAc_ac_c* ac, const cCcD_GObjInf* at) noexcept {
  DknLdg r;
  if (!dn.enabled || !ac || fopAcM_GetName(const_cast<fopAc_ac_c*>(ac)) != fpcNm_B_TN_e) return r;
  auto* a = static_cast<daB_TN_c*>(const_cast<fopAc_ac_c*>(ac));
  DknSlot* s = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!s) return r;
  r.own = true;
  bool sword = at == static_cast<const cCcD_GObjInf*>(&a->mCps);
  for (int i = 0; i < 4 && !sword; ++i) sword = at == static_cast<const cCcD_GObjInf*>(&a->mSwordSphs[i]);
  if (s->ph == dkn::Ph::Transform) return r.impact = s->mace.serial ? 0x2000000000000000ull | s->mace.serial : 0, r;
  dkn::Move m;
  if (dkn_combat(*s) && dkn_move_now(*s, a, m)) {
    r.cls = sword && m == dkn::Move::F2A ? 1 : 0;
    if (s->led.open) r.impact = s->led.serial;
  }
  return r;
}

// ---------------------------------------------------------------- FIX06 / FIX07: the sword darkens with the body in the DEFINITIVE death
// FIX06 wrote TEV color 0 of the sword in the draw PRE and restored it in the draw POST. Measured cause of the failure (2.0.3 source + EXE): inside draw() the
// native calls g_env_light.setLightTevColorType_MAJI(sword model, &tevStr) right before mpModelMorf1->entryDL() (EXE 0xcc882d -> 0xcc8839), and
// setLightTevColorType_MAJI_sub does material->setTevColor(0, &tevstr->TevColor) for every material of that model: the FIX06 value was overwritten with
// tevStr.TevColor before mDoExt_modelEntryDL diffed the material (J3DModel::diff, TevReg flag 0x1000000 of the 0x11000084 diff flags) into the instance
// display list that the GPU consumes. The body escapes it because the native writes mMaterialColor AFTER its own MAJI call (EXE 0xcc866b -> 0xcc86d7).
// FIX07: the draw PRE only arms THIS instance's sword morf (the exact mDoExt_McaMorfSO pointer, definitive death, mMaterialColor != 0, sword drawn); the
// write happens in the PRE of that morf's own entryDL (a new hook target: exported, called out of line from draw), i.e. after MAJI and before the diff, with
// the body's own value; its POST restores the exact saved colours, so the shared J3DModelData of every other mType-1 Darknut is untouched once entryDL
// returns. The per-instance differed display list keeps the darkened TEV register for the frame(s) it is drawn (also the PC frame-interpolation replay,
// which captures the material inside J3DModel::entry, between the PRE and the POST). The draw POST disarms (and restores anything left, never expected).
HookAction on_dkn_draw_pre(ModContext*, void* args, void*, void*) noexcept {
  dn.tint_data = nullptr, dn.tint_n = 0, dn.tint_morf = nullptr, dn.tint_slot = nullptr;
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a || a->mType != 1) return HOOK_CONTINUE;
  DknSlot* s = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!s || !dkn::sword_tint(s->ph == dkn::Ph::Dead, a->mMaterialColor)) return HOOK_CONTINUE;
  if (a->mDisappear || a->mInactive || a->mStates[15] == 7 || !a->mpModelMorf1 || !a->mpModelMorf1->getModel()) return HOOK_CONTINUE;  // draw() draws no sword then
  // FIX08-05: the sword's own value for the body's progress (same relative darkening after the TEV clamp, kSwordTint); FIX07 wrote the body's raw value
  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;
  const short body = dkn::tint_value(a->mMaterialColor);
  if (body != s->tint_body_last && s->tint_samples < 24 && (body == -1 || body % 4 == 0)) {  // the whole series, sampled (never per frame)
    s->tint_body_last = body, ++s->tint_samples;
    char ex[200];
    std::snprintf(ex, sizeof(ex), "material_color=%.2f body_c0=%d sword_c0=%d sword_state=%u mode=%d frame=%.1f rule=SAME_RELATIVE_DARKENING_AFTER_TEV_CLAMP(kSwordTint,RASC0.85)",
                  static_cast<double>(a->mMaterialColor), static_cast<int>(body), static_cast<int>(dn.tint_value), static_cast<unsigned>(a->mStates[15]), a->mActionMode2,
                  static_cast<double>(dkn_frame(a)));
    dkn_log("SWORD_DARKENING_STEP", *s, a, ex);
  }
  return HOOK_CONTINUE;
}
// mDoExt_McaMorfSO::entryDL PRE: the armed morf only (every other morf of the game: one pointer comparison).
HookAction on_dkn_morf_entry_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.tint_morf || !args || mods::arg<const mDoExt_McaMorfSO*>(args, 0) != dn.tint_morf) return HOOK_CONTINUE;
  mDoExt_McaMorfSO* mf = mods::arg<mDoExt_McaMorfSO*>(args, 0);
  J3DModel* m = mf->getModel();
  J3DModelData* d = m ? m->getModelData() : nullptr;
  if (!d) return HOOK_CONTINUE;
  const unsigned n = d->getMaterialNum() < 8 ? d->getMaterialNum() : 8u;
  const short v = dn.tint_value;
  bool maji = false;
  for (unsigned i = 0; i < n; ++i) {
    J3DMaterial* mat = d->getMaterialNodePointer(static_cast<u16>(i));
    const J3DGXColorS10* c = mat ? mat->getTevColor(0) : nullptr;
    dn.tint_saved[i][0] = c ? c->r : 0, dn.tint_saved[i][1] = c ? c->g : 0, dn.tint_saved[i][2] = c ? c->b : 0;
    if (c && (c->r != v || c->g != v || c->b != v)) maji = true;
    if (c) dkn_native_sword_color(mat, v, v, v);
  }
  dn.tint_data = d, dn.tint_n = n, ++dn.tint_draws, ++dn.tint_entry_writes;
  dn.tint_overwritten_by_maji += maji;
  if (DknSlot* s = dn.tint_slot) {
    ++s->st_sword_tint_draws, ++s->st_sword_tint_entries;
    fopAc_ac_c* p = nullptr;
    if (s->st_sword_tint_entries == 1 && fopAcM_SearchByID(s->id, &p) && p && fopAcM_GetName(p) == fpcNm_B_TN_e) {
      auto* a = static_cast<daB_TN_c*>(p);
      char ex[260];
      std::snprintf(ex, sizeof(ex), "material_color=%.2f body_c0=%d tev_c0=%d materials=%u stage=DEAD definitive=1 sword_state=%u where=mDoExt_McaMorfSO::entryDL_PRE after=MAJI(tevStr.TevColor=%d) "
                    "before=mDoExt_modelEntryDL_diff scope=THIS_ENTRY_ONLY", static_cast<double>(a->mMaterialColor), static_cast<int>(dkn::tint_value(a->mMaterialColor)), v, n,
                    static_cast<unsigned>(a->mStates[15]), static_cast<int>(a->tevStr.TevColor.r));
      dkn_log("SWORD_DARKENING_START", *s, a, ex);
    }
  }
  return HOOK_CONTINUE;
}
void dkn_tint_restore() noexcept {
  J3DModelData* d = dn.tint_data;
  if (!d) return;
  for (unsigned i = 0; i < dn.tint_n; ++i) {
    J3DMaterial* mat = d->getMaterialNodePointer(static_cast<u16>(i));
    if (mat && mat->getTevColor(0)) dkn_native_sword_color(mat, dn.tint_saved[i][0], dn.tint_saved[i][1], dn.tint_saved[i][2]);
  }
  dn.tint_data = nullptr, dn.tint_n = 0, ++dn.tint_restores;
}
void on_dkn_morf_entry_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.tint_data || !args || mods::arg<const mDoExt_McaMorfSO*>(args, 0) != dn.tint_morf) return;
  dkn_tint_restore();
  ++dn.tint_entry_restores;
}
void on_dkn_draw_post(ModContext*, void*, void*, void*) noexcept {
  if (dn.tint_data) ++dn.tint_unrestored, dkn_tint_restore();  // an entryDL POST that never ran (never expected): nothing stays written past this draw
  dn.tint_morf = nullptr, dn.tint_slot = nullptr;
}

// ---------------------------------------------------------------- deletion, HUD, summary, install
void dkn_slot_summary(const DknSlot& s, const char* why) noexcept {
  for (unsigned g = 0; g < 3; ++g) {
    const DknStage& t = s.stg[g];
    if (!t.opportunities && !t.combo_start[0] && !t.combo_start[1] && !t.combo_start[2] && !t.combo_start[3] && !t.singles && !t.specials && !t.breaks) continue;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line),
                  "DARKNUT_STAGE_SUMMARY actor=%u generation=%llu why=%s stage=F%u opportunities=%llu decisions=%llu combo_sel=%llu/%llu/%llu/%llu combo_start=%llu/%llu/%llu/%llu "
                  "combo_end=%llu/%llu/%llu/%llu combo_cancel=%llu/%llu/%llu/%llu singles=%llu counters=%llu specials=%llu native_suppressed=%llu sa_rejected=%llu helm_preps=%llu "
                  "adaptive_guards=%llu enemy_parries=%llu breaks=%llu executions=%llu idle_max=%llu",
                  s.id, static_cast<unsigned long long>(s.generation), why, g + 1, static_cast<unsigned long long>(t.opportunities), static_cast<unsigned long long>(t.decisions),
                  static_cast<unsigned long long>(t.combo_sel[0]), static_cast<unsigned long long>(t.combo_sel[1]), static_cast<unsigned long long>(t.combo_sel[2]),
                  static_cast<unsigned long long>(t.combo_sel[3]), static_cast<unsigned long long>(t.combo_start[0]), static_cast<unsigned long long>(t.combo_start[1]),
                  static_cast<unsigned long long>(t.combo_start[2]), static_cast<unsigned long long>(t.combo_start[3]), static_cast<unsigned long long>(t.combo_end[0]),
                  static_cast<unsigned long long>(t.combo_end[1]), static_cast<unsigned long long>(t.combo_end[2]), static_cast<unsigned long long>(t.combo_end[3]),
                  static_cast<unsigned long long>(t.combo_cancel[0]), static_cast<unsigned long long>(t.combo_cancel[1]), static_cast<unsigned long long>(t.combo_cancel[2]),
                  static_cast<unsigned long long>(t.combo_cancel[3]), static_cast<unsigned long long>(t.singles), static_cast<unsigned long long>(t.counters),
                  static_cast<unsigned long long>(t.specials), static_cast<unsigned long long>(t.suppressed), static_cast<unsigned long long>(t.sa_rejected),
                  static_cast<unsigned long long>(t.helm_preps), static_cast<unsigned long long>(t.guards), static_cast<unsigned long long>(t.parries),
                  static_cast<unsigned long long>(t.breaks), static_cast<unsigned long long>(t.executions), static_cast<unsigned long long>(s.ini.idle_max));
    dkn_line(false, line);
  }
  {  // FIX04: the defense by ordinal (swings seen / real rolls / attempts / defended contacts / parries per ordinal 1..6), credits, counters, slips, exchange
    char line[kLogLineMax * 2];
    const dkn::Defense& d = s.def;
    using U = unsigned long long;
    std::snprintf(line, sizeof(line),
                  "DARKNUT_DEFENSE_SUMMARY actor=%u ordinal_pct=0/0/0/25/45/60 swings=%llu/%llu/%llu/%llu/%llu/%llu rolls=%llu/%llu/%llu/%llu/%llu/%llu attempts=%llu/%llu/%llu/%llu/%llu/%llu "
                  "contacts=%llu/%llu/%llu/%llu/%llu/%llu parries=%llu/%llu/%llu/%llu/%llu/%llu confirmed=%llu unconfirmed=%llu repeats=%llu no_route=%llu link_parry_gains=%llu "
                  "counters=%llu/%llu/%llu counter_dropped=%llu windows_expired=%llu rerolls_refused=%llu no_shield=%llu gap_breaks=%llu whiff_breaks=%llu hold_ticks=%llu hold_releases=%llu "
                  "offense_refused_by_exchange=%llu ordinary_blocks=%llu rebound_suppressed=%llu slips_seen=%llu slips_capped=%llu native_entries_post=%llu unauthorized_executed=%llu kick_slowed=%llu",
                  s.id, U(d.swings[0]), U(d.swings[1]), U(d.swings[2]), U(d.swings[3]), U(d.swings[4]), U(d.swings[5]), U(d.rolls[0]), U(d.rolls[1]), U(d.rolls[2]), U(d.rolls[3]),
                  U(d.rolls[4]), U(d.rolls[5]), U(d.attempts[0]), U(d.attempts[1]), U(d.attempts[2]), U(d.attempts[3]), U(d.attempts[4]), U(d.attempts[5]), U(d.contacts[0]),
                  U(d.contacts[1]), U(d.contacts[2]), U(d.contacts[3]), U(d.contacts[4]), U(d.contacts[5]), U(d.parries_at[0]), U(d.parries_at[1]), U(d.parries_at[2]),
                  U(d.parries_at[3]), U(d.parries_at[4]), U(d.parries_at[5]), U(d.confirmed), U(d.unconfirmed), U(d.repeats), U(s.st_rebound_no_route), U(s.st_parry_gain),
                  U(d.counters[0]), U(d.counters[1]), U(d.counters[2]), U(d.counter_dropped + s.st_counter_dropped), U(d.expired), U(d.rerolls_refused), U(d.no_shield), U(d.gap_breaks),
                  U(d.whiff_breaks), U(s.st_hold_ticks), U(s.st_hold_releases), U(s.st_hold_refused), U(s.st_ordinary_blocks), U(d.rebound_suppressed), U(s.st_slip_seen),
                  U(s.st_slip_capped), U(s.st_entry_post), U(s.st_unauthorized), U(s.st_kick_slowed));
    dkn_line(false, line);
  }
  {  // FIX05: chain registration, exchange beyond 300, rebound persistence, F2/F3 idle kick refused, orphan AT, transformation re-anchor
    char line[kLogLineMax];
    using U = unsigned long long;
    std::snprintf(line, sizeof(line),
                  "DARKNUT_FIX05_SUMMARY actor=%u tracked_far=%llu late_rolls=%llu late_registered=%llu(core %llu) hold_ticks_beyond_300=%llu rebound_seen=%llu rebound_lost=%llu(core %llu) "
                  "idle_special_refused_f2f3=%llu at_orphan_off=%llu transform_reanchor=%llu transform_at_restored=%llu",
                  s.id, U(s.st_tracked_far), U(s.st_late_rolls), U(s.st_late_registered), U(s.def.late_registered), U(s.st_hold_beyond_300), U(s.st_rebound_seen), U(s.st_rebound_lost),
                  U(s.def.rebound_lost), U(s.st_idle_special_refused), U(s.st_at_orphan_off), U(s.st_tf_reanchor), U(s.st_tf_restore));
    dkn_line(false, line);
  }
  {  // FIX06: the sequences (routes, continuations, ends by reason), phase-1 executions, reposition, sword darkening
    char line[kLogLineMax * 2];
    using U = unsigned long long;
    const dkn::Seq& q = s.seq;
    const dkn::Defense& d = s.def;
    std::snprintf(line, sizeof(line),
                  "DARKNUT_FIX06_SUMMARY actor=%u generation=%llu parry_routes=%llu direct_combo=%llu(%llu/%llu/%llu/%llu) counter=%llu seq_begun=%llu counters_done=%llu continue_rolls=%llu "
                  "continue_combo=%llu(%llu/%llu/%llu/%llu) continue_exit=%llu combos_started=%llu combos_done=%llu ends=done:%llu,counter_only:%llu,break:%llu,phase:%llu,defeat:%llu,"
                  "no_target:%llu,event:%llu,link_broken:%llu,far:%llu,watchdog:%llu,incapacitated:%llu,refused:%llu,dropped:%llu,reset:%llu bridge_held_ticks=%llu link_hits_in_seq=%llu "
                  "link_blocks_in_seq=%llu f1_md=%llu f1_helm=%llu f1_helm_prep=%llu repo_jumps=%llu repo_holds=%llu repo_landing_attacks_taken_back=%llu repo_skipped=%llu sword_tint_draws=%llu",
                  s.id, U(s.generation), U(d.route_rolls), U(d.route_combos), U(d.route_pick[0]), U(d.route_pick[1]), U(d.route_pick[2]), U(d.route_pick[3]), U(d.route_counters), U(q.begun),
                  U(q.counters_done), U(q.cont_rolls), U(q.cont_combo), U(q.cont_pick[0]), U(q.cont_pick[1]), U(q.cont_pick[2]), U(q.cont_pick[3]), U(q.cont_exit), U(q.combos_started),
                  U(q.combos_done), U(q.ended[1]), U(q.ended[2]), U(q.ended[3]), U(q.ended[4]), U(q.ended[5]), U(q.ended[6]), U(q.ended[7]), U(q.ended[8]), U(q.ended[9]), U(q.ended[10]),
                  U(q.ended[11]), U(q.ended[12]), U(q.ended[13]), U(q.ended[14]), U(s.st_seq_bridge_held), U(s.st_seq_hits_taken), U(s.st_seq_blocks), U(s.st_f1_md_contact),
                  U(s.st_f1_helm_contact), U(s.st_f1_helm_prep), U(s.st_repo_jumps), U(s.st_repo_holds), U(s.st_repo_land_attack), U(s.st_repo_cancel), U(s.st_sword_tint_draws));
    dkn_line(false, line);
  }
  {  // FIX07: offensive leap, reviewed reposition, thrown mace, sword darkening at entryDL
    char line[kLogLineMax * 2];
    using U = unsigned long long;
    const dkn::GapCloser& gc = s.gap;
    std::snprintf(line, sizeof(line),
                  "DARKNUT_FIX07_SUMMARY actor=%u generation=%llu leap_rolls=%llu leap_pick=%llu(%llu/%llu/%llu/%llu) normal_approach=%llu rearmed=%llu geo_refused=%llu f1_not_available=%llu "
                  "leap_started=%llu landed=%llu combos=%llu cancelled=%llu landing_attacks_taken_back=%llu native_entries_cut=%llu repo_long=%llu repo_wait_front=%llu repo_geo_unsafe=%llu "
                  "mace_throws=%llu mace_block=%llu mace_direct=%llu mace_parry=%llu mace_contacts=%llu mace_repeats=%llu sword_tint_entries=%llu",
                  s.id, U(s.generation), U(gc.rolls), U(gc.leaps), U(gc.pick[0]), U(gc.pick[1]), U(gc.pick[2]), U(gc.pick[3]), U(gc.normals), U(gc.rearmed), U(gc.geo_refused),
                  U(gc.f1_refused), U(s.st_leap_start), U(s.st_leap_land), U(s.st_leap_combo), U(s.st_leap_cancel), U(s.st_leap_land_attack), U(s.st_leap_native_cut), U(s.st_repo_long),
                  U(s.st_repo_wait_front), U(s.st_repo_geo_unsafe), U(s.mace.throws), U(s.mace.blocks), U(s.mace.directs), U(s.mace.parries), U(s.st_mace_contacts), U(s.st_mace_repeat),
                  U(s.st_sword_tint_entries));
    dkn_line(false, line);
  }
  {  // FIX08: the Helm recovery look, the evasion (denominators: eligible swings by ordinal / real rolls / won / starts), the neutral receipts, B1, the parry routes
    char line[kLogLineMax * 2];
    using U = unsigned long long;
    const dkn::Evade& e = s.evade;
    std::snprintf(line, sizeof(line),
                  "DARKNUT_FIX08_SUMMARY actor=%u generation=%llu helm_look_runs=%llu follows=%llu turn_ticks=%llu tails=%llu ends=%llu evade_pct=0/10/15/20/20/20 "
                  "evade_opportunities=%llu/%llu/%llu/%llu/%llu/%llu evade_rolls=%llu/%llu/%llu/%llu/%llu/%llu evade_won=%llu/%llu/%llu/%llu/%llu/%llu starts=%llu(L%llu,R%llu,B%llu) "
                  "cooldown_refused=%llu rerolls_refused=%llu not_eligible=%llu no_direction=%llu lands=%llu cancels=%llu hits_during=%llu landing_attacks_taken_back=%llu "
                  "native_evades_suppressed=%llu neutral_arms=%llu neutral_receipts=%llu link_swing_refused=%llu b1_recoveries=%llu parry_routes=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
                  "darknut_parries=F1:%llu,F2F3:%llu",
                  s.id, U(s.generation), U(s.helm_look.runs), U(s.helm_look.follows), U(s.helm_look.turn_ticks), U(s.helm_look.tails), U(s.helm_look.ends), U(e.opportunities[0]),
                  U(e.opportunities[1]), U(e.opportunities[2]), U(e.opportunities[3]), U(e.opportunities[4]), U(e.opportunities[5]), U(e.rolls[0]), U(e.rolls[1]), U(e.rolls[2]), U(e.rolls[3]),
                  U(e.rolls[4]), U(e.rolls[5]), U(e.won[0]), U(e.won[1]), U(e.won[2]), U(e.won[3]), U(e.won[4]), U(e.won[5]), U(e.starts), U(e.dir[1]), U(e.dir[2]), U(e.dir[3]),
                  U(e.cooldown_refused), U(e.rerolls_refused), U(e.not_eligible), U(e.no_direction), U(e.lands), U(e.cancels), U(e.hits_taken), U(e.landing_attacks), U(e.native_suppressed),
                  U(s.ngate.arms), U(s.ngate.receipts), U(s.st_link_swing_refused), U(s.st_b1_recover), U(s.fx_route[0]), U(s.fx_route[1]), U(s.fx_route[2]), U(s.fx_route[3]), U(s.fx_route[4]),
                  U(s.fx_route[5]), U(s.fx_route[6]), U(s.fx_route[7]), U(s.fx_route[8]), U(s.fx_route[9]), U(s.fx_route_enemy[0]), U(s.fx_route_enemy[1]));
    dkn_line(false, line);
  }
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "DARKNUT_INITIATIVE_REJECTS actor=%u busy=%llu unsafe=%llu no_link=%llu far=%llu not_front=%llu gap=%llu reentry=%llu not_neutral=%llu defending=%llu link_swing=%llu idle_avg=%.1f",
                s.id, static_cast<unsigned long long>(s.ini.rejects[1]), static_cast<unsigned long long>(s.ini.rejects[2]), static_cast<unsigned long long>(s.ini.rejects[3]),
                static_cast<unsigned long long>(s.ini.rejects[4]), static_cast<unsigned long long>(s.ini.rejects[5]), static_cast<unsigned long long>(s.ini.rejects[6]),
                static_cast<unsigned long long>(s.ini.rejects[7]), static_cast<unsigned long long>(s.ini.rejects[8]), static_cast<unsigned long long>(s.ini.rejects[9]),
                static_cast<unsigned long long>(s.ini.rejects[10]), s.ini.idle_n ? static_cast<double>(s.ini.idle_total) / static_cast<double>(s.ini.idle_n) : 0.0);
  dkn_line(false, line);
}
HookAction on_dkn_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  if (DknSlot* s = dkn_find(id)) {
    dkn_slot_summary(*s, "DELETE");
    dkn_log("DELETE", *s, a, "slot=RETIRED");
    *s = DknSlot{};
    ++dn.deletes;
  }
  for (std::uint32_t& r : dn.rejected_ids)
    if (r == id) r = 0;
  return HOOK_CONTINUE;
}
void on_dkn_link_damage_post(ModContext*, void*, void*, void*) noexcept {
  if (!dn.enabled) return;
  daAlink_c* link = dknb_link();
  if (!link) return;
  dknb_direct_capture(link);
  for (DknSlot& s : dn.s) {  // FIX02: a guard slip this call started from a Darknut contact keeps at most its capped speed
    if (!s.on || !s.slip_pending) continue;
    fopAc_ac_c* p = nullptr;
    const daB_TN_c* a = fopAcM_SearchByID(s.id, &p) && p && fopAcM_GetName(p) == fpcNm_B_TN_e ? static_cast<const daB_TN_c*>(p) : nullptr;
    dkn_slip_cap(s, link, a, "LINK_CHECK_DAMAGE_POST");
  }
}
void dkn_hud_samples() noexcept {
  if (!dn.enabled) return;
  unsigned slot = 0;
  for (DknSlot& s : dn.s) {
    if (!s.on || slot >= kDknHudSlots) continue;
    if (!dkn_combat(s)) continue;  // no bar while the transformation, the fall, the down time, the Finish, the getup or the resurrection own the actor
    fopAc_ac_c* p = nullptr;
    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;
    bool busy = false;
    const int pct = s.post.max_pts > 0 ? s.post.points() * 100 / s.post.max_pts : 0;
    (void)dknb_hud(slot++, s.id, s.generation, pct, s.post.latched, false, s.brk.vulnerable(), static_cast<daB_TN_c*>(p), busy);
  }
}
void dkn_summary(const char* why) noexcept {
  std::uint64_t t[32] = {};
  for (const DknSlot& s : dn.s) {
    const std::uint64_t v[32] = {s.st_hp1, s.st_hp2, s.st_gain, s.st_parry, s.st_block_link, s.st_direct_link, s.st_blocked_by_dk, s.st_reverted, s.st_yoroke_blocked, s.st_breaks,
                                 s.st_punish, s.st_combos, s.st_combo_cancel, s.st_specials, s.st_chain_cut, s.st_impacts, s.st_armor_calls, s.st_regen_ticks, s.st_suppressed,
                                 s.st_sa_rejected, s.st_helm_prep, s.st_md_start, s.st_md_contact, s.st_helm_start, s.st_helm_contact, s.st_exec, s.st_exec_lethal, s.st_getups,
                                 s.st_guards, s.st_enemy_parry, s.st_counters, s.st_headlock_off};
    for (unsigned i = 0; i < 32; ++i) t[i] += v[i];
    if (s.on) dkn_slot_summary(s, why);
  }
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "DKN01_SUMMARY why=%s version=%s tag=%s enabled=%d admitted=%llu ineligible=%llu full=%llu deletes=%llu hp1_hits=%llu hp2_hits=%llu posture_gains=%llu link_parries=%llu "
                "link_blocks=%llu link_direct=%llu dk_blocks=%llu reactions_reverted=%llu yoroke_to_block=%llu breaks=%llu hits_in_break=%llu combos=%llu move_cancels=%llu specials=%llu "
                "late_native_entries=%llu impacts=%llu armor_calls=%llu regen_ticks=%llu native_suppressed=%llu sa_rejected=%llu helm_preps=%llu mortal_starts=%llu mortal_contacts=%llu "
                "helm_starts=%llu helm_contacts=%llu executions=%llu exec_lethal=%llu getups=%llu adaptive_guards=%llu enemy_parries=%llu counters=%llu headlock_off=%llu unload_fixed=%llu "
                "dropped_lines=%llu runtime=HUMAN_RUNTIME_PENDING",
                why, dkn::kVersion, dkn::kTag, dn.enabled ? 1 : 0, static_cast<unsigned long long>(dn.admitted), static_cast<unsigned long long>(dn.ineligible),
                static_cast<unsigned long long>(dn.full), static_cast<unsigned long long>(dn.deletes), static_cast<unsigned long long>(t[0]), static_cast<unsigned long long>(t[1]),
                static_cast<unsigned long long>(t[2]), static_cast<unsigned long long>(t[3]), static_cast<unsigned long long>(t[4]), static_cast<unsigned long long>(t[5]),
                static_cast<unsigned long long>(t[6]), static_cast<unsigned long long>(t[7]), static_cast<unsigned long long>(t[8]), static_cast<unsigned long long>(t[9]),
                static_cast<unsigned long long>(t[10]), static_cast<unsigned long long>(t[11]), static_cast<unsigned long long>(t[12]), static_cast<unsigned long long>(t[13]),
                static_cast<unsigned long long>(t[14]), static_cast<unsigned long long>(t[15]), static_cast<unsigned long long>(t[16]), static_cast<unsigned long long>(t[17]),
                static_cast<unsigned long long>(t[18]), static_cast<unsigned long long>(t[19]), static_cast<unsigned long long>(t[20]), static_cast<unsigned long long>(t[21]),
                static_cast<unsigned long long>(t[22]), static_cast<unsigned long long>(t[23]), static_cast<unsigned long long>(t[24]), static_cast<unsigned long long>(t[25]),
                static_cast<unsigned long long>(t[26]), static_cast<unsigned long long>(t[27]), static_cast<unsigned long long>(t[28]), static_cast<unsigned long long>(t[29]),
                static_cast<unsigned long long>(t[30]), static_cast<unsigned long long>(t[31]), static_cast<unsigned long long>(dn.unload_fixed), static_cast<unsigned long long>(dn.dropped));
  dknb_write(false, line);
}
// Unload (hooks already removed by the host): a Darknut held in a stretched stagger clip (rate down to 0) or mid-move gets the native rate back and its
// HeadLock off, so the native clip ends and the native AI continues; found again by process id, never by a kept pointer.
void dkn_shutdown() noexcept {
  const bool was = dn.enabled;
  dn.enabled = false;
  for (DknSlot& s : dn.s) {
    if (!s.on || !(s.lease != DknLease::None || s.brk.active() || s.neck_on)) continue;
    fopAc_ac_c* p = nullptr;
    if (fopAcM_SearchByID(s.id, &p) && p && fopAcM_GetName(p) == fpcNm_B_TN_e) {
      dkn_native_rate(static_cast<daB_TN_c*>(p), 1.0f);
      dkn_native_headlock(static_cast<daB_TN_c*>(p), false);
      if (s.neck_on) dkn_native_neck(static_cast<daB_TN_c*>(p), false), s.neck_on = false;  // FIX08: the native value of mType 1 back
      ++dn.unload_fixed;
    }
  }
  if (was) dkn_summary("SHUTDOWN");
}
void dkn_install() noexcept {
  for (unsigned i = 0; i < dkn::kPlans; ++i) dn.plans[i] = dkn::plan(dkn::combo_of(i), i), dn.tls[i] = dkn::timeline(dn.plans[i]);
  HookOptions late = HOOK_OPTIONS_INIT;
  late.priority = -402;  // Link's damage receipt: after the FIX01 capture, the Lizalfos (-400) and the Chilfos (-401) receipts of the same call
  HookOptions after_kk = HOOK_OPTIONS_INIT;
  after_kk.priority = -60;  // Link's cut choice / rebound: after the Lizalfos (default) and the Chilfos (-50) callbacks of the same call
  ModResult r[20];
  for (ModResult& x : r) x = MOD_ERROR;
  r[0] = mods::hook::add_pre<DknAction>(on_dkn_action_pre), r[1] = mods::hook::add_post<DknAction>(on_dkn_action_post);
  r[2] = mods::hook::add_pre<DknDamageCheck>(on_dkn_damage_pre), r[3] = mods::hook::add_post<DknDamageCheck>(on_dkn_damage_post);
  r[4] = mods::hook::add_pre<DknSetDamage>(on_dkn_setdamage_pre), r[5] = mods::hook::add_post<DknSetDamage>(on_dkn_setdamage_post);
  r[6] = mods::hook::add_pre<DknAttackH>(on_dkn_attack_pre), r[7] = mods::hook::add_post<DknAttackH>(on_dkn_attack_post);
  r[8] = mods::hook::add_pre<DknAttackL>(on_dkn_attack_pre), r[9] = mods::hook::add_post<DknAttackL>(on_dkn_attack_post);
  r[10] = mods::hook::add_post<DknShieldH>(on_dkn_attack_post), r[11] = mods::hook::add_post<DknShieldL>(on_dkn_attack_post);
  r[12] = mods::hook::add_pre<DkrDelete>(on_dkn_delete_pre), r[13] = mods::hook::add_post<LinkDamageFix01>(on_dkn_link_damage_post, &late);
  r[14] = mods::hook::add_post<LinkCutAction>(on_dkn_cut_action_post, &after_kk), r[15] = mods::hook::add_post<LinkCutReverse>(on_dkn_cut_reverse_post, &after_kk);
  r[16] = mods::hook::add_pre<DknDraw>(on_dkn_draw_pre), r[17] = mods::hook::add_post<DknDraw>(on_dkn_draw_post);  // FIX06: the sword in the definitive death
  // FIX07: the darkening is written where the sword's material is really diffed (mDoExt_McaMorfSO::entryDL, after the native MAJI of the sword)
  r[18] = mods::hook::add_pre<DknMorfEntryDL>(on_dkn_morf_entry_pre), r[19] = mods::hook::add_post<DknMorfEntryDL>(on_dkn_morf_entry_post);
  bool ok = true;
  for (ModResult x : r) ok = ok && x == MOD_OK;
  dn.hooks = ok;
  if (!ok) {
    (void)mods::hook::uninstall<DknAction>(), (void)mods::hook::uninstall<DknDamageCheck>(), (void)mods::hook::uninstall<DknSetDamage>();
    (void)mods::hook::uninstall<DknAttackH>(), (void)mods::hook::uninstall<DknAttackL>(), (void)mods::hook::uninstall<DknShieldH>(), (void)mods::hook::uninstall<DknShieldL>();
    (void)mods::hook::uninstall<DknDraw>(), (void)mods::hook::uninstall<DknMorfEntryDL>();
  }
  dn.enabled = dn.hooks && dknb_dkr_enabled();  // all or nothing, and only with DKR01 (its resurrection, its identity filter, its execution fall) installed
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "DKN01 event=INSTALL version=%s tag=%s enabled=%d hooks=%d dkr01=%d target=B_TN_param_000001ff(mType1,mType2_0,switch_ff) hp1=%d res=%d posture=%d/%d/%d regen_delay=60 "
                "break=90_no_hit_cap executions=MORTAL_DRAW|SHIELD_ATTACK_HELM_SPLITTER down=180 combos=8 impacts=46 offense=OWN_CONTROLLER single_pct=%d(PROVISIONAL) "
                "passive=38/75 passive_cooldown=%u(PROVISIONAL) parry=REAL_REBOUND_CONFIRMED_F1_F2_F3 parry_ordinal_pct=0/0/0/25/45/60 exchange=LINK_CHAIN_HOLDS "
                "table_a_f1=+4blk/+6hp/+12lat/+14vert/+14bash link_on_enemy_parry=+20/+25(on_PROC_CUT_REVERSE) counter=THIRDS(F2:kick/thrust/double,F1:bash/lateral/vertical) "
                "counter_lands>=%u spl=LARGE_1 revive_clip=FIX02 f1_scale=2.59/1.44/2.56/1.45(FROZEN) f2_tempo=x1.40_vs_FIX02 singles=x%.2f kick=x%.2f "
                "slip_cap=12/12/10_per_impact_x0.80/x0.65 stagger_b=TNB_DAMAGE_R parry_fx=LIZALFOS_ORANGE_HIGH_BOTH_WAYS fix05=chain_every_defended_contact,"
                "hold_within_engage,f2f3_four_combos_only,f2f3_special_shield_held_only,rebound_persisting_2_reads,counter_double_f2c1_ref,draw_capsule_reanchor",
                dkn::kVersion, dkn::kTag, dn.enabled ? 1 : 0, dn.hooks ? 1 : 0, dknb_dkr_enabled() ? 1 : 0, dkn::policy::kHp1Max, dkn::policy::kRes2Max, dkn::policy::kPosture1Max,
                dkn::policy::kPosture2Max, dkn::policy::kPosture3Max, dkn::policy::kSinglePct, dkn::policy::kPassiveCooldownTicks, dkn::policy::kCounterLandTicks,
                static_cast<double>(dkn::policy::kF2SingleScale), static_cast<double>(dkn::policy::kKickScale));
  dknb_write(!dn.enabled, line);
  std::snprintf(line, sizeof(line),
                "DKN01 event=INSTALL_FIX06 version=%s tag=%s retime=F2-C2(1-2:27,3-4:30)F2-C4(1-2:27,2-3:25.5,4-5:30)_final_ticks_after_x1.40 parry_route=%d%%combo/%d%%counter "
                "counter_continue=%d%%combo direct_combo_first_impact=%u(window %u..%u) bridge=settle%u/land%u sequence=OWNS_INITIATIVE_PER_ACTOR_GENERATION "
                "f1_executions=MORTAL_DRAW|SHIELD_ATTACK_HELM_SPLITTER_%d_total helm_stagger=SWAY_+-%.1f/%.0f repo=<%.0f_back,%.0f..%.0f_hold,>%.0f_forward_if_landing>=%.0f "
                "sword_dark=DEFINITIVE_DEATH_ONLY(daB_TN_c::draw arms,mDoExt_McaMorfSO::entryDL writes after MAJI) runtime=HUMAN_RUNTIME_PENDING",
                dkn::kVersion, dkn::kTag, dkn::policy::kParryComboPct, 100 - dkn::policy::kParryComboPct, dkn::policy::kCounterComboPct, dkn::policy::kParryComboLandTicks,
                dkn::policy::kParryComboLandMin, dkn::policy::kParryComboLandMax, dkn::policy::kBridgeSettleTicks, dkn::policy::kBridgeLandTicks, dkn::policy::kF1ExecDamage,
                static_cast<double>(dkn::kSwayAmp), static_cast<double>(dkn::kSwayPeriod), static_cast<double>(dkn::policy::kRepoNear), static_cast<double>(dkn::policy::kRepoNear),
                static_cast<double>(dkn::policy::kRepoFar), static_cast<double>(dkn::policy::kRepoFar), static_cast<double>(dkn::policy::kRepoNear));
  dknb_write(!dn.enabled, line);
  std::snprintf(line, sizeof(line),
                "DKN01 event=INSTALL_FIX07 version=%s tag=%s f1c3=24/56/57/40_ticks(177) f1_executions=MORTAL_DRAW|HELM_SPLITTER_%d_total_saturated leap=F2F3_%d%%_leap_combo/%d%%_normal "
                "leap_modes=8(%.0f)/17(%.0f) land=%.0f..%.0f cooldown=%u(PROVISIONAL) episode=%u(PROVISIONAL) f1_leap=NOT_AVAILABLE repo=geo_checked_back_and_forward,native_turn_%u,long_leap_17 "
                "mace=block+%d/direct+%d/parry+0/dodge+0/darknut+0 sword_dark=entryDL_after_MAJI runtime=HUMAN_RUNTIME_PENDING",
                dkn::kVersion, dkn::kTag, dkn::policy::kF1ExecDamage, dkn::policy::kLeapPct, 100 - dkn::policy::kLeapPct, static_cast<double>(dkn::leap_model(dkn::Leap::Fwd).dist),
                static_cast<double>(dkn::leap_model(dkn::Leap::FwdLong).dist), static_cast<double>(dkn::policy::kLeapLandMin), static_cast<double>(dkn::policy::kEngage2),
                dkn::policy::kLeapCooldownTicks, dkn::policy::kLeapEpisodeTicks, dkn::policy::kRepoTurnTicks, dkn::policy::kMaceBlockLink, dkn::policy::kMaceDirectLink);
  dknb_write(!dn.enabled, line);
  std::snprintf(line, sizeof(line),
                "DKN01 event=INSTALL_FIX08 version=%s tag=%s table_b_f1=lateral_%d/%d,vertical_%d/%d,bash_%d/%d f1c3=36/56/57/40_ticks(189)_pair1_cut61_start16_morph8_rate1.50 "
                "helm_f1=native_neck_after_%u,body_turn_from_frame_%.0f_native_ChaseH,tail_%u(PROVISIONAL) evade_f2f3=%d/%d/%d/%d%%_after_parry_roll,cooldown_%u,native_jumps_11/14/6,"
                "native_evades_taken_back initiative=no_ordinary_over_live_swing(%.0f/%.0f,%u),defense_neutral_receipt_%u b1=TNB_WAIT_B_2_eased_morph_%.0f,wait_%d sword_dark=perceptual(kSwordTint)_DARKNUT_ONLY "
                "runtime=HUMAN_RUNTIME_PENDING",
                dkn::kVersion, dkn::kTag, dkn::link_gain(dkn::Move::F1A, dkn::Outcome::Block), dkn::link_gain(dkn::Move::F1A, dkn::Outcome::Direct),
                dkn::link_gain(dkn::Move::F1B, dkn::Outcome::Block), dkn::link_gain(dkn::Move::F1B, dkn::Outcome::Direct), dkn::link_gain(dkn::Move::F1Shield, dkn::Outcome::Block),
                dkn::link_gain(dkn::Move::F1Shield, dkn::Outcome::Direct), dkn::policy::kHelmLookDelayTicks, static_cast<double>(dkn::policy::kHelmTurnFromFrame), dkn::policy::kHelmLookTailTicks,
                dkn::policy::kEvadeOrdinalPct[0], dkn::policy::kEvadeOrdinalPct[1], dkn::policy::kEvadeOrdinalPct[2], dkn::policy::kEvadeOrdinalPct[3], dkn::policy::kEvadeCooldownTicks,
                static_cast<double>(dkn::policy::kSwingReachNormal), static_cast<double>(dkn::policy::kSwingReachLong), dkn::policy::kSwingActiveTicks, dkn::policy::kGuardLeaveTicks,
                static_cast<double>(dkn::policy::kB1RecoverMorph), dkn::policy::kB1RecoverTicks);
  dknb_write(!dn.enabled, line);
}
#undef DKN_NOINLINE
