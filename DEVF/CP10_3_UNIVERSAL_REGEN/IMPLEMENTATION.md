# CP10.3 — implementation and evidence (Dusklight 40457c6)

## What the host source says (read, not assumed)
- `include/d/actor/d_a_e_oc.h`, `src/d/actor/d_a_e_oc.cpp`: one class `daE_OC_c` serves resources `E_OC` and `E_OC2`; `create()` (L2776-2778) sets `field_0x560 = health = 40` for `mName=="E_OC"`, else `220`. `field_0x560` is not written anywhere else in the file, so it is the native maximum. Damage is applied inside `execute()` → `action()` → `damage_check()` (L637-790, `cc_at_check`); death sets `health = 0` and `setActionMode(E_OC_ACTION_DEATH, …)`; `executeDeath` ends in `fopAcM_delete`.
- Actions (`enum OC_ACTIONS`, L20-37): WAIT 0, WALK 1, TALK 2, FIND 3, ATTACK 4, DAMAGE 5, BIG_DAMAGE 6, WATCH 7, SOUND_WATCH 8, **DEATH 9, WATER_DEATH 10**, DEMO_MASTER 11, DEMO_CHILD 12, **FALL_DEAD 13, FALL 14**, FIND_STAY 15, MOVE_OUT 16. `blocks_regen()` mirrors the irreversible/scripted ones (9-14).
- Time: `src/dusk/game_clock.h` `kSimPeriod = 1/30 s`; `src/m_Do/m_Do_main.cpp` runs `fapGm_Execute()` once per simulation tick; `src/f_op/f_op_actor.cpp` (L323-335) skips an actor while the game is paused, a menu/event pauses it or its stop status is set. Hence **one completed `daE_OC_c::execute` = one tick = 1/30 s of game time**, and a paused/loading/frozen actor executes zero times. Render FPS and interpolation only change how often ticks are *presented*, not how many ticks an actor executes per game second.

## Mechanism
- Same hook target as FIX01 (`daE_OC_c::execute`), now a **PRE + POST pair** (host `HookService`, POST installed first). No `drawFast`, no XF, no actor scan.
- PRE (`regen_begin`): register one pending tick for exactly this actor (bounded registry, 64 slots, keyed by actor id + address; an id re-used by another address, or a new native creation marker, drops the old state; a second PRE before POST is not counted). The existing exact-record Black admission and one-shot `HP_APPLIED` run unchanged; the admission only records `black_hp` in the actor's entry.
- POST (`after_execute`): after the native execute — so native damage/death of this tick is already decided and a lethal hit is never pre-empted — classify once and then `step()`.
- Classification (`classify`): name `E_OC` → max 40; `E_OC2` → max 220; anything else is not touched. If `field_0x560` differs from the native value (another mod changed the maximum) the actor is **rejected**, not guessed. Max for an actor admitted by the exact DEVF Black registration is its configured `hp` (150), never `field_0x560` (220); Black is never inferred from colour, position or profile alone.
- `step()`: integer accumulator in 1/30 HP units: `acc += rate` per tick; every 30 becomes 1 HP, clamped to the maximum; `health <= 0` or a blocking action heals nothing and drops the fraction; at/above the maximum nothing is written and no fraction is banked. 2 HP/s = 1 HP every 15 ticks; 4 HP/s = 2 HP every 15 ticks; exact over any whole number of game seconds (tested over 1000 s, floor-exact at every one of 9999 ticks).
- Release: `mod_update` checks at most 8 tracked actors per update (round-robin `fopAcM_SearchByID`): gone / address changed / init_state 3 → release with a summary log. Registry full → new actors are not tracked (`REGEN_REJECTED REGISTRY_FULL`, once). Shutdown: flag off, host removes the hooks, no HP is restored artificially. The FIX01 private texture, its restore and its retention rules are untouched.
- Hooks are now installed always (not only when Black is configured); the Black admission/texture code only acts on exact registered Black encounters, so a file without Black behaves like native + regeneration.

## Time source / precision
Source: the simulation tick (1/30 s) as above; **no wall-clock, no fallback**. Precision: exact in ticks. Caveats: with interpolation off the host runs one tick per rendered frame by its own design (the same for every native timer); the host `set_sim_rate` scales game time, and the rate stays HP per *game* second.

## Logs (`DEVF_CP10_3_REGEN`, never per frame)
`REGEN_TRACKED` (once per actor, kind/max/rate/source), `REGEN_APPLIED` (first heal, then each time 10 more HP are healed, max 32 per actor; hp before/after, `sim_ticks`, `sim_seconds`, `healed_total`), `REGEN_RELEASED`, `REGEN_REJECTED` (+reason). Black keeps `DEVF_CP10_2_BLACK event=HP_APPLIED` and `DEVF_CP10_2_BLACK_TEXTURE event=BLACK_TEXTURE_*`.

## Tests
`tests/regen_lab.cpp` (23 cases, pure): classification 40/220/Black150, exact 2 and 4 HP/s, zero drift, fractions, FPS-pattern independence, pause, PRE/POST pair semantics, damage without cooldown, no revive/never heal in death states, caps (also Black 150 although `field_0x560` is 220), never above max/never lowered/no banking, s16, fail-closed classification, independent simultaneous actors, id reuse and re-entry, bounded registry, bounded telemetry, regeneration without any Black. Also run under ASan/UBSan locally. Retained: CP08/CP09/CP10/CP10.1/CP10.2 labs, FIX01 texture lab (bit-exact goldens), editor self-tests, PE/ABI/imports audit, independent download.

## Not verified
Actual in-game rates, interaction with other mods that change Bokoblin HP or hook `daE_OC_c::execute`, behaviour with interpolation/time-scale combinations, E_OC variants outside the two native names.
