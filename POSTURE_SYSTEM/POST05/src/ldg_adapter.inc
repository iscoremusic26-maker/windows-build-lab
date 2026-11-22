// LIGHT FADES TWICE 0.13.16 LDG01 (DKN01 FIX07 change 6): production adapter of Link's directional dodge i-frames (src/ldg_core.hpp). Included once in
// mod.cpp's anonymous namespace AFTER dkn_adapter.inc (it asks the Darknut module about its contacts) and after the Lizalfos / Chilfos / E_OC trackers.
//
// Two hook callbacks, one of them a NEW target:
//   daAlink_c::setCollision PRE (an existing LinkCollisionFx target, one more callback): once per EXECUTED Link tick, at the end of his execute, his procedure
//     is final and his Tg cylinders are about to be registered for this tick's collision pass: the tracker sees the real action (proc + its clip frame).
//   dCcS::ChkAtTgHitAfterCross POST (NEW, virtual: reached through cCcS::SetAtTgCommonHitInf's vtable call, B 0x1388b5d `call [rbp+0x28]`; prologue
//     `mov [rsp+8],rbx`, nothing IP-relative): the commit of ONE crossing. Only a crossing of an eligible enemy's AT with one of Link's three Tg cylinders, inside
//     a protected window of a real, eligible dodge (or the continuation of a contact already evaded), gets `true` (= no hit, exactly what the native answers
//     for its own no-continuous-hit rule). Everything else keeps the native answer.
// It writes no actor field: its only effect is the return value of that one call. No life is rewritten, no damage healed, no flag left set, no global
// invulnerability: hazards, environment, events, script damage, grabs of other actors, wolf / horse / swim and every non-supported actor are never touched.
// Eligible attackers = the enemies this mod supports and tracks: the admitted Darknut (DKN01, its thrown mace included), the admitted Lizalfos / Dinolfos (LIZ),
// the admitted Chilfos (CHILFOS01), the tracked Bokoblins (E_OC ledger, every variant). Their weapon / body AT colliders only (any AT of that actor).
struct LdgState {
  bool enabled = false;
  ldg::Tracker tr;
  ldg::HeldSet held;
  std::uint32_t link_id = 0;
  std::uint64_t epoch = 0;
  std::uint64_t calls_obs = 0, calls_cross = 0, crossings_link = 0, eligible_contacts = 0, evaded = 0, held_continued = 0, excluded = 0, outside = 0, not_eligible = 0;
  std::uint64_t evaded_by[unsigned(ldg::Dir::Count)] = {}, by_enemy[4] = {}, scene_resets = 0;
  unsigned lines = 0;
  std::uint64_t dropped = 0;
  std::uint64_t last_logged_serial = 0;
} lq;
void ldg_line(bool warn, const char* line) noexcept {
  if (lq.lines >= 1200) {
    ++lq.dropped;
    return;
  }
  ++lq.lines;
  lnk_write(warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}
// 0 = not eligible, 1 = Darknut, 2 = Lizalfos, 3 = Chilfos, 4 = Bokoblin (E_OC)
int ldg_enemy_kind(const fopAc_ac_c* ac) noexcept {
  if (!ac) return 0;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(const_cast<fopAc_ac_c*>(ac)));
  const s16 nm = fopAcM_GetName(const_cast<fopAc_ac_c*>(ac));
  if (nm == fpcNm_B_TN_e) return dkn_find(id) ? 1 : 0;
  if (nm == fpcNm_E_DN_e) {
    for (const auto& s : lz.slots)
      if (s.on && s.id == id) return 2;
    return 0;
  }
  if (nm == fpcNm_E_KK_e) {
    for (const auto& s : kz.slots)
      if (s.on && s.id == id) return 3;
    return 0;
  }
  if (nm == fpcNm_E_OC_e) {
    for (unsigned i = 0; i < post08::kSlots; ++i) {
      const auto& l = g.eng.obs.ledger.slot(i);
      if (l.id == id && l.life == Life::Active) return 4;
    }
  }
  return 0;
}
// Combat context at the dodge start: Link Z-targets an enemy, or an eligible enemy stands within kCombatReach. Read once per NEW dodge, never per tick.
bool ldg_combat_context(daAlink_c* k) noexcept {
  if (k->mTargetedActor && fopAcM_GetGroup(k->mTargetedActor) == fopAc_ENEMY_e && ldg_enemy_kind(k->mTargetedActor)) return true;
  const auto near = [k](std::uint32_t id) noexcept {
    fopAc_ac_c* p = nullptr;
    return id && fopAcM_SearchByID(id, &p) && p && fopAcM_searchActorDistance(k, p) <= ldg::policy::kCombatReach;
  };
  for (const auto& s : dn.s)
    if (s.on && dkn_combat(s) && near(s.id)) return true;
  for (const auto& s : lz.slots)
    if (s.on && near(s.id)) return true;
  for (const auto& s : kz.slots)
    if (s.on && near(s.id)) return true;
  for (unsigned i = 0; i < post08::kSlots; ++i) {
    const auto& l = g.eng.obs.ledger.slot(i);
    if (l.life == Life::Active && near(l.id)) return true;
  }
  return false;
}
HookAction on_ldg_collision_pre(ModContext*, void* args, void*, void*) noexcept {
  if (!lq.enabled || !args) return HOOK_CONTINUE;
  daAlink_c* k = mods::arg<daAlink_c*>(args, 0);
  if (!k || k != f1_link()) return HOOK_CONTINUE;
  ++lq.calls_obs;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(k));
  const std::uint64_t ep = lg.posture.epoch();
  if (id != lq.link_id || ep != lq.epoch) lq.link_id = id, lq.epoch = ep, lq.tr.reset(), lq.held.clear(), ++lq.scene_resets;  // a new Link / scene: nothing carries over
  const int proc = static_cast<int>(k->mProcID), side = static_cast<int>(k->field_0x2f98);
  const float frame = k->mUnderFrameCtrl[0].getFrame();
  const bool human = !k->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);
  const ldg::Dir dr = human ? ldg::dir_of(proc, side) : ldg::Dir::None;
  // the combat context is read only when this observation can start a NEW action (the core's own rule), never on the ticks of an action already open
  const bool fresh = dr != ldg::Dir::None && (!lq.tr.d.on || lq.tr.d.proc != proc || lq.tr.d.dir != dr || frame + 0.5f < lq.tr.d.last_frame);
  const bool combat = fresh && cp04_world_safe() && ldg_combat_context(k);
  const std::uint64_t before = lq.tr.serial_n;
  const ldg::Dodge prev = lq.tr.d;
  lq.tr.observe(g.p8.tick, proc, side, frame, human, combat);
  if (prev.on && prev.serial != lq.tr.d.serial && lq.lines < 1200) {  // the previous action ended (or was replaced): its window really used
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "LDG01 event=DODGE_END version=%s serial=%llu dir=%s eligible=%d window_ticks=%u ticks_seen=%u next=%s tick=%llu", ldg::kVersion,
                  static_cast<unsigned long long>(prev.serial), ldg::name(prev.dir), prev.eligible ? 1 : 0, ldg::window_of(prev.dir), prev.age + 1,
                  lq.tr.d.on ? "ANOTHER_DODGE" : "OTHER_PROCEDURE", static_cast<unsigned long long>(g.p8.tick));
    ldg_line(false, line);
  }
  if (lq.tr.serial_n != before) {
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "LDG01 event=DODGE_START version=%s serial=%llu dir=%s proc=0x%02x side_dir=%d prev_proc=0x%02x eligible=%d reason=%s window=[%llu,%llu) ticks=%u "
                  "life=%d link_posture=%d.%03d tick=%llu", ldg::kVersion, static_cast<unsigned long long>(lq.tr.d.serial), ldg::name(lq.tr.d.dir), proc, side, prev.on ? prev.proc : -1,
                  lq.tr.d.eligible ? 1 : 0, lq.tr.d.eligible ? "COMBAT_DODGE" : !combat ? "NO_COMBAT_CONTEXT" : "FROM_FALL_OR_JUMP_RECOVERY",
                  static_cast<unsigned long long>(g.p8.tick), static_cast<unsigned long long>(g.p8.tick + ldg::window_of(lq.tr.d.dir)), ldg::window_of(lq.tr.d.dir),
                  static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife()), lg.posture.milli() / 1000, lg.posture.milli() % 1000,
                  static_cast<unsigned long long>(g.p8.tick));
    ldg_line(false, line);
  }
  lq.held.prune(g.p8.tick);
  return HOOK_CONTINUE;
}
void on_ldg_after_cross_post(ModContext*, void* args, void* ret, void*) noexcept {
  ++lq.calls_cross;
  if (!lq.enabled || !args || !ret) return;
  bool* r = static_cast<bool*>(ret);
  if (*r) return;  // the native already answered "no hit"
  const cCcD_GObjInf* at = mods::arg<const cCcD_GObjInf*>(args, 3);
  const cCcD_GObjInf* tg = mods::arg<const cCcD_GObjInf*>(args, 4);
  if (!at || !tg) return;
  daAlink_c* k = f1_link();
  if (!k) return;
  bool link_tg = false;
  for (int i = 0; i < 3 && !link_tg; ++i) link_tg = tg == static_cast<const cCcD_GObjInf*>(&k->mTgCyls[i]);
  if (!link_tg) return;  // every crossing that is not on Link's own Tg cylinders: two pointer comparisons, nothing else
  ++lq.crossings_link;
  const fopAc_ac_c* ac = const_cast<cCcD_GObjInf*>(at)->GetAc();
  const int kind = ldg_enemy_kind(ac);
  if (!kind) return;  // hazards, environment, other actors, unsupported enemies: native
  ++lq.eligible_contacts;
  const std::uint64_t pass = g.p8.tick;
  const std::uint32_t aid = static_cast<std::uint32_t>(fopAcM_GetID(const_cast<fopAc_ac_c*>(ac)));
  const DknLdg dq = kind == 1 ? dkn_ldg_query(ac, at) : DknLdg{};
  const std::uintptr_t key = reinterpret_cast<std::uintptr_t>(at);
  if (lq.held.continues(key, aid, dq.impact, pass)) {  // the same contact already evaded: still no contact (no deferred fictitious damage)
    *r = true, ++lq.held_continued;
    return;
  }
  const ldg::Atk cls = dq.cls == 1 ? ldg::Atk::Thrust : ldg::Atk::Ordinary;
  if (!cp04_world_safe()) return;  // an event / cutscene: never protected
  const ldg::Res v = lq.tr.verdict(pass, cls);
  if (v == ldg::Res::Contact) return;  // no dodge at all: the ordinary contact (nothing logged)
  if (v == ldg::Res::Evaded) {
    *r = true;
    lq.held.add(key, aid, dq.impact, pass, lq.tr.d.serial);
    ++lq.evaded, ++lq.evaded_by[unsigned(lq.tr.d.dir)], ++lq.by_enemy[kind - 1];
    if (kind == 1)
      if (DknSlot* s = dkn_find(aid)) ++s->st_ldg_evaded;
  } else {
    ++(v == ldg::Res::Excluded ? lq.excluded : v == ldg::Res::OutsideWindow ? lq.outside : lq.not_eligible);
  }
  static constexpr const char* kEnemy[] = {"DARKNUT", "LIZALFOS", "CHILFOS", "BOKOBLIN"};
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LDG01 event=CONTACT version=%s result=%s serial=%llu dir=%s age=%u window=%u attacker=%s actor=%u attack_class=%s impact=%llu life=%d "
                "link_posture=%d.%03d hp_damage=%s posture=%s parry=0 enemy_posture=0 tick=%llu", ldg::kVersion, ldg::name(v), static_cast<unsigned long long>(lq.tr.d.serial),
                ldg::name(lq.tr.d.dir), lq.tr.d.age, ldg::window_of(lq.tr.d.dir), kEnemy[kind - 1], aid, ldg::name(cls), static_cast<unsigned long long>(dq.impact),
                static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife()), lg.posture.milli() / 1000, lg.posture.milli() % 1000,
                v == ldg::Res::Evaded ? "NONE_EVADED" : "NATIVE", v == ldg::Res::Evaded ? "NONE_EVADED" : "NATIVE", static_cast<unsigned long long>(pass));
  ldg_line(false, line);
}
void ldg_summary() noexcept {
  using U = unsigned long long;
  const ldg::Tracker& t = lq.tr;
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "LDG01_SUMMARY version=%s enabled=%d window_ticks=fwd%u/side%u/back%u starts=fwd%llu/side%llu/back%llu eligible=%llu not_combat=%llu from_recovery=%llu not_human=%llu "
                "restarts=%llu protected_ticks=%llu crossings_on_link=%llu eligible_contacts=%llu evaded=%llu(fwd%llu/side%llu/back%llu) held=%llu excluded=%llu outside_window=%llu "
                "not_eligible=%llu by_enemy=dkn%llu/liz%llu/kk%llu/oc%llu scene_resets=%llu calls=%llu/%llu lines_dropped=%llu runtime=HUMAN_RUNTIME_PENDING",
                ldg::kVersion, lq.enabled ? 1 : 0, ldg::policy::kForwardTicks, ldg::policy::kSideTicks, ldg::policy::kBackTicks, U(t.starts[1]), U(t.starts[2]), U(t.starts[3]),
                U(t.eligible_n), U(t.not_combat), U(t.from_recovery_n), U(t.not_human), U(t.restarts), U(t.ticks_protected), U(lq.crossings_link), U(lq.eligible_contacts), U(lq.evaded),
                U(lq.evaded_by[1]), U(lq.evaded_by[2]), U(lq.evaded_by[3]), U(lq.held_continued), U(lq.excluded), U(lq.outside), U(lq.not_eligible), U(lq.by_enemy[0]),
                U(lq.by_enemy[1]), U(lq.by_enemy[2]), U(lq.by_enemy[3]), U(lq.scene_resets), U(lq.calls_obs), U(lq.calls_cross), U(lq.dropped));
  lnk_write(LOG_LEVEL_INFO, line);
}
void ldg_shutdown() noexcept {
  if (lq.enabled) ldg_summary();
  lq.enabled = false, lq.tr.reset(), lq.held.clear();
}
void ldg_install() noexcept {
  lq = LdgState{};
  const ModResult r0 = mods::hook::add_pre<LinkCollisionFx>(on_ldg_collision_pre);
  const ModResult r1 = mods::hook::add_post<LdgAfterCross>(on_ldg_after_cross_post);
  lq.enabled = r0 == MOD_OK && r1 == MOD_OK;  // all or nothing: a missing observer never leaves a veto without its window (and the other way round)
  if (!lq.enabled) (void)mods::hook::uninstall<LdgAfterCross>();
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LDG01 event=INSTALL version=%s tag=%s enabled=%d observe_hook=%d cross_hook=%d window_ticks=fwd%u/side%u/back%u exclusions=FORWARD_vs_THRUST(DKN_F2A),SWEEP(NONE_MAPPED) "
                "eligible_attackers=DKN01|LIZ|CHILFOS|E_OC scope=CONTACT_COMMIT_ONLY combat_reach=%.0f(PROVISIONAL) runtime=HUMAN_RUNTIME_PENDING",
                ldg::kVersion, ldg::kTag, lq.enabled ? 1 : 0, int(r0), int(r1), ldg::policy::kForwardTicks, ldg::policy::kSideTicks, ldg::policy::kBackTicks,
                static_cast<double>(ldg::policy::kCombatReach));
  lnk_write(lq.enabled ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}
