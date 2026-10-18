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
