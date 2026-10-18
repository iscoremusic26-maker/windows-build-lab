# DEVF CP10.5 — base attack power per variant (Expansion 0.7.0)

`BokoblinExpansion_0.7.0_TEST.dusk`, mod id `dev.bokoblin.expansion` (replaces 0.6.0; never install both). Provider `DEVF_Provider_0.10.0_TEST.dusk`, editor `DEVF Map Editor CP10.2.exe`, `encounters.json` (Black `bokoblin-004` = 440 HP), schema, placements and the approved Black texture are **unchanged**. Identity, HP and regeneration are exactly CP10.4.

| Actor | Identity (CP10.4) | Native attack power | **Base attack power now** (both spheres) |
|---|---|---|---|
| native `E_OC` | BLUE | 1 | **1** (no write) |
| DEVF Normal / legacy (physically `E_OC2`) | BLUE | 2 | **1** (normalised 2 → 1) |
| native `E_OC2` | RED | 2 | **2** (no write; not doubled again) |
| DEVF Black (physically `E_OC2`) | BLACK | 2 | **3** (2 → 3, not 6) |

Target values are **absolute** `SetAtAtp(1/2/3)` values that replace the native one on `mSphs_at[0]` and `mSphs_at[1]`; they are never multipliers. They are applied once per actor generation at the first execute of an actor that CP10.4 has accepted. Any actor CP10.4 rejects (unknown name, modified native signature, failed Black admission, ...), and any actor whose two spheres are not the pristine native pair, is left exactly as the game made it and a reason is logged.

## What this does and does not prove
- **Source / CI:** the host reads the attack power of the sphere that hits Link, keeps the maximum of the hits of the frame and subtracts it (times Link's own damage magnification, rounded up) from life. At normal magnification the base hit is therefore proportional to 1 : 2 : 3. Details and file/line references: `IMPLEMENTATION.md`.
- **Not proven:** hearts actually lost by Link in the game. That is the human Windows round (`HUMAN_TEST.md`). The logs say what was written to the colliders (`link_damage=NOT_MEASURED`), not what Link lost.
- The exact 1 : 2 : 3 holds for the **base hit** under equal conditions. Link-side modifiers (Wolf, the 1.5 factor of some states, Zora armor, magic armor, the Dusklight damage-multiplier setting, invincibility frames) are outside this mod and can change the proportion; a non-integer factor rounds each hit up.

## Not implemented here (specification only)
Blue x2 near another Bokoblin, Black fury, Red fire, ice/electric, vision, chase, attack speed, shield-block changes, AI, Silver/White/Golden, new textures, new editor options, schema 3. The native "shield stare" after a blocked hit predates this mod and is untouched.

Status: Windows CI per the pull request; **HUMAN_RUNTIME_PENDING** (damage not measured in game).
