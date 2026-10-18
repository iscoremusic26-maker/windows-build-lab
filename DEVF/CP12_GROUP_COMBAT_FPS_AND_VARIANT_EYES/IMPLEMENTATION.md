# CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES — Expansion 0.9.0 (base: 0.8.9 / CP11, source d60e534)

Evidence classes:
- **SOURCE:** exact Dusklight 40457c6 and borealis 4ac5e70 source, plus this mod.
- **LOG:** the human 0.8.9 log `DEVF/evidence/CP12/dusklight-20261018-100753.log`, branch `evidence/cp12-human-log-20261018-100753`, git blob 7989d18, 11 058 lines. It was read from GitHub as the user asked; user paths are redacted here.
- **LAB:** CI-run tests.
- **LOCAL_MICROBENCH / STUB:** the real mod code with fake engine types, gcc, WSL2.
- **WINDOWS_CI:** build and audit only.
- **HUMAN:** pending.

**No FPS or frame time was measured by me.** The game cannot run here. The log has no timing data except Dusklight's own `fapGm_Execute frame=N` marker every 300 simulation ticks.

## A. Phase A — what the 0.8.9 log shows (LOG + SOURCE), what it does not
| Hypothesis | Evidence | Impact | Change in 0.9.0 | Validation | Limitation |
|---|---|---|---|---|---|
| **H1 Particles are the bottleneck** | SOURCE: `JPAResource::draw_particle_batch` draws a whole emitter in ONE batched `GXBegin/GXEnd` when there is no particle callback / projection (cross shape 4 supported, 8 vertices per particle). LOG `RESOURCE_PROBE user_work=0x00000002` (no 0x100 callback, no 0x20/0x40 environment colour), `authored_rate_step=0`, keys: rate 2.0→0.5 and life 15→45 over the first 15 ticks (a ~19-particle start burst), then ~22 live particles per aura. | per aura: 1 batched draw, ~22 thin crossed quads, alpha-blended overdraw | none (look unchanged; Blue byte-identical emitter trace) | LOCAL parity trace | GPU cost not measurable here; overdraw grows with camera proximity |
| **H2 Engine warnings flushed to disk every frame during combat** | SOURCE: `cCcD_DivideArea::CalcDivideInfo` (c_cc_d.cpp 74) warns when a **collision** object's AABB lies exactly on the +X edge of the frame's collider union (zero-width collider). It warns every frame while that lasts, and borealis flushes every WARNING line (`flushOn = Warning`). LOG: 1 125 identical lines, 52 runs. Per 300-tick window: 143 (frames 3900), **171** (4200), **270** (10200), **300 = every tick** (10500), 86 (10800), exactly in the group-combat windows. They also continue with **no aura alive** (e.g. 108 lines around 9900-10200) and after the Bokoblins died. | up to one synchronous log flush per frame (file, console if enabled) | none (engine) | external A/B with `--log-level error` (HUMAN_TEST) | not caused by the particles; collider source not identified from the log |
| **H3 Engine DEBUG logging** | LOG: 8 964 DEBUG lines from `dusk` (`fpcBs_Create` for every process), in bursts at room loads (1 715 lines in window 3900, 2 127 in 7500). Not flushed per line. | load-time cost, not sustained combat | none | external A/B (`--log-level warning`) | engine |
| **H4 Mod CPU** | LOCAL_MICROBENCH (0.8.9 code): steady aura tick 58 ns, activation 1.07 µs, inactive 19 ns. 689 INFO lines (27 % of bytes), never flushed. | microseconds | +7 ns per active Blue tick for the new eye step; Blue eye lines budgeted | PERF_WINDOW now measures it in the game | — |
| **H5 Emitter churn ON/OFF** | LOG: 33 BLUE_GROUP_ON, **31 of them with the companion at 640-650** (threshold 650): the group flips at the edge, each ON creating an emitter with its start burst. | small per activation | none (thresholds/hysteresis forbidden) | — | gameplay rule kept |
| **H6 Lost auras (functional)** | LOG: Blues 454/455, 772/773 and 1574: `TEVSTR_NOT_READY_YET` then a PERMANENT rejection (no `EMITTER_ACQUIRED` ever). SOURCE: `Material_use_fg = 124` is set only in the actor's DRAW (`daE_OC_c::draw` → `settingTevStruct`, d_kankyo.cpp 3717); an actor that is not drawn exhausts the 60-tick budget. | auras missing for the rest of the actor's life | **fixed:** the wait for the first draw is unbudgeted (field read, no engine call); engine refusals stay bounded | strokes_lab (actor not drawn for 30 s, then drawn: aura appears; pool refusals still exactly 61 calls) | — |
| **H7 Texture replacements** | LOG: 190 `texture_replacement` mip warnings, mostly at load windows (0-900). | load time | none (user files untouched) | — | external |

