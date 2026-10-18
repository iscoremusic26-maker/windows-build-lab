# CP10.5 — implementation and evidence (Dusklight 40457c6)

## 1. Host facts used (read from source, not from the executable)
| Fact | Where |
|---|---|
| Both attack spheres are created and given their attack power in `create()`: `mSphs_at[i].Set(E_OC_n::at_sph_src)`, `SetStts`, then `SetAtAtp(1)` for `"E_OC"` or `SetAtAtp(2)` otherwise, for `i = 0` and `1`. The last statement of `create()` is `daE_OC_Execute(this)`: our first PRE sees both colliders complete. | `src/d/actor/d_a_e_oc.cpp` L2786-2802, L2829 |
| `mAtp` is written only by `cCcD_ObjAt::Set(src)` (called from `create()`) and `SetAtp()`. In `d_a_e_oc.cpp` nothing else writes it; `action()` only toggles the set bit (`OnAtSetBit` frames 14-22 of the attack, `OffAtSetBit` every action). So one write per generation is enough. | `c_cc_d.cpp` L699-703; `d_a_e_oc.cpp` L1455-1456, L2416-2417 |
| On a hit, `cCcS::CalcTgPlusDmg` / `dCcS::SetAtTgGObjInf` call `cCcD_Stts::PlusDmg(atp)` on the target's status, which keeps the **maximum** (`if (m_dmg >= dmg) return;`). Two spheres hitting in the same frame, or repeated hits, do not add. | `c_cc_s.cpp` L246-248; `d_cc_s.cpp` L573-577; `c_cc_d.cpp` L263-268 |
| Link: `procDamage` reads `mCcStts.GetDmg()` (a `u8`) in the non-shield branch and calls `setDamagePoint(dmg, ...)`, which computes `dmg * damageMagnification()`, rounds a fractional result **up**, and subtracts it with `dComIfGp_setItemLifeCount(-dmg)`; the meter then adds that to the life value. | `d_a_alink_damage.inc` L148-230, L695-714; `d_meter2.cpp` L620-640 |
| `damageMagnification()`: 1.0, or 1.5 when `FLG3_UNK_40000000` is set and not reset, times Dusklight's own `damageMultiplier` setting; Wolf x2; Zora armor x10 for fire/ice hits; `instantDeath` returns 9999. Magic armor converts the damage to rupees (x10) instead. | same file L148-176, L184-203 |
| A shield hit takes the `ChkTgShieldHit()` branch (`setGuardSe`, guard slip...) and **never** calls `setDamagePoint`. `dCcS::ChkShield()` decides it from the shield flags of the target and the attack's no-guard flag / `AtSpl`; it does not read `mAtp`. The attack's reaction to a block (`ChkAtHit` + `checkPlayerGuard()` -> `mOcState = 3`, playback speed -1) is also independent of `mAtp`: this is the pre-existing "shield stare", untouched. | `d_a_alink_damage.inc` L653-690; `d_cc_s.cpp` L195-212; `d_a_e_oc.cpp` L1465-1481 |
| `mAtInfo` / `at_power_get()` in `d_a_e_oc.cpp` and `d_cc_uty.cpp` concern attacks received **by the Bokoblin**, not its attack on Link. | `d_a_e_oc.cpp` L657-723 |

**Contract that follows (source reading, not game measurement):** for a base hit with no Link-side modifier, life lost = attack power: 1, 2, 3 for Blue, Red, Black, i.e. 1 : 2 : 3. Equal conditions are needed: Link-side factors above multiply the same atp, and a non-integer factor rounds per hit (x1.5: 2, 3, 5), so the ratio is only exact at integer factors. The unit of the subtraction is the game's life unit (`dComIfGs_getLife()`); that four of them make one heart is general knowledge of the game, not something this source read verified. Hence the human round compares hit against hit in the same conditions instead of trusting a number.

