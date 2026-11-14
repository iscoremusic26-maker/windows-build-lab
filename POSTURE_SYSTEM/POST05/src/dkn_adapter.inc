// LIGHT FADES TWICE 0.13.9 DARKNUT MINIBOSS (DKN01): production adapter of the normal Darknut of Enemy Spawner (daB_TN_c, parameters 0x000001FF,
// mType 1, mType2 0, switch 0xFF, no stay-no-2 flag: the DKR01 identity, dkr::eligible). Included once in mod.cpp's anonymous namespace after
// dkn_bridge.inc. The pure decisions live in dkn_core.hpp (tests/dkn_lab.cpp).
//
// Hook targets (all exported by dusklight.exe 2.0.3 and called for real, out of line, by action()/damage_check() - measured, see DKN01 IMPLEMENTATION.md):
//   daB_TN_c::action PRE/POST        per-tick life of the instance: phases, pending break, passivity, regeneration, stagger hold, AT authority, spl
//   daB_TN_c::damage_check PRE/POST  480 threshold for setDamage, the Darknut's impacts on Link (parry / block / direct), Link's blocked attacks
//   daB_TN_c::setDamage PRE/POST     the native damage commit: 420 HP + armour (phase 1), the 480 accumulator (phase 2), Table A, stagger suppression
//   daB_TN_c::executeAttackH/L, executeAttackShieldH/L PRE/POST   the combo director (native clips, sounds, AT frames, turning) and the passivity specials
//   daB_TN_c::_delete PRE (a second callback on the DKR01 target)  the slot dies with the actor
// Every write to the native actor goes through the DKN_NOINLINE writers below (ci/dkn_audit.py). Nothing else of the game is written; Link never.
// Ineligible actors (Darknut (Sword) 0x010001FF, Temple of Time mType 0, placed / Hyrule Castle variants) never get a slot: every callback returns at once.
#if defined(_MSC_VER)
#define DKN_NOINLINE __declspec(noinline)
#else
#define DKN_NOINLINE __attribute__((noinline))
#endif

struct DknSlot {
  bool on = false, admitted_logged = false;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  dkn::Ph ph = dkn::Ph::Armor;
  int hp1 = dkn::policy::kHp1Max;
  dkn::Posture post;
  dkn::Break brk;
  dkn::Passive pas;
  dkn::Selector sel;
  dkn::Ledger led;
  // combo / special
  bool combo = false, special = false, tail = false;
  unsigned combo_idx = 0, seg = 0, ct = 0, impacts = 0;
  dkn::Plan plan;
  dkn::Timeline tl;
  dkn::Move special_move = dkn::Move::F2C;
  std::uint64_t combo_tick = 0, special_tick = 0, impact_tick[9] = {};
  // damage_check / setDamage snapshots of THIS call
  int pre_act1 = 0, pre_act2 = 0, sd_nbp = 0, sd_acc = 0, sd_act1 = 0, sd_act2 = 0;
  bool sd_seen = false;
  // Link's blocked-attack dedupe (one gain per Link action: procedure + cut type + cut count)
  std::uint64_t link_key = 0, blocked_key = ~0ull;
  int link_prev_proc = -1, link_prev_cut = -1, link_prev_count = -1;
  // bookkeeping
  unsigned last_band = 99;
  int last_acc = 0;
  std::uint64_t st_hp1 = 0, st_hp2 = 0, st_gain = 0, st_parry = 0, st_block_link = 0, st_direct_link = 0, st_blocked_by_dk = 0, st_reverted = 0, st_yoroke_blocked = 0,
                st_breaks = 0, st_punish = 0, st_combos = 0, st_combo_cancel = 0, st_specials = 0, st_chain_cut = 0, st_impacts = 0, st_armor_calls = 0, st_regen_ticks = 0;
};
struct DknState {
  bool enabled = false, hooks = false;
  DknSlot s[dkn::kSlots];
  std::uint64_t next_generation = 1, admitted = 0, ineligible = 0, full = 0, deletes = 0, unload_fixed = 0, dropped = 0;
  std::uint32_t rejected_ids[16] = {};
  unsigned rejected_next = 0, lines = 0;
  std::uint64_t direct_tick = ~0ull;
  dkn::Plan plans[8];
  dkn::Timeline tls[8];
} dn;

void dkn_line(bool warn, const char* line) noexcept {
  if (dn.lines >= 2400) {
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
bool dkn_combat(const DknSlot& s) noexcept { return s.ph == dkn::Ph::Armor || dkn::phase2(s.ph); }
int dkn_max(const DknSlot& s) noexcept { return dkn::phase2(s.ph) ? dkn::policy::kPosture2Max : dkn::policy::kPosture1Max; }
int dkn_remaining(const DknSlot& s, const daB_TN_c* a) noexcept { return dkn::phase2(s.ph) ? dkn::res2_remaining(a->field_0x6fc) : s.hp1; }
int dkn_life_max(const DknSlot& s) noexcept { return dkn::phase2(s.ph) ? dkn::policy::kRes2Max : dkn::policy::kHp1Max; }
void dkn_log(const char* ev, const DknSlot& s, const daB_TN_c* a, const char* extra = "", bool warn = false) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "DARKNUT_%s actor=%u generation=%llu phase=%s hp1=%d/%d res2=%d/%d posture=%d/%d break=%s act=%d mode=%d frame=%.1f nbp=%d acc=%d %s tick=%llu", ev, s.id,
                static_cast<unsigned long long>(s.generation), dkn::name(s.ph), s.hp1, dkn::policy::kHp1Max, a ? dkn::res2_remaining(a->field_0x6fc) : -1, dkn::policy::kRes2Max, s.post.points(),
                s.post.max_pts, dkn::name(s.brk.st), a ? a->mActionMode1 : -1, a ? a->mActionMode2 : -1, a ? static_cast<double>(dkn_frame(a)) : -1.0, a ? a->mNextBreakPart : -1,
                a ? a->field_0x6fc : -1, extra, static_cast<unsigned long long>(dknb_tick()));
  dkn_line(warn, line);
}

