// LIGHT FADES TWICE 0.13.0 CHILFOS01: production adapter of the Chilfos (E_KK) duelist. Included once in mod.cpp's anonymous namespace after
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
//   kk_native_eye         eyePos.y for ONE executeSpearThrow call of the C3 throw (the native melee cancel would drop the spear), restored in the POST
//   kk_native_shield      mCyl TgShield: the prepared parry, replacing the native mCutTypeCheck(0) auto-deflect
//   kk_native_headlock    HeadLock off: stagger entry/exit, and a Shield Attack outside the stagger (Helm Splitter is the finisher, not a shortcut)
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
  bool in_dc = false, conv = false, decisive = false, shield_atk = false, spear_credit_lock = false;
  std::uint8_t saved_67d = 0;
  int hp_before = 0, act_before = 0, mode_before = 0;
  kk::V cur_v = kk::V::None;
  std::uint64_t cur_serial = 0;
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
struct KkRuntime {
  bool enabled = false;
  KkSlot slots[kk::kSlots]{};
  kk::Identity ident;
  std::uint64_t generations = 0, overflow = 0, deleted = 0, excluded = 0, admitted = 0, swing_serial = 0, swing_tick = ~0ull, hud_shown = 0, hud_missing = 0, hud_busy = 0;
  std::uint64_t link_gain = 0, link_saturations = 0, reverse_kept = 0, reverse_suppressed = 0;
  std::uint32_t stage = 0;
  kk::Stats totals{};
  unsigned lines[12] = {};
  std::uint64_t dropped[12] = {};
  bool self_bck = false;  // a setBck this module issues itself: the PRE leaves its arguments alone
  std::uint64_t maint_tick = ~0ull;
} kz;
enum KkLog : unsigned { kKkAdmit = 0, kKkCombo, kKkRhythm, kKkDefense, kKkLink, kKkSpear, kKkBreak, kKkFx, kKkAnim, kKkCritical, kKkPosture, kKkSummary };
inline constexpr unsigned kKkCaps[12] = {64, 400, 120, 400, 300, 200, 200, 120, 60, 80, 300, 40};
void kk_line(KkLog f, bool warn, const char* line) noexcept {
  if (kkb_take(kz.lines[f], kz.dropped[f], kKkCaps[f])) kkb_write(warn, line);
}
bool kk_alive() noexcept { return kz.enabled; }
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
  const int act = a->mActionMode;
  return kk_spear(a) && !s.brk.active() && act != kk::kYoroke && act != kk::kDamage && act != kk::kDead && act != kk::kIcicle && act != kk::kWeaponMove && !kk_attacking(a);
}
bool kk_link_has_rebound(daAlink_c* k) noexcept {  // the procs that run changeCutReverseProc with a recoil clip (FIX05 LINK_COMBO_MAP)
  const int proc = static_cast<int>(k->mProcID);
  return proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH;
}
// ---------------------------------------------------------------- event lines
void kk_log_ep_end(KkSlot& s, daE_KK_c* a, kk::End why) noexcept {
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_COMBO event=END actor=%u generation=%llu pattern=%u counter=%d cause=%s act=%u impacts=%llu tick=%llu posture=%d hp=%d action=%d", s.id,
                static_cast<unsigned long long>(s.generation), s.ep.pattern + 1, s.ep.counter ? 1 : 0, kk::name(why), s.ep.act + 1, static_cast<unsigned long long>(s.st.impacts_open),
                static_cast<unsigned long long>(kkb_tick()), s.p.points(), a->health, a->mActionMode);
  kk_line(kKkCombo, false, line);
}
void kk_end_episode(KkSlot& s, daE_KK_c* a, kk::End why) noexcept {
  if (!s.ep.active) return;
  ++s.st.ended[unsigned(why)];
  if (why == kk::End::Completed && !s.ep.counter) ++s.st.completed[s.ep.pattern];
  kk_log_ep_end(s, a, why);
  if (s.ep.ph == KkEp::Throw && s.eye_saved) kk_native_eye(a, s.eye_y), s.eye_saved = false;
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
  if (had != kk::V::None) return had;
  const std::uint64_t now = kkb_tick();
  kk::V v = kk::V::Damage;
  const bool frontal = kk_frontal(a, k);
  if (s.brk.active()) v = s.brk.sword_finishes(serial) ? kk::V::Decisive : kk::V::Ignored;
  else if (s.shield_on && s.def.presenting(now, serial) && frontal && kk_spear(a) && k && kk_link_has_rebound(k)) v = kk::V::EnemyParry;
  else if (kk_guard_ready(s, a) && frontal && s.rng.pct(s.def.block_pct(now))) v = kk::V::Block;
  s.sv.set(serial, v);
  ++s.st.verdict[unsigned(v)];
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=VERDICT actor=%u generation=%llu swing=%llu verdict=%s presented=%d frontal=%d spear=%d ready=%d block_pct=%d wounds=%u hp=%d posture=%d action=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(serial), kk::name(v), s.shield_on ? 1 : 0, frontal ? 1 : 0, kk_spear(a) ? 1 : 0, kk_guard_ready(s, a) ? 1 : 0,
                s.def.block_pct(now), s.def.wound_streak, a->health, s.p.points(), a->mActionMode, static_cast<unsigned long long>(now));
  kk_line(kKkDefense, false, line);
  return v;
}
void kk_apply_enemy_parry(KkSlot& s, daE_KK_c* a) noexcept {
  const std::uint64_t now = kkb_tick();
  ++s.st.enemy_parries;
  s.def.on_enemy_parry(now);
  kk_link_gain(s, kk::policy::kLinkEnemyParry, false, s.parry_serial, "ENEMY_PARRY");
  s.counter_pending = true, s.counter_until = now + kk::policy::kCounterTtl;
  if (s.ep.active) kk_end_episode(s, a, kk::End::Defense);
  s.rest_until = 0;
  if (a->mActionMode == kk::kWait) kk_native_wait(a, 0);  // a rest wait this module wrote must not delay the counter past its lifetime
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=ENEMY_PARRY actor=%u generation=%llu swing=%llu link_gain=+%d chilfos_gain=+0 rebound=NATIVE hp=%d posture=%d cooldown_until=%llu tick=%llu", s.id,
                static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(s.parry_serial), kk::policy::kLinkEnemyParry, a->health, s.p.points(),
                static_cast<unsigned long long>(s.def.cooldown_until), static_cast<unsigned long long>(now));
  kk_line(kKkDefense, false, line);
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
void kk_enter_break(KkSlot& s, daE_KK_c* a) noexcept {
  if (s.ep.active) kk_end_episode(s, a, kk::End::Posture100);
  s.counter_pending = false, s.def.prepared = false, s.parry_pending = false;
  s.brk.enter(kkb_tick());
  ++s.st.breaks;
  kk_native_shield(a, false), s.shield_on = false;
  kk_native_headlock(a);
  kk_native_action(a, kk::kYoroke, 0);  // executeYoroke mode 0: DAMAGE_BOW (with spear) or DAMAGE_BOWB, looped by the setBck PRE while the stagger lasts
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=STAGGER actor=%u generation=%llu window=%u hp=%d spear=%d latch_swing=%llu tick=%llu", s.id, static_cast<unsigned long long>(s.generation),
                kk::policy::kBreakTicks, a->health, kk_spear(a) ? 1 : 0, static_cast<unsigned long long>(s.brk.latch_serial), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkBreak, false, line);
}
void kk_exit_break(KkSlot& s, daE_KK_c* a) noexcept {
  ++s.st.recovered;
  const bool spear = kk_spear(a);
  s.brk.reset();
  s.p.reset();
  kk_native_headlock(a);
  kk_native_action(a, spear ? kk::kWait : kk::kBackWalk, 0);  // without its spear the native backstep restores it
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=RECOVERED actor=%u generation=%llu hp=%d posture=0 spear=%d next_action=%d tick=%llu", s.id, static_cast<unsigned long long>(s.generation), a->health,
                spear ? 1 : 0, a->mActionMode, static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkBreak, false, line);
}
void kk_maintenance() noexcept;
void kk_tick_maintenance() noexcept {  // once per simulation tick, from this module's own hooks (mod_update's frozen body is not touched)
  if (kz.maint_tick == kkb_tick()) return;
  kz.maint_tick = kkb_tick();
  kk_maintenance();
}
HookAction on_kk_exec_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  kk_tick_maintenance();
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  KkSlot* s = kk_find(a);
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  if (s && s->id != id) kk_retire(*s), s = nullptr;  // address reused by a new process: nothing is inherited
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
      if (s->ep.active) kk_end_episode(*s, a, kk::End::Death);
      s->brk.st = s->brk.st == kk::Bk::Broken ? kk::Bk::Dead : s->brk.st;
    }
    return HOOK_CONTINUE;
  }
  if (!kkb_world_safe()) {
    if (s->ep.active) kk_end_episode(*s, a, kk::End::Event);
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
  if (s->counter_pending && now > s->counter_until) s->counter_pending = false, ++s->st.counter_expired;
  // stagger
  if (s->brk.st == kk::Bk::Pending && a->health > 1 && a->mActionMode != kk::kIcicle && a->mActionMode != kk::kWeaponMove) kk_enter_break(*s, a);
  else if (s->brk.expired(now)) kk_exit_break(*s, a);
  // posture regeneration
  const int act = a->mActionMode;
  const bool eligible = !s->brk.active() && act != kk::kYoroke && act != kk::kDamage && act != kk::kDead;
  if (eligible && !s->p.latched && s->p.u > 0) ++s->st.regen_ticks;
  s->p.tick(a->health, eligible);
  // the episode against the native state
  if (s->ep.active) {
    if (now - s->ep.last_progress > kk::policy::kWatchdogTicks) {
      ++s->st.watchdog;
      kk_end_episode(*s, a, kk::End::Watchdog);
    } else if (s->ep.ph == KkEp::Melee && act != kk::kAttack) {
      kk_end_episode(*s, a, s->ep.closed && s->ep.act + 1 >= s->ep.n ? kk::End::Completed : kk::End::NativePriority);
    } else if (s->ep.ph == KkEp::Back) {
      if (act != kk::kBackWalk) kk_end_episode(*s, a, kk::End::NativePriority);
      else if (now >= s->ep.back_until) {  // the planned backstep is over: face Link and throw (executeSpearThrow mode 1)
        s->ep.ph = KkEp::Throw, s->ep.last_progress = now;
        kk_native_action(a, kk::kThrow, 1);
        ++s->st.throws;
      }
    } else if ((s->ep.ph == KkEp::Throw || s->ep.ph == KkEp::Restore) && act != kk::kThrow) {
      kk_end_episode(*s, a, s->ep.ph == KkEp::Restore && kk_spear(a) ? kk::End::Completed : kk::End::NativePriority);
    }
  }
  return HOOK_CONTINUE;
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
  kkb_pfx_settle(true, s->id, false);   // a preview no verdict of this Execute confirmed goes back to its own colour
  kkb_pfx_settle(false, s->id, false);
  kkb_pfx_end();
}
HookAction on_kk_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kz.enabled || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (s) {
    if (s->ep.active) kk_end_episode(*s, a, kk::End::SceneOrDelete);
    kk_retire(*s);
    ++kz.deleted;
  }
  return HOOK_CONTINUE;
}
// ---------------------------------------------------------------- damage_check: the one place a contact is judged
HookAction on_kk_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return HOOK_CONTINUE;
  s->in_dc = true, s->conv = s->decisive = s->shield_atk = false, s->cur_v = kk::V::None;
  s->hp_before = a->health, s->act_before = a->mActionMode, s->mode_before = a->mMoveMode, s->saved_67d = a->field_0x67d;
  if (a->health <= 1 || a->mActionMode == kk::kDead) return HOOK_CONTINUE;  // the native returns at once too
  daAlink_c* k = kkb_link();
  if (a->mDamageTimer != 0) return HOOK_CONTINUE;  // the native ignores every contact of this call
  kk_link_contact(*s, a, k);
  if (!a->mCyl.cCcD_ObjHitInf::ChkTgHit()) return HOOK_CONTINUE;
  const cCcD_Obj* o = a->mCyl.cCcD_ObjHitInf::GetTgHitObj();
  if (!o) return HOOK_CONTINUE;
  if (o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) {
    s->shield_atk = true;
    if (s->brk.active()) {
      s->brk.shield_attack();
      ++s->st.shield_preps;
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=SHIELD_PREP actor=%u generation=%llu age=%llu headlock=NATIVE tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                    static_cast<unsigned long long>(kkb_tick() - s->brk.start), static_cast<unsigned long long>(kkb_tick()));
      kk_line(kKkBreak, false, line);
    } else if (s->ep.active) {
      kk_end_episode(*s, a, kk::End::NativePriority);  // the native DRAWBACK stagger takes over
    }
    return HOOK_CONTINUE;
  }
  if (!o->ChkAtType(kkb_sword_mask()) || !k || const_cast<cCcD_Obj*>(o)->GetAc() != static_cast<fopAc_ac_c*>(k)) {
    if (s->ep.active && !o->ChkAtType(kkb_iron_mask())) kk_end_episode(*s, a, kk::End::NativePriority);  // arrows, boomerang, hookshot, bombs: native
    return HOOK_CONTINUE;
  }
  const std::uint64_t serial = kz.swing_serial;
  s->cur_serial = serial;
  s->cur_v = kk_decide(*s, a, k, serial);
  switch (s->cur_v) {
    case kk::V::Ignored:  // the very swing that raised posture to 100 (or a second record of a decided swing during the stagger): no second effect
      ++s->st.ignored;
      a->mCyl.cCcD_ObjHitInf::ClrTgHit();
      break;
    case kk::V::Decisive:
      if (k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP) break;  // Helm Splitter: its own native lethal branch (mCutTypeCheck(2)), nothing written
      s->decisive = true, s->conv = true;  // any other NEW sword swing on the staggered Chilfos: the native lethal branch (`health <= 1` after cc_at_check)
      kk_native_spear(a, 1);
      if (a->health > 2) kk_native_health(a, 2);
      break;
    case kk::V::Damage:  // uncovered: the native spear-less branch for this one call (cc_at_check, hit mark, damage SE)
      if (kk_spear(a)) s->conv = true, kk_native_spear(a, 1);
      break;
    case kk::V::EnemyParry:
      kkb_pfx_settle(false, s->id, true);
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
    if (!died) {  // never seen in the transcription; logged, and the iron-ball branch's own two writes finish it
      ++s->st.fallback_deaths;
      kk_native_health(a, 0);
      kk_native_action(a, kk::kDead, 0);
    }
    ++s->st.decisive_sword;
    std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=SWORD_DECISIVE actor=%u generation=%llu swing=%llu hp_before=%d route=%s tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                  static_cast<unsigned long long>(s->cur_serial), s->hp_before, died ? "NATIVE_LETHAL_BRANCH" : "FALLBACK_HEALTH0_ACTION10", static_cast<unsigned long long>(now));
    kk_line(kKkBreak, !died, line);
  }
  if (died) {
    ++s->st.native_deaths;
    const bool helm = k && k->getCutType() == daPy_py_c::CUT_TYPE_HEAD_JUMP;
    if (s->brk.active() && helm && s->brk.shield_prep) {
      ++s->st.decisive_helm;
      std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=HELM_HIT actor=%u generation=%llu age=%llu tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                    static_cast<unsigned long long>(now - s->brk.start), static_cast<unsigned long long>(now));
      kk_line(kKkBreak, false, line);
    }
    std::snprintf(line, sizeof(line), "CHILFOS_BREAK event=NATIVE_DEATH actor=%u generation=%llu route=%s hp_before=%d broken=%d tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                  s->decisive ? "SWORD_DECISIVE" : (s->brk.active() && helm) ? "HELM_SPLITTER" : "NATIVE", s->hp_before, s->brk.active() ? 1 : 0, static_cast<unsigned long long>(now));
    kk_line(kKkBreak, false, line);
    if (s->ep.active) kk_end_episode(*s, a, kk::End::Death);
    if (s->brk.active()) s->brk.st = kk::Bk::Dead;
    s->dead = true;
  }
  if (!died) {
    switch (s->cur_v) {
      case kk::V::Damage:
        if (a->health < s->hp_before) {
          ++s->st.sword_damage;
          s->def.on_wound(now);
          kk_posture_gain(*s, a, kk::policy::kGainSwordHp, "SWORD_HP", s->cur_serial);
          if (s->ep.active && s->ep.ph == KkEp::Melee && a->mActionMode == kk::kDamage) {  // poise: the combo keeps its clip, HP and hit mark stay native
            ++s->st.poise;
            kk_native_action(a, kk::kAttack, 3);
          }
        }
        break;
      case kk::V::Block:
        if (a->mActionMode == kk::kDamage && a->mMoveMode == 0 && s->saved_67d == 0) {  // a strong hit broke the spear natively (CLASH): +5 once, no HP lost
          if (!s->spear_credit_lock) {
            s->spear_credit_lock = true;
            ++s->st.spear_breaks;
            kk_posture_gain(*s, a, kk::policy::kGainSpearBreak, "SPEAR_BREAK", s->cur_serial);
          }
          if (s->ep.active) kk_end_episode(*s, a, kk::End::NativePriority);
        } else {
          ++s->st.blocks;
          s->def.on_block(now, s->cur_serial);
          kk_posture_gain(*s, a, kk::policy::kGainBlock, "BLOCK", s->cur_serial);
          if (s->ep.active && a->mActionMode == kk::kGuard) kk_end_episode(*s, a, kk::End::Defense);
        }
        break;
      case kk::V::EnemyParry:
        s->parry_serial = s->cur_serial;
        if (s->sv.rebound_seen) kk_apply_enemy_parry(*s, a);
        else s->parry_pending = true, s->parry_pending_tick = now;  // Link's clip may check its rebound after this Execute: settled next tick
        break;
      default: break;
    }
    if (s->shield_atk && !s->brk.active() && a->checkHeadLockFlg()) {  // outside the stagger a Shield Attack staggers natively but does not open Helm Splitter
      ++s->st.headlock_stripped;
      kk_native_headlock(a);
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
    ++s.st.counters;
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
      if (p.burst) kkb_burst_gap(s->id);  // the D1->D2 fast re-arm of Link's shield chain for this burst pair
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
    if (c.duplicate) ++s->st.duplicates;
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
// ---------------------------------------------------------------- clip calls: our clip and rate, the looped stagger, the C3 tail
HookAction on_kk_setbck_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args || kz.self_bck) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted) return HOOK_CONTINUE;
  int& idx = mods::arg_ref<int>(args, 1);
  u8& attr = mods::arg_ref<u8>(args, 2);
  float& rate = mods::arg_ref<float>(args, 4);
  if (s->brk.active() && (idx == kk::policy::kBckDamageBow || idx == kk::policy::kBckDamageBowB || idx == kk::policy::kBckDrawback)) {
    attr = 2;  // J3D loop: the native Yoroke waits for isStop, so the stagger holds until this module ends it (90 ticks)
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
  if (!s || !s->admitted || !s->ep.active || s->ep.ph != KkEp::Back || a->mMoveMode != 3) return;
  ++s->st.early_throw, ++s->st.throws;
  s->ep.ph = KkEp::Throw, s->ep.last_progress = kkb_tick();
  kk_native_action(a, kk::kThrow, 1);
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=BACKSTEP_STOPPED actor=%u generation=%llu reason=NATIVE_WALL_CLIFF_OR_TIMER left=%lld tick=%llu", s->id, static_cast<unsigned long long>(s->generation),
                static_cast<long long>(s->ep.back_until) - static_cast<long long>(kkb_tick()), static_cast<unsigned long long>(kkb_tick()));
  kk_line(kKkSpear, false, line);
}
HookAction on_kk_throw_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!kk_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->ep.active || s->ep.ph != KkEp::Throw) return HOOK_CONTINUE;
  s->d_before = a->field_0x67d;
  if (a->mMoveMode == 2 && !s->eye_saved) {  // the native melee cancel (Link within 900 and frame < 23) would drop the planned throw
    s->eye_saved = true, s->eye_y = a->eyePos.y;
    kk_native_eye(a, -1.0e9f);
  }
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
  auto* a = mods::arg<daE_KK_c*>(args, 0);
  KkSlot* s = a ? kk_find(a) : nullptr;
  if (!s || !s->admitted || !s->ep.active) return;
  if (s->eye_saved) kk_native_eye(a, s->eye_y), s->eye_saved = false;
  if (s->clip.snapped && a->mpMorfSO) kk_native_frame(a->mpMorfSO, s->clip.true_frame), s->clip.snapped = false;
  if (s->ep.ph == KkEp::Throw && s->d_before == 0 && a->field_0x67d == 1 && a->field_0x764 == kk::policy::kBckThrow) {
    ++s->spawns, ++s->st.spawns;
    if (s->spawns > 1) ++s->st.spawn_dupes;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS_SPEAR event=THROW_SPAWN actor=%u generation=%llu count=%u rate=%.2f frame=%.2f tick=%llu", s->id, static_cast<unsigned long long>(s->generation), s->spawns,
                  a->mpMorfSO ? a->mpMorfSO->getPlaySpeed() : 0.0f, kk_frame(a), static_cast<unsigned long long>(kkb_tick()));
    kk_line(kKkSpear, s->spawns > 1, line);
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
  if (s->ep.active && s->ep.ph == KkEp::Melee && s->ep.closed && s->ep.act + 1 >= s->ep.n && !s->ep.plan.tail.on) kk_end_episode(*s, a, kk::End::Completed);  // the native ends the last clip
  if (s->ep.active || s->counter_pending) return;
  if (a->mActionMode == kk::kAttack && now < s->rest_until) {
    ++s->st.rest;
    kk_native_action(a, kk::kWait, 0);
    kk_native_wait(a, static_cast<std::int16_t>(s->rest_until - now > 32767 ? 32767 : s->rest_until - now));
  }
}
// ---------------------------------------------------------------- Link: swing start, rebound of his sword, native damage receipt
void on_kk_cut_post(ModContext*, void* args, void*, void*) noexcept {
  if (!kz.enabled || !args) return;
  auto* k = mods::arg<daAlink_c*>(args, 0);
  const int t = mods::arg<u8>(args, 1);
  if (!k || k != kkb_link() || k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)) return;
  if (t == daPy_py_c::CUT_TYPE_NONE || t == daPy_py_c::CUT_TYPE_GUARD_ATTACK) return;
  const std::uint64_t now = kkb_tick();
  if (kz.swing_tick == now) return;
  kz.swing_tick = now;
  ++kz.swing_serial;
  for (auto& s : kz.slots) {
    if (!s.on || !s.admitted || s.dead) continue;
    auto* a = reinterpret_cast<daE_KK_c*>(s.key);
    const int to_kk = kk_link_angle(a, k) + 0x8000;
    if (kk::angle_diff(k->shape_angle.y, to_kk) > kk::policy::kTargetArc || kk_link_dist(a, k) > kk::policy::kTargetDist) continue;
    const kk::Np why = s.def.on_swing(now, kz.swing_serial, kk_spear(a), kk_attacking(a), kk_frontal(a, k), s.brk.active() || s.brk.st == kk::Bk::Pending, s.rng);
    ++s.st.no_parry[unsigned(why)];
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "CHILFOS_DEFENSE event=%s actor=%u generation=%llu swing=%llu cut=%d level=%c pct_two_blocks=%d reason=%s tick=%llu", why == kk::Np::None ? "PARRY_PREPARED" : "ROLL",
                  s.id, static_cast<unsigned long long>(s.generation), static_cast<unsigned long long>(kz.swing_serial), t, "ABC"[s.def.level(now)], s.def.two_blocks(now) ? 1 : 0, kk::name(why),
                  static_cast<unsigned long long>(now));
    kk_line(kKkDefense, false, line);
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
                "native_deaths=%llu fallback_deaths=%llu headlock_stripped=%llu snaps=%llu duplicates=%llu gaps=%llu watchdog=%llu regen_ticks=%llu ends=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
                "identity=%s votes5=%u/%u votes6=%u/%u hud=%llu/%llu/%llu dropped=%llu runtime=HUMAN_RUNTIME_PENDING",
                kk::kVersion, kk::kTag, u(kz.admitted), u(kz.excluded), u(kz.deleted), u(kz.overflow), u(t.selected[0]), u(t.selected[1]), u(t.selected[2]), u(t.selected[3]), u(t.completed[0]), u(t.completed[1]),
                u(t.completed[2]), u(t.completed[3]), u(t.actions), u(t.impacts_open), u(t.impacts_closed), u(t.link_parry), u(t.link_block), u(t.link_direct_offered), u(kz.link_gain), u(kz.link_saturations),
                u(t.sword_damage), u(t.blocks), u(t.enemy_parries), u(t.downgraded_parries), u(t.ignored), u(t.spear_breaks), u(t.verdict[1]), u(t.verdict[2]), u(t.verdict[3]), u(t.verdict[4]), u(t.verdict[5]),
                u(t.no_parry[0]), u(t.no_parry[1]), u(t.no_parry[2]), u(t.no_parry[3]), u(t.no_parry[4]), u(t.no_parry[5]), u(t.no_parry[6]), u(kz.reverse_kept), u(kz.reverse_suppressed), u(t.counters),
                u(t.counter_expired), u(t.rest), u(t.poise), u(t.throws), u(t.spawns), u(t.spawn_dupes), u(t.restores), u(t.backsteps), u(t.early_throw), u(t.breaks), u(t.shield_preps), u(t.decisive_sword),
                u(t.decisive_helm), u(t.recovered), u(t.native_deaths), u(t.fallback_deaths), u(t.headlock_stripped), u(t.snaps), u(t.duplicates), u(t.gaps_skipped), u(t.watchdog), u(t.regen_ticks),
                u(t.ended[0]), u(t.ended[1]), u(t.ended[2]), u(t.ended[3]), u(t.ended[4]), u(t.ended[5]), u(t.ended[6]), u(t.ended[7]), u(t.ended[8]),
                kz.ident.verified ? (kz.ident.swapped ? "MEASURED_SWAPPED" : "MEASURED_DEFAULT") : (kz.ident.conflict ? "CONFLICT_DEFAULT_KEPT" : "DEFAULT_UNVERIFIED"),
                kz.ident.votes[0][1], kz.ident.votes[0][2], kz.ident.votes[1][1], kz.ident.votes[1][2], u(kz.hud_shown), u(kz.hud_missing), u(kz.hud_busy), [&] {
                  std::uint64_t d = 0;
                  for (auto x : kz.dropped) d += x;
                  return u(d);
                }());
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
      mods::hook::add_post<LinkCutReverse>(on_kk_cut_reverse_post, &after_liz), mods::hook::add_post<LinkDamageFix01>(on_kk_link_damage_post, &late)};
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
