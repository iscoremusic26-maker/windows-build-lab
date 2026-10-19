#pragma once
// CP14 pure policy (no game headers): the first RED abilities. The mod and the lab run THIS code.
//
// IDENTITY (unchanged, CP10.4 make_plan): native E_OC2 without a DEVF registration -> RED (330 HP, 3 HP/s, base attack 2 from CP10.5/CP10.6).
// An exact DEVF Black registration stays BLACK and an exact DEVF Normal registration stays BLUE (precedence untouched).
//
// ABILITY 1 - LIFESTEAL. Each EFFECTIVE hit of a Red on Link (a hit that really lowered Link's life, see "ATTRIBUTION") heals that Red by 25 % of
// its MAXIMUM HP (330 -> 82.5), capped at the maximum. Fraction policy: a per-generation milli-HP accumulator. Each hit adds max_hp x 250 milli-HP;
// the integer part is healed, the remainder (< 1 HP) is carried to the next hit of the same Red. 330: 82, 83, 82, 83 ... (exactly 82.5 on average).
// The cap only clips the HP written; the remainder is still carried (deterministic, independent of the current HP). A new generation starts at 0.
//
// ABILITY 2 - GROUP REGENERATION WINDOW. The same effective hit opens (or renews) a window of kWindowTicks = 120 of that Red's own simulation ticks
// (4.000 s at 30 Hz, the CP10.3 time base): tick rule as the Black fury (the POST that applies the hit is tick 0 and sets 120; each later POST
// decrements first; the POST where it reaches 0 closes it), so ticks 0..119 are bonus ticks. During the window:
//   * the Red regenerates x2 (3 -> 6 HP/s);
//   * every OTHER live classified Bokoblin within the existing CP10.6 companion range of that Red (650 XZ / 250 Y, same room and stage, executed
//     recently; devf_dyn::eligible, unchanged) regenerates x2 of its OWN rate (Blue 2 -> 4, Red 3 -> 6, Black 4 -> 8) on each tick it is in range.
//     Leaving the range ends it on the next tick; coming back while the window is open gives x2 again for the remaining ticks.
//   * x2 is a boolean: several Reds, a Red that is itself in another Red's range, or a renewal never give more than x2.
// A renewal while open resets the window to 120 (no stacking). A terminal Red (dead, dying, falling, water death, demo) closes it at once and
// discards any pending heal. With no companion in range the window still opens and benefits only the Red itself.
//
// ATTRIBUTION (engine path, Dusklight 40457c6; see IMPLEMENTATION.md). One management pass = duskExecute (mod_update) -> actor executes by line
// (Link line 5, E_OC line 7, METER2 line 12) -> draws (dScnPly_Draw -> dCcS::Move: at/tg hits of this pass, hit flags kept until the next Move).
// Link applies a Move-N hit in its execute of pass N+1 (daAlink_c::checkDamageAction): it takes the FIRST of its 3 tg cylinders with a hit, credits
// the attacker recorded on THAT cylinder, ignores it entirely while mDamageTimer != 0, takes the shield branch if the cylinder has the tg-shield bit,
// otherwise calls setDamagePoint(dmg) -> dComIfGp_setItemLifeCount(-dmg) and mDamageTimer = invincible time (30). METER2 then moves the pending
// count into the saved life.
//   CLAIM   (mod_update of pass N+1, after Move N, before Link executes): the cylinder Link will use holds a non-shield hit whose hitting collider
//           is one of the two attack spheres of a live classified Red; that Red's sphere reports the reverse hit on one of Link's cylinders, also
//           non-shield; Link's damage status holds dmg > 0; mDamageTimer <= 1 (Link decrements a running timer BEFORE checkDamageAction, so 1 ends in
//           this very execute); Link alive; no positive pending life count.
//   CONFIRM (mod_update of pass N+2): same Link object; mDamageTimer restarted (now > the claimed value: setDamagePoint ran); effective life
//           life+pending dropped by >= 1.
// Rejected (never credited): shield hits (both bits), hits during invincibility, magic-armour hits (rupees, not life), hits whose collider is not a
// Red's sphere, and any claim without a real life drop one pass later. Residual (documented, not detectable without a new hook): floor/electric
// damage of Link in the SAME 1/30 s pass as a Red hit that Link then did not process.
#include "../../CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
#include "../../CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/variant_eyes.hpp"
#include <array>
#include <cstdint>
namespace devf_red {
using devf_variant::Variant;
constexpr unsigned kLifestealPm=250;                        // 25 % of the maximum HP
constexpr unsigned kWindowTicks=120;                        // 4 s of the Red's own simulation ticks
constexpr unsigned kRegenMultiplier=2;                      // the only multiplier this ability can produce
constexpr unsigned kMaxPending=4;                           // confirmed hits waiting for the Red's next POST (normally 0 or 1)
static_assert(kWindowTicks==4*devf_variant::kTicksPerSecond,"4.000 s at the CP10.3 time base");
static_assert(devf_variant::kMaxHpRed==330&&devf_variant::kRateRed==3,"Red 330 HP / 3 HP/s (CP10.4)");
// ---------------- ability 1 ----------------
struct Heal {int heal=0;std::int16_t new_health=0;unsigned whole=0,remainder_milli=0;bool capped=false;};
// `acc_milli` is the per-generation remainder (0..999). Only called for a live, non-terminal Red.
inline Heal lifesteal(std::uint16_t& acc_milli,std::int16_t health,std::int16_t max_hp) noexcept {
 Heal h;h.new_health=health;
 const std::uint32_t total=std::uint32_t(acc_milli)+std::uint32_t(max_hp>0?max_hp:0)*kLifestealPm;
 h.whole=total/1000u;acc_milli=static_cast<std::uint16_t>(total%1000u);h.remainder_milli=acc_milli;
 int target=int(health)+int(h.whole);
 if(target>max_hp){target=max_hp;h.capped=true;}
 if(target<health)target=health;                           // an HP already above the maximum (another mod) is never lowered
 h.heal=target-int(health);h.new_health=static_cast<std::int16_t>(target);
 return h;
}
// ---------------- ability 2: per-Red state, side table indexed like the CP10.6 ledger, stamped with id+generation ----------------
constexpr unsigned kLogsPerRed=48;                         // telemetry lines per actor generation (the session cap is separate)
struct State {std::uint32_t id=0,generation=0;unsigned window=0,pending=0,hits=0,heals=0,logs=0;std::uint16_t acc_milli=0;int healed_total=0;bool bonus_prev=false;std::uint32_t bonus_from=0;};   // bonus_*: x2 transition logging of ANY classified Bokoblin in this slot
template<std::size_t N> inline State& state_for(std::array<State,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 State& s=table[index<N?index:0];if(s.id!=id||s.generation!=generation){s=State{};s.id=id;s.generation=generation;}return s;
}
template<std::size_t N> inline const State* state_if(const std::array<State,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 if(index>=N)return nullptr;const State& s=table[index];return s.id==id&&s.generation==generation?&s:nullptr;
}
enum class Ev:std::uint8_t {None,Heal,WindowStart,WindowRenew,WindowEnd,PendingDiscarded};
struct PostOut {Ev window_ev=Ev::None;const char* reason="NONE";Heal heals[kMaxPending];unsigned applied=0,discarded=0;std::int16_t hp_before=0,hp_after=0;};
// The Red's own POST (after the native execute, before regeneration). Writes nothing to the game: the caller writes `hp_after` if it changed.
inline PostOut red_post(State& s,std::int16_t health,std::int16_t max_hp,int action) noexcept {
 PostOut o;o.hp_before=o.hp_after=health;
 const bool terminal=health<=0||devf_variant::blocks_regen(action);
 if(terminal){
  if(s.pending){o.discarded=s.pending;s.pending=0;}
  if(s.window){s.window=0;o.window_ev=Ev::WindowEnd;o.reason="TERMINAL_STATE_DEATH_FALL_WATER_OR_DEMO";}
  else if(o.discarded){o.window_ev=Ev::PendingDiscarded;o.reason="RED_TERMINAL_HIT_NOT_HEALED";}
  return o;
 }
 if(s.pending){
  const unsigned n=s.pending<kMaxPending?s.pending:kMaxPending;s.pending=0;
  for(unsigned i=0;i<n;++i){o.heals[i]=lifesteal(s.acc_milli,o.hp_after,max_hp);o.hp_after=o.heals[i].new_health;s.healed_total+=o.heals[i].heal;++s.heals;}
  o.applied=n;
  o.window_ev=s.window?Ev::WindowRenew:Ev::WindowStart;o.reason=s.window?"EFFECTIVE_HIT_RENEWS_TO_120_NO_STACK":"EFFECTIVE_HIT_ON_LINK_OPENS_4S_WINDOW";
  s.window=kWindowTicks;
  return o;
 }
 if(s.window&&--s.window==0){o.window_ev=Ev::WindowEnd;o.reason="EXPIRED_120_SIM_TICKS";}
 return o;
}
constexpr bool window_open(const State& s) noexcept {return s.window>0;}
// The Red that gives `self` the x2 bonus this tick (nullptr = none). `self` is any classified Bokoblin's CP10.6 entry (snapshot of this tick
// already written by post_update). A Red's own window is NOT found here (eligible() excludes self): the caller adds it.
template<std::size_t N> inline const devf_dyn::Entry* ally_source(const devf_dyn::Ledger& L,const std::array<State,N>& table,const devf_dyn::Entry& self,std::uint32_t seq,unsigned window) noexcept {
 if(!self.snap||self.health<=0||devf_variant::blocks_regen(self.action))return nullptr;   // only live, valid Bokoblins receive the bonus
 for(std::size_t i=0;i<L.entries.size();++i){
  const auto& c=L.entries[i];
  if(!c.used||c.variant!=Variant::Red)continue;
  const State* s=state_if(table,i,c.id,c.generation);
  if(!s||!window_open(*s))continue;
  if(devf_dyn::eligible(self,c,seq,window))return &c;
 }
 return nullptr;
}
// Regeneration multiplier of one Bokoblin for this tick: 2 if its own Red window is open or a Red with an open window is in range, else 1.
constexpr unsigned regen_multiplier(bool own_window,bool ally_in_range) noexcept {return own_window||ally_in_range?kRegenMultiplier:1u;}
// ---------------- attribution: claim (pass N+1) and confirmation (pass N+2) ----------------
// What the mod reads from the engine at mod_update, as plain values (pointers are compared, never dereferenced here).
struct Cyl {bool raw_hit=false,no_actor=false,shield=false;const void* hit_obj=nullptr;const void* self=nullptr;};
struct LinkSnap {
 bool present=false,canary=false;const void* link=nullptr;std::uint32_t id=0;
 int timer=0,life=0;float pending=0.0f;unsigned dmg=0,cyl_mask=0;
 Cyl cyl[3];
 float effective() const noexcept {return float(life)+pending;}
};
struct RedSnap {bool live=false;const void* sph[2]{};bool at_hit[2]{},at_shield[2]{};const void* at_obj[2]{};};
enum class Why:std::uint8_t {Claimed,NoLink,LayoutCanary,NoTgHit,CylinderMask,NotARedCollider,RedNotLive,LinkShield,RedSideMismatch,RedSideShield,NoDamageStatus,LinkInvulnerable,LinkLifeZero,PositivePending,
 Confirmed,LinkChanged,NotProcessedByLink,NoLifeLoss};
inline const char* why_text(Why w) noexcept {
 switch(w){case Why::Claimed:return "CLAIMED_RED_SPHERE_HIT_LINK_TG_CYLINDER_NON_SHIELD";case Why::NoLink:return "NO_LINK";case Why::LayoutCanary:return "LINK_LAYOUT_CANARY_FAILED";
  case Why::NoTgHit:return "NO_TG_HIT";case Why::CylinderMask:return "LINK_CYLINDER_MASK_SET_ENGINE_SKIPS_CYLINDERS";case Why::NotARedCollider:return "HIT_BY_A_NON_RED_COLLIDER";
  case Why::RedNotLive:return "RED_NOT_LIVE_OR_REPLACED";case Why::LinkShield:return "BLOCKED_BY_SHIELD_LINK_SIDE";case Why::RedSideMismatch:return "RED_SPHERE_HAS_NO_REVERSE_HIT_ON_LINK";
  case Why::RedSideShield:return "BLOCKED_BY_SHIELD_RED_SIDE";case Why::NoDamageStatus:return "LINK_DAMAGE_STATUS_ZERO";case Why::LinkInvulnerable:return "LINK_INVULNERABLE_TIMER_RUNNING";
  case Why::LinkLifeZero:return "LINK_LIFE_ZERO";case Why::PositivePending:return "POSITIVE_PENDING_LIFE_COUNT_AMBIGUOUS";case Why::Confirmed:return "CONFIRMED_LINK_LIFE_DROPPED_AND_INVULNERABILITY_RESTARTED";
  case Why::LinkChanged:return "LINK_OBJECT_CHANGED";case Why::NotProcessedByLink:return "LINK_DID_NOT_PROCESS_THE_HIT_TIMER_NOT_RESTARTED";case Why::NoLifeLoss:return "NO_REAL_LIFE_LOSS";default:return "NONE";}
}
// The cylinder checkDamageAction will use: the first one with a hit (mask must be 0, as in the engine's condition). -1 = none.
inline int selected_cylinder(const LinkSnap& L) noexcept {
 if(L.cyl_mask)return -1;
 for(int i=0;i<3;++i)if(L.cyl[i].raw_hit&&(L.cyl[i].no_actor||L.cyl[i].hit_obj))return i;
 return -1;
}
// Which sphere (0/1) of `r` hit the selected cylinder; -1 = not this Red.
inline int sphere_of(const LinkSnap& L,int cyl,const RedSnap& r) noexcept {
 if(cyl<0)return -1;const void* h=L.cyl[cyl].hit_obj;if(!h)return -1;
 for(int k=0;k<2;++k)if(r.sph[k]==h)return k;
 return -1;
}
struct Claim {bool armed=false;std::uint32_t red_id=0,red_generation=0,link_id=0;const void* link=nullptr;float e0=0;int cyl=-1,sphere=-1,timer0=0;unsigned dmg=0;int life0=0;};
// Full claim decision once the mod has found the Red whose sphere is the selected cylinder's hit object (`r`; `found` = such a Red exists).
inline Why make_claim(const LinkSnap& L,bool found,const RedSnap& r,Claim& out) noexcept {
 out=Claim{};
 if(!L.present)return Why::NoLink;
 if(!L.canary)return Why::LayoutCanary;
 bool any=false;for(const auto& c:L.cyl)any=any||c.raw_hit;
 if(!any)return Why::NoTgHit;
 if(L.cyl_mask)return Why::CylinderMask;
 const int cyl=selected_cylinder(L);
 if(cyl<0)return Why::NoTgHit;
 if(!found)return Why::NotARedCollider;
 if(!r.live)return Why::RedNotLive;
 const int k=sphere_of(L,cyl,r);
 if(k<0)return Why::NotARedCollider;
 if(L.cyl[cyl].shield)return Why::LinkShield;
 bool reverse=false,reverse_shield=false;
 for(int s=0;s<2;++s){
  if(!r.at_hit[s])continue;
  for(const auto& c:L.cyl)if(c.self&&r.at_obj[s]==c.self){reverse=true;reverse_shield=reverse_shield||r.at_shield[s];}
 }
 if(!reverse)return Why::RedSideMismatch;
 if(reverse_shield)return Why::RedSideShield;
 if(L.dmg==0)return Why::NoDamageStatus;
 if(L.timer>1)return Why::LinkInvulnerable;                 // 1: Link's damageTimerCount reaches 0 just before checkDamageAction
 if(L.life<=0)return Why::LinkLifeZero;
 if(L.pending>0.0f)return Why::PositivePending;
 out.armed=true;out.link=L.link;out.link_id=L.id;out.e0=L.effective();out.cyl=cyl;out.sphere=k;out.dmg=L.dmg;out.life0=L.life;out.timer0=L.timer;
 return Why::Claimed;
}
constexpr bool loggable(Why w) noexcept {return w==Why::Claimed||w==Why::LinkShield||w==Why::RedSideShield||w==Why::RedSideMismatch||w==Why::NoDamageStatus||w==Why::RedNotLive||w==Why::LinkLifeZero||w==Why::PositivePending;}
inline Why confirm(const Claim& c,const LinkSnap& now) noexcept {
 if(!now.present||now.link!=c.link||now.id!=c.link_id)return Why::LinkChanged;
 if(now.timer<=c.timer0)return Why::NotProcessedByLink;    // setDamagePoint restarts the timer (30); otherwise it stays or counts down
 if(!(now.effective()<=c.e0-0.5f))return Why::NoLifeLoss;
 return Why::Confirmed;
}
// ---------------- gold eyes (Red, during its window) ----------------
// Same compressed-domain recolour as the CP12 eyes, tinted with the aura's gold; the Red uses the E_OC2 atlas whose eyes are green (the same
// detection the human-validated Black red eyes use). No fallback hue rule: if the green rule does not see the eyes, the gold eyes are refused.
inline constexpr devf_eye::Params kGoldEyes{70,170,25,12,32,devf_stk::kRedGoldColour.r,devf_stk::kRedGoldColour.g,devf_stk::kRedGoldColour.b};
struct GoldChoice {const devf_eye::Params* params=nullptr;std::size_t endpoints=0;const char* why="NONE";};
inline GoldChoice choose_gold(const std::uint8_t* atlas,std::size_t bytes) noexcept {
 GoldChoice c;c.endpoints=devf_eyes12::count_endpoints(atlas,bytes,kGoldEyes);
 if(c.endpoints<devf_eyes12::kMinEyeEndpoints){c.why="NO_GREEN_EYE_ENDPOINTS";return c;}
 c.params=&kGoldEyes;return c;
}
} // namespace devf_red