// ---------------------------------------------------------------- writers (the ONLY native writes of this module)
// 1/12: the per-actor phase-2 threshold, written in the damage_check PRE: setDamage (called only from damage_check) reads it; action() rewrites 360 after.
DKN_NOINLINE void dkn_native_threshold(daB_TN_c* a, int v) noexcept { a->field_0x700 = v; }
// 2/12: the native setActionMode, replicated (offHeadLockFlg + the two modes; ci/dkn_native_audit.py pins that body).
DKN_NOINLINE void dkn_native_action(daB_TN_c* a, int act, int mode) noexcept {
  a->offHeadLockFlg();
  a->mActionMode1 = act;
  a->mActionMode2 = mode;
}
// 3/12: the armour: the native setBreakPart / setBreakHeadPart (sound + part physics) for a progress ABOVE what is broken, then mNextBreakPart.
DKN_NOINLINE void dkn_native_armor(daB_TN_c* a, int call, bool head, int floor) noexcept {
  if (call >= 0) {
    if (head) a->setBreakHeadPart(call);
    else a->setBreakPart(call);
  }
  a->mNextBreakPart = floor >= 12 ? 12 : dkn::armor_next(floor, dkn_broken_parts(a));
}
// 4/12: a native clip of the B_tn archive (setBck), its start frame, its rate.
DKN_NOINLINE void dkn_native_clip(daB_TN_c* a, int bck, float morph, float frame, float rate) noexcept {
  a->setBck(bck, 0, morph, rate);
  a->mpModelMorf2->setFrame(frame);
}
DKN_NOINLINE void dkn_native_wait_clip(daB_TN_c* a) noexcept { a->setBck(dkn::native::kBckTnbWait, 2, 10.0f, 1.0f); }
// 5/12: the J3D play rate of the body clip (the next play of action()).
DKN_NOINLINE void dkn_native_rate(daB_TN_c* a, float rate) noexcept { a->mpModelMorf2->mFrameCtrl.mRate = rate; }
// 6/12: the weapon / shield-kick AT of the open impact window (native setSwordAtBit for the weapon, the native mSphC bits for the shield and the kick).
DKN_NOINLINE void dkn_native_at(daB_TN_c* a, bool sword, int want) noexcept {
  if (sword) a->setSwordAtBit(want);
  else if (want) a->mSphC.OnAtSetBit();
  else a->mSphC.OffAtSetBit();
}
// 7/12: spl 1 (a LARGE attack: native guard slip) on the weapon and shield/kick colliders, never 9/10/11 (native guard break before Link's 100).
DKN_NOINLINE void dkn_native_spl(daB_TN_c* a) noexcept {
  for (int i = 0; i < 4; ++i) a->mSwordSphs[i].SetAtSpl(static_cast<dCcG_At_Spl>(dkn::native::kSplLarge));
  a->mCps.SetAtSpl(static_cast<dCcG_At_Spl>(dkn::native::kSplLarge));
  a->mSphC.SetAtSpl(static_cast<dCcG_At_Spl>(dkn::native::kSplLarge));
}
// 8/12: the stagger is ours: no native timed guard (mTimer5 kept > 0, mTimer10 0), no cut-flag guard, no early ACT_YOROKE exit (mCutJumpStatus 0).
DKN_NOINLINE void dkn_native_stagger_hold(daB_TN_c* a, bool entry) noexcept {
  if (entry) a->mTimer5 = 90;  // executeDamageH must never see mTimer5 0 (its native timed guard) during OUR stagger
  else if (a->mTimer5 < 2) a->mTimer5 = 2;
  a->mTimer10 = 0;
  a->mCutFlag = false;
  a->mCutJumpStatus = 0;
}
// 9/12: the recovered Darknut raises its guard: the native forced guard timer (damage_check / setBodyShield block what reaches the front) + ready flag.
DKN_NOINLINE void dkn_native_guard(daB_TN_c* a, unsigned ticks) noexcept {
  a->mTimer10 = static_cast<u8>(ticks > 255 ? 255 : ticks);
  a->field_0xa91 = true;
}
// 10/12: inside a combo move: no native B->A chain (mActionMode2Copy != 0), not guard-ready while attacking.
DKN_NOINLINE void dkn_native_attack_flags(daB_TN_c* a) noexcept {
  a->mActionMode2Copy = 1;
  a->field_0xa91 = false;
}
// 11/12: distance correction of a combo wind-up: forward speed along the facing, and the facing chased toward Link (bounded per tick).
DKN_NOINLINE void dkn_native_motion(daB_TN_c* a, float speed, std::int16_t facing) noexcept {
  a->speedF = speed;
  a->shape_angle.y = facing;
  a->current.angle.y = facing;
}
// 12/12: the native recovery wait after B1 (initChaseL(2): TNB_WAIT, 30 ticks), mTimer1.
DKN_NOINLINE void dkn_native_timer1(daB_TN_c* a, int t) noexcept { a->mTimer1 = t; }

