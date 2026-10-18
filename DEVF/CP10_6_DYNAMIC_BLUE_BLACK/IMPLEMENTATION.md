# CP10.6 — implementation and evidence (Dusklight 40457c6)

## 1. Host facts used
| Fact | Where |
|---|---|
| Both attack spheres get `SetAtAtp(1/2)` in `create()`, then the first execute runs inside `create()`; `mAtp` is only rewritten by `Set(src)`/`SetAtp()` (CP10.5, unchanged). | `d_a_e_oc.cpp` L2786-2829, `c_cc_d.cpp` L699-703 |
| The collision phase keeps the **maximum** attack power of the frame and Link subtracts it times its own magnification: two spheres or repeated hits do not add (CP10.5 section 1). | `c_cc_d.cpp` L263-268, `d_a_alink_damage.inc` L148-230 |
| The Bokoblin loses health **inside its own execute**: `action()` -> `damage_check()` -> `cc_at_check(this,&mAtInfo)` does `i_enemy->health -= i_AtInfo->mAttackPower` (clamped to 0). Bombs and the head-jump set `health = 0`. The only other writes of `health` in `d_a_e_oc.cpp` are `create()` (40/220) and the slingshot/iron-ball/`AT_TYPE_40` branches in the same `damage_check`. | `d_cc_uty.cpp` L442-448; `d_a_e_oc.cpp` L643, L702-736, L2776-2778 |
| `fopAc_Execute` skips paused/stopped actors: one completed PRE+POST pair is one 1/30 s tick of that actor (CP10.3). | `f_op_actor.cpp` ~L323-335, `game_clock.h` |
| `plyr_srch_min_radius 400`, `battle_participation_radius 1200`, `teammate_attention_radius 2000` are HIO values of other behaviours; 650 is a separate, fixed, moderate radius chosen by the requester. | `d_a_e_oc.cpp` HIO ctor |
| `current.pos`, `fopAcM_GetRoomNo()`, `mActionMode`, `health` are plain members/inline accessors (no new import). | `d_a_e_oc.h`, `f_op_actor_mng.h` |

