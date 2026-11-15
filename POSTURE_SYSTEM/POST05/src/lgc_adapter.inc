// LIGHT FADES TWICE 0.13.14 (DKN01 FIX05) LGC01: production adapter of Link's guard cancel of an ordinary sword cut (src/lgc_core.hpp decides).
// Included once in mod.cpp's anonymous namespace after dkn_adapter.inc. ONE hook target: daAlink_c::procCutNormal PRE (exported, called for real through
// the procedure table by daAlink_c::execute; prologue `mov [rsp+20h],rbp` 5 bytes, nothing IP-relative: ci/hook_prologue_audit.py). ONE native writer:
// lgc_native_cut_end = the very end of procCutNormal (resetCombo(TRUE) + checkNextAction(0)), run earlier, inside the cut's own procedure call, exactly
// where the native runs it; then the original body is skipped for that call (the cut is gone). The other verdicts: the original body runs (NATIVE /
// BUFFER), or is skipped for ONE call of a cut that received R+B in its recovery (HOLD: that B never chains a cut). Link's input is never written; the
// buffered B reaches the native Shield Attack site only through the manual shield's own decide_bash (lgc_bash_now, true only inside the cancel call).
// Nothing here depends on an enemy: Darknut, Lizalfos, Chilfos, Bokoblins, other mods and the rest of Link are untouched (only PROC_CUT_NORMAL with an
// ordinary cut type, human Link, sword, ground, lock-on, the manual shield READY, no damage timer, no break penalty).
#if defined(_MSC_VER)
#define LGC_NOINLINE __declspec(noinline)
#else
#define LGC_NOINLINE __attribute__((noinline))
#endif
static_assert(static_cast<int>(daAlink_c::PROC_CUT_NORMAL) == 0x21 && static_cast<int>(daAlink_c::PROC_GUARD_ATTACK) == 0x1e, "LGC01 Link procedures != exe 2.0.3");
static_assert(static_cast<int>(daPy_py_c::CUT_TYPE_NM_VERTICAL) == lgc::kCutNmVertical && static_cast<int>(daPy_py_c::CUT_TYPE_NM_STAB) == lgc::kCutNmStab &&
                  static_cast<int>(daPy_py_c::CUT_TYPE_NM_RIGHT) == lgc::kCutNmRight && static_cast<int>(daPy_py_c::CUT_TYPE_NM_LEFT) == lgc::kCutNmLeft &&
                  static_cast<int>(daPy_py_c::CUT_TYPE_COMBO_STAB) == lgc::kCutComboStab,
              "LGC01 ordinary cut types != procCutNormalInit cutParamTable");
static_assert(std::is_same_v<decltype(&daAlink_c::procCutNormal), int (daAlink_c::*)()> && std::is_same_v<decltype(&daAlink_c::checkNextAction), int (daAlink_c::*)(int)> &&
                  std::is_same_v<decltype(&daAlink_c::resetCombo), void (daAlink_c::*)(int)> && std::is_same_v<decltype(&daAlink_c::checkGuardActionChange), BOOL (daAlink_c::*)()>,
              "LGC01 native signatures");