// ---------------------------------------------------------------- moves and windows
bool dkn_move_now(const DknSlot& s, const daB_TN_c* a, dkn::Move& m) noexcept {
  using namespace dkn::native;
  if (s.combo && !s.tail) return m = s.plan.seg[s.seg].m, true;
  const int act = a->mActionMode1, mode = a->mActionMode2;
  if (s.special && (act == kActAttackShieldH || act == kActAttackShieldL)) return m = s.special_move, true;
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
    dkn_log("BREAK", s, a, "event=LATCH_MAX next=BREAK_PENDING");
  }
}
void dkn_link_gain(DknSlot& s, daB_TN_c* a, dkn::Move m, dkn::Outcome o) noexcept {
  const int pts = dkn::link_gain(m, o);
  if (pts <= 0) return;
  const DknGain r = dknb_link_gain(pts, o == dkn::Outcome::Block, s.id, s.generation, s.led.serial);
  char ex[200];
  std::snprintf(ex, sizeof(ex), "move=%s outcome=%s link_points=%d confirmed=%d link_before=%d.%03d link_after=%d.%03d saturated=%d", dkn::name(m), dkn::name(o), pts, r.confirmed ? 1 : 0,
                r.before / 1000, r.before % 1000, r.after / 1000, r.after % 1000, r.saturated ? 1 : 0);
  dkn_log("IMPACT", s, a, ex);
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
  const bool burst_follows = (m == dkn::Move::F2B && kidx == 0) || (s.combo && s.seg + 1 < s.plan.n && s.plan.seg[s.seg + 1].m == dkn::Move::F2A && m == dkn::Move::F2A);
  if (shield) {
    const DknPass pass = dknb_link_pass(k);
    if (pass.in_window) {
      ++s.st_parry;
      dknb_link_parry_confirmed(s.id, pass.serial);
      char ex[160];
      std::snprintf(ex, sizeof(ex), "move=%s impact=%u link_serial=%llu delta=%.2f link_gain=+0 combo=%s", dkn::name(m), kidx + 1, static_cast<unsigned long long>(pass.serial), pass.delta,
                    s.combo ? dkn::kCombos[s.combo_idx].name : "NONE");
      dkn_log("PARRY", s, a, ex);
      dkn_posture_gain(s, a, dkn::dk_parry_gain(m, kidx), "LINK_PARRY");  // a parry never ends the combo
      return;
    }
    ++s.st_block_link;
    dknb_link_blocked(s.id, burst_follows);
    dkn_link_gain(s, a, m, dkn::Outcome::Block);
    return;
  }
  ++s.st_direct_link;
  dknb_direct_offer(dkn_index(&s), s.id, s.generation, s.led.serial, sph);
  char ex[96];
  std::snprintf(ex, sizeof(ex), "move=%s impact=%u outcome=DIRECT_OFFERED (credited on Link's native damage receipt)", dkn::name(m), kidx + 1);
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
    const int pts = dkn::link_gain(m, dkn::Outcome::Direct);
    const DknGain r = dknb_link_gain(pts, false, s.id, s.generation, token);
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "DARKNUT_IMPACT actor=%u generation=%llu move=%s outcome=DIRECT link_points=%d confirmed=%d link_after=%d.%03d saturated=%d tick=%llu", s.id,
                  static_cast<unsigned long long>(s.generation), dkn::name(m), pts, r.confirmed ? 1 : 0, r.after / 1000, r.after % 1000, r.saturated ? 1 : 0, static_cast<unsigned long long>(now));
    dkn_line(false, line);
  }
}

