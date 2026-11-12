// LIGHT FADES TWICE 0.13.5 CHILFOS FIX05: production adapter of the Chilfos (E_KK) duelist. Included once in mod.cpp's anonymous namespace after
// src/kk_bridge.inc, and verbatim by tests/kk_world_lab.cpp. Decisions live in kk_core.hpp; this file reads native fields by value, calls the core
// from the native hooks and applies the result through the KK_NOINLINE writers below (each one the native value of a native field of THIS actor):
//   kk_native_hp          health + max health, once per admitted identity, only from the exact native 140/140 of daE_KK_c::create
//   kk_native_action      mActionMode/mMoveMode: the stagger entry/exit, the C3 backstep and throw, the rest wait, the combo's poise
//   kk_native_frame       morf frame: a combo clip's start frame, and the one-call frame snap of the crossing guarantee (restored in the POST)
//   kk_native_spear       field_0x67d around ONE damage_check call: the native spear-less damage branch for a hit the defense did not cover
//   kk_native_health      health 2 before that call for the decisive sword (the native `health <= 1` lethal branch), 0 only as a logged fallback
//   kk_native_close       field_0x67e = 1 (the native close value) in the Execute where a combo clip restarts
//   kk_native_wait        field_0x672 of the native wait (rest after an episode)
//   kk_native_back        field_0x76c = 0 so the native backstep arms its own escape timer
//   kk_native_eye         eyePos.y for ONE executeSpearThrow call of the C3 throw (the native melee cancel would drop the spear), restored in the POST.
//                         FIX04: parked kThrowEyeBelow under Link's feet (kk::throw_eye), never at -1e9: the native cancel casts its BG line from that eye
//   kk_native_shield      mCyl TgShield: the prepared parry, replacing the native mCutTypeCheck(0) auto-deflect
//   kk_native_headlock    HeadLock off: stagger entry/exit, and a Shield Attack outside the stagger (Helm Splitter is the finisher, not a shortcut)
// FIX01 0.13.1 (posture below 100 never staggers, FIX05 rule):
//   kk_native_resume      mActionMode/mMoveMode/current.angle.y back to their values before ONE damage_check call whose native spear-less reaction (7,1)
//                         came from a non-lethal sword contact; damage_check is the first call of action(), so executeDamage never starts
//   kk_native_tg_clear    mCyl Tg-hit clear: Link's Shield Attack on the body below the stagger (no native DRAWBACK), and a crossing swing re-contact
//   kk_native_bash_se     the native collision SE of that Shield Attack contact (startCollisionSE(Z2SE_HIT_SWORD, 0x28), as the native branch plays it)
//   kk_native_speed       speedF of the C3 backstep: the native 15 that executeBackWalk just wrote, scaled by the backstep clip rate;
//                         FIX02: also the native -70 recoil damage_check just wrote when ITS spear met Link's shield (-> 0), and the native frame-24 lunge
//                         (80) capped so the Chilfos stops kContactDist from Link instead of shoving him
// FIX03 0.13.3:
//   kk_native_mortal_draw Link's native procCutFinishInit(CUT_FINISH_PARAM_MORTAL_DRAW_B): ONLY inside the POST of a native checkCutAction that just chose an
//                         ordinary sword cut, ONLY with a broken admitted Chilfos in front and in reach (the native Mortal Draw finisher; it kills on contact)
//   (hook) daE_KK_c::mCutTypeCheck PRE, inside THIS Chilfos' damage_check only: param 2 (the native instant-kill branch) answers false while it is not
//                         broken, so a hidden skill deals its ordinary damage; param 0 (the native auto-deflect TgShield) answers false while it is broken
// FIX04 0.13.4:
//   kk_native_at_clear    the native dCcD_GObjInf::ClrAtHit (virtual, through the vtable: no import) of THIS Chilfos' own spear spheres, in its damage_check PRE:
//                         stale flags of spheres no pass registered, the echo of an attack that already had its contact, the extra spheres of one pass
//   kk_native_face        current.angle.y/shape_angle.y of a READY Chilfos in its native wait: the native guard's own turn toward Link (1/8, at most 0x500)
//   kk_native_action      also (6,0), the native guard, raised when Link starts a swing it is going to cover, and after a wound outside its own attacks
//   (hook) dCcS::ProcAtTgHitmark PRE (after the parry FX PRE): a hit mark of THIS Chilfos' spear on Link that its attack already had is skipped
//   (bridge) kkb_guard_mark: the native weak-branch guard spark for a block/parry whose native strong branch has none
// FIX05 0.13.5:
//   kk_native_bow         the stagger clip's J3DFrameCtrl rate and loop frame (mRate, mLoop: inline fields, no import): ONE play of DAMAGE_BOW across the
//                         window (jolt at the native rate, then stretched so its own upright end is reached at RECOVERED; loop frame = last frame)
//   kk_native_head        mHeadLockPos.y of a broken Chilfos whose native HeadLock (Shield Attack) is on: lifted to its own measured upright head height,
//                         never lowered; cc_set rewrites it every Execute (the Helm Splitter aim point of procCutHeadInit)
//   kk_native_eye         also: eyePos.y of a Chilfos the Mortal Draw just killed keeps its kill height while Link is still in that Mortal Draw
//                         (setBodyAngleXReadyAnime aims Link's torso at it), at most kMdPoseHold ticks; cc_set rewrites it every Execute
//   (no write) Link's Jump Attack is a rebound proc (procCutJump / procCutJumpLand run changeCutReverseProc), one swing serial per jump; the burst pair's
//                         whole gap feeds the shared fast shield re-arm (kkb_burst_gap), not only its skipped ticks
// FIX02 0.13.2 (the spear never breaks on a block):
//   kk_native_action      also (6,0), the native guard reaction, in place of the native spear-break reaction (7,0) a strong sword hit chose for a contact
//                         the Chilfos blocked or parried; damage_check is the first call of action(), so executeDamage (CLASH, field_0x67d = 1, -80) never runs
#if defined(_MSC_VER)
#define KK_NOINLINE __declspec(noinline)
#else
#define KK_NOINLINE __attribute__((noinline))
#endif
struct KkEp {
  bool active = false, counter = false, closed = false, window_open = false, window_seen = false;
  unsigned pattern = 0, act = 0, n = 0;
  kk::Plan plan{};
  std::uint64_t t0 = 0, back_until = 0, last_progress = 0;
  enum Ph : std::uint8_t { Melee = 0, Back, Throw, Restore } ph = Melee;
  int want_bck = -1;
  float want_rate = 1.0f;
  kk::K counter_kind = kk::K::D;
};
struct KkClip {
  int bck = -1;
  float last_read = -1.0f, true_frame = 0.0f;
  std::uint32_t fired = 0;
  bool tracked = false, snapped = false, throw_clip = false;
};
struct KkSlot {
  bool on = false, admitted = false, hp_set = false, dead = false, shield_on = false;
  std::uintptr_t key = 0;
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  kk::Excl excl = kk::Excl::None;
  kk::Posture p;
  kk::Defense def;
  kk::Break brk;
  kk::Rng rng;
  kk::SwingVerdict sv;
  KkEp ep;
  KkClip clip;
  kk::Stats st;
  std::uint64_t rest_until = 0, counter_until = 0, parry_pending_tick = 0, parry_serial = 0;
  bool counter_pending = false, parry_pending = false;
  // damage_check scope (PRE -> POST of one call)
  bool in_dc = false, conv = false, decisive = false, shield_atk = false, spear_credit_lock = false, iron_hit = false;
  std::uint8_t saved_67d = 0;
  int hp_before = 0, act_before = 0, mode_before = 0;
  std::int16_t heading_before = 0;
  // FIX02
  bool dec_ready_frontal = false, recoil_pre = false, spear_prev = true;
  float speed_before = 0;
  std::uint64_t parry_tick = 0, def_tick = 0;
  kk::Cc counter_block = kk::Cc::Action;
  bool motion_on = false;
  std::uint64_t motion_tick = 0;
  float mkx = 0, mkz = 0, mlx = 0, mlz = 0;
  const char* motion_kind = "NONE";
  // FIX03
  const char* guard_state = "NONE";
  bool md_active = false, md_extended = false, skill_suppressed = false;
  std::uint64_t md_tick = 0;
  int death_cut = -1;
  std::uint64_t sword_credit_serial = 0, bash_last_tick = 0, last_link_tick = 0;
  const char* last_link = "NONE";
  kk::V cur_v = kk::V::None;
  std::uint64_t cur_serial = 0;
  // FIX04 P1: the spear's AT windows (one native attack = one token) and what its contacts did
  bool cc_live = false, tok_contact = false;
  std::uint64_t at_token = 0, at_sounded = 0, at_marked = 0, tok_tick = 0, tok_close_at = 0;
  unsigned tok_spheres = 0, tok_rounds = 0, tok_echo = 0, tok_stale = 0, tok_marks = 0, tok_marks_cut = 0, tok_self = 0, tok_merged = 0;
  const char* tok_class = "NONE";
  // FIX04 P3: the guard roll of a swing, taken when Link starts it
  std::uint64_t raise_serial = 0;
  bool raise_ok = false;
  // FIX04 P0: the cost of the native wind-up Execute calls of the C3 throw
  std::int64_t thr_t0 = 0, thr_max = 0, thr_sum = 0;
  unsigned thr_calls = 0;
  float thr_len_max = 0;
  // FIX05
  bool bow_pending = false, bow_on = false, bow_stretched = false;
  float bow_end = 0, bow_last = -1.0f, bow_rate = 1.0f;
  unsigned bow_wraps = 0;
  float head_up = 0, head_native = 0, head_lifted = 0;
  bool head_logged = false, helm_seen = false;
  float helm_y0 = 0, helm_peak = 0;
  std::uint64_t rolled_serial = 0, block_credit_serial = 0;
  bool md_hold = false;
  float md_hold_y = 0, md_eye_min = 0;
  std::uint64_t md_hold_until = 0;
  unsigned md_hold_ticks = 0;
  std::int16_t md_body_min = 0, md_body_max = 0;
  // throw scope
  bool eye_saved = false;
  float eye_y = 0;
  std::uint8_t d_before = 0;
  unsigned spawns = 0;
  // impacts of the Chilfos on Link
  std::uint64_t imp_serial = 0, imp_resolved = 0;
  // trajectory probe of the current clip
  bool probe0 = false, probe1 = false;
  float tip0[3] = {}, tip1[3] = {};
};
struct KkPerf {
  static constexpr unsigned kRing = 128;
  std::int64_t last = 0, q0 = 0, ft0 = 0, mod_ticks = 0;
  double freq = 0.0;
  std::uint32_t ring[kRing] = {}, pre[kk::policy::kPerfFrames] = {}, post[kk::policy::kPerfFrames] = {};
  unsigned n = 0, pre_n = 0, post_n = 0, throw_id = 0, windows = 0;
  bool open = false;
};
struct KkRuntime {
  bool enabled = false;
  KkPerf perf;
  std::uint32_t child_ids[16] = {};
  unsigned child_n = 0;
  std::uint64_t spear_children = 0;
  KkSlot slots[kk::kSlots]{};
  kk::Identity ident;
  std::uint64_t generations = 0, overflow = 0, deleted = 0, excluded = 0, admitted = 0, swing_serial = 0, swing_tick = ~0ull, hud_shown = 0, hud_missing = 0, hud_busy = 0;
  std::uint64_t link_gain = 0, link_saturations = 0, reverse_kept = 0, reverse_suppressed = 0;
  std::uint32_t stage = 0;
  kk::Stats totals{};
  unsigned lines[18] = {};
  std::uint64_t dropped[18] = {};
  bool self_bck = false;  // a setBck this module issues itself: the PRE leaves its arguments alone
  bool jump_open = false;  // FIX05: the last swing was a Jump Attack takeoff (PROC_CUT_JUMP): its landing setCutType is the same swing
  std::uint64_t jump_serial = 0, jump_land_merged = 0;
  std::uint64_t maint_tick = ~0ull;
} kz;
enum KkLog : unsigned { kKkAdmit = 0, kKkCombo, kKkRhythm, kKkDefense, kKkLink, kKkSpear, kKkBreak, kKkFx, kKkAnim, kKkCritical, kKkPosture, kKkSummary, kKkPoise, kKkMotion, kKkExec, kKkPerf, kKkContact, kKkFix05 };
inline constexpr unsigned kKkCaps[18] = {64, 400, 120, 400, 300, 200, 260, 120, 60, 80, 300, 40, 300, 160, 200, 60, 300, 240};  // FIX05: its own budget (jump, stagger, helm, pose)
void kk_line(KkLog f, bool warn, const char* line) noexcept {
  if (kkb_take(kz.lines[f], kz.dropped[f], kKkCaps[f])) kkb_write(warn, line);
}
bool kk_alive() noexcept { return kz.enabled; }
void kk_break_trace(KkSlot& s, daE_KK_c* a, const char* what, int asked) noexcept;
void kk_md_track(KkSlot& s, daE_KK_c* a, std::uint64_t now) noexcept;
KkSlot* kk_find(const daE_KK_c* a) noexcept {
  const std::uintptr_t k = reinterpret_cast<std::uintptr_t>(a);
  for (auto& s : kz.slots)
    if (s.on && s.key == k) return &s;
  return nullptr;
}
void kk_merge(kk::Stats& t, const kk::Stats& x) noexcept { t.add(x); }
void kk_retire(KkSlot& s) noexcept {
  if (s.on && s.admitted) kk_merge(kz.totals, s.st);
  s = KkSlot{};
}
// ---------------------------------------------------------------- writers
KK_NOINLINE void kk_native_hp(daE_KK_c* a) noexcept { a->field_0x560 = kk::policy::kMaxHp, a->health = kk::policy::kMaxHp; }
KK_NOINLINE void kk_native_action(daE_KK_c* a, int act, int mode) noexcept { a->mActionMode = act, a->mMoveMode = mode; }
KK_NOINLINE void kk_native_frame(mDoExt_McaMorfSO* m, float f) noexcept { m->mFrameCtrl.mFrame = f; }
KK_NOINLINE void kk_native_spear(daE_KK_c* a, std::uint8_t v) noexcept { a->field_0x67d = v; }
KK_NOINLINE void kk_native_health(daE_KK_c* a, std::int16_t hp) noexcept { a->health = hp; }
KK_NOINLINE void kk_native_close(daE_KK_c* a) noexcept { a->field_0x67e = 1; }
KK_NOINLINE void kk_native_wait(daE_KK_c* a, std::int16_t t) noexcept { a->field_0x672 = t; }
KK_NOINLINE void kk_native_back(daE_KK_c* a) noexcept { a->field_0x76c = 0; }
KK_NOINLINE void kk_native_eye(daE_KK_c* a, float y) noexcept { a->eyePos.y = y; }
KK_NOINLINE void kk_native_shield(daE_KK_c* a, bool on) noexcept {
  if (on) a->mCyl.OnTgShield();
  else a->mCyl.OffTgShield();
}
KK_NOINLINE void kk_native_headlock(daE_KK_c* a) noexcept { a->offHeadLockFlg(); }
KK_NOINLINE void kk_native_resume(daE_KK_c* a, int act, int mode, std::int16_t heading) noexcept { a->mActionMode = act, a->mMoveMode = mode, a->current.angle.y = heading; }
KK_NOINLINE void kk_native_tg_clear(daE_KK_c* a) noexcept { a->mCyl.cCcD_ObjHitInf::ClrTgHit(); }
KK_NOINLINE void kk_native_bash_se(Z2Creature* c) noexcept { c->startCollisionSE(kk::policy::kSeHitSword, 0x28); }  // virtual: no new import
KK_NOINLINE void kk_native_speed(daE_KK_c* a, float v) noexcept { a->speedF = v; }
KK_NOINLINE int kk_native_mortal_draw(daAlink_c* k, int type) noexcept { return k->procCutFinishInit(type); }
KK_NOINLINE void kk_native_at_clear(dCcD_GObjInf* o) noexcept { o->ClrAtHit(); }
KK_NOINLINE void kk_native_face(daE_KK_c* a, std::int16_t h) noexcept { a->current.angle.y = h, a->shape_angle.y = h; }
KK_NOINLINE void kk_native_bow(mDoExt_McaMorfSO* m, float rate, std::int16_t loop) noexcept { m->mFrameCtrl.mRate = rate, m->mFrameCtrl.mLoop = loop; }
KK_NOINLINE void kk_native_head(daE_KK_c* a, float y) noexcept { a->getHeadLockPos().y = y; }
void kk_restart(KkSlot& s, daE_KK_c* a, int bck, float rate, float frame) noexcept {  // the native clip call, then its start frame
  kk_native_close(a);
  kz.self_bck = true;
  a->setBck(bck, 0, 3.0f, rate);
  kz.self_bck = false;
  if (a->mpMorfSO) kk_native_frame(a->mpMorfSO, frame);
  s.clip = KkClip{};
  s.clip.bck = bck, s.clip.last_read = frame, s.clip.tracked = true;
  s.probe0 = s.probe1 = false;
}
// ---------------------------------------------------------------- reads
float kk_frame(daE_KK_c* a) noexcept { return a->mpMorfSO ? a->mpMorfSO->getFrame() : 0.0f; }
bool kk_spear(daE_KK_c* a) noexcept { return a->field_0x67d == 0; }
int kk_link_angle(daE_KK_c* a, daAlink_c* k) noexcept {
  const float dx = k->current.pos.x - a->current.pos.x, dz = k->current.pos.z - a->current.pos.z;
  return int(cM_atan2s(dx, dz));
}
float kk_link_dist(daE_KK_c* a, daAlink_c* k) noexcept {
  const float dx = k->current.pos.x - a->current.pos.x, dz = k->current.pos.z - a->current.pos.z;
  return kk::fsqrt(dx * dx + dz * dz);
}
bool kk_frontal(daE_KK_c* a, daAlink_c* k) noexcept { return k && kk::angle_diff(a->shape_angle.y, kk_link_angle(a, k)) <= kk::policy::kFrontalArc; }
bool kk_attacking(daE_KK_c* a) noexcept { return a->mActionMode == kk::kAttack && a->field_0x67e == 0; }
bool kk_guard_ready(KkSlot& s, daE_KK_c* a) noexcept {
  // FIX02: an offensive episode is committed: the combo, its C3 backstep/throw/restore and the counter after a parry never guard or parry. A blocked swing no
  // longer ends a combo (0.13.1: a block in the gap between two hits handed the turn back); Link's hits land with the FIX01 anti-stagger (HP, +5, combo
  // goes on), an honest trade that feeds the posture break. The defense that holds against repeated B is the one between episodes.
  if (s.ep.active) return false;
  const int act = a->mActionMode;
  return kk_spear(a) && !s.brk.active() && act != kk::kYoroke && act != kk::kDamage && act != kk::kDead && act != kk::kIcicle && act != kk::kWeaponMove && !kk_attacking(a);
}
bool kk_link_has_rebound(daAlink_c* k) noexcept {  // the procs that run changeCutReverseProc (FIX05 LINK_COMBO_MAP + the Jump Attack)
  const int proc = static_cast<int>(k->mProcID);
  // FIX05 P1: procCutJump -> changeCutReverseProc(ANM_CUT_JUMP) (the aerial rebound: heading reversed, mNormalSpeed 27, POWER5 shock) and procCutJumpLand ->
  // changeCutReverseProc(ANM_CUT_RECOIL_B) (PROC_CUT_REVERSE with its recoil clip) are native rebound routes too (d_a_alink_cut.inc).
  return proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH || proc == daAlink_c::PROC_CUT_JUMP || proc == daAlink_c::PROC_CUT_JUMP_LAND;
}
bool kk_jump_proc(daAlink_c* k) noexcept { return k && (static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP || static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP_LAND); }
// ---------------------------------------------------------------- event lines
inline constexpr const char* kKkPhase[4] = {"MELEE", "BACKSTEP", "THROW", "RESTORE"};
void kk_log_ep_end(KkSlot& s, daE_KK_c* a, kk::End why, const char* origin) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_COMBO event=END actor=%u generation=%llu pattern=%u counter=%d cause=%s act=%u impacts=%llu tick=%llu posture=%d hp=%d action=%d origin=%s phase=%s mode=%d bck=%d "
                "frame=%.1f last_link=%s last_link_tick=%llu", s.id, static_cast<unsigned long long>(s.generation), s.ep.pattern + 1, s.ep.counter ? 1 : 0, kk::name(why), s.ep.act + 1,
                static_cast<unsigned long long>(s.st.impacts_open), static_cast<unsigned long long>(kkb_tick()), s.p.points(), a->health, a->mActionMode, origin, kKkPhase[s.ep.ph & 3], a->mMoveMode,
                a->field_0x764, a->mpMorfSO ? a->mpMorfSO->getFrame() : -1.0f, s.last_link, static_cast<unsigned long long>(s.last_link_tick));
  kk_line(kKkCombo, why != kk::End::Completed && why != kk::End::Posture100 && why != kk::End::Death && why != kk::End::Defense, line);
}
void kk_end_episode(KkSlot& s, daE_KK_c* a, kk::End why, const char* origin) noexcept {
  if (!s.ep.active) return;
  ++s.st.ended[unsigned(why)];
  if (why == kk::End::Completed && !s.ep.counter) ++s.st.completed[s.ep.pattern];
  kk_log_ep_end(s, a, why, origin);
  if (s.eye_saved) kk_native_eye(a, s.eye_y), s.eye_saved = false;
  s.ep = KkEp{};
  if (why != kk::End::Posture100 && why != kk::End::Death && why != kk::End::SceneOrDelete) s.rest_until = kkb_tick() + kk::policy::kRestMin + s.rng.below(kk::policy::kRestMax - kk::policy::kRestMin + 1);
}
void kk_posture_gain(KkSlot& s, daE_KK_c* a, int pts, const char* src, std::uint64_t latch_serial) noexcept {
  if (pts <= 0) return;
  const int before = s.p.points();
  const bool latched = s.p.gain(pts, kkb_tick());
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_POSTURE event=GAIN actor=%u generation=%llu source=%s gain=+%d before=%d after=%d hp=%d latched=%d tick=%llu", s.id, static_cast<unsigned long long>(s.generation), src, pts,
                before, s.p.points(), a->health, latched ? 1 : 0, static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkPosture, false, line);
  if (latched) {
    s.brk.latch(latch_serial);
    std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=LATCH_100 actor=%u generation=%llu source=%s swing=%llu hp=%d tick=%llu", s.id, static_cast<unsigned long long>(s.generation), src,
                  static_cast<unsigned long long>(latch_serial), a->health, static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkBreak, false, line);
  }
}
void kk_link_gain(KkSlot& s, int points, bool block, std::uint64_t token, const char* kind) noexcept {
  const KkGain o = kkb_link_gain(points, block, s.id, s.generation, token);
  if (!o.confirmed) return;
  ++kz.link_gain;
  if (o.saturated) ++kz.link_saturations;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_LINK event=POSTURE_GAIN actor=%u generation=%llu kind=%s points=%d before=%d.%03d after=%d.%03d saturated=%d tick=%llu", s.id, static_cast<unsigned long long>(s.generation),
                kind, points, o.before / 1000, o.before % 1000, o.after / 1000, o.after % 1000, o.saturated ? 1 : 0, static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkLink, false, line);
}
// ---------------------------------------------------------------- one verdict per swing of Link's sword on this Chilfos
kk::V kk_decide(KkSlot& s, daE_KK_c* a, daAlink_c* k, std::uint64_t serial) noexcept {
  const kk::V had = s.sv.of(serial);
  // A verdict cached before the stagger (the crossing swing's Damage/Block/parry) is never reused during it: that swing re-contacting after the native
  // 8-tick timer would otherwise run the spear-less damage branch (and could kill with the very swing that raised posture to 100) or the native guard.
  if (had != kk::V::None && (!s.brk.active() || had == kk::V::Decisive || had == kk::V::Ignored)) return had;
  const std::uint64_t now = kkb_tick();
  kk::V v = kk::V::Damage;
  const bool frontal = kk_frontal(a, k);
  s.guard_state = "NONE";
  if (s.brk.active()) v = s.brk.sword_finishes(serial) ? kk::V::Decisive : kk::V::Ignored;
  else if (s.shield_on && s.def.presenting(now, serial) && frontal && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;
  // FIX03: a guard that is really up (the native GUARD action) covers its front for sure; the pressure roll only decides whether a ready Chilfos guards
  else if (a->mActionMode == kk::kGuard && kk_guard_ready(s, a) && frontal) v = kk::V::Block, s.guard_state = "ACTIVE", ++s.st.guard_active_blocks;
  // FIX04: the roll of this swing was taken when Link started it (and raised the native guard if it succeeded): never a second chance at contact
  else if (s.raise_serial == serial && s.raise_serial != 0) {
    if (s.raise_ok && kk_guard_ready(s, a) && frontal) v = kk::V::Block, s.guard_state = "RAISED";
    else s.guard_state = s.raise_ok ? "RAISED_FLANKED" : "RAISE_FAILED";
  }
  else if (kk_guard_ready(s, a) && frontal && s.rng.pct(s.def.block_pct(now, serial))) v = kk::V::Block, s.guard_state = "ROLL";
  s.dec_ready_frontal = !s.brk.active() && kk_guard_ready(s, a) && frontal;
  if (v == kk::V::Block && s.def.guaranteed(now)) ++s.st.guard_guarantees, ++s.def.guarantees;
  s.sv.set(serial, v);
  ++s.st.verdict[unsigned(v)];
  if (k && k->getCutType() == daPy_py_c::CUT_TYPE_JUMP && !s.brk.active()) {  // FIX05 P1: what a Jump Attack met (once per jump: the verdict is per serial)
    ++(v == kk::V::EnemyParry ? s.st.jump_parries : v == kk::V::Block ? s.st.jump_blocks : s.st.jump_damage);
    char jl[kLogLineMax];
    std::snprintf(jl, sizeof(jl), "CHILFOS_JUMP event=VERDICT actor=%u generation=%llu swing=%llu cut=%d link_proc=%d phase=%s front=%d enemy_guard=%d enemy_parry=%d verdict=%s hp=%d posture=%d "
                  "action=%d streak=%u tick=%llu", s.id, static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(serial), k->getCutType(), static_cast<int>(k->mProcID),
                  static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP ? "AIR" : static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP_LAND ? "LAND" : "OTHER", frontal ? 1 : 0,
                  a->mActionMode == kk::kGuard ? 1 : 0, s.shield_on ? 1 : 0, kk::name(v), a->health, s.p.points(), a->mActionMode, s.def.jump_streak(now), static_cast<unsigned long long>(now));
    kk_line(kKkFix05, false, jl);
  }
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=VERDICT actor=%u generation=%llu swing=%llu verdict=%s presented=%d frontal=%d spear=%d ready=%d block_pct=%d wounds=%u hp=%d posture=%d action=%d tick=%llu "
                "pressure=%u uncovered=%u guarantee=%d guard_state=%s cut=%d", s.id,
                static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(serial), kk::name(v), s.shield_on ? 1 : 0, frontal ? 1 : 0, kk_spear(a) ? 1 : 0, kk_guard_ready(s, a) ? 1 : 0,
                s.def.block_pct(now), s.def.wound_streak, a->health, s.p.points(), a->mActionMode, static_cast<unsigned long long>(now), s.def.within(now, kk::policy::kSustainedWindow),
                s.def.uncovered, s.def.guaranteed(now) ? 1 : 0, s.guard_state, k ? k->getCutType() : -1);
  kk_line(kKkDefense, false, line);
  return v;
}
void kk_apply_enemy_parry(KkSlot& s, daE_KK_c* a) noexcept {
  const std::uint64_t now = kkb_tick();
  ++s.st.enemy_parries;
  s.def.on_enemy_parry(now);
  kk_link_gain(s, kk::policy::kLinkEnemyParry, false, s.parry_serial, "ENEMY_PARRY");
  s.counter_pending = true, s.counter_until = now + kk::policy::kCounterTtl, s.parry_tick = now, s.def_tick = now;
  if (s.ep.active) kk_end_episode(s, a, kk::End::Defense, "ENEMY_PARRY");
  s.rest_until = 0;
  if (a->mActionMode == kk::kWait) kk_native_wait(a, 0);  // a rest wait this module wrote must not delay the counter past its lifetime
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=ENEMY_PARRY actor=%u generation=%llu swing=%llu link_gain=+%d chilfos_gain=+0 rebound=NATIVE hp=%d posture=%d cooldown_until=%llu tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(s.parry_serial), kk::policy::kLinkEnemyParry, a->health, s.p.points(),
                static_cast<unsigned long long>(s.def.cooldown_until), static_cast<unsigned long long>(now));
  kk_line(kKkDefense, false, line);
}
// ---------------------------------------------------------------- FIX02 motion evidence: both actors at a contact, and again 8 ticks later
void kk_motion(KkSlot& s, daE_KK_c* a, const char* kind, float before, float after) noexcept {
  daAlink_c* k = kkb_link();
  if (!k) return;
  s.motion_on = true, s.motion_tick = kkb_tick(), s.motion_kind = kind;
  s.mkx = a->current.pos.x, s.mkz = a->current.pos.z, s.mlx = k->current.pos.x, s.mlz = k->current.pos.z;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_MOTION event=CONTACT actor=%u generation=%llu kind=%s dist=%.1f speed_before=%.1f speed_after=%.1f action=%d mode=%d link_proc=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), kind, kk_link_dist(a, k), before, after, a->mActionMode, a->mMoveMode, static_cast<int>(k->mProcID),
                static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkMotion, false, line);
}
void kk_motion_settle(KkSlot& s, daE_KK_c* a) noexcept {
  if (!s.motion_on || kkb_tick() < s.motion_tick + 8) return;
  s.motion_on = false;
  daAlink_c* k = kkb_link();
  if (!k) return;
  const float dk = kk::fsqrt((a->current.pos.x - s.mkx) * (a->current.pos.x - s.mkx) + (a->current.pos.z - s.mkz) * (a->current.pos.z - s.mkz));
  const float dl = kk::fsqrt((k->current.pos.x - s.mlx) * (k->current.pos.x - s.mlx) + (k->current.pos.z - s.mlz) * (k->current.pos.z - s.mlz));
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_MOTION event=SETTLE actor=%u generation=%llu kind=%s ticks=8 moved_chilfos=%.1f moved_link=%.1f dist=%.1f speed=%.1f action=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), s.motion_kind, dk, dl, kk_link_dist(a, k), a->speedF, a->mActionMode, static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkMotion, false, line);
}
// FIX02: why a pending counter cannot start this tick (Cc::Count = it can)
kk::Cc kk_counter_blocker(KkSlot& s, daE_KK_c* a, daAlink_c* k) noexcept {
  if (s.dead || a->mActionMode == kk::kDead) return kk::Cc::Dead;
  if (s.brk.active() || s.brk.st == kk::Bk::Pending) return kk::Cc::Broken;
  if (!kk_spear(a)) return kk::Cc::NoSpear;
  if (!kkb_world_safe()) return kk::Cc::Event;
  if (!k || kk_link_dist(a, k) > kk::policy::kComboLeaveDist) return kk::Cc::LinkFar;
  const int act = a->mActionMode;
  if (act != kk::kWait && act != kk::kGuard && act != kk::kWalk && act != kk::kAttack) return kk::Cc::Action;
  return kk::Cc::Count;
}
void kk_counter_cancel(KkSlot& s, kk::Cc why) noexcept {
  s.counter_pending = false;
  ++s.st.counter_cancel[unsigned(why)], ++s.st.counter_expired;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=COUNTER_CANCELLED actor=%u generation=%llu reason=%s waited=%llu tick=%llu", s.id, static_cast<unsigned long long>(s.generation), kk::name(why),
                static_cast<unsigned long long>(kkb_tick() - s.parry_tick), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkDefense, true, line);
}
// ---------------------------------------------------------------- the Chilfos' impact on Link (its own AT spheres): parry / block / direct, once per window
void kk_link_contact(KkSlot& s, daE_KK_c* a, daAlink_c* k) noexcept {
  if (!k || s.imp_serial == s.imp_resolved) return;
  bool hit = false, shield = false;
  const void* sph = nullptr;
  auto look = [&](dCcD_Sph& x) noexcept {
    if (!x.cCcD_ObjHitInf::ChkAtHit()) return;
    const cCcD_Obj* o = x.cCcD_ObjHitInf::GetAtHitObj();
    for (int i = 0; i < 3; ++i)
      if (o == static_cast<const cCcD_Obj*>(&k->mTgCyls[i])) {
        hit = true, shield = shield || x.ChkAtShieldHit() != 0;
        if (!sph) sph = static_cast<const void*>(&x);
      }
  };
  look(a->mSph);
  for (int i = 0; i < 6; ++i) look(a->mSpheres[i]);
  if (!hit) return;
  s.imp_resolved = s.imp_serial;
  const KkPass pass = shield ? kkb_link_pass(k) : KkPass{};
  const bool parry = shield && pass.in_window;
  const bool burst_follows = s.ep.active && s.ep.act + 1 < s.ep.n && s.ep.plan.a[s.ep.act + 1].burst;
  char line[kLogLineMax];
  if (parry) {
    ++s.st.link_parry;
    kkb_link_parry_confirmed(s.id, pass.serial);
    kkb_pfx_settle(true, s.id, true);
    kk_posture_gain(s, a, kk::policy::kGainLinkParry, "LINK_PARRY", kz.swing_serial);
    std::snprintf(line, sizeof(line), "CHILFOS_LINK event=PARRY actor=%u generation=%llu pattern=%u act=%u impact=%llu gain=+%d posture=%d link_serial=%llu delta=%.2f burst_next=%d tick=%llu", s.id,
                  static_cast<unsigned long long>(s.generation), s.ep.active ? s.ep.pattern + 1 : 0u, s.ep.act + 1, static_cast<unsigned long long>(s.imp_serial), kk::policy::kGainLinkParry, s.p.points(),
                  static_cast<unsigned long long>(pass.serial), pass.delta, burst_follows ? 1 : 0, static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkLink, false, line);
    return;
  }
  kkb_pfx_settle(true, s.id, false);
  if (shield) {
    ++s.st.link_block;
    kkb_link_blocked(s.id, burst_follows);
    kk_link_gain(s, kk::policy::kLinkBlock, true, s.imp_serial, "BLOCK");
    return;
  }
  ++s.st.link_direct_offered;
  kkb_direct_offer(unsigned(&s - kz.slots), s.id, s.generation, s.imp_serial, sph);
}
// ---------------------------------------------------------------- FIX04 P1: one native contact round per spear attack
dCcD_GObjInf* kk_sphere(daE_KK_c* a, unsigned i) noexcept { return i == 0 ? static_cast<dCcD_GObjInf*>(&a->mSph) : static_cast<dCcD_GObjInf*>(&a->mSpheres[i - 1]); }
unsigned kk_sphere_bit(daE_KK_c* a, const cCcD_Obj* o) noexcept {
  for (unsigned i = 0; i < 7; ++i)
    if (o == static_cast<const cCcD_Obj*>(kk_sphere(a, i))) return 1u << i;
  return 0;
}
void kk_spear_masks(daE_KK_c* a, unsigned& any, unsigned& shield) noexcept {
  any = shield = 0;
  for (unsigned i = 0; i < 7; ++i) {
    dCcD_GObjInf* x = kk_sphere(a, i);
    if (!x->cCcD_ObjHitInf::ChkAtHit()) continue;
    any |= 1u << i;
    if (x->ChkAtShieldHit()) shield |= 1u << i;
  }
}
void kk_token_open(KkSlot& s) noexcept {
  ++s.at_token, ++s.st.contact_tokens;
  s.tok_contact = false, s.tok_tick = kkb_tick(), s.tok_spheres = s.tok_rounds = s.tok_echo = s.tok_stale = s.tok_marks = s.tok_marks_cut = s.tok_self = s.tok_merged = 0;
  s.tok_class = "NONE";
}
void kk_token_close(KkSlot& s, daE_KK_c* a) noexcept {  // one bounded line per attack that touched Link (CHILFOS_CONTACT), none for a miss
  s.tok_close_at = 0;
  if (!s.tok_contact) return;
  s.tok_contact = false;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_CONTACT event=ATTACK actor=%u generation=%llu token=%llu impact=%llu class=%s spheres=0x%02x native_rounds=%u self_se=%u merged=%u echo_suppressed=%u stale_cleared=%u "
                "marks=%u marks_suppressed=%u window_ticks=%llu action=%d bck=%d tick=%llu", s.id, static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(s.at_token),
                static_cast<unsigned long long>(s.imp_serial), s.tok_class, s.tok_spheres, s.tok_rounds, s.tok_self, s.tok_merged, s.tok_echo, s.tok_stale, s.tok_marks, s.tok_marks_cut,
                static_cast<unsigned long long>(kkb_tick() - s.tok_tick), a->mActionMode, a->field_0x764, static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkContact, s.tok_rounds + s.tok_self > 1, line);
}
// The flags this damage_check is about to read: stale ones cleared, an attack's echo cleared, one sphere left for the native round of a new contact.
kk::RoundOut kk_spear_round(KkSlot& s, daE_KK_c* a) noexcept {
  unsigned any = 0, shield = 0;
  kk_spear_masks(a, any, shield);
  const kk::RoundOut o = kk::spear_round(any, shield, s.cc_live, a->mDamageTimer == 0, s.at_token, s.at_sounded);
  if (o.r == kk::Round::Stale) {
    ++s.st.stale_cleared, ++s.tok_stale;
  } else if (any) {
    s.tok_contact = true, s.tok_spheres |= any;
    s.tok_class = shield ? "SHIELD" : "BODY";
  }
  switch (o.r) {
    case kk::Round::Echo: ++s.st.echo_suppressed, ++s.tok_echo; break;
    case kk::Round::Keep:
      s.at_sounded = s.at_token, ++s.tok_rounds;
      if (kk::bits(shield) > 1) ++s.st.se_merged, ++s.tok_merged;
      break;
    case kk::Round::Self: s.at_sounded = s.at_token; break;
    default: break;
  }
  return o;
}
void kk_round_apply(KkSlot& s, daE_KK_c* a, const kk::RoundOut& o) noexcept {
  for (unsigned i = 0; i < 7; ++i)
    if (o.clear & (1u << i)) kk_native_at_clear(kk_sphere(a, i));
  if (o.r == kk::Round::Self) {  // the native timer of an older event would mute this new attack's first clash, then replay it 8 ticks late: it sounds now, once
    kk_native_bash_se(&a->mCreatureSound);
    ++s.st.se_self, ++s.tok_self;
  }
}
// dCcS::ProcAtTgHitmark PRE, after the parry FX PRE: the spark of a contact of this Chilfos' spear on Link, once per attack (the first pass keeps its spark).
HookAction on_kk_hitmark_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args || !mods::arg<bool>(args, 1) || !mods::arg<bool>(args, 2)) return HOOK_CONTINUE;
  dCcD_GObjInf* ati = mods::arg<dCcD_GObjInf*>(args, 5);
  dCcD_GObjInf* tgi = mods::arg<dCcD_GObjInf*>(args, 6);
  const cCcD_Obj* at = mods::arg<cCcD_Obj*>(args, 3);
  if (!ati || !tgi || !at) return HOOK_CONTINUE;
  fopAc_ac_c* aa = ati->GetAc();
  if (!aa || fopAcM_GetName(aa) != fpcNm_E_KK_e) return HOOK_CONTINUE;  // every other mark of the game: two comparisons
  auto* a = static_cast<daE_KK_c*>(aa);
  KkSlot* s = kk_find(a);
  daAlink_c* k = kkb_link();
  if (!s || !s->admitted || !k || tgi->GetAc() != static_cast<fopAc_ac_c*>(k) || !kk_sphere_bit(a, at)) return HOOK_CONTINUE;
  if (s->at_marked != s->at_token) {
    s->at_marked = s->at_token, ++s->st.marks_kept, ++s->tok_marks;
    return HOOK_CONTINUE;
  }
  ++s->st.marks_suppressed, ++s->tok_marks_cut;
  return HOOK_SKIP_ORIGINAL;
}
// ---------------------------------------------------------------- hooks: Execute
void kk_admit(KkSlot& s, daE_KK_c* a) noexcept {
  kk::IdIn in;
  in.profile_ok = fopAcM_GetName(a) == fpcNm_E_KK_e;
  in.sub = a->field_0x679;
  in.hidden = a->field_0x679 == 2 && a->attention_info.distances[fopAc_attn_BATTLE_e] == 0;
  s.excl = kk::classify(in);
  if (s.excl == kk::Excl::HiddenIronBall) return;  // asked again next Execute: it joins the fight later
  s.admitted = s.excl == kk::Excl::None;
  char line[kLogLineMax];
  if (!s.admitted) {
    ++kz.excluded;
    std::snprintf(line, sizeof(line), "CHILFOS event=EXCLUDED reason=%s actor=%u sub=%d hp=%d action=%d native=UNCHANGED", kk::name(s.excl), s.id, in.sub, a->health, a->mActionMode);
    kk_line(kKkAdmit, false, line);
    return;
  }
  ++kz.admitted;
  std::snprintf(line, sizeof(line), "CHILFOS event=ADMITTED version=%s tag=%s actor=%u generation=%llu sub=%d hp=%d max=%d action=%d diagonal_bck=%d thrust_bck=%d identity=%s", kk::kVersion, kk::kTag, s.id,
                static_cast<unsigned long long>(s.generation), in.sub, a->health, a->field_0x560, a->mActionMode, kz.ident.diagonal(), kz.ident.thrust(),
                kz.ident.verified ? (kz.ident.swapped ? "MEASURED_SWAPPED" : "MEASURED_DEFAULT") : "DEFAULT_UNVERIFIED");
  kk_line(kKkAdmit, false, line);
}
void kk_enter_break(KkSlot& s, daE_KK_c* a, const char* origin) noexcept {
  if (s.ep.active) kk_end_episode(s, a, kk::End::Posture100, origin);
  s.counter_pending = false, s.def.prepared = false, s.parry_pending = false;
  s.brk.enter(kkb_tick());
  ++s.st.breaks;
  kk_native_shield(a, false), s.shield_on = false;
  kk_native_headlock(a);
  kk_native_action(a, kk::kYoroke, 0);  // executeYoroke mode 0: DAMAGE_BOW (with spear) or DAMAGE_BOWB, looped by the setBck PRE while the stagger lasts
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=STAGGER actor=%u generation=%llu window=%u hp=%d spear=%d latch_swing=%llu tick=%llu origin=%s", s.id, static_cast<unsigned long long>(s.generation),
                kk::policy::kBreakTicks, a->health, kk_spear(a) ? 1 : 0, static_cast<unsigned long long>(s.brk.latch_serial), static_cast<unsigned long long>(kkb_tick()), origin);
  kk_line(kKkBreak, false, line);
}
bool kk_try_break(KkSlot& s, daE_KK_c* a, const char* origin) noexcept {  // posture 100 latched: the native stagger now, whatever native reaction was chosen
  if (s.brk.st != kk::Bk::Pending || a->health <= 1 || a->mActionMode == kk::kDead || a->mActionMode == kk::kIcicle || a->mActionMode == kk::kWeaponMove) return false;
  kk_enter_break(s, a, origin);
  return true;
}
void kk_exit_break(KkSlot& s, daE_KK_c* a) noexcept {
  ++s.st.recovered;
  if (s.md_active) {
    s.md_active = false, ++s.st.md_cancelled;
    char ml[kLogLineMax];
    std::snprintf(ml, sizeof(ml), "CHILFOS_EXEC event=CANCELLED actor=%u generation=%llu reason=WINDOW_ENDED tick=%llu", s.id, static_cast<unsigned long long>(s.generation),
                  static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkExec, true, ml);
  }
  s.md_extended = false;
  const bool spear = kk_spear(a);
  // FIX05 P0: where the one play of the stagger clip stands when the window ends (its last frame = the native upright stance)
  const float bow_f = kk_frame(a), bow_end = s.bow_end;
  const bool at_end = s.bow_on && bow_end > 0 && bow_f >= bow_end - 1.5f;
  if (at_end) ++s.st.bow_exit_end;
  s.bow_on = s.bow_pending = false;
  s.brk.reset();
  s.p.reset();
  kk_native_headlock(a);
  kk_native_action(a, spear ? kk::kWait : kk::kBackWalk, 0);  // without its spear the native backstep restores it
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=RECOVERED actor=%u generation=%llu hp=%d posture=0 spear=%d next_action=%d tick=%llu clip_frame=%.1f clip_end=%.0f at_end=%d wraps=%u "
                "bow_rate=%.2f", s.id, static_cast<unsigned long long>(s.generation), a->health, spear ? 1 : 0, a->mActionMode, static_cast<unsigned long long>(kkb_tick()), bow_f, bow_end, at_end ? 1 : 0,
                s.bow_wraps, s.bow_rate);
  kk_line(kKkBreak, false, line);
}
void kk_maintenance() noexcept;
void kk_tick_maintenance() noexcept {  // once per simulation tick, from this module's own hooks (mod_update's frozen body is not touched)
  if (kz.maint_tick == kkb_tick()) return;
  kz.maint_tick = kkb_tick();
  kk_maintenance();
}
// FIX03: thrown spear children are counted once each (a small ring of recent ids), the first one of a session is logged
void kk_note_spear_child(std::uint32_t id) noexcept {
  for (auto x : kz.child_ids)
    if (x == id) return;
  kz.child_ids[kz.child_n++ % 16] = id;
  ++kz.spear_children;
  if (kz.spear_children == 1) {
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS event=EXCLUDED reason=SPEAR_CHILD actor=%u native=UNCHANGED slot=NONE note=later_children_counted_in_summary", id);
    kk_line(kKkAdmit, false, line);
  }
}
inline double kk_ms(const KkPerf& f, double ticks) noexcept { return f.freq > 0.0 ? ticks * 1000.0 / f.freq : -1.0; }
void kk_perf_stats(const std::uint32_t* v, unsigned n, double& avg, std::uint32_t& p50, std::uint32_t& p95, std::uint32_t& p99, std::uint32_t& mx) noexcept {
  std::uint32_t tmp[kk::policy::kPerfFrames];
  avg = 0, p50 = p95 = p99 = mx = 0;
  if (!n) return;
  for (unsigned i = 0; i < n; ++i) tmp[i] = v[i], avg += v[i];
  avg /= n;
  for (unsigned i = 1; i < n; ++i)  // insertion sort: at most 90 values, once per throw
    for (unsigned j = i; j > 0 && tmp[j - 1] > tmp[j]; --j) std::swap(tmp[j - 1], tmp[j]);
  p50 = tmp[n / 2], p95 = tmp[(n * 95) / 100 < n ? (n * 95) / 100 : n - 1], p99 = tmp[(n * 99) / 100 < n ? (n * 99) / 100 : n - 1], mx = tmp[n - 1];
}
void kk_perf_open() noexcept {
  KkPerf& f = kz.perf;
  if (f.open) return;
  f.open = true, f.post_n = 0, f.mod_ticks = 0, ++f.throw_id;
  const unsigned have = f.n < KkPerf::kRing ? f.n : KkPerf::kRing;
  f.pre_n = have < kk::policy::kPerfFrames ? have : kk::policy::kPerfFrames;
  for (unsigned i = 0; i < f.pre_n; ++i) f.pre[i] = f.ring[(f.n - f.pre_n + i) % KkPerf::kRing];
}
// FIX04: the frame-pacing window opens when the throw starts (the wind-up is where 0.13.3 lost its frames; 0.13.3 opened it at the release)
void kk_throw_begin(KkSlot& s) noexcept {
  s.thr_max = s.thr_sum = 0, s.thr_calls = 0, s.thr_len_max = 0, s.thr_t0 = 0;
  kk_perf_open();
}
void kk_perf_close() noexcept {
  KkPerf& f = kz.perf;
  f.open = false;
  ++f.windows;
  double a0, a1;
  std::uint32_t m0, q0, r0, x0, m1, q1, r1, x1;
  kk_perf_stats(f.pre, f.pre_n, a0, m0, q0, r0, x0);
  kk_perf_stats(f.post, f.post_n, a1, m1, q1, r1, x1);
  unsigned slow = 0;
  for (unsigned i = 0; i < f.post_n; ++i) slow += f.post[i] * 2 > m0 * 3 ? 1u : 0u;  // frames over 1.5 x the median frame before the release
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_PERF event=THROW_WINDOW throw=%u frames_pre=%u frames_post=%u pre_avg_ms=%.2f pre_p50_ms=%.2f pre_p95_ms=%.2f post_avg_ms=%.2f post_p50_ms=%.2f "
                "post_p95_ms=%.2f post_p99_ms=%.2f post_max_ms=%.2f slow_frames=%u mod_hooks_ms=%.3f clock=%s anchor=THROW_START sampler=METER_DRAW_NOT_PRESENT gpu=GPU_NOT_MEASURED", f.throw_id, f.pre_n, f.post_n, kk_ms(f, a0), kk_ms(f, m0),
                kk_ms(f, q0), kk_ms(f, a1), kk_ms(f, m1), kk_ms(f, q1), kk_ms(f, r1), kk_ms(f, x1), slow, kk_ms(f, double(f.mod_ticks)), f.freq > 0.0 ? "QPC_CALIBRATED" : "UNCALIBRATED");
  kk_line(kKkPerf, false, line);
}
struct KkPerfScope {  // the module's own time inside one hook call, only while a throw window is open
  std::int64_t t0;
  KkPerfScope() noexcept : t0(kz.perf.open ? kkb_qpc() : 0) {}
  ~KkPerfScope() { if (t0) kz.perf.mod_ticks += kkb_qpc() - t0; }
};
HookAction on_kk_exec_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  KkPerfScope ps;
  kk_tick_maintenance();
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  KkSlot* s = kk_find(a);
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  if (s && s->id != id) kk_retire(*s), s = nullptr;  // address reused by a new process: nothing is inherited
  if (!s && a->field_0x679 == 1 && fopAcM_GetName(a) == fpcNm_E_KK_e) {  // FIX03: a thrown spear child never takes a slot (no state, no log per child)
    kk_note_spear_child(id);
    return HOOK_CONTINUE;
  }
  if (!s) {
    for (auto& x : kz.slots)
      if (!x.on) {
        s = &x;
        break;
      }
    if (!s) {
      ++kz.overflow;
      return HOOK_CONTINUE;
    }
    *s = KkSlot{};
    s->on = true, s->key = reinterpret_cast<std::uintptr_t>(a), s->id = id, s->generation = ++kz.generations;
    s->rng.seed(kkb_tick() ^ (std::uint64_t(id) << 32) ^ (kz.generations << 20));
  }
  if (!s->admitted) {
    if (s->excl == kk::Excl::None) kk_admit(*s, a);
    else if (s->excl == kk::Excl::HiddenIronBall) kk_admit(*s, a);
    if (!s->admitted) return HOOK_CONTINUE;
  }
  const std::uint64_t now = kkb_tick();
  char line[kLogLineMax];
  if (!s->hp_set) {
    s->hp_set = true;
    if (a->health == kk::policy::kNativeHp && a->field_0x560 == kk::policy::kNativeHp) {
      kk_native_hp(a);
      std::snprintf(line, sizeof(line), "CHILFOS event=HP_SET actor=%u generation=%llu hp=%d from=%d/%d once=1", s->id, static_cast<unsigned long long>(s->generation), a->health, kk::policy::kNativeHp, kk::policy::kNativeHp);
    } else {
      std::snprintf(line, sizeof(line), "CHILFOS event=HP_FOREIGN actor=%u generation=%llu hp=%d max=%d kept=1", s->id, static_cast<unsigned long long>(s->generation), a->health, a->field_0x560);
    }
    kk_line(kKkAdmit, false, line);
  }
  kkb_pfx_begin(a, s->id);
  if (a->mActionMode == kk::kDead) {
    if (!s->dead) {
      s->dead = true;
      if (s->ep.active) kk_end_episode(*s, a, kk::End::Death, "EXECUTE_PRE_DEAD");
      s->brk.st = s->brk.st == kk::Bk::Broken ? kk::Bk::Dead : s->brk.st;
    }
    return HOOK_CONTINUE;
  }
  if (!kkb_world_safe()) {
    if (s->ep.active) kk_end_episode(*s, a, kk::End::Event, "EXECUTE_PRE_EVENT");
    return HOOK_CONTINUE;
  }
  // a prepared parry of the last tick whose rebound Link's clip confirmed (or not) after this actor's damage_check ran
  if (s->parry_pending && now > s->parry_pending_tick) {
    s->parry_pending = false;
    if (s->sv.rebound_seen && s->sv.serial == s->parry_serial) kk_apply_enemy_parry(*s, a);
    else {
      ++s->st.downgraded_parries;
      kk_posture_gain(*s, a, kk::policy::kGainBlock, "BLOCK_NO_REBOUND", s->parry_serial);
      s->def.on_block(now, s->parry_serial);
    }
  }
  s->def.expire(now);
  // FIX02: a confirmed parry's counter is dispatched through the native attack entry (8,0) once the parry contact had kCounterDelay ticks; the
  // native executeAttack then picks the clip, which the setBck PRE turns into the counter's D or E. Every opportunity ends STARTED or CANCELLED(reason).
  if (s->counter_pending && !s->ep.active) {
    const kk::Cc why = kk_counter_blocker(*s, a, kkb_link());
    if (why != kk::Cc::Count) s->counter_block = why;
    if (now > s->counter_until) kk_counter_cancel(*s, s->counter_block);
    else if (why == kk::Cc::Count && now >= s->parry_tick + kk::policy::kCounterDelay && a->mActionMode != kk::kAttack) {
      kk_native_action(a, kk::kAttack, 0);
      ++s->st.counter_dispatched;
      std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=COUNTER_DISPATCH actor=%u generation=%llu after=%llu tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                    static_cast<unsigned long long>(now - s->parry_tick), static_cast<unsigned long long>(now));
      kk_line(kKkDefense, false, line);
    }
  }
  // FIX02 invariant: a spear lost right after a contact the Chilfos blocked or parried is a defect (must stay 0); the C3 throw is intentional
  {
    const bool spear = kk_spear(a);
    if (s->spear_prev && !spear && !s->dead && !(s->ep.active && (s->ep.ph == KkEp::Throw || s->ep.ph == KkEp::Restore))) {
      const bool after_defense = s->def_tick != 0 && now - s->def_tick <= 2;
      ++(after_defense ? s->st.blocked_spear_loss : s->st.other_spear_loss);
      std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=LOST actor=%u generation=%llu after_block_or_parry=%d last_link=%s action=%d mode=%d tick=%llu", s->id,
                    static_cast<unsigned long long>(s->generation), after_defense ? 1 : 0, s->last_link, a->mActionMode, a->mMoveMode, static_cast<unsigned long long>(now));
      kk_line(kKkSpear, after_defense, line);
    }
    s->spear_prev = spear;
  }
  // stagger
  if (s->md_active) kk_md_track(*s, a, now);
  if (!kk_try_break(*s, a, "EXECUTE_PRE") && s->brk.expired(now)) {
    // FIX03: a Mortal Draw that really started on this broken Chilfos may finish its swing (at most kMdExtendMax ticks more), nothing else extends it
    if (s->md_active && now < s->brk.start + kk::policy::kBreakTicks + kk::policy::kMdExtendMax) {
      if (!s->md_extended) {
        s->md_extended = true, ++s->st.md_extended;
        std::snprintf(line, sizeof(line), "CHILFOS_EXEC event=WINDOW_EXTENDED actor=%u generation=%llu max=%u tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                      kk::policy::kMdExtendMax, static_cast<unsigned long long>(now));
        kk_line(kKkExec, false, line);
      }
    } else {
      kk_exit_break(*s, a);
    }
  }
  // posture regeneration
  const int act = a->mActionMode;
  const bool eligible = !s->brk.active() && act != kk::kYoroke && act != kk::kDamage && act != kk::kDead;
  if (eligible && !s->p.latched && s->p.u > 0) ++s->st.regen_ticks;
  s->p.tick(a->health, eligible);
  // the episode against the native state
  if (s->ep.active) {
    if (now - s->ep.last_progress > kk::policy::kWatchdogTicks) {
      ++s->st.watchdog;
      kk_end_episode(*s, a, kk::End::Watchdog, "EXECUTE_PRE_WATCHDOG");
    } else if (s->ep.ph == KkEp::Melee && act != kk::kAttack) {
      kk_end_episode(*s, a, s->ep.closed && s->ep.act + 1 >= s->ep.n ? kk::End::Completed : kk::End::NativePriority, "EXECUTE_PRE_ACTION_CHANGED");
    } else if (s->ep.ph == KkEp::Back) {
      if (act != kk::kBackWalk) kk_end_episode(*s, a, kk::End::NativePriority, "EXECUTE_PRE_BACKSTEP_ACTION_CHANGED");
      else if (now >= s->ep.back_until) {  // the planned backstep is over: face Link and throw (executeSpearThrow mode 1)
        s->ep.ph = KkEp::Throw, s->ep.last_progress = now;
        kk_native_action(a, kk::kThrow, 1);
  kk_throw_begin(*s);
        kk_throw_begin(*s);
        ++s->st.throws;
        std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=THROW_START actor=%u generation=%llu reason=PLANNED back_mode=%d bck=%d frame=%.1f speed=%.2f tick=%llu", s->id,
                      static_cast<unsigned long long>(s->generation), a->mMoveMode, a->field_0x764, kk_frame(a), a->speedF, static_cast<unsigned long long>(now));
        kk_line(kKkSpear, false, line);
      }
    } else if ((s->ep.ph == KkEp::Throw || s->ep.ph == KkEp::Restore) && act != kk::kThrow) {
      kk_end_episode(*s, a, s->ep.ph == KkEp::Restore && kk_spear(a) ? kk::End::Completed : kk::End::NativePriority, "EXECUTE_PRE_THROW_ACTION_CHANGED");
    }
  }
  // FIX04 P3: a ready Chilfos in its native wait watches Link (the native guard's turn); its own attacks, the stagger and events keep their native heading
  if (!s->dead && a->mActionMode == kk::kWait && kk_guard_ready(*s, a) && kkb_world_safe()) {
    daAlink_c* k = kkb_link();
    if (k && kk_link_dist(a, k) <= kk::policy::kFaceRange) {
      const std::int16_t h = kk::turn_toward(a->shape_angle.y, static_cast<std::int16_t>(kk_link_angle(a, k)), kk::policy::kFaceScale, kk::policy::kFaceStep);
      if (h != a->shape_angle.y) kk_native_face(a, h), ++s->st.face_ticks;
    }
  }
  return HOOK_CONTINUE;
}
// ---------------------------------------------------------------- FIX05: after this Chilfos' whole Execute (action, mtx_set, cc_set already ran)
bool kk_burst_partner(const KkSlot& s, unsigned i) noexcept {  // act i is the second thrust of an E-E burst recorded closer than kBurstMax
  return i > 0 && i < s.ep.n && s.ep.plan.a[i].burst && s.ep.plan.a[i - 1].burst && s.ep.plan.a[i].recorded_s < kk::policy::kBurstMax;
}
void kk_fix05_post(KkSlot& s, daE_KK_c* a) noexcept {
  const std::uint64_t now = kkb_tick();
  char line[kLogLineMax];
  // P0: the one play of the stagger clip. The setBck PRE marked a new stagger clip; the native setAnm has run (mEnd = its length, loop frame 0).
  mDoExt_McaMorfSO* m = a->mpMorfSO;
  if (m && s.brk.active() && (a->field_0x764 == kk::policy::kBckDamageBow || a->field_0x764 == kk::policy::kBckDamageBowB)) {
    const float f = m->getFrame();
    if (s.bow_pending) {
      s.bow_pending = false, s.bow_on = true, s.bow_stretched = false, s.bow_wraps = 0, s.bow_last = f;
      s.bow_end = m->getEndFrame(), s.bow_rate = 1.0f;
      kk_native_bow(m, 1.0f, kk::bow_loop(static_cast<std::int16_t>(s.bow_end)));
      ++s.st.bow_plans;
      std::snprintf(line, sizeof(line), "CHILFOS_STAGGER event=PLAN actor=%u generation=%llu bck=%d end=%.0f jolt=%.1f left=%llu loop=%d tick=%llu", s.id, static_cast<unsigned long long>(s.generation),
                    a->field_0x764, s.bow_end, kk::bow_jolt(s.bow_end), static_cast<unsigned long long>(s.brk.start + kk::policy::kBreakTicks - now), int(kk::bow_loop(static_cast<std::int16_t>(s.bow_end))),
                    static_cast<unsigned long long>(now));
      kk_line(kKkFix05, false, line);
    } else if (s.bow_on) {
      if (f + 1.5f < s.bow_last) ++s.bow_wraps, ++s.st.bow_wraps;  // a jump back toward frame 0 without a new clip: must never happen
      s.bow_last = f;
      const std::uint64_t end_tick = s.brk.start + kk::policy::kBreakTicks;
      if (!s.bow_stretched && f >= kk::bow_jolt(s.bow_end) && now < end_tick) {
        s.bow_stretched = true, s.bow_rate = kk::bow_rate(f, s.bow_end, end_tick - now > 1 ? end_tick - now - 1 : 1);  // plays left: the exit runs in the PRE of end_tick
        kk_native_bow(m, s.bow_rate, kk::bow_loop(static_cast<std::int16_t>(s.bow_end)));
        ++s.st.bow_stretches;
      }
    }
  }
  // P2: between the first and the second thrust of a burst pair the shared fast shield re-arm is fed every tick (0.13.4: only on skipped gap ticks, which a
  // gap-0 burst never has: 0 parries of 21 second thrusts in the 0.13.4 log). It still needs a parry of the first thrust and a NEW real press of Link.
  if (s.ep.active && s.ep.ph == KkEp::Melee && !s.ep.counter && !s.ep.window_open &&
      ((s.ep.closed && kk_burst_partner(s, s.ep.act + 1)) || (!s.ep.window_seen && kk_burst_partner(s, s.ep.act)))) {
    kkb_burst_gap(s.id);
    ++s.st.burst_gap_ticks;
  }
  // P3: the upright head-lock height of THIS actor, sampled in its stance; lifted for the Helm Splitter while broken with the native HeadLock on
  if (!s.brk.active() && !s.dead && (a->mActionMode == kk::kWait || a->mActionMode == kk::kWalk || a->mActionMode == kk::kGuard)) {
    const float rel = a->getHeadLockPos().y - a->current.pos.y;
    if (kk::head_sample_ok(rel)) {
      if (s.head_up == 0.0f) ++s.st.head_samples;
      s.head_up = rel;
    }
  }
  if (s.brk.active() && !s.dead && a->checkHeadLockFlg()) {
    const float native = a->getHeadLockPos().y, y = kk::head_lock_y(native, a->current.pos.y, s.head_up);
    if (y > native) {
      kk_native_head(a, y);
      if (!s.head_logged) {
        s.head_logged = true, ++s.st.head_lifts, s.head_native = native - a->current.pos.y, s.head_lifted = y - a->current.pos.y;
        daAlink_c* k = kkb_link();
        std::snprintf(line, sizeof(line), "CHILFOS_HELM event=AIM_LIFTED actor=%u generation=%llu native_rel=%.1f lifted_rel=%.1f upright_rel=%.1f link_rel=%.1f bck=%d frame=%.1f tick=%llu", s.id,
                      static_cast<unsigned long long>(s.generation), s.head_native, s.head_lifted, s.head_up, k ? k->current.pos.y - a->current.pos.y : 0.0f, a->field_0x764, kk_frame(a),
                      static_cast<unsigned long long>(now));
        kk_line(kKkFix05, false, line);
      }
    }
  }
  if (s.brk.active()) {  // the Helm Splitter as Link really flew it (procCutHead): its highest point over its takeoff
    daAlink_c* k = kkb_link();
    if (k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_HEAD) {
      if (!s.helm_seen) s.helm_seen = true, s.helm_y0 = k->current.pos.y, s.helm_peak = 0;
      if (k->current.pos.y - s.helm_y0 > s.helm_peak) s.helm_peak = k->current.pos.y - s.helm_y0;
    }
  } else {
    s.head_logged = false;
  }
  // P4: the eye of the Chilfos the Mortal Draw killed keeps its kill height while Link's Mortal Draw still aims his torso at it
  if (s.md_hold) {
    daAlink_c* k = kkb_link();
    const bool on = k && now <= s.md_hold_until && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH && kk::mortal_draw_cut(k->getCutType()) &&
                    k->mTargetedActor == static_cast<fopAc_ac_c*>(a);
    if (on) {
      if (a->eyePos.y < s.md_eye_min) s.md_eye_min = a->eyePos.y;
      if (a->eyePos.y < s.md_hold_y) kk_native_eye(a, s.md_hold_y), ++s.md_hold_ticks;
      const std::int16_t bx = k->mBodyAngle.x;
      if (bx < s.md_body_min) s.md_body_min = bx;
      if (bx > s.md_body_max) s.md_body_max = bx;
    } else {
      s.md_hold = false, ++s.st.md_pose_holds;
      std::snprintf(line, sizeof(line), "CHILFOS_EXEC event=POSE_HOLD_END actor=%u generation=%llu held_ticks=%u eye_kill_rel=%.1f native_eye_drop=%.1f link_body_x=%d..%d link_proc=%d tick=%llu", s.id,
                    static_cast<unsigned long long>(s.generation), s.md_hold_ticks, s.md_hold_y - a->current.pos.y, s.md_hold_y - s.md_eye_min, int(s.md_body_min), int(s.md_body_max),
                    k ? static_cast<int>(k->mProcID) : -1, static_cast<unsigned long long>(now));
      kk_line(kKkFix05, false, line);
    }
  }
}
void on_kk_exec_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return;
  // D/E identity from the engine's pose of THIS clip (mtx_set already ran in this Execute)
  if (a->mActionMode == kk::kAttack && s->clip.tracked && (s->clip.bck == kk::policy::kBckAttack01 || s->clip.bck == kk::policy::kBckAttack03)) {
    const float f = kk_frame(a);
    float lx = 0, lz = 0;
    kk::local_of(a->field_0x68c.x - a->current.pos.x, a->field_0x68c.z - a->current.pos.z, a->shape_angle.y, lx, lz);
    const float ly = a->field_0x68c.y - a->current.pos.y;
    if (!s->probe0 && f >= 18.0f && f < 23.0f) s->probe0 = true, s->tip0[0] = lx, s->tip0[1] = ly, s->tip0[2] = lz;
    if (s->probe0 && !s->probe1 && f >= 28.0f && f < 33.0f) {
      s->probe1 = true, s->tip1[0] = lx, s->tip1[1] = ly, s->tip1[2] = lz;
      const kk::Tip t = kk::classify_tip(s->tip1[0] - s->tip0[0], s->tip1[1] - s->tip0[1], s->tip1[2] - s->tip0[2]);
      const bool settled = kz.ident.observe(s->clip.bck, t);
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "CHILFOS_ANIM event=TRAJECTORY actor=%u bck=%d rate=%.2f lateral=%.1f vertical=%.1f forward=%.1f class=%s votes5=%u/%u votes6=%u/%u identity=%s", s->id, s->clip.bck,
                    a->mpMorfSO ? a->mpMorfSO->getPlaySpeed() : 0.0f, s->tip1[0] - s->tip0[0], s->tip1[1] - s->tip0[1], s->tip1[2] - s->tip0[2], kk::name(t),
                    kz.ident.votes[0][unsigned(kk::Tip::Thrust)], kz.ident.votes[0][unsigned(kk::Tip::Diagonal)], kz.ident.votes[1][unsigned(kk::Tip::Thrust)], kz.ident.votes[1][unsigned(kk::Tip::Diagonal)],
                    kz.ident.verified ? (kz.ident.swapped ? "MEASURED_SWAPPED" : "MEASURED_DEFAULT") : (kz.ident.conflict ? "CONFLICT_DEFAULT_KEPT" : "DEFAULT_UNVERIFIED"));
      kk_line(kKkAnim, settled, line);
    }
  }
  kk_motion_settle(*s, a);
  {  // FIX04 P1: cc_set just ran: the AT registration the next collision pass uses (one native attack window = one token)
    const bool live = a->field_0x67d == 0 && a->field_0x67e == 0;
    if (live && !s->cc_live) kk_token_close(*s, a), kk_token_open(*s);
    else if (!live && s->cc_live) s->tok_close_at = kkb_tick() + 9;  // reported once the native 8-tick timer could have replayed it
    if (!live && s->tok_close_at && kkb_tick() >= s->tok_close_at) kk_token_close(*s, a);
    s->cc_live = live;
  }
  kk_fix05_post(*s, a);
  kkb_pfx_settle(true, s->id, false);   // a preview no verdict of this Execute confirmed goes back to its own colour
  kkb_pfx_settle(false, s->id, false);
  kkb_pfx_end();
}
HookAction on_kk_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kz.enabled || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (s) {
    if (s->ep.active) kk_end_episode(*s, a, kk::End::SceneOrDelete, "DELETE_PRE");
    kk_retire(*s);
    ++kz.deleted;
  }
  return HOOK_CONTINUE;
}
// FIX01: the anti-stagger of a non-lethal sword contact below posture 100. The native picked its spear-less reaction (7,1) for this contact (our temporary
// spear-less state, or a really disarmed Chilfos); damage_check is the first call of action(), so putting back the action, mode and heading of before the
// call means executeDamage (the flinch clip, then the native BACKWALK) never starts. HP, hit mark, damage SE, mDamageTimer and posture are untouched.
void kk_resume(KkSlot& s, daE_KK_c* a) noexcept {
  if (a->mActionMode != kk::kDamage) return;
  if (s.act_before == kk::kDamage || s.act_before == kk::kDead) {
    ++s.st.resume_skipped;  // already in a native reaction before this contact (a spear break): nothing of ours to undo
    return;
  }
  const int from_mode = a->mMoveMode;
  kk_native_resume(a, s.act_before, s.mode_before, s.heading_before);
  ++s.st.resumed;
  if (s.act_before == kk::kAttack) ++s.st.poise;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_POISE event=RESUMED actor=%u generation=%llu swing=%llu native=7/%d kept=%d/%d hp=%d->%d posture=%d combo=%d phase=%s tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(s.cur_serial), from_mode, a->mActionMode, a->mMoveMode, s.hp_before, a->health, s.p.points(),
                s.ep.active ? 1 : 0, s.ep.active ? kKkPhase[s.ep.ph & 3] : "NONE", static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkPoise, false, line);
}
// ---------------------------------------------------------------- damage_check: the one place a contact is judged
HookAction on_kk_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return HOOK_CONTINUE;
  s->in_dc = true, s->conv = s->decisive = s->shield_atk = s->iron_hit = false, s->cur_v = kk::V::None;
  s->speed_before = a->speedF;
  s->skill_suppressed = false;
  {
    daAlink_c* lk = kkb_link();
    s->death_cut = lk ? lk->getCutType() : -1;
  }
  s->recoil_pre = false;
  s->hp_before = a->health, s->act_before = a->mActionMode, s->mode_before = a->mMoveMode, s->saved_67d = a->field_0x67d, s->heading_before = a->current.angle.y;
  if (a->health <= 1 || a->mActionMode == kk::kDead) return HOOK_CONTINUE;  // the native returns at once too
  daAlink_c* k = kkb_link();
  // FIX04 P1: what the spear's flags really say (stale / echo / one sphere of a new contact), decided before anything reads them.
  const kk::RoundOut ro = kk_spear_round(*s, a);
  // FIX01: the Chilfos' own impact on Link is resolved whatever this actor's damage timer says (a hit Link landed a few ticks before must not
  // swallow the block/parry receipt of the next thrust); still one resolution per AT window. FIX04: never from stale flags (nothing touched Link).
  if (ro.r != kk::Round::Stale) kk_link_contact(*s, a, k);
  kk_round_apply(*s, a, ro);
  s->recoil_pre = ro.r == kk::Round::Keep;  // the native branch that writes speedF = -70 runs for the one sphere kept
  if (a->mDamageTimer != 0) return HOOK_CONTINUE;  // the native ignores every contact of this call
  if (!a->mCyl.cCcD_ObjHitInf::ChkTgHit()) return HOOK_CONTINUE;
  const cCcD_Obj* o = a->mCyl.cCcD_ObjHitInf::GetTgHitObj();
  if (!o) return HOOK_CONTINUE;
  const std::uint64_t now_dc = kkb_tick();
  if (o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
    s->shield_atk = true;
    s->last_link = "SHIELD_ATTACK", s->last_link_tick = now_dc;
    if (!s->brk.active()) {
      // FIX01 (FIX05 rule): below the stagger Link's Shield Attack on the body neither staggers (native DRAWBACK + HeadLock) nor ends the combo.
      // Its Tg mark is cleared before the native reads it; the native contact SE plays once per press.
      const bool fresh = s->bash_last_tick == 0 || now_dc > s->bash_last_tick + 1;
      s->bash_last_tick = now_dc;
      kk_native_tg_clear(a);
      ++s->st.bash_absorbed;
      if (fresh) {
        ++s->st.bash_se;
        kk_native_bash_se(&a->mCreatureSound);
        char line[kLogLineMax];
        std::snprintf(line, sizeof(line), "CHILFOS_POISE event=SHIELD_ATTACK_ABSORBED actor=%u generation=%llu action=%d mode=%d posture=%d hp=%d combo=%d phase=%s native=DRAWBACK_SUPPRESSED tick=%llu", s->id,
                      static_cast<unsigned long long>(s->generation), a->mActionMode, a->mMoveMode, s->p.points(), a->health, s->ep.active ? 1 : 0, s->ep.active ? kKkPhase[s->ep.ph & 3] : "NONE",
                      static_cast<unsigned long long>(now_dc));
        kk_line(kKkPoise, false, line);
      }
      return HOOK_CONTINUE;
    }
    {
      s->brk.shield_attack();
      ++s->st.shield_preps;
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=SHIELD_PREP actor=%u generation=%llu age=%llu headlock=NATIVE tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                    static_cast<unsigned long long>(kkb_tick() - s->brk.start), static_cast<unsigned long long>(kkb_tick()));
      kk_line(kKkBreak, false, line);
    }
    return HOOK_CONTINUE;
  }
  s->iron_hit = o->ChkAtType(kkb_iron_mask()) && k && const_cast<cCcD_Obj*>(o)->GetAc() == static_cast<fopAc_ac_c*>(k);  // native: lethal unless Link's status 0x400
  if (!o->ChkAtType(kkb_sword_mask()) || !k || const_cast<cCcD_Obj*>(o)->GetAc() != static_cast<fopAc_ac_c*>(k)) {
    s->last_link = s->iron_hit ? "IRON_BALL" : "OTHER_WEAPON", s->last_link_tick = now_dc;
    // arrows, boomerang, hookshot, bombs: their native reaction is kept (the FIX05 rule for non-sword weapons), so the combo ends on its own terms
    if (s->ep.active && !o->ChkAtType(kkb_iron_mask())) kk_end_episode(*s, a, kk::End::NativePriority, "DAMAGE_PRE_OTHER_WEAPON");
    return HOOK_CONTINUE;
  }
  const std::uint64_t serial = kz.swing_serial;
  s->cur_serial = serial;
  s->last_link = "SWORD", s->last_link_tick = now_dc;
  s->cur_v = kk_decide(*s, a, k, serial);
  switch (s->cur_v) {
    case kk::V::Ignored:  // the very swing that raised posture to 100 (or a second record of a decided swing during the stagger): no second effect
      ++s->st.ignored;
      kk_native_tg_clear(a);
      break;
    case kk::V::Decisive:
      if (k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP || kk::mortal_draw_cut(k->getCutType())) break;  // Helm Splitter / Mortal Draw: their own native lethal branch (mCutTypeCheck(2)), nothing written
      s->decisive = true, s->conv = true;  // any other NEW sword swing on the staggered Chilfos: the native lethal branch (`health <= 1` after cc_at_check)
      kk_native_spear(a, 1);
      if (a->health > 2) kk_native_health(a, 2);
      break;
    case kk::V::Damage:  // uncovered: the native spear-less branch for this one call (cc_at_check, hit mark, damage SE)
      if (kk_spear(a)) s->conv = true, kk_native_spear(a, 1);
      break;
    case kk::V::EnemyParry:  // FIX04 P2: settled in the POST, once the native (or our guard) spark of this contact exists
      if (kkb_strong_hit(k, o)) kkb_pfx_clash_map(kk::policy::kSeMapClash);
      break;
    default: break;
  }
  if (s->cur_v != kk::V::EnemyParry) kkb_pfx_settle(false, s->id, false);
  return HOOK_CONTINUE;
}
void on_kk_damage_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->in_dc) return;
  s->in_dc = false;
  const std::uint64_t now = kkb_tick();
  daAlink_c* k = kkb_link();
  char line[kLogLineMax];
  if (s->conv) kk_native_spear(a, s->saved_67d);
  const bool died = a->mActionMode == kk::kDead && s->act_before != kk::kDead;
  if (s->decisive) {
    if (!died) {
      // No native lethal receipt: undo only our temporary HP cap. Never synthesize a death.
      kk_native_health(a, s->hp_before);
      // The spear-less reaction (7,1) the native chose for that contact came from our temporary spear-less state: damage_check runs first in action(),
      // so putting back the stagger action here means executeDamage never starts and the 90-tick stagger keeps holding.
      if (s->brk.active() && (a->mActionMode != s->act_before || a->mMoveMode != s->mode_before)) kk_native_action(a, s->act_before, s->mode_before);
      ++s->st.decisive_no_receipt;
      std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=DECISIVE_NO_RECEIPT actor=%u generation=%llu swing=%llu hp=%d action=%d death=NONE tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                    static_cast<unsigned long long>(s->cur_serial), a->health, a->mActionMode, static_cast<unsigned long long>(now));
      kk_line(kKkBreak, true, line);
    } else {
      ++s->st.decisive_sword;
      std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=SWORD_DECISIVE actor=%u generation=%llu swing=%llu hp_before=%d route=NATIVE_LETHAL_BRANCH tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                    static_cast<unsigned long long>(s->cur_serial), s->hp_before, static_cast<unsigned long long>(now));
      kk_line(kKkBreak, false, line);
    }
  }
  if (died) {
    ++s->st.native_deaths;
    const bool helm = k && k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP;
    if (s->brk.active() && helm && s->brk.shield_prep) {
      ++s->st.decisive_helm;
      std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=HELM_HIT actor=%u generation=%llu age=%llu tick=%llu aim_native_rel=%.1f aim_rel=%.1f link_peak=%.1f", s->id,
                    static_cast<unsigned long long>(s->generation), static_cast<unsigned long long>(now - s->brk.start), static_cast<unsigned long long>(now), s->head_native, s->head_lifted,
                    s->helm_peak);
      kk_line(kKkBreak, false, line);
    }
    // FIX03: every death says why. HP_ZERO = a legitimate hit took the last HP (any posture); a native instant kill outside the stagger is a defect now.
    const bool md = kk::mortal_draw_cut(s->death_cut);
    const char* cause = s->decisive ? "SWORD_DECISIVE" : (s->brk.active() && helm) ? "HELM_SPLITTER" : (s->brk.active() && md) ? "MORTAL_DRAW" : s->iron_hit ? "IRON_BALL"
                      : (s->brk.active() && kk::native_instant_cut(s->death_cut)) ? "POSTURE_FINISHER_SKILL" : kk::native_instant_cut(s->death_cut) && s->hp_before > 1 && !s->skill_suppressed ? "NATIVE_SKILL_INSTANT"
                      : (s->cur_v == kk::V::Damage || s->cur_v == kk::V::None) ? "HP_ZERO" : "OTHER";
    if (s->decisive) ++s->st.death_sword_decisive;
    else if (s->brk.active() && (helm || md)) md ? ++s->st.md_kills : ++s->st.death_helm;
    else if (s->iron_hit) ++s->st.death_iron;
    else if (cause[0] == 'N') ++s->st.death_skill_instant;
    else if (cause[0] == 'H' && cause[1] == 'P') ++s->st.death_hp_zero;
    else ++s->st.death_other;
    if (md && s->brk.active() && k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH) {  // FIX05 P4: the eye keeps its kill height for Link's torso aim
      s->md_hold = true, s->md_hold_y = a->eyePos.y, s->md_eye_min = a->eyePos.y, s->md_hold_until = now + kk::policy::kMdPoseHold, s->md_hold_ticks = 0;
      s->md_body_min = s->md_body_max = k->mBodyAngle.x;
    }
    if (md && s->brk.active()) {
      std::snprintf(line, sizeof(line), "CHILFOS_EXEC event=KILL actor=%u generation=%llu cut=%d age=%llu tick=%llu", s->id, static_cast<unsigned long long>(s->generation), s->death_cut,
                    static_cast<unsigned long long>(now - s->brk.start), static_cast<unsigned long long>(now));
      kk_line(kKkExec, false, line);
      s->md_active = false;
    }
    std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=NATIVE_DEATH actor=%u generation=%llu route=%s hp_before=%d broken=%d tick=%llu cause=%s cut=%d posture=%d hp_after=%d", s->id,
                  static_cast<unsigned long long>(s->generation), s->decisive ? "SWORD_DECISIVE" : (s->brk.active() && helm) ? "HELM_SPLITTER" : (s->brk.active() && md) ? "MORTAL_DRAW" : "NATIVE",
                  s->hp_before, s->brk.active() ? 1 : 0, static_cast<unsigned long long>(now), cause, s->death_cut, s->p.points(), a->health);
    kk_line(kKkBreak, false, line);
    if (s->ep.active) kk_end_episode(*s, a, kk::End::Death, "DAMAGE_POST_DEATH");
    if (s->brk.active()) s->brk.st = kk::Bk::Dead;
    s->dead = true;
  }
  if (!died && s->iron_hit && a->mDamageTimer == 8 && a->mActionMode == kk::kDamage) {  // an Iron Ball contact the native branch did not kill with: +30 once for that contact
    s->iron_hit = false;
    kk_posture_gain(*s, a, kk::policy::kGainIron, "IRON_BALL", kz.swing_serial);
  }
  // FIX02: its spear met Link's shield (Link blocked or parried): the native -70 recoil would slide it ~280 units back mid-exchange. It stops instead.
  if (s->recoil_pre && a->speedF == kk::policy::kRecoilNative) {
    kk_native_speed(a, 0.0f);
    ++s->st.recoil_fixed;
    kk_motion(*s, a, "SPEAR_ON_LINK_SHIELD", kk::policy::kRecoilNative, 0.0f);
  }
  // FIX02 ABSOLUTE RULE: a contact the Chilfos blocked or parried never breaks its spear. With the spear in hand a strong sword hit (at_power_check
  // mHitStatus != 0, Link's status 0x400 or a Midna lock) makes the native choose (7,0): executeDamage mode 0 = CLASH, field_0x67d = 1, speedF -80, then
  // the BACKWALK that regrows it. damage_check is the first call of action(), so the native guard reaction (6,0) is put in its place before that runs.
  if (!died && (s->cur_v == kk::V::Block || s->cur_v == kk::V::EnemyParry) && s->saved_67d == 0 && a->mActionMode == kk::kDamage && a->mMoveMode == 0) {
    kk_native_action(a, kk::kGuard, 0);
    ++s->st.spear_break_prevented;
    kkb_guard_mark(a);  // FIX04 P2: the guard spark the native strong branch never plays (blue for a block, the parry FX book makes a parry's orange)
    ++s->st.guard_marks;
    std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=BLOCK_INTACT actor=%u generation=%llu swing=%llu verdict=%s native=7/0 kept=6/0 spear=%d hp=%d tick=%llu", s->id,
                  static_cast<unsigned long long>(s->generation), static_cast<unsigned long long>(s->cur_serial), kk::name(s->cur_v), kk_spear(a) ? 1 : 0, a->health,
                  static_cast<unsigned long long>(now));
    kk_line(kKkSpear, false, line);
  }
  if (s->cur_v == kk::V::EnemyParry) {  // FIX04 P2: the parry record is settled after this damage_check made its spark, so FX_PAIR lists it
    kkb_pfx_settle(false, s->id, !died);
    kkb_pfx_clash_map(kk::policy::kSeMapGuard);
    ++s->st.parry_settle_post;
  }
  if (!died && (s->cur_v == kk::V::Block || s->cur_v == kk::V::EnemyParry)) s->def_tick = now;
  if (!died && s->brk.active() && a->mActionMode == kk::kGuard) {  // FIX03: never a guard reaction while broken (no path of ours writes it; the native's is undone)
    kk_native_action(a, kk::kYoroke, 2);
    kk_break_trace(*s, a, "GUARD_ACTION_UNDONE", -1);
  }
  if (!died) {
    switch (s->cur_v) {
      case kk::V::Damage:
        if (s->dec_ready_frontal) s->def.on_uncovered(now);
        if (a->health < s->hp_before) {
          ++s->st.sword_damage;
          if (s->cur_serial != 0 && s->cur_serial == s->sword_credit_serial) {
            ++s->st.sword_dupes;  // the same swing again after the native 8-tick timer: HP as native, no second posture credit, no second wound
          } else {
            s->sword_credit_serial = s->cur_serial;
            s->def.on_wound(now);
            kk_posture_gain(*s, a, kk::policy::kGainSwordHp, "SWORD_HP", s->cur_serial);
          }
        }
        break;
      case kk::V::Block:  // FIX02: always a real block (the spear-break branch is gone: see BLOCK_INTACT above)
        if (s->cur_serial != 0 && s->cur_serial == s->block_credit_serial) {  // FIX05: a jump's landing after its blocked aerial contact: one block, one gain
          ++s->st.block_dupes;
          break;
        }
        s->block_credit_serial = s->cur_serial;
        ++s->st.blocks;
        s->def.on_block(now, s->cur_serial);
        kk_posture_gain(*s, a, kk::policy::kGainBlock, "BLOCK", s->cur_serial);
        if (s->ep.active && a->mActionMode == kk::kGuard) kk_end_episode(*s, a, kk::End::Defense, "DAMAGE_POST_GUARD");
        break;
      case kk::V::EnemyParry:
        s->parry_serial = s->cur_serial;
        if (s->sv.rebound_seen) kk_apply_enemy_parry(*s, a);
        else s->parry_pending = true, s->parry_pending_tick = now;  // Link's clip may check its rebound after this Execute: settled next tick
        break;
      default: break;
    }
    // FIX01: posture 100 reached in this call -> the native stagger NOW (never one tick of another native reaction first);
    // below 100 a non-lethal sword contact keeps the Chilfos in what it was doing (HP, hit mark, SE and posture stay as above).
    if (kk_try_break(*s, a, "DAMAGE_POST")) ++s->st.immediate_breaks;
    else if (!s->brk.active() && s->cur_v == kk::V::Damage) {
      kk_resume(*s, a);
      // FIX04 P3: a wound outside its own attack wakes its guard: the native guard (6,0), which covers its front (and only its front) for the next swing
      if (!s->ep.active && kk_spear(a) && (a->mActionMode == kk::kWait || a->mActionMode == kk::kWalk || a->mActionMode == kk::kGuard) && a->health > 1) {
        kk_native_action(a, kk::kGuard, 0);
        ++s->st.guard_after_wound;
        std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=GUARD_RAISED actor=%u generation=%llu swing=%llu reason=WOUND hp=%d posture=%d tick=%llu", s->id,
                      static_cast<unsigned long long>(s->generation), static_cast<unsigned long long>(s->cur_serial), a->health, s->p.points(), static_cast<unsigned long long>(now));
        kk_line(kKkDefense, false, line);
      }
    }
  }
  if (kk_spear(a)) s->spear_credit_lock = false;
  // the prepared parry is presented on the body cylinder for the NEXT collision pass (the native mCutTypeCheck(0) auto-deflect is replaced)
  const bool present = !s->dead && s->def.presenting(now, kz.swing_serial) && k && kk_frontal(a, k) && kk_guard_ready(*s, a) && kk_link_dist(a, k) <= 900.0f;
  kk_native_shield(a, present);
  s->shield_on = present;
}
// ---------------------------------------------------------------- executeAttack: combos, rhythm, frame-crossing guarantee
void kk_start_episode(KkSlot& s, daE_KK_c* a) noexcept {
  const std::uint64_t now = kkb_tick();
  s.ep = KkEp{};
  s.ep.active = true, s.ep.t0 = now, s.ep.last_progress = now;
  char line[kLogLineMax];
  if (s.counter_pending) {
    s.counter_pending = false;
    s.ep.counter = true, s.ep.n = 1;
    s.ep.counter_kind = s.rng.below(2) ? kk::K::E : kk::K::D;
    s.ep.want_bck = kz.ident.bck(s.ep.counter_kind), s.ep.want_rate = s.ep.counter_kind == kk::K::D ? kk::policy::kRateD : kk::policy::kRateE;
    ++s.st.counters, ++s.st.counter_started;
    std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=COUNTER actor=%u generation=%llu kind=%c bck=%d rate=%.2f tick=%llu", s.id, static_cast<unsigned long long>(s.generation),
                  s.ep.counter_kind == kk::K::D ? 'D' : 'E', s.ep.want_bck, s.ep.want_rate, static_cast<unsigned long long>(now));
    kk_line(kKkDefense, false, line);
    return;
  }
  s.ep.pattern = s.rng.below(kk::kPatterns);  // ONE selection per offensive episode, 25% each
  s.ep.plan = kk::make_plan(s.ep.pattern);
  s.ep.n = s.ep.plan.n;
  s.ep.want_bck = kz.ident.bck(s.ep.plan.a[0].kind), s.ep.want_rate = s.ep.plan.a[0].rate;
  ++s.st.selected[s.ep.pattern];
  std::snprintf(line, sizeof(line), "CHILFOS_COMBO event=START actor=%u generation=%llu pattern=%u acts=%u ranged_tail=%d tick=%llu posture=%d hp=%d identity=%s", s.id, static_cast<unsigned long long>(s.generation),
                s.ep.pattern + 1, s.ep.n, s.ep.plan.tail.on ? 1 : 0, static_cast<unsigned long long>(now), s.p.points(), a->health,
                kz.ident.verified ? (kz.ident.swapped ? "MEASURED_SWAPPED" : "MEASURED_DEFAULT") : "DEFAULT_UNVERIFIED");
  kk_line(kKkCombo, false, line);
}
void kk_log_action(KkSlot& s, unsigned i, int bck, float start, float rate) noexcept {
  ++s.st.actions;
  char line[kLogLineMax];
  const kk::ActPlan& p = s.ep.plan.a[i];
  std::snprintf(line, sizeof(line), "CHILFOS_COMBO event=ACTION actor=%u pattern=%u act=%u kind=%c anim_id=%d rate=%.2f start_frame=%.0f burst=%d tick=%llu", s.id, s.ep.pattern + 1, i + 1,
                s.ep.counter ? (s.ep.counter_kind == kk::K::D ? 'D' : 'E') : (p.kind == kk::K::D ? 'D' : 'E'), bck, rate, start, p.burst ? 1 : 0, static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkCombo, false, line);
}
void kk_close_window(KkSlot& s, daE_KK_c* a) noexcept {
  s.ep.window_open = false, s.ep.closed = true, s.ep.last_progress = kkb_tick();
  ++s.st.impacts_closed;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_COMBO event=IMPACT_CLOSE actor=%u pattern=%u act=%u impact=%llu frame=%.2f tick=%llu", s.id, s.ep.pattern + 1, s.ep.act + 1,
                static_cast<unsigned long long>(s.imp_serial), kk_frame(a), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkCombo, false, line);
}
HookAction on_kk_attack_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || s->dead || s->brk.active()) return HOOK_CONTINUE;
  const std::uint64_t now = kkb_tick();
  if (a->mMoveMode == 2 && !s->ep.active && kkb_world_safe()) {
    if (!s->counter_pending && now < s->rest_until) {  // the rest after an episode, however it ended: the native wait, no attack
      ++s->st.rest;
      kk_native_action(a, kk::kWait, 0);
      kk_native_wait(a, static_cast<std::int16_t>(s->rest_until - now > 32767 ? 32767 : s->rest_until - now));
      return HOOK_SKIP_ORIGINAL;
    }
    kk_start_episode(*s, a);  // the native picks a clip in this call: the setBck PRE substitutes our clip and rate
    return HOOK_CONTINUE;
  }
  if (a->mMoveMode != 3 || !a->mpMorfSO) return HOOK_CONTINUE;
  const bool closing_now = s->ep.window_open && kk_frame(a) >= float(kk::policy::kAtCloseFrame);  // the native closes the AT in this very call
  if (s->ep.active && s->ep.ph == KkEp::Melee && !s->ep.counter && s->ep.window_seen && (s->ep.closed || closing_now)) {
    const unsigned next = s->ep.act + 1;
    if (next < s->ep.n) {
      const kk::ActPlan& p = s->ep.plan.a[next];
      const kk::Clip c{p.start, p.rate};
      const std::uint64_t due = s->ep.t0 + (p.open - c.open_tick());
      if (now >= due) {
        const int bck = kz.ident.bck(p.kind);
        if (s->ep.window_open) kk_close_window(*s, a);
        kk_restart(*s, a, bck, p.rate, p.start);
        s->ep.act = next, s->ep.window_seen = false, s->ep.closed = false, s->ep.last_progress = now;
        kk_log_action(*s, next, bck, p.start, p.rate);
        return HOOK_CONTINUE;  // the native reads the start frame: nothing fires
      }
      if (s->ep.window_open) return HOOK_CONTINUE;  // still inside its own window: the native closes it
      ++s->st.gaps_skipped;
      if (kk_burst_partner(*s, next)) kkb_burst_gap(s->id);  // the D1->D2 fast re-arm of Link's shield chain for this burst pair (FIX05: only a real pair's gap)
      return HOOK_SKIP_ORIGINAL;  // between two planned actions the native neither turns the attack into nextActionCheck nor re-reads the frame
    }
    if (s->ep.plan.tail.on) {  // C3: the seventh window is closed: backstep at once (gap 0 in the plan)
      if (s->ep.window_open) kk_close_window(*s, a), kk_native_close(a);
      s->ep.ph = KkEp::Back, s->ep.back_until = now + s->ep.plan.tail.back_ticks, s->ep.last_progress = now;
      ++s->st.backsteps;
      kk_native_back(a);
      kk_native_action(a, kk::kBackWalk, 0);
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=BACKSTEP actor=%u generation=%llu ticks=%u rate=%.2f tick=%llu", s->id, static_cast<unsigned long long>(s->generation), s->ep.plan.tail.back_ticks,
                    kk::policy::kRateBack, static_cast<unsigned long long>(now));
      kk_line(kKkSpear, false, line);
      return HOOK_SKIP_ORIGINAL;
    }
  }
  // frame-crossing guarantee of the native `(s32)frame == N` events, for this module's accelerated clips only
  if (s->clip.tracked && a->mpMorfSO->getPlaySpeed() > 1.0f) {
    const float f = kk_frame(a);
    const kk::Cross c = kk::crossing(s->clip.last_read, f, kk::kAttackEvents, 2, s->clip.fired);
    if (c.duplicate) {
      ++s->st.duplicates;
      return HOOK_SKIP_ORIGINAL;  // repeated event frame: no second AT opening or attack sound
    }
    for (unsigned i = 0; i < 2; ++i)
      if (int(f) == kk::kAttackEvents[i]) s->clip.fired |= 1u << i;
    if (c.snap) {
      s->clip.true_frame = f, s->clip.snapped = true, s->clip.fired |= 1u << c.event;
      ++s->st.snaps;
      kk_native_frame(a->mpMorfSO, c.frame);
      s->clip.last_read = c.frame;
    } else {
      s->clip.last_read = f;
    }
  }
  return HOOK_CONTINUE;
}
void on_kk_attack_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return;
  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;
  // FIX02: the native frame-24 lunge (speedF = 80, written in this very call) is capped so the Chilfos stops kContactDist from a Link already in reach:
  // the spear still reaches him, the bodies never shove each other across the floor. Farther away the native lunge is untouched.
  if (a->mActionMode == kk::kAttack && a->speedF == kk::policy::kLungeNative && !s->dead) {
    daAlink_c* k = kkb_link();
    if (k && kk_frontal(a, k)) {
      const float allowed = kk_link_dist(a, k) - kk::policy::kContactDist;
      const float v = kk::lunge_for(allowed);
      if (v < kk::policy::kLungeNative) {
        kk_native_speed(a, v);
        ++s->st.lunge_clamped;
        kk_motion(*s, a, "LUNGE_CAPPED", kk::policy::kLungeNative, v);
      }
    }
  }
  if (!s->ep.active || s->ep.ph != KkEp::Melee) return;
  const std::uint64_t now = kkb_tick();
  char line[kLogLineMax];
  if (a->field_0x67e == 0 && !s->ep.window_open && a->mActionMode == kk::kAttack) {
    s->ep.window_open = true, s->ep.window_seen = true, s->ep.last_progress = now;
    ++s->imp_serial, ++s->st.impacts_open;
    const unsigned i = s->ep.act;
    const kk::ActPlan& p = s->ep.plan.a[i];
    const float achieved = i == 0 ? 0.0f : float(now - s->ep.t0) / 30.0f - float(s->ep.plan.a[i - 1].open) / 30.0f;
    std::snprintf(line, sizeof(line), "CHILFOS_COMBO event=IMPACT_OPEN actor=%u pattern=%u act=%u impact=%llu anim_id=%d rate=%.2f frame=%.2f tick=%llu", s->id, s->ep.pattern + 1, i + 1,
                  static_cast<unsigned long long>(s->imp_serial), s->clip.bck, a->mpMorfSO ? a->mpMorfSO->getPlaySpeed() : 0.0f, kk_frame(a), static_cast<unsigned long long>(now));
    kk_line(kKkCombo, false, line);
    if (!s->ep.counter && i > 0) {
      std::snprintf(line, sizeof(line), "CHILFOS_RHYTHM event=RECORDED_VS_ACTUAL actor=%u pattern=%u act=%u recorded_s=%.3f planned_s=%.3f achieved_s=%.3f start_frame=%.0f anim_rate=%.2f", s->id, s->ep.pattern + 1, i + 1,
                    p.recorded_s, p.achieved_s, achieved, p.start, p.rate);
      kk_line(kKkRhythm, false, line);
    }
  } else if (a->field_0x67e != 0 && s->ep.window_open) {
    kk_close_window(*s, a);
  }
}
// ---------------------------------------------------------------- FIX03: the stagger, the hidden skills, the Mortal Draw finisher, the throw's frame pacing
inline constexpr int kBckGuardA = 0x10, kBckGuardB = 0x11;
void kk_break_trace(KkSlot& s, daE_KK_c* a, const char* what, int asked) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=TRACE actor=%u generation=%llu what=%s asked_bck=%d bck=%d action=%d mode=%d frame=%.1f tgshield=%d headlock=%d shield_prep=%d age=%llu tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), what, asked, a->field_0x764, a->mActionMode, a->mMoveMode, kk_frame(a), a->mCyl.ChkTgShield() ? 1 : 0, a->checkHeadLockFlg() ? 1 : 0,
                s.brk.shield_prep ? 1 : 0, static_cast<unsigned long long>(kkb_tick() - s.brk.start), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkBreak, false, line);
}
HookAction on_kk_cutcheck_pre(ModContext*, void* args, void* ret, void*) noexcept {
  if (!kk_alive() || !args || !ret) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->in_dc) return HOOK_CONTINUE;  // only inside THIS admitted Chilfos' own damage_check
  const int param = mods::arg<int>(args, 1);
  if (param == 2 && !s->brk.active()) {  // the instant-kill branch: not before the stagger; the hit takes the ordinary damage path (HP 0 still kills)
    daAlink_c* k = kkb_link();
    const int cut = k ? k->getCutType() : -1;
    if (kk::native_instant_cut(cut) && !s->skill_suppressed) {
      s->skill_suppressed = true, ++s->st.skill_instant_suppressed;
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "CHILFOS_HP event=SKILL_INSTANT_SUPPRESSED actor=%u generation=%llu cut=%d hp=%d posture=%d route=ORDINARY_DAMAGE tick=%llu", s->id,
                    static_cast<unsigned long long>(s->generation), cut, a->health, s->p.points(), static_cast<unsigned long long>(kkb_tick()));
      kk_line(kKkExec, false, line);
    }
    *static_cast<bool*>(ret) = false;
    return HOOK_SKIP_ORIGINAL;
  }
  if (param == 0 && s->brk.active()) {  // the native auto-deflect would put TgShield up for a Helm Splitter / Mortal Draw: never while broken
    ++s->st.stagger_shield_closed;
    *static_cast<bool*>(ret) = false;
    return HOOK_SKIP_ORIGINAL;
  }
  return HOOK_CONTINUE;
}
void kk_md_track(KkSlot& s, daE_KK_c* a, std::uint64_t now) noexcept {
  daAlink_c* k = kkb_link();
  const bool on = k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH && kk::mortal_draw_cut(k->getCutType());
  if (on || s.dead) return;
  s.md_active = false, ++s.st.md_cancelled;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_EXEC event=CANCELLED actor=%u generation=%llu reason=LINK_LEFT_MORTAL_DRAW proc=%d cut=%d hp=%d broken=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), k ? static_cast<int>(k->mProcID) : -1, k ? k->getCutType() : -1, a->health, s.brk.active() ? 1 : 0,
                static_cast<unsigned long long>(now));
  kk_line(kKkExec, true, line);
}
// B on a broken Chilfos: the native checkCutAction has just chosen Link's ordinary sword cut (PROC_CUT_NORMAL/FINISH). If a broken, admitted Chilfos without a
// prepared Helm Splitter stands in front of Link and in reach, the native Mortal Draw finisher (procCutFinishInit(MORTAL_DRAW_B): its animation, AT, sound and
// cut type 0x1F) replaces it in the same call. The kill is the native mCutTypeCheck(2) branch of that Chilfos' damage_check, on contact only.
void on_kk_cut_action_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!kk_alive() || !args || !ret || *static_cast<int*>(ret) == 0) return;
  auto* k = mods::arg<daAlink_c*>(args, 0);
  if (!k || k != kkb_link() || k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)) return;
  const int proc = static_cast<int>(k->mProcID), cut = k->getCutType();
  if ((proc != daAlink_c::PROC_CUT_NORMAL && proc != daAlink_c::PROC_CUT_FINISH) || kk::native_instant_cut(cut)) return;  // a hidden skill or a finisher: native
  KkSlot* best = nullptr;
  float best_d = kk::policy::kMdReach + 1.0f;
  for (auto& s : kz.slots) {
    if (!s.on || !s.admitted || s.dead || !s.brk.active() || s.brk.shield_prep || s.md_active) continue;
    auto* a = reinterpret_cast<daE_KK_c*>(s.key);
    const float d = kk_link_dist(a, k);
    const int to_kk = kk_link_angle(a, k) + 0x8000;
    if (d <= kk::policy::kMdReach && kk::angle_diff(k->shape_angle.y, to_kk) <= kk::policy::kMdArc && d < best_d) best = &s, best_d = d;
  }
  if (!best) return;
  auto* a = reinterpret_cast<daE_KK_c*>(best->key);
  ++best->st.md_requests;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_EXEC event=B_REQUEST actor=%u generation=%llu native_proc=%d native_cut=%d dist=%.1f age=%llu tick=%llu", best->id, static_cast<unsigned long long>(best->generation),
                proc, cut, best_d, static_cast<unsigned long long>(kkb_tick() - best->brk.start), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkExec, false, line);
  kk_native_mortal_draw(k, kk::policy::kFinishMortalDrawB);
  const bool started = static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_FINISH && kk::mortal_draw_cut(k->getCutType());
  if (started) best->md_active = true, best->md_tick = kkb_tick(), ++best->st.md_started;
  else ++best->st.md_cancelled;
  std::snprintf(line, sizeof(line), "CHILFOS_EXEC event=%s actor=%u generation=%llu proc=%d cut=%d index=%d hp=%d posture=%d tick=%llu", started ? "START_NATIVE" : "CANCELLED reason=NATIVE_REFUSED",
                best->id, static_cast<unsigned long long>(best->generation), static_cast<int>(k->mProcID), k->getCutType(), kk::policy::kFinishMortalDrawB, a->health, best->p.points(),
                static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkExec, !started, line);
}
// Frame pacing around the C3 spear release (presented frames: dMeter2_c::_draw POST, one QueryPerformanceCounter read each). GPU time is not measurable here.
void on_kk_frame_post(ModContext*, void*, void*, void*) noexcept {
  if (!kz.enabled) return;
  KkPerf& f = kz.perf;
  const std::int64_t q = kkb_qpc();
  if (f.ft0 == 0) f.ft0 = kkb_filetime(), f.q0 = q;
  else if (f.freq == 0.0) {
    const std::int64_t dft = kkb_filetime() - f.ft0;
    if (dft >= 50000000) f.freq = double(q - f.q0) * 1.0e7 / double(dft);  // 5 s of wall clock: QPC ticks per second, no new import needed
  }
  if (f.last != 0 && q > f.last) {
    const std::uint32_t dt = q - f.last > 0x7FFFFFFF ? 0x7FFFFFFFu : std::uint32_t(q - f.last);
    f.ring[f.n++ % KkPerf::kRing] = dt;
    if (f.open && f.post_n < kk::policy::kPerfFrames) f.post[f.post_n++] = dt;
  }
  f.last = q;
  if (f.open && f.post_n >= kk::policy::kPerfFrames) kk_perf_close();
}

