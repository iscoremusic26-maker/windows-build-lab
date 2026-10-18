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
| `mLocalDir` (`setDirection`) | (0,1,0) | vertical strokes, upward drift |
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