## 2. Mechanism (no new hook)
The existing single PRE/POST pair on `daE_OC_c::execute` (CP10.4) is the only entry point. In PRE, for a **fresh** actor generation that CP10.4 has just **accepted** (`plan.variant != None`, so first execute, exact native signature, valid registration), after the HP step, `damage_apply()` runs once:
1. Drop any ledger record of the same actor id (id reuse / new creation: nothing inherited).
2. Read `GetAtAtp()` of both spheres. `devf_damage::make_plan()` (`damage_policy.hpp`, pure) accepts only: a classified variant, name `E_OC`/`E_OC2`, Red/Black only on `E_OC2`, both spheres equal, and equal to the pristine native value (1 for `E_OC`, 2 for `E_OC2`). Otherwise reject, write nothing.
3. Reserve a ledger slot (64, bounded; full -> reject, write nothing).
4. Write `SetAtAtp(target)` on both spheres only if target != native (Blue native E_OC and Red native E_OC2 need no write), read both back; if either differs, put the exact native value back and reject.
5. Log `DAMAGE_PROFILE_APPLIED`.
No write happens in any later execute (the entry is classified), none in POST, none per hit; no census of actors; no Link, global damage or collision hook; shared resource tables (`E_OC_n::at_sph_src`) are never written (only the per-actor collider objects).

Identity precedence, 64-slot registry, id+address reuse protection, first-execute signature, HP/regeneration, Black admission and private texture are the unchanged CP10.4 / FIX01 code (`variant_policy.hpp`, `variant_registration.hpp` are included from `CP10_4_VARIANT_IDENTITY_HP`, not copied).

## 3. Unload / release safety
- Actor gone (room exit, despawn, destruction, id reuse): `regen_poll` / the stale path emits `DAMAGE_PROFILE_RELEASED` and drops the record. No collider is read or written (the actor may be dead memory).
- `mod_shutdown`: for each record, `fopAcM_SearchByID(id)` must return the **same** address with `init_state != 3` and profile 510; only then are the two atp values read. They are restored to the exact native value **only if both still equal the value this module wrote**; if anything else changed them (another mod, the game) they are left alone and the record is released (`DAMAGE_PROFILE_RELEASED reason=COLLIDER_CHANGED_BY_OTHERS_NOT_OWNED`). If the host has already destroyed the actors, the lookup fails and nothing is touched.
- Not-ours cases: a record exists only for generations where this module wrote or verified the values; pristine-signature failures never create one.

## 4. Logs (bounded, never per frame or per hit)
`DEVF_CP10_5_DAMAGE event=DAMAGE_PROFILE_APPLIED | DAMAGE_PROFILE_REJECTED | DAMAGE_PROFILE_RELEASED | DAMAGE_PROFILE_RESTORED` with actor id, generation, source (`NATIVE_E_OC`, `NATIVE_E_OC2`, `DEVF_NORMAL`, `DEVF_BLACK`), classified variant, encounter id, native attack power, `collider0/1_before/after`, `target_base`, `written=YES|NO`, reason, `link_damage=NOT_MEASURED`. At most 400 lines per session. The CP10.4 `DEVF_CP10_4_VARIANT`, the CP10.3 `REGEN_*` and `HP_APPLIED`/`BLACK_TEXTURE_*` lines are unchanged.

## 5. Evidence classes
| Class | What | Covers |
|---|---|---|
| Source reading | section 1 | the contract between attack power and subtracted life |
| Lab (`tests/damage_lab.cpp`, 22 pure cases, also ASan/UBSan locally) | policy + the mirror of the first-execute flow with a fake actor | which values are decided, when, how often, rejection, ownership, bounded ledger. The three `MODEL_OF_SOURCE_*` cases transcribe the host formulas (max rule, magnification, round-up) to document them; they are **not** the game |
| Regression | CP10.4 variant lab (30), runtime 66, policy 26, texture 20, editor 42/71/110/25, C# <-> C++ parity | no change in identity, HP, regeneration, registration, texture, schema, editor |
| PE/ABI audit | same import set, one hook on `daE_OC_c::execute`, services, 7 negatives | no new host dependency (`GetAtAtp`/`SetAtAtp` are header inlines) |
| Windows CI + independent download | MSVC build, package, SHA256 | the bytes delivered |
| **Not available** | hearts lost by Link in the game | human round |

## 6. Not verified
Hearts lost by Link per variant in the real game; whether other mods that hook `daE_OC_c::execute`, change Bokoblin colliders or Link's damage interact; real scenes with native `E_OC`/`E_OC2` outside F_SP121; the exact numeric size of one life unit in hearts (not read here). A different build of the game or other mods writing `mAtp` after create would make this module's one-time write moot (they would also be detected as not pristine only at creation).