struct LgcRuntime {
  bool enabled = false, hooks = false, bash_now = false;
  lgc::Ctl ctl;
  std::uint64_t calls = 0, cancel_ok = 0, cancel_native_refused = 0, shield_attacks = 0, guard_procs = 0, pos_moved = 0, angle_moved = 0, holds = 0;
  std::uint64_t lat_n[3] = {}, lat_sum[3] = {}, lat_max[3] = {}, saved_sum[3] = {}, saved_max[3] = {};
  unsigned lines = 0;
  std::uint64_t dropped = 0, failures = 0;
} lgc_rt;
// True only inside the cancel call of a cut that buffered R+B: the manual shield's decide_bash then reads that B (once) at the native Shield Attack site.
bool lgc_bash_now() noexcept { return lgc_rt.enabled && lgc_rt.bash_now; }
void lgc_line(bool warn, const char* line) noexcept {
  if (lgc_rt.lines >= 600) {
    ++lgc_rt.dropped;
    return;
  }
  ++lgc_rt.lines;
  lnk_write(warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}
// The ONE native writer: the cut's own end (procCutNormal, cut.inc: checkAnmEnd -> resetCombo(TRUE); checkNextAction(0)), earlier.
LGC_NOINLINE int lgc_native_cut_end(daAlink_c* k) noexcept {
  k->resetCombo(1);
  return k->checkNextAction(0);
}
HookAction on_lgc_cut_normal_pre(ModContext*, void* args, void* ret, void*) noexcept {
  try {
    if (!lgc_rt.enabled || !args || !ret) return HOOK_CONTINUE;
    auto* k = mods::arg<daAlink_c*>(args, 0);
    std::uint32_t id = 0;
    ++lgc_rt.calls;
    lgc::View v;
    v.human = lnkfx_link(k, id);
    if (!v.human) {
      lgc_rt.ctl.reset();
      return HOOK_CONTINUE;
    }
    const bool sample = lf.sample_ok && lf.key == reinterpret_cast<std::uintptr_t>(k) && lf.sample_tick == g.p8.tick;
    v.cut_normal = static_cast<int>(k->mProcID) == daAlink_c::PROC_CUT_NORMAL;
    v.cut = k->getCutType(), v.combo = k->getCutCount();
    v.safe = sample && cp04_link_safe(k);
    v.sword = k->mEquipItem == 0x103;
    v.ground = k->mLinkAcch.ChkGroundHit();
    v.r_held = sample && lf.r_held, v.r_down_tick = lf.r_down_tick, v.b_trig = sample && lf.b_trig;
    v.frame = k->mUnderFrameCtrl[0].getFrame(), v.at_start = k->field_0x3478, v.at_end = k->field_0x347c, v.speed = k->mNormalSpeed, v.tick = g.p8.tick;
    v.lock = v.r_held && k->checkGuardActionChange() != 0;  // the guard's own lock-on branch (setShieldGuard); asked only while R is held
    const lgc::Out o = lgc_rt.ctl.step(v);
    if (o.v == lgc::Verdict::Native || o.v == lgc::Verdict::Buffer) {
      if (o.v == lgc::Verdict::Buffer && o.intent_age == 0) {
        char line[kLogLineMax];
        std::snprintf(line, sizeof(line), "LGC01 event=INTENT_BUFFERED region=%s cut=%d frame=%.1f at=%.1f..%.1f speed=%.1f reason=%s tick=%llu", lgc::name(o.region), v.cut,
                      static_cast<double>(v.frame), static_cast<double>(v.at_start), static_cast<double>(v.at_end), static_cast<double>(v.speed), lgc::name(o.why),
                      static_cast<unsigned long long>(v.tick));
        lgc_line(false, line);
      } else if (o.why == lgc::Why::Unsafe || o.why == lgc::Why::NoLock || o.why == lgc::Why::Airborne || o.why == lgc::Why::NotSword) {
        char line[kLogLineMax];
        std::snprintf(line, sizeof(line), "LGC01 event=CANCEL_REFUSED reason=%s region=%s cut=%d frame=%.1f native=CUT_KEPT tick=%llu", lgc::name(o.why), lgc::name(o.region), v.cut,
                      static_cast<double>(v.frame), static_cast<unsigned long long>(v.tick));
        lgc_line(false, line);
      }
      return HOOK_CONTINUE;
    }
    if (o.v == lgc::Verdict::Hold) {  // R+B in the recovery: this call of the cut's procedure is skipped (its B never becomes a combo cut); next call cancels
      ++lgc_rt.holds;
      *static_cast<int*>(ret) = 1;
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "LGC01 event=HOLD_RB region=%s cut=%d frame=%.1f reason=NEW_B_WITH_R_NEVER_A_CUT next=CANCEL_WITH_SHIELD_ATTACK_OFFER tick=%llu", lgc::name(o.region), v.cut,
                    static_cast<double>(v.frame), static_cast<unsigned long long>(v.tick));
      lgc_line(false, line);
      return HOOK_SKIP_ORIGINAL;
    }
    // CANCEL: the cut's own end, now. Nothing of the pose / frame / position / angle is written: the native next procedure morphs from the current pose.
    const float end = k->mUnderFrameCtrl[0].getEnd(), rate = k->mUnderFrameCtrl[0].getRate();
    const float remaining = rate > 0.01f && end > v.frame ? (end - v.frame) / rate : 0.0f;  // ticks the native cut would still have run (to its clip end)
    const cXyz p0 = k->current.pos;
    const std::int16_t a0 = k->shape_angle.y;
    lgc_rt.bash_now = o.bash;
    const int r = lgc_native_cut_end(k);
    lgc_rt.bash_now = false;
    const int proc = static_cast<int>(k->mProcID);
    if (proc == daAlink_c::PROC_CUT_NORMAL) {  // the native refused to leave the cut (never expected with checkNextAction(0)): the cut simply runs on
      ++lgc_rt.cancel_native_refused;
      char line[kLogLineMax];
      std::snprintf(line, sizeof(line), "LGC01 event=CANCEL_NATIVE_REFUSED region=%s cut=%d frame=%.1f ret=%d native=CUT_CONTINUES combo=DROPPED tick=%llu", lgc::name(o.region), v.cut,
                    static_cast<double>(v.frame), r, static_cast<unsigned long long>(v.tick));
      lgc_line(true, line);
      return HOOK_CONTINUE;
    }
    ++lgc_rt.cancel_ok;
    const unsigned ri = static_cast<unsigned>(o.region) < 3 ? static_cast<unsigned>(o.region) : 0u;
    const std::uint64_t lat = o.intent_age + 1, saved = static_cast<std::uint64_t>(remaining);  // the guard flag is set by the next setShieldGuard
    ++lgc_rt.lat_n[ri], lgc_rt.lat_sum[ri] += lat, lgc_rt.saved_sum[ri] += saved;
    if (lat > lgc_rt.lat_max[ri]) lgc_rt.lat_max[ri] = lat;
    if (saved > lgc_rt.saved_max[ri]) lgc_rt.saved_max[ri] = saved;
    const float dx = k->current.pos.x - p0.x, dy = k->current.pos.y - p0.y, dz = k->current.pos.z - p0.z;
    const bool moved = dx * dx + dy * dy + dz * dz > 1e-4f, turned = k->shape_angle.y != a0;
    lgc_rt.pos_moved += moved, lgc_rt.angle_moved += turned;
    if (proc == daAlink_c::PROC_GUARD_ATTACK) ++lgc_rt.shield_attacks;
    else ++lgc_rt.guard_procs;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line),
                  "LGC01 event=CANCEL region=%s cut=%d combo=%d frame=%.1f at=%.1f..%.1f end=%.1f rate=%.2f intent_to_guard_ticks=%llu native_ticks_saved=%llu proc_after=%d bash=%d "
                  "shield_attack=%d pos_written=%d angle_written=%d transition=NATIVE_resetCombo+checkNextAction(0) tick=%llu visual=HUMAN_RUNTIME_PENDING",
                  lgc::name(o.region), v.cut, v.combo, static_cast<double>(v.frame), static_cast<double>(v.at_start), static_cast<double>(v.at_end), static_cast<double>(end),
                  static_cast<double>(rate), static_cast<unsigned long long>(lat), static_cast<unsigned long long>(saved), proc, o.bash ? 1 : 0, proc == daAlink_c::PROC_GUARD_ATTACK ? 1 : 0,
                  moved ? 1 : 0, turned ? 1 : 0, static_cast<unsigned long long>(v.tick));
    lgc_line(moved || turned, line);
    *static_cast<int*>(ret) = 1;
    return HOOK_SKIP_ORIGINAL;
  } catch (...) {
    ++lgc_rt.failures;
    lgc_rt.enabled = false;
    lgc_rt.bash_now = false;
    return HOOK_CONTINUE;
  }
}
void lgc_install() noexcept {
  lgc_rt = LgcRuntime{};
  const ModResult r = mods::hook::add_pre<LinkCutNormalLgc>(on_lgc_cut_normal_pre);
  lgc_rt.hooks = r == MOD_OK;
  // all or nothing, and only with the manual shield (R sampler, decide_bash) and CP04 (the world / Link safety it shares)
  lgc_rt.enabled = lgc_rt.hooks && lf.enabled && c4.enabled;
  if (lgc_rt.hooks && !lgc_rt.enabled) (void)mods::hook::uninstall<LinkCutNormalLgc>();
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "LGC01 event=INSTALL version=%s tag=%s enabled=%d hook=%d manual_shield=%d cp04=%d target=daAlink_c::procCutNormal(PRE) cuts=NM_VERTICAL|NM_LEFT|NM_RIGHT|NM_STAB|COMBO_STAB "
                "regions=PREP_cancel/AT_buffer/RECOVERY_cancel transition=NATIVE_resetCombo(TRUE)+checkNextAction(0) rb=native_site_once_buffer_%u fail_closed=UNSAFE|NO_LOCK|AIRBORNE|NOT_SWORD|PENALTY "
                "excluded=REBOUND|GUARD_BREAK|FINISHERS|MORTAL_DRAW|HELM_SPLITTER|JUMP|SPIN|WOLF|HORSE runtime=HUMAN_RUNTIME_PENDING",
                lgc::kVersion, lgc::kTag, lgc_rt.enabled ? 1 : 0, lgc_rt.hooks ? 1 : 0, lf.enabled ? 1 : 0, c4.enabled ? 1 : 0, lgc::policy::kBashBufferTicks);
  lnk_write(lgc_rt.enabled ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}
