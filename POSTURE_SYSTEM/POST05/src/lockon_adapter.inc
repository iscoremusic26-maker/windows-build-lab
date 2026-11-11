#if defined(_MSC_VER)
#define LOCKON_NOINLINE __declspec(noinline)
#else
#define LOCKON_NOINLINE __attribute__((noinline))
#endif
// LIGHT FADES TWICE 0.12.3 FIX03 P1-4 production adapter (included once inside mod.cpp's anonymous namespace, after cp04_adapter.inc).
// Option: ConfigService bool "sekiro_lockon" (full key mod.<escaped id>.sekiro_lockon in the host config.json), default ON, shown as a toggle bound
// to that var in this mod's own panel of the host Mods window (UiService register_mods_panel). Without ConfigService the feature is UNAVAILABLE:
// nothing is written, the native lock-on stays, and the log says so; it is never a hardcoded flag.
// Native writes (two scoped writers, audited): the logical L of PAD_1's mDoCPd_c::m_cpadInfo (mHoldLockL/mTrigLockL) and its right stick (neutral)
// for THIS frame only (the next mDoCPd_c::read rewrites them); dAttention_c fields of the native lock (mAttnStatus NONE on a SWITCH-option release,
// mLockOnOffset / mLockTargetID / field_0x32e on a flick, exactly what nextAttention changes). No actor, camera or save field is ever written.
struct LockonRuntime {
  bool available = false, enabled = false, ui = false;
  ConfigVarHandle var = 0;
  lockon::Toggle t;
  lockon::St logged = lockon::St::Idle;
  unsigned lines = 0;
  std::uint64_t dropped = 0, changes = 0, writes_pad = 0, writes_attention = 0;
} lk;
void lockon_line(const char* event, const char* detail, int a = 0, int b = 0) noexcept {
  if (lk.lines >= 200) {
    ++lk.dropped;
    return;
  }
  ++lk.lines;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "LOCKON event=%s version=%s option=%d state=%s detail=%s a=%d b=%d tick=%llu runtime=HUMAN_RUNTIME_PENDING", event, lockon::kVersion,
                lk.enabled ? 1 : 0, lockon::name(lk.t.st), detail, a, b, static_cast<unsigned long long>(g.p8.tick));
  lnk_write(LOG_LEVEL_INFO, line);
}
// Writer 1: this frame's logical L and right stick of PAD_1 (the frame's own input copy; mDoCPd_c::read rewrites all of it next tick).
LOCKON_NOINLINE void lockon_native_pad(interface_of_controller_pad& p, const lockon::Out& o) noexcept {
  if (o.write) {
    p.mHoldLockL = o.hold ? 1 : 0;
    p.mTrigLockL = o.trig ? 1 : 0;
  }
  if (o.mute_stick) {
    p.mCStickPosX = 0.0f, p.mCStickPosY = 0.0f, p.mCStickValue = 0.0f;
    p.mCStickAngle = 0;
  }
}
// Writer 2: the native lock state, only the fields nextAttention / a release change.
LOCKON_NOINLINE void lockon_native_attention(dAttention_c* att, bool release, int offset, fpc_ProcID target) noexcept {
  if (release) {
    att->mAttnStatus = dAttention_c::EState_NONE;  // the next Run's NONE branch frees the target and rebuilds its list (native path)
  } else {
    att->mLockOnOffset = offset;
    att->mLockTargetID = target;
    att->field_0x32e = 15;  // the native hold-off of a fresh lock / nextAttention before the list is rebuilt
  }
}
bool lockon_safe(dAttention_c* att) noexcept {
  if (!att || att->mPadNo != 0) return false;
  if (g_dComIfG_gameInfo.play.isPauseFlag() || dMeter2Info_getWindowStatus() != 0 || g_dComIfG_gameInfo.play.getEvent()->runCheck()) return false;
  if (g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() <= 0) return false;
  auto* lp = dComIfGp_getLinkPlayer();
  if (!lp || lp->profname != fpcNm_ALINK_e || lp->state.init_state == 3) return false;
  auto* link = static_cast<daAlink_c*>(lp);
  return !link->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF) && !link->checkHorseRide();
}
// The candidates the native lock list already holds, seen from the camera (horizontal screen coordinate = side / depth).
unsigned lockon_candidates(dAttention_c* att, lockon::Cand out[lockon::kMaxCand], fpc_ProcID pid[lockon::kMaxCand]) noexcept {
  const int n = att->mLockonCount;
  if (n <= 0 || n > int(lockon::kMaxCand)) return 0;
  dCamera_c* cam = dCam_getBody();
  if (!cam) return 0;
  const cXyz e = cam->iEye(), c = cam->iCenter();
  const float fx = c.x - e.x, fz = c.z - e.z;
  const int cur = att->mLockOnOffset % n;
  for (int i = 0; i < n; ++i) {
    out[i] = lockon::Cand{};
    pid[i] = att->mLockOnList[i].getPid();
    out[i].current = i == cur;
    fopAc_ac_c* ac = nullptr;
    if (!fopAcM_SearchByID(pid[i], &ac) || !ac) continue;
    const cXyz& p = ac->attention_info.position;
    const float rx = p.x - e.x, rz = p.z - e.z;
    const float depth = rx * fx + rz * fz, side = rx * -fz + rz * fx;  // screen right = forward x up = (-fz, fx)
    if (depth <= 1.0f) continue;
    out[i].x = side / depth;
    out[i].valid = att->mLockOnList[i].mType == fopAc_attn_BATTLE_e;
  }
  return unsigned(n);
}
void lockon_update() noexcept {
  if (!lk.available) return;
  dAttention_c* att = dComIfGp_getAttention();
  lockon::In in;
  in.enabled = lk.enabled;
  in.safe = lockon_safe(att);
  interface_of_controller_pad* pad = in.safe ? &mDoCPd_c::m_cpadInfo[0] : nullptr;
  if (pad) {
    in.trigger = pad->mTriggerLeft;
    in.cx = pad->mCStickPosX;
    in.locked = att->mAttnStatus == dAttention_c::EState_LOCK;
    const int n = att->mLockonCount;
    in.enemy = in.locked && n > 0 && n <= int(lockon::kMaxCand) && att->mLockOnList[att->mLockOnOffset % n].mType == fopAc_attn_BATTLE_e;
    in.mode = dComIfGs_getOptAttentionType() == 0 ? lockon::Mode::Hold : lockon::Mode::Switch;
  }
  const auto c0 = lk.t.c;
  const lockon::Out o = lk.t.step(in);
  if (pad && (o.write || o.mute_stick)) lockon_native_pad(*pad, o), ++lk.writes_pad;
  if (pad && o.force_release) lockon_native_attention(att, true, 0, 0), ++lk.writes_attention;
  if (pad && o.switch_dir != 0) {
    lockon::Cand cand[lockon::kMaxCand];
    fpc_ProcID pid[lockon::kMaxCand] = {};
    const unsigned n = lockon_candidates(att, cand, pid);
    const int best = lockon::choose(cand, n, o.switch_dir);
    if (best >= 0) {
      lockon_native_attention(att, false, best, pid[best]);
      ++lk.writes_attention;
      ++lk.t.c.switched;
      lockon_line("SWITCH", o.switch_dir > 0 ? "RIGHT" : "LEFT", best, int(n));
    } else {
      ++lk.t.c.no_candidate;
      lockon_line("SWITCH_NO_CANDIDATE", o.switch_dir > 0 ? "RIGHT" : "LEFT", -1, int(n));
    }
  }
  if (lk.t.c.toggled != c0.toggled) lockon_line("TOGGLED", "LOCK_KEPT_AFTER_RELEASE");
  if (lk.t.c.cancelled != c0.cancelled) lockon_line("CANCELLED", o.force_release ? "SECOND_PRESS_SWITCH_OPTION" : "SECOND_PRESS_HOLD_OPTION");
  if (lk.t.c.lost != c0.lost) lockon_line("LOST", "NATIVE_ATTENTION_LEFT_LOCK");
  if (lk.t.c.option_off != c0.option_off) lockon_line("NATIVE_RESTORED", "OPTION_OFF");
  if (lk.t.c.unsafe != c0.unsafe) lockon_line("NATIVE_RESTORED", "EVENT_MENU_PAUSE_WOLF_HORSE_OR_DEATH");
}
void lockon_option_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void*) noexcept {
  if (!value || value->type != CONFIG_VAR_BOOL) return;
  lk.enabled = value->bool_value;
  ++lk.changes;
  lockon_line("OPTION_CHANGED", lk.enabled ? "ON" : "OFF");
}
ModResult lockon_panel(ModContext* ctx, UiElementHandle pane, void*, ModError*) noexcept {
  if (!svc_ui || !lk.available) return MOD_OK;
  (void)svc_ui->pane_add_section(ctx, pane, "Light Fades Twice");
  UiControlDesc d = UI_CONTROL_DESC_INIT;
  d.kind = UI_CONTROL_TOGGLE;
  d.label = "Fijaci\xC3\xB3n de objetivos estilo Sekiro";
  d.help_rml = "Pulsa una vez el bot\xC3\xB3n de fijaci\xC3\xB3n para fijar a un enemigo; al soltarlo sigue fijado. Vuelve a pulsarlo para soltarlo. "
               "Con un enemigo fijado, mueve el stick derecho a izquierda o derecha para pasar al enemigo de ese lado. Desactivado: fijaci\xC3\xB3n nativa del juego.";
  d.tooltip = "Fijar con una pulsaci\xC3\xB3n, soltar con otra; stick derecho cambia de enemigo";
  d.binding = UI_BINDING_CONFIG_VAR;
  d.config_var = lk.var;
  return svc_ui->pane_add_control(ctx, pane, &d, nullptr);
}
void lockon_install() noexcept {
  lk = LockonRuntime{};
  if (!svc_config || !svc_config->register_var || !svc_config->get_bool) {
    lockon_line("UNAVAILABLE", "NO_CONFIG_SERVICE_NATIVE_LOCK_ON_KEPT");
    return;
  }
  ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
  desc.name = "sekiro_lockon";
  desc.type = CONFIG_VAR_BOOL;
  desc.default_bool = true;
  ConfigVarHandle h = 0;
  bool value = false;
  if (svc_config->register_var(mod_ctx, &desc, &h) != MOD_OK || h == 0 || svc_config->get_bool(mod_ctx, h, &value) != MOD_OK) {
    lockon_line("UNAVAILABLE", "CONFIG_VAR_REGISTRATION_FAILED_NATIVE_LOCK_ON_KEPT");
    return;
  }
  lk.var = h;
  lk.enabled = value;
  if (svc_config->subscribe) (void)svc_config->subscribe(mod_ctx, h, lockon_option_changed, nullptr, nullptr);
  lk.available = true;
  if (svc_ui && svc_ui->register_mods_panel && svc_ui->pane_add_control) {
    UiModsPanelDesc p = UI_MODS_PANEL_DESC_INIT;
    p.build = lockon_panel;
    lk.ui = svc_ui->register_mods_panel(mod_ctx, &p) == MOD_OK;
  }
  lockon_line("INSTALL", lk.ui ? "MOD_PANEL_TOGGLE_BOUND_TO_CONFIG_VAR" : "CONFIG_VAR_ONLY_NO_UI_SERVICE", lk.enabled ? 1 : 0, lk.ui ? 1 : 0);
}
void lockon_shutdown() noexcept {
  char line[kLogLineMax];
  const auto& c = lk.t.c;
  const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
  std::snprintf(line, sizeof(line),
                "LOCKON_SUMMARY version=%s available=%d ui=%d option=%d changes=%llu frames=%llu held=%llu toggled=%llu cancelled=%llu forced_releases=%llu lost=%llu flicks=%llu switched=%llu no_candidate=%llu "
                "option_off_restores=%llu unsafe_restores=%llu synth_frames=%llu muted_frames=%llu pad_writes=%llu attention_writes=%llu lines_dropped=%llu runtime=HUMAN_RUNTIME_PENDING",
                lockon::kVersion, lk.available ? 1 : 0, lk.ui ? 1 : 0, lk.enabled ? 1 : 0, u(lk.changes), u(c.frames), u(c.held), u(c.toggled), u(c.cancelled), u(c.forced_releases), u(c.lost), u(c.flicks), u(c.switched),
                u(c.no_candidate), u(c.option_off), u(c.unsafe), u(c.synth_frames), u(c.muted_frames), u(lk.writes_pad), u(lk.writes_attention), u(lk.dropped));
  lnk_write(LOG_LEVEL_INFO, line);
  lk.available = false;  // nothing is synthesized after this; the host removes the var/panel registrations with the mod
  lk.t.reset();
}
#undef LOCKON_NOINLINE