// ---------------------------------------------------------------- combos and specials
void dkn_start_move(DknSlot& s, daB_TN_c* a, unsigned seg, float frame, bool first) noexcept {
  const dkn::Seg& sg = s.plan.seg[seg];
  const dkn::MoveInfo& mi = dkn::info(sg.m);
  s.seg = seg;
  dkn_native_action(a, mi.act, mi.mode);
  dkn_native_clip(a, mi.bck, first ? 3.0f : mi.morph, frame, sg.rate_in);
  dkn_native_attack_flags(a);
  s.led.close();
}
void dkn_combo_end(DknSlot& s, daB_TN_c* a, const char* why, bool cancel) noexcept {
  if (!s.combo) return;
  char ex[200];
  std::snprintf(ex, sizeof(ex), "combo=%s reason=%s impacts=%u/%u moves=%u/%u ticks=%llu", dkn::kCombos[s.combo_idx].name, why, s.impacts, dkn::kCombos[s.combo_idx].n_impacts, s.seg + 1,
                s.plan.n, static_cast<unsigned long long>(dknb_tick() - s.combo_tick));
  s.combo = false, s.tail = false;
  s.sel.ended(dknb_tick());
  if (cancel) {
    ++s.st_combo_cancel;
    dkn_native_at(a, true, 0);
    dkn_native_at(a, false, 0);
    dkn_native_rate(a, 1.0f);
  }
  s.led.close();
  dkn_log(cancel ? "COMBO_CANCEL" : "COMBO_END", s, a, ex);
}
bool dkn_combo_start(DknSlot& s, daB_TN_c* a, daAlink_c* k, unsigned which) noexcept {
  const unsigned idx = (dkn::phase2(s.ph) ? 4u : 0u) + which;
  const dkn::Combo& c = dkn::kCombos[idx];
  if (!k || dkn_dist(a, k) > dkn::policy::kComboLeaveDist || !dknb_world_safe()) return false;
  s.combo = true, s.tail = false, s.combo_idx = idx, s.seg = 0, s.ct = 0, s.impacts = 0, s.combo_tick = dknb_tick();
  s.plan = dn.plans[idx], s.tl = dn.tls[idx];
  ++s.st_combos;
  dkn_start_move(s, a, 0, s.plan.seg[0].start, true);
  char ex[160];
  std::snprintf(ex, sizeof(ex), "combo=%s impacts=%u moves=%u distance=%.0f share_pct=%d(PROVISIONAL) pick=25/25/25/25", c.name, c.n_impacts, c.n_moves, dkn_dist(a, k), dkn::policy::kComboPct);
  dkn_log("COMBO_START", s, a, ex);
  return true;
}
// executeAttack* POST of a combo move: impacts, the switch to the next move, the rate that keeps the timeline, distance correction.
void dkn_combo_post(DknSlot& s, daB_TN_c* a, daAlink_c* k) noexcept {
  using namespace dkn;
  if (s.tail) {  // the last move plays to its native end at its own rate
    const MoveInfo& mi = info(s.plan.seg[s.seg].m);
    if (a->mActionMode1 != mi.act) dkn_combo_end(s, a, "NATIVE_END", false);
    return;
  }
  const Seg& cur = s.plan.seg[s.seg];
  const MoveInfo& mi = info(cur.m);
  if (a->mActionMode1 != mi.act) {  // something native took the actor (should not happen inside a combo): end at once, never fight it
    dkn_combo_end(s, a, "ACTION_LOST", true);
    return;
  }
  // impacts of the previous play (bookkeeping against the recording)
  const float f = dkn_frame(a);
  unsigned seen = 0;
  for (unsigned i = 0; i < s.seg; ++i) seen += info(s.plan.seg[i].m).impacts;
  for (unsigned kx = 0; kx < mi.impacts; ++kx)
    if (f >= mi.impact[kx] && s.impacts < seen + kx + 1 && s.impacts < 9) {
      s.impact_tick[s.impacts] = dknb_tick();
      const Combo& c = kCombos[s.combo_idx];
      double err = 0;
      if (s.impacts > 0) err = static_cast<double>(s.impact_tick[s.impacts] - s.impact_tick[s.impacts - 1]) - (c.peak_ms[s.impacts] - c.peak_ms[s.impacts - 1]) * 0.03;
      char ex[200];
      std::snprintf(ex, sizeof(ex), "combo=%s impact=%u/%u move=%s sub=%u audio_interval_ms=%d interval_error_ticks=%+.2f", c.name, s.impacts + 1, c.n_impacts, name(cur.m), kx + 1,
                    s.impacts ? c.peak_ms[s.impacts] - c.peak_ms[s.impacts - 1] : 0, err);
      dkn_log("COMBO_SWING", s, a, ex);
      ++s.impacts;
    }
  unsigned ts = 0;
  float want = 0;
  const bool on = locate(s.plan, s.tl, static_cast<float>(s.ct + 1), ts, want);
  float now_frame = f;
  if (on && ts != s.seg) {
    if (!k || dkn_dist(a, k) > policy::kComboLeaveDist || !dknb_world_safe() || dknb_link_broken()) {
      dkn_combo_end(s, a, !k ? "NO_TARGET" : dknb_link_broken() ? "LINK_GUARD_BROKEN" : !dknb_world_safe() ? "EVENT" : "LINK_FAR", true);
      dkn_native_action(a, s.ph == Ph::Armor ? native::kActChaseH : native::kActChaseL, 0);
      return;
    }
    now_frame = switch_frame(s.plan.seg[ts], static_cast<float>(s.ct) - s.tl.T[ts]);
    dkn_start_move(s, a, ts, now_frame, false);
  }
  if (!on) {
    const Seg& last = s.plan.seg[s.plan.n - 1];
    if (last.m == Move::F2B1) {  // B1: after the first cut only, a natural recovery into the native guard wait (initChaseL(2)): no second cut, no AT
      dkn_native_at(a, true, 0);
      dkn_native_action(a, native::kActChaseL, 2);
      dkn_native_wait_clip(a);
      dkn_native_timer1(a, 30);
      dkn_combo_end(s, a, "B1_RECOVERY", false);
      return;
    }
    s.tail = true;
    dkn_native_rate(a, 1.0f);
    return;
  }
  float rate = want - now_frame;
  rate = rate < 0 ? 0 : rate > kTickRateMax ? kTickRateMax : rate;
  dkn_native_rate(a, rate);
  dkn_native_attack_flags(a);
  // wind-up only: face Link (bounded) and close the distance the impact needs
  const Seg& sg = s.plan.seg[s.seg];
  const MoveInfo& m2 = info(sg.m);
  float sp = 0.0f;
  std::int16_t facing = a->shape_angle.y;
  if (k && now_frame < m2.impact[0]) {
    std::int16_t diff = static_cast<std::int16_t>(dkn_angle_to(a, k) - a->shape_angle.y);
    const std::int16_t step = 0x500;
    if (diff > step) diff = step;
    if (diff < -step) diff = -step;
    facing = static_cast<std::int16_t>(a->shape_angle.y + diff);
    const float d = dkn_dist(a, k), left = (m2.impact[0] - now_frame) / (rate > 0.2f ? rate : 0.2f);
    sp = d > m2.reach ? (d - m2.reach) / (left > 1 ? left : 1) : 0.0f;
    if (sp > 18.0f) sp = 18.0f;
  }
  dkn_native_motion(a, sp, facing);
  ++s.ct;
}
// A passivity special (shield bash / kick): ONE native move, never a combo, never the native shield->mace / kick->attack chain afterwards.
void dkn_special_post(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn::native;
  const int act = a->mActionMode1;
  if (act == kActAttackShieldH || act == kActAttackShieldL) return;
  s.special = false;
  if (act == kActAttackH || act == kActAttackL) {  // the native chain just started at the clip end: back to the native AI instead
    ++s.st_chain_cut;
    dkn_native_action(a, act == kActAttackH ? kActChaseH : kActChaseL, 0);
  }
  dkn_log("SPECIAL_END", s, a, act == kActAttackH || act == kActAttackL ? "native_chain=CUT back=NATIVE_AI" : "back=NATIVE_AI");
}

