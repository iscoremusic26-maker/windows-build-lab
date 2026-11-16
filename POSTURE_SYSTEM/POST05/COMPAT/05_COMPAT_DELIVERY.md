# POST05-COMPAT — Entrega (0.5.1, TEST privado)

**Veredicto:** `POST05_COMPAT_PARTIAL_WITH_SAFE_EXCLUSIONS`. Estado runtime: `HUMAN_RUNTIME_PENDING`.

## Artefacto
- **Descarga:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41344244739/artifacts/13098463747. Es un repositorio privado; la API confirma que el id `13098463747` pertenece al run `41344244739`, que se llama `POST05C_TEST_41344244739_1` y que se construyó desde el espejo `a67b0d6`.
- **Run:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41344244739 (`build` e `independent-download` en éxito).
- **Comando:** `gh run download 41344244739 --repo iscoremusic26-maker/codex-cp00-inputs --name POST05C_TEST_41344244739_1`.
- **Contenido del ZIP** (33414 bytes, SHA-256 `86135a52ed8d159da2f5be296fb9104db7ce3fe0635a67804eaf8efa8c0367e1`): `POSTURE_POST05_1_COMPAT_ALL_BOKOBLINS_windows-amd64_ac99b4dfb141.dusk`, `DOWNLOAD_MANIFEST.json` y `SHA256SUMS.txt`. **Instala solo el `.dusk`.**
- **`.dusk`:** **32350 bytes**, SHA-256 **`f5be0ed2e00c196ce261db72756256fd5513229c4370ae4deee62ae6d90b83fb`**.
- **DLL interna:** 65536 bytes, `614207b7178fa8c2b8c03d18a7f0e02fead2eb8c69f99498bae807db5911a951`.
- **Caducidad:** 2026-12-01 19:57 UTC.
- **Fuente compilada:** `ac99b4dfb1413355e3e935e9b22d311c40d84bee` (rama `feature/posture-post05-compat-all-bokoblins`).
- **CI:** rama `ci/posture-post05-compat-all-bokoblins`, cabeza `a67b0d628435fda7cec44e99e201d13603c1a04f`.
- **Línea base reproducible:** POST05 0.5.0 (`a9627a2`, run `41337085240`, artefacto `13095946275`, `.dusk` `f6c1a3c6…db068`) sigue intacto. No se reemplazó ni se tocó su rama CI.

## Runbook
`README_TEST_ALL_VARIANTS.md`: fases A y B, casos T1–T9, criterios de STOP y qué anotar.

## Documentos de este checkpoint (`POSTURE_SYSTEM/POST05/COMPAT/`)
**Procedencia y evidencia de logs:**
- `05_COMPAT_SOURCE_PROVENANCE.md`: incluye la **corrección** de la afirmación de 0.5.0 sobre la fuente de Expansion/EVF;
- `05_POST05_A1_B1_RUNTIME_COMPARISON.md` y `evidence/`.

**Auditorías:**
- `05_VARIANT_IDENTITY_AND_TRANSITIONS.md` (C0);
- `05_EOC2_HIT_PATH_ABI_AUDIT.md` (C1);
- `05_BLUE_DYNAMIC_ADMISSION_AUDIT.md` (C2);
- `05_RED_COMPAT_AUDIT.md` (C3);
- `05_BLACK_COMPAT_AUDIT.md` (C4);
- `05_WRITE_SET_AND_ABI_AUDIT.md` (C5).

**Resultados y gates:**
- `05_COMPAT_GATE_MATRIX.md`;
- `05_COMPAT_TEST_RESULTS.md` y `.json`.

**Herramientas:** `tools/` (extractor, pruebas y mutaciones del extractor).

## Qué NO se hizo
- No se ejecutó el juego ni se cargó la DLL.
- No se instaló nada.
- No se tocó Expansion, EVF, HM, Bars, Zora Swim, Luau ni Randomizer.
- No se hizo PR, merge, tag ni release.
- No se subieron logs crudos.