**Conclusion:** nothing in the log proves that the particles cause the dips. The strongest correlated, verifiable candidate is H2: an engine warning flushed to disk on every frame of the combat windows, independent of the auras. 0.9.0 therefore adds the **instrument** that was missing, so the next human round can tell the candidates apart.

## B. The instrument: `DEVF_CP12 event=PERF_WINDOW` (one line every 300 simulation ticks ≈ 10 s)
- **What it measures:** `mod_update` runs once per simulation tick (f_ap_game.cpp 807, inside the `numSimTicks` loop of m_Do_main.cpp). The wall-clock interval between two ticks is ~33.3 ms at 30 Hz, and any whole-frame hitch shows up as a long interval. The line reports:
  - mean, p50, p95, p99 and max of that interval;
  - the number of intervals ≥ 40, ≥ 50 and ≥ 67 ms;
  - catch-up ticks (< 8 ms) and pauses or loads (≥ 1 s), which are excluded from the percentiles;
  - the mod's own CPU (PRE + POST hooks + update): total, per tick and worst tick;
  - live auras (average and max), tracked Bokoblins, emitter creations and eye texture swaps.
- **Clock:** `QueryPerformanceCounter`, calibrated against `GetSystemTimeAsFileTime`. Both were already in the reviewed import set (used by the CRT), so the audited ABI is unchanged. `<windows.h>` is confined to `expansion/perf_clock.cpp`.
- **Log level:** the line is written at ERROR level only so that it survives `--log-level error`, the A/B round that silences every engine warning. It carries `level_note=MEASUREMENT_NOT_AN_ERROR`.
- **Cost:** two counter reads per hook call and one line per 10 s.
- **It is not:** a GPU timer or a presented-frame time.

## C. Visual changes (the only authorised ones)
| Item | 0.8.9 | 0.9.0 |
|---|---|---|
| Black aura global prm / env | (190,100,255) / (70,25,130) violet | **(255,32,24) / (130,0,6) intense red**: no blue above green (not pink), green ≤ 32 (not orange) |
| Black eyes | 7 private CMPR variants tinted (158,60,255) purple | same machine, tint **(255,32,24)**, the aura's red; green eye endpoints only, luminance kept, pupils and neutrals kept, block mode kept |
| Blue aura | (80,190,255) | unchanged: **emitter trace FNV identical to 0.8.9** |
| Blue eyes | native | **NEW:** blue/cyan (80,190,255), the aura's colour, during the group bonus; 10 in / 15 out with the aura; native texture restored at OFF, death, room change and unload |
| Red | no aura, native eyes | unchanged |

**How the Blue eyes work:**
- **Contract per instance, read-only:** 3 materials and 3 shapes; material 2 is `oc_eye_mat`, with 1 TEV stage sampling texture 0; the eye packet holds that material; the texture is a 128x128 single-mip CMPR atlas.
- **When the set is built:** a private 7-variant cyan set per atlas (cache keyed by image pointer and FNV of the bytes) is built when the Blue is CLASSIFIED (spawn or room load), never in combat.
- **Detection:** the proven green-endpoint rule is tried first. If the E_OC atlas has no green eye endpoints, every chromatic endpoint of the copy is tinted, which is safe because the copy is bound only to that model's eye packet. If neither rule finds eye endpoints, the Blue eyes are refused and logged (`BLUE_EYES_UNAVAILABLE`); success is never faked.
- **Fade:** `devf_eyes12::step_eyes` is the exact eye block of `devf_vis::step` without its Black-only guard (lab: 3 000 random sequences identical to the Black machine).
- **Ownership:** recorded per visual slot, stamped with id and generation. Every write checks that the packet still holds what this module put there.

## D. Before / after (LOCAL_MICROBENCH: real mod code, fake engine, gcc -O2, WSL2, 2 x 2 990 ON/OFF cycles; not FPS)
| Metric | 0.8.9 | 0.9.0 | Note |
|---|---|---|---|
| Activation tick (median / p95 / p99) | 1 068 / 1 208 / 1 648 ns | 1 317 / 1 468 / 2 051 ns | + the Blue eye step and its texture swap |
| Steady active tick | 58 / 64 / 75 ns | 65 / 79 / 86 ns | + `step_eyes` for Blue |
| Inactive classified actor | 18 / 20 / 22 ns | 19 / 20 / 31 ns | ≈ unchanged |
| Log lines / bytes per ON/OFF cycle (after the detail budget) | 4 / 1 331 | 5 / 1 612 (Blue eye lines budgeted afterwards) | |
| Blue emitter trace (every field and position, every tick) | `78f322ed0f681c73` | **identical** | also identical under ASan/UBSan |
| Black emitter trace without colour | `4b8885710527f5c3` | **identical** | colour (190,100,255) → (255,32,24) |
| Blue eye packet: cyan while on / native after off | 0/20, 20/20 | **20/20, 20/20** | stub with fake E_OC model |
| In-game FPS / frame time | — | — | **HUMAN pending**; PERF_WINDOW will measure it |

