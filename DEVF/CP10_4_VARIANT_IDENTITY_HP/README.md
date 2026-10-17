# DEVF CP10.4 — Blue / Red / Black identity and HP (Expansion 0.6.0)

`BokoblinExpansion_0.6.0_TEST.dusk`, mod id `dev.bokoblin.expansion` (replaces 0.5.0; never install both). Provider `DEVF_Provider_0.10.0_TEST.dusk`, editor `DEVF Map Editor CP10.2.exe`, schema, placements and the approved Black texture are **unchanged**.

| Actor | Identity | HP (initial = max) | Regeneration |
|---|---|---|---|
| native `E_OC` (any stage) | BLUE | 40 → **220** | 2 HP/s |
| native `E_OC2` (any stage) | RED | 220 → **330** | 3 HP/s |
| DEVF encounter `normal` / legacy (no `bokoblin_variant`) | BLUE | 220 | 2 HP/s |
| DEVF encounter `black` | BLACK | its JSON `hp` | 4 HP/s |

- Precedence per actor: exact DEVF registration (Black, then Normal) > native `E_OC` > native `E_OC2`. Never proximity, colour, texture or the profile alone.
- HP is set **once**, at the actor's first execute (inside the game's own `create()`), then only bounded regeneration. A modified native signature, a non-first-execute sighting, a failed Black admission or any other enemy name is **rejected** with a logged reason and no HP write.
- Black keeps its configured `hp`: a JSON with `hp: 150` stays 150. The balance target is 440: change that one value (see `HUMAN_TEST.md`); nothing is migrated automatically.
- Honest limitation: the DEVF *Normal* encounter is spawned by the unchanged provider with the E_OC2 **resource**, so it still looks like a native E_OC2 even though it is classified BLUE. No appearance change is claimed. The editor still labels it "Normal".
- Not implemented here (specification only): damage multipliers, fire/ice/electric, vision, attack speed, fury, pairs, Silver/White/Golden, new textures. The native "shield stare" behaviour predates this mod (reproduced in 0.4.1) and is untouched.

Details: `IMPLEMENTATION.md`; QA: `HUMAN_TEST.md`. Status: CI per the pull request; **HUMAN_RUNTIME_PENDING**.