void lgc_shutdown() noexcept {
  const bool was = lgc_rt.enabled;
  lgc_rt.enabled = false, lgc_rt.bash_now = false;
  if (!was && !lgc_rt.hooks) return;
  const lgc::Ctl& c = lgc_rt.ctl;
  using U = unsigned long long;
  auto avg = [](std::uint64_t s, std::uint64_t n) { return n ? static_cast<double>(s) / static_cast<double>(n) : 0.0; };
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "LGC01_SUMMARY version=%s calls=%llu cuts=%llu intents=%llu cancels=%llu(prep %llu recovery %llu) native_refused=%llu guard_procs=%llu shield_attacks=%llu bash_offers=%llu "
                "bash_expired=%llu holds=%llu buffered_ticks=%llu released_r=%llu refused=unsafe:%llu/no_lock:%llu/airborne:%llu/not_sword:%llu "
                "latency_ticks_avg/max=prep:%.1f/%llu recovery:%.1f/%llu native_ticks_saved_avg/max=prep:%.1f/%llu recovery:%.1f/%llu pos_written=%llu angle_written=%llu failures=%llu "
                "lines_dropped=%llu visual=HUMAN_RUNTIME_PENDING",
                lgc::kVersion, U(lgc_rt.calls), U(c.cuts), U(c.intents), U(lgc_rt.cancel_ok), U(c.cancels[0]), U(c.cancels[2]), U(lgc_rt.cancel_native_refused), U(lgc_rt.guard_procs),
                U(lgc_rt.shield_attacks), U(c.bashes), U(c.bash_expired), U(lgc_rt.holds), U(c.buffered), U(c.released), U(c.refused[unsigned(lgc::Why::Unsafe)]),
                U(c.refused[unsigned(lgc::Why::NoLock)]), U(c.refused[unsigned(lgc::Why::Airborne)]), U(c.refused[unsigned(lgc::Why::NotSword)]), avg(lgc_rt.lat_sum[0], lgc_rt.lat_n[0]),
                U(lgc_rt.lat_max[0]), avg(lgc_rt.lat_sum[2], lgc_rt.lat_n[2]), U(lgc_rt.lat_max[2]), avg(lgc_rt.saved_sum[0], lgc_rt.lat_n[0]), U(lgc_rt.saved_max[0]),
                avg(lgc_rt.saved_sum[2], lgc_rt.lat_n[2]), U(lgc_rt.saved_max[2]), U(lgc_rt.pos_moved), U(lgc_rt.angle_moved), U(lgc_rt.failures), U(lgc_rt.dropped));
  lnk_write(LOG_LEVEL_INFO, line);  // (the host removes the hooks itself at unload: nothing of Link is held or pending here)
}
#undef LGC_NOINLINE