// ---------------------------------------------------------------- admission and phases
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
      s.post.reset(a->mActionMode1 >= dkn::native::kActChaseL ? dkn::policy::kPosture2Max : dkn::policy::kPosture1Max);
      s.ph = a->mActionMode1 >= dkn::native::kActChaseL ? dkn::Ph::Light : dkn::Ph::Armor;  // a Darknut admitted already in its second phase (mod loaded late)
      s.sel.rng.seed((static_cast<std::uint64_t>(id) << 20) ^ s.generation ^ dknb_tick());
      s.last_acc = a->field_0x6fc;
      ++dn.admitted;
      dkn_log("CREATE", s, a, "eligibility=B_TN_000001FF_mType1_mType2_0_switch_ff max_hp1=420 max_res2=480 posture_max=120/130");
      return &s;
    }
  ++dn.full;
  return nullptr;
}
void dkn_phase_track(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn;
  const int act = a->mActionMode1;
  const dkr::Phase dp = dknb_dkr_phase(s.id);
  if (s.ph == Ph::Armor && act == native::kActChangeDemo) {
    s.ph = Ph::Transform, s.brk.abort(BkEnd::Phase);
    if (s.combo) dkn_combo_end(s, a, "PHASE_CHANGE", false);
    s.special = false;
  }
  if (s.ph == Ph::Transform && act >= native::kActChaseL && act != native::kActEnding) {
    s.ph = Ph::Light;
    s.post.reset(policy::kPosture2Max);
    s.brk.abort(BkEnd::Phase);
    s.last_acc = a->field_0x6fc;
    dkn_log("PHASE_TRANSITION", s, a, "event=PHASE2_START posture=0/130 res2=480/480 weapon=LIGHT_SWORD armour_never_rebuilt=1");
  }
  if (act == native::kActEnding && (s.ph == Ph::Light || s.ph == Ph::Revived)) {
    const bool second = s.ph == Ph::Revived || dp == dkr::Phase::Consumed;
    s.ph = second ? Ph::Dead : Ph::Dkr;
    s.brk.abort(BkEnd::Death);
    if (s.combo) dkn_combo_end(s, a, "DEFEAT", false);
    s.special = false;
    dkn_log(second ? "SECOND_DEATH" : "DKR01_FALL", s, a, second ? "owner=NATIVE_executeZakoEnding definitive=1" : "owner=DKR01 combos=OFF parry=OFF posture=FROZEN");
  }
  if (s.ph == Ph::Dkr && act != native::kActEnding) {  // DKR01 handed the actor back (end of the FIX02 rise): second phase, full resistance, posture 0/130
    s.ph = Ph::Revived;
    s.post.reset(policy::kPosture2Max);
    s.last_acc = a->field_0x6fc;
    char ex[96];
    std::snprintf(ex, sizeof(ex), "event=REVIVE_RESET clip=FIX02 dkr_phase=%s res2=%d/480 posture=0/130", dkr::name(dp), res2_remaining(a->field_0x6fc));
    dkn_log("THRESHOLD", s, a, ex);
  }
}
bool dkn_stagger_action(const DknSlot& s, const daB_TN_c* a) noexcept {
  return a->mActionMode1 == (dkn::phase2(s.ph) ? dkn::native::kActYoroke : dkn::native::kActDamageH);
}
void dkn_begin_break(DknSlot& s, daB_TN_c* a) noexcept {
  using namespace dkn;
  if (s.combo) dkn_combo_end(s, a, "POSTURE_BREAK", true);
  s.special = false;
  dkn_native_at(a, true, 0);
  dkn_native_at(a, false, 0);
  dkn_native_stagger_hold(a, true);
  // phase 1: the native first-phase damage reaction (TNA_DAMAGE_L, voice, facing); phase 2: the native ACT_YOROKE (TNB_DOWN_SIPPU). Never DAMAGE_LAST.
  dkn_native_action(a, phase2(s.ph) ? native::kActYoroke : native::kActDamageH, 0);
  s.brk.begin();
  ++s.st_breaks;
  dkn_log("BREAK", s, a, phase2(s.ph) ? "event=STAGGER_START clip=TNB_DOWN_SIPPU_0x1B window=90_or_3_hits" : "event=STAGGER_START clip=TNA_DAMAGE_L_0x09 window=90_or_3_hits");
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
  daAlink_c* k = dknb_link();
  if (s.brk.st == Bk::Pending) {
    const int act = a->mActionMode1;
    if (act == native::kActChangeDemo || act == native::kActEnding) s.brk.abort(BkEnd::Phase);
    else dkn_begin_break(s, a);
  }
  if (s.brk.st == Bk::Stagger) dkn_native_stagger_hold(a, false);
  if (s.combo && !s.tail && (!k || !dknb_link_alive())) {
    dkn_combo_end(s, a, "NO_TARGET", true);
    dkn_native_action(a, s.ph == Ph::Armor ? native::kActChaseH : native::kActChaseL, 0);
  }
  if (s.combo && s.ct > policy::kComboWatchdogTicks) {
    dkn_combo_end(s, a, "WATCHDOG", true);
    dkn_native_action(a, s.ph == Ph::Armor ? native::kActChaseH : native::kActChaseL, 0);
  }
  // Link's action key (blocked-attack dedupe) and passivity
  if (k) {
    const int lp = dknb_link_proc(k), lc = k->getCutType(), ln = k->getCutCount();
    if (classify(lp) == Act::Active && (lp != s.link_prev_proc || lc != s.link_prev_cut || ln != s.link_prev_count)) ++s.link_key;  // a new Link action
    s.link_prev_proc = lp, s.link_prev_cut = lc, s.link_prev_count = ln;
    const int act = a->mActionMode1, mode = a->mActionMode2;
    const bool attacking = act == native::kActAttackH || act == native::kActAttackShieldH || act == native::kActAttackL || act == native::kActAttackShieldL;
    const bool dk_busy = s.combo || s.special || s.brk.active() || attacking;
    const Pv pv = s.pas.step(classify(dknb_link_proc(k)), dknb_link_guarding(k), dk_busy);
    if (pv != Pv::None && s.pas.ready(dknb_tick()) && !dk_busy && dknb_world_safe() && dknb_link_alive()) {
      const bool p2 = phase2(s.ph);
      const bool chase = p2 ? (act == native::kActChaseL && (mode <= 5 || mode == 16)) : (act == native::kActChaseH && mode < 100);
      const bool shield_ok = p2 || a->mStates[12] == 0;  // phase 1: the bash needs the shield (part 12 breaks at armour progress 11)
      const float reach = p2 ? policy::kPassiveReach2 : policy::kPassiveReach1;
      const float d = dkn_dist(a, k);
      if (chase && shield_ok && d < reach && dkn_front(a, k)) {
        s.pas.fire(dknb_tick());
        s.special = true, s.special_tick = dknb_tick(), s.special_move = p2 ? Move::F2C : Move::F1Shield;
        ++s.st_specials;
        dkn_native_action(a, p2 ? native::kActAttackShieldL : native::kActAttackShieldH, 0);
        char ex[160];
        std::snprintf(ex, sizeof(ex), "kind=%s move=%s distance=%.0f cooldown_ticks=%u(PROVISIONAL)", pv == Pv::Shield ? "SHIELD_HELD_38" : "NO_SHIELD_75", name(s.special_move), d,
                      policy::kPassiveCooldownTicks);
        dkn_log("PASSIVE_DETECT", s, a, ex);
        dkn_log("SPECIAL_START", s, a, ex);
      }
    }
  }
  return HOOK_CONTINUE;
}
void on_dkn_action_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return;
  DknSlot& s = *sp;
  using namespace dkn;
  const int act = a->mActionMode1;
  if (act == native::kActChangeDemo || act == native::kActEnding) return;  // the native transformation / DKR01 own the actor
  dkn_native_spl(a);
  // the stagger: one eligible tick, the clip held across the window, then the natural recovery and the raised guard
  if (s.brk.st == Bk::Stagger) {
    const float end = phase2(s.ph) ? native::kEndTnbDownSippu : native::kEndTnaDamageL;
    if (!dkn_stagger_action(s, a)) {  // the native clip ended early (should not happen while held): recover now
      s.brk.st = Bk::Recover, s.brk.why = BkEnd::Timeout;
    } else if (s.brk.tick()) {
      dkn_log("GUARD_RECOVER", s, a, "event=RECOVERY_START reason=TIMEOUT_90");
    } else {
      dkn_native_rate(a, stagger_rate(s.brk.ticks, dkn_frame(a), end));
    }
  }
  if (s.brk.st == Bk::Recover) {
    const bool done = !dkn_stagger_action(s, a);
    if (!done) dkn_native_rate(a, kRecoverRate);
    if (s.brk.recover(done)) {
      if (!done) dkn_native_action(a, phase2(s.ph) ? native::kActChaseL : native::kActChaseH, 0);
      dkn_native_guard(a, policy::kGuardRecoverTicks);
      s.post.reset(dkn_max(s));
      char ex[96];
      std::snprintf(ex, sizeof(ex), "event=RECOVERED reason=%s posture=0/%d guard_ticks=%u", name(s.brk.why), dkn_max(s), policy::kGuardRecoverTicks);
      dkn_log("GUARD_RECOVER", s, a, ex);
    }
  }
  // regeneration: only in plain combat (never during a stagger, the transformation, the fall, the Finish, the rise or death)
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
  dkn_at_authority(s, a);
}