## E. Regression matrix
- **Unchanged and re-run in CI:** runtime 66, Black 26, texture 20, variant 30, damage 22, dynamic 28, visual 22, eye 9, perf 4, editor 42/71/110/25.
- **strokes_lab, 36 cases (35 + 1):**
  - Black red, not pink nor orange;
  - first-draw wait: 30 s undrawn then drawn gives the aura; engine refusals still bounded.
- **cp12_lab, 7 cases:**
  - red and cyan rules on synthetic CMPR atlases, including yellow eyes (fallback) and a neutral atlas (honest refusal);
  - eye tints equal the aura colours;
  - `step_eyes` exactly equal to the Black machine;
  - the Blue lifecycle, including the "never restored" bug and re-activation mid-fade;
  - foreign packet and write failure; generation stamping;
  - PERF_WINDOW percentiles, thresholds, pause exclusion and GHz clock calibration.
- **Mutations:** 8 mutations of the eye and perf policies are each caught.
- **Stub runs:** the stub of the real mod code ran under ASan/UBSan with `-Werror=format` (visual region and the PERF_WINDOW region).
- **CI gates:** MSVC C4477/C4473, PE/ABI, imports unchanged, one hook, independent download.

## F. Limits
- The Blue E_OC eye atlas and its native eye colour are not verified from here. The runtime contract and the adaptive detection either work or log a refusal; only the human round shows the colour on screen.
- The CalcDivideInfo collider is not identified, and the engine warning cannot be silenced from a `.dusk`.
- The measured cost is mod CPU only. GPU time and presented frame time are not measured.
- The particle look is not reduced in any way.

---

# CP11_PERFORMANCE_OPTIMIZATION — Expansion 0.8.9 (base: 0.8.8 / CP10.6D-FIX02, source 9cb49ea)