## 2. Mechanism (still ONE hook target, one PRE and one POST)
- **PRE** (`on_execute`): unchanged CP10.4/CP10.5 flow (Black admission and HP write, identity, native Blue/Red HP write, base attack power). **Last**, it stores `pre_health = a->health` in the actor's record: the baseline therefore already contains every HP initialisation write (220 -> 440, 220 -> 150, 40 -> 220).
- **POST** (`after_execute`): reads `health` right after the NATIVE execute, calls `dynamic_post()` (snapshot, Blue group, Black fury, collider write), and **only then** the CP10.3 regeneration `step()`. A heal can therefore never be seen as a hit, and a 1 HP hit is seen even if regeneration would give it back in the same tick.
- `dynamic_policy.hpp` is pure and templated over a two-method collider adapter (`get(i)`, `set(i,v)`). The mod and `tests/dynamic_damage_lab.cpp` run the same `admit`, `post_update`, `reconcile` and `restore_plan`.
- `devf_dyn::Ledger` (64 records, id + address + registry generation) replaces the CP10.5 ledger. Every CP10.4-**classified** generation gets a record (so it can be somebody's companion); only a pristine native pair is `owned`. A record is dropped by the same paths as before (`regen_poll`, id reuse / new creation) and a new generation never inherits anything.
- **Single writer** `reconcile()`: changes the pair only when the effective target changes. It first checks both spheres still equal the value this module wrote last (`applied`); if not (another mod or the game), it writes nothing, drops ownership and logs `DYNAMIC_REJECTED` once. After writing it reads both back; on a mismatch it restores `applied` and drops ownership. No writes per tick or per hit (the lab asserts zero writes over 2000 steady ticks).

## 3. Blue companion rule
A candidate is another ledger record that: is classified (CP10.4 accepted), has a snapshot, `health > 0`, is not in a terminal state (`blocks_regen`: death, water death, fall, fall dead, demos), has the same room and the same stage tag (FNV of the start-stage name), has executed within the last `2*tracked+2` completed pairs, and lies within **XZ euclidean <= 650** (compared squared, no sqrt) and **|dY| <= 250** (both inclusive). Blue, Red and Black, native or DEVF, all count; the Blue itself never does.
- **Position source:** the candidate's `current.pos` as recorded at its own last POST (values only). A companion is never dereferenced through the record. Initial coordinates and encounter tickets are not used. Proximity is only an ability of an already classified actor, never used to decide which Bokoblin a JSON entry is.
- **Liveness proof:** a record only changes when that actor really executes. Destroyed or stopped actors stop refreshing their stamp and fall out of the window within about two ticks, without waiting for any poll. In addition, when a Blue is about to turn the bonus **on**, the candidate is verified authoritatively (`fopAcM_SearchByID(id)` returns the very same address, `init_state != 3`, profile 510); this costs one search per transition, never one per tick. A candidate that fails it is ignored until it executes again.
- **Cost:** a scan of at most 64 integer records per Blue per tick, no `fpcM_Search`, no actor census. Not every Bokoblin of the game is guaranteed to be tracked: only those the registry holds (64 slots, created at their first execute). A Bokoblin that the registry refused (full) or that CP10.4 rejected is not a companion; this is a documented limitation, not a claim that every native Bokoblin is covered.
- **Latency:** snapshots are as old as the companion's previous POST, so a change of distance is seen on the same or the next tick (<= 1/30 s). No artificial delay.
- **No line of sight check, no hysteresis (decisions):** nothing in the request or the source requires visibility, and a wall check would need a collision query per pair per tick. Hysteresis would make the exact 650 limit untestable; the cost of flicker at the border is two integer writes per crossing and logs are capped (32 events per actor, 400 per session).

## 4. Black fury rule
- Trigger: `post_health < pre_health`, `post_health > 0`, action not terminal, in the POST of a completed pair. One trigger per POST however many contacts landed. A hit that ends with `health <= 0` starts nothing; a terminal state during fury ends it (the 3 is restored while the actor is still executing).
- Timing: the POST that observes the loss is tick 0 and sets 300. Each later POST decrements first; the POST that reaches 0 restores 3 (tick 300). The 6 is effective in the collision phase of ticks 0..299 = exactly 300 ticks. A hit in the POST that would expire renews instead, so there is no gap. Only completed pairs count: pauses, loading and event stops execute nothing; render FPS is irrelevant.
- **Author of the hit is not verified.** A drop of HP inside the native execute is taken as damage whether it comes from Link's sword, arrows, bombs, a boomerang, the environment or another mod. No hook on Link or on the global damage routines was added to tell them apart. A change of HP between the previous POST and the next PRE (another mod) is never seen as a hit: the PRE baseline is re-read.

## 5. Unload (`mod_shutdown`)
For each owned record: the actor is looked up by id and must be the same address, `init_state != 3`, profile 510; only then the two spheres are read, and they are restored to the **native** value (E_OC 1, E_OC2 2) **only if both still equal `applied`** (the base or the dynamic value written last: 1, 2, 3 or 6). Gone, replaced, destroyed, not owned or third-party-modified: nothing is read or written. Red (2 = native) and native E_OC at 1 need no write. `DAMAGE_PROFILE_RESTORED` / `DAMAGE_PROFILE_RELEASED` lines say which.

## 6. Logs (bounded)
`DEVF_CP10_6_DYNAMIC event=BLUE_GROUP_ON | BLUE_GROUP_OFF | BLACK_FURY_START | BLACK_FURY_REFRESH | BLACK_FURY_END | DYNAMIC_REJECTED | DYNAMIC_RELEASED` with actor id, generation, source, variant, encounter, `atp_from/atp_to`, both colliders as read back, companion id/variant/XZ/Y distance, room, `hp_pre`, `hp_post_native`, `fury_ticks_remaining`, `duration_ticks=300`, reason, `link_damage=NOT_MEASURED`. The `DEVF_CP10_5_DAMAGE`, `DEVF_CP10_4_VARIANT`, `DEVF_CP10_3_REGEN`, `HP_APPLIED` and `BLACK_TEXTURE_*` lines are unchanged.

## 7. Evidence classes
| Class | What | Covers |
|---|---|---|
| Source reading | section 1 | where HP changes, the max rule, tick basis |
| Lab (`dynamic_damage_lab`, 28 cases, also ASan/UBSan locally) | the real templates against fake actors in the mod's PRE/native/POST order | radius edges (649/650/651, 249/250/251, 3-4-5 diagonal, Chebyshev trap), exclusions, companion lifecycle, stale records, fury 0/1/299/300/301, renewal, hit on the expiry tick, HP init/regen/stagger/lethal, pause, unpaired PRE/POST, id reuse, ownership failures, partial writes, bounded ledger, the unload matrix |
| Regression (unchanged) | CP10.5 damage lab 22, CP10.4 variant lab 30, runtime 66, policy 26, texture 20, editor 42/71/110/25, C# <-> C++ parity | identity, HP, regeneration, base profile, texture, schema, editor |
| PE/ABI audit + Windows CI + independent download | MSVC build, import set, one hook, package bytes, SHA256 | the delivered bytes |
| **Not available** | hearts lost by Link, real-game feel, other mods | human round |

## 8. Not verified
The real effect on Link's hearts; the real-game distances between the placed Bokoblins (they may be farther apart than 650); native Bokoblins outside the registry; other mods hooking `daE_OC_c::execute` or writing colliders/HP; shield behaviour beyond "the shield branch does not read the attack power" (CP10.5).
