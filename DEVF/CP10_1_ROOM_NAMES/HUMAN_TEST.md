# CP10.1 — visual check (Windows, no gameplay)
Gameplay did not change; do not repeat CP08–CP10 game tests.
1. Close Dusklight. Keep your CP10 `.dusk` pair and `encounters.json` as they are. Back up your current editor EXE, then run `DEVF Map Editor CP10.1.exe` from the TEST zip (it does not install anything).
2. Open "CP10 · edición multi-room QA" → "CISO / Catálogo" → open your catalog. Check the tree: stage nodes show `Name — ID` (for example `Hyrule Field — F_SP121`, `Zora's Domain — F_SP113`), room nodes show `Room nn` (plus a name where one exists, such as `Room 01 — Sacred Grove` under F_SP117). Hover a node to see the full label and its provenance.
3. Select F_SP121 R03 and R01, send each to CP10, then switch between them in the selector (labels like `Hyrule Field · Room 01 [F_SP121/R01_00.arc]`). Your encounters must not change; the list shows `bokoblin-00x · Hyrule Field · Room nn / Layer n`.
Report anything truncated, misleading or wrong. Names marked `(inferred)` are code-comment based; `Room nn` is not a geographic name. Rollback: run the previous editor EXE.
