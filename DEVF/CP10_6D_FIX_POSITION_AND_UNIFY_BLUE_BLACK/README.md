# DEVF CP10.6D — FIX_POSITION_AND_UNIFY_BLUE_BLACK (Expansion 0.8.6)

`BokoblinExpansion_0.8.6_TEST.dusk`, mod id `dev.bokoblin.expansion`. It **replaces** 0.8.5 (never two Expansion files). Rollback: 0.8.5 (or 0.8.4). Provider 0.10.0, editor 0.10.2, `encounters.json`, schema and saves are untouched.

**Problem (USER_TESTIMONY with 0.8.4 and 0.8.5):** the Black's strokes appear clumped in a small spot at the waist / side / lower back instead of around the torso.

**Fix:** the effect the Black already used (`0x0B9D` `ID_ZI_J_LK_SWL_C`, common bank) is kept, but each emitter now gets a **canonical frame**: full-circle sweep, no authored local translation / rotation / scale, vertical direction. That removes the sword-frame data the resource carries, which (engine source) can confine every particle to one fixed sector, a squashed line or a tilted plane whatever the ring radius. The ring is centred on the actor axis at 0.65 of the measured eye height (chest), radius 91, spawn band 58–91 outside the 50-unit torso collider, thin vertical quads. **One emitter per actor** (max 8).

**Unified:** Blue now uses **that same corrected route** — same resource `0x0B9D`, same frame, same geometry, same alpha curve, same retry/cleanup. Blue's old `0x0A58` route is gone. Only colour and the logic condition differ: **Blue (70,170,255) blue/cyan** during the group bonus, **Black (185,90,255) violet/purple** during the fury (purple eyes unchanged).

Log prefix `DEVF_CP10_6D`; every aura line carries `route=SHARED_CANONICAL_RING` and the same `geometry=(...)` for both variants; `TUNING_APPLIED` shows the frame read back from the emitter next to what the resource authored. None of this proves pixels. Status: **GAME_VISUAL_HUMAN: PENDING**.
