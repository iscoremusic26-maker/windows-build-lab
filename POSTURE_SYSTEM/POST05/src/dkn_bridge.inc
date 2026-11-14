// LIGHT FADES TWICE 0.13.9 DKN01: the ONLY place the Darknut miniboss module touches the systems it shares with the rest of the mod (simulation tick,
// Link, Link's posture and LNK05 break request, the CP04 Shield Attack chain and its parry window, the direct-hit join, the HUD samples, the log, and a
// READ-ONLY view of the DKR01 resurrection table). Included once in mod.cpp's anonymous namespace after dkr_adapter.inc, before dkn_adapter.inc.
// It writes no actor field. The same contracts the Chilfos module uses (kk_bridge.inc), with the Darknut's own identity and HUD slots.
struct DknPass { bool valid = false, guard_attack = false, in_window = false; std::uint64_t serial = 0; float delta = 0; };
struct DknGain { bool confirmed = false, saturated = false; int before = 0, after = 0; };
std::uint64_t dknb_tick() noexcept { return g.p8.tick; }
daAlink_c* dknb_link() noexcept { return f1_link(); }
bool dknb_world_safe() noexcept { return cp04_world_safe(); }
bool dknb_link_alive() noexcept { return g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() > 0; }
void dknb_write(bool warn, const char* line) noexcept { lnk_write(warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line); }
// Link's REAL guard: the native virtual daPy_py_c::checkPlayerGuard (upper guard animation: held shield or guard slip), never the R button alone.
bool dknb_link_guarding(daAlink_c* k) noexcept { return k && static_cast<daPy_py_c*>(k)->checkPlayerGuard() != 0; }
int dknb_link_proc(daAlink_c* k) noexcept { return k ? static_cast<int>(k->mProcID) : -1; }
// The Shield Attack pass of THIS tick (LinkTrack + the CP04 counted gate + the shared [-1,+7] window): the provenance every enemy's parry uses.
DknPass dknb_link_pass(daAlink_c* k) noexcept {
  DknPass p;
  if (!k) return p;
  post08::LinkView v = g.link_track.pass_for(g.p8.tick, true, static_cast<std::uint32_t>(fopAcM_GetID(k)));
  if (v.guard_attack && !cp04_guard_pass()) v.guard_attack = false;
  p.valid = v.valid, p.guard_attack = v.guard_attack, p.in_window = post08::Parry::in_window(v), p.serial = v.serial, p.delta = v.frame - v.start;
  return p;
}
// A confirmed Link parry of a Darknut impact: the shield chain contract, and the fast re-arm of a pair (the two cuts of one B, two thrusts in 0.21 s).
void dknb_link_parry_confirmed(std::uint32_t id, std::uint64_t pass_serial) noexcept {
  (void)c4.shield.confirm(g.p8.tick);
  lz.d1_parry_id = id;
  g.link_track.note(pass_serial, false, false, true);
}
void dknb_link_blocked(std::uint32_t id, bool burst_follows) noexcept {
  if (c4.shield.pass_valid) c4.shield.cancel();
  if (!burst_follows) return;
  c4.shield.armed = true, c4.shield.buffer = false, c4.shield.permit_serial = c4.shield.native_serial, c4.shield.permit_tick = g.p8.tick;  // a NEW real press may restart it
  lz.d1_parry_id = id;
}
void dknb_burst_gap(std::uint32_t id) noexcept {
  if (lz.d1_parry_id == id) lz.double_gap_tick = g.p8.tick;
}
// Link's own posture (max 100, unchanged): the shared LNK scale and LNK05's native guard-break request on saturation.
DknGain dknb_link_gain(int points, bool block, std::uint32_t id, std::uint64_t generation, std::uint64_t token) noexcept {
  DknGain r;
  const lnk01::Outcome o = lg.posture.on_external(points);
  if (o.v != lnk01::Verdict::Confirmed) return r;
  r.confirmed = true, r.saturated = o.saturated, r.before = o.before, r.after = o.after;
  if (o.after >= lnk01::policy::kMaxMilli) lnk05_request(block ? lnk05::Kind::Block : lnk05::Kind::Direct, id, generation, token, o.before, o.after);
  return r;
}
bool dknb_link_broken() noexcept {  // Link already guard-broken / saturated: no further impacts are generated artificially by this module
  daAlink_c* k = f1_link();
  return k && static_cast<int>(k->mProcID) == daAlink_c::PROC_GUARD_BREAK;
}
// Direct impacts on Link are credited only with Link's own native damage receipt (the FIX01 join the Lizalfos and the Chilfos use).
lnks1fix::DirectJoin dkn_direct;
void dknb_direct_offer(unsigned slot, std::uint32_t id, std::uint64_t generation, std::uint64_t token, const void* sphere) noexcept {
  lnk01::Contact k;
  k.result = post08::Result::NotBlocked, k.token = token ? token : 1, k.slot = static_cast<int>(slot), k.id = id, k.generation = generation, k.atp = 1;
  k.link_ok = true, k.link_human = true, k.link_id = lg.posture.link_id(), k.tick = g.p8.tick;
  dkn_direct.offer(k, reinterpret_cast<std::uintptr_t>(sphere), 0, true);
}
bool dknb_direct_take(unsigned slot, std::uint32_t& id, std::uint64_t& token) noexcept {
  daAlink_c* k = f1_link();
  const auto r = dkn_direct.take(slot, g.p8.tick, k != nullptr, k ? static_cast<std::uint32_t>(fopAcM_GetID(k)) : 0u, lg.sim_active);
  if (!r.present || !r.matched) return false;
  id = r.k.id, token = r.k.token;
  return true;
}
void dknb_direct_capture(daAlink_c* link) noexcept {
  if (fx.native.valid && fx.native.link_key == reinterpret_cast<std::uintptr_t>(link) && !fx.native_reentered && link == dComIfGp_getLinkPlayer()) {
    auto n = fx.native;
    n.timer_after = link->mDamageTimer;
    n.life_after = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife();
    n.event = n.event || g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
    n.pending_after = g_dComIfG_gameInfo.play.getItemLifeCount();
    n.electric = link->checkResetFlg1(daPy_py_c::RFLG1_UNK_2) != 0;
    dkn_direct.capture(n);
  }
}
// HUD: the same bar geometry, in four slots of its own just below the Lizalfos (16) and Chilfos (8) ones. The Enemy Health Bars tracker never presents
// a B_TN (profile 531 is the "Darknut" miniboss of the boss catalogue), so the anchor is computed here from the live actor (above its head) instead of
// borrowed: the bar of every other enemy is untouched. The posture is shown as a fraction of THIS Darknut's maximum (120 / 130) on the 0..100 bar.
inline constexpr unsigned kDknHudSlots = 4;
bool dknb_hud(unsigned slot, std::uint32_t id, std::uint64_t generation, int posture_pct, bool latched, bool dead, bool punish, daB_TN_c* act, bool& busy) noexcept {
  busy = false;
  if (g.eng.obs.disabled || !act) return false;
  auto* player = g_dComIfG_gameInfo.play.getPlayer(0);
  if (!player || slot >= kDknHudSlots || liz::kSlots + kk::kSlots + kDknHudSlots >= post07::kSlots) return false;
  const unsigned h = post07::kSlots - 1u - liz::kSlots - kk::kSlots - slot;
  if (hg.samples[h].in.present) {
    busy = true;
    return false;
  }
  const float d = fopAcM_searchActorDistance(player, act);
  if (!(d < 3000.0f)) return false;
  post07::Sample& smp = hg.samples[h];
  smp.in.present = true, smp.in.id = id, smp.in.generation = (3ull << 40) + generation;
  smp.in.posture = posture_pct < 0 ? 0 : posture_pct > 100 ? 100 : posture_pct, smp.in.latched = latched, smp.in.dead = dead, smp.in.frozen = false;
  smp.in.phase = dead ? post07::Ph::Dead : punish ? post07::Ph::Punish : post07::Ph::Tracking;
  smp.in.opacity = d < 2500.0f ? 1.0f : (3000.0f - d) / 500.0f;
  smp.anchor = {act->current.pos.x, act->eyePos.y + 85.0f, act->current.pos.z};
  smp.distance = d;
  hg.slot_interp[h] = 0;
  ++hg.aligned_samples;
  return true;
}
// READ-ONLY view of the DKR01 table: the resurrection owns the actor from the lethal transition to the end of the FIX02 clip.
dkr::Phase dknb_dkr_phase(std::uint32_t id) noexcept {
  const dkr::Ctl* c = dz.t.find(id);
  return c ? c->phase : dkr::Phase::Armed;
}
bool dknb_dkr_enabled() noexcept { return dz.enabled; }