// ---------------------------------------------------------------- hooks: damage_check
HookAction on_dkn_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return HOOK_CONTINUE;
  DknSlot& s = *sp;
  s.pre_act1 = a->mActionMode1, s.pre_act2 = a->mActionMode2, s.sd_seen = false;
  if (dkn::phase2(s.ph)) dkn_native_threshold(a, dkn::policy::kRes2Max);
  dkn_link_contact(s, a, dknb_link());
  return HOOK_CONTINUE;
}
void on_dkn_damage_post(ModContext*, void* args, void*, void*) noexcept {
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
  if (!changed) return;
  // Link's attack met the Darknut's defence: the native guard (blocked), or the native ACT_YOROKE of a strong cut on a guarding phase-2 Darknut,
  // which becomes a plain block (only the posture break may stagger him).
  const bool yoroke = act == native::kActYoroke && !(s.brk.active());
  const bool guard = act == native::kActGuardH || act == native::kActGuardL;
  if ((act == native::kActDamageH || act == native::kActDamageL) && !s.brk.active()) {  // a reaction without damage (hookshot pull): no ordinary stagger
    ++s.st_reverted;
    dkn_native_action(a, s.pre_act1, s.pre_act2);
    return;
  }
  if (!yoroke && !guard) return;
  daAlink_c* k = dknb_link();
  if (yoroke) ++s.st_yoroke_blocked;
  if (s.link_key != s.blocked_key) {
    s.blocked_key = s.link_key;
    ++s.st_blocked_by_dk;
    dkn_posture_gain(s, a, phase2(s.ph) ? policy::kGainBlocked2 : policy::kGainBlocked1, yoroke ? "LINK_BLOCKED_STRONG_CUT" : "LINK_BLOCKED");
  }
  if (s.combo || s.special || s.brk.active()) {  // a block never cuts a combo, a special or the stagger
    ++s.st_reverted;
    dkn_native_action(a, s.pre_act1, s.pre_act2);
  } else if (yoroke) {
    dkn_native_action(a, native::kActGuardL, 0);
  }
  (void)k;
}

