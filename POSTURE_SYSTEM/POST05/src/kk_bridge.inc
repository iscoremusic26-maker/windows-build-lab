// LIGHT FADES TWICE 0.13.0 CHILFOS01 (+ FIX03 clock, + FIX04 guard spark): the ONLY place the Chilfos module touches the systems it shares with FIX05 (tick, Link, Link's posture and
// break request, the CP04 shield chain, the parry FX book, the HUD samples, the log budget). Included in mod.cpp after the parry FX adapter.
// tests/kk_world_lab.cpp provides its own definitions of exactly these functions; src/kk_adapter.inc never reaches past them.
struct KkPass { bool valid = false, guard_attack = false, in_window = false; std::uint64_t serial = 0; float delta = 0; };
struct KkGain { bool confirmed = false, saturated = false; int before = 0, after = 0; };
std::uint64_t kkb_tick() noexcept { return g.p8.tick; }
// FIX03: wall clock for the throw's frame-pacing window. Both functions are already KERNEL32 imports of this DLL (the CRT start-up uses them), so no import
// is added; C linkage makes these the global Win32 entry points even inside this namespace. QueryPerformanceFrequency is not imported: the window
// calibrates QPC against the file-time clock instead.
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(long long* count);
extern "C" __declspec(dllimport) void __stdcall GetSystemTimeAsFileTime(void* file_time);
std::int64_t kkb_qpc() noexcept {
  long long v = 0;
  QueryPerformanceCounter(&v);
  return v;
}
std::int64_t kkb_filetime() noexcept {
  unsigned long long v = 0;  // FILETIME: two DWORDs, low first: the same bytes as one little-endian 64-bit count of 100 ns
  GetSystemTimeAsFileTime(&v);
  return static_cast<std::int64_t>(v);
}
daAlink_c* kkb_link() noexcept { return f1_link(); }
bool kkb_world_safe() noexcept { return cp04_world_safe(); }
bool kkb_link_alive() noexcept { return g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() > 0; }
std::uint32_t kkb_stage() noexcept { return stage_token(dComIfGp_getStartStageName()); }
void kkb_write(bool warn, const char* line) noexcept { lnk_write(warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line); }
bool kkb_take(unsigned& lines, std::uint64_t& dropped, unsigned cap) noexcept { return lnk_take(lines, dropped, cap); }
std::uint32_t kkb_sword_mask() noexcept { return kMasks.sword; }
std::uint32_t kkb_iron_mask() noexcept { return kMasks.iron_ball; }
// Link's Shield Attack pass for a contact of THIS tick: the same provenance the Lizalfos uses (LinkTrack + CP04 counted gate + the shared [-1,+7] window).
KkPass kkb_link_pass(daAlink_c* k) noexcept {
  KkPass p;
  if (!k) return p;
  post08::LinkView v = g.link_track.pass_for(g.p8.tick, true, static_cast<std::uint32_t>(fopAcM_GetID(k)));
  if (v.guard_attack && !cp04_guard_pass()) v.guard_attack = false;
  p.valid = v.valid, p.guard_attack = v.guard_attack, p.in_window = post08::Parry::in_window(v), p.serial = v.serial, p.delta = v.frame - v.start;
  return p;
}
// A Link parry of a Chilfos impact: the shield chain contract every enemy uses, and the fast re-arm of the Lizalfos D1->D2 pair for a burst of this actor.
void kkb_link_parry_confirmed(std::uint32_t id, std::uint64_t pass_serial) noexcept {
  (void)c4.shield.confirm(g.p8.tick);
  lz.d1_parry_id = id;
  g.link_track.note(pass_serial, false, false, true);
}
void kkb_link_blocked(std::uint32_t id, bool burst_follows) noexcept {
  if (c4.shield.pass_valid) c4.shield.cancel();
  if (!burst_follows) return;
  c4.shield.armed = true, c4.shield.buffer = false, c4.shield.permit_serial = c4.shield.native_serial, c4.shield.permit_tick = g.p8.tick;  // a NEW real press may restart it
  lz.d1_parry_id = id;
}
void kkb_burst_gap(std::uint32_t id) noexcept {
  if (lz.d1_parry_id == id) lz.double_gap_tick = g.p8.tick;
}
// Link's own posture: the shared LNK scale (kind Block / Direct), and LNK05's break request on saturation.
KkGain kkb_link_gain(int points, bool block, std::uint32_t id, std::uint64_t generation, std::uint64_t token) noexcept {
  KkGain r;
  const lnk01::Outcome o = lg.posture.on_external(points);
  if (o.v != lnk01::Verdict::Confirmed) return r;
  r.confirmed = true, r.saturated = o.saturated, r.before = o.before, r.after = o.after;
  if (o.after >= lnk01::policy::kMaxMilli) lnk05_request(block ? lnk05::Kind::Block : lnk05::Kind::Direct, id, generation, token, o.before, o.after);
  return r;
}
// Parry FX: the native orange of the clash and the x1.22 clash SE, owned by parry_fx_adapter.inc (two kinds added for E_KK).
void kkb_pfx_begin(daE_KK_c* a, std::uint32_t id) noexcept { pfx_kk_exec_begin(a, id); }
void kkb_pfx_end() noexcept { pfx_liz_exec_end(); }
void kkb_pfx_settle(bool link_on_kk, std::uint32_t id, bool parry) noexcept { pfx_settle(link_on_kk ? pfx::Kind::LinkOnChilfos : pfx::Kind::ChilfosOnLink, id, parry); }
// FIX04 P2. A contact the Chilfos blocks or parries with its spear in hand: the native damage_check plays its guard spark (setHitMark(2, this, pos + 100,
// shape_angle, NULL, 0)) only in the WEAK branch; its strong branch (at_power_check mHitStatus != 0, Link's status 0x400 or a Midna lock) is the spear-break
// CLASH that FIX02 turns into the guard reaction, and it has no spark at all (the log: all 16 BLOCK_INTACT contacts, the 4 parries without a captured
// emitter). This is the weak branch's own call, for that one contact; inside this Chilfos' Execute, so the parry FX book claims it like the native one.
void kkb_guard_mark(daE_KK_c* a) noexcept {
  cXyz p = a->current.pos;
  p.y += 100.0f;
  g_dComIfG_gameInfo.play.getParticle()->setHitMark(2, a, &p, &a->shape_angle, nullptr, 0);
}
// The native strong-branch test of that contact, read before damage_check runs (at_power_check: Link's sword AT spl 1; then status 0x400; then MIDNA_LOCK).
bool kkb_strong_hit(daAlink_c* k, const cCcD_Obj* o) noexcept {
  if (!o) return false;
  if (g_dComIfG_gameInfo.play.checkPlayerStatus(0, 0, 0x400)) return true;
  if (const_cast<cCcD_Obj*>(o)->ChkAtType(AT_TYPE_MIDNA_LOCK)) return true;
  if (!k) return false;
  for (auto& c : k->mAtCps)
    if (o == static_cast<const cCcD_Obj*>(&c)) return c.GetAtSpl() == 1;
  return o == static_cast<const cCcD_Obj*>(&k->mAtCyl) && k->mAtCyl.GetAtSpl() == 1;
}
// The strong branch plays its clash through def_se_set(map 0x1F) instead of the guard branch's startCollisionSE(map 7): for an enemy parry the clash that
// gets the parry pitch is that one. Only while this Chilfos' Execute owns the FX context; put back to the native 7 in the damage_check POST.
void kkb_pfx_clash_map(std::uint32_t m) noexcept {
  if (pf.exec_kind == pfx::Kind::ChilfosOnLink && pf.exec_id) pf.exec_map = m;
}
// Direct hits on Link are credited only with Link's own native damage receipt (the FIX01 join the Lizalfos uses).
lnks1fix::DirectJoin kk_direct;
void kkb_direct_offer(unsigned slot, std::uint32_t id, std::uint64_t generation, std::uint64_t token, const void* sphere) noexcept {
  lnk01::Contact k;
  k.result = post08::Result::NotBlocked, k.token = token ? token : 1, k.slot = static_cast<int>(slot), k.id = id, k.generation = generation, k.atp = 1;
  k.link_ok = true, k.link_human = true, k.link_id = lg.posture.link_id(), k.tick = g.p8.tick;
  kk_direct.offer(k, reinterpret_cast<std::uintptr_t>(sphere), 0, true);
}
bool kkb_direct_take(unsigned slot, std::uint32_t& id, std::uint64_t& token) noexcept {
  daAlink_c* k = f1_link();
  const auto r = kk_direct.take(slot, g.p8.tick, k != nullptr, k ? static_cast<std::uint32_t>(fopAcM_GetID(k)) : 0u, lg.sim_active);
  if (!r.present || !r.matched) return false;
  id = r.k.id, token = r.k.token;
  return true;
}
void kkb_direct_capture(daAlink_c* link) noexcept {
  if (fx.native.valid && fx.native.link_key == reinterpret_cast<std::uintptr_t>(link) && !fx.native_reentered && link == dComIfGp_getLinkPlayer()) {
    auto n = fx.native;
    n.timer_after = link->mDamageTimer;
    n.life_after = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife();
    n.event = n.event || g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
    n.pending_after = g_dComIfG_gameInfo.play.getItemLifeCount();
    n.electric = link->checkResetFlg1(daPy_py_c::RFLG1_UNK_2) != 0;
    kk_direct.capture(n);
  }
}
// HUD: the same 0..100 bar, in the slots just below the Lizalfos ones. A taken slot or an actor the life-bar tracker does not present: no bar, counted.
bool kkb_hud(unsigned slot, std::uintptr_t key, std::uint32_t id, std::uint64_t generation, int posture, bool latched, bool dead, bool punish, fopAc_ac_c* act, bool& busy) noexcept {
  busy = false;
  if (g.eng.obs.disabled) return false;
  auto* player = g_dComIfG_gameInfo.play.getPlayer(0);
  if (!player || slot >= kk::kSlots || liz::kSlots + kk::kSlots >= post07::kSlots) return false;
  const unsigned h = post07::kSlots - 1u - liz::kSlots - slot;
  if (hg.samples[h].in.present) {
    busy = true;
    return false;
  }
  ehz::DrawEntry life;
  if (!hg.mirror.tracker.presented({key, id, static_cast<std::int16_t>(fpcNm_E_KK_e)}, life)) return false;
  post07::Sample& smp = hg.samples[h];
  smp.in.present = true, smp.in.id = id, smp.in.generation = (2ull << 40) + generation;
  smp.in.posture = posture, smp.in.latched = latched, smp.in.dead = dead, smp.in.frozen = false;
  smp.in.phase = dead ? post07::Ph::Dead : punish ? post07::Ph::Punish : post07::Ph::Tracking;
  smp.in.opacity = life.alpha;
  smp.anchor = {life.anchor.x, life.anchor.y, life.anchor.z};
  smp.distance = fopAcM_searchActorDistance(player, act);
  hg.slot_interp[h] = life.key;
  ++hg.aligned_samples;
  return true;
}
