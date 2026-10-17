# CP10.4 — implementation and evidence (Dusklight 40457c6)

## Host facts used
- `src/d/actor/d_a_e_oc.cpp` `create()` (L2704-2830): `mName = "E_OC"` when `(current.angle.x >> 8) & 0xFF` is 0 or 0xFF, else `"E_OC2"`; then `field_0x560 = health = 40` (E_OC) or `220`; sets `current.angle.x = 0`, and its last statement is `daE_OC_Execute(this)`: the **first execute runs inside create()** (the CP03 "initializing" marker: `init_state` 0/1, `current.angle.x == 0`, `fpcM_IsCreating`).
- `field_0x560` is written only there in this file. Attack power (`SetAtAtp(1)` E_OC, `(2)` E_OC2, L2788-2802) is untouched here; a future damage CP must not stack a multiplier on the native E_OC2 power.
- Time base, pause and death states: as CP10.3 (`game_clock.h` 1/30 s tick; `fopAc_Execute` skips paused actors; `enum OC_ACTIONS` L20-37).

## Mechanism (one PRE/POST pair on `daE_OC_c::execute`, as 0.5.0)
1. PRE reads `health` before any write, registers the tick pair in the 64-slot registry (id + address; a first-execute marker on a known id drops the old state).
2. A fresh entry is classified **once** by `make_plan()` (`variant_policy.hpp`), with the precedence: Black registration (exact record match, then the unchanged CP10.2/FIX01 admission writes the configured HP and binds the private texture) > Normal registration (identity BLUE) > native `E_OC` (BLUE, writes 220) > native `E_OC2` (RED, writes 330). The registrations are two exact-match tables built from the JSON (`variant_registration.hpp`): the same float32/record/room/layer/stage rule as before, so a nearby but different actor is never admitted. An encounter registered as Black whose admission fails is rejected, never RED/BLUE.
3. `make_plan` also verifies the native creation signature (`field_0x560` equal to 40/220 by name, current health equal to it, first execute observed). Anything else is rejected without writing. `field_0x560` is never written; the new maximum lives only in the registry entry, never confused with the Black 220 signature.
4. POST (after native damage/death) calls `step()`: the CP10.3 integer accumulator (30 units = 1 HP), caps at the variant maximum, nothing for health ≤ 0 or death/water/fall/demo actions. 3 HP/s = 1 HP every 10 ticks; 2 = every 15; 4 = 2 per 15.
5. Hooks are installed always; with no file, an empty file, only Normal, only Black or a mix, natives are classified by name. An invalid JSON still fails the whole mod (unchanged contract).

## Logs (bounded, no per-frame output)
`DEVF_CP10_4_VARIANT event=VARIANT_ADMITTED | HP_INITIALIZED | VARIANT_REJECTED | VARIANT_RELEASED` with actor id, generation, source (NATIVE_E_OC / NATIVE_E_OC2 / DEVF_NORMAL / DEVF_BLACK), encounter id, classified variant, native base HP, observed HP, hp before/after, effective max, rate, reason. The CP10.3 `REGEN_*` lines now carry `kind=BLUE|RED|BLACK`; `HP_APPLIED` and `BLACK_TEXTURE_*` are unchanged.

## Tests (`tests/variant_lab.cpp`, 30 pure cases; also ASan/UBSan locally)
Identity per row of the table; Normal-registered E_OC2 is BLUE; Black 440 / 150; failed Black never RED; exact-match only (1 ulp, other room/layer/stage/yaw/range code/params/profile); natives work without a file, with an empty file, only Normal, only Black, mixed; all rejection paths write nothing; one initial write across 200 later ticks; id reuse/re-entry start unclassified; 2/3/4 HP per 30 ticks, fractions, 1000 s zero drift, FPS-pattern independence, pause, PRE/POST pairing, damage without cooldown, no revive, death/water/fall/demo states, caps 220/330/440/150, never lowered, s16, independent simultaneous actors, bounded registry and telemetry. All CP10.2/FIX01 labs and editor self-tests retained.

## Not verified
Gameplay of any of this; the native E_OC2 outside DEVF (RED) in real scenes; other mods that also hook `daE_OC_c::execute` or change Bokoblin HP; generic host code that might read `field_0x560` of an enemy (not found for E_OC in this file).
