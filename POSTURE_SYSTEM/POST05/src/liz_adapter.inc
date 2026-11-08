// LIZALFOS DUELIST 0.12.0 production adapter. Included once inside mod.cpp's anonymous namespace after the CP06 FIX01 adapter.
// Every decision lives in liz_core.hpp (liz::Brain); this file only reads native fields by value, calls the Brain from the native hook and applies Write.
// E_OC, Link's own hooks and every existing writer are not touched. Native writes live in the LIZ_NOINLINE functions below (hash-audited by ci/liz_audit.py):
//   liz_native_fields     action/mode/timer[2]/invulnerability/current.angle.y/current.pos of THIS e_dn_class, inside its own Execute, before CrrPos
//   liz_native_anim       morf frame field and play speed of THIS actor's own morf (mode-0 start of a combo action, guard native exit)
//   liz_native_hp         health + max health, once per admitted identity, only from the exact native 100/100 of daE_DN_Create
//   liz_native_guard      the exact field set of the native guard entry in e_dn_fight_run
//   liz_native_break      at_info.mHitDirection + the ORIGINAL big_damage() + the invulnerability its native caller writes
//   liz_native_tg_clear   the inline Tg-hit clear of a cc_sph whose Tg mark is Link's Shield Attack (native DRAWBACK below posture 100)
#if defined(_MSC_VER)
#define LIZ_NOINLINE __declspec(noinline)
#else
#define LIZ_NOINLINE __attribute__((noinline))
#endif
struct LizSlot {
  bool on = false;
  std::uintptr_t key = 0;
  std::uint32_t id = 0;
  liz::Brain b;
};
struct LizRuntime {
  bool enabled = false;
  LizSlot slots[liz::kSlots]{};
  std::uint64_t generations = 0, swing_serial = 0, swing_tick = ~0ull, overflow = 0, deleted = 0, excluded = 0, admitted = 0;
  std::uint32_t stage = 0;
  std::uint32_t d1_parry_id = 0;
  std::uint64_t double_gap_tick = ~0ull, double_rearms = 0, link_gain = 0, link_saturations = 0, break_retry_until = 0, break_retries = 0;
  std::uint32_t retry_attacker = 0;
  std::uint64_t retry_generation = 0, hud_missing = 0, hud_shown = 0, hud_slot_busy = 0;
  bool swing_jump = false;
  lnks1fix::DirectJoin direct;
  liz::Stats totals{};
  std::uint64_t rolls[3] = {}, attempts[3] = {};
  unsigned lines[10] = {};
  std::uint64_t dropped[10] = {};
} lz;
enum LizLog : unsigned { kLogAdmit = 0, kLogCombo, kLogSwing, kLogDefense, kLogParry, kLogTurn, kLogBreak, kLogPosture, kLogCritical, kLogLink };
inline constexpr unsigned kLizCaps[10] = {64, 240, 400, 240, 240, 120, 120, 120, 80, 160};
void liz_line(LizLog fam, LogLevel lvl, const char* line) noexcept {
  if (lnk_take(lz.lines[fam], lz.dropped[fam], kLizCaps[fam])) lnk_write(lvl, line);
}
bool liz_alive() noexcept { return lz.enabled && !g.eng.obs.disabled; }
LizSlot* liz_find(const e_dn_class* a) noexcept {
  const std::uintptr_t k = reinterpret_cast<std::uintptr_t>(a);
  for (auto& s : lz.slots)
    if (s.on && s.key == k) return &s;
  return nullptr;
}
void liz_summary() noexcept;
void liz_merge(liz::Stats& t, std::uint64_t* rolls, std::uint64_t* atts, const liz::Brain& b) noexcept {
  const auto& x = b.st;
  for (unsigned i = 0; i < liz::kPatterns; ++i) t.selected[i] += x.selected[i], t.completed[i] += x.completed[i];
  for (unsigned i = 0; i <= liz::kPatterns; ++i) t.impacts_open[i] += x.impacts_open[i];
  for (unsigned i = 0; i < unsigned(liz::End::Count); ++i) t.ended[i] += x.ended[i];
  for (unsigned i = 0; i < unsigned(liz::Hit::Count); ++i) t.contact[i] += x.contact[i];
  t.link_parry += x.link_parry, t.d_pair_parry += x.d_pair_parry, t.sword_hits += x.sword_hits, t.iron_hits += x.iron_hits, t.blocks += x.blocks, t.enemy_parries += x.enemy_parries;
  t.cancels_suppressed += x.cancels_suppressed, t.stagger_suppressed_small += x.stagger_suppressed_small, t.stagger_suppressed_big += x.stagger_suppressed_big, t.shield_bash_cleared += x.shield_bash_cleared;
  t.guards_native += x.guards_native, t.guards_attempt += x.guards_attempt, t.guards_boundary += x.guards_boundary, t.watchdog += x.watchdog, t.sample_gap += x.sample_gap, t.windows += x.windows;
  t.counter_short += x.counter_short, t.initiative += x.initiative, t.reposition += x.reposition;
  for (unsigned i = 0; i < 3; ++i) rolls[i] += b.def.rolls[i], atts[i] += b.def.attempts[i];
}
void liz_retire(LizSlot& s) noexcept {
  if (s.on && s.b.admitted) liz_merge(lz.totals, lz.rolls, lz.attempts, s.b);
  s.on = false;
}
// ---------------------------------------------------------------- writers
LIZ_NOINLINE void liz_native_fields(e_dn_class* a, const liz::Write& w) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  if (w.set_action) a->action = w.action;
  if (w.set_mode) a->mode = w.mode;
  if (w.set_invuln) a->invulnerability_timer = w.invuln;
  if (w.set_timer2) a->timer[2] = w.timer2;
  if (w.set_cur_y) ac->current.angle.y = w.cur_y;
  if (w.fwd != 0.0f) {
    float sn = 0, cs = 1;
    cp06f1::sincos16(w.fwd_angle, sn, cs);
    ac->current.pos.x += sn * w.fwd;
    ac->current.pos.z += cs * w.fwd;
  }
}
LIZ_NOINLINE void liz_native_anim(mDoExt_McaMorfSO* m, bool set_frame, float frame, bool set_rate, float rate) noexcept {
  if (set_frame) m->mFrameCtrl.mFrame = frame;
  if (set_rate) m->setPlaySpeed(rate);
}
LIZ_NOINLINE void liz_native_hp(e_dn_class* a) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  ac->field_0x560 = liz::policy::kMaxHp;
  ac->health = liz::policy::kMaxHp;
}
LIZ_NOINLINE void liz_native_guard(e_dn_class* a) noexcept {  // e_dn_fight_run: field_0x708=30, field_0x70c=shape_angle.y, timer[0]=0, invulnerability_timer=15
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  a->field_0x708 = 30.0f;
  a->field_0x70c = ac->shape_angle.y;
  a->timer[0] = 0;
  a->invulnerability_timer = 15;
}
LIZ_NOINLINE void liz_native_break(e_dn_class* a) noexcept {
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  a->at_info.mHitDirection.y = a->search_angle_y;  // cc_at_check's value for a hit from Link (enemy->attacker direction)
  LizBig::g_orig(a);
  a->invulnerability_timer = 1000;
  ac->speedF = 0.0f;
}
LIZ_NOINLINE void liz_native_tg_clear(e_dn_class* a) noexcept {
  for (int i = 0; i < 3; ++i) {
    auto& s = a->cc_sph[i];
    if (!s.cCcD_ObjHitInf::ChkTgHit()) continue;
    const cCcD_Obj* o = s.cCcD_ObjHitInf::GetTgHitObj();
    if (o && o->ChkAtType(AT_TYPE_SHIELD_ATTACK)) s.cCcD_ObjHitInf::ClrTgHit();
  }
}
void liz_apply(e_dn_class* a, const liz::Write& w) noexcept {
  if (!w.any) return;
  if (w.set_hp) liz_native_hp(a);
  if (w.clear_tg) liz_native_tg_clear(a);
  if (w.big_damage) {
    liz_native_break(a);
    return;
  }
  if (w.set_action || w.set_mode || w.set_invuln || w.set_timer2 || w.set_cur_y || w.fwd != 0.0f) liz_native_fields(a, w);
  if (w.guard_entry) liz_native_guard(a);
  if (a->anm_p && (w.set_frame || w.set_rate)) liz_native_anim(a->anm_p, w.set_frame, w.frame, w.set_rate, w.rate);
}
// ---------------------------------------------------------------- reads
bool liz_stage_is(const char* have, const char* want) noexcept {
  if (!have) return false;
  for (unsigned i = 0; i < 16; ++i) {
    if (have[i] != want[i]) return false;
    if (!want[i]) return true;
  }
  return false;
}
liz::View liz_view(e_dn_class* a, daAlink_c* k) noexcept {
  liz::View v;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  v.action = a->action, v.mode = a->mode, v.hp = ac->health, v.max_hp = ac->field_0x560, v.invuln = a->invulnerability_timer, v.timer2 = a->timer[2], v.ut2 = a->unk_timer_2;
  v.search_y = a->search_angle_y, v.shape_y = ac->shape_angle.y, v.cur_y = ac->current.angle.y, v.fc = a->frame_counter, v.at_chk = a->at_chk_flag, v.tail = a->tail_at_sph_flag;
  v.anm_no = a->anm_no, v.pl_dir = a->pl_dir, v.recoil = a->field_0x74c;
  if (a->anm_p) v.frame = a->anm_p->getFrame(), v.rate = a->anm_p->getPlaySpeed(), v.end = a->anm_p->getEndFrame();
  v.event = !cp04_world_safe();
  v.link_ok = k != nullptr;
  v.link_human = k && !k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);
  v.horse = k && k->checkHorseRide();
  v.ground = a->objacch.ChkGroundHit() != 0;
  const cXyz& c = a->at_sph.GetC();
  v.at_c[0] = c.x, v.at_c[1] = c.y, v.at_c[2] = c.z, v.at_r = a->at_sph.GetR();
  return v;
}
void liz_contact_reads(e_dn_class* a, daAlink_c* k, liz::View& v) noexcept {
  auto& s = a->at_sph;
  if (s.cCcD_ObjHitInf::ChkAtHit()) {
    v.at_hit = true;
    v.at_shield = s.ChkAtShieldHit();
    const void* obj = static_cast<const void*>(s.cCcD_ObjHitInf::GetAtHitObj());
    for (int i = 0; i < 3 && k; ++i) v.at_on_link = v.at_on_link || obj == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mTgCyls[i]));
  }
  auto& t = a->tate_sph;
  if (t.cCcD_ObjHitInf::ChkTgHit()) {
    v.tate_hit = true;
    const cCcD_Obj* o = t.cCcD_ObjHitInf::GetTgHitObj();
    bool sword = o && o->ChkAtType(kMasks.sword) != 0;
    bool own = false;
    for (int i = 0; i < 3 && k; ++i) own = own || static_cast<const void*>(o) == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mAtCps[i]));
    if (k && static_cast<const void*>(o) == static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mAtCyl))) own = true;  // spin attacks use the cylinder
    v.tate_link_sword = sword && own;
  }
}
liz::LinkFrame liz_link_frame(daAlink_c* k) noexcept {
  liz::LinkFrame L;
  L.swing_serial = lz.swing_serial;
  L.jump_cut = lz.swing_jump;
  if (k) L.shape_y = static_cast<fopAc_ac_c*>(k)->shape_angle.y;
  return L;
}
// ---------------------------------------------------------------- event drain: telemetry and Link's own posture (the ONLY place this module touches lg.*)
void liz_link_gain(LizSlot& s, int points, lnk05::Kind kind, std::uint64_t token) noexcept {
  const lnk01::Outcome o = lg.posture.on_external(points);
  if (o.v != lnk01::Verdict::Confirmed) return;
  ++lz.link_gain;
  if (o.saturated) ++lz.link_saturations;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LIZ_LINK event=POSTURE_GAIN actor=%u generation=%llu kind=%s points=%d before=%d.%03d after=%d.%03d saturated=%d tick=%llu", s.id,
                static_cast<unsigned long long>(s.b.generation), lnk05::name(kind), points, o.before / 1000, o.before % 1000, o.after / 1000, o.after % 1000, o.saturated ? 1 : 0,
                static_cast<unsigned long long>(g.p8.tick));
  liz_line(kLogLink, LOG_LEVEL_INFO, line);
  if (o.after >= lnk01::policy::kMaxMilli) {
    lnk05_request(kind, s.id, s.b.generation, token, o.before, o.after);
    // Link may be mid-cut: the request's own gate only fires in a safe native origin. Offer it again while he stays saturated, never a forced state.
    lz.break_retry_until = g.p8.tick + 90;
    lz.retry_attacker = s.id, lz.retry_generation = s.b.generation;
  }
}
void liz_drain(LizSlot& s, e_dn_class* a) noexcept {
  auto& b = s.b;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  char line[kLogLineMax];
  for (unsigned i = 0; i < b.nev; ++i) {
    const liz::Ev& e = b.ev[i];
    switch (e.k) {
      case liz::EvK::Admitted:
        ++lz.admitted;
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=ADMITTED version=%s actor=%u generation=%llu arg0=%d arg1=%d hp=%d max=%d action=%d", liz::kVersion, s.id,
                      static_cast<unsigned long long>(b.generation), e.b, e.c, ac->health, ac->field_0x560, a->action);
        liz_line(kLogAdmit, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Excluded:
        ++lz.excluded;
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=EXCLUDED_VANILLA reason=%s actor=%u arg0=%d arg1=%d action=%d", liz::name(static_cast<liz::Excl>(e.a)), s.id, e.b, e.c, a->action);
        liz_line(kLogAdmit, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::HpSet:
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=HP_SET actor=%u generation=%llu hp=%d from=%d/%d once=1", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c);
        liz_line(kLogAdmit, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ComboStart:
        std::snprintf(line, sizeof(line), "LIZ_COMBO event=START actor=%u generation=%llu pattern=%d acts=%d impacts=%d token=%llu tick=%llu posture=%d hp=%d", s.id,
                      static_cast<unsigned long long>(b.generation), e.a + 1, e.b, e.c, static_cast<unsigned long long>(b.ep.token), static_cast<unsigned long long>(e.t), b.p.points(), ac->health);
        liz_line(kLogCombo, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ComboEnd:
        std::snprintf(line, sizeof(line), "LIZ_COMBO event=END actor=%u generation=%llu pattern=%d cause=%s impacts_opened=%d tick=%llu posture=%d hp=%d action=%d", s.id,
                      static_cast<unsigned long long>(b.generation), e.a + 1, liz::name(static_cast<liz::End>(e.b)), e.c, static_cast<unsigned long long>(e.t), b.p.points(), ac->health, a->action);
        liz_line(kLogCombo, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ActionStart:
        std::snprintf(line, sizeof(line), "LIZ_COMBO event=ACTION actor=%u pattern=%d act=%d kind=%d start_frame=%.2f rate=%.2f tick=%llu dist=%.1f", s.id, e.a + 1, e.b, e.c, e.f, e.g,
                      static_cast<unsigned long long>(e.t), a->pl_dir);
        liz_line(kLogCombo, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::ImpactOpen:
        std::snprintf(line, sizeof(line), "LIZ_SWING event=OPEN actor=%u pattern=%d impact=%d kind=%d frame=%.2f rate=%.2f tick=%llu dist=%.1f r=%.1f atp=%d", s.id, e.a + 1, e.b, e.c, e.f, e.g,
                      static_cast<unsigned long long>(e.t), a->pl_dir, a->at_sph.GetR(), a->at_sph.GetAtAtp());
        liz_line(kLogSwing, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::WindowClose:
        std::snprintf(line, sizeof(line), "LIZ_SWING event=CLOSE actor=%u pattern=%d impact=%d frame=%.2f max_center_step=%.1f tick=%llu", s.id, e.a + 1, e.b, e.f, e.g, static_cast<unsigned long long>(e.t));
        liz_line(kLogSwing, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Contact:
        std::snprintf(line, sizeof(line), "LIZ_SWING event=CONTACT actor=%u generation=%llu result=%s pattern=%d impact=%d link_delta=%.2f frame=%.2f tick=%llu posture=%d", s.id,
                      static_cast<unsigned long long>(b.generation), liz::name(static_cast<liz::Hit>(e.a)), e.b + 1, e.c, e.f, e.g, static_cast<unsigned long long>(e.t), b.p.points());
        liz_line(kLogSwing, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::LinkBlock: liz_link_gain(s, e.a, lnk05::Kind::Block, b.cur_impact); break;
      case liz::EvK::LinkGain: liz_link_gain(s, e.a, lnk05::Kind::Direct, b.cur_impact); break;
      case liz::EvK::LinkDirectOffer: {
        lnk01::Contact k;
        k.result = post08::Result::NotBlocked, k.token = b.cur_impact ? b.cur_impact : 1, k.slot = static_cast<int>(&s - lz.slots), k.id = s.id, k.generation = b.generation, k.atp = e.b;
        k.link_ok = true, k.link_human = true, k.link_id = lg.posture.link_id(), k.tick = g.p8.tick;
        lz.direct.offer(k, reinterpret_cast<std::uintptr_t>(static_cast<const cCcD_Obj*>(&a->at_sph)), 0, true);
        break;
      }
      case liz::EvK::Roll:
        std::snprintf(line, sizeof(line), "LIZ_DEFENSE event=ROLL actor=%u level=%c pct=%d attempt=%d tick=%llu cooldown_ok=%d unused=%u dist=%.1f", s.id, "ABC"[e.a < 0 ? 0 : e.a > 2 ? 2 : e.a], e.c, e.b,
                      static_cast<unsigned long long>(e.t), b.def.cooled(b.local) ? 1 : 0, b.def.unused, a->pl_dir);
        liz_line(kLogDefense, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::GuardEntry:
        std::snprintf(line, sizeof(line), "LIZ_DEFENSE event=GUARD_ENTRY actor=%u level=%d mode=%d attempt=%d tick=%llu", s.id, e.a, e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogDefense, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Block:
        std::snprintf(line, sizeof(line), "LIZ_DEFENSE event=ORDINARY_BLOCK actor=%u generation=%llu gain=+%d posture_before=%d hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, ac->health,
                      static_cast<unsigned long long>(e.t));
        liz_line(kLogDefense, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::EnemyParry:
        std::snprintf(line, sizeof(line), "LIZ_PARRY event=ENEMY_PARRY actor=%u generation=%llu link_gain=+%d level=%d guard_age=%d hp=%d tick=%llu cooldown_until=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c,
                      ac->health, static_cast<unsigned long long>(e.t), static_cast<unsigned long long>(b.def.cooldown_until));
        liz_line(kLogParry, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Initiative:
        std::snprintf(line, sizeof(line), "LIZ_TURN event=INITIATIVE actor=%u choice=%s until=%llu tick=%llu", s.id, e.a == 1 ? "SHORT_NORMAL" : e.a == 2 ? "ONE_OF_FOUR" : "REPOSITION", static_cast<unsigned long long>(b.init_until),
                      static_cast<unsigned long long>(e.t));
        liz_line(kLogTurn, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Break:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=LATCH actor=%u generation=%llu posture=%d hp=%d native_knockdown=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Rearm:
        std::snprintf(line, sizeof(line), "LIZ_BREAK event=REARM actor=%u generation=%llu posture=%d next_action=%d mode=%d hp=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation), e.a, e.b, e.c, ac->health, static_cast<unsigned long long>(e.t));
        liz_line(kLogBreak, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::PostureGain:
        std::snprintf(line, sizeof(line), "LIZ_POSTURE event=GAIN actor=%u generation=%llu source=%s gain=+%d posture=%d hp=%d latched=%d tick=%llu", s.id, static_cast<unsigned long long>(b.generation),
                      e.c == 0 ? "LINK_PARRY" : e.c == 1 ? "SWORD" : e.c == 2 ? "IRON_BALL" : "BLOCK", e.a, e.b, ac->health, b.p.latched ? 1 : 0, static_cast<unsigned long long>(e.t));
        liz_line(kLogPosture, LOG_LEVEL_INFO, line);
        break;
      case liz::EvK::Critical:
      case liz::EvK::Coverage:
        std::snprintf(line, sizeof(line), "LIZ_CRITICAL event=%s actor=%u code=%d a=%d b=%d f=%.1f g=%.1f tick=%llu", e.k == liz::EvK::Coverage ? "AT_SAMPLE_GAP" : "STATE", s.id, e.a, e.b, e.c, e.f, e.g, static_cast<unsigned long long>(e.t));
        liz_line(kLogCritical, LOG_LEVEL_WARN, line);
        break;
      default: break;
    }
  }
  b.nev = 0;
}
// ---------------------------------------------------------------- hooks
HookAction on_liz_exec_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  if (!a) return HOOK_CONTINUE;
  fopAc_ac_c* ac = static_cast<fopAc_ac_c*>(&a->actor);
  LizSlot* s = liz_find(a);
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(ac));
  if (s && s->id != id) {  // address reused by a new process: never inherits anything
    liz_retire(*s);
    s = nullptr;
  }
  if (!s) {
    for (auto& x : lz.slots)
      if (!x.on) {
        s = &x;
        break;
      }
    if (!s) {
      ++lz.overflow;
      return HOOK_CONTINUE;  // vanilla actor, counted
    }
    s->on = true, s->key = reinterpret_cast<std::uintptr_t>(a), s->id = id;
    s->b.bind(id, ++lz.generations, g.p8.tick ^ (static_cast<std::uint64_t>(lz.generations) << 20));
    liz::IdIn in;
    in.profile_ok = ac->profname == fpcNm_E_DN_e && ac->name == fpcNm_E_DN_e;
    in.skull_model = a->skull_model != nullptr, in.arg0 = a->arg0, in.arg1 = a->arg1, in.status = a->status, in.action = a->action;
    in.special_room = liz_stage_is(dComIfGp_getStartStageName(), "D_MN07") && fopAcM_GetRoomNo(ac) == 8;
    s->b.admit(in);
    liz_drain(*s, a);
  }
  if (!s->b.admitted) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  liz::View v = liz_view(a, k);
  liz_contact_reads(a, k, v);
  liz::LinkFrame L = liz_link_frame(k);
  if (v.at_hit && v.at_on_link && k) {  // the shield provenance only when there is a contact: permits() is a counted gate
    L.pass = g.link_track.pass_for(g.p8.tick, true, static_cast<std::uint32_t>(fopAcM_GetID(k)));
    if (L.pass.guard_attack && !cp04_guard_pass()) L.pass.guard_attack = false;
  }
  const std::uint64_t parries = s->b.st.link_parry;
  const std::uint64_t def_contact = s->b.st.contact[unsigned(liz::Hit::Guard)] + s->b.st.contact[unsigned(liz::Hit::Direct)] + s->b.st.contact[unsigned(liz::Hit::MissedEarly)];
  liz::Write w = s->b.on_exec_pre(v, L);
  if (s->b.st.link_parry != parries) {  // the same shield chain contract CP04 uses for every enemy
    (void)c4.shield.confirm(g.p8.tick);
    lz.d1_parry_id = s->id;  // the fast re-arm below only ever serves the D pair of the actor Link just parried
    g.link_track.note(L.pass.serial, false, false, true);
    g.p8.watch_life = true;
    g.p8.life_at = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "LIZ_PARRY event=LINK_PARRY actor=%u generation=%llu pattern=%d impact=%d gain=+%d posture=%d latched=%d link_serial=%llu delta=%.2f tick=%llu", s->id,
                  static_cast<unsigned long long>(s->b.generation), s->b.ep.pattern + 1, s->b.ep.impact, liz::policy::kGainLinkParry, s->b.p.points(), s->b.p.latched ? 1 : 0,
                  static_cast<unsigned long long>(L.pass.serial), L.pass.frame - L.pass.start, static_cast<unsigned long long>(g.p8.tick));
    liz_line(kLogParry, LOG_LEVEL_INFO, line);
  } else if (s->b.st.contact[unsigned(liz::Hit::Guard)] + s->b.st.contact[unsigned(liz::Hit::Direct)] + s->b.st.contact[unsigned(liz::Hit::MissedEarly)] != def_contact && c4.shield.pass_valid) {
    c4.shield.cancel();
  }
  liz_apply(a, w);
  liz_drain(*s, a);
  return HOOK_CONTINUE;
}
void on_liz_exec_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  daAlink_c* k = f1_link();
  liz::View v = liz_view(a, k);
  liz_contact_reads(a, k, v);
  const liz::Write w = s->b.on_exec_post(v, liz_link_frame(k));
  liz_apply(a, w);
  if (s->b.double_gap() && lz.d1_parry_id == s->id) lz.double_gap_tick = g.p8.tick;
  liz_drain(*s, a);
}
HookAction on_liz_delete_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (s) {
    s->b.on_delete();
    if (s->b.admitted) liz_drain(*s, a);
    liz_retire(*s);
    ++lz.deleted;
  }
  return HOOK_CONTINUE;
}
HookAction on_liz_damage_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  bool shield = false;
  if (a->invulnerability_timer == 0) {
    for (int i = 0; i < 3 && !shield; ++i) {
      if (!a->cc_sph[i].cCcD_ObjHitInf::ChkTgHit()) continue;
      const cCcD_Obj* o = a->cc_sph[i].cCcD_ObjHitInf::GetTgHitObj();
      shield = o && o->ChkAtType(AT_TYPE_SHIELD_ATTACK);
    }
  }
  const liz::Write w = s->b.on_damage_pre(liz_view(a, f1_link()), shield);
  liz_apply(a, w);
  liz_drain(*s, a);
  return HOOK_CONTINUE;
}
void on_liz_damage_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  const liz::Write w = s->b.on_damage_post(liz_view(a, f1_link()));
  liz_apply(a, w);
  liz_drain(*s, a);
}
liz::HitKind liz_hit_kind(e_dn_class* a, daAlink_c* k) noexcept {
  const cCcD_Obj* o = a->at_info.mpCollider;
  if (!o || !k || a->at_info.mpActor != static_cast<fopAc_ac_c*>(k)) return o ? liz::HitKind::Other : liz::HitKind::None;
  if (o->ChkAtType(kMasks.iron_ball)) return liz::HitKind::IronBall;
  if (o->ChkAtType(kMasks.sword)) return liz::HitKind::Sword;
  return liz::HitKind::Other;
}
HookAction on_liz_small_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  const bool skip = s->b.on_small_pre(liz_view(a, k), liz_hit_kind(a, k));
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction on_liz_big_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  const bool skip = s->b.on_big_pre(liz_view(a, k), liz_hit_kind(a, k));
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction on_liz_run_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  liz::Write w;
  const bool skip = s->b.on_run_pre(liz_view(a, f1_link()), w);
  liz_apply(a, w);
  liz_drain(*s, a);
  return skip ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
void on_liz_run_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted || s->b.ep.active) return;
  const liz::Write w = s->b.on_run_post(liz_view(a, f1_link()), lz.swing_jump);
  liz_apply(a, w);
  liz_drain(*s, a);
}
HookAction on_liz_attack_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  liz_apply(a, s->b.on_attack_pre(liz_view(a, f1_link())));
  return HOOK_CONTINUE;
}
void on_liz_attack_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  liz_apply(a, s->b.on_attack_post(liz_view(a, f1_link())));
  liz_drain(*s, a);
}
void on_liz_at_hit_post(ModContext*, void* args, void* ret, void*) noexcept {
  if (!liz_alive() || !args || !ret) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  auto** r = static_cast<fopAc_ac_c**>(ret);
  const bool is_link = *r && fopAcM_GetName(*r) == fpcNm_ALINK_e;
  if (s->b.on_at_hit_post(liz_view(a, f1_link()), is_link)) *r = nullptr;
}
HookAction on_liz_anm_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return HOOK_CONTINUE;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return HOOK_CONTINUE;
  float& rate = mods::arg_ref<float>(args, 4);
  (void)s->b.on_anm_pre(mods::arg<int>(args, 1), rate);
  return HOOK_CONTINUE;
}
void on_liz_anm_post(ModContext*, void* args, void*, void*) noexcept {
  if (!liz_alive() || !args) return;
  auto* a = mods::arg<e_dn_class*>(args, 0);
  LizSlot* s = a ? liz_find(a) : nullptr;
  if (!s || !s->b.admitted) return;
  float frame = 0;
  if (s->b.on_anm_post(mods::arg<int>(args, 1), frame) && a->anm_p) liz_native_anim(a->anm_p, true, frame, false, 0.0f);
  liz_drain(*s, a);
}
void on_liz_cut_post(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return;
  auto* k = mods::arg<daAlink_c*>(args, 0);
  const int t = mods::arg<u8>(args, 1);
  if (!k || k != f1_link() || k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)) return;
  if (t == daPy_py_c::CUT_TYPE_NONE || t == daPy_py_c::CUT_TYPE_GUARD_ATTACK) return;  // the Shield Attack is not a sword swing
  if (lz.swing_tick == g.p8.tick) return;                                             // one distinct swing per tick
  lz.swing_tick = g.p8.tick;
  ++lz.swing_serial;
  lz.swing_jump = t == daPy_py_c::CUT_TYPE_JUMP;
}
// Link's native damage receipt for THIS module's direct hits + the only relaxation of the shield re-arm: inside an open D1->D2 pair.
void on_link_damage_post_liz(ModContext*, void* args, void*, void*) noexcept {
  if (!lz.enabled || !args) return;
  auto* link = mods::arg<daAlink_c*>(args, 0);
  if (fx.native.valid && fx.native.link_key == reinterpret_cast<std::uintptr_t>(link) && !fx.native_reentered && link == dComIfGp_getLinkPlayer()) {
    auto n = fx.native;
    n.timer_after = link->mDamageTimer;
    n.life_after = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife();
    n.event = n.event || g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
    n.pending_after = g_dComIfG_gameInfo.play.getItemLifeCount();
    n.electric = link->checkResetFlg1(daPy_py_c::RFLG1_UNK_2) != 0;
    lz.direct.capture(n);
  }
  if (!c4.enabled || lz.double_gap_tick == ~0ull || g.p8.tick > lz.double_gap_tick + 1) return;
  const bool safe = cp04_link_safe(link) && link->mEquipItem == 0x103;
  if (!safe) return;
  const bool slip = link->mProcID == daAlink_c::PROC_GUARD_SLIP && link->mProcVar1.field_0x300a == 0 && g.p8.tick >= c4.shield.permit_tick + liz::policy::kDoubleRearmTicks;
  if (slip && c4.shield.can_restart(g.p8.tick, safe, true)) {
    if (cp04_native_shield_restart(link)) {
      ++c4.shield.c.rearms;
      ++lz.double_rearms;
      cp04_log("SHIELD_CHAIN_ARMED", nullptr, "LIZ_DOUBLE_GAP_NATIVE_RESTART_READBACK");
    } else {
      ++c4.shield.c.failed_native;
      c4.shield.cancel();
    }
  }
}
// ---------------------------------------------------------------- once per mod_update
void liz_maintenance() noexcept {
  if (!lz.enabled) return;
  const std::uint32_t st = stage_token(dComIfGp_getStartStageName());
  if (st != lz.stage) {  // stage change: no pointer is ever dereferenced again, no state survives
    lz.stage = st;
    for (auto& s : lz.slots) liz_retire(s);
    lz.break_retry_until = 0;
  }
  daAlink_c* k = f1_link();
  const std::uint32_t link_id = k ? static_cast<std::uint32_t>(fopAcM_GetID(k)) : 0;
  for (unsigned i = 0; i < liz::kSlots; ++i) {
    const auto r = lz.direct.take(i, g.p8.tick, k != nullptr, link_id, lg.sim_active);
    if (r.present && r.matched) {
      const int pts = r.k.atp == 2 ? liz::policy::kLinkDirectAtp2 : liz::policy::kLinkDirectAtp1;
      LizSlot& s = lz.slots[i];
      if (s.on && s.id == r.k.id) liz_link_gain(s, pts, lnk05::Kind::Direct, r.k.token);
    }
  }
  if ((g.p8.tick % 9000u) == 0 && lz.admitted) liz_summary();  // every ~5 min of simulation, never per frame
  if (lz.break_retry_until && g.p8.tick <= lz.break_retry_until && (g.p8.tick & 7u) == 0 && lg.posture.milli() >= lnk01::policy::kMaxMilli && lb.m.state() == lnk05::State::Ready) {
    ++lz.break_retries;
    lnk05_request(lnk05::Kind::Direct, lz.retry_attacker, lz.retry_generation, 1, lnk01::policy::kMaxMilli, lnk01::policy::kMaxMilli);
  }
}
// HUD: same 0..100 bar of the other enemies, top hud slots (a Bokoblin ledger slot always wins; then no bar and a counter). Display only. The two lists are
// walked by value; the Delete hook and the stage reset guarantee a slot never outlives its actor.
void liz_hud_samples() noexcept {
  if (!lz.enabled || g.eng.obs.disabled) return;
  auto* player = g_dComIfG_gameInfo.play.getPlayer(0);
  if (!player) return;
  for (unsigned i = 0; i < liz::kSlots; ++i) {
    LizSlot& s = lz.slots[i];
    if (!s.on || !s.b.admitted) continue;
    auto* act = reinterpret_cast<fopAc_ac_c*>(s.key);
    const unsigned h = post07::kSlots - 1u - i;
    if (hg.samples[h].in.present) {
      ++lz.hud_slot_busy;
      continue;
    }
    ehz::DrawEntry life;
    if (!hg.mirror.tracker.presented({s.key, s.id, static_cast<std::int16_t>(fpcNm_E_DN_e)}, life)) {
      ++lz.hud_missing;
      continue;
    }
    post07::Sample& smp = hg.samples[h];
    smp.in.present = true, smp.in.id = s.id, smp.in.generation = (1ull << 40) + s.b.generation;
    smp.in.posture = s.b.p.points(), smp.in.latched = s.b.p.latched, smp.in.dead = s.b.dead || act->health <= 0, smp.in.frozen = false;
    smp.in.phase = smp.in.dead ? post07::Ph::Dead : s.b.in_break ? post07::Ph::Down : post07::Ph::Tracking;
    smp.in.opacity = life.alpha;
    smp.anchor = {life.anchor.x, life.anchor.y, life.anchor.z};
    smp.distance = fopAcM_searchActorDistance(player, act);
    hg.slot_interp[h] = life.key;
    ++hg.aligned_samples;
    ++lz.hud_shown;
  }
}
void liz_summary() noexcept {
  liz::Stats t = lz.totals;
  std::uint64_t rolls[3] = {lz.rolls[0], lz.rolls[1], lz.rolls[2]}, atts[3] = {lz.attempts[0], lz.attempts[1], lz.attempts[2]};
  for (auto& s : lz.slots)
    if (s.on && s.b.admitted) liz_merge(t, rolls, atts, s.b);
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "LIZ_SUMMARY version=%s session=1 admitted=%llu excluded=%llu deleted=%llu overflow=%llu selected=%llu/%llu/%llu/%llu completed=%llu/%llu/%llu/%llu windows=%llu parry=%llu dpair=%llu direct=%llu guard=%llu sword=%llu iron=%llu "
                "blocks=%llu enemy_parry=%llu cancels_suppressed=%llu stagger_small=%llu stagger_big=%llu bash=%llu rolls=%llu/%llu/%llu attempts=%llu/%llu/%llu guards=%llu/%llu/%llu end_posture=%llu end_native=%llu end_adaptive=%llu watchdog=%llu sample_gap=%llu "
                "link_gain=%llu link_sat=%llu break_retries=%llu double_rearms=%llu hud=%llu/%llu/%llu",
                liz::kVersion, static_cast<unsigned long long>(lz.admitted), static_cast<unsigned long long>(lz.excluded), static_cast<unsigned long long>(lz.deleted), static_cast<unsigned long long>(lz.overflow),
                static_cast<unsigned long long>(t.selected[0]), static_cast<unsigned long long>(t.selected[1]), static_cast<unsigned long long>(t.selected[2]), static_cast<unsigned long long>(t.selected[3]),
                static_cast<unsigned long long>(t.completed[0]), static_cast<unsigned long long>(t.completed[1]), static_cast<unsigned long long>(t.completed[2]), static_cast<unsigned long long>(t.completed[3]),
                static_cast<unsigned long long>(t.windows), static_cast<unsigned long long>(t.link_parry), static_cast<unsigned long long>(t.d_pair_parry), static_cast<unsigned long long>(t.contact[unsigned(liz::Hit::Direct)]),
                static_cast<unsigned long long>(t.contact[unsigned(liz::Hit::Guard)]), static_cast<unsigned long long>(t.sword_hits), static_cast<unsigned long long>(t.iron_hits), static_cast<unsigned long long>(t.blocks),
                static_cast<unsigned long long>(t.enemy_parries), static_cast<unsigned long long>(t.cancels_suppressed), static_cast<unsigned long long>(t.stagger_suppressed_small), static_cast<unsigned long long>(t.stagger_suppressed_big),
                static_cast<unsigned long long>(t.shield_bash_cleared), static_cast<unsigned long long>(rolls[0]), static_cast<unsigned long long>(rolls[1]), static_cast<unsigned long long>(rolls[2]), static_cast<unsigned long long>(atts[0]),
                static_cast<unsigned long long>(atts[1]), static_cast<unsigned long long>(atts[2]), static_cast<unsigned long long>(t.guards_native), static_cast<unsigned long long>(t.guards_attempt), static_cast<unsigned long long>(t.guards_boundary),
                static_cast<unsigned long long>(t.ended[unsigned(liz::End::Posture100)]), static_cast<unsigned long long>(t.ended[unsigned(liz::End::NativePriority)]), static_cast<unsigned long long>(t.ended[unsigned(liz::End::AdaptiveDefense)]),
                static_cast<unsigned long long>(t.watchdog), static_cast<unsigned long long>(t.sample_gap), static_cast<unsigned long long>(lz.link_gain), static_cast<unsigned long long>(lz.link_saturations),
                static_cast<unsigned long long>(lz.break_retries), static_cast<unsigned long long>(lz.double_rearms), static_cast<unsigned long long>(lz.hud_shown), static_cast<unsigned long long>(lz.hud_missing),
                static_cast<unsigned long long>(lz.hud_slot_busy));
  lnk_write(LOG_LEVEL_INFO, line);
}
void liz_shutdown() noexcept {  // callbacks inert from here on; nothing of this module persists in the game
  liz_summary();
  lz.enabled = false;
}
void liz_install() noexcept {
  HookOptions late = HOOK_OPTIONS_INIT;
  late.priority = -400;  // after every existing Link damage observer (FIX01 receipt, CP04 re-arm)
  ModResult r[] = {
      mods::hook::add_pre<LizExec>(on_liz_exec_pre),     mods::hook::add_post<LizExec>(on_liz_exec_post),   mods::hook::add_pre<LizDelete>(on_liz_delete_pre),
      mods::hook::add_pre<LizDamage>(on_liz_damage_pre), mods::hook::add_post<LizDamage>(on_liz_damage_post), mods::hook::add_pre<LizSmall>(on_liz_small_pre),
      mods::hook::add_pre<LizBig>(on_liz_big_pre),       mods::hook::add_pre<LizRun>(on_liz_run_pre),       mods::hook::add_post<LizRun>(on_liz_run_post),
      mods::hook::add_pre<LizAttack0>(on_liz_attack_pre), mods::hook::add_post<LizAttack0>(on_liz_attack_post), mods::hook::add_pre<LizAttack>(on_liz_attack_pre),
      mods::hook::add_post<LizAttack>(on_liz_attack_post), mods::hook::add_pre<LizTail>(on_liz_attack_pre),   mods::hook::add_post<LizTail>(on_liz_attack_post),
      mods::hook::add_post<LizAtHit>(on_liz_at_hit_post), mods::hook::add_pre<LizAnm>(on_liz_anm_pre),       mods::hook::add_post<LizAnm>(on_liz_anm_post),
      mods::hook::add_post<LinkCutType>(on_liz_cut_post), mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_liz, &late)};
  bool ok = c4.enabled;
  unsigned failed = 0;
  for (auto x : r)
    if (x != MOD_OK) {
      ok = false;
      ++failed;
    }
  lz.enabled = ok;  // all or nothing: without every hook the E_DN is vanilla and the rest of the mod is untouched (a callback with lz.enabled=false returns at once)
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LIZ_POSTURE event=INSTALL version=%s enabled=%d hooks=%u failed=%u hp=%d capacity=%d gains=15/22/30/5/25 regen=10/7/4/2/1 delay=%u combos=3-4,6-7,6-9,5-6 pct=12/35/45 cooldown=%u window=%u..%u",
                liz::kVersion, ok ? 1 : 0, static_cast<unsigned>(sizeof(r) / sizeof(r[0])), failed, liz::policy::kMaxHp, liz::policy::kMaxPosture, liz::policy::kRegenDelayTicks, liz::policy::kParryCooldownTicks,
                liz::policy::kParryWindowFirst, liz::policy::kParryWindowLast);
  lnk_write(ok ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}
#undef LIZ_NOINLINE