Goal: recover CPU time and remove avoidable work in everything added after the spawn/placement worked, with **identical** gameplay and look.
Evidence classes are kept apart: **SOURCE** (exact code reading: mod and Dusklight 40457c6, borealis 4ac5e70), **LAB** (CI-run exactness tests),
**LOCAL_MICROBENCH** (the mod's own code with fake engine calls, gcc -O2, WSL2 x86-64: not Windows, not MSVC, not the game),
**WINDOWS_CI** (build/audit only) and **HUMAN** (FPS in the game, pending). No FPS figure exists in this checkpoint.

## A. Frozen baseline: where the mod spends time (SOURCE)
| Path | Frequency | 0.8.8 work | Verdict |
|---|---|---|---|
| PRE `on_execute` — registration snapshot | every Bokoblin, every tick, while a Black is configured | `dComIfGp_getStartStageName` + `strncmp` + `dComIfG_play_c::getLayerNo(0)`. `getLayerNo` recomputes the layer on each call: `dKy_darkworld_stage_check` plus a chain of ~40 stage-name `strcmp` and event-bit reads (`d_com_inf_game.cpp` 160-889) | **optimised (exact):** lazy context |
| PRE — registry / ledger lookups | every Bokoblin, every tick | linear scans of 64-slot tables | small; early exit where the id is absent |
| POST `dynamic_post` | every classified Bokoblin, every tick | snapshot, `stage_tag()` (short FNV), companion scan (≤ 64), `reconcile` | sub-microsecond; untouched (inclusive thresholds, order and window kept) |
| POST `visual_post`, inactive actor | every classified Bokoblin, every tick | visual-ledger lookup scanning all 64 slots | **optimised (exact):** early-exit lookup |
| POST `visual_post`, active aura | every tick while the aura is on | keyed `dComIfGp_particle_set` + `configure` writes, **plus** colour/geometry/presence read-backs and the draw-basis arithmetic (two 40-step Newton square roots) | **optimised (exact):** read-backs only when the emitter is created; all writes kept |
| Activation / deactivation | each group ON/OFF, each fury start/end | 8 log lines, 4.6 KB, ~50 formatted floats; the host copies each line into std::string twice under a mutex (mods log ring + borealis) | **optimised:** detail budget, compact lines, anomalies always in full |
| `mod_update`: `regen_poll` (≤ 8 `fopAcM_SearchByID`) + `black_poll` | every frame | linear searches of the actor list | kept: changing the cadence would change release latency (forbidden) |
| Black texture + 7 eye variants | once per session, at the first Black admission | 8 × (alloc + 8 KB copy + recolor + J3DTexture) | already cached once per session; untouched |
| JPA particles on the GPU | every drawn frame while an aura is on | ~22 particles per emitter, shape 4 = two crossed quads each, alpha blended, 14 x 180 nominal, base colour opaque, rate step 0 | **not reducible without changing the approved look** (see F) |

## B. Hotspots verified vs hypotheses discarded
- **Verified (SOURCE + LOCAL_MICROBENCH):** activation-tick logging dominated the mod's own CPU (7.9 µs median, p99 16 µs, versus 0.42 µs for a steady tick). The per-tick read-backs were pure waste on every active tick. `getLayerNo` ran for every Bokoblin on every tick while a Black was configured.
- **Discarded:**
  - Log disk flushes as a cause of stutter. borealis flushes only at `Warning` and above (`log.cpp` `flushOn = LogLevel::Warning`), so our INFO lines are buffered. The many texture-mip WARNINGS of external replacements, however, ARE flushed per line: an external confounder, not touched.
  - The keyed emitter lookup: it is O(1), a `u8` hash in `level_c::get`.
  - The pool / companion scans: sub-microsecond.
- **Most probable cause of the human-reported dips (NOT measured):** GPU or driver cost of the particles themselves on Intel UHD (blended overdraw of tall quads). This cannot be reduced without changing what was approved (F).

## C. Changes (files: `expansion/mod.cpp`, new `expansion/perf_policy.hpp`, new `tests/perf_lab.cpp`, `tests/bench/*`, CI)
1. **Lazy registration context** (`perf_policy.hpp` `complete_snapshot`). The stage and layer fields are read only by `Table::context_matches`, which every consumer evaluates after `record_matches` for a used registration. The context is now computed only if some used Black or Normal registration record-matches the actor, and when it is, it is computed exactly as before. LAB: 200 000 random tables and snapshots give identical `find`, `matches`, `devf_variant::match` and `apply` decision and state. Context calls for 8 Bokoblins (1 Black) over 30 ticks: **240 → 30**.
2. **Read-backs only on creation** (`RealEngine::configure`). Every write of 0.8.8 still happens every tick:
   - frame, geometry, base colour and rate step;
   - colours and alpha, which are required because `dPa_control_c::set` → `dPa_kankyocolor_set` can re-apply environment colours on each keyed refresh.

   The colour, geometry and presence read-backs and the draw-basis check feed only the creation lines, so they now run once per created emitter.
3. **Early-exit ledger lookup** (`find_used`). Both ledgers keep `used` exact, so the scan stops after `used` occupied slots. LAB: 300 000 random record/release/query operations match `Ledger::find` exactly on the real `devf_vis` and `devf_dyn` ledgers.
4. **Telemetry budget:**
   - **Full detail** (REQUESTED, COLOR_APPLIED, TUNING_APPLIED, DRAW_BASIS_CHECK and the geometry tuple) for the first 2 emitters of each variant per session.
   - **After that, compact lines:** ACTOR_STATE_ON/OFF, EMITTER_ACQUIRED and EMITTER_RELEASED, with the same event names, ids and `route=`, plus `detail=COMPACT`.
   - **Logged in full regardless of the budget:** every retry, rejection, colour/geometry/presence mismatch and degenerate basis.
   - One `TELEMETRY_BUDGET` line per variant says when the switch happens.
   - Gameplay logs (CP10.3–CP10.6) are untouched.
   - Side benefit: the 24-per-actor / 400-per-session caps now cover more activations.
5. `char text[N]{}` → `text[0]=0` in the visual log functions (snprintf always terminates). This saves zeroing up to 1.7 KB per line.

Not changed: gameplay, classification, HP, regeneration, damage, colliders, group/fury logic, eyes, texture, `strokes_policy.hpp` (byte-identical to 0.8.8), emitter count, every emitter parameter, hook, imports, provider, editor, JSON.

## D. Before / after
| Metric | 0.8.8 | 0.8.9 | Source / conditions |
|---|---|---|---|
| `visual_post`, activation tick (emitter created + its lines), median / p95 / p99 | 7 901 / 10 012 / 16 708 ns | **1 079 / 1 267 / 1 659 ns** | LOCAL_MICROBENCH, 2 x 2 990 activations, Blue and Black, budget exhausted (the first 2 per variant cost as in 0.8.8) |
| `visual_post`, steady active tick | 424 / 455 / 463 ns | **58 / 63 / 74 ns** | LOCAL_MICROBENCH, 352 820 ticks |
| `visual_post`, inactive classified actor | 33 / 36 / 45 ns | **18 / 20 / 24 ns** | LOCAL_MICROBENCH, 478 400 ticks |
| Log lines / bytes per ON→OFF cycle | 8 / 4 604 | **4 / 1 331** (first 2 per variant: as 0.8.8, plus 1 budget line) | LOCAL_MICROBENCH sink (exact byte count) |
| Layer/stage context evaluations, 8 Bokoblins incl. 1 Black, 30 ticks | 240 | **30** | LAB (exact count) |
| Emitter state written per tick (all fields + position + set calls) | FNV `e4d0afeb47b6fbed` | **identical** | LOCAL parity trace, 444 000 `set` calls; also identical under ASan/UBSan |
| In-game FPS / frame time on Intel UHD | not measured | not measured | **HUMAN pending** |

The microbench times only the mod's own code; the engine calls are fakes. The real per-line host cost (string copies, mutexes, borealis formatting, console if attached) comes on top of the measured formatting, and the halved line count and −71 % bytes reduce it proportionally. Reproduce with `tests/bench/make_bench.py <mod.cpp> out.cpp`, then `g++ -O2 -std=c++20 out.cpp` from `tests/bench/` (the 0.8.8 `mod.cpp` is at commit 9cb49ea).

## E. Parity matrix
| Area | How it is kept identical | Evidence |
|---|---|---|
| Spawn, placement, provider/editor API, JSON | not touched | SOURCE (diff), editor source identity gate, CI |
| Classification Blue/Red/Black, Black admission, HP_APPLIED / BLACK_REJECTED | lazy context exactness | LAB perf 1 (200 000 cases) + CI black/variant labs unchanged |
| HP init, regeneration 1/30 s, death, damage, colliders, group (<, =, > thresholds), fury 300 ticks | code untouched; dynamic ledger lookup exact | LAB perf 3 + CI dynamic/damage/variant/regen labs unchanged |
| Aura: resource, geometry, frame, dir, sweep, band, height, colours, alpha, fades, base prm, rate step, follow, cleanup | `strokes_policy.hpp` identical; every write kept per tick | LOCAL parity hash identical; CI strokes_lab 35 unchanged; static gates on every setter |
| Eyes, private texture | untouched | CI eye/texture labs |
| ID reuse, room change, death, unload, rollback | lookups return exactly what `find` returns; paths untouched | LAB perf 3, CI visual lab |
| Logs | budget; anomalies always full | LAB perf 4, printed sequence |

## F. What could NOT be optimised without losing fidelity (documented, not done)
- **GPU cost of the particles:** about 22 particles per active actor, each a crossed pair of tall alpha-blended quads, with base colour opaque, rate step 0 and length 0.60 (all part of the approved 0.8.8 look). Fewer, shorter or fainter particles, culling or distance limits would all change the look, so none was applied.
- **Keyed refresh every tick (`dComIfGp_particle_set`) and the colour/alpha writes every tick:** needed for the ring to follow the actor, and because the engine can re-apply environment colours on each refresh.
- **Frame polls** (`regen_poll`, `black_poll`): reducing their cadence would delay releases.
- **First-time engine costs:** texture uploads of the eye variants at the first fury, and the first draw of the particle resource.
- **External factors:** texture-replacement WARNING lines are flushed to disk one by one by the host logger; other mods.

## G. Limits
CPU savings are microseconds per tick. If the dips are GPU-bound, as suspected, 0.8.9 may feel the same as 0.8.8. Only the human A/B test can say, and the result must not be claimed beforehand.

---

# CP10.6D-FIX02 — BODY_HUGGING_STROKES (Expansion 0.8.8)

## FIX02.1 Human ground truth (0.8.7) and reference
- 0.8.7 is visible and roughly centred on the body, but too faint.
- It drifts away from the body and floats around the enemy like a loose ring instead of covering the silhouette.
- The target is the Dark Souls Grass Crest Shield look: thin, bright, near-vertical lines very close to and over the body. The user's reference captures show lines passing in front of the torso and rising above the head.

## FIX02.2 What changed (same resource 0x0B9D, same shared route, ONE emitter per actor)
| Item | 0.8.7 | 0.8.8 | Reason / source |
|---|---|---|---|
| Ring centre (XZ) | `current.pos` (feet axis) | **midpoint of `mSphs_cc[0]` (torso, R 50) and `mSphs_cc[1]` (head, R 45) centres**; feet fallback if a radius is 0 or the offset is > 80 per axis | The Bokoblin is hunched: the body is not over the feet. `daE_OC_c::cc_set` (`d_a_e_oc.cpp`) recomputes both spheres from the animated joints during `execute`; our POST reads them with the inline getters `GetCX/GetCZ/GetR` (read-only). |
| Ring radius | 24 x 3.8 = 91.2 | **24 x 2.4 = 57.6** | 0.8.7 never placed a stroke closer than 58.4 (lab), the loose ring that was seen. |
| Spawn band | 58.4 .. 91.2 (thickness 32.8) | **41.5 .. 57.6** (thickness 16.1) | 0.83 .. 1.15 x the 50-unit torso collider: strokes straddle the silhouette. Some are born in front of or just outside the body, some behind it (hidden naturally). |
| Stroke size | 0.70 x 0.55 (14 x 165) | **0.70 x 0.60 (14 x 180)** | Same thin width; slightly longer to cover from the legs to above the head (reference). |
| Height | 0.65 x eye offset | unchanged | User: already centred. |
| Alpha cap | 234 | **255** | The CP10.6A fade is unchanged (10 in / 15 out). |
| Emitter base prm colour | resource copy (255,255,195,**150**) | **(255,255,255,255) on this emitter** | The draw multiplies the emitter's `mPrmClr` by the global colour (`JPABaseShape.cpp` L24-28, L64-68, ...; particles copy it at birth, `JPAParticle.cpp` L93). If the resource animates prm colour, the engine overwrites it every tick (`JPACalcPrm`); this is reported as `prm_anim=1, base_prm_effective=NO_RESOURCE_ANIMATES_PRM`. The shared `JPAResource` is untouched. |
| Rate step | resource value (logged) | **0** (`setRateStep`, inline) | Emits on every tick; no-op if the resource already uses 0. The rate and lifetime curves still rule density. |
| Colours | Blue (70,170,255) / Black (185,90,255) | **Blue (80,190,255) / Black (190,100,255)** | Slightly brighter, same hues; env colours brightened alike. |
| Route id in logs | `SHARED_CANONICAL_RING` | **`SHARED_BODY_HUGGING_STROKES`** | |

Unchanged:
- the canonical frame (sweep 1, translation 0, rotation 0, scale 1);
- the non-degenerate draw basis (dir (0.10, 0.99499, 0), `|cross| = 0.10`);
- the shared Blue/Black geometry object, retry, cleanup and capacity 8;
- eyes, gameplay, provider, editor and JSON.

**One emitter per actor is enough:** the reference look is SPARSE lines, and a single ring already yields roughly rate x lifetime (about 0.5 x 45, about 22) strokes around the body. More emitters would turn it into a curtain or swarm and double the shared-pool cost.

## FIX02.3 Checks
- **Lab, 35 cases (32 + 3):**
  - the ring centre follows the torso/head colliders, with a feet fallback and the offset reported;
  - the band straddles the collider surface (over 300 of 2000 model births on each side), while 0.8.7's nearest birth was 58.4;
  - presence on the emitter (alpha 255, base prm opaque, rate step 0) is read back and a mismatch is reported;
  - all earlier expectations are updated to the new values;
  - 8 mutations of the FIX02 values or logic are each caught.
- **Stub compile** of the real `mod.cpp` aura path with gcc `-Werror=format`, ASan and UBSan. It caught a real `snprintf` argument mismatch in `TUNING_APPLIED` during development (a missing `volume_scale` argument), fixed before commit.
- **New CI gate:** the build fails on MSVC C4477/C4473 (printf format mismatch). The previous run's MSVC log had none.

## FIX02.4 Limits
- Appearance is pending the human round.
- The resource's own texture, alpha animation and colour animation (if any) still shape brightness; the log now says whether the base colour override is effective.
- Births inside the body are hidden by the model, by design.
- The collider centres are the game's own approximation of the body.

---

# CP10.6D-FIX01 — non-degenerate draw basis (Expansion 0.8.7)

## FIX01.1 Cause of the 0.8.6 invisibility (SOURCE, exact engine 40457c6) — my error in 0.8.6
Human result 0.8.6: Blue and Black totally invisible (HUMAN_VISUAL_FAIL), while the log showed the resource found, 15 emitters acquired, 0 rejected, triggers on, canonical frame read back. The emitters existed; the engine skipped every quad.

- Shape 4 is the direction cross. `JPAResource.cpp` L618-623 selects `JPADrawDirection` for it, or `JPADrawRotDirection` if a rotate animation is on. Dir type 3 is `dirTypeEmtrDir` (`JPABaseShape.cpp` L701-704): `axisY = mGlobalEmtrDir = global x local rotation x mLocalDir` (`calcWorkData_d`).
- `JPABaseParticle::init_p` (`JPAParticle.cpp`): `mBaseAxis = (mGlobalRot[0][1], mGlobalRot[1][1], mGlobalRot[2][1])`, the Y column of global x local rotation. Our emitters have the identity global rotation (no rotation passed to `dPa_control_c::set`), and 0.8.6 forced local rotation 0, so `mBaseAxis = (0,1,0)`.
- `JPADrawDirection` / `JPADrawRotDirection`: `axisY.normalize(); axisZ.cross(ptcl->mBaseAxis, axisY); if (axisZ.isZero()) return;`, where `isZero` is `squared() <= 32 x FLT_EPSILON` (`JGeometry.h` L391).
- 0.8.6 set `mLocalDir = (0,1,0)`, so `axisZ = (0,1,0) x (0,1,0) = 0` and the function returned before submitting the quad, for every particle and every frame.
- **Dusklight detail that sets the margin:** on presentation frames the draw runs on an interpolated copy (`dusk::interp::particle::present`, `src/dusk/interp/particle.cpp` L230-250). The re-orthogonalised `mBaseAxis` written at the end of `JPADrawDirection` is therefore not kept on the real particle, so the base can stay (0,1,0) for the particle's whole life. The cross product must be safely non-zero in every frame, not just the first.

## FIX01.2 The fix (one value of the one shared geometry)
- `kGeometry.dir = (0.10, 0.99498744, 0)`: unit length, 5.7 degrees from vertical.
- With `mBaseAxis = (0,1,0)`: `|cross| = 0.10` and `|cross|^2 = 0.01` in **every** frame, about 2600 x the engine threshold (3.8e-6). Where the base is written back, `|cross| = 1` after the first draw.
- **Why the ring is unchanged:** the direction is not a term of the birth position (`init_p`: `mLocalPosition = mGlobalSR x volumePos`; the volume uses rotation x scale only). Sweep 1.0, translation 0, rotation 0, scale 1, radius 91.2, band 58.4-91.2, quads 14 x 165 and height 0.65 are all kept exactly.
- **Drift:** it follows the same vector (`JPAGetDirMtx` column 2 with spread 0 = dir), so it is upward, 5.7 degrees off vertical.
- **Quad orientation:** the length axis is the tilted direction; the width axis is horizontal (~(-1,0.1,0)). The cross shape draws a second perpendicular quad, so it is visible from the sides as well.
- **Alternatives rejected:**
  - Exactly vertical quads would need a base axis that is not vertical. That requires a global or local rotation whose Y column is not up, and the same rotation would tilt the emission ring (SR = rotation x scale), bringing the clump problem back.
  - A per-particle callback would add host-to-mod virtual dispatch for one degree of verticality.
- Blue and Black still share the same geometry object; only colour and the logic condition differ.

## FIX01.3 Checks added
- **Compile time** (`strokes_policy.hpp`):
  - `static_assert` that the configured basis is drawable with a margin above 100 x eps;
  - `static_assert` that the 0.8.6 basis is degenerate.
- **Lab** (32 cases = 28 + 4): an independent double-precision port of `init_p` + `JPADrawDirection` (with and without base write-back) and of `JPAGetDirMtx`.
  - **A.** The 0.8.6 frame is drawn in 0 of 300 frames.
  - **B.** The configured Blue and Black emitters are drawn in 300/300 frames, with `|cross| >= 0.10` and the same numbers as `DRAW_BASIS_CHECK`.
  - **C.** The ring is unchanged: 8/8 sectors, birth positions identical to 0.8.6, upward drift.
  - **D.** The quad is near-vertical (5.7 deg) with a horizontal width; Blue is cyan and Black violet.
  - Mutation: with the 0.8.6 direction, case B fails on its own (also with the table check removed), and near-degenerate (0.002) or over-tilted (0.30) directions fail too.
- **Telemetry:** `DRAW_BASIS_CHECK`, once per created emitter, computed from values read back from the emitter (`mGlobalRot`, `mLocalDir`, local rotation 0 checked): `dir_applied`, `base_axis`, `axis_y`, `dot`, `cross_len`, `cross_sq`, `engine_threshold`, `status=NON_DEGENERATE|DEGENERATE_QUAD_WOULD_NOT_DRAW`, `evidence=MATHEMATICAL_READBACK_NOT_RENDERED`. It is not a pixel claim.

## FIX01.4 Not addressed / limits
- What it looks like is pending the human round.
- Size, alpha, density and colours were **not** changed: there is no independent reason, and the draw returned early anyway.
- If the strokes are still invisible with `DRAW_BASIS_CHECK status=NON_DEGENERATE`, the cause is somewhere else. That must be reported as FAIL, not patched blindly.

---

# CP10.6D — implementation and evidence (Dusklight 40457c6)

Evidence classes: **SOURCE** (exact engine/mod code), **LAB** (offline model), **CI** (Windows build), **HUMAN** (the user). The visual result is HUMAN and pending.

## 1. Why the strokes clumped (SOURCE) and what 0.8.4 / 0.8.5 did not control
`JPABaseEmitter::init` (JPAEmitter.cpp L17-55) copies from the resource into EACH emitter: local scale, local translation, local direction, local rotation and the circle **sweep**. A circle-volume particle is then born at (JPADynamicsBlock.cpp `JPAVolumeCircle`, JPAResource.cpp `calcWorkData_c` L1352-1377, JPAParticle.cpp `init_p`):

```
theta    = mVolumeSweep * random_s16                         -> sweep < 1: only an ARC, i.e. one fixed sector of the circle
distance = size * (minRad + r * (1 - minRad))
local    = (distance sin theta, 0, distance cos theta)
position = (globalTrs + globalScale * mLocalTrs) + (globalRot * localRot(mLocalRot) * localScale(mLocalScl)) * local * globalScale
quad axis = globalRot * localRot * mLocalDir                 (dir type 3, JPABaseShape.cpp dirTypeEmtrDir)
```

This family is Link's sword glow: its data is authored for a sword matrix. Our emitter's global rotation is identity (no sword matrix), so any authored sweep < 1, anisotropic local scale, local rotation or local translation produces a clump in a **fixed world sector**, a squashed line or a tilted plane. A larger radius cannot fix that (lab: an 8x radius with sweep 0.15 still covers <= 2 of 8 sectors). 0.8.4 controlled none of these, and 0.8.5 controlled only the translation, plus the direction when the resource was unrotated. The exact authored values of `0x0B9D` were not available to me (the 0.8.5 log was not provided). 0.8.6 logs them in `RESOURCE_GEOMETRY` and makes the result **independent of them**.

## 2. The fix: one canonical frame + one geometry, on each emitter (never on the shared JPAResource)
Written in `configure()` every tick (idempotent; the resource's only key curves drive `mRate` and `mLifeTime`, nothing rewrites these fields):

| Field | Value | Effect |
|---|---|---|
| `mLocalTrs` (`setLocalTranslation`) | (0,0,0) | ring centre = the position we pass |
| `mLocalRot` | (0,0,0) | ring in the horizontal plane |
| `mLocalScl` (`setLocalScale`) | (1,1,1) | round ring, velocities unscaled |
| `mLocalDir` (`setDirection`) | 0.8.6: (0,1,0) — **degenerate, see FIX01**; 0.8.7: (0.10, 0.99499, 0) | near-vertical strokes, upward drift |
| `mVolumeSweep` | 1.0 | **full 360-degree circle** |
| `mVolumeMinRad` | 0.64 | births between 58.4 and 91.2 from the axis, all outside the 50-unit torso collider (`d_a_e_oc.cpp cc_set`) |
| global dynamics scale | 3.8 | ring radius 24 x 3.8 = 91.2 (about 1.8x the torso collider: around the body, not a far halo) |
| global particle scale | 0.70 x 0.55 | thin quads, 14 x 165 nominal |
| emitter position | `current.pos` + (0, 0.65 x clamp(eyePos.y - current.pos.y, 60, 500), 0) | on the actor axis, at chest height (0.8.5 used 0.58), follows every tick |
| alpha | `level x 234 / 100` (same for both) | CP10.6A fade unchanged (10 in / 15 out); first sample 23, peak 234 |

**Emitters:** still **one per actor**, max 8. With a full sweep a single ring already surrounds the body; more emitters would not be needed and would strain the shared pool.

## 3. Unification (Blue = Black's corrected route, colour only)
`strokes_policy.hpp`: `resource_for(Blue) == resource_for(Black) == 0x0B9D`; `geometry_for(v)` returns the **same object** `kGeometry` for every variant; `colour_for(v)` is the only per-variant value: Blue (70,170,255)/(20,70,150), Black (185,90,255)/(60,20,120). `0x0A58` (Blue's old SWL_A route) is in `kNeverUsed`, and a CI gate checks it is absent from `mod.cpp`. Logic conditions are unchanged: Blue = group bonus, Black = fury.

## 4. Files
`expansion/strokes_policy.hpp` (rewritten: shared resource, canonical frame, `kGeometry`, `Colour`, shared alpha), `expansion/mod.cpp` (visual block only: probe, telemetry, `RealEngine::configure`; version 0.8.6), `tests/strokes_lab.cpp` (28 cases), `ci/*` (names, version, static gates), `expansion/mod.json`, docs. Gameplay, provider, editor, JSON, eyes and cleanup code are untouched.

## 5. Verification here
LAB 28 cases, ASan/UBSan locally, including:
- identical emitters for Blue and Black except colour;
- the canonical frame replacing a sword-authored frame and being restored every tick;
- a **coverage model** porting the birth formulas: the canonical frame gives births outside the torso in **8 of 8** sectors, while authored narrow-sweep, squash, offset or tilt frames clump;
- shared alpha progression, missing shared resource without fallback (eyes kept), plus all the previous retry, cleanup and capacity cases.

8 policy mutations are all caught. The reused labs run unchanged. A stub compile of the real `mod.cpp` aura path ran under ASan/UBSan with a deliberately bad authored frame and read back the canonical one. CI adds MSVC, PE/ABI audit and the independent download check. **No game executed, no pixel rendered.**

## 6. Telemetry (`DEVF_CP10_6D`, transitions only, bounded)
`RESOURCE_PROBE` / `RESOURCE_FOUND` / `RESOURCE_MISSING` (`used_by=BLUE_AND_BLACK`), `RESOURCE_GEOMETRY` (authored sweep, min radius, translation, rotation, scale, direction), `CURVE_PROBE` (key table plus one live sample with sweep), `ACTOR_STATE_ON/OFF`, `REQUESTED`, `EMITTER_ACQUIRED/REJECTED/RELEASED`, `COLOR_APPLIED/UNAVAILABLE`, `TUNING_APPLIED` (route, geometry, `y_off`, radii, readback of the canonical frame, authored values, colour, alpha). Eye lines keep `DEVF_CP10_6A_VISUAL`.

## 7. Limits
- Appearance is pending the human round.
- The resource's own fields (`fields=` in `RESOURCE_PROBE`), its scale or colour animation and its texture are not modified, and could still shape the look.
- Density is what the rate and lifetime curves give.
- Nominal sizes are not rendered sizes.
