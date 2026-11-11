#if defined(_MSC_VER)
#define PFX_NOINLINE __declspec(noinline)
#else
#define PFX_NOINLINE __attribute__((noinline))
#endif
// LIGHT FADES TWICE 0.12.4 FIX04 (P1-3 of FIX03 repaired) production adapter (included once inside mod.cpp's anonymous namespace, after liz_adapter.inc).
// The orange spark is the game's OWN spark instance(s) of the parry contact; the high clash is the game's OWN collision SE playback of that contact.
// Native writes (three scoped writers, audited): the instance colours of the captured emitters (emtr->mPrmClr/mEnvClr and their particles'
// mPrmClr/mEnvClr) inside their own executeAfter; the emitter callback pointer of those captured emitters (chained: the native one keeps running);
// JAISoundParamsMove::movePitch of the returned playback handle. No JPAResource, texture, sample, sound table or other emitter is touched.
struct PfxEmitter {
  JPABaseEmitter* e = nullptr;
  JPAEmitterCallBack* prev = nullptr;
  int rec = -1;
  GXColor prm{}, env{};
  std::uint16_t res = 0;
  bool colored = false;
};
// FIX04: every hook by name, its kind and its install result; every callback counted, so "installed" and "ran" are two separate facts in the log.
enum PfxHook : unsigned { kPfxHitmarkPre = 0, kPfxHitmarkPost, kPfxMarkPre, kPfxMarkPost, kPfxParticlePost, kPfxGuardPre, kPfxGuardPost, kPfxCollisionPost, kPfxHookCount };
struct PfxRuntime {
  bool enabled = false, spark = false, sound = false;  // enabled = the decision hooks; spark and sound fail separately: one missing hook never fakes the other
  std::uint64_t calls[kPfxHookCount] = {};
  JAISoundHandle* readback = nullptr;
  std::uint64_t readback_tick = ~0ull, readback_ok = 0, readback_changed = 0;
  float readback_want = 0;
  std::uint32_t readback_owner = 0;
  pfx::Book book;
  PfxEmitter em[pfx::kRecords * pfx::kEmitters];
  int ctx_pass = -1;     // record of the ProcAtTgHitmark call in progress (collision pass)
  int ctx_mark = -1;     // record whose setHitMark is in progress: every emitter created inside belongs to it
  int ctx_sound = -1;    // record whose clash SE is being started
  std::uint32_t exec_id = 0;   // admitted Lizalfos whose Execute is running (its own setHitMark / def_se_set)
  const void* exec_sound = nullptr;  // the Z2SoundObjBase of its Z2Creature: the `this` of the clash SE
  JAISoundHandle* last_handle = nullptr;
  std::uint64_t last_handle_tick = ~0ull;
  float last_pitch = 1.0f;
  std::uint64_t epoch = 0, captured_res[2] = {}, depth_errors = 0;
  unsigned lines = 0;
  std::uint64_t dropped = 0;
} pf;
void pfx_line(const char* event, int rec, const char* detail, double a = 0, double b = 0) noexcept {
  if (pf.lines >= 240) {
    ++pf.dropped;
    return;
  }
  ++pf.lines;
  const pfx::Rec* r = rec >= 0 && unsigned(rec) < pfx::kRecords ? &pf.book.r[rec] : nullptr;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "PARRY_FX event=%s version=%s kind=%s actor=%u pass_tick=%llu emitters=%u sound=%d detail=%s a=%.3f b=%.3f tick=%llu runtime=HUMAN_RUNTIME_PENDING", event, pfx::kVersion,
                r ? pfx::name(r->kind) : "NONE", r ? r->actor : 0u, r ? static_cast<unsigned long long>(r->pass) : 0ull, r ? r->emitters : 0u, r && r->sound ? 1 : 0, detail, a, b,
                static_cast<unsigned long long>(g.p8.tick));
  lnk_write(LOG_LEVEL_INFO, line);
}
PfxEmitter* pfx_slot(const JPABaseEmitter* e) noexcept {
  for (auto& x : pf.em)
    if (x.e == e && x.rec >= 0) return &x;
  return nullptr;
}
// Writer 1: the colours of THIS emitter instance and of its own living particles, as whole GXColor stores (alpha always the native one).
// Idempotent (HSV hue only), so a repeated calc never drifts; a reverted preview gets the resource's own colour back.
PFX_NOINLINE GXColor pfx_tint(GXColor c, GXColor original, bool orange) noexcept {  // pure: no store to any native object
  if (!orange) return GXColor{original.r, original.g, original.b, c.a};
  const pfx::Rgb o = pfx::orange(pfx::Rgb{c.r, c.g, c.b});
  return GXColor{o.r, o.g, o.b, c.a};
}
PFX_NOINLINE void pfx_native_colors(JPABaseEmitter* e, GXColor prm, GXColor env, bool orange) noexcept {
  GXColor t = pfx_tint(e->mPrmClr, prm, orange);
  std::memcpy(&e->mPrmClr, &t, sizeof(t));  // one 4-byte store per colour field
  t = pfx_tint(e->mEnvClr, env, orange);
  std::memcpy(&e->mEnvClr, &t, sizeof(t));
  for (JPANode<JPABaseParticle>* n = e->mAlivePtclBase.getFirst(); n; n = n->getNext()) {
    JPABaseParticle* p = n->getObject();
    t = pfx_tint(p->mPrmClr, prm, orange);
    std::memcpy(&p->mPrmClr, &t, sizeof(t));
    t = pfx_tint(p->mEnvClr, env, orange);
    std::memcpy(&p->mEnvClr, &t, sizeof(t));
  }
  for (JPANode<JPABaseParticle>* n = e->mAlivePtclChld.getFirst(); n; n = n->getNext()) {
    JPABaseParticle* p = n->getObject();
    t = pfx_tint(p->mPrmClr, prm, orange);
    std::memcpy(&p->mPrmClr, &t, sizeof(t));
    t = pfx_tint(p->mEnvClr, env, orange);
    std::memcpy(&p->mEnvClr, &t, sizeof(t));
  }
}
// The chained callback: every call goes to the native callback the emitter already had; executeAfter (end of calc, before any draw) recolours.
struct PfxCallBack final : JPAEmitterCallBack {
  ~PfxCallBack() override {}
  void execute(JPABaseEmitter* e) override {
    PfxEmitter* s = pfx_slot(e);
    if (!s) {
      ++pf.book.c.orphan_callbacks;
      return;
    }
    if (s->prev) s->prev->execute(e);
  }
  void executeAfter(JPABaseEmitter* e) override {
    PfxEmitter* s = pfx_slot(e);
    if (!s) {
      ++pf.book.c.orphan_callbacks;
      return;
    }
    if (s->prev) s->prev->executeAfter(e);
    pfx::Rec& r = pf.book.r[s->rec];
    r.last_seen = g.p8.tick;
    const bool orange = pf.enabled && pf.book.orange_now(s->rec);
    if (orange || s->colored) {
      pfx_native_colors(e, s->prm, s->env, orange);
      s->colored = orange;
    }
  }
  // JPAResource::calc runs executeAfter BEFORE calc_p, and calc_p may rewrite a particle's colour from the resource's key frames; the emitter draw
  // callback runs after every calc and before the particles' colours are read (JPARegistPrmAlpha*, batch ctx): the last word on each drawn frame.
  void draw(JPABaseEmitter* e) override {
    PfxEmitter* s = pfx_slot(e);
    if (!s) return;
    if (s->prev) s->prev->draw(e);
    if (pf.enabled && pf.book.orange_now(s->rec)) pfx_native_colors(e, s->prm, s->env, true);
  }
  void drawAfter(JPABaseEmitter* e) override {
    PfxEmitter* s = pfx_slot(e);
    if (s && s->prev) s->prev->drawAfter(e);
  }
};
// Built once in static storage and never destroyed: no atexit, and the object outlives any emitter that still points at it.
alignas(PfxCallBack) unsigned char g_pfx_cb_mem[sizeof(PfxCallBack)];
PfxCallBack* g_pfx_cb = nullptr;
// Writer 2: the emitter's callback pointer (captured native emitter of a parry contact only; the previous callback is kept and chained).
PFX_NOINLINE void pfx_native_callback(JPABaseEmitter* e, JPAEmitterCallBack* cb) noexcept { e->mpEmtrCallBack = cb; }
bool pfx_attach(JPABaseEmitter* e, int rec, std::uint16_t res) noexcept {
  if (!e || e->mpEmtrCallBack == g_pfx_cb) return false;
  PfxEmitter* free_slot = nullptr;
  for (auto& x : pf.em)
    if (x.rec < 0 || x.e == nullptr) {
      free_slot = &x;
      break;
    }
  if (!free_slot || pf.book.r[rec].emitters >= pfx::kEmitters) {
    ++pf.book.c.emitter_overflow;
    return false;
  }
  free_slot->e = e, free_slot->prev = e->mpEmtrCallBack, free_slot->rec = rec, free_slot->prm = e->mPrmClr, free_slot->env = e->mEnvClr, free_slot->res = res, free_slot->colored = false;
  pfx_native_callback(e, g_pfx_cb);
  ++pf.book.r[rec].emitters;
  ++pf.book.c.emitters;
  {  // FIX04: what this spark really carries. If prm and env are neutral the blue lives in the texture and no instance colour can change it: the log says so.
    const pfx::Rgb p0{e->mPrmClr.r, e->mPrmClr.g, e->mPrmClr.b}, e0{e->mEnvClr.r, e->mEnvClr.g, e->mEnvClr.b};
    const pfx::Rgb p1 = pfx::orange(p0), e1 = pfx::orange(e0);
    const bool tintable = p1.r != p0.r || p1.g != p0.g || p1.b != p0.b || e1.r != e0.r || e1.g != e0.g || e1.b != e0.b;
    if (!tintable) ++pf.book.c.neutral_emitters;
    char d[96];
    std::snprintf(d, sizeof(d), "res=0x%04x_prm=%02x%02x%02x_env=%02x%02x%02x_to_%02x%02x%02x_%02x%02x%02x", res, p0.r, p0.g, p0.b, e0.r, e0.g, e0.b, p1.r, p1.g, p1.b, e1.r, e1.g, e1.b);
    pfx_line(tintable ? "EMITTER_CAPTURED" : "EMITTER_NEUTRAL_NOT_RECOLORABLE", rec, d);
  }
  return true;
}
// Writer 3: the pitch of the clash playback just started for the parry contact (this playback only).
PFX_NOINLINE void pfx_native_pitch(JAISoundHandle* h, float pitch) noexcept {
  JAISoundParamsMove& m = (*h)->getAuxiliary();
  m.movePitch(pitch, 0);
}
// ---------------------------------------------------------------- the decision inside the collision pass
// Link as the next mod_update will observe him (post08_observe_link + cp04_observe_motion inputs), without changing any tracker.
post08::LinkView pfx_link_pass(daAlink_c* link, std::uint64_t tick) noexcept {
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(link));
  const bool ga = link->mProcID == daAlink_c::PROC_GUARD_ATTACK;
  post08::LinkView v = g.link_track.peek(tick, true, id, static_cast<int>(link->mProcID), ga, link->mUnderFrameCtrl[0].getFrame(), link->field_0x3478, link->field_0x347c);
  if (v.guard_attack && c4.enabled) {
    const bool safe = cp04_link_safe(link);
    const bool clip = safe && ga && link->checkUnderMove0BckNoArc(daAlink_c::ANM_SHIELD_ATTACK);
    if (!c4.shield.peek_permits(tick, clip, clip ? link->mUnderFrameCtrl[0].getFrame() : 0.f, clip ? link->mUnderFrameCtrl[0].getRate() : 0.f, safe)) v.guard_attack = false;
  }
  return v;
}
bool pfx_link_cyl(daAlink_c* link, const cCcD_Obj* o) noexcept {
  for (int i = 0; i < 3; ++i)
    if (o == static_cast<const cCcD_Obj*>(&link->mTgCyls[i])) return true;
  return false;
}
bool pfx_link_sword(daAlink_c* link, const cCcD_Obj* o) noexcept {
  for (int i = 0; i < 3; ++i)
    if (o == static_cast<const cCcD_Obj*>(&link->mAtCps[i])) return true;
  return o == static_cast<const cCcD_Obj*>(&link->mAtCyl);
}
// (A) Link's Shield Attack on a Lizalfos at_sph: the brain's own HitIn with the pass Link -> judge_hit, exactly what the next Execute PRE judges.
bool pfx_preview_liz_link(e_dn_class* a, daAlink_c* k) noexcept {
  LizSlot* s = liz_find(a);
  if (!s || !s->b.admitted || static_cast<const void*>(&a->at_sph) == nullptr) return false;
  liz::View v = liz_view(a, k);
  v.at_hit = true, v.at_on_link = true, v.at_shield = true;  // this very pass: the engine is recording a SHIELD contact of at_sph on Link's Tg
  liz::LinkFrame L = liz_link_frame(k);
  L.pass = pfx_link_pass(k, g.p8.tick + 1);
  const liz::Hit h = liz::judge_hit(s->b.hit_input(v, L));
  return h == liz::Hit::Parry || h == liz::Hit::ParryLatched;
}
// (C) Link's sword on the Lizalfos tate_sph: the brain's contact_is_parry with Link's checkCutReverseAt arguments (the verdict is cached per swing,
// so the rebound, the credit and this colour are one and the same decision).
bool pfx_preview_liz_enemy(e_dn_class* a, daAlink_c* k) noexcept {
  LizSlot* s = liz_find(a);
  if (!s || !s->b.admitted) return false;
  liz::View v = liz_view(a, k);
  liz_contact_reads(a, k, v);
  return s->b.contact_is_parry(v, lz.swing_serial, s->b.local + 1);
}
// (B) Link's Shield Attack on an E_OC At sphere: POST08's own conditions with the pass Link; the commit of the next damage_check settles it.
int pfx_oc_slot(daE_OC_c* a) noexcept {
  if (!a || a->profname != fpcNm_E_OC_e || a->state.init_state == 3) return -1;
  const auto id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  for (unsigned i = 0; i < post08::kSlots; ++i) {
    const auto& l = g.eng.obs.ledger.slot(i);
    if (l.id == id && l.ptr == reinterpret_cast<std::uintptr_t>(a) && l.life == Life::Active && is_candidate(l.adm)) return int(i);
  }
  return -1;
}
bool pfx_preview_oc_link(daE_OC_c* a, daAlink_c* k) noexcept {
  if (!g.parry.enabled || (g.p8.layout < 0 ? !parry_layout_ok(a) : g.p8.layout != 1)) return false;  // FIX04: the very first contact with a Bokoblin: layout not cached yet, same pure check POST08 is about to make
  const int slot = pfx_oc_slot(a);
  if (slot < 0) return false;
  const auto& l = g.eng.obs.ledger.slot(unsigned(slot));
  const auto& e = g.pe.px.entry(unsigned(slot));
  if (!e.bound || e.generation != l.generation || e.id != l.id || e.latched || e.frozen || e.dead || a->health <= 1) return false;
  if (g_dComIfG_gameInfo.play.getEvent()->runCheck()) return false;
  const int mode = a->mActionMode, state = a->mOcState;
  if (mode != post08::policy::kModeAttack || (state != 1 && state != 2)) return false;
  const post08::Parry::Episode& ep = g.parry.episode(unsigned(slot));
  const float frame = a->mpMorf ? a->mpMorf->getFrame() : 0.f;
  const bool restart = !ep.bound || ep.id != l.id || ep.generation != l.generation || !ep.active || ep.state != state || (a->mpMorf && frame + 0.01f < ep.last_frame);
  if (!restart && (ep.resolved || ep.parried)) return false;
  const post08::LinkView p = pfx_link_pass(k, g.p8.tick + 1);
  if (!post08::Parry::in_window(p)) return false;
  const auto x = read_aux(a);
  if (x.resource == static_cast<std::uint8_t>(NameKind::E_OC2) && x.atp != post08::policy::kRedAtp) return false;  // possible fury chip: no preview, native colour
  return true;
}
HookAction on_pfx_hitmark_pre(ModContext*, void* args, void*, void*) noexcept {
  ++pf.calls[kPfxHitmarkPre];
  if (!pf.enabled || !args) return HOOK_CONTINUE;
  if (pf.ctx_pass >= 0) ++pf.depth_errors;
  pf.ctx_pass = -1;
  if (!mods::arg<bool>(args, 1) || !mods::arg<bool>(args, 2) || !mods::arg<bool>(args, 12)) return HOOK_CONTINUE;  // shield contacts only
  const cCcD_Obj* at = mods::arg<cCcD_Obj*>(args, 3);
  const cCcD_Obj* tg = mods::arg<cCcD_Obj*>(args, 4);
  dCcD_GObjInf* ati = mods::arg<dCcD_GObjInf*>(args, 5);
  dCcD_GObjInf* tgi = mods::arg<dCcD_GObjInf*>(args, 6);
  if (!at || !tg || !ati || !tgi) return HOOK_CONTINUE;
  daAlink_c* k = f1_link();
  if (!k || !cp04_world_safe()) return HOOK_CONTINUE;
  fopAc_ac_c* aa = ati->GetAc();
  fopAc_ac_c* ta = tgi->GetAc();
  pfx::Kind kind = pfx::Kind::None;
  fopAc_ac_c* other = nullptr;
  bool parry = false;
  if (ta == static_cast<fopAc_ac_c*>(k) && aa && pfx_link_cyl(k, tg)) {
    other = aa;
    if (fopAcM_GetName(aa) == fpcNm_E_DN_e) {
      auto* d = reinterpret_cast<e_dn_class*>(aa);
      kind = pfx::Kind::LinkOnLizalfos;
      parry = at == static_cast<const cCcD_Obj*>(&d->at_sph) && pfx_preview_liz_link(d, k);
    } else if (fopAcM_GetName(aa) == fpcNm_E_OC_e) {
      kind = pfx::Kind::LinkOnEoc;
      parry = pfx_preview_oc_link(static_cast<daE_OC_c*>(aa), k);
    }
  } else if (aa == static_cast<fopAc_ac_c*>(k) && ta && fopAcM_GetName(ta) == fpcNm_E_DN_e && pfx_link_sword(k, at)) {
    auto* d = reinterpret_cast<e_dn_class*>(ta);
    other = ta;
    kind = pfx::Kind::LizalfosOnLink;
    parry = tg == static_cast<const cCcD_Obj*>(&d->tate_sph) && pfx_preview_liz_enemy(d, k);
  }
  if (!parry || !other) return HOOK_CONTINUE;
  pf.ctx_pass = pf.book.open(kind, static_cast<std::uint32_t>(fopAcM_GetID(other)), g.p8.tick);
  return HOOK_CONTINUE;
}
void on_pfx_hitmark_post(ModContext*, void*, void*, void*) noexcept {
  ++pf.calls[kPfxHitmarkPost];
  pf.ctx_pass = -1;
}
// setHitMark: inside the parry pass, or E_DN's own shield spark (setHitMark(2, NULL, ...)) in the Execute that follows the same contact.
HookAction on_pfx_mark_pre(ModContext*, void* args, void*, void*) noexcept {
  ++pf.calls[kPfxMarkPre];
  if (!pf.enabled || !pf.spark || !args) return HOOK_CONTINUE;
  pf.ctx_mark = -1;
  const unsigned type = mods::arg<u16>(args, 1);
  if (pf.ctx_pass >= 0) {
    pf.ctx_mark = pf.ctx_pass;
  } else if (pf.exec_id && type == 2 && mods::arg<fopAc_ac_c*>(args, 2) == nullptr && g.p8.tick > 0) {
    pf.ctx_mark = pf.book.find(pfx::Kind::LizalfosOnLink, pf.exec_id, g.p8.tick - 1);
  }
  return HOOK_CONTINUE;
}
void on_pfx_mark_post(ModContext*, void*, void*, void*) noexcept {
  ++pf.calls[kPfxMarkPost];
  pf.ctx_mark = -1;
}
void on_pfx_particle_post(ModContext*, void* args, void* ret, void*) noexcept {
  ++pf.calls[kPfxParticlePost];
  if (pf.ctx_mark < 0 || !ret || !args) return;  // every other particle of the game: one comparison, nothing else
  JPABaseEmitter* e = *static_cast<JPABaseEmitter**>(ret);
  const std::uint16_t res = mods::arg<u16>(args, 2);
  if (!e) {
    ++pf.book.c.no_emitter;
    return;
  }
  if (pfx_attach(e, pf.ctx_mark, res)) {
    if (pf.captured_res[0] == 0) pf.captured_res[0] = res;
    else if (pf.captured_res[0] != res) pf.captured_res[1] = res;
  }
}
// Sound: the parry contact's clash. Link's guard SE (Link parries) or the Lizalfos' shield SE (it parries).
HookAction on_pfx_guard_se_pre(ModContext*, void* args, void*, void*) noexcept {
  ++pf.calls[kPfxGuardPre];
  pf.ctx_sound = -1;
  if (!pf.enabled || !pf.sound || !args || g.p8.tick == 0) return HOOK_CONTINUE;
  dCcD_GObjInf* o = mods::arg<dCcD_GObjInf*>(args, 1);
  fopAc_ac_c* att = o ? o->GetTgHitAc() : nullptr;
  if (!att) return HOOK_CONTINUE;
  const auto id = static_cast<std::uint32_t>(fopAcM_GetID(att));
  int r = pf.book.find(pfx::Kind::LinkOnLizalfos, id, g.p8.tick - 1);
  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnEoc, id, g.p8.tick - 1);
  if (r >= 0 && pf.book.orange_now(r) && !pf.book.r[r].sound) pf.ctx_sound = r;
  return HOOK_CONTINUE;
}
void on_pfx_guard_se_post(ModContext*, void*, void*, void*) noexcept {
  ++pf.calls[kPfxGuardPost];
  pf.ctx_sound = -1;
}
// FIX04: def_se_set cannot be hooked (its prologue `test rcx,rcx; je short` puts a rel8 branch inside the 5 bytes funchook relocates). Its only job is to call this
// function, so the Lizalfos' shield clash is recognised HERE: this playback's owner is the sound object of the actor whose Execute is running, map info 40 (e_dn's tate contact).
void on_pfx_collision_se_post(ModContext*, void* args, void* ret, void*) noexcept {
  ++pf.calls[kPfxCollisionPost];
  if (pf.ctx_sound < 0 && pf.sound && args && pf.exec_id && g.p8.tick > 0 && static_cast<const void*>(mods::arg<Z2SoundObjBase*>(args, 0)) == pf.exec_sound && mods::arg<u32>(args, 2) == 40) {
    const int e = pf.book.find(pfx::Kind::LizalfosOnLink, pf.exec_id, g.p8.tick - 1);
    if (e >= 0 && pf.book.orange_now(e) && !pf.book.r[e].sound) pf.ctx_sound = e;
  }
  if (pf.ctx_sound < 0) return;
  const int r = pf.ctx_sound;
  pf.ctx_sound = -1;  // one playback per parry contact
  Z2SoundHandlePool* h = ret ? *static_cast<Z2SoundHandlePool**>(ret) : nullptr;
  if (!h || !h->isSoundAttached()) {
    ++pf.book.c.no_sound;
    pfx_line("NATIVE_UNMODIFIED_SAFE", r, "NO_PLAYBACK_HANDLE");
    return;
  }
  const float base = (*h)->getAuxiliary().params_.mPitch;
  pfx_native_pitch(h, base * pfx::kPitch);
  pf.book.r[r].sound = true;
  pf.last_handle = h, pf.last_handle_tick = g.p8.tick, pf.last_pitch = base;
  pf.readback = h, pf.readback_tick = g.p8.tick, pf.readback_want = base * pfx::kPitch, pf.readback_owner = pf.book.r[r].kind == pfx::Kind::LizalfosOnLink ? pf.book.r[r].actor : 0u;
  ++pf.book.c.sounds;
  pfx_line("NATIVE_AUDIO_PITCHED", r, "SAME_SAMPLE_THIS_PLAYBACK", double(base), double(base * pfx::kPitch));
}
// ---------------------------------------------------------------- verdicts of the next tick
void pfx_settle(pfx::Kind k, std::uint32_t id, bool parry) noexcept {
  if (!pf.enabled || g.p8.tick == 0) return;
  const int r = pf.book.find(k, id, g.p8.tick - 1);
  if (r < 0 || pf.book.r[r].st != pfx::St::Preview) return;
  pf.book.settle(r, parry);
  if (parry) {
    pfx_line(pf.book.r[r].emitters ? "CONFIRMED_NATIVE_EMITTER_RECOLORED" : "NATIVE_UNMODIFIED_SAFE", r, pf.book.r[r].emitters ? "SAME_INSTANCE_HUE_ONLY" : "NO_NATIVE_EMITTER_CAPTURED",
             double(pf.captured_res[0]), double(pf.captured_res[1]));
    return;
  }
  // The preview judged a parry the verdict did not: colour back to the resource's, and the pitch back while the playback has not been rendered yet.
  if (pf.book.r[r].sound && pf.last_handle && pf.last_handle_tick == g.p8.tick && pf.last_handle->isSoundAttached()) {
    pfx_native_pitch(pf.last_handle, pf.last_pitch);
    ++pf.book.c.sound_reverted;
  }
  pfx_line("UNCONFIRMED", r, "PREVIEW_MISMATCH_REVERTED");
}
void pfx_liz_exec_begin(e_dn_class* a, std::uint32_t id) noexcept { pf.exec_id = id, pf.exec_sound = static_cast<const void*>(static_cast<Z2SoundObjBase*>(&a->sound.mSoundObjAnime)); }
void pfx_liz_exec_end() noexcept { pf.exec_id = 0, pf.exec_sound = nullptr; }
void pfx_maintenance() noexcept {
  pf.exec_id = 0, pf.exec_sound = nullptr, pf.ctx_pass = pf.ctx_mark = pf.ctx_sound = -1;  // between ticks no native call of this module is in progress
  if (!pf.enabled) return;
  const std::uint64_t ep = lg.posture.epoch();
  if (ep != pf.epoch) {  // scene / stage change: the particle manager owning those emitters is gone; nothing of it is touched again
    pf.epoch = ep;
    pf.readback = nullptr;
    pf.book.clear();
    for (auto& x : pf.em) x = PfxEmitter{};
    return;
  }
  if (pf.readback && g.p8.tick > pf.readback_tick) {  // FIX04: did anything rewrite that playback's pitch after a whole frame of audio? (Execution evidence, not hearing.)
    JAISoundHandle* h = pf.readback;
    pf.readback = nullptr;
    bool alive = pf.readback_owner == 0;  // Link's own sound object lives as long as this scene; a Lizalfos' only while its slot does (its Delete retires the slot first)
    for (const auto& s : lz.slots) alive = alive || (s.on && s.id == pf.readback_owner);
    if (alive && h->isSoundAttached()) {
      const float now = (*h)->getAuxiliary().params_.mPitch;
      ++(now == pf.readback_want ? pf.readback_ok : pf.readback_changed);
      pfx_line(now == pf.readback_want ? "AUDIO_PITCH_READBACK_OK" : "AUDIO_PITCH_REWRITTEN", -1, "ONE_TICK_LATER", double(pf.readback_want), double(now));
    }
  }
  pf.book.tick(g.p8.tick, [](int rec) {
    for (auto& x : pf.em)
      if (x.rec == rec) x = PfxEmitter{};  // the emitter stopped calling back for kKeepTicks: dead or reused (reuse resets its callback)
  });
}
inline constexpr const char* kPfxHookNames[kPfxHookCount] = {"dCcS::ProcAtTgHitmark", "dCcS::ProcAtTgHitmark", "dPa_control_c::setHitMark", "dPa_control_c::setHitMark", "dPa_control_c::set", "daAlink_c::setGuardSe", "daAlink_c::setGuardSe", "Z2SoundObjBase::startCollisionSE"};
inline constexpr const char* kPfxHookKinds[kPfxHookCount] = {"PRE", "POST", "PRE", "POST", "POST", "PRE", "POST", "POST"};
void pfx_install() noexcept {
  pf = PfxRuntime{};
  if (!g_pfx_cb) g_pfx_cb = new (g_pfx_cb_mem) PfxCallBack();
  const ModResult r[kPfxHookCount] = {mods::hook::add_pre<PfxProcHitmark>(on_pfx_hitmark_pre), mods::hook::add_post<PfxProcHitmark>(on_pfx_hitmark_post),
                                      mods::hook::add_pre<PfxSetHitMark>(on_pfx_mark_pre),     mods::hook::add_post<PfxSetHitMark>(on_pfx_mark_post),
                                      mods::hook::add_post<PfxParticleSet>(on_pfx_particle_post), mods::hook::add_pre<LinkGuardSe>(on_pfx_guard_se_pre),
                                      mods::hook::add_post<LinkGuardSe>(on_pfx_guard_se_post), mods::hook::add_post<PfxCollisionSe>(on_pfx_collision_se_post)};
  // The target's distance to a fixed exported anchor is the same under any ASLR base: offline it resolves to one symbol. No raw pointer reaches the log.
  const std::uintptr_t anchor = reinterpret_cast<std::uintptr_t>(&g_dComIfG_gameInfo);
  const std::uintptr_t tg[kPfxHookCount] = {reinterpret_cast<std::uintptr_t>(PfxProcHitmark::resolved_target()), reinterpret_cast<std::uintptr_t>(PfxProcHitmark::resolved_target()),
                                            reinterpret_cast<std::uintptr_t>(PfxSetHitMark::resolved_target()),  reinterpret_cast<std::uintptr_t>(PfxSetHitMark::resolved_target()),
                                            reinterpret_cast<std::uintptr_t>(PfxParticleSet::resolved_target()), reinterpret_cast<std::uintptr_t>(LinkGuardSe::resolved_target()),
                                            reinterpret_cast<std::uintptr_t>(LinkGuardSe::resolved_target()),    reinterpret_cast<std::uintptr_t>(PfxCollisionSe::resolved_target())};
  char line[kLogLineMax];
  for (unsigned i = 0; i < kPfxHookCount; ++i) {
    std::snprintf(line, sizeof(line), "PARRY_FX event=HOOK name=%s kind=%s result=%d resolved=%d anchor_delta=%lld version=%s", kPfxHookNames[i], kPfxHookKinds[i], int(r[i]), tg[i] ? 1 : 0,
                  tg[i] ? static_cast<long long>(tg[i]) - static_cast<long long>(anchor) : 0ll, pfx::kVersion);
    lnk_write(r[i] == MOD_OK ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  }
  // Fail-safe by function: without the decision hooks nothing is touched; a missing spark hook leaves the clash working, and the other way round. Never "enabled" over a hook that failed.
  const bool decide = lz.enabled && r[kPfxHitmarkPre] == MOD_OK && r[kPfxHitmarkPost] == MOD_OK;
  pf.spark = decide && r[kPfxMarkPre] == MOD_OK && r[kPfxMarkPost] == MOD_OK && r[kPfxParticlePost] == MOD_OK;
  pf.sound = decide && r[kPfxGuardPre] == MOD_OK && r[kPfxGuardPost] == MOD_OK && r[kPfxCollisionPost] == MOD_OK;
  pf.enabled = pf.spark || pf.sound;
  pf.epoch = lg.posture.epoch();
  const char* why = pf.spark && pf.sound ? "ALL_HOOKS_INSTALLED" : !lz.enabled ? "LIZALFOS_MODULE_DISABLED" : !decide ? "DECISION_HOOK_FAILED" : !pf.spark ? "SPARK_HOOK_FAILED_NATIVE_SPARK_KEPT" : "SOUND_HOOK_FAILED_NATIVE_CLASH_KEPT";
  std::snprintf(line, sizeof(line), "PARRY_FX event=INSTALL version=%s enabled=%d spark=%d sound=%d reason=%s hue=orange_hsv_h28 pitch=%.2f scope=CONFIRMED_PARRY_CONTACT_ONLY native_emitter=HITMARK_OF_THE_CONTACT runtime=HUMAN_RUNTIME_PENDING",
                pfx::kVersion, pf.enabled ? 1 : 0, pf.spark ? 1 : 0, pf.sound ? 1 : 0, why, double(pfx::kPitch));
  lnk_write(pf.enabled ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}
void pfx_shutdown() noexcept {
  // Emitters still calling back get their own callback again (only those seen alive in the last ticks: their pool memory is current).
  for (auto& x : pf.em)
    if (x.e && x.rec >= 0 && pf.book.r[x.rec].last_seen + 2 >= g.p8.tick && x.e->mpEmtrCallBack == g_pfx_cb) pfx_native_callback(x.e, x.prev);
  const auto& c = pf.book.c;
  const auto u = [](std::uint64_t v) { return static_cast<unsigned long long>(v); };
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "PARRY_FX_SUMMARY version=%s enabled=%d preview=%llu/%llu/%llu confirmed=%llu/%llu/%llu reverted=%llu/%llu/%llu expired=%llu duplicate=%llu full=%llu emitters=%llu emitter_overflow=%llu no_emitter=%llu "
                "sounds=%llu sound_reverted=%llu no_sound=%llu orphan_callbacks=%llu depth_errors=%llu res=%llu/%llu lines_dropped=%llu spark=%d sound=%d neutral_emitters=%llu pitch_readback_ok=%llu pitch_rewritten=%llu "
                "calls=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu telemetry=ROUTE_EXECUTION_ONLY_NOT_VISUAL_OR_AUDIBLE_PROOF runtime=HUMAN_RUNTIME_PENDING",
                pfx::kVersion, pf.enabled ? 1 : 0, u(c.preview[1]), u(c.preview[2]), u(c.preview[3]), u(c.confirmed[1]), u(c.confirmed[2]), u(c.confirmed[3]), u(c.reverted[1]), u(c.reverted[2]), u(c.reverted[3]),
                u(c.expired), u(c.duplicate), u(c.full), u(c.emitters), u(c.emitter_overflow), u(c.no_emitter), u(c.sounds), u(c.sound_reverted), u(c.no_sound), u(c.orphan_callbacks), u(pf.depth_errors),
                u(pf.captured_res[0]), u(pf.captured_res[1]), u(pf.dropped), pf.spark ? 1 : 0, pf.sound ? 1 : 0, u(c.neutral_emitters), u(pf.readback_ok), u(pf.readback_changed),
                u(pf.calls[0]), u(pf.calls[1]), u(pf.calls[2]), u(pf.calls[3]), u(pf.calls[4]), u(pf.calls[5]), u(pf.calls[6]), u(pf.calls[7]));
  lnk_write(LOG_LEVEL_INFO, line);
  pf.enabled = false;
}
#undef PFX_NOINLINE