// ---------------------------------------------------------------- hooks: setDamage (the native damage commit)
HookAction on_dkn_setdamage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return HOOK_CONTINUE;
  sp->sd_nbp = a->mNextBreakPart, sp->sd_acc = a->field_0x6fc, sp->sd_act1 = a->mActionMode1, sp->sd_act2 = a->mActionMode2, sp->sd_seen = true;
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
  const cCcD_Obj* o = a->mAtInfo.mpCollider;
  const bool sword = o && const_cast<cCcD_Obj*>(o)->ChkAtType(AT_TYPE_NORMAL_SWORD);
  const bool iron = o && const_cast<cCcD_Obj*>(o)->ChkAtType(AT_TYPE_IRON_BALL);
  const bool head_jump = dknb_link() && dknb_link()->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP;
  if (s.sd_act1 < native::kActChangeDemo) {  // ---- phase 1: the 420 HP reserve and the armour
    int dmg = 100 - a->health;
    if (dmg < 0) dmg = 0;
    s.hp1 -= dmg;
    if (s.hp1 < 0) s.hp1 = 0;
    ++s.st_hp1;
    const int prog = armor_progress(s.hp1);
    const int broken = dkn_broken_parts(a);
    const int call = armor_call(prog, broken);
    char ex[200];
    std::snprintf(ex, sizeof(ex), "damage=%d source=%s native_nbp_added=%d armor_progress=%d broken_before=%d", dmg, sword ? "SWORD" : iron ? "IRON_BALL" : "OTHER", a->mNextBreakPart - s.sd_nbp,
                  prog, broken);
    if (s.hp1 > 0) {
      if (call >= 0) ++s.st_armor_calls;
      dkn_native_armor(a, call, head_jump, prog > s.sd_nbp ? prog : s.sd_nbp);  // never the native +1/+2: our progress, never below what really broke
      if (call >= 0) {
        char ax[96];
        std::snprintf(ax, sizeof(ax), "progress=%d parts_broken=%d hp1=%d/420", a->mNextBreakPart, dkn_broken_parts(a), s.hp1);
        dkn_log("ARMOR_PROGRESS", s, a, ax);
      }
    }
    dkn_log("HP1_DAMAGE", s, a, ex);
    if (s.hp1 <= 0) {  // the reserve is exhausted: the native mType-1 transformation (executeZakoChangeDemo), with priority over everything
      if (s.combo) dkn_combo_end(s, a, "PHASE_CHANGE", true);
      s.special = false;
      s.brk.abort(BkEnd::Phase);
      dkn_native_armor(a, -1, false, 12);
      dkn_native_action(a, native::kActChangeDemo, 0);
      s.ph = Ph::Transform;
      dkn_log("PHASE_TRANSITION", s, a, "event=ACT_CHANGEDEMO executeZakoChangeDemo=1 armour=NATIVE_LAST_PARTS mace_throw=NATIVE sword_draw=NATIVE");
      return;
    }
  } else {  // ---- phase 2 / revived: the native accumulator against 480
    ++s.st_hp2;
    char ex[160];
    std::snprintf(ex, sizeof(ex), "added=%d acc=%d/480 remaining=%d source=%s lethal=%d", a->field_0x6fc - s.sd_acc, a->field_0x6fc, res2_remaining(a->field_0x6fc),
                  sword ? "SWORD" : iron ? "IRON_BALL" : "OTHER", a->mActionMode1 == native::kActEnding ? 1 : 0);
    dkn_log("HP2_DAMAGE", s, a, ex);
    if (a->mActionMode1 == native::kActEnding) {  // defeat: DKR01 (first) / the native death (second), whatever the stagger state
      if (s.combo) dkn_combo_end(s, a, "DEFEAT", true);
      s.special = false;
      s.brk.abort(BkEnd::Death);
      return;
    }
  }
  // Table A for an effective hit; the stagger counts unique effective hits (one per native setDamage: the native invulnerability dedupes frames)
  if (s.brk.st == Bk::Stagger) {
    ++s.st_punish;
    const bool rec = s.brk.hit();
    char ex[64];
    std::snprintf(ex, sizeof(ex), "hits=%u/3 recovery=%d", s.brk.hits, rec ? 1 : 0);
    dkn_log("PUNISH_HIT", s, a, ex);
  } else if (sword) {
    dkn_posture_gain(s, a, phase2(s.ph) ? policy::kGainSwordHp2 : policy::kGainSwordHp1, "LINK_SWORD_HP");
  } else if (iron) {
    dkn_posture_gain(s, a, phase2(s.ph) ? policy::kGainIron2 : policy::kGainIron1, "LINK_IRON_BALL");
  }
  // no ordinary stagger: the native ACT_DAMAGEH / ACT_DAMAGEL of this hit is taken back (hit mark, sounds, invulnerability and armour kept)
  if (a->mActionMode1 != s.sd_act1 || a->mActionMode2 != s.sd_act2) {
    ++s.st_reverted;
    dkn_native_action(a, s.sd_act1, s.sd_act2);
  }
}

// ---------------------------------------------------------------- hooks: the attacks (combo director and specials)
HookAction on_dkn_attack_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp) || sp->brk.active() || sp->combo || sp->special) return HOOK_CONTINUE;
  DknSlot& s = *sp;
  using namespace dkn::native;
  const int act = a->mActionMode1, mode = a->mActionMode2;
  const bool start = (act == kActAttackH && mode == 0) || (act == kActAttackL && (mode == 0 || mode == 10));
  if (!start) return HOOK_CONTINUE;
  const int k = s.sel.pick(dknb_tick());
  char ex[96];
  std::snprintf(ex, sizeof(ex), "native_mode=%d result=%s share_pct=%d(PROVISIONAL)", mode, k < 0 ? "NATIVE_SINGLE" : "COMBO", dkn::policy::kComboPct);
  dkn_log("COMBO_SELECT", s, a, ex);
  if (k >= 0) (void)dkn_combo_start(s, a, dknb_link(), static_cast<unsigned>(k));
  return HOOK_CONTINUE;
}
void on_dkn_attack_post(ModContext*, void* args, void*, void*) noexcept {
  if (!dn.enabled || !args) return;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return;
  DknSlot* sp = dkn_find(static_cast<std::uint32_t>(fopAcM_GetID(a)));
  if (!sp || !dkn_combat(*sp)) return;
  if (sp->combo) dkn_combo_post(*sp, a, dknb_link());
  else if (sp->special) dkn_special_post(*sp, a);
}

