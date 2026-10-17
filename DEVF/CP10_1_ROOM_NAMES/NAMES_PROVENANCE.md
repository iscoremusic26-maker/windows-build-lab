# CP10.1 — English stage and room names: provenance

Editor-only display aliases. They are **never** stored in schema1/schema2 JSON, never used as keys or lookups, and never enable QA, export or placement. Identity is always the technical id `stage / Rnn_vv.arc` (or `stage / STG_vv.arc`). The alias table is `app/NameData.cs`, generated from the sources below; the full list is `ALIASES.tsv`. Verified 2026-10-17.

## Sources actually used

| Code | Source | Evidence type | Use | License |
|---|---|---|---|---|
| CML | **Community map list.** `TwilitRealm/dusklight@40457c6adb381928e4b5fef6ed459ed291edd5e2`, `src/dusk/map_loader_definitions.h`. The same 102 stage/room entries appear in `magcius/noclip.website` `src/ZeldaTwilightPrincess/Main.ts` @d63a2af (credited there to Jawchewa and SkrillerArt); the host list only adds `S_MV000`. Treated as ONE lineage, not two independent confirmations. | Community scene names keyed by stage id + room numbers | Stage names; room names where one stage is split into several named maps | Host file: CC0-1.0 (repo license). noclip: not asserted; used only as a cross-check, nothing copied from it. |
| K64 | https://kuribo64.net/wiki/?page=Stage_Names_%28TP%29 (79 stage ids, stage level only, no rooms) | Community wiki | Cross-check; sole source for `R_SP260`; coarse stage names for 5 multi-map stages | No license stated. Facts (place names) only. |
| SRC | Comments in the Dusklight host sources at the same commit: `src/d/actor/d_a_e_wb.cpp:5593`, `src/d/d_s_play.cpp:1315/1321/1327`, `src/d/d_com_inf_game.cpp:2901/2907` | Developer comments next to room-number checks | **Inferred** sub-area names for `F_SP121` rooms 0, 2, 9, 10 | CC0-1.0 |

## Sources evaluated and NOT used
- https://wiki.tprandomizer.com/index.php?title=List_of_Stages — pairs stage with room ids only; its "translation" column is empty (red links). IDs alone are not labels.
- https://github.com/JaxonKEKW/TPDebugMenuEditor — its `Menu1.dat` is the game's own debug-menu file (Japanese strings extracted from the commercial game), and the repository has no license. Not copied, not used.
- Randomizer check names (`chest:F_SP121:N`): N is a flag index, not a room number.

## Confidence tiers shown in the editor
1. **Stage named** — stage name documented (CML, or K64 where CML has several maps). Room shows as `Stage name · Room nn`. `Room nn` is a fallback, not a geographic name.
2. **Room named** — CML splits that stage into maps and names this room (41 entries, 9 stages). Shown plainly.
3. **Inferred** — code-comment evidence only, medium confidence; always labelled `(inferred)` (4 entries, all `F_SP121`).
4. **Fallback** — `Stage F_SP999 · Room 42`, `Stage Resource` for `STG_vv.arc`. Nothing invented.
5. **Disputed** — sources contradict each other; the alias is withheld and the dispute is shown in the tooltip:
   `F_SP118` (K64 "Arbiter's Grounds" vs CML "Bulblin Camp"), `F_SP127` ("Faron Woods" vs "Fishing Pond"), `R_SP107` ("Twilight Hyrule Castle" vs "Castle Town Sewers"), `R_SP110` ("Death Mountain Dojo" vs "Goron Elder's Hall"), `S_MV000` ("Empty (unused)" vs "Title screen movie map").

Where K64 and CML word a name differently but describe the same place (for example "Hyrule Fields" vs "Hyrule Field", "Hyrule Town" vs "Castle Town"), the CML wording is used and the difference is recorded in `ALIASES.tsv`. Alias keys are `(stage, room)`; the variant (`_00`, `_01`) is displayed but is not part of the name key and is never read as a layer.

## Coverage (from `ALIASES.tsv`; not an in-game or full-catalog guarantee)
- 80 stage ids known to the sources; 71 have a stage-level name; 5 are disputed; 4 (`D_MN11B`, `F_SP117`, `F_SP122`, `R_SP160`) have no single stage name but have room-level names.
- 45 room entries: 41 community-named (9 stages), 4 inferred.
- Whether every stage id in a given CISO is covered is unknown: the importer reported 79 stages and the tables have 80 ids; the sets were not compared.

## Stages and rooms of immediate interest
| Technical id | Label shown | Tier | Basis |
|---|---|---|---|
| F_SP121 / R01_00 | Hyrule Field · Room 01 | Stage named | CML "Hyrule Field"; no room-specific name exists |
| F_SP121 / R02_00 | Hyrule Field · Eldin Gorge south entrance (inferred) · Room 02 | Inferred | SRC d_s_play.cpp:1315, d_com_inf_game.cpp:2901 |
| F_SP121 / R03_00 | Hyrule Field · Room 03 | Stage named | no room-specific name found |
| F_SP121 / R04_00, R05_00 | Hyrule Field · Room 04 / 05 | Stage named | no room-specific name found |
| F_SP113 / R01_00 | Zora's Domain · Room 01 | Stage named | CML lists rooms 0 and 1 under one map; K64 agrees |
| F_SP117 / R01_00 | Sacred Grove · Room 01 | Room named | CML (K64: "Sacred Grove / Temple of Time") |

`F_SP121` rooms 1, 3, 4, 5 remain unidentified at room level: do not infer a region from their coordinates.
