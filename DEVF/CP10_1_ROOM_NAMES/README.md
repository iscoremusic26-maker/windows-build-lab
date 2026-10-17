# DEVF CP10.1 — English stage and room names (editor only)

Editor 0.10.1 ("DEVF Map Editor CP10.1"). Shows English names for stages and rooms in the catalog tree, the room selector and the encounter list/details, always next to the technical id (`F_SP121/R01_00.arc`). Layer is shown separately. Rooms without a verified name show `Room 01` and the stage name or `Stage <id>`; `Stage Resource` is used for `STG_xx.arc`. Names, tiers and sources: `NAMES_PROVENANCE.md`, `ALIASES.tsv`.

Not changed: schema1/schema2 JSON, hashes, coordinates, E_OC2 parameters, geometry, validators (including the legacy `CP08_OBSERVED` path that fixed `FLOOR_AMBIGUOUS`), capacity 8, QA modes, ABI, StageService, broker. F_SP113 and F_SP117 remain view-only; naming a resource does not make it editable. No `.dusk` is built: keep the CP10 pair (`DEVF_Provider_0.10.0_TEST.dusk`, `BokoblinExpansion_0.3.0_TEST.dusk`).

Layout: this folder holds only the files that differ from CP10 (`Browser.cs`, `MultiEditor.cs`, `Editor.cs`, `MultiTests.cs` copied and edited, plus `NameResolver.cs`, `NameData.cs`, `NameTests.cs`); `MultiConfig.cs`, `PlacementGeometry.cs`, `Program.cs` and the CP09/CP08 sources are linked unchanged. A separate folder keeps the CP10 files, and their pins in the historical `SOURCE_CI.json`, byte-identical.

Status: build and automated tests run in Windows CI (see the PR). USER_VISUAL_TEST_PENDING.