// ---------------------------------------------------------------- deletion, HUD, summary, install
HookAction on_dkn_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!args) return HOOK_CONTINUE;
  daB_TN_c* a = mods::arg<daB_TN_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  if (DknSlot* s = dkn_find(id)) {
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
  if (daAlink_c* link = dknb_link()) dknb_direct_capture(link);
}
void dkn_hud_samples() noexcept {
  if (!dn.enabled) return;
  unsigned slot = 0;
  for (DknSlot& s : dn.s) {
    if (!s.on || slot >= kDknHudSlots) continue;
    if (!dkn_combat(s)) continue;  // no bar while the transformation, the fall, the Finish or the rise own the actor
    fopAc_ac_c* p = nullptr;
    if (!fopAcM_SearchByID(s.id, &p) || !p || fopAcM_GetName(p) != fpcNm_B_TN_e) continue;
    bool busy = false;
    const int pct = s.post.max_pts > 0 ? s.post.points() * 100 / s.post.max_pts : 0;
    (void)dknb_hud(slot++, s.id, s.generation, pct, s.post.latched, false, s.brk.st == dkn::Bk::Stagger, static_cast<daB_TN_c*>(p), busy);
  }
}
void dkn_summary(const char* why) noexcept {
  std::uint64_t t[18] = {};
  for (const DknSlot& s : dn.s) {
    const std::uint64_t v[18] = {s.st_hp1, s.st_hp2, s.st_gain, s.st_parry, s.st_block_link, s.st_direct_link, s.st_blocked_by_dk, s.st_reverted, s.st_yoroke_blocked, s.st_breaks, s.st_punish,
                                 s.st_combos, s.st_combo_cancel, s.st_specials, s.st_chain_cut, s.st_impacts, s.st_armor_calls, s.st_regen_ticks};
    for (unsigned i = 0; i < 18; ++i) t[i] += v[i];
  }
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "DKN01_SUMMARY why=%s version=%s enabled=%d admitted=%llu ineligible=%llu full=%llu deletes=%llu hp1_hits=%llu hp2_hits=%llu posture_gains=%llu link_parries=%llu link_blocks=%llu "
                "link_direct=%llu dk_blocks=%llu reactions_reverted=%llu yoroke_to_block=%llu breaks=%llu punish_hits=%llu combos=%llu combo_cancels=%llu specials=%llu chains_cut=%llu impacts=%llu "
                "armor_calls=%llu regen_ticks=%llu unload_fixed=%llu dropped_lines=%llu",
                why, dkn::kVersion, dn.enabled ? 1 : 0, static_cast<unsigned long long>(dn.admitted), static_cast<unsigned long long>(dn.ineligible), static_cast<unsigned long long>(dn.full),
                static_cast<unsigned long long>(dn.deletes), static_cast<unsigned long long>(t[0]), static_cast<unsigned long long>(t[1]), static_cast<unsigned long long>(t[2]),
                static_cast<unsigned long long>(t[3]), static_cast<unsigned long long>(t[4]), static_cast<unsigned long long>(t[5]), static_cast<unsigned long long>(t[6]),
                static_cast<unsigned long long>(t[7]), static_cast<unsigned long long>(t[8]), static_cast<unsigned long long>(t[9]), static_cast<unsigned long long>(t[10]),
                static_cast<unsigned long long>(t[11]), static_cast<unsigned long long>(t[12]), static_cast<unsigned long long>(t[13]), static_cast<unsigned long long>(t[14]),
                static_cast<unsigned long long>(t[15]), static_cast<unsigned long long>(t[16]), static_cast<unsigned long long>(t[17]), static_cast<unsigned long long>(dn.unload_fixed),
                static_cast<unsigned long long>(dn.dropped));
  dknb_write(false, line);
}
// Unload (hooks already removed by the host): a Darknut held in a stretched stagger clip (rate down to 0) or mid-combo gets the native rate back, so the
// native clip ends and the native AI continues; found again by process id, never by a kept pointer.
void dkn_shutdown() noexcept {
  const bool was = dn.enabled;
  dn.enabled = false;
  for (DknSlot& s : dn.s) {
    if (!s.on || !(s.combo || s.special || s.brk.active())) continue;
    fopAc_ac_c* p = nullptr;
    if (fopAcM_SearchByID(s.id, &p) && p && fopAcM_GetName(p) == fpcNm_B_TN_e) {
      dkn_native_rate(static_cast<daB_TN_c*>(p), 1.0f);
      ++dn.unload_fixed;
    }
  }
  if (was) dkn_summary("SHUTDOWN");
}
void dkn_install() noexcept {
  for (unsigned i = 0; i < 8; ++i) dn.plans[i] = dkn::plan(dkn::kCombos[i]), dn.tls[i] = dkn::timeline(dn.plans[i]);
  HookOptions late = HOOK_OPTIONS_INIT;
  late.priority = -402;  // Link's damage receipt: after the FIX01 capture, the Lizalfos (-400) and the Chilfos (-401) receipts of the same call
  ModResult r[14];
  for (ModResult& x : r) x = MOD_ERROR;
  r[0] = mods::hook::add_pre<DknAction>(on_dkn_action_pre), r[1] = mods::hook::add_post<DknAction>(on_dkn_action_post);
  r[2] = mods::hook::add_pre<DknDamageCheck>(on_dkn_damage_pre), r[3] = mods::hook::add_post<DknDamageCheck>(on_dkn_damage_post);
  r[4] = mods::hook::add_pre<DknSetDamage>(on_dkn_setdamage_pre), r[5] = mods::hook::add_post<DknSetDamage>(on_dkn_setdamage_post);
  r[6] = mods::hook::add_pre<DknAttackH>(on_dkn_attack_pre), r[7] = mods::hook::add_post<DknAttackH>(on_dkn_attack_post);
  r[8] = mods::hook::add_pre<DknAttackL>(on_dkn_attack_pre), r[9] = mods::hook::add_post<DknAttackL>(on_dkn_attack_post);
  r[10] = mods::hook::add_post<DknShieldH>(on_dkn_attack_post), r[11] = mods::hook::add_post<DknShieldL>(on_dkn_attack_post);
  r[12] = mods::hook::add_pre<DkrDelete>(on_dkn_delete_pre), r[13] = mods::hook::add_post<LinkDamageFix01>(on_dkn_link_damage_post, &late);
  bool ok = true;
  for (ModResult x : r) ok = ok && x == MOD_OK;
  dn.hooks = ok;
  if (!ok) {
    (void)mods::hook::uninstall<DknAction>(), (void)mods::hook::uninstall<DknDamageCheck>(), (void)mods::hook::uninstall<DknSetDamage>();
    (void)mods::hook::uninstall<DknAttackH>(), (void)mods::hook::uninstall<DknAttackL>(), (void)mods::hook::uninstall<DknShieldH>(), (void)mods::hook::uninstall<DknShieldL>();
  }
  dn.enabled = dn.hooks && dknb_dkr_enabled();  // all or nothing, and only with DKR01 (its resurrection and its identity filter) installed
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "DKN01 event=INSTALL version=%s tag=%s enabled=%d hooks=%d dkr01=%d target=B_TN_param_000001ff(mType1,mType2_0,switch_ff) hp1=420 res2=480 posture=120/130 regen_delay=60 "
                "break=90_or_3_hits combos=8 impacts=46 combo_share_pct=%d(PROVISIONAL) combo_cooldown=%u(PROVISIONAL) passive=38/75 passive_cooldown=%u(PROVISIONAL) guard_after_break=%u(PROVISIONAL) "
                "spl=LARGE_1 revive_clip=FIX02",
                dkn::kVersion, dkn::kTag, dn.enabled ? 1 : 0, dn.hooks ? 1 : 0, dknb_dkr_enabled() ? 1 : 0, dkn::policy::kComboPct, dkn::policy::kComboCooldownTicks,
                dkn::policy::kPassiveCooldownTicks, dkn::policy::kGuardRecoverTicks);
  dknb_write(!dn.enabled, line);
}
#undef DKN_NOINLINE