HookAction on_kk_setbck_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args || kz.self_bck) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return HOOK_CONTINUE;
  int& idx = mods::arg_ref<int>(args, 1);
  u8& attr = mods::arg_ref<u8>(args, 2);
  float& rate = mods::arg_ref<float>(args, 4);
  if (s->brk.active() && (idx == kk::policy::kBckDamageBow || idx == kk::policy::kBckDamageBowB || idx == kk::policy::kBckDrawback || idx == kBckGuardA || idx == kBckGuardB)) {
    // FIX03: while broken it only ever shows its stagger. The native Shield Attack reaction (Yoroke mode 1) asks for KK_DRAWBACK, a raised-spear recoil that
    // reads as a guard; a guard clip could only come from a native guard. Both become the stagger clip (with or without spear); HeadLock stays native.
    const int asked = idx;
    if (idx != kk::policy::kBckDamageBow && idx != kk::policy::kBckDamageBowB) {
      idx = kk_spear(a) ? kk::policy::kBckDamageBow : kk::policy::kBckDamageBowB;
      ++s->st.stagger_clip_kept;
    }
    attr = 2;  // EMode_LOOP never sets the stop state: the native Yoroke (which leaves at isStop) holds until this module ends it (90 ticks)
    s->bow_pending = true;  // FIX05 P0: after the native setAnm, the exec POST makes it ONE play across the window (loop frame = last frame, stretched rate)
    kk_break_trace(*s, a, asked == idx ? "STAGGER_CLIP" : "STAGGER_CLIP_KEPT", asked);
    return HOOK_CONTINUE;
  }
  if (s->ep.active && s->ep.ph == KkEp::Melee && s->ep.want_bck >= 0 && (idx == kk::policy::kBckAttack01 || idx == kk::policy::kBckAttack03)) {
    idx = s->ep.want_bck, rate = s->ep.want_rate;
    s->clip = KkClip{};
    s->clip.bck = idx, s->clip.last_read = -1.0f, s->clip.tracked = true;
    s->probe0 = s->probe1 = false;
    kk_log_action(*s, 0, idx, 0.0f, rate);
    s->ep.want_bck = -1;
    return HOOK_CONTINUE;
  }
  if (s->ep.active && s->ep.ph == KkEp::Back) {
    if (idx == kk::policy::kBckBackWalk) rate = kk::policy::kRateBack;
    if (idx == kk::policy::kBckRestoreB) return HOOK_SKIP_ORIGINAL;  // the spear is still in the hand: no restore clip
  }
  if (s->ep.active && (s->ep.ph == KkEp::Throw || s->ep.ph == KkEp::Restore)) {
    if (idx == kk::policy::kBckThrow) {
      rate = kk::policy::kRateThrow;
      s->clip = KkClip{};
      s->clip.bck = idx, s->clip.last_read = -1.0f, s->clip.tracked = true, s->clip.throw_clip = true;
      s->spawns = 0;
    } else if (idx == kk::policy::kBckRestoreA) {
      rate = kk::policy::kRateRestore;
      s->ep.ph = KkEp::Restore, s->ep.last_progress = kkb_tick();
      ++s->st.restores;
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=RESTORE actor=%u generation=%llu rate=%.2f tick=%llu", s->id, static_cast<unsigned long long>(s->generation), rate, static_cast<unsigned long long>(kkb_tick()));
      kk_line(kKkSpear, false, line);
    }
  }
  return HOOK_CONTINUE;
}
HookAction on_kk_setweapon_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->ep.active) return HOOK_CONTINUE;
  const int idx = mods::arg<int>(args, 1);
  if (s->ep.ph == KkEp::Back && idx == kk::policy::kWeaponRestore) return HOOK_SKIP_ORIGINAL;
  if ((s->ep.ph == KkEp::Throw || s->ep.ph == KkEp::Restore) && idx == kk::policy::kWeaponRestore) mods::arg_ref<float>(args, 4) = kk::policy::kRateRestore;
  return HOOK_CONTINUE;
}
void on_kk_back_post(ModContext*, void* args, void*, void*) noexcept {  // the native backstep met a wall, a cliff or its own timer: throw from here
  if (!kk_alive() || !args) return;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->ep.active || s->ep.ph != KkEp::Back) return;
  if ((a->mMoveMode == 1 || a->mMoveMode == 2) && a->speedF == kk::policy::kBackSpeedNative) {  // the native just wrote its 15 for BCK7 at rate 1.0
    kk_native_speed(a, kk::policy::kBackSpeedNative * kk::policy::kRateBack);
    ++s->st.back_speed_fixes;
    return;
  }
  if (a->mMoveMode != 3) return;
  ++s->st.early_throw, ++s->st.throws;
  s->ep.ph = KkEp::Throw, s->ep.last_progress = kkb_tick();
  kk_native_action(a, kk::kThrow, 1);
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=BACKSTEP_STOPPED actor=%u generation=%llu reason=NATIVE_WALL_CLIFF_OR_TIMER left=%lld tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                static_cast<long long>(s->ep.back_until) - static_cast<long long>(kkb_tick()), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkSpear, false, line);
}
// FIX04 P0: the C3 wind-up as the CPU really ran it: every native executeSpearThrow call of mode 2 up to the release, timed with QPC (calibrated against
// the file-time clock after 5 s; 10 MHz is Windows' QPC rate otherwise), and the longest BG line its melee cancel cast. One line per throw.
void kk_throw_cost(KkSlot& s) noexcept {
  const double hz = kz.perf.freq > 0.0 ? kz.perf.freq : 1.0e7;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_PERF event=THROW_WINDUP actor=%u generation=%llu calls=%u native_ms_max=%.3f native_ms_avg=%.3f los_len_max=%.1f los_strides_per_kcl=%llu eye=LINK_FEET_MINUS_%d "
                "clock=%s tick=%llu", s.id, static_cast<unsigned long long>(s.generation), s.thr_calls, double(s.thr_max) * 1000.0 / hz, s.thr_calls ? double(s.thr_sum) * 1000.0 / hz / s.thr_calls : 0.0,
                double(s.thr_len_max), static_cast<unsigned long long>(kk::los_strides(s.thr_len_max)), int(kk::policy::kThrowEyeBelow), kz.perf.freq > 0.0 ? "QPC_CALIBRATED" : "QPC_ASSUMED_10MHZ",
                static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkPerf, false, line);
  s.thr_max = s.thr_sum = 0, s.thr_calls = 0, s.thr_len_max = 0;
}
HookAction on_kk_throw_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  KkPerfScope ps;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->ep.active || (s->ep.ph != KkEp::Throw && s->ep.ph != KkEp::Restore)) return HOOK_CONTINUE;
  s->d_before = a->field_0x67d;
  if (s->ep.ph != KkEp::Throw) return HOOK_CONTINUE;
  if (a->mMoveMode == 2 && a->mpMorfSO && kk_frame(a) < float(kk::policy::kThrowSpawnFrame) && !s->eye_saved) {  // the native melee cancel (Link within 900 and frame < 23) would drop the planned throw
    // FIX04 P0: the cancel needs `eyePos.y + 50 > Link.y`; the eye goes just under Link's feet, so the BG line the same native call casts from it stays as
    // short as the vanilla one (0.13.3's -1e9 made dBgWKCol::LineCheck walk ~2 000 000 strides per KCL in every wind-up Execute).
    daAlink_c* k = kkb_link();
    if (k) {
      const float y = kk::throw_eye(a->eyePos.y, k->current.pos.y);
      if (y < a->eyePos.y) {
        s->eye_saved = true, s->eye_y = a->eyePos.y;
        kk_native_eye(a, y);
        ++s->st.throw_eye_writes;
      } else {
        ++s->st.throw_eye_kept;  // already under Link: the native cancel is off by itself
      }
      const float dx = k->current.pos.x - a->current.pos.x, dy = k->current.pos.y + kk::policy::kLosOffsetY - a->eyePos.y, dz = k->current.pos.z - a->current.pos.z;
      const float len = kk::fsqrt(dx * dx + dy * dy + dz * dz);  // the segment fopAcM_otherBgCheck casts in this call
      if (len > s->thr_len_max) s->thr_len_max = len;
    }
  }
  if (a->mMoveMode == 2) s->thr_t0 = kkb_qpc();  // the wall time of this native wind-up/release call (BG line, clip, child create)
  if (a->mMoveMode == 2 && s->clip.tracked && s->clip.throw_clip && a->mpMorfSO) {
    const float f = kk_frame(a);
    const kk::Cross c = kk::crossing(s->clip.last_read, f, kk::kThrowEvents, 4, s->clip.fired);
    if (c.duplicate) ++s->st.duplicates;
    for (unsigned i = 0; i < 4; ++i)
      if (int(f) == kk::kThrowEvents[i]) s->clip.fired |= 1u << i;
    if (c.snap) {
      s->clip.true_frame = f, s->clip.snapped = true, s->clip.fired |= 1u << c.event;
      ++s->st.snaps;
      kk_native_frame(a->mpMorfSO, c.frame);
      s->clip.last_read = c.frame;
    } else {
      s->clip.last_read = f;
    }
    if (c.duplicate && c.event < 0 && int(f) == kk::policy::kThrowSpawnFrame && s->spawns > 0) {  // never a second spear: show the native the next frame instead
      s->clip.true_frame = f, s->clip.snapped = true;
      kk_native_frame(a->mpMorfSO, float(kk::policy::kThrowSpawnFrame + 1));
    }
  }
  return HOOK_CONTINUE;
}
void on_kk_throw_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return;
  KkPerfScope ps;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return;
  if (s->eye_saved) kk_native_eye(a, s->eye_y), s->eye_saved = false;  // FIX01: restored even when the episode ended inside the native call
  if (s->thr_t0) {
    const std::int64_t dt = kkb_qpc() - s->thr_t0;
    s->thr_t0 = 0;
    if (dt > 0) {
      ++s->thr_calls, s->thr_sum += dt;
      if (dt > s->thr_max) s->thr_max = dt;
    }
  }
  if (!s->ep.active) return;
  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;
  if (s->ep.ph == KkEp::Restore && s->d_before != 0 && a->field_0x67d == 0) {
    ++s->st.rearmed;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=REARMED actor=%u generation=%llu body_frame=%.1f weapon_frame=%.1f tick=%llu", s->id, static_cast<unsigned long long>(s->generation), kk_frame(a),
                  a->mpWeaponMorfSO ? a->mpWeaponMorfSO->getFrame() : -1.0f, static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkSpear, false, line);
  }
  if (s->ep.ph == KkEp::Throw && s->d_before == 0 && a->field_0x67d == 1 && a->field_0x764 == kk::policy::kBckThrow) {
    ++s->spawns, ++s->st.spawns;
    if (s->spawns > 1) ++s->st.spawn_dupes;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=THROW_SPAWN actor=%u generation=%llu count=%u rate=%.2f frame=%.2f tick=%llu", s->id, static_cast<unsigned long long>(s->generation), s->spawns,
                  a->mpMorfSO ? a->mpMorfSO->getPlaySpeed() : 0.0f, kk_frame(a), static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkSpear, s->spawns > 1, line);
    kk_throw_cost(*s);
    std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=DISARMED actor=%u generation=%llu tick=%llu", s->id, static_cast<unsigned long long>(s->generation), static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkSpear, false, line);
  }
}
// After a converted hit with the spear still in the hand the native would walk away and play the spear restore: the Chilfos fights on instead.
void on_kk_dmgexec_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || s->dead) return;
  if (a->mActionMode == kk::kBackWalk && kk_spear(a)) kk_native_action(a, kk::kWait, 0);
}
void on_kk_next_post(ModContext*, void* args, void*, void*) noexcept {  // the rest after an episode: the native wait instead of the next attack
  if (!kk_alive() || !args) return;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return;
  const std::uint64_t now = kkb_tick();
  if (s->ep.active && s->ep.ph == KkEp::Melee && s->ep.closed && s->ep.act + 1 >= s->ep.n && !s->ep.plan.tail.on) kk_end_episode(*s, a, kk::End::Completed, "NEXT_ACTION_CHECK");  // the native ends the last clip
  if (s->ep.active || s->counter_pending) return;
  if (a->mActionMode == kk::kAttack && now < s->rest_until) {
    ++s->st.rest;
    kk_native_action(a, kk::kWait, 0);
    kk_native_wait(a, static_cast<std::int16_t>(s->rest_until - now > 32767 ? 32767 : s->rest_until - now));
  }
}
// ---------------------------------------------------------------- Link: swing start, rebound of his sword, native damage receipt
// FIX04 P3: a ready Chilfos Link faces decides NOW whether it covers this swing (the pressure / wound roll), and if it does the native guard comes up
// before the blade arrives: what the player sees is what the contact gets. A prepared parry raises it too. Its own attacks are never interrupted.
void kk_raise_guard(KkSlot& s, daE_KK_c* a, daAlink_c* k, kk::Np why, std::uint64_t now) noexcept {
  if (s.dead || !kk_guard_ready(s, a) || !kk_frontal(a, k)) return;
  const int act = a->mActionMode;
  if (act != kk::kWait && act != kk::kWalk && act != kk::kGuard) return;
  s.raise_serial = kz.swing_serial;
  s.raise_ok = why == kk::Np::None || act == kk::kGuard || s.rng.pct(s.def.block_pct(now, kz.swing_serial));
  if (!s.raise_ok) {
    ++s.st.guard_raise_fail;
    return;
  }
  if (act == kk::kGuard) return;  // already up: the native clip goes on
  kk_native_action(a, kk::kGuard, 0);
  ++s.st.guard_raised;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=GUARD_RAISED actor=%u generation=%llu swing=%llu reason=%s block_pct=%d hp=%d posture=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(kz.swing_serial), why == kk::Np::None ? "PARRY_PREPARED" : "ROLL", s.def.block_pct(now, kz.swing_serial), a->health,
                s.p.points(), static_cast<unsigned long long>(now));
  kk_line(kKkDefense, false, line);
}
void on_kk_cut_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kz.enabled || !args) return;
  auto* k = mods::arg<daAlink_c*>(args, 0);
  const int t = mods::arg<u8>(args, 1);
  if (!k || k != kkb_link() || k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)) return;
  if (t == daPy_py_c::CUT_TYPE_NONE || t == daPy_py_c::CUT_TYPE_GUARD_ATTACK) return;
  const std::uint64_t now = kkb_tick();
  const int proc = static_cast<int>(k->mProcID);
  const bool jump = t == daPy_py_c::CUT_TYPE_JUMP;
  // FIX05 P1: procCutJumpLandInit calls setCutType(CUT_TYPE_JUMP) again: the landing is the SAME Jump Attack (one serial: one roll, one verdict, one gain)
  const bool land = jump && proc == daAlink_c::PROC_CUT_JUMP_LAND && kz.jump_open && kz.jump_serial == kz.swing_serial;
  if (land) {
    kz.jump_open = false, ++kz.jump_land_merged;
  } else {
    if (kz.swing_tick == now) return;
    kz.swing_tick = now;
    ++kz.swing_serial;
    kz.jump_open = jump && proc == daAlink_c::PROC_CUT_JUMP, kz.jump_serial = kz.swing_serial;
  }
  for (auto& s : kz.slots) {
    if (!s.on || !s.admitted || s.dead) continue;
    if (land && s.rolled_serial == kz.swing_serial) {  // already decided at the takeoff
      ++s.st.jump_land_merged;
      continue;
    }
    auto* a = reinterpret_cast<daE_KK_c*>(s.key);
    const int to_kk = kk_link_angle(a, k) + 0x8000;
    if (kk::angle_diff(k->shape_angle.y, to_kk) > kk::policy::kTargetArc || kk_link_dist(a, k) > kk::policy::kTargetDist) continue;
    s.rolled_serial = kz.swing_serial;
    if (jump) ++s.st.jump_swings;
    const kk::Np why = s.def.on_swing(now, kz.swing_serial, kk_spear(a), kk_attacking(a), kk_frontal(a, k), s.brk.active() || s.brk.st == kk::Bk::Pending, s.rng, jump);
    ++s.st.no_parry[unsigned(why)];
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=%s actor=%u generation=%llu swing=%llu cut=%d level=%c pct_two_blocks=%d reason=%s tick=%llu", why == kk::Np::None ? "PARRY_PREPARED" : "ROLL",
                  s.id, static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(kz.swing_serial), t, "ABC"[s.def.level(now)], s.def.two_blocks(now) ? 1 : 0, kk::name(why),
                  static_cast<unsigned long long>(now));
    kk_line(kKkDefense, false, line);
    kk_raise_guard(s, a, k, why, now);
  }
}
void on_kk_cut_reverse_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!kk_alive() || !args || !ret) return;
  auto* r = static_cast<BOOL*>(ret);
  if (!*r) return;
  auto* o = mods::arg<dCcD_GObjInf*>(args, 1);
  const cCcD_Obj* hit = o ? o->cCcD_ObjHitInf::GetAtHitObj() : nullptr;
  if (!hit) return;
  daAlink_c* k = kkb_link();
  for (auto& s : kz.slots) {
    if (!s.on || !s.admitted || s.dead) continue;
    auto* a = reinterpret_cast<daE_KK_c*>(s.key);
    if (hit != static_cast<const cCcD_Obj*>(&a->mCyl)) continue;
    const kk::V v = kk_decide(s, a, k, kz.swing_serial);  // the same verdict the damage_check of this swing uses, whichever runs first
    if (v == kk::V::EnemyParry) {
      s.sv.rebound_seen = true;
      ++kz.reverse_kept;
      if (k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP) ++s.st.jump_rebound_air;  // procCutJump: the native aerial rebound follows this TRUE
      else if (k && static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_JUMP_LAND) ++s.st.jump_rebound_land;  // procCutJumpLand: ANM_CUT_RECOIL_B
    } else {
      *r = FALSE;  // an ordinary block: no rebound
      ++kz.reverse_suppressed;
    }
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=SWORD_%s actor=%u generation=%llu swing=%llu cut=%d proc=%d tick=%llu", v == kk::V::EnemyParry ? "REBOUND_NATIVE" : "NO_REBOUND", s.id,
                  static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(kz.swing_serial), k ? k->getCutType() : -1, k ? static_cast<int>(k->mProcID) : -1,
                  static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkDefense, false, line);
    return;
  }
}
void on_kk_link_damage_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kz.enabled || !args) return;
  auto* link = mods::arg<daAlink_c*>(args, 0);
  if (link) kkb_direct_capture(link);
}
// ---------------------------------------------------------------- once per mod_update
void kk_maintenance() noexcept {
  if (!kz.enabled) return;
  const std::uint32_t st = kkb_stage();
  if (st != kz.stage) {  // stage change: no pointer is dereferenced again, no state survives
    kz.stage = st;
    for (auto& s : kz.slots) kk_retire(s);
  }
  for (unsigned i = 0; i < kk::kSlots; ++i) {
    std::uint32_t id = 0;
    std::uint64_t token = 0;
    if (!kkb_direct_take(i, id, token)) continue;
    KkSlot& s = kz.slots[i];
    if (s.on && s.admitted && s.id == id) kk_link_gain(s, kk::policy::kLinkDirect, false, token, "DIRECT");
  }
}
void kk_hud_samples() noexcept {
  if (!kz.enabled) return;
  if (kkb_stage() != kz.stage) {  // a stage change before any Chilfos executed: nothing is dereferenced, every slot is retired
    kz.stage = kkb_stage();
    for (auto& s : kz.slots) kk_retire(s);
  }
  for (unsigned i = 0; i < kk::kSlots; ++i) {
    KkSlot& s = kz.slots[i];
    if (!s.on || !s.admitted) continue;
    auto* act = reinterpret_cast<fopAc_ac_c*>(s.key);
    bool busy = false;
    if (kkb_hud(i, s.key, s.id, s.generation, s.p.points(), s.p.latched, s.dead || act->health <= 0, s.brk.active(), act, busy)) ++kz.hud_shown;
    else if (busy) ++kz.hud_busy;
    else ++kz.hud_missing;
  }
}
void kk_summary() noexcept {
  kk::Stats t = kz.totals;
  for (auto& s : kz.slots)
    if (s.on && s.admitted) kk_merge(t, s.st);
  const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
  char line[kLogLineMax * 4];
  std::snprintf(line, sizeof(line),
                "CHILFOS_SUMMARY version=%s tag=%s admitted=%llu excluded=%llu deleted=%llu overflow=%llu selected=%llu/%llu/%llu/%llu completed=%llu/%llu/%llu/%llu actions=%llu impacts=%llu/%llu "
                "link_parry=%llu link_block=%llu link_direct_offered=%llu link_gain=%llu link_sat=%llu sword_damage=%llu blocks=%llu enemy_parry=%llu downgraded=%llu ignored=%llu spear_breaks=%llu "
                "verdicts=%llu/%llu/%llu/%llu/%llu no_parry=%llu/%llu/%llu/%llu/%llu/%llu/%llu rebound_kept=%llu rebound_suppressed=%llu counters=%llu counter_expired=%llu rest=%llu poise=%llu "
                "throws=%llu spawns=%llu spawn_dupes=%llu restores=%llu backsteps=%llu early_throw=%llu breaks=%llu shield_preps=%llu decisive_sword=%llu decisive_helm=%llu recovered=%llu "
                "native_deaths=%llu fallback_deaths=%llu decisive_no_receipt=%llu headlock_stripped=%llu snaps=%llu duplicates=%llu gaps=%llu watchdog=%llu regen_ticks=%llu ends=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
                "bash_absorbed=%llu bash_se=%llu resumed=%llu resume_skipped=%llu immediate_breaks=%llu sword_dupes=%llu back_speed=%llu rearmed=%llu "
                "blocked_spear_break=%llu spear_break_prevented=%llu other_spear_loss=%llu intentional_throw=%llu counter_dispatched=%llu counter_started=%llu "
                "counter_cancelled=%llu/%llu/%llu/%llu/%llu/%llu motion_recoil_fixed=%llu lunge_capped=%llu guard_guarantees=%llu "
                "guard_active_blocks=%llu skill_instant_suppressed=%llu stagger_shield_closed=%llu stagger_clip_kept=%llu md=%llu/%llu/%llu/%llu/%llu "
                "deaths=hp_zero:%llu,skill_instant:%llu,sword_decisive:%llu,helm:%llu,mortal_draw:%llu,iron:%llu,other:%llu spear_children=%llu perf_windows=%llu "
                "identity=%s votes5=%u/%u votes6=%u/%u hud=%llu/%llu/%llu dropped=%llu runtime=HUMAN_RUNTIME_PENDING",
                kk::kVersion, kk::kTag, u(kz.admitted), u(kz.excluded), u(kz.deleted), u(kz.overflow), u(t.selected[0]), u(t.selected[1]), u(t.selected[2]), u(t.selected[3]), u(t.completed[0]), u(t.completed[1]),
                u(t.completed[2]), u(t.completed[3]), u(t.actions), u(t.impacts_open), u(t.impacts_closed), u(t.link_parry), u(t.link_block), u(t.link_direct_offered), u(kz.link_gain), u(kz.link_saturations),
                u(t.sword_damage), u(t.blocks), u(t.enemy_parries), u(t.downgraded_parries), u(t.ignored), u(t.spear_breaks), u(t.verdict[1]), u(t.verdict[2]), u(t.verdict[3]), u(t.verdict[4]), u(t.verdict[5]),
                u(t.no_parry[0]), u(t.no_parry[1]), u(t.no_parry[2]), u(t.no_parry[3]), u(t.no_parry[4]), u(t.no_parry[5]), u(t.no_parry[6]), u(kz.reverse_kept), u(kz.reverse_suppressed), u(t.counters),
                u(t.counter_expired), u(t.rest), u(t.poise), u(t.throws), u(t.spawns), u(t.spawn_dupes), u(t.restores), u(t.backsteps), u(t.early_throw), u(t.breaks), u(t.shield_preps), u(t.decisive_sword),
                u(t.decisive_helm), u(t.recovered), u(t.native_deaths), u(t.fallback_deaths), u(t.decisive_no_receipt), u(t.headlock_stripped), u(t.snaps), u(t.duplicates), u(t.gaps_skipped), u(t.watchdog), u(t.regen_ticks),
                u(t.ended[0]), u(t.ended[1]), u(t.ended[2]), u(t.ended[3]), u(t.ended[4]), u(t.ended[5]), u(t.ended[6]), u(t.ended[7]), u(t.ended[8]),
                u(t.bash_absorbed), u(t.bash_se), u(t.resumed), u(t.resume_skipped), u(t.immediate_breaks), u(t.sword_dupes), u(t.back_speed_fixes), u(t.rearmed),
                u(t.blocked_spear_loss), u(t.spear_break_prevented), u(t.other_spear_loss), u(t.spawns), u(t.counter_dispatched), u(t.counter_started),
                u(t.counter_cancel[0]), u(t.counter_cancel[1]), u(t.counter_cancel[2]), u(t.counter_cancel[3]), u(t.counter_cancel[4]), u(t.counter_cancel[5]),
                u(t.recoil_fixed), u(t.lunge_clamped), u(t.guard_guarantees),
                u(t.guard_active_blocks), u(t.skill_instant_suppressed), u(t.stagger_shield_closed), u(t.stagger_clip_kept), u(t.md_requests), u(t.md_started), u(t.md_kills),
                u(t.md_cancelled), u(t.md_extended), u(t.death_hp_zero), u(t.death_skill_instant), u(t.death_sword_decisive), u(t.death_helm), u(t.md_kills), u(t.death_iron), u(t.death_other),
                u(kz.spear_children), u(kz.perf.windows),
                kz.ident.verified ? (kz.ident.swapped ? "MEASURED_SWAPPED" : "MEASURED_DEFAULT") : (kz.ident.conflict ? "CONFLICT_DEFAULT_KEPT" : "DEFAULT_UNVERIFIED"),
                kz.ident.votes[0][1], kz.ident.votes[0][2], kz.ident.votes[1][1], kz.ident.votes[1][2], u(kz.hud_shown), u(kz.hud_missing), u(kz.hud_busy), [&] {
                  std::uint64_t d = 0;
                  for (auto x : kz.dropped) d += x;
                  return u(d);
                }());
  kkb_write(false, line);
  // FIX04: what the module did to the spear's contacts, the C3 wind-up and the guard (execution counters of this module, not proof of what was seen or heard)
  std::snprintf(line, sizeof(line),
                "CHILFOS_CONTACT_SUMMARY version=%s tag=%s attacks=%llu stale_cleared=%llu echo_suppressed=%llu se_merged=%llu se_self=%llu marks_kept=%llu marks_suppressed=%llu guard_marks=%llu "
                "parry_settle_post=%llu throw_eye_writes=%llu throw_eye_kept=%llu guard_raised=%llu guard_raise_fail=%llu guard_after_wound=%llu face_ticks=%llu "
                "telemetry=MODULE_EXECUTION_COUNTERS runtime=HUMAN_RUNTIME_PENDING",
                kk::kVersion, kk::kTag, u(t.contact_tokens), u(t.stale_cleared), u(t.echo_suppressed), u(t.se_merged), u(t.se_self), u(t.marks_kept), u(t.marks_suppressed), u(t.guard_marks),
                u(t.parry_settle_post), u(t.throw_eye_writes), u(t.throw_eye_kept), u(t.guard_raised), u(t.guard_raise_fail), u(t.guard_after_wound), u(t.face_ticks));
  kkb_write(false, line);
  // FIX05: the stagger's one play, the Jump Attack defense, the burst re-arm, the Helm Splitter aim, the Mortal Draw pose hold
  std::snprintf(line, sizeof(line),
                "CHILFOS_FIX05_SUMMARY version=%s tag=%s bow_plans=%llu bow_stretches=%llu bow_wraps=%llu recovered=%llu recovered_at_end=%llu jump_swings=%llu jump_land_merged=%llu/%llu "
                "jump_verdicts=%llu/%llu/%llu jump_rebound_air=%llu jump_rebound_land=%llu block_dupes=%llu burst_gap_ticks=%llu head_samples=%llu head_lifts=%llu helm=%llu md_pose_holds=%llu md_kills=%llu "
                "dropped_fix05=%llu telemetry=MODULE_EXECUTION_COUNTERS runtime=HUMAN_RUNTIME_PENDING",
                kk::kVersion, kk::kTag, u(t.bow_plans), u(t.bow_stretches), u(t.bow_wraps), u(t.recovered), u(t.bow_exit_end), u(t.jump_swings), u(t.jump_land_merged), u(kz.jump_land_merged),
                u(t.jump_parries), u(t.jump_blocks), u(t.jump_damage), u(t.jump_rebound_air), u(t.jump_rebound_land), u(t.block_dupes), u(t.burst_gap_ticks), u(t.head_samples), u(t.head_lifts),
                u(t.death_helm), u(t.md_pose_holds), u(t.md_kills), u(kz.dropped[kKkFix05]));
  kkb_write(false, line);
}
void kk_shutdown() noexcept {
  if (kz.enabled) kk_summary();
  kz.enabled = false;
}
void kk_install() noexcept {
  HookOptions late = HOOK_OPTIONS_INIT;
  late.priority = -401;  // after the Lizalfos receipt of the same Link damage call
  HookOptions after_liz = HOOK_OPTIONS_INIT;
  after_liz.priority = -50;
  ModResult r[] = {
      mods::hook::add_pre<KkExec>(on_kk_exec_pre),       mods::hook::add_post<KkExec>(on_kk_exec_post),         mods::hook::add_pre<KkDelete>(on_kk_delete_pre),
      mods::hook::add_pre<KkDamage>(on_kk_damage_pre),   mods::hook::add_post<KkDamage>(on_kk_damage_post),     mods::hook::add_pre<KkAttack>(on_kk_attack_pre),
      mods::hook::add_post<KkAttack>(on_kk_attack_post), mods::hook::add_pre<KkSetBck>(on_kk_setbck_pre),       mods::hook::add_pre<KkSetWeaponBck>(on_kk_setweapon_pre),
      mods::hook::add_post<KkBack>(on_kk_back_post),     mods::hook::add_pre<KkThrow>(on_kk_throw_pre),         mods::hook::add_post<KkThrow>(on_kk_throw_post),
      mods::hook::add_post<KkDmgExec>(on_kk_dmgexec_post), mods::hook::add_post<KkNext>(on_kk_next_post),      mods::hook::add_post<LinkCutType>(on_kk_cut_post, &after_liz),
      mods::hook::add_post<LinkCutReverse>(on_kk_cut_reverse_post, &after_liz), mods::hook::add_post<LinkDamageFix01>(on_kk_link_damage_post, &late),
      mods::hook::add_pre<KkCutCheck>(on_kk_cutcheck_pre),  mods::hook::add_post<LinkCutAction>(on_kk_cut_action_post, &after_liz),
      mods::hook::add_post<MeterDrawPost07>(on_kk_frame_post), mods::hook::add_pre<PfxProcHitmark>(on_kk_hitmark_pre, &after_liz)};
  bool ok = true;
  unsigned failed = 0;
  for (auto x : r)
    if (x != MOD_OK) ok = false, ++failed;
  kz.enabled = ok;  // all or nothing: without every hook the E_KK stays vanilla and the rest of the mod is untouched
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "CHILFOS event=INSTALL version=%s tag=%s enabled=%d hooks=%u failed=%u hp=%d posture=%d gains=13/5/3/5/30 enemy_parry_self=0 link=20/30/25 regen=12/8/5/2/1 delay=%u break=%u combos=6/6/8/7 "
                "actions=%u rates=D%.2f/E%.2f/Eb%.2f/Df%.2f/back%.2f/throw%.2f/restore%.2f pct=12/35/45 two_blocks=80 wound_guard=45/80/100 block_base=%d diagonal_bck=%d thrust_bck=%d identity=RUNTIME_TIP_TRAJECTORY",
                kk::kVersion, kk::kTag, ok ? 1 : 0, static_cast<unsigned>(sizeof(r) / sizeof(r[0])), failed, kk::policy::kMaxHp, kk::policy::kMaxPosture, kk::policy::kRegenDelayTicks, kk::policy::kBreakTicks,
                kk::total_actions(), kk::policy::kRateD, kk::policy::kRateE, kk::policy::kRateEBurst, kk::policy::kRateDFast, kk::policy::kRateBack, kk::policy::kRateThrow, kk::policy::kRateRestore,
                kk::policy::kBlockBase, kk::kBckDiagonal, kk::kBckThrust);
  kkb_write(!ok, line);
}
#undef KK_NOINLINE
