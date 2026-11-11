#if defined(_MSC_VER)
#define PFX_NOINLINE __declspec(noinline)
#else
#define PFX_NOINLINE __attribute__((noinline))
#endif
// LIGHT FADES TWICE 0.12.5 FIX05 (P1-3 of FIX03, repaired by FIX04) production adapter (included once inside mod.cpp's anonymous namespace, after liz_adapter.inc).
// The orange spark is the game's OWN spark instance(s) of the parry contact; the high clash is the game's OWN collision SE playback of that contact.
// Native writes (three scoped writers, audited): the instance colours of the captured emitters (emtr->mPrmClr/mEnvClr and their particles'
// mPrmClr/mEnvClr) inside their own executeAfter; the emitter callback pointer of those captured emitters (chained: the native one keeps running);
// JAISoundParamsMove::movePitch of the returned playback handle. No JPAResource, texture, sample, sound table or other emitter is touched.
// FIX05: a private copy of one resource with its colour tables already orange (see parry_fx_core.hpp). Plain storage: the native constructors are neither run nor imported; nothing shared is written.
struct PfxClone {
  const JPAResource* src = nullptr;
  const void* src_shape = nullptr;
  alignas(JPAResource) unsigned char res[sizeof(JPAResource)];
  alignas(JPABaseShape) unsigned char bsp[sizeof(JPABaseShape)];
  alignas(JPAChildShape) unsigned char csp[sizeof(JPAChildShape)];
  alignas(JPAChildShapeData) unsigned char cdata[sizeof(JPAChildShapeData)];
  GXColor prm[pfx::kClrMax], env[pfx::kClrMax];
};
enum PfxOrigin : unsigned char { kPfxOriginPass = 0, kPfxOriginExec, kPfxOriginSibling, kPfxOriginRetro };
struct PfxEmitter {
  JPABaseEmitter* e = nullptr;
  JPAEmitterCallBack* prev = nullptr;
  int rec = -1;
  GXColor prm{}, env{};
  std::uint16_t res = 0;
  bool colored = false, child_static = false;
  unsigned char origin = kPfxOriginPass;
  pfx::Risk risk = pfx::Risk::None;
  PfxClone* clone = nullptr;
};
struct PfxPending {  // an emitter of a mark in this very pass, before anybody knows whether it belongs to a parry. Never read after the pass.
  JPABaseEmitter* e = nullptr;
  std::uint32_t actor = 0;
  std::uint16_t res = 0;
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
  std::uint32_t ctx_cand = 0, ctx_mark_cand = 0;  // FIX05: actor of a mark that no parry claims yet (Link's own At can be processed before the parry contact of the same pass)
  unsigned char ctx_origin = kPfxOriginPass;
  PfxPending pending[pfx::kPending];
  unsigned pending_n = 0;
  std::uint64_t pending_tick = ~0ull;
  PfxClone clones[pfx::kClones];
  std::uint16_t seen_res[pfx::kSeenRes] = {};
  std::uint32_t exec_id = 0;   // admitted Lizalfos whose Execute is running (its own setHitMark / def_se_set)
  const void* exec_sound = nullptr;  // the Z2SoundObjBase of its Z2Creature: the `this` of the clash SE
  // CHILFOS01: whose Execute it is. Lizalfos: setHitMark(2, NULL) and def_se_set map 40. Chilfos (damage_check guard branch): setHitMark(2, this) and startCollisionSE map 7.
  pfx::Kind exec_kind = pfx::Kind::LizalfosOnLink;
  std::uint32_t exec_map = 40;
  const void* exec_mark_actor = nullptr;
  JAISoundHandle* last_handle = nullptr;
  std::uint64_t last_handle_tick = ~0ull;
  float last_pitch = 1.0f;
  std::uint64_t epoch = 0, captured_res[2] = {}, depth_errors = 0;
  unsigned lines = 0;
  std::uint64_t dropped = 0;
} pf;
void pfx_line(const char* event, int rec, const char* detail, double a = 0, double b = 0) noexcept {
  if (pf.lines >= 1500) {
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
// Writer 4 (FIX05): the work data of the calc in progress points at this emitter's orange copy of its resource, until the next emitter's calc rewrites it. The shared resource is never written.
PFX_NOINLINE void pfx_native_work(JPABaseEmitter* e, JPAResource* r) noexcept { e->mpEmtrMgr->pWd->mpRes = r; }
// Writer 5 (FIX05): the child pass of THIS emitter registers the child shape's colours just before this callback: the same values, in orange. Same multiplication as JPARegistChildPrmEnv.
struct PfxTev { GXColor prm, env; };
PFX_NOINLINE PfxTev pfx_child_tev_colors(JPABaseEmitter* e) noexcept {  // pure: no store to any native object
  GXColor p, n;
  JPAChildShape* c = e->pRes->getCsp();
  c->getPrmClr(&p);
  c->getEnvClr(&n);
  const pfx::Rgb po = pfx::orange(pfx::Rgb{p.r, p.g, p.b}), no = pfx::orange(pfx::Rgb{n.r, n.g, n.b});
  const auto mul = [](unsigned a, unsigned b) { return static_cast<std::uint8_t>((a * (b + 1)) >> 8); };
  const GXColor pg = e->mGlobalPrmClr, ng = e->mGlobalEnvClr;
  return PfxTev{GXColor{mul(po.r, pg.r), mul(po.g, pg.g), mul(po.b, pg.b), mul(p.a, pg.a)}, GXColor{mul(no.r, ng.r), mul(no.g, ng.g), mul(no.b, ng.b), n.a}};
}
PFX_NOINLINE void pfx_native_child_tev(JPABaseEmitter* e) noexcept {
  const PfxTev c = pfx_child_tev_colors(e);
  GXSetTevColor(GX_TEVREG0, c.prm);
  GXSetTevColor(GX_TEVREG1, c.env);
}
// Writer 6 (FIX05): the environment flash this very mark just asked for, not yet applied: the game's own warm palette instead of the green-blue one.
PFX_NOINLINE bool pfx_native_flash() noexcept {
  if ((g_env_light.eflight.mState != 1 && g_env_light.eflight.mState != 4) || g_env_light.eflight.mLightType != 0) return false;
  g_env_light.eflight.mLightType = 1;
  return true;
}
pfx::ResFlags pfx_read_flags(JPABaseEmitter* e) noexcept {
  pfx::ResFlags f;
  JPAResource* r = e->pRes;
  JPABaseShape* b = r->getBsp();
  f.glbl = b->isGlblClrAnm(), f.prm_anm = b->isPrmAnm(), f.env_anm = b->isEnvAnm();
  f.tev_sel = b->getTevColorArgSel();
  f.entries = static_cast<unsigned>(b->getClrAnmMaxFrm()) + 1u;
  if (JPAChildShape* c = r->getCsp()) f.child = true, f.child_color_inherit = c->isColorInherited(), f.child_alpha_out = c->isAlphaOutOn(), f.child_alpha_inherit = c->isAlphaInherited();
  JPAResourceManager* m = e->mpEmtrMgr->pResMgrAry[e->mResMgrID];
  const u16 tex = r->getTexIdx(b->getTexIdx());  // the base texture: getTexIdx(n) reads a table that only texture-animated resources have
  if (m && tex < m->texRegNum && m->pTexAry[tex] && m->pTexAry[tex]->mpData) f.tex_fmt = m->pTexAry[tex]->mpData->mResTIMG.format;
  return f;
}
PfxClone* pfx_clone_of(JPABaseEmitter* e, const pfx::ResFlags& f) noexcept {
  JPAResource* r = e->pRes;
  JPABaseShape* b = r->getBsp();
  if (!pfx::needs_clone(f) || f.entries > pfx::kClrMax) return nullptr;
  PfxClone* free_clone = nullptr;
  for (auto& x : pf.clones) {
    if (x.src == r && x.src_shape == static_cast<const void*>(b->pBsd)) return &x;
    if (!x.src && !free_clone) free_clone = &x;
  }
  if (!free_clone) return nullptr;
  PfxClone& x = *free_clone;
  std::memcpy(x.res, r, sizeof(JPAResource));
  std::memcpy(x.bsp, b, sizeof(JPABaseShape));
  auto* rc = reinterpret_cast<JPAResource*>(x.res);
  auto* bc = reinterpret_cast<JPABaseShape*>(x.bsp);
  rc->pBsp = bc;
  const auto tint = [](GXColor c) { const pfx::Rgb o = pfx::orange(pfx::Rgb{c.r, c.g, c.b}); return GXColor{o.r, o.g, o.b, c.a}; };
  if (b->mpPrmClrAnmTbl) {
    for (unsigned i = 0; i < f.entries; ++i) x.prm[i] = tint(b->mpPrmClrAnmTbl[i]);
    bc->mpPrmClrAnmTbl = x.prm;
  }
  if (b->mpEnvClrAnmTbl) {
    for (unsigned i = 0; i < f.entries; ++i) x.env[i] = tint(b->mpEnvClrAnmTbl[i]);
    bc->mpEnvClrAnmTbl = x.env;
  }
  if (JPAChildShape* c = r->getCsp()) {
    std::memcpy(x.csp, c, sizeof(JPAChildShape));
    std::memcpy(x.cdata, c->mpData, sizeof(JPAChildShapeData));
    auto* cd = reinterpret_cast<JPAChildShapeData*>(x.cdata);
    cd->mPrmClr = tint(cd->mPrmClr), cd->mEnvClr = tint(cd->mEnvClr);
    auto* cc = reinterpret_cast<JPAChildShape*>(x.csp);
    cc->mpData = cd;
    rc->pCsp = cc;
  }
  x.src = r, x.src_shape = static_cast<const void*>(b->pBsd);
  ++pf.book.c.clones;
  return &x;
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
    if (orange) {  // FIX05: what the previous calc_p left on the living particles: execution evidence that the per-particle route is orange too
      for (JPANode<JPABaseParticle>* n = e->mAlivePtclBase.getFirst(); n; n = n->getNext()) {
        const JPABaseParticle* p = n->getObject();
        if (p->mAge > 0 && p->mPrmClr.b > p->mPrmClr.r && p->mPrmClr.b > p->mPrmClr.g) ++pf.book.c.blue_after_calc;
      }
    }
    if (orange || s->colored) {
      pfx_native_colors(e, s->prm, s->env, orange);
      s->colored = orange;
    }
    if (orange && s->clone) pfx_native_work(e, reinterpret_cast<JPAResource*>(s->clone->res));
  }
  // JPAResource::calc runs executeAfter BEFORE calc_p, and calc_p may rewrite a particle's colour from the resource's key frames; the emitter draw
  // callback runs after every calc and before the particles' colours are read (JPARegistPrmAlpha*, batch ctx): the last word on each drawn frame.
  void draw(JPABaseEmitter* e) override {
    PfxEmitter* s = pfx_slot(e);
    if (!s) return;
    if (s->prev) s->prev->draw(e);
    if (pf.enabled && pf.book.orange_now(s->rec)) {
      pfx_native_colors(e, s->prm, s->env, true);
      if (s->child_static && e->checkStatus(0x80)) {
        pfx_native_child_tev(e);
        ++pf.book.c.child_static_draws;
      }
    }
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
bool pfx_attach(JPABaseEmitter* e, int rec, std::uint16_t res, unsigned char origin = kPfxOriginPass) noexcept {
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
  const pfx::ResFlags fl = pfx_read_flags(e);
  free_slot->clone = pfx_clone_of(e, fl);
  free_slot->child_static = pfx::child_static(fl);
  free_slot->origin = origin;
  free_slot->risk = pfx::risk_of(fl, free_slot->clone != nullptr);
  if (pfx::per_particle_colors(fl)) ++pf.book.c.per_particle_emitters;
  if (free_slot->risk != pfx::Risk::None) ++pf.book.c.risk;
  {  // once per resource: the native colour route that will be used for it, the evidence for or against a blue texture
    bool seen = false;
    for (auto v : pf.seen_res) seen = seen || v == res;
    if (!seen) {
      for (auto& v : pf.seen_res)
        if (!v) {
          v = res;
          break;
        }
      char rd[160];
      std::snprintf(rd, sizeof(rd), "res=0x%04x_glbl=%d_prm_anm=%d_env_anm=%d_child=%d_child_inherit=%d_child_alpha_out=%d_tev=%u_texfmt=%u_entries=%u_clone=%d_risk=%s", res, fl.glbl ? 1 : 0, fl.prm_anm ? 1 : 0, fl.env_anm ? 1 : 0, fl.child ? 1 : 0, fl.child_color_inherit ? 1 : 0, fl.child_alpha_out ? 1 : 0, fl.tev_sel, fl.tex_fmt, fl.entries, free_slot->clone ? 1 : 0, pfx::name(free_slot->risk));
      pfx_line("RESOURCE", rec, rd);
    }
  }
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
  pf.ctx_pass = -1, pf.ctx_cand = 0;
  if (!mods::arg<bool>(args, 1) || !mods::arg<bool>(args, 2)) return HOOK_CONTINUE;
  const bool shield_contact = mods::arg<bool>(args, 12);
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
      parry = shield_contact && at == static_cast<const cCcD_Obj*>(&d->at_sph) && pfx_preview_liz_link(d, k);
    } else if (fopAcM_GetName(aa) == fpcNm_E_OC_e) {
      kind = pfx::Kind::LinkOnEoc;
      parry = shield_contact && pfx_preview_oc_link(static_cast<daE_OC_c*>(aa), k);
    } else if (fopAcM_GetName(aa) == fpcNm_E_KK_e) {  // CHILFOS01
      kind = pfx::Kind::LinkOnChilfos;
      parry = shield_contact && kk_preview_link_parry(static_cast<daE_KK_c*>(aa), k, at);
    }
  } else if (aa == static_cast<fopAc_ac_c*>(k) && ta && (fopAcM_GetName(ta) == fpcNm_E_DN_e || fopAcM_GetName(ta) == fpcNm_E_OC_e || fopAcM_GetName(ta) == fpcNm_E_KK_e)) {
    other = ta;
    if (fopAcM_GetName(ta) == fpcNm_E_DN_e && pfx_link_sword(k, at)) {
      auto* d = reinterpret_cast<e_dn_class*>(ta);
      kind = pfx::Kind::LizalfosOnLink;
      parry = shield_contact && tg == static_cast<const cCcD_Obj*>(&d->tate_sph) && pfx_preview_liz_enemy(d, k);
    } else if (fopAcM_GetName(ta) == fpcNm_E_KK_e && pfx_link_sword(k, at)) {  // CHILFOS01
      auto* c = static_cast<daE_KK_c*>(ta);
      kind = pfx::Kind::ChilfosOnLink;
      parry = shield_contact && tg == static_cast<const cCcD_Obj*>(&c->mCyl) && kk_preview_enemy_parry(c, k);
    }
  }
  if (!other) return HOOK_CONTINUE;
  const auto id = static_cast<std::uint32_t>(fopAcM_GetID(other));
  if (parry) {
    pf.ctx_pass = pf.book.open(kind, id, g.p8.tick);
    pf.ctx_origin = kPfxOriginPass;
    if (pf.ctx_pass >= 0 && pf.pending_tick == g.p8.tick) {  // marks of the same two actors already drawn earlier in this very pass: they are this clash too
      for (unsigned i = 0; i < pf.pending_n; ++i) {
        if (pf.pending[i].actor != id || !pf.pending[i].e) continue;
        if (pfx_attach(pf.pending[i].e, pf.ctx_pass, pf.pending[i].res, kPfxOriginRetro)) ++pf.book.c.retro;
        pf.pending[i].e = nullptr;
      }
      if (pfx_native_flash()) ++pf.book.c.flashes;
    }
    return HOOK_CONTINUE;
  }
  // FIX05: any other contact between the two actors of a parry that just happened is part of that clash; if there is none yet, it may still turn out to be, later in this pass.
  const int ex = pf.book.exchange(id, g.p8.tick);
  if (ex >= 0) {
    pf.ctx_pass = ex;
    pf.ctx_origin = kPfxOriginSibling;
    ++pf.book.c.siblings;
  } else {
    pf.ctx_cand = id;
  }
  return HOOK_CONTINUE;
}
void on_pfx_hitmark_post(ModContext*, void*, void*, void*) noexcept {
  ++pf.calls[kPfxHitmarkPost];
  pf.ctx_pass = -1, pf.ctx_cand = 0;
}
// setHitMark: inside the parry pass, or E_DN's own shield spark (setHitMark(2, NULL, ...)) in the Execute that follows the same contact.
HookAction on_pfx_mark_pre(ModContext*, void* args, void*, void*) noexcept {
  ++pf.calls[kPfxMarkPre];
  if (!pf.enabled || !pf.spark || !args) return HOOK_CONTINUE;
  pf.ctx_mark = -1, pf.ctx_mark_cand = 0;
  const unsigned type = mods::arg<u16>(args, 1);
  if (pf.ctx_pass >= 0 && (pf.ctx_origin == kPfxOriginPass || pfx::hitng_mark(type))) {  // a sibling's damage spark (HITOK/HITCR) is not the blue one: it keeps its own colour
    pf.ctx_mark = pf.ctx_pass;
  } else if (pf.ctx_cand && pfx::hitng_mark(type)) {
    pf.ctx_mark_cand = pf.ctx_cand;
    ++pf.book.c.block_marks;  // counted down again if a parry claims it in this same pass
  } else if (pf.exec_id && type == 2 && static_cast<const void*>(mods::arg<fopAc_ac_c*>(args, 2)) == pf.exec_mark_actor && g.p8.tick > 0) {
    pf.ctx_mark = pf.book.find(pf.exec_kind, pf.exec_id, g.p8.tick - 1);
    pf.ctx_origin = kPfxOriginExec;
  }
  return HOOK_CONTINUE;
}
void on_pfx_mark_post(ModContext*, void*, void*, void*) noexcept {
  ++pf.calls[kPfxMarkPost];
  if (pf.ctx_mark >= 0 && pf.book.orange_now(pf.ctx_mark) && pfx_native_flash()) ++pf.book.c.flashes;
  pf.ctx_mark = -1, pf.ctx_mark_cand = 0;
}
void on_pfx_particle_post(ModContext*, void* args, void* ret, void*) noexcept {
  ++pf.calls[kPfxParticlePost];
  if ((pf.ctx_mark < 0 && !pf.ctx_mark_cand) || !ret || !args) return;  // every other particle of the game: two comparisons, nothing else
  JPABaseEmitter* e = *static_cast<JPABaseEmitter**>(ret);
  const std::uint16_t res = mods::arg<u16>(args, 2);
  if (!e) {
    if (pf.ctx_mark >= 0) ++pf.book.c.no_emitter;
    return;
  }
  if (pf.ctx_mark < 0) {  // not claimed by any parry yet: just remembered until the end of this pass, nothing about it is touched
    if (pf.pending_tick != g.p8.tick) pf.pending_tick = g.p8.tick, pf.pending_n = 0;
    if (pf.pending_n < pfx::kPending) pf.pending[pf.pending_n++] = PfxPending{e, pf.ctx_mark_cand, res};
    return;
  }
  if (pfx_attach(e, pf.ctx_mark, res, pf.ctx_origin)) {
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
  if (r < 0) r = pf.book.find(pfx::Kind::LinkOnChilfos, id, g.p8.tick - 1);
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
  if (pf.ctx_sound < 0 && pf.sound && args && pf.exec_id && g.p8.tick > 0 && static_cast<const void*>(mods::arg<Z2SoundObjBase*>(args, 0)) == pf.exec_sound && mods::arg<u32>(args, 2) == pf.exec_map) {
    const int e = pf.book.find(pf.exec_kind, pf.exec_id, g.p8.tick - 1);
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
    {  // FIX05: every native part of this clash, where it came from and whether its whole colour route is orange. A technical statement: only the eye can confirm it.
      char d[320];
      int n = 0;
      bool any_risk = false;
      static constexpr const char* kOrigins[] = {"PASS", "EXEC", "SIBLING", "RETRO"};
      for (const auto& x : pf.em) {
        if (x.rec != r || !x.e || n >= int(sizeof(d)) - 56) continue;
        n += std::snprintf(d + n, sizeof(d) - unsigned(n), "%s%04x:%s:%s", n ? "," : "", x.res, kOrigins[x.origin], x.risk == pfx::Risk::None ? "ORANGE_CONFIRMED_TECHNICALLY" : pfx::name(x.risk));
        any_risk = any_risk || x.risk != pfx::Risk::None;
      }
      pfx_line(any_risk ? "PARRY_SPARK_BLUE_RISK" : "FX_PAIR", r, n ? d : "NO_NATIVE_EMITTER_CAPTURED");
    }
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
void pfx_liz_exec_begin(e_dn_class* a, std::uint32_t id) noexcept {
  pf.exec_id = id, pf.exec_sound = static_cast<const void*>(static_cast<Z2SoundObjBase*>(&a->sound.mSoundObjAnime));
  pf.exec_kind = pfx::Kind::LizalfosOnLink, pf.exec_map = 40, pf.exec_mark_actor = nullptr;
}
void pfx_kk_exec_begin(daE_KK_c* a, std::uint32_t id) noexcept {  // CHILFOS01
  pf.exec_id = id, pf.exec_sound = static_cast<const void*>(static_cast<Z2SoundObjBase*>(&a->mCreatureSound.mSoundObjAnime));
  pf.exec_kind = pfx::Kind::ChilfosOnLink, pf.exec_map = 7, pf.exec_mark_actor = static_cast<const void*>(static_cast<fopAc_ac_c*>(a));
}
void pfx_liz_exec_end() noexcept { pf.exec_id = 0, pf.exec_sound = nullptr, pf.exec_kind = pfx::Kind::LizalfosOnLink, pf.exec_map = 40, pf.exec_mark_actor = nullptr; }
void pfx_maintenance() noexcept {
  pf.exec_id = 0, pf.exec_sound = nullptr, pf.ctx_pass = pf.ctx_mark = pf.ctx_sound = -1;  // between ticks no native call of this module is in progress
  pf.ctx_cand = pf.ctx_mark_cand = 0;
  pf.pending_n = 0;  // the pass that remembered them is over: those pointers are never read again
  if (!pf.enabled) return;
  const std::uint64_t ep = lg.posture.epoch();
  if (ep != pf.epoch) {  // scene / stage change: the particle manager owning those emitters is gone; nothing of it is touched again
    pf.epoch = ep;
    pf.readback = nullptr;
    pf.book.clear();
    for (auto& x : pf.em) x = PfxEmitter{};
    for (auto& x : pf.clones) x.src = nullptr, x.src_shape = nullptr;  // a scene's resources go with it
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
  std::snprintf(line, sizeof(line), "PARRY_FX_SUMMARY part=FIX05 version=%s clones=%llu per_particle=%llu child_static_draws=%llu blue_after_calc=%llu siblings=%llu retro=%llu flashes=%llu risk=%llu block_spark_original=%llu lines_dropped=%llu telemetry=ROUTE_EXECUTION_ONLY_NOT_VISUAL_OR_AUDIBLE_PROOF runtime=HUMAN_RUNTIME_PENDING",
                pfx::kVersion, u(c.clones), u(c.per_particle_emitters), u(c.child_static_draws), u(c.blue_after_calc), u(c.siblings), u(c.retro), u(c.flashes), u(c.risk), u(c.block_marks >= c.retro ? c.block_marks - c.retro : 0), u(pf.dropped));
  lnk_write(LOG_LEVEL_INFO, line);
  pf.enabled = false;
}
#undef PFX_NOINLINE
