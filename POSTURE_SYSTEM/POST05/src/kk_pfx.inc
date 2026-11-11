// LIGHT FADES TWICE 0.13.0 CHILFOS01: the two previews the parry FX pass asks about a Chilfos contact. Production only (the world laboratory judges
// the same verdicts through kk_decide / kk_link_contact). Read-only: no RNG is consumed and no state changes here.
// (A) Link's Shield Attack on a Chilfos AT sphere: an open, unresolved impact window and Link's pass of the NEXT tick inside the shared window.
bool kk_preview_link_parry(daE_KK_c* a, daAlink_c* k, const cCcD_Obj* at) noexcept {
  KkSlot* s = kk_find(a);
  if (!s || !s->admitted || s->dead || !k || s->imp_serial == s->imp_resolved) return false;
  bool own = at == static_cast<const cCcD_Obj*>(&a->mSph);
  for (int i = 0; i < 6 && !own; ++i) own = at == static_cast<const cCcD_Obj*>(&a->mSpheres[i]);
  if (!own) return false;
  const post08::LinkView p = pfx_link_pass(k, g.p8.tick + 1);
  return post08::Parry::in_window(p);
}
// (C) Link's sword on the Chilfos body cylinder presented as a shield: the verdict kk_decide will give this swing (cached, or the same rule).
bool kk_preview_enemy_parry(daE_KK_c* a, daAlink_c* k) noexcept {
  KkSlot* s = kk_find(a);
  if (!s || !s->admitted || s->dead || !k) return false;
  const kk::V had = s->sv.of(kz.swing_serial);
  if (had != kk::V::None) return had == kk::V::EnemyParry;
  return !s->brk.active() && s->shield_on && s->def.presenting(g.p8.tick + 1, kz.swing_serial) && kk_frontal(a, k) && kk_spear(a) && kk_link_has_rebound(k);
}
