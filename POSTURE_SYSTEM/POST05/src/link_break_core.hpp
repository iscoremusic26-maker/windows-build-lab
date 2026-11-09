#pragma once
// LIGHT FADES TWICE 0.12.2: +50% to Link's real damage while his posture break really runs. GENERAL: no enemy kind or id, value logic only.
// [S d_a_alink_damage.inc] setDamagePoint(i_dmgAmount, ...) is where every Tg contact of checkDamageAction (line 714, mCcStts.GetDmg()) commits; the same function also receives poly/floor (561), freeze (48),
// scene (d_a_alink.cpp:4709), landing, throw (field_0x318c from setThrowDamage: no attacker known) and daPy_py_c::setPlayerDamage. Only a contact that can be attributed gets the bonus; the argument is raised BEFORE the
// native code runs, so damageMagnification, armor, the pending life count and the invulnerability timer all stay native. The life itself is never rewritten.
// [S d_a_alink_damage.inc] direct dComIfGp_setItemLifeCount callers (e_db, e_yh, e_hb, e_yd bites, a mod's own chip) do not pass here: they stay native and are not counted.
#include <cstdint>
namespace lnkdmg {
inline constexpr const char* kVersion = "0.12.2";
enum class Why : std::uint8_t { None = 0, Applied, NoDamage, LinkState, ArmorNoHp, Unattributed, Environment, Blocked, NotHostile, Duplicate, Count };
inline const char* name(Why w) noexcept {
  static constexpr const char* n[] = {"NONE", "APPLIED", "SKIP_NO_DAMAGE", "SKIP_LINK_STATE", "SKIP_ARMOR_NO_HP", "SKIP_UNATTRIBUTED", "SKIP_ENVIRONMENT", "SKIP_BLOCKED", "SKIP_NOT_HOSTILE", "SKIP_DUPLICATE"};
  return unsigned(w) < unsigned(Why::Count) ? n[unsigned(w)] : "?";
}
struct In {
  bool episode = false;      // the real, saturated posture break of Link (LNK05/06 episode, penalty window); never the number 100
  bool human = false, alive = false, event = false, armor_no_hp = false;
  bool in_action = false;    // inside exactly one checkDamageAction, not re-entered
  bool poly_clear = false;   // that call's checkPolyDamage returned 0: not the floor branch
  bool has_record = false;   // a Tg record on one of Link's cylinders in this call
  bool blocked = false;      // ... and the engine marked it as a shield hit
  bool attacker_known = false, attacker_enemy = false, parent_enemy = false;
  int amount = 0;
  std::uint64_t tick = 0, last_applied_tick = ~0ull;
};
struct Out {
  Why why = Why::None;
  int extra = 0;
};
inline constexpr int extra_of(int base) noexcept { return base > 0 ? (base + 1) / 2 : 0; }  // 2->3 4->6 6->9 8->12; odd bases round up in Link's disfavour: 1->2 3->5
inline Out decide(const In& i) noexcept {
  Out o;
  if (!i.episode) return o;                 // outside the episode nothing is even reported
  if (i.amount <= 0) return o.why = Why::NoDamage, o;
  if (!i.human || !i.alive || i.event) return o.why = Why::LinkState, o;
  if (i.armor_no_hp) return o.why = Why::ArmorNoHp, o;
  if (!i.in_action) return o.why = Why::Unattributed, o;
  if (!i.poly_clear) return o.why = Why::Environment, o;
  if (!i.has_record) return o.why = Why::Unattributed, o;
  if (i.blocked) return o.why = Why::Blocked, o;
  if (!i.attacker_known) return o.why = Why::Unattributed, o;
  if (!i.attacker_enemy && !i.parent_enemy) return o.why = Why::NotHostile, o;
  if (i.tick == i.last_applied_tick) return o.why = Why::Duplicate, o;  // one commit per native call and tick: x1.5 never compounds
  o.why = Why::Applied, o.extra = extra_of(i.amount);
  return o;
}
struct Counters {
  std::uint64_t episodes = 0, applied = 0, base_units = 0, extra_units = 0, why[unsigned(Why::Count)] = {};
};
}  // namespace lnkdmg
